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
BOOL WINAPI LpkExtTextOut(HDC hdc, int x, int y,
                          UINT fuOptions, const RECT *lprc, LPCWSTR lpString,
                          UINT uCount , const INT *lpDx, INT unknown)
{   
    LPWSTR lpReorderedString = (LPWSTR)lpString;
    WORD *glyphs = NULL;
    INT cGlyphs;
    
    UNREFERENCED_PARAMETER(unknown);
 
    fuOptions |= ETO_IGNORELANGUAGE;
    
    /* Check if the string requires complex script processing */
    if (ScriptIsComplex(lpString, uCount, SIC_COMPLEX) == S_OK)
    {
            lpReorderedString = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WCHAR));

            BIDI_Reorder(hdc, lpString, uCount, GCP_REORDER, WINE_GCPW_FORCE_LTR,
                         lpReorderedString, uCount, NULL, &glyphs, &cGlyphs);
                         
            if (glyphs)
            {
                fuOptions |= ETO_GLYPH_INDEX;
            }
            
            if(uCount > cGlyphs)
                cGlyphs = uCount;

            return ExtTextOutW(hdc, x, y, fuOptions, lprc, (LPWSTR)glyphs, cGlyphs, lpDx);
            
    }
    
    return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpString, uCount, lpDx);
}
