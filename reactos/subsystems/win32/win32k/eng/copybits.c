/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI EngCopyBits Function
 * FILE:             subsys/win32k/eng/copybits.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/18/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOL APIENTRY
EngCopyBits(SURFOBJ *Dest,
	    SURFOBJ *Source,
	    CLIPOBJ *Clip,
	    XLATEOBJ *ColorTranslation,
	    RECTL *DestRect,
	    POINTL *SourcePoint)
{
  BOOLEAN   ret;
  BYTE      clippingType;
  RECT_ENUM RectEnum;
  BOOL      EnumMore;
  BLTINFO   BltInfo;
  BITMAPOBJ *DestObj;
  BITMAPOBJ *SourceObj;

  ASSERT(Dest != NULL && Source != NULL && DestRect != NULL && SourcePoint != NULL);

  SourceObj = CONTAINING_RECORD(Source, BITMAPOBJ, SurfObj);
  BITMAPOBJ_LockBitmapBits(SourceObj);
  MouseSafetyOnDrawStart(Source, SourcePoint->x, SourcePoint->y,
                         (SourcePoint->x + abs(DestRect->right - DestRect->left)),
                         (SourcePoint->y + abs(DestRect->bottom - DestRect->top)));

  DestObj = CONTAINING_RECORD(Dest, BITMAPOBJ, SurfObj);
  if (Dest != Source)
  {
    BITMAPOBJ_LockBitmapBits(DestObj);
  }

  MouseSafetyOnDrawStart(Dest, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

  // FIXME: Don't punt to the driver's DrvCopyBits immediately. Instead,
  //        mark the copy block function to be DrvCopyBits instead of the
  //        GDI's copy bit function so as to remove clipping from the
  //        driver's responsibility

  // If one of the surfaces isn't managed by the GDI
  if((Dest->iType!=STYPE_BITMAP) || (Source->iType!=STYPE_BITMAP))
  {
    // Destination surface is device managed
    if(Dest->iType!=STYPE_BITMAP)
    {
      /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
      if (DestObj->flHooks & HOOK_COPYBITS)
      {
        ret = GDIDEVFUNCS(Dest).CopyBits(
          Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint);

        MouseSafetyOnDrawEnd(Dest);
        if (Dest != Source)
        {
          BITMAPOBJ_UnlockBitmapBits(DestObj);
        }
        MouseSafetyOnDrawEnd(Source);
        BITMAPOBJ_UnlockBitmapBits(SourceObj);

        return ret;
      }
    }

    // Source surface is device managed
    if(Source->iType!=STYPE_BITMAP)
    {
      /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
      if (SourceObj->flHooks & HOOK_COPYBITS)
      {
        ret = GDIDEVFUNCS(Source).CopyBits(
          Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint);

        MouseSafetyOnDrawEnd(Dest);
        if (Dest != Source)
        {
          BITMAPOBJ_UnlockBitmapBits(DestObj);
        }
        MouseSafetyOnDrawEnd(Source);
        BITMAPOBJ_UnlockBitmapBits(SourceObj);

        return ret;
      }
    }

    // If CopyBits wasn't hooked, BitBlt must be
    ret = IntEngBitBlt(Dest, Source,
                       NULL, Clip, ColorTranslation, DestRect, SourcePoint,
                       NULL, NULL, NULL, ROP3_TO_ROP4(SRCCOPY));

    MouseSafetyOnDrawEnd(Dest);
    if (Dest != Source)
    {
      BITMAPOBJ_UnlockBitmapBits(DestObj);
    }
    MouseSafetyOnDrawEnd(Source);
    BITMAPOBJ_UnlockBitmapBits(SourceObj);

    return ret;
  }

  // Determine clipping type
  if (Clip == (CLIPOBJ *) NULL)
  {
    clippingType = DC_TRIVIAL;
  } else {
    clippingType = Clip->iDComplexity;
  }

  BltInfo.DestSurface = Dest;
  BltInfo.SourceSurface = Source;
  BltInfo.PatternSurface = NULL;
  BltInfo.XlateSourceToDest = ColorTranslation;
  BltInfo.XlatePatternToDest = NULL;
  BltInfo.Rop4 = SRCCOPY;

  switch(clippingType)
    {
      case DC_TRIVIAL:
        BltInfo.DestRect = *DestRect;
        BltInfo.SourcePoint = *SourcePoint;

        DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

        MouseSafetyOnDrawEnd(Dest);
        if (Dest != Source)
        {
          BITMAPOBJ_UnlockBitmapBits(DestObj);
        }
        MouseSafetyOnDrawEnd(Source);
        BITMAPOBJ_UnlockBitmapBits(SourceObj);

        return(TRUE);

      case DC_RECT:
        // Clip the blt to the clip rectangle
        EngIntersectRect(&BltInfo.DestRect, DestRect, &Clip->rclBounds);

        BltInfo.SourcePoint.x = SourcePoint->x + BltInfo.DestRect.left - DestRect->left;
        BltInfo.SourcePoint.y = SourcePoint->y + BltInfo.DestRect.top  - DestRect->top;

        DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo);

        MouseSafetyOnDrawEnd(Dest);
        if (Dest != Source)
        {
          BITMAPOBJ_UnlockBitmapBits(DestObj);
        }
        MouseSafetyOnDrawEnd(Source);
        BITMAPOBJ_UnlockBitmapBits(SourceObj);

        return(TRUE);

      case DC_COMPLEX:

        CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

          if (RectEnum.c > 0)
          {
            RECTL* prclEnd = &RectEnum.arcl[RectEnum.c];
            RECTL* prcl    = &RectEnum.arcl[0];

            do {
              EngIntersectRect(&BltInfo.DestRect, prcl, DestRect);

              BltInfo.SourcePoint.x = SourcePoint->x + prcl->left - DestRect->left;
              BltInfo.SourcePoint.y = SourcePoint->y + prcl->top - DestRect->top;

              if(!DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBltSrcCopy(&BltInfo))
                return FALSE;

              prcl++;

              } while (prcl < prclEnd);
            }

          } while(EnumMore);

          MouseSafetyOnDrawEnd(Dest);
          if (Dest != Source)
          {
            BITMAPOBJ_UnlockBitmapBits(DestObj);
          }
          MouseSafetyOnDrawEnd(Source);
          BITMAPOBJ_UnlockBitmapBits(SourceObj);

          return(TRUE);
    }

  MouseSafetyOnDrawEnd(Dest);
  if (Dest != Source)
  {
    BITMAPOBJ_UnlockBitmapBits(DestObj);
  }
  MouseSafetyOnDrawEnd(Source);
  BITMAPOBJ_UnlockBitmapBits(SourceObj);

  return FALSE;
}

/* EOF */
