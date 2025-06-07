
#pragma once

class CInputContext : public ITfContext
{
public:
    CInputContext() { }
    virtual ~CInputContext() { }

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfContext methods **
    STDMETHODIMP RequestEditSession(
        _In_ TfClientId tid,
        _In_ ITfEditSession *pes,
        _In_ DWORD dwFlags,
        _Out_ HRESULT *phrSession) override;
    STDMETHODIMP InWriteSession(_In_ TfClientId tid, _Out_ BOOL *pfWriteSession) override;
    STDMETHODIMP GetSelection(
        _In_ TfEditCookie ec,
        _In_ ULONG ulIndex,
        _In_ ULONG ulCount,
        _Out_ TF_SELECTION *pSelection,
        _Out_ ULONG *pcFetched) override;
    STDMETHODIMP SetSelection(
        _In_ TfEditCookie ec,
        _In_ ULONG ulCount,
        _In_ const TF_SELECTION *pSelection) override;
    STDMETHODIMP GetStart(_In_ TfEditCookie ec, _Out_ ITfRange **ppStart) override;
    STDMETHODIMP GetEnd(_In_ TfEditCookie ec, _Out_ ITfRange **ppEnd) override;
    STDMETHODIMP GetActiveView(_Out_ ITfContextView **ppView) override;
    STDMETHODIMP EnumViews(_Out_ IEnumTfContextViews **ppEnum) override;
    STDMETHODIMP GetStatus(_Out_ TF_STATUS *pdcs) override;
    STDMETHODIMP GetProperty(_In_ REFGUID guidProp, _Out_ ITfProperty **ppProp) override;
    STDMETHODIMP GetAppProperty(_In_ REFGUID guidProp, _Out_ ITfReadOnlyProperty **ppProp) override;
    STDMETHODIMP TrackProperties(
        _In_ const GUID **prgProp,
        _In_ ULONG cProp,
        _In_ const GUID **prgAppProp,
        _In_ ULONG cAppProp,
        _Out_ ITfReadOnlyProperty **ppProperty) override;
    STDMETHODIMP EnumProperties(_Out_ IEnumTfProperties **ppEnum) override;
    STDMETHODIMP GetDocumentMgr(_Out_ ITfDocumentMgr **ppDm) override;
    STDMETHODIMP CreateRangeBackup(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pRange,
        _Out_ ITfRangeBackup **ppBackup) override;
};
