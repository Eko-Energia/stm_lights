#include "main.h"
#include "can_driver.h"
#include "error_handler.h"
#include "board_selector.h"
#include "app.h"
#include "pwm_driver.h"
#include "global_variables.h"
#include "adc_driver.h"
#include "rear_service.h"
#include "front_service.h"
#include <stdbool.h>

CAN_RxHeaderTypeDef   CAN_currentMessageHeader;
uint8_t               CAN_currentMessageData[8];
bool datacheck;
CAN_FilterTypeDef canfilterconfig;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim1;
struct PWM_Out_signal PWM;
ADC_ChannelsConfigTypeDefs cadc1;
ADC_ChannelsConfigTypeDefs cadc2;


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_currentMessageHeader, CAN_currentMessageData);

	switch (CAN_currentMessageHeader.StdId){

		case Dashboard_SwitchesFrameID:

			if((CAN_currentMessageData[0] & (1 << Turn_Signal_LeftBitpos)) && !getLeftTurnStatus() ){
				setLeftTurnStatusTrue();
			}

			if((CAN_currentMessageData[0] & (1 << Turn_Signal_RightBitpos)) && !getRightTurnStatus() ){
				setRightTurnStatusTrue();
			}

			if((CAN_currentMessageData[0] & (1 << EmergencyBitpos)) && !getEmergencyStatus() ){
				setEmergencyStatusTrue();
			}

			if((CAN_currentMessageData[0] & (0 << Turn_Signal_LeftBitpos)) && getLeftTurnStatus() ){
				setLeftTurnStatusFalse();
			}

			if((CAN_currentMessageData[0] & (0 << Turn_Signal_RightBitpos)) && getRightTurnStatus() ){
				setRightTurnStatusFalse();
			}

			if((CAN_currentMessageData[0] & (0 << EmergencyBitpos)) && getEmergencyStatus() ){
				setEmergencyStatusFalse();
			}

			break;


		case Dashboard_ImportantFrameID:

			switch(CAN_currentMessageData[Light_SwitchIntpos]){

				case 0:
					if(getPositionStatus()){
						setPositionStatusFalse();
						setPositionChangeFlagTrue();
						break;
					}
					if(getLowBeamStatus()){
						setLowBeamStatusFalse();
						setLowBeamChangeFlagTrue();
						break;
					}
					if(getHighBeamStatus()){
						setHighBeamStatusFalse();
						setHighBeamChangeFlagTrue();
						break;
					}
					break;

				case 1:
					if(!getPositionStatus()){
						setPositionStatusTrue();
						setPositionChangeFlagTrue();
					}
					if(getLowBeamStatus()){
						setLowBeamStatusFalse();
						setLowBeamChangeFlagTrue();
						break;
					}
					if(getHighBeamStatus()){
						setHighBeamStatusFalse();
						setHighBeamChangeFlagTrue();
						break;
					}
					break;

				case 2:
					if(!getLowBeamStatus()){
						setLowBeamStatusTrue();
						setLowBeamChangeFlagTrue();
					}
					if(getPositionStatus()){
						setPositionStatusFalse();
						setPositionChangeFlagTrue();
						break;
					}
					if(getHighBeamStatus()){
						setHighBeamStatusFalse();
						setHighBeamChangeFlagTrue();
						break;
					}
					break;

				case 3:
					if(!getHighBeamStatus()){
						setHighBeamStatusTrue();
						setHighBeamChangeFlagTrue();
					}
					if(getLowBeamStatus()){
						setLowBeamStatusFalse();
						setLowBeamChangeFlagTrue();
						break;
					}
					if(getPositionStatus()){
						setPositionStatusFalse();
						setPositionChangeFlagTrue();
						break;
					}
					break;
			}
			break;


		case Pedals_JTNS_WORKSFrameID:

			if(CAN_currentMessageData[BreaksHallIntpos] > BreakHallEps && !getBrakeStatus()){
				setBrakeStatusTrue();
				setBrakeChangeFlagTrue();
				break;
			}

			if(CAN_currentMessageData[BreaksHallIntpos] < BreakHallEps && getBrakeStatus()){
				setBrakeStatusFalse();
				setBrakeChangeFlagTrue();
				break;
			}
			break;

		case Dashboard_ControlFrameID:
		{
		    uint8_t prnd =
		        ((CAN_currentMessageData[PRND_BITPOS / 8] >> (PRND_BITPOS % 8)) &
		         ((1U << (8 - (PRND_BITPOS % 8))) - 1)) |

		        ((CAN_currentMessageData[(PRND_BITPOS / 8) + 1] &
		         ((1U << (PRND_LENGTH - (8 - (PRND_BITPOS % 8)))) - 1))
		         << (8 - (PRND_BITPOS % 8)));

		    if (prnd == PRND_REVERSE_VALUE && !getReverseStatus()) {
		        setReverseStatusTrue();
		        setReverseChangeFlagTrue();
		    }
		    else if (prnd != PRND_REVERSE_VALUE && getReverseStatus()) {
		        setReverseStatusFalse();
		        setReverseChangeFlagTrue();
		    }

		    break;
		}
		case SafeStateFrameID:
            setBrakeStatusFalse();
            setBrakeChangeFlagTrue();
            setReverseStatusFalse();
            setReverseChangeFlagTrue();
			setPositionStatusFalse();
			setPositionChangeFlagTrue();
			setLowBeamStatusFalse();
			setLowBeamChangeFlagTrue();
			setHighBeamStatusFalse();
			setHighBeamChangeFlagTrue();
			setEmergencyStatusTrue();
			setSafeStateStatusTrue();
			break;
	}
}
void static CAN_setFilters(){

    canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig.FilterMode = CAN_FILTERMODE_IDLIST;
    canfilterconfig.FilterScale = CAN_FILTERSCALE_16BIT;
    canfilterconfig.FilterBank = 0;

    canfilterconfig.FilterIdHigh =
        ((uint16_t)(Dashboard_SwitchesFrameID << 5)) |
        ((uint16_t)(Dashboard_ImportantFrameID << 5) << 16);

    canfilterconfig.FilterIdLow  =
        ((uint16_t)(Pedals_JTNS_WORKSFrameID << 5)) |
        ((uint16_t)(SafeStateFrameID << 5) << 16);

    HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);


    canfilterconfig.FilterBank = 1;

    canfilterconfig.FilterIdHigh =
        ((uint16_t)(Dashboard_ControlFrameID << 5)) |
        ((uint16_t)(Dashboard_ControlFrameID << 5) << 16);

    canfilterconfig.FilterIdLow = 0;

    HAL_CAN_ConfigFilter(&hcan, &canfilterconfig);
}
void app_main(){
	CAN_Init(&hcan);
	ADC_Init(&hadc1, &cadc1);
	ADC_Init(&hadc2, &cadc2);
	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
	CAN_setFilters();
	PWM_Out_Init(&PWM,&htim1,TIM_CHANNEL_2,0,250);
	uint32_t board = chooseBoard();
	switch(board){
		case leftFront:
			LF_service();
			break;
		case rightFront:
			RF_service();
			break;
		case leftBack:
			LB_service();
			break;
		case rightBack:
			RB_service();
			break;
		default:

	}
}
