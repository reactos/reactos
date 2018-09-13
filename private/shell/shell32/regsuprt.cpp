#include "shellprv.h"
#pragma  hdrstop

#include "regsuprt.h"

#ifdef DEBUG
UINT CRegSupport::_cRefHKEY = 0;
UINT CRegSupport::_cRefExternalHKEY = 0;
#endif

void CRegSupport::RSInitRoot(HKEY hkey, LPCTSTR pszSubKey1, LPCTSTR pszSubKey2,
        DWORD dwRootOptions, DWORD dwDefaultOptions)
{
    _dwRootOptions = dwRootOptions; 
    _dwDefaultOptions = dwDefaultOptions;
    
    _hkeyInit = hkey;

    _InitSetRoot(pszSubKey1, pszSubKey2);

#ifdef DEBUG
    ASSERT(!_fInited);

    _fInited = TRUE;
#endif
}

BOOL CRegSupport::RSSubKeyExist(LPCTSTR pszSubKey)
{
    BOOL fRet = FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        fRet = TRUE;
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

BOOL CRegSupport::RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    BOOL fRet = FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        fRet = (RegQueryValueEx(hkeySubKey, pszValueName, 0, NULL, NULL, NULL) ==
            ERROR_SUCCESS);

        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

BOOL CRegSupport::RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    BOOL fRet = FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == RegDeleteValue(hkeySubKey, pszValueName))
        {
            fRet = TRUE;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

BOOL CRegSupport::RSDeleteKey()
{
    TCHAR szRoot[MAX_ROOT];

    return (ERROR_SUCCESS == SHRegDeleteKey(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot))));
}

BOOL CRegSupport::RSDeleteSubKey(LPCTSTR pszSubKey)
{
    BOOL fRet = FALSE;

    _EnterCSKeyRoot();

    HKEY hkey = _GetRootKey(FALSE);

    if (hkey)
    {
        if (ERROR_SUCCESS == SHRegDeleteKey(hkey, pszSubKey))
        {
            fRet = TRUE;
        }
        _CloseRegSubKey(hkey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

BOOL CRegSupport::RSSetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               PBYTE pb, DWORD cb,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, pb, cb, REG_BINARY, dwOptions);
}

BOOL CRegSupport::RSSetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               LPCTSTR pszValue,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, (PBYTE)pszValue,
        (lstrlen(pszValue) + 1) * sizeof(TCHAR), REG_SZ, dwOptions);
}

BOOL CRegSupport::RSSetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               DWORD dwValue,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, (PBYTE)&dwValue, 
        sizeof(DWORD), REG_DWORD, dwOptions);
}

BOOL CRegSupport::RSGetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD* pcb)
{
    return _GetGeneric(pszSubKey, pszValueName, pb, pcb);
}

BOOL CRegSupport::RSGetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                LPTSTR pszValue, DWORD* pcchValue)
{
    DWORD cbData = *pcchValue * sizeof(TCHAR);

    return _GetGeneric(pszSubKey, pszValueName, (PBYTE)pszValue, &cbData);
}

BOOL CRegSupport::RSGetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, DWORD* pdwValue)
{
    DWORD cbData = sizeof(DWORD);

    return _GetGeneric(pszSubKey, pszValueName, (PBYTE)pdwValue, &cbData);
}

BOOL CRegSupport::_SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD cb, DWORD dwType,
                                DWORD dwOptions)
{
    BOOL fRet = FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, TRUE, dwOptions);
    else
        hkeySubKey = _GetRootKey(TRUE, dwOptions);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkeySubKey, pszValueName, 0, 
            dwType, pb, cb))
        {
            fRet = TRUE;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

BOOL CRegSupport::_GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               PBYTE pb, DWORD* pcb)
{
    BOOL fRet = FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == SHQueryValueEx(hkeySubKey, pszValueName, 0, 
            NULL, pb, pcb))
        {
            fRet = TRUE;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return fRet;
}

HKEY CRegSupport::RSDuplicateRootKey()
{
    RIP(_fInited);
#ifdef DEBUG
    // we need to decrement here since it will be icnremented inside this fct
    // and the key will not be close by this object
    --_cRefHKEY;
    ++_cRefExternalHKEY;
#endif
    TCHAR szRoot[MAX_ROOT];

    return _RegCreateKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)), _dwRootOptions);
}

HKEY CRegSupport::_GetRootKey(BOOL fCreate, DWORD dwOptions)
{
    RIP(_fInited);

    HKEY hkey;
    TCHAR szRoot[MAX_ROOT];

    if (REG_OPTION_INVALID == dwOptions)
        dwOptions = _dwRootOptions;

    if (fCreate)
        hkey = _RegCreateKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)), dwOptions);
    else
        hkey = _RegOpenKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)));

    return hkey;
}

void CRegSupport::_CloseRegSubKey(HKEY hkeySubKey)
{
    RegCloseKey(hkeySubKey);

#ifdef DEBUG
    --_cRefHKEY;
#endif
}

// Always need to be called from within the _csRootKey critical section (when critical section
// stuff is enabled)

HKEY CRegSupport::_GetSubKey(LPCTSTR pszSubKey, BOOL fCreate, DWORD dwOptions)
{
    HKEY hkey = NULL;

    HKEY hRootKey = _GetRootKey(fCreate, dwOptions);

    if (REG_OPTION_INVALID == dwOptions)
        dwOptions = _dwDefaultOptions;

    if (hRootKey)
    {
        if (fCreate)
            hkey = _RegCreateKeyExHelper(hRootKey, pszSubKey, dwOptions);
        else
            hkey = _RegOpenKeyExHelper(hRootKey, pszSubKey);

        _CloseRegSubKey(hRootKey);
    }

    return hkey;
}

//static
HKEY CRegSupport::_RegCreateKeyExHelper(HKEY hkey, LPCTSTR pszSubKey, DWORD dwOptions)
{
    HKEY hkeyTmp;
    DWORD dwDisp;

    ASSERT(REG_OPTION_INVALID != dwOptions);

    if (ERROR_SUCCESS != RegCreateKeyEx(hkey, pszSubKey, 0, NULL, 
        dwOptions, MAXIMUM_ALLOWED, NULL, &hkeyTmp, &dwDisp))
    {
        hkeyTmp = NULL;
    }
#ifdef DEBUG
    else
    {
        ++_cRefHKEY;
    }
#endif

    return hkeyTmp;
}

//static
HKEY CRegSupport::_RegOpenKeyExHelper(HKEY hkey, LPCTSTR pszSubKey)
{
    HKEY hkeyTmp;

    if (ERROR_SUCCESS != RegOpenKeyEx(hkey, pszSubKey, 0,
        MAXIMUM_ALLOWED, &hkeyTmp))
    {
        hkeyTmp = NULL;
    }
#ifdef DEBUG
    else
    {
        ++_cRefHKEY;
    }
#endif

    return hkeyTmp;
}

BOOL CRegSupport::_InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2)
{
    _pszSubKey1 = pszSubKey1;
    _pszSubKey2 = pszSubKey2;

    return TRUE;
}

LPCTSTR CRegSupport::_GetRoot(LPTSTR pszRoot, DWORD cchRoot)
{
    ASSERT(cchRoot > 0);

    lstrcpyn(pszRoot, _pszSubKey1, cchRoot);

    if (_pszSubKey2)
    {
        lstrcatn(pszRoot, TEXT("\\"), cchRoot);
        lstrcatn(pszRoot, _pszSubKey2, cchRoot);
    }

    return pszRoot;
}

void CRegSupport::_InitCSKeyRoot()
{
    ASSERT(!_fcsKeyRoot);

    _fcsKeyRoot = TRUE;
    InitializeCriticalSection(&_csKeyRoot);
}

void CRegSupport::_EnterCSKeyRoot()
{
    if (_fcsKeyRoot)
    {
        EnterCriticalSection(&_csKeyRoot);
    }
}

void CRegSupport::_LeaveCSKeyRoot()
{
    if (_fcsKeyRoot)
    {
        LeaveCriticalSection(&_csKeyRoot);
    }
}

CRegSupport::CRegSupport()
{}

CRegSupport::~CRegSupport()
{}
