/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing ImmSendIMEMessageExA/W
 * COPYRIGHT:   Copyright 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <ime.h>

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#ifdef IMM_WIN3_SUPPORT /* 3.x support */

static BOOL Imm32IsForegroundThread(HWND hWnd)
{
    HWND hwndFore = GetForegroundWindow();
    DWORD dwTID1 = IsWindow(hWnd) ? GetWindowThreadProcessId(hWnd, NULL) : GetCurrentThreadId();
    DWORD dwTID2 = GetWindowThreadProcessId(hwndFore, NULL);
    return dwTID1 == dwTID2;
}

static BOOL Imm32PostImsMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);
    if (!hwndIme)
        return FALSE;
    return PostMessageW(hwndIme, WM_IME_SYSTEM, wParam, lParam);
}

static BOOL Imm32SetCandidateWindow(HWND hWnd, HIMC hIMC, PCANDIDATEFORM lpCandidate)
{
    PINPUTCONTEXT pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    BOOL ret;
    DWORD dwCompatFlags = ImmGetAppCompatFlags(hIMC);
    if (dwCompatFlags & _IME_APP_COMPAT_DIRECT_IME_SYSTEM)
    {
        RtlCopyMemory(&pIC->cfCandForm[lpCandidate->dwIndex], lpCandidate, sizeof(CANDIDATEFORM));
        ret = Imm32PostImsMessage(hWnd, IMS_SETCANDFORM, lpCandidate->dwIndex);
    }
    else
    {
        ret = ImmSetCandidateWindow(hIMC, lpCandidate);
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

static BOOL Imm32SetCompWindow(HWND hWnd, HIMC hIMC, PCOMPOSITIONFORM lpCompForm)
{
    PINPUTCONTEXTDX pIC = (PINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    BOOL ret;
    DWORD dwCompatFlags = ImmGetAppCompatFlags(hIMC);
    if (dwCompatFlags & _IME_APP_COMPAT_DIRECT_IME_SYSTEM)
    {
        RtlCopyMemory(&pIC->cfCompForm, lpCompForm, sizeof(COMPOSITIONFORM));
        if (dwCompatFlags & _IME_APP_COMPAT_SPECIAL_IME)
            ret = PostMessageW(hWnd, WM_IME_SYSTEM, IMS_SETCOMPFORM, 0);
        else
            ret = Imm32PostImsMessage(hWnd, IMS_SETCOMPFORM, 0);
    }
    else
    {
        pIC->dwUIFlags |= _IME_UI_NO_COMPFORM;
        ret = ImmSetCompositionWindow(hIMC, lpCompForm);
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/* NOTE: K stands for Korean. J stands for Japanese. */
static DWORD Imm32Get31ModeFrom40ModeK(DWORD fdwConversion)
{
    DWORD flags = 0;
    if (!(fdwConversion & IME_CMODE_NATIVE))
        flags |= IME_MODE_ALPHANUMERIC;
    if (!(fdwConversion & IME_CMODE_FULLSHAPE))
        flags |= _IME_MODE_KOR_SBCSCHAR;
    if (fdwConversion & IME_CMODE_HANJACONVERT)
        flags |= IME_MODE_HANJACONVERT;
    return flags;
}

static DWORD Imm32Get31ModeFrom40ModeJ(DWORD fdwConversion)
{
    DWORD flags = 0;
    if (fdwConversion & IME_CMODE_NATIVE)
    {
        if ((fdwConversion & IME_CMODE_KATAKANA))
            flags |= IME_MODE_KATAKANA;
        else
            flags |= IME_MODE_HIRAGANA;
    }
    else
    {
        flags |= IME_MODE_ALPHANUMERIC;
    }
    if (fdwConversion & IME_CMODE_FULLSHAPE)
        flags |= IME_MODE_DBCSCHAR;
    else
        flags |= _IME_MODE_JPN_SBCSCHAR;
    if (fdwConversion & IME_CMODE_ROMAN)
        flags |= IME_MODE_ROMAN;
    else
        flags |= IME_MODE_NOROMAN;
    if (fdwConversion & IME_CMODE_CHARCODE)
        flags |= IME_MODE_CODEINPUT;
    else
        flags |= IME_MODE_NOCODEINPUT;
    return flags;
}

static LRESULT Imm32TransGetMode(HIMC hIMC)
{
    DWORD fdwConversion = 0, fdwSentence;
    ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);
    return Imm32Get31ModeFrom40ModeK(fdwConversion) | _IME_CMODE_EXTENDED;
}

static DWORD Imm32TransSetMode(HIMC hIMC, PIMESTRUCT pIme)
{
    DWORD fdwConversion = 0, fdwSentence;
    ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);

    DWORD dw31Mode = Imm32Get31ModeFrom40ModeK(fdwConversion);
    WPARAM wParam = pIme->wParam;
    if (!(wParam & _IME_MODE_KOR_SBCSCHAR))
        fdwConversion |= IME_CMODE_FULLSHAPE;

    DWORD dwNewFlags = fdwConversion;
    const DWORD targetMask = IME_CMODE_NATIVE | IME_CMODE_KATAKANA | IME_CMODE_FULLSHAPE |
                             IME_CMODE_HANJACONVERT;
    if (wParam & IME_MODE_ALPHANUMERIC)
        dwNewFlags &= ~targetMask;
    else
        dwNewFlags |= targetMask;

    if (!ImmSetConversionStatus(hIMC, dwNewFlags, fdwSentence))
        return 0;
    return dw31Mode;
}

static BOOL Imm32TransCodeConvert(HIMC hIMC, PIMESTRUCT pIme)
{
    HKL hKL = GetKeyboardLayout(0);
    UINT uSubFunc = HIWORD(pIme->wParam);
    switch (uSubFunc)
    {
        case IME_BANJAtoJUNJA:
        case IME_JUNJAtoBANJA:
        case IME_JOHABtoKS:
        case IME_KStoJOHAB:
            return (BOOL)ImmEscapeW(hKL, hIMC, uSubFunc, pIme);
        default:
            return FALSE;
    }
}

static LRESULT Imm32TransGetOpenK(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme, BOOL bAnsi)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);
    LPARAM OldlParam2 = pIme->lParam2;
    pIme->lParam2 = MAKELONG(rc.top, rc.left); /* Correct! */
    HKL hKL = GetKeyboardLayout(0);
    LRESULT result = ImmEscapeW(hKL, hIMC, IME_GETOPEN, pIme);
    pIme->lParam2 = OldlParam2;
    return result;
}

static BOOL Imm32TransGetOpenJ(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme, BOOL bAnsi)
{
    BOOL fOpen = ImmGetOpenStatus(hIMC);

    LONG cbCompStr;
    if (bAnsi)
        cbCompStr = ImmGetCompositionStringA(hIMC, GCS_COMPSTR, NULL, 0);
    else
        cbCompStr = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);

    pIme->wCount = (cbCompStr > 0) ? cbCompStr : 0;
    return fOpen;
}

static LRESULT Imm32TransSetOpenK(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme)
{
    HKL hKL = GetKeyboardLayout(0);
    return ImmEscapeW(hKL, hIMC, IME_SETOPEN, pIme);
}

static BOOL Imm32TransSetOpenJ(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme)
{
    LRESULT fOldOpen = ImmGetOpenStatus(hIMC);

    if (Imm32IsForegroundThread(NULL) || GetFocus())
    {
        ImmSetOpenStatus(hIMC, (BOOL)pIme->wParam);
        return fOldOpen;
    }

    PINPUTCONTEXT pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return fOldOpen;

    BOOL bRequestOpen = !!pIme->wParam;
    if (pIC->fOpen != bRequestOpen)
    {
        pIC->fOpen = bRequestOpen;
        ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETOPENSTATUS);
    }

    ImmUnlockIMC(hIMC);
    return fOldOpen;
}

static UINT Imm32TransConvertList(HIMC hIMC, PIMESTRUCT pIme)
{
    /* SECURITY: Check memory block size */
    const SIZE_T cbIme = GlobalSize(GlobalHandle(pIme));
    if (pIme->dchDest >= cbIme)
        return 0;

    /* Get conversion list size */
    HKL hKL = GetKeyboardLayout(0);
    const CHAR *pszSource = (const CHAR *)pIme + pIme->dchSource;
    const UINT uBufLen = ImmGetConversionListA(hKL, hIMC, pszSource, NULL, 0, GCL_CONVERSION);
    if (!uBufLen)
        return 0;

    /* Allocate */
    HGLOBAL hCandList = GlobalAlloc(GHND, uBufLen);
    if (!hCandList)
        return 0;

    /* Lock */
    PCANDIDATELIST pCL = (PCANDIDATELIST)GlobalLock(hCandList);
    if (!pCL)
    {
        GlobalFree(hCandList);
        return 0;
    }

    /* Get the conversion list */
    UINT ret = ImmGetConversionListA(hKL, hIMC, pszSource, pCL, uBufLen, GCL_CONVERSION);

    /* Store the conversion list into pIme */
    UINT wCount = 0;
    PCHAR pszDest = (PCHAR)pIme + pIme->dchDest;
    SIZE_T cbDestRemaining = cbIme - pIme->dchDest;
    for (DWORD i = 0; i < pCL->dwCount && (cbDestRemaining > 2); ++i)
    {
        const CHAR *pCandidate = (const CHAR *)pCL + pCL->dwOffset[i];
        *pszDest++ = pCandidate[0];
        *pszDest++ = pCandidate[1];

        cbDestRemaining -= 2;
        wCount += 2;
    }
    pIme->wCount = wCount;

    /* Add terminator safely */
    if (cbDestRemaining > 0)
        *pszDest = ANSI_NULL;

    /* Clean up */
    GlobalUnlock(hCandList);
    GlobalFree(hCandList);
    return ret;
}

static LRESULT Imm32TransHanjaMode(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme)
{
    DWORD dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (!dwThreadId)
        return 0;

    HKL hKL = GetKeyboardLayout(dwThreadId);
    PIMEDPI pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi))
    {
        BYTE offset = HIBYTE(HIWORD(pIme->wParam));
        SIZE_T cbIme = sizeof(*pIme) + (SIZE_T)pIme->wCount;
        PCHAR pMbcsCh;
        WORD MbcsCh;

        /* SECURITY: Check boundary */
        if ((SIZE_T)offset > cbIme || cbIme - (SIZE_T)offset < sizeof(MbcsCh))
        {
            ImmUnlockImeDpi(pImeDpi);
            return 0;
        }

        pMbcsCh = (PCHAR)pIme + offset;
        RtlCopyMemory(&MbcsCh, pMbcsCh, sizeof(MbcsCh));
        pIme->wParam = MAKELONG(MbcsCh, HIWORD(pIme->wParam));

        WCHAR wch = UNICODE_NULL;
        if (!MultiByteToWideChar(CP_ACP, 0, pMbcsCh, 2, &wch, 1))
        {
            ImmUnlockImeDpi(pImeDpi);
            return 0;
        }

        RtlCopyMemory(pMbcsCh, &wch, sizeof(wch));
    }

    ImmUnlockImeDpi(pImeDpi);

    hKL = GetKeyboardLayout(0);
    LRESULT ret = ImmEscapeW(hKL, hIMC, IME_ESC_HANJA_MODE, pIme);
    if (ret)
        SendMessageW(hWnd, WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1);

    return ret;
}

static DWORD Imm32TransGetLevel(HWND hWnd)
{
    DWORD result = NtUserGetAppImeLevel(hWnd);
    return result ? result : IME_RS_ERROR;
}

static BOOL Imm32TransSetLevel(HWND hWnd, PIMESTRUCT pIme)
{
    WPARAM wParam = pIme->wParam;
    return wParam && wParam <= 5 && NtUserSetAppImeLevel(hWnd, (DWORD)wParam);
}

static BOOL Imm32TransGetMNTable(HIMC hIMC, PIMESTRUCT pIme)
{
#define INPUT_METHOD_BASE 100
    static const WORD g_MNTable[3][96] =
    {
        /* InputMethod = 100 (Wansung standard) */
        {
            0xA1A1, 0xA3A1, 0xA1A8, 0xA3A3, 0xA3A4, 0xA3A5, 0xA3A6, 0xA1AE,
            0xA3A8, 0xA3A9, 0xA3AA, 0xA3AB, 0xA3A7, 0xA3AD, 0xA3AE, 0xA3AF,
            0xA3B0, 0xA3B1, 0xA3B2, 0xA3B3, 0xA3B4, 0xA3B5, 0xA3B6, 0xA3B7,
            0xA3B8, 0xA3B9, 0xA3BA, 0xA3BB, 0xA3BC, 0xA3BD, 0xA3BE, 0xA3BF,
            0xA3C0, 0xA4B1, 0xA4D0, 0xA4BA, 0xA4B7, 0xA4A8, 0xA4A9, 0xA4BE,
            0xA4C7, 0xA4C1, 0xA4C3, 0xA4BF, 0xA4D3, 0xA4D1, 0xA4CC, 0xA4C2,
            0xA4C6, 0xA4B3, 0xA4A2, 0xA4A4, 0xA4B6, 0xA4C5, 0xA4BD, 0xA4B9,
            0xA4BC, 0xA4CB, 0xA4BB, 0xA3DB, 0xA1AC, 0xA3DD, 0xA3DE, 0xA3DF,
            0xA1A2, 0xA4B1, 0xA4D0, 0xA4BA, 0xA4B7, 0xA4A7, 0xA4A9, 0xA4BE,
            0xA4C7, 0xA4C1, 0xA4C3, 0xA4BF, 0xA4D3, 0xA4D1, 0xA4CC, 0xA4C0,
            0xA4C4, 0xA4B2, 0xA4A1, 0xA4A4, 0xA4B5, 0xA4C5, 0xA4BD, 0xA4B8,
            0xA4BC, 0xA4CB, 0xA4BB, 0xA3FB, 0xA3FC, 0xA3FD, 0xA1AD, 0x0000
        },
        /* InputMethod = 101 (Old Johab compatible) */
        {
            0xA1A1, 0xA4B8, 0xA1A8, 0xA3A3, 0xA3A4, 0xA3A5, 0xA3A6, 0xA1AE,
            0xA3A8, 0xA3A9, 0xA3AA, 0xA3AB, 0xA4BC, 0xA3AD, 0xA3AE, 0xA4C7,
            0xA4BB, 0xA4BE, 0xA4B6, 0xA4B2, 0xA4CB, 0xA4D0, 0xA4C1, 0xA4C6,
            0xA4D2, 0xA4CC, 0xA3BA, 0xA4B2, 0xA3B2, 0xA3BD, 0xA3B3, 0xA3BF,
            0xA3C0, 0xA4A7, 0xA3A1, 0xA4AB, 0xA4AA, 0xA4BB, 0xA4A2, 0xA3AF,
            0xA1AF, 0xA3B8, 0xA3B4, 0xA3B5, 0xA3B6, 0xA3B1, 0xA3B0, 0xA3B9,
            0xA3BE, 0xA4BD, 0xA4C2, 0xA4A6, 0xA4C3, 0xA3B7, 0xA4B0, 0xA4BC,
            0xA4B4, 0xA3BC, 0xA4BA, 0xA3DB, 0xA3DC, 0xA3DD, 0xA3DE, 0xA3DF,
            0xA1AE, 0xA4B7, 0xA4CC, 0xA4C4, 0xA4D3, 0xA4C5, 0xA4BF, 0xA4D1,
            0xA4A4, 0xA4B1, 0xA4B7, 0xA4A1, 0xA4B8, 0xA4BE, 0xA4B5, 0xA4BA,
            0xA4BD, 0xA4B5, 0xA4C0, 0xA4A4, 0xA4C3, 0xA4A7, 0xA4C7, 0xA4A9,
            0xA4A1, 0xA4A9, 0xA4B1, 0xA3FB, 0xA3FC, 0xA3FD, 0xA1AD, 0x0000
        },
        /* InputMethod = 102 (Extended array) */
        {
            0xA1A1, 0xA4A2, 0xA3AE, 0xA4B8, 0xA4AF, 0xA4AE, 0xA1B0, 0xA3AA,
            0xA1A2, 0xA1AD, 0xA1B1, 0xA3AB, 0xA4BC, 0xA3A9, 0xA3AE, 0xA4C7,
            0xA4BB, 0xA4BE, 0xA4B6, 0xA4B2, 0xA4CB, 0xA4D0, 0xA4C1, 0xA4C6,
            0xA4D2, 0xA4CC, 0xA3B4, 0xA4B2, 0xA3A7, 0xA1B5, 0xA3AE, 0xA3A1,
            0xA4AA, 0xA4A7, 0xA3BF, 0xA4BC, 0xA4AC, 0xA4A5, 0xA4AB, 0xA4C2,
            0xA3B0, 0xA3B7, 0xA3B1, 0xA3B2, 0xA3B3, 0xA1A8, 0xA3AD, 0xA3B8,
            0xA3B9, 0xA4BD, 0xA4B0, 0xA4A6, 0xA4AD, 0xA3B6, 0xA4A3, 0xA4BC,
            0xA4B4, 0xA3B5, 0xA4BA, 0xA3A8, 0xA3BA, 0xA1B4, 0xA3BD, 0xA3BB,
            0xA3AA, 0xA4B7, 0xA4CC, 0xA4C4, 0xA4D3, 0xA4C5, 0xA4BF, 0xA4D1,
            0xA4A4, 0xA4B1, 0xA4B7, 0xA4A1, 0xA4B8, 0xA4BE, 0xA4B5, 0xA4BA,
            0xA4BD, 0xA4B5, 0xA4C0, 0xA4A4, 0xA4C3, 0xA4A7, 0xA4C7, 0xA4A9,
            0xA4A1, 0xA4B1, 0xA4B1, 0xA3A5, 0xA3CC, 0xA3AF, 0xA1AD, 0x0000
        }
    };

    UINT nInputMethod = GetProfileIntW(L"WANSUNG", L"InputMethod", INPUT_METHOD_BASE);
    UINT index = nInputMethod - INPUT_METHOD_BASE;
    if (index >= _countof(g_MNTable))
        index = 0;

    PWORD pDst = (PWORD)pIme->lParam1;
    RtlCopyMemory(pDst, g_MNTable[index], sizeof(g_MNTable[0]));
    return TRUE;
}

static LRESULT Imm32TransMoveImeWindow(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme)
{
    if (pIme->wParam == 2)
    {
        POINT pt = { (SHORT)LOWORD(pIme->lParam1), (SHORT)HIWORD(pIme->lParam1) };
        ClientToScreen(hWnd, &pt);
        pIme->lParam1 = MAKELONG(pt.x, pt.y);
    }
    HKL hKL = GetKeyboardLayout(0);
    return ImmEscapeW(hKL, hIMC, IME_SETCONVERSIONWINDOW, pIme);
}

static BOOL Imm32SetCompFont(HWND hWnd, HIMC hIMC, PLOGFONTW plfW)
{
    PINPUTCONTEXT pIMC = ImmLockIMC(hIMC);
    if (!pIMC)
        return FALSE;

    DWORD dwCompatFlags = ImmGetAppCompatFlags(hIMC);
    PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    BOOL bIsUnicode = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    const LOGFONTW* pCurrentFontW;
    LOGFONTW currentLF;
    if (bIsUnicode)
    {
        pCurrentFontW = &pIMC->lfFont.W;
    }
    else
    {
        LogFontAnsiToWide(&pIMC->lfFont.A, &currentLF);
        pCurrentFontW = &currentLF;
    }

    BOOL ret;
    if (memcmp(pCurrentFontW, plfW, offsetof(LOGFONTW, lfFaceName)) != 0 ||
        lstrcmpW(pCurrentFontW->lfFaceName, plfW->lfFaceName) != 0)
    {
        if (dwCompatFlags & _IME_APP_COMPAT_DIRECT_IME_SYSTEM)
        {
            pIMC->lfFont.W = *plfW;

            if (dwCompatFlags & _IME_APP_COMPAT_DIRECT_IME_FONT)
                ret = Imm32PostImsMessage(hWnd, IMC_SETCOMPOSITIONFONT, 0);
            else
                ret = PostMessageW(hWnd, WM_IME_SYSTEM, IMC_SETCOMPOSITIONFONT, 0);
        }
        else
        {
            ret = ImmSetCompositionFontW(hIMC, plfW);
        }
    }
    else
    {
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

static BOOL Imm32FixLogFont(PLOGFONTW plfW, BOOL bVertical)
{
    if (!plfW)
        return FALSE;

    /* SECURITY: Avoid buffer overrun */
    plfW->lfFaceName[_countof(plfW->lfFaceName) - 1] = UNICODE_NULL;

    if (bVertical)
    {
        plfW->lfEscapement = plfW->lfOrientation = 2700;

        if (plfW->lfCharSet == SHIFTJIS_CHARSET && plfW->lfFaceName[0] != L'@')
        {
            size_t len = wcslen(plfW->lfFaceName);
            if (len >= (_countof(plfW->lfFaceName) - 1))
                return FALSE;

            RtlMoveMemory(&plfW->lfFaceName[1], &plfW->lfFaceName[0], (len + 1) * sizeof(WCHAR));
            plfW->lfFaceName[0] = L'@';
        }
    }
    else
    {
        plfW->lfEscapement = plfW->lfOrientation = 0;

        if (plfW->lfCharSet == SHIFTJIS_CHARSET && plfW->lfFaceName[0] == L'@')
        {
            size_t len = wcslen(plfW->lfFaceName);
            RtlMoveMemory(&plfW->lfFaceName[0], &plfW->lfFaceName[1], len * sizeof(WCHAR));
        }
    }

    return TRUE;
}

static BOOL
Imm32SetFontForVertical(HWND hWnd, HIMC hIMC, PINPUTCONTEXTDX pIC, BOOL bVertical)
{
    LOGFONTW lfW;
    if (pIC->fdwInit & INIT_LOGFONT)
    {
        PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
        if (!pClientImc)
            return FALSE;
        BOOL bWide = !!(pClientImc->dwFlags & CLIENTIMC_WIDE);
        ImmUnlockClientImc(pClientImc);

        if (bWide)
            lfW = pIC->lfFont.W;
        else
            LogFontAnsiToWide(&pIC->lfFont.A, &lfW);
    }
    else
    {
        GetObjectW(GetStockObject(SYSTEM_FONT), sizeof(lfW), &lfW);
    }

    if (!Imm32FixLogFont(&lfW, bVertical))
        return FALSE;

    return Imm32SetCompFont(hWnd, hIMC, &lfW);
}

static BOOL Imm32TransSetConversionWindow(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme)
{
    if (!Imm32IsForegroundThread(NULL) && !GetFocus())
        return TRUE; /* Ignore as success */

    PINPUTCONTEXTDX pIC = (PINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    COMPOSITIONFORM cfComp;
    cfComp.dwStyle = 0;

    POINT pt = { (SHORT)LOWORD(pIme->lParam1), (SHORT)HIWORD(pIme->lParam1) };
    RECT rcArea = { (SHORT)LOWORD(pIme->lParam2), (SHORT)HIWORD(pIme->lParam2),
                    (SHORT)LOWORD(pIme->lParam3), (SHORT)HIWORD(pIme->lParam3) };

    WPARAM wParam = pIme->wParam;
    HWND hwndIC = pIC->hWnd;

    if (wParam & MCW_HIDDEN)
    {
        pIC->dwUIFlags |= _IME_UI_HIDDEN;
        ScreenToClient(hwndIC, &pt);
        MapWindowPoints(NULL, hwndIC, (PPOINT)&rcArea, 2);
    }
    else
    {
        pIC->dwUIFlags &= ~_IME_UI_HIDDEN;
    }

    if (wParam & MCW_WINDOW)
    {
        BOOL bUseCrossWindowLogic = TRUE;

        if (LOWORD(hWnd) == LOWORD(hwndIC))
        {
            PVOID pWndEntry = (hWnd && gpsi) ? ValidateHwnd(hWnd) : NULL;
            PVOID pWndICEntry = (hwndIC && gpsi) ? ValidateHwnd(hwndIC) : NULL;

            if (pWndEntry == pWndICEntry)
                bUseCrossWindowLogic = FALSE;
        }

        if (bUseCrossWindowLogic)
        {
            ClientToScreen(hWnd, &pt);
            ScreenToClient(hwndIC, &pt);

            if (wParam & MCW_RECT)
            {
                cfComp.dwStyle = CFS_RECT;
                MapWindowPoints(hWnd, NULL, (PPOINT)&rcArea, 2);
                MapWindowPoints(NULL, hwndIC, (PPOINT)&rcArea, 2);
            }
            else
            {
                cfComp.dwStyle = CFS_POINT;
            }
        }
        else
        {
            if (wParam & MCW_SCREEN)
            {
                ScreenToClient(hwndIC, &pt);
                cfComp.dwStyle = (wParam & MCW_RECT) ? CFS_RECT : CFS_POINT;
                if (wParam & MCW_RECT)
                {
                    MapWindowPoints(NULL, hwndIC, (PPOINT)&rcArea, 2);
                }
            }
            else
            {
                cfComp.dwStyle = (wParam & MCW_RECT) ? CFS_RECT : CFS_POINT;
            }
        }
    }

    if (wParam & MCW_VERTICAL)
    {
        if (!(pIC->dwUIFlags & _IME_UI_VERTICAL))
        {
            pIC->dwUIFlags |= _IME_UI_VERTICAL;
            Imm32SetFontForVertical(hWnd, hIMC, pIC, TRUE);
        }
    }
    else
    {
        if (pIC->dwUIFlags & _IME_UI_VERTICAL)
        {
            pIC->dwUIFlags &= ~_IME_UI_VERTICAL;
            Imm32SetFontForVertical(hWnd, hIMC, pIC, FALSE);
        }
    }

    cfComp.ptCurrentPos = pt;
    cfComp.rcArea = rcArea;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        pIC->cfCompForm.ptCurrentPos = pt;
        pIC->cfCompForm.rcArea = rcArea;

        for (DWORD i = 0; i < MAX_CANDIDATEFORM; i++)
        {
            PCANDIDATEFORM pCF = &pIC->cfCandForm[i];
            if (pCF->dwIndex == (DWORD)-1)
                continue;

            CANDIDATEFORM cf;
            cf.dwIndex      = i;
            cf.dwStyle      = CFS_EXCLUDE;
            cf.ptCurrentPos = pt;
            cf.rcArea       = rcArea;
            Imm32SetCandidateWindow(hWnd, hIMC, &cf);
        }
    }
    else
    {
        Imm32SetCompWindow(hWnd, hIMC, &cfComp);
    }

    ImmUnlockIMC(hIMC);
    return TRUE;
}

static DWORD Imm32TransGetConversionMode(HIMC hIMC)
{
    DWORD fdwConversion = 0, fdwSentence;
    ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);
    return Imm32Get31ModeFrom40ModeJ(fdwConversion);
}

static BOOL Imm32TransVKDBEMode(HIMC hIMC, UINT vKey)
{
    DWORD fdwConversion, fdwSentence;
    BOOL ret = ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);
    if (!ret)
        return FALSE;

    switch (vKey)
    {
        case VK_DBE_ALPHANUMERIC:
            fdwConversion &= ~IME_CMODE_LANGUAGE;
            break;
        case VK_DBE_KATAKANA:
            fdwConversion |= IME_CMODE_LANGUAGE;
            break;
        case VK_DBE_HIRAGANA:
            fdwConversion = (fdwConversion & ~IME_CMODE_LANGUAGE) | IME_CMODE_NATIVE;
            break;
        case VK_DBE_SBCSCHAR:
            fdwConversion &= ~IME_CMODE_FULLSHAPE;
            break;
        case VK_DBE_DBCSCHAR:
            fdwConversion |= IME_CMODE_FULLSHAPE;
            break;
        case VK_DBE_ROMAN:
            fdwConversion |= IME_CMODE_ROMAN;
            break;
        case VK_DBE_NOROMAN:
            fdwConversion &= ~IME_CMODE_ROMAN;
            break;
        case VK_DBE_CODEINPUT:
            fdwConversion |= IME_CMODE_CHARCODE;
            break;
        case VK_DBE_NOCODEINPUT:
            fdwConversion &= ~IME_CMODE_CHARCODE;
            break;
        default:
            break;
    }

    return ImmSetConversionStatus(hIMC, fdwConversion, fdwSentence);
}

static BOOL Imm32TransSendVKey(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme, BOOL bAnsi)
{
    WPARAM wParam = pIme->wParam;

    if (wParam >= VK_DBE_DETERMINESTRING)
    {
        if (wParam == VK_DBE_DETERMINESTRING)
        {
            HIMC hContext = ImmGetContext(hWnd);
            BOOL ret = (BOOL)ImmNotifyIME(hContext, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            ImmReleaseContext(hWnd, hContext);
            return ret;
        }

        if (wParam > VK_DBE_ENTERDLGCONVERSIONMODE && (wParam == 0xFFFF || (LONG)wParam == -1))
        {
            UINT wCount = pIme->wCount;
            if (wCount == 28 || (240 <= wCount && wCount <= 253))
                return TRUE;
        }

        return FALSE;
    }

    if (wParam >= VK_DBE_CODEINPUT)
        return Imm32TransVKDBEMode(hIMC, (UINT)wParam);

    if (wParam == VK_CONVERT)
        return ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CONVERT, 0);

    if (wParam < VK_DBE_ALPHANUMERIC)
        return FALSE;

    if (wParam <= VK_DBE_NOROMAN)
        return Imm32TransVKDBEMode(hIMC, (UINT)wParam);

    HKL hKL;
    UINT nImeConfig;
    if (wParam == VK_DBE_ENTERWORDREGISTERMODE)
    {
        hKL = GetKeyboardLayout(0);
        nImeConfig = IME_CONFIG_REGISTERWORD;
    }
    else if (wParam == VK_DBE_ENTERIMECONFIGMODE)
    {
        hKL = GetKeyboardLayout(0);
        nImeConfig = IME_CONFIG_GENERAL;
    }
    else
    {
        return ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    }

    if (bAnsi)
        return ImmConfigureIMEA(hKL, hWnd, nImeConfig, NULL);
    return ImmConfigureIMEW(hKL, hWnd, nImeConfig, NULL);
}

static DWORD Imm32TransSetConversionMode(HIMC hIMC, PIMESTRUCT pIme)
{
    DWORD fdwOldConversion = 0, fdwSentence;
    if (!ImmGetConversionStatus(hIMC, &fdwOldConversion, &fdwSentence))
        return 0;

    const DWORD mode31 = Imm32Get31ModeFrom40ModeJ(fdwOldConversion);
    const WPARAM wParam = pIme->wParam;
    DWORD dwNewValues = 0, dwMask = 0;

    const DWORD langFlags = (IME_MODE_ALPHANUMERIC | IME_MODE_KATAKANA | IME_MODE_HIRAGANA);
    if (wParam & langFlags)
    {
        dwMask |= IME_CMODE_LANGUAGE;
        if (wParam & IME_MODE_KATAKANA)
            dwNewValues |= IME_CMODE_LANGUAGE;
        else if (wParam & IME_MODE_HIRAGANA)
            dwNewValues |= IME_CMODE_NATIVE;
        else
            dwNewValues |= IME_CMODE_ALPHANUMERIC;
    }

    if (wParam & (IME_MODE_DBCSCHAR | _IME_MODE_JPN_SBCSCHAR))
    {
        dwMask |= IME_CMODE_FULLSHAPE;
        if (!(wParam & _IME_MODE_JPN_SBCSCHAR))
            dwNewValues |= IME_CMODE_FULLSHAPE;
    }

    if (wParam & (IME_MODE_ROMAN | IME_MODE_NOROMAN))
    {
        dwMask |= IME_CMODE_ROMAN;
        if (wParam & IME_MODE_ROMAN)
            dwNewValues |= IME_CMODE_ROMAN;
    }

    if (wParam & (IME_MODE_CODEINPUT | IME_MODE_NOCODEINPUT))
    {
        dwMask |= IME_CMODE_CHARCODE;
        if (wParam & IME_MODE_CODEINPUT)
            dwNewValues |= IME_CMODE_CHARCODE;
    }

    DWORD fdwNewConversion = (fdwOldConversion & ~dwMask) | (dwNewValues & dwMask);

    if (!ImmSetConversionStatus(hIMC, fdwNewConversion, fdwSentence))
        return 0;

    return mode31;
}

static BOOL Imm32TransEnterWordRegisterMode(HWND hWnd, PIMESTRUCT pIme, BOOL bAnsi)
{
    HKL hKL = GetKeyboardLayout(0);
    if (!ImmIsIME(hKL))
        return FALSE;

    HGLOBAL hReading = (HGLOBAL)pIme->lParam1, hWord = (HGLOBAL)pIme->lParam2;
    REGISTERWORDW rwInfo = { NULL };
    if (hReading)
        rwInfo.lpReading = GlobalLock(hReading);
    if (hWord)
        rwInfo.lpWord = GlobalLock(hWord);

    BOOL ret;
    if (bAnsi)
        ret = ImmConfigureIMEA(hKL, hWnd, IME_CONFIG_REGISTERWORD, (PREGISTERWORDA)&rwInfo);
    else
        ret = ImmConfigureIMEW(hKL, hWnd, IME_CONFIG_REGISTERWORD, &rwInfo);

    if (rwInfo.lpReading)
        GlobalUnlock(hReading);
    if (rwInfo.lpWord)
        GlobalUnlock(hWord);

    return ret;
}

static BOOL Imm32TransSetConversionFontEx(HWND hWnd, HIMC hIMC, PIMESTRUCT pIme, BOOL bAnsi)
{
    PINPUTCONTEXTDX pIC = (PINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    PVOID pLogFont = GlobalLock((HGLOBAL)pIme->lParam1);
    if (!pLogFont)
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    LOGFONTW lfW;
    if (bAnsi)
        LogFontAnsiToWide(pLogFont, &lfW);
    else
        RtlCopyMemory(&lfW, pLogFont, sizeof(LOGFONTW));

    GlobalUnlock((HGLOBAL)pIme->lParam1);

    BOOL bFixed = Imm32FixLogFont(&lfW, !!(pIC->dwUIFlags & _IME_UI_VERTICAL));
    ImmUnlockIMC(hIMC);

    if (!bFixed)
        return FALSE;
    return Imm32SetCompFont(hWnd, hIMC, &lfW);
}

static LRESULT Imm32TranslateIMESubFunctions(HWND hWnd, PIMESTRUCT pIme, BOOL bAnsi)
{
    HIMC hImc = ImmGetSaveContext(hWnd, 1);
    if (!hImc)
        return 0;

    const WORD wLang = PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID()));
    const BOOL bKorean = (wLang == LANG_KOREAN), bJapanese = (wLang == LANG_JAPANESE);
    const UINT fnc = pIme->fnc;
    switch (fnc)
    {
        case 3:
            return 1;
        case 4:
            if (bKorean)
                return Imm32TransSetOpenK(hWnd, hImc, pIme);
            return Imm32TransSetOpenJ(hWnd, hImc, pIme);
        case 5:
            if (bKorean)
                return Imm32TransGetOpenK(hWnd, hImc, pIme, bAnsi);
            return Imm32TransGetOpenJ(hWnd, hImc, pIme, bAnsi);
        case 6:
            return 0;
        case 7:
            return 0xA03;
        case 8:
            if (bKorean)
                return Imm32TransMoveImeWindow(hWnd, hImc, pIme);
            return Imm32TransSetConversionWindow(hWnd, hImc, pIme);
        case 16:
            if (bJapanese)
                return Imm32TransSetConversionMode(hImc, pIme);
            break;
        case 17:
            if (bKorean)
                return Imm32TransGetMode(hImc);
            if (bJapanese)
                return Imm32TransGetConversionMode(hImc);
            break;
        case 18:
            if (bKorean)
                return Imm32TransSetMode(hImc, pIme);
            break;
        case 19:
            if (bJapanese)
                return Imm32TransSendVKey(hWnd, hImc, pIme, bAnsi);
            break;
        case 24:
            if (bJapanese)
                return Imm32TransEnterWordRegisterMode(hWnd, pIme, bAnsi);
            break;
        case 25:
            if (bJapanese)
                return Imm32TransSetConversionFontEx(hWnd, hImc, pIme, bAnsi);
            break;
        case 32:
            if (bKorean)
                return Imm32TransCodeConvert(hImc, pIme) ? pIme->wParam : 0;
            break;
        case 34:
            if (bKorean)
                return Imm32TransConvertList(hImc, pIme);
            break;
        case 48:
            if (bKorean)
                return ImmEscapeW(GetKeyboardLayout(0), hImc, 0x30, pIme);
            break;
        case 49:
            if (bKorean)
                return Imm32TransHanjaMode(hWnd, hImc, pIme);
            break;
        case 64:
            if (bKorean)
                return Imm32TransGetLevel(hWnd);
            break;
        case 65:
            if (bKorean)
                return Imm32TransSetLevel(hWnd, pIme);
            break;
        case 66:
            if (bKorean)
                return Imm32TransGetMNTable(hImc, pIme);
            break;
        default:
            ERR("Unknown fnc %d\n", fnc);
            break;
    }

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
