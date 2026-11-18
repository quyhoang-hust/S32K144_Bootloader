#include "flash_job_queue.h"

#include <stddef.h>
#include <string.h>

#include "compiler_abstraction.h"

BOOT_STATIC_ASSERT(
    FLASH_JOB_QUEUE_SIZE > 1u,
    "FLASH_JOB_QUEUE_SIZE must be greater than one");

BOOT_STATIC_ASSERT(
    FLASH_JOB_MAX_DATA_SIZE > 0u,
    "FLASH_JOB_MAX_DATA_SIZE must be greater than zero");

BOOT_STATIC_ASSERT(
    FLASH_JOB_MAX_DATA_SIZE <= UINT16_MAX,
    "Flash program-data length type is too small");

static FlashJobTypeData
    g_jobStorage[FLASH_JOB_QUEUE_SIZE];

static uint32_t g_headIndex = 0u;

static uint32_t g_tailIndex = 0u;

static uint32_t g_jobCount = 0u;

/*
 * Peak occupancy since Init().
 */
static uint32_t g_peakUsage = 0u;

static uint32_t FlashJobQueue_GetNextIndex(
    uint32_t currentIndex)
{
    currentIndex++;

    if (currentIndex >= FLASH_JOB_QUEUE_SIZE)
    {
        currentIndex = 0u;
    }

    return currentIndex;
}

static void FlashJobQueue_ClearEntry(
    FlashJobTypeData *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->type = FLASH_JOB_NONE;

    entry->programData.address = 0u;

    entry->programData.length = 0u;

    (void)memset(
        entry->programData.data,
        0,
        sizeof(entry->programData.data));
}

static bool FlashJobQueue_PushJob(
    const FlashJobTypeData *job)
{
    if (job == NULL)
    {
        return false;
    }

    if (g_jobCount >= FLASH_JOB_QUEUE_SIZE)
    {
        return false;
    }

    /*
     * Queue is accessed only from
     * scheduler context.
     *
     * No locking required.
     */
    g_jobStorage[g_tailIndex] = *job;

    COMPILER_MEMORY_BARRIER();

    g_tailIndex =
        FlashJobQueue_GetNextIndex(
            g_tailIndex);

    g_jobCount++;

    if (g_jobCount > g_peakUsage)
    {
        g_peakUsage = g_jobCount;
    }

    return true;
}

void FlashJobQueue_Init(void)
{
    uint32_t index;

    g_headIndex = 0u;

    g_tailIndex = 0u;

    g_jobCount = 0u;

    g_peakUsage = 0u;

    for (index = 0u;
         index < FLASH_JOB_QUEUE_SIZE;
         index++)
    {
        FlashJobQueue_ClearEntry(
            &g_jobStorage[index]);
    }
}

void FlashJobQueue_Clear(void)
{
    FlashJobQueue_Init();
}

bool FlashJobQueue_PushEraseApp(void)
{
    FlashJobTypeData job;

    FlashJobQueue_ClearEntry(&job);

    job.type = FLASH_JOB_ERASE_APP;

    return FlashJobQueue_PushJob(&job);
}

bool FlashJobQueue_PushProgramData(
    uint32_t address,
    const uint8_t *data,
    uint16_t length)
{
    FlashJobTypeData job;

    if (data == NULL)
    {
        return false;
    }

    if ((length == 0u) ||
        (length > FLASH_JOB_MAX_DATA_SIZE))
    {
        return false;
    }

    FlashJobQueue_ClearEntry(&job);

    job.type = FLASH_JOB_PROGRAM_DATA;

    job.programData.address = address;

    job.programData.length = length;

    (void)memcpy(
        job.programData.data,
        data,
        length);

    return FlashJobQueue_PushJob(&job);
}

bool FlashJobQueue_PushEndDownload(void)
{
    FlashJobTypeData job;

    FlashJobQueue_ClearEntry(&job);

    job.type = FLASH_JOB_END_DOWNLOAD;

    return FlashJobQueue_PushJob(&job);
}

bool FlashJobQueue_Peek(
    FlashJobTypeData *job)
{
    if (job == NULL)
    {
        return false;
    }

    if (g_jobCount == 0u)
    {
        return false;
    }

    *job = g_jobStorage[g_headIndex];

    return true;
}

bool FlashJobQueue_Pop(void)
{
    if (g_jobCount == 0u)
    {
        return false;
    }

    FlashJobQueue_ClearEntry(
        &g_jobStorage[g_headIndex]);

    g_headIndex =
        FlashJobQueue_GetNextIndex(
            g_headIndex);

    g_jobCount--;

    return true;
}

bool FlashJobQueue_IsEmpty(void)
{
    return (g_jobCount == 0u);
}

bool FlashJobQueue_IsFull(void)
{
    return (g_jobCount ==
            FLASH_JOB_QUEUE_SIZE);
}

uint32_t FlashJobQueue_GetUsage(void)
{
    return g_jobCount;
}

uint32_t FlashJobQueue_GetCapacity(void)
{
    return FLASH_JOB_QUEUE_SIZE;
}

uint32_t FlashJobQueue_GetPeakUsage(void)
{
    return g_peakUsage;
}
