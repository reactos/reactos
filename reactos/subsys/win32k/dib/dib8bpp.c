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
/* $Id: dib8bpp.c,v 1.27 2004/07/03 13:55:35 navaraf Exp $ */
#include <w32k.h>

VOID
DIB_8BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  *byteaddr = c;
}

ULONG
DIB_8BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  return (ULONG)(*byteaddr);
}

VOID
DIB_8BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PBYTE addr = byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *addr = c;
    ++addr;
    ++cx;
  }
}

VOID
DIB_8BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PBYTE addr = byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = addr;
  while(y1++ < y2) {
    *addr = c;

    addr += lDelta;
  }
}

BOOLEAN
DIB_8BPP_BitBltSrcCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		       PRECTL  DestRect,  POINTL  *SourcePoint,
		       XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left;

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
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case BMF_4BPP:
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
          DIB_8BPP_PutPixel(DestSurf, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
      }
      break;

    case BMF_8BPP:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, DestRect->right - DestRect->left);
		SourceBits += SourceSurf->lDelta;
		DestBits += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + SourcePoint->x;
	    DestBits = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, DestRect->right - DestRect->left);
		SourceBits -= SourceSurf->lDelta;
		DestBits -= DestSurf->lDelta;
	      }
	  }
      }
      else
      {
	if (DestRect->top < SourcePoint->y)
	  {
	    SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
	    DestLine = DestBits;
	    for (j = DestRect->top; j < DestRect->bottom; j++)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=DestRect->left; i<DestRect->right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(ColorTranslation, *SourceBits++);
		  }
		SourceLine += SourceSurf->lDelta;
		DestLine += DestSurf->lDelta;
	      }
	  }
	else
	  {
	    SourceLine = SourceSurf->pvScan0 + ((SourcePoint->y + DestRect->bottom - DestRect->top - 1) * SourceSurf->lDelta) + SourcePoint->x;
	    DestLine = DestSurf->pvScan0 + ((DestRect->bottom - 1) * DestSurf->lDelta) + DestRect->left;
	    for (j = DestRect->bottom - 1; DestRect->top <= j; j--)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
		for (i=DestRect->left; i<DestRect->right; i++)
		  {
		    *DestBits++ = XLATEOBJ_iXlate(ColorTranslation, *SourceBits++);
		  }
		SourceLine -= SourceSurf->lDelta;
		DestLine -= DestSurf->lDelta;
	      }
	  }
      }
      break;

    case BMF_16BPP:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 2;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case BMF_24BPP:
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
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 3;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case BMF_32BPP:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 4;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DPRINT1("DIB_8BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		 PRECTL  DestRect,  POINTL  *SourcePoint,
		 BRUSHOBJ *Brush,   POINTL BrushOrigin,
		 XLATEOBJ *ColorTranslation, ULONG Rop4)
{
   LONG i, j, k, sx, sy;
   ULONG Dest, Source, Pattern = 0, PatternY;
   PULONG DestBits;
   BOOL UsesSource;
   BOOL UsesPattern;
   LONG RoundedRight;
   /* Pattern brushes */
   PGDIBRUSHOBJ GdiBrush;
   HBITMAP PatternSurface = NULL;
   SURFOBJ *PatternObj;
   ULONG PatternWidth, PatternHeight;

   if (Rop4 == SRCCOPY)
   {
      return DIB_8BPP_BitBltSrcCopy(
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
         Pattern = (Brush->iSolidColor & 0xFF) |
                    ((Brush->iSolidColor & 0xFF) << 8) |
                    ((Brush->iSolidColor & 0xFF) << 16) |
                    ((Brush->iSolidColor & 0xFF) << 24);
      }
   }
   
   RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x3);
   sy = SourcePoint->y;

   for (j = DestRect->top; j < DestRect->bottom; j++)
   {
      sx = SourcePoint->x;
      DestBits = (PULONG)(DestSurf->pvScan0 + DestRect->left + j * DestSurf->lDelta);

      if(UsesPattern)
        PatternY = (j + BrushOrigin.y) % PatternHeight;

      for (i = DestRect->left; i < RoundedRight; i += 4, DestBits++)
      {
         Dest = *DestBits;

         if (UsesSource)
         {
            Source = 0;
            for (k = 0; k < 4; k++)
               Source |= (DIB_GetSource(SourceSurf, sx + (i - DestRect->left) + k, sy, ColorTranslation) << (k * 8));
         }

         if (UsesPattern)
         {
            Pattern = DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 1) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 8;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 2) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 16;
            Pattern |= (DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x + 3) % PatternWidth, PatternY) ? GdiBrush->crFore : GdiBrush->crBack) << 24;
         }
         *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
      }

      if (i < DestRect->right)
      {
         for (; i < DestRect->right; i++)
         {
            Dest = DIB_8BPP_GetPixel(DestSurf, i, j);
            
            if (UsesSource)
	    {
               Source = DIB_GetSource(SourceSurf, sx + (i - DestRect->left), sy, ColorTranslation);
            }

            if (UsesPattern)
            {
               Pattern = DIB_1BPP_GetPixel(PatternObj, (i + BrushOrigin.x) % PatternWidth,PatternY) ? GdiBrush->crFore : GdiBrush->crBack;
            }

            DIB_8BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xFFFF);
         }
      }

      sy++;
   }

   if (PatternSurface != NULL)
      BITMAPOBJ_UnlockBitmap(PatternSurface);

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

typedef unsigned char PIXEL;

/* 16-bit HiColor (565 format) */
inline PIXEL average8(PIXEL a, PIXEL b)
{
  return a; // FIXME: Depend on SetStretchMode
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
void ScaleLineAvg8(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
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
      p = average8(p, *(Source+1));
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

static BOOLEAN
FinalCopy8(PIXEL *Target, PIXEL *Source, PSPAN ClipSpans, UINT ClipSpansCount, UINT *SpanIndex,
           UINT DestY, RECTL *DestRect)
{
  LONG Left, Right;
  
  while (ClipSpans[*SpanIndex].Y < DestY
         || (ClipSpans[*SpanIndex].Y == DestY
             && ClipSpans[*SpanIndex].X + ClipSpans[*SpanIndex].Width < DestRect->left))
    {
      (*SpanIndex)++;
      if (ClipSpansCount <= *SpanIndex)
        {
          /* No more spans, everything else is clipped away, we're done */
          return FALSE;
        }
    }
  while (ClipSpans[*SpanIndex].Y == DestY)
    {
      if (ClipSpans[*SpanIndex].X < DestRect->right)
        {
          Left = max(ClipSpans[*SpanIndex].X, DestRect->left);
          Right = min(ClipSpans[*SpanIndex].X + ClipSpans[*SpanIndex].Width, DestRect->right);
          memcpy(Target + Left - DestRect->left, Source + Left - DestRect->left,
                 (Right - Left) * sizeof(PIXEL));
        }
      (*SpanIndex)++;
      if (ClipSpansCount <= *SpanIndex)
        {
          /* No more spans, everything else is clipped away, we're done */
          return FALSE;
        }
    }

  return TRUE;
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
BOOLEAN ScaleRectAvg8(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                      RECTL* DestRect, RECTL *SourceRect,
                      POINTL* MaskOrigin, POINTL BrushOrigin,
                      CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                      ULONG Mode)
{
  int NumPixels = DestRect->bottom - DestRect->top;
  int IntPart = (((SourceRect->bottom - SourceRect->top) / (DestRect->bottom - DestRect->top)) * SourceSurf->lDelta); //((SourceRect->bottom - SourceRect->top) / (DestRect->bottom - DestRect->top)) * (SourceRect->right - SourceRect->left);
  int FractPart = (SourceRect->bottom - SourceRect->top) % (DestRect->bottom - DestRect->top);
  int Mid = (DestRect->bottom - DestRect->top) >> 1;
  int E = 0;
  int skip;
  PIXEL *ScanLine, *ScanLineAhead;
  PIXEL *PrevSource = NULL;
  PIXEL *PrevSourceAhead = NULL;
  PIXEL *Target = (PIXEL *) (DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left);
  PIXEL *Source = (PIXEL *) (SourceSurf->pvScan0 + (SourceRect->top * SourceSurf->lDelta) + SourceRect->left);
  PSPAN ClipSpans;
  UINT ClipSpansCount;
  UINT SpanIndex;
  LONG DestY;

  if (! ClipobjToSpans(&ClipSpans, &ClipSpansCount, ClipRegion, DestRect))
    {
      return FALSE;
    }
  if (0 == ClipSpansCount)
    {
      /* No clip spans == empty clipping region, everything clipped away */
      ASSERT(NULL == ClipSpans);
      return TRUE;
    }
  skip = (DestRect->bottom - DestRect->top < SourceRect->bottom - SourceRect->top) ? 0 : ((DestRect->bottom - DestRect->top) / (2 * (SourceRect->bottom - SourceRect->top)) + 1);
  NumPixels -= skip;

  ScanLine = (PIXEL*)ExAllocatePool(PagedPool, (DestRect->right - DestRect->left) * sizeof(PIXEL));
  ScanLineAhead = (PIXEL *)ExAllocatePool(PagedPool, (DestRect->right - DestRect->left) * sizeof(PIXEL));

  DestY = DestRect->top;
  SpanIndex = 0;
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
        ScaleLineAvg8(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      } /* if */
      PrevSource = Source;
    } /* if */
    
    if (E >= Mid && PrevSourceAhead != (PIXEL *)((BYTE *)Source + SourceSurf->lDelta)) {
      int x;
      ScaleLineAvg8(ScanLineAhead, (PIXEL *)((BYTE *)Source + SourceSurf->lDelta), SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      for (x = 0; x < DestRect->right - DestRect->left; x++)
        ScanLine[x] = average8(ScanLine[x], ScanLineAhead[x]);
      PrevSourceAhead = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
    } /* if */
    
    if (! FinalCopy8(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
      {
        /* No more spans, everything else is clipped away, we're done */
        ExFreePool(ClipSpans);
        ExFreePool(ScanLine);
        ExFreePool(ScanLineAhead);
        return TRUE;
      }
    DestY++;
    Target = (PIXEL *)((BYTE *)Target + DestSurf->lDelta);
    Source += IntPart;
    E += FractPart;
    if (E >= DestRect->bottom - DestRect->top) {
      E -= DestRect->bottom - DestRect->top;
      Source = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
    } /* if */
  } /* while */

  if (skip > 0 && Source != PrevSource)
    ScaleLineAvg8(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
  while (skip-- > 0) {
    if (! FinalCopy8(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
      {
        /* No more spans, everything else is clipped away, we're done */
        ExFreePool(ClipSpans);
        ExFreePool(ScanLine);
        ExFreePool(ScanLineAhead);
        return TRUE;
      }
    DestY++;
    Target = (PIXEL *)((BYTE *)Target + DestSurf->lDelta);
  } /* while */

  ExFreePool(ClipSpans);
  ExFreePool(ScanLine);
  ExFreePool(ScanLineAhead);

  return TRUE;
}

BOOLEAN DIB_8BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode)
{
  DPRINT("DIB_8BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
     BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
     DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    switch(SourceSurf->iBitmapFormat)
    {
      case BMF_1BPP:
      case BMF_4BPP:
      case BMF_16BPP:
      case BMF_24BPP:
      case BMF_32BPP:
         /* Not implemented yet. */
         return FALSE;      
      break;
  
      case BMF_8BPP:
        return ScaleRectAvg8(DestSurf, SourceSurf, DestRect, SourceRect, MaskOrigin, BrushOrigin,
                             ClipRegion, ColorTranslation, Mode);
      break;
      
      default:
         DPRINT1("DIB_8BPP_StretchBlt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
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
  DestBits = (ULONG*)(DestSurf->pvScan0 + DestRect->left +
                      (DestRect->top * DestSurf->lDelta));
  wd = DestSurf->lDelta - (DestRect->right - DestRect->left);
  
  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    DestBits = (ULONG*)(DestSurf->pvScan0 + DestRect->left +
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

/* EOF */
