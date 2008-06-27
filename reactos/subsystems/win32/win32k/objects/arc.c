
#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * a couple macros to fill a single pixel or a line
 */
#define PUTPIXEL(x,y,BrushInst)        \
  ret = ret && IntEngLineTo(&BitmapObj->SurfObj, \
       dc->CombinedClip,                         \
       &BrushInst.BrushObject,                   \
       x, y, (x)+1, y,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(Dc_Attr->jROP2));

#define PUTLINE(x1,y1,x2,y2,BrushInst) \
  ret = ret && IntEngLineTo(&BitmapObj->SurfObj, \
       dc->CombinedClip,                         \
       &BrushInst.BrushObject,                   \
       x1, y1, x2, y2,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(Dc_Attr->jROP2));

#define Rsin(d) ((d) == 0.0 ? 0.0 : ((d) == 90.0 ? 1.0 : sin(d*M_PI/180.0)))
#define Rcos(d) ((d) == 0.0 ? 1.0 : ((d) == 90.0 ? 0.0 : cos(d*M_PI/180.0)))

static
BOOL
FASTCALL
IntArc( DC *dc, 
        int  Left,
        int  Top,
        int  Right,
        int  Bottom,
        int  XRadialStart,
        int  YRadialStart,
        int  XRadialEnd,
        int  YRadialEnd,
        ARCTYPE arctype)
{
    PDC_ATTR Dc_Attr;
    RECTL RectBounds;
    PGDIBRUSHOBJ PenBrushObj, FillBrushObj;
    GDIBRUSHINST FillBrushInst, PenBrushInst;
    BITMAPOBJ *BitmapObj;
    BOOL ret = TRUE;
    double AngleStart, AngleEnd;
    LONG RadiusX, RadiusY, CenterX, CenterY;
    LONG SfCx, SfCy, EfCx, EfCy;

/*                  top
            ___________________
          +|                   |
           |                   |
           |                   |
      left |                   | right
           |                   |
           |                   |
          0|___________________|
            0     bottom       +
 */
    if (Right <= Left || Top <= Bottom)
    {
        DPRINT1("Arc Fail 1\n");
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
/*
    if (Right - Left != Bottom - Top)
    {
        UNIMPLEMENTED;
    }
*/
    Dc_Attr = dc->pDc_Attr;
    if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    FillBrushObj = BRUSHOBJ_LockBrush(Dc_Attr->hbrush);
    if (NULL == FillBrushObj)
    {
        DPRINT1("Arc Fail 2\n");
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    PenBrushObj = PENOBJ_LockPen(Dc_Attr->hpen);
    if (NULL == PenBrushObj)
    {
        DPRINT1("Arc Fail 3\n");
        BRUSHOBJ_UnlockBrush(FillBrushObj);
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
    if (NULL == BitmapObj)
    {
        DPRINT1("Arc Fail 4\n");
        BRUSHOBJ_UnlockBrush(FillBrushObj);
        PENOBJ_UnlockPen(PenBrushObj);
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    IntGdiInitBrushInstance(&FillBrushInst, FillBrushObj, dc->XlateBrush);
    IntGdiInitBrushInstance(&PenBrushInst, PenBrushObj, dc->XlatePen);

    Left   += dc->ptlDCOrig.x;
    Right  += dc->ptlDCOrig.x;
    Top    += dc->ptlDCOrig.y;
    Bottom += dc->ptlDCOrig.y;

    XRadialStart += dc->ptlDCOrig.x;
    YRadialStart += dc->ptlDCOrig.y;
    XRadialEnd   += dc->ptlDCOrig.x;
    YRadialEnd   += dc->ptlDCOrig.y;

    DPRINT1("1: StartX: %d, StartY: %d, EndX: %d, EndY: %d\n",
               XRadialStart,YRadialStart,XRadialEnd,YRadialEnd);

    RectBounds.left   = Left;
    RectBounds.right  = Right;
    RectBounds.top    = Top;
    RectBounds.bottom = Bottom;
    DPRINT1("1: Left: %d, Top: %d, Right: %d, Bottom: %d\n",
               RectBounds.left,RectBounds.top,RectBounds.right,RectBounds.bottom);

    if (Left == Right)
    {
        DPRINT1("Arc Good Exit\n");
        PUTPIXEL(Left, Top, PenBrushInst);
        BITMAPOBJ_UnlockBitmap(BitmapObj);
        BRUSHOBJ_UnlockBrush(FillBrushObj);
        PENOBJ_UnlockPen(PenBrushObj);
        return ret;
    }
    RadiusX = (RectBounds.right - RectBounds.left)/2;
    RadiusY = (RectBounds.bottom - RectBounds.top)/2;
    CenterX = RectBounds.left + RadiusX;
    CenterY = RectBounds.top + RadiusY;

    AngleEnd   = atan2(-(YRadialEnd - CenterY), XRadialEnd - CenterX)* (180.0 / M_PI);
    AngleStart = atan2(-(YRadialStart - CenterY), XRadialStart - CenterX)* (180.0 / M_PI);

    SfCx = (Rcos(AngleStart) * RadiusX);
    SfCy = (Rsin(AngleStart) * RadiusY);

    EfCx = (Rcos(AngleEnd) * RadiusX);
    EfCy = (Rsin(AngleEnd) * RadiusY);
    {
       FLOAT AngS = AngleStart, Factor = 1;
       int x,y, ox = 0, oy = 0;

       if (arctype == GdiTypePie)
       {
          PUTLINE(SfCx + CenterX, SfCy + CenterY, CenterX, CenterY, PenBrushInst);
       }

       for(; AngS < AngleEnd; AngS += Factor)
       {
          x = (Rcos(AngS) * RadiusX);
          y = (Rsin(AngS) * RadiusY);
  
          if (arctype == GdiTypePie)
             PUTLINE((x + CenterX) - 1, (y + CenterY) - 1, CenterX, CenterY, FillBrushInst);

          PUTPIXEL (x + CenterX, y + CenterY, PenBrushInst);
          ox = x;
          oy = y;
       }

       if (arctype == GdiTypePie)
           PUTLINE(EfCx + CenterX, EfCy + CenterY, CenterX, CenterY, PenBrushInst);

    }
    BITMAPOBJ_UnlockBitmap(BitmapObj);
    BRUSHOBJ_UnlockBrush(FillBrushObj);
    PENOBJ_UnlockPen(PenBrushObj);
    DPRINT1("IntArc Exit.\n");
    return ret;
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
  BOOL Ret;
  RECT rc, rc1;
  double AngleStart, AngleEnd;
  LONG RadiusX, RadiusY, CenterX, CenterY, Width, Height;
  LONG SfCx, SfCy, EfCx = 0, EfCy = 0;

  DPRINT1("StartX: %d, StartY: %d, EndX: %d, EndY: %d\n",
           XStartArc,YStartArc,XEndArc,YEndArc);
  DPRINT1("Left: %d, Top: %d, Right: %d, Bottom: %d\n",
           LeftRect,TopRect,RightRect,BottomRect);

  if (PATH_IsPathOpen(dc->DcLevel))
  {
     return PATH_Arc( dc,
                LeftRect,
                 TopRect,
               RightRect,
              BottomRect,
               XStartArc,
               YStartArc,
                 XEndArc,
                 YEndArc,
                 arctype);
  }

  if (arctype == GdiTypeArcTo)
  {
    Width   = fabs(RightRect - LeftRect);
    Height  = fabs(BottomRect - TopRect);
    RadiusX = Width/2;
    RadiusY = Height/2;
    CenterX = RightRect > LeftRect ? LeftRect + RadiusX : RightRect + RadiusX;
    CenterY = BottomRect > TopRect ? TopRect + RadiusY : BottomRect + RadiusY;

    AngleStart = atan2((YStartArc - CenterY)/Height, (XStartArc - CenterX)/Width);
    AngleEnd   = atan2((YEndArc - CenterY)/Height, (XEndArc - CenterX)/Width);

    EfCx = GDI_ROUND(CenterX+cos(AngleEnd) * RadiusX);
    EfCy = GDI_ROUND(CenterY+sin(AngleEnd) * RadiusY);
    SfCx = GDI_ROUND(CenterX+cos(AngleStart) * RadiusX);
    SfCy = GDI_ROUND(CenterY+sin(AngleStart) * RadiusY);

    IntGdiLineTo(dc, SfCx, SfCy);
  }

  IntGdiSetRect(&rc, LeftRect, TopRect, RightRect, BottomRect);
  IntGdiSetRect(&rc1, XStartArc, YStartArc, XEndArc, YEndArc);

//  IntLPtoDP(dc, (LPPOINT)&rc, 2);
//  IntLPtoDP(dc, (LPPOINT)&rc1, 2);

  Ret = IntArc( dc,
           rc.left,
            rc.top,
          rc.right,
         rc.bottom,
          rc1.left,
           rc1.top,
         rc1.right,
        rc1.bottom,
           arctype);

  if (arctype == GdiTypeArcTo)
  {
     dc->ptlDCOrig.x = EfCx;
     dc->ptlDCOrig.y = EfCy;
  }

  return Ret;
}

BOOL
FASTCALL
IntGdiAngleArc( PDC pDC,
                  INT x,
                  INT y,
         DWORD dwRadius,
      FLOAT eStartAngle,
      FLOAT eSweepAngle)
{
  INT  x1, y1, x2, y2, arcdir;
  BOOL result;

  /* Calculate the end point */
  x2 = x + (INT)(cos(((eStartAngle+eSweepAngle)/360)*(M_PI*2)) * dwRadius);
  y2 = y - (INT)(sin(((eStartAngle+eSweepAngle)/360)*(M_PI*2)) * dwRadius);

  x1 = x + (INT)(cos((eStartAngle/360)*(M_PI*2)) * dwRadius);
  y1 = y - (INT)(sin((eStartAngle/360)*(M_PI*2)) * dwRadius);

  arcdir = pDC->DcLevel.flPath & DCPATH_CLOCKWISE;
  if (eSweepAngle >= 0)
     pDC->DcLevel.flPath &= ~DCPATH_CLOCKWISE;
  else
     pDC->DcLevel.flPath |= DCPATH_CLOCKWISE;

  result = IntGdiArcInternal( GdiTypeArcTo,
                                       pDC,
                                x-dwRadius,
                                y+dwRadius,
                                x+dwRadius,
                                y-dwRadius,
                                        x1,
                                        y1,
                                        x2,
                                        y2 );

  pDC->DcLevel.flPath |= (arcdir & DCPATH_CLOCKWISE);

  if (result)
  {
     POINT point;
     point.x=x2;
     point.y=y2;
//     CoordLPtoDP ( pDC, &point );
     pDC->ptlDCOrig.x = point.x;
     pDC->ptlDCOrig.y = point.y;
  }
  return result;
}

/* FUNCTIONS *****************************************************************/

BOOL
APIENTRY
NtGdiAngleArc(
    IN HDC hDC,
    IN INT x,
    IN INT y,
    IN DWORD dwRadius,
    IN DWORD dwStartAngle,
    IN DWORD dwSweepAngle)
{
  DC *pDC;
  BOOL Ret = FALSE;
  gxf_long worker, worker1;

  pDC = DC_LockDc (hDC);
  if(!pDC)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (pDC->DC_Type == DC_TYPE_INFO)
  {
    DC_UnlockDc(pDC);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }
  worker.l  = dwStartAngle;
  worker1.l = dwSweepAngle;
  Ret = IntGdiAngleArc( pDC, x, y, dwRadius, worker.f, worker1.f);
  DC_UnlockDc( pDC );
  return Ret;
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
  if (dc->DC_Type == DC_TYPE_INFO)
  {
    DC_UnlockDc(dc);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
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

  DC_UnlockDc( dc );
  return Ret;
}

