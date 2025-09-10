/*
 * PROJECT:     ReactOS Power Configuration Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header
 * COPYRIGHT:   Copyright 2006 Alexander Wurzinger <lohnegrim@gmx.net>
 *              Copyright 2006 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2006 Martin Rottensteiner <2005only@pianonote.at>
 */

#ifndef _POWERCFG_H
#define _POWERCFG_H

#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <cpl.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <powrprof.h>
#include <ndk/rtlfuncs.h>
#include <strsafe.h>
#include <setupapi.h>
#include <batclass.h>
#include <initguid.h>
#include <devguid.h>

#include "resource.h"

extern HINSTANCE hApplet;
extern GLOBAL_POWER_POLICY gGPP;

#define MAX_POWER_PAGES 32

INT_PTR CALLBACK PowerSchemesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AlarmsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HibernateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PowerMeterDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* _POWERCFG_H */
