/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGBINED.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Binary edit dialog for use by the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGBINED
#define _INC_REGBINED

INT_PTR
CALLBACK
EditBinaryValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
RegisterHexEditClass(
    VOID
    );

#endif // _INC_REGBINED
