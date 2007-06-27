/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/timedate/internettime.c
 * PURPOSE:     Internet Time property page
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include <timedate.h>

static VOID
CreateNTPServerList(HWND hwnd)
{
    HWND hList;
    TCHAR szValName[MAX_VALUE_NAME];
    TCHAR szData[256];
    DWORD dwIndex = 0;
    DWORD dwValSize;
    DWORD dwNameSize;
    DWORD dwDefault = 1;
    LONG lRet;
    HKEY hKey;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
        return;

    while (TRUE)
    {
        dwValSize = MAX_VALUE_NAME * sizeof(TCHAR);
        szValName[0] = '\0';
        lRet = RegEnumValue(hKey,
                            dwIndex,
                            szValName,
                            &dwValSize,
                            NULL,
                            NULL,
                            (LPBYTE)szData,
                            &dwNameSize);
        if (lRet == ERROR_SUCCESS)
        {
            /* get date from default reg value */
            if (_tcscmp(szValName, _T("")) == 0) // if (Index == 0)
            {
                dwDefault = _ttoi(szData);
                dwIndex++;
            }
            else
            {
                SendMessage(hList,
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

    /* server reg entries count from 1,
     * combo boxes count from 0 */
    dwDefault--;

    SendMessage(hList,
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
    TCHAR szSel[4];
    LONG lRet;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    uSel = (UINT)SendMessage(hList, CB_GETCURSEL, 0, 0);

    /* server reg entries count from 1,
     * combo boxes count from 0 */
    uSel++;

    /* convert to wide char */
    _itow(uSel, szSel, 10);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers"),
                        0,
                        KEY_SET_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        DisplayWin32Error(lRet);
        return;
    }

    lRet = RegSetValueExW(hKey,
                          _T(""),
                          0,
                          REG_SZ,
                          (LPBYTE)szSel,
                          sizeof(szSel));
    if (lRet != ERROR_SUCCESS)
        DisplayWin32Error(lRet);

    RegCloseKey(hKey);
}


/* get the domain name from the registry */
static BOOL
GetNTPServerAddress(LPTSTR *lpAddress)
{
    HKEY hKey;
    TCHAR szSel[4];
    DWORD dwSize;
    LONG lRet;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers"),
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (lRet != ERROR_SUCCESS)
        goto fail;

    /* Get data from default value */
    dwSize = 4 * sizeof(TCHAR);
    lRet = RegQueryValueEx(hKey,
                           _T(""),
                           NULL,
                           NULL,
                           (LPBYTE)szSel,
                           &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto fail;

    dwSize = 0;
    lRet = RegQueryValueEx(hKey,
                           szSel,
                           NULL,
                           NULL,
                           NULL,
                           &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto fail;

    (*lpAddress) = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                     0,
                                     dwSize);
    if ((*lpAddress) == NULL)
    {
        lRet = ERROR_NOT_ENOUGH_MEMORY;
        goto fail;
    }

    lRet = RegQueryValueEx(hKey,
                           szSel,
                           NULL,
                           NULL,
                           (LPBYTE)*lpAddress,
                           &dwSize);
    if (lRet != ERROR_SUCCESS)
        goto fail;

    RegCloseKey(hKey);

    return TRUE;

fail:
    DisplayWin32Error(lRet);
    if (hKey)
        RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, *lpAddress);
    return FALSE;
}


/* request the time from the current NTP server */
static ULONG
GetTimeFromServer(VOID)
{
    LPTSTR lpAddress = NULL;
    ULONG ulTime = 0;

    if (GetNTPServerAddress(&lpAddress))
    {
        ulTime = GetServerTime(lpAddress);

        HeapFree(GetProcessHeap(),
                 0,
                 lpAddress);
    }

    return ulTime;
}

/*
 * NTP servers state the number of seconds passed since
 * 1st Jan, 1900. The time returned from the server
 * needs adding to that date to get the current Gregorian time
 */
static VOID
UpdateSystemTime(ULONG ulTime)
{
    FILETIME ftNew;
    LARGE_INTEGER li;
    SYSTEMTIME stNew;

    /* time at 1st Jan 1900 */
    stNew.wYear = 1900;
    stNew.wMonth = 1;
    stNew.wDay = 1;
    stNew.wHour = 0;
    stNew.wMinute = 0;
    stNew.wSecond = 0;
    stNew.wMilliseconds = 0;

    /* convert to a file time */
    if (!SystemTimeToFileTime(&stNew, &ftNew))
    {
        DisplayWin32Error(GetLastError());
        return;
    }

    /* add on the time passed since 1st Jan 1900 */
    li = *(LARGE_INTEGER *)&ftNew;
    li.QuadPart += (LONGLONG)10000000 * ulTime;
    ftNew = * (FILETIME *)&li;

    /* convert back to a system time */
    if (!FileTimeToSystemTime(&ftNew, &stNew))
    {
        DisplayWin32Error(GetLastError());
        return;
    }

    if (!SystemSetLocalTime(&stNew))
         DisplayWin32Error(GetLastError());
}


static VOID
EnableDialogText(HWND hwnd)
{
    BOOL bChecked;
    UINT uCheck;

    uCheck = (UINT)SendDlgItemMessage(hwnd, IDC_AUTOSYNC, BM_GETCHECK, 0, 0);
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
    TCHAR szData[8];
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Parameters"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
    {
        dwSize = 8 * sizeof(TCHAR);
        if (RegQueryValueEx(hKey,
                            _T("Type"),
                            NULL,
                            NULL,
                            (LPBYTE)szData,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (_tcscmp(szData, _T("NTP")) == 0)
                SendDlgItemMessage(hwnd, IDC_AUTOSYNC, BM_SETCHECK, 0, 0);
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
                    ULONG ulTime;

                    SetNTPServer(hwndDlg);

                    ulTime = GetTimeFromServer();
                    if (ulTime != 0)
                        UpdateSystemTime(ulTime);
                }
                break;

                case IDC_SERVERLIST:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
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
                    return TRUE;

                default:
                    break;
            }
        }
        break;
    }

    return FALSE;
}
