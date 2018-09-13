/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDWDED.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        24 Sep 1994
*
*  Dword edit dialog for use by the Registry Editor.
*
*******************************************************************************/

#ifndef _INC_REGDWDED
#define _INC_REGDWDED

INT_PTR
CALLBACK
EditDwordValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

#endif // _INC_REGDWDED
