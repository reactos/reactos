
#include <w32k.h>

#define NDEBUG
#include <debug.h>

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
    Radius = (Right - Left) >> 1;

    if (Even)
    {
        X = 0;
        Y = Radius;
        d = 2 - Radius;
        X18 = Right;
        X27 = ((Left + Right) >> 1) + 1;
        X36 = (Left + Right) >> 1;
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
        X27 = (Left + Right) >> 1;
        X36 = (Left + Right) >> 1;
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
            d += (X << 1) + (Even ? 4 : 3);

            X27++;
            X36--;
            Y14--;
            Y58++;
        }
        else
        {
            d += ((X - Y) << 1) + 5;
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

    x = (Right + Left) >> 1;
    y = (Bottom + Top) >> 1;
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
    PDC_ATTR;
    RECTL RectBounds;
    SURFOBJ *SurfObj;
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
    if (dc->DC_Type == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    Dc_Attr = dc->pDc_Attr;
    if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    FillBrushObj = BRUSHOBJ_LockBrush(Dc_Attr->hbrush);
    if (NULL == FillBrushObj)
    {
        DC_UnlockDc(dc);
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    Left += dc->ptlDCOrig.x;
    Right += dc->ptlDCOrig.x;
    Top += dc->ptlDCOrig.y;
    Bottom += dc->ptlDCOrig.y;
    XRadialStart += dc->ptlDCOrig.x;
    YRadialStart += dc->ptlDCOrig.y;
    XRadialEnd += dc->ptlDCOrig.x;
    YRadialEnd += dc->ptlDCOrig.y;

    RectBounds.left = Left;
    RectBounds.right = Right;
    RectBounds.top = Top;
    RectBounds.bottom = Bottom;

    SurfObj = (SURFOBJ*) AccessUserObject((ULONG)dc->Surface);
    HPenToBrushObj(&PenBrushObj, Dc_Attr->hpen);

    /* Number of points for the circle is 4 * sqrt(2) * Radius, start
       and end line have at most Radius points, so allocate at least
       that much */
    ShapePoints = ExAllocatePoolWithTag(PagedPool, 8 * (Right - Left + 1) / 2 * sizeof(SHAPEPOINT), TAG_SHAPE);
    if (NULL == ShapePoints)
    {
        BRUSHOBJ_UnlockBrush(FillBrushObj);
        DC_UnlockDc(dc);

        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (Left == Right)
    {
        PUTPIXEL(Left, Top, &PenBrushObj);
        BRUSHOBJ_UnlockBrush(FillBrushObj);
        DC_UnlockDc(dc);

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
    BRUSHOBJ_UnlockBrush(FillBrushObj);
    DC_UnlockDc(dc);

    return ret;
#else
    return TRUE;
#endif
}


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

  rx = (rc.right - rc.left)/2 - 1;
  ry = (rc.bottom - rc.top)/2 -1;
  rc.left += rx;
  rc.top += ry;

  return  IntArc( dc, rc.left, rc.top, rx, ry,
          rc1.left, rc1.top, rc1.right, rc1.bottom, arctype);
}

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
  DC *dc;
  BOOL Ret = FALSE;
  gxf_long worker, worker1;

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
  worker.l  = dwStartAngle;
  worker1.l = dwSweepAngle;

  DC_UnlockDc( dc );
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

