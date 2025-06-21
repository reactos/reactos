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
#include <debug.h>
#include <initguid.h>
#include <devguid.h>

#include "resource.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;
extern GLOBAL_POWER_POLICY gGPP;

#define MAX_POWER_PAGES 32

INT_PTR CALLBACK PowerSchemesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AlarmsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HibernateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PowerMeterDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif /* _POWERCFG_H */
