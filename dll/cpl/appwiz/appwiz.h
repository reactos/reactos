#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <shellapi.h>
#include <cpl.h>
#include <tchar.h>
#include <shlobj.h>

#include "resource.h"

typedef struct
{
   WCHAR szTarget[MAX_PATH];
   WCHAR szWorkingDirectory[MAX_PATH];
   WCHAR szDescription[MAX_PATH];
   WCHAR szLinkName[MAX_PATH];
} CREATE_LINK_CONTEXT, *PCREATE_LINK_CONTEXT;

extern HINSTANCE hApplet;

/* createlink.c */
INT_PTR CALLBACK
WelcomeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK
FinishDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

LONG CALLBACK
NewLinkHere(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

void ShowLastWin32Error(HWND hWndOwner);

/* EOF */
