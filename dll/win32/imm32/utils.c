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

HANDLE g_hImm32Heap = NULL;

BOOL APIENTRY Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName)
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

PWND FASTCALL ValidateHwndNoErr(HWND hwnd)
{
    INT index;
    PUSER_HANDLE_TABLE ht;
    WORD generation;

    if (!NtUserValidateHandleSecure(hwnd))
        return NULL;

    ht = g_SharedInfo.aheList; /* handle table */
    index = (LOWORD(hwnd) - FIRST_USER_HANDLE) >> 1;
    if (index < 0 || index >= ht->nb_handles || ht->handles[index].type != TYPE_WINDOW)
        return NULL;

    generation = HIWORD(hwnd);
    if (generation != ht->handles[index].generation && generation && generation != 0xFFFF)
        return NULL;

    return (PWND)&ht->handles[index];
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

VOID APIENTRY Imm32HeapFree(LPVOID lpData)
{
    HeapFree(g_hImm32Heap, 0, lpData);
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
