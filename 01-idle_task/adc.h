#ifndef ADC_H_
#define ADC_H_
#include "device/device.h"

#define BUFF_SIZE 1024
#define ADC_NUM 16

extern uint16_t adca_buf[BUFF_SIZE];
extern uint16_t adcc_buf[BUFF_SIZE];

void adc_average(uint16_t *adc_buffer, uint16_t *avg_buffer);
void adc_init();
#endif
