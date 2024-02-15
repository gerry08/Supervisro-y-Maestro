/*
 * DMA2.h
 *
 *  Created on: 11 may 2023
 *      Author: maxim
 */

#ifndef DMA2_H_
#define DMA2_H_

#include <registers.h>
#include <stdint.h>

#define CRC_DR_ADDRESS 0x40023000
#define FLASH_START_ADDRESS 0x08000000
#define NUM_WORDS 2

void DMA_Init();
void DMA_Init(uint8_t source[],uint32_t tamanio);

#endif /* DMA2_H_ */
