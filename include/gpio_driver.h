#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    GPIO_DRIVER_PORT_A = 0,
    GPIO_DRIVER_PORT_B,
    GPIO_DRIVER_PORT_C,
    GPIO_DRIVER_PORT_D,
    GPIO_DRIVER_PORT_E

} GpioDriverPortType;


typedef struct
{
    GpioDriverPortType port;
    uint32_t pin;
    bool pullEnable;
    bool pullUp;

} GpioDriverInputConfigType;

void GpioDriver_Init(void);


bool GpioDriver_ConfigureInput(
    const GpioDriverInputConfigType *config);

bool GpioDriver_ReadInput(
    GpioDriverPortType port,
    uint32_t pin,
    bool *level);

#endif
