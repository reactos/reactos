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
    UINT uCodePage;
} ENUM_WORD_A2W, *LPENUM_WORD_A2W;

typedef struct ENUM_WORD_W2A
{
    REGISTERWORDENUMPROCA lpfnEnumProc;
    LPVOID lpData;
    UINT ret;
    UINT uCodePage;
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
        pszReadingW = Imm32WideFromAnsi(lpEnumData->uCodePage, pszReadingA);
        if (IS_NULL_UNEXPECTEDLY(pszReadingW))
            goto Quit;
    }

    if (pszRegisterA)
    {
        pszRegisterW = Imm32WideFromAnsi(lpEnumData->uCodePage, pszRegisterA);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterW))
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingW, dwStyle, pszRegisterW, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    ImmLocalFree(pszReadingW);
    ImmLocalFree(pszRegisterW);
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
        pszReadingA = Imm32AnsiFromWide(lpEnumData->uCodePage, pszReadingW);
        if (IS_NULL_UNEXPECTEDLY(pszReadingW))
            goto Quit;
    }

    if (pszRegisterW)
    {
        pszRegisterA = Imm32AnsiFromWide(lpEnumData->uCodePage, pszRegisterW);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterA))
            goto Quit;
    }

    ret = lpEnumData->lpfnEnumProc(pszReadingA, dwStyle, pszRegisterA, lpEnumData->lpData);
    lpEnumData->ret = ret;

Quit:
    ImmLocalFree(pszReadingA);
    ImmLocalFree(pszRegisterA);
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

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)\n", hKL, lpfnEnumProc, debugstr_a(lpszReading),
          dwStyle, debugstr_a(lpszRegister), lpData);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingW))
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszRegister);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterW))
            goto Quit;
    }

    EnumDataW2A.lpfnEnumProc = lpfnEnumProc;
    EnumDataW2A.lpData = lpData;
    EnumDataW2A.ret = 0;
    EnumDataW2A.uCodePage = pImeDpi->uCodePage;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcW2A, pszReadingW, dwStyle,
                                 pszRegisterW, &EnumDataW2A);
    ret = EnumDataW2A.ret;

Quit:
    ImmLocalFree(pszReadingW);
    ImmLocalFree(pszRegisterW);
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

    TRACE("(%p, %p, %s, 0x%lX, %s, %p)\n", hKL, lpfnEnumProc, debugstr_w(lpszReading),
          dwStyle, debugstr_w(lpszRegister), lpData);

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                           lpszRegister, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingA))
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszRegister);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterA))
            goto Quit;
    }

    EnumDataA2W.lpfnEnumProc = lpfnEnumProc;
    EnumDataA2W.lpData = lpData;
    EnumDataA2W.ret = 0;
    EnumDataA2W.uCodePage = pImeDpi->uCodePage;
    pImeDpi->ImeEnumRegisterWord(Imm32EnumWordProcA2W, pszReadingA, dwStyle,
                                 pszRegisterA, &EnumDataA2W);
    ret = EnumDataA2W.ret;

Quit:
    ImmLocalFree(pszReadingA);
    ImmLocalFree(pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %u\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (!ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesW = ImmLocalAlloc(0, nItem * sizeof(STYLEBUFW));
        if (IS_NULL_UNEXPECTEDLY(pNewStylesW))
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
            cchA = WideCharToMultiByte(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                       pSrcW->szDescription, (INT)cchW,
                                       pDestA->szDescription, _countof(pDestA->szDescription),
                                       NULL, NULL);
            if (cchA > _countof(pDestA->szDescription) - 1)
                cchA = _countof(pDestA->szDescription) - 1;
            pDestA->szDescription[cchA] = 0;
        }
    }

Quit:
    ImmLocalFree(pNewStylesW);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %u\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return 0;

    if (ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeGetRegisterWordStyle(nItem, lpStyleBuf);
        goto Quit;
    }

    if (nItem > 0)
    {
        pNewStylesA = ImmLocalAlloc(0, nItem * sizeof(STYLEBUFA));
        if (IS_NULL_UNEXPECTEDLY(pNewStylesA))
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
            cchW = MultiByteToWideChar(pImeDpi->uCodePage, MB_PRECOMPOSED,
                                       pSrcA->szDescription, (INT)cchA,
                                       pDestW->szDescription, _countof(pDestW->szDescription));
            if (cchW > _countof(pDestW->szDescription) - 1)
                cchW = _countof(pDestW->szDescription) - 1;
            pDestW->szDescription[cchW] = 0;
        }
    }

Quit:
    ImmLocalFree(pNewStylesA);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %u\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    if (!ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingW))
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszRegister);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterW))
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingW, dwStyle, pszRegisterW);

Quit:
    ImmLocalFree(pszReadingW);
    ImmLocalFree(pszRegisterW);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    if (ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeRegisterWord(lpszReading, dwStyle, lpszRegister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingA))
            goto Quit;
    }

    if (lpszRegister)
    {
        pszRegisterA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszRegister);
        if (IS_NULL_UNEXPECTEDLY(pszRegisterA))
            goto Quit;
    }

    ret = pImeDpi->ImeRegisterWord(pszReadingA, dwStyle, pszRegisterA);

Quit:
    ImmLocalFree(pszReadingA);
    ImmLocalFree(pszRegisterA);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    if (!ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingW))
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterW = Imm32WideFromAnsi(pImeDpi->uCodePage, lpszUnregister);
        if (IS_NULL_UNEXPECTEDLY(pszUnregisterW))
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingW, dwStyle, pszUnregisterW);

Quit:
    ImmLocalFree(pszReadingW);
    ImmLocalFree(pszUnregisterW);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
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

    pImeDpi = Imm32FindOrLoadImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return FALSE;

    if (ImeDpi_IsUnicode(pImeDpi)) /* No conversion needed */
    {
        ret = pImeDpi->ImeUnregisterWord(lpszReading, dwStyle, lpszUnregister);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (lpszReading)
    {
        pszReadingA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszReading);
        if (IS_NULL_UNEXPECTEDLY(pszReadingA))
            goto Quit;
    }

    if (lpszUnregister)
    {
        pszUnregisterA = Imm32AnsiFromWide(pImeDpi->uCodePage, lpszUnregister);
        if (IS_NULL_UNEXPECTEDLY(pszUnregisterA))
            goto Quit;
    }

    ret = pImeDpi->ImeUnregisterWord(pszReadingA, dwStyle, pszUnregisterA);

Quit:
    ImmLocalFree(pszReadingA);
    ImmLocalFree(pszUnregisterA);
    ImmUnlockImeDpi(pImeDpi);
    TRACE("ret: %d\n", ret);
    return ret;
}
