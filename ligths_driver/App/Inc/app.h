#include "main.h"
#include <stdbool.h>

#define Dashboard_SwitchesFrameID 995
#define Dashboard_ImportantFrameID 994
#define Pedals_JTNS_WORKSFrameID 65
#define Dashboard_ControlFrameID 993
#define SafeStateFrameID  0 // todo
#define Turn_Signal_LeftBitpos 5
#define Turn_Signal_RightBitpos 6
#define EmergencyBitpos 7
#define Light_SwitchIntpos 0
#define BreaksLinearIntpos 1
#define BreaksHallIntpos 2
#define PRND_BITPOS 14
#define PRND_LENGTH 8
#define PRND_REVERSE_VALUE 1
#define directionPeriod 571
#define directionHigh 150
#define BreakHallEps 100
extern TIM_HandleTypeDef htim1;

void app_main();



