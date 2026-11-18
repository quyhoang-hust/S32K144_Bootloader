#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H

#include <stdbool.h>
#include <stdint.h>

bool InputFilter_IsPrintableAscii(uint8_t data);
bool InputFilter_IsLineEnding(uint8_t data);
bool InputFilter_IsAllowedByte(uint8_t data);

#endif
