#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>

//#define NDEBUG
#include <win32k/debug1.h>

/* FUNCTIONS *****************************************************************/

INT STDCALL
W32kGetBoxRgn(HRGN hRgn, PRECT Rect)
{
	return 0;
}

HRGN STDCALL
W32kCropRgn(HRGN hDest, HRGN hSrc, const RECT* Rect, const POINT* Point)
{
	return NULL;
}

HRGN STDCALL
W32kUnionRectWithRgn(HRGN hDest, const RECT* Rect)
{
}

INT STDCALL
W32kCombineRgn(HRGN  hDest,
	       HRGN  hSrc1,
	       HRGN  hSrc2,
	       INT  CombineMode)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kCreateEllipticRgn(INT  LeftRect,
		      INT  TopRect,
		      INT  RightRect,
		      INT  BottomRect)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kCreateEllipticRgnIndirect(CONST PRECT rc)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kCreatePolygonRgn(CONST PPOINT  pt,
		     INT  Count,
		     INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kCreatePolyPolygonRgn(CONST PPOINT  pt,
			 CONST PINT  PolyCounts,
			 INT  Count,
			 INT  PolyFillMode)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kCreateRectRgn(INT  LeftRect,
		  INT  TopRect,
		  INT  RightRect,
		  INT  BottomRect)
{
  RGNDATA* Region;
  PRECT Rect;
/*
  DPRINT("W32kCreateRectRgn(LeftRect %d, TopRect %d, RightRect %d, "
	 "BottomRect %d)\n", LeftRect, TopRect, RightRect, BottomRect);

  Region = (RGNDATA*)GDIOBJ_AllocObject(sizeof(RGNDATA) + sizeof(RECT) - 1, 0);
  Region->rdh.dwSize = sizeof(RGNDATA) + sizeof(RECT) - 1;
  Region->rdh.iType = RDH_RECTANGLES;
  Region->rdh.nCount = 0;
  Rect = (PRECT)Region->Buffer;
  Rect->left = LeftRect;
  Rect->right = RightRect;
  Rect->top = TopRect;
  Rect->bottom = BottomRect;
  Region->rdh.rcBound = *Rect;

  return(GDIOBJ_PtrToHandle((PGDIOBJ)Region, 0));
*/
return NULL;
}

HRGN STDCALL
W32kCreateRectRgnIndirect(CONST PRECT rc)
{
  return(W32kCreateRectRgn(rc->left, rc->top, rc->right, rc->bottom));
}

HRGN STDCALL
W32kCreateRoundRectRgn(INT  LeftRect,
		       INT  TopRect,
		       INT  RightRect,
		       INT  BottomRect,
		       INT  WidthEllipse,
		       INT  HeightEllipse)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kEqualRgn(HRGN  hSrcRgn1,
	     HRGN  hSrcRgn2)
{
  UNIMPLEMENTED;
}

HRGN STDCALL
W32kExtCreateRegion(CONST PXFORM  Xform,
		    DWORD  Count,
		    CONST PRGNDATA  RgnData)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kFillRgn(HDC  hDC,
	    HRGN  hRgn,
	    HBRUSH  hBrush)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kFrameRgn(HDC  hDC,
	     HRGN  hRgn,
	     HBRUSH  hBrush,
	     INT  Width,
	     INT  Height)
{
  UNIMPLEMENTED;
}

INT STDCALL
W32kGetRgnBox(HRGN  hRgn,
	      LPRECT  hRect)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kInvertRgn(HDC  hDC,
	      HRGN  hRgn)
{
  UNIMPLEMENTED;
}

INT STDCALL
W32kOffsetRgn(HRGN  hRgn,
	      INT  XOffset,
	      INT  YOffset)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kPaintRgn(HDC  hDC,
	     HRGN  hRgn)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kPtInRegion(HRGN  hRgn,
	       INT  X,
	       INT  Y)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kRectInRegion(HRGN  hRgn,
		 CONST LPRECT  rc)
{
  UNIMPLEMENTED;
}

BOOL STDCALL
W32kSetRectRgn(HRGN  hRgn,
	       INT  LeftRect,
	       INT  TopRect,
	       INT  RightRect,
	       INT  BottomRect)
{
  UNIMPLEMENTED;
}

