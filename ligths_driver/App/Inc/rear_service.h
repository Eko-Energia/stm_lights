#include "main.h"
#include "error_handler.h"
#include "app.h"

#define reverse_pin GPIO_PIN_15
#define reverse_port GPIOA

#define sideStop_pin GPIO_PIN_3
#define sideStop_port GPIOB

#define direction_pin GPIO_PIN_1
#define direction_port GPIOB

#define positionCircles_TIM htim1
#define positionCircles_Channel TIM_CHANNEL_2

#define safeState_pin GPIO_PIN_8
#define safeState_port GPIOA

#define sidePosition_pin GPIO_PIN_5
#define sidePosition_port GPIOB

#define middleStop_pin GPIO_PIN_4
#define middleStop_port GPIOB

#define wholePosition_pin GPIO_PIN_4
#define wholePosition_port GPIOB

#define sidePositionOverload 1.375f
#define middleStopOverload 0.77f
#define wholePositionOverload 0.77f
#define positionCirclesOverload 1.375f
#define directionOverload 1.375f
#define safeStateOverload 2.508f

#define voltageLow 0.1f

#define nodeFrameIdRB 0x780
#define statusFrameIdRB 0x781

#define nodeFrameIdLB 0x7A0
#define statusFrameIdLB 0x7A1


void LB_service();
void RB_service();
void getLBStatus(uint8_t *data,void *context);
void getRBStatus(uint8_t *data,void *context);
