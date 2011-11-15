/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/hardprof.c
 * PURPOSE:     Modify hardware profiles
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

/* Property page dialog callback */
static INT_PTR CALLBACK
RenameProfDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hwndDlg,
                          LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}


static
DWORD
GetUserWaitInterval(VOID)
{
    DWORD dwWaitInterval = 30;
    DWORD dwSize;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\IDConfigDB",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey))
        return dwWaitInterval;

    dwSize = sizeof(DWORD);
    RegQueryValueExW(hKey,
                     L"UserWaitInterval",
                     NULL,
                     NULL,
                     (LPBYTE)&dwWaitInterval,
                     &dwSize);

    RegCloseKey(hKey);

    return dwWaitInterval;
}


static
VOID
SetUserWaitInterval(DWORD dwWaitInterval)
{
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\IDConfigDB",
                      0,
                      KEY_SET_VALUE,
                      &hKey))
        return;

    RegSetValueExW(hKey,
                   L"UserWaitInterval",
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwWaitInterval,
                   sizeof(DWORD));

    RegCloseKey(hKey);
}


static
VOID
OnInitDialog(HWND hwndDlg)
{
    DWORD dwWaitInterval;

    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFUP),
                BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_UP)));
    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFDWN),
                BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_DOWN)));


    SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG((SHORT)500, 0));

    dwWaitInterval = GetUserWaitInterval();
    if (dwWaitInterval == (DWORD)-1)
    {
        CheckDlgButton(hwndDlg, IDC_HRDPROFWAIT, BST_CHECKED);
        SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETPOS, 0, 30);
        EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), FALSE);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_HRDPROFSELECT, BST_CHECKED);
        SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_SETPOS, 0, dwWaitInterval);
    }
}


static
VOID
OnOk(HWND hwndDlg)
{
    DWORD dwWaitInterval;

    if (IsDlgButtonChecked(hwndDlg, IDC_HRDPROFWAIT) == BST_CHECKED)
    {
        dwWaitInterval = (DWORD)-1;
    }
    else
    {
        dwWaitInterval = LOWORD(SendDlgItemMessageW(hwndDlg, IDC_HRDPROFUPDWN, UDM_GETPOS, 0, 0));
    }

    SetUserWaitInterval(dwWaitInterval);
}


/* Property page dialog callback */
INT_PTR CALLBACK
HardProfDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_HRDPROFRENAME:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_RENAMEPROFILE),
                              hwndDlg,
                              (DLGPROC)RenameProfDlgProc);
                    break;

                case IDC_HRDPROFWAIT:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), FALSE);
                    return TRUE;

                case IDC_HRDPROFSELECT:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HRDPROFEDIT), TRUE);
                    return TRUE;

                case IDOK:
                    OnOk(hwndDlg);

                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
