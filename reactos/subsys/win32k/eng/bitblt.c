/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI BitBlt Functions
 * FILE:             subsys/win32k/eng/bitblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        2/10/1999: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ntos/minmax.h>
#include "brush.h"
#include "enum.h"
#include "objects.h"

VOID BitBltCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                PRECTL  DestRect,  POINTL  *SourcePoint,
                ULONG   Delta)
{
   ULONG dy, leftOfSource, leftOfDest, Width, CopyPos;

   // FIXME: Get ColorTranslation passed here and do something with it

   leftOfSource = SourcePoint->x * SourceGDI->BytesPerPixel;
   leftOfDest   = DestRect->left * DestGDI->BytesPerPixel;
   Width        = (DestRect->right - DestRect->left) * DestGDI->BytesPerPixel;
   CopyPos      = leftOfDest;

   for(dy=DestRect->top; dy<DestRect->bottom; dy++)
   {
      RtlCopyMemory(DestSurf->pvBits+CopyPos,
                    SourceSurf->pvBits+CopyPos,
                    Width);

      CopyPos += Delta;
   }
}

BOOL EngIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2)

{
   static const RECTL rclEmpty = { 0, 0, 0, 0 };

   prcDst->left  = max(prcSrc1->left, prcSrc2->left);
   prcDst->right = min(prcSrc1->right, prcSrc2->right);

   if (prcDst->left < prcDst->right)
   {
      prcDst->top    = max(prcSrc1->top, prcSrc2->top);
      prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

      if (prcDst->top < prcDst->bottom)
         return(TRUE);
   }

   *prcDst = rclEmpty;

   return(FALSE);
}

BOOL EngBitBlt(SURFOBJ *Dest, SURFOBJ *Source,
               SURFOBJ *Mask, CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation, RECTL *DestRect,
               POINTL *SourcePoint, POINTL *MaskRect,
               BRUSHOBJ *Brush, POINTL *BrushOrigin, ROP4 rop4)
{
   BYTE      clippingType;
   RECTL     rclTmp;
   POINTL    ptlTmp;
   RECT_ENUM RectEnum;
   BOOL      EnumMore;
   SURFGDI   *DestGDI, *SourceGDI;
   BOOLEAN   canCopyBits;

   // If we don't have to do anything special, we can punt to DrvCopyBits
   // if it exists
   if( (Mask == NULL)        && (MaskRect == NULL) && (Brush == NULL) &&
       (BrushOrigin == NULL) && (rop4 == 0) )
   {
     canCopyBits = TRUE;
   } else
     canCopyBits = FALSE;

   // FIXME: Use XLATEOBJ to translate source bitmap into destination bitmap's
   //        format. Call DrvDitherColor function where necessary and if available

   // FIXME: If canCopyBits == TRUE AND the driver has a DrvCopyBits then
   //        punt to EngCopyBits and not the driver's DrvCopyBits just yet so
   //        that the EngCopyBits can take care of the clipping drivers
   //        DrvCopyBits

   // FIXME: Don't punt to DrvBitBlt straight away. Instead, mark a typedef'd
   //        function to go there instead of the Engine's bltting function
   //        so as to do the clipping for the driver

   // Check for CopyBits or BitBlt hooks if one is not a GDI managed bitmap
   if((Dest->iType!=STYPE_BITMAP) || (Source->iType!=STYPE_BITMAP))
   {
      // Destination surface is device managed
      if(Dest->iType!=STYPE_BITMAP)
      {
         DestGDI = AccessInternalObjectFromUserObject(Dest);

         if ((DestGDI->CopyBits!=NULL) && (canCopyBits == TRUE))
         {
            return DestGDI->CopyBits(Dest, Source, ClipRegion,
                                     ColorTranslation, DestRect, SourcePoint);
         }

         if (DestGDI->BitBlt!=NULL)
         {
            return DestGDI->BitBlt(Dest, Source, Mask, ClipRegion,
                                   ColorTranslation, DestRect, SourcePoint,
                                   MaskRect, Brush, BrushOrigin, rop4);
         }
      }

      // Source surface is device managed
      if(Source->iType!=STYPE_BITMAP)
      {
         SourceGDI = AccessInternalObjectFromUserObject(Source);

         if ((SourceGDI->CopyBits!=NULL) && (canCopyBits == TRUE))
         {
            return SourceGDI->CopyBits(Dest, Source, ClipRegion,
                                       ColorTranslation, DestRect, SourcePoint);
         }

         if (SourceGDI->BitBlt!=NULL)
         {
            return SourceGDI->BitBlt(Dest, Source, Mask, ClipRegion,
                                     ColorTranslation, DestRect, SourcePoint,
                                     MaskRect, Brush, BrushOrigin, rop4);
         }


         // Should never get here, if it's not GDI managed then the device
         // should take care of it

         // FIXME: Error message here
      }

   }

   DestGDI   = AccessInternalObjectFromUserObject(Dest);
   SourceGDI = AccessInternalObjectFromUserObject(Source);

   // Determine clipping type
   if (ClipRegion == (CLIPOBJ *) NULL)
   {
      clippingType = DC_TRIVIAL;
   } else {
      clippingType = ClipRegion->iDComplexity;
   }

   // We don't handle color translation just yet

   if ((rop4 == 0x0000CCCC) &&
       ((ColorTranslation == NULL) || (ColorTranslation->flXlate & XO_TRIVIAL)))
   {
      switch(clippingType)
      {
         case DC_TRIVIAL:
            BitBltCopy(Dest,     Source,
                       DestGDI,  SourceGDI,
                       DestRect, SourcePoint, Source->lDelta);

            return(TRUE);

         case DC_RECT:

            // Clip the blt to the clip rectangle

            EngIntersectRect(&rclTmp, DestRect, &ClipRegion->rclBounds);

            ptlTmp.x = SourcePoint->x + rclTmp.left - DestRect->left;
            ptlTmp.y = SourcePoint->y + rclTmp.top  - DestRect->top;

            BitBltCopy(Dest,    Source,
                       DestGDI, SourceGDI,
                       &rclTmp, &ptlTmp, Source->lDelta);

            return(TRUE);

         case DC_COMPLEX:

            CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES,
            CD_ANY, ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum),
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

                     BitBltCopy(Dest,    Source,
                                DestGDI, SourceGDI,
                                prcl, &ptlTmp, Source->lDelta);

                     prcl++;

                  } while (prcl < prclEnd);
               }

            } while(EnumMore);

            return(TRUE);
      }
   }

   return(FALSE);
}
