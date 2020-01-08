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

// TODO: Windows Explorer appears to be calling NewLinkHere / ConfigStartMenu directly for both items.
VOID OnAddStartMenuItems(HWND hDlg)
{
    WCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, szPath)))
    {
        WCHAR szCommand[MAX_PATH] = L"appwiz.cpl,NewLinkHere ";
        if (SUCCEEDED(StringCchCatW(szCommand, _countof(szCommand), szPath)))
            ShellExecuteW(hDlg, L"open", L"rundll32.exe", szCommand, NULL, SW_SHOWNORMAL);
    }
}

VOID OnRemoveStartmenuItems(HWND hDlg)
{
    ShellExecuteW(hDlg, L"open", L"rundll32.exe", L"appwiz.cpl,ConfigStartMenu", NULL, SW_SHOWNORMAL);
}

VOID OnAdvancedStartMenuItems()
{
    WCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, szPath)))
    {
        ShellExecuteW(NULL, L"explore", szPath, NULL, NULL, SW_SHOWNORMAL);
    }
}

VOID OnClearRecentItems(HWND hwnd)
{
   WCHAR szPath[MAX_PATH], szFile[MAX_PATH];
   WIN32_FIND_DATAW info;
   HANDLE hPath;

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, szPath)))
    {
        StringCchPrintfW(szFile, _countof(szFile), L"%s\\*.*", szPath);
        hPath = FindFirstFileW(szFile, &info);
        do
        {
            StringCchPrintfW(szFile, _countof(szFile), L"%s\\%s", szPath, info.cFileName);
            DeleteFileW(szFile);
        }
        while (FindNextFileW(hPath, &info));
        FindClose(hPath);

        EnableWindow(GetDlgItem(hwnd, IDC_CLASSICSTART_CLEAR), FALSE);
    }
}

typedef void (*STARTMENU_CUSTOM_OPTION)(DWORD, DWORD);

VOID DefaultFunc(DWORD dwItem, DWORD dwState);

static struct
{
    PCWSTR OptionName;
    STARTMENU_CUSTOM_OPTION StartMenu_CustomOption;
    BOOL IsDirty;
} ClassicOption[] = {
    {L"Start_EnableDragDrop", DefaultFunc, FALSE},       // REG_DWORD
    {L"Start_ShowRun", DefaultFunc, FALSE},              // REG_DWORD
    {L"StartMenuScrollPrograms", DefaultFunc, FALSE},    // REG_SZ
    {L"CascadeMyPictures", DefaultFunc, FALSE},          // REG_SZ
    {L"CascadePrinters", DefaultFunc, FALSE},            // REG_SZ
    {L"CascadeNetworkConnections", DefaultFunc, FALSE},  // REG_SZ
    {L"CascadeMyDocuments", DefaultFunc, FALSE},         // REG_SZ
    {L"CascadeControlPanel", DefaultFunc, FALSE},        // REG_SZ
    {L"StartMenuFavorites", DefaultFunc, FALSE},         // REG_DWORD
    {L"StartMenuAdminToolsRoot", DefaultFunc, FALSE},    // REG_SZ
    {L"ShowSmallIcons", DefaultFunc, FALSE},            // REG_DWORD
    {L"StartCustomMenus", DefaultFunc, FALSE},           // REG_DWORD
    {NULL, NULL, FALSE}                                  // Last IsDirty stores the bitmap of all current checkBtn state
};

VOID DefaultFunc(DWORD dwItem, DWORD dwState)
{
    WCHAR buf[128];
    const WCHAR *ptr = dwState ? L"ON" : L"OFF";
    wsprintf(buf, L"%s   %s\n", ClassicOption[dwItem].OptionName, ptr);

    MessageBoxW(0, buf, L"Not implemented", MB_OK);
}

VOID ExecuteCustomOptions(DWORD dwItem, DWORD dwUserOptions, DWORD dwOptSize)
{
    for (DWORD idx = 0; idx < dwOptSize; idx++)
    {
        if ((dwItem >> idx) & 0x01)
            ClassicOption[idx].StartMenu_CustomOption(idx, !((dwUserOptions >> idx) & 0x01));
    }
}

DWORD LoadUserConfData(DWORD *dwLength)
{
    HKEY hKey;
    DWORD iItem;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwStatus = 0;
    WCHAR chBuffer[16];

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                     0, KEY_QUERY_VALUE,
                     &hKey) != ERROR_SUCCESS)
         return 0;
    for (iItem = 0; ClassicOption[iItem].OptionName != NULL; iItem++)
    {
        if (((IDS_DRAG_DROP + iItem) > IDS_SHOW_RUN) &&
            ((IDS_DRAG_DROP + iItem) < IDS_SHOW_SMALL_ICO) &&
            ((IDS_DRAG_DROP + iItem) != IDS_SHOW_FAVORITES))
            dwSize = (lstrlen(L"YES") + 1) * sizeof(WCHAR);
        else
            dwSize = sizeof(REG_DWORD);

        if (RegQueryValueExW(hKey,
                             ClassicOption[iItem].OptionName,
                             NULL,
                             &dwType,
                             (LPBYTE) &chBuffer,
                             &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_SZ)
            {
                BOOL stat = (lstrcmpiW(chBuffer, L"YES") == 0) ? TRUE : FALSE;
                if (stat)
                    dwStatus |= (1 << iItem);
                else
                    dwStatus &= ~(1 << iItem);
            }
            else if (dwType == REG_DWORD && dwSize == sizeof(REG_DWORD))
            {
                DWORD regValue = *(reinterpret_cast<DWORD *>(chBuffer));
                if (regValue)
                    dwStatus |= (1 << iItem);
                else
                    dwStatus &= ~(1 << iItem);
            }
        }
    }
    *dwLength = iItem;
    //  Save Current User options
    ClassicOption[iItem].IsDirty = dwStatus;
    RegCloseKey(hKey);
    return dwStatus;
}

DWORD UpLoadUserConfData(DWORD &dwUserData)
{
    HKEY hKey;
    DWORD iItem = 0;
    DWORD nItems = 0;
    DWORD dwRes;

    while (ClassicOption[nItems].OptionName != NULL)
        nItems++;

   if (RegCreateKeyExW(HKEY_CURRENT_USER,
                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_SET_VALUE,
                       NULL,
                       &hKey,
                       &dwRes) == ERROR_SUCCESS)
    {
        DWORD dwQueryData = ClassicOption[nItems].IsDirty;

        while (iItem < nItems)
        {
            if (((dwQueryData >> iItem) & 0x01) != ((dwUserData >> iItem) & 0x01))
            {
                if (((IDS_DRAG_DROP + iItem) > IDS_SHOW_RUN) &&
                    ((IDS_DRAG_DROP + iItem) < IDS_SHOW_SMALL_ICO) &&
                    ((IDS_DRAG_DROP + iItem) != IDS_SHOW_FAVORITES))
                {
                    const wchar_t *sKey = ((dwQueryData >> iItem) & 0x01) ? L"YES" : L"NO";
                    RegSetValueExW(hKey, ClassicOption[iItem].OptionName, NULL, REG_SZ, (LPBYTE) sKey, (lstrlen(sKey) + 1) * sizeof(WCHAR));
                }
                else
                {
                    DWORD regValue = ((dwQueryData >> iItem) & 0x01) ? 1 : 0;
                    RegSetValueExW(hKey, ClassicOption[iItem].OptionName, NULL, REG_DWORD, (LPBYTE) &regValue, sizeof(REG_DWORD));
                }
            }
            iItem++;
        }
        return dwQueryData;
    }
    return 0;
}

HWND InitClassicListView(HWND hwnd)
{
    RECT rcClient;
    HWND hListView = GetDlgItem(hwnd, IDC_CLASSICSTART_SETTINGS);
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    GetClientRect(hListView, &rcClient);
    WCHAR chBuffer[80];
    int id = IDS_DRAG_DROP;
    int nItems = 0;

    while (ClassicOption[nItems].OptionName != NULL)
        nItems++;
    DWORD dwOptions = ClassicOption[nItems].IsDirty;

    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.pszText = NULL;
    lvc.cx = rcClient.right;
    SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc);

    for (int iItem = 0; iItem < nItems; iItem++)
    {
        LVITEM hitem = {0};
        hitem.mask = LVIF_TEXT;
        hitem.iItem = iItem;
        LoadStringW(NULL, id++, chBuffer, 79);
        hitem.pszText = chBuffer;
        hitem.cchTextMax = (lstrlen(chBuffer)) * sizeof(WCHAR);
        SendMessage(hListView, LVM_INSERTITEM, 0, (LPARAM)&hitem);

        BOOL bStatus = ((dwOptions >> iItem) & 0x01);
        ListView_SetCheckState(hListView, iItem, bStatus);
    }
    return hListView;
}

VOID UpdateClassicOptions(HWND hListView)
{
    int nItemCount = ListView_GetItemCount(hListView);
    DWORD dwStatus = ClassicOption[nItemCount].IsDirty;

    for (int iItem = 0; iItem < nItemCount; iItem++)
    {
        if (ClassicOption[iItem].IsDirty == TRUE)
        {
            BOOL bState = ListView_GetCheckState(hListView, iItem);
            if (bState)
                dwStatus |= (1 << iItem);
            else
                dwStatus &= ~(1 << iItem);
            ClassicOption[iItem].IsDirty = FALSE;
        }
    }
    ClassicOption[nItemCount].IsDirty = dwStatus;
}

INT_PTR CALLBACK CustomizeClassicProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HWND hListView = NULL;

    switch (Message)
    {
        case WM_INITDIALOG:
            /* FIXME: Properly initialize the dialog (check whether 'clear' button must be disabled, for example) */
            hListView = InitClassicListView(hwnd);
            return TRUE;
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom == IDC_CLASSICSTART_SETTINGS && ((LPNMHDR)lParam)->code == NM_CLICK)
            {
                int item = ((LPNMITEMACTIVATE)lParam)->iItem;
                ClassicOption[item].IsDirty = TRUE;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CLASSICSTART_ADD:
                    OnAddStartMenuItems(hwnd);
                    break;
                case IDC_CLASSICSTART_REMOVE:
                    OnRemoveStartmenuItems(hwnd);
                    break;
                case IDC_CLASSICSTART_ADVANCED:
                    OnAdvancedStartMenuItems();
                    break;
                case IDC_CLASSICSTART_CLEAR:
                    OnClearRecentItems(hwnd);
                    break;
                case IDOK:
                    UpdateClassicOptions(hListView);
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    for (int i = 0; ClassicOption[i].OptionName != NULL; i++)
                        if (ClassicOption[i].IsDirty == TRUE)
                            ClassicOption[i].IsDirty = FALSE;
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
        case WM_DESTROY:
            SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
            return FALSE;
        default:
            return FALSE;
    }
    return TRUE;
}

INT ShowCustomizeClassic(HINSTANCE hInst, HWND hExplorer)
{
    return DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_CLASSICSTART_CUSTOMIZE), hExplorer, CustomizeClassicProc);
}
