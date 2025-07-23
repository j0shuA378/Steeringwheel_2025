#include "bsp_EC200.h"

uint8_t ATCommand_upload(char *uploadstr,char *targetstr){
	uint8_t i = 0;

	osDelay(RESPONSEWAITING);
	Tx_Flag = 2;
	printf("%s\r\n",uploadstr);
	osDelay(RESPONSEWAITING);
	while(strstr((const char *)Rx_string,(const char *)targetstr ) == NULL){
		printf("%s\r\n",uploadstr);
		osDelay(RESPONSEWAITING);
		i++;
		if(i>=5){
			Tx_Flag = 1;
			printf("/n%s Fail\n",uploadstr);
			return 0;
		}
	}
	Tx_Flag = 1;
	printf("\n%s OK\n",uploadstr);
	memset(Rx_string,0x00,sizeof(Rx_string)); //清空数组
	Rx_Count = 0;
	return 1;
}

uint8_t EC200_Init(void)
{
	uint16_t err_Count = 0;
	HAL_UART_Receive_IT(&huart3,(uint8_t *)&Rx_buff,1);
	Tx_Flag = 1;
	IRQ_JudgEnable = 1;
	printf("\nec200 开机\n");
	printf("...等待就绪...\n");
	while(!EC200_RdyFlag){
		osDelay(1000);
		printf(".");
		err_Count++;
		if(err_Count>20){
			printf("重启失败");			//失败后，开机只能靠重新上电

			return 0;
		}
	}
	printf("\n重启成功\n");

	//基础检查
	IRQ_JudgEnable = 0; 	//不在中断判断
	Tx_Flag = 2;
	printf("ATE0\r\n"); //关闭回显

	//CIMI,检查卡号
	//CSQ,检查信号
	//CGATT,检查网络附着情况 										/*重要*/
	return ATCommand_upload("AT+CIMI","460") \
		&  ATCommand_upload("AT+CSQ","CSQ") \
		&  ATCommand_upload("AT+CGATT?","+CGATT: 1");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART3){
		if(Rx_Count >= 255){
			Rx_Count = 0;
			memset(Rx_string,0x00,sizeof(Rx_string));
			HAL_UART_Transmit(&huart1,(uint8_t *)"overflow",10,0xffff);
		}
		else{
			Rx_string[Rx_Count++] = Rx_buff;
			if((Rx_string[Rx_Count-2] == 0x0D) && (Rx_string[Rx_Count-1] == 0x0A) && (Rx_Count != 2) && (IRQ_JudgEnable)){        //判断结束符

				HAL_UART_Transmit(&huart1, (uint8_t *)&Rx_string, Rx_Count-1,0xFFFF);
				while(HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX);

				//过滤无效字符串
				if(strcmp((const char*)Rx_string,(const char*)" ") == 0 || strlen(Rx_string) == 0){
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);
				}

				//4G模块就绪
				if(strstr((const char*)Rx_string,(const char*)"RDY")){
					EC200_RdyFlag = 1;
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);
				}

				//连接IP
				if(strstr((const char *)Rx_string,(const char *)"OPEN")){
					QMTOPEN_Flag = 1;
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);
				}

				//连接客户端
				if(strstr((const char *)Rx_string,(const char *)"CONN")){
					QMTCONN_Flag = 1;
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);
				}

				//MQTT发布状态判断
				if(strstr((const char *)Rx_string,(const char *)"PUBEX")){
					PUBOK_Flag = 1;
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);

				}

				//失效判断
				if(strstr((const char *)Rx_string,(const char *)"ERROR")){
					EC200_RdyFlag = 0;
					QMTOPEN_Flag = 0;
					QMTCONN_Flag = 0;
					PUBOK_Flag = 0;
					memset(Rx_string,0x00,sizeof(Rx_string));
					HAL_UART_Receive_IT(&huart3, (uint8_t *)&Rx_string, 1);
				}

				Rx_Count = 0;
			}
		}
		HAL_UART_Receive_IT(&huart3,(uint8_t *)&Rx_buff,1);
	}

}
