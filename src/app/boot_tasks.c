#include "boot_tasks.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "boot_menu.h"
#include "boot_state_machine.h"
#include "boot_statistic.h"
#include "bootloader_config.h"
#include "error_handler.h"
#include "flash_engine.h"
#include "flash_job_queue.h"
#include "input_filter.h"
#include "line_builder.h"
#include "rx_ring_buffer.h"
#include "srec_parser.h"
#include "srec_types.h"
#include "uart_driver.h"

#ifndef FLASH_ENGINE_STATE_TRACE_ENABLED
#define FLASH_ENGINE_STATE_TRACE_ENABLED    (0u)
#endif

#ifndef DOWNLOAD_SUMMARY_REPORT_ENABLED
#define DOWNLOAD_SUMMARY_REPORT_ENABLED     (1u)
#endif

static BootloaderStateType g_lastReportedState = BOOT_STATE_MENU;

static BootloaderStateType g_previousObservedState = BOOT_STATE_MENU;

static uint32_t g_lastReportedErasePercent = 0u;

static uint32_t g_lastReportedDownloadStep = 0u;

static BootErrorCodeType g_lastReportedError = BOOT_ERROR_NONE;

static bool g_stateReportInitialized = false;

static bool g_eraseReportInitialized = false;

static bool g_finalDownloadReportSent = false;

#if (FLASH_ENGINE_STATE_TRACE_ENABLED == 1u)
static FlsSrecJobStateType   g_lastTracedSrecState = FLS_SREC_JOB_DONE;
static FlsPhraseJobStateType g_lastTracedPhraseState = FLS_JOB_DONE;
static bool                  g_lastTracedSubmitted = false;
static bool                  g_traceInitialized = false;
#endif

static void BootTasks_SendString(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    (void)UartDriver_SendString(
        UART_DRIVER_CHANNEL_BOOTLOADER,
        text);
}

static void BootTasks_SendUnsignedDecimal(uint32_t value)
{
    char buffer[11];
    uint32_t index;
    uint32_t digit;

    index = (uint32_t)sizeof(buffer);

    buffer[--index] = '\0';

    do
    {
        digit = value % 10u;
        value /= 10u;

        buffer[--index] = (char)('0' + digit);

    } while ((value != 0u) && (index > 0u));

    BootTasks_SendString(&buffer[index]);
}

static void BootTasks_SendHex32(uint32_t value)
{
    static const char hexDigits[] = "0123456789ABCDEF";

    char buffer[11];
    uint32_t index;
    uint32_t shift;

    buffer[0] = '0';
    buffer[1] = 'x';

    for (index = 0u; index < 8u; index++)
    {
        shift = (7u - index) * 4u;

        buffer[index + 2u] = hexDigits[(value >> shift) & 0x0Fu];
    }

    buffer[10] = '\0';

    BootTasks_SendString(buffer);
}

static const char *BootTasks_GetStateName(BootloaderStateType state)
{
    switch (state)
    {
        case BOOT_STATE_MENU:        return "MENU";
        case BOOT_STATE_ERASING_APP: return "ERASING_APP";
        case BOOT_STATE_RECEIVING:   return "RECEIVING";
        case BOOT_STATE_FLUSHING:    return "FLUSHING";
        case BOOT_STATE_FINISHED:    return "FINISHED";
        case BOOT_STATE_ERROR:       return "ERROR";
        default:                     return "UNKNOWN";
    }
}

static const char *BootTasks_GetErrorName(BootErrorCodeType code)
{
    switch (code)
    {
        case BOOT_ERROR_NONE:
            return "BOOT_ERROR_NONE";

        case BOOT_ERROR_RX_OVERFLOW:
            return "BOOT_ERROR_RX_OVERFLOW";

        case BOOT_ERROR_LINE_TOO_LONG:
            return "BOOT_ERROR_LINE_TOO_LONG";

        case BOOT_ERROR_INVALID_SREC:
            return "BOOT_ERROR_INVALID_SREC";

        case BOOT_ERROR_ADDRESS_OUT_OF_RANGE:
            return "BOOT_ERROR_ADDRESS_OUT_OF_RANGE";

        case BOOT_ERROR_ADDRESS_OVERLAP:
            return "BOOT_ERROR_ADDRESS_OVERLAP";

        case BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW:
            return "BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW";

        case BOOT_ERROR_FLASH_ERASE_FAILED:
            return "BOOT_ERROR_FLASH_ERASE_FAILED";

        case BOOT_ERROR_FLASH_WRITE_FAILED:
            return "BOOT_ERROR_FLASH_WRITE_FAILED";

        case BOOT_ERROR_UNSUPPORTED_FLASH_COMMAND:
            return "BOOT_ERROR_UNSUPPORTED_FLASH_COMMAND";

        case BOOT_ERROR_FLASH_DRIVER_ADDRESS_VIOLATION:
            return "BOOT_ERROR_FLASH_DRIVER_ADDRESS_VIOLATION";

        case BOOT_ERROR_FLASH_DRIVER_ALIGNMENT_VIOLATION:
            return "BOOT_ERROR_FLASH_DRIVER_ALIGNMENT_VIOLATION";

        case BOOT_ERROR_INTERNAL_STATE:
            return "BOOT_ERROR_INTERNAL_STATE";

        default:
            return "BOOT_ERROR_UNKNOWN";
    }
}

#if (FLASH_ENGINE_STATE_TRACE_ENABLED == 1u)

static const char *BootTasks_GetSrecJobStateName(FlsSrecJobStateType state)
{
    return (state == FLS_SREC_JOB_PROGRESSING) ? "PROGRESSING" : "DONE";
}

static const char *BootTasks_GetPhraseJobStateName(FlsPhraseJobStateType state)
{
    return (state == FLS_JOB_BUSY) ? "BUSY" : "DONE";
}

#endif

static void BootTasks_StartErase(void)
{
    if (!FlashJobQueue_PushEraseApp())
    {
        ErrorHandler_SetFatal(BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW, 0u);
        return;
    }

    if (!BootStateMachine_SetState(BOOT_STATE_ERASING_APP))
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
        return;
    }

    BootStatistic_IncrementMenuCommandCount();

    g_eraseReportInitialized = false;
    g_lastReportedErasePercent = 0u;
}

static void BootTasks_StartDownload(void)
{
    UartDriver_DisableRxInterrupt(UART_DRIVER_CHANNEL_BOOTLOADER);

    RxRingBuffer_Clear();

    LineBuilder_Reset();
    SrecParser_Reset();

    UartDriver_ClearRxFlags(UART_DRIVER_CHANNEL_BOOTLOADER);

    if (!FlashEngine_BeginDownloadSession())
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
        return;
    }

    BootStatistic_ResetDownload();

    if (!BootStateMachine_SetState(BOOT_STATE_RECEIVING))
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INTERNAL_STATE, 0u);
        return;
    }

    UartDriver_EnableRxInterrupt(UART_DRIVER_CHANNEL_BOOTLOADER);

    BootStatistic_IncrementMenuCommandCount();

    g_lastReportedDownloadStep = 0u;
    g_finalDownloadReportSent = false;

    BootMenu_ReportDownloadReady();
}

static void BootTasks_HandleCompleteSrecLine(void)
{
    SrecRecordType record;
    SrecParseResultType parseResult;

    BootStatistic_IncrementCompletedLineCount();

    parseResult =
        SrecParser_Parse(
            LineBuilder_GetLine(),
            LineBuilder_GetLength(),
            &record);

    if (parseResult != SREC_PARSE_OK)
    {
        char typeText[3];

        BootTasks_SendString(
            "[SREC] parse_error=");

        BootTasks_SendUnsignedDecimal(
            (uint32_t)parseResult);

        BootTasks_SendString(
            " length=");

        BootTasks_SendUnsignedDecimal(
            LineBuilder_GetLength());

        BootTasks_SendString(
            " type=");

        if (LineBuilder_GetLength() >= 2u)
        {
            typeText[0] =
                LineBuilder_GetLine()[0];

            typeText[1] =
                LineBuilder_GetLine()[1];

            typeText[2] = '\0';

            BootTasks_SendString(typeText);
        }
        else
        {
            BootTasks_SendString("??");
        }

        BootTasks_SendString("\r\n");

        ErrorHandler_SetFatal(
            BOOT_ERROR_INVALID_SREC,
            (uint32_t)parseResult);

        LineBuilder_Reset();

        return;
    }

    LineBuilder_Reset();

    /* ---- Data record: S1 / S2 / S3 ---------------------------------- */
    if (record.recordClass == SREC_RECORD_CLASS_DATA)
    {
        if (!FlashJobQueue_PushProgramData(
                record.address,
                record.data,
                record.dataLength))
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW,
                record.address);

            return;
        }

        BootStatistic_IncrementDataRecordCount();

        return;
    }

    /* ---- End record: S7 / S8 / S9 ----------------------------------- */
    if (record.recordClass == SREC_RECORD_CLASS_END)
    {
        if (!FlashJobQueue_PushEndDownload())
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW,
                0u);

            return;
        }

        BootStatistic_IncrementEndRecordCount();

        if (!BootStateMachine_SetState(
                BOOT_STATE_FLUSHING))
        {
            ErrorHandler_SetFatal(
                BOOT_ERROR_INTERNAL_STATE,
                0u);

            return;
        }

        UartDriver_DisableRxInterrupt(
            UART_DRIVER_CHANNEL_BOOTLOADER);

        RxRingBuffer_Clear();

        UartDriver_ClearRxFlags(
            UART_DRIVER_CHANNEL_BOOTLOADER);

        return;
    }

    /* ---- Informational record: S0 / S5 / S6 ------------------------- */
    if (record.recordClass == SREC_RECORD_CLASS_INFO)
    {
        return;
    }

    ErrorHandler_SetFatal(
        BOOT_ERROR_INVALID_SREC,
        (uint32_t)SREC_PARSE_UNSUPPORTED_RECORD);
}

static void BootTasks_ProcessSrecByte(uint8_t data)
{
    LineBuilderResultType lineResult;

    if (!InputFilter_IsAllowedByte(data))
    {
        ErrorHandler_SetFatal(BOOT_ERROR_INVALID_SREC, 0u);
        return;
    }

    lineResult = LineBuilder_PushByte(data);

    switch (lineResult)
    {
        case LINE_BUILDER_IN_PROGRESS:
        case LINE_BUILDER_EMPTY_LINE:
        {
            break;
        }

        case LINE_BUILDER_COMPLETE:
        {
            BootTasks_HandleCompleteSrecLine();
            break;
        }

        case LINE_BUILDER_TOO_LONG:
        {
            ErrorHandler_SetFatal(BOOT_ERROR_LINE_TOO_LONG, 0u);
            break;
        }

        case LINE_BUILDER_INVALID_START:
        case LINE_BUILDER_INVALID_CHARACTER:
        case LINE_BUILDER_INTERNAL_ERROR:
        default:
        {
            ErrorHandler_SetFatal(BOOT_ERROR_INVALID_SREC, 0u);
            break;
        }
    }
}


static void BootTasks_ReportFatalIfRequired(void)
{
#if (FATAL_ERROR_REPORT_ENABLED == 1u)

    BootFatalErrorType error;

    if (!ErrorHandler_HasFatalError())
    {
        return;
    }

    error = ErrorHandler_GetSnapshot();

    if ((error.code == BOOT_ERROR_NONE) ||
        (error.code == g_lastReportedError))
    {
        return;
    }

    BootTasks_SendString("[FATAL] code=");
    BootTasks_SendString(BootTasks_GetErrorName(error.code));
    BootTasks_SendString(" address=");
    BootTasks_SendHex32(error.address);
    BootTasks_SendString("\r\n");

    g_lastReportedError = error.code;

#endif
}

static void BootTasks_ReportStateIfRequired(void)
{
#if (BOOT_STATE_REPORT_ON_CHANGE == 1u)

    const BootloaderStateType currentState = BootStateMachine_GetState();

    bool arrivedFromOtherState;

    /* Erase finished: announce it before the MENU banner is redrawn. */
    if ((g_previousObservedState == BOOT_STATE_ERASING_APP) &&
        (currentState == BOOT_STATE_MENU) &&
        !ErrorHandler_HasFatalError())
    {
        BootTasks_SendString("\r\n[BOOT] Erase application done\r\n");
    }

    if (!g_stateReportInitialized ||
        (currentState != g_lastReportedState))
    {
        arrivedFromOtherState =
            (g_stateReportInitialized &&
             (g_previousObservedState != currentState));

        BootTasks_SendString("[BOOT] state=");
        BootTasks_SendString(BootTasks_GetStateName(currentState));
        BootTasks_SendString("\r\n");

        g_lastReportedState = currentState;
        g_stateReportInitialized = true;

        /* Only a genuine re-entry into MENU needs a fresh banner. */
        if ((currentState == BOOT_STATE_MENU) && arrivedFromOtherState)
        {
            BootMenu_Reset();
        }
    }

    g_previousObservedState = currentState;

#endif
}

static void BootTasks_ReportEraseIfRequired(void)
{
#if (ERASE_REPORT_ON_PERCENT_CHANGE == 1u)

    FlashEngineStatisticType statistic;
    uint32_t progress;

    if (BootStateMachine_GetState() != BOOT_STATE_ERASING_APP)
    {
        return;
    }

    statistic = FlashEngine_GetStatisticSnapshot();
    progress = FlashEngine_GetEraseProgressPercent();

    if (!g_eraseReportInitialized ||
        (progress != g_lastReportedErasePercent))
    {
        BootTasks_SendString("[ERASE] progress=");
        BootTasks_SendUnsignedDecimal(progress);
        BootTasks_SendString("% sectors=");
        BootTasks_SendUnsignedDecimal(statistic.erasedSectorCount);
        BootTasks_SendString("/");
        BootTasks_SendUnsignedDecimal(statistic.totalSectorCount);
        BootTasks_SendString("\r\n");

        g_lastReportedErasePercent = progress;
        g_eraseReportInitialized = true;
    }

#endif
}

static void BootTasks_ReportDownloadSummary(void)
{
#if (DOWNLOAD_SUMMARY_REPORT_ENABLED == 1u)

    BootStatisticType bootStat;
    FlashEngineStatisticType flashStat;

    bootStat =
        BootStatistic_GetSnapshot();

    flashStat =
        FlashEngine_GetStatisticSnapshot();

    BootTasks_SendString(
        "[STAT] lines=");

    BootTasks_SendUnsignedDecimal(
        bootStat.completedLineCount);

    BootTasks_SendString(
        " data_rec=");

    BootTasks_SendUnsignedDecimal(
        bootStat.receivedDataRecordCount);

    BootTasks_SendString(
        " end_rec=");

    BootTasks_SendUnsignedDecimal(
        bootStat.receivedEndRecordCount);

    BootTasks_SendString(
        " phrases=");

    BootTasks_SendUnsignedDecimal(
        flashStat.programmedPhraseCount);

    BootTasks_SendString(
        " queue_peak=");

    BootTasks_SendUnsignedDecimal(
        FlashJobQueue_GetPeakUsage());

    BootTasks_SendString("/");

    BootTasks_SendUnsignedDecimal(
        FlashJobQueue_GetCapacity());

    BootTasks_SendString("\r\n");

#endif
}

static void BootTasks_ReportDownloadIfRequired(void)
{
    FlashEngineStatisticType statistic;
    BootloaderStateType currentState;
    uint32_t currentStep;

    statistic =
        FlashEngine_GetStatisticSnapshot();

    currentState =
        BootStateMachine_GetState();

    currentStep =
        statistic.programmedDataBytes /
        DOWNLOAD_REPORT_STEP_BYTES;

    if ((currentState == BOOT_STATE_FINISHED) &&
        !g_finalDownloadReportSent)
    {
        BootTasks_SendString(
            "[DOWNLOAD] bytes_written=");

        BootTasks_SendUnsignedDecimal(
            statistic.programmedDataBytes);

        BootTasks_SendString("\r\n");

        BootTasks_ReportDownloadSummary();

        g_lastReportedDownloadStep =
            currentStep;

        g_finalDownloadReportSent = true;

        return;
    }

    if (currentStep >
        g_lastReportedDownloadStep)
    {
        BootTasks_SendString(
            "[DOWNLOAD] bytes_written=");

        BootTasks_SendUnsignedDecimal(
            statistic.programmedDataBytes);

        BootTasks_SendString("\r\n");

        g_lastReportedDownloadStep =
            currentStep;
    }
}

static void BootTasks_TraceFlashEngineState(void)
{
#if (FLASH_ENGINE_STATE_TRACE_ENABLED == 1u)

    const FlsSrecJobStateType srecState = FlashEngine_GetSrecJobState();
    const FlsPhraseJobStateType phraseState = FlashEngine_GetPhraseJobState();
    const bool submitted = FlashEngine_IsPhraseSubmitted();

    if (g_traceInitialized &&
        (srecState == g_lastTracedSrecState) &&
        (phraseState == g_lastTracedPhraseState) &&
        (submitted == g_lastTracedSubmitted))
    {
        return;
    }

    BootTasks_SendString("[FLASH] srec=");
    BootTasks_SendString(BootTasks_GetSrecJobStateName(srecState));

    BootTasks_SendString(" phrase=");
    BootTasks_SendString(BootTasks_GetPhraseJobStateName(phraseState));

    BootTasks_SendString(" submitted=");
    BootTasks_SendUnsignedDecimal(submitted ? 1u : 0u);

    BootTasks_SendString(" next=");
    BootTasks_SendHex32(FlashEngine_GetExpectedNextAddress());

    BootTasks_SendString("\r\n");

    g_lastTracedSrecState = srecState;
    g_lastTracedPhraseState = phraseState;
    g_lastTracedSubmitted = submitted;
    g_traceInitialized = true;

#endif
}

void BootTasks_Init(void)
{
    BootMenu_Init();

    LineBuilder_Init();
    SrecParser_Init();

    g_lastReportedState = BootStateMachine_GetState();
    g_previousObservedState = BootStateMachine_GetState();

    g_lastReportedErasePercent = 0u;
    g_lastReportedDownloadStep = 0u;

    g_lastReportedError = BOOT_ERROR_NONE;

    g_stateReportInitialized = false;
    g_eraseReportInitialized = false;
    g_finalDownloadReportSent = false;

#if (FLASH_ENGINE_STATE_TRACE_ENABLED == 1u)
    g_traceInitialized = false;
#endif
}

void Task_MenuProcess(void)
{
    uint8_t receivedByte;
    BootMenuCommandType command;

    if (BootStateMachine_GetState() != BOOT_STATE_MENU)
    {
        return;
    }

    BootMenu_DisplayIfRequired();

    /* One byte per cycle keeps the task bounded. */
    if (!RxRingBuffer_Pop(&receivedByte))
    {
        return;
    }

    command = BootMenu_ProcessByte(receivedByte);

    switch (command)
    {
        case BOOT_MENU_COMMAND_NONE:
        {
            break;
        }

        case BOOT_MENU_COMMAND_ERASE_APPLICATION:
        {
            BootTasks_StartErase();
            break;
        }

        case BOOT_MENU_COMMAND_START_DOWNLOAD:
        {
            BootTasks_StartDownload();
            break;
        }

        case BOOT_MENU_COMMAND_INVALID:
        default:
        {
            BootStatistic_IncrementInvalidMenuCommandCount();

            BootMenu_ReportInvalidOption();
            break;
        }
    }
}

void Task_SrecRxProcess(void)
{
    uint8_t receivedByte;
    uint32_t processedByteCount = 0u;

    if (BootStateMachine_GetState() != BOOT_STATE_RECEIVING)
    {
        return;
    }

    while ((processedByteCount < SREC_RX_MAX_BYTES_PER_CYCLE) &&
           RxRingBuffer_Pop(&receivedByte))
    {
        BootTasks_ProcessSrecByte(receivedByte);

        processedByteCount++;

        if (BootStateMachine_GetState() != BOOT_STATE_RECEIVING)
        {
            return;
        }
    }
}

void Task_FlashProgram(void)
{
    FlashEngine_MainFunction();
}

void Task_ReportStatistic(void)
{

    BootTasks_ReportFatalIfRequired();

    BootTasks_ReportStateIfRequired();

    BootTasks_ReportEraseIfRequired();

    BootTasks_ReportDownloadIfRequired();

    BootTasks_TraceFlashEngineState();
}
