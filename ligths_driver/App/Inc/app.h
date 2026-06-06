/**
  * @file app.h
  * @brief Application core: CAN frame interpretation and per-board dispatch for the PERLA lights driver.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef APP_H
#define APP_H

#include "main.h"
#include <stdbool.h>

#define DASHBOARD_SWITCHES_FRAME_ID (995)
#define DASHBOARD_IMPORTANT_FRAME_ID (994)
#define PEDALS_JTNS_WORKS_FRAME_ID (65)
#define DASHBOARD_CONTROL_FRAME_ID (993)
#define SAFE_STATE_FRAME_ID (0) // TODO: assign real safe state frame id
#define TURN_SIGNAL_LEFT_BITPOS (5)
#define TURN_SIGNAL_RIGHT_BITPOS (6)
#define EMERGENCY_BITPOS (7)
#define LIGHT_SWITCH_INTPOS (0)
#define BREAKS_LINEAR_INTPOS (1)
#define BREAKS_HALL_INTPOS (2)
#define PRND_BITPOS (14)
#define PRND_LENGTH (8)
#define PRND_REVERSE_VALUE (1)
#define DIRECTION_PERIOD (1000)
#define DIRECTION_HIGH (500)
#define BREAK_HALL_EPS (100)
#define CAN_STD_ID(id) ((uint16_t)((id) << 5))

// Shared light state, written while interpreting CAN frames and read by the board services.
extern volatile bool brakeStatus;
extern volatile bool positionStatus;
extern volatile bool lowBeamStatus;
extern volatile bool highBeamStatus;
extern volatile bool rightTurnStatus;
extern volatile bool leftTurnStatus;
extern volatile bool safeStateStatus;
extern volatile bool emergencyStatus;
extern volatile bool reverseStatus;

extern volatile bool brakeChangeFlag;
extern volatile bool positionChangeFlag;
extern volatile bool lowBeamChangeFlag;
extern volatile bool highBeamChangeFlag;
extern volatile bool reverseChangeFlag;
extern volatile bool indicatorChangeFlag;

/**
  * @brief Decode any pending CAN frames into the shared light state.
  */
void APP_InterpretFrames(void);

/**
  * @brief Application entry point, called once from main().
  */
void APP_Main(void);

#endif // APP_H
