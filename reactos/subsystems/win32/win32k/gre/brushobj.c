/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/brushobj.c
 * PURPOSE:         Pen and brushes support
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

PBRUSHGDI
NTAPI
GreCreatePen(
   DWORD dwPenStyle,
   DWORD dwWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pStyle,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen)
{
    static const BYTE PatternAlternate[] = {0x55, 0x55, 0x55};
    static const BYTE PatternDash[] = {0xFF, 0xFF, 0xC0};
    static const BYTE PatternDot[] = {0xE3, 0x8E, 0x38};
    static const BYTE PatternDashDot[] = {0xFF, 0x81, 0xC0};
    static const BYTE PatternDashDotDot[] = {0xFF, 0x8E, 0x38};
    PBRUSHGDI pBrush;
    XLATEOBJ *pXlate;
    HPALETTE hPalette;
    SIZEL szPatSize;
    PSURFACE pPattern;

    /* Allocate memory for the object */
    pBrush = EngAllocMem(FL_ZERO_MEMORY, sizeof(BRUSHGDI), TAG_BRUSHOBJ);
    if (!pBrush) return NULL;

    /* Normalize the width */
    dwWidth = abs(dwWidth);

    /* Check pen style */
    if ((dwPenStyle & PS_STYLE_MASK) == PS_NULL)
    {
        //DPRINT1("NULL pen requested!\n");
        //return StockObjects[NULL_PEN];
        /* Stock objects support is missing, so we just fall back */
    }

    /* If nWidth is zero, the pen is a single pixel wide, regardless
       of the current transformation. */
    if ((bOldStylePen) && (!dwWidth) && (dwPenStyle & PS_STYLE_MASK) != PS_SOLID)
        dwWidth = 1;

    pBrush->ptPenWidth.x = dwWidth;
    pBrush->ptPenWidth.y = 0;
    pBrush->ulPenStyle = dwPenStyle;
    pBrush->BrushObj.iSolidColor = 0xFFFFFFFF;
    pBrush->ulStyle = ulBrushStyle;
    //pbrushPen->hbmClient = (HANDLE)ulClientHatch;
    pBrush->dwStyleCount = dwStyleCount;
    pBrush->pStyle = pStyle;

    pBrush->flAttrs = bOldStylePen? GDIBRUSH_IS_OLDSTYLEPEN : GDIBRUSH_IS_PEN;

    /* Initialize default pattern bitmap size */
    szPatSize.cx = 1; szPatSize.cy = 1;

    // If dwPenStyle is PS_COSMETIC, the width must be set to 1.
    if ( !(bOldStylePen) && ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) && ( dwWidth != 1) )
    {
        EngFreeMem(pBrush);
        return NULL;
    }

    switch (dwPenStyle & PS_STYLE_MASK)
    {
    case PS_NULL:
        pBrush->flAttrs |= GDIBRUSH_IS_NULL;
        pBrush->BrushObj.iSolidColor = 0;
        break;

    case PS_SOLID:
        pBrush->flAttrs |= GDIBRUSH_IS_SOLID;

        // FIXME: Take hDIBPalette in account if it exists!
        hPalette = pPrimarySurface->DevInfo.hpalDefault;
        pXlate = IntEngCreateXlate(0, PAL_RGB, hPalette, NULL);
        pBrush->BrushObj.iSolidColor = XLATEOBJ_iXlate(pXlate, ulColor);
        EngDeleteXlate(pXlate);
        break;

    case PS_ALTERNATE:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 1, BMF_24BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDotDot), (PVOID)PatternAlternate);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 1, BMF_24BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDotDot), (PVOID)PatternDot);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASH:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 1, BMF_24BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDotDot), (PVOID)PatternDash);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASHDOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 1, BMF_24BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDotDot), (PVOID)PatternDashDot);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASHDOTDOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 1, BMF_24BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDotDot), (PVOID)PatternDashDotDot);
        SURFACE_Unlock(pPattern);
        break;

    case PS_INSIDEFRAME:
        pBrush->flAttrs |= (GDIBRUSH_IS_SOLID|GDIBRUSH_IS_INSIDEFRAME);
        break;

    case PS_USERSTYLE:
        DPRINT1("PS_COSMETIC | PS_USERSTYLE not handled\n");
        break;

    default:
        DPRINT1("GreCreatePen unknown penstyle %x\n", dwPenStyle);
    }

    return pBrush;
}

PBRUSHGDI
NTAPI
GreCreateNullBrush()
{
    PBRUSHGDI pBrush;

    /* Allocate memory for the object */
    pBrush = EngAllocMem(FL_ZERO_MEMORY, sizeof(BRUSHGDI), TAG_BRUSHOBJ);
    if (!pBrush) return NULL;

    /* Set NULL flag */
    pBrush->flAttrs |= GDIBRUSH_IS_NULL;

    /* Return newly created brush */
    return pBrush;
}


PBRUSHGDI
NTAPI
GreCreateSolidBrush(COLORREF crColor)
{
    PBRUSHGDI pBrush;
    XLATEOBJ *pXlate;
    HPALETTE hPalette;

    /* Allocate memory for the object */
    pBrush = EngAllocMem(FL_ZERO_MEMORY, sizeof(BRUSHGDI), TAG_BRUSHOBJ);
    if (!pBrush) return NULL;

    /* Set SOLID flag */
    pBrush->flAttrs |= GDIBRUSH_IS_SOLID;

    /* Set color */
    // FIXME: Take hDIBPalette in account if it exists!
    hPalette = pPrimarySurface->DevInfo.hpalDefault;
    pXlate = IntEngCreateXlate(0, PAL_RGB, hPalette, NULL);
    pBrush->BrushObj.iSolidColor = XLATEOBJ_iXlate(pXlate, crColor);
    EngDeleteXlate(pXlate);

    /* Return newly created brush */
    return pBrush;
}

PBRUSHGDI
NTAPI
GreCreatePatternBrush(HBITMAP hbmPattern)
{
    PBRUSHGDI pBrush;

    /* Allocate memory for the object */
    pBrush = EngAllocMem(FL_ZERO_MEMORY, sizeof(BRUSHGDI), TAG_BRUSHOBJ);
    if (!pBrush) return NULL;

    /* Set SOLID flag */
    pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;

    /* Set bitmap */
    pBrush->hbmPattern = hbmPattern;

    /* Set color to the reserved value */
    pBrush->BrushObj.iSolidColor = 0xFFFFFFFF;

    /* Return newly created brush */
    return pBrush;
}

VOID
NTAPI
GreFreeBrush(PBRUSHGDI pBrush)
{
    /* Just free the memory */
    EngFreeMem(pBrush);
}

/* EOF */
