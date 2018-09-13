/****************************** Module Header ******************************\
* Module Name: cldib.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*
* 09-26-95 ChrisWil  Ported dib-scale code.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Constants.
 */
#define FX1  65536   // 1.0 in fixed point


/*
 * Local Macros.
 */
#define BPPTOINDEX(bpp) ((UINT)(bpp) >> 3)

#define RGBQ(r,g,b) RGB(b, g, r)
#define RGBQR(rgb)  GetBValue(rgb)
#define RGBQG(rgb)  GetGValue(rgb)
#define RGBQB(rgb)  GetRValue(rgb)

#define Pel4(p,x)  (BYTE)(((x) & 1) ? (((LPBYTE)(p))[(x)/2] & 15) : (((LPBYTE)(p))[(x)/2] >> 4))
#define Pel8(p,x)  (BYTE)(((LPBYTE)(p))[(x)])
#define Pel16(p,x) (((WORD UNALIGNED *)(p))[(x)])
#define Pel24(p,x) (*(DWORD UNALIGNED *)((LPBYTE)(p) + (x) * 3))

/*
 * Function Typedefs.
 */
typedef VOID (*SCALEPROC)(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
typedef VOID (*INITPROC)(LPBITMAPINFOHEADER);

/*
 * Local Routines.
 */
BOOL     ScaleDIB(LPBITMAPINFOHEADER, LPVOID, LPBITMAPINFOHEADER, LPVOID);
VOID     InitDst8(LPBITMAPINFOHEADER);
VOID     Scale48(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
VOID     Scale88(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
VOID     Scale424(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
VOID     Scale824(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
VOID     Scale2424(LPDWORD, LPBYTE, long, int, int, int, int, LPBYTE, long, int, int);
BYTE     Map8(COLORREF);
COLORREF MixRGBI(LPDWORD, BYTE, BYTE,BYTE, BYTE, int, int);
COLORREF MixRGB(DWORD, DWORD, DWORD, DWORD, int, int);

/*
 * Globals needed for color-mapping of resources.
 */
SCALEPROC  ScaleProc[4][4] = {
    NULL,  Scale48, NULL, Scale424,
    NULL,  Scale88, NULL, Scale824,
    NULL,  NULL   , NULL, NULL,
    NULL,  NULL   , NULL, Scale2424
};

INITPROC InitDst[] = {
    NULL, InitDst8, NULL, NULL
};

BYTE rmap[256], gmap[256], bmap[256];


/***************************************************************************\
* SmartStretchDIBits
*
* calls GDI StretchDIBits, unless the stretch is 2:1 and the source
* is 4bpp, then is does it itself in a nice way.
*
* this optimization should just be put into GDI.
*
* can we change the passed bits? I assume we cant, could avoid a alloc
* if so.
*
\***************************************************************************/

int SmartStretchDIBits(
    HDC          hdc,
    int          xD,
    int          yD,
    int          dxD,
    int          dyD,
    int          xS,
    int          yS,
    int          dxS,
    int          dyS,
    LPVOID       lpBits,
    LPBITMAPINFO lpbi,
    UINT         wUsage,
    DWORD        rop)
{
    LPBYTE             lpBitsNew;
    UINT               bpp;
    RGBQUAD            rgb;
    LPBITMAPINFOHEADER lpbiNew = NULL;

    int i;

    /*
     * thunk to USER32
     */
    UserAssert(rop == SRCCOPY);
    UserAssert(wUsage == DIB_RGB_COLORS);
    UserAssert(xS == 0 && yS == 0);

    if ((GetDIBColorTable(hdc, 0, 1, &rgb) != 1) &&
        (dxD != dxS || dyD != dyS) &&           // 1:1 stretch just call GDI
        (dxD >= dxS/2) && (dyD >= dyS/2) &&     // less than 1:2 shrink call GDI
        (lpbi->bmiHeader.biCompression == 0) && // must be un-compressed
        (lpbi->bmiHeader.biBitCount == 4 ||     // input must be 4,8,24
         lpbi->bmiHeader.biBitCount == 8 ||
         lpbi->bmiHeader.biBitCount == 24)) {

        bpp = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

        bpp = (bpp > 8 ? 24 : 8);

        lpbiNew = (LPBITMAPINFOHEADER)UserLocalAlloc(0,
                 sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)) +
                 (UINT)((((dxD * bpp) / 8) + 3) & ~3) * (UINT)dyD);

        if (lpbiNew) {

            *lpbiNew            = lpbi->bmiHeader;
            lpbiNew->biWidth    = dxD;
            lpbiNew->biHeight   = dyD;
            lpbiNew->biBitCount = (WORD)bpp;
            lpBitsNew = (LPBYTE)lpbiNew          +
                        sizeof(BITMAPINFOHEADER) +
                        (256 * sizeof(RGBQUAD));

            if (ScaleDIB((LPBITMAPINFOHEADER)lpbi,
                         (LPVOID)lpBits,
                         lpbiNew,
                         lpBitsNew)) {

                lpbi   = (LPBITMAPINFO)lpbiNew;
                lpBits = lpBitsNew;
                dxS    = dxD;
                dyS    = dyD;
            }
        }
    }

    i = StretchDIBits(hdc,
                      xD,
                      yD,
                      dxD,
                      dyD,
                      xS,
                      yS,
                      dxS,
                      dyS,
                      lpBits,
                      lpbi,
                      wUsage,
                      rop);

    if (lpbiNew)
        UserLocalFree(lpbiNew);

    return i;
}

/***************************************************************************\
* ScaleDIB
*
*   Parameters
*   ----------
*   lpbiSrc - BITMAPINFO of source.
*   lpSrc   - Input bits to crunch.
*   lpbiDst - BITMAPINFO of destination.
*   lpDst   - Output bits to crunch.
*
*
\***************************************************************************/

BOOL ScaleDIB(
    LPBITMAPINFOHEADER lpbiSrc,
    LPVOID             lpSrc,
    LPBITMAPINFOHEADER lpbiDst,
    LPVOID             lpDst)
{
    LPBYTE  pbSrc;
    LPBYTE  pbDst;
    LPDWORD pal;
    int     dxSrc;
    int     dySrc;
    int     dxDst;
    int     dyDst;
    int     iSrc;
    int     iDst;
    int     x0;
    int     y0;
    int     sdx;
    int     sdy;
    LONG    WidthBytesSrc;
    LONG    WidthBytesDst;

    if ((lpbiSrc->biCompression != BI_RGB) ||
        (lpbiDst->biCompression != BI_RGB)) {
        return FALSE;
    }

    iSrc = BPPTOINDEX(lpbiSrc->biBitCount);
    iDst = BPPTOINDEX(lpbiDst->biBitCount);

    if (ScaleProc[iSrc][iDst] == NULL)
        return FALSE;

    dxSrc = (int)lpbiSrc->biWidth;
    dySrc = (int)lpbiSrc->biHeight;

    dxDst = (int)lpbiDst->biWidth;
    dyDst = (int)lpbiDst->biHeight;

    WidthBytesDst = BitmapWidth(lpbiDst->biWidth, lpbiDst->biBitCount);
    WidthBytesSrc = BitmapWidth(lpbiSrc->biWidth, lpbiSrc->biBitCount);

    lpbiDst->biSizeImage = (WidthBytesDst * dyDst);

    pbSrc = (LPBYTE)lpSrc;
    pbDst = (LPBYTE)lpDst;

    pal = (LPDWORD)(lpbiSrc + 1);

    if (InitDst[iDst])
        InitDst[iDst](lpbiDst);

    if ((dxSrc == (dxDst * 2)) && (dySrc == (dyDst * 2))) {

        sdx = FX1 * 2;
        sdy = FX1 * 2;
        y0  = FX1 / 2;
        x0  = FX1 / 2;

    } else {

        sdx = (dxSrc - 1) * FX1 / (dxDst - 1);
        sdy = (dySrc - 1) * FX1 / (dyDst - 1);
        y0  = 0;
        x0  = 0;
    }

    /*
     * Make sure we don't croak on a bad bitmap since this can hose winlogon
     * if it's the desktop wallpaper.
     */
    try {
        ScaleProc[iSrc][iDst](pal,
                              pbSrc,
                              WidthBytesSrc,
                              x0,
                              y0,
                              sdx,
                              sdy,
                              pbDst,
                              WidthBytesDst,
                              dxDst,
                              dyDst);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* Scale48
*
*
\***************************************************************************/

VOID Scale48(
    LPDWORD pal,
    LPBYTE  pbSrc,
    LONG    WidthBytesSrc,
    int     x0,
    int     y0,
    int     sdx,
    int     sdy,
    LPBYTE  pbDst,
    LONG    WidthBytesDst,
    int     dxDst,
    int     dyDst)
{
    BYTE    b0;
    BYTE    b1;
    BYTE    b2;
    BYTE    b3;
    LPBYTE  pb;
    int     x;
    int     y;
    UINT    sx;
    UINT    sy;

    for (sy=y0, y=0; y < dyDst; y++, sy+=sdy) {

        pb = pbSrc + (WidthBytesSrc * (sy / FX1));

        for (sx=x0, x=0; x < dxDst; x++, sx+=sdx) {

            b0 = Pel4(pb, (sx / FX1));
            b1 = Pel4(pb, (sx / FX1) + 1);
            b2 = Pel4(pb + WidthBytesSrc, (sx / FX1));
            b3 = Pel4(pb + WidthBytesSrc, (sx / FX1) + 1);

            pbDst[x] = Map8(MixRGBI(pal, b0, b1, b2, b3, sx % FX1, sy % FX1));
        }

        pbDst += WidthBytesDst;
    }
}

/***************************************************************************\
* Scale88
*
*
\***************************************************************************/

VOID Scale88(
    LPDWORD pal,
    LPBYTE  pbSrc,
    LONG    WidthBytesSrc,
    int     x0,
    int     y0,
    int     sdx,
    int     sdy,
    LPBYTE  pbDst,
    LONG    WidthBytesDst,
    int     dxDst,
    int     dyDst)
{
    BYTE   b0;
    BYTE   b1;
    BYTE   b2;
    BYTE   b3;
    LPBYTE pb;
    int    x;
    int    y;
    UINT   sx;
    UINT   sy;

    for (sy=y0, y=0; y < dyDst; y++, sy+=sdy) {

        pb = pbSrc + (WidthBytesSrc * (sy / FX1));

        for (sx=x0, x=0; x < dxDst; x++, sx+=sdx) {

            b0 = Pel8(pb, (sx / FX1));
            b1 = Pel8(pb, (sx / FX1) + 1);
            b2 = Pel8(pb + WidthBytesSrc, (sx / FX1));
            b3 = Pel8(pb + WidthBytesSrc, (sx / FX1) + 1);

            pbDst[x] = Map8(MixRGBI(pal, b0, b1, b2, b3, sx % FX1, sy % FX1));
        }

        pbDst += WidthBytesDst;
    }
}

/***************************************************************************\
* Scale424
*
*
\***************************************************************************/

VOID Scale424(
    LPDWORD pal,
    LPBYTE  pbSrc,
    LONG    WidthBytesSrc,
    int     x0,
    int     y0,
    int     sdx,
    int     sdy,
    LPBYTE  pbDst,
    LONG    WidthBytesDst,
    int     dxDst,
    int     dyDst)
{
    BYTE     b0;
    BYTE     b1;
    BYTE     b2;
    BYTE     b3;
    LPBYTE   pb;
    int      x;
    int      y;
    UINT     sx;
    UINT     sy;
    COLORREF rgb;

    for (sy=y0, y=0; y < dyDst; y++, sy+=sdy) {

        pb = pbSrc + (WidthBytesSrc * (sy / FX1));

        for (sx=x0, x=0; x < dxDst; x++, sx+=sdx) {

            b0 = Pel4(pb, (sx / FX1));
            b1 = Pel4(pb, (sx / FX1) + 1);
            b2 = Pel4(pb + WidthBytesSrc, (sx / FX1));
            b3 = Pel4(pb + WidthBytesSrc, (sx / FX1) + 1);

            rgb = MixRGBI(pal, b0, b1, b2, b3, sx % FX1, sy % FX1);

            *pbDst++ = GetBValue(rgb);
            *pbDst++ = GetGValue(rgb);
            *pbDst++ = GetRValue(rgb);
        }

        pbDst += (WidthBytesDst - (dxDst * 3));
    }
}

/***************************************************************************\
* Scale824
*
*
\***************************************************************************/

VOID Scale824(
    LPDWORD pal,
    LPBYTE  pbSrc,
    LONG    WidthBytesSrc,
    int     x0,
    int     y0,
    int     sdx,
    int     sdy,
    LPBYTE  pbDst,
    LONG    WidthBytesDst,
    int     dxDst,
    int     dyDst)
{
    BYTE     b0;
    BYTE     b1;
    BYTE     b2;
    BYTE     b3;
    LPBYTE   pb;
    int      x;
    int      y;
    UINT     sx;
    UINT     sy;
    COLORREF rgb;

    for (sy=y0, y=0; y < dyDst; y++, sy+=sdy) {

        pb = pbSrc + (WidthBytesSrc * (sy / FX1));

        for (sx=x0, x=0; x < dxDst; x++, sx+=sdx) {

            b0 = Pel8(pb, (sx / FX1));
            b1 = Pel8(pb, (sx / FX1) + 1);
            b2 = Pel8(pb + WidthBytesSrc, (sx / FX1));
            b3 = Pel8(pb + WidthBytesSrc, (sx / FX1) + 1);

            rgb = MixRGBI(pal, b0, b1, b2, b3, sx % FX1, sy % FX1);

            *pbDst++ = GetBValue(rgb);
            *pbDst++ = GetGValue(rgb);
            *pbDst++ = GetRValue(rgb);
        }

        pbDst += (WidthBytesDst - (dxDst * 3));
    }
}

/***************************************************************************\
* Scale2424
*
*
\***************************************************************************/

VOID Scale2424(
    LPDWORD pal,
    LPBYTE  pbSrc,
    LONG    WidthBytesSrc,
    int     x0,
    int     y0,
    int     sdx,
    int     sdy,
    LPBYTE  pbDst,
    LONG    WidthBytesDst,
    int     dxDst,
    int     dyDst)
{
    DWORD    bgr0;
    DWORD    bgr1;
    DWORD    bgr2;
    DWORD    bgr3;
    LPBYTE   pb;
    int      x;
    int      y;
    UINT     sx;
    UINT     sy;
    COLORREF rgb;

    UNREFERENCED_PARAMETER(pal);

    for (sy=y0, y=0; y < dyDst; y++, sy+=sdy) {

        pb = pbSrc + (WidthBytesSrc * (sy / FX1));

        for (sx=x0, x=0; x < dxDst; x++, sx+=sdx) {

            bgr0 = Pel24(pb, (sx / FX1));
            bgr1 = Pel24(pb, (sx / FX1) + 1);
            bgr2 = Pel24(pb + WidthBytesSrc, (sx / FX1));
            bgr3 = Pel24(pb + WidthBytesSrc, (sx / FX1) + 1);

            rgb = MixRGB(bgr0, bgr1, bgr2, bgr3, sx % FX1, sy % FX1);

            *pbDst++ = GetBValue(rgb);
            *pbDst++ = GetGValue(rgb);
            *pbDst++ = GetRValue(rgb);
        }

        pbDst += (WidthBytesDst - (dxDst * 3));
    }
}

/***************************************************************************\
* MixRGB
*
*   r0  x   r1
*   y   *
*   r2  r3
*
* Note: inputs are RGBQUADs, output is a COLORREF.
*
\***************************************************************************/

COLORREF MixRGB(
    DWORD r0,
    DWORD r1,
    DWORD r2,
    DWORD r3,
    int   x,
    int   y)
{
    int r;
    int g;
    int b;

    if (x == 0 && y == 0) {

        r = RGBQR(r0);
        g = RGBQG(r0);
        b = RGBQB(r0);

    } else if (x == 0) {

        r = ((FX1-y) * RGBQR(r0) + y * RGBQR(r2))/FX1;
        g = ((FX1-y) * RGBQG(r0) + y * RGBQG(r2))/FX1;
        b = ((FX1-y) * RGBQB(r0) + y * RGBQB(r2))/FX1;
    }
    else if (y == 0)
    {
        r = ((FX1-x) * RGBQR(r0) + x * RGBQR(r1))/FX1;
        g = ((FX1-x) * RGBQG(r0) + x * RGBQG(r1))/FX1;
        b = ((FX1-x) * RGBQB(r0) + x * RGBQB(r1))/FX1;
    }
    else if (x == FX1/2 && y == FX1/2)
    {
        r = (RGBQR(r0) + RGBQR(r1) + RGBQR(r2) + RGBQR(r3))/4;
        g = (RGBQG(r0) + RGBQG(r1) + RGBQG(r2) + RGBQG(r3))/4;
        b = (RGBQB(r0) + RGBQB(r1) + RGBQB(r2) + RGBQB(r3))/4;
    }
    else
    {
        r =((ULONG)RGBQR(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQR(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQR(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQR(r3) * x / FX1 * y       )/FX1;

        g =((ULONG)RGBQG(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQG(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQG(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQG(r3) * x / FX1 * y       )/FX1;

        b =((ULONG)RGBQB(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQB(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQB(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQB(r3) * x / FX1 * y       )/FX1;
    }

    return RGB(r,g,b);
}

/***************************************************************************\
* MixRGBI
*
*
\***************************************************************************/

_inline COLORREF MixRGBI(
    LPDWORD pal,
    BYTE    b0,
    BYTE    b1,
    BYTE    b2,
    BYTE    b3,
    int     x,
    int     y)
{
    if (((b0 == b1) && (b1 == b2) && (b2 == b3)) || ((x == 0) && (y == 0)))
        return RGBX(pal[b0]);
    else
        return MixRGB(pal[b0], pal[b1], pal[b2], pal[b3], x, y);
}

/***************************************************************************\
* InitDst8
*
*
\***************************************************************************/

_inline VOID InitDst8(
    LPBITMAPINFOHEADER lpbi)
{
    int     r;
    int     g;
    int     b;
    LPDWORD pdw;

    pdw = (LPDWORD)(lpbi + 1);
    lpbi->biClrUsed = 6*6*6;

    for (r=0; r < 6; r++) {

        for (g=0; g < 6; g++) {

            for (b=0; b < 6; b++) {
                *pdw++ = RGBQ((r * 255) / 5, (g * 255) / 5, (b * 255) / 5);
            }
        }
    }

    for (b=0; b < 256; b++) {
        bmap[b] = (b * 5 + 128) / 255;
        gmap[b] = 6 * bmap[b];
        rmap[b] = 36 * bmap[b];
    }
}

/***************************************************************************\
* Map8
*
*
\***************************************************************************/

_inline BYTE Map8(
    COLORREF rgb)
{
    return rmap[GetRValue(rgb)] +
           gmap[GetGValue(rgb)] +
           bmap[GetBValue(rgb)];
}
