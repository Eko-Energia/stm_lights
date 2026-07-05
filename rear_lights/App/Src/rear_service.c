/**
  * @file rear_service.c
  * @brief Service routine shared by the rear (LB/RB) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "rear_service.h"
#include "can_app.h"
#include "can_driver.h"
#include "error_handler.h"
#include "led_driver.h"
#include "main.h"
#include <stdbool.h>

extern CAN_HandleTypeDef hcan;

const bool boardIsLeft = true;

static EH_HandleTypeDef errorHandler;
static struct CAN_scheduledMsgList canBuffer;
static struct CAN_scheduledMsg statusFrame;
static uint32_t safeStateTimer = 0;

struct LED ledReverse         = { LED_OFF, reverse_GPIO_Port,          reverse_Pin };
struct LED ledSideStop        = { LED_OFF, side_stop_GPIO_Port,        side_stop_Pin };
struct LED ledDirection       = { LED_OFF, direction_GPIO_Port,        direction_Pin };
struct LED ledPositionCircles = { LED_OFF, position_circles_GPIO_Port, position_circles_Pin };
struct LED ledSafeState       = { LED_OFF, safe_state_GPIO_Port,       safe_state_Pin };
struct LED ledSidePosition    = { LED_OFF, side_position_GPIO_Port,    side_position_Pin };
struct LED ledLongLight       = { LED_OFF, long_light_GPIO_Port,       long_light_Pin };

static void ReadBoardStatus(uint8_t *data, void *context)
{
	uint8_t status = 0;
	status |= (HAL_GPIO_ReadPin(reverse_GPIO_Port,          reverse_Pin)          == GPIO_PIN_SET ? 1 : 0) << 0;
	status |= (HAL_GPIO_ReadPin(side_stop_GPIO_Port,        side_stop_Pin)        == GPIO_PIN_SET ? 1 : 0) << 1;
	status |= (HAL_GPIO_ReadPin(direction_GPIO_Port,        direction_Pin)        == GPIO_PIN_SET ? 1 : 0) << 2;
	status |= (HAL_GPIO_ReadPin(position_circles_GPIO_Port, position_circles_Pin) == GPIO_PIN_SET ? 1 : 0) << 3;
	status |= (HAL_GPIO_ReadPin(safe_state_GPIO_Port,       safe_state_Pin)       == GPIO_PIN_SET ? 1 : 0) << 4;
	status |= (HAL_GPIO_ReadPin(long_light_GPIO_Port,       long_light_Pin)       == GPIO_PIN_SET ? 1 : 0) << 5;
	status |= (HAL_GPIO_ReadPin(side_position_GPIO_Port,    side_position_Pin)    == GPIO_PIN_SET ? 1 : 0) << 6;
	if (data)
	{
		*data = status;
	}
}

// Event-driven LEDs (direction / position / circles) are written directly in
// APP_InterpretFrames, so ServiceLights only handles the deferred brake/reverse
// and keeps the blinking outputs ticking via LED_Handle.
static void ServiceLights(void)
{
	if (brakeChangeFlag)
	{
		brakeChangeFlag = false;
		const LED_STATE_e s = brakeStatus ? LED_ON : LED_OFF;
		LED_ChangeState(&ledSideStop, s);
		if (!boardIsLeft)
		{
			LED_ChangeState(&ledLongLight, s);
		}
	}

	if (reverseChangeFlag)
	{
		reverseChangeFlag = false;
		LED_ChangeState(&ledReverse, reverseStatus ? LED_ON : LED_OFF);
	}

	LED_Handle(&ledReverse);
	LED_Handle(&ledSideStop);
	LED_Handle(&ledDirection);
	LED_Handle(&ledPositionCircles);
	LED_Handle(&ledSafeState);
	LED_Handle(&ledSidePosition);
	LED_Handle(&ledLongLight);
}

// During the safe-state hold, only the safe-state indicator and the emergency
// direction blink need to keep ticking. All other outputs are already off.
static void CheckForSafeState(void)
{
	if (safeStateStatus)
	{
		while (true)
		{
			LED_Handle(&ledSafeState);
			LED_Handle(&ledDirection);
			CAN_HandleScheduled(&hcan, &canBuffer);
			if (safeStateStatus)
			{
				safeStateTimer = HAL_GetTick();
				safeStateStatus = false;
				LED_ChangeState(&ledSafeState, LED_ON);
			}
			if ((HAL_GetTick() - safeStateTimer > SAFE_STATE_DURATION_MS) && !safeStateStatus)
			{
				LED_ChangeState(&ledSafeState, LED_OFF);
				break;
			}
		}
	}
}

static void InitService(void)
{
	const uint16_t nodeId    = boardIsLeft ? NODE_FRAME_ID_LB   : NODE_FRAME_ID_RB;
	const uint32_t statusId  = boardIsLeft ? STATUS_FRAME_ID_LB : STATUS_FRAME_ID_RB;

	EH_init(&errorHandler, &hcan, nodeId, &canBuffer);
	statusFrame.header.DLC   = STATUS_FRAME_DLC;
	statusFrame.header.StdId = statusId;
	statusFrame.periodMs     = STATUS_FRAME_PERIOD_MS;
	statusFrame.getData      = &ReadBoardStatus;
	statusFrame.lastTick     = 0;
	CAN_AddScheduledMsg(&statusFrame, &canBuffer);
}

void RearService(void)
{
	CAN_Init(&hcan);
	SetCanFilters();
	InitService();
	while (true)
	{
		CAN_HandleScheduled(&hcan, &canBuffer);
		APP_InterpretFrames();
		ServiceLights();
		CheckForSafeState();
	}
}
