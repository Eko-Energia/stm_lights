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

// 1U = LB, 0U = RB. Definition is in rear_service.c.
extern const uint8_t boardIsLeft;

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

// Nonzero while safe state is active; APP_InterpretFrames keeps frames pending then.
extern uint8_t safeStateActive;

/**
  * @brief Rear-board service entry point; initializes CAN and runs the main
  *        loop (does not return).
  */
void RearService(void);

#endif // REAR_SERVICE_H
