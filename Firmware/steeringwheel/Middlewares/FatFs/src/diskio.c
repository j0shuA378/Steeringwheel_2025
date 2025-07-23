/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs                             (C)ChaN, 2019       */
/*-----------------------------------------------------------------------*/
/* This is a glue layer between the FatFs library and the STM32 HAL      */
/* library for SD card access. It's tailored for STM32 microcontrollers  */
/* with a cache (like the H7 series) and uses blocking HAL functions.    */
/*-----------------------------------------------------------------------*/

#include "ff.h"         /* Obtains integer types for FatFs */
#include "diskio.h"     /* Declarations of disk I/O functions */
#include "sdmmc.h"      /* For HAL_SD_... functions and hsd1 handle */
#include "main.h"       /* For basic types and HAL functions */

/*
 * This implementation assumes the use of an STM32 microcontroller with an SDMMC peripheral
 * managed by the STM32Cube HAL library. The SDMMC handle is expected to be `hsd1`.
 * If your handle has a different name (e.g., `hsd2`), you must update it below.
 */

/* Definitions of physical drive numbers */
#define DEV_MMC     0   /* Map MMC/SD card to physical drive 0 */
#define DEV_RAM     1   /* Example: Map Ramdisk to physical drive 1 (Not implemented) */
#define DEV_USB     2   /* Example: Map USB MSD to physical drive 2 (Not implemented) */

/* SDMMC Handle (must be defined in your project, typically in main.c) */
extern SD_HandleTypeDef hsd1;

/* Timeout for SD card operations in milliseconds */
#define SD_TIMEOUT      1000U

/* Standard SD Card Block Size */
#define SD_BLOCK_SIZE   512U

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE pdrv       /* Physical drive number to identify the drive */
)
{
    DSTATUS stat = STA_NOINIT; // Assume not initialized by default
    HAL_SD_CardStateTypeDef card_state;

    switch (pdrv) {
    case DEV_MMC:
        // Use the HAL function to get the current state of the SD card.
        card_state = HAL_SD_GetCardState(&hsd1);

        // The card is considered ready if it's in the transfer state.
        if (card_state == HAL_SD_CARD_TRANSFER) {
            stat = 0; // Status OK, drive is ready
        }
        // Note: HAL_SD_CARD_READY is a state during initialization, not normal operation.
        // The stable "ready" state for read/write is HAL_SD_CARD_TRANSFER.

        // Optional: Check for a physical card detect pin if your hardware has one.
        // This provides a more robust way to detect if a card is present.
        // Example:
        // if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_Port, SD_DETECT_Pin) == GPIO_PIN_SET) {
        //     stat |= STA_NODISK; // Set STA_NODISK if card is not present
        // }

        return stat;

    // The following cases are for other drive types and are not implemented.
    case DEV_RAM:
        // return RAM_disk_status(); // Placeholder
        return STA_NOINIT;

    case DEV_USB:
        // return USB_disk_status(); // Placeholder
        return STA_NOINIT;
    }
    return STA_NOINIT; // Default for any other drive number
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive number to identify the drive */
)
{
    DSTATUS stat = STA_NOINIT;
    HAL_StatusTypeDef hal_status;

    switch (pdrv) {
    case DEV_MMC:
        /*
         * The HAL_SD_Init function performs the full SD card initialization sequence,
         * including sending CMD0, CMD8, CMD55/ACMD41, etc.
         */
        hal_status = HAL_SD_Init(&hsd1);
        if (hal_status != HAL_OK) {
            return STA_NOINIT; // Initialization failed
        }

        /*
         * After successful initialization, it's highly recommended to switch to
         * a wider bus (4-bit) for significantly better performance.
         */
        hal_status = HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B);
        if (hal_status != HAL_OK) {
            return STA_NOINIT; // Failed to configure bus width
        }

        // If all initialization steps are successful, the disk is ready.
        stat = 0;
        return stat;

    // The following cases are for other drive types and are not implemented.
    case DEV_RAM:
        // return RAM_disk_initialize(); // Placeholder
        return STA_NOINIT;

    case DEV_USB:
        // return USB_disk_initialize(); // Placeholder
        return STA_NOINIT;
    }
    return STA_NOINIT; // Default for any other drive number
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    HAL_StatusTypeDef hal_status;

    if (pdrv != DEV_MMC || count == 0) {
        return RES_PARERR; // Parameter error
    }

    // On MCUs with a D-Cache (like STM32H7), it's crucial to invalidate the cache
    // for the buffer area before a DMA-based read operation. This ensures the CPU
    // will fetch the new data from RAM instead of using stale data from the cache.
    // The buffer address must be 32-byte aligned for this to work correctly.
    SCB_InvalidateDCache_by_Addr((uint32_t *)buff, count * SD_BLOCK_SIZE);

    // Perform the block read operation using the blocking HAL function.
    hal_status = HAL_SD_ReadBlocks(&hsd1, buff, sector, count, SD_TIMEOUT);

    if (hal_status == HAL_OK) {
        // The HAL_SD_ReadBlocks function is blocking and should wait for completion.
        // However, as a safeguard, we can check if the card has returned to the
        // transfer state. A robust implementation would have a timeout here.
        // For simplicity with blocking calls, we trust the HAL's internal timeout.
        return RES_OK;
    }

    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    HAL_StatusTypeDef hal_status;
    uint32_t tickstart;

    if (pdrv != DEV_MMC || count == 0) {
        return RES_PARERR; // Parameter error
    }

    // On MCUs with a D-Cache, the cache must be cleaned before a DMA-based write.
    // This flushes any pending changes from the cache to main memory, ensuring
    // the DMA controller reads the most up-to-date data from the buffer.
    // The buffer address must be 32-byte aligned.
    SCB_CleanDCache_by_Addr((uint32_t *)buff, count * SD_BLOCK_SIZE);

    // Perform the block write operation using the blocking HAL function.
    hal_status = HAL_SD_WriteBlocks(&hsd1, (uint8_t*)buff, sector, count, SD_TIMEOUT);

    if (hal_status == HAL_OK) {
        // After a write operation, the card might enter a busy state.
        // It's essential to wait until the card is ready for the next command.
        // We poll the card state until it returns to the transfer state, with a timeout.
        tickstart = HAL_GetTick();
        while(HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER) {
            if ((HAL_GetTick() - tickstart) >= SD_TIMEOUT) {
                return RES_ERROR; // Timed out waiting for card to become ready
            }
        }
        return RES_OK;
    }

    return RES_ERROR;
}

#endif // FF_FS_READONLY

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res = RES_ERROR;
    HAL_SD_CardInfoTypeDef card_info;

    if (pdrv != DEV_MMC) {
        return RES_PARERR;
    }

    switch (cmd) {
    /* Make sure that no pending write process */
    case CTRL_SYNC:
        // For this blocking implementation, HAL_SD_WriteBlocks ensures data is
        // written before returning. The wait loop in disk_write confirms the
        // card is ready, so this command can be considered successful.
        res = RES_OK;
        break;

    /* Get number of sectors on the disk */
    case GET_SECTOR_COUNT:
        if (HAL_SD_GetCardInfo(&hsd1, &card_info) == HAL_OK) {
            *(LBA_t*)buff = card_info.LogBlockNbr;
            res = RES_OK;
        }
        break;

    /* Get R/W sector size */
    case GET_SECTOR_SIZE:
        // SD cards always use 512-byte sectors
        *(WORD*)buff = SD_BLOCK_SIZE;
        res = RES_OK;
        break;

    /* Get erase block size in unit of sector */
    case GET_BLOCK_SIZE:
        if (HAL_SD_GetCardInfo(&hsd1, &card_info) == HAL_OK) {
            // The erase block size is related to the CSD register's AU_SIZE.
            // For simplicity and compatibility, returning a value of 1 sector is a safe default.
            // A more advanced implementation could parse the CSD register.
            *(DWORD*)buff = 1;
            res = RES_OK;
        }
        break;

    default:
        res = RES_PARERR;
        break;
    }

    return res;
}

/*-----------------------------------------------------------------------*/
/* Get Current Time                                                      */
/*-----------------------------------------------------------------------*/
#if FF_FS_NORTC == 0 && FF_FS_TIMESTAMP == 1
DWORD get_fattime (void)
{
    /*
     * This function should return the current time.
     * If you have an RTC (Real-Time Clock) in your system, you would:
     * 1. Read the current time and date from the RTC.
     * 2. Pack the data into a DWORD with the following format:
     * bit31:25 - Year from 1980 (0..127)
     * bit24:21 - Month (1..12)
     * bit20:16 - Day in month (1..31)
     * bit15:11 - Hour (0..23)
     * bit10:5  - Minute (0..59)
     * bit4:0   - Second / 2 (0..29)
     *
     * Example (requires RTC_HandleTypeDef hrtc to be defined):
     *
     * RTC_TimeTypeDef sTime;
     * RTC_DateTypeDef sDate;
     *
     * HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
     * HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
     *
     * return ((DWORD)(sDate.Year + 20) << 25) | // Year is offset from 2000 for HAL, FatFs from 1980
     * ((DWORD)sDate.Month << 21) |
     * ((DWORD)sDate.Day << 16)   |
     * ((DWORD)sTime.Hours << 11) |
     * ((DWORD)sTime.Minutes << 5)|
     * ((DWORD)sTime.Seconds / 2);
     *
     * If you don't have an RTC, you can return a fixed timestamp or 0.
     */
    return 0;
}
#endif
