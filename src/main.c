
#include <stdbool.h>
#include <stdint.h>

#include "device_registers.h"

#include "app_validator.h"
#include "boot_button.h"
#include "boot_menu.h"
#include "boot_state_machine.h"
#include "boot_statistic.h"
#include "boot_tasks.h"
#include "bootloader_config.h"
#include "clock_driver.h"
#include "error_handler.h"
#include "flash_engine.h"
#include "flash_job_queue.h"
#include "gpio_driver.h"
#include "jump_manager.h"
#include "rx_ring_buffer.h"
#include "scheduler.h"
#include "uart_driver.h"

#define BOOT_BUTTON_ENTRY_ENABLED       (1u)

#define BOOT_ENTRY_WINDOW_LOOPS         (4000000u)

#define BOOT_INIT_STAGE_CLOCK           (1u)
#define BOOT_INIT_STAGE_BOOT_BUTTON     (2u)
#define BOOT_INIT_STAGE_UART            (3u)

#define BOOT_WDOG_UNLOCK_VALUE          (0xD928C520u)

static void Bootloader_Print(const char *text)
{
    (void)UartDriver_SendString(
        UART_DRIVER_CHANNEL_BOOTLOADER,
        text);
}

static void Bootloader_PrintHex32(uint32_t value)
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

        buffer[index + 2u] =
            hexDigits[(value >> shift) & 0x0Fu];
    }

    buffer[10] = '\0';

    Bootloader_Print(buffer);
}

static void Bootloader_ReportResetSource(void)
{
    uint32_t resetStatus = (uint32_t)RCM->SRS;

    Bootloader_Print("[BOOT] reset_src=");
    Bootloader_PrintHex32(resetStatus);

    if ((resetStatus & RCM_SRS_WDOG_MASK) != 0u)
    {
        Bootloader_Print(" WDOG");
    }

    if ((resetStatus & RCM_SRS_LOCKUP_MASK) != 0u)
    {
        Bootloader_Print(" LOCKUP");
    }

    if ((resetStatus & RCM_SRS_PIN_MASK) != 0u)
    {
        Bootloader_Print(" PIN");
    }

    if ((resetStatus & RCM_SRS_POR_MASK) != 0u)
    {
        Bootloader_Print(" POR");
    }

    Bootloader_Print("\r\n");
}

static void Bootloader_DisableWatchdog(void)
{
    WDOG->CNT = BOOT_WDOG_UNLOCK_VALUE;

    WDOG->TOVAL = 0x0000FFFFu;

    /* Read-modify-write: clear EN, keep CMD32EN and CLK intact. */
    WDOG->CS = (WDOG->CS & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;
}

static bool Bootloader_Initialize(void)
{
    UartDriverStatusType uartStatus;

    Bootloader_DisableWatchdog();

    BootStateMachine_Init();
    ErrorHandler_Init();

    if (!ClockDriver_Init())
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            BOOT_INIT_STAGE_CLOCK);

        return false;
    }

    GpioDriver_Init();

    if (!BootButton_Init())
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            BOOT_INIT_STAGE_BOOT_BUTTON);

        return false;
    }

    AppValidator_Init();
    FlashJobQueue_Init();
    FlashEngine_Init();
    BootStatistic_Init();
    BootTasks_Init();

    uartStatus = UartDriver_Init(
        UART_DRIVER_CHANNEL_BOOTLOADER,
        BOOTLOADER_UART_BAUDRATE);

    if (uartStatus != UART_DRIVER_STATUS_OK)
    {
        ErrorHandler_SetFatal(
            BOOT_ERROR_INTERNAL_STATE,
            BOOT_INIT_STAGE_UART);

        return false;
    }

    return true;
}

static bool Bootloader_WaitForEntryRequest(void)
{
    volatile uint32_t remaining = BOOT_ENTRY_WINDOW_LOOPS;

    while (remaining > 0u)
    {
        if (!RxRingBuffer_IsEmpty())
        {
            return true;
        }

        remaining--;
    }

    return false;
}

static void Bootloader_ReportValidApplication(void)
{
    AppVectorInfoType vectorInfo = AppValidator_GetVectorInfo();

    Bootloader_Print("[BOOT] app_valid msp=");
    Bootloader_PrintHex32(vectorInfo.initialMsp);

    Bootloader_Print(" reset=");
    Bootloader_PrintHex32(vectorInfo.resetVector);

    Bootloader_Print("\r\n[BOOT] jumping to application\r\n");
}

static void Bootloader_ReportInvalidApplication(void)
{
    AppVectorInfoType vectorInfo = AppValidator_GetVectorInfo();

    Bootloader_Print("[BOOT] app_invalid diag=");

    Bootloader_Print(
        AppValidator_GetDiagnosticName(
            AppValidator_GetDiagnostic()));

    Bootloader_Print(" msp=");
    Bootloader_PrintHex32(vectorInfo.initialMsp);

    Bootloader_Print(" reset=");
    Bootloader_PrintHex32(vectorInfo.resetVector);

    Bootloader_Print("\r\n");
}


int main(void)
{
    bool stayInBootloader = false;

    if (Bootloader_Initialize())
    {
        Bootloader_ReportResetSource();

        /* Diagnostic only: shows the pin state without acting on it. */
        Bootloader_Print("[BOOT] button raw=");
        Bootloader_Print(BootButton_ReadRawLevel() ? "HIGH" : "LOW");
        Bootloader_Print("\r\n");
        Bootloader_Print("[BOOT] button decision=");
        Bootloader_Print(
        					BootButton_IsHeld()
							? "HELD"
							: "RELEASED");
        Bootloader_Print("\r\n");
#if (BOOT_BUTTON_ENTRY_ENABLED == 1u)

        if (BootButton_IsHeld())
        {
            Bootloader_Print(
                "[BOOT] entry requested by switch\r\n");

            stayInBootloader = true;
        }

#endif

        BootMenu_DisplayIfRequired();

        Bootloader_Print(
            "[BOOT] send any key within 2s to stay in bootloader\r\n");

        if (Bootloader_WaitForEntryRequest())
        {
            Bootloader_Print("[BOOT] entry requested over UART\r\n");
            stayInBootloader = true;
        }

        if (!stayInBootloader)
        {
            if (AppValidator_IsValid())
            {
                Bootloader_ReportValidApplication();

                JumpManager_JumpToApplication();
            }
            else
            {
                Bootloader_ReportInvalidApplication();

                stayInBootloader = true;
            }
        }

        if (stayInBootloader)
        {
        	if (!BootStateMachine_SetState(BOOT_STATE_MENU))
        	{
        		ErrorHandler_SetFatal(
            		BOOT_ERROR_INTERNAL_STATE,
					0U);
        	}
        	else
        	{
            BootMenu_DisplayIfRequired();
        	}
        }
    }

    Scheduler_Run();
}
