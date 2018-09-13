/* MOU.C */

//#define     WINVER 0x0300

#include	<string.h>
#include	<stdlib.h>
#include	"windows.h"
//#include "winstric.h"                        /* added for win 3.1 compatibility 1/92 */
#include	"vars.h"
#include	"gide.h"
#include	"mou.h"
#include	"kbd.h"
#include	"sk_ex.h"				// Serial Keys Interface Routines

#if 0
#define	SF_MOVEMENT	0x0001
#define	SF_B1_DOWN	0x0002
#define	SF_B1_UP	0x0004
#define	SF_B2_DOWN	0x0008
#define	SF_B2_UP	0x0010
#define	SF_ABSOLUTE	0x8000
#else
#define	SF_MOVEMENT		MOUSEEVENTF_MOVE      
#define	SF_B1_DOWN 		MOUSEEVENTF_LEFTDOWN  
#define	SF_B1_UP 		MOUSEEVENTF_LEFTUP    
#define	SF_B2_DOWN 		MOUSEEVENTF_RIGHTDOWN 
#define	SF_B2_UP 		MOUSEEVENTF_RIGHTUP   
#define	SF_ABSOLUTE 	MOUSEEVENTF_ABSOLUTE  
#endif

void pressMouseButtonDown(void);
void pressMouseButtonUp(void);
void setTheMouseAbsolute(int PosX,int PosY);

BOOL  bmouseanchored =  FALSE;           /* flag, if false, mouse not pinned to a point */
POINT mouseanchor;

void SendMouseToQueue(MOUSEKEYSPARAM *p)
{
	SkEx_SendMouse(p);
}

void processMouReset(unsigned char cGideiCode)
{
	if (cGideiCode == TERMCODE) {

		mouData.Delta_Y = 0;
		mouData.Delta_X = 0;
		mouData.Status = SF_B1_UP | SF_B2_UP | SF_ABSOLUTE | SF_MOVEMENT;

		SendMouseToQueue(mouseDataPtr);
		mouseX = mouseY = 0;
		}
	else {
		handleErrorReport();
		commandVector = noOpRoutine;
		}
	return;
}

void moveTheMouseAbsolute(void)
{
 	short tempX, tempY;

	tempX = tempList.list[1];
	tempX = (tempX << 8) + tempList.list[0];
	tempY = tempList.list[3];
	tempY = (tempY << 8) + tempList.list[2];

	mouseX = tempX;
	mouseY = tempY;
	setTheMouseAbsolute(tempX, tempY);
	return;
}

void setTheMouseAbsolute(int PosX,int PosY)
{
	mouData.Status = SF_ABSOLUTE | SF_MOVEMENT;
	mouData.Delta_Y = (int) 0;
	mouData.Delta_X = (int) 0;
	SendMouseToQueue(mouseDataPtr);

	mouData.Status = SF_MOVEMENT;
	mouData.Delta_Y = (int) PosY;
	mouData.Delta_X = (int) PosX;
	SendMouseToQueue(mouseDataPtr);
	return;	
}

void moveTheMouseRelative(void)
{
	short xDist, yDist, newX, newY;

	xDist = tempList.list[1];
	xDist = (xDist << 8) + tempList.list[0];
	yDist = tempList.list[3];
	yDist = (yDist << 8) + tempList.list[2];
	newX = mouseX + xDist;
	newY = mouseY + yDist;
	if (newX < 0) newX = 0;
	if (newY < 0) newY = 0;
	mouseX = newX;
	mouseY = newY;
	mouData.Status = SF_MOVEMENT;
	mouData.Delta_Y = (int) yDist;
	mouData.Delta_X = (int) xDist;
	SendMouseToQueue(mouseDataPtr);
	return;
}


void processMouAnchor(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case TERMCODE:

//			if (tempList.len < 4)
// 				for ( ; tempList.len >= 4; tempList.list[tempList.len++] = 0);

         if (bmouseanchored) /* if true, need to release mouse */
            {
            bmouseanchored = FALSE;
			 if (SkEx_GetAnchor(&mouseanchor))
               {
               setTheMouseAbsolute(mouseanchor.x,mouseanchor.y);
               mouseX = mouseanchor.x;
               mouseY = mouseanchor.y;
               }
            else
               SkEx_SendBeep();  

            }
         else /* if false, need to pin the mouse */
            {
			 SkEx_SetAnchor();
            bmouseanchored = TRUE;
            }

			tempList.len = 0;
			beginOK = TRUE;
			break;
//		case INTEGERCODE:
//			commandVector = collectGotoInteger;
//			beginOK = FALSE;
//			break;
		default:
			handleFatalError();
			break;
		}
	return;
}

void collectGotoInteger(unsigned char moveByte)
{
	if (tempList.len >= 4) handleFatalError();
	else {
		tempList.list[tempList.len++] = moveByte;
		if ((tempList.len == 2) || (tempList.len == 4)) commandVector = processMouGoto;
		}
	return;
}

void collectGotoByte(unsigned char moveByte)
{
	if (tempList.len >= 4) handleFatalError();
	else {
		tempList.list[tempList.len++] = moveByte;
      if (moveByte >127) 
         tempList.list[tempList.len++] = 0xFF;
      else
   		tempList.list[tempList.len++] = 0;
		commandVector = processMouGoto;
		}
	return;
}

void processMouGoto(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case TERMCODE:
			if (tempList.len < 4)
				for ( ; tempList.len >= 4; tempList.list[tempList.len++] = 0);
			moveTheMouseAbsolute();
			tempList.len = 0;
			beginOK = TRUE;
			break;
		case BYTECODE:
			commandVector = collectGotoByte;
			beginOK = FALSE;
			break;
		case INTEGERCODE:
			commandVector = collectGotoInteger;
			beginOK = FALSE;
			break;
		default:
			handleFatalError();
			break;
		}
	return;
}

void collectMoveInteger(unsigned char moveByte)
{
	if (tempList.len >= 4) handleFatalError();
	else {
		tempList.list[tempList.len++] = moveByte;
		if ((tempList.len == 2) || (tempList.len == 4)) commandVector = processMouMove;
		}
	return;
}

void collectMoveByte(unsigned char moveByte)
{
	if (tempList.len >= 4) handleFatalError();
	else {
		tempList.list[tempList.len++] = moveByte;
      if (moveByte >127) 
         tempList.list[tempList.len++] = 0xFF;
      else
   		tempList.list[tempList.len++] = 0;
		commandVector = processMouMove;
		}
	return;
}

void processMouMove(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case TERMCODE:
			if (tempList.len < 4)
				for ( ; tempList.len >= 4; tempList.list[tempList.len++] = 0);
			moveTheMouseRelative();
			tempList.len = 0;
			beginOK = TRUE;
			break;
		case BYTECODE:
			commandVector = collectMoveByte;
			beginOK = FALSE;
			break;
		case INTEGERCODE:
			commandVector = collectMoveInteger;
			beginOK = FALSE;
			break;
		default:
			handleFatalError();
			break;
		}
	return;
}

void pressMouseButtonDown()
{
	mouData.Status = 0;
	if (requestButton1) {
		if (!button1Status) {
			mouData.Status += SF_B1_DOWN;
			button1Status = TRUE;
			}
		}
	if (requestButton2) {
		if (!button2Status) {
			mouData.Status += SF_B2_DOWN;
			button2Status = TRUE;
			}
		}
	if (mouData.Status != 0) SendMouseToQueue(mouseDataPtr);
	return;
}

void	pressMouseButtonUp()
{
	mouData.Status = 0;
	if (requestButton1) {
		if (button1Status) {
			mouData.Status += SF_B1_UP;
			button1Status = FALSE;
			}
		}
	if (requestButton2) {
		if (button2Status) {
			mouData.Status += SF_B2_UP;
			button2Status = FALSE;
			}
		}
	if (mouData.Status != 0) SendMouseToQueue(mouseDataPtr);
	return;
}

void processMouRel(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case TERMCODE:
			if ((!requestButton1) && (!requestButton2) && (!requestButton3))
				requestButton1 = requestButton2 = requestButton3 = TRUE;
			pressMouseButtonUp();
			requestButton1 = requestButton2 = requestButton3 = FALSE;
			beginOK = TRUE;
			break;
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			break;
		case DEFAULTCODE:
		case LEFTBUTTONCODE:
			requestButton1 = TRUE;
			beginOK = FALSE;
			break;
		case RIGHTBUTTONCODE:
			requestButton2 = TRUE;
			beginOK = FALSE;
			break;
		default:
			if (cGideiCode >= LOWESTGIDEICODE)
				{
				handleFatalError();
				break;
				}
			requestButton3 = TRUE;
			beginOK = FALSE;
			break;
		}
	return;
}

void processMouLock(unsigned char cGideiCode)
{
	switch (cGideiCode)
		{
		case TERMCODE:
			if ((!requestButton1) && (!requestButton2) && (!requestButton3))
				requestButton1 = TRUE;
			pressMouseButtonDown();
			requestButton1 = requestButton2 = requestButton3 = FALSE;
			beginOK = TRUE;
			break;
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			break;
		case DEFAULTCODE:
		case LEFTBUTTONCODE:
			requestButton1 = TRUE;
			beginOK = FALSE;
			break;
		case RIGHTBUTTONCODE:
			requestButton2 = TRUE;
			beginOK = FALSE;
			break;
		default:
			if (cGideiCode >= LOWESTGIDEICODE)
				{
				handleFatalError();
				break;
				}
			requestButton3 = TRUE;
			beginOK = FALSE;
			break;
		}
	return;
}

void processMouDoubleClick(unsigned char cGideiCode)
{
	if (cGideiCode == TERMCODE) {
		if ((!requestButton1) && (!requestButton2) && (!requestButton3 ))
			requestButton1 = TRUE;
		mouData.Status = 0;
		if ((requestButton1) && (button1Status)) {
			mouData.Status += SF_B1_UP;
			button1Status = FALSE;
			}
		if ((requestButton2) && (button2Status)) {
			mouData.Status += SF_B2_UP;
			button2Status = FALSE;
			}
		if (!mouData.Status) SendMouseToQueue(mouseDataPtr);
		pressMouseButtonDown();
		pressMouseButtonUp();
		pressMouseButtonDown();
		pressMouseButtonUp();
		requestButton1 = requestButton2 = requestButton3 = FALSE;
		beginOK = TRUE;
		}
	else
		processMouClick(cGideiCode);
	return;
}

void processMouClick(unsigned char cGideiCode)
{
	switch (cGideiCode)
		{
		case TERMCODE:
			if ((!requestButton1) && (!requestButton2) && (!requestButton3 ))
				requestButton1 = TRUE;
			mouData.Status = 0;
			if ((requestButton1) && (button1Status)) {
				mouData.Status += SF_B1_UP;
				button1Status = FALSE;
				}
			if ((requestButton2) && (button2Status)) {
				mouData.Status += SF_B2_UP;
				button2Status = FALSE;
				}
			if (!mouData.Status) SendMouseToQueue(mouseDataPtr);
			pressMouseButtonDown();
			pressMouseButtonUp();
			requestButton1 = requestButton2 = requestButton3 = FALSE;
			beginOK = TRUE;
			break;
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			break;
		case DEFAULTCODE:
		case LEFTBUTTONCODE:
			requestButton1 = TRUE;
			beginOK = FALSE;
			break;
		case RIGHTBUTTONCODE:
			requestButton2 = TRUE;
			beginOK = FALSE;
			break;
		default:
			if (cGideiCode >= LOWESTGIDEICODE)
				{
				handleFatalError();
				break;
				}
			requestButton3 = TRUE;
			beginOK = FALSE;
			break;
		}
	return;
}

void processMou(unsigned char cGideiCode)
{
	switch (cGideiCode) {
		case UNKNOWNCODE:
			handleErrorReport();
			commandVector = noOpRoutine;
			beginOK = TRUE;
			break;
		default:
			if (cGideiCode < LOWESTGIDEICODE) {
				handleErrorReport();
				commandVector = noOpRoutine;
				beginOK = TRUE;
				}
			else handleFatalError();
		}
	return;
}
