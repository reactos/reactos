/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Clip region functions
 * FILE:             win32ss/gdi/ntgdi/cliprgn.c
 * PROGRAMER:        Unknown
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(GdiClipRgn);

VOID
FASTCALL
IntGdiReleaseRaoRgn(PDC pDC)
{
    INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    pDC->fs |= DC_DIRTY_RAO;
    Entry->Flags |= GDI_ENTRY_VALIDATE_VIS; // Need to validate Vis.
}

VOID
FASTCALL
IntGdiReleaseVisRgn(PDC pDC)
{
    IntGdiReleaseRaoRgn(pDC);
    REGION_Delete(pDC->prgnVis);
    pDC->prgnVis = prgnDefault; // Vis can not be NULL!!!
}

//
// Updating Vis Region Attribute for DC Attributes.
// BTW: This system region has an user attribute for it.
//
VOID
FASTCALL
UpdateVisRgn(
    PDC pdc)
{
    INT Index = GDI_HANDLE_GET_INDEX(pdc->BaseObject.hHmgr);
    PGDI_TABLE_ENTRY pEntry = &GdiHandleTable->Entries[Index];

    /* Setup Vis Region Attribute information to User side */
    pEntry->Flags |= GDI_ENTRY_VALIDATE_VIS;
    pdc->pdcattr->VisRectRegion.iComplexity = REGION_GetRgnBox(pdc->prgnVis, &pdc->pdcattr->VisRectRegion.Rect);
    pdc->pdcattr->VisRectRegion.AttrFlags = ATTR_RGN_VALID;
    pEntry->Flags &= ~GDI_ENTRY_VALIDATE_VIS;
}

//
//  Selecting Vis Region.
//
VOID
FASTCALL
GdiSelectVisRgn(
    HDC hdc,
    PREGION prgn)
{
    DC *dc;

    if (!(dc = DC_LockDc(hdc)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return;
    }

    if (!prgn)
    {
       DPRINT1("SVR: Setting NULL Region\n");
       IntGdiReleaseVisRgn(dc);
       IntSetDefaultRegion(dc);
       DC_UnlockDc(dc);
       return;
    }

    dc->fs |= DC_DIRTY_RAO;

    ASSERT(dc->prgnVis != NULL);
    ASSERT(prgn != NULL);

    REGION_bCopy(dc->prgnVis, prgn);
    REGION_bOffsetRgn(dc->prgnVis, -dc->ptlDCOrig.x, -dc->ptlDCOrig.y);

    DC_UnlockDc(dc);
}

_Success_(return!=ERROR)
int
FASTCALL
IntSelectClipRgn(
    _In_ PDC dc,
    _In_ PREGION prgn,
    _In_ int fnMode)
{
    int Ret = ERROR;
    PREGION prgnNClip, prgnOrigClip = dc->dclevel.prgnClip;

    //
    // No Coping Regions and no intersecting Regions or an User calling w NULL Region or have the Original Clip Region.
    //
    if (fnMode != RGN_COPY && (fnMode != RGN_AND || !prgn || prgnOrigClip))
    {
        prgnNClip = IntSysCreateRectpRgn(0, 0, 0, 0);

        // Have Original Clip Region.
        if (prgnOrigClip)
        {
           // This will fail on NULL prgn.
           Ret = IntGdiCombineRgn(prgnNClip, prgnOrigClip, prgn, fnMode);

           if (Ret)
           {
              REGION_Delete(prgnOrigClip);
              dc->dclevel.prgnClip = prgnNClip;
              IntGdiReleaseRaoRgn(dc);
           }
           else
              REGION_Delete(prgnNClip);
        }
        else // NULL Original Clip Region, setup a new one and process mode.
        {
            PREGION prgnClip;
            RECTL rcl;
#if 0
            PSURFACE pSurface;

            // See IntSetDefaultRegion.

            rcl.left   = 0;
            rcl.top    = 0;
            rcl.right  = dc->dclevel.sizl.cx;
            rcl.bottom = dc->dclevel.sizl.cy;

            //EngAcquireSemaphoreShared(pdc->ppdev->hsemDevLock);
            if (dc->ppdev->flFlags & PDEV_META_DEVICE)
            {
                pSurface = dc->dclevel.pSurface;
                if (pSurface && pSurface->flags & PDEV_SURFACE)
                {
                   rcl.left   += dc->ppdev->ptlOrigion.x;
                   rcl.top    += dc->ppdev->ptlOrigion.y;
                   rcl.right  += dc->ppdev->ptlOrigion.x;
                   rcl.bottom += dc->ppdev->ptlOrigion.y;
                }
            }
            //EngReleaseSemaphore(pdc->ppdev->hsemDevLock);
//#if 0
            rcl.left   += dc->ptlDCOrig.x;
            rcl.top    += dc->ptlDCOrig.y;
            rcl.right  += dc->ptlDCOrig.x;
            rcl.bottom += dc->ptlDCOrig.y;
#endif
            REGION_GetRgnBox(dc->prgnVis, &rcl);

            prgnClip = IntSysCreateRectpRgnIndirect(&rcl);

            Ret = IntGdiCombineRgn(prgnNClip, prgnClip, prgn, fnMode);

            if (Ret)
            {
                dc->dclevel.prgnClip = prgnNClip;
                IntGdiReleaseRaoRgn(dc);
            }
            else
                REGION_Delete(prgnNClip);

            REGION_Delete(prgnClip);
        }
        return Ret;
    }

    // Fall through to normal RectOS mode.

    //
    // Handle NULL Region and Original Clip Region.
    //
    if (!prgn)
    {
        if (prgnOrigClip)
        {
            REGION_Delete(dc->dclevel.prgnClip);
            dc->dclevel.prgnClip = NULL;
            IntGdiReleaseRaoRgn(dc);
        }
        return SIMPLEREGION;
    }

    //
    // Combine the new Clip region with original Clip and caller Region.
    //
    if ( prgnOrigClip &&
        (Ret = IntGdiCombineRgn(prgnOrigClip, prgn, NULL, RGN_COPY)) ) // Clip could fail.
    {
        IntGdiReleaseRaoRgn(dc);
    }
    else // NULL original Clip, just copy caller region to new.
    {
       prgnNClip = IntSysCreateRectpRgn(0, 0, 0, 0);
       REGION_bCopy(prgnNClip, prgn);
       Ret = REGION_Complexity(prgnNClip);
       dc->dclevel.prgnClip = prgnNClip;
       IntGdiReleaseRaoRgn(dc);
    }
    return Ret;
}

//
// Call from Gdi Batch Subsystem.
//
// Was setup to just handle RGN_COPY only and return VOID, since this was called from Gdi32.
// Tested in place of the other, complexity aside.
//

_Success_(return!=ERROR)
int
FASTCALL
IntGdiExtSelectClipRect(
    _In_ PDC dc,
    _In_ PRECTL prcl,
    _In_ int fnMode)
{
    int Ret = ERROR;
    PREGION prgn;
    RECTL rect;
    BOOL NoRegion = fnMode & GDIBS_NORECT;

    fnMode &= ~GDIBS_NORECT;

    if (NoRegion) // NULL Region.
    {
        if (fnMode == RGN_COPY)
        {
           Ret = IntSelectClipRgn( dc, NULL, RGN_COPY);

           if (dc->fs & DC_DIRTY_RAO)
               CLIPPING_UpdateGCRegion(dc);

           if (Ret) // Copy? Return Vis complexity.
               Ret = REGION_Complexity(dc->prgnVis);
        }
    }
    else // Have a box to build a region with.
    {                             //       See CORE-16246 : Needs to be a one box Clip Region.
        if ( dc->dclevel.prgnClip && (REGION_Complexity(dc->dclevel.prgnClip) == SIMPLEREGION) )
        {
            REGION_GetRgnBox(dc->dclevel.prgnClip, &rect);

            if (prcl->left   == rect.left  &&
                prcl->top    == rect.top   &&
                prcl->right  == rect.right &&
                prcl->bottom == rect.bottom)
            {
                return REGION_Complexity( dc->prgnRao ? dc->prgnRao : dc->prgnVis );
            }
        }

        prgn = IntSysCreateRectpRgnIndirect(prcl);

        Ret = IntSelectClipRgn( dc, prgn, fnMode);

        if (dc->fs & DC_DIRTY_RAO)
            CLIPPING_UpdateGCRegion(dc);

        if (Ret) // In this case NtGdiExtSelectClipRgn tests pass.
            Ret = REGION_Complexity( dc->prgnRao ? dc->prgnRao : dc->prgnVis );

        REGION_Delete(prgn);
    }
    return Ret;
}

_Success_(return!=ERROR)
int
FASTCALL
IntGdiExtSelectClipRgn(
    _In_ PDC dc,
    _In_ PREGION prgn,
    _In_ int fnMode)
{
    int Ret = ERROR;

    if (!prgn)
    {
        if (fnMode == RGN_COPY)
        {
           if ((Ret = IntSelectClipRgn( dc, NULL, RGN_COPY)))
               Ret = REGION_Complexity(dc->prgnVis);
        }
    }
    else
    {
        if ((Ret = IntSelectClipRgn( dc, prgn, fnMode)))
        {
            DPRINT("IntGdiExtSelectClipRgn A %d\n",Ret);
            // Update the Rao, it must be this way for now.
            if (dc->fs & DC_DIRTY_RAO)
                CLIPPING_UpdateGCRegion(dc);

            Ret = REGION_Complexity( dc->prgnRao ? dc->prgnRao : dc->prgnVis );
            DPRINT("IntGdiExtSelectClipRgn B %d\n",Ret);
        }
    }
    return Ret;
}

int
APIENTRY
NtGdiExtSelectClipRgn(
    HDC  hDC,
    HRGN  hrgn,
    int  fnMode)
{
    int retval;
    DC *dc;
    PREGION prgn;

    if ( fnMode < RGN_AND || fnMode > RGN_COPY )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return ERROR;
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    prgn = REGION_LockRgn(hrgn);

    if ((prgn == NULL) && (fnMode != RGN_COPY))
    {
        //EngSetLastError(ERROR_INVALID_HANDLE); doesn't set this.
        retval = ERROR;
    }
    else
    {
#if 0   // Testing GDI Batch.
        {
            RECTL rcl;
            if (prgn)
                REGION_GetRgnBox(prgn, &rcl);
            else
                fnMode |= GDIBS_NORECT;
            retval = IntGdiExtSelectClipRect(dc, &rcl, fnMode);
        }
#else
        retval = IntGdiExtSelectClipRgn(dc, prgn, fnMode);
#endif
    }

    if (prgn)
        REGION_UnlockRgn(prgn);

    DC_UnlockDc(dc);
    return retval;
}

_Success_(return!=ERROR)
INT
FASTCALL
GdiGetClipBox(
    _In_ HDC hdc,
    _Out_ LPRECT prc)
{
    PDC pdc;
    INT iComplexity;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return ERROR;
    }

    /* Update RAO region if necessary */
    if (pdc->fs & DC_DIRTY_RAO)
        CLIPPING_UpdateGCRegion(pdc);

    /* Check if we have a RAO region (intersection of API and VIS region) */
    if (pdc->prgnRao)
    {
        /* We have a RAO region, use it */
        iComplexity = REGION_GetRgnBox(pdc->prgnRao, prc);
    }
    else
    {
        /* No RAO region means no API region, so use the VIS region */
        ASSERT(pdc->prgnVis);
        iComplexity = REGION_GetRgnBox(pdc->prgnVis, prc);
    }

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Convert the rect to logical coordinates */
    IntDPtoLP(pdc, (LPPOINT)prc, 2);

    /* Return the complexity */
    return iComplexity;
}

_Success_(return!=ERROR)
INT
APIENTRY
NtGdiGetAppClipBox(
    _In_ HDC hdc,
    _Out_ LPRECT prc)
{
    RECT rect;
    INT iComplexity;

    /* Call the internal function */
    iComplexity = GdiGetClipBox(hdc, &rect);

    if (iComplexity != ERROR)
    {
        _SEH2_TRY
        {
            ProbeForWrite(prc, sizeof(RECT), 1);
            *prc = rect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            iComplexity = ERROR;
        }
        _SEH2_END
    }

    /* Return the complexity */
    return iComplexity;
}

INT
APIENTRY
NtGdiExcludeClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    INT iComplexity = ERROR;
    RECTL rect;
    PDC pdc;
    PREGION prgn;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    /* Convert coordinates to device space */
    rect.left   = xLeft;
    rect.top    = yTop;
    rect.right  = xRight;
    rect.bottom = yBottom;
    RECTL_vMakeWellOrdered(&rect);
    IntLPtoDP(pdc, (LPPOINT)&rect, 2);

    prgn = IntSysCreateRectpRgnIndirect(&rect);
    if ( prgn )
    {
        iComplexity = IntSelectClipRgn( pdc, prgn, RGN_DIFF );

        REGION_Delete(prgn);
    }

    /* Emulate Windows behavior */
    if (iComplexity == SIMPLEREGION)
        iComplexity = COMPLEXREGION;

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    return iComplexity;
}

INT
APIENTRY
NtGdiIntersectClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    INT iComplexity = ERROR;
    RECTL rect;
    PDC pdc;
    PREGION prgn;

    DPRINT("NtGdiIntersectClipRect(%p, %d,%d-%d,%d)\n",
            hdc, xLeft, yTop, xRight, yBottom);

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    /* Convert coordinates to device space */
    rect.left   = xLeft;
    rect.top    = yTop;
    rect.right  = xRight;
    rect.bottom = yBottom;
    RECTL_vMakeWellOrdered(&rect);
    IntLPtoDP(pdc, (LPPOINT)&rect, 2);

    prgn = IntSysCreateRectpRgnIndirect(&rect);
    if ( prgn )
    {
        iComplexity = IntSelectClipRgn( pdc, prgn, RGN_AND );

        REGION_Delete(prgn);
    }

    /* Emulate Windows behavior */
    if ( iComplexity == SIMPLEREGION )
        iComplexity = COMPLEXREGION;

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    return iComplexity;
}

INT
APIENTRY
NtGdiOffsetClipRgn(
    _In_ HDC hdc,
    _In_ INT xOffset,
    _In_ INT yOffset)
{
    INT iComplexity;
    PDC pdc;
    POINTL apt[2];

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        if (!hdc) EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    /* Check if we have a clip region */
    if (pdc->dclevel.prgnClip != NULL)
    {
        /* Convert coordinates into device space. Note that we need to convert
           2 coordinates to account for rotation / shear / offset */
        apt[0].x = 0;
        apt[0].y = 0;
        apt[1].x = xOffset;
        apt[1].y = yOffset;
        IntLPtoDP(pdc, apt, 2);

        /* Offset the clip region */
        if (!REGION_bOffsetRgn(pdc->dclevel.prgnClip,
                               apt[1].x - apt[0].x,
                               apt[1].y - apt[0].y))
        {
            iComplexity = ERROR;
        }
        else
        {
            IntGdiReleaseRaoRgn(pdc);
            UpdateVisRgn(pdc);
            iComplexity = REGION_Complexity(pdc->dclevel.prgnClip);
        }

        /* Mark the RAO region as dirty */
        pdc->fs |= DC_DIRTY_RAO;
    }
    else
    {
        /* NULL means no clipping, i.e. the "whole" region */
        iComplexity = SIMPLEREGION;
    }

    /* Unlock the DC and return the complexity */
    DC_UnlockDc(pdc);
    return iComplexity;
}

BOOL APIENTRY NtGdiPtVisible(HDC  hDC,
                    int  X,
                    int  Y)
{
    BOOL ret = FALSE;
    PDC dc;
    PREGION prgn;

    if(!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    prgn = dc->prgnRao ? dc->prgnRao : dc->prgnVis;

    if (prgn)
    {
        POINT pt = {X, Y};
        IntLPtoDP(dc, &pt, 1);
        ret = REGION_PtInRegion(prgn, pt.x, pt.y);
    }

    DC_UnlockDc(dc);

    return ret;
}

BOOL
APIENTRY
NtGdiRectVisible(
    HDC hDC,
    LPRECT UnsafeRect)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDC dc = DC_LockDc(hDC);
    BOOL Result = FALSE;
    RECTL Rect;
    PREGION prgn;

    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnsafeRect,
                   sizeof(RECT),
                   1);
        Rect = *UnsafeRect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if(!NT_SUCCESS(Status))
    {
        DC_UnlockDc(dc);
        SetLastNtError(Status);
        return FALSE;
    }

    if (dc->fs & DC_DIRTY_RAO)
        CLIPPING_UpdateGCRegion(dc);

    prgn = dc->prgnRao ? dc->prgnRao : dc->prgnVis;
    if (prgn)
    {
         IntLPtoDP(dc, (LPPOINT)&Rect, 2);
         Result = REGION_RectInRegion(prgn, &Rect);
    }
    DC_UnlockDc(dc);

    return Result;
}

int
FASTCALL
IntGdiSetMetaRgn(PDC pDC)
{
    INT Ret = ERROR;

    if ( pDC->dclevel.prgnMeta )
    {
        if ( pDC->dclevel.prgnClip )
        {
            PREGION prgn = IntSysCreateRectpRgn(0,0,0,0);
            if ( prgn )
            {
                if (REGION_bIntersectRegion(prgn, pDC->dclevel.prgnMeta, pDC->dclevel.prgnClip))
                {
                    // See Restore/SaveDC
                    REGION_Delete(pDC->dclevel.prgnMeta);
                    pDC->dclevel.prgnMeta = prgn;

                    REGION_Delete(pDC->dclevel.prgnClip);
                    pDC->dclevel.prgnClip = NULL;
                    IntGdiReleaseRaoRgn(pDC);

                    Ret = REGION_Complexity(pDC->dclevel.prgnMeta);
                }
                else
                    REGION_Delete(prgn);
            }
        }
        else
            Ret = REGION_Complexity(pDC->dclevel.prgnMeta);
    }
    else
    {
        if ( pDC->dclevel.prgnClip )
        {
            Ret = REGION_Complexity(pDC->dclevel.prgnClip);
            pDC->dclevel.prgnMeta = pDC->dclevel.prgnClip;
            pDC->dclevel.prgnClip = NULL;
        }
        else
            Ret = SIMPLEREGION;
    }

    return Ret;
}


int APIENTRY NtGdiSetMetaRgn(HDC  hDC)
{
  INT Ret;
  PDC pDC = DC_LockDc(hDC);

  if (!pDC)
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     return ERROR;
  }
  Ret = IntGdiSetMetaRgn(pDC);

  DC_UnlockDc(pDC);
  return Ret;
}

VOID
FASTCALL
CLIPPING_UpdateGCRegion(PDC pDC)
{
    // Moved from Release Rao. Though it still gets over written.
    RECTL_vSetEmptyRect(&pDC->erclClip);

    /* Must have VisRgn set to a valid state! */
    ASSERT (pDC->prgnVis);
#if 0 // (w2k3) This works with limitations. (w7u) ReactOS relies on Rao.
    if ( !pDC->dclevel.prgnClip &&
         !pDC->dclevel.prgnMeta &&
         !pDC->prgnAPI)
    {
        if (pDC->prgnRao)
            REGION_Delete(pDC->prgnRao);
        pDC->prgnRao = NULL;

        REGION_bOffsetRgn(pDC->prgnVis, pDC->ptlDCOrig.x, pDC->ptlDCOrig.y);

        RtlCopyMemory(&pDC->erclClip,
                      &pDC->prgnVis->rdh.rcBound,
                       sizeof(RECTL));

        IntEngUpdateClipRegion(&pDC->co,
                                pDC->prgnVis->rdh.nCount,
                                pDC->prgnVis->Buffer,
                               &pDC->erclClip);

        REGION_bOffsetRgn(pDC->prgnVis, -pDC->ptlDCOrig.x, -pDC->ptlDCOrig.y);

        pDC->fs &= ~DC_DIRTY_RAO;
        UpdateVisRgn(pDC);
        return;
    }
#endif
    if (pDC->prgnAPI)
    {
        REGION_Delete(pDC->prgnAPI);
        pDC->prgnAPI = NULL;
    }

    if (pDC->dclevel.prgnMeta || pDC->dclevel.prgnClip)
    {
        pDC->prgnAPI = IntSysCreateRectpRgn(0,0,0,0);
        if (!pDC->prgnAPI)
        {
            /* Best we can do here. Better than crashing. */
            ERR("Failed to allocate prgnAPI! Expect drawing issues!\n");
            return;
        }

        if (!pDC->dclevel.prgnMeta)
        {
            REGION_bCopy(pDC->prgnAPI,
                         pDC->dclevel.prgnClip);
        }
        else if (!pDC->dclevel.prgnClip)
        {
            REGION_bCopy(pDC->prgnAPI,
                         pDC->dclevel.prgnMeta);
        }
        else
        {
            REGION_bIntersectRegion(pDC->prgnAPI,
                                    pDC->dclevel.prgnClip,
                                    pDC->dclevel.prgnMeta);
        }
    }

    if (pDC->prgnRao)
        REGION_Delete(pDC->prgnRao);

    pDC->prgnRao = IntSysCreateRectpRgn(0,0,0,0);
    if (!pDC->prgnRao)
    {
        /* Best we can do here. Better than crashing. */
        ERR("Failed to allocate prgnRao! Expect drawing issues!\n");
        return;
    }

    if (pDC->prgnAPI)
    {
        REGION_bIntersectRegion(pDC->prgnRao,
                                pDC->prgnVis,
                                pDC->prgnAPI);
    }
    else
    {
        REGION_bCopy(pDC->prgnRao,
                     pDC->prgnVis);
    }


    REGION_bOffsetRgn(pDC->prgnRao, pDC->ptlDCOrig.x, pDC->ptlDCOrig.y);

    RtlCopyMemory(&pDC->erclClip,
                  &pDC->prgnRao->rdh.rcBound,
                  sizeof(RECTL));

    pDC->fs &= ~DC_DIRTY_RAO;
    UpdateVisRgn(pDC);

    // pDC->co should be used. Example, CLIPOBJ_cEnumStart uses XCLIPOBJ to build
    // the rects from region objects rects in pClipRgn->Buffer.
    // With pDC->co.pClipRgn->Buffer,
    // pDC->co.pClipRgn = pDC->prgnRao ? pDC->prgnRao : pDC->prgnVis;

    IntEngUpdateClipRegion(&pDC->co,
                           pDC->prgnRao->rdh.nCount,
                           pDC->prgnRao->Buffer,
                           &pDC->erclClip);

    REGION_bOffsetRgn(pDC->prgnRao, -pDC->ptlDCOrig.x, -pDC->ptlDCOrig.y);
}

/* EOF */
