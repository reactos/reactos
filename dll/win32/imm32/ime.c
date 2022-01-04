/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME manipulation of IMM32
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

BOOL APIENTRY Imm32InquireIme(PIMEDPI pImeDpi)
{
    WCHAR szUIClass[64];
    WNDCLASSW wcW;
    DWORD dwSysInfoFlags = 0;
    LPIMEINFO pImeInfo = &pImeDpi->ImeInfo;

    if (NtUserGetThreadState(16))
        dwSysInfoFlags |= IME_SYSINFO_WINLOGON;

    if (IS_IME_HKL(pImeDpi->hKL))
    {
        if (!pImeDpi->ImeInquire(pImeInfo, szUIClass, dwSysInfoFlags))
            return FALSE;
    }
    else if (Imm32IsCiceroMode() && pImeDpi->CtfImeInquireExW)
    {
        if (!pImeDpi->CtfImeInquireExW(pImeInfo, szUIClass, dwSysInfoFlags, pImeDpi->hKL))
            return FALSE;
    }
    else
    {
        return FALSE;
    }

    szUIClass[_countof(szUIClass) - 1] = 0;

    if (pImeInfo->dwPrivateDataSize == 0)
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

BOOL APIENTRY Imm32LoadImeInfo(PIMEINFOEX pImeInfoEx, PIMEDPI pImeDpi)
{
    WCHAR szPath[MAX_PATH];
    HINSTANCE hIME;
    FARPROC fn;
    BOOL ret = FALSE;

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

#define DEFINE_IME_ENTRY(type, name, params, optional) \
    do { \
        fn = GetProcAddress(hIME, #name); \
        if (fn) pImeDpi->name = (FN_##name)fn; \
        else if (!(optional)) { \
            ERR("'%s' not found in the IME module '%s'.\n", #name, debugstr_w(szPath)); \
            goto Failed; \
        } \
    } while (0);
#include "imetable.h"
#undef DEFINE_IME_ENTRY

    if (Imm32InquireIme(pImeDpi))
    {
        ret = TRUE;
    }
    else
    {
        ERR("Imm32InquireIme failed\n");
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
        Imm32HeapFree(pImeDpiNew);
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
        Imm32HeapFree(pImeDpiNew);
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

PIMEDPI APIENTRY ImmLockOrLoadImeDpi(HKL hKL)
{
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL) && (!Imm32IsCiceroMode() || Imm32Is16BitMode()))
        return NULL;

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, TRUE);
    return pImeDpi;
}

static LRESULT APIENTRY
ImeDpi_Escape(PIMEDPI pImeDpi, HIMC hIMC, UINT uSubFunc, LPVOID lpData, HKL hKL)
{
    if (IS_IME_HKL(hKL))
        return pImeDpi->ImeEscape(hIMC, uSubFunc, lpData);

    if (Imm32IsCiceroMode() && pImeDpi->CtfImeEscapeEx)
        return pImeDpi->CtfImeEscapeEx(hIMC, uSubFunc, lpData, hKL);

    return 0;
}

BOOL APIENTRY Imm32ReleaseIME(HKL hKL)
{
    BOOL ret = TRUE;
    PIMEDPI pImeDpi0, pImeDpi1;

    RtlEnterCriticalSection(&g_csImeDpi);

    for (pImeDpi0 = g_pImeDpiList; pImeDpi0; pImeDpi0 = pImeDpi0->pNext)
    {
        if (pImeDpi0->hKL == hKL)
            break;
    }

    if (!pImeDpi0)
        goto Quit;

    if (pImeDpi0->cLockObj)
    {
        pImeDpi0->dwFlags |= IMEDPI_FLAG_UNKNOWN;
        ret = FALSE;
        goto Quit;
    }

    if (g_pImeDpiList == pImeDpi0)
    {
        g_pImeDpiList = pImeDpi0->pNext;
    }
    else if (g_pImeDpiList)
    {
        for (pImeDpi1 = g_pImeDpiList; pImeDpi1; pImeDpi1 = pImeDpi1->pNext)
        {
            if (pImeDpi1->pNext == pImeDpi0)
            {
                pImeDpi1->pNext = pImeDpi0->pNext;
                break;
            }
        }
    }

    Imm32FreeImeDpi(pImeDpi0, TRUE);
    Imm32HeapFree(pImeDpi0);

Quit:
    RtlLeaveCriticalSection(&g_csImeDpi);
    return ret;
}

DWORD APIENTRY
Imm32GetImeMenuItemWCrossProcess(HIMC hIMC, DWORD dwFlags, DWORD dwType, LPVOID lpImeParentMenu,
                                 LPVOID lpImeMenu, DWORD dwSize)
{
    FIXME("We have to do something\n");
    return 0;
}

DWORD APIENTRY
Imm32GetImeMenuItemsAW(HIMC hIMC, DWORD dwFlags, DWORD dwType, LPVOID lpImeParentMenu,
                       LPVOID lpImeMenu, DWORD dwSize, BOOL bTargetIsAnsi)
{
    DWORD ret = 0, cbTotal, dwProcessId, dwThreadId, iItem;
    LPINPUTCONTEXT pIC;
    PIMEDPI pImeDpi = NULL;
    IMEMENUITEMINFOA ParentA;
    IMEMENUITEMINFOW ParentW;
    LPIMEMENUITEMINFOA pItemA;
    LPIMEMENUITEMINFOW pItemW;
    LPVOID pNewItems = NULL, pNewParent = NULL;
    BOOL bImcIsAnsi;
    HKL hKL;

    if (!hIMC)
        return 0;

    dwProcessId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTPROCESSID);
    if (dwProcessId == 0)
        return 0;

    if (dwProcessId != GetCurrentProcessId())
    {
        if (bTargetIsAnsi)
            return 0;
        return Imm32GetImeMenuItemWCrossProcess(hIMC, dwFlags, dwType, lpImeParentMenu,
                                                lpImeMenu, dwSize);
    }

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return 0;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (dwThreadId == 0)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    hKL = GetKeyboardLayout(dwThreadId);
    pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    if (pImeDpi->ImeGetImeMenuItems == NULL)
        goto Quit;

    bImcIsAnsi = Imm32IsImcAnsi(hIMC);

    if (bImcIsAnsi != bTargetIsAnsi)
    {
        if (bTargetIsAnsi)
        {
            if (lpImeParentMenu)
                pNewParent = &ParentW;

            if (lpImeMenu)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOA)) * sizeof(IMEMENUITEMINFOW));
                pNewItems = Imm32HeapAlloc(0, cbTotal);
                if (!pNewItems)
                    goto Quit;
            }
        }
        else
        {
            if (lpImeParentMenu)
                pNewParent = &ParentA;

            if (lpImeMenu)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOW)) * sizeof(IMEMENUITEMINFOA));
                pNewItems = Imm32HeapAlloc(0, cbTotal);
                if (!pNewItems)
                    goto Quit;
            }
        }
    }
    else
    {
        pNewItems = lpImeMenu;
        pNewParent = lpImeParentMenu;
    }

    ret = pImeDpi->ImeGetImeMenuItems(hIMC, dwFlags, dwType, pNewParent, pNewItems, dwSize);
    if (!ret || !lpImeMenu)
        goto Quit;

    if (bImcIsAnsi != bTargetIsAnsi)
    {
        if (bTargetIsAnsi)
        {
            if (pNewParent)
                Imm32ImeMenuWideToAnsi(pNewParent, lpImeParentMenu, CP_ACP);

            pItemW = pNewItems;
            pItemA = lpImeMenu;
            for (iItem = 0; iItem < ret; ++iItem, ++pItemW, ++pItemA)
            {
                if (!Imm32ImeMenuWideToAnsi(pItemW, pItemA, CP_ACP))
                {
                    ret = 0;
                    break;
                }
            }
        }
        else
        {
            if (pNewParent)
                Imm32ImeMenuAnsiToWide(pNewParent, lpImeParentMenu, pImeDpi->uCodePage, TRUE);

            pItemA = pNewItems;
            pItemW = lpImeMenu;
            for (iItem = 0; iItem < dwSize; ++iItem, ++pItemA, ++pItemW)
            {
                if (!Imm32ImeMenuAnsiToWide(pItemA, pItemW, pImeDpi->uCodePage, TRUE))
                {
                    ret = 0;
                    break;
                }
            }
        }
    }

Quit:
    if (pNewItems != lpImeMenu)
        Imm32HeapFree(pNewItems);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
    return ret;
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
    if (!pszFileNameW)
        goto Quit;

    pszLayoutTextW = Imm32WideFromAnsi(lpszLayoutText);
    if (!pszLayoutTextW)
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
    WCHAR szImeFileName[MAX_PATH], szImeDestPath[MAX_PATH], szImeKey[20];
    IMEINFOEX InfoEx;
    LPWSTR pchFilePart;
    UINT iLayout, cLayouts;
    HKL hNewKL;
    WORD wLangID;
    PREG_IME pLayouts = NULL;

    TRACE("(%s, %s)\n", debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText));

    GetFullPathNameW(lpszIMEFileName, _countof(szImeFileName), szImeFileName, &pchFilePart);
    CharUpperW(szImeFileName);
    if (!pchFilePart)
        return NULL;

    /* Load the IME version info */
    InfoEx.hkl = hNewKL = NULL;
    StringCchCopyW(InfoEx.wszImeFile, _countof(InfoEx.wszImeFile), pchFilePart);
    if (Imm32LoadImeVerInfo(&InfoEx) && InfoEx.hkl)
        wLangID = LOWORD(InfoEx.hkl);
    else
        return NULL;

    /* Get the IME layouts from registry */
    cLayouts = Imm32GetRegImes(NULL, 0);
    if (cLayouts)
    {
        pLayouts = Imm32HeapAlloc(0, cLayouts * sizeof(REG_IME));
        if (!pLayouts || !Imm32GetRegImes(pLayouts, cLayouts))
        {
            Imm32HeapFree(pLayouts);
            return NULL;
        }

        for (iLayout = 0; iLayout < cLayouts; ++iLayout)
        {
            if (lstrcmpiW(pLayouts[iLayout].szFileName, pchFilePart) == 0)
            {
                if (wLangID != LOWORD(pLayouts[iLayout].hKL))
                    goto Quit; /* The language is different */

                hNewKL = pLayouts[iLayout].hKL; /* Found */
                break;
            }
        }
    }

    /* If the IME for the specified filename is valid, then unload it now */
    /* FIXME: ImmGetImeInfoEx is broken */
    if (ImmGetImeInfoEx(&InfoEx, 3, pchFilePart) &&
        !UnloadKeyboardLayout(InfoEx.hkl))
    {
        hNewKL = NULL;
        goto Quit;
    }

    Imm32GetSystemLibraryPath(szImeDestPath, _countof(szImeDestPath), pchFilePart);
    CharUpperW(szImeDestPath);

    /* If the source and the destination pathnames were different, then copy the IME file */
    if (lstrcmpiW(szImeFileName, szImeDestPath) != 0 &&
        !Imm32CopyFile(szImeFileName, szImeDestPath))
    {
        hNewKL = NULL;
        goto Quit;
    }

    if (hNewKL == NULL)
        hNewKL = Imm32GetNextHKL(cLayouts, pLayouts, wLangID);

    if (hNewKL)
    {
        /* Write the IME layout to registry */
        if (Imm32WriteRegIme(hNewKL, pchFilePart, lpszLayoutText))
        {
            /* Load the keyboard layout */
            Imm32UIntToStr((DWORD)(DWORD_PTR)hNewKL, 16, szImeKey, _countof(szImeKey));
            hNewKL = LoadKeyboardLayoutW(szImeKey, KLF_REPLACELANG);
        }
        else
        {
            hNewKL = NULL;
        }
    }

Quit:
    Imm32HeapFree(pLayouts);
    return hNewKL;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    IMEINFOEX info;
    TRACE("(%p)\n", hKL);
    /* FIXME: ImmGetImeInfoEx is broken */
    return !!ImmGetImeInfoEx(&info, 1, &hKL);
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    if (!Imm32IsImmMode())
        return NULL;

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    if (hWnd == NULL)
        return (HWND)NtUserGetThreadState(3);

    return (HWND)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_IME);
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
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
 *              ImmDisableLegacyIME(IMM32.@)
 */
BOOL WINAPI ImmDisableLegacyIME(void)
{
    FIXME("stub\n");
    return TRUE;
}

/***********************************************************************
 *              ImmGetImeInfoEx (IMM32.@)
 */
BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey)
{
    BOOL bDisabled = FALSE;
    HKL hKL;

    /* FIXME: broken */
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
        if (Imm32IsCiceroMode())
        {
            if (Imm32Is16BitMode())
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
    Imm32HeapFree(pImeDpi);

    RtlLeaveCriticalSection(&g_csImeDpi);
}

/***********************************************************************
 *		ImmLoadIME (IMM32.@)
 */
BOOL WINAPI ImmLoadIME(HKL hKL)
{
    PIMEDPI pImeDpi;

    if (!IS_IME_HKL(hKL) && (!Imm32IsCiceroMode() || Imm32Is16BitMode()))
        return FALSE;

    pImeDpi = Imm32FindImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = Ime32LoadImeDpi(hKL, FALSE);
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
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA( HKL hKL, LPSTR lpszFileName, UINT uBufLen)
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

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi) || !lpData)
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
                szW[_countof(szW) - 1] = 0;
                WideCharToMultiByte(pImeDpi->uCodePage, 0, szW, -1,
                                    lpData, MAX_IMM_FILENAME, NULL, NULL);
                ((LPSTR)lpData)[MAX_IMM_FILENAME - 1] = 0;
            }
            break;

        case IME_ESC_SET_EUDC_DICTIONARY:
        case IME_ESC_HANJA_MODE:
            MultiByteToWideChar(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                lpData, -1, szW, _countof(szW));
            szW[_countof(szW) - 1] = 0;
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, szW, hKL);
            break;

        default:
            ret = ImeDpi_Escape(pImeDpi, hIMC, uSubFunc, lpData, hKL);
            break;
    }

    ImmUnlockImeDpi(pImeDpi);
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

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi) || !lpData)
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
                ((LPWSTR)lpData)[MAX_IMM_FILENAME - 1] = 0;
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
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    DWORD dwConversion;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    BOOL bHasChange = FALSE;

    TRACE("(%p, %d)\n", hIMC, fOpen);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
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
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                          IMC_SETOPENSTATUS, IMN_SETOPENSTATUS, 0);
        NtUserNotifyIMEStatus(hWnd, fOpen, dwConversion);
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
    if (pIC == NULL)
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

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->ptStatusWndPos = *lpptPos;
    pIC->fdwInit |= INIT_STATUSWNDPOS;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
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
    if (!pIC)
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
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pIC->cfCompForm = *lpCompForm;
    pIC->fdwInit |= INIT_COMPFORM;

    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
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
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
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
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
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
    LCID lcid;
    HWND hWnd;
    PTEB pTeb;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (bWide)
    {
        LogFontAnsiToWide(lplf, &lfW);
        return ImmSetCompositionFontW(hIMC, &lfW);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE && !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageA(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.A = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
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
    PTEB pTeb;
    LCID lcid;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (!bWide)
    {
        LogFontWideToAnsi(lplf, &lfA);
        return ImmSetCompositionFontA(hIMC, &lfA);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE &&
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

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
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

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_a(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (pImeDpi == NULL)
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi))
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcW = Imm32WideFromAnsi(pSrc);
        if (pszSrcW == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (pCL == NULL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, pCL, cb, uFlag);
    if (cb == 0)
        goto Quit;

    ret = CandidateListWideToAnsi(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    Imm32HeapFree(pszSrcW);
    Imm32HeapFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
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

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_w(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi))
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcA = Imm32AnsiFromWide(pSrc);
        if (pszSrcA == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (!pCL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, pCL, cb, uFlag);
    if (!cb)
        goto Quit;

    ret = CandidateListAnsiToWide(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    Imm32HeapFree(pszSrcA);
    Imm32HeapFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
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
    if (!pIC)
        return FALSE;

    if (lpfdwConversion)
        *lpfdwConversion = pIC->fdwConversion;
    if (lpfdwSentence)
        *lpfdwSentence = pIC->fdwSentence;

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
    if (!IS_IME_HKL(hKL) && Imm32IsCiceroMode() && !Imm32Is16BitMode())
        fUseCicero = TRUE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
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
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldConversion,
                          IMC_SETCONVERSIONMODE, IMN_SETCONVERSIONMODE, 0);
        if (fConversionChange)
            NtUserNotifyIMEStatus(hWnd, fOpen, fdwConversion);
    }

    if (fSentenceChange || fUseCicero)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldSentence,
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

    TRACE("(%p, %p, 0x%lX, %p)", hKL, hWnd, dwMode, lpData);

    if (!ValidateHwndNoErr(hWnd) || Imm32IsCrossProcessAccess(hWnd))
        return FALSE;

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    RtlZeroMemory(&RegWordW, sizeof(RegWordW));

    if (!ImeDpi_IsUnicode(pImeDpi) || !lpData || dwMode != IME_CONFIG_REGISTERWORD)
        goto DoIt;

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
    Imm32HeapFree(RegWordW.lpReading);
    Imm32HeapFree(RegWordW.lpWord);
    ImmUnlockImeDpi(pImeDpi);
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

    TRACE("(%p, %p, 0x%lX, %p)", hKL, hWnd, dwMode, lpData);

    if (!ValidateHwndNoErr(hWnd) || Imm32IsCrossProcessAccess(hWnd))
        return FALSE;

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    RtlZeroMemory(&RegWordA, sizeof(RegWordA));

    if (ImeDpi_IsUnicode(pImeDpi) || !lpData || dwMode != IME_CONFIG_REGISTERWORD)
        goto DoIt;

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
    Imm32HeapFree(RegWordA.lpReading);
    Imm32HeapFree(RegWordA.lpWord);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsA(HIMC hIMC, DWORD dwFlags, DWORD dwType,
                    LPIMEMENUITEMINFOA lpImeParentMenu,
                    LPIMEMENUITEMINFOA lpImeMenu, DWORD dwSize)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n",
          hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize);
    return Imm32GetImeMenuItemsAW(hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu,
                                  dwSize, TRUE);
}

/***********************************************************************
 *		ImmGetImeMenuItemsW (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsW(HIMC hIMC, DWORD dwFlags, DWORD dwType,
                    LPIMEMENUITEMINFOW lpImeParentMenu,
                    LPIMEMENUITEMINFOW lpImeMenu, DWORD dwSize)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n",
          hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize);
    return Imm32GetImeMenuItemsAW(hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu,
                                  dwSize, FALSE);
}
