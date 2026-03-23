################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ds1804.c \
../Core/Src/freertos.c \
../Core/Src/input.c \
../Core/Src/jsmn.c \
../Core/Src/json_cmd.c \
../Core/Src/main.c \
../Core/Src/max3485.c \
../Core/Src/monitors_config.c \
../Core/Src/output.c \
../Core/Src/pcf8575.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c 

OBJS += \
./Core/Src/ds1804.o \
./Core/Src/freertos.o \
./Core/Src/input.o \
./Core/Src/jsmn.o \
./Core/Src/json_cmd.o \
./Core/Src/main.o \
./Core/Src/max3485.o \
./Core/Src/monitors_config.o \
./Core/Src/output.o \
./Core/Src/pcf8575.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o 

C_DEPS += \
./Core/Src/ds1804.d \
./Core/Src/freertos.d \
./Core/Src/input.d \
./Core/Src/jsmn.d \
./Core/Src/json_cmd.d \
./Core/Src/main.d \
./Core/Src/max3485.d \
./Core/Src/monitors_config.d \
./Core/Src/output.d \
./Core/Src/pcf8575.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L431xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/ds1804.cyclo ./Core/Src/ds1804.d ./Core/Src/ds1804.o ./Core/Src/ds1804.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/input.cyclo ./Core/Src/input.d ./Core/Src/input.o ./Core/Src/input.su ./Core/Src/jsmn.cyclo ./Core/Src/jsmn.d ./Core/Src/jsmn.o ./Core/Src/jsmn.su ./Core/Src/json_cmd.cyclo ./Core/Src/json_cmd.d ./Core/Src/json_cmd.o ./Core/Src/json_cmd.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/max3485.cyclo ./Core/Src/max3485.d ./Core/Src/max3485.o ./Core/Src/max3485.su ./Core/Src/monitors_config.cyclo ./Core/Src/monitors_config.d ./Core/Src/monitors_config.o ./Core/Src/monitors_config.su ./Core/Src/output.cyclo ./Core/Src/output.d ./Core/Src/output.o ./Core/Src/output.su ./Core/Src/pcf8575.cyclo ./Core/Src/pcf8575.d ./Core/Src/pcf8575.o ./Core/Src/pcf8575.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su

.PHONY: clean-Core-2f-Src

