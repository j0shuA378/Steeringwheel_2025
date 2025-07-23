#ifndef _APP_EC200_H_
#define _APP_EC200_H_

#include "main.h"
#include "string.h"
#include "bsp_EC200.h"
#include "cmsis_os.h"

uint8_t EC200_MQTTInit(void);
void carDataUpdate(void);
void MQTT_Pubdata(char *MQTTdata);
void jsonPack(void);

#endif
