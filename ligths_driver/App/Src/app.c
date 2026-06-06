/**
  * @file app.c
  * @brief Application core: CAN frame interpretation and per-board dispatch for the PERLA lights driver.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "app.h"
#include "board_selector.h"
#include "rear_service.h"
#include "front_service.h"
#include "can_driver.h"
#include "adc_driver.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CAN_HandleTypeDef hcan;

ADC_ChannelsConfigTypeDefs cadc1;
ADC_ChannelsConfigTypeDefs cadc2;

static CAN_RxHeaderTypeDef CAN_currentMessageHeader;

static uint8_t canDashboardSwitchesData[CAN_MAX_DLC];
static uint8_t canDashboardImportantData[CAN_MAX_DLC];
static uint8_t canPedalsJtnsWorksData[CAN_MAX_DLC];
static uint8_t canDashboardControlData[CAN_MAX_DLC];
static uint8_t canSafeStateData[CAN_MAX_DLC];

static volatile bool dashboardSwitchesDataCheck = false;
static volatile bool dashboardImportantDataCheck = false;
static volatile bool pedalsJtnsWorksDataCheck = false;
static volatile bool dashboardControlDataCheck = false;
static volatile bool safeStateDataCheck = false;

volatile bool brakeStatus = false;
volatile bool positionStatus = false;
volatile bool lowBeamStatus = false;
volatile bool highBeamStatus = false;
volatile bool rightTurnStatus = false;
volatile bool leftTurnStatus = false;
volatile bool safeStateStatus = false;
volatile bool emergencyStatus = false;
volatile bool reverseStatus = false;

volatile bool brakeChangeFlag = false;
volatile bool positionChangeFlag = false;
volatile bool lowBeamChangeFlag = false;
volatile bool highBeamChangeFlag = false;
volatile bool reverseChangeFlag = false;
volatile bool indicatorChangeFlag = false;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	uint8_t tempData[CAN_MAX_DLC];

	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_currentMessageHeader, tempData);

	switch (CAN_currentMessageHeader.StdId)
	{
		case DASHBOARD_SWITCHES_FRAME_ID:
			memcpy(canDashboardSwitchesData, tempData, CAN_MAX_DLC);
			dashboardSwitchesDataCheck = true;
			break;

		case DASHBOARD_IMPORTANT_FRAME_ID:
			memcpy(canDashboardImportantData, tempData, CAN_MAX_DLC);
			dashboardImportantDataCheck = true;
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
	if (dashboardSwitchesDataCheck)
	{
		dashboardSwitchesDataCheck = false;

		const bool leftSignal = (canDashboardSwitchesData[0] & (1 << TURN_SIGNAL_LEFT_BITPOS)) != 0;
		const bool rightSignal = (canDashboardSwitchesData[0] & (1 << TURN_SIGNAL_RIGHT_BITPOS)) != 0;
		const bool emergencySignal = (canDashboardSwitchesData[0] & (1 << EMERGENCY_BITPOS)) != 0;

		if (leftSignal != leftTurnStatus)
		{
			indicatorChangeFlag = true;

			if (leftSignal)
			{
				leftTurnStatus = true;
			}
			else
			{
				leftTurnStatus = false;
			}
		}

		if (rightSignal != rightTurnStatus)
		{
			indicatorChangeFlag = true;

			if (rightSignal)
			{
				rightTurnStatus = true;
			}
			else
			{
				rightTurnStatus = false;
			}
		}

		if (emergencySignal != emergencyStatus)
		{
			indicatorChangeFlag = true;

			if (emergencySignal)
			{
				emergencyStatus = true;
			}
			else
			{
				emergencyStatus = false;
			}
		}
	}

	if (dashboardImportantDataCheck)
	{
		dashboardImportantDataCheck = false;

		switch (canDashboardImportantData[LIGHT_SWITCH_INTPOS])
		{
			case 0:
				if (positionStatus)
				{
					positionStatus = false;
					positionChangeFlag = true;
					break;
				}

				if (lowBeamStatus)
				{
					lowBeamStatus = false;
					lowBeamChangeFlag = true;
					break;
				}

				if (highBeamStatus)
				{
					highBeamStatus = false;
					highBeamChangeFlag = true;
					break;
				}

				break;

			case 1:
				if (!positionStatus)
				{
					positionStatus = true;
					positionChangeFlag = true;
				}

				if (lowBeamStatus)
				{
					lowBeamStatus = false;
					lowBeamChangeFlag = true;
					break;
				}

				if (highBeamStatus)
				{
					highBeamStatus = false;
					highBeamChangeFlag = true;
					break;
				}

				break;

			case 2:
				if (!lowBeamStatus)
				{
					lowBeamStatus = true;
					lowBeamChangeFlag = true;
				}

				// Position lights stay on while low beams are active.
				if (!positionStatus)
				{
					positionStatus = true;
					positionChangeFlag = true;
				}

				if (highBeamStatus)
				{
					highBeamStatus = false;
					highBeamChangeFlag = true;
					break;
				}

				break;

			case 3:
				if (!highBeamStatus)
				{
					highBeamStatus = true;
					highBeamChangeFlag = true;
				}

				if (lowBeamStatus)
				{
					lowBeamStatus = false;
					lowBeamChangeFlag = true;
					break;
				}

				// Position lights stay on while high beams are active.
				if (!positionStatus)
				{
					positionStatus = true;
					positionChangeFlag = true;
				}

				break;
		}
	}

	if (pedalsJtnsWorksDataCheck)
	{
		pedalsJtnsWorksDataCheck = false;

		if (canPedalsJtnsWorksData[BREAKS_HALL_INTPOS] > BREAK_HALL_EPS && !brakeStatus)
		{
			brakeStatus = true;
			brakeChangeFlag = true;
		}

		if (canPedalsJtnsWorksData[BREAKS_HALL_INTPOS] < BREAK_HALL_EPS && brakeStatus)
		{
			brakeStatus = false;
			brakeChangeFlag = true;
		}
	}

	if (dashboardControlDataCheck)
	{
		dashboardControlDataCheck = false;

		const uint8_t prnd =
			((canDashboardControlData[PRND_BITPOS / 8] >>
			(PRND_BITPOS % 8)) &
			((1U << (8 - (PRND_BITPOS % 8))) - 1)) |
			((canDashboardControlData[(PRND_BITPOS / 8) + 1] &
			((1U << (PRND_LENGTH - (8 - (PRND_BITPOS % 8)))) - 1))
			<< (8 - (PRND_BITPOS % 8)));

		if (prnd == PRND_REVERSE_VALUE && !reverseStatus)
		{
			reverseStatus = true;
			reverseChangeFlag = true;
		}
		else if (prnd != PRND_REVERSE_VALUE && reverseStatus)
		{
			reverseStatus = false;
			reverseChangeFlag = true;
		}
	}

	if (safeStateDataCheck)
	{
		safeStateDataCheck = false;

		brakeStatus = false;
		brakeChangeFlag = true;

		reverseStatus = false;
		reverseChangeFlag = true;

		positionStatus = false;
		positionChangeFlag = true;

		lowBeamStatus = false;
		lowBeamChangeFlag = true;

		highBeamStatus = false;
		highBeamChangeFlag = true;

		emergencyStatus = true;
		safeStateStatus = true;
	}
}

static void SetCanFilters(void)
{
	CAN_FilterTypeDef filterConfig;

	filterConfig.FilterActivation = CAN_FILTER_ENABLE;
	filterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
	filterConfig.FilterScale = CAN_FILTERSCALE_16BIT;

	filterConfig.FilterBank = 0;
	filterConfig.FilterIdHigh = CAN_STD_ID(DASHBOARD_IMPORTANT_FRAME_ID);
	filterConfig.FilterIdLow = CAN_STD_ID(DASHBOARD_SWITCHES_FRAME_ID);
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);

	filterConfig.FilterBank = 1;
	filterConfig.FilterIdHigh = CAN_STD_ID(PEDALS_JTNS_WORKS_FRAME_ID);
	filterConfig.FilterIdLow = CAN_STD_ID(SAFE_STATE_FRAME_ID);
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);

	filterConfig.FilterBank = 2;
	filterConfig.FilterIdHigh = CAN_STD_ID(DASHBOARD_CONTROL_FRAME_ID);
	filterConfig.FilterIdLow = 0;
	HAL_CAN_ConfigFilter(&hcan, &filterConfig);
}

void APP_Main(void)
{
	CAN_Init(&hcan);
	HAL_ADC_Init(&hadc1);
	HAL_ADC_Init(&hadc2);
	ADC_Init(&hadc1, &cadc1);
	ADC_Init(&hadc2, &cadc2);
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
	SetCanFilters();

	const board_e board = BOARD_ChooseBoard();
	switch (board)
	{
		case BOARD_LEFT_FRONT:
			LF_Service();
			break;
		case BOARD_RIGHT_FRONT:
			RF_Service();
			break;
		case BOARD_LEFT_BACK:
			RearService(true);
			break;
		case BOARD_RIGHT_BACK:
			RearService(false);
			break;
		case BOARD_UNKNOWN:
			break;
	}
	RearService(true);
}
