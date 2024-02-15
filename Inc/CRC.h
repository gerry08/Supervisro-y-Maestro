/*
 * CRC.h
 *
 *  Created on: 11 may 2023
 *      Author: maxim
 */

#ifndef CRC_H_
#define CRC_H_

#include <registers.h>
#include <stdint.h>

void CRC_Restart(void);
void CRC_Enable(void);
void CRC_Acummulate(uint32_t dato);
uint32_t  CRC_Read(void);

#endif /* CRC_H_ */
