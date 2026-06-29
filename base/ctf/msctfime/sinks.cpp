/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     The sinks of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/// @implemented
CCompartmentEventSink::CCompartmentEventSink(FN_EVENTSINK fnEventSink, LPVOID pUserData)
    : m_array()
    , m_cRefs(1)
    , m_fnEventSink(fnEventSink)
    , m_pUserData(pUserData)
{
}

/// @implemented
CCompartmentEventSink::~CCompartmentEventSink()
{
}

/// @implemented
STDMETHODIMP CCompartmentEventSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CCompartmentEventSink, ITfCompartmentEventSink),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

/// @implemented
STDMETHODIMP_(ULONG) CCompartmentEventSink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CCompartmentEventSink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
STDMETHODIMP CCompartmentEventSink::OnChange(REFGUID rguid)
{
    return m_fnEventSink(m_pUserData, rguid);
}

/// @implemented
HRESULT
CCompartmentEventSink::_Advise(IUnknown *pUnknown, REFGUID rguid, BOOL bThread)
{
    CESMAP *pCesMap = m_array.Append(1);
    if (!pCesMap)
        return E_OUTOFMEMORY;

    ITfSource *pSource = NULL;

    HRESULT hr = GetCompartment(pUnknown, rguid, &pCesMap->m_pComp, bThread);
    if (FAILED(hr))
    {
        hr = pCesMap->m_pComp->QueryInterface(IID_ITfSource, (void **)&pSource);
        if (FAILED(hr))
        {
            hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, this, &pCesMap->m_dwCookie);
            if (FAILED(hr))
            {
                if (pCesMap->m_pComp)
                {
                    pCesMap->m_pComp->Release();
                    pCesMap->m_pComp = NULL;
                }
                m_array.Remove(m_array.size() - 1, 1);
            }
            else
            {
                hr = S_OK;
            }
        }
    }

    if (pSource)
        pSource->Release();

    return hr;
}

/// @implemented
HRESULT CCompartmentEventSink::_Unadvise()
{
    CESMAP *pCesMap = m_array.data();
    size_t cItems = m_array.size();
    if (!cItems)
        return S_OK;

    do
    {
        ITfSource *pSource = NULL;
        HRESULT hr = pCesMap->m_pComp->QueryInterface(IID_ITfSource, (void **)&pSource);
        if (SUCCEEDED(hr))
            pSource->UnadviseSink(pCesMap->m_dwCookie);

        if (pCesMap->m_pComp)
        {
            pCesMap->m_pComp->Release();
            pCesMap->m_pComp = NULL;
        }

        if (pSource)
            pSource->Release();

        ++pCesMap;
        --cItems;
    } while (cItems);

    return S_OK;
}

/***********************************************************************/

/// @implemented
CTextEventSink::CTextEventSink(FN_ENDEDIT fnEndEdit, LPVOID pCallbackPV)
{
    m_cRefs = 1;
    m_pUnknown = NULL;
    m_dwEditSinkCookie = (DWORD)-1;
    m_dwLayoutSinkCookie = (DWORD)-1;
    m_fnLayoutChange = NULL;
    m_fnEndEdit = fnEndEdit;
    m_pCallbackPV = pCallbackPV;
}

/// @implemented
CTextEventSink::~CTextEventSink()
{
}

/// @implemented
STDMETHODIMP CTextEventSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CTextEventSink, ITfTextEditSink),
        QITABENT(CTextEventSink, ITfTextLayoutSink),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

/// @implemented
STDMETHODIMP_(ULONG) CTextEventSink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CTextEventSink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

struct TEXT_EVENT_SINK_END_EDIT
{
    TfEditCookie m_ecReadOnly;
    ITfEditRecord *m_pEditRecord;
    ITfContext *m_pContext;
};

/// @implemented
STDMETHODIMP CTextEventSink::OnEndEdit(
    ITfContext *pic,
    TfEditCookie ecReadOnly,
    ITfEditRecord *pEditRecord)
{
    TEXT_EVENT_SINK_END_EDIT Data = { ecReadOnly, pEditRecord, pic };
    return m_fnEndEdit(1, m_pCallbackPV, (LPVOID)&Data);
}

/// @implemented
STDMETHODIMP CTextEventSink::OnLayoutChange(
    ITfContext *pContext,
    TfLayoutCode lcode,
    ITfContextView *pContextView)
{
    switch (lcode)
    {
        case TF_LC_CREATE:
            return m_fnLayoutChange(3, m_fnEndEdit, pContextView);
        case TF_LC_CHANGE:
            return m_fnLayoutChange(2, m_fnEndEdit, pContextView);
        case TF_LC_DESTROY:
            return m_fnLayoutChange(4, m_fnEndEdit, pContextView);
        default:
            return E_INVALIDARG;
    }
}

/// @implemented
HRESULT CTextEventSink::_Advise(IUnknown *pUnknown, UINT uFlags)
{
    m_pUnknown = NULL;
    m_uFlags = uFlags;

    ITfSource *pSource = NULL;
    HRESULT hr = pUnknown->QueryInterface(IID_ITfSource, (void**)&pSource);
    if (SUCCEEDED(hr))
    {
        ITfTextEditSink *pSink = static_cast<ITfTextEditSink*>(this);
        if (uFlags & 1)
            hr = pSource->AdviseSink(IID_ITfTextEditSink, pSink, &m_dwEditSinkCookie);
        if (SUCCEEDED(hr) && (uFlags & 2))
            hr = pSource->AdviseSink(IID_ITfTextLayoutSink, pSink, &m_dwLayoutSinkCookie);

        if (SUCCEEDED(hr))
        {
            m_pUnknown = pUnknown;
            pUnknown->AddRef();
        }
        else
        {
            pSource->UnadviseSink(m_dwEditSinkCookie);
        }
    }

    if (pSource)
        pSource->Release();

    return hr;
}

/// @implemented
HRESULT CTextEventSink::_Unadvise()
{
    if (!m_pUnknown)
        return E_FAIL;

    ITfSource *pSource = NULL;
    HRESULT hr = m_pUnknown->QueryInterface(IID_ITfSource, (void**)&pSource);
    if (SUCCEEDED(hr))
    {
        if (m_uFlags & 1)
            hr = pSource->UnadviseSink(m_dwEditSinkCookie);
        if (m_uFlags & 2)
            hr = pSource->UnadviseSink(m_dwLayoutSinkCookie);

        pSource->Release();
    }

    m_pUnknown->Release();
    m_pUnknown = NULL;

    return E_NOTIMPL;
}

/***********************************************************************/

/// @implemented
CThreadMgrEventSink::CThreadMgrEventSink(
    _In_ FN_INITDOCMGR fnInit,
    _In_ FN_PUSHPOP fnPushPop,
    _Inout_ LPVOID pvCallbackPV)
{
    m_fnInit = fnInit;
    m_fnPushPop = fnPushPop;
    m_pCallbackPV = pvCallbackPV;
    m_cRefs = 1;
}

/// @implemented
STDMETHODIMP CThreadMgrEventSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CThreadMgrEventSink, ITfThreadMgrEventSink),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

/// @implemented
STDMETHODIMP_(ULONG) CThreadMgrEventSink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CThreadMgrEventSink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

INT CALLBACK
CThreadMgrEventSink::DIMCallback(
    UINT nCode,
    ITfDocumentMgr *pDocMgr1,
    ITfDocumentMgr *pDocMgr2,
    LPVOID pUserData)
{
    return E_NOTIMPL;
}

STDMETHODIMP CThreadMgrEventSink::OnInitDocumentMgr(ITfDocumentMgr *pdim)
{
    if (!m_fnInit)
        return S_OK;
    return m_fnInit(0, pdim, NULL, m_pCallbackPV);
}

STDMETHODIMP CThreadMgrEventSink::OnUninitDocumentMgr(ITfDocumentMgr *pdim)
{
    if (!m_fnInit)
        return S_OK;
    return m_fnInit(1, pdim, NULL, m_pCallbackPV);
}

STDMETHODIMP
CThreadMgrEventSink::OnSetFocus(ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus)
{
    if (!m_fnInit)
        return S_OK;
    return m_fnInit(2, pdimFocus, pdimPrevFocus, m_pCallbackPV);
}

STDMETHODIMP CThreadMgrEventSink::OnPushContext(ITfContext *pic)
{
    if (!m_fnPushPop)
        return S_OK;
    return m_fnPushPop(3, pic, m_pCallbackPV);
}

STDMETHODIMP CThreadMgrEventSink::OnPopContext(ITfContext *pic)
{
    if (!m_fnPushPop)
        return S_OK;
    return m_fnPushPop(4, pic, m_pCallbackPV);
}

void CThreadMgrEventSink::SetCallbackPV(_Inout_ LPVOID pv)
{
    if (!m_pCallbackPV)
        m_pCallbackPV = pv;
}

HRESULT CThreadMgrEventSink::_Advise(ITfThreadMgr *pThreadMgr)
{
    m_pThreadMgr = NULL;

    HRESULT hr = E_FAIL;
    ITfSource *pSource = NULL;
    if (pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource) == S_OK &&
        pSource->AdviseSink(IID_ITfThreadMgrEventSink, this, &m_dwCookie) == S_OK)
    {
        m_pThreadMgr = pThreadMgr;
        pThreadMgr->AddRef();
        hr = S_OK;
    }

    if (pSource)
        pSource->Release();

    return hr;
}

HRESULT CThreadMgrEventSink::_Unadvise()
{
    HRESULT hr = E_FAIL;
    ITfSource *pSource = NULL;

    if (m_pThreadMgr)
    {
        if (m_pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource) == S_OK &&
            pSource->UnadviseSink(m_dwCookie) == S_OK)
        {
            hr = S_OK;
        }

        if (pSource)
            pSource->Release();
    }

    if (m_pThreadMgr)
    {
        m_pThreadMgr->Release();
        m_pThreadMgr = NULL;
    }

    return hr;
}

/***********************************************************************/

/// @implemented
CActiveLanguageProfileNotifySink::CActiveLanguageProfileNotifySink(
    _In_ FN_COMPARE fnCompare,
    _Inout_opt_ void *pUserData)
{
    m_dwConnection = (DWORD)-1;
    m_fnCompare = fnCompare;
    m_cRefs = 1;
    m_pUserData = pUserData;
}

/// @implemented
CActiveLanguageProfileNotifySink::~CActiveLanguageProfileNotifySink()
{
}

/// @implemented
STDMETHODIMP CActiveLanguageProfileNotifySink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CActiveLanguageProfileNotifySink, ITfActiveLanguageProfileNotifySink),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

/// @implemented
STDMETHODIMP_(ULONG) CActiveLanguageProfileNotifySink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CActiveLanguageProfileNotifySink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
STDMETHODIMP
CActiveLanguageProfileNotifySink::OnActivated(
    REFCLSID clsid,
    REFGUID guidProfile,
    BOOL fActivated)
{
    if (!m_fnCompare)
        return 0;

    return m_fnCompare(clsid, guidProfile, fActivated, m_pUserData);
}

/// @implemented
HRESULT
CActiveLanguageProfileNotifySink::_Advise(
    ITfThreadMgr *pThreadMgr)
{
    m_pThreadMgr = NULL;

    ITfSource *pSource = NULL;
    HRESULT hr = pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (FAILED(hr))
        return E_FAIL;

    hr = pSource->AdviseSink(IID_ITfActiveLanguageProfileNotifySink, this, &m_dwConnection);
    if (SUCCEEDED(hr))
    {
        m_pThreadMgr = pThreadMgr;
        pThreadMgr->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    if (pSource)
        pSource->Release();

    return hr;
}

/// @implemented
HRESULT
CActiveLanguageProfileNotifySink::_Unadvise()
{
    if (!m_pThreadMgr)
        return E_FAIL;

    ITfSource *pSource = NULL;
    HRESULT hr = m_pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (SUCCEEDED(hr))
    {
        hr = pSource->UnadviseSink(m_dwConnection);
        if (SUCCEEDED(hr))
            hr = S_OK;
    }

    if (pSource)
        pSource->Release();

    if (m_pThreadMgr)
    {
        m_pThreadMgr->Release();
        m_pThreadMgr = NULL;
    }

    return hr;
}
