#ifndef TASK_H_
#define TASK_H_
#include <ti/sysbios/knl/Task.h>

Void idle_led_task(UArg a0, UArg a1);
Void idle_adc_task(UArg a0, UArg a1);
Void idle_scia_task(UArg a0, UArg a1);
// Void device_init_task(UArg a0, UArg a1);
#endif
