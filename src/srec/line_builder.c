#include "line_builder.h"

#include <stddef.h>

#include "bootloader_config.h"
#include "input_filter.h"

static char g_lineBuffer[SREC_MAX_LINE_LENGTH + 1u];

static uint32_t g_lineLength = 0u;

static uint8_t g_lineComplete = 0u;

static void LineBuilder_TerminateBuffer(void)
{
    if (g_lineLength <= SREC_MAX_LINE_LENGTH)
    {
        g_lineBuffer[g_lineLength] = '\0';
    }
}

void LineBuilder_Init(void)
{
    g_lineLength = 0u;
    g_lineComplete = 0u;
    g_lineBuffer[0] = '\0';
}

void LineBuilder_Reset(void)
{
    g_lineLength = 0u;
    g_lineComplete = 0u;
    g_lineBuffer[0] = '\0';
}

LineBuilderResultType LineBuilder_PushByte(uint8_t data)
{

    if (g_lineComplete != 0u)
    {
        return LINE_BUILDER_INTERNAL_ERROR;
    }

    if (!InputFilter_IsAllowedByte(data))
    {
        return LINE_BUILDER_INVALID_CHARACTER;
    }

    if (InputFilter_IsLineEnding(data))
    {
        if (g_lineLength == 0u)
        {

            return LINE_BUILDER_EMPTY_LINE;
        }

        LineBuilder_TerminateBuffer();
        g_lineComplete = 1u;

        return LINE_BUILDER_COMPLETE;
    }

    if ((g_lineLength == 0u) &&
        (data != (uint8_t)'S'))
    {
        return LINE_BUILDER_INVALID_START;
    }

    if (g_lineLength >= SREC_MAX_LINE_LENGTH)
    {
        return LINE_BUILDER_TOO_LONG;
    }

    g_lineBuffer[g_lineLength] = (char)data;
    g_lineLength++;

    LineBuilder_TerminateBuffer();

    return LINE_BUILDER_IN_PROGRESS;
}

const char *LineBuilder_GetLine(void)
{
    return g_lineBuffer;
}

uint32_t LineBuilder_GetLength(void)
{
    return g_lineLength;
}

uint8_t LineBuilder_IsEmpty(void)
{
    return (g_lineLength == 0u) ? 1u : 0u;
}
