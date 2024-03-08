/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Input Context of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/***********************************************************************
 * CInputContextOwner
 */

/// @unimplemented
CInputContextOwner::CInputContextOwner(LPVOID fnCallback, LPVOID pCallbackPV)
{
    m_dwCookie = -1;
    m_fnCallback = fnCallback;
    m_cRefs = 1;
    m_pCallbackPV = pCallbackPV;
}

/// @implemented
CInputContextOwner::~CInputContextOwner()
{
}

/// @implemented
HRESULT CInputContextOwner::_Advise(IUnknown *pContext)
{
    ITfSource *pSource = NULL;

    m_pContext = NULL;

    HRESULT hr = E_FAIL;
    if (SUCCEEDED(m_pContext->QueryInterface(IID_ITfSource, (LPVOID*)&pSource)) &&
        SUCCEEDED(pSource->AdviseSink(IID_ITfContextOwner,
                                      static_cast<ITfContextOwner*>(this), &m_dwCookie)))
    {
        m_pContext = pContext;
        m_pContext->AddRef();
        hr = S_OK;
    }

    if (pSource)
        pSource->Release();

    return hr;
}

/// @implemented
HRESULT CInputContextOwner::_Unadvise()
{
    ITfSource *pSource = NULL;

    HRESULT hr = E_FAIL;
    if (m_pContext)
    {
        if (SUCCEEDED(m_pContext->QueryInterface(IID_ITfSource, (LPVOID*)&pSource)) &&
            SUCCEEDED(pSource->UnadviseSink(m_dwCookie)))
        {
            hr = S_OK;
        }
    }

    if (m_pContext)
    {
        m_pContext->Release();
        m_pContext = NULL;
    }

    if (pSource)
        pSource->Release();

    return hr;
}

/// @implemented
STDMETHODIMP CInputContextOwner::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfContextOwner))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_ITfMouseTrackerACP))
    {
        *ppvObj = static_cast<ITfMouseTrackerACP*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CInputContextOwner::AddRef()
{
    return ++m_cRefs;
}

/// @implemented
STDMETHODIMP_(ULONG) CInputContextOwner::Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetACPFromPoint(
    const POINT *ptScreen,
    DWORD       dwFlags,
    LONG        *pacp)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetTextExt(
    LONG acpStart,
    LONG acpEnd,
    RECT *prc,
    BOOL *pfClipped)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetScreenExt(RECT *prc)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetStatus(TF_STATUS *pdcs)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetWnd(HWND *phwnd)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::GetAttribute(REFGUID rguidAttribute, VARIANT *pvarValue)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::AdviseMouseSink(
    ITfRangeACP *range,
    ITfMouseSink *pSink,
    DWORD *pdwCookie)
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CInputContextOwner::UnadviseMouseSink(DWORD dwCookie)
{
    return E_NOTIMPL;
}

/***********************************************************************
 * CicInputContext
 */

/// @unimplemented
CicInputContext::CicInputContext(
    _In_ TfClientId cliendId,
    _Inout_ PCIC_LIBTHREAD pLibThread,
    _In_ HIMC hIMC)
{
    m_hIMC = hIMC;
    m_dwQueryPos = 0;
    m_cRefs = 1;
}

/// @implemented
STDMETHODIMP CicInputContext::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
    {
        *ppvObj = static_cast<ITfContextOwnerCompositionSink*>(this);
        AddRef();
        return S_OK;
    }
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCleanupContextSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CicInputContext::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CicInputContext::Release()
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
CicInputContext::OnStartComposition(
    ITfCompositionView *pComposition,
    BOOL *pfOk)
{
    if ((m_cCompLocks <= 0) || m_bReconverting)
    {
        *pfOk = TRUE;
        ++m_cCompLocks;
    }
    else
    {
        *pfOk = FALSE;
    }
    return S_OK;
}

/// @implemented
STDMETHODIMP
CicInputContext::OnUpdateComposition(
    ITfCompositionView *pComposition,
    ITfRange *pRangeNew)
{
    return S_OK;
}

/// @implemented
STDMETHODIMP
CicInputContext::OnEndComposition(
    ITfCompositionView *pComposition)
{
    --m_cCompLocks;
    return S_OK;
}

/// @implemented
HRESULT
CicInputContext::GetGuidAtom(
    _Inout_ CicIMCLock& imcLock,
    _In_ BYTE iAtom,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCompStr);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    HRESULT hr = E_FAIL;
    if (iAtom < m_cGuidAtoms)
    {
        *pdwGuidAtom = m_adwGuidAtoms[iAtom];
        hr = S_OK;
    }

    return hr;
}

/// @unimplemented
HRESULT
CicInputContext::CreateInputContext(
    _Inout_ ITfThreadMgr *pThreadMgr,
    _Inout_ CicIMCLock& imcLock)
{
    //FIXME
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT
CicInputContext::DestroyInputContext()
{
    ITfSourceSingle *pSource = NULL;

    if (m_pContext && m_pContext->QueryInterface(IID_ITfSourceSingle, (void **)&pSource) == S_OK)
        pSource->UnadviseSingleSink(m_clientId, IID_ITfCleanupContextSink);

    //FIXME: m_dwUnknown5

    if (m_pTextEventSink)
    {
        m_pTextEventSink->_Unadvise();
        m_pTextEventSink->Release();
        m_pTextEventSink = NULL;
    }

    if (m_pCompEventSink2)
    {
        m_pCompEventSink2->_Unadvise();
        m_pCompEventSink2->Release();
        m_pCompEventSink2 = NULL;
    }

    if (m_pCompEventSink1)
    {
        m_pCompEventSink1->_Unadvise();
        m_pCompEventSink1->Release();
        m_pCompEventSink1 = NULL;
    }

    //FIXME: m_pInputContextOwner

    if (m_pDocumentMgr)
        m_pDocumentMgr->Pop(1);

    if (m_pContext)
    {
        ClearCompartment(m_clientId, m_pContext, GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT, 0);
        m_pContext->Release();
        m_pContext = NULL;
    }

    if (m_pContextOwnerServices)
    {
        m_pContextOwnerServices->Release();
        m_pContextOwnerServices = NULL;
    }

    // FIXME: m_pICOwnerCallback

    if (m_pDocumentMgr)
    {
        m_pDocumentMgr->Release();
        m_pDocumentMgr = NULL;
    }

    if (pSource)
        pSource->Release();

    return S_OK;
}

/// @implemented
STDMETHODIMP
CicInputContext::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition)
{
    return S_OK;
}

/// @implemented
STDMETHODIMP
CicInputContext::OnCleanupContext(
    _In_ TfEditCookie ecWrite,
    _Inout_ ITfContext *pic)
{
    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS || !pTLS->m_pProfile)
        return E_OUTOFMEMORY;

    LANGID LangID;
    pTLS->m_pProfile->GetLangId(&LangID);

    IMEINFO IMEInfo;
    WCHAR szPath[MAX_PATH];
    if (Inquire(&IMEInfo, szPath, 0, (HKL)UlongToHandle(LangID)) != S_OK)
        return E_FAIL;

    ITfProperty *pProp = NULL;
    if (!(IMEInfo.fdwProperty & IME_PROP_COMPLETE_ON_UNSELECT))
        return S_OK;

    HRESULT hr = pic->GetProperty(GUID_PROP_COMPOSING, &pProp);
    if (FAILED(hr))
        return S_OK;

    IEnumTfRanges *pRanges = NULL;
    hr = pProp->EnumRanges(ecWrite, &pRanges, NULL);
    if (SUCCEEDED(hr))
    {
        ITfRange *pRange = NULL;
        while (pRanges->Next(1, &pRange, 0) == S_OK)
        {
            VARIANT vari;
            V_VT(&vari) = VT_EMPTY;
            pProp->GetValue(ecWrite, pRange, &vari);
            if (V_VT(&vari) == VT_I4)
            {
                if (V_I4(&vari))
                    pProp->Clear(ecWrite, pRange);
            }
            pRange->Release();
            pRange = NULL;
        }
        pRanges->Release();
    }
    pProp->Release();

    return S_OK;
}

/// @unimplemented
HRESULT CicInputContext::SetupDocFeedString(CicIMCLock& imcLock, UINT uCodePage)
{
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT CicInputContext::EscbClearDocFeedBuffer(CicIMCLock& imcLock, BOOL bFlag)
{
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT CicInputContext::EscbCompComplete(CicIMCLock& imcLock)
{
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT CicInputContext::DelayedReconvertFuncCall(CicIMCLock& imcLock)
{
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT
CicInputContext::MsImeMouseHandler(
    DWORD dwUnknown58,
    DWORD dwUnknown59,
    UINT keys,
    CicIMCLock& imcLock)
{
    return E_NOTIMPL;
}

/// @unimplemented
HRESULT
CicInputContext::SetupReconvertString(
    CicIMCLock& imcLock,
    ITfThreadMgr_P *pThreadMgr,
    UINT uCodePage,
    UINT uMsg,
    BOOL bUndo)
{
    return E_NOTIMPL;
}

void CicInputContext::ClearPrevCandidatePos()
{
    m_dwUnknown8 = 0;
    ZeroMemory(&m_rcCandidate1, sizeof(m_rcCandidate1));
    ZeroMemory(&m_CandForm, sizeof(m_CandForm));
    ZeroMemory(&m_rcCandidate2, sizeof(m_rcCandidate2));
    m_dwQueryPos = 0;
}

/// @unimplemented
HRESULT CicInputContext::EndReconvertString(CicIMCLock& imcLock)
{
    return E_NOTIMPL;
}
