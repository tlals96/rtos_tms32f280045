/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <xdc/cfg/global.h>

// 2 task run with semaphore
Void task1(UArg arg0, UArg arg1);
Void task2(UArg arg0, UArg arg1);

Int run_count = 0;

// 2 task handler
Task_Handle tsk1;
Task_Handle tsk2;

/*
 *  ======== main ========
 */
Int main()
{
    BIOS_start(); /* does not return */
    return (0);
}

/*
 *  ======== task1 ========
 */
Void task1(UArg a0, UArg a1)
{
    UInt32 time;

    for (;;)
    {
        System_printf("Running task1\n");

        if (Semaphore_getCount(sem) == 0)
        {
            System_printf("Semaphore blocked in task1\n");
        }

         System_flush();

        /* Get access to resource */
        Semaphore_pend(sem, BIOS_WAIT_FOREVER);

        /* do work by waiting for 2 system ticks to pass */
        time = Clock_getTicks();
        while (Clock_getTicks() <= (time + 1))
        {
            ;
        }

        /* do work on locked resource */
        run_count += 1;

        /* unlock resource */
        Semaphore_post(sem);

        // sleep time 100ms
        Task_sleep(100);
    }
}

/*
 *  ======== task2 ========
 */
Void task2(UArg a0, UArg a1)
{
    for (;;)
    {
        System_printf("Running task2\n");

        if (Semaphore_getCount(sem) == 0)
        {
            System_printf("Semaphore blocked in task1\n");
        }

        System_flush();

        /* Get access to resource */
        Semaphore_pend(sem, BIOS_WAIT_FOREVER);

        /* do work on locked resource */
        run_count += 1;

        /* unlock resource */
        Semaphore_post(sem);

        // sleep time 100ms
        Task_sleep(100);
    }
}
