#include <LEDS.h>

void LEDS_Init(void){
	//Configura LED
	RCC_AHB1ENR |= BIT_3;
	for (int i = 0; i < 1000; i++){}; //Espera a que el reloj se estabilice
	GPIOD_MODER |= BIT_30; //Lo configura como salida
	GPIOD_MODER |= BIT_24;
	GPIOD_MODER |= BIT_26;
	GPIOD_MODER |= BIT_28;
}

void LEDS_TurnOn(void){
	GPIOD_ODR |= BIT_12; //Escribe 1 en el puerto
	GPIOD_ODR |= BIT_13; //Escribe 1 en el puerto
	GPIOD_ODR |= BIT_14; //Escribe 1 en el puerto
	GPIOD_ODR |= BIT_15; //Escribe 1 en el puerto
}
void LEDS_TurnOff(void){
	GPIOD_ODR &= ~BIT_12; //Escribe 1 en el puerto
	GPIOD_ODR &= ~BIT_13; //Escribe 1 en el puerto
	GPIOD_ODR &= ~BIT_14; //Escribe 1 en el puerto
	GPIOD_ODR &= ~BIT_15; //Escribe 1 en el puerto
}

void LEDS_Toggle(void){
	GPIOD_ODR ^= BIT_12; //Escribe 1 en el puerto
	GPIOD_ODR ^= BIT_13; //Escribe 1 en el puerto
	//GPIOD_ODR ^= BIT_14; //Escribe 1 en el puerto
	//GPIOD_ODR ^= BIT_15; //Escribe 1 en el puerto
}
