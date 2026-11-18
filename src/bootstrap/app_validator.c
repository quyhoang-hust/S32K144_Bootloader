#include "app_validator.h"
#include "memory_map.h"

#define APP_VALIDATOR_MSP_ALIGNMENT_MASK    (0x3u)

static AppDiagnosticCodeType g_appDiagnostic =
    APP_DIAG_NONE;

static AppVectorInfoType g_vectorInfo =
{
    0u,
    0u,
    0u
};

static uint32_t AppValidator_ReadWord(
    uint32_t address)
{
    const volatile uint32_t *memoryAddress;

    memoryAddress =
        (const volatile uint32_t *)address;

    return *memoryAddress;
}

void AppValidator_Init(void)
{
    g_appDiagnostic =
        APP_DIAG_NONE;

    g_vectorInfo.initialMsp = 0u;
    g_vectorInfo.resetVector = 0u;
    g_vectorInfo.resetHandlerAddress = 0u;
}

bool AppValidator_IsMspValid(uint32_t initialMsp)
{
    if ((initialMsp &
         APP_VALIDATOR_MSP_ALIGNMENT_MASK) != 0u)
    {
        return false;
    }

    return MemoryMap_IsStackTopValid(
        initialMsp);
}

bool AppValidator_IsResetVectorValid(
    uint32_t resetVector)
{
    uint32_t handlerAddress;

    if (!MemoryMap_HasThumbBit(
            resetVector))
    {
        return false;
    }

    handlerAddress =
        MemoryMap_GetHandlerAddress(
            resetVector);

    return MemoryMap_IsApplicationAddress(
        handlerAddress);
}

bool AppValidator_IsValid(void)
{
    g_appDiagnostic =
        APP_DIAG_NONE;

    g_vectorInfo.initialMsp =
        AppValidator_ReadWord(
            APPLICATION_INITIAL_MSP_ADDRESS);

    g_vectorInfo.resetVector =
        AppValidator_ReadWord(
            APPLICATION_RESET_VECTOR_ADDRESS);

    g_vectorInfo.resetHandlerAddress =
        MemoryMap_GetHandlerAddress(
            g_vectorInfo.resetVector);

    if (!AppValidator_IsMspValid(
            g_vectorInfo.initialMsp))
    {
        g_appDiagnostic =
            APP_DIAG_INVALID_MSP;

        return false;
    }

    if (!MemoryMap_HasThumbBit(
            g_vectorInfo.resetVector))
    {
        g_appDiagnostic =
            APP_DIAG_INVALID_THUMB_BIT;

        return false;
    }

    if (!MemoryMap_IsApplicationAddress(
            g_vectorInfo.resetHandlerAddress))
    {
        g_appDiagnostic =
            APP_DIAG_INVALID_RESET_HANDLER;

        return false;
    }

    return true;
}

AppDiagnosticCodeType
AppValidator_GetDiagnostic(void)
{
    return g_appDiagnostic;
}

AppVectorInfoType
AppValidator_GetVectorInfo(void)
{
    return g_vectorInfo;
}

const char *AppValidator_GetDiagnosticName(
    AppDiagnosticCodeType diagnostic)
{
    switch (diagnostic)
    {
        case APP_DIAG_NONE:
        {
            return "APP_DIAG_NONE";
        }

        case APP_DIAG_INVALID_MSP:
        {
            return "APP_DIAG_INVALID_MSP";
        }

        case APP_DIAG_INVALID_THUMB_BIT:
        {
            return "APP_DIAG_INVALID_THUMB_BIT";
        }

        case APP_DIAG_INVALID_RESET_HANDLER:
        {
            return "APP_DIAG_INVALID_RESET_HANDLER";
        }

        default:
        {
            return "APP_DIAG_UNKNOWN";
        }
    }
}
