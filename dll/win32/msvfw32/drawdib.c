/*
 * Copyright 2000 Bradley Baetz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: Some flags are ignored
 *
 * Handle palettes
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

typedef struct tagWINE_HDD {
    HDC                 hdc;
    INT                 dxDst;
    INT                 dyDst;
    LPBITMAPINFOHEADER  lpbi;
    INT                 dxSrc;
    INT                 dySrc;
    HPALETTE            hpal;		/* Palette to use for the DIB */
    BOOL                begun;		/* DrawDibBegin has been called */
    LPBITMAPINFOHEADER  lpbiOut;	/* Output format */
    HIC                 hic;		/* HIC for decompression */
    HDC                 hMemDC;		/* DC for buffering */
    HBITMAP             hOldDib;	/* Original Dib */
    HBITMAP             hDib;		/* DibSection */
    LPVOID              lpvbits;	/* Buffer for holding decompressed dib */
    HDRAWDIB            hSelf;
    struct tagWINE_HDD* next;
} WINE_HDD;

static int num_colours(const BITMAPINFOHEADER *lpbi)
{
	if(lpbi->biClrUsed)
		return lpbi->biClrUsed;
	if(lpbi->biBitCount<=8)
		return 1<<lpbi->biBitCount;
	return 0;
}

static WINE_HDD*        HDD_FirstHdd /* = NULL */;

static WINE_HDD*       MSVIDEO_GetHddPtr(HDRAWDIB hd)
{
    WINE_HDD*   hdd;

    for (hdd = HDD_FirstHdd; hdd != NULL && hdd->hSelf != hd; hdd = hdd->next);
    return hdd;
}

static UINT_PTR HDD_HandleRef = 1;

/***********************************************************************
 *		DrawDibOpen		[MSVFW32.@]
 */
HDRAWDIB VFWAPI DrawDibOpen(void) 
{
    WINE_HDD*   whdd;

    TRACE("(void)\n");

    whdd = calloc(1, sizeof(WINE_HDD));
    TRACE("=> %p\n", whdd);

    while (MSVIDEO_GetHddPtr((HDRAWDIB)HDD_HandleRef) != NULL) HDD_HandleRef++;
    whdd->hSelf = (HDRAWDIB)HDD_HandleRef++;

    whdd->next = HDD_FirstHdd;
    HDD_FirstHdd = whdd;

    return whdd->hSelf;
}

/***********************************************************************
 *		DrawDibClose		[MSVFW32.@]
 */
BOOL VFWAPI DrawDibClose(HDRAWDIB hdd) 
{
    WINE_HDD* whdd = MSVIDEO_GetHddPtr(hdd);
    WINE_HDD** p;

    TRACE("(%p)\n", hdd);

    if (!whdd) return FALSE;

    if (whdd->begun) DrawDibEnd(hdd);

    for (p = &HDD_FirstHdd; *p != NULL; p = &((*p)->next))
    {
        if (*p == whdd)
        {
            *p = whdd->next;
            break;
        }
    }

    free(whdd);

    return TRUE;
}

/***********************************************************************
 *		DrawDibEnd		[MSVFW32.@]
 */
BOOL VFWAPI DrawDibEnd(HDRAWDIB hdd) 
{
    BOOL ret = TRUE;
    WINE_HDD *whdd = MSVIDEO_GetHddPtr(hdd);

    TRACE("(%p)\n", hdd);

    if (!whdd) return FALSE;

    whdd->hpal = 0; /* Do not free this */
    whdd->hdc = 0;
    free(whdd->lpbi);
    whdd->lpbi = NULL;
    free(whdd->lpbiOut);
    whdd->lpbiOut = NULL;

    whdd->begun = FALSE;

    /*if (whdd->lpvbits)
      free(whdd->lpvbuf);*/

    if (whdd->hMemDC) 
    {
        SelectObject(whdd->hMemDC, whdd->hOldDib);
        DeleteDC(whdd->hMemDC);
        whdd->hMemDC = 0;
    }

    if (whdd->hDib) DeleteObject(whdd->hDib);
    whdd->hDib = 0;

    if (whdd->hic) 
    {
        ICDecompressEnd(whdd->hic);
        ICClose(whdd->hic);
        whdd->hic = 0;
    }

    whdd->lpvbits = NULL;

    return ret;
}

/***********************************************************************
 *              DrawDibBegin            [MSVFW32.@]
 */
BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd,
                         HDC      hdc,
                         INT      dxDst,
                         INT      dyDst,
                         LPBITMAPINFOHEADER lpbi,
                         INT      dxSrc,
                         INT      dySrc,
                         UINT     wFlags) 
{
    BOOL ret = TRUE;
    WINE_HDD *whdd;

    TRACE("(%p,%p,%d,%d,%p,%d,%d,0x%08x)\n",
          hdd, hdc, dxDst, dyDst, lpbi, dxSrc, dySrc, wFlags);

    TRACE("lpbi: %ld,%ld/%ld,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld\n",
          lpbi->biSize, lpbi->biWidth, lpbi->biHeight, lpbi->biPlanes,
          lpbi->biBitCount, lpbi->biCompression, lpbi->biSizeImage,
          lpbi->biXPelsPerMeter, lpbi->biYPelsPerMeter, lpbi->biClrUsed,
          lpbi->biClrImportant);

    if (wFlags & ~(DDF_BUFFER))
        FIXME("wFlags == 0x%08x not handled\n", wFlags & ~(DDF_BUFFER));

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    if (whdd->begun) DrawDibEnd(hdd);

    if (lpbi->biCompression) 
    {
        DWORD size = 0;

        whdd->hic = ICOpen(ICTYPE_VIDEO, lpbi->biCompression, ICMODE_DECOMPRESS);
        if (!whdd->hic) 
        {
            WARN("Could not open IC. biCompression == 0x%08lx\n", lpbi->biCompression);
            ret = FALSE;
        }

        if (ret) 
        {
            size = ICDecompressGetFormat(whdd->hic, lpbi, NULL);
            if (size == ICERR_UNSUPPORTED) 
            {
                WARN("Codec doesn't support GetFormat, giving up.\n");
                ret = FALSE;
            }
        }

        if (ret) 
        {
            whdd->lpbiOut = malloc(size);

            if (ICDecompressGetFormat(whdd->hic, lpbi, whdd->lpbiOut) != ICERR_OK)
                ret = FALSE;
        }

        if (ret) 
        {
            /* FIXME: Use Ex functions if available? */
            if (ICDecompressBegin(whdd->hic, lpbi, whdd->lpbiOut) != ICERR_OK)
                ret = FALSE;
            
            TRACE("biSizeImage == %ld\n", whdd->lpbiOut->biSizeImage);
            TRACE("biCompression == %ld\n", whdd->lpbiOut->biCompression);
            TRACE("biBitCount == %d\n", whdd->lpbiOut->biBitCount);
        }
    }
    else 
    {
        DWORD dwSize;
        /* No compression */
        TRACE("Not compressed!\n");
        if (lpbi->biHeight <= 0)
        {
            /* we don't draw inverted DIBs */
            TRACE("detected inverted DIB\n");
            ret = FALSE;
        }
        else
        {
            dwSize = lpbi->biSize + num_colours(lpbi)*sizeof(RGBQUAD);
            whdd->lpbiOut = malloc(dwSize);
            memcpy(whdd->lpbiOut, lpbi, dwSize);
        }
    }

    if (ret) 
    {
        /*whdd->lpvbuf = malloc(whdd->lpbiOut->biSizeImage);*/

        whdd->hMemDC = CreateCompatibleDC(hdc);
        TRACE("Creating: %ld, %p\n", whdd->lpbiOut->biSize, whdd->lpvbits);
        whdd->hDib = CreateDIBSection(whdd->hMemDC, (BITMAPINFO *)whdd->lpbiOut, DIB_RGB_COLORS, &(whdd->lpvbits), 0, 0);
        if (whdd->hDib) 
        {
            TRACE("Created: %p,%p\n", whdd->hDib, whdd->lpvbits);
        }
        else
        {
            ret = FALSE;
            TRACE("Error: %ld\n", GetLastError());
        }
        whdd->hOldDib = SelectObject(whdd->hMemDC, whdd->hDib);
    }

    if (ret) 
    {
        whdd->hdc = hdc;
        whdd->dxDst = dxDst;
        whdd->dyDst = dyDst;
        whdd->lpbi = malloc(lpbi->biSize);
        memcpy(whdd->lpbi, lpbi, lpbi->biSize);
        whdd->dxSrc = dxSrc;
        whdd->dySrc = dySrc;
        whdd->begun = TRUE;
        whdd->hpal = 0;
    } 
    else 
    {
        if (whdd->hic)
            ICClose(whdd->hic);
        free(whdd->lpbiOut);
        whdd->lpbiOut = NULL;
    }

    return ret;
}

/**********************************************************************
 *		DrawDibDraw		[MSVFW32.@]
 */
BOOL VFWAPI DrawDibDraw(HDRAWDIB hdd, HDC hdc,
                        INT xDst, INT yDst, INT dxDst, INT dyDst,
                        LPBITMAPINFOHEADER lpbi,
                        LPVOID lpBits,
                        INT xSrc, INT ySrc, INT dxSrc, INT dySrc,
                        UINT wFlags)
{
    WINE_HDD *whdd;
    BOOL ret;
    int reopen = 0;

    TRACE("(%p,%p,%d,%d,%d,%d,%p,%p,%d,%d,%d,%d,0x%08x)\n",
          hdd, hdc, xDst, yDst, dxDst, dyDst, lpbi, lpBits, xSrc, ySrc, dxSrc, dySrc, wFlags);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    TRACE("whdd=%p\n", whdd);

    if (wFlags & ~(DDF_SAME_HDC | DDF_SAME_DRAW | DDF_NOTKEYFRAME | DDF_UPDATE | DDF_DONTDRAW | DDF_BACKGROUNDPAL))
        FIXME("wFlags == 0x%08x not handled\n", wFlags);

    if (!lpBits) 
    {
        /* Undocumented? */
        lpBits = (LPSTR)lpbi + (WORD)(lpbi->biSize) + (WORD)(num_colours(lpbi)*sizeof(RGBQUAD));
    }


#define CHANGED(x) (whdd->x != x)

    /* Check if anything changed from the parameters passed and our struct.
     * If anything changed we need to run DrawDibBegin again to ensure we
     * can support the changes.
     */
    if (!whdd->begun)
        reopen = 1;
    else if (!(wFlags & DDF_SAME_HDC) && CHANGED(hdc))
        reopen = 2;
    else if (!(wFlags & DDF_SAME_DRAW))
    {
        if (CHANGED(lpbi) && memcmp(lpbi, whdd->lpbi, sizeof(*lpbi))) reopen = 3;
        else if (CHANGED(dxSrc)) reopen = 4;
        else if (CHANGED(dySrc)) reopen = 5;
        else if (CHANGED(dxDst)) reopen = 6;
        else if (CHANGED(dyDst)) reopen = 7;
    }
    if (reopen)
    {
        TRACE("Something changed (reason %d)!\n", reopen);
        ret = DrawDibBegin(hdd, hdc, dxDst, dyDst, lpbi, dxSrc, dySrc, 0);
        if (!ret)
            return ret;
    }

#undef CHANGED

    /* If source dimensions are not specified derive them from bitmap header */
    if (dxSrc == -1 && dySrc == -1)
    {
        dxSrc = lpbi->biWidth;
        dySrc = lpbi->biHeight;
    }
    /* If destination dimensions are not specified derive them from source */
    if (dxDst == -1 && dyDst == -1)
    {
        dxDst = dxSrc;
        dyDst = dySrc;
    }

    if (!(wFlags & DDF_UPDATE)) 
    {
        if (lpbi->biCompression) 
        {
            DWORD flags = 0;

            TRACE("Compression == 0x%08lx\n", lpbi->biCompression);

            if (wFlags & DDF_NOTKEYFRAME)
                flags |= ICDECOMPRESS_NOTKEYFRAME;

            ICDecompress(whdd->hic, flags, lpbi, lpBits, whdd->lpbiOut, whdd->lpvbits);
        }
        else
        {
            /* BI_RGB: lpbi->biSizeImage isn't reliable */
            DWORD biSizeImage = ((lpbi->biWidth * lpbi->biBitCount + 31) / 32) * 4 * lpbi->biHeight;
            memcpy(whdd->lpvbits, lpBits, biSizeImage);
        }
    }
    if (!(wFlags & DDF_DONTDRAW) && whdd->hpal)
    {
        if ((wFlags & DDF_BACKGROUNDPAL) && ! (wFlags & DDF_SAME_HDC))
         SelectPalette(hdc, whdd->hpal, TRUE);
        else
         SelectPalette(hdc, whdd->hpal, FALSE);
    }

    ret = StretchBlt(whdd->hdc, xDst, yDst, dxDst, dyDst, whdd->hMemDC, xSrc, ySrc, dxSrc, dySrc, SRCCOPY);
    TRACE("Painting %dx%d at %d,%d from %dx%d at %d,%d -> %d\n",
          dxDst, dyDst, xDst, yDst, dxSrc, dySrc, xSrc, ySrc, ret);

    return ret;
}

/*************************************************************************
 *		DrawDibStart		[MSVFW32.@]
 */
BOOL VFWAPI DrawDibStart(HDRAWDIB hdd, DWORD rate) {
	FIXME("(%p, %ld), stub\n", hdd, rate);
	return TRUE;
}

/*************************************************************************
 *		DrawDibStop		[MSVFW32.@]
 */
BOOL VFWAPI DrawDibStop(HDRAWDIB hdd) {
	FIXME("(%p), stub\n", hdd);
	return TRUE;
}

/***********************************************************************
 *              DrawDibChangePalette       [MSVFW32.@]
 */
BOOL VFWAPI DrawDibChangePalette(HDRAWDIB hdd, int iStart, int iLen, LPPALETTEENTRY lppe)
{
    FIXME("(%p, 0x%08x, 0x%08x, %p), stub\n", hdd, iStart, iLen, lppe);
    return TRUE;
}

/***********************************************************************
 *              DrawDibSetPalette       [MSVFW32.@]
 */
BOOL VFWAPI DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal) 
{
    WINE_HDD *whdd;

    TRACE("(%p, %p)\n", hdd, hpal);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    whdd->hpal = hpal;

    if (whdd->begun) 
    {
        SelectPalette(whdd->hdc, hpal, 0);
        RealizePalette(whdd->hdc);
    }

    return TRUE;
}

/***********************************************************************
 *              DrawDibGetBuffer       [MSVFW32.@]
 */
LPVOID VFWAPI DrawDibGetBuffer(HDRAWDIB hdd, LPBITMAPINFOHEADER lpbi, DWORD dwSize, DWORD dwFlags)
{
    FIXME("(%p, %p, 0x%08lx, 0x%08lx), stub\n", hdd, lpbi, dwSize, dwFlags);
    return NULL;
}

/***********************************************************************
 *              DrawDibGetPalette       [MSVFW32.@]
 */
HPALETTE VFWAPI DrawDibGetPalette(HDRAWDIB hdd) 
{
    WINE_HDD *whdd;

    TRACE("(%p)\n", hdd);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    return whdd->hpal;
}

/***********************************************************************
 *              DrawDibRealize          [MSVFW32.@]
 */
UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground) 
{
    WINE_HDD *whdd;
    UINT ret = 0;

    FIXME("(%p, %p, %d), stub\n", hdd, hdc, fBackground);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    if (!whdd->begun)
    {
        ret = 0;
        goto out;
    }

    if (!whdd->hpal)
        whdd->hpal = CreateHalftonePalette(hdc);

    SelectPalette(hdc, whdd->hpal, fBackground);
    ret = RealizePalette(hdc);

 out:
    TRACE("=> %u\n", ret);
    return ret;
}

/***********************************************************************
 *              DrawDibTime          [MSVFW32.@]
 */
BOOL VFWAPI DrawDibTime(HDRAWDIB hdd, LPDRAWDIBTIME lpddtime)
{
    FIXME("(%p, %p) stub\n", hdd, lpddtime);
    return FALSE;
}

/***********************************************************************
 *              DrawDibProfileDisplay          [MSVFW32.@]
 */
DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi)
{
    FIXME("(%p) stub\n", lpbi);

    return PD_CAN_DRAW_DIB;
}
