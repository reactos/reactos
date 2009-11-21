/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gdi/dc.c
 * PURPOSE:         ReactOS GDI DC syscalls
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#include "object.h"
#include "handle.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

extern PDEVOBJ PrimarySurface;
BOOL FASTCALL IntCreatePrimarySurface();

/* PUBLIC FUNCTIONS **********************************************************/

// FIXME: don't use floating point in the kernel! use XFORMOBJ function
static
BOOL
NTAPI
IntGdiCombineTransform(
    LPXFORM XFormResult,
    LPXFORM xform1,
    LPXFORM xform2)
{
    XFORM xformTemp;
    /* Check for illegal parameters */
    if (!XFormResult || !xform1 || !xform2)
    {
        return  FALSE;
    }

    /* Create the result in a temporary XFORM, since xformResult may be
     * equal to xform1 or xform2 */
    xformTemp.eM11 = xform1->eM11 * xform2->eM11 + xform1->eM12 * xform2->eM21;
    xformTemp.eM12 = xform1->eM11 * xform2->eM12 + xform1->eM12 * xform2->eM22;
    xformTemp.eM21 = xform1->eM21 * xform2->eM11 + xform1->eM22 * xform2->eM21;
    xformTemp.eM22 = xform1->eM21 * xform2->eM12 + xform1->eM22 * xform2->eM22;
    xformTemp.eDx  = xform1->eDx  * xform2->eM11 + xform1->eDy  * xform2->eM21 + xform2->eDx;
    xformTemp.eDy  = xform1->eDx  * xform2->eM12 + xform1->eDy  * xform2->eM22 + xform2->eDy;
    *XFormResult = xformTemp;

    return TRUE;
}

// FIXME: Don't use floating point in the kernel!
static
BOOL
NTAPI
DC_InvertXform(const XFORM *xformSrc,
               XFORM *xformDest)
{
    FLOAT  determinant;

    determinant = xformSrc->eM11*xformSrc->eM22 - xformSrc->eM12*xformSrc->eM21;
    if (determinant > -1e-12 && determinant < 1e-12)
    {
        return  FALSE;
    }

    xformDest->eM11 =  xformSrc->eM22 / determinant;
    xformDest->eM12 = -xformSrc->eM12 / determinant;
    xformDest->eM21 = -xformSrc->eM21 / determinant;
    xformDest->eM22 =  xformSrc->eM11 / determinant;
    xformDest->eDx  = -xformSrc->eDx * xformDest->eM11 - xformSrc->eDy * xformDest->eM21;
    xformDest->eDy  = -xformSrc->eDx * xformDest->eM12 - xformSrc->eDy * xformDest->eM22;

    return  TRUE;
}

BOOL APIENTRY RosGdiCreateDC( PROS_DCINFO dc, HDC *pdev, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData )
{
    HGDIOBJ hNewDC;
    PDC pNewDC;
    XFORM xformWorld2Vport, xformVport2World;

    /* TESTING: Create primary surface */
    if (!PrimarySurface.pSurface && !IntCreatePrimarySurface())
        return STATUS_UNSUCCESSFUL;

    /* Allocate storage for DC structure */
    pNewDC = (PDC)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DC);
    if (!pNewDC) return FALSE;

    /* Save a handle to it */
    hNewDC = pNewDC->BaseObject.hHmgr;

    /* Set physical device pointer */
    pNewDC->pPDevice = (PVOID)&PrimarySurface;

    /* Initialize xform objects */
    IntGdiCombineTransform(&xformWorld2Vport, &dc->xfWorld2Wnd, &dc->xfWnd2Vport);

    /* Create inverse of world-to-viewport transformation */
    DC_InvertXform(&xformWorld2Vport, &xformVport2World);
    XForm2MatrixS(&(pNewDC->mxWorldToDevice), &xformWorld2Vport);
    XForm2MatrixS(&(pNewDC->mxDeviceToWorld), &xformVport2World);

    /* Save the world transformation */
    XForm2MatrixS(&(pNewDC->mxWorldToPage), &dc->xfWorld2Wnd);

    /* Set default fg/bg colors */
    pNewDC->crBackgroundClr = RGB(255, 255, 255);
    pNewDC->crForegroundClr = RGB(0, 0, 0);

    /* Check if it's a compatible DC */
    if (*pdev)
    {
        DPRINT("Creating a compatible with %x DC!\n", *pdev);
    }

    /* Create default NULL brushes */
    pNewDC->pLineBrush = GreCreateNullBrush();
    pNewDC->pFillBrush = GreCreateNullBrush();

    if (dc->dwType == OBJ_MEMDC)
    {
        DPRINT("Creating a memory DC %x\n", hNewDC);
        pNewDC->pBitmap = SURFACE_ShareLock(hStockBmp);

        /* Set DC rectangles */
        pNewDC->rcDcRect.left = 0; pNewDC->rcDcRect.top = 0;
        pNewDC->rcDcRect.right = 1; pNewDC->rcDcRect.bottom = 1;
        pNewDC->rcVport = pNewDC->rcDcRect;
    }
    else
    {
        DPRINT("Creating a display DC %x\n", hNewDC);
        pNewDC->pBitmap = SURFACE_ShareLock(PrimarySurface.pSurface);

        /* Set DC rectangles */
        pNewDC->rcVport.left = 0;
        pNewDC->rcVport.top = 0;
        pNewDC->rcVport.right = PrimarySurface.GDIInfo.ulHorzRes;
        pNewDC->rcVport.bottom = PrimarySurface.GDIInfo.ulVertRes;

        pNewDC->rcDcRect = pNewDC->rcVport;
    }

    /* Create an empty combined clipping region */
    pNewDC->CombinedClip = EngCreateClip();
    pNewDC->Clipping = create_empty_region();

    /* Set default palette */
    pNewDC->hPalette = hSystemPal;

    /* Give handle to the caller */
    *pdev = hNewDC;

    /* Unlock the DC */
    DC_Unlock(pNewDC);

    /* Indicate success */
    return TRUE;
}

BOOL APIENTRY RosGdiDeleteDC( HDC physDev )
{
    PDC pDC = DC_Lock(physDev);

    DPRINT("RosGdiDeleteDC(%x)\n", physDev);

    /* Release the surface */
    SURFACE_ShareUnlock(pDC->pBitmap);

    /* Unlock DC */
    DC_Unlock(pDC);

    /* Free DC */
    GDIOBJ_FreeObjByHandle(physDev, GDI_OBJECT_TYPE_DC);

    /* Indicate success */
    return TRUE;
}

BOOL APIENTRY RosGdiGetDCOrgEx( HDC physDev, LPPOINT lpp )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPaintRgn( HDC physDev, HRGN hrgn )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiSelectBitmap( HDC physDev, HBITMAP hbitmap, BOOL bStock )
{
    PDC pDC;
    PSURFACE pSurface;
    HGDIOBJ hBmpKern;

    if (bStock)
    {
        /* Selecting stock bitmap */
        hBmpKern = hStockBmp;
    }
    else
    {
        /* Selecting usual bitmap */
        hBmpKern = GDI_MapUserHandle(hbitmap);
        if (!hBmpKern)
        {
            DPRINT1("Trying to select an unknown bitmap %x to the DC %x!\n", hbitmap, physDev);
            return FALSE;
        }
    }

    DPRINT("Selecting %x bitmap to hdc %x\n", hBmpKern, physDev);

    /* Get a pointer to the DC and the bitmap*/
    pDC = DC_Lock(physDev);
    pSurface = SURFACE_ShareLock(hBmpKern);

    /* Release the old bitmap */
    SURFACE_ShareUnlock(pDC->pBitmap);

    /* Select it */
    pDC->pBitmap = pSurface;

    /* Set DC rectangles */
    pDC->rcVport.left = 0;
    pDC->rcVport.top = 0;
    pDC->rcVport.right = pSurface->SurfObj.sizlBitmap.cx;
    pDC->rcVport.bottom = pSurface->SurfObj.sizlBitmap.cy;
    pDC->rcDcRect = pDC->rcVport;

    /* Release the DC object */
    DC_Unlock(pDC);

    return TRUE;
}

// TODO: Move somewhere, give it a better name
XLATEOBJ *GrepBrushCreateXlate(PDC pDC, PBRUSHGDI pBrush, COLORREF lbColor)
{
    SURFACE *pSurfPattern;
    HPALETTE hPalette;

    pSurfPattern = SURFACE_Lock(pBrush->hbmPattern);
    if (!pSurfPattern) return NULL;

    /* Get default palette */
    hPalette = pDC->pBitmap->hDIBPalette;
    if (!hPalette) hPalette = pPrimarySurface->DevInfo.hpalDefault;

    /* Special case: 1bpp pattern */
    if (pSurfPattern->SurfObj.iBitmapFormat == BMF_1BPP)
    {
        if (BitsPerFormat(pDC->pBitmap->SurfObj.iBitmapFormat) != 1)
        {
            pBrush->XlateObject =
                IntEngCreateSrcMonoXlate(hPalette,
                                         pDC->crBackgroundClr,
                                         lbColor);
        }
    }
    else if (pBrush->flAttrs & GDIBRUSH_IS_DIB)
    {
        pBrush->XlateObject =
            IntEngCreateXlate(0, 0, hPalette, pSurfPattern->hDIBPalette);
    }

    SURFACE_Unlock(pSurfPattern);

    return pBrush->XlateObject;
}

VOID APIENTRY RosGdiSelectBrush( HDC physDev, LOGBRUSH *pLogBrush )
{
    PDC pDC;
    HGDIOBJ hBmpKern;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    DPRINT("RosGdiSelectBrush(): dc %x, brush style %x, brush color %x\n", physDev, pLogBrush->lbStyle, pLogBrush->lbColor);

    /* Free previous brush */
    if (pDC->pFillBrush)
    {
        GreFreeBrush(pDC->pFillBrush);
        pDC->pFillBrush = NULL;
    }

    /* Create the brush */
    switch(pLogBrush->lbStyle)
    {
    case BS_NULL:
        DPRINT("BS_NULL\n" );
        pDC->pFillBrush = GreCreateNullBrush();
        break;

    case BS_SOLID:
        DPRINT("BS_SOLID\n" );
        pDC->pFillBrush = GreCreateSolidBrush(pDC->pBitmap->hDIBPalette, pLogBrush->lbColor);
        break;

    case BS_HATCHED:
        DPRINT("BS_HATCHED\n" );
        pDC->pFillBrush = GreCreateHatchedBrush(pLogBrush->lbHatch, pLogBrush->lbColor);
        break;

    case BS_PATTERN:
        DPRINT("BS_PATTERN\n");
        hBmpKern = GDI_MapUserHandle((HBITMAP)pLogBrush->lbHatch);
        if (!hBmpKern)
        {
            DPRINT1("Trying to select an unknown bitmap %x to the DC %x!\n", pLogBrush->lbHatch, physDev);
            break;
        }
        GDIOBJ_SetOwnership(hBmpKern, NULL);
        pDC->pFillBrush = GreCreatePatternBrush(hBmpKern);
        break;

    case BS_DIBPATTERN:
        UNIMPLEMENTED;
        break;
    }

    /* Create XLATE for hatched/pattern brushes */
    if (pLogBrush->lbStyle == BS_HATCHED || pLogBrush->lbStyle == BS_PATTERN)
    {
        GrepBrushCreateXlate(pDC, pDC->pFillBrush, pLogBrush->lbColor);
    }

    /* Release the object */
    DC_Unlock(pDC);
}

HFONT APIENTRY RosGdiSelectFont( HDC physDev, HFONT hfont, HANDLE gdiFont )
{
    UNIMPLEMENTED;
    return 0;
}

VOID APIENTRY RosGdiSelectPen( HDC physDev, LOGPEN *pLogPen, EXTLOGPEN *pExtLogPen )
{
    PDC pDC;

    /* Check parameters */
    if (!pLogPen && !pExtLogPen) return;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    DPRINT("RosGdiSelectPen(): dc %x, pen style %x, pen color %x\n", physDev, pLogPen->lopnStyle, pLogPen->lopnColor);

    /* Free previous brush */
    if (pDC->pLineBrush) GreFreeBrush(pDC->pLineBrush);

    /* Create the pen */
    if (pLogPen)
    {
        pDC->pLineBrush =
            GreCreatePen(pLogPen->lopnStyle,
                         pLogPen->lopnWidth.x,
                         BS_SOLID,
                         pLogPen->lopnColor,
                         0,
                         0,
                         0,
                         NULL,
                         0,
                         TRUE);

        /* Create XLATE if necessary */
        if (pDC->pLineBrush && pDC->pLineBrush->flAttrs & GDIBRUSH_IS_BITMAP)
        {
            GrepBrushCreateXlate(pDC, pDC->pLineBrush, pLogPen->lopnColor);
        }
    }
    else
    {
        /* Extended pen information */
        pDC->pLineBrush =
            GreCreatePen(pExtLogPen->elpPenStyle,
                         pExtLogPen->elpWidth,
                         pExtLogPen->elpBrushStyle,
                         pExtLogPen->elpColor,
                         0,
                         pExtLogPen->elpHatch,
                         pExtLogPen->elpNumEntries,
                         pExtLogPen->elpStyleEntry,
                         0,
                         FALSE);

        /* Create XLATE if necessary */
        if (pDC->pLineBrush && pDC->pLineBrush->flAttrs & GDIBRUSH_IS_BITMAP)
        {
            GrepBrushCreateXlate(pDC, pDC->pLineBrush, pExtLogPen->elpColor);
        }
    }

    /* Release the object */
    DC_Unlock(pDC);
}

COLORREF APIENTRY RosGdiSetBkColor( HDC physDev, COLORREF color )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Set the color */
    pDC->crBackgroundClr = color;

    /* Release the object */
    DC_Unlock(pDC);

    /* Return the color set */
    return color;
}

COLORREF APIENTRY RosGdiSetDCBrushColor( HDC physDev, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

DWORD APIENTRY RosGdiSetDCOrg( HDC physDev, INT x, INT y )
{
    UNIMPLEMENTED;
    return 0;
}

VOID APIENTRY RosGdiSetBrushOrg( HDC physDev, INT x, INT y )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Set brush origin */
    pDC->ptBrushOrg.x = x;
    pDC->ptBrushOrg.y = y;

    /* Release the object */
    DC_Unlock(pDC);
}

COLORREF APIENTRY RosGdiSetDCPenColor( HDC physDev, COLORREF crColor )
{
    UNIMPLEMENTED;
    return 0;
}

VOID APIENTRY RosGdiUpdateClipping(PDC pDC)
{
    struct region *inter;
    if (!pDC->pWindow)
    {
        /* Easy case, just copy the existing clipping region */
        if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
        pDC->CombinedClip = IntEngCreateClipRegionFromRegion(pDC->Clipping);
    }
    else
    {
        /* Intersect with window's visibility */
        inter = create_empty_region();
        copy_region(inter, pDC->Clipping);

        /* Acquire SWM lock */
        SwmAcquire();

        /* Intersect current clipping region and window's visible region */
        intersect_region(inter, inter, pDC->pWindow->Visible);

        /* Release SWM lock */
        SwmRelease();

        if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
        pDC->CombinedClip = IntEngCreateClipRegionFromRegion(inter);
        free_region(inter);
    }
}

void APIENTRY RosGdiSetDeviceClipping( HDC physDev, UINT count, PRECTL pRects, PRECTL rcBounds )
{
    PDC pDC;
    RECTL pStackBuf[8];
    RECTL *pSafeRects = pStackBuf;
    RECTL rcSafeBounds, rcSurface;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Capture the rects buffer */
    _SEH2_TRY
    {
        if (count > 0)
        {
            ProbeForRead(pRects, count * sizeof(RECTL), 1);

            /* Use pool allocated buffer if data doesn't fit */
            if (count > sizeof(pStackBuf) / sizeof(RECTL))
                pSafeRects = ExAllocatePool(PagedPool, sizeof(RECTL) * count);

            /* Copy points data */
            RtlCopyMemory(pSafeRects, pRects, count * sizeof(RECTL));
        }

        /* Copy bounding rect */
        ProbeForRead(rcBounds, sizeof(RECTL), 1);
        rcSafeBounds = *rcBounds;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        /* Release the object */
        DC_Unlock(pDC);

        /* Free the buffer if it was allocated */
        if (pSafeRects != pStackBuf) ExFreePool(pSafeRects);

        /* Return failure */
        return;
    }

    /* Offset all rects */
    for (i=0; i<count; i++)
    {
        RECTL_vOffsetRect(&pSafeRects[i],
                          pDC->rcDcRect.left + pDC->rcVport.left,
                          pDC->rcDcRect.top + pDC->rcVport.top);
    }

    /* Offset bounding rect */
    RECTL_vOffsetRect(&rcSafeBounds,
                      pDC->rcDcRect.left + pDC->rcVport.left,
                      pDC->rcDcRect.top + pDC->rcVport.top);

    /* Delete old clipping region */
    if (pDC->Clipping)
        free_region(pDC->Clipping);

    if (count == 0)
    {
        /* Set unclipped mode (clip by dc rect) */
        RECTL_vSetRect(&rcSafeBounds,
                       pDC->rcDcRect.left,
                       pDC->rcDcRect.top,
                       pDC->rcDcRect.right,
                       pDC->rcDcRect.bottom);

        RECTL_vOffsetRect(&rcSafeBounds, pDC->rcVport.left, pDC->rcVport.top);

        /* Intersect it with an underlying surface rectangle */
        RECTL_vSetRect(&rcSurface,
                       0,
                       0,
                       pDC->pBitmap->SurfObj.sizlBitmap.cx,
                       pDC->pBitmap->SurfObj.sizlBitmap.cy);

        RECTL_bIntersectRect(&rcSafeBounds, &rcSafeBounds, &rcSurface);

        /* Set the clipping object */
        pDC->Clipping = create_region_from_rects(&rcSafeBounds, 1);
    }
    else
    {
        /* Set the clipping object */
        pDC->Clipping = create_region_from_rects(pSafeRects, count);
    }

    DPRINT("RosGdiSetDeviceClipping() for DC %x, bounding rect (%d,%d)-(%d, %d)\n",
        physDev, rcSafeBounds.left, rcSafeBounds.top, rcSafeBounds.right, rcSafeBounds.bottom);

    DPRINT("rects: %d\n", count);
    for (i=0; i<count; i++)
    {
        DPRINT("%d: (%d,%d)-(%d, %d)\n", i, pSafeRects[i].left, pSafeRects[i].top, pSafeRects[i].right, pSafeRects[i].bottom);
    }

    /* Update the combined clipping */
    RosGdiUpdateClipping(pDC);

    /* Release the object */
    DC_Unlock(pDC);

    /* Free the buffer if it was allocated */
    if (pSafeRects != pStackBuf) ExFreePool(pSafeRects);
}

BOOL APIENTRY RosGdiSetDeviceGammaRamp(HDC physDev, LPVOID ramp)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiSetPixelFormat(HDC physDev,
                                   int iPixelFormat,
                                   const PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF APIENTRY RosGdiSetTextColor( HDC physDev, COLORREF color )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Set the color */
    pDC->crForegroundClr = color;

    /* Release the object */
    DC_Unlock(pDC);

    /* Return the color set */
    return color;
}

VOID APIENTRY RosGdiSetDcRects( HDC physDev, RECT *rcDcRect, RECT *rcVport )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    _SEH2_TRY
    {
        /* Set DC rectangle */
        if (rcDcRect)
            pDC->rcDcRect = *rcDcRect;

        /* Set viewport rectangle */
        if (rcVport)
            pDC->rcVport = *rcVport;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Release the object */
    DC_Unlock(pDC);
}

VOID APIENTRY RosGdiGetDcRects( HDC physDev, RECT *rcDcRect, RECT *rcVport )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    _SEH2_TRY
    {
        /* Set DC rectangle */
        if (rcDcRect)
            *rcDcRect = pDC->rcDcRect;

        /* Set viewport rectangle */
        if (rcVport)
            *rcVport = pDC->rcVport;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Release the object */
    DC_Unlock(pDC);
}

VOID APIENTRY RosGdiGetDC(HDC physDev, HWND hwnd, BOOL clipChildren)
{
    PDC pDC;

    /* Acquire SWM lock before locking the DC */
    SwmAcquire();

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Get a pointer to this window */
    pDC->pWindow = SwmFindByHwnd(hwnd);

    /* Release the object */
    DC_Unlock(pDC);

    /* Release SWM lock */
    SwmRelease();
}

VOID APIENTRY RosGdiReleaseDC(HDC physDev)
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* No window clipping is to be performed */
    pDC->pWindow = NULL;

    /* Release the object */
    DC_Unlock(pDC);
}

/* EOF */
