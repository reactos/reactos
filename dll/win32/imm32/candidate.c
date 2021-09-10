/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 candidate lists
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

DWORD APIENTRY
CandidateListWideToAnsi(const CANDIDATELIST *pWideCL, LPCANDIDATELIST pAnsiCL, DWORD dwBufLen,
                        UINT uCodePage)
{
    BOOL bUsedDefault;
    DWORD dwSize, dwIndex, cbGot, cbLeft;
    const BYTE *pbWide;
    LPBYTE pbAnsi;
    LPDWORD pibOffsets;

    /* calculate total ansi size */
    if (pWideCL->dwCount > 0)
    {
        dwSize = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        for (dwIndex = 0; dwIndex < pWideCL->dwCount; ++dwIndex)
        {
            pbWide = (const BYTE *)pWideCL + pWideCL->dwOffset[dwIndex];
            cbGot = WideCharToMultiByte(uCodePage, 0, (LPCWSTR)pbWide, -1, NULL, 0,
                                        NULL, &bUsedDefault);
            dwSize += cbGot;
        }
    }
    else
    {
        dwSize = sizeof(CANDIDATELIST);
    }

    dwSize = ROUNDUP4(dwSize);
    if (dwBufLen == 0)
        return dwSize;
    if (dwBufLen < dwSize)
        return 0;

    /* store to ansi */
    pAnsiCL->dwSize = dwBufLen;
    pAnsiCL->dwStyle = pWideCL->dwStyle;
    pAnsiCL->dwCount = pWideCL->dwCount;
    pAnsiCL->dwSelection = pWideCL->dwSelection;
    pAnsiCL->dwPageStart = pWideCL->dwPageStart;
    pAnsiCL->dwPageSize = pWideCL->dwPageSize;

    pibOffsets = pAnsiCL->dwOffset;
    if (pWideCL->dwCount > 0)
    {
        pibOffsets[0] = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        cbLeft = dwBufLen - pibOffsets[0];

        for (dwIndex = 0; dwIndex < pWideCL->dwCount; ++dwIndex)
        {
            pbWide = (const BYTE *)pWideCL + pWideCL->dwOffset[dwIndex];
            pbAnsi = (LPBYTE)pAnsiCL + pibOffsets[dwIndex];

            /* convert to ansi */
            cbGot = WideCharToMultiByte(uCodePage, 0, (LPCWSTR)pbWide, -1,
                                        (LPSTR)pbAnsi, cbLeft, NULL, &bUsedDefault);
            cbLeft -= cbGot;

            if (dwIndex < pWideCL->dwCount - 1)
                pibOffsets[dwIndex + 1] = pibOffsets[dwIndex] + cbGot;
        }
    }
    else
    {
        pibOffsets[0] = sizeof(CANDIDATELIST);
    }

    return dwBufLen;
}

DWORD APIENTRY
CandidateListAnsiToWide(const CANDIDATELIST *pAnsiCL, LPCANDIDATELIST pWideCL, DWORD dwBufLen,
                        UINT uCodePage)
{
    DWORD dwSize, dwIndex, cchGot, cbGot, cbLeft;
    const BYTE *pbAnsi;
    LPBYTE pbWide;
    LPDWORD pibOffsets;

    /* calculate total wide size */
    if (pAnsiCL->dwCount > 0)
    {
        dwSize = sizeof(CANDIDATELIST) + ((pAnsiCL->dwCount - 1) * sizeof(DWORD));
        for (dwIndex = 0; dwIndex < pAnsiCL->dwCount; ++dwIndex)
        {
            pbAnsi = (const BYTE *)pAnsiCL + pAnsiCL->dwOffset[dwIndex];
            cchGot = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, (LPCSTR)pbAnsi, -1, NULL, 0);
            dwSize += cchGot * sizeof(WCHAR);
        }
    }
    else
    {
        dwSize = sizeof(CANDIDATELIST);
    }

    dwSize = ROUNDUP4(dwSize);
    if (dwBufLen == 0)
        return dwSize;
    if (dwBufLen < dwSize)
        return 0;

    /* store to wide */
    pWideCL->dwSize = dwBufLen;
    pWideCL->dwStyle = pAnsiCL->dwStyle;
    pWideCL->dwCount = pAnsiCL->dwCount;
    pWideCL->dwSelection = pAnsiCL->dwSelection;
    pWideCL->dwPageStart = pAnsiCL->dwPageStart;
    pWideCL->dwPageSize = pAnsiCL->dwPageSize;

    pibOffsets = pWideCL->dwOffset;
    if (pAnsiCL->dwCount > 0)
    {
        pibOffsets[0] = sizeof(CANDIDATELIST) + ((pWideCL->dwCount - 1) * sizeof(DWORD));
        cbLeft = dwBufLen - pibOffsets[0];

        for (dwIndex = 0; dwIndex < pAnsiCL->dwCount; ++dwIndex)
        {
            pbAnsi = (const BYTE *)pAnsiCL + pAnsiCL->dwOffset[dwIndex];
            pbWide = (LPBYTE)pWideCL + pibOffsets[dwIndex];

            /* convert to wide */
            cchGot = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, (LPCSTR)pbAnsi, -1,
                                         (LPWSTR)pbWide, cbLeft / sizeof(WCHAR));
            cbGot = cchGot * sizeof(WCHAR);
            cbLeft -= cbGot;

            if (dwIndex + 1 < pAnsiCL->dwCount)
                pibOffsets[dwIndex + 1] = pibOffsets[dwIndex] + cbGot;
        }
    }
    else
    {
        pibOffsets[0] = sizeof(CANDIDATELIST);
    }

    return dwBufLen;
}

static DWORD APIENTRY
ImmGetCandidateListAW(HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen,
                      BOOL bAnsi)
{
    DWORD ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCANDIDATEINFO pCI;
    LPCANDIDATELIST pCL;
    DWORD dwSize;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pCI = ImmLockIMCC(pIC->hCandInfo);
    if (pCI == NULL)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (pCI->dwSize < sizeof(CANDIDATEINFO) || pCI->dwCount <= dwIndex)
        goto Quit;

    /* get required size */
    pCL = (LPCANDIDATELIST)((LPBYTE)pCI + pCI->dwOffset[dwIndex]);
    if (bAnsi)
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            dwSize = CandidateListAnsiToWide(pCL, NULL, 0, CP_ACP);
        else
            dwSize = pCL->dwSize;
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
            dwSize = pCL->dwSize;
        else
            dwSize = CandidateListWideToAnsi(pCL, NULL, 0, CP_ACP);
    }

    if (dwBufLen != 0 && dwSize != 0)
    {
        if (lpCandList == NULL || dwBufLen < dwSize)
            goto Quit;

        /* store */
        if (bAnsi)
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
                CandidateListAnsiToWide(pCL, lpCandList, dwSize, CP_ACP);
            else
                RtlCopyMemory(lpCandList, pCL, dwSize);
        }
        else
        {
            if (pClientImc->dwFlags & CLIENTIMC_WIDE)
                RtlCopyMemory(lpCandList, pCL, dwSize);
            else
                CandidateListWideToAnsi(pCL, lpCandList, dwSize, CP_ACP);
        }
    }

    ret = dwSize;

Quit:
    ImmUnlockIMCC(pIC->hCandInfo);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

DWORD APIENTRY
ImmGetCandidateListCountAW(HIMC hIMC, LPDWORD lpdwListCount, BOOL bAnsi)
{
    DWORD ret = 0, cbGot, dwIndex;
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    const CANDIDATEINFO *pCI;
    const BYTE *pb;
    const CANDIDATELIST *pCL;
    const DWORD *pdwOffsets;

    if (lpdwListCount == NULL)
        return 0;

    *lpdwListCount = 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return 0;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    pCI = ImmLockIMCC(pIC->hCandInfo);
    if (pCI == NULL)
    {
        ImmUnlockIMC(hIMC);
        ImmUnlockClientImc(pClientImc);
        return 0;
    }

    if (pCI->dwSize < sizeof(CANDIDATEINFO))
        goto Quit;

    *lpdwListCount = pCI->dwCount; /* the number of candidate lists */

    /* calculate total size of candidate lists */
    if (bAnsi)
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
        {
            ret = ROUNDUP4(pCI->dwPrivateSize);
            pdwOffsets = pCI->dwOffset;
            for (dwIndex = 0; dwIndex < pCI->dwCount; ++dwIndex)
            {
                pb = (const BYTE *)pCI + pdwOffsets[dwIndex];
                pCL = (const CANDIDATELIST *)pb;
                cbGot = CandidateListWideToAnsi(pCL, NULL, 0, CP_ACP);
                ret += cbGot;
            }
        }
        else
        {
            ret = pCI->dwSize;
        }
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_WIDE)
        {
            ret = pCI->dwSize;
        }
        else
        {
            ret = ROUNDUP4(pCI->dwPrivateSize);
            pdwOffsets = pCI->dwOffset;
            for (dwIndex = 0; dwIndex < pCI->dwCount; ++dwIndex)
            {
                pb = (const BYTE *)pCI + pdwOffsets[dwIndex];
                pCL = (const CANDIDATELIST *)pb;
                cbGot = CandidateListAnsiToWide(pCL, NULL, 0, CP_ACP);
                ret += cbGot;
            }
        }
    }

Quit:
    ImmUnlockIMCC(pIC->hCandInfo);
    ImmUnlockIMC(hIMC);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI
ImmGetCandidateListA(HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    return ImmGetCandidateListAW(hIMC, dwIndex, lpCandList, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(HIMC hIMC, LPDWORD lpdwListCount)
{
    return ImmGetCandidateListCountAW(hIMC, lpdwListCount, TRUE);
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(HIMC hIMC, LPDWORD lpdwListCount)
{
    return ImmGetCandidateListCountAW(hIMC, lpdwListCount, FALSE);
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI
ImmGetCandidateListW(HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    return ImmGetCandidateListAW(hIMC, dwIndex, lpCandList, dwBufLen, FALSE);
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI
ImmGetCandidateWindow(HIMC hIMC, DWORD dwIndex, LPCANDIDATEFORM lpCandidate)
{
    BOOL ret = FALSE;
    LPINPUTCONTEXT pIC;
    LPCANDIDATEFORM pCF;

    TRACE("(%p, %lu, %p)\n", hIMC, dwIndex, lpCandidate);

    pIC = ImmLockIMC(hIMC);
    if (pIC  == NULL)
        return FALSE;

    pCF = &pIC->cfCandForm[dwIndex];
    if (pCF->dwIndex != IMM_INVALID_CANDFORM)
    {
        *lpCandidate = *pCF;
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    HWND hWnd;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lpCandidate);

    if (lpCandidate->dwIndex >= MAX_CANDIDATEFORM)
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->cfCandForm[lpCandidate->dwIndex] = *lpCandidate;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS,
                      IMN_SETCANDIDATEPOS, (1 << (BYTE)lpCandidate->dwIndex));
    return TRUE;
}
