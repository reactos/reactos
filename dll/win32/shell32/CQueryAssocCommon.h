/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IQueryAssociations common (pre-Vista and Vista+)
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#if NTDDI_VERSION < NTDDI_WIN10_RS1
    #define ASSOCF_PER_MACHINE_ONLY 0x00008000
#endif

static inline HRESULT SHRegDuplicateHKey(HKEY hKey, HKEY *phKey)
{
    hKey = *phKey = ::SHRegDuplicateHKey(hKey);
    return hKey ? S_OK : HResultFromWin32(GetLastError());
}

static inline HRESULT SH32_AssocCreateElement(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ PVOID *ppv)
{
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA
    return AssocCreateElement(rclsid, riid, ppv);
#else
    return AssocCreate(rclsid, riid, ppv);
#endif
}

HRESULT AssocCreateElementInternal(REFCLSID rclsid, IQuerySource *pQS, REFIID riid, PVOID *ppv);

HRESULT AssocCreateElementOnKeyInternal(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                        _In_ REFIID riid, _Outptr_ PVOID *ppv, _In_ BYTE Major);

HRESULT SH32_AssocCreateElementOnKey(_In_ REFCLSID rclsid, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey,
                                     _In_ REFIID riid, _Outptr_ PVOID *ppv);

HRESULT SH32_QueryAssocElementString(_In_ REFCLSID rclsid, _In_ UINT Query, _In_ HKEY hKey,
                                     _In_opt_ PCWSTR pszSubKey, _In_ PCWSTR pszExtra, _Out_ PWSTR *ppszValue);

EXTERN_C HRESULT SHELL32_AssocGetFSDirectoryDescription(PWSTR Buf, UINT cchBuf);

HRESULT GetExtensionDefaultDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf);

HRESULT SHELL32_AssocGetExtensionDescription(PCWSTR DotExt, PWSTR Buf, UINT cchBuf);

EXTERN_C HRESULT SHELL32_AssocGetFileDescription(PCWSTR Name, PWSTR Buf, UINT cchBuf);

HRESULT OpenShellVerbKey(HKEY hClass, LPCWSTR pszVerb, HKEY *phKey);
HRESULT OpenClassWithShellExecKey(ASSOCF fA, HKEY hClass, LPCWSTR pszVerb, HKEY *phKey);
HRESULT GetClassKeyName(ASSOCF fA, PCWSTR pszSource, HKEY hSource, LPWSTR pszClass, UINT cchMax);
HRESULT OpenClassKey(ASSOCF fA, PCWSTR pszSource, HKEY hCR, HKEY hSource, HKEY *phKey);
HRESULT OpenClassKey(ASSOCF fA, const WCHAR *const pszSource, HKEY *phKey);

template <class T>
HRESULT GetKeyFromObjectWithQuerySource(T &Obj, HKEY *phKey)
{
    HRESULT hr;
    C_ASSERT(&IID_IObjectWithQuerySource == &IID_IObjectWithQuerySourceOld);
    CComPtr<IObjectWithQuerySource> pOWQS;
    if (FAILED(hr = Obj.QueryInterface(IID_PPV_ARG(IObjectWithQuerySource, &pOWQS))))
        return hr;
    CComPtr<IObjectWithRegistryKeyOld> pOWRKO;
    CComPtr<IObjectWithRegistryKey> pOWRK;
    if (SUCCEEDED(hr = pOWQS->GetSource(IID_PPV_ARG(IObjectWithRegistryKeyOld, &pOWRKO))))
        hr = pOWRKO->GetKey(phKey);
    return hr;
}

struct CRegKeyRef
{
    HKEY hKey = NULL, hKeyClose = NULL;

    CRegKeyRef(HKEY hNewKey = NULL) : hKey(hNewKey), hKeyClose(hNewKey) {}

    ~CRegKeyRef()
    {
        Close();
    }

    void Close()
    {
        if (hKeyClose)
            RegCloseKey(hKeyClose);
        hKeyClose = NULL;
    }

    void Attach(HKEY hNewKey)
    {
        Close();
        hKey = hKeyClose = hNewKey;
    }

    HKEY Detach()
    {
        HKEY hRet = hKeyClose;
        hKey = hKeyClose = NULL;
        return hRet;
    }

    operator HKEY() { return hKey; }
};

struct ClassesRegKey : CRegKeyRef
{
    HRESULT Initialize(ASSOCF fA, PCWSTR pszSource);
};
