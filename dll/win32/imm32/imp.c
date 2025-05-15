/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing ImmIMP* functions
 * COPYRIGHT:   Copyright 2020-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static VOID
Imm32ConvertImeProWideToAnsi(_In_ const IMEPROW *pProW, _Out_ PIMEPROA pProA)
{
    pProA->hWnd = pProW->hWnd;
    pProA->InstDate = pProW->InstDate;
    pProA->wVersion = pProW->wVersion;

    WideCharToMultiByte(CP_ACP, 0, pProW->szDescription, -1,
                        (PSTR)pProA->szDescription, _countof(pProA->szDescription), NULL, NULL);
    pProA->szDescription[_countof(pProA->szDescription) - 1] = ANSI_NULL; /* Avoid buffer overrun */

    WideCharToMultiByte(CP_ACP, 0, pProW->szName, -1,
                        (PSTR)pProA->szName, _countof(pProA->szName), NULL, NULL);
    pProA->szName[_countof(pProA->szName) - 1] = ANSI_NULL; /* Avoid buffer overrun */

    pProA->szOptions[0] = ANSI_NULL;
}

static BOOL
Imm32IMPGetIME(_In_ HKL hKL, _Out_ PIMEPROW pProW)
{
    IMEINFOEX ImeInfoEx;
    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
        return FALSE;

    pProW->hWnd = NULL;
    ZeroMemory(&pProW->InstDate, sizeof(pProW->InstDate));
    pProW->wVersion = ImeInfoEx.dwImeWinVersion;

    StringCchCopyNW(pProW->szDescription, _countof(ImeInfoEx.wszImeDescription),
                    ImeInfoEx.wszImeDescription, _countof(ImeInfoEx.wszImeDescription));
    StringCchCopyNW(pProW->szName, _countof(pProW->szName),
                    ImeInfoEx.wszImeFile, _countof(ImeInfoEx.wszImeFile));
    pProW->szOptions[0] = UNICODE_NULL;

    return TRUE;
}

/***********************************************************************
 *		ImmIMPGetIMEA(IMM32.@)
 */
BOOL WINAPI
ImmIMPGetIMEA(_In_opt_ HWND hWnd, _Out_ LPIMEPROA pImePro)
{
    UNREFERENCED_PARAMETER(hWnd);

    TRACE("(%p, %p)\n", hWnd, pImePro);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    IMEPROW ImeProW;
    HKL hKL = GetKeyboardLayout(0);
    if (!Imm32IMPGetIME(hKL, &ImeProW))
        return FALSE;

    Imm32ConvertImeProWideToAnsi(&ImeProW, pImePro);
    return TRUE;
}

/***********************************************************************
 *		ImmIMPGetIMEW(IMM32.@)
 */
BOOL WINAPI
ImmIMPGetIMEW(_In_opt_ HWND hWnd, _Out_ LPIMEPROW pImePro)
{
    UNREFERENCED_PARAMETER(hWnd);

    TRACE("(%p, %p)\n", hWnd, pImePro);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    HKL hKL = GetKeyboardLayout(0);
    return Imm32IMPGetIME(hKL, pImePro);
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
