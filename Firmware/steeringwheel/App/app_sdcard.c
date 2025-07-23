//
// Created by joshu on 25-7-22.
//

#include "app_sdcard.h"

extern DSTATUS disk_initialize(BYTE pdrv); //Diskio_Init

void sdcard_init() {
    // FatFs variables
    FATFS fs;         // File system object for the logical drive
    FRESULT res;      // FatFs function result code

    //  Mount the SD card. The "0:" path corresponds to DEV_MMC in diskio.c
    //  The f_mount function will call disk_initialize() internally.
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK)
    {
        // An error occurred, check the 'res' variable for the reason.
        // Common errors:
        // FR_NOT_READY: The disk_initialize function failed.
        // FR_NO_FILESYSTEM: The SD card is not formatted or the format is not supported.
        Error_Handler();
    }

    __NOP();
}

/******************This section will tell you how to use this function*****************/

//path is the name to be written
//data is what you want to write
//length is tells the function how many bits of data to write

/*************you can use this function if you want to write sth to sdcard*************/
void sdcard_write(const char* path, const uint8_t* data, UINT length) {
    FIL fil;          // File object
    FRESULT res;      // FatFs function result code
    UINT bytes_written;

    //  Open or create a file named by 'path' for writing.
    //  FA_CREATE_ALWAYS: Creates a new file. If the file already exists, it will be overwritten.
    res = f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        Error_Handler();
    }

    //  Write data to the file.
    res = f_write(&fil, data, length, &bytes_written);
    if (res != FR_OK || bytes_written != length)
    {
        // An error occurred during write.
        f_close(&fil); // Close the file before handling the error.
        Error_Handler();
    }

    // Close the file.
    res = f_close(&fil);
    if (res != FR_OK)
    {
        Error_Handler();
    }
    // Unmount the drive
    f_mount(NULL, "0:", 1);

    __NOP();
}

//there is the sdcard function original

// void sdcard_init() {
//     // FatFs variables
//     FATFS fs;         // File system object for the logical drive
//     FIL fil;          // File object
//     FRESULT res;      // FatFs function result code
//
//     // Define a buffer for writing to the file
//     uint8_t write_buffer[] = "Ciallo";
//     UINT bytes_written;
//
//     //  Mount the SD card. The "0:" path corresponds to DEV_MMC in diskio.c
//     //  The f_mount function will call disk_initialize() internally.
//     res = f_mount(&fs, "0:", 1);
//     if (res != FR_OK)
//     {
//         // An error occurred, check the 'res' variable for the reason.
//         // Common errors:
//         // FR_NOT_READY: The disk_initialize function failed.
//         // FR_NO_FILESYSTEM: The SD card is not formatted or the format is not supported.
//         Error_Handler();
//     }
//
//     //  Open or create a file named "test.txt" for writing.
//     //  FA_CREATE_ALWAYS: Creates a new file. If the file already exists, it will be overwritten.
//     res = f_open(&fil, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
//     if (res != FR_OK)
//     {
//         Error_Handler();
//     }
//
//     //  Write data to the file.
//     res = f_write(&fil, write_buffer, sizeof(write_buffer) - 1, &bytes_written);
//     if (res != FR_OK || bytes_written != (sizeof(write_buffer) - 1))
//     {
//         // An error occurred during write.
//         f_close(&fil); // Close the file before handling the error.
//         Error_Handler();
//     }
//
//     // Close the file.
//     res = f_close(&fil);
//     if (res != FR_OK)
//     {
//         Error_Handler();
//     }
//
//     // Unmount the drive
//     f_mount(NULL, "0:", 1);
//
//     __NOP();
// }