/*  MOU.H  */


void processMouClick(unsigned char cGideiCode);
void processMou(unsigned char cGideiCode);
void processMouDoubleClick(unsigned char cGideiCode);
void processMouRel(unsigned char cGideiCode);

void processMouReset(unsigned char cGideiCode);
void processMouAnchor(unsigned char cGideiCode);
//void processMouPin(unsigned char cGideiCode);
void processMouGoto(unsigned char cGideiCode);
void processMouLock(unsigned char cGideiCode);
void processMouMove(unsigned char cGideiCode);
void pressMouseButtonUp();
void pressMouseButtonDown();
void moveTheMouseAbsolute(void);
void moveTheMouseRelative(void);
void collectGotoInteger(unsigned char moveByte);
void collectGotoByte(unsigned char moveByte);
void collectMoveInteger(unsigned char moveByte);
void collectMoveByte(unsigned char moveByte);

