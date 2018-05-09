/*
 * PROJECT:     ReactOS LPK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Language Pack DLL.
 * COPYRIGHT:   Copyright 2018 Baruch Rutman & the Wine Project 
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

        if (glyphs)
        {
            fuOptions |= ETO_GLYPH_INDEX;
        }

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
    WORD *glyphs = NULL;
    UINT *lpOrder = NULL;
    LPWSTR lpOutString;
    INT cGlyphs;

    UNREFERENCED_PARAMETER(dwUnused);
    
    /* Sanity checks */
    if ( !(dwFlags & GCP_REORDER))
       return FALSE;

    lpOutString = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WCHAR));

    BIDI_Reorder(hdc, lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpOutString,
                 lpResults->nGlyphs, lpOrder, &glyphs, &cGlyphs);

    if (lpResults->lpOutString)
        wcscpy(lpResults->lpOutString, lpOutString);

    if (lpResults->lpOrder)
        memcpy(lpResults->lpOrder, lpOrder, sizeof(lpOrder));
    
    if (lpResults->lpGlyphs)
        wcscpy(lpResults->lpGlyphs, glyphs);
    
    return TRUE;
}

