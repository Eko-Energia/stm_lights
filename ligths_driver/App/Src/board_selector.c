/**
  * @file board_selector.c
  * @brief Detects which light board the firmware runs on from the selector ADC pin voltage.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#include "board_selector.h"
#include "adc_driver.h"
#include "main.h"
#include <math.h>

// Half-width of the voltage window accepted around each board's nominal level.
#define SELECTOR_VOLTAGE_TOLERANCE (0.2f)

extern ADC_ChannelsConfigTypeDefs cadc2;
extern ADC_HandleTypeDef hadc2;

board_e BOARD_ChooseBoard(void)
{
	float selectorPinVoltage = 0.0f;
	ADC_Get_PinVoltage(&hadc2, &cadc2, ADC_CHANNEL_2, &selectorPinVoltage);

	if (fabs(LEFT_BACK_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
	{
		return BOARD_LEFT_BACK;
	}
	else if (fabs(RIGHT_BACK_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
	{
		return BOARD_RIGHT_BACK;
	}
	else if (fabs(LEFT_FRONT_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
	{
		return BOARD_LEFT_FRONT;
	}
	else if (fabs(RIGHT_FRONT_VOLTAGE - selectorPinVoltage) < SELECTOR_VOLTAGE_TOLERANCE)
	{
		return BOARD_RIGHT_FRONT;
	}

	return BOARD_UNKNOWN;
}
