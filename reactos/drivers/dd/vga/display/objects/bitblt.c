#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"
#include "bitblt.h"

// FIXME:
// RGBtoULONG (eng/xlate.c) will be faster than RtlCopyMemory?

// Note: All of our BitBlt ops expect to be working with 4BPP data

typedef BOOL (*PFN_VGABlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, XLATEOBJ *,
                           RECTL *, POINTL *);

BOOL DIBtoVGA(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, alterx, altery, idxColor, RGBulong = 0;
  BYTE  *GDIpos, *initial;

  GDIpos = Source->pvBits /* +
           SourcePoint->y * Source->lDelta + (SourcePoint->x >> 1) */ ;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = abs(SourcePoint->x - DestRect->left);
  altery = abs(SourcePoint->y - DestRect->top);

  // FIXME: ColorTranslation will never be null. We must always map the colors (see PCGPE's bmp.txt)

  if(ColorTranslation == NULL)
  {

    // No color translation necessary, we assume BPP = 1
    DIB_BltToVGA(DestRect->left, DestRect->top, dx, dy, Source->pvBits, Source->lDelta);

  } else {

    // Perform color translation
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
      initial = GDIpos;

      for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
      {
        idxColor = XLATEOBJ_iXlate(ColorTranslation, *GDIpos);
        vgaPutPixel(i+alterx, j+altery, idxColor);
        GDIpos+=1;
      }
      GDIpos = initial + Source->lDelta;
    }
  }
}

BOOL VGAtoDIB(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, RGBulong;
  BYTE  *GDIpos, *initial, idxColor;

  // Used by the temporary DFB
  PDEVSURF	TargetSurf;
  DEVSURF	DestDevSurf;
  PSURFOBJ	TargetBitmapSurf;
  HBITMAP	hTargetBitmap;
  SIZEL		InterSize;
  POINTL	ZeroPoint;

  // FIXME: Optimize to retrieve entire bytes at a time (see /display/vgavideo/vgavideo.c:vgaGetByte)

  GDIpos = Dest->pvBits /* + (DestRect->top * Dest->lDelta) + (DestRect->left >> 1) */ ;
  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if(ColorTranslation == NULL)
  {
    // Prepare a Dest Dev Target and copy from the DFB to the DIB
    DestDevSurf.NextScan = Dest->lDelta;
    DestDevSurf.StartBmp = Dest->pvScan0;

    DIB_BltFromVGA(SourcePoint->x, SourcePoint->y, dx, dy, Dest->pvBits, Dest->lDelta);

  } else {
    // Color translation
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
       initial = GDIpos;
       for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
       {
         *GDIpos = XLATEOBJ_iXlate(ColorTranslation, vgaGetPixel(i, j));
         GDIpos+=1;
       }
       GDIpos = initial + Dest->lDelta;
    }
  }
}

BOOL DFBtoVGA(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL VGAtoDFB(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL VGAtoVGA(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  // FIXME: Use fast blts instead of get and putpixels

  int i, j, dx, dy, alterx, altery, BltDirection;

  // Calculate deltas

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = abs(SourcePoint->x - DestRect->left);
  altery = abs(SourcePoint->y - DestRect->top);

  // Determine bltting direction
  // FIXME: should we perhaps make this an EngXxx function? Determining
  // direction is probably used whenever the surfaces are the same (not
  // just VGA screen)
  if (SourcePoint->y >= DestRect->top)
  {
    if (SourcePoint->x >= DestRect->left)
    {
      BltDirection = CD_RIGHTDOWN;
    }
    else
    {
      BltDirection = CD_LEFTDOWN;
    }
  }
  else
  {
    if (SourcePoint->x >= DestRect->left)
    {
      BltDirection = CD_RIGHTUP;
    }
    else
    {
      BltDirection = CD_LEFTUP;
    }
  }

  // Do the VGA to VGA BitBlt
  // FIXME: Right now we're only doing CN_LEFTDOWN and we're using slow
  // get and put pixel routines

  for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
  {
     for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
     {
        vgaPutPixel(i+alterx, j+altery, vgaGetPixel(i, j));
     }
  }

  return TRUE;
}

BOOL VGADDIBitBlt(SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask,
                  CLIPOBJ *Clip, XLATEOBJ *ColorTranslation,
                  RECTL *DestRect, POINTL *SourcePoint, POINTL *MaskPoint,
                  BRUSHOBJ *Brush, POINTL *BrushPoint, ROP4 rop4)
{
   RECT_ENUM RectEnum;
   BOOL EnumMore;
   PFN_VGABlt  BltOperation;

DPRINT("VGADDIBitBlt: Dest->pvScan0: %08x\n", Dest->pvScan0);

   // Determine the bltbit operation

   if((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVICE))
   {
DPRINT("DIB2VGA\n");
      BltOperation = DIBtoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_BITMAP))
   {
DPRINT("VGA2DIB\n");
      BltOperation = VGAtoDIB;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVICE))
   {
DPRINT("VGA2VGA\n");
      BltOperation = VGAtoVGA;
   } else
   if((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_DEVICE))
   {
DPRINT("DFB2VGA\n");
      BltOperation = DFBtoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVBITMAP))
   {
DPRINT("VGA2DFB\n");
      BltOperation = VGAtoDFB;
   } else
   {
DPRINT("VGA:bitblt.c: Can't handle requested BitBlt operation (source:%u dest:%u)\n", Source->iType, Dest->iType);
      // Cannot handle given surfaces for VGA BitBlt
      return FALSE;
   }

   // Perform the necessary operatings according to the clipping

   if(Clip == NULL)
   {
      BltOperation(Dest, Source, Mask, ColorTranslation, DestRect,
                   SourcePoint);
   } else
   {
      switch(Clip->iMode) {

         case TC_RECTANGLES:

         if (Clip->iDComplexity == DC_RECT)
         {
            // FIXME: Intersect clip rectangle

            BltOperation(Dest, Source, Mask, ColorTranslation,
                         DestRect, SourcePoint);
         } else {

            // Enumerate all the rectangles and draw them

   /*         CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_ANY,
                               ENUM_RECT_LIMIT);

            do {
               EnumMore = CLIPOBJ_bEnum(Clip, sizeof(RectEnum), (PVOID) &RectEnum);
               // FIXME: Calc new source point (diff between new & old destrects?)

               VGADDIFillSolid(Dest, Srouce, Mask,
                               &RectEnum.arcl[0], NewSourcePoint);

            } while (EnumMore); */
         }
      }
   }

   return TRUE;
}
