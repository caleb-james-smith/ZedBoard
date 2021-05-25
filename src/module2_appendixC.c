#include "stdio.h"
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xuartps_hw.h"

#define VT100_ESC 27 // Tera Term emulates VT100, define the escape code
#define ClearScreen xil_printf("%c[2J",VT100_ESC) // clear screen using VT100 format string
#define CursorHome xil_printf("%c[f",VT100_ESC) // cursor home
#define UART_BASEADDR XPAR_XUARTPS_0_BASEADDR
#define GPIO XPAR_GPIO_0_DEVICE_ID // less typing
#define GPIO0 XPAR_GPIO_0_BASEADDR // less typing
#define LED_CHANNEL 1 // LED resides in channel 1 of GPIO
#define LED_BIT 0x00000001 // LED associated with LSB of GPIO
#define DDR (u32)0b100 // data direction magic word

XGpio Gpio; // Create instance of GPIO structure
volatile u32 shadow_write; // GPIO can be written from multiple sources, use shadow register

#define TMRCTR_DEVICE_ID XPAR_TMRCTR_0_DEVICE_ID // aliases to make reference easier
#define TMRCTR_INTERRUPT_ID XPAR_FABRIC_TMRCTR_0_VEC_ID
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_CNTR_0 0 // the timer contains two physical timers, 0 and 1
#define TIMER_FREQUENCY XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ // clock into timer
#define FBLINK 100.0e3
#define PBLINK 1.0/FBLINK // blink period in seconds
#define RESET_VALUE (u32)(0xffffffff - PBLINK * TIMER_FREQUENCY) // make the magic

// using the interrupt controller built into
// the ZYNC and so giving it an alias that
// resembles the name used for Microblaze code
// if this code is ported to a new processor
#define INTC XScuGic
#define INTC_HANDLER XScuGic_InterruptHandler // less typing later

INTC InterruptController; // The instance of the Interrupt Controller
XTmrCtr TimerCounterInst; // The instance of the Timer Counter

volatile u32 tic; // a flag indicating the timer has rolled over
volatile int error;

int TmrCtrIntrInit(INTC *IntcInstancePtr, XTmrCtr *InstancePtr, u16 DeviceId,
    u16 IntrId, u8 TmrCtrNumber); // initialize the timer

static int TmrCtrSetupIntrSystem(INTC *IntcInstancePtr, XTmrCtr *InstancePtr,
    u16 DeviceId, u16 IntrId, u8 TmrCtrNumber); // initialize the interrupt system

static void TimerCounterHandler(void *CallBackRef, u8 TmrCtrNumber); // timer ISR

int InitGPIO(void); // initialize the GPIO

int main() {
    int i = 0;
    int doblink = 1; // assume we want to blink
    u8 c;
    init_platform();
    ClearScreen;
    CursorHome;
    xil_printf("Hello World\n\r");
    if (InitGPIO() != XST_SUCCESS)
        xil_printf("\r\nThere was a problem with gpio."); // drat

    if (TmrCtrIntrInit(&InterruptController, &TimerCounterInst,
            TMRCTR_DEVICE_ID, TMRCTR_INTERRUPT_ID, TIMER_CNTR_0) != XST_SUCCESS) {
        xil_printf("\r\nError initializing timer & interrupt."); // drat
    } else {
        XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0); // start the timer!
    }

    // because blinking LEDs is typically not time sensitive the code will manage
    // the blink in the main loop instead of directly in the timer ISR
    // the tic counter is checked for exact count or greater
    error = 0;
    while (1) {
        if (XUartPs_IsReceiveData(UART_BASEADDR)) { // is there a char waiting?
            c = toupper((int)XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET));
            switch (c) {
                case 'A':
                    doblink = 1;
                    break;
                case 'B':
                    doblink = 0;
                    break;
                default:
                    break; // ignore
        }
    }
    if (tic >= (u32) FBLINK) {
        tic = 0; // we have recognized the interrupt & are processing
        if (doblink) {
            if (i % 2)
                shadow_write = shadow_write | LED_BIT; // set
            else
                shadow_write = shadow_write & (~LED_BIT); // clear
            Xil_Out32(GPIO0, shadow_write);
        }
        xil_printf("The count is %d\t", i++);
        if (error) {
            xil_printf("err \r");
            error = 0;
        } else
            xil_printf(" \r");
        }
    }
    cleanup_platform(); // never reached
    return 0;
}

void TimerCounterHandler(void *CallBackRef, u8 TmrCtrNumber) {
    static u32 din;
    static u32 dout = 0;

    shadow_write = (shadow_write & ~(u32) 0b10) | dout; // jam test bit into the shadow register
    Xil_Out32(GPIO0, shadow_write); // write the shadow register w/ least overhead
    ++tic; // burns a little time & counts interrupts This prevents read/write/modify errors
    din = (Xil_In32(GPIO0) & (u32) 0b100) >> 1; // read back, mask and shift
    if (din != dout) // bits are the same?
        error = 1; // flag error
    dout = dout ^ (u32) 0b10; // flip our test bit
}

int InitGPIO(void) {
    int Status;
    Status = XGpio_Initialize(&Gpio, GPIO); // initialize control of GPIO
    if (Status != XST_SUCCESS) {
        xil_printf("\r\nGPIO init failed\r\n");
        return (XST_FAILURE); // early exit
    }
    XGpio_SetDataDirection(&Gpio, LED_CHANNEL, DDR); // set just bit 0 as output
    shadow_write = 0;
    Xil_Out32(GPIO0, shadow_write);
    return (Status);
}

int TmrCtrIntrInit(INTC *IntcInstancePtr, XTmrCtr *TmrCtrInstancePtr,
        u16 DeviceId, u16 IntrId, u8 TmrCtrNumber) {
    int Status;

    // Initialize the timer counter so that it's ready to use,
    // specify the device ID that is generated in xparameters.h
    Status = XTmrCtr_Initialize(TmrCtrInstancePtr, DeviceId);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    // Perform a self-test to ensure that the hardware was built
    // correctly, use the 1st timer in the device (0)
    Status = XTmrCtr_SelfTest(TmrCtrInstancePtr, TmrCtrNumber);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Connect the timer counter to the interrupt subsystem such that
    // interrupts can occur. This function is application specific.
    Status = TmrCtrSetupIntrSystem(IntcInstancePtr, TmrCtrInstancePtr, DeviceId,
                IntrId, TmrCtrNumber);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Setup the handler for the timer counter that will be called from the
    // interrupt context when the timer expires, specify a pointer to the
    // timer counter driver instance as the callback reference so the
    // handler is able to access the instance data
    XTmrCtr_SetHandler(TmrCtrInstancePtr, TimerCounterHandler,
                        TmrCtrInstancePtr);

    // Enable the interrupt of the timer counter so interrupts will occur
    // and use auto reload mode such that the timer counter will reload
    // itself automatically and continue repeatedly, without this option
    // it would expire once only
    XTmrCtr_SetOptions(TmrCtrInstancePtr, TmrCtrNumber,
                        XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

    // Set a reset value for the timer counter such that it will expire
    // eariler than letting it roll over from 0, the reset value is loaded
    // into the timer counter when it is started
    XTmrCtr_SetResetValue(TmrCtrInstancePtr, TmrCtrNumber, RESET_VALUE);
    return XST_SUCCESS;

}

static int TmrCtrSetupIntrSystem(INTC *IntcInstancePtr,
        XTmrCtr *TmrCtrInstancePtr, u16 DeviceId, u16 IntrId, u8 TmrCtrNumber) {
    int Status;
    XScuGic_Config *IntcConfig;

    // Initialize the interrupt controller driver so that it is ready to use.
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (NULL == IntcConfig) {
        return XST_FAILURE;
    }

    Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
    IntcConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    XScuGic_SetPriorityTriggerType(IntcInstancePtr, IntrId, 0xA0, 0x3);

    // Connect the interrupt handler that will be called when an
    // interrupt occurs for the device.
    Status = XScuGic_Connect(IntcInstancePtr, IntrId,
    (Xil_ExceptionHandler) XTmrCtr_InterruptHandler, TmrCtrInstancePtr);
    if (Status != XST_SUCCESS) {
        return Status;
    }

    // Enable the interrupt for the Timer device.
    XScuGic_Enable(IntcInstancePtr, IntrId);

    // Initialize the exception table.
    Xil_ExceptionInit();

    //Register the interrupt controller handler with the exception table.
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)
        INTC_HANDLER, IntcInstancePtr);
    Xil_ExceptionEnable(); // Enable non-critical exceptions.
    return XST_SUCCESS;
}
