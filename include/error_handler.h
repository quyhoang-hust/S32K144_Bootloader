#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdbool.h>
#include <stdint.h>
#include "compiler_abstraction.h"

typedef enum
{
    BOOT_ERROR_NONE = 0,

    BOOT_ERROR_RX_OVERFLOW,

    BOOT_ERROR_LINE_TOO_LONG,

    BOOT_ERROR_INVALID_SREC,

    BOOT_ERROR_ADDRESS_OUT_OF_RANGE,

    BOOT_ERROR_ADDRESS_OVERLAP,

    BOOT_ERROR_FLASH_JOB_QUEUE_OVERFLOW,

    BOOT_ERROR_FLASH_ERASE_FAILED,

    BOOT_ERROR_FLASH_WRITE_FAILED,

    BOOT_ERROR_UNSUPPORTED_FLASH_COMMAND,

    BOOT_ERROR_FLASH_DRIVER_ADDRESS_VIOLATION,

    BOOT_ERROR_FLASH_DRIVER_ALIGNMENT_VIOLATION,

    BOOT_ERROR_INTERNAL_STATE

} BootErrorCodeType;


typedef struct
{
    BootErrorCodeType code;
    uint32_t address;
} BootFatalErrorType;

void ErrorHandler_Init(void);

void ErrorHandler_SetFatal(
    BootErrorCodeType code,
    uint32_t address);

RAM_FUNC void ErrorHandler_SetFatalFromIsr(
    BootErrorCodeType code,
    uint32_t address);

void ErrorHandler_ProcessPending(void);

bool ErrorHandler_HasFatalError(void);

BootFatalErrorType ErrorHandler_GetSnapshot(void);

#endif
