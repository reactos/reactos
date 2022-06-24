/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 Cicero (modern input method)
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

// Win: LoadCtfIme
HMODULE APIENTRY Imm32LoadCtfIme(VOID)
{
    FIXME("()\n");
    return NULL;
}

// Win: Internal_CtfImeDestroyInputContext
HRESULT APIENTRY Imm32CtfImeDestroyInputContext(HIMC hIMC)
{
    if (!Imm32LoadCtfIme())
        return E_FAIL;
#if 1
    FIXME("(%p)\n", hIMC);
    return E_NOTIMPL;
#else
    return g_pfnCtfImeDestroyInputContext(hIMC);
#endif
}

// Win: CtfImmTIMDestroyInputContext
HRESULT APIENTRY CtfImmTIMDestroyInputContext(HIMC hIMC)
{
    if (!Imm32IsCiceroMode() || (GetWin32ClientInfo()->dwCompatFlags2 & 2))
        return E_NOINTERFACE;

    return Imm32CtfImeDestroyInputContext(hIMC);
}

// Win: CtfImmTIMCreateInputContext
HRESULT APIENTRY CtfImmTIMCreateInputContext(HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(VOID)
{
    return Imm32IsCiceroMode();
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID)
{
    return !!(GetWin32ClientInfo()->CI_flags & CI_TFSDISABLED);
}

/***********************************************************************
 *		CtfImmTIMActivate(IMM32.@)
 */
HRESULT WINAPI CtfImmTIMActivate(HKL hKL)
{
    FIXME("(%p)\n", hKL);
    return E_NOTIMPL;
}

/***********************************************************************
 *		CtfImmRestoreToolbarWnd(IMM32.@)
 */
VOID WINAPI CtfImmRestoreToolbarWnd(DWORD dwStatus)
{
    FIXME("(0x%lx)\n", dwStatus);
}

/***********************************************************************
 *		CtfImmHideToolbarWnd(IMM32.@)
 */
DWORD WINAPI CtfImmHideToolbarWnd(VOID)
{
    FIXME("()\n");
    return 0;
}

/***********************************************************************
 *		CtfImmDispatchDefImeMessage(IMM32.@)
 */
LRESULT WINAPI CtfImmDispatchDefImeMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("(%p, %u, %p, %p)\n", hWnd, uMsg, wParam, lParam);
    return 0;
}
