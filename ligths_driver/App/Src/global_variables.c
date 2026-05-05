#include "global_variables.h"

bool brakeStatus = false,
     positionStatus = false,
     lowBeamStatus = false,
     highBeamStatus = false,
     rightTurnStatus = false,
     leftTurnStatus = false,
     safeStateStatus = false,
	 emergencyStatus = false,
	 reverseStatus = false;

bool brakeChangeFlag = false,
     positionChangeFlag = false,
     lowBeamChangeFlag = false,
     highBeamChangeFlag = false,
	 reverseChangeFlag = false;

void setReverseStatusTrue(void)  { reverseStatus = true; }
void setReverseStatusFalse(void) { reverseStatus = false; }
bool getReverseStatus(void)      { return reverseStatus; }

void setEmergencyStatusTrue(void)  { emergencyStatus = true; }
void setEmergencyStatusFalse(void) { emergencyStatus = false; }
bool getEmergencyStatus(void)      { return emergencyStatus; }

void setBrakeStatusTrue(void)  { brakeStatus = true; }
void setBrakeStatusFalse(void) { brakeStatus = false; }
bool getBrakeStatus(void)      { return brakeStatus; }

void setPositionStatusTrue(void)  { positionStatus = true; }
void setPositionStatusFalse(void)  { positionStatus = false; }
bool getPositionStatus(void)       { return positionStatus; }

void setLowBeamStatusTrue(void)  { lowBeamStatus = true; }
void setLowBeamStatusFalse(void) { lowBeamStatus = false; }
bool getLowBeamStatus(void)      { return lowBeamStatus; }

void setHighBeamStatusTrue(void)  { highBeamStatus = true; }
void setHighBeamStatusFalse(void) { highBeamStatus = false; }
bool getHighBeamStatus(void)      { return highBeamStatus; }

void setRightTurnStatusTrue(void)  { rightTurnStatus = true; }
void setRightTurnStatusFalse(void) { rightTurnStatus = false; }
bool getRightTurnStatus(void)      { return rightTurnStatus; }

void setLeftTurnStatusTrue(void)  { leftTurnStatus = true; }
void setLeftTurnStatusFalse(void) { leftTurnStatus = false; }
bool getLeftTurnStatus(void)      { return leftTurnStatus; }

void setSafeStateStatusTrue(void)  { safeStateStatus = true; }
void setSafeStateStatusFalse(void) { safeStateStatus = false; }
bool getSafeStateStatus(void)      { return safeStateStatus; }

void setBrakeChangeFlagTrue(void)  { brakeChangeFlag = true; }
void setBrakeChangeFlagFalse(void) { brakeChangeFlag = false; }
bool getBrakeChangeFlag(void)      { return brakeChangeFlag; }

void setPositionChangeFlagTrue(void)  { positionChangeFlag = true; }
void setPositionChangeFlagFalse(void) { positionChangeFlag = false; }
bool getPositionChangeFlag(void)      { return positionChangeFlag; }

void setLowBeamChangeFlagTrue(void)  { lowBeamChangeFlag = true; }
void setLowBeamChangeFlagFalse(void) { lowBeamChangeFlag = false; }
bool getLowBeamChangeFlag(void)      { return lowBeamChangeFlag; }

void setHighBeamChangeFlagTrue(void)  { highBeamChangeFlag = true; }
void setHighBeamChangeFlagFalse(void) { highBeamChangeFlag = false; }
bool getHighBeamChangeFlag(void)      { return highBeamChangeFlag; }

void setReverseChangeFlagTrue(void)  { reverseChangeFlag = true; }
void setReverseChangeFlagFalse(void) { reverseChangeFlag = false; }
bool getReverseChangeFlag(void)      { return reverseChangeFlag; }
