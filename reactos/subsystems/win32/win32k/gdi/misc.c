/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gdi/bitmap.c
 * PURPOSE:         ReactOS GDI misc support functions
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL APIENTRY RosGdiArc( HDC physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiChord( HDC physDev, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiEllipse( HDC physDev, INT left, INT top, INT right, INT bottom )
{
    UNIMPLEMENTED;
    return FALSE;
}

INT APIENTRY RosGdiExtEscape( HDC physDev, INT escape, INT in_count, LPCVOID in_data,
                            INT out_count, LPVOID out_data )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiExtFloodFill( HDC physDev, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiExtTextOut( HDC physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx, gsCacheEntryFormat *formatEntry )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Call GRE routine */
    GreTextOut(pDC, x, y, flags, lprect, wstr, count, lpDx, formatEntry);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    return TRUE;
}

BOOL APIENTRY RosGdiLineTo( HDC physDev, INT x1, INT y1, INT x2, INT y2 )
{
    PDC pDC;
    CLIPOBJ *pClipObj;
    RECTL scrRect;
    POINT pt[2];

    DPRINT("LineTo: (%d,%d)-(%d,%d)\n", x1, y1, x2, y2);

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    scrRect.left = scrRect.top = 0;
    scrRect.right = 800;
    scrRect.bottom = 600;

    pClipObj = IntEngCreateClipRegion(1, NULL, &scrRect);

    pt[0].x = x1; pt[0].y = y1;
    pt[1].x = x2; pt[1].y = y2;

    /* Add DC origin */
    pt[0].x += pDC->ptVportOrg.x + pDC->rcDcRect.left;
    pt[0].y += pDC->ptVportOrg.y + pDC->rcDcRect.top;
    pt[1].x += pDC->ptVportOrg.x + pDC->rcDcRect.left;
    pt[1].y += pDC->ptVportOrg.y + pDC->rcDcRect.top;

    GreLineTo(&pDC->pBitmap->SurfObj,
              pClipObj,
              //pDC->CombinedClip,
              &(pDC->pLineBrush->BrushObj),
              pt[0].x, pt[0].y,
              pt[1].x, pt[1].y,
              &scrRect,
              0);

    IntEngDeleteClipRegion(pClipObj);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    return TRUE;
}

BOOL APIENTRY RosGdiPie( HDC physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPolyPolygon( HDC physDev, const POINT* pt, const INT* counts, UINT polygons)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPolyPolyline( HDC physDev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPolygon( HDC physDev, const POINT* pt, INT count )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Draw the polygon */
    GrePolygon(pDC, pt, count);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    return TRUE;
}

BOOL APIENTRY RosGdiPolyline( HDC physDev, const POINT* pt, INT count )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Draw the polygon */
    GrePolyline(pDC, pt, count);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    return TRUE;
}

UINT APIENTRY RosGdiRealizeDefaultPalette( HDC physDev )
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT APIENTRY RosGdiRealizePalette( HDC physDev, HPALETTE hpal, BOOL primary )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiRectangle(HDC physDev, PRECT rc)
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = GDI_GetObjPtr(physDev, (SHORT)GDI_OBJECT_TYPE_DC);

    /* Draw the rectangle */
    GreRectangle(pDC, rc->left, rc->top, rc->right, rc->bottom);

    /* Release the object */
    GDI_ReleaseObj(physDev);

    return TRUE;
}

BOOL APIENTRY RosGdiRoundRect( HDC physDev, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiSwapBuffers(HDC physDev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiUnrealizePalette( HPALETTE hpal )
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
