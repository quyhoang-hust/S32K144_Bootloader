#include "boot_statistic.h"

static BootStatisticType g_bootStatistic;

void BootStatistic_Init(void)
{
    g_bootStatistic.receivedDataRecordCount = 0u;
    g_bootStatistic.receivedEndRecordCount = 0u;
    g_bootStatistic.completedLineCount = 0u;
    g_bootStatistic.menuCommandCount = 0u;
    g_bootStatistic.invalidMenuCommandCount = 0u;
}

void BootStatistic_ResetDownload(void)
{
    g_bootStatistic.receivedDataRecordCount = 0u;
    g_bootStatistic.receivedEndRecordCount = 0u;
    g_bootStatistic.completedLineCount = 0u;
}

void BootStatistic_IncrementDataRecordCount(void)
{
    g_bootStatistic.receivedDataRecordCount++;
}

void BootStatistic_IncrementEndRecordCount(void)
{
    g_bootStatistic.receivedEndRecordCount++;
}

void BootStatistic_IncrementCompletedLineCount(void)
{
    g_bootStatistic.completedLineCount++;
}

void BootStatistic_IncrementMenuCommandCount(void)
{
    g_bootStatistic.menuCommandCount++;
}

void BootStatistic_IncrementInvalidMenuCommandCount(void)
{
    g_bootStatistic.invalidMenuCommandCount++;
}

BootStatisticType BootStatistic_GetSnapshot(void)
{
    return g_bootStatistic;
}
