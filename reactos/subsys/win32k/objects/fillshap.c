
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <include/object.h>

// #define NDEBUG
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

BOOL
STDCALL
W32kPolygon(HDC  hDC,
                  CONST PPOINT  Points,
                  int  Count)
{
  UNIMPLEMENTED;
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

    ret = EngLineTo(SurfObj,
                    NULL, // ClipObj,
                    BrushObj,
                    LeftRect, TopRect, RightRect, TopRect,
                    RectBounds, // Bounding rectangle
                    dc->w.ROPmode); // MIX

    ret = EngLineTo(SurfObj,
                    NULL, // ClipObj,
                    BrushObj,
                    RightRect, TopRect, RightRect, BottomRect,
                    RectBounds, // Bounding rectangle
                    dc->w.ROPmode); // MIX

    ret = EngLineTo(SurfObj,
                    NULL, // ClipObj,
                    BrushObj,
                    LeftRect, BottomRect, RightRect, BottomRect,
                    RectBounds, // Bounding rectangle
                    dc->w.ROPmode); // MIX

    ret = EngLineTo(SurfObj,
                    NULL, // ClipObj,
                    BrushObj,
                    LeftRect, TopRect, LeftRect, BottomRect,
                    RectBounds, // Bounding rectangle
                    dc->w.ROPmode); // MIX */

    BrushObj = (BRUSHOBJ*) GDIOBJ_LockObj(dc->w.hBrush, GO_BRUSH_MAGIC);
    assert(BrushObj);
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
    GDIOBJ_UnlockObj( dc->w.hBrush, GO_PEN_MAGIC );	
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
