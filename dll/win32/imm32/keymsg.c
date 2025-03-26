/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 keys and messages
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <jpnvkeys.h>

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/* Win: IMENonIMEToggle */
BOOL APIENTRY Imm32ImeNonImeToggle(HIMC hIMC, HKL hKL, HWND hWnd, BOOL bNowIME, LANGID LangID)
{
    HKL hOldKL, LayoutList[32], hFoundKL = NULL;
    UINT iLayout, nLayoutCount;

    /* Get the previous layout */
    hOldKL = (HKL)NtUserGetThreadState(THREADSTATE_OLDKEYBOARDLAYOUT);

    /* Get the layout list */
    nLayoutCount = GetKeyboardLayoutList(_countof(LayoutList), LayoutList);

    /* Is there hOldKL in the list in the specified language ID? */
    if (hOldKL && (LangID == 0 || LOWORD(hOldKL) == LangID))
    {
        for (iLayout = 0; iLayout < nLayoutCount; ++iLayout)
        {
            if (LayoutList[iLayout] == hOldKL)
            {
                hFoundKL = hOldKL;
                break;
            }
        }
    }

    if (hFoundKL == NULL) /* Not found? */
    {
        /* Is there the keyboard layout of another kind in LangID? */
        for (iLayout = 0; iLayout < nLayoutCount; ++iLayout)
        {
            if (bNowIME == ImmIsIME(LayoutList[iLayout])) /* Same kind? */
                continue;

            if (LangID == 0 || LangID == LOWORD(LayoutList[iLayout]))
            {
                hFoundKL = LayoutList[iLayout];
                break;
            }
        }
    }

    if (hFoundKL && hKL != hFoundKL) /* Found and different layout */
    {
        PostMessageW(hWnd, WM_INPUTLANGCHANGEREQUEST, 1, (LPARAM)hFoundKL);
    }

    return ImmIsIME(hFoundKL);
}

/* Open or close the IME on Chinese or Taiwanese */
/* Win: CIMENonIMEToggle */
BOOL APIENTRY Imm32CImeNonImeToggle(HIMC hIMC, HKL hKL, HWND hWnd, LANGID LangID)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;

    if (IS_NULL_UNEXPECTEDLY(hWnd))
        return FALSE;

    if (LOWORD(hKL) != LangID || !ImmIsIME(hKL))
    {
        Imm32ImeNonImeToggle(hIMC, hKL, hWnd, FALSE, LangID);
        return TRUE;
    }

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return TRUE;

    fOpen = pIC->fOpen;
    ImmUnlockIMC(hIMC);

    if (fOpen)
        Imm32ImeNonImeToggle(hIMC, hKL, hWnd, TRUE, 0);
    else
        ImmSetOpenStatus(hIMC, TRUE);

    return TRUE;
}

/* Toggle shape mode on Chinese or Taiwanese */
/* Win: TShapeToggle */
BOOL APIENTRY Imm32CShapeToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !ImmIsIME(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
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

/* Win: CSymbolToggle */
BOOL APIENTRY Imm32CSymbolToggle(HIMC hIMC, HKL hKL, HWND hWnd)
{
    LPINPUTCONTEXT pIC;
    BOOL fOpen;
    DWORD dwConversion, dwSentence;

    if (hWnd == NULL || !ImmIsIME(hKL))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
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

/* Open or close Japanese IME */
BOOL APIENTRY Imm32JCloseOpen(HIMC hIMC, HKL hKL, HWND hWnd)
{
    BOOL fOpen;
    LPINPUTCONTEXTDX pIC;

    if (LOWORD(hKL) == LANGID_JAPANESE && ImmIsIME(hKL)) /* Japanese IME is selected */
    {
        fOpen = ImmGetOpenStatus(hIMC);
        ImmSetOpenStatus(hIMC, !fOpen);
        return TRUE;
    }

    /* Japanese IME is not selected. Select now */
    if (Imm32ImeNonImeToggle(hIMC, hKL, hWnd, FALSE, LANGID_JAPANESE))
    {
        pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        if (pIC)
        {
            pIC->dwChange |= INPUTCONTEXTDX_CHANGE_FORCE_OPEN;
            ImmUnlockIMC(hIMC);
        }
    }

    return TRUE;
}

/* Win: KShapeToggle */
BOOL APIENTRY Imm32KShapeToggle(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
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

/* Win: KHanjaConvert */
BOOL APIENTRY Imm32KHanjaConvert(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_HANJACONVERT);
    dwSentence = pIC->fdwSentence;
    ImmUnlockIMC(hIMC);

    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);
    return TRUE;
}

/* Win: KEnglishHangul */
BOOL APIENTRY Imm32KEnglish(HIMC hIMC)
{
    LPINPUTCONTEXT pIC;
    DWORD dwConversion, dwSentence;
    BOOL fOpen;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    dwConversion = (pIC->fdwConversion ^ IME_CMODE_NATIVE);
    dwSentence = pIC->fdwSentence;
    ImmSetConversionStatus(hIMC, dwConversion, dwSentence);

    fOpen = ((pIC->fdwConversion & (IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE)) != 0);
    ImmSetOpenStatus(hIMC, fOpen);

    ImmUnlockIMC(hIMC);
    return TRUE;
}

/* Win: HotKeyIDDispatcher */
BOOL APIENTRY Imm32ProcessHotKey(HWND hWnd, HIMC hIMC, HKL hKL, DWORD dwHotKeyID)
{
    PIMEDPI pImeDpi;
    BOOL ret;

    if (hIMC && IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    switch (dwHotKeyID)
    {
        case IME_CHOTKEY_IME_NONIME_TOGGLE:
            return Imm32CImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_SIMPLIFIED);

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
            return Imm32CImeNonImeToggle(hIMC, hKL, hWnd, LANGID_CHINESE_TRADITIONAL);

        case IME_THOTKEY_SHAPE_TOGGLE:
            return Imm32CShapeToggle(hIMC, hKL, hWnd);

        case IME_THOTKEY_SYMBOL_TOGGLE:
            return Imm32CSymbolToggle(hIMC, hKL, hWnd);

        default:
            WARN("0x%X\n", dwHotKeyID);
            break;
    }

    if (dwHotKeyID < IME_HOTKEY_PRIVATE_FIRST || IME_HOTKEY_PRIVATE_LAST < dwHotKeyID)
        return FALSE;

    pImeDpi = ImmLockImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    ret = (BOOL)pImeDpi->ImeEscape(hIMC, IME_ESC_PRIVATE_HOTKEY, &dwHotKeyID);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/* Win: ImmIsUIMessageWorker */
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

    if (IS_NULL_UNEXPECTEDLY(hWndIME))
        return TRUE;

    if (bAnsi)
        SendMessageA(hWndIME, msg, wParam, lParam);
    else
        SendMessageW(hWndIME, msg, wParam, lParam);

    return TRUE;
}

static BOOL CALLBACK
Imm32SendNotificationProc(
    _In_ HIMC hIMC,
    _In_ LPARAM lParam)
{
    HWND hWnd;
    LPINPUTCONTEXTDX pIC;

    UNREFERENCED_PARAMETER(lParam);

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return TRUE;

    hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        goto Quit;

    TRACE("dwChange: 0x%08X\n", pIC->dwChange);

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

BOOL APIENTRY Imm32SendNotification(BOOL bProcess)
{
    return ImmEnumInputContext((bProcess ? -1 : 0), Imm32SendNotificationProc, 0);
}

LRESULT APIENTRY
Imm32ProcessRequest(HIMC hIMC, PWND pWnd, DWORD dwCommand, LPVOID pData, BOOL bAnsiAPI)
{
    HWND hWnd;
    DWORD ret = 0, dwCharPos, cchCompStr, dwSize;
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
    {
        ERR("Out of boundary\n");
        return 0; /* Out of range */
    }

    dwSize = acbData[bAnsiAPI * 7 + dwCommand - 1];
    if (pData && IsBadWritePtr(pData, dwSize))
    {
        ERR("\n");
        return 0; /* Invalid pointer */
    }

    /* Sanity check */
    switch (dwCommand)
    {
        case IMR_RECONVERTSTRING: case IMR_DOCUMENTFEED:
            pRS = pData;
            if (pRS && (pRS->dwVersion != 0 || pRS->dwSize < sizeof(RECONVERTSTRING)))
            {
                ERR("Invalid pRS\n");
                return 0;
            }
            break;

        case IMR_CONFIRMRECONVERTSTRING:
            pRS = pData;
            if (!pRS || pRS->dwVersion != 0)
            {
                ERR("Invalid pRS\n");
                return 0;
            }
            break;

        default:
            if (IS_NULL_UNEXPECTEDLY(pData))
                return 0;
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
                goto DoIt; /* No conversion needed */

            if (bAnsiWnd)
                pTempData = ImmLocalAlloc(0, sizeof(LOGFONTA));
            else
                pTempData = ImmLocalAlloc(0, sizeof(LOGFONTW));

            if (IS_NULL_UNEXPECTEDLY(pTempData))
                return 0;
            break;

        case IMR_RECONVERTSTRING: case IMR_CONFIRMRECONVERTSTRING: case IMR_DOCUMENTFEED:
            if (bAnsiAPI == bAnsiWnd || !pData)
                goto DoIt; /* No conversion needed */

            if (bAnsiWnd)
                ret = Imm32ReconvertAnsiFromWide(NULL, pData, uCodePage);
            else
                ret = Imm32ReconvertWideFromAnsi(NULL, pData, uCodePage);

            pTempData = ImmLocalAlloc(0, ret + sizeof(WCHAR));
            if (IS_NULL_UNEXPECTEDLY(pTempData))
                return 0;

            pRS = pTempData;
            pRS->dwSize = ret;
            pRS->dwVersion = 0;

            if (dwCommand == IMR_CONFIRMRECONVERTSTRING)
            {
                if (bAnsiWnd)
                    ret = Imm32ReconvertAnsiFromWide(pTempData, pData, uCodePage);
                else
                    ret = Imm32ReconvertWideFromAnsi(pTempData, pData, uCodePage);
            }
            break;

        case IMR_QUERYCHARPOSITION:
            if (bAnsiAPI == bAnsiWnd)
                goto DoIt; /* No conversion needed */

            pICP = pData;
            dwCharPos = pICP->dwCharPos;

            if (bAnsiAPI)
            {
                cchCompStr = ImmGetCompositionStringA(hIMC, GCS_COMPSTR, NULL, 0);
                if (IS_ZERO_UNEXPECTEDLY(cchCompStr))
                    return 0;

                pCS = ImmLocalAlloc(0, (cchCompStr + 1) * sizeof(CHAR));
                if (IS_NULL_UNEXPECTEDLY(pCS))
                    return 0;

                ImmGetCompositionStringA(hIMC, GCS_COMPSTR, pCS, cchCompStr);
                pICP->dwCharPos = IchWideFromAnsi(pICP->dwCharPos, pCS, uCodePage);
            }
            else
            {
                cchCompStr = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
                if (IS_ZERO_UNEXPECTEDLY(cchCompStr))
                    return 0;

                pCS = ImmLocalAlloc(0, (cchCompStr + 1) * sizeof(WCHAR));
                if (IS_NULL_UNEXPECTEDLY(pCS))
                    return 0;

                ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pCS, cchCompStr);
                pICP->dwCharPos = IchAnsiFromWide(pICP->dwCharPos, pCS, uCodePage);
            }

            ImmLocalFree(pCS);
            break;

        default:
            WARN("0x%X\n", dwCommand);
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
        goto Quit; /* No conversion needed */

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
                break;

            if (ret < sizeof(RECONVERTSTRING))
            {
                ret = 0;
                break;
            }

            if (pTempData)
            {
                if (bAnsiWnd)
                    ret = Imm32ReconvertWideFromAnsi(pData, pTempData, uCodePage);
                else
                    ret = Imm32ReconvertAnsiFromWide(pData, pTempData, uCodePage);
            }
            break;

        case IMR_QUERYCHARPOSITION:
            pICP->dwCharPos = dwCharPos;
            break;

        default:
            WARN("0x%X\n", dwCommand);
            break;
    }

Quit:
    if (pTempData != pData)
        ImmLocalFree(pTempData);
    return ret;
}

/* Win: ImmRequestMessageWorker */
LRESULT APIENTRY ImmRequestMessageAW(HIMC hIMC, WPARAM wParam, LPARAM lParam, BOOL bAnsi)
{
    LRESULT ret = 0;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    PWND pWnd = NULL;

    if (IS_NULL_UNEXPECTEDLY(hIMC) || IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    hWnd = pIC->hWnd;
    if (hWnd)
        pWnd = ValidateHwnd(hWnd);

    if (pWnd && pWnd->head.pti == Imm32CurrentPti())
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
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return ret;

    if (pIC->bNeedsTrans)
        ret = pIC->nVKey;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetAppCompatFlags (IMM32.@)
 */
DWORD WINAPI ImmGetAppCompatFlags(HIMC hIMC)
{
    PCLIENTIMC pClientIMC;
    DWORD dwFlags;

    TRACE("(%p)\n", hIMC);

    pClientIMC = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientIMC))
        return 0;

    dwFlags = pClientIMC->dwCompatFlags;
    ImmUnlockClientImc(pClientIMC);
    return (dwFlags | g_aimm_compat_flags);
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
    BOOL bLowWordOnly = FALSE, bSkipThisKey = FALSE, bHotKeyDone = TRUE;

    TRACE("(%p, %p, 0x%X, %p, 0x%lX)\n", hWnd, hKL, vKey, lParam, dwHotKeyID);

    /* Process the key by the IME */
    hIMC = ImmGetContext(hWnd);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi)
    {
        pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        if (pIC)
        {
            if ((LOBYTE(vKey) == VK_PACKET) &&
                !(pImeDpi->ImeInfo.fdwProperty & IME_PROP_ACCEPT_WIDE_VKEY))
            {
                if (ImeDpi_IsUnicode(pImeDpi))
                {
                    bLowWordOnly = TRUE;
                }
                else
                {
                    if (pIC->fOpen)
                        ret |= IPHK_SKIPTHISKEY;

                    bSkipThisKey = TRUE;
                }
            }

            if (!bSkipThisKey && GetKeyboardState(KeyState))
            {
                UINT vk = (bLowWordOnly ? LOWORD(vKey) : vKey);
                if (pImeDpi->ImeProcessKey(hIMC, vk, lParam, KeyState))
                {
                    pIC->bNeedsTrans = TRUE;
                    pIC->nVKey = vKey;
                    ret |= IPHK_PROCESSBYIME;
                }
            }

            ImmUnlockIMC(hIMC);
        }

        ImmUnlockImeDpi(pImeDpi);
    }

    /* Process the hot-key if necessary */
    if (!CtfImmIsCiceroStartedInThread()) /* Not Cicero? */
    {
        /* Process IMM IME hotkey */
        if ((dwHotKeyID == INVALID_HOTKEY_ID) || !Imm32ProcessHotKey(hWnd, hIMC, hKL, dwHotKeyID))
            bHotKeyDone = FALSE;
    }
    else if (!CtfImeProcessCicHotkey(hIMC, vKey, lParam)) /* CTF IME not processed the hotkey? */
    {
        /* Process IMM IME hotkey */
        if (!IS_IME_HKL(hKL) ||
            ((dwHotKeyID == INVALID_HOTKEY_ID) || !Imm32ProcessHotKey(hWnd, hIMC, hKL, dwHotKeyID)))
        {
            bHotKeyDone = FALSE;
        }
    }

    if (bHotKeyDone && ((vKey != VK_KANJI) || (dwHotKeyID != IME_JHOTKEY_CLOSE_OPEN)))
        ret |= IPHK_HOTKEY;

    if ((ret & IPHK_PROCESSBYIME) && (ImmGetAppCompatFlags(hIMC) & 0x10000))
    {
        /* The key has been processed by IME's ImeProcessKey */
        LANGID wLangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        if ((PRIMARYLANGID(wLangID) == LANG_KOREAN) &&
            ((vKey == VK_PROCESSKEY) || (ret & IPHK_HOTKEY)))
        {
            /* Korean don't want VK_PROCESSKEY and IME hot-keys */
        }
        else
        {
            /* Add WM_KEYDOWN:VK_PROCESSKEY message */
            ImmTranslateMessage(hWnd, WM_KEYDOWN, VK_PROCESSKEY, lParam);

            ret &= ~IPHK_PROCESSBYIME;
            ret |= IPHK_SKIPTHISKEY;
        }
    }

    ImmReleaseContext(hWnd, hIMC);
    return ret; /* Returns IPHK_... flags */
}

/***********************************************************************
 *		ImmSystemHandler(IMM32.@)
 */
LRESULT WINAPI ImmSystemHandler(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);

    switch (wParam)
    {
        case IMS_SENDNOTIFICATION:
            Imm32SendNotification((BOOL)lParam);
            return 0;

        case IMS_COMPLETECOMPSTR:
            ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            return 0;

        case IMS_SETLANGBAND:
        case IMS_UNSETLANGBAND:
            return CtfImmSetLangBand((HWND)lParam, (wParam == IMS_SETLANGBAND));

        default:
            WARN("%p\n", wParam);
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

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bAnsi = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    dwCount = pIC->dwNumMsgBuf;
    if (dwCount == 0)
        goto Quit;

    hMsgBuf = pIC->hMsgBuf;
    pMsgs = ImmLockIMCC(hMsgBuf);
    if (IS_NULL_UNEXPECTEDLY(pMsgs))
        goto Quit;

    cbTrans = dwCount * sizeof(TRANSMSG);
    pTrans = ImmLocalAlloc(0, cbTrans);
    if (IS_NULL_UNEXPECTEDLY(pTrans))
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
            dwCount = WINNLSTranslateMessage(dwCount, pTrans, hIMC, bAnsi, wLang);
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
    ImmLocalFree(pTrans);
    if (hMsgBuf)
        ImmUnlockIMCC(hMsgBuf);
    pIC->dwNumMsgBuf = 0; /* done */
    ImmUnlockIMC(hIMC);
    return TRUE;
}

VOID APIENTRY
ImmPostMessages(HWND hwnd, HIMC hIMC, DWORD dwCount, LPTRANSMSG lpTransMsg)
{
    DWORD dwIndex;
    PCLIENTIMC pClientImc;
    LPTRANSMSG pNewTransMsg = lpTransMsg, pItem;
    BOOL bAnsi;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
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
            pNewTransMsg = ImmLocalAlloc(0, cbTransMsg);
            if (pNewTransMsg)
            {
                RtlCopyMemory(pNewTransMsg, lpTransMsg, cbTransMsg);
                dwCount = WINNLSTranslateMessage(dwCount, pNewTransMsg, hIMC, bAnsi, Lang);
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
        ImmLocalFree(pNewTransMsg);
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
    if (IS_NULL_UNEXPECTEDLY(pIC))
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
            ImmPostMessages(hwnd, hIMC, dwCount, pTransMsg);
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
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        goto Quit;

    if (!GetKeyboardState(abKeyState)) /* get keyboard ON/OFF status */
    {
        WARN("\n");
        goto Quit;
    }

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
            {
                if ((BYTE)vk == VK_PACKET)
                {
                    vk &= 0xFF;
                    vk |= (wChar << 8);
                }
                else
                {
                    vk = MAKEWORD(vk, wChar);
                }
            }
        }
    }

    /* allocate a list */
    cbList = offsetof(TRANSMSGLIST, TransMsg) + MSG_COUNT * sizeof(TRANSMSG);
    pList = ImmLocalAlloc(0, cbList);
    if (IS_NULL_UNEXPECTEDLY(pList))
        goto Quit;

    /* use IME conversion engine and convert the list */
    pList->uMsgCount = MSG_COUNT;
    kret = pImeDpi->ImeToAsciiEx(vk, HIWORD(lKeyData), abKeyState, pList, 0, hIMC);
    if (kret <= 0)
        goto Quit;

    /* post them */
    if (kret <= MSG_COUNT)
    {
        ImmPostMessages(hwnd, hIMC, kret, pList->TransMsg);
        ret = TRUE;
    }
    else
    {
        pTransMsg = ImmLockIMCC(pIC->hMsgBuf);
        if (IS_NULL_UNEXPECTEDLY(pTransMsg))
            goto Quit;
        ImmPostMessages(hwnd, hIMC, kret, pTransMsg);
        ImmUnlockIMCC(pIC->hMsgBuf);
    }

Quit:
    ImmLocalFree(pList);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
    ImmReleaseContext(hwnd, hIMC);
    TRACE("ret: %d\n", ret);
    return ret;
#undef MSG_COUNT
}

/***********************************************************************
 *              ImmRequestMessageA(IMM32.@)
 */
LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);
    return ImmRequestMessageAW(hIMC, wParam, lParam, TRUE);
}

/***********************************************************************
 *              ImmRequestMessageW(IMM32.@)
 */
LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %p, %p)\n", hIMC, wParam, lParam);
    return ImmRequestMessageAW(hIMC, wParam, lParam, FALSE);
}

/***********************************************************************
 *              ImmCallImeConsoleIME (IMM32.@)
 */
DWORD WINAPI
ImmCallImeConsoleIME(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ LPUINT puVK)
{
    DWORD dwThreadId, ret = 0;
    HKL hKL;
    PWND pWnd = NULL;
    HIMC hIMC;
    PIMEDPI pImeDpi;
    UINT uVK;
    PIMC pIMC;

    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            break;

        default:
            return 0;
    }

    dwThreadId = GetWindowThreadProcessId(hWnd, NULL);
    hKL = GetKeyboardLayout(dwThreadId);

    if (hWnd && gpsi)
        pWnd = ValidateHwndNoErr(hWnd);
    if (IS_NULL_UNEXPECTEDLY(pWnd))
        return 0;

    hIMC = ImmGetContext(hWnd);
    if (IS_NULL_UNEXPECTEDLY(hIMC))
        return 0;

    uVK = *puVK = (wParam & 0xFF);

    pIMC = ValidateHandleNoErr(hIMC, TYPE_INPUTCONTEXT);
    if (IS_NULL_UNEXPECTEDLY(pIMC))
        return 0;

    pImeDpi = ImmLockImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if ((lParam & MAKELPARAM(0, KF_UP)) && (pImeDpi->ImeInfo.fdwProperty & IME_PROP_IGNORE_UPKEYS))
        goto Quit;

    switch (uVK)
    {
        case VK_DBE_ROMAN:
        case VK_DBE_NOROMAN:
        case VK_DBE_HIRAGANA:
        case VK_DBE_KATAKANA:
        case VK_DBE_CODEINPUT:
        case VK_DBE_NOCODEINPUT:
        case VK_DBE_ENTERWORDREGISTERMODE:
        case VK_DBE_ENTERCONFIGMODE:
            break;

        default:
        {
            if (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)
            {
                if (uVK != VK_MENU && uVK != VK_F10)
                    goto Quit;
            }

            if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_NEED_ALTKEY))
            {
                if (uVK == VK_MENU || (lParam & MAKELPARAM(0, KF_ALTDOWN)))
                    goto Quit;
            }
        }
    }

    ret = ImmProcessKey(hWnd, hKL, uVK, lParam, INVALID_HOTKEY_ID);

Quit:
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}
