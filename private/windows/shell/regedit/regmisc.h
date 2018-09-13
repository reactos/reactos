/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGMISC.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Miscellaneous routines for the Registry Editor.
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

#ifndef _INC_REGMISC
#define _INC_REGMISC

PTSTR
CDECL
LoadDynamicString(
    UINT StringID,
    ...
    );

//  Wrapper for LocalFree to make the code a little easier to read.
#define DeleteDynamicString(x)          LocalFree((HLOCAL) (x))

VOID
PASCAL
CopyRegistry(
    HKEY hSourceKey,
    HKEY hDestinationKey
    );

HBRUSH
PASCAL
CreateDitheredBrush(
    VOID
    );

VOID
PASCAL
SendChildrenMessage(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
MessagePump(
    HWND hDialogWnd
    );

LPTSTR
PASCAL
GetNextSubstring(
    LPTSTR lpString
    );

int
PASCAL
InternalMessageBox(
    HINSTANCE hInst,
    HWND hWnd,
    LPCTSTR pszFormat,
    LPCTSTR pszTitle,
    UINT fuStyle,
    ...
    );

//  The Windows 95 and Windows NT implementations of RegDeleteKey differ in how
//  they handle subkeys of the specified key to delete.  Windows 95 will delete
//  them, but NT won't, so we hide the differences using this macro.
#ifdef WINNT
LONG
RegDeleteKeyRecursive(
    IN HKEY hKey,
    IN LPCTSTR lpszSubKey
    );
#else
#define RegDeleteKeyRecursive(hkey, lpsz)   RegDeleteKey(hkey, lpsz)
#endif

#define IsRegStringType(x)  (((x) == REG_SZ) || ((x) == REG_EXPAND_SZ) || ((x) == REG_MULTI_SZ))
#endif // _INC_REGMISC
