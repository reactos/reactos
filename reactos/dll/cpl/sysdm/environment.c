/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/environment.c
 * PURPOSE:     Environment variable settings
 * COPYRIGHT:   Copyright Eric Kohl
 *
 */

#include "precomp.h"


typedef struct _VARIABLE_DATA
{
    LPTSTR lpName;
    LPTSTR lpRawValue;
    LPTSTR lpCookedValue;
} VARIABLE_DATA, *PVARIABLE_DATA;


INT_PTR CALLBACK
EditVariableDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PVARIABLE_DATA VarData;
    DWORD dwNameLength;
    DWORD dwValueLength;

    VarData = (PVARIABLE_DATA)GetWindowLongPtr(hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (LONG_PTR)lParam);
            VarData = (PVARIABLE_DATA)lParam;

            if (VarData->lpName != NULL)
            {
                SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_SETTEXT, 0, (LPARAM)VarData->lpName);
            }

            if (VarData->lpRawValue != NULL)
            {
                SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_SETTEXT, 0, (LPARAM)VarData->lpRawValue);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    dwNameLength = (DWORD)SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_GETTEXTLENGTH, 0, 0);
                    dwValueLength = (DWORD)SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_GETTEXTLENGTH, 0, 0);
                    if (dwNameLength != 0 && dwValueLength != 0)
                    {
                        if (VarData->lpName == NULL)
                        {
                            VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
                        }
                        else if (_tcslen(VarData->lpName) < dwNameLength)
                        {
                            GlobalFree(VarData->lpName);
                            VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
                        }
                        SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_GETTEXT, dwNameLength + 1, (LPARAM)VarData->lpName);

                        if (VarData->lpRawValue == NULL)
                        {
                            VarData->lpRawValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
                        }
                        else if (_tcslen(VarData->lpRawValue) < dwValueLength)
                        {
                            GlobalFree(VarData->lpRawValue);
                            VarData->lpRawValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
                        }
                        SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_GETTEXT, dwValueLength + 1, (LPARAM)VarData->lpRawValue);

                        if (_tcschr(VarData->lpRawValue, _T('%')))
                        {
                            if (VarData->lpCookedValue == NULL)
                            {
                                VarData->lpCookedValue = GlobalAlloc(GPTR, 2 * MAX_PATH * sizeof(TCHAR));
                            }

                            ExpandEnvironmentStrings(VarData->lpRawValue,
                                                     VarData->lpCookedValue,
                                                     2 * MAX_PATH);
                        }
                        else if (VarData->lpCookedValue)
                        {
                            GlobalFree(VarData->lpCookedValue);
                        }
                    }
                    EndDialog(hwndDlg, 1);
                    return TRUE;
                
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static VOID
SetEnvironmentVariables(HWND hwndListView,
                        HKEY hRootKey,
                        LPTSTR lpSubKeyName)
{
    HKEY hKey;
    DWORD dwValues;
    DWORD dwMaxValueNameLength;
    DWORD dwMaxValueDataLength;
    DWORD i;
    LPTSTR lpName;
    LPTSTR lpData;
    LPTSTR lpExpandData = NULL;
    DWORD dwNameLength;
    DWORD dwDataLength;
    DWORD dwType;
    PVARIABLE_DATA VarData;

    LV_ITEM lvi;
    int iItem;

    if (RegOpenKeyEx(hRootKey,
                     lpSubKeyName,
                     0,
                     KEY_READ,
                     &hKey))
        return;

    if (RegQueryInfoKey(hKey,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwValues,
                        &dwMaxValueNameLength,
                        &dwMaxValueDataLength,
                        NULL,
                        NULL))
    {
        RegCloseKey(hKey);
        return;
    }

    lpName = GlobalAlloc(GPTR, (dwMaxValueNameLength + 1) * sizeof(TCHAR));
    if (lpName == NULL)
    {
        RegCloseKey(hKey);
        return;
    }

    lpData = GlobalAlloc(GPTR, (dwMaxValueDataLength + 1) * sizeof(TCHAR));
    if (lpData == NULL)
    {
        GlobalFree(lpData);
        RegCloseKey(hKey);
        return;
    }

    for (i = 0; i < dwValues; i++)
    {
        dwNameLength = dwMaxValueNameLength + 1;
        dwDataLength = dwMaxValueDataLength + 1;

        if (RegEnumValue(hKey,
                 i,
                 lpName,
                 &dwNameLength,
                 NULL,
                 &dwType,
                 (LPBYTE)lpData,
                 &dwDataLength))
        {
            GlobalFree(lpName);
            GlobalFree(lpData);
            RegCloseKey(hKey);
            return;
        }

        VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));

        VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpName, lpName);

        VarData->lpRawValue = GlobalAlloc(GPTR, (dwDataLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpRawValue, lpData);

        if (dwType == REG_EXPAND_SZ)
        {
            lpExpandData = GlobalAlloc(GPTR, MAX_PATH * 2* sizeof(TCHAR));
            if (lpExpandData == NULL)
            {
                GlobalFree(lpName);
                GlobalFree(lpData);
                RegCloseKey(hKey);
                return;
            }

            ExpandEnvironmentStrings(lpData,
                                     lpExpandData,
                                     2 * MAX_PATH);

            VarData->lpCookedValue = GlobalAlloc(GPTR, (_tcslen(lpExpandData) + 1) * sizeof(TCHAR));
            _tcscpy(VarData->lpCookedValue, lpExpandData);
            GlobalFree(lpExpandData);
        }

        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        lvi.lParam = (LPARAM)VarData;
        lvi.pszText = VarData->lpName;
        lvi.state = (i == 0) ? LVIS_SELECTED : 0;
        iItem = ListView_InsertItem(hwndListView, &lvi);

        ListView_SetItemText(hwndListView, iItem, 1,
                             (VarData->lpCookedValue) ? VarData->lpCookedValue : VarData->lpRawValue);
    }

    GlobalFree(lpName);
    GlobalFree(lpData);
    RegCloseKey(hKey);
}


static VOID
SetListViewColumns(HWND hwndListView)
{
    RECT rect;
    LV_COLUMN column;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    memset(&column, 0x00, sizeof(column));
    column.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt=LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.32);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_VARIABLE, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.63);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_VALUE, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);
}


static VOID
OnInitDialog(HWND hwndDlg)
{
    HWND hwndListView;

    /* Set user environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetListViewColumns(hwndListView);

    SetEnvironmentVariables(hwndListView,
                            HKEY_CURRENT_USER,
                            _T("Environment"));

    (void)ListView_SetColumnWidth(hwndListView,2,LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView,0,LVIS_FOCUSED,LVIS_FOCUSED);

    (void)ListView_Update(hwndListView,0);


    /* Set system environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetListViewColumns(hwndListView);

    SetEnvironmentVariables(hwndListView,
                            HKEY_LOCAL_MACHINE,
                            _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"));

    (void)ListView_SetColumnWidth(hwndListView,2,LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView,0,LVIS_FOCUSED,LVIS_FOCUSED);

    (void)ListView_Update(hwndListView,0);
}


VOID
OnNewVariable(HWND hwndDlg,
              int iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    int iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));

    if (!DialogBoxParam(hApplet,
                        MAKEINTRESOURCE(IDD_EDIT_VARIABLE),
                        hwndDlg,
                        (DLGPROC) EditVariableDlgProc,
                        (LPARAM)VarData) > 0)
    {
        if (VarData->lpName != NULL)
            GlobalFree(VarData->lpName);

        if (VarData->lpRawValue != NULL)
            GlobalFree(VarData->lpRawValue);

        if (VarData->lpCookedValue != NULL)
            GlobalFree(VarData->lpCookedValue);
    
        GlobalFree(VarData);
    } 
    else
    {
        if(VarData->lpName != NULL && (VarData->lpCookedValue || VarData->lpRawValue))
        {
            memset(&lvi, 0x00, sizeof(lvi));
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.lParam = (LPARAM)VarData;
            lvi.pszText = VarData->lpName;
            lvi.state = 0;
            iItem = ListView_InsertItem(hwndListView, &lvi);

            ListView_SetItemText(hwndListView, iItem, 1,
                         (VarData->lpCookedValue) ? VarData->lpCookedValue : VarData->lpRawValue);
        }
    }
}


VOID
OnEditVariable(HWND hwndDlg,
               int iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    int iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = ListView_GetSelectionMark(hwndListView);
    if (iItem != -1)
    {
        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            VarData = (PVARIABLE_DATA)lvi.lParam;

            if (DialogBoxParam(hApplet,
                               MAKEINTRESOURCE(IDD_EDIT_VARIABLE),
                               hwndDlg,
                               (DLGPROC) EditVariableDlgProc,
                               (LPARAM)VarData) > 0)
            {
                ListView_SetItemText(hwndListView, iItem, 0, VarData->lpName);
                ListView_SetItemText(hwndListView, iItem, 1,
                                     (VarData->lpCookedValue) ? VarData->lpCookedValue : VarData->lpRawValue);
            }
        }
    }
}


VOID
OnDeleteVariable(HWND hwndDlg,
                 int iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    int iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = ListView_GetSelectionMark(hwndListView);
    if (iItem != -1)
    {
        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            VarData = (PVARIABLE_DATA)lvi.lParam;
            if (VarData != NULL)
            {
                if (VarData->lpName != NULL)
                    GlobalFree(VarData->lpName);

                if (VarData->lpRawValue != NULL)
                    GlobalFree(VarData->lpRawValue);

                if (VarData->lpCookedValue != NULL)
                    GlobalFree(VarData->lpCookedValue);
                
                GlobalFree(VarData);
                lvi.lParam = 0;
            }
        }
        
        (void)ListView_DeleteItem(hwndListView, iItem);
    }
}


VOID
ReleaseListViewItems(HWND hwndDlg,
                     int iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    int nItemCount;
    LV_ITEM lvi;
    int i;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    memset(&lvi, 0x00, sizeof(lvi));

    nItemCount = ListView_GetItemCount(hwndListView);
    for (i = 0; i < nItemCount; i++)
    {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            VarData = (PVARIABLE_DATA)lvi.lParam;
            if (VarData != NULL)
            {
                if (VarData->lpName != NULL)
                    GlobalFree(VarData->lpName);

                if (VarData->lpRawValue != NULL)
                    GlobalFree(VarData->lpRawValue);

                if (VarData->lpCookedValue != NULL)
                    GlobalFree(VarData->lpCookedValue);

                GlobalFree(VarData);
                lvi.lParam = 0;
            }
        }
    }
}

VOID
SetAllVars(HWND hwndDlg,
           INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;
    HKEY hk;
    DWORD Type = 0;

    memset(&lvi, 0x00, sizeof(lvi));

    /* Get the handle to the list box with all system vars in it */
    hwndListView = GetDlgItem(hwndDlg, iDlgItem);
    /* first item is 0 */
    iItem = 0;
    /* set up struct to retreive item */
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;

    /* Open or create the key */
    if (RegCreateKeyEx((iDlgItem == IDC_SYSTEM_VARIABLE_LIST ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                       (iDlgItem == IDC_SYSTEM_VARIABLE_LIST ? _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment") : _T("Environment")),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE | KEY_READ,
                       NULL,
                       &hk,
                       NULL))
    {
        return;
    }

    /* loop through all system variables */
    while(ListView_GetItem(hwndListView, &lvi))
    {
        /* Get the data in each item */
        VarData = (PVARIABLE_DATA)lvi.lParam;
        if (VarData != NULL)
        {
            /* Get the type */
            RegQueryValueEx(hk,VarData->lpName,NULL,&Type,NULL,NULL);

            /* Set the new value */
            if (RegSetValueEx(hk,
                              VarData->lpName,
                              0,
                              Type,
                              (LPBYTE) VarData->lpRawValue,
                              (DWORD) (_tcslen(VarData->lpRawValue)* sizeof(TCHAR))+1))  // was _tcsclen. lstrlen?
            {
                RegCloseKey(hk);
                return;
            }
        }
        /* Fill struct for next item */
        lvi.mask = LVIF_PARAM;
        lvi.iItem = ++iItem;
    }

    RegCloseKey(hk);
}

/* Environment dialog procedure */
INT_PTR CALLBACK
EnvironmentDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_VARIABLE_NEW:
                    OnNewVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_USER_VARIABLE_EDIT:
                    OnEditVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_USER_VARIABLE_DELETE:
                    OnDeleteVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_NEW:
                    OnNewVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_EDIT:
                    OnEditVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_DELETE:
                    OnDeleteVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDOK:
                    SetAllVars(hwndDlg, IDC_USER_VARIABLE_LIST);
                    SetAllVars(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);

                case IDCANCEL:
                    ReleaseListViewItems(hwndDlg, IDC_USER_VARIABLE_LIST);
                    ReleaseListViewItems(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;
        
        case WM_NOTIFY:
        {
            NMHDR *phdr;

            phdr = (NMHDR*)lParam;
            switch(phdr->code)
            {
                case NM_DBLCLK:
                {
                    if (phdr->idFrom == IDC_USER_VARIABLE_LIST ||
                        phdr->idFrom == IDC_SYSTEM_VARIABLE_LIST)
                    {
                        OnEditVariable(hwndDlg, (INT)phdr->idFrom);
                        return TRUE;
                    }
                }
            }
        }
        break;
    }
    
    return FALSE;
}

/* EOF */
