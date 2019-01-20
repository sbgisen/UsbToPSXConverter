/*
 * SD.c
 *
 *  Created on: 12.02.2018
 *      Author: NELO
 */

#include "SDCard.h"

#include "stm32F4xx.h"
#include "stm32F4xx_hal.h"
#include "MySPI_HAL.h"

//SD Card Hardware Init
void SDCard_HWInit(void){

	GPIO_InitTypeDef GPIO_InitStruct;
	// __HAL_RCC_GPIOC_CLK_ENABLE(); ////Enable RCC
	__GPIOA_CLK_ENABLE();
	__GPIOB_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	GPIO_InitStruct.Pin = SDCARD_CS_Pin;				////Configure the Pins
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(SDCARD_CS_GPIOx, &GPIO_InitStruct);	////Load the Initialization Structure
	MySPI3_Init_Slow();
}

////Selects the SD Card (Chip Select Pin)
void SDCard_ChipSelect(void){
	HAL_GPIO_WritePin(SDCARD_CS_GPIOx, SDCARD_CS_Pin, GPIO_PIN_RESET);	//Chip Select (Low Active)
}

////Deselects the SD Card (Chip Select Pin)
void SDCard_ChipDeselect(void){
	HAL_GPIO_WritePin(SDCARD_CS_GPIOx, SDCARD_CS_Pin, GPIO_PIN_SET);	//Chip Select (Low Active)
}

