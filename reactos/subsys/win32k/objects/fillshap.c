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
/* $Id: fillshap.c,v 1.18 2003/05/18 17:16:18 ea Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/path.h>
#include <include/paint.h>

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
                                  RECTL BoundRect,
                                  int OrigX,
                                  int OrigY);


//WINDING Selects winding mode (fills any region with a nonzero winding value). 
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value. 
//This value is defined as the number of times a pen used to draw the polygon would go around the region. 
//The direction of each edge of the polygon is important. 
extern BOOL FillPolygon_WINDING(SURFOBJ *SurfObj,
                                PBRUSHOBJ BrushObj,MIX RopMode,
                                CONST PPOINT Points,
                                int Count,
                                RECTL BoundRect,
                                int OrigX,
                                int OrigY);
#endif

//This implementation is blatantly ripped off from W32kRectangle
BOOL
STDCALL
W32kPolygon(HDC  hDC,
            CONST PPOINT  Points,
            int  Count)
{
  DC		*dc = DC_HandleToPtr(hDC);
  SURFOBJ	*SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  PBRUSHOBJ	OutBrushObj, FillBrushObj;
  BOOL      ret;
  PRECTL	RectBounds;
  PENOBJ    *pen;
  RECTL     DestRect;
  int       CurrentPoint;

  DPRINT("In W32kPolygon()\n");
  
  if(0 == dc)
   return FALSE;

  if(0 == Points)
   return FALSE;

  if (2 > Count)
   return FALSE;

  RectBounds = GDIOBJ_LockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  //ei not yet implemented ASSERT(RectBounds);
	
  DestRect.bottom   = Points[0].y + dc->w.DCOrgY + 1;
  DestRect.top      = Points[0].y + dc->w.DCOrgY;
  DestRect.right    = Points[0].y + dc->w.DCOrgX;
  DestRect.left     = Points[0].y + dc->w.DCOrgX + 1;
  


  if(PATH_IsPathOpen(dc->w.path)) 
  {	  
      ret = PATH_Polygon(hDC, Points, Count);
  } 
  else 
  {
	  //Get the current pen.
	  pen = (PENOBJ*) GDIOBJ_LockObj(dc->w.hPen, GO_PEN_MAGIC);
      ASSERT(pen);
      OutBrushObj = (PBRUSHOBJ)PenToBrushObj(dc, pen);
      GDIOBJ_UnlockObj( dc->w.hPen, GO_PEN_MAGIC );
	  
      // Draw the Polygon Edges with the current pen
	  for (CurrentPoint = 0; CurrentPoint < Count; ++CurrentPoint)
      {
		  DestRect.bottom   = MAX(DestRect.bottom, Points[CurrentPoint].y + dc->w.DCOrgY);
		  DestRect.top      = MIN(DestRect.top, Points[CurrentPoint].y + dc->w.DCOrgY);
		  DestRect.right    = MAX(DestRect.right, Points[CurrentPoint].y + dc->w.DCOrgX);
		  DestRect.left     = MIN(DestRect.left, Points[CurrentPoint].y + dc->w.DCOrgX);
	  }//for
	
	  //Now fill the polygon with the current brush.
	  FillBrushObj = (BRUSHOBJ*) GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
	  // determine the fill mode to fill the polygon.
	  if (dc->w.polyFillMode == WINDING)
		  ret = FillPolygon_WINDING(SurfObj,  FillBrushObj, dc->w.ROPmode, Points, Count, DestRect, dc->w.DCOrgX, dc->w.DCOrgY);
	  else//default
		  ret = FillPolygon_ALTERNATE(SurfObj,  FillBrushObj, dc->w.ROPmode, Points, Count, DestRect, dc->w.DCOrgX, dc->w.DCOrgY);
      // Draw the Polygon Edges with the current pen
	  for (CurrentPoint = 0; CurrentPoint < Count; ++CurrentPoint)
	  { 
        POINT To,From;
	    //Let CurrentPoint be i
	    //if i+1 > Count, Draw a line from Points[i] to Points[0]
		//Draw a line from Points[i] to Points[i+1] 
		if (CurrentPoint + 1 >= Count)
		{
		  To = Points[CurrentPoint];
		  From = Points[0];
		}
		else
		{
		  From = Points[CurrentPoint];
		  To = Points[CurrentPoint + 1];
		}
		DPRINT("Polygon Making line from (%d,%d) to (%d,%d)\n", From.x, From.y, To.x, To.y );
		ret = EngLineTo(SurfObj,
                        NULL, // ClipObj,
                        OutBrushObj,
                        From.x + dc->w.DCOrgX, 
		      	        From.y + dc->w.DCOrgY, 
					    To.x + dc->w.DCOrgX, 
					    To.y + dc->w.DCOrgY,
                        RectBounds, // Bounding rectangle
                        dc->w.ROPmode); // MIX
		  
	  }//for
      GDIOBJ_UnlockObj( dc->w.hBrush, GO_BRUSH_MAGIC );
  }// else
  
  GDIOBJ_UnlockObj(dc->w.hGCClipRgn, GO_REGION_MAGIC);
  DC_ReleasePtr( hDC );
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
