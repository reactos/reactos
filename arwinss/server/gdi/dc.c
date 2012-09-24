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
HGDIOBJ StockObjects[NB_STOCK_OBJECTS];

static LOGPEN WhitePen =
    { PS_SOLID, { 0, 0 }, RGB(255,255,255) };

static LOGPEN BlackPen =
    { PS_SOLID, { 0, 0 }, RGB(0,0,0) };

static LOGPEN NullPen =
    { PS_NULL, { 0, 0 }, 0 };

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

    /* Delete clipping regions */
    if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
    if (pDC->Clipping) free_region(pDC->Clipping);

    return TRUE;
}

VOID
FASTCALL
DC_AllocateDcAttr(HDC hDC)
{
  PVOID NewMem = NULL;
  PDC pDC;
  HANDLE Pid = NtCurrentProcess();
  ULONG MemSize = sizeof(DC_ATTR); //PAGE_SIZE it will allocate that size

  NTSTATUS Status = ZwAllocateVirtualMemory(Pid,
                                        &NewMem,
                                              0,
                                       &MemSize,
                         MEM_COMMIT|MEM_RESERVE,
                                 PAGE_READWRITE);
  {
    INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hDC);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    // FIXME: dc could have been deleted!!! use GDIOBJ_InsertUserData
    if (NT_SUCCESS(Status))
    {
      RtlZeroMemory(NewMem, MemSize);
      Entry->UserData  = NewMem;
      DPRINT("DC_ATTR allocated! 0x%x\n",NewMem);
    }
    else
    {
       DPRINT("DC_ATTR not allocated!\n");
    }
  }
  pDC = DC_LockDc(hDC);

  if(NewMem)
  {
     pDC->pdcattr = NewMem; // Store pointer
  }
  DC_UnlockDc(pDC);
}

VOID
FASTCALL
DC_FreeDcAttr(HDC DCToFree)
{
    NTSTATUS Status;
    SIZE_T Size = 0;

    PDC pDC = DC_LockDc(DCToFree);
    if (pDC->pdcattr == NULL) return; // Internal DC object!
    pDC->pdcattr = NULL;
    DC_UnlockDc(pDC);

    {
        INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)DCToFree);
        PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
        if (Entry->UserData)
        {
            Status = ZwFreeVirtualMemory(NtCurrentProcess(), &Entry->UserData, &Size, MEM_RELEASE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed freeing dcattr memory with Status 0x%08X\n", Status);
            }
            Entry->UserData = NULL;
        }
    }
}

BOOL APIENTRY RosGdiCreateDC( HDC *pdev, LPCWSTR driver, LPCWSTR device,
                            LPCWSTR output, const DEVMODEW* initData, ULONG dcType )
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

    /* Allocate dc shared memory */
    DC_AllocateDcAttr(hNewDC);

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
    pNewDC->dclevel.hpal = StockObjects[DEFAULT_PALETTE];
    pNewDC->dclevel.ppal = PALETTE_ShareLockPalette(pNewDC->dclevel.hpal);

    pNewDC->type = dcType;

    if (dcType == OBJ_MEMDC)
    {
        DPRINT("Creating a memory DC %x\n", hNewDC);
        pNewDC->dclevel.pSurface = SURFACE_ShareLockSurface(StockObjects[DEFAULT_BITMAP]);

        /* Set DC origin */
        pNewDC->ptlDCOrig.x = 0; pNewDC->ptlDCOrig.y = 0;

        pNewDC->pWindow = NULL;
    }
    else
    {
        DPRINT("Creating a display DC %x\n", hNewDC);
        pNewDC->dclevel.pSurface = SURFACE_ShareLockSurface(PrimarySurface.pSurface);

        /* Set DC origin */
        pNewDC->ptlDCOrig.x = 0;
        pNewDC->ptlDCOrig.y = 0;

        pNewDC->pWindow = &SwmRoot;
    }

    /* Create an empty combined clipping region */
    pNewDC->CombinedClip = NULL;
    pNewDC->Clipping = NULL;
    pNewDC->ClipChildren = FALSE;

    /* Set default palette */
    pNewDC->dclevel.hpal = StockObjects[DEFAULT_PALETTE];

    /* Calculate new combined clipping region */
    RosGdiUpdateClipping(pNewDC, FALSE);

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

    /* Free DC attr */
    DC_FreeDcAttr(physDev);

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

BOOL APIENTRY RosGdiSelectBitmap( HDC physDev, HBITMAP hbitmap )
{
    PDC pDC;
    PSURFACE pSurface;

    DPRINT("Selecting %x bitmap to hdc %x\n", hbitmap, physDev);
    
    pSurface = SURFACE_ShareLockSurface(hbitmap);
    if(pSurface== NULL)
    {
        DPRINT1("SURFACE_ShareLockSurface failed\n");
        return FALSE;
    }

    /* Get a pointer to the DC and the bitmap*/
    pDC = DC_LockDc(physDev);

    /* Release the old bitmap */
    SURFACE_ShareUnlockSurface(pDC->dclevel.pSurface);

    /* Select it */
    pDC->dclevel.pSurface = pSurface;

    /* Set DC origin */
    pDC->ptlDCOrig.x = 0;
    pDC->ptlDCOrig.y = 0;

    /* Update clipping to reflect changes in the surface */
    RosGdiUpdateClipping(pDC, FALSE);

    /* Release the DC object */
    DC_UnlockDc(pDC);

    return TRUE;
}

HBRUSH APIENTRY GreCreateBrush(LOGBRUSH *pLogBrush)
{
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
        GDIOBJ_SetOwnership((HBITMAP)pLogBrush->lbHatch, NULL);
        return GreCreatePatternBrush((HBITMAP)pLogBrush->lbHatch);
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

VOID APIENTRY RosGdiUpdateClipping(PDC pDC, BOOLEAN IgnoreVisibility)
{
    struct region *window, *surface;
    rectangle_t surfrect = {0,0,0,0};
    RECTL EmptyRect = {0,0,0,0};

    surface = create_empty_region();
    surfrect.right = pDC->dclevel.pSurface->SurfObj.sizlBitmap.cx;
    surfrect.bottom = pDC->dclevel.pSurface->SurfObj.sizlBitmap.cy;
    set_region_rect(surface, &surfrect);

    if (pDC->type == OBJ_MEMDC)
    {
        /* underlying surface rect X user clipping (if any)  */
        if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);

        if (pDC->Clipping)
            intersect_region(surface, surface, pDC->Clipping);

        pDC->CombinedClip = IntEngCreateClipRegionFromRegion(surface);
    }
    else
    {
        if (!pDC->pWindow)
        {
            /* Drawing is forbidden */
            if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
            pDC->CombinedClip = IntEngCreateClipRegion(1, &EmptyRect, &EmptyRect);
            return;
        }

        /* Root window's visibility may be ignored */
        if ((pDC->pWindow == &SwmRoot) && !pDC->ClipChildren)
        {
            IgnoreVisibility = TRUE;
        }

        /* window visibility X user clipping (if any) X underlying surface */

        /* Acquire SWM lock */
        SwmAcquire();

        //window = SwmSetClipWindow(pDC->pWindow, NULL, pDC->ExcludeChildren);
        window = pDC->pWindow->Visible;

        if (window)
        {
            /* Intersect window's visible region X underlying surface */
            if (!IgnoreVisibility)
                intersect_region(surface, surface, window);

            /* Intersect result X user clipping (if any) */
            if (pDC->Clipping)
                intersect_region(surface, surface, pDC->Clipping);

            /* Release SWM lock */
            SwmRelease();

            if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
            pDC->CombinedClip = IntEngCreateClipRegionFromRegion(surface);
        }
        else
        {
            /* Release SWM lock */
            SwmRelease();

            /* Drawing is forbidden */
            if (pDC->CombinedClip) EngDeleteClip(pDC->CombinedClip);
            pDC->CombinedClip = IntEngCreateClipRegion(1, &EmptyRect, &EmptyRect);
        }
    }

    free_region(surface);
}

void APIENTRY RosGdiSetDeviceClipping( HDC physDev, UINT count, PRECTL pRects, PRECTL rcBounds )
{
    PDC pDC;
    RECTL pStackBuf[8];
    RECTL *pSafeRects = pStackBuf;
    RECTL rcSafeBounds = {0};
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
                          pDC->ptlDCOrig.x,
                          pDC->ptlDCOrig.y);
    }

    /* Offset bounding rect */
    RECTL_vOffsetRect(&rcSafeBounds,
                      pDC->ptlDCOrig.x,
                      pDC->ptlDCOrig.y);

    /* Delete old clipping region */
    if (pDC->Clipping)
        free_region(pDC->Clipping);

    pDC->Clipping = NULL;

    if (count)
    {
        /* Set the clipping object */
        pDC->Clipping = create_region_from_rects(pSafeRects, count);
    }
    else
    {
        pDC->Clipping = create_empty_region();
    }

    DPRINT("RosGdiSetDeviceClipping() for DC %x, bounding rect (%d,%d)-(%d, %d)\n",
        physDev, rcSafeBounds.left, rcSafeBounds.top, rcSafeBounds.right, rcSafeBounds.bottom);

    DPRINT("rects: %d\n", count);
    for (i=0; i<count; i++)
    {
        DPRINT("%d: (%d,%d)-(%d, %d)\n", i, pSafeRects[i].left, pSafeRects[i].top, pSafeRects[i].right, pSafeRects[i].bottom);
    }

    /* Update the combined clipping */
    RosGdiUpdateClipping(pDC, FALSE);

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

VOID APIENTRY RosGdiSetWindow(HDC physDev, SWM_WINDOW_ID Wid, BOOL clipChildren, POINTL ptOrigin)
{
    PDC pDC;

    /* Acquire SWM lock before locking the DC */
    SwmAcquire();

    /* Get a pointer to the DC */
    pDC = DC_LockDc(physDev);

    /* Get a pointer to this window */
    if (Wid)
    {
        pDC->pWindow = SwmGetWindowById(Wid);
        //PDC->ptlDCOrig.x = pDC->pWindow->Window.left;
        //PDC->ptlDCOrig.y = pDC->pWindow->Window.top
        pDC->ptlDCOrig = ptOrigin;

        DPRINT("hdc %x set window hwnd %x\n", physDev, pDC->pWindow->hwnd);
    }
    else
    {
        /* Handle situation when drawing is forbidden */
        pDC->pWindow = NULL;
        DPRINT("hdc %x, restricting any drawing\n", physDev);
    }

    pDC->ClipChildren = clipChildren;

    RosGdiUpdateClipping(pDC, FALSE);

    /* Release the object */
    DC_UnlockDc(pDC);

    /* Release SWM lock */
    SwmRelease();
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
    UINT Object;

    memset(StockObjects, 0, sizeof(StockObjects));

    /* Create GDI Stock Objects from the logical structures we've defined */

    StockObjects[DEFAULT_BITMAP] = IntGdiCreateBitmap(1, 1, 1, 1, NULL);
    StockObjects[DEFAULT_PALETTE] = (HGDIOBJ)PALETTE_Init();
    StockObjects[WHITE_PEN] = IntCreateStockPen(WhitePen.lopnStyle, WhitePen.lopnWidth.x, BS_SOLID, WhitePen.lopnColor);
    StockObjects[BLACK_PEN] = IntCreateStockPen(BlackPen.lopnStyle, BlackPen.lopnWidth.x, BS_SOLID, BlackPen.lopnColor);
    StockObjects[DC_PEN]    = IntCreateStockPen(BlackPen.lopnStyle, BlackPen.lopnWidth.x, BS_SOLID, BlackPen.lopnColor);
    StockObjects[NULL_PEN]  = IntCreateStockPen(NullPen.lopnStyle, NullPen.lopnWidth.x, BS_SOLID, NullPen.lopnColor);

    for (Object = 0; Object < NB_STOCK_OBJECTS; Object++)
    {
        if (NULL != StockObjects[Object])
        {
            GDIOBJ_ConvertToStockObj(&StockObjects[Object]);
        }
    }
}

/*!
 * Return stock object.
 * \param	Object - stock object id.
 * \return	Handle to the object.
*/
HGDIOBJ APIENTRY
NtGdiGetStockObject(INT Object)
{
    DPRINT("NtGdiGetStockObject index %d\n", Object);

    return ((Object < 0) || (NB_STOCK_OBJECTS <= Object)) ? NULL : StockObjects[Object];
}

/* EOF */
