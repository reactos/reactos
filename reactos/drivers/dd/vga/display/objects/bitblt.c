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
  ULONG i, j, dx, dy, alterx, altery, idxColor, RGBulong = 0, BPP;
  BYTE  *GDIpos, *initial;

  BPP = bytesPerPixel(Source->iBitmapFormat);
  GDIpos = Source->pvBits +
           SourcePoint->y * Source->lDelta + SourcePoint->x;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = abs(SourcePoint->x - DestRect->left);
  altery = abs(SourcePoint->y - DestRect->top);

  for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
  {
     initial = GDIpos;

     for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
     {
        RtlCopyMemory(&RGBulong, GDIpos, BPP);
        idxColor = XLATEOBJ_iXlate(ColorTranslation, RGBulong);

        vgaPutPixel(i+alterx, j+altery, idxColor);
        GDIpos+=BPP;
     }
     GDIpos = initial + Source->lDelta;
  }
}

BOOL VGAtoGDI(
   SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
   RECTL   *DestRect, POINTL *SourcePoint)
{
  ULONG i, j, dx, dy, idxColor, RGBulong, BPP;
  BYTE  *GDIpos, *initial;

  BPP = bytesPerPixel(Dest->iBitmapFormat);
  GDIpos = Dest->pvBits +
           DestRect->top * Dest->lDelta + DestRect->left;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  for(j=SourcePoint->y; j<SourcePoint->y+dy; j++)
  {
     initial = GDIpos;

     for(i=SourcePoint->x; i<SourcePoint->x+dx; i++)
     {
        idxColor = vgaGetPixel(i, j);

        RGBulong = XLATEOBJ_iXlate(ColorTranslation, idxColor);
        RtlCopyMemory(GDIpos, &RGBulong, BPP);

        GDIpos+=BPP;
     }
     GDIpos = initial + Dest->lDelta;
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

   // Determine the bltbit operation

   if((Source->iType == STYPE_BITMAP) && (Dest->iType == STYPE_DEVICE))
   {
      BltOperation = GDItoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_BITMAP))
   {
      BltOperation = VGAtoGDI;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVICE))
   {
      BltOperation = VGAtoVGA;
   } else
   if((Source->iType == STYPE_DEVBITMAP) && (Dest->iType == STYPE_DEVICE))
   {
      BltOperation = DFBtoVGA;
   } else
   if((Source->iType == STYPE_DEVICE) && (Dest->iType == STYPE_DEVBITMAP))
   {
      BltOperation = VGAtoDFB;
   } else
   {
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

            BltOperation(Dest, Source, ColorTranslation, Mask,
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
