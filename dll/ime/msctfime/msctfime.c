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
    LPIMEINFO lpIMEInfo,
    LPWSTR lpszWndClass,
    DWORD dwSystemInfoFlags)
{
    FIXME("stub:(%p, %p, 0x%lX)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags);
    return FALSE;
}

DWORD WINAPI
ImeConversionList(
    HIMC hIMC,
    LPCWSTR lpSrc,
    LPCANDIDATELIST lpDst,
    DWORD dwBufLen,
    UINT uFlag)
{
    FIXME("stub:(%p, %s, %p, 0x%lX, %u)\n", hIMC, debugstr_w(lpSrc), lpDst, dwBufLen, uFlag);
    return 0;
}

BOOL WINAPI
ImeRegisterWord(
    LPCWSTR lpszReading,
    DWORD dwStyle,
    LPCWSTR lpszString)
{
    FIXME("stub:(%s, 0x%lX, %s)\n", debugstr_w(lpszReading), dwStyle, debugstr_w(lpszString));
    return FALSE;
}

BOOL WINAPI
ImeUnregisterWord(
    LPCWSTR lpszReading,
    DWORD dwStyle,
    LPCWSTR lpszString)
{
    FIXME("stub:(%s, 0x%lX, %s)\n", debugstr_w(lpszReading), dwStyle, debugstr_w(lpszString));
    return FALSE;
}

UINT WINAPI
ImeGetRegisterWordStyle(
    UINT nItem,
    LPSTYLEBUFW lpStyleBuf)
{
    FIXME("stub:(%u, %p)\n", nItem, lpStyleBuf);
    return 0;
}

UINT WINAPI
ImeEnumRegisterWord(
    REGISTERWORDENUMPROCW lpfnEnumProc,
    LPCWSTR lpszReading,
    DWORD dwStyle,
    LPCWSTR lpszString,
    LPVOID lpData)
{
    FIXME("stub:(%p, %s, %lu, %s, %p)\n", lpfnEnumProc, debugstr_w(lpszReading),
          dwStyle, debugstr_w(lpszString), lpData);
    return 0;
}

BOOL WINAPI
ImeConfigure(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    LPVOID lpData)
{
    FIXME("stub:(%p, %p, %lu, %p)\n", hKL, hWnd, dwMode, lpData);
    return FALSE;
}

BOOL WINAPI
ImeDestroy(
    UINT uReserved)
{
    FIXME("stub:(%u)\n", uReserved);
    return FALSE;
}

LRESULT WINAPI
ImeEscape(
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData)
{
    FIXME("stub:(%p, %u, %p)\n", hIMC, uEscape, lpData);
    return 0;
}

BOOL WINAPI
ImeProcessKey(
    HIMC hIMC,
    UINT uVirKey,
    LPARAM lParam,
    CONST LPBYTE lpbKeyState)
{
    FIXME("stub:(%p, %u, %p, lpbKeyState)\n", hIMC, uVirKey, lParam, lpbKeyState);
    return FALSE;
}

BOOL WINAPI
ImeSelect(
    HIMC hIMC,
    BOOL fSelect)
{
    FIXME("stub:(%p, %u)\n", hIMC, fSelect);
    return FALSE;
}

BOOL WINAPI
ImeSetActiveContext(
    HIMC hIMC,
    BOOL fFlag)
{
    FIXME("stub:(%p, %u)\n", hIMC, fFlag);
    return FALSE;
}

UINT WINAPI
ImeToAsciiEx(
    UINT uVirKey,
    UINT uScanCode,
    CONST LPBYTE lpbKeyState,
    LPTRANSMSGLIST lpTransMsgList,
    UINT fuState,
    HIMC hIMC)
{
    FIXME("stub:(%u, %u, %p, %p, %u, %p)\n", uVirKey, uScanCode, lpbKeyState, lpTransMsgList,
          fuState, hIMC);
    return 0;
}

BOOL WINAPI
NotifyIME(
    HIMC hIMC,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD dwValue)
{
    FIXME("stub:(%p, 0x%lX, 0x%lX, 0x%lX)\n", hIMC, dwAction, dwIndex, dwValue);
    return FALSE;
}

BOOL WINAPI
ImeSetCompositionString(
    HIMC hIMC,
    DWORD dwIndex,
    LPCVOID lpComp,
    DWORD dwCompLen,
    LPCVOID lpRead,
    DWORD dwReadLen)
{
    FIXME("stub:(%p, 0x%lX, %p, 0x%lX, %p, 0x%lX)\n", hIMC, dwIndex, lpComp, dwCompLen,
          lpRead, dwReadLen);
    return FALSE;
}

DWORD WINAPI
ImeGetImeMenuItems(
    HIMC hIMC,
    DWORD dwFlags,
    DWORD dwType,
    LPIMEMENUITEMINFOW lpImeParentMenu,
    LPIMEMENUITEMINFOW lpImeMenu,
    DWORD dwSize)
{
    FIXME("stub:(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n", hIMC, dwFlags, dwType, lpImeParentMenu,
          lpImeMenu, dwSize);
    return 0;
}

BOOL WINAPI
CtfImeInquireExW(
    LPIMEINFO lpIMEInfo,
    LPVOID lpszWndClass,
    DWORD dwSystemInfoFlags,
    HKL hKL)
{
    FIXME("stub:(%p, %p, 0x%lX, %p)\n", lpIMEInfo, lpszWndClass, dwSystemInfoFlags, hKL);
    return FALSE;
}

BOOL WINAPI
CtfImeSelectEx(
    HIMC hIMC,
    BOOL fSelect,
    HKL hKL)
{
    FIXME("stub:(%p, %d, %p)\n", hIMC, fSelect, hKL);
    return FALSE;
}

LRESULT WINAPI
CtfImeEscapeEx(
    HIMC hIMC,
    UINT uSubFunc,
    LPVOID lpData,
    HKL hKL)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hIMC, uSubFunc, lpData, hKL);
    return 0;
}

HRESULT WINAPI
CtfImeGetGuidAtom(
    HIMC hIMC,
    DWORD dwUnknown,
    LPDWORD pdwGuidAtom)
{
    FIXME("stub:(%p, 0x%lX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);
    return E_FAIL;
}

BOOL WINAPI
CtfImeIsGuidMapEnable(
    HIMC hIMC)
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
CtfImeCreateInputContext(HIMC hIMC)
{
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeDestroyInputContext(HIMC hIMC)
{
    FIXME("stub:(%p)\n", hIMC);
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeSetActiveContextAlways(HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL)
{
    FIXME("stub:(%p, %d, %p, %p)\n", hIMC, fActive, hWnd, hKL);
    return E_NOTIMPL;
}

HRESULT WINAPI
CtfImeProcessCicHotkey(HIMC hIMC, UINT vKey, LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p)\n", hIMC, vKey, lParam);
    return E_NOTIMPL;
}

LRESULT WINAPI
CtfImeDispatchDefImeMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("stub:(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);
    return 0;
}

BOOL WINAPI
CtfImeIsIME(HKL hKL)
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
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (uMsg == WM_CREATE)
    {
        FIXME("stub\n");
        return -1;
    }
    return 0;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
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
