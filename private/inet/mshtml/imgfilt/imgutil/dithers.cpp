#include "stdafx.h"
#include "dithers.h"

#ifndef _DEBUG
#define INLINE  __inline
#else
#define INLINE
#endif

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

const BYTE g_abClamp[] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,
47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,
69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,
90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,
109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,
126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,
143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,
177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,
194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,
211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,
228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,
245,246,247,248,249,250,251,252,253,254,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255
};

INLINE UINT Clamp8(int z)
{
#ifdef _DEBUG
    UINT t = (z & 0xff00) ? (0xff & ~(z >> 16)) : z;

    if (t != g_abClamp[z + 128])
        DebugBreak();
#endif

    return g_abClamp[z + 128];
}

INLINE WORD rgb555(BYTE r, BYTE g, BYTE b)
{
    return( WORD( ((((WORD)(r) << 5) | (WORD)(g)) << 5) | (WORD)(b) ) );
}

//-----------------------------------------------------------------------------
// Halftoning stuff
//-----------------------------------------------------------------------------
//
// This table is used to halftone from 8 to 5 bpp.  Typically, 16 bit
// halftoning code will map an 8 bit value to a 5 bit value, map it back to
// 8 bits, compute some error, and use a halftone table to adjust the 5 bit
// value for the error.  This array is a concatenation of 8 different 8-to-5
// tables that include the error factoring in their mapping.  It is used with
// the halftoning table below, which gives indices to each of the mapping
// tables within the array.   Given the correct table pointer for a pixel,
// callers can perform a single lookup per color component in this table to get
// a halftoned 5 bit component.
//
#pragma data_seg(".text", "CODE")
BYTE aHT16Data[] =
{
      0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
      5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,
      7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,
      9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
     11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12,
     13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14,
     15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
     17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
     19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20,
     21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
     23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24,
     25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
     27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28,
     29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 31,
      0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,
      3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  5,
      5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  7,
      7,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,  9,
      9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 11,
     11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13,
     13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 15,
     15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 17,
     17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19,
     19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21,
     21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23,
     23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25,
     25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27,
     27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29,
     29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 31,
     31, 31, 31, 31, 31, 31
};
UINT aHT16Heads[4][4] =
{
    262, 256, 261,   0,
    258, 260, 257, 259,
    261,   0, 262, 256,
    257, 259, 258, 260,
};
#pragma data_seg()

INLINE UINT *
Get555HalftoneRow(UINT y)
{
    return aHT16Heads[y % 4];
}

INLINE BYTE *
Get555HalftoneTable(UINT *row, UINT x)
{
    return aHT16Data + row[x % 4];
}

//-----------------------------------------------------------------------------
// Rounding stuff
//-----------------------------------------------------------------------------
//
// round an 8bit value to a 5bit value with good distribution
//
#if 0   // not presently used
#pragma data_seg(".text", "CODE")
BYTE aRound8to5[] = {
      0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
      2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
      4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,
      6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,
      8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9, 10,
     10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 12,
     12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
     14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
     16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17,
     18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19,
     19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21,
     21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23,
     23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25,
     25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27,
     27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29,
     29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31,
};
#pragma data_seg()
#endif  // not presently used

//
// complement of table above
//
#pragma data_seg(".text", "CODE")
BYTE aRound5to8[] = {
      0,  8, 16, 25, 33, 41, 49, 58, 66, 74, 82, 90, 99,107,115,123,
    132,140,148,156,165,173,181,189,197,206,214,222,230,239,247,255,
};
#pragma data_seg()

///////////////////////////////////////////////////////////////////////////////
//
// Dithering stuff.
//
// This code implements error-diffusion to an arbitrary set of colors,
// optionally with transparency.  Since the output colors can be arbitrary,
// the color picker for the dither is a 32k inverse-mapping table.  24bpp
// values are whacked down to 16bpp (555) and used as indices into the table.
// To compensate for posterization effects when converting 24bpp to 16bpp, an
// ordered dither (16bpp halftone) is used to generate the 555 color.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void SwapError(ERRBUF **a, ERRBUF **b)
{
    ERRBUF *te;

    te = *a;
    *a = *b;
    *b = te;
}

INLINE void ZeroError(ERRBUF *err, size_t pels)
{
    ZeroMemory(err - 1, ErrbufBytes(pels));
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith8to8()                                              8bpp to 8bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color table
// entry with accumulated error for the pixel.  Halftones this 24bpp value to a
// 16bpp 555 color.  Uses the 16bpp color as a lookup into an inverse mapping
// table to pick the output color for the pixel.  Uses the destination color
// table entry to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan8to8(BYTE *dst, const BYTE *src, const RGBQUAD *colorsIN,
    const RGBQUAD *colorsOUT, const BYTE *map, ERRBUF *cur_err,
    ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src++, x++)
    {
        register const RGBQUAD *pChosen;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        pChosen = colorsIN + *src;

        r = Clamp8((int)pChosen->rgbRed   + (cur_err->r >> 4));
        g = Clamp8((int)pChosen->rgbGreen + (cur_err->g >> 4));
        b = Clamp8((int)pChosen->rgbBlue  + (cur_err->b >> 4));
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        pChosen = colorsOUT + (*dst++ = map[rgb555(tbl[r], tbl[g], tbl[b])]);

        r -= (int)pChosen->rgbRed;
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        g -= (int)pChosen->rgbGreen;
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        b -= (int)pChosen->rgbBlue;
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        nxt_err++;
    }
}

void Dith8to8(BYTE *dst, const BYTE *src, int dst_next_scan, int src_next_scan,
    const RGBQUAD *colorsIN, const RGBQUAD *colorsOUT, const BYTE *map,
    ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx, UINT y, int cy)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan8to8(dst, src, colorsIN, colorsOUT, map, cur_err, nxt_err,
            x, cx, y);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

INLINE void ConvertScan8to8( BYTE* pbDest, const BYTE* pbSrc, 
   const RGBQUAD* prgbColors, const BYTE* pbMap, UINT x, UINT xl, UINT y )
{
   UINT* pRow = Get555HalftoneRow( y );
   BYTE* pbTable;
   BYTE r;
   BYTE g;
   BYTE b;

   for (; x < xl; pbSrc += 3, x++ )
   {
      r = prgbColors[*pbSrc].rgbRed;
      g = prgbColors[*pbSrc].rgbGreen;
      b = prgbColors[*pbSrc].rgbBlue;

      pbTable = Get555HalftoneTable( pRow, x );
      *pbDest = pbMap[rgb555( pbTable[r], pbTable[g], pbTable[b] )];
   }
}

void Convert8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const RGBQUAD* prgbColors, const BYTE* pbMap, UINT x, 
   UINT nWidth, UINT y, int nHeight )
{
   int dy;
   UINT x2;

   x2 = x+nWidth;

   if( nHeight < 0 )
   {
      dy = -1;
      nHeight *= -1;
   }
   else
   {
      dy = 1;
   }

   while( nHeight )
   {
      ConvertScan8to8( pbDest, pbSrc, prgbColors, pbMap, x, x2, y );

      pbSrc += nSrcPitch;
      pbDest += nDestPitch;

      y += dy;
      nHeight--;
   }
}

INLINE void DithScanGray8to8( BYTE* pbDest, const BYTE* pbSrc,
   const RGBQUAD* prgbColors, const BYTE* pbMap, ERRBUF* pCurrentError,
   ERRBUF* pNextError, UINT x, UINT xl, UINT y )
{
   BYTE bSrc;
   BYTE bDest;
    UINT* pRow = Get555HalftoneRow( y );

    for(; x < xl; pbSrc++, x++ )
    {
        const RGBQUAD* prgbChosen;
        BYTE* pbTable;
        int r;
        int g;
        int b;

        bSrc = *pbSrc;
        r = Clamp8( (int)bSrc + pCurrentError->r/16 );
        g= Clamp8( (int)bSrc + pCurrentError->g/16 );
        b = Clamp8( (int)bSrc + pCurrentError->b/16 );
        pCurrentError++;

        pbTable = Get555HalftoneTable( pRow, x );
        bDest = pbMap[rgb555( pbTable[r], pbTable[g], pbTable[b] )];
         prgbChosen = prgbColors+bDest;
         *pbDest = bDest;
         pbDest++;

        r -= (int)prgbChosen->rgbRed;
        (pNextError+1)->r += r * 1;
        (pNextError-1)->r += r * 3;
        (pNextError)->r += r * 5;
        (pCurrentError)->r += r * 7;

        g -= (int)prgbChosen->rgbGreen;
        (pNextError+1)->g += g * 1;
        (pNextError-1)->g += g * 3;
        (pNextError)->g += g * 5;
        (pCurrentError)->g += g * 7;

        b -= (int)prgbChosen->rgbBlue;
        (pNextError+1)->b += b * 1;
        (pNextError-1)->b += b * 3;
        (pNextError)->b += b * 5;
        (pCurrentError)->b += b * 7;

        pNextError++;
    }
}

void DithGray8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const RGBQUAD* prgbColors, const BYTE* pbMap, 
   ERRBUF* pCurrentError, ERRBUF* pNextError, UINT x, UINT cx, UINT y, int cy )
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
    {
        dy = 1;
    }

    if (y & 1)
    {
        SwapError( &pCurrentError, &pNextError );
    }
    while (cy)
    {
        DithScanGray8to8( pbDest, pbSrc, prgbColors, pbMap, pCurrentError, 
           pNextError, x, cx, y );

        ZeroError( pCurrentError, cx );
        SwapError( &pCurrentError, &pNextError );

        *(BYTE **)&pbSrc += nSrcPitch;
        *(BYTE **)&pbDest += nDestPitch;
        y += dy;
        cy--;
    }
}

INLINE void ConvertScanGray8to8( BYTE* pbDest, const BYTE* pbSrc,
   const BYTE* pbMap, UINT x, UINT xl, UINT y )
{
   UINT* pRow = Get555HalftoneRow( y );
   BYTE* pbTable;
   BYTE g;

   for (; x < xl; pbSrc++, x++ )
   {
      g = *pbSrc;

      pbTable = Get555HalftoneTable( pRow, x );
      *pbDest = pbMap[rgb555( pbTable[g], pbTable[g], pbTable[g] )];
   }
}

void ConvertGray8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const BYTE* pbMap, UINT x, UINT nWidth, UINT y, int nHeight )
{
   int dy;
   UINT x2;

   x2 = x+nWidth;

   if( nHeight < 0 )
   {
      dy = -1;
      nHeight *= -1;
   }
   else
   {
      dy = 1;
   }

   while( nHeight )
   {
      ConvertScanGray8to8( pbDest, pbSrc, pbMap, x, x2, y );

      pbSrc += nSrcPitch;
      pbDest += nDestPitch;

      y += dy;
      nHeight--;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith8to8t()                           8bpp to 8bpp dither with transparency.
//
// If the source pixel is the given source transparency color, this routine
// picks the destination transparency color for output and zero error is
// accumulated to the pixel's neighbors.
// Otherwise, this routine functions identically to Dith8to8.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan8to8t(BYTE *dst, const BYTE *src,
    const RGBQUAD *colorsIN, const RGBQUAD *colorsOUT, const BYTE *map,
    ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y,
    BYTE indexTxpOUT, BYTE indexTxpIN)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src++, x++)
    {
        register const RGBQUAD *pChosen;
        register BYTE *tbl;
        register BYTE index;
        register int r;
        register int g;
        register int b;

        index = *src;
        if (index == indexTxpIN)
        {
            *dst++ = indexTxpOUT;
            cur_err++;
            nxt_err++;
            continue;
        }

        pChosen = colorsIN + index;
        r = Clamp8((int)pChosen->rgbRed   + (cur_err->r >> 4));
        g = Clamp8((int)pChosen->rgbGreen + (cur_err->g >> 4));
        b = Clamp8((int)pChosen->rgbBlue  + (cur_err->b >> 4));
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        pChosen = colorsOUT + (*dst++ = map[rgb555(tbl[r], tbl[g], tbl[b])]);

        r -= (int)pChosen->rgbRed;
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        g -= (int)pChosen->rgbGreen;
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        b -= (int)pChosen->rgbBlue;
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        nxt_err++;
    }
}

void Dith8to8t(BYTE *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colorsIN, const RGBQUAD *colorsOUT,
    const BYTE *map, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx, UINT y,
    int cy, BYTE indexTxpOUT, BYTE indexTxpIN)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan8to8t(dst, src, colorsIN, colorsOUT, map, cur_err, nxt_err,
            x, cx, y, indexTxpOUT, indexTxpIN);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith8to16()                                            8bpp to 16bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color table
// entry with accumulated error for the pixel.  Halftones this 24bpp value to a
// 16bpp 555 color.  Remaps this color to 24bpp to compute and accumulates
// error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan8to16(WORD *dst, const BYTE *src, const RGBQUAD *colors,
    ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src++, x++)
    {
        register const RGBQUAD *pChosen;
        register WORD wColor;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        pChosen = colors + *src;
        r = Clamp8((int)pChosen->rgbRed   + cur_err->r / 16);
        g = Clamp8((int)pChosen->rgbGreen + cur_err->g / 16);
        b = Clamp8((int)pChosen->rgbBlue  + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        wColor = (*dst++ = rgb555(tbl[r], tbl[g], tbl[b]));

        b -= (int)aRound5to8[wColor & 0x1f];
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        g -= (int)aRound5to8[(wColor >> 5) & 0x1f];
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        r -= (int)aRound5to8[(wColor >> 10) & 0x1f];
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        nxt_err++;
    }
}

void Dith8to16(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, ERRBUF *cur_err, ERRBUF *nxt_err,
    UINT x, UINT cx, UINT y, int cy)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan8to16(dst, src, colors, cur_err, nxt_err, x, cx, y);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith8to16t()                         8bpp to 16bpp dither with transparency.
//
// If the source pixel is the given source transparency color, this routine
// picks the destination transparency color for output and zero error is
// accumulated to the pixel's neighbors.
// Otherwise, this routine functions identically to Dith8to16.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan8to16t(WORD *dst, const BYTE *src, const RGBQUAD *colors,
    ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y,
    WORD wColorTxpOUT, BYTE indexTxpIN)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src ++, x++)
    {
        register const RGBQUAD *pChosen;
        register WORD wColor;
        register BYTE *tbl;
        register BYTE index;
        register int r;
        register int g;
        register int b;

        index = *src;
        if (index == indexTxpIN)
        {
            *dst++ = wColorTxpOUT;
            cur_err++;
            nxt_err++;
            continue;
        }

        pChosen = colors + index;
        r = Clamp8((int)pChosen->rgbRed   + cur_err->r / 16);
        g = Clamp8((int)pChosen->rgbGreen + cur_err->g / 16);
        b = Clamp8((int)pChosen->rgbBlue  + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        wColor = (*dst++ = rgb555(tbl[r], tbl[g], tbl[b]));

        b -= (int)aRound5to8[wColor & 0x1f];
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        g -= (int)aRound5to8[(wColor >> 5) & 0x1f];
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        r -= (int)aRound5to8[(wColor >> 10) & 0x1f];
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        nxt_err++;
    }
}

void Dith8to16t(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, ERRBUF *cur_err, ERRBUF *nxt_err,
    UINT x, UINT cx, UINT y, int cy, WORD wColorTxpOUT, BYTE indexTxpIN)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan8to16t(dst, src, colors, cur_err, nxt_err, x, cx, y,
            wColorTxpOUT, indexTxpIN);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith24to8()                                            24bpp to 8bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  Halftones this 24bpp value to a 16bpp 555
// color.  Uses the 16bpp color as a lookup into an inverse mapping table to
// pick the output color for the pixel.  Uses the destination color table entry
// to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24to8(BYTE *dst, const BYTE *src, const RGBQUAD *colors,
    const BYTE *map, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src += 3, x++)
    {
        register const RGBQUAD *pChosen;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        r = Clamp8((int)src[2] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[0] + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        pChosen = colors + (*dst++ = map[rgb555(tbl[r], tbl[g], tbl[b])]);

        r -= (int)pChosen->rgbRed;
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        g -= (int)pChosen->rgbGreen;
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        b -= (int)pChosen->rgbBlue;
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        nxt_err++;
    }
}

void Dith24to8(BYTE *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, const BYTE *map, ERRBUF *cur_err,
    ERRBUF *nxt_err, UINT x, UINT cx, UINT y, int cy)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan24to8(dst, src, colors, map, cur_err, nxt_err, x, cx, y);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

INLINE void ConvertScan24to8( BYTE* pbDest, const BYTE* pbSrc, 
   const BYTE* pbMap, UINT x, UINT xl, UINT y )
{
   UINT* pRow;
   BYTE* pbTable;
   BYTE r;
   BYTE g;
   BYTE b;

   pRow = Get555HalftoneRow( y );

   for (; x < xl; pbSrc += 3, x++ )
   {
      r = pbSrc[2];
      g = pbSrc[1];
      b = pbSrc[0];

      pbTable = Get555HalftoneTable( pRow, x );
      *pbDest = pbMap[rgb555( pbTable[r], pbTable[g], pbTable[b] )];
   }
}

void Convert24to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const BYTE* pbMap, UINT x, UINT nWidth, UINT y, int nHeight )
{
   int dy;
   UINT x2;

   x2 = x+nWidth;

   if( nHeight < 0 )
   {
      dy = -1;
      nHeight *= -1;
   }
   else
   {
      dy = 1;
   }

   while( nHeight )
   {
      ConvertScan24to8( pbDest, pbSrc, pbMap, x, x2, y );

      pbSrc += nSrcPitch;
      pbDest += nDestPitch;

      y += dy;
      nHeight--;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith24to16()                                          24bpp to 16bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  Halftones this 24bpp value to a 16bpp 555
// color.  Remaps this color to 24bpp to compute and accumulates error for
// neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24to16(WORD *dst, const BYTE *src, ERRBUF *cur_err,
    ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; src += 3, x++)
    {
        register WORD wColor;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        r = Clamp8((int)src[2] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[0] + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        wColor = (*dst++ = rgb555(tbl[r], tbl[g], tbl[b]));

        b -= (int)aRound5to8[wColor & 0x1f];
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        g -= (int)aRound5to8[(wColor >> 5) & 0x1f];
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        r -= (int)aRound5to8[(wColor >> 10) & 0x1f];
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        nxt_err++;
    }
}

void Dith24to16(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx,
    UINT y, int cy)
{
    int dy;

    cx += x;

    if (cy < 0)
    {
        dy = -1;
        cy *= -1;
    }
    else
        dy = 1;

    if (y & 1)
        SwapError(&cur_err, &nxt_err);

    while (cy)
    {
        DithScan24to16(dst, src, cur_err, nxt_err, x, cx, y);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////

