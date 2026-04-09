/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing ImmSendIMEMessageExA/W
 * COPYRIGHT:   Copyright 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <wine/ime.h>

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#ifdef IMM_WIN3_SUPPORT /* 3.x support */

static DWORD Imm32TranslateIMESubFunctions(HWND hWnd, PIMESTRUCT pIme, BOOL bAnsi)
{
    FIXME("(%p, %p, %d)\n", hWnd, pIme, bAnsi);
    return 0;
}

static LRESULT Imm32SendIMEMessageExAW(HWND hWnd, HGLOBAL hIME, BOOL bAnsi)
{
    PIMESTRUCT pIme = GlobalLock(hIME);
    if (!pIme)
        return 0;

    LRESULT ret;
    HKL hKL = GetKeyboardLayout(0);
    if (Imm32IsSystemJapaneseOrKorean() && ImmIsIME(hKL))
    {
        HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);
        if (IsWindow(hwndIme))
        {
            HWND hTargetWnd = IsWindow(hWnd) ? hWnd : GetFocus();
            ret = Imm32TranslateIMESubFunctions(hTargetWnd, pIme, bAnsi);
        }
        else
        {
            ret = (pIme->fnc == IME_GETVERSION) ? IME_RS_INVALID : IME_RS_ERROR;
        }
    }
    else
    {
        pIme->wParam = IME_RS_INVALID;
        ret = 0;
    }

    GlobalUnlock(hIME);
    return ret;
}

#endif /* def IMM_WIN3_SUPPORT */

/***********************************************************************
 *		ImmSendIMEMessageExA(IMM32.@)
 */
LRESULT WINAPI
ImmSendIMEMessageExA(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
#ifdef IMM_WIN3_SUPPORT
    return Imm32SendIMEMessageExAW(hWnd, (HGLOBAL)lParam, TRUE);
#else
    return 0;
#endif
}

/***********************************************************************
 *		ImmSendIMEMessageExW(IMM32.@)
 */
LRESULT WINAPI
ImmSendIMEMessageExW(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
#ifdef IMM_WIN3_SUPPORT
    return Imm32SendIMEMessageExAW(hWnd, (HGLOBAL)lParam, FALSE);
#else
    return 0;
#endif
}
