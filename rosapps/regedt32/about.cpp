/*
 *  ReactOS About Dialog Box
 *
 *  about.cpp
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
    
#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "regedt32.h"
#include "about.h"


extern HINSTANCE hInst;
//extern HWND hMainWnd;

LRESULT CALLBACK AboutDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


void ShowAboutBox(HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)AboutDialogWndProc);
}

LRESULT CALLBACK AboutDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hLicenseEditWnd;
    TCHAR   strLicense[0x1000];

    switch (message) {
    case WM_INITDIALOG:
        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE_EDIT);
        LoadString(hInst, IDS_LICENSE, strLicense, 0x1000);
        SetWindowText(hLicenseEditWnd, strLicense);
        return TRUE;
    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return 0;
}
