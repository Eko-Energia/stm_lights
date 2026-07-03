/**
  * @file can_app.c
  * @brief Application-level CAN frame interpretation and acceptance-filter setup.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "can_app.h"
#include "rear_service.h"
#include "can_driver.h"
#include "led_driver.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

extern CAN_HandleTypeDef hcan;

static CAN_RxHeaderTypeDef CAN_currentMessageHeader;

static uint8_t canDashboardLightsData[CAN_MAX_DLC];
static uint8_t canPedalsJtnsWorksData[CAN_MAX_DLC];
static uint8_t canDashboardControlData[CAN_MAX_DLC];
static uint8_t canSafeStateData[CAN_MAX_DLC];

static volatile bool dashboardLightsDataCheck = false;
static volatile bool pedalsJtnsWorksDataCheck = false;
static volatile bool dashboardControlDataCheck = false;
static volatile bool safeStateDataCheck = false;

volatile bool brakeStatus = false;
volatile bool safeStateStatus = false;
volatile bool reverseStatus = false;

volatile bool brakeChangeFlag = false;
volatile bool reverseChangeFlag = false;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	uint8_t tempData[CAN_MAX_DLC];

	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_currentMessageHeader, tempData);

	switch (CAN_currentMessageHeader.StdId)
	{
		case DASHBOARD_LIGHTS_FRAME_ID:
			memcpy(canDashboardLightsData, tempData, CAN_MAX_DLC);
			dashboardLightsDataCheck = true;
			break;

		case PEDALS_JTNS_WORKS_FRAME_ID:
			memcpy(canPedalsJtnsWorksData, tempData, CAN_MAX_DLC);
			pedalsJtnsWorksDataCheck = true;
			break;

		case DASHBOARD_CONTROL_FRAME_ID:
			memcpy(canDashboardControlData, tempData, CAN_MAX_DLC);
			dashboardControlDataCheck = true;
			break;

		case SAFE_STATE_FRAME_ID:
			memcpy(canSafeStateData, tempData, CAN_MAX_DLC);
			safeStateDataCheck = true;
			break;
	}
}

void APP_InterpretFrames(void)
{
	if (dashboardLightsDataCheck)
	{
		dashboardLightsDataCheck = false;

		// Dashboard_Lights (994) byte 0 layout, per CAN_DB.dbc:
		//   bits 0-2 Headlights   (0 OFF, 1 AUTO, 2 DAY, 3 NIGHT, 4 HIGHBEAMS)
		//   bits 3-4 TurnSig_Left (0 OFF, 1 ONCE, 2 ON)
		//   bits 5-6 TurnSig_Right
		//   bit  7   Emergency
		const uint8_t byte        = canDashboardLightsData[0];
		const uint8_t headlights  = byte & 0x7U;
		const uint8_t leftSignal  = (byte >> 3) & 0x3U;
		const uint8_t rightSignal = (byte >> 5) & 0x3U;

		const bool leftTurn  = (leftSignal  != 0U);
		const bool rightTurn = (rightSignal != 0U);
		const bool emergency = ((byte >> 7) & 0x1U) != 0U;

		const bool blinkOn = (leftTurn && boardIsLeft)
		                     || emergency
		                     || (rightTurn && !boardIsLeft);
		LED_ChangeState(&ledDirection, blinkOn ? LED_BLINK : LED_OFF);

		bool applyPosition = true;
		bool wantPosition  = false;
		switch (headlights)
		{
			case 0U:
				wantPosition = false;
				break;
			case 2U:
			case 3U:
			case 4U:
				wantPosition = true;
				break;
			default:
				// AUTO (1) or reserved: leave position state untouched.
				applyPosition = false;
				break;
		}

		if (applyPosition)
		{
			const LED_STATE_e s = wantPosition ? LED_ON : LED_OFF;
			LED_ChangeState(&ledSidePosition, s);
			if (boardIsLeft)
			{
				LED_ChangeState(&ledLongLight, s);
			}
		}

		// Read position back from the LED itself so AUTO (which skips the block
		// above) still gets the right circles output when blinkOn changes.
		const bool positionOn = (ledSidePosition.state == LED_ON);
		LED_ChangeState(&ledPositionCircles,
		                blinkOn ? LED_OFF : (positionOn ? LED_ON : LED_OFF));
	}

	if (pedalsJtnsWorksDataCheck)
	{
		pedalsJtnsWorksDataCheck = false;

		const bool wantBrake = canPedalsJtnsWorksData[BREAKS_HALL_INTPOS] > BREAK_HALL_EPS;
		if (wantBrake != brakeStatus)
		{
			brakeStatus = wantBrake;
			brakeChangeFlag = true;
		}
	}

	if (dashboardControlDataCheck)
	{
		dashboardControlDataCheck = false;

		// PRND: bit 14, length 2 -> high 2 bits of byte 1.
		const uint8_t prnd = (canDashboardControlData[1] >> 6) & 0x3U;

		const bool wantReverse = (prnd == PRND_REVERSE_VALUE);
		if (wantReverse != reverseStatus)
		{
			reverseStatus = wantReverse;
			reverseChangeFlag = true;
		}
	}

	if (safeStateDataCheck)
	{
		safeStateDataCheck = false;

		brakeStatus     = false;
		reverseStatus   = false;
		safeStateStatus = true;

		LED_ChangeState(&ledDirection,       LED_BLINK);
		LED_ChangeState(&ledSidePosition,    LED_OFF);
		if (boardIsLeft)
		{
			LED_ChangeState(&ledLongLight, LED_OFF);
		}
		LED_ChangeState(&ledPositionCircles, LED_OFF);

		brakeChangeFlag   = true;
		reverseChangeFlag = true;
	}
}

void SetCanFilters(void)
{
	CAN_FilterTypeDef filterConfig;

	filterConfig.FilterActivation = CAN_FILTER_ENABLE;
	filterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	filterConfig.FilterScale = CAN_FILTERSCALE_16BIT;

	filterConfig.FilterBank = 0;
	filterConfig.FilterIdHigh = CAN_STD_ID(DASHBOARD_LIGHTS_FRAME_ID);
	filterConfig.FilterIdLow  = CAN_STD_ID(DASHBOARD_CONTROL_FRAME_ID);
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);

	filterConfig.FilterBank = 1;
	filterConfig.FilterIdHigh = CAN_STD_ID(PEDALS_JTNS_WORKS_FRAME_ID);
	filterConfig.FilterIdLow  = CAN_STD_ID(SAFE_STATE_FRAME_ID);
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);
}

// Weak-symbol override: HAL's default HAL_IncTick doesn't drive the LED_driver's
// syncTick, so blinking wouldn't advance without this.
void HAL_IncTick(void)
{
	uwTick += uwTickFreq;
	LED_IncSyncTick();
}
