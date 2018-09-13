#include <urlmon.hxx>
#include "wreg.hxx"

#define DEB_REG DEB_ERROR

CRegistryA g_Reg;
CRegistryA *g_vpReg = &g_Reg;

//+---------------------------------------------------------------------------
//
//  Method:     CRegistryW::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CRegistryW::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hr = NOERROR;

    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::QueryInterface (%lx, %p)\n", this, riid, ppv));

    if ((riid == IID_IUnknown) || (riid == IID_IRegistryW))
    {
        *ppv = (void FAR *)this;
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::QueryInterface (%lx)[%p]\n", this, hr, *ppv));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRegistryW::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRegistryW::AddRef( void )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::AddRef\n", this));

    LONG lRet = _CRefs++;

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::AddRef (%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRegistryW::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRegistryW::Release( void )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::Release\n", this));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }
    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::Release (%ld)\n", this, lRet));
    return lRet;
}

// can this method be supported
STDMETHODIMP_(LONG) CRegistryW::ConnectRegistry (
     LPWSTR lpMachineName,
     HKEY hKey,
     IRegistryW **ppReg
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::ConnectRegistry\n", this));
    HKEY hKeyLoc = 0;

    LONG lRet;

    lRet  = RegConnectRegistryW(lpMachineName, hKey, &hKeyLoc);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::ConnectRegistry (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::CloseKey (
     HKEY hKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::CloseKey\n", this));
    LONG lRet = 0;
    lRet = RegCloseKey( hKey );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::CloseKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::CreateKey (
     HKEY hKey,
     LPCWSTR lpSubKey,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::CreateKey\n", this));
    LONG lRet = 0;

    lRet = RegCreateKeyW(hKey, lpSubKey, phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::CreateKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::CreateKeyEx (
     HKEY hKey,
     LPCWSTR lpSubKey,
     DWORD Reserved,
     LPWSTR lpClass,
     DWORD dwOptions,
     REGSAM samDesired,
     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
     PHKEY phkResult,
     LPDWORD lpdwDisposition
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::CreateKeyEx\n", this));
    LONG lRet = 0;
    lRet = RegCreateKeyExW (
             hKey,
             lpSubKey,
             Reserved,
             lpClass,
             dwOptions,
             samDesired,
             lpSecurityAttributes,
             phkResult,
             lpdwDisposition
            );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::CreateKeyEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::DeleteKey (
     HKEY hKey,
     LPCWSTR lpSubKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::DeleteKey\n", this));
    LONG lRet = 0;

    lRet = RegDeleteKeyW(hKey, lpSubKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::DeleteKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::DeleteValue (
     HKEY hKey,
     LPCWSTR lpValueName
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::DeleteValue\n", this));
    LONG lRet = 0;

    lRet = RegDeleteValueW(hKey, lpValueName);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::DeleteValue (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::EnumKey (
     HKEY hKey,
     DWORD dwIndex,
     LPWSTR lpName,
     DWORD cbName
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::EnumKey\n", this));
    LONG lRet = 0;

    lRet = RegEnumKeyW (hKey, dwIndex, lpName, cbName);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::EnumKey (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::EnumKeyEx (
     HKEY hKey,
     DWORD dwIndex,
     LPWSTR lpName,
     LPDWORD lpcbName,
     LPDWORD lpReserved,
     LPWSTR lpClass,
     LPDWORD lpcbClass,
     PFILETIME lpftLastWriteTime
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::EnumKeyEx\n", this));
    LONG lRet = 0;

    lRet = RegEnumKeyExW(hKey,dwIndex,lpName,lpcbName,lpReserved,
                 lpClass,lpcbClass,lpftLastWriteTime);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::EnumKeyEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::EnumValue (
     HKEY hKey,
     DWORD dwIndex,
     LPWSTR lpValueName,
     LPDWORD lpcbValueName,
     LPDWORD lpReserved,
     LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::EnumValue\n", this));
    LONG lRet = 0;
    lRet = RegEnumValueW(hKey,dwIndex,lpValueName,
                lpcbValueName,lpReserved,lpType,lpData,lpcbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::EnumValue (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::FlushKey (HKEY hKey)
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::FlushKey\n", this));
    LONG lRet = 0;

    lRet = RegFlushKey(hKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::FlushKey (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::GetKeySecurity (
     HKEY hKey,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR pSecurityDescriptor,
     LPDWORD lpcbSecurityDescriptor)
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::GetKeySecurity\n", this));
    LONG lRet = 0;

    lRet = RegGetKeySecurity (hKey,SecurityInformation, pSecurityDescriptor,lpcbSecurityDescriptor);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::GetKeySecurity (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::LoadKey (
     HKEY  hKey,
     LPCWSTR  lpSubKey,
     LPCWSTR  lpFile
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::LoadKey\n", this));
    LONG lRet = 0;

    lRet = RegLoadKeyW(hKey, lpSubKey, lpFile);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::LoadKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::NotifyChangeKeyValue (
     HKEY hKey,
     BOOL bWatchSubtree,
     DWORD dwNotifyFilter,
     HANDLE hEvent,
     BOOL fAsynchronus
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::NotifyChangeKeyValue\n", this));
    LONG lRet = 0;

    lRet = RegNotifyChangeKeyValue(hKey, bWatchSubtree, dwNotifyFilter, hEvent,fAsynchronus);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::NotifyChangeKeyValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::OpenKey (
     HKEY hKey,
     LPCWSTR lpSubKey,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::OpenKey\n", this));
    LONG lRet = 0;

    lRet =  RegOpenKeyW(hKey, lpSubKey, phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::OpenKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::OpenKeyEx (
     HKEY hKey,
     LPCWSTR lpSubKey,
     DWORD ulOptions,
     REGSAM samDesired,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::OpenKeyEx\n", this));
    LONG lRet = 0;

    lRet = RegOpenKeyExW(hKey,lpSubKey,ulOptions,samDesired,phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::OpenKeyEx (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryW::QueryInfoKey (
     HKEY    hKey,
     LPWSTR  lpClass,
     LPDWORD lpcbClass,
     LPDWORD lpReserved,
     LPDWORD lpcSubKeys,
     LPDWORD lpcbMaxSubKeyLen,
     LPDWORD lpcbMaxClassLen,
     LPDWORD lpcValues,
     LPDWORD lpcbMaxValueNameLen,
     LPDWORD lpcbMaxValueLen,
     LPDWORD lpcbSecurityDescriptor,
     PFILETIME lpftLastWriteTime
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::QueryInfoKey\n", this));
    LONG lRet = 0;

    lRet = RegQueryInfoKeyW (
         hKey,
         lpClass,
         lpcbClass,
         lpReserved,
         lpcSubKeys,
         lpcbMaxSubKeyLen,
         lpcbMaxClassLen,
         lpcValues,
         lpcbMaxValueNameLen,
         lpcbMaxValueLen,
         lpcbSecurityDescriptor,
         lpftLastWriteTime);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::QueryInfoKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::QueryValue (
     HKEY hKey,
     LPCWSTR lpSubKey,
     LPWSTR lpValue,
     LONG   *lpcbValue
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::QueryValue\n", this));
    LONG lRet = 0;

    lRet = RegQueryValueW(hKey,lpSubKey,lpValue,lpcbValue);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::QueryValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::QueryMultipleValues (
     HKEY hKey,
     PVALENTXW val_list,
     DWORD num_vals,
     LPWSTR lpValueBuf,
     LPDWORD ldwTotsize
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::QueryMultipleValues\n", this));
    LONG lRet = 0;

    lRet = RegQueryMultipleValuesW(hKey,(PVALENTW) val_list, num_vals, lpValueBuf, ldwTotsize);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::QueryMultipleValues (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::QueryValueEx (
     HKEY hKey,
     LPCWSTR lpValueName,
     LPDWORD lpReserved,
     LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::QueryValueEx\n", this));
    LONG lRet = 0;

    lRet = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::QueryValueEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::ReplaceKey (
     HKEY     hKey,
     LPCWSTR  lpSubKey,
     LPCWSTR  lpNewFile,
     LPCWSTR  lpOldFile
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::ReplaceKey\n", this));
    LONG lRet = 0;

    lRet = RegReplaceKeyW(hKey, lpSubKey, lpNewFile, lpOldFile);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::ReplaceKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::RestoreKey (
     HKEY hKey,
     LPCWSTR lpFile,
     DWORD   dwFlags
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::RestoreKey\n", this));
    LONG lRet = 0;

    lRet = RegRestoreKeyW(hKey, lpFile, dwFlags);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::RestoreKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::SaveKey (
     HKEY hKey,
     LPCWSTR lpFile,
     LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::SaveKey\n", this));
    LONG lRet = 0;

    lRet = RegSaveKeyW(hKey, lpFile, lpSecurityAttributes);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::SaveKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::SetKeySecurity (
     HKEY hKey,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::SetKeySecurity\n", this));
    LONG lRet = 0;

    lRet = RegSetKeySecurity(hKey, SecurityInformation, pSecurityDescriptor);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::SetKeySecurity (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::SetValue (
     HKEY hKey,
     LPCWSTR lpSubKey,
     DWORD dwType,
     LPCWSTR lpData,
     DWORD cbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::SetValue\n", this));
    LONG lRet = 0;

    lRet = RegSetValueW(hKey, lpSubKey, dwType,lpData, cbData );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::SetValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::SetValueEx (
     HKEY hKey,
     LPCWSTR lpValueName,
     DWORD Reserved,
     DWORD dwType,
     const BYTE* lpData,
     DWORD cbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::SetValueEx\n", this));
    LONG lRet = 0;

    lRet = RegSetValueExW(hKey, lpValueName, Reserved, dwType,lpData, cbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::SetValueEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryW::UnLoadKey (
     HKEY hKey,
     LPCWSTR lpSubKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryW::UnLoadKey\n", this));
    LONG lRet = 0;

    lRet = RegUnLoadKeyW(hKey, lpSubKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryW::UnLoadKey (%ld)\n", this, lRet));
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRegistryW::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CRegistryA::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hr = NOERROR;

    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::QueryInterface (%lx, %p)\n", this, riid, ppv));

    if ((riid == IID_IUnknown) || (riid == IID_IRegistryA))
    {
        *ppv = (void FAR *)this;
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::QueryInterface (%lx)[%p]\n", this, hr, *ppv));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRegistryA::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRegistryA::AddRef( void )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::AddRef\n", this));

    LONG lRet = _CRefs++;

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::AddRef (%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRegistryA::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRegistryA::Release( void )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::Release\n", this));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }
    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::Release (%ld)\n", this, lRet));
    return lRet;
}

// can this method be supported
STDMETHODIMP_(LONG) CRegistryA::ConnectRegistry (
     LPSTR lpMachineName,
     HKEY hKey,
     IRegistryA **ppReg
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::ConnectRegistry\n", this));
    HKEY hKeyLoc = 0;

    LONG lRet;

    lRet  = RegConnectRegistryA(lpMachineName, hKey, &hKeyLoc);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::ConnectRegistry (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::CloseKey (
     HKEY hKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::CloseKey\n", this));
    LONG lRet = 0;
    lRet = RegCloseKey( hKey );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::CloseKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::CreateKey (
     HKEY hKey,
     LPCSTR lpSubKey,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::CreateKey\n", this));
    LONG lRet = 0;

    lRet = RegCreateKeyA(hKey, lpSubKey, phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::CreateKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::CreateKeyEx (
     HKEY hKey,
     LPCSTR lpSubKey,
     DWORD Reserved,
     LPSTR lpClass,
     DWORD dwOptions,
     REGSAM samDesired,
     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
     PHKEY phkResult,
     LPDWORD lpdwDisposition
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::CreateKeyEx\n", this));
    LONG lRet = 0;
    lRet = RegCreateKeyExA (
             hKey,
             lpSubKey,
             Reserved,
             lpClass,
             dwOptions,
             samDesired,
             lpSecurityAttributes,
             phkResult,
             lpdwDisposition
            );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::CreateKeyEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::DeleteKey (
     HKEY hKey,
     LPCSTR lpSubKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::DeleteKey\n", this));
    LONG lRet = 0;

    lRet = RegDeleteKeyA(hKey, lpSubKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::DeleteKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::DeleteValue (
     HKEY hKey,
     LPCSTR lpValueName
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::DeleteValue\n", this));
    LONG lRet = 0;

    lRet = RegDeleteValueA(hKey, lpValueName);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::DeleteValue (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::EnumKey (
     HKEY hKey,
     DWORD dwIndex,
     LPSTR lpName,
     DWORD cbName
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::EnumKey\n", this));
    LONG lRet = 0;

    lRet = RegEnumKeyA (hKey, dwIndex, lpName, cbName);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::EnumKey (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::EnumKeyEx (
     HKEY hKey,
     DWORD dwIndex,
     LPSTR lpName,
     LPDWORD lpcbName,
     LPDWORD lpReserved,
     LPSTR lpClass,
     LPDWORD lpcbClass,
     PFILETIME lpftLastWriteTime
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::EnumKeyEx\n", this));
    LONG lRet = 0;

    lRet = RegEnumKeyExA(hKey,dwIndex,lpName,lpcbName,lpReserved,
                 lpClass,lpcbClass,lpftLastWriteTime);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::EnumKeyEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::EnumValue (
     HKEY hKey,
     DWORD dwIndex,
     LPSTR lpValueName,
     LPDWORD lpcbValueName,
     LPDWORD lpReserved,
     LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::EnumValue\n", this));
    LONG lRet = 0;
    lRet = RegEnumValueA(hKey,dwIndex,lpValueName,
                lpcbValueName,lpReserved,lpType,lpData,lpcbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::EnumValue (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::FlushKey (HKEY hKey)
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::FlushKey\n", this));
    LONG lRet = 0;

    lRet = RegFlushKey(hKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::FlushKey (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::GetKeySecurity (
     HKEY hKey,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR pSecurityDescriptor,
     LPDWORD lpcbSecurityDescriptor)
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::GetKeySecurity\n", this));
    LONG lRet = 0;

    lRet = RegGetKeySecurity (hKey,SecurityInformation, pSecurityDescriptor,lpcbSecurityDescriptor);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::GetKeySecurity (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::LoadKey (
     HKEY  hKey,
     LPCSTR  lpSubKey,
     LPCSTR  lpFile
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::LoadKey\n", this));
    LONG lRet = 0;

    lRet = RegLoadKeyA(hKey, lpSubKey, lpFile);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::LoadKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::NotifyChangeKeyValue (
     HKEY hKey,
     BOOL bWatchSubtree,
     DWORD dwNotifyFilter,
     HANDLE hEvent,
     BOOL fAsynchronus
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::NotifyChangeKeyValue\n", this));
    LONG lRet = 0;

    lRet = RegNotifyChangeKeyValue(hKey, bWatchSubtree, dwNotifyFilter, hEvent,fAsynchronus);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::NotifyChangeKeyValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::OpenKey (
     HKEY hKey,
     LPCSTR lpSubKey,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::OpenKey (lpSubKey:%s)\n", this,lpSubKey));
    LONG lRet = 0;

    lRet =  RegOpenKeyA(hKey, lpSubKey, phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::OpenKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::OpenKeyEx (
     HKEY hKey,
     LPCSTR lpSubKey,
     DWORD ulOptions,
     REGSAM samDesired,
     PHKEY phkResult
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::OpenKeyEx\n", this));
    LONG lRet = 0;

    lRet = RegOpenKeyExA(hKey,lpSubKey,ulOptions,samDesired,phkResult);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::OpenKeyEx (%ld)\n", this, lRet));
    return lRet;
}


STDMETHODIMP_(LONG) CRegistryA::QueryInfoKey (
     HKEY    hKey,
     LPSTR  lpClass,
     LPDWORD lpcbClass,
     LPDWORD lpReserved,
     LPDWORD lpcSubKeys,
     LPDWORD lpcbMaxSubKeyLen,
     LPDWORD lpcbMaxClassLen,
     LPDWORD lpcValues,
     LPDWORD lpcbMaxValueNameLen,
     LPDWORD lpcbMaxValueLen,
     LPDWORD lpcbSecurityDescriptor,
     PFILETIME lpftLastWriteTime
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::QueryInfoKey\n", this));
    LONG lRet = 0;

    lRet = RegQueryInfoKeyA (
         hKey,
         lpClass,
         lpcbClass,
         lpReserved,
         lpcSubKeys,
         lpcbMaxSubKeyLen,
         lpcbMaxClassLen,
         lpcValues,
         lpcbMaxValueNameLen,
         lpcbMaxValueLen,
         lpcbSecurityDescriptor,
         lpftLastWriteTime);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::QueryInfoKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::QueryValue (
     HKEY hKey,
     LPCSTR lpSubKey,
     LPSTR lpValue,
     LONG   *lpcbValue
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::QueryValue (szValue:%s)\n", this,lpSubKey));
    LONG lRet = 0;

    lRet = RegQueryValueA(hKey,lpSubKey,lpValue,lpcbValue);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::QueryValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::QueryMultipleValues (
     HKEY hKey,
     PVALENTXA val_list,
     DWORD num_vals,
     LPSTR lpValueBuf,
     LPDWORD ldwTotsize
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::QueryMultipleValues\n", this));
    LONG lRet = 0;

    lRet = RegQueryMultipleValuesA(hKey,(PVALENTA) val_list, num_vals, lpValueBuf, ldwTotsize);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::QueryMultipleValues (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::QueryValueEx (
     HKEY hKey,
     LPCSTR lpValueName,
     LPDWORD lpReserved,
     LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::QueryValueEx\n", this));
    LONG lRet = 0;

    lRet = RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::QueryValueEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::ReplaceKey (
     HKEY     hKey,
     LPCSTR  lpSubKey,
     LPCSTR  lpNewFile,
     LPCSTR  lpOldFile
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::ReplaceKey\n", this));
    LONG lRet = 0;

    lRet = RegReplaceKeyA(hKey, lpSubKey, lpNewFile, lpOldFile);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::ReplaceKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::RestoreKey (
     HKEY hKey,
     LPCSTR lpFile,
     DWORD   dwFlags
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::RestoreKey\n", this));
    LONG lRet = 0;

    lRet = RegRestoreKeyA(hKey, lpFile, dwFlags);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::RestoreKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::SaveKey (
     HKEY hKey,
     LPCSTR lpFile,
     LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::SaveKey\n", this));
    LONG lRet = 0;

    lRet = RegSaveKeyA(hKey, lpFile, lpSecurityAttributes);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::SaveKey (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::SetKeySecurity (
     HKEY hKey,
     SECURITY_INFORMATION SecurityInformation,
     PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::SetKeySecurity\n", this));
    LONG lRet = 0;

    lRet = RegSetKeySecurity(hKey, SecurityInformation, pSecurityDescriptor);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::SetKeySecurity (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::SetValue (
     HKEY hKey,
     LPCSTR lpSubKey,
     DWORD dwType,
     LPCSTR lpData,
     DWORD cbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::SetValue\n", this));
    LONG lRet = 0;

    lRet = RegSetValueA(hKey, lpSubKey, dwType,lpData, cbData );

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::SetValue (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::SetValueEx (
     HKEY hKey,
     LPCSTR lpValueName,
     DWORD Reserved,
     DWORD dwType,
     const BYTE* lpData,
     DWORD cbData
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::SetValueEx\n", this));
    LONG lRet = 0;

    lRet = RegSetValueExA(hKey, lpValueName, Reserved, dwType,lpData, cbData);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::SetValueEx (%ld)\n", this, lRet));
    return lRet;
}

STDMETHODIMP_(LONG) CRegistryA::UnLoadKey (
     HKEY hKey,
     LPCSTR lpSubKey
    )
{
    UrlMkDebugOut((DEB_REG, "%p _IN CRegistryA::UnLoadKey\n", this));
    LONG lRet = 0;

    lRet = RegUnLoadKeyA(hKey, lpSubKey);

    UrlMkDebugOut((DEB_REG, "%p OUT CRegistryA::UnLoadKey (%ld)\n", this, lRet));
    return lRet;
}




