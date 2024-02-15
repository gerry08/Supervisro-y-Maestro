################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/CRC.c \
../Src/DMA2.c \
../Src/FPU.c \
../Src/GPIOx.c \
../Src/LEDS.c \
../Src/NVIC.c \
../Src/RCC.c \
../Src/SYSCFG.c \
../Src/SYSTICK.c \
../Src/TIMERx.c \
../Src/UARTx.c \
../Src/embeddedFLASH.c \
../Src/main.c \
../Src/syscalls.c \
../Src/sysmem.c 

OBJS += \
./Src/CRC.o \
./Src/DMA2.o \
./Src/FPU.o \
./Src/GPIOx.o \
./Src/LEDS.o \
./Src/NVIC.o \
./Src/RCC.o \
./Src/SYSCFG.o \
./Src/SYSTICK.o \
./Src/TIMERx.o \
./Src/UARTx.o \
./Src/embeddedFLASH.o \
./Src/main.o \
./Src/syscalls.o \
./Src/sysmem.o 

C_DEPS += \
./Src/CRC.d \
./Src/DMA2.d \
./Src/FPU.d \
./Src/GPIOx.d \
./Src/LEDS.d \
./Src/NVIC.d \
./Src/RCC.d \
./Src/SYSCFG.d \
./Src/SYSTICK.d \
./Src/TIMERx.d \
./Src/UARTx.d \
./Src/embeddedFLASH.d \
./Src/main.d \
./Src/syscalls.d \
./Src/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F407G_DISC1 -DSTM32F4 -DSTM32F407VGTx -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/CRC.d ./Src/CRC.o ./Src/CRC.su ./Src/DMA2.d ./Src/DMA2.o ./Src/DMA2.su ./Src/FPU.d ./Src/FPU.o ./Src/FPU.su ./Src/GPIOx.d ./Src/GPIOx.o ./Src/GPIOx.su ./Src/LEDS.d ./Src/LEDS.o ./Src/LEDS.su ./Src/NVIC.d ./Src/NVIC.o ./Src/NVIC.su ./Src/RCC.d ./Src/RCC.o ./Src/RCC.su ./Src/SYSCFG.d ./Src/SYSCFG.o ./Src/SYSCFG.su ./Src/SYSTICK.d ./Src/SYSTICK.o ./Src/SYSTICK.su ./Src/TIMERx.d ./Src/TIMERx.o ./Src/TIMERx.su ./Src/UARTx.d ./Src/UARTx.o ./Src/UARTx.su ./Src/embeddedFLASH.d ./Src/embeddedFLASH.o ./Src/embeddedFLASH.su ./Src/main.d ./Src/main.o ./Src/main.su ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su

.PHONY: clean-Src

