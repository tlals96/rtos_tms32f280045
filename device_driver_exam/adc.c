/**
 * 
 * @brief ADC Conversion with DMA
 * @details Using EPWM2 trigger for adc conversion
 * @author tlals96
 * @date 2021-09-29
 * @version 0.0.1
 * 
*/

#include "adc.h"
#include "device/device.h"

__interrupt void adcAISR(void);
__interrupt void adcCISR(void);
__interrupt void dmach1ISR(void);
__interrupt void dmach2ISR(void);
void configureADC(uint32_t adcBase);
void configureEPWM(uint32_t epwmBase);
void setup_adc_continuous(uint32_t adcBase);
void initializeDMA(void);
void configureDMAChannels(void);

uint16_t adca_buf[BUFF_SIZE] = {
    0,
};
uint16_t adcc_buf[BUFF_SIZE] = {
    0,
};

/**
 * 
 * @brief average to adc data
 * @details none
 * @param uint16_t *adc_buffer adc buffer
 * @param uint16_t *avg_buffer result buffer
 * @return none
 * @throw none
 * 
*/
void adc_average(uint16_t *adc_buffer, uint16_t *avg_buffer)
{
    int i, j;
    uint32_t sum = 0, result = 0;

    for (i = 0; i < ADC_NUM; i++)
    {
        for (j = i; j < BUFF_SIZE; j += ADC_NUM)
        {
            sum += *(adc_buffer + j);
        }

        result = sum / (BUFF_SIZE / ADC_NUM);
        *(avg_buffer + i) = result;

        result = 0;
        sum = 0;
    }
}

void adc_init()
{
    //
    // Set up ISRs used by this example
    //
    // ISR for ADCA INT1 - occurs after first conversion
    // ISR for DMA ch1 - occurs when DMA transfer is complete
    //
    Interrupt_register(INT_ADCA1, &adcAISR);
    Interrupt_register(INT_DMA_CH1, &dmach1ISR);

    //
    // Set up ISRs used by this example
    //
    // ISR for ADCC INT1 - occurs after first conversion
    // ISR for DMA ch2 - occurs when DMA transfer is complete
    //
    Interrupt_register(INT_ADCC1, &adcCISR);
    Interrupt_register(INT_DMA_CH2, &dmach2ISR);

    //
    // Enable specific PIE & CPU interrupts:
    // ADCA INT1 - Group 1, interrupt 1
    // DMA interrupt - Group 7, interrupt 1
    //
    Interrupt_enable(INT_ADCA1);
    Interrupt_enable(INT_ADCC1);

    Interrupt_enable(INT_DMA_CH1);
    Interrupt_enable(INT_DMA_CH2);

    //
    // Stop the ePWM clock
    //
    // SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Call the set up function for ePWM 2
    //
    configureEPWM(EPWM2_BASE);

    //
    // Start the ePWM clock
    //
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Enable internal reference on ADCs
    //
    ADC_setVREF(ADCA_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    ADC_setVREF(ADCB_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);
    ADC_setVREF(ADCC_BASE, ADC_REFERENCE_INTERNAL, ADC_REFERENCE_3_3V);

    //
    // Configure the ADCA & ADCC and power it up
    //
    configureADC(ADCA_BASE);
    configureADC(ADCC_BASE);

    //
    // Setup the ADC for continuous conversions on channels A3 and C3
    //
    setup_adc_continuous(ADCA_BASE);
    setup_adc_continuous(ADCC_BASE);

    //
    // Initialize the DMA & configure DMA channels 1 & 2
    //
    initializeDMA();
    configureDMAChannels();

    //
    // Clearing all pending interrupt flags
    //
    DMA_clearTriggerFlag(DMA_CH1_BASE);                         // DMA channel 1
    DMA_clearTriggerFlag(DMA_CH2_BASE);                         // DMA channel 2
    HWREGH(ADCA_BASE + ADC_O_INTFLGCLR) = 0x3U;                 // ADCA
    HWREGH(ADCC_BASE + ADC_O_INTFLGCLR) = 0x3U;                 // ADCC
    EPWM_forceADCTriggerEventCountInit(EPWM2_BASE, EPWM_SOC_A); // EPWM2 SOCA
    EPWM_clearADCTriggerFlag(EPWM2_BASE, EPWM_SOC_A);           // EPWM2 SOCA

    //
    // Enable continuous operation by setting the last SOC to re-trigger
    // the first
    //
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER0, // ADCA
                               ADC_INT_SOC_TRIGGER_ADCINT2);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER0, // ADCC
                               ADC_INT_SOC_TRIGGER_ADCINT2);

    //
    // Enable global Interrupts and higher priority real-time debug events:
    //
    EINT; // Enable Global interrupt INTM
    ERTM; // Enable Global realtime interrupt DBGM

    //
    // Start DMA
    //
    DMA_startChannel(DMA_CH1_BASE);
    DMA_startChannel(DMA_CH2_BASE);

    //
    // Finally, enable the SOCA trigger from ePWM. This will kick off
    // conversions at the next ePWM event.
    //
    EPWM_enableADCTrigger(EPWM2_BASE, EPWM_SOC_A);
}

// configureADC - Write ADC configurations and power up the ADC for both
//                ADC A and ADC C
//
void configureADC(uint32_t adcBase)
{
    //
    // Set ADCCLK divider to /2
    //
    ADC_setPrescaler(adcBase, ADC_CLK_DIV_2_0);

    //
    // Set pulse positions to late
    //
    ADC_setInterruptPulseMode(adcBase, ADC_PULSE_END_OF_CONV);

    //
    // Power up the ADCs and then delay for 1 ms
    //
    ADC_enableConverter(adcBase);

    //
    // Delay for 1ms to allow ADC time to power up
    //
    //DEVICE_DELAY_US(1000);
}

//
// configureEPWM - Set up the ePWM2 module so that the A output has a period
//                 of 40us with a 50% duty. The SOCA signal is coincident with
//                 the rising edge of this.
//
void configureEPWM(uint32_t epwmBase)
{
    //
    // Make the timer count up with a period of 40us
    //
    HWREGH(epwmBase + EPWM_O_TBCTL) = 0x0000U;
    EPWM_setTimeBasePeriod(epwmBase, 4000U);

    //
    // Set the A output on zero and reset on CMPA
    //
    EPWM_setActionQualifierAction(epwmBase, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(epwmBase, EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);

    //
    // Set CMPA to 20us to get a 50% duty
    //
    EPWM_setCounterCompareValue(epwmBase, EPWM_COUNTER_COMPARE_A, 2000U);

    //
    // Start ADC when timer equals zero (note: don't enable yet)
    //
    EPWM_setADCTriggerSource(epwmBase, EPWM_SOC_A, EPWM_SOC_TBCTR_ZERO);
    EPWM_setADCTriggerEventPrescale(epwmBase, EPWM_SOC_A, 1U);

    //
    // Enable initialization of the SOCA event counter. Since we are
    // disabling the ETSEL.SOCAEN bit, we need a way to reset the SOCACNT.
    // Hence, enable the counter initialize control.
    //
    EPWM_enableADCTriggerEventCountInit(epwmBase, EPWM_SOC_A);
}

void setup_adc_continuous(uint32_t adcBase)
{
    uint16_t acqps;

    //
    // Determine minimum acquisition window (in SYSCLKS)
    //
    acqps = 8; // 80ns
    //
    // - NOTE: A longer sampling window will be required if the ADC driving
    //   source is less than ideal (an ideal source would be a high bandwidth
    //   op-amp with a small series resistance). See TI application report
    //   SPRACT6 for guidance on ADC driver design.
    //

    //
    // Configure SOCs channel no. & acquisition window.
    // Trigger SCO0 from EPWM2SOCA.
    // Trigger all other SOCs from INT1 (EOC on SOC0).
    //
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM2_SOCA,
                 (ADC_Channel)0, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER1, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)1, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER2, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)2, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER3, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)3, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER4, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)4, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER5, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)5, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER6, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)6, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER7, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)7, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER8, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)8, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER9, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)9, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER10, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)10, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER11, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)11, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER12, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)12, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER13, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)13, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER14, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)14, acqps);
    ADC_setupSOC(adcBase, ADC_SOC_NUMBER15, ADC_TRIGGER_SW_ONLY,
                 (ADC_Channel)15, acqps);

    //
    // Configure ADCINT1 trigger for SOC1-SOC15
    //
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER1,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER2,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER3,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER4,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER5,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER6,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER7,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER8,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER9,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER10,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER11,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER12,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER13,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER14,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(adcBase, ADC_SOC_NUMBER15,
                               ADC_INT_SOC_TRIGGER_ADCINT1);

    //
    // Enable ADCINT1 & ADCINT2. Disable ADCINT3 & ADCINT4.
    //
    ADC_enableInterrupt(adcBase, ADC_INT_NUMBER1);
    ADC_enableInterrupt(adcBase, ADC_INT_NUMBER2);
    ADC_disableInterrupt(adcBase, ADC_INT_NUMBER3);
    ADC_disableInterrupt(adcBase, ADC_INT_NUMBER4);

    //
    // Enable continuous mode
    //
    ADC_enableContinuousMode(adcBase, ADC_INT_NUMBER1);
    ADC_enableContinuousMode(adcBase, ADC_INT_NUMBER2);

    //
    // Configure interrupt triggers
    //
    ADC_setInterruptSource(adcBase, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_setInterruptSource(adcBase, ADC_INT_NUMBER2, ADC_SOC_NUMBER15);
}

//
// initializeDMA - Initialize DMA through hard reset
//
void initializeDMA(void)
{
    //
    // Perform a hard reset on DMA
    //
    DMA_initController();

    //
    // Allow DMA to run free on emulation suspend
    //
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);
}

//
// configureDMAChannels - Initialize DMA ch 1 to transfer ADCA results
//                        and DMA ch 2 to transfer ADCB results
//
void configureDMAChannels(void)
{
    //
    // DMA channel 1 set up for ADCA
    //
    DMA_configAddresses(DMA_CH1_BASE, (uint16_t *)&adca_buf,
                        (uint16_t *)ADCARESULT_BASE);

    //
    // Perform enough 16-word bursts to fill the results buffer. Data will be
    // transferred 16 bits at a time hence the address steps below.
    DMA_configBurst(DMA_CH1_BASE, ADC_NUM, 1, 1);
    DMA_configTransfer(DMA_CH1_BASE, BUFF_SIZE / ADC_NUM, -15, 1);
    DMA_configMode(DMA_CH1_BASE, DMA_TRIGGER_ADCA2,
                   (DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE |
                    DMA_CFG_SIZE_16BIT));

    DMA_enableTrigger(DMA_CH1_BASE);
    DMA_disableOverrunInterrupt(DMA_CH1_BASE);
    DMA_setInterruptMode(DMA_CH1_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA_CH1_BASE);

    //
    // DMA channel 2 set up for ADCC
    //
    DMA_configAddresses(DMA_CH2_BASE, (uint16_t *)&adcc_buf,
                        (uint16_t *)ADCCRESULT_BASE);

    //
    // Perform enough 16-word bursts to fill the results buffer. Data will be
    // transferred 16 bits at a time hence the address steps below.
    //
    DMA_configBurst(DMA_CH2_BASE, ADC_NUM, 1, 1);
    DMA_configTransfer(DMA_CH2_BASE, BUFF_SIZE / ADC_NUM, -15, 1);
    DMA_configMode(DMA_CH2_BASE, DMA_TRIGGER_ADCC2,
                   (DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE |
                    DMA_CFG_SIZE_16BIT));

    DMA_enableTrigger(DMA_CH2_BASE);
    DMA_disableOverrunInterrupt(DMA_CH2_BASE);
    DMA_setInterruptMode(DMA_CH2_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA_CH2_BASE);
}

//
// adcA1ISR - This is called after the very first conversion and will disable
//             the ePWM SOC to avoid re-triggering problems.
//
// #pragma CODE_SECTION(adcAISR, ".TI.ramfunc");
__interrupt void adcAISR(void)
{
    //
    // Acknowledge interrupt
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//
// adcC1ISR - This is called after the very first conversion and will disable
//             the ePWM SOC to avoid re-triggering problems.
//
// #pragma CODE_SECTION(adcCISR, ".TI.ramfunc");
__interrupt void adcCISR(void)
{
    // Acknowledge interrupt
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//
// dmach1ISR - This is called at the end of the DMA transfer, the conversions
//              are stopped by removing the trigger of the first SOC from
//              the last.
//
// #pragma CODE_SECTION(dmach1ISR, ".TI.ramfunc");
__interrupt void dmach1ISR(void)
{
    //
    // Stop the ADC by removing the trigger for SOC0
    //
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER0,
                               ADC_INT_SOC_TRIGGER_NONE);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER1,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER2,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER3,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER4,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER5,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER6,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER7,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER8,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER9,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER10,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER11,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER12,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER13,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER14,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER15,
                               ADC_INT_SOC_TRIGGER_ADCINT1);

    //
    // Acknowledge interrupt
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
}

//
// dmach2ISR - This is called at the end of the DMA transfer, the conversions
//              are stopped by removing the trigger of the first SOC from
//              the last.
//
// #pragma CODE_SECTION(dmach2ISR, ".TI.ramfunc");
__interrupt void dmach2ISR(void)
{
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER0,
                               ADC_INT_SOC_TRIGGER_NONE);
    ADC_setInterruptSOCTrigger(ADCA_BASE, ADC_SOC_NUMBER1,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER2,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER3,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER4,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER5,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER6,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER7,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER8,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER9,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER10,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER11,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER12,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER13,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER14,
                               ADC_INT_SOC_TRIGGER_ADCINT1);
    ADC_setInterruptSOCTrigger(ADCC_BASE, ADC_SOC_NUMBER15,
                               ADC_INT_SOC_TRIGGER_ADCINT1);

    //
    // Acknowledge interrupt
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
}
