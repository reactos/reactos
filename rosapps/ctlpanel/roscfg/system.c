/*
 *  ReactOS control
 *
 *  system.c
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <windowsx.h>
#include "main.h"
#include "system.h"

#include "assert.h"
#include "trace.h"


LRESULT CALLBACK SystemGeneralPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemNetworkPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemHardwarePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemUsersPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

LRESULT CALLBACK SystemAdvancedPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

extern HMODULE hModule;

extern HWND hApplyButton;
/*
HWND hSystemGeneralPage;
HWND hSystemNetworkPage;
HWND hSystemHardwarePage;
HWND hSystemUsersPage;
HWND hSystemAdvancedPage;
 */
//typedef BOOL(CALLBACK *DLGPROC)(HWND,UINT,WPARAM,LPARAM);

#define DLGPROC_RESULT BOOL
//#define DLGPROC_RESULT LRESULT 

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

//DLGPROC_RESULT CALLBACK DateTimeAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

DLGPROC_RESULT CALLBACK SystemAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
/*
    case WM_COMMAND:
        // Handle the button clicks
        switch (LOWORD(wParam)) {
//        case IDC_ENDTASK:
//            break;
//        }
        case IDOK:
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        break;

    case WM_NOTIFY:
        //OnNotify(hDlg, wParam, lParam, &CPlAppletDlgPagesData[nThisApp]);
        break;
 */
    default:
        break;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK KeyboardAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK MouseAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK UsersAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK DisplayAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK FoldersAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK RegionalAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(hApplyButton, FALSE);
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK IrDaAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK AccessibleAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return 0L;
}

DLGPROC_RESULT CALLBACK PhoneModemAppletDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return 0L;
}
