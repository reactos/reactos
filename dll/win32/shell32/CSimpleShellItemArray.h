
#pragma once

class CSimpleShellItemArray :
    public CComCoClass<CSimpleShellItemArray, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellItemArray
{
    CAtlArray<CComPtr<IShellItem>> m_items;

public:
    HRESULT Initialize(_In_reads_(count) IShellItem **pitems, _In_ UINT count);

    // IShellItemArray
    STDMETHODIMP BindToHandler(_In_opt_ IBindCtx *pbc, _In_ REFGUID rbhid, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetPropertyStore(_In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetPropertyDescriptionList(_In_ REFPROPERTYKEY keyType, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetAttributes(_In_ SIATTRIBFLAGS dwAttribFlags, _In_ SFGAOF sfgaoMask, _Out_ SFGAOF *psfgaoAttribs) override;
    STDMETHODIMP GetCount(_Out_ DWORD *pdwNumItems) override;
    STDMETHODIMP GetItemAt(_In_ DWORD dwIndex, _Out_ IShellItem **ppsi) override;
    STDMETHODIMP EnumItems(_Out_ IEnumShellItems **ppenumShellItems) override;

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSimpleShellItemArray)

BEGIN_COM_MAP(CSimpleShellItemArray)
    COM_INTERFACE_ENTRY_IID(IID_IShellItemArray, IShellItemArray)
END_COM_MAP()
};

HRESULT CreateShellItemArrayFromItems(
    _In_reads_(cidl) IShellItem **items,
    _In_ UINT cidl,
    _In_ REFIID riid,
    _Out_ void **ppv);
