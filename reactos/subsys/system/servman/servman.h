#ifndef __SERVMAN_H
#define __SERVMAN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

#define MAX_KEY_LENGTH 256
#define NUM_BUTTONS 13

BOOL RefreshServiceList(VOID);

BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL Start(LPCTSTR, LPCTSTR, INT);

VOID GetError(DWORD);
VOID FreeMemory(VOID);
VOID DisplayString(PTCHAR);

BOOL GetDescription(HKEY, LPTSTR *);
BOOL GetExecutablePath(LPTSTR *);

LONG APIENTRY OpenPropSheet(HWND);

DWORD GetServiceList(VOID);


#endif /* __SERVMAN_H */
