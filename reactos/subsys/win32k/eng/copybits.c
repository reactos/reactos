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
/* $Id: copybits.c,v 1.25 2004/07/03 13:55:35 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI EngCopyBits Function
 * FILE:             subsys/win32k/eng/copybits.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/18/1999: Created
 */
#include <w32k.h>

/*
 * @implemented
 */
BOOL STDCALL
EngCopyBits(SURFOBJ *Dest,
	    SURFOBJ *Source,
	    CLIPOBJ *Clip,
	    XLATEOBJ *ColorTranslation,
	    RECTL *DestRect,
	    POINTL *SourcePoint)
{
  BOOLEAN   ret;
  BYTE      clippingType;
  RECTL     rclTmp;
  POINTL    ptlTmp;
  RECT_ENUM RectEnum;
  BOOL      EnumMore;
  POINTL BrushOrigin;

  MouseSafetyOnDrawStart(Source, SourcePoint->x, SourcePoint->y,
                         (SourcePoint->x + abs(DestRect->right - DestRect->left)),
                         (SourcePoint->y + abs(DestRect->bottom - DestRect->top)));
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
      /* FIXME: Remove typecast. */
      if (((BITMAPOBJ*)Dest)->flHooks & HOOK_COPYBITS)
      {
        ret = GDIDEVFUNCS(Dest).CopyBits(
          Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint);

        MouseSafetyOnDrawEnd(Dest);
        MouseSafetyOnDrawEnd(Source);

        return ret;
      }
    }

    // Source surface is device managed
    if(Source->iType!=STYPE_BITMAP)
    {
      /* FIXME: Eng* functions shouldn't call Drv* functions. ? */
      /* FIXME: Remove typecast. */
      if (((BITMAPOBJ*)Source)->flHooks & HOOK_COPYBITS)
      {
        ret = GDIDEVFUNCS(Source).CopyBits(
          Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint);

        MouseSafetyOnDrawEnd(Dest);
        MouseSafetyOnDrawEnd(Source);

        return ret;
      }
    }

    // If CopyBits wasn't hooked, BitBlt must be
    /* FIXME: Remove the typecast! */
    ret = IntEngBitBlt((BITMAPOBJ*)Dest, (BITMAPOBJ*)Source,
                       NULL, Clip, ColorTranslation, DestRect, SourcePoint,
                       NULL, NULL, NULL, 0);

    MouseSafetyOnDrawEnd(Dest);
    MouseSafetyOnDrawEnd(Source);

    return ret;
  }

  // Determine clipping type
  if (Clip == (CLIPOBJ *) NULL)
  {
    clippingType = DC_TRIVIAL;
  } else {
    clippingType = Clip->iDComplexity;
  }

  BrushOrigin.x = BrushOrigin.y = 0;

  switch(clippingType)
    {
      case DC_TRIVIAL:
        DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBlt(
          Dest, Source, DestRect, SourcePoint, NULL, BrushOrigin, ColorTranslation, SRCCOPY);

        MouseSafetyOnDrawEnd(Dest);
        MouseSafetyOnDrawEnd(Source);

        return(TRUE);

      case DC_RECT:
        // Clip the blt to the clip rectangle
        EngIntersectRect(&rclTmp, DestRect, &Clip->rclBounds);

        ptlTmp.x = SourcePoint->x + rclTmp.left - DestRect->left;
        ptlTmp.y = SourcePoint->y + rclTmp.top  - DestRect->top;

        DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBlt(
          Dest, Source, &rclTmp, &ptlTmp, NULL, BrushOrigin, ColorTranslation, SRCCOPY);

        MouseSafetyOnDrawEnd(Dest);
        MouseSafetyOnDrawEnd(Source);

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
              EngIntersectRect(prcl, prcl, DestRect);

              ptlTmp.x = SourcePoint->x + prcl->left - DestRect->left;
              ptlTmp.y = SourcePoint->y + prcl->top - DestRect->top;

              if(!DibFunctionsForBitmapFormat[Dest->iBitmapFormat].DIB_BitBlt(
                    Dest, Source, prcl, &ptlTmp, NULL, BrushOrigin, ColorTranslation, SRCCOPY)) return FALSE;

              prcl++;

              } while (prcl < prclEnd);
            }

          } while(EnumMore);

          MouseSafetyOnDrawEnd(Dest);
          MouseSafetyOnDrawEnd(Source);

          return(TRUE);
    }

  MouseSafetyOnDrawEnd(Dest);
  MouseSafetyOnDrawEnd(Source);

  return FALSE;
}

/* EOF */
