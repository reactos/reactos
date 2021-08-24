/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME-related items of IMM32
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

RTL_CRITICAL_SECTION g_csImeDpi;
PIMEDPI g_pImeDpiList = NULL;

static BOOL APIENTRY Imm32InquireIme(PIMEDPI pImeDpi)
{
    WCHAR szUIClass[64];
    WNDCLASSW wcW;
    DWORD dwSysInfoFlags = 0; // TODO: ???
    LPIMEINFO pImeInfo = &pImeDpi->ImeInfo;

    // TODO: NtUserGetThreadState(16);

    if (!IS_IME_HKL(pImeDpi->hKL))
    {
        if (IS_CICERO_ENABLED() && pImeDpi->CtfImeInquireExW)
        {
            // TODO:
            return FALSE;
        }
    }

    if (!pImeDpi->ImeInquire(pImeInfo, szUIClass, dwSysInfoFlags))
        return FALSE;

    szUIClass[_countof(szUIClass) - 1] = 0;

    if (pImeInfo->dwPrivateDataSize == 0)
        pImeInfo->dwPrivateDataSize = 4;

#define VALID_IME_PROP (IME_PROP_AT_CARET              | \
                        IME_PROP_SPECIAL_UI            | \
                        IME_PROP_CANDLIST_START_FROM_1 | \
                        IME_PROP_UNICODE               | \
                        IME_PROP_COMPLETE_ON_UNSELECT  | \
                        IME_PROP_END_UNLOAD            | \
                        IME_PROP_KBD_CHAR_FIRST        | \
                        IME_PROP_IGNORE_UPKEYS         | \
                        IME_PROP_NEED_ALTKEY           | \
                        IME_PROP_NO_KEYS_ON_CLOSE      | \
                        IME_PROP_ACCEPT_WIDE_VKEY)
#define VALID_CMODE_CAPS (IME_CMODE_ALPHANUMERIC | \
                          IME_CMODE_NATIVE       | \
                          IME_CMODE_KATAKANA     | \
                          IME_CMODE_LANGUAGE     | \
                          IME_CMODE_FULLSHAPE    | \
                          IME_CMODE_ROMAN        | \
                          IME_CMODE_CHARCODE     | \
                          IME_CMODE_HANJACONVERT | \
                          IME_CMODE_SOFTKBD      | \
                          IME_CMODE_NOCONVERSION | \
                          IME_CMODE_EUDC         | \
                          IME_CMODE_SYMBOL       | \
                          IME_CMODE_FIXED)
#define VALID_SMODE_CAPS (IME_SMODE_NONE          | \
                          IME_SMODE_PLAURALCLAUSE | \
                          IME_SMODE_SINGLECONVERT | \
                          IME_SMODE_AUTOMATIC     | \
                          IME_SMODE_PHRASEPREDICT | \
                          IME_SMODE_CONVERSATION)
#define VALID_UI_CAPS (UI_CAP_2700    | \
                       UI_CAP_ROT90   | \
                       UI_CAP_ROTANY  | \
                       UI_CAP_SOFTKBD)
#define VALID_SCS_CAPS (SCS_CAP_COMPSTR            | \
                        SCS_CAP_MAKEREAD           | \
                        SCS_CAP_SETRECONVERTSTRING)
#define VALID_SELECT_CAPS (SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE)

    if (pImeInfo->fdwProperty & ~VALID_IME_PROP)
        return FALSE;
    if (pImeInfo->fdwConversionCaps & ~VALID_CMODE_CAPS)
        return FALSE;
    if (pImeInfo->fdwSentenceCaps & ~VALID_SMODE_CAPS)
        return FALSE;
    if (pImeInfo->fdwUICaps & ~VALID_UI_CAPS)
        return FALSE;
    if (pImeInfo->fdwSCSCaps & ~VALID_SCS_CAPS)
        return FALSE;
    if (pImeInfo->fdwSelectCaps & ~VALID_SELECT_CAPS)
        return FALSE;

#undef VALID_IME_PROP
#undef VALID_CMODE_CAPS
#undef VALID_SMODE_CAPS
#undef VALID_UI_CAPS
#undef VALID_SCS_CAPS
#undef VALID_SELECT_CAPS

    if (pImeInfo->fdwProperty & IME_PROP_UNICODE)
    {
        StringCchCopyW(pImeDpi->szUIClass, _countof(pImeDpi->szUIClass), szUIClass);
    }
    else
    {
        if (pImeDpi->uCodePage != GetACP() && pImeDpi->uCodePage)
            return FALSE;

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR)szUIClass, -1,
                            pImeDpi->szUIClass, _countof(pImeDpi->szUIClass));
    }

    return GetClassInfoW(pImeDpi->hInst, pImeDpi->szUIClass, &wcW);
}

static BOOL APIENTRY Imm32LoadImeInfo(PIMEINFOEX pImeInfoEx, PIMEDPI pImeDpi)
{
    WCHAR szPath[MAX_PATH];
    HINSTANCE hIME;
    FARPROC fn;

    if (!Imm32GetSystemLibraryPath(szPath, _countof(szPath), pImeInfoEx->wszImeFile))
        return FALSE;

    hIME = GetModuleHandleW(szPath);
    if (hIME == NULL)
    {
        hIME = LoadLibraryW(szPath);
        if (hIME == NULL)
        {
            ERR("Imm32LoadImeInfo: LoadLibraryW(%S) failed\n", szPath);
            return FALSE;
        }
    }
    pImeDpi->hInst = hIME;

#define DEFINE_IME_ENTRY(type, name, params, extended) \
    do { \
        fn = GetProcAddress(hIME, #name); \
        if (fn) pImeDpi->name = (FN_##name)fn; \
        else if (!extended) goto Failed; \
    } while (0);
#include "../../../win32ss/include/imetable.h"
#undef DEFINE_IME_ENTRY

    if (!Imm32InquireIme(pImeDpi))
    {
        ERR("Imm32LoadImeInfo: Imm32InquireIme failed\n");
        goto Failed;
    }

    if (pImeInfoEx->fLoadFlag)
        return TRUE;

    NtUserSetImeOwnerWindow(pImeInfoEx, TRUE);
    return TRUE;

Failed:
    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;
    return FALSE;
}

PIMEDPI APIENTRY Ime32LoadImeDpi(HKL hKL, BOOL bLock)
{
    IMEINFOEX ImeInfoEx;
    CHARSETINFO ci;
    PIMEDPI pImeDpiNew, pImeDpiFound;
    UINT uCodePage;
    LCID lcid;

    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL) ||
        ImeInfoEx.fLoadFlag == 1)
    {
        return NULL;
    }

    pImeDpiNew = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(IMEDPI));
    if (pImeDpiNew == NULL)
        return NULL;

    pImeDpiNew->hKL = hKL;

    lcid = LOWORD(hKL);
    if (TranslateCharsetInfo((LPDWORD)(DWORD_PTR)lcid, &ci, TCI_SRCLOCALE))
        uCodePage = ci.ciACP;
    else
        uCodePage = CP_ACP;
    pImeDpiNew->uCodePage = uCodePage;

    if (!Imm32LoadImeInfo(&ImeInfoEx, pImeDpiNew))
    {
        HeapFree(g_hImm32Heap, 0, pImeDpiNew);
        return FALSE;
    }

    RtlEnterCriticalSection(&g_csImeDpi);

    pImeDpiFound = Imm32FindImeDpi(hKL);
    if (pImeDpiFound)
    {
        if (!bLock)
            pImeDpiFound->dwFlags &= ~IMEDPI_FLAG_LOCKED;

        RtlLeaveCriticalSection(&g_csImeDpi);

        Imm32FreeImeDpi(pImeDpiNew, FALSE);
        HeapFree(g_hImm32Heap, 0, pImeDpiNew);
        return pImeDpiFound;
    }
    else
    {
        if (bLock)
        {
            pImeDpiNew->dwFlags |= IMEDPI_FLAG_LOCKED;
            pImeDpiNew->cLockObj = 1;
        }

        pImeDpiNew->pNext = g_pImeDpiList;
        g_pImeDpiList = pImeDpiNew;

        RtlLeaveCriticalSection(&g_csImeDpi);
        return pImeDpiNew;
    }
}

/***********************************************************************
 *		ImmLockImeDpi (IMM32.@)
 */
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi = NULL;

    TRACE("(%p)\n", hKL);

    RtlEnterCriticalSection(&g_csImeDpi);

    /* Find by hKL */
    for (pImeDpi = g_pImeDpiList; pImeDpi; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL) /* found */
        {
            /* lock if possible */
            if (pImeDpi->dwFlags & IMEDPI_FLAG_UNKNOWN)
                pImeDpi = NULL;
            else
                ++(pImeDpi->cLockObj);
            break;
        }
    }

    RtlLeaveCriticalSection(&g_csImeDpi);
    return pImeDpi;
}

/***********************************************************************
 *		ImmUnlockImeDpi (IMM32.@)
 */
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi)
{
    PIMEDPI *ppEntry;

    TRACE("(%p)\n", pImeDpi);

    if (pImeDpi == NULL)
        return;

    RtlEnterCriticalSection(&g_csImeDpi);

    /* unlock */
    --(pImeDpi->cLockObj);
    if (pImeDpi->cLockObj != 0)
    {
        RtlLeaveCriticalSection(&g_csImeDpi);
        return;
    }

    if ((pImeDpi->dwFlags & IMEDPI_FLAG_UNKNOWN) == 0)
    {
        if ((pImeDpi->dwFlags & IMEDPI_FLAG_LOCKED) == 0 ||
            (pImeDpi->ImeInfo.fdwProperty & IME_PROP_END_UNLOAD) == 0)
        {
            RtlLeaveCriticalSection(&g_csImeDpi);
            return;
        }
    }

    /* Remove from list */
    for (ppEntry = &g_pImeDpiList; *ppEntry; ppEntry = &((*ppEntry)->pNext))
    {
        if (*ppEntry == pImeDpi) /* found */
        {
            *ppEntry = pImeDpi->pNext;
            break;
        }
    }

    Imm32FreeImeDpi(pImeDpi, TRUE);
    HeapFree(g_hImm32Heap, 0, pImeDpi);

    RtlLeaveCriticalSection(&g_csImeDpi);
}

/***********************************************************************
 *		ImmDisableIME (IMM32.@)
 */
BOOL WINAPI ImmDisableIME(DWORD dwThreadId)
{
    return NtUserDisableThreadIme(dwThreadId);
}

/***********************************************************************
 *		ImmLoadIME (IMM32.@)
 */
BOOL WINAPI ImmLoadIME(HKL hKL)
{
    PW32CLIENTINFO pInfo;
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL))
    {
        if (!IS_CICERO_ENABLED())
            return FALSE;

        pInfo = (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo);
        if ((pInfo->W32ClientInfo[0] & 2))
            return FALSE;
    }

    pImeDpi = Imm32FindImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, FALSE);
    return (pImeDpi != NULL);
}

PIMEDPI APIENTRY ImmLockOrLoadImeDpi(HKL hKL)
{
    PW32CLIENTINFO pInfo;
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL))
    {
        if (!IS_CICERO_ENABLED())
            return NULL;

        pInfo = (PW32CLIENTINFO)(NtCurrentTeb()->Win32ClientInfo);
        if ((pInfo->W32ClientInfo[0] & 2))
            return NULL;
    }

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, TRUE);
    return pImeDpi;
}

PIMEDPI APIENTRY Imm32FindImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi;

    RtlEnterCriticalSection(&g_csImeDpi);
    for (pImeDpi = g_pImeDpiList; pImeDpi != NULL; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL)
            break;
    }
    RtlLeaveCriticalSection(&g_csImeDpi);

    return pImeDpi;
}

VOID APIENTRY Imm32FreeImeDpi(PIMEDPI pImeDpi, BOOL bDestroy)
{
    if (pImeDpi->hInst == NULL)
        return;
    if (bDestroy)
        pImeDpi->ImeDestroy(0);
    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;
}

/*
 * These functions absorb the difference between Ansi and Wide.
 */
typedef struct ENUM_WORD_A2W
{
    REGISTERWORDENUMPROCW lpfnEnumProc;
    LPVOID lpData;
    UINT ret;
} ENUM_WORD_A2W, *LPENUM_WORD_A2W;

typedef struct ENUM_WORD_W2A
{
    REGISTERWORDENUMPROCA lpfnEnumProc;
    LPVOID lpData;
    UINT ret;
} ENUM_WORD_W2A, *LPENUM_WORD_W2A;

static INT CALLBACK
Imm32EnumWordProcA2W(LPCSTR pszReadingA, DWORD dwStyle, LPCSTR pszRegisterA, LPVOID lpData)
{
    INT ret = 0;
    LPENUM_WORD_A2W lpEnumData = lpData;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;

    if (pszReadingA)
    {
        pszReadingW = Imm32WideFromAnsi(pszReadingA);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (pszRegisterA)
    {
        pszRegisterW = Imm32WideFromAnsi(pszRegisterA);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingW, dwStyle, pszRegisterW, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    return ret;
}

static INT CALLBACK
Imm32EnumWordProcW2A(LPCWSTR pszReadingW, DWORD dwStyle, LPCWSTR pszRegisterW, LPVOID lpData)
{
    INT ret = 0;
    LPENUM_WORD_W2A lpEnumData = lpData;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;

    if (pszReadingW)
    {
        pszReadingA = Imm32AnsiFromWide(pszReadingW);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (pszRegisterW)
    {
        pszRegisterA = Imm32AnsiFromWide(pszRegisterW);
        if (pszRegisterA == NULL)
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingA, dwStyle, pszRegisterA, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    return ret;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    IMEINFOEX info;
    TRACE("(%p)\n", hKL);
    return !!ImmGetImeInfoEx(&info, ImeInfoExImeWindow, &hKL);
}

/***********************************************************************
 *              ImmGetImeInfoEx (IMM32.@)
 */
BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey)
{
    BOOL bDisabled = FALSE;
    HKL hKL;
    PTEB pTeb;

    switch (SearchType)
    {
        case ImeInfoExKeyboardLayout:
            break;

        case ImeInfoExImeWindow:
            bDisabled = CtfImmIsTextFrameServiceDisabled();
            SearchType = ImeInfoExKeyboardLayout;
            break;

        case ImeInfoExImeFileName:
            StringCchCopyW(pImeInfoEx->wszImeFile, _countof(pImeInfoEx->wszImeFile),
                           pvSearchKey);
            goto Quit;
    }

    hKL = *(HKL*)pvSearchKey;
    pImeInfoEx->hkl = hKL;

    if (!IS_IME_HKL(hKL))
    {
        if (IS_CICERO_ENABLED())
        {
            pTeb = NtCurrentTeb();
            if (((PW32CLIENTINFO)pTeb->Win32ClientInfo)->W32ClientInfo[0] & 2)
                return FALSE;
            if (!bDisabled)
                goto Quit;
        }
        return FALSE;
    }

Quit:
    return NtUserGetImeInfoEx(pImeInfoEx, SearchType);
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    BOOL ret = FALSE;
    PWND pWnd;
    PIMEDPI pImeDpi;
    REGISTERWORDW RegWordW;
    LPREGISTERWORDA pRegWordA;

    TRACE("(%p, %p, %ld, %p)\n", hKL, hWnd, dwMode, lpData);

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd || Imm32IsCrossProcessAccess(hWnd))
        return FALSE;

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    RtlZeroMemory(&RegWordW, sizeof(RegWordW));

    if (!lpData || dwMode != IME_CONFIG_REGISTERWORD ||
        !(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        goto DoIt;
    }

    pRegWordA = lpData;

    if (pRegWordA->lpReading)
    {
        RegWordW.lpReading = Imm32WideFromAnsi(pRegWordA->lpReading);
        if (!RegWordW.lpReading)
            goto Quit;
    }

    if (pRegWordA->lpWord)
    {
        RegWordW.lpWord = Imm32WideFromAnsi(pRegWordA->lpWord);
        if (!RegWordW.lpWord)
            goto Quit;
    }

    lpData = &RegWordW;

DoIt:
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1B, 0);
    ret = pImeDpi->ImeConfigure(hKL, hWnd, dwMode, lpData);
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1A, 0);

Quit:
    if (RegWordW.lpReading)
        HeapFree(g_hImm32Heap, 0, RegWordW.lpReading);
    if (RegWordW.lpWord)
        HeapFree(g_hImm32Heap, 0, RegWordW.lpWord);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    BOOL ret = FALSE;
    PWND pWnd;
    PIMEDPI pImeDpi;
    REGISTERWORDA RegWordA;
    LPREGISTERWORDW pRegWordW;

    TRACE("(%p, %p, %ld, %p)\n", hKL, hWnd, dwMode, lpData);

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd || Imm32IsCrossProcessAccess(hWnd))
        return FALSE;

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    RtlZeroMemory(&RegWordA, sizeof(RegWordA));

    if (!lpData || dwMode != IME_CONFIG_REGISTERWORD ||
        (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        goto DoIt;
    }

    pRegWordW = lpData;

    if (pRegWordW->lpReading)
    {
        RegWordA.lpReading = Imm32AnsiFromWide(pRegWordW->lpReading);
        if (!RegWordA.lpReading)
            goto Quit;
    }

    if (pRegWordW->lpWord)
    {
        RegWordA.lpWord = Imm32AnsiFromWide(pRegWordW->lpWord);
        if (!RegWordA.lpWord)
            goto Quit;
    }

    lpData = &RegWordA;

DoIt:
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1B, 0);
    ret = pImeDpi->ImeConfigure(hKL, hWnd, dwMode, lpData);
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1A, 0);

Quit:
    if (RegWordA.lpReading)
        HeapFree(g_hImm32Heap, 0, RegWordA.lpReading);
    if (RegWordA.lpWord)
        HeapFree(g_hImm32Heap, 0, RegWordA.lpWord);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI
ImmEnumRegisterWordA(HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
                     LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister, LPVOID lpData)
{
    UINT ret = 0;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;
    ENUM_WORD_W2A EnumDataW2A;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)", hKL, lpfnEnumProc, debugstr_a(lpszReading),
          dwStyle, debugstr_a(lpszRegister), lpData);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(lpszRegister);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    EnumDataW2A.lpfnEnumProc = lpfnEnumProc;
    EnumDataW2A.lpData = lpData;
    EnumDataW2A.ret = 0;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcW2A, pszReadingW, dwStyle,
                                 pszRegisterW, &EnumDataW2A);
    ret = EnumDataW2A.ret;

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI
ImmEnumRegisterWordW(HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
                     LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister, LPVOID lpData)
{
    UINT ret = 0;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;
    ENUM_WORD_A2W EnumDataA2W;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)", hKL, lpfnEnumProc, debugstr_w(lpszReading),
          dwStyle, debugstr_w(lpszRegister), lpData);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (pszReadingA == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(lpszRegister);
        if (pszRegisterA == NULL)
            goto Quit;
    }

    EnumDataA2W.lpfnEnumProc = lpfnEnumProc;
    EnumDataA2W.lpData = lpData;
    EnumDataA2W.ret = 0;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcA2W, pszReadingA, dwStyle,
                                 pszRegisterA, &EnumDataA2W);
    ret = EnumDataA2W.ret;

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

DWORD APIENTRY Imm32AllocAndBuildHimcList(DWORD dwThreadId, HIMC **pphList)
{
#define INITIAL_COUNT 0x40
#define MAX_RETRY 10
    NTSTATUS Status;
    DWORD dwCount = INITIAL_COUNT, cRetry = 0;
    HIMC *phNewList;

    phNewList = Imm32HeapAlloc(0, dwCount * sizeof(HIMC));
    if (phNewList == NULL)
        return 0;

    Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    while (Status == STATUS_BUFFER_TOO_SMALL)
    {
        HeapFree(g_hImm32Heap, 0, phNewList);
        if (cRetry++ >= MAX_RETRY)
            return 0;

        phNewList = Imm32HeapAlloc(0, dwCount * sizeof(HIMC));
        if (phNewList == NULL)
            return 0;

        Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    }

    if (NT_ERROR(Status) || !dwCount)
    {
        HeapFree(g_hImm32Heap, 0, phNewList);
        return 0;
    }

    *pphList = phNewList;
    return dwCount;
#undef INITIAL_COUNT
#undef MAX_RETRY
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA(HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
    BOOL bDefUsed;
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %u)\n", hKL, lpszFileName, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
    {
        if (uBufLen > 0)
            lpszFileName[0] = 0;
        return 0;
    }

    StringCchLengthW(info.wszImeFile, _countof(info.wszImeFile), &cch);

    cch = WideCharToMultiByte(CP_ACP, 0, info.wszImeFile, (INT)cch,
                              lpszFileName, uBufLen, NULL, &bDefUsed);
    if (uBufLen == 0)
        return (UINT)cch;

    if (cch > uBufLen - 1)
        cch = uBufLen - 1;

    lpszFileName[cch] = 0;
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %u)\n", hKL, lpszFileName, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
    {
        if (uBufLen > 0)
            lpszFileName[0] = 0;
        return 0;
    }

    StringCchLengthW(info.wszImeFile, _countof(info.wszImeFile), &cch);
    if (uBufLen == 0)
        return (UINT)cch;

    StringCchCopyNW(lpszFileName, uBufLen, info.wszImeFile, cch);

    if (cch > uBufLen - 1)
        cch = uBufLen - 1;

    lpszFileName[cch] = 0;
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p,%p,%d)\n", hKL, lpszDescription, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
        return 0;

    StringCchLengthW(info.wszImeDescription, _countof(info.wszImeDescription), &cch);
    cch = WideCharToMultiByte(CP_ACP, 0, info.wszImeDescription, (INT)cch,
                              lpszDescription, uBufLen, NULL, NULL);
    if (uBufLen)
        lpszDescription[cch] = 0;
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %d)\n", hKL, lpszDescription, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL) || !IS_IME_HKL(hKL))
        return 0;

    if (uBufLen != 0)
        StringCchCopyW(lpszDescription, uBufLen, info.wszImeDescription);

    StringCchLengthW(info.wszImeDescription, _countof(info.wszImeDescription), &cch);
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
    BOOL ret;
    LPINPUTCONTEXT pIC;

    TRACE("(%p)\n", hIMC);

    if (!hIMC)
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    ret = pIC->fOpen;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
    IMEINFOEX ImeInfoEx;
    LPIMEINFO pImeInfo;
    DWORD dwValue;
    PIMEDPI pImeDpi = NULL;

    TRACE("(%p, %lu)\n", hKL, fdwIndex);

    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
        return FALSE;

    if (fdwIndex == IGP_GETIMEVERSION)
        return ImeInfoEx.dwImeWinVersion;

    if (ImeInfoEx.fLoadFlag != 2)
    {
        pImeDpi = ImmLockOrLoadImeDpi(hKL);
        if (pImeDpi == NULL)
            return FALSE;

        pImeInfo = &pImeDpi->ImeInfo;
    }
    else
    {
        pImeInfo = &ImeInfoEx.ImeInfo;
    }

    switch (fdwIndex)
    {
        case IGP_PROPERTY:      dwValue = pImeInfo->fdwProperty; break;
        case IGP_CONVERSION:    dwValue = pImeInfo->fdwConversionCaps; break;
        case IGP_SENTENCE:      dwValue = pImeInfo->fdwSentenceCaps; break;
        case IGP_UI:            dwValue = pImeInfo->fdwUICaps; break;
        case IGP_SETCOMPSTR:    dwValue = pImeInfo->fdwSCSCaps; break;
        case IGP_SELECT:        dwValue = pImeInfo->fdwSelectCaps; break;
        default:                dwValue = 0; break;
    }

    if (pImeDpi)
        ImmUnlockImeDpi(pImeDpi);
    return dwValue;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
    UINT iItem, ret = 0;
    PIMEDPI pImeDpi;
    LPSTYLEBUFA pDestA;
    LPSTYLEBUFW pSrcW, pNewStylesW = NULL;
    size_t cchW;
    INT cchA;

    TRACE("(%p, %u, %p)\n", hKL, nItem, lpStyleBuf);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesW = Imm32HeapAlloc(0, nItem * sizeof(STYLEBUFW));
        if (!pNewStylesW)
            goto Quit;
    }

    ret = pImeDpi->ImeGetRegisterWordStyle(nItem, pNewStylesW);

    if (nItem > 0)
    {
        /* lpStyleBuf <-- pNewStylesW */
        for (iItem = 0; iItem < ret; ++iItem)
        {
            pSrcW = &pNewStylesW[iItem];
            pDestA = &lpStyleBuf[iItem];
            pDestA->dwStyle = pSrcW->dwStyle;
            StringCchLengthW(pSrcW->szDescription, _countof(pSrcW->szDescription), &cchW);
            cchA = WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED,
                                       pSrcW->szDescription, (INT)cchW,
                                       pDestA->szDescription, _countof(pDestA->szDescription),
                                       NULL, NULL);
            if (cchA > _countof(pDestA->szDescription) - 1)
                cchA = _countof(pDestA->szDescription) - 1;
            pDestA->szDescription[cchA] = 0;
        }
    }

Quit:
    if (pNewStylesW)
        HeapFree(g_hImm32Heap, 0, pNewStylesW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
    UINT iItem, ret = 0;
    PIMEDPI pImeDpi;
    LPSTYLEBUFA pSrcA, pNewStylesA = NULL;
    LPSTYLEBUFW pDestW;
    size_t cchA;
    INT cchW;

    TRACE("(%p, %u, %p)\n", hKL, nItem, lpStyleBuf);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesA = Imm32HeapAlloc(0, nItem * sizeof(STYLEBUFA));
        if (!pNewStylesA)
            goto Quit;
    }

    ret = pImeDpi->ImeGetRegisterWordStyle(nItem, pNewStylesA);

    if (nItem > 0)
    {
        /* lpStyleBuf <-- pNewStylesA */
        for (iItem = 0; iItem < ret; ++iItem)
        {
            pSrcA = &pNewStylesA[iItem];
            pDestW = &lpStyleBuf[iItem];
            pDestW->dwStyle = pSrcA->dwStyle;
            StringCchLengthA(pSrcA->szDescription, _countof(pSrcA->szDescription), &cchA);
            cchW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                       pSrcA->szDescription, (INT)cchA,
                                       pDestW->szDescription, _countof(pDestW->szDescription));
            if (cchW > _countof(pDestW->szDescription) - 1)
                cchW = _countof(pDestW->szDescription) - 1;
            pDestW->szDescription[cchW] = 0;
        }
    }

Quit:
    if (pNewStylesA)
        HeapFree(g_hImm32Heap, 0, pNewStylesA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    LPINPUTCONTEXT pIC;
    BOOL ret;

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    ret = !!(pIC->fdwInit & INIT_STATUSWNDPOS);
    if (ret)
        *lpptPos = pIC->ptStatusWndPos;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    if (!IS_IME_ENABLED())
        return NULL;

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    if (hWnd == NULL)
        return (HWND)NtUserGetThreadState(3);

    return (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_IME);
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI
ImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    HKL hKL;
    PIMEDPI pImeDpi;
    BOOL ret;

    TRACE("(%p, %lu, %lu, %lu)\n", hIMC, dwAction, dwIndex, dwValue);

    if (hIMC && Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    ret = pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI
ImmRegisterWordA(HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPWSTR pszReadingW = NULL, pszRegisterW = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_a(lpszReading), dwStyle,
          debugstr_a(lpszRegister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(lpszRegister);
        if (pszRegisterW == NULL)
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingW, dwStyle, pszRegisterW);

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszRegisterW)
        HeapFree(g_hImm32Heap, 0, pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI
ImmRegisterWordW(HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPSTR pszReadingA = NULL, pszRegisterA = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_w(lpszReading), dwStyle,
          debugstr_w(lpszRegister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (!pszReadingA)
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(lpszRegister);
        if (!pszRegisterA)
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingA, dwStyle, pszRegisterA);

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszRegisterA)
        HeapFree(g_hImm32Heap, 0, pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI
ImmUnregisterWordA(HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPWSTR pszReadingW = NULL, pszUnregisterW = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_a(lpszReading), dwStyle,
          debugstr_a(lpszUnregister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (pImeDpi == NULL)
        return FALSE;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(lpszReading);
        if (pszReadingW == NULL)
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterW = Imm32WideFromAnsi(lpszUnregister);
        if (pszUnregisterW == NULL)
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingW, dwStyle, pszUnregisterW);

Quit:
    if (pszReadingW)
        HeapFree(g_hImm32Heap, 0, pszReadingW);
    if (pszUnregisterW)
        HeapFree(g_hImm32Heap, 0, pszUnregisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI
ImmUnregisterWordW(HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    LPSTR pszReadingA = NULL, pszUnregisterA = NULL;

    TRACE("(%p, %s, 0x%lX, %s)\n", hKL, debugstr_w(lpszReading), dwStyle,
          debugstr_w(lpszUnregister));

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(lpszReading);
        if (!pszReadingA)
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterA = Imm32AnsiFromWide(lpszUnregister);
        if (!pszUnregisterA)
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingA, dwStyle, pszUnregisterA);

Quit:
    if (pszReadingA)
        HeapFree(g_hImm32Heap, 0, pszReadingA);
    if (pszUnregisterA)
        HeapFree(g_hImm32Heap, 0, pszUnregisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}
