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
    DWORD dwType;
    LPTSTR lpName;
    LPTSTR lpRawValue;
    LPTSTR lpCookedValue;
} VARIABLE_DATA, *PVARIABLE_DATA;


static INT
GetSelectedListViewItem(HWND hwndListView)
{
    INT iCount;
    INT iItem;

    iCount = SendMessage(hwndListView,
                         LVM_GETITEMCOUNT,
                         0,
                         0);
    if (iCount != LB_ERR)
    {
        for (iItem = 0; iItem < iCount; iItem++)
        {
            if (SendMessage(hwndListView,
                            LVM_GETITEMSTATE,
                            iItem,
                            (LPARAM) LVIS_SELECTED) == LVIS_SELECTED)
            {
                return iItem;
            }
        }
    }

    return -1;
}


static INT_PTR CALLBACK
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
                    if (dwNameLength > 0 && dwValueLength > 0)
                    {
                        LPTSTR p;

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

                        if (VarData->lpCookedValue != NULL)
                        {
                            GlobalFree(VarData->lpCookedValue);
                            VarData->lpCookedValue = NULL;
                        }

                        p = _tcschr(VarData->lpRawValue, _T('%'));
                        if (p && _tcschr(++p, _T('%')))
                        {
                            VarData->dwType = REG_EXPAND_SZ;
                            VarData->lpCookedValue = GlobalAlloc(GPTR, 2 * MAX_PATH * sizeof(TCHAR));

                            ExpandEnvironmentStrings(VarData->lpRawValue,
                                                     VarData->lpCookedValue,
                                                     2 * MAX_PATH);
                        }
                        else
                        {
                            VarData->dwType = REG_SZ;
                            VarData->lpCookedValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
                            _tcscpy(VarData->lpCookedValue, VarData->lpRawValue);
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
GetEnvironmentVariables(HWND hwndListView,
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
    LPTSTR lpExpandData;
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

    lpExpandData = GlobalAlloc(GPTR, 2048 * sizeof(TCHAR));
    if (lpExpandData == NULL)
    {
        GlobalFree(lpName);
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

        if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
            continue;

        VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));

        VarData->dwType = dwType;

        VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpName, lpName);

        VarData->lpRawValue = GlobalAlloc(GPTR, (dwDataLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpRawValue, lpData);

        ExpandEnvironmentStrings(lpData, lpExpandData, 2048);

        VarData->lpCookedValue = GlobalAlloc(GPTR, (_tcslen(lpExpandData) + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpCookedValue, lpExpandData);

        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        lvi.lParam = (LPARAM)VarData;
        lvi.pszText = VarData->lpName;
        lvi.state = (i == 0) ? LVIS_SELECTED : 0;
        iItem = ListView_InsertItem(hwndListView, &lvi);

        ListView_SetItemText(hwndListView, iItem, 1, VarData->lpCookedValue);
    }

    GlobalFree(lpExpandData);
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

    GetEnvironmentVariables(hwndListView,
                            HKEY_CURRENT_USER,
                            _T("Environment"));

    (void)ListView_SetColumnWidth(hwndListView, 2, LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView, 0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    (void)ListView_Update(hwndListView,0);

    /* Set system environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetListViewColumns(hwndListView);

    GetEnvironmentVariables(hwndListView,
                            HKEY_LOCAL_MACHINE,
                            _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"));

    (void)ListView_SetColumnWidth(hwndListView, 2, LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView, 0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    (void)ListView_Update(hwndListView, 0);
}


static VOID
OnNewVariable(HWND hwndDlg,
              INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));

    if (!DialogBoxParam(hApplet,
                        MAKEINTRESOURCE(IDD_EDIT_VARIABLE),
                        hwndDlg,
                        (DLGPROC)EditVariableDlgProc,
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
        if (VarData->lpName != NULL && (VarData->lpCookedValue || VarData->lpRawValue))
        {
            memset(&lvi, 0x00, sizeof(lvi));
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.lParam = (LPARAM)VarData;
            lvi.pszText = VarData->lpName;
            lvi.state = 0;
            iItem = ListView_InsertItem(hwndListView, &lvi);

            ListView_SetItemText(hwndListView, iItem, 1, VarData->lpCookedValue);
        }
    }
}


static VOID
OnEditVariable(HWND hwndDlg,
               INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = GetSelectedListViewItem(hwndListView);
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
                               (DLGPROC)EditVariableDlgProc,
                               (LPARAM)VarData) > 0)
            {
                ListView_SetItemText(hwndListView, iItem, 0, VarData->lpName);
                ListView_SetItemText(hwndListView, iItem, 1, VarData->lpCookedValue);
            }
        }
    }
}


static VOID
OnDeleteVariable(HWND hwndDlg,
                 INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = GetSelectedListViewItem(hwndListView);
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

        /* Select the previous item */
        if (iItem > 0)
            iItem--;

        ListView_SetItemState(hwndListView, iItem,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
    }
}


static VOID
ReleaseListViewItems(HWND hwndDlg,
                     INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT nItemCount;
    INT i;

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


static VOID
SetAllVars(HWND hwndDlg,
           INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;
    HKEY hKey;
    DWORD dwValueCount;
    DWORD dwMaxValueNameLength;
    LPTSTR *aValueArray;
    DWORD dwNameLength;
    DWORD i;
    TCHAR szBuffer[256];
    LPTSTR lpBuffer;

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
                       &hKey,
                       NULL))
    {
        return;
    }

    /* Get the number of values and the maximum value name length */
    if (RegQueryInfoKey(hKey,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwValueCount,
                        &dwMaxValueNameLength,
                        NULL,
                        NULL,
                        NULL))
    {
        RegCloseKey(hKey);
        return;
    }

    if (dwValueCount > 0)
    {
        /* Allocate the value array */
        aValueArray = GlobalAlloc(GPTR, dwValueCount * sizeof(LPTSTR));
        if (aValueArray != NULL)
        {
            /* Get all value names */
            for (i = 0; i < dwValueCount; i++)
            {
                dwNameLength = 256;
                if (!RegEnumValue(hKey,
                                  i,
                                  szBuffer,
                                  &dwNameLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL))
                {
                    /* Allocate a value name buffer, fill it and attach it to the array */
                    lpBuffer = (LPTSTR)GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
                    if (lpBuffer != NULL)
                    {
                        _tcscpy(lpBuffer, szBuffer);
                        aValueArray[i] = lpBuffer;
                    }
                }
            }

            /* Delete all values */
            for (i = 0; i < dwValueCount; i++)
            {
                if (aValueArray[i] != NULL)
                {
                    /* Delete the value*/
                    RegDeleteValue(hKey,
                                   aValueArray[i]);

                    /* Free the value name */
                    GlobalFree(aValueArray[i]);
                }
            }

            /* Free the value array */
            GlobalFree(aValueArray);
        }
    }

    /* Loop through all variables */
    while (ListView_GetItem(hwndListView, &lvi))
    {
        /* Get the data in each item */
        VarData = (PVARIABLE_DATA)lvi.lParam;
        if (VarData != NULL)
        {
            /* Set the new value */
            if (RegSetValueEx(hKey,
                              VarData->lpName,
                              0,
                              VarData->dwType,
                              (LPBYTE)VarData->lpRawValue,
                              (DWORD)(_tcslen(VarData->lpRawValue) + 1) * sizeof(TCHAR)))
            {
                RegCloseKey(hKey);
                return;
            }
        }

        /* Fill struct for next item */
        lvi.mask = LVIF_PARAM;
        lvi.iItem = ++iItem;
    }

    RegCloseKey(hKey);
}


static BOOL
OnNotify(HWND hwndDlg, NMHDR *phdr)
{
    switch(phdr->code)
    {
        case NM_DBLCLK:
            if (phdr->idFrom == IDC_USER_VARIABLE_LIST ||
                phdr->idFrom == IDC_SYSTEM_VARIABLE_LIST)
            {
                OnEditVariable(hwndDlg, (INT)phdr->idFrom);
                return TRUE;
            }
            break;

        case LVN_KEYDOWN:
            if (((LPNMLVKEYDOWN)phdr)->wVKey == VK_DELETE &&
                (phdr->idFrom == IDC_USER_VARIABLE_LIST ||
                 phdr->idFrom == IDC_SYSTEM_VARIABLE_LIST))
            {
                OnDeleteVariable(hwndDlg, (INT)phdr->idFrom);
                return TRUE;
            }
            break;
    }

    return FALSE;
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
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;

        case WM_DESTROY:
            ReleaseListViewItems(hwndDlg, IDC_USER_VARIABLE_LIST);
            ReleaseListViewItems(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
            break;

        case WM_NOTIFY:
            return OnNotify(hwndDlg, (NMHDR*)lParam);
    }

    return FALSE;
}

/* EOF */
