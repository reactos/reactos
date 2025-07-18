/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfThreadMgr implementation
 * COPYRIGHT:   Copyright 2008 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

typedef struct tagPreservedKey
{
    struct list     entry;
    GUID            guid;
    TF_PRESERVEDKEY prekey;
    LPWSTR          description;
    TfClientId      tid;
} PreservedKey;

typedef struct tagDocumentMgrs
{
    struct list     entry;
    ITfDocumentMgr  *docmgr;
} DocumentMgrEntry;

typedef struct tagAssociatedWindow
{
    struct list     entry;
    HWND            hwnd;
    ITfDocumentMgr  *docmgr;
} AssociatedWindow;

////////////////////////////////////////////////////////////////////////////

class CThreadMgr
    : public ITfThreadMgrEx
    , public ITfSource
    , public ITfKeystrokeMgr
    , public ITfMessagePump
    , public ITfClientId
    // , public ITfConfigureSystemKeystrokeFeed
    // , public ITfLangBarItemMgr
    , public ITfUIElementMgr
    , public ITfSourceSingle
    , public ITfThreadMgrEventSink
{
public:
    CThreadMgr();
    virtual ~CThreadMgr();

    static HRESULT CreateInstance(IUnknown *pUnkOuter, CThreadMgr **ppOut);
    void OnDocumentMgrDestruction(ITfDocumentMgr *mgr);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfThreadMgr methods **
    STDMETHODIMP Activate(_Out_ TfClientId *ptid) override;
    STDMETHODIMP Deactivate() override;
    STDMETHODIMP CreateDocumentMgr(_Out_ ITfDocumentMgr **ppdim) override;
    STDMETHODIMP EnumDocumentMgrs(_Out_ IEnumTfDocumentMgrs **ppEnum) override;
    STDMETHODIMP GetFocus(_Out_ ITfDocumentMgr **ppdimFocus) override;
    STDMETHODIMP SetFocus(_In_ ITfDocumentMgr *pdimFocus) override;
    STDMETHODIMP AssociateFocus(
        _In_ HWND hwnd,
        _In_ ITfDocumentMgr *pdimNew,
        _Out_ ITfDocumentMgr **ppdimPrev) override;
    STDMETHODIMP IsThreadFocus(_Out_ BOOL *pfThreadFocus) override;
    STDMETHODIMP GetFunctionProvider(
        _In_ REFCLSID clsid,
        _Out_ ITfFunctionProvider **ppFuncProv) override;
    STDMETHODIMP EnumFunctionProviders(_Out_ IEnumTfFunctionProviders **ppEnum) override;
    STDMETHODIMP GetGlobalCompartment(_Out_ ITfCompartmentMgr **ppCompMgr) override;

    // ** ITfThreadMgrEx methods **
    STDMETHODIMP ActivateEx(
        _Out_ TfClientId *id,
        _In_ DWORD flags) override;
    STDMETHODIMP GetActiveFlags(_Out_ DWORD *flags) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(
        _In_ REFIID riid,
        _In_ IUnknown *punk,
        _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(_In_ DWORD dwCookie) override;

    // ** ITfKeystrokeMgr methods **
    STDMETHODIMP AdviseKeyEventSink(
        _In_ TfClientId tid,
        _In_ ITfKeyEventSink *pSink,
        _In_ BOOL fForeground) override;
    STDMETHODIMP UnadviseKeyEventSink(_In_ TfClientId tid) override;
    STDMETHODIMP GetForeground(_Out_ CLSID *pclsid) override;
    STDMETHODIMP TestKeyDown(
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _Out_ BOOL *pfEaten) override;
    STDMETHODIMP TestKeyUp(
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _Out_ BOOL *pfEaten) override;
    STDMETHODIMP KeyDown(
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _Out_ BOOL *pfEaten) override;
    STDMETHODIMP KeyUp(
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _Out_ BOOL *pfEaten) override;
    STDMETHODIMP GetPreservedKey(
        _In_ ITfContext *pic,
        _In_ const TF_PRESERVEDKEY *pprekey,
        _Out_ GUID *pguid) override;
    STDMETHODIMP IsPreservedKey(
        _In_ REFGUID rguid,
        _In_ const TF_PRESERVEDKEY *pprekey,
        _Out_ BOOL *pfRegistered) override;
    STDMETHODIMP PreserveKey(
        _In_ TfClientId tid,
        _In_ REFGUID rguid,
        _In_ const TF_PRESERVEDKEY *prekey,
        _In_ const WCHAR *pchDesc,
        _In_ ULONG cchDesc) override;
    STDMETHODIMP UnpreserveKey(
        _In_ REFGUID rguid,
        _In_ const TF_PRESERVEDKEY *pprekey) override;
    STDMETHODIMP SetPreservedKeyDescription(
        _In_ REFGUID rguid,
        _In_ const WCHAR *pchDesc,
        _In_ ULONG cchDesc) override;
    STDMETHODIMP GetPreservedKeyDescription(
        _In_ REFGUID rguid,
        _Out_ BSTR *pbstrDesc) override;
    STDMETHODIMP SimulatePreservedKey(
        _In_ ITfContext *pic,
        _In_ REFGUID rguid,
        _Out_ BOOL *pfEaten) override;

    // ** ITfMessagePump methods **
    STDMETHODIMP PeekMessageA(
        _Out_ LPMSG pMsg,
        _In_ HWND hwnd,
        _In_ UINT wMsgFilterMin,
        _In_ UINT wMsgFilterMax,
        _In_ UINT wRemoveMsg,
        _Out_ BOOL *pfResult) override;
    STDMETHODIMP GetMessageA(
        _Out_ LPMSG pMsg,
        _In_ HWND hwnd,
        _In_ UINT wMsgFilterMin,
        _In_ UINT wMsgFilterMax,
        _Out_ BOOL *pfResult) override;
    STDMETHODIMP PeekMessageW(
        _Out_ LPMSG pMsg,
        _In_ HWND hwnd,
        _In_ UINT wMsgFilterMin,
        _In_ UINT wMsgFilterMax,
        _In_ UINT wRemoveMsg,
        _Out_ BOOL *pfResult) override;
    STDMETHODIMP GetMessageW(
        _Out_ LPMSG pMsg,
        _In_ HWND hwnd,
        _In_ UINT wMsgFilterMin,
        _In_ UINT wMsgFilterMax,
        _Out_ BOOL *pfResult) override;

    // ** ITfClientId methods **
    STDMETHODIMP GetClientId(
        _In_ REFCLSID rclsid,
        _Out_ TfClientId *ptid) override;

    // ** ITfUIElementMgr methods **
    STDMETHODIMP BeginUIElement(
        _In_ ITfUIElement *element,
        _Inout_ BOOL *show,
        _Out_ DWORD *id) override;
    STDMETHODIMP UpdateUIElement(_In_ DWORD id) override;
    STDMETHODIMP EndUIElement(_In_ DWORD id) override;
    STDMETHODIMP GetUIElement(
        _In_ DWORD id,
        _Out_ ITfUIElement **element) override;
    STDMETHODIMP EnumUIElements(_Out_ IEnumTfUIElements **enum_elements) override;

    // ** ITfSourceSingle methods **
    STDMETHODIMP AdviseSingleSink(
        _In_ TfClientId tid,
        _In_ REFIID riid,
        _In_ IUnknown *punk) override;
    STDMETHODIMP UnadviseSingleSink(
        _In_ TfClientId tid,
        _In_ REFIID riid) override;

    // ** ITfThreadMgrEventSink methods **
    STDMETHODIMP OnInitDocumentMgr(_In_ ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnUninitDocumentMgr(_In_ ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnSetFocus(
        _In_ ITfDocumentMgr *pdimFocus,
        _In_ ITfDocumentMgr *pdimPrevFocus) override;
    STDMETHODIMP OnPushContext(_In_ ITfContext *pic) override;
    STDMETHODIMP OnPopContext(_In_ ITfContext *pic) override;

protected:
    LONG m_cRefs;

    /* Aggregation */
    ITfCompartmentMgr *m_CompartmentMgr;

    ITfDocumentMgr *m_focus;
    LONG m_activationCount;

    ITfKeyEventSink *m_foregroundKeyEventSink;
    CLSID m_foregroundTextService;

    struct list m_CurrentPreservedKeys;
    struct list m_CreatedDocumentMgrs;

    struct list m_AssociatedFocusWindows;
    HHOOK  m_focusHook;

    /* kept as separate lists to reduce unnecessary iterations */
    struct list m_ActiveLanguageProfileNotifySink;
    struct list m_DisplayAttributeNotifySink;
    struct list m_KeyTraceEventSink;
    struct list m_PreservedKeyNotifySink;
    struct list m_ThreadFocusSink;
    struct list m_ThreadMgrEventSink;
    struct list m_UIElementSink;
    struct list m_InputProcessorProfileActivationSink;

    static LRESULT CALLBACK ThreadFocusHookProc(INT nCode, WPARAM wParam, LPARAM lParam);
    LRESULT _ThreadFocusHookProc(INT nCode, WPARAM wParam, LPARAM lParam);

    HRESULT SetupWindowsHook();
};

////////////////////////////////////////////////////////////////////////////

class CEnumTfDocumentMgr
    : public IEnumTfDocumentMgrs
{
public:
    CEnumTfDocumentMgr();
    virtual ~CEnumTfDocumentMgr();

    static HRESULT CreateInstance(struct list* head, CEnumTfDocumentMgr **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumTfDocumentMgrs methods **
    STDMETHODIMP Clone(_Out_ IEnumTfDocumentMgrs **ppEnum) override;
    STDMETHODIMP Next(
        _In_ ULONG ulCount,
        _Out_ ITfDocumentMgr **rgDocumentMgr,
        _Out_ ULONG *pcFetched) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Skip(_In_ ULONG ulCount) override;

protected:
    LONG m_cRefs;
    struct list *m_index;
    struct list *m_head;
};

////////////////////////////////////////////////////////////////////////////

CThreadMgr::CThreadMgr()
    : m_cRefs(1)
    , m_CompartmentMgr(NULL)
    , m_focus(NULL)
    , m_activationCount(0)
    , m_foregroundKeyEventSink(NULL)
{
    m_foregroundTextService = GUID_NULL;

    list_init(&m_CurrentPreservedKeys);
    list_init(&m_CreatedDocumentMgrs);

    list_init(&m_AssociatedFocusWindows);
    m_focusHook = NULL;

    /* kept as separate lists to reduce unnecessary iterations */
    list_init(&m_ActiveLanguageProfileNotifySink);
    list_init(&m_DisplayAttributeNotifySink);
    list_init(&m_KeyTraceEventSink);
    list_init(&m_PreservedKeyNotifySink);
    list_init(&m_ThreadFocusSink);
    list_init(&m_ThreadMgrEventSink);
    list_init(&m_UIElementSink);
    list_init(&m_InputProcessorProfileActivationSink);
}

CThreadMgr::~CThreadMgr()
{
    struct list *cursor, *cursor2;

    /* unhook right away */
    if (m_focusHook)
        UnhookWindowsHookEx(m_focusHook);

    TlsSetValue(g_dwTLSIndex, NULL);
    TRACE("destroying %p\n", this);

    if (m_focus)
        m_focus->Release();

    free_sinks(&m_ActiveLanguageProfileNotifySink);
    free_sinks(&m_DisplayAttributeNotifySink);
    free_sinks(&m_KeyTraceEventSink);
    free_sinks(&m_PreservedKeyNotifySink);
    free_sinks(&m_ThreadFocusSink);
    free_sinks(&m_ThreadMgrEventSink);
    free_sinks(&m_UIElementSink);
    free_sinks(&m_InputProcessorProfileActivationSink);

    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor, PreservedKey, entry);
        list_remove(cursor);
        cicMemFree(key->description);
        cicMemFree(key);
    }

    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_CreatedDocumentMgrs)
    {
        DocumentMgrEntry *mgr = LIST_ENTRY(cursor, DocumentMgrEntry, entry);
        list_remove(cursor);
        FIXME("Left Over ITfDocumentMgr.  Should we do something with it?\n");
        cicMemFree(mgr);
    }

    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_AssociatedFocusWindows)
    {
        AssociatedWindow *wnd = LIST_ENTRY(cursor, AssociatedWindow, entry);
        list_remove(cursor);
        cicMemFree(wnd);
    }

    m_CompartmentMgr->Release();
}

STDMETHODIMP CThreadMgr::QueryInterface(REFIID iid, LPVOID *ppvObject)
{
    *ppvObject = NULL;

    IUnknown *pUnk = NULL;
    if (iid == IID_IUnknown || iid == IID_ITfThreadMgr || iid == IID_ITfThreadMgrEx)
        pUnk = static_cast<ITfThreadMgrEx *>(this);
    else if (iid == IID_ITfSource)
        pUnk = static_cast<ITfSource *>(this);
    else if (iid == IID_ITfKeystrokeMgr)
        pUnk = static_cast<ITfKeystrokeMgr *>(this);
    else if (iid == IID_ITfMessagePump)
        pUnk = static_cast<ITfMessagePump *>(this);
    else if (iid == IID_ITfClientId)
        pUnk = static_cast<ITfClientId *>(this);
    else if (iid == IID_ITfCompartmentMgr)
        pUnk = m_CompartmentMgr;
    else if (iid == IID_ITfUIElementMgr)
        pUnk = static_cast<ITfUIElementMgr *>(this);
    else if (iid == IID_ITfSourceSingle)
        pUnk = static_cast<ITfSourceSingle *>(this);

    if (pUnk)
    {
        *ppvObject = pUnk;
        pUnk->AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CThreadMgr::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CThreadMgr::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CThreadMgr::Activate(_Out_ TfClientId *ptid)
{
    TRACE("(%p) %p\n", this, ptid);
    return ActivateEx(ptid, 0);
}

STDMETHODIMP CThreadMgr::Deactivate()
{
    TRACE("(%p)\n", this);

    if (m_activationCount == 0)
        return E_UNEXPECTED;

    --m_activationCount;

    if (m_activationCount == 0)
    {
        if (m_focus)
        {
            OnSetFocus(NULL, m_focus);
            m_focus->Release();
            m_focus = NULL;
        }
    }

    deactivate_textservices();
    return S_OK;
}

STDMETHODIMP CThreadMgr::CreateDocumentMgr(_Out_ ITfDocumentMgr **ppdim)
{
    TRACE("(%p)\n", this);

    if (!ppdim)
        return E_INVALIDARG;

    DocumentMgrEntry *mgrentry = (DocumentMgrEntry *)cicMemAlloc(sizeof(DocumentMgrEntry));
    if (!mgrentry)
        return E_OUTOFMEMORY;

    HRESULT hr = DocumentMgr_Constructor(this, ppdim);
    if (SUCCEEDED(hr))
    {
        mgrentry->docmgr = *ppdim;
        list_add_head(&m_CreatedDocumentMgrs, &mgrentry->entry);
    }
    else
    {
        cicMemFree(mgrentry);
    }

    return hr;
}

STDMETHODIMP CThreadMgr::EnumDocumentMgrs(_Out_ IEnumTfDocumentMgrs **ppEnum)
{
    TRACE("(%p) %p\n", this, ppEnum);

    if (!ppEnum)
        return E_INVALIDARG;

    return CEnumTfDocumentMgr::CreateInstance(&m_CreatedDocumentMgrs, (CEnumTfDocumentMgr **)ppEnum);
}

STDMETHODIMP CThreadMgr::GetFocus(_Out_ ITfDocumentMgr **ppdimFocus)
{
    TRACE("(%p)\n", this);

    if (!ppdimFocus)
        return E_INVALIDARG;

    *ppdimFocus = m_focus;

    TRACE("->%p\n", m_focus);

    if (!m_focus)
        return S_FALSE;

    m_focus->AddRef();
    return S_OK;
}

STDMETHODIMP CThreadMgr::SetFocus(_In_ ITfDocumentMgr *pdimFocus)
{
    ITfDocumentMgr *check;

    TRACE("(%p) %p\n", this, pdimFocus);

    if (!pdimFocus)
        check = NULL;
    else if (FAILED(pdimFocus->QueryInterface(IID_ITfDocumentMgr, (LPVOID*)&check)))
        return E_INVALIDARG;

    OnSetFocus(check, m_focus);

    if (m_focus)
        m_focus->Release();

    m_focus = check;
    return S_OK;
}

LRESULT CThreadMgr::_ThreadFocusHookProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (!m_focusHook)
    {
        ERR("Hook proc but no ThreadMgr focus Hook. Serious Error\n");
        return 0;
    }

    if (nCode == HCBT_SETFOCUS) /* focus change within our thread */
    {
        struct list *cursor;

        LIST_FOR_EACH(cursor, &m_AssociatedFocusWindows)
        {
            AssociatedWindow *wnd = LIST_ENTRY(cursor, AssociatedWindow, entry);
            if (wnd->hwnd == (HWND)wParam)
            {
                TRACE("Triggering Associated window focus\n");
                if (m_focus != wnd->docmgr)
                    SetFocus(wnd->docmgr);
                break;
            }
        }
    }

    return CallNextHookEx(m_focusHook, nCode, wParam, lParam);
}

LRESULT CALLBACK CThreadMgr::ThreadFocusHookProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    CThreadMgr *This = (CThreadMgr *)TlsGetValue(g_dwTLSIndex);
    if (!This)
    {
        ERR("Hook proc but no ThreadMgr for this thread. Serious Error\n");
        return 0;
    }
    return This->_ThreadFocusHookProc(nCode, wParam, lParam);
}

HRESULT CThreadMgr::SetupWindowsHook()
{
    if (!m_focusHook)
    {
        m_focusHook = SetWindowsHookExW(WH_CBT, ThreadFocusHookProc, 0, GetCurrentThreadId());
        if (!m_focusHook)
        {
            ERR("Unable to set focus hook\n");
            return E_FAIL;
        }
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CThreadMgr::AssociateFocus(
    _In_ HWND hwnd,
    _In_ ITfDocumentMgr *pdimNew,
    _Out_ ITfDocumentMgr **ppdimPrev)
{
    struct list *cursor, *cursor2;
    AssociatedWindow *wnd;

    TRACE("(%p) %p %p %p\n", this, hwnd, pdimNew, ppdimPrev);

    if (!ppdimPrev)
        return E_INVALIDARG;

    *ppdimPrev = NULL;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_AssociatedFocusWindows)
    {
        wnd = LIST_ENTRY(cursor, AssociatedWindow, entry);
        if (wnd->hwnd == hwnd)
        {
            if (wnd->docmgr)
                wnd->docmgr->AddRef();
            *ppdimPrev = wnd->docmgr;
            wnd->docmgr = pdimNew;
            if (::GetFocus() == hwnd)
                SetFocus(pdimNew);
            return S_OK;
        }
    }

    wnd = (AssociatedWindow *)cicMemAlloc(sizeof(AssociatedWindow));
    wnd->hwnd = hwnd;
    wnd->docmgr = pdimNew;
    list_add_head(&m_AssociatedFocusWindows, &wnd->entry);

    if (::GetFocus() == hwnd)
        SetFocus(pdimNew);

    this->SetupWindowsHook();
    return S_OK;
}

STDMETHODIMP CThreadMgr::IsThreadFocus(_Out_ BOOL *pfThreadFocus)
{
    TRACE("(%p) %p\n", this, pfThreadFocus);

    if (!pfThreadFocus)
        return E_INVALIDARG;

    HWND focus = ::GetFocus();
    *pfThreadFocus = !focus;
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetFunctionProvider(
    _In_ REFCLSID clsid,
    _Out_ ITfFunctionProvider **ppFuncProv)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::EnumFunctionProviders(_Out_ IEnumTfFunctionProviders **ppEnum)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::GetGlobalCompartment(_Out_ ITfCompartmentMgr **ppCompMgr)
{
    HRESULT hr;
    TRACE("(%p) %p\n", this, ppCompMgr);

    if (!ppCompMgr)
        return E_INVALIDARG;

    if (!g_globalCompartmentMgr)
    {
        hr = CompartmentMgr_Constructor(NULL, IID_ITfCompartmentMgr, (IUnknown **)&g_globalCompartmentMgr);
        if (FAILED(hr))
            return hr;
    }

    g_globalCompartmentMgr->AddRef();
    *ppCompMgr = g_globalCompartmentMgr;
    return S_OK;
}

STDMETHODIMP CThreadMgr::ActivateEx(
    _Out_ TfClientId *id,
    _In_ DWORD flags)
{
    TRACE("(%p) %p, %#x\n", this, id, flags);

    if (!id)
        return E_INVALIDARG;

    if (flags)
        FIXME("Unimplemented flags %#x\n", flags);

    if (!g_processId)
    {
        GUID guid;
        CoCreateGuid(&guid);
        GetClientId(guid, &g_processId);
    }

    activate_textservices(this);
    ++m_activationCount;
    *id = g_processId;
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetActiveFlags(_Out_ DWORD *flags)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::AdviseSink(
    _In_ REFIID riid,
    _In_ IUnknown *punk,
    _Out_ DWORD *pdwCookie)
{
    TRACE("(%p) %s %p %p\n", this, debugstr_guid(&riid), punk, pdwCookie);

    if (cicIsNullPtr(&riid) || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (riid == IID_ITfThreadMgrEventSink)
        return advise_sink(&m_ThreadMgrEventSink, IID_ITfThreadMgrEventSink, COOKIE_MAGIC_TMSINK, punk, pdwCookie);

    if (riid == IID_ITfThreadFocusSink)
    {
        WARN("semi-stub for ITfThreadFocusSink: sink won't be used.\n");
        return advise_sink(&m_ThreadFocusSink, IID_ITfThreadFocusSink, COOKIE_MAGIC_THREADFOCUSSINK, punk, pdwCookie);
    }

    if (riid == IID_ITfActiveLanguageProfileNotifySink)
    {
        WARN("semi-stub for ITfActiveLanguageProfileNotifySink: sink won't be used.\n");
        return advise_sink(&m_ActiveLanguageProfileNotifySink, IID_ITfActiveLanguageProfileNotifySink,
                           COOKIE_MAGIC_ACTIVELANGSINK, punk, pdwCookie);
    }

    if (riid == IID_ITfKeyTraceEventSink)
    {
        WARN("semi-stub for ITfKeyTraceEventSink: sink won't be used.\n");
        return advise_sink(&m_KeyTraceEventSink, IID_ITfKeyTraceEventSink,
                           COOKIE_MAGIC_KEYTRACESINK, punk, pdwCookie);
    }

    if (riid == IID_ITfUIElementSink)
    {
        WARN("semi-stub for ITfUIElementSink: sink won't be used.\n");
        return advise_sink(&m_UIElementSink, IID_ITfUIElementSink,
                           COOKIE_MAGIC_UIELEMENTSINK, punk, pdwCookie);
    }

    if (riid == IID_ITfInputProcessorProfileActivationSink)
    {
        WARN("semi-stub for ITfInputProcessorProfileActivationSink: sink won't be used.\n");
        return advise_sink(&m_InputProcessorProfileActivationSink, IID_ITfInputProcessorProfileActivationSink,
                           COOKIE_MAGIC_INPUTPROCESSORPROFILEACTIVATIONSINK, punk, pdwCookie);
    }

    FIXME("(%p) Unhandled Sink: %s\n", this, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::UnadviseSink(_In_ DWORD dwCookie)
{
    DWORD magic;

    TRACE("(%p) %x\n", this, dwCookie);

    magic = get_Cookie_magic(dwCookie);
    if (magic != COOKIE_MAGIC_TMSINK &&
        magic != COOKIE_MAGIC_THREADFOCUSSINK &&
        magic != COOKIE_MAGIC_KEYTRACESINK &&
        magic != COOKIE_MAGIC_UIELEMENTSINK &&
        magic != COOKIE_MAGIC_INPUTPROCESSORPROFILEACTIVATIONSINK)
    {
        return E_INVALIDARG;
    }

    return unadvise_sink(dwCookie);
}

STDMETHODIMP CThreadMgr::AdviseKeyEventSink(
    _In_ TfClientId tid,
    _In_ ITfKeyEventSink *pSink,
    _In_ BOOL fForeground)
{
    CLSID textservice;
    ITfKeyEventSink *check = NULL;

    TRACE("(%p) %x %p %i\n", this, tid, pSink, fForeground);

    if (!tid || !pSink)
        return E_INVALIDARG;

    textservice = get_textservice_clsid(tid);
    if (GUID_NULL == textservice)
        return E_INVALIDARG;

    get_textservice_sink(tid, IID_ITfKeyEventSink, (IUnknown **)&check);
    if (check)
        return CONNECT_E_ADVISELIMIT;

    if (FAILED(pSink->QueryInterface(IID_ITfKeyEventSink, (LPVOID*)&check)))
        return E_INVALIDARG;

    set_textservice_sink(tid, IID_ITfKeyEventSink, check);

    if (fForeground)
    {
        if (m_foregroundKeyEventSink)
        {
            m_foregroundKeyEventSink->OnSetFocus(FALSE);
            m_foregroundKeyEventSink->Release();
        }
        check->AddRef();
        check->OnSetFocus(TRUE);
        m_foregroundKeyEventSink = check;
        m_foregroundTextService = textservice;
    }
    return S_OK;
}

STDMETHODIMP CThreadMgr::UnadviseKeyEventSink(_In_ TfClientId tid)
{
    CLSID textservice;
    ITfKeyEventSink *check = NULL;
    TRACE("(%p) %x\n", this, tid);

    if (!tid)
        return E_INVALIDARG;

    textservice = get_textservice_clsid(tid);
    if (GUID_NULL == textservice)
        return E_INVALIDARG;

    get_textservice_sink(tid, IID_ITfKeyEventSink, (IUnknown **)&check);

    if (!check)
        return CONNECT_E_NOCONNECTION;

    set_textservice_sink(tid, IID_ITfKeyEventSink, NULL);
    check->Release();

    if (m_foregroundKeyEventSink == check)
    {
        m_foregroundKeyEventSink->Release();
        m_foregroundKeyEventSink = NULL;
        m_foregroundTextService = GUID_NULL;
    }
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetForeground(_Out_ CLSID *pclsid)
{
    TRACE("(%p) %p\n", this, pclsid);
    if (!pclsid)
        return E_INVALIDARG;

    if (m_foregroundTextService == GUID_NULL)
        return S_FALSE;

    *pclsid = m_foregroundTextService;
    return S_OK;
}

STDMETHODIMP CThreadMgr::TestKeyDown(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ BOOL *pfEaten)
{
    FIXME("STUB:(%p)\n", this);
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CThreadMgr::TestKeyUp(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ BOOL *pfEaten)
{
    FIXME("STUB:(%p)\n", this);
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CThreadMgr::KeyDown(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ BOOL *pfEaten)
{
    FIXME("STUB:(%p)\n", this);
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::KeyUp(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ BOOL *pfEaten)
{
    FIXME("STUB:(%p)\n", this);
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::GetPreservedKey(
    _In_ ITfContext *pic,
    _In_ const TF_PRESERVEDKEY *pprekey,
    _Out_ GUID *pguid)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::IsPreservedKey(
    _In_ REFGUID rguid,
    _In_ const TF_PRESERVEDKEY *pprekey,
    _Out_ BOOL *pfRegistered)
{
    struct list *cursor;

    TRACE("(%p) %s (%x %x) %p\n", this, debugstr_guid(&rguid), (pprekey ? pprekey->uVKey : 0), (pprekey ? pprekey->uModifiers : 0), pfRegistered);

    if (cicIsNullPtr(&rguid) || !pprekey || !pfRegistered)
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &m_CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor, PreservedKey, entry);
        if (rguid == key->guid && pprekey->uVKey == key->prekey.uVKey && pprekey->uModifiers == key->prekey.uModifiers)
        {
            *pfRegistered = TRUE;
            return S_OK;
        }
    }

    *pfRegistered = FALSE;
    return S_FALSE;
}

STDMETHODIMP CThreadMgr::PreserveKey(
    _In_ TfClientId tid,
    _In_ REFGUID rguid,
    _In_ const TF_PRESERVEDKEY *prekey,
    _In_ const WCHAR *pchDesc,
    _In_ ULONG cchDesc)
{
    struct list *cursor;

    TRACE("(%p) %x %s (%x,%x) %s\n", this, tid, debugstr_guid(&rguid), (prekey ? prekey->uVKey : 0), (prekey ? prekey->uModifiers : 0), debugstr_wn(pchDesc, cchDesc));

    if (!tid || cicIsNullPtr(&rguid) || !prekey || (cchDesc && !pchDesc))
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &m_CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor, PreservedKey, entry);
        if (rguid == key->guid && prekey->uVKey == key->prekey.uVKey && prekey->uModifiers == key->prekey.uModifiers)
            return TF_E_ALREADY_EXISTS;
    }

    PreservedKey *newkey = (PreservedKey *)cicMemAlloc(sizeof(PreservedKey));
    if (!newkey)
        return E_OUTOFMEMORY;

    newkey->guid  = rguid;
    newkey->prekey = *prekey;
    newkey->tid = tid;
    newkey->description = NULL;
    if (cchDesc)
    {
        newkey->description = (LPWSTR)cicMemAlloc((cchDesc + 1) * sizeof(WCHAR));
        if (!newkey->description)
        {
            cicMemFree(newkey);
            return E_OUTOFMEMORY;
        }
        CopyMemory(newkey->description, pchDesc, cchDesc * sizeof(WCHAR));
        newkey->description[cchDesc] = UNICODE_NULL;
    }

    list_add_head(&m_CurrentPreservedKeys, &newkey->entry);
    return S_OK;
}

STDMETHODIMP CThreadMgr::UnpreserveKey(
    _In_ REFGUID rguid,
    _In_ const TF_PRESERVEDKEY *pprekey)
{
    PreservedKey* key = NULL;
    struct list *cursor;
    TRACE("(%p) %s (%x %x)\n", this, debugstr_guid(&rguid), (pprekey ? pprekey->uVKey : 0), (pprekey ? pprekey->uModifiers : 0));

    if (!pprekey || cicIsNullPtr(&rguid))
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &m_CurrentPreservedKeys)
    {
        key = LIST_ENTRY(cursor, PreservedKey, entry);
        if (rguid == key->guid && pprekey->uVKey == key->prekey.uVKey && pprekey->uModifiers == key->prekey.uModifiers)
            break;
        key = NULL;
    }

    if (!key)
        return CONNECT_E_NOCONNECTION;

    list_remove(&key->entry);
    cicMemFree(key->description);
    cicMemFree(key);

    return S_OK;
}

STDMETHODIMP CThreadMgr::SetPreservedKeyDescription(
    _In_ REFGUID rguid,
    _In_ const WCHAR *pchDesc,
    _In_ ULONG cchDesc)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::GetPreservedKeyDescription(
    _In_ REFGUID rguid,
    _Out_ BSTR *pbstrDesc)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::SimulatePreservedKey(
    _In_ ITfContext *pic,
    _In_ REFGUID rguid,
    _Out_ BOOL *pfEaten)
{
    FIXME("STUB:(%p)\n", this);
    if (!pfEaten)
        return E_INVALIDARG;
    *pfEaten = FALSE;
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::PeekMessageA(
    _Out_ LPMSG pMsg,
    _In_ HWND hwnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _In_ UINT wRemoveMsg,
    _Out_ BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = ::PeekMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetMessageA(
    _Out_ LPMSG pMsg,
    _In_ HWND hwnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _Out_ BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = ::GetMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);
    return S_OK;
}

STDMETHODIMP CThreadMgr::PeekMessageW(
    _Out_ LPMSG pMsg,
    _In_ HWND hwnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _In_ UINT wRemoveMsg,
    _Out_ BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = ::PeekMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetMessageW(
    _Out_ LPMSG pMsg,
    _In_ HWND hwnd,
    _In_ UINT wMsgFilterMin,
    _In_ UINT wMsgFilterMax,
    _Out_ BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = ::GetMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);
    return S_OK;
}

STDMETHODIMP CThreadMgr::GetClientId(
    _In_ REFCLSID rclsid,
    _Out_ TfClientId *ptid)
{
    HRESULT hr;
    ITfCategoryMgr *catmgr;

    TRACE("(%p) %s\n", this, debugstr_guid(&rclsid));

    CategoryMgr_Constructor(NULL, (IUnknown **)&catmgr);
    hr = catmgr->RegisterGUID(rclsid, ptid);
    catmgr->Release();

    return hr;
}

STDMETHODIMP CThreadMgr::OnInitDocumentMgr(_In_ ITfDocumentMgr *pdim)
{
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n", this, pdim);

    SINK_FOR_EACH(cursor, &m_ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        sink->OnInitDocumentMgr(pdim);
    }

    return S_OK;
}

STDMETHODIMP CThreadMgr::OnUninitDocumentMgr(_In_ ITfDocumentMgr *pdim)
{
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n", this, pdim);

    SINK_FOR_EACH(cursor, &m_ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        sink->OnUninitDocumentMgr(pdim);
    }

    return S_OK;
}

STDMETHODIMP CThreadMgr::OnSetFocus(
    _In_ ITfDocumentMgr *pdimFocus,
    _In_ ITfDocumentMgr *pdimPrevFocus)
{
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p %p\n", this, pdimFocus, pdimPrevFocus);

    SINK_FOR_EACH(cursor, &m_ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        sink->OnSetFocus(pdimFocus, pdimPrevFocus);
    }

    return S_OK;
}

STDMETHODIMP CThreadMgr::OnPushContext(_In_ ITfContext *pic)
{
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n", this, pic);

    SINK_FOR_EACH(cursor, &m_ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        sink->OnPushContext(pic);
    }

    return S_OK;
}

STDMETHODIMP CThreadMgr::OnPopContext(_In_ ITfContext *pic)
{
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n", this, pic);

    SINK_FOR_EACH(cursor, &m_ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        sink->OnPopContext(pic);
    }

    return S_OK;
}

STDMETHODIMP CThreadMgr::BeginUIElement(
    _In_ ITfUIElement *element,
    _Inout_ BOOL *show,
    _Out_ DWORD *id)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::UpdateUIElement(_In_ DWORD id)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::EndUIElement(_In_ DWORD id)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::GetUIElement(
    _In_ DWORD id,
    _Out_ ITfUIElement **element)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::EnumUIElements(_Out_ IEnumTfUIElements **enum_elements)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::AdviseSingleSink(
    _In_ TfClientId tid,
    _In_ REFIID riid,
    _In_ IUnknown *punk)
{
    FIXME("STUB:(%p) %i %s %p\n", this, tid, debugstr_guid(&riid), punk);
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgr::UnadviseSingleSink(
    _In_ TfClientId tid,
    _In_ REFIID riid)
{
    FIXME("STUB:(%p) %i %s\n", this, tid, debugstr_guid(&riid));
    return E_NOTIMPL;
}

HRESULT CThreadMgr::CreateInstance(IUnknown *pUnkOuter, CThreadMgr **ppOut)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Only 1 ThreadMgr is created per thread */
    CThreadMgr *This = (CThreadMgr *)TlsGetValue(g_dwTLSIndex);
    if (This)
    {
        This->AddRef();
        *ppOut = This;
        return S_OK;
    }

    This = new(cicNoThrow) CThreadMgr();
    if (!This)
        return E_OUTOFMEMORY;

    TlsSetValue(g_dwTLSIndex, This);

    ITfCompartmentMgr *pCompMgr = NULL;
    CompartmentMgr_Constructor(static_cast<ITfThreadMgrEx *>(This), IID_IUnknown, (IUnknown **)&pCompMgr);
    This->m_CompartmentMgr = pCompMgr;

    TRACE("returning %p\n", This);
    *ppOut = This;
    return S_OK;
}

void CThreadMgr::OnDocumentMgrDestruction(ITfDocumentMgr *mgr)
{
    struct list *cursor;
    LIST_FOR_EACH(cursor, &m_CreatedDocumentMgrs)
    {
        DocumentMgrEntry *mgrentry = LIST_ENTRY(cursor, DocumentMgrEntry, entry);
        if (mgrentry->docmgr == mgr)
        {
            list_remove(cursor);
            cicMemFree(mgrentry);
            return;
        }
    }
    FIXME("ITfDocumentMgr %p not found in this thread\n", mgr);
}

////////////////////////////////////////////////////////////////////////////

CEnumTfDocumentMgr::CEnumTfDocumentMgr()
    : m_cRefs(1)
    , m_index(NULL)
    , m_head(NULL)
{
}

CEnumTfDocumentMgr::~CEnumTfDocumentMgr()
{
    TRACE("destroying %p\n", this);
}

STDMETHODIMP CEnumTfDocumentMgr::QueryInterface(REFIID iid, LPVOID *ppvObject)
{
    *ppvObject = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumTfDocumentMgrs)
        *ppvObject = static_cast<IEnumTfDocumentMgrs *>(this);

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumTfDocumentMgr::AddRef()
{
    return InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumTfDocumentMgr::Release()
{
    ULONG ret = InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CEnumTfDocumentMgr::Clone(_Out_ IEnumTfDocumentMgrs **ppEnum)
{
    TRACE("(%p)\n", this);

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = NULL;

    CEnumTfDocumentMgr *cloned;
    HRESULT hr = CEnumTfDocumentMgr::CreateInstance(m_head, &cloned);
    if (SUCCEEDED(hr))
    {
        *ppEnum = static_cast<IEnumTfDocumentMgrs *>(cloned);
        cloned->m_index = m_index;
    }

    return hr;
}

STDMETHODIMP CEnumTfDocumentMgr::Next(
    _In_ ULONG ulCount,
    _Out_ ITfDocumentMgr **rgDocumentMgr,
    _Out_ ULONG *pcFetched)
{
    ULONG fetched = 0;

    TRACE("(%p)\n", this);

    if (!rgDocumentMgr)
        return E_POINTER;

    while (fetched < ulCount)
    {
        DocumentMgrEntry *mgrentry;
        if (!m_index)
            break;

        mgrentry = LIST_ENTRY(m_index, DocumentMgrEntry, entry);
        if (!mgrentry)
            break;

        *rgDocumentMgr = mgrentry->docmgr;
        (*rgDocumentMgr)->AddRef();

        m_index = list_next(m_head, m_index);
        ++fetched;
        ++rgDocumentMgr;
    }

    if (pcFetched)
        *pcFetched = fetched;
    return fetched == ulCount ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumTfDocumentMgr::Reset()
{
    TRACE("(%p)\n", this);
    m_index = list_head(m_head);
    return S_OK;
}

STDMETHODIMP CEnumTfDocumentMgr::Skip(_In_ ULONG ulCount)
{
    TRACE("(%p)\n", this);
    for (ULONG i = 0; i < ulCount && m_index; ++i)
        m_index = list_next(m_head, m_index);
    return S_OK;
}

HRESULT CEnumTfDocumentMgr::CreateInstance(struct list* head, CEnumTfDocumentMgr **ppOut)
{
    CEnumTfDocumentMgr *This = new(cicNoThrow) CEnumTfDocumentMgr();
    if (!This)
        return E_OUTOFMEMORY;

    This->m_head = head;
    This->m_index = list_head(This->m_head);

    *ppOut = This;
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    return CThreadMgr::CreateInstance(pUnkOuter, (CThreadMgr **)ppOut);
}

EXTERN_C
void ThreadMgr_OnDocumentMgrDestruction(ITfThreadMgr *iface, ITfDocumentMgr *mgr)
{
    CThreadMgr *This = static_cast<CThreadMgr *>(iface);
    This->OnDocumentMgrDestruction(mgr);
}
