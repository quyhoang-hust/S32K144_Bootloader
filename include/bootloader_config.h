#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H


#define BOOTLOADER_UART_BAUDRATE               (115200u)

#define BOOTLOADER_LPUART_CLOCK_HZ             (48000000u)

#define BOOTLOADER_LPUART_PCC_PCS              (3u)

#define BOOTLOADER_UART_TX_POLL_LIMIT          (1000000u)

#define RX_RING_BUFFER_SIZE                    (512u)

#define SREC_MAX_BYTE_COUNT                    (255u)

#define SREC_MAX_LINE_LENGTH                   (520u)

#define SREC_MAX_DATA_SIZE                     (252u)

#define SREC_TERMINATION_PAIR_CHECK_ENABLED    (0u)

#define SREC_RX_MAX_BYTES_PER_CYCLE            (16u)

#define FLASH_JOB_QUEUE_SIZE                   (8u)

#define FLASH_JOB_MAX_DATA_SIZE                (SREC_MAX_DATA_SIZE)

#define FLASH_ENGINE_MAX_NEW_JOBS_PER_CYCLE    (1u)

#define FLASH_ENGINE_MAX_OPERATIONS_PER_CYCLE  (1u)

#define DOWNLOAD_REPORT_STEP_BYTES             (256u)

#define ERASE_REPORT_ON_PERCENT_CHANGE         (1u)

#define BOOT_STATE_REPORT_ON_CHANGE            (1u)

#define FATAL_ERROR_REPORT_ENABLED             (1u)


#if (BOOTLOADER_UART_BAUDRATE == 0u)
#error "BOOTLOADER_UART_BAUDRATE must be greater than zero"
#endif

#if (BOOTLOADER_LPUART_CLOCK_HZ == 0u)
#error "BOOTLOADER_LPUART_CLOCK_HZ must be greater than zero"
#endif

#if (BOOTLOADER_UART_TX_POLL_LIMIT == 0u)
#error "BOOTLOADER_UART_TX_POLL_LIMIT must be greater than zero"
#endif


#if (RX_RING_BUFFER_SIZE <= 1u)
#error "RX_RING_BUFFER_SIZE must be greater than one"
#endif

#if ((RX_RING_BUFFER_SIZE & (RX_RING_BUFFER_SIZE - 1u)) != 0u)
#error "RX_RING_BUFFER_SIZE must be a power of two"
#endif


#if (SREC_MAX_BYTE_COUNT != 255u)
#error "SREC_MAX_BYTE_COUNT must match the one-byte SREC count field"
#endif

#if (SREC_MAX_LINE_LENGTH == 0u)
#error "SREC_MAX_LINE_LENGTH must be greater than zero"
#endif

#if (SREC_MAX_DATA_SIZE == 0u)
#error "SREC_MAX_DATA_SIZE must be greater than zero"
#endif

#if (SREC_MAX_DATA_SIZE < \
     (SREC_MAX_BYTE_COUNT - 2u - 1u))
#error "SREC_MAX_DATA_SIZE is too small for a maximum-size S1 record"
#endif

#if (SREC_MAX_DATA_SIZE > \
     (SREC_MAX_BYTE_COUNT - 2u - 1u))
#error "SREC_MAX_DATA_SIZE exceeds the maximum possible SREC payload"
#endif

#if (SREC_MAX_LINE_LENGTH < \
     (4u + (2u * SREC_MAX_BYTE_COUNT)))
#error "SREC_MAX_LINE_LENGTH must hold the longest encoded S-record"
#endif

#if (SREC_MAX_LINE_LENGTH > 65535u)
#error "SREC_MAX_LINE_LENGTH is unnecessarily large"
#endif

#if ((SREC_TERMINATION_PAIR_CHECK_ENABLED != 0u) && \
     (SREC_TERMINATION_PAIR_CHECK_ENABLED != 1u))
#error "SREC_TERMINATION_PAIR_CHECK_ENABLED must be zero or one"
#endif

#if (SREC_RX_MAX_BYTES_PER_CYCLE == 0u)
#error "SREC_RX_MAX_BYTES_PER_CYCLE must be greater than zero"
#endif

#if (SREC_RX_MAX_BYTES_PER_CYCLE >= RX_RING_BUFFER_SIZE)
#error "SREC_RX_MAX_BYTES_PER_CYCLE must be smaller than RX_RING_BUFFER_SIZE"
#endif


#if (FLASH_JOB_QUEUE_SIZE < 2u)
#error "FLASH_JOB_QUEUE_SIZE must be at least two"
#endif

#if (FLASH_JOB_MAX_DATA_SIZE < SREC_MAX_DATA_SIZE)
#error "FLASH_JOB_MAX_DATA_SIZE must store a complete SREC payload"
#endif

#if (FLASH_JOB_MAX_DATA_SIZE > UINT16_MAX)
#error "FLASH_JOB_MAX_DATA_SIZE does not fit FlashProgramDataType.length"
#endif

#if (FLASH_ENGINE_MAX_NEW_JOBS_PER_CYCLE != 1u)
#error "FlashEngine may acquire exactly one new job per scheduler cycle"
#endif

#if (FLASH_ENGINE_MAX_OPERATIONS_PER_CYCLE != 1u)
#error "FlashEngine may perform exactly one flash-operation step per cycle"
#endif

#if (DOWNLOAD_REPORT_STEP_BYTES == 0u)
#error "DOWNLOAD_REPORT_STEP_BYTES must be greater than zero"
#endif

#if ((ERASE_REPORT_ON_PERCENT_CHANGE != 0u) && \
     (ERASE_REPORT_ON_PERCENT_CHANGE != 1u))
#error "ERASE_REPORT_ON_PERCENT_CHANGE must be zero or one"
#endif

#if ((BOOT_STATE_REPORT_ON_CHANGE != 0u) && \
     (BOOT_STATE_REPORT_ON_CHANGE != 1u))
#error "BOOT_STATE_REPORT_ON_CHANGE must be zero or one"
#endif

#if ((FATAL_ERROR_REPORT_ENABLED != 0u) && \
     (FATAL_ERROR_REPORT_ENABLED != 1u))
#error "FATAL_ERROR_REPORT_ENABLED must be zero or one"
#endif

#endif
