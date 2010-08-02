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

/* GLOBALS *******************************************************************/
HGDIOBJ hStockBmp;
HGDIOBJ hStockPalette;
HGDIOBJ hNullPen;

/* PUBLIC FUNCTIONS **********************************************************/

BOOL INTERNAL_CALL
DC_Cleanup(PVOID ObjectBody)
{
    PDC pDC = (PDC)ObjectBody;

    /* Release the surface */
    SURFACE_ShareUnlockSurface(pDC->dclevel.pSurface);

    /* Dereference default brushes */
    BRUSH_ShareUnlockBrush(pDC->eboFill.pbrush);
    BRUSH_ShareUnlockBrush(pDC->eboLine.pbrush);

    GreDeleteObject(pDC->eboFill.pbrush->BaseObject.hHmgr);
    GreDeleteObject(pDC->eboLine.pbrush->BaseObject.hHmgr);

    /* Cleanup the dc brushes */
    EBRUSHOBJ_vCleanup(&pDC->eboFill);
    EBRUSHOBJ_vCleanup(&pDC->eboLine);

    return TRUE;
}

BOOL APIENTRY RosGdiCreateDC( PROS_DCINFO dc, HDC *pdev, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData )
{
    HGDIOBJ hNewDC;
    PDC pNewDC;

    /* TESTING: Create primary surface */
    if (!PrimarySurface.pSurface && !IntCreatePrimarySurface())
        return STATUS_UNSUCCESSFUL;

    /* Allocate storage for DC structure */
    pNewDC = (PDC)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DC);
    if (!pNewDC) return FALSE;

    /* Save a handle to it */
    hNewDC = pNewDC->BaseObject.hHmgr;

    /* Set physical device pointer */
    pNewDC->ppdev = (PVOID)&PrimarySurface;

    /* Set default fg/bg colors */
    pNewDC->crBackgroundClr = RGB(255, 255, 255);
    pNewDC->crForegroundClr = RGB(0, 0, 0);

    /* Check if it's a compatible DC */
    if (*pdev)
    {
        DPRINT("Creating a compatible with %x DC!\n", *pdev);
    }

    /* Create default NULL brushes */
    pNewDC->dclevel.pbrLine = BRUSH_ShareLockBrush(GreCreateNullBrush());
    EBRUSHOBJ_vInit(&pNewDC->eboLine, pNewDC->dclevel.pbrLine, pNewDC);

    pNewDC->dclevel.pbrFill = BRUSH_ShareLockBrush(GreCreateNullBrush());
    EBRUSHOBJ_vInit(&pNewDC->eboFill, pNewDC->dclevel.pbrFill, pNewDC);

    /* Set the default palette */
    pNewDC->dclevel.hpal = hStockPalette;
    pNewDC->dclevel.ppal = PALETTE_ShareLockPalette(pNewDC->dclevel.hpal);

    if (dc->dwType == OBJ_MEMDC)
    {
        DPRINT("Creating a memory DC %x\n", hNewDC);
        pNewDC->dclevel.pSurface = SURFACE_ShareLockSurface(hStockBmp);

        /* Set DC rectangles */
        pNewDC->rcDcRect.left = 0; pNewDC->rcDcRect.top = 0;
        pNewDC->rcDcRect.right = 1; pNewDC->rcDcRect.bottom = 1;
        pNewDC->rcVport = pNewDC->rcDcRect;
    }
    else
    {
        DPRINT("Creating a display DC %x\n", hNewDC);
        pNewDC->dclevel.pSurface = SURFACE_ShareLockSurface(PrimarySurface.pSurface);

        /* Set DC rectangles */
        pNewDC->rcVport.left = 0;
        pNewDC->rcVport.top = 0;
        pNewDC->rcVport.right = PrimarySurface.gdiinfo.ulHorzRes;
        pNewDC->rcVport.bottom = PrimarySurface.gdiinfo.ulVertRes;

        pNewDC->rcDcRect = pNewDC->rcVport;
    }

    /* Create an empty combined clipping region */
    pNewDC->CombinedClip = EngCreateClip();
    pNewDC->Clipping = create_empty_region();
    pNewDC->pWindow = NULL;

    /* Set default palette */
    pNewDC->dclevel.hpal = hStockPalette;

    /* Give handle to the caller */
    *pdev = hNewDC;

    /* Unlock the DC */
    DC_UnlockDc(pNewDC);

    /* Indicate success */
    return TRUE;
}

BOOL APIENTRY RosGdiDeleteDC( HDC physDev )
{
    DPRINT("RosGdiDeleteDC(%x)\n", physDev);

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
    pDC = DC_LockDc(physDev);
    pSurface = SURFACE_ShareLockSurface(hBmpKern);

    /* Release the old bitmap */
    SURFACE_ShareUnlockSurface(pDC->dclevel.pSurface);

    /* Select it */
    pDC->dclevel.pSurface = pSurface;

    /* Set DC rectangles */
    pDC->rcVport.left = 0;
    pDC->rcVport.top = 0;
    pDC->rcVport.right = pSurface->SurfObj.sizlBitmap.cx;
    pDC->rcVport.bottom = pSurface->SurfObj.sizlBitmap.cy;
    pDC->rcDcRect = pDC->rcVport;

    /* Release the DC object */
    DC_UnlockDc(pDC);

    return TRUE;
}

HBRUSH APIENTRY GreCreateBrush(LOGBRUSH *pLogBrush)
{
    HGDIOBJ hBmpKern;

    /* Create the brush */
    switch(pLogBrush->lbStyle)
    {
    case BS_NULL:
        return GreCreateNullBrush();
    case BS_SOLID:
        return GreCreateSolidBrush(pLogBrush->lbColor);
    case BS_HATCHED:
        return GreCreateHatchBrush(pLogBrush->lbHatch, pLogBrush->lbColor);
    case BS_PATTERN:
        hBmpKern = GDI_MapUserHandle((HBITMAP)pLogBrush->lbHatch);
        if (!hBmpKern)
        {
            DPRINT1("Trying to create a pattern brush with an unknown bitmap %x !\n", pLogBrush->lbHatch);
            return NULL;
        }
        GDIOBJ_SetOwnership(hBmpKern, NULL);
        return GreCreatePatternBrush(hBmpKern);
    case BS_DIBPATTERN:
    default:
        UNIMPLEMENTED;
        return NULL;
    }
}

HBRUSH APIENTRY GreSelectBrush( HDC hdc, HBRUSH hbrush )
{
    //PDC_ATTR pdcattr = pdc->pdcattr;
    PDC pdc;
    PBRUSH pbrFill;
    HBRUSH hbrushOld;

    /* Lock the dc */
    pdc = DC_LockDc(hdc);

    hbrushOld = pdc->dclevel.pbrFill->BaseObject.hHmgr;

    /* Check if the brush handle has changed */
    if (hbrush != hbrushOld)
    {
        /* Try to lock the new brush */
        pbrFill = BRUSH_ShareLockBrush(hbrush);
        if (pbrFill)
        {
            /* Unlock old brush, set new brush */
            BRUSH_ShareUnlockBrush(pdc->dclevel.pbrFill);
            pdc->dclevel.pbrFill = pbrFill;

            /* Update eboFill */
            EBRUSHOBJ_vUpdate(&pdc->eboFill, pdc->dclevel.pbrFill, pdc);
        }
    }

#if 0
    /* Check for DC brush */
    if (pdcattr->hbrush == StockObjects[DC_BRUSH])
    {
        /* ROS HACK, should use surf xlate */
        /* Update the eboFill's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboFill, pdcattr->crPenClr);
    }
#endif

    /* Release the dc */
    DC_UnlockDc(pdc);

    return hbrushOld;
}

VOID APIENTRY RosGdiSelectBrush( HDC physDev, LOGBRUSH *pLogBrush )
{
    HBRUSH hbrNew,hbrOld;
    
    hbrNew = GreCreateBrush(pLogBrush);

    if(hbrNew == NULL)
        return;

    hbrOld = GreSelectBrush(physDev, hbrNew);

    GreDeleteObject(hbrOld);
}

HFONT APIENTRY RosGdiSelectFont( HDC physDev, HFONT hfont, HANDLE gdiFont )
{
    UNIMPLEMENTED;
    return 0;
}

HPEN GreSelectPen( HDC hdc, HPEN hpen)
{
    PDC pdc;
    //PDC_ATTR pdcattr = pdc->pdcattr;
    PBRUSH pbrLine;
    HPEN hpenOld;

    /* Lock the dc */
    pdc = DC_LockDc(hdc);

    hpenOld = pdc->dclevel.pbrLine->BaseObject.hHmgr;

    /* Check if the pen handle has changed */
    if (hpen != hpenOld)
    {
        /* Try to lock the new pen */
        pbrLine = PEN_ShareLockPen(hpen);
        if (pbrLine)
        {
            /* Unlock old brush, set new brush */
            BRUSH_ShareUnlockBrush(pdc->dclevel.pbrLine);
            pdc->dclevel.pbrLine = pbrLine;

            /* Update eboLine */
            EBRUSHOBJ_vUpdate(&pdc->eboLine, pdc->dclevel.pbrLine, pdc);
        }
    }

#if 0
    /* Check for DC pen */
    if (pdcattr->hpen == StockObjects[DC_PEN])
    {
        /* Update the eboLine's solid color */
        EBRUSHOBJ_vSetSolidBrushColor(&pdc->eboLine, pdcattr->crPenClr);
    }
#endif

    /* Release the dc */
    DC_UnlockDc(pdc);

    return hpenOld;
}

VOID APIENTRY RosGdiSelectPen( HDC physDev, LOGPEN *pLogPen, EXTLOGPEN *pExtLogPen )
{
    HPEN new_pen, old_pen;

    if(pExtLogPen)
    {
        new_pen = GreExtCreatePen(pExtLogPen->elpPenStyle, 
                                  pExtLogPen->elpWidth,
                                  pExtLogPen->elpPenStyle, 
                                  pExtLogPen->elpColor,
                                  pExtLogPen->elpHatch,
                                  pExtLogPen->elpHatch,
                                  pExtLogPen->elpNumEntries,
                                  pExtLogPen->elpStyleEntry,
                                  0,
                                  FALSE,
                                  NULL);
        new_pen = NULL;
    }
    else
    {       
        new_pen = GreCreatePen(pLogPen->lopnStyle, 
                               pLogPen->lopnWidth.x, 
                               pLogPen->lopnColor, 
                               NULL);
    }

    if(new_pen == NULL)
        return;

    old_pen = GreSelectPen(physDev, new_pen);

    GreDeleteObject(old_pen);
}

COLORREF APIENTRY RosGdiSetBkColor( HDC physDev, COLORREF color )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

    /* Set the color */
    pDC->crBackgroundClr = color;

    /* Release the object */
    DC_UnlockDc(pDC);

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
    pDC = DC_LockDc(physDev);

    /* Set brush origin */
    pDC->dclevel.ptlBrushOrigin.x = x;
    pDC->dclevel.ptlBrushOrigin.y = y;

    /* Release the object */
    DC_UnlockDc(pDC);
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
    pDC = DC_LockDc(physDev);

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
        DC_UnlockDc(pDC);

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
                       pDC->dclevel.pSurface->SurfObj.sizlBitmap.cx,
                       pDC->dclevel.pSurface->SurfObj.sizlBitmap.cy);

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
    DC_UnlockDc(pDC);

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
    pDC = DC_LockDc(physDev);

    /* Set the color */
    pDC->crForegroundClr = color;

    /* Release the object */
    DC_UnlockDc(pDC);

    /* Return the color set */
    return color;
}

VOID APIENTRY RosGdiSetDcRects( HDC physDev, RECT *rcDcRect, RECT *rcVport )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

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
    DC_UnlockDc(pDC);
}

VOID APIENTRY RosGdiGetDC(HDC physDev, HWND hwnd, BOOL clipChildren)
{
    PDC pDC;

    /* Acquire SWM lock before locking the DC */
    SwmAcquire();

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

    /* Get a pointer to this window */
    pDC->pWindow = SwmFindByHwnd(hwnd);

    /* Release the object */
    DC_UnlockDc(pDC);

    /* Release SWM lock */
    SwmRelease();
}

VOID APIENTRY RosGdiReleaseDC(HDC physDev)
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

    /* No window clipping is to be performed */
    pDC->pWindow = NULL;

    /* Release the object */
    DC_UnlockDc(pDC);
}


static
HPEN
FASTCALL
IntCreateStockPen(DWORD dwPenStyle,
                  DWORD dwWidth,
                  ULONG ulBrushStyle,
                  ULONG ulColor)
{
    HPEN hPen;
    PBRUSH pbrushPen = PEN_AllocPenWithHandle();

    if ((dwPenStyle & PS_STYLE_MASK) == PS_NULL) dwWidth = 1;

    pbrushPen->ptPenWidth.x = abs(dwWidth);
    pbrushPen->ptPenWidth.y = 0;
    pbrushPen->ulPenStyle = dwPenStyle;
    pbrushPen->BrushAttr.lbColor = ulColor;
    pbrushPen->ulStyle = ulBrushStyle;
    pbrushPen->hbmClient = (HANDLE)NULL;
    pbrushPen->dwStyleCount = 0;
    pbrushPen->pStyle = 0;
    pbrushPen->flAttrs = GDIBRUSH_IS_OLDSTYLEPEN;

    switch (dwPenStyle & PS_STYLE_MASK)
    {
        case PS_NULL:
            pbrushPen->flAttrs |= GDIBRUSH_IS_NULL;
            break;

        case PS_SOLID:
            pbrushPen->flAttrs |= GDIBRUSH_IS_SOLID;
            break;
    }
    hPen = pbrushPen->BaseObject.hHmgr;
    PEN_UnlockPen(pbrushPen);
    return hPen;
}

VOID CreateStockObjects()
{
    hStockBmp = IntGdiCreateBitmap(1, 1, 1, 1, NULL);
    hStockPalette = (HGDIOBJ)PALETTE_Init();
    hNullPen = IntCreateStockPen(PS_NULL, 0, BS_SOLID, 0);

    GDIOBJ_ConvertToStockObj(&hStockBmp);
    GDIOBJ_ConvertToStockObj(&hStockPalette);
    GDIOBJ_ConvertToStockObj(&hNullPen);
}
/* EOF */
