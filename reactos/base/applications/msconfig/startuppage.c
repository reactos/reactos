/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/startuppage.c
 * PURPOSE:     Startup page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include <precomp.h>

HWND hStartupPage;
HWND hStartupListCtrl;
HWND hStartupDialog;

void GetAutostartEntriesFromRegistry ( HKEY hRootKey, TCHAR* KeyName );

INT_PTR CALLBACK
StartupPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LV_COLUMN   column;
    TCHAR       szTemp[256];
    DWORD dwStyle;

    switch (message) {
    case WM_INITDIALOG:

        hStartupListCtrl = GetDlgItem(hDlg, IDC_STARTUP_LIST);
        hStartupDialog = hDlg;

        dwStyle = (DWORD) SendMessage(hStartupListCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
        dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        SendMessage(hStartupListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

        SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        // Initialize the application page's controls
        column.mask = LVCF_TEXT | LVCF_WIDTH;

        LoadString(hInst, IDS_STARTUP_COLUMN_ELEMENT, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 150;
        (void)ListView_InsertColumn(hStartupListCtrl, 0, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_STARTUP_COLUMN_CMD, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 150;
        (void)ListView_InsertColumn(hStartupListCtrl, 1, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_STARTUP_COLUMN_PATH, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 250;
        (void)ListView_InsertColumn(hStartupListCtrl, 2, &column);

        GetAutostartEntriesFromRegistry(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"));
        GetAutostartEntriesFromRegistry(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"));
        //FIXME: What about HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Userinit
        //FIXME: Common Startup (startmenu)

        return TRUE;
    }

    return 0;
}

void
GetAutostartEntriesFromRegistry ( HKEY hRootKey, TCHAR* KeyName )
{
    HKEY hKey;
    DWORD Index, dwValues, retVal, dwType;
    DWORD dwValueLength, dwDataLength = MAX_VALUE_NAME; 
    TCHAR* Data;
    TCHAR lpValueName[MAX_KEY_LENGTH];
    TCHAR Path[MAX_KEY_LENGTH + 5];
    LV_ITEM item;

    if (RegOpenKeyEx(hRootKey, KeyName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            for (Index = 0, retVal = ERROR_SUCCESS; Index < dwValues; Index++) 
            {
                dwValueLength = MAX_KEY_LENGTH;
                dwDataLength = MAX_VALUE_NAME;
                Data = (TCHAR*) HeapAlloc(GetProcessHeap(), 0, MAX_VALUE_NAME * sizeof(TCHAR));
                if (Data == NULL)
                    break;
                retVal = RegEnumValue(hKey, Index, lpValueName, &dwValueLength, NULL, &dwType, (LPBYTE)Data, &dwDataLength);
                if (retVal == ERROR_SUCCESS) 
                {
                    memset(&item, 0, sizeof(LV_ITEM));
                    item.mask = LVIF_TEXT;
                    item.iImage = 0;
                    item.pszText = lpValueName;
                    item.iItem = ListView_GetItemCount(hStartupListCtrl);
                    item.lParam = 0;
                    (void)ListView_InsertItem(hStartupListCtrl, &item);

                    if (dwType == REG_SZ)
                    {
                        GetLongPathName(Data, Data, (DWORD) _tcsclen(Data));
                        item.pszText = Data;
                        item.iSubItem = 1;
                        SendMessage(hStartupListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                    }
                    
                    switch (PtrToLong(hRootKey))
                    {
                    case PtrToLong(HKEY_LOCAL_MACHINE):
                        _tcscpy(Path, _T("HKLM\\\0"));
                        break;
                    case PtrToLong(HKEY_CURRENT_USER):
                        _tcscpy(Path, _T("HKCU\\\0"));
                        break;
                    default:
                        _tcscpy(Path, _T("\0"));
                    }

                    _tcscat(Path, KeyName);
                    item.pszText = Path;
                    item.iSubItem = 2;
                    SendMessage(hStartupListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                    HeapFree(GetProcessHeap(), 0, Data);
                }
            }
        }
        RegCloseKey(hKey);
    }

}
