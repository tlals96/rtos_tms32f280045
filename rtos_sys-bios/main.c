/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>

#include "device/device.h"
#include "device/driverlib.h"

// Grobal Variable
uint16_t loopCounter = 0;

Void gpio_init();

/*
 *  ======== taskFxn ========
 */
Void idle_led_task(UArg a0, UArg a1)
{
    System_printf("enter taskFxn()\n");

    Task_sleep(1000);

    System_printf("exit taskFxn()\n");

    GPIO_togglePin(25);

    System_flush(); /* force SysMin output to console */
}

Void idle_scia_task()
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

/*
 *  ======== main ========
 */
Int main()
{
    // Gpio25 pin init
    gpio_init();

    // Sci A Init
    //sci_init();

    BIOS_start(); /* does not return */
    return (0);
}

Void gpio_init()
{
    //gpio25 pull-up
    GPIO_setPadConfig(25, GPIO_PIN_TYPE_PULLUP);

    //run_led init
    GPIO_setDirectionMode(25, GPIO_DIR_MODE_OUT);
}

Void sci_init()
{
    //
    // GPIO28 is the SCI Rx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // GPIO29 is the SCI Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //
    // Initialize interrupt controller and vector table.
    //
    Interrupt_initModule();
    Interrupt_initVectorTable();

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);

    //
    // Configure SCIA for echoback.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 9600, (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
}
