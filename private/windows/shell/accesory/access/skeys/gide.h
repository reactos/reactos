/*  GIDE.H   */

#include "gidei.h"

int pushCommandVector(void);
int popCommandVector(void);
int restoreCommandVector(void);

void noOpRoutine(unsigned char cJunk);

int storebyte(unsigned char *bytePtr);
int retrievebyte(unsigned char *bytePtr);

void processComm(unsigned char);
void processGen(unsigned char);
void processEnd(unsigned char iGideiCode);
void processBytes(unsigned char iGideiCode);
void processBlock(unsigned char iGideiCode);
void processGideiBlockCount(unsigned char iGideiCode);
void processClear(unsigned char iGideiCode);
void processGideiCode(unsigned char iGideiCode);

void executeAlias(void);
void processAlias(unsigned char ucSerialByte);
short	writeCommPort (char *outStr);

void passAllCodes(unsigned char cGideiCode);
void determineFormat(unsigned char ucSerialByte);
void charHandler(unsigned char ucSerialByte);
void processCharMode(unsigned char ucSerialByte);
void processCommand(unsigned char cGideiCode);
void	processBaudrate(unsigned char Code);

BOOL FAR serialKeysBegin(unsigned char c);

void handleErrorReport(void);
void handleFatalError(void);
