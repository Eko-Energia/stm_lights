/**
  * @file rear_service.h
  * @brief Service entry point shared by the rear (LB/RB) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef REAR_SERVICE_H
#define REAR_SERVICE_H

#include "main.h"
#include "led_driver.h"
#include <stdbool.h>

// true = LB, false = RB. Definition is in rear_service.c.
extern const bool boardIsLeft;

#define NODE_FRAME_ID_RB (0x780)
#define STATUS_FRAME_ID_RB (0x781)

#define NODE_FRAME_ID_LB (0x7A0)
#define STATUS_FRAME_ID_LB (0x7A1)

#define STATUS_FRAME_PERIOD_MS (1000)
#define STATUS_FRAME_DLC (1)

#define SAFE_STATE_DURATION_MS (1200)

extern struct LED ledReverse;
extern struct LED ledSideStop;
extern struct LED ledDirection;
extern struct LED ledPositionCircles;
extern struct LED ledSafeState;
extern struct LED ledSidePosition;
extern struct LED ledLongLight;

void RearService(void);

#endif // REAR_SERVICE_H
