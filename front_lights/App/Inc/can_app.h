/**
  * @file can_app.h
  * @brief Application-level CAN frame IDs and interpret/filter entry points.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef CAN_APP_H
#define CAN_APP_H

#include "main.h"

/* Frame IDs of every message this node subscribes to; all taken from CAN_DB.dbc. */

/** Dashboard_Lights: headlight mode, turn signals and emergency. Sent on event. */
#define DASHBOARD_LIGHTS_FRAME_ID (994)
/** SafeState_NODE: car-wide safe-state broadcast, forces all lamps to a safe pattern. */
#define SAFE_STATE_FRAME_ID (1)
/** SafeState_SyncTick: network millisecond counter that keeps blink phases aligned. */
#define SAFE_STATE_SYNC_TICK_FRAME_ID (30)

/**
  * Converts a standard 11-bit CAN ID into the register layout the bxCAN filter
  * banks expect, which hold the ID left-aligned from bit 5 upwards.
  */
#define CAN_STD_ID(id) ((uint16_t)((id) << 5))

// Set by the RX ISR on a SafeState_NODE frame, consumed by front_service.
extern volatile uint8_t safeStateDataCheck;

/**
  * @brief Applies pending CAN frames to the light outputs
  *        (does nothing while safe state is active).
  */
void APP_InterpretFrames(void);

/**
  * @brief Configures the bxCAN acceptance filters for the consumed frame IDs.
  */
void SetCanFilters(void);

#endif // CAN_APP_H
