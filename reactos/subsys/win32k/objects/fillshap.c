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
/* $Id: fillshap.c,v 1.42 2004/03/03 04:09:20 royce Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <win32k/region.h>
#include <include/error.h>
#include <include/object.h>
#include <include/inteng.h>
#include <include/path.h>
#include <include/paint.h>
#include <include/palette.h>
#include <include/eng.h>
#include <include/intgdi.h>
#include <include/tags.h>
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

/*
 * a couple macros to fill a single pixel or a line
 */
#define PUTPIXEL(x,y,brushObj)      \
  ret = ret && IntEngLineTo(SurfObj,  \
       dc->CombinedClip,              \
       brushObj,                      \
       x, y, (x)+1, y,                \
       &RectBounds,                   \
       dc->w.ROPmode);

#define PUTLINE(x1,y1,x2,y2,brushObj)  \
  ret = ret && IntEngLineTo(SurfObj,  \
       dc->CombinedClip,              \
       brushObj,                      \
       x1, y1, x2, y2,                \
       &RectBounds,                   \
       dc->w.ROPmode);

BOOL FASTCALL
IntGdiPolygon(PDC    dc,
              PPOINT UnsafePoints,
              int    Count)
{
  SURFOBJ *SurfObj;
  BRUSHOBJ PenBrushObj, *FillBrushObj;
  BOOL ret = FALSE; // default to failure
  PRECTL RectBounds;
  RECTL DestRect;
  int CurrentPoint;

  ASSERT(dc); // caller's responsibility to pass a valid dc

  if ( NULL == UnsafePoints || Count < 2 )
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  ASSERT(SurfObj);

      /* Convert to screen coordinates */
      for (CurrentPoint = 0; CurrentPoint < Count; CurrentPoint++)
	{
	  UnsafePoints[CurrentPoint].x += dc->w.DCOrgX;
	  UnsafePoints[CurrentPoint].y += dc->w.DCOrgY;
	}

      RectBounds = (PRECTL) RGNDATA_LockRgn(dc->w.hGCClipRgn);
      //ei not yet implemented ASSERT(RectBounds);

      if (PATH_IsPathOpen(dc->w.path)) 
	ret = PATH_Polygon(dc, UnsafePoints, Count );
      else
      {
	DestRect.left   = UnsafePoints[0].x;
	DestRect.right  = UnsafePoints[0].x;
	DestRect.top    = UnsafePoints[0].y;
	DestRect.bottom = UnsafePoints[0].y;

	for (CurrentPoint = 1; CurrentPoint < Count; ++CurrentPoint)
	{
	  DestRect.left     = MIN(DestRect.left, UnsafePoints[CurrentPoint].x);
	  DestRect.right    = MAX(DestRect.right, UnsafePoints[CurrentPoint].x);
	  DestRect.top      = MIN(DestRect.top, UnsafePoints[CurrentPoint].y);
	  DestRect.bottom   = MAX(DestRect.bottom, UnsafePoints[CurrentPoint].y);
	}

#if 1
	/* Now fill the polygon with the current brush. */
	FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
	ASSERT(FillBrushObj);
	if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
	  ret = FillPolygon ( dc, SurfObj, FillBrushObj, dc->w.ROPmode, UnsafePoints, Count, DestRect );
	BRUSHOBJ_UnlockBrush(dc->w.hBrush);
#endif

	/* make BRUSHOBJ from current pen. */
	HPenToBrushObj ( &PenBrushObj, dc->w.hPen );

	// Draw the Polygon Edges with the current pen ( if not a NULL pen )
	if ( PenBrushObj.logbrush.lbStyle != BS_NULL )
	{
	  for ( CurrentPoint = 0; CurrentPoint < Count; ++CurrentPoint )
	  {
	    POINT To, From; //, Next;

	    /* Let CurrentPoint be i
	     * if i+1 > Count, Draw a line from Points[i] to Points[0]
	     * Draw a line from Points[i] to Points[i+1]
	     */
	    From = UnsafePoints[CurrentPoint];
	    if (Count <= CurrentPoint + 1)
	      To = UnsafePoints[0];
	    else
	      To = UnsafePoints[CurrentPoint + 1];

	    //DPRINT("Polygon Making line from (%d,%d) to (%d,%d)\n", From.x, From.y, To.x, To.y );
	    ret = IntEngLineTo(SurfObj,
			       dc->CombinedClip,
			       &PenBrushObj,
			       From.x,
			       From.y,
			       To.x,
			       To.y,
			       &DestRect,
			       dc->w.ROPmode); /* MIX */
	  }
	}
#if 0
	/* Now fill the polygon with the current brush. */
	FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
	ASSERT(FillBrushObj);
	if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
	  ret = FillPolygon ( dc, SurfObj, FillBrushObj, dc->w.ROPmode, UnsafePoints, Count, DestRect );
	BRUSHOBJ_UnlockBrush(dc->w.hBrush);
#endif
      }

      RGNDATA_UnlockRgn(dc->w.hGCClipRgn);
  
  return ret;
}

BOOL FASTCALL
IntGdiPolyPolygon(DC      *dc,
                  LPPOINT Points,
                  LPINT   PolyCounts,
                  int     Count)
{
  int i;
  LPPOINT pt;
  LPINT pc;
  BOOL ret = FALSE; // default to failure

  pt = Points;
  pc = PolyCounts;
  
  for (i=0;i<Count;i++)
  {
    ret = IntGdiPolygon ( dc, pt, *pc );
    if (ret == FALSE)
    {
      return ret;
    }
    pt+=*pc++;
  }
  
  return ret;
}

/******************************************************************************/

BOOL
STDCALL
NtGdiChord(HDC  hDC,
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

/*
 * NtGdiEllipse
 * 
 * Author
 *    Filip Navara
 *
 * Remarks
 *    This function uses optimized Bresenham's ellipse algorithm. It draws
 *    four lines of the ellipse in one pass.
 *
 * Todo
 *    Make it look like a Windows ellipse.
 */

BOOL STDCALL
NtGdiEllipse(
   HDC hDC,
   int nLeftRect,
   int nTopRect,
   int nRightRect,
   int nBottomRect)
{
   int ix, iy;
   int A, B, C, D;
   int da, db;
   int NewA, NewB, NewC, NewD;
   int nx, ny;
   int CenterX, CenterY;
   int RadiusX, RadiusY;
   int Temp;
   BRUSHOBJ PenBrush;
   PBRUSHOBJ FillBrush;
   PSURFOBJ SurfObj;
   RECTL RectBounds;
   PDC dc;
   BOOL ret = TRUE, Cond1, Cond2;

   /*
    * Check the parameters.
    */

   if (nRightRect <= nLeftRect || nBottomRect <= nTopRect)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   /*
    * Get pointers to all necessary GDI objects.
    */

   dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   FillBrush = BRUSHOBJ_LockBrush(dc->w.hBrush);
   if (NULL == FillBrush)
   {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return FALSE;
   }

   SurfObj = (PSURFOBJ)AccessUserObject((ULONG)dc->Surface);
   HPenToBrushObj(&PenBrush, dc->w.hPen);

   nLeftRect += dc->w.DCOrgX;
   nRightRect += dc->w.DCOrgX - 1;
   nTopRect += dc->w.DCOrgY;
   nBottomRect += dc->w.DCOrgY - 1;

   RadiusX = max((nRightRect - nLeftRect) >> 1, 1);
   RadiusY = max((nBottomRect - nTopRect) >> 1, 1);
   CenterX = nLeftRect + RadiusX;
   CenterY = nTopRect + RadiusY;

   RectBounds.left = nLeftRect;
   RectBounds.right = nRightRect;
   RectBounds.top = nTopRect;
   RectBounds.bottom = nBottomRect;

   if (RadiusX > RadiusY)
   {
      nx = RadiusX;
      ny = RadiusY;
   }
   else
   {
      nx = RadiusY;
      ny = RadiusX;
   }
   
   da = -1;
   db = 0xFFFF;
   ix = 0; 
   iy = nx * 64;
   NewA = 0; 
   NewB = (iy + 32) >> 6;
   NewC = 0; 
   NewD = (NewB * ny) / nx;

   do {
      A = NewA; 
      B = NewB; 
      C = NewC; 
      D = NewD;

      ix += iy / nx;
      iy -= ix / nx;
      NewA = (ix + 32) >> 6; 
      NewB = (iy + 32) >> 6;
      NewC = (NewA * ny) / nx; 
      NewD = (NewB * ny) / nx;

      if (RadiusX > RadiusY)
      {
         Temp = A; A = C; C = Temp;
         Temp = B; B = D; D = Temp;
         Cond1 = ((C != NewA) || (B != NewD)) && (NewC <= NewD);
         Cond2 = ((D != NewB) || (A != NewC)) && (NewC <= B);
      }
      else
      {
         Cond1 = ((C != NewC) || (B != NewB)) && (NewA <= NewB);
         Cond2 = ((D != NewD) || (A != NewA)) && (NewA <= B);
      }

      /*
       * Draw the lines going from inner to outer (+ mirrored).
       */
      
      if ((A > da) && (A < db))
      {
         PUTLINE(CenterX - D, CenterY + A, CenterX + D, CenterY + A, FillBrush);
         if (A)
         {
            PUTLINE(CenterX - D, CenterY - A, CenterX + D, CenterY - A, FillBrush);
         }
         da = A;
      }

      /*
       * Draw the lines going from outer to inner (+ mirrored).
       */
      
      if ((B < db) && (B > da))
      { 
         PUTLINE(CenterX - C, CenterY + B, CenterX + C, CenterY + B, FillBrush);
         PUTLINE(CenterX - C, CenterY - B, CenterX + C, CenterY - B, FillBrush);
         db = B;
      }

      /*
       * Draw the pixels on the margin.
       */

      if (Cond1)
      {
         PUTPIXEL(CenterX + C, CenterY + B, &PenBrush);
         if (C)
            PUTPIXEL(CenterX - C, CenterY + B, &PenBrush);
         if (B)
         {
            PUTPIXEL(CenterX + C, CenterY - B, &PenBrush);
            if (C)
               PUTPIXEL(CenterX - C, CenterY - B, &PenBrush);
         }
      }

      if (Cond2)
      {
         PUTPIXEL(CenterX + D, CenterY + A, &PenBrush);
         if (D)
            PUTPIXEL(CenterX - D, CenterY + A, &PenBrush);
         if (A)
         {
            PUTPIXEL(CenterX + D, CenterY - A, &PenBrush);
            if (D)
               PUTPIXEL(CenterX - D, CenterY - A, &PenBrush);
         }
      }
   } while (B > A);

   BRUSHOBJ_UnlockBrush(dc->w.hBrush);
   DC_UnlockDc(hDC);

   return ret;
}

typedef struct tagSHAPEPOINT
{
  int X;
  int Y;
  int Type;
} SHAPEPOINT, *PSHAPEPOINT;

#define SHAPEPOINT_TYPE_CIRCLE     'C'
#define SHAPEPOINT_TYPE_LINE_RIGHT 'R' /* Fill at right side of line */
#define SHAPEPOINT_TYPE_LINE_LEFT  'L' /* Fill at left side of line */

#define SETPOINT(x, y, type) \
  ShapePoints[*PointCount].X = (x); \
  ShapePoints[*PointCount].Y = (y); \
  ShapePoints[*PointCount].Type = (type); \
  (*PointCount)++

#define SETCIRCLEPOINT(x, y) \
  SETPOINT(x, y, SHAPEPOINT_TYPE_CIRCLE)

#ifdef TODO
STATIC VOID
FASTCALL
CirclePoints(UINT *PointCount, PSHAPEPOINT ShapePoints, int Left, int Top,
             int Right, int Bottom)
{
  int X, X18, X27, X36, X45;
  int Y, Y14, Y23, Y58, Y67;
  int d, Radius;
  BOOL Even;

  Even = (0 == (Right - Left) % 2);
  Right--;
  Bottom--;
  Radius = (Right - Left) / 2;

  if (Even)
    {
      X = 0;
      Y = Radius;
      d = 2 - Radius;
      X18 = Right;
      X27 = (Left + Right) / 2 + 1;
      X36 = (Left + Right) / 2;
      X45 = Left;
      Y14 = Top + Radius;
      Y23 = Top;
      Y58 = Top + Radius + 1;
      Y67 = Top + (Right - Left);
      ShapePoints[*PointCount].X = X27;
      SETCIRCLEPOINT(X27, Y23);
      SETCIRCLEPOINT(X36, Y23);
      SETCIRCLEPOINT(X18, Y14);
      SETCIRCLEPOINT(X45, Y14);
      SETCIRCLEPOINT(X18, Y58);
      SETCIRCLEPOINT(X45, Y58);
      SETCIRCLEPOINT(X27, Y67);
      SETCIRCLEPOINT(X36, Y67);
    }
  else
    {
      X = 0;
      Y = Radius;
      d = 1 - Radius;
      X18 = Right;
      X27 = (Left + Right) / 2;
      X36 = (Left + Right) / 2;
      X45 = Left;
      Y14 = Top + Radius;
      Y23 = Top;
      Y58 = Top + Radius;
      Y67 = Top + (Right - Left);
      SETCIRCLEPOINT(X27, Y23);
      SETCIRCLEPOINT(X45, Y14);
      SETCIRCLEPOINT(X18, Y58);
      SETCIRCLEPOINT(X27, Y67);
    }

  while (X < Y)
    {
      if (d < 0)
	{
	  d += 2 * X + (Even ? 4 : 3);

	  X27++;
	  X36--;
	  Y14--;
	  Y58++;
	}
      else
	{
	  d += 2 * (X - Y) + 5;
	  Y--;

	  Y23++;
	  Y67--;
	  X18--;
	  X45++;
	  X27++;
	  X36--;
	  Y14--;
	  Y58++;
	}
      X++;

      SETCIRCLEPOINT(X27, Y23);
      SETCIRCLEPOINT(X36, Y23);
      SETCIRCLEPOINT(X18, Y14);
      SETCIRCLEPOINT(X45, Y14);
      SETCIRCLEPOINT(X18, Y58);
      SETCIRCLEPOINT(X45, Y58);
      SETCIRCLEPOINT(X27, Y67);
      SETCIRCLEPOINT(X36, Y67);
    }
}

STATIC VOID
LinePoints(UINT *PointCount, PSHAPEPOINT ShapePoints, int Left, int Top,
           int Right, int Bottom, int XTo, int YTo, BOOL Start)
{
  LONG x, y, deltax, deltay, i, xchange, ychange, error;
  int Type;

  x = (Right + Left) / 2;
  y = (Bottom + Top) / 2;
  deltax = XTo - x;
  deltay = YTo - y;

  if (deltax < 0)
    {
      xchange = -1;
      deltax = - deltax;
      x--;
    }
  else
    {
      xchange = 1;
    }

  if (deltay < 0)
    {
      ychange = -1;
      deltay = - deltay;
      y--;
      Type = (Start ? SHAPEPOINT_TYPE_LINE_LEFT : SHAPEPOINT_TYPE_LINE_RIGHT);
    }
  else
    {
      ychange = 1;
      Type = (Start ? SHAPEPOINT_TYPE_LINE_RIGHT : SHAPEPOINT_TYPE_LINE_LEFT);
    }

  if (y == YTo)
    {
      for (i = x; i <= XTo; i++)
	{
	  SETPOINT(i, y, Type);
	}
    }
  else if (x == XTo)
    {
      for (i = y; i <= YTo; i++)
	{
	  SETPOINT(x, i, Type);
	}
    }
  else
    {
      error = 0;

      if (deltax < deltay)
	{
	  for (i = 0; i < deltay; i++)
	    {
	      SETPOINT(x, y, Type);
	      y = y + ychange;
	      error = error + deltax;

	      if (deltay <= error)
		{
		  x = x + xchange;
		  error = error - deltay;
		}
	    }
	}
      else
 	{
	  for (i = 0; i < deltax; i++)
	    {
	      SETPOINT(x, y, Type);
	      x = x + xchange;
	      error = error + deltay;
	      if (deltax <= error)
		{
		  y = y + ychange;
		  error = error - deltax;
		}
	    }
	}
    }
}

STATIC int
CDECL
CompareShapePoints(const void *pv1, const void *pv2)
{
  if (((const PSHAPEPOINT) pv1)->Y < ((const PSHAPEPOINT) pv2)->Y)
    {
      return -1;
    }
  else if (((const PSHAPEPOINT) pv2)->Y < ((const PSHAPEPOINT) pv1)->Y)
    {
      return +1;
    }
  else if (((const PSHAPEPOINT) pv1)->X < ((const PSHAPEPOINT) pv2)->X)
    {
      return -1;
    }
  else if (((const PSHAPEPOINT) pv2)->X < ((const PSHAPEPOINT) pv1)->X)
    {
      return +1;
    }
  else
    {
      return 0;
    }
}
#endif


BOOL
STDCALL
NtGdiPie(HDC  hDC,
              int  Left,
              int  Top,
              int  Right,
              int  Bottom,
              int  XRadialStart,
              int  YRadialStart,
              int  XRadialEnd,
              int  YRadialEnd)
{
#ifdef TODO
  PDC dc;
  RECTL RectBounds;
  PSURFOBJ SurfObj;
  BRUSHOBJ PenBrushObj;
  PBRUSHOBJ FillBrushObj;
  PSHAPEPOINT ShapePoints;
  UINT Point, PointCount;
  BOOL ret = TRUE;
  int Y, CircleStart, CircleEnd, LineStart, LineEnd;
  BOOL FullFill;

  if (Right <= Left || Bottom <= Top)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  if (Right - Left != Bottom - Top)
    {
      UNIMPLEMENTED;
    }

  dc = DC_LockDc ( hDC );
  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
  if (NULL == FillBrushObj)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return FALSE;
    }

  Left += dc->w.DCOrgX;
  Right += dc->w.DCOrgX;
  Top += dc->w.DCOrgY;
  Bottom += dc->w.DCOrgY;
  XRadialStart += dc->w.DCOrgX;
  YRadialStart += dc->w.DCOrgY;
  XRadialEnd += dc->w.DCOrgX;
  YRadialEnd += dc->w.DCOrgY;

  RectBounds.left = Left;
  RectBounds.right = Right;
  RectBounds.top = Top;
  RectBounds.bottom = Bottom;

  SurfObj = (PSURFOBJ) AccessUserObject((ULONG)dc->Surface);
  HPenToBrushObj(&PenBrushObj, dc->w.hPen);

  /* Number of points for the circle is 4 * sqrt(2) * Radius, start
     and end line have at most Radius points, so allocate at least
     that much */
  ShapePoints = ExAllocatePoolWithTag(PagedPool, 8 * (Right - Left + 1) / 2 * sizeof(SHAPEPOINT), TAG_SHAPE);
  if (NULL == ShapePoints)
    {
      BRUSHOBJ_UnlockBrush(dc->w.hBrush);
      DC_UnlockDc(hDC);

      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (Left == Right)
    {
      PUTPIXEL(Left, Top, &PenBrushObj);
      BRUSHOBJ_UnlockBrush(dc->w.hBrush);
      DC_UnlockDc(hDC);

      return ret;
    }

  PointCount = 0;
  CirclePoints(&PointCount, ShapePoints, Left, Top, Right, Bottom);
  LinePoints(&PointCount, ShapePoints, Left, Top, Right, Bottom,
             XRadialStart, YRadialStart, TRUE);
  LinePoints(&PointCount, ShapePoints, Left, Top, Right, Bottom,
             XRadialEnd, YRadialEnd, FALSE);
  ASSERT(PointCount <= 8 * (Right - Left + 1) / 2);
  EngSort((PBYTE) ShapePoints, sizeof(SHAPEPOINT), PointCount, CompareShapePoints);

  FullFill = TRUE;
  Point = 0;
  while (Point < PointCount)
    {
      Y = ShapePoints[Point].Y;

      /* Skip any line pixels before circle */
      while (Point < PointCount && ShapePoints[Point].Y == Y
             && SHAPEPOINT_TYPE_CIRCLE != ShapePoints[Point].Type)
	{
	  Point++;
	}

      /* Handle left side of circle */
      if (Point < PointCount && ShapePoints[Point].Y == Y)
	{
	  CircleStart = ShapePoints[Point].X;
	  Point++;
	  while (Point < PointCount && ShapePoints[Point].Y == Y
	         && ShapePoints[Point].X == ShapePoints[Point - 1].X + 1
	         && SHAPEPOINT_TYPE_CIRCLE == ShapePoints[Point].Type)
	    {
	      Point++;
	    }
	  CircleEnd = ShapePoints[Point - 1].X;

	  PUTLINE(CircleStart, Y, CircleEnd + 1, Y, &PenBrushObj);
	}

      /* Handle line(s) (max 2) inside the circle */
      while (Point < PointCount && ShapePoints[Point].Y == Y
             && SHAPEPOINT_TYPE_CIRCLE != ShapePoints[Point].Type)
	{
	  LineStart = ShapePoints[Point].X;
	  Point++;
	  while (Point < PointCount && ShapePoints[Point].Y == Y
	         && ShapePoints[Point].X == ShapePoints[Point - 1].X + 1
	         && ShapePoints[Point].Type == ShapePoints[Point - 1].Type)
	    {
	      Point++;
	    }
	  LineEnd = ShapePoints[Point - 1].X;

	  PUTLINE(LineStart, Y, LineEnd + 1, Y, &PenBrushObj);
	}

      /* Handle right side of circle */
      while (Point < PointCount && ShapePoints[Point].Y == Y
             && SHAPEPOINT_TYPE_CIRCLE == ShapePoints[Point].Type)
	{
	  CircleStart = ShapePoints[Point].X;
	  Point++;
	  while (Point < PointCount && ShapePoints[Point].Y == Y
	         && ShapePoints[Point].X == ShapePoints[Point - 1].X + 1
	         && SHAPEPOINT_TYPE_CIRCLE == ShapePoints[Point].Type)
	    {
	      Point++;
	    }
	  CircleEnd = ShapePoints[Point - 1].X;

	  PUTLINE(CircleStart, Y, CircleEnd + 1, Y, &PenBrushObj);
	}

      /* Skip any line pixels after circle */
      while (Point < PointCount && ShapePoints[Point].Y == Y)
	{
	  Point++;
	}
    }

  ExFreePool(ShapePoints);
  BRUSHOBJ_UnlockBrush(dc->w.hBrush);
  DC_UnlockDc(hDC);

  return ret;
#else
return TRUE;
#endif
}

#if 0

//When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and 
//even-numbered polygon sides on each scan line. That is, GDI fills the area between the 
//first and second side, between the third and fourth side, and so on. 

//WINDING Selects winding mode (fills any region with a nonzero winding value). 
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value.
//This value is defined as the number of times a pen used to draw the polygon would go around the region.
//The direction of each edge of the polygon is important. 

extern BOOL FillPolygon(PDC dc,
				  SURFOBJ *SurfObj,
				  PBRUSHOBJ BrushObj,
				  MIX RopMode,
				  CONST PPOINT Points,
				  int Count,
				  RECTL BoundRect);

#endif

//This implementation is blatantly ripped off from NtGdiRectangle
BOOL
STDCALL
NtGdiPolygon(HDC          hDC,
             CONST PPOINT UnsafePoints,
             int          Count)
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
    Safept = ExAllocatePoolWithTag(NonPagedPool, sizeof(POINT) * Count, TAG_SHAPE);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    Status = MmCopyFromCaller(Safept, UnsafePoints, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Safept);
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
  
  Ret = IntGdiPolygon(dc, Safept, Count);

  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}


BOOL
STDCALL
NtGdiPolyPolygon(HDC           hDC,
                 CONST LPPOINT Points,
                 CONST LPINT   PolyCounts,
                 int           Count)
{
  DC *dc;
  LPPOINT Safept;
  LPINT SafePolyPoints;
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
    Safept = ExAllocatePoolWithTag(NonPagedPool, (sizeof(POINT) + sizeof(INT)) * Count, TAG_SHAPE);
    if(!Safept)
    {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    
    SafePolyPoints = (LPINT)&Safept[Count];
    
    Status = MmCopyFromCaller(Safept, Points, sizeof(POINT) * Count);
    if(!NT_SUCCESS(Status))
    {
      DC_UnlockDc(hDC);
      ExFreePool(Safept);
      SetLastNtError(Status);
      return FALSE;
    }
    Status = MmCopyFromCaller(SafePolyPoints, PolyCounts, sizeof(INT) * Count);
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
  
  Ret = IntGdiPolyPolygon(dc, Safept, SafePolyPoints, Count);
  
  ExFreePool(Safept);
  DC_UnlockDc(hDC);
  
  return Ret;
}

BOOL
FASTCALL
IntRectangle(PDC dc,
	     int LeftRect,
	     int TopRect,
	     int RightRect,
	     int BottomRect)
{
  SURFOBJ   *SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  BRUSHOBJ   PenBrushObj, *FillBrushObj;
  BOOL       ret = FALSE; // default to failure
  RECTL      DestRect;

  ASSERT ( dc ); // caller's responsibility to set this up

  if ( PATH_IsPathOpen(dc->w.path) )
  {
    ret = PATH_Rectangle ( dc, LeftRect, TopRect, RightRect, BottomRect );
  }
  else
  {
    LeftRect   += dc->w.DCOrgX;
    RightRect  += dc->w.DCOrgX - 1;
    TopRect    += dc->w.DCOrgY;
    BottomRect += dc->w.DCOrgY - 1;

    DestRect.left = LeftRect;
    DestRect.right = RightRect;
    DestRect.top = TopRect;
    DestRect.bottom = BottomRect;

    FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);

    if ( FillBrushObj )
    {
      if ( FillBrushObj->logbrush.lbStyle != BS_NULL )
      {
	ret = IntEngBitBlt(SurfObj,
                           NULL,
                           NULL,
                           dc->CombinedClip,
                           NULL,
                           &DestRect,
                           NULL,
                           NULL,
                           FillBrushObj,
                           NULL,
                           PATCOPY);
      }
    }

    BRUSHOBJ_UnlockBrush(dc->w.hBrush);

    /* make BRUSHOBJ from current pen. */
    HPenToBrushObj ( &PenBrushObj, dc->w.hPen );

    // Draw the rectangle with the current pen

    ret = TRUE; // change default to success

    if ( PenBrushObj.logbrush.lbStyle != BS_NULL )
    {
      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 LeftRect, TopRect, RightRect, TopRect,
			 &DestRect, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 RightRect, TopRect, RightRect, BottomRect,
			 &DestRect, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 RightRect, BottomRect, LeftRect, BottomRect,
			 &DestRect, // Bounding rectangle
			 dc->w.ROPmode); // MIX

      ret = ret && IntEngLineTo(SurfObj,
			 dc->CombinedClip,
			 &PenBrushObj,
			 LeftRect, BottomRect, LeftRect, TopRect,
			 &DestRect, // Bounding rectangle
			 dc->w.ROPmode); // MIX */
    }
  }

  /* Move current position in DC?
     MSDN: The current position is neither used nor updated by Rectangle. */

  return TRUE;
}

BOOL
STDCALL
NtGdiRectangle(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect)
{
  DC   *dc;
  BOOL ret; // default to failure
  
  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  ret = IntRectangle ( dc, LeftRect, TopRect, RightRect, BottomRect );
  DC_UnlockDc ( hDC );

  return ret;
}


BOOL
FASTCALL
IntRoundRect(
	PDC  dc,
	int  left,
	int  top,
	int  right,
	int  bottom,
	int  xradius,
	int  yradius)
{
  SURFOBJ   *SurfObj;
  BRUSHOBJ   PenBrush, *PenBrushObj, *FillBrushObj;
  RECTL      RectBounds;
  int i, col, row, width, height, x1, x1start, x2, x2start, y1, y2;
  //float aspect_square;
  long a_square, b_square,
    two_a_square, two_b_square,
    four_a_square, four_b_square,
    d, dinc, ddec;
  BOOL first,
    ret = TRUE; // default to success

  ASSERT ( dc ); // caller's responsibility to set this up

  if ( PATH_IsPathOpen(dc->w.path) )
    return PATH_RoundRect ( dc, left, top, right, bottom, xradius, yradius );

  left += dc->w.DCOrgX;
  right += dc->w.DCOrgX;
  top += dc->w.DCOrgY;
  bottom += dc->w.DCOrgY;

  RectBounds.left = left;
  RectBounds.right = right;
  RectBounds.top = top;
  RectBounds.bottom = bottom;

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);

  FillBrushObj = BRUSHOBJ_LockBrush(dc->w.hBrush);
  ASSERT(FillBrushObj);
  if ( FillBrushObj->logbrush.lbStyle == BS_NULL )
    FillBrushObj = NULL; // make null brush check simpler...

  HPenToBrushObj ( &PenBrush, dc->w.hPen );
  if ( PenBrush.logbrush.lbStyle != BS_NULL )
    PenBrushObj = &PenBrush;
  else
    PenBrushObj = NULL;

  right--;
  bottom--;

  width = right - left;
  height = bottom - top;

  if ( (xradius<<1) > width )
    xradius = width >> 1;
  if ( (yradius<<1) > height )
    yradius = height >> 1;

  b_square = yradius * yradius;
  a_square = xradius * xradius;
  row = yradius;
  col = 0;
  two_a_square = a_square << 1;
  four_a_square = a_square << 2;
  four_b_square = b_square << 2;
  two_b_square = b_square << 1;
  d = two_a_square * ((row - 1) * (row))
    + a_square
    + two_b_square * (1 - a_square);

  x1 = left+xradius;
  x2 = right-xradius;
  y1 = top;
  y2 = bottom;

  x1start = x1;
  x2start = x2;

  dinc = two_b_square*3; /* two_b_square * (3 + (col << 1)); */
  ddec = four_a_square * row;

  first = TRUE;
  for ( ;; )
  {
    if ( d >= 0 )
    {
      if ( FillBrushObj )
        PUTLINE ( x1, y1, x2, y1, FillBrushObj );
      if ( first )
      {
	if ( PenBrushObj )
	{
	  if ( x1start > x1 )
	  {
	    PUTLINE ( x1, y1, x1start, y1, PenBrushObj );
	    PUTLINE ( x2start+1, y2, x2+1, y2, PenBrushObj );
	  }
	  else
	  {
	    PUTPIXEL ( x1, y1, PenBrushObj );
	    PUTPIXEL ( x2, y2, PenBrushObj );
	  }
	}
	first = FALSE;
      }
      else
      {
	if ( FillBrushObj )
	  PUTLINE ( x1, y2, x2, y2, FillBrushObj );
	if ( PenBrushObj )
	{
	  if ( x1start >= x1 )
	  {
	    PUTLINE ( x1, y1, x1start+1, y1, PenBrushObj );
	    PUTLINE ( x2start, y2, x2+1, y2, PenBrushObj );
	  }
	  else
	  {
	    PUTPIXEL ( x1, y1, PenBrushObj );
	    PUTPIXEL ( x2, y2, PenBrushObj );
	  }
	}
      }
      if ( PenBrushObj )
      {
	if ( x1start > x1 )
	{
	  PUTLINE ( x1, y2, x1start+1, y2, PenBrushObj );
	  PUTLINE ( x2start, y1, x2+1, y1, PenBrushObj );
	}
	else
	{
	  PUTPIXEL ( x1, y2, PenBrushObj );
	  PUTPIXEL ( x2, y1, PenBrushObj );
	}
      }
      x1start = x1-1;
      x2start = x2+1;
      row--, y1++, y2--, ddec -= four_a_square;
      d -= ddec;
    }

    int potential_steps = ( a_square * row ) / b_square - col + 1;
    while ( d < 0 && potential_steps-- )
    {
      d += dinc; /* two_b_square * (3 + (col << 1)); */
      col++, x1--, x2++, dinc += four_b_square;
    }

    if ( a_square * row <= b_square * col )
      break;
  };

  d = two_b_square * (col + 1) * col
    + two_a_square * (row * (row - 2) + 1)
    + (1 - two_a_square) * b_square;
  dinc = ddec; /* four_b_square * col; */
  ddec = two_a_square * ((row << 1) - 3);

  while ( row )
  {
    if ( FillBrushObj )
    {
      PUTLINE ( x1, y1, x2, y1, FillBrushObj );
      PUTLINE ( x1, y2, x2, y2, FillBrushObj );
    }
    if ( PenBrushObj )
    {
      PUTPIXEL ( x2, y1, PenBrushObj );
      PUTPIXEL ( x1, y2, PenBrushObj );
      PUTPIXEL ( x2, y2, PenBrushObj );
      PUTPIXEL ( x1, y1, PenBrushObj );
    }

    if ( d <= 0 )
    {
      col++, x1--, x2++, dinc += four_b_square;
      d += dinc; //four_b_square * col;
    }

    row--, y1++, y2--, ddec -= four_a_square;
    d -= ddec; //two_a_square * ((row << 1) - 3);
  }

  if ( FillBrushObj )
  {
    PUTLINE ( left, y1, right, y1, FillBrushObj );
    PUTLINE ( left, y2, right, y2, FillBrushObj );
  }
  if ( PenBrushObj )
  {
    if ( x1 > (left+1) )
    {
      PUTLINE ( left, y1, x1, y1, PenBrushObj );
      PUTLINE ( x2+1, y1, right, y1, PenBrushObj );
      PUTLINE ( left+1, y2, x1, y2, PenBrushObj );
      PUTLINE ( x2+1, y2, right+1, y2, PenBrushObj );
    }
    else
    {
      PUTPIXEL ( left, y1, PenBrushObj );
      PUTPIXEL ( right, y2, PenBrushObj );
    }
  }

  x1 = left+xradius;
  x2 = right-xradius;
  y1 = top+yradius;
  y2 = bottom-yradius;

  if ( FillBrushObj )
  {
    for ( i = y1+1; i < y2; i++ )
      PUTLINE ( left, i, right, i, FillBrushObj );
  }

  if ( PenBrushObj )
  {
    PUTLINE ( x1,    top,    x2,    top,    PenBrushObj );
    PUTLINE ( right, y1,     right, y2,     PenBrushObj );
    PUTLINE ( x2,    bottom, x1,    bottom, PenBrushObj );
    PUTLINE ( left,  y2,     left,  y1,     PenBrushObj );
  }

  BRUSHOBJ_UnlockBrush(dc->w.hBrush);

  return ret;
}

BOOL
STDCALL
NtGdiRoundRect(
	HDC  hDC,
	int  LeftRect,
	int  TopRect,
	int  RightRect,
	int  BottomRect,
	int  Width,
	int  Height)
{
  DC   *dc = DC_LockDc(hDC);
  BOOL  ret = FALSE; /* default to failure */

  DPRINT("NtGdiRoundRect(0x%x,%i,%i,%i,%i,%i,%i)\n",hDC,LeftRect,TopRect,RightRect,BottomRect,Width,Height);
  if ( !dc )
  {
    DPRINT1("NtGdiRoundRect() - hDC is invalid\n");
    SetLastWin32Error(ERROR_INVALID_HANDLE);
  }
  else
  {
    ret = IntRoundRect ( dc, LeftRect, TopRect, RightRect, BottomRect, Width, Height );
    DC_UnlockDc ( hDC );
  }

  return ret;
}

BOOL FASTCALL
IntGdiGradientFill(
  DC *dc,
  PTRIVERTEX pVertex,
  ULONG uVertex,
  PVOID pMesh,
  ULONG uMesh,
  ULONG ulMode)
{
  SURFOBJ *SurfObj;
  PPALGDI PalDestGDI;
  XLATEOBJ *XlateObj;
  RECTL Extent;
  POINTL DitherOrg;
  ULONG Mode, i;
  BOOL Ret;
  
  ASSERT(dc);
  ASSERT(pVertex);
  ASSERT(uVertex);
  ASSERT(pMesh);
  ASSERT(uMesh);
  
  /* check parameters */
  if(ulMode & GRADIENT_FILL_TRIANGLE)
  {
    PGRADIENT_TRIANGLE tr = (PGRADIENT_TRIANGLE)pMesh;
    
    for(i = 0; i < uMesh; i++, tr++)
    {
      if(tr->Vertex1 >= uVertex ||
         tr->Vertex2 >= uVertex ||
         tr->Vertex3 >= uVertex)
      {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
      }
    }
  }
  else
  {
    PGRADIENT_RECT rc = (PGRADIENT_RECT)pMesh;
    for(i = 0; i < uMesh; i++, rc++)
    {
      if(rc->UpperLeft >= uVertex || rc->LowerRight >= uVertex)
      {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
      }
    }
  }
  
  /* calculate extent */
  Extent.left = Extent.right = pVertex->x;
  Extent.top = Extent.bottom = pVertex->y;
  for(i = 0; i < uVertex; i++)
  {
    Extent.left = min(Extent.left, (pVertex + i)->x);
    Extent.right = max(Extent.right, (pVertex + i)->x);
    Extent.top = min(Extent.top, (pVertex + i)->y);
    Extent.bottom = max(Extent.bottom, (pVertex + i)->y);
  }
  
  DitherOrg.x = dc->w.DCOrgX;
  DitherOrg.y = dc->w.DCOrgY;
  Extent.left += DitherOrg.x;
  Extent.right += DitherOrg.x;
  Extent.top += DitherOrg.y;
  Extent.bottom += DitherOrg.y;
  
  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  ASSERT(SurfObj);
  
  PalDestGDI = PALETTE_LockPalette(dc->w.hPalette);
  ASSERT(PalDestGDI);
  Mode = PalDestGDI->Mode;
  PALETTE_UnlockPalette(dc->w.hPalette);
  
  XlateObj = (PXLATEOBJ)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
  ASSERT(XlateObj);
  
  Ret = IntEngGradientFill(SurfObj,
                           dc->CombinedClip,
                           XlateObj,
                           pVertex,
                           uVertex,
                           pMesh,
                           uMesh,
                           &Extent,
                           &DitherOrg,
                           ulMode);
  
  EngDeleteXlate(XlateObj);
  
  return Ret;
}

BOOL
STDCALL
NtGdiGradientFill(
  HDC hdc,
  PTRIVERTEX pVertex,
  ULONG uVertex,
  PVOID pMesh,
  ULONG uMesh,
  ULONG ulMode)
{
  DC *dc;
  BOOL Ret;
  PTRIVERTEX SafeVertex;
  PVOID SafeMesh;
  ULONG SizeMesh;
  NTSTATUS Status;
  
  dc = DC_LockDc(hdc);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if(!pVertex || !uVertex || !pMesh || !uMesh)
  {
    DC_UnlockDc(hdc);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  switch(ulMode)
  {
    case GRADIENT_FILL_RECT_H:
    case GRADIENT_FILL_RECT_V:
      SizeMesh = uMesh * sizeof(GRADIENT_RECT);
      break;
    case GRADIENT_FILL_TRIANGLE:
      SizeMesh = uMesh * sizeof(TRIVERTEX);
      break;
    default:
      DC_UnlockDc(hdc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
  }

  if(!(SafeVertex = ExAllocatePoolWithTag(PagedPool, (uVertex * sizeof(TRIVERTEX)) + SizeMesh, TAG_SHAPE)))
  {
    DC_UnlockDc(hdc);
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  Status = MmCopyFromCaller(SafeVertex, pVertex, uVertex * sizeof(TRIVERTEX));
  if(!NT_SUCCESS(Status))
  {
    DC_UnlockDc(hdc);
    ExFreePool(SafeVertex);
    SetLastNtError(Status);
    return FALSE;
  }
  SafeMesh = (PTRIVERTEX)(SafeVertex + uVertex);
  Status = MmCopyFromCaller(SafeMesh, pMesh, SizeMesh);
  if(!NT_SUCCESS(Status))
  {
    DC_UnlockDc(hdc);
    ExFreePool(SafeVertex);
    SetLastNtError(Status);
    return FALSE;
  }
  
  Ret = IntGdiGradientFill(dc, SafeVertex, uVertex, SafeMesh, uMesh, ulMode);
  
  DC_UnlockDc(hdc);
  ExFreePool(SafeVertex);
  return Ret;
}

/* EOF */
