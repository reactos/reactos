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
#include <ime/indicml.h> /* INDICATOR_CLASS, INDICM_... */

#include "resource.h"

// Character Count of a layout ID like "00000409"
#define CCH_LAYOUT_ID    8

// Maximum Character Count of a ULONG in decimal
#define CCH_ULONG_DEC    10

#define WM_KEY_PRESSED     (WM_USER + 10100)
#define WM_LANG_CHANGED    (WM_USER + 10200)
#define WM_WINDOW_ACTIVATE (WM_USER + 10300)

// wParam for WM_LANG_CHANGED
#define LANG_CHANGED_FROM_SHELL_MSG     0x10000
#define LANG_CHANGED_FROM_SHELL         0x10001
#define LANG_CHANGED_FROM_KBD_LL        0x10002

// wParam for WM_WINDOW_ACTIVATE
#define WINDOW_ACTIVATE_FROM_SETTING    0x20000
#define WINDOW_ACTIVATE_FROM_SHELL_MSG  0x20001
#define WINDOW_ACTIVATE_FROM_FOCUS      0x20002
#define WINDOW_ACTIVATE_FROM_SHELL      0x20003

typedef BOOL (APIENTRY *FN_KbSwitchSetHooks)(BOOL bHook);

const TCHAR szKbSwitcherName[] = INDICATOR_CLASS;
