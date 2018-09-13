//
// ident.cpp - implementation of CUserIdentity class
//
#include "private.h"
#include "shlwapi.h"
#include "multiusr.h"
#include "strconst.h"
#include "multiutl.h"
#include <shfolder.h>

//
// Constructor / destructor
//
CUserIdentity::CUserIdentity()
    : m_cRef(1),
      m_fSaved(FALSE),
      m_fUsePassword(0)
{
    m_szUsername[0] = 0;
    m_szPassword[0] = 0;
    ZeroMemory(&m_uidCookie, sizeof(GUID));

    DllAddRef();
}


CUserIdentity::~CUserIdentity()
{
    DllRelease();
}


//
// IUnknown members
//
STDMETHODIMP CUserIdentity::QueryInterface(
    REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }
    
    *ppv=NULL;

    // Validate requested interface
    if(IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
    }
    else if(IID_IUserIdentity == riid)
    {
        *ppv = (IUserIdentity *)this;
    }

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CUserIdentity::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CUserIdentity::Release()
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}


// 
// IUserIdentity members
//
STDMETHODIMP CUserIdentity::GetCookie(GUID *puidCookie)
{
    if (!m_fSaved)
        return E_INVALIDARG;

    *puidCookie = m_uidCookie;
    return S_OK;

}

STDMETHODIMP CUserIdentity::OpenIdentityRegKey(DWORD dwDesiredAccess, HKEY *phKey)
{
    TCHAR    szRootPath[MAX_PATH];
    HRESULT  hr = S_OK;

    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    MU_GetRegRootForUserID(&m_uidCookie, szRootPath);

    hr = RegCreateKey(HKEY_CURRENT_USER, szRootPath, phKey);
    RegCloseKey(*phKey);

    hr = RegOpenKeyEx(HKEY_CURRENT_USER, szRootPath, 0, dwDesiredAccess, phKey);
    return (hr == ERROR_SUCCESS ? S_OK : E_FAIL);
}


STDMETHODIMP CUserIdentity::GetIdentityFolder(DWORD dwFlags, WCHAR *pszPath, ULONG ulBuffSize)
{
    WCHAR    szwRootPath[MAX_PATH];
    HRESULT hr;

    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    hr = MU_GetUserDirectoryRoot(&m_uidCookie, dwFlags, szwRootPath, MAX_PATH);
    
    if (SUCCEEDED(hr))
    {
        StrCpyW(pszPath, szwRootPath);
    }

    return hr;
}



STDMETHODIMP CUserIdentity::GetName(WCHAR *pszName, ULONG ulBuffSize)
{
    if (!m_fSaved || ulBuffSize == 0)
        return E_IDENTITY_NOT_FOUND;

    if (MultiByteToWideChar(CP_ACP, 0, m_szUsername, -1, pszName, ulBuffSize) == 0)
        return GetLastError();
    
    return S_OK;
}

STDMETHODIMP CUserIdentity::SetName(WCHAR *pszName)
{
    TCHAR       szRegPath[MAX_PATH];
    HRESULT     hr = S_OK;
    HKEY        hKey;
    USERINFO    uiCurrent;
    LPARAM      lpNotify = IIC_CURRENT_IDENTITY_CHANGED;
    
    if (WideCharToMultiByte(CP_ACP, 0, pszName, -1, m_szUsername, CCH_USERNAME_MAX_LENGTH, NULL, NULL) == 0)
        return GetLastError();
    
    hr = _SaveUser();
    
    // if its not the current identity, then just broadcast that an identity changed
    if (MU_GetUserInfo(NULL, &uiCurrent) && (m_uidCookie != uiCurrent.uidUserID))
        lpNotify = IIC_IDENTITY_CHANGED;

    // tell apps that the user's name changed
    if (SUCCEEDED(hr))
        PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, lpNotify);

    return hr;
}

STDMETHODIMP CUserIdentity::SetPassword(WCHAR *pszPassword)
{
#ifdef IDENTITY_PASSWORDS
    TCHAR       szRegPath[MAX_PATH];
    HRESULT     hr = S_OK;
    HKEY        hKey;
    
    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    if (WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, m_szPassword, CCH_USERPASSWORD_MAX_LENGTH, NULL, NULL) == 0)
        return GetLastError();
    
    m_fUsePassword = (*m_szPassword != 0);

    hr = _SaveUser();

    return hr;
#else
    return E_NOTIMPL;
#endif
}


STDMETHODIMP CUserIdentity::_SaveUser()
{
    DWORD   dwType, dwSize, dwValue, dwStatus;
    HKEY    hkCurrUser;
    TCHAR   szPath[MAX_PATH];
    TCHAR   szUid[255];
    HRESULT hr;

    if (*m_szUsername == 0)
        return E_INVALIDARG;

    if (!m_fUsePassword)
        m_szPassword[0] = 0;

    if (!m_fSaved)
        hr = _ClaimNextUserId(&m_uidCookie);

    Assert(m_uidCookie != GUID_NULL);
    Assert(SUCCEEDED(hr));
    
    MU_GetRegRootForUserID(&m_uidCookie, szPath);
    
    Assert(pszRegPath && *pszRegPath);
    
    if ((dwStatus = RegCreateKey(HKEY_CURRENT_USER, szPath, &hkCurrUser)) == ERROR_SUCCESS)
    {
        ULONG   cbSize;
        TCHAR   szBuffer[255];
        
        // write out the correct values
        dwType = REG_SZ;
        dwSize = lstrlen(m_szUsername) + 1;
        RegSetValueEx(hkCurrUser, c_szUsername, 0, dwType, (LPBYTE)m_szUsername, dwSize);

#ifdef IDENTITY_PASSWORDS
        dwType = REG_BINARY ;
        cbSize = strlen(m_szPassword) + 1;
        lstrcpy(szBuffer, m_szPassword);
        EncodeUserPassword(szBuffer, &cbSize);
        dwSize = cbSize;
        RegSetValueEx(hkCurrUser, c_szPassword, 0, dwType, (LPBYTE)szBuffer, dwSize);

        dwType = REG_DWORD;
        dwValue = (m_fUsePassword ? 1 : 0);
        dwSize = sizeof(dwValue);
        RegSetValueEx(hkCurrUser, c_szUsePassword, 0, dwType, (LPBYTE)&dwValue, dwSize);
#endif

        // make sure the shortened name is there for directories.  If not, create it.
        dwSize = sizeof(DWORD);
        if ((dwStatus = RegQueryValueEx(hkCurrUser, c_szDirName, NULL, &dwType, (LPBYTE)&dwValue, &dwSize)) != ERROR_SUCCESS)
        {
            dwValue = MU_GenerateDirectoryNameForIdentity(&m_uidCookie);
        
            dwType = REG_DWORD;
            dwSize = sizeof(dwValue);
            RegSetValueEx(hkCurrUser, c_szDirName, 0, dwType, (LPBYTE)&dwValue, dwSize);
        }

        AStringFromGUID(&m_uidCookie,  szUid, 255);

        dwType = REG_SZ;
        dwSize = lstrlen(szUid) + 1;
        RegSetValueEx(hkCurrUser, c_szUserID, 0, dwType, (LPBYTE)&szUid, dwSize);

        RegCloseKey(hkCurrUser);
        
        m_fSaved = TRUE;

        return S_OK;
    }
    return E_FAIL;
}


STDMETHODIMP CUserIdentity::InitFromUsername(TCHAR *pszUsername)
{
    GUID   uidCookie;
    HRESULT hr;

    if(FAILED(hr = MU_UsernameToUserId(pszUsername, &uidCookie)))
        return hr;

    return InitFromCookie(&uidCookie);
}


STDMETHODIMP CUserIdentity::InitFromCookie(GUID *puidCookie)
{
    TCHAR   szPWBuffer[255];
    TCHAR   szRegPath[MAX_PATH];
    HKEY    hKey;
    BOOL    bResult = false;
    LONG    lValue;
    DWORD   dwStatus, dwType, dwSize;
    GUID    uidCookie;

    if( puidCookie == NULL)
        MU_GetCurrentUserID(&uidCookie);
    else
        uidCookie = *puidCookie;

    MU_GetRegRootForUserID(&uidCookie, szRegPath);
    
    Assert(pszRegPath && *pszRegPath);

    if (RegOpenKey(HKEY_CURRENT_USER, szRegPath, &hKey) == ERROR_SUCCESS)
    {
        *m_szPassword = 0;
        m_fUsePassword = false;
        ZeroMemory(&m_uidCookie, sizeof(GUID));

        dwSize = sizeof(m_szUsername);
        if ((dwStatus = RegQueryValueEx(hKey, c_szUsername, NULL, &dwType, (LPBYTE)m_szUsername, &dwSize)) == ERROR_SUCCESS &&
                (0 != *m_szUsername))
        {
            //we have the username, that is the only required part.  The others are optional.
            bResult = true;
            
#ifdef IDENTITY_PASSWORDS
            dwSize = sizeof(lValue);
            if ((dwStatus = RegQueryValueEx(hKey, c_szUsePassword, NULL, &dwType, (LPBYTE)&lValue, &dwSize)) == ERROR_SUCCESS)
            {
                m_fUsePassword = (lValue != 0);
            }

            dwSize = sizeof(szPWBuffer);
            dwStatus = RegQueryValueEx(hKey, c_szPassword, NULL, &dwType, (LPBYTE)szPWBuffer, &dwSize);
            ULONG   cbSize;
            
            cbSize = dwSize;
            if (ERROR_SUCCESS == dwStatus  && cbSize > 1)
            {
                DecodeUserPassword(szPWBuffer, &cbSize);
                strcpy(m_szPassword, szPWBuffer);
            }
            else
                *m_szPassword = 0;

#endif
            
            m_uidCookie = uidCookie;
            m_fSaved = TRUE;
        }
        RegCloseKey(hKey);
    }
        
    return (bResult ? S_OK : E_FAIL);
}
