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
    IntGdiOffsetRgn(dc->prgnVis, -dc->ptlDCOrig.x, -dc->ptlDCOrig.y);

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

INT FASTCALL
GdiGetClipBox(HDC hDC, PRECTL rc)
{
   INT retval;
   PDC dc;
   PROSRGNDATA pRgnNew, pRgn = NULL;

   if (!(dc = DC_LockDc(hDC)))
   {
      return ERROR;
   }

   if (dc->fs & DC_FLAG_DIRTY_RAO)
       CLIPPING_UpdateGCRegion(dc);

   /* FIXME: Rao and Vis only! */
   if (dc->prgnAPI) // APIRGN
   {
      pRgn = dc->prgnAPI;
   }
   else if (dc->dclevel.prgnMeta) // METARGN
   {
      pRgn = dc->dclevel.prgnMeta;
   }
   else if (dc->dclevel.prgnClip) // CLIPRGN
   {
       pRgn = dc->dclevel.prgnClip;
   }

   if (pRgn)
   {
      pRgnNew = IntSysCreateRectpRgn( 0, 0, 0, 0 );

	  if (!pRgnNew)
      {
         DC_UnlockDc(dc);
         return ERROR;
      }

      IntGdiCombineRgn(pRgnNew, dc->prgnVis, pRgn, RGN_AND);

      retval = REGION_GetRgnBox(pRgnNew, rc);

	  REGION_Delete(pRgnNew);

      DC_UnlockDc(dc);
      return retval;
   }

   retval = REGION_GetRgnBox(dc->prgnVis, rc);

   DC_UnlockDc(dc);

   return retval;
}

INT APIENTRY
NtGdiGetAppClipBox(HDC hDC, PRECTL rc)
{
  INT Ret;
  NTSTATUS Status = STATUS_SUCCESS;
  RECTL Saferect;

  Ret = GdiGetClipBox(hDC, &Saferect);

  _SEH2_TRY
  {
    ProbeForWrite(rc,
                  sizeof(RECT),
                  1);
    *rc = Saferect;
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
    Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return ERROR;
  }

  return Ret;
}

int APIENTRY NtGdiExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
    INT Result;
    RECTL Rect;
    PREGION prgnNew;
    PDC dc = DC_LockDc(hDC);

    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    Rect.left = LeftRect;
    Rect.top = TopRect;
    Rect.right = RightRect;
    Rect.bottom = BottomRect;

    IntLPtoDP(dc, (LPPOINT)&Rect, 2);

    prgnNew = IntSysCreateRectpRgnIndirect(&Rect);
    if (!prgnNew)
    {
        Result = ERROR;
    }
    else
    {
        if (!dc->dclevel.prgnClip)
        {
            dc->dclevel.prgnClip = IntSysCreateRectpRgn(0, 0, 0, 0);
            IntGdiCombineRgn(dc->dclevel.prgnClip, dc->prgnVis, prgnNew, RGN_DIFF);
            Result = SIMPLEREGION;
        }
        else
        {
            Result = IntGdiCombineRgn(dc->dclevel.prgnClip, dc->dclevel.prgnClip, prgnNew, RGN_DIFF);
        }
        REGION_Delete(prgnNew);
    }
    if (Result != ERROR)
        dc->fs |= DC_FLAG_DIRTY_RAO;

    DC_UnlockDc(dc);

    return Result;
}

int APIENTRY NtGdiIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
    INT Result;
    RECTL Rect;
    PREGION pNewRgn;
    PDC dc = DC_LockDc(hDC);

    DPRINT("NtGdiIntersectClipRect(%p, %d,%d-%d,%d)\n",
            hDC, LeftRect, TopRect, RightRect, BottomRect);

    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    Rect.left = LeftRect;
    Rect.top = TopRect;
    Rect.right = RightRect;
    Rect.bottom = BottomRect;

    IntLPtoDP(dc, (LPPOINT)&Rect, 2);

    pNewRgn = IntSysCreateRectpRgnIndirect(&Rect);
    if (!pNewRgn)
    {
        Result = ERROR;
    }
    else if (!dc->dclevel.prgnClip)
    {
        dc->dclevel.prgnClip = pNewRgn;
        Result = SIMPLEREGION;
    }
    else
    {
        Result = IntGdiCombineRgn(dc->dclevel.prgnClip, dc->dclevel.prgnClip, pNewRgn, RGN_AND);
        REGION_Delete(pNewRgn);
    }
    if (Result != ERROR)
        dc->fs |= DC_FLAG_DIRTY_RAO;

    DC_UnlockDc(dc);

    return Result;
}

int APIENTRY NtGdiOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
    INT Result;
    DC *dc;

    if(!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return ERROR;
    }

    if(dc->dclevel.prgnClip != NULL)
    {
        Result = IntGdiOffsetRgn(dc->dclevel.prgnClip,
                                XOffset,
                                YOffset);
        dc->fs |= DC_FLAG_DIRTY_RAO;
    }
    else
    {
        Result = NULLREGION;
    }

    DC_UnlockDc(dc);
    return Result;
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


    IntGdiOffsetRgn(pDC->prgnRao, pDC->ptlDCOrig.x, pDC->ptlDCOrig.y);

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

    IntGdiOffsetRgn(pDC->prgnRao, -pDC->ptlDCOrig.x, -pDC->ptlDCOrig.y);
}

/* EOF */
