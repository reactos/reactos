/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGNET.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        03 May 1994
*
*  Remote registry support for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  03 May 1994 TCS Moved existing connection code from REGEDIT.C.
*
*******************************************************************************/

#ifndef _INC_REGNET
#define _INC_REGNET

VOID
PASCAL
RegEdit_OnCommandConnect(
    HWND hWnd
    );

VOID
PASCAL
RegEdit_OnCommandDisconnect(
    HWND hWnd
    );

#endif // _INC_REGNET
