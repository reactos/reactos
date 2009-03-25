/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

int FASTCALL
CLIPPING_UpdateGCRegion(DC* Dc)
{
   PROSRGNDATA CombinedRegion;

   if (Dc->rosdc.hGCClipRgn == NULL)
      Dc->rosdc.hGCClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

   if (Dc->rosdc.hClipRgn == NULL)
      NtGdiCombineRgn(Dc->rosdc.hGCClipRgn, Dc->rosdc.hVisRgn, 0, RGN_COPY);
   else
      NtGdiCombineRgn(Dc->rosdc.hGCClipRgn, Dc->rosdc.hClipRgn, Dc->rosdc.hVisRgn, RGN_AND);
   NtGdiOffsetRgn(Dc->rosdc.hGCClipRgn, Dc->ptlDCOrig.x, Dc->ptlDCOrig.y);

   if((CombinedRegion = REGION_LockRgn(Dc->rosdc.hGCClipRgn)))
   {
     if (Dc->rosdc.CombinedClip != NULL)
        IntEngDeleteClipRegion(Dc->rosdc.CombinedClip);

     Dc->rosdc.CombinedClip = IntEngCreateClipRegion(
        CombinedRegion->rdh.nCount,
        CombinedRegion->Buffer,
        &CombinedRegion->rdh.rcBound);

     REGION_UnlockRgn(CombinedRegion);
   }

   if ( NULL == Dc->rosdc.CombinedClip )
   {
       DPRINT1("IntEngCreateClipRegion() failed\n");
       return ERROR;
   }

   return NtGdiOffsetRgn(Dc->rosdc.hGCClipRgn, -Dc->ptlDCOrig.x, -Dc->ptlDCOrig.y);
}

INT FASTCALL
GdiSelectVisRgn(HDC hdc, HRGN hrgn)
{
  int retval;
  DC *dc;

  if (!hrgn)
  {
  	SetLastWin32Error(ERROR_INVALID_PARAMETER);
  	return ERROR;
  }
  if (!(dc = DC_LockDc(hdc)))
  {
  	SetLastWin32Error(ERROR_INVALID_HANDLE);
  	return ERROR;
  }

  dc->fs &= ~DC_FLAG_DIRTY_RAO;
  
  if (dc->rosdc.hVisRgn == NULL)
  {
    dc->rosdc.hVisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
    GDIOBJ_CopyOwnership(hdc, dc->rosdc.hVisRgn);
  }

  retval = NtGdiCombineRgn(dc->rosdc.hVisRgn, hrgn, 0, RGN_COPY);
  if ( retval != ERROR )
  {
    NtGdiOffsetRgn(dc->rosdc.hVisRgn, -dc->ptlDCOrig.x, -dc->ptlDCOrig.y);
    CLIPPING_UpdateGCRegion(dc);
  }
  DC_UnlockDc(dc);

  return retval;
}


int FASTCALL GdiExtSelectClipRgn(PDC dc,
                              HRGN hrgn,
                             int fnMode)
{
  int retval;
  //  dc->fs &= ~DC_FLAG_DIRTY_RAO;

  if (!hrgn)
  {
    if (fnMode == RGN_COPY)
    {
      if (dc->rosdc.hClipRgn != NULL)
      {
        GreDeleteObject(dc->rosdc.hClipRgn);
        dc->rosdc.hClipRgn = NULL;
        retval = NULLREGION;
      }
    }
    else
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return ERROR;
    }
  }
  else
  {
    if (!dc->rosdc.hClipRgn)
    {
      PROSRGNDATA Rgn;
      RECTL rect;
      if((Rgn = REGION_LockRgn(dc->rosdc.hVisRgn)))
      {
        REGION_GetRgnBox(Rgn, &rect);
        REGION_UnlockRgn(Rgn);
        dc->rosdc.hClipRgn = UnsafeIntCreateRectRgnIndirect(&rect);
      }
      else
      {
        dc->rosdc.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
    }
    if(fnMode == RGN_COPY)
    {
      NtGdiCombineRgn(dc->rosdc.hClipRgn, hrgn, 0, fnMode);
    }
    else
      NtGdiCombineRgn(dc->rosdc.hClipRgn, dc->rosdc.hClipRgn, hrgn, fnMode);
  }

  retval = CLIPPING_UpdateGCRegion(dc);
  return retval;
}


int APIENTRY NtGdiExtSelectClipRgn(HDC  hDC,
                                HRGN  hrgn,
                               int  fnMode)
{
  int retval;
  DC *dc;

  if (!(dc = DC_LockDc(hDC)))
  {
  	SetLastWin32Error(ERROR_INVALID_HANDLE);
  	return ERROR;
  }

  retval = GdiExtSelectClipRgn ( dc, hrgn, fnMode );

  DC_UnlockDc(dc);
  return retval;
}

INT FASTCALL
GdiGetClipBox(HDC hDC, PRECTL rc)
{
   PROSRGNDATA Rgn;
   INT retval;
   PDC dc;

   if (!(dc = DC_LockDc(hDC)))
   {
      return ERROR;
   }

   if (!(Rgn = REGION_LockRgn(dc->rosdc.hGCClipRgn)))
   {
      DC_UnlockDc(dc);
      return ERROR;
   }
   retval = REGION_GetRgnBox(Rgn, rc);
   REGION_UnlockRgn(Rgn);
   IntDPtoLP(dc, (LPPOINT)rc, 2);
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
   HRGN NewRgn;
   PDC dc = DC_LockDc(hDC);

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }

   Rect.left = LeftRect;
   Rect.top = TopRect;
   Rect.right = RightRect;
   Rect.bottom = BottomRect;

   IntLPtoDP(dc, (LPPOINT)&Rect, 2);

   NewRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
   if (!NewRgn)
   {
      Result = ERROR;
   }
   else
   {
      if (!dc->rosdc.hClipRgn)
      {
         dc->rosdc.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(dc->rosdc.hClipRgn, dc->rosdc.hVisRgn, NewRgn, RGN_DIFF);
         Result = SIMPLEREGION;
      }
      else
      {
         Result = NtGdiCombineRgn(dc->rosdc.hClipRgn, dc->rosdc.hClipRgn, NewRgn, RGN_DIFF);
      }
      GreDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

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
   HRGN NewRgn;
   PDC dc = DC_LockDc(hDC);

   DPRINT("NtGdiIntersectClipRect(%x, %d,%d-%d,%d)\n",
      hDC, LeftRect, TopRect, RightRect, BottomRect);

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return ERROR;
   }

   Rect.left = LeftRect;
   Rect.top = TopRect;
   Rect.right = RightRect;
   Rect.bottom = BottomRect;

   IntLPtoDP(dc, (LPPOINT)&Rect, 2);

   NewRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
   if (!NewRgn)
   {
      Result = ERROR;
   }
   else if (!dc->rosdc.hClipRgn)
   {
      dc->rosdc.hClipRgn = NewRgn;
      Result = SIMPLEREGION;
   }
   else
   {
      Result = NtGdiCombineRgn(dc->rosdc.hClipRgn, dc->rosdc.hClipRgn, NewRgn, RGN_AND);
      GreDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

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
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return ERROR;
  }

  if(dc->rosdc.hClipRgn != NULL)
  {
    Result = NtGdiOffsetRgn(dc->rosdc.hClipRgn,
                            XOffset,
                            YOffset);
    CLIPPING_UpdateGCRegion(dc);
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
  HRGN rgn;
  DC *dc;

  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  rgn = dc->rosdc.hGCClipRgn;
  DC_UnlockDc(dc);

  return (rgn ? NtGdiPtInRegion(rgn, X, Y) : FALSE);
}

BOOL APIENTRY NtGdiRectVisible(HDC  hDC,
                      LPRECT UnsafeRect)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PROSRGNDATA Rgn;
   PDC dc = DC_LockDc(hDC);
   BOOL Result = FALSE;
   RECTL Rect;

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
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

   if (dc->rosdc.hGCClipRgn)
   {
      if((Rgn = (PROSRGNDATA)REGION_LockRgn(dc->rosdc.hGCClipRgn)))
      {
         IntLPtoDP(dc, (LPPOINT)&Rect, 2);
         Result = REGION_RectInRegion(Rgn, &Rect);
         REGION_UnlockRgn(Rgn);
      }
   }
   DC_UnlockDc(dc);

   return Result;
}

int
FASTCALL 
IntGdiSetMetaRgn(PDC pDC)
{
  INT Ret = ERROR;
  PROSRGNDATA TempRgn;

  if ( pDC->dclevel.prgnMeta )
  {
     if ( pDC->dclevel.prgnClip )
     {
        TempRgn = IntGdiCreateRectRgn(0,0,0,0);
        if (TempRgn)
        {        
           Ret = IntGdiCombineRgn( TempRgn,
                     pDC->dclevel.prgnMeta,
                     pDC->dclevel.prgnClip,
                                   RGN_AND);
           if ( Ret )
           {
              GDIOBJ_ShareUnlockObjByPtr(pDC->dclevel.prgnMeta);
              if (!((PROSRGNDATA)pDC->dclevel.prgnMeta)->BaseObject.ulShareCount)
                 REGION_Delete(pDC->dclevel.prgnMeta);

              pDC->dclevel.prgnMeta = TempRgn;

              GDIOBJ_ShareUnlockObjByPtr(pDC->dclevel.prgnClip);
              if (!((PROSRGNDATA)pDC->dclevel.prgnClip)->BaseObject.ulShareCount)
                 REGION_Delete(pDC->dclevel.prgnClip);

              pDC->dclevel.prgnClip = NULL;

              IntGdiReleaseRaoRgn(pDC);
           }
           else
              REGION_Delete(TempRgn);
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
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     return ERROR;
  }
  Ret = IntGdiSetMetaRgn(pDC);

  DC_UnlockDc(pDC);
  return Ret;
}

INT FASTCALL
NEW_CLIPPING_UpdateGCRegion(PDC pDC)
{
  CLIPOBJ * co;

  if (!pDC->prgnVis) return 0;

  if (pDC->prgnAPI)
  {
     REGION_Delete(pDC->prgnAPI);
     pDC->prgnAPI = IntGdiCreateRectRgn(0,0,0,0);
  }

  if (pDC->prgnRao)
  {
     REGION_Delete(pDC->prgnRao);
     pDC->prgnRao = IntGdiCreateRectRgn(0,0,0,0);
  }
  
  if (pDC->dclevel.prgnMeta && pDC->dclevel.prgnClip)
  {
     IntGdiCombineRgn( pDC->prgnAPI,
              pDC->dclevel.prgnClip,
              pDC->dclevel.prgnMeta,
                            RGN_AND);
  }
  else
  {
     if (pDC->dclevel.prgnClip)
        IntGdiCombineRgn( pDC->prgnAPI,
                 pDC->dclevel.prgnClip,
                                  NULL,
                              RGN_COPY);
     else if (pDC->dclevel.prgnMeta)
        IntGdiCombineRgn( pDC->prgnAPI,
                 pDC->dclevel.prgnMeta,
                                  NULL,
                              RGN_COPY);
  }

  IntGdiCombineRgn( pDC->prgnRao,
                    pDC->prgnVis,
                    pDC->prgnAPI,
                         RGN_AND);

  RtlCopyMemory(&pDC->erclClip, &((PROSRGNDATA)pDC->prgnRao)->rdh.rcBound , sizeof(RECTL));
  pDC->fs &= ~DC_FLAG_DIRTY_RAO;

//  if (Dc->CombinedClip != NULL) IntEngDeleteClipRegion(Dc->CombinedClip);
  
  co = IntEngCreateClipRegion( ((PROSRGNDATA)pDC->prgnRao)->rdh.nCount,
                           ((PROSRGNDATA)pDC->prgnRao)->Buffer,
                                 &pDC->erclClip);

  return REGION_Complexity(pDC->prgnRao);
}

/* EOF */
