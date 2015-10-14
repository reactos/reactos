/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 *                  Based on gdi/gdiobj.c from Wine:
 *                  Copyright 1993 Alexandre Julliard
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

extern PDEVOBJ PrimarySurface;

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
GreLineTo(SURFOBJ *psoDest,
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

    GdiBrush = CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject);
    ASSERT(GdiBrush);

    /* Don't do anything if null pen is selected */
    if (GdiBrush->flattrs & GDIBRUSH_IS_NULL)
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

    SURFACE_LockBitmapBits(psurfDest);
    MouseSafetyOnDrawStart(psoDest, x1, y1, x2, y2);

    if (psurfDest->flHooks & HOOK_LINETO)
    {
        /* Call the driver's DrvLineTo */
        ret = GDIDEVFUNCS(psoDest).LineTo(
                  psoDest, ClipObj, pbo, x1, y1, x2, y2, &b, Mix);
    }

    if (!ret)
    {
        ret = EngLineTo(psoDest, ClipObj, pbo, x1, y1, x2, y2, RectBounds, Mix);
    }

    MouseSafetyOnDrawEnd(psoDest);
    SURFACE_UnlockBitmapBits(psurfDest);

    return ret;
}

VOID
NTAPI
GrePolyline(PDC pDC,
           const POINT *ptPoints,
           INT count)
{
    POINT ptLine[2];
    RECTL DestRect;
    MIX Mix;
    INT i;

    /* Draw pen-based polygon */
    if (!(pDC->dclevel.pbrLine->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = ROP2_TO_MIX(R2_COPYPEN);/*pdcattr->jROP2*/
        for (i=0; i<count-1; i++)
        {
            ptLine[0].x = ptPoints[i].x + pDC->ptlDCOrig.x;
            ptLine[0].y = ptPoints[i].y + pDC->ptlDCOrig.y;
            ptLine[1].x = ptPoints[i+1].x + pDC->ptlDCOrig.x;
            ptLine[1].y = ptPoints[i+1].y + pDC->ptlDCOrig.y;

            DestRect.left = min(ptLine[0].x, ptLine[1].x);
            DestRect.top = min(ptLine[0].y, ptLine[1].y);
            DestRect.right = max(ptLine[0].x, ptLine[1].x);
            DestRect.bottom = max(ptLine[0].y, ptLine[1].y);

            GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                             pDC->CombinedClip,
                             &pDC->eboLine.BrushObject,
                             ptLine[0].x,
                             ptLine[0].y,
                             ptLine[1].x,
                             ptLine[1].y,
                             &DestRect,
                             Mix);
        }
    }
}

BOOL APIENTRY
IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
            SURFOBJ *psoDest,
            RECTL *DestRect,
            BOOL ReadOnly,
            POINTL *Translate,
            SURFOBJ **ppsoOutput)
{
  LONG Exchange;
  SIZEL BitmapSize;
  POINTL SrcPoint;
  LONG Width;
  RECTL ClippedDestRect;

  /* Normalize */
  if (DestRect->right < DestRect->left)
    {
    Exchange = DestRect->left;
    DestRect->left = DestRect->right;
    DestRect->right = Exchange;
    }
  if (DestRect->bottom < DestRect->top)
    {
    Exchange = DestRect->top;
    DestRect->top = DestRect->bottom;
    DestRect->bottom = Exchange;
    }

  if (NULL != psoDest && STYPE_BITMAP != psoDest->iType &&
      (NULL == psoDest->pvScan0 || 0 == psoDest->lDelta))
    {
    /* Driver needs to support DrvCopyBits, else we can't do anything */
    SURFACE *psurfDest = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    if (!(psurfDest->flHooks & HOOK_COPYBITS))
    {
      return FALSE;
    }

    /* Allocate a temporary bitmap */
    BitmapSize.cx = DestRect->right - DestRect->left;
    BitmapSize.cy = DestRect->bottom - DestRect->top;
    Width = DIB_GetDIBWidthBytes(BitmapSize.cx, BitsPerFormat(psoDest->iBitmapFormat));
    EnterLeave->OutputBitmap = EngCreateBitmap(BitmapSize, Width,
                                               psoDest->iBitmapFormat,
                                               BMF_TOPDOWN | BMF_NOZEROINIT, NULL);

    if (!EnterLeave->OutputBitmap)
      {
      DPRINT1("EngCreateBitmap() failed\n");
      return FALSE;
      }

    *ppsoOutput = EngLockSurface((HSURF)EnterLeave->OutputBitmap);

    EnterLeave->DestRect.left = 0;
    EnterLeave->DestRect.top = 0;
    EnterLeave->DestRect.right = BitmapSize.cx;
    EnterLeave->DestRect.bottom = BitmapSize.cy;
    SrcPoint.x = DestRect->left;
    SrcPoint.y = DestRect->top;
    ClippedDestRect = EnterLeave->DestRect;
    if (SrcPoint.x < 0)
      {
        ClippedDestRect.left -= SrcPoint.x;
        SrcPoint.x = 0;
      }
    if (psoDest->sizlBitmap.cx < SrcPoint.x + ClippedDestRect.right - ClippedDestRect.left)
      {
        ClippedDestRect.right = ClippedDestRect.left + psoDest->sizlBitmap.cx - SrcPoint.x;
      }
    if (SrcPoint.y < 0)
      {
        ClippedDestRect.top -= SrcPoint.y;
        SrcPoint.y = 0;
      }
    if (psoDest->sizlBitmap.cy < SrcPoint.y + ClippedDestRect.bottom - ClippedDestRect.top)
      {
        ClippedDestRect.bottom = ClippedDestRect.top + psoDest->sizlBitmap.cy - SrcPoint.y;
      }
    EnterLeave->TrivialClipObj = EngCreateClip();
    EnterLeave->TrivialClipObj->iDComplexity = DC_TRIVIAL;
    if (ClippedDestRect.left < (*ppsoOutput)->sizlBitmap.cx &&
        0 <= ClippedDestRect.right &&
        SrcPoint.x < psoDest->sizlBitmap.cx &&
        ClippedDestRect.top <= (*ppsoOutput)->sizlBitmap.cy &&
        0 <= ClippedDestRect.bottom &&
        SrcPoint.y < psoDest->sizlBitmap.cy &&
        ! GDIDEVFUNCS(psoDest).CopyBits(
                                        *ppsoOutput, psoDest,
                                        EnterLeave->TrivialClipObj, NULL,
                                        &ClippedDestRect, &SrcPoint))
      {
      EngDeleteClip(EnterLeave->TrivialClipObj);
      EngFreeMem((*ppsoOutput)->pvBits);
      EngUnlockSurface(*ppsoOutput);
      EngDeleteSurface((HSURF)EnterLeave->OutputBitmap);
      return FALSE;
      }
    EnterLeave->DestRect.left = DestRect->left;
    EnterLeave->DestRect.top = DestRect->top;
    EnterLeave->DestRect.right = DestRect->right;
    EnterLeave->DestRect.bottom = DestRect->bottom;
    Translate->x = - DestRect->left;
    Translate->y = - DestRect->top;
    }
  else
    {
    Translate->x = 0;
    Translate->y = 0;
    *ppsoOutput = psoDest;
    }

  if (NULL != *ppsoOutput)
  {
    SURFACE* psurfOutput = CONTAINING_RECORD(*ppsoOutput, SURFACE, SurfObj);
    if (0 != (psurfOutput->flHooks & HOOK_SYNCHRONIZE))
    {
      if (NULL != GDIDEVFUNCS(*ppsoOutput).SynchronizeSurface)
        {
          GDIDEVFUNCS(*ppsoOutput).SynchronizeSurface(*ppsoOutput, DestRect, 0);
        }
      else if (STYPE_BITMAP == (*ppsoOutput)->iType
               && NULL != GDIDEVFUNCS(*ppsoOutput).Synchronize)
        {
          GDIDEVFUNCS(*ppsoOutput).Synchronize((*ppsoOutput)->dhpdev, DestRect);
        }
    }
  }
  else return FALSE;

  EnterLeave->DestObj = psoDest;
  EnterLeave->OutputObj = *ppsoOutput;
  EnterLeave->ReadOnly = ReadOnly;

  return TRUE;
}

BOOL APIENTRY
IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave)
{
  POINTL SrcPoint;
  BOOL Result = TRUE;

  if (EnterLeave->OutputObj != EnterLeave->DestObj && NULL != EnterLeave->OutputObj)
    {
    if (! EnterLeave->ReadOnly)
      {
      SrcPoint.x = 0;
      SrcPoint.y = 0;
      if (EnterLeave->DestRect.left < 0)
        {
          SrcPoint.x = - EnterLeave->DestRect.left;
          EnterLeave->DestRect.left = 0;
        }
      if (EnterLeave->DestObj->sizlBitmap.cx < EnterLeave->DestRect.right)
        {
          EnterLeave->DestRect.right = EnterLeave->DestObj->sizlBitmap.cx;
        }
      if (EnterLeave->DestRect.top < 0)
        {
          SrcPoint.y = - EnterLeave->DestRect.top;
          EnterLeave->DestRect.top = 0;
        }
      if (EnterLeave->DestObj->sizlBitmap.cy < EnterLeave->DestRect.bottom)
        {
          EnterLeave->DestRect.bottom = EnterLeave->DestObj->sizlBitmap.cy;
        }
      if (SrcPoint.x < EnterLeave->OutputObj->sizlBitmap.cx &&
          EnterLeave->DestRect.left <= EnterLeave->DestRect.right &&
          EnterLeave->DestRect.left < EnterLeave->DestObj->sizlBitmap.cx &&
          SrcPoint.y < EnterLeave->OutputObj->sizlBitmap.cy &&
          EnterLeave->DestRect.top <= EnterLeave->DestRect.bottom &&
          EnterLeave->DestRect.top < EnterLeave->DestObj->sizlBitmap.cy)
        {
          Result = GDIDEVFUNCS(EnterLeave->DestObj).CopyBits(
                                                 EnterLeave->DestObj,
                                                 EnterLeave->OutputObj,
                                                 EnterLeave->TrivialClipObj, NULL,
                                                 &EnterLeave->DestRect, &SrcPoint);
        }
      else
        {
          Result = TRUE;
        }
      }
    EngFreeMem(EnterLeave->OutputObj->pvBits);
    EngUnlockSurface(EnterLeave->OutputObj);
    EngDeleteSurface((HSURF)EnterLeave->OutputBitmap);
    EngDeleteClip(EnterLeave->TrivialClipObj);
    }
  else
    {
    Result = TRUE;
    }

  return Result;
}

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
void NTAPI
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

void NTAPI
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

void NTAPI
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

void NTAPI
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

/* EOF */
