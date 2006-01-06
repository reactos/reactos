#ifndef __SERVMAN_H
#define __SERVMAN_H


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

BOOL RefreshServiceList(VOID);

BOOL CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

VOID GetError(VOID);
VOID FreeMemory(VOID);
VOID DisplayString(PTCHAR);

LONG APIENTRY
PropSheets(HWND hwnd);

DWORD GetServiceList(VOID);


#endif /* __SERVMAN_H */
