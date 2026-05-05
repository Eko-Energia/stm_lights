################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Eko_Drivers/Src/adc_driver.c \
../App/Eko_Drivers/Src/adc_utils.c \
../App/Eko_Drivers/Src/can_driver.c \
../App/Eko_Drivers/Src/dma_driver.c \
../App/Eko_Drivers/Src/error_handler.c \
../App/Eko_Drivers/Src/pwm_driver.c 

OBJS += \
./App/Eko_Drivers/Src/adc_driver.o \
./App/Eko_Drivers/Src/adc_utils.o \
./App/Eko_Drivers/Src/can_driver.o \
./App/Eko_Drivers/Src/dma_driver.o \
./App/Eko_Drivers/Src/error_handler.o \
./App/Eko_Drivers/Src/pwm_driver.o 

C_DEPS += \
./App/Eko_Drivers/Src/adc_driver.d \
./App/Eko_Drivers/Src/adc_utils.d \
./App/Eko_Drivers/Src/can_driver.d \
./App/Eko_Drivers/Src/dma_driver.d \
./App/Eko_Drivers/Src/error_handler.d \
./App/Eko_Drivers/Src/pwm_driver.d 


# Each subdirectory must supply rules for building sources it contributes
App/Eko_Drivers/Src/%.o App/Eko_Drivers/Src/%.su App/Eko_Drivers/Src/%.cyclo: ../App/Eko_Drivers/Src/%.c App/Eko_Drivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303x8 -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/lukas/STM32CubeIDE/ligths_driver/App/Inc" -I"C:/Users/lukas/STM32CubeIDE/ligths_driver/App/Eko_Drivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Eko_Drivers-2f-Src

clean-App-2f-Eko_Drivers-2f-Src:
	-$(RM) ./App/Eko_Drivers/Src/adc_driver.cyclo ./App/Eko_Drivers/Src/adc_driver.d ./App/Eko_Drivers/Src/adc_driver.o ./App/Eko_Drivers/Src/adc_driver.su ./App/Eko_Drivers/Src/adc_utils.cyclo ./App/Eko_Drivers/Src/adc_utils.d ./App/Eko_Drivers/Src/adc_utils.o ./App/Eko_Drivers/Src/adc_utils.su ./App/Eko_Drivers/Src/can_driver.cyclo ./App/Eko_Drivers/Src/can_driver.d ./App/Eko_Drivers/Src/can_driver.o ./App/Eko_Drivers/Src/can_driver.su ./App/Eko_Drivers/Src/dma_driver.cyclo ./App/Eko_Drivers/Src/dma_driver.d ./App/Eko_Drivers/Src/dma_driver.o ./App/Eko_Drivers/Src/dma_driver.su ./App/Eko_Drivers/Src/error_handler.cyclo ./App/Eko_Drivers/Src/error_handler.d ./App/Eko_Drivers/Src/error_handler.o ./App/Eko_Drivers/Src/error_handler.su ./App/Eko_Drivers/Src/pwm_driver.cyclo ./App/Eko_Drivers/Src/pwm_driver.d ./App/Eko_Drivers/Src/pwm_driver.o ./App/Eko_Drivers/Src/pwm_driver.su

.PHONY: clean-App-2f-Eko_Drivers-2f-Src

