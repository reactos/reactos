/*
 * PROJECT:         ReactOS Utility Manager Resources DLL (UManDlg.dll)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         DLL header file
 * COPYRIGHT:       Copyright 2020 Bișoc George (fraizeraust99 at gmail dot com)
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
    LPCWSTR lpProgram;
    UINT    uNameId;
    WCHAR   szResource[MAX_BUFFER];
    BOOL    bState;
} UTILMAN_STATE, *PUTILMAN_STATE;

typedef struct _REGISTRY_SETTINGS
{
    /* Accessibility Registry settings */
    LPCWSTR wszAppPath;
    DWORD dwAppType;
    DWORD dwClientControlCode;
    LPCWSTR wszAppName;
    LPCWSTR wszErrorOnLaunch;
    BOOL bHideClient;
    BOOL bStartWithUtilman;
    BOOL bStartWithROS;
    LPCWSTR wszHungRespondAction;
    DWORD dwHungTimeOut;

    /* Utility Manager Registry settings */
    BOOL bShowWarning;
} REGISTRY_SETTINGS, *PREGISTRY_SETTINGS;

typedef struct _REGISTRY_DATA
{
    /* On-Screen Keyboard Registry data */
    LPCWSTR lpwsOskPath;
    LPCWSTR lpwszOskDisplayName;

    /* Magnify Registry data */
    LPCWSTR lpwszMagnifyPath;
    LPCWSTR lpwszMagnifyDisplayName;
} REGISTRY_DATA, *PREGISTRY_DATA;

/* ENUMERATIONS ***************************************************************/

typedef enum _WRITE_REGISTRY
{
    REGISTRY_ACCESSIBILITY,
    REGISTRY_UTILMAN
} WRITE_REGISTRY, *PWRITE_REGISTRY;

/* DECLARATIONS ***************************************************************/

/* umandlg.c */
BOOL DlgInitHandler(HWND hDlg);
INT_PTR APIENTRY DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT ListBoxRefreshContents(VOID);
VOID CheckUtilityState(BOOL bUtilState);
BOOL WINAPI UManStartDlg(VOID);

/* process.c */
DWORD GetProcessID(IN LPCWSTR lpProcessName);
BOOL IsProcessRunning(IN LPCWSTR lpProcessName);
BOOL LaunchProcess(LPCWSTR lpProcessName);
BOOL CloseProcess(IN LPCWSTR lpProcessName);

/* about.c */
VOID ShowAboutDlg(HWND hDlgParent);
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

/* registry.c */
BOOL InitAppRegKey(IN HKEY hPredefinedKey, IN LPCWSTR lpwszSubKey, OUT PHKEY phKey, OUT LPDWORD lpdwDisposition);
BOOL QueryAppSettings(IN HKEY hKey, IN LPCWSTR lpwszSubKey, IN LPCWSTR lpwszRegValue, OUT PVOID ReturnedData, IN OUT LPDWORD lpdwSizeData);
BOOL SaveAppSettings(IN HKEY hKey, IN LPCWSTR lpwszRegValue, IN DWORD dwRegType, IN PVOID Data, IN DWORD cbSize);

/* Struct variable declaration */
extern UTILMAN_GLOBALS Globals;
extern REGISTRY_SETTINGS Settings;
extern REGISTRY_DATA RegData;

#endif /* UMANDLG_H_ */

/* EOF */
