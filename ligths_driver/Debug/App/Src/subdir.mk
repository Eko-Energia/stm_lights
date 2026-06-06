################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/LF_service.c \
../App/Src/RF_service.c \
../App/Src/app.c \
../App/Src/board_selector.c \
../App/Src/rear_service.c 

OBJS += \
./App/Src/LF_service.o \
./App/Src/RF_service.o \
./App/Src/app.o \
./App/Src/board_selector.o \
./App/Src/rear_service.o 

C_DEPS += \
./App/Src/LF_service.d \
./App/Src/RF_service.d \
./App/Src/app.d \
./App/Src/board_selector.d \
./App/Src/rear_service.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.c App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303x8 -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/lukas/STM32CubeIDE/ligths_driver/App/Inc" -I"C:/Users/lukas/STM32CubeIDE/ligths_driver/App/Eko_Drivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src

clean-App-2f-Src:
	-$(RM) ./App/Src/LF_service.cyclo ./App/Src/LF_service.d ./App/Src/LF_service.o ./App/Src/LF_service.su ./App/Src/RF_service.cyclo ./App/Src/RF_service.d ./App/Src/RF_service.o ./App/Src/RF_service.su ./App/Src/app.cyclo ./App/Src/app.d ./App/Src/app.o ./App/Src/app.su ./App/Src/board_selector.cyclo ./App/Src/board_selector.d ./App/Src/board_selector.o ./App/Src/board_selector.su ./App/Src/rear_service.cyclo ./App/Src/rear_service.d ./App/Src/rear_service.o ./App/Src/rear_service.su

.PHONY: clean-App-2f-Src

