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
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

VOID
DIB_8BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  *byteaddr = c;
}

ULONG
DIB_8BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  return (ULONG)(*byteaddr);
}

VOID
DIB_8BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  memset((PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x1, (BYTE) c, x2 - x1);
}

VOID
DIB_8BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PBYTE addr = byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = addr;
  while(y1++ < y2) {
    *addr = c;

    addr += lDelta;
  }
}

BOOLEAN
DIB_8BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;

  switch(BltInfo->SourceSurface->iBitmapFormat)
  {
    case BMF_1BPP:
      sx = BltInfo->SourcePoint.x;
      sy = BltInfo->SourcePoint.y;

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        sx = BltInfo->SourcePoint.x;
        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          if(DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
          {
            DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          } else {
            DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case BMF_4BPP:
      SourceBits_4BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + (BltInfo->SourcePoint.x >> 1);

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        sx = BltInfo->SourcePoint.x;
        f1 = sx & 1;

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest,
              (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
          DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += BltInfo->SourceSurface->lDelta;
      }
      break;

    case BMF_8BPP:
      if (NULL == BltInfo->XlateSourceToDest || 0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, BltInfo->DestRect.right - BltInfo->DestRect.left);
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, BltInfo->DestRect.right - BltInfo->DestRect.left);
		SourceBits -= BltInfo->SourceSurface->lDelta;
		DestBits -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      else
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    DestLine = DestBits;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits++);
		  }
		SourceLine += BltInfo->SourceSurface->lDelta;
		DestLine += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits++);
		  }
		SourceLine -= BltInfo->SourceSurface->lDelta;
		DestLine -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      break;

    case BMF_16BPP:
      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 2;
	  DestBits += 1;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_24BPP:
      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = (*(SourceBits + 2) << 0x10) +
             (*(SourceBits + 1) << 0x08) +
             (*(SourceBits));
          *DestBits = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 3;
	  DestBits += 1;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_32BPP:
      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 4;
	  DestBits += 1;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DPRINT1("DIB_8BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

/* BitBlt Optimize */
BOOLEAN
DIB_8BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  ULONG DestY;
  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
    {
      DIB_8BPP_HLine(DestSurface, DestRect->left, DestRect->right, DestY, color);
    }

  return TRUE;
}


BOOLEAN
DIB_8BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL*  DestRect,  POINTL  *SourcePoint,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  ULONG RoundedRight, X, Y, SourceX, SourceY, Source, wd, Dest;
  ULONG *DestBits;

  RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x3);
  SourceY = SourcePoint->y;
  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 + DestRect->left +
                      (DestRect->top * DestSurf->lDelta));
  wd = DestSurf->lDelta - (DestRect->right - DestRect->left);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 + DestRect->left +
                        (Y * DestSurf->lDelta));
    SourceX = SourcePoint->x;
    for (X = DestRect->left; X < RoundedRight; X += 4, DestBits++)
    {
      Dest = *DestBits;

      Source = DIB_GetSourceIndex(SourceSurf, SourceX++, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFFFF00;
        Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFF);
      }

      Source = DIB_GetSourceIndex(SourceSurf, SourceX++, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFF00FF;
        Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 8) & 0xFF00);
      }

      Source = DIB_GetSourceIndex(SourceSurf, SourceX++, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFF00FFFF;
        Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 16) & 0xFF0000);
      }

      Source = DIB_GetSourceIndex(SourceSurf, SourceX++, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0x00FFFFFF;
        Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 24) & 0xFF000000);
      }

      *DestBits = Dest;
    }

    if(X < DestRect->right)
    {
      for (; X < DestRect->right; X++)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX++, SourceY);
        if(Source != iTransColor)
        {
          *((BYTE*)DestBits) = (BYTE)(XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFF);
        }
        DestBits = (PULONG)((ULONG_PTR)DestBits + 1);
      }
    }
    SourceY++;
  }

  return TRUE;
}

typedef union {
   ULONG ul;
   struct {
      UCHAR red;
      UCHAR green;
      UCHAR blue;
      UCHAR alpha;
   } col;
} NICEPIXEL32;

typedef union {
   USHORT us;
   struct {
      USHORT red:5,
             green:6,
             blue:5;
   } col;
} NICEPIXEL16;

static __inline UCHAR
Clamp8(ULONG val)
{
   return (val > 255) ? 255 : val;
}

BOOLEAN
DIB_8BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                    RECTL* SourceRect, CLIPOBJ* ClipRegion,
                    XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
   INT Rows, Cols, SrcX, SrcY;
   register PUCHAR Dst;
   ULONG DstDelta;
   BLENDFUNCTION BlendFunc;
   register NICEPIXEL32 DstPixel;
   register NICEPIXEL32 SrcPixel;
   register NICEPIXEL16 SrcPixel16;
   UCHAR Alpha, SrcBpp;
   HPALETTE SrcPalette, DstPalette;
   XLATEOBJ *SrcTo32Xlate, *DstTo32Xlate, *DstFrom32Xlate;

   DPRINT("DIB_8BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
          SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
          DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

   ASSERT(DestRect->bottom - DestRect->top == SourceRect->bottom - SourceRect->top &&
          DestRect->right - DestRect->left == SourceRect->right - SourceRect->left);

   BlendFunc = BlendObj->BlendFunction;
   if (BlendFunc.BlendOp != AC_SRC_OVER)
   {
      DPRINT1("BlendOp != AC_SRC_OVER\n");
      return FALSE;
   }
   if (BlendFunc.BlendFlags != 0)
   {
      DPRINT1("BlendFlags != 0\n");
      return FALSE;
   }
   if ((BlendFunc.AlphaFormat & ~AC_SRC_ALPHA) != 0)
   {
      DPRINT1("Unsupported AlphaFormat (0x%x)\n", BlendFunc.AlphaFormat);
      return FALSE;
   }
   if ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0 &&
       BitsPerFormat(Source->iBitmapFormat) != 32)
   {
      DPRINT1("Source bitmap must be 32bpp when AC_SRC_ALPHA is set\n");
      return FALSE;
   }

   SrcBpp = BitsPerFormat(Source->iBitmapFormat);
   SrcPalette = IntEngGetXlatePalette(ColorTranslation, XO_SRCPALETTE);
   if (SrcPalette != 0)
   {
      SrcTo32Xlate = IntEngCreateXlate(PAL_RGB, 0, NULL, SrcPalette);
      if (SrcTo32Xlate == NULL)
      {
         DPRINT1("IntEngCreateXlate failed\n");
         return FALSE;
      }
   }
   else
   {
      SrcTo32Xlate = NULL;
      ASSERT(SrcBpp >= 16);
   }

   DstPalette = IntEngGetXlatePalette(ColorTranslation, XO_DESTPALETTE);
   DstTo32Xlate = IntEngCreateXlate(PAL_RGB, 0, NULL, DstPalette);
   DstFrom32Xlate = IntEngCreateXlate(0, PAL_RGB, DstPalette, NULL);
   if (DstTo32Xlate == NULL || DstFrom32Xlate == NULL)
   {
      if (SrcTo32Xlate != NULL)
         EngDeleteXlate(SrcTo32Xlate);
      if (DstTo32Xlate != NULL)
         EngDeleteXlate(DstTo32Xlate);
      if (DstFrom32Xlate != NULL)
         EngDeleteXlate(DstFrom32Xlate);
      DPRINT1("IntEngCreateXlate failed\n");
      return FALSE;
   }

   Dst = (PUCHAR)((ULONG_PTR)Dest->pvScan0 + (DestRect->top * Dest->lDelta) +
                             DestRect->left);
   DstDelta = Dest->lDelta - (DestRect->right - DestRect->left);

   Rows = DestRect->bottom - DestRect->top;
   SrcY = SourceRect->top;
   while (--Rows >= 0)
   {
      Cols = DestRect->right - DestRect->left;
      SrcX = SourceRect->left;
      while (--Cols >= 0)
      {
         if (SrcTo32Xlate != NULL)
         {
            SrcPixel.ul = DIB_GetSource(Source, SrcX++, SrcY, SrcTo32Xlate);
         }
         else if (SrcBpp <= 16)
         {
            SrcPixel16.us = DIB_GetSource(Source, SrcX++, SrcY, ColorTranslation);
            SrcPixel.col.red = (SrcPixel16.col.red << 3) | (SrcPixel16.col.red >> 2);
            SrcPixel.col.green = (SrcPixel16.col.green << 2) | (SrcPixel16.col.green >> 4);
            SrcPixel.col.blue = (SrcPixel16.col.blue << 3) | (SrcPixel16.col.blue >> 2);
         }
         else
         {
            SrcPixel.ul = DIB_GetSourceIndex(Source, SrcX++, SrcY);
         }
         SrcPixel.col.red = SrcPixel.col.red * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.green = SrcPixel.col.green * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.blue = SrcPixel.col.blue * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.alpha = (SrcBpp == 32) ? (SrcPixel.col.alpha * BlendFunc.SourceConstantAlpha / 255) : BlendFunc.SourceConstantAlpha;

         Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
                 SrcPixel.col.alpha : BlendFunc.SourceConstantAlpha;

         DstPixel.ul = XLATEOBJ_iXlate(DstTo32Xlate, *Dst);
         DstPixel.col.red = Clamp8(DstPixel.col.red * (255 - Alpha) / 255 + SrcPixel.col.red);
         DstPixel.col.green = Clamp8(DstPixel.col.green * (255 - Alpha) / 255 + SrcPixel.col.green);
         DstPixel.col.blue = Clamp8(DstPixel.col.blue * (255 - Alpha) / 255 + SrcPixel.col.blue);
         *Dst++ = XLATEOBJ_iXlate(DstFrom32Xlate, DstPixel.ul);
      }
      Dst = (PUCHAR)((ULONG_PTR)Dst + DstDelta);
      SrcY++;
   }

   if (SrcTo32Xlate != NULL)
      EngDeleteXlate(SrcTo32Xlate);
   if (DstTo32Xlate != NULL)
      EngDeleteXlate(DstTo32Xlate);
   if (DstFrom32Xlate != NULL)
      EngDeleteXlate(DstFrom32Xlate);

   return TRUE;
}

/* EOF */
