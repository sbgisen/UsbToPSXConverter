/*
 * SD.h
 *
 *  Created on: 12.02.2018
 *      Author: NELO
 */

#ifndef MYLIBRARY_SDCARD_H_
#define MYLIBRARY_SDCARD_H_


#include <integer.h>

/* HARDWARE DEFINITION */
#define SDCARD_CS_Pin 		GPIO_PIN_3		////Chip Select Pin (Low-Active)
#define SDCARD_CS_GPIOx		GPIOC			////Select Pin GPIO Group

void SDCard_HWInit(void);
void SDCard_ChipSelect(void);
void SDCard_ChipDeselect(void);


#endif /* MYLIBRARY_SDCARD_H_ */


