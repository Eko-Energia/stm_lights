#include "main.h"
#include <stdbool.h>

#define Dashboard_SwitchesFrameID 995
#define Dashboard_ImportantFrameID 994
#define Pedals_JTNS_WORKSFrameID 65
#define EngineLeft_STATIC_TPDO1 422
#define EngineRight_STATIC_TPDO1 423
#define SafeStateFrameID  0 // todo
#define Turn_Signal_LeftBitpos 5
#define Turn_Signal_RightBitpos 6
#define EmergencyBitpos 7
#define Light_SwitchIntpos 0
#define BreaksLinearIntpos 1
#define BreaksHallIntpos 2
#define directionPeriod 571
#define directionHigh 150
#define BreakHallEps 100
extern TIM_HandleTypeDef htim1;

void app_main();



