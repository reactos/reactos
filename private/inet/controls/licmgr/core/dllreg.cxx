//+----------------------------------------------------------------------------
//  File:       dllreg.cxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <core.hxx>


// Prototypes -----------------------------------------------------------------
static const TCHAR * DeleteSubkeys(HKEY hkeyParent, const TCHAR * pszKey);
static const TCHAR * RegisterKey(HKEY hkeyParent, const TCHAR * pszKey);


//+----------------------------------------------------------------------------
//  Function:   DllRegisterServer
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDAPI
DllRegisterServer()
{
    const TCHAR **  ppszKey;
    const TCHAR *   pszKey;
    HKEY            hkey;
    LONG            lError;
    HRESULT         hr;

    hr = EnsureThreadState();
    if (hr)
        goto Cleanup;

    for (ppszKey=g_aszKeys; *ppszKey; ppszKey++)
    {
        pszKey = *ppszKey;

        lError = ::RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                pszKey, 0, KEY_ALL_ACCESS,
                                &hkey);
        if (lError != ERROR_SUCCESS)
        {
            hr = E_FAIL;
            break;
        }

        pszKey += _tcslen(pszKey) + 1;
        Assert(*pszKey);

        if (!RegisterKey(hkey, pszKey))
        {
            hr = E_FAIL;
            break;
        }

#ifdef _DEBUG
        Verify(::RegCloseKey(hkey) == ERROR_SUCCESS);
#else
        ::RegCloseKey(hkey);
#endif
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDAPI
DllUnregisterServer()
{
    const TCHAR **  ppszKey;
    const TCHAR *   pszKey;
    HKEY            hkey;
    LONG            lError;
    HRESULT         hr;

    hr = EnsureThreadState();
    if (hr)
        goto Cleanup;

    for (ppszKey=g_aszKeys; *ppszKey; ppszKey++)
    {
        pszKey = *ppszKey;

        lError = ::RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                pszKey, 0, KEY_ALL_ACCESS,
                                &hkey);
        if (lError != ERROR_SUCCESS)
        {
            hr = E_FAIL;
            break;
        }

        pszKey += _tcslen(pszKey) + 1;
        Assert(*pszKey);

        if (!DeleteSubkeys(hkey, pszKey) ||
            ::RegDeleteKey(hkey, pszKey) != ERROR_SUCCESS)
        {
            hr = E_FAIL;
        }
#ifdef _DEBUG
        Verify(::RegCloseKey(hkey) == ERROR_SUCCESS);
#else
        ::RegCloseKey(hkey);
#endif
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   DeleteSubkeys
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
const TCHAR *
DeleteSubkeys(
    HKEY            hkeyParent,
    const TCHAR *   pszKey)
{
    const TCHAR *   psz;
    HKEY            hkey = NULL;
    LONG            lError;

    lError = ::RegOpenKeyEx(hkeyParent, pszKey, 0, KEY_ALL_ACCESS, &hkey);
    if (lError != ERROR_SUCCESS)
        goto Error;

    pszKey += _tcslen(pszKey) + 1;

    while (*pszKey)
    {
        switch (*pszKey)
        {
        case chDEFAULT_SECTION:
            pszKey++;
            pszKey += _tcslen(pszKey) + 1;
            break;

        case chVALUES_SECTION:
            pszKey++;
            while (*pszKey)
            {
                pszKey += _tcslen(pszKey) + 1;
                pszKey += _tcslen(pszKey) + 1;
            }
            pszKey++;
            break;

        case chSUBKEY_SECTION:
            pszKey++;
            psz = pszKey;
            pszKey = DeleteSubkeys(hkey, pszKey);
            if (!pszKey)
                goto Error;
            lError = ::RegDeleteKey(hkey, psz);
            if (lError != ERROR_SUCCESS)
                goto Error;
            break;

#ifdef _DEBUG
        default:
            AssertF("Invalid section in registry key data");
#endif
        }
    }

    pszKey++;

Cleanup:
    if (hkey)
    {
#ifdef _DEBUG
        Verify(::RegCloseKey(hkey) == ERROR_SUCCESS);
#else
        ::RegCloseKey(hkey);
#endif
    }
    return pszKey;

Error:
    pszKey = NULL;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//  Function:   RegisterKey
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
const TCHAR *
RegisterKey(
    HKEY            hkeyParent,
    const TCHAR *   pszKey)
{
    const TCHAR *   psz;
    HKEY            hkey = NULL;
    DWORD           dwDisposition;
    LONG            lError;

    lError = ::RegCreateKeyEx(hkeyParent, pszKey, 0, _T(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL,
                            &hkey, &dwDisposition);
    if (lError != ERROR_SUCCESS)
        goto Error;

    pszKey += _tcslen(pszKey) + 1;

    while (*pszKey)
    {
        switch (*pszKey)
        {
        case chDEFAULT_SECTION:
            pszKey++;
            if (!::lstrcmpi(pszKey, szMODULE_PATH))
            {
                TCHAR   szModule[MAX_PATH+1];

                Verify(::GetModuleFileName((HMODULE)g_hinst, szModule, ARRAY_SIZE(szModule)));
                lError = ::RegSetValueEx(hkey, NULL, 0,
                                        REG_SZ,
                                        (const BYTE *)szModule,
                                        sizeof(TCHAR) * (_tcslen(szModule) + 1));
            }
            else
            {
                lError = ::RegSetValueEx(hkey, NULL, 0,
                                        REG_SZ,
                                        (const BYTE *)pszKey,
                                        sizeof(TCHAR) * (_tcslen(pszKey) + 1));
            }
            if (lError != ERROR_SUCCESS)
                goto Error;
            pszKey += _tcslen(pszKey) + 1;
            break;

        case chVALUES_SECTION:
            pszKey++;
            while (*pszKey)
            {
                psz = pszKey;
                pszKey += _tcslen(pszKey) + 1;

                lError = ::RegSetValueEx(hkey, psz, 0,
                                        REG_SZ,
                                        (const BYTE *)pszKey,
                                        sizeof(TCHAR) * (_tcslen(pszKey) + 1));
                if (lError != ERROR_SUCCESS)
                    goto Error;

                pszKey += _tcslen(pszKey) + 1;
            }
            pszKey++;
            break;

        case chSUBKEY_SECTION:
            pszKey++;
            pszKey = RegisterKey(hkey, pszKey);
            if (!pszKey)
                goto Error;
            break;

#ifdef _DEBUG
        default:
            AssertF("Invalid section in registry key data");
#endif
        }
    }

    pszKey++;

Cleanup:
    if (hkey)
    {
#ifdef _DEBUG
        Verify(::RegCloseKey(hkey) == ERROR_SUCCESS);
#else
        ::RegCloseKey(hkey);
#endif
    }
    return pszKey;

Error:
    pszKey = NULL;
    goto Cleanup;
}
