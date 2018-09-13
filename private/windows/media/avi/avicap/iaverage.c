/*
 *
 * iaverage.c   Image averaging
 *
 * (C) Copyright Microsoft Corporation 1993. All rights reserved.
 *
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <win32.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <msvideo.h>

#include "ivideo32.h"
#include "iaverage.h"

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)
#define RGB16(r,g,b)      ((((WORD)(r) >> 3) << 10) | \
                          (((WORD)(g) >> 3) << 5)  | \
                          (((WORD)(b) >> 3) ) )

typedef BYTE HUGE  *   HPBYTE;
typedef WORD HUGE  *   HPWORD;
typedef DWORD HUGE *   HPDWORD;

#ifdef _WIN32
typedef RGBQUAD FAR * LPRGBQUAD;
#endif

#ifdef _WIN32
#define _fmemcpy	memcpy
#endif

/* Description:
        A sequence of images are averaged together using 16 bit
        accumulators for each of the Red, Green, and Blue components.
        The final processing step is to divide the accumulated values
        by the number of frames averaged, and transfer the results back
        into the destination DIB.

        Certain death will result if the image format is changed between
        iaverageInit and iaverageFini calls.
*/

//
// table to map a 5bit index (0-31) to a 8 bit value (0-255)
//
static BYTE aw5to8[32] = {(BYTE)-1};



/*
 *  iaverageInit
 *      Allocate memory for subsequent image averaging
 *      Return FALSE on error
 *
 */
BOOL iaverageInit (LPIAVERAGE FAR * lppia, LPBITMAPINFO lpbi, HPALETTE hPal)
{
    DWORD       dwSizeImage;
    LPIAVERAGE  lpia;
    int         i;

    *lppia = NULL;

    // Check for legal DIB formats
    if (lpbi->bmiHeader.biCompression != BI_RGB)
        return FALSE;

    if (lpbi->bmiHeader.biBitCount != 8 &&
        lpbi->bmiHeader.biBitCount != 16 &&
        lpbi->bmiHeader.biBitCount != 24 &&
        lpbi->bmiHeader.biBitCount != 32)
        return FALSE;

    //
    // init the 5bit to 8bit conversion table.
    //
    if (aw5to8[0] != 0)
        for (i=0; i<32; i++)
            aw5to8[i] = (BYTE)(i * 255 / 31);

    // Alloc memory for the image average structure
    lpia = (LPIAVERAGE) GlobalAllocPtr(GHND, sizeof (IAVERAGE));

    if (!lpia)
        return FALSE;

    // Save a copy of the header
    lpia->bi.bmiHeader = lpbi->bmiHeader;

    // and a copy of the color table and an inverse mapping table
    // if the image is 8 bit
    if (lpbi->bmiHeader.biBitCount == 8) {
        WORD r, g, b;
        LPBYTE lpB;

        hmemcpy (lpia->bi.bmiColors,
                        lpbi->bmiColors,
                        lpbi->bmiHeader.biClrUsed * sizeof (RGBQUAD));

        // Allocate and init the inverse LUT
        lpia->lpInverseMap= (LPBYTE) GlobalAllocPtr(GHND, 1024L * 32);
        lpB = lpia-> lpInverseMap;
        for (r = 0; r < 256; r += 8)
            for (g = 0; g < 256; g += 8)
                for (b = 0; b < 256; b += 8)
                    *lpB++ = (BYTE) GetNearestPaletteIndex (hPal, RGB(r,g,b));

    }

    dwSizeImage = lpbi->bmiHeader.biSizeImage;

    lpia->lpRGB = (LPWORD) GlobalAllocPtr(GHND,
					dwSizeImage * sizeof (WORD) * 3);

    if (lpia->lpRGB == 0) {
        // Allocation failed, clean up
        iaverageFini (lpia);
        return FALSE;
    }

    *lppia = lpia;

    return TRUE;
}


/*
 *  iaverageFini
 *      Free memory used for image averaging
 *      and the iaverage structure itself
 *
 */
BOOL iaverageFini (LPIAVERAGE lpia)
{
    if (lpia == NULL)
        return FALSE;

    if (lpia->lpInverseMap)
        GlobalFreePtr(lpia->lpInverseMap);
    if (lpia->lpRGB)
        GlobalFreePtr(lpia->lpRGB);

    GlobalFreePtr(lpia);

    return TRUE;
}


/*
 *  iaverageZero
 *      Zeros the accumulator
 *
 */
BOOL iaverageZero (LPIAVERAGE lpia)
{
    DWORD   dwC;
    HPWORD  hpW;

    if (lpia == NULL)
        return FALSE;

    hpW = (HPWORD) lpia->lpRGB;
    dwC = lpia->bi.bmiHeader.biSizeImage * 3;
    while (--dwC)
        *hpW++ = 0;

    lpia-> iCount = 0;

    return TRUE;
}

/*
 *  iaverageSum
 *      Add the current image into the accumulator
 *      Image format must be 16 or 24 bit RGB
 */
BOOL iaverageSum (LPIAVERAGE lpia, LPVOID lpBits)
{
    HPWORD      hpRGB;
    DWORD       dwC;
    WORD        wRGB16;
    HPWORD      hpW;
    HPBYTE      hpB;
    WORD        w;

    if (lpia == NULL)
        return FALSE;

    hpRGB   = (HPWORD) lpia->lpRGB;

    if (lpia->bi.bmiHeader.biBitCount == 8) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage; --dwC; ) {
            w = (WORD) *hpB++;
            *hpRGB++   += lpia->bi.bmiColors[w].rgbBlue;
            *hpRGB++   += lpia->bi.bmiColors[w].rgbGreen;
            *hpRGB++   += lpia->bi.bmiColors[w].rgbRed;
        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 16) {
        hpW = (HPWORD) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage / 2; --dwC; ) {
            wRGB16 = *hpW++;

            *hpRGB++  += aw5to8 [wRGB16         & 0x1f]; // b
            *hpRGB++  += aw5to8 [(wRGB16 >> 5)  & 0x1f]; // g
            *hpRGB++  += aw5to8 [(wRGB16 >> 10) & 0x1f]; // r

        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 24) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage; --dwC; ) {
            *hpRGB++  += *hpB++;
        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 32) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage / 4; --dwC; ) {
            *hpRGB++  += *hpB++; // b
            *hpRGB++  += *hpB++; // g
            *hpRGB++  += *hpB++; // r
            hpB++;
        }
    }

    lpia-> iCount++;            // Image counter

    return TRUE;
}

/*
 *  iaverageDivide
 *      Divide by the number of images captured, and xfer back into the
 *      destination DIB.
 *
 */
BOOL iaverageDivide (LPIAVERAGE lpia, LPVOID lpBits)
{
    HPWORD      hpRGB;
    WORD        r, g, b, w;
    DWORD       dwC;
    HPWORD      hpW;
    HPBYTE      hpB;

    if (lpia == NULL || lpia-> iCount == 0)
        return FALSE;

    hpRGB   = (HPWORD) lpia->lpRGB;

    if (lpia->bi.bmiHeader.biBitCount == 8) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage; --dwC; ) {
            b = *hpRGB++ / lpia-> iCount;
            g = *hpRGB++ / lpia-> iCount;
            r = *hpRGB++ / lpia-> iCount;

            w = RGB16(r,g,b) & 0x7FFF;
            *hpB++ = * (lpia->lpInverseMap + w);

        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 16) {
        hpW = (HPWORD) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage / 2; --dwC; ) {
            b = *hpRGB++ / lpia-> iCount;
            g = *hpRGB++ / lpia-> iCount;
            r = *hpRGB++ / lpia-> iCount;

            *hpW++ = RGB16 (r, g, b);
        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 24) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage; --dwC; ) {
            *hpB++ = (BYTE) (*hpRGB++  / lpia-> iCount);
        }
    }

    else if (lpia->bi.bmiHeader.biBitCount == 32) {
        hpB = (HPBYTE) lpBits;
        for (dwC = lpia->bi.bmiHeader.biSizeImage / 4; --dwC; ) {
            *hpB++ = (BYTE) (*hpRGB++ / lpia-> iCount); // b
            *hpB++ = (BYTE) (*hpRGB++ / lpia-> iCount); // g
            *hpB++ = (BYTE) (*hpRGB++ / lpia-> iCount); // r
            hpB++;
        }
    }

    return TRUE;
}

// The following appropriated from Toddla's CDIB

/*****************************************************************************
 *
 *  SumRGB
 *
 *****************************************************************************/

#define SumRGB16(b0,b1,b2,b3) (\
             ((((WORD)pal.palPalEntry[b0].peRed +         \
                (WORD)pal.palPalEntry[b1].peRed +         \
                (WORD)pal.palPalEntry[b2].peRed +         \
                (WORD)pal.palPalEntry[b3].peRed)          \
                & 0x03E) << 5) |                          \
                                                          \
             ((((WORD)pal.palPalEntry[b0].peGreen +       \
                (WORD)pal.palPalEntry[b1].peGreen +       \
                (WORD)pal.palPalEntry[b2].peGreen +       \
                (WORD)pal.palPalEntry[b3].peGreen)        \
                & 0x003E)) |                              \
                                                          \
             ((((WORD)pal.palPalEntry[b0].peBlue +        \
                (WORD)pal.palPalEntry[b1].peBlue +        \
                (WORD)pal.palPalEntry[b2].peBlue +        \
                (WORD)pal.palPalEntry[b3].peBlue)         \
                & 0x003E) >> 5) )

/*****************************************************************************
 *
 *  RGB16
 *
 *****************************************************************************/

typedef struct { BYTE b,g,r; } RGB24;

#define rgb16(r,g,b) (\
            ((UINT)(r) << 10) |  \
            ((UINT)(g) << 5)  |  \
            ((UINT)(b) << 0)  )

#define RGB16R(rgb)     aw5to8[((UINT)(rgb) >> 10) & 0x1F]
#define RGB16G(rgb)     aw5to8[((UINT)(rgb) >> 5)  & 0x1F]
#define RGB16B(rgb)     aw5to8[((UINT)(rgb) >> 0)  & 0x1F]
#define RGB16r(rgb)     ((BYTE)((UINT)(rgb) >> 10) & 0x1F)
#define RGB16g(rgb)     ((BYTE)((UINT)(rgb) >> 5)  & 0x1F)
#define RGB16b(rgb)     ((BYTE)((UINT)(rgb) >> 0)  & 0x1F)

/*****************************************************************************
 *
 *  Pel() used for 24bit Crunch
 *
 *****************************************************************************/

#define Pel(p,x) (BYTE)(BitCount == 1 ? Pel1(p,x) : \
                        BitCount == 4 ? Pel4(p,x) : Pel8(p,x))

#define Pel1(p,x)   (BYTE)bit(((HPBYTE)(p))[(x)/8],7-((x)%8))
#define Pel4(p,x)   (BYTE)((x & 1) ? (((HPBYTE)(p))[(x)/2] & 15) : (((HPBYTE)(p))[(x)/2] >> 4))
#define Pel8(p,x)   (BYTE)(((HPBYTE)(p))[(x)])
#define Pel16(p,x)  (((HPWORD)(p))[(x)])
#define Pel24(p,x)  (((RGB24 HUGE *)(p))[(x)])

/*****************************************************************************
 *
 *  CrunchDIB  - shrink a DIB down by 2 with color averaging
 *
 *     this routine works on 8, 16, 24, and 32 bpp DIBs
 *
 *     this routine can't be used "in place"
 *
 *****************************************************************************/

BOOL CrunchDIB(
    LPIAVERAGE lpia,                // image averaging structure
    LPBITMAPINFOHEADER  lpbiSrc,    // BITMAPINFO of source
    LPVOID              lpSrc,      // input bits to crunch
    LPBITMAPINFOHEADER  lpbiDst,    // BITMAPINFO of dest
    LPVOID              lpDst)      // output bits to crunch
{
    HPBYTE      pbSrc;
    HPBYTE      pbDst;
    HPBYTE      pb;
    HPWORD      pw;
    BYTE        r,g,b,b0,b1,b2,b3;
    WORD        w0,w1,w2,w3;
    RGB24       rgb0,rgb1,rgb2,rgb3;
    int         WidthBytesSrc;
    int         WidthBytesDst;
    UINT        x;
    UINT        y;
    UINT        dx;
    UINT        dy;
    int         i;
    COLORREF    rgb;
    int         BitCount;
    UINT        aw5to8[32];

     struct {
        WORD         palVersion;
	WORD         palNumEntries;
	PALETTEENTRY palPalEntry[256];
    }   pal;

   if (lpbiSrc->biCompression != BI_RGB)
        return FALSE;

    BitCount = (int)lpbiSrc->biBitCount;

    if (BitCount == 16)
        for (i=0; i<32; i++)
            aw5to8[i] = (UINT)i * 255u / 31u;

    dx = (int)lpbiDst->biWidth;
    WidthBytesDst = (((UINT)lpbiDst->biBitCount * dx + 31)&~31) / 8;

    dy = (int)lpbiSrc->biHeight;
    dx = (int)lpbiSrc->biWidth;
    WidthBytesSrc = (((UINT)lpbiSrc->biBitCount * dx + 31)&~31) / 8;

    dx &= ~1;
    dy &= ~1;

    pbSrc = lpSrc;
    pbDst = lpDst;

    if (lpbiSrc->biClrUsed == 0 && lpbiSrc->biBitCount <= 8)
        lpbiSrc->biClrUsed = (1 << (int)lpbiSrc->biBitCount);

    pal.palVersion = 0x300;
    pal.palNumEntries = (int)lpbiSrc->biClrUsed;

    for (i=0; i<(int)pal.palNumEntries; i++)
    {
        pal.palPalEntry[i].peRed   = ((LPRGBQUAD)(lpbiSrc+1))[i].rgbRed;
        pal.palPalEntry[i].peGreen = ((LPRGBQUAD)(lpbiSrc+1))[i].rgbGreen;
        pal.palPalEntry[i].peBlue  = ((LPRGBQUAD)(lpbiSrc+1))[i].rgbBlue;
        pal.palPalEntry[i].peFlags = 0;
    }

    if (lpbiDst->biBitCount == 8)
        _fmemcpy(lpbiDst+1,lpbiSrc+1,(int)lpbiSrc->biClrUsed*sizeof(RGBQUAD));

    if ((int)lpbiDst->biBitCount == (int)lpbiSrc->biBitCount)
    {
        switch((int)lpbiSrc->biBitCount)
        {
        case 8:
            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

                for (x=0; x<dx; x+=2)
                {
                    b0 = Pel8(pbSrc,x);
                    b1 = Pel8(pbSrc+WidthBytesSrc, x);
                    b2 = Pel8(pbSrc,x+1);
                    b3 = Pel8(pbSrc+WidthBytesSrc,x+1);

                    r = (BYTE) ((
                        (WORD)pal.palPalEntry[b0].peRed +
                        (WORD)pal.palPalEntry[b1].peRed +
                        (WORD)pal.palPalEntry[b2].peRed +
                        (WORD)pal.palPalEntry[b3].peRed) >> 2);

                    g = (BYTE) ((
                        (WORD)pal.palPalEntry[b0].peGreen +
                        (WORD)pal.palPalEntry[b1].peGreen +
                        (WORD)pal.palPalEntry[b2].peGreen +
                        (WORD)pal.palPalEntry[b3].peGreen) >> 2);

                    b = (BYTE) ((
                        (WORD)pal.palPalEntry[b0].peBlue +
                        (WORD)pal.palPalEntry[b1].peBlue +
                        (WORD)pal.palPalEntry[b2].peBlue +
                        (WORD)pal.palPalEntry[b3].peBlue) >> 2);

                    *pb++ = (BYTE)(*(lpia->lpInverseMap +
                                RGB16 (r, g, b)));
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        case 16:
            for (y=0; y<dy; y+=2)
            {
                pw = (HPWORD)pbDst;

		for (x=0; x<dx; x += 2)
                {
                    w0 = Pel16(pbSrc,x);
                    w1 = Pel16(pbSrc,x+1);
                    w2 = Pel16(pbSrc+WidthBytesSrc,x);
                    w3 = Pel16(pbSrc+WidthBytesSrc,x+1);

                    r = ((BYTE)RGB16r(w0) + RGB16r(w1) + RGB16r(w2) + RGB16r(w3)) >> 2;
                    g = ((BYTE)RGB16g(w0) + RGB16g(w1) + RGB16g(w2) + RGB16g(w3)) >> 2;
                    b = ((BYTE)RGB16b(w0) + RGB16b(w1) + RGB16b(w2) + RGB16b(w3)) >> 2;

                    *pw++ = rgb16(r,g,b);
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        case 24:
            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

		for (x=0; x<dx; x += 2)
                {
                    rgb0 = Pel24(pbSrc,x);
                    rgb1 = Pel24(pbSrc,x+1);
                    rgb2 = Pel24(pbSrc+WidthBytesSrc,x);
                    rgb3 = Pel24(pbSrc+WidthBytesSrc,x+1);

                    rgb = RGB(
                        ((UINT)rgb0.r + rgb1.r + rgb2.r + rgb3.r)/4,
                        ((UINT)rgb0.g + rgb1.g + rgb2.g + rgb3.g)/4,
                        ((UINT)rgb0.b + rgb1.b + rgb2.b + rgb3.b)/4);

                    *pb++ = GetBValue(rgb);
                    *pb++ = GetGValue(rgb);
                    *pb++ = GetRValue(rgb);
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        case 32:
            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

		for (x=0; x<dx; x += 2)
                {
                    rgb0 = Pel24(pbSrc,x);
                    rgb1 = Pel24(pbSrc,x+1);
                    rgb2 = Pel24(pbSrc+WidthBytesSrc,x);
                    rgb3 = Pel24(pbSrc+WidthBytesSrc,x+1);

                    rgb = RGB(
                        ((UINT)rgb0.r + rgb1.r + rgb2.r + rgb3.r)/4,
                        ((UINT)rgb0.g + rgb1.g + rgb2.g + rgb3.g)/4,
                        ((UINT)rgb0.b + rgb1.b + rgb2.b + rgb3.b)/4);

                    *pb++ = GetBValue(rgb);
                    *pb++ = GetGValue(rgb);
                    *pb++ = GetRValue(rgb);
                    pb++;
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        default:
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}
