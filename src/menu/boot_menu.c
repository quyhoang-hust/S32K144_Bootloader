#include "boot_menu.h"
#include <stdbool.h>
#include "uart_driver.h"

static const char g_menuText[] =
    "\r\n"
    "==== S32K144 Bootloader Menu ====\r\n"
    "1. Erase application region\r\n"
    "2. Start download program\r\n"
    "Select option: ";

static const char g_invalidOptionText[] =
    "\r\n[BOOT] Invalid option\r\n";

static const char g_downloadReadyText[] =
    "\r\n[BOOT] Ready to receive SREC\r\n";

static const char g_eraseDoneText[] =
    "\r\n[BOOT] Erase application done\r\n";

static bool g_menuDisplayed = false;

static void BootMenu_SendText(const char *text)
{
    (void)UartDriver_SendString(
        UART_DRIVER_CHANNEL_BOOTLOADER,
        text);
}

void BootMenu_Init(void)
{
    g_menuDisplayed = false;
}

void BootMenu_Reset(void)
{
    g_menuDisplayed = false;
}

void BootMenu_DisplayIfRequired(void)
{
    if (g_menuDisplayed)
    {
        return;
    }

    BootMenu_SendText(g_menuText);

    g_menuDisplayed = true;
}

BootMenuCommandType BootMenu_ProcessByte(uint8_t data)
{
    if ((data == (uint8_t)'\r') ||
        (data == (uint8_t)'\n'))
    {
        return BOOT_MENU_COMMAND_NONE;
    }

    if (data == (uint8_t)'1')
    {
        return BOOT_MENU_COMMAND_ERASE_APPLICATION;
    }

    if (data == (uint8_t)'2')
    {
        return BOOT_MENU_COMMAND_START_DOWNLOAD;
    }

    return BOOT_MENU_COMMAND_INVALID;
}

void BootMenu_ReportInvalidOption(void)
{
    BootMenu_SendText(g_invalidOptionText);
    g_menuDisplayed = false;
}

void BootMenu_ReportDownloadReady(void)
{
    BootMenu_SendText(g_downloadReadyText);
}

void BootMenu_ReportEraseDone(void)
{
    BootMenu_SendText(g_eraseDoneText);
    g_menuDisplayed = false;
}
