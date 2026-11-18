#ifndef FLASH_ENGINE_H
#define FLASH_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    FLS_SREC_JOB_DONE = 0,

    FLS_SREC_JOB_PROGRESSING

} FlsSrecJobStateType;

typedef enum
{

    FLS_JOB_DONE = 0,

    FLS_JOB_BUSY

} FlsPhraseJobStateType;


typedef struct
{

    uint32_t totalSectorCount;

    uint32_t erasedSectorCount;

    uint32_t programmedDataBytes;

    uint32_t programmedPhraseCount;

} FlashEngineStatisticType;

void FlashEngine_Init(void);

bool FlashEngine_BeginDownloadSession(void);

void FlashEngine_MainFunction(void);

bool FlashEngine_IsOperationActive(void);

FlashEngineStatisticType FlashEngine_GetStatisticSnapshot(void);

uint32_t FlashEngine_GetEraseProgressPercent(void);

FlsSrecJobStateType FlashEngine_GetSrecJobState(void);

FlsPhraseJobStateType FlashEngine_GetPhraseJobState(void);

bool FlashEngine_IsPhraseSubmitted(void);

uint32_t FlashEngine_GetExpectedNextAddress(void);

#endif
