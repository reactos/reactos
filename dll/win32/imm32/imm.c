/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing Far-Eastern languages input
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

HMODULE g_hImm32Inst = NULL;
PSERVERINFO g_psi = NULL;
SHAREDINFO g_SharedInfo = { NULL };
BYTE g_bClientRegd = FALSE;

static BOOL APIENTRY Imm32InitInstance(HMODULE hMod)
{
    NTSTATUS status;

    if (hMod)
        g_hImm32Inst = hMod;

    if (g_bClientRegd)
        return TRUE;

    status = RtlInitializeCriticalSection(&g_csImeDpi);
    if (NT_ERROR(status))
        return FALSE;

    g_bClientRegd = TRUE;
    return TRUE;
}

/***********************************************************************
 *		ImmRegisterClient(IMM32.@)
 *       ( Undocumented, called from user32.dll )
 */
BOOL WINAPI ImmRegisterClient(PSHAREDINFO ptr, HINSTANCE hMod)
{
    g_SharedInfo = *ptr;
    g_psi = g_SharedInfo.psi;
    return Imm32InitInstance(hMod);
}

/***********************************************************************
 *		ImmLoadLayout (IMM32.@)
 */
HKL WINAPI ImmLoadLayout(HKL hKL, PIMEINFOEX pImeInfoEx)
{
    DWORD cbData;
    UNICODE_STRING UnicodeString;
    HKEY hLayoutKey = NULL, hLayoutsKey = NULL;
    LONG error;
    NTSTATUS Status;
    WCHAR szLayout[MAX_PATH];

    TRACE("(%p, %p)\n", hKL, pImeInfoEx);

    if (IS_IME_HKL(hKL) || !Imm32IsCiceroMode() || Imm32Is16BitMode())
    {
        UnicodeString.Buffer = szLayout;
        UnicodeString.MaximumLength = sizeof(szLayout);
        Status = RtlIntegerToUnicodeString((DWORD_PTR)hKL, 16, &UnicodeString);
        if (!NT_SUCCESS(Status))
            return NULL;

        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hLayoutsKey);
        if (error)
            return NULL;

        error = RegOpenKeyW(hLayoutsKey, szLayout, &hLayoutKey);
    }
    else
    {
        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_IMM, &hLayoutKey);
    }

    if (error)
    {
        ERR("RegOpenKeyW error: 0x%08lX\n", error);
        hKL = NULL;
    }
    else
    {
        cbData = sizeof(pImeInfoEx->wszImeFile);
        error = RegQueryValueExW(hLayoutKey, L"Ime File", 0, 0,
                                 (LPBYTE)pImeInfoEx->wszImeFile, &cbData);
        if (error)
            hKL = NULL;
    }

    RegCloseKey(hLayoutKey);
    if (hLayoutsKey)
        RegCloseKey(hLayoutsKey);
    return hKL;
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
            pList = Imm32HeapAlloc(0, cKLs * sizeof(HKL));
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

            Imm32HeapFree(pList);
        }

        StringCchPrintfW(szKBD, _countof(szKBD), L"%08X", LangID);
        if (!LoadKeyboardLayoutW(szKBD, KLF_ACTIVATE))
            LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | 0x200);
    }
    else if (dwUnknown == 2)
    {
        RtlEnterCriticalSection(&g_csImeDpi);
Retry:
        for (pImeDpi = g_pImeDpiList; pImeDpi; pImeDpi = pImeDpi->pNext)
        {
            if (Imm32ReleaseIME(pImeDpi->hKL))
                goto Retry;
        }
        RtlLeaveCriticalSection(&g_csImeDpi);
    }
    else
    {
        hNewKL = (HKL)(DWORD_PTR)dwUnknown;
        if (IS_IME_HKL(hNewKL) && hNewKL != hOldKL)
            Imm32ReleaseIME(hNewKL);
    }

    return TRUE;
}

VOID APIENTRY Imm32SelectLayout(HKL hNewKL, HKL hOldKL, HIMC hIMC)
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
        if (Imm32IsImmMode() && !Imm32IsCiceroMode())
        {
            bIsNewHKLIme = IS_IME_HKL(hNewKL);
            bIsOldHKLIme = IS_IME_HKL(hOldKL);
        }
    }

    pIC = (LPINPUTCONTEXTDX)Imm32LockIMCEx(hIMC, FALSE);
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

static BOOL CALLBACK Imm32SelectLayoutProc(HIMC hIMC, LPARAM lParam)
{
    LPSELECT_LAYOUT pSelect = (LPSELECT_LAYOUT)lParam;
    Imm32SelectLayout(pSelect->hNewKL, pSelect->hOldKL, hIMC);
    return TRUE;
}

static BOOL CALLBACK Imm32NotifyCompStrProc(HIMC hIMC, LPARAM lParam)
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

            ImmEnumInputContext(0, Imm32NotifyCompStrProc, lParam);
        }

        hwndDefIME = ImmGetDefaultIMEWnd(NULL);
        if (IsWindow(hwndDefIME))
            SendMessageW(hwndDefIME, WM_IME_SELECT, FALSE, (LPARAM)hOldKL);

        NtUserSetThreadLayoutHandles(hKL, hOldKL);
    }

    SelectLayout.hNewKL = hKL;
    SelectLayout.hOldKL = hOldKL;
    ImmEnumInputContext(0, Imm32SelectLayoutProc, (LPARAM)&SelectLayout);

    if (IsWindow(hwndDefIME))
        SendMessageW(hwndDefIME, WM_IME_SELECT, TRUE, (LPARAM)hKL);

    return TRUE;
}

typedef struct _tagImmHkl
{
    struct list entry;
    HKL         hkl;
    HMODULE     hIME;
    IMEINFO     imeInfo;
    WCHAR       imeClassName[17]; /* 16 character max */
    ULONG       uSelected;
    HWND        UIWnd;

    /* Function Pointers */
    BOOL (WINAPI *pImeInquire)(IMEINFO *, WCHAR *, const WCHAR *);
    BOOL (WINAPI *pImeConfigure)(HKL, HWND, DWORD, void *);
    BOOL (WINAPI *pImeDestroy)(UINT);
    LRESULT (WINAPI *pImeEscape)(HIMC, UINT, void *);
    BOOL (WINAPI *pImeSelect)(HIMC, BOOL);
    BOOL (WINAPI *pImeSetActiveContext)(HIMC, BOOL);
    UINT (WINAPI *pImeToAsciiEx)(UINT, UINT, const BYTE *, DWORD *, UINT, HIMC);
    BOOL (WINAPI *pNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI *pImeRegisterWord)(const WCHAR *, DWORD, const WCHAR *);
    BOOL (WINAPI *pImeUnregisterWord)(const WCHAR *, DWORD, const WCHAR *);
    UINT (WINAPI *pImeEnumRegisterWord)(REGISTERWORDENUMPROCW, const WCHAR *, DWORD, const WCHAR *, void *);
    BOOL (WINAPI *pImeSetCompositionString)(HIMC, DWORD, const void *, DWORD, const void *, DWORD);
    DWORD (WINAPI *pImeConversionList)(HIMC, const WCHAR *, CANDIDATELIST *, DWORD, UINT);
    BOOL (WINAPI *pImeProcessKey)(HIMC, UINT, LPARAM, const BYTE *);
    UINT (WINAPI *pImeGetRegisterWordStyle)(UINT, STYLEBUFW *);
    DWORD (WINAPI *pImeGetImeMenuItems)(HIMC, DWORD, DWORD, IMEMENUITEMINFOW *, IMEMENUITEMINFOW *, DWORD);
} ImmHkl;

typedef struct tagInputContextData
{
    DWORD           dwLock;
    INPUTCONTEXT    IMC;
    DWORD           threadID;

    ImmHkl          *immKbd;
    UINT            lastVK;
    BOOL            threadDefault;
    DWORD           magic;
} InputContextData;

#define WINE_IMC_VALID_MAGIC 0x56434D49

static const WCHAR szwWineIMCProperty[] = {'W','i','n','e','I','m','m','H','I','M','C','P','r','o','p','e','r','t','y',0};
static const WCHAR szImeFileW[] = {'I','m','e',' ','F','i','l','e',0};
static const WCHAR szLayoutTextW[] = {'L','a','y','o','u','t',' ','T','e','x','t',0};
static const WCHAR szImeRegFmt[] = {'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','s','\\','%','0','8','l','x',0};

static inline BOOL is_himc_ime_unicode(const InputContextData *data)
{
    return !!(data->immKbd->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static InputContextData* get_imc_data(HIMC hIMC)
{
    InputContextData *data = (InputContextData *)hIMC;

    if (hIMC == NULL)
        return NULL;

    if(IsBadReadPtr(data, sizeof(InputContextData)) || data->magic != WINE_IMC_VALID_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return data;
}

static VOID APIENTRY Imm32CiceroSetActiveContext(HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL)
{
    FIXME("We have to do something\n");
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

    if (!Imm32IsImmMode())
        return NULL;

    pWnd = ValidateHwndNoErr(hWnd);
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

    if (!Imm32IsImmMode())
        return FALSE;

    if (hIMC && !(dwFlags & IACE_DEFAULT) && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hwndFocus = (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_FOCUS);
    pFocusWnd = ValidateHwndNoErr(hwndFocus);
    if (pFocusWnd)
        hOldIMC = pFocusWnd->hImc;

    dwValue = NtUserAssociateInputContext(hWnd, hIMC, dwFlags);
    switch (dwValue)
    {
        case 0:
            return TRUE;

        case 1:
            pFocusWnd = ValidateHwndNoErr(hwndFocus);
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

    if (!Imm32IsImmMode())
        return NULL;

    pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
    if (pClientImc == NULL)
        return NULL;

    hIMC = NtUserCreateInputContext(pClientImc);
    if (hIMC == NULL)
    {
        Imm32HeapFree(pClientImc);
        return NULL;
    }

    RtlInitializeCriticalSection(&pClientImc->cs);

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    pClientImc->unknown = NtUserGetThreadState(13);

    return hIMC;
}

static VOID APIENTRY Imm32FreeImeStates(LPINPUTCONTEXTDX pIC)
{
    PIME_STATE pState, pStateNext;
    PIME_SUBSTATE pSubState, pSubStateNext;

    pState = pIC->pState;
    pIC->pState = NULL;
    for (; pState; pState = pStateNext)
    {
        pStateNext = pState->pNext;
        for (pSubState = pState->pSubState; pSubState; pSubState = pSubStateNext)
        {
            pSubStateNext = pSubState->pNext;
            Imm32HeapFree(pSubState);
        }
        Imm32HeapFree(pState);
    }
}

BOOL APIENTRY Imm32CleanupContext(HIMC hIMC, HKL hKL, BOOL bKeep)
{
    PIMEDPI pImeDpi;
    LPINPUTCONTEXTDX pIC;
    PCLIENTIMC pClientImc;
    PIMC pIMC;

    if (!Imm32IsImmMode() || hIMC == NULL)
        return FALSE;

    pIMC = ValidateHandleNoErr(hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC || pIMC->head.pti != NtCurrentTeb()->Win32ThreadInfo)
        return FALSE;

    pClientImc = (PCLIENTIMC)pIMC->dwClientImcData;
    if (!pClientImc)
        return FALSE;

    if (pClientImc->hInputContext == NULL)
    {
        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
        ImmUnlockClientImc(pClientImc);
        if (!bKeep)
            return NtUserDestroyInputContext(hIMC);
        return TRUE;
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return FALSE;
    }

    FIXME("We have do something to do here\n");

    if (pClientImc->hKL == hKL)
    {
        pImeDpi = ImmLockImeDpi(hKL);
        if (pImeDpi != NULL)
        {
            if (IS_IME_HKL(hKL))
            {
                pImeDpi->ImeSelect(hIMC, FALSE);
            }
            else if (Imm32IsCiceroMode() && pImeDpi->CtfImeSelectEx)
            {
                pImeDpi->CtfImeSelectEx(hIMC, FALSE, hKL);
            }
            ImmUnlockImeDpi(pImeDpi);
        }
        pClientImc->hKL = NULL;
    }

    pIC->hPrivate = ImmDestroyIMCC(pIC->hPrivate);
    pIC->hMsgBuf = ImmDestroyIMCC(pIC->hMsgBuf);
    pIC->hGuideLine = ImmDestroyIMCC(pIC->hGuideLine);
    pIC->hCandInfo = ImmDestroyIMCC(pIC->hCandInfo);
    pIC->hCompStr = ImmDestroyIMCC(pIC->hCompStr);

    Imm32FreeImeStates(pIC);

    ImmUnlockIMC(hIMC);

    pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
    ImmUnlockClientImc(pClientImc);

    if (!bKeep)
        return NtUserDestroyInputContext(hIMC);

    return TRUE;
}

BOOL APIENTRY
Imm32InitContext(HIMC hIMC, LPINPUTCONTEXT pIC, PCLIENTIMC pClientImc, HKL hKL, BOOL fSelect)
{
    DWORD dwIndex, cbPrivate;
    PIMEDPI pImeDpi = NULL;
    LPCOMPOSITIONSTRING pCS;
    LPCANDIDATEINFO pCI;
    LPGUIDELINE pGL;
    /* NOTE: Windows does recursive call ImmLockIMC here but we don't do so. */

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

LPINPUTCONTEXT APIENTRY Imm32LockIMCEx(HIMC hIMC, BOOL fSelect)
{
    HANDLE hIC;
    LPINPUTCONTEXT pIC = NULL;
    PCLIENTIMC pClientImc;
    WORD Word;
    DWORD dwThreadId;
    HKL hKL, hNewKL;
    PIMEDPI pImeDpi = NULL;
    BOOL bInited;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return NULL;

    RtlEnterCriticalSection(&pClientImc->cs);

    if (!pClientImc->hInputContext)
    {
        dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, 1);

        if (dwThreadId == GetCurrentThreadId() && Imm32IsCiceroMode() && !Imm32Is16BitMode())
        {
            hKL = GetKeyboardLayout(0);
            Word = LOWORD(hKL);
            hNewKL = (HKL)(DWORD_PTR)MAKELONG(Word, Word);

            pImeDpi = ImmLockOrLoadImeDpi(hNewKL);
            if (pImeDpi)
            {
                FIXME("We have to do something here\n");
            }
        }

        if (!NtUserQueryInputContext(hIMC, 2))
        {
            RtlLeaveCriticalSection(&pClientImc->cs);
            goto Quit;
        }

        hIC = LocalAlloc(LHND, sizeof(INPUTCONTEXTDX));
        if (!hIC)
        {
            RtlLeaveCriticalSection(&pClientImc->cs);
            goto Quit;
        }
        pClientImc->hInputContext = hIC;

        pIC = LocalLock(pClientImc->hInputContext);
        if (!pIC)
        {
            pClientImc->hInputContext = LocalFree(pClientImc->hInputContext);
            RtlLeaveCriticalSection(&pClientImc->cs);
            goto Quit;
        }

        hKL = GetKeyboardLayout(dwThreadId);
        // bInited = Imm32InitContext(hIMC, hKL, fSelect);
        bInited = Imm32InitContext(hIMC, pIC, pClientImc, hKL, fSelect);
        LocalUnlock(pClientImc->hInputContext);

        if (!bInited)
        {
            pIC = NULL;
            pClientImc->hInputContext = LocalFree(pClientImc->hInputContext);
            RtlLeaveCriticalSection(&pClientImc->cs);
            goto Quit;
        }
    }

    FIXME("We have to do something here\n");

    RtlLeaveCriticalSection(&pClientImc->cs);
    pIC = LocalLock(pClientImc->hInputContext);
    InterlockedIncrement(&pClientImc->cLockObj);

Quit:
    ImmUnlockClientImc(pClientImc);
    return pIC;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    HKL hKL;

    TRACE("(%p)\n", hIMC);

    if (!Imm32IsImmMode())
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    return Imm32CleanupContext(hIMC, hKL, FALSE);
}

/***********************************************************************
 *		ImmLockClientImc (IMM32.@)
 */
PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc)
{
    PIMC pIMC;
    PCLIENTIMC pClientImc;

    TRACE("(%p)\n", hImc);

    if (hImc == NULL)
        return NULL;

    pIMC = ValidateHandleNoErr(hImc, TYPE_INPUTCONTEXT);
    if (pIMC == NULL || !Imm32CheckImcProcess(pIMC))
        return NULL;

    pClientImc = (PCLIENTIMC)pIMC->dwClientImcData;
    if (!pClientImc)
    {
        pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
        if (!pClientImc)
            return NULL;

        RtlInitializeCriticalSection(&pClientImc->cs);

        // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
        pClientImc->unknown = NtUserGetThreadState(13);

        if (!NtUserUpdateInputContext(hImc, 0, pClientImc))
        {
            Imm32HeapFree(pClientImc);
            return NULL;
        }

        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN2;
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_UNKNOWN1)
            return NULL;
    }

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
    if (cLocks != 0 || !(pClientImc->dwFlags & CLIENTIMC_UNKNOWN1))
        return;

    hInputContext = pClientImc->hInputContext;
    if (hInputContext)
        LocalFree(hInputContext);

    RtlDeleteCriticalSection(&pClientImc->cs);
    Imm32HeapFree(pClientImc);
}

static HIMC APIENTRY Imm32GetContextEx(HWND hWnd, DWORD dwContextFlags)
{
    HIMC hIMC;
    PCLIENTIMC pClientImc;
    PWND pWnd;

    if (!Imm32IsImmMode())
        return NULL;

    if (!hWnd)
    {
        // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
        hIMC = (HIMC)NtUserGetThreadState(4);
        goto Quit;
    }

    pWnd = ValidateHwndNoErr(hWnd);
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


/* Helpers for the GetCompositionString functions */

/* Source encoding is defined by context, source length is always given in respective characters. Destination buffer
   length is always in bytes. */
static INT
CopyCompStringIMEtoClient(const InputContextData *data, const void *src, INT src_len, void *dst,
                          INT dst_len, BOOL unicode)
{
    int char_size = unicode ? sizeof(WCHAR) : sizeof(char);
    INT ret;

    if (is_himc_ime_unicode(data) ^ unicode)
    {
        if (unicode)
            ret = MultiByteToWideChar(CP_ACP, 0, src, src_len, dst, dst_len / sizeof(WCHAR));
        else
            ret = WideCharToMultiByte(CP_ACP, 0, src, src_len, dst, dst_len, NULL, NULL);
        ret *= char_size;
    }
    else
    {
        if (dst_len)
        {
            ret = min(src_len * char_size, dst_len);
            memcpy(dst, src, ret);
        }
        else
            ret = src_len * char_size;
    }

    return ret;
}

/* Composition string encoding is defined by context, returned attributes correspond to string, converted according to
   passed mode. String length is in characters, attributes are in byte arrays. */
static INT
CopyCompAttrIMEtoClient(const InputContextData *data, const BYTE *src, INT src_len, const void *comp_string,
                        INT str_len, BYTE *dst, INT dst_len, BOOL unicode)
{
    union
    {
        const void *str;
        const WCHAR *strW;
        const char *strA;
    } string;
    INT rc;

    string.str = comp_string;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, string.strW, str_len, NULL, 0, NULL, NULL);
        if (dst_len)
        {
            int i, j = 0, k = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, string.strW + i, 1, NULL, 0, NULL, NULL);
                for (; len > 0; --len)
                {
                    dst[j++] = src[k];

                    if (j >= dst_len)
                        goto end;
                }
                ++k;
            }
        end:
            rc = j;
        }
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, string.strA, str_len, NULL, 0);
        if (dst_len)
        {
            int i, j = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                if (IsDBCSLeadByte(string.strA[i]))
                    continue;

                dst[j++] = src[i];

                if (j >= dst_len)
                    break;
            }
            rc = j;
        }
    }
    else
    {
        memcpy(dst, src, min(src_len, dst_len));
        rc = src_len;
    }

    return rc;
}

static INT
CopyCompClauseIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource,
                          LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0,
                                                          NULL, NULL);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT
CopyCompOffsetIMEtoClient(InputContextData *data, DWORD offset, LPBYTE ssource, BOOL unicode)
{
    int rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, offset, NULL, 0);
    }
    else
        rc = offset;

    return rc;
}

static LONG
ImmGetCompositionStringT(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf,
                         DWORD dwBufLen, BOOL unicode)
{
    LONG rc = 0;
    InputContextData *data = get_imc_data(hIMC);
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
        break;
    default:
        FIXME("Unhandled index 0x%x\n",dwIndex);
        break;
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    TRACE("(%p)\n", hWnd);
    if (hWnd == NULL)
        return NULL;
    return Imm32GetContextEx(hWnd, 2);
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(VOID)
{
    return Imm32IsCiceroMode();
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
    HKL hKL = NULL;
    LPWSTR pszFileNameW = NULL, pszLayoutTextW = NULL;

    TRACE("(%s, %s)\n", debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText));

    pszFileNameW = Imm32WideFromAnsi(lpszIMEFileName);
    if (pszFileNameW == NULL)
        goto Quit;

    pszLayoutTextW = Imm32WideFromAnsi(lpszLayoutText);
    if (pszLayoutTextW == NULL)
        goto Quit;

    hKL = ImmInstallIMEW(pszFileNameW, pszLayoutTextW);

Quit:
    Imm32HeapFree(pszFileNameW);
    Imm32HeapFree(pszLayoutTextW);
    return hKL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
    INT lcid = GetUserDefaultLCID();
    INT count;
    HKL hkl;
    DWORD rc;
    HKEY hkey;
    WCHAR regKey[ARRAY_SIZE(szImeRegFmt)+8];

    TRACE ("(%s, %s):\n", debugstr_w(lpszIMEFileName),
                          debugstr_w(lpszLayoutText));

    /* Start with 2.  e001 will be blank and so default to the wine internal IME */
    count = 2;

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)MAKELPARAM( lcid, 0xe000 | count );
        wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hkl);

        rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &disposition);
        if (rc == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
            break;
        else if (rc == ERROR_SUCCESS)
            RegCloseKey(hkey);

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(hkey, szImeFileW, 0, REG_SZ, (const BYTE*)lpszIMEFileName,
                            (lstrlenW(lpszIMEFileName) + 1) * sizeof(WCHAR));
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(hkey, szLayoutTextW, 0, REG_SZ, (const BYTE*)lpszLayoutText,
                                (lstrlenW(lpszLayoutText) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
        return hkl;
    }
    else
    {
        WARN("Unable to set IME registry values\n");
        return 0;
    }
}

/***********************************************************************
 *		ImmLockIMC(IMM32.@)
 *
 * NOTE: This is not ImmLockIMCC. Don't confuse.
 */
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    TRACE("(%p)\n", hIMC);
    return Imm32LockIMCEx(hIMC, TRUE);
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
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringA(HIMC hIMC, DWORD dwIndex,
                         LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    WCHAR *CompBuffer = NULL;
    WCHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringW(HIMC hIMC, DWORD dwIndex,
                         LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    CHAR *CompBuffer = NULL;
    CHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len);
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len);
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
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

    dwCount = Imm32AllocAndBuildHimcList(dwThreadId, &phList);
    if (!dwCount)
        return FALSE;

    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
        hIMC = phList[dwIndex];
        ret = (*lpfn)(hIMC, lParam);
        if (!ret)
            break;
    }

    Imm32HeapFree(phList);
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

    if (!Imm32IsImmMode())
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
        hIMC = Imm32GetContextEx(hWnd, TRUE);
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

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    HKL hKL;
    HIMC hIMC;
    PTEB pTeb;

    TRACE("(%p, 0x%X, %p)\n", hinstDLL, fdwReason, lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!Imm32InitInstance(hinstDLL))
            {
                ERR("Imm32InitInstance failed\n");
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
            if (!Imm32IsImmMode())
                return TRUE;

            pTeb = NtCurrentTeb();
            if (pTeb->Win32ThreadInfo == NULL)
                return TRUE;

            hKL = GetKeyboardLayout(0);
            // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
            hIMC = (HIMC)NtUserGetThreadState(4);
            Imm32CleanupContext(hIMC, hKL, TRUE);
            break;

        case DLL_PROCESS_DETACH:
            RtlDeleteCriticalSection(&g_csImeDpi);
            TRACE("imm32.dll is unloaded\n");
            break;
    }

    return TRUE;
}
