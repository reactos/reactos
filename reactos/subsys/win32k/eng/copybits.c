/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI EngCopyBits Function
 * FILE:             subsys/win32k/eng/copybits.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/18/1999: Created
 */

#include <ddk/winddi.h>
#include "objects.h"
#include "enum.h"

VOID CopyBitsCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                  SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                  PRECTL  DestRect,  POINTL  *SourcePoint,
                  ULONG   Delta,     XLATEOBJ *ColorTranslation)
{
   ULONG dy, leftOfSource, leftOfDest, Width, SourceBPP, DestBPP, RGBulong = 0, idxColor, i, TrivialCopy = 0;
   BYTE  *SourcePos, *SourceInitial, *DestPos, *DestInitial;

   // FIXME: Get ColorTranslation passed here and do something with it

   if(ColorTranslation == NULL)
   {
      TrivialCopy = 1;
   } else if(ColorTranslation->flXlate & XO_TRIVIAL)
   {
      TrivialCopy = 1;
   }

   leftOfSource = SourcePoint->x * SourceGDI->BytesPerPixel;
   leftOfDest   = DestRect->left * DestGDI->BytesPerPixel;
   Width        = (DestRect->right - DestRect->left) * DestGDI->BytesPerPixel;

   if(TrivialCopy == 1)
   {
      for(dy=DestRect->top; dy<DestRect->bottom; dy++)
      {
         memcpy(DestSurf->pvBits+Delta*dy+leftOfDest,
                SourceSurf->pvBits+Delta*dy+leftOfSource,
                Width);
      }
   } else
   if(ColorTranslation->flXlate & XO_TABLE)
   {
      SourceBPP = bytesPerPixel(SourceSurf->iBitmapFormat);
      DestBPP   = bytesPerPixel(DestSurf->iBitmapFormat);

      SourcePos = SourceSurf->pvBits +
                  (SourcePoint->y * SourceSurf->lDelta) + (SourcePoint->x * SourceBPP);
      SourceInitial = SourcePos;

      DestPos = DestSurf->pvBits +
                ((DestRect->bottom - DestRect->top) * DestSurf->lDelta) + (DestRect->left * DestBPP);
      DestInitial = DestPos;

      for(i=DestRect->left; i<DestRect->right; i++)
      {
         memcpy(&RGBulong, SourcePos, SourceBPP);
         idxColor = XLATEOBJ_iXlate(ColorTranslation, RGBulong);
         memcpy(DestPos, &idxColor, DestBPP);

         SourcePos+=SourceBPP;
         DestPos+=DestBPP;
      }
      SourcePos = SourceInitial + SourceSurf->lDelta;
      DestPos   = DestInitial   + DestSurf->lDelta;
   }
}

BOOL EngCopyBits(SURFOBJ *Dest, SURFOBJ *Source,
                 CLIPOBJ *Clip, XLATEOBJ *ColorTranslation,
                 RECTL *DestRect, POINTL *SourcePoint)
{
   SURFGDI   *DestGDI, *SourceGDI;
   BYTE      clippingType;
   RECTL     rclTmp;
   POINTL    ptlTmp;
   RECT_ENUM RectEnum;
   BOOL      EnumMore;

   // FIXME: Do color translation -- refer to eng\bitblt.c

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
         DestGDI = AccessInternalObjectFromUserObject(Dest);

         if (DestGDI->CopyBits!=NULL)
         {
            return DestGDI->CopyBits(Dest, Source, Clip,
                                     ColorTranslation, DestRect, SourcePoint);
         }
      }

      // Source surface is device managed
      if(Source->iType!=STYPE_BITMAP)
      {
         SourceGDI = AccessInternalObjectFromUserObject(Source);

         if (SourceGDI->CopyBits!=NULL)
         {
            return SourceGDI->CopyBits(Dest, Source, Clip,
                                       ColorTranslation, DestRect, SourcePoint);
         }
      }

      // If CopyBits wasn't hooked, BitBlt must be
      return EngBitBlt(Dest, Source,
                       NULL, Clip, ColorTranslation, DestRect, SourcePoint,
                       NULL, NULL, NULL, NULL);
   }

   // Determine clipping type
   if (Clip == (CLIPOBJ *) NULL)
   {
      clippingType = DC_TRIVIAL;
   } else {
      clippingType = Clip->iDComplexity;
   }

   // We only handle XO_TABLE translations at the momement
   if ((ColorTranslation == NULL) || (ColorTranslation->flXlate & XO_TRIVIAL) ||
       (ColorTranslation->flXlate & XO_TABLE))
   {
      SourceGDI = AccessInternalObjectFromUserObject(Source);
      DestGDI   = AccessInternalObjectFromUserObject(Dest);

      switch(clippingType)
      {
         case DC_TRIVIAL:
            CopyBitsCopy(Dest,     Source,
                         DestGDI,  SourceGDI,
                         DestRect, SourcePoint, Source->lDelta, ColorTranslation);

            return(TRUE);

         case DC_RECT:

            // Clip the blt to the clip rectangle

            EngIntersectRect(&rclTmp, DestRect, &Clip->rclBounds);

            ptlTmp.x = SourcePoint->x + rclTmp.left - DestRect->left;
            ptlTmp.y = SourcePoint->y + rclTmp.top  - DestRect->top;

            CopyBitsCopy(Dest,    Source,
                         DestGDI, SourceGDI,
                         &rclTmp, &ptlTmp, Source->lDelta, ColorTranslation);

            return(TRUE);

         case DC_COMPLEX:

            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
            CD_ANY, ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum),
               (PVOID) &RectEnum);

               if (RectEnum.c > 0)
               {
                  RECTL* prclEnd = &RectEnum.arcl[RectEnum.c];
                  RECTL* prcl    = &RectEnum.arcl[0];

                  do {
                     EngIntersectRect(prcl, prcl, DestRect);

                     ptlTmp.x = SourcePoint->x + prcl->left
                                - DestRect->left;
                     ptlTmp.y = SourcePoint->y + prcl->top
                                - DestRect->top;

                     CopyBitsCopy(Dest,    Source,
                                  DestGDI, SourceGDI,
                                  prcl, &ptlTmp, Source->lDelta, ColorTranslation);

                     prcl++;

                  } while (prcl < prclEnd);
               }

            } while(EnumMore);

            return(TRUE);
      }
   }

   return FALSE;
}
