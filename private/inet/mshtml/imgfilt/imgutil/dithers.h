#ifndef _DITHERS_H
#define _DITHERS_H

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct
{
    int r, g, b;

} ERRBUF;

__inline size_t ErrbufBytes(size_t pels)
{
    return (pels + 2) * sizeof(ERRBUF);
}

///////////////////////////////////////////////////////////////////////////////

void Dith8to8(BYTE *dst, const BYTE *src, int dst_next_scan, int src_next_scan,
    const RGBQUAD *colorsIN, const RGBQUAD *colorsOUT, const BYTE *map,
    ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx, UINT y, int cy);

void Convert8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const RGBQUAD* prgbColors, const BYTE* pbMap, UINT x, 
   UINT nWidth, UINT y, int nHeight );

void DithGray8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const RGBQUAD* prgbColors, const BYTE* pbMap,
   ERRBUF* pCurrentError, ERRBUF* pNextError, UINT x, UINT cx, UINT y, 
   int cy );

void ConvertGray8to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const BYTE* pbMap, UINT x, UINT nWidth, UINT y, 
   int nHeight );

void Dith8to8t(BYTE *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colorsIN, const RGBQUAD *colorsOUT,
    const BYTE *map, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx, UINT y,
    int cy, BYTE indexTxpOUT, BYTE indexTxpIN);

void Dith8to16(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, ERRBUF *cur_err, ERRBUF *nxt_err,
    UINT x, UINT cx, UINT y, int cy);

void Dith8to16t(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, ERRBUF *cur_err, ERRBUF *nxt_err,
    UINT x, UINT cx, UINT y, int cy, WORD wColorTxpOUT, BYTE indexTxpIN);

void Dith24to8(BYTE *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, const RGBQUAD *colors, const BYTE *map, ERRBUF *cur_err,
    ERRBUF *nxt_err, UINT x, UINT cx, UINT y, int cy);

void Convert24to8( BYTE* pbDest, const BYTE* pbSrc, int nDestPitch, 
   int nSrcPitch, const BYTE* pbMap, UINT x, UINT nWidth, UINT y, 
   int nHeight );

void Dith24to16(WORD *dst, const BYTE *src, int dst_next_scan,
    int src_next_scan, ERRBUF *cur_err, ERRBUF *nxt_err, UINT x, UINT cx,
    UINT y, int cy);

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // _DITHERS_H

