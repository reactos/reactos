/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Miscellaneous of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/// East-Asian language?
/// @implemented
BOOL IsEALang(_In_opt_ LANGID LangID)
{
    if (LangID == 0)
    {
        TLS *pTLS = TLS::GetTLS();
        if (!pTLS || !pTLS->m_pProfile)
            return FALSE;

        pTLS->m_pProfile->GetLangId(&LangID);
    }

    switch (PRIMARYLANGID(LangID))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            return TRUE;

        default:
            return FALSE;
    }
}

typedef BOOLEAN (WINAPI *FN_DllShutdownInProgress)(VOID);

/// This function calls ntdll!RtlDllShutdownInProgress.
/// It can detect the system is shutting down or not.
/// @implemented
BOOLEAN DllShutdownInProgress(VOID)
{
    HMODULE hNTDLL;
    static FN_DllShutdownInProgress s_fnDllShutdownInProgress = NULL;

    if (s_fnDllShutdownInProgress)
        return s_fnDllShutdownInProgress();

    hNTDLL = cicGetSystemModuleHandle(L"ntdll.dll", FALSE);
    s_fnDllShutdownInProgress =
        (FN_DllShutdownInProgress)GetProcAddress(hNTDLL, "RtlDllShutdownInProgress");
    if (!s_fnDllShutdownInProgress)
        return FALSE;

    return s_fnDllShutdownInProgress();
}

/// This function checks if the current user logon session is interactive.
/// @implemented
BOOL IsInteractiveUserLogon(VOID)
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

/// Gets the charset from a language ID.
/// @implemented
BYTE GetCharsetFromLangId(_In_ DWORD dwValue)
{
    CHARSETINFO info;
    if (!::TranslateCharsetInfo((DWORD*)(DWORD_PTR)dwValue, &info, TCI_SRCLOCALE))
        return 0;
    return info.ciCharset;
}

/// Get the active input context.
/// @implemented
HIMC GetActiveContext(VOID)
{
    HWND hwndFocus = ::GetFocus();
    if (!hwndFocus)
        hwndFocus = ::GetActiveWindow();
    return ::ImmGetContext(hwndFocus);
}

// MSIMTF.dll!MsimtfIsGuidMapEnable
typedef BOOL (WINAPI *FN_MsimtfIsGuidMapEnable)(HIMC hIMC, LPBOOL pbValue);
HINSTANCE g_hMSIMTF = NULL;

/// @implemented
BOOL MsimtfIsGuidMapEnable(_In_ HIMC hIMC, _Out_opt_ LPBOOL pbValue)
{
    static FN_MsimtfIsGuidMapEnable s_fn = NULL;
    if (!cicGetFN(g_hMSIMTF, s_fn, L"msimtf.dll", "MsimtfIsGuidMapEnable"))
        return FALSE;
    return s_fn(hIMC, pbValue);
}

/// @implemented
BOOL IsVKDBEKey(_In_ UINT uVirtKey)
{
    switch (uVirtKey)
    {
        case VK_KANJI:
        case VK_CONVERT:
            return TRUE;
        default:
            return (VK_OEM_ATTN <= uVirtKey && uVirtKey <= VK_PA1);
    }
}

/// @implemented
ITfCategoryMgr *GetUIMCat(PCIC_LIBTHREAD pLibThread)
{
    if (!pLibThread)
        return NULL;

    if (pLibThread->m_pCategoryMgr)
        return pLibThread->m_pCategoryMgr;

    if (FAILED(cicCoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
                                   IID_ITfCategoryMgr, (void **)&pLibThread->m_pCategoryMgr)))
    {
        return NULL;
    }
    return pLibThread->m_pCategoryMgr;
}

/// @implemented
static HRESULT
LibEnumItemsInCategory(PCIC_LIBTHREAD pLibThread, REFGUID rguid, IEnumGUID **ppEnum)
{
    ITfCategoryMgr *pCat = GetUIMCat(pLibThread);
    if (!pCat)
        return E_FAIL;
    return pCat->EnumItemsInCategory(rguid, ppEnum);
}

/// @implemented
HRESULT InitDisplayAttrbuteLib(PCIC_LIBTHREAD pLibThread)
{
    if (!pLibThread)
        return E_FAIL;

    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }

    if (FAILED(cicCoCreateInstance(CLSID_TF_DisplayAttributeMgr, NULL, CLSCTX_INPROC_SERVER,
                                   IID_ITfDisplayAttributeMgr,
                                   (void **)&pLibThread->m_pDisplayAttrMgr)))
    {
        return E_FAIL;
    }

    IEnumGUID *pEnumGuid;
    LibEnumItemsInCategory(pLibThread, GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY, &pEnumGuid);

    HRESULT hr = E_OUTOFMEMORY;

    ::EnterCriticalSection(&g_csLock);
    if (pEnumGuid && !g_pPropCache)
    {
        g_pPropCache = new(cicNoThrow) CDispAttrPropCache();
        if (g_pPropCache)
        {
            g_pPropCache->Add(GUID_PROP_ATTRIBUTE);
            GUID guid;
            while (pEnumGuid->Next(1, &guid, NULL) == S_OK)
            {
                if (!IsEqualGUID(guid, GUID_PROP_ATTRIBUTE))
                    g_pPropCache->Add(guid);
            }
            hr = S_OK;
        }
    }
    ::LeaveCriticalSection(&g_csLock);

    return hr;
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

/***********************************************************************/

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

/***********************************************************************/

struct MODEBIAS
{
    REFGUID m_guid;
    LONG m_bias;
};

static const MODEBIAS g_ModeBiasMap[] =
{
    { GUID_MODEBIAS_FILENAME,   0x00000001 },
    { GUID_MODEBIAS_NUMERIC,    0x00000004 },
    { GUID_MODEBIAS_URLHISTORY, 0x00010000 },
    { GUID_MODEBIAS_DEFAULT,    0x00000000 },
    { GUID_MODEBIAS_NONE,       0x00000000 },
};

/// @implemented
void CModeBias::SetModeBias(REFGUID rguid)
{
    m_guid = rguid;
}

/// @implemented
GUID CModeBias::ConvertModeBias(LONG bias)
{
    const GUID *pguid = &GUID_NULL;
    for (auto& item : g_ModeBiasMap)
    {
        if (item.m_bias == bias)
        {
            pguid = &item.m_guid;
            break;
        }
    }

    return *pguid;
}

/// @implemented
LONG CModeBias::ConvertModeBias(REFGUID guid)
{
    for (auto& item : g_ModeBiasMap)
    {
        if (IsEqualGUID(guid, item.m_guid))
            return item.m_bias;
    }
    return 0;
}

/***********************************************************************/

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
    if (!DllShutdownInProgress())
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

/// @implemented
STDMETHODIMP
CFunctionProviderBase::QueryInterface(
    _In_ REFIID riid,
    _Out_ LPVOID* ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CFunctionProviderBase, ITfFunctionProvider),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
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

/***********************************************************************/

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

/***********************************************************************/

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
    static const QITAB c_tab[] =
    {
        QITABENT(CFnDocFeed, IAImmFnDocFeed),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
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

/// @implemented
STDMETHODIMP CFnDocFeed::DocFeed()
{
    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;

    HIMC hIMC = GetActiveContext();
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return E_FAIL;

    UINT uCodePage = CP_ACP;
    pTLS->m_pProfile->GetCodePageA(&uCodePage);
    pCicIC->SetupDocFeedString(imcLock, uCodePage);
    return S_OK;
}

/// @implemented
STDMETHODIMP CFnDocFeed::ClearDocFeedBuffer()
{
    if (!TLS::GetTLS())
        return E_OUTOFMEMORY;

    HIMC hIMC = GetActiveContext();
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;

    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return E_FAIL;

    pCicIC->EscbClearDocFeedBuffer(imcLock, TRUE);
    return S_OK;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::StartReconvert()
{
    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;
    auto *pThreadMgr = pTLS->m_pThreadMgr;
    if (!pThreadMgr)
        return E_OUTOFMEMORY;

    HIMC hIMC = GetActiveContext();
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return E_FAIL;

    UINT uCodePage = CP_ACP;
    pTLS->m_pProfile->GetCodePageA(&uCodePage);

    pCicIC->m_bReconverting = TRUE;
    pCicIC->SetupReconvertString(imcLock, pThreadMgr, uCodePage, 0, 0);
    pCicIC->EndReconvertString(imcLock);
    pCicIC->m_bReconverting = FALSE;
    return S_OK;
}

/// @implemented
STDMETHODIMP CFnDocFeed::StartUndoCompositionString()
{
    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return E_OUTOFMEMORY;
    auto *pThreadMgr = pTLS->m_pThreadMgr;
    if (!pThreadMgr)
        return E_OUTOFMEMORY;

    HIMC hIMC = GetActiveContext();
    CicIMCLock imcLock(hIMC);
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imeContext(imcLock.get().hCtfImeContext);
    if (FAILED(imeContext.m_hr))
        return imeContext.m_hr;
    CicInputContext *pCicIC = imeContext.get().m_pCicIC;
    if (!pCicIC)
        return E_FAIL;

    UINT uCodePage = CP_ACP;
    pTLS->m_pProfile->GetCodePageA(&uCodePage);

    pCicIC->SetupReconvertString(imcLock, pThreadMgr, uCodePage, 0, TRUE);
    pCicIC->EndReconvertString(imcLock);
    return S_OK;
}
