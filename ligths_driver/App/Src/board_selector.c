
#include "board_selector.h"
#include "main.h"
#include "adc_driver.h"
#include <math.h>
extern ADC_ChannelsConfigTypeDefs cadc2;
extern ADC_HandleTypeDef hadc2;

float chooseBoard() {
    float selectorPinVoltage;
    ADC_Get_PinVoltage(&hadc2, &cadc2, ADC_CHANNEL_2, &selectorPinVoltage);
    if (fabs(LEFT_BACK - selectorPinVoltage) < 0.2f) {
        return leftBack;
    }
    else if (fabs(RIGHT_BACK - selectorPinVoltage) < 0.2f) {
        return rightBack;
    }
    else if (fabs(LEFT_FRONT - selectorPinVoltage) < 0.2f) {
        return leftFront;
    }
    else if (fabs(RIGHT_FRONT - selectorPinVoltage) < 0.2f) {
        return rightFront;
    }
    return -69.0f; // dodałem żeby ide nie rzucało warninga bo mnie irytował no ale jeśli kod tu dotre to MA spaść z rowerka
}
