#pragma once

#define COBJMACROS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>
#include <prsht.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlguid.h>

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
