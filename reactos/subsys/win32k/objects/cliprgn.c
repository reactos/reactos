

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include <win32k/region.h>
#include <win32k/cliprgn.h>

// #define NDEBUG
#include <win32k/debug1.h>

HRGN WINAPI SaveVisRgn(HDC hdc)
{
  HRGN copy;
  PROSRGNDATA obj, copyObj;
  PDC dc = DC_HandleToPtr(hdc);

  if (!dc) return 0;

  obj = RGNDATA_LockRgn(dc->w.hVisRgn);

  if(!(copy = W32kCreateRectRgn(0, 0, 0, 0)))
  {
    RGNDATA_UnlockRgn(dc->w.hVisRgn);
    DC_ReleasePtr(hdc);
    return 0;
  }
  W32kCombineRgn(copy, dc->w.hVisRgn, 0, RGN_COPY);
  copyObj = RGNDATA_LockRgn(copy);
/*  copyObj->header.hNext = obj->header.hNext;
  header.hNext = copy; */

  return copy;
}

INT WINAPI SelectVisRgn(HDC hdc, HRGN hrgn)
{
  int retval;
  DC *dc;

  if (!hrgn) return ERROR;
  if (!(dc = DC_HandleToPtr(hdc))) return ERROR;

  dc->w.flags &= ~DC_DIRTY;

  retval = W32kCombineRgn(dc->w.hVisRgn, hrgn, 0, RGN_COPY);
  //ei CLIPPING_UpdateGCRegion(dc);

  return retval;
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
  int retval;
  DC *dc;

  if (!(dc = DC_HandleToPtr(hDC))) return ERROR;
  retval = W32kGetRgnBox(dc->w.hGCClipRgn, rc);
  rc->left -= dc->w.DCOrgX;
  rc->right -= dc->w.DCOrgX;
  rc->top -= dc->w.DCOrgY;
  rc->bottom -= dc->w.DCOrgY;
  W32kDPtoLP(hDC, (LPPOINT)rc, 2);
  return(retval);
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



