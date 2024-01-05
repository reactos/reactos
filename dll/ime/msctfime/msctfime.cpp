/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Supporting IME interface of Text Input Processors (TIPs)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"
#include <ndk/ldrfuncs.h> /* for RtlDllShutdownInProgress */

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

HINSTANCE g_hInst = NULL; /* The instance of this module */
BOOL g_bWinLogon = FALSE;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
BOOL gfTFInitLib = FALSE;
CRITICAL_SECTION g_csLock;

DEFINE_GUID(GUID_COMPARTMENT_CTFIME_DIMFLAGS,        0xA94C5FD2, 0xC471, 0x4031, 0x95, 0x46, 0x70, 0x9C, 0x17, 0x30, 0x0C, 0xB9);
DEFINE_GUID(GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT, 0x85A688F7, 0x6DC8, 0x4F17, 0xA8, 0x3A, 0xB1, 0x1C, 0x09, 0xCD, 0xD7, 0xBF);

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

/// @implemented
BOOL IsMsImeMessage(_In_ UINT uMsg)
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

/// @implemented
BOOL RegisterMSIMEMessage(VOID)
{
    // Using ANSI (A) version here can reduce binary size.
    WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    WM_MSIME_UIREADY = RegisterWindowMessageA("MSIMEUIReady");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    WM_MSIME_MODEBIAS = RegisterWindowMessageA("MSIMEModeBias");
    WM_MSIME_SHOWIMEPAD = RegisterWindowMessageA("MSIMEShowImePad");
    WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    WM_MSIME_KEYMAP = RegisterWindowMessageA("MSIMEKeyMap");
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

/// This function calls ntdll!RtlDllShutdownInProgress.
/// It can detect the system is shutting down or not.
/// @implemented
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

/// This function checks if the current user logon session is interactive.
/// @implemented
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

HRESULT InitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread)
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

HIMC GetActiveContext(VOID)
{
    HWND hwndFocus = ::GetFocus();
    if (!hwndFocus)
        hwndFocus = ::GetActiveWindow();
    return ::ImmGetContext(hwndFocus);
}

/// @implemented
HRESULT UninitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread)
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

/// Gets the charset from a language ID.
/// @implemented
BYTE GetCharsetFromLangId(_In_ DWORD dwValue)
{
    CHARSETINFO info;
    if (!::TranslateCharsetInfo((DWORD*)(DWORD_PTR)dwValue, &info, TCI_SRCLOCALE))
        return 0;
    return info.ciCharset;
}

/// Selects or unselects the input context.
/// @implemented
HRESULT
InternalSelectEx(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ LANGID LangID)
{
    CicIMCLock imcLock(hIMC);
    if (!imcLock)
        imcLock.m_hr = E_FAIL;
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    if (PRIMARYLANGID(LangID) == LANG_CHINESE)
    {
        imcLock.get().cfCandForm[0].dwStyle = 0;
        imcLock.get().cfCandForm[0].dwIndex = (DWORD)-1;
    }

    if (!fSelect)
    {
        imcLock.get().fdwInit &= ~INIT_GUIDMAP;
        return imcLock.m_hr;
    }

    if (!imcLock.ClearCand())
        return imcLock.m_hr;

    // Populate conversion mode
    if (!(imcLock.get().fdwInit & INIT_CONVERSION))
    {
        DWORD dwConv = (imcLock.get().fdwConversion & IME_CMODE_SOFTKBD);
        if (LangID)
        {
            if (PRIMARYLANGID(LangID) == LANG_JAPANESE)
            {
                dwConv |= IME_CMODE_ROMAN | IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
            }
            else if (PRIMARYLANGID(LangID) != LANG_KOREAN)
            {
                dwConv |= IME_CMODE_NATIVE;
            }
        }
        imcLock.get().fdwConversion |= dwConv;
        imcLock.get().fdwInit |= INIT_CONVERSION;
    }

    // Populate sentence mode
    imcLock.get().fdwSentence |= IME_SMODE_PHRASEPREDICT;

    // Populate LOGFONT
    if (!(imcLock.get().fdwInit & INIT_LOGFONT))
    {
        // Get logical font
        LOGFONTW lf;
        HDC hDC = ::GetDC(imcLock.get().hWnd);
        HGDIOBJ hFont = GetCurrentObject(hDC, OBJ_FONT);
        ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
        ::ReleaseDC(imcLock.get().hWnd, hDC);

        imcLock.get().lfFont.W = lf;
        imcLock.get().fdwInit |= INIT_LOGFONT;
    }
    imcLock.get().lfFont.W.lfCharSet = GetCharsetFromLangId(LangID);

    imcLock.InitContext();

    return imcLock.m_hr;
}

/***********************************************************************
 *      Compartment
 */

/// @implemented
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

/// @implemented
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

/// @implemented
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

/// @implemented
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

/// @implemented
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
    CicArray<CESMAP> m_array;
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
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCompartmentEventSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
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

class CInputContextOwnerCallBack;
class CInputContextOwner;

typedef INT (CALLBACK *FN_ENDEDIT)(INT, LPVOID, LPVOID);
typedef INT (CALLBACK *FN_LAYOUTCHANGE)(UINT nType, FN_ENDEDIT fnEndEdit, ITfContextView *pView);

class CTextEventSink : public ITfTextEditSink, ITfTextLayoutSink
{
protected:
    LONG m_cRefs;
    IUnknown *m_pUnknown;
    DWORD m_dwEditSinkCookie;
    DWORD m_dwLayoutSinkCookie;
    union
    {
        UINT m_uFlags;
        FN_LAYOUTCHANGE m_fnLayoutChange;
    };
    FN_ENDEDIT m_fnEndEdit;
    LPVOID m_pCallbackPV;

public:
    CTextEventSink(FN_ENDEDIT fnEndEdit, LPVOID pCallbackPV);
    virtual ~CTextEventSink();

    HRESULT _Advise(IUnknown *pUnknown, UINT uFlags);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextEditSink interface
    STDMETHODIMP OnEndEdit(
        ITfContext *pic,
        TfEditCookie ecReadOnly,
        ITfEditRecord *pEditRecord) override;

    // ITfTextLayoutSink interface
    STDMETHODIMP
    OnLayoutChange(
        ITfContext *pContext,
        TfLayoutCode lcode,
        ITfContextView *pContextView) override;
};

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
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    if (IsEqualIID(riid, IID_ITfTextLayoutSink))
    {
        *ppvObj = static_cast<ITfTextLayoutSink*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
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

/***********************************************************************
 *      CicInputContext
 *
 * The msctfime.ime's input context.
 */
class CicInputContext
    : public ITfCleanupContextSink
    , public ITfContextOwnerCompositionSink
    , public ITfCompositionSink
{
public:
    LONG m_cRefs;
    HIMC m_hIMC;
    ITfDocumentMgr *m_pDocumentMgr;
    ITfContext *m_pContext;
    ITfContextOwnerServices *m_pContextOwnerServices;
    CInputContextOwnerCallBack *m_pICOwnerCallback;
    CTextEventSink *m_pTextEventSink;
    CCompartmentEventSink *m_pCompEventSink1;
    CCompartmentEventSink *m_pCompEventSink2;
    CInputContextOwner *m_pInputContextOwner;
    DWORD m_dwUnknown3[3];
    DWORD m_dwUnknown4[2];
    DWORD m_dwQueryPos;
    DWORD m_dwUnknown5;
    GUID m_guid;
    DWORD m_dwUnknown6[11];
    BOOL m_bSelecting;
    DWORD m_dwUnknown6_5;
    LONG m_cCompLocks;
    DWORD m_dwUnknown7[5];
    WORD m_cGuidAtoms;
    WORD m_padding;
    DWORD m_adwGuidAtoms[256];
    DWORD m_dwUnknown8[17];
    TfClientId m_clientId;
    DWORD m_dwUnknown9;

public:
    CicInputContext(
        _In_ TfClientId cliendId,
        _Inout_ PCIC_LIBTHREAD pLibThread,
        _In_ HIMC hIMC);
    virtual ~CicInputContext() { }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfCleanupContextSink interface
    STDMETHODIMP OnCleanupContext(_In_ TfEditCookie ecWrite, _Inout_ ITfContext *pic) override;

    // ITfContextOwnerCompositionSink interface
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk) override;
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew) override;
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition) override;

    // ITfCompositionSink interface
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition) override;

    HRESULT
    GetGuidAtom(
        _Inout_ CicIMCLock& imcLock,
        _In_ BYTE iAtom,
        _Out_opt_ LPDWORD pdwGuidAtom);

    HRESULT CreateInputContext(_Inout_ ITfThreadMgr *pThreadMgr, _Inout_ CicIMCLock& imcLock);
    HRESULT DestroyInputContext();
};

/// @unimplemented
CicInputContext::CicInputContext(
    _In_ TfClientId cliendId,
    _Inout_ PCIC_LIBTHREAD pLibThread,
    _In_ HIMC hIMC)
{
    m_hIMC = hIMC;
    m_guid = GUID_NULL;
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
    if ((m_cCompLocks <= 0) || m_dwUnknown6_5)
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

/// Retrieves the IME information.
/// @implemented
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
        _In_ FN_INITDOCMGR fnInit,
        _In_ FN_PUSHPOP fnPushPop = NULL,
        _Inout_ LPVOID pvCallbackPV = NULL);
    virtual ~CThreadMgrEventSink() { }

    void SetCallbackPV(_Inout_ LPVOID pv);
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
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
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

class CFunctionProviderBase : public ITfFunctionProvider
{
protected:
    TfClientId m_clientId;
    GUID m_guid;
    BSTR m_bstr;
    LONG m_cRefs;

public:
    CFunctionProviderBase(_In_ TfClientId clientId);
    virtual ~CFunctionProviderBase();

    // IUnknown interface
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfFunctionProvider interface
    STDMETHODIMP GetType(_Out_ GUID *guid) override;
    STDMETHODIMP GetDescription(_Out_ BSTR *desc) override;
    //STDMETHODIMP GetFunction(_In_ REFGUID guid, _In_ REFIID riid, _Out_ IUnknown **func) = 0;

    BOOL Init(_In_ REFGUID rguid, _In_ LPCWSTR psz);
};

/// @implemented
CFunctionProviderBase::CFunctionProviderBase(_In_ TfClientId clientId)
{
    m_clientId = clientId;
    m_guid = GUID_NULL;
    m_bstr = NULL;
    m_cRefs = 1;
}

/// @implemented
CFunctionProviderBase::~CFunctionProviderBase()
{
    if (!RtlDllShutdownInProgress())
        ::SysFreeString(m_bstr);
}

/// @implemented
BOOL
CFunctionProviderBase::Init(
    _In_ REFGUID rguid,
    _In_ LPCWSTR psz)
{
    m_bstr = ::SysAllocString(psz);
    m_guid = rguid;
    return (m_bstr != NULL);
}

class CFnDocFeed : public IAImmFnDocFeed
{
    LONG m_cRefs;

public:
    CFnDocFeed();
    virtual ~CFnDocFeed();

    // IUnknown interface
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IAImmFnDocFeed interface
    STDMETHODIMP DocFeed() override;
    STDMETHODIMP ClearDocFeedBuffer() override;
    STDMETHODIMP StartReconvert() override;
    STDMETHODIMP StartUndoCompositionString() override;
};

CFnDocFeed::CFnDocFeed()
{
    m_cRefs = 1;
}

CFnDocFeed::~CFnDocFeed()
{
}

/// @implemented
STDMETHODIMP CFnDocFeed::QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CFnDocFeed::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CFnDocFeed::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::DocFeed()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::ClearDocFeedBuffer()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::StartReconvert()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::StartUndoCompositionString()
{
    return E_NOTIMPL;
}

/// @implemented
STDMETHODIMP
CFunctionProviderBase::QueryInterface(
    _In_ REFIID riid,
    _Out_ LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CFunctionProviderBase::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CFunctionProviderBase::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
STDMETHODIMP CFunctionProviderBase::GetType(_Out_ GUID *guid)
{
    *guid = m_guid;
    return S_OK;
}

/// @implemented
STDMETHODIMP CFunctionProviderBase::GetDescription(_Out_ BSTR *desc)
{
    *desc = ::SysAllocString(m_bstr);
    return (*desc ? S_OK : E_OUTOFMEMORY);
}

class CFunctionProvider : public CFunctionProviderBase
{
public:
    CFunctionProvider(_In_ TfClientId clientId);

    STDMETHODIMP GetFunction(_In_ REFGUID guid, _In_ REFIID riid, _Out_ IUnknown **func) override;
};

/// @implemented
CFunctionProvider::CFunctionProvider(_In_ TfClientId clientId) : CFunctionProviderBase(clientId)
{
    Init(CLSID_CAImmLayer, L"MSCTFIME::Function Provider");
}

/// @implemented
STDMETHODIMP
CFunctionProvider::GetFunction(
    _In_ REFGUID guid,
    _In_ REFIID riid,
    _Out_ IUnknown **func)
{
    *func = NULL;

    if (IsEqualGUID(guid, GUID_NULL) &&
        IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *func = new(cicNoThrow) CFnDocFeed();
        if (*func)
            return S_OK;
    }

    return E_NOINTERFACE;
}

/* FIXME */
class CicBridge : public ITfSysHookSink
{
protected:
    LONG m_cRefs;
    BOOL m_bImmxInited;
    BOOL m_bUnknown1;
    BOOL m_bDeactivating;
    DWORD m_cActivateLocks;
    ITfKeystrokeMgr *m_pKeystrokeMgr;
    ITfDocumentMgr *m_pDocMgr;
    CThreadMgrEventSink *m_pThreadMgrEventSink;
    TfClientId m_cliendId;
    CIC_LIBTHREAD m_LibThread;
    BOOL m_bUnknown2;

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

    HRESULT InitIMMX(_Inout_ TLS *pTLS);
    BOOL UnInitIMMX(_Inout_ TLS *pTLS);
    HRESULT ActivateIMMX(_Inout_ TLS *pTLS, _Inout_ ITfThreadMgr_P *pThreadMgr);
    HRESULT DeactivateIMMX(_Inout_ TLS *pTLS, _Inout_ ITfThreadMgr_P *pThreadMgr);

    HRESULT CreateInputContext(TLS *pTLS, HIMC hIMC);
    HRESULT DestroyInputContext(TLS *pTLS, HIMC hIMC);
    ITfContext *GetInputContext(CicIMCCLock<CTFIMECONTEXT>& imeContext);

    HRESULT SelectEx(
        _Inout_ TLS *pTLS,
        _Inout_ ITfThreadMgr_P *pThreadMgr,
        _In_ HIMC hIMC,
        _In_ BOOL fSelect,
        _In_ HKL hKL);
    HRESULT OnSetOpenStatus(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        CicIMCLock& imcLock,
        CicInputContext *pCicIC);

    void PostTransMsg(_In_ HWND hWnd, _In_ INT cTransMsgs, _In_ const TRANSMSG *pTransMsgs);
    void GetDocumentManager(_Inout_ CicIMCCLock<CTFIMECONTEXT>& imeContext);

    HRESULT
    ConfigureGeneral(_Inout_ TLS* pTLS,
        _In_ ITfThreadMgr *pThreadMgr,
        _In_ HKL hKL,
        _In_ HWND hWnd);
    HRESULT ConfigureRegisterWord(
        _Inout_ TLS* pTLS,
        _In_ ITfThreadMgr *pThreadMgr,
        _In_ HKL hKL,
        _In_ HWND hWnd,
        _Inout_opt_ LPVOID lpData);
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
    CActiveLanguageProfileNotifySink(_In_ FN_COMPARE fnCompare, _Inout_opt_ void *pUserData);
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
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
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
    DWORD   m_dwUnknown1;
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

    HRESULT
    GetActiveLanguageProfile(
        _In_ HKL hKL,
        _In_ REFGUID rguid,
        _Out_ TF_LANGUAGEPROFILE *pProfile);
    HRESULT GetLangId(_Out_ LANGID *pLangID);
    HRESULT GetCodePageA(_Out_ UINT *puCodePage);

    HRESULT InitProfileInstance(_Inout_ TLS *pTLS);
};

/// @implemented
CicProfile::CicProfile()
{
    m_dwFlags &= 0xFFFFFFF0;
    m_cRefs = 1;
    m_pIPProfiles = NULL;
    m_pActiveLanguageProfileNotifySink = NULL;
    m_LangID1 = 0;
    m_nCodePage = CP_ACP;
    m_LangID2 = 0;
    m_dwUnknown1 = 0;
}

/// @implemented
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

/// @implemented
STDMETHODIMP CicProfile::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CicProfile::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CicProfile::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
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

/// @implemented
HRESULT CicProfile::GetCodePageA(_Out_ UINT *puCodePage)
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

/// @implemented
HRESULT CicProfile::GetLangId(_Out_ LANGID *pLangID)
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
    ITfThreadMgr_P *m_pThreadMgr;
    DWORD m_dwFlags1;
    DWORD m_dwFlags2;
    DWORD m_dwUnknown2;
    BOOL m_bDestroyed;
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

/// @implemented
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

    pTLS->m_dwFlags1 |= 1;
    pTLS->m_dwUnknown2 |= 1;
    return pTLS;
}

/// @implemented
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

/// @implemented
HRESULT
CicProfile::InitProfileInstance(_Inout_ TLS *pTLS)
{
    HRESULT hr = TF_CreateInputProcessorProfiles(&m_pIPProfiles);
    if (FAILED(hr))
        return hr;

    if (!m_pActiveLanguageProfileNotifySink)
    {
        CActiveLanguageProfileNotifySink *pSink =
            new(cicNoThrow) CActiveLanguageProfileNotifySink(
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

/***********************************************************************
 *      CicBridge
 */

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
    *ppvObj = NULL;

    if (!IsEqualIID(riid, IID_ITfSysHookSink))
        return E_NOINTERFACE;

    *ppvObj = this;
    AddRef();

    return S_OK;
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

void CicBridge::GetDocumentManager(_Inout_ CicIMCCLock<CTFIMECONTEXT>& imeContext)
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

/// @unimplemented
HRESULT
CicBridge::CreateInputContext(
    _Inout_ TLS *pTLS,
    _In_ HIMC hIMC)
{
    CicIMCLock imcLock(hIMC);
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

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
    {
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

/// @implemented
HRESULT CicBridge::DestroyInputContext(TLS *pTLS, HIMC hIMC)
{
    CicIMCLock imcLock(hIMC);
    HRESULT hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
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

ITfContext *
CicBridge::GetInputContext(CicIMCCLock<CTFIMECONTEXT>& imeContext)
{
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return NULL;
    return pCicIC->m_pContext;
}

/// @unimplemented
HRESULT CicBridge::OnSetOpenStatus(
    TLS *pTLS,
    ITfThreadMgr_P *pThreadMgr,
    CicIMCLock& imcLock,
    CicInputContext *pCicIC)
{
    return E_NOTIMPL;
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
    if (!imeContext)
        imeContext.m_hr = E_FAIL;
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (pCicIC)
        pCicIC->m_bSelecting = TRUE;

    if (fSelect)
    {
        if (pCicIC)
            pCicIC->m_dwUnknown6[1] &= ~1;
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

/***********************************************************************
 *      CicProfile
 */

/// @unimplemented
HRESULT
CicProfile::GetActiveLanguageProfile(
    _In_ HKL hKL,
    _In_ REFGUID rguid,
    _Out_ TF_LANGUAGEPROFILE *pProfile)
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

/***********************************************************************
 *      CtfImeSelectEx (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
CtfImeSelectEx(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ HKL hKL)
{
    TRACE("(%p, %d, %p)\n", hIMC, fSelect, hKL);

    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;

    InternalSelectEx(hIMC, fSelect, LOWORD(hKL));

    if (!pTLS->m_pBridge || !pTLS->m_pThreadMgr)
        return E_OUTOFMEMORY;

    return pTLS->m_pBridge->SelectEx(pTLS, pTLS->m_pThreadMgr, hIMC, fSelect, hKL);
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

    CicIMCLock imcLock(hIMC);

    HRESULT hr = imcLock.m_hr;
    if (!imcLock)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    CicIMCCLock<CTFIMECONTEXT> imccLock(imcLock.get().hCtfImeContext);
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
    CicIMCLock imcLock(hIMC);

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
        pTLS->m_pBridge = new(cicNoThrow) CicBridge();
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
        pTLS->m_pBridge = new(cicNoThrow) CicBridge();
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


/***********************************************************************
 *      CtfImeProcessCicHotkey (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeProcessCicHotkey(
    _In_ HIMC hIMC,
    _In_ UINT vKey,
    _In_ LPARAM lParam)
{
    TRACE("(%p, %u, %p)\n", hIMC, vKey, lParam);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return S_OK;

    HRESULT hr = S_OK;
    ITfThreadMgr *pThreadMgr = NULL;
    ITfThreadMgr_P *pThreadMgr_P = NULL;
    if ((TF_GetThreadMgr(&pThreadMgr) == S_OK) &&
        (pThreadMgr->QueryInterface(IID_ITfThreadMgr_P, (void**)&pThreadMgr_P) == S_OK) &&
        CtfImmIsCiceroStartedInThread())
    {
        HRESULT hr2;
        if (SUCCEEDED(pThreadMgr_P->CallImm32HotkeyHandler(vKey, lParam, &hr2)))
            hr = hr2;
    }

    if (pThreadMgr)
        pThreadMgr->Release();
    if (pThreadMgr_P)
        pThreadMgr_P->Release();

    return hr;
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

/***********************************************************************
 *      CtfImeThreadDetach (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeThreadDetach(VOID)
{
    ImeDestroy(0);
    return S_OK;
}

/***********************************************************************
 *      UIComposition
 */
struct UIComposition
{
    void OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd);
    void OnImeCompositionUpdate(CicIMCLock& imcLock);
    void OnImeEndComposition();
    void OnImeSetContext(CicIMCLock& imcLock, HWND hUIWnd, WPARAM wParam, LPARAM lParam);
    void OnPaintTheme(WPARAM wParam);
    void OnDestroy();

    static LRESULT CALLBACK CompWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/// @unimplemented
void UIComposition::OnImeStartComposition(CicIMCLock& imcLock, HWND hUIWnd)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnImeCompositionUpdate(CicIMCLock& imcLock)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnImeEndComposition()
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnImeSetContext(CicIMCLock& imcLock, HWND hUIWnd, WPARAM wParam, LPARAM lParam)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnPaintTheme(WPARAM wParam)
{
    //FIXME
}

/// @unimplemented
void UIComposition::OnDestroy()
{
    //FIXME
}

/// @unimplemented
LRESULT CALLBACK
UIComposition::CompWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CREATE)
        return -1; // FIXME
    return 0;
}

/***********************************************************************
 *      UI
 */
struct UI
{
    HWND m_hWnd;
    UIComposition *m_pComp;

    UI(HWND hWnd);
    virtual ~UI();

    HRESULT _Create();
    void _Destroy();

    static void OnCreate(HWND hWnd);
    static void OnDestroy(HWND hWnd);
    void OnImeSetContext(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam);
};

// For GetWindowLongPtr/SetWindowLongPtr
#define UIGWLP_HIMC 0
#define UIGWLP_UI   sizeof(HIMC)
#define UIGWLP_SIZE (UIGWLP_UI + sizeof(UI*))

/// @implemented
UI::UI(HWND hWnd) : m_hWnd(hWnd)
{
}

/// @implemented
UI::~UI()
{
    delete m_pComp;
}

/// @unimplemented
HRESULT UI::_Create()
{
    m_pComp = new(cicNoThrow) UIComposition();
    if (!m_pComp)
        return E_OUTOFMEMORY;

    SetWindowLongPtrW(m_hWnd, UIGWLP_UI, (LONG_PTR)this);
    //FIXME
    return S_OK;
}

/// @implemented
void UI::_Destroy()
{
    m_pComp->OnDestroy();
    SetWindowLongPtrW(m_hWnd, UIGWLP_UI, 0);
}

/// @implemented
void UI::OnCreate(HWND hWnd)
{
    UI *pUI = (UI*)GetWindowLongPtrW(hWnd, UIGWLP_UI);
    if (pUI)
        return;
    pUI = new(cicNoThrow) UI(hWnd);
    if (pUI)
        pUI->_Create();
}

/// @implemented
void UI::OnDestroy(HWND hWnd)
{
    UI *pUI = (UI*)GetWindowLongPtrW(hWnd, UIGWLP_UI);
    if (!pUI)
        return;

    pUI->_Destroy();
    delete pUI;
}

/// @implemented
void UI::OnImeSetContext(CicIMCLock& imcLock, WPARAM wParam, LPARAM lParam)
{
    m_pComp->OnImeSetContext(imcLock, m_hWnd, wParam, lParam);
}

/***********************************************************************
 *      CIMEUIWindowHandler
 */

struct CIMEUIWindowHandler
{
    static LRESULT CALLBACK ImeUIMsImeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeMouseHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeModeBiasHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeReconvertRequest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0; //FIXME
}

/// @implemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIMsImeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_MSIME_MOUSE)
        return ImeUIMsImeMouseHandler(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_MODEBIAS)
        return ImeUIMsImeModeBiasHandler(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_RECONVERTREQUEST)
        return ImeUIMsImeReconvertRequest(hWnd, wParam, lParam);
    if (uMsg == WM_MSIME_SERVICE)
    {
        TLS *pTLS = TLS::GetTLS();
        if (pTLS && pTLS->m_pProfile)
        {
            LANGID LangID;
            pTLS->m_pProfile->GetLangId(&LangID);
            if (PRIMARYLANGID(LangID) == LANG_KOREAN)
                return FALSE;
        }
        return TRUE;
    }
    return 0;
}

/// @unimplemented
LRESULT CALLBACK
CIMEUIWindowHandler::ImeUIWndProcWorker(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TLS *pTLS = TLS::GetTLS();
    if (pTLS && (pTLS->m_dwSystemInfoFlags & IME_SYSINFO_WINLOGON))
    {
        if (uMsg == WM_CREATE)
            return -1;
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
        case WM_CREATE:
        {
            UI::OnCreate(hWnd);
            break;
        }
        case WM_DESTROY:
        case WM_ENDSESSION:
        {
            UI::OnDestroy(hWnd);
            break;
        }
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_SETCONTEXT:
        case WM_IME_NOTIFY:
        case WM_IME_SELECT:
        case WM_TIMER:
        {
            HIMC hIMC = (HIMC)GetWindowLongPtrW(hWnd, UIGWLP_HIMC);
            UI* pUI = (UI*)GetWindowLongPtrW(hWnd, UIGWLP_UI);
            CicIMCLock imcLock(hIMC);
            switch (uMsg)
            {
                case WM_IME_STARTCOMPOSITION:
                {
                    pUI->m_pComp->OnImeStartComposition(imcLock, pUI->m_hWnd);
                    break;
                }
                case WM_IME_COMPOSITION:
                {
                    if (lParam & GCS_COMPSTR)
                    {
                        pUI->m_pComp->OnImeCompositionUpdate(imcLock);
                        ::SetTimer(hWnd, 0, 10, NULL);
                        //FIXME
                    }
                    break;
                }
                case WM_IME_ENDCOMPOSITION:
                {
                    ::KillTimer(hWnd, 0);
                    pUI->m_pComp->OnImeEndComposition();
                    break;
                }
                case WM_IME_SETCONTEXT:
                {
                    pUI->OnImeSetContext(imcLock, wParam, lParam);
                    ::KillTimer(hWnd, 1);
                    ::SetTimer(hWnd, 1, 300, NULL);
                    break;
                }
                case WM_TIMER:
                {
                    //FIXME
                    ::KillTimer(hWnd, wParam);
                    break;
                }
                case WM_IME_NOTIFY:
                case WM_IME_SELECT:
                default:
                {
                    pUI->m_pComp->OnPaintTheme(wParam);
                    break;
                }
            }
            break;
        }
        default:
        {
            if (IsMsImeMessage(uMsg))
                return CIMEUIWindowHandler::ImeUIMsImeHandler(hWnd, uMsg, wParam, lParam);
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

/// @implemented
EXTERN_C LRESULT CALLBACK
UIWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    return CIMEUIWindowHandler::ImeUIWndProcWorker(hWnd, uMsg, wParam, lParam);
}

/// @unimplemented
BOOL RegisterImeClass(VOID)
{
    WNDCLASSEXW wcx;

    if (!GetClassInfoExW(g_hInst, L"MSCTFIME UI", &wcx))
    {
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize          = sizeof(WNDCLASSEXW);
        wcx.cbWndExtra      = UIGWLP_SIZE;
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
        wcx.lpfnWndProc     = UIComposition::CompWndProc;
        wcx.lpszClassName   = L"MSCTFIME Composition";
        if (!RegisterClassExW(&wcx))
            return FALSE;
    }

    return TRUE;
}

/// @implemented
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

/// @implemented
BOOL AttachIME(VOID)
{
    return RegisterImeClass() && RegisterMSIMEMessage();
}

/// @implemented
VOID DetachIME(VOID)
{
    UnregisterImeClass();
}

/// @unimplemented
VOID InitUIFLib(VOID)
{
    //FIXME
}

/// @unimplemented
VOID DoneUIFLib(VOID)
{
    //FIXME
}

/// @implemented
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    g_hInst = hinstDLL;

    InitializeCriticalSectionAndSpinCount(&g_csLock, 0);

    if (!TLS::Initialize())
        return FALSE;

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);

    InitUIFLib();

    if (!TFInitLib())
        return FALSE;

    gfTFInitLib = TRUE;
    return AttachIME();
}

/// @unimplemented
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    // FIXME

    TF_DllDetachInOther();

    if (gfTFInitLib)
    {
        DetachIME();
        TFUninitLib();
    }

    DeleteCriticalSection(&g_csLock);
    TLS::InternalDestroyTLS();
    TLS::Uninitialize();
    DoneUIFLib();
}

/// @implemented
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
