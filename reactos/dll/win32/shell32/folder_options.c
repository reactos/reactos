/*
 *	Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "wine/debug.h"

#include "windef.h"
#include "wingdi.h"
#include "pidl.h"
#include "undocshell.h"
#include "shlobj.h"
#include "objbase.h"
#include "commdlg.h"

#include "shell32_main.h"
#include "shellfolder.h"
#include "shresdef.h"
#include "stdio.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

/// Folder Options:
/// CLASSKEY = HKEY_CLASSES_ROOT\CLSID\{6DFD7C5C-2451-11d3-A299-00C04F8EF6AF}
/// DefaultIcon = %SystemRoot%\system32\SHELL32.dll,-210
/// Verbs: Open / RunAs
///       Cmd: rundll32.exe shell32.dll,Options_RunDLL 0

/// ShellFolder Attributes: 0x0

INT_PTR
CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{



    return FALSE;
}

INT_PTR
CALLBACK
FolderOptionsViewDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{



    return FALSE;
}

INT_PTR
CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{



    return FALSE;
}


VOID
ShowFolderOptionsDialog(HWND hWnd, HINSTANCE hInst)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[3];
    HPROPSHEETPAGE hpage;
	UINT num_pages = 0;
    WCHAR szOptions[100];

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_GENERAL_DLG", FolderOptionsGeneralDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_VIEW_DLG", FolderOptionsViewDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_FILETYPES_DLG", FolderOptionsFileTypesDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;
    
    szOptions[0] = L'\0';
    LoadStringW(hInst, IDS_FOLDER_OPTIONS, szOptions, sizeof(szOptions) / sizeof(WCHAR));
    szOptions[99] = L'\0';
    
    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    pinfo.nPages = num_pages;
    pinfo.u3.phpage = hppages;
    pinfo.pszCaption = szOptions;

    PropertySheetW(&pinfo);
}

VOID
Options_RunDLLCommon(HWND hWnd, HINSTANCE hInst, int fOptions, DWORD nCmdShow)
{
    switch(fOptions)
    {
    case 0:
        ShowFolderOptionsDialog(hWnd, hInst);
        break;
    case 1:
        // show taskbar options dialog
        FIXME("notify explorer to show taskbar options dialog");
        //PostMessage(GetShellWindow(), WM_USER+22, fOptions, 0);
        break;
    default:
        FIXME("unrecognized options id %d\n", fOptions);
    }
}

/*************************************************************************
 *              Options_RunDLL (SHELL32.@)
 */
VOID WINAPI Options_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}
/*************************************************************************
 *              Options_RunDLLA (SHELL32.@)
 */
VOID WINAPI Options_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLW (SHELL32.@)
 */
VOID WINAPI Options_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntW(cmd), nCmdShow);
}



