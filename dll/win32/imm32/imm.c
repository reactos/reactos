/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing Far-Eastern languages input
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

HMODULE ghImm32Inst = NULL; // Win: ghInst
PSERVERINFO gpsi = NULL; // Win: gpsi
SHAREDINFO gSharedInfo = { NULL }; // Win: gSharedInfo
BYTE gfImmInitialized = FALSE; // Win: gfInitialized

// Win: ImmInitializeGlobals
static BOOL APIENTRY ImmInitializeGlobals(HMODULE hMod)
{
    NTSTATUS status;

    if (hMod)
        ghImm32Inst = hMod;

    if (gfImmInitialized)
        return TRUE;

    status = RtlInitializeCriticalSection(&gcsImeDpi);
    if (NT_ERROR(status))
        return FALSE;

    gfImmInitialized = TRUE;
    return TRUE;
}

/***********************************************************************
 *		ImmRegisterClient(IMM32.@)
 *       ( Undocumented, called from user32.dll )
 */
BOOL WINAPI ImmRegisterClient(PSHAREDINFO ptr, HINSTANCE hMod)
{
    gSharedInfo = *ptr;
    gpsi = gSharedInfo.psi;
    return ImmInitializeGlobals(hMod);
}

/***********************************************************************
 *		ImmLoadLayout (IMM32.@)
 */
BOOL WINAPI ImmLoadLayout(HKL hKL, PIMEINFOEX pImeInfoEx)
{
    DWORD cbData;
    HKEY hLayoutKey = NULL, hLayoutsKey = NULL;
    LONG error;
    WCHAR szLayout[MAX_PATH];

    TRACE("(%p, %p)\n", hKL, pImeInfoEx);

    if (IS_IME_HKL(hKL) || !Imm32IsCiceroMode() || Imm32Is16BitMode())
    {
        Imm32UIntToStr((DWORD)(DWORD_PTR)hKL, 16, szLayout, _countof(szLayout));

        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hLayoutsKey);
        if (error)
        {
            ERR("RegOpenKeyW: 0x%08lX\n", error);
            return FALSE;
        }

        error = RegOpenKeyW(hLayoutsKey, szLayout, &hLayoutKey);
        if (error)
        {
            ERR("RegOpenKeyW: 0x%08lX\n", error);
            RegCloseKey(hLayoutsKey);
            return FALSE;
        }
    }
    else
    {
        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_IMM, &hLayoutKey);
        if (error)
        {
            ERR("RegOpenKeyW: 0x%08lX\n", error);
            return FALSE;
        }
    }

    cbData = sizeof(pImeInfoEx->wszImeFile);
    error = RegQueryValueExW(hLayoutKey, L"Ime File", 0, 0,
                             (LPBYTE)pImeInfoEx->wszImeFile, &cbData);
    pImeInfoEx->wszImeFile[_countof(pImeInfoEx->wszImeFile) - 1] = 0;

    RegCloseKey(hLayoutKey);
    if (hLayoutsKey)
        RegCloseKey(hLayoutsKey);

    pImeInfoEx->fLoadFlag = 0;

    if (error)
    {
        ERR("RegQueryValueExW: 0x%08lX\n", error);
        pImeInfoEx->hkl = NULL;
        return FALSE;
    }

    pImeInfoEx->hkl = hKL;
    return Imm32LoadImeVerInfo(pImeInfoEx);
}

/***********************************************************************
 *		ImmFreeLayout (IMM32.@)
 */
BOOL WINAPI ImmFreeLayout(DWORD dwUnknown)
{
    WCHAR szKBD[9];
    UINT iKL, cKLs;
    HKL hOldKL, hNewKL, *pList;
    PIMEDPI pImeDpi;
    LANGID LangID;

    TRACE("(0x%lX)\n", dwUnknown);

    hOldKL = GetKeyboardLayout(0);

    if (dwUnknown == 1)
    {
        if (!IS_IME_HKL(hOldKL))
            return TRUE;

        LangID = LANGIDFROMLCID(GetSystemDefaultLCID());

        cKLs = GetKeyboardLayoutList(0, NULL);
        if (cKLs)
        {
            pList = ImmLocalAlloc(0, cKLs * sizeof(HKL));
            if (pList == NULL)
                return FALSE;

            cKLs = GetKeyboardLayoutList(cKLs, pList);
            for (iKL = 0; iKL < cKLs; ++iKL)
            {
                if (!IS_IME_HKL(pList[iKL]))
                {
                    LangID = LOWORD(pList[iKL]);
                    break;
                }
            }

            ImmLocalFree(pList);
        }

        StringCchPrintfW(szKBD, _countof(szKBD), L"%08X", LangID);
        if (!LoadKeyboardLayoutW(szKBD, KLF_ACTIVATE))
            LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | 0x200);
    }
    else if (dwUnknown == 2)
    {
        RtlEnterCriticalSection(&gcsImeDpi);
Retry:
        for (pImeDpi = gpImeDpiList; pImeDpi; pImeDpi = pImeDpi->pNext)
        {
            if (Imm32ReleaseIME(pImeDpi->hKL))
                goto Retry;
        }
        RtlLeaveCriticalSection(&gcsImeDpi);
    }
    else
    {
        hNewKL = (HKL)(DWORD_PTR)dwUnknown;
        if (IS_IME_HKL(hNewKL) && hNewKL != hOldKL)
            Imm32ReleaseIME(hNewKL);
    }

    return TRUE;
}

// Win: SelectInputContext
VOID APIENTRY Imm32SelectInputContext(HKL hNewKL, HKL hOldKL, HIMC hIMC)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXTDX pIC;
    LPGUIDELINE pGL;
    LPCANDIDATEINFO pCI;
    LPCOMPOSITIONSTRING pCS;
    LOGFONTA LogFontA;
    LOGFONTW LogFontW;
    BOOL fOpen, bIsNewHKLIme = TRUE, bIsOldHKLIme = TRUE, bClientWide, bNewDpiWide;
    DWORD cbNewPrivate = 0, cbOldPrivate = 0, dwConversion, dwSentence, dwSize, dwNewSize;
    PIMEDPI pNewImeDpi = NULL, pOldImeDpi = NULL;
    HANDLE hPrivate;
    PIME_STATE pNewState = NULL, pOldState = NULL;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return;

    pNewImeDpi = ImmLockImeDpi(hNewKL);

    if (hNewKL != hOldKL)
        pOldImeDpi = ImmLockImeDpi(hOldKL);

    if (pNewImeDpi)
    {
        cbNewPrivate = pNewImeDpi->ImeInfo.dwPrivateDataSize;
        pClientImc->uCodePage = pNewImeDpi->uCodePage;
    }
    else
    {
        pClientImc->uCodePage = CP_ACP;
    }

    if (cbNewPrivate < sizeof(DWORD))
        cbNewPrivate = sizeof(DWORD);

    if (pOldImeDpi)
        cbOldPrivate = pOldImeDpi->ImeInfo.dwPrivateDataSize;

    if (cbOldPrivate < sizeof(DWORD))
        cbOldPrivate = sizeof(DWORD);

    if (pClientImc->hKL == hOldKL)
    {
        if (pOldImeDpi)
        {
            if (IS_IME_HKL(hOldKL))
                pOldImeDpi->ImeSelect(hIMC, FALSE);
            else if (Imm32IsCiceroMode() && !Imm32Is16BitMode() && pOldImeDpi->CtfImeSelectEx)
                pOldImeDpi->CtfImeSelectEx(hIMC, FALSE, hOldKL);
        }
        pClientImc->hKL = NULL;
    }

    if (CtfImmIsTextFrameServiceDisabled())
    {
        if (IS_IMM_MODE() && !Imm32IsCiceroMode())
        {
            bIsNewHKLIme = IS_IME_HKL(hNewKL);
            bIsOldHKLIme = IS_IME_HKL(hOldKL);
        }
    }

    pIC = (LPINPUTCONTEXTDX)Imm32InternalLockIMC(hIMC, FALSE);
    if (!pIC)
    {
        if (pNewImeDpi)
        {
            if (IS_IME_HKL(hNewKL))
                pNewImeDpi->ImeSelect(hIMC, TRUE);
            else if (Imm32IsCiceroMode() && !Imm32Is16BitMode() && pNewImeDpi->CtfImeSelectEx)
                pNewImeDpi->CtfImeSelectEx(hIMC, TRUE, hNewKL);

            pClientImc->hKL = hNewKL;
        }
    }
    else
    {
        dwConversion = pIC->fdwConversion;
        dwSentence = pIC->fdwSentence;
        fOpen = pIC->fOpen;

        if (pNewImeDpi)
        {
            bClientWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
            bNewDpiWide = ImeDpi_IsUnicode(pNewImeDpi);
            if (bClientWide && !bNewDpiWide)
            {
                if (pIC->fdwInit & INIT_LOGFONT)
                {
                    LogFontWideToAnsi(&pIC->lfFont.W, &LogFontA);
                    pIC->lfFont.A = LogFontA;
                }
                pClientImc->dwFlags &= ~CLIENTIMC_WIDE;
            }
            else if (!bClientWide && bNewDpiWide)
            {
                if (pIC->fdwInit & INIT_LOGFONT)
                {
                    LogFontAnsiToWide(&pIC->lfFont.A, &LogFontW);
                    pIC->lfFont.W = LogFontW;
                }
                pClientImc->dwFlags |= CLIENTIMC_WIDE;
            }
        }

        if (cbOldPrivate != cbNewPrivate)
        {
            hPrivate = ImmReSizeIMCC(pIC->hPrivate, cbNewPrivate);
            if (!hPrivate)
            {
                ImmDestroyIMCC(pIC->hPrivate);
                hPrivate = ImmCreateIMCC(cbNewPrivate);
            }
            pIC->hPrivate = hPrivate;
        }

#define MAX_IMCC_SIZE 0x1000
        dwSize = ImmGetIMCCSize(pIC->hMsgBuf);
        if (ImmGetIMCCLockCount(pIC->hMsgBuf) || dwSize > MAX_IMCC_SIZE)
        {
            ImmDestroyIMCC(pIC->hMsgBuf);
            pIC->hMsgBuf = ImmCreateIMCC(sizeof(UINT));
            pIC->dwNumMsgBuf = 0;
        }

        dwSize = ImmGetIMCCSize(pIC->hGuideLine);
        dwNewSize = sizeof(GUIDELINE);
        if (ImmGetIMCCLockCount(pIC->hGuideLine) ||
            dwSize < dwNewSize || dwSize > MAX_IMCC_SIZE)
        {
            ImmDestroyIMCC(pIC->hGuideLine);
            pIC->hGuideLine = ImmCreateIMCC(dwNewSize);
            pGL = ImmLockIMCC(pIC->hGuideLine);
            if (pGL)
            {
                pGL->dwSize = dwNewSize;
                ImmUnlockIMCC(pIC->hGuideLine);
            }
        }

        dwSize = ImmGetIMCCSize(pIC->hCandInfo);
        dwNewSize = sizeof(CANDIDATEINFO);
        if (ImmGetIMCCLockCount(pIC->hCandInfo) ||
            dwSize < dwNewSize || dwSize > MAX_IMCC_SIZE)
        {
            ImmDestroyIMCC(pIC->hCandInfo);
            pIC->hCandInfo = ImmCreateIMCC(dwNewSize);
            pCI = ImmLockIMCC(pIC->hCandInfo);
            if (pCI)
            {
                pCI->dwSize = dwNewSize;
                ImmUnlockIMCC(pIC->hCandInfo);
            }
        }

        dwSize = ImmGetIMCCSize(pIC->hCompStr);
        dwNewSize = sizeof(COMPOSITIONSTRING);
        if (ImmGetIMCCLockCount(pIC->hCompStr) ||
            dwSize < dwNewSize || dwSize > MAX_IMCC_SIZE)
        {
            ImmDestroyIMCC(pIC->hCompStr);
            pIC->hCompStr = ImmCreateIMCC(dwNewSize);
            pCS = ImmLockIMCC(pIC->hCompStr);
            if (pCS)
            {
                pCS->dwSize = dwNewSize;
                ImmUnlockIMCC(pIC->hCompStr);
            }
        }
#undef MAX_IMCC_SIZE

        if (pOldImeDpi && bIsOldHKLIme)
        {
            pOldState = Imm32FetchImeState(pIC, hOldKL);
            if (pOldState)
                Imm32SaveImeStateSentence(pIC, pOldState, hOldKL);
        }

        if (pNewImeDpi && bIsNewHKLIme)
            pNewState = Imm32FetchImeState(pIC, hNewKL);

        if (pOldState != pNewState)
        {
            if (pOldState)
            {
                pOldState->fOpen = !!pIC->fOpen;
                pOldState->dwConversion = (pIC->fdwConversion & ~IME_CMODE_EUDC);
                pOldState->dwSentence = pIC->fdwSentence;
                pOldState->dwInit = pIC->fdwInit;
            }

            if (pNewState)
            {
                if (pIC->dwChange & INPUTCONTEXTDX_CHANGE_FORCE_OPEN)
                {
                    pIC->dwChange &= ~INPUTCONTEXTDX_CHANGE_FORCE_OPEN;
                    pIC->fOpen = TRUE;
                }
                else
                {
                    pIC->fOpen = pNewState->fOpen;
                }

                pIC->fdwConversion = (pNewState->dwConversion & ~IME_CMODE_EUDC);
                pIC->fdwSentence = pNewState->dwSentence;
                pIC->fdwInit = pNewState->dwInit;
            }
        }

        if (pNewState)
            Imm32LoadImeStateSentence(pIC, pNewState, hNewKL);

        if (pNewImeDpi)
        {
            if (IS_IME_HKL(hNewKL))
                pNewImeDpi->ImeSelect(hIMC, TRUE);
            else if (Imm32IsCiceroMode() && !Imm32Is16BitMode() && pNewImeDpi->CtfImeSelectEx)
                pNewImeDpi->CtfImeSelectEx(hIMC, TRUE, hNewKL);

            pClientImc->hKL = hNewKL;
        }

        pIC->dwChange = 0;
        if (pIC->fOpen != fOpen)
            pIC->dwChange |= INPUTCONTEXTDX_CHANGE_OPEN;
        if (pIC->fdwConversion != dwConversion)
            pIC->dwChange |= INPUTCONTEXTDX_CHANGE_CONVERSION;
        if (pIC->fdwSentence != dwSentence)
            pIC->dwChange |= INPUTCONTEXTDX_CHANGE_SENTENCE;

        ImmUnlockIMC(hIMC);
    }

    ImmUnlockImeDpi(pOldImeDpi);
    ImmUnlockImeDpi(pNewImeDpi);
    ImmUnlockClientImc(pClientImc);
}

typedef struct SELECT_LAYOUT
{
    HKL hNewKL;
    HKL hOldKL;
} SELECT_LAYOUT, *LPSELECT_LAYOUT;

// Win: SelectContextProc
static BOOL CALLBACK Imm32SelectContextProc(HIMC hIMC, LPARAM lParam)
{
    LPSELECT_LAYOUT pSelect = (LPSELECT_LAYOUT)lParam;
    Imm32SelectInputContext(pSelect->hNewKL, pSelect->hOldKL, hIMC);
    return TRUE;
}

// Win: NotifyIMEProc
static BOOL CALLBACK Imm32NotifyIMEProc(HIMC hIMC, LPARAM lParam)
{
    ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, (DWORD)lParam, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmActivateLayout (IMM32.@)
 */
BOOL WINAPI ImmActivateLayout(HKL hKL)
{
    PIMEDPI pImeDpi;
    HKL hOldKL;
    LPARAM lParam;
    HWND hwndDefIME = NULL;
    SELECT_LAYOUT SelectLayout;

    hOldKL = GetKeyboardLayout(0);

    if (hOldKL == hKL && !(GetWin32ClientInfo()->CI_flags & CI_IMMACTIVATE))
        return TRUE;

    ImmLoadIME(hKL);

    if (hOldKL != hKL)
    {
        pImeDpi = ImmLockImeDpi(hOldKL);
        if (pImeDpi)
        {
            if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_COMPLETE_ON_UNSELECT)
                lParam = CPS_COMPLETE;
            else
                lParam = CPS_CANCEL;
            ImmUnlockImeDpi(pImeDpi);

            ImmEnumInputContext(0, Imm32NotifyIMEProc, lParam);
        }

        hwndDefIME = ImmGetDefaultIMEWnd(NULL);
        if (IsWindow(hwndDefIME))
            SendMessageW(hwndDefIME, WM_IME_SELECT, FALSE, (LPARAM)hOldKL);

        NtUserSetThreadLayoutHandles(hKL, hOldKL);
    }

    SelectLayout.hNewKL = hKL;
    SelectLayout.hOldKL = hOldKL;
    ImmEnumInputContext(0, Imm32SelectContextProc, (LPARAM)&SelectLayout);

    if (IsWindow(hwndDefIME))
        SendMessageW(hwndDefIME, WM_IME_SELECT, TRUE, (LPARAM)hKL);

    return TRUE;
}

static VOID APIENTRY Imm32CiceroSetActiveContext(HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL)
{
    TRACE("We have to do something\n");
}

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    PWND pWnd;
    HWND hwndFocus;
    DWORD dwValue;
    HIMC hOldIMC;

    TRACE("(%p, %p)\n", hWnd, hIMC);

    if (!IS_IMM_MODE())
        return NULL;

    pWnd = ValidateHwnd(hWnd);
    if (!pWnd)
        return NULL;

    if (hIMC && Imm32IsCrossThreadAccess(hIMC))
        return NULL;

    hOldIMC = pWnd->hImc;
    if (hOldIMC == hIMC)
        return hIMC;

    dwValue = NtUserAssociateInputContext(hWnd, hIMC, 0);
    if (dwValue == 0)
        return hOldIMC;
    if (dwValue != 1)
        return NULL;

    hwndFocus = (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_FOCUS);
    if (hwndFocus == hWnd)
    {
        ImmSetActiveContext(hWnd, hOldIMC, FALSE);
        ImmSetActiveContext(hWnd, hIMC, TRUE);
    }

    return hOldIMC;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    HWND hwndFocus;
    PWND pFocusWnd;
    HIMC hOldIMC = NULL;
    DWORD dwValue;

    TRACE("(%p, %p, 0x%lX)\n", hWnd, hIMC, dwFlags);

    if (!IS_IMM_MODE())
        return FALSE;

    if (hIMC && !(dwFlags & IACE_DEFAULT) && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hwndFocus = (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_FOCUS);
    pFocusWnd = ValidateHwnd(hwndFocus);
    if (pFocusWnd)
        hOldIMC = pFocusWnd->hImc;

    dwValue = NtUserAssociateInputContext(hWnd, hIMC, dwFlags);
    switch (dwValue)
    {
        case 0:
            return TRUE;

        case 1:
            pFocusWnd = ValidateHwnd(hwndFocus);
            if (pFocusWnd)
            {
                hIMC = pFocusWnd->hImc;
                if (hIMC != hOldIMC)
                {
                    ImmSetActiveContext(hwndFocus, hOldIMC, FALSE);
                    ImmSetActiveContext(hwndFocus, hIMC, TRUE);
                }
            }
            return TRUE;

        default:
            return FALSE;
    }
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    PCLIENTIMC pClientImc;
    HIMC hIMC;

    TRACE("()\n");

    if (!IS_IMM_MODE())
        return NULL;

    pClientImc = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
    if (pClientImc == NULL)
        return NULL;

    hIMC = NtUserCreateInputContext((ULONG_PTR)pClientImc);
    if (hIMC == NULL)
    {
        ImmLocalFree(pClientImc);
        return NULL;
    }

    RtlInitializeCriticalSection(&pClientImc->cs);

    pClientImc->dwCompatFlags = (DWORD)NtUserGetThreadState(THREADSTATE_IMECOMPATFLAGS);

    return hIMC;
}

// Win: DestroyImeModeSaver
static VOID APIENTRY Imm32DestroyImeModeSaver(LPINPUTCONTEXTDX pIC)
{
    PIME_STATE pState, pNext;
    PIME_SUBSTATE pSubState, pSubNext;

    for (pState = pIC->pState; pState; pState = pNext)
    {
        pNext = pState->pNext;

        for (pSubState = pState->pSubState; pSubState; pSubState = pSubNext)
        {
            pSubNext = pSubState->pNext;
            ImmLocalFree(pSubState);
        }

        ImmLocalFree(pState);
    }

    pIC->pState = NULL;
}

// Win: DestroyInputContext
BOOL APIENTRY Imm32DestroyInputContext(HIMC hIMC, HKL hKL, BOOL bKeep)
{
    PIMEDPI pImeDpi;
    LPINPUTCONTEXTDX pIC;
    PCLIENTIMC pClientImc;
    PIMC pIMC;

    if (!hIMC || !IS_IMM_MODE())
        return FALSE;

    pIMC = ValidateHandle(hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC || pIMC->head.pti != Imm32CurrentPti())
    {
        ERR("invalid pIMC: %p\n", pIMC);
        return FALSE;
    }

    pClientImc = (PCLIENTIMC)pIMC->dwClientImcData;
    if (!pClientImc)
        goto Finish;

    if ((pClientImc->dwFlags & CLIENTIMC_UNKNOWN2) && !bKeep)
    {
        ERR("CLIENTIMC_UNKNOWN2\n");
        return FALSE;
    }

    if (pClientImc->dwFlags & CLIENTIMC_DESTROY)
        return TRUE;

    InterlockedIncrement(&pClientImc->cLockObj);

    if (!pClientImc->hInputContext)
        goto Quit;

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
    {
        ImmUnlockClientImc(pClientImc);
        ERR("!pIC\n");
        return FALSE;
    }

    CtfImmTIMDestroyInputContext(hIMC);

    if (pClientImc->hKL == hKL)
    {
        pImeDpi = ImmLockImeDpi(hKL);
        if (pImeDpi)
        {
            if (IS_IME_HKL(hKL))
                pImeDpi->ImeSelect(hIMC, FALSE);
            else if (Imm32IsCiceroMode() && !Imm32Is16BitMode())
                pImeDpi->CtfImeSelectEx(hIMC, FALSE, hKL);

            ImmUnlockImeDpi(pImeDpi);
        }

        pClientImc->hKL = NULL;
    }

    ImmDestroyIMCC(pIC->hPrivate);
    ImmDestroyIMCC(pIC->hMsgBuf);
    ImmDestroyIMCC(pIC->hGuideLine);
    ImmDestroyIMCC(pIC->hCandInfo);
    ImmDestroyIMCC(pIC->hCompStr);
    Imm32DestroyImeModeSaver(pIC);
    ImmUnlockIMC(hIMC);

Quit:
    pClientImc->dwFlags |= CLIENTIMC_DESTROY;
    ImmUnlockClientImc(pClientImc);

Finish:
    if (bKeep)
        return TRUE;
    return NtUserDestroyInputContext(hIMC);
}

// NOTE: Windows does recursive call ImmLockIMC here but we don't do so.
// Win: BOOL CreateInputContext(HIMC hIMC, HKL hKL, BOOL fSelect)
BOOL APIENTRY
Imm32CreateInputContext(HIMC hIMC, LPINPUTCONTEXT pIC, PCLIENTIMC pClientImc, HKL hKL, BOOL fSelect)
{
    DWORD dwIndex, cbPrivate;
    PIMEDPI pImeDpi = NULL;
    LPCOMPOSITIONSTRING pCS;
    LPCANDIDATEINFO pCI;
    LPGUIDELINE pGL;

    /* Create IC components */
    pIC->hCompStr = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    pIC->hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    pIC->hGuideLine = ImmCreateIMCC(sizeof(GUIDELINE));
    pIC->hMsgBuf = ImmCreateIMCC(sizeof(UINT));
    if (!pIC->hCompStr || !pIC->hCandInfo || !pIC->hGuideLine || !pIC->hMsgBuf)
        goto Fail;

    /* Initialize IC components */
    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
        goto Fail;
    pCS->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(pIC->hCompStr);

    pCI = ImmLockIMCC(pIC->hCandInfo);
    if (!pCI)
        goto Fail;
    pCI->dwSize = sizeof(CANDIDATEINFO);
    ImmUnlockIMCC(pIC->hCandInfo);

    pGL = ImmLockIMCC(pIC->hGuideLine);
    if (!pGL)
        goto Fail;
    pGL->dwSize = sizeof(GUIDELINE);
    ImmUnlockIMCC(pIC->hGuideLine);

    pIC->dwNumMsgBuf = 0;
    pIC->fOpen = FALSE;
    pIC->fdwConversion = pIC->fdwSentence = 0;

    for (dwIndex = 0; dwIndex < MAX_CANDIDATEFORM; ++dwIndex)
        pIC->cfCandForm[dwIndex].dwIndex = IMM_INVALID_CANDFORM;

    /* Get private data size */
    pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
    {
        cbPrivate = sizeof(DWORD);
    }
    else
    {
        /* Update CLIENTIMC */
        pClientImc->uCodePage = pImeDpi->uCodePage;
        if (ImeDpi_IsUnicode(pImeDpi))
            pClientImc->dwFlags |= CLIENTIMC_WIDE;

        cbPrivate = pImeDpi->ImeInfo.dwPrivateDataSize;
    }

    /* Create private data */
    pIC->hPrivate = ImmCreateIMCC(cbPrivate);
    if (!pIC->hPrivate)
        goto Fail;

    if (pImeDpi)
    {
        /* Select the IME */
        if (fSelect)
        {
            if (IS_IME_HKL(hKL))
                pImeDpi->ImeSelect(hIMC, TRUE);
            else if (Imm32IsCiceroMode() && !Imm32Is16BitMode() && pImeDpi->CtfImeSelectEx)
                pImeDpi->CtfImeSelectEx(hIMC, TRUE, hKL);
        }

        /* Set HKL */
        pClientImc->hKL = hKL;

        ImmUnlockImeDpi(pImeDpi);
    }

    return TRUE;

Fail:
    if (pImeDpi)
        ImmUnlockImeDpi(pImeDpi);

    pIC->hMsgBuf = ImmDestroyIMCC(pIC->hMsgBuf);
    pIC->hGuideLine = ImmDestroyIMCC(pIC->hGuideLine);
    pIC->hCandInfo = ImmDestroyIMCC(pIC->hCandInfo);
    pIC->hCompStr = ImmDestroyIMCC(pIC->hCompStr);
    return FALSE;
}

// Win: InternalImmLockIMC
LPINPUTCONTEXT APIENTRY Imm32InternalLockIMC(HIMC hIMC, BOOL fSelect)
{
    HANDLE hIC;
    LPINPUTCONTEXT pIC = NULL;
    PCLIENTIMC pClientImc;
    WORD LangID;
    DWORD dwThreadId;
    HKL hOldKL, hNewKL;
    PIMEDPI pImeDpi = NULL;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return NULL;

    RtlEnterCriticalSection(&pClientImc->cs);

    if (pClientImc->hInputContext)
    {
        pIC = LocalLock(pClientImc->hInputContext);
        if (pIC)
            goto Success;
        else
            goto Failure;
    }

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (dwThreadId == GetCurrentThreadId() && Imm32IsCiceroMode() && !Imm32Is16BitMode())
    {
        hOldKL = GetKeyboardLayout(0);
        LangID = LOWORD(hOldKL);
        hNewKL = (HKL)(DWORD_PTR)MAKELONG(LangID, LangID);

        pImeDpi = Imm32FindOrLoadImeDpi(hNewKL);
        if (pImeDpi)
        {
            CtfImmTIMActivate(hNewKL);
        }
    }

    if (!NtUserQueryInputContext(hIMC, QIC_DEFAULTWINDOWIME))
        goto Failure;

    hIC = LocalAlloc(LHND, sizeof(INPUTCONTEXTDX));
    pIC = LocalLock(hIC);
    if (!pIC)
    {
        LocalFree(hIC);
        goto Failure;
    }
    pClientImc->hInputContext = hIC;

    hNewKL = GetKeyboardLayout(dwThreadId);
    if (!Imm32CreateInputContext(hIMC, pIC, pClientImc, hNewKL, fSelect))
    {
        pClientImc->hInputContext = LocalFree(pClientImc->hInputContext);
        goto Failure;
    }

Success:
    CtfImmTIMCreateInputContext(hIMC);
    RtlLeaveCriticalSection(&pClientImc->cs);
    InterlockedIncrement(&pClientImc->cLockObj);
    ImmUnlockClientImc(pClientImc);
    return pIC;

Failure:
    RtlLeaveCriticalSection(&pClientImc->cs);
    ImmUnlockClientImc(pClientImc);
    return NULL;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    HKL hKL;

    TRACE("(%p)\n", hIMC);

    if (!IS_IMM_MODE())
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    return Imm32DestroyInputContext(hIMC, hKL, FALSE);
}

/***********************************************************************
 *		ImmLockClientImc (IMM32.@)
 */
PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc)
{
    PIMC pIMC;
    PCLIENTIMC pClientImc;

    TRACE("(%p)\n", hImc);

    if (!hImc)
        return NULL;

    pIMC = ValidateHandle(hImc, TYPE_INPUTCONTEXT);
    if (!pIMC || !Imm32CheckImcProcess(pIMC))
        return NULL;

    pClientImc = (PCLIENTIMC)pIMC->dwClientImcData;
    if (pClientImc)
    {
        if (pClientImc->dwFlags & CLIENTIMC_DESTROY)
            return NULL;
        goto Finish;
    }

    pClientImc = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
    if (!pClientImc)
        return NULL;

    RtlInitializeCriticalSection(&pClientImc->cs);
    pClientImc->dwCompatFlags = (DWORD)NtUserGetThreadState(THREADSTATE_IMECOMPATFLAGS);

    if (!NtUserUpdateInputContext(hImc, UIC_CLIENTIMCDATA, (DWORD_PTR)pClientImc))
    {
        ImmLocalFree(pClientImc);
        return NULL;
    }

    pClientImc->dwFlags |= CLIENTIMC_UNKNOWN2;

Finish:
    InterlockedIncrement(&pClientImc->cLockObj);
    return pClientImc;
}

/***********************************************************************
 *		ImmUnlockClientImc (IMM32.@)
 */
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc)
{
    LONG cLocks;
    HANDLE hInputContext;

    TRACE("(%p)\n", pClientImc);

    cLocks = InterlockedDecrement(&pClientImc->cLockObj);
    if (cLocks != 0 || !(pClientImc->dwFlags & CLIENTIMC_DESTROY))
        return;

    hInputContext = pClientImc->hInputContext;
    if (hInputContext)
        LocalFree(hInputContext);

    RtlDeleteCriticalSection(&pClientImc->cs);
    ImmLocalFree(pClientImc);
}

// Win: ImmGetSaveContext
static HIMC APIENTRY ImmGetSaveContext(HWND hWnd, DWORD dwContextFlags)
{
    HIMC hIMC;
    PCLIENTIMC pClientImc;
    PWND pWnd;

    if (!IS_IMM_MODE())
        return NULL;

    if (!hWnd)
    {
        hIMC = (HIMC)NtUserGetThreadState(THREADSTATE_DEFAULTINPUTCONTEXT);
        goto Quit;
    }

    pWnd = ValidateHwnd(hWnd);
    if (!pWnd || Imm32IsCrossProcessAccess(hWnd))
        return NULL;

    hIMC = pWnd->hImc;
    if (!hIMC && (dwContextFlags & 1))
        hIMC = (HIMC)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_ICONTEXT);

Quit:
    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return NULL;
    if ((dwContextFlags & 2) && (pClientImc->dwFlags & CLIENTIMC_UNKNOWN3))
        hIMC = NULL;
    ImmUnlockClientImc(pClientImc);
    return hIMC;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    TRACE("(%p)\n", hWnd);
    if (hWnd == NULL)
        return NULL;
    return ImmGetSaveContext(hWnd, 2);
}

/***********************************************************************
 *		ImmLockIMC(IMM32.@)
 *
 * NOTE: This is not ImmLockIMCC. Don't confuse.
 */
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);
    return Imm32InternalLockIMC(hIMC, TRUE);
}

/***********************************************************************
*		ImmUnlockIMC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    PCLIENTIMC pClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    if (pClientImc->hInputContext)
        LocalUnlock(pClientImc->hInputContext);

    InterlockedDecrement(&pClientImc->cLockObj);
    ImmUnlockClientImc(pClientImc);
    return TRUE;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
    TRACE("(%p, %p)\n", hWnd, hIMC);
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hIMC);
    return TRUE; // Do nothing. This is correct.
}

/***********************************************************************
 *              ImmCreateSoftKeyboard(IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, UINT hOwner, int x, int y)
{
    FIXME("(%d, %d, %d, %d): stub\n", uType, hOwner, x, y);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              ImmDestroySoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftWnd)
{
    TRACE("(%p)\n", hSoftWnd);
    return DestroyWindow(hSoftWnd);
}

/***********************************************************************
 *              ImmShowSoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftWnd, int nCmdShow)
{
    TRACE("(%p, %d)\n", hSoftWnd, nCmdShow);
    if (hSoftWnd)
        return ShowWindow(hSoftWnd, nCmdShow);
    return FALSE;
}

/***********************************************************************
*		ImmDisableTextFrameService(IMM32.@)
*/
BOOL WINAPI ImmDisableTextFrameService(DWORD dwThreadId)
{
    FIXME("Stub\n");
    return FALSE;
}

/***********************************************************************
 *              ImmEnumInputContext(IMM32.@)
 */
BOOL WINAPI ImmEnumInputContext(DWORD dwThreadId, IMCENUMPROC lpfn, LPARAM lParam)
{
    HIMC *phList;
    DWORD dwIndex, dwCount;
    BOOL ret = TRUE;
    HIMC hIMC;

    TRACE("(%lu, %p, %p)\n", dwThreadId, lpfn, lParam);

    dwCount = Imm32BuildHimcList(dwThreadId, &phList);
    if (!dwCount)
        return FALSE;

    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
        hIMC = phList[dwIndex];
        ret = (*lpfn)(hIMC, lParam);
        if (!ret)
            break;
    }

    ImmLocalFree(phList);
    return ret;
}

/***********************************************************************
 *              ImmSetActiveContext(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContext(HWND hWnd, HIMC hIMC, BOOL fActive)
{
    PCLIENTIMC pClientImc;
    LPINPUTCONTEXTDX pIC;
    PIMEDPI pImeDpi;
    HKL hKL;
    BOOL fOpen = FALSE;
    DWORD dwConversion = 0, iShow = ISC_SHOWUIALL;
    HWND hwndDefIME;

    TRACE("(%p, %p, %d)\n", hWnd, hIMC, fActive);

    if (!IS_IMM_MODE())
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);

    if (!fActive)
    {
        if (pClientImc)
            pClientImc->dwFlags &= ~CLIENTIMC_UNKNOWN4;
    }
    else if (hIMC)
    {
        if (!pClientImc)
            return FALSE;

        pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        if (!pIC)
        {
            ImmUnlockClientImc(pClientImc);
            return FALSE;
        }

        pIC->hWnd = hWnd;
        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN5;

        if (pIC->dwUIFlags & 2)
            iShow = (ISC_SHOWUIGUIDELINE | ISC_SHOWUIALLCANDIDATEWINDOW);

        fOpen = pIC->fOpen;
        dwConversion = pIC->fdwConversion;

        ImmUnlockIMC(hIMC);
    }
    else
    {
        hIMC = ImmGetSaveContext(hWnd, 1);
        pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        if (pIC)
        {
            pIC->hWnd = hWnd;
            ImmUnlockIMC(hIMC);
        }
        hIMC = NULL;
    }

    hKL = GetKeyboardLayout(0);

    if (Imm32IsCiceroMode() && !Imm32Is16BitMode())
    {
        Imm32CiceroSetActiveContext(hIMC, fActive, hWnd, hKL);
        hKL = GetKeyboardLayout(0);
    }

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi)
    {
        if (IS_IME_HKL(hKL))
            pImeDpi->ImeSetActiveContext(hIMC, fActive);
        ImmUnlockImeDpi(pImeDpi);
    }

    if (IsWindow(hWnd))
    {
        SendMessageW(hWnd, WM_IME_SETCONTEXT, fActive, iShow);
        if (fActive)
            NtUserNotifyIMEStatus(hWnd, fOpen, dwConversion);
    }
    else if (!fActive)
    {
        hwndDefIME = ImmGetDefaultIMEWnd(NULL);
        if (hwndDefIME)
            SendMessageW(hwndDefIME, WM_IME_SETCONTEXT, 0, iShow);
    }

    if (pClientImc)
        ImmUnlockClientImc(pClientImc);

    return TRUE;
}

/***********************************************************************
 *              ImmWINNLSGetEnableStatus (IMM32.@)
 */

BOOL WINAPI ImmWINNLSGetEnableStatus(HWND hWnd)
{
    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    return !!ImmGetSaveContext(hWnd, 2);
}

/***********************************************************************
 *              ImmSetActiveContextConsoleIME(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContextConsoleIME(HWND hwnd, BOOL fFlag)
{
    HIMC hIMC;
    TRACE("(%p, %d)\n", hwnd, fFlag);

    hIMC = ImmGetContext(hwnd);
    if (hIMC)
        return ImmSetActiveContext(hwnd, hIMC, fFlag);
    return FALSE;
}

BOOL WINAPI User32InitializeImmEntryTable(DWORD);

// Win: ImmDllInitialize
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    HKL hKL;
    HIMC hIMC;

    TRACE("(%p, 0x%X, %p)\n", hinstDLL, fdwReason, lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!ImmInitializeGlobals(hinstDLL))
            {
                ERR("ImmInitializeGlobals failed\n");
                return FALSE;
            }
            if (!User32InitializeImmEntryTable(IMM_INIT_MAGIC))
            {
                ERR("User32InitializeImmEntryTable failed\n");
                return FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            if (!IS_IMM_MODE() || NtCurrentTeb()->Win32ThreadInfo == NULL)
                return TRUE;

            hKL = GetKeyboardLayout(0);
            hIMC = (HIMC)NtUserGetThreadState(THREADSTATE_DEFAULTINPUTCONTEXT);
            Imm32DestroyInputContext(hIMC, hKL, TRUE);
            break;

        case DLL_PROCESS_DETACH:
            RtlDeleteCriticalSection(&gcsImeDpi);
            TRACE("imm32.dll is unloaded\n");
            break;
    }

    return TRUE;
}
