/*
 * PROJECT:         ReactOS Utility Manager (Accessibility)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Pre-compiled header file
 * COPYRIGHT:       Copyright 2019 Bișoc George (fraizeraust99 at gmail dot com)
 */

#ifndef _UTILMAN_H
#define _UTILMAN_H

/* INCLUDES ******************************************************************/

#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <windowsx.h>
#include <debug.h>
#include <wchar.h>
#include <strsafe.h>

#include "resource.h"

/* DEFINES ********************************************************************/

#define MAX_BUFFER 256

/* TYPES **********************************************************************/

typedef struct
{
    HINSTANCE hInstance;
    HICON     hIcon;
    UINT_PTR  iTimer;
    INT       iSelectedIndex;
    HWND      hDlgCtlStart;
    HWND      hDlgCtlStop;
    HWND      hListDlg;
    HWND      hMainDlg;
    WCHAR     szRunning[MAX_BUFFER];
    WCHAR     szNotRunning[MAX_BUFFER];
    WCHAR     szGrpBoxTitle[MAX_BUFFER];
} UTILMAN_GLOBALS;

typedef struct _UTILMAN_STATE
{
    LPCWSTR lpProgram;
    UINT    uNameId;
    WCHAR   szResource[MAX_BUFFER];
    BOOL    bState;
} UTILMAN_STATE, *PUTILMAN_STATE;

/* DECLARATIONS ***************************************************************/

/* dialog.c */
BOOL DlgInitHandler(HWND hDlg);
INT_PTR APIENTRY DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT ListBoxRefreshContents(VOID);
VOID CheckUtilityState(BOOL bUtilState);

/* process.c */
BOOL IsProcessRunning(IN LPCWSTR lpProcessName);
BOOL LaunchProcess(LPCWSTR lpProcessName);
BOOL CloseProcess(IN LPCWSTR lpProcessName);

/* about.c */
VOID ShowAboutDlg(HWND hDlgParent);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

/* Struct variable declaration */
extern UTILMAN_GLOBALS Globals;

#endif /* _UTILMAN_H */

/* EOF */
