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

typedef BOOLEAN (WINAPI *FN_DllShutDownInProgress)(VOID);

EXTERN_C BOOLEAN WINAPI
DllShutDownInProgress(VOID)
{
    HMODULE hNTDLL;
    static FN_DllShutDownInProgress s_fnDllShutDownInProgress = NULL;

    if (s_fnDllShutDownInProgress)
        return s_fnDllShutDownInProgress();

    hNTDLL = GetSystemModuleHandle(L"ntdll.dll", FALSE);
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

/* FIXME */
class CicInputContext : public ITfContextOwnerCompositionSink
{
    LONG m_cRefs;
public:
    CicInputContext()
    {
        m_cRefs = 1;
    }
    virtual ~CicInputContext()
    {
    }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfContextOwnerCompositionSink interface
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk) override;
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew) override;
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition) override;

    HRESULT
    GetGuidAtom(
        _Inout_ IMCLock& imcLock,
        _In_ DWORD dwUnknown,
        _Out_opt_ LPDWORD pdwGuidAtom);

    HRESULT DestroyInputContext();
};

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
 * @unimplemented
 */
HRESULT
CicInputContext::GetGuidAtom(
    _Inout_ IMCLock& imcLock,
    _In_ DWORD dwUnknown,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    IMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCompStr);

    HRESULT hr = imeContext.m_hr;
    if (!imeContext)
        hr = E_FAIL;

    if (FAILED(hr))
        return hr;

    // FIXME
    return hr;
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

/* FIXME */
class CicBridge : public ITfSysHookSink
{
protected:
    LONG m_cRefs;
    DWORD m_dwImmxInit;
    DWORD m_dwUnknown[10];

public:
    CicBridge();
    virtual ~CicBridge();

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP OnPreFocusDIM(HWND hwnd) override;
    STDMETHODIMP OnSysKeyboardProc(UINT, LONG) override;
    STDMETHODIMP OnSysShellProc(INT, UINT, LONG) override;

    HRESULT InitIMMX(TLS *pTLS);
    BOOL UnInitIMMX(TLS *pTLS);
    HRESULT ActivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr);
    HRESULT DeactivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr);
    HRESULT DestroyInputContext(TLS *pTLS, HIMC hIMC);

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
    CActiveLanguageProfileNotifySink *m_pActiveLanguageProfileNotifySync;
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
    m_pActiveLanguageProfileNotifySync = NULL;
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

    CActiveLanguageProfileNotifySink *pSink = m_pActiveLanguageProfileNotifySync;
    if (pSink)
    {
        pSink->_Unadvise();
        pSink->Release();
        m_pActiveLanguageProfileNotifySync = NULL;
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
    DWORD m_dwUnknown2[4];
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

HRESULT
CicProfile::InitProfileInstance(TLS *pTLS)
{
    HRESULT hr = TF_CreateInputProcessorProfiles(&m_pIPProfiles);
    if (FAILED(hr))
        return hr;

    if (!m_pActiveLanguageProfileNotifySync)
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
        m_pActiveLanguageProfileNotifySync = pSink;
    }

    if (pTLS->m_pThreadMgr)
        m_pActiveLanguageProfileNotifySync->_Advise(pTLS->m_pThreadMgr);

    return hr;
}

/***********************************************************************
 *      CicBridge
 */

CicBridge::CicBridge()
{
    m_dwImmxInit &= ~1;
    m_dwUnknown[0] &= ~1;
    m_dwUnknown[1] &= ~1;
    m_dwUnknown[9] &= ~1;
    m_dwUnknown[3] = 0;
    m_dwUnknown[4] = 0;
    m_dwUnknown[5] = 0;
    m_dwUnknown[6] = 0;
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

/**
 * @unimplemented
 */
HRESULT CicBridge::InitIMMX(TLS *pTLS)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
HRESULT CicBridge::ActivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
HRESULT CicBridge::DeactivateIMMX(TLS *pTLS, ITfThreadMgr *pThreadMgr)
{
    return E_NOTIMPL;
}

/**
 * @unimplemented
 */
BOOL CicBridge::UnInitIMMX(TLS *pTLS)
{
    //FIXME

    if (pTLS->m_pProfile)
    {
        pTLS->m_pProfile->Release();
        pTLS->m_pProfile = NULL;
    }

    //FIXME

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

    g_dwOSInfo = GetOSInfo();

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
 * @unimplemented
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
            // FIXME
            CtfImeThreadDetach();
            TLS::InternalDestroyTLS();
            break;
        }
    }
    return TRUE;
}
