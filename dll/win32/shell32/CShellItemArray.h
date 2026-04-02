

#pragma once

class CShellItemArray :
    public CComCoClass<CShellItemArray, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellItemArray
{
    CIDA *m_pCIDA;
    STGMEDIUM m_Medium;

public:
    CShellItemArray();
    virtual ~CShellItemArray();

    HRESULT Initialize(_In_ IDataObject *pdo);

    UINT ItemCount() const;

    // IShellItemArray
    STDMETHODIMP BindToHandler(_In_opt_ IBindCtx *pbc, _In_ REFGUID rbhid, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetPropertyStore(_In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetPropertyDescriptionList(_In_ REFPROPERTYKEY keyType, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHODIMP GetAttributes(_In_ SIATTRIBFLAGS dwAttribFlags, _In_ SFGAOF sfgaoMask, _Out_ SFGAOF *psfgaoAttribs) override;
    STDMETHODIMP GetCount(_Out_ DWORD *pCount) override;
    STDMETHODIMP GetItemAt(_In_ DWORD nIndex, _Out_ IShellItem **ppItem) override;
    STDMETHODIMP EnumItems(_Out_ IEnumShellItems **ppESI) override;

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItemArray)

BEGIN_COM_MAP(CShellItemArray)
    COM_INTERFACE_ENTRY_IID(IID_IShellItemArray, IShellItemArray)
END_COM_MAP()
};
