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
DIB_32BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  *addr = c;
}

ULONG
DIB_32BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  return (ULONG)(*addr);
}


VOID
DIB_32BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{


  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta >> 2; // >> 2 == / sizeof(DWORD)

  byteaddr = (PBYTE)addr;
  while(y1++ < y2)
  {
    *addr = (DWORD)c;
    addr += lDelta;
  }

}

BOOLEAN
DIB_32BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PDWORD   Source32, Dest32;

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + 4 * BltInfo->DestRect.left;

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
            DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          } else {
            DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
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
          DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += BltInfo->SourceSurface->lDelta;
      }
      break;

    case BMF_8BPP:
      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *SourceBits;
          *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 1;
	  DestBits += 4;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
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
          *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 2;
	  DestBits += 4;
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
          *((PDWORD)DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 3;
	  DestBits += 4;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_32BPP:
      if (NULL == BltInfo->XlateSourceToDest || 0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
	    DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 4 * BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits -= BltInfo->SourceSurface->lDelta;
		DestBits -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      else
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceBits = ((PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x);
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
                if (BltInfo->DestRect.left < BltInfo->SourcePoint.x)
                  {
                    Dest32 = (DWORD *) DestBits;
                    Source32 = (DWORD *) SourceBits;
                    for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
                      {
                        *Dest32++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32++);
                      }
                  }
                else
                  {
                    Dest32 = (DWORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
                    Source32 = (DWORD *) SourceBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
                    for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
                      {
                        *Dest32-- = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32--);
                      }
                  }
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
	    DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 4 * BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
                if (BltInfo->DestRect.left < BltInfo->SourcePoint.x)
                  {
                    Dest32 = (DWORD *) DestBits;
                    Source32 = (DWORD *) SourceBits;
                    for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
                      {
                        *Dest32++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32++);
                      }
                  }
                else
                  {
                    Dest32 = (DWORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
                    Source32 = (DWORD *) SourceBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
                    for (i = BltInfo->DestRect.right; BltInfo->DestRect.left <= i; i--)
                      {
                        *Dest32-- = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32--);
                      }
                  }
		SourceBits -= BltInfo->SourceSurface->lDelta;
		DestBits -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      break;

    default:
      DPRINT1("DIB_32BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
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

typedef unsigned long PIXEL;

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!

/* 32-bit Color (___ format) */
__inline PIXEL average32(PIXEL a, PIXEL b)
{
  return a; // FIXME: Temp hack to remove "PCB-effect" from the image
}

void ScaleLineAvg32(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
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
      p = average32(p, *(Source+1));
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
FinalCopy32(PIXEL *Target, PIXEL *Source, PSPAN ClipSpans, UINT ClipSpansCount, UINT *SpanIndex,
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
BOOLEAN ScaleRectAvg32(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                       RECTL* DestRect, RECTL *SourceRect,
                       POINTL* MaskOrigin, POINTL BrushOrigin,
                       CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                       ULONG Mode)
{
  int NumPixels = DestRect->bottom - DestRect->top;
  int IntPart = (((SourceRect->bottom - SourceRect->top) / (DestRect->bottom - DestRect->top)) * SourceSurf->lDelta) / 4;
  int FractPart = (SourceRect->bottom - SourceRect->top) % (DestRect->bottom - DestRect->top);
  int Mid = (DestRect->bottom - DestRect->top) >> 1;
  int E = 0;
  int skip;
  PIXEL *ScanLine, *ScanLineAhead;
  PIXEL *PrevSource = NULL;
  PIXEL *PrevSourceAhead = NULL;
  PIXEL *Target = (PIXEL *) ((PBYTE)DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + 4 * DestRect->left);
  PIXEL *Source = (PIXEL *) ((PBYTE)SourceSurf->pvScan0 + (SourceRect->top * SourceSurf->lDelta) + 4 * SourceRect->left);
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

  if (!ScanLine || !ScanLineAhead)
    {
      if (ScanLine) ExFreePool(ScanLine);
      if (ScanLineAhead) ExFreePool(ScanLineAhead);
      return FALSE;
    }

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
        ScaleLineAvg32(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      } /* if */
      PrevSource = Source;
    } /* if */

    if (E >= Mid && PrevSourceAhead != (PIXEL *)((BYTE *)Source + SourceSurf->lDelta)) {
      int x;
      ScaleLineAvg32(ScanLineAhead, (PIXEL *)((BYTE *)Source + SourceSurf->lDelta), SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      for (x = 0; x < DestRect->right - DestRect->left; x++)
        ScanLine[x] = average32(ScanLine[x], ScanLineAhead[x]);
      PrevSourceAhead = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
    } /* if */

    if (! FinalCopy32(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
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
    ScaleLineAvg32(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
  while (skip-- > 0) {
    if (! FinalCopy32(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
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
BOOLEAN DIB_32BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode)
{

   LONG SrcSizeY;
   LONG SrcSizeX;
   LONG DesSizeY;
   LONG DesSizeX;
   LONG sx = 0;
   LONG sy = 0;
   LONG DesX;
   LONG DesY;
   PULONG DestBits;
   LONG DifflDelta;

   LONG SrcZoomXHight;
   LONG SrcZoomXLow;
   LONG SrcZoomYHight;
   LONG SrcZoomYLow;

   LONG sy_dec = 0;
   LONG sy_max;

   LONG sx_dec = 0;
   LONG sx_max;


   DPRINT("DIB_32BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
            BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right,
            SourceRect->bottom, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    /* Calc the Zoom height of Source */
    SrcSizeY = SourceRect->bottom - SourceRect->top;

    /* Calc the Zoom Width of Source */
    SrcSizeX = SourceRect->right - SourceRect->left;

    /* Calc the Zoom height of Destinations */
    DesSizeY = DestRect->bottom - DestRect->top;

    /* Calc the Zoom width of Destinations */
    DesSizeX = DestRect->right - DestRect->left;

    /* Calc the zoom factor of source height */
    SrcZoomYHight = SrcSizeY / DesSizeY;
    SrcZoomYLow = SrcSizeY - (SrcZoomYHight * DesSizeY);

    /* Calc the zoom factor of source width */
    SrcZoomXHight = SrcSizeX / DesSizeX;
    SrcZoomXLow = SrcSizeX - (SrcZoomXHight * DesSizeX);

    sx_max = DesSizeX;
    sy_max = DesSizeY;
    sy = SourceRect->top;

    DestBits = (PULONG)((PBYTE)DestSurf->pvScan0 + (DestRect->left << 2) +
                               DestRect->top * DestSurf->lDelta);

    DifflDelta = DestSurf->lDelta -  (DesSizeX << 2);

    switch(SourceSurf->iBitmapFormat)
    {
      case BMF_1BPP:
       /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
       /* This is a reference implementation, it hasn't been optimized for speed */
       for (DesY=0; DesY<DesSizeY; DesY++)
       {
            sx = SourceRect->left;
            sx_dec = 0;
            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                            DIB_1BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 4);

                sx += SrcZoomXHight;
                sx_dec += SrcZoomXLow;
                if (sx_dec >= sx_max)
                {
                    sx++;
                    sx_dec -= sx_max;
                }
            }

            DestBits = (PULONG)((ULONG_PTR)DestBits + DifflDelta);

            sy += SrcZoomYHight;
            sy_dec += SrcZoomYLow;
            if (sy_dec >= sy_max)
            {
                sy++;
                sy_dec -= sy_max;
            }
       }
       break;

      case BMF_4BPP:
      /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
      /* This is a reference implementation, it hasn't been optimized for speed */

       for (DesY=0; DesY<DesSizeY; DesY++)
       {
            sx = SourceRect->left;
            sx_dec = 0;
            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                            DIB_4BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 4);

                sx += SrcZoomXHight;
                sx_dec += SrcZoomXLow;
                if (sx_dec >= sx_max)
                {
                    sx++;
                    sx_dec -= sx_max;
                }
            }

            DestBits = (PULONG)((ULONG_PTR)DestBits + DifflDelta);

            sy += SrcZoomYHight;
            sy_dec += SrcZoomYLow;
            if (sy_dec >= sy_max)
            {
                sy++;
                sy_dec -= sy_max;
            }
       }
      break;

      case BMF_8BPP:
      /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
      /* This is a reference implementation, it hasn't been optimized for speed */

       for (DesY=0; DesY<DesSizeY; DesY++)
       {
            sx = SourceRect->left;
            sx_dec = 0;
            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                            DIB_8BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 4);

                sx += SrcZoomXHight;
                sx_dec += SrcZoomXLow;
                if (sx_dec >= sx_max)
                {
                    sx++;
                    sx_dec -= sx_max;
                }
            }

            DestBits = (PULONG)((ULONG_PTR)DestBits + DifflDelta);

            sy += SrcZoomYHight;
            sy_dec += SrcZoomYLow;
            if (sy_dec >= sy_max)
            {
                sy++;
                sy_dec -= sy_max;
            }
       }
      break;

      case BMF_16BPP:
      /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
      /* This is a reference implementation, it hasn't been optimized for speed */

       for (DesY=0; DesY<DesSizeY; DesY++)
       {
            sx = SourceRect->left;
            sx_dec = 0;
            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                            DIB_16BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 4);

                sx += SrcZoomXHight;
                sx_dec += SrcZoomXLow;
                if (sx_dec >= sx_max)
                {
                    sx++;
                    sx_dec -= sx_max;
                }
            }

            DestBits = (PULONG)((ULONG_PTR)DestBits + DifflDelta);

            sy += SrcZoomYHight;
            sy_dec += SrcZoomYLow;
            if (sy_dec >= sy_max)
            {
                sy++;
                sy_dec -= sy_max;
            }
       }
       break;

      case BMF_24BPP:
      /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
      /* This is a reference implementation, it hasn't been optimized for speed */

       for (DesY=0; DesY<DesSizeY; DesY++)
       {
            sx = SourceRect->left;
            sx_dec = 0;
            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                            DIB_24BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 4);

                sx += SrcZoomXHight;
                sx_dec += SrcZoomXLow;
                if (sx_dec >= sx_max)
                {
                    sx++;
                    sx_dec -= sx_max;
                }
            }

            DestBits = (PULONG)((ULONG_PTR)DestBits + DifflDelta);

            sy += SrcZoomYHight;
            sy_dec += SrcZoomYLow;
            if (sy_dec >= sy_max)
            {
                sy++;
                sy_dec -= sy_max;
            }
       }
       break;

      case BMF_32BPP:
        return ScaleRectAvg32(DestSurf, SourceSurf, DestRect, SourceRect, MaskOrigin, BrushOrigin,
                              ClipRegion, ColorTranslation, Mode);
      break;

      default:
        DPRINT1("DIB_32BPP_StretchBlt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
    }



  return TRUE;
}

BOOLEAN
DIB_32BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  ULONG X, Y, SourceX, SourceY, Source, wd;
  ULONG *DestBits;

  SourceY = SourcePoint->y;
  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 +
                      (DestRect->left << 2) +
                      DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) << 2);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceX = SourcePoint->x;
    for(X = DestRect->left; X < DestRect->right; X++, DestBits++, SourceX++)
    {
      Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
      if(Source != iTransColor)
      {
        *DestBits = XLATEOBJ_iXlate(ColorTranslation, Source);
      }
    }

    SourceY++;
    DestBits = (ULONG*)((ULONG_PTR)DestBits + wd);
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

static __inline UCHAR
Clamp8(ULONG val)
{
   return (val > 255) ? 255 : val;
}

BOOLEAN
DIB_32BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                     RECTL* SourceRect, CLIPOBJ* ClipRegion,
                     XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
   INT Rows, Cols, SrcX, SrcY;
   register PULONG Dst;
   ULONG DstDelta;
   BLENDFUNCTION BlendFunc;
   register NICEPIXEL32 DstPixel, SrcPixel;
   UCHAR Alpha, SrcBpp;

   DPRINT("DIB_32BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
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

   Dst = (PULONG)((ULONG_PTR)Dest->pvScan0 + (DestRect->top * Dest->lDelta) +
                             (DestRect->left << 2));
   DstDelta = Dest->lDelta - ((DestRect->right - DestRect->left) << 2);
   SrcBpp = BitsPerFormat(Source->iBitmapFormat);

   Rows = DestRect->bottom - DestRect->top;
   SrcY = SourceRect->top;
   while (--Rows >= 0)
   {
      Cols = DestRect->right - DestRect->left;
      SrcX = SourceRect->left;
      while (--Cols >= 0)
      {
         SrcPixel.ul = DIB_GetSource(Source, SrcX++, SrcY, ColorTranslation);
         SrcPixel.col.red = SrcPixel.col.red * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.green = SrcPixel.col.green * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.blue = SrcPixel.col.blue * BlendFunc.SourceConstantAlpha / 255;
         SrcPixel.col.alpha = (SrcBpp == 32) ? (SrcPixel.col.alpha * BlendFunc.SourceConstantAlpha / 255) : BlendFunc.SourceConstantAlpha;

         Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
                 SrcPixel.col.alpha : BlendFunc.SourceConstantAlpha;

         DstPixel.ul = *Dst;
         DstPixel.col.red = Clamp8(DstPixel.col.red * (255 - Alpha) / 255 + SrcPixel.col.red);
         DstPixel.col.green = Clamp8(DstPixel.col.green * (255 - Alpha) / 255 + SrcPixel.col.green);
         DstPixel.col.blue = Clamp8(DstPixel.col.blue * (255 - Alpha) / 255 + SrcPixel.col.blue);
         DstPixel.col.alpha = Clamp8(DstPixel.col.alpha * (255 - Alpha) / 255 + SrcPixel.col.alpha);
         *Dst++ = DstPixel.ul;
      }
      Dst = (PULONG)((ULONG_PTR)Dst + DstDelta);
      SrcY++;
   }

   return TRUE;
}

/* EOF */
