/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     IQueryAssociations
 * COPYRIGHT:   Copyright 2002 Jon Griffiths (Wine)
 *              Copyright 2024-2026 Whindmar Saksit <whindsaks@proton.me>
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CQueryAssociations :
    public CComCoClass<CQueryAssociations, &CLSID_QueryAssociations>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
#if (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    public IAssociationArrayOld,
    public IAssociationArrayInitialize,
#endif
    public IQueryAssociations
{
public:
    struct RegKeyRef
    {
        HKEY hKey = NULL, hKeyClose = NULL;

        RegKeyRef(HKEY hNewKey = NULL) : hKey(hNewKey), hKeyClose(hNewKey) {}

        ~RegKeyRef()
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

private:
    HKEY hkeySource;
    HKEY hkeyProgID;
    CComPtr<CQueryAssociations> m_pBaseClass;
    ASSOCF m_InitFlags;
    BYTE m_InitType;
    bool m_SkipBaseClass = false;

    bool EmulateWin2000() const { return false; }
    bool IsNT5() const { return LOBYTE(GetVersion()) < 6; }
    void Reset();
    HRESULT DuplicateHKey(HKEY hKey, HKEY *phKey);
    HKEY GetClassHandle() const { return hkeyProgID ? hkeyProgID : hkeySource; }
    CQueryAssociations* TryBaseClass(ASSOCF flags);

    HRESULT OpenShellVerbKey(ASSOCF flags, LPCWSTR pszVerb, LPCWSTR pszSubKey, RegKeyRef &KeyRef);
    HRESULT OpenShellVerbKey(ASSOCF flags, LPCWSTR pszVerb, RegKeyRef &KeyRef)
    {
        return OpenShellVerbKey(flags, pszVerb, NULL, KeyRef);
    }
    HRESULT OpenDdeKey(ASSOCF flags, LPCWSTR pszExtra, RegKeyRef &KeyRef)
    {
        return OpenShellVerbKey(flags, pszExtra, L"ddeexec", KeyRef);
    }

    HRESULT GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size);
    HRESULT GetCommand(const WCHAR *extra, WCHAR **command);
    HRESULT GetExecutable(LPCWSTR pszExtra, LPWSTR path, DWORD pathlen, DWORD *len);
    HRESULT ReturnData(void *out, DWORD *outlen, const void *data, DWORD datalen);
    HRESULT ReturnRegData(ASSOCF flags, HKEY hKey, PCWSTR pszName, void *pvOut, DWORD *pcbOut);
    HRESULT ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen);
    HRESULT ReturnAndFreeString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPWSTR data, DWORD datalen)
    {
        HRESULT hr = ReturnString(flags, out, outlen, data, datalen);
        SHFree(data);
        return hr;
    }
    HRESULT ReturnAndFreeString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPWSTR data)
    {
        return ReturnAndFreeString(flags, out, outlen, data, lstrlenW(data) + 1);
    }

public:
    CQueryAssociations();
    ~CQueryAssociations();

#if (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    // *** IAssociationArrayOld ***
    STDMETHODIMP EnumElements(ULONG flags, IEnumAssociationElements **ppElement) override;
    STDMETHODIMP QueryString(ULONG flags, ASSOCQUERY query, PCWSTR key, PWSTR *ppszValue) override;
    STDMETHODIMP QueryDword(ULONG flags, ASSOCQUERY query, PCWSTR key, DWORD *pdwValue) override;
    STDMETHODIMP QueryExists(ULONG flags, ASSOCQUERY query, PCWSTR key) override;
    STDMETHODIMP QueryDirect(ULONG flags, ASSOCQUERY query, PCWSTR key, FLAGGED_BYTE_BLOB **ppBlob) override;
    STDMETHODIMP QueryObject(ULONG flags, ASSOCQUERY query, PCWSTR key, REFIID riid, PVOID *ppvObj) override;

    // *** IAssociationArrayInitialize ***
    STDMETHODIMP InitClassElements(ULONG flags, PCWSTR pszClass) override;
    STDMETHODIMP InsertElements(ULONG flags, IEnumAssociationElements *pElements) override;
    STDMETHODIMP FilterElements(ULONG dwFilter) override;
#endif // DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA

    // *** IQueryAssociations methods ***
    STDMETHOD(Init)(ASSOCF flags, LPCWSTR pwszAssoc, HKEY hkProgid, HWND hwnd) override;
    STDMETHOD(GetString)(ASSOCF flags, ASSOCSTR str, LPCWSTR pwszExtra, LPWSTR pwszOut, DWORD *pcchOut) override;
    STDMETHOD(GetKey)(ASSOCF flags, ASSOCKEY key, LPCWSTR pwszExtra, HKEY *phkeyOut) override;
    STDMETHOD(GetData)(ASSOCF flags, ASSOCDATA data, LPCWSTR pwszExtra, void *pvOut, DWORD *pcbOut) override;
    STDMETHOD(GetEnum)(ASSOCF cfFlags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut) override;

DECLARE_REGISTRY_RESOURCEID(IDR_QUERYASSOCIATIONS)
DECLARE_NOT_AGGREGATABLE(CQueryAssociations)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CQueryAssociations)
    #if (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
        COM_INTERFACE_ENTRY_IID(IID_IAssociationArrayOld, IAssociationArrayOld)
        COM_INTERFACE_ENTRY_IID(IID_IAssociationArrayInitialize, IAssociationArrayInitialize)
    #endif
    COM_INTERFACE_ENTRY_IID(IID_IQueryAssociations, IQueryAssociations)
END_COM_MAP()
};
