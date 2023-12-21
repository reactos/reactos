/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Supporting IME interface of Text Input Processors (TIPs)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

HINSTANCE g_hInst = NULL; /* The instance of this module */
BOOL g_bWinLogon = FALSE;
DWORD g_dwOSInfo = 0;
BOOL gfTFInitLib = FALSE;
CRITICAL_SECTION g_csLock;

DEFINE_GUID(GUID_COMPARTMENT_CTFIME_DIMFLAGS, 0xA94C5FD2, 0xC471, 0x4031, 0x95, 0x46, 0x70, 0x9C, 0x17, 0x30, 0x0C, 0xB9);

EXTERN_C void __cxa_pure_virtual(void)
{
    ERR("__cxa_pure_virtual\n");
}

UINT WM_MSIME_SERVICE = 0;
UINT WM_MSIME_UIREADY = 0;
UINT WM_MSIME_RECONVERTREQUEST = 0;
UINT WM_MSIME_RECONVERT = 0;
UINT WM_MSIME_DOCUMENTFEED = 0;
UINT WM_MSIME_QUERYPOSITION = 0;
UINT WM_MSIME_MODEBIAS = 0;
UINT WM_MSIME_SHOWIMEPAD = 0;
UINT WM_MSIME_MOUSE = 0;
UINT WM_MSIME_KEYMAP = 0;

/**
 * @implemented
 */
BOOL IsMsImeMessage(UINT uMsg)
{
    return (uMsg == WM_MSIME_SERVICE ||
            uMsg == WM_MSIME_UIREADY ||
            uMsg == WM_MSIME_RECONVERTREQUEST ||
            uMsg == WM_MSIME_RECONVERT ||
            uMsg == WM_MSIME_DOCUMENTFEED ||
            uMsg == WM_MSIME_QUERYPOSITION ||
            uMsg == WM_MSIME_MODEBIAS ||
            uMsg == WM_MSIME_SHOWIMEPAD ||
            uMsg == WM_MSIME_MOUSE ||
            uMsg == WM_MSIME_KEYMAP);
}

/**
 * @implemented
 */
BOOL RegisterMSIMEMessage(VOID)
{
    WM_MSIME_SERVICE = RegisterWindowMessageW(L"MSIMEService");
    WM_MSIME_UIREADY = RegisterWindowMessageW(L"MSIMEUIReady");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageW(L"MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageW(L"MSIMEReconvert");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageW(L"MSIMEDocumentFeed");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageW(L"MSIMEQueryPosition");
    WM_MSIME_MODEBIAS = RegisterWindowMessageW(L"MSIMEModeBias");
    WM_MSIME_SHOWIMEPAD = RegisterWindowMessageW(L"MSIMEShowImePad");
    WM_MSIME_MOUSE = RegisterWindowMessageW(L"MSIMEMouseOperation");
    WM_MSIME_KEYMAP = RegisterWindowMessageW(L"MSIMEKeyMap");
    return (WM_MSIME_SERVICE &&
            WM_MSIME_UIREADY &&
            WM_MSIME_RECONVERTREQUEST &&
            WM_MSIME_RECONVERT &&
            WM_MSIME_DOCUMENTFEED &&
            WM_MSIME_QUERYPOSITION &&
            WM_MSIME_MODEBIAS &&
            WM_MSIME_SHOWIMEPAD &&
            WM_MSIME_MOUSE &&
            WM_MSIME_KEYMAP);
}

typedef BOOLEAN (WINAPI *FN_DllShutDownInProgress)(VOID);

EXTERN_C BOOLEAN WINAPI
DllShutDownInProgress(VOID)
{
    HMODULE hNTDLL;
    static FN_DllShutDownInProgress s_fnDllShutDownInProgress = NULL;

    if (s_fnDllShutDownInProgress)
        return s_fnDllShutDownInProgress();

    hNTDLL = cicGetSystemModuleHandle(L"ntdll.dll", FALSE);
    s_fnDllShutDownInProgress =
        (FN_DllShutDownInProgress)GetProcAddress(hNTDLL, "RtlDllShutdownInProgress");
    if (!s_fnDllShutDownInProgress)
        return FALSE;

    return s_fnDllShutDownInProgress();
}

static BOOL
IsInteractiveUserLogon(VOID)
{
    BOOL bOK, IsMember = FALSE;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY IdentAuth = { SECURITY_NT_AUTHORITY };

    if (!AllocateAndInitializeSid(&IdentAuth, 1, SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pSid))
    {
        ERR("Error: %ld\n", GetLastError());
        return FALSE;
    }

    bOK = CheckTokenMembership(NULL, pSid, &IsMember);

    if (pSid)
        FreeSid(pSid);

    return bOK && IsMember;
}

typedef struct LIBTHREAD
{
    IUnknown *m_pUnknown1;
    ITfDisplayAttributeMgr *m_pDisplayAttrMgr;
} LIBTHREAD, *PLIBTHREAD;

HRESULT InitDisplayAttrbuteLib(PLIBTHREAD pLibThread)
{
    if (!pLibThread)
        return E_FAIL;

    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }

    //FIXME
    return E_NOTIMPL;
}

HRESULT UninitDisplayAttrbuteLib(PLIBTHREAD pLibThread)
{
    if (!pLibThread)
        return E_FAIL;

    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }

    return S_OK;
}

void TFUninitLib_Thread(PLIBTHREAD pLibThread)
{
    if (!pLibThread)
        return;

    if (pLibThread->m_pUnknown1)
    {
        pLibThread->m_pUnknown1->Release();
        pLibThread->m_pUnknown1 = NULL;
    }
    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }
}

/***********************************************************************
 *      Compartment
 */

/**
 * @implemented
 */
HRESULT
GetCompartment(
    IUnknown *pUnknown,
    REFGUID rguid,
    ITfCompartment **ppComp,
    BOOL bThread)
{
    *ppComp = NULL;

    ITfThreadMgr *pThreadMgr = NULL;
    ITfCompartmentMgr *pCompMgr = NULL;

    HRESULT hr;
    if (bThread)
    {
        hr = pUnknown->QueryInterface(IID_ITfThreadMgr, (void **)&pThreadMgr);
        if (FAILED(hr))
            return hr;

        hr = pThreadMgr->GetGlobalCompartment(&pCompMgr);
    }
    else
    {
        hr = pUnknown->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr);
    }

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (pCompMgr)
        {
            hr = pCompMgr->GetCompartment(rguid, ppComp);
            pCompMgr->Release();
        }
    }

    if (pThreadMgr)
        pThreadMgr->Release();

    return hr;
}

/**
 * @implemented
 */
HRESULT
SetCompartmentDWORD(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    DWORD dwValue,
    BOOL bThread)
{
    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, bThread);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    V_I4(&vari) = dwValue;
    V_VT(&vari) = VT_I4;
    hr = pComp->SetValue(cookie, &vari);

    pComp->Release();
    return hr;
}

/**
 * @implemented
 */
HRESULT
GetCompartmentDWORD(
    IUnknown *pUnknown,
    REFGUID rguid,
    LPDWORD pdwValue,
    BOOL bThread)
{
    *pdwValue = 0;

    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, bThread);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    hr = pComp->GetValue(&vari);
    if (hr == S_OK)
        *pdwValue = V_I4(&vari);

    pComp->Release();
    return hr;
}

/**
 * @implemented
 */
HRESULT
SetCompartmentUnknown(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    IUnknown *punkValue)
{
    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, FALSE);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    V_UNKNOWN(&vari) = punkValue;
    V_VT(&vari) = VT_UNKNOWN;
    hr = pComp->SetValue(cookie, &vari);

    pComp->Release();
    return hr;
}

/**
 * @implemented
 */
HRESULT
ClearCompartment(
    TfClientId tid,
    IUnknown *pUnknown,
    REFGUID rguid,
    BOOL bThread)
{
    ITfCompartmentMgr *pCompMgr = NULL;
    ITfThreadMgr *pThreadMgr = NULL;

    HRESULT hr;
    if (bThread)
    {
        hr = pUnknown->QueryInterface(IID_ITfThreadMgr, (void **)&pThreadMgr);
        if (FAILED(hr))
            return hr;

        hr = pThreadMgr->GetGlobalCompartment(&pCompMgr);
    }
    else
    {
        hr = pUnknown->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr);
    }

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (pCompMgr)
        {
            hr = pCompMgr->ClearCompartment(tid, rguid);
            pCompMgr->Release();
        }
    }

    if (pThreadMgr)
        pThreadMgr->Release();

    return hr;
}

typedef struct CESMAP
{
    ITfCompartment *m_pComp;
    DWORD m_dwCookie;
} CESMAP, *PCESMAP;

typedef INT (CALLBACK *FN_EVENTSINK)(LPVOID, REFGUID);

class CCompartmentEventSink : public ITfCompartmentEventSink
{
    CicTypedArray<CESMAP> m_array;
    LONG m_cRefs;
    FN_EVENTSINK m_fnEventSink;
    LPVOID m_pUserData;

public:
    CCompartmentEventSink(FN_EVENTSINK fnEventSink, LPVOID pUserData);
    virtual ~CCompartmentEventSink();

    HRESULT _Advise(IUnknown *pUnknown, REFGUID rguid, BOOL bThread);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfCompartmentEventSink interface
    STDMETHODIMP OnChange(REFGUID rguid) override;
};

/**
 * @implemented
 */
CCompartmentEventSink::CCompartmentEventSink(FN_EVENTSINK fnEventSink, LPVOID pUserData)
    : m_array()
    , m_cRefs(1)
    , m_fnEventSink(fnEventSink)
    , m_pUserData(pUserData)
{
}

/**
 * @implemented
 */
CCompartmentEventSink::~CCompartmentEventSink()
{
}

/**
 * @implemented
 */
STDMETHODIMP CCompartmentEventSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCompartmentEventSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CCompartmentEventSink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CCompartmentEventSink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/**
 * @implemented
 */
STDMETHODIMP CCompartmentEventSink::OnChange(REFGUID rguid)
{
    return m_fnEventSink(m_pUserData, rguid);
}

/**
 * @implemented
 */
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

/**
 * @implemented
 */
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

/***********************************************************************
 *      CicInputContext
 */

class CInputContextOwnerCallBack;

/* FIXME */
class CicInputContext
    : public ITfCleanupContextSink
    , public ITfContextOwnerCompositionSink
    , public ITfCompositionSink
{
public:
    DWORD m_dw[2];
    LONG m_cRefs;
    HIMC m_hIMC;
    ITfDocumentMgr *m_pDocumentMgr;
    ITfContext *m_pContext;
    DWORD m_dw0_0[1];
    CInputContextOwnerCallBack *m_pICOwnerCallback;
    DWORD m_dw0;
    CCompartmentEventSink *m_pCompEventSink1;
    CCompartmentEventSink *m_pCompEventSink2;
    DWORD m_dw0_5[4];
    DWORD m_dw1[2];
    DWORD m_dwQueryPos;
    DWORD m_dw1_5[1];
    GUID m_guid;
    DWORD m_dw2[19];
    WORD m_cGuidAtoms;
    WORD m_padding;
    DWORD m_adwGuidAtoms[256];
    DWORD m_dw3[19];

public:
    CicInputContext(TfClientId cliendId, LIBTHREAD *pLibThread, HIMC hIMC);
    virtual ~CicInputContext()
    {
    }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfCleanupContextSink interface
    STDMETHODIMP OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic) override;

    // ITfContextOwnerCompositionSink interface
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk) override;
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew) override;
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition) override;

    // ITfCompositionSink interface
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition) override;

    HRESULT
    GetGuidAtom(
        _Inout_ IMCLock& imcLock,
        _In_ BYTE iAtom,
        _Out_opt_ LPDWORD pdwGuidAtom);

    HRESULT CreateInputContext(ITfThreadMgr *pThreadMgr, IMCLock& imcLock);
    HRESULT DestroyInputContext();
};

/**
 * @unimplemented
 */
CicInputContext::CicInputContext(TfClientId cliendId, LIBTHREAD *pLibThread, HIMC hIMC)
{
    m_hIMC = hIMC;
    m_guid = GUID_NULL;
    m_dwQueryPos = 0;
    m_cRefs = 1;
}

/**
 * @unimplemented
 */
STDMETHODIMP CicInputContext::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
    {
        *ppvObj = (ITfContextOwnerCompositionSink*)this;
        AddRef();
        return S_OK;
    }
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicInputContext::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicInputContext::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/**
 * @unimplemented
 */
STDMETHODIMP
CicInputContext::OnStartComposition(
    ITfCompositionView *pComposition,
    BOOL *pfOk)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
STDMETHODIMP
CicInputContext::OnUpdateComposition(
    ITfCompositionView *pComposition,
    ITfRange *pRangeNew)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
STDMETHODIMP
CicInputContext::OnEndComposition(
    ITfCompositionView *pComposition)
{
    return E_NOTIMPL;
}

/**
 * @implemented
 */
HRESULT
CicInputContext::GetGuidAtom(
    _Inout_ IMCLock& imcLock,
    _In_ BYTE iAtom,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    IMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCompStr);
    HRESULT hr = imeContext.m_hr;
    if (!imeContext)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    hr = E_FAIL;
    if (iAtom < m_cGuidAtoms)
    {
        *pdwGuidAtom = m_adwGuidAtoms[iAtom];
        hr = S_OK;
    }

    return hr;
}

/**
 * @unimplemented
 */
HRESULT
CicInputContext::CreateInputContext(ITfThreadMgr *pThreadMgr, IMCLock& imcLock)
{
    //FIXME
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
HRESULT
CicInputContext::DestroyInputContext()
{
    // FIXME
    return E_NOTIMPL;
}

/**
 * @implemented
 */
STDMETHODIMP
CicInputContext::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition)
{
    return S_OK;
}

/**
 * @implemented
 */
HRESULT
Inquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags,
    _In_ HKL hKL)
{
    if (!lpIMEInfo)
        return E_OUTOFMEMORY;

    StringCchCopyW(lpszWndClass, 64, L"MSCTFIME UI");
    lpIMEInfo->dwPrivateDataSize = 0;

    switch (LOWORD(hKL)) // Language ID
    {
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT): // Japanese
        {
            lpIMEInfo->fdwProperty = IME_PROP_COMPLETE_ON_UNSELECT | IME_PROP_SPECIAL_UI |
                                     IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY |
                                     IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_KATAKANA |
                                           IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = IME_SMODE_CONVERSATION | IME_SMODE_PLAURALCLAUSE;
            lpIMEInfo->fdwSelectCaps = SELECT_CAP_SENTENCE | SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD |
                                    SCS_CAP_COMPSTR;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        case MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT): // Korean
        {
            lpIMEInfo->fdwProperty = IME_PROP_COMPLETE_ON_UNSELECT | IME_PROP_SPECIAL_UI |
                                     IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY |
                                     IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = 0;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_COMPSTR;
            lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED): // Simplified Chinese
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL): // Traditional Chinese
        {
            lpIMEInfo->fdwProperty = IME_PROP_SPECIAL_UI | IME_PROP_AT_CARET |
                                     IME_PROP_NEED_ALTKEY | IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwSelectCaps = 0;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD |
                                    SCS_CAP_COMPSTR;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        default: // Otherwise
        {
            lpIMEInfo->fdwProperty = IME_PROP_UNICODE | IME_PROP_AT_CARET;
            lpIMEInfo->fdwConversionCaps = 0;
            lpIMEInfo->fdwSentenceCaps = 0;
            lpIMEInfo->fdwSCSCaps = 0;
            lpIMEInfo->fdwUICaps = 0;
            lpIMEInfo->fdwSelectCaps = 0;
            break;
        }
    }

    return S_OK;
}

DEFINE_GUID(IID_ITfSysHookSink, 0x495388DA, 0x21A5, 0x4852, 0x8B, 0xB1, 0xED, 0x2F, 0x29, 0xDA, 0x8D, 0x60);

struct ITfSysHookSink : IUnknown
{
    STDMETHOD(OnPreFocusDIM)(HWND hwnd) = 0;
    STDMETHOD(OnSysKeyboardProc)(UINT, LONG) = 0;
    STDMETHOD(OnSysShellProc)(INT, UINT, LONG) = 0;
};

class TLS;

typedef INT (CALLBACK *FN_INITDOCMGR)(UINT, ITfDocumentMgr *, ITfDocumentMgr *, LPVOID);
typedef INT (CALLBACK *FN_PUSHPOP)(UINT, ITfContext *, LPVOID);

class CThreadMgrEventSink : public ITfThreadMgrEventSink
{
protected:
    ITfThreadMgr *m_pThreadMgr;
    DWORD m_dwCookie;
    FN_INITDOCMGR m_fnInit;
    FN_PUSHPOP m_fnPushPop;
    DWORD m_dw;
    LPVOID m_pCallbackPV;
    LONG m_cRefs;

public:
    CThreadMgrEventSink(
        FN_INITDOCMGR fnInit,
        FN_PUSHPOP fnPushPop = NULL,
        LPVOID pvCallbackPV = NULL);
    virtual ~CThreadMgrEventSink() { }

    void SetCallbackPV(LPVOID pv);
    HRESULT _Advise(ITfThreadMgr *pThreadMgr);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfThreadMgrEventSink interface
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus) override;
    STDMETHODIMP OnPushContext(ITfContext *pic) override;
    STDMETHODIMP OnPopContext(ITfContext *pic) override;

    static INT CALLBACK DIMCallback(
        UINT nCode,
        ITfDocumentMgr *pDocMgr1,
        ITfDocumentMgr *pDocMgr2,
        LPVOID pUserData);
};

/**
 * @implemented
 */
CThreadMgrEventSink::CThreadMgrEventSink(
    FN_INITDOCMGR fnInit,
    FN_PUSHPOP fnPushPop,
    LPVOID pvCallbackPV)
{
    m_fnInit = fnInit;
    m_fnPushPop = fnPushPop;
    m_pCallbackPV = pvCallbackPV;
    m_cRefs = 1;
}

/**
 * @implemented
 */
STDMETHODIMP CThreadMgrEventSink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CThreadMgrEventSink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
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

void CThreadMgrEventSink::SetCallbackPV(LPVOID pv)
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

/* FIXME */
class CFunctionProvider : public IUnknown
{
public:
    CFunctionProvider(TfClientId clientId)
    {
    }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
};

/**
 * @unimplemented
 */
STDMETHODIMP CFunctionProvider::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
STDMETHODIMP_(ULONG) CFunctionProvider::AddRef()
{
    return 1;
}

/**
 * @unimplemented
 */
STDMETHODIMP_(ULONG) CFunctionProvider::Release()
{
    return 0;
}

/* FIXME */
class CicBridge : public ITfSysHookSink
{
protected:
    LONG m_cRefs;
    DWORD m_dwImmxInit;
    DWORD m_dw[2];
    DWORD m_cActivateLocks;
    ITfKeystrokeMgr *m_pKeystrokeMgr;
    ITfDocumentMgr *m_pDocMgr;
    CThreadMgrEventSink *m_pThreadMgrEventSink;
    TfClientId m_cliendId;
    LIBTHREAD m_LibThread;
    DWORD m_dw21;

    static BOOL CALLBACK EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam);
    static BOOL CALLBACK EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam);

public:
    CicBridge();
    virtual ~CicBridge();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfSysHookSink interface
    STDMETHODIMP OnPreFocusDIM(HWND hwnd) override;
    STDMETHODIMP OnSysKeyboardProc(UINT, LONG) override;
    STDMETHODIMP OnSysShellProc(INT, UINT, LONG) override;

    HRESULT InitIMMX(TLS *pTLS);
    BOOL UnInitIMMX(TLS *pTLS);
    HRESULT ActivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr);
    HRESULT DeactivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr);

    HRESULT CreateInputContext(TLS *pTLS, HIMC hIMC);
    HRESULT DestroyInputContext(TLS *pTLS, HIMC hIMC);

    void PostTransMsg(HWND hWnd, INT cTransMsgs, LPTRANSMSG pTransMsgs);
    void GetDocumentManager(IMCCLock<CTFIMECONTEXT>& imeContext);

    HRESULT ConfigureGeneral(TLS* pTLS, ITfThreadMgr *pThreadMgr, HKL hKL, HWND hWnd);
    HRESULT ConfigureRegisterWord(TLS* pTLS, ITfThreadMgr *pThreadMgr, HKL hKL, HWND hWnd, LPVOID lpData);
};

class CActiveLanguageProfileNotifySink : public ITfActiveLanguageProfileNotifySink
{
protected:
    typedef INT (CALLBACK *FN_COMPARE)(REFGUID rguid1, REFGUID rguid2, BOOL fActivated, LPVOID pUserData);
    LONG m_cRefs;
    ITfThreadMgr *m_pThreadMgr;
    DWORD m_dwConnection;
    FN_COMPARE m_fnCompare;
    LPVOID m_pUserData;

public:
    CActiveLanguageProfileNotifySink(FN_COMPARE fnCompare, void *pUserData);
    virtual ~CActiveLanguageProfileNotifySink();

    HRESULT _Advise(ITfThreadMgr *pThreadMgr);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfActiveLanguageProfileNotifySink interface
    STDMETHODIMP
    OnActivated(
        REFCLSID clsid,
        REFGUID guidProfile,
        BOOL fActivated) override;
};

/**
 * @implemented
 */
CActiveLanguageProfileNotifySink::CActiveLanguageProfileNotifySink(
    FN_COMPARE fnCompare,
    void *pUserData)
{
    m_dwConnection = (DWORD)-1;
    m_fnCompare = fnCompare;
    m_cRefs = 1;
    m_pUserData = pUserData;
}

/**
 * @implemented
 */
CActiveLanguageProfileNotifySink::~CActiveLanguageProfileNotifySink()
{
}

/**
 * @implemented
 */
STDMETHODIMP CActiveLanguageProfileNotifySink::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CActiveLanguageProfileNotifySink::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CActiveLanguageProfileNotifySink::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/**
 * @implemented
 */
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

/**
 * @implemented
 */
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

/**
 * @implemented
 */
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

/* FIXME */
class CicProfile : public IUnknown
{
protected:
    ITfInputProcessorProfiles *m_pIPProfiles;
    CActiveLanguageProfileNotifySink *m_pActiveLanguageProfileNotifySink;
    LANGID  m_LangID1;
    WORD    m_padding1;
    DWORD   m_dwFlags;
    UINT    m_nCodePage;
    LANGID  m_LangID2;
    WORD    m_padding2;
    DWORD   m_dw3[1];
    LONG    m_cRefs;

    static INT CALLBACK
    ActiveLanguageProfileNotifySinkCallback(
        REFGUID rguid1,
        REFGUID rguid2,
        BOOL fActivated,
        LPVOID pUserData);

public:
    CicProfile();
    virtual ~CicProfile();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    HRESULT GetActiveLanguageProfile(HKL hKL, REFGUID rguid, TF_LANGUAGEPROFILE *pProfile);
    HRESULT GetLangId(LANGID *pLangID);
    HRESULT GetCodePageA(UINT *puCodePage);

    HRESULT InitProfileInstance(TLS *pTLS);
};

/**
 * @implemented
 */
CicProfile::CicProfile()
{
    m_dwFlags &= 0xFFFFFFF0;
    m_cRefs = 1;
    m_pIPProfiles = NULL;
    m_pActiveLanguageProfileNotifySink = NULL;
    m_LangID1 = 0;
    m_nCodePage = CP_ACP;
    m_LangID2 = 0;
    m_dw3[0] = 0;
}

/**
 * @implemented
 */
CicProfile::~CicProfile()
{
    if (m_pIPProfiles)
    {
        if (m_LangID1)
            m_pIPProfiles->ChangeCurrentLanguage(m_LangID1);

        m_pIPProfiles->Release();
        m_pIPProfiles = NULL;
    }

    if (m_pActiveLanguageProfileNotifySink)
    {
        m_pActiveLanguageProfileNotifySink->_Unadvise();
        m_pActiveLanguageProfileNotifySink->Release();
        m_pActiveLanguageProfileNotifySink = NULL;
    }
}

/**
 * @implemented
 */
STDMETHODIMP CicProfile::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicProfile::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicProfile::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/**
 * @implemented
 */
INT CALLBACK
CicProfile::ActiveLanguageProfileNotifySinkCallback(
    REFGUID rguid1,
    REFGUID rguid2,
    BOOL fActivated,
    LPVOID pUserData)
{
    CicProfile *pThis = (CicProfile *)pUserData;
    pThis->m_dwFlags &= ~0xE;
    return 0;
}

/**
 * @implemented
 */
HRESULT CicProfile::GetCodePageA(UINT *puCodePage)
{
    if (!puCodePage)
        return E_INVALIDARG;

    if (m_dwFlags & 2)
    {
        *puCodePage = m_nCodePage;
        return S_OK;
    }

    *puCodePage = 0;

    LANGID LangID;
    HRESULT hr = GetLangId(&LangID);
    if (FAILED(hr))
        return E_FAIL;

    WCHAR szBuff[12];
    INT cch = ::GetLocaleInfoW(LangID, LOCALE_IDEFAULTANSICODEPAGE, szBuff, _countof(szBuff));
    if (cch)
    {
        szBuff[cch] = 0;
        m_nCodePage = *puCodePage = wcstoul(szBuff, NULL, 10);
        m_dwFlags |= 2;
    }

    return S_OK;
}

/**
 * @implemented
 */
HRESULT CicProfile::GetLangId(LANGID *pLangID)
{
    *pLangID = 0;

    if (!m_pIPProfiles)
        return E_FAIL;

    if (m_dwFlags & 4)
    {
        *pLangID = m_LangID2;
        return S_OK;
    }

    HRESULT hr = m_pIPProfiles->GetCurrentLanguage(pLangID);
    if (SUCCEEDED(hr))
    {
        m_dwFlags |= 4;
        m_LangID2 = *pLangID;
    }

    return hr;
}

class TLS
{
public:
    static DWORD s_dwTlsIndex;

    DWORD m_dwSystemInfoFlags;
    CicBridge *m_pBridge;
    CicProfile *m_pProfile;
    ITfThreadMgr *m_pThreadMgr;
    DWORD m_dwFlags1;
    DWORD m_dwFlags2;
    DWORD m_dwUnknown2[2];
    DWORD m_dwNowOpening;
    DWORD m_NonEAComposition;
    DWORD m_cWnds;

    /**
     * @implemented
     */
    static BOOL Initialize()
    {
        s_dwTlsIndex = ::TlsAlloc();
        return s_dwTlsIndex != (DWORD)-1;
    }

    /**
     * @implemented
     */
    static VOID Uninitialize()
    {
        if (s_dwTlsIndex != (DWORD)-1)
        {
            ::TlsFree(s_dwTlsIndex);
            s_dwTlsIndex = (DWORD)-1;
        }
    }

    /**
     * @implemented
     */
    static TLS* GetTLS()
    {
        if (s_dwTlsIndex == (DWORD)-1)
            return NULL;

        return InternalAllocateTLS();
    }

    /**
     * @implemented
     */
    static TLS* PeekTLS()
    {
        return (TLS*)::TlsGetValue(TLS::s_dwTlsIndex);
    }

    static TLS* InternalAllocateTLS();
    static BOOL InternalDestroyTLS();

};

DWORD TLS::s_dwTlsIndex = (DWORD)-1;

/**
 * @implemented
 */
TLS* TLS::InternalAllocateTLS()
{
    TLS *pTLS = TLS::PeekTLS();
    if (pTLS)
        return pTLS;

    if (DllShutDownInProgress())
        return NULL;

    pTLS = (TLS *)cicMemAllocClear(sizeof(TLS));
    if (!pTLS)
        return NULL;

    if (!::TlsSetValue(s_dwTlsIndex, pTLS))
    {
        cicMemFree(pTLS);
        return NULL;
    }

    pTLS->m_dwUnknown2[0] |= 1;
    pTLS->m_dwUnknown2[2] |= 1;
    return pTLS;
}

/**
 * @implemented
 */
BOOL TLS::InternalDestroyTLS()
{
    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS)
        return FALSE;

    if (pTLS->m_pBridge)
        pTLS->m_pBridge->Release();
    if (pTLS->m_pProfile)
        pTLS->m_pProfile->Release();
    if (pTLS->m_pThreadMgr)
        pTLS->m_pThreadMgr->Release();

    cicMemFree(pTLS);
    ::TlsSetValue(s_dwTlsIndex, NULL);
    return TRUE;
}

/**
 * @implemented
 */
HRESULT
CicProfile::InitProfileInstance(TLS *pTLS)
{
    HRESULT hr = TF_CreateInputProcessorProfiles(&m_pIPProfiles);
    if (FAILED(hr))
        return hr;

    if (!m_pActiveLanguageProfileNotifySink)
    {
        CActiveLanguageProfileNotifySink *pSink =
            new CActiveLanguageProfileNotifySink(
                CicProfile::ActiveLanguageProfileNotifySinkCallback, this);
        if (!pSink)
        {
            m_pIPProfiles->Release();
            m_pIPProfiles = NULL;
            return E_FAIL;
        }
        m_pActiveLanguageProfileNotifySink = pSink;
    }

    if (pTLS->m_pThreadMgr)
        m_pActiveLanguageProfileNotifySink->_Advise(pTLS->m_pThreadMgr);

    return hr;
}

/**
 * @implemented
 */
STDMETHODIMP CicInputContext::OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic)
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

/***********************************************************************
 *      CicBridge
 */

CicBridge::CicBridge()
{
    m_dwImmxInit &= ~1;
    m_dw[0] &= ~1;
    m_dw[1] &= ~1;
    m_dw21 &= ~1;
    m_pKeystrokeMgr = NULL;
    m_pDocMgr = NULL;
    m_pThreadMgrEventSink = NULL;
    m_cliendId = 0;
    m_cRefs = 1;
}

/**
 * @implemented
 */
STDMETHODIMP CicBridge::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;

    if (!IsEqualIID(riid, IID_ITfSysHookSink))
        return E_NOINTERFACE;

    *ppvObj = this;
    AddRef();

    return S_OK;
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicBridge::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/**
 * @implemented
 */
STDMETHODIMP_(ULONG) CicBridge::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/**
 * @implemented
 */
CicBridge::~CicBridge()
{
    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS || !pTLS->m_pThreadMgr)
        return;

    if (SUCCEEDED(DeactivateIMMX(pTLS, pTLS->m_pThreadMgr)))
        UnInitIMMX(pTLS);
}

void CicBridge::GetDocumentManager(IMCCLock<CTFIMECONTEXT>& imeContext)
{
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
    {
        m_pDocMgr = pCicIC->m_pDocumentMgr;
        m_pDocMgr->AddRef();
    }
    else
    {
        m_pDocMgr->Release();
        m_pDocMgr = NULL;
    }
}

/**
 * @unimplemented
 */
HRESULT CicBridge::CreateInputContext(TLS *pTLS, HIMC hIMC)
{
    IMCLock imcLock(hIMC);
    HRESULT hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    if (!imcLock.get().hCtfImeContext)
    {
        HIMCC hCtfImeContext = ImmCreateIMCC(sizeof(CTFIMECONTEXT));
        if (!hCtfImeContext)
            return E_OUTOFMEMORY;
        imcLock.get().hCtfImeContext = hCtfImeContext;
    }

    IMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
    {
        pCicIC = new CicInputContext(m_cliendId, &m_LibThread, hIMC);
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
    }

    hr = pCicIC->CreateInputContext(pTLS->m_pThreadMgr, imcLock);
    if (FAILED(hr))
    {
        pCicIC->Release();
        imeContext.get().m_pCicIC = NULL;
    }
    else
    {
        if (imcLock.get().hWnd && imcLock.get().hWnd == ::GetFocus())
        {
            GetDocumentManager(imeContext);
            //FIXME
        }
    }

    return E_NOTIMPL;
}

/**
 * @implemented
 */
HRESULT CicBridge::DestroyInputContext(TLS *pTLS, HIMC hIMC)
{
    IMCLock imcLock(hIMC);
    HRESULT hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    hr = E_FAIL;
    IMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
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

typedef struct ENUM_CREATE_DESTROY_IC
{
    TLS *m_pTLS;
    CicBridge *m_pBridge;
} ENUM_CREATE_DESTROY_IC, *PENUM_CREATE_DESTROY_IC;

BOOL CALLBACK CicBridge::EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    PENUM_CREATE_DESTROY_IC pData = (PENUM_CREATE_DESTROY_IC)lParam;
    pData->m_pBridge->CreateInputContext(pData->m_pTLS, hIMC);
    return TRUE;
}

BOOL CALLBACK CicBridge::EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    PENUM_CREATE_DESTROY_IC pData = (PENUM_CREATE_DESTROY_IC)lParam;
    pData->m_pBridge->DestroyInputContext(pData->m_pTLS, hIMC);
    return TRUE;
}

/**
 * @unimplemented
 */
HRESULT CicBridge::ActivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr)
{
    //FIXME

    if (m_cActivateLocks++ != 0)
        return S_OK;

    ITfSourceSingle *pSource = NULL;
    HRESULT hr = pThreadMgr->QueryInterface(IID_ITfSourceSingle, (void**)&pSource);
    if (FAILED(hr))
    {
        DeactivateIMMX(pTLS, pThreadMgr);
        return hr;
    }

    CFunctionProvider *pProvider = new CFunctionProvider(m_cliendId);
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

    //FIXME

    hr = S_OK;
    if (pTLS->m_dwUnknown2[1] & 1)
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

/**
 * @unimplemented
 */
HRESULT CicBridge::DeactivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr)
{
    if (m_dw[1] & 1)
        return TRUE;

    m_dw[1] |= 1;

    if (m_cliendId)
    {
        ENUM_CREATE_DESTROY_IC Data = { pTLS, this };
        ImmEnumInputContext(0, CicBridge::EnumDestroyInputContextCallback, (LPARAM)&Data);
        pTLS->m_dwUnknown2[1] |= 1u;

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

    //FIXME

    m_dw[1] &= ~1;

    return S_OK;
}

/**
 * @implemented
 */
HRESULT CicBridge::InitIMMX(TLS *pTLS)
{
    if (m_dwImmxInit & 1)
        return S_OK;

    HRESULT hr;
    if (!pTLS->m_pThreadMgr)
    {
        hr = TF_CreateThreadMgr(&pTLS->m_pThreadMgr);
        if (FAILED(hr))
            return E_FAIL;

        hr = pTLS->m_pThreadMgr->QueryInterface(IID_ITfThreadMgr, (void **)&pTLS->m_pThreadMgr);
        if (FAILED(hr))
        {
            pTLS->m_pThreadMgr->Release();
            pTLS->m_pThreadMgr = NULL;
            return E_FAIL;
        }
    }

    if (!m_pThreadMgrEventSink)
    {
        m_pThreadMgrEventSink =
            new CThreadMgrEventSink(CThreadMgrEventSink::DIMCallback, NULL, NULL);
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
        pTLS->m_pProfile = new CicProfile();
        if (!pTLS->m_pProfile)
            return E_OUTOFMEMORY;
        hr = pTLS->m_pProfile->InitProfileInstance(pTLS);
        if (FAILED(hr))
        {
            UnInitIMMX(pTLS);
            return E_FAIL;
        }
    }

    hr = pTLS->m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&m_pKeystrokeMgr);
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

    m_dwImmxInit |= 1;
    return S_OK;
}

/**
 * @implemented
 */
BOOL CicBridge::UnInitIMMX(TLS *pTLS)
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

    m_dwImmxInit &= ~1;
    return TRUE;
}

/**
 * @implemented
 */
STDMETHODIMP CicBridge::OnPreFocusDIM(HWND hwnd)
{
    return S_OK;
}

/**
 * @unimplemented
 */
STDMETHODIMP CicBridge::OnSysKeyboardProc(UINT, LONG)
{
    return E_NOTIMPL;
}

/**
 * @implemented
 */
STDMETHODIMP CicBridge::OnSysShellProc(INT, UINT, LONG)
{
    return S_OK;
}

/**
 * @implemented
 */
void CicBridge::PostTransMsg(HWND hWnd, INT cTransMsgs, LPTRANSMSG pTransMsgs)
{
    for (INT i = 0; i < cTransMsgs; ++i, ++pTransMsgs)
    {
        ::PostMessageW(hWnd, pTransMsgs->message, pTransMsgs->wParam, pTransMsgs->lParam);
    }
}

/**
 * @implemented
 */
HRESULT
CicBridge::ConfigureGeneral(
    TLS* pTLS,
    ITfThreadMgr *pThreadMgr,
    HKL hKL,
    HWND hWnd)
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

/**
 * @implemented
 */
HRESULT
CicBridge::ConfigureRegisterWord(
    TLS* pTLS,
    ITfThreadMgr *pThreadMgr,
    HKL hKL,
    HWND hWnd,
    LPVOID lpData)
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

/***********************************************************************
 *      CicProfile
 */

/**
 * @unimplemented
 */
HRESULT
CicProfile::GetActiveLanguageProfile(
    HKL hKL,
    REFGUID rguid,
    TF_LANGUAGEPROFILE *pProfile)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *      ImeInquire (MSCTFIME.@)
 *
 * MSCTFIME's ImeInquire does nothing.
 *
 * @implemented
 * @see CtfImeInquireExW
 */
EXTERN_C
BOOL WINAPI
ImeInquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags)
{
    TRACE("(%p, %p, 0x%lX)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags);
    return FALSE;
}

/***********************************************************************
 *      ImeConversionList (MSCTFIME.@)
 *
 * MSCTFIME's ImeConversionList does nothing.
 *
 * @implemented
 * @see ImmGetConversionListW
 */
EXTERN_C DWORD WINAPI
ImeConversionList(
    _In_ HIMC hIMC,
    _In_ LPCWSTR lpSrc,
    _Out_ LPCANDIDATELIST lpDst,
    _In_ DWORD dwBufLen,
    _In_ UINT uFlag)
{
    TRACE("(%p, %s, %p, 0x%lX, %u)\n", hIMC, debugstr_w(lpSrc), lpDst, dwBufLen, uFlag);
    return 0;
}

/***********************************************************************
 *      ImeRegisterWord (MSCTFIME.@)
 *
 * MSCTFIME's ImeRegisterWord does nothing.
 *
 * @implemented
 * @see ImeUnregisterWord
 */
EXTERN_C BOOL WINAPI
ImeRegisterWord(
    _In_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCWSTR lpszString)
{
    TRACE("(%s, 0x%lX, %s)\n", debugstr_w(lpszReading), dwStyle, debugstr_w(lpszString));
    return FALSE;
}

/***********************************************************************
 *      ImeUnregisterWord (MSCTFIME.@)
 *
 * MSCTFIME's ImeUnregisterWord does nothing.
 *
 * @implemented
 * @see ImeRegisterWord
 */
EXTERN_C BOOL WINAPI
ImeUnregisterWord(
    _In_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_ LPCWSTR lpszString)
{
    TRACE("(%s, 0x%lX, %s)\n", debugstr_w(lpszReading), dwStyle, debugstr_w(lpszString));
    return FALSE;
}

/***********************************************************************
 *      ImeGetRegisterWordStyle (MSCTFIME.@)
 *
 * MSCTFIME's ImeGetRegisterWordStyle does nothing.
 *
 * @implemented
 * @see ImeRegisterWord
 */
EXTERN_C UINT WINAPI
ImeGetRegisterWordStyle(
    _In_ UINT nItem,
    _Out_ LPSTYLEBUFW lpStyleBuf)
{
    TRACE("(%u, %p)\n", nItem, lpStyleBuf);
    return 0;
}

/***********************************************************************
 *      ImeEnumRegisterWord (MSCTFIME.@)
 *
 * MSCTFIME's ImeEnumRegisterWord does nothing.
 *
 * @implemented
 * @see ImeRegisterWord
 */
EXTERN_C UINT WINAPI
ImeEnumRegisterWord(
    _In_ REGISTERWORDENUMPROCW lpfnEnumProc,
    _In_opt_ LPCWSTR lpszReading,
    _In_ DWORD dwStyle,
    _In_opt_ LPCWSTR lpszString,
    _In_opt_ LPVOID lpData)
{
    TRACE("(%p, %s, %lu, %s, %p)\n", lpfnEnumProc, debugstr_w(lpszReading),
          dwStyle, debugstr_w(lpszString), lpData);
    return 0;
}

EXTERN_C BOOL WINAPI
ImeConfigure(
    _In_ HKL hKL,
    _In_ HWND hWnd,
    _In_ DWORD dwMode,
    _Inout_opt_ LPVOID lpData)
{
    TRACE("(%p, %p, %lu, %p)\n", hKL, hWnd, dwMode, lpData);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->m_pBridge || !pTLS->m_pThreadMgr)
        return FALSE;

    CicBridge *pBridge = pTLS->m_pBridge;
    ITfThreadMgr *pThreadMgr = pTLS->m_pThreadMgr;

    if (dwMode & 1)
        return (pBridge->ConfigureGeneral(pTLS, pThreadMgr, hKL, hWnd) == S_OK);

    if (dwMode & 2)
        return (pBridge->ConfigureRegisterWord(pTLS, pThreadMgr, hKL, hWnd, lpData) == S_OK);

    return FALSE;
}

/***********************************************************************
 *      ImeDestroy (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
ImeDestroy(
    _In_ UINT uReserved)
{
    TRACE("(%u)\n", uReserved);

    TLS *pTLS = TLS::PeekTLS();
    if (pTLS)
        return FALSE;

    if (!pTLS->m_pBridge || !pTLS->m_pThreadMgr)
        return FALSE;

    if (pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON)
        return TRUE;

    if (pTLS->m_pBridge->DeactivateIMMX(pTLS, pTLS->m_pThreadMgr) != S_OK)
        return FALSE;

    return pTLS->m_pBridge->UnInitIMMX(pTLS);
}

/***********************************************************************
 *      ImeEscape (MSCTFIME.@)
 *
 * MSCTFIME's ImeEscape does nothing.
 *
 * @implemented
 * @see CtfImeEscapeEx
 */
EXTERN_C LRESULT WINAPI
ImeEscape(
    _In_ HIMC hIMC,
    _In_ UINT uEscape,
    _Inout_opt_ LPVOID lpData)
{
    TRACE("(%p, %u, %p)\n", hIMC, uEscape, lpData);
    return 0;
}

EXTERN_C BOOL WINAPI
ImeProcessKey(
    _In_ HIMC hIMC,
    _In_ UINT uVirKey,
    _In_ LPARAM lParam,
    _In_ CONST LPBYTE lpbKeyState)
{
    FIXME("stub:(%p, %u, %p, lpbKeyState)\n", hIMC, uVirKey, lParam, lpbKeyState);
    return FALSE;
}

/***********************************************************************
 *      ImeSelect (MSCTFIME.@)
 *
 * MSCTFIME's ImeSelect does nothing.
 *
 * @implemented
 * @see CtfImeSelectEx
 */
EXTERN_C BOOL WINAPI
ImeSelect(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect)
{
    TRACE("(%p, %u)\n", hIMC, fSelect);
    return FALSE;
}

/***********************************************************************
 *      ImeSetActiveContext (MSCTFIME.@)
 *
 * MSCTFIME's ImeSetActiveContext does nothing.
 *
 * @implemented
 * @see CtfImeSetActiveContextAlways
 */
EXTERN_C BOOL WINAPI
ImeSetActiveContext(
    _In_ HIMC hIMC,
    _In_ BOOL fFlag)
{
    TRACE("(%p, %u)\n", hIMC, fFlag);
    return FALSE;
}

EXTERN_C UINT WINAPI
ImeToAsciiEx(
    _In_ UINT uVirKey,
    _In_ UINT uScanCode,
    _In_ CONST LPBYTE lpbKeyState,
    _Out_ LPTRANSMSGLIST lpTransMsgList,
    _In_ UINT fuState,
    _In_ HIMC hIMC)
{
    FIXME("stub:(%u, %u, %p, %p, %u, %p)\n", uVirKey, uScanCode, lpbKeyState, lpTransMsgList,
          fuState, hIMC);
    return 0;
}

EXTERN_C BOOL WINAPI
NotifyIME(
    _In_ HIMC hIMC,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _In_ DWORD_PTR dwValue)
{
    FIXME("stub:(%p, 0x%lX, 0x%lX, %p)\n", hIMC, dwAction, dwIndex, dwValue);
    return FALSE;
}

EXTERN_C BOOL WINAPI
ImeSetCompositionString(
    _In_ HIMC hIMC,
    _In_ DWORD dwIndex,
    _In_opt_ LPCVOID lpComp,
    _In_ DWORD dwCompLen,
    _In_opt_ LPCVOID lpRead,
    _In_ DWORD dwReadLen)
{
    FIXME("stub:(%p, 0x%lX, %p, 0x%lX, %p, 0x%lX)\n", hIMC, dwIndex, lpComp, dwCompLen,
          lpRead, dwReadLen);
    return FALSE;
}

EXTERN_C DWORD WINAPI
ImeGetImeMenuItems(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ LPIMEMENUITEMINFOW lpImeParentMenu,
    _Inout_opt_ LPIMEMENUITEMINFOW lpImeMenu,
    _In_ DWORD dwSize)
{
    FIXME("stub:(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n", hIMC, dwFlags, dwType, lpImeParentMenu,
          lpImeMenu, dwSize);
    return 0;
}

/***********************************************************************
 *      CtfImeInquireExW (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeInquireExW(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags,
    _In_ HKL hKL)
{
    TRACE("(%p, %p, 0x%lX, %p)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags, hKL);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;

    if (!IsInteractiveUserLogon())
    {
        dwSystemInfoFlags |= IME_SYSINFO_WINLOGON;
        g_bWinLogon = TRUE;
    }

    pTLS->m_dwSystemInfoFlags = dwSystemInfoFlags;

    return Inquire(lpIMEInfo, lpszWndClass, dwSystemInfoFlags, hKL);
}

EXTERN_C BOOL WINAPI
CtfImeSelectEx(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %d, %p)\n", hIMC, fSelect, hKL);
    return FALSE;
}

EXTERN_C LRESULT WINAPI
CtfImeEscapeEx(
    _In_ HIMC hIMC,
    _In_ UINT uSubFunc,
    _Inout_opt_ LPVOID lpData,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hIMC, uSubFunc, lpData, hKL);
    return 0;
}

/***********************************************************************
 *      CtfImeGetGuidAtom (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeGetGuidAtom(
    _In_ HIMC hIMC,
    _In_ DWORD dwUnknown,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    TRACE("(%p, 0x%lX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);

    IMCLock imcLock(hIMC);

    HRESULT hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    IMCCLock<CTFIMECONTEXT> imccLock(imcLock.get().hCtfImeContext);
    hr = imccLock.m_hr;
    if (!imccLock)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    if (!imccLock.get().m_pCicIC)
        return E_OUTOFMEMORY;

    hr = imccLock.get().m_pCicIC->GetGuidAtom(imcLock, dwUnknown, pdwGuidAtom);
    return hr;
}

/***********************************************************************
 *      CtfImeIsGuidMapEnable (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
CtfImeIsGuidMapEnable(
    _In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);

    BOOL ret = FALSE;
    HRESULT hr;
    IMCLock imcLock(hIMC);

    hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
    if (SUCCEEDED(hr))
        ret = !!(imcLock.get().fdwInit & INIT_GUIDMAP);

    return ret;
}

/***********************************************************************
 *      CtfImeCreateThreadMgr (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeCreateThreadMgr(VOID)
{
    TRACE("()\n");

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;

    if (!pTLS->m_pBridge)
    {
        pTLS->m_pBridge = new CicBridge();
        if (!pTLS->m_pBridge)
            return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    if (!g_bWinLogon && !(pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON))
    {
        hr = pTLS->m_pBridge->InitIMMX(pTLS);
        if (SUCCEEDED(hr))
        {
            if (!pTLS->m_pThreadMgr)
                return E_OUTOFMEMORY;

            hr = pTLS->m_pBridge->ActivateIMMX(pTLS, pTLS->m_pThreadMgr);
            if (FAILED(hr))
                pTLS->m_pBridge->UnInitIMMX(pTLS);
        }
    }

    return hr;
}


/***********************************************************************
 *      CtfImeDestroyThreadMgr (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeDestroyThreadMgr(VOID)
{
    TRACE("()\n");

    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;

    if (pTLS->m_pBridge)
    {
        pTLS->m_pBridge = new CicBridge();
        if (!pTLS->m_pBridge)
            return E_OUTOFMEMORY;
    }

    if (!pTLS->m_pThreadMgr)
        return E_OUTOFMEMORY;

    if (pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON)
        return S_OK;

    HRESULT hr = pTLS->m_pBridge->DeactivateIMMX(pTLS, pTLS->m_pThreadMgr);
    if (hr == S_OK)
        pTLS->m_pBridge->UnInitIMMX(pTLS);

    return hr;
}

EXTERN_C HRESULT WINAPI
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *      CtfImeDestroyInputContext (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeDestroyInputContext(
    _In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);

    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS || !pTLS->m_pBridge)
        return E_OUTOFMEMORY;

    return pTLS->m_pBridge->DestroyInputContext(pTLS, hIMC);
}

EXTERN_C HRESULT WINAPI
CtfImeSetActiveContextAlways(
    _In_ HIMC hIMC,
    _In_ BOOL fActive,
    _In_ HWND hWnd,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %d, %p, %p)\n", hIMC, fActive, hWnd, hKL);
    return E_NOTIMPL;
}

EXTERN_C HRESULT WINAPI
CtfImeProcessCicHotkey(
    _In_ HIMC hIMC,
    _In_ UINT vKey,
    _In_ LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p)\n", hIMC, vKey, lParam);
    return E_NOTIMPL;
}

/***********************************************************************
 *      CtfImeDispatchDefImeMessage (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C LRESULT WINAPI
CtfImeDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    TRACE("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);

    TLS *pTLS = TLS::GetTLS();
    if (pTLS)
    {
        if (uMsg == WM_CREATE)
            ++pTLS->m_cWnds;
        else if (uMsg == WM_DESTROY)
            --pTLS->m_cWnds;
    }

    if (!IsMsImeMessage(uMsg))
        return 0;

    HKL hKL = GetKeyboardLayout(0);
    if (IS_IME_HKL(hKL))
        return 0;

    HWND hImeWnd = (HWND)SendMessageW(hWnd, WM_IME_NOTIFY, 0x17, 0);
    if (!IsWindow(hImeWnd))
        return 0;

    return SendMessageW(hImeWnd, uMsg, wParam, lParam);
}

EXTERN_C BOOL WINAPI
CtfImeIsIME(
    _In_ HKL hKL)
{
    FIXME("stub:(%p)\n", hKL);
    return FALSE;
}

/**
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeThreadDetach(VOID)
{
    ImeDestroy(0);
    return S_OK;
}

/**
 * @unimplemented
 */
EXTERN_C LRESULT CALLBACK
UIWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        FIXME("stub\n");
        return -1;
    }
    return 0;
}

/**
 * @unimplemented
 */
BOOL RegisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = sizeof(DWORD) * 2;
        wcx.hIcon           = LoadIconW(0, (LPCWSTR)IDC_ARROW);
        wcx.hInstance       = g_hInst;
        wcx.hCursor         = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
        wcx.hbrBackground   = (HBRUSH)GetStockObject(NULL_BRUSH);
        wcx.style           = CS_IME | CS_GLOBALCLASS;
        wcx.lpfnWndProc     = UIWndProc;
        wcx.lpszClassName   = L"MSCTFIME UI";
        if (!RegisterClassExW(&wcx))
            return FALSE;
    }

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME Composition", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = sizeof(DWORD);
        wcx.hIcon           = NULL;
        wcx.hInstance       = g_hInst;
        wcx.hCursor         = LoadCursorW(NULL, (LPCWSTR)IDC_IBEAM);
        wcx.hbrBackground   = (HBRUSH)GetStockObject(NULL_BRUSH);
        wcx.style           = CS_IME | CS_HREDRAW | CS_VREDRAW;
        //wcx.lpfnWndProc     = UIComposition::CompWndProc; // FIXME
        wcx.lpszClassName   = L"MSCTFIME Composition";
        if (!RegisterClassExW(&wcx))
            return FALSE;
    }

    return TRUE;
}

/**
 * @implemented
 */
VOID UnregisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx);
    UnregisterClassW(L"MSCTFIME UI", g_hInst);
    DestroyIcon(wcx.hIcon);
    DestroyIcon(wcx.hIconSm);

    GetClassInfoExW(g_hInst, L"MSCTFIME Composition", &wcx);
    UnregisterClassW(L"MSCTFIME Composition", g_hInst);
    DestroyIcon(wcx.hIcon);
    DestroyIcon(wcx.hIconSm);
}

/**
 * @implemented
 */
BOOL AttachIME(VOID)
{
    return RegisterImeClass() && RegisterMSIMEMessage();
}

/**
 * @implemented
 */
VOID DetachIME(VOID)
{
    UnregisterImeClass();
}

/**
 * @unimplemented
 */
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    g_hInst = hinstDLL;

    InitializeCriticalSectionAndSpinCount(&g_csLock, 0);

    if (!TLS::Initialize())
        return FALSE;

    g_dwOSInfo = cicGetOSInfo();

    // FIXME

    gfTFInitLib = TRUE;
    return AttachIME();
}

/**
 * @unimplemented
 */
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    // FIXME

    if (gfTFInitLib)
        DetachIME();

    DeleteCriticalSection(&g_csLock);
    TLS::InternalDestroyTLS();
    TLS::Uninitialize();

    // FIXME
}

/**
 * @implemented
 */
EXTERN_C BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD dwReason,
    _Inout_opt_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            TRACE("(%p, %lu, %p)\n", hinstDLL, dwReason, lpvReserved);
            if (!ProcessAttach(hinstDLL))
            {
                ProcessDetach(hinstDLL);
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            ProcessDetach(hinstDLL);
            break;
        }
        case DLL_THREAD_DETACH:
        {
            TF_DllDetachInOther();
            CtfImeThreadDetach();
            TLS::InternalDestroyTLS();
            break;
        }
    }
    return TRUE;
}
