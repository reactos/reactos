#pragma once

DEFINE_GUID(IID_PRIV_CRANGE, 0xB68832F0, 0x34B9, 0x11D3, 0xA7, 0x45, 0x00, 0x50, 0x04, 0x0A, 0xB4, 0x07);

class CRange
    : public ITfRangeACP
    , public ITfRangeAnchor
    , public ITfSource
{
public:
    CRange(
        _In_ ITfContext *context,
        _In_ TfAnchor anchorStart,
        _In_ TfAnchor anchorEnd);
    virtual ~CRange();

    static HRESULT TF_SELECTION_to_TS_SELECTION_ACP(const TF_SELECTION *tf, TS_SELECTION_ACP *tsAcp);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfRange methods **
    STDMETHODIMP GetText(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _Out_ WCHAR *pchText,
        _In_ ULONG cchMax,
        _Out_ ULONG *pcch) override;
    STDMETHODIMP SetText(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _In_ const WCHAR *pchText,
        _In_ LONG cch) override;
    STDMETHODIMP GetFormattedText(
        _In_ TfEditCookie ec,
        _Out_ IDataObject **ppDataObject) override;
    STDMETHODIMP GetEmbedded(
        _In_ TfEditCookie ec,
        _In_ REFGUID rguidService,
        _In_ REFIID riid,
        _Out_ IUnknown **ppunk) override;
    STDMETHODIMP InsertEmbedded(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _In_ IDataObject *pDataObject) override;
    STDMETHODIMP ShiftStart(
        _In_ TfEditCookie ec,
        _In_ LONG cchReq,
        _Out_ LONG *pcch,
        _In_ const TF_HALTCOND *pHalt) override;
    STDMETHODIMP ShiftEnd(
        _In_ TfEditCookie ec,
        _In_ LONG cchReq,
        _Out_ LONG *pcch,
        _In_ const TF_HALTCOND *pHalt) override;
    STDMETHODIMP ShiftStartToRange(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pRange,
        _In_ TfAnchor aPos) override;
    STDMETHODIMP ShiftEndToRange(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pRange,
        _In_ TfAnchor aPos) override;
    STDMETHODIMP ShiftStartRegion(
        _In_ TfEditCookie ec,
        _In_ TfShiftDir dir,
        _Out_ BOOL *pfNoRegion) override;
    STDMETHODIMP ShiftEndRegion(
        _In_ TfEditCookie ec,
        _In_ TfShiftDir dir,
        _Out_ BOOL *pfNoRegion) override;
    STDMETHODIMP IsEmpty(_In_ TfEditCookie ec, _Out_ BOOL *pfEmpty) override;
    STDMETHODIMP Collapse(_In_ TfEditCookie ec, _In_ TfAnchor aPos) override;
    STDMETHODIMP IsEqualStart(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ BOOL *pfEqual) override;
    STDMETHODIMP IsEqualEnd(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ BOOL *pfEqual) override;
    STDMETHODIMP CompareStart(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ LONG *plResult) override;
    STDMETHODIMP CompareEnd(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ LONG *plResult) override;
    STDMETHODIMP AdjustForInsert(
        _In_ TfEditCookie ec,
        _In_ ULONG cchInsert,
        _Out_ BOOL *pfInsertOk) override;
    STDMETHODIMP GetGravity(_Out_ TfGravity *pgStart, _Out_ TfGravity *pgEnd) override;
    STDMETHODIMP SetGravity(
        _In_ TfEditCookie ec,
        _In_ TfGravity gStart,
        _In_ TfGravity gEnd) override;
    STDMETHODIMP Clone(_Out_ ITfRange **ppClone) override;
    STDMETHODIMP GetContext(_Out_ ITfContext **ppContext) override;

    // ** ITfRangeACP methods **
    STDMETHODIMP GetExtent(_Out_ LONG *pacpAnchor, _Out_ LONG *pcch) override;
    STDMETHODIMP SetExtent(_In_ LONG acpAnchor, _In_ LONG cch) override;

    // ** ITfRangeAnchor methods **
    STDMETHODIMP GetExtent(_Out_ IAnchor **ppStart, _Out_ IAnchor **ppEnd) override;
    STDMETHODIMP SetExtent(_In_ IAnchor *pAnchorStart, _In_ IAnchor *pAnchorEnd) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(_In_ REFIID riid, _In_ IUnknown *punk, _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(_In_ DWORD dwCookie) override;

protected:
    LONG m_cRefs;
    ITfContext *m_context;
    DWORD m_dwLockType;
    TfAnchor m_anchorStart;
    TfAnchor m_anchorEnd;
    DWORD m_dwCookie;

    CRange *_Clone();

    HRESULT _IsEqualX(TfEditCookie ec, BOOL bEnd, ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual);

    HRESULT _CompareX(
        TfEditCookie ec,
        BOOL bEnd,
        ITfRange *pWidth,
        TfAnchor aPos,
        LONG *plResult);
};
