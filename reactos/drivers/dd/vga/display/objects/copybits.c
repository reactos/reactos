#include <ddk/winddi.h>
#include "../../../../ntoskrnl/include/internal/i386/io.h"

#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"

// FIXME: There's a lot of redundancy in here -- break into functions
// FIXME: All functions involving the screen use terribly slow vgaPutPixel -- use something better

VOID Screen2Screen(PDEVSURF PDSurf, RECTL *DestRect, POINTL *SourcePoint,
                   ULONG iCopyDir)
{
   ULONG i, j, spx, spy;
   spy = SourcePoint->y;

   // FIXME: Right now assumes top->down and left->right

   for (j=DestRect->top; j<DestRect->bottom; j++)
   {
      spx = SourcePoint->x;
      for(i=DestRect->left; i<DestRect->right; i++)
      {
         vgaPutPixel(spx, spy, vgaGetPixel(i, j));
         spx++;
      }
      spy++;
   }
}

VOID CopyRect4BPP(PDEVSURF DestSurf, PDEVSURF SourceSurf, RECTL *DestRect,
                   POINTL *SourcePoint)
{
   ULONG i, j, Width, spy;
   spy = SourcePoint->y;
   Width = DestRect->right - DestRect->left;

   for (j=DestRect->top; j<DestRect->bottom; j++)
   {
      RtlCopyMemory(DestSurf->StartBmp+DestSurf->NextScan*j+DestRect->left,
                    SourceSurf->StartBmp+SourceSurf->NextScan*j+SourcePoint->x,
                    Width);
      spy++;
   }
}

VOID DFB2DIB(PDEVSURF DestSurf, PDEVSURF SourceSurf, RECTL *DestRect,
             POINTL *SourcePoint)
{
   CopyRect4BPP(DestSurf, SourceSurf, DestRect, SourcePoint);
}

VOID DFB2DFB(PDEVSURF DestSurf, PDEVSURF SourceSurf, RECTL *DestRect,
             POINTL *SourcePoint)
{
   CopyRect4BPP(DestSurf, SourceSurf, DestRect, SourcePoint);
}

#define DFB_TARGET 0
#define VGA_TARGET 1

VOID DIB2VGA(PDEVSURF DestSurf, PDEVSURF SourceSurf,
             RECTL *DestRect, POINTL *SourcePoint, UCHAR *ConversionTables,
             BOOL DFBorVGAtarget)
{
   ULONG i, j, spx, spy;
   BYTE c;
   spy = SourcePoint->y;

   for (j=DestRect->top; j<DestRect->bottom; j++)
   {
      spx = SourcePoint->x;
      for(i=DestRect->left; i<DestRect->right; i++)
      {
         if(DFBorVGAtarget == DFB_TARGET)
         {
            c = vgaGetPixel(spx, spy);
            RtlCopyMemory(DestSurf->StartBmp+j*DestSurf->NextScan+i, c, 1);
         } else
         {
            RtlCopyMemory(c, SourceSurf->StartBmp+j*SourceSurf->NextScan+i, 1);
            vgaPutPixel(spx, spy, c);
         }
         spx++;
      }
      spy++;
   }
}

VOID DFB2VGA(DEVSURF *DestSurf, DEVSURF *SourceSurf, RECTL *DestRect,
             POINTL *SourcePoint)
{
   DIB2VGA(DestSurf, SourceSurf, DestRect, SourcePoint, NULL, VGA_TARGET);
}


VOID VGA2DIB(DEVSURF *DestSurf, ULONG xSource, ULONG ySource,
             PVOID DestScan0, ULONG left, ULONG top, ULONG width,
             ULONG height, ULONG lDelta, ULONG Format, ULONG *Conversion)
{
   ULONG i, j, spx, spy, right = left + width, bottom = top + height;
   BYTE c;
   spy = ySource;

   for (j=top; j<bottom; j++)
   {
      spx = xSource;
      for(i=left; i<right; i++)
      {
         c = vgaGetPixel(spx, spy);
         RtlCopyMemory(DestSurf->StartBmp+j*DestSurf->NextScan+i, c, 1);
         spx++;
      }
      spy++;
   }
}

BOOL VGADDICopyBits(SURFOBJ *Dest, SURFOBJ *Source,
                    CLIPOBJ *Clip, XLATEOBJ *ColorTranslation,
                    PRECTL DestRect, PPOINTL SourcePoint)
{
   PDEVSURF    pdsurf;             // Pointer to a device surface

   LONG        lDelta;             // Delta to next scan of destination
   PVOID       pjDstScan0;         // Pointer to scan 0 of destination DIB
   ULONG      *pulXlate;           // Pointer to color xlate vector
   BOOL        EnumMore;              // Clip continuation flag
   ULONG       ircl;               // Clip enumeration rectangle index
   RECT_ENUM   cben;               // Clip enumerator
   RECTL       TempRect;
   PRECTL      PRect;
   POINTL      TempPoint;
   DEVSURF     TempDSurf;
   PDEVSURF    SurfDest;          // Pointer for target
   PDEVSURF    SurfSource;          // Pointer for source if present
   INT         iCopyDir;
   RECT_ENUM   RectEnum;               // Clip enumerator
   BYTE        jClipping;
   UCHAR      *pucDIB4ToVGAConvTables;
   ULONG       ulWidth;
   ULONG       ulNumSlices;
   ULONG       ulRight;
   ULONG       x;

   // If translation is XO_TRIVIAL then no translation is required
   if(ColorTranslation && (ColorTranslation->flXlate & XO_TRIVIAL))
   {
      ColorTranslation = NULL;
   }

   // If the destination is a device managed bitmap give it to VGADDIBitBlt
   if(ColorTranslation &&
      ((Dest->iType == STYPE_DEVICE) || (Dest->iType == STYPE_DEVBITMAP)))
   {
      return(VGADDIBitBlt(Dest, Source, (SURFOBJ *)NULL,
                          Clip, ColorTranslation,
                          DestRect, SourcePoint,
                          (POINTL *)NULL, (BRUSHOBJ *)NULL, (POINTL *)NULL,
                          0x0000CCCC));
   }

   if ((Dest->iType == STYPE_DEVICE) && (Source->iType == STYPE_DEVICE)) {
      SurfDest = (PDEVSURF) Dest->dhsurf;
      SurfSource = (PDEVSURF) Source->dhsurf;

      if (Clip == (CLIPOBJ *) NULL)
      {
         jClipping = DC_TRIVIAL;
      }
      else
      {
         jClipping = Clip->iDComplexity;
      }


      if (SourcePoint->y >= DestRect->top) {
         if (SourcePoint->x >= DestRect->left) {
            iCopyDir = CD_RIGHTDOWN;
         } else {
            iCopyDir = CD_LEFTDOWN;
         }
      } else {
         if (SourcePoint->x >= DestRect->left) {
            iCopyDir = CD_RIGHTUP;
         } else {
            iCopyDir = CD_LEFTUP;
         }
      }

      switch(jClipping) {

         case DC_TRIVIAL:
            Screen2Screen(SurfDest, DestRect, SourcePoint, iCopyDir);
            break;

         case DC_RECT:
            if (!VGADDIIntersectRect(&TempRect, DestRect,
                                     &Clip->rclBounds))
            {
               return TRUE;
            }

            TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
            TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;

            Screen2Screen(SurfDest, &TempRect, &TempPoint, iCopyDir);
            break;

         case DC_COMPLEX:

            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
                               iCopyDir, ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(Clip, (ULONG)sizeof(RectEnum),
                                     (PVOID)&RectEnum);

               PRect = RectEnum.arcl;
               for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++) {

                   VGADDIIntersectRect(PRect, PRect, DestRect);
                   // Adjust the source point for clipping too
                   TempPoint.x = SourcePoint->x + PRect->left -
                                 DestRect->left;
                   TempPoint.y = SourcePoint->y + PRect->top -
                                 DestRect->top;
                   Screen2Screen(SurfDest, PRect, &TempPoint, iCopyDir);
                }
            } while(EnumMore);
            break;
      }
      return TRUE;
   }

   // Device managed to standard bitmap
   if ((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_BITMAP))
   {
      switch(Dest->iBitmapFormat)
      {
         case BMF_4BPP:
            if (ColorTranslation == NULL)
            {
               // Make just enough of a fake DEVSURF for the dest so that
               // the DFB to DIB code can work

               TempDSurf.NextScan = Dest->lDelta;
               TempDSurf.StartBmp = Dest->pvScan0;

               if ((Clip == NULL) || (Clip->iDComplexity == DC_TRIVIAL))
               {
                  ulWidth = DestRect->right - DestRect->left;
                  if (ulWidth <= MAX_SCAN_WIDTH)
                  {
                     DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                             DestRect, SourcePoint);
                  } else {
                     TempRect.left = DestRect->left;
                     TempRect.right = DestRect->right;
                     TempRect.top = DestRect->top;
                     TempRect.bottom = DestRect->bottom;

                     // cut rect into slices MAX_SCAN_WIDTH wide
                     ulRight = TempRect.right;   // save right edge
                     TempRect.right = TempRect.left+MAX_SCAN_WIDTH;
                     ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                     for (x=0; x<ulNumSlices-1; x++)
                     {
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                &TempRect, &TempPoint);
                        TempRect.left = TempRect.right;
                        TempRect.right += MAX_SCAN_WIDTH;
                     }
                     TempRect.right = ulRight;
                     // Adjust the source point for clipping too
                     TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                     TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                     DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                             &TempRect, &TempPoint);
                  }
               }
               else if (Clip->iDComplexity == DC_RECT)
               {
                  if (VGADDIIntersectRect(&TempRect, DestRect, &Clip->rclBounds))
                  {
                     ulWidth = TempRect.right - TempRect.left;

                     if (ulWidth <= MAX_SCAN_WIDTH)
                     {
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                &TempRect, &TempPoint);
                     } else {
                        // cut rect into slices MAX_SCAN_WIDTH wide
                        ulRight = TempRect.right;   // save right edge
                        TempRect.right = TempRect.left+MAX_SCAN_WIDTH;
                        ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                        for (x=0; x<ulNumSlices-1; x++)
                        {
                           // Adjust the source point for clipping too
                           TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                           TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                           DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                   &TempRect, &TempPoint);
                           TempRect.left = TempRect.right;
                           TempRect.right += MAX_SCAN_WIDTH;
                        }
                        TempRect.right = ulRight;
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                &TempRect, &TempPoint);
                     }
                  }
                  return(TRUE);
               } else {
                  // DC_COMPLEX:

                  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
                                     CD_ANY, ENUM_RECT_LIMIT);

                  do {
                     EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum),
                                           (PVOID) &RectEnum);
                     PRect = RectEnum.arcl;
                     for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++)
                     {
                        VGADDIIntersectRect(PRect,PRect,DestRect);
                        ulWidth = PRect->right - PRect->left;

                        if (ulWidth <= MAX_SCAN_WIDTH)
                        {
                           // Adjust the source point for clipping too
                           TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                           TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                           DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                   &TempRect, &TempPoint);
                        } else {
                           // cut rect into slices MAX_SCAN_WIDTH wide
                           ulRight = PRect->right;   // save right edge
                           PRect->right = PRect->left+MAX_SCAN_WIDTH;
                           ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                           for (x=0; x<ulNumSlices-1; x++)
                           {
                              // Adjust the source point for clipping too
                              TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                              TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                              DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                      PRect, &TempPoint);
                              PRect->left = PRect->right;
                              PRect->right += MAX_SCAN_WIDTH;
                           }
                           PRect->right = ulRight;
                           // Adjust the source point for clipping too
                           TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                           TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                           DFB2DIB(&TempDSurf, (PDEVSURF) Source->dhsurf,
                                   PRect, &TempPoint);
                        }
                     }
                  } while(EnumMore);

               }
               return(TRUE);
            }
            break;
      }
   }

   // Device managed to device managed
   if ((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_DEVBITMAP)) {

      if (Source==Dest)
      {
         TempRect.top =    SourcePoint->y;
         TempRect.left =   SourcePoint->x;
         TempRect.bottom = SourcePoint->y + (DestRect->bottom - DestRect->top);
         TempRect.right =  SourcePoint->x + (DestRect->right - DestRect->left);

         if (DestRect->top >= SourcePoint->y)
         {
           if (VGADDIIntersectRect(&TempRect,&TempRect,DestRect))
           {
               return(VGADDIBitBlt(Dest, Source, (SURFOBJ *)NULL,
                                Clip, ColorTranslation,
                                DestRect, SourcePoint,
                                (POINTL *)NULL, (BRUSHOBJ *)NULL,
                                (POINTL *)NULL, 0x0000CCCC));
            }
         }
      }

      if (ColorTranslation == NULL)
      {
         if ((Clip == NULL) || (Clip->iDComplexity == DC_TRIVIAL))
         {
            DFB2DFB((PDEVSURF) Dest->dhsurf, (PDEVSURF) Source->dhsurf,
                    DestRect, SourcePoint);
         } else if (Clip->iDComplexity == DC_RECT)
         {
            if (VGADDIIntersectRect(&TempRect, DestRect, &Clip->rclBounds))
            {
               TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
               TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;

               DFB2DFB((PDEVSURF) Dest->dhsurf, (PDEVSURF) Source->dhsurf,
                       &TempRect, &TempPoint);
            }
            return(TRUE);
         } else {
            // DC_COMPLEX:

            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
                              CD_ANY, ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(Clip, (ULONG)sizeof(RectEnum), (PVOID)&RectEnum);
               PRect = RectEnum.arcl;
               for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++)
               {
                   VGADDIIntersectRect(PRect,PRect,DestRect);

                   TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                   TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;

                   DFB2DFB((PDEVSURF)Dest->dhsurf, (PDEVSURF)Source->dhsurf,
                           PRect, &TempPoint);
               }
            } while(EnumMore);
         }
         return(TRUE);
      }
   }

   // Device managed to screen
   if((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_DEVICE))
   {
      if(ColorTranslation == NULL)
      {
         if ((Clip == NULL) || (Clip->iDComplexity == DC_TRIVIAL))
         {
            DFB2VGA((PDEVSURF) Dest->dhsurf, (PDEVSURF) Source->dhsurf, DestRect,
                    SourcePoint);
         } else if (Clip->iDComplexity == DC_RECT)
         {
            if (VGADDIIntersectRect(&TempRect, DestRect, &Clip->rclBounds))
            {
               TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
               TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;

               DFB2VGA((PDEVSURF) Dest->dhsurf, (PDEVSURF) Source->dhsurf,
                       &TempRect, &TempPoint);
            }
            return(TRUE);
         } else {
            // DC_COMPLEX:
            CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(Clip, (ULONG)sizeof(RectEnum), (PVOID)&RectEnum);
               PRect = RectEnum.arcl;
               for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++)
               {
                   VGADDIIntersectRect(PRect,PRect,DestRect);

                   TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                   TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;

                   DFB2VGA((PDEVSURF)Dest->dhsurf, (PDEVSURF)Source->dhsurf,
                           PRect, &TempPoint);
               }
            } while(EnumMore);
         }
         return(TRUE);
      }
   }

   // Standard bitmap to device managed
   if ((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVBITMAP))
   {
      switch(Source->iBitmapFormat)
      {
         case BMF_4BPP:
            if (ColorTranslation == NULL)
            {
               pucDIB4ToVGAConvTables =
                           ((PDEVSURF) Dest->dhsurf)->ppdev->
                           pucDIB4ToVGAConvTables;

               // Make just enough of a fake DEVSURF for the source so that
               // the DIB to DFB code can work

               TempDSurf.NextScan = Source->lDelta;
               TempDSurf.StartBmp = Source->pvScan0;

               // Clip as needed

               if ((Clip == NULL) || (Clip->iDComplexity == DC_TRIVIAL))
               {
                  ulWidth = DestRect->right - DestRect->left;

                  if (ulWidth <= MAX_SCAN_WIDTH)
                  {
                     DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf, DestRect,
                             SourcePoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                  } else {
                     TempRect.left = DestRect->left;
                     TempRect.right = DestRect->right;
                     TempRect.top = DestRect->top;
                     TempRect.bottom = DestRect->bottom;

                     // cut rect into slices MAX_SCAN_WIDTH wide
                     ulRight = TempRect.right;   // save right edge
                     TempRect.right = TempRect.left+MAX_SCAN_WIDTH;
                     ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                     for (x=0; x<ulNumSlices-1; x++)
                     {
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                &TempRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                        TempRect.left = TempRect.right;
                        TempRect.right += MAX_SCAN_WIDTH;
                     }
                     TempRect.right = ulRight;
                     // Adjust the source point for clipping too
                     TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                     TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                     DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                             &TempRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                  }
               }
               else if (Clip->iDComplexity == DC_RECT)
               {
                  if (VGADDIIntersectRect(&TempRect, DestRect, &Clip->rclBounds))
                  {
                     ulWidth = TempRect.right - TempRect.left;

                     if (ulWidth <= MAX_SCAN_WIDTH)
                     {
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                &TempRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                     } else {
                        // cut rect into slices MAX_SCAN_WIDTH wide
                        ulRight = TempRect.right;   // save right edge
                        TempRect.right = TempRect.left+MAX_SCAN_WIDTH;
                        ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                        for (x=0; x<ulNumSlices-1; x++)
                        {
                           TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                           TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                           DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                   &TempRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                           TempRect.left = TempRect.right;
                           TempRect.right += MAX_SCAN_WIDTH;
                        }
                        TempRect.right = ulRight;
                        // Adjust the source point for clipping too
                        TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                        TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;
                        DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                &TempRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                     }
                  }
                  return(TRUE);
               } else {
                  // DC_COMPLEX:

                  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
                                     CD_ANY, ENUM_RECT_LIMIT);

                  do {
                     EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(RectEnum),
                                           (PVOID) &RectEnum);
                     PRect = RectEnum.arcl;
                     for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++)
                     {
                        VGADDIIntersectRect(PRect,PRect,DestRect);
                        ulWidth = PRect->right - PRect->left;

                        if (ulWidth <= MAX_SCAN_WIDTH)
                        {
                           // Adjust the source point for clipping too
                           TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                           TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                           DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                    PRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                        } else {
                           // cut rect into slices MAX_SCAN_WIDTH wide
                           ulRight = PRect->right;   // save right edge
                           PRect->right = PRect->left+MAX_SCAN_WIDTH;
                           ulNumSlices = (ulWidth+MAX_SCAN_WIDTH-1) / MAX_SCAN_WIDTH;
                           for (x=0; x<ulNumSlices-1; x++)
                           {
                              // Adjust the source point for clipping too
                              TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                              TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                              DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                              PRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                              PRect->left = PRect->right;
                              PRect->right += MAX_SCAN_WIDTH;
                           }
                           PRect->right = ulRight;
                           // Adjust the source point for clipping too
                           TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                           TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;
                           DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                                     PRect, &TempPoint, pucDIB4ToVGAConvTables, DFB_TARGET);
                        }
                     }
                  } while(EnumMore);

               }
               return(TRUE);
            }
            break;

         case BMF_1BPP:
         case BMF_8BPP:
            return(VGADDIBitBlt(Dest, Source, (SURFOBJ *)NULL,
                             Clip, ColorTranslation,
                             DestRect, SourcePoint,
                             (POINTL *)NULL, (BRUSHOBJ *)NULL, (POINTL *)NULL,
                             0x0000CCCC));
      }
   }

   // Standard bitmap to screen
   if ((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVICE))
   {
      switch(Source->iBitmapFormat)
      {
         case BMF_4BPP:
         if (ColorTranslation == NULL)
         {
            pucDIB4ToVGAConvTables =
                           ((PDEVSURF) Dest->dhsurf)->ppdev->
                           pucDIB4ToVGAConvTables;

            // Make just enough of a fake DEVSURF for the source so that
            // the DIB to VGA code can work

            TempDSurf.NextScan = Source->lDelta;
            TempDSurf.StartBmp = Source->pvScan0;

            // Clip as needed

            if ((Clip == NULL) || (Clip->iDComplexity == DC_TRIVIAL))
            {
               DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf, DestRect,
                       SourcePoint, pucDIB4ToVGAConvTables, VGA_TARGET);

            } else if (Clip->iDComplexity == DC_RECT)
            {
               if (VGADDIIntersectRect(&TempRect, DestRect, &Clip->rclBounds))
               {
                  TempPoint.x = SourcePoint->x + TempRect.left - DestRect->left;
                  TempPoint.y = SourcePoint->y + TempRect.top - DestRect->top;

                  DIB2VGA((PDEVSURF) Dest->dhsurf, &TempDSurf,
                          &TempRect, &TempPoint, pucDIB4ToVGAConvTables, VGA_TARGET);
               }
               return(TRUE);

            } else {
               // DC_COMPLEX:

               CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES,
                                  CD_ANY, ENUM_RECT_LIMIT);

               do {
                  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG)sizeof(RectEnum), (PVOID)&RectEnum);
                  PRect = RectEnum.arcl;
                  for (ircl = 0; ircl < RectEnum.c; ircl++, PRect++)
                  {
                     VGADDIIntersectRect(PRect,PRect,DestRect);

                     TempPoint.x = SourcePoint->x + PRect->left - DestRect->left;
                     TempPoint.y = SourcePoint->y + PRect->top - DestRect->top;

                     DIB2VGA((PDEVSURF) Dest->dhsurf,
                             &TempDSurf, PRect, &TempPoint,
                             pucDIB4ToVGAConvTables, VGA_TARGET);
                  }
               } while(EnumMore);
            }
            return(TRUE);
         }
         break;

         case BMF_1BPP:
         case BMF_8BPP:
            return(VGADDIBitBlt(Dest, Source, (SURFOBJ *)NULL,
                             Clip, ColorTranslation,
                             DestRect, SourcePoint,
                             (POINTL *)NULL, (BRUSHOBJ *)NULL, (POINTL *)NULL,
                             0x0000CCCC));

         case BMF_8RLE:
         case BMF_4RLE:
//            return(RleBlt(Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint));

      }
   }
   // Screen to standard bitmap
   if ((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_BITMAP))
   {
      if (Dest->iBitmapFormat == BMF_4BPP)
      {
//         pdsurf = (PDEVSURF) Source->dhsurf;

         // Get the data for the destination DIB
         lDelta = Dest->lDelta;
         pjDstScan0 = (PBYTE) Dest->pvScan0;

         // Setup for any color translation which may be needed !!! Is any needed at all?
         if (ColorTranslation == NULL)
         {
            pulXlate = NULL;
         }
         else
         {
            if (ColorTranslation->flXlate & XO_TABLE)
            {
                pulXlate = ColorTranslation->pulXlate;
            } else {
               pulXlate = (PULONG) NULL;
            }
         }

         if (Clip != (CLIPOBJ *) NULL)
         {
            switch(Clip->iDComplexity)
            {
               case DC_TRIVIAL:
                  EnumMore = FALSE;
                  cben.c = 1;
                  cben.arcl[0] = *DestRect;
                  break;

               case DC_RECT:
                  EnumMore = FALSE;
                  cben.c = 1;
                  cben.arcl[0] = Clip->rclBounds;
                  break;

               case DC_COMPLEX:
                  EnumMore = TRUE;
                  cben.c = 0;
                  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);
                  break;
            }
         }
         else
         {
            EnumMore = FALSE;
            cben.c = 1;
            cben.arcl[0] = *DestRect;            // Use the target for clipping
         }

         // Call the VGA conversion routine, adjusted for each rectangle

         do
         {
            LONG  xSrc;
            LONG  ySrc;
            RECTL *PRect;

            if (EnumMore)
               EnumMore = CLIPOBJ_bEnum(Clip,(ULONG) sizeof(cben), (PVOID) &cben);

            for (ircl = 0; ircl < cben.c; ircl++)
            {
               PRect = &cben.arcl[ircl];

               xSrc = SourcePoint->x + PRect->left - DestRect->left;
               ySrc = SourcePoint->y + PRect->top  - DestRect->top;

               VGAtoGDI(Dest, Source, NULL, ColorTranslation, DestRect, SourcePoint);

/*               VGA2DIB(pdsurf, xSrc, ySrc, pjDstScan0,
                       PRect->left, PRect->top, PRect->right - PRect->left,
                       PRect->bottom - PRect->top,
                       lDelta, Dest->iBitmapFormat, pulXlate); */
            }
         } while (EnumMore);
         return(TRUE);
      }
   }
   // If unsupported
   return(SimCopyBits(Dest, Source, Clip, ColorTranslation, DestRect, SourcePoint));
}

BOOL SimCopyBits(SURFOBJ *Dest, SURFOBJ *Source,
                 CLIPOBJ *Clip, XLATEOBJ *ColorTranslation,
                 PRECTL DestRect, PPOINTL  SourcePoint)
{
   HBITMAP  hbmTmp;
   SURFOBJ *psoTmp;
   RECTL    rclTmp;
   SIZEL    sizlTmp;
   BOOL     bReturn = FALSE;
   POINTL   ptl00 = {0,0};

   rclTmp.top = rclTmp.left = 0;
   rclTmp.right  = sizlTmp.cx = DestRect->right - DestRect->left;
   rclTmp.bottom = sizlTmp.cy = DestRect->bottom - DestRect->top;

   // Create bitmap in our compatible format
   hbmTmp = EngCreateBitmap(sizlTmp, sizlTmp.cx * 1, BMF_4BPP, 0, NULL); // FIXME: Replace *1 with *BPP?

   if (hbmTmp)
   {
      if ((psoTmp = EngLockSurface((HSURF)hbmTmp)) != NULL)
      {
         if (((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVICE)) ||
             ((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVBITMAP)))
         {
            // DIB to VGA  or  DIB to DFB
            if (EngCopyBits(psoTmp, Source, NULL, ColorTranslation, &rclTmp, SourcePoint))
            {
               bReturn = VGADDICopyBits(Dest, psoTmp, Clip, NULL, DestRect, &ptl00);
            }
         }
         else if (((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_BITMAP)) ||
                  ((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_BITMAP)))
         {
            // VGA to DIB  or  DFB to DIB
            if (VGADDICopyBits(psoTmp, Source, NULL, NULL, &rclTmp, SourcePoint))
            {
               bReturn = EngCopyBits(Dest, psoTmp, Clip, ColorTranslation, DestRect, &ptl00);
            }
         }
         else if (((Source->iType == STYPE_DEVICE) || (Source->iType == STYPE_DEVBITMAP)) &&
                  ((Dest->iType == STYPE_DEVICE) || (Dest->iType == STYPE_DEVBITMAP)))
         {
            // VGA or DFB  to  VGA or DFB
            if (VGADDICopyBits(psoTmp, Source, NULL, NULL, &rclTmp, SourcePoint))
            {
               bReturn = VGADDICopyBits(Dest, psoTmp, Clip, ColorTranslation, DestRect, &ptl00);
            }
         }

         EngUnlockSurface(psoTmp);
      }

      EngDeleteSurface((HSURF)hbmTmp);
   }

   return(bReturn);
}
