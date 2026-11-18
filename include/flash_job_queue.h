#ifndef FLASH_JOB_QUEUE_H
#define FLASH_JOB_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_config.h"

typedef enum
{
    FLASH_JOB_NONE = 0,

    FLASH_JOB_ERASE_APP,

    FLASH_JOB_PROGRAM_DATA,

    FLASH_JOB_END_DOWNLOAD

} FlashJobType;

typedef struct
{
    uint32_t address;

    uint16_t length;

    uint8_t data[FLASH_JOB_MAX_DATA_SIZE];

} FlashProgramDataType;

typedef struct
{
    FlashJobType type;

    FlashProgramDataType programData;

} FlashJobTypeData;

void FlashJobQueue_Init(void);

void FlashJobQueue_Clear(void);

bool FlashJobQueue_PushEraseApp(void);

bool FlashJobQueue_PushProgramData(
    uint32_t address,
    const uint8_t *data,
    uint16_t length);

bool FlashJobQueue_PushEndDownload(void);

bool FlashJobQueue_Peek(
    FlashJobTypeData *job);

bool FlashJobQueue_Pop(void);

bool FlashJobQueue_IsEmpty(void);

bool FlashJobQueue_IsFull(void);

uint32_t FlashJobQueue_GetUsage(void);

uint32_t FlashJobQueue_GetCapacity(void);

uint32_t FlashJobQueue_GetPeakUsage(void);

#endif
