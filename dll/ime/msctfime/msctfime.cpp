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
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT):
        {
            lpIMEInfo->fdwProperty = IME_PROP_COMPLETE_ON_UNSELECT | IME_PROP_SPECIAL_UI | IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY | IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_KATAKANA | IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = IME_SMODE_CONVERSATION | IME_SMODE_PLAURALCLAUSE;
            lpIMEInfo->fdwSelectCaps = SELECT_CAP_SENTENCE | SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD | SCS_CAP_COMPSTR;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        case MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT):
        {
            lpIMEInfo->fdwProperty = IME_PROP_COMPLETE_ON_UNSELECT | IME_PROP_SPECIAL_UI | IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY | IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = 0;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_COMPSTR;
            lpIMEInfo->fdwSelectCaps = SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL):
        {
            lpIMEInfo->fdwProperty = IME_PROP_SPECIAL_UI | IME_PROP_AT_CARET | IME_PROP_NEED_ALTKEY | IME_PROP_KBD_CHAR_FIRST;
            lpIMEInfo->fdwConversionCaps = IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
            lpIMEInfo->fdwSentenceCaps = SELECT_CAP_CONVERSION;
            lpIMEInfo->fdwSelectCaps = 0;
            lpIMEInfo->fdwSCSCaps = SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD | SCS_CAP_COMPSTR;
            lpIMEInfo->fdwUICaps = UI_CAP_ROT90;
            break;
        }
        default:
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

/* FIXME */
struct CicBridge : IUnknown
{
};

/* FIXME */
struct CicProfile : IUnknown
{
};

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

    static TLS* InternalAllocateTLS();
    static BOOL InternalDestroyTLS();
};

DWORD TLS::s_dwTlsIndex = (DWORD)-1;

/**
 * @implemented
 */
TLS* TLS::InternalAllocateTLS()
{
    TLS *pTLS = (TLS *)::TlsGetValue(TLS::s_dwTlsIndex);
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
    if (s_dwTlsIndex == (DWORD)-1)
        return FALSE;

    TLS *pTLS = (TLS *)::TlsGetValue(s_dwTlsIndex);
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
    FIXME("stub:(%p, %p, %lu, %p)\n", hKL, hWnd, dwMode, lpData);
    return FALSE;
}

EXTERN_C BOOL WINAPI
ImeDestroy(
    _In_ UINT uReserved)
{
    FIXME("stub:(%u)\n", uReserved);
    return FALSE;
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

EXTERN_C HRESULT WINAPI
CtfImeGetGuidAtom(
    _In_ HIMC hIMC,
    _In_ DWORD dwUnknown,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    FIXME("stub:(%p, 0x%lX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);
    return E_FAIL;
}

EXTERN_C BOOL WINAPI
CtfImeIsGuidMapEnable(
    _In_ HIMC hIMC)
{
    FIXME("stub:(%p)\n", hIMC);
    return FALSE;
}

EXTERN_C HRESULT WINAPI
CtfImeCreateThreadMgr(VOID)
{
    FIXME("stub:()\n");
    return E_NOTIMPL;
}

EXTERN_C HRESULT WINAPI
CtfImeDestroyThreadMgr(VOID)
{
    FIXME("stub:()\n");
    return E_NOTIMPL;
}

EXTERN_C HRESULT WINAPI
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    return E_NOTIMPL;
}

EXTERN_C HRESULT WINAPI
CtfImeDestroyInputContext(
    _In_ HIMC hIMC)
{
    FIXME("stub:(%p)\n", hIMC);
    return E_NOTIMPL;
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

EXTERN_C LRESULT WINAPI
CtfImeDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);
    return 0;
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
 * @implemented
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
