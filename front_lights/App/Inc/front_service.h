/**
  * @file front_service.h
  * @brief Service entry point shared by the front (FL/FR) light boards.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef FRONT_SERVICE_H
#define FRONT_SERVICE_H

#include "main.h"
#include "led_driver.h"

/**
  * Which front board this firmware is built for: 1U = front-left, 0U = front-right.
  * Selects the CAN node/status IDs and which turn signal drives the indicators.
  * Compile-time constant - edit the definition in front_service.c and reflash.
  */
extern const uint8_t boardIsLeft;

/** Error-handler node ID, base of this node's error frames (DBC LightsFR_NODE). */
#define NODE_FRAME_ID_FR (0x740)
/** Periodic status frame ID for the front-right board (DBC LightsFR_Status). */
#define STATUS_FRAME_ID_FR (0x741)

/** Error-handler node ID, base of this node's error frames (DBC LightsFL_NODE). */
#define NODE_FRAME_ID_FL (0x760)
/** Periodic status frame ID for the front-left board (DBC LightsFL_Status). */
#define STATUS_FRAME_ID_FL (0x761)

/** Transmit period of the status frame, in milliseconds. */
#define STATUS_FRAME_PERIOD_MS (1000)
/** Status frame payload length; one byte holds all seven output bits. */
#define STATUS_FRAME_DLC (1)

/**
  * How long the safe-state outputs are held after the last SafeState frame.
  * Every further SafeState frame restarts this window, so an uninterrupted
  * broadcast keeps the board latched in safe state.
  */
#define SAFE_STATE_DURATION_MS (1200)

// LED contexts owned by front_service, exposed so the CAN interpretation layer
// can push event-driven light states directly.
extern struct LED ledHighBeam;
extern struct LED ledLowBeam;
extern struct LED ledIndicator;
extern struct LED ledSideIndicator;
extern struct LED ledDayLight1;
extern struct LED ledDayLight2;
extern struct LED ledPosition;

// Nonzero while safe state is active; APP_InterpretFrames keeps frames pending then.
extern uint8_t safeStateActive;

/**
  * @brief Front-board service entry point; initializes CAN and runs the main
  *        loop (does not return).
  */
void FrontService(void);

#endif // FRONT_SERVICE_H
