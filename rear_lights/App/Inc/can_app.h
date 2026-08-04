/**
  * @file can_app.h
  * @brief Application-level CAN frame IDs, shared light state, and interpret/filter entry points.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef CAN_APP_H
#define CAN_APP_H

#include "main.h"

/* Frame IDs of every message this node subscribes to; all taken from CAN_DB.dbc. */

/** Dashboard_Lights: headlight mode, turn signals and emergency. Sent on event. */
#define DASHBOARD_LIGHTS_FRAME_ID (994)
/** Dashboard_Control: steering encoder plus the PRND gear selector. */
#define DASHBOARD_CONTROL_FRAME_ID (993)
/** Pedals_JTSN_WORKS: steering, both brake sensors and the accelerator. */
#define PEDALS_JTNS_WORKS_FRAME_ID (65)
/** SafeState_NODE: car-wide safe-state broadcast, forces all lamps to a safe pattern. */
#define SAFE_STATE_FRAME_ID (1)
/** SafeState_SyncTick: network millisecond counter that keeps blink phases aligned. */
#define SAFE_STATE_SYNC_TICK_FRAME_ID (30)

/** Byte index of BrakesLinear inside the pedals frame (DBC bit 8, length 8). */
#define BREAKS_LINEAR_INTPOS (1)
/** Byte index of BrakesHall inside the pedals frame (DBC bit 16, length 8). */
#define BREAKS_HALL_INTPOS (2)

/*
 * Brake-detection thresholds. Both sensors report 0-100 %, and the brake lamp
 * lights when either reads strictly above its threshold. Zero means no deadband,
 * so any nonzero reading counts as braking - raise these if sensor noise causes
 * the lamp to flicker at rest.
 */
#define BREAK_HALL_EPS (0)
#define BREAK_LINEAR_EPS (0)

/** PRND value that means reverse. DBC enum: 0 P, 1 R, 2 N, 3 D. */
#define PRND_REVERSE_VALUE (1)

/**
  * Converts a standard 11-bit CAN ID into the register layout the bxCAN filter
  * banks expect, which hold the ID left-aligned from bit 5 upwards.
  */
#define CAN_STD_ID(id) ((uint16_t)((id) << 5))

// Light state shared with rear_service: *Status holds the requested output,
// *ChangeFlag marks it as not yet applied to the LED.
extern volatile uint8_t brakeStatus;
extern volatile uint8_t reverseStatus;

extern volatile uint8_t brakeChangeFlag;
extern volatile uint8_t reverseChangeFlag;

// Set by the RX ISR on a SafeState_NODE frame, consumed by rear_service.
extern volatile uint8_t safeStateDataCheck;

/**
  * @brief Applies pending CAN frames to the light outputs and shared statuses
  *        (does nothing while safe state is active).
  */
void APP_InterpretFrames(void);

/**
  * @brief Configures the bxCAN acceptance filters for the consumed frame IDs.
  */
void SetCanFilters(void);

#endif // CAN_APP_H
