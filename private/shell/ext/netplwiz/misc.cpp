/********************************************************
 misc.cpp

  Utility function implementation

 History:
  09/23/98: dsheldon created
********************************************************/
#include "stdafx.h"

#include "misc.h"

// Miscellanious function implementations

HRESULT ValidateName(LPCTSTR pszName)
{
    // We need to use illegal fat chars, says SBurns
    TCHAR* pszBadChars = ILLEGAL_FAT_CHARS;
    HRESULT hrStringOK = S_OK;

    while ((NULL != *pszBadChars) && (hrStringOK == S_OK))
    {
        if (NULL != StrChr(pszName, *pszBadChars))
        {
            hrStringOK = E_FAIL;
        }
        else
        {
            pszBadChars ++;
        }
    }

    if (SUCCEEDED(hrStringOK))
    {
        // See if the whole string is dots
        TCHAR* pszChar = const_cast<TCHAR*>(pszName);
        BOOL fAllDots = TRUE;

        while (fAllDots && (0 != *pszChar))
        {
            if (TEXT('.') == *pszChar)
            {
                pszChar ++;
            }
            else
            {
                fAllDots = FALSE;
            }
        }
        
        if (fAllDots)
        {
            hrStringOK = E_FAIL;
        }
    }

    return hrStringOK;
}

HRESULT ParseDisplayNameHelper(HWND hwnd, LPTSTR szPath, LPITEMIDLIST* ppidl)
{
    USES_CONVERSION;
    LPSHELLFOLDER psf;
    HRESULT hr = SHGetDesktopFolder(&psf);

    if (SUCCEEDED(hr))
    {
        ULONG cchEaten;
        hr = psf->ParseDisplayName(hwnd, 0, T2W(szPath), &cchEaten, ppidl, NULL);
        psf->Release();
    }

    return hr;
}

void FetchText(HWND hWndDlg, UINT uID, LPTSTR lpBuffer, DWORD dwMaxSize)
// This routine fetches the text from a control and
// strips off the leading and trailing whitespace
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering FetchText\n"));
    TCHAR*  pszTemp;
    LPTSTR  pszString;

    *lpBuffer = L'\0';

    HWND hwndCtl = GetDlgItem(hWndDlg, uID);

    if (hwndCtl)
    {
        int iSize = GetWindowTextLength(hwndCtl);

        pszTemp = new TCHAR[iSize + 1];

        if (pszTemp)
        {
            GetWindowText(hwndCtl, pszTemp, iSize + 1);

            for (pszString = pszTemp ; *pszString && (*pszString == L' ') ; )
                pszString++; 

            if ( !*pszString )
                return;

            lstrcpyn(lpBuffer, pszString, dwMaxSize);
            pszString = lpBuffer+(lstrlen(lpBuffer)-1);

            while ( (pszString > lpBuffer) && (*pszString == L' ') )
                pszString--;

            *++pszString = L'\0';

            delete [] pszTemp;
        }
    }
}

INT FetchTextLength(HWND hWndDlg, UINT uID) 
{
    TCHAR szBuffer[MAX_PATH];
    FetchText(hWndDlg, uID, szBuffer, ARRAYSIZE(szBuffer));
    return lstrlen(szBuffer);
}

HRESULT AttemptLookupAccountName(LPCTSTR szUsername, PSID* ppsid,
    LPTSTR szDomain, DWORD* pcchDomain, SID_NAME_USE* psUse)
{
    TraceEnter(TRACE_USR_CORE, "::AttemptLookupAccountName");
    HRESULT hr = S_OK;

    // Attempt to allocate and lookup a SID
    TraceAssert(*ppsid == NULL);

    // First try to find required size of SID
    DWORD cbSid = 0;
    DWORD cchDomain = *pcchDomain;
    BOOL fSuccess = LookupAccountName(NULL, szUsername, *ppsid, &cbSid,
        szDomain, pcchDomain, psUse);

    // Should have failed since we passed 0 as SID buffer size
    TraceAssert(!fSuccess);

    // Now create the SID buffer and try again
    *ppsid = LocalAlloc(0, cbSid);

    if (*ppsid != NULL)
    {
        *pcchDomain = cchDomain;
        fSuccess = LookupAccountName(NULL, szUsername, *ppsid, &cbSid,
            szDomain, pcchDomain, psUse);

        if (!fSuccess)
        {
            // Free our allocated SID
            LocalFree(*ppsid);
            *ppsid = NULL;
            hr = E_FAIL;
        }
    }
    else
    {
        TraceMsg("Couldn't LocalAlloc SID");
        hr = E_OUTOFMEMORY;
    }

    TraceLeaveResult(hr);
}

BOOL FormatMessageString(UINT idTemplate, LPTSTR pszStrOut, DWORD cchSize, ...)
{
    BOOL fResult = FALSE;

    va_list vaParamList;
    
    TCHAR szFormat[MAX_STATIC + 1];
    if (LoadString(g_hInstance, idTemplate, szFormat, ARRAYSIZE(szFormat)))
    {
        va_start(vaParamList, cchSize);
        
        fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, pszStrOut, cchSize, &vaParamList);

        va_end(vaParamList);
    }

    return fResult;
}

int DisplayFormatMessage(HWND hwnd, UINT idCaption, UINT idFormatString, UINT uType, ...)
{
    TraceEnter(TRACE_USR_CORE, "::DisplayFormatMessage");

    int iResult = IDCANCEL;
    TCHAR szError[MAX_STATIC + 1]; *szError = 0;
    TCHAR szCaption[MAX_CAPTION + 1];
    TCHAR szFormat[MAX_STATIC + 1]; *szFormat = 0;

    // Load and format the error body
    if (LoadString(g_hInstance, idFormatString, szFormat, ARRAYSIZE(szFormat)))
    {
        va_list arguments;
        va_start(arguments, uType);

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, szError, ARRAYSIZE(szError), &arguments))
        {
            // Load the caption
            if (LoadString(g_hInstance, idCaption, szCaption, MAX_CAPTION))
            {
                iResult = MessageBox(hwnd, szError, szCaption, uType);
            }
        }

        va_end(arguments);
    }


    TraceLeaveValue(iResult);
}

void EnableControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, BOOL fEnable)
{
    DWORD i;
    for (i = 0; i < cIDs; i ++)
    {
        EnableWindow(GetDlgItem(hwnd, prgIDs[i]), fEnable);
    }
}

void MakeDomainUserString(LPCTSTR szDomain, LPCTSTR szUsername, LPTSTR szDomainUser, DWORD cchBuffer)
{
    *szDomainUser = 0;

    if ((!szDomain) || szDomain[0] == TEXT('\0'))
    {
        // No domain - just use username
        lstrcpyn(szDomainUser, szUsername, cchBuffer);
    }
    else
    {
        // Otherwise we have to build a DOMAIN\username string
        wnsprintf(szDomainUser, cchBuffer, TEXT("%s\\%s"), szDomain, szUsername);
    }    
}

// From the NT knowledge base
BOOL GetCurrentUserAndDomainName(LPTSTR UserName, LPDWORD cchUserName, LPTSTR DomainName, LPDWORD cchDomainName)
{
    TraceEnter(TRACE_USR_CORE, "::GetCurrentUserAndDomainName");

    HANDLE hToken;
    #define MY_BUFSIZE 512  // highly unlikely to exceed 512 bytes
    
    UCHAR InfoBuffer[ MY_BUFSIZE ];
    DWORD cbInfoBuffer = MY_BUFSIZE;
    
    SID_NAME_USE snu;
    BOOL bSuccess;
    
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) 
    {
        if(GetLastError() == ERROR_NO_TOKEN) 
        {   
            //
            // attempt to open the process token, since no thread token
            // exists
            //               
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) 
                TraceLeaveValue(FALSE);           
        } 
        else 
        {
            //
            // error trying to get thread token
            //
            TraceLeaveValue(FALSE);           
        }
    }
    
    bSuccess = GetTokenInformation(hToken, TokenUser, InfoBuffer, cbInfoBuffer,
        &cbInfoBuffer);
    
    if(!bSuccess) 
    {
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        {
            //
            // alloc buffer and try GetTokenInformation() again
            //
            CloseHandle(hToken);
            TraceLeaveValue(FALSE);
        } 
        else 
        {
            //
            // error getting token info
            //
            CloseHandle(hToken);
            TraceLeaveValue(FALSE);
        }
    }
    CloseHandle(hToken);
    
    TraceLeaveValue(LookupAccountSid(NULL, ((PTOKEN_USER)InfoBuffer)->User.Sid,
        UserName, cchUserName, DomainName, cchDomainName, &snu));
}

HRESULT IsUserLocalAdmin(HANDLE TokenHandle, BOOL* pfIsAdmin)
{
    // Pass NULL as TokenHandle to see if thread token is admin

    TraceEnter(TRACE_USR_CORE, "::IsUserLocalAdmin");
    TraceAssert(pfIsAdmin != NULL);

    HRESULT hr;
    BOOL fSuccess;
    // First we must check if the current user is a local administrator; if this is
    // the case, our dialog doesn't even display

    // Get the admin localgroup SID
    PSID psidAdminGroup = NULL;
    
    SID_IDENTIFIER_AUTHORITY security_nt_authority = SECURITY_NT_AUTHORITY;
    
    fSuccess = ::AllocateAndInitializeSid(&security_nt_authority, 2, 
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
        &psidAdminGroup);

    if (fSuccess)
    {
        // See if the user for this process is a local admin
        fSuccess = CheckTokenMembership(TokenHandle, psidAdminGroup, pfIsAdmin);

        if (!fSuccess)
        {
            TraceMsg("CheckTokenMembership failed");
        }

        FreeSid(psidAdminGroup);
    }
    else
    {
        TraceMsg("AllocateAndInitializeSid failed to get local admin group SID");
    }

    if (fSuccess)
        hr = S_OK;
    else
        hr = E_FAIL;

    TraceLeaveResult(hr);
}

BOOL IsComputerInDomain()
{
    TraceEnter(TRACE_USR_CORE, "::IsComputerInDomain");

    static BOOL fInDomain = FALSE;
    static BOOL fValid = FALSE;

    if (!fValid)
    {
        fValid = TRUE;

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdspdinfb = {0};
        DWORD err = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, 
            (BYTE**) &pdspdinfb);

        if ((err == NO_ERROR) && (pdspdinfb != NULL))
        {
            if ((pdspdinfb->MachineRole == DsRole_RoleStandaloneWorkstation) ||
                (pdspdinfb->MachineRole == DsRole_RoleStandaloneServer))
            {
                fInDomain = FALSE;
            }
            else
            {
                fInDomain = TRUE;
            }

            DsRoleFreeMemory(pdspdinfb);
        }
        else
        {
            TraceMsg("DsRoleGetPrimaryDomainInformation failed,");
        }
    }
    
    TraceLeaveValue(fInDomain);
}

void OffsetControls(HWND hwnd, const UINT* prgIDs, DWORD cIDs, int dx, int dy)
{
    TraceEnter(TRACE_USR_CORE, "::OffsetControlsEnableControls");

    for (DWORD i = 0; i < cIDs; i ++)
    {
        OffsetWindow(GetDlgItem(hwnd, prgIDs[i]), dx, dy);
    }

    TraceLeaveVoid();
}

void OffsetWindow(HWND hwnd, int dx, int dy)
{
    TraceEnter(TRACE_USR_CORE, "::OffsetWindow");

    RECT rc;
    GetWindowRect(hwnd, &rc);
    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rc, 2);
    OffsetRect(&rc, dx, dy);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

    TraceLeaveVoid();
}

BOOL AddPropSheetPageCallback(HPROPSHEETPAGE hpsp, LPARAM lParam)
{
    TraceEnter(TRACE_USR_COM, "::AddPropSheetPageCallback");
    BOOL fSuccess = FALSE;
    
    // lParam is really a ADDPROPSHEETDATA*
    ADDPROPSHEETDATA* ppsd = (ADDPROPSHEETDATA*) lParam;

    if (ppsd->nPages < ARRAYSIZE(ppsd->rgPages))
    {
        ppsd->rgPages[ppsd->nPages++] = hpsp;
        fSuccess = TRUE;
    }

    TraceLeaveValue(fSuccess);
}

// Code to ensure only one instance of a particular window is running
CEnsureSingleInstance::CEnsureSingleInstance(LPCTSTR szCaption)
{
    // Create an event
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, szCaption);

    // If any weird errors occur, default to running the instance
    m_fShouldExit = FALSE;

    if (NULL != m_hEvent)
    {
        // If our event isn't signaled, we're the first instance
        m_fShouldExit = (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvent, 0));

        if (m_fShouldExit)
        {
            // app should exit after calling ShouldExit()

            // Find and show the caption'd window
            HWND hwndActivate = FindWindow(NULL, szCaption);
            if (IsWindow(hwndActivate))
            {
                SetForegroundWindow(hwndActivate);
            }
        }
        else
        {
            // Signal that damn event
            SetEvent(m_hEvent);
        }
    }
}

CEnsureSingleInstance::~CEnsureSingleInstance()
{
    if (NULL != m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
}

#ifdef WINNT
// Browse for a user
HRESULT BrowseForUser(HWND hwndDlg, TCHAR* pszUser, DWORD cchUser,
              TCHAR* pszDomain, DWORD cchDomain)
// This routine activates the appropriate Object Picker to allow
// the user to select a user
// uiTextLocation  -- The resource ID of the Edit control where the selected 
//                    object should be printed 
{
    TraceEnter(TRACE_MND_CORE, "::BrowseForUser");

    HRESULT             hr;
   
    DSOP_SCOPE_INIT_INFO scopeInfo = {0};
    scopeInfo.cbSize = sizeof (scopeInfo);
    scopeInfo.flType = 
        DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE   |
        DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE | 
        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN      |
        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN    |
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN            |
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    scopeInfo.flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
    scopeInfo.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    scopeInfo.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    scopeInfo.FilterFlags.Uplevel.flMixedModeOnly = 0;
    scopeInfo.FilterFlags.Uplevel.flNativeModeOnly = 0;
    scopeInfo.pwzADsPath = NULL;
    scopeInfo.pwzDcName = NULL;
    scopeInfo.hr = E_FAIL;

    DSOP_INIT_INFO initInfo = {0};
    initInfo.cbSize = sizeof (initInfo);
    initInfo.pwzTargetComputer = NULL;
    initInfo.cDsScopeInfos = 1;
    initInfo.aDsScopeInfos = &scopeInfo;
    initInfo.flOptions = 0;

    IDsObjectPicker* pPicker;
    
    hr = CoCreateInstance(CLSID_DsObjectPicker,
        NULL, CLSCTX_INPROC_SERVER, IID_IDsObjectPicker, 
        (LPVOID*) &pPicker);

    if (SUCCEEDED(hr))
    {
        hr = pPicker->Initialize(&initInfo);

        if (SUCCEEDED(hr))
        {
            IDataObject* pdo;
            hr = pPicker->InvokeDialog(hwndDlg, &pdo);

            // S_FALSE indicates cancel
            if ((S_OK == hr) && (NULL != pdo))
            {
                // Get the DS_SELECTION_LIST out of the data obj
                FORMATETC fmt;
                fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
                fmt.ptd = NULL;
                fmt.dwAspect = DVASPECT_CONTENT;
                fmt.lindex = -1;
                fmt.tymed = TYMED_HGLOBAL;

                STGMEDIUM medium = {0};
                
                hr = pdo->GetData(&fmt, &medium);

                if (SUCCEEDED(hr))
                {
                    DS_SELECTION_LIST* plist;
                    plist = (DS_SELECTION_LIST*)
                        GlobalLock(medium.hGlobal);

                    if (NULL != plist)
                    {
                        TraceAssert(1 == plist->cItems);

                        if (plist->cItems >= 1)
                        {
                            WCHAR szWinNTProviderName[MAX_DOMAIN + MAX_USER + 10];
                            lstrcpyn(szWinNTProviderName, plist->aDsSelection[0].pwzADsPath, ARRAYSIZE(szWinNTProviderName));

                            // Is the name in the correct format?
                            if (StrCmpNI(szWinNTProviderName, TEXT("WinNT://"), 8) == 0)
                            {
                                // Yes, copy over the user name and password
                                LPTSTR szDomain = szWinNTProviderName + 8;

                                LPTSTR szUser = StrChr(szDomain, TEXT('/'));

                                if (szUser)
                                {
                                    *szUser++ = 0;

                                    // Just in case, remove the trailing slash
                                    LPTSTR szTrailingSlash = StrChr(szUser, TEXT('/'));
                                    if (szTrailingSlash)
                                    {
                                        *szTrailingSlash = 0;
                                    }

                                    lstrcpyn(pszUser, szUser, cchUser);

                                    lstrcpyn(pszDomain, szDomain, cchDomain);

                                    hr = S_OK;
                                }
                            }
                        }
                    }
                    else
                    {
                        // No selection list!
                        hr = E_UNEXPECTED;
                    }

                    GlobalUnlock(medium.hGlobal);
                }

                pdo->Release();
            }
        }

        pPicker->Release();
    }

    TraceLeaveResult(hr);
}
#endif // WINNT

//
// create the intro/done large font for wizards
// 

HFONT GetIntroFont(HWND hwnd)
{
    static HFONT _hfontIntro;

    if ( !_hfontIntro )
    {
        TCHAR szBuffer[64];
        NONCLIENTMETRICS ncm = { 0 };
        LOGFONT lf;

        ncm.cbSize = SIZEOF(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        lf = ncm.lfMessageFont;
        LoadString(GLOBAL_HINSTANCE, IDS_TITLEFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
        lf.lfWeight = FW_BOLD;

        LoadString(GLOBAL_HINSTANCE, IDS_TITLEFONTSIZE, szBuffer, ARRAYSIZE(szBuffer));
        lf.lfHeight = 0 - (GetDeviceCaps(NULL, LOGPIXELSY) * StrToInt(szBuffer) / 72);

        _hfontIntro = CreateFontIndirect(&lf);
    }

    return _hfontIntro;
}


void DomainUserString_GetParts(LPCTSTR szDomainUser, LPTSTR szUser, DWORD cchUser, LPTSTR szDomain, DWORD cchDomain)
{
    // Check for invalid args
    if ((!szUser) ||
        (!szDomain) ||
        (!cchUser) ||
        (!cchDomain))
    {
        return;
    }
    else
    {
        *szUser = 0;
        *szDomain = 0;

        TCHAR szTemp[MAX_USER + MAX_DOMAIN + 2];
        lstrcpyn(szTemp, szDomainUser, ARRAYSIZE(szTemp));

        LPTSTR szWhack = StrChr(szTemp, TEXT('\\'));

        if (!szWhack)
        {
            // Also check for forward slash to be friendly
            szWhack = StrChr(szTemp, TEXT('/'));
        }

        if (szWhack)
        {
            LPTSTR szUserPointer = szWhack + 1;
            *szWhack = 0;

            // Temp now points to domain.
            lstrcpyn(szDomain, szTemp, cchDomain);
            lstrcpyn(szUser, szUserPointer, cchUser);
        }
        else
        {
            // Don't have a domain name - just a username
            lstrcpyn(szUser, szTemp, cchUser);
        }
    }
}

LPITEMIDLIST GetComputerParent()
{
    USES_CONVERSION;
    LPITEMIDLIST pidl = NULL;

    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        TCHAR szName[MAX_PATH];

        lstrcpy(szName, TEXT("\\\\"));

        LPTSTR pszAfterWhacks = szName + 2;

        DWORD cchName = MAX_PATH - 2;
        if (GetComputerName(pszAfterWhacks, &cchName))
        {
            hres = psfDesktop->ParseDisplayName(NULL, NULL, T2W(szName), NULL, &pidl, NULL);
            if (SUCCEEDED(hres))
            {
                ILRemoveLastID(pidl);
            }
        }
        else
        {
            hres = E_FAIL;
        }

        psfDesktop->Release();
    }

    if (FAILED(hres))
    {
        pidl = NULL;
    }

    return pidl;    
}

int CALLBACK ShareBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    TraceEnter(TRACE_MND_CORE, "::ShareBrowseCallback");
    
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        {
            // Try to set the selected item according to the path string passed in lpData
            LPTSTR pszPath = (LPTSTR) lpData;

            if (pszPath && pszPath[0])
            {
                int i = lstrlen(pszPath) - 1;
                if ((pszPath[i] == TEXT('\\')) ||
                    (pszPath[i] == TEXT('/')))
                {
                    pszPath[i] = 0;
                }
   
                SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) TRUE, (LPARAM) (LPTSTR) pszPath);
            }
            else
            {
                // Try to get the computer's container folder
                LPITEMIDLIST pidl = GetComputerParent();

                if (pidl)
                {
                    SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) FALSE, (LPARAM) (LPTSTR) pidl);                
                    ILFree(pidl);
                }
            }
        }
        break;

    case BFFM_SELCHANGED:
        // Disable OK if this isn't a UNC path type thing
        {
            TCHAR szPath[MAX_PATH];
            LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;

            BOOL fEnableOk = FALSE;
            if (SHGetPathFromIDList(pidl, szPath))
            {
                fEnableOk = PathIsUNC(szPath);
            }

            SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM) 0, (LPARAM) fEnableOk);
        }
        break;
    }

    TraceLeaveValue(0);
}

void RemoveControl(HWND hwnd, UINT idControl, UINT idNextControl, const UINT* prgMoveControls, DWORD cControls, BOOL fShrinkParent)
{
    HWND hwndControl = GetDlgItem(hwnd, idControl);
    HWND hwndNextControl = GetDlgItem(hwnd, idNextControl);
    RECT rcControl;
    RECT rcNextControl;

    if (hwndControl && GetWindowRect(hwndControl, &rcControl) && 
        hwndNextControl && GetWindowRect(hwndNextControl, &rcNextControl))
    {
        int dx = rcControl.left - rcNextControl.left;
        int dy = rcControl.top - rcNextControl.top;

        MoveControls(hwnd, prgMoveControls, cControls, dx, dy);

        if (fShrinkParent)
        {
            RECT rcParent;

            if (GetWindowRect(hwnd, &rcParent))
            {
                MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rcParent, 2);

                rcParent.right += dx;
                rcParent.bottom += dy;

                SetWindowPos(hwnd, NULL, 0, 0, RECTWIDTH(rcParent), RECTHEIGHT(rcParent), SWP_NOMOVE | SWP_NOZORDER);
            }
        }

        EnableWindow(hwndControl, FALSE);
        ShowWindow(hwndControl, SW_HIDE);
    }
}

void MoveControls(HWND hwnd, const UINT* prgControls, DWORD cControls, int dx, int dy)
{
    DWORD iControl;
    for (iControl = 0; iControl < cControls; iControl ++)
    {
        HWND hwndControl = GetDlgItem(hwnd, prgControls[iControl]);
        
        RECT rcControl;
        
        if (hwndControl && GetWindowRect(hwndControl, &rcControl))
        {
            MapWindowPoints(NULL, hwnd, (LPPOINT)&rcControl, 2);
            SetWindowPos(hwndControl, NULL, rcControl.left + dx, rcControl.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
}

/***************************************************************************\
* FUNCTION: EnableDomainForUPN
*
* PURPOSE:  Enables or disables the domain text box based on whether or not
*           a UPN-style name is typed into the username box.
*
* RETURNS:  none
*
* HISTORY:
*
*   04-17-1998 dsheldon created
*
\***************************************************************************/
void EnableDomainForUPN(HWND hwndUsername, HWND hwndDomain)
{
    BOOL fEnable;

    // Get the string the user is typing
    TCHAR* pszLogonName;
    int cchBuffer = (int)SendMessage(hwndUsername, WM_GETTEXTLENGTH, 0, 0) + 1;

    pszLogonName = (TCHAR*) LocalAlloc(0, cchBuffer * sizeof(TCHAR));
    if (pszLogonName != NULL)
    {
        SendMessage(hwndUsername, WM_GETTEXT, (WPARAM) cchBuffer, (LPARAM) pszLogonName);

        // Disable the domain combo if the user is using a
        // UPN (if there is a "@") - ie foo@microsoft.com
        fEnable = (NULL == StrChr(pszLogonName, TEXT('@')));

        EnableWindow(hwndDomain, fEnable);

        LocalFree(pszLogonName);
    }
}