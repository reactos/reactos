/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    util.h

Abstract:

    Utility functions for System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_UTIL_H_
#define _SYSDM_UTIL_H_

//
// Type definitions
//
typedef enum {
    VCREG_OK,
    VCREG_READONLY,
    VCREG_ERROR,
} VCREG_RET;  // Error return codes from opening registry

//
// Public function prototypes
//
void 
ErrMemDlg( 
    IN HWND hParent 
);

LPTSTR 
SkipWhiteSpace( 
    IN LPTSTR sz 
);

int
StringToInt( 
    IN LPTSTR sz 
);

void 
IntToString( 
    IN INT i, 
    OUT LPTSTR sz
);

LPTSTR 
CheckSlash(
    IN LPTSTR lpDir
);

BOOL 
Delnode(
    IN LPTSTR lpDir
);

LONG 
MyRegSaveKey(
    IN HKEY hKey, 
    LPCTSTR lpSubKey
);

UINT 
CreateNestedDirectory(
    IN LPCTSTR lpDirectory, 
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

LONG 
MyRegLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey, 
    IN LPTSTR lpFile
);

LONG 
MyRegUnLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey
);

int 
GetSelectedItem(
    IN HWND hCtrl
);

BOOL
IsUserAdmin(
    VOID
);

int 
MsgBoxParam( 
    IN HWND hWnd, 
    IN DWORD wText, 
    IN DWORD wCaption, 
    IN DWORD wType, 
    ... 
);

LPTSTR 
CloneString( 
    IN LPTSTR pszSrc 
);

DWORD 
SetLBWidthEx(
    IN HWND hwndLB, 
    IN LPTSTR szBuffer, 
    IN DWORD cxCurWidth, 
    IN DWORD cxExtra
);

void
HourGlass(
    IN BOOL bOn
);

VOID
SetDefButton(
    IN HWND hwndDlg,
    IN int idButton
);

VCREG_RET 
OpenRegKey( 
    IN LPTSTR szKeyName, 
    OUT PHKEY phkMM 
);

LONG CloseRegKey( 
    IN HKEY hkey 
);

#endif _SYSDM_UTIL_H_
