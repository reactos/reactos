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
/* $Id: dib4bpp.c,v 1.30 2004/04/07 20:04:22 navaraf Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <win32k/debug.h>
#include <debug.h>
#include <include/object.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
   PBYTE addr = SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
}

ULONG
DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
   PBYTE addr = SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   return (*addr >> ((1-(x&1))<<2)) & 0x0f;
}

VOID
DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
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
DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
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
		       SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		       PRECTL  DestRect,  POINTL  *SourcePoint,
		       XLATEOBJ* ColorTranslation)
{
  LONG     i, j, sx, sy, f2, xColor;
  PBYTE    SourceBits_24BPP, SourceLine_24BPP;
  PBYTE    DestBits, DestLine, SourceBits_8BPP, SourceLine_8BPP;
  PBYTE    SourceBits, SourceLine;

  DestBits = DestSurf->pvScan0 + (DestRect->left>>1) + DestRect->top * DestSurf->lDelta;

  switch(SourceGDI->BitsPerPixel)
  {
    case 1:
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

    case 4:
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

    case 8:
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

    case 16:
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

    case 24:
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

    case 32:
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
      DbgPrint("DIB_4BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }
  return(TRUE);
}

BOOLEAN
DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		PRECTL  DestRect,  POINTL  *SourcePoint,
		PBRUSHOBJ Brush, PPOINTL BrushOrigin,
		XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   LONG i, j, sx, sy;
   ULONG Dest, Source, Pattern = 0, PatternY;
   PULONG DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   LONG RoundedRight;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   PSURFOBJ PatternObj;
   ULONG PatternWidth, PatternHeight;
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
         DestGDI,
         SourceGDI,
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

         PatternBitmap = BITMAPOBJ_LockBitmap(GdiBrush->hbmPattern);
         PatternSurface = BitmapToSurf(PatternBitmap, NULL);
         BITMAPOBJ_UnlockBitmap(GdiBrush->hbmPattern);

         PatternObj = (PSURFOBJ)AccessUserObject((ULONG)PatternSurface);
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
         PatternY = j % PatternHeight;

      if (i & 0x1)
      {
         Dest = DIB_4BPP_GetPixel(DestSurf, i, j);

         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, SourceGDI, sx, sy, ColorTranslation);
         }

         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, i % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
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
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 1, sy, ColorTranslation)) | 
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 0, sy, ColorTranslation) << 4) |
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 3, sy, ColorTranslation) << 8) | 
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 2, sy, ColorTranslation) << 12) |
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 5, sy, ColorTranslation) << 16) | 
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 4, sy, ColorTranslation) << 20) |
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 7, sy, ColorTranslation) << 24) | 
               (DIB_GetSource(SourceSurf, SourceGDI, sx + 6, sy, ColorTranslation) << 28);
         }
         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, i % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 1) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 4;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 2) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 8;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 3) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 12;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 4) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 16;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 5) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 20;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 6) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 24;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + 7) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 28;
         }
         *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
      }

      /* Process the rest of pixel on the line */
      for (; i < DestRect->right; i++, sx++)
      {
         Dest = DIB_4BPP_GetPixel(DestSurf, i, j);
         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, SourceGDI, sx, sy, ColorTranslation);
         }
         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, i % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
         }				
         DIB_4BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xF);
      }	 
   }

   if (PatternSurface != NULL)
      EngDeleteSurface(PatternSurface);

   return TRUE;
}

BOOLEAN DIB_4BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode)
{
  DbgPrint("DIB_4BPP_StretchBlt: Source BPP: %u\n", SourceGDI->BitsPerPixel);
  return FALSE;
}

BOOLEAN 
DIB_4BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                        RECTL*  DestRect,  POINTL  *SourcePoint,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
