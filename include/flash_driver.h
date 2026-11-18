#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "compiler_abstraction.h"
#include "memory_map.h"

typedef enum
{
    /**
     * No erase/program operation is active.
     */
    FLASH_DRIVER_OPERATION_NONE = 0,

    /**
     * Erase one application flash sector.
     */
    FLASH_DRIVER_OPERATION_ERASE_SECTOR,

    /**
     * Program one eight-byte application flash phrase.
     */
    FLASH_DRIVER_OPERATION_PROGRAM_PHRASE

} FlashDriverOperationType;

typedef enum
{
    /**
     * Driver is initialized and no command result is pending.
     */
    FLASH_DRIVER_STATUS_IDLE = 0,

    /**
     * An FTFC command is currently active.
     */
    FLASH_DRIVER_STATUS_BUSY,

    /**
     * The most recent command completed successfully.
     *
     * This result remains available until
     * FlashDriver_AcknowledgeResult() is called.
     */
    FLASH_DRIVER_STATUS_SUCCESS,

    /**
     * The most recent FTFC command completed with a hardware error.
     */
    FLASH_DRIVER_STATUS_FAILED,

    /**
     * Requested address is outside the application flash region.
     */
    FLASH_DRIVER_STATUS_ADDRESS_VIOLATION,

    /**
     * Requested address does not satisfy sector/phrase alignment.
     */
    FLASH_DRIVER_STATUS_ALIGNMENT_VIOLATION,

    /**
     * A flash command outside the project whitelist was requested.
     *
     * The current public API never intentionally requests such a command.
     * This value is retained for defensive error mapping.
     */
    FLASH_DRIVER_STATUS_UNSUPPORTED_COMMAND,

    /**
     * A new command was requested while FlashDriver or FTFC was not ready.
     */
    FLASH_DRIVER_STATUS_NOT_READY

} FlashDriverStatusType;

void FlashDriver_Init(void);

RAM_FUNC bool FlashDriver_StartEraseSector(
    uint32_t sectorAddress);

RAM_FUNC bool FlashDriver_StartProgramPhrase(
    uint32_t phraseAddress,
    const uint8_t data[FLASH_PHRASE_SIZE]);

RAM_FUNC FlashDriverStatusType FlashDriver_GetStatus(void);

FlashDriverOperationType FlashDriver_GetOperation(void);

uint32_t FlashDriver_GetOperationAddress(void);

RAM_FUNC bool FlashDriver_IsBusy(void);

void FlashDriver_AcknowledgeResult(void);

#endif
