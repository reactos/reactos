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
#include <lm.h>
#include <prsht.h>

#include "resource.h"


#define MAX_COMPUTERDESCRIPTION_LENGTH 255
#define MAX_HOSTNAME_LENGTH             63
#define MAX_DOMAINNAME_LENGTH          255

typedef struct _NETIDDATA
{
    WCHAR szHostName[MAX_HOSTNAME_LENGTH + 1];
    WCHAR szOldHostName[MAX_HOSTNAME_LENGTH + 1];
    WCHAR szDomainName[MAX_DOMAINNAME_LENGTH + 1];
    WCHAR szOldDomainName[MAX_DOMAINNAME_LENGTH + 1];
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL bSyncDomainWithMembership;
    LPWSTR JoinName;
    NETSETUP_JOIN_STATUS JoinStatus;
    BOOL bHostNameChanged;
    BOOL bDomainNameChanged;
    BOOL bSyncDomainWithMembershipChanged;
    BOOL bEnable;
} NETIDDATA, *PNETIDDATA;


static HINSTANCE hDllInstance;

static
INT
FormatMessageBox(
    HWND hDlg,
    UINT uType,
    DWORD dwMessage,
    ...)
{
    WCHAR szTitle[256], szMessage[256], szText[512];
    va_list args = NULL;

    LoadStringW(hDllInstance, 4, szTitle, ARRAYSIZE(szTitle));

    LoadStringW(hDllInstance, dwMessage, szMessage, ARRAYSIZE(szMessage));

    va_start(args, dwMessage);
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING,
                   szMessage,
                   0,
                   0,
                   szText,
                   ARRAYSIZE(szText),
                   &args);
    va_end(args);

    return MessageBoxW(hDlg, szText, szTitle, uType);
}

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

        dwSize = sizeof(pNetIdData->szOldHostName);
        RegQueryValueExW(KeyHandle,
                         L"HostName",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szOldHostName,
                         &dwSize);

        dwSize = sizeof(pNetIdData->szDomainName);
        RegQueryValueExW(KeyHandle,
                         L"NV Domain",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szDomainName,
                         &dwSize);

        dwSize = sizeof(pNetIdData->szOldDomainName);
        RegQueryValueExW(KeyHandle,
                         L"Domain",
                         0,
                         NULL,
                         (LPBYTE)&pNetIdData->szOldDomainName,
                         &dwSize);

        dwSize = sizeof(pNetIdData->bSyncDomainWithMembership);
        if (RegQueryValueExW(KeyHandle,
                             L"SyncDomainWithMembership",
                             0,
                             NULL,
                             (LPBYTE)&pNetIdData->bSyncDomainWithMembership,
                             &dwSize))
            pNetIdData->bSyncDomainWithMembership = TRUE;

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

    if (NetGetJoinInformation(NULL, &pNetIdData->JoinName, &pNetIdData->JoinStatus) != NERR_Success)
    {
        pNetIdData->JoinName = NULL;
        pNetIdData->JoinStatus = NetSetupUnknownStatus;
    }

    return TRUE;
}

static BOOL
IsUserAdmin(VOID)
{
    BOOL bIsAdmin;
    SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
    PSID pAdminsSid;

    if (!AllocateAndInitializeSid(&Authority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &pAdminsSid))
    {
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, pAdminsSid, &bIsAdmin))
        bIsAdmin = FALSE;
    FreeSid(pAdminsSid);

    return bIsAdmin;
}

static
BOOL
IsValidDomainName(
    HWND hDlg,
    UINT uId)
{
    WCHAR szDomainName[256];
    DWORD dwError;

    if (GetDlgItemTextW(hDlg, uId, szDomainName, ARRAYSIZE(szDomainName)) == 0)
        return TRUE;

    dwError = DnsValidateName_W(szDomainName, DnsNameDomain);
    if (dwError != ERROR_SUCCESS)
    {
        switch (dwError)
        {
            case DNS_ERROR_NON_RFC_NAME:
                if (FormatMessageBox(hDlg, MB_YESNO | MB_ICONWARNING, 7, szDomainName) == IDYES)
                    return TRUE;
                break;

            case ERROR_INVALID_NAME:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 8, szDomainName);
                break;

            case DNS_ERROR_NUMERIC_NAME:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 1031, szDomainName);
                break;

            case DNS_ERROR_INVALID_NAME_CHAR:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 1032, szDomainName);
                break;
        }

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
                SendDlgItemMessage(hDlg, 115, BM_SETCHECK, (WPARAM)pNetIdData->bSyncDomainWithMembership, 0);
                SetDlgItemTextW(hDlg, 1013, pNetIdData->szComputerName);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 115:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                        pNetIdData->bSyncDomainWithMembershipChanged = TRUE;
                    }
                    break;

                case 1011:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                        pNetIdData->bDomainNameChanged = TRUE;
                    }
                    break;

                case IDOK:
                    if (!IsValidDomainName(hDlg, 1011))
                    {
                        SetFocus(GetDlgItem(hDlg, 1011));
                        break;
                    }

                    if (pNetIdData->bDomainNameChanged)
                        GetDlgItemTextW(hDlg, 1011, pNetIdData->szDomainName, ARRAYSIZE(pNetIdData->szDomainName));

                    if (pNetIdData->bSyncDomainWithMembershipChanged)
                    {
                        if (SendDlgItemMessage(hDlg, 115, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            pNetIdData->bSyncDomainWithMembership = TRUE;
                        else
                            pNetIdData->bSyncDomainWithMembership = FALSE;
                    }

                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    pNetIdData->bDomainNameChanged = FALSE;
                    pNetIdData->bSyncDomainWithMembershipChanged = FALSE;
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

#if 0
static VOID
DisableControls(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, 1008), FALSE);
    EnableWindow(GetDlgItem(hDlg, 1004), FALSE);
    EnableWindow(GetDlgItem(hDlg, 116), FALSE);
    EnableWindow(GetDlgItem(hDlg, 1007), FALSE);
}
#endif

static
BOOL
IsValidComputerName(
    HWND hDlg,
    UINT uId)
{
    WCHAR szHostName[256];
    DWORD dwError;

    GetWindowText(GetDlgItem(hDlg, uId), szHostName, ARRAYSIZE(szHostName));

    dwError = DnsValidateName_W(szHostName, DnsNameHostnameLabel);
    if (dwError != ERROR_SUCCESS)
    {
        switch (dwError)
        {
            case DNS_ERROR_NON_RFC_NAME:
                if (FormatMessageBox(hDlg, MB_YESNO | MB_ICONWARNING, 10, szHostName) == IDYES)
                    return TRUE;
                break;

            case ERROR_INVALID_NAME:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 11);
                return FALSE;

            case DNS_ERROR_NUMERIC_NAME:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 1029, szHostName);
                break;

            case DNS_ERROR_INVALID_NAME_CHAR:
                FormatMessageBox(hDlg, MB_OK | MB_ICONERROR, 1030, szHostName);
                break;
        }

        return FALSE;
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

    wsprintf(szFullComputerName, L"%s.%s", pNetIdData->szHostName, pNetIdData->szDomainName);
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

    wsprintf(szFullComputerName, L"%s.%s", szHostName, pNetIdData->szDomainName);
    SetDlgItemText(hDlg, uId, szFullComputerName);
}

static
VOID
UpdateNetbiosName(
    HWND hDlg,
    UINT uId,
    PNETIDDATA pNetIdData)
{
    WCHAR szHostName[256];
    DWORD dwSize;

    GetWindowText(GetDlgItem(hDlg, 1002), szHostName, ARRAYSIZE(szHostName));

    dwSize = ARRAYSIZE(pNetIdData->szComputerName);
    DnsHostnameToComputerNameW(szHostName,
                               pNetIdData->szComputerName,
                               &dwSize);
}

static
VOID
CheckForChangedNames(
    _In_ HWND hwndDlg,
    _In_ PNETIDDATA pNetIdData)
{
    INT nShow = SW_HIDE;

    if ((_wcsicmp(pNetIdData->szOldHostName, pNetIdData->szHostName) != 0) ||
        (_wcsicmp(pNetIdData->szOldDomainName, pNetIdData->szDomainName) != 0))
    {
        nShow = SW_SHOW;
    }

    ShowWindow(GetDlgItem(hwndDlg, IDC_RESTARTICON), nShow);
    ShowWindow(GetDlgItem(hwndDlg, IDC_RESTARTTEXT), nShow);
}

static
VOID
NetworkDlg_OnInitDialog(
    HWND hDlg,
    PNETIDDATA pNetIdData)
{
    WCHAR MsgText[MAX_PATH * 2];

    if (LoadStringW(hDllInstance, 25, MsgText, ARRAYSIZE(MsgText)))
        SetDlgItemText(hDlg, 1017, MsgText);

    SendMessage(GetDlgItem(hDlg, 1002), EM_SETLIMITTEXT, MAX_HOSTNAME_LENGTH, 0);
    SetDlgItemText(hDlg, 1002, pNetIdData->szHostName);
    SetFullComputerName(hDlg, 1001, pNetIdData);

    /* Display the workgroup or domain name */
    switch (pNetIdData->JoinStatus)
    {
        case NetSetupDomainName:
            SetDlgItemText(hDlg, 116, pNetIdData->JoinName);
            SetRadioBtnState(hDlg, TRUE);
            break;

        case NetSetupWorkgroupName:
            SetDlgItemText(hDlg, 1007, pNetIdData->JoinName);
            SetRadioBtnState(hDlg, FALSE);
            break;

        default:
            break;
    }
}

static
BOOL
NetworkDlg_OnOK(
    HWND hDlg,
    PNETIDDATA pNetIdData)
{
    WCHAR szMsgText[MAX_PATH], szMsgTitle[MAX_PATH];
    HKEY KeyHandle;
    DWORD dwError;

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

        pNetIdData->bHostNameChanged = FALSE;
    }

    if (pNetIdData->bDomainNameChanged)
    {
        if (!SetComputerNameExW(ComputerNamePhysicalDnsDomain, pNetIdData->szDomainName))
        {
            /* FIXME: Show error message */
            return FALSE;
        }

        pNetIdData->bDomainNameChanged = FALSE;
    }

    if (pNetIdData->bSyncDomainWithMembershipChanged)
    {
        dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SYSTEM\\CurrentControlSet\\Services\\TcpIp\\Parameters",
                                0,
                                KEY_SET_VALUE,
                                &KeyHandle);
        if (dwError == ERROR_SUCCESS)
        {
            RegSetValueExW(KeyHandle,
                           L"SyncDomainWithMembership",
                           0,
                           REG_DWORD,
                           (LPBYTE)&pNetIdData->bSyncDomainWithMembership,
                           sizeof(pNetIdData->bSyncDomainWithMembership));
            RegCloseKey(KeyHandle);
        }

        pNetIdData->bSyncDomainWithMembershipChanged = FALSE;
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
                        UpdateNetbiosName(hDlg, 1001, pNetIdData);
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
    WCHAR szBuffer[512];
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

    /* Set the workgroup or domain name */
    LoadStringW(hDllInstance, (pNetIdData->JoinStatus == NetSetupDomainName)? 6 : 5, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(hwndDlg, IDC_WORKGROUPDOMAIN, szBuffer);
    SetDlgItemText(hwndDlg, IDC_WORKGROUPDOMAIN_NAME, pNetIdData->JoinName);

    /* Show the administrator note and disable controls when the user is not an administator */
    if (!IsUserAdmin())
    {
        LoadStringW(hDllInstance, 1021, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hwndDlg, IDC_MESSAGETEXT, szBuffer);

        EnableWindow(GetDlgItem(hwndDlg, IDC_NETWORK_ID), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_NETWORK_PROPERTY), FALSE);
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
                CheckForChangedNames(hwndDlg, pNetIdData);
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
                        CheckForChangedNames(hwndDlg, pNetIdData);
                    }
                    break;
            }
            break;

        case WM_DESTROY:
            if (pNetIdData != NULL)
            {
                if (pNetIdData->JoinName != NULL)
                    NetApiBufferFree(pNetIdData->JoinName);
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
