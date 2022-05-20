/*
 * MySPI.c
 *
 *  Created on: 16.06.2017
 *      Author: NELO
 */

#include "MySPI_HAL.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>


SPI_HandleTypeDef hspi3;

/* SPI3 init slow function */
void MySPI3_Init_Slow(void)
{
	////Config and Enable Pins
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	////Config and Enable SPI1 Peripheral
	__HAL_RCC_SPI3_CLK_ENABLE();
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi3.Init.CRCPolynomial = 10;
	HAL_SPI_Init(&hspi3);
}

/* SPI3 init fast function */
void MySPI3_Init_Fast(void)
{
	////Config and Enable Pins
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	////Config and Enable SPI1 Peripheral
	__HAL_RCC_SPI3_CLK_ENABLE();
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi3.Init.CRCPolynomial = 10;
	HAL_SPI_Init(&hspi3);
}

/* SPI3 Deinit function */
void MySPI3_DeInit(void){
	HAL_SPI_DeInit(&hspi3);
}

/* SPI3 read one byte*/
uint8_t MySPI3_ReadByte(void){
	uint8_t ReceivedByte = 0, Dummy = 0xFF;
	while ((HAL_SPI_GetState(&hspi3) != HAL_SPI_STATE_READY));
	HAL_SPI_TransmitReceive(&hspi3, &Dummy, &ReceivedByte, 1, 5000);
	return ReceivedByte;
}

/* SPI3 read x bytes*/
uint8_t MySPI3_ReadBytes(uint8_t *data, uint16_t numBytes) {
	uint16_t i = 0;
	for(i=0;i<numBytes;i++){
		*(data+i) = MySPI3_ReadByte();
	}
	return 0;
}

/* SPI3 send one byte*/
uint8_t MySPI3_SendByte(uint8_t data) {
	uint8_t ReceivedByte = 0;;
	while (HAL_SPI_GetState(&hspi3) != HAL_SPI_STATE_READY);
	HAL_SPI_TransmitReceive(&hspi3, &data, &ReceivedByte, 1, 5000);
	return ReceivedByte;
}

/* SPI3 send x bytes*/
uint8_t MySPI3_SendBytes(uint8_t *data, uint16_t numBytes) {
	HAL_SPI_Transmit(&hspi3, data, numBytes, HAL_MAX_DELAY);
	return 0;
}
