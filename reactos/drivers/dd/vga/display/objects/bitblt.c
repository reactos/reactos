#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"
#include "bitblt.h"

typedef BOOL (*PFN_VGABlt)(SURFOBJ*, SURFOBJ*, XLATEOBJ*, RECTL*, POINTL*);

BOOL
DIBtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  LONG i, j, dx, dy, alterx, altery, idxColor, RGBulong = 0, c8;
  BYTE  *GDIpos, *initial, *tMask, *lMask;

  GDIpos = Source->pvBits;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  alterx = abs(SourcePoint->x - DestRect->left);
  altery = abs(SourcePoint->y - DestRect->top);

  if (ColorTranslation == NULL)
    {
      DIB_BltToVGA(DestRect->left, DestRect->top, dx, dy, Source->pvBits,
		   Source->lDelta);
    }
  else
    {
      /* Perform color translation */
      for (j = SourcePoint->y; j < SourcePoint->y+dy; j++)
	{
	  initial = GDIpos;

	  for (i=SourcePoint->x; i<SourcePoint->x+dx; i++)
	    {
	      idxColor = XLATEOBJ_iXlate(ColorTranslation, *GDIpos);
	      vgaPutPixel(i+alterx, j+altery, idxColor);
	      GDIpos+=1;
	    }
	  GDIpos = initial + Source->lDelta;
	}
    }
}

BOOL 
VGAtoDIB(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
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

BOOL 
DFBtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL
VGAtoDFB(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
{
  // Do DFBs need color translation??
}

BOOL 
VGAtoVGA(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	 RECTL *DestRect, POINTL *SourcePoint)
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

BOOL STDCALL
VGADDI_BltBrush(SURFOBJ* Dest, XLATEOBJ* ColorTranslation, RECTL* DestRect,
		BRUSHOBJ* Brush, POINTL* BrushPoint, ROP4 Rop4)
{
  UCHAR SolidColor;
  ULONG Left;
  ULONG Right;
  ULONG Length;
  PUCHAR Video;
  UCHAR Mask;
  ULONG i, j;

  /* Punt brush blts to non-device surfaces. */
  if (Dest->iType != STYPE_DEVICE)
    {
      return(FALSE);
    }

  /* Punt pattern fills. */
  if (Rop4 == PATCOPY && Brush->iSolidColor == 0xFFFFFFFF)
    {
      return(FALSE);
    }
  if (Rop4 == PATCOPY)
    {
      SolidColor = Brush->iSolidColor;
    }
  else if (Rop4 == WHITENESS)
    {
      SolidColor = 1;
    }
  else
    {
      SolidColor = 0;
    }

  /* Fill any pixels on the left which don't fall into a full row of eight. */
  if ((DestRect->left % 8) != 0)
    {
      /* Disable writes to pixels outside of the destination rectangle. */
      Mask = (1 << (8 - (DestRect->left % 8))) - 1;
      if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
	{
	  Mask &= ~((1 << (8 - (DestRect->right % 8))) - 1);
	}
      WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)GRA_D, Mask);

      /* Write the same color to each pixel. */
      Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->left >> 3);
      for (i = DestRect->top; i < DestRect->bottom; i++, Video+=80)
	{
	  (VOID)READ_REGISTER_UCHAR(Video);
	  WRITE_REGISTER_UCHAR(Video, SolidColor);
	}
    }

  /* Enable writes to all pixels. */
  WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
  WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);

  /* Have we finished. */
  if ((DestRect->right - DestRect->left) < (8 - (DestRect->left % 8)))
    {
      return(TRUE);
    }

  /* Fill any whole rows of eight pixels. */
  Left = (DestRect->left + 7) & ~0x7;
  Length = (DestRect->right >> 3) - (Left >> 3);
  for (i = DestRect->top; i < DestRect->bottom; i++)
    {
      Video = (PUCHAR)vidmem + i * 80 + (Left >> 3);
      for (j = 0; j < Length; j++, Video++)
	{
	  WRITE_REGISTER_UCHAR(Video, SolidColor);
	}
    }

  /* Fill any pixels on the right which don't fall into a complete row. */
  if ((DestRect->right % 8) != 0)
    {
      /* Disable writes to pixels outside the destination rectangle. */
      Mask = ~((1 << (8 - (DestRect->right % 8))) - 1);
      WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)GRA_D, Mask);

      Video = (PUCHAR)vidmem + DestRect->top * 80 + (DestRect->right >> 3);
      for (i = DestRect->top; i < DestRect->bottom; i++, Video+=80)
	{
	  /* Read the existing colours for this pixel into the latches. */
	  (VOID)READ_REGISTER_UCHAR(Video);
	  /* Write the new colour for the pixels selected in the mask. */
	  WRITE_REGISTER_UCHAR(Video, SolidColor);
	}

      /* Restore the default write masks. */
      WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xFF);
    }
  return(TRUE);
}

BOOL STDCALL
VGADDI_BltSrc(SURFOBJ *Dest, SURFOBJ *Source, XLATEOBJ *ColorTranslation,
	      RECTL *DestRect, POINTL *SourcePoint)
{
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  PFN_VGABlt  BltOperation;
  ULONG SourceType;

  SourceType = Source->iType;

  if (SourceType == STYPE_BITMAP && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = DIBtoVGA;
    }
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_BITMAP)
    {
      BltOperation = VGAtoDIB;
    }
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = VGAtoVGA;
    } 
  else if (SourceType == STYPE_DEVBITMAP && Dest->iType == STYPE_DEVICE)
    {
      BltOperation = DFBtoVGA;
    } 
  else if (SourceType == STYPE_DEVICE && Dest->iType == STYPE_DEVBITMAP)
    {
      BltOperation = VGAtoDFB;
    } 
  else
    {
      /* Punt blts not involving a device or a device-bitmap. */
      return(FALSE);
   }

  BltOperation(Dest, Source, ColorTranslation, DestRect, SourcePoint);
  return(TRUE);
}

BOOL STDCALL
VGADDI_BltMask(SURFOBJ *Dest, SURFOBJ *Mask, XLATEOBJ *ColorTranslation,
	       RECTL *DestRect, POINTL *MaskPoint, BRUSHOBJ* Brush,
	       POINTL* BrushPoint)
{
  LONG i, j, dx, dy, idxColor, RGBulong = 0, c8;
  BYTE *initial, *tMask, *lMask;

  dx = DestRect->right  - DestRect->left;
  dy = DestRect->bottom - DestRect->top;

  if (ColorTranslation == NULL)
    {
      if (Mask != NULL)
	{
	  tMask = Mask->pvBits;
	  for (j=0; j<dy; j++)
	    {
	      lMask = tMask;
	      c8 = 0;
	      for (i=0; i<dx; i++)
		{
		  if((*lMask & maskbit[c8]) != 0)
		    {
		      vgaPutPixel(DestRect->left + i, DestRect->top + j, Brush->iSolidColor);
		    }
		  c8++;
		  if(c8 == 8) { lMask++; c8=0; }
		}
	      tMask += Mask->lDelta;
	    }
	}
    }
}

BOOL STDCALL
DrvBitBlt(SURFOBJ *Dest,
	  SURFOBJ *Source,
	  SURFOBJ *Mask,
	  CLIPOBJ *Clip,
	  XLATEOBJ *ColorTranslation,
	  RECTL *DestRect,
	  POINTL *SourcePoint,
	  POINTL *MaskPoint,
	  BRUSHOBJ *Brush,
	  POINTL *BrushPoint,
	  ROP4 rop4)
{
  /* Punt bitblts with complex clipping to the GDI. */
  if (Clip != NULL)
    {
      return(FALSE);
    }
  
  switch (rop4)
    {
    case BLACKNESS:
    case PATCOPY:
    case WHITENESS:
      return(VGADDI_BltBrush(Dest, ColorTranslation, DestRect, Brush,
			     BrushPoint, rop4));

    case SRCCOPY:
      return(VGADDI_BltSrc(Dest, Source, ColorTranslation, DestRect,
			   SourcePoint));

    case 0xAACC:
      return(VGADDI_BltMask(Dest, Mask, ColorTranslation, DestRect,
			    MaskPoint, Brush, BrushPoint));

    default:
      return(FALSE);
    }
}
