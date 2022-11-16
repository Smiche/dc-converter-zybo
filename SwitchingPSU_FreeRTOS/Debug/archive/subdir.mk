################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../archive/main2.c 

OBJS += \
./archive/main2.o 

C_DEPS += \
./archive/main2.d 


# Each subdirectory must supply rules for building sources it contributes
archive/%.o: ../archive/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../SwitchingPSU_FreeRTOS_bsp/ps7_cortexa9_0/include -I"C:\Users\smich\workspace\zybo_platform" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


