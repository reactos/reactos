/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfContext implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <cguid.h>
#include <olectl.h>
#include <oleauto.h>
#include <msctf.h>
#include <msctf_undoc.h>

// Cicero
#include <cicbase.h>
#include <cicreg.h>
#include <cicutb.h>

#include "msctf_internal.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

class CContext
    : public ITfContext
    , public ITfSource
    // , public ITfContextComposition
    , public ITfContextOwnerCompositionServices
    // , public ITfContextOwnerServices
    , public ITfInsertAtSelection
    // , public ITfMouseTracker
    // , public ITfQueryEmbedded
    , public ITfSourceSingle
    , public ITextStoreACPSink
    , public ITextStoreACPServices
{
public:
    CContext();
    virtual ~CContext();

    static HRESULT CreateInstance(
        TfClientId tidOwner,
        IUnknown *punk,
        ITfDocumentMgr *mgr,
        ITfContext **ppOut,
        TfEditCookie *pecTextStore);
    HRESULT Initialize(ITfDocumentMgr *manager);
    HRESULT Uninitialize();

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
    STDMETHODIMP InWriteSession(
        _In_ TfClientId tid,
        _Out_ BOOL *pfWriteSession) override;
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
    STDMETHODIMP GetStart(
        _In_ TfEditCookie ec,
        _Out_ ITfRange **ppStart) override;
    STDMETHODIMP GetEnd(
        _In_ TfEditCookie ec,
        _Out_ ITfRange **ppEnd) override;
    STDMETHODIMP GetActiveView(_Out_ ITfContextView **ppView) override;
    STDMETHODIMP EnumViews(_Out_ IEnumTfContextViews **ppEnum) override;
    STDMETHODIMP GetStatus(_Out_ TF_STATUS *pdcs) override;
    STDMETHODIMP GetProperty(
        _In_ REFGUID guidProp,
        _Out_ ITfProperty **ppProp) override;
    STDMETHODIMP GetAppProperty(
        _In_ REFGUID guidProp,
        _Out_ ITfReadOnlyProperty **ppProp) override;
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

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(
        _In_ REFIID riid,
        _In_ IUnknown *punk,
        _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(_In_ DWORD dwCookie) override;

    // ** ITfContextOwnerCompositionServices methods **
    STDMETHODIMP StartComposition(
        _In_ TfEditCookie ecWrite,
        _In_ ITfRange *pCompositionRange,
        _In_ ITfCompositionSink *pSink,
        _Out_ ITfComposition **ppComposition);
    STDMETHODIMP EnumCompositions(_Out_ IEnumITfCompositionView **ppEnum);
    STDMETHODIMP FindComposition(
        _In_ TfEditCookie ecRead,
        _In_ ITfRange *pTestRange,
        _Out_ IEnumITfCompositionView **ppEnum);
    STDMETHODIMP TakeOwnership(
        _In_ TfEditCookie ecWrite,
        _In_ ITfCompositionView *pComposition,
        _In_ ITfCompositionSink *pSink,
        _Out_ ITfComposition **ppComposition);
    STDMETHODIMP TerminateComposition(_In_ ITfCompositionView *pComposition) override;

    // ** ITfInsertAtSelection methods **
    STDMETHODIMP InsertTextAtSelection(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _In_ const WCHAR *pchText,
        _In_ LONG cch,
        _Out_ ITfRange **ppRange) override;
    STDMETHODIMP InsertEmbeddedAtSelection(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _In_ IDataObject *pDataObject,
        _Out_ ITfRange **ppRange) override;

    // ** ITfSourceSingle methods **
    STDMETHODIMP AdviseSingleSink(
        _In_ TfClientId tid,
        _In_ REFIID riid,
        _In_ IUnknown *punk) override;
    STDMETHODIMP UnadviseSingleSink(
        _In_ TfClientId tid,
        _In_ REFIID riid) override;

    // ** ITextStoreACPSink methods **
    STDMETHODIMP OnTextChange(
        _In_ DWORD dwFlags,
        _In_ const TS_TEXTCHANGE *pChange) override;
    STDMETHODIMP OnSelectionChange() override;
    STDMETHODIMP OnLayoutChange(
        _In_ TsLayoutCode lcode,
        _In_ TsViewCookie vcView) override;
    STDMETHODIMP OnStatusChange(_In_ DWORD dwFlags) override;
    STDMETHODIMP OnAttrsChange(
        _In_ LONG acpStart,
        _In_ LONG acpEnd,
        _In_ ULONG cAttrs,
        _In_ const TS_ATTRID *paAttrs) override;
    STDMETHODIMP OnLockGranted(_In_ DWORD dwLockFlags) override;
    STDMETHODIMP OnStartEditTransaction() override;
    STDMETHODIMP OnEndEditTransaction() override;

    // ** ITextStoreACPServices methods **
    STDMETHODIMP Serialize(
        _In_ ITfProperty *prop,
        _In_ ITfRange *range,
        _Out_ TF_PERSISTENT_PROPERTY_HEADER_ACP *header,
        _In_ IStream *stream) override;
    STDMETHODIMP Unserialize(
        _In_ ITfProperty *prop,
        _In_ const TF_PERSISTENT_PROPERTY_HEADER_ACP *header,
        _In_ IStream *stream,
        _In_ ITfPersistentPropertyLoaderACP *loader) override;
    STDMETHODIMP ForceLoadProperty(_In_ ITfProperty *prop) override;
    STDMETHODIMP CreateRange(
        _In_ LONG start,
        _In_ LONG end,
        _Out_ ITfRangeACP **range) override;

protected:
    LONG m_cRefs;
    BOOL m_connected;

    // Aggregation
    ITfCompartmentMgr *m_CompartmentMgr;

    TfClientId m_tidOwner;
    TfEditCookie m_defaultCookie;
    TS_STATUS m_documentStatus;
    ITfDocumentMgr *m_manager;

    ITextStoreACP *m_pITextStoreACP;
    ITfContextOwnerCompositionSink *m_pITfContextOwnerCompositionSink;
    ITfEditSession *m_currentEditSession;

    // kept as separate lists to reduce unnecessary iterations
    struct list m_pContextKeyEventSink;
    struct list m_pEditTransactionSink;
    struct list m_pStatusSink;
    struct list m_pTextEditSink;
    struct list m_pTextLayoutSink;
};

////////////////////////////////////////////////////////////////////////////

typedef struct tagEditCookie
{
    DWORD lockType;
    CContext *pOwningContext;
} EditCookie;

////////////////////////////////////////////////////////////////////////////

CContext::CContext()
    : m_cRefs(1)
    , m_connected(FALSE)
    , m_CompartmentMgr(NULL)
    , m_manager(NULL)
    , m_pITextStoreACP(NULL)
    , m_pITfContextOwnerCompositionSink(NULL)
    , m_currentEditSession(NULL)
{
    list_init(&m_pContextKeyEventSink);
    list_init(&m_pEditTransactionSink);
    list_init(&m_pStatusSink);
    list_init(&m_pTextEditSink);
    list_init(&m_pTextLayoutSink);
}

CContext::~CContext()
{
    EditCookie *cookie;
    TRACE("destroying %p\n", this);

    if (m_pITextStoreACP)
        m_pITextStoreACP->Release();

    if (m_pITfContextOwnerCompositionSink)
        m_pITfContextOwnerCompositionSink->Release();

    if (m_defaultCookie)
    {
        cookie = (EditCookie *)remove_Cookie(m_defaultCookie);
        HeapFree(GetProcessHeap(), 0, cookie);
        m_defaultCookie = 0;
    }

    free_sinks(&m_pContextKeyEventSink);
    free_sinks(&m_pEditTransactionSink);
    free_sinks(&m_pStatusSink);
    free_sinks(&m_pTextEditSink);
    free_sinks(&m_pTextLayoutSink);

    if (m_CompartmentMgr)
        m_CompartmentMgr->Release();
}

STDMETHODIMP CContext::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_ITfContext)
        *ppvObj = static_cast<ITfContext *>(this);
    else if (riid == IID_ITfSource)
        *ppvObj = static_cast<ITfSource *>(this);
    else if (riid == IID_ITfContextOwnerCompositionServices)
        *ppvObj = static_cast<ITfContextOwnerCompositionServices *>(this);
    else if (riid == IID_ITfInsertAtSelection)
        *ppvObj = static_cast<ITfInsertAtSelection *>(this);
    else if (riid == IID_ITfCompartmentMgr)
        *ppvObj = m_CompartmentMgr;
    else if (riid == IID_ITfSourceSingle)
        *ppvObj = static_cast<ITfSourceSingle *>(this);
    else if (riid == IID_ITextStoreACPSink)
        *ppvObj = static_cast<ITextStoreACPSink *>(this);
    else if (riid == IID_ITextStoreACPServices)
        *ppvObj = static_cast<ITextStoreACPServices *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CContext::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CContext::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CContext::RequestEditSession(
    _In_ TfClientId tid,
    _In_ ITfEditSession *pes,
    _In_ DWORD dwFlags,
    _Out_ HRESULT *phrSession)
{
    HRESULT hr;
    DWORD dwLockFlags = 0x0;

    TRACE("(%p) %i %p %x %p\n", this, tid, pes, dwFlags, phrSession);

    if (!(dwFlags & TF_ES_READ) && !(dwFlags & TF_ES_READWRITE))
    {
        *phrSession = E_FAIL;
        return E_INVALIDARG;
    }

    if (!m_pITextStoreACP)
    {
        FIXME("No ITextStoreACP available\n");
        *phrSession = E_FAIL;
        return E_FAIL;
    }

    if (!(dwFlags & TF_ES_ASYNC))
        dwLockFlags |= TS_LF_SYNC;

    if ((dwFlags & TF_ES_READWRITE) == TF_ES_READWRITE)
        dwLockFlags |= TS_LF_READWRITE;
    else if (dwFlags & TF_ES_READ)
        dwLockFlags |= TS_LF_READ;

    if (!m_documentStatus.dwDynamicFlags)
        m_pITextStoreACP->GetStatus(&m_documentStatus);

    if (((dwFlags & TF_ES_READWRITE) == TF_ES_READWRITE) &&
        (m_documentStatus.dwDynamicFlags & TS_SD_READONLY))
    {
        *phrSession = TS_E_READONLY;
        return S_OK;
    }

    hr = pes->QueryInterface(IID_ITfEditSession, (LPVOID *)&m_currentEditSession);
    if (FAILED(hr))
    {
        *phrSession = E_FAIL;
        return E_INVALIDARG;
    }

    return m_pITextStoreACP->RequestLock(dwLockFlags, phrSession);
}

STDMETHODIMP CContext::InWriteSession(
    _In_ TfClientId tid,
    _Out_ BOOL *pfWriteSession)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::GetSelection(
    _In_ TfEditCookie ec,
    _In_ ULONG ulIndex,
    _In_ ULONG ulCount,
    _Out_ TF_SELECTION *pSelection,
    _Out_ ULONG *pcFetched)
{
    EditCookie *cookie;
    ULONG count, i;
    ULONG totalFetched = 0;
    HRESULT hr = S_OK;

    if (!pSelection || !pcFetched)
        return E_INVALIDARG;

    *pcFetched = 0;

    if (!m_connected)
        return TF_E_DISCONNECTED;

    if (get_Cookie_magic(ec) != COOKIE_MAGIC_EDITCOOKIE)
        return TF_E_NOLOCK;

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    cookie = (EditCookie *)get_Cookie_data(ec);

    count = (ulIndex == (ULONG)TF_DEFAULT_SELECTION) ? 1 : ulCount;

    for (i = 0; i < count; i++)
    {
        DWORD fetched;
        TS_SELECTION_ACP acps;

        hr = m_pITextStoreACP->GetSelection(ulIndex + i, 1, &acps, &fetched);
        if (hr == TS_E_NOLOCK)
            return TF_E_NOLOCK;
        else if (FAILED(hr))
            break;

        pSelection[totalFetched].style.ase = (TfActiveSelEnd)acps.style.ase;
        pSelection[totalFetched].style.fInterimChar = acps.style.fInterimChar;
        Range_Constructor(this, m_pITextStoreACP, cookie->lockType, acps.acpStart, acps.acpEnd, &pSelection[totalFetched].range);
        totalFetched++;
    }

    *pcFetched = totalFetched;
    return hr;
}

STDMETHODIMP
CContext::SetSelection(
    _In_ TfEditCookie ec,
    _In_ ULONG ulCount,
    _In_ const TF_SELECTION *pSelection)
{
    TS_SELECTION_ACP *acp;
    ULONG i;
    HRESULT hr;

    TRACE("(%p) %i %i %p\n", this, ec, ulCount, pSelection);

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    if (get_Cookie_magic(ec) != COOKIE_MAGIC_EDITCOOKIE)
        return TF_E_NOLOCK;

    acp = (TS_SELECTION_ACP *)HeapAlloc(GetProcessHeap(), 0, sizeof(TS_SELECTION_ACP) * ulCount);
    if (!acp)
        return E_OUTOFMEMORY;

    for (i = 0; i < ulCount; i++)
    {
        if (FAILED(TF_SELECTION_to_TS_SELECTION_ACP(&pSelection[i], &acp[i])))
        {
            TRACE("Selection Conversion Failed\n");
            HeapFree(GetProcessHeap(), 0 , acp);
            return E_FAIL;
        }
    }

    hr = m_pITextStoreACP->SetSelection(ulCount, acp);

    HeapFree(GetProcessHeap(), 0, acp);

    return hr;
}

STDMETHODIMP
CContext::GetStart(
    _In_ TfEditCookie ec,
    _Out_ ITfRange **ppStart)
{
    EditCookie *cookie;
    TRACE("(%p) %i %p\n", this, ec, ppStart);

    if (!ppStart)
        return E_INVALIDARG;

    *ppStart = NULL;

    if (!m_connected)
        return TF_E_DISCONNECTED;

    if (get_Cookie_magic(ec) != COOKIE_MAGIC_EDITCOOKIE)
        return TF_E_NOLOCK;

    cookie = (EditCookie *)get_Cookie_data(ec);
    return Range_Constructor(this, m_pITextStoreACP, cookie->lockType, 0, 0, ppStart);
}

STDMETHODIMP
CContext::GetEnd(
    _In_ TfEditCookie ec,
    _Out_ ITfRange **ppEnd)
{
    EditCookie *cookie;
    LONG end;

    TRACE("(%p) %i %p\n", this, ec, ppEnd);

    if (!ppEnd)
        return E_INVALIDARG;

    *ppEnd = NULL;

    if (!m_connected)
        return TF_E_DISCONNECTED;

    if (get_Cookie_magic(ec) != COOKIE_MAGIC_EDITCOOKIE)
        return TF_E_NOLOCK;

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    cookie = (EditCookie *)get_Cookie_data(ec);
    m_pITextStoreACP->GetEndACP(&end);

    return Range_Constructor(this, m_pITextStoreACP, cookie->lockType, end, end, ppEnd);
}

STDMETHODIMP CContext::GetActiveView(_Out_ ITfContextView **ppView)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::EnumViews(_Out_ IEnumTfContextViews **ppEnum)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::GetStatus(_Out_ TF_STATUS *pdcs)
{
    TRACE("(%p) %p\n", this, pdcs);

    if (!m_connected)
        return TF_E_DISCONNECTED;

    if (!pdcs)
        return E_INVALIDARG;

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    m_pITextStoreACP->GetStatus(&m_documentStatus);

    *pdcs = m_documentStatus;

    return S_OK;
}

STDMETHODIMP CContext::GetProperty(
    _In_ REFGUID guidProp,
    _Out_ ITfProperty **ppProp)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::GetAppProperty(
    _In_ REFGUID guidProp,
    _Out_ ITfReadOnlyProperty **ppProp)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::TrackProperties(
    _In_ const GUID **prgProp,
    _In_ ULONG cProp,
    _In_ const GUID **prgAppProp,
    _In_ ULONG cAppProp,
    _Out_ ITfReadOnlyProperty **ppProperty)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::EnumProperties(_Out_ IEnumTfProperties **ppEnum)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::GetDocumentMgr(_Out_ ITfDocumentMgr **ppDm)
{
    TRACE("(%p) %p\n", this, ppDm);

    if (!ppDm)
        return E_INVALIDARG;

    *ppDm = m_manager;
    if (!m_manager)
        return S_FALSE;

    m_manager->AddRef();
    return S_OK;
}

STDMETHODIMP CContext::CreateRangeBackup(
    _In_ TfEditCookie ec,
    _In_ ITfRange *pRange,
    _Out_ ITfRangeBackup **ppBackup)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::AdviseSink(
    _In_ REFIID riid,
    _In_ IUnknown *punk,
    _Out_ DWORD *pdwCookie)
{
    TRACE("(%p) %s %p %p\n", this, debugstr_guid(&riid), punk, pdwCookie);

    if (cicIsNullPtr(&riid) || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (riid == IID_ITfTextEditSink)
        return advise_sink(&m_pTextEditSink, IID_ITfTextEditSink, COOKIE_MAGIC_CONTEXTSINK, punk, pdwCookie);

    FIXME("(%p) Unhandled Sink: %s\n", this, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CContext::UnadviseSink(_In_ DWORD dwCookie)
{
    TRACE("(%p) %x\n", this, dwCookie);

    if (get_Cookie_magic(dwCookie) != COOKIE_MAGIC_CONTEXTSINK)
        return E_INVALIDARG;

    return unadvise_sink(dwCookie);
}

STDMETHODIMP CContext::StartComposition(
    _In_ TfEditCookie ecWrite,
    _In_ ITfRange *pCompositionRange,
    _In_ ITfCompositionSink *pSink,
    _Out_ ITfComposition **ppComposition)
{
    FIXME("STUB:(%p) %#x %p %p %p\n", this, ecWrite, pCompositionRange, pSink, ppComposition);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::EnumCompositions(_Out_ IEnumITfCompositionView **ppEnum)
{
    FIXME("STUB:(%p) %p\n", this, ppEnum);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::FindComposition(
    _In_ TfEditCookie ecRead,
    _In_ ITfRange *pTestRange,
    _Out_ IEnumITfCompositionView **ppEnum)
{
    FIXME("STUB:(%p) %#x %p %p\n", this, ecRead, pTestRange, ppEnum);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::TakeOwnership(
    _In_ TfEditCookie ecWrite,
    _In_ ITfCompositionView *pComposition,
    _In_ ITfCompositionSink *pSink,
    _Out_ ITfComposition **ppComposition)
{
    FIXME("STUB:(%p) %#x %p %p %p\n", this, ecWrite, pComposition, pSink, ppComposition);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::TerminateComposition(_In_ ITfCompositionView *pComposition)
{
    FIXME("STUB:(%p) %p\n", this, pComposition);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::InsertTextAtSelection(
    _In_ TfEditCookie ec,
    _In_ DWORD dwFlags,
    _In_ const WCHAR *pchText,
    _In_ LONG cch,
    _Out_ ITfRange **ppRange)
{
    EditCookie *cookie;
    LONG acpStart, acpEnd;
    TS_TEXTCHANGE change;
    HRESULT hr;

    TRACE("(%p) %i %x %s %p\n", this, ec, dwFlags, debugstr_wn(pchText,cch), ppRange);

    if (!m_connected)
        return TF_E_DISCONNECTED;

    if (get_Cookie_magic(ec) != COOKIE_MAGIC_EDITCOOKIE)
        return TF_E_NOLOCK;

    cookie = (EditCookie *)get_Cookie_data(ec);

    if ((cookie->lockType & TS_LF_READWRITE) != TS_LF_READWRITE)
        return TS_E_READONLY;

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    hr = m_pITextStoreACP->InsertTextAtSelection(dwFlags, pchText, cch, &acpStart, &acpEnd, &change);
    if (SUCCEEDED(hr))
        Range_Constructor(this, m_pITextStoreACP, cookie->lockType, change.acpStart, change.acpNewEnd, ppRange);

    return hr;
}

STDMETHODIMP CContext::InsertEmbeddedAtSelection(
    _In_ TfEditCookie ec,
    _In_ DWORD dwFlags,
    _In_ IDataObject *pDataObject,
    _Out_ ITfRange **ppRange)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::AdviseSingleSink(
    _In_ TfClientId tid,
    _In_ REFIID riid,
    _In_ IUnknown *punk)
{
    FIXME("STUB:(%p) %i %s %p\n", this, tid, debugstr_guid(&riid), punk);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::UnadviseSingleSink(
    _In_ TfClientId tid,
    _In_ REFIID riid)
{
    FIXME("STUB:(%p) %i %s\n", this, tid, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CContext::OnTextChange(
    _In_ DWORD dwFlags,
    _In_ const TS_TEXTCHANGE *pChange)
{
    FIXME("STUB:(%p)\n", this);
    return S_OK;
}

STDMETHODIMP CContext::OnSelectionChange()
{
    FIXME("STUB:(%p)\n", this);
    return S_OK;
}

STDMETHODIMP CContext::OnLayoutChange(
    _In_ TsLayoutCode lcode,
    _In_ TsViewCookie vcView)
{
    FIXME("STUB:(%p)\n", this);
    return S_OK;
}

STDMETHODIMP CContext::OnStatusChange(_In_ DWORD dwFlags)
{
    HRESULT hr, hrSession;

    TRACE("(%p) %x\n", this, dwFlags);

    if (!m_pITextStoreACP)
    {
        FIXME("Context does not have a ITextStoreACP\n");
        return E_NOTIMPL;
    }

    hr = m_pITextStoreACP->RequestLock(TS_LF_READ, &hrSession);

    if(SUCCEEDED(hr) && SUCCEEDED(hrSession))
        m_documentStatus.dwDynamicFlags = dwFlags;

    return S_OK;
}

STDMETHODIMP CContext::OnAttrsChange(
    _In_ LONG acpStart,
    _In_ LONG acpEnd,
    _In_ ULONG cAttrs,
    _In_ const TS_ATTRID *paAttrs)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::OnLockGranted(_In_ DWORD dwLockFlags)
{
    HRESULT hr;
    EditCookie *cookie, *sinkcookie;
    TfEditCookie ec;
    struct list *cursor;

    TRACE("(%p) %x\n", this, dwLockFlags);

    if (!m_currentEditSession)
    {
        FIXME("OnLockGranted called for something other than an EditSession\n");
        return S_OK;
    }

    cookie = (EditCookie *)HeapAlloc(GetProcessHeap(), 0, sizeof(EditCookie));
    if (!cookie)
        return E_OUTOFMEMORY;

    sinkcookie = (EditCookie *)HeapAlloc(GetProcessHeap(), 0, sizeof(EditCookie));
    if (!sinkcookie)
    {
        HeapFree(GetProcessHeap(), 0, cookie);
        return E_OUTOFMEMORY;
    }

    cookie->lockType = dwLockFlags;
    cookie->pOwningContext = this;
    ec = generate_Cookie(COOKIE_MAGIC_EDITCOOKIE, cookie);

    hr = m_currentEditSession->DoEditSession(ec);

    if ((dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE)
    {
        ITfTextEditSink *sink;
        TfEditCookie sc;

        sinkcookie->lockType = TS_LF_READ;
        sinkcookie->pOwningContext = this;
        sc = generate_Cookie(COOKIE_MAGIC_EDITCOOKIE, sinkcookie);

        /*TODO: implement ITfEditRecord */
        SINK_FOR_EACH(cursor, &m_pTextEditSink, ITfTextEditSink, sink)
        {
            sink->OnEndEdit(static_cast<ITfContext *>(this), sc, NULL);
        }
        sinkcookie = (EditCookie *)remove_Cookie(sc);
    }
    HeapFree(GetProcessHeap(), 0, sinkcookie);

    m_currentEditSession->Release();
    m_currentEditSession = NULL;

    /* Edit Cookie is only valid during the edit session */
    cookie = (EditCookie *)remove_Cookie(ec);
    HeapFree(GetProcessHeap(), 0, cookie);

    return hr;
}

STDMETHODIMP CContext::OnStartEditTransaction()
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::OnEndEditTransaction()
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::Serialize(
    _In_ ITfProperty *prop,
    _In_ ITfRange *range,
    _Out_ TF_PERSISTENT_PROPERTY_HEADER_ACP *header,
    _In_ IStream *stream)
{
    FIXME("stub: %p %p %p %p %p\n", this, prop, range, header, stream);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::Unserialize(
    _In_ ITfProperty *prop,
    _In_ const TF_PERSISTENT_PROPERTY_HEADER_ACP *header,
    _In_ IStream *stream,
    _In_ ITfPersistentPropertyLoaderACP *loader)
{
    FIXME("stub: %p %p %p %p %p\n", this, prop, header, stream, loader);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::ForceLoadProperty(_In_ ITfProperty *prop)
{
    FIXME("stub: %p %p\n", this, prop);
    return E_NOTIMPL;
}

STDMETHODIMP CContext::CreateRange(
    _In_ LONG start,
    _In_ LONG end,
    _Out_ ITfRangeACP **range)
{
    FIXME("stub: %p %d %d %p\n", this, start, end, range);
    return S_OK;
}

HRESULT CContext::CreateInstance(
    TfClientId tidOwner,
    IUnknown *punk,
    ITfDocumentMgr *mgr,
    ITfContext **ppOut,
    TfEditCookie *pecTextStore)
{
    CContext *This = new(cicNoThrow) CContext();
    if (This == NULL)
        return E_OUTOFMEMORY;

    EditCookie *cookie = (EditCookie *)HeapAlloc(GetProcessHeap(), 0, sizeof(EditCookie));
    if (cookie == NULL)
    {
        delete This;
        return E_OUTOFMEMORY;
    }

    TRACE("(%p) %x %p %p %p\n", This, tidOwner, punk, ppOut, pecTextStore);

    This->m_tidOwner = tidOwner;
    This->m_manager = mgr;

    CompartmentMgr_Constructor(static_cast<ITfContext *>(This), IID_IUnknown, (IUnknown **)&This->m_CompartmentMgr);

    cookie->lockType = TF_ES_READ;
    cookie->pOwningContext = This;

    if (punk)
    {
        punk->QueryInterface(IID_ITextStoreACP, (LPVOID*)&This->m_pITextStoreACP);
        punk->QueryInterface(IID_ITfContextOwnerCompositionSink, (LPVOID*)&This->m_pITfContextOwnerCompositionSink);

        if (!This->m_pITextStoreACP && !This->m_pITfContextOwnerCompositionSink)
            FIXME("Unhandled pUnk\n");
    }

    This->m_defaultCookie = generate_Cookie(COOKIE_MAGIC_EDITCOOKIE, cookie);
    *pecTextStore = This->m_defaultCookie;

    list_init(&This->m_pContextKeyEventSink);
    list_init(&This->m_pEditTransactionSink);
    list_init(&This->m_pStatusSink);
    list_init(&This->m_pTextEditSink);
    list_init(&This->m_pTextLayoutSink);

    *ppOut = static_cast<ITfContext *>(This);
    TRACE("returning %p\n", *ppOut);

    return S_OK;
}

HRESULT CContext::Initialize(ITfDocumentMgr *manager)
{
    if (m_pITextStoreACP)
    {
        m_pITextStoreACP->AdviseSink(IID_ITextStoreACPSink,
                                     static_cast<ITextStoreACPSink *>(this),
                                     TS_AS_ALL_SINKS);
    }
    m_connected = TRUE;
    m_manager = manager;
    return S_OK;
}

HRESULT CContext::Uninitialize()
{
    if (m_pITextStoreACP)
        m_pITextStoreACP->UnadviseSink(static_cast<ITextStoreACPSink *>(this));
    m_connected = FALSE;
    m_manager = NULL;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT
Context_Constructor(
    TfClientId tidOwner,
    IUnknown *punk,
    ITfDocumentMgr *mgr,
    ITfContext **ppOut,
    TfEditCookie *pecTextStore)
{
    return CContext::CreateInstance(tidOwner, punk, mgr, ppOut, pecTextStore);
}

EXTERN_C
HRESULT Context_Initialize(ITfContext *iface, ITfDocumentMgr *manager)
{
    CContext *This = static_cast<CContext *>(iface);
    return This->Initialize(manager);
}

EXTERN_C
HRESULT Context_Uninitialize(ITfContext *iface)
{
    CContext *This = static_cast<CContext *>(iface);
    return This->Uninitialize();
}
