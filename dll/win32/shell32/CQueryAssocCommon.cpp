/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IQueryAssociations common (pre-Vista and Vista+)
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <shlwapi_undoc.h> // AssocCreateElement
#include "evalcmd/elements.h" // ASSOCQUERY_*
#include <evalcmd.h> // SHELL32_CAssocElement_Version
#include "CQueryAssocCommon.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HRESULT AssocCreateElementInternal(REFCLSID rclsid, IQuerySource *pQS, REFIID riid, PVOID *ppv)
{
    HRESULT hr;
    C_ASSERT(&IID_IObjectWithQuerySource == &IID_IObjectWithQuerySourceOld);
    CComPtr<IObjectWithQuerySource> pOWQS;
    if (FAILED(hr = SH32_AssocCreateElement(rclsid, IID_PPV_ARG(IObjectWithQuerySource, &pOWQS))))
        return hr;
    return (SUCCEEDED(hr = pOWQS->SetSource(pQS))) ? pOWQS->QueryInterface(riid, ppv) : hr;
}

HRESULT AssocCreateElementOnKeyInternal(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                        _In_ REFIID riid, _Outptr_ PVOID *ppv, _In_ BYTE Major)
{
    CComPtr<IUnknown> pQSO;
    REFIID riidQS = Major > 5 ? IID_IQuerySource : IID_IQuerySourceOld;
    HRESULT hr = QuerySourceCreateFromKey(hKey, pszSubKey, false, riidQS, (void**)&pQSO);
    if (FAILED(hr))
        return hr;
    return AssocCreateElementInternal(rclsid, (IQuerySource*)static_cast<IUnknown*>(pQSO), riid, ppv);
}

HRESULT SH32_AssocCreateElementOnKey(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                     _In_ REFIID riid, _Outptr_ PVOID *ppv)
{
    if (GetProcessOsVersion() >= _WIN32_WINNT_VISTA)
    {
        HRESULT hr = AssocCreateElementOnKeyInternal(rclsid, hKey, pszSubKey, riid, ppv, 6);
        if (SUCCEEDED(hr))
            return hr;
    }

    return AssocCreateElementOnKeyInternal(rclsid, hKey, pszSubKey, riid, ppv, 5);
}

HRESULT SH32_QueryAssocElementString(_In_ REFCLSID rclsid, _In_ UINT Query, _In_ HKEY hKey,
                                     _In_opt_ PCWSTR pszSubKey, _In_ PCWSTR pszExtra, _Out_ PWSTR *ppszValue)
{
    CComPtr<IAssociationElementOld> pAE;
    REFIID riid = IID_IAssociationElementOld;
    HRESULT hr = SH32_AssocCreateElementOnKey(rclsid, hKey, pszSubKey, riid, (void**)&pAE);
    if (FAILED(hr))
        return hr;
    return pAE->QueryString(Query, pszExtra, ppszValue);
}

EXTERN_C HRESULT SHELL32_AssocGetFSDirectoryDescription(PWSTR Buf, UINT cchBuf)
{
    static WCHAR cache[33] = {};
    if (!*cache)
        LoadStringW(shell32_hInstance, IDS_DIRECTORY, cache, _countof(cache));
    return StringCchCopyW(Buf, cchBuf, cache);
}

HRESULT GetExtensionDefaultDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf)
{
    static WCHAR fmt[33] = {};
    if (!*fmt)
        LoadStringW(shell32_hInstance, IDS_ANY_FILE, fmt, _countof(fmt));
    return StringCchPrintfW(Buf, cchBuf, fmt, DotExt);
}

HRESULT SHELL32_AssocGetExtensionDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf)
{
    HRESULT hr;
    if (!DotExt[0] || (!DotExt[1] && DotExt[0] == '.'))
    {
        if (SUCCEEDED(hr = GetExtensionDefaultDescription(L"", Buf, cchBuf)))
            StrTrimW(Buf, L" -"); // Remove the empty %s so we are left with "File"
        return hr;
    }
    HKEY hKey;
    if (SUCCEEDED(hr = HCR_GetProgIdKeyOfExtension(DotExt, &hKey, TRUE)))
    {
        DWORD err = RegLoadMUIStringW(hKey, L"FriendlyTypeName", Buf, cchBuf, NULL, 0, NULL);
        if (err && hr == S_OK) // ProgId default value fallback (but not if we only have a .ext key)
        {
            DWORD cb = cchBuf * sizeof(*Buf);
            err = RegGetValueW(hKey, NULL, NULL, RRF_RT_REG_SZ, NULL, Buf, &cb);
        }
        RegCloseKey(hKey);
        if (!err)
            return err;
    }
    // No information in the registry, default to "UPPERCASEEXT File"
    WCHAR ext[MAX_PATH + 33];
    if (LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, ++DotExt, -1, ext, _countof(ext)))
        DotExt = ext;
    return GetExtensionDefaultDescription(DotExt, Buf, cchBuf);
}

EXTERN_C HRESULT SHELL32_AssocGetFileDescription(PCWSTR Name, PWSTR Buf, UINT cchBuf)
{
    return SHELL32_AssocGetExtensionDescription(PathFindExtensionW(Name), Buf, cchBuf);
}

HRESULT ClassesRegKey::Initialize(ASSOCF fA, PCWSTR pszSource)
{
    if (!(fA & ASSOCF_PER_MACHINE_ONLY))
    {
        hKey = HKEY_CLASSES_ROOT;
        return S_OK;
    }
    HKEY hKeyClasses;
    LONG err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes", 0, KEY_READ, &hKeyClasses);
    if (err)
        return HResultFromWin32(err);
    err = RegOpenKeyExW(hKeyClasses, pszSource, 0, KEY_READ, &hKey);
    RegCloseKey(hKeyClasses);
    if (err == ERROR_SUCCESS)
        hKeyClose = hKey;
    return HResultFromWin32(err);
}

HRESULT OpenShellVerbKey(HKEY hClass, LPCWSTR pszVerb, HKEY *phKey)
{
    const SIZE_T cchPrefix = sizeof("shell\\") - 1;
    WCHAR szPath[cchPrefix + MAX_PATH];
    if (FAILED(StringCchPrintfW(szPath, _countof(szPath), L"shell\\%s", pszVerb ? pszVerb : L"")))
        return E_FAIL;

    if (!pszVerb && !HCR_GetDefaultVerbW(hClass, NULL, szPath + cchPrefix, _countof(szPath) - cchPrefix))
        return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);

    return HResultFromWin32(RegOpenKeyExW(hClass, szPath, 0, KEY_READ, phKey));
}

HRESULT OpenClassWithShellExecKey(ASSOCF fA, HKEY hClass, LPCWSTR pszVerb, HKEY *phKey)
{
    if (fA & ASSOCF_VERIFY)
    {
        HRESULT hr = OpenShellVerbKey(hClass, pszVerb, phKey);
        if (FAILED(hr))
            return HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        RegCloseKey(*phKey);
    }
    return SHRegDuplicateHKey(hClass, phKey);
}

HRESULT GetClassKeyName(ASSOCF fA, PCWSTR pszSource, HKEY hSource, LPWSTR pszClass, UINT cchMax)
{
    PCWSTR pszSubKey = *pszSource == L'{' ? L"ProgID" : NULL;
    return SHELL_RegGetString(hSource, pszSubKey, NULL, pszClass, cchMax);
}

HRESULT OpenClassKey(ASSOCF fA, PCWSTR pszSource, HKEY hCR, HKEY hSource, HKEY *phKey)
{
    WCHAR szBuf[MAX_PATH];
    HRESULT hr = E_FAIL;
    if (*pszSource == L'.' || (*pszSource == L'{' && !(fA & ASSOCF_INIT_NOREMAPCLSID)))
    {
        hr = GetClassKeyName(fA, pszSource, hSource, szBuf, _countof(szBuf));
        if (FAILED(hr) && *pszSource == L'.')
            return hr; // .ext can only have its ProgId in another location
    }

    HKEY hClassKey;
    if (SUCCEEDED(hr))
        hr = HResultFromWin32(RegOpenKeyExW(hCR, szBuf, 0, KEY_READ, &hClassKey));
    else
        hr = SHRegDuplicateHKey(hSource, &hClassKey);

    if (FAILED(hr))
        return hr;

    // Check for CurVer redirection 
    if (SUCCEEDED(SHELL_RegGetString(hClassKey, L"CurVer", NULL, szBuf, _countof(szBuf))))
    {
        if (RegOpenKeyExW(hCR, szBuf, 0, KEY_READ, phKey) == ERROR_SUCCESS)
        {
            if (RegKeyExists(*phKey, L"shell")) // Does it look like a valid ProgId?
            {
                RegCloseKey(hClassKey);
                return S_OK;
            }
            RegCloseKey(*phKey);
        }
    }
    *phKey = hClassKey;
    return hr;
}

HRESULT OpenClassKey(ASSOCF fA, const WCHAR *const pszSource, HKEY *phKey)
{
    // TODO: Handle the FileExts key and ASSOCF_NOUSERSETTINGS?
    PCWSTR pszSourcePath = pszSource;
    WCHAR buf[100];
    if (*pszSource == L'{')
    {
        HRESULT hr = StringCchPrintfW(buf, _countof(buf), L"CLSID\\%s", pszSource);
        if (FAILED(hr))
            return hr;
        pszSourcePath = buf;
    }
    ClassesRegKey cr;
    if (HRESULT hr = cr.Initialize(fA, pszSource))
        return hr;
    HKEY hSourceKey;
    LONG err = RegOpenKeyExW(cr.hKey, pszSourcePath, 0, KEY_READ, &hSourceKey);
    if (err)
        return HResultFromWin32(err);
    HRESULT hr = OpenClassKey(fA, pszSource, cr.hKey, hSourceKey, phKey);
    RegCloseKey(hSourceKey);
    return hr;
}
