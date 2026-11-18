#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdbool.h>
#include <stdint.h>

#define BOOTLOADER_START_ADDRESS             (0x00000000u)
#define BOOTLOADER_END_ADDRESS               (0x00020000u)

#define APPLICATION_START_ADDRESS            (0x00020000u)
#define APPLICATION_END_ADDRESS              (0x00080000u)

#define FLASH_CONFIG_FIELD_START_ADDRESS     (0x00000400u)
#define FLASH_CONFIG_FIELD_END_ADDRESS       (0x00000410u)


#define SRAM_START_ADDRESS                   (0x1FFF8000u)
#define SRAM_END_ADDRESS                     (0x20007000u)

#define FLASH_SECTOR_SIZE                    (4096u)
#define FLASH_PHRASE_SIZE                    (8u)

#define APPLICATION_SIZE                     \
    (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS)

#define APP_SECTOR_COUNT                     \
    (APPLICATION_SIZE / FLASH_SECTOR_SIZE)

#define APP_PHRASE_COUNT                     \
    (APPLICATION_SIZE / FLASH_PHRASE_SIZE)

/* Cortex-M vector table layout at the application base. */
#define APPLICATION_INITIAL_MSP_ADDRESS      \
    (APPLICATION_START_ADDRESS)

#define APPLICATION_RESET_VECTOR_ADDRESS     \
    (APPLICATION_START_ADDRESS + (uint32_t)sizeof(uint32_t))


static inline bool MemoryMap_IsBootloaderAddress(uint32_t address)
{
    return (address < BOOTLOADER_END_ADDRESS);
}

static inline bool MemoryMap_IsApplicationAddress(uint32_t address)
{
    return ((address >= APPLICATION_START_ADDRESS) &&
            (address < APPLICATION_END_ADDRESS));
}

static inline bool MemoryMap_IsFlashConfigFieldAddress(uint32_t address)
{
    return ((address >= FLASH_CONFIG_FIELD_START_ADDRESS) &&
            (address < FLASH_CONFIG_FIELD_END_ADDRESS));
}


static inline bool MemoryMap_IsSramAddress(uint32_t address)
{
    return ((address >= SRAM_START_ADDRESS) &&
            (address < SRAM_END_ADDRESS));
}

static inline bool MemoryMap_IsStackTopValid(uint32_t address)
{
    return ((address > SRAM_START_ADDRESS) &&
            (address <= SRAM_END_ADDRESS));
}

static inline bool MemoryMap_IsApplicationRangeValid(
    uint32_t address,
    uint32_t length)
{
    if (length == 0u)
    {
        return false;
    }

    if (address < APPLICATION_START_ADDRESS)
    {
        return false;
    }

    if (address >= APPLICATION_END_ADDRESS)
    {
        return false;
    }

    if (length > (APPLICATION_END_ADDRESS - address))
    {
        return false;
    }

    return true;
}

static inline bool MemoryMap_IsSectorAligned(uint32_t address)
{
    return ((address % FLASH_SECTOR_SIZE) == 0u);
}

static inline bool MemoryMap_IsPhraseAligned(uint32_t address)
{
    return ((address % FLASH_PHRASE_SIZE) == 0u);
}

static inline uint32_t MemoryMap_GetPhraseBaseAddress(uint32_t address)
{
    return (address & ~(FLASH_PHRASE_SIZE - 1u));
}

static inline bool MemoryMap_IsApplicationSectorValid(
    uint32_t sectorAddress)
{
    if (!MemoryMap_IsSectorAligned(sectorAddress))
    {
        return false;
    }

    return MemoryMap_IsApplicationRangeValid(
        sectorAddress,
        FLASH_SECTOR_SIZE);
}

static inline bool MemoryMap_IsApplicationPhraseValid(
    uint32_t phraseAddress)
{
    if (!MemoryMap_IsPhraseAligned(phraseAddress))
    {
        return false;
    }

    return MemoryMap_IsApplicationRangeValid(
        phraseAddress,
        FLASH_PHRASE_SIZE);
}

static inline bool MemoryMap_HasThumbBit(uint32_t vectorValue)
{
    return ((vectorValue & 1u) != 0u);
}

static inline uint32_t MemoryMap_GetHandlerAddress(uint32_t vectorValue)
{
    return (vectorValue & ~((uint32_t)1u));
}


#if (BOOTLOADER_START_ADDRESS >= BOOTLOADER_END_ADDRESS)
#error "Invalid bootloader address range"
#endif

#if (APPLICATION_START_ADDRESS >= APPLICATION_END_ADDRESS)
#error "Invalid application address range"
#endif

#if (BOOTLOADER_END_ADDRESS != APPLICATION_START_ADDRESS)
#error "Bootloader end must equal application start"
#endif

#if ((APPLICATION_SIZE % FLASH_SECTOR_SIZE) != 0u)
#error "Application region must contain whole flash sectors"
#endif

#if ((APPLICATION_SIZE % FLASH_PHRASE_SIZE) != 0u)
#error "Application region must contain whole flash phrases"
#endif

#if ((APPLICATION_START_ADDRESS % FLASH_SECTOR_SIZE) != 0u)
#error "Application start must be sector-aligned"
#endif

#if ((APPLICATION_START_ADDRESS % FLASH_PHRASE_SIZE) != 0u)
#error "Application start must be phrase-aligned"
#endif

#if ((FLASH_PHRASE_SIZE == 0u) || \
     ((FLASH_PHRASE_SIZE & (FLASH_PHRASE_SIZE - 1u)) != 0u))
#error "FLASH_PHRASE_SIZE must be a non-zero power of two"
#endif

#if ((FLASH_SECTOR_SIZE == 0u) || \
     ((FLASH_SECTOR_SIZE & (FLASH_SECTOR_SIZE - 1u)) != 0u))
#error "FLASH_SECTOR_SIZE must be a non-zero power of two"
#endif

#if (FLASH_CONFIG_FIELD_START_ADDRESS >= FLASH_CONFIG_FIELD_END_ADDRESS)
#error "Invalid Flash Configuration Field range"
#endif

#if ((FLASH_CONFIG_FIELD_START_ADDRESS < BOOTLOADER_START_ADDRESS) || \
     (FLASH_CONFIG_FIELD_END_ADDRESS > BOOTLOADER_END_ADDRESS))
#error "Flash Configuration Field must remain inside bootloader region"
#endif

#if (SRAM_START_ADDRESS >= SRAM_END_ADDRESS)
#error "Invalid SRAM address range"
#endif

#endif
