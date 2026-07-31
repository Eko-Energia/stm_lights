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

extern CAN_HandleTypeDef hcan;

static CAN_RxHeaderTypeDef CAN_currentMessageHeader;

// Only the single byte each frame's logic consumes is latched; byte accesses
// are atomic on Cortex-M, so ISR <-> main-loop sharing cannot tear.
static volatile uint8_t dashboardLightsByte; // frame 994, byte 0
static volatile uint8_t pedalsBrakeHallByte; // frame 65, byte BREAKS_HALL_INTPOS
static volatile uint8_t dashboardPrndByte;   // frame 993, byte 1
// SafeState_SyncTick (30): SyncTick 0|32@1+ little-endian ms
static volatile uint32_t safeStateSyncTick;

static volatile uint8_t dashboardLightsDataCheck = 0U;
static volatile uint8_t pedalsJtnsWorksDataCheck = 0U;
static volatile uint8_t dashboardControlDataCheck = 0U;
static volatile uint8_t safeStateSyncTickDataCheck = 0U;
volatile uint8_t safeStateDataCheck = 0U;

volatile uint8_t brakeStatus = 0U;
volatile uint8_t reverseStatus = 0U;

volatile uint8_t brakeChangeFlag = 0U;
volatile uint8_t reverseChangeFlag = 0U;

/**
  * @brief RX FIFO0 interrupt callback: latches the consumed payload byte and
  *        raises the pending flag of the received frame.
  * @param hcan   CAN handle that raised the interrupt.
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	uint8_t tempData[CAN_MAX_DLC];

	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_currentMessageHeader, tempData) != HAL_OK)
	{
		return;
	}

	switch (CAN_currentMessageHeader.StdId)
	{
		case DASHBOARD_LIGHTS_FRAME_ID:
			dashboardLightsByte = tempData[0];
			dashboardLightsDataCheck = 1U;
			break;

		case PEDALS_JTNS_WORKS_FRAME_ID:
			pedalsBrakeHallByte = tempData[BREAKS_HALL_INTPOS];
			pedalsJtnsWorksDataCheck = 1U;
			break;

		case DASHBOARD_CONTROL_FRAME_ID:
			dashboardPrndByte = tempData[1];
			dashboardControlDataCheck = 1U;
			break;

		case SAFE_STATE_FRAME_ID:
			safeStateDataCheck = 1U;
			break;

		case SAFE_STATE_SYNC_TICK_FRAME_ID:
			// SyncTick : 0|32@1+ (1,0) — Intel/little-endian, bytes 0..3
			safeStateSyncTick = (uint32_t)tempData[0]
			                  | ((uint32_t)tempData[1] << 8)
			                  | ((uint32_t)tempData[2] << 16)
			                  | ((uint32_t)tempData[3] << 24);
			safeStateSyncTickDataCheck = 1U;
			break;
	}
}

/**
  * @brief Applies a pending SafeState_SyncTick frame to the LED sync counter.
  */
static void UpdateLedSyncTick(void)
{
	if (safeStateSyncTickDataCheck)
	{
		safeStateSyncTickDataCheck = 0U;
		LED_SetSyncTick(safeStateSyncTick);
	}
}

/**
  * @brief Applies pending CAN frames to the light outputs and shared statuses.
  *
  * Does nothing while safe state is active: the pending flags and latched
  * payload bytes are left untouched, so the newest state is applied on the
  * first call after safe state ends.
  */
void APP_InterpretFrames(void)
{
	// Keep blink phase aligned even while safe state holds light outputs.
	UpdateLedSyncTick();

	if (safeStateActive)
	{
		return;
	}

	if (dashboardLightsDataCheck)
	{
		dashboardLightsDataCheck = 0U;

		// Dashboard_Lights (994) byte 0 layout, per CAN_DB.dbc:
		//   bits 0-2 Headlights   (0 OFF, 1 AUTO, 2 DAY, 3 NIGHT, 4 HIGHBEAMS)
		//   bits 3-4 TurnSig_Left (0 OFF, 1 ONCE, 2 ON)
		//   bits 5-6 TurnSig_Right
		//   bit  7   Emergency
		const uint8_t byte        = dashboardLightsByte;
		const uint8_t headlights  = byte & 0x7U;
		const uint8_t leftSignal  = (byte >> 3) & 0x3U;
		const uint8_t rightSignal = (byte >> 5) & 0x3U;

		const uint8_t leftTurn  = (leftSignal  != 0U);
		const uint8_t rightTurn = (rightSignal != 0U);
		const uint8_t emergency = ((byte >> 7) & 0x1U) != 0U;

		const uint8_t blinkOn = (leftTurn && boardIsLeft)
		                     || emergency
		                     || (rightTurn && !boardIsLeft);
		LED_ChangeState(&ledDirection, blinkOn ? LED_BLINK : LED_OFF);

		uint8_t applyPosition = 1U;
		uint8_t wantPosition  = 0U;
		switch (headlights)
		{
			case 0U:
				wantPosition = 0U;
				break;
			case 2U:
			case 3U:
			case 4U:
				wantPosition = 1U;
				break;
			default:
				// AUTO (1) or reserved: leave position state untouched.
				applyPosition = 0U;
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

		// Position is read back from the LED itself so AUTO (which skips the
		// block above) still yields the right circles output when blinkOn changes.
		const uint8_t positionOn = (ledSidePosition.state == LED_ON);
		LED_ChangeState(&ledPositionCircles,
		                blinkOn ? LED_OFF : (positionOn ? LED_ON : LED_OFF));
	}

	if (pedalsJtnsWorksDataCheck)
	{
		pedalsJtnsWorksDataCheck = 0U;

		const uint8_t wantBrake = pedalsBrakeHallByte > BREAK_HALL_EPS;
		if (wantBrake != brakeStatus)
		{
			brakeStatus = wantBrake;
			brakeChangeFlag = 1U;
		}
	}

	if (dashboardControlDataCheck)
	{
		dashboardControlDataCheck = 0U;

		// PRND: bit 14, length 2 -> high 2 bits of byte 1.
		const uint8_t prnd = (dashboardPrndByte >> 6) & 0x3U;

		const uint8_t wantReverse = (prnd == PRND_REVERSE_VALUE);
		if (wantReverse != reverseStatus)
		{
			reverseStatus = wantReverse;
			reverseChangeFlag = 1U;
		}
	}
}

/**
  * @brief Configures the bxCAN acceptance filters for the consumed frame IDs.
  */
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

	filterConfig.FilterBank = 2;
	filterConfig.FilterIdHigh = CAN_STD_ID(SAFE_STATE_SYNC_TICK_FRAME_ID);
	filterConfig.FilterIdLow  = CAN_STD_ID(SAFE_STATE_SYNC_TICK_FRAME_ID);
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);
}
