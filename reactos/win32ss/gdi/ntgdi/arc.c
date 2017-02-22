#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
 * A couple of macros to fill a single pixel or a line
 */
#define PUTPIXEL(x,y,BrushInst)        \
  ret = ret && IntEngLineTo(&psurf->SurfObj, \
       &dc->co.ClipObj,                         \
       &BrushInst.BrushObject,                   \
       x, y, (x)+1, y,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(pdcattr->jROP2));

#define PUTLINE(x1,y1,x2,y2,BrushInst) \
  ret = ret && IntEngLineTo(&psurf->SurfObj, \
       &dc->co.ClipObj,                         \
       &BrushInst.BrushObject,                   \
       x1, y1, x2, y2,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(pdcattr->jROP2));

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
    PDC_ATTR pdcattr;
    RECTL RectBounds, RectSEpts;
    PBRUSH pbrPen;
    SURFACE *psurf;
    BOOL ret = TRUE;
    LONG PenWidth, PenOrigWidth;
    double AngleStart, AngleEnd;
    LONG CenterX, CenterY;

    if (Right < Left)
    {
       INT tmp = Right; Right = Left; Left = tmp;
    }
    if (Bottom < Top)
    {
       INT tmp = Bottom; Bottom = Top; Top = tmp;
    }

    /* Check if the target rect is empty */
    if ((Left == Right) || (Top == Bottom)) return TRUE;

    // FIXME: this needs to be verified
    if ((arctype == GdiTypeChord ) || (arctype == GdiTypePie))
    {
        if ((Right - Left == 1) || (Bottom - Top == 1))
           return TRUE;
    }


    pdcattr = dc->pdcattr;

    pbrPen = PEN_ShareLockPen(pdcattr->hpen);
    if (!pbrPen)
    {
        DPRINT1("Arc Fail 1\n");
        EngSetLastError(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    PenOrigWidth = PenWidth = pbrPen->lWidth;
    if (pbrPen->ulPenStyle == PS_NULL) PenWidth = 0;

    if (pbrPen->ulPenStyle == PS_INSIDEFRAME)
    {
       if (2*PenWidth > (Right - Left)) PenWidth = (Right -Left + 1)/2;
       if (2*PenWidth > (Bottom - Top)) PenWidth = (Bottom -Top + 1)/2;
       Left   += PenWidth / 2;
       Right  -= (PenWidth - 1) / 2;
       Top    += PenWidth / 2;
       Bottom -= (PenWidth - 1) / 2;
    }

    if (!PenWidth) PenWidth = 1;
    pbrPen->lWidth = PenWidth;

    RectBounds.left   = Left;
    RectBounds.right  = Right;
    RectBounds.top    = Top;
    RectBounds.bottom = Bottom;

    RectSEpts.left   = XRadialStart;
    RectSEpts.top    = YRadialStart;
    RectSEpts.right  = XRadialEnd;
    RectSEpts.bottom = YRadialEnd;

    IntLPtoDP(dc, (LPPOINT)&RectBounds, 2);
    IntLPtoDP(dc, (LPPOINT)&RectSEpts, 2);

    RectBounds.left   += dc->ptlDCOrig.x;
    RectBounds.right  += dc->ptlDCOrig.x;
    RectBounds.top    += dc->ptlDCOrig.y;
    RectBounds.bottom += dc->ptlDCOrig.y;

    RectSEpts.left    += dc->ptlDCOrig.x;
    RectSEpts.top     += dc->ptlDCOrig.y;
    RectSEpts.right   += dc->ptlDCOrig.x;
    RectSEpts.bottom  += dc->ptlDCOrig.y;

    DPRINT("1: StartX: %d, StartY: %d, EndX: %d, EndY: %d\n",
               RectSEpts.left,RectSEpts.top,RectSEpts.right,RectSEpts.bottom);

    DPRINT("1: Left: %d, Top: %d, Right: %d, Bottom: %d\n",
               RectBounds.left,RectBounds.top,RectBounds.right,RectBounds.bottom);

    CenterX = (RectBounds.right + RectBounds.left) / 2;
    CenterY = (RectBounds.bottom + RectBounds.top) / 2;
    AngleEnd   = atan2((RectSEpts.bottom - CenterY), RectSEpts.right - CenterX)*(360.0/(M_PI*2));
    AngleStart = atan2((RectSEpts.top - CenterY), RectSEpts.left - CenterX)*(360.0/(M_PI*2));

    /* Edge Case: Check if the start segments overlaps(is equal) the end segment */
    if (AngleEnd == AngleStart)
    {
        AngleStart = AngleEnd + 360.0; // Arc(), ArcTo(), Pie() and Chord() are counterclockwise APIs.
    }

    if ((arctype == GdiTypePie) || (arctype == GdiTypeChord))
    {
        ret = IntFillArc( dc,
              RectBounds.left,
              RectBounds.top,
              abs(RectBounds.right-RectBounds.left), // Width
              abs(RectBounds.bottom-RectBounds.top), // Height
              AngleStart,
              AngleEnd,
              arctype);
    }

    if(ret)
    {
        ret = IntDrawArc( dc,
                  RectBounds.left,
                  RectBounds.top,
                  abs(RectBounds.right-RectBounds.left), // Width
                  abs(RectBounds.bottom-RectBounds.top), // Height
                  AngleStart,
                  AngleEnd,
                  arctype,
                  pbrPen);
    }

    psurf = dc->dclevel.pSurface;
    if (NULL == psurf)
    {
        DPRINT1("Arc Fail 2\n");
        PEN_ShareUnlockPen(pbrPen);
        EngSetLastError(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    if (arctype == GdiTypePie)
    {
        PUTLINE(CenterX, CenterY, RectSEpts.left, RectSEpts.top, dc->eboLine);
        PUTLINE(RectSEpts.right, RectSEpts.bottom, CenterX, CenterY, dc->eboLine);
    }
    if (arctype == GdiTypeChord)
        PUTLINE(RectSEpts.right, RectSEpts.bottom, RectSEpts.left, RectSEpts.top, dc->eboLine);

    pbrPen->lWidth = PenOrigWidth;
    PEN_ShareUnlockPen(pbrPen);
    DPRINT("IntArc Exit.\n");
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
  //PDC_ATTR pdcattr;

  DPRINT("StartX: %d, StartY: %d, EndX: %d, EndY: %d\n",
           XStartArc,YStartArc,XEndArc,YEndArc);
  DPRINT("Left: %d, Top: %d, Right: %d, Bottom: %d\n",
           LeftRect,TopRect,RightRect,BottomRect);

  if ((LeftRect == RightRect) || (TopRect == BottomRect)) return TRUE;

  if (PATH_IsPathOpen(dc->dclevel))
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
                       0,
                 arctype);
  }

  //pdcattr = dc->pdcattr;

  if (arctype == GdiTypeArcTo)
  {
    if (dc->dclevel.flPath & DCPATH_CLOCKWISE)
       IntGdiLineTo(dc, XEndArc, YEndArc);
    else
       IntGdiLineTo(dc, XStartArc, YStartArc);
  }

  Ret = IntArc( dc,
          LeftRect,
           TopRect,
         RightRect,
        BottomRect,
         XStartArc,
         YStartArc,
           XEndArc,
           YEndArc,
           arctype);

  if (arctype == GdiTypeArcTo)
  {
     if (dc->dclevel.flPath & DCPATH_CLOCKWISE)
       IntGdiMoveToEx(dc, XStartArc, YStartArc, NULL);
     else
       IntGdiMoveToEx(dc, XEndArc, YEndArc, NULL);
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

  arcdir = pDC->dclevel.flPath & DCPATH_CLOCKWISE;
  if (eSweepAngle >= 0)
     pDC->dclevel.flPath &= ~DCPATH_CLOCKWISE;
  else
     pDC->dclevel.flPath |= DCPATH_CLOCKWISE;

  result = IntGdiArcInternal( GdiTypeArcTo,
                                       pDC,
                                x-dwRadius,
                                y-dwRadius,
                                x+dwRadius,
                                y+dwRadius,
                                        x1,
                                        y1,
                                        x2,
                                        y2 );

  pDC->dclevel.flPath |= (arcdir & DCPATH_CLOCKWISE);

  if (result)
  {
     IntGdiMoveToEx(pDC, x2, y2, NULL);
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
  KFLOATING_SAVE FloatSave;
  NTSTATUS status;

  pDC = DC_LockDc (hDC);
  if(!pDC)
  {
    EngSetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (pDC->dctype == DC_TYPE_INFO)
  {
    DC_UnlockDc(pDC);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }

  status = KeSaveFloatingPointState(&FloatSave);
  if (!NT_SUCCESS(status))
  {
      DC_UnlockDc( pDC );
      return FALSE;
  }

  worker.l  = dwStartAngle;
  worker1.l = dwSweepAngle;
  DC_vPrepareDCsForBlit(pDC, NULL, NULL, NULL);
  if (pDC->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
    DC_vUpdateFillBrush(pDC);
  if (pDC->pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
    DC_vUpdateLineBrush(pDC);
  Ret = IntGdiAngleArc( pDC, x, y, dwRadius, worker.f, worker1.f);
  DC_vFinishBlit(pDC, NULL);
  DC_UnlockDc( pDC );

  KeRestoreFloatingPointState(&FloatSave);

  return Ret;
}

BOOL
APIENTRY
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
  KFLOATING_SAVE FloatSave;
  NTSTATUS status;

  dc = DC_LockDc (hDC);
  if(!dc)
  {
    EngSetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (dc->dctype == DC_TYPE_INFO)
  {
    DC_UnlockDc(dc);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }
  if (arctype > GdiTypePie)
  {
    DC_UnlockDc(dc);
    EngSetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);

  if (dc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
    DC_vUpdateFillBrush(dc);

  if (dc->pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
    DC_vUpdateLineBrush(dc);

  status = KeSaveFloatingPointState(&FloatSave);
  if (!NT_SUCCESS(status))
  {
      DC_UnlockDc( dc );
      return FALSE;
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

  KeRestoreFloatingPointState(&FloatSave);
  DC_vFinishBlit(dc, NULL);
  DC_UnlockDc( dc );
  return Ret;
}

