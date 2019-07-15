/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/internettime.c
 * PURPOSE:     Internet Time property page
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "timedate.h"
#include <stdlib.h>

DWORD WINAPI W32TimeSyncNow(LPCWSTR cmdline, UINT blocking, UINT flags);

static VOID
CreateNTPServerList(HWND hwnd)
{
    HWND hList;
    WCHAR szValName[MAX_VALUE_NAME];
    WCHAR szData[256];
    DWORD dwIndex = 0;
    DWORD dwValSize;
    DWORD dwNameSize;
    DWORD dwDefault = 1;
    LONG lRet;
    HKEY hKey;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
        return;

    while (TRUE)
    {
        dwValSize = MAX_VALUE_NAME * sizeof(WCHAR);
        szValName[0] = L'\0';
        lRet = RegEnumValueW(hKey,
                             dwIndex,
                             szValName,
                             &dwValSize,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &dwNameSize);
        if (lRet == ERROR_SUCCESS)
        {
            /* Get date from default reg value */
            if (wcscmp(szValName, L"") == 0) // if (Index == 0)
            {
                dwDefault = _wtoi(szData);
                dwIndex++;
            }
            else
            {
                SendMessageW(hList,
                             CB_ADDSTRING,
                             0,
                             (LPARAM)szData);
                dwIndex++;
            }
        }
        else if (lRet != ERROR_MORE_DATA)
        {
            break;
        }
    }

    if (dwDefault < 1 || dwDefault > dwIndex)
        dwDefault = 1;

    /* Server reg entries count from 1,
     * Combo boxes count from 0 */
    dwDefault--;

    SendMessageW(hList,
                 CB_SETCURSEL,
                 dwDefault,
                 0);

    RegCloseKey(hKey);
}


/* Set the selected server in the registry */
static VOID
SetNTPServer(HWND hwnd)
{
    HKEY hKey;
    HWND hList;
    UINT uSel;
    WCHAR szSel[4];
    LONG lRet;
    WCHAR buffer[256];

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    uSel = (UINT)SendMessageW(hList, CB_GETCURSEL, 0, 0);

    SendDlgItemMessageW(hwnd, IDC_SERVERLIST, WM_GETTEXT, _countof(buffer), (LPARAM)buffer);

    /* If there is new data entered then save it in the registry 
       The same key name of "0" is used to store all user entered values
    */
    if (uSel == -1)
    {
        lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                             0,
                             KEY_SET_VALUE,
                             &hKey);
        if (lRet != ERROR_SUCCESS)
        {
            DisplayWin32Error(lRet);
            return;
        }
        lRet = RegSetValueExW(hKey,
                              L"0",
                              0,
                              REG_SZ,
                              (LPBYTE)buffer,
                              (wcslen(buffer) + 1) * sizeof(WCHAR));
        if (lRet != ERROR_SUCCESS)
            DisplayWin32Error(lRet);
    }

    /* Server reg entries count from 1,
     * Combo boxes count from 0 */
    uSel++;

    /* Convert to wide char */
    _itow(uSel, szSel, 10);

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                         0,
                         KEY_SET_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        DisplayWin32Error(lRet);
        return;
    }

    lRet = RegSetValueExW(hKey,
                          L"",
                          0,
                          REG_SZ,
                          (LPBYTE)szSel,
                          (wcslen(szSel) + 1) * sizeof(WCHAR));
    if (lRet != ERROR_SUCCESS)
        DisplayWin32Error(lRet);

    RegCloseKey(hKey);
}


static VOID
EnableDialogText(HWND hwnd)
{
    BOOL bChecked;
    UINT uCheck;

    uCheck = (UINT)SendDlgItemMessageW(hwnd, IDC_AUTOSYNC, BM_GETCHECK, 0, 0);
    bChecked = (uCheck == BST_CHECKED) ? TRUE : FALSE;

    EnableWindow(GetDlgItem(hwnd, IDC_SERVERTEXT), bChecked);
    EnableWindow(GetDlgItem(hwnd, IDC_SERVERLIST), bChecked);
    EnableWindow(GetDlgItem(hwnd, IDC_UPDATEBUTTON), bChecked);
    EnableWindow(GetDlgItem(hwnd, IDC_SUCSYNC), bChecked);
    EnableWindow(GetDlgItem(hwnd, IDC_NEXTSYNC), bChecked);
}


static VOID
GetSyncSetting(HWND hwnd)
{
    HKEY hKey;
    WCHAR szData[8];
    DWORD dwSize;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\Parameters",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        dwSize = 8 * sizeof(WCHAR);
        if (RegQueryValueExW(hKey,
                             L"Type",
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &dwSize) == ERROR_SUCCESS)
        {
            if (wcscmp(szData, L"NTP") == 0)
                SendDlgItemMessageW(hwnd, IDC_AUTOSYNC, BM_SETCHECK, BST_CHECKED, 0);
            else
                SendDlgItemMessageW(hwnd, IDC_AUTOSYNC, BM_SETCHECK, BST_UNCHECKED, 0);
        }

        RegCloseKey(hKey);
    }
}


static VOID
OnInitDialog(HWND hwnd)
{
    GetSyncSetting(hwnd);
    EnableDialogText(hwnd);
    CreateNTPServerList(hwnd);
}

static VOID
OnAutoSync(BOOL Sync)
{
    HKEY hKey;
    LONG lRet;
    LPCWSTR szAuto;

    if (Sync)
        szAuto = L"NTP";
    else
        szAuto = L"NoSync";

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\Parameters",
                         0,
                         KEY_SET_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        DisplayWin32Error(lRet);
        return;
    }

    lRet = RegSetValueExW(hKey,
                          L"Type",
                          0,
                          REG_SZ,
                          (LPBYTE)szAuto,
                          (wcslen(szAuto) + 1) * sizeof(WCHAR));
    if (lRet != ERROR_SUCCESS)
        DisplayWin32Error(lRet);

    RegCloseKey(hKey);
}

/* Property page dialog callback */
INT_PTR CALLBACK
InetTimePageProc(HWND hwndDlg,
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
            switch(LOWORD(wParam))
            {
                case IDC_UPDATEBUTTON:
                {
                    DWORD dwError;

                    SetNTPServer(hwndDlg);

                    dwError = W32TimeSyncNow(L"localhost", 0, 0);
                    if (dwError != ERROR_SUCCESS)
                    {
                        DisplayWin32Error(dwError);
                    }
                }
                break;

                case IDC_SERVERLIST:
                    if ((HIWORD(wParam) == CBN_SELCHANGE) || (HIWORD(wParam) == CBN_EDITCHANGE))
                    {
                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_AUTOSYNC:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EnableDialogText(hwndDlg);

                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                    SetNTPServer(hwndDlg);

                    if (SendDlgItemMessageW(hwndDlg, IDC_AUTOSYNC, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        OnAutoSync(TRUE);
                    else
                        OnAutoSync(FALSE);

                    return TRUE;

                default:
                    break;
            }
        }
        break;
    }

    return FALSE;
}
