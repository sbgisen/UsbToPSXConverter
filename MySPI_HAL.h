/*
 * MySPI.h
 *
 *  Created on: 16.06.2017
 *      Author: NELO
 */

#ifndef MYLIBRARY_MYSPI_HAL_H_
#define MYLIBRARY_MYSPI_HAL_H_

#include <stdint.h>


void MySPI3_DeInit(void);
void MySPI3_Init_Slow(void);
void MySPI3_Init_Fast(void);
uint8_t MySPI3_ReadByte(void);
uint8_t MySPI3_ReadBytes(uint8_t *data, uint16_t numBytes);
uint8_t MySPI3_SendByte(uint8_t data);
uint8_t MySPI3_SendBytes(uint8_t *data, uint16_t numBytes);



#endif /* MYLIBRARY_MYSPI_HAL_H_ */
