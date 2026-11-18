#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include "compiler_abstraction.h"

typedef uint32_t UartDriverChannelType;

#define UART_DRIVER_CHANNEL_BOOTLOADER       (0u)

typedef enum
{
    UART_DRIVER_STATUS_OK = 0,
    UART_DRIVER_STATUS_INVALID_ARGUMENT,
    UART_DRIVER_STATUS_INVALID_CHANNEL,
    UART_DRIVER_STATUS_INVALID_BAUDRATE,
    UART_DRIVER_STATUS_NOT_INITIALIZED,
    UART_DRIVER_STATUS_TX_TIMEOUT

} UartDriverStatusType;

UartDriverStatusType UartDriver_Init(
    UartDriverChannelType channel,
    uint32_t baudrate);

void UartDriver_Deinit(
    UartDriverChannelType channel);

UartDriverStatusType UartDriver_SendByte(
    UartDriverChannelType channel,
    uint8_t data);

UartDriverStatusType UartDriver_SendString(
    UartDriverChannelType channel,
    const char *text);

void UartDriver_EnableRxInterrupt(
    UartDriverChannelType channel);

void UartDriver_DisableRxInterrupt(
    UartDriverChannelType channel);

void UartDriver_ClearRxFlags(
    UartDriverChannelType channel);

bool UartDriver_IsInitialized(
    UartDriverChannelType channel);

RAM_FUNC void LPUART1_RxTx_IRQHandler(void);

#endif
