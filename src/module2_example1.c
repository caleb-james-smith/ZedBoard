#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"

int main()
{
    int i=0;
    init_platform();

    xil_printf("Hello World\n\r");

    while(1)
    {
        xil_printf("The count is %d \r", i++);
        usleep(500000);
    }

    cleanup_platform();
    return 0;
}
