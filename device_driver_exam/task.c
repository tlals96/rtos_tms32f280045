#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>

#include "device/device.h"
#include "adc.h"
//#include "device_init.h"

// Grobal Variable
uint16_t loopCounter = 0;

/*
 *  ======== taskFxn ========
 */
Void idle_led_task(UArg a0, UArg a1)
{
    System_printf("enter idle_led_task()\n");
    System_flush(); /* force SysMin output to console */

    Task_sleep(1000);

    GPIO_togglePin(25);

    System_printf("exit idle_led_task()\n");
    System_flush(); /* force SysMin output to console */
}

Void idle_adc_task(UArg a0, UArg a1)
{
}

Void idle_scia_task(UArg a0, UArg a1)
{
    uint16_t receivedChar;
    unsigned char *msg;
    uint16_t rxStatus = 0U;

    msg = "\r\nEnter a character: \0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t *)msg, 22);

    //
    // Read a character from the FIFO.
    //
    receivedChar = SCI_readCharBlockingFIFO(SCIA_BASE);

    rxStatus = SCI_getRxStatus(SCIA_BASE);
    if ((rxStatus & SCI_RXSTATUS_ERROR) != 0)
    {
        //
        //If Execution stops here there is some error
        //Analyze SCI_getRxStatus() API return value
        //
        //ESTOP0;
        //asm("NOP");
    }

    //
    // Echo back the character.
    //
    msg = "  You sent: \0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t *)msg, 13);
    SCI_writeCharBlockingFIFO(SCIA_BASE, receivedChar);

    //
    // Increment the loop count variable.
    //
    loopCounter++;
}
