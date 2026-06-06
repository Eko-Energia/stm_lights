/**
  * @file rear_service.h
  * @brief Pin mapping, thresholds and service entry points for the rear (LB/RB) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef REAR_SERVICE_H
#define REAR_SERVICE_H

#include "main.h"
#include <stdbool.h>

#define REVERSE_PIN (GPIO_PIN_15)
#define REVERSE_PORT (GPIOA)

#define SIDE_STOP_PIN (GPIO_PIN_3)
#define SIDE_STOP_PORT (GPIOB)

#define DIRECTION_PIN (GPIO_PIN_1)
#define DIRECTION_PORT (GPIOB)

#define POSITION_CIRCLES_PIN (GPIO_PIN_0)
#define POSITION_CIRCLES_PORT (GPIOB)

#define SAFE_STATE_PIN (GPIO_PIN_8)
#define SAFE_STATE_PORT (GPIOA)

#define SIDE_POSITION_PIN (GPIO_PIN_5)
#define SIDE_POSITION_PORT (GPIOB)

#define MIDDLE_STOP_PIN (GPIO_PIN_4)
#define MIDDLE_STOP_PORT (GPIOB)

#define WHOLE_POSITION_PIN (GPIO_PIN_4)
#define WHOLE_POSITION_PORT (GPIOB)

#define SIDE_POSITION_OVERLOAD (1.375f)
#define MIDDLE_STOP_OVERLOAD (0.77f)
#define WHOLE_POSITION_OVERLOAD (0.77f)
#define POSITION_CIRCLES_OVERLOAD (1.375f)
#define DIRECTION_OVERLOAD (1.375f)
#define SAFE_STATE_OVERLOAD (2.508f)

#define VOLTAGE_LOW (0.01f)

#define NODE_FRAME_ID_RB (0x780)
#define STATUS_FRAME_ID_RB (0x781)

#define NODE_FRAME_ID_LB (0x7A0)
#define STATUS_FRAME_ID_LB (0x7A1)

#define STATUS_FRAME_PERIOD_MS (1000)
#define STATUS_FRAME_DLC (1)

#define SAFE_STATE_DURATION_MS (1200)

/**
  * @brief Service routine for a rear light board (does not return).
  * @param left   true to service the left-back board, false for the right-back board.
  */
void RearService(bool left);


#endif // REAR_SERVICE_H
