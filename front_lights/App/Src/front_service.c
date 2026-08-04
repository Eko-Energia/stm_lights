/**
  * @file front_service.c
  * @brief Service routine shared by the front (FL/FR) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "front_service.h"
#include "can_app.h"
#include "can_driver.h"
#include "error_handler.h"
#include "led_driver.h"
#include "main.h"

extern CAN_HandleTypeDef hcan;

// Build target: 1U = front-left board, 0U = front-right board.
const uint8_t boardIsLeft = 1U;

static EH_HandleTypeDef errorHandler;
static struct CAN_scheduledMsgList canBuffer;
static struct CAN_scheduledMsg statusFrame;
// HAL tick captured when safe state was last entered or refreshed.
static uint32_t safeStateTimer = 0;

uint8_t safeStateActive = 0U;

// One context per physical output; both boards drive the same set of lamps.
struct LED ledHighBeam      = { LED_OFF, high_beam_GPIO_Port,      high_beam_Pin };
struct LED ledLowBeam       = { LED_OFF, low_beam_GPIO_Port,       low_beam_Pin };
struct LED ledIndicator     = { LED_OFF, indicator_GPIO_Port,      indicator_Pin };
struct LED ledSideIndicator = { LED_OFF, side_indicator_GPIO_Port, side_indicator_Pin };
struct LED ledDayLight1     = { LED_OFF, day_light1_GPIO_Port,     day_light1_Pin };
struct LED ledDayLight2     = { LED_OFF, day_light2_GPIO_Port,     day_light2_Pin };
struct LED ledPosition      = { LED_OFF, position_GPIO_Port,       position_Pin };

/**
  * @brief Packs the GPIO level of every light output into one status byte.
  *
  * Bit order follows LightsF*_Status in CAN_DB.dbc:
  *   0 smallLens1, 1 smallLens2, 2 direction, 3 day,
  *   4 sideDirection, 5 headlights, 6 highBeams.
  *
  * Levels are read back from the pins rather than from the LED states, so the
  * frame reports what the hardware is actually doing.
  *
  * Registered as the getData callback of the periodic status frame.
  *
  * @param data      Destination for the packed status byte (one bit per output).
  * @param context   Unused scheduler callback context.
  */
static void ReadBoardStatus(uint8_t *data, void *context)
{
	uint8_t status = 0;
	status |= (HAL_GPIO_ReadPin(day_light1_GPIO_Port,     day_light1_Pin)     == GPIO_PIN_SET ? 1 : 0) << 0;
	status |= (HAL_GPIO_ReadPin(day_light2_GPIO_Port,     day_light2_Pin)     == GPIO_PIN_SET ? 1 : 0) << 1;
	status |= (HAL_GPIO_ReadPin(indicator_GPIO_Port,      indicator_Pin)      == GPIO_PIN_SET ? 1 : 0) << 2;
	status |= (HAL_GPIO_ReadPin(position_GPIO_Port,       position_Pin)       == GPIO_PIN_SET ? 1 : 0) << 3;
	status |= (HAL_GPIO_ReadPin(side_indicator_GPIO_Port, side_indicator_Pin) == GPIO_PIN_SET ? 1 : 0) << 4;
	status |= (HAL_GPIO_ReadPin(low_beam_GPIO_Port,       low_beam_Pin)       == GPIO_PIN_SET ? 1 : 0) << 5;
	status |= (HAL_GPIO_ReadPin(high_beam_GPIO_Port,      high_beam_Pin)      == GPIO_PIN_SET ? 1 : 0) << 6;
	if (data)
	{
		*data = status;
	}
}

/**
  * @brief Ticks every LED state machine.
  *
  * All front outputs are event-driven and written directly by
  * APP_InterpretFrames; this only advances the blinking ones.
  */
static void ServiceLights(void)
{
	LED_Handle(&ledHighBeam);
	LED_Handle(&ledLowBeam);
	LED_Handle(&ledIndicator);
	LED_Handle(&ledSideIndicator);
	LED_Handle(&ledDayLight1);
	LED_Handle(&ledDayLight2);
	LED_Handle(&ledPosition);
}

/**
  * @brief Non-blocking safe-state handling, called every main-loop pass.
  *
  * A SafeState frame enters safe state (all lamps off, emergency blink on the
  * indicators) or refreshes the hold timer if already active. After
  * SAFE_STATE_DURATION_MS without a re-broadcast the hold is released.
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

			LED_ChangeState(&ledIndicator,     LED_BLINK);
			LED_ChangeState(&ledSideIndicator, LED_BLINK);
			LED_ChangeState(&ledHighBeam,      LED_OFF);
			LED_ChangeState(&ledLowBeam,       LED_OFF);
			LED_ChangeState(&ledDayLight1,     LED_OFF);
			LED_ChangeState(&ledDayLight2,     LED_OFF);
			LED_ChangeState(&ledPosition,      LED_OFF);
		}
	}
	else if (safeStateActive && (HAL_GetTick() - safeStateTimer > SAFE_STATE_DURATION_MS))
	{
		safeStateActive = 0U;
	}
}

/**
  * @brief Registers the error handler and this board's periodic status frame.
  */
static void InitService(void)
{
	const uint16_t nodeId    = boardIsLeft ? NODE_FRAME_ID_FL   : NODE_FRAME_ID_FR;
	const uint32_t statusId  = boardIsLeft ? STATUS_FRAME_ID_FL : STATUS_FRAME_ID_FR;

	EH_init(&errorHandler, &hcan, nodeId, &canBuffer);
	statusFrame.header.DLC   = STATUS_FRAME_DLC;
	statusFrame.header.StdId = statusId;
	statusFrame.periodMs     = STATUS_FRAME_PERIOD_MS;
	statusFrame.getData      = &ReadBoardStatus;
	statusFrame.lastTick     = 0;
	CAN_AddScheduledMsg(&statusFrame, &canBuffer);
}

/**
  * @brief Front-board service entry point; initializes CAN and runs the main
  *        loop (does not return).
  */
void FrontService(void)
{
	CAN_Init(&hcan);
	SetCanFilters();
	InitService();
	while (1U)
	{
		CAN_HandleScheduled(&hcan, &canBuffer);
		HandleSafeState();
		APP_InterpretFrames();
		ServiceLights();
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
