/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/netid/netid.c
 * PURPOSE:     Network ID Page
 * COPYRIGHT:   Thomas Weidenmueller <w3seek@reactos.org>
 *              Dmitry Chapyshev <dmitry@reactos.org>
 *
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <windns.h>
#include <tchar.h>
#include <lm.h>
#include <prsht.h>

#include "resource.h"


#define MAX_COMPUTERDESCRIPTION_LENGTH 256

typedef struct _NETIDDATA
{
    WCHAR szHostName[256];
    WCHAR szDomainName[256];
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL bHostNameChanged;
    BOOL bDomainNameChanged;
    BOOL bEnable;
} NETIDDATA, *PNETIDDATA;


static HINSTANCE hDllInstance;

static
BOOL
GetComputerNames(
    PNETIDDATA pNetIdData)
{
    HKEY KeyHandle;
    DWORD dwSize;
    DWORD dwError;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\TcpIp\\Parameters",
                            0,
                            KEY_QUERY_VALUE,
                            &KeyHandle);
    if (dwError == ERROR_SUCCESS)
    {
        dwSize = sizeof(pNetIdData->szHostName);
        RegQueryValueExW(KeyHandle,
                         L"NV HostName",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szHostName,
                         &dwSize);

        dwSize = sizeof(pNetIdData->szDomainName);
        RegQueryValueExW(KeyHandle,
                         L"NV Domain",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szDomainName,
                         &dwSize);

        RegCloseKey(KeyHandle);
    }

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
                            0,
                            KEY_QUERY_VALUE,
                            &KeyHandle);
    if (dwError == ERROR_SUCCESS)
    {
        dwSize = sizeof(pNetIdData->szComputerName);
        RegQueryValueExW(KeyHandle,
                         L"ComputerName",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szComputerName,
                         &dwSize);

        RegCloseKey(KeyHandle);
    }

    return TRUE;
}

static
BOOL
IsValidDomainName(
    HWND hDlg,
    UINT uId)
{
    WCHAR szDomainName[256];
    DWORD dwError;

    GetDlgItemTextW(hDlg, uId, szDomainName, ARRAYSIZE(szDomainName));
    dwError = DnsValidateName_W(szDomainName, DnsNameDomain);
    if (dwError != ERROR_SUCCESS)
    {
        /* FIXME: Show error message */
        return FALSE;
    }

    return TRUE;
}

static
INT_PTR CALLBACK
DNSSuffixPropDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PNETIDDATA pNetIdData;

    pNetIdData = (PNETIDDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (Msg)
    {
        case WM_INITDIALOG:
            pNetIdData = (PNETIDDATA)lParam;
            if (pNetIdData != NULL)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pNetIdData);

                SetDlgItemTextW(hDlg, 1011, pNetIdData->szDomainName);

                SetDlgItemTextW(hDlg, 1013, pNetIdData->szComputerName);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 1011:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                    }
                    break;

                case IDOK:
                    if (!IsValidDomainName(hDlg, 1011))
                    {
                        SetFocus(GetDlgItem(hDlg, 1011));
                        break;
                    }

                    GetDlgItemTextW(hDlg, 1011, pNetIdData->szDomainName, ARRAYSIZE(pNetIdData->szDomainName));
                    pNetIdData->bDomainNameChanged = TRUE;
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
            }
            break;
    }

    return FALSE;
}

static VOID
SetRadioBtnState(HWND hDlg, BOOL IsDomain)
{
    SendDlgItemMessage(hDlg, 1008, BM_SETCHECK, (WPARAM)IsDomain, 0);
    SendDlgItemMessage(hDlg, 1004, BM_SETCHECK, (WPARAM)!IsDomain, 0);
    EnableWindow(GetDlgItem(hDlg, 116), IsDomain);
    EnableWindow(GetDlgItem(hDlg, 1007), !IsDomain);
}

static VOID
DisableControls(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, 1008), FALSE);
    EnableWindow(GetDlgItem(hDlg, 1004), FALSE);
    EnableWindow(GetDlgItem(hDlg, 116), FALSE);
    EnableWindow(GetDlgItem(hDlg, 1007), FALSE);
}

static
BOOL
IsValidComputerName(
    HWND hDlg,
    UINT uId)
{
    WCHAR szMsgText[512], szText[256], s[256];
    int i;

    GetWindowText(GetDlgItem(hDlg, uId), s, ARRAYSIZE(s));

    for (i = 0; i <= wcslen(s); i++)
    {
        if (s[i] == L'!' || s[i] == L'@' || s[i] == L'#' || s[i] == L'$'
            || s[i] == L'^' || s[i] == L'&' || s[i] == L'\\' || s[i] == L'|'
            || s[i] == L')' || s[i] == L'(' || s[i] == L'{' || s[i] == L'"'
            || s[i] == L'}' || s[i] == L'~' || s[i] == L'/' || s[i] == L'\''
            || s[i] == L'=' || s[i] == L':' || s[i] == L';' || s[i] == L'+'
            || s[i] == L'<' || s[i] == L'>' || s[i] == L'?' || s[i] == L'['
            || s[i] == L']' || s[i] == L'`' || s[i] == L'%' || s[i] == L'_'
            || s[i] == L'.')
        {
            LoadStringW(hDllInstance, 1030, szText, ARRAYSIZE(szText));
            swprintf(szMsgText, szText, s);
            MessageBoxW(hDlg, szMsgText, NULL, MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    return TRUE;
}

static
VOID
SetFullComputerName(
    HWND hDlg,
    UINT uId,
    PNETIDDATA pNetIdData)
{
    WCHAR szFullComputerName[512];

    swprintf(szFullComputerName, L"%s.%s", pNetIdData->szHostName, pNetIdData->szDomainName);
    SetDlgItemText(hDlg, uId, szFullComputerName);
}

static
VOID
UpdateFullComputerName(
    HWND hDlg,
    UINT uId,
    PNETIDDATA pNetIdData)
{
    WCHAR szFullComputerName[512];
    WCHAR szHostName[256];

    GetWindowText(GetDlgItem(hDlg, 1002), szHostName, ARRAYSIZE(szHostName));

    swprintf(szFullComputerName, L"%s.%s", szHostName, pNetIdData->szDomainName);
    SetDlgItemText(hDlg, uId, szFullComputerName);
}

static
VOID
NetworkDlg_OnInitDialog(
    HWND hDlg,
    PNETIDDATA pNetIdData)
{
    LPWKSTA_INFO_101 wki = NULL;
    TCHAR MsgText[MAX_PATH * 2];
    LPWSTR JoinName = NULL;
    NETSETUP_JOIN_STATUS JoinStatus;

    if (LoadString(hDllInstance, 25, MsgText, sizeof(MsgText) / sizeof(TCHAR)))
        SetDlgItemText(hDlg, 1017, MsgText);

//    SendMessage(GetDlgItem(hDlg, 1002), EM_SETLIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);

    SetDlgItemText(hDlg, 1002, pNetIdData->szHostName);
    SetFullComputerName(hDlg, 1001, pNetIdData);

    if (NetGetJoinInformation(NULL, &JoinName, &JoinStatus) != NERR_Success)
    {
        SetRadioBtnState(hDlg, FALSE);

        if (NetWkstaGetInfo(NULL,
                            101,
                            (LPBYTE*)&wki) == NERR_Success)
        {
            SetDlgItemText(hDlg,
                           1007,
                           wki->wki101_langroup);
        }
        else
        {
            DisableControls(hDlg);
        }

        if (wki)
            NetApiBufferFree(wki);
    }
    else
    {
        switch (JoinStatus)
        {
            case NetSetupDomainName:
                SetDlgItemText(hDlg, 116, JoinName);
                SetRadioBtnState(hDlg, TRUE);
                break;

            case NetSetupWorkgroupName:
                SetDlgItemText(hDlg, 1007, JoinName);
                SetRadioBtnState(hDlg, FALSE);
                break;

            case NetSetupUnjoined:
                break;

            case NetSetupUnknownStatus:
            default:
                SetRadioBtnState(hDlg, FALSE);

                if (NetWkstaGetInfo(NULL,
                                    101,
                                    (LPBYTE*)&wki) == NERR_Success)
                {
                    SetDlgItemText(hDlg,
                                   1007,
                                   wki->wki101_langroup);
                }
                else
                {
                    DisableControls(hDlg);
                }

                if (wki)
                    NetApiBufferFree(wki);
                break;
        }

        if (JoinName)
            NetApiBufferFree(JoinName);
    }
}

static
BOOL
NetworkDlg_OnOK(
    HWND hDlg,
    PNETIDDATA pNetIdData)
{
    WCHAR szMsgText[MAX_PATH], szMsgTitle[MAX_PATH];

    if (pNetIdData->bHostNameChanged)
    {
        if (!IsValidComputerName(hDlg, 1002))
        {
            SetFocus(GetDlgItem(hDlg, 1002));
            return FALSE;
        }

        GetWindowText(GetDlgItem(hDlg, 1002), pNetIdData->szHostName, ARRAYSIZE(pNetIdData->szHostName));

        if (!SetComputerNameExW(ComputerNamePhysicalDnsHostname, pNetIdData->szHostName))
        {
            LoadStringW(hDllInstance, 4001, szMsgText, ARRAYSIZE(szMsgText));
            MessageBoxW(hDlg, szMsgText, NULL, MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    if (pNetIdData->bDomainNameChanged)
    {
        if (!SetComputerNameExW(ComputerNamePhysicalDnsDomain, pNetIdData->szDomainName))
        {
            /* FIXME: Show error message */
            return FALSE;
        }
    }

    LoadStringW(hDllInstance, 4000, szMsgTitle, ARRAYSIZE(szMsgTitle));
    LoadStringW(hDllInstance, 24, szMsgText, ARRAYSIZE(szMsgText));
    MessageBoxW(hDlg, szMsgText, szMsgTitle, MB_OK | MB_ICONINFORMATION);

    return TRUE;
}

static
INT_PTR CALLBACK
NetworkPropDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PNETIDDATA pNetIdData;

    pNetIdData = (PNETIDDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (Msg)
    {
        case WM_INITDIALOG:
            pNetIdData = (PNETIDDATA)lParam;
            if (pNetIdData != NULL)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pNetIdData);
                NetworkDlg_OnInitDialog(hDlg, pNetIdData);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 1002:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        UpdateFullComputerName(hDlg, 1001, pNetIdData);
                        pNetIdData->bHostNameChanged = TRUE;
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                    }
                    break;

                case 1008: /* Domain radio button */
                case 1004: /* Workgroup radio button */
                    if (SendDlgItemMessage(hDlg, 1008, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        SetRadioBtnState(hDlg, TRUE);
                    else
                        SetRadioBtnState(hDlg, FALSE);
                    break;

                case 1003:
                    if (DialogBoxParam(hDllInstance,
                                       MAKEINTRESOURCE(IDD_PROPPAGEDNSANDNETBIOS),
                                       hDlg,
                                       DNSSuffixPropDlgProc,
                                       (LPARAM)pNetIdData))
                    {
                        UpdateFullComputerName(hDlg, 1001, pNetIdData);
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                    }
                    break;

                case IDOK:
                    if (NetworkDlg_OnOK(hDlg, pNetIdData))
                        EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
            }
            break;
    }

    return FALSE;
}

static
VOID
NetIDPage_OnInitDialog(
    HWND hwndDlg,
    PNETIDDATA pNetIdData)
{
    WCHAR ComputerDescription[MAX_COMPUTERDESCRIPTION_LENGTH + 1];
    DWORD RegSize = sizeof(ComputerDescription);
    HKEY KeyHandle;
    LPWKSTA_INFO_101 wki;
    LONG lError;

    /* Display computer name and description */
    SendDlgItemMessage(hwndDlg, IDC_COMPDESC, EM_SETLIMITTEXT, MAX_COMPUTERDESCRIPTION_LENGTH, 0);

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters",
                           0,
                           KEY_QUERY_VALUE,
                           &KeyHandle);
    if (lError == ERROR_SUCCESS)
    {
        lError = RegQueryValueExW(KeyHandle,
                                  L"srvcomment",
                                  0,
                                  NULL,
                                  (LPBYTE)ComputerDescription,
                                  &RegSize);
        if (lError == ERROR_SUCCESS)
        {
            ComputerDescription[RegSize / sizeof(WCHAR)] = UNICODE_NULL;
            SetDlgItemText(hwndDlg, IDC_COMPDESC, ComputerDescription);
        }

        RegCloseKey(KeyHandle);
    }

    if (NetWkstaGetInfo(NULL, 101, (LPBYTE*)&wki) == NERR_Success)
    {
        SetDlgItemText(hwndDlg, IDC_WORKGROUPDOMAIN_NAME, wki->wki101_langroup);
        NetApiBufferFree(wki);
    }
}

static
LONG
NetIDPage_OnApply(
    HWND hwndDlg)
{
    WCHAR ComputerDescription[MAX_COMPUTERDESCRIPTION_LENGTH + 1];
    WCHAR NewComputerDescription[MAX_COMPUTERDESCRIPTION_LENGTH + 1];
    HKEY KeyHandle = NULL;
    DWORD dwSize;
    LONG lError;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters",
                           0,
                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                           &KeyHandle);
    if (lError != ERROR_SUCCESS)
        return lError;

    GetDlgItemTextW(hwndDlg,
                    IDC_COMPDESC,
                    NewComputerDescription,
                    ARRAYSIZE(NewComputerDescription));
    if (GetLastError() != ERROR_SUCCESS)
    {
        lError = GetLastError();
        goto done;
    }

    dwSize = sizeof(ComputerDescription);
    lError = RegQueryValueExW(KeyHandle,
                              L"srvcomment",
                              0,
                              NULL,
                              (PBYTE)ComputerDescription,
                              &dwSize);
    if (lError != ERROR_SUCCESS && lError != ERROR_FILE_NOT_FOUND)
        goto done;

    lError = ERROR_SUCCESS;
    if (wcscmp(ComputerDescription, NewComputerDescription) != 0)
    {
        lError = RegSetValueExW(KeyHandle,
                                L"srvcomment",
                                0,
                                REG_SZ,
                                (PBYTE)NewComputerDescription,
                                (wcslen(NewComputerDescription) + 1) * sizeof(WCHAR));
    }

done:
    if (KeyHandle != NULL)
        RegCloseKey(KeyHandle);

    return lError;
}

static INT_PTR CALLBACK
NetIDPageProc(IN HWND hwndDlg,
              IN UINT uMsg,
              IN WPARAM wParam,
              IN LPARAM lParam)
{
    PNETIDDATA pNetIdData;

    pNetIdData = (PNETIDDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pNetIdData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NETIDDATA));
            if (pNetIdData != NULL)
            {
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pNetIdData);
                GetComputerNames(pNetIdData);
                SetFullComputerName(hwndDlg, IDC_COMPUTERNAME, pNetIdData);
                NetIDPage_OnInitDialog(hwndDlg, pNetIdData);
                pNetIdData->bEnable = TRUE;
            }
            return TRUE;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    NetIDPage_OnApply(hwndDlg);
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_COMPDESC:
                    if (HIWORD(wParam) == EN_CHANGE && pNetIdData->bEnable == TRUE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_NETWORK_PROPERTY:
                    if (DialogBoxParam(hDllInstance,
                                       MAKEINTRESOURCE(IDD_PROPPAGECOMPNAMECHENGE),
                                       hwndDlg,
                                       NetworkPropDlgProc,
                                       (LPARAM)pNetIdData))
                    {
                        UpdateFullComputerName(hwndDlg, IDC_COMPUTERNAME, pNetIdData);
                    }
                    break;
            }
            break;

        case WM_DESTROY:
            if (pNetIdData != NULL)
            {
                HeapFree(GetProcessHeap(), 0, pNetIdData);
                pNetIdData = NULL;
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }

    return FALSE;
}

HPROPSHEETPAGE WINAPI
CreateNetIDPropertyPage(VOID)
{
    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hDllInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGECOMPUTER);
    psp.pfnDlgProc = NetIDPageProc;

    return CreatePropertySheetPage(&psp);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
