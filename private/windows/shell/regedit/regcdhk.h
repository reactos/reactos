/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGCDHK.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Common dialog box hook functions for the Registry Editor.
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

#ifndef _INC_REGCDHK
#define _INC_REGCDHK

//  Buffer to store the starting path for a registry export or print operation.
extern TCHAR g_SelectedPath[SIZE_SELECTED_PATH];

//  TRUE if registry operation should be applied to the entire registry or to
//  only start at g_SelectedPath.
extern BOOL g_fRangeAll;

//  Contains the resource identifier for the dialog that is currently being
//  used.  Assumes that there is only one instance of a hook dialog at a time.
extern UINT g_RegCommDlgDialogTemplate;

UINT_PTR
CALLBACK
RegCommDlgHookProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

#endif // _INC_REGCDHK
