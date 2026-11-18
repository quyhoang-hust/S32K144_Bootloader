#include "clock_driver.h"
#include "device_registers.h"
#include "bootloader_config.h"

#define CLOCK_SOSC_FREQUENCY_HZ       (8000000u)

#define CLOCK_FIRC_FREQUENCY_HZ       (48000000u)

#define CLOCK_SPLL_FREQUENCY_HZ       (96000000u)
#define CLOCK_SPLL_DIV2_FREQUENCY_HZ  (48000000u)

#define CLOCK_CORE_FREQUENCY_HZ       (48000000u)  /* SPLL / 2 */
#define CLOCK_BUS_FREQUENCY_HZ        (24000000u)  /* CORE / 2 */
#define CLOCK_SLOW_FREQUENCY_HZ       (24000000u)  /* CORE / 2 */

#define CLOCK_TIMEOUT_ITERATIONS      (1000000u)

static bool g_clockInitialized = false;

static bool ClockDriver_ConfigureFirc(void)
{
    uint32_t timeout = CLOCK_TIMEOUT_ITERATIONS;

    while (((SCG->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK) == 0u) &&
           (timeout > 0u))
    {
        timeout--;
    }

    if (timeout == 0u)
    {
        return false;
    }

    SCG->FIRCDIV =
        (SCG->FIRCDIV & ~SCG_FIRCDIV_FIRCDIV2_MASK) |
        SCG_FIRCDIV_FIRCDIV2(1u);

    (void)SCG->FIRCDIV;

    return true;
}


static bool ClockDriver_ConfigureSystemOscillator(void)
{
    uint32_t timeout = CLOCK_TIMEOUT_ITERATIONS;

    SCG->SOSCCSR &= ~SCG_SOSCCSR_SOSCEN_MASK;

    SCG->SOSCCFG =
        SCG_SOSCCFG_EREFS_MASK |
        SCG_SOSCCFG_RANGE(2u);

    SCG->SOSCDIV =
        SCG_SOSCDIV_SOSCDIV1(1u) |
        SCG_SOSCDIV_SOSCDIV2(1u);

    SCG->SOSCCSR |= SCG_SOSCCSR_SOSCEN_MASK;

    while (((SCG->SOSCCSR & SCG_SOSCCSR_SOSCVLD_MASK) == 0u) &&
           (timeout > 0u))
    {
        timeout--;
    }

    return (timeout > 0u);
}

static bool ClockDriver_ConfigureSystemPll(void)
{
    uint32_t timeout = CLOCK_TIMEOUT_ITERATIONS;

    SCG->SPLLCSR &= ~SCG_SPLLCSR_SPLLEN_MASK;

    SCG->SPLLDIV =
        SCG_SPLLDIV_SPLLDIV1(2u) |
        SCG_SPLLDIV_SPLLDIV2(2u);

    SCG->SPLLCFG =
        SCG_SPLLCFG_PREDIV(0u) |
        SCG_SPLLCFG_MULT(8u);

    SCG->SPLLCSR |= SCG_SPLLCSR_SPLLEN_MASK;

    while (((SCG->SPLLCSR & SCG_SPLLCSR_SPLLVLD_MASK) == 0u) &&
           (timeout > 0u))
    {
        timeout--;
    }

    return (timeout > 0u);
}

static bool ClockDriver_SwitchRunClock(void)
{
    uint32_t timeout = CLOCK_TIMEOUT_ITERATIONS;

    SCG->RCCR =
        SCG_RCCR_SCS(6u) |
        SCG_RCCR_DIVCORE(1u) |
        SCG_RCCR_DIVBUS(1u) |
        SCG_RCCR_DIVSLOW(1u);

    while ((((SCG->CSR & SCG_CSR_SCS_MASK) >>
             SCG_CSR_SCS_SHIFT) != 6u) &&
           (timeout > 0u))
    {
        timeout--;
    }

    return (timeout > 0u);
}


bool ClockDriver_Init(void)
{
    g_clockInitialized = false;

    if (!ClockDriver_ConfigureFirc())
    {
        return false;
    }

    if (!ClockDriver_ConfigureSystemOscillator())
    {
        return false;
    }

    if (!ClockDriver_ConfigureSystemPll())
    {
        return false;
    }

    if (!ClockDriver_SwitchRunClock())
    {
        return false;
    }

    if (CLOCK_FIRC_FREQUENCY_HZ != BOOTLOADER_LPUART_CLOCK_HZ)
    {
        return false;
    }

    g_clockInitialized = true;

    return true;
}

uint32_t ClockDriver_GetCoreClockHz(void)
{
    return CLOCK_CORE_FREQUENCY_HZ;
}

uint32_t ClockDriver_GetBusClockHz(void)
{
    return CLOCK_BUS_FREQUENCY_HZ;
}

uint32_t ClockDriver_GetSlowClockHz(void)
{
    return CLOCK_SLOW_FREQUENCY_HZ;
}

uint32_t ClockDriver_GetLpuartClockHz(void)
{
    return CLOCK_FIRC_FREQUENCY_HZ;
}

bool ClockDriver_IsInitialized(void)
{
    return g_clockInitialized;
}
