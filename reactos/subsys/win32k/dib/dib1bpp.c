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
/* $Id: dib1bpp.c,v 1.17 2004/04/05 21:26:24 navaraf Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <win32k/debug.h>
#include <include/object.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_1BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x >> 3);

  if ( !c )
    *addr &= ~MASK1BPP(x);
  else
    *addr |= MASK1BPP(x);
}

ULONG
DIB_1BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x >> 3);

  return (*addr & MASK1BPP(x) ? 1 : 0);
}

VOID
DIB_1BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  while(x1 < x2) {
    DIB_1BPP_PutPixel(SurfObj, x1, y, c);
    x1++;
  }
}

VOID
DIB_1BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  while(y1 < y2) {
    DIB_1BPP_PutPixel(SurfObj, x, y1, c);
    y1++;
  }
}

static
void
DIB_1BPP_BitBltSrcCopy_From1BPP (
	SURFOBJ* DestSurf, SURFOBJ* SourceSurf,
	PRECTL DestRect, POINTL *SourcePoint )
{
	// the 'window' in this sense is the x-position that corresponds
	// to the left-edge of the 8-pixel byte we are currently working with.
	// dwx is current x-window, dwx2 is the 'last' window we need to process
	int dwx, dwx2; // destination window x-position
	int swx; // source window y-position

	// left and right edges of source and dest rectangles
	int dl = DestRect->left; // dest left
	int dr = DestRect->right-1; // dest right (inclusive)
	int sl = SourcePoint->x; // source left
	int sr = sl + dr - dl; // source right (inclusive)

	// which direction are we going?
	int xinc;
	int yinc;

	// following 4 variables are used for the y-sweep
	int dy; // dest y
	int dy1; // dest y start
	int dy2; // dest y end
	int sy1; // src y start

	int shift;
	BYTE srcmask, dstmask;

	// 'd' and 's' are the dest & src buffer pointers that I use on my x-sweep
	// 'pd' and 'ps' are the dest & src buffer pointers used on the inner y-sweep
	PBYTE d, pd; // dest ptrs
	PBYTE s, ps; // src ptrs

	shift = (dl-sl)&7;

	if ( DestRect->top <= SourcePoint->y )
	{
		// moving up ( scan top -> bottom )
		dy1 = DestRect->top;
		dy2 = DestRect->bottom - 1;
		sy1 = SourcePoint->y;
		yinc = 1;
	}
	else
	{
		// moving down ( scan bottom -> top )
		dy1 = DestRect->bottom - 1;
		dy2 = DestRect->top;
		sy1 = SourcePoint->y + dy1 - dy2;
		yinc = -1;
	}
	if ( DestRect->left <= SourcePoint->x )
	{
		// moving left ( scan left->right )
		dwx = dl&~7;
		swx = (sl-(dl&7))&~7;
		dwx2 = dr&~7;
		xinc = 1;
	}
	else
	{
		// moving right ( scan right->left )
		dwx = dr&~7;
		swx = (sr-(dr&7))&~7; //(sr-7)&~7; // we need the left edge of this block... thus the -7
		dwx2 = dl&~7;
		xinc = -1;
	}
	d = &(((PBYTE)DestSurf->pvScan0)[dy1*DestSurf->lDelta + (dwx>>3)]);
	s = &(((PBYTE)SourceSurf->pvScan0)[sy1*SourceSurf->lDelta + (swx>>3)]);
	for ( ;; )
	{
		dy = dy1;
		pd = d;
		ps = s;
		srcmask = 0xff;
		int dx = dwx; /* dest x for this pass */
		if ( dwx < dl )
		{
			int diff = dl-dwx;
			srcmask &= (1<<(8-diff))-1;
			dx = dl;
		}
		if ( dwx+7 > dr )
		{
			int diff = dr-dwx+1;
			srcmask &= ~((1<<(8-diff))-1);
		}
		dstmask = ~srcmask;

		// we unfortunately *must* have 5 different versions of the inner
		// loop to be certain we don't try to read from memory that is not
		// needed and may in fact be invalid
		if ( !shift )
		{
			for ( ;; )
			{
				*pd = (BYTE)((*pd & dstmask) | (*ps & srcmask));

				// this *must* be here, because we could be going up *or* down...
				if ( dy == dy2 )
					break;
				dy += yinc;
				pd += yinc * DestSurf->lDelta;
				ps += yinc * SourceSurf->lDelta;
			}
		}
		else if ( !(0xFF00 & (srcmask<<shift) ) ) // check if ps[0] not needed...
		{
			for ( ;; )
			{
				*pd = (BYTE)((*pd & dstmask)
					| ( ( ps[1] >> shift ) & srcmask ));

				// this *must* be here, because we could be going up *or* down...
				if ( dy == dy2 )
					break;
				dy += yinc;
				pd += yinc * DestSurf->lDelta;
				ps += yinc * SourceSurf->lDelta;
			}
		}
		else if ( !(0xFF & (srcmask<<shift) ) ) // check if ps[1] not needed...
		{
			for ( ;; )
			{
				*pd = (*pd & dstmask)
					| ( ( ps[0] << ( 8 - shift ) ) & srcmask );

				// this *must* be here, because we could be going up *or* down...
				if ( dy == dy2 )
					break;
				dy += yinc;
				pd += yinc * DestSurf->lDelta;
				ps += yinc * SourceSurf->lDelta;
			}
		}
		else // both ps[0] and ps[1] are needed
		{
			for ( ;; )
			{
				*pd = (*pd & dstmask)
					| ( ( ( (ps[1])|(ps[0]<<8) ) >> shift ) & srcmask );

				// this *must* be here, because we could be going up *or* down...
				if ( dy == dy2 )
					break;
				dy += yinc;
				pd += yinc * DestSurf->lDelta;
				ps += yinc * SourceSurf->lDelta;
			}
		}

		// this *must* be here, because we could be going right *or* left...
		if ( dwx == dwx2 )
			break;
		d += xinc;
		s += xinc;
		dwx += xinc<<3;
		swx += xinc<<3;
	}
}

BOOLEAN
DIB_1BPP_BitBltSrcCopy(
	SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
	SURFGDI *DestGDI,  SURFGDI *SourceGDI,
	PRECTL  DestRect,  POINTL  *SourcePoint,
	XLATEOBJ *ColorTranslation)
{
	LONG i, j, sx, sy = SourcePoint->y;

	switch ( SourceGDI->BitsPerPixel )
	{
	case 1:
		DIB_1BPP_BitBltSrcCopy_From1BPP ( DestSurf, SourceSurf, DestRect, SourcePoint );
		break;

	case 4:
		for (j=DestRect->top; j<DestRect->bottom; j++)
		{
			sx = SourcePoint->x;
			for (i=DestRect->left; i<DestRect->right; i++)
			{
				if(XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)) == 0)
				{
					DIB_1BPP_PutPixel(DestSurf, i, j, 0);
				} else {
					DIB_1BPP_PutPixel(DestSurf, i, j, 1);
				}
				sx++;
			}
			sy++;
		}
		break;

	case 8:
		for (j=DestRect->top; j<DestRect->bottom; j++)
		{
			sx = SourcePoint->x;
			for (i=DestRect->left; i<DestRect->right; i++)
			{
				if(XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy)) == 0)
				{
					DIB_1BPP_PutPixel(DestSurf, i, j, 0);
				} else {
					DIB_1BPP_PutPixel(DestSurf, i, j, 1);
				}
				sx++;
			}
			sy++;
		}
		break;

	case 16:
		for (j=DestRect->top; j<DestRect->bottom; j++)
		{
			sx = SourcePoint->x;
			for (i=DestRect->left; i<DestRect->right; i++)
			{
				if(XLATEOBJ_iXlate(ColorTranslation, DIB_16BPP_GetPixel(SourceSurf, sx, sy)) == 0)
				{
					DIB_1BPP_PutPixel(DestSurf, i, j, 0);
				} else {
					DIB_1BPP_PutPixel(DestSurf, i, j, 1);
				}
				sx++;
			}
			sy++;
		}
		break;

	case 24:
		for (j=DestRect->top; j<DestRect->bottom; j++)
		{
			sx = SourcePoint->x;
			for (i=DestRect->left; i<DestRect->right; i++)
			{
				if(XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy)) == 0)
				{
					DIB_1BPP_PutPixel(DestSurf, i, j, 0);
				} else {
					DIB_1BPP_PutPixel(DestSurf, i, j, 1);
				}
				sx++;
			}
			sy++;
		}
		break;

	case 32:
		for (j=DestRect->top; j<DestRect->bottom; j++)
		{
			sx = SourcePoint->x;
			for (i=DestRect->left; i<DestRect->right; i++)
			{
				if(XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy)) == 0)
				{
					DIB_1BPP_PutPixel(DestSurf, i, j, 0);
				} else {
					DIB_1BPP_PutPixel(DestSurf, i, j, 1);
				}
				sx++;
			}
			sy++;
		}
		break;

	default:
		DbgPrint("DIB_1BPP_BitBlt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
DIB_1BPP_BitBlt(
	SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
	SURFGDI *DestGDI,  SURFGDI *SourceGDI,
	PRECTL  DestRect,  POINTL  *SourcePoint,
	PBRUSHOBJ Brush, PPOINTL BrushOrigin,
	XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   ULONG X, Y, SourceX, SourceY, k;
   ULONG Dest, Source, Pattern;
   PULONG DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   LONG RoundedRight;
   BYTE NoBits;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   PSURFOBJ PatternObj;
   ULONG PatternWidth, PatternHeight;

   if (Rop4 == SRCCOPY)
   {
      return DIB_1BPP_BitBltSrcCopy(
         DestSurf,
         SourceSurf,
         DestGDI,
         SourceGDI,
         DestRect,
         SourcePoint,
         ColorTranslation);
   }

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

   UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
   UsesPattern = ((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000);  
   RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x7);
   SourceY = SourcePoint->y;

   for (Y = DestRect->top; Y < DestRect->bottom; Y++)
   {
      SourceX = SourcePoint->x;
      DestBits = (PULONG)(
         DestSurf->pvScan0 +
         (DestRect->left >> 3) +
         Y * DestSurf->lDelta);

      X = DestRect->left;
      if (X & 7)
      {
         Dest = *((PBYTE)DestBits);
         NoBits = 7 - (X & 7);

         if (UsesSource)
         {
            Source = 0;
            for (k = 7 - NoBits; k < NoBits; k++)
               Source |= (DIB_GetSource(SourceSurf, SourceGDI, SourceX + k, SourceY, ColorTranslation) << (7 - k));
         }

         if (UsesPattern)
         {
            if (Brush->iSolidColor == 0xFFFFFFFF)
            {
               Pattern = 0;
               for (k = 7 - NoBits; k < NoBits; k++)
                  Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + k) % PatternWidth, Y % PatternHeight) << (7 - k));
            }
            else
            {
               Pattern = Brush->iSolidColor ? 0xFFFFFFFF : 0x00000000;
            }
         }

         Dest = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
         Dest &= ~((1 << (7 - NoBits)) - 1);
         Dest |= *((PBYTE)DestBits) & ((1 << (7 - NoBits)) - 1);
         
         X += NoBits;
         SourceX += NoBits;
      }

      for (; X < RoundedRight; X += 32, DestBits++, SourceX++)
      {
         Dest = *DestBits;

         if (UsesSource)
         {
            Source = 0;
            for (k = 0; k < 8; k++)
            {
               Source |= (DIB_GetSource(SourceSurf, SourceGDI, SourceX + k, SourceY, ColorTranslation) << (7 - k));
               Source |= (DIB_GetSource(SourceSurf, SourceGDI, SourceX + k + 8, SourceY, ColorTranslation) << (8 + (7 - k)));
               Source |= (DIB_GetSource(SourceSurf, SourceGDI, SourceX + k + 16, SourceY, ColorTranslation) << (16 + (7 - k)));
               Source |= (DIB_GetSource(SourceSurf, SourceGDI, SourceX + k + 24, SourceY, ColorTranslation) << (24 + (7 - k)));
            }
         }

         if (UsesPattern)
         {
            if (Brush->iSolidColor == 0xFFFFFFFF)
            {
               Pattern = 0;
               for (k = 0; k < 8; k++)
               {
                  Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + k) % PatternWidth, Y % PatternHeight) << (7 - k));
                  Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + k + 8) % PatternWidth, Y % PatternHeight) << (8 + (7 - k)));
                  Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + k + 16) % PatternWidth, Y % PatternHeight) << (16 + (7 - k)));
                  Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + k + 24) % PatternWidth, Y % PatternHeight) << (24 + (7 - k)));
               }
            }
            else
            {
               Pattern = Brush->iSolidColor ? 0xFFFFFFFF : 0x00000000;
            }
         }

         *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
      }

      if (X < DestRect->right)
      {
//         Dest = *DestBits;
         for (; X < DestRect->right; X++, SourceX++)
         {
//            Dest = *DestBits;
            Dest = DIB_1BPP_GetPixel(DestSurf, X, Y);

            if (UsesSource)
            {
               Source = DIB_GetSource(SourceSurf, SourceGDI, SourceX, SourceY, ColorTranslation);
            }

            if (UsesPattern)
            {
               if (Brush->iSolidColor == 0xFFFFFFFF)
                  Pattern = DIB_1BPP_GetPixel(PatternObj, X % PatternWidth, Y % PatternHeight);
               else
                  Pattern = Brush->iSolidColor ? 0xFFFFFFFF : 0x00000000;
            }

            DIB_1BPP_PutPixel(DestSurf, X, Y, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xF);
//            Dest >>= 1;
         }
      }

      SourceY++;
   }

   if (PatternSurface != NULL)
      EngDeleteSurface(PatternSurface);
  
   return TRUE;
}

BOOLEAN
DIB_1BPP_StretchBlt (
	SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
	SURFGDI *DestGDI, SURFGDI *SourceGDI,
	RECTL* DestRect, RECTL *SourceRect,
	POINTL* MaskOrigin, POINTL* BrushOrigin,
	XLATEOBJ *ColorTranslation, ULONG Mode)
{
	DbgPrint("DIB_1BPP_StretchBlt: Source BPP: %u\n", SourceGDI->BitsPerPixel);
	return FALSE;
}

/* EOF */
