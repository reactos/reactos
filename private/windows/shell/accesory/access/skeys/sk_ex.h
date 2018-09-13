/*--------------------------------------------------------------
 *
 * FILE:			SK_EX.h
 *
 * PURPOSE:			Header file for SK_EX.C
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/
BOOL SkEx_GetAnchor(LPPOINT Mouse);
void SkEx_SetAnchor();
void SkEx_SendBeep();
void SkEx_SendKeyUp(int scanCode); 			// Send char from SerialKeys
void SkEx_SendKeyDown(int scanCode);
void SkEx_SendMouse(MOUSEKEYSPARAM *p);		// Send mouse from SerialKeys
void SkEx_SetBaud(int Baud);

BOOL DeskSwitchToInput();
