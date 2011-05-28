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
