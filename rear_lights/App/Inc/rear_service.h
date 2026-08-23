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

/**
  * Which rear board this firmware is built for: 1U = left-back, 0U = right-back.
  * Selects the CAN node/status IDs and which turn signal drives the indicator.
  * Compile-time constant - edit the definition in rear_service.c and reflash.
  */
extern const uint8_t boardIsLeft;

/** Error-handler node ID, base of this node's error frames (DBC LightsRR_NODE). */
#define NODE_FRAME_ID_RB (0x780)
/** Periodic status frame ID for the right-back board (DBC LightsRR_Status). */
#define STATUS_FRAME_ID_RB (0x781)

/** Error-handler node ID, base of this node's error frames (DBC LightsRL_NODE). */
#define NODE_FRAME_ID_LB (0x7A0)
/** Periodic status frame ID for the left-back board (DBC LightsRL_Status). */
#define STATUS_FRAME_ID_LB (0x7A1)

/** Transmit period of the status frame, in milliseconds. */
#define STATUS_FRAME_PERIOD_MS (100)
/** Status frame payload length; one byte holds all seven output bits. */
#define STATUS_FRAME_DLC (1)

/**
  * How long the safe-state outputs are held after the last SafeState frame.
  * Every further SafeState frame restarts this window, so an uninterrupted
  * broadcast keeps the board latched in safe state.
  */
#define SAFE_STATE_DURATION_MS (1200)

// LED contexts owned by rear_service, exposed so the CAN interpretation layer
// can push event-driven light states directly.
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
