#ifndef BOOT_STATISTIC_H
#define BOOT_STATISTIC_H

#include <stdint.h>

typedef struct
{

    uint32_t receivedDataRecordCount;

    uint32_t receivedEndRecordCount;

    uint32_t completedLineCount;

    uint32_t menuCommandCount;

    uint32_t invalidMenuCommandCount;

} BootStatisticType;

void BootStatistic_Init(void);

void BootStatistic_ResetDownload(void);

void BootStatistic_IncrementDataRecordCount(void);

void BootStatistic_IncrementEndRecordCount(void);

void BootStatistic_IncrementCompletedLineCount(void);

void BootStatistic_IncrementMenuCommandCount(void);

void BootStatistic_IncrementInvalidMenuCommandCount(void);

BootStatisticType BootStatistic_GetSnapshot(void);

#endif
