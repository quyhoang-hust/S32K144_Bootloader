#ifndef SREC_TYPES_H
#define SREC_TYPES_H

#include <stdint.h>

#include "bootloader_config.h"

/*
 * ============================================================================
 * SREC TYPE ID
 * ============================================================================
 */

typedef enum
{
    SREC_TYPE_UNKNOWN = 0,

    SREC_TYPE_S0,
    SREC_TYPE_S1,
    SREC_TYPE_S2,
    SREC_TYPE_S3,
    SREC_TYPE_S4,
    SREC_TYPE_S5,
    SREC_TYPE_S6,
    SREC_TYPE_S7,
    SREC_TYPE_S8,
    SREC_TYPE_S9

} SrecRecordTypeId;

/*
 * ============================================================================
 * RECORD CLASS
 * ============================================================================
 *
 * DATA
 *     S1 S2 S3
 *
 * END
 *     S7 S8 S9
 *
 * INFO
 *     S0 S5 S6
 *
 * UNSUPPORTED
 *     S4
 *     UNKNOWN
 *
 * This separates valid informational records from
 * unsupported records.
 */

typedef enum
{
    SREC_RECORD_CLASS_UNKNOWN = 0,

    SREC_RECORD_CLASS_DATA,

    SREC_RECORD_CLASS_END,

    SREC_RECORD_CLASS_INFO,

    SREC_RECORD_CLASS_UNSUPPORTED

} SrecRecordClassType;

/*
 * ============================================================================
 * PARSED RECORD
 * ============================================================================
 */

typedef struct
{
    SrecRecordTypeId type;

    SrecRecordClassType recordClass;

    uint32_t address;

    uint16_t dataLength;

    uint8_t data[SREC_MAX_DATA_SIZE];

} SrecRecordType;

#endif
