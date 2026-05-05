#include <stdbool.h>

extern bool brakeStatus;
extern bool positionStatus;
extern bool lowBeamStatus;
extern bool highBeamStatus;
extern bool rightTurnStatus;
extern bool leftTurnStatus;
extern bool safeStateStatus;
extern bool emergencyStatus;
extern bool reverseStatus;

extern bool brakeChangeFlag;
extern bool positionChangeFlag;
extern bool lowBeamChangeFlag;
extern bool highBeamChangeFlag;
extern bool reverseChangeFlag;

void setReverseStatusTrue(void);
void setReverseStatusFalse(void);
bool getReverseStatus(void);

void setReverseChangeFlagTrue(void);
void setReverseChangeFlagFalse(void);
bool getReverseChangeFlag(void);

void setEmergencyStatusTrue(void);
void setEmergencyStatusFalse(void);
bool getEmergencyStatus(void);

void setBrakeStatusTrue(void);
void setBrakeStatusFalse(void);
bool getBrakeStatus(void);

void setPositionStatusTrue(void);
void setPositionStatusFalse(void);
bool getPositionStatus(void);

void setLowBeamStatusTrue(void);
void setLowBeamStatusFalse(void);
bool getLowBeamStatus(void);

void setHighBeamStatusTrue(void);
void setHighBeamStatusFalse(void);
bool getHighBeamStatus(void);

void setRightTurnStatusTrue(void);
void setRightTurnStatusFalse(void);
bool getRightTurnStatus(void);

void setLeftTurnStatusTrue(void);
void setLeftTurnStatusFalse(void);
bool getLeftTurnStatus(void);

void setSafeStateStatusTrue(void);
void setSafeStateStatusFalse(void);
bool getSafeStateStatus(void);

void setBrakeChangeFlagTrue(void);
void setBrakeChangeFlagFalse(void);
bool getBrakeChangeFlag(void);

void setPositionChangeFlagTrue(void);
void setPositionChangeFlagFalse(void);
bool getPositionChangeFlag(void);

void setLowBeamChangeFlagTrue(void);
void setLowBeamChangeFlagFalse(void);
bool getLowBeamChangeFlag(void);

void setHighBeamChangeFlagTrue(void);
void setHighBeamChangeFlagFalse(void);
bool getHighBeamChangeFlag(void);

void setRightTurnChangeFlagTrue(void);
void setRightTurnChangeFlagFalse(void);
bool getRightTurnChangeFlag(void);

void setLeftTurnChangeFlagTrue(void);
void setLeftTurnChangeFlagFalse(void);
bool getLeftTurnChangeFlag(void);
