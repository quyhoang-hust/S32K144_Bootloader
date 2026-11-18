#include "boot_button.h"
#include <stdint.h>
#include "gpio_driver.h"

#define BOOT_BUTTON_PORT                (GPIO_DRIVER_PORT_C)
#define BOOT_BUTTON_PIN                 (12u)

#define BOOT_BUTTON_ACTIVE_LEVEL        (true)

#define BOOT_BUTTON_SETTLE_LOOPS        (20000u)

#define BOOT_BUTTON_SAMPLE_COUNT        (5u)

#define BOOT_BUTTON_SAMPLE_GAP_LOOPS    (200u)

static bool g_bootButtonInitialized = false;

static void BootButton_Delay(volatile uint32_t loops)
{
    while (loops > 0u)
    {
        loops--;
    }
}

static bool BootButton_SampleRaw(bool *pinLevel)
{
    return GpioDriver_ReadInput(
        BOOT_BUTTON_PORT,
        BOOT_BUTTON_PIN,
        pinLevel);
}

bool BootButton_Init(void)
{
    GpioDriverInputConfigType inputConfig;
    bool pinLevel;

    g_bootButtonInitialized = false;

    inputConfig.port = BOOT_BUTTON_PORT;
    inputConfig.pin = BOOT_BUTTON_PIN;
    inputConfig.pullEnable = false;
    inputConfig.pullUp = false;

    if (!GpioDriver_ConfigureInput(&inputConfig))
    {
        return false;
    }

    BootButton_Delay(BOOT_BUTTON_SETTLE_LOOPS);

    if (!BootButton_SampleRaw(&pinLevel))
    {
        return false;
    }

    g_bootButtonInitialized = true;

    return true;
}

bool BootButton_IsHeld(void)
{
    uint32_t index;
    uint32_t activeCount = 0u;
    bool pinLevel;

    if (!g_bootButtonInitialized)
    {

        return false;
    }

    for (index = 0u;
         index < BOOT_BUTTON_SAMPLE_COUNT;
         index++)
    {
        if (!BootButton_SampleRaw(&pinLevel))
        {
            return false;
        }

        if (pinLevel == BOOT_BUTTON_ACTIVE_LEVEL)
        {
            activeCount++;
        }

        BootButton_Delay(BOOT_BUTTON_SAMPLE_GAP_LOOPS);
    }

    /* Majority vote: rejects bounce and single-sample glitches. */
    return (activeCount > (BOOT_BUTTON_SAMPLE_COUNT / 2u));
}

bool BootButton_IsInitialized(void)
{
    return g_bootButtonInitialized;
}

bool BootButton_ReadRawLevel(void)
{
    bool pinLevel = true;

    (void)BootButton_SampleRaw(&pinLevel);

    return pinLevel;
}
