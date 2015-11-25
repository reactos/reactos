/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *                  2015 Robert Naumann <gonzomdx@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include "precomp.h"

VOID OnAddStartmenuItems(HWND hDlg)
{
    WCHAR szPath[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, szPath))) 
    {
        WCHAR szCommand[MAX_PATH] = L"appwiz.cpl,NewLinkHere ";
        if(SUCCEEDED(StringCchCatW(szCommand, MAX_PATH, szPath)))
            ShellExecuteW(hDlg, L"open", L"rundll32.exe", szCommand, NULL, SW_SHOWNORMAL);
    }
}

VOID OnAdvancedStartMenuItems()
{
    WCHAR szPath[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, szPath))) 
    {
        ShellExecuteW(NULL, L"explore", szPath, NULL, NULL, SW_SHOWNORMAL);
    }
}

VOID OnClearRecentItems()
{
   WCHAR szPath[MAX_PATH], szFile[MAX_PATH];
   WIN32_FIND_DATA info;
   HANDLE hPath;

    if(SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, szPath))) 
    {
        StringCchPrintf(szFile,MAX_PATH, L"%s\\*.*", szPath);
        hPath = FindFirstFileW(szFile, &info);
        do
        {
            StringCchPrintf(szFile,MAX_PATH, L"%s\\%s", szPath, info.cFileName);
            DeleteFileW(szFile);
 
        }while(FindNextFileW(hPath, &info)); 
        FindClose(hPath);
        /* FIXME: Disable the button*/
    } 
}

BOOL CALLBACK CustomizeClassicProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
            /* FIXME: Properly intialize the dialog (check whether 'clear' button must be disabled, for example) */
        return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_CLASSICSTART_ADD:
                    OnAddStartmenuItems(hwnd);
                break;
                case IDC_CLASSICSTART_ADVANCED:
                    OnAdvancedStartMenuItems();
                break;
                case IDC_CLASSICSTART_CLEAR:
                    OnClearRecentItems();
                break;
                case IDOK:
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}
 
VOID ShowCustomizeClassic(HINSTANCE hInst, HWND hExplorer)
 {
     DialogBox(hInst, MAKEINTRESOURCE(IDD_CLASSICSTART_CUSTOMIZE), hExplorer, CustomizeClassicProc);
 }