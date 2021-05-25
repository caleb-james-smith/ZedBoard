#include "stdio.h"
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xgpio.h"

#define GPIO XPAR_GPIO_0_DEVICE_ID  // less typing
#define LED_CHANNEL 1               // LED resides in channel 1 of GPIO
#define LED_BIT 0x00000001          // LED associated with LSB of GPIO
XGpio Gpio;                         // Create instance of GPIO structure

int InitGPIO(void);

int main()
{
    int i = 0;
    init_platform();

    xil_printf("Hello World\n\r");

    if (InitGPIO() != XST_SUCCESS)
        xil_printf("\r\nThere was a problem with gpio.");

    while(1)
    {
    	xil_printf("The count is %d \r", i++);
    	XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, LED_BIT); // on
    	usleep(200000);
    	XGpio_DiscreteClear(&Gpio, LED_CHANNEL, LED_BIT); // off
    	usleep(200000);
    }

    cleanup_platform();
    return 0;
}

int InitGPIO(void) {
    int Status;
    Status = XGpio_Initialize(&Gpio, GPIO); // initialize control of GPIO
    if (Status != XST_SUCCESS) {
    	xil_printf("\r\nGPIO init failed\r\n");
    	return (XST_FAILURE); // early exit
    }
    XGpio_SetDataDirection(&Gpio, LED_CHANNEL, ~LED_BIT); // set just bit 0 as output
    return (Status);
}
