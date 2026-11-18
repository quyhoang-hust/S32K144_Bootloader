#ifndef BOOT_BUTTON_H
#define BOOT_BUTTON_H

#include <stdbool.h>

bool BootButton_Init(void);

bool BootButton_IsHeld(void);

bool BootButton_IsInitialized(void);

bool BootButton_ReadRawLevel(void);

#endif
