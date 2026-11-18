#ifndef BOOT_STATE_MACHINE_H
#define BOOT_STATE_MACHINE_H

#include <stdbool.h>
#include "compiler_abstraction.h"

typedef enum
{
    /** Waiting for a menu command. */
    BOOT_STATE_MENU = 0,

    /** FlashEngine is erasing the application region sector by sector. */
    BOOT_STATE_ERASING_APP,

    /** Receiving and parsing the SREC stream. */
    BOOT_STATE_RECEIVING,

    /** End record received; FlashEngine is completing the last phrase. */
    BOOT_STATE_FLUSHING,

    /** Download completed successfully; waiting for reset. */
    BOOT_STATE_FINISHED,

    /** Fatal error latched; held until reset. */
    BOOT_STATE_ERROR

} BootloaderStateType;

void BootStateMachine_Init(void);

RAM_FUNC BootloaderStateType BootStateMachine_GetState(void);

bool BootStateMachine_SetState(
    BootloaderStateType newState);

RAM_FUNC bool BootStateMachine_IsUartInputAccepted(void);

bool BootStateMachine_IsError(void);

#endif
