/*
 * TIMERx.c
 *
 *  Created on: Mar 4, 2023
 *      Author: luisju04
 */


#include "TIMERx.h"

uint32_t *valor1 = 0, *valor2 = 0, *valor3 = 0, *valor4 = 0;
uint32_t valor=0;
//uint8_t CRC[] = {0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF};
uint32_t CRC[] = {0xFEFFFFFF,0xFEFFFFFF,0xFEFFFFFF,0xFEFFFFFF,0xFEFFFFFF,0x00000000};

////////////////
void Tim_Cfg (TIM_HandlerDef * timer){
/*****************************************
	1. Enable timer clock
	2. set the direction of counter
	3. set the prescalar and the ARR(auto-reload)
	4. Clear the counter
	*****************************************/

	RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;//reloj timer2
	timer->Registers->CR1 &=~ TIM_CR1_DIR;//Up
	timer->Registers->PSC =0;
	timer->Registers->ARR =0xFFFFFFFF;//MAX VALOR DE RECARGA
	timer->Registers->CNT =0;

}

///////////

void TIMx_GPIO_Init(GPIO_TypeDef *Port_, uint8_t Pin_, uint8_t AFR_){
	//RCC_EnPort(Port_);
	GPIOx_InitAF(Port_, Pin_, GPIO_OTYPER_PP , GPIO_OSPEEDR_0,  AFR_);
}

void TIMx_Init(TIM_HandlerDef * tim, TIM_ChannelCFG * chcfg, uint8_t mms, uint8_t sms, uint8_t ts, uint8_t cms, uint8_t up_down){
	uint8_t timertype = TIMx_VerifyFunctionalities(tim);
	uint8_t i=0, numchannel=0;
	RCC_EnTimx(tim);
	TIMx_DisableCEN(tim);
	if((timertype==TIM_TYPE_ADVANCED)||(timertype==TIM_TYPE_GP_4CH)){
		numchannel=4;
	}
	else if(timertype==TIM_TYPE_GP_2CH){
		numchannel=2;
	}
	else if(timertype==TIM_TYPE_GP_1CH){
		numchannel=1;
	}

	if(numchannel > 1){
		TIMx_SetMMS(tim, mms);//Master Mode
		TIMx_SetSMS(tim, sms);//Slave Mode
		TIMx_SetTS(tim, ts); //trigger selection
		tim->Registers->CCR2 &= ~TIM_CR2_TI1S;//Canal 1 como entrada
		//tim->Registers->SMCR |= TIM_SMCR_MSM;//Delay para mejorar la sincronización
	}

	if((tim->Registers == TIM1)||(tim->Registers == TIM8)||(tim->Registers == TIM9)||(tim->Registers == TIM10)||(tim->Registers == TIM11)){
		//APB2
		if(SYS_CLK.APB2CLK == SYS_CLK.AHB1CLK){//APB PRESCALER > 1
			TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB2CLK)*1000000, numchannel);
		}
		else{
			TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB2CLK)*2000000, numchannel);//El temporizador es un múltiplo de 2
		}
	}
	else{//APB1
		if(SYS_CLK.APB1CLK == SYS_CLK.AHB1CLK){//APB PRESCALER = 1
			TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB1CLK)*1000000, numchannel);
		}
		else{//APB PRESCALER > 1
			TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB1CLK)*2000000, numchannel);//El temporizador es un múltiplo de 2
		}
	}
	//CR1 Minimum conf
	if((timertype!=TIM_TYPE_GP_1CH)||(timertype!=TIM_TYPE_BASIC)){
		TIMx_SetDir(tim, up_down);
	}
	TIMx_SetCMS(tim, cms);
	TIMx_EnableARPE(tim);//ARPE

	while(i<numchannel){
		if((tim->Parameter.channel_mode[i])==TIM_MODE_OUTPWM){
			//CCMR1 Minimum Conf
			//Channel 1 bits 0-7
			//Channel 2 bits 8-15 (Same as above)
			//CCMR2 for CN3 y CN4
			TIMx_SetCMMR(tim,i);
			//CCER Minimum conf
			TIMx_SetPolarity(tim, chcfg, i );
			//BDTR Minimum Conf
			//Se debe configurar respecto a la tabla 109 del CCER
			//for (i = 0; i < 4; ++i) {
			if(timertype==TIM_TYPE_ADVANCED){
				TIMx_OutputCB(tim, chcfg[i].CCxE , i+1);
			}
			else{
				TIMx_OutputCBStandard(tim, chcfg[i].CCxE, i+1);
			}
			//}

		}
		else if(((tim->Parameter.channel_mode[i])==TIM_MODE_INCPT)&&(i==0)){//únicamente canal 1
			tim->Registers->CCMR1 |= (1<<TIM_CCMR1_CC1S_Pos);//as Input IC1 is mapped on TI1 ----------------------------------------------------------
			tim->Registers->CCMR1 &= (~TIM_CCMR1_IC1PSC);//No prescaler
			tim->Registers->CCMR1 &= (~TIM_CCMR1_IC1F);//No filter
			TIMx_SetPolarityDetection(tim, chcfg, i );//Rising edge and falling edge
			tim->Registers->CCER |= TIM_CCER_CC1E; //Capture enable

		}
		//No se pueden combinar en canales temporizadores de conteo hacia arriba y abajo al mismo tiempo
		else if(((tim->Parameter.channel_mode[i])==TIM_MODE_UPCOUNT)||((tim->Parameter.channel_mode[i])==TIM_MODE_DWCOUNT)){
			TIMx_SetPolarity(tim, chcfg, i );
			if(timertype==TIM_TYPE_ADVANCED){
				TIMx_OutputCB(tim, chcfg[i].CCxE , i+1);
			}
			else{
				TIMx_OutputCBStandard(tim, chcfg[i].CCxE, i+1);
			}

		}
		//else{}//No se eligió algún modo de operación
		i++;
	}

	TIMx_SetUG(tim);
	TIMx_EnableCEN(tim);
}

//Función para manejar las interrupciones
void TIM_CallBack(TIM_HandlerDef * tim){
	uint8_t bitflag =  (tim->Registers->DIER)&0xFF;

	if(bitflag==TIM_DIER_UIE){

	}
	else if(bitflag==TIM_DIER_CC1IE){

	}
	else if(bitflag==TIM_DIER_CC2IE){

	}
	else if(bitflag==TIM_DIER_CC3IE){

	}
	else if(bitflag==TIM_DIER_CC4IE){

	}
	else if(bitflag==TIM_DIER_COMIE){

	}
	else if(bitflag==TIM_DIER_TIE){

	}
	else if(bitflag==TIM_DIER_BIE){

	}

}

void TIM1_UP_TIM10_IRQHandler(){
	//Limpiar bandera
	TIM10->SR &= (~TIM_SR_UIF);
}

void TIM1_TRG_COM_TIM11_IRQHandler(){
	TIM11->SR &= (~TIM_SR_UIF);
}

void TIM8_UP_TIM13_IRQHandler(){
	TIM13->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
}

void TIM2_IRQHandler(){
	//Limpiar bandera
	*valor1 = TIM2->CNT;
	TIM2->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
}

void TIM3_IRQHandler(){
	//Limpiar bandera
	*valor2 = TIM3->CNT;
	TIM3->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
}

void TIM4_IRQHandler(){
	//Limpiar bandera
	*valor3 = TIM4->CNT;
	TIM4->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
}

void TIM5_IRQHandler(){
	//uint32_t CRC = 0;
	TIM5->SR &= (~(TIM_SR_UIF|TIM_SR_TIF));
	/*                   Agregar código
	 *                                */
	LEDS_Toggle();
	//CRC = CRC_Read();
	//DMA_Enable();
	UARTx_sendData(&UART3_Struct,(CRC[valor]>>24)&0xFF);
	UARTx_sendData(&UART3_Struct,(CRC[valor]>>16)&0xFF);
	UARTx_sendData(&UART3_Struct,(CRC[valor]>>8)&0xFF);
	UARTx_sendData(&UART3_Struct,(CRC[valor])&0xFF);
	valor++;
}

//Función para varios valores de ciclo
//Use of internal CLK for TIMES
//Frequency in Hz
bool TIMx_InternalCLK(TIM_HandlerDef * tim, TIM_ChannelCFG * chcfg, uint32_t in_Freq, uint8_t numchannels){//could be float duty
	bool flag = false;//Bandera para indicar operación realizada

	uint32_t load=0; //= (in_Freq / out_Freq);
	uint32_t compareValue=0; //= (duty * load);
	uint16_t res=0;//Residuo para máximo dos decimales
	uint8_t i=0, escaler=100;//Auxiliares; escalador para obtener dos decimales
	uint32_t prescaler=1; //Prescalador
	float out_Freq = tim->Frequency;
	float aux;//Se usa un auxiliar flotante para las primeras operaciones, después se opera con uint32 para mejorar la precisión

	//Uint32 rango de operación

	for (i = 0; i < numchannels; ++i) {
		if((tim->Parameter.channel_mode[i])!=TIM_MODE_OUTPWM){//Si no es PWM
			chcfg[i].DutyCycle = 100;
			//Se puede considerar el 50% 0 100% del ciclo del trabajo
		}
		else if(chcfg[i].DutyCycle==0){
			chcfg[i].DutyCycle = 10;
		}
	}

	while((!flag)){//Se opera mientras no se encuentre coincidencia
		prescaler=1;//Inicializa prescalador

		aux = ( (float)in_Freq / (float)out_Freq);//Cálculo de carga
		if(escaler==1){//Si no se encontró coincidencia, hacemos que coincida la aproximación
			flag=true;
		}
		else{
			aux = (unsigned int)(aux*escaler);//Obtenemos el entero para mayor precisión y escalamos
		}
		//Ingresamos dentro del rango del contador y se busca el máximo valor permitido para la carga
		while(aux > (65535*escaler)){//valor del contador de 16 bits escalado
			prescaler++;
			aux = ( (float)in_Freq / (float)(out_Freq*prescaler));//Carga
			if(escaler>1){//Si el escalador llega a 1, no es necesario escalar
				aux = (unsigned int)(aux*escaler);//Valor entero de carga escalado
			}
		}
		if(escaler==1){
			load = (uint32_t)aux;//Casteo de la carga y almacenamiento para operar el valor correcto
		}
		while((!flag)&&(prescaler<65535)){//Se busca un valor que coincida mejor, y que no exceda el preescalador
			i=0;//Se reinicia cuenta
			flag=true;//Se inicializa la bandera
			load = (uint32_t)aux;//Casteo de la carga y almacenamiento para operar el valor correcto
			while((i<numchannels)||(numchannels==0)){
				if(numchannels==0){
					res = escaler*(load/escaler);//Ponemos en cero las decimales escaladas
					res= load - res;//Obtenemos los últimos dos decimales
					if(res){//Si no son cero, se busca una mejor aproximación
						flag&=false;
					}
					else{
						flag&=true;
					}
				}
				else if(tim->Parameter.channel_mode[i]){//Verifica que haya un modo ==TIM_MODE_OUTPWM
					aux = (chcfg[i].DutyCycle * load);//Obtiene el valor a comparar escalado
					compareValue = (uint32_t)aux;//Obtenemos el valor a comparar escalado y almacenamos
					res = compareValue - (100*escaler*(compareValue/escaler/100));//Ponemos en cero las decimales escaladas
					//res= compareValue - res;//Obtenemos los últimos dos decimales
					if(res){//Si no son cero, se busca una mejor aproximación
						flag&=false;
					}
					else{
						flag&=true;
					}
				}
				i++;

			}
			if(!flag){
				prescaler++;//Incrementa el preescalador
				aux = ( (float)in_Freq / (float)(out_Freq*prescaler));//Se calcula la carga de nuevo
				aux = (unsigned int)(aux*escaler);//Se escala el valor entero de la carga
			}
		}
		if(!flag){
			escaler/=10;//Si no se encuentra con dos decimales, se disminuye a una, hasta ninguna
		}
	}

	if((!res)&&(flag)){//Se encontró coincidencia
		tim->Registers->PSC = prescaler-1;
		tim->Prescaler=prescaler;
		tim->Registers->ARR = ((load/escaler))-1;
		tim->Load=(load/escaler);
		if(numchannels!=0){
			if(tim->Parameter.channel_mode[0]){//Canal 1
				//Cambiar
				if(chcfg[0].DutyCycle!=0){
					aux = (chcfg[0].DutyCycle * load);
					compareValue = (uint32_t)aux;
					tim->Registers->CCR1 = ( ( ( compareValue / 100 ) / escaler) ) - 1;
				}
			}
			if(tim->Parameter.channel_mode[1]){//Canal 2
				if(chcfg[1].DutyCycle!=0){
					aux = (chcfg[1].DutyCycle * load);
					compareValue = (uint32_t)aux;
					tim->Registers->CCR2 = ( ( ( compareValue / 100 ) / escaler) ) - 1;
				}
			}
			if(tim->Parameter.channel_mode[2]){//Canal 3
				if(chcfg[2].DutyCycle!=0){
					aux = (chcfg[2].DutyCycle * load);
					compareValue = (uint32_t)aux;
					tim->Registers->CCR3 = ( ( ( compareValue / 100 ) / escaler) ) - 1;
				}
			}
			if(tim->Parameter.channel_mode[3]){//Canal 4
				if(chcfg[3].DutyCycle!=0){
					aux = (chcfg[3].DutyCycle * load);
					compareValue = (uint32_t)aux;
					tim->Registers->CCR4 = ( ( ( compareValue / 100 ) / escaler) ) - 1;
				}
			}
		}
		flag=true;
	}
	else{//No se encontró coincidencia
		flag = false; //Indica que no se completó
	}
	return flag;//Siempre debe regresar true
}

//Only can use OPM in all channels
void TIMx_OPM(TIM_HandlerDef * tim, TIM_ChannelCFG * chcfg, uint8_t cms, uint8_t mms, uint8_t up_down){
	uint8_t timertype = TIMx_VerifyFunctionalities(tim);
	uint8_t i=0, numchannel=0;
	RCC_EnTimx(tim);
	TIMx_DisableCEN(tim);
	if((timertype==TIM_TYPE_ADVANCED)||(timertype==TIM_TYPE_GP_4CH)){
		numchannel=4;
	}
	else if(timertype==TIM_TYPE_GP_2CH){
		numchannel=2;
	}
	else if(timertype==TIM_TYPE_GP_1CH){
		numchannel=1;
	}
	TIMx_EnableOPM(tim);//ONE PULSE MODE
	if((tim->Registers == TIM1)||(tim->Registers == TIM8)){//APB2
		TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB2CLK)*1000000, numchannel);
	}
	else{//APB1
		TIMx_InternalCLK(tim, chcfg, (SYS_CLK.APB1CLK)*1000000, numchannel);
	}
	TIMx_SetDir(tim, up_down);
	TIMx_SetCMS(tim, cms);
	TIMx_EnableARPE(tim);
	TIMx_SetMMS(tim, mms);
	for (i = 0; i < 4; ++i) {
		TIMx_SetCMMR(tim,i);
		//CCER Minimum conf
		TIMx_SetPolarity(tim, chcfg, i );
		//BDTR Minimum Conf
		//Se debe configurar respecto a la tabla 109 del CCER
		//for (i = 0; i < 4; ++i) {
		TIMx_OutputCB(tim, chcfg[i].CCxE, i+1);
	}
	TIMx_SetUG(tim);
	TIMx_EnableCEN(tim);
}

/*Output control Bits
Parameter OCX_OCXN values:
0 OCX, 1 OCXN, 2 both OCX and OCXN, 3 OCX off state and OCXN ON, 4 OCX ON and OCXN off state,
5 both OCX and OCXN off state
*/
void TIMx_OutputCB(TIM_HandlerDef *tim, uint8_t OCX_OCXN, uint8_t CN){
	if(OCX_OCXN==0){//ONLY OCX EN
		//Possible combinations
		tim->Registers->BDTR |= TIM_BDTR_MOE;//MOE=1
		//OSSI=x
		tim->Registers->BDTR &= (~TIM_BDTR_OSSR);//OSSR=0 or 1
		if(CN==1){
			tim->Registers->CCER |= (TIM_CCER_CC1E);//OUTPUT ENABLED CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC1NE);//CCXNE=0 or 1 if only 1 cn CCXNE=0
		}
		else if(CN==2){
			tim->Registers->CCER |= (TIM_CCER_CC2E);//OUTPUT ENABLED CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC2NE);//CCXNE=0 or 1 if only 1 cn CCXNE=0
		}
		else if(CN==3){
			tim->Registers->CCER |= (TIM_CCER_CC3E);//OUTPUT ENABLED CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC3NE);//CCXNE=0 or 1 if only 1 cn CCXNE=0
		}
		else if(CN==4){
			tim->Registers->CCER |= (TIM_CCER_CC4E);//OUTPUT ENABLED CCXE=1
			//tim->Registers->CCER &= (~TIM_CCER_CC4NE);//CCXNE=0 or 1 if only 1 cn CCXNE=0
		}

	}
	else if(OCX_OCXN==1){
		//Possible combinations
		tim->Registers->BDTR |= TIM_BDTR_MOE;//MOE=1
		//OSSI=x
		tim->Registers->BDTR &= (~TIM_BDTR_OSSR);//OSSR=0 or 1
		if(CN==1){
			tim->Registers->CCER &= (~TIM_CCER_CC1E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC1NE);//CCXNE=1
		}
		else if(CN==2){
			tim->Registers->CCER &= (~TIM_CCER_CC2E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC2NE);//CCXNE=1
		}
		else if(CN==3){
			tim->Registers->CCER &= (~TIM_CCER_CC3E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC3NE);//CCXNE=1
		}
		else if(CN==4){
			tim->Registers->CCER &= (~TIM_CCER_CC4E);//CCXE=0
			//tim->Registers->CCER |= (TIM_CCER_CC4NE);//CCXNE=1
		}
	}
	else if(OCX_OCXN==2){
		tim->Registers->BDTR |= TIM_BDTR_MOE;//MOE=1
		//OSSI=x
		tim->Registers->BDTR &= (~TIM_BDTR_OSSR);//OSSR=0 or 1
		if(CN==1){
			tim->Registers->CCER |= (TIM_CCER_CC1E);//CCXE=1
			tim->Registers->CCER |= (TIM_CCER_CC1NE);//CCXNE=1
		}
		else if(CN==2){
			tim->Registers->CCER |= (TIM_CCER_CC2E);//CCXE=1
			tim->Registers->CCER |= (TIM_CCER_CC2NE);//CCXNE=1
		}
		else if(CN==3){
			tim->Registers->CCER |= (TIM_CCER_CC3E);//CCXE=1
			tim->Registers->CCER |= (TIM_CCER_CC3NE);//CCXNE=1
		}
		else if(CN==4){
			tim->Registers->CCER |= (TIM_CCER_CC4E);//CCXE=1
			//tim->Registers->CCER |= (TIM_CCER_CC4NE);//CCXNE=1
		}
	}
	else if(OCX_OCXN==3){
		tim->Registers->BDTR |= TIM_BDTR_MOE;//MOE=1
		//OSSI=x
		tim->Registers->BDTR &= (~TIM_BDTR_OSSR);//OSSR=1
		if(CN==1){
			tim->Registers->CCER &= (~TIM_CCER_CC1E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC1NE);//CCXNE=1
		}
		else if(CN==2){
			tim->Registers->CCER &= (~TIM_CCER_CC2E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC2NE);//CCXNE=1
		}
		else if(CN==3){
			tim->Registers->CCER &= (~TIM_CCER_CC3E);//CCXE=0
			tim->Registers->CCER |= (TIM_CCER_CC3NE);//CCXNE=1
		}
		else if(CN==4){
			tim->Registers->CCER &= (~TIM_CCER_CC4E);//CCXE=0
			//tim->Registers->CCER |= (TIM_CCER_CC4NE);//CCXNE=1
		}
	}
	else if(OCX_OCXN==4){
		tim->Registers->BDTR |= TIM_BDTR_MOE;//MOE=1
		//OSSI=x
		tim->Registers->BDTR |= (TIM_BDTR_OSSR);//OSSR=1
		if(CN==1){
			tim->Registers->CCER |= (TIM_CCER_CC1E);//CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC1NE);//CCXNE=0
		}
		else if(CN==2){
			tim->Registers->CCER |= (TIM_CCER_CC2E);//CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC2NE);//CCXNE=0
		}
		else if(CN==3){
			tim->Registers->CCER |= (TIM_CCER_CC3E);//CCXE=1
			tim->Registers->CCER &= (~TIM_CCER_CC3NE);//CCXNE=0
		}
		else if(CN==4){
			tim->Registers->CCER |= (TIM_CCER_CC4E);//CCXE=1
			//tim->Registers->CCER &= (~TIM_CCER_CC4NE);//CCXNE=0
		}
	}
	else if(OCX_OCXN==5){
		tim->Registers->BDTR &= (~TIM_BDTR_MOE);//MOE=0
		tim->Registers->BDTR |= (TIM_BDTR_OSSI);//OSSI=1
		//OSSR=x
		if(CN==1){
			tim->Registers->CCER |= (TIM_CCER_CC1E);//CCXE=0 or 1
			tim->Registers->CCER |= (TIM_CCER_CC1NE);//CCXNE=if CCXE 0 CCXNE=1, other 0 or 1
		}
		else if(CN==2){
			tim->Registers->CCER |= (TIM_CCER_CC2E);//CCXE=0 or 1
			tim->Registers->CCER |= (TIM_CCER_CC2NE);//CCXNE=if CCXE 0 CCXNE=1, other 0 or 1
		}
		else if(CN==3){
			tim->Registers->CCER |= (TIM_CCER_CC3E);//CCXE=0 or 1
			tim->Registers->CCER |= (TIM_CCER_CC3NE);//CCXNE=if CCXE 0 CCXNE=1, other 0 or 1
		}
		else if(CN==4){
			tim->Registers->CCER |= (TIM_CCER_CC4E);//CCXE=0 or 1
			//tim->Registers->CCER |= (TIM_CCER_CC4NE);//CCXNE=if CCXE 0 CCXNE=1, other 0 or 1
		}
	}
	//else{}//Disabled OCx and OCxN
}

void TIMx_OutputCBStandard(TIM_HandlerDef *tim, uint8_t OCX, uint8_t CN){
	if(OCX==1){//ONLY OCX EN
		if(CN==1){
			tim->Registers->CCER |= (TIM_CCER_CC1E);//OUTPUT ENABLED CCXE=1
		}
		else if(CN==2){
			tim->Registers->CCER |= (TIM_CCER_CC2E);//OUTPUT ENABLED CCXE=1
		}
		else if(CN==3){
			tim->Registers->CCER |= (TIM_CCER_CC3E);//OUTPUT ENABLED CCXE=1
		}
		else if(CN==4){
			tim->Registers->CCER |= (TIM_CCER_CC4E);//OUTPUT ENABLED CCXE=1
		}
	}
	else if(OCX==0){//ONLY OCX DISABLE
		if(CN==1){
			tim->Registers->CCER &= (~TIM_CCER_CC1E);//OUTPUT DISABLED CCXE=0
		}
		else if(CN==2){
			tim->Registers->CCER &= (~TIM_CCER_CC2E);//OUTPUT DISABLED CCXE=0
		}
		else if(CN==3){
			tim->Registers->CCER &= (~TIM_CCER_CC3E);//OUTPUT DISABLED CCXE=0
		}
		else if(CN==4){
			tim->Registers->CCER &= (~TIM_CCER_CC4E);//OUTPUT DISABLED CCXE=0
		}
	}
	//else{}//Disabled OCx and OCxN
}


void TIMx_SetCMMR(TIM_HandlerDef *tim, uint8_t i){
	//uint8_t i=0;
	//while(i<4){
	if((tim->Parameter.channel_mode[i])==TIM_MODE_OUTPWM){
		if(i==0){
			tim->Registers->CCMR1 &= (~(0<<TIM_CCMR1_CC1S_Pos));//as Output
			tim->Registers->CCMR1 |= (TIM_CCMR1_OC1PE);//Preload Enabled
			tim->Registers->CCMR1 |= (6<<TIM_CCMR1_OC1M_Pos);//Mode OpPWM1
		}
		else if(i==1){
			tim->Registers->CCMR1 &= (~(0<<TIM_CCMR1_CC2S_Pos));//as Output
			tim->Registers->CCMR1 |= (TIM_CCMR1_OC2PE);//Preload Enabled
			tim->Registers->CCMR1 |= (6<<TIM_CCMR1_OC2M_Pos);//Mode OpPWM1
		}
		else if(i==2){
			tim->Registers->CCMR2 &= (~(0<<TIM_CCMR2_CC3S_Pos));//as Output
			tim->Registers->CCMR2 |= (TIM_CCMR2_OC3PE);//Preload Enabled
			tim->Registers->CCMR2 |= (6<<TIM_CCMR2_OC3M_Pos);//Mode OpPWM1
		}
		else if(i==3){
			tim->Registers->CCMR2 &= (~(0<<TIM_CCMR2_CC4S_Pos));//as Output
			tim->Registers->CCMR2 |= (TIM_CCMR2_OC4PE);//Preload Enabled
			tim->Registers->CCMR2 |= (6<<TIM_CCMR2_OC4M_Pos);//Mode OpPWM1
		}
	}
	else if((tim->Parameter.channel_mode[i])==TIM_MODE_OUTOP){
		if(i==0){
			tim->Registers->CCMR1 &= (~(0<<TIM_CCMR1_CC1S_Pos));//as Output
			tim->Registers->CCMR1 |= (TIM_CCMR1_OC1PE);//Preload Enabled
			tim->Registers->CCMR1 |= (7<<TIM_CCMR1_OC1M_Pos);//Mode PWM 2 for a pulse 0 to 1 and 1 to 0
		}
		else if(i==1){
			tim->Registers->CCMR1 &= (~(0<<TIM_CCMR1_CC2S_Pos));//as Output
			tim->Registers->CCMR1 |= (TIM_CCMR1_OC2PE);//Preload Enabled
			tim->Registers->CCMR1 |= (7<<TIM_CCMR1_OC2M_Pos);//Mode PWM 2 for a pulse 0 to 1 and 1 to 0
		}
		else if(i==2){
			tim->Registers->CCMR2 &= (~(0<<TIM_CCMR2_CC3S_Pos));//as Output
			tim->Registers->CCMR2 |= (TIM_CCMR2_OC3PE);//Preload Enabled
			tim->Registers->CCMR2 |= (7<<TIM_CCMR2_OC3M_Pos);//Mode PWM 2 for a pulse 0 to 1 and 1 to 0
		}
		else if(i==3){
			tim->Registers->CCMR2 &= (~(0<<TIM_CCMR2_CC4S_Pos));//as Output
			tim->Registers->CCMR2 |= (TIM_CCMR2_OC4PE);//Preload Enabled
			tim->Registers->CCMR2 |= (7<<TIM_CCMR2_OC4M_Pos);//Mode PWM 2 for a pulse 0 to 1 and 1 to 0
		}
	}
	else if((tim->Parameter.channel_mode[i])==TIM_MODE_OUTCMP){

	}
	//i++;
	//}

}
//0 OCx Lox; 1 OCxN Low; 2 both Low
//3 OCx High; 4OCxN High; 5 both high
void TIMx_SetPolarity(TIM_HandlerDef *tim, TIM_ChannelCFG *chcfg, uint8_t i){
	//uint8_t i=0;
	//while(i<4){
	if(chcfg[i].CCxP==0){
		if(i==0){
			tim->Registers->CCER |= (TIM_CCER_CC1P);//PLOARITY LOW
		}
		else if(i==1){
			tim->Registers->CCER |= (TIM_CCER_CC2P);//PLOARITY LOW
		}
		else if(i==2){
			tim->Registers->CCER |= (TIM_CCER_CC3P);//PLOARITY LOW
		}
		else if(i==3){
			tim->Registers->CCER |= (TIM_CCER_CC4P);//PLOARITY LOW
		}
	}
	else if(chcfg[i].CCxP==1){
		if(i==0){
			tim->Registers->CCER |= (TIM_CCER_CC1NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==1){
			tim->Registers->CCER |= (TIM_CCER_CC2NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==2){
			tim->Registers->CCER |= (TIM_CCER_CC3NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==3){
			tim->Registers->CCER |= (TIM_CCER_CC4NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
	}
	else if(chcfg[i].CCxP==2){
		if(i==0){
			tim->Registers->CCER |= (TIM_CCER_CC1P);//PLOARITY LOW
			tim->Registers->CCER |= (TIM_CCER_CC1NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==1){
			tim->Registers->CCER |= (TIM_CCER_CC2P);//PLOARITY LOW
			tim->Registers->CCER |= (TIM_CCER_CC2NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==2){
			tim->Registers->CCER |= (TIM_CCER_CC3P);//PLOARITY LOW
			tim->Registers->CCER |= (TIM_CCER_CC3NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
		else if(i==3){
			tim->Registers->CCER |= (TIM_CCER_CC4P);//PLOARITY LOW
			tim->Registers->CCER |= (TIM_CCER_CC4NP);//PLOARITY LOW OF COMPLEMENTARY OUT
		}
	}
	else if(chcfg[i].CCxP==3){
		if(i==0){
			tim->Registers->CCER &= (~TIM_CCER_CC1P);//PLOARITY HIGH
		}
		else if(i==1){
			tim->Registers->CCER &= (~TIM_CCER_CC2P);//PLOARITY HIGH
		}
		else if(i==2){
			tim->Registers->CCER &= (~TIM_CCER_CC3P);//PLOARITY HIGH
		}
		else if(i==3){
			tim->Registers->CCER &= (~TIM_CCER_CC4P);//PLOARITY HIGH
		}
	}
	else if(chcfg[i].CCxP==4){
		if(i==0){
			tim->Registers->CCER &= (~TIM_CCER_CC1NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==1){
			tim->Registers->CCER &= (~TIM_CCER_CC2NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==2){
			tim->Registers->CCER &= (~TIM_CCER_CC3NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==3){
			tim->Registers->CCER &= (~TIM_CCER_CC4NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
	}
	else if(chcfg[i].CCxP==5){
		if(i==0){
			tim->Registers->CCER &= (~TIM_CCER_CC1P);//PLOARITY HIGH
			tim->Registers->CCER &= (~TIM_CCER_CC1NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==1){
			tim->Registers->CCER &= (~TIM_CCER_CC2P);//PLOARITY HIGH
			tim->Registers->CCER &= (~TIM_CCER_CC2NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==2){
			tim->Registers->CCER &= (~TIM_CCER_CC3P);//PLOARITY HIGH
			tim->Registers->CCER &= (~TIM_CCER_CC3NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
		else if(i==3){
			tim->Registers->CCER &= (~TIM_CCER_CC4P);//PLOARITY HIGH
			tim->Registers->CCER &= (~TIM_CCER_CC4NP);//PLOARITY HIGH OF COMPLEMENTARY OUT
		}
	}
	//else{}
	//i++;
	//}
}

void TIMx_SetPolarityDetection(TIM_HandlerDef *tim, TIM_ChannelCFG *chcfg, uint8_t i){
	if(chcfg[i].CCxP==TIM_CCxP_ICx_FALLING){
		tim->Registers->CCER |= TIM_CCER_CC1P;
	}
	else if(chcfg[i].CCxP==TIM_CCxP_ICx_BOTHEDGES){
		tim->Registers->CCER |= (3<<TIM_CCER_CC1P_Pos);
	}
	else{
		tim->Registers->CCER &= ~(3<<TIM_CCER_CC1P_Pos);
	}
}

//Master Mode Selection
void TIMx_SetMMS(TIM_HandlerDef *tim, uint8_t mms){
	tim->Registers->CR2 |= (mms<<(TIM_CR2_MMS_Pos));//
}
//Slave Mode Selection
void TIMx_SetSMS(TIM_HandlerDef *tim, uint8_t sms){
	tim->Registers->SMCR |= (sms<<(TIM_SMCR_SMS_Pos));//
}

//Trigger Selection
void TIMx_SetTS(TIM_HandlerDef *tim, uint8_t ts){
	tim->Registers->SMCR |= (ts<<(TIM_SMCR_TS_Pos));//
}

void TIMx_SetDir(TIM_HandlerDef *tim, uint8_t up_dw){
	if(up_dw){//downcounter
		tim->Registers->CR1 |= (TIM_CR1_DIR);//Down
	}
	else{//upcounter
		tim->Registers->CR1 &= (~(0<<TIM_CR1_DIR_Pos));//Up
	}
}

void TIMx_SetCMS(TIM_HandlerDef *tim, uint8_t cms){
	if(cms==0){//Edge aligned
		tim->Registers->CR1 &= (~(0<<TIM_CR1_CMS_Pos));//Edge aligned
	}
	else if(cms==1){//Center aligned mode 1
		tim->Registers->CR1 |= (TIM_CR1_CMS_0);//
	}
	else if(cms==2){//Center aligned mode 2
		tim->Registers->CR1 |= (TIM_CR1_CMS_1);//
	}
	else if(cms==3){//Center aligned mode 3
		tim->Registers->CR1 |= (3<<TIM_CR1_CMS_Pos);//
	}

}
//Update generation
void TIMx_SetUG(TIM_HandlerDef *tim){
	tim->Registers->EGR |= (TIM_EGR_UG);//Update generation, restart counter and prescaler counter reinitialize
}
//Repetition Counter
void TIMx_SetRCR(TIM_HandlerDef *tim, uint8_t rcr){
	tim->Registers->RCR = rcr;
}

//Dead Time insertion: Se deben activar ambos canales (normal y complementario) para este modo
//Time in ns
bool TIMx_SetDTG(TIM_HandlerDef *tim, uint8_t in_Freq, uint32_t deadtime){
	uint32_t dtg = ((in_Freq) * (deadtime))/1000;//Calculando valor (MHz*ns)(10^(-9+6)=10^(-3))
	bool flag = true;
	if(dtg>(128-1)){
		dtg = ((in_Freq) * (deadtime))/(2*1000) - 64;//segunda forma de cálculo

		if(dtg>(64-1)){
			dtg = ((in_Freq) * (deadtime))/(8*1000) - 32;//Tercera forma de cálculo

			if(dtg>(32-1)){
				dtg = ((in_Freq) * (deadtime))/(16*1000) - 32;//Cuarta forma de cálculo

				if(dtg>(32-1)){
					flag=false;//No se encontró un valor adecuado
				}
				else{
					tim->Registers->BDTR |= (0B11100000|((0x1F&dtg)<<TIM_BDTR_DTG_Pos));
				}
			}
			else{
				tim->Registers->BDTR |= (0B11000000|((0x1F&dtg)<<TIM_BDTR_DTG_Pos));
			}
		}
		else{
			tim->Registers->BDTR |= (0B10000000|((0x3F&dtg)<<TIM_BDTR_DTG_Pos));
		}
	}
	else{
		tim->Registers->BDTR |= (0B01111111&((0x3F&dtg)<<TIM_BDTR_DTG_Pos));
	}
	return flag;
}

bool TIMx_SetCCRx(TIM_HandlerDef *tim, uint16_t CCR, uint8_t channel){
	bool flag = true;

	if(CCR!=0){
		CCR--;
	}

	if(channel==1){
	tim->Registers->CCR1 = CCR;
	}
	else if(channel==2){
		tim->Registers->CCR2 = CCR;
	}
	else if(channel==3){
		tim->Registers->CCR3 = CCR;
	}
	else if(channel==4){
		tim->Registers->CCR4 = CCR;
	}
	else {
		flag = false;
	}

	return flag;
}

//SET IDLE STATE

void TIMx_EnableCEN(TIM_HandlerDef *tim){
	tim -> Registers -> CR1 |= TIM_CR1_CEN;
}

void TIMx_DisableCEN(TIM_HandlerDef *tim){
	tim -> Registers -> CR1 &= ~TIM_CR1_CEN;
}

void TIMx_EnableARPE(TIM_HandlerDef *tim){
	tim->Registers->CR1 |= TIM_CR1_ARPE;//Auto-reload preload enabled
}

void TIMx_DisableARP(TIM_HandlerDef *tim){
	tim->Registers->CR1 &= ~TIM_CR1_ARPE;//Auto-reload preload enabled
}

//ONE PULSE MODE
void TIMx_EnableOPM(TIM_HandlerDef *tim){
	tim->Registers->CR1 |= TIM_CR1_OPM;;
}

void TIMx_DisableOPM(TIM_HandlerDef *tim){
	tim->Registers->CR1 &= ~TIM_CR1_OPM;
}

//Interruptions
void Timx_EnableInt(TIM_HandlerDef *tim, uint16_t interrupt){
	tim->Registers->DIER = interrupt & 0xFF;
}

void Timx_DisableInt(TIM_HandlerDef *tim, uint16_t interrupt){
	tim->Registers->DIER &= (~(interrupt&0xFF)) ;
}


void RCC_EnTimx(TIM_HandlerDef *tim){
	if(tim->Registers==TIM1){
		RCC_APB2ENR |= (RCC_APB2ENR_TIM1EN) ;//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM2){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM2EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM3){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM3EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM4){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM4EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM5){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM5EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM6){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM6EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM7){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM7EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM8){
		RCC_APB2ENR |= (RCC_APB2ENR_TIM8EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM9){
		RCC_APB2ENR |= (RCC_APB2ENR_TIM9EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM10){
		RCC_APB2ENR |= (RCC_APB2ENR_TIM10EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM11){
		RCC_APB2ENR |= (RCC_APB2ENR_TIM11EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM12){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM12EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM13){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM13EN);//Habilitar Fuente de reloj antes de configurar
	}
	else if(tim->Registers==TIM14){
		RCC_APB1ENR |= (RCC_APB1ENR_TIM14EN);//Habilitar Fuente de reloj antes de configurar
	}

}

uint8_t TIMx_VerifyFunctionalities(TIM_HandlerDef *tim){
	uint8_t timertype=0;
	if((tim->Registers==TIM1)||(tim->Registers==TIM8)){
		timertype=TIM_TYPE_ADVANCED;//Advanced
	}
	else if((tim->Registers==TIM2)||(tim->Registers==TIM3)||(tim->Registers==TIM4)||(tim->Registers==TIM5)){
		timertype=TIM_TYPE_GP_4CH;//General Purpose: 4 channels, not complementary
	}
	else if((tim->Registers==TIM9)||(tim->Registers==TIM12)){
		timertype=TIM_TYPE_GP_2CH;//General Purpose: 2 channels, not complementary
	}
	else if((tim->Registers==TIM10)||(tim->Registers==TIM11)||(tim->Registers==TIM13)||(tim->Registers==TIM14)){
		timertype=TIM_TYPE_GP_1CH;//General Purpose: 1 channel, not complementary
	}
	else if((tim->Registers==TIM6)||(tim->Registers==TIM7)){
		timertype=TIM_TYPE_BASIC;//Basic Timer, not channels
	}
	return timertype;
}

void TIMx_ClearUIFlag(TIM_HandlerDef *tim){
	tim->Registers->SR &= (~TIM_SR_UIF);
}
