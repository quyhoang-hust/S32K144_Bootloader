#ifndef SREC_PARSER_H
#define SREC_PARSER_H

#include <stdint.h>

#include "srec_types.h"

typedef enum
{
    SREC_PARSE_OK = 0,

    SREC_PARSE_INVALID_ARGUMENT,

    SREC_PARSE_INVALID_LENGTH,

    SREC_PARSE_INVALID_FORMAT,

    SREC_PARSE_INVALID_HEX,

    SREC_PARSE_INVALID_BYTE_COUNT,

    SREC_PARSE_INVALID_CHECKSUM,

    /*
     * S4
     * unknown record
     */
    SREC_PARSE_UNSUPPORTED_RECORD,

    /*
     * S1->S9
     * S2->S8
     * S3->S7
     */
    SREC_PARSE_INVALID_TERMINATION_PAIR,

    SREC_PARSE_DATA_TOO_LONG

} SrecParseResultType;

void SrecParser_Init(void);

void SrecParser_Reset(void);

SrecParseResultType SrecParser_Parse(
    const char *line,
    uint32_t lineLength,
    SrecRecordType *record);

#endif
