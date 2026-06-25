/**
  * @file rear_service.c
  * @brief Service routine shared by the rear (LB/RB) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "rear_service.h"
#include "app.h"
#include "can_driver.h"
#include "error_handler.h"
#include "adc_driver.h"
#include "main.h"
#include <stdbool.h>

static float directionVoltage;
static float positionCirclesVoltage;
static float safeStateVoltage;
static float wholePositionVoltage;
static float sidePositionVoltage;
static float middleStopVoltage;


extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CAN_HandleTypeDef hcan;
extern ADC_ChannelsConfigTypeDefs cadc1;
extern ADC_ChannelsConfigTypeDefs cadc2;

static uint32_t directionTimer = 0;
static uint32_t safeStateTimer = 0;
static bool turnStatus = false;
static EH_HandleTypeDef errorHandler;
static struct CAN_scheduledMsgList canBuffer;
static struct CAN_scheduledMsg statusFrame;

static void ReadBoardStatus(uint8_t *data, void *context)
{
	uint8_t status = 0;
	status |= (HAL_GPIO_ReadPin(REVERSE_PORT, REVERSE_PIN) == GPIO_PIN_SET ? 1 : 0) << 0;
	status |= (HAL_GPIO_ReadPin(SIDE_STOP_PORT, SIDE_STOP_PIN) == GPIO_PIN_SET ? 1 : 0) << 1;
	status |= (HAL_GPIO_ReadPin(DIRECTION_PORT, DIRECTION_PIN) == GPIO_PIN_SET ? 1 : 0) << 2;
	status |= (HAL_GPIO_ReadPin(POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN) == GPIO_PIN_SET ? 1 : 0) << 3;
	status |= (HAL_GPIO_ReadPin(SAFE_STATE_PORT, SAFE_STATE_PIN) == GPIO_PIN_SET ? 1 : 0) << 4;
	status |= (HAL_GPIO_ReadPin(WHOLE_POSITION_PORT, WHOLE_POSITION_PIN) == GPIO_PIN_SET ? 1 : 0) << 5;
	status |= (HAL_GPIO_ReadPin(SIDE_POSITION_PORT, SIDE_POSITION_PIN) == GPIO_PIN_SET ? 1 : 0) << 6;
	if (data)
	{
		*data = status;
	}
}

static void CheckChannel(float voltage, float overload, uint16_t openLoadCode,
                         GPIO_TypeDef *port, uint16_t pin)
{
	if (voltage < VOLTAGE_LOW && HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
	{
		EH_report(&errorHandler, openLoadCode, ERROR_SEVERITY_ERROR);
	}
	if (voltage > overload)
	{
		HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
		EH_stop(&errorHandler, 0xDEAD, ERROR_SEVERITY_ERROR);
	}
}

static void CheckForMalfunction(bool left)
{
	CheckChannel(safeStateVoltage,       SAFE_STATE_OVERLOAD,       0x01, SAFE_STATE_PORT,       SAFE_STATE_PIN);
	if (left)
	{
		CheckChannel(wholePositionVoltage, WHOLE_POSITION_OVERLOAD, 0x02, WHOLE_POSITION_PORT, WHOLE_POSITION_PIN);
	}
	else
	{
		CheckChannel(middleStopVoltage, MIDDLE_STOP_OVERLOAD,       0x02, MIDDLE_STOP_PORT,    MIDDLE_STOP_PIN);
	}
	CheckChannel(positionCirclesVoltage, POSITION_CIRCLES_OVERLOAD, 0x03, POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN);
	CheckChannel(directionVoltage,       DIRECTION_OVERLOAD,        0x04, DIRECTION_PORT,        DIRECTION_PIN);
}

static void MakeAdcReadings(bool left)
{
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_1, &sidePositionVoltage);

	if(left){
		ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_2, &wholePositionVoltage);
	}
	else{
		ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_2, &middleStopVoltage);
	}
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_3, &positionCirclesVoltage);
	ADC_Get_PinVoltage(&hadc1, &cadc1, ADC_CHANNEL_4, &directionVoltage);
	ADC_Get_PinVoltage(&hadc2, &cadc2, ADC_CHANNEL_1, &safeStateVoltage);
}

static void ServiceLights(bool left)
{
	const uint32_t now = HAL_GetTick();
	if ((leftTurnStatus && left) || emergencyStatus || (rightTurnStatus && !left))
	{
		if (turnStatus)
		{
			if ((now - directionTimer) >= DIRECTION_HIGH)
			{
				HAL_GPIO_WritePin(DIRECTION_PORT, DIRECTION_PIN, GPIO_PIN_RESET);
				if (positionStatus)
				{
					HAL_GPIO_WritePin(POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN, GPIO_PIN_SET);
				}
				turnStatus = false;
				directionTimer = now;
			}
		}
		else
		{
			if ((now - directionTimer) >= (DIRECTION_PERIOD - DIRECTION_HIGH))
			{
				HAL_GPIO_WritePin(POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(DIRECTION_PORT, DIRECTION_PIN, GPIO_PIN_SET);
				turnStatus = true;
				directionTimer = now;
			}
		}
	}
	else if (indicatorChangeFlag)
	{
		indicatorChangeFlag = false;
		HAL_GPIO_WritePin(DIRECTION_PORT, DIRECTION_PIN, GPIO_PIN_RESET);
		turnStatus = false;
		if (positionStatus)
		{
			HAL_GPIO_WritePin(POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN, GPIO_PIN_SET);
		}
	}

	if (positionChangeFlag)
	{
		const GPIO_PinState s = positionStatus ? GPIO_PIN_SET : GPIO_PIN_RESET;
		HAL_GPIO_WritePin(SIDE_POSITION_PORT, SIDE_POSITION_PIN, s);
		if (left)
		{
			HAL_GPIO_WritePin(WHOLE_POSITION_PORT, WHOLE_POSITION_PIN, s);
		}
		if (!positionStatus || !turnStatus)
		{
			HAL_GPIO_WritePin(POSITION_CIRCLES_PORT, POSITION_CIRCLES_PIN, s);
		}
		positionChangeFlag = false;
	}
	if (brakeChangeFlag)
	{
		const GPIO_PinState s = brakeStatus ? GPIO_PIN_SET : GPIO_PIN_RESET;
		HAL_GPIO_WritePin(SIDE_STOP_PORT, SIDE_STOP_PIN, s);
		if (!left)
		{
			HAL_GPIO_WritePin(MIDDLE_STOP_PORT, MIDDLE_STOP_PIN, s);
		}
		brakeChangeFlag = false;
	}
	if (reverseChangeFlag)
	{
		HAL_GPIO_WritePin(REVERSE_PORT, REVERSE_PIN, reverseStatus ? GPIO_PIN_SET : GPIO_PIN_RESET);
		reverseChangeFlag = false;
	}
}

static void CheckForSafeState(bool left)
{
	if (safeStateStatus)
	{
		while(true)
		{
			ServiceLights(left);
			if (safeStateStatus)
			{
				safeStateTimer = HAL_GetTick();
				safeStateStatus = false;
				HAL_GPIO_WritePin(SAFE_STATE_PORT, SAFE_STATE_PIN, GPIO_PIN_SET);
			}
			if ((HAL_GetTick() - safeStateTimer > SAFE_STATE_DURATION_MS) && !safeStateStatus)
			{
				HAL_GPIO_WritePin(SAFE_STATE_PORT, SAFE_STATE_PIN, GPIO_PIN_RESET);
				break;
			}
		}
	}
}

static void InitService(bool left)
{
	if(left){
		EH_init(&errorHandler, &hcan, NODE_FRAME_ID_LB, &canBuffer);
		statusFrame.header.DLC = STATUS_FRAME_DLC;
		statusFrame.header.StdId = STATUS_FRAME_ID_LB;
		statusFrame.periodMs = STATUS_FRAME_PERIOD_MS;
		statusFrame.getData = &ReadBoardStatus;
		statusFrame.lastTick = 0;
		CAN_AddScheduledMsg(&statusFrame, &canBuffer);
	}
	else{
		EH_init(&errorHandler, &hcan, NODE_FRAME_ID_RB, &canBuffer);
		statusFrame.header.DLC = STATUS_FRAME_DLC;
		statusFrame.header.StdId = STATUS_FRAME_ID_RB;
		statusFrame.periodMs = STATUS_FRAME_PERIOD_MS;
		statusFrame.getData = &ReadBoardStatus;
		statusFrame.lastTick = 0;
		CAN_AddScheduledMsg(&statusFrame, &canBuffer);
	}
}

void RearService(bool left)
{
	InitService(left);
	while (true)
	{
		CAN_HandleScheduled(&hcan, &canBuffer);
		APP_InterpretFrames();
		MakeAdcReadings(left);
		//CheckForMalfunction(left);
		ServiceLights(left);
		CheckForSafeState(left);
	}
}
