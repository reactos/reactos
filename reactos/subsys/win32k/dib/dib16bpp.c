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
/* $Id: dib16bpp.c,v 1.24 2004/04/06 21:53:48 weiden Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <debug.h>
#include <ddk/winddi.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
#include <win32k/debug.h>
#include <include/object.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_16BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;

  *addr = (WORD)c;
}

ULONG
DIB_16BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;

  return (ULONG)(*addr);
}

VOID
DIB_16BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *addr = (WORD)c;
    ++addr;
    ++cx;
  }
}

VOID
DIB_16BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ < y2) {
    *addr = (WORD)c;

    byteaddr += lDelta;
    addr = (PWORD)byteaddr;
  }
}

BOOLEAN STATIC
DIB_16BPP_BitBltSrcCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		        PRECTL  DestRect,  POINTL  *SourcePoint,
		        XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + 2 * DestRect->left;

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
            DIB_16BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_16BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
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
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
          DIB_16BPP_PutPixel(DestSurf, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
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
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(ColorTranslation, *SourceBits);
          SourceBits += 1;
	  DestBits += 2;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 16:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 2 * (DestRect->right - DestRect->left));
		SourceBits += SourceSurf->lDelta;
		DestBits += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + 2 * SourcePoint->x;
	    DestBits = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + 2 * DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 2 * (DestRect->right - DestRect->left));
		SourceBits -= SourceSurf->lDelta;
		DestBits -= DestSurf->lDelta;
	      }
	  }
      }
      else
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
	    DestLine = DestBits;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
	        for (i = DestRect->left; i < DestRect->right; i++)
		  {
		    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(ColorTranslation, *((WORD *)SourceBits));
		    SourceBits += 2;
		    DestBits += 2;
	          }
		SourceLine += SourceSurf->lDelta;
		DestLine += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceLine = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + 2 * SourcePoint->x;
	    DestLine = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + 2 * DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
	        for (i = DestRect->left; i < DestRect->right; i++)
		  {
		    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(ColorTranslation, *((WORD *)SourceBits));
		    SourceBits += 2;
		    DestBits += 2;
	          }
		SourceLine -= SourceSurf->lDelta;
		DestLine -= DestSurf->lDelta;
	      }
	  }
      }
      break;

    case 24:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 3 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = (*(SourceBits + 2) << 0x10) +
             (*(SourceBits + 1) << 0x08) +
             (*(SourceBits));
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 3;
	  DestBits += 2;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
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
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(ColorTranslation, *((PDWORD) SourceBits));
          SourceBits += 4;
	  DestBits += 2;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_16BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_16BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		 SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		 PRECTL  DestRect,  POINTL  *SourcePoint,
		 PBRUSHOBJ Brush, PPOINTL BrushOrigin,
		 XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   ULONG X, Y;
   ULONG SourceX, SourceY;
   ULONG wd, Dest, Source, Pattern = 0;
   PULONG DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   LONG RoundedRight;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   PSURFOBJ PatternObj;
   ULONG PatternWidth, PatternHeight;

   if (Rop4 == SRCCOPY)
   {
      return DIB_16BPP_BitBltSrcCopy(
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
   
   wd = ((DestRect->right - DestRect->left) << 1) - DestSurf->lDelta;
   RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x1);
   SourceY = SourcePoint->y;
   DestBits = (PULONG)(
      DestSurf->pvScan0 +
      (DestRect->left << 1) +
      DestRect->top * DestSurf->lDelta);

   for (Y = DestRect->top; Y < DestRect->bottom; Y++)
   {
      SourceX = SourcePoint->x;
      for (X = DestRect->left; X < RoundedRight; X += 2, DestBits++, SourceX += 2)
      {
         Dest = *DestBits;
 
         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, SourceGDI, SourceX, SourceY, ColorTranslation);
            Source |= DIB_GetSource(SourceSurf, SourceGDI, SourceX + 1, SourceY, ColorTranslation) << 16;
         }

         if (UsesPattern)
	 {
            if (Brush->iSolidColor == 0xFFFFFFFF)
            {
               Pattern = (DIB_1BPP_GetPixel(PatternObj, X % PatternWidth, Y % PatternHeight) ? GdiBrush->crFore : GdiBrush->crBack);
               Pattern |= (DIB_1BPP_GetPixel(PatternObj, (X + 1) % PatternWidth, Y % PatternHeight) ? GdiBrush->crFore : GdiBrush->crBack) << 16;
            }
            else
            {
               Pattern = (Brush->iSolidColor & 0xFFFF) |
                        ((Brush->iSolidColor & 0xFFFF) << 16);
            }
         }

         *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);
      }

      if (X < DestRect->right)
      {
         Dest = *((PUSHORT)DestBits);

         if (UsesSource)
         {
            Source = DIB_GetSource(SourceSurf, SourceGDI, SourceX, SourceY, ColorTranslation);
         }

         if (UsesPattern)
         {
            if (Brush->iSolidColor == 0xFFFFFFFF)
               Pattern = DIB_1BPP_GetPixel(PatternObj, X % PatternWidth, Y % PatternHeight) ? GdiBrush->crFore : GdiBrush->crBack;
            else
               Pattern = Brush->iSolidColor & 0xFFFF;
         }				

         DIB_16BPP_PutPixel(DestSurf, X, Y, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xFFFF);
         DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
      }

      SourceY++;
      DestBits = (PULONG)(
         (ULONG_PTR)DestBits - wd);
   }

   if (PatternSurface != NULL)
      EngDeleteSurface(PatternSurface);
  
   return TRUE;
}


/*
=======================================
 Stretching functions goes below
 Some parts of code are based on an
 article "Bresenhame image scaling"
 Dr. Dobb Journal, May 2002
=======================================
*/

typedef unsigned short PIXEL;

/* 16-bit HiColor (565 format) */
inline PIXEL average16(PIXEL a, PIXEL b)
{
// This one doesn't work
/*
  if (a == b) {
    return a;
  } else {
    unsigned short mask = ~ (((a | b) & 0x0410) << 1);
    return ((a & mask) + (b & mask)) >> 1;
  }*/ /* if */

// This one should be correct, but it's too long
/*  
  unsigned char r1, g1, b1, r2, g2, b2, rr, gr, br;
  unsigned short res;
  
  r1 = (a & 0xF800) >> 11;
  g1 = (a & 0x7E0) >> 5;
  b1 = (a & 0x1F);
  
  r2 = (b & 0xF800) >> 11;
  g2 = (b & 0x7E0) >> 5;
  b2 = (b & 0x1F);
  
  rr = (r1+r2) / 2;
  gr = (g1+g2) / 2;
  br = (b1+b2) / 2;
  
  res = (rr << 11) + (gr << 5) + br;

  return res;
*/
  return a; // FIXME: Depend on SetStretchMode
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
void ScaleLineAvg16(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
{
  int NumPixels = TgtWidth;
  int IntPart = SrcWidth / TgtWidth;
  int FractPart = SrcWidth % TgtWidth;
  int Mid = TgtWidth >> 1;
  int E = 0;
  int skip;
  PIXEL p;

  skip = (TgtWidth < SrcWidth) ? 0 : (TgtWidth / (2*SrcWidth) + 1);
  NumPixels -= skip;

  while (NumPixels-- > 0) {
    p = *Source;
    if (E >= Mid)
      p = average16(p, *(Source+1));
    *Target++ = p;
    Source += IntPart;
    E += FractPart;
    if (E >= TgtWidth) {
      E -= TgtWidth;
      Source++;
    } /* if */
  } /* while */
  while (skip-- > 0)
    *Target++ = *Source;
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
void ScaleRectAvg16(PIXEL *Target, PIXEL *Source, int SrcWidth, int SrcHeight,
                  int TgtWidth, int TgtHeight, int srcPitch, int dstPitch)
{
  int NumPixels = TgtHeight;
  int IntPart = ((SrcHeight / TgtHeight) * srcPitch) >> 1; //(SrcHeight / TgtHeight) * SrcWidth;
  int FractPart = SrcHeight % TgtHeight;
  int Mid = TgtHeight >> 1;
  int E = 0;
  int skip;
  PIXEL *ScanLine, *ScanLineAhead;
  PIXEL *PrevSource = NULL;
  PIXEL *PrevSourceAhead = NULL;

  skip = (TgtHeight < SrcHeight) ? 0 : (TgtHeight / (2*SrcHeight) + 1);
  NumPixels -= skip;

  ScanLine = (PIXEL*)ExAllocatePool(NonPagedPool, TgtWidth*sizeof(PIXEL)); // FIXME: Should we use PagedPool here?
  ScanLineAhead = (PIXEL *)ExAllocatePool(NonPagedPool, TgtWidth*sizeof(PIXEL));

  while (NumPixels-- > 0) {
    if (Source != PrevSource) {
      if (Source == PrevSourceAhead) {
        /* the next scan line has already been scaled and stored in
         * ScanLineAhead; swap the buffers that ScanLine and ScanLineAhead
         * point to
         */
        PIXEL *tmp = ScanLine;
        ScanLine = ScanLineAhead;
        ScanLineAhead = tmp;
      } else {
        ScaleLineAvg16(ScanLine, Source, SrcWidth, TgtWidth);
      } /* if */
      PrevSource = Source;
    } /* if */
    
    if (E >= Mid && PrevSourceAhead != (PIXEL *)((BYTE *)Source + srcPitch)) {
      int x;
      ScaleLineAvg16(ScanLineAhead, (PIXEL *)((BYTE *)Source + srcPitch), SrcWidth, TgtWidth);
      for (x = 0; x < TgtWidth; x++)
        ScanLine[x] = average16(ScanLine[x], ScanLineAhead[x]);
      PrevSourceAhead = (PIXEL *)((BYTE *)Source + srcPitch);
    } /* if */
    
    memcpy(Target, ScanLine, TgtWidth*sizeof(PIXEL));
    Target = (PIXEL *)((BYTE *)Target + dstPitch);
    Source += IntPart;
    E += FractPart;
    if (E >= TgtHeight) {
      E -= TgtHeight;
      Source = (PIXEL *)((BYTE *)Source + srcPitch);
    } /* if */
  } /* while */

  if (skip > 0 && Source != PrevSource)
    ScaleLineAvg16(ScanLine, Source, SrcWidth, TgtWidth);
  while (skip-- > 0) {
    memcpy(Target, ScanLine, TgtWidth*sizeof(PIXEL));
    Target = (PIXEL *)((BYTE *)Target + dstPitch);
  } /* while */

  ExFreePool(ScanLine);
  ExFreePool(ScanLineAhead);
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
BOOLEAN DIB_16BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode)
{
  BYTE *SourceLine, *DestLine;
  
  DbgPrint("DIB_16BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
     SourceGDI->BitsPerPixel, SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
     DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    switch(SourceGDI->BitsPerPixel)
    {
      case 1:
         return FALSE;      
      break;
  
      case 4:
         return FALSE;
      break;
  
      case 8:
         return FALSE;
      break;

      case 16:
	    SourceLine = SourceSurf->pvScan0 + (SourceRect->top * SourceSurf->lDelta) + 2 * SourceRect->left;
	    DestLine = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + 2 * DestRect->left;

        ScaleRectAvg16((PIXEL *)DestLine, (PIXEL *)SourceLine,
           SourceRect->right-SourceRect->left, SourceRect->bottom-SourceRect->top, 
           DestRect->right-DestRect->left, DestRect->bottom-DestRect->top, SourceSurf->lDelta, DestSurf->lDelta);
      break;
    
      case 24:
         return FALSE;
      break;
      
      case 32:
         return FALSE;
      break;
      
      default:
         DbgPrint("DIB_16BPP_StretchBlt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
    }

  
    
  return TRUE;
}

BOOLEAN 
DIB_16BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                         RECTL*  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  ULONG X, Y, SourceX, SourceY, Source, wd, Dest;
  LONG RoundedRight;
  ULONG *DestBits;
  
  RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x1);
  SourceY = SourcePoint->y;
  DestBits = (ULONG*)(DestSurf->pvScan0 +
                      (DestRect->left << 1) +
                      DestRect->top * DestSurf->lDelta);
  wd = ((DestRect->right - DestRect->left) << 1) - DestSurf->lDelta;
  
  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceX = SourcePoint->x;
    for(X = DestRect->left; X < RoundedRight; X += 2, DestBits++, SourceX += 2)
    {
      Dest = *DestBits;
      
      Source = DIB_GetSourceIndex(SourceSurf, SourceGDI, SourceX, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFF0000;
        Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFFFF);
      }

      Source = DIB_GetSourceIndex(SourceSurf, SourceGDI, SourceX + 1, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFF;
        Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) << 16);
      }

      *DestBits = Dest;
    }
    
    if(X < DestRect->right)
    {
      Source = DIB_GetSourceIndex(SourceSurf, SourceGDI, SourceX, SourceY);
      if(Source != iTransColor)
      {
        *((USHORT*)DestBits) = (USHORT)(XLATEOBJ_iXlate(ColorTranslation, Source) << 16);
      }
      
      DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
    }
    SourceY++;
    DestBits = (ULONG*)((ULONG_PTR)DestBits - wd);
  }
  
  return TRUE;
}

/* EOF */
