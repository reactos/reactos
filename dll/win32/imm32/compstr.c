/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing composition strings of IMM32
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

LONG APIENTRY
Imm32GetCompStrA(HIMC hIMC, const COMPOSITIONSTRING *pCS, DWORD dwIndex,
                 LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsiClient, UINT uCodePage)
{
    LONG ret = IMM_ERROR_GENERAL;

    if (bAnsiClient)
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
            case GCS_COMPREADATTR:
            case GCS_COMPREADCLAUSE:
            case GCS_COMPSTR:
            case GCS_COMPATTR:
            case GCS_COMPCLAUSE:
            case GCS_CURSORPOS:
            case GCS_DELTASTART:
            case GCS_RESULTREADSTR:
            case GCS_RESULTREADCLAUSE:
            case GCS_RESULTSTR:
            case GCS_RESULTCLAUSE:
            default:
                FIXME("TODO:\n");
                break;
        }
    }
    else /* !bAnsiClient */
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
            case GCS_COMPREADATTR:
            case GCS_COMPREADCLAUSE:
            case GCS_COMPSTR:
            case GCS_COMPATTR:
            case GCS_COMPCLAUSE:
            case GCS_CURSORPOS:
            case GCS_DELTASTART:
            case GCS_RESULTREADSTR:
            case GCS_RESULTREADCLAUSE:
            case GCS_RESULTSTR:
            case GCS_RESULTCLAUSE:
            default:
                FIXME("TODO:\n");
                break;
        }
    }

    return ret;
}

LONG APIENTRY
Imm32GetCompStrW(HIMC hIMC, const COMPOSITIONSTRING *pCS, DWORD dwIndex,
                 LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsiClient, UINT uCodePage)
{
    LONG ret = IMM_ERROR_GENERAL;

    if (bAnsiClient)
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
            case GCS_COMPREADATTR:
            case GCS_COMPREADCLAUSE:
            case GCS_COMPSTR:
            case GCS_COMPATTR:
            case GCS_COMPCLAUSE:
            case GCS_CURSORPOS:
            case GCS_DELTASTART:
            case GCS_RESULTREADSTR:
            case GCS_RESULTREADCLAUSE:
            case GCS_RESULTSTR:
            case GCS_RESULTCLAUSE:
            default:
                FIXME("TODO:\n");
                break;
        }
    }
    else /* !bAnsiClient */
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
            case GCS_COMPREADATTR:
            case GCS_COMPREADCLAUSE:
            case GCS_COMPSTR:
            case GCS_COMPATTR:
            case GCS_COMPCLAUSE:
            case GCS_CURSORPOS:
            case GCS_DELTASTART:
            case GCS_RESULTREADSTR:
            case GCS_RESULTREADCLAUSE:
            case GCS_RESULTSTR:
            case GCS_RESULTCLAUSE:
            default:
                FIXME("TODO:\n");
                break;
        }
    }

    return ret;
}

BOOL APIENTRY
Imm32SetCompositionStringAW(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                            LPCVOID lpRead, DWORD dwReadLen, BOOL bAnsi)
{
    FIXME("TODO:\n");
    return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    LONG ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCOMPOSITIONSTRING pCS;
    BOOL bAnsiClient;

    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (dwBufLen && !lpBuf)
        return 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    bAnsiClient = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    ret = Imm32GetCompStrA(hIMC, pCS, dwIndex, lpBuf, dwBufLen, bAnsiClient, CP_ACP);
    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    LONG ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCOMPOSITIONSTRING pCS;
    BOOL bAnsiClient;

    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (dwBufLen && !lpBuf)
        return 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    bAnsiClient = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    ret = Imm32GetCompStrW(hIMC, pCS, dwIndex, lpBuf, dwBufLen, bAnsiClient, CP_ACP);
    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    TRACE("(%p, %lu, %p, %lu, %p, %lu)\n",
          hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    return Imm32SetCompositionStringAW(hIMC, dwIndex, lpComp, dwCompLen,
                                       lpRead, dwReadLen, TRUE);
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    TRACE("(%p, %lu, %p, %lu, %p, %lu)\n",
          hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    return Imm32SetCompositionStringAW(hIMC, dwIndex, lpComp, dwCompLen,
                                       lpRead, dwReadLen, FALSE);
}
