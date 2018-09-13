/* KBD.C  */

//#define     WINVER 0x0300

#include	<string.h>
#include	<stdlib.h>
#include	"windows.h"
//#include "winstric.h"                        /* added for win 3.1 compatibility 1/92 */

#include	"gidei.h"
#include	"vars.h"
#include	"gide.h"
#include	"kbd.h"
#include	"tables.h"
#include	"sk_ex.h"

BOOL IsInBuff(char *buf, unsigned char SerchChar, int Len);


void sendDownKeyCode(unsigned char cKeyCode)
{
	int 	scanCode;

	if (cKeyCode == NOKEY)
		return;

	if ((scanCode = IBMextendedScanCodeSet1[cKeyCode]) == 0) 
		return;

	SkEx_SendKeyDown(scanCode);
}


void sendUpKeyCode(unsigned char cKeyCode)
{
	int		scanCode;

	if (cKeyCode == NOKEY)
		return;

	if ((scanCode = IBMextendedScanCodeSet1[cKeyCode]) == 0) 
		return;

	SkEx_SendKeyUp(scanCode);
}

void sendExtendedKey(unsigned char cKeyCode)
{
	unsigned char Key[4], Tmp;

	// Start with the Alt Key
	Key[0] = ralt_key;

	Tmp = cKeyCode/10;					// Calc One's
	Key[3] = cKeyCode - (Tmp * 10);		

	cKeyCode = Tmp;						// Calc Ten's
	Tmp /= 10;
	Key[2] = cKeyCode - (Tmp * 10);		
	Key[1] = Tmp;						// Calc Hundreds

	// Translate Numbers into ScanCodes
	Key[1] = xlateNumToScanCode(Key[1]);
	Key[2] = xlateNumToScanCode(Key[2]);
	Key[3] = xlateNumToScanCode(Key[3]);

	// Send Keys to Host
	sendDownKeyCode(Key[0]);		// Send Alt Key Down
	sendDownKeyCode(Key[1]);		// Send Hundreds Key Down
	sendDownKeyCode(Key[2]);		// Send Tens Key Down
	sendDownKeyCode(Key[3]);		// Send Ones Key Down
	sendUpKeyCode(Key[3]);
	sendUpKeyCode(Key[2]);
	sendUpKeyCode(Key[1]);
	sendUpKeyCode(Key[0]);
}

unsigned char xlateNumToScanCode(unsigned char Value)
{
	switch (Value)
	{
		case 0:	return(kp0_key);
		case 1:	return(kp1_key);
		case 2: return(kp2_key);
		case 3:	return(kp3_key);
		case 4:	return(kp4_key);
		case 5:	return(kp5_key);
		case 6:	return(kp6_key);
		case 7:	return(kp7_key);
		case 8:	return(kp8_key);
		case 9:	return(kp9_key);
	}

    // should never be reached
    return 0;
}


void sendPressList(void)
{
	int i;

	for (i=0; i < keyHoldList.len; sendDownKeyCode(keyHoldList.list[i++]));
	for (i=0; i < tempList.len; i++) {
		sendDownKeyCode(tempList.list[i]);
		sendUpKeyCode(tempList.list[i]);
		}
	for (i=keyHoldList.len; i > 0; sendUpKeyCode(keyHoldList.list[--i]));
	keyHoldList.len = tempList.len = 0;
	return;
}

void sendCombineList(void)
{
	int i;

	for (i=0; i < keyHoldList.len; sendDownKeyCode(keyHoldList.list[i++]));
	for (i=0; i < tempList.len; sendDownKeyCode(tempList.list[i++]));
	for (i=tempList.len; i > 0; sendUpKeyCode(tempList.list[--i]));
	for (i=keyHoldList.len; i > 0; sendUpKeyCode(keyHoldList.list[--i]));
	keyHoldList.len = tempList.len = 0;
	return;
}

int inLockList(unsigned char searchChar)
{
	return(IsInBuff(keyLockList.list,searchChar,keyLockList.len));
}

int inHoldList(unsigned char searchChar)
{
	return(IsInBuff(keyHoldList.list,searchChar,keyHoldList.len));
}

int inTempList(unsigned char searchChar)
{
	return(IsInBuff(tempList.list,searchChar,tempList.len));
}

BOOL IsInBuff(char *buf, unsigned char SearchChar, int Len)
{
	int x = 0;

	if (!Len)					// Are there any Chars to search?
		return(FALSE);			// No - Return False
		
	while (x < Len)				// Loop until num chars reached
	{
		if (*buf == SearchChar)	// Does buffer and search char match?
			return(TRUE);		// Yes - Return found it.

		buf++;					// Inc buffer;
		x++;					// Inc byte count
	}
 	return(FALSE);				// character not found in buffer
}


void releaseKeysFromHoldList(void)
{
	unsigned char cTemp;
	int i,j,k;

	if (tempList.len) {
		k = 0;
		for (i=0; (i<tempList.len) && ((cTemp = tempList.list[i]) != DEFAULTCODE); i++) {
			for (j=0; j < keyHoldList.len; j++)
				if ((keyHoldList.list[k] = keyHoldList.list[j]) != cTemp) k++;
			keyHoldList.len = k;
			}
		if (tempList.list[i] == DEFAULTCODE) keyHoldList.len = 0;
		}
	return;
}

void removeKeyFromHoldList(unsigned char cTheKey)
{
//	unsigned char cTemp;
	int j,k;

	if (cTheKey != NOKEY) {
		k = 0;
		for (j=0; j < keyHoldList.len; j++)
			if ((keyHoldList.list[k] = keyHoldList.list[j]) != cTheKey) k++;
		keyHoldList.len = k;
		}
	return;
}

void releaseKeysFromLockList(void)
{
	int i,j,k;
	unsigned char cTemp;

	if (tempList.len) {
		k = 0;
		for (i=0; (i<tempList.len) && ((cTemp = tempList.list[i]) != DEFAULTCODE); i++) {
			for (j=0; j < keyLockList.len; j++) {
				if ((keyLockList.list[k] = keyLockList.list[j]) != cTemp) k++;
				else sendUpKeyCode(cTemp);
				}
			keyHoldList.len = k;
			}
		if (tempList.list[i] == DEFAULTCODE) {
			for (i=0; i < keyLockList.len; i++) sendUpKeyCode(keyLockList.list[i]);
			tempList.len = keyLockList.len = 0;
			}
		}
	return;
}



void processKbdIndicator(unsigned char cGideiCode)
{
	return;
}

void processKbdVersion(unsigned char cGideiCode)
{
	return;
}

void processKbdDescription(unsigned char cGideiCode)
{
	return;
}

void processKbdUnknown(unsigned char cGideiCode)
{
	return;
}


void processKbdModel(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case TERMCODE:
			break;
		default:
			break;
		}
	return;
}


void processKbdRel(unsigned char cGideiCode)
{
	unsigned char iKeyNumber;

	switch (cGideiCode)
		{
		case TERMCODE:
			if (!tempList.len)
				{
				tempList.list[0] = DEFAULTCODE;
				++tempList.len;
				}
			releaseKeysFromLockList();
			releaseKeysFromHoldList();
			tempList.len = 0;
			break;

		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			tempList.len = 0;
			beginOK = TRUE;
			break;

		default:
			if ((cGideiCode >= LOWESTGIDEICODE) && (cGideiCode != DEFAULTCODE))
				{
				handleFatalError();
				break;
				}
			if (tempList.len >= MAXLISTLENGTH)
				{
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if (cGideiCode == DEFAULTCODE)
				iKeyNumber = DEFAULTCODE;
			else
				{
				if ((iKeyNumber=cGideiCode) == NOKEY)
					{
					handleErrorReport();
					commandVector = noOpRoutine;
					tempList.len = 0;
					break;
					}
				if ((inLockList(iKeyNumber)) || (inHoldList(iKeyNumber)))
					iKeyNumber = NOKEY;
				}
			if (!inTempList(iKeyNumber)) tempList.list[tempList.len++]	= iKeyNumber;
			beginOK = FALSE;
			break;
		}
	return;
}

void processKbdLock(unsigned char cGideiCode)
{
	int i;
	unsigned char iKeyNumber;
	unsigned char temp;

	switch (cGideiCode) {
		case TERMCODE:
			for (i=0; i < tempList.len; i++) {
				if ((temp = tempList.list[i]) != NOKEY) {
					keyLockList.list[keyLockList.len++] = temp;
					sendDownKeyCode(temp);
					if (inHoldList(temp)) removeKeyFromHoldList(temp);
					}
				}
			if (tempList.len == 0) handleErrorReport();
			tempList.len = 0;
			break;
		
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			tempList.len = 0;
			beginOK = TRUE;
			break;

		default:
			if (cGideiCode >= (int)LOWESTGIDEICODE) {
				handleFatalError();
				break;
				}
			if ((keyLockList.len + tempList.len) >= MAXLISTLENGTH) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((iKeyNumber=cGideiCode) == NOKEY) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if (inLockList(iKeyNumber)) iKeyNumber = NOKEY;
			if (!inTempList(iKeyNumber)) tempList.list[tempList.len++]	= iKeyNumber;
			beginOK = FALSE;
			break;
		}
}


void processKbdHold(unsigned char cGideiCode)
{
	int i;
	unsigned char iKeyNumber;

	switch (cGideiCode) {
		case TERMCODE:
			for (i=0; i < tempList.len; i++)
				if ((keyHoldList.list[keyHoldList.len] = tempList.list[i]) != NOKEY)
					++(keyHoldList.len);
			if (tempList.len == 0) handleErrorReport();
			tempList.len = 0;
			break;
		
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			tempList.len = 0;
			beginOK = TRUE;
			break;

		default:
			if (cGideiCode >= (int)LOWESTGIDEICODE) {
				handleFatalError();
				break;
				}
			if ((keyHoldList.len + tempList.len) >= MAXLISTLENGTH) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((iKeyNumber=cGideiCode) == NOKEY) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((inLockList(iKeyNumber)) || (inHoldList(iKeyNumber))) iKeyNumber = NOKEY;
			if (!inTempList(iKeyNumber)) tempList.list[tempList.len++]	= iKeyNumber;
			beginOK = FALSE;
			break;
		}
}

void processKbdCombine(unsigned char cGideiCode)
{
	unsigned char iKeyNumber;

	switch (cGideiCode) {
		case TERMCODE:
			sendCombineList();
			keyHoldList.len = tempList.len = 0;
			break;
		
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			tempList.len = 0;
			beginOK = TRUE;
			break;

		default:
			if (cGideiCode >= LOWESTGIDEICODE) {
				handleFatalError();
				break;
				}
			if (tempList.len >= MAXLISTLENGTH) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((iKeyNumber=cGideiCode) == NOKEY) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((inLockList(iKeyNumber)) || (inHoldList(iKeyNumber))) iKeyNumber = NOKEY;
			if (!inTempList(iKeyNumber)) tempList.list[tempList.len++] = iKeyNumber;
			beginOK = FALSE;
			break;
		}
}

void processKbdPress(unsigned char cGideiCode)
{
	unsigned char iKeyNumber;

	switch (cGideiCode) {
		case TERMCODE:
			sendPressList();
			keyHoldList.len = tempList.len = 0;
			break;
		
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			tempList.len = 0;
			beginOK = TRUE;
			break;

		default:
			if (cGideiCode >= LOWESTGIDEICODE) {
				handleFatalError();
				break;
				}
			if (tempList.len >= MAXLISTLENGTH) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((iKeyNumber=cGideiCode) == NOKEY) {
				handleErrorReport();
				commandVector = noOpRoutine;
				tempList.len = 0;
				break;
				}
			if ((inLockList(iKeyNumber)) || (inHoldList(iKeyNumber))) iKeyNumber = NOKEY;
			tempList.list[tempList.len++] = iKeyNumber;
			beginOK = FALSE;
			break;
		}
}



void processKbd(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case KBDINDICATORCODE:
			commandVector = processKbdIndicator;
			aliasPtr = kbdIndicatorAliasTable;
			beginOK = TRUE;
			break;

		case KBDVERSIONCODE:
			commandVector = processKbdVersion;
			aliasPtr = kbdVersionAliasTable;
			beginOK = TRUE;
			break;

		case KBDMODELCODE:
			commandVector = processKbdModel;
			aliasPtr = kbdModelAliasTable;
			beginOK = TRUE;
			break;

		case KBDDESCRIPTIONCODE:
			commandVector = processKbdDescription;
			aliasPtr = kbdDescriptionAliasTable;
			beginOK = TRUE;
			break;

/*		case KBDUNKNOWNCODE:
			commandVector = processKbdUnknown;
			aliasPtr = kbdUnknownAliasTable;
			beginOK = TRUE;
			break;
*/
		default:
			if (cGideiCode < LOWESTGIDEICODE) handleFatalError();
			else {
				handleErrorReport();
				commandVector = noOpRoutine;
				beginOK = TRUE;
				}
			break;
		}
	return;
}
