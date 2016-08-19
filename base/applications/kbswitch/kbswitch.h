#pragma once

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <shellapi.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

#define WM_KEY_PRESSED     (WM_USER + 10100)
#define WM_LANG_CHANGED    (WM_USER + 10200)
#define WM_WINDOW_ACTIVATE (WM_USER + 10300)
#define WM_LOAD_LAYOUT     (WM_USER + 10400)

typedef BOOL (WINAPI *PKBSWITCHSETHOOKS) (VOID);
typedef VOID (WINAPI *PKBSWITCHDELETEHOOKS) (VOID);

TCHAR szKbSwitcherName[] = _T("kbswitcher");
