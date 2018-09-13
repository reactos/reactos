//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       imgutil.cxx
//
//  Contents:   Utilities for imaging code
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_DITHERS_H_
#define X_DITHERS_H_
#include "dithers.h"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_IMGBITS_HXX_
#define X_IMGBITS_HXx_
#include "imgbits.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif


#undef NEW_DITHERER
#define whSlop   1      // amount of difference between element width or height and image
                        // width or height within which we will blt the image in its native
                        // proportions. For now, this applies only to GIF images.

DeclareTag(tagForce4BPP,     "Dwn", "Img: Force 4-bit-per-pixel color mode");

MtDefine(ImgDithData, Dwn, "CreateDitherData")
MtDefine(DIBSection, WorkingSet, "Bitmaps")

int         g_colorModeDefault = 0;

WORD        g_wIdxTrans;
WORD        g_wIdxBgColor;
WORD        g_wIdxFgColor;
COLORREF    g_crBgColor;
COLORREF    g_crFgColor;
RGBQUAD     g_rgbBgColor;
RGBQUAD     g_rgbFgColor;

#ifdef UNIX
PALETTEENTRY g_peVga[16] =
{
    { 0x00, 0x00, 0x00, 0x00 }, // Black
    { 0x00, 0x00, 0x00, 0x80 }, // Dark red
    { 0x00, 0x00, 0x80, 0x00 }, // Dark green
    { 0x00, 0x00, 0x80, 0x80 }, // Dark yellow
    { 0x00, 0x80, 0x00, 0x00 }, // Dark blue
    { 0x00, 0x80, 0x00, 0x80 }, // Dark purple
    { 0x00, 0x80, 0x80, 0x00 }, // Dark aqua
    { 0x00, 0xC0, 0xC0, 0xC0 }, // Light grey
    { 0x00, 0x80, 0x80, 0x80 }, // Dark grey
    { 0x00, 0x00, 0x00, 0xFF }, // Light red
    { 0x00, 0x00, 0xFF, 0x00 }, // Light green
    { 0x00, 0x00, 0xFF, 0xFF }, // Light yellow
    { 0x00, 0xFF, 0x00, 0x00 }, // Light blue
    { 0x00, 0xFF, 0x00, 0xFF }, // Light purple
    { 0x00, 0xFF, 0xFF, 0x00 }, // Light aqua
    { 0x00, 0xFF, 0xFF, 0xFF }  // White
};
#else
PALETTEENTRY g_peVga[16] =
{
    { 0x00, 0x00, 0x00, 0x00 }, // Black
    { 0x80, 0x00, 0x00, 0x00 }, // Dark red
    { 0x00, 0x80, 0x00, 0x00 }, // Dark green
    { 0x80, 0x80, 0x00, 0x00 }, // Dark yellow
    { 0x00, 0x00, 0x80, 0x00 }, // Dark blue
    { 0x80, 0x00, 0x80, 0x00 }, // Dark purple
    { 0x00, 0x80, 0x80, 0x00 }, // Dark aqua
    { 0xC0, 0xC0, 0xC0, 0x00 }, // Light grey
    { 0x80, 0x80, 0x80, 0x00 }, // Dark grey
    { 0xFF, 0x00, 0x00, 0x00 }, // Light red
    { 0x00, 0xFF, 0x00, 0x00 }, // Light green
    { 0xFF, 0xFF, 0x00, 0x00 }, // Light yellow
    { 0x00, 0x00, 0xFF, 0x00 }, // Light blue
    { 0xFF, 0x00, 0xFF, 0x00 }, // Light purple
    { 0x00, 0xFF, 0xFF, 0x00 }, // Light aqua
    { 0xFF, 0xFF, 0xFF, 0x00 }  // White
};
#endif

#define MASK565_0   0x0000F800
#define MASK565_1   0x000007E0
#define MASK565_2   0x0000001F



void *pCreateDitherData(int xsize)
{
    UINT    cdw = (xsize + 1);
    DWORD * pdw = (DWORD *)MemAlloc(Mt(ImgDithData), cdw * sizeof(DWORD));

    if (pdw)
    {
        pdw += cdw;
        while (cdw-- > 0) *--pdw = 0x909090;
    }

    return(pdw);
}

// Mask off the high byte for comparing PALETTEENTRIES, RGBQUADS, etc.
#define RGBMASK(pe)    (*((DWORD *)&(pe)) & 0x00FFFFFF)

int x_ComputeConstrainMap(int cEntries, PALETTEENTRY *pcolors, int transparent, int *pmapconstrained)
{
    int i;
    int nDifferent = 0;

    for (i = 0; i < cEntries; i++)
    {
        if (i != transparent)
        {
            pmapconstrained[i] = RGB2Index(pcolors[i].peRed, pcolors[i].peGreen, pcolors[i].peBlue);
            
            if (RGBMASK(pcolors[i]) != RGBMASK(g_lpHalftone.ape[pmapconstrained[i]]))
                ++nDifferent;
        }
    }

    // Turns out the transparent index can be outside the color set.  In this
    // case we still want to map the transparent index correctly.

    if (transparent >= 0 && transparent <= 255)
    {
        pmapconstrained[transparent] = g_wIdxTrans;
    }

    return nDifferent;
}

/*
    constrains colors to 6X6X6 cube we use
*/
void x_ColorConstrain(unsigned char HUGEP *psrc, unsigned char HUGEP *pdst, int *pmapconstrained, long xsize)
{
    int x;

    for (x = 0; x < xsize; x++)
    {
        *pdst++ = (BYTE)pmapconstrained[*psrc++];
    }
}

void x_DitherRelative(BYTE *pbSrc, BYTE * pbDst, PALETTEENTRY *pe,
    int xsize, int ysize, int transparent, int *v_rgb_mem,
    int yfirst, int ylast)
{
    RGBQUAD argb[256];
    int cbScan;

    cbScan = (xsize + 3) & ~3;
    pbSrc  = pbSrc + cbScan * (ysize - yfirst - 1);
    pbDst  = pbDst + cbScan * (ysize - yfirst - 1);

    CopyColorsFromPaletteEntries(argb, pe, 256);
    
    DitherTo8( pbDst, -cbScan, 
                   pbSrc, -cbScan, BFID_RGB_8, 
                   g_rgbHalftone, argb,
                   g_pInvCMAP,
                   0, yfirst, xsize, ylast - yfirst + 1,
                   g_wIdxTrans, transparent);
}

HRESULT x_Dither(unsigned char *pdata, PALETTEENTRY *pe, int xsize, int ysize, int transparent)
{
    x_DitherRelative(pdata, pdata, pe, xsize, ysize, transparent, NULL, 
                        0, ysize - 1);
    
    return S_OK;
}

#ifdef OLDIMAGECODE // replaced by CImgBitsDIB::StretchBlt

DeclareTag(tagNoMaskBlt,     "Dwn", "Img: Don't use MaskBlt");
DeclareTag(tagTimeBltDib,    "Dwn", "Img: Measure BltDib (hold shift key down)");

void ImgBltDib(HDC hdc, HBITMAP hbmDib, HBITMAP hbmMask, LONG lTrans,
    RECT * prcDst, RECT * prcSrc, LONG yDibBot, LONG xDibWid, LONG yDibHei, DWORD dwRop)
{
    int         xDst            = prcDst->left;
    int         yDst            = prcDst->top;
    int         xDstWid         = prcDst->right - xDst;
    int         yDstHei         = prcDst->bottom - yDst;
    int         xSrc            = prcSrc->left;
    int         ySrc            = prcSrc->top;
    int         xSrcWid         = prcSrc->right - xSrc;
    int         ySrcHei         = prcSrc->bottom - ySrc;
    RGBQUAD     rgbBlack        = { 0, 0, 0, 0};
    RGBQUAD     rgbWhite        = { 255, 255, 255, 0 };
    HDC         hdcDib          = NULL;
    HBITMAP     hbmSav          = NULL;
    int         cSetColors      = 0;
    void *      pvSav           = NULL;
    UINT        cbSav           = 0;
    BOOL        fCritical       = FALSE;
    BYTE        abSav[sizeof(RGBQUAD)*2];
    RGBQUAD     argbOld[256];
    RGBQUAD     argbNew[256];
    BOOL        fPrinter = (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER);
    BOOL        fTrans = TRUE;

    if (    yDibBot == 0
        ||  xDstWid <= 0 || xSrcWid <= 0 || xDibWid <= 0
        ||  yDstHei <= 0 || ySrcHei <= 0 || yDibHei <= 0)
        return;

    #if DBG==1
    __int64     t1, t2, t3, t4, tfrq;
    BOOL        fTransBlt = 0;
    DIBSECTION  ds;
    Verify(GetObject(hbmDib, sizeof(DIBSECTION), &ds));
    Assert(xDibWid == ds.dsBmih.biWidth && yDibHei == ds.dsBmih.biHeight);
    QueryPerformanceFrequency((LARGE_INTEGER *)&tfrq);
    #endif

    // If the caller is attempting to show the bits which have not yet
    // been decoded, limit the source and dest rectangles to the visible
    // area only.

    if (yDibBot > 0 && yDibBot < yDibHei)
        yDibHei = yDibBot;

    if (xSrc < 0)
    {
        xDst += MulDivQuick(-xSrc, xDstWid, xSrcWid);
        xDstWid = prcDst->right - xDst;
        xSrcWid += xSrc;
        xSrc = 0;
        if (xDstWid <=0 || xSrcWid <= 0)
            return;
    }
    if (ySrc < 0)
    {
        yDst += MulDivQuick(-ySrc, yDstHei, ySrcHei);
        yDstHei = prcDst->bottom - yDst;
        ySrcHei += ySrc;
        ySrc = 0;
        if (yDstHei <=0 || ySrcHei <= 0)
            return;
    }

    if (xSrc + xSrcWid > xDibWid)
    {
        xDstWid = MulDivQuick(xDstWid, xDibWid - xSrc, xSrcWid);
        xSrcWid = xDibWid - xSrc;
        if (xDstWid <= 0 || xSrcWid <= 0)
            return;
    }
    if (ySrc + ySrcHei > yDibHei)
    {
        yDstHei = MulDivQuick(yDstHei, yDibHei - ySrc, ySrcHei);
        ySrcHei = yDibHei - ySrc;
        if (yDstHei <= 0 || ySrcHei <= 0)
            return;
    }

    hdcDib = GetMemoryDC();

    if (hdcDib == NULL)
        return;

    hbmSav = (HBITMAP)SelectObject(hdcDib, hbmDib);

    if (hbmSav == NULL)
        goto Cleanup;

    SetStretchBltMode(hdc, COLORONCOLOR);

#ifndef WIN16
    if (fPrinter)
    {
        int iEscapeFunction = POSTSCRIPT_PASSTHROUGH;
        THREADSTATE *   pts = GetThreadState();

        // Filter out printers that we know lie about their support for transparency.
        fTrans = (!pts || !(pts->dwPrintMode & PRINTMODE_NO_TRANSPARENCY))
            && !Escape(hdc, QUERYESCSUPPORT, sizeof(int), (LPCSTR) &iEscapeFunction, NULL);
        // fTrans = FALSE for a postscript driver
    }
#endif // ndef WIN16

    if (hbmMask)
    {
        if (fTrans)
        {
            #if DBG==1
            if (IsTagEnabled(tagNoMaskBlt)) ; else
            #endif

#ifndef WIN16
            if (    g_dwPlatformID == VER_PLATFORM_WIN32_NT
                &&  xSrcWid == xDstWid && ySrcHei == yDstHei)
            {
                #if DBG==1
                QueryPerformanceCounter((LARGE_INTEGER *)&t1);
                #endif

                MaskBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                     hdcDib, xSrc, ySrc, hbmMask, xSrc, ySrc, 0xAACC0020);

                #if DBG==1 && !defined(WIN16)
                QueryPerformanceCounter((LARGE_INTEGER *)&t2);
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    TraceTag((tagTimeBltDib, "MaskBlt (%ldx%ld) took %ld us",
                        xDstWid, yDstHei, ((LONG)(((t2 - t1) * 1000000) / tfrq))));
                #endif

                goto Cleanup;
            }
#endif //ndef WIN16

            if (!SelectObject(hdcDib, hbmMask))
                goto Cleanup;

            #if DBG==1
            QueryPerformanceCounter((LARGE_INTEGER *)&t1);
            #endif

            StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                    hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, SRCPAINT);
            dwRop = SRCAND;

            #if DBG==1
            QueryPerformanceCounter((LARGE_INTEGER *)&t2);
            fTransBlt = 1;
            #endif

            if (SelectObject(hdcDib, hbmDib) != hbmMask)
                goto Cleanup;
        }
    }
    else if (lTrans >= 0)
    {
        // WINCE - handled with TransparentImage, below.
        #if !defined(WINCE) || defined(WINCE_NT)
        g_csImgTransBlt.Enter();
        fCritical = TRUE;

        Verify(GetDIBColorTable(hdcDib, 0, 256, argbOld) > 0);

        RGBQUAD * prgb = argbNew;
        for (int c = 256; c-- > 0; )
            *prgb++ = rgbWhite;
        argbNew[lTrans] = rgbBlack;

        pvSav = &argbOld[lTrans];
        cbSav = sizeof(RGBQUAD);
        *(RGBQUAD *)abSav = *(RGBQUAD *)pvSav;
        *(RGBQUAD *)pvSav = rgbWhite;

        if (fTrans)
        {
            Verify(SetDIBColorTable(hdcDib, 0, 256, argbNew) == 256);

            cSetColors = 256;

            #if DBG==1
            QueryPerformanceCounter((LARGE_INTEGER *)&t1);
            #endif

            StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                    hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, SRCPAINT);
            dwRop = SRCAND;

            #if DBG==1
            QueryPerformanceCounter((LARGE_INTEGER *)&t2);
            fTransBlt = 2;
            #endif

            Verify(SetDIBColorTable(hdcDib, 0, 256, argbOld) == 256);
        }
        #else
            TransparentImage(hdc, xDst, yDst, xDstWid, yDstHei,
                             hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, PALETTEINDEX(lTrans));
        #endif
    }

    #if DBG==1 && !defined(WIN16) && !defined(NO_PERFDBG)
    if (hbmMask || lTrans < 0)
        IsIdentityBlt(hdc, hdcDib, ds.dsBmih.biWidth);
    QueryPerformanceCounter((LARGE_INTEGER *)&t3);
    #endif

    if (fPrinter && ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
#ifdef UNIX
                  || (g_dwPlatformID == VER_PLATFORM_WIN32_UNIX)
#endif
        ))
    {
        DIBSECTION dsPrint;

        if (GetObject(hbmDib, sizeof(DIBSECTION), &dsPrint))
        {
            struct
            {
                BITMAPINFOHEADER bmih;
                RGBQUAD argb[256];
            } bmi;

            bmi.bmih = dsPrint.dsBmih;
            GetDIBColorTable(hdcDib, 0, 256, bmi.argb);

            Assert(bmi.bmih.biHeight > 0);

            StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                          xSrc, bmi.bmih.biHeight - prcSrc->bottom, xSrcWid, ySrcHei,
                          dsPrint.dsBm.bmBits, (BITMAPINFO *) &bmi, DIB_RGB_COLORS, dwRop);
        }
    }
    else
    // WINCE - Transparent images were handled above with the new TransparentImage() api,
    //         so we don't have to do the second Blt with the SRCAND rop here.
	#if defined(WINCE) && !defined(WINCE_NT)
        if (lTrans < 0)
    #endif
		{
			StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
					   hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, dwRop);
		}

    #if DBG==1 && !defined(WIN16)
    QueryPerformanceCounter((LARGE_INTEGER *)&t4);

    if (IsTagEnabled(tagTimeBltDib) && (GetAsyncKeyState(VK_SHIFT) & 0x8000))
    {
        if (fTransBlt)
            TraceTag((tagTimeBltDib, "TransBlt (%s) (%ldx%ld) took %ld+%ld=%ld us",
                fTransBlt == 1 ? "fast" : "slow", xDstWid, yDstHei,
                ((LONG)(((t2 - t1) * 1000000) / tfrq)),
                ((LONG)(((t4 - t3) * 1000000) / tfrq)),
                ((LONG)(((t2 - t1 + t4 - t3) * 1000000) / tfrq))));
        else
            TraceTag((tagTimeBltDib, "DibBlt (%ldx%ld) took %ld us",
                xDstWid, yDstHei, ((LONG)(((t4 - t3) * 1000000) / tfrq))));
    }
    #endif

Cleanup:
    if (cbSav)
    {
        memcpy(pvSav, abSav, cbSav);
    }

    if (cSetColors)
    {
        Verify(SetDIBColorTable(hdcDib, 0, cSetColors, argbOld) == (UINT)cSetColors);
    }

    if (hbmSav)
        SelectObject(hdcDib, hbmSav);
    if (hdcDib)
        ReleaseMemoryDC(hdcDib);

    if (fCritical)
        g_csImgTransBlt.Leave();
}

#endif

/*****************************************************************/
/*****************************************************************/
/*****************************************************************/

static int CALLBACK VgaPenCallback(void * pvLogPen, LPARAM lParam)
{
    LOGPEN * pLogPen = (LOGPEN *)pvLogPen;

    if (pLogPen->lopnStyle == PS_SOLID)
    {
        PALETTEENTRY ** pppe = (PALETTEENTRY **)lParam;
        PALETTEENTRY * ppe = (*pppe)++;
        COLORREF cr = pLogPen->lopnColor;

        if (cr != *(DWORD *)ppe)
        {
            TraceTag((tagPerf, "Updating VGA color %d to %08lX",
                ppe - g_peVga, cr));
            *(DWORD *)ppe = cr;
        }

        return(ppe < &g_peVga[15]);
    }

    return(1);
}

// This function differentiates between "555" and "565" 16bpp color modes, returning 15 and 16, resp.

int GetRealColorMode(HDC hdc)
{
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[256];
            DWORD   dwMasks[3];
        } u;
    } bmi;
    HBITMAP hbm;

    hbm = CreateCompatibleBitmap(hdc, 1, 1);
    if (hbm == NULL)
        return 0;

    // NOTE: The two calls to GetDIBits are INTENTIONAL.  Don't muck with this!
    bmi.bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biBitCount = 0;
    GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);
    GetDIBits(hdc, hbm, 0, 1, NULL, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

    DeleteObject(hbm);

    if (bmi.bmih.biBitCount != 16)
        return bmi.bmih.biBitCount;

    if (bmi.bmih.biCompression != BI_BITFIELDS)
        return 15;
    
    if (bmi.u.dwMasks[0] == MASK565_0 
        && bmi.u.dwMasks[1] == MASK565_1 
        && bmi.u.dwMasks[2] == MASK565_2)
        return 16;
    else
        return 15;
}

BOOL InitImageUtil()
{
    /* Snoop around system and determine its capabilities.
     * Initialize data structures accordingly.
     */
    HDC hdc;

    HPALETTE hPal;

    hdc = GetDC(NULL);

    g_colorModeDefault = GetRealColorMode(hdc);

#ifndef UNIX
    g_crBgColor = GetSysColorQuick(COLOR_WINDOW) & 0xFFFFFF;
    g_crFgColor = GetSysColorQuick(COLOR_WINDOWTEXT) & 0xFFFFFF;
#else
    g_crBgColor = GetSysColorQuick(COLOR_WINDOW);
    g_crFgColor = GetSysColorQuick(COLOR_WINDOWTEXT);
#endif
    g_rgbBgColor.rgbRed   = GetRValue(g_crBgColor);
    g_rgbBgColor.rgbGreen = GetGValue(g_crBgColor);
    g_rgbBgColor.rgbBlue  = GetBValue(g_crBgColor);
    g_rgbFgColor.rgbRed   = GetRValue(g_crFgColor);
    g_rgbFgColor.rgbGreen = GetGValue(g_crFgColor);
    g_rgbFgColor.rgbBlue  = GetBValue(g_crFgColor);
    
#ifdef UNIX
    // For Motif colors which have a special flag in high byte
    g_rgbBgColor.rgbReserved = ((BYTE)(( g_crBgColor )>>24));
    g_rgbFgColor.rgbReserved = ((BYTE)(( g_crFgColor )>>24));
#endif

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        hPal = g_hpalHalftone;

        /*
            Now the extra colors
        */

        g_wIdxBgColor = (WORD)GetNearestPaletteIndex(hPal, g_crBgColor);
        g_wIdxFgColor = (WORD)GetNearestPaletteIndex(hPal, g_crFgColor);

        /*
            Choose a transparent color that lies outside of 6x6x6 cube - we will
            replace the actual color for this right before drawing.  We are going
            to use one of the "magic colors" in the static 20 for the transparent
            color. 
        */
        g_wIdxTrans = 246;  
    }

    if (g_colorModeDefault == 4)
    {
        PALETTEENTRY * ppe = g_peVga;
        EnumObjects(hdc, OBJ_PEN, VgaPenCallback, (LONG_PTR)&ppe);
        Assert(ppe == &g_peVga[16]);
    }

    ReleaseDC(NULL, hdc);

    return TRUE;
}

/*
    Some of the extra colors in our palette may deliberately be set to
    windows system colors, so we can simulate things like transparent
    bitmaps.  If the windows system colors change, then we made need to
    fix the corresponding palette entries.
*/
#if 0
//$BUGBUG (dinartem) Need call when fg/bg colors change
void GTR_FixExtraPaletteColors(void)
{
    COLORREF color;
    HDC hDC = GetDC(NULL);
    HPALETTE hPal = CreateHalftonePalette(hDC);

    color = PREF_GetBackgroundColor();
    colorIdxBg = GetNearestPaletteIndex(hPal,color);
    color = PREF_GetForegroundColor();
    colorIdxFg = GetNearestPaletteIndex(hPal,color);

    DeleteObject(hPal);
    ReleaseDC(NULL, hDC);
}
#endif

int GetDefaultColorMode()
{
    if (g_colorModeDefault == 0)
    {
        InitImageUtil();
    }

    #if DBG==1
    if (IsTagEnabled(tagForce4BPP))
        return(4);
    #endif

    return(g_colorModeDefault);
}

void FreeGifAnimData(GIFANIMDATA * pgad, CImgBitsDIB *pibd)
{
    GIFFRAME * pgf, * pgfNext;

    if (pgad == NULL)
        return;

    for (pgf = pgad->pgf; pgf != NULL; pgf = pgfNext)
    {
        if (pgf->pibd != pibd)
            delete pgf->pibd;
        if (pgf->hrgnVis)
            Verify(DeleteRgn(pgf->hrgnVis));
        pgfNext = pgf->pgfNext;
        MemFree(pgf);
    }
    pgad->pgf = NULL;
}

void CalcStretchRect(RECT * prectStretch, LONG wImage, LONG hImage, LONG wDisplayedImage, LONG hDisplayedImage, GIFFRAME * pgf)
{
    // set ourselves up for a stretch if the element width doesn't match that of the image

    if ((wDisplayedImage >= pgf->width - whSlop) &&
        (wDisplayedImage <= pgf->width + whSlop))
    {
        wDisplayedImage = pgf->width;
    }

    if ((hDisplayedImage >= pgf->height - whSlop) &&
        (hDisplayedImage <= pgf->height + whSlop))
    {
        hDisplayedImage = pgf->height;
    }

    if (wImage != 0)
    {
        prectStretch->left = MulDivQuick(pgf->left, wDisplayedImage, wImage);
        prectStretch->right = prectStretch->left +
                              MulDivQuick(pgf->width, wDisplayedImage, wImage);
    }
    else
    {
        prectStretch->left = prectStretch->right = pgf->left;
    }

    if (hImage != 0)
    {
        prectStretch->top = MulDivQuick(pgf->top, hDisplayedImage, hImage);
        prectStretch->bottom = prectStretch->top +
                               MulDivQuick(pgf->height, hDisplayedImage, hImage);
    }
    else
    {
        prectStretch->top = prectStretch->bottom = pgf->top;
    }
}

void getPassInfo(int logicalRowX, int height, int *pPassX, int *pRowX, int *pBandX)
{
    int passLow, passHigh, passBand;
    int pass = 0;
    int step = 8;
    int ypos = 0;

    if (logicalRowX >= height)
        logicalRowX = height - 1;
    passBand = 8;
    passLow = 0;
    while (step > 1)
    {
        if (pass == 3)
            passHigh = height - 1;
        else
            passHigh = (height - 1 - ypos) / step + passLow;
        if (logicalRowX >= passLow && logicalRowX <= passHigh)
        {
            *pPassX = pass;
            *pRowX = ypos + (logicalRowX - passLow) * step;
            *pBandX = passBand;
            return;
        }
        if (pass++ > 0)
            step /= 2;
        ypos = step / 2;
        passBand /= 2;
        passLow = passHigh + 1;
    }
}

CImgBits *GetPlaceHolderBitmap(BOOL fMissing)
{
    CImgBits **ppImgBits;
    CImgBitsDIB *pibd = NULL;
    HBITMAP hbm = NULL;

    ppImgBits = fMissing ? &g_pImgBitsMissing : &g_pImgBitsNotLoaded;

    if (*ppImgBits == NULL)
    {
        LOCK_GLOBALS;

        if (*ppImgBits == NULL)
        {
#ifdef WIN16
            HRSRC               hRes;
            HGLOBAL             hBmpFile;

            BYTE                *pbBits;
            int                 cbRow;

            BITMAPINFOHEADER    *lpbmih;

            UINT                ncolors;
            RGBQUAD             *lprgb;
            LONG                xWidth;
            LONG                yHeight;
            BYTE                *lpCurrent;

            // Find the Bitmap
            hRes = FindResource(g_hInstCore,(LPCWSTR)(fMissing ? IDB_MISSING : IDB_NOTLOADED), RT_BITMAP);
            if (hRes)
            {
                // Load the BMP from the resource file.
                hBmpFile = LoadResource(g_hInstCore, hRes);
                if ((hBmpFile))
                {
                    // copy out the appropriate info from the bitmap
                    lpCurrent = (BYTE *)LockResource(hBmpFile);
                    // The BITMAPFILEHEADER is striped for us, so we just start with a BITMAPINFOHEADER
                    lpbmih = (BITMAPINFOHEADER *)lpCurrent;
                    lpCurrent += sizeof(BITMAPINFOHEADER);

                    // Compute some usefull information from the bitmap
                    if (lpbmih->biPlanes != 1)
                        goto Cleanup;

                    if (    lpbmih->biBitCount != 1
                        &&  lpbmih->biBitCount != 4
                        &&  lpbmih->biBitCount != 8
                        &&  lpbmih->biBitCount != 16
                        &&  lpbmih->biBitCount != 24
                        &&  lpbmih->biBitCount != 32)
                        goto Cleanup;

                    if (lpbmih->biBitCount <= 8)
                    {
                        ncolors = 1 << lpbmih->biBitCount;

                        if (lpbmih->biClrUsed > 0 && lpbmih->biClrUsed < ncolors)
                        {
                            ncolors = lpbmih->biClrUsed;
                        }
                    }

                    if (ncolors)
                    {
                        lprgb = (RGBQUAD *)lpCurrent;
                        lpCurrent += ncolors * sizeof(RGBQUAD);
                    }

                    xWidth  = lpbmih->biWidth;
                    yHeight = lpbmih->biHeight;

                    pibd = new CImgBitsDIB();
                    if (!pibd)
                        goto Cleanup;

                    hr = THR(pibd->AllocDIB(lpbmih->biBitCount, xWidth, yHeight, lprgb, nColors, -1, TRUE));
                    if (hr)
                        goto Cleanup;

                    // Get the actual Bitmap bits
                    memcpy(pibd->GetBits(), lpCurrent, pibd->CbLine() * yHeight);

                    Cleanup:
                        UnlockResource(hBmpFile);
                        FreeResource(hBmpFile);
                }
            }

#else
            hbm = (HBITMAP) LoadImage(g_hInstCore, (LPCWSTR)(DWORD_PTR)(fMissing ? IDB_MISSING : IDB_NOTLOADED),
                                      IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            // NOTE (lmollico): bitmaps are in mshtml.dll

            pibd = new CImgBitsDIB();
            if (!pibd)
                goto Cleanup;

            Verify(!pibd->AllocCopyBitmap(hbm, FALSE, -1));

            Verify(DeleteObject(hbm));
#endif
            *ppImgBits = pibd;
            pibd = NULL;
            
Cleanup:
            if (pibd)
                delete pibd;
        }

    }

    return *ppImgBits;
}

void GetPlaceHolderBitmapSize(BOOL fMissing, SIZE * pSize)
{
    CImgBits *pib;

    pib = GetPlaceHolderBitmap(fMissing);

    pSize->cx = pib->Width();
    pSize->cy = pib->Height();
}

//+---------------------------------------------------------------------------
//
//  DrawPlaceHolder
//
//  Synopsis:   Draw the Place holder, the ALT string and the bitmap
//
//----------------------------------------------------------------------------

void DrawPlaceHolder(HDC hdc,
#ifdef WIN16
                    RECTL rectlImg,
#else
                    RECT rectImg,
#endif
                     TCHAR * lpString, CODEPAGE codepage, LCID lcid, SHORT sBaselineFont,
                     SIZE * psizeGrab, BOOL fMissing,
                     COLORREF fgColor, COLORREF bgColor, SIZE * psizePrint,
                     BOOL fRTL, DWORD dwFlags)
{
    LONG     xDstWid;
    LONG     yDstHei;
    LONG     xSrcWid;
    LONG     ySrcHei;
    CImgBits  *pib;
    RECT rcDst;
    RECT rcSrc; 
#ifdef WIN16
    GDIRECT  rectImg = { rectlImg.left, rectlImg.top, rectlImg.right, rectlImg.bottom };
#endif
    BOOL fDrawBlackRectForPrinting = psizePrint != NULL;

    bgColor &= 0x00FFFFFF;

    pib = GetPlaceHolderBitmap(fMissing);

    xDstWid = psizePrint ? psizePrint->cx : pib->Width();
    yDstHei = psizePrint ? psizePrint->cy : pib->Height();
    xSrcWid = pib->Width();
    ySrcHei = pib->Height();

    if ((rectImg.right - rectImg.left >= 10) &&
        (rectImg.bottom - rectImg.top >= 10))
    {
        if (fDrawBlackRectForPrinting)
        {
            COLORREF crBlack = 0;  // zero is black
            HBRUSH   hBrush = 0;

            hBrush = CreateSolidBrush(crBlack);
            if (hBrush)
            {
                FrameRect(hdc, &rectImg, hBrush);
                DeleteObject(hBrush);
            }
            else
                fDrawBlackRectForPrinting = FALSE;
        }

        if (!fDrawBlackRectForPrinting)
        {
            if ((bgColor == 0x00ffffff) || (bgColor == 0x00000000))
            {
                DrawEdge(hdc, &rectImg, BDR_SUNKENOUTER, BF_TOPLEFT);
                DrawEdge(hdc, &rectImg, BDR_SUNKENINNER, BF_BOTTOMRIGHT);
            }
            else
            {
                DrawEdge(hdc, &rectImg, BDR_SUNKENOUTER, BF_RECT);
            }
        }
    }

    if (lpString != NULL)
    {
        RECT rc;
        BOOL fGlyph = FALSE;
        UINT cch = _tcslen(lpString);

        rc.left = rectImg.left + xDstWid + 2 * psizeGrab->cx;
        rc.right = rectImg.right - psizeGrab->cx;
        rc.top = rectImg.top + psizeGrab->cy;
        rc.bottom = rectImg.bottom - psizeGrab->cy;

        CIntlFont intlFont(hdc, codepage, lcid, sBaselineFont, lpString);
        SetTextColor(hdc, fgColor);

        if(!fRTL)
        {
            for(UINT i = 0; i < cch; i++)
            {
                WCHAR ch = lpString[i];
                if(ch >= 0x300 && IsGlyphableChar(ch))
                {
                    fGlyph = TRUE;
                    break;
                }
            }
        }

        // send complex text or text layed out right-to-left
        // to be drawn through Uniscribe
        if(fGlyph || fRTL)
        {
            HRESULT hr;
            UINT taOld = 0;
            UINT fuOptions = ETO_CLIPPED;

            if(fRTL)
            {
                taOld = GetTextAlign(hdc);
                SetTextAlign(hdc, TA_RTLREADING | TA_RIGHT);
                fuOptions |= ETO_RTLREADING;
            }

            extern HRESULT LSUniscribeTextOut(HDC hdc, 
                                           int iX, 
                                           int iY, 
                                           UINT uOptions, 
                                           CONST RECT *prc, 
                                           LPCTSTR pString, 
                                           UINT cch,
                                           int *piDx); 

            hr = LSUniscribeTextOut(hdc,
                                    !fRTL ? rc.left : rc.right, 
                                    rc.top,
                                    fuOptions,
                                    &rc,
                                    lpString,
                                    cch,
                                    NULL);

            if(fRTL)
                SetTextAlign(hdc, taOld);
        }
        else
        {
            DrawTextInCodePage(WindowsCodePageFromCodePage(codepage),
                hdc, lpString, -1, &rc, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        }
    }

    if (((rectImg.right - rectImg.left) <= 2 * psizeGrab->cx) ||
        ((rectImg.bottom - rectImg.top) <= 2 * psizeGrab->cy))
        return;

    InflateRect(&rectImg, -psizeGrab->cx, -psizeGrab->cy);

    if (xDstWid > rectImg.right - rectImg.left)
    {
        xSrcWid = MulDivQuick(xSrcWid, rectImg.right - rectImg.left, xDstWid);
        xDstWid = rectImg.right - rectImg.left;
    }
    if (yDstHei > rectImg.bottom - rectImg.top)
    {
        ySrcHei = MulDivQuick(ySrcHei, rectImg.bottom - rectImg.top, yDstHei);
        yDstHei = rectImg.bottom - rectImg.top;
    }

    rcDst.left = rectImg.left;
    rcDst.top = rectImg.top;
    rcDst.right = rcDst.left + xDstWid;
    rcDst.bottom = rcDst.top + yDstHei;

    rcSrc.left = 0;
    rcSrc.top = 0;
    rcSrc.right = xSrcWid;
    rcSrc.bottom = ySrcHei;

    pib->StretchBlt(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags);
}

int Union(int _yTop, int _yBottom, BOOL fInvalidateAll, int yBottom)
{
    if (    (_yTop != -1)
        &&  (   fInvalidateAll
            ||  (   (_yBottom >= _yTop)
                &&  (yBottom >= _yTop)
                &&  (yBottom <= _yBottom))
            ||  (   (_yBottom < _yTop)
                &&  (   (yBottom >= _yTop)
                     || (yBottom <= _yBottom))
                )))
    {
        return -1;
    }
    return _yTop;
}

#ifdef OLDIMAGECODE // now replaced by CImgBitsDIB

ULONG ImgDibSize(HBITMAP hbm)
{
    DIBSECTION ds;

    if (hbm && GetObject(hbm, sizeof(DIBSECTION), &ds))
        return(ds.dsBmih.biWidth * ds.dsBmih.biHeight *
            ds.dsBmih.biBitCount / 8);
    else
        return(0);
}

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
    int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow, BOOL fMono)
{
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[256];
            WORD aw[256];
            DWORD adw[3];
        } u;
    } bmi;
    int i;

    if (cBitsPerPix != 8)
        fPal = FALSE;

    bmi.bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth         = xWid;
    bmi.bmih.biHeight        = yHei;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = (cBitsPerPix == 15) ? 16 : cBitsPerPix;
    bmi.bmih.biCompression   = (cBitsPerPix == 16) ? BI_BITFIELDS : BI_RGB;
    bmi.bmih.biSizeImage     = 0;
    bmi.bmih.biXPelsPerMeter = 0;
    bmi.bmih.biYPelsPerMeter = 0;
    bmi.bmih.biClrUsed       = 0;
    bmi.bmih.biClrImportant  = 0;

    if (cBitsPerPix == 1)
    {
        bmi.bmih.biClrUsed = 2;

        if (cEnt > 2)
            cEnt = 2;

        if (cEnt > 0)
        {
            bmi.bmih.biClrImportant = cEnt;
            CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
        }
        else
        {
            if (fMono)  //IEUNIX monochrome.
            {
                bmi.u.argb[0].rgbBlue = 0; //foreground color
#ifdef BIG_ENDIAN
                *((DWORD*)&bmi.u.argb[1]) = 0xffffff00;
#else
                *((DWORD*)&bmi.u.argb[1]) = 0x00ffffff;
#endif
            }
            else
            {
                bmi.u.argb[0] = g_rgbBgColor;
                bmi.u.argb[1] = g_rgbFgColor;
            }
        }
    }
    else if (cBitsPerPix == 4)
    {
        bmi.bmih.biClrUsed = 16;

        if (cEnt > 16)
            cEnt = 16;

        if (cEnt > 0)
        {
            bmi.bmih.biClrImportant = cEnt;
            CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
        }
        else
        {
            bmi.bmih.biClrImportant = 16;
            CopyColorsFromPaletteEntries(bmi.u.argb, g_peVga, 16);
        }
    }
    else if (cBitsPerPix == 8)
    {
        if (fPal)
        {
            bmi.bmih.biClrUsed = 256;

            for (i = 0; i < 256; ++i)
                bmi.u.aw[i] = i;
        }
        else
        {
            if (cEnt > 0 && cEnt < 256)
            {
                bmi.bmih.biClrUsed = cEnt;
                bmi.bmih.biClrImportant = cEnt;
            }
            else
                bmi.bmih.biClrUsed = 256;

            if (cEnt && ppe)
            {
                CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
            }
        }
    }
    else if (cBitsPerPix == 16)
    {
        bmi.u.adw[0] = MASK565_0;
        bmi.u.adw[1] = MASK565_1;
        bmi.u.adw[2] = MASK565_2;
    }

    return ImgCreateDibFromInfo((BITMAPINFO *)&bmi, fPal ? DIB_PAL_COLORS : DIB_RGB_COLORS, ppbBits, pcbRow);
}

HBITMAP ImgCreateDibFromInfo(BITMAPINFO * pbmi, UINT wUsage, BYTE ** ppbBits, int * pcbRow)
{
    HDC 	hdcMem = NULL;
    HBITMAP	hbm = NULL;
    BYTE * 	pbBits;
    int 	cbRow;
    LONG    xWid, yHei;
    int 	cBitsPerPix;

    xWid = pbmi->bmiHeader.biWidth;
    yHei = pbmi->bmiHeader.biHeight;
    cBitsPerPix = pbmi->bmiHeader.biBitCount;
    
    Assert(cBitsPerPix == 1 || cBitsPerPix == 4 ||
        cBitsPerPix == 8 || cBitsPerPix == 16 || cBitsPerPix == 24 || cBitsPerPix == 32);
    Assert(xWid > 0 && yHei > 0);

	cbRow = ((xWid * cBitsPerPix + 31) & ~31) / 8;

    if (pcbRow)
    {
        *pcbRow = cbRow;
    }

    hdcMem = GetMemoryDC();

    if (hdcMem == NULL)
        goto Cleanup;

    hbm = CreateDIBSection(hdcMem, pbmi, wUsage, (void **)&pbBits, NULL, 0);

    if (hbm && ppbBits)
    {
        *ppbBits = pbBits;
    }

    #ifdef PERFMETER
    if (hbm)
    {
        MtAdd(Mt(DIBSection), +1, ImgDibSize(hbm));
    }
    #endif

    // Fill the bits with garbage so that the client doesn't assume that
    // the DIB gets created cleared (on WinNT it does, on Win95 it doesn't).

    #if DBG==1 && !defined(WIN16)
    if (hbm && pbBits)
        for (int c = cbRow * yHei; --c >= 0; ) pbBits[c] = (BYTE)c;
    #endif

Cleanup:
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);

    return(hbm);
}

void ImgDeleteDib(HBITMAP hbm)
{
    if (hbm)
    {
        #ifdef PERFMETER
        MtAdd(Mt(DIBSection), -1, -(LONG)ImgDibSize(hbm));
        #endif

        Verify(DeleteObject(hbm));
    }
}

HBITMAP ComputeTransMask(HBITMAP hbmDib, BOOL fPal, BYTE bTrans)
{
    DIBSECTION      ds;
    HBITMAP         hbmMask;
    DWORD *         pdw;
    DWORD           dwBits;
    BYTE *          pb;
    int             cb;
    int             cbPad;
    int             x, y, b;
#ifdef UNIX
    PALETTEENTRY    ape[2] = { { 0, 0, 0, 0 }, 
                               { 0, 255, 255, 255 } };
#else
    PALETTEENTRY    ape[2] = { { 0, 0, 0, 0 }, 
                               { 255, 255, 255, 0 } };
#endif
    BOOL            fTrans = FALSE;

    if (    !GetObject(hbmDib, sizeof(DIBSECTION), &ds)
        ||  ds.dsBmih.biBitCount != 8)
        return(NULL);

    pb    = (BYTE *)ds.dsBm.bmBits;
    cbPad = ((ds.dsBmih.biWidth + 3) & ~3) - ds.dsBmih.biWidth;

    if (fPal)
    {
        bTrans = (BYTE)g_wIdxTrans;
    }

    for (y = ds.dsBmih.biHeight; y-- > 0; pb += cbPad)
        for (x = ds.dsBmih.biWidth; x-- > 0; )
            if (*pb++ == bTrans)
                goto trans;

    return((HBITMAP)0xFFFFFFFF);

trans:

    hbmMask = ImgCreateDib(ds.dsBmih.biWidth, ds.dsBmih.biHeight, FALSE, 1,
        2, ape, &pb, &cb);

    if (hbmMask == NULL)
        return(NULL);

    pdw   = (DWORD *)ds.dsBm.bmBits;
    cbPad = cb - (ds.dsBmih.biWidth + 7) / 8;

    for (y = ds.dsBmih.biHeight; y-- > 0; pb += cbPad)
    {
        for (x = ds.dsBmih.biWidth; x > 0; x -= 8)
        {
            dwBits = *pdw++; b = 0;
#ifdef UNIX
            b |= ((BYTE)(dwBits >> 24) != bTrans); b <<=1;
            b |= ((BYTE)(dwBits >> 16) != bTrans); b <<=1;
            b |= ((BYTE)(dwBits >> 8) != bTrans); b <<=1;
            b |= ((BYTE)dwBits != bTrans); b <<= 1;
#else
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1;
#endif
            if (x <= 4)
                b = (b << 3) | 0xF;
            else
            {
                dwBits = *pdw++;
#ifdef UNIX
                b |= ((BYTE)(dwBits >> 24) != bTrans); b <<= 1;
                b |= ((BYTE)(dwBits >> 16) != bTrans); b <<= 1;
                b |= ((BYTE)(dwBits >> 8 ) != bTrans); b <<= 1;
                b |= ((BYTE)dwBits != bTrans);
#else
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans);
#endif
            }

            *pb++ = (BYTE)b;
        }
    }

    if (fPal)
    {
        pb = (BYTE *)ds.dsBm.bmBits - 1;
        cb = ((ds.dsBmih.biWidth + 3) & ~3) * ds.dsBmih.biHeight;
        b = 255;        // want white index

        while (--cb >= 0)
            if (*++pb == bTrans)
                *pb = b;
    }
    else
    {
        HDC hdcMem = GetMemoryDC();

        if (hdcMem == NULL)
        {
            ImgDeleteDib(hbmMask);
            return(NULL);
        }

        HBITMAP hbmSav = (HBITMAP)SelectObject(hdcMem, hbmDib);
        RGBQUAD rgbWhite = { 255, 255, 255, 0 };
        Verify(SetDIBColorTable(hdcMem, bTrans, 1, &rgbWhite) == 1);
        SelectObject(hdcMem, hbmSav);
        ReleaseMemoryDC(hdcMem);
    }

    return(hbmMask);
}

#endif

void ComputeFrameVisibility(IMGANIMSTATE *pImgAnimState, LONG xWidth, LONG yHeight, LONG xDispWid, LONG yDispHei)
{
    GIFFRAME *  pgf;
    GIFFRAME *  pgfClip;
    GIFFRAME *  pgfDraw = pImgAnimState->pgfDraw;
    GIFFRAME *  pgfDrawNext = pgfDraw->pgfNext;
    RECT        rectCur;

    // determine which frames are visible or partially visible at this time

    for (pgf = pImgAnimState->pgfFirst; pgf != pgfDrawNext; pgf = pgf->pgfNext)
    {
        if (pgf->hrgnVis != NULL)
        {
            DeleteRgn( pgf->hrgnVis );
            pgf->hrgnVis = NULL;
            pgf->bRgnKind = NULLREGION;
        }

        // This is kinda complicated.
        // We only want to subtract out this frame from its predecessors under certain
        // conditions.
        // If it's the current frame, then all we care about is transparency.
        // If its a preceding frame, then any bits from frames that preceded should
        // be clipped out if it wasn't transparent, but also wasn't to be replaced by
        // previous pixels.
        if (((pgf == pgfDraw) && !pgf->bTransFlags) ||
            ((pgf != pgfDraw) && !pgf->bTransFlags && (pgf->bDisposalMethod != gifRestorePrev)) ||
            ((pgf != pgfDraw) && (pgf->bDisposalMethod == gifRestoreBkgnd)))
        {
            // clip this rgn out of those that came before us if it's not trasparent,
            // or if it leaves a background-colored hole and is not the current frame.
            // The current frame, being current, hasn't left a background-colored hole yet.
            for (pgfClip = pImgAnimState->pgfFirst; pgfClip != pgf; pgfClip = pgfClip->pgfNext)
            {
                if (pgfClip->hrgnVis != NULL)
                {
                    if (pgf->hrgnVis == NULL)
                    {
                        // Since we'll use these regions to clip when drawing, we need them mapped
                        // for destination stretching.
                        CalcStretchRect(&rectCur, xWidth, yHeight, xDispWid, yDispHei, pgf);

                        pgf->hrgnVis = CreateRectRgnIndirect(&rectCur);
                        pgf->bRgnKind = SIMPLEREGION;
                    }

                    pgfClip->bRgnKind = (BYTE)CombineRgn(pgfClip->hrgnVis, pgfClip->hrgnVis, pgf->hrgnVis, RGN_DIFF);
                }
            }
        } // if we need to clip this frame out of its predecessor(s)

        // If this is a replace with background frame preceding the current draw frame,
        // then it is not visible at all, so set the visibility traits so it won't be drawn.
        if ((pgf != pgfDraw) && (pgf->bDisposalMethod >= gifRestoreBkgnd))
        {
            if (pgf->hrgnVis != NULL)
            {
                DeleteRgn(pgf->hrgnVis);
                pgf->hrgnVis = NULL;
                pgf->bRgnKind = NULLREGION;
            }
        }
        else if (pgf->hrgnVis == NULL)
        {
            // Since we'll use these regions to clip when drawing, we need them mapped
            // for destination stretching.
            CalcStretchRect(&rectCur, xWidth, yHeight, xDispWid, yDispHei, pgf);

            pgf->hrgnVis = CreateRectRgnIndirect(&rectCur);
            pgf->bRgnKind = SIMPLEREGION;
        }

    } // for check each frame's visibility
}
