#ifndef RX_RING_BUFFER_H
#define RX_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

#include "compiler_abstraction.h"

void RxRingBuffer_Init(void);

void RxRingBuffer_Clear(void);

RAM_FUNC bool RxRingBuffer_PushFromIsr(uint8_t data);

bool RxRingBuffer_Pop(uint8_t *data);

bool RxRingBuffer_IsEmpty(void);

bool RxRingBuffer_IsFull(void);

uint32_t RxRingBuffer_GetUsage(void);

uint32_t RxRingBuffer_GetCapacity(void);

#endif
