

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/cliprgn.h>

// #define NDEBUG
#include <win32k/debug1.h>

HRGN WINAPI SaveVisRgn(HDC hdc)
{
  HRGN copy;
  PRGNDATA obj, copyObj;
  PDC dc = DC_HandleToPtr(hdc);
/*ei
  if (!dc) return 0;

  obj = RGNDATA_HandleToPtr(dc->w.hVisRgn);

  if(!(copy = CreateRectRgn(0, 0, 0, 0)))
  {
    GDI_ReleaseObj(dc->w.hVisRgn);
    GDI_ReleaseObj(hdc);
    return 0;
  }
  CombineRgn(copy, dc->w.hVisRgn, 0, RGN_COPY);
  copyObj = RGNDATA_HandleToPtr(copy);
*/
/*  copyObj->header.hNext = obj->header.hNext;
  header.hNext = copy; */
  DC_ReleasePtr( hdc );
  return copy;
}

INT16 WINAPI SelectVisRgn(HDC hdc, HRGN hrgn)
{
	return ERROR;
/*ei
  int retval;
  DC *dc;

  if (!hrgn) return ERROR;
  if (!(dc = DC_HandleToPtr(hdc))) return ERROR;

  dc->flags &= ~DC_DIRTY;

  retval = CombineRgn(dc->hVisRgn, hrgn, 0, RGN_COPY);
  CLIPPING_UpdateGCRegion(dc);

  return retval;
*/
}

int STDCALL W32kExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kExtSelectClipRgn(HDC  hDC,
                          HRGN  hrgn,
                          int  fnMode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kGetClipBox(HDC  hDC,
                    LPRECT  rc)
{
  UNIMPLEMENTED;
}

int STDCALL W32kGetMetaRgn(HDC  hDC,
                    HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kPtVisible(HDC  hDC,
                    int  X,
                    int  Y)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kRectVisible(HDC  hDC,
                      CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSelectClipPath(HDC  hDC,
                         int  Mode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSelectClipRgn(HDC  hDC,
                       HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
}



