/*
 * PROJECT:     ReactOS LPK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Language Pack DLL.
 * PROGRAMMERS: Magnus Olsen  (greatlrd)
 *              Baruch Rutman (peterooch at gmail dot com)
 */

#include "ros_lpk.h"

LPK_LPEDITCONTROL_LIST LpkEditControl = {EditCreate,       EditIchToXY,  EditMouseToIch, EditCchInWidth,
                                         EditGetLineWidth, EditDrawText, EditHScroll,    EditMoveSelection,
                                         EditVerifyText,   EditNextWord, EditSetMenu,    EditProcessMenu,
                                         EditCreateCaret, EditAdjustCaret};

BOOL
WINAPI
DllMain(
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
{

    return LpkDllInitialize(hDll,dwReason,lpReserved);
}

BOOL
WINAPI
LpkDllInitialize(
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDll);
            /* Tell usp10 it is activated usp10 */
            //LpkPresent();
            break;

        default:
            break;
    }

    return TRUE;
}


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

    /* Check if the string requires complex script processing and not a "glyph indices" array */
    if (ScriptIsComplex(lpString, uCount, SIC_COMPLEX) == S_OK && !(fuOptions & ETO_GLYPH_INDEX))
    {
        BIDI_Reorder(hdc, lpString, uCount, GCP_REORDER, WINE_GCPW_FORCE_LTR,
                     NULL, uCount, NULL, &glyphs, &cGlyphs);

        fuOptions |= ETO_GLYPH_INDEX;

        if (uCount > cGlyphs)
        cGlyphs = uCount;

        return ExtTextOutW(hdc, x, y, fuOptions, lprc, (LPWSTR)glyphs, cGlyphs, lpDx);
    }

    if (!(fuOptions & ETO_IGNORELANGUAGE))
        fuOptions |= ETO_IGNORELANGUAGE;

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

    /* Sanity check (most likely a direct call) */
    if (!(dwFlags & GCP_REORDER))
       return GetCharacterPlacementW(hdc, lpString, uCount, nMaxExtent, lpResults, dwFlags);

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
