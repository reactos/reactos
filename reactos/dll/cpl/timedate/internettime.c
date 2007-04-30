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
    WCHAR ValName[MAX_VALUE_NAME];
    WCHAR Data[256];
    DWORD Index = 0;
    DWORD ValSize;
    DWORD dwNameSize;
    DWORD Default = 1;
    LONG Ret;
    HKEY hKey;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
        return;

    while (TRUE)
    {
            ValSize = MAX_VALUE_NAME;
            ValName[0] = '\0';
            Ret = RegEnumValueW(hKey,
                                Index,
                                ValName,
                                &ValSize,
                                NULL,
                                NULL,
                                (LPBYTE)Data,
                                &dwNameSize);

            if (Ret == ERROR_SUCCESS)
            {
                /* get date from default reg value */
                if (wcscmp(ValName, L"") == 0) // if (Index == 0)
                {
                    Default = _wtoi(Data);
                    Index++;
                }
                else
                {
                    SendMessageW(hList,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)Data);
                    Index++;
                }
            }
            else if (Ret != ERROR_MORE_DATA)
                break;
    }

    if (Default < 1 || Default > Index)
        Default = 1;

    /* server reg entries count from 1,
     * combo boxes count from 0 */
    Default--;

    SendMessage(hList,
                CB_SETCURSEL,
                Default,
                0);

    RegCloseKey(hKey);

}


/* Set the selected server in the registry */
static VOID
SetNTPServer(HWND hwnd)
{
    HKEY hKey;
    HWND hList;
    UINT Sel;
    WCHAR szSel[4];
    LONG Ret;

    hList = GetDlgItem(hwnd,
                       IDC_SERVERLIST);

    Sel = (UINT)SendMessage(hList,
                            CB_GETCURSEL,
                            0,
                            0);

    /* server reg entries count from 1,
     * combo boxes count from 0 */
    Sel++;

    /* convert to wide char */
    _itow(Sel, szSel, 10);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_SET_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        DisplayWin32Error(Ret);
        return;
    }

    Ret = RegSetValueExW(hKey,
                         L"",
                         0,
                         REG_SZ,
                         (LPBYTE)szSel,
                         sizeof(szSel));
    if (Ret != ERROR_SUCCESS)
        DisplayWin32Error(Ret);

    RegCloseKey(hKey);

}


/* get the domain name from the registry */
static BOOL
GetNTPServerAddress(LPWSTR* lpAddress)
{
    HKEY hKey;
    WCHAR szSel[4];
    DWORD dwSize;
    LONG Ret;

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        goto fail;
    }

    /* Get data from default value */
    dwSize = sizeof(szSel);
    Ret = RegQueryValueExW(hKey,
                           L"",
                           NULL,
                           NULL,
                           (LPBYTE)szSel,
                           &dwSize);
    if (Ret != ERROR_SUCCESS)
        goto fail;


    dwSize = 0;
    Ret = RegQueryValueExW(hKey,
                           szSel,
                           NULL,
                           NULL,
                           NULL,
                           &dwSize);
    if (Ret == ERROR_SUCCESS)
    {
        (*lpAddress) = (LPWSTR) HeapAlloc(GetProcessHeap(),
                                          0,
                                          dwSize);
        if ((*lpAddress) == NULL)
        {
            Ret = ERROR_NOT_ENOUGH_MEMORY;
            goto fail;
        }

        Ret = RegQueryValueExW(hKey,
                               szSel,
                               NULL,
                               NULL,
                               (LPBYTE)*lpAddress,
                               &dwSize);
        if (Ret != ERROR_SUCCESS)
            goto fail;

    }
    else
        goto fail;

    RegCloseKey(hKey);

    return TRUE;

fail:
    DisplayWin32Error(Ret);
    if (hKey) RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, *lpAddress);
    return FALSE;

}


/* request the time from the current NTP server */
static ULONG
GetTimeFromServer(VOID)
{
    LPWSTR lpAddress = NULL;
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
    if (! SystemTimeToFileTime(&stNew, &ftNew))
    {
        DisplayWin32Error(GetLastError());
        return;
    }

    /* add on the time passed since 1st Jan 1900 */
    li = *(LARGE_INTEGER *)&ftNew;
    li.QuadPart += (LONGLONG)10000000 * ulTime;
    ftNew = * (FILETIME *)&li;

    /* convert back to a system time */
    if (! FileTimeToSystemTime(&ftNew, &stNew))
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
    HWND hCheck = GetDlgItem(hwnd, IDC_AUTOSYNC);
    UINT Check = (UINT)SendMessageW(hCheck, BM_GETCHECK, 0, 0);

    bChecked = (Check == BST_CHECKED) ? TRUE : FALSE;

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
    HWND hCheck;
    WCHAR Data[8];
    DWORD Size;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Parameters",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey) == ERROR_SUCCESS)
        {
        Size = sizeof(Data);
        if (RegQueryValueExW(hKey,
                           L"Type",
                           NULL,
                           NULL,
                           (LPBYTE)Data,
                           &Size) == ERROR_SUCCESS)
        {
            if (lstrcmp(Data, L"NTP") == 0)
            {
                hCheck = GetDlgItem(hwnd, IDC_AUTOSYNC);
                SendMessageW(hCheck, BM_SETCHECK, 0, 0);
            }
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
        {
            OnInitDialog(hwndDlg);
        }
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
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;

                case IDC_AUTOSYNC:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EnableDialogText(hwndDlg);

                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
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
