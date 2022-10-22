/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 Win3.x compatibility
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#ifdef IMM_WIN3_SUPPORT /* 3.x support */

/* Win: JTransCompositionA */
DWORD APIENTRY
Imm32JTransCompA(LPINPUTCONTEXTDX pIC, LPCOMPOSITIONSTRING pCS,
                 const TRANSMSG *pSrc, LPTRANSMSG pDest)
{
    FIXME("\n");
    *pDest = *pSrc;
    return 1;
}

/* Win: JTransCompositionW */
DWORD APIENTRY
Imm32JTransCompW(LPINPUTCONTEXTDX pIC, LPCOMPOSITIONSTRING pCS,
                 const TRANSMSG *pSrc, LPTRANSMSG pDest)
{
    FIXME("\n");
    *pDest = *pSrc;
    return 1;
}

typedef LRESULT (WINAPI *FN_SendMessage)(HWND, UINT, WPARAM, LPARAM);

DWORD APIENTRY
WINNLSTranslateMessageJ(DWORD dwCount, LPTRANSMSG pTrans, LPINPUTCONTEXTDX pIC,
                        LPCOMPOSITIONSTRING pCS, BOOL bAnsi)
{
    DWORD ret = 0;
    HWND hWnd, hwndDefIME;
    LPTRANSMSG pTempList, pEntry, pNext;
    DWORD dwIndex, iCandForm, dwNumber, cbTempList;
    HGLOBAL hGlobal;
    CANDIDATEFORM CandForm;
    FN_SendMessage pSendMessage;

    hWnd = pIC->hWnd;
    hwndDefIME = ImmGetDefaultIMEWnd(hWnd);
    pSendMessage = (IsWindowUnicode(hWnd) ? SendMessageW : SendMessageA);

    // clone the message list
    cbTempList = (dwCount + 1) * sizeof(TRANSMSG);
    pTempList = ImmLocalAlloc(HEAP_ZERO_MEMORY, cbTempList);
    if (pTempList == NULL)
        return 0;
    RtlCopyMemory(pTempList, pTrans, dwCount * sizeof(TRANSMSG));

    if (pIC->dwUIFlags & 0x2)
    {
        // find WM_IME_ENDCOMPOSITION
        pEntry = pTempList;
        for (dwIndex = 0; dwIndex < dwCount; ++dwIndex, ++pEntry)
        {
            if (pEntry->message == WM_IME_ENDCOMPOSITION)
                break;
        }

        if (pEntry->message == WM_IME_ENDCOMPOSITION) // if found
        {
            // move WM_IME_ENDCOMPOSITION to the end of the list
            for (pNext = pEntry + 1; pNext->message != 0; ++pEntry, ++pNext)
                *pEntry = *pNext;

            pEntry->message = WM_IME_ENDCOMPOSITION;
            pEntry->wParam = 0;
            pEntry->lParam = 0;
        }
    }

    for (pEntry = pTempList; pEntry->message != 0; ++pEntry)
    {
        switch (pEntry->message)
        {
            case WM_IME_STARTCOMPOSITION:
                if (!(pIC->dwUIFlags & 0x2))
                {
                    // send IR_OPENCONVERT
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_OPENCONVERT, 0);

                    goto DoDefault;
                }
                break;

            case WM_IME_ENDCOMPOSITION:
                if (pIC->dwUIFlags & 0x2)
                {
                    // send IR_UNDETERMINE
                    hGlobal = GlobalAlloc(GHND | GMEM_SHARE, sizeof(UNDETERMINESTRUCT));
                    if (hGlobal)
                    {
                        pSendMessage(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal);
                        GlobalFree(hGlobal);
                    }
                }
                else
                {
                    // send IR_CLOSECONVERT
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_CLOSECONVERT, 0);

                    goto DoDefault;
                }
                break;

            case WM_IME_COMPOSITION:
                if (bAnsi)
                    dwNumber = Imm32JTransCompA(pIC, pCS, pEntry, pTrans);
                else
                    dwNumber = Imm32JTransCompW(pIC, pCS, pEntry, pTrans);

                ret += dwNumber;
                pTrans += dwNumber;

                // send IR_CHANGECONVERT
                if (!(pIC->dwUIFlags & 0x2))
                {
                    if (pIC->cfCompForm.dwStyle != CFS_DEFAULT)
                        pSendMessage(hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
                }
                break;

            case WM_IME_NOTIFY:
                if (pEntry->wParam == IMN_OPENCANDIDATE)
                {
                    if (IsWindow(hWnd) && (pIC->dwUIFlags & 0x2))
                    {
                        // send IMC_SETCANDIDATEPOS
                        for (iCandForm = 0; iCandForm < MAX_CANDIDATEFORM; ++iCandForm)
                        {
                            if (!(pEntry->lParam & (1 << iCandForm)))
                                continue;

                            CandForm.dwIndex = iCandForm;
                            CandForm.dwStyle = CFS_EXCLUDE;
                            CandForm.ptCurrentPos = pIC->cfCompForm.ptCurrentPos;
                            CandForm.rcArea = pIC->cfCompForm.rcArea;
                            pSendMessage(hwndDefIME, WM_IME_CONTROL, IMC_SETCANDIDATEPOS,
                                         (LPARAM)&CandForm);
                        }
                    }
                }

                if (!(pIC->dwUIFlags & 0x2))
                    goto DoDefault;

                // send a WM_IME_NOTIFY notification to the default ime window
                pSendMessage(hwndDefIME, pEntry->message, pEntry->wParam, pEntry->lParam);
                break;

DoDefault:
            default:
                // default processing
                *pTrans++ = *pEntry;
                ++ret;
                break;
        }
    }

    ImmLocalFree(pTempList);
    return ret;
}

DWORD APIENTRY
WINNLSTranslateMessageK(DWORD dwCount, LPTRANSMSG pEntries, LPINPUTCONTEXTDX pIC,
                        LPCOMPOSITIONSTRING pCS, BOOL bAnsi)
{
    FIXME("(0x%X, %p, %p, %p, %d)\n", dwCount, pEntries, pIC, pCS, bAnsi);
    return dwCount;
}

DWORD APIENTRY
WINNLSTranslateMessage(DWORD dwCount, LPTRANSMSG pEntries, HIMC hIMC, BOOL bAnsi, WORD wLang)
{
    BOOL ret = FALSE;
    LPINPUTCONTEXTDX pIC;
    LPCOMPOSITIONSTRING pCS;

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTED(pIC))
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (IS_NULL_UNEXPECTED(pCS))
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    if (wLang == LANG_JAPANESE)
        ret = WINNLSTranslateMessageJ(dwCount, pEntries, pIC, pCS, bAnsi);
    else if (wLang == LANG_KOREAN)
        ret = WINNLSTranslateMessageK(dwCount, pEntries, pIC, pCS, bAnsi);

    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
}

#endif /* IMM_WIN3_SUPPORT */

/***********************************************************************
 *		ImmSendIMEMessageExA(IMM32.@)
 */
WORD WINAPI ImmSendIMEMessageExA(HWND hWnd, LPARAM lParam)
{
    FIXME("(%p, %p)\n", hWnd, lParam);
    return 0;
}

/***********************************************************************
 *		ImmSendIMEMessageExW(IMM32.@)
 */
WORD WINAPI ImmSendIMEMessageExW(HWND hWnd, LPARAM lParam)
{
    FIXME("(%p, %p)\n", hWnd, lParam);
    return 0;
}
