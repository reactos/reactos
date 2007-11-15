
#include <w32k.h>

#define NDEBUG
#include <debug.h>

static BOOL FASTCALL
IntArc( DC *dc, int LeftRect, int TopRect, int RightRect, int BottomRect,
        int XStartArc, int YStartArc, int XEndArc, int YEndArc, ARCTYPE arctype)
{
  return TRUE;
}

BOOL FASTCALL
IntGdiArcInternal(
          ARCTYPE arctype,
          DC  *dc,
          int LeftRect,
          int TopRect,
          int RightRect,
          int BottomRect,
          int XStartArc,
          int YStartArc,
          int XEndArc,
          int YEndArc)
{
  INT rx, ry;
  RECT rc, rc1;

  if(PATH_IsPathOpen(dc->w.path))
  {
    INT type = arctype;
    if (arctype == GdiTypeArcTo) type = GdiTypeArc;
    return PATH_Arc(dc, LeftRect, TopRect, RightRect, BottomRect,
                    XStartArc, YStartArc, XEndArc, YEndArc, type);
  }

  IntGdiSetRect(&rc, LeftRect, TopRect, RightRect, BottomRect);
  IntGdiSetRect(&rc1, XStartArc, YStartArc, XEndArc, YEndArc);

  if (dc->w.flags & DCX_WINDOW) //window rectangle instead of client rectangle
  {
    HWND hWnd;
    PWINDOW_OBJECT Window;

    hWnd = IntWindowFromDC((HDC) dc->hHmgr);
    Window = UserGetWindowObject(hWnd);
    if(!Window) return FALSE;

    rc.left += Window->Wnd->ClientRect.left;
    rc.top += Window->Wnd->ClientRect.top;
    rc.right += Window->Wnd->ClientRect.left;
    rc.bottom += Window->Wnd->ClientRect.top;

    rc1.left += Window->Wnd->ClientRect.left;
    rc1.top += Window->Wnd->ClientRect.top;
    rc1.right += Window->Wnd->ClientRect.left;
    rc1.bottom += Window->Wnd->ClientRect.top;
  }

  rx = (rc.right - rc.left)/2 - 1;
  ry = (rc.bottom - rc.top)/2 -1;
  rc.left += rx;
  rc.top += ry;

  return  IntArc( dc, rc.left, rc.top, rx, ry,
          rc1.left, rc1.top, rc1.right, rc1.bottom, arctype);
}

BOOL
STDCALL
NtGdiArcInternal(
        ARCTYPE arctype,
        HDC  hDC,
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
  if (dc->IsIC)
  {
    DC_UnlockDc(dc);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }

  if (arctype == GdiTypeArcTo)
  {
    // Line from current position to starting point of arc
    if ( !IntGdiLineTo(dc, XStartArc, YStartArc) )
    {
      DC_UnlockDc(dc);
      return FALSE;
    }
  }

  Ret = IntGdiArcInternal(
                  arctype,
                  dc,
                  LeftRect,
                  TopRect,
                  RightRect,
                  BottomRect,
                  XStartArc,
                  YStartArc,
                  XEndArc,
                  YEndArc);

  if (arctype == GdiTypeArcTo)
  {
    // If no error occured, the current position is moved to the ending point of the arc.
    if(Ret)
      IntGdiMoveToEx(dc, XEndArc, YEndArc, NULL);
  }

  DC_UnlockDc( dc );
  return Ret;
}

