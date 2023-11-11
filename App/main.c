/*
 * main.c
 *
 *  Created on: Oct 17, 2023
 *      Author: Hazim Emad
 */
#include "../Library/STD_types.h"
#include "../Library/Bit_Math.h"
#include "../MCAL/DIO/Header/DIO_Interface.h"
#include "../HAL/LCD/Header/LCD_Interface.h"
#include "../MCAL/GIE/Header/GIE_Interface.h"
#include "../MCAL/USART/Header/USART_Interface.h"
#include "../MCAL/I2C/Header/I2C_Interface.h"
#include "util/delay.h"
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_SIZE	20
#define SLAVE_ADDRESS				0xD0 //1010 XXXX
#define TIME_REGISTER_ADDRESS		0x00 //XXXX Address

void RXC();
u8* IntToString(s32 anInteger,u8 *str);
u8 ConvertFromRTC(u8 num);
void RTC_Read_Clock();
u8 Random(u8 lower, u8 upper);;

volatile u8 RX_Index = 0;
volatile u8 doneFlag = 0;
u8 securityAccess = 0;
u8 data[MAX_CMD_SIZE];
u8 key[] = "11111111";
u8 successfulAccess[] = "6701";
u8 cipher[9];
u8 tempStr[20];
u8 errorFlag = 0;
u8 second =0;
u8 minute = 0;
u8 hour = 0;

int main(){
	DIO_voidInit();
	DIO_voidSetPinValue(DIO_PortC, DIO_PIN7, Low);
	LCD_voidInit();
	GIE_voidEnable();
	I2C_Master_voidInit();
	_delay_ms(200);
	RTC_Read_Clock();
	second = ConvertFromRTC(second);
	minute = ConvertFromRTC(minute);
	hour = ConvertFromRTC(hour);
	srand(second^minute^hour);
	u32 randNum = Random(1,250) * Random(1,250) * Random(1,250) * Random(1,250) + 0x12345678;
//	LCD_voidWriteIntData(randNum);
	//	LCD_voidSendCommand(LCD_DisplayOFF);
	USART_voidRXCSetCallBack(RXC);
	USART_voidInit();
	USART_voidEnableRXCInterrupt();
	while(1){
		if(doneFlag==1){
			doneFlag = 0;
			if(memcmp(data, "2701", 4) == 0){
				strcat(successfulAccess, IntToString(randNum, tempStr));
				LCD_voidWriteString(successfulAccess);
				for(u8 i = 4; i < 12; i++){
					cipher[i-4]= ((successfulAccess[i] - '0') ^ (key[i-4] - '0')) + '0';
				}
				cipher[8] = '\0';
				LCD_voidGoToPosition(1,0);
				LCD_voidWriteString(cipher);
			}
			else if(memcmp(data, "3101AA00", 8) == 0){
				if(securityAccess == 1){
					LCD_voidWriteString((u8*)"7101AA00");
					DIO_voidSetPinValue(DIO_PortC, DIO_PIN7, High);
				}
				else{
					LCD_voidWriteString((u8*)"7F3135");
				}
			}
			else if(memcmp(data, "2702", 4) == 0){
				for(u8 i = 4; i < 12; i++){
					if(cipher[i-4] != data[i]){
						errorFlag = 1;
						break;
					}
				}
				if(errorFlag == 0){
					LCD_voidWriteString((u8*)"6702");
					securityAccess = 1;
				}
				else{
					LCD_voidWriteString((u8*)"7F2735");
				}

			}
			//			USART_voidEnableUDREInterrupt();
			RX_Index = 0;
			USART_voidEnableRXCInterrupt();
		}

	}
	return 0;
}


void RXC(){
	u8 temp = USART_u8ReceiveInterruptByte();
	if(temp == '\r'){
		data[RX_Index] = '\0';
		doneFlag = 1;
		USART_voidDisableRXCInterrupt();
		LCD_voidSendCommand(LCD_ClearDisplay);
	}
	else if(temp == 0x08){
		data[--RX_Index] = '\0';
	}
	else{
		data[RX_Index] = temp;
		RX_Index++;
		//		LCD_voidWriteChar(data[RX_Index]);
	}

}


u8 Random(u8 lower, u8 upper){
	u8 num = (rand() %  (upper - lower + 1)) + lower;
	return num;
}

u8* IntToString(s32 anInteger,u8 *str){
	u32 flag = 0;
	u32 i = INT_TO_STRING_ARR_SIZE - 1;
	str[i--] = '\0';
	if (anInteger < 0) {
		flag = 1;
		anInteger = -anInteger;
	}
	while (anInteger != 0) {
		str[i--] = (anInteger % 10) + '0';
		anInteger /= 10;
	}
	if (flag){
		str[i--] = '-';
	}
	i++;
	return str + i;

}

void RTC_Read_Clock(){
	I2C_Master_enuSendStartCond();
	I2C_Master_enuSendSlaveAddressWithWrite(SLAVE_ADDRESS);
	I2C_Master_enuSendu8Data(TIME_REGISTER_ADDRESS);
	I2C_Master_enuSendRepeatedStartCond();
	I2C_Master_enuSendSlaveAddressWithRead(SLAVE_ADDRESS);
	I2C_Master_enuReadu8Data(&second);
	I2C_Master_enuReadu8Data(&minute);
	I2C_Master_enuReadu8Data(&hour);
	I2C_Master_voidFlush();
	I2C_Master_enuSendStopCond();
}

u8 ConvertFromRTC(u8 num){
	return ((num>>4) * 10 + (num&0x0F));
}

