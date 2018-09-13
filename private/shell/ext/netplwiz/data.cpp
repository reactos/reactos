/********************************************************
 data.cpp

  User Manager primary data class Implementation

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "data.h"
#include "misc.h"

CUserManagerData::CUserManagerData(LPCTSTR pszCurrentDomainUser)
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::CUserManagerData");
        
    m_szHelpfilePath[0] = TEXT('\0');

    // Initialize everything except for the user loader thread
    // and the group list here; the rest is done in
    // ::Initialize.
    
    // Fill in the computer name
    DWORD cchComputername = ARRAYSIZE(m_szComputername);
    ::GetComputerName(m_szComputername, &cchComputername);
 
    // Detect if 'puter is in a domain
    SetComputerDomainFlag();

    // Get the current user information
    DWORD cchUsername = ARRAYSIZE(m_LoggedOnUser.m_szUsername);
    DWORD cchDomain = ARRAYSIZE(m_LoggedOnUser.m_szDomain);
    GetCurrentUserAndDomainName(m_LoggedOnUser.m_szUsername, &cchUsername,
        m_LoggedOnUser.m_szDomain, &cchDomain);

    // Get the extra data for this user
    m_LoggedOnUser.GetExtraUserInfo();

    // We'll set logoff required only if the current user has been updated
    m_pszCurrentDomainUser = (LPTSTR) pszCurrentDomainUser;
    m_fLogoffRequired = FALSE;

    TraceLeaveVoid();
}

CUserManagerData::~CUserManagerData()
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::~CUserManagerData");
    
    TraceLeaveVoid();
}

HRESULT CUserManagerData::Initialize(HWND hwndUserListPage)
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::Initialize");

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    HRESULT hr = S_OK;

    m_GroupList.Initialize();
    m_UserListLoader.Initialize(hwndUserListPage);

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    TraceLeaveResult(hr);
}

// Registry access constants for auto admin logon
static const TCHAR szWinlogonSubkey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static const TCHAR szAutologonValueName[] = TEXT("AutoAdminLogon");
static const TCHAR szDefaultUserNameValueName[] = TEXT("DefaultUserName");
static const TCHAR szDefaultDomainValueName[] = TEXT("DefaultDomainName");
static const TCHAR szDefaultPasswordValueName[] = TEXT("DefaultPassword");

BOOL CUserManagerData::IsAutologonEnabled()
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::IsAutologonEnabled");
    BOOL fAutologon = FALSE;


    // Read the registry to see if autologon is enabled
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szWinlogonSubkey, 0, 
        KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szResult[2];
        DWORD dwType;
        DWORD cbSize = sizeof(szResult);
        if (RegQueryValueEx(hkey, szAutologonValueName, 0, &dwType, (BYTE*) szResult, &cbSize) !=
            ERROR_SUCCESS)
        {
            TraceMsg("RegQueryValueEx failed");
        }
        else
        {
            long lResult = StrToLong(szResult);
            fAutologon = (lResult != 0);
        }

        RegCloseKey(hkey);
    }
    else
    {
        TraceMsg("RegOpenKeyEx failed");
    }

    TraceLeaveValue(fAutologon);
}

#define STRINGBYTESIZE(x) ((lstrlen((x)) + 1) * sizeof(TCHAR))

void CUserManagerData::SetComputerDomainFlag()
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::SetComputerDomainFlag");

    m_fInDomain = ::IsComputerInDomain();

    TraceLeaveVoid();
}

TCHAR* CUserManagerData::GetHelpfilePath()
{
    TraceEnter(TRACE_USR_CORE, "CUserManagerData::GetHelpfilePath");

    static const TCHAR szHelpfileUnexpanded[] = 
        TEXT("%systemroot%\\system32\\users.hlp");

    if (m_szHelpfilePath[0] == TEXT('\0'))
    {
        ExpandEnvironmentStrings(szHelpfileUnexpanded, m_szHelpfilePath, 
            ARRAYSIZE(m_szHelpfilePath));
    }

    TraceLeaveValue(m_szHelpfilePath);
}

void CUserManagerData::UserInfoChanged(LPCTSTR pszUser, LPCTSTR pszDomain)
{
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2]; szDomainUser[0] = 0;

    MakeDomainUserString(pszDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));

    if (StrCmpI(szDomainUser, m_pszCurrentDomainUser) == 0)
    {
        m_fLogoffRequired = TRUE;
    }
}

BOOL CUserManagerData::LogoffRequired()
{
    TraceLeaveValue(m_fLogoffRequired);
}
