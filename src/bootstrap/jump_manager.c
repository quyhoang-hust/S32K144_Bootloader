#include "jump_manager.h"

#include <stdint.h>

#include "app_validator.h"
#include "memory_map.h"
#include "uart_driver.h"

#define JUMP_SCB_VTOR_ADDRESS           (0xE000ED08u)

#define JUMP_SYSTICK_CSR_ADDRESS        (0xE000E010u)
#define JUMP_SYSTICK_RVR_ADDRESS        (0xE000E014u)
#define JUMP_SYSTICK_CVR_ADDRESS        (0xE000E018u)

#define JUMP_NVIC_ICER_BASE_ADDRESS     (0xE000E180u)
#define JUMP_NVIC_ICPR_BASE_ADDRESS     (0xE000E280u)

#define JUMP_NVIC_REGISTER_COUNT        (8u)

#define JUMP_REG32(address)             \
    (*(volatile uint32_t *)(address))

static inline void JumpManager_DisableIrq(void)
{
    __asm volatile ("cpsid i" : : : "memory");
}

static inline void JumpManager_DataSyncBarrier(void)
{
    __asm volatile ("dsb 0xF" : : : "memory");
}

static inline void JumpManager_InstructionSyncBarrier(void)
{
    __asm volatile ("isb 0xF" : : : "memory");
}

static inline void JumpManager_Nop(void)
{
    __asm volatile ("nop");
}

static inline void JumpManager_SetMainStackPointer(uint32_t stackPointer)
{
    __asm volatile ("MSR msp, %0"
                    :
                    : "r" (stackPointer)
                    : "sp");
}

typedef void (*ApplicationResetHandlerType)(void);

static void JumpManager_DisableSystemInterrupts(void)
{
    uint32_t index;

    JumpManager_DisableIrq();

    JUMP_REG32(JUMP_SYSTICK_CSR_ADDRESS) = 0u;
    JUMP_REG32(JUMP_SYSTICK_RVR_ADDRESS) = 0u;
    JUMP_REG32(JUMP_SYSTICK_CVR_ADDRESS) = 0u;


    for (index = 0u;
         index < JUMP_NVIC_REGISTER_COUNT;
         index++)
    {
        JUMP_REG32(
            JUMP_NVIC_ICER_BASE_ADDRESS +
            (index * 4u)) = 0xFFFFFFFFu;

        JUMP_REG32(
            JUMP_NVIC_ICPR_BASE_ADDRESS +
            (index * 4u)) = 0xFFFFFFFFu;
    }

    JumpManager_DataSyncBarrier();
    JumpManager_InstructionSyncBarrier();
}

bool JumpManager_CanJump(void)
{
    return AppValidator_IsValid();
}

bool JumpManager_TryJumpToApplication(void)
{
    if (!AppValidator_IsValid())
    {
        return false;
    }

    JumpManager_JumpToApplication();

    /* Not reached. */
    return false;
}

NO_RETURN void JumpManager_JumpToApplication(void)
{
    AppVectorInfoType vectorInfo;
    ApplicationResetHandlerType applicationResetHandler;

    if (!AppValidator_IsValid())
    {
        for (;;)
        {
            JumpManager_Nop();
        }
    }

    vectorInfo =
        AppValidator_GetVectorInfo();

    UartDriver_Deinit(
        UART_DRIVER_CHANNEL_BOOTLOADER);

    JumpManager_DisableSystemInterrupts();

    /* Relocate the vector table to the application image. */
    JUMP_REG32(JUMP_SCB_VTOR_ADDRESS) =
        APPLICATION_START_ADDRESS;

    JumpManager_DataSyncBarrier();
    JumpManager_InstructionSyncBarrier();

    JumpManager_SetMainStackPointer(
        vectorInfo.initialMsp);

    JumpManager_DataSyncBarrier();
    JumpManager_InstructionSyncBarrier();

    applicationResetHandler =
        (ApplicationResetHandlerType)
        vectorInfo.resetVector;

    applicationResetHandler();

    for (;;)
    {
        JumpManager_Nop();
    }
}
