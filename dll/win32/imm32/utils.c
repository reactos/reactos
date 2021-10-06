/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 helper functions
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

HANDLE g_hImm32Heap = NULL;

BOOL WINAPI Imm32IsImcAnsi(HIMC hIMC)
{
    BOOL ret;
    PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return -1;
    ret = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);
    return ret;
}

LPWSTR APIENTRY Imm32WideFromAnsi(LPCSTR pszA)
{
    INT cch = lstrlenA(pszA);
    LPWSTR pszW = Imm32HeapAlloc(0, (cch + 1) * sizeof(WCHAR));
    if (pszW == NULL)
        return NULL;
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, cch, pszW, cch + 1);
    pszW[cch] = 0;
    return pszW;
}

LPSTR APIENTRY Imm32AnsiFromWide(LPCWSTR pszW)
{
    INT cchW = lstrlenW(pszW);
    INT cchA = (cchW + 1) * sizeof(WCHAR);
    LPSTR pszA = Imm32HeapAlloc(0, cchA);
    if (!pszA)
        return NULL;
    cchA = WideCharToMultiByte(CP_ACP, 0, pszW, cchW, pszA, cchA, NULL, NULL);
    pszA[cchA] = 0;
    return pszA;
}

DWORD APIENTRY IchWideFromAnsi(DWORD cchAnsi, LPCSTR pchAnsi, UINT uCodePage)
{
    DWORD cchWide;
    for (cchWide = 0; cchAnsi; ++cchWide)
    {
        if (IsDBCSLeadByteEx(uCodePage, *pchAnsi))
        {
            if (cchAnsi <= 1)
            {
                ++cchWide;
                break;
            }
            else
            {
                cchAnsi -= 2;
                pchAnsi += 2;
            }
        }
        else
        {
            --cchAnsi;
            ++pchAnsi;
        }
    }
    return cchWide;
}

DWORD APIENTRY IchAnsiFromWide(DWORD cchWide, LPCWSTR pchWide, UINT uCodePage)
{
    DWORD cb, cchAnsi;
    for (cchAnsi = 0; cchWide; ++cchAnsi, ++pchWide, --cchWide)
    {
        cb = WideCharToMultiByte(uCodePage, 0, pchWide, 1, NULL, 0, NULL, NULL);
        if (cb > 1)
            ++cchAnsi;
    }
    return cchAnsi;
}

BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName)
{
    if (!pszFileName[0] || !GetSystemDirectoryW(pszPath, cchPath))
        return FALSE;
    StringCchCatW(pszPath, cchPath, L"\\");
    StringCchCatW(pszPath, cchPath, pszFileName);
    return TRUE;
}

VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW)
{
    size_t cch;
    RtlCopyMemory(plfW, plfA, offsetof(LOGFONTA, lfFaceName));
    StringCchLengthA(plfA->lfFaceName, _countof(plfA->lfFaceName), &cch);
    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, plfA->lfFaceName, (INT)cch,
                              plfW->lfFaceName, _countof(plfW->lfFaceName));
    if (cch > _countof(plfW->lfFaceName) - 1)
        cch = _countof(plfW->lfFaceName) - 1;
    plfW->lfFaceName[cch] = 0;
}

VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA)
{
    size_t cch;
    RtlCopyMemory(plfA, plfW, offsetof(LOGFONTW, lfFaceName));
    StringCchLengthW(plfW->lfFaceName, _countof(plfW->lfFaceName), &cch);
    cch = WideCharToMultiByte(CP_ACP, 0, plfW->lfFaceName, (INT)cch,
                              plfA->lfFaceName, _countof(plfA->lfFaceName), NULL, NULL);
    if (cch > _countof(plfA->lfFaceName) - 1)
        cch = _countof(plfA->lfFaceName) - 1;
    plfA->lfFaceName[cch] = 0;
}

LPVOID FASTCALL ValidateHandleNoErr(HANDLE hObject, UINT uType)
{
    INT index;
    PUSER_HANDLE_TABLE ht;
    PUSER_HANDLE_ENTRY he;
    WORD generation;

    if (!NtUserValidateHandleSecure(hObject))
        return NULL;

    ht = g_SharedInfo.aheList; /* handle table */
    ASSERT(ht);
    /* ReactOS-Specific! */
    ASSERT(g_SharedInfo.ulSharedDelta != 0);
    he = (PUSER_HANDLE_ENTRY)((ULONG_PTR)ht->handles - g_SharedInfo.ulSharedDelta);

    index = (LOWORD(hObject) - FIRST_USER_HANDLE) >> 1;
    if (index < 0 || ht->nb_handles <= index || he[index].type != uType)
        return NULL;

    generation = HIWORD(hObject);
    if (generation != he[index].generation && generation && generation != 0xFFFF)
        return NULL;

    return &he[index];
}

PWND FASTCALL ValidateHwndNoErr(HWND hwnd)
{
    /* See if the window is cached */
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    if (hwnd == ClientInfo->CallbackWnd.hWnd)
        return ClientInfo->CallbackWnd.pWnd;

    return ValidateHandleNoErr(hwnd, TYPE_WINDOW);
}

BOOL APIENTRY Imm32CheckImcProcess(PIMC pIMC)
{
    HIMC hIMC;
    DWORD dwProcessID;
    if (pIMC->head.pti == NtCurrentTeb()->Win32ThreadInfo)
        return TRUE;

    hIMC = pIMC->head.h;
    dwProcessID = NtUserQueryInputContext(hIMC, 0);
    return dwProcessID == (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
}

LPVOID APIENTRY Imm32HeapAlloc(DWORD dwFlags, DWORD dwBytes)
{
    if (!g_hImm32Heap)
    {
        g_hImm32Heap = RtlGetProcessHeap();
        if (g_hImm32Heap == NULL)
            return NULL;
    }
    return HeapAlloc(g_hImm32Heap, dwFlags, dwBytes);
}

BOOL APIENTRY
Imm32NotifyAction(HIMC hIMC, HWND hwnd, DWORD dwAction, DWORD_PTR dwIndex, DWORD_PTR dwValue,
                  DWORD_PTR dwCommand, DWORD_PTR dwData)
{
    DWORD dwThreadId;
    HKL hKL;
    PIMEDPI pImeDpi;

    if (dwAction)
    {
        dwThreadId = NtUserQueryInputContext(hIMC, 1);
        if (dwThreadId)
        {
            /* find keyboard layout and lock it */
            hKL = GetKeyboardLayout(dwThreadId);
            pImeDpi = ImmLockImeDpi(hKL);
            if (pImeDpi)
            {
                /* do notify */
                pImeDpi->NotifyIME(hIMC, dwAction, dwIndex, dwValue);

                ImmUnlockImeDpi(pImeDpi); /* unlock */
            }
        }
    }

    if (hwnd && dwCommand)
        SendMessageW(hwnd, WM_IME_NOTIFY, dwCommand, dwData);

    return TRUE;
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
        Imm32HeapFree(phNewList);
        if (cRetry++ >= MAX_RETRY)
            return 0;

        phNewList = Imm32HeapAlloc(0, dwCount * sizeof(HIMC));
        if (phNewList == NULL)
            return 0;

        Status = NtUserBuildHimcList(dwThreadId, dwCount, phNewList, &dwCount);
    }

    if (NT_ERROR(Status) || !dwCount)
    {
        Imm32HeapFree(phNewList);
        return 0;
    }

    *pphList = phNewList;
    return dwCount;
#undef INITIAL_COUNT
#undef MAX_RETRY
}

INT APIENTRY
Imm32ImeMenuAnsiToWide(const IMEMENUITEMINFOA *pItemA, LPIMEMENUITEMINFOW pItemW,
                       UINT uCodePage, BOOL bBitmap)
{
    INT ret;
    pItemW->cbSize = pItemA->cbSize;
    pItemW->fType = pItemA->fType;
    pItemW->fState = pItemA->fState;
    pItemW->wID = pItemA->wID;
    if (bBitmap)
    {
        pItemW->hbmpChecked = pItemA->hbmpChecked;
        pItemW->hbmpUnchecked = pItemA->hbmpUnchecked;
        pItemW->hbmpItem = pItemA->hbmpItem;
    }
    pItemW->dwItemData = pItemA->dwItemData;
    ret = MultiByteToWideChar(uCodePage, 0, pItemA->szString, -1,
                              pItemW->szString, _countof(pItemW->szString));
    if (ret >= _countof(pItemW->szString))
    {
        ret = 0;
        pItemW->szString[0] = 0;
    }
    return ret;
}

INT APIENTRY
Imm32ImeMenuWideToAnsi(const IMEMENUITEMINFOW *pItemW, LPIMEMENUITEMINFOA pItemA,
                       UINT uCodePage)
{
    INT ret;
    pItemA->cbSize = pItemW->cbSize;
    pItemA->fType = pItemW->fType;
    pItemA->fState = pItemW->fState;
    pItemA->wID = pItemW->wID;
    pItemA->hbmpChecked = pItemW->hbmpChecked;
    pItemA->hbmpUnchecked = pItemW->hbmpUnchecked;
    pItemA->dwItemData = pItemW->dwItemData;
    pItemA->hbmpItem = pItemW->hbmpItem;
    ret = WideCharToMultiByte(uCodePage, 0, pItemW->szString, -1,
                              pItemA->szString, _countof(pItemA->szString), NULL, NULL);
    if (ret >= _countof(pItemA->szString))
    {
        ret = 0;
        pItemA->szString[0] = 0;
    }
    return ret;
}

PIME_STATE APIENTRY
Imm32FetchImeState(LPINPUTCONTEXTDX pIC, HKL hKL)
{
    PIME_STATE pState;
    WORD Lang = PRIMARYLANGID(LOWORD(hKL));
    for (pState = pIC->pState; pState; pState = pState->pNext)
    {
        if (pState->wLang == Lang)
            break;
    }
    if (!pState)
    {
        pState = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(IME_STATE));
        if (pState)
        {
            pState->wLang = Lang;
            pState->pNext = pIC->pState;
            pIC->pState = pState;
        }
    }
    return pState;
}

PIME_SUBSTATE APIENTRY
Imm32FetchImeSubState(PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState;
    for (pSubState = pState->pSubState; pSubState; pSubState = pSubState->pNext)
    {
        if (pSubState->hKL == hKL)
            return pSubState;
    }
    pSubState = Imm32HeapAlloc(0, sizeof(IME_SUBSTATE));
    if (!pSubState)
        return NULL;
    pSubState->dwValue = 0;
    pSubState->hKL = hKL;
    pSubState->pNext = pState->pSubState;
    pState->pSubState = pSubState;
    return pSubState;
}

BOOL APIENTRY
Imm32LoadImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState = Imm32FetchImeSubState(pState, hKL);
    if (pSubState)
    {
        pIC->fdwSentence |= pSubState->dwValue;
        return TRUE;
    }
    return FALSE;
}

BOOL APIENTRY
Imm32SaveImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL)
{
    PIME_SUBSTATE pSubState = Imm32FetchImeSubState(pState, hKL);
    if (pSubState)
    {
        pSubState->dwValue = (pIC->fdwSentence & 0xffff0000);
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID)
{
    return !!(GetWin32ClientInfo()->CI_flags & CI_TFSDISABLED);
}

/***********************************************************************
 *		ImmCreateIMCC(IMM32.@)
 */
HIMCC WINAPI ImmCreateIMCC(DWORD size)
{
    if (size < sizeof(DWORD))
        size = sizeof(DWORD);
    return LocalAlloc(LHND, size);
}

/***********************************************************************
 *       ImmDestroyIMCC(IMM32.@)
 */
HIMCC WINAPI ImmDestroyIMCC(HIMCC block)
{
    if (block)
        return LocalFree(block);
    return NULL;
}

/***********************************************************************
 *		ImmLockIMCC(IMM32.@)
 */
LPVOID WINAPI ImmLockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalLock(imcc);
    return NULL;
}

/***********************************************************************
 *		ImmUnlockIMCC(IMM32.@)
 */
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalUnlock(imcc);
    return FALSE;
}

/***********************************************************************
 *		ImmGetIMCCLockCount(IMM32.@)
 */
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    return LocalFlags(imcc) & LMEM_LOCKCOUNT;
}

/***********************************************************************
 *		ImmReSizeIMCC(IMM32.@)
 */
HIMCC WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    if (!imcc)
        return NULL;
    return LocalReAlloc(imcc, size, LHND);
}

/***********************************************************************
 *		ImmGetIMCCSize(IMM32.@)
 */
DWORD WINAPI ImmGetIMCCSize(HIMCC imcc)
{
    if (imcc)
        return LocalSize(imcc);
    return 0;
}

/***********************************************************************
 *		ImmGetIMCLockCount(IMM32.@)
 */
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    DWORD ret;
    HANDLE hInputContext;
    PCLIENTIMC pClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return 0;

    ret = 0;
    hInputContext = pClientImc->hInputContext;
    if (hInputContext)
        ret = (LocalFlags(hInputContext) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmIMPGetIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPGetIMEA(HWND hWnd, LPIMEPROA pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPGetIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPGetIMEW(HWND hWnd, LPIMEPROW pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPQueryIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPQueryIMEA(LPIMEPROA pImePro)
{
    FIXME("(%p)\n", pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPQueryIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPQueryIMEW(LPIMEPROW pImePro)
{
    FIXME("(%p)\n", pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPSetIMEA(IMM32.@)
 */
BOOL WINAPI ImmIMPSetIMEA(HWND hWnd, LPIMEPROA pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}

/***********************************************************************
 *		ImmIMPSetIMEW(IMM32.@)
 */
BOOL WINAPI ImmIMPSetIMEW(HWND hWnd, LPIMEPROW pImePro)
{
    FIXME("(%p, %p)\n", hWnd, pImePro);
    return FALSE;
}
