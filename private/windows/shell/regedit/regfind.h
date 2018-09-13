/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGFIND.H
*
*  VERSION:     4.00
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        14 Jul 1994
*
*******************************************************************************/

#ifndef _INC_REGFIND
#define _INC_REGFIND

extern DWORD g_FindFlags;

VOID
PASCAL
RegEdit_OnCommandFindNext(
    HWND hWnd,
    BOOL fForceDialog
    );

#endif // _INC_REGFIND
