/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     The bridge of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/// @implemented
CicBridge::CicBridge()
{
    m_bImmxInited = FALSE;
    m_bUnknown1 = FALSE;
    m_bDeactivating = FALSE;
    m_bUnknown2 = FALSE;
    m_pKeystrokeMgr = NULL;
    m_pDocMgr = NULL;
    m_pThreadMgrEventSink = NULL;
    m_cliendId = 0;
    m_cRefs = 1;
}

/// @implemented
STDMETHODIMP CicBridge::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CicBridge, ITfSysHookSink),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

/// @implemented
STDMETHODIMP_(ULONG) CicBridge::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CicBridge::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
CicBridge::~CicBridge()
{
    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS || !pTLS->m_pThreadMgr)
        return;

    if (SUCCEEDED(DeactivateIMMX(pTLS, pTLS->m_pThreadMgr)))
        UnInitIMMX(pTLS);
}

/// @implemented
ITfDocumentMgr*
CicBridge::GetDocumentManager(_Inout_ CicIMCCLock<CTFIMECONTEXT>& imeContext)
{
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return NULL;

    pCicIC->m_pDocumentMgr->AddRef();
    return pCicIC->m_pDocumentMgr;
}

/// @implemented
HRESULT
CicBridge::CreateInputContext(
    _Inout_ TLS *pTLS,
    _In_ HIMC hIMC)
{
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    if (!imcLock.get().hCtfImeContext)
    {
        HIMCC hCtfImeContext = ImmCreateIMCC(sizeof(CTFIMECONTEXT));
        if (!hCtfImeContext)
            return E_OUTOFMEMORY;
        imcLock.get().hCtfImeContext = hCtfImeContext;
    }

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
        return S_OK;

    pCicIC = new(cicNoThrow) CicInputContext(m_cliendId, &m_LibThread, hIMC);
    if (!pCicIC)
    {
        imeContext.unlock();
        imcLock.unlock();
        DestroyInputContext(pTLS, hIMC);
        return E_OUTOFMEMORY;
    }

    if (!pTLS->m_pThreadMgr)
    {
        pCicIC->Release();
        imeContext.unlock();
        imcLock.unlock();
        DestroyInputContext(pTLS, hIMC);
        return E_NOINTERFACE;
    }

    imeContext.get().m_pCicIC = pCicIC;

    HRESULT hr = pCicIC->CreateInputContext(pTLS->m_pThreadMgr, imcLock);
    if (FAILED(hr))
    {
        pCicIC->Release();
        imeContext.get().m_pCicIC = NULL;
        return hr;
    }

    HWND hWnd = imcLock.get().hWnd;
    if (hWnd && hWnd == ::GetFocus())
    {
        ITfDocumentMgr *pDocMgr = GetDocumentManager(imeContext);
        if (pDocMgr)
        {
            SetAssociate(pTLS, hWnd, hIMC, pTLS->m_pThreadMgr, pDocMgr);
            pDocMgr->Release();
        }
    }

    return hr;
}

/// @implemented
HRESULT CicBridge::DestroyInputContext(TLS *pTLS, HIMC hIMC)
{
    CicIMCLock imcLock(hIMC);
    HRESULT hr = imcLock.m_hr;
    if (FAILED(hr))
        return hr;

    hr = E_FAIL;
    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (imeContext)
        hr = imeContext.m_hr;

    if (SUCCEEDED(hr) && !(imeContext.get().m_dwCicFlags & 1))
    {
        imeContext.get().m_dwCicFlags |= 1;

        CicInputContext *pCicIC = imeContext.get().m_pCicIC;
        if (pCicIC)
        {
            imeContext.get().m_pCicIC = NULL;
            hr = pCicIC->DestroyInputContext();
            pCicIC->Release();
            imeContext.get().m_pCicIC = NULL;
        }
    }

    if (imcLock.get().hCtfImeContext)
    {
        ImmDestroyIMCC(imcLock.get().hCtfImeContext);
        imcLock.get().hCtfImeContext = NULL;
        hr = S_OK;
    }

    return hr;
}

/// @implemented
ITfContext *
CicBridge::GetInputContext(CicIMCCLock<CTFIMECONTEXT>& imeContext)
{
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return NULL;
    return pCicIC->m_pContext;
}

/// @implemented
HRESULT CicBridge::OnSetOpenStatus(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    CicIMCLock& imcLock,
    CicInputContext *pCicIC)
{
    if (!imcLock.get().fOpen && imcLock.ValidCompositionString())
        pCicIC->EscbCompComplete(imcLock);

    pTLS->m_bNowOpening = TRUE;
    HRESULT hr = SetCompartmentDWORD(m_cliendId, pThreadMgr,
                                     GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                     imcLock.get().fOpen, FALSE);
    pTLS->m_bNowOpening = FALSE;
    return hr;
}

/// Selects the IME context.
/// @implemented
HRESULT
CicBridge::SelectEx(
    _Inout_ TLS *pTLS,
    _Inout_ ITfThreadMgr_P *pThreadMgr,
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ HKL hKL)
{
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
        pCicIC->m_bSelecting = TRUE;

    if (fSelect)
    {
        if (pCicIC)
            pCicIC->m_bCandidateOpen = FALSE;
        if (imcLock.get().fOpen)
            OnSetOpenStatus(pTLS, pThreadMgr, imcLock, pCicIC);
    }
    else
    {
        ITfContext *pContext = GetInputContext(imeContext);
        pThreadMgr->RequestPostponedLock(pContext);
        if (pCicIC)
            pCicIC->m_bSelecting = FALSE;
        if (pContext)
            pContext->Release();
    }

    return imeContext.m_hr;
}

/// Used in CicBridge::EnumCreateInputContextCallback and
/// CicBridge::EnumDestroyInputContextCallback.
typedef struct ENUM_CREATE_DESTROY_IC
{
    TLS *m_pTLS;
    CicBridge *m_pBridge;
} ENUM_CREATE_DESTROY_IC, *PENUM_CREATE_DESTROY_IC;

/// Creates input context for the current thread.
/// @implemented
BOOL CALLBACK CicBridge::EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    PENUM_CREATE_DESTROY_IC pData = (PENUM_CREATE_DESTROY_IC)lParam;
    pData->m_pBridge->CreateInputContext(pData->m_pTLS, hIMC);
    return TRUE;
}

/// Destroys input context for the current thread.
/// @implemented
BOOL CALLBACK CicBridge::EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    PENUM_CREATE_DESTROY_IC pData = (PENUM_CREATE_DESTROY_IC)lParam;
    pData->m_pBridge->DestroyInputContext(pData->m_pTLS, hIMC);
    return TRUE;
}

/// @implemented
HRESULT
CicBridge::ActivateIMMX(
    _Inout_ TLS *pTLS,
    _Inout_ ITfThreadMgr_P *pThreadMgr)
{
    HRESULT hr = pThreadMgr->ActivateEx(&m_cliendId, 1);
    if (hr != S_OK)
    {
        m_cliendId = 0;
        return E_FAIL;
    }

    if (m_cActivateLocks++ != 0)
        return S_OK;

    ITfSourceSingle *pSource = NULL;
    hr = pThreadMgr->QueryInterface(IID_ITfSourceSingle, (void**)&pSource);
    if (FAILED(hr))
    {
        DeactivateIMMX(pTLS, pThreadMgr);
        return hr;
    }

    CFunctionProvider *pProvider = new(cicNoThrow) CFunctionProvider(m_cliendId);
    if (!pProvider)
    {
        hr = E_FAIL;
        goto Finish;
    }

    pSource->AdviseSingleSink(m_cliendId, IID_ITfFunctionProvider, pProvider);
    pProvider->Release();

    if (!m_pDocMgr)
    {
        hr = pThreadMgr->CreateDocumentMgr(&m_pDocMgr);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            goto Finish;
        }

        SetCompartmentDWORD(m_cliendId, m_pDocMgr, GUID_COMPARTMENT_CTFIME_DIMFLAGS, TRUE, FALSE);
    }

    pThreadMgr->SetSysHookSink(this);

    hr = S_OK;
    if (pTLS->m_bDestroyed)
    {
        ENUM_CREATE_DESTROY_IC Data = { pTLS, this };
        ImmEnumInputContext(0, CicBridge::EnumCreateInputContextCallback, (LPARAM)&Data);
    }

Finish:
    if (FAILED(hr))
        DeactivateIMMX(pTLS, pThreadMgr);
    if (pSource)
        pSource->Release();
    return hr;
}

/// @implemented
HRESULT
CicBridge::DeactivateIMMX(
    _Inout_ TLS *pTLS,
    _Inout_ ITfThreadMgr_P *pThreadMgr)
{
    if (m_bDeactivating)
        return TRUE;

    m_bDeactivating = TRUE;

    if (m_cliendId)
    {
        ENUM_CREATE_DESTROY_IC Data = { pTLS, this };
        ImmEnumInputContext(0, CicBridge::EnumDestroyInputContextCallback, (LPARAM)&Data);
        pTLS->m_bDestroyed = TRUE;

        ITfSourceSingle *pSource = NULL;
        if (pThreadMgr->QueryInterface(IID_ITfSourceSingle, (void **)&pSource) == S_OK)
            pSource->UnadviseSingleSink(m_cliendId, IID_ITfFunctionProvider);

        m_cliendId = 0;

        while (m_cActivateLocks > 0)
        {
            --m_cActivateLocks;
            pThreadMgr->Deactivate();
        }

        if (pSource)
            pSource->Release();
    }

    if (m_pDocMgr)
    {
        m_pDocMgr->Release();
        m_pDocMgr = NULL;
    }

    pThreadMgr->SetSysHookSink(NULL);

    m_bDeactivating = FALSE;

    return S_OK;
}

/// @implemented
HRESULT
CicBridge::InitIMMX(_Inout_ TLS *pTLS)
{
    if (m_bImmxInited)
        return S_OK;

    HRESULT hr = S_OK;
    if (!pTLS->m_pThreadMgr)
    {
        ITfThreadMgr *pThreadMgr = NULL;
        hr = TF_CreateThreadMgr(&pThreadMgr);
        if (FAILED(hr))
            return E_FAIL;

        hr = pThreadMgr->QueryInterface(IID_ITfThreadMgr_P, (void **)&pTLS->m_pThreadMgr);
        if (pThreadMgr)
            pThreadMgr->Release();
        if (FAILED(hr))
            return E_FAIL;
    }

    if (!m_pThreadMgrEventSink)
    {
        m_pThreadMgrEventSink =
            new(cicNoThrow) CThreadMgrEventSink(CThreadMgrEventSink::DIMCallback, NULL, NULL);
        if (!m_pThreadMgrEventSink)
        {
            UnInitIMMX(pTLS);
            return E_FAIL;
        }
    }

    m_pThreadMgrEventSink->SetCallbackPV(m_pThreadMgrEventSink);
    m_pThreadMgrEventSink->_Advise(pTLS->m_pThreadMgr);

    if (!pTLS->m_pProfile)
    {
        pTLS->m_pProfile = new(cicNoThrow) CicProfile();
        if (!pTLS->m_pProfile)
            return E_OUTOFMEMORY;

        hr = pTLS->m_pProfile->InitProfileInstance(pTLS);
        if (FAILED(hr))
        {
            UnInitIMMX(pTLS);
            return E_FAIL;
        }
    }

    hr = pTLS->m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr_P, (void **)&m_pKeystrokeMgr);
    if (FAILED(hr))
    {
        UnInitIMMX(pTLS);
        return E_FAIL;
    }

    hr = InitDisplayAttrbuteLib(&m_LibThread);
    if (FAILED(hr))
    {
        UnInitIMMX(pTLS);
        return E_FAIL;
    }

    m_bImmxInited = TRUE;
    return S_OK;
}

/// @implemented
BOOL CicBridge::UnInitIMMX(_Inout_ TLS *pTLS)
{
    UninitDisplayAttrbuteLib(&m_LibThread);
    TFUninitLib_Thread(&m_LibThread);

    if (m_pKeystrokeMgr)
    {
        m_pKeystrokeMgr->Release();
        m_pKeystrokeMgr = NULL;
    }

    if (pTLS->m_pProfile)
    {
        pTLS->m_pProfile->Release();
        pTLS->m_pProfile = NULL;
    }

    if (m_pThreadMgrEventSink)
    {
        m_pThreadMgrEventSink->_Unadvise();
        m_pThreadMgrEventSink->Release();
        m_pThreadMgrEventSink = NULL;
    }

    if (pTLS->m_pThreadMgr)
    {
        pTLS->m_pThreadMgr->Release();
        pTLS->m_pThreadMgr = NULL;
    }

    m_bImmxInited = FALSE;
    return TRUE;
}

/// @implemented
STDMETHODIMP CicBridge::OnPreFocusDIM(HWND hwnd)
{
    return S_OK;
}

/// @unimplemented
STDMETHODIMP CicBridge::OnSysKeyboardProc(UINT, LONG)
{
    return E_NOTIMPL;
}

/// @implemented
STDMETHODIMP CicBridge::OnSysShellProc(INT, UINT, LONG)
{
    return S_OK;
}

/// @implemented
void
CicBridge::PostTransMsg(
    _In_ HWND hWnd,
    _In_ INT cTransMsgs,
    _In_ const TRANSMSG *pTransMsgs)
{
    for (INT i = 0; i < cTransMsgs; ++i, ++pTransMsgs)
    {
        ::PostMessageW(hWnd, pTransMsgs->message, pTransMsgs->wParam, pTransMsgs->lParam);
    }
}

/// @implemented
HRESULT
CicBridge::ConfigureGeneral(
    _Inout_ TLS* pTLS,
    _In_ ITfThreadMgr *pThreadMgr,
    _In_ HKL hKL,
    _In_ HWND hWnd)
{
    CicProfile *pProfile = pTLS->m_pProfile;
    if (!pProfile)
        return E_OUTOFMEMORY;

    TF_LANGUAGEPROFILE profile;
    HRESULT hr = pProfile->GetActiveLanguageProfile(hKL, GUID_TFCAT_TIP_KEYBOARD, &profile);
    if (FAILED(hr))
        return hr;

    ITfFunctionProvider *pProvider = NULL;
    hr = pThreadMgr->GetFunctionProvider(profile.clsid, &pProvider);
    if (FAILED(hr))
        return hr;

    ITfFnConfigure *pFnConfigure = NULL;
    hr = pProvider->GetFunction(GUID_NULL, IID_ITfFnConfigure, (IUnknown**)&pFnConfigure);
    if (FAILED(hr))
    {
        pProvider->Release();
        return hr;
    }

    hr = pFnConfigure->Show(hWnd, profile.langid, profile.guidProfile);

    pFnConfigure->Release();
    pProvider->Release();
    return hr;
}

/// @implemented
HRESULT
CicBridge::ConfigureRegisterWord(
    _Inout_ TLS* pTLS,
    _In_ ITfThreadMgr *pThreadMgr,
    _In_ HKL hKL,
    _In_ HWND hWnd,
    _Inout_opt_ LPVOID lpData)
{
    CicProfile *pProfile = pTLS->m_pProfile;
    if (!pProfile)
        return E_OUTOFMEMORY;

    TF_LANGUAGEPROFILE profile;
    HRESULT hr = pProfile->GetActiveLanguageProfile(hKL, GUID_TFCAT_TIP_KEYBOARD, &profile);
    if (FAILED(hr))
        return hr;

    ITfFunctionProvider *pProvider = NULL;
    hr = pThreadMgr->GetFunctionProvider(profile.clsid, &pProvider);
    if (FAILED(hr))
        return hr;

    ITfFnConfigureRegisterWord *pFunction = NULL;
    hr = pProvider->GetFunction(GUID_NULL, IID_ITfFnConfigureRegisterWord, (IUnknown**)&pFunction);
    if (FAILED(hr))
    {
        pProvider->Release();
        return hr;
    }

    REGISTERWORDW* pRegWord = (REGISTERWORDW*)lpData;
    if (pRegWord)
    {
        if (pRegWord->lpWord)
        {
            hr = E_OUTOFMEMORY;
            BSTR bstrWord = SysAllocString(pRegWord->lpWord);
            if (bstrWord)
            {
                hr = pFunction->Show(hWnd, profile.langid, profile.guidProfile, bstrWord);
                SysFreeString(bstrWord);
            }
        }
        else
        {
            hr = pFunction->Show(hWnd, profile.langid, profile.guidProfile, NULL);
        }
    }

    pProvider->Release();
    pFunction->Release();
    return hr;
}

/// @unimplemented
void CicBridge::SetAssociate(
    TLS *pTLS,
    HWND hWnd,
    HIMC hIMC,
    ITfThreadMgr_P *pThreadMgr,
    ITfDocumentMgr *pDocMgr)
{
    //FIXME
}

HRESULT
CicBridge::SetActiveContextAlways(TLS *pTLS, HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL)
{
    auto pThreadMgr = pTLS->m_pThreadMgr;
    if (!pThreadMgr)
        return E_OUTOFMEMORY;

    if (fActive)
    {
        if (!hIMC)
        {
            SetAssociate(pTLS, hWnd, hIMC, pThreadMgr, m_pDocMgr);
            return S_OK;
        }

        CicIMCLock imcLock(hIMC);
        if (FAILED(imcLock.m_hr))
            return imcLock.m_hr;

        CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
        if (FAILED(imeContext.m_hr))
            return imeContext.m_hr;

        if (hIMC == ::ImmGetContext(hWnd))
        {
            ITfDocumentMgr *pDocMgr = GetDocumentManager(imeContext);
            if (pDocMgr)
            {
                SetAssociate(pTLS, imcLock.get().hWnd, hIMC, pThreadMgr, pDocMgr);
                pDocMgr->Release();
            }
        }

        return S_OK;
    }

    if (hIMC && !IsEALang(LOWORD(hKL)))
    {
        CicIMCLock imcLock(hIMC);
        if (FAILED(imcLock.m_hr))
            return imcLock.m_hr;

        CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
        if (FAILED(imeContext.m_hr))
            return imeContext.m_hr;

        CicInputContext *pCicIC = imeContext.get().m_pCicIC;
        if (!pCicIC->m_dwUnknown6_5[2] && !pCicIC->m_dwUnknown6_5[3])
            ::ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    }

    if (!hIMC || (::GetFocus() != hWnd) || (hIMC != ::ImmGetContext(hWnd)))
        SetAssociate(pTLS, hWnd, hIMC, pThreadMgr, m_pDocMgr);

    return S_OK;
}

/// @unimplemented
BOOL
CicBridge::DoOpenCandidateHanja(
    ITfThreadMgr_P *pThreadMgr,
    CicIMCLock& imcLock,
    CicInputContext *pCicIC)
{
    return FALSE;
}

/// @unimplemented
HRESULT
CicBridge::OnSetConversionSentenceMode(
    ITfThreadMgr_P *pThreadMgr,
    CicIMCLock& imcLock,
    CicInputContext *pCicIC,
    DWORD dwValue,
    LANGID LangID)
{
    return E_NOTIMPL;
}

/// @implemented
HRESULT CicBridge::Notify(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    HIMC hIMC,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD_PTR dwValue)
{
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return E_OUTOFMEMORY;

    CicProfile *pProfile = pTLS->m_pProfile;
    if (!pProfile)
        return E_OUTOFMEMORY;

    LANGID LangID;
    pProfile->GetLangId(&LangID);

    switch (dwAction)
    {
        case NI_OPENCANDIDATE:
            if (PRIMARYLANGID(LangID) == LANG_KOREAN)
            {
                if (DoOpenCandidateHanja(pThreadMgr, imcLock, pCicIC))
                    return S_OK;
                return E_FAIL;
            }
            return E_NOTIMPL;

        case NI_COMPOSITIONSTR:
            switch (dwIndex)
            {
                case CPS_COMPLETE:
                    pCicIC->EscbCompComplete(imcLock);
                    break;

                case CPS_CONVERT:
                case CPS_REVERT:
                    return E_NOTIMPL;

                case CPS_CANCEL:
                    pCicIC->EscbCompCancel(imcLock);
                    break;

                default:
                    return E_FAIL;
            }
            return S_OK;

        case NI_CONTEXTUPDATED:
            switch (dwValue)
            {
                case IMC_SETCONVERSIONMODE:
                case IMC_SETSENTENCEMODE:
                    return OnSetConversionSentenceMode(pThreadMgr, imcLock, pCicIC, dwValue, LangID);

                case IMC_SETOPENSTATUS:
                    return OnSetOpenStatus(pTLS, pThreadMgr, imcLock, pCicIC);

                case IMC_SETCANDIDATEPOS:
                    return pCicIC->OnSetCandidatePos(pTLS, imcLock);

                case IMC_SETCOMPOSITIONFONT:
                case IMC_SETCOMPOSITIONWINDOW:
                    return E_NOTIMPL;

                default:
                    return E_FAIL;
            }
            break;

        default:
            return E_NOTIMPL;
    }
}

/// @unimplemented
BOOL CicBridge::ProcessKey(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    CONST LPBYTE lpbKeyState,
    INT *pnUnknown60)
{
    return FALSE; // FIXME
}

/// @unimplemented
HRESULT
CicBridge::ToAsciiEx(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    UINT uVirtKey,
    UINT uScanCode,
    CONST LPBYTE lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT fuState,
    HIMC hIMC,
    UINT *pResult)
{
    return E_NOTIMPL; // FIXME
}

/// @implemented
BOOL
CicBridge::SetCompositionString(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    HIMC hIMC,
    DWORD dwIndex,
    LPCVOID lpComp,
    DWORD dwCompLen,
    LPCVOID lpRead,
    DWORD dwReadLen)
{
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return FALSE;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return FALSE;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    auto pProfile = pTLS->m_pProfile;
    if (!pCicIC || !pProfile)
        return FALSE;

    UINT uCodePage;
    pProfile->GetCodePageA(&uCodePage);

    LANGID LangID;
    if (dwIndex != SCS_SETSTR ||
        !lpComp || *(WORD*)lpComp ||
        !dwCompLen ||
        FAILED(pProfile->GetLangId(&LangID)) ||
        PRIMARYLANGID(LangID) != LANG_KOREAN)
    {
        return pCicIC->SetCompositionString(imcLock, pThreadMgr, dwIndex,
                                            lpComp, dwCompLen, lpRead, dwReadLen,
                                            uCodePage);
    }

    if (imcLock.get().fdwConversion & IME_CMODE_NATIVE)
    {
        ::ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        return TRUE;
    }

    return FALSE;
}

/// @unimplemented
LRESULT
CicBridge::EscHanjaMode(TLS *pTLS, HIMC hIMC, LPVOID lpData)
{
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return TRUE;

    if (pCicIC->m_bCandidateOpen)
        return TRUE;

    pCicIC->m_dwUnknown6_5[4] |= 0x1;

    //FIXME

    pCicIC->m_dwUnknown6_5[4] &= ~0x1;

    return TRUE;
}

/// @implemented
LRESULT
CicBridge::EscapeKorean(TLS *pTLS, HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    if (uSubFunc == IME_ESC_QUERY_SUPPORT)
        return *(DWORD*)lpData == IME_ESC_HANJA_MODE;
    if (uSubFunc == IME_ESC_HANJA_MODE)
        return EscHanjaMode(pTLS, hIMC, lpData);
    return 0;
}

/// @implemented
BOOL CicBridge::IsOwnDim(ITfDocumentMgr *pDocMgr)
{
    DWORD dwDimFlags = 0;
    HRESULT hr = ::GetCompartmentDWORD(pDocMgr, GUID_COMPARTMENT_CTFIME_DIMFLAGS,
                                       &dwDimFlags, FALSE);
    if (FAILED(hr))
        return FALSE;
    return !!(dwDimFlags & 0x1);
}
