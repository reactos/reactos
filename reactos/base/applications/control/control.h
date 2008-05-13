/*
 * PROJECT:         ReactOS System Control Panel
 * FILE:            base/applications/control/control.h
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMERS:     Gero Kuehn (reactos.filter@gkware.com)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "resource.h"

#define MYWNDCLASS _T("CTLPANELCLASS")
typedef LONG (CALLBACK *CPLAPPLETFUNC)(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

#define CCH_UINT_MAX   11
#define MAX_VALUE_NAME 16383

/* Macro for calling "rundll32.exe"
   According to MSDN, ShellExecute returns a value greater than 32 if the operation was successful. */
#define RUNDLL(param)   ((int)ShellExecute(NULL, _T("open"), _T("rundll32.exe"), (param), NULL, SW_SHOWDEFAULT) > 32)
