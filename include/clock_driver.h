#ifndef CLOCK_DRIVER_H
#define CLOCK_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

bool ClockDriver_Init(void);

uint32_t ClockDriver_GetCoreClockHz(void);

uint32_t ClockDriver_GetBusClockHz(void);

uint32_t ClockDriver_GetSlowClockHz(void);

uint32_t ClockDriver_GetLpuartClockHz(void);

bool ClockDriver_IsInitialized(void);

#endif
