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
/* $Id: line.c,v 1.33 2004/06/20 00:45:37 navaraf Exp $ */
#include <w32k.h>

// Some code from the WINE project source (www.winehq.com)


BOOL FASTCALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point)
{
  BOOL  PathIsOpen;
  
  if ( Point )
  {
    Point->x = dc->w.CursPosX;
    Point->y = dc->w.CursPosY;
  }
  dc->w.CursPosX = X;
  dc->w.CursPosY = Y;
  
  PathIsOpen = PATH_IsPathOpen(dc->w.path);
  
  if ( PathIsOpen )
    return PATH_MoveTo ( dc );
  
  return TRUE;
}

BOOL FASTCALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd)
{
  SURFOBJ *SurfObj;
  BOOL     Ret;
  PGDIBRUSHOBJ PenBrushObj;
  RECTL    Bounds;
  POINT    Points[2];

  SurfObj = (SURFOBJ*)AccessUserObject ( (ULONG)dc->Surface );
  if (NULL == SurfObj)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (PATH_IsPathOpen(dc->w.path))
    {
      Ret = PATH_LineTo(dc, XEnd, YEnd);
      if (Ret)
	  {
	    // FIXME - PATH_LineTo should maybe do this...
	    dc->w.CursPosX = XEnd;
	    dc->w.CursPosY = YEnd;
	  }
      return Ret;
    }
  else
    {
      Points[0].x = dc->w.CursPosX;
      Points[0].y = dc->w.CursPosY;
      Points[1].x = XEnd;
      Points[1].y = YEnd;

      IntLPtoDP(dc, Points, 2);

      /* FIXME: Is it correct to do this after the transformation? */
      Points[0].x += dc->w.DCOrgX;
      Points[0].y += dc->w.DCOrgY;
      Points[1].x += dc->w.DCOrgX;
      Points[1].y += dc->w.DCOrgY;

      Bounds.left = min(Points[0].x, Points[1].x);
      Bounds.top = min(Points[0].y, Points[1].y);
      Bounds.right = max(Points[0].x, Points[1].x);
      Bounds.bottom = max(Points[0].y, Points[1].y);

      /* get BRUSHOBJ from current pen. */
      PenBrushObj = PENOBJ_LockPen( dc->w.hPen );
      ASSERT(PenBrushObj);

      if (!(PenBrushObj->flAttrs & GDIBRUSH_IS_NULL))
      {
        Ret = IntEngLineTo(SurfObj,
                           dc->CombinedClip,
                           &PenBrushObj->BrushObject,
                           Points[0].x, Points[0].y,
                           Points[1].x, Points[1].y,
                           &Bounds,
                           dc->w.ROPmode);
      }

      PENOBJ_UnlockPen( dc->w.hPen );
    }

  if (Ret)
    {
      dc->w.CursPosX = XEnd;
      dc->w.CursPosY = YEnd;
    }

  return Ret;
}

BOOL FASTCALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count)
{
  BOOL ret = FALSE; // default to FAILURE

  if ( PATH_IsPathOpen(dc->w.path) )
  {
    return PATH_PolyBezier ( dc, pt, Count );
  }

  /* We'll convert it into line segments and draw them using Polyline */
  {
    POINT *Pts;
    INT nOut;

    Pts = GDI_Bezier ( pt, Count, &nOut );
    if ( Pts )
    {
      DbgPrint("Pts = %p, no = %d\n", Pts, nOut);
      ret = IntGdiPolyline(dc, Pts, nOut);
      ExFreePool(Pts);
    }
  }
  
  return ret;
}

BOOL FASTCALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD  Count)
{
  BOOL ret = FALSE; // default to failure

  if ( PATH_IsPathOpen(dc->w.path) )
    ret = PATH_PolyBezierTo ( dc, pt, Count );
  else /* We'll do it using PolyBezier */
  {
    POINT *npt;
    npt = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * (Count + 1), TAG_BEZIER);
    if ( npt )
    {
      npt[0].x = dc->w.CursPosX;
      npt[0].y = dc->w.CursPosY;
      memcpy(npt + 1, pt, sizeof(POINT) * Count);
      ret = IntGdiPolyBezier(dc, npt, Count+1);
      ExFreePool(npt);
    }
  }
  if ( ret )
  {
    dc->w.CursPosX = pt[Count-1].x;
    dc->w.CursPosY = pt[Count-1].y;
  }
  
  return ret;
}

BOOL FASTCALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count)
{
  SURFOBJ     *SurfObj = NULL;
  BOOL         ret = FALSE; // default to failure
  LONG         i;
  PGDIBRUSHOBJ PenBrushObj;
  POINT       *pts;

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  ASSERT(SurfObj);

  if ( PATH_IsPathOpen ( dc->w.path ) )
    return PATH_Polyline ( dc, pt, Count );

  //Allocate "Count" bytes of memory to hold a safe copy of pt
  pts = (POINT*)ExAllocatePoolWithTag ( PagedPool, sizeof(POINT)*Count, TAG_SHAPE );
  if ( pts )
  {
    // safely copy pt to local version
    if ( STATUS_SUCCESS == MmCopyFromCaller(pts, pt, sizeof(POINT)*Count) )
    {
      //offset the array of point by the dc->w.DCOrg
      for ( i = 0; i < Count; i++ )
      {
	pts[i].x += dc->w.DCOrgX;
	pts[i].y += dc->w.DCOrgY;
      }

      /* get BRUSHOBJ from current pen. */
      PenBrushObj = PENOBJ_LockPen( dc->w.hPen );
      ASSERT(PenBrushObj);

      if (!(PenBrushObj->flAttrs & GDIBRUSH_IS_NULL))
      {
        //get IntEngPolyline to do the drawing.
        ret = IntEngPolyline(SurfObj,
  			   dc->CombinedClip,
  			   &PenBrushObj->BrushObject,
  			   pts,
  			   Count,
  			   dc->w.ROPmode);
      }

      PENOBJ_UnlockPen( dc->w.hPen );
    }

    ExFreePool ( pts );
  }

  return ret;
}

BOOL FASTCALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count)
{
  BOOL ret = FALSE; // default to failure

  if(PATH_IsPathOpen(dc->w.path))
  {
    ret = PATH_PolylineTo(dc, pt, Count);
  }
  else /* do it using Polyline */
  {
    POINT *pts = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * (Count + 1), TAG_SHAPE);
    if ( pts )
    {
      pts[0].x = dc->w.CursPosX;
      pts[0].y = dc->w.CursPosY;
      memcpy( pts + 1, pt, sizeof(POINT) * Count);
      ret = IntGdiPolyline(dc, pts, Count + 1);
      ExFreePool(pts);
    }
  }
  if ( ret )
  {
    dc->w.CursPosX = pt[Count-1].x;
    dc->w.CursPosY = pt[Count-1].y;
  }
  
  return ret;
}

INT FASTCALL
IntGdiGetArcDirection(DC *dc)
{
  return dc->w.ArcDirection;
}

BOOL FASTCALL
IntGdiArc(DC  *dc,
          int LeftRect,
          int TopRect,
          int RightRect,
          int BottomRect,
          int XStartArc,
          int YStartArc,
          int XEndArc,
          int YEndArc)
{
  if(PATH_IsPathOpen(dc->w.path))
  {
    return PATH_Arc(dc, LeftRect, TopRect, RightRect, BottomRect,
                    XStartArc, YStartArc, XEndArc, YEndArc);
  }

  // FIXME
//   EngArc(dc, LeftRect, TopRect, RightRect, BottomRect, UNIMPLEMENTED
//          XStartArc, YStartArc, XEndArc, YEndArc);

  return TRUE;
}

BOOL FASTCALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   LPDWORD PolyPoints,
                   DWORD   Count)
{
  int i;
  LPPOINT pts;
  LPDWORD pc;
  BOOL ret = FALSE; // default to failure
  pts = pt;
  pc = PolyPoints;

  for (i = 0; i < Count; i++)
  {
    ret = IntGdiPolyline ( dc, pts, *pc );
    if (ret == FALSE)
    {
      return ret;
    }
    pts+=*pc++;
  }

  return ret;
}

/******************************************************************************/

BOOL
STDCALL
NtGdiAngleArc(HDC  hDC,
             int  X,
             int  Y,
             DWORD  Radius,
             FLOAT  StartAngle,
             FLOAT  SweepAngle)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiArc(HDC  hDC,
        int  LeftRect,
        int  TopRect,
        int  RightRect,
        int  BottomRect,
        int  XStartArc,
        int  YStartArc,
        int  XEndArc,
        int  YEndArc)
{
  DC *dc;
  BOOL Ret;
  
  dc = DC_LockDc (hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  Ret = IntGdiArc(dc,
                  LeftRect,
                  TopRect,
                  RightRect,
                  BottomRect,
                  XStartArc,
                  YStartArc,
                  XEndArc,
                  YEndArc);
  
  DC_UnlockDc( hDC );
  return Ret;
}

BOOL
STDCALL
NtGdiArcTo(HDC  hDC,
          int  LeftRect,
          int  TopRect,
          int  RightRect,
          int  BottomRect,
          int  XRadial1,
          int  YRadial1,
          int  XRadial2,
          int  YRadial2)
{
  BOOL result;
  DC *dc;
  
  dc = DC_LockDc (hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  // Line from current position to starting point of arc
  if ( !IntGdiLineTo(dc, XRadial1, YRadial1) )
  {
    DC_UnlockDc( hDC );
    return FALSE;
  }

  //dc = DC_LockDc(hDC);

  //if(!dc) return FALSE;

  // Then the arc is drawn.
  result = IntGdiArc(dc, LeftRect, TopRect, RightRect, BottomRect,
                     XRadial1, YRadial1, XRadial2, YRadial2);

  //DC_UnlockDc( hDC );

  // If no error occured, the current position is moved to the ending point of the arc.
  if(result)
    IntGdiMoveToEx(dc, XRadial2, YRadial2, NULL);
  
  DC_UnlockDc( hDC );

  return result;
}

INT
STDCALL
NtGdiGetArcDirection(HDC  hDC)
{
  PDC dc = DC_LockDc (hDC);
  int ret = 0; // default to failure

  if ( dc )
  {
    ret = IntGdiGetArcDirection ( dc );
    DC_UnlockDc( hDC );
  }
  else
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
  }

  return ret;
}

BOOL
STDCALL
NtGdiLineTo(HDC  hDC,
           int  XEnd,
           int  YEnd)
{
  DC *dc;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  Ret = IntGdiLineTo(dc, XEnd, YEnd);
  
  DC_UnlockDc(hDC);
  return Ret;
}

BOOL
STDCALL
NtGdiMoveToEx(HDC      hDC,
             int      X,
             int      Y,
             LPPOINT  Point)
{
  DC *dc;
  POINT SafePoint;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Point)
  {
    Status = MmCopyFromCaller(&SafePoint, Point, sizeof(POINT));
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  
  Ret = IntGdiMoveToEx(dc, X, Y, (Point ? &SafePoint : NULL));
  
  DC_UnlockDc(hDC);
  return Ret;
}

BOOL
STDCALL
NtGdiPolyBezier(HDC           hDC,
               CONST LPPOINT  pt,
               DWORD          Count)
{
  DC *dc;
  LPPOINT Safept;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Count > 0)
  {
    Safept = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * Count, TAG_BEZIER);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    Status = MmCopyFromCaller(Safept, pt, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  else
  {
    DC_UnlockDc(hDC);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGdiPolyBezier(dc, Safept, Count);
  
  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

BOOL
STDCALL
NtGdiPolyBezierTo(HDC  hDC,
                 CONST LPPOINT  pt,
                 DWORD  Count)
{
  DC *dc;
  LPPOINT Safept;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Count > 0)
  {
    Safept = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * Count, TAG_BEZIER);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    Status = MmCopyFromCaller(Safept, pt, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  else
  {
    DC_UnlockDc(hDC);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGdiPolyBezierTo(dc, Safept, Count);
  
  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

BOOL
STDCALL
NtGdiPolyDraw(HDC            hDC,
             CONST LPPOINT  pt,
             CONST LPBYTE   Types,
             int            Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiPolyline(HDC            hDC,
             CONST LPPOINT  pt,
             int            Count)
{
  DC *dc;
  LPPOINT Safept;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Count >= 2)
  {
    Safept = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * Count, TAG_SHAPE);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    Status = MmCopyFromCaller(Safept, pt, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  else
  {
    DC_UnlockDc(hDC);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGdiPolyline(dc, Safept, Count);

  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

BOOL
STDCALL
NtGdiPolylineTo(HDC            hDC,
               CONST LPPOINT  pt,
               DWORD          Count)
{
  DC *dc;
  LPPOINT Safept;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Count > 0)
  {
    Safept = ExAllocatePoolWithTag(PagedPool, sizeof(POINT) * Count, TAG_SHAPE);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    Status = MmCopyFromCaller(Safept, pt, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  else
  {
    DC_UnlockDc(hDC);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGdiPolylineTo(dc, Safept, Count);
  
  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

BOOL
STDCALL
NtGdiPolyPolyline(HDC            hDC,
                 CONST LPPOINT  pt,
                 CONST LPDWORD  PolyPoints,
                 DWORD          Count)
{
  DC *dc;
  LPPOINT Safept;
  LPDWORD SafePolyPoints;
  NTSTATUS Status;
  BOOL Ret;
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(Count > 0)
  {
    Safept = ExAllocatePoolWithTag(PagedPool, (sizeof(POINT) + sizeof(DWORD)) * Count, TAG_SHAPE);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    SafePolyPoints = (LPDWORD)&Safept[Count];
    
    Status = MmCopyFromCaller(Safept, pt, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      ExFreePool(Safept);
      SetLastNtError(Status);
      return FALSE;
    }
    Status = MmCopyFromCaller(SafePolyPoints, PolyPoints, sizeof(DWORD) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      ExFreePool(Safept);
      SetLastNtError(Status);
      return FALSE;
    }
  }
  else
  {
    DC_UnlockDc(hDC);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGdiPolyPolyline(dc, Safept, SafePolyPoints, Count);
  
  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

int
STDCALL
NtGdiSetArcDirection(HDC  hDC,
                    int  ArcDirection)
{
  PDC  dc;
  INT  nOldDirection = 0; // default to FAILURE

  dc = DC_LockDc (hDC);
  if ( !dc ) return 0;

  if ( ArcDirection == AD_COUNTERCLOCKWISE || ArcDirection == AD_CLOCKWISE )
  {
    nOldDirection = dc->w.ArcDirection;
    dc->w.ArcDirection = ArcDirection;
  }

  DC_UnlockDc( hDC );
  return nOldDirection;
}
/* EOF */
