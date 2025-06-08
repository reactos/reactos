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
#include "indicdll/resource.h"

#define CCH_LAYOUT_ID       8   // Character Count of a layout ID like "00000409"
#define CCH_ULONG_DEC       10  // Maximum Character Count of a ULONG in decimal

// Far East Language IDs
#define LANGID_CHINESE_SIMPLIFIED MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_CHINESE_TRADITIONAL MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_JAPANESE MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)
#define LANGID_KOREAN MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT)

#define WM_LANG_CHANGED    (WM_USER + 10200)
#define WM_WINDOW_ACTIVATE (WM_USER + 10300)

#define IME_STATUS_NO_IME 0
#define IME_STATUS_IME_CLOSED 1
#define IME_STATUS_IME_OPEN 2
#define IME_STATUS_IME_NATIVE 4
#define IME_STATUS_IME_FULLSHAPE 8

static inline BOOL
IsWndClassName(_In_opt_ HWND hwndTarget, PCTSTR pszName)
{
    TCHAR szClass[32];
    GetClassName(hwndTarget, szClass, _countof(szClass));
    return lstrcmpi(szClass, pszName) == 0;
}

static inline BOOL
IsConsoleWnd(_In_opt_ HWND hwndTarget)
{
    return IsWndClassName(hwndTarget, TEXT("ConsoleWindowClass"));
}
