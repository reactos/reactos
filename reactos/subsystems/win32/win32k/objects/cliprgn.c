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

   if (Dc->w.hGCClipRgn == NULL)
      Dc->w.hGCClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

   if (Dc->w.hClipRgn == NULL)
      NtGdiCombineRgn(Dc->w.hGCClipRgn, Dc->w.hVisRgn, 0, RGN_COPY);
   else
      NtGdiCombineRgn(Dc->w.hGCClipRgn, Dc->w.hClipRgn, Dc->w.hVisRgn, RGN_AND);
   NtGdiOffsetRgn(Dc->w.hGCClipRgn, Dc->ptlDCOrig.x, Dc->ptlDCOrig.y);

   if((CombinedRegion = REGION_LockRgn(Dc->w.hGCClipRgn)))
   {
     if (Dc->CombinedClip != NULL)
        IntEngDeleteClipRegion(Dc->CombinedClip);

     Dc->CombinedClip = IntEngCreateClipRegion(
        CombinedRegion->rdh.nCount,
        (PRECTL)CombinedRegion->Buffer,
        (PRECTL)&CombinedRegion->rdh.rcBound);

     REGION_UnlockRgn(CombinedRegion);
   }

   if ( NULL == Dc->CombinedClip )
   {
       DPRINT1("IntEngCreateClipRegion() failed\n");
       return ERROR;
   }

   return NtGdiOffsetRgn(Dc->w.hGCClipRgn, -Dc->ptlDCOrig.x, -Dc->ptlDCOrig.y);
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

  dc->DC_Flags &= ~DC_FLAG_DIRTY_RAO;
  
  if (dc->w.hVisRgn == NULL)
  {
    dc->w.hVisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
    GDIOBJ_CopyOwnership(hdc, dc->w.hVisRgn);
  }

  retval = NtGdiCombineRgn(dc->w.hVisRgn, hrgn, 0, RGN_COPY);
  if ( retval != ERROR )
  {
    NtGdiOffsetRgn(dc->w.hVisRgn, -dc->ptlDCOrig.x, -dc->ptlDCOrig.y);
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
  //  dc->DC_Flags &= ~DC_FLAG_DIRTY_RAO;

  if (!hrgn)
  {
    if (fnMode == RGN_COPY)
    {
      if (dc->w.hClipRgn != NULL)
      {
        NtGdiDeleteObject(dc->w.hClipRgn);
        dc->w.hClipRgn = NULL;
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
    if (!dc->w.hClipRgn)
    {
      PROSRGNDATA Rgn;
      RECT rect;
      if((Rgn = REGION_LockRgn(dc->w.hVisRgn)))
      {
        REGION_GetRgnBox(Rgn, &rect);
        REGION_UnlockRgn(Rgn);
        dc->w.hClipRgn = UnsafeIntCreateRectRgnIndirect(&rect);
      }
      else
      {
        dc->w.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      }
    }
    if(fnMode == RGN_COPY)
    {
      NtGdiCombineRgn(dc->w.hClipRgn, hrgn, 0, fnMode);
    }
    else
      NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, hrgn, fnMode);
  }

  retval = CLIPPING_UpdateGCRegion(dc);
  return retval;
}


int STDCALL NtGdiExtSelectClipRgn(HDC  hDC,
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
GdiGetClipBox(HDC hDC, LPRECT rc)
{
   PROSRGNDATA Rgn;
   INT retval;
   PDC dc;

   if (!(dc = DC_LockDc(hDC)))
   {
      return ERROR;
   }

   if (!(Rgn = REGION_LockRgn(dc->w.hGCClipRgn)))
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

INT STDCALL
NtGdiGetAppClipBox(HDC hDC, LPRECT rc)
{
  INT Ret;
  NTSTATUS Status = STATUS_SUCCESS;
  RECT Saferect;

  Ret = GdiGetClipBox(hDC, &Saferect);

  _SEH_TRY
  {
    ProbeForWrite(rc,
                  sizeof(RECT),
                  1);
    *rc = Saferect;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return ERROR;
  }

  return Ret;
}

int STDCALL NtGdiExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
   INT Result;
   RECT Rect;
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
      if (!dc->w.hClipRgn)
      {
         dc->w.hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hVisRgn, NewRgn, RGN_DIFF);
         Result = SIMPLEREGION;
      }
      else
      {
         Result = NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, NewRgn, RGN_DIFF);
      }
      NtGdiDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

   DC_UnlockDc(dc);

   return Result;
}

int STDCALL NtGdiIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
   INT Result;
   RECT Rect;
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
   else if (!dc->w.hClipRgn)
   {
      dc->w.hClipRgn = NewRgn;
      Result = SIMPLEREGION;
   }
   else
   {
      Result = NtGdiCombineRgn(dc->w.hClipRgn, dc->w.hClipRgn, NewRgn, RGN_AND);
      NtGdiDeleteObject(NewRgn);
   }
   if (Result != ERROR)
      CLIPPING_UpdateGCRegion(dc);

   DC_UnlockDc(dc);

   return Result;
}

int STDCALL NtGdiOffsetClipRgn(HDC  hDC,
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

  if(dc->w.hClipRgn != NULL)
  {
    Result = NtGdiOffsetRgn(dc->w.hClipRgn,
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

BOOL STDCALL NtGdiPtVisible(HDC  hDC,
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

  rgn = dc->w.hGCClipRgn;
  DC_UnlockDc(dc);

  return (rgn ? NtGdiPtInRegion(rgn, X, Y) : FALSE);
}

BOOL STDCALL NtGdiRectVisible(HDC  hDC,
                      CONST PRECT  UnsafeRect)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PROSRGNDATA Rgn;
   PDC dc = DC_LockDc(hDC);
   BOOL Result = FALSE;
   RECT Rect;

   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   _SEH_TRY
   {
      ProbeForRead(UnsafeRect,
                   sizeof(RECT),
                   1);
      Rect = *UnsafeRect;
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

   if(!NT_SUCCESS(Status))
   {
      DC_UnlockDc(dc);
      SetLastNtError(Status);
      return FALSE;
   }

   if (dc->w.hGCClipRgn)
   {
      if((Rgn = (PROSRGNDATA)REGION_LockRgn(dc->w.hGCClipRgn)))
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

  if ( pDC->DcLevel.prgnMeta )
  {
     if ( pDC->DcLevel.prgnClip )
     {
        TempRgn = IntGdiCreateRectRgn(0,0,0,0);
        if (TempRgn)
        {        
           Ret = IntGdiCombineRgn( TempRgn,
                     pDC->DcLevel.prgnMeta,
                     pDC->DcLevel.prgnClip,
                                   RGN_AND);
           if ( Ret )
           {
              GDIOBJ_ShareUnlockObjByPtr(pDC->DcLevel.prgnMeta);
              if (!((PROSRGNDATA)pDC->DcLevel.prgnMeta)->BaseObject.ulShareCount)
                 REGION_Delete(pDC->DcLevel.prgnMeta);

              pDC->DcLevel.prgnMeta = TempRgn;

              GDIOBJ_ShareUnlockObjByPtr(pDC->DcLevel.prgnClip);
              if (!((PROSRGNDATA)pDC->DcLevel.prgnClip)->BaseObject.ulShareCount)
                 REGION_Delete(pDC->DcLevel.prgnClip);

              pDC->DcLevel.prgnClip = NULL;

              IntGdiReleaseRaoRgn(pDC);
           }
           else
              REGION_Delete(TempRgn);
        }
     }
     else
        Ret = REGION_Complexity(pDC->DcLevel.prgnMeta);
  }
  else
  {
     if ( pDC->DcLevel.prgnClip )
     {
        Ret = REGION_Complexity(pDC->DcLevel.prgnClip);
        pDC->DcLevel.prgnMeta = pDC->DcLevel.prgnClip;
        pDC->DcLevel.prgnClip = NULL;
     }
     else 
       Ret = SIMPLEREGION;
  }
  return Ret;
}


int STDCALL NtGdiSetMetaRgn(HDC  hDC)
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
  
  if (pDC->DcLevel.prgnMeta && pDC->DcLevel.prgnClip)
  {
     IntGdiCombineRgn( pDC->prgnAPI,
              pDC->DcLevel.prgnClip,
              pDC->DcLevel.prgnMeta,
                            RGN_AND);
  }
  else
  {
     if (pDC->DcLevel.prgnClip)
        IntGdiCombineRgn( pDC->prgnAPI,
                 pDC->DcLevel.prgnClip,
                                  NULL,
                              RGN_COPY);
     else if (pDC->DcLevel.prgnMeta)
        IntGdiCombineRgn( pDC->prgnAPI,
                 pDC->DcLevel.prgnMeta,
                                  NULL,
                              RGN_COPY);
  }

  IntGdiCombineRgn( pDC->prgnRao,
                    pDC->prgnVis,
                    pDC->prgnAPI,
                         RGN_AND);

  RtlCopyMemory(&pDC->erclClip, &((PROSRGNDATA)pDC->prgnRao)->rdh.rcBound , sizeof(RECTL));
  pDC->DC_Flags &= ~DC_FLAG_DIRTY_RAO;

//  if (Dc->CombinedClip != NULL) IntEngDeleteClipRegion(Dc->CombinedClip);
  
  co = IntEngCreateClipRegion( ((PROSRGNDATA)pDC->prgnRao)->rdh.nCount,
                           (PRECTL)((PROSRGNDATA)pDC->prgnRao)->Buffer,
                                 (PRECTL)&pDC->erclClip);

  return REGION_Complexity(pDC->prgnRao);
}

/* EOF */
