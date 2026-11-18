#include "rx_ring_buffer.h"

#include <stddef.h>

#include "bootloader_config.h"
#include "compiler_abstraction.h"

BOOT_STATIC_ASSERT(
    RX_RING_BUFFER_SIZE > 1u,
    "RX_RING_BUFFER_SIZE must be greater than one");

BOOT_STATIC_ASSERT(
    (RX_RING_BUFFER_SIZE &
     (RX_RING_BUFFER_SIZE - 1u)) == 0u,
    "RX_RING_BUFFER_SIZE must be a power of two");

#define RX_RING_BUFFER_INDEX_MASK \
    (RX_RING_BUFFER_SIZE - 1u)

static uint8_t g_rxBuffer[RX_RING_BUFFER_SIZE];

static volatile uint32_t g_headIndex = 0u;
static volatile uint32_t g_tailIndex = 0u;

void RxRingBuffer_Init(void)
{
    g_headIndex = 0u;
    g_tailIndex = 0u;
}

void RxRingBuffer_Clear(void)
{
    g_headIndex = 0u;
    g_tailIndex = 0u;
    COMPILER_MEMORY_BARRIER();
}

RAM_FUNC bool RxRingBuffer_PushFromIsr(uint8_t data)
{
    const uint32_t currentHead = g_headIndex;

    const uint32_t nextHead =
        (currentHead + 1u) &
        RX_RING_BUFFER_INDEX_MASK;

    if (nextHead == g_tailIndex)
    {
        return false;
    }

    g_rxBuffer[currentHead] = data;

    COMPILER_MEMORY_BARRIER();

    g_headIndex = nextHead;

    return true;
}

bool RxRingBuffer_Pop(uint8_t *data)
{
    const uint32_t currentTail = g_tailIndex;

    if (data == NULL)
    {
        return false;
    }

    if (currentTail == g_headIndex)
    {

        return false;
    }

    *data = g_rxBuffer[currentTail];

    COMPILER_MEMORY_BARRIER();

    g_tailIndex =
        (currentTail + 1u) &
        RX_RING_BUFFER_INDEX_MASK;

    return true;
}

bool RxRingBuffer_IsEmpty(void)
{
    return (g_headIndex == g_tailIndex);
}

bool RxRingBuffer_IsFull(void)
{
    const uint32_t nextHead =
        (g_headIndex + 1u) &
        RX_RING_BUFFER_INDEX_MASK;

    return (nextHead == g_tailIndex);
}

uint32_t RxRingBuffer_GetUsage(void)
{
    const uint32_t headSnapshot = g_headIndex;
    const uint32_t tailSnapshot = g_tailIndex;

    return ((headSnapshot - tailSnapshot) &
            RX_RING_BUFFER_INDEX_MASK);
}

uint32_t RxRingBuffer_GetCapacity(void)
{
    return (RX_RING_BUFFER_SIZE - 1u);
}
