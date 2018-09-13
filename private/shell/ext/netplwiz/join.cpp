#include "stdafx.h"
#pragma hdrstop

//
// registry information
//

const WCHAR c_szWinLogon[]          = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon";

const WCHAR c_szAutoLogon[]         = L"AutoAdminLogon";
const WCHAR c_szDisableCAD[]        = L"DisableCAD";

const WCHAR c_szDefUserName[]       = L"DefaultUserName";
const WCHAR c_szDefDomain[]         = L"DefaultDomainName";
const WCHAR c_szDefPassword[]       = L"DefaultPassword";

const WCHAR c_szDefaultPwdKey[]     = L"DefaultPassword";

//
// registry helpers
//

BOOL _RegSetSZ(HKEY hk, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    DWORD dwSize = lstrlen(pszValue)*SIZEOF(WCHAR);
    return ERROR_SUCCESS == RegSetValueEx(hk, pszValueName, 0x0, REG_SZ, (BYTE *)pszValue, dwSize);
}

BOOL _RegSetDWORD(HKEY hk, LPCWSTR pszValueName, DWORD dwValue)
{
    DWORD dwSize = SIZEOF(dwValue);
    return ERROR_SUCCESS == RegSetValueEx(hk, pszValueName, 0x0, REG_DWORD, (BYTE *)&dwValue, dwSize);
}

BOOL _RegDelValue(HKEY hk, LPCWSTR pszValueName)
{
    return ERROR_SUCCESS == RegDeleteValue(hk, pszValueName);
}


INT_PTR CALLBACK _CredDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    LPCREDINFO pci = (LPCREDINFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch ( uMsg ) 
    {
        case WM_INITDIALOG:
        {
            pci = (LPCREDINFO)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            SetDlgItemText(hwnd, IDC_USER, pci->pszUser);
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), pci->cchUser - 1);

            SetDlgItemText(hwnd, IDC_DOMAIN, pci->pszDomain);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), pci->cchDomain - 1);

            SetDlgItemText(hwnd, IDC_PASSWORD, pci->pszPassword);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), pci->cchPassword - 1);

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDOK:
                    {
                        FetchText(hwnd, IDC_DOMAIN, pci->pszDomain, pci->cchDomain);
                        FetchText(hwnd, IDC_USER, pci->pszUser, pci->cchUser);

                        if (StrChr(pci->pszUser, TEXT('@')))
                        {
                            *(pci->pszDomain) = 0;
                        }

                        GetDlgItemText(hwnd, IDC_PASSWORD, pci->pszPassword, pci->cchPassword);
                        return EndDialog(hwnd, IDOK);
                    }

                case IDCANCEL:
                    return EndDialog(hwnd, IDCANCEL);

                case IDC_USER:
                {
                    if ( HIWORD(wParam) == EN_CHANGE )
                    {
                        EnableWindow(GetWindow(hwnd, IDOK), FetchTextLength(hwnd, IDC_USER) > 0);

                        EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
                    }
                    break;
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}


//
// attempt to join a domain/workgroup using the specified names and OU.
//

HRESULT _AttemptJoin(HWND hwnd, DWORD dwFlags, LPCWSTR pszDomain, LPCWSTR pszUser, LPCWSTR pszUserDomain, LPCWSTR pszPassword)
{
#ifndef DONT_JOIN
    HRESULT hres = E_FAIL;
    NET_API_STATUS nas;
    TCHAR szDomainUser[MAX_DOMAINUSER + 1];

    TraceEnter(TRACE_PSW, "_AttemptJoin");
    Trace(TEXT("dwFlags %x"), dwFlags);
    Trace(TEXT("pszDomain: %s, pszUser: %s, pszUserDomain: %s"), pszDomain, 
                        pszUser ? pszUser:TEXT("<NULL>"), 
                        pszUserDomain ? pszUserDomain:TEXT("<NULL>"));

    if ( pszUser )
    {
        MakeDomainUserString(pszUserDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));
    }

    Trace(TEXT("NetJoinDomain (1st); pszDomain: %s, dwFlags %x"), pszDomain, dwFlags);
    nas = NetJoinDomain(NULL, pszDomain, NULL, szDomainUser, pszPassword, dwFlags);
    
    if ( (nas == ERROR_ACCESS_DENIED) )
    {
        TraceMsg("NetJoinDomain returned ERROR_ACCESS_DENIED");

        // perhaps an account exists, but we can't delete it so try and remove
        // the account create flag

        if ( dwFlags & NETSETUP_ACCT_CREATE )
        {    
            TraceMsg("Trying NetJoinDomain without NETSETUP_ACCT_CREATE flag");

            dwFlags &= ~NETSETUP_ACCT_CREATE;
            nas = NetJoinDomain(NULL, pszDomain, NULL, szDomainUser, *pszPassword ? pszPassword : NULL, dwFlags);
        }
    }

    if ( (nas != NERR_Success) && (nas != NERR_SetupAlreadyJoined) )
    {
        Trace(TEXT("Failed in NetJoinDomain %x (%d)"), nas, nas);
// BUGBUG: tell the user why
        ExitGracefully(hres, E_FAIL, "Failed to join the new domain/workgroup");
    }

    hres = S_OK;            // success

exit_gracefully:

    if (FAILED(hres))
    {
        TCHAR szMessage[512];

        DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) nas, 0, szMessage, ARRAYSIZE(szMessage), NULL);

        if (0 == dwFormatResult)
        {
            LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
        }

        ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_NAW_JOIN_GENERICERROR, MB_OK|MB_ICONERROR, szMessage);
    }

    TraceLeaveResult(hres);
#else
    TraceEnter(TRACE_PSW, "_AttemptJoin");
    TraceLeaveResult(S_OK);
#endif
}


//
// Handle moving from to a workgroup or domain.  To do this we are passed
// a structure containing all the information we need.
//

HRESULT JoinDomain(HWND hwnd, BOOL fDomain, LPCWSTR pszDomain, CREDINFO* pci)
{
    HRESULT hres = E_FAIL;
    DWORD dwFlags = 0x0;
    LPWSTR pszCurrentDomain = NULL;
    NET_API_STATUS nas;
    BOOL fPassedCredentials = (pci && pci->pszUser && pci->pszUser[0] && pci->pszPassword);
    DECLAREWAITCURSOR;

    SetWaitCursor();

    if (fDomain && !pci)
        ExitGracefully(hres, E_INVALIDARG, "Failed to pass a PCI when fDomain is TRUE");

    TraceEnter(TRACE_PSW, "JoinDomain");
    Trace(TEXT("fDomain %d, pszDomain: %s, pszUser: %s, pszUserDomain: %s"),
                    fDomain, pszDomain, 
                    (pci && pci->pszUser) ? pci->pszUser:TEXT("<NULL>"),
                    (pci && pci->pszDomain) ? pci->pszDomain:TEXT("<NULL>"));

    //
    // lets validate the domain name before we go and use it, therefore avoiding
    // orphaning the computer too badly
    //

    nas = NetValidateName(NULL, pszDomain, NULL, NULL, fDomain ? NetSetupDomain:NetSetupWorkgroup);
    if ( NERR_Success != nas )
    {
        Trace(TEXT("NetValidateName returned %x (%d)"), nas, nas);

        ShellMessageBox(GLOBAL_HINSTANCE, hwnd,
                        fDomain ? MAKEINTRESOURCE(IDS_ERR_BADDOMAIN) : MAKEINTRESOURCE(IDS_ERR_BADWORKGROUP), 
                        MAKEINTRESOURCE(IDS_NETWIZCAPTION),
                        MB_OK|MB_ICONWARNING,
                        pszDomain);

        ExitGracefully(hres, E_FAIL, "Bad domain specified");
    }

    // 
    // now attempt to join the domain, prompt for credentails if the ones
    // specified are not good enough
    //

    TraceMsg("Doing the domain change");

    if ( fDomain )
    {
        TraceMsg("Attempting domain join so setting: NETSETUP_JOIN_DOMAIN|NETSETUP_ACCT_CREATE|NETSETUP_DOMAIN_JOIN_IF_JOINED");
        dwFlags |= NETSETUP_JOIN_DOMAIN|NETSETUP_ACCT_CREATE|NETSETUP_DOMAIN_JOIN_IF_JOINED;
    }
    else
    {
        TraceMsg("Attempting to join a WORKGROUP, so leaving where we are now");

        nas = NetUnjoinDomain(NULL, NULL, NULL, NETSETUP_ACCT_DELETE);
        if ( (nas != NERR_Success) && (nas != NERR_SetupNotJoined) )
        {
            Trace(TEXT("NetUnjoinDomain returned %x (%d) - trying again with no delete"), nas, nas);
            nas = NetUnjoinDomain(NULL, NULL, NULL, 0x0);
        }

        if ( (nas != NERR_Success) && (nas != NERR_SetupNotJoined) )
        {
            Trace(TEXT("Failed in NetUnjoinDomain %x (%d)"), nas, nas);
            ExitGracefully(hres, E_UNEXPECTED, "Computer object orphaned");
        }

        g_fRebootOnExit = TRUE;               // we changed the domain
    }

    if ( !fDomain || fPassedCredentials)
    {
        TraceMsg("Not a domain join, or we have user information so using to call _AttemptJoin");

        if (fPassedCredentials)
        {
            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, pci->pszUser, pci->pszDomain, pci->pszPassword);
        }
        else
        {
            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, NULL, NULL, NULL);
        }
    }

    if ( fDomain && ((FAILED(hres) || (!fPassedCredentials))) )
    {
        TraceMsg("Either a domain join (and failed first time, or we didn't have credentials");

        do
        {
            TraceMsg("Prompting to credentials");

            if ( IDCANCEL == DialogBoxParam(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDD_PSW_JOINCREDENTIALS), 
                                                hwnd, _CredDlgProc, (LPARAM)pci) )
            {
                ExitGracefully(hres, E_FAIL, "User pressed 'Cancel' in user credentails");
            }

            // The dialog box changed the cursor from a wait cursor to an arrow cursor, so the cursor
            // needs to be changed back.. This call could be moved to _AttemptJoin (along with a call to
            // reset the cursor).  This call is made synchronously from the message loop for this hwnd
            SetCursor(LoadCursor(NULL, IDC_WAIT));

            TraceMsg("Trying to join again");

            hres = _AttemptJoin(hwnd, dwFlags, pszDomain, pci->pszUser, pci->pszDomain, pci->pszPassword);

        }
        while ( FAILED(hres) );
    }

exit_gracefully:

    ResetWaitCursor();

    if ( SUCCEEDED(hres) )
    {
       ClearAutoLogon();
        g_fRebootOnExit = TRUE;               // we changed the domain
    }

    NetApiBufferFree(pszCurrentDomain);

    TraceLeaveResult(hres);                                                                                                                                                                 
}


//
// set and clear the auto admin logon state.
//
// we set the default user and default domain to the specified strings, we then blow away
// the clear text password stored in the registry to replace it with a password stored
// in the LSA secret space.
//

NTSTATUS _SetDefaultPassword(LPCWSTR PasswordBuffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0L, (HANDLE)NULL, NULL);

    Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_CREATE_SECRET, &LsaHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&SecretName, c_szDefaultPwdKey);
    RtlInitUnicodeString(&SecretValue, PasswordBuffer);

    Status = LsaStorePrivateData(LsaHandle, &SecretName, &SecretValue);
    LsaClose(LsaHandle);

    return Status;
}


//
// Set and clear auto logon for a particular
//

VOID SetAutoLogon(LPCWSTR pszUserName, LPCWSTR pszPassword)
{
#ifndef DONT_JOIN
    WCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerName = ARRAYSIZE(szComputerName);
    HKEY hk;

    TraceEnter(TRACE_PSW, "SetAutoLogon");
    Trace(TEXT("pszUserName: %s"), pszUserName);

    GetComputerName(szComputerName, &dwComputerName);
    SetDefAccount(pszUserName, szComputerName);         // also clears auto logon

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szAutoLogon, L"1");             // auto admin logon
        _RegDelValue(hk, c_szDefPassword);              // use the LSA secret for the password
        RegCloseKey (hk);
    }

    _SetDefaultPassword(pszPassword);    

    TraceLeave();
#endif
}


//
// clear the auto admin logon
//

STDAPI ClearAutoLogon(VOID)
{
#ifndef DONT_JOIN
    TraceEnter(TRACE_PSW, "ClearAutoLogon");

    HKEY hk;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szAutoLogon, L"0");         // no auto admin logon
        _RegDelValue(hk, c_szDefPassword);  

        RegCloseKey(hk);
    }

    _SetDefaultPassword(L"");            // clear the LSA secret

    TraceLeave();
#endif
    return(S_OK);
}


//
// set the default account
//

VOID SetDefAccount(LPCWSTR pszUser, LPCWSTR pszDomain)
{
#ifndef DONT_JOIN
    HKEY hk;

    TraceEnter(TRACE_PSW, "SetDefAccount");

   ClearAutoLogon();

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinLogon, 0x0, KEY_WRITE, &hk) )
    {
        _RegSetSZ(hk, c_szDefUserName, pszUser);             
        _RegSetSZ(hk, c_szDefDomain, pszDomain);
       RegCloseKey(hk);
    }

    TraceLeave();
#endif
}
