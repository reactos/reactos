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

#define ALIGN_DWORD(len)      (((len) + 3) & ~3)
#define ALIGN_ASTR_SIZE(len)  ALIGN_DWORD(2 * (len) + sizeof(ANSI_NULL))
#define ALIGN_WSTR_SIZE(len)  ALIGN_DWORD((len) * sizeof(WCHAR) + sizeof(UNICODE_NULL))

/* Korean lead byte */
#define KOR_LEAD_BYTE_FIRST  ((BYTE)0xB0)
#define KOR_LEAD_BYTE_LAST   ((BYTE)0xC8)
#define KOR_IS_LEAD_BYTE(ch) \
    (KOR_LEAD_BYTE_FIRST <= (BYTE)(ch) && (BYTE)(ch) <= KOR_LEAD_BYTE_LAST)

#define KOR_SCAN_CODE_SBCS 0xFFF10001 /* Korean single-byte character string */
#define KOR_SCAN_CODE_DBCS 0xFFF20001 /* Korean double-byte character string */
#define KOR_KEYDOWN_FLAGS 0xE0001
#define KOR_INTERIM_FLAGS 0xF00001

static inline INT Imm32WideToAnsi(PCWCH pchWide, INT cchWide, PSTR pszAnsi, INT cchAnsi)
{
    BOOL usedDefaultChar = FALSE;
    INT len = WideCharToMultiByte(CP_ACP, 0, pchWide, cchWide, pszAnsi, cchAnsi,
                                  NULL, &usedDefaultChar);
    if (pszAnsi && len >= 0 && len < cchAnsi)
        pszAnsi[len] = ANSI_NULL;
    return len;
}

static inline INT Imm32AnsiToWide(PCCH pchAnsi, INT cchAnsi, PWSTR pszWide, INT cchWide)
{
    INT len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pchAnsi, cchAnsi, pszWide, cchWide);
    if (pszWide && len >= 0 && len < cchWide)
        pszWide[len] = UNICODE_NULL;
    return len;
}

static DWORD Imm32CompStrWToStringA(const COMPOSITIONSTRING *pCS, PCHAR pch)
{
    BOOL bUsedDef;
    PCWSTR pwch = (PCWSTR)((const BYTE*)pCS + pCS->dwResultStrOffset);
    INT cchNeed = WideCharToMultiByte(CP_ACP, 0, pwch, pCS->dwResultStrLen,
                                      pch, 0, NULL, &bUsedDef) + 1;
    if (!pch)
        return cchNeed;
    INT cch = WideCharToMultiByte(CP_ACP, 0, pwch, pCS->dwResultStrLen,
                                  pch, cchNeed, NULL, &bUsedDef);
    pch[cch] = ANSI_NULL;
    return cch + 1;
}

static DWORD Imm32CompStrWToStringW(const COMPOSITIONSTRING *pCS, PWCHAR pch)
{
    DWORD dwResultStrLen = pCS->dwResultStrLen;
    if (pch)
    {
        DWORD cb = dwResultStrLen * sizeof(WCHAR);
        RtlCopyMemory(pch, (const BYTE*)pCS + pCS->dwResultStrOffset, cb);
        pch[dwResultStrLen] = UNICODE_NULL;
    }
    return (dwResultStrLen + 1) * sizeof(WCHAR);
}

static DWORD
Imm32CompStrWToUndetW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDet,
    DWORD dwSize)
{
    const DWORD cbRequired =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_WSTR_SIZE(pCS->dwCompStrLen) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pDet)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pDet->dwSize = cbRequired;

    PBYTE pbDet = (PBYTE)pDet;
    DWORD ib = sizeof(UNDETERMINESTRUCT);
    const BYTE* pbCS = (const BYTE*)pCS;

    if (dwGCS & GCS_COMPSTR)
    {
        pDet->uUndetTextPos = ib;
        pDet->uUndetTextLen = pCS->dwCompStrLen;
        DWORD cb = pCS->dwCompStrLen * sizeof(WCHAR);
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwCompStrOffset, cb);
        ((PWCHAR)(pbDet + ib))[pCS->dwCompStrLen] = UNICODE_NULL;

        ib += ALIGN_WSTR_SIZE(pCS->dwCompStrLen);
    }

    if ((dwGCS & GCS_COMPATTR) || pCS->dwCompAttrLen)
    {
        pDet->uUndetAttrPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwCompAttrOffset, pCS->dwCompAttrLen);

        ib += ALIGN_DWORD(pCS->dwCompAttrLen);
    }

    if (dwGCS & GCS_CURSORPOS)
        pDet->uCursorPos = pCS->dwCursorPos;

    if (dwGCS & GCS_DELTASTART)
        pDet->uDeltaStart = pCS->dwDeltaStart;

    if (dwGCS & GCS_RESULTSTR)
    {
        pDet->uDetermineTextPos = ib;
        pDet->uDetermineTextLen = pCS->dwResultStrLen;
        DWORD cb = pCS->dwResultStrLen * sizeof(WCHAR);
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultStrOffset, cb);
        ((PWCHAR)(pbDet + ib))[pCS->dwResultStrLen] = UNICODE_NULL;

        ib += ALIGN_WSTR_SIZE(pCS->dwResultStrLen);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
    {
        pDet->uDetermineDelimPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultClauseOffset, pCS->dwResultClauseLen);

        ib += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        pDet->uYomiTextPos = ib;
        pDet->uYomiTextLen = pCS->dwResultReadStrLen;
        DWORD cb = pCS->dwResultReadStrLen * sizeof(WCHAR);
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultReadStrOffset, cb);
        ((PWCHAR)(pbDet + ib))[pCS->dwResultReadStrLen] = UNICODE_NULL;

        ib += ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
    {
        pDet->uYomiDelimPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);

        //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
    }

    return cbRequired;
}

static DWORD
Imm32CompStrWToUndetA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDet,
    DWORD dwSize)
{
    const DWORD cbRequired =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_ASTR_SIZE(pCS->dwCompStrLen) +
        ALIGN_ASTR_SIZE(pCS->dwCompAttrLen) +
        ALIGN_ASTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_ASTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pDet)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pDet->dwSize = cbRequired;

    DWORD ib = sizeof(UNDETERMINESTRUCT);
    PBYTE pbDet = (PBYTE)pDet;
    const BYTE* pbCS = (const BYTE*)pCS;
    PCWSTR pCompStrW = (PCWSTR)(pbCS + pCS->dwCompStrOffset);
    PCWSTR pResultStr = (PCWSTR)(pbCS + pCS->dwResultStrOffset);
    PCWSTR pResultReadStr = (PCWSTR)(pbCS + pCS->dwResultReadStrOffset);

    if (dwGCS & GCS_COMPSTR)
    {
        PSTR pchA = (PSTR)(pbDet + ib);
        INT cchAnsi = Imm32WideToAnsi(pCompStrW, pCS->dwCompStrLen, pchA, cbRequired - ib);
        pDet->uUndetTextPos = ib;
        pDet->uUndetTextLen = cchAnsi;
        (pbDet + ib)[cchAnsi] = ANSI_NULL;

        ib += ALIGN_DWORD(cchAnsi + 1);

        if ((dwGCS & GCS_COMPATTR) || pCS->dwCompAttrLen)
        {
            pDet->uUndetAttrPos = ib;

            const BYTE *pSrcAttr = pbCS + pCS->dwCompAttrOffset;
            PBYTE pDestAttr = pbDet + ib;

            UINT ibIndex = 0;
            for (UINT i = 0; i < pCS->dwCompAttrLen; ++i)
            {
                if (IsDBCSLeadByte(pchA[ibIndex]))
                {
                    pDestAttr[ibIndex++] = pSrcAttr[i];
                    pDestAttr[ibIndex++] = pSrcAttr[i];
                }
                else
                {
                    pDestAttr[ibIndex++] = pSrcAttr[i];
                }
            }

            ib += ALIGN_DWORD(ibIndex);
        }
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (pCS->dwCursorPos == (DWORD)-1)
            pDet->uCursorPos = (UINT)-1;
        else
            pDet->uCursorPos = IchAnsiFromWide(pCS->dwCursorPos, pCompStrW, CP_ACP);
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (pCS->dwDeltaStart == (DWORD)-1)
            pDet->uDeltaStart = (UINT)-1;
        else
            pDet->uDeltaStart = IchAnsiFromWide(pCS->dwDeltaStart, pCompStrW, CP_ACP);
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        INT cchAnsi = Imm32WideToAnsi(pResultStr, pCS->dwResultStrLen,
                                      (PSTR)(pbDet + ib), cbRequired - ib);
        pDet->uDetermineTextPos = ib;
        pDet->uDetermineTextLen = cchAnsi;

        ib += ALIGN_DWORD(cchAnsi + sizeof(ANSI_NULL));

        if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
        {
            pDet->uDetermineDelimPos = ib;
            const DWORD *pSrcClause = (const DWORD *)(pbCS + pCS->dwResultClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbDet + ib);
            DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);

            for (DWORD i = 0; i < nClauses; ++i)
                pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pResultStr, CP_ACP);

            ib += ALIGN_DWORD(pCS->dwResultClauseLen);
        }
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        INT cchAnsi = Imm32WideToAnsi(pResultReadStr, pCS->dwResultReadStrLen,
                                      (PSTR)(pbDet + ib), cbRequired - ib);
        pDet->uYomiTextPos = ib;
        pDet->uYomiTextLen = cchAnsi;
        ib += ALIGN_DWORD(cchAnsi + sizeof(ANSI_NULL));

        if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
        {
            pDet->uYomiDelimPos = ib;
            const DWORD *pSrcClause = (const DWORD *)(pbCS + pCS->dwResultReadClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbDet + ib);
            DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
            PCWSTR pReadW = (PCWSTR)(pbCS + pCS->dwResultReadStrOffset);

            for (DWORD i = 0; i < nClauses; ++i)
                pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pReadW, CP_ACP);

            //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
        }
    }

    return cbRequired;
}

static DWORD
Imm32CompStrWToStringExW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    LPSTRINGEXSTRUCT pSX,
    DWORD dwSize)
{
    const DWORD cbRequired =
        sizeof(STRINGEXSTRUCT) +
        ALIGN_WSTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pSX)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pSX->dwSize = dwSize;

    DWORD ib = sizeof(STRINGEXSTRUCT);
    PBYTE pBase = (PBYTE)pSX;
    const BYTE* pbCS = (const BYTE*)pCS;

    {
        pSX->uDeterminePos = ib;
        DWORD cbResultStr = pCS->dwResultStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + ib, pbCS + pCS->dwResultStrOffset, cbResultStr);
        ((PWSTR)(pBase + ib))[pCS->dwResultStrLen] = UNICODE_NULL;

        ib += ALIGN_WSTR_SIZE(pCS->dwResultStrLen);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
    {
        pSX->uDetermineDelimPos = ib;
        RtlCopyMemory(pBase + ib, pbCS + pCS->dwResultClauseOffset, pCS->dwResultClauseLen);

        ib += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    {
        pSX->uYomiPos = ib;
        DWORD cbResultReadStr = pCS->dwResultReadStrLen * sizeof(WCHAR);
        RtlCopyMemory(pBase + ib, pbCS + pCS->dwResultReadStrOffset, cbResultReadStr);
        ((PWSTR)(pBase + ib))[pCS->dwResultReadStrLen] = UNICODE_NULL;

        ib += ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
    {
        pSX->uYomiDelimPos = ib;
        RtlCopyMemory(pBase + ib, pbCS + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);

        //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
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
    const DWORD cbRequired =
        sizeof(STRINGEXSTRUCT) +
        ALIGN_ASTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_ASTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pSX)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pSX->dwSize = dwSize;

    DWORD ib = sizeof(STRINGEXSTRUCT);
    PBYTE pbSX = (PBYTE)pSX;
    const BYTE* pbCS = (const BYTE*)pCS;
    PCWSTR pResultStr = (PCWSTR)(pbCS + pCS->dwResultStrOffset);
    PCWSTR pResultReadStr = (PCWSTR)(pbCS + pCS->dwResultReadStrOffset);

    if (pCS->dwResultStrLen)
    {
        pSX->uDeterminePos = ib;
        INT nAnsiResultLen = Imm32WideToAnsi(pResultStr, pCS->dwResultStrLen,
                                             (PSTR)(pbSX + ib), dwSize - ib);
        if (nAnsiResultLen <= 0)
            return 0;

        ib += ALIGN_DWORD(nAnsiResultLen + 1);

        if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
        {
            pSX->uDetermineDelimPos = ib;
            const DWORD *pSrcClause = (const DWORD *)(pbCS + pCS->dwResultClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbSX + ib);
            DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);
            PCWSTR pResultW = (PCWSTR)(pbCS + pCS->dwResultStrOffset);

            for (DWORD i = 0; i < nClauses; ++i)
                pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pResultW, CP_ACP);

            ib += ALIGN_DWORD(pCS->dwResultClauseLen);
        }
    }

    if (pCS->dwResultReadStrLen)
    {
        pSX->uYomiPos = ib;
        INT nAnsiReadLen = Imm32WideToAnsi(pResultReadStr, pCS->dwResultReadStrLen,
                                           (PSTR)(pbSX + ib), dwSize - ib);
        if (nAnsiReadLen <= 0)
            return 0;

        ib += ALIGN_DWORD(nAnsiReadLen + 1);

        if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
        {
            pSX->uYomiDelimPos = ib;
            const DWORD *pSrcClause = (const DWORD *)(pbCS + pCS->dwResultReadClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbSX + ib);
            DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
            PCWSTR pReadW = (PCWSTR)(pbCS + pCS->dwResultReadStrOffset);

            for (DWORD i = 0; i < nClauses; ++i)
                pDestClause[i] = IchAnsiFromWide(pSrcClause[i], pReadW, CP_ACP);

            //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
        }
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToUndetA(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pDet,
    DWORD dwSize)
{
    const DWORD cbRequired =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD(pCS->dwCompStrLen + sizeof(ANSI_NULL)) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_DWORD(pCS->dwResultStrLen + sizeof(ANSI_NULL)) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(pCS->dwResultReadStrLen + sizeof(ANSI_NULL)) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pDet)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pDet->dwSize = dwSize;

    DWORD ib = sizeof(UNDETERMINESTRUCT);
    PBYTE pbDet = (PBYTE)pDet, pbCS = (PBYTE)pCS;

    if (dwGCS & GCS_COMPSTR)
    {
        pDet->uUndetTextPos = ib;
        pDet->uUndetTextLen = pCS->dwCompStrLen;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwCompStrOffset, pCS->dwCompStrLen);
        pbDet[ib + pCS->dwCompStrLen] = ANSI_NULL;

        ib += ALIGN_DWORD(pCS->dwCompStrLen + 1);

        if (pCS->dwCompAttrLen)
            dwGCS |= GCS_COMPATTR;
    }

    if (dwGCS & GCS_COMPATTR)
    {
        pDet->uUndetAttrPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwCompAttrOffset, pCS->dwCompAttrLen);

        ib += ALIGN_DWORD(pCS->dwCompAttrLen);
    }

    if (dwGCS & GCS_CURSORPOS)
        pDet->uCursorPos = pCS->dwCursorPos;

    if (dwGCS & GCS_DELTASTART)
        pDet->uDeltaStart = pCS->dwDeltaStart;

    if (dwGCS & GCS_RESULTSTR)
    {
        pDet->uDetermineTextPos = ib;
        pDet->uDetermineTextLen = pCS->dwResultStrLen;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultStrOffset, pCS->dwResultStrLen);
        pbDet[ib + pCS->dwResultStrLen] = ANSI_NULL;

        ib += ALIGN_DWORD(pCS->dwResultStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
    {
        pDet->uDetermineDelimPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultClauseOffset, pCS->dwResultClauseLen);

        ib += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        pDet->uYomiTextPos = ib;
        pDet->uYomiTextLen = pCS->dwResultReadStrLen;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultReadStrOffset, pCS->dwResultReadStrLen);
        pbDet[ib + pCS->dwResultReadStrLen] = ANSI_NULL;

        ib += ALIGN_DWORD(pCS->dwResultReadStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
    {
        pDet->uYomiDelimPos = ib;
        RtlCopyMemory(pbDet + ib, pbCS + pCS->dwResultReadClauseOffset,
                      pCS->dwResultReadClauseLen);

        //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
    }

    return dwSize;
}

static DWORD
Imm32CompStrAToUndetW(
    DWORD dwGCS,
    const COMPOSITIONSTRING *pCS,
    PUNDETERMINESTRUCT pUndet,
    DWORD dwSize)
{
    const DWORD cbRequired =
        sizeof(UNDETERMINESTRUCT) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(pCS->dwCompAttrLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_WSTR_SIZE(pCS->dwCompStrLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pUndet)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pUndet->dwSize = dwSize;

    DWORD ib = sizeof(UNDETERMINESTRUCT);
    PBYTE pbUndet = (PBYTE)pUndet;
    const BYTE* pbCS = (const BYTE*)pCS;
    PCSTR pCompStrA = (PCSTR)(pbCS + pCS->dwCompStrOffset);
    PCSTR pResultStr = (PCSTR)(pbCS + pCS->dwResultStrOffset);
    PCSTR pResultReadStr = (PCSTR)(pbCS + pCS->dwResultReadStrOffset);

    if (dwGCS & GCS_COMPSTR)
    {
        INT nWideLen = Imm32AnsiToWide(pCompStrA, pCS->dwCompStrLen,
                                       (PWSTR)(pbUndet + ib), (dwSize - ib) / sizeof(WCHAR));
        if (nWideLen <= 0)
            return 0;

        pUndet->uUndetTextPos = ib;
        pUndet->uUndetTextLen = nWideLen;

        ib += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));

        if ((dwGCS & GCS_COMPATTR) || pCS->dwCompAttrLen)
        {
            pUndet->uUndetAttrPos = ib;
            const BYTE* pSrcAttr = pbCS + pCS->dwCompAttrOffset;
            PBYTE pDestAttr = pbUndet + ib;
            PCWSTR pWStr = (PCWSTR)(pbUndet + pUndet->uUndetTextPos);

            for (UINT i = 0; i < pUndet->uUndetTextLen; ++i)
            {
                pDestAttr[i] = *pSrcAttr;
                USHORT wChar = pWStr[i];
                ULONG cbMultiByte = 0;
                RtlUnicodeToMultiByteSize(&cbMultiByte, &wChar, sizeof(WCHAR));
                pSrcAttr += (cbMultiByte == 2) ? 2 : 1;
            }

            ib += ALIGN_DWORD(pUndet->uUndetTextLen);
        }
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (pCS->dwCursorPos == (DWORD)-1)
            pUndet->uCursorPos = (UINT)-1;
        else
            pUndet->uCursorPos = IchWideFromAnsi(pCS->dwCursorPos, pCompStrA, CP_ACP);
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (pCS->dwDeltaStart == (DWORD)-1)
            pUndet->uDeltaStart = (UINT)-1;
        else
            pUndet->uDeltaStart = IchWideFromAnsi(pCS->dwDeltaStart, pCompStrA, CP_ACP);
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        INT nWideLen = Imm32AnsiToWide(pResultStr, pCS->dwResultStrLen,
                                       (PWSTR)(pbUndet + ib), (dwSize - ib) / sizeof(WCHAR));
        if (nWideLen > 0)
        {
            pUndet->uDetermineTextPos = ib;
            pUndet->uDetermineTextLen = nWideLen;

            ib += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));

            if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
            {
                pUndet->uDetermineDelimPos = ib;
                PDWORD pSrcClause = (PDWORD)(pbCS + pCS->dwResultClauseOffset);
                PDWORD pDestClause = (PDWORD)(pbUndet + ib);
                DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);

                for (DWORD i = 0; i < nClauses; ++i)
                    pDestClause[i] = IchWideFromAnsi(pSrcClause[i], pResultStr, CP_ACP);

                ib += ALIGN_DWORD(pCS->dwResultClauseLen);
            }
        }
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        INT nWideLen = Imm32AnsiToWide(pResultReadStr, pCS->dwResultReadStrLen,
                                       (PWSTR)(pbUndet + ib), (dwSize - ib) / sizeof(WCHAR));
        if (nWideLen > 0)
        {
            pUndet->uYomiTextPos = ib;
            pUndet->uYomiTextLen = nWideLen;

            ib += ALIGN_DWORD((nWideLen + 1) * sizeof(WCHAR));

            if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
            {
                pUndet->uYomiDelimPos = ib;
                PDWORD pSrcClause = (PDWORD)(pbCS + pCS->dwResultReadClauseOffset);
                PDWORD pDestClause = (PDWORD)(pbUndet + ib);
                DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);

                for (DWORD i = 0; i < nClauses; ++i)
                    pDestClause[i] = IchWideFromAnsi(pSrcClause[i], pResultReadStr, CP_ACP);

                //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
            }
        }
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
    DWORD cbRequired =
        sizeof(STRINGEXSTRUCT) +
        ALIGN_DWORD(pCS->dwResultStrLen + sizeof(ANSI_NULL)) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_DWORD(pCS->dwResultReadStrLen + sizeof(ANSI_NULL)) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (!pSX)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pSX->dwSize = dwSize;

    DWORD ib = sizeof(STRINGEXSTRUCT);
    PBYTE pbSX = (PBYTE)pSX;
    const BYTE* pbCS = (const BYTE*)pCS;

    {
        pSX->uDeterminePos = ib;
        RtlCopyMemory(pbSX + ib, pbCS + pCS->dwResultStrOffset, pCS->dwResultStrLen);
        pbSX[ib + pCS->dwResultStrLen] = ANSI_NULL;

        ib += ALIGN_DWORD(pCS->dwResultStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
    {
        pSX->uDetermineDelimPos = ib;
        RtlCopyMemory(pbSX + ib, pbCS + pCS->dwResultClauseOffset, pCS->dwResultClauseLen);

        ib += ALIGN_DWORD(pCS->dwResultClauseLen);
    }

    {
        pSX->uYomiPos = ib;
        RtlCopyMemory(pbSX + ib, pbCS + pCS->dwResultReadStrOffset, pCS->dwResultReadStrLen);
        pbSX[ib + pCS->dwResultReadStrLen] = ANSI_NULL;

        ib += ALIGN_DWORD(pCS->dwResultReadStrLen + 1);
    }

    if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
    {
        pSX->uYomiDelimPos = ib;
        RtlCopyMemory(pbSX + ib, pbCS + pCS->dwResultReadClauseOffset, pCS->dwResultReadClauseLen);

        //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
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
    const DWORD cbRequired =
        sizeof(STRINGEXSTRUCT) +
        ALIGN_WSTR_SIZE(pCS->dwResultStrLen) +
        ALIGN_DWORD(pCS->dwResultClauseLen) +
        ALIGN_WSTR_SIZE(pCS->dwResultReadStrLen) +
        ALIGN_DWORD(pCS->dwResultReadClauseLen);

    if (pSX == NULL)
        return cbRequired;

    if (dwSize < cbRequired)
        return 0;

    pSX->dwSize = dwSize;

    DWORD ib = sizeof(STRINGEXSTRUCT);
    PBYTE pbSX = (PBYTE)pSX;
    const BYTE* pbCS = (const BYTE*)pCS;
    PCSTR pResultStr = (PCSTR)(pbCS + pCS->dwResultStrOffset);
    PCSTR pResultReadStr = (PCSTR)(pbCS + pCS->dwResultReadStrOffset);

    if (pCS->dwResultStrLen)
    {
        pSX->uDeterminePos = ib;
        INT nWideResultLen = Imm32AnsiToWide(pResultStr, pCS->dwResultStrLen,
                                             (PWSTR)(pbSX + ib), (dwSize - ib) / sizeof(WCHAR));
        if (nWideResultLen <= 0)
            return 0;

        ib += ALIGN_DWORD((nWideResultLen + 1) * sizeof(WCHAR));

        if ((dwGCS & GCS_RESULTCLAUSE) && pCS->dwResultClauseLen)
        {
            pSX->uDetermineDelimPos = ib;
            const DWORD* pSrcClause = (const DWORD*)(pbCS + pCS->dwResultClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbSX + ib);
            DWORD nClauses = pCS->dwResultClauseLen / sizeof(DWORD);

            for (DWORD i = 0; i < nClauses; ++i)
            {
                pDestClause[i] = IchWideFromAnsi(pSrcClause[i],
                    (PCSTR)(pbCS + pCS->dwResultStrOffset), CP_ACP);
            }

            ib += ALIGN_DWORD(pCS->dwResultClauseLen);
        }
    }

    if (pCS->dwResultReadStrLen)
    {
        pSX->uYomiPos = ib;
        INT nWideReadLen = Imm32AnsiToWide(pResultReadStr, pCS->dwResultReadStrLen,
                                           (PWSTR)(pbSX + ib), (dwSize - ib) / sizeof(WCHAR));
        if (nWideReadLen <= 0)
            return 0;

        ib += ALIGN_DWORD((nWideReadLen + 1) * sizeof(WCHAR));

        if ((dwGCS & GCS_RESULTREADCLAUSE) && pCS->dwResultReadClauseLen)
        {
            pSX->uYomiDelimPos = ib;
            const DWORD* pSrcClause = (const DWORD*)(pbCS + pCS->dwResultReadClauseOffset);
            PDWORD pDestClause = (PDWORD)(pbSX + ib);
            DWORD nClauses = pCS->dwResultReadClauseLen / sizeof(DWORD);
            for (DWORD i = 0; i < nClauses; ++i)
                pDestClause[i] = IchWideFromAnsi(pSrcClause[i], pResultReadStr, CP_ACP);

            //ib += ALIGN_DWORD(pCS->dwResultReadClauseLen); // The last one (ineffective)
        }
    }

    return dwSize;
}

static DWORD Imm32CompStrAToStringA(const COMPOSITIONSTRING *pCS, PSTR pszString, DWORD dwSize)
{
    DWORD dwResultStrLen = pCS->dwResultStrLen;
    if (!pszString)
        return dwResultStrLen + 1;
    if (dwSize < dwResultStrLen + 1)
        return 0;
    RtlCopyMemory(pszString, (const BYTE*)pCS + pCS->dwResultStrOffset, dwResultStrLen);
    pszString[dwResultStrLen] = ANSI_NULL;
    return dwResultStrLen + 1;
}

static DWORD Imm32CompStrAToStringW(const COMPOSITIONSTRING *pCS, PWSTR pszString, DWORD dwSize)
{
    PCSTR pch = (PCSTR)pCS + pCS->dwResultStrOffset;
    INT cchWide = Imm32AnsiToWide(pch, pCS->dwResultStrLen, pszString, 0);
    DWORD dwResultStrLen = (cchWide + 1) * sizeof(WCHAR);
    if (!pszString)
        return dwResultStrLen;
    if (dwSize < dwResultStrLen)
        return 0;
    INT cch = Imm32AnsiToWide(pch, pCS->dwResultStrLen, pszString, cchWide);
    if (cch < cchWide)
        return 0;
    pszString[cch] = UNICODE_NULL;
    return (cch + 1) * sizeof(WCHAR);
}

static VOID Imm32CompStrAToCharA(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    PCSTR pch = (PCSTR)((const BYTE*)pCS + pCS->dwResultStrOffset);

    BOOL bImeSupportDBCS = FALSE;
    if (GetWin32ClientInfo()->dwExpWinVer >= 0x30A)
        bImeSupportDBCS = (BOOL)SendMessageA(hWnd, WM_IME_REPORT, IR_DBCSCHAR, 0);

    PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    if (!*pch)
    {
        PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
        return;
    }

    PCSTR pNext;
    for (; *pch; pch = pNext)
    {
        BYTE bFirst = (BYTE)(*pch);
        pNext = CharNextA(pch);

        if (!*pNext)
            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        if (!IsDBCSLeadByte(bFirst))
        {
            PostMessageA(hWnd, WM_CHAR, bFirst, 1);
            continue;
        }

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
}

static VOID Imm32CompStrAToCharW(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    PCSTR pCurrent = (PCSTR)((const BYTE*)pCS + pCS->dwResultStrOffset);

    PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    while (*pCurrent)
    {
        BYTE bTestChar = *pCurrent;
        PCHAR pNext = CharNextA(pCurrent);

        if (!*pNext)
            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        WCHAR szWide[2] = L"";
        INT nConverted = 0;
        if (IsDBCSLeadByte(bTestChar))
            nConverted = Imm32AnsiToWide(pCurrent, 2, szWide, _countof(szWide));
        else
            nConverted = Imm32AnsiToWide(pCurrent, 1, szWide, _countof(szWide));

        if (nConverted > 0)
            PostMessageW(hWnd, WM_CHAR, szWide[0], 1);

        pCurrent = pNext;
    }
}

static VOID Imm32CompStrWToCharA(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    PCWSTR pCurrent = (PCWSTR)((const BYTE*)pCS + pCS->dwResultStrOffset);

    BOOL bSupportDBCSReport = FALSE;
    if (GetWin32ClientInfo()->dwExpWinVer >= 0x30A)
        bSupportDBCSReport = (BOOL)SendMessageA(hWnd, WM_IME_REPORT, IR_DBCSCHAR, 0);

    PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    PCWSTR pNext;
    for (; *pCurrent; pCurrent = pNext)
    {
        pNext = CharNextW(pCurrent);
        if (!*pNext)
            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        CHAR szAnsi[3] = {0};
        if (!Imm32WideToAnsi(pCurrent, 1, szAnsi, _countof(szAnsi)))
            continue;

        if (!IsDBCSLeadByte(szAnsi[0]))
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
}

static VOID Imm32CompStrWToCharW(HWND hWnd, const COMPOSITIONSTRING *pCS)
{
    PCWSTR pCurrent = (PCWSTR)((const BYTE*)pCS + pCS->dwResultStrOffset);

    PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

    PCWSTR pNext;
    for (; *pCurrent; pCurrent = pNext)
    {
        pNext = CharNextW(pCurrent);

        if (!*pNext)
            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

        PostMessageW(hWnd, WM_CHAR, *pCurrent, 1);
    }
}

static INT
Imm32JTransCompositionA(
    const INPUTCONTEXTDX *pIC,
    const COMPOSITIONSTRING *pCS,
    const TRANSMSG *pCurrent,
    PTRANSMSG pEntries)
{
    HWND hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        return 0;

    BOOL bIsUnicodeWnd = IsWindowUnicode(hWnd);
    DWORD dwGCS = (DWORD)pCurrent->lParam;
    INT cMessages = 0;
    BOOL bUndeterminedProcessed = FALSE;
    BOOL bResultProcessed = FALSE;
    HGLOBAL hUndet = NULL;
    DWORD cbUndet = 0;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        if (!bIsUnicodeWnd)
        {
            cbUndet = Imm32CompStrAToUndetA(dwGCS, pCS, NULL, 0);
            if (cbUndet)
            {
                hUndet = GlobalAlloc(GHND | GMEM_SHARE, cbUndet);
                if (hUndet)
                {
                    PUNDETERMINESTRUCT pUndet = GlobalLock(hUndet);
                    if (pUndet)
                    {
                        cbUndet = Imm32CompStrAToUndetA(dwGCS, pCS, pUndet, cbUndet);
                        GlobalUnlock(hUndet);
                        if (cbUndet)
                        {
                            if (SendMessageA(hWnd, WM_IME_REPORT, IR_UNDETERMINE,
                                             (LPARAM)hUndet) == 0)
                            {
                                bUndeterminedProcessed = TRUE;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cbUndet = Imm32CompStrAToUndetW(dwGCS, pCS, NULL, 0);
            if (cbUndet)
            {
                hUndet = GlobalAlloc(GHND | GMEM_SHARE, cbUndet);
                if (hUndet)
                {
                    PUNDETERMINESTRUCT pUndet = GlobalLock(hUndet);
                    if (pUndet)
                    {
                        cbUndet = Imm32CompStrAToUndetW(dwGCS, pCS, pUndet, cbUndet);
                        GlobalUnlock(hUndet);
                        if (cbUndet)
                        {
                            if (SendMessageW(hWnd, WM_IME_REPORT, IR_UNDETERMINE,
                                             (LPARAM)hUndet) == 0)
                            {
                                bUndeterminedProcessed = TRUE;
                            }
                        }
                    }
                }
            }
        }

        if (hUndet)
            GlobalFree(hUndet);
        if (bUndeterminedProcessed)
            return 0;
    }

    HGLOBAL hEx, hStr;
    PVOID pEx = NULL, pStr = NULL;
    BOOL bExSuccess = FALSE, bStrSuccess = FALSE;
    LRESULT res;
    DWORD exSize, strSize;

    if (dwGCS & GCS_RESULTSTR)
    {
        if (dwGCS & GCS_RESULTREADSTR)
        {
            exSize = bIsUnicodeWnd ? Imm32CompStrAToStringExW(dwGCS, pCS, NULL, 0)
                                   : Imm32CompStrAToStringExA(dwGCS, pCS, NULL, 0);
            if (exSize)
            {
                hEx = GlobalAlloc(GHND | GMEM_SHARE, exSize);
                if (hEx)
                {
                    pEx = GlobalLock(hEx);
                    if (pEx)
                    {
                        bExSuccess = bIsUnicodeWnd
                            ? Imm32CompStrAToStringExW(dwGCS, pCS, pEx, exSize)
                            : Imm32CompStrAToStringExA(dwGCS, pCS, pEx, exSize);
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
                    }
                    GlobalFree(hEx);
                }
            }
        }

        strSize = bIsUnicodeWnd ? Imm32CompStrAToStringW(pCS, NULL, 0) : (pCS->dwResultStrLen + 1);
        if (strSize && strSize != (DWORD)-1)
        {
            hStr = GlobalAlloc(GHND | GMEM_SHARE, strSize);
            if (hStr)
            {
                pStr = GlobalLock(hStr);
                if (pStr)
                {
                    bStrSuccess = bIsUnicodeWnd
                        ? Imm32CompStrAToStringW(pCS, pStr, strSize)
                        : Imm32CompStrAToStringA(pCS, pStr, strSize);
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
                cMessages = 1;
            }
        }
        else
        {
            *pEntries = *pCurrent;
            cMessages = 1;
        }
    }

    return cMessages;
}

static INT
Imm32JTransCompositionW(
    const INPUTCONTEXTDX *pIC,
    const COMPOSITIONSTRING *pCS,
    const TRANSMSG *pCurrent,
    PTRANSMSG pEntries)
{
    HWND hWnd = pIC->hWnd;
    if (!IsWindow(hWnd))
        return 0;

    BOOL bIsUnicodeWnd = IsWindowUnicode(hWnd);
    DWORD dwGCS = (DWORD)pCurrent->lParam;
    INT cMessages = 0;
    BOOL bUndeterminedProcessed = FALSE;
    BOOL bResultProcessed = FALSE;

    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        HGLOBAL hUndet = NULL;
        DWORD cbUndet = 0;

        if (!bIsUnicodeWnd)
        {
            cbUndet = Imm32CompStrWToUndetA(dwGCS, pCS, NULL, 0);
            if (cbUndet)
            {
                hUndet = GlobalAlloc(GHND | GMEM_SHARE, cbUndet);
                if (hUndet)
                {
                    PUNDETERMINESTRUCT pUndet = GlobalLock(hUndet);
                    if (pUndet)
                    {
                        cbUndet = Imm32CompStrWToUndetA(dwGCS, pCS, pUndet, cbUndet);
                        GlobalUnlock(hUndet);
                        if (cbUndet)
                        {
                            if (SendMessageA(hWnd, WM_IME_REPORT, IR_UNDETERMINE,
                                             (LPARAM)hUndet) == 0)
                            {
                                bUndeterminedProcessed = TRUE;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cbUndet = Imm32CompStrWToUndetW(dwGCS, pCS, NULL, 0);
            if (cbUndet)
            {
                hUndet = GlobalAlloc(GHND | GMEM_SHARE, cbUndet);
                if (hUndet)
                {
                    PUNDETERMINESTRUCT pUndet = GlobalLock(hUndet);
                    if (pUndet)
                    {
                        cbUndet = Imm32CompStrWToUndetW(dwGCS, pCS, pUndet, cbUndet);
                        GlobalUnlock(hUndet);
                        if (cbUndet)
                        {
                            if (SendMessageW(hWnd, WM_IME_REPORT, IR_UNDETERMINE,
                                             (LPARAM)hUndet) == 0)
                            {
                                bUndeterminedProcessed = TRUE;
                            }
                        }
                    }
                }
            }
        }

        if (hUndet)
            GlobalFree(hUndet);
        if (bUndeterminedProcessed)
            return 0;
    }

    HGLOBAL hEx, hStr;
    PVOID pEx = NULL, pStr = NULL;
    BOOL bExSuccess = FALSE, bStrSuccess = FALSE;
    LRESULT res;
    DWORD exSize, strSize;

    if (dwGCS & GCS_RESULTSTR)
    {
        if (dwGCS & GCS_RESULTREADSTR)
        {
            exSize = bIsUnicodeWnd
                ? Imm32CompStrWToStringExW(dwGCS, pCS, NULL, 0)
                : Imm32CompStrWToStringExA(dwGCS, pCS, NULL, 0);
            if (exSize)
            {
                hEx = GlobalAlloc(GHND | GMEM_SHARE, exSize);
                if (hEx)
                {
                    pEx = GlobalLock(hEx);
                    if (pEx)
                    {
                        bExSuccess = bIsUnicodeWnd
                            ? Imm32CompStrWToStringExW(dwGCS, pCS, pEx, exSize)
                            : Imm32CompStrWToStringExA(dwGCS, pCS, pEx, exSize);
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
                    }
                    GlobalFree(hEx);
                }
            }
        }

        strSize = bIsUnicodeWnd
            ? Imm32CompStrWToStringW(pCS, NULL)
            : Imm32CompStrWToStringA(pCS, NULL);
        if (strSize && strSize != (DWORD)-1)
        {
            hStr = GlobalAlloc(GHND | GMEM_SHARE, strSize);
            if (hStr)
            {
                pStr = GlobalLock(hStr);
                if (pStr)
                {
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
                cMessages = 1;
            }
        }
        else
        {
            *pEntries = *pCurrent;
            cMessages = 1;
        }
    }

    return cMessages;
}

/* Japanese */
static DWORD
WINNLSTranslateMessageJ(
    INT cEntries,
    PTRANSMSG pEntries,
    const INPUTCONTEXTDX *pIC,
    const COMPOSITIONSTRING *pCS,
    BOOL bAnsi)
{
    // Clone the message list with growing
    DWORD dwBufSize = (cEntries + 1) * sizeof(TRANSMSG);
    PTRANSMSG pBuf = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwBufSize);
    if (!pBuf)
        return 0;
    RtlCopyMemory(pBuf, pEntries, cEntries * sizeof(TRANSMSG));

    HWND hWnd = pIC->hWnd;
    HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);

    PTRANSMSG pCurrent = pBuf;
    if (pIC->dwUIFlags & _IME_UI_HIDDEN)
    {
        // Find WM_IME_ENDCOMPOSITION
        PTRANSMSG pEndComp = NULL;
        for (INT i = 0; i < cEntries; ++i)
        {
            if (pBuf[i].message == WM_IME_ENDCOMPOSITION)
            {
                pEndComp = &pBuf[i];
                break;
            }
        }

        if (pEndComp)
        {
            // Move WM_IME_ENDCOMPOSITION to the end of the list
            PTRANSMSG pSrc = pEndComp + 1, pDest = pEndComp;
            for (INT iEntry = 0; iEntry < cEntries - 1 && pSrc->message != WM_NULL; ++iEntry)
                *pDest++ = *pSrc++;

            pDest->message = WM_IME_ENDCOMPOSITION;
            pDest->wParam = pDest->lParam = 0;
        }

        pCurrent = pBuf;
    }

    DWORD dwResult;
    for (dwResult = 0; pCurrent->message != WM_NULL; ++pCurrent)
    {
        switch (pCurrent->message)
        {
            case WM_IME_STARTCOMPOSITION:
                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN))
                {
                    // Send IR_OPENCONVERT
                    if (pIC->cfCompForm.dwStyle)
                        SendMessageW(hWnd, WM_IME_REPORT, IR_OPENCONVERT, 0);

                    *pEntries++ = *pCurrent;
                    ++dwResult;
                }
                break;

            case WM_IME_ENDCOMPOSITION:
                if (!(pIC->dwUIFlags & _IME_UI_HIDDEN))
                {
                    // Send IR_CLOSECONVERT
                    if (pIC->cfCompForm.dwStyle)
                        SendMessageW(hWnd, WM_IME_REPORT, IR_CLOSECONVERT, 0);

                    *pEntries++ = *pCurrent;
                    ++dwResult;
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
                INT cMessages;
                if (bAnsi)
                    cMessages = Imm32JTransCompositionA(pIC, pCS, pCurrent, pEntries);
                else
                    cMessages = Imm32JTransCompositionW(pIC, pCS, pCurrent, pEntries);

                dwResult += cMessages;
                pEntries += cMessages;

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
                {
                    *pEntries++ = *pCurrent;
                    ++dwResult;
                }
                else
                {
                    SendMessageW(hwndIme, pCurrent->message, pCurrent->wParam, pCurrent->lParam);
                }
                break;

            default:
                *pEntries++ = *pCurrent;
                ++dwResult;
                break;
        }
    }

    ImmLocalFree(pBuf);
    return dwResult;
}

typedef LRESULT (WINAPI *FN_SendMessage)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (WINAPI *FN_PostMessage)(HWND, UINT, WPARAM, LPARAM);

/* Korean */
static DWORD
WINNLSTranslateMessageK(
    INT cEntries,
    PTRANSMSG pEntries,
    const INPUTCONTEXTDX *pIC,
    const COMPOSITIONSTRING *pCS,
    BOOL bSrcIsAnsi)
{
    HWND hWnd = pIC->hWnd;
    HWND hwndIme = ImmGetDefaultIMEWnd(hWnd);
    BOOL bDestIsUnicode = IsWindowUnicode(hWnd);
    FN_PostMessage pPostMessage = bDestIsUnicode ? PostMessageW : PostMessageA;
    FN_SendMessage pSendMessage = bDestIsUnicode ? SendMessageW : SendMessageA;
    static WORD s_chKorean = 0; /* One or two Korean character(s) */

    if (cEntries <= 0)
        return 0;

    for (INT i = 0; i < cEntries; ++i)
    {
        UINT   uMsg   = pEntries[i].message;
        WPARAM wParam = pEntries[i].wParam;
        LPARAM lParam = pEntries[i].lParam;

        switch (uMsg)
        {
            case WM_IME_COMPOSITION:
                if (lParam & GCS_RESULTSTR)
                {
                    pPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

                    if (pCS->dwResultStrLen)
                    {
                        DWORD dwProcessedLen = 0;
                        while (dwProcessedLen < pCS->dwResultStrLen)
                        {
                            LPARAM lKeyData = 1; /* WM_CHAR lParam */
                            WCHAR szWide[2] = {0};
                            CHAR  szMBStr[4]  = {0};

                            if (bSrcIsAnsi)
                            {
                                PSTR pResStr = (LPSTR)((BYTE*)pCS + pCS->dwResultStrOffset);
                                BYTE bChar = pResStr[dwProcessedLen];

                                if (bDestIsUnicode)
                                {
                                    INT cbMB = 1;

                                    szMBStr[0] = bChar;
                                    if (IsDBCSLeadByte(bChar) &&
                                        dwProcessedLen + 1 < pCS->dwResultStrLen)
                                    {
                                        szMBStr[1] = pResStr[dwProcessedLen + 1];
                                        cbMB = 2;
                                        ++dwProcessedLen;
                                    }
                                    Imm32AnsiToWide(szMBStr, cbMB, szWide, _countof(szWide));
                                    PostMessageW(hWnd, WM_CHAR, szWide[0], 1);
                                }
                                else
                                {
                                    if (IsDBCSLeadByte(bChar))
                                    {
                                        if (KOR_IS_LEAD_BYTE(bChar))
                                            lKeyData = KOR_SCAN_CODE_DBCS;
                                        else
                                            lKeyData = KOR_SCAN_CODE_SBCS;

                                        PostMessageA(hWnd, WM_CHAR, bChar, lKeyData);
                                        ++dwProcessedLen;
                                        bChar = pResStr[dwProcessedLen];
                                    }
                                    PostMessageA(hWnd, WM_CHAR, bChar, lKeyData);
                                }
                            }
                            else
                            {
                                PWSTR pResStrW = (PWSTR)((PBYTE)pCS + pCS->dwResultStrOffset);
                                szWide[0] = pResStrW[dwProcessedLen];

                                if (bDestIsUnicode)
                                {
                                    PostMessageW(hWnd, WM_CHAR, szWide[0], 1);
                                }
                                else
                                {
                                    Imm32WideToAnsi(szWide, 1, szMBStr, _countof(szMBStr));
                                    BYTE bLead = (BYTE)szMBStr[0], bChar;
                                    if (IsDBCSLeadByte(bLead))
                                    {
                                        if (KOR_IS_LEAD_BYTE(bLead))
                                            lKeyData = KOR_SCAN_CODE_DBCS;
                                        else
                                            lKeyData = KOR_SCAN_CODE_SBCS;

                                        PostMessageA(hWnd, WM_CHAR, bLead, lKeyData);
                                        bChar = szMBStr[1];
                                    }
                                    else
                                    {
                                        bChar = szMBStr[0];
                                    }

                                    PostMessageA(hWnd, WM_CHAR, bChar, lKeyData);
                                }
                            }
                            ++dwProcessedLen;
                        }
                    }
                    pPostMessage(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                }

                if (wParam)
                {
                    pPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);

                    BYTE bFirst = HIBYTE(wParam), bSecond = LOBYTE(wParam);
                    s_chKorean = MAKEWORD(bFirst, bSecond);

                    if (bSrcIsAnsi)
                    {
                        if (bDestIsUnicode)
                        {
                            CHAR szTmp[2] = { (CHAR)bFirst, (CHAR)bSecond };
                            WCHAR wTmp[2];
                            if (Imm32AnsiToWide(szTmp, 2, wTmp, _countof(wTmp)))
                                PostMessageW(hWnd, WM_INTERIM, wTmp[0], KOR_INTERIM_FLAGS);
                        }
                        else
                        {
                            PostMessageA(hWnd, WM_INTERIM, bFirst, KOR_INTERIM_FLAGS);
                            PostMessageA(hWnd, WM_INTERIM, bSecond, KOR_INTERIM_FLAGS);
                        }
                    }
                    else
                    {
                        if (bDestIsUnicode)
                        {
                            PostMessageW(hWnd, WM_INTERIM, wParam, KOR_INTERIM_FLAGS);
                        }
                        else
                        {
                            WCHAR wTmp[2] = { (WCHAR)wParam, 0 };
                            CHAR szTmp[2];
                            Imm32WideToAnsi(wTmp, 1, szTmp, _countof(szTmp));
                            PostMessageA(hWnd, WM_INTERIM, (BYTE)szTmp[0], KOR_INTERIM_FLAGS);
                            PostMessageA(hWnd, WM_INTERIM, (BYTE)szTmp[1], KOR_INTERIM_FLAGS);
                        }
                    }

                    if (bDestIsUnicode)
                        PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                    else
                        PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);

                    pSendMessage(hwndIme, WM_IME_ENDCOMPOSITION, 0, 0);
                }
                else
                {
                    pPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0);
                    if (bSrcIsAnsi)
                    {
                        if (bDestIsUnicode)
                        {
                            CHAR szTmp[2];
                            szTmp[0] = LOBYTE(s_chKorean);
                            szTmp[1] = HIBYTE(s_chKorean);
                            WCHAR wTmp[2];
                            if (Imm32AnsiToWide(szTmp, 2, wTmp, _countof(wTmp)))
                                PostMessageW(hWnd, WM_CHAR, wTmp[0], KOR_SCAN_CODE_SBCS);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, KOR_KEYDOWN_FLAGS);
                        }
                        else
                        {
                            PostMessageA(hWnd, WM_CHAR, LOBYTE(s_chKorean), KOR_SCAN_CODE_SBCS);
                            PostMessageA(hWnd, WM_CHAR, HIBYTE(s_chKorean), KOR_SCAN_CODE_SBCS);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, KOR_KEYDOWN_FLAGS);
                        }
                    }
                    else
                    {
                        if (bDestIsUnicode)
                        {
                            PostMessageW(hWnd, WM_CHAR, (WCHAR)s_chKorean, KOR_SCAN_CODE_SBCS);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, KOR_KEYDOWN_FLAGS);
                        }
                        else
                        {
                            CHAR szTmp[2];
                            WCHAR wTmp[2] = { (WCHAR)s_chKorean, 0 };
                            Imm32WideToAnsi(wTmp, 1, szTmp, _countof(szTmp));
                            PostMessageA(hWnd, WM_CHAR, (BYTE)szTmp[0], KOR_SCAN_CODE_SBCS);
                            PostMessageA(hWnd, WM_CHAR, (BYTE)szTmp[1], KOR_SCAN_CODE_SBCS);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, KOR_KEYDOWN_FLAGS);
                        }
                    }
                }
                break;

            case WM_IMEKEYDOWN:
                pPostMessage(hWnd, WM_KEYDOWN, (WPARAM)LOWORD(wParam), lParam);
                break;

            case WM_IMEKEYUP:
                pPostMessage(hWnd, WM_KEYUP, (WPARAM)LOWORD(wParam), lParam);
                break;

            default:
                pSendMessage(hwndIme, uMsg, wParam, lParam);
                break;
        }
    }

    return 0;
}

#endif /* def IMM_WIN3_SUPPORT */

/* This function is used in ImmGenerateMessage and ImmTranslateMessage */
DWORD
WINNLSTranslateMessage(
    _In_ INT cEntries,
    _Inout_ PTRANSMSG pEntries,
    _In_ HIMC hIMC,
    _In_ BOOL bAnsi,
    _In_ WORD wLang)
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
        ret = WINNLSTranslateMessageK(cEntries, pEntries, pIC, pCompStr, bAnsi);
    else if (wLang == LANG_JAPANESE)
        ret = WINNLSTranslateMessageJ(cEntries, pEntries, pIC, pCompStr, bAnsi);
    else
        ret = 0;

    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
#else
    return 0;
#endif
}
