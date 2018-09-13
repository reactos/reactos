/*****************************************************************************
 *
 *  CrunchDIB  - shrink a DIB down by 2 with color averaging
 *
 *****************************************************************************/

#define _WINDOWS
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <memory.h>

//
// support huge >64k DIBs?
//
#ifndef WIN32
#define HUGE_DIBS
#endif
#ifdef HUGE_DIBS
#define LPBYTE  BYTE _huge *
#define LPWORD  WORD _huge *
#define FAR _huge
#endif

/*****************************************************************************
 *
 *  These are the standard VGA colors, we will be stuck with until the
 *  end of time!
 *
 *****************************************************************************/

static DWORD VGAColors[16] = {
     0x00000000        // 0000  black
    ,0x00800000        // 0001  dark red
    ,0x00008000        // 0010  dark green
    ,0x00808000        // 0011  mustard
    ,0x00000080        // 0100  dark blue
    ,0x00800080        // 0101  purple
    ,0x00008080        // 0110  dark turquoise
    ,0x00C0C0C0        // 1000  gray
    ,0x00808080        // 0111  dark gray
    ,0x00FF0000        // 1001  red
    ,0x0000FF00        // 1010  green
    ,0x00FFFF00        // 1011  yellow
    ,0x000000FF        // 1100  blue
    ,0x00FF00FF        // 1101  pink (magenta)
    ,0x0000FFFF        // 1110  cyan
    ,0x00FFFFFF        // 1111  white
    };

/*****************************************************************************
 *
 *  bit(b,n)   - get the Nth bit of BYTE b
 *
 *****************************************************************************/

#define bit(b,n) (BYTE)(((b) & (1 << (n))) >> (n))

/*****************************************************************************
 *
 *  SumMono
 *
 *     this routine taks four "mono" values and returns the average value.
 *
 *     ((b0) + (b1) + (b2) + (b3) >= 2)
 *
 *
 *****************************************************************************/

#define SumMono(b0,b1,b2,b3) (BYTE)(((b0) + (b1) + (b2) + (b3) + 2) / 4)

/*****************************************************************************
 *
 *  MapVGA
 *
 *     map a rgb value to a VGA index
 *
 *        0000  black
 *        0001  dark red
 *        0010  dark green
 *        0011  mustard
 *        0100  dark blue
 *        0101  purple
 *        0110  dark turquoise
 *        1000  gray
 *        0111  dark gray
 *        1001  red
 *        1010  green
 *        1011  yellow
 *        1100  blue
 *        1101  pink (magenta)
 *        1110  cyan
 *        1111  white
 *
 *****************************************************************************/

#define MapVGA(r,g,b) (((r&~3) == (g&~3) && (g&~3) == (b&~3)) ?        \
        ((r < 64) ? 0 : (r <= 128) ? 8 : (r <= 192) ? 7 : 15) :        \
        (((r>192) || (g>192) || (b>192)) ?                             \
           (((r>191) ? 1:0) | ((g>191) ? 2:0) | ((b>191) ? 4:0) | 8) : \
           (((r>64) ? 1:0) | ((g>64) ? 2:0) | ((b>64) ? 4:0))) )

/*****************************************************************************
 *
 *  SumRGB
 *
 *****************************************************************************/

#define SumRGB(b0,b1,b2,b3) RGB(\
        ((int)pal.palPalEntry[b0].peRed +        \
         (int)pal.palPalEntry[b1].peRed +        \
         (int)pal.palPalEntry[b2].peRed +        \
         (int)pal.palPalEntry[b3].peRed) >> 2,   \
                                                 \
        ((int)pal.palPalEntry[b0].peGreen +      \
         (int)pal.palPalEntry[b1].peGreen +      \
         (int)pal.palPalEntry[b2].peGreen +      \
         (int)pal.palPalEntry[b3].peGreen) >> 2, \
                                                  \
        ((int)pal.palPalEntry[b0].peBlue +       \
         (int)pal.palPalEntry[b1].peBlue +       \
         (int)pal.palPalEntry[b2].peBlue +       \
         (int)pal.palPalEntry[b3].peBlue) >> 2)

/*****************************************************************************
 *
 *  RGB16
 *
 *****************************************************************************/

typedef struct { BYTE b,g,r; } RGB24;

#define RGB16(r,g,b) (\
            (((UINT)(r) >> 3) << 10) |  \
            (((UINT)(g) >> 3) << 5)  |  \
            (((UINT)(b) >> 3) << 0)  )

#define rgb16(r,g,b) (\
            ((UINT)(r) << 10) |  \
            ((UINT)(g) << 5)  |  \
            ((UINT)(b) << 0)  )

//#define RGB16R(rgb)     ((((UINT)(rgb) >> 10) & 0x1F) * 255u / 31u)
//#define RGB16G(rgb)     ((((UINT)(rgb) >> 5)  & 0x1F) * 255u / 31u)
//#define RGB16B(rgb)     ((((UINT)(rgb) >> 0)  & 0x1F) * 255u / 31u)

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

#define Pel1(p,x)   (BYTE)bit(((LPBYTE)(p))[(x)/8],7-((x)%8))
#define Pel4(p,x)   (BYTE)((x & 1) ? (((LPBYTE)(p))[(x)/2] & 15) : (((LPBYTE)(p))[(x)/2] >> 4))
#define Pel8(p,x)   (BYTE)(((LPBYTE)(p))[(x)])
#define Pel16(p,x)  (((LPWORD)(p))[(x)])
#define Pel24(p,x)  (((RGB24 FAR *)(p))[(x)])

/*****************************************************************************
 *
 *  CrunchDIB  - shrink a DIB down by 2 with color averaging
 *
 *     this routine works on 1,4 bpp DIBs
 *
 *     for mono DIBs it is assumed they are black and white
 *
 *     for 4bpp DIBs it is assumed they use the VGA colors
 *
 *     this routine can't be used "in place"
 *
 *****************************************************************************/

BOOL CrunchDIB(
    LPBITMAPINFOHEADER  lpbiSrc,    // BITMAPINFO of source
    LPVOID              lpSrc,      // input bits to crunch
    LPBITMAPINFOHEADER  lpbiDst,    // BITMAPINFO of dest
    LPVOID              lpDst)      // output bits to crunch
{
    LPBYTE      pbSrc;
    LPBYTE      pbDst;
    LPBYTE      pb;
    LPWORD      pw;
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

    if (lpbiDst->biBitCount == 4)
        _fmemcpy(lpbiDst+1,VGAColors,sizeof(VGAColors));

    if ((int)lpbiDst->biBitCount == (int)lpbiSrc->biBitCount)
    {
        switch((int)lpbiSrc->biBitCount)
        {
        case 1:
            dx = dx / 8;    // dx is byte count

            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

                for (x=0; x<dx; x += 2)
                {
                    b0 = pbSrc[x];
                    b1 = pbSrc[x + WidthBytesSrc];

                    b = (BYTE)(
                        (SumMono(bit(b0,7), bit(b0,6), bit(b1,7), bit(b1,6)) << 7) |
                        (SumMono(bit(b0,5), bit(b0,4), bit(b1,5), bit(b1,4)) << 6) |
                        (SumMono(bit(b0,3), bit(b0,2), bit(b1,3), bit(b1,2)) << 5) |
                        (SumMono(bit(b0,1), bit(b0,0), bit(b1,1), bit(b1,0)) << 4));

                    b0 = pbSrc[x + 1];
                    b1 = pbSrc[x + 1 + WidthBytesSrc];

                    b |=(SumMono(bit(b0,7), bit(b0,6), bit(b1,7), bit(b1,6)) << 3) |
                        (SumMono(bit(b0,5), bit(b0,4), bit(b1,5), bit(b1,4)) << 2) |
                        (SumMono(bit(b0,3), bit(b0,2), bit(b1,3), bit(b1,2)) << 1) |
                        (SumMono(bit(b0,1), bit(b0,0), bit(b1,1), bit(b1,0)) << 0);

                    *pb++ = b;
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        case 4:
            dx = dx / 2;    // dx is byte count

            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

                for (x=0; x<dx; x+=2)
                {
                    b0 = pbSrc[x];
                    b1 = pbSrc[x + WidthBytesSrc];

                    rgb = SumRGB((b0 >> 4),(b0 & 0x0F),
                                 (b1 >> 4),(b1 & 0x0F));

                    b = (BYTE)(MapVGA(GetRValue(rgb),GetGValue(rgb),GetBValue(rgb)) << 4);

                    b0 = pbSrc[x + 1];
                    b1 = pbSrc[x + 1 + WidthBytesSrc];

                    rgb = SumRGB((b0 >> 4),(b0 & 0x0F),
                                 (b1 >> 4),(b1 & 0x0F));

                    b |= MapVGA(GetRValue(rgb),GetGValue(rgb),GetBValue(rgb));

                    *pb++ = b;
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;
#if 0
        case 8:
            {
            HPALETTE hpal;

            hpal = CreatePalette((LPLOGPALETTE)&pal);

            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

                for (x=0; x<dx; x+=2)
                {
                    b0 = Pel8(pbSrc,x);
                    b1 = Pel8(pbSrc+WidthBytesSrc, x);
                    b2 = Pel8(pbSrc,x+1);
                    b3 = Pel8(pbSrc+WidthBytesSrc,x+1);

                    *pb++ = (BYTE)GetNearestPaletteIndex(hpal,
                        SumRGB(b0,b1,b2,b3));
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }

            DeleteObject(hpal);
            }
            break;
#endif
        case 16:
            for (y=0; y<dy; y+=2)
            {
                pw = (LPWORD)pbDst;

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

        default:
            return FALSE;
        }
    }
    else if ((int)lpbiDst->biBitCount == 24)
    {
        switch((int)lpbiSrc->biBitCount)
        {
        case 1:
        case 4:
        case 8:
            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

		for (x=0; x<dx; x += 2)
                {
                    b0 = Pel(pbSrc,x);
                    b1 = Pel(pbSrc,x+1);
                    b2 = Pel(pbSrc+WidthBytesSrc,x);
                    b3 = Pel(pbSrc+WidthBytesSrc,x+1);

                    rgb = SumRGB(b0,b1,b2,b3);

                    *pb++ = GetBValue(rgb);
                    *pb++ = GetGValue(rgb);
                    *pb++ = GetRValue(rgb);
                }

                pbSrc += WidthBytesSrc*2;
                pbDst += WidthBytesDst;
            }
            break;

        case 16:
            for (y=0; y<dy; y+=2)
            {
                pb = pbDst;

		for (x=0; x<dx; x += 2)
                {
                    w0 = Pel16(pbSrc,x);
                    w1 = Pel16(pbSrc,x+1);
                    w2 = Pel16(pbSrc+WidthBytesSrc,x);
                    w3 = Pel16(pbSrc+WidthBytesSrc,x+1);

                    r = (RGB16R(w0) + RGB16R(w1) + RGB16R(w2) + RGB16R(w3)) / 4;
                    g = (RGB16G(w0) + RGB16G(w1) + RGB16G(w2) + RGB16G(w3)) / 4;
                    b = (RGB16B(w0) + RGB16B(w1) + RGB16B(w2) + RGB16B(w3)) / 4;

                    rgb = RGB(r,g,b);

                    *pb++ = GetBValue(rgb);
                    *pb++ = GetGValue(rgb);
                    *pb++ = GetRValue(rgb);
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
