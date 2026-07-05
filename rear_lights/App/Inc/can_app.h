/**
  * @file can_app.h
  * @brief Application-level CAN frame IDs, shared light state, and interpret/filter entry points.
  * @author AGH EKO-ENERGIA
  * @author Andrzej Gondek
  */

#ifndef CAN_APP_H
#define CAN_APP_H

#include "main.h"
#include <stdbool.h>

// Frame IDs come from CAN_DB.dbc.
#define DASHBOARD_LIGHTS_FRAME_ID (994)
#define DASHBOARD_CONTROL_FRAME_ID (993)
#define PEDALS_JTNS_WORKS_FRAME_ID (65)
#define SAFE_STATE_FRAME_ID (1)

#define BREAKS_LINEAR_INTPOS (1)
#define BREAKS_HALL_INTPOS (2)
#define BREAK_HALL_EPS (2)

// PRND enum: 0 P, 1 R, 2 N, 3 D.
#define PRND_REVERSE_VALUE (1)

#define CAN_STD_ID(id) ((uint16_t)((id) << 5))

extern volatile bool brakeStatus;
extern volatile bool reverseStatus;

extern volatile bool brakeChangeFlag;
extern volatile bool reverseChangeFlag;

extern volatile bool safeStateDataCheck;

void APP_InterpretFrames(void);
void SetCanFilters(void);

#endif // CAN_APP_H
