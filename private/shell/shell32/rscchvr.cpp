#include "shellprv.h"
#pragma  hdrstop

#include "rscchvr.h"

void CRSCacheVersion::_RSCVUpdateVersionOnCacheRead()
{
    DWORD dwVersion;

    if (RSGetDWORDValue(_pszSubKey, TEXT("Version"), &dwVersion))
    {
        _dwVersion = dwVersion;
    }
    else
    {
        _dwVersion = 0;
        RSSetDWORDValue(_pszSubKey, TEXT("Version"), _dwVersion);
    }
}

void CRSCacheVersion::_RSCVUpdateVersionOnCacheWrite()
{
    DWORD dwVersion;

    if (RSGetDWORDValue(_pszSubKey, TEXT("Version"), &dwVersion))
    {
        _dwVersion = dwVersion + 1;
    }
    else
    {
        _dwVersion = 0;
    }

    RSSetDWORDValue(_pszSubKey, TEXT("Version"), _dwVersion);
}

BOOL CRSCacheVersion::_RSCVIsValidVersion()
{
    DWORD dwVersion;
    BOOL fRet = FALSE;

    if (RSGetDWORDValue(_pszSubKey, TEXT("Version"), &dwVersion))
    {
        if (_dwVersion == dwVersion)
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

void CRSCacheVersion::_RSCVIncrementRegVersion()
{
    DWORD dwVersion;

    _RSCVDeleteRegCache();

    if (RSGetDWORDValue(_pszSubKey, TEXT("Version"), &dwVersion))
    {
        ++dwVersion;

        RSSetDWORDValue(_pszSubKey, TEXT("Version"), dwVersion);
    }
}

void CRSCacheVersion::RSCVInitSubKey(LPCTSTR pszSubKey)
{
    _pszSubKey = pszSubKey;
}

LPCTSTR CRSCacheVersion::RSCVGetSubKey()
{
    return _pszSubKey;
}