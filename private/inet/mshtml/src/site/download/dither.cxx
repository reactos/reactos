#include "headers.hxx"

#ifndef X_DITHERS_H_
#define X_DITHERS_H_
#include "dithers.h"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include "shlobj.h"
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include "shlobjp.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef WIN16
#ifndef X_SHLGUIDP_H_
#define X_SHLGUIDP_H_
#include "shlguidp.h"
#endif
#endif

#if DBG==1
#define INLINE
#else
#define INLINE __inline
#endif

MtExtern(Dwn)
MtDefine(ImgDithBufs, Dwn, "AllocDitherBuffers")
MtDefine(CIntDitherer, Dwn, "CIntDitherer")

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

const BYTE g_abClamp[] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

INLINE UINT Clamp8(int z)
{
#if DBG==1
    UINT t = (z & 0xff00) ? (0xff & ~(z >> 16)) : z;

    if (t != g_abClamp[z + 256])
        DebugBreak();
#endif

    return g_abClamp[z + 256];
}

INLINE WORD rgb555(BYTE r, BYTE g, BYTE b)
{
    return( WORD( ((((WORD)(r) << 5) | (WORD)(g)) << 5) | (WORD)(b) ) );
}

INLINE WORD rgb565(BYTE r, BYTE g, BYTE b)
{
//    return( WORD( ((((WORD)(r) << 5) | (WORD)(g)) << 6) | (WORD)(b) ) );
//    return( WORD( ((((WORD)(r) << 6) | (WORD)((g << 1) | !!(g > 16))) << 5) | (WORD)(b) ) );
    return((WORD)(((((((WORD)(r) << 5) | (WORD)(g)) << 1) | (WORD)((g) > 16)) << 5) | (WORD)(b)));
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
const BYTE aHT16Data[] =
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
const UINT aHT16Heads[4][4] =
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
    return (UINT*) (aHT16Heads[y % 4]);
}

INLINE BYTE *
Get555HalftoneTable(UINT *row, UINT x)
{
    return (BYTE*) (aHT16Data + row[x % 4]);
}

//-----------------------------------------------------------------------------
// Rounding stuff
//-----------------------------------------------------------------------------
//
// round an 8bit value to a 5bit value with good distribution
//
#pragma data_seg(".text", "CODE")
const BYTE aRound8to5[] = {
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

#pragma data_seg(".text", "CODE")
const BYTE aRound8to6[] =
{
  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,
  3,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,
  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11, 11, 11,
 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15,
 15, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19,
 19, 20, 20, 20, 20, 21, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23,
 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27,
 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31,
 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35,
 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39,
 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 42, 43,
 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47,
 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51,
 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55,
 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59,
 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 63,
};
#pragma data_seg()

//
// complement of table above
//
#pragma data_seg(".text", "CODE")
const BYTE aRound5to8[] = {
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

#if 0
// UNUSED DITHERING CODE
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

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
      *pbDest++ = pbMap[rgb555( pbTable[g], pbTable[g], pbTable[g] )];
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

// END OF UNUSED DITHERING CODE
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Dith24rto15()                                         24bpp to 15bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  THE SOURCE COMPONENT ORDER IS OPPOSITE A DIB:
// RED, GREEN, BLUE.  Halftones this 24bpp value to a 16bpp 555 color.  Remaps this 
// color to 24bpp to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24rto15(WORD *dst, const BYTE *src, ERRBUF *cur_err,
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

        r = Clamp8((int)src[0] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[2] + cur_err->b / 16);
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

void Dith24rto15(WORD *dst, const BYTE *src, int dst_next_scan,
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
        DithScan24rto15(dst, src, cur_err, nxt_err, x, cx, y);

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
// Dith24rto16()                                         24bpp to 16bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  THE SOURCE COMPONENT ORDER IS OPPOSITE A DIB:
// RED, GREEN, BLUE.  Halftones this 24bpp value to a 16bpp 565 color.  Remaps this 
// color to 24bpp to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24rto16(WORD *dst, const BYTE *src, ERRBUF *cur_err,
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

        r = Clamp8((int)src[0] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[2] + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        wColor = (*dst++ = rgb565(tbl[r], tbl[g], tbl[b]));

        b -= (int)aRound5to8[wColor & 0x1f];
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        g -= (int)aRound5to8[(wColor >> 6) & 0x1f];
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        r -= (int)aRound5to8[(wColor >> 11) & 0x1f];
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        nxt_err++;
    }
}

void Dith24rto16(WORD *dst, const BYTE *src, int dst_next_scan,
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
        DithScan24rto16(dst, src, cur_err, nxt_err, x, cx, y);

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
// Convert24rto16()                                         24bpp to 16bpp translation.
//
// Converts 24bpp source color to 16bpp (565) format by mapping each component
// to the closest value in 565 space.  THE SOURCE COMPONENT ORDER IS OPPOSITE A DIB:
// RED, GREEN, BLUE. 
//
///////////////////////////////////////////////////////////////////////////////

void Convert24rto16(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, UINT x, UINT cx, UINT y, int cy)
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

    while (cy)
    {
        WORD *pdst;
        const BYTE *psrc;
        UINT xt;

        pdst = dst;
        psrc = src;
        for (xt = x; xt < cx; psrc += 3, xt++)
        {
            *pdst++ = (WORD)((((WORD)aRound8to5[psrc[0]] << 6) | (WORD)aRound8to6[psrc[1]]) << 5) | (WORD)aRound8to5[psrc[2]];
        }
        
        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Convert24rto15()                                         24bpp to 15bpp translation.
//
// Converts 24bpp source color to 15bpp (555) format by mapping each component
// to the closest value in 555 space.  THE SOURCE COMPONENT ORDER IS OPPOSITE A DIB:
// RED, GREEN, BLUE. 
//
///////////////////////////////////////////////////////////////////////////////

void Convert24rto15(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, UINT x, UINT cx, UINT y, int cy)
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

    while (cy)
    {
        WORD *pdst;
        const BYTE *psrc;
        UINT xt;

        pdst = dst;
        psrc = src;
        for (xt = x; xt < cx; psrc += 3, xt++)
        {
#ifdef _MAC
            WORD    wVal;
            
            wVal = (WORD)((((WORD)aRound8to5[psrc[0]] << 5) | (WORD)aRound8to5[psrc[1]]) << 5) | (WORD)aRound8to5[psrc[2]];
            *pdst++ = (wVal << 8) | (wVal >> 8);
#else
            *pdst++ = (WORD)((((WORD)aRound8to5[psrc[0]] << 5) | (WORD)aRound8to5[psrc[1]]) << 5) | (WORD)aRound8to5[psrc[2]];
#endif
        }
        
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

///////////////////////////////////////////////////////////////////////////////
//
// Dith24rto8()                                            24bpp to 8bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  Halftones this 24bpp value to a 16bpp 555
// color.  Uses the 16bpp color as a lookup into an inverse mapping table to
// pick the output color for the pixel.  Uses the destination color table entry
// to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24rto8(BYTE *dst, const BYTE *src, const RGBQUAD *colors,
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

        r = Clamp8((int)src[0] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[2] + cur_err->b / 16);

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

void Dith24rto8(BYTE *dst, const BYTE *src, int dst_next_scan,
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
        DithScan24rto8(dst, src, colors, map, cur_err, nxt_err, x, cx, y);

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
// Dith15to8()                                            16bpp to 8bpp dither.
//
// Uses the 16bpp color as a lookup into an inverse mapping table to
// pick the output color for the pixel.  Uses the destination color table entry
// to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan15to8(BYTE *dst, const WORD *src, const RGBQUAD *colors,
    const BYTE *map, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; ++src, x++)
    {
        register const RGBQUAD *pChosen;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        r = Clamp8((int)aRound5to8[(*src >> 10) & 0x1F] + cur_err->r / 16);
        g = Clamp8((int)aRound5to8[(*src >> 5) & 0x1F] + cur_err->g / 16);
        b = Clamp8((int)aRound5to8[*src & 0x1F] + cur_err->b / 16);
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

void Dith15to8(BYTE *dst, const WORD *src, int dst_next_scan,
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
        DithScan15to8(dst, src, colors, map, cur_err, nxt_err, x, cx, y);

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
// Dith16to8()                                            16bpp to 8bpp dither.
//
// Uses the 16bpp color as a lookup into an inverse mapping table to
// pick the output color for the pixel.  Uses the destination color table entry
// to compute and accumulates error for neighboring pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan16to8(BYTE *dst, const WORD *src, const RGBQUAD *colors,
    const BYTE *map, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; ++src, x++)
    {
        register const RGBQUAD *pChosen;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;

        r = Clamp8((int)aRound5to8[(*src >> 11) & 0x1F] + cur_err->r / 16);
        g = Clamp8((int)aRound5to8[(*src >> 6) & 0x1F] + cur_err->g / 16);
        b = Clamp8((int)aRound5to8[*src & 0x1F] + cur_err->b / 16);
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

void Dith16to8(BYTE *dst, const WORD *src, int dst_next_scan,
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
        DithScan16to8(dst, src, colors, map, cur_err, nxt_err, x, cx, y);

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

INLINE void DithScanGray8to16( WORD* dst, const BYTE* src,
   ERRBUF* cur_err, ERRBUF* nxt_err, UINT x, UINT xl, UINT y )
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; ++src, x++)
    {
        register WORD wColor;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;
        int bSrc;

        bSrc = (int)*src;
        r = Clamp8(bSrc + cur_err->r / 16);
        g = Clamp8(bSrc + cur_err->g / 16);
        b = Clamp8(bSrc + cur_err->b / 16);
        cur_err++;

        tbl = Get555HalftoneTable(row, x);
        wColor = (*dst++ = rgb565(tbl[r], tbl[g], tbl[b]));

        b -= (int)aRound5to8[wColor & 0x1f];
        (nxt_err+1)->b += b * 1;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        g -= (int)aRound5to8[(wColor >> 6) & 0x1f];
        (nxt_err+1)->g += g * 1;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        r -= (int)aRound5to8[(wColor >> 11) & 0x1f];
        (nxt_err+1)->r += r * 1;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        nxt_err++;
    }
}

void DithGray8to16( WORD* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, ERRBUF* pCurrentError, ERRBUF* pNextError, UINT x, 
   UINT cx, UINT y, int cy )
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
        DithScanGray8to16( pbDest, pbSrc, pCurrentError, pNextError, x, cx, y );

        ZeroError( pCurrentError, cx );
        SwapError( &pCurrentError, &pNextError );

        *(BYTE **)&pbSrc += nSrcPitch;
        *(BYTE **)&pbDest += nDestPitch;
        y += dy;
        cy--;
    }
}

INLINE void DithScanGray8to15( WORD* dst, const BYTE* src,
   ERRBUF* cur_err, ERRBUF* nxt_err, UINT x, UINT xl, UINT y )
{
    UINT *row = Get555HalftoneRow(y);

    for (; x < xl; ++src, x++)
    {
        register WORD wColor;
        register BYTE *tbl;
        register int r;
        register int g;
        register int b;
        int bSrc;

        bSrc = (int)*src;
        r = Clamp8(bSrc + cur_err->r / 16);
        g = Clamp8(bSrc + cur_err->g / 16);
        b = Clamp8(bSrc + cur_err->b / 16);
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

void DithGray8to15( WORD* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, ERRBUF* pCurrentError, ERRBUF* pNextError, UINT x, 
   UINT cx, UINT y, int cy )
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
        DithScanGray8to15( pbDest, pbSrc, pCurrentError, pNextError, x, cx, y );

        ZeroError( pCurrentError, cx );
        SwapError( &pCurrentError, &pNextError );

        *(BYTE **)&pbSrc += nSrcPitch;
        *(BYTE **)&pbDest += nDestPitch;
        y += dy;
        cy--;
    }
}

#ifdef UNIX
INLINE void DithScanGray8to1( BYTE* pbDest, const BYTE* pbSrc,
   ERRBUF* pCurrentError, ERRBUF* pNextError, UINT x, UINT xl, UINT y )
{
    BYTE bSrc;
    BYTE bDest;
    register BYTE byOut = 0;

    for(; x < xl; pbSrc++, x++ )
    {
        int r;
        int g;
        int b;
        int v;

        bSrc = *pbSrc;
        r = Clamp8( (int)bSrc + pCurrentError->r/16 );
        g = Clamp8( (int)bSrc + pCurrentError->g/16 );
        b = Clamp8( (int)bSrc + pCurrentError->b/16 );
        v = (r * 30 + g * 59 + b * 11) / 100;
        v = (v > 127) ? 255 : 0;
        pCurrentError++;

        byOut = (byOut << 1) | (v & 1);
        if ((x & 7) == 7)
        {
            *pbDest++ = byOut;
            byOut = 0;
        }
        
        r -= v;
        (pNextError+1)->r += r;
        (pNextError-1)->r += r * 3;
        (pNextError)->r += r * 5;
        (pCurrentError)->r += r * 7;

        g -= v;
        (pNextError+1)->g += g;
        (pNextError-1)->g += g * 3;
        (pNextError)->g += g * 5;
        (pCurrentError)->g += g * 7;

        b -= v;
        (pNextError+1)->b += b;
        (pNextError-1)->b += b * 3;
        (pNextError)->b += b * 5;
        (pCurrentError)->b += b * 7;

        pNextError++;
    }

    if (x & 7)
    {
        *pbDest++ = byOut << (8 - (x & 7));
    }
    
}

void DithGray8to1( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, ERRBUF* pCurrentError, ERRBUF* pNextError, 
   UINT x, UINT cx, UINT y, int cy )
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
        DithScanGray8to1( pbDest, pbSrc, pCurrentError, pNextError, x, cx, y );

        ZeroError( pCurrentError, cx );
        SwapError( &pCurrentError, &pNextError );

        *(BYTE **)&pbSrc += nSrcPitch;
        *(BYTE **)&pbDest += nDestPitch;
        y += dy;
        cy--;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Dith24rto1()                                            24bpp to 1bpp dither.
//
// Computes the 24bpp source color by combining the source pixel's color with
// accumulated error for the pixel.  Halftones this 24bpp value to a monochrome
// color.  Uses threshold of 128 to determine the output color for the pixel.  
// Uses the destination color to compute and accumulates error for neighboring 
// pixels.
//
///////////////////////////////////////////////////////////////////////////////

INLINE void DithScan24rto1(BYTE *dst, const BYTE *src, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT xl, UINT y)
{
    register BYTE byOut = 0;

    for (; x < xl; src += 3, x++)
    {
        register int r;
        register int g;
        register int b;
        register int v;

        r = Clamp8((int)src[0] + cur_err->r / 16);
        g = Clamp8((int)src[1] + cur_err->g / 16);
        b = Clamp8((int)src[2] + cur_err->b / 16);
        v = (r * 30 + g * 59 + b * 11) / 100;
        v = (v > 127) ? 255 : 0;
        cur_err++;

        byOut = (byOut << 1) | (v & 1);
        if ((x & 7) == 7)
        {
            *dst++ = byOut;
            byOut = 0;
        }

        r -= v;
        (nxt_err+1)->r += r;
        (nxt_err-1)->r += r * 3;
        (nxt_err  )->r += r * 5;
        (cur_err  )->r += r * 7;

        g -= v;
        (nxt_err+1)->g += g;
        (nxt_err-1)->g += g * 3;
        (nxt_err  )->g += g * 5;
        (cur_err  )->g += g * 7;

        b -= v;
        (nxt_err+1)->b += b;
        (nxt_err-1)->b += b * 3;
        (nxt_err  )->b += b * 5;
        (cur_err  )->b += b * 7;

        nxt_err++;
    }

    if (x & 7)
    {
        *dst++ = byOut << (8 - (x & 7));
    }
}

void Dith24rto1(BYTE *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, ERRBUF *cur_err, ERRBUF *nxt_err, 
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
        DithScan24rto1(dst, src, cur_err, nxt_err, x, cx, y);

        ZeroError(cur_err, cx);
        SwapError(&cur_err, &nxt_err);

        *(BYTE **)&src += src_next_scan;
        *(BYTE **)&dst += dst_next_scan;
        y += dy;
        cy--;
    }
}
#endif

#ifndef WIN16

class CIntDitherer : public IIntDitherer
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CIntDitherer))

    CIntDitherer();
    
    // IUnknown members

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IIntDitherer members
    
    STDMETHOD(DitherTo8bpp)( BYTE * pDestBits, LONG nDestPitch, 
                    BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                    RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                    BYTE * pbDestInvMap,
                    LONG x, LONG y, LONG cx, LONG cy,
                    LONG lDestTrans, LONG lSrcTrans);

public:
    ULONG   m_cRef;
};

CIntDitherer::CIntDitherer()
{
    m_cRef = 1;
}

STDMETHODIMP
CIntDitherer::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IIntDitherer || riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
        ((LPUNKNOWN)*ppv)->AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG)
CIntDitherer::AddRef()
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG)
CIntDitherer::Release()
{
    --m_cRef;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CIntDitherer::DitherTo8bpp(  BYTE * pDestBits, LONG nDestPitch, 
                    BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                    RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                    BYTE * pbDestInvMap,
                    LONG x, LONG y, LONG cx, LONG cy,
                    LONG lDestTrans, LONG lSrcTrans)
{
    return DitherTo8(pDestBits, nDestPitch, pSrcBits, nSrcPitch, bfidSrc, 
                    prgbDestColors, prgbSrcColors, pbDestInvMap,
                    x, y, cx, cy, lDestTrans, lSrcTrans);
}
#endif // ndef WIN16

HRESULT DitherTo8(  BYTE * pDestBits, LONG nDestPitch, 
                    BYTE * pSrcBits, LONG nSrcPitch, REFGUID bfidSrc, 
                    RGBQUAD * prgbDestColors, RGBQUAD * prgbSrcColors,
                    BYTE * pbDestInvMap,
                    LONG x, LONG y, LONG cx, LONG cy,
                    LONG lDestTrans, LONG lSrcTrans)
{
    ERRBUF* m_pErrBuf1;
    ERRBUF* m_pErrBuf2;

    HRESULT hr;

    hr = AllocDitherBuffers(cx, &m_pErrBuf1, &m_pErrBuf2);
    if (FAILED(hr))
        return hr;
        
    if (bfidSrc == BFID_RGB_24)
    {
        Dith24to8( pDestBits, pSrcBits, nDestPitch, nSrcPitch, 
            prgbDestColors, pbDestInvMap, 
            m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
    }
    else if (bfidSrc == BFID_RGB_555)
    {
        Dith15to8( pDestBits, (WORD *)pSrcBits, nDestPitch, nSrcPitch,
            prgbDestColors, pbDestInvMap,
            m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
    }
    else if (bfidSrc == BFID_RGB_565)
    {
        Dith16to8( pDestBits, (WORD *)pSrcBits, nDestPitch, nSrcPitch,
            prgbDestColors, pbDestInvMap,
            m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
    }
    else if (bfidSrc == BFID_RGB_8)
    {
        if (lDestTrans == -1 || lSrcTrans == -1)
        {
            Dith8to8( pDestBits, pSrcBits, nDestPitch, nSrcPitch,
                prgbSrcColors, prgbDestColors, pbDestInvMap, 
                m_pErrBuf1, m_pErrBuf2, x, cx, y, cy );
        }
        else
        {
            Dith8to8t( pDestBits, pSrcBits, nDestPitch, nSrcPitch,
                prgbSrcColors, prgbDestColors, pbDestInvMap, 
                m_pErrBuf1, m_pErrBuf2, x, cx, y, cy, (BYTE)lDestTrans, (BYTE)lSrcTrans );
        }
    }
    else
    {
        hr = E_FAIL;
    }

    FreeDitherBuffers(m_pErrBuf1, m_pErrBuf2);
    
    return hr;
}

HRESULT AllocDitherBuffers(LONG cx, ERRBUF **ppBuf1, ERRBUF **ppBuf2)
{
    ERRBUF *pBuf;

    *ppBuf1 = *ppBuf2 = NULL;
    
    pBuf = (ERRBUF *)MemAllocClear(Mt(ImgDithBufs), (cx+2)*2*sizeof(ERRBUF));
    if (pBuf == NULL)
        return E_OUTOFMEMORY;

    *ppBuf1 = &pBuf[1];
    *ppBuf2 = &pBuf[cx+3];

    return S_OK;       
}

void FreeDitherBuffers(ERRBUF *pBuf1, ERRBUF *pBuf2)
{
    MemFree(&pBuf1[-1]);
}

HRESULT
CreateIIntDitherer(IUnknown * pUnkOuter, IUnknown **ppUnk)
{
    if (pUnkOuter != NULL)
    {
        *ppUnk = NULL;
        return(CLASS_E_NOAGGREGATION);
    }

    CIntDitherer * pDitherer = new CIntDitherer;

    *ppUnk = pDitherer;

    RRETURN(pDitherer ? S_OK : E_OUTOFMEMORY);
}

