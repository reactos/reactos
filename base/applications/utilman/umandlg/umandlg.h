/*
 * PROJECT:         ReactOS Utility Manager Resources DLL (UManDlg.dll)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         DLL header file
 * COPYRIGHT:       Copyright 2020 George Bișoc (george.bisoc@reactos.org)
 */

#ifndef _UMANDLG_H
#define _UMANDLG_H

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
    LPCWSTR lpszProgram;
    UINT    uNameId;
    WCHAR   szResource[MAX_BUFFER];
    BOOL    bState;
} UTILMAN_STATE, *PUTILMAN_STATE;

typedef struct _REGISTRY_SETTINGS
{
    /* Accessibility Registry settings */
    LPCWSTR lpszAppPath;
    DWORD dwAppType;
    DWORD dwClientControlCode;
    LPCWSTR lpszAppName;
    LPCWSTR lpszErrorOnLaunch;
    BOOL bHideClient;
    BOOL bStartWithUtilman;
    BOOL bStartWithROS;
    LPCWSTR lpszHungRespondAction;
    DWORD dwHungTimeOut;

    /* Utility Manager Registry settings */
    BOOL bShowWarning;
} REGISTRY_SETTINGS, *PREGISTRY_SETTINGS;

typedef struct _REGISTRY_DATA
{
    /* On-Screen Keyboard Registry data */
    LPCWSTR lpszOskPath;
    LPCWSTR lpszOskDisplayName;

    /* Magnify Registry data */
    LPCWSTR lpszMagnifyPath;
    LPCWSTR lpszMagnifyDisplayName;
} REGISTRY_DATA, *PREGISTRY_DATA;

/* ENUMERATIONS ***************************************************************/

typedef enum _WRITE_REGISTRY
{
    REGISTRY_ACCESSIBILITY,
    REGISTRY_UTILMAN
} WRITE_REGISTRY, *PWRITE_REGISTRY;

/* DECLARATIONS ***************************************************************/

/* umandlg.c */
VOID InitUtilsList(IN BOOL bInitGui);
BOOL DlgInitHandler(IN HWND hDlg);
VOID ShowAboutDlg(IN HWND hDlgParent);
VOID GroupBoxUpdateTitle(VOID);
VOID UpdateUtilityState(IN BOOL bUtilState);
INT_PTR APIENTRY DlgProc(IN HWND hDlg, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam);
INT ListBoxRefreshContents(VOID);
BOOL WINAPI UManStartDlg(VOID);

/* process.c */
DWORD GetProcessID(IN LPCWSTR lpszProcessName);
BOOL IsProcessRunning(IN LPCWSTR lpszProcessName);
BOOL LaunchProcess(IN LPCWSTR lpszProcessName);
BOOL CloseProcess(IN LPCWSTR lpszProcessName);

/* registry.c */
BOOL InitAppRegKey(IN HKEY hPredefinedKey, IN LPCWSTR lpszSubKey, OUT PHKEY phKey, OUT LPDWORD lpdwDisposition);
BOOL QueryAppSettings(IN HKEY hKey, IN LPCWSTR lpszSubKey, IN LPCWSTR lpszRegValue, OUT PVOID ReturnedData, IN OUT LPDWORD lpdwSizeData);
BOOL SaveAppSettings(IN HKEY hKey, IN LPCWSTR lpszRegValue, IN DWORD dwRegType, IN PVOID Data, IN DWORD cbSize);

/* Struct variable declaration */
extern UTILMAN_GLOBALS Globals;
extern REGISTRY_SETTINGS Settings;
extern REGISTRY_DATA RegData;

#endif /* UMANDLG_H_ */

/* EOF */
