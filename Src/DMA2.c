#include <DMA2.h>


void DMA_Enable(void){
	DMA2_S0CR |= BIT_0;
	while((DMA2_S0CR & BIT_0)== 0x0){} //Espera a que el bit se ponga en 1
}

void DMA_Init(uint8_t source[],uint32_t tamanio){
    RCC_AHB1ENR |= BIT_22; //activa reloj para DMA2
    for(int i = 0; i<100;i++){}

    DMA2_S0CR &= ~BIT_0; //Deshabilita DMA
    while((DMA2_S0CR & BIT_0)== 0x1){} //Espera a que el bit se ponga en 0

	DMA_HIFCR = 0xFFFFFFFF; //Limpia banderas de interrupcion
	DMA_LIFCR = 0xFFFFFFFF; //Limpia banderas de interrupcion

    DMA2_S0CR |= BIT_7; //Habilita Memory to memory
    //8 bits de source, 8 bits de destination
	//DMA2_S0CR |=  BIT_12; //Source data size (Full word)- Peripheral Data Size
	//DMA2_S0CR |= BIT_14; //Destination data size (Full word) - Memory Data Size
	DMA2_S0CR |= BIT_9; //PINC Habilita incrementar el puntero del source despues de cada transaccion.
	//DMA2_S0CR |= BIT_10; //MINC habilita incrementar el puntero del destination
	DMA2_S0M0AR = (uint32_t)(CRC_DR_ADDRESS); //destination
	DMA2_S0PAR = (uint32_t)(FLASH_START_ADDRESS); //source
	DMA2_S0NDTR = NUM_WORDS;

	DMA2_S0FCR |= BIT_2;//Habilitar FIFO
	DMA2_S0FCR |= 0x03;//BIT_1 | BIT_0; //Threshold level: Full FIFO
	DMA2_S0CR |= BIT_21; //PBURST: INCR4
	DMA2_S0CR |= BIT_23; //MBURST: INCR4

	DMA2_S0CR |= BIT_4; //Habilita interrupcion de transaccion completa.
}
