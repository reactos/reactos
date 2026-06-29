#pragma once

DEFINE_GUID(IID_CDisplayAttributeMgr, 0xFF4619E8, 0xEA5E, 0x43E5, 0xB3, 0x08, 0x11, 0xCD, 0x26, 0xAB, 0x6B, 0x3A);

class CDisplayAttributeMgr
    : public ITfDisplayAttributeMgr
    , public ITfDisplayAttributeCollectionMgr
{
public:
    CDisplayAttributeMgr();
    virtual ~CDisplayAttributeMgr();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfDisplayAttributeMgr methods **
    STDMETHODIMP OnUpdateInfo() override;
    STDMETHODIMP EnumDisplayAttributeInfo(_Out_ IEnumTfDisplayAttributeInfo **ppEnum) override;
    STDMETHODIMP GetDisplayAttributeInfo(
        _In_ REFGUID guid,
        _Out_ ITfDisplayAttributeInfo **ppInfo,
        _Out_ CLSID *pclsidOwner) override;

    // ** ITfDisplayAttributeCollectionMgr methods **
    STDMETHODIMP UnknownMethod(_In_ DWORD unused) override; // FIXME

protected:
    LONG m_cRefs;
    CicArray<IUnknown *> m_array1;
    CicArray<DWORD> m_array2;

    BOOL _IsInCollection(REFGUID rguid);
    void _AdviseMarkupCollection(ITfTextInputProcessor *pProcessor, DWORD dwCookie);
    void _UnadviseMarkupCollection(DWORD dwCookie);
    void _SetThis();
};
