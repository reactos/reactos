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
/* $Id: dib24bpp.c,v 1.19 2004/04/06 17:54:32 weiden Exp $ */
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
DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + (y * SurfObj->lDelta) + (x << 1) + x;
  *(PUSHORT)(addr) = c & 0xFFFF;
  *(addr + 2) = (c >> 16) & 0xFF;
}

ULONG
DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x << 1) + x;
  return *(PUSHORT)(addr) + (*(addr + 2) << 16);
}

VOID
DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x1 << 1) + x1;
  LONG cx = x1;

  c &= 0xFFFFFF;
  while(cx < x2) {
    *(PUSHORT)(addr) = c & 0xFFFF;
    addr += 2;
    *(addr) = c >> 16;
    addr += 1;
    ++cx;
  }
}

VOID
DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + y1 * SurfObj->lDelta + (x << 1) + x;
  LONG lDelta = SurfObj->lDelta;

  c &= 0xFFFFFF;
  while(y1++ < y2) {
    *(PUSHORT)(addr) = c & 0xFFFF;
    *(addr + 2) = c >> 16;

    addr += lDelta;
  }
}

BOOLEAN
DIB_24BPP_BitBltSrcCopy(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		          SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		          PRECTL  DestRect,  POINTL  *SourcePoint,
		          XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left * 3;

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
            DIB_24BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_24BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      SourceBits_4BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + (SourcePoint->x >> 1);

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case 8:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation, *SourceBits);
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 1;
	  DestBits += 3;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 16:
      SourceBits_16BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_16BPP = SourceBits_16BPP;
        DestLine = DestBits;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation, *SourceLine_16BPP);
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          SourceLine_16BPP++;
        }

        SourceBits_16BPP = (PWORD)((PBYTE)SourceBits_16BPP + SourceSurf->lDelta);
        DestBits += DestSurf->lDelta;
      }
      break;

    case 24:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 3 * SourcePoint->x;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (DestRect->right - DestRect->left));
		SourceBits += SourceSurf->lDelta;
		DestBits += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + 3 * SourcePoint->x;
	    DestBits = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + 3 * DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (DestRect->right - DestRect->left));
		SourceBits -= SourceSurf->lDelta;
		DestBits -= DestSurf->lDelta;
	      }
	  }
      }
      else
      {
	/* FIXME */
	DPRINT1("DIB_24BPP_Bitblt: Unhandled ColorTranslation for 16 -> 16 copy");
        return FALSE;
      }
      break;

    case 32:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation, *((PDWORD) SourceBits));
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 4;
	  DestBits += 3;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_24BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		 SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		 PRECTL  DestRect,  POINTL  *SourcePoint,
		 PBRUSHOBJ Brush, PPOINTL BrushOrigin,
		 XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   ULONG X, Y;
   ULONG SourceX, SourceY;
   ULONG Dest, Source, Pattern;
   PBYTE DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   PSURFOBJ PatternObj;
   ULONG PatternWidth, PatternHeight;

   if (Rop4 == SRCCOPY)
   {
      return DIB_24BPP_BitBltSrcCopy(
         DestSurf,
         SourceSurf,
         DestGDI,
         SourceGDI,
         DestRect,
         SourcePoint,
         ColorTranslation);
   }

   UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
   UsesPattern = ((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000);  
      
   if (UsesPattern)
   {
      if (Brush == NULL)
      {
         UsesPattern = FALSE;
      } else
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
      }
   }

   SourceY = SourcePoint->y;
   DestBits = (PBYTE)(
      DestSurf->pvScan0 +
      (DestRect->left << 1) + DestRect->left +
      DestRect->top * DestSurf->lDelta);

   for (Y = DestRect->top; Y < DestRect->bottom; Y++)
   {
      SourceX = SourcePoint->x;
      for (X = DestRect->left; X < DestRect->right; X++, DestBits += 3, SourceX++)
      {
         Dest = *((PUSHORT)DestBits) + (*(DestBits + 2) << 16);
 
         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, SourceGDI, SourceX, SourceY, ColorTranslation);
         }

         if (UsesPattern)
	 {
            if (Brush->iSolidColor == 0xFFFFFFFF)
            {
               Pattern = DIB_1BPP_GetPixel(PatternObj, X % PatternWidth, Y % PatternHeight) ? GdiBrush->crFore : GdiBrush->crBack;
            }
            else
            {
               Pattern = Brush->iSolidColor;
            }
         }

         Dest = DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xFFFFFF;
         *(PUSHORT)(DestBits) = Dest & 0xFFFF;
         *(DestBits + 2) = Dest >> 16;
      }

      SourceY++;
      DestBits -= (DestRect->right - DestRect->left) * 3;
      DestBits += DestSurf->lDelta;
   }

   if (PatternSurface != NULL)
      EngDeleteSurface(PatternSurface);
  
   return TRUE;
}

BOOLEAN DIB_24BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode)
{
  DbgPrint("DIB_24BPP_StretchBlt: Source BPP: %u\n", SourceGDI->BitsPerPixel);
  return FALSE;
}

BOOLEAN 
DIB_24BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                         RECTL*  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
