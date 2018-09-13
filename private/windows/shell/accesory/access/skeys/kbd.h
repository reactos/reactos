/* KBD.H  */



void sendPressList(void);
void sendCombineList(void);
void sendExtendedKey(unsigned char cKeyCode);
void sendDownKeyCode(unsigned char cKeyCode);
void sendUpKeyCode(unsigned char cKeyCode);

int inLockList(unsigned char searchChar);
int inHoldList(unsigned char searchChar);
int inTempList(unsigned char searchChar);

void releaseKeysFromHoldList(void);
void removeKeyFromHoldList(unsigned char cTheKey);
void releaseKeysFromLockList(void);

void processKbdIndicator(unsigned char cGideiCode);
void processKbdVersion(unsigned char cGideiCode);
void processKbdDescription(unsigned char cGideiCode);
void processKbdModel(unsigned char cGideiCode);

void processKbdRel(unsigned char cGideiCode);
void processKbdLock(unsigned char cGideiCode);
void processKbdHold(unsigned char cGideiCode);
void processKbdCombine(unsigned char cGideiCode);
void processKbdPress(unsigned char cGideiCode);
void processKbd(unsigned char cGideiCode);

unsigned char xlateNumToScanCode(unsigned char Value);

