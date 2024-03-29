/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include "GPIOx.h"
#include "RCC.h"
#include "SYSTICK.h"
#include "NVIC.h"
#include "UARTx.h"
#include "TIMERx.h"
#include "LEDS.h"
#include "CRC.h"
#include "DMA2.h"

#define ACK_BYTE 0x79
#define NACK_BYTE 0x7F
#define START_BYTE 0xFF

void TIMx_RestartStruct(TIM_HandlerDef * tim, TIM_TypeDef * timerbase);
void TIMx_SetChannelCFG(TIM_ChannelCFG * channel, uint8_t CCxE, uint8_t CCxP, float duty);

//modificando codigo
void Function_timeMess(TIM_HandlerDef * timer);
void Function_ReturnTime(TIM_HandlerDef * timer);

TIM_ChannelCFG chcfg[4] = {0};
uint32_t valoractual=0;
TIM_HandlerDef timer5;
float TimerTimeinseg=0;



uint8_t bytes_Received_Counter = 0;
uint8_t prog_Num_Bytes[] = {0,0,0,0};
uint8_t first_Byte = 1;
uint8_t last_Byte = 0;

uint8_t dato;
uint8_t intFlag;
void USART1_IRQHandler(void){
	intFlag = UARTx_GetInterruptFlags(&UART1_Struct);
	while(!(UARTx_SolveCurrentErrors(&UART1_Struct,UARTx_GetCurrentErrors(&UART1_Struct)))){
		UARTx_SolveCurrentErrors(&UART1_Struct,UARTx_GetCurrentErrors(&UART1_Struct));
	}
	if(!((intFlag&UART_SR_RXNE)==0)){
		dato=UARTx_receiveData(&UART1_Struct);
		UART1_Struct.Registers->SR &=~UART_SR_RXNE;
	}else if(!((intFlag&UART_SR_TC)==0)){
		UART1_Struct.Registers->SR &=~UART_SR_TC;
	}else if(!((intFlag&UART_SR_TXE)==0)){
		UARTx_sendData(&UART1_Struct, 0x7E);
		UART1_Struct.Registers->SR &=~UART_SR_TXE;
	}
}
void UART4_IRQHandler(void){
	intFlag = UARTx_GetInterruptFlags(&UART4_Struct);
	while(!(UARTx_SolveCurrentErrors(&UART4_Struct,UARTx_GetCurrentErrors(&UART4_Struct)))){
		UARTx_SolveCurrentErrors(&UART4_Struct,UARTx_GetCurrentErrors(&UART4_Struct));
	}
	if(!((intFlag&UART_SR_RXNE)==0)){
		dato=UARTx_receiveData(&UART4_Struct);
		UART4_Struct.Registers->SR &=~UART_SR_RXNE;
	}else if(!((intFlag&UART_SR_TC)==0)){
		UART4_Struct.Registers->SR &=~UART_SR_TC;
	}else if(!((intFlag&UART_SR_TXE)==0)){
		UARTx_sendData(&UART4_Struct, 0x7E);
		UART4_Struct.Registers->SR &=~UART_SR_TXE;
	}
}

void USART2_IRQHandler(void){
	intFlag = UARTx_GetInterruptFlags(&UART2_Struct);
		while(!(UARTx_SolveCurrentErrors(&UART2_Struct,UARTx_GetCurrentErrors(&UART2_Struct)))){
			UARTx_SolveCurrentErrors(&UART2_Struct,UARTx_GetCurrentErrors(&UART2_Struct));
		}
		if(!((intFlag&UART_SR_RXNE)==0)){
			dato=UARTx_receiveData(&UART2_Struct);
			UART3_Struct.Registers->SR &=~UART_SR_RXNE;
		}else if(!((intFlag&UART_SR_TC)==0)){
			UART2_Struct.Registers->SR &=~UART_SR_TC;
		}else if(!((intFlag&UART_SR_TXE)==0)){

			UART2_Struct.Registers->SR &=~UART_SR_TXE;
		}
}

uint8_t prog_Num_Bytes_Checksum_Local = 0;

void monitoring_Function_Init(){
	uint8_t prog_Num_Bytes_Checksum_Received = 0;
	uint8_t response = 0;

	if(first_Byte == 1){
		dato = UARTx_receiveData(&UART3_Struct);
		if (dato == START_BYTE) {
			UARTx_sendData(&UART3_Struct,ACK_BYTE);
			first_Byte = 0;
		} else {
			first_Byte = 1;
			UARTx_sendData(&UART3_Struct,NACK_BYTE);
		}
	}else if (last_Byte == 1) {
		//DMA_Init();
		response = UARTx_receiveData(&UART3_Struct);
		if (response == START_BYTE) {
			UARTx_sendData(&UART3_Struct,ACK_BYTE);
			Timx_EnableInt(&timer5, TIM_DIER_UIE);
		}else{
			UARTx_sendData(&UART3_Struct,NACK_BYTE);
		}
		first_Byte = 1;
		last_Byte = 0;
		bytes_Received_Counter = 0;
	}else {
		if(bytes_Received_Counter == 4){
			bytes_Received_Counter = 0;
			prog_Num_Bytes_Checksum_Received = UARTx_receiveData(&UART3_Struct);
			prog_Num_Bytes_Checksum_Local = prog_Num_Bytes[3] ^ prog_Num_Bytes[2] ^ prog_Num_Bytes[1] ^ prog_Num_Bytes[0];
			if (prog_Num_Bytes_Checksum_Local == prog_Num_Bytes_Checksum_Received) {
				UARTx_sendData(&UART3_Struct,ACK_BYTE);
				last_Byte = 1;
			}else {
				UARTx_sendData(&UART3_Struct,NACK_BYTE);
				first_Byte = 1;
				bytes_Received_Counter = 0;
			}
		}else{
			prog_Num_Bytes[bytes_Received_Counter] = UARTx_receiveData(&UART3_Struct);
			bytes_Received_Counter++;
		}
	}
}

void USART3_IRQHandler(void){
	intFlag = UARTx_GetInterruptFlags(&UART3_Struct);
		while(!(UARTx_SolveCurrentErrors(&UART3_Struct,UARTx_GetCurrentErrors(&UART3_Struct)))){
			UARTx_SolveCurrentErrors(&UART3_Struct,UARTx_GetCurrentErrors(&UART3_Struct));
		}
		if(!((intFlag&UART_SR_RXNE)==0)){
			//dato=UARTx_receiveData(&UART3_Struct);
			monitoring_Function_Init();
			UART3_Struct.Registers->SR &=~UART_SR_RXNE;
			//LEDS_Toggle();
			//Timx_EnableInt(&timer5, TIM_DIER_UIE);
		}else if(!((intFlag&UART_SR_TC)==0)){
			UART3_Struct.Registers->SR &=~UART_SR_TC;
		}else if(!((intFlag&UART_SR_TXE)==0)){
			UART3_Struct.Registers->SR &=~UART_SR_TXE;
		}
}
void USART6_IRQHandler(void){
	intFlag = UARTx_GetInterruptFlags(&UART6_Struct);
		while(!(UARTx_SolveCurrentErrors(&UART6_Struct,UARTx_GetCurrentErrors(&UART6_Struct)))){
			UARTx_SolveCurrentErrors(&UART6_Struct,UARTx_GetCurrentErrors(&UART6_Struct));
		}
		if(!((intFlag&UART_SR_RXNE)==0)){
			dato=UARTx_receiveData(&UART6_Struct);
			UART6_Struct.Registers->SR &=~UART_SR_RXNE;
		}else if(!((intFlag&UART_SR_TC)==0)){
			UART6_Struct.Registers->SR &=~UART_SR_TC;
		}else if(!((intFlag&UART_SR_TXE)==0)){
			UART6_Struct.Registers->SR &=~UART_SR_TXE;
		}
}

void UART_Config(void){
	/*//Los prescaladores se seleccionan por n=1,2,...,8. Siendo 2^n el valor del preescalador
	SystClock_Init(2,0,80,0,0,0);//SYSCLK -> PLLP, SYSPLL -> HSI, SYSCLK -> 80MHz, preAHB1 -> divided by 2^0
	//preAPB1 -> Not divided, preAPB2 -> not divided, APB1 = 40MHZ, APB2=80MHz.
	NVIC_SetCFGR(30, 3);
    */
	UARTx_ResetStruct(&UART3_Struct,UART3_R);
	UARTx_GPIO_Init(&UART3_Struct);
	UARTx_Init(9600,0x00,0x00,0x00,true,true,&UART3_Struct); //9600 baudrate, sobremuestreo por 8, 8-b de datos, even parity, transmisor y receptor habilitados
	UARTx_InterruptEN(&UART3_Struct,true,false,false,0);
	//UARTx_InterruptEN(&UART3_Struct,false,false,true,1);
	//UARTx_HalfDuplexSingleWire(&UART3_Struct);
}

void Timer_Config(void){
    SYS_CLK.SYSCLK = 16;
	//Los prescaladores se seleccionan por n=1,2,...,8. Siendo 2^n el valor del preescalador
	SystClock_Init(&SYS_CLK,2,1,0,0,0);//SYSCLK -> PLLP, SYSPLL -> HSI, preAHB1 -> divided by 2^1
	//preAPB1 -> Not divided, preAPB2 -> not divided, APB1 = 40MHZ, APB2=40MHz.
	//STK_CONF(currentAHB1CLK);//Configurar después de SYSTCLOCK_INIT, no configurar al usar TIMERS
	TIMx_RestartStruct(&timer5, TIM5);

	timer5.Frequency = 1;  //100Hz->10ms

    timer5.Parameter.channel_mode[0] = TIM_MODE_OUTPWM;
    //PA7 CH1N, PA8 CH1, PA9 CH2 y PB0 CH2N
    //TIMx_GPIO_Init(GPIOA, 8, 1);//PWM1
    //TIMx_GPIO_Init(GPIOA, 9, 1);//PWM2

    //Los canales en este caso no se usan (TIM4 y TIM5); Polaridad no necesaria; Ciclo del 100%
    TIMx_SetChannelCFG(&chcfg[0], TIM_CCxE_OCxOCxN_OFF, TIM_CCxP_OCxOCxN_LOW, 100);

    //0->Reset; 0-> Disable Slave mode; 1->TIM3 as internal trigger; 0->Edge aligned; 0->Upcounter
    TIMx_Init(&timer5, chcfg, 0, 0, TIM_SMCR_TS_ITR1, 0, 0);//
    //0->Reset; 4-> Reset mode in Slave; 2->TIM3 as internal trigger; 0->Edge aligned; 0->Upcounter
    TIM5->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
    NVIC_SetCFGR(50, 3);//TIM5  //Checar

    //Timx_EnableInt(&timer5, TIM_DIER_UIE);
}

int main(void){
	//uint8_t vector[] =  {0,0,2};
	Timer_Config();
	UART_Config();
	LEDS_Init();
	CRC_Enable();
	//DMA_Init(vector,3);
	//DMA_Enable();
	//Timx_EnableInt(&timer5, TIM_DIER_UIE); //Esta linea es de prueba
	while(1){
		//dato=UARTx_receiveData(&UART4_Struct);
		for(int i = 0; i<= 1000000; i++){}
		//UARTx_sendData(&UART3_Struct, 10);
		//LEDS_Toggle();
	}
	return 0;
}

void Function_timeMess(TIM_HandlerDef * timer){
	valoractual=0;//limpia valor actual
	TimerTimeinseg=0;//limpia valor actual en segundos
	timer->Registers -> CNT =0;//limpia contador
	timer->Registers -> CR1 |= TIM_CR1_CEN;//habilita timer
}

void Function_ReturnTime(TIM_HandlerDef * timer){
	valoractual=timer->Registers -> CNT;//guarda valor del timer
	timer->Registers -> CR1 &=~ TIM_CR1_CEN;//deshabilita timer

	//Max time=4294967295*(1/16Mhz)=268.43 seg=4.4minutes
	//convierto valor
	TimerTimeinseg=((((float)timer->Registers -> PSC+1)*(float)valoractual)/15999999);
}

void TIMx_RestartStruct( TIM_HandlerDef * time ,TIM_TypeDef * timerbase){
    time->Registers = timerbase;
    time->Load=0;
    time->Prescaler=0;
    time->Parameter.channel_mode[0]=0;
    time->Parameter.channel_mode[1]=0;
    time->Parameter.channel_mode[2]=0;
    time->Parameter.channel_mode[3]=0;

}

void TIMx_SetChannelCFG(TIM_ChannelCFG * channel, uint8_t CCxE, uint8_t CCxP, float duty){
	channel->CCxE = CCxE; //Habilitación
	channel->CCxP = CCxP; //Polaridad
	channel->DutyCycle = duty; //Ciclo de trabajo
}
