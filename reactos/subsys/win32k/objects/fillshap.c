
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/dc.h>
#include <win32k/pen.h>

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
   SURFOBJ	*SurfObj = AccessUserObject(dc->Surface);
   PBRUSHOBJ	BrushObj;
   BOOL ret;
   PRECTL	RectBounds = GDIOBJ_HandleToPtr(dc->w.hGCClipRgn, GO_REGION_MAGIC);

   if(!dc) return FALSE;

   if(PATH_IsPathOpen(dc->w.path)) {
      ret = PATH_Rectangle(hDC, LeftRect, TopRect, RightRect, BottomRect);
   } else {

      DbgPrint("W32kRectangle pen: ");
      DbgPrint("--- %08x\n", GDIOBJ_HandleToPtr(dc->w.hPen, GO_PEN_MAGIC));

      BrushObj = PenToBrushObj(dc, GDIOBJ_HandleToPtr(dc->w.hPen, GO_PEN_MAGIC));

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

      ExFreePool(BrushObj);
   }

// FIXME: Move current position in DC?

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


