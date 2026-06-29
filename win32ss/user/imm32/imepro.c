/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing ImmIMP* functions
 * COPYRIGHT:   Copyright 2020-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/*
 * An IMEPROA/IMEPROW structure is an IME program information.
 * The ImmIMP* functions just treat these information.
 */

static VOID
Imm32ConvertImeProWideToAnsi(
    _In_ const IMEPROW *pProW,
    _Out_ PIMEPROA pProA)
{
    ASSERT(pProW);
    ASSERT(pProA);

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
Imm32IMPGetIME(
    _In_ HKL hKL,
    _Out_ PIMEPROW pProW)
{
    ASSERT(pProW);

    IMEINFOEX ImeInfoEx;
    if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
        return FALSE;

    pProW->hWnd = NULL;
    ZeroMemory(&pProW->InstDate, sizeof(pProW->InstDate));
    pProW->wVersion = ImeInfoEx.dwImeWinVersion;

    StringCchCopyNW(pProW->szDescription, _countof(pProW->szDescription),
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
ImmIMPGetIMEA(
    _In_opt_ HWND hWnd,
    _Out_ LPIMEPROA pImePro)
{
    UNREFERENCED_PARAMETER(hWnd);

    TRACE("(%p, %p)\n", hWnd, pImePro);

    ASSERT(pImePro);

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
ImmIMPGetIMEW(
    _In_opt_ HWND hWnd,
    _Out_ LPIMEPROW pImePro)
{
    UNREFERENCED_PARAMETER(hWnd);

    TRACE("(%p, %p)\n", hWnd, pImePro);

    ASSERT(pImePro);

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
BOOL WINAPI
ImmIMPQueryIMEA(_Inout_ LPIMEPROA pImePro)
{
    TRACE("(%p)\n", pImePro);

    ASSERT(pImePro);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    IMEPROW ProW;
    if (pImePro->szName[0])
    {
        /* pImePro->szName is BYTE[], so we need type cast */
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (PSTR)pImePro->szName, -1,
                                 ProW.szName, _countof(ProW.szName)))
        {
            ERR("szName: %s\n", debugstr_a((PSTR)pImePro->szName));
            return FALSE;
        }
        ProW.szName[_countof(ProW.szName) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
    }
    else
    {
        ProW.szName[0] = UNICODE_NULL;
    }

    if (!ImmIMPQueryIMEW(&ProW))
        return FALSE;

    Imm32ConvertImeProWideToAnsi(&ProW, pImePro);
    return TRUE;
}

/***********************************************************************
 *		ImmIMPQueryIMEW(IMM32.@)
 */
BOOL WINAPI
ImmIMPQueryIMEW(_Inout_ LPIMEPROW pImePro)
{
    TRACE("(%p)\n", pImePro);

    ASSERT(pImePro);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    INT nLayouts = GetKeyboardLayoutList(0, NULL);
    if (nLayouts <= 0)
    {
        ERR("nLayouts: %d\n", nLayouts);
        return FALSE;
    }

    HKL *phKLs = ImmLocalAlloc(0, nLayouts * sizeof(HKL));
    if (!phKLs)
    {
        ERR("Out of memory\n");
        return FALSE;
    }

    if (GetKeyboardLayoutList(nLayouts, phKLs) != nLayouts)
    {
        ERR("KL count mismatch\n");
        ImmLocalFree(phKLs);
        return FALSE;
    }

    BOOL result = FALSE;
    if (pImePro->szName[0])
    {
        IMEINFOEX ImeInfoEx;
        if (ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExImeFileName, pImePro->szName))
        {
            for (INT iKL = 0; iKL < nLayouts; ++iKL)
            {
                if (phKLs[iKL] == ImeInfoEx.hkl)
                {
                    result = Imm32IMPGetIME(phKLs[iKL], pImePro);
                    break;
                }
            }
        }
    }
    else
    {
        for (INT iKL = 0; iKL < nLayouts; ++iKL)
        {
            result = Imm32IMPGetIME(phKLs[iKL], pImePro);
            if (result)
                break;
        }
    }

    ImmLocalFree(phKLs);
    return result;
}

/***********************************************************************
 *		ImmIMPSetIMEA(IMM32.@)
 */
BOOL WINAPI
ImmIMPSetIMEA(
    _In_opt_ HWND hWnd,
    _Inout_ LPIMEPROA pImePro)
{
    TRACE("(%p, %p)\n", hWnd, pImePro);

    ASSERT(pImePro);

    IMEPROW ProW;
    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (pImePro->szName[0])
    {
        /* pImePro->szName is BYTE[], so we need type cast */
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (PSTR)pImePro->szName, -1,
                                 ProW.szName, _countof(ProW.szName)))
        {
            ERR("szName: %s\n", debugstr_a((PSTR)pImePro->szName));
            return FALSE;
        }
        ProW.szName[_countof(ProW.szName) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
    }
    else
    {
        ProW.szName[0] = UNICODE_NULL;
    }

    return ImmIMPSetIMEW(hWnd, &ProW);
}

/***********************************************************************
 *		ImmIMPSetIMEW(IMM32.@)
 */
BOOL WINAPI
ImmIMPSetIMEW(
    _In_opt_ HWND hWnd,
    _Inout_ LPIMEPROW pImePro)
{
    UNREFERENCED_PARAMETER(hWnd);

    TRACE("(%p, %p)\n", hWnd, pImePro);

    ASSERT(pImePro);

    if (!Imm32IsSystemJapaneseOrKorean())
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    HKL hTargetKL = NULL;
    if (pImePro->szName[0])
    {
        IMEINFOEX ImeInfoEx;
        if (!ImmGetImeInfoEx(&ImeInfoEx, ImeInfoExImeFileName, pImePro->szName))
            return FALSE;

        hTargetKL = ImeInfoEx.hkl;
    }
    else
    {
        INT nLayouts = GetKeyboardLayoutList(0, NULL);
        if (nLayouts <= 0)
        {
            ERR("nLayouts: %d\n", nLayouts);
            return FALSE;
        }

        HKL *phKLs = ImmLocalAlloc(0, nLayouts * sizeof(HKL));
        if (!phKLs)
        {
            ERR("Out of memory\n");
            return FALSE;
        }

        if (GetKeyboardLayoutList(nLayouts, phKLs) == nLayouts)
        {
            for (INT iKL = 0; iKL < nLayouts; ++iKL)
            {
                if (!ImmIsIME(phKLs[iKL]))
                {
                    hTargetKL = phKLs[iKL];
                    break;
                }
            }
        }
        else
        {
            ERR("KL count mismatch\n");
        }

        ImmLocalFree(phKLs);
    }

    if (hTargetKL && GetKeyboardLayout(0) != hTargetKL)
    {
        HWND hwndFocus = GetFocus();
        if (hwndFocus)
        {
            PostMessageW(hwndFocus, WM_INPUTLANGCHANGEREQUEST, INPUTLANGCHANGE_SYSCHARSET,
                         (LPARAM)hTargetKL);
            return TRUE;
        }
    }

    return FALSE;
}
