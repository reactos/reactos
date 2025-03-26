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
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
BOOL gfTFInitLib = FALSE;
CRITICAL_SECTION g_csLock;
CDispAttrPropCache *g_pPropCache = NULL;

/// Selects or unselects the input context.
/// @implemented
static HRESULT
InternalSelectEx(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ LANGID LangID)
{
    CicIMCLock imcLock(hIMC);
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
        HGDIOBJ hFont = ::GetCurrentObject(hDC, OBJ_FONT);
        ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
        ::ReleaseDC(imcLock.get().hWnd, hDC);

        imcLock.get().lfFont.W = lf;
        imcLock.get().fdwInit |= INIT_LOGFONT;
    }
    imcLock.get().lfFont.W.lfCharSet = GetCharsetFromLangId(LangID);

    imcLock.InitContext();

    return imcLock.m_hr;
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

/***********************************************************************
 *      ImeInquire (MSCTFIME.@)
 *
 * MSCTFIME's ImeInquire does nothing.
 *
 * @implemented
 * @see CtfImeInquireExW
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeInquire.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeConversionList.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeRegisterWord.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeUnregisterWord.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeGetRegisterWordStyle.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeEnumRegisterWord.html
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

/***********************************************************************
 *      ImeConfigure (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeConfigure.html
 */
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

    auto pBridge = pTLS->m_pBridge;
    auto pThreadMgr = pTLS->m_pThreadMgr;

    if (dwMode & 0x1)
        return (pBridge->ConfigureGeneral(pTLS, pThreadMgr, hKL, hWnd) == S_OK);

    if (dwMode & 0x2)
        return (pBridge->ConfigureRegisterWord(pTLS, pThreadMgr, hKL, hWnd, lpData) == S_OK);

    return FALSE;
}

/***********************************************************************
 *      ImeDestroy (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeDestroy.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeEscape.html
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

/***********************************************************************
 *      ImeProcessKey (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeProcessKey.html
 */
EXTERN_C BOOL WINAPI
ImeProcessKey(
    _In_ HIMC hIMC,
    _In_ UINT uVirtKey,
    _In_ LPARAM lParam,
    _In_ CONST LPBYTE lpbKeyState)
{
    TRACE("(%p, %u, %p, lpbKeyState)\n", hIMC, uVirtKey, lParam, lpbKeyState);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return FALSE;

    auto pBridge = pTLS->m_pBridge;
    auto pThreadMgr = pTLS->m_pThreadMgr;
    if (!pBridge || !pThreadMgr)
        return FALSE;

    if (pTLS->m_dwFlags1 & 0x1)
    {
        ITfDocumentMgr *pDocMgr = NULL;
        pThreadMgr->GetFocus(&pDocMgr);
        if (pDocMgr && !CicBridge::IsOwnDim(pDocMgr))
        {
            pDocMgr->Release();
            return FALSE;
        }

        if (pDocMgr)
            pDocMgr->Release();
    }

    LANGID LangID = LOWORD(::GetKeyboardLayout(0));
    if (((pTLS->m_dwFlags2 & 1) && MsimtfIsGuidMapEnable(hIMC, NULL)) ||
        ((lParam & (KF_ALTDOWN << 16)) &&
         (LangID == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)) &&
         IsVKDBEKey(uVirtKey)))
    {
        return FALSE;
    }

    INT nUnknown60 = 0;
    return pBridge->ProcessKey(pTLS, pThreadMgr, hIMC, uVirtKey, lParam, lpbKeyState, &nUnknown60);
}

/***********************************************************************
 *      ImeSelect (MSCTFIME.@)
 *
 * MSCTFIME's ImeSelect does nothing.
 *
 * @implemented
 * @see CtfImeSelectEx
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeSelect.html
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
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeSetActiveContext.html
 */
EXTERN_C BOOL WINAPI
ImeSetActiveContext(
    _In_ HIMC hIMC,
    _In_ BOOL fFlag)
{
    TRACE("(%p, %u)\n", hIMC, fFlag);
    return FALSE;
}

/***********************************************************************
 *      ImeToAsciiEx (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeToAsciiEx.html
 */
EXTERN_C UINT WINAPI
ImeToAsciiEx(
    _In_ UINT uVirtKey,
    _In_ UINT uScanCode,
    _In_ CONST LPBYTE lpbKeyState,
    _Out_ LPTRANSMSGLIST lpTransMsgList,
    _In_ UINT fuState,
    _In_ HIMC hIMC)
{
    TRACE("(%u, %u, %p, %p, %u, %p)\n", uVirtKey, uScanCode, lpbKeyState, lpTransMsgList,
          fuState, hIMC);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return 0;

    auto pBridge = pTLS->m_pBridge;
    auto pThreadMgr = pTLS->m_pThreadMgr;
    if (!pBridge || !pThreadMgr)
        return 0;

    UINT ret = 0;
    HRESULT hr = pBridge->ToAsciiEx(pTLS, pThreadMgr, uVirtKey, uScanCode, lpbKeyState,
                                    lpTransMsgList, fuState, hIMC, &ret);
    return ((hr == S_OK) ? ret : 0);
}

/***********************************************************************
 *      NotifyIME (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/NotifyIME.html
 */
EXTERN_C BOOL WINAPI
NotifyIME(
    _In_ HIMC hIMC,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _In_ DWORD_PTR dwValue)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p)\n", hIMC, dwAction, dwIndex, dwValue);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return FALSE;

    auto pBridge = pTLS->m_pBridge;
    auto pThreadMgr = pTLS->m_pThreadMgr;
    if (!pBridge || !pThreadMgr)
        return FALSE;

    HRESULT hr = pBridge->Notify(pTLS, pThreadMgr, hIMC, dwAction, dwIndex, dwValue);
    return (hr == S_OK);
}

/***********************************************************************
 *      ImeSetCompositionString (MSCTFIME.@)
 *
 * @implemented
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImeSetCompositionString.html
 */
EXTERN_C BOOL WINAPI
ImeSetCompositionString(
    _In_ HIMC hIMC,
    _In_ DWORD dwIndex,
    _In_opt_ LPCVOID lpComp,
    _In_ DWORD dwCompLen,
    _In_opt_ LPCVOID lpRead,
    _In_ DWORD dwReadLen)
{
    TRACE("(%p, 0x%lX, %p, 0x%lX, %p, 0x%lX)\n", hIMC, dwIndex, lpComp, dwCompLen,
          lpRead, dwReadLen);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS)
        return FALSE;

    auto pBridge = pTLS->m_pBridge;
    auto pThreadMgr = pTLS->m_pThreadMgr;
    if (!pBridge || !pThreadMgr)
        return FALSE;

    return pBridge->SetCompositionString(pTLS, pThreadMgr, hIMC, dwIndex,
                                         lpComp, dwCompLen, lpRead, dwReadLen);
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

/***********************************************************************
 *      CtfImeEscapeEx (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C LRESULT WINAPI
CtfImeEscapeEx(
    _In_ HIMC hIMC,
    _In_ UINT uSubFunc,
    _Inout_opt_ LPVOID lpData,
    _In_ HKL hKL)
{
    TRACE("(%p, %u, %p, %p)\n", hIMC, uSubFunc, lpData, hKL);

    if (LOWORD(hKL) != MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT))
        return 0;

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->m_pBridge)
        return 0;

    return pTLS->m_pBridge->EscapeKorean(pTLS, hIMC, uSubFunc, lpData);
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
    if (FAILED(imcLock.m_hr))
        return imcLock.m_hr;

    CicIMCCLock<CTFIMECONTEXT> imccLock(imcLock.get().hCtfImeContext);
    if (FAILED(imccLock.m_hr))
        return imccLock.m_hr;

    if (!imccLock.get().m_pCicIC)
        return E_OUTOFMEMORY;

    return imccLock.get().m_pCicIC->GetGuidAtom(imcLock, dwUnknown, pdwGuidAtom);
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
    CicIMCLock imcLock(hIMC);
    if (SUCCEEDED(imcLock.m_hr))
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

/***********************************************************************
 *      CtfImeCreateInputContext (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->m_pBridge)
        return E_OUTOFMEMORY;

    return pTLS->m_pBridge->CreateInputContext(pTLS, hIMC);
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

/***********************************************************************
 *      CtfImeSetActiveContextAlways (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
CtfImeSetActiveContextAlways(
    _In_ HIMC hIMC,
    _In_ BOOL fActive,
    _In_ HWND hWnd,
    _In_ HKL hKL)
{
    TRACE("(%p, %d, %p, %p)\n", hIMC, fActive, hWnd, hKL);

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->m_pBridge)
        return E_OUTOFMEMORY;
    return pTLS->m_pBridge->SetActiveContextAlways(pTLS, hIMC, fActive, hWnd, hKL);
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

    HKL hKL = ::GetKeyboardLayout(0);
    if (IS_IME_HKL(hKL))
        return 0;

    HWND hImeWnd = (HWND)::SendMessageW(hWnd, WM_IME_NOTIFY, 0x17, 0);
    if (!IsWindow(hImeWnd))
        return 0;

    return ::SendMessageW(hImeWnd, uMsg, wParam, lParam);
}

/***********************************************************************
 *      CtfImeIsIME (MSCTFIME.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
CtfImeIsIME(
    _In_ HKL hKL)
{
    TRACE("(%p)\n", hKL);

    if (IS_IME_HKL(hKL))
        return TRUE;

    TLS *pTLS = TLS::GetTLS();
    if (!pTLS || !pTLS->m_pProfile)
        return FALSE;

    // The return value of CicProfile::IsIME is brain-damaged
    return !pTLS->m_pProfile->IsIME(hKL);
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

EXTERN_C VOID TFUninitLib(VOID)
{
    if (g_pPropCache)
    {
        delete g_pPropCache;
        g_pPropCache = NULL;
    }
}

/// @implemented
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    g_hInst = hinstDLL;

    ::InitializeCriticalSectionAndSpinCount(&g_csLock, 0);

    if (!TLS::Initialize())
        return FALSE;

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);

    cicInitUIFLib();

    if (!TFInitLib())
        return FALSE;

    gfTFInitLib = TRUE;
    return AttachIME();
}

/// @implemented
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    TF_DllDetachInOther();

    if (gfTFInitLib)
    {
        DetachIME();
        TFUninitLib();
    }

    ::DeleteCriticalSection(&g_csLock);
    TLS::InternalDestroyTLS();
    TLS::Uninitialize();
    cicDoneUIFLib();
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
