#include "rear_service.h"
#include "app.h"
#include "main.h"
#include "can_driver.h"

void getLBStatus(uint8_t *data, void *context) {
    uint8_t status = 0;
    status |= (HAL_GPIO_ReadPin(reverse_port, reverse_pin) == GPIO_PIN_SET ? 1 : 0) << 0;
    status |= (HAL_GPIO_ReadPin(sideStop_port, sideStop_pin) == GPIO_PIN_SET ? 1 : 0) << 1;
    status |= (HAL_GPIO_ReadPin(direction_port, direction_pin) == GPIO_PIN_SET ? 1 : 0) << 2;
    uint32_t pulse = __HAL_TIM_GET_COMPARE(&positionCircles_TIM, positionCircles_Channel);
    status |= (pulse > 0 ? 1 : 0) << 3;
    status |= (HAL_GPIO_ReadPin(safeState_port, safeState_pin) == GPIO_PIN_SET ? 1 : 0) << 4;
    status |= (HAL_GPIO_ReadPin(wholePosition_port, wholePosition_pin) == GPIO_PIN_SET ? 1 : 0) << 5;
    status |= (HAL_GPIO_ReadPin(sidePosition_port, sidePosition_pin) == GPIO_PIN_SET ? 1 : 0) << 6;
    if (data) {
        *data = status;
    }
}

void LB_service(){

}
