#include "stdafx.h"

// Group page base and group info list headers
#include "grpinfo.h"
#include "grppage.h"

#pragma hdrstop


//
// global information used for domains, groups etc.
//

WCHAR g_szUser[MAX_DOMAINUSER + 1] = { L'\0' };
WCHAR g_szDomain[MAX_DOMAIN + 1] = { L'\0' };
WCHAR g_szCompDomain[MAX_DOMAIN + 1] = { L'\0' };

// If this flag is set to TRUE, when the wizard exits the all connections are closed
BOOL g_fWizardCreatedRASConnection = FALSE;

CGroupPageBase* g_pGroupPageBase;

//
// return a BSTR containing the user/domain name to present to a net
// call.
//

/*-----------------------------------------------------------------------------
/ Intro page to the wizard.
/----------------------------------------------------------------------------*/

INT_PTR CALLBACK _DomainInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE: 
                    SetWizardButtons(hwnd, PSWIZB_NEXT|PSWIZB_BACK);
                    return TRUE;              

                case PSN_WIZBACK:
                {
                    if ( g_uWizardIs != NAW_NETID )
                        WizardNext(hwnd, IDD_PSW_WELCOME);

                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}                                    


/*-----------------------------------------------------------------------------
/ Take the user information and attempt to join a domain for the given
/ computer fvect.
/----------------------------------------------------------------------------*/

//
// Search columns are returns as a ADS_SEARCH_COLUMN which is like a variant,
// but, the data form is more specific to a DS.
//
// We only need strings, therefore barf if any other type is given to us.
//

HRESULT _GetStringFromColumn(ADS_SEARCH_COLUMN *pasc, LPWSTR pBuffer, INT cchBuffer)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_PSW, "_GetStringFromColumn");

    switch ( pasc->dwADsType )
    {
        case ADSTYPE_DN_STRING:
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
            StrCpyN(pBuffer, pasc->pADsValues[0].DNString, cchBuffer);
            break;

        default:
            ExitGracefully(hres, E_FAIL, "Bad search column type");
    }

exit_gracefully:

    TraceLeaveResult(hres);
}


//
// Search the DS for a computer object that matches this computer name, if
// we find one then try and crack the name to give us something that
// can be used to join a domain.
//

HRESULT _FindComputerInDomain(LPWSTR pszUserName, LPWSTR pszUserDomain, LPWSTR pszSearchDomain, LPWSTR pszPassword, BSTR *pbstrCompDomain)
{
    HRESULT hres;
    WCHAR szBuffer[MAX_PATH + 1];
    LPWSTR c_aszAttributes[] = { L"ADsPath", };
    WCHAR szADsPath[MAX_PATH + 1];
    WCHAR szComputerName[MAX_COMPUTERNAME + 1];
    DWORD dwComputerName = ARRAYSIZE(szComputerName);
    IDirectorySearch* pds = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[1];
    ADS_SEARCH_COLUMN ascADsPath;
    IADsPathname* padp = NULL;
    BSTR bstrX500DN = NULL;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    PDS_NAME_RESULT pdnr = NULL;
    DWORD dwres;
    TCHAR szDomainUser[MAX_DOMAINUSER + 1];
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_PSW, "_FindComputerInDomain");
    Trace(TEXT("pszUserName: %s, pszUserDomain: %s, pszSearchDomain: %s"), pszUserName, pszUserDomain, pszSearchDomain);

    SetWaitCursor();
    CoInitialize(NULL);

    //
    // Lets try and deterrmine the domain to search by taking the users domain and
    // calling DsGetDcName with it.
    //
    
    dwres = DsGetDcName(NULL, pszSearchDomain, NULL, NULL, DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED, &pdci);
    if ( (NO_ERROR != dwres) || !pdci->DnsForestName )
    {
        Trace(TEXT("DsGetDcName returned: %x (%d)"), dwres, dwres);
        ExitGracefully(hres, E_FAIL, "DsGetDcName failed/no forest name");
    }
        
    wsprintf(szBuffer, L"GC://%s", pdci->DnsForestName);
    Trace(TEXT("GC path is: %s"), szBuffer);

    //
    // now open the GC with the domain user
    //

    MakeDomainUserString(pszUserDomain, pszUserName, szDomainUser, ARRAYSIZE(szDomainUser));

    hres = ADsOpenObject(szBuffer, 
                         szDomainUser, pszPassword, ADS_SECURE_AUTHENTICATION, 
                         IID_IDirectorySearch, (void **)&pds);

    FailGracefully(hres, "Failed to get IDirectorySearch for GC path");

    //
    // we have a GC object, so lets search it...
    //

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    hres = pds->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
    FailGracefully(hres, "Failed to set search preferences");

    GetComputerName(szComputerName, &dwComputerName);
    wsprintf(szBuffer, L"(&(sAMAccountType=805306369)(sAMAccountName=%s$))", szComputerName);

    hres = pds->ExecuteSearch(szBuffer, c_aszAttributes, ARRAYSIZE(c_aszAttributes), &hSearch);
    FailGracefully(hres, "Failed in ExecuteSearch");


    // 
    // we executed the search, so we can now attempt to read the results back
    //

    hres = pds->GetNextRow(hSearch);
    FailGracefully(hres, "Failed when requesting results");

    if ( (hres == S_ADS_NOMORE_ROWS) )
        ExitGracefully(hres, E_FAIL, "Didn't find an object that matches the criteria specified");

    hres = pds->GetColumn(hSearch, L"ADsPath", &ascADsPath);
    FailGracefully(hres, "Failed to get the ADsPath column");

    _GetStringFromColumn(&ascADsPath, szADsPath, ARRAYSIZE(szADsPath));
    

    //
    // So we found an object that is of the category computer, and it has the same name
    // as the computer object we are looking for.  Lets try and crack the name now
    // and determine which domain it is in.
    //
    
    hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&padp);
    FailGracefully(hres, "Failed to get the IADsPathname interface");

    hres = padp->Set(szADsPath, ADS_SETTYPE_FULL);
    FailGracefully(hres, "Failed to set the pathname into the path cracker");

    hres = padp->Retrieve(ADS_FORMAT_X500_DN, &bstrX500DN);
    FailGracefully(hres, "Failed to get the X500 DN from the path name API");

    //
    // crack the name, we want the domain and the OU. 
    //

    Trace(TEXT("Calling DsCrackNames on: %s"), bstrX500DN);

    dwres = DsCrackNames(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                         DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                         1, &bstrX500DN, 
                         &pdnr);

    if ( (NO_ERROR != dwres) || (pdnr->cItems != 1))
    {
        Trace(TEXT("DsCrackNames returned: %x (%d)"), dwres, dwres);
        ExitGracefully(hres, E_FAIL, "Failed when calling DsCrackNames to get canonical name");
    }

    //
    // try and get the NETBIOS name for the domain
    //

    Trace(TEXT("Domain name returned from DsCrackNames: %s"), pdnr->rItems->pDomain);

#if 1
    NetApiBufferFree(pdci);

    TraceMsg("Attempting to convert to FLAT domain name");

    dwres = DsGetDcName(NULL, pdnr->rItems->pDomain, NULL, NULL, DS_IS_DNS_NAME|DS_RETURN_FLAT_NAME, &pdci);
    if ( NO_ERROR != dwres )
    {
        Trace(TEXT("DsGetDcName returned: %x (%d)"), dwres, dwres);
        ExitGracefully(hres, E_FAIL, "Failed to get the flat name for the domain");
    }

    Trace(TEXT("Resulting name is: %s"), pdci->DomainName);

    if ( pbstrCompDomain )
        *pbstrCompDomain = SysAllocString(pdci->DomainName);
#else
    if ( pbstrCompDomain )
        *pbstrCompDomain = SysAllocString(pdnr->rItems->pDomain);
#endif
    
    if ( (pbstrCompDomain && !*pbstrCompDomain) )
        ExitGracefully(hres, E_FAIL, "Failed to allocate copies of domain");

    hres = S_OK;

exit_gracefully:

    NetApiBufferFree(pdci);
    DsFreeNameResult(pdnr);
    
    SysFreeString(bstrX500DN);
   
    if ( pds && hSearch )
        pds->CloseSearchHandle(hSearch);

    if ( pds )
        pds->Release();

    if ( padp )
        padp->Release();    

    CoUninitialize();
    ResetWaitCursor();

    TraceLeaveResult(hres);
}


//
// This is the phonebook callback, it is used to notify the book of the user name, domain
// and password to be used in this connection.  It is also used to receive changes made by
// the user.
//

VOID WINAPI _PhoneBkCB(ULONG_PTR dwCallBkID, DWORD dwEvent, LPWSTR pszEntry, void *pEventArgs)
{
    RASNOUSER *pInfo = (RASNOUSER *)pEventArgs;
    CREDINFO *pci = (CREDINFO *)dwCallBkID;

    TraceEnter(TRACE_PSW, "_PhoneBkCB");

    switch ( dwEvent )
    {
        case RASPBDEVENT_NoUser:
        {
            // 
            // we are about to initialize the phonebook dialog, therefore
            // lets pass through our credential information.
            //

            TraceMsg("Returning 'phone book infor from CB");

            pInfo->dwSize = SIZEOF(RASNOUSER);
            pInfo->dwFlags = 0;
            pInfo->dwTimeoutMs = 0;
            StrCpy(pInfo->szUserName, pci->pszUser);
            StrCpy(pInfo->szDomain, pci->pszDomain);
            StrCpy(pInfo->szPassword, pci->pszPassword);

            break;     
        }

        case RASPBDEVENT_NoUserEdit:
        {
            //
            // the user has changed the credetials we supplied for the
            // login, therefore we must update them in our copy accordingly.
            //

            TraceMsg("User modified the information we passed");

            if ( pInfo->szUserName[0] )
                StrCpyN(pci->pszUser, pInfo->szUserName, pci->cchUser);

            if ( pInfo->szPassword[0] )
                StrCpyN(pci->pszPassword, pInfo->szPassword, pci->cchPassword);

            if ( pInfo->szDomain[0] )
                StrCpyN(pci->pszDomain, pInfo->szDomain, pci->cchDomain);

            break;
        }
    }

    TraceLeave();
}


BOOL SetAllowKey(DWORD dwNewValue, DWORD* pdwOldValue)
{
    HKEY hkey = NULL;
    BOOL fValueWasSet = FALSE;

    if (ERROR_SUCCESS == RegCreateKeyEx(
        HKEY_USERS, 
        TEXT(".DEFAULT\\Software\\Microsoft\\RAS Logon Phonebook"),
        NULL,
        TEXT(""),
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hkey,
        NULL))
    {
        DWORD dwType = 0;
        // Read the previous value if required
        if (NULL != pdwOldValue)
        {
            DWORD cbSize = sizeof (DWORD);
            if (ERROR_SUCCESS != RegQueryValueEx(
                hkey, 
                TEXT("AllowLogonPhonebookEdits"),
                NULL,
                &dwType,
                (LPBYTE) pdwOldValue,
                &cbSize))
            {
                // Assume FALSE if the value doesn't exist
                *pdwOldValue = 0;
            }
        }

        // Set the new value
        if (ERROR_SUCCESS == RegSetValueEx(
            hkey,
            TEXT("AllowLogonPhonebookEdits"),
            NULL,
            REG_DWORD,
            (CONST BYTE*) &dwNewValue,
            sizeof (DWORD)))
        {
            // Function was successful; value was set
            fValueWasSet = TRUE;
        }

        RegCloseKey(hkey);
    }

    return fValueWasSet;
}

//
// The user is trying to advance from the user info tab in the Wizard.  Therefore
// we must take the information they have entered and:
//
//  - log in using RAS (if ras is selected)
//  - try and locate a computer object
//  - if we find a computer object then allow them to use it
//  
// If we failed to find a computer object, or the user found one and decided not
// to use then we advance them to the 'computer info' page in the wizard.  If
// they decide to use it then we must apply it and advance to permissions.
//

void _DoUserInfoNext(HWND hwnd)
{
    HRESULT hres;
    WCHAR szPassword[MAX_PASSWORD + 1];
    BSTR bstrCompDomain = NULL;
    LONG idNextPage = -1;
    TCHAR szSearchDomain[MAX_DOMAIN + 1]; *szSearchDomain = 0;
    BOOL fTranslateNameTriedAndFailed = FALSE;

    // fSetAllowKey - Have we set the regval that says "allow connectiod creation before logon?"
    BOOL fSetAllowKey = FALSE;
    DWORD dwPreviousAllowValue = 0;


    TraceEnter(TRACE_PSW, "_DoUserInfoNext");

    //
    // read the user, domain and password from the dialog.  then 
    // lets search for the computer object that matches the currently
    // configure computer name.
    //

    FetchText(hwnd, IDC_USER, g_szUser, ARRAYSIZE(g_szUser));
    FetchText(hwnd, IDC_DOMAIN, g_szDomain, ARRAYSIZE(g_szDomain));
    FetchText(hwnd, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));

    Trace(TEXT("User name: %s, Domain: %s"), g_szUser, g_szDomain);

    // Handle possible UPN case
    if (StrChr(g_szUser, TEXT('@')))
    {
        *g_szDomain = 0;
    }

    //
    // before we search for the computer object lets check to see if we should be using RAS 
    // to get ourselves onto the network.
    //

    if ( IsDlgButtonChecked(hwnd, IDC_DIALUP) == BST_CHECKED )
    {    
        fSetAllowKey = SetAllowKey(1, &dwPreviousAllowValue);

        TraceMsg("RAS enabled");

        RASPBDLG info = { 0 };

        // Its ok to use globals here - we want to overwrite them.
        CREDINFO ci = { g_szUser, ARRAYSIZE(g_szUser), 
                        g_szDomain, ARRAYSIZE(g_szDomain),
                        szPassword, ARRAYSIZE(szPassword) };

        info.dwSize = SIZEOF(info);
        info.hwndOwner = hwnd;
        info.dwFlags = RASPBDFLAG_NoUser;
        info.pCallback = _PhoneBkCB;
        info.dwCallbackId = (ULONG_PTR)&ci;

        if ( !RasPhonebookDlg(NULL, NULL, &info) )
            ExitGracefully(hres, E_FAIL, "Failed to connect using RAS");

        // Signal that the wizard has created a RAS connection.
        
        // Just to be extra paranoid, only do this if the wizard isn't a NETID wizard
        if (g_uWizardIs != NAW_NETID)
        {
            g_fWizardCreatedRASConnection = TRUE;
        }

        SetDlgItemText(hwnd, IDC_USER, g_szUser);
        SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);
    }


    //
    // now attempt to look up the computer object in the user domain.
    //

    if (StrChr(g_szUser, TEXT('@')))
    {
        TCHAR szDomainUser[MAX_DOMAINUSER + 1];
        ULONG ch = ARRAYSIZE(szDomainUser);
    
        if (TranslateName(g_szUser, NameUserPrincipal, NameSamCompatible, szDomainUser, &ch))
        {
            TCHAR szUser[MAX_USER + 1];
            DomainUserString_GetParts(szDomainUser, szUser, ARRAYSIZE(szUser), szSearchDomain, ARRAYSIZE(szSearchDomain));
        }
        else
        {
            fTranslateNameTriedAndFailed = TRUE;
        }
    }

    if (0 == *szSearchDomain)
    {
        lstrcpyn(szSearchDomain, g_szDomain, ARRAYSIZE(szSearchDomain));
    }

    hres = _FindComputerInDomain(g_szUser, g_szDomain, szSearchDomain, szPassword, &bstrCompDomain);
    switch ( hres )
    {
        case S_OK:
        {
            TraceMsg("Returned S_OK, so attempting to join");

            StrCpy(g_szCompDomain, bstrCompDomain);     // they want to change the domain

            //
            // we found an object in the DS that matches the current computer name
            // and domain.  show the domain to the user before we join, allowing them
            // to confirm that this is what they want to do.
            //

            if ( IDYES == ShellMessageBox(GLOBAL_HINSTANCE, hwnd,
                                          MAKEINTRESOURCE(IDS_ABOUTTOJOIN), MAKEINTRESOURCE(IDS_USERINFO),
                                          MB_YESNO|MB_ICONQUESTION, 
                                          bstrCompDomain) )
            {
                TraceMsg("They answered Yes to the prompt for RU sure");

                // 
                // they don't want to modify the parameters so lets do the join.
                //

                idNextPage = IDD_PSW_ADDUSER;
                            
                // Make local copies of the user/domain buffers since we don't want to modify globals
                TCHAR szUser[MAX_DOMAINUSER + 1]; lstrcpyn(szUser, g_szUser, ARRAYSIZE(szUser));
                TCHAR szDomain[MAX_DOMAIN + 1]; lstrcpyn(szDomain, g_szDomain, ARRAYSIZE(szDomain));
                
                CREDINFO ci = {szUser, ARRAYSIZE(szUser), szDomain, ARRAYSIZE(szDomain), szPassword, ARRAYSIZE(szPassword)};
                if ( FAILED(JoinDomain(hwnd, TRUE, bstrCompDomain, &ci)) )
                {
                    TraceMsg("The domain joined failed, so not advancing to another page");
                    idNextPage = -1;            // don't advance they failed to join
                }                
            }
            else
            {
                idNextPage = IDD_PSW_COMPINFO;
            }

            break;
        }
        
        case HRESULT_FROM_WIN32(ERROR_INVALID_DOMAINNAME):
        {
            // 
            // the domain was invalid, so we should really tell them
            //

            TraceMsg("Invalid domain was returned");

            ShellMessageBox(GLOBAL_HINSTANCE, hwnd,
                            MAKEINTRESOURCE(IDS_ERR_BADDOMAIN), MAKEINTRESOURCE(IDS_USERINFO),
                            MB_OK|MB_ICONWARNING, g_szDomain);
            break;            

        }

        case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):
        case HRESULT_FROM_WIN32(ERROR_BAD_USERNAME):
        {
            //
            // this was a credentail failure, so lets tell the user they got something
            // wrong, and let them correct it.
            //

            if (!fTranslateNameTriedAndFailed)
            {

                TraceMsg("Invalid user name/user account/bad password message");
            
                ShellMessageBox(GLOBAL_HINSTANCE, hwnd,
                                MAKEINTRESOURCE(IDS_ERR_BADPWUSER), MAKEINTRESOURCE(IDS_USERINFO),
                                MB_OK|MB_ICONWARNING);
                break;            
            }
            else
            {
                // Fall through... We tried to translate a UPN but we failed, so
                // we want to act as if we just failed to find a computer account
                goto default_label;
            }
        }


        default:
        {
default_label:
            // 
            // failed to find a computer that matches the information we have, therefore
            // lets advance to the computer information page.
            //

            TraceMsg("Other failure, so just dumping the user into the domain join page");

            StrCmp(g_szCompDomain, g_szDomain);
            idNextPage = IDD_PSW_COMPINFO;
            break;
        }
    }


exit_gracefully:
    
    // Reset the "allow connectiod creation before login" value if appropriate
    if (fSetAllowKey)
    {
        SetAllowKey(dwPreviousAllowValue, NULL);
    }

    SysFreeString(bstrCompDomain);
    SetDlgItemText(hwnd, IDC_PASSWORD, L"");

    Trace(TEXT("idNextPage was %d (-1 is no advanced)"), idNextPage);
    WizardNext(hwnd, idNextPage);                       

    TraceLeave();
}


//
// wizard page to handle the user information (name, password and domain);
// 

BOOL _UserInfoBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;

    // the username/domain fields cannot be blank

    if ( !FetchTextLength(hwnd, IDC_USER) )
        dwButtons &= ~PSWIZB_NEXT;
    
    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_DOMAIN)))
    {
        if ( !FetchTextLength(hwnd, IDC_DOMAIN) )
            dwButtons &= ~PSWIZB_NEXT;
    }

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;
}

INT_PTR CALLBACK _UserInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), MAX_DOMAINUSER);
            Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), MAX_PASSWORD);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);

            // if we are launched from the netid tab then lets read the current 
            // user and domain and display accordingly.

            if ( g_uWizardIs == NAW_NETID ) 
            {
                DWORD dwcchUser = ARRAYSIZE(g_szUser);
                DWORD dwcchDomain = ARRAYSIZE(g_szDomain);
                GetCurrentUserAndDomainName(g_szUser, &dwcchUser, g_szDomain, &dwcchDomain);
                ShowWindow(GetDlgItem(hwnd, IDC_DIALUP), SW_HIDE);
            }

            SetDlgItemText(hwnd, IDC_USER, g_szUser);
            SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);

            EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));

            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                    return _UserInfoBtnState(hwnd);

                case PSN_WIZBACK:
                    WizardNext(hwnd, IDD_PSW_DOMAININFO);
                    return TRUE;

                case PSN_WIZNEXT:
                    _DoUserInfoNext(hwnd);      // handles setting the next page etc
                    return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                if ((IDC_USER == LOWORD(wParam)) || (IDC_DOMAIN == LOWORD(wParam)))
                {
                    EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
                    _UserInfoBtnState(hwnd);
                }
            }
            break;
        }
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ Handle the compter info page
/----------------------------------------------------------------------------*/

BOOL _IsTCPIPAvailable(void)
{
    TraceEnter(TRACE_PSW, "_IsTCPIPAvailable");
    
    BOOL fTCPIPAvailable = FALSE;
    HKEY hk;
    DWORD dwSize;

    //
    // we check to see if the TCP/IP stack is installed and which object it is
    // bound to, this is a string, we don't check the value only that the
    // length is non-zero.
    //

    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                       TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Linkage"),
                                       0x0, 
                                       KEY_QUERY_VALUE, &hk) )
    {
        if ( ERROR_SUCCESS == RegQueryValueEx(hk, 
                                              TEXT("Export"),
                                              0x0, 
                                              NULL, NULL, &dwSize) )
        {
            if ( dwSize > 2 )
            {
                TraceMsg("TCP/IP installed");
                fTCPIPAvailable = TRUE;
            }
        }

        RegCloseKey(hk);
    }

    TraceLeaveValue(fTCPIPAvailable);
}

/****************************************************************
_ChangeMachineName
    Does all of the crap necessary to change a machine's name in the domain and locally

  dsheldon - 03/18/99
****************************************************************/
BOOL _ChangeMachineName(HWND hwnd, WCHAR* pszDomainUser, WCHAR* pszPassword)
{
    TraceEnter(TRACE_PSW, "_ChangeMachineName");
    BOOL fSuccess = FALSE;

    // the user has entered a short computer name (possibly a DNS host name), retrieve it
    WCHAR szNewShortMachineName[MAX_COMPUTERNAME + 1];
    FetchText(hwnd, IDC_COMPUTERNAME, szNewShortMachineName, ARRAYSIZE(szNewShortMachineName));
    
    // get the current short computer name
    WCHAR szOldShortMachineName[MAX_COMPUTERNAME + 1];
    DWORD cchShort = ARRAYSIZE(szOldShortMachineName);
    BOOL fGotOldName = GetComputerName(szOldShortMachineName, &cchShort);
    if (fGotOldName)
    {
        // did the user change the short computer name?
        if (0 != StrCmpI(szOldShortMachineName, szNewShortMachineName))
        {
            // if so we need to rename the machine in the domain. For this we need the NetBIOS computer name
            WCHAR szNewNetBIOSMachineName[MAX_COMPUTERNAME + 1];

            // Get the netbios name from the short name
            DWORD cchNetbios = ARRAYSIZE(szNewNetBIOSMachineName);
            DnsHostnameToComputerName(szNewShortMachineName, szNewNetBIOSMachineName, &cchNetbios);

            // rename the computer in the domain
            NET_API_STATUS rename_status = ::NetRenameMachineInDomain(0, szNewNetBIOSMachineName,
                pszDomainUser, pszPassword, NETSETUP_ACCT_CREATE);

            // if the domain rename succeeded
            BOOL fDomainRenameSucceeded = (rename_status == ERROR_SUCCESS);
            if (fDomainRenameSucceeded)
            {
                // set the new short name locally
                BOOL fLocalRenameSucceeded;

                // do we have TCPIP?
                if (_IsTCPIPAvailable())
                {
                    // We can set the name using the short name
                    fLocalRenameSucceeded = ::SetComputerNameEx(ComputerNamePhysicalDnsHostname,
                        szNewShortMachineName);
                }
                else
                {
                    // We need to set using the netbios name - kind of a hack
                    fLocalRenameSucceeded = ::SetComputerNameEx(ComputerNamePhysicalNetBIOS,
                        szNewNetBIOSMachineName);
                }

                fSuccess = fLocalRenameSucceeded;
            }

			// Handle errors that may have occured changing the name
            if (rename_status != ERROR_SUCCESS)
            {
                TCHAR szMessage[512];

                switch (rename_status)
                {
                case NERR_UserExists:
                    {
                        // We don't really mean "user exists" in this case, we mean
                        // "computer name exists", so load that reason string
                        LoadString(g_hInstance, IDS_COMPNAME_EXISTS, szMessage, ARRAYSIZE(szMessage));
                    }
                    break;
                default:
                    {
                        DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) rename_status, 0, szMessage, ARRAYSIZE(szMessage), NULL);

                        if (0 == dwFormatResult)
                        {
                            LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
                        }
                    }
                    break;
                }

                // Note that this is not a hard error, so we use the information icon
                ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_NAW_NAMECHANGE_ERROR, MB_OK|MB_ICONINFORMATION, szMessage);
            }
        }
		else
		{
			// Computer name hasn't changed - just return success
			fSuccess = TRUE;
		}
    }

    TraceLeaveValue(fSuccess);
}


//
// handle processing the changes
//

HRESULT _ChangeNameAndJoin(HWND hwnd)
{
    HRESULT hres;
    WCHAR szDomain[MAX_DOMAIN + 1];
    WCHAR szUser[MAX_DOMAINUSER + 1]; szUser[0] = 0;
    WCHAR szPassword[MAX_PASSWORD + 1]; szPassword[0] = 0;

    BOOL fNameChangeSucceeded = FALSE;

    TraceEnter(TRACE_PSW, "_ChangeNameAndJoin");

    FetchText(hwnd, IDC_DOMAIN, szDomain, ARRAYSIZE(szDomain));

    Trace(TEXT("szDomain: %s"), szDomain);

    // 
    // try to join the new domain
    //
    
    TCHAR szUserDomain[MAX_DOMAIN + 1]; *szUserDomain = 0;
    CREDINFO ci = { szUser, ARRAYSIZE(szUser), szUserDomain, ARRAYSIZE(szUserDomain), szPassword, ARRAYSIZE(szPassword) };

    hres = JoinDomain(hwnd, TRUE, szDomain, &ci);
    FailGracefully(hres, "Failed to join domain");

#ifndef DONT_JOIN

    fNameChangeSucceeded = _ChangeMachineName(hwnd,
        szUser[0] ? szUser : NULL,
        szPassword[0] ?szPassword : NULL);

    if ( !fNameChangeSucceeded )
    {
		// We'll allow the whole operation to succeed even if the machine name couldn't be changed
        Trace(TEXT("Failed to change the computer name"));
    }
#endif

    hres = S_OK;                // success

exit_gracefully:

    TraceLeaveResult(hres);
}


//
// set the wizard buttons accordingly
//

BOOL _CompInfoBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;

    if ( !FetchTextLength(hwnd, IDC_COMPUTERNAME) )
        dwButtons &= ~PSWIZB_NEXT;
    if ( !FetchTextLength(hwnd, IDC_DOMAIN) )
        dwButtons &= ~PSWIZB_NEXT;

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;
}

/*********************************************************************************
    _ValidateMachineName

  in: HWND hwnd - window that contains machine name edit box and parent for errors
  out: BOOL - TRUE if machine name is available and FALSE if machine name is invalid

  If FALSE is returned, errors have already been shown to the user

  by:
  dsheldon - 04/16/99
*********************************************************************************/
BOOL _ValidateMachineName(HWND hwnd)
{
    BOOL fNameInUse = FALSE;
    NET_API_STATUS name_status = NERR_Success;

    // the user has entered a short computer name (possibly a DNS host name), retrieve it
    WCHAR szNewShortMachineName[MAX_COMPUTERNAME + 1];
    FetchText(hwnd, IDC_COMPUTERNAME, szNewShortMachineName, ARRAYSIZE(szNewShortMachineName));
    
    // get the current short computer name
    WCHAR szOldShortMachineName[MAX_COMPUTERNAME + 1];
    DWORD cchShort = ARRAYSIZE(szOldShortMachineName);
    BOOL fGotOldName = GetComputerName(szOldShortMachineName, &cchShort);
    if (fGotOldName)
    {
        // did the user change the short computer name?
        if (0 != StrCmpI(szOldShortMachineName, szNewShortMachineName))
        {
            // first we need to check the flat, netbios name
            WCHAR szNewNetBIOSMachineName[MAX_COMPUTERNAME + 1];

            // Get the netbios name from the short name
            DWORD cchNetbios = ARRAYSIZE(szNewNetBIOSMachineName);
            DnsHostnameToComputerName(szNewShortMachineName, szNewNetBIOSMachineName, &cchNetbios);
            
            name_status = NetValidateName(NULL, szNewNetBIOSMachineName, NULL, NULL, NetSetupMachine);
        }
    }

    if (name_status != NERR_Success)
    {
        TCHAR szMessage[512];
        DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) name_status, 0, szMessage, ARRAYSIZE(szMessage), NULL);

        if (0 == dwFormatResult)
        {
            LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
        }

        DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_MACHINENAMEINUSE, MB_ICONERROR | MB_OK, szMessage);
    }

    return (name_status == NERR_Success);
}

//
// handle the dialog proc
//

INT_PTR CALLBACK _CompInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);
            Edit_LimitText(GetDlgItem(hwnd, IDC_COMPUTERNAME), MAX_COMPUTERNAME);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    WCHAR szCompName[MAX_PATH + 1], szMessage[MAX_PATH+MAX_DOMAIN];
                    DWORD dwBuffer = ARRAYSIZE(szCompName);

                    // fill in the user domain

                    FormatMessageString(IDS_COMPNOTFOUND, szMessage, ARRAYSIZE(szMessage), g_szDomain);
                    SetDlgItemText(hwnd, IDC_COMPINFO, szMessage);

                    // default the computer name to something sensible

                    GetComputerName(szCompName, &dwBuffer);

                    SetDlgItemText(hwnd, IDC_COMPUTERNAME, szCompName);
                    SetDlgItemText(hwnd, IDC_DOMAIN, g_szCompDomain);

                    return _CompInfoBtnState(hwnd);
                }

                case PSN_WIZBACK:
                    WizardNext(hwnd, IDD_PSW_USERINFO);
                    return TRUE;

                case PSN_WIZNEXT:
                {
                    INT idNextPage = -1;

                    if (_ValidateMachineName(hwnd))
                    {
                        if (SUCCEEDED(_ChangeNameAndJoin(hwnd)))
                        {
                            idNextPage = IDD_PSW_ADDUSER;
                        }
                    }

                    WizardNext(hwnd, idNextPage);
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            if ( HIWORD(wParam) == EN_CHANGE )
                return _CompInfoBtnState(hwnd);

            break;
        }
    }

    return FALSE;
}



/*-----------------------------------------------------------------------------
/ Allow changing of the group membership of an account on this machine.
/----------------------------------------------------------------------------*/

//
// attempt to add the specified user to the group (specified by an
// index).
//

BOOL _AddUserToGroup(HWND hwnd, LPCTSTR pszLocalGroup, LPCWSTR pszUser, LPCWSTR pszDomain)
{
#ifndef DONT_JOIN
    BOOL fResult = FALSE;
    NET_API_STATUS nas;
    LOCALGROUP_MEMBERS_INFO_3 lgm;
    TCHAR szDomainUser[MAX_DOMAINUSER + 1];
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_PSW, "_AddUserToGroup");
    Trace(TEXT("pszLocalGroup: %s, pszUser: %s, pszDomain: %s"), pszLocalGroup, pszUser, pszDomain);

    MakeDomainUserString(pszDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));
    lgm.lgrmi3_domainandname = szDomainUser;

    Trace(TEXT("Domain user string: %s"), szDomainUser);

    SetWaitCursor();
    nas = NetLocalGroupAddMembers(NULL, pszLocalGroup, 3, (BYTE *)&lgm, 1);
    ResetWaitCursor();

    Trace(TEXT("NetLocalGropuAddMembers returned %x (%d)"), nas, nas);

    switch ( nas )
    {
        // Success conditions
        case NERR_Success:
        case ERROR_MEMBER_IN_GROUP:
        case ERROR_MEMBER_IN_ALIAS:
        {
            fResult = TRUE;
            break;
        }
        case ERROR_INVALID_MEMBER:
        {
            TraceMsg("Invalid member");

            DisplayFormatMessage(hwnd,
                            IDS_PERMISSIONS,
                            IDS_ERR_BADUSER,                            
                            MB_OK|MB_ICONWARNING,
                            pszUser, pszDomain);
                        
            break;
        }

        case ERROR_NO_SUCH_MEMBER:
        {
            TraceMsg("No such member");

            DisplayFormatMessage(hwnd,
                            IDS_PERMISSIONS,
                            IDS_ERR_NOSUCHUSER,
                            MB_OK|MB_ICONWARNING,
                            pszUser, pszDomain);
            break;
        }
        default:
        {
            // Unexpected error
            TCHAR szMessage[512];
            DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) nas, 0, szMessage, ARRAYSIZE(szMessage), NULL);

            if (0 == dwFormatResult)
            {
                LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
            }

            ::DisplayFormatMessage(hwnd, IDS_ERR_CAPTION, IDS_ERR_ADDUSER, MB_OK|MB_ICONERROR, szMessage);

            fResult = FALSE;

            break;
        }
    }

    TraceLeaveValue(fResult);
#else
    TraceEnter(TRACE_PSW, "Group");
    TraceLeaveValue(TRUE);
#endif
}



//
// ensure the wizard buttons reflect what we can do
//

BOOL _PermissionsBtnState(HWND hwnd)
{
    // Next is always valid
    DWORD dwButtons = PSWIZB_NEXT | PSWIZB_BACK;

    SetWizardButtons(hwnd, dwButtons);
    return TRUE;              
}

//
// BtnState function for _AddUserDlgProc
//
BOOL _AddUserBtnState(HWND hwnd)
{
    DWORD dwButtons = PSWIZB_NEXT|PSWIZB_BACK;
    BOOL fEnableEdits;

    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_ADDUSER)))
    {
        // Enable the user and domain edits
        fEnableEdits = TRUE;

        if ( !FetchTextLength(hwnd, IDC_USER) )
            dwButtons &= ~PSWIZB_NEXT;
    }
    else
    {
        // Disable user and domain edits
        fEnableEdits = FALSE;
    }

    EnableWindow(GetDlgItem(hwnd, IDC_USER), fEnableEdits);

    if (fEnableEdits)
    {
        EnableDomainForUPN(GetDlgItem(hwnd, IDC_USER), GetDlgItem(hwnd, IDC_DOMAIN));
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_DOMAIN), FALSE);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_USER_STATIC), fEnableEdits);
    EnableWindow(GetDlgItem(hwnd, IDC_DOMAIN_STATIC), fEnableEdits);
    SetWizardButtons(hwnd, dwButtons);
    return TRUE;              
}


//
// DlgProc for the select-user-for-permission page
//

INT_PTR CALLBACK _AddUserDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Edit_LimitText(GetDlgItem(hwnd, IDC_USER), MAX_DOMAINUSER);
            Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);
            Button_SetCheck(GetDlgItem(hwnd, IDC_ADDUSER), BST_CHECKED);
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    SetDlgItemText(hwnd, IDC_USER, g_szUser);
                    SetDlgItemText(hwnd, IDC_DOMAIN, g_szDomain);

                    _AddUserBtnState(hwnd);
                    return TRUE;
                }
                case PSN_WIZBACK:
                {
                    if ( g_uWizardIs == NAW_PSDOMAINJOINED )
                        WizardNext(hwnd, IDD_PSW_WELCOME);
                    else
                        WizardNext(hwnd, IDD_PSW_USERINFO);

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_ADDUSER)))
                    {
                        FetchText(hwnd, IDC_USER, g_szUser, ARRAYSIZE(g_szUser));
                        FetchText(hwnd, IDC_DOMAIN, g_szDomain, ARRAYSIZE(g_szDomain));

                        if (StrChr(g_szUser, TEXT('@')))
                        {
                            *g_szDomain = 0;
                        }

                        WizardNext(hwnd, IDD_PSW_PERMISSIONS);
                    }
                    else
                    {
                        WizardNext(hwnd, IDD_PSW_DONE);
                    }

                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch ( HIWORD(wParam) )
            {            
                case EN_CHANGE:
                case BN_CLICKED:
                    _AddUserBtnState(hwnd);
                    break;
            }
            break;
        }
    }

    return FALSE;
}


//
// DlgProc for the permissions page.
//

INT_PTR CALLBACK _PermissionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Handle local-group related messages
    g_pGroupPageBase->HandleGroupMessage(hwnd, uMsg, wParam, lParam);

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;             
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:            
                {
                    // Set the "What level of access do you want to grant %S" message

                    TCHAR szMessage[256];
                    TCHAR szDisplayName[MAX_DOMAINUSER];
    
                    // Make a domain/user string
                    MakeDomainUserString(g_szDomain, g_szUser, szDisplayName, ARRAYSIZE(szDisplayName));

                    FormatMessageString(IDS_WHATACCESS_FORMAT, szMessage, ARRAYSIZE(szMessage), szDisplayName);
                    SetDlgItemText(hwnd, IDC_WHATACCESS, szMessage);
                    
                    return _PermissionsBtnState(hwnd);
                }
                case PSN_WIZBACK:
                {
                    WizardNext(hwnd, IDD_PSW_ADDUSER);

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    // Get the local group here! TODO
                    TCHAR szGroup[MAX_GROUP + 1];

                    CUserInfo::GROUPPSEUDONYM gs;
                    g_pGroupPageBase->GetSelectedGroup(hwnd, szGroup, ARRAYSIZE(szGroup), &gs);

                    if ( !_AddUserToGroup(hwnd, szGroup, g_szUser, g_szDomain) )
                    {
                        WizardNext(hwnd, -1);
                    }
                    else
                    {
                        SetDefAccount(g_szUser, g_szDomain);
                        WizardNext(hwnd, IDD_PSW_DONE);
                    }

                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch ( HIWORD(wParam) )
            {            
                case EN_CHANGE:
                    return _PermissionsBtnState(hwnd);
            }
            break;
        }
    }

    return FALSE;
}
