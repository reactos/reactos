/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implementation of ITfDocumentMgr and IEnumTfContexts
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "documentmgr.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

CDocumentMgr::CDocumentMgr(ITfThreadMgrEventSink *threadMgrSink)
    : m_cRefs(1)
    , m_pCompartmentMgr(NULL)
    , m_initialContext(NULL)
    , m_pThreadMgrSink(threadMgrSink)
{
    m_contextStack[1] = m_contextStack[0] = NULL;

    list_init(&m_transitoryExtensionSink);

    ITfDocumentMgr *pDocMgr = static_cast<ITfDocumentMgr *>(this);
    ITfCompartmentMgr **ppCompMgr = static_cast<ITfCompartmentMgr **>(&m_pCompartmentMgr);
    CompartmentMgr_Constructor(pDocMgr, IID_IUnknown, reinterpret_cast<IUnknown **>(ppCompMgr));

    DWORD cookie;
    Context_Constructor(g_processId, NULL, pDocMgr, &m_initialContext, &cookie);
}

CDocumentMgr::~CDocumentMgr()
{
    TRACE("destroying %p\n", this);

    ITfThreadMgr *tm = NULL;
    TF_GetThreadMgr(&tm);
    if (tm)
    {
        ThreadMgr_OnDocumentMgrDestruction(tm, static_cast<ITfDocumentMgr*>(this));
        tm->Release();
    }

    if (m_initialContext)
        m_initialContext->Release();
    if (m_contextStack[0])
        m_contextStack[0]->Release();
    if (m_contextStack[1])
        m_contextStack[1]->Release();

    free_sinks(&m_transitoryExtensionSink);

    if (m_pCompartmentMgr)
    {
        m_pCompartmentMgr->Release();
        m_pCompartmentMgr = NULL;
    }
}

HRESULT
CDocumentMgr::CreateInstance(
    _In_ ITfThreadMgrEventSink *pThreadMgrSink,
    _Out_ ITfDocumentMgr **ppOut)
{
    if (!ppOut)
    {
        ERR("!ppOut\n");
        return E_POINTER;
    }

    if (!pThreadMgrSink)
    {
        ERR("!pThreadMgrSink\n");
        return E_INVALIDARG;
    }

    CDocumentMgr *This = new(cicNoThrow) CDocumentMgr(pThreadMgrSink);
    if (!This)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    *ppOut = static_cast<ITfDocumentMgr *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

STDMETHODIMP CDocumentMgr::QueryInterface(REFIID iid, LPVOID *ppvObject)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&iid), ppvObject);
    *ppvObject = NULL;

    IUnknown *pUnk = NULL;
    if (iid == IID_IUnknown || iid == IID_ITfDocumentMgr)
        pUnk = static_cast<ITfDocumentMgr *>(this);
    else if (iid == IID_ITfSource)
        pUnk = static_cast<ITfSource *>(this);
    else if (iid == IID_ITfCompartmentMgr)
        pUnk = m_pCompartmentMgr;

    if (pUnk)
    {
        pUnk->AddRef();
        *ppvObject = pUnk;
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDocumentMgr::AddRef()
{
    TRACE("%p -> ()\n", this);
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CDocumentMgr::Release()
{
    TRACE("%p -> ()\n", this);
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP
CDocumentMgr::CreateContext(
    TfClientId tidOwner,
    DWORD dwFlags,
    IUnknown *punk,
    ITfContext **ppic,
    TfEditCookie *pecTextStore)
{
    TRACE("%p -> (%d, 0x%lX, %p, %p, %p)\n", this, tidOwner, dwFlags, punk, ppic, pecTextStore);
    return Context_Constructor(tidOwner, punk, this, ppic, pecTextStore);
}

STDMETHODIMP CDocumentMgr::Push(ITfContext *pic)
{
    TRACE("%p -> (%p)\n", this, pic);

    if (m_contextStack[1]) /* Full */
    {
        ERR("TF_E_STACKFULL\n");
        return TF_E_STACKFULL;
    }

    if (!pic)
    {
        ERR("!pic\n");
        return E_INVALIDARG;
    }

    ITfContext *check;
    HRESULT hr = pic->QueryInterface(IID_ITfContext, reinterpret_cast<LPVOID *>(&check));
    if (FAILED(hr))
    {
        ERR("hr: 0x%lX\n", hr);
        return E_INVALIDARG;
    }

    if (!m_contextStack[0])
        m_pThreadMgrSink->OnInitDocumentMgr(this);

    m_contextStack[1] = m_contextStack[0];
    m_contextStack[0] = check;

    Context_Initialize(check, this);
    m_pThreadMgrSink->OnPushContext(check);

    return S_OK;
}

STDMETHODIMP CDocumentMgr::Pop(DWORD dwFlags)
{
    TRACE("%p -> (0x%lX)\n", this, dwFlags);

    if (dwFlags == TF_POPF_ALL)
    {
        for (SIZE_T i = 0; i < _countof(m_contextStack); i++)
        {
            if (!m_contextStack[i])
                continue;

            m_pThreadMgrSink->OnPopContext(m_contextStack[i]);
            Context_Uninitialize(m_contextStack[i]);
            m_contextStack[i]->Release();
            m_contextStack[i] = NULL;
        }

        m_pThreadMgrSink->OnUninitDocumentMgr(this);
        return S_OK;
    }

    if (dwFlags)
    {
        ERR("E_INVALIDARG: 0x%lX\n", dwFlags);
        return E_INVALIDARG;
    }

    if (!m_contextStack[1]) // Cannot pop last context
    {
        ERR("!m_contextStack[1]\n");
        return E_FAIL;
    }

    m_pThreadMgrSink->OnPopContext(m_contextStack[0]);
    Context_Uninitialize(m_contextStack[0]);

    if (m_contextStack[0])
        m_contextStack[0]->Release();

    m_contextStack[0] = m_contextStack[1];
    m_contextStack[1] = NULL;

    if (!m_contextStack[0])
        m_pThreadMgrSink->OnUninitDocumentMgr(this);

    return S_OK;
}

STDMETHODIMP CDocumentMgr::GetTop(ITfContext **ppic)
{
    TRACE("%p -> (%p)\n", this, ppic);

    if (!ppic)
    {
        ERR("!ppic\n");
        return E_INVALIDARG;
    }

    ITfContext *target;
    if (m_contextStack[0])
        target = m_contextStack[0];
    else
        target = m_initialContext;

    if (target)
        target->AddRef();

    *ppic = target;
    return S_OK;
}

STDMETHODIMP CDocumentMgr::GetBase(ITfContext **ppic)
{
    TRACE("%p -> (%p)\n", this, ppic);

    if (!ppic)
    {
        ERR("!ppic\n");
        return E_INVALIDARG;
    }

    ITfContext *target;
    if (m_contextStack[1])
        target = m_contextStack[1];
    else if (m_contextStack[0])
        target = m_contextStack[0];
    else
        target = m_initialContext;

    if (target)
        target->AddRef();

    *ppic = target;
    return S_OK;
}

STDMETHODIMP CDocumentMgr::EnumContexts(IEnumTfContexts **ppEnum)
{
    TRACE("%p -> (%p)\n", this, ppEnum);
    return EnumTfContext_Constructor(this, ppEnum);
}

STDMETHODIMP CDocumentMgr::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    TRACE("%p -> (%s, %p, %p)\n", this, wine_dbgstr_guid(&riid), punk, pdwCookie);

    if (cicIsNullPtr(&riid) || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (riid == IID_ITfTransitoryExtensionSink)
    {
        WARN("semi-stub for ITfTransitoryExtensionSink: callback won't be used.\n");
        return advise_sink(&m_transitoryExtensionSink, IID_ITfTransitoryExtensionSink,
                           COOKIE_MAGIC_DMSINK, punk, pdwCookie);
    }

    FIXME("(%p) Unhandled Sink: %s\n", this, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CDocumentMgr::UnadviseSink(DWORD pdwCookie)
{
    TRACE("%p -> (%p)\n", this, pdwCookie);

    if (get_Cookie_magic(pdwCookie) != COOKIE_MAGIC_DMSINK)
        return E_INVALIDARG;

    return unadvise_sink(pdwCookie);
}

////////////////////////////////////////////////////////////////////////////

CEnumTfContext::CEnumTfContext(_In_opt_ CDocumentMgr *mgr)
    : m_cRefs(1)
    , m_index(0)
    , m_pDocMgr(mgr)
{
    if (mgr)
        mgr->AddRef();
}

CEnumTfContext::~CEnumTfContext()
{
    if (m_pDocMgr)
    {
        m_pDocMgr->Release();
        m_pDocMgr = NULL;
    }
}

HRESULT CEnumTfContext::CreateInstance(_In_opt_ CDocumentMgr *mgr, _Out_ IEnumTfContexts **ppOut)
{
    if (!ppOut)
    {
        ERR("!ppOut\n");
        return E_POINTER;
    }

    CEnumTfContext *This = new(cicNoThrow) CEnumTfContext(mgr);
    if (!This)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    *ppOut = static_cast<IEnumTfContexts *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

STDMETHODIMP CEnumTfContext::QueryInterface(REFIID iid, LPVOID *ppvObject)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&iid), ppvObject);

    *ppvObject = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumTfContexts)
        *ppvObject = static_cast<IEnumTfContexts *>(this);

    if (*ppvObject)
    {
        AddRef();
        return S_OK;
    }

    WARN("E_NOINTERFACE: %s\n", wine_dbgstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumTfContext::AddRef()
{
    TRACE("%p -> ()\n", this);
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumTfContext::Release()
{
    TRACE("%p -> ()\n", this);
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CEnumTfContext::Next(ULONG ulCount, ITfContext **rgContext, ULONG *pcFetched)
{
    TRACE("%p -> (%lu, %p, %p)\n",this, ulCount, rgContext, pcFetched);

    if (!rgContext)
    {
        ERR("!rgContext\n");
        return E_POINTER;
    }

    ULONG fetched;
    for (fetched = 0; fetched < ulCount; ++fetched, ++m_index, ++rgContext)
    {
        if (m_index >= _countof(m_pDocMgr->m_contextStack))
            break;

        if (!m_pDocMgr->m_contextStack[m_index])
            break;

        *rgContext = m_pDocMgr->m_contextStack[m_index];
        (*rgContext)->AddRef();
    }

    if (pcFetched)
        *pcFetched = fetched;

    return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumTfContext::Skip(ULONG celt)
{
    TRACE("%p -> (%lu)\n", this, celt);
    m_index += celt;
    return S_OK;
}

STDMETHODIMP CEnumTfContext::Reset()
{
    TRACE("%p -> ()\n", this);
    m_index = 0;
    return S_OK;
}

STDMETHODIMP CEnumTfContext::Clone(IEnumTfContexts **ppenum)
{
    TRACE("%p -> (%p)\n", this, ppenum);

    if (!ppenum)
    {
        ERR("!ppenum\n");
        return E_POINTER;
    }

    CEnumTfContext *This = new(cicNoThrow) CEnumTfContext(m_pDocMgr);
    if (!This)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    This->m_index = m_index;
    *ppenum = This;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT DocumentMgr_Constructor(ITfThreadMgrEventSink *pThreadMgrSink, ITfDocumentMgr **ppOut)
{
    return CDocumentMgr::CreateInstance(pThreadMgrSink, ppOut);
}

EXTERN_C
HRESULT EnumTfContext_Constructor(CDocumentMgr *mgr, IEnumTfContexts **ppOut)
{
    return CEnumTfContext::CreateInstance(mgr, ppOut);
}
