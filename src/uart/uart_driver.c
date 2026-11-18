#include "uart_driver.h"
#include <stddef.h>
#include "device_registers.h"

#include "boot_state_machine.h"
#include "bootloader_config.h"
#include "compiler_abstraction.h"
#include "error_handler.h"
#include "rx_ring_buffer.h"

#ifndef BOOTLOADER_LPUART_CLOCK_HZ
#error "BOOTLOADER_LPUART_CLOCK_HZ must be defined in bootloader_config.h"
#endif

#ifndef BOOTLOADER_LPUART_PCC_PCS
#error "BOOTLOADER_LPUART_PCC_PCS must be defined in bootloader_config.h"
#endif


#define UART_NVIC_ISER_BASE_ADDRESS     (0xE000E100u)
#define UART_NVIC_ICER_BASE_ADDRESS     (0xE000E180u)
#define UART_NVIC_ICPR_BASE_ADDRESS     (0xE000E280u)

#define UART_REG32(address)             \
    (*(volatile uint32_t *)(address))

static RAM_FUNC void UartDriver_NvicEnableIrq(int32_t irqNumber)
{
    const uint32_t irqIndex = (uint32_t)irqNumber;

    UART_REG32(
        UART_NVIC_ISER_BASE_ADDRESS +
        ((irqIndex >> 5u) * 4u)) =
            (uint32_t)(1UL << (irqIndex & 0x1Fu));
}

static RAM_FUNC void UartDriver_NvicDisableIrq(int32_t irqNumber)
{
    const uint32_t irqIndex = (uint32_t)irqNumber;

    UART_REG32(
        UART_NVIC_ICER_BASE_ADDRESS +
        ((irqIndex >> 5u) * 4u)) =
            (uint32_t)(1UL << (irqIndex & 0x1Fu));
}

static RAM_FUNC void UartDriver_NvicClearPendingIrq(int32_t irqNumber)
{
    const uint32_t irqIndex = (uint32_t)irqNumber;

    UART_REG32(
        UART_NVIC_ICPR_BASE_ADDRESS +
        ((irqIndex >> 5u) * 4u)) =
            (uint32_t)(1UL << (irqIndex & 0x1Fu));
}


typedef struct
{
    LPUART_Type *uartBase;
    PORT_Type *portBase;

    uint32_t rxPin;
    uint32_t txPin;

    uint32_t pccUartIndex;
    uint32_t pccPortIndex;

    int32_t irqNumber;

    uint32_t pinMux;

} UartDriverMappingType;

static const UartDriverMappingType g_uartMapping[] =
{
    {
        LPUART1,
        PORTC,

        6u,
        7u,

        PCC_LPUART1_INDEX,
        PCC_PORTC_INDEX,

        (int32_t)LPUART1_RxTx_IRQn,

        2u
    }
};

#define UART_DRIVER_CHANNEL_COUNT \
    ((uint32_t)(sizeof(g_uartMapping) / sizeof(g_uartMapping[0])))

#define UART_OVERSAMPLING_REGISTER_VALUE     (15u)

#define UART_OVERSAMPLING_RATIO              \
    (UART_OVERSAMPLING_REGISTER_VALUE + 1u)

static bool g_uartInitialized[UART_DRIVER_CHANNEL_COUNT] =
{
    false
};


static bool UartDriver_IsChannelValid(
    UartDriverChannelType channel)
{
    return (channel < UART_DRIVER_CHANNEL_COUNT);
}

static void UartDriver_MaskChannelIrq(
    const UartDriverMappingType *mapping)
{
    UartDriver_NvicDisableIrq(mapping->irqNumber);
    UartDriver_NvicClearPendingIrq(mapping->irqNumber);
}

static uint32_t UartDriver_CalculateSbr(
    uint32_t baudrate)
{
    const uint32_t denominator =
        UART_OVERSAMPLING_RATIO *
        baudrate;

    return ((BOOTLOADER_LPUART_CLOCK_HZ +
             (denominator / 2u)) /
            denominator);
}

static void UartDriver_ConfigurePort(
    const UartDriverMappingType *mapping)
{
    PCC->PCCn[mapping->pccPortIndex] |=
        PCC_PCCn_CGC_MASK;

    /* Pull-up keeps the idle line high if the cable is disconnected. */
    mapping->portBase->PCR[mapping->rxPin] =
        PORT_PCR_MUX(mapping->pinMux) |
        PORT_PCR_PE_MASK |
        PORT_PCR_PS_MASK;

    mapping->portBase->PCR[mapping->txPin] =
        PORT_PCR_MUX(mapping->pinMux) |
        PORT_PCR_PE_MASK |
        PORT_PCR_PS_MASK;
}

static void UartDriver_ConfigureClock(
    const UartDriverMappingType *mapping)
{

    PCC->PCCn[mapping->pccUartIndex] &=
        ~PCC_PCCn_CGC_MASK;

    PCC->PCCn[mapping->pccUartIndex] =
        PCC_PCCn_PCS(BOOTLOADER_LPUART_PCC_PCS);

    PCC->PCCn[mapping->pccUartIndex] |=
        PCC_PCCn_CGC_MASK;

    (void)PCC->PCCn[mapping->pccUartIndex];
}

static void UartDriver_ResetPeripheral(
    const UartDriverMappingType *mapping)
{
    mapping->uartBase->CTRL = 0u;

    mapping->uartBase->BAUD = 0u;

    mapping->uartBase->STAT =
        LPUART_STAT_OR_MASK |
        LPUART_STAT_NF_MASK |
        LPUART_STAT_FE_MASK |
        LPUART_STAT_PF_MASK;
}

static bool UartDriver_ConfigureBaudrate(
    const UartDriverMappingType *mapping,
    uint32_t baudrate)
{
    uint32_t sbr;

    if (baudrate == 0u)
    {
        return false;
    }

    sbr = UartDriver_CalculateSbr(baudrate);

    if ((sbr == 0u) ||
        (sbr > 0x1FFFu))
    {
        return false;
    }

    mapping->uartBase->BAUD =
        LPUART_BAUD_OSR(
            UART_OVERSAMPLING_REGISTER_VALUE) |
        LPUART_BAUD_SBR(sbr);

    mapping->uartBase->CTRL = 0u;

    return true;
}

UartDriverStatusType UartDriver_Init(
    UartDriverChannelType channel,
    uint32_t baudrate)
{
    const UartDriverMappingType *mapping;

    if (!UartDriver_IsChannelValid(channel))
    {
        return UART_DRIVER_STATUS_INVALID_CHANNEL;
    }

    if (baudrate == 0u)
    {
        return UART_DRIVER_STATUS_INVALID_BAUDRATE;
    }

    mapping = &g_uartMapping[channel];

    g_uartInitialized[channel] = false;

    UartDriver_MaskChannelIrq(mapping);

    UartDriver_ConfigurePort(mapping);
    UartDriver_ConfigureClock(mapping);

    UartDriver_ResetPeripheral(mapping);

    if (!UartDriver_ConfigureBaudrate(
            mapping,
            baudrate))
    {
        return UART_DRIVER_STATUS_INVALID_BAUDRATE;
    }

    RxRingBuffer_Init();

    g_uartInitialized[channel] = true;

    UartDriver_ClearRxFlags(channel);

    mapping->uartBase->CTRL |=
        LPUART_CTRL_TE_MASK |
        LPUART_CTRL_RE_MASK;

    UartDriver_EnableRxInterrupt(channel);

    return UART_DRIVER_STATUS_OK;
}

void UartDriver_Deinit(
    UartDriverChannelType channel)
{
    const UartDriverMappingType *mapping;

    if (!UartDriver_IsChannelValid(channel))
    {
        return;
    }

    mapping = &g_uartMapping[channel];

    UartDriver_NvicDisableIrq(mapping->irqNumber);
    UartDriver_NvicClearPendingIrq(mapping->irqNumber);

    g_uartInitialized[channel] = false;
}

UartDriverStatusType UartDriver_SendByte(
    UartDriverChannelType channel,
    uint8_t data)
{
    const UartDriverMappingType *mapping;
    uint32_t pollCount = 0u;

    if (!UartDriver_IsChannelValid(channel))
    {
        return UART_DRIVER_STATUS_INVALID_CHANNEL;
    }

    if (!g_uartInitialized[channel])
    {
        return UART_DRIVER_STATUS_NOT_INITIALIZED;
    }

    mapping = &g_uartMapping[channel];

    while ((mapping->uartBase->STAT &
            LPUART_STAT_TDRE_MASK) == 0u)
    {
        pollCount++;

        if (pollCount >= BOOTLOADER_UART_TX_POLL_LIMIT)
        {
            return UART_DRIVER_STATUS_TX_TIMEOUT;
        }
    }

    mapping->uartBase->DATA =
        (uint32_t)data;

    return UART_DRIVER_STATUS_OK;
}

UartDriverStatusType UartDriver_SendString(
    UartDriverChannelType channel,
    const char *text)
{
    UartDriverStatusType status;

    if (!UartDriver_IsChannelValid(channel))
    {
        return UART_DRIVER_STATUS_INVALID_CHANNEL;
    }

    if (text == NULL)
    {
        return UART_DRIVER_STATUS_INVALID_ARGUMENT;
    }

    if (!g_uartInitialized[channel])
    {
        return UART_DRIVER_STATUS_NOT_INITIALIZED;
    }

    while (*text != '\0')
    {
        status = UartDriver_SendByte(
            channel,
            (uint8_t)*text);

        if (status != UART_DRIVER_STATUS_OK)
        {
            return status;
        }

        text++;
    }

    return UART_DRIVER_STATUS_OK;
}

void UartDriver_EnableRxInterrupt(
    UartDriverChannelType channel)
{
    const UartDriverMappingType *mapping;

    if (!UartDriver_IsChannelValid(channel))
    {
        return;
    }

    mapping = &g_uartMapping[channel];

    UartDriver_NvicClearPendingIrq(mapping->irqNumber);

    if (g_uartInitialized[channel])
    {
        mapping->uartBase->CTRL |=
            LPUART_CTRL_RE_MASK |
            LPUART_CTRL_RIE_MASK;
    }

    UartDriver_NvicEnableIrq(mapping->irqNumber);
}

void UartDriver_DisableRxInterrupt(
    UartDriverChannelType channel)
{
    const UartDriverMappingType *mapping;

    if (!UartDriver_IsChannelValid(channel))
    {
        return;
    }

    mapping = &g_uartMapping[channel];

    UartDriver_NvicDisableIrq(mapping->irqNumber);

    if (g_uartInitialized[channel])
    {
        mapping->uartBase->CTRL &=
            ~LPUART_CTRL_RIE_MASK;
    }

    UartDriver_NvicClearPendingIrq(mapping->irqNumber);
}

void UartDriver_ClearRxFlags(
    UartDriverChannelType channel)
{
    const UartDriverMappingType *mapping;
    volatile uint32_t dummyData;

    if (!UartDriver_IsChannelValid(channel))
    {
        return;
    }

    mapping = &g_uartMapping[channel];

    if (g_uartInitialized[channel])
    {
        if ((mapping->uartBase->STAT &
             LPUART_STAT_RDRF_MASK) != 0u)
        {
            dummyData =
                mapping->uartBase->DATA;

            COMPILER_UNUSED(dummyData);
        }

        mapping->uartBase->STAT =
            LPUART_STAT_OR_MASK |
            LPUART_STAT_NF_MASK |
            LPUART_STAT_FE_MASK |
            LPUART_STAT_PF_MASK;
    }

    UartDriver_NvicClearPendingIrq(mapping->irqNumber);
}

bool UartDriver_IsInitialized(
    UartDriverChannelType channel)
{
    if (!UartDriver_IsChannelValid(channel))
    {
        return false;
    }

    return g_uartInitialized[channel];
}

RAM_FUNC void LPUART1_RxTx_IRQHandler(void)
{
    const uint32_t status =
        LPUART1->STAT;

    const uint32_t errorMask =
        LPUART_STAT_OR_MASK |
        LPUART_STAT_NF_MASK |
        LPUART_STAT_FE_MASK |
        LPUART_STAT_PF_MASK;

    uint8_t receivedByte;

    if ((status & errorMask) != 0u)
    {
        volatile uint32_t dummyData =
            LPUART1->DATA;

        COMPILER_UNUSED(dummyData);

        LPUART1->STAT =
            (status & errorMask);

        if ((status & LPUART_STAT_OR_MASK) != 0u)
        {
            ErrorHandler_SetFatalFromIsr(
                BOOT_ERROR_RX_OVERFLOW,
                0u);
        }
        else if (BootStateMachine_GetState() ==
                 BOOT_STATE_RECEIVING)
        {
            ErrorHandler_SetFatalFromIsr(
                BOOT_ERROR_INVALID_SREC,
                0u);
        }
        else
        {
            /* Framing noise outside RECEIVING is ignored. */
        }

        return;
    }

    if ((status & LPUART_STAT_RDRF_MASK) == 0u)
    {
        return;
    }

    receivedByte =
        (uint8_t)(LPUART1->DATA & 0xFFu);

    if (!BootStateMachine_IsUartInputAccepted())
    {
        return;
    }

    if (!RxRingBuffer_PushFromIsr(receivedByte))
    {
        ErrorHandler_SetFatalFromIsr(
            BOOT_ERROR_RX_OVERFLOW,
            0u);
    }
}
