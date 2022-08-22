/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing the IMM32 Cicero-aware Text Framework (CTF)
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/*
 * NOTE: Microsoft CTF protocol (ctfmon.exe and CTF clients) has massive vulnerability.
 *       We don't follow the design of some parts of Microsoft's CTF protocol if insecure.
 *
 * See also:
 * https://www.zdnet.com/article/vulnerability-in-microsoft-ctf-protocol-goes-back-to-windows-xp/
 */

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

/***********************************************************************
 *		CtfImmIsGuidMapEnable(IMM32.@)
 */
BOOL WINAPI CtfImmIsGuidMapEnable(HIMC hIMC)
{
    DWORD dwThreadId;
    HKL hKL;
    PIMEDPI pImeDpi;
    BOOL ret = FALSE;

    TRACE("(%p)\n", hIMC);

    if (!Imm32IsCiceroMode() || Imm32Is16BitMode())
        return ret;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    hKL = GetKeyboardLayout(dwThreadId);

    if (IS_IME_HKL(hKL))
        return ret;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return ret;

    if (pImeDpi->CtfImeIsGuidMapEnable)
        ret = pImeDpi->CtfImeIsGuidMapEnable(hIMC);

    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		CtfImmGetGuidAtom(IMM32.@)
 */
HRESULT WINAPI CtfImmGetGuidAtom(HIMC hIMC, DWORD dwUnknown, LPDWORD pdwGuidAtom)
{
    HRESULT hr = E_FAIL;
    PIMEDPI pImeDpi;
    DWORD dwThreadId;
    HKL hKL;

    TRACE("(%p, 0xlX, %p)\n", hIMC, dwUnknown, pdwGuidAtom);

    *pdwGuidAtom = 0;

    if (!Imm32IsCiceroMode() || Imm32Is16BitMode())
        return hr;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    hKL = GetKeyboardLayout(dwThreadId);
    if (IS_IME_HKL(hKL))
        return S_OK;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return hr;

    if (pImeDpi->CtfImeGetGuidAtom)
        hr = pImeDpi->CtfImeGetGuidAtom(hIMC, dwUnknown, pdwGuidAtom);

    ImmUnlockImeDpi(pImeDpi);
    return hr;
}
