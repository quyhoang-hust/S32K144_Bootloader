#include "flash_driver.h"

#include <stddef.h>
#include <stdint.h>

#include "device_registers.h"

#include "compiler_abstraction.h"
#include "memory_map.h"


#define FTFC_FCCOB0_REG     (FTFC->FCCOB[3])
#define FTFC_FCCOB1_REG     (FTFC->FCCOB[2])
#define FTFC_FCCOB2_REG     (FTFC->FCCOB[1])
#define FTFC_FCCOB3_REG     (FTFC->FCCOB[0])

#define FTFC_FCCOB4_REG     (FTFC->FCCOB[7])
#define FTFC_FCCOB5_REG     (FTFC->FCCOB[6])
#define FTFC_FCCOB6_REG     (FTFC->FCCOB[5])
#define FTFC_FCCOB7_REG     (FTFC->FCCOB[4])

#define FTFC_FCCOB8_REG     (FTFC->FCCOB[11])
#define FTFC_FCCOB9_REG     (FTFC->FCCOB[10])
#define FTFC_FCCOBA_REG     (FTFC->FCCOB[9])
#define FTFC_FCCOBB_REG     (FTFC->FCCOB[8])

#define FLASH_DRIVER_COMMAND_PROGRAM_PHRASE    (0x07u)
#define FLASH_DRIVER_COMMAND_ERASE_SECTOR      (0x09u)

#define FLASH_DRIVER_CLEARABLE_ERROR_MASK      \
    (FTFC_FSTAT_ACCERR_MASK  |                 \
     FTFC_FSTAT_FPVIOL_MASK  |                 \
     FTFC_FSTAT_RDCOLERR_MASK)

#define FLASH_DRIVER_COMMAND_ERROR_MASK        \
    (FTFC_FSTAT_ACCERR_MASK   |                \
     FTFC_FSTAT_FPVIOL_MASK   |                \
     FTFC_FSTAT_RDCOLERR_MASK |                \
     FTFC_FSTAT_MGSTAT0_MASK)

#define FLASH_DRIVER_COMMAND_POLL_LIMIT        (50000000u)

static volatile FlashDriverStatusType g_flashDriverStatus =
    FLASH_DRIVER_STATUS_IDLE;

static volatile FlashDriverOperationType g_flashDriverOperation =
    FLASH_DRIVER_OPERATION_NONE;

static volatile uint32_t g_flashDriverOperationAddress = 0u;

static uint8_t g_flashDriverPhraseData[FLASH_PHRASE_SIZE];

static RAM_FUNC uint32_t FlashDriver_EnterCritical(void)
{
    uint32_t primask;

    __asm volatile ("MRS %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" : : : "memory");

    return primask;
}

static RAM_FUNC void FlashDriver_ExitCritical(uint32_t primask)
{
    if ((primask & 1u) == 0u)
    {
        __asm volatile ("cpsie i" : : : "memory");
    }
}

static void FlashDriver_ResetPhraseBuffer(void)
{
    uint32_t index;

    for (index = 0u;
         index < FLASH_PHRASE_SIZE;
         index++)
    {
        g_flashDriverPhraseData[index] = 0xFFu;
    }
}

static RAM_FUNC void FlashDriver_CopyPhraseData(
    const uint8_t source[FLASH_PHRASE_SIZE])
{
    uint32_t index;

    for (index = 0u;
         index < FLASH_PHRASE_SIZE;
         index++)
    {
        g_flashDriverPhraseData[index] =
            source[index];
    }
}

static RAM_FUNC bool FlashDriver_IsHardwareReady(void)
{
    return ((FTFC->FSTAT &
             FTFC_FSTAT_CCIF_MASK) != 0u);
}

static RAM_FUNC bool FlashDriver_CanStartCommand(void)
{
    if (g_flashDriverStatus ==
        FLASH_DRIVER_STATUS_BUSY)
    {
        return false;
    }

    return FlashDriver_IsHardwareReady();
}

static RAM_FUNC FlashDriverStatusType
FlashDriver_ValidateEraseAddress(
    uint32_t sectorAddress)
{
    if (!MemoryMap_IsSectorAligned(
            sectorAddress))
    {
        return FLASH_DRIVER_STATUS_ALIGNMENT_VIOLATION;
    }

    if (!MemoryMap_IsApplicationSectorValid(
            sectorAddress))
    {
        return FLASH_DRIVER_STATUS_ADDRESS_VIOLATION;
    }

    return FLASH_DRIVER_STATUS_SUCCESS;
}

static RAM_FUNC FlashDriverStatusType
FlashDriver_ValidateProgramRequest(
    uint32_t phraseAddress,
    const uint8_t data[FLASH_PHRASE_SIZE])
{
    if (data == NULL)
    {
        return FLASH_DRIVER_STATUS_NOT_READY;
    }

    if (!MemoryMap_IsPhraseAligned(
            phraseAddress))
    {
        return FLASH_DRIVER_STATUS_ALIGNMENT_VIOLATION;
    }

    if (!MemoryMap_IsApplicationPhraseValid(
            phraseAddress))
    {
        return FLASH_DRIVER_STATUS_ADDRESS_VIOLATION;
    }

    return FLASH_DRIVER_STATUS_SUCCESS;
}

static RAM_FUNC void FlashDriver_ClearHardwareErrors(void)
{
    FTFC->FSTAT =
        FLASH_DRIVER_CLEARABLE_ERROR_MASK;
}

static RAM_FUNC void FlashDriver_LoadAddress(
    uint32_t address)
{
    FTFC_FCCOB1_REG =
        (uint8_t)((address >> 16u) & 0xFFu);

    FTFC_FCCOB2_REG =
        (uint8_t)((address >> 8u) & 0xFFu);

    FTFC_FCCOB3_REG =
        (uint8_t)(address & 0xFFu);
}

static RAM_FUNC void FlashDriver_LoadPhraseData(
    const uint8_t data[FLASH_PHRASE_SIZE])
{
    /* First 32-bit phrase word. */
    FTFC_FCCOB4_REG = data[3];
    FTFC_FCCOB5_REG = data[2];
    FTFC_FCCOB6_REG = data[1];
    FTFC_FCCOB7_REG = data[0];

    /* Second 32-bit phrase word. */
    FTFC_FCCOB8_REG = data[7];
    FTFC_FCCOB9_REG = data[6];
    FTFC_FCCOBA_REG = data[5];
    FTFC_FCCOBB_REG = data[4];
}

static RAM_FUNC FlashDriverStatusType
FlashDriver_LaunchAndWait(void)
{
    uint32_t pollCount = 0u;
    uint8_t hardwareStatus;

    COMPILER_MEMORY_BARRIER();

    FTFC->FSTAT = FTFC_FSTAT_CCIF_MASK;

    COMPILER_MEMORY_BARRIER();

    while ((FTFC->FSTAT & FTFC_FSTAT_CCIF_MASK) == 0u)
    {
        pollCount++;

        if (pollCount >= FLASH_DRIVER_COMMAND_POLL_LIMIT)
        {
            return FLASH_DRIVER_STATUS_NOT_READY;
        }
    }

    hardwareStatus = FTFC->FSTAT;

    if ((hardwareStatus &
         FLASH_DRIVER_COMMAND_ERROR_MASK) != 0u)
    {
        return FLASH_DRIVER_STATUS_FAILED;
    }

    return FLASH_DRIVER_STATUS_SUCCESS;
}

void FlashDriver_Init(void)
{
    g_flashDriverStatus =
        FLASH_DRIVER_STATUS_IDLE;

    g_flashDriverOperation =
        FLASH_DRIVER_OPERATION_NONE;

    g_flashDriverOperationAddress = 0u;

    FlashDriver_ResetPhraseBuffer();

    if (FlashDriver_IsHardwareReady())
    {
        FlashDriver_ClearHardwareErrors();
    }
}

RAM_FUNC bool FlashDriver_StartEraseSector(
    uint32_t sectorAddress)
{
    FlashDriverStatusType validationStatus;
    FlashDriverStatusType commandStatus;
    uint32_t primask;

    g_flashDriverOperation =
        FLASH_DRIVER_OPERATION_ERASE_SECTOR;

    g_flashDriverOperationAddress =
        sectorAddress;

    if (!FlashDriver_CanStartCommand())
    {
        g_flashDriverStatus =
            FLASH_DRIVER_STATUS_NOT_READY;

        return false;
    }

    validationStatus =
        FlashDriver_ValidateEraseAddress(
            sectorAddress);

    if (validationStatus !=
        FLASH_DRIVER_STATUS_SUCCESS)
    {
        g_flashDriverStatus =
            validationStatus;

        return false;
    }

    primask = FlashDriver_EnterCritical();

    FlashDriver_ClearHardwareErrors();

    FTFC_FCCOB0_REG =
        FLASH_DRIVER_COMMAND_ERASE_SECTOR;

    FlashDriver_LoadAddress(sectorAddress);

    commandStatus = FlashDriver_LaunchAndWait();

    FlashDriver_ExitCritical(primask);
    /* -------------------------------------------------------------------- */

    g_flashDriverStatus = commandStatus;

    return (commandStatus == FLASH_DRIVER_STATUS_SUCCESS);
}

RAM_FUNC bool FlashDriver_StartProgramPhrase(
    uint32_t phraseAddress,
    const uint8_t data[FLASH_PHRASE_SIZE])
{
    FlashDriverStatusType validationStatus;
    FlashDriverStatusType commandStatus;
    uint32_t primask;

    g_flashDriverOperation =
        FLASH_DRIVER_OPERATION_PROGRAM_PHRASE;

    g_flashDriverOperationAddress =
        phraseAddress;

    if (!FlashDriver_CanStartCommand())
    {
        g_flashDriverStatus =
            FLASH_DRIVER_STATUS_NOT_READY;

        return false;
    }

    validationStatus =
        FlashDriver_ValidateProgramRequest(
            phraseAddress,
            data);

    if (validationStatus !=
        FLASH_DRIVER_STATUS_SUCCESS)
    {
        g_flashDriverStatus =
            validationStatus;

        return false;
    }

    FlashDriver_CopyPhraseData(data);

    primask = FlashDriver_EnterCritical();

    FlashDriver_ClearHardwareErrors();

    FTFC_FCCOB0_REG =
        FLASH_DRIVER_COMMAND_PROGRAM_PHRASE;

    FlashDriver_LoadAddress(phraseAddress);

    FlashDriver_LoadPhraseData(g_flashDriverPhraseData);

    commandStatus = FlashDriver_LaunchAndWait();

    FlashDriver_ExitCritical(primask);
    /* -------------------------------------------------------------------- */

    g_flashDriverStatus = commandStatus;

    return (commandStatus == FLASH_DRIVER_STATUS_SUCCESS);
}

RAM_FUNC FlashDriverStatusType
FlashDriver_GetStatus(void)
{
    return g_flashDriverStatus;
}

FlashDriverOperationType
FlashDriver_GetOperation(void)
{
    return g_flashDriverOperation;
}

uint32_t FlashDriver_GetOperationAddress(void)
{
    return g_flashDriverOperationAddress;
}

RAM_FUNC bool FlashDriver_IsBusy(void)
{
    return false;
}

void FlashDriver_AcknowledgeResult(void)
{
    g_flashDriverStatus =
        FLASH_DRIVER_STATUS_IDLE;

    g_flashDriverOperation =
        FLASH_DRIVER_OPERATION_NONE;

    g_flashDriverOperationAddress = 0u;

    FlashDriver_ResetPhraseBuffer();
}
