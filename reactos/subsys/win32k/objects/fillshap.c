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
/* $Id: fillshap.c,v 1.20 2003/06/25 16:55:33 gvg Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <include/error.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/path.h>
#include <include/paint.h>
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

BOOL
STDCALL
W32kChord(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kEllipse(HDC  hDC,
                  int  LeftRect,
                  int  TopRect,
                  int  RightRect,
                  int  BottomRect)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kPie(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XRadial1,
              int  YRadial1,
              int  XRadial2,
              int  YRadial2)
{
  UNIMPLEMENTED;
}

#if 0

//ALTERNATE Selects alternate mode (fills the area between odd-numbered and even-numbered 
//polygon sides on each scan line). 
//When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and 
//even-numbered polygon sides on each scan line. That is, GDI fills the area between the 
//first and second side, between the third and fourth side, and so on. 
extern BOOL FillPolygon_ALTERNATE(SURFOBJ *SurfObj,
                                  PBRUSHOBJ BrushObj,
                                  MIX RopMode,
                                  CONST PPOINT Points,
                                  int Count,
                                  RECTL BoundRect);


//WINDING Selects winding mode (fills any region with a nonzero winding value). 
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value. 
//This value is defined as the number of times a pen used to draw the polygon would go around the region. 
//The direction of each edge of the polygon is important. 
extern BOOL FillPolygon_WINDING(SURFOBJ *SurfObj,
                                PBRUSHOBJ BrushObj,MIX RopMode,
                                CONST PPOINT Points,
                                int Count,
                                RECTL BoundRect);
#endif

//This implementation is blatantly ripped off from W32kRectangle
BOOL
STDCALL
W32kPolygon(HDC  hDC,
            CONST PPOINT UnsafePoints,
            int Count)
{
  DC *dc = DC_HandleToPtr(hDC);
  SURFOBJ *SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  PBRUSHOBJ OutBrushObj, FillBrushObj;
  BOOL ret;
  PRECTL RectBounds;
  PENOBJ *pen;
  RECTL DestRect;
  int CurrentPoint;
  PPOINT Points;
  NTSTATUS Status;

  DPRINT("In W32kPolygon()\n");
  
  if (NULL == dc || NULL == Points || Count < 2)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  /* Copy points from userspace to kernelspace */
  Points = ExAllocatePool(PagedPool, Count * sizeof(POINT));
  if (NULL == Points)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
  Status = MmCopyFromCaller(Points, UnsafePoints, Count * sizeof(POINT));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      ExFreePool(Points);
      return FALSE;
    }

  /* Convert to screen coordinates */
  for (CurrentPoint = 0; CurrentPoint < Count; CurrentPoint++)
    {
      Points[CurrentPoint].x += dc->w.DCOrgX;
      Points[CurrentPoint].y += dc->w.DCOrgY;
    }

  RectBounds = GDIOBJ_LockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  //ei not yet implemented ASSERT(RectBounds);

  if (PATH_IsPathOpen(dc->w.path)) 
    {	  
      ret = PATH_Polygon(hDC, Points, Count);
    } 
  else 
    {
      /* Get the current pen. */
      pen = (PENOBJ*) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
      ASSERT(pen);
      OutBrushObj = (PBRUSHOBJ) PenToBrushObj(dc, pen);
      GDIOBJ_UnlockObj(dc->w.hPen, GO_PEN_MAGIC);
	
      DestRect.left   = Points[0].x;
      DestRect.right  = Points[0].x;
      DestRect.top    = Points[0].y;
      DestRect.bottom = Points[0].y;
	  
      for (CurrentPoint = 1; CurrentPoint < Count; ++CurrentPoint)
	{
	  DestRect.left     = MIN(DestRect.left, Points[CurrentPoint].x);
	  DestRect.right    = MAX(DestRect.right, Points[CurrentPoint].x);
	  DestRect.top      = MIN(DestRect.top, Points[CurrentPoint].y);
	  DestRect.bottom   = MAX(DestRect.bottom, Points[CurrentPoint].y);
	}
	
      /* Now fill the polygon with the current brush. */
      FillBrushObj = (BRUSHOBJ*) GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
      /* determine the fill mode to fill the polygon. */
      if (WINDING == dc->w.polyFillMode)
	{
	  ret = FillPolygon_WINDING(SurfObj,  FillBrushObj, dc->w.ROPmode, Points, Count, DestRect);
	}
      else /* default */
	{
	  ret = FillPolygon_ALTERNATE(SurfObj,  FillBrushObj, dc->w.ROPmode, Points, Count, DestRect);
	}

      // Draw the Polygon Edges with the current pen
      for (CurrentPoint = 0; CurrentPoint < Count; ++CurrentPoint)
	{ 
          POINT To, From, Next;

	  /* Let CurrentPoint be i
	   * if i+1 > Count, Draw a line from Points[i] to Points[0]
	   * Draw a line from Points[i] to Points[i+1]
	   */
	  From = Points[CurrentPoint];
	  if (Count <= CurrentPoint + 1)
	    {
	      To = Points[0];
	    }
	  else
	    {
	      To = Points[CurrentPoint + 1];
	    }

	  /* Special handling of lower right corner of a rectangle. If we
	   * don't treat it specially, it will end up looking like this:
	   *
	   *                *
	   *                *
	   *                *
	   *       *********
	   */
	  if (3 < Count)
	    {
	      if (Count <= CurrentPoint + 2)
		{
		  Next = Points[CurrentPoint + 2 - Count];
		}
	      else
		{
		  Next = Points[CurrentPoint + 2];
		}
	      if (From.x == To.x &&
	          From.y <= To.y &&
	          To.y == Next.y &&
	          Next.x <= To.x)
		{
		  To.y++;
		}
	      else if (From.y == To.y &&
	               From.x <= To.x &&
	               To.x == Next.x &&
	               Next.y <= To.y)
		{
		  To.x++;
		}
	    }
	  DPRINT("Polygon Making line from (%d,%d) to (%d,%d)\n", From.x, From.y, To.x, To.y );
	  ret = IntEngLineTo(SurfObj,
	                     NULL, /* ClipObj */
	                     OutBrushObj,
	                     From.x, 
	                     From.y, 
	                     To.x, 
	                     To.y,
	                     &DestRect,
	                     dc->w.ROPmode); /* MIX */
		  
	}
      GDIOBJ_UnlockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
    }
  
  GDIOBJ_UnlockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  DC_ReleasePtr(hDC);
  ExFreePool(Points);

  return ret;
}


BOOL
STDCALL
W32kPolyPolygon(HDC  hDC,
                      CONST LPPOINT  Points,
                      CONST LPINT  PolyCounts,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kRectangle(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect)
{
  DC		*dc = DC_HandleToPtr(hDC);
  SURFOBJ	*SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  PBRUSHOBJ	BrushObj;
  BOOL ret;
  PRECTL	RectBounds;
  PENOBJ * pen;
  RECTL        DestRect;

  if(!dc)
   return FALSE;

  RectBounds = GDIOBJ_LockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  //ei not yet implemented ASSERT(RectBounds);

  if(PATH_IsPathOpen(dc->w.path)) {
    ret = PATH_Rectangle(hDC, LeftRect, TopRect, RightRect, BottomRect);
  } else {
    // Draw the rectangle with the current pen
    pen = (PENOBJ*) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
    ASSERT(pen);
    BrushObj = (PBRUSHOBJ)PenToBrushObj(dc, pen);
    GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC );

    LeftRect += dc->w.DCOrgX;
    RightRect += dc->w.DCOrgX;
    TopRect += dc->w.DCOrgY;
    BottomRect += dc->w.DCOrgY;

    ret = IntEngLineTo(SurfObj,
                       NULL, // ClipObj,
                       BrushObj,
                       LeftRect, TopRect, RightRect, TopRect,
                       RectBounds, // Bounding rectangle
                       dc->w.ROPmode); // MIX

    ret = IntEngLineTo(SurfObj,
                       NULL, // ClipObj,
                       BrushObj,
                       RightRect, TopRect, RightRect, BottomRect,
                       RectBounds, // Bounding rectangle
                       dc->w.ROPmode); // MIX

    ret = IntEngLineTo(SurfObj,
                       NULL, // ClipObj,
                       BrushObj,
                       LeftRect, BottomRect, RightRect, BottomRect,
                       RectBounds, // Bounding rectangle
                       dc->w.ROPmode); // MIX

    ret = IntEngLineTo(SurfObj,
                       NULL, // ClipObj,
                       BrushObj,
                       LeftRect, TopRect, LeftRect, BottomRect,
                       RectBounds, // Bounding rectangle
                       dc->w.ROPmode); // MIX */

    // FIXME: BrushObj is obtained above; decide which one is correct
    BrushObj = (BRUSHOBJ*) GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);

    if (BrushObj)
    {
      if (BrushObj->logbrush.lbStyle != BS_NULL)
        {
          DestRect.left = LeftRect + 1;
          DestRect.right = RightRect - 1;
          DestRect.top = TopRect + 1;
          DestRect.bottom = BottomRect - 1;
          ret = EngBitBlt(SurfObj,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &DestRect,
                          NULL,
                          NULL,
                          BrushObj,
                          NULL,
                          PATCOPY);
        }
    }
    GDIOBJ_UnlockObj( dc->w.hBrush, GO_BRUSH_MAGIC );
  }

// FIXME: Move current position in DC?
  GDIOBJ_UnlockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  DC_ReleasePtr( hDC );
  return TRUE;
}

BOOL
STDCALL
W32kRoundRect(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect,
                    int  Width,
                    int  Height)
{
  UNIMPLEMENTED;
}
/* EOF */
