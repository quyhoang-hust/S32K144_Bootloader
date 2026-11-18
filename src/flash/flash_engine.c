#include "flash_engine.h"

#include <stddef.h>
#include <string.h>

#include "boot_state_machine.h"
#include "bootloader_config.h"
#include "error_handler.h"
#include "flash_driver.h"
#include "flash_job_queue.h"
#include "memory_map.h"

typedef enum
{
    FLASH_ENGINE_MODE_IDLE = 0,

    FLASH_ENGINE_MODE_ERASE,

    FLASH_ENGINE_MODE_ERASE_COMPLETE,

    FLASH_ENGINE_MODE_DOWNLOAD

} FlashEngineModeType;


typedef struct
{

    uint32_t baseAddress;

    uint8_t data[FLASH_PHRASE_SIZE];

    uint8_t nextFillOffset;

    uint8_t actualDataByteCount;

    bool isSubmitted;

} FlashActivePhraseType;


typedef struct
{
    bool hasCurrentJob;

    FlashJobTypeData currentJob;

    uint16_t currentJobOffset;

    FlsSrecJobStateType srecJobState;

} FlashCurrentJobContextType;



static FlashEngineModeType g_engineMode = FLASH_ENGINE_MODE_IDLE;

static FlashEngineStatisticType g_statistic;

static FlashCurrentJobContextType g_jobContext;

static FlashActivePhraseType g_activePhrase;

static FlsPhraseJobStateType g_phraseState = FLS_JOB_DONE;

static bool g_expectedNextAddressValid = false;

static uint32_t g_expectedNextAddress = 0u;

static bool g_endDownloadPending = false;

static uint32_t g_nextEraseAddress = APPLICATION_START_ADDRESS;


static void FlashEngine_ResetCurrentJob(void)
{
    g_jobContext.hasCurrentJob = false;

    (void)memset(
        &g_jobContext.currentJob,
        0,
        sizeof(g_jobContext.currentJob));

    g_jobContext.currentJob.type = FLASH_JOB_NONE;
    g_jobContext.currentJobOffset = 0u;
    g_jobContext.srecJobState = FLS_SREC_JOB_DONE;
}

static void FlashEngine_ResetActivePhrase(void)
{
    g_activePhrase.baseAddress = 0u;

    (void)memset(
        g_activePhrase.data,
        0xFF,
        sizeof(g_activePhrase.data));

    g_activePhrase.nextFillOffset = 0u;
    g_activePhrase.actualDataByteCount = 0u;
    g_activePhrase.isSubmitted = false;

    g_phraseState = FLS_JOB_DONE;
}

static void FlashEngine_ResetStatistic(void)
{
    g_statistic.totalSectorCount = APP_SECTOR_COUNT;
    g_statistic.erasedSectorCount = 0u;
    g_statistic.programmedDataBytes = 0u;
    g_statistic.programmedPhraseCount = 0u;
}

static bool FlashEngine_IsPhraseBusy(void)
{
    return (g_phraseState == FLS_JOB_BUSY);
}

static bool FlashEngine_IsPhraseFillable(void)
{
    return (FlashEngine_IsPhraseBusy() &&
            !g_activePhrase.isSubmitted);
}

static bool FlashEngine_IsPhraseFull(void)
{
    return (FlashEngine_IsPhraseBusy() &&
            (g_activePhrase.nextFillOffset >= FLASH_PHRASE_SIZE));
}

static void FlashEngine_SetDriverStartError(
    FlashDriverStatusType driverStatus,
    uint32_t address,
    BootErrorCodeType normalFailureCode)
{
    switch (driverStatus)
    {
        case FLASH_DRIVER_STATUS_ADDRESS_VIOLATION:
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_FLASH_DRIVER_ADDRESS_VIOLATION,
                address);
            break;
        }

        case FLASH_DRIVER_STATUS_ALIGNMENT_VIOLATION:
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_FLASH_DRIVER_ALIGNMENT_VIOLATION,
                address);
            break;
        }

        case FLASH_DRIVER_STATUS_UNSUPPORTED_COMMAND:
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_UNSUPPORTED_FLASH_COMMAND,
                address);
            break;
        }

        case FLASH_DRIVER_STATUS_NOT_READY:
        case FLASH_DRIVER_STATUS_FAILED:
        default:
        {
            ErrorHandler_SetFatal(normalFailureCode, address);
            break;
        }
    }
}

static bool FlashEngine_StartPhrase(uint32_t firstDataAddress)
{
    uint32_t phraseBase;
    uint32_t leadingPadding;

    if (FlashEngine_IsPhraseBusy())
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            firstDataAddress);

        return false;
    }

    phraseBase = MemoryMap_GetPhraseBaseAddress(firstDataAddress);

    if (!MemoryMap_IsApplicationPhraseValid(phraseBase))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_ADDRESS_OUT_OF_RANGE,
            firstDataAddress);

        return false;
    }

    leadingPadding = firstDataAddress - phraseBase;

    if (leadingPadding >= FLASH_PHRASE_SIZE)
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            firstDataAddress);

        return false;
    }

    (void)memset(
        g_activePhrase.data,
        0xFF,
        sizeof(g_activePhrase.data));

    g_activePhrase.baseAddress = phraseBase;
    g_activePhrase.nextFillOffset = (uint8_t)leadingPadding;
    g_activePhrase.actualDataByteCount = 0u;
    g_activePhrase.isSubmitted = false;

    g_phraseState = FLS_JOB_BUSY;

    return true;
}

static void FlashEngine_PadPhraseToEnd(void)
{
    if (!FlashEngine_IsPhraseFillable())
    {
        return;
    }

    while (g_activePhrase.nextFillOffset < FLASH_PHRASE_SIZE)
    {
        g_activePhrase.data[g_activePhrase.nextFillOffset] = 0xFFu;
        g_activePhrase.nextFillOffset++;
    }
}

static bool FlashEngine_PadPhraseToAddress(uint32_t targetAddress)
{
    uint32_t targetOffset;

    if (!FlashEngine_IsPhraseFillable())
    {
        return false;
    }

    if (targetAddress < g_activePhrase.baseAddress)
    {
        return false;
    }

    targetOffset = targetAddress - g_activePhrase.baseAddress;

    if (targetOffset >= FLASH_PHRASE_SIZE)
    {
        return false;
    }

    if (targetOffset < g_activePhrase.nextFillOffset)
    {
        return false;
    }

    while (g_activePhrase.nextFillOffset < targetOffset)
    {
        g_activePhrase.data[g_activePhrase.nextFillOffset] = 0xFFu;
        g_activePhrase.nextFillOffset++;
    }

    return true;
}

static void FlashEngine_CompletePhraseSuccess(void)
{
    g_statistic.programmedDataBytes += g_activePhrase.actualDataByteCount;
    g_statistic.programmedPhraseCount++;

    FlashEngine_ResetActivePhrase();

    FlashDriver_AcknowledgeResult();
}

static bool FlashEngine_ProgramActivePhrase(void)
{
    uint32_t phraseAddress;

    if (!FlashEngine_IsPhraseFillable())
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            g_activePhrase.baseAddress);

        return false;
    }

    if (!FlashEngine_IsPhraseFull())
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            g_activePhrase.baseAddress);

        return false;
    }

    phraseAddress = g_activePhrase.baseAddress;

    if (!MemoryMap_IsApplicationPhraseValid(phraseAddress))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_ADDRESS_OUT_OF_RANGE,
            phraseAddress);

        return false;
    }

    g_activePhrase.isSubmitted = true;

    if (!FlashDriver_StartProgramPhrase(phraseAddress, g_activePhrase.data))
    {
        FlashEngine_SetDriverStartError(
            FlashDriver_GetStatus(),
            phraseAddress,
            BOOT_ERROR_FLASH_WRITE_FAILED);

        return false;
    }

    FlashEngine_CompletePhraseSuccess();

    return true;
}

static void FlashEngine_ProcessErase(void)
{
    if (g_statistic.erasedSectorCount >= g_statistic.totalSectorCount)
    {
        g_engineMode = FLASH_ENGINE_MODE_ERASE_COMPLETE;
        return;
    }

    if (!MemoryMap_IsApplicationSectorValid(g_nextEraseAddress))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_ADDRESS_OUT_OF_RANGE,
            g_nextEraseAddress);

        return;
    }

    if (!FlashDriver_StartEraseSector(g_nextEraseAddress))
    {
        FlashEngine_SetDriverStartError(
            FlashDriver_GetStatus(),
            g_nextEraseAddress,
            BOOT_ERROR_FLASH_ERASE_FAILED);

        return;
    }

    g_statistic.erasedSectorCount++;
    g_nextEraseAddress += FLASH_SECTOR_SIZE;

    FlashDriver_AcknowledgeResult();

    if (g_statistic.erasedSectorCount >= g_statistic.totalSectorCount)
    {
        g_engineMode = FLASH_ENGINE_MODE_ERASE_COMPLETE;
    }
}


static void FlashEngine_FinishErase(void)
{
    g_engineMode = FLASH_ENGINE_MODE_IDLE;

    (void)BootStateMachine_SetState(BOOT_STATE_MENU);
}

static bool FlashEngine_AcquireNextJob(void)
{
    FlashJobTypeData nextJob;

    if (g_jobContext.hasCurrentJob)
    {
        return true;
    }

    if (!FlashJobQueue_Peek(&nextJob))
    {
        return false;
    }

    g_jobContext.currentJob = nextJob;
    g_jobContext.currentJobOffset = 0u;
    g_jobContext.hasCurrentJob = true;

    g_jobContext.srecJobState =
        (nextJob.type == FLASH_JOB_PROGRAM_DATA)
            ? FLS_SREC_JOB_PROGRESSING
            : FLS_SREC_JOB_DONE;

    return true;
}

static bool FlashEngine_ReleaseCurrentJob(void)
{
    if (!g_jobContext.hasCurrentJob)
    {
        return false;
    }

    if (!FlashJobQueue_Pop())
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
        return false;
    }

    FlashEngine_ResetCurrentJob();

    return true;
}

static bool FlashEngine_ValidateCurrentProgramJob(void)
{
    const FlashProgramDataType *programData =
        &g_jobContext.currentJob.programData;

    if (!MemoryMap_IsApplicationRangeValid(
            programData->address,
            programData->length))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_ADDRESS_OUT_OF_RANGE,
            programData->address);

        return false;
    }

    /* Backwards address or overlap is fatal (section 15). */
    if (g_expectedNextAddressValid &&
        (programData->address < g_expectedNextAddress))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_ADDRESS_OVERLAP,
            programData->address);

        return false;
    }

    return true;
}

static bool FlashEngine_PrepareProgramJobStart(void)
{
    const FlashProgramDataType *programData =
        &g_jobContext.currentJob.programData;

    uint32_t recordAddress;
    uint32_t activePhraseEnd;

    if (!FlashEngine_ValidateCurrentProgramJob())
    {
        return false;
    }

    recordAddress = programData->address;

    if (!g_expectedNextAddressValid)
    {
        g_expectedNextAddress = recordAddress;
        g_expectedNextAddressValid = true;
    }

    if (!FlashEngine_IsPhraseBusy())
    {
        return FlashEngine_StartPhrase(recordAddress);
    }

    if (g_activePhrase.isSubmitted)
    {
        return false;
    }

    if (recordAddress == g_expectedNextAddress)
    {
        return true;
    }

    activePhraseEnd = g_activePhrase.baseAddress + FLASH_PHRASE_SIZE;

    if (recordAddress < activePhraseEnd)
    {
        if (!FlashEngine_PadPhraseToAddress(recordAddress))
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_INTERNAL_STATE,
                recordAddress);

            return false;
        }

        g_expectedNextAddress = recordAddress;

        return true;
    }

    FlashEngine_PadPhraseToEnd();

    (void)FlashEngine_ProgramActivePhrase();

    return false;
}

static void FlashEngine_ConsumeOneProgramByte(void)
{
    FlashProgramDataType *programData =
        &g_jobContext.currentJob.programData;

    uint32_t currentDataAddress;

    if (!g_jobContext.hasCurrentJob ||
        (g_jobContext.currentJob.type != FLASH_JOB_PROGRAM_DATA))
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
        return;
    }

    if (g_jobContext.currentJobOffset >= programData->length)
    {
        g_jobContext.srecJobState = FLS_SREC_JOB_DONE;
        return;
    }

    currentDataAddress =
        programData->address + g_jobContext.currentJobOffset;

    if (!FlashEngine_IsPhraseBusy())
    {
        if (!FlashEngine_StartPhrase(currentDataAddress))
        {
            return;
        }
    }

    if (g_activePhrase.isSubmitted)
    {
        return;
    }

    if (currentDataAddress >=
        (g_activePhrase.baseAddress + FLASH_PHRASE_SIZE))
    {
        FlashEngine_PadPhraseToEnd();
        (void)FlashEngine_ProgramActivePhrase();
        return;
    }

    if (!FlashEngine_PadPhraseToAddress(currentDataAddress))
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            currentDataAddress);

        return;
    }

    if (g_activePhrase.nextFillOffset >= FLASH_PHRASE_SIZE)
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            currentDataAddress);

        return;
    }

    g_activePhrase.data[g_activePhrase.nextFillOffset] =
        programData->data[g_jobContext.currentJobOffset];

    g_activePhrase.nextFillOffset++;
    g_activePhrase.actualDataByteCount++;

    g_jobContext.currentJobOffset++;

    g_expectedNextAddress = currentDataAddress + 1u;
    g_expectedNextAddressValid = true;

    if (FlashEngine_IsPhraseFull())
    {
        (void)FlashEngine_ProgramActivePhrase();
        return;
    }

    if (g_jobContext.currentJobOffset >= programData->length)
    {
        g_jobContext.srecJobState = FLS_SREC_JOB_DONE;
    }
}

static void FlashEngine_ProcessProgramJob(void)
{
    if (g_jobContext.currentJobOffset == 0u)
    {
        if (!FlashEngine_PrepareProgramJobStart())
        {
            return;
        }
    }

    FlashEngine_ConsumeOneProgramByte();

    if (BootStateMachine_IsError())
    {
        return;
    }

    if (g_jobContext.srecJobState == FLS_SREC_JOB_DONE)
    {
        (void)FlashEngine_ReleaseCurrentJob();
    }
}

static void FlashEngine_ProcessEndDownloadJob(void)
{
    g_endDownloadPending = true;

    if (FlashEngine_IsPhraseBusy())
    {
        if (g_activePhrase.isSubmitted)
        {
            /* Should not persist with a synchronous driver; retry safely. */
            return;
        }

        if (!FlashEngine_IsPhraseFull())
        {
            FlashEngine_PadPhraseToEnd();
        }

        (void)FlashEngine_ProgramActivePhrase();

        return;
    }

    if (!FlashEngine_ReleaseCurrentJob())
    {
        return;
    }

    g_endDownloadPending = false;
    g_engineMode = FLASH_ENGINE_MODE_IDLE;

    (void)BootStateMachine_SetState(BOOT_STATE_FINISHED);
}

static void FlashEngine_ProcessEraseJob(void)
{
    if (!FlashEngine_ReleaseCurrentJob())
    {
        return;
    }

    g_engineMode = FLASH_ENGINE_MODE_ERASE;

    g_nextEraseAddress = APPLICATION_START_ADDRESS;

    g_statistic.totalSectorCount = APP_SECTOR_COUNT;
    g_statistic.erasedSectorCount = 0u;
}

void FlashEngine_Init(void)
{
    g_engineMode = FLASH_ENGINE_MODE_IDLE;

    FlashEngine_ResetStatistic();
    FlashEngine_ResetCurrentJob();
    FlashEngine_ResetActivePhrase();

    g_expectedNextAddressValid = false;
    g_expectedNextAddress = 0u;

    g_endDownloadPending = false;

    g_nextEraseAddress = APPLICATION_START_ADDRESS;

    FlashDriver_Init();
}

bool FlashEngine_BeginDownloadSession(void)
{
    if (BootStateMachine_GetState() != BOOT_STATE_MENU)
    {
        return false;
    }

    if (!FlashJobQueue_IsEmpty())
    {
        return false;
    }

    if (FlashEngine_IsOperationActive())
    {
        return false;
    }

    g_engineMode = FLASH_ENGINE_MODE_DOWNLOAD;

    FlashEngine_ResetCurrentJob();
    FlashEngine_ResetActivePhrase();

    g_expectedNextAddressValid = false;
    g_expectedNextAddress = 0u;

    g_endDownloadPending = false;

    g_statistic.programmedDataBytes = 0u;
    g_statistic.programmedPhraseCount = 0u;

    return true;
}

void FlashEngine_MainFunction(void)
{
    FlashJobType jobType;

    if (BootStateMachine_IsError())
    {
        return;
    }

    /* Erase owns the cycle budget while it runs. */
    if (g_engineMode == FLASH_ENGINE_MODE_ERASE_COMPLETE)
    {
        FlashEngine_FinishErase();
        return;
    }

    if (g_engineMode == FLASH_ENGINE_MODE_ERASE)
    {
        FlashEngine_ProcessErase();
        return;
    }

    if (!g_jobContext.hasCurrentJob)
    {
        if (!FlashEngine_AcquireNextJob())
        {
            return;
        }
    }

    jobType = g_jobContext.currentJob.type;

    switch (jobType)
    {
        case FLASH_JOB_ERASE_APP:
        {
            FlashEngine_ProcessEraseJob();
            break;
        }

        case FLASH_JOB_PROGRAM_DATA:
        {
            const BootloaderStateType state = BootStateMachine_GetState();

            if ((state != BOOT_STATE_RECEIVING) &&
                (state != BOOT_STATE_FLUSHING))
            {
                ErrorHandler_SetFatal(
                    BOOT_ERROR_INTERNAL_STATE,
                    g_jobContext.currentJob.programData.address);

                return;
            }

            g_engineMode = FLASH_ENGINE_MODE_DOWNLOAD;

            FlashEngine_ProcessProgramJob();
            break;
        }

        case FLASH_JOB_END_DOWNLOAD:
        {
            if (BootStateMachine_GetState() != BOOT_STATE_FLUSHING)
            {
                ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
                return;
            }

            FlashEngine_ProcessEndDownloadJob();
            break;
        }

        case FLASH_JOB_NONE:
        default:
        {
            ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
            break;
        }
    }
}

bool FlashEngine_IsOperationActive(void)
{
    return ((g_engineMode != FLASH_ENGINE_MODE_IDLE) ||
            g_jobContext.hasCurrentJob ||
            FlashEngine_IsPhraseBusy() ||
            g_endDownloadPending);
}

FlashEngineStatisticType FlashEngine_GetStatisticSnapshot(void)
{
    return g_statistic;
}

uint32_t FlashEngine_GetEraseProgressPercent(void)
{
    if (g_statistic.totalSectorCount == 0u)
    {
        return 0u;
    }

    return ((g_statistic.erasedSectorCount * 100u) /
            g_statistic.totalSectorCount);
}

FlsSrecJobStateType FlashEngine_GetSrecJobState(void)
{
    return g_jobContext.srecJobState;
}

FlsPhraseJobStateType FlashEngine_GetPhraseJobState(void)
{
    return g_phraseState;
}

bool FlashEngine_IsPhraseSubmitted(void)
{
    return (FlashEngine_IsPhraseBusy() && g_activePhrase.isSubmitted);
}

uint32_t FlashEngine_GetExpectedNextAddress(void)
{
    return (g_expectedNextAddressValid ? g_expectedNextAddress : 0u);
}
