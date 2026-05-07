#include "rear_service.h"
#include "app.h"
#include "main.h"
#include "can_driver.h"
#include "pwm_driver.h"
#include "error_handler.h"
#include "global_variables.h"
#include "adc_driver.h"
#include "tim.h"
#include <stdbool.h>

static float directionVoltage,
	  positionCirclesVoltage,
	  safeStateVoltage,
	  middleStopVoltage,
	  sidePositionVoltage;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim1;
extern struct PWM_Out_signal PWM;
extern ADC_ChannelsConfigTypeDefs cadc1;
extern ADC_ChannelsConfigTypeDefs cadc2;
static uint32_t directionTimer = 0;
static uint32_t safeStateTimer = 0;
static bool turnStatus = false;
static EH_HandleTypeDef errorHandler;
static struct CAN_scheduledMsgList CAN_buffer;
static struct CAN_scheduledMsg statusFrame;


void getRBStatus(uint8_t *data, void *context) {
    uint8_t status = 0;
    status |= (HAL_GPIO_ReadPin(reverse_port, reverse_pin) == GPIO_PIN_SET ? 1 : 0) << 0;
    status |= (HAL_GPIO_ReadPin(sideStop_port, sideStop_pin) == GPIO_PIN_SET ? 1 : 0) << 1;
    status |= (HAL_GPIO_ReadPin(direction_port, direction_pin) == GPIO_PIN_SET ? 1 : 0) << 2;
    uint32_t pulse = __HAL_TIM_GET_COMPARE(&positionCircles_TIM, positionCircles_Channel);
    status |= (pulse > 0 ? 1 : 0) << 3;
    status |= (HAL_GPIO_ReadPin(safeState_port, safeState_pin) == GPIO_PIN_SET ? 1 : 0) << 4;
    status |= (HAL_GPIO_ReadPin(middleStop_port, middleStop_pin) == GPIO_PIN_SET ? 1 : 0) << 5;
    status |= (HAL_GPIO_ReadPin(sidePosition_port, sidePosition_pin) == GPIO_PIN_SET ? 1 : 0) << 6;
    if (data) {
        *data = status;
    }
}


static void ADC_makeReadings(){
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_1, &sidePositionVoltage);
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_2, &middleStopVoltage);
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_3, &positionCirclesVoltage);
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_4, &directionVoltage);
	ADC_Get_PinVoltage(&hadc2, &cadc2, ADC_CHANNEL_1, &safeStateVoltage);
}

static void checkForMalfunction(){
	if((sidePositionVoltage < voltageLow && getPositionStatus()) || sidePositionVoltage > sidePositionOverload ){
		// error
	}
	if((middleStopVoltage < voltageLow && getBrakeStatus()) || middleStopVoltage > middleStopOverload ){
		// error
	}
	if((positionCirclesVoltage < voltageLow && getPositionStatus()) || positionCirclesVoltage > positionCirclesOverload ){
		// error
	}
	if((directionVoltage < voltageLow && getRightTurnStatus()) || directionVoltage > directionOverload ){
		// error
	}

}

static void serviceLights(){
	uint32_t now = HAL_GetTick();
	if(getRightTurnStatus() || getEmergencyStatus() ){
		if(turnStatus){
			if((now - directionTimer) >= directionHigh){
				HAL_GPIO_WritePin(direction_port, direction_pin, GPIO_PIN_RESET);
				turnStatus = false;
				directionTimer = now;
			}
		}
		else{
			if((now - directionTimer) >= (directionPeriod - directionHigh)){
				HAL_GPIO_WritePin(direction_port, direction_pin, GPIO_PIN_SET);
				turnStatus = true;
				directionTimer = now;
			}
		}
	}
	else{
		HAL_GPIO_WritePin(direction_port, direction_pin, GPIO_PIN_RESET);
		turnStatus = false;
	}

	if(getPositionChangeFlag()){
		if(getPositionStatus()){
			HAL_GPIO_WritePin(sidePosition_port, sidePosition_pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(safeState_port, safeState_pin, GPIO_PIN_RESET);
			PWM_Out_setDuty(&PWM,95.0f); // Position Circles
			setPositionChangeFlagFalse();
		}
		else{
			HAL_GPIO_WritePin(sidePosition_port, sidePosition_pin, GPIO_PIN_RESET);
			PWM_Out_setDuty(&PWM,0.0f); // Position circles
			setPositionChangeFlagFalse();
		}
	}
	if(getBrakeChangeFlag()){
		if(getBrakeStatus()){
			HAL_GPIO_WritePin(sideStop_port, sideStop_pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(middleStop_port, middleStop_pin, GPIO_PIN_SET);
			setBrakeChangeFlagFalse();
		}
		else{
			HAL_GPIO_WritePin(sideStop_port, sideStop_pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(middleStop_port, middleStop_pin, GPIO_PIN_RESET);
			setBrakeChangeFlagFalse();
		}
	}


}

static void checkForSafeState(){
	if(getSafeStateStatus()){
		while(true){
			serviceLights();
			if(getSafeStateStatus()){
				safeStateTimer = HAL_GetTick();
				setSafeStateStatusFalse();
				PWM_Out_setDuty(&PWM,0.0f); // Position circles
				HAL_GPIO_WritePin(safeState_port, safeState_pin, GPIO_PIN_SET);
			}
			if((HAL_GetTick() - safeStateTimer > 1000) && !getSafeStateStatus()){
				HAL_GPIO_WritePin(safeState_port, safeState_pin, GPIO_PIN_RESET);
				break;
			}
		}
	}
}

void RB_service(){
	EH_init(&errorHandler, &hcan,nodeFrameIdRB,&CAN_buffer);
	statusFrame.header.DLC = 1;
	statusFrame.header.StdId = statusFrameIdRB;
	statusFrame.periodMs = 1000;
	statusFrame.getData = &getRBStatus;
	statusFrame.lastTick = 0;
	CAN_AddScheduledMsg(&statusFrame,&CAN_buffer);
	while(true){
		CAN_HandleScheduled(&hcan,&CAN_buffer);
		ADC_makeReadings();
		checkForMalfunction();
		serviceLights();
		checkForSafeState();
	}
}
