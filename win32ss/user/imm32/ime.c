/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME manipulation of IMM32
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

RTL_CRITICAL_SECTION gcsImeDpi; // Win: gcsImeDpi
PIMEDPI gpImeDpiList = NULL; // Win: gpImeDpi

// Win: ImmGetImeDpi
PIMEDPI APIENTRY Imm32FindImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi;

    RtlEnterCriticalSection(&gcsImeDpi);
    for (pImeDpi = gpImeDpiList; pImeDpi != NULL; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL)
            break;
    }
    RtlLeaveCriticalSection(&gcsImeDpi);

    return pImeDpi;
}

// Win: UnloadIME
VOID APIENTRY Imm32FreeIME(PIMEDPI pImeDpi, BOOL bDestroy)
{
    if (pImeDpi->hInst == NULL)
        return;
    if (bDestroy)
        pImeDpi->ImeDestroy(0);
    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;
}

// Win: InquireIme
BOOL APIENTRY Imm32InquireIme(PIMEDPI pImeDpi)
{
    WCHAR szUIClass[64];
    WNDCLASSW wcW;
    DWORD dwSysInfoFlags = 0;
    LPIMEINFO pImeInfo = &pImeDpi->ImeInfo;

    if (NtUserGetThreadState(THREADSTATE_ISWINLOGON))
        dwSysInfoFlags |= IME_SYSINFO_WINLOGON;

    if (GetWin32ClientInfo()->dwTIFlags & TIF_16BIT)
        dwSysInfoFlags |= IME_SYSINFO_WOW16;

    if (IS_IME_HKL(pImeDpi->hKL))
    {
        if (!pImeDpi->ImeInquire(pImeInfo, szUIClass, dwSysInfoFlags))
        {
            ERR("\n");
            return FALSE;
        }
    }
    else if (IS_CICERO_MODE() && !IS_16BIT_MODE())
    {
        if (pImeDpi->CtfImeInquireExW(pImeInfo, szUIClass, dwSysInfoFlags, pImeDpi->hKL) != S_OK)
        {
            ERR("\n");
            return FALSE;
        }
    }
    else
    {
        ERR("\n");
        return FALSE;
    }

    szUIClass[_countof(szUIClass) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

    if (pImeInfo->dwPrivateDataSize < sizeof(DWORD))
        pImeInfo->dwPrivateDataSize = sizeof(DWORD);

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
    {
        ERR("Bad flags\n");
        return FALSE;
    }
    if (pImeInfo->fdwConversionCaps & ~VALID_CMODE_CAPS)
    {
        ERR("Bad flags\n");
        return FALSE;
    }
    if (pImeInfo->fdwSentenceCaps & ~VALID_SMODE_CAPS)
    {
        ERR("Bad flags\n");
        return FALSE;
    }
    if (pImeInfo->fdwUICaps & ~VALID_UI_CAPS)
    {
        ERR("Bad flags\n");
        return FALSE;
    }
    if (pImeInfo->fdwSCSCaps & ~VALID_SCS_CAPS)
    {
        ERR("Bad flags\n");
        return FALSE;
    }
    if (pImeInfo->fdwSelectCaps & ~VALID_SELECT_CAPS)
    {
        ERR("Bad flags\n");
        return FALSE;
    }

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
        if (pImeDpi->uCodePage != GetACP() && pImeDpi->uCodePage != CP_ACP)
            return FALSE;

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPSTR)szUIClass, -1,
                            pImeDpi->szUIClass, _countof(pImeDpi->szUIClass));

        pImeDpi->szUIClass[_countof(pImeDpi->szUIClass) - 1] = UNICODE_NULL;
    }

    if (!GetClassInfoW(pImeDpi->hInst, pImeDpi->szUIClass, &wcW))
    {
        ERR("\n");
        return FALSE;
    }

    return TRUE;
}

/* Define stub IME functions */
#define DEFINE_IME_ENTRY(type, name, params, optional) \
    type APIENTRY Stub##name params { \
        FIXME("%s: Why stub called?\n", #name); \
        return (type)0; \
    }
#include <imetable.h>
#undef DEFINE_IME_ENTRY

BOOL APIENTRY Imm32LoadIME(PIMEINFOEX pImeInfoEx, PIMEDPI pImeDpi)
{
    WCHAR szPath[MAX_PATH];
    HINSTANCE hIME;
    FARPROC fn;
    BOOL ret = FALSE;

    if (!Imm32GetSystemLibraryPath(szPath, _countof(szPath), pImeInfoEx->wszImeFile))
        return FALSE;

    pImeDpi->hInst = hIME = LoadLibraryW(szPath);
    if (hIME == NULL)
    {
        ERR("LoadLibraryW(%S) failed\n", szPath);
        return FALSE;
    }

    /* Populate the table by stub IME functions */
#define DEFINE_IME_ENTRY(type, name, params, optional) pImeDpi->name = Stub##name;
#include <imetable.h>
#undef DEFINE_IME_ENTRY

    /* Populate the table by real IME functions */
#define DEFINE_IME_ENTRY(type, name, params, optional) \
    do { \
        fn = GetProcAddress(hIME, #name); \
        if (fn) pImeDpi->name = (FN_##name)fn; \
        else if (!(optional)) { \
            ERR("'%s' not found in IME module '%S'.\n", #name, szPath); \
            goto Failed; \
        } \
    } while (0);
#include <imetable.h>
#undef DEFINE_IME_ENTRY

    /* Check for Cicero IMEs */
    if (!IS_IME_HKL(pImeDpi->hKL) && IS_CICERO_MODE() && !IS_CICERO_COMPAT_DISABLED())
    {
#define CHECK_IME_FN(name) do { \
    if (!pImeDpi->name) { \
        ERR("'%s' not found in Cicero IME module '%S'.\n", #name, szPath); \
        goto Failed; \
    } \
} while(0)
        CHECK_IME_FN(CtfImeInquireExW);
        CHECK_IME_FN(CtfImeSelectEx);
        CHECK_IME_FN(CtfImeEscapeEx);
        CHECK_IME_FN(CtfImeGetGuidAtom);
        CHECK_IME_FN(CtfImeIsGuidMapEnable);
#undef CHECK_IME_FN
    }

    if (Imm32InquireIme(pImeDpi))
    {
        ret = TRUE;
    }
    else
    {
Failed:
        ret = FALSE;
        FreeLibrary(pImeDpi->hInst);
        pImeDpi->hInst = NULL;
    }

    if (pImeInfoEx->fLoadFlag == 0)
    {
        if (ret)
        {
            C_ASSERT(sizeof(pImeInfoEx->wszUIClass) == sizeof(pImeDpi->szUIClass));
            pImeInfoEx->ImeInfo = pImeDpi->ImeInfo;
            RtlCopyMemory(pImeInfoEx->wszUIClass, pImeDpi->szUIClass,
                          sizeof(pImeInfoEx->wszUIClass));
            pImeInfoEx->fLoadFlag = 2;
        }
        else
        {
            pImeInfoEx->fLoadFlag = 1;
        }

        NtUserSetImeInfoEx(pImeInfoEx);
    }

    return ret;
}

PIMEDPI APIENTRY Imm32LoadImeDpi(HKL hKL, BOOL bLock)
{
    IMEINFOEX ImeInfoEx;
    CHARSETINFO ci;
    PIMEDPI pImeDpiNew, pImeDpiFound;
    UINT uCodePage;
    LCID lcid;

    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
    {
        ERR("\n");
        return NULL;
    }

    if (ImeInfoEx.fLoadFlag == 1)
    {
        ERR("\n");
        return NULL;
    }

    pImeDpiNew = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(IMEDPI));
    if (IS_NULL_UNEXPECTEDLY(pImeDpiNew))
        return NULL;

    pImeDpiNew->hKL = hKL;

    lcid = LOWORD(hKL);
    if (TranslateCharsetInfo(UlongToPtr(lcid), &ci, TCI_SRCLOCALE))
        uCodePage = ci.ciACP;
    else
        uCodePage = CP_ACP;
    pImeDpiNew->uCodePage = uCodePage;

    if (!Imm32LoadIME(&ImeInfoEx, pImeDpiNew))
    {
        ERR("\n");
        ImmLocalFree(pImeDpiNew);
        return FALSE;
    }

    RtlEnterCriticalSection(&gcsImeDpi);

    pImeDpiFound = Imm32FindImeDpi(hKL);
    if (pImeDpiFound)
    {
        if (!bLock)
            pImeDpiFound->dwFlags &= ~IMEDPI_FLAG_LOCKED;

        RtlLeaveCriticalSection(&gcsImeDpi);
        Imm32FreeIME(pImeDpiNew, FALSE);
        ImmLocalFree(pImeDpiNew);
        return pImeDpiFound;
    }
    else
    {
        if (bLock)
        {
            pImeDpiNew->dwFlags |= IMEDPI_FLAG_LOCKED;
            pImeDpiNew->cLockObj = 1;
        }

        pImeDpiNew->pNext = gpImeDpiList;
        gpImeDpiList = pImeDpiNew;

        RtlLeaveCriticalSection(&gcsImeDpi);
        return pImeDpiNew;
    }
}

// Win: FindOrLoadImeDpi
PIMEDPI APIENTRY Imm32FindOrLoadImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL) && (!IS_CICERO_MODE() || IS_16BIT_MODE()))
    {
        TRACE("\n");
        return NULL;
    }

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Imm32LoadImeDpi(hKL, TRUE);
    return pImeDpi;
}

static LRESULT APIENTRY
ImeDpi_Escape(PIMEDPI pImeDpi, HIMC hIMC, UINT uSubFunc, LPVOID lpData, HKL hKL)
{
    if (IS_IME_HKL(hKL))
        return pImeDpi->ImeEscape(hIMC, uSubFunc, lpData);
    if (IS_CICERO_MODE() && !IS_16BIT_MODE())
        return pImeDpi->CtfImeEscapeEx(hIMC, uSubFunc, lpData, hKL);

    return 0;
}

// Win: ImmUnloadIME
BOOL APIENTRY Imm32ReleaseIME(HKL hKL)
{
    BOOL ret = TRUE;
    PIMEDPI pImeDpi0, pImeDpi1;

    RtlEnterCriticalSection(&gcsImeDpi);

    for (pImeDpi0 = gpImeDpiList; pImeDpi0; pImeDpi0 = pImeDpi0->pNext)
    {
        if (pImeDpi0->hKL == hKL)
            break;
    }

    if (!pImeDpi0)
        goto Quit;

    if (pImeDpi0->cLockObj)
    {
        pImeDpi0->dwFlags |= IMEDPI_FLAG_UNLOADED;
        ret = FALSE;
        goto Quit;
    }

    if (gpImeDpiList == pImeDpi0)
    {
        gpImeDpiList = pImeDpi0->pNext;
    }
    else if (gpImeDpiList)
    {
        for (pImeDpi1 = gpImeDpiList; pImeDpi1; pImeDpi1 = pImeDpi1->pNext)
        {
            if (pImeDpi1->pNext == pImeDpi0)
            {
                pImeDpi1->pNext = pImeDpi0->pNext;
                break;
            }
        }
    }

    Imm32FreeIME(pImeDpi0, TRUE);
    ImmLocalFree(pImeDpi0);

Quit:
    RtlLeaveCriticalSection(&gcsImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    IMEINFOEX info;
    TRACE("(%p)\n", hKL);
    return !!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayoutTFS, &hKL);
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    if (!IS_IMM_MODE())
    {
        TRACE("\n");
        return NULL;
    }

    if (hWnd == NULL)
        return (HWND)NtUserGetThreadState(THREADSTATE_DEFAULTIMEWINDOW);

    return (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_IME);
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD_PTR dwValue)
{
    HKL hKL;
    PIMEDPI pImeDpi;
    BOOL ret;

    TRACE("(%p, %lu, %lu, %lu)\n", hIMC, dwAction, dwIndex, dwValue);

    if (hIMC && IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    pImeDpi = ImmLockImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    TRACE("NotifyIME(%p, %lu, %lu, %p)\n", hIMC, dwAction, dwIndex, dwValue);
    ret = pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);
    if (!ret)
        WARN("NotifyIME(%p, %lu, %lu, %p) failed\n", hIMC, dwAction, dwIndex, dwValue);

    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *              ImmGetImeInfoEx (IMM32.@)
 */
BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey)
{
    BOOL bTextServiceDisabled = FALSE;

    if (SearchType == ImeInfoExKeyboardLayoutTFS)
    {
        SearchType = ImeInfoExKeyboardLayout;
        bTextServiceDisabled = CtfImmIsTextFrameServiceDisabled();
    }

    if (SearchType == ImeInfoExKeyboardLayout)
    {
        HKL hKL = *(HKL *)pvSearchKey;
        pImeInfoEx->hkl = hKL;

        if (!IS_IME_HKL(hKL) &&
            (!IS_CICERO_MODE() || IS_CICERO_COMPAT_DISABLED() || bTextServiceDisabled))
        {
            TRACE("IME is disabled\n");
            return FALSE;
        }

        return NtUserGetImeInfoEx(pImeInfoEx, SearchType);
    }

    if (SearchType == ImeInfoExImeFileName)
    {
        StringCchCopyW(pImeInfoEx->wszImeFile, _countof(pImeInfoEx->wszImeFile), pvSearchKey);
        return NtUserGetImeInfoEx(pImeInfoEx, SearchType);
    }

    /* NOTE: ImeInfoExImeWindow is ignored */
    ERR("SearchType: %d\n", SearchType);
    return FALSE;
}

/***********************************************************************
 *		ImmLockImeDpi (IMM32.@)
 */
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi = NULL;

    TRACE("(%p)\n", hKL);

    RtlEnterCriticalSection(&gcsImeDpi);

    /* Find by hKL */
    for (pImeDpi = gpImeDpiList; pImeDpi; pImeDpi = pImeDpi->pNext)
    {
        if (pImeDpi->hKL == hKL) /* found */
        {
            /* lock if possible */
            if (pImeDpi->dwFlags & IMEDPI_FLAG_UNLOADED)
                pImeDpi = NULL;
            else
                ++(pImeDpi->cLockObj);
            break;
        }
    }

    RtlLeaveCriticalSection(&gcsImeDpi);
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

    RtlEnterCriticalSection(&gcsImeDpi);

    /* unlock */
    --(pImeDpi->cLockObj);
    if (pImeDpi->cLockObj != 0)
    {
        RtlLeaveCriticalSection(&gcsImeDpi);
        return;
    }

    if ((pImeDpi->dwFlags & IMEDPI_FLAG_UNLOADED) == 0)
    {
        if ((pImeDpi->dwFlags & IMEDPI_FLAG_LOCKED) == 0 ||
            (pImeDpi->ImeInfo.fdwProperty & IME_PROP_END_UNLOAD) == 0)
        {
            RtlLeaveCriticalSection(&gcsImeDpi);
            return;
        }
    }

    /* Remove from list */
    for (ppEntry = &gpImeDpiList; *ppEntry; ppEntry = &((*ppEntry)->pNext))
    {
        if (*ppEntry == pImeDpi) /* found */
        {
            *ppEntry = pImeDpi->pNext;
            break;
        }
    }

    Imm32FreeIME(pImeDpi, TRUE);
    ImmLocalFree(pImeDpi);

    RtlLeaveCriticalSection(&gcsImeDpi);
}

/***********************************************************************
 *		ImmLoadIME (IMM32.@)
 */
BOOL WINAPI ImmLoadIME(HKL hKL)
{
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL) && (!IS_CICERO_MODE() || IS_16BIT_MODE()))
    {
        TRACE("\n");
        return FALSE;
    }

    pImeDpi = Imm32FindImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Imm32LoadImeDpi(hKL, FALSE);
    return (pImeDpi != NULL);
}

/***********************************************************************
 *		ImmDisableIME (IMM32.@)
 */
BOOL WINAPI ImmDisableIME(DWORD dwThreadId)
{
    return NtUserDisableThreadIme(dwThreadId);
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p,%p,%d)\n", hKL, lpszDescription, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL))
    {
        ERR("\n");
        return 0;
    }

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

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL))
    {
        ERR("\n");
        return 0;
    }

    if (uBufLen != 0)
        StringCchCopyW(lpszDescription, uBufLen, info.wszImeDescription);

    StringCchLengthW(info.wszImeDescription, _countof(info.wszImeDescription), &cch);
    return (UINT)cch;
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA( HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
    BOOL bDefUsed;
    IMEINFOEX info;
    size_t cch;

    TRACE("(%p, %p, %u)\n", hKL, lpszFileName, uBufLen);

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL))
    {
        ERR("\n");
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

    if (!ImmGetImeInfoEx(&info, ImeInfoExKeyboardLayout, &hKL))
    {
        ERR("\n");
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
    {
        ERR("\n");
        return FALSE;
    }

    if (fdwIndex == IGP_GETIMEVERSION)
        return ImeInfoEx.dwImeWinVersion;

    if (ImeInfoEx.fLoadFlag != 2)
    {
        pImeDpi = Imm32FindOrLoadImeDpi(hKL);
        if (IS_NULL_UNEXPECTEDLY(pImeDpi))
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
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(HKL hKL, HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    LRESULT ret;
    PIMEDPI pImeDpi;
    INT cch;
    CHAR szA[MAX_IMM_FILENAME];
    WCHAR szW[MAX_IMM_FILENAME];

    TRACE("(%p, %p, %u, %p)\n", hKL, hIMC, uSubFunc, lpData);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi) || !lpData) /* No conversion needed */
    {
        ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    switch (uSubFunc)
    {
        case IME_ESC_SEQUENCE_TO_INTERNAL:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);

            cch = 0;
            if (HIWORD(ret))
                szW[cch++] = HIWORD(ret);
            if (LOWORD(ret))
                szW[cch++] = LOWORD(ret);

            cch = WideCharToMultiByte(pImeDpi->uCodePage, 0, szW, cch, szA, _countof(szA),
                                      NULL, NULL);
            switch (cch)
            {
                case 1:
                    ret = MAKEWORD(szA[0], 0);
                    break;
                case 2:
                    ret = MAKEWORD(szA[1], szA[0]);
                    break;
                case 3:
                    ret = MAKELONG(MAKEWORD(szA[2], szA[1]), MAKEWORD(szA[0], 0));
                    break;
                case 4:
                    ret = MAKELONG(MAKEWORD(szA[3], szA[2]), MAKEWORD(szA[1], szA[0]));
                    break;
                default:
                    ret = 0;
                    break;
            }
            break;

        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_IME_NAME:
        case IME_ESC_GETHELPFILENAME:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, szW, hKL);
            if (ret)
            {
                szW[_countof(szW) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
                WideCharToMultiByte(pImeDpi->uCodePage, 0, szW, -1,
                                    lpData, MAX_IMM_FILENAME, NULL, NULL);
                ((LPSTR)lpData)[MAX_IMM_FILENAME - 1] = 0;
            }
            break;

        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_ESC_HANJA_MODE:
            MultiByteToWideChar(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                lpData, -1, szW, _countof(szW));
            szW[_countof(szW) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, szW, hKL);
            break;

        default:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);
            break;
    }

    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %p\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(HKL hKL, HIMC hIMC, UINT uSubFunc, LPVOID lpData)
{
    LRESULT ret;
    PIMEDPI pImeDpi;
    INT cch;
    CHAR szA[MAX_IMM_FILENAME];
    WCHAR szW[MAX_IMM_FILENAME];
    WORD word;

    TRACE("(%p, %p, %u, %p)\n", hKL, hIMC, uSubFunc, lpData);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi) || !lpData) /* No conversion needed */
    {
        ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    switch (uSubFunc)
    {
        case IME_ESC_SEQUENCE_TO_INTERNAL:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);

            word = LOWORD(ret);
            cch = 0;
            if (HIBYTE(word))
                szA[cch++] = HIBYTE(word);
            if (LOBYTE(word))
                szA[cch++] = LOBYTE(word);

            cch = MultiByteToWideChar(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                      szA, cch, szW, _countof(szW));
            switch (cch)
            {
                case 1:  ret = szW[0]; break;
                case 2:  ret = MAKELONG(szW[1], szW[0]); break;
                default: ret = 0; break;
            }
            break;

        case IME_ESC_GET_EUDC_DICTIONARY:
        case IME_ESC_IME_NAME:
        case IME_ESC_GETHELPFILENAME:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, szA, hKL);
            if (ret)
            {
                szA[_countof(szA) - 1] = 0;
                MultiByteToWideChar(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                    szA, -1, lpData, MAX_IMM_FILENAME);
                ((LPWSTR)lpData)[MAX_IMM_FILENAME - 1] = UNICODE_NULL; /* Avoid buffer overrun */
            }
            break;

        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_ESC_HANJA_MODE:
            WideCharToMultiByte(pImeDpi->uCodePage, 0,
                                lpData, -1, szA, _countof(szA), NULL, NULL);
            szA[_countof(szA) - 1] = 0;
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, szA, hKL);
            break;

        default:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);
            break;
    }

    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %p\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
    BOOL ret;
    LPINPUTCONTEXT pIC;

    TRACE("(%p)\n", hIMC);

    if (IS_NULL_UNEXPECTEDLY(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    ret = pIC->fOpen;
    ImmUnlockIMC(hIMC);
    TRACE("ret: %d\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    DWORD dwConversion;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    BOOL bHasChange = FALSE;

    TRACE("(%p, %d)\n", hIMC, fOpen);

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (pIC->fOpen != fOpen)
    {
        pIC->fOpen = fOpen;
        hWnd = pIC->hWnd;
        dwConversion = pIC->fdwConversion;
        bHasChange = TRUE;
    }

    ImmUnlockIMC(hIMC);

    if (bHasChange)
    {
        Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                           IMC_SETOPENSTATUS, IMN_SETOPENSTATUS, 0);
        NtUserNotifyIMEStatus(hWnd, fOpen, dwConversion);
    }
    else
    {
        TRACE("No change.\n");
    }

    return TRUE;
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
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    ret = !!(pIC->fdwInit & INIT_STATUSWNDPOS);
    if (ret)
        *lpptPos = pIC->ptStatusWndPos;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    TRACE("(%p, {%ld, %ld})\n", hIMC, lpptPos->x, lpptPos->y);

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->ptStatusWndPos = *lpptPos;
    pIC->fdwInit |= INIT_STATUSWNDPOS;

    ImmUnlockIMC(hIMC);

    Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                       IMC_SETSTATUSWINDOWPOS, IMN_SETSTATUSWINDOWPOS, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (pIC->fdwInit & INIT_COMPFORM)
    {
        *lpCompForm = pIC->cfCompForm;
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXTDX pIC;
    HWND hWnd;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    pIC->cfCompForm = *lpCompForm;
    pIC->fdwInit |= INIT_COMPFORM;

    if (pIC->dwUIFlags & 0x8)
        pIC->dwUIFlags &= ~0x8;
    else
        pIC->dwUIFlags &= ~0x2;

    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                       IMC_SETCOMPOSITIONWINDOW, IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    PCLIENTIMC pClientImc;
    BOOL ret = FALSE, bWide;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            LogFontWideToAnsi(&pIC->lfFont.W, lplf);
        else
            *lplf = pIC->lfFont.A;

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            *lplf = pIC->lfFont.W;
        else
            LogFontAnsiToWide(&pIC->lfFont.A, lplf);

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    LOGFONTW lfW;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXTDX pIC;
    LANGID LangID;
    HWND hWnd;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (bWide)
    {
        LogFontAnsiToWide(lplf, &lfW);
        return ImmSetCompositionFontW(hIMC, &lfW);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (GetWin32ClientInfo()->dwExpWinVer < _WIN32_WINNT_NT4) /* old version (3.x)? */
    {
        LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        if (PRIMARYLANGID(LangID) == LANG_JAPANESE &&
            !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageA(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.A = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                       IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    LOGFONTA lfA;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    HWND hWnd;
    LPINPUTCONTEXTDX pIC;
    LANGID LangID;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (!bWide)
    {
        LogFontWideToAnsi(lplf, &lfA);
        return ImmSetCompositionFontA(hIMC, &lfA);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (GetWin32ClientInfo()->dwExpWinVer < _WIN32_WINNT_NT4) /* old version (3.x)? */
    {
        LangID = LANGIDFROMLCID(GetSystemDefaultLCID());
        if (PRIMARYLANGID(LangID) == LANG_JAPANESE &&
            !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageW(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.W = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                       IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI
ImmGetConversionListA(HKL hKL, HIMC hIMC, LPCSTR pSrc, LPCANDIDATELIST lpDst,
                      DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    UINT cb;
    LPWSTR pszSrcW = NULL;
    LPCANDIDATELIST pCL = NULL;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, pSrc, lpDst, dwBufLen, uFlag);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcW = Imm32WideFromAnsi(pImeDpi->uCodePage, pSrc);
        if (IS_NULL_UNEXPECTEDLY(pszSrcW))
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, NULL, 0, uFlag);
    if (IS_ZERO_UNEXPECTEDLY(cb))
        goto Quit;

    pCL = ImmLocalAlloc(0, cb);
    if (IS_NULL_UNEXPECTEDLY(pCL))
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, pCL, cb, uFlag);
    if (IS_ZERO_UNEXPECTEDLY(cb))
        goto Quit;

    ret = CandidateListWideToAnsi(pCL, lpDst, dwBufLen, pImeDpi->uCodePage);

Quit:
    ImmLocalFree(pszSrcW);
    ImmLocalFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: 0x%X\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI
ImmGetConversionListW(HKL hKL, HIMC hIMC, LPCWSTR pSrc, LPCANDIDATELIST lpDst,
                      DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    INT cb;
    PIMEDPI pImeDpi;
    LPCANDIDATELIST pCL = NULL;
    LPSTR pszSrcA = NULL;

    TRACE("(%p, %p, %S, %p, %lu, 0x%lX)\n", hKL, hIMC, pSrc, lpDst, dwBufLen, uFlag);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcA = Imm32AnsiFromWide(pImeDpi->uCodePage, pSrc);
        if (IS_NULL_UNEXPECTEDLY(pszSrcA))
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, NULL, 0, uFlag);
    if (IS_ZERO_UNEXPECTEDLY(cb))
        goto Quit;

    pCL = ImmLocalAlloc(0, cb);
    if (IS_NULL_UNEXPECTEDLY(pCL))
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, pCL, cb, uFlag);
    if (IS_ZERO_UNEXPECTEDLY(cb))
        goto Quit;

    ret = CandidateListAnsiToWide(pCL, lpDst, dwBufLen, pImeDpi->uCodePage);

Quit:
    ImmLocalFree(pszSrcA);
    ImmLocalFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: 0x%X\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    LPINPUTCONTEXT pIC;

    TRACE("(%p %p %p)\n", hIMC, lpfdwConversion, lpfdwSentence);

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (lpfdwConversion)
    {
        *lpfdwConversion = pIC->fdwConversion;
        TRACE("0x%X\n", *lpfdwConversion);
    }

    if (lpfdwSentence)
    {
        *lpfdwSentence = pIC->fdwSentence;
        TRACE("0x%X\n", *lpfdwSentence);
    }

    ImmUnlockIMC(hIMC);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    HKL hKL;
    LPINPUTCONTEXT pIC;
    DWORD dwOldConversion, dwOldSentence;
    BOOL fOpen = FALSE, fConversionChange = FALSE, fSentenceChange = FALSE, fUseCicero = FALSE;
    HWND hWnd;

    TRACE("(%p, 0x%lX, 0x%lX)\n", hIMC, fdwConversion, fdwSentence);

    hKL = GetKeyboardLayout(0);
    if (!IS_IME_HKL(hKL) && IS_CICERO_MODE() && !IS_16BIT_MODE())
        fUseCicero = TRUE;

    if (IS_CROSS_THREAD_HIMC(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pIC))
        return FALSE;

    if (pIC->fdwConversion != fdwConversion)
    {
        dwOldConversion = pIC->fdwConversion;
        pIC->fdwConversion = fdwConversion;
        fConversionChange = TRUE;
    }

    if (pIC->fdwSentence != fdwSentence)
    {
        dwOldSentence = pIC->fdwSentence;
        pIC->fdwSentence = fdwSentence;
        fSentenceChange = TRUE;
    }

    hWnd = pIC->hWnd;
    fOpen = pIC->fOpen;
    ImmUnlockIMC(hIMC);

    if (fConversionChange || fUseCicero)
    {
        Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldConversion,
                           IMC_SETCONVERSIONMODE, IMN_SETCONVERSIONMODE, 0);
        if (fConversionChange)
            NtUserNotifyIMEStatus(hWnd, fOpen, fdwConversion);
    }

    if (fSentenceChange || fUseCicero)
    {
        Imm32MakeIMENotify(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldSentence,
                           IMC_SETSENTENCEMODE, IMN_SETSENTENCEMODE, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    REGISTERWORDW RegWordW;
    LPREGISTERWORDA pRegWordA;

    TRACE("(%p, %p, 0x%lX, %p)\n", hKL, hWnd, dwMode, lpData);

    if (IS_NULL_UNEXPECTEDLY(ValidateHwnd(hWnd)) || IS_CROSS_PROCESS_HWND(hWnd))
        return FALSE;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    RtlZeroMemory(&RegWordW, sizeof(RegWordW));

    if (!ImeDpi_IsUnicode(pImeDpi) || !lpData || dwMode != IME_CONFIG_REGISTERWORD)
        goto DoIt; /* No conversion needed */

    pRegWordA = lpData;

    if (pRegWordA->lpReading)
    {
        RegWordW.lpReading = Imm32WideFromAnsi(pImeDpi->uCodePage, pRegWordA->lpReading);
        if (IS_NULL_UNEXPECTEDLY(RegWordW.lpReading))
            goto Quit;
    }

    if (pRegWordA->lpWord)
    {
        RegWordW.lpWord = Imm32WideFromAnsi(pImeDpi->uCodePage, pRegWordA->lpWord);
        if (IS_NULL_UNEXPECTEDLY(RegWordW.lpWord))
            goto Quit;
    }

    lpData = &RegWordW;

DoIt:
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1B, 0);
    ret = pImeDpi->ImeConfigure(hKL, hWnd, dwMode, lpData);
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1A, 0);

Quit:
    ImmLocalFree(RegWordW.lpReading);
    ImmLocalFree(RegWordW.lpWord);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    BOOL ret = FALSE;
    PIMEDPI pImeDpi;
    REGISTERWORDA RegWordA;
    LPREGISTERWORDW pRegWordW;

    TRACE("(%p, %p, 0x%lX, %p)\n", hKL, hWnd, dwMode, lpData);

    if (IS_NULL_UNEXPECTEDLY(ValidateHwnd(hWnd)) || IS_CROSS_PROCESS_HWND(hWnd))
        return FALSE;

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    RtlZeroMemory(&RegWordA, sizeof(RegWordA));

    if (ImeDpi_IsUnicode(pImeDpi) || !lpData || dwMode != IME_CONFIG_REGISTERWORD)
        goto DoIt; /* No conversion needed */

    pRegWordW = lpData;

    if (pRegWordW->lpReading)
    {
        RegWordA.lpReading = Imm32AnsiFromWide(pImeDpi->uCodePage, pRegWordW->lpReading);
        if (IS_NULL_UNEXPECTEDLY(RegWordA.lpReading))
            goto Quit;
    }

    if (pRegWordW->lpWord)
    {
        RegWordA.lpWord = Imm32AnsiFromWide(pImeDpi->uCodePage, pRegWordW->lpWord);
        if (IS_NULL_UNEXPECTEDLY(RegWordA.lpWord))
            goto Quit;
    }

    lpData = &RegWordA;

DoIt:
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1B, 0);
    ret = pImeDpi->ImeConfigure(hKL, hWnd, dwMode, lpData);
    SendMessageW(hWnd, WM_IME_SYSTEM, 0x1A, 0);

Quit:
    ImmLocalFree(RegWordA.lpReading);
    ImmLocalFree(RegWordA.lpWord);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmWINNLSEnableIME (IMM32.@)
 */
BOOL WINAPI ImmWINNLSEnableIME(HWND hWnd, BOOL enable)
{
    HIMC hIMC;
    PCLIENTIMC pClientImc;
    HWND hImeWnd;
    BOOL bImeWnd, ret;

    TRACE("(%p, %d)\n", hWnd, enable);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    hIMC = (HIMC)NtUserGetThreadState(THREADSTATE_DEFAULTINPUTCONTEXT);
    if (IS_NULL_UNEXPECTEDLY(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (IS_NULL_UNEXPECTEDLY(pClientImc))
        return FALSE;

    ret = !(pClientImc->dwFlags & CLIENTIMC_DISABLEIME);
    if (!!enable == ret)
    {
        TRACE("Same\n");
        ImmUnlockClientImc(pClientImc);
        return ret;
    }

    if (!IsWindow(hWnd))
        hWnd = GetFocus();

    hImeWnd = ImmGetDefaultIMEWnd(hWnd);
    bImeWnd = IsWindow(hImeWnd);
    if (bImeWnd)
        ImmSetActiveContext(hWnd, (enable ? NULL : hIMC), FALSE);

    if (enable)
        pClientImc->dwFlags &= ~CLIENTIMC_DISABLEIME;
    else
        pClientImc->dwFlags |= CLIENTIMC_DISABLEIME;

    ImmUnlockClientImc(pClientImc);

    if (bImeWnd)
        ImmSetActiveContext(hWnd, (enable ? hIMC : NULL), TRUE);

    return ret;
}
