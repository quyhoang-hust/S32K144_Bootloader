#include "input_filter.h"

bool InputFilter_IsPrintableAscii(uint8_t data)
{
    return ((data >= 0x20u) &&
            (data <= 0x7Eu));
}

bool InputFilter_IsLineEnding(uint8_t data)
{
    return ((data == (uint8_t)'\r') ||
            (data == (uint8_t)'\n'));
}

bool InputFilter_IsAllowedByte(uint8_t data)
{
    return (InputFilter_IsPrintableAscii(data) ||
            InputFilter_IsLineEnding(data));
}
