#include "error_handler.h"
#include "boot_state_machine.h"
#include "compiler_abstraction.h"

static BootFatalErrorType g_fatalError =
{
    BOOT_ERROR_NONE,
    0u
};

static volatile BootErrorCodeType g_pendingIsrErrorCode =
    BOOT_ERROR_NONE;

static volatile uint32_t g_pendingIsrErrorAddress =
    0u;

static volatile bool g_isrErrorPending =
    false;

static void ErrorHandler_StoreFirstError(
    BootErrorCodeType code,
    uint32_t address)
{
    if ((g_fatalError.code == BOOT_ERROR_NONE) &&
        (code != BOOT_ERROR_NONE))
    {
        g_fatalError.code = code;
        g_fatalError.address = address;
    }
}

void ErrorHandler_Init(void)
{
    g_fatalError.code = BOOT_ERROR_NONE;
    g_fatalError.address = 0u;

    g_pendingIsrErrorCode = BOOT_ERROR_NONE;
    g_pendingIsrErrorAddress = 0u;
    g_isrErrorPending = false;
}

void ErrorHandler_SetFatal(
    BootErrorCodeType code,
    uint32_t address)
{
    if (code == BOOT_ERROR_NONE)
    {
        return;
    }

    ErrorHandler_StoreFirstError(
        code,
        address);

    (void)BootStateMachine_SetState(
        BOOT_STATE_ERROR);
}

RAM_FUNC void ErrorHandler_SetFatalFromIsr(
    BootErrorCodeType code,
    uint32_t address)
{
    if (code == BOOT_ERROR_NONE)
    {
        return;
    }

    if (!g_isrErrorPending)
    {
        g_pendingIsrErrorCode = code;
        g_pendingIsrErrorAddress = address;

        /*
         * Complete payload writes before publishing pending == true.
         */
        COMPILER_MEMORY_BARRIER();

        g_isrErrorPending = true;
    }
}

void ErrorHandler_ProcessPending(void)
{
    BootErrorCodeType pendingCode;
    uint32_t pendingAddress;

    if (!g_isrErrorPending)
    {
        return;
    }

    pendingCode = g_pendingIsrErrorCode;
    pendingAddress = g_pendingIsrErrorAddress;

    g_pendingIsrErrorCode = BOOT_ERROR_NONE;
    g_pendingIsrErrorAddress = 0u;

    COMPILER_MEMORY_BARRIER();


    g_isrErrorPending = false;

    ErrorHandler_SetFatal(
        pendingCode,
        pendingAddress);
}

bool ErrorHandler_HasFatalError(void)
{
    return (g_fatalError.code !=
            BOOT_ERROR_NONE);
}

BootFatalErrorType ErrorHandler_GetSnapshot(void)
{
    BootFatalErrorType snapshot;

    snapshot.code = g_fatalError.code;
    snapshot.address = g_fatalError.address;

    return snapshot;
}
