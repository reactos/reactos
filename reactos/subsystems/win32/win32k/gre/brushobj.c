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


static const USHORT HatchBrushes[NB_HATCH_STYLES][8] =
{
    {0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}, /* HS_HORIZONTAL */
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /* HS_VERTICAL   */
    {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, /* HS_FDIAGONAL  */
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}, /* HS_BDIAGONAL  */
    {0x08, 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08}, /* HS_CROSS      */
    {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}  /* HS_DIAGCROSS  */
};

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

    /* Initialize default pattern bitmap size (24x1) */
    szPatSize.cx = 24;
    szPatSize.cy = 1;

    // If dwPenStyle is PS_COSMETIC, the width must be set to 1.
    if ( !(bOldStylePen) && ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) && ( dwWidth != 1) )
    {
        EngFreeMem(pBrush);
        return NULL;
    }

    /* Default to a pen which needs realization */
    pBrush->BrushObj.iSolidColor = 0xFFFFFFFF;

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
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternAlternate), (PVOID)PatternAlternate);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDot), (PVOID)PatternDot);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASH:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDash), (PVOID)PatternDash);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASHDOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
        pPattern = SURFACE_Lock(pBrush->hbmPattern);
        GreSetBitmapBits(pPattern, sizeof(PatternDashDot), (PVOID)PatternDashDot);
        SURFACE_Unlock(pPattern);
        break;

    case PS_DASHDOTDOT:
        pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
        pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
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

    /* Set BITMAP and USER_BITMAP flags */
    pBrush->flAttrs |= GDIBRUSH_IS_BITMAP;
    pBrush->flAttrs |= GDIBRUSH_USER_BITMAP;

    /* Set bitmap */
    pBrush->hbmPattern = hbmPattern;

    /* Set color to the reserved value */
    pBrush->BrushObj.iSolidColor = 0xFFFFFFFF;

    /* Return newly created brush */
    return pBrush;
}

PBRUSHGDI
NTAPI
GreCreateHatchedBrush(INT iHatchStyle, COLORREF crColor)
{
    PBRUSHGDI pBrush;
    SIZEL szPatSize;
    PSURFACE pPattern;

    /* Make sure hatch style is in range */
    if (iHatchStyle < 0 || iHatchStyle >= NB_HATCH_STYLES)
        return NULL;

    /* Allocate memory for the object */
    pBrush = EngAllocMem(FL_ZERO_MEMORY, sizeof(BRUSHGDI), TAG_BRUSHOBJ);
    if (!pBrush) return NULL;

    /* Set HATCH flag */
    pBrush->flAttrs |= GDIBRUSH_IS_HATCH;

    /* Create and set pattern bitmap */
    szPatSize.cx = 8; szPatSize.cy = 8;
    pBrush->hbmPattern = GreCreateBitmap(szPatSize, 0, BMF_1BPP, BMF_NOZEROINIT, NULL);
    GDIOBJ_SetOwnership(pBrush->hbmPattern, NULL);
    pPattern = SURFACE_Lock(pBrush->hbmPattern);
    GreSetBitmapBits(pPattern, 8 * sizeof(USHORT), (PVOID)HatchBrushes[iHatchStyle]);
    SURFACE_Unlock(pPattern);

    /* Set color to the reserved value */
    pBrush->BrushObj.iSolidColor = 0xFFFFFFFF;

    /* Set real brush color */
    pBrush->crForegroundClr = crColor;

    /* Return newly created brush */
    return pBrush;
}

VOID
NTAPI
GreFreeBrush(PBRUSHGDI pBrush)
{
    /* Free the pattern bitmap if it wasn't user-provided */
    if (pBrush->hbmPattern && !(pBrush->flAttrs & GDIBRUSH_USER_BITMAP))
    {
        GDIOBJ_SetOwnership(pBrush->hbmPattern, PsGetCurrentProcess());
        GreDeleteBitmap(pBrush->hbmPattern);
    }

    /* Free the memory */
    EngFreeMem(pBrush);
}

SURFOBJ *
NTAPI
GreRealizeBrush(PBRUSHGDI GdiBrush, PSURFACE psurfPattern, SURFOBJ *OutputObj)
{
    SURFOBJ *psoRealize;
    HBITMAP hbmpRealize;
    POINTL ptlSrc = {0, 0};
    RECTL rclDest;
    HPALETTE hPalette;
    LONG lWidth;

    rclDest = (RECTL){0, 0, psurfPattern->SurfObj.sizlBitmap.cx, psurfPattern->SurfObj.sizlBitmap.cy};

    hPalette = NULL;//pDC->pBitmap->hDIBPalette; // FIXME: use dest surface palette!
    if (!hPalette) hPalette = pPrimarySurface->DevInfo.hpalDefault;

    lWidth = DIB_GetDIBWidthBytes(psurfPattern->SurfObj.sizlBitmap.cx, BitsPerFormat(OutputObj->iBitmapFormat));
    hbmpRealize = EngCreateBitmap(psurfPattern->SurfObj.sizlBitmap,
                                  lWidth,
                                  OutputObj->iBitmapFormat,
                                  BMF_NOZEROINIT,
                                  NULL);


    psoRealize = EngLockSurface(hbmpRealize);
    if (!psoRealize)
    {
        EngDeleteSurface(hbmpRealize);
        return FALSE;
    }

    GdiBrush->XlateObject = IntEngCreateSrcMonoXlate(hPalette, GdiBrush->crBackgroundClr, GdiBrush->crForegroundClr);

    EngCopyBits(psoRealize, &psurfPattern->SurfObj, NULL, GdiBrush->XlateObject, &rclDest, &ptlSrc);

    EngUnlockSurface(psoRealize);

    return psoRealize;
}

VOID
NTAPI
GreUpdateBrush(PBRUSHGDI pBrush, PDC pDc)
{
    /* Update background color, important for hatch brushes */
    pBrush->crBackgroundClr = pDc->crBackgroundClr;
}

/* EOF */
