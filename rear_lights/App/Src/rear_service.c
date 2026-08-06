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

extern CAN_HandleTypeDef hcan;

// Build target: 1U = left-back board, 0U = right-back board.
const uint8_t boardIsLeft = 1U;

static EH_HandleTypeDef errorHandler;
static struct CAN_scheduledMsgList canBuffer;
static struct CAN_scheduledMsg statusFrame;
// HAL tick captured when safe state was last entered or refreshed.
static uint32_t safeStateTimer = 0;

uint8_t safeStateActive = 0U;

// One context per physical output. long_light is wired to a different lamp on
// each board: whole-position on LB, middle-stop on RB.
struct LED ledReverse         = { LED_OFF, reverse_GPIO_Port,          reverse_Pin };
struct LED ledSideStop        = { LED_OFF, side_stop_GPIO_Port,        side_stop_Pin };
struct LED ledDirection       = { LED_OFF, direction_GPIO_Port,        direction_Pin };
struct LED ledPositionCircles = { LED_OFF, position_circles_GPIO_Port, position_circles_Pin };
struct LED ledSafeState       = { LED_OFF, safe_state_GPIO_Port,       safe_state_Pin };
struct LED ledSidePosition    = { LED_OFF, side_position_GPIO_Port,    side_position_Pin };
struct LED ledLongLight       = { LED_OFF, long_light_GPIO_Port,       long_light_Pin };

/**
  * @brief Packs the GPIO level of every light output into one status byte.
  *
  * Bit order follows LightsR*_Status in CAN_DB.dbc. Levels are read back from
  * the pins rather than from the LED states, so the frame reports what the
  * hardware is actually doing.
  *
  * Registered as the getData callback of the periodic status frame.
  *
  * @param data      Destination for the packed status byte (one bit per output).
  * @param context   Unused scheduler callback context.
  */
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

/**
  * @brief Applies deferred brake/reverse changes and ticks every LED state machine.
  *
  * The event-driven outputs (direction, position, circles) are written directly
  * by APP_InterpretFrames; only brake and reverse go through change flags here.
  */
static void ServiceLights(void)
{
	if (brakeChangeFlag)
	{
		brakeChangeFlag = 0U;
		const LED_STATE_e s = brakeStatus ? LED_ON : LED_OFF;
		LED_ChangeState(&ledSideStop, s);
		if (!boardIsLeft)
		{
			LED_ChangeState(&ledLongLight, s);
		}
	}

	if (reverseChangeFlag)
	{
		reverseChangeFlag = 0U;
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

/**
  * @brief Non-blocking safe-state handling, called every main-loop pass.
  *
  * A SafeState frame enters safe state (all lights off, emergency blink,
  * safe-state pin high) or refreshes the hold timer if already active.
  * After SAFE_STATE_DURATION_MS without a re-broadcast the pin is released.
  */
static void HandleSafeState(void)
{
	if (safeStateDataCheck)
	{
		safeStateDataCheck = 0U;
		safeStateTimer = HAL_GetTick();
		if (!safeStateActive)
		{
			safeStateActive = 1U;
			brakeStatus   = 0U;
			reverseStatus = 0U;
			brakeChangeFlag   = 1U;
			reverseChangeFlag = 1U;

			LED_ChangeState(&ledSafeState,       LED_ON);
			LED_ChangeState(&ledDirection,       LED_BLINK);
			LED_ChangeState(&ledSidePosition,    LED_OFF);
			if (boardIsLeft)
			{
				LED_ChangeState(&ledLongLight, LED_OFF);
			}
			LED_ChangeState(&ledPositionCircles, LED_OFF);
		}
	}
	else if (safeStateActive && (HAL_GetTick() - safeStateTimer > SAFE_STATE_DURATION_MS))
	{
		safeStateActive = 0U;
		LED_ChangeState(&ledSafeState, LED_OFF);
		LED_ChangeState(&ledDirection, LED_OFF);
	}
}

/**
  * @brief Registers the error handler and this board's periodic status frame.
  */
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

/**
  * @brief Rear-board service entry point; initializes CAN and runs the main
  *        loop (does not return).
  */
void RearService(void)
{
	struct LED LED_GREEN = {LED_OFF, LED_GREEN_GPIO_Port, LED_GREEN_Pin};
	struct LED LED_RED = {LED_OFF, LED_RED_GPIO_Port, LED_RED_Pin};

	CAN_Init(&hcan);
	SetCanFilters();
	InitService();
	LED_ChangeState(&LED_GREEN, LED_BLINK);
	while (1U)
	{
		CAN_HandleScheduled(&hcan, &canBuffer);
		HandleSafeState();
		APP_InterpretFrames();
		ServiceLights();

		LED_Handle(&LED_GREEN);
		LED_Handle(&LED_RED);
	}


}

/**
  * @brief Overrides HAL's weak tick hook so the LED driver's syncTick advances
  *        every millisecond.
  */
void HAL_IncTick(void)
{
	uwTick += uwTickFreq;
	LED_IncSyncTick();
}
