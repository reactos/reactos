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

static TCHAR BugLink[] = _T("http://jira.reactos.org/");

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

static const WCHAR ProductOptions[] = L"System\\CurrentControlSet\\Control\\ProductOptions";

static BOOL
DoGetProductType(PNT_PRODUCT_TYPE ProductType)
{
    HKEY hKey;
    LONG error;
    WCHAR szValue[32];
    DWORD cbValue;

    *ProductType = NtProductServer;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ProductOptions, 0, KEY_READ, &hKey);
    if (error)
        return FALSE;

    cbValue = sizeof(szValue);
    error = RegQueryValueExW(hKey, L"ProductType", NULL, NULL, (LPBYTE)szValue, &cbValue);
    if (!error)
    {
        if (lstrcmpW(szValue, L"WinNT") == 0)
            *ProductType = NtProductWinNt;
        else if (lstrcmpW(szValue, L"LanmanNT") == 0)
            *ProductType = NtProductLanManNt;
    }

    RegCloseKey(hKey);
    return TRUE;
}

static BOOL
DoSetProductType(NT_PRODUCT_TYPE ProductType)
{
    HKEY hKey;
    LONG error;
    DWORD cbValue;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ProductOptions, 0, KEY_WRITE, &hKey);
    if (error)
        return FALSE;

    switch (ProductType)
    {
        case NtProductWinNt:
            cbValue = sizeof(L"WinNT");
            error = RegSetValueExW(hKey, L"ProductType", 0, REG_SZ, (LPBYTE)L"WinNT", cbValue);
            break;
        case NtProductLanManNt:
            // FIXME: Not supported yet
            error = ERROR_NOT_SUPPORTED;
            break;
        case NtProductServer:
            cbValue = sizeof(L"ServerNT");
            error = RegSetValueExW(hKey, L"ProductType", 0, REG_SZ, (LPBYTE)L"ServerNT", cbValue);
            break;
    }

    RegCloseKey(hKey);
    return !error;
}

static BOOL
OnOK(HWND hwndDlg)
{
    INT iItem;
    iItem = SendDlgItemMessageW(hwndDlg, IDC_PRODUCTOPTIONS, CB_GETCURSEL, 0, 0);

    switch (iItem)
    {
        case 0:
            DoSetProductType(NtProductServer);
            return TRUE;
        case 1:
            DoSetProductType(NtProductWinNt);
            return TRUE;
        default:
            return FALSE;
    }
}

static VOID
OnInitSysSettingsDialog(HWND hwndDlg)
{
    NT_PRODUCT_TYPE Type;
    WCHAR szText[64];

    LoadStringW(hApplet, IDS_PRODUCTSERVER, szText, ARRAYSIZE(szText));
    SendDlgItemMessageW(hwndDlg, IDC_PRODUCTOPTIONS, CB_ADDSTRING, 0, (LPARAM)szText);

    LoadStringW(hApplet, IDS_PRODUCTWORKSTATION, szText, ARRAYSIZE(szText));
    SendDlgItemMessageW(hwndDlg, IDC_PRODUCTOPTIONS, CB_ADDSTRING, 0, (LPARAM)szText);

    if (DoGetProductType(&Type))
    {
        switch (Type)
        {
            case NtProductWinNt:
                SendDlgItemMessageW(hwndDlg, IDC_PRODUCTOPTIONS, CB_SETCURSEL, 1, 0);
                break;
            case NtProductLanManNt:
                // FIXME: Not supported yet
                break;
            case NtProductServer:
                SendDlgItemMessageW(hwndDlg, IDC_PRODUCTOPTIONS, CB_SETCURSEL, 0, 0);
                break;
        }
    }
}

static BOOL
DoEnableProcessPriviledge(LPCTSTR pszSE_)
{
    BOOL f;
    HANDLE hProcess;
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;
    
    f = FALSE;
    hProcess = GetCurrentProcess();
    if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        if (LookupPrivilegeValue(NULL, pszSE_, &luid))
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            tp.Privileges[0].Luid = luid;
            f = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
        }
        CloseHandle(hToken);
    }
    return f;
}

static void
DoExitOS(HWND hwndDlg)
{
    DoEnableProcessPriviledge(SE_SHUTDOWN_NAME);
    ExitWindowsEx(EWX_REBOOT, 0);
}

INT_PTR CALLBACK
SysSettingsDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    WCHAR szText[128], szTitle[64];

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
                    if (OnOK(hwndDlg))
                    {
                        LoadStringW(hApplet, IDS_REBOOTNOWTEXT, szText, ARRAYSIZE(szText));
                        LoadStringW(hApplet, IDS_REBOOTNOWTITLE, szTitle, ARRAYSIZE(szText));
                        if (MessageBoxW(NULL, szText, szTitle, MB_ICONWARNING | MB_YESNO) == IDYES)
                        {
                            DoExitOS(hwndDlg);
                        }
                        EndDialog(hwndDlg, IDOK);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;
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
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_VIRTMEM),
                              hwndDlg,
                              VirtMemDlgProc);
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
