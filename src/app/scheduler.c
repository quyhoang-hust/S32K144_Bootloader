#include "scheduler.h"

#include "boot_tasks.h"
#include "error_handler.h"

void Scheduler_Run(void)
{
    for (;;)
    {

        ErrorHandler_ProcessPending();

        Task_MenuProcess();
        Task_SrecRxProcess();
        Task_FlashProgram();
        Task_ReportStatistic();
    }
}
