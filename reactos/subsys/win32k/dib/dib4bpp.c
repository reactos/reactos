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
/* $Id: dib4bpp.c,v 1.37 2004/07/03 17:40:24 navaraf Exp $ */
#include <w32k.h>

VOID
DIB_4BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
   PBYTE addr = SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
}

ULONG
DIB_4BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
   PBYTE addr = SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   return (*addr >> ((1-(x&1))<<2)) & 0x0f;
}

VOID
DIB_4BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE  addr = SurfObj->pvScan0 + (x1>>1) + y * SurfObj->lDelta;
  LONG  cx = x1;

  while(cx < x2) {
    *addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
    if((++x1 & 1) == 0)
      ++addr;
    ++cx;
  }
}

VOID
DIB_4BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE  addr = SurfObj->pvScan0;
  int  lDelta = SurfObj->lDelta;

  addr += (x>>1) + y1 * lDelta;
  while(y1++ < y2) {
    *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
    addr += lDelta;
  }
}

BOOLEAN STATIC
DIB_4BPP_BitBltSrcCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		       PRECTL  DestRect,  POINTL  *SourcePoint,
		       XLATEOBJ* ColorTranslation)
{
  LONG     i, j, sx, sy, f2, xColor;
  PBYTE    SourceBits_24BPP, SourceLine_24BPP;
  PBYTE    DestBits, DestLine, SourceBits_8BPP, SourceLine_8BPP;
  PBYTE    SourceBits, SourceLine;

  DestBits = DestSurf->pvScan0 + (DestRect->left>>1) + DestRect->top * DestSurf->lDelta;

  switch(SourceSurf->iBitmapFormat)
  {
    case BMF_1BPP:
      sx = SourcePoint->x;
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
          {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case BMF_4BPP:
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
	  if (NULL != ColorTranslation)
	  {
	    DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)));
	  }
	  else
	  {
	    DIB_4BPP_PutPixel(DestSurf, i, j, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
	  }
          sx++;
        }
        sy++;
      }
      break;

    case BMF_8BPP:
      SourceBits_8BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_8BPP = SourceBits_8BPP;
        DestLine = DestBits;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          *DestLine = (*DestLine & notmask[f2]) |
            ((XLATEOBJ_iXlate(ColorTranslation, *SourceLine_8BPP)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_8BPP++;
        }

        SourceBits_8BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case BMF_16BPP:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;
        f2 = DestRect->left & 1;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = (*DestBits & notmask[f2]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }
          SourceBits += 2;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case BMF_24BPP:
      SourceBits_24BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x * 3;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_24BPP = SourceBits_24BPP;
        DestLine = DestBits;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = (*(SourceLine_24BPP + 2) << 0x10) +
             (*(SourceLine_24BPP + 1) << 0x08) +
             (*(SourceLine_24BPP));
          *DestLine = (*DestLine & notmask[f2]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_24BPP+=3;
        }

        SourceBits_24BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case BMF_32BPP:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;
        f2 = DestRect->left & 1;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = (*DestBits & notmask[f2]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }
          SourceBits += 4;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_4BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
  }
  return(TRUE);
}

BOOLEAN
DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		PRECTL  DestRect,  POINTL  *SourcePoint,
		BRUSHOBJ *Brush,   POINTL BrushOrigin,
		XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   LONG i, j, sx, sy;
   ULONG Dest, Source = 0, Pattern = 0;
   PULONG DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   LONG RoundedRight;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush = NULL;
   HBITMAP PatternSurface = NULL;
   SURFOBJ *PatternObj = NULL;
   ULONG PatternWidth = 0, PatternHeight = 0, PatternY = 0;
   static const ULONG ExpandSolidColor[16] = 
   {
      0x00000000 /* 0 */,
      0x11111111 /* 1 */,
      0x22222222 /* 2 */,
      0x33333333 /* 3 */,
      0x44444444 /* 4 */,
      0x55555555 /* 5 */,
      0x66666666 /* 6 */,
      0x77777777 /* 7 */,
      0x88888888 /* 8 */,
      0x99999999 /* 9 */,
      0xAAAAAAAA /* 10 */,
      0xBBBBBBBB /* 11 */,
      0xCCCCCCCC /* 12 */,
      0xDDDDDDDD /* 13 */,
      0xEEEEEEEE /* 14 */,
      0xFFFFFFFF /* 15 */,
   };

   if (Rop4 == SRCCOPY)
   {
      return DIB_4BPP_BitBltSrcCopy(
         DestSurf,
         SourceSurf,
         DestRect,
         SourcePoint,
         ColorTranslation);
   }

   UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
   UsesPattern = (((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000)) && Brush;  

   if (UsesPattern)
   {
      if (Brush->iSolidColor == 0xFFFFFFFF)
      {
         PBITMAPOBJ PatternBitmap;

         GdiBrush = CONTAINING_RECORD(
            Brush,
            GDIBRUSHOBJ,
            BrushObject);

         PatternSurface = GdiBrush->hbmPattern;
         PatternBitmap = BITMAPOBJ_LockBitmap(GdiBrush->hbmPattern);

         PatternObj = &PatternBitmap->SurfObj;
         PatternWidth = PatternObj->sizlBitmap.cx;
         PatternHeight = PatternObj->sizlBitmap.cy;

         UsesPattern = TRUE;
      }
      else
      {
         UsesPattern = FALSE;
         Pattern = ExpandSolidColor[Brush->iSolidColor & 0xF];
      }
   }
   
   sy = SourcePoint->y;
   RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x7);

   for (j = DestRect->top; j < DestRect->bottom; j++, sy++)
   {
      DestBits = (PULONG)(DestSurf->pvScan0 + (DestRect->left >> 1) + j * DestSurf->lDelta);
      sx = SourcePoint->x;
      i = DestRect->left;

      if (UsesPattern)
         PatternY = (j + BrushOrigin.y) % PatternHeight;

      if (i & 0x1)
      {
         Dest = DIB_4BPP_GetPixel(DestSurf, i, j);

         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, sx, sy, ColorTranslation);
         }

         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
         }				

         DIB_4BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xF);

         i++;
         sx++;
         DestBits = (PULONG)((ULONG_PTR)DestBits + 1);
      }

      for (; i < RoundedRight; i += 8, sx += 8, DestBits++)
      {
         Dest = *DestBits;
         if (UsesSource)
         {
            Source =
               (DIB_GetSource(SourceSurf, sx + 1, sy, ColorTranslation)) | 
               (DIB_GetSource(SourceSurf, sx + 0, sy, ColorTranslation) << 4) |
               (DIB_GetSource(SourceSurf, sx + 3, sy, ColorTranslation) << 8) | 
               (DIB_GetSource(SourceSurf, sx + 2, sy, ColorTranslation) << 12) |
               (DIB_GetSource(SourceSurf, sx + 5, sy, ColorTranslation) << 16) | 
               (DIB_GetSource(SourceSurf, sx + 4, sy, ColorTranslation) << 20) |
               (DIB_GetSource(SourceSurf, sx + 7, sy, ColorTranslation) << 24) | 
               (DIB_GetSource(SourceSurf, sx + 6, sy, ColorTranslation) << 28);
         }
         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 1) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 0) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 4;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 3) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 8;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 2) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 12;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 5) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 16;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 4) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 20;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 7) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 24;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 6) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 28;
         }
         *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
      }

      /* Process the rest of pixel on the line */
      for (; i < DestRect->right; i++, sx++)
      {
         Dest = DIB_4BPP_GetPixel(DestSurf, i, j);
         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, sx, sy, ColorTranslation);
         }
         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
         }				
         DIB_4BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xF);
      }	 
   }

   if (PatternSurface != NULL)
      BITMAPOBJ_UnlockBitmap(PatternSurface);

   return TRUE;
}

BOOLEAN DIB_4BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode)
{
  DbgPrint("DIB_4BPP_StretchBlt: Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
  return FALSE;
}

BOOLEAN 
DIB_4BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL*  DestRect,  POINTL  *SourcePoint,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
