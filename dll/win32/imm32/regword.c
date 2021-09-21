/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 registering/unregistering words
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

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

/*
 * These functions absorb the difference between Ansi and Wide.
 */
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
    Imm32HeapFree(pszReadingW);
    Imm32HeapFree(pszRegisterW);
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
    Imm32HeapFree(pszReadingA);
    Imm32HeapFree(pszRegisterA);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI
ImmEnumRegisterWordA(HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
                     LPCSTR lpszReading, DWORD dwStyle,
                     LPCSTR lpszRegister, LPVOID lpData)
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

    if (!ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingW);
    Imm32HeapFree(pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI
ImmEnumRegisterWordW(HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
                     LPCWSTR lpszReading, DWORD dwStyle,
                     LPCWSTR lpszRegister, LPVOID lpData)
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

    if (ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingA);
    Imm32HeapFree(pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
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

    if (!ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pNewStylesW);
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

    if (ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pNewStylesA);
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

    if (!ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingW);
    Imm32HeapFree(pszRegisterW);
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

    if (ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingA);
    Imm32HeapFree(pszRegisterA);
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

    if (!ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingW);
    Imm32HeapFree(pszUnregisterW);
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

    if (ImeDpi_IsUnicode(pImeDpi))
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
    Imm32HeapFree(pszReadingA);
    Imm32HeapFree(pszUnregisterA);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}
