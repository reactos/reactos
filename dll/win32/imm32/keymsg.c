/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 keys and messages
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

BOOL APIENTRY Imm32ImeNonImeToggle(HIMC hIMC, HKL hKL, HWND hWnd, LANGID LangID)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;

    if (hWnd != NULL)
        return FALSE;

    if (!IS_IME_HKL(hKL) || LOWORD(hKL) != LangID)
    {
        FIXME("We have to do something here\n");
        return TRUE;
    }

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    ImmUnlockIMC(hIMC);

    if (!fOpen)
    {
        ImmSetOpenStatus(hIMC, TRUE);
        return TRUE;
    }

    FIXME("We have to do something here\n");
    return TRUE;
}

BOOL APIENTRY Imm32CShapeToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !IS_IME_HKL(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    if (fOpen)
    {
        dwConversion = (pIC->fdwConversion ^ IME_CMODE_FULLSHAPE);
        dwSentence = pIC->fdwSentence;
    }

    ImmUnlockIMC(hIMC);

    if (fOpen)
        ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    else
        ImmSetOpenStatus(hIMC, TRUE);

    return TRUE;
}

BOOL APIENTRY Imm32CSymbolToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !IS_IME_HKL(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return TRUE;

    fOpen = pIC->fOpen;
    if (fOpen)
    {
        dwConversion = (pIC->fdwConversion ^ IME_CMODE_SYMBOL);
        dwSentence = pIC->fdwSentence;
    }

    ImmUnlockIMC(hIMC);

    if (fOpen)
        ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    else
        ImmSetOpenStatus(hIMC, TRUE);

    return TRUE;
}

BOOL APIENTRY Imm32JCloseOpen(HIMC hIMC, HKL hKL, HWND hWnd)
{
    BOOL fOpen;

    if (ImmIsIME(hKL) && LOWORD(hKL) == LANGID_JAPANESE)
    {
        fOpen = ImmGetOpenStatus(hIMC);
        ImmSetOpenStatus(hIMC, !fOpen);
        return TRUE;
    }

    FIXME("We have to do something here\n");
    return TRUE;
}

BOOL APIENTRY Imm32KShapeToggle(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_FULLSHAPE);
    dwSentence = pIC->fdwSentence;
    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);

    if (pIC->fdwConversion & (IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE))
        ImmSetOpenStatus(hIMC, TRUE);
    else
        ImmSetOpenStatus(hIMC, FALSE);

    ImmUnlockIMC(hIMC);
    return TRUE;
}

BOOL APIENTRY Imm32KHanjaConvert(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_HANJACONVERT);
    dwSentence = pIC->fdwSentence;
    ImmUnlockIMC(hIMC);

    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    return TRUE;
}

BOOL APIENTRY Imm32KEnglish(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;
    BOOL fOpen;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_NATIVE);
    dwSentence = pIC->fdwSentence;
    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);

    fOpen = ((pIC->fdwConversion & (IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE)) != 0);
    ImmSetOpenStatus(hIMC, fOpen);

    ImmUnlockIMC(hIMC);
    return TRUE;
}

BOOL APIENTRY Imm32ProcessHotKey(HWND hWnd, HIMC hIMC, HKL hKL, DWORD dwHotKeyID)
{
    PIMEDPI pImeDpi;
    BOOL ret;

    if (hIMC && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    switch (dwHotKeyID)
    {
        case IME_CHOTKEY_IME_NONIME_TOGGLE:
            return Imm32ImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_SIMPLIFIED);

        case IME_CHOTKEY_SHAPE_TOGGLE:
            return Imm32CShapeToggle(hIMC, hKL, hWnd);

        case IME_CHOTKEY_SYMBOL_TOGGLE:
            return Imm32CSymbolToggle(hIMC, hKL, hWnd);

        case IME_JHOTKEY_CLOSE_OPEN:
            return Imm32JCloseOpen(hIMC, hKL, hWnd);

        case IME_KHOTKEY_SHAPE_TOGGLE:
            return Imm32KShapeToggle(hIMC);

        case IME_KHOTKEY_HANJACONVERT:
            return Imm32KHanjaConvert(hIMC);

        case IME_KHOTKEY_ENGLISH:
            return Imm32KEnglish(hIMC);

        case IME_THOTKEY_IME_NONIME_TOGGLE:
            return Imm32ImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_TRADITIONAL);

        case IME_THOTKEY_SHAPE_TOGGLE:
            return Imm32CShapeToggle(hIMC, hKL, hWnd);

        case IME_THOTKEY_SYMBOL_TOGGLE:
            return Imm32CSymbolToggle(hIMC, hKL, hWnd);

        default:
            break;
    }

    if (dwHotKeyID < IME_HOTKEY_PRIVATE_FIRST || IME_HOTKEY_PRIVATE_LAST < dwHotKeyID)
        return FALSE;

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    ret = (BOOL)pImeDpi->ImeEscape(hIMC, IME_ESC_PRIVATE_HOTKEY, &dwHotKeyID);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

static BOOL APIENTRY
ImmIsUIMessageAW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bAnsi)
{
    switch (msg)
    {
        case WM_IME_STARTCOMPOSITION: case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION: case WM_IME_SETCONTEXT: case WM_IME_NOTIFY:
        case WM_IME_COMPOSITIONFULL: case WM_IME_SELECT: case WM_IME_SYSTEM:
            break;
        default:
            return FALSE;
    }

    if (!hWndIME)
        return TRUE;

    if (bAnsi)
        SendMessageA(hWndIME, msg, wParam, lParam);
    else
        SendMessageW(hWndIME, msg, wParam, lParam);

    return TRUE;
}

typedef struct IMM_UNKNOWN_PROCESS1
{
    HWND hWnd;
    BOOL fFlag;
} IMM_UNKNOWN_PROCESS1, *PIMM_UNKNOWN_PROCESS1;

static DWORD WINAPI Imm32UnknownProcess1Proc(LPVOID arg)
{
    HWND hwndDefIME;
    UINT uValue;
    DWORD_PTR lResult;
    PIMM_UNKNOWN_PROCESS1 pUnknown = arg;

    Sleep(3000);
    hwndDefIME = ImmGetDefaultIMEWnd(pUnknown->hWnd);
    if (hwndDefIME)
    {
        uValue = (pUnknown->fFlag ? 0x23 : 0x24);
        SendMessageTimeoutW(hwndDefIME, WM_IME_SYSTEM, uValue, (LPARAM)pUnknown->hWnd,
                            SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &lResult);
    }
    Imm32HeapFree(pUnknown);
    return FALSE;
}

LRESULT APIENTRY Imm32UnknownProcess1(HWND hWnd, BOOL fFlag)
{
    HANDLE hThread;
    PWND pWnd = NULL;
    PIMM_UNKNOWN_PROCESS1 pUnknown1;
    DWORD_PTR lResult = 0;

    if (hWnd && g_psi)
        pWnd = ValidateHwndNoErr(hWnd);

    if (!pWnd)
        return 0;

    if (pWnd->state2 & WNDS2_WMCREATEMSGPROCESSED)
    {
        SendMessageTimeoutW(hWnd, 0x505, 0, fFlag, 3, 5000, &lResult);
        return lResult;
    }

    pUnknown1 = Imm32HeapAlloc(0, sizeof(IMM_UNKNOWN_PROCESS1));
    if (!pUnknown1)
        return 0;

    pUnknown1->hWnd = hWnd;
    pUnknown1->fFlag = fFlag;

    hThread = CreateThread(NULL, 0, Imm32UnknownProcess1Proc, pUnknown1, 0, NULL);
    if (hThread)
        CloseHandle(hThread);
    return 0;
}

static BOOL CALLBACK Imm32SendChangeProc(HIMC hIMC, LPARAM lParam)
{
    HWND hWnd;
    LPINPUTCONTEXTDX pIC;

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return TRUE;

    hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        goto Quit;

    if (pIC->dwChange & INPUTCONTEXTDX_CHANGE_OPEN)
        SendMessageW(hWnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0);
    if (pIC->dwChange & INPUTCONTEXTDX_CHANGE_CONVERSION)
        SendMessageW(hWnd, WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0);
    if (pIC->dwChange & (INPUTCONTEXTDX_CHANGE_OPEN | INPUTCONTEXTDX_CHANGE_CONVERSION))
        NtUserNotifyIMEStatus(hWnd, pIC->fOpen, pIC->fdwConversion);
    if (pIC->dwChange & INPUTCONTEXTDX_CHANGE_SENTENCE)
        SendMessageW(hWnd, WM_IME_NOTIFY, IMN_SETSENTENCEMODE, 0);
Quit:
    pIC->dwChange = 0;
    ImmUnlockIMC(hIMC); // ??? Windows doesn't unlock here
    return TRUE;
}

BOOL APIENTRY Imm32SendChange(BOOL bProcess)
{
    return ImmEnumInputContext((bProcess ? -1 : 0), Imm32SendChangeProc, 0);
}

VOID APIENTRY Imm32RequestError(DWORD dwError)
{
    FIXME("()\n");
    SetLastError(dwError);
}

DWORD APIENTRY Imm32ReconvertSize(DWORD dwSize, BOOL bAnsi, BOOL bConvert)
{
    DWORD dwOffset;
    if (dwSize < sizeof(RECONVERTSTRING))
        return 0;
    if (!bConvert)
        return dwSize;
    dwOffset = dwSize - sizeof(RECONVERTSTRING);
    if (bAnsi)
        dwOffset /= sizeof(WCHAR);
    else
        dwOffset *= sizeof(WCHAR);
    return sizeof(RECONVERTSTRING) + dwOffset;
}

DWORD APIENTRY
Imm32ConvertReconvert(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, BOOL bAnsi,
                      UINT uCodePage)
{
    DWORD ret = sizeof(RECONVERTSTRING), cch0, cch1, cchDest;

    if ((pSrc->dwVersion != 0) || (pDest->dwVersion != 0))
        return 0;

    pDest->dwStrOffset = sizeof(RECONVERTSTRING);

    /*
     * See RECONVERTSTRING structure:
     * https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/RECONVERTSTRING.html
     *
     * The dwCompStrOffset and dwTargetOffset members are the relative position of dwStrOffset.
     * dwStrLen, dwCompStrLen, and dwTargetStrLen are the TCHAR count. dwStrOffset,
     * dwCompStrOffset, and dwTargetStrOffset are the byte offset.
     */
    if (bAnsi) /* Ansi <-- Wide */
    {
        LPCWSTR pchSrc = (LPCWSTR)((LPCSTR)pSrc + pSrc->dwStrOffset);
        LPSTR pchDest = (LPSTR)pDest + pDest->dwStrOffset;

        cchDest = WideCharToMultiByte(uCodePage, 0, pchSrc, pSrc->dwStrLen,
                                      pchDest, pSrc->dwStrLen, NULL, NULL);

        /* dwCompStrOffset */
        cch1 = pSrc->dwCompStrOffset / sizeof(WCHAR);
        cch0 = IchAnsiFromWide(cch1, pchSrc, uCodePage);
        pDest->dwCompStrOffset = cch0 * sizeof(CHAR);

        /* dwCompStrLen */
        cch0 = IchAnsiFromWide(cch1 + pSrc->dwCompStrLen, pchSrc, uCodePage);
        pDest->dwCompStrLen = cch0 * sizeof(CHAR) - pDest->dwCompStrOffset;

        /* dwTargetStrOffset */
        cch1 = pSrc->dwTargetStrOffset / sizeof(WCHAR);
        cch0 = IchAnsiFromWide(cch1, pchSrc, uCodePage);
        pDest->dwTargetStrOffset = cch0 * sizeof(CHAR);

        /* dwTargetStrLen */
        cch0 = IchAnsiFromWide(cch1 + pSrc->dwTargetStrLen, pchSrc, uCodePage);
        pDest->dwTargetStrLen = cch0 * sizeof(CHAR) - pDest->dwTargetStrOffset;

        /* dwStrLen */
        pDest->dwStrLen = cchDest;
        pchDest[cchDest] = 0;

        ret += (cchDest + 1) * sizeof(CHAR);
    }
    else /* Wide <-- Ansi */
    {
        LPCSTR pchSrc = (LPCSTR)pSrc + pSrc->dwStrOffset;
        LPWSTR pchDest = (LPWSTR)((LPBYTE)pDest + pDest->dwStrOffset);

        cchDest = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pchSrc, pSrc->dwStrLen,
                                      pchDest, pSrc->dwStrLen);

        /* dwCompStrOffset */
        cch0 = IchWideFromAnsi(pSrc->dwCompStrOffset, pchSrc, uCodePage);
        pDest->dwCompStrOffset = cch0 * sizeof(WCHAR);

        /* dwCompStrLen */
        cch0 = IchWideFromAnsi(pSrc->dwCompStrOffset + pSrc->dwCompStrLen, pchSrc, uCodePage);
        pDest->dwCompStrLen = (cch0 * sizeof(WCHAR) - pDest->dwCompStrOffset) / sizeof(WCHAR);

        /* dwTargetStrOffset */
        cch0 = IchWideFromAnsi(pSrc->dwTargetStrOffset, pchSrc, uCodePage);
        pDest->dwTargetStrOffset = cch0 * sizeof(WCHAR);

        /* dwTargetStrLen */
        cch0 = IchWideFromAnsi(pSrc->dwTargetStrOffset + pSrc->dwTargetStrLen, pchSrc, uCodePage);
        pDest->dwTargetStrLen = (cch0 * sizeof(WCHAR) - pSrc->dwTargetStrOffset) / sizeof(WCHAR);

        /* dwStrLen */
        pDest->dwStrLen = cchDest;
        pchDest[cchDest] = 0;

        ret += (cchDest + 1) * sizeof(WCHAR);
    }

    return ret;
}

LRESULT APIENTRY
Imm32ProcessRequest(HIMC hIMC, PWND pWnd, DWORD dwCommand, LPVOID pData, BOOL bAnsiAPI)
{
    HWND hWnd;
    DWORD ret = 0, dwCharPos, cchCompStr;
    LPVOID pCS, pTempData = pData;
    LPRECONVERTSTRING pRS;
    LPIMECHARPOSITION pICP;
    PCLIENTIMC pClientImc;
    UINT uCodePage = CP_ACP;
    BOOL bAnsiWnd = !!(pWnd->state & WNDS_ANSIWINDOWPROC);
    static const size_t acbData[7 * 2] =
    {
        /* UNICODE */
        sizeof(COMPOSITIONFORM), sizeof(CANDIDATEFORM), sizeof(LOGFONTW),
        sizeof(RECONVERTSTRING), sizeof(RECONVERTSTRING),
        sizeof(IMECHARPOSITION), sizeof(RECONVERTSTRING),
        /* ANSI */
        sizeof(COMPOSITIONFORM), sizeof(CANDIDATEFORM), sizeof(LOGFONTA),
        sizeof(RECONVERTSTRING), sizeof(RECONVERTSTRING),
        sizeof(IMECHARPOSITION), sizeof(RECONVERTSTRING),
    };

    if (dwCommand == 0 || dwCommand > IMR_DOCUMENTFEED)
        return 0; /* Out of range */

    if (pData && IsBadWritePtr(pData, acbData[bAnsiAPI * 7 + dwCommand - 1]))
        return 0; /* Invalid pointer */

    /* Sanity check */
    switch (dwCommand)
    {
        case IMR_RECONVERTSTRING: case IMR_DOCUMENTFEED:
            pRS = pData;
            if (pRS && (pRS->dwVersion != 0 || pRS->dwSize < sizeof(RECONVERTSTRING)))
            {
                Imm32RequestError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            break;

        case IMR_CONFIRMRECONVERTSTRING:
            pRS = pData;
            if (!pRS || pRS->dwVersion != 0)
            {
                Imm32RequestError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            break;

        default:
            if (!pData)
            {
                Imm32RequestError(ERROR_INVALID_PARAMETER);
                return 0;
            }
            break;
    }

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc)
    {
        uCodePage = pClientImc->uCodePage;
        ImmUnlockClientImc(pClientImc);
    }

    /* Prepare */
    switch (dwCommand)
    {
        case IMR_COMPOSITIONFONT:
            if (bAnsiAPI == bAnsiWnd)
                goto DoIt;
            if (bAnsiWnd)
                pTempData = Imm32HeapAlloc(0, sizeof(LOGFONTA));
            else
                pTempData = Imm32HeapAlloc(0, sizeof(LOGFONTW));
            if (!pTempData)
                return 0;
            break;

        case IMR_RECONVERTSTRING: case IMR_CONFIRMRECONVERTSTRING: case IMR_DOCUMENTFEED:
            if (bAnsiAPI == bAnsiWnd || !pData)
                goto DoIt;

            pRS = pData;
            ret = Imm32ReconvertSize(pRS->dwSize, FALSE, bAnsiWnd);
            pTempData = Imm32HeapAlloc(0, ret + sizeof(WCHAR));
            if (!pTempData)
                return 0;

            pRS = pTempData;
            pRS->dwSize = ret;
            pRS->dwVersion = 0;

            if (dwCommand == IMR_CONFIRMRECONVERTSTRING)
                Imm32ConvertReconvert(pData, pTempData, bAnsiWnd, uCodePage);
            break;

        case IMR_QUERYCHARPOSITION:
            if (bAnsiAPI == bAnsiWnd)
                goto DoIt;

            pICP = pData;
            dwCharPos = pICP->dwCharPos;

            if (bAnsiAPI)
            {
                cchCompStr = ImmGetCompositionStringA(hIMC, GCS_COMPSTR, NULL, 0);
                if (!cchCompStr)
                    return 0;

                pCS = Imm32HeapAlloc(0, (cchCompStr + 1) * sizeof(CHAR));
                if (!pCS)
                    return 0;

                ImmGetCompositionStringA(hIMC, GCS_COMPSTR, pCS, cchCompStr);
                pICP->dwCharPos = IchWideFromAnsi(pICP->dwCharPos, pCS, uCodePage);
            }
            else
            {
                cchCompStr = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
                if (!cchCompStr)
                    return 0;

                pCS = Imm32HeapAlloc(0, (cchCompStr + 1) * sizeof(WCHAR));
                if (!pCS)
                    return 0;

                ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pCS, cchCompStr);
                pICP->dwCharPos = IchAnsiFromWide(pICP->dwCharPos, pCS, uCodePage);
            }

            Imm32HeapFree(pCS);
            break;

        default:
            break;
    }

DoIt:
    /* The main task */
    hWnd = pWnd->head.h;
    if (bAnsiWnd)
        ret = SendMessageA(hWnd, WM_IME_REQUEST, dwCommand, (LPARAM)pTempData);
    else
        ret = SendMessageW(hWnd, WM_IME_REQUEST, dwCommand, (LPARAM)pTempData);

    if (bAnsiAPI == bAnsiWnd)
        goto Quit;

    /* Get back to caller */
    switch (dwCommand)
    {
        case IMR_COMPOSITIONFONT:
            if (bAnsiAPI)
                LogFontWideToAnsi(pTempData, pData);
            else
                LogFontAnsiToWide(pTempData, pData);
            break;

        case IMR_RECONVERTSTRING: case IMR_DOCUMENTFEED:
            if (!ret)
                goto Quit;

            ret = Imm32ReconvertSize(ret, TRUE, bAnsiWnd);
            if (ret < sizeof(RECONVERTSTRING) ||
                (pTempData && !Imm32ConvertReconvert(pData, pTempData, bAnsiAPI, uCodePage)))
            {
                ret = 0;
            }
            break;

        case IMR_QUERYCHARPOSITION:
            pICP->dwCharPos = dwCharPos;
            break;

        default:
            break;
    }

Quit:
    if (pTempData != pData)
        Imm32HeapFree(pTempData);
    return ret;
}

LRESULT APIENTRY Imm32RequestMessageAW(HIMC hIMC, WPARAM wParam, LPARAM lParam, BOOL bAnsi)
{
    LRESULT ret = 0;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    PWND pWnd = NULL;

    if (!hIMC || Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    hWnd = pIC->hWnd;
    if (hWnd)
        pWnd = ValidateHwndNoErr(hWnd);

    if (pWnd && pWnd->head.pti == NtCurrentTeb()->Win32ThreadInfo)
        ret = Imm32ProcessRequest(hIMC, pWnd, (DWORD)wParam, (LPVOID)lParam, bAnsi);

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, 0x%X, %p, %p)\n", hWndIME, msg, wParam, lParam);
    return ImmIsUIMessageAW(hWndIME, msg, wParam, lParam, TRUE);
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, 0x%X, %p, %p)\n", hWndIME, msg, wParam, lParam);
    return ImmIsUIMessageAW(hWndIME, msg, wParam, lParam, FALSE);
}

/***********************************************************************
 *              ImmGetHotKey(IMM32.@)
 */
BOOL WINAPI
ImmGetHotKey(IN DWORD dwHotKey, OUT LPUINT lpuModifiers, OUT LPUINT lpuVKey,
             OUT LPHKL lphKL)
{
    TRACE("(0x%lX, %p, %p, %p)\n", dwHotKey, lpuModifiers, lpuVKey, lphKL);
    if (lpuModifiers && lpuVKey)
        return NtUserGetImeHotKey(dwHotKey, lpuModifiers, lpuVKey, lphKL);
    return FALSE;
}

/***********************************************************************
 *              ImmWINNLSGetIMEHotkey (IMM32.@)
 */
UINT WINAPI ImmWINNLSGetIMEHotkey(HWND hwndIme)
{
    TRACE("(%p)\n", hwndIme);
    UNREFERENCED_PARAMETER(hwndIme);
    return 0; /* This is correct. This function of Windows just returns zero. */
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
    HIMC hIMC;
    DWORD dwThreadId;
    HKL hKL;
    BOOL ret;

    TRACE("(%p, 0x%lX)\n", hWnd, dwHotKeyID);

    hIMC = ImmGetContext(hWnd);
    dwThreadId = GetWindowThreadProcessId(hWnd, NULL);
    hKL = GetKeyboardLayout(dwThreadId);
    ret = Imm32ProcessHotKey(hWnd, hIMC, hKL, dwHotKeyID);
    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
    HIMC hIMC;
    LPINPUTCONTEXTDX pIC;
    UINT ret = VK_PROCESSKEY;

    TRACE("(%p)\n", hWnd);

    hIMC = ImmGetContext(hWnd);
    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return ret;

    if (pIC->bNeedsTrans)
        ret = pIC->nVKey;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmProcessKey(IMM32.@)
 *       ( Undocumented, called from user32.dll )
 */
DWORD WINAPI
ImmProcessKey(HWND hWnd, HKL hKL, UINT vKey, LPARAM lParam, DWORD dwHotKeyID)
{
    DWORD ret = 0;
    HIMC hIMC;
    PIMEDPI pImeDpi;
    LPINPUTCONTEXTDX pIC;
    BYTE KeyState[256];
    UINT vk;
    BOOL bUseIme = TRUE, bSkipThisKey = FALSE, bLowWordOnly = FALSE;

    TRACE("(%p, %p, 0x%X, %p, 0x%lX)\n", hWnd, hKL, vKey, lParam, dwHotKeyID);

    hIMC = ImmGetContext(hWnd);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi)
    {
        pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        if (pIC)
        {
            if (LOBYTE(vKey) == VK_PACKET &&
                !(pImeDpi->ImeInfo.fdwProperty & IME_PROP_ACCEPT_WIDE_VKEY))
            {
                if (ImeDpi_IsUnicode(pImeDpi))
                {
                    bLowWordOnly = TRUE;
                }
                else
                {
                    bUseIme = FALSE;
                    if (pIC->fOpen)
                        bSkipThisKey = TRUE;
                }
            }

            if (bUseIme)
            {
                if (GetKeyboardState(KeyState))
                {
                    vk = (bLowWordOnly ? LOWORD(vKey) : vKey);
                    if (pImeDpi->ImeProcessKey(hIMC, vk, lParam, KeyState))
                    {
                        pIC->bNeedsTrans = TRUE;
                        pIC->nVKey = vKey;
                        ret |= IPHK_PROCESSBYIME;
                    }
                }
            }
            else if (bSkipThisKey)
            {
                ret |= IPHK_SKIPTHISKEY;
            }

            ImmUnlockIMC(hIMC);
        }

        ImmUnlockImeDpi(pImeDpi);
    }

    if (dwHotKeyID != INVALID_HOTKEY_ID)
    {
        if (Imm32ProcessHotKey(hWnd, hIMC, hKL, dwHotKeyID))
        {
            if (vKey != VK_KANJI || dwHotKeyID != IME_JHOTKEY_CLOSE_OPEN)
                ret |= IPHK_HOTKEY;
        }
    }

    if (ret & IPHK_PROCESSBYIME)
    {
        FIXME("TODO: We have to do something here.\n");
    }

    ImmReleaseContext(hWnd, hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSystemHandler(IMM32.@)
 */
LRESULT WINAPI ImmSystemHandler(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);

    switch (wParam)
    {
        case 0x1f:
            Imm32SendChange((BOOL)lParam);
            return 0;

        case 0x20:
            ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            return 0;

        case 0x23: case 0x24:
            return Imm32UnknownProcess1((HWND)lParam, (wParam == 0x23));

        default:
            return 0;
    }
}

/***********************************************************************
 *		ImmGenerateMessage(IMM32.@)
 */
BOOL WINAPI ImmGenerateMessage(HIMC hIMC)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXT pIC;
    LPTRANSMSG pMsgs, pTrans = NULL, pItem;
    HWND hWnd;
    DWORD dwIndex, dwCount, cbTrans;
    HIMCC hMsgBuf = NULL;
    BOOL bAnsi;

    TRACE("(%p)\n", hIMC);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bAnsi = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    dwCount = pIC->dwNumMsgBuf;
    if (dwCount == 0)
        goto Quit;

    hMsgBuf = pIC->hMsgBuf;
    pMsgs = ImmLockIMCC(hMsgBuf);
    if (pMsgs == NULL)
        goto Quit;

    cbTrans = dwCount * sizeof(TRANSMSG);
    pTrans = Imm32HeapAlloc(0, cbTrans);
    if (pTrans == NULL)
        goto Quit;

    RtlCopyMemory(pTrans, pMsgs, cbTrans);

#ifdef IMM_WIN3_SUPPORT
    if (GetWin32ClientInfo()->dwExpWinVer < _WIN32_WINNT_NT4) /* old version (3.x)? */
    {
        LANGID LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        WORD wLang = PRIMARYLANGID(LangID);

        /* translate the messages if Japanese or Korean */
        if (wLang == LANG_JAPANESE ||
            (wLang == LANG_KOREAN && NtUserGetAppImeLevel(pIC->hWnd) == 3))
        {
            dwCount = ImmNt3Trans(dwCount, pTrans, hIMC, bAnsi, wLang);
        }
    }
#endif

    /* send them */
    hWnd = pIC->hWnd;
    pItem = pTrans;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pItem)
    {
        if (bAnsi)
            SendMessageA(hWnd, pItem->message, pItem->wParam, pItem->lParam);
        else
            SendMessageW(hWnd, pItem->message, pItem->wParam, pItem->lParam);
    }

Quit:
    Imm32HeapFree(pTrans);
    if (hMsgBuf)
        ImmUnlockIMCC(hMsgBuf);
    pIC->dwNumMsgBuf = 0; /* done */
    ImmUnlockIMC(hIMC);
    return TRUE;
}

VOID APIENTRY
Imm32PostMessages(HWND hwnd, HIMC hIMC, DWORD dwCount, LPTRANSMSG lpTransMsg)
{
    DWORD dwIndex;
    PCLIENTIMC pClientImc;
    LPTRANSMSG pNewTransMsg = lpTransMsg, pItem;
    BOOL bAnsi;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return;

    bAnsi = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

#ifdef IMM_WIN3_SUPPORT
    if (GetWin32ClientInfo()->dwExpWinVer < _WIN32_WINNT_NT4) /* old version (3.x)? */
    {
        LANGID LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        WORD Lang = PRIMARYLANGID(LangID);

        /* translate the messages if Japanese or Korean */
        if (Lang == LANG_JAPANESE ||
            (Lang == LANG_KOREAN && NtUserGetAppImeLevel(hwnd) == 3))
        {
            DWORD cbTransMsg = dwCount * sizeof(TRANSMSG);
            pNewTransMsg = Imm32HeapAlloc(0, cbTransMsg);
            if (pNewTransMsg)
            {
                RtlCopyMemory(pNewTransMsg, lpTransMsg, cbTransMsg);
                dwCount = ImmNt3Trans(dwCount, pNewTransMsg, hIMC, bAnsi, Lang);
            }
            else
            {
                pNewTransMsg = lpTransMsg;
            }
        }
    }
#endif

    /* post them */
    pItem = pNewTransMsg;
    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pItem)
    {
        if (bAnsi)
            PostMessageA(hwnd, pItem->message, pItem->wParam, pItem->lParam);
        else
            PostMessageW(hwnd, pItem->message, pItem->wParam, pItem->lParam);
    }

#ifdef IMM_WIN3_SUPPORT
    if (pNewTransMsg != lpTransMsg)
        Imm32HeapFree(pNewTransMsg);
#endif
}

/***********************************************************************
 *       ImmTranslateMessage(IMM32.@)
 *       ( Undocumented, call internally and from user32.dll )
 */
BOOL WINAPI ImmTranslateMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lKeyData)
{
#define MSG_COUNT 0x100
    BOOL ret = FALSE;
    INT kret;
    LPINPUTCONTEXTDX pIC;
    PIMEDPI pImeDpi = NULL;
    LPTRANSMSGLIST pList = NULL;
    LPTRANSMSG pTransMsg;
    BYTE abKeyState[256];
    HIMC hIMC;
    HKL hKL;
    UINT vk;
    DWORD dwThreadId, dwCount, cbList;
    WCHAR wch;
    WORD wChar;

    TRACE("(%p, 0x%X, %p, %p)\n", hwnd, msg, wParam, lKeyData);

    /* filter the message */
    switch (msg)
    {
        case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            break;
        default:
            return FALSE;
    }

    hIMC = ImmGetContext(hwnd);
    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmReleaseContext(hwnd, hIMC);
        return FALSE;
    }

    if (!pIC->bNeedsTrans) /* is translation needed? */
    {
        /* directly post them */
        dwCount = pIC->dwNumMsgBuf;
        if (dwCount == 0)
            goto Quit;

        pTransMsg = ImmLockIMCC(pIC->hMsgBuf);
        if (pTransMsg)
        {
            Imm32PostMessages(hwnd, hIMC, dwCount, pTransMsg);
            ImmUnlockIMCC(pIC->hMsgBuf);
            ret = TRUE;
        }
        pIC->dwNumMsgBuf = 0; /* done */
        goto Quit;
    }
    pIC->bNeedsTrans = FALSE; /* clear the flag */

    dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
    hKL = GetKeyboardLayout(dwThreadId);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        goto Quit;

    if (!GetKeyboardState(abKeyState)) /* get keyboard ON/OFF status */
        goto Quit;

    /* convert a virtual key if IME_PROP_KBD_CHAR_FIRST */
    vk = pIC->nVKey;
    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_KBD_CHAR_FIRST)
    {
        if (ImeDpi_IsUnicode(pImeDpi))
        {
            wch = 0;
            kret = ToUnicode(vk, HIWORD(lKeyData), abKeyState, &wch, 1, 0);
            if (kret == 1)
                vk = MAKELONG(LOBYTE(vk), wch);
        }
        else
        {
            wChar = 0;
            kret = ToAsciiEx(vk, HIWORD(lKeyData), abKeyState, &wChar, 0, hKL);
            if (kret > 0)
                vk = MAKEWORD(vk, wChar);
        }
    }

    /* allocate a list */
    cbList = offsetof(TRANSMSGLIST, TransMsg) + MSG_COUNT * sizeof(TRANSMSG);
    pList = Imm32HeapAlloc(0, cbList);
    if (!pList)
        goto Quit;

    /* use IME conversion engine and convert the list */
    pList->uMsgCount = MSG_COUNT;
    kret = pImeDpi->ImeToAsciiEx(vk, HIWORD(lKeyData), abKeyState, pList, 0, hIMC);
    if (kret <= 0)
        goto Quit;

    /* post them */
    if (kret <= MSG_COUNT)
    {
        Imm32PostMessages(hwnd, hIMC, kret, pList->TransMsg);
        ret = TRUE;
    }
    else
    {
        pTransMsg = ImmLockIMCC(pIC->hMsgBuf);
        if (pTransMsg == NULL)
            goto Quit;
        Imm32PostMessages(hwnd, hIMC, kret, pTransMsg);
        ImmUnlockIMCC(pIC->hMsgBuf);
    }

Quit:
    Imm32HeapFree(pList);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
    ImmReleaseContext(hwnd, hIMC);
    return ret;
#undef MSG_COUNT
}

/***********************************************************************
 *              ImmRequestMessageA(IMM32.@)
 */
LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);
    return Imm32RequestMessageAW(hIMC, wParam, lParam, TRUE);
}

/***********************************************************************
 *              ImmRequestMessageW(IMM32.@)
 */
LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);
    return Imm32RequestMessageAW(hIMC, wParam, lParam, FALSE);
}
