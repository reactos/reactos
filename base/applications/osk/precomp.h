/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Pre-compiled header
 * COPYRIGHT:       Denis ROBERT
 *                  Copyright 2019 George Bișoc (george.bisoc@reactos.org)
 */

#ifndef _OSK_PRECOMP_H
#define _OSK_PRECOMP_H

/* INCLUDES *******************************************************************/

#include <stdio.h>

#include <windows.h>
#include <commctrl.h>
#include <debug.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"

#include "osk_res.h"

/* TYPES **********************************************************************/

typedef struct
{
    HINSTANCE  hInstance;
    HWND       hMainWnd;
    HBRUSH     hBrushGreenLed;
    UINT_PTR   iTimer;
    /* FIXME: To be deleted when ReactOS will support WS_EX_NOACTIVATE */
    HWND       hActiveWnd;

    /* On-Screen Keyboard registry settings */
    BOOL       bShowWarning;
    BOOL       bIsEnhancedKeyboard;
    BOOL       bSoundClick;
    BOOL       bAlwaysOnTop;
    INT        PosX;
    INT        PosY;
} OSK_GLOBALS;

typedef struct
{
    INT vKey;
    INT DlgResource;
    WORD wScanCode;
    BOOL bWasKeyPressed;
} OSK_KEYLEDINDICATOR;

/* PROTOTYPES *****************************************************************/

/* main.c */
int OSK_SetImage(int IdDlgItem, int IdResource);
int OSK_DlgInitDialog(HWND hDlg);
int OSK_DlgClose(void);
int OSK_DlgTimer(void);
BOOL OSK_DlgCommand(WPARAM wCommand, HWND hWndControl);
BOOL OSK_ReleaseKey(WORD ScanCode);
INT_PTR APIENTRY OSK_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY OSK_ThemeHandler(HWND hDlg, NMCUSTOMDRAW *pNmDraw);
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
VOID OSK_RestoreDlgPlacement(HWND hDlg);
VOID OSK_RefreshLEDKeys(VOID);
INT_PTR CALLBACK OSK_WarningProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

/* settings.c */
LONG LoadDataFromRegistry(IN LPCWSTR lpValueDataName,
                          OUT PDWORD pdwValueData);

LONG SaveDataToRegistry(IN LPCWSTR lpValueDataName,
                        IN DWORD dwValueData);

VOID LoadSettings(VOID);
VOID SaveSettings(VOID);

/* DEFINES ********************************************************************/

extern OSK_GLOBALS Globals;

#define countof(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_BUFF 256

#endif /* _OSK_PRECOMP_H */

/* EOF */
