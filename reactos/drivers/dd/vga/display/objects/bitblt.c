#include "..\vgaddi.h"
#include "..\vgavideo\vgavideo.h"
#include "brush.h"
#include "bitblt.h"

// FIXME:
// RGBtoULONG (eng/xlate.c) will be faster than RtlCopyMemory?

// Note: All of our BitBlt ops expect to be working with 4BPP data

typedef BOOL (*PFN_VGABlt)(SURFOBJ *, SURFOBJ *, SURFOBJ *, XLATEOBJ *,
                           RECTL *, POINTL *);

BOOL GDItoVGA(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, alterx, altery, idxColor, RGBulong = 0, BPP;
  BYTE  *GDIpos, *initial;

  BPP = bytesPerPixel(Source->iBitmapFormat);
  GDIpos = Source->pvBits +
           SourcePoint->y * Source->lDelta + SourcePoint->x;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = abs(SourcePoint->x - DestRect->left);
  altery = abs(SourcePoint->y - DestRect->top);

  if(ColorTranslation == NULL)
  {
DbgPrint("GDItoVGA: No color translation\n");
    // No color translation necessary, we assume BPP = 1

    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
      initial = GDIpos;

      for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
      {
        vgaPutPixel(i+alterx, j+altery, *GDIpos);
        GDIpos+=BPP;
      }
      GDIpos = initial + Source->lDelta;
    }
  } else {
    // Perform color translation
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
      initial = GDIpos;

      for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
      {
        idxColor = XLATEOBJ_iXlate(ColorTranslation, *GDIpos);
        vgaPutPixel(i+alterx, j+altery, idxColor);
        GDIpos+=BPP;
      }
      GDIpos = initial + Source->lDelta;
    }
  }
DbgPrint("GDItoVGA: Done\n");
}

BOOL VGAtoGDI(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, RGBulong, BPP;
  BYTE  *GDIpos, *initial, idxColor;

  // FIXME: Optimize to retrieve entire bytes at a time (see /display/vgavideo/vgavideo.c:vgaGetByte)

  BPP = bytesPerPixel(Dest->iBitmapFormat);

DbgPrint("VGAtoGDI: BPP: %u\n", BPP);

  GDIpos = Dest->pvBits +
           (DestRect->top * Dest->lDelta) + (DestRect->left * BPP);

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if(ColorTranslation == NULL)
  {
DbgPrint("VGAtoGDI: No color translation\n");
    // No color translation necessary, we assume BPP = 1
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
       initial = GDIpos;
       for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
       {
         *GDIpos = vgaGetPixel(i, j);
         GDIpos++;
       }
       GDIpos = initial + Dest->lDelta;
    }
  } else {
    // Color translation
    for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
    {
       initial = GDIpos;
       for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
       {
         *GDIpos = XLATEOBJ_iXlate(ColorTranslation, vgaGetPixel(i, j));
         GDIpos+=BPP;
       }
       GDIpos = initial + Dest->lDelta;
    }
  }
DbgPrint("VGAtoGDI: Done\n");
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

   // Determine the bltbit operation

   if((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVICE))
   {
DbgPrint("GDI2VGA\n");
      BltOperation = GDItoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_BITMAP))
   {
DbgPrint("VGA2GDI\n");
      BltOperation = VGAtoGDI;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVICE))
   {
DbgPrint("VGA2VGA\n");
      BltOperation = VGAtoVGA;
   } else
   if((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_DEVICE))
   {
DbgPrint("DFB2VGA\n");
      BltOperation = DFBtoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVBITMAP))
   {
DbgPrint("VGA2DFB\n");
      BltOperation = VGAtoDFB;
   } else
   {
DbgPrint("VGA:bitblt.c: Can't handle requested BitBlt operation\n");
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
