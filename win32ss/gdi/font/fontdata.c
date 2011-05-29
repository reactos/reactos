/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

W32KAPI
ULONG
APIENTRY
NtGdiGetFontData(
    IN HDC hdc,
    IN DWORD dwTable,
    IN DWORD dwOffset,
    OUT OPTIONAL PVOID pvBuf,
    IN ULONG cjBuf)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetWidthTable(
    IN HDC hdc,
    IN ULONG cSpecial,
    IN WCHAR *pwc,
    IN ULONG cwc,
    OUT USHORT *psWidth,
    OUT OPTIONAL WIDTHDATA *pwd,
    OUT FLONG *pflInfo)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    IN HDC hdc,
    IN INT cChar,
    OUT OPTIONAL LPWSTR pszOut,
    IN BOOL bAliasName)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
INT
APIENTRY
NtGdiGetTextCharsetInfo(
    IN HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    IN DWORD dwFlags)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
DWORD
APIENTRY
NtGdiGetFontUnicodeRanges(
    IN HDC hdc,
    OUT OPTIONAL LPGLYPHSET pgs)
{
    ASSERT(FALSE);
    return 0;
}

VOID
NTAPI
RFONT_vXlateGlpyhs(
    PRFONT prfnt,
    ULONG cwc,
    WCHAR *pwc,
    HGLYPH *phg,
    HGLYPH hgDefault)
{
    FD_GLYPHSET *pfdg = prfnt->ppfe->pfdg;
    WCRUN *pwcrun;
    HGLYPH hg;
    WCHAR wc;
    ULONG idx;

    /* Loop all WCHARs */
    while (cwc--)
    {
        wc = *pwc++;
        hg = hgDefault;

        /* Loop all WCHAR runs */
        for (pwcrun = &pfdg->awcrun[0];
             pwcrun < &pfdg->awcrun[pfdg->cRuns];
             pwcrun++)
        {
            /* Check if the char is below the current run */
            if (wc < pwcrun->wcLow)
            {
                /* We couldn't find it, use default */
                break;
            }

            /* Calculate index into the current run */
            idx = wc - pwcrun->wcLow;
            if (idx < pwcrun->cGlyphs)
            {
                hg = pwcrun->phg[idx];
                break;
            }
        }

        *phg++ = hg;
    }
}


W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesWInternal(
    IN HDC hdc,
    IN OPTIONAL LPWSTR pwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD pgi,
    IN DWORD iMode,
    IN BOOL bSubset)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
ULONG
APIENTRY
NtGdiGetKerningPairs(
    IN HDC hdc,
    IN ULONG cPairs,
    OUT OPTIONAL KERNINGPAIR *pkpDst)
{
    ASSERT(FALSE);
    return 0;
}


W32KAPI
BOOL
APIENTRY
NtGdiGetRealizationInfo(
    IN HDC hdc,
    OUT PREALIZATION_INFO pri,
    IN HFONT hf)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
ULONG
APIENTRY
NtGdiGetGlyphOutline(
    IN HDC hdc,
    IN WCHAR wch,
    IN UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf,
    IN LPMAT2 pmat2,
    IN BOOL bIgnoreRotation)
{
    ASSERT(FALSE);
    return 0;
}
