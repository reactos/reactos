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

#if defined(_M_IX86) && !defined(_MSC_VER)
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

    if (0 != (cx & 0x01))
    {
        *((PWORD) addr) = c;
        cx++;
        addr = (PDWORD)((PWORD)(addr) + 1);
    }
    cc = ((c & 0xffff) << 16) | (c & 0xffff);
    while(cx + 1 < x2)
    {
        *addr++ = cc;
        cx += 2;
    }
    if (cx < x2)
    {
        *((PWORD) addr) = c;
    }
#endif /* _M_IX86 */
}


VOID
DIB_16BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
#if defined(_M_IX86) && !defined(_MSC_VER)
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
    PBYTE byteaddr = (PBYTE)(ULONG_PTR)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
    PWORD addr = (PWORD)byteaddr + x;
    LONG lDelta = SurfObj->lDelta;

    byteaddr = (PBYTE)addr;
    while(y1++ < y2)
    {
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
                        DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j,
                                  XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
                    }
                    else
                    {
                        DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j,
                                  XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
                    }
                    sx++;
                }
                sy++;
            }
            break;

        case BMF_4BPP:
            SourceBits_4BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                           (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
                           (BltInfo->SourcePoint.x >> 1);

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
                    if(f1 == 1)
                    {
                        SourceLine_4BPP++;
                        f1 = 0;
                    }
                    else
                    {
                        f1 = 1;
                    }
                    sx++;
                }
                SourceBits_4BPP += BltInfo->SourceSurface->lDelta;
            }
            break;

        case BMF_8BPP:
            SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                         (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
                         BltInfo->SourcePoint.x;
            DestLine = DestBits;

            for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
            {
                SourceBits = SourceLine;
                DestBits = DestLine;

                for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
                {
                    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
                                           BltInfo->XlateSourceToDest, *SourceBits);
                    SourceBits += 1;
                    DestBits += 2;
                }

                SourceLine += BltInfo->SourceSurface->lDelta;
                DestLine += BltInfo->DestSurface->lDelta;
            }
            break;

        case BMF_16BPP:
            if (NULL == BltInfo->XlateSourceToDest || 0 !=
               (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
            {
                if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
                {
                    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                                 (BltInfo->SourcePoint.y *
                                 BltInfo->SourceSurface->lDelta) + 2 *
                                 BltInfo->SourcePoint.x;

                    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
                    {
                        RtlMoveMemory(DestBits, SourceBits,
                                      2 * (BltInfo->DestRect.right -
                                      BltInfo->DestRect.left));

                        SourceBits += BltInfo->SourceSurface->lDelta;
                        DestBits += BltInfo->DestSurface->lDelta;
                    }
                }
                else
                {
                    SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                                 ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom -
                                 BltInfo->DestRect.top - 1) *
                                 BltInfo->SourceSurface->lDelta) + 2 *
                                 BltInfo->SourcePoint.x;

                    DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 +
                               ((BltInfo->DestRect.bottom - 1) *
                               BltInfo->DestSurface->lDelta) + 2 *
                               BltInfo->DestRect.left;

                    for (j = BltInfo->DestRect.bottom - 1;
                         BltInfo->DestRect.top <= j; j--)
                    {
                        RtlMoveMemory(DestBits, SourceBits, 2 *
                                      (BltInfo->DestRect.right -
                                      BltInfo->DestRect.left));

                        SourceBits -= BltInfo->SourceSurface->lDelta;
                        DestBits -= BltInfo->DestSurface->lDelta;
                    }
                }
            }
            else
            {
                if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
                {
                    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                                 (BltInfo->SourcePoint.y *
                                 BltInfo->SourceSurface->lDelta) + 2 *
                                 BltInfo->SourcePoint.x;

                    DestLine = DestBits;
                    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
                    {
                        SourceBits = SourceLine;
                        DestBits = DestLine;
                        for (i = BltInfo->DestRect.left; i <
                                 BltInfo->DestRect.right; i++)
                        {
                            *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
                                                  BltInfo->XlateSourceToDest,
                                                  *((WORD *)SourceBits));
                            SourceBits += 2;
                            DestBits += 2;
                        }
                        SourceLine += BltInfo->SourceSurface->lDelta;
                        DestLine += BltInfo->DestSurface->lDelta;
                    }
                }
                else
                {
                    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                                 ((BltInfo->SourcePoint.y +
                                 BltInfo->DestRect.bottom -
                                 BltInfo->DestRect.top - 1) *
                                 BltInfo->SourceSurface->lDelta) + 2 *
                                 BltInfo->SourcePoint.x;

                    DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 +
                               ((BltInfo->DestRect.bottom - 1) *
                               BltInfo->DestSurface->lDelta) + 2 *
                               BltInfo->DestRect.left;

                    for (j = BltInfo->DestRect.bottom - 1;
                             BltInfo->DestRect.top <= j; j--)
                    {
                        SourceBits = SourceLine;
                        DestBits = DestLine;
                        for (i = BltInfo->DestRect.left; i <
                                 BltInfo->DestRect.right; i++)
                        {
                            *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
                                                  BltInfo->XlateSourceToDest,
                                                  *((WORD *)SourceBits));
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
            SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                         (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
                          3 * BltInfo->SourcePoint.x;

            DestLine = DestBits;

            for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
            {
                SourceBits = SourceLine;
                DestBits = DestLine;

                for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
                {
                    xColor = (*(SourceBits + 2) << 0x10) +
                             (*(SourceBits + 1) << 0x08) + (*(SourceBits));

                    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
                                           BltInfo->XlateSourceToDest, xColor);

                    SourceBits += 3;
                    DestBits += 2;
                }
                SourceLine += BltInfo->SourceSurface->lDelta;
                DestLine += BltInfo->DestSurface->lDelta;
            }
            break;

        case BMF_32BPP:
            SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                         (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
                          4 * BltInfo->SourcePoint.x;

            DestLine = DestBits;

            for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
            {
                SourceBits = SourceLine;
                DestBits = DestLine;

                for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
                {
                    *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
                                           BltInfo->XlateSourceToDest,
                                           *((PDWORD) SourceBits));
                    SourceBits += 4;
                    DestBits += 2;
                }

                SourceLine += BltInfo->SourceSurface->lDelta;
                DestLine += BltInfo->DestSurface->lDelta;
            }
            break;

        default:
            DPRINT1("DIB_16BPP_Bitblt: Unhandled Source BPP: %u\n",
                     BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
            return FALSE;
    }

    return TRUE;
}

/* Optimize for bitBlt */
BOOLEAN
DIB_16BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
    ULONG DestY;

#if defined(_M_IX86) && !defined(_MSC_VER)
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
__inline PIXEL average16(PIXEL a, PIXEL b)
{
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
  // This one is the short form of the correct one, but does not work for QEMU (expects 555 format)
  //return (((a ^ b) & 0xf7deU) >> 1) + (a & b);

  //hack until short version works properly
  return a;
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

    while (NumPixels-- > 0)
    {
        p = *Source;
        if (E >= Mid)
        {
            p = average16(p, *(Source+1));
        }
        *Target++ = p;
        Source += IntPart;
        E += FractPart;
        if (E >= TgtWidth)
        {
            E -= TgtWidth;
            Source++;
        }
    }
    while (skip-- > 0)
    {
        *Target++ = *Source;
    }
}

static BOOLEAN
FinalCopy16(PIXEL *Target, PIXEL *Source, PSPAN ClipSpans, UINT ClipSpansCount, UINT *SpanIndex,
            UINT DestY, RECTL *DestRect)
{
    LONG Left, Right;

    while ( ClipSpans[*SpanIndex].Y < DestY ||
            (ClipSpans[*SpanIndex].Y == DestY &&
            ClipSpans[*SpanIndex].X + ClipSpans[*SpanIndex].Width < DestRect->left))
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

            Right = min(ClipSpans[*SpanIndex].X + ClipSpans[*SpanIndex].Width,
                    DestRect->right);

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

    int IntPart = (((SourceRect->bottom - SourceRect->top) /
                  (DestRect->bottom - DestRect->top)) * SourceSurf->lDelta) >> 1;

    int FractPart = (SourceRect->bottom - SourceRect->top) %
                    (DestRect->bottom - DestRect->top);

    int Mid = (DestRect->bottom - DestRect->top) >> 1;
    int E = 0;
    int skip;
    PIXEL *ScanLine, *ScanLineAhead;
    PIXEL *PrevSource = NULL;
    PIXEL *PrevSourceAhead = NULL;

    PIXEL *Target = (PIXEL *) ((PBYTE)DestSurf->pvScan0 + (DestRect->top *
                    DestSurf->lDelta) + 2 * DestRect->left);

    PIXEL *Source = (PIXEL *) ((PBYTE)SourceSurf->pvScan0 + (SourceRect->top *
                    SourceSurf->lDelta) + 2 * SourceRect->left);

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
    skip = (DestRect->bottom - DestRect->top < SourceRect->bottom - SourceRect->top)
            ? 0 : ((DestRect->bottom - DestRect->top) /
                  (2 * (SourceRect->bottom - SourceRect->top)) + 1);

    NumPixels -= skip;

    ScanLine = (PIXEL*)ExAllocatePool(PagedPool, (DestRect->right - DestRect->left) *
                sizeof(PIXEL));

    ScanLineAhead = (PIXEL *)ExAllocatePool(PagedPool, (DestRect->right -
                    DestRect->left) * sizeof(PIXEL));
  
    if (!ScanLine || !ScanLineAhead)
    {
      if (ScanLine) ExFreePool(ScanLine);
      if (ScanLineAhead) ExFreePool(ScanLineAhead);
      return FALSE;
    }

    DestY = DestRect->top;
    SpanIndex = 0;
    while (NumPixels-- > 0)
    {
        if (Source != PrevSource)
        {
            if (Source == PrevSourceAhead)
            {
                /* the next scan line has already been scaled and stored in
                 * ScanLineAhead; swap the buffers that ScanLine and ScanLineAhead
                 * point to
                 */
                PIXEL *tmp = ScanLine;
                ScanLine = ScanLineAhead;
                ScanLineAhead = tmp;
            }
            else
            {
                ScaleLineAvg16(ScanLine, Source, SourceRect->right - SourceRect->left,
                DestRect->right - DestRect->left);
            }
            PrevSource = Source;
        }

        if (E >= Mid && PrevSourceAhead != (PIXEL *)((BYTE *)Source +
            SourceSurf->lDelta))
        {
            int x;

            ScaleLineAvg16(ScanLineAhead, (PIXEL *)((BYTE *)Source +
                           SourceSurf->lDelta), SourceRect->right -
                           SourceRect->left, DestRect->right - DestRect->left);

            for (x = 0; x < DestRect->right - DestRect->left; x++)
            {
                ScanLine[x] = average16(ScanLine[x], ScanLineAhead[x]);
            }

            PrevSourceAhead = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
        }

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

        if (E >= DestRect->bottom - DestRect->top)
        {
            E -= DestRect->bottom - DestRect->top;
            Source = (PIXEL *)((BYTE *)Source + SourceSurf->lDelta);
        }
    } /* while */

    if (skip > 0 && Source != PrevSource)
    {
        ScaleLineAvg16(ScanLine, Source, SourceRect->right - SourceRect->left,
                       DestRect->right - DestRect->left);
    }

    while (skip-- > 0)
    {
        if (! FinalCopy16(Target, ScanLine, ClipSpans, ClipSpansCount, &SpanIndex,
                          DestY, DestRect))
        {
            /* No more spans, everything else is clipped away, we're done */
            ExFreePool(ClipSpans);
            ExFreePool(ScanLine);
            ExFreePool(ScanLineAhead);
            return TRUE;
        }
        DestY++;
        Target = (PIXEL *)((BYTE *)Target + DestSurf->lDelta);
    }

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

  DPRINT("DIB_16BPP_StretchBlt: Source BPP: %u, srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
     BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
     DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    /* Calc the Zoom height of Source */
    SrcSizeY = SourceRect->bottom - SourceRect->top;

    /* Calc the Zoom Width of Source */
    SrcSizeX = SourceRect->right - SourceRect->left;

    /* Calc the Zoom height of Destions */
    DesSizeY = DestRect->bottom - DestRect->top;

    /* Calc the Zoom width of Destions */
    DesSizeX = DestRect->right - DestRect->left;

    /* Calc the zoom factor of soruce height */
    SrcZoomYHight = SrcSizeY / DesSizeY;
    SrcZoomYLow = SrcSizeY - (SrcZoomYHight * DesSizeY);

    /* Calc the zoom factor of soruce width */
    SrcZoomXHight = SrcSizeX / DesSizeX;
    SrcZoomXLow = SrcSizeX - (SrcZoomXHight * DesSizeX);

    sx_max = DesSizeX;
    sy_max = DesSizeY;
    sy = SourceRect->top;

    DestBits = (PULONG)((PBYTE)DestSurf->pvScan0 + (DestRect->left << 1) +
                               DestRect->top * DestSurf->lDelta);

    DifflDelta = DestSurf->lDelta -  (DesSizeX << 1);

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

                DestBits = (PULONG)((ULONG_PTR)DestBits + 2);

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

                  DestBits = (PULONG)((ULONG_PTR)DestBits + 2);

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

                   DestBits = (PULONG)((ULONG_PTR)DestBits + 2);

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

        DestBits = (PULONG)((PBYTE)DestSurf->pvScan0 + (DestRect->left << 1) +
                   DestRect->top * DestSurf->lDelta);

        DifflDelta = DestSurf->lDelta -  (DesSizeX << 1);

        for (DesY=0; DesY<DesSizeY; DesY++)
        {
            sx = SourceRect->left;
            sx_dec = 0;

            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                        DIB_24BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 2);

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
        /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
        /* This is a reference implementation, it hasn't been optimized for speed */

        for (DesY=0; DesY<DesSizeY; DesY++)
        {
            sx = SourceRect->left;
            sx_dec = 0;

            for (DesX=0; DesX<DesSizeX; DesX++)
            {
                *DestBits = XLATEOBJ_iXlate(ColorTranslation,
                                        DIB_32BPP_GetPixel(SourceSurf, sx, sy));

                DestBits = (PULONG)((ULONG_PTR)DestBits + 2);

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
        return ScaleRectAvg16(DestSurf, SourceSurf, DestRect, SourceRect, MaskOrigin, BrushOrigin,
                              ClipRegion, ColorTranslation, Mode);
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
                    *((USHORT*)DestBits) = (USHORT)XLATEOBJ_iXlate(ColorTranslation,
                                                                   Source);
                }

                DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
            }

            SourceY++;
            DestBits = (ULONG*)((ULONG_PTR)DestBits + wd);
        }

    return TRUE;
}

typedef union
{
    ULONG ul;
    struct
    {
        UCHAR red;
        UCHAR green;
        UCHAR blue;
        UCHAR alpha;
    } col;
} NICEPIXEL32;

typedef union
{
    USHORT us;
    struct
    {
        USHORT  red:5,
                green:6,
                blue:5;
   } col;
} NICEPIXEL16;

static __inline UCHAR
Clamp5(ULONG val)
{
    return (val > 31) ? 31 : val;
}

static __inline UCHAR
Clamp6(ULONG val)
{
    return (val > 63) ? 63 : val;
}

BOOLEAN
DIB_16BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                     RECTL* SourceRect, CLIPOBJ* ClipRegion,
                     XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
    INT Rows, Cols, SrcX, SrcY;
    register PUSHORT Dst;
    ULONG DstDelta;
    BLENDFUNCTION BlendFunc;
    register NICEPIXEL16 DstPixel;
    register NICEPIXEL32 SrcPixel;
    UCHAR Alpha, SrcBpp;

    DPRINT("DIB_16BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
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

    Dst = (PUSHORT)((ULONG_PTR)Dest->pvScan0 + (DestRect->top * Dest->lDelta) +
          (DestRect->left << 1));
    DstDelta = Dest->lDelta - ((DestRect->right - DestRect->left) << 1);
    SrcBpp = BitsPerFormat(Source->iBitmapFormat);

    Rows = DestRect->bottom - DestRect->top;
    SrcY = SourceRect->top;
    while (--Rows >= 0)
    {
        Cols = DestRect->right - DestRect->left;
        SrcX = SourceRect->left;
        while (--Cols >= 0)
        {
            if (SrcBpp <= 16)
            {
                DstPixel.us = DIB_GetSource(Source, SrcX++, SrcY, ColorTranslation);
                SrcPixel.col.red = (DstPixel.col.red << 3) | (DstPixel.col.red >> 2);

                SrcPixel.col.green = (DstPixel.col.green << 2) |
                                     (DstPixel.col.green >> 4);

                SrcPixel.col.blue = (DstPixel.col.blue << 3) | (DstPixel.col.blue >> 2);
            }
            else
            {
                SrcPixel.ul = DIB_GetSourceIndex(Source, SrcX++, SrcY);
            }
            SrcPixel.col.red = SrcPixel.col.red *
                               BlendFunc.SourceConstantAlpha / 255;

            SrcPixel.col.green = SrcPixel.col.green *
                                 BlendFunc.SourceConstantAlpha / 255;

            SrcPixel.col.blue = SrcPixel.col.blue *
                                BlendFunc.SourceConstantAlpha / 255;

            SrcPixel.col.alpha = (SrcBpp == 32) ?
                                 (SrcPixel.col.alpha *
                                 BlendFunc.SourceConstantAlpha / 255) :
                                 BlendFunc.SourceConstantAlpha;

            Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
                    SrcPixel.col.alpha : BlendFunc.SourceConstantAlpha;

         DstPixel.us = *Dst;
         DstPixel.col.red = Clamp5(DstPixel.col.red * (255 - Alpha) / 255 +
                                   (SrcPixel.col.red >> 3));

         DstPixel.col.green = Clamp6(DstPixel.col.green * (255 - Alpha) / 255 +
                                     (SrcPixel.col.green >> 2));

         DstPixel.col.blue = Clamp5(DstPixel.col.blue * (255 - Alpha) / 255 +
                                    (SrcPixel.col.blue >> 3));

         *Dst++ = DstPixel.us;
      }

      Dst = (PUSHORT)((ULONG_PTR)Dst + DstDelta);
      SrcY++;
    }

    return TRUE;
}

/* EOF */
