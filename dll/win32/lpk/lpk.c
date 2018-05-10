/*
 * PROJECT:     ReactOS LPK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Language Pack DLL.
 * PROGRAMMERS: Baruch Rutman (peterooch at gmail dot com)
 */

#include "ros_lpk.h"

/*
 * @implemented 
 */
BOOL
WINAPI
LpkExtTextOut(
    HDC hdc,
    int x,
    int y,
    UINT fuOptions,
    const RECT *lprc,
    LPCWSTR lpString,
    UINT uCount,
    const INT *lpDx,
    INT unknown)
{
    WORD *glyphs = NULL;
    INT cGlyphs;

    UNREFERENCED_PARAMETER(unknown);

    fuOptions |= ETO_IGNORELANGUAGE;

    /* Check if the string requires complex script processing */
    if (ScriptIsComplex(lpString, uCount, SIC_COMPLEX) == S_OK)
    {
        BIDI_Reorder(hdc, lpString, uCount, GCP_REORDER, WINE_GCPW_FORCE_LTR,
                     NULL, uCount, NULL, &glyphs, &cGlyphs);

        fuOptions |= ETO_GLYPH_INDEX;

        if (uCount > cGlyphs)
        cGlyphs = uCount;

        return ExtTextOutW(hdc, x, y, fuOptions, lprc, (LPWSTR)glyphs, cGlyphs, lpDx);
    }

    return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpString, uCount, lpDx);
}

/*
 * @implemented
 */
DWORD
WINAPI
LpkGetCharacterPlacement(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSW *lpResults,
    DWORD dwFlags,
    DWORD dwUnused)
{
    LPWSTR lpOutString;
    WORD *glyphs = NULL;
    UINT *lpOrder = NULL;
    UINT nSet;
    INT cGlyphs;

    UNREFERENCED_PARAMETER(dwUnused);

    /* Sanity check */
    if (!(dwFlags & GCP_REORDER))
       return FALSE;

    lpOutString = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WCHAR));

    nSet = (UINT)uCount;
    if (nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    BIDI_Reorder(hdc, lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpOutString,
                 nSet, lpOrder, &glyphs, &cGlyphs);

    lpResults->nGlyphs = (UINT)cGlyphs;

    if (lpResults->lpOutString && lpOutString)
        wcscpy(lpResults->lpOutString, lpOutString);

    if (lpResults->lpOrder)
        memcpy(lpResults->lpOrder, lpOrder, sizeof(lpOrder));

    if (lpResults->lpGlyphs)
        wcscpy(lpResults->lpGlyphs, glyphs);

    HeapFree(GetProcessHeap(), 0, lpOutString);
    HeapFree(GetProcessHeap(), 0, lpOrder);
    HeapFree(GetProcessHeap(), 0, glyphs);

    return TRUE;
}
