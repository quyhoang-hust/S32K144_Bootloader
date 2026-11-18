#ifndef APP_VALIDATOR_H
#define APP_VALIDATOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{

    APP_DIAG_NONE = 0,

    APP_DIAG_INVALID_MSP,

    APP_DIAG_INVALID_THUMB_BIT,

    APP_DIAG_INVALID_RESET_HANDLER

} AppDiagnosticCodeType;


typedef struct
{

    uint32_t initialMsp;

    uint32_t resetVector;

    uint32_t resetHandlerAddress;

} AppVectorInfoType;

void AppValidator_Init(void);

bool AppValidator_IsValid(void);

bool AppValidator_IsMspValid(uint32_t initialMsp);

bool AppValidator_IsResetVectorValid(uint32_t resetVector);

AppDiagnosticCodeType AppValidator_GetDiagnostic(void);

AppVectorInfoType AppValidator_GetVectorInfo(void);

const char *AppValidator_GetDiagnosticName(
    AppDiagnosticCodeType diagnostic);

#endif
