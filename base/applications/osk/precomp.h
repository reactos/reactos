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
#include <debug.h>
#include "commctrl.h"
#include "strsafe.h"

#include "osk_res.h"

/* TYPES **********************************************************************/

typedef struct _KEY
{
    LPCWSTR name;
    INT_PTR scancode;
    INT x;
    INT y;
    INT cx;
    INT cy;
    INT flags;
    BOOL translate;
} KEY, *PKEY;

typedef struct _KEYBOARD_STRUCT
{
    PKEY Keys;
    INT KeyCount;
    SIZE Size;
    POINT LedTextStart;
    SIZE LedTextSize;
    INT LedTextOffset; 
    POINT LedStart;
    SIZE LedSize;
    INT LedGap;
} KEYBOARD_STRUCT, *PKEYBOARD_STRUCT;

typedef struct
{
    HINSTANCE  hInstance;
    HWND       hMainWnd;
    HBRUSH     hBrushGreenLed;
    UINT_PTR   iTimer;
    PKEYBOARD_STRUCT Keyboard;
    HWND*      hKeys;
    HFONT      hFont;
    WCHAR      szTitle[MAX_PATH];
    /* FIXME: To be deleted when ReactOS will support WS_EX_NOACTIVATE */
    HWND       hActiveWnd;

    /* On-Screen Keyboard registry settings */
    BOOL       bShowWarning;
    BOOL       bIsEnhancedKeyboard;
    BOOL       bSoundClick;
    BOOL       bAlwaysOnTop;
    INT        PosX;
    INT        PosY;
    WCHAR      FontFaceName[LF_FACESIZE];
    LONG       FontHeight;
} OSK_GLOBALS;

typedef struct
{
    INT vKey;
    INT DlgResource;
    WORD wScanCode;
    BOOL bWasKeyPressed;
} OSK_KEYLEDINDICATOR;

/* PROTOTYPES *****************************************************************/

/* keyboard.c */
extern KEYBOARD_STRUCT StandardKeyboard;
extern KEYBOARD_STRUCT EnhancedKeyboard;

/* main.c */
int OSK_SetImage(int IdDlgItem, int IdResource);
LRESULT OSK_Create(HWND hwnd);
int OSK_Close(void);
int OSK_Timer(void);
BOOL OSK_Command(WPARAM wCommand, HWND hWndControl);
BOOL OSK_ReleaseKey(WORD ScanCode);
LRESULT APIENTRY OSK_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI OSK_WarningDlgThread(LPVOID lpParameter);
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
VOID OSK_RestoreDlgPlacement(HWND hDlg);
VOID OSK_RefreshLEDKeys(VOID);
INT_PTR CALLBACK OSK_WarningProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

/* settings.c */
LONG LoadDWORDFromRegistry(IN LPCWSTR lpValueDataName,
                           OUT PDWORD pdwValueData);

LONG LoadStringFromRegistry(IN LPCWSTR lpValueDataName,
                            OUT LPWSTR lpValueData,
                            IN OUT LPUINT cchCount);

LONG SaveDWORDToRegistry(IN LPCWSTR lpValueDataName,
                         IN DWORD dwValueData);

LONG SaveStringToRegistry(IN LPCWSTR lpValueDataName,
                          IN LPCWSTR lpValueData,
                          IN UINT cchCount);

VOID LoadSettings(VOID);
VOID SaveSettings(VOID);

/* DEFINES ********************************************************************/

#define SCANCODE_MASK 0xFF

extern OSK_GLOBALS Globals;

#define OSK_CLASS L"OSKMainWindow"
#define DEFAULT_FONTSIZE 15

/* OSK_SetKeys reasons */
enum SetKeys_Reason
{
    SETKEYS_INIT,
    SETKEYS_LAYOUT,
    SETKEYS_LANG
};

#endif /* _OSK_PRECOMP_H */

/* EOF */
