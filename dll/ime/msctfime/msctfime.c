/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Supporting IME interface of Text Input Processors (TIPs)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

HINSTANCE g_hInst = NULL; /* The instance of this module */

BOOL WINAPI
ImeInquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags)
{
    FIXME("stub:(%p, %p, 0x%lX)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags);
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
DWORD WINAPI
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
BOOL WINAPI
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
BOOL WINAPI
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
UINT WINAPI
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
UINT WINAPI
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

BOOL WINAPI
ImeConfigure(
    _In_ HKL hKL,
    _In_ HWND hWnd,
    _In_ DWORD dwMode,
    _Inout_opt_ LPVOID lpData)
{
    FIXME("stub:(%p, %p, %lu, %p)\n", hKL, hWnd, dwMode, lpData);
    return FALSE;
}

BOOL WINAPI
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
LRESULT WINAPI
ImeEscape(
    _In_ HIMC hIMC,
    _In_ UINT uEscape,
    _Inout_opt_ LPVOID lpData)
{
    TRACE("(%p, %u, %p)\n", hIMC, uEscape, lpData);
    return 0;
}

BOOL WINAPI
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
BOOL WINAPI
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
BOOL WINAPI
ImeSetActiveContext(
    _In_ HIMC hIMC,
    _In_ BOOL fFlag)
{
    TRACE("(%p, %u)\n", hIMC, fFlag);
    return FALSE;
}

UINT WINAPI
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

BOOL WINAPI
NotifyIME(
    _In_ HIMC hIMC,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _In_ DWORD_PTR dwValue)
{
    FIXME("stub:(%p, 0x%lX, 0x%lX, %p)\n", hIMC, dwAction, dwIndex, dwValue);
    return FALSE;
}

BOOL WINAPI
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

DWORD WINAPI
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

BOOL WINAPI
CtfImeInquireExW(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %p, 0x%lX, %p)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags, hKL);
    return FALSE;
}

BOOL WINAPI
CtfImeSelectEx(
    _In_ HIMC hIMC,
    _In_ BOOL fSelect,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %d, %p)\n", hIMC, fSelect, hKL);
    return FALSE;
}

LRESULT WINAPI
CtfImeEscapeEx(
    _In_ HIMC hIMC,
    _In_ UINT uSubFunc,
    _Inout_opt_ LPVOID lpData,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hIMC, uSubFunc, lpData, hKL);
    return 0;
}

HRESULT WINAPI
CtfImeGetGuidAtom(
    _In_ HIMC hIMC,
    _In_ DWORD dwUnknown,
    _Out_opt_ LPDWORD pdwGuidAtom)
{
    FIXME("stub:(%p, 0x%lX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);
    return E_FAIL;
}

BOOL WINAPI
CtfImeIsGuidMapEnable(
    _In_ HIMC hIMC)
{
    FIXME("stub:(%p)\n", hIMC);
    return FALSE;
}

HRESULT WINAPI
CtfImeCreateThreadMgr(VOID)
{
    FIXME("stub:()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeDestroyThreadMgr(VOID)
{
    FIXME("stub:()\n");
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeCreateInputContext(
    _In_ HIMC hIMC)
{
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeDestroyInputContext(
    _In_ HIMC hIMC)
{
    FIXME("stub:(%p)\n", hIMC);
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeSetActiveContextAlways(
    _In_ HIMC hIMC,
    _In_ BOOL fActive,
    _In_ HWND hWnd,
    _In_ HKL hKL)
{
    FIXME("stub:(%p, %d, %p, %p)\n", hIMC, fActive, hWnd, hKL);
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeProcessCicHotkey(
    _In_ HIMC hIMC,
    _In_ UINT vKey,
    _In_ LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p)\n", hIMC, vKey, lParam);
    return E_NOTIMPL;
}

LRESULT WINAPI
CtfImeDispatchDefImeMessage(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);
    return 0;
}

BOOL WINAPI
CtfImeIsIME(
    _In_ HKL hKL)
{
    FIXME("stub:(%p)\n", hKL);
    return FALSE;
}

HRESULT WINAPI
CtfImeThreadDetach(VOID)
{
    ImeDestroy(0);
    return S_OK;
}

LRESULT CALLBACK
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

BOOL WINAPI
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
            g_hInst = hinstDLL;
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }
    return TRUE;
}
