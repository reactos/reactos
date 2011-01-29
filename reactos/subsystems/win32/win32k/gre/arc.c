/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/arc.c
 * PURPOSE:         Graphic engine: arcs
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * a couple macros to fill a single pixel or a line
 */
#define PUTPIXEL(x,y,BrushInst)        \
  ret = ret && GreLineTo(&psurf->SurfObj, \
       dc->CombinedClip,                         \
       &BrushInst.BrushObject,                    \
       x, y, (x)+1, y,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(R2_COPYPEN/*pdcattr->jROP2*/));

#define PUTLINE(x1,y1,x2,y2,BrushInst) \
  ret = ret && GreLineTo(&psurf->SurfObj, \
       dc->CombinedClip,                         \
       &BrushInst.BrushObject,                    \
       x1, y1, x2, y2,                           \
       &RectBounds,                              \
       ROP2_TO_MIX(R2_COPYPEN/*pdcattr->jROP2*/));

#define Rsin(d) ((d) == 0.0 ? 0.0 : ((d) == 90.0 ? 1.0 : sin(d*M_PI/180.0)))
#define Rcos(d) ((d) == 0.0 ? 1.0 : ((d) == 90.0 ? 0.0 : cos(d*M_PI/180.0)))

BOOLEAN
APIENTRY
GrepArc(PDC  dc,
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
    RECTL RectBounds, RectSEpts;
    PBRUSH pbrushPen;
    SURFACE *psurf;
    BOOLEAN ret = TRUE;
    LONG PenWidth, PenOrigWidth;
    double AngleStart, AngleEnd;
    LONG RadiusX, RadiusY, CenterX, CenterY;
    LONG SfCx, SfCy, EfCx, EfCy;
    POINTL BrushOrg;

    if (Right < Left)
    {
       INT tmp = Right; Right = Left; Left = tmp;
    }
    if (Bottom < Top)
    {
       INT tmp = Bottom; Bottom = Top; Top = tmp;
    }
    if ((Left == Right) ||
        (Top == Bottom) ||
        (((arctype != GdiTypeArc) || (arctype != GdiTypeArcTo)) &&
        ((Right - Left == 1) ||
        (Bottom - Top == 1))))
       return TRUE;

    pbrushPen = dc->dclevel.pbrLine;
    PenOrigWidth = PenWidth = pbrushPen->ptPenWidth.x;
    if (pbrushPen->ulPenStyle == PS_NULL) PenWidth = 0;

    if (pbrushPen->ulPenStyle == PS_INSIDEFRAME)
    {
       if (2*PenWidth > (Right - Left)) PenWidth = (Right -Left + 1)/2;
       if (2*PenWidth > (Bottom - Top)) PenWidth = (Bottom -Top + 1)/2;
       Left   += PenWidth / 2;
       Right  -= (PenWidth - 1) / 2;
       Top    += PenWidth / 2;
       Bottom -= (PenWidth - 1) / 2;
    }

    if (!PenWidth) PenWidth = 1;
    pbrushPen->ptPenWidth.x = PenWidth;

    RectBounds.left   = Left;
    RectBounds.right  = Right;
    RectBounds.top    = Top;
    RectBounds.bottom = Bottom;

    RectSEpts.left   = XRadialStart;
    RectSEpts.top    = YRadialStart;
    RectSEpts.right  = XRadialEnd;
    RectSEpts.bottom = YRadialEnd;

    DPRINT("1: StartX: %d, StartY: %d, EndX: %d, EndY: %d\n",
               RectSEpts.left,RectSEpts.top,RectSEpts.right,RectSEpts.bottom);

    DPRINT("1: Left: %d, Top: %d, Right: %d, Bottom: %d\n",
               RectBounds.left,RectBounds.top,RectBounds.right,RectBounds.bottom);

    RadiusX = max((RectBounds.right - RectBounds.left) / 2, 1);
    RadiusY = max((RectBounds.bottom - RectBounds.top) / 2, 1);
    CenterX = (RectBounds.right + RectBounds.left) / 2;
    CenterY = (RectBounds.bottom + RectBounds.top) / 2;
    AngleEnd   = atan2((RectSEpts.bottom - CenterY), RectSEpts.right - CenterX)*(360.0/(M_PI*2));
    AngleStart = atan2((RectSEpts.top - CenterY), RectSEpts.left - CenterX)*(360.0/(M_PI*2));

    SfCx = (Rcos(AngleStart) * RadiusX);
    SfCy = (Rsin(AngleStart) * RadiusY);
    EfCx = (Rcos(AngleEnd) * RadiusX);
    EfCy = (Rsin(AngleEnd) * RadiusY);

    BrushOrg.x = 0;
    BrushOrg.y = 0;

    if ((arctype == GdiTypePie) || (arctype == GdiTypeChord))
    {
        GrepFillArc( dc,
              RectBounds.left,
              RectBounds.top,
              abs(RectBounds.right-RectBounds.left), // Width
              abs(RectBounds.bottom-RectBounds.top), // Height
              AngleStart,
              AngleEnd,
              arctype,
              &BrushOrg);
    }

    ret = GrepDrawArc( dc,
              RectBounds.left,
              RectBounds.top,
              abs(RectBounds.right-RectBounds.left), // Width
              abs(RectBounds.bottom-RectBounds.top), // Height
              AngleStart,
              AngleEnd,
              arctype,
              pbrushPen,
              &BrushOrg);

    psurf = dc->dclevel.pSurface;
    if (NULL == psurf)
    {
        DPRINT1("Arc Fail 2\n");
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return FALSE;
    }

    if (arctype == GdiTypePie)
    {
       PUTLINE(CenterX, CenterY, SfCx + CenterX, SfCy + CenterY, dc->eboLine);
       PUTLINE(EfCx + CenterX, EfCy + CenterY, CenterX, CenterY, dc->eboLine);
    }
    if (arctype == GdiTypeChord)
        PUTLINE(EfCx + CenterX, EfCy + CenterY, SfCx + CenterX, SfCy + CenterY, dc->eboLine);

    pbrushPen->ptPenWidth.x = PenOrigWidth;

    return ret;
}

/* EOF */
