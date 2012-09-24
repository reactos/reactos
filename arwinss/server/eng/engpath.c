/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpath.c
 * PURPOSE:         Path Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

PATHOBJ*
APIENTRY
EngCreatePath(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngDeletePath(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mix,
  IN FLONG  flOptions)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngStrokeAndFillPath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pboStroke,
  IN LINEATTRS  *plineattrs,
  IN BRUSHOBJ  *pboFill,
  IN POINTL  *pptlBrushOrg,
  IN MIX  mixFill,
  IN FLONG  flOptions)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngStrokePath(
  IN SURFOBJ  *pso,
  IN PATHOBJ  *ppo,
  IN CLIPOBJ  *pco,
  IN XFORMOBJ  *pxo,
  IN BRUSHOBJ  *pbo,
  IN POINTL  *pptlBrushOrg,
  IN LINEATTRS  *plineattrs,
  IN MIX  mix)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngLineTo(SURFOBJ *DestObj,
          CLIPOBJ *Clip,
          BRUSHOBJ *pbo,
          LONG x1,
          LONG y1,
          LONG x2,
          LONG y2,
          RECTL *RectBounds,
          MIX mix)
{
    LONG x, y, deltax, deltay, xchange, ychange, hx, vy;
    ULONG i;
    ULONG Pixel = pbo->iSolidColor;
    SURFOBJ *OutputObj;
    RECTL DestRect;
    POINTL Translate;
    INTENG_ENTER_LEAVE EnterLeave;
    RECT_ENUM RectEnum;
    BOOL EnumMore;
    CLIPOBJ *pcoPriv = NULL;
    DPRINT("LineTo: (%d,%d)-(%d,%d)\n", x1,y1,x2,y2);
    if (x1 < x2)
    {
        DestRect.left = x1;
        DestRect.right = x2;
    }
    else
    {
        DestRect.left = x2;
        DestRect.right = x1 + 1;
    }
    if (y1 < y2)
    {
        DestRect.top = y1;
        DestRect.bottom = y2;
    }
    else
    {
        DestRect.top = y2;
        DestRect.bottom = y1 + 1;
    }

    if (! IntEngEnter(&EnterLeave, DestObj, &DestRect, FALSE, &Translate, &OutputObj))
    {
        return FALSE;
    }

    if (!Clip)
    {
        Clip = pcoPriv = IntEngCreateClipRegion(0, 0, RectBounds);
        if (!Clip)
        {
            return FALSE;
        }
    }

    x1 += Translate.x;
    x2 += Translate.x;
    y1 += Translate.y;
    y2 += Translate.y;

    x = x1;
    y = y1;
    deltax = x2 - x1;
    deltay = y2 - y1;

    if (0 == deltax && 0 == deltay)
    {
        return TRUE;
    }

    if (deltax < 0)
    {
        xchange = -1;
        deltax = - deltax;
        hx = x2 + 1;
    }
    else
    {
        xchange = 1;
        hx = x1;
    }

    if (deltay < 0)
    {
        ychange = -1;
        deltay = - deltay;
        vy = y2 + 1;
    }
    else
    {
        ychange = 1;
        vy = y1;
    }

    if (y1 == y2)
    {
        CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
        do
        {
            EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
            for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top + Translate.y <= y1; i++)
            {
                if (y1 < RectEnum.arcl[i].bottom + Translate.y &&
                        RectEnum.arcl[i].left + Translate.x <= hx + deltax &&
                        hx < RectEnum.arcl[i].right + Translate.x &&
                        max(hx, RectEnum.arcl[i].left + Translate.x) <
                        min(hx + deltax, RectEnum.arcl[i].right + Translate.x))
                {
                    DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_HLine(
                        OutputObj,
                        max(hx, RectEnum.arcl[i].left + Translate.x),
                        min(hx + deltax, RectEnum.arcl[i].right + Translate.x),
                        y1, Pixel);
                }
            }
        }
        while (EnumMore);
    }
    else if (x1 == x2)
    {
        CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
        do
        {
            EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
            for (i = 0; i < RectEnum.c; i++)
            {
                if (RectEnum.arcl[i].left + Translate.x <= x1 &&
                        x1 < RectEnum.arcl[i].right + Translate.x &&
                        RectEnum.arcl[i].top + Translate.y <= vy + deltay &&
                        vy < RectEnum.arcl[i].bottom + Translate.y)
                {
                    DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_VLine(
                        OutputObj, x1,
                        max(vy, RectEnum.arcl[i].top + Translate.y),
                        min(vy + deltay, RectEnum.arcl[i].bottom + Translate.y),
                        Pixel);
                }
            }
        }
        while (EnumMore);
    }
    else
    {
        if (0 < xchange)
        {
            if (0 < ychange)
            {
                NWtoSE(OutputObj, Clip, pbo, x, y, deltax, deltay, &Translate);
            }
            else
            {
                SWtoNE(OutputObj, Clip, pbo, x, y, deltax, deltay, &Translate);
            }
        }
        else
        {
            if (0 < ychange)
            {
                NEtoSW(OutputObj, Clip, pbo, x, y, deltax, deltay, &Translate);
            }
            else
            {
                SEtoNW(OutputObj, Clip, pbo, x, y, deltax, deltay, &Translate);
            }
        }
    }

    if (pcoPriv)
    {
        IntEngDeleteClipRegion(pcoPriv);
    }

    return IntEngLeave(&EnterLeave);
}

BOOL
APIENTRY
PATHOBJ_bCloseFigure(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnum(IN PATHOBJ  *ppo,
              OUT PATHDATA  *ppd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bEnumClipLines(IN PATHOBJ  *ppo,
                       IN ULONG  cb,
                       OUT CLIPLINE  *pcl)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bMoveTo(IN PATHOBJ  *ppo,
                IN POINTFIX  ptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyBezierTo(IN PATHOBJ  *ppo,
                      IN POINTFIX  *pptfx,
                      IN ULONG  cptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
PATHOBJ_bPolyLineTo(IN PATHOBJ  *ppo,
                    IN POINTFIX  *pptfx,
                    IN ULONG  cptfx)
{
    UNIMPLEMENTED;
	return FALSE;
}

VOID
APIENTRY
PATHOBJ_vEnumStart(IN PATHOBJ  *ppo)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vEnumStartClipLines(IN PATHOBJ  *ppo,
                            IN CLIPOBJ  *pco,
                            IN SURFOBJ  *pso,
                            IN LINEATTRS  *pla)
{
    UNIMPLEMENTED;
}

VOID
APIENTRY
PATHOBJ_vGetBounds(IN PATHOBJ  *ppo,
                   OUT PRECTFX  prectfx)
{
    UNIMPLEMENTED;
}
