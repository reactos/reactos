#pragma once

#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <shellapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <ime/indicml.h> /* INDICATOR_CLASS, INDICM_... */

#include "resource.h"

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

#define WM_KEY_PRESSED     (WM_USER + 10100)

// WM_LANG_CHANGED message:
//   wParam: HWND hwndTarget or NULL
//   lParam: HKL hConsoleKL or NULL
#define WM_LANG_CHANGED    (WM_USER + 10200)

// WM_WINDOW_ACTIVATE message:
//   wParam: HWND hwndTarget or NULL
//   lParam: NULL
#define WM_WINDOW_ACTIVATE (WM_USER + 10300)

typedef BOOL (APIENTRY *FN_KbSwitchSetHooks)(BOOL bHook);

const TCHAR szKbSwitcherName[] = INDICATOR_CLASS;

static inline BOOL
CheckWndClassName(_In_opt_ HWND hwndTarget, PCTSTR pszName)
{
    TCHAR szClass[32];
    GetClassName(hwndTarget, szClass, _countof(szClass));
    return lstrcmpi(szClass, pszName) == 0;
}

static inline BOOL
IsConsoleWnd(_In_opt_ HWND hwndTarget)
{
    return CheckWndClassName(hwndTarget, TEXT("ConsoleWindowClass"));
}
