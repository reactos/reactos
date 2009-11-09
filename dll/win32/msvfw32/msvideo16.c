/*
 * msvideo 16-bit functions
 *
 * Copyright 1998 Marcus Meissner
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "winnls.h"
#include "winreg.h"
#include "winuser.h"
#include "vfw16.h"
#include "msvideo_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

/* Drivers32 settings */
#define HKLM_DRIVERS32 "Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"

/***********************************************************************
 *		DrawDibOpen		[MSVIDEO.102]
 */
HDRAWDIB16 VFWAPI DrawDibOpen16(void)
{
    return HDRAWDIB_16(DrawDibOpen());
}

/***********************************************************************
 *		DrawDibClose		[MSVIDEO.103]
 */
BOOL16 VFWAPI DrawDibClose16(HDRAWDIB16 hdd)
{
    return DrawDibClose(HDRAWDIB_32(hdd));
}

/************************************************************************
 *		DrawDibBegin		[MSVIDEO.104]
 */
BOOL16 VFWAPI DrawDibBegin16(HDRAWDIB16 hdd, HDC16 hdc, INT16 dxDst,
			     INT16 dyDst, LPBITMAPINFOHEADER lpbi, INT16 dxSrc,
			     INT16 dySrc, UINT16 wFlags)
{
    return DrawDibBegin(HDRAWDIB_32(hdd), HDC_32(hdc), dxDst, dyDst, lpbi,
			dxSrc, dySrc, wFlags);
}

/***********************************************************************
 *		DrawDibEnd		[MSVIDEO.105]
 */
BOOL16 VFWAPI DrawDibEnd16(HDRAWDIB16 hdd)
{
    return DrawDibEnd(HDRAWDIB_32(hdd));
}

/**********************************************************************
 *		DrawDibDraw		[MSVIDEO.106]
 */
BOOL16 VFWAPI DrawDibDraw16(HDRAWDIB16 hdd, HDC16 hdc, INT16 xDst, INT16 yDst,
			    INT16 dxDst, INT16 dyDst, LPBITMAPINFOHEADER lpbi,
			    LPVOID lpBits, INT16 xSrc, INT16 ySrc, INT16 dxSrc,
			    INT16 dySrc, UINT16 wFlags)
{
    return DrawDibDraw(HDRAWDIB_32(hdd), HDC_32(hdc), xDst, yDst, dxDst,
		       dyDst, lpbi, lpBits, xSrc, ySrc, dxSrc, dySrc, wFlags);
}

/***********************************************************************
 *              DrawDibGetPalette       [MSVIDEO.108]
 */
HPALETTE16 VFWAPI DrawDibGetPalette16(HDRAWDIB16 hdd)
{
    return HPALETTE_16(DrawDibGetPalette(HDRAWDIB_32(hdd)));
}

/***********************************************************************
 *              DrawDibSetPalette       [MSVIDEO.110]
 */
BOOL16 VFWAPI DrawDibSetPalette16(HDRAWDIB16 hdd, HPALETTE16 hpal)
{
    return DrawDibSetPalette(HDRAWDIB_32(hdd), HPALETTE_32(hpal));
}

/***********************************************************************
 *              DrawDibRealize          [MSVIDEO.112]
 */
UINT16 VFWAPI DrawDibRealize16(HDRAWDIB16 hdd, HDC16 hdc,
			       BOOL16 fBackground)
{
    return (UINT16)DrawDibRealize(HDRAWDIB_32(hdd), HDC_32(hdc), fBackground);
}

/*************************************************************************
 *		DrawDibStart		[MSVIDEO.118]
 */
BOOL16 VFWAPI DrawDibStart16(HDRAWDIB16 hdd, DWORD rate)
{
    return DrawDibStart(HDRAWDIB_32(hdd), rate);
}

/*************************************************************************
 *		DrawDibStop		[MSVIDEO.119]
 */
BOOL16 VFWAPI DrawDibStop16(HDRAWDIB16 hdd)
{
    return DrawDibStop(HDRAWDIB_32(hdd));
}

/***********************************************************************
 *		ICOpen				[MSVIDEO.203]
 */
HIC16 VFWAPI ICOpen16(DWORD fccType, DWORD fccHandler, UINT16 wMode)
{
    return HIC_16(ICOpen(fccType, fccHandler, wMode));
}

/***********************************************************************
 *		ICClose			[MSVIDEO.204]
 */
LRESULT WINAPI ICClose16(HIC16 hic)
{
    return ICClose(HIC_32(hic));
}

/***********************************************************************
 *		_ICMessage			[MSVIDEO.207]
 */
LRESULT VFWAPIV ICMessage16( HIC16 hic, UINT16 msg, UINT16 cb, VA_LIST16 valist )
{
    LPWORD lpData;
    SEGPTR segData;
    LRESULT ret;
    UINT16 i;

    lpData = HeapAlloc(GetProcessHeap(), 0, cb);

    TRACE("0x%08x, %u, %u, ...)\n", (DWORD) hic, msg, cb);

    for (i = 0; i < cb / sizeof(WORD); i++) 
    {
	lpData[i] = VA_ARG16(valist, WORD);
    }

    segData = MapLS(lpData);
    ret = ICSendMessage16(hic, msg, segData, (DWORD) cb);
    UnMapLS(segData);
    HeapFree(GetProcessHeap(), 0, lpData);
    return ret;
}

/***********************************************************************
 *		ICGetInfo			[MSVIDEO.212]
 */
LRESULT VFWAPI ICGetInfo16(HIC16 hic, ICINFO16 * picinfo, DWORD cb)
{
    LRESULT ret;

    TRACE("(0x%08x,%p,%d)\n", (DWORD) hic, picinfo, cb);
    ret = ICSendMessage16(hic, ICM_GETINFO, (DWORD) picinfo, cb);
    TRACE("	-> 0x%08lx\n", ret);
    return ret;
}

/***********************************************************************
 *		ICLocate			[MSVIDEO.213]
 */
HIC16 VFWAPI ICLocate16(DWORD fccType, DWORD fccHandler,
			LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut,
		       	WORD wFlags)
{
    return HIC_16(ICLocate(fccType, fccHandler, lpbiIn, lpbiOut, wFlags));
}

/***********************************************************************
 *		_ICCompress			[MSVIDEO.224]
 */
DWORD VFWAPIV ICCompress16(HIC16 hic, DWORD dwFlags,
			   LPBITMAPINFOHEADER lpbiOutput, LPVOID lpData,
			   LPBITMAPINFOHEADER lpbiInput, LPVOID lpBits,
			   LPDWORD lpckid, LPDWORD lpdwFlags,
			   LONG lFrameNum, DWORD dwFrameSize,
			   DWORD dwQuality, LPBITMAPINFOHEADER lpbiPrev,
			   LPVOID lpPrev)
{
    DWORD ret;
    ICCOMPRESS iccmp;
    SEGPTR seg_iccmp;
    
    TRACE("(0x%08x,%d,%p,%p,%p,%p,...)\n", (DWORD) hic, dwFlags,
	  lpbiOutput, lpData, lpbiInput, lpBits);

    iccmp.dwFlags = dwFlags;

    iccmp.lpbiOutput = lpbiOutput;
    iccmp.lpOutput = lpData;
    iccmp.lpbiInput = lpbiInput;
    iccmp.lpInput = lpBits;

    iccmp.lpckid = lpckid;
    iccmp.lpdwFlags = lpdwFlags;
    iccmp.lFrameNum = lFrameNum;
    iccmp.dwFrameSize = dwFrameSize;
    iccmp.dwQuality = dwQuality;
    iccmp.lpbiPrev = lpbiPrev;
    iccmp.lpPrev = lpPrev;
    seg_iccmp = MapLS(&iccmp);
    ret = ICSendMessage16(hic, ICM_COMPRESS, seg_iccmp, sizeof(ICCOMPRESS));
    UnMapLS(seg_iccmp);
    return ret;
}

/***********************************************************************
 *		_ICDecompress			[MSVIDEO.230]
 */
DWORD VFWAPIV ICDecompress16(HIC16 hic, DWORD dwFlags,
			     LPBITMAPINFOHEADER lpbiFormat, LPVOID lpData,
			     LPBITMAPINFOHEADER lpbi, LPVOID lpBits)
{
    ICDECOMPRESS icd;
    SEGPTR segptr;
    DWORD ret;

    TRACE("(0x%08x,%d,%p,%p,%p,%p)\n", (DWORD) hic, dwFlags, lpbiFormat,
	  lpData, lpbi, lpBits);

    icd.dwFlags = dwFlags;
    icd.lpbiInput = lpbiFormat;
    icd.lpInput = lpData;
    icd.lpbiOutput = lpbi;
    icd.lpOutput = lpBits;
    icd.ckid = 0;
    segptr = MapLS(&icd);
    ret = ICSendMessage16(hic, ICM_DECOMPRESS, segptr, sizeof(ICDECOMPRESS));
    UnMapLS(segptr);
    return ret;
}

/***********************************************************************
 *		_ICDrawBegin		[MSVIDEO.232]
 */
DWORD VFWAPIV ICDrawBegin16(HIC16 hic,		/* [in] */
			    DWORD dwFlags,	/* [in] flags */
			    HPALETTE16 hpal,	/* [in] palette to draw with */
			    HWND16 hwnd,	/* [in] window to draw to */
			    HDC16 hdc,		/* [in] HDC to draw to */
			    INT16 xDst,		/* [in] destination rectangle */
			    INT16 yDst,		/* [in] */
			    INT16 dxDst,	/* [in] */
			    INT16 dyDst,	/* [in] */
			    LPBITMAPINFOHEADER lpbi,	/* [in] format of frame to draw NOTE: SEGPTR */
			    INT16 xSrc,		/* [in] source rectangle */
			    INT16 ySrc,		/* [in] */
			    INT16 dxSrc,	/* [in] */
			    INT16 dySrc,	/* [in] */
			    DWORD dwRate,	/* [in] frames/second = (dwRate/dwScale) */
			    DWORD dwScale)	/* [in] */
{
    DWORD ret;
    ICDRAWBEGIN16 icdb;
    SEGPTR seg_icdb;

    TRACE ("(0x%08x,%d,0x%08x,0x%08x,0x%08x,%u,%u,%u,%u,%p,%u,%u,%u,%u,%d,%d)\n",
	   (DWORD) hic, dwFlags, (DWORD) hpal, (DWORD) hwnd, (DWORD) hdc,
	   xDst, yDst, dxDst, dyDst, lpbi, xSrc, ySrc, dxSrc, dySrc, dwRate,
	   dwScale);

    icdb.dwFlags = dwFlags;
    icdb.hpal = hpal;
    icdb.hwnd = hwnd;
    icdb.hdc = hdc;
    icdb.xDst = xDst;
    icdb.yDst = yDst;
    icdb.dxDst = dxDst;
    icdb.dyDst = dyDst;
    icdb.lpbi = lpbi;		/* Keep this as SEGPTR for the mapping code to deal with */
    icdb.xSrc = xSrc;
    icdb.ySrc = ySrc;
    icdb.dxSrc = dxSrc;
    icdb.dySrc = dySrc;
    icdb.dwRate = dwRate;
    icdb.dwScale = dwScale;
    seg_icdb = MapLS(&icdb);
    ret = (DWORD) ICSendMessage16(hic, ICM_DRAW_BEGIN, seg_icdb,
				  sizeof(ICDRAWBEGIN16));
    UnMapLS(seg_icdb);
    return ret;
}

/***********************************************************************
 *		_ICDraw			[MSVIDEO.234]
 */
DWORD VFWAPIV ICDraw16(HIC16 hic, DWORD dwFlags,
		       LPVOID lpFormat,	/* [???] NOTE: SEGPTR */
		       LPVOID lpData,	/* [???] NOTE: SEGPTR */
		       DWORD cbData, LONG lTime)
{
    DWORD ret;
    ICDRAW icd;
    SEGPTR seg_icd;

    TRACE("(0x%08x,0x%08x,%p,%p,%d,%d)\n", (DWORD) hic, dwFlags,
	  lpFormat, lpData, cbData, lTime);
    icd.dwFlags = dwFlags;
    icd.lpFormat = lpFormat;
    icd.lpData = lpData;
    icd.cbData = cbData;
    icd.lTime = lTime;
    seg_icd = MapLS(&icd);
    ret = ICSendMessage16(hic, ICM_DRAW, seg_icd, sizeof(ICDRAW));
    UnMapLS(seg_icd);
    return ret;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVIDEO.239]
 */
HIC16 VFWAPI ICGetDisplayFormat16(HIC16 hic, LPBITMAPINFOHEADER lpbiIn,
				  LPBITMAPINFOHEADER lpbiOut, INT16 depth,
				  INT16 dx, INT16 dy)
{
    return HIC_16(ICGetDisplayFormat(HIC_32(hic), lpbiIn, lpbiOut, depth,
				     dx, dy));
}

#define COPY(x,y) (x->y = x##16->y);
#define COPYPTR(x,y) (x->y = MapSL((SEGPTR)x##16->y));

/******************************************************************
 *		MSVIDEO_MapICDEX16To32
 *
 *
 */
static LPVOID MSVIDEO_MapICDEX16To32(LPDWORD lParam) 
{
    LPVOID ret;

    ICDECOMPRESSEX *icdx = HeapAlloc(GetProcessHeap(), 0, sizeof(ICDECOMPRESSEX));
    ICDECOMPRESSEX16 *icdx16 = MapSL(*lParam);
    ret = icdx16;

    COPY(icdx, dwFlags);
    COPYPTR(icdx, lpbiSrc);
    COPYPTR(icdx, lpSrc);
    COPYPTR(icdx, lpbiDst);
    COPYPTR(icdx, lpDst);
    COPY(icdx, xDst);
    COPY(icdx, yDst);
    COPY(icdx, dxDst);
    COPY(icdx, dyDst);
    COPY(icdx, xSrc);
    COPY(icdx, ySrc);
    COPY(icdx, dxSrc);
    COPY(icdx, dySrc);

    *lParam = (DWORD)(icdx);
    return ret;
}

/******************************************************************
 *		MSVIDEO_MapMsg16To32
 *
 *
 */
static LPVOID MSVIDEO_MapMsg16To32(UINT msg, LPDWORD lParam1, LPDWORD lParam2)
{
    LPVOID ret = 0;

    TRACE("Mapping %d\n", msg);

    switch (msg) 
    {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case ICM_ABOUT:
    case ICM_CONFIGURE:
    case ICM_COMPRESS_END:
    case ICM_DECOMPRESS_END:
    case ICM_DECOMPRESSEX_END:
    case ICM_SETQUALITY:
    case ICM_DRAW_START_PLAY:
    case ICM_DRAW_STOP_PLAY:
    case ICM_DRAW_REALIZE:
    case ICM_DRAW_RENDERBUFFER:
    case ICM_DRAW_END:
        break;
    case DRV_OPEN:
    case ICM_GETDEFAULTQUALITY:
    case ICM_GETQUALITY:
    case ICM_SETSTATE:
    case ICM_DRAW_WINDOW:
    case ICM_GETBUFFERSWANTED:
        *lParam1 = (DWORD)MapSL(*lParam1);
        break;
    case ICM_GETINFO:
        {
            ICINFO *ici = HeapAlloc(GetProcessHeap(), 0, sizeof(ICINFO));
            ICINFO16 *ici16;
            
            ici16 = MapSL(*lParam1);
            ret = ici16;
            
            ici->dwSize = sizeof(ICINFO);
            COPY(ici, fccType);
            COPY(ici, fccHandler);
            COPY(ici, dwFlags);
            COPY(ici, dwVersion);
            COPY(ici, dwVersionICM);
            MultiByteToWideChar( CP_ACP, 0, ici16->szName, -1, ici->szName, 16 );
            MultiByteToWideChar( CP_ACP, 0, ici16->szDescription, -1, ici->szDescription, 128 );
            MultiByteToWideChar( CP_ACP, 0, ici16->szDriver, -1, ici->szDriver, 128 );
            *lParam1 = (DWORD)(ici);
            *lParam2 = sizeof(ICINFO);
        }
        break;
    case ICM_COMPRESS:
        {
            ICCOMPRESS *icc = HeapAlloc(GetProcessHeap(), 0, sizeof(ICCOMPRESS));
            ICCOMPRESS *icc16;

            icc16 = MapSL(*lParam1);
            ret = icc16;

            COPY(icc, dwFlags);
            COPYPTR(icc, lpbiOutput);
            COPYPTR(icc, lpOutput);
            COPYPTR(icc, lpbiInput);
            COPYPTR(icc, lpInput);
            COPYPTR(icc, lpckid);
            COPYPTR(icc, lpdwFlags);
            COPY(icc, lFrameNum);
            COPY(icc, dwFrameSize);
            COPY(icc, dwQuality);
            COPYPTR(icc, lpbiPrev);
            COPYPTR(icc, lpPrev);

            *lParam1 = (DWORD)(icc);
            *lParam2 = sizeof(ICCOMPRESS);
        }
        break;
    case ICM_DECOMPRESS:
        {
            ICDECOMPRESS *icd = HeapAlloc(GetProcessHeap(), 0, sizeof(ICDECOMPRESS));
            ICDECOMPRESS *icd16; /* Same structure except for the pointers */

            icd16 = MapSL(*lParam1);
            ret = icd16;

            COPY(icd, dwFlags);
            COPYPTR(icd, lpbiInput);
            COPYPTR(icd, lpInput);
            COPYPTR(icd, lpbiOutput);
            COPYPTR(icd, lpOutput);
            COPY(icd, ckid);

            *lParam1 = (DWORD)(icd);
            *lParam2 = sizeof(ICDECOMPRESS);
        }
        break;
    case ICM_COMPRESS_BEGIN:
    case ICM_COMPRESS_GET_FORMAT:
    case ICM_COMPRESS_GET_SIZE:
    case ICM_COMPRESS_QUERY:
    case ICM_DECOMPRESS_GET_FORMAT:
    case ICM_DECOMPRESS_QUERY:
    case ICM_DECOMPRESS_BEGIN:
    case ICM_DECOMPRESS_SET_PALETTE:
    case ICM_DECOMPRESS_GET_PALETTE:
        *lParam1 = (DWORD)MapSL(*lParam1);
        *lParam2 = (DWORD)MapSL(*lParam2);
        break;
    case ICM_DECOMPRESSEX_QUERY:
        if ((*lParam2 != sizeof(ICDECOMPRESSEX16)) && (*lParam2 != 0))
            WARN("*lParam2 has unknown value %p\n", (ICDECOMPRESSEX16*)*lParam2);
        /* FIXME: *lParm2 is meant to be 0 or an ICDECOMPRESSEX16*, but is sizeof(ICDECOMRPESSEX16)
         * This is because of ICMessage(). Special case it?
         {
         LPVOID* addr = HeapAlloc(GetProcessHeap(), 0, 2*sizeof(LPVOID));
         addr[0] = MSVIDEO_MapICDEX16To32(lParam1);
         if (*lParam2)
         addr[1] = MSVIDEO_MapICDEX16To32(lParam2);
         else
         addr[1] = 0;
         
         ret = addr;
         }
         break;*/
    case ICM_DECOMPRESSEX_BEGIN:
    case ICM_DECOMPRESSEX:
        ret = MSVIDEO_MapICDEX16To32(lParam1);
        *lParam2 = sizeof(ICDECOMPRESSEX);
        break;
    case ICM_DRAW_BEGIN:
        {
            ICDRAWBEGIN *icdb = HeapAlloc(GetProcessHeap(), 0, sizeof(ICDRAWBEGIN));
            ICDRAWBEGIN16 *icdb16 = MapSL(*lParam1);
            ret = icdb16;

            COPY(icdb, dwFlags);
            icdb->hpal = HPALETTE_32(icdb16->hpal);
            icdb->hwnd = HWND_32(icdb16->hwnd);
            icdb->hdc = HDC_32(icdb16->hdc);
            COPY(icdb, xDst);
            COPY(icdb, yDst);
            COPY(icdb, dxDst);
            COPY(icdb, dyDst);
            COPYPTR(icdb, lpbi);
            COPY(icdb, xSrc);
            COPY(icdb, ySrc);
            COPY(icdb, dxSrc);
            COPY(icdb, dySrc);
            COPY(icdb, dwRate);
            COPY(icdb, dwScale);

            *lParam1 = (DWORD)(icdb);
            *lParam2 = sizeof(ICDRAWBEGIN);
        }
        break;
    case ICM_DRAW_SUGGESTFORMAT:
        {
            ICDRAWSUGGEST *icds = HeapAlloc(GetProcessHeap(), 0, sizeof(ICDRAWSUGGEST));
            ICDRAWSUGGEST16 *icds16 = MapSL(*lParam1);

            ret = icds16;

            COPY(icds, dwFlags);
            COPYPTR(icds, lpbiIn);
            COPYPTR(icds, lpbiSuggest);
            COPY(icds, dxSrc);
            COPY(icds, dySrc);
            COPY(icds, dxDst);
            COPY(icds, dyDst);
            icds->hicDecompressor = HIC_32(icds16->hicDecompressor);

            *lParam1 = (DWORD)(icds);
            *lParam2 = sizeof(ICDRAWSUGGEST);
        }
        break;
    case ICM_DRAW:
        {
            ICDRAW *icd = HeapAlloc(GetProcessHeap(), 0, sizeof(ICDRAW));
            ICDRAW *icd16 = MapSL(*lParam1);
            ret = icd16;

            COPY(icd, dwFlags);
            COPYPTR(icd, lpFormat);
            COPYPTR(icd, lpData);
            COPY(icd, cbData);
            COPY(icd, lTime);

            *lParam1 = (DWORD)(icd);
            *lParam2 = sizeof(ICDRAW);
        }
        break;
    case ICM_DRAW_START:
    case ICM_DRAW_STOP:
        break;
    default:
        FIXME("%d is not yet handled. Expect a crash.\n", msg);
    }
    return ret;
}

#undef COPY
#undef COPYPTR

/******************************************************************
 *		MSVIDEO_UnmapMsg16To32
 *
 *
 */
static void MSVIDEO_UnmapMsg16To32(UINT msg, LPVOID data16, LPDWORD lParam1, LPDWORD lParam2)
{
    TRACE("Unmapping %d\n", msg);

#define UNCOPY(x, y) (x##16->y = x->y);

    switch (msg) 
    {
    case ICM_GETINFO:
        {
            ICINFO *ici = (ICINFO*)(*lParam1);
            ICINFO16 *ici16 = data16;

            UNCOPY(ici, fccType);
            UNCOPY(ici, fccHandler);
            UNCOPY(ici, dwFlags);
            UNCOPY(ici, dwVersion);
            UNCOPY(ici, dwVersionICM);
            WideCharToMultiByte( CP_ACP, 0, ici->szName, -1, ici16->szName, 
                                 sizeof(ici16->szName), NULL, NULL );
            ici16->szName[sizeof(ici16->szName)-1] = 0;
            WideCharToMultiByte( CP_ACP, 0, ici->szDescription, -1, ici16->szDescription, 
                                 sizeof(ici16->szDescription), NULL, NULL );
            ici16->szDescription[sizeof(ici16->szDescription)-1] = 0;
            /* This just gives garbage for some reason - BB
               lstrcpynWtoA(ici16->szDriver, ici->szDriver, 128);*/

            HeapFree(GetProcessHeap(), 0, ici);
        }
        break;
    case ICM_DECOMPRESS_QUERY:
        /*{
          LPVOID* x = data16;
          HeapFree(GetProcessHeap(), 0, x[0]);
          if (x[1])
          HeapFree(GetProcessHeap(), 0, x[1]);
          }
          break;*/
    case ICM_COMPRESS:
    case ICM_DECOMPRESS:
    case ICM_DECOMPRESSEX_QUERY:
    case ICM_DECOMPRESSEX_BEGIN:
    case ICM_DECOMPRESSEX:
    case ICM_DRAW_BEGIN:
    case ICM_DRAW_SUGGESTFORMAT:
    case ICM_DRAW:
        HeapFree(GetProcessHeap(), 0, data16);
        break;
    default:
        ERR("Unmapping unmapped msg %d\n", msg);
    }
#undef UNCOPY
}

/***********************************************************************
 *		ICInfo				[MSVIDEO.200]
 */
BOOL16 VFWAPI ICInfo16(DWORD fccType, DWORD fccHandler, ICINFO16 *lpicinfo)
{
    BOOL16 ret;
    LPVOID lpv;
    DWORD lParam = (DWORD)lpicinfo;
    DWORD size = ((ICINFO*)(MapSL((SEGPTR)lpicinfo)))->dwSize;
    
    /* Use the mapping functions to map the ICINFO structure */
    lpv = MSVIDEO_MapMsg16To32(ICM_GETINFO, &lParam, &size);

    ret = ICInfo(fccType, fccHandler, (ICINFO*)lParam);

    MSVIDEO_UnmapMsg16To32(ICM_GETINFO, lpv, &lParam, &size);

    return ret;
}

/******************************************************************
 *		IC_Callback3216
 *
 *
 */
static  LRESULT CALLBACK  IC_Callback3216(HIC hic, HDRVR hdrv, UINT msg, DWORD lp1, DWORD lp2)
{
    WINE_HIC*   whic;
    WORD args[8];

    whic = MSVIDEO_GetHicPtr(hic);
    if (whic)
    {
        DWORD ret = 0;
        switch (msg)
        {
        case DRV_OPEN:
            lp2 = (DWORD)MapLS((void*)lp2);
            break;
        }
        args[7] = HIWORD(hic);
        args[6] = LOWORD(hic);
        args[5] = HDRVR_16(whic->hdrv);
        args[4] = msg;
        args[3] = HIWORD(lp1);
        args[2] = LOWORD(lp1);
        args[1] = HIWORD(lp2);
        args[0] = LOWORD(lp2);
        WOWCallback16Ex( whic->driverproc16, WCB16_PASCAL, sizeof(args), args, &ret );

        switch (msg)
        {
        case DRV_OPEN:
            UnMapLS(lp2);
            break;
        }
        return ret;
    }
    else return ICERR_BADHANDLE;
}

/***********************************************************************
 *		ICOpenFunction			[MSVIDEO.206]
 */
HIC16 VFWAPI ICOpenFunction16(DWORD fccType, DWORD fccHandler, UINT16 wMode, FARPROC16 lpfnHandler)
{
    HIC         hic32;

    hic32 = MSVIDEO_OpenFunction(fccType, fccHandler, wMode, 
                                 (DRIVERPROC)IC_Callback3216, (DWORD)lpfnHandler);
    return HIC_16(hic32);
}

/***********************************************************************
 *		ICSendMessage			[MSVIDEO.205]
 */
LRESULT VFWAPI ICSendMessage16(HIC16 hic, UINT16 msg, DWORD lParam1, DWORD lParam2) 
{
    LRESULT     ret = ICERR_BADHANDLE;
    WINE_HIC*   whic;

    whic = MSVIDEO_GetHicPtr(HIC_32(hic));
    if (whic)
    {
        /* we've got a 16 bit driver proc... call it directly */
        if (whic->driverproc16)
        {
            WORD args[8];
            DWORD result;

            /* FIXME: original code was passing hdrv first and hic second */
            /* but this doesn't match what IC_Callback3216 does */
            args[7] = HIWORD(hic);
            args[6] = LOWORD(hic);
            args[5] = HDRVR_16(whic->hdrv);
            args[4] = msg;
            args[3] = HIWORD(lParam1);
            args[2] = LOWORD(lParam1);
            args[1] = HIWORD(lParam2);
            args[0] = LOWORD(lParam2);
            WOWCallback16Ex( whic->driverproc16, WCB16_PASCAL, sizeof(args), args, &result );
            ret = result;
        }
        else
        {
            /* map the message for a 32 bit infrastructure, and pass it along */
            void*       data16 = MSVIDEO_MapMsg16To32(msg, &lParam1, &lParam2);
    
            ret = MSVIDEO_SendMessage(whic, msg, lParam1, lParam2);
            if (data16)
                MSVIDEO_UnmapMsg16To32(msg, data16, &lParam1, &lParam2);
        }
    }
    return ret;
}

/***********************************************************************
 *		VideoCapDriverDescAndVer	[MSVIDEO.22]
 */
DWORD WINAPI VideoCapDriverDescAndVer16(WORD nr, LPSTR buf1, WORD buf1len,
                                        LPSTR buf2, WORD buf2len)
{
    static const char version_info_spec[] = "\\StringFileInfo\\040904E4\\FileDescription";
    DWORD	verhandle;
    DWORD	infosize;
    UINT	subblocklen;
    char	*s, buf[2048], fn[260];
    LPBYTE	infobuf;
    LPVOID	subblock;
    DWORD	i, cnt = 0, lRet;
    DWORD	bufLen, fnLen;
    FILETIME	lastWrite;
    HKEY	hKey;
    BOOL        found = FALSE;

    TRACE("(%d,%p,%d,%p,%d)\n", nr, buf1, buf1len, buf2, buf2len);
    lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, HKLM_DRIVERS32, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) 
    {
	RegQueryInfoKeyA( hKey, 0, 0, 0, &cnt, 0, 0, 0, 0, 0, 0, 0);
	for (i = 0; i < cnt; i++) 
	{
	    bufLen = sizeof(buf) / sizeof(buf[0]);
	    lRet = RegEnumKeyExA(hKey, i, buf, &bufLen, 0, 0, 0, &lastWrite);
	    if (lRet != ERROR_SUCCESS) continue;
	    if (strncasecmp(buf, "vid", 3)) continue;
	    if (nr--) continue;
	    fnLen = sizeof(fn);
	    lRet = RegQueryValueExA(hKey, buf, 0, 0, (LPBYTE)fn, &fnLen);
	    if (lRet == ERROR_SUCCESS) found = TRUE;
	    break;
	}
    	RegCloseKey( hKey );
    } 

    /* search system.ini if not found in the registry */
    if (!found && GetPrivateProfileStringA("drivers32", NULL, NULL, buf, sizeof(buf), "system.ini"))
    {
	for (s = buf; *s; s += strlen(s) + 1)
	{
	    if (strncasecmp(s, "vid", 3)) continue;
	    if (nr--) continue;
	    if (GetPrivateProfileStringA("drivers32", s, NULL, fn, sizeof(fn), "system.ini"))
		found = TRUE;
	    break;
	}
    }

    if (!found)
    {
        TRACE("No more VID* entries found nr=%d\n", nr);
        return 20;
    }
    infosize = GetFileVersionInfoSizeA(fn, &verhandle);
    if (!infosize) 
    {
        TRACE("%s has no fileversioninfo.\n", fn);
        return 18;
    }
    infobuf = HeapAlloc(GetProcessHeap(), 0, infosize);
    if (GetFileVersionInfoA(fn, verhandle, infosize, infobuf)) 
    {
        /* Yes, two space behind : */
        /* FIXME: test for buflen */
        snprintf(buf2, buf2len, "Version:  %d.%d.%d.%d\n",
                ((WORD*)infobuf)[0x0f],
                ((WORD*)infobuf)[0x0e],
                ((WORD*)infobuf)[0x11],
                ((WORD*)infobuf)[0x10]
	    );
        TRACE("version of %s is %s\n", fn, buf2);
    }
    else 
    {
        TRACE("GetFileVersionInfoA failed for %s.\n", fn);
        lstrcpynA(buf2, fn, buf2len); /* msvideo.dll appears to copy fn*/
    }
    /* FIXME: language problem? */
    if (VerQueryValueA(	infobuf,
                        version_info_spec,
                        &subblock,
                        &subblocklen
            )) 
    {
        UINT copylen = min(subblocklen,buf1len-1);
        memcpy(buf1, subblock, copylen);
        buf1[copylen] = '\0';
        TRACE("VQA returned %s\n", (LPCSTR)subblock);
    }
    else 
    {
        TRACE("VQA did not return on query \\StringFileInfo\\040904E4\\FileDescription?\n");
        lstrcpynA(buf1, fn, buf1len); /* msvideo.dll appears to copy fn*/
    }
    HeapFree(GetProcessHeap(), 0, infobuf);
    return 0;
}

/******************************************************************
 *		IC_CallTo16
 *
 *
 */
static  LRESULT CALLBACK IC_CallTo16(HDRVR hdrv, HIC hic, UINT msg, LPARAM lp1, LPARAM lp2)
{
#if 0
    WINE_HIC*   whic = IC_GetPtr(hic);
    LRESULT     ret = 0;
    
    
    if (whic->driverproc) 
    {
        ret = whic->driverproc(hic, whic->hdrv, msg, lParam1, lParam2);
    }
    else
    {
        ret = SendDriverMessage(whic->hdrv, msg, lParam1, lParam2);
    }
#else
    FIXME("No 32=>16 conversion yet\n");
#endif
    return 0;
}

/**************************************************************************
 *                      DllEntryPoint (MSVIDEO.3)
 *
 * MSVIDEO DLL entry point
 *
 */
BOOL WINAPI VIDEO_LibMain(DWORD fdwReason, HINSTANCE hinstDLL, WORD ds,
                          WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    switch (fdwReason) 
    {
    case DLL_PROCESS_ATTACH:
        /* hook in our 16 bit management functions */
        pFnCallTo16 = IC_CallTo16;
        break;
    case DLL_PROCESS_DETACH:
        /* remove our 16 bit management functions */
        pFnCallTo16 = NULL;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
