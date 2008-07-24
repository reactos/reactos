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

VOID
InitializeFileTypesListCtrlColumns(HWND hDlgCtrl)
{
    RECT clientRect;
    LVCOLUMNW col;
    WCHAR szName[50];

    if (!LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szName, sizeof(szName) / sizeof(WCHAR)))
        szName[0] = 0;
    szName[(sizeof(szName)/sizeof(WCHAR))-1] = 0;

    GetClientRect(hDlgCtrl, &clientRect);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    col.mask      = LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
    col.iSubItem  = 0;
    col.pszText = szName;
    col.fmt = LVCFMT_LEFT;
    col.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumnW(hDlgCtrl, 0, &col);
}
INT
FindItem(HWND hDlgCtrl, WCHAR * ItemName)
{
    LVFINDINFOW findInfo;
    ZeroMemory(&findInfo, sizeof(LVFINDINFOW));

    findInfo.flags = LVFI_STRING;
    findInfo.psz = ItemName;
    return ListView_FindItem(hDlgCtrl, 0, &findInfo);
}

VOID
InsertFileType(HWND hDlgCtrl, WCHAR * szName, DWORD Size, INT iItem)
{
    WCHAR szPath[100];
    HKEY hKey;
    LVITEMW lvItem;
    DWORD dwSize;

    if (FindItem(hDlgCtrl, szName) != -1)
        return;

    wcscpy(szPath, szName);
    /* get the name */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegLoadMUIStringW(hKey, L"FriendlyTypeName", szName, Size, NULL, 0, NULL) != ERROR_SUCCESS)
        {
            dwSize = Size;
            if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)szName, &dwSize) != ERROR_SUCCESS)
            {
                wcscpy(szName, szPath);
            }
        }
        RegCloseKey(hKey);
        szName[(Size/sizeof(WCHAR))-1] = 0;
    }
    wcscat(szPath, L"\\shell");

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvItem.state = LVIS_SELECTED; 
    lvItem.pszText = szName;
    lvItem.iItem = iItem;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        lvItem.lParam = 0;
        (void)ListView_InsertItemW(hDlgCtrl, &lvItem);
        RegCloseKey(hKey);
    }
}

BOOL
InitializeFileTypesListCtrl(HWND hwndDlg)
{
    HWND hDlgCtrl;
    DWORD dwIndex = 0;
    WCHAR szName[50];
    DWORD dwName;
    INT iItem = 0;


    hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    InitializeFileTypesListCtrlColumns(hDlgCtrl);



    dwName = sizeof(szName) / sizeof(WCHAR);

    while(RegEnumKeyExW(HKEY_CLASSES_ROOT, dwIndex++, szName, &dwName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        InsertFileType(hDlgCtrl, szName, sizeof(szName), iItem++);
        dwName = sizeof(szName) / sizeof(WCHAR);
    }
    return TRUE;
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
    switch(uMsg)
    {
        case WM_INITDIALOG:
            InitializeFileTypesListCtrl(hwndDlg);
            return TRUE;
    }


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
    LoadStringW(shell32_hInstance, IDS_FOLDER_OPTIONS, szOptions, sizeof(szOptions) / sizeof(WCHAR));
    szOptions[(sizeof(szOptions)/sizeof(WCHAR))-1] = L'\0';

    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP;
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



