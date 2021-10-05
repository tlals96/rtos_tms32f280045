/*
 *  ======== main.c ========
 */
#include <ti/sysbios/BIOS.h>

#include "adc.h"
#include "device/device.h"
#include "device/driverlib.h"
#include "task.h"

Void device_init();
Void gpio_init();

/*
 *  ======== main ========
 */
Int main()
{
    // Device init, gpio, adc .. etc
    device_init();

    BIOS_start(); /* does not return */

    return (0);
}

Void device_init()
{
    // Disable pin locks and enable internal pullups.
    Device_initGPIO();

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    //Interrupt_initVectorTable();

    // Gpio25 pin init
    gpio_init();

    //adc initialize
    adc_init();
}

Void gpio_init()
{
    //gpio25 pull-up
    GPIO_setPadConfig(25, GPIO_PIN_TYPE_PULLUP);

    //run_led init
    GPIO_setDirectionMode(25, GPIO_DIR_MODE_OUT);
}
