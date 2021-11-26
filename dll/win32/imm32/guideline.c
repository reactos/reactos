/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 guidelines
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

DWORD APIENTRY
ImmGetGuideLineAW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsi)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    LPGUIDELINE pGuideLine;
    DWORD cb, ret = 0;
    LPVOID pvStr, pvPrivate;
    BOOL bUsedDefault;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pGuideLine = ImmLockIMCC(pIC->hGuideLine);
    if (!pGuideLine)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (dwIndex == GGL_LEVEL)
    {
        ret = pGuideLine->dwLevel;
        goto Quit;
    }

    if (dwIndex == GGL_INDEX)
    {
        ret = pGuideLine->dwIndex;
        goto Quit;
    }

    if (dwIndex == GGL_STRING)
    {
        pvStr = (LPBYTE)pGuideLine + pGuideLine->dwStrOffset;

        /* get size */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                cb = WideCharToMultiByte(CP_ACP, 0, pvStr, pGuideLine->dwStrLen,
                                         NULL, 0, NULL, &bUsedDefault);
            }
            else
            {
                cb = pGuideLine->dwStrLen * sizeof(CHAR);
            }
        }
        else
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                cb = pGuideLine->dwStrLen * sizeof(WCHAR);
            }
            else
            {
                cb = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvStr, pGuideLine->dwStrLen,
                                         NULL, 0) * sizeof(WCHAR);
            }
        }

        if (dwBufLen == 0 || cb == 0 || lpBuf == NULL || dwBufLen < cb)
        {
            ret = cb;
            goto Quit;
        }

        /* store to buffer */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            {
                ret = WideCharToMultiByte(CP_ACP, 0, pvStr, pGuideLine->dwStrLen,
                                          lpBuf, dwBufLen, NULL, &bUsedDefault);
                goto Quit;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE))
            {
                ret = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pvStr, pGuideLine->dwStrLen,
                                          lpBuf, dwBufLen) * sizeof(WCHAR);
                goto Quit;
            }
        }

        RtlCopyMemory(lpBuf, pvStr, cb);
        ret = cb;
        goto Quit;
    }

    if (dwIndex == GGL_PRIVATE)
    {
        pvPrivate = (LPBYTE)pGuideLine + pGuideLine->dwPrivateOffset;

        /* get size */
        if (bAnsi)
        {
            if ((pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                cb = CandidateListWideToAnsi(pvPrivate, NULL, 0, CP_ACP);
            }
            else
            {
                cb = pGuideLine->dwPrivateSize;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                cb = CandidateListAnsiToWide(pvPrivate, NULL, 0, CP_ACP);
            }
            else
            {
                cb = pGuideLine->dwPrivateSize;
            }
        }

        if (dwBufLen == 0 || cb == 0 || lpBuf == NULL || dwBufLen < cb)
        {
            ret = cb;
            goto Quit;
        }

        /* store to buffer */
        if (bAnsi)
        {
            if ((pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                ret = CandidateListWideToAnsi(pvPrivate, lpBuf, cb, CP_ACP);
                goto Quit;
            }
        }
        else
        {
            if (!(pClientImc->dwFlags & CLIENTIMC_WIDE) &&
                pGuideLine->dwIndex == GL_ID_REVERSECONVERSION)
            {
                ret = CandidateListAnsiToWide(pvPrivate, lpBuf, cb, CP_ACP);
                goto Quit;
            }
        }

        RtlCopyMemory(lpBuf, pvPrivate, cb);
        ret = cb;
        goto Quit;
    }

Quit:
    ImmUnlockIMCC(pIC->hGuideLine);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);
    return ImmGetGuideLineAW(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetGuideLineW (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineW(HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);
    return ImmGetGuideLineAW(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}
