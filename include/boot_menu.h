#ifndef BOOT_MENU_H
#define BOOT_MENU_H

#include <stdint.h>

typedef enum
{

    BOOT_MENU_COMMAND_NONE = 0,

    BOOT_MENU_COMMAND_ERASE_APPLICATION,

    BOOT_MENU_COMMAND_START_DOWNLOAD,

    BOOT_MENU_COMMAND_INVALID

} BootMenuCommandType;

void BootMenu_Init(void);

void BootMenu_Reset(void);

void BootMenu_DisplayIfRequired(void);

BootMenuCommandType BootMenu_ProcessByte(uint8_t data);

void BootMenu_ReportInvalidOption(void);

void BootMenu_ReportDownloadReady(void);

void BootMenu_ReportEraseDone(void);

#endif
