/* GIDE.C  */

//#define     WINVER 0x0300
#define     USECOMM                     /* added to be compatible with new windows.h (12/91) and wintric.h */
                                          /* last rellease of 3.1 SDK switched back to using NOCOMM in windows.h */

#include	<string.h>
#include	<stdlib.h>
#include	"windows.h"
//#include "winstric.h"                        /* added for win 3.1 compatibility 1/92 */
#include "gidei.h"
#include "vars.h"
#include "gide.h"
#include "kbd.h"
#include "mou.h"
#include "tables.h"
#include "dialogs.h"
#include "sk_ex.h"


typedef	struct	tagAliasTable {
	char	*Alias;
	BYTE	Code;
} ALIASTABLE;

long 	AtoL(char *Str);

extern	void	initClear(void);

BOOL	bGIDEIokay = TRUE;				/* general flag for error in processing */
int   	nFrameCnt = 0;					/* keep track of framming errors	*/

char cInBuffer[2];						/* buffer for receiving characters	*/

//char	cInBuffer[7];					/* buffer for receiving characters, size increased */
//short	cInBuffer_count =0;			/* count of chars. returned in ReadComm */
//int		intct = 0;					/* counter for looping thru cInBuffer_count */

//char	cOutStr[2] = {0,0};				/* single char output string		*/


void handleFatalError(void)
{
	SkEx_SendBeep();
	SkEx_SendBeep();
	initClear();
	return;
}

void handleErrorReport(void)
{
	SkEx_SendBeep();
	initClear();
	return;
}

int aliasForGideiCode(unsigned char *cTempPtr)
{
	struct aliasTable *tablePtr;
	int found;

	found = FALSE;
	for (tablePtr=gideiAliasTable;(tablePtr->aliasName[0] != '\x0') && (!found);tablePtr++)
 		if (strcmp(cAliasString,tablePtr->aliasName) == 0) {
			found = TRUE;
			*cTempPtr = tablePtr->gideiCode;
			}
	return (found);
}


int aliasUsedInStandard(unsigned char *cTempPtr)
{
	struct aliasTable *tablePtr;
	int found;
	unsigned char iCode;

	if (aliasPtr == keyAliasTable) {
		if (lstrlenA(cAliasString) == 1) {
			/* use ASCII table */
			if ((iCode = asciiTable[cAliasString[0]].gideiCode1) == NOCODE) return(FALSE);
			if ((iCode == control_Code) || (iCode == shift_Code))
				iCode = asciiTable[cAliasString[0]].gideiCode2;
			*cTempPtr = iCode;
			return(TRUE);
			}
		}
	found = FALSE;
	for (tablePtr=aliasPtr;(tablePtr->aliasName[0] != '\x0') && (!found);tablePtr++)
		if (lstrcmpA(cAliasString,tablePtr->aliasName) == 0) {
			found = TRUE;
			*cTempPtr = tablePtr->gideiCode;
			}
	return (found);
}



/****************************************************************************

	FUNCTION:	pushCommandVector

	PURPOSE:	push CommandVector on to vectorStack

	COMMENTS:
****************************************************************************/

int pushCommandVector(void)
{
	if (stackPointer < MAXVECTORSTACK) {
		aliasStack[stackPointer] = aliasPtr;
		vectorStack[stackPointer++] = commandVector;
		return(TRUE);
		}
	else return(FALSE);
}

/****************************************************************************

	FUNCTION:	popCommandVector

	PURPOSE:	pop CommandVector from vectorStack

	COMMENTS:

*****************************************************************************/

int popCommandVector(void)
{
	if (stackPointer > 0) {
		aliasPtr = aliasStack[--stackPointer];
		commandVector = vectorStack[stackPointer];
		return(TRUE);
		}
	else return(FALSE);
}

/****************************************************************************

	FUNCTION:	restoreCommandVector

	PURPOSE:	restore CommandVector from vectorStack but does not update
				stack pointer.

	COMMENTS:

*****************************************************************************/

int restoreCommandVector(void)
{
	if (stackPointer > 0) {
		aliasPtr = aliasStack[--stackPointer];
		commandVector = vectorStack[stackPointer];
		++stackPointer;
		return(TRUE);
		}
	else return(FALSE);
}

/****************************************************************************/
int storeByte(unsigned char *bytePtr)
{
	if ((spos+1==rpos) || (spos+1==CODEBUFFERLEN && !rpos)) return notOKstatus;
	buf[spos++] = *bytePtr;
	if (spos==CODEBUFFERLEN) spos = 0;
	return okStatus;
}

int retrieveByte(unsigned char *bytePtr)
{
	if (rpos==CODEBUFFERLEN) rpos = 0;
	if (rpos==spos) return notOKstatus;
	++rpos;
	*bytePtr = buf[rpos-1];
	return okStatus;
}


/****************************************************************************

	FUNCTION:	noOpRoutine

	PURPOSE:	"Do nothing" routine

	COMMENTS:

****************************************************************************/
void noOpRoutine(unsigned char cJunk)
{
	return;
}

void processGen(unsigned char c)
{
	return;
}

void processComm(unsigned char c)
{
	return;
}
/****************************************************************************

	FUNCTION:	processCommand

	PURPOSE:	Determine which command is active.  Then set commandVector to
				point to appropriate routine.

	COMMENTS:

****************************************************************************/
void processCommand(unsigned char cGideiCode)
{
	switch(cGideiCode) {
		case KBDEXPANSIONCODE:
			commandVector = processKbd;
			aliasPtr = kbdAliasTable;
			beginOK = TRUE;
			break;
		case MOUEXPANSIONCODE:
			commandVector = processMou;
			aliasPtr = mouseAliasTable;
			beginOK = TRUE;
			break;
		case GENCODE:
			commandVector = processGen;
			aliasPtr = genAliasTable;
			beginOK = TRUE;
			break;
		case COMMCODE:
			commandVector = processComm;
			aliasPtr = commAliasTable;
			beginOK = TRUE;
			break;
		case KBDLOCKCODE:
			commandVector = processKbdLock;
			aliasPtr = keyAliasTable;
			beginOK = TRUE;
			break;
		case KBDRELCODE:
			commandVector = processKbdRel;
			aliasPtr = keyAliasTable;
			beginOK = TRUE;
			break;
		case KBDPRESSCODE:
			commandVector = processKbdPress;
			aliasPtr = keyAliasTable;
			beginOK = TRUE;
			break;
		case KBDCOMBINECODE:
			commandVector = processKbdCombine;
			aliasPtr = keyAliasTable;
			beginOK = TRUE;
			break;
		case KBDHOLDCODE:
			commandVector = processKbdHold;
			aliasPtr = keyAliasTable;
			beginOK = TRUE;
			break;
		case MOULOCKCODE:
			commandVector = processMouLock;
			aliasPtr = mouButtonAliasTable;
			beginOK = TRUE;
			break;
		case MOURELCODE:
			commandVector = processMouRel;
			aliasPtr = mouButtonAliasTable;
			beginOK = TRUE;
			break;
		case MOUCLICKCODE:
			commandVector = processMouClick;
			aliasPtr = mouButtonAliasTable;
			beginOK = TRUE;
			break;
		case MOUDOUBLECLICKCODE:
			commandVector = processMouDoubleClick;
			aliasPtr = mouButtonAliasTable;
			beginOK = TRUE;
			break;
		case MOUMOVECODE:
			commandVector = processMouMove;
			aliasPtr = nullTable;
			beginOK = TRUE;
			break;
		case MOUGOTOCODE:
			commandVector = processMouGoto;
			aliasPtr = nullTable;
			beginOK = TRUE;
			break;
		case MOURESETCODE:
			commandVector = processMouReset;
			aliasPtr = nullTable;
			beginOK = TRUE;
			break;
		case MOUANCHORCODE:
			commandVector = processMouAnchor;
			aliasPtr = nullTable;
			beginOK = TRUE;
			break;
		case BAUDRATECODE:
			commandVector = processBaudrate;
			aliasPtr = baudrateAliasTable;
			beginOK = TRUE;
			break;
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			beginOK = TRUE;
		default:
			if (cGideiCode >= LOWESTGIDEICODE) handleFatalError();
			else {
				handleErrorReport();
				commandVector = noOpRoutine;
				beginOK = TRUE;
				}
			break;
		}
	return;
}



void processBytes(unsigned char iGideiCode)
{
	(*commandVector)(iGideiCode);
	if (!(--blockCount))
      {
      passAll = FALSE;
      codeVector = processGideiCode;
      }
}

void processBlock(unsigned char iGideiCode)
{
	if (blockCount--) (*commandVector)(iGideiCode);
	else {
      passAll = FALSE;
		if (iGideiCode == TERMCODE) codeVector = processGideiCode;
		else handleFatalError();
		}
}

void processGideiBlockCount(unsigned char iGideiCode)
{
	blockCount = iGideiCode;
	codeVector = processBlock;
   passAll = TRUE;
}

void processGideiClear(unsigned char iGideiCode)
{
	if (iGideiCode == TERMCODE) initClear();
	else handleFatalError();
}

void processGideiEnd(unsigned char iGideiCode)
{
	if (iGideiCode == TERMCODE) {
		if (!popCommandVector()) handleFatalError();
		else {
			if (restoreCommandVector()) {
				beginOK = TRUE;
				codeVector = processGideiCode;
				}
			else {
				commandVector = processCommand;
				codeVector = processCharMode;
				serialVector = charHandler;
				beginOK = FALSE;
				}
			lastCode = iGideiCode;
			}
		}
	else handleFatalError();
}


/****************************************************************************

	FUNCTION:	processCOMMbaudrate(Code)

	PURPOSE:	Processes the baudrate commands.

	COMMENTS:

****************************************************************************/

void processBaudrate(unsigned char Code)
{
	static int SetBaud = 0;

	switch(Code)
	{
		case TERMCODE:
			if (SetBaud != 0) 			/* valid one set */
				SkEx_SendBeep();
			break;

		case BAUD300CODE:
			SetBaud = ID_BAUD_300;
			SkEx_SetBaud(300);
			break;

		case BAUD600CODE:
			SetBaud = ID_BAUD_600;
			SkEx_SetBaud(600);
			break;

		case BAUD1200CODE:
			SetBaud = ID_BAUD_1200;
			SkEx_SetBaud(1200);
			break;

		case BAUD2400CODE:
			SetBaud = ID_BAUD_2400;
			SkEx_SetBaud(2400);
			break;

		case BAUD4800CODE:
			SetBaud = ID_BAUD_4800;
			SkEx_SetBaud(4800);
			break;

		case BAUD9600CODE:
			SetBaud = ID_BAUD_9600;
			SkEx_SetBaud(9600);
			break;

		case BAUD19200CODE:
			SetBaud = ID_BAUD_19200;
			SkEx_SetBaud(19200);
			break;

		default:
			handleErrorReport();
			break;
	}
}

/****************************************************************************

	FUNCTION:	processGideiCode

	PURPOSE:
				

	COMMENTS:

*****************************************************************************/

void processGideiCode(unsigned char iGideiCode)
{
	if (waitForClear) {
		if (iGideiCode == CLEARCODE) codeVector = processGideiClear;
		else handleFatalError();
		return;
		}
	switch (iGideiCode) {

		case BEGINCODE:
			if (beginOK) {
				if (pushCommandVector()) lastCode = iGideiCode;
				else handleFatalError();
				}
			else handleFatalError();
			break;

		case ENDCODE:
			if (lastCode == TERMCODE) {
				codeVector = processGideiEnd;
				beginOK = FALSE;
				lastCode = iGideiCode;
				}
			else handleFatalError();
			break;

		case CLEARCODE:
			codeVector = processGideiClear;
			lastCode = iGideiCode;
			break;

		case TERMCODE:
			(*commandVector)(iGideiCode);
			if (!restoreCommandVector()) {
				commandVector = processCommand;
				codeVector = processCharMode;
				serialVector = charHandler;
				beginOK = FALSE;
				}
			else
				beginOK = TRUE;
			lastCode = iGideiCode;
			break;

		case BLKTRANSCODE:
			codeVector = processGideiBlockCount;
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;

		case BYTECODE:
			codeVector = processBytes;
			blockCount = 1;
			passAll = TRUE;
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;

		case INTEGERCODE:
			codeVector = processBytes;
			blockCount = 2;
			passAll = TRUE;
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;

		case LONGCODE:
			codeVector = processBytes;
			blockCount = 4;
			passAll = TRUE;
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;

		case DOUBLECODE:
			codeVector = processBytes;
			blockCount = 8;
			passAll = TRUE;
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;

		default:
			(*commandVector)(iGideiCode);
			lastCode = iGideiCode;
			break;
		}

}


/****************************************************************************

	FUNCTION:	processCharMode

	PURPOSE:	Handles processing of ASCII characters in Character Mode
				

	COMMENTS:

***************************************************************************/
void processCharMode(unsigned char ucSerialByte)
{
	unsigned char tempKeyCode;

	if (ucSerialByte == ESCAPE) {
		codeVector = processGideiCode;
		return;
		}

	if (waitForClear) {
		handleFatalError();
		return;
		}

	if ( ucSerialByte > 127 )			// Are We processing an Extended Code
	{
		sendExtendedKey(ucSerialByte);	// Yes - Send Code
		return;							// Exit
	}

	if ((tempKeyCode = (asciiTable[ucSerialByte]).gideiCode1) == NOCODE) {
		handleErrorReport();
		tempList.len = 0;
		return;
		}

	if ((!inLockList(tempKeyCode)) && (!inHoldList(tempKeyCode)))
		tempList.list[tempList.len++] = tempKeyCode;

	if ((tempKeyCode = asciiTable[ucSerialByte].gideiCode2) != NOCODE) {
		if ((!inLockList(tempKeyCode)) && (!inHoldList(tempKeyCode)))
			tempList.list[tempList.len++] = tempKeyCode;
		}

	sendCombineList();
	keyHoldList.len = tempList.len = 0;
	return;
}



/****************************************************************************

	FUNCTION:	executeAlias()

	PURPOSE:	Takes the alias string, convert to code, and then does
				proper processing.

	COMMENTS:

*****************************************************************************/
void executeAlias(void)
{
	static unsigned char *cTempPtr;
	static int iTemp;

	cTempPtr = cAliasString;
	if (lstrlenA(cAliasString) > MAXALIASLEN) *cTempPtr = UNKNOWNCODE;
	else {
		if (!aliasForGideiCode(cTempPtr)) {
			CharLowerA(cAliasString);
			if (!aliasUsedInStandard(cTempPtr))
 					/* Must be a number.  But is it an ASCII coded number
 					or ASCII coded GIDEI code */
				switch (cAliasString[0]) {
					case '0':
					case '+':
					case '-':
						iTemp = AtoL(cAliasString);
						*cTempPtr = INTEGERCODE;
						storeByte(cTempPtr);
						cTempPtr = (unsigned char*) &iTemp;
						storeByte(cTempPtr++);
						break;
					default:
						/* must be a ASCII coded GIDEI code */
						iTemp = AtoL(cAliasString);
						if ((unsigned)iTemp > 255) *cTempPtr = UNKNOWNCODE;
						else *cTempPtr = (unsigned char) iTemp;
						break;
					}
			}
		}
	storeByte(cTempPtr);
	return;
}


/****************************************************************************

	FUNCTION:	processAlias(ucSerialByte)

	PURPOSE:	This routine builds up the alias string and then passes
				control onto executeAlias.

	COMMENTS:
*****************************************************************************/

void processAlias(unsigned char ucSerialByte)
{
	static unsigned char	tempCode, *codePtr;
	static unsigned char sbtemp[2];

	codePtr = &tempCode;

	switch (ucSerialByte) {
		case ESCAPE:
			cAliasString[0] = '\0';
			break;
		case TAB:
		case LINEFEED:
		case VERTICALTAB:
		case FORMFEED:
		case RETURN:
		case SPACE:
			if (!lstrlenA(cAliasString)) break;	/* if previous character was a */
				 								/* delimiter then eat white space */
		case COMMA:
		case PERIOD:
			if (lstrlenA(cAliasString)) executeAlias();
			else
				{
				tempCode = DEFAULTCODE;
				storeByte(codePtr);
				}
			if (ucSerialByte == '.')
				{
				tempCode = TERMCODE;
				storeByte(codePtr);
				}
			cAliasString[0] = '\0';
			for (;retrieveByte(codePtr);) (*codeVector)(tempCode);
			break;
		default:
			/* just add the char to the string */
			if ((ucSerialByte >= ' ') && (ucSerialByte <= '~'))
				{
				if (lstrlenA(cAliasString) < MAXALIASLEN+1)	/*make sure room */
					{
					sbtemp[0] = ucSerialByte;
					sbtemp[1] = 0;
					lstrcatA(cAliasString,sbtemp);
					}
				}
			else
				handleFatalError();					/* not an alias */
		}
	return;
}

/****************************************************************************

	FUNCTION:	passAllCodes

	PURPOSE:	Just keeps the GIDEI hierarchy consistant

	COMMENTS:	
****************************************************************************/

void passAllCodes(unsigned char cGideiCode)
{
	(*codeVector)(cGideiCode);
	return;
}



/****************************************************************************

	FUNCTION:	determineFormat

	PURPOSE:	Figure out what Escape Sequence form (i.e. Alias, Code, KEI, etc)

	COMMENTS:
****************************************************************************/

void determineFormat(unsigned char ucSerialByte)
{
	static char cStuffStr[7], *cPtr;

	switch (ucSerialByte)
		{
		case COMMA:
			serialVector = processAlias;
			aliasPtr = commandsAliasTable;
			break;
		case ESC:
			break;
		default:
			if ((ucSerialByte >= ' ') && (ucSerialByte <= '~')) /* KEI Implied Press */
				{
				serialVector = processAlias;
				aliasPtr = commandsAliasTable;
				for (cPtr = strcpy(cStuffStr,"press,"); *cPtr != '\0'; cPtr++) processAlias(*cPtr);
				processAlias(ucSerialByte);
				}
			else
				{
				serialVector = passAllCodes;
				(*codeVector)(ucSerialByte);
				}
			break;
		}
	return;
}

/****************************************************************************

	FUNCTION:	charHandler

	PURPOSE:	If ESCAPE then set up new vectors.  Also processes the char

	COMMENTS:
****************************************************************************/

void charHandler(unsigned char ucSerialByte)
{
	if (ucSerialByte == ESC) {
		serialVector = determineFormat;
		commandVector = processCommand;
		beginOK = TRUE;
		}
	(*codeVector)(ucSerialByte);
	return;
}


BOOL  serialKeysBegin(unsigned char c)
{
	static	char junk[2];
	
	junk[0] = c;
	junk[1] = '\0';

	if (!passAll) {
		if (c == '\0')
			{
			SkEx_SendBeep();
			if ((++nullCount) >= 3) {
				initClear();
				SkEx_SendBeep();
				SkEx_SendBeep();
				SkEx_SendBeep();

				}
			}
		}
	(*serialVector)(c);
	return(TRUE);
}


long AtoL(char *Str)
{
	int		x = strlen(Str);		// Calc Number of chars in String
	int		Mul = 1;				// Start with a Multipiler of 1
	int		ChVal = 0;				// Value of the current char
	long	RetVal = 0;

	while (x)
	{
		ChVal = Str[x] - '0';		// Calc the value of the Char
		RetVal += (ChVal * Mul);	// Calc value of the Return
		Mul *= 10;					// Increase the Multiplier by 10
		x--;						// Point to next char
	}

	return(RetVal);					// Return Value
	
}
