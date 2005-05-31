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
  memset(SurfObj->pvScan0 + y * SurfObj->lDelta + x1, (BYTE) c, x2 - x1);
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
DIB_8BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;

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
      SourceBits_4BPP = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + (BltInfo->SourcePoint.x >> 1);

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
	    SourceBits = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, BltInfo->DestRect.right - BltInfo->DestRect.left);
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    DestBits = BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
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
	    SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
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
	    SourceLine = BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
	    DestLine = BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
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
      SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
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
      SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
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
      SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
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

BOOLEAN
DIB_8BPP_BitBlt(PBLTINFO BltInfo)
{
   ULONG DestX, DestY;
   ULONG SourceX, SourceY;
   ULONG PatternY = 0;
   ULONG Dest, Source = 0, Pattern = 0;
   BOOL UsesSource;
   BOOL UsesPattern;
   PULONG DestBits;
   LONG RoundedRight;

   UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
   UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

   SourceY = BltInfo->SourcePoint.y;
   RoundedRight = BltInfo->DestRect.right -
                  ((BltInfo->DestRect.right - BltInfo->DestRect.left) & 0x7);

   if (UsesPattern)
   {
      if (BltInfo->PatternSurface)
      {
         PatternY = (BltInfo->DestRect.top + BltInfo->BrushOrigin.y) %
                    BltInfo->PatternSurface->sizlBitmap.cy;
      }
      else
      {
         Pattern = BltInfo->Brush->iSolidColor |
                   (BltInfo->Brush->iSolidColor << 8) |
                   (BltInfo->Brush->iSolidColor << 16) |
                   (BltInfo->Brush->iSolidColor << 24);
      }
   }

   for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
   {
      SourceX = BltInfo->SourcePoint.x;
      DestBits = (PULONG)(
         BltInfo->DestSurface->pvScan0 +
         BltInfo->DestRect.left +
         DestY * BltInfo->DestSurface->lDelta);

      for (DestX = BltInfo->DestRect.left; DestX < RoundedRight; DestX += 4, DestBits++)
      {
         Dest = *DestBits;

         if (UsesSource)
         {
            Source = DIB_GetSource(BltInfo->SourceSurface, SourceX + (DestX - BltInfo->DestRect.left), SourceY, BltInfo->XlateSourceToDest);
            Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + (DestX - BltInfo->DestRect.left) + 1, SourceY, BltInfo->XlateSourceToDest) << 8;
            Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + (DestX - BltInfo->DestRect.left) + 2, SourceY, BltInfo->XlateSourceToDest) << 16;
            Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + (DestX - BltInfo->DestRect.left) + 3, SourceY, BltInfo->XlateSourceToDest) << 24;
         }

         if (BltInfo->PatternSurface)
         {
            Pattern = DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest);
            Pattern |= DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 1) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest) << 8;
            Pattern |= DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 2) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest) << 16;
            Pattern |= DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 3) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest) << 24;
         }

         *DestBits = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern);
      }

      if (DestX < BltInfo->DestRect.right)
      {
         for (; DestX < BltInfo->DestRect.right; DestX++)
         {
            Dest = DIB_8BPP_GetPixel(BltInfo->DestSurface, DestX, DestY);

            if (UsesSource)
	    {
               Source = DIB_GetSource(BltInfo->SourceSurface, SourceX + (DestX - BltInfo->DestRect.left), SourceY, BltInfo->XlateSourceToDest);
            }

            if (BltInfo->PatternSurface)
            {
               Pattern = DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest);
            }

            DIB_8BPP_PutPixel(BltInfo->DestSurface, DestX, DestY, DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xFFFF);
         }
      }

      SourceY++;
      if (BltInfo->PatternSurface)
      {
         PatternY++;
         PatternY %= BltInfo->PatternSurface->sizlBitmap.cy;
      }
   }

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

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
BOOLEAN DIB_8BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode)
{
   int SrcSizeY;
   int SrcSizeX;
   int DesSizeY;
   int DesSizeX;      
   int sx;
   int sy;
   int DesX;
   int DesY;
   int color;

  DPRINT("DIB_8BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
     BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
     DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    switch(SourceSurf->iBitmapFormat)
    {     
	        case BMF_1BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

 		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{				
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY);                 				

				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

				    if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
					{
					 DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, 0));
					 } else {
                     DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, 1));
                     }					
					}
				}
      break;

      case BMF_4BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{				
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				

				if (sy > SourceRect->bottom) break;


				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_4BPP_GetPixel(SourceSurf, sx, sy);
					DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}
      break;

      case BMF_16BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_16BPP_GetPixel(SourceSurf, sx, sy);
					DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}
	  break;

      case BMF_24BPP:
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_24BPP_GetPixel(SourceSurf, sx, sy);
					DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}		  
      break;

      case BMF_32BPP:
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_32BPP_GetPixel(SourceSurf, sx, sy);
					DIB_8BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}		  
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
