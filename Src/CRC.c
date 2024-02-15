#include <CRC.h>

void CRC_Enable(){
	int i = 0;
	RCC_AHB1ENR |= BIT_12; //Activa reloj para CRC
	for (i = 0; i < 1000; i++){}; //Espera a que el reloj se estabilice
	CRC_Restart();
}

void CRC_Restart(void){
	CRC_CR |= BIT_0;
}

void CRC_Acummulate(uint32_t dato){
	CRC_DR = dato;
}

uint32_t CRC_Read(){
	uint32_t dato;
	dato = CRC_DR;
	return dato;
}
