#include "boot_state_machine.h"

static volatile BootloaderStateType g_bootloaderState =
    BOOT_STATE_MENU;

static bool BootStateMachine_IsTransitionAllowed(
    BootloaderStateType currentState,
    BootloaderStateType newState)
{

    if (currentState == newState)
    {
        return (currentState == BOOT_STATE_MENU);
    }

    /* Any state may fail into ERROR exactly once. */
    if (newState == BOOT_STATE_ERROR)
    {
        return (currentState != BOOT_STATE_ERROR);
    }

    switch (currentState)
    {
        case BOOT_STATE_MENU:
        {
            return ((newState == BOOT_STATE_ERASING_APP) ||
                    (newState == BOOT_STATE_RECEIVING));
        }

        case BOOT_STATE_ERASING_APP:
        {
            return (newState == BOOT_STATE_MENU);
        }

        case BOOT_STATE_RECEIVING:
        {
            return (newState == BOOT_STATE_FLUSHING);
        }

        case BOOT_STATE_FLUSHING:
        {
            return (newState == BOOT_STATE_FINISHED);
        }

        case BOOT_STATE_FINISHED:
        {
            /* Terminal: only a reset leaves this state. */
            return false;
        }

        case BOOT_STATE_ERROR:
        {
            /* Terminal: held until reset. */
            return false;
        }

        default:
        {
            return false;
        }
    }
}

void BootStateMachine_Init(void)
{
    g_bootloaderState = BOOT_STATE_MENU;
}

RAM_FUNC BootloaderStateType BootStateMachine_GetState(void)
{
    return g_bootloaderState;
}

bool BootStateMachine_SetState(
    BootloaderStateType newState)
{
    const BootloaderStateType currentState =
        g_bootloaderState;

    if (!BootStateMachine_IsTransitionAllowed(
            currentState,
            newState))
    {
        return false;
    }

    g_bootloaderState = newState;

    return true;
}

RAM_FUNC bool BootStateMachine_IsUartInputAccepted(void)
{
    const BootloaderStateType currentState =
        g_bootloaderState;

    return ((currentState == BOOT_STATE_MENU) ||
            (currentState == BOOT_STATE_RECEIVING));
}

bool BootStateMachine_IsError(void)
{
    return (g_bootloaderState == BOOT_STATE_ERROR);
}
