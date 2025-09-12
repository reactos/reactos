/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/advanced.c
 * PURPOSE:     Memory, start-up and profiles settings
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
                Copyright 2006 - 2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"
#define WIN32_NO_STATUS
#include "pstypes.h" /* SharedUserData */

static TCHAR BugLink[] = _T("http://jira.reactos.org/");
static TCHAR ReportAsWorkstationKey[] = _T("SYSTEM\\CurrentControlSet\\Control\\ReactOS\\Settings\\Version");


static VOID
OnOK(HWND hwndDlg)
{
    HKEY hKey;
    DWORD ReportAsWorkstation;

    ReportAsWorkstation = (SendDlgItemMessageW(hwndDlg,
                                               IDC_REPORTASWORKSTATION,
                                               BM_GETCHECK,
                                               0,
                                               0) == BST_CHECKED);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       ReportAsWorkstationKey,
                       0,
                       NULL,
                       0,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey,
                      _T("ReportAsWorkstation"),
                      0,
                      REG_DWORD,
                      (LPBYTE)&ReportAsWorkstation,
                      sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

static VOID
OnInitSysSettingsDialog(HWND hwndDlg)
{
    HKEY hKey;
    DWORD dwVal = 0;
    DWORD dwType = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    BOOL ReportAsWorkstation = SharedUserData->NtProductType == VER_NT_WORKSTATION;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     ReportAsWorkstationKey,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey,
                            _T("ReportAsWorkstation"),
                            0,
                            &dwType,
                            (LPBYTE)&dwVal,
                            &cbData) == ERROR_SUCCESS)
        {
            if (cbData == sizeof(DWORD))
                ReportAsWorkstation = dwVal != FALSE;
        }

        RegCloseKey(hKey);
    }
    SendDlgItemMessageW(hwndDlg, IDC_REPORTASWORKSTATION, BM_SETCHECK,
                        ReportAsWorkstation ? BST_CHECKED : BST_UNCHECKED, 0);
}

INT_PTR CALLBACK
SysSettingsDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitSysSettingsDialog(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    OnOK(hwndDlg);
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PERFOR:
                    ShowPerformanceOptions(hwndDlg);
                    break;

                case IDC_USERPROFILE:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_USERPROFILE),
                              hwndDlg,
                              UserProfileDlgProc);
                    break;

                case IDC_STAREC:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_STARTUPRECOVERY),
                              hwndDlg,
                              StartRecDlgProc);
                    break;

                case IDC_SYSSETTINGS:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_SYSSETTINGS),
                              hwndDlg,
                              SysSettingsDlgProc);
                    break;

                case IDC_ENVVAR:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ENVIRONMENT_VARIABLES),
                              hwndDlg,
                              EnvironmentDlgProc);
                    break;

                case IDC_ERRORREPORT:
                    ShellExecute(NULL,
                                 _T("open"),
                                 BugLink,
                                 NULL,
                                 NULL,
                                 SW_SHOWNORMAL);
                    break;
            }
        }

        break;
    }

    return FALSE;
}
