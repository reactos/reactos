#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winuser.h>
#include <tchar.h>

#include "resource.h"

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

#define WM_KEY_PRESSED     (WM_USER + 10100)
#define WM_LANG_CHANGED    (WM_USER + 10200)
#define WM_WINDOW_ACTIVATE (WM_USER + 10300)
#define WM_LOAD_LAYOUT     (WM_USER + 10400)

TCHAR szKbSwitcherName[] = _T("kbswitcher");
