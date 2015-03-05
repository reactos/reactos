/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Clip region functions
 * FILE:             subsystems/win32/win32k/objects/cliprgn.c
 * PROGRAMER:        Unknown
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

VOID
FASTCALL
IntGdiReleaseRaoRgn(PDC pDC)
{
    INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    pDC->fs |= DC_FLAG_DIRTY_RAO;
    Entry->Flags |= GDI_ENTRY_VALIDATE_VIS;
    RECTL_vSetEmptyRect(&pDC->erclClip);
    REGION_Delete(pDC->prgnRao);
    pDC->prgnRao = NULL;
}

VOID
FASTCALL
IntGdiReleaseVisRgn(PDC pDC)
{
    INT Index = GDI_HANDLE_GET_INDEX(pDC->BaseObject.hHmgr);
    PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
    pDC->fs |= DC_FLAG_DIRTY_RAO;
    Entry->Flags |= GDI_ENTRY_VALIDATE_VIS;
    RECTL_vSetEmptyRect(&pDC->erclClip);
    REGION_Delete(pDC->prgnVis);
    pDC->prgnVis = prgnDefault;
}

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

    dc->fs |= DC_FLAG_DIRTY_RAO;

    ASSERT(dc->prgnVis != NULL);
    ASSERT(prgn != NULL);

    IntGdiCombineRgn(dc->prgnVis, prgn, NULL, RGN_COPY);
    REGION_bOffsetRgn(dc->prgnVis, -dc->ptlDCOrig.x, -dc->ptlDCOrig.y);

    DC_UnlockDc(dc);
}


int
FASTCALL
IntGdiExtSelectClipRgn(
    PDC dc,
    PREGION prgn,
    int fnMode)
{
    if (fnMode == RGN_COPY)
    {
        if (!prgn)
        {
            if (dc->dclevel.prgnClip != NULL)
            {
                REGION_Delete(dc->dclevel.prgnClip);
                dc->dclevel.prgnClip = NULL;
                dc->fs |= DC_FLAG_DIRTY_RAO;
            }
            return SIMPLEREGION;
        }

        if (!dc->dclevel.prgnClip)
            dc->dclevel.prgnClip = IntSysCreateRectpRgn(0, 0, 0, 0);

        dc->fs |= DC_FLAG_DIRTY_RAO;

        return IntGdiCombineRgn(dc->dclevel.prgnClip, prgn, NULL, RGN_COPY);
    }

    ASSERT(prgn != NULL);

    if (!dc->dclevel.prgnClip)
    {
        RECTL rect;

        REGION_GetRgnBox(dc->prgnVis, &rect);
        dc->dclevel.prgnClip = IntSysCreateRectpRgnIndirect(&rect);
    }

    dc->fs |= DC_FLAG_DIRTY_RAO;

    return IntGdiCombineRgn(dc->dclevel.prgnClip, dc->dclevel.prgnClip, prgn, fnMode);
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

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    prgn = REGION_LockRgn(hrgn);

    if ((prgn == NULL) && (fnMode != RGN_COPY))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        retval = ERROR;
    }
    else
    {
        retval = IntGdiExtSelectClipRgn(dc, prgn, fnMode);
    }

    if (prgn)
        REGION_UnlockRgn(prgn);

    DC_UnlockDc(dc);
    return retval;
}

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
    if (pdc->fs & DC_FLAG_DIRTY_RAO)
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
    INT iComplexity;
    RECTL rect;
    PDC pdc;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (pdc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    /* Convert coordinates to device space */
    rect.left = xLeft;
    rect.top = yTop;
    rect.right = xRight;
    rect.bottom = yBottom;
    RECTL_vMakeWellOrdered(&rect);
    IntLPtoDP(pdc, (LPPOINT)&rect, 2);

    /* Check if we already have a clip region */
    if (pdc->dclevel.prgnClip != NULL)
    {
        /* We have a region, subtract the rect */
        iComplexity = REGION_SubtractRectFromRgn(pdc->dclevel.prgnClip,
                                                 pdc->dclevel.prgnClip,
                                                 &rect);
    }
    else
    {
        /* We don't have a clip region yet, create an empty region */
        pdc->dclevel.prgnClip = IntSysCreateRectpRgn(0, 0, 0, 0);
        if (pdc->dclevel.prgnClip == NULL)
        {
            iComplexity = ERROR;
        }
        else
        {
            /* Subtract the rect from the VIS region */
            iComplexity = REGION_SubtractRectFromRgn(pdc->dclevel.prgnClip,
                                                     pdc->prgnVis,
                                                     &rect);
        }
    }

    /* Emulate Windows behavior */
    if (iComplexity == SIMPLEREGION)
        iComplexity = COMPLEXREGION;

    /* If we succeeded, mark the RAO region as dirty */
    if (iComplexity != ERROR)
        pdc->fs |= DC_FLAG_DIRTY_RAO;

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
    INT iComplexity;
    RECTL rect;
    PREGION prgnNew;
    PDC pdc;

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
    rect.left = xLeft;
    rect.top = yTop;
    rect.right = xRight;
    rect.bottom = yBottom;
    IntLPtoDP(pdc, (LPPOINT)&rect, 2);

    /* Check if we already have a clip region */
    if (pdc->dclevel.prgnClip != NULL)
    {
        /* We have a region, crop it */
        iComplexity = REGION_CropRegion(pdc->dclevel.prgnClip,
                                        pdc->dclevel.prgnClip,
                                        &rect);
    }
    else
    {
        /* We don't have a region yet, allocate a new one */
        prgnNew = IntSysCreateRectpRgnIndirect(&rect);
        if (prgnNew == NULL)
        {
            iComplexity = ERROR;
        }
        else
        {
            /* Set the new region */
            pdc->dclevel.prgnClip = prgnNew;
            iComplexity = SIMPLEREGION;
        }
    }

    /* If we succeeded, mark the RAO region as dirty */
    if (iComplexity != ERROR)
        pdc->fs |= DC_FLAG_DIRTY_RAO;

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
        IntLPtoDP(pdc, &apt, 2);

        /* Offset the clip region */
        if (!REGION_bOffsetRgn(pdc->dclevel.prgnClip,
                               apt[1].x - apt[0].x,
                               apt[1].y - apt[0].y))
        {
            iComplexity = ERROR;
        }
        else
        {
            iComplexity = REGION_Complexity(pdc->dclevel.prgnClip);
        }

        /* Mark the RAO region as dirty */
        pdc->fs |= DC_FLAG_DIRTY_RAO;
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

    if(!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (dc->prgnRao)
    {
        POINT pt = {X, Y};
        IntLPtoDP(dc, &pt, 1);
        ret = REGION_PtInRegion(dc->prgnRao, pt.x, pt.y);
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

    if (dc->fs & DC_FLAG_DIRTY_RAO)
        CLIPPING_UpdateGCRegion(dc);

    if (dc->prgnRao)
    {
         IntLPtoDP(dc, (LPPOINT)&Rect, 2);
         Result = REGION_RectInRegion(dc->prgnRao, &Rect);
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
            // preferably REGION_IntersectRegion
            Ret = IntGdiCombineRgn(pDC->dclevel.prgnMeta, pDC->dclevel.prgnMeta, pDC->dclevel.prgnClip, RGN_AND);
            if (Ret != ERROR)
            {
                REGION_Delete(pDC->dclevel.prgnClip);
                pDC->dclevel.prgnClip = NULL;
                IntGdiReleaseRaoRgn(pDC);
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

    if (Ret != ERROR)
        pDC->fs |= DC_FLAG_DIRTY_RAO;

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
    /* Must have VisRgn set to a valid state! */
    ASSERT (pDC->prgnVis);

    if (pDC->prgnAPI)
    {
        REGION_Delete(pDC->prgnAPI);
        pDC->prgnAPI = NULL;
    }

    if (pDC->prgnRao)
        REGION_Delete(pDC->prgnRao);

    pDC->prgnRao = IntSysCreateRectpRgn(0,0,0,0);

    ASSERT(pDC->prgnRao);

    if (pDC->dclevel.prgnMeta || pDC->dclevel.prgnClip)
    {
        pDC->prgnAPI = IntSysCreateRectpRgn(0,0,0,0);
        if (!pDC->dclevel.prgnMeta)
        {
            IntGdiCombineRgn(pDC->prgnAPI,
                             pDC->dclevel.prgnClip,
                             NULL,
                             RGN_COPY);
        }
        else if (!pDC->dclevel.prgnClip)
        {
            IntGdiCombineRgn(pDC->prgnAPI,
                             pDC->dclevel.prgnMeta,
                             NULL,
                             RGN_COPY);
        }
        else
        {
            IntGdiCombineRgn(pDC->prgnAPI,
                             pDC->dclevel.prgnClip,
                             pDC->dclevel.prgnMeta,
                             RGN_AND);
        }
    }

    if (pDC->prgnAPI)
    {
        IntGdiCombineRgn(pDC->prgnRao,
                         pDC->prgnVis,
                         pDC->prgnAPI,
                         RGN_AND);
    }
    else
    {
        IntGdiCombineRgn(pDC->prgnRao,
                         pDC->prgnVis,
                         NULL,
                         RGN_COPY);
    }


    REGION_bOffsetRgn(pDC->prgnRao, pDC->ptlDCOrig.x, pDC->ptlDCOrig.y);

    RtlCopyMemory(&pDC->erclClip,
                &pDC->prgnRao->rdh.rcBound,
                sizeof(RECTL));

    pDC->fs &= ~DC_FLAG_DIRTY_RAO;

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
