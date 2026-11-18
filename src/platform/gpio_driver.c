#include "gpio_driver.h"
#include <stddef.h>
#include "device_registers.h"

typedef struct
{
    PORT_Type *portBase;
    GPIO_Type *gpioBase;
    uint32_t pccIndex;

} GpioDriverPortMappingType;

static const GpioDriverPortMappingType g_gpioPortMapping[] =
{
    { PORTA, PTA, PCC_PORTA_INDEX },
    { PORTB, PTB, PCC_PORTB_INDEX },
    { PORTC, PTC, PCC_PORTC_INDEX },
    { PORTD, PTD, PCC_PORTD_INDEX },
    { PORTE, PTE, PCC_PORTE_INDEX }
};

#define GPIO_DRIVER_PORT_COUNT \
    ((uint32_t)(sizeof(g_gpioPortMapping) / \
                sizeof(g_gpioPortMapping[0])))

#define GPIO_DRIVER_MAX_PIN_NUMBER (31u)


static bool GpioDriver_IsPortValid(
    GpioDriverPortType port)
{
    return ((uint32_t)port <
            GPIO_DRIVER_PORT_COUNT);
}

static bool GpioDriver_IsPinValid(uint32_t pin)
{
    return (pin <=
            GPIO_DRIVER_MAX_PIN_NUMBER);
}


void GpioDriver_Init(void)
{
}

bool GpioDriver_ConfigureInput(
    const GpioDriverInputConfigType *config)
{
    const GpioDriverPortMappingType *mapping;
    uint32_t pinControl;

    if (config == NULL)
    {
        return false;
    }

    if (!GpioDriver_IsPortValid(
            config->port))
    {
        return false;
    }

    if (!GpioDriver_IsPinValid(
            config->pin))
    {
        return false;
    }

    mapping =
        &g_gpioPortMapping[config->port];

    PCC->PCCn[mapping->pccIndex] |=
        PCC_PCCn_CGC_MASK;

    pinControl =
        PORT_PCR_MUX(1u);

    if (config->pullEnable)
    {
        pinControl |=
            PORT_PCR_PE_MASK;

        if (config->pullUp)
        {
            pinControl |=
                PORT_PCR_PS_MASK;
        }
    }

    mapping->portBase->PCR[config->pin] =
        pinControl;

    mapping->gpioBase->PDDR &=
        ~(1UL << config->pin);

    return true;
}

bool GpioDriver_ReadInput(
    GpioDriverPortType port,
    uint32_t pin,
    bool *level)
{
    const GpioDriverPortMappingType *mapping;

    if (level == NULL)
    {
        return false;
    }

    if (!GpioDriver_IsPortValid(port) ||
        !GpioDriver_IsPinValid(pin))
    {
        return false;
    }

    mapping =
        &g_gpioPortMapping[port];

    *level =
        ((mapping->gpioBase->PDIR &
          (1UL << pin)) != 0u);

    return true;
}
