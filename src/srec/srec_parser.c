#include "srec_parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "bootloader_config.h"

#define SREC_FIXED_CHARACTER_COUNT       (4u)

#define SREC_HEX_CHARACTERS_PER_BYTE     (2u)

#define SREC_CHECKSUM_BYTE_COUNT         (1u)

#define SREC_MAX_BYTE_COUNT              (255u)

#define SREC_PAYLOAD_CAPACITY                                   \
    ((uint16_t)(sizeof(((SrecRecordType *)0)->data)))

static SrecRecordTypeId g_lastDataRecordType = SREC_TYPE_UNKNOWN;

static void SrecParser_ClearRecord(SrecRecordType *record)
{
    if (record == NULL)
    {
        return;
    }

    record->type = SREC_TYPE_UNKNOWN;
    record->recordClass = SREC_RECORD_CLASS_UNKNOWN;
    record->address = 0u;
    record->dataLength = 0u;

    (void)memset(record->data, 0, sizeof(record->data));
}

static bool SrecParser_ConvertHexNibble(char character, uint8_t *nibble)
{
    if (nibble == NULL)
    {
        return false;
    }

    if ((character >= '0') && (character <= '9'))
    {
        *nibble = (uint8_t)(character - '0');
        return true;
    }

    if ((character >= 'A') && (character <= 'F'))
    {
        *nibble = (uint8_t)(character - 'A' + 10);
        return true;
    }

    if ((character >= 'a') && (character <= 'f'))
    {
        *nibble = (uint8_t)(character - 'a' + 10);
        return true;
    }

    return false;
}

static bool SrecParser_ParseHexByte(
    const char *line,
    uint32_t characterIndex,
    uint8_t *value)
{
    uint8_t highNibble;
    uint8_t lowNibble;

    if ((line == NULL) || (value == NULL))
    {
        return false;
    }

    if (!SrecParser_ConvertHexNibble(line[characterIndex], &highNibble))
    {
        return false;
    }

    if (!SrecParser_ConvertHexNibble(line[characterIndex + 1u], &lowNibble))
    {
        return false;
    }

    *value = (uint8_t)((highNibble << 4u) | lowNibble);

    return true;
}

static SrecRecordTypeId SrecParser_GetRecordType(char typeCharacter)
{
    switch (typeCharacter)
    {
        case '0': return SREC_TYPE_S0;
        case '1': return SREC_TYPE_S1;
        case '2': return SREC_TYPE_S2;
        case '3': return SREC_TYPE_S3;
        case '4': return SREC_TYPE_S4;
        case '5': return SREC_TYPE_S5;
        case '6': return SREC_TYPE_S6;
        case '7': return SREC_TYPE_S7;
        case '8': return SREC_TYPE_S8;
        case '9': return SREC_TYPE_S9;
        default:  return SREC_TYPE_UNKNOWN;
    }
}

static SrecRecordClassType SrecParser_GetRecordClass(SrecRecordTypeId type)
{
    switch (type)
    {
        case SREC_TYPE_S1:
        case SREC_TYPE_S2:
        case SREC_TYPE_S3:
            return SREC_RECORD_CLASS_DATA;

        case SREC_TYPE_S7:
        case SREC_TYPE_S8:
        case SREC_TYPE_S9:
            return SREC_RECORD_CLASS_END;

        case SREC_TYPE_S0:
        case SREC_TYPE_S5:
        case SREC_TYPE_S6:
            return SREC_RECORD_CLASS_INFO;

        case SREC_TYPE_S4:
        case SREC_TYPE_UNKNOWN:
        default:
            return SREC_RECORD_CLASS_UNSUPPORTED;
    }
}

static uint8_t SrecParser_GetAddressByteCount(SrecRecordTypeId type)
{
    switch (type)
    {
        case SREC_TYPE_S0:
        case SREC_TYPE_S1:
        case SREC_TYPE_S5:
        case SREC_TYPE_S9:
            return 2u;

        case SREC_TYPE_S2:
        case SREC_TYPE_S6:
        case SREC_TYPE_S8:
            return 3u;

        case SREC_TYPE_S3:
        case SREC_TYPE_S7:
            return 4u;

        default:
            return 0u;
    }
}

#if (SREC_TERMINATION_PAIR_CHECK_ENABLED == 1u)

static bool SrecParser_IsTerminationPairValid(
    SrecRecordTypeId dataType,
    SrecRecordTypeId endType)
{
    switch (dataType)
    {
        case SREC_TYPE_S1: return (endType == SREC_TYPE_S9);
        case SREC_TYPE_S2: return (endType == SREC_TYPE_S8);
        case SREC_TYPE_S3: return (endType == SREC_TYPE_S7);

        case SREC_TYPE_UNKNOWN:
        default:
            return false;
    }
}

#endif

void SrecParser_Init(void)
{
    g_lastDataRecordType = SREC_TYPE_UNKNOWN;
}

void SrecParser_Reset(void)
{
    g_lastDataRecordType = SREC_TYPE_UNKNOWN;
}

SrecParseResultType SrecParser_Parse(
    const char *line,
    uint32_t lineLength,
    SrecRecordType *record)
{
    SrecRecordTypeId recordType;
    SrecRecordClassType recordClass;

    uint8_t byteCount;
    uint8_t addressByteCount;
    uint8_t parsedByte;

    uint16_t dataLength;

    uint32_t expectedLineLength;
    uint32_t characterIndex;
    uint32_t address;
    uint32_t dataIndex;
    uint32_t countedByteIndex;
    uint32_t checksumSum;

    if ((line == NULL) || (record == NULL))
    {
        return SREC_PARSE_INVALID_ARGUMENT;
    }

    /* Shortest legal record is "Sx" + count + checksum. */
    if (lineLength < (SREC_FIXED_CHARACTER_COUNT +
                      SREC_HEX_CHARACTERS_PER_BYTE))
    {
        return SREC_PARSE_INVALID_LENGTH;
    }

    SrecParser_ClearRecord(record);

    if (line[0] != 'S')
    {
        return SREC_PARSE_INVALID_FORMAT;
    }

    recordType = SrecParser_GetRecordType(line[1]);

    if (recordType == SREC_TYPE_UNKNOWN)
    {
        return SREC_PARSE_INVALID_FORMAT;
    }

    recordClass = SrecParser_GetRecordClass(recordType);

    if (recordClass == SREC_RECORD_CLASS_UNSUPPORTED)
    {
        return SREC_PARSE_UNSUPPORTED_RECORD;
    }

    addressByteCount = SrecParser_GetAddressByteCount(recordType);

    if (addressByteCount == 0u)
    {
        return SREC_PARSE_INVALID_FORMAT;
    }

    if (!SrecParser_ParseHexByte(line, 2u, &byteCount))
    {
        return SREC_PARSE_INVALID_HEX;
    }

    if ((uint32_t)byteCount <
        ((uint32_t)addressByteCount + SREC_CHECKSUM_BYTE_COUNT))
    {
        return SREC_PARSE_INVALID_BYTE_COUNT;
    }

    expectedLineLength =
        SREC_FIXED_CHARACTER_COUNT +
        ((uint32_t)byteCount * SREC_HEX_CHARACTERS_PER_BYTE);

    if (lineLength != expectedLineLength)
    {
        return SREC_PARSE_INVALID_LENGTH;
    }

    dataLength = (uint16_t)((uint32_t)byteCount -
                            (uint32_t)addressByteCount -
                            SREC_CHECKSUM_BYTE_COUNT);

    if (dataLength > SREC_PAYLOAD_CAPACITY)
    {
        return SREC_PARSE_DATA_TOO_LONG;
    }

    if ((recordClass == SREC_RECORD_CLASS_END) && (dataLength != 0u))
    {
        return SREC_PARSE_INVALID_BYTE_COUNT;
    }

    checksumSum = byteCount;
    characterIndex = SREC_FIXED_CHARACTER_COUNT;

    for (countedByteIndex = 0u;
         countedByteIndex < byteCount;
         countedByteIndex++)
    {
        if (!SrecParser_ParseHexByte(line, characterIndex, &parsedByte))
        {
            return SREC_PARSE_INVALID_HEX;
        }

        checksumSum += parsedByte;
        characterIndex += SREC_HEX_CHARACTERS_PER_BYTE;
    }

    if ((checksumSum & 0xFFu) != 0xFFu)
    {
        return SREC_PARSE_INVALID_CHECKSUM;
    }

    /* Address field. */
    address = 0u;
    characterIndex = SREC_FIXED_CHARACTER_COUNT;

    for (countedByteIndex = 0u;
         countedByteIndex < addressByteCount;
         countedByteIndex++)
    {
        if (!SrecParser_ParseHexByte(line, characterIndex, &parsedByte))
        {
            return SREC_PARSE_INVALID_HEX;
        }

        address = (address << 8u) | (uint32_t)parsedByte;

        characterIndex += SREC_HEX_CHARACTERS_PER_BYTE;
    }

    if (recordClass == SREC_RECORD_CLASS_DATA)
    {
        for (dataIndex = 0u; dataIndex < dataLength; dataIndex++)
        {
            if (!SrecParser_ParseHexByte(line, characterIndex, &parsedByte))
            {
                return SREC_PARSE_INVALID_HEX;
            }

            record->data[dataIndex] = parsedByte;

            characterIndex += SREC_HEX_CHARACTERS_PER_BYTE;
        }
    }

#if (SREC_TERMINATION_PAIR_CHECK_ENABLED == 1u)

    if (recordClass == SREC_RECORD_CLASS_END)
    {
        if (!SrecParser_IsTerminationPairValid(g_lastDataRecordType,
                                               recordType))
        {
            return SREC_PARSE_INVALID_TERMINATION_PAIR;
        }
    }

#endif

    record->type = recordType;
    record->recordClass = recordClass;

    if (recordClass == SREC_RECORD_CLASS_DATA)
    {
        record->address = address;
        record->dataLength = dataLength;

        g_lastDataRecordType = recordType;
    }
    else if (recordClass == SREC_RECORD_CLASS_END)
    {
        record->address = address;
        record->dataLength = 0u;
    }
    else
    {

        record->address = 0u;
        record->dataLength = 0u;
    }

    return SREC_PARSE_OK;
}
