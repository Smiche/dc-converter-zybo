################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/FreeRTOS_CLI.c \
../src/UARTCommandConsole.c \
../src/led.c \
../src/main.c \
../src/serial.c 

OBJS += \
./src/FreeRTOS_CLI.o \
./src/UARTCommandConsole.o \
./src/led.o \
./src/main.o \
./src/serial.o 

C_DEPS += \
./src/FreeRTOS_CLI.d \
./src/UARTCommandConsole.d \
./src/led.d \
./src/main.d \
./src/serial.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../SwitchingPSU_FreeRTOS_bsp/ps7_cortexa9_0/include -I"D:\SoftwareDev\dc-converter-zybo\zybo_platform" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


