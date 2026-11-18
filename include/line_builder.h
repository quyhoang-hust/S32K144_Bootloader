#ifndef LINE_BUILDER_H
#define LINE_BUILDER_H

#include <stdint.h>

typedef enum
{

    LINE_BUILDER_IN_PROGRESS = 0,

    LINE_BUILDER_COMPLETE,

    LINE_BUILDER_EMPTY_LINE,

    LINE_BUILDER_INVALID_START,

    LINE_BUILDER_INVALID_CHARACTER,

    LINE_BUILDER_TOO_LONG,

    LINE_BUILDER_INTERNAL_ERROR

} LineBuilderResultType;

void LineBuilder_Init(void);

void LineBuilder_Reset(void);

LineBuilderResultType LineBuilder_PushByte(uint8_t data);

const char *LineBuilder_GetLine(void);

uint32_t LineBuilder_GetLength(void);

uint8_t LineBuilder_IsEmpty(void);

#endif
