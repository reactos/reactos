#pragma once

class CQueryAssociations :
    public CComCoClass<CQueryAssociations, &CLSID_QueryAssociations>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IQueryAssociations
{
private:
    HKEY hkeySource;
    HKEY hkeyProgID;

    HRESULT GetValue(HKEY hkey, const WCHAR *name, void **data, DWORD *data_size);
    HRESULT GetCommand(const WCHAR *extra, WCHAR **command);
    HRESULT GetExecutable(LPCWSTR pszExtra, LPWSTR path, DWORD pathlen, DWORD *len);
    HRESULT ReturnData(void *out, DWORD *outlen, const void *data, DWORD datalen);
    HRESULT ReturnString(ASSOCF flags, LPWSTR out, DWORD *outlen, LPCWSTR data, DWORD datalen);

public:
    CQueryAssociations();
    ~CQueryAssociations();

    // *** IQueryAssociations methods ***
    virtual HRESULT STDMETHODCALLTYPE Init(ASSOCF flags, LPCWSTR pwszAssoc, HKEY hkProgid, HWND hwnd);
    virtual HRESULT STDMETHODCALLTYPE GetString(ASSOCF flags, ASSOCSTR str, LPCWSTR pwszExtra, LPWSTR pwszOut, DWORD *pcchOut);
    virtual HRESULT STDMETHODCALLTYPE GetKey(ASSOCF flags, ASSOCKEY key, LPCWSTR pwszExtra, HKEY *phkeyOut);
    virtual HRESULT STDMETHODCALLTYPE GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pwszExtra, void *pvOut, DWORD *pcbOut);
    virtual HRESULT STDMETHODCALLTYPE GetEnum(ASSOCF cfFlags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut);

DECLARE_REGISTRY_RESOURCEID(IDR_QUERYASSOCIATIONS)
DECLARE_NOT_AGGREGATABLE(CQueryAssociations)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CQueryAssociations)
    COM_INTERFACE_ENTRY_IID(IID_IQueryAssociations, IQueryAssociations)
END_COM_MAP()
};