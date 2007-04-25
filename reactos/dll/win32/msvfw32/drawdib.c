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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME: Some flags are ignored
 *
 * Handle palettes
 */

#include <string.h>
#include "msvideo_private.h"

#include "wingdi.h"
#include "winuser.h"

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

int num_colours(LPBITMAPINFOHEADER lpbi)
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

static DWORD HDD_HandleRef = 1;

/***********************************************************************
 *		DrawDibOpen		[MSVFW32.@]
 */
HDRAWDIB VFWAPI DrawDibOpen(void) 
{
    WINE_HDD*   whdd;

    TRACE("(void)\n");
	
    whdd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_HDD));
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

    HeapFree(GetProcessHeap(), 0, whdd);

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

    whdd->hpal = 0; /* Do not free this */
    whdd->hdc = 0;
    if (whdd->lpbi) 
    {
        HeapFree(GetProcessHeap(), 0, whdd->lpbi);
        whdd->lpbi = NULL;
    }
    if (whdd->lpbiOut) 
    {
        HeapFree(GetProcessHeap(), 0, whdd->lpbiOut);
        whdd->lpbiOut = NULL;
    }

    whdd->begun = FALSE;

    /*if (whdd->lpvbits)
      HeapFree(GetProcessHeap(), 0, whdd->lpvbuf);*/

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

    TRACE("(%p,%p,%d,%d,%p,%d,%d,0x%08lx)\n",
          hdd, hdc, dxDst, dyDst, lpbi, dxSrc, dySrc, (DWORD)wFlags);

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
            whdd->lpbiOut = HeapAlloc(GetProcessHeap(), 0, size);

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
        dwSize = lpbi->biSize + num_colours(lpbi)*sizeof(RGBQUAD);
        whdd->lpbiOut = HeapAlloc(GetProcessHeap(), 0, dwSize);
        memcpy(whdd->lpbiOut, lpbi, dwSize);
    }

    if (ret) 
    {
        /*whdd->lpvbuf = HeapAlloc(GetProcessHeap(), 0, whdd->lpbiOut->biSizeImage);*/

        whdd->hMemDC = CreateCompatibleDC(hdc);
        TRACE("Creating: %ld, %p\n", whdd->lpbiOut->biSize, whdd->lpvbits);
        whdd->hDib = CreateDIBSection(whdd->hMemDC, (BITMAPINFO *)whdd->lpbiOut, DIB_RGB_COLORS, &(whdd->lpvbits), 0, 0);
        if (!whdd->hDib) 
        {
            TRACE("Error: %ld\n", GetLastError());
        }
        TRACE("Created: %p,%p\n", whdd->hDib, whdd->lpvbits);
        whdd->hOldDib = SelectObject(whdd->hMemDC, whdd->hDib);
    }

    if (ret) 
    {
        whdd->hdc = hdc;
        whdd->dxDst = dxDst;
        whdd->dyDst = dyDst;
        whdd->lpbi = HeapAlloc(GetProcessHeap(), 0, lpbi->biSize);
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
        if (whdd->lpbiOut) 
        {
            HeapFree(GetProcessHeap(), 0, whdd->lpbiOut);
            whdd->lpbiOut = NULL;
        }
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
    BOOL ret = TRUE;

    TRACE("(%p,%p,%d,%d,%d,%d,%p,%p,%d,%d,%d,%d,0x%08lx)\n",
          hdd, hdc, xDst, yDst, dxDst, dyDst, lpbi, lpBits, xSrc, ySrc, dxSrc, dySrc, (DWORD)wFlags);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    if (wFlags & ~(DDF_SAME_HDC | DDF_SAME_DRAW | DDF_NOTKEYFRAME | DDF_UPDATE | DDF_DONTDRAW))
        FIXME("wFlags == 0x%08lx not handled\n", (DWORD)wFlags);

    if (!lpBits) 
    {
        /* Undocumented? */
        lpBits = (LPSTR)lpbi + (WORD)(lpbi->biSize) + (WORD)(num_colours(lpbi)*sizeof(RGBQUAD));
    }


#define CHANGED(x) (whdd->x != x)

    if ((!whdd->begun) || 
        (!(wFlags & DDF_SAME_HDC) && CHANGED(hdc)) || 
        (!(wFlags & DDF_SAME_DRAW) && (CHANGED(lpbi) || CHANGED(dxSrc) || CHANGED(dySrc) || CHANGED(dxDst) || CHANGED(dyDst)))) 
    {
        TRACE("Something changed!\n");
        ret = DrawDibBegin(hdd, hdc, dxDst, dyDst, lpbi, dxSrc, dySrc, 0);
    }

#undef CHANGED

    if ((dxDst == -1) && (dyDst == -1)) 
    {
        dxDst = dxSrc;
        dyDst = dySrc;
    }

    if (!(wFlags & DDF_UPDATE)) 
    {
        /* biSizeImage may be set to 0 for BI_RGB (uncompressed) bitmaps */
        if ((lpbi->biCompression == BI_RGB) && (lpbi->biSizeImage == 0))
            lpbi->biSizeImage = ((lpbi->biWidth * lpbi->biBitCount + 31) / 32) * 4 * lpbi->biHeight;

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
            memcpy(whdd->lpvbits, lpBits, lpbi->biSizeImage);
        }
    }
    if (!(wFlags & DDF_DONTDRAW) && whdd->hpal)
        SelectPalette(hdc, whdd->hpal, FALSE);

    if (!(StretchBlt(whdd->hdc, xDst, yDst, dxDst, dyDst, whdd->hMemDC, xSrc, ySrc, dxSrc, dySrc, SRCCOPY)))
        ret = FALSE;
    
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
    HPALETTE oldPal;
    UINT ret = 0;

    FIXME("(%p, %p, %d), stub\n", hdd, hdc, fBackground);

    whdd = MSVIDEO_GetHddPtr(hdd);
    if (!whdd) return FALSE;

    if (!whdd || !(whdd->begun)) 
    {
        ret = 0;
        goto out;
    }

    if (!whdd->hpal)
        whdd->hpal = CreateHalftonePalette(hdc);

    oldPal = SelectPalette(hdc, whdd->hpal, fBackground);
    ret = RealizePalette(hdc);

 out:
    TRACE("=> %u\n", ret);
    return ret;
}

DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi)
{
    FIXME("(%p) stub\n", lpbi);

    return PD_CAN_DRAW_DIB;
}
