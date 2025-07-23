//
// Created by joshu on 25-7-22.
//

#ifndef APP_SDCARD_H
#define APP_SDCARD_H

#include "main.h"
#include "stm32h7xx_hal.h"
#include "ff.h"
#include "diskio.h"
#include "ffconf.h"

void sdcard_init();
void sdcard_write();

#endif //APP_SDCARD_H
