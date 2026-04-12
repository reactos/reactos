/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32 Win3.x compatibility
 * COPYRIGHT:   Copyright 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <ime.h>

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#ifdef IMM_WIN3_SUPPORT /* 3.x support */

#define ALIGN_DWORD(len) (((len) + 3) & ~3)

WORD g_dchKorean = 0; /* One or two Korean character(s) */

static DWORD
Imm32CompStrWToUndetW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDeter)
{
    DWORD dwRequiredSize =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD((pCS->dwCompStrLen + 1) * sizeof(WCHAR)) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_DWORD((pCS->dwResultStrLen + 1) * sizeof(WCHAR)) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD((pCS->dwResultReadStrLen + 1) * sizeof(WCHAR)) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pDeter)
        return dwRequiredSize;

    if (dwRequiredSize == 0)
        return 0;

    RtlZeroMemory(pDeter, sizeof(UNDETERMINESTRUCT));
    pDeter->dwSize = dwRequiredSize;

    DWORD dwCurrentOffset = sizeof(UNDETERMINESTRUCT);
    PBYTE pBase = (PBYTE)pDeter;
    const BYTE *pSrcBase = (const BYTE *)pCS;

    if ((dwGCS & GCS_COMPSTR) && (pCS->dwCompStrLen > 0))
    {
        pDeter->uUndetTextPos = dwCurrentOffset;
        pDeter->uUndetTextLen = pCS->dwCompStrLen;
        DWORD cb = pCS->dwCompStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwCompStrOffset, cb);
        ((PWCHAR)(pBase + dwCurrentOffset))[pCS->dwCompStrLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD((pCS->dwCompStrLen + 1) * sizeof(WCHAR));
    }

    if ((dwGCS & GCS_COMPATTR) && (pCS->dwCompAttrLen > 0) && (pDeter->uUndetTextLen > 0))
    {
        pDeter->uUndetAttrPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwCompAttrOffset,
                      pCS->dwCompAttrLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwCompAttrLen);
    }

    if (dwGCS & GCS_CURSORPOS)
        pDeter->uCursorPos = pCS->dwCursorPos;

    if (dwGCS & GCS_DELTASTART)
        pDeter->uDeltaStart = pCS->dwDeltaStart;

    if ((dwGCS & GCS_RESULTSTR) && (pCS->dwResultStrLen > 0))
    {
        pDeter->uDetermineTextPos = dwCurrentOffset;
        pDeter->uDetermineTextLen = pCS->dwResultStrLen;
        DWORD cb = pCS->dwResultStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultStrOffset, cb);
        ((PWCHAR)(pBase + dwCurrentOffset))[pCS->dwResultStrLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD((pCS->dwResultStrLen + 1) * sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && (pCS->dwResultClauseLen > 0) &&
        (pDeter->uDetermineTextLen > 0))
    {
        pDeter->uDetermineDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset,
                      pSrcBase + pCS->dwResultClauseOffset,
                      pCS->dwResultClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if ((dwGCS & GCS_RESULTREADSTR) && (pCS->dwResultReadStrLen > 0))
    {
        pDeter->uYomiTextPos = dwCurrentOffset;
        pDeter->uYomiTextLen = pCS->dwResultReadStrLen;
        DWORD cb = pCS->dwResultReadStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadStrOffset, cb);
        ((PWCHAR)(pBase + dwCurrentOffset))[pCS->dwResultReadStrLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD((pCS->dwResultReadStrLen + 1) * sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && (pCS->dwResultReadClauseLen > 0) &&
        (pDeter->uYomiTextLen > 0))
    {
        pDeter->uYomiDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset,
                      pSrcBase + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwRequiredSize;
}

static DWORD
Imm32CompStrWToUndetA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDeter)
{
    DWORD dwRequiredSize =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD(2 * pCS->dwCompStrLen + 3) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_DWORD(2 * pCS->dwResultStrLen + 3) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(2 * pCS->dwResultReadStrLen + 3) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen) + 60;

    if (!pDeter)
        return dwRequiredSize;

    if (dwRequiredSize == 0)
        return 0;

    RtlZeroMemory(pDeter, sizeof(UNDETERMINESTRUCT));
    pDeter->dwSize = dwRequiredSize;

    DWORD dwCurrentOffset = sizeof(UNDETERMINESTRUCT);
    PBYTE pBase = (PBYTE)pDeter;
    const BYTE *pSrcBase = (const BYTE *)pCS;
    BOOL bUsedDef;

    if ((dwGCS & GCS_COMPSTR) && (pCS->dwCompStrLen > 0))
    {
        INT nAnsiLen = WideCharToMultiByte(
            CP_ACP, 0,
            (PCWSTR)(pSrcBase + pCS->dwCompStrOffset), pCS->dwCompStrLen,
            (PSTR)(pBase + dwCurrentOffset), dwRequiredSize - dwCurrentOffset,
            NULL, &bUsedDef);
        if (nAnsiLen <= 0)
            return 0;
        pDeter->uUndetTextPos = dwCurrentOffset;
        pDeter->uUndetTextLen = nAnsiLen;
        (pBase + dwCurrentOffset)[nAnsiLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(nAnsiLen + 1);
    }

    if ((dwGCS & GCS_COMPATTR) && (pCS->dwCompAttrLen > 0) && (pDeter->uUndetTextLen > 0))
    {
        pDeter->uUndetAttrPos = dwCurrentOffset;
        const BYTE *pSrcAttr = pSrcBase + pCS->dwCompAttrOffset;
        PBYTE pDestAttr = pBase + dwCurrentOffset;
        const WCHAR *pWStr = (const WCHAR *)(pSrcBase + pCS->dwCompStrOffset);
        UINT nDestIdx = 0;

        for (UINT i = 0; i < pCS->dwCompAttrLen; i++)
        {
            ULONG cbMultiByte = 0;
            USHORT wChar = pWStr[i];
            RtlUnicodeToMultiByteSize(&cbMultiByte, &wChar, sizeof(WCHAR));
            if (cbMultiByte == 2)
            {
                pDestAttr[nDestIdx++] = pSrcAttr[i];
                pDestAttr[nDestIdx++] = pSrcAttr[i];
            }
            else
            {
                pDestAttr[nDestIdx++] = pSrcAttr[i];
            }
        }
        dwCurrentOffset += ALIGN_DWORD(nDestIdx);
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (pCS->dwCursorPos == (DWORD)-1)
        {
            pDeter->uCursorPos = (UINT)-1;
        }
        else
        {
            pDeter->uCursorPos = IchAnsiFromWide(pCS->dwCursorPos,
                (PCWSTR)(pSrcBase + pCS->dwCompStrOffset), CP_ACP);
        }
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (pCS->dwDeltaStart == (DWORD)-1)
        {
            pDeter->uDeltaStart = (UINT)-1;
        }
        else
        {
            pDeter->uDeltaStart = IchAnsiFromWide(pCS->dwDeltaStart,
                (PCWSTR)(pSrcBase + pCS->dwCompStrOffset), CP_ACP);
        }
    }

    if ((dwGCS & GCS_RESULTSTR) && (pCS->dwResultStrLen > 0))
    {
        INT nAnsiLen = WideCharToMultiByte(
            CP_ACP, 0,
            (PCWSTR)(pSrcBase + pCS->dwResultStrOffset), pCS->dwResultStrLen,
            (PSTR)(pBase + dwCurrentOffset), dwRequiredSize - dwCurrentOffset,
            NULL, &bUsedDef);
        if (nAnsiLen > 0)
        {
            pDeter->uDetermineTextPos = dwCurrentOffset;
            pDeter->uDetermineTextLen = nAnsiLen;
            (pBase + dwCurrentOffset)[nAnsiLen] = ANSI_NULL;
            dwCurrentOffset += ALIGN_DWORD(nAnsiLen + 1);
        }
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && (pCS->dwResultClauseLen > 0) &&
        (pDeter->uDetermineTextLen > 0))
    {
        pDeter->uDetermineDelimPos = dwCurrentOffset;
        const DWORD *pSrcClause = (const DWORD *)(pSrcBase + pCS->dwResultClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);
        const WCHAR *pResultW = (const WCHAR *)(pSrcBase + pCS->dwResultStrOffset);

        for (DWORD i = 0; i < nClauses; i++)
            pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pResultW, CP_ACP);

        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if ((dwGCS & GCS_RESULTREADSTR) && (pCS->dwResultReadStrLen > 0))
    {
        INT nAnsiLen = WideCharToMultiByte(
            CP_ACP, 0,
            (PCWSTR)(pSrcBase + pCS->dwResultReadStrOffset), pCS->dwResultReadStrLen,
            (PSTR)(pBase + dwCurrentOffset), dwRequiredSize - dwCurrentOffset,
            NULL, &bUsedDef);
        if (nAnsiLen > 0)
        {
            pDeter->uYomiTextPos = dwCurrentOffset;
            pDeter->uYomiTextLen = nAnsiLen;
            (pBase + dwCurrentOffset)[nAnsiLen] = ANSI_NULL;
            dwCurrentOffset += ALIGN_DWORD(nAnsiLen + 1);
        }
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && (pCS->dwResultReadClauseLen > 0) &&
        (pDeter->uYomiTextLen > 0))
    {
        pDeter->uYomiDelimPos = dwCurrentOffset;
        const DWORD *pSrcClause = (const DWORD *)(pSrcBase + pCS->dwResultReadClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
        const WCHAR *pReadW = (const WCHAR *)(pSrcBase + pCS->dwResultReadStrOffset);

        for (DWORD i = 0; i < nClauses; i++)
            pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pReadW, CP_ACP);

        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwRequiredSize;
}

static DWORD
Imm32CompStrWToStringExW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    LPSTRINGEXSTRUCT pSX,
    DWORD dwSize)
{
    DWORD dwResultStrSize    = pCS->dwResultStrLen
        ? ALIGN_DWORD((pCS->dwResultStrLen + 1) * sizeof(WCHAR)) : 0;
    DWORD dwResultClauseSize = pCS->dwResultClauseLen
        ? ALIGN_DWORD(pCS->dwResultClauseLen) : 0;
    DWORD dwResultReadStrSize = pCS->dwResultReadStrLen
        ? ALIGN_DWORD((pCS->dwResultReadStrLen + 1) * sizeof(WCHAR)) : 0;
    DWORD dwReadClauseSize   = pCS->dwResultReadClauseLen
        ? ALIGN_DWORD(pCS->dwResultReadClauseLen) : 0;
    DWORD dwRequiredSize = sizeof(STRINGEXSTRUCT) + dwResultStrSize + dwResultClauseSize +
                           dwResultReadStrSize + dwReadClauseSize;

    if (!pSX)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pSX, sizeof(STRINGEXSTRUCT));
    pSX->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(STRINGEXSTRUCT);
    PBYTE pBase = (PBYTE)pSX;
    const BYTE *pSrcBase = (const BYTE *)pCS;

    if (pCS->dwResultStrLen > 0)
    {
        pSX->uDeterminePos = dwCurrentOffset;
        DWORD cb = pCS->dwResultStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultStrOffset, cb);
        ((PWSTR)(pBase + dwCurrentOffset))[pCS->dwResultStrLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD(cb + sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen > 0 && pSX->uDeterminePos)
    {
        pSX->uDetermineDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultClauseOffset,
                      pCS->dwResultClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (pCS->dwResultReadStrLen > 0)
    {
        pSX->uYomiPos = dwCurrentOffset;
        DWORD cb = pCS->dwResultReadStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadStrOffset, cb);
        ((PWSTR)(pBase + dwCurrentOffset))[pCS->dwResultReadStrLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD(cb + sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen > 0 && pSX->uYomiPos)
    {
        pSX->uYomiDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwSize;
}

static DWORD
Imm32CompStrWToStringExA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    LPSTRINGEXSTRUCT pSX,
    DWORD dwSize)
{
    DWORD dwResultStrSize = pCS->dwResultStrLen
        ? ALIGN_DWORD(2 * pCS->dwResultStrLen + 5) : 0;
    DWORD dwResultClauseSize = pCS->dwResultClauseLen
        ? ALIGN_DWORD(pCS->dwResultClauseLen + 4) : 0;
    DWORD dwResultReadStrSize = pCS->dwResultReadStrLen
        ? ALIGN_DWORD(2 * pCS->dwResultReadStrLen + 5) : 0;
    DWORD dwReadClauseSize = pCS->dwResultReadClauseLen
        ? ALIGN_DWORD(pCS->dwResultReadClauseLen + 4) : 0;
    DWORD dwRequiredSize = sizeof(STRINGEXSTRUCT) + dwResultStrSize + dwResultClauseSize +
                           dwResultReadStrSize + dwReadClauseSize;

    if (!pSX)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pSX, sizeof(STRINGEXSTRUCT));
    pSX->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(STRINGEXSTRUCT);
    PBYTE pBase = (PBYTE)pSX;
    const BYTE *pSrcBase = (const BYTE *)pCS;
    BOOL bUsedDef;
    INT nAnsiResultLen = 0, nAnsiReadLen = 0;

    if (pCS->dwResultStrLen > 0)
    {
        pSX->uDeterminePos = dwCurrentOffset;
        nAnsiResultLen = WideCharToMultiByte(
            CP_ACP, 0,
            (PCWSTR)(pSrcBase + pCS->dwResultStrOffset), pCS->dwResultStrLen,
            (PSTR)(pBase + dwCurrentOffset), dwSize - dwCurrentOffset,
            NULL, &bUsedDef);
        if (nAnsiResultLen <= 0)
            return 0;
        (pBase + dwCurrentOffset)[nAnsiResultLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(nAnsiResultLen + 1);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen > 0 && nAnsiResultLen > 0)
    {
        pSX->uDetermineDelimPos = dwCurrentOffset;
        const DWORD *pSrcClause = (const DWORD *)(pSrcBase + pCS->dwResultClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);
        const WCHAR *pResultW = (const WCHAR *)(pSrcBase + pCS->dwResultStrOffset);

        for (DWORD i = 0; i < nClauses; i++)
            pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pResultW, CP_ACP);

        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (pCS->dwResultReadStrLen > 0)
    {
        pSX->uYomiPos = dwCurrentOffset;
        nAnsiReadLen = WideCharToMultiByte(
            CP_ACP, 0,
            (PCWSTR)(pSrcBase + pCS->dwResultReadStrOffset), pCS->dwResultReadStrLen,
            (PSTR)(pBase + dwCurrentOffset), dwSize - dwCurrentOffset,
            NULL, &bUsedDef);
        if (nAnsiReadLen <= 0)
            return 0;
        (pBase + dwCurrentOffset)[nAnsiReadLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(nAnsiReadLen + 1);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen > 0 && nAnsiReadLen > 0)
    {
        pSX->uYomiDelimPos = dwCurrentOffset;
        const DWORD *pSrcClause = (const DWORD *)(pSrcBase + pCS->dwResultReadClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
        const WCHAR *pReadW = (const WCHAR *)(pSrcBase + pCS->dwResultReadStrOffset);

        for (DWORD i = 0; i < nClauses; i++)
            pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pReadW, CP_ACP);

        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwSize;
}

static DWORD Imm32CompStrWToStringW(const COMPOSITIONSTRING *pCS, PWCHAR pch)
{
    DWORD dwResultStrLen = pCS->dwResultStrLen;
    if (!pch)
        return (dwResultStrLen + 1) * sizeof(WCHAR);
    DWORD cb = dwResultStrLen * sizeof(WCHAR);
    RtlCopyMemory(pch, (const BYTE*)pCS + pCS->dwResultStrOffset, cb);
    pch[cb / sizeof(WCHAR)] = UNICODE_NULL;
    return cb + sizeof(UNICODE_NULL);
}

static DWORD Imm32CompStrWToStringA(const COMPOSITIONSTRING *pCS, PCHAR pch)
{
    const WCHAR *pwch = (const WCHAR *)((PBYTE)pCS + pCS->dwResultStrOffset);
    BOOL bUsedDef;
    INT cchNeed = WideCharToMultiByte(CP_ACP, 0, pwch, pCS->dwResultStrLen,
                                      pch, 0, NULL, &bUsedDef) + 1;
    if (!pch)
        return cchNeed;
    INT cch = WideCharToMultiByte(CP_ACP, 0, pwch, pCS->dwResultStrLen,
                                  pch, cchNeed, NULL, &bUsedDef);
    pch[cch] = 0;
    return cch + 1;
}

const WCHAR *Imm32CompStrWToCharW(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    const WCHAR *pCurrent = (const WCHAR *)((const BYTE *)pCS + pCS->dwResultStrOffset);

    PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    const WCHAR *pNext;
    for (; *pCurrent; pCurrent = pNext)
    {
        pNext = CharNextW(pCurrent);

        if (!*pNext)
            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        PostMessageW(hWnd, WM_CHAR, *pCurrent, 1);
    }

    return pCurrent;
}

static DWORD
Imm32CompStrAToUndetA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDeter,
    DWORD dwSize)
{
    DWORD dwRequiredSize =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD(pCS->dwCompStrLen) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_DWORD(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pDeter)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pDeter, sizeof(UNDETERMINESTRUCT));
    pDeter->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(UNDETERMINESTRUCT);
    PBYTE pBase = (PBYTE)pDeter, pSrcBase = (PBYTE)pCS;

    if ((dwGCS & GCS_COMPSTR) && (pCS->dwCompStrLen > 0))
    {
        pDeter->uUndetTextPos = dwCurrentOffset;
        pDeter->uUndetTextLen = pCS->dwCompStrLen;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwCompStrOffset, pCS->dwCompStrLen);
        pBase[dwCurrentOffset + pCS->dwCompStrLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(pCS->dwCompStrLen + 1);
    }

    if ((dwGCS & GCS_COMPATTR) && (pCS->dwCompAttrLen > 0))
    {
        pDeter->uUndetAttrPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwCompAttrOffset,
                      pCS->dwCompAttrLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwCompAttrLen);
    }

    if (dwGCS & GCS_CURSORPOS)
        pDeter->uCursorPos = pCS->dwCursorPos;

    if (dwGCS & GCS_DELTASTART)
        pDeter->uDeltaStart = pCS->dwDeltaStart;

    if ((dwGCS & GCS_RESULTSTR) && (pCS->dwResultStrLen > 0))
    {
        pDeter->uDetermineTextPos = dwCurrentOffset;
        pDeter->uDetermineTextLen = pCS->dwResultStrLen;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultStrOffset,
                      pCS->dwResultStrLen);
        pBase[dwCurrentOffset + pCS->dwResultStrLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && (pCS->dwResultClauseLen > 0))
    {
        pDeter->uDetermineDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultClauseOffset,
                      pCS->dwResultClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if ((dwGCS & GCS_RESULTREADSTR) && (pCS->dwResultReadStrLen > 0))
    {
        pDeter->uYomiTextPos = dwCurrentOffset;
        pDeter->uYomiTextLen = pCS->dwResultReadStrLen;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadStrOffset,
                      pCS->dwResultReadStrLen);
        pBase[dwCurrentOffset + pCS->dwResultReadStrLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && (pCS->dwResultReadClauseLen > 0))
    {
        pDeter->uYomiDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToUndetW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pUndeter,
    DWORD dwSize)
{
    DWORD dwRequiredSize =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(2 * pCS->dwCompAttrLen + 3) +
        ALIGN_DWORD(2 * pCS->dwResultStrLen + 5) +
        ALIGN_DWORD(2 * pCS->dwCompStrLen + 5) +
        ALIGN_DWORD(2 * pCS->dwResultReadStrLen + 5) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen) + 60;

    if (!pUndeter)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pUndeter, sizeof(UNDETERMINESTRUCT));
    pUndeter->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(UNDETERMINESTRUCT);
    PBYTE pBase = (PBYTE)pUndeter;
    const BYTE* pSrcBase = (const BYTE*)pCS;

    if ((dwGCS & GCS_COMPSTR) && (pCS->dwCompStrLen > 0))
    {
        INT nWideLen = MultiByteToWideChar(CP_ACP, 0,
            (PCSTR)(pSrcBase + pCS->dwCompStrOffset), pCS->dwCompStrLen,
            (PWSTR)(pBase + dwCurrentOffset), (dwSize - dwCurrentOffset) / sizeof(WCHAR));
        if (nWideLen <= 0)
            return 0;
        pUndeter->uUndetTextPos = dwCurrentOffset;
        pUndeter->uUndetTextLen = nWideLen;
        ((PWCHAR)(pBase + dwCurrentOffset))[nWideLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));
    }

    if ((dwGCS & GCS_COMPATTR) && pCS->dwCompAttrLen > 0 && (pUndeter->uUndetTextLen > 0))
    {
        pUndeter->uUndetAttrPos = dwCurrentOffset;
        const BYTE* pSrcAttr = pSrcBase + pCS->dwCompAttrOffset;
        PBYTE pDestAttr = pBase + dwCurrentOffset;
        PCWSTR pWStr = (PCWSTR)(pBase + pUndeter->uUndetTextPos);

        for (UINT i = 0; i < pUndeter->uUndetTextLen; i++)
        {
            pDestAttr[i] = *pSrcAttr;
            USHORT wChar = pWStr[i];
            ULONG cbMultiByte = 0;
            RtlUnicodeToMultiByteSize(&cbMultiByte, &wChar, sizeof(WCHAR));
            pSrcAttr += (cbMultiByte == 2) ? 2 : 1;
        }
        dwCurrentOffset += ALIGN_DWORD(pUndeter->uUndetTextLen);
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (pCS->dwCursorPos == (DWORD)-1)
        {
            pUndeter->uCursorPos = (UINT)-1;
        }
        else
        {
            pUndeter->uCursorPos = IchWideFromAnsi(
                pCS->dwCursorPos, (PCSTR)(pSrcBase + pCS->dwCompStrOffset), CP_ACP);
        }
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (pCS->dwDeltaStart == (DWORD)-1)
        {
            pUndeter->uDeltaStart = (UINT)-1;
        }
        else
        {
            pUndeter->uDeltaStart = IchWideFromAnsi(
                pCS->dwDeltaStart, (PCSTR)(pSrcBase + pCS->dwCompStrOffset), CP_ACP);
        }
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        INT nWideLen = MultiByteToWideChar(CP_ACP, 0,
            (PCSTR)(pSrcBase + pCS->dwResultStrOffset), pCS->dwResultStrLen,
            (PWSTR)(pBase + dwCurrentOffset), (dwSize - dwCurrentOffset) / sizeof(WCHAR));
        if (nWideLen > 0)
        {
            pUndeter->uDetermineTextPos = dwCurrentOffset;
            pUndeter->uDetermineTextLen = nWideLen;
            ((PWSTR)(pBase + dwCurrentOffset))[nWideLen] = UNICODE_NULL;
            dwCurrentOffset += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));
        }
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && (pCS->dwResultClauseLen > 0) &&
        (pUndeter->uDetermineTextLen > 0))
    {
        pUndeter->uDetermineDelimPos = dwCurrentOffset;
        PDWORD pSrcClause = (PDWORD)(pSrcBase + pCS->dwResultClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);

        for (DWORD i = 0; i < nClauses; i++)
        {
            pDestClause[i] = IchWideFromAnsi(pSrcClause[i],
                (PCSTR)(pSrcBase + pCS->dwResultStrOffset), CP_ACP);
        }
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        INT nWideLen = MultiByteToWideChar(CP_ACP, 0,
            (PCSTR)(pSrcBase + pCS->dwResultReadStrOffset), pCS->dwResultReadStrLen,
            (PWSTR)(pBase + dwCurrentOffset), (dwSize - dwCurrentOffset) / sizeof(WCHAR));
        if (nWideLen > 0)
        {
            pUndeter->uYomiTextPos = dwCurrentOffset;
            pUndeter->uYomiTextLen = nWideLen;
            ((PWSTR)(pBase + dwCurrentOffset))[nWideLen] = UNICODE_NULL;
            dwCurrentOffset += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));
        }
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && (pCS->dwResultReadClauseLen > 0) &&
        (pUndeter->uYomiTextLen > 0))
    {
        pUndeter->uYomiDelimPos = dwCurrentOffset;
        PDWORD pSrcClause = (PDWORD)(pSrcBase + pCS->dwResultReadClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);

        for (DWORD i = 0; i < nClauses; i++)
        {
            pDestClause[i] = IchWideFromAnsi(pSrcClause[i],
                (PCSTR)(pSrcBase + pCS->dwResultReadStrOffset), CP_ACP);
        }
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToStringExA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    LPSTRINGEXSTRUCT pSX,
    DWORD dwSize)
{
    DWORD dwRequiredSize =
        sizeof(STRINGEXSTRUCT) +
        ALIGN_DWORD(pCS->dwResultStrLen + 1) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(pCS->dwResultReadStrLen + 1) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pSX)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pSX, sizeof(STRINGEXSTRUCT));
    pSX->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(STRINGEXSTRUCT);
    PBYTE pBase = (PBYTE)pSX;
    const BYTE* pSrcBase = (const BYTE*)pCS;

    if (pCS->dwResultStrLen > 0)
    {
        pSX->uDeterminePos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultStrOffset,
                      pCS->dwResultStrLen);
        pBase[dwCurrentOffset + pCS->dwResultStrLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen > 0)
    {
        pSX->uDetermineDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultClauseOffset,
                      pCS->dwResultClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (pCS->dwResultReadStrLen > 0)
    {
        pSX->uYomiPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadStrOffset,
                      pCS->dwResultReadStrLen);
        pBase[dwCurrentOffset + pCS->dwResultReadStrLen] = ANSI_NULL;
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen > 0)
    {
        pSX->uYomiDelimPos = dwCurrentOffset;
        RtlCopyMemory(pBase + dwCurrentOffset, pSrcBase + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultReadClauseLen);
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToStringExW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    LPSTRINGEXSTRUCT pSX,
    DWORD dwSize)
{
    DWORD dwResultStrSize =
        pCS->dwResultStrLen ? ALIGN_DWORD(2 * pCS->dwResultStrLen + 5) : 0;
    DWORD dwResultClauseSize =
        pCS->dwResultClauseLen ? ALIGN_DWORD(pCS->dwResultClauseLen + 4) : 0;
    DWORD dwResultReadStrSize =
        pCS->dwResultReadStrLen ? ALIGN_DWORD(2 * pCS->dwResultReadStrLen + 5) : 0;
    DWORD dwReadClauseSize
        = pCS->dwResultReadClauseLen ? ALIGN_DWORD(pCS->dwResultReadClauseLen + 4) : 0;
    DWORD dwRequiredSize = sizeof(STRINGEXSTRUCT) + dwResultStrSize + dwResultClauseSize +
                           dwResultReadStrSize + dwReadClauseSize;

    if (pSX == NULL)
        return dwRequiredSize;

    if (dwSize < dwRequiredSize)
        return 0;

    RtlZeroMemory(pSX, sizeof(STRINGEXSTRUCT));
    pSX->dwSize = dwSize;

    DWORD dwCurrentOffset = sizeof(STRINGEXSTRUCT);
    PBYTE pBase = (PBYTE)pSX;
    const BYTE* pSrcBase = (const BYTE*)pCS;

    INT nWideResultLen = 0, nWideReadLen = 0;

    if (pCS->dwResultStrLen > 0) {
        pSX->uDeterminePos = dwCurrentOffset;
        nWideResultLen = MultiByteToWideChar(
            CP_ACP, 0,
            (PCSTR)(pSrcBase + pCS->dwResultStrOffset), pCS->dwResultStrLen,
            (PWSTR)(pBase + dwCurrentOffset), (dwSize - dwCurrentOffset) / sizeof(WCHAR));
        if (nWideResultLen <= 0)
            return 0;
        ((PWSTR)(pBase + dwCurrentOffset))[nWideResultLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD(nWideResultLen * sizeof(WCHAR) + sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen > 0 && nWideResultLen > 0)
    {
        pSX->uDetermineDelimPos = dwCurrentOffset;
        const DWORD* pSrcClause = (const DWORD*)(pSrcBase + pCS->dwResultClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);

        for (DWORD i = 0; i < nClauses; i++)
        {
            pDestClause[i] = IchWideFromAnsi(pSrcClause[i],
                (PCSTR)(pSrcBase + pCS->dwResultStrOffset), CP_ACP);
        }
        dwCurrentOffset += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (pCS->dwResultReadStrLen > 0)
    {
        pSX->uYomiPos = dwCurrentOffset;
        nWideReadLen = MultiByteToWideChar(CP_ACP, 0,
            (PCSTR)(pSrcBase + pCS->dwResultReadStrOffset), pCS->dwResultReadStrLen,
            (PWSTR)(pBase + dwCurrentOffset), (dwSize - dwCurrentOffset) / sizeof(WCHAR));
        if (nWideReadLen <= 0)
            return 0;

        ((PWSTR)(pBase + dwCurrentOffset))[nWideReadLen] = UNICODE_NULL;
        dwCurrentOffset += ALIGN_DWORD(nWideReadLen * sizeof(WCHAR) + sizeof(WCHAR));
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen > 0 && nWideReadLen > 0)
    {
        pSX->uYomiDelimPos = dwCurrentOffset;
        const DWORD* pSrcClause = (const DWORD*)(pSrcBase + pCS->dwResultReadClauseOffset);
        PDWORD pDestClause = (PDWORD)(pBase + dwCurrentOffset);
        DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
        for (DWORD i = 0; i < nClauses; i++)
        {
            pDestClause[i] = IchWideFromAnsi(pSrcClause[i],
                (PCSTR)(pSrcBase + pCS->dwResultReadStrOffset), CP_ACP);
        }
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToStringA(const COMPOSITIONSTRING *pCS, PSTR pszString, DWORD dwSize)
{
    DWORD dwResultStrLen = pCS->dwResultStrLen;
    if (!pszString)
        return dwResultStrLen + 1;
    if (dwResultStrLen > dwSize)
        return 0;
    RtlCopyMemory(pszString, (PBYTE)pCS + pCS->dwResultStrOffset, dwResultStrLen);
    pszString[dwResultStrLen] = ANSI_NULL;
    return dwResultStrLen + 1;
}

static DWORD
Imm32CompStrAToStringW(const COMPOSITIONSTRING *pCS, PWSTR pszString, DWORD dwSize)
{
    const CHAR *pch = (const CHAR *)pCS + pCS->dwResultStrOffset;
    INT cchWide = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pch, pCS->dwResultStrLen,
                                      pszString, 0);
    DWORD dwResultStrLen = (cchWide + 1) * sizeof(WCHAR);
    if (!pszString)
        return dwResultStrLen;
    if (dwResultStrLen > dwSize)
        return 0;
    INT cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pch, pCS->dwResultStrLen,
                                  pszString, cchWide);
    if (cch < cchWide)
        return 0;
    pszString[cch] = UNICODE_NULL;
    return (cch + 1) * sizeof(WCHAR);
}

const WCHAR* Imm32CompStrWToCharA(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    const WCHAR *pCurrent = (const WCHAR *)((const BYTE *)pCS + pCS->dwResultStrOffset);

    BOOL bSupportDBCSReport = FALSE;
    if (GetWin32ClientInfo()->dwExpWinVer >= 0x30A)
        bSupportDBCSReport = (BOOL)SendMessageA(hWnd, WM_IME_REPORT, IR_DBCSCHAR, 0);

    PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    const WCHAR *pNext;
    for (; *pCurrent != UNICODE_NULL; pCurrent = pNext)
    {
        pNext = CharNextW(pCurrent);
        if (!*pNext)
            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        char szAnsi[3] = {0};
        BOOL bUsedDef = FALSE;
        if (!WideCharToMultiByte(CP_ACP, 0, pCurrent, 1, szAnsi, sizeof(szAnsi), NULL, &bUsedDef))
            continue;

        if (!IsDBCSLeadByte((BYTE)szAnsi[0]))
        {
            PostMessageA(hWnd, WM_CHAR, (BYTE)szAnsi[0], 1);
            continue;
        }

        if (bSupportDBCSReport)
        {
            WPARAM wParam = 0x80000000 | MAKEWORD(szAnsi[1], szAnsi[0]);
            PostMessageA(hWnd, WM_CHAR, wParam, 1);
        }
        else
        {
            PostMessageA(hWnd, WM_CHAR, (BYTE)szAnsi[0], 1);
            PostMessageA(hWnd, WM_CHAR, (BYTE)szAnsi[1], 1);
        }
    }

    return pCurrent;
}

static CHAR Imm32CompStrAToCharA(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    const CHAR *pch = (const CHAR *)((PBYTE)pCS + pCS->dwResultStrOffset);

    BOOL bImeSupportDBCS = FALSE;
    if (GetWin32ClientInfo()->dwExpWinVer >= 0x30A)
        bImeSupportDBCS = (BOOL)SendMessageA(hWnd, WM_IME_REPORT, IR_DBCSCHAR, 0);

    PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    CHAR ch = *pch;
    if (*pch == ANSI_NULL)
        return ch;

    while (*pch)
    {
        BYTE bFirst = (BYTE)(*pch);
        const CHAR *pNext = CharNextA(pch);

        if (*pNext == ANSI_NULL)
            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        if (IsDBCSLeadByte(bFirst))
        {
            BYTE bSecond = pch[1];
            if (bImeSupportDBCS)
            {
                WPARAM wParamDBCS = 0x80000000 | MAKEWORD(bSecond, bFirst);
                PostMessageA(hWnd, WM_CHAR, wParamDBCS, 1);
            }
            else
            {
                PostMessageA(hWnd, WM_CHAR, bFirst, 1);
                PostMessageA(hWnd, WM_CHAR, bSecond, 1);
            }
        }
        else
        {
            PostMessageA(hWnd, WM_CHAR, bFirst, 1);
        }

        pch = pNext;
        ch = *pch;
    }

    return ch;
}

static CHAR Imm32CompStrAToCharW(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    const CHAR *pCurrent = (const CHAR *)((PBYTE)pCS + pCS->dwResultStrOffset);
    CHAR ch = *pCurrent;

    PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    while (*pCurrent)
    {
        ch = *pCurrent;
        BYTE bTestChar = *pCurrent;
        PCHAR pNext = CharNextA(pCurrent);

        if (*pNext == ANSI_NULL)
            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        WCHAR wCharBuffer = UNICODE_NULL;
        INT nConverted = 0;
        if (IsDBCSLeadByte(bTestChar))
            nConverted = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pCurrent, 2, &wCharBuffer, 1);
        else
            nConverted = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pCurrent, 1, &wCharBuffer, 1);

        if (nConverted > 0)
            PostMessageW(hWnd, WM_CHAR, wCharBuffer, 1);

        pCurrent = pNext;
    }

    return ch;
}

static DWORD
Imm32JTransCompositionA(
    PINPUTCONTEXTDX pIC,
    PCOMPOSITIONSTRING pCS,
    PTRANSMSG pCurrent,
    PTRANSMSG pEntries)
{
    HWND hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        return 0;

    BOOL bIsUnicodeWnd = IsWindowUnicode(hWnd);
    DWORD dwGCS = (DWORD)pCurrent->lParam;
    int msgCount = 0;
    BOOL bUndeterminedProcessed = FALSE;
    BOOL bResultProcessed = FALSE;
    HGLOBAL hGlobal = NULL;
    SIZE_T dataSize = 0;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        if (!bIsUnicodeWnd)
        {
            dataSize = Imm32CompStrAToUndetA(dwGCS, pCS, NULL, 0);
            if (dataSize)
            {
                hGlobal = GlobalAlloc(GHND, dataSize);
                if (hGlobal) {
                    PUNDETERMINESTRUCT pUndet = (PUNDETERMINESTRUCT)GlobalLock(hGlobal);
                    if (Imm32CompStrAToUndetA(dwGCS, pCS, pUndet, (DWORD)dataSize))
                    {
                        GlobalUnlock(hGlobal);
                        if (SendMessageA(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal) == 0)
                            bUndeterminedProcessed = TRUE;
                    }
                }
            }
        }
        else
        {
            dataSize = Imm32CompStrAToUndetW(dwGCS, pCS, NULL, 0);
            if (dataSize)
            {
                hGlobal = GlobalAlloc(GHND, dataSize);
                if (hGlobal)
                {
                    PUNDETERMINESTRUCT pUndet = (PUNDETERMINESTRUCT)GlobalLock(hGlobal);
                    if (Imm32CompStrAToUndetW(dwGCS, pCS, pUndet, (DWORD)dataSize))
                    {
                        GlobalUnlock(hGlobal);
                        if (SendMessageW(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal) == 0)
                            bUndeterminedProcessed = TRUE;
                    }
                }
            }
        }

        if (hGlobal && !bUndeterminedProcessed)
        {
            GlobalUnlock(hGlobal);
            GlobalFree(hGlobal);
        }
        else if (hGlobal && bUndeterminedProcessed)
        {
            GlobalFree(hGlobal);
            return 0;
        }
    }

    SIZE_T exSize;
    HGLOBAL hEx, hStr;
    PVOID pEx, pStr;
    BOOL bExSuccess, bStrSuccess;
    LRESULT res;
    SIZE_T strSize;

    if (dwGCS & GCS_RESULTSTR)
    {
        if (dwGCS & GCS_RESULTREADSTR)
        {
            exSize = bIsUnicodeWnd ? Imm32CompStrAToStringExW(dwGCS, pCS, NULL, 0)
                                   : Imm32CompStrAToStringExA(dwGCS, pCS, NULL, 0);
            if (exSize)
            {
                hEx = GlobalAlloc(GHND, exSize);
                if (hEx)
                {
                    pEx = GlobalLock(hEx);
                    bExSuccess = bIsUnicodeWnd
                        ? Imm32CompStrAToStringExW(dwGCS, pCS, pEx, (DWORD)exSize)
                        : Imm32CompStrAToStringExA(dwGCS, pCS, pEx, (DWORD)exSize);
                    GlobalUnlock(hEx);
                    if (bExSuccess)
                    {
                        res = bIsUnicodeWnd
                            ? SendMessageW(hWnd, WM_IME_REPORT, IR_STRINGEX, (LPARAM)hEx)
                            : SendMessageA(hWnd, WM_IME_REPORT, IR_STRINGEX, (LPARAM)hEx);
                        if (res)
                        {
                            GlobalFree(hEx);
                            bResultProcessed = TRUE;
                            goto FINALIZE;
                        }
                    }
                    GlobalFree(hEx);
                }
            }
        }

        strSize = bIsUnicodeWnd ? Imm32CompStrAToStringW(pCS, NULL, 0) : (pCS->dwResultStrLen + 1);
        if (strSize && strSize != (SIZE_T)-1)
        {
            hStr = GlobalAlloc(GHND, strSize);
            if (hStr)
            {
                pStr = GlobalLock(hStr);
                bStrSuccess = bIsUnicodeWnd
                    ? Imm32CompStrAToStringW(pCS, pStr, (DWORD)strSize)
                    : Imm32CompStrAToStringA(pCS, pStr, (DWORD)strSize);
                GlobalUnlock(hStr);
                if (bStrSuccess)
                {
                    res = bIsUnicodeWnd
                        ? SendMessageW(hWnd, WM_IME_REPORT, IR_STRING, (LPARAM)hStr)
                        : SendMessageA(hWnd, WM_IME_REPORT, IR_STRING, (LPARAM)hStr);
                    if (res)
                    {
                        GlobalFree(hStr);
                        bResultProcessed = TRUE;
                        goto FINALIZE;
                    }
                }
                GlobalFree(hStr);
            }
        }

        if (bIsUnicodeWnd)
            Imm32CompStrAToCharW(hWnd, pCS);
        else
            Imm32CompStrAToCharA(hWnd, pCS);

        bResultProcessed = TRUE;
    }

FINALIZE:
    if (!bUndeterminedProcessed)
    {
        if (bResultProcessed)
        {
            if (dwGCS & GCS_COMPSTR)
            {
                pEntries->message = pCurrent->message;
                pEntries->wParam = pCurrent->wParam;
                pEntries->lParam =
                    dwGCS & ~(GCS_RESULTCLAUSE | GCS_RESULTSTR | GCS_RESULTREADCLAUSE |
                              GCS_RESULTREADSTR);
                msgCount = 1;
            }
        }
        else
        {
            *pEntries = *pCurrent;
            msgCount = 1;
        }
    }

    return msgCount;
}

DWORD
Imm32JTransCompositionW(
    PINPUTCONTEXTDX pIC,
    PCOMPOSITIONSTRING pCS,
    PTRANSMSG pCurrent,
    PTRANSMSG pEntries)
{
    HWND hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        return 0;

    BOOL bIsUnicodeWnd = IsWindowUnicode(hWnd);
    DWORD dwGCS = (DWORD)pCurrent->lParam;
    int msgCount = 0;
    BOOL bUndeterminedProcessed = FALSE;
    BOOL bResultProcessed = FALSE;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        HGLOBAL hGlobal = NULL;
        SIZE_T dataSize = 0;

        if (!bIsUnicodeWnd)
        {
            dataSize = Imm32CompStrWToUndetA(dwGCS, pCS, NULL);
            if (dataSize)
            {
                hGlobal = GlobalAlloc(GHND, dataSize);
                if (hGlobal)
                {
                    PUNDETERMINESTRUCT pUndet = (PUNDETERMINESTRUCT)GlobalLock(hGlobal);
                    if (Imm32CompStrWToUndetA(dwGCS, pCS, pUndet))
                    {
                        GlobalUnlock(hGlobal);
                        if (SendMessageA(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal) == 0)
                            bUndeterminedProcessed = TRUE;
                    }
                }
            }
        }
        else
        {
            dataSize = Imm32CompStrWToUndetW(dwGCS, pCS, NULL);
            if (dataSize)
            {
                hGlobal = GlobalAlloc(GHND, dataSize);
                if (hGlobal)
                {
                    PUNDETERMINESTRUCT pUndet = (PUNDETERMINESTRUCT)GlobalLock(hGlobal);
                    if (Imm32CompStrWToUndetW(dwGCS, pCS, pUndet))
                    {
                        GlobalUnlock(hGlobal);
                        if (SendMessageW(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hGlobal) == 0)
                            bUndeterminedProcessed = TRUE;
                    }
                }
            }
        }

        if (hGlobal && !bUndeterminedProcessed)
        {
            GlobalUnlock(hGlobal);
            GlobalFree(hGlobal);
        }
        else if (hGlobal && bUndeterminedProcessed)
        {
            GlobalFree(hGlobal);
            return 0;
        }
    }

    SIZE_T exSize;
    HGLOBAL hEx, hStr;
    PVOID pEx, pStr;
    BOOL bExSuccess, bStrSuccess;
    LRESULT res;
    SIZE_T strSize;

    if (dwGCS & GCS_RESULTSTR)
    {
        if (dwGCS & GCS_RESULTREADSTR)
        {
            exSize = bIsUnicodeWnd
                ? Imm32CompStrWToStringExW(dwGCS, pCS, NULL, 0)
                : Imm32CompStrWToStringExA(dwGCS, pCS, NULL, 0);
            if (exSize)
            {
                hEx = GlobalAlloc(GHND, exSize);
                if (hEx)
                {
                    pEx = GlobalLock(hEx);
                    bExSuccess = bIsUnicodeWnd
                        ? Imm32CompStrWToStringExW(dwGCS, pCS, pEx, (DWORD)exSize)
                        : Imm32CompStrWToStringExA(dwGCS, pCS, pEx, (DWORD)exSize);
                    GlobalUnlock(hEx);
                    if (bExSuccess)
                    {
                        res = bIsUnicodeWnd
                            ? SendMessageW(hWnd, WM_IME_REPORT, IR_STRINGEX, (LPARAM)hEx)
                            : SendMessageA(hWnd, WM_IME_REPORT, IR_STRINGEX, (LPARAM)hEx);
                        if (res)
                        {
                            GlobalFree(hEx);
                            bResultProcessed = TRUE;
                            goto FINALIZE;
                        }
                    }
                    GlobalFree(hEx);
                }
            }
        }

        strSize = bIsUnicodeWnd
            ? Imm32CompStrWToStringW(pCS, NULL)
            : Imm32CompStrWToStringA(pCS, NULL);
        if (strSize && strSize != (SIZE_T)-1)
        {
            hStr = GlobalAlloc(GHND, strSize);
            if (hStr)
            {
                pStr = GlobalLock(hStr);
                bStrSuccess = bIsUnicodeWnd ? Imm32CompStrWToStringW(pCS, pStr)
                                            : Imm32CompStrWToStringA(pCS, pStr);
                GlobalUnlock(hStr);
                if (bStrSuccess)
                {
                    res = bIsUnicodeWnd
                        ? SendMessageW(hWnd, WM_IME_REPORT, IR_STRING, (LPARAM)hStr)
                        : SendMessageA(hWnd, WM_IME_REPORT, IR_STRING, (LPARAM)hStr);
                    if (res)
                    {
                        GlobalFree(hStr);
                        bResultProcessed = TRUE;
                        goto FINALIZE;
                    }
                }
                GlobalFree(hStr);
            }
        }

        if (bIsUnicodeWnd)
            Imm32CompStrWToCharW(hWnd, pCS);
        else
            Imm32CompStrWToCharA(hWnd, pCS);

        bResultProcessed = TRUE;
    }

FINALIZE:
    if (!bUndeterminedProcessed)
    {
        if (bResultProcessed)
        {
            if (dwGCS & GCS_COMPSTR)
            {
                pEntries->message = pCurrent->message;
                pEntries->wParam = pCurrent->wParam;
                pEntries->lParam =
                    dwGCS & ~(GCS_RESULTCLAUSE | GCS_RESULTSTR | GCS_RESULTREADCLAUSE |
                              GCS_RESULTREADSTR);
                msgCount = 1;
            }
        }
        else
        {
            *pEntries = *pCurrent;
            msgCount = 1;
        }
    }

    return msgCount;
}

/* Japanese */
static DWORD
WINNLSTranslateMessageJ(DWORD dwCount, PTRANSMSG pEntries, PINPUTCONTEXTDX pIC,
                        PCOMPOSITIONSTRING pCS, BOOL bAnsi)
{
    // Clone the message list with growing
    SIZE_T dwBufSize = (dwCount + 1) * sizeof(TRANSMSG);
    PTRANSMSG pBuf = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwBufSize);
    if (!pBuf)
        return 0;
    RtlCopyMemory(pBuf, pEntries, dwCount * sizeof(TRANSMSG));

    HWND hWnd = pIC->hWnd;
    HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);
    PTRANSMSG pCurrent = pBuf;
    DWORD dwResult = 0;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        // Find WM_IME_ENDCOMPOSITION
        PTRANSMSG pEnd = NULL;
        for (DWORD i = 0; i < dwCount; i++)
        {
            if (pBuf[i].message == WM_IME_ENDCOMPOSITION)
            {
                pEnd = &pBuf[i];
                break;
            }
        }

        if (pEnd)
        {
            // Move WM_IME_ENDCOMPOSITION to the end of the list
            PTRANSMSG pSrc = pEnd + 1, pDst = pEnd;
            while (pSrc->message)
                *pDst++ = *pSrc++;

            pDst->message = WM_IME_ENDCOMPOSITION;
            pDst->wParam = pDst->lParam = 0;
        }

        pCurrent = pBuf;
    }

#define EMIT_ENTRY_J() do { \
    *pEntries++ = *pCurrent; \
    dwResult++; \
} while (0)

    while (pCurrent->message)
    {
        switch (pCurrent->message)
        {
            case WM_IME_STARTCOMPOSITION:
                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN))
                {
                    // Send IR_OPENCONVERT
                    if (pIC->cfCompForm.dwStyle)
                        SendMessageW(hWnd, WM_IME_REPORT, IR_OPENCONVERT, 0);
                    EMIT_ENTRY_J();
                }
                break;

            case WM_IME_ENDCOMPOSITION:
                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN))
                {
                    // Send IR_CLOSECONVERT
                    if (pIC->cfCompForm.dwStyle)
                        SendMessageW(hWnd, WM_IME_REPORT, IR_CLOSECONVERT, 0);
                    EMIT_ENTRY_J();
                }
                else
                {
                    // Send IR_UNDETERMINE
                    HGLOBAL hMem = GlobalAlloc(GHND | GMEM_SHARE, sizeof(UNDETERMINESTRUCT));
                    if (hMem)
                    {
                        if (IsWindowUnicode(hWnd))
                            SendMessageW(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hMem);
                        else
                            SendMessageA(hWnd, WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hMem);
                        GlobalFree(hMem);
                    }
                }
                break;

            case WM_IME_COMPOSITION:
            {
                DWORD dwWritten;
                if (bAnsi)
                    dwWritten = Imm32JTransCompositionA(pIC, pCS, pCurrent, pEntries);
                else
                    dwWritten = Imm32JTransCompositionW(pIC, pCS, pCurrent, pEntries);

                dwResult += dwWritten;
                pEntries += dwWritten;

                // Send IR_CHANGECONVERT
                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN) && pIC->cfCompForm.dwStyle)
                    SendMessageW(hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
                break;
            }

            case WM_IME_NOTIFY:
                if (pCurrent->wParam == IMN_OPENCANDIDATE)
                {
                    if (IsWindow(hWnd) && (pIC->dwUIFlags & _IME_UI_HIDDEN))
                    {
                        // Send IMC_SETCANDIDATEPOS
                        CANDIDATEFORM CandForm;
                        LPARAM lParam = pCurrent->lParam;
                        for (DWORD iCandForm = 0; iCandForm < MAX_CANDIDATEFORM; ++iCandForm)
                        {
                            if (!(lParam & (1 << iCandForm)))
                                continue;

                            CandForm.dwIndex = iCandForm;
                            CandForm.dwStyle = CFS_EXCLUDE;
                            CandForm.ptCurrentPos = pIC->cfCompForm.ptCurrentPos;
                            CandForm.rcArea = pIC->cfCompForm.rcArea;
                            SendMessageW(hwndIme, WM_IME_CONTROL, IMC_SETCANDIDATEPOS,
                                         (LPARAM)&CandForm);
                        }
                    }
                }

                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN))
                    EMIT_ENTRY_J();
                else
                    SendMessageW(hwndIme, pCurrent->message, pCurrent->wParam, pCurrent->lParam);
                break;

            default:
                EMIT_ENTRY_J();
                break;
        }

        pCurrent++;
    }

#undef EMIT_ENTRY_J
    ImmLocalFree(pBuf);
    return dwResult;
}

typedef LRESULT (WINAPI *FN_SendMessage)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (WINAPI *FN_PostMessage)(HWND, UINT, WPARAM, LPARAM);

/* Korean */
static DWORD
WINNLSTranslateMessageK(
    DWORD dwCount,
    PTRANSMSG pEntries,
    PINPUTCONTEXTDX pIC,
    PCOMPOSITIONSTRING pCS,
    BOOL bSrcIsAnsi)
{
    HWND hWnd = pIC->hWnd;
    HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);
    BOOL bDestIsUnicode = IsWindowUnicode(hWnd);
    BOOL bDestIsAnsi = !bDestIsUnicode;
    FN_PostMessage pfnPostMessage = bDestIsUnicode ? PostMessageW : PostMessageA;
    FN_SendMessage pfnSendMessage = bDestIsUnicode ? SendMessageW : SendMessageA;

    if ((LONG)dwCount <= 0)
        return 0;

    for (DWORD i = 0; i < dwCount; ++i)
    {
        UINT   uMsg   = pEntries[i].message;
        WPARAM wParam = pEntries[i].wParam;
        LPARAM lParam = pEntries[i].lParam;

        if (uMsg < WM_IME_STARTCOMPOSITION || uMsg > WM_IME_ENDCOMPOSITION)
        {
            switch (uMsg)
            {
            case WM_IME_COMPOSITION:
                if (wParam & 0x8000000)
                {
                    pfnPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

                    if ((LONG)pCS->dwResultStrLen > 0)
                    {
                        DWORD dwProcessedLen = 0;
                        while (dwProcessedLen < pCS->dwResultStrLen)
                        {
                            LPARAM charLParam = 1;
                            WCHAR wCharStr[2] = {0};
                            CHAR  szMBStr[4]  = {0};

                            if (bSrcIsAnsi)
                            {
                                PSTR pResStr = (LPSTR)((BYTE*)pCS + pCS->dwResultStrOffset);
                                BYTE bChar = (BYTE)pResStr[dwProcessedLen];

                                if (bDestIsAnsi)
                                {
                                    if (IsDBCSLeadByte(bChar))
                                    {
                                        if (bChar < 0xB0 || bChar > 0xC8)
                                            charLParam = 0xFFF10001; 
                                        else
                                            charLParam = 0xFFF20001;

                                        PostMessageA(hWnd, WM_CHAR, bChar, charLParam);
                                        dwProcessedLen++;
                                        bChar = (BYTE)pResStr[dwProcessedLen];
                                    }
                                    PostMessageA(hWnd, WM_CHAR, bChar, charLParam);
                                }
                                else
                                {
                                    szMBStr[0] = bChar;
                                    szMBStr[1] = pResStr[++dwProcessedLen];
                                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szMBStr, 2,
                                                        wCharStr, 1);
                                    PostMessageW(hWnd, WM_CHAR, wCharStr[0], 1);
                                }
                            }
                            else
                            {
                                PWSTR pResStrW = (PWSTR)((PBYTE)pCS + pCS->dwResultStrOffset);
                                wCharStr[0] = pResStrW[dwProcessedLen];

                                if (!bDestIsAnsi)
                                {
                                    PostMessageW(hWnd, WM_CHAR, wCharStr[0], 1);
                                }
                                else
                                {
                                    WideCharToMultiByte(CP_ACP, 0, wCharStr, 1, szMBStr, 2,
                                                        NULL, NULL);
                                    BYTE bLead = (BYTE)szMBStr[0], bChar;
                                    if (IsDBCSLeadByte(bLead))
                                    {
                                        charLParam = (bLead < 0xB0 || bLead > 0xC8)
                                            ? 0xFFF10001
                                            : 0xFFF20001;
                                        PostMessageA(hWnd, WM_CHAR, bLead, charLParam);
                                        bChar = (BYTE)szMBStr[1];
                                    }
                                    else
                                    {
                                        bChar = (BYTE)szMBStr[0];
                                    }
                                    PostMessageA(hWnd, WM_CHAR, bChar, charLParam);
                                }
                            }
                            dwProcessedLen++;
                        }
                    }
                    pfnPostMessage(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                }
                

                if (wParam != 0)
                {
                    pfnPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

                    BYTE bLow = HIBYTE(wParam), bHigh = LOBYTE(wParam);
                    g_dchKorean = MAKEWORD(bLow, bHigh);

                    if (bSrcIsAnsi)
                    {
                        if (bDestIsAnsi)
                        {
                            PostMessageA(hWnd, WM_INTERIM, bLow, 0xF00001);
                            PostMessageA(hWnd, WM_INTERIM, bHigh, 0xF00001);
                        }
                        else
                        {
                            CHAR szTmp[2] = { (CHAR)bLow, (CHAR)bHigh };
                            WCHAR wTmp[2];
                            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTmp, 2, wTmp, 1))
                                PostMessageW(hWnd, WM_INTERIM, wTmp[0], 0xF00001);
                        }
                    }
                    else
                    {
                        if (!bDestIsAnsi)
                        {
                            PostMessageW(hWnd, WM_INTERIM, wParam, 0xF00001);
                        }
                        else
                        {
                            WCHAR wTmp[2] = { (WCHAR)wParam, 0 };
                            CHAR  szTmp[2];
                            WideCharToMultiByte(CP_ACP, 0, wTmp, 1, szTmp, 2, NULL, NULL);
                            PostMessageA(hWnd, WM_INTERIM, (BYTE)szTmp[0], 0xF00001);
                            PostMessageA(hWnd, WM_INTERIM, (BYTE)szTmp[1], 0xF00001);
                        }
                    }

                    if (bDestIsUnicode)
                        PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                    else
                        PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

                    pfnSendMessage(hwndIme, WM_IME_ENDCOMPOSITION, 0, 0);
                }
                else
                {
                    pfnPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);
                    if (bSrcIsAnsi)
                    {
                        if (bDestIsAnsi)
                        {
                            PostMessageA(hWnd, WM_CHAR, (BYTE)g_dchKorean, 0xFFF10001);
                            PostMessageA(hWnd, WM_CHAR, HIBYTE(g_dchKorean), 0xFFF10001);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, 917505);
                        }
                        else
                        {
                            CHAR szTmp[2];
                            szTmp[0] = (CHAR)LOBYTE(g_dchKorean);
                            szTmp[1] = (CHAR)HIBYTE(g_dchKorean);
                            WCHAR wTmp[2];
                            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTmp, 2, wTmp, 1))
                            {
                                PostMessageW(hWnd, WM_CHAR, wTmp[0], 0xFFF10001);
                            }
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, 917505);
                        }
                    }
                    else
                    {
                        if (bDestIsAnsi)
                        {
                            WCHAR wTmp[2] = { (WCHAR)g_dchKorean, 0 };
                            CHAR  szTmp[2];
                            WideCharToMultiByte(CP_ACP, 0, wTmp, 1, szTmp, 2, NULL, NULL);
                            PostMessageA(hWnd, WM_CHAR, (BYTE)szTmp[0], 0xFFF10001);
                            PostMessageA(hWnd, WM_CHAR, (BYTE)szTmp[1], 0xFFF10001);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, 917505);
                        }
                        else
                        {
                            PostMessageW(hWnd, WM_CHAR, (WCHAR)g_dchKorean, 0xFFF10001);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, 917505);
                        }
                    }
                }
                break;

            case WM_IMEKEYDOWN:
                pfnPostMessage(hWnd, WM_KEYDOWN, (WPARAM)(LOWORD(wParam)), lParam);
                break;

            case WM_IMEKEYUP:
                pfnPostMessage(hWnd, WM_KEYUP, (WPARAM)(LOWORD(wParam)), lParam);
                break;

            default:
                pfnSendMessage(hwndIme, uMsg, wParam, lParam);
                break;
            }
        }
        else
        {
            pfnSendMessage(hwndIme, uMsg, wParam, lParam);
        }
    }

    return 0;
}

#endif /* def IMM_WIN3_SUPPORT */

/* This function is used in ImmGenerateMessage and ImmTranslateMessage */
DWORD
WINNLSTranslateMessage(DWORD dwCount, PTRANSMSG pEntries, HIMC hIMC, BOOL bAnsi, WORD wLang)
{
#ifdef IMM_WIN3_SUPPORT
    PINPUTCONTEXTDX pIC = (PINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return 0;

    PCOMPOSITIONSTRING pCompStr = ImmLockIMCC(pIC->hCompStr);
    if (!pCompStr)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    DWORD ret;
    if (wLang == LANG_KOREAN)
        ret = WINNLSTranslateMessageK(dwCount, pEntries, pIC, pCompStr, bAnsi);
    else if (wLang == LANG_JAPANESE)
        ret = WINNLSTranslateMessageJ(dwCount, pEntries, pIC, pCompStr, bAnsi);
    else
        ret = 0;

    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
#else
    return 0;
#endif
}
