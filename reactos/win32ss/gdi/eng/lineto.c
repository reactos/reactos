/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Line functions
 * FILE:             subsystems/win32/win32k/eng/lineto.c
 * PROGRAMER:        ReactOS Team
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static void FASTCALL
TranslateRects(RECT_ENUM *RectEnum, POINTL* Translate)
{
    RECTL* CurrentRect;

    if (0 != Translate->x || 0 != Translate->y)
    {
        for (CurrentRect = RectEnum->arcl; CurrentRect < RectEnum->arcl + RectEnum->c; CurrentRect++)
        {
            CurrentRect->left += Translate->x;
            CurrentRect->right += Translate->x;
            CurrentRect->top += Translate->y;
            CurrentRect->bottom += Translate->y;
        }
    }
}

/*
 * Draw a line from top-left to bottom-right
 */
void FASTCALL
NWtoSE(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
    int i;
    int error;
    BOOLEAN EnumMore;
    RECTL* ClipRect;
    RECT_ENUM RectEnum;
    ULONG Pixel = pbo->iSolidColor;
    LONG delta;

    CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
    EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    TranslateRects(&RectEnum, Translate);
    ClipRect = RectEnum.arcl;
    delta = max(deltax, deltay);
    i = 0;
    error = delta >> 1;
    while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
        while ((ClipRect < RectEnum.arcl + RectEnum.c /* there's still a current clip rect */
                && (ClipRect->bottom <= y             /* but it's above us */
                    || (ClipRect->top <= y && ClipRect->right <= x))) /* or to the left of us */
                || EnumMore)                           /* no current clip rect, but rects left */
        {
            /* Skip to the next clip rect */
            if (RectEnum.arcl + RectEnum.c <= ClipRect)
            {
                EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
                TranslateRects(&RectEnum, Translate);
                ClipRect = RectEnum.arcl;
            }
            else
            {
                ClipRect++;
            }
        }
        if (ClipRect < RectEnum.arcl + RectEnum.c) /* If there's no current clip rect we're done */
        {
            if (ClipRect->left <= x && ClipRect->top <= y)
            {
                DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_PutPixel(
                    OutputObj, x, y, Pixel);
            }
            if (deltax < deltay)
            {
                y++;
                error = error + deltax;
                if (deltay <= error)
                {
                    x++;
                    error = error - deltay;
                }
            }
            else
            {
                x++;
                error = error + deltay;
                if (deltax <= error)
                {
                    y++;
                    error = error - deltax;
                }
            }
            i++;
        }
    }
}

void FASTCALL
SWtoNE(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
    int i;
    int error;
    BOOLEAN EnumMore;
    RECTL* ClipRect;
    RECT_ENUM RectEnum;
    ULONG Pixel = pbo->iSolidColor;
    LONG delta;

    CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTUP, 0);
    EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    TranslateRects(&RectEnum, Translate);
    ClipRect = RectEnum.arcl;
    delta = max(deltax, deltay);
    i = 0;
    error = delta >> 1;
    while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
        while ((ClipRect < RectEnum.arcl + RectEnum.c
                && (y < ClipRect->top
                    || (y < ClipRect->bottom && ClipRect->right <= x)))
                || EnumMore)
        {
            if (RectEnum.arcl + RectEnum.c <= ClipRect)
            {
                EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
                TranslateRects(&RectEnum, Translate);
                ClipRect = RectEnum.arcl;
            }
            else
            {
                ClipRect++;
            }
        }
        if (ClipRect < RectEnum.arcl + RectEnum.c)
        {
            if (ClipRect->left <= x && y < ClipRect->bottom)
            {
                DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_PutPixel(
                    OutputObj, x, y, Pixel);
            }
            if (deltax < deltay)
            {
                y--;
                error = error + deltax;
                if (deltay <= error)
                {
                    x++;
                    error = error - deltay;
                }
            }
            else
            {
                x++;
                error = error + deltay;
                if (deltax <= error)
                {
                    y--;
                    error = error - deltax;
                }
            }
            i++;
        }
    }
}

void FASTCALL
NEtoSW(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
    int i;
    int error;
    BOOLEAN EnumMore;
    RECTL* ClipRect;
    RECT_ENUM RectEnum;
    ULONG Pixel = pbo->iSolidColor;
    LONG delta;

    CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTDOWN, 0);
    EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    TranslateRects(&RectEnum, Translate);
    ClipRect = RectEnum.arcl;
    delta = max(deltax, deltay);
    i = 0;
    error = delta >> 1;
    while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
        while ((ClipRect < RectEnum.arcl + RectEnum.c
                && (ClipRect->bottom <= y
                    || (ClipRect->top <= y && x < ClipRect->left)))
                || EnumMore)
        {
            if (RectEnum.arcl + RectEnum.c <= ClipRect)
            {
                EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
                TranslateRects(&RectEnum, Translate);
                ClipRect = RectEnum.arcl;
            }
            else
            {
                ClipRect++;
            }
        }
        if (ClipRect < RectEnum.arcl + RectEnum.c)
        {
            if (x < ClipRect->right && ClipRect->top <= y)
            {
                DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_PutPixel(
                    OutputObj, x, y, Pixel);
            }
            if (deltax < deltay)
            {
                y++;
                error = error + deltax;
                if (deltay <= error)
                {
                    x--;
                    error = error - deltay;
                }
            }
            else
            {
                x--;
                error = error + deltay;
                if (deltax <= error)
                {
                    y++;
                    error = error - deltax;
                }
            }
            i++;
        }
    }
}

void FASTCALL
SEtoNW(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate)
{
    int i;
    int error;
    BOOLEAN EnumMore;
    RECTL* ClipRect;
    RECT_ENUM RectEnum;
    ULONG Pixel = pbo->iSolidColor;
    LONG delta;

    CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTUP, 0);
    EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    TranslateRects(&RectEnum, Translate);
    ClipRect = RectEnum.arcl;
    delta = max(deltax, deltay);
    i = 0;
    error = delta >> 1;
    while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
        while ((ClipRect < RectEnum.arcl + RectEnum.c
                && (y < ClipRect->top
                    || (y < ClipRect->bottom && x < ClipRect->left)))
                || EnumMore)
        {
            if (RectEnum.arcl + RectEnum.c <= ClipRect)
            {
                EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
                TranslateRects(&RectEnum, Translate);
                ClipRect = RectEnum.arcl;
            }
            else
            {
                ClipRect++;
            }
        }
        if (ClipRect < RectEnum.arcl + RectEnum.c)
        {
            if (x < ClipRect->right && y < ClipRect->bottom)
            {
                DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_PutPixel(
                    OutputObj, x, y, Pixel);
            }
            if (deltax < deltay)
            {
                y--;
                error = error + deltax;
                if (deltay <= error)
                {
                    x--;
                    error = error - deltay;
                }
            }
            else
            {
                x--;
                error = error + deltay;
                if (deltax <= error)
                {
                    y--;
                    error = error - deltax;
                }
            }
            i++;
        }
    }
}

/*
 * @implemented
 */
BOOL APIENTRY
EngLineTo(
    _Inout_ SURFOBJ *DestObj,
    _In_ CLIPOBJ *Clip,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_opt_ RECTL *RectBounds,
    _In_ MIX mix)
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
        Clip = pcoPriv = EngCreateClip();
        if (!Clip)
        {
            return FALSE;
        }
        IntEngUpdateClipRegion((XCLIPOBJ*)Clip, 0, 0, RectBounds);
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
        EngDeleteClip(pcoPriv);
    }

    return IntEngLeave(&EnterLeave);
}

BOOL APIENTRY
IntEngLineTo(SURFOBJ *psoDest,
             CLIPOBJ *ClipObj,
             BRUSHOBJ *pbo,
             LONG x1,
             LONG y1,
             LONG x2,
             LONG y2,
             RECTL *RectBounds,
             MIX Mix)
{
    BOOLEAN ret;
    SURFACE *psurfDest;
    PEBRUSHOBJ GdiBrush;
    RECTL b;

    ASSERT(psoDest);
    psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    ASSERT(psurfDest);

    GdiBrush = CONTAINING_RECORD(
                   pbo,
                   EBRUSHOBJ,
                   BrushObject);
    ASSERT(GdiBrush);
    ASSERT(GdiBrush->pbrush);

    if (GdiBrush->pbrush->flAttrs & BR_IS_NULL)
        return TRUE;

    /* No success yet */
    ret = FALSE;

    /* Clip lines totally outside the clip region. This is not done as an
     * optimization (there are very few lines drawn outside the region) but
     * as a workaround for what seems to be a problem in the CL54XX driver */
    if (NULL == ClipObj || DC_TRIVIAL == ClipObj->iDComplexity)
    {
        b.left = 0;
        b.right = psoDest->sizlBitmap.cx;
        b.top = 0;
        b.bottom = psoDest->sizlBitmap.cy;
    }
    else
    {
        b = ClipObj->rclBounds;
    }
    if ((x1 < b.left && x2 < b.left) || (b.right <= x1 && b.right <= x2) ||
            (y1 < b.top && y2 < b.top) || (b.bottom <= y1 && b.bottom <= y2))
    {
        return TRUE;
    }

    b.left = min(x1, x2);
    b.right = max(x1, x2);
    b.top = min(y1, y2);
    b.bottom = max(y1, y2);
    if (b.left == b.right) b.right++;
    if (b.top == b.bottom) b.bottom++;

    if (psurfDest->flags & HOOK_LINETO)
    {
        /* Call the driver's DrvLineTo */
        ret = GDIDEVFUNCS(psoDest).LineTo(
                  psoDest, ClipObj, pbo, x1, y1, x2, y2, &b, Mix);
    }

#if 0
    if (! ret && (psurfDest->flags & HOOK_STROKEPATH))
    {
        /* FIXME: Emulate LineTo using drivers DrvStrokePath and set ret on success */
    }
#endif

    if (! ret)
    {
        ret = EngLineTo(psoDest, ClipObj, pbo, x1, y1, x2, y2, RectBounds, Mix);
    }

    return ret;
}

BOOL APIENTRY
IntEngPolyline(SURFOBJ *psoDest,
               CLIPOBJ *Clip,
               BRUSHOBJ *pbo,
               CONST LPPOINT  pt,
               LONG dCount,
               MIX Mix)
{
    LONG i;
    RECTL rect;
    BOOL ret = FALSE;

    // Draw the Polyline with a call to IntEngLineTo for each segment.
    for (i = 1; i < dCount; i++)
    {
        rect.left = min(pt[i-1].x, pt[i].x);
        rect.top = min(pt[i-1].y, pt[i].y);
        rect.right = max(pt[i-1].x, pt[i].x);
        rect.bottom = max(pt[i-1].y, pt[i].y);
        ret = IntEngLineTo(psoDest,
                           Clip,
                           pbo,
                           pt[i-1].x,
                           pt[i-1].y,
                           pt[i].x,
                           pt[i].y,
                           &rect,
                           Mix);
        if (!ret)
        {
            break;
        }
    }

    return ret;
}

/* EOF */
