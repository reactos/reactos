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
DIB_16BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;

  *addr = (WORD)c;
}

ULONG
DIB_16BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;

  return (ULONG)(*addr);
}

VOID
DIB_16BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PDWORD addr = (PDWORD)((PWORD)((PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta) + x1);

 
#ifdef _M_IX86
  /* This is about 10% faster than the generic C code below */
  LONG Count = x2 - x1;

  __asm__ __volatile__ (
"  cld\n"
"  mov  %0, %%eax\n"
"  shl  $16, %%eax\n"
"  andl $0xffff, %0\n"  /* If the pixel value is "abcd", put "abcdabcd" in %eax */
"  or   %0, %%eax\n"
"  mov  %2, %%edi\n"
"  test $0x03, %%edi\n" /* Align to fullword boundary */
"  jz   0f\n"
"  stosw\n"
"  dec  %1\n"
"  jz   1f\n"
"0:\n"
"  mov  %1,%%ecx\n"     /* Setup count of fullwords to fill */
"  shr  $1,%%ecx\n"
"  rep stosl\n"         /* The actual fill */
"  test $0x01, %1\n"    /* One left to do at the right side? */
"  jz   1f\n"
"  stosw\n"
"1:\n"
  : /* no output */
  : "r"(c), "r"(Count), "m"(addr)
  : "%eax", "%ecx", "%edi");
#else /* _M_IX86 */
  LONG cx = x1;
  DWORD cc;

  if (0 != (cx & 0x01)) {
    *((PWORD) addr) = c;
    cx++;
    addr = (PDWORD)((PWORD)(addr) + 1);
  }
  cc = ((c & 0xffff) << 16) | (c & 0xffff);
  while(cx + 1 < x2) {
    *addr++ = cc;
    cx += 2;
  }
  if (cx < x2) {
    *((PWORD) addr) = c;
  }
#endif /* _M_IX86 */
}


VOID
DIB_16BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
#ifdef _M_IX86
  asm volatile(
    "   testl %2, %2"       "\n\t"
    "   jle   2f"           "\n\t"
    "   movl  %2, %%ecx"    "\n\t"
    "   shrl  $2, %2"       "\n\t"
    "   andl  $3, %%ecx"    "\n\t"
    "   jz    1f"           "\n\t"
    "0:"                    "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   decl  %%ecx"        "\n\t"
    "   jnz   0b"           "\n\t"
    "   testl %2, %2"       "\n\t"
    "   jz    2f"           "\n\t"
    "1:"                    "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   decl  %2"           "\n\t"
    "   jnz   1b"           "\n\t"
    "2:"                    "\n\t"
    : /* no output */
    : "r"((PBYTE)SurfObj->pvScan0 + (y1 * SurfObj->lDelta) + (x * sizeof (WORD))),
      "r"(SurfObj->lDelta), "r"(y2 - y1), "a"(c)
    : "cc", "memory", "%ecx");
#else
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ < y2) {
    *addr = (WORD)c;

    byteaddr += lDelta;
    addr = (PWORD)byteaddr;
  }
#endif
}

BOOLEAN
DIB_16BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + 2 * BltInfo->DestRect.left;

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
            DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          } else {
            DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
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
          DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
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
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits);
          SourceBits += 1;
	  DestBits += 2;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_16BPP:
      if (NULL == BltInfo->XlateSourceToDest || 0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 2 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
	    DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 2 * BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 2 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits -= BltInfo->SourceSurface->lDelta;
		DestBits -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      else
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
	    DestLine = DestBits;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
	        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
		  {
		    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *((WORD *)SourceBits));
		    SourceBits += 2;
		    DestBits += 2;
	          }
		SourceLine += BltInfo->SourceSurface->lDelta;
		DestLine += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
	    DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 2 * BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		SourceBits = SourceLine;
		DestBits = DestLine;
	        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
		  {
		    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *((WORD *)SourceBits));
		    SourceBits += 2;
		    DestBits += 2;
	          }
		SourceLine -= BltInfo->SourceSurface->lDelta;
		DestLine -= BltInfo->DestSurface->lDelta;
	      }
	  }
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
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          SourceBits += 3;
	  DestBits += 2;
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
          *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *((PDWORD) SourceBits));
          SourceBits += 4;
	  DestBits += 2;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DPRINT1("DIB_16BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_16BPP_BitBlt(PBLTINFO BltInfo)
{
   ULONG DestX, DestY;
   ULONG SourceX, SourceY;
   ULONG PatternY = 0;
   ULONG Dest, Source = 0, Pattern = 0;
   BOOL UsesSource;
   BOOL UsesPattern;
   PULONG DestBits;
   ULONG RoundedRight;

   UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
   UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

   RoundedRight = BltInfo->DestRect.right -
                  ((BltInfo->DestRect.right - BltInfo->DestRect.left) & 0x1);
   SourceY = BltInfo->SourcePoint.y;
   DestBits = (PULONG)(
      (PBYTE)BltInfo->DestSurface->pvScan0 +
      (BltInfo->DestRect.left << 1) +
      BltInfo->DestRect.top * BltInfo->DestSurface->lDelta);

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
                   (BltInfo->Brush->iSolidColor << 16);
      }
   }

   for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
   {
      SourceX = BltInfo->SourcePoint.x;

      for (DestX = BltInfo->DestRect.left; DestX < RoundedRight; DestX += 2, DestBits++, SourceX += 2)
      {
         Dest = *DestBits;

         if (UsesSource)
         {
            Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
            Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + 1, SourceY, BltInfo->XlateSourceToDest) << 16;
         }

         if (BltInfo->PatternSurface)
	 {
            Pattern = DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest);
            Pattern |= DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 1) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest) << 16;
         }

         *DestBits = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern);
      }

      if (DestX < BltInfo->DestRect.right)
      {
         Dest = *((PUSHORT)DestBits);

         if (UsesSource)
         {
            Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
         }

         if (BltInfo->PatternSurface)
         {
            Pattern = DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest);
         }

         DIB_16BPP_PutPixel(BltInfo->DestSurface, DestX, DestY, DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xFFFF);
         DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
      }

      SourceY++;
      if (BltInfo->PatternSurface)
      {
         PatternY++;
         PatternY %= BltInfo->PatternSurface->sizlBitmap.cy;
      }
      DestBits = (PULONG)(
         (ULONG_PTR)DestBits -
         ((BltInfo->DestRect.right - BltInfo->DestRect.left) << 1) +
         BltInfo->DestSurface->lDelta);
   }

   return TRUE;
}

/* Optimize for bitBlt */
BOOLEAN
DIB_16BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  ULONG DestY;	

#ifdef _M_IX86
  /* This is about 10% faster than the generic C code below */ 
  ULONG delta = DestSurface->lDelta;
  ULONG width = (DestRect->right - DestRect->left) ;
  PULONG pos =  (PULONG) ((PBYTE)DestSurface->pvScan0 + DestRect->top * delta + (DestRect->left<<1));
  color = (color&0xffff);  /* If the color value is "abcd", put "abcdabcd" into color */
  color += (color<<16);
  
  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {   
  __asm__ __volatile__ (
    "  cld\n"
    "  mov  %1,%%ebx\n" 
    "  mov  %2,%%edi\n" 
    "  test $0x03, %%edi\n" /* Align to fullword boundary */
    "  jz   .FL1\n"
    "  stosw\n"
    "  dec  %%ebx\n"
    "  jz   .FL2\n"
    ".FL1:\n"
    "  mov  %%ebx,%%ecx\n"     /* Setup count of fullwords to fill */
    "  shr  $1,%%ecx\n"
    "  rep stosl\n"         /* The actual fill */
    "  test $0x01, %%ebx\n"    /* One left to do at the right side? */
    "  jz   .FL2\n"
    "  stosw\n"
    ".FL2:\n"
    :
    : "a" (color), "r" (width), "m" (pos)
    : "%ecx", "%ebx", "%edi");
     pos =(PULONG)((ULONG_PTR)pos + delta);	 
  }

#else /* _M_IX86 */

	for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_16BPP_HLine (DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
#endif
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

static BOOLEAN
FinalCopy16(PIXEL *Target, PIXEL *Source, PSPAN ClipSpans, UINT ClipSpansCount, UINT *SpanIndex,
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
BOOLEAN ScaleRectAvg16(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                       RECTL* DestRect, RECTL *SourceRect,
                       POINTL* MaskOrigin, POINTL BrushOrigin,
                       CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                       ULONG Mode)
{
  int NumPixels = DestRect->bottom - DestRect->top;
  int IntPart = (((SourceRect->bottom - SourceRect->top) / (DestRect->bottom - DestRect->top)) * SourceSurf->lDelta) >> 1;
  int FractPart = (SourceRect->bottom - SourceRect->top) % (DestRect->bottom - DestRect->top);
  int Mid = (DestRect->bottom - DestRect->top) >> 1;
  int E = 0;
  int skip;
  PIXEL *ScanLine, *ScanLineAhead;
  PIXEL *PrevSource = NULL;
  PIXEL *PrevSourceAhead = NULL;
  PIXEL *Target = (PIXEL *) ((PBYTE)DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + 2 * DestRect->left);
  PIXEL *Source = (PIXEL *) ((PBYTE)SourceSurf->pvScan0 + (SourceRect->top * SourceSurf->lDelta) + 2 * SourceRect->left);
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
        ScaleLineAvg16(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      } /* if */
      PrevSource = Source;
    } /* if */

    if (E >= Mid && PrevSourceAhead != (PIXEL *)((BYTE *)Source + SourceSurf->lDelta)) {
      int x;
      ScaleLineAvg16(ScanLineAhead, (PIXEL *)((BYTE *)Source + SourceSurf->lDelta), SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
      for (x = 0; x < DestRect->right - DestRect->left; x++)
        ScanLine[x] = average16(ScanLine[x], ScanLineAhead[x]);
      PrevSourceAhead = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
    } /* if */

    if (! FinalCopy16(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
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
    ScaleLineAvg16(ScanLine, Source, SourceRect->right - SourceRect->left, DestRect->right - DestRect->left);
  while (skip-- > 0) {
    if (! FinalCopy16(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex, DestY, DestRect))
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
BOOLEAN DIB_16BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
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
   int zoomX;
   int zoomY;
   int count;
   int saveX;
   int saveY;
   BOOLEAN DesIsBiggerY=FALSE;

  DPRINT("DIB_16BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
     BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
     DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    SrcSizeY = SourceRect->bottom;
    SrcSizeX = SourceRect->right;
  
    DesSizeY = DestRect->bottom;
    DesSizeX = DestRect->right;   
   
	zoomX = DesSizeX / SrcSizeX;
    if (zoomX==0) zoomX=1;

	zoomY = DesSizeY / SrcSizeY;
    if (zoomY==0) zoomY=1;

	if (DesSizeY>SrcSizeY)
      DesIsBiggerY = TRUE;

      

    switch(SourceSurf->iBitmapFormat)
    {
      

      case BMF_1BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */
           if (zoomX>1)
		   {
		     /* Draw one Hline on X - Led to the Des Zoom In*/
		     if (DesSizeX>SrcSizeX)
			 {
		  		for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						saveX = DesX + zoomX;

						if (DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0) 
						   for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, 0);
						else
							for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, 1);
					                    																	
					  }										
				  }
			    }
			 else
			 {
			   /* Draw one Hline on X - Led to the Des Zoom Out*/

               for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						saveX = DesX + zoomX;

						if (DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0) 
						   for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, 0);
						else
							for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, 1);
					                    																	
					  }										
				  }
			 }
		   }			
		   else
		   {
		    
		    if (DesSizeX>SrcSizeX)
			{
				/* Draw one pixel on X - Led to the Des Zoom In*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						if (DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0) 
						   for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, 0);										
						else
							for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, 1);										
					                    
						
					 }
		          }
			 }
			else
			{
				/* Draw one pixel on X - Led to the Des Zoom Out*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						if (DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0) 
						   for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, 0);										
						else
							for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, 1);										
					                    						
					 }
		          }
			}
		   }
		break;

      case BMF_4BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */
           if (zoomX>1)
		   {
		     /* Draw one Hline on X - Led to the Des Zoom In*/
		     if (DesSizeX>SrcSizeX)
			 {
		  		for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			    }
			 else
			 {
			   /* Draw one Hline on X - Led to the Des Zoom Out*/

               for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			 }
		   }
			
		   else
		   {
		    
		    if (DesSizeX>SrcSizeX)
			{
				/* Draw one pixel on X - Led to the Des Zoom In*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			 }
			else
			{
				/* Draw one pixel on X - Led to the Des Zoom Out*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			}
		   }
		break;

      case BMF_8BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */
           if (zoomX>1)
		   {
		     /* Draw one Hline on X - Led to the Des Zoom In*/
		     if (DesSizeX>SrcSizeX)
			 {
		  		for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			    }
			 else
			 {
			   /* Draw one Hline on X - Led to the Des Zoom Out*/

               for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			 }
		   }
			
		   else
		   {
		    
		    if (DesSizeX>SrcSizeX)
			{
				/* Draw one pixel on X - Led to the Des Zoom In*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			 }
			else
			{
				/* Draw one pixel on X - Led to the Des Zoom Out*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			}
		   }
		break;

      case BMF_16BPP:
        return ScaleRectAvg16(DestSurf, SourceSurf, DestRect, SourceRect, MaskOrigin, BrushOrigin,
                              ClipRegion, ColorTranslation, Mode);
      break;

      case BMF_24BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */
           if (zoomX>1)
		   {
		     /* Draw one Hline on X - Led to the Des Zoom In*/
		     if (DesSizeX>SrcSizeX)
			 {
		  		for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			    }
			 else
			 {
			   /* Draw one Hline on X - Led to the Des Zoom Out*/

               for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			 }
		   }
			
		   else
		   {
		    
		    if (DesSizeX>SrcSizeX)
			{
				/* Draw one pixel on X - Led to the Des Zoom In*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			 }
			else
			{
				/* Draw one pixel on X - Led to the Des Zoom Out*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			}
		   }
		break;

      case BMF_32BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */
           if (zoomX>1)
		   {
		     /* Draw one Hline on X - Led to the Des Zoom In*/
		     if (DesSizeX>SrcSizeX)
			 {
		  		for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			    }
			 else
			 {
			   /* Draw one Hline on X - Led to the Des Zoom Out*/

               for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy));
					                    					
						saveX = DesX + zoomX;
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_HLine(DestSurf, DesX, saveX, count, color);
					  }										
				  }
			 }
		   }
			
		   else
		   {
		    
		    if (DesSizeX>SrcSizeX)
			{
				/* Draw one pixel on X - Led to the Des Zoom In*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{						
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
						
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			 }
			else
			{
				/* Draw one pixel on X - Led to the Des Zoom Out*/
				for (DesY=DestRect->bottom-zoomY; DesY>=0; DesY-=zoomY)
				{
					if (DesIsBiggerY)
						sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
					else
						sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
					if (sy > SourceRect->bottom) break;

					saveY = DesY+zoomY;

					for (DesX=DestRect->right-zoomX; DesX>=0; DesX-=zoomX)				
					{
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
						if (sx > SourceRect->right) break;
					
						color =  XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy));
					                    
						for (count=DesY;count<saveY;count++)
							DIB_16BPP_PutPixel(DestSurf, DesX, count, color);										
					 }
		          }
			}
		   }
		break;

      break;

      default:
         DPRINT1("DIB_16BPP_StretchBlt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
    }



  return TRUE;
}

BOOLEAN
DIB_16BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  ULONG RoundedRight, X, Y, SourceX, SourceY, Source, wd, Dest;
  ULONG *DestBits;

  RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x1);
  SourceY = SourcePoint->y;
  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 +
                      (DestRect->left << 1) +
                      DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) << 1);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceX = SourcePoint->x;
    for(X = DestRect->left; X < RoundedRight; X += 2, DestBits++, SourceX += 2)
    {
      Dest = *DestBits;

      Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFF0000;
        Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFFFF);
      }

      Source = DIB_GetSourceIndex(SourceSurf, SourceX + 1, SourceY);
      if(Source != iTransColor)
      {
        Dest &= 0xFFFF;
        Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) << 16);
      }

      *DestBits = Dest;
    }

    if(X < DestRect->right)
    {
      Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
      if(Source != iTransColor)
      {
        *((USHORT*)DestBits) = (USHORT)XLATEOBJ_iXlate(ColorTranslation, Source);
      }

      DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
    }
    SourceY++;
    DestBits = (ULONG*)((ULONG_PTR)DestBits + wd);
  }

  return TRUE;
}

/* EOF */
