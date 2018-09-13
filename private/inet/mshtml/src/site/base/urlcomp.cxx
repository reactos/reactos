//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       urlcomp.cxx
//
//  Contents:   URL compatibility code
//
//              Provides compatibility bits for specific URLs
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include "urlcomp.hxx"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

static TCHAR achUrlCompatRootKey[] = _T("Software\\Microsoft\\Internet Explorer\\URL Compatibility");
static TCHAR achCompatFlags[] =      _T("Compatibility Flags");
static TCHAR achIcwKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE");
static TCHAR szIcwPath[MAX_PATH];
extern TCHAR szOurVersion[];

static CPtrBagCi<DWORD> g_bCompatUrls;

static DWORD g_dwDefaultUrlCompatFlags;

//+---------------------------------------------------------------------------
//
//  Function:   ShouldWeRegisterUrlCompatibilityTable
//
//  Synopsis:   Determine whether we should write our compatibility table,
//              as recorded in selfreg.inf, to the registry.
//  
//              We do if the table is not in the registry at all, or if
//              our version of the table is more recent than that in the
//              registry.
//
//              NOTE: szOurVersion comes from clstab.cxx
//
//  Returns:    TRUE - yes, write the table please.
//
//----------------------------------------------------------------------------

BOOL 
ShouldWeRegisterUrlCompatibilityTable()
{
    BOOL fOurRet = TRUE;
    LRESULT lr;
    HKEY hkeyRoot = NULL;
    DWORD dwSize, dwType;
    TCHAR szVersion[10];

    lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, achUrlCompatRootKey, 0, KEY_READ, &hkeyRoot);
    if (lr != ERROR_SUCCESS)
        return(TRUE);

    dwSize = sizeof(szVersion);
    lr = RegQueryValueEx(hkeyRoot, _T("Version"), NULL, &dwType, (BYTE*)szVersion, &dwSize);
    if (lr == ERROR_SUCCESS)
        fOurRet = (_tcscmp(szVersion, szOurVersion) < 0);

    RegCloseKey(hkeyRoot);

    return(fOurRet);
}

void AppendIcwPath(TCHAR * pch)
{
    TCHAR ach[MAX_PATH];

    _tcscpy(ach, szIcwPath);
    _tcscat(ach, pch + 1);  // ignoring the leading '~'
    _tcscpy(pch, ach);
}
//+---------------------------------------------------------------------------
//
//  Function:   LegalKeyCharFromChar
//
//  Synopsis:   Some chars are not legal in a registry key, so replace them
//              with legal chars.
//
//  Returns:    TCHAR
//
//----------------------------------------------------------------------------

inline TCHAR
LegalKeyCharFromChar(TCHAR ch)
{
    if (ch <= _T(' '))
        return _T('!');

    if (ch >= 127)
        return _T('~');

    if (ch == _T('\\'))
        return _T('/');
        
    if (ch == _T('?') || ch == _T('*'))
        return _T('_');

    return ch;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupUrl
//
//  Synopsis:   Remove \'s etc from the URL (by converting to / etc)
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void CleanupUrl(TCHAR * pchTo, TCHAR * pch, ULONG cch)
{
    cch--;
    while (*pch && cch)
    {
        *pchTo++ = LegalKeyCharFromChar(*pch++);
        cch--;
    }
    *pchTo = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitUrlCompatTable
//
//  Synopsis:   Reads URL compat bits from the registry and puts them in
//              a hash table for fast lookup.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
InitUrlCompatTable()
{
    HKEY hkeyRoot = NULL;
    HKEY hkey = NULL;
    ULONG iKey;
    TCHAR ach[pdlUrlLen];
    ULONG cch;
    ULONG cb;
    DWORD dwType;
    DWORD dwFlags;
    HRESULT hr = S_OK;
    LRESULT lr;

    lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, achIcwKey, 0, KEY_READ, &hkeyRoot);
    if (lr != ERROR_SUCCESS)
        return S_OK;

    cb = ARRAY_SIZE(szIcwPath);
    lr = RegQueryValueEx(hkeyRoot, _T("Path"), NULL, &dwType, (BYTE*) szIcwPath, &cb);
    if (lr != ERROR_SUCCESS)
        goto Win32Error;

    lr = RegCloseKey(hkeyRoot);
    if (lr != ERROR_SUCCESS)
        goto Win32Error;

    // replace drive letter with '_' and remove the trailing ';'
    if (*szIcwPath)
    {
        szIcwPath[_tcslen(szIcwPath) - 1] = 0;
        GetShortPathName(szIcwPath, szIcwPath, ARRAY_SIZE(szIcwPath));
        szIcwPath[0]=_T('_');
    }

    CleanupUrl(szIcwPath, szIcwPath, ARRAY_SIZE(szIcwPath));

    // Open root key
    lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, achUrlCompatRootKey, 0, KEY_READ, &hkeyRoot);
    if (lr != ERROR_SUCCESS)
        goto Win32Error;

    // Read each table entry
    for (iKey = 0; ; iKey++)
    {
        cch = ARRAY_SIZE(ach);
        lr = RegEnumKeyEx(hkeyRoot, iKey, ach, &cch, NULL, NULL, NULL, NULL);
        if (lr != ERROR_SUCCESS )
            break;
            
        ach[cch] = _T('\0');

        lr = RegOpenKeyEx(hkeyRoot, ach, 0, KEY_READ, &hkey);
        if (lr != ERROR_SUCCESS)
            continue;

        cb = sizeof(dwFlags);
        lr = RegQueryValueEx(hkey, achCompatFlags, NULL, &dwType, (BYTE*)&dwFlags, &cb);
        
        if (lr == ERROR_SUCCESS && dwType == REG_DWORD && cb == sizeof(dwFlags))
        {
            if (ach[0] == _T('~'))
                AppendIcwPath(ach);

            hr = THR(g_bCompatUrls.SetCi(ach, dwFlags));
            if (hr)
                goto Cleanup;
        }

        lr = RegCloseKey(hkey);
        if (lr != ERROR_SUCCESS)
            goto Win32Error;
            
        hkey = NULL;
    }
    
    g_dwDefaultUrlCompatFlags = g_bCompatUrls.GetCi(_T("Default"));
    
Cleanup:
    if (hkey)
        RegCloseKey(hkey);
    if (hkeyRoot)
        RegCloseKey(hkeyRoot);

    RRETURN(hr);

Win32Error:
    hr = HRESULT_FROM_WIN32(lr);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   CompatBitsFromUrl
//
//  Synopsis:   Looks up URL in a case-insensitive hash table and returns
//              compat bits.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CompatBitsFromUrl(TCHAR *pchUrl, DWORD *dwUrlCompat)
{
    TCHAR *pch;
    TCHAR ach[pdlUrlLen];
    ULONG cch = ARRAY_SIZE(ach);
    DWORD dwFlags;
    HRESULT hr = S_OK;

    // File URLs become paths

    if (GetUrlScheme(pchUrl) == URL_SCHEME_FILE)
    {
        hr = THR(PathCreateFromUrl(pchUrl, ach, &cch, 0));
        if (hr)
            goto Cleanup;

        GetShortPathName(ach, ach, ARRAY_SIZE(ach));

        // replace drive letter with '_'
        if (*ach)
        {
            ach[0]=_T('_');
        }

        pch = ach;
        cch = ARRAY_SIZE(ach);
    }
    else
    {
        pch = pchUrl;
    }

    CleanupUrl(ach, pch, cch);

    dwFlags = g_bCompatUrls.GetCi(ach);

    *dwUrlCompat = dwFlags ? dwFlags : g_dwDefaultUrlCompatFlags;

Cleanup:
    RRETURN(hr);
}


