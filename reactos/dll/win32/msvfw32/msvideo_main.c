/*
 * Copyright 1998 Marcus Meissner
 * Copyright 2000 Bradley Baetz
 * Copyright 2003 Michael Günnewig
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
 * FIXME: This all assumes 32 bit codecs
 *		Win95 appears to prefer 32 bit codecs, even from 16 bit code.
 *		There is the ICOpenFunction16 to worry about still, though.
 *      
 * TODO
 *      - no thread safety
 */

#include <stdio.h>
#include <string.h>

#include "msvideo_private.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"

#include "windowsx.h"

#include "wine/debug.h"

/* Drivers32 settings */
#define HKLM_DRIVERS32 "Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

static inline const char *wine_dbgstr_fcc( DWORD fcc )
{
    return wine_dbg_sprintf("%c%c%c%c", 
                            LOBYTE(LOWORD(fcc)), HIBYTE(LOWORD(fcc)),
                            LOBYTE(HIWORD(fcc)), HIBYTE(HIWORD(fcc)));
}

LRESULT (CALLBACK *pFnCallTo16)(HDRVR, HIC, UINT, LPARAM, LPARAM) = NULL;

static WINE_HIC*        MSVIDEO_FirstHic /* = NULL */;

typedef struct _reg_driver reg_driver;
struct _reg_driver
{
    DWORD       fccType;
    DWORD       fccHandler;
    DRIVERPROC  proc;
    LPWSTR      name;
    reg_driver* next;
};

static reg_driver* reg_driver_list = NULL;

/* This one is a macro such that it works for both ASCII and Unicode */
#define fourcc_to_string(str, fcc) do { \
	(str)[0] = LOBYTE(LOWORD(fcc)); \
	(str)[1] = HIBYTE(LOWORD(fcc)); \
	(str)[2] = LOBYTE(HIWORD(fcc)); \
	(str)[3] = HIBYTE(HIWORD(fcc)); \
	} while(0)

HMODULE MSVFW32_hModule;

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p,%lx,%p\n", hinst, reason, reserved);

    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            MSVFW32_hModule = (HMODULE)hinst;
            break;
    }
    return TRUE;
}

static int compare_fourcc(DWORD fcc1, DWORD fcc2)
{
  char fcc_str1[4];
  char fcc_str2[4];
  fourcc_to_string(fcc_str1, fcc1);
  fourcc_to_string(fcc_str2, fcc2);
  return strncasecmp(fcc_str1, fcc_str2, 4);
}

typedef BOOL (*enum_handler_t)(const char*, int, void*);

static BOOL enum_drivers(DWORD fccType, enum_handler_t handler, void* param)
{
    CHAR buf[2048], fccTypeStr[5], *s;
    DWORD i, cnt = 0, bufLen, lRet;
    BOOL result = FALSE;
    FILETIME lastWrite;
    HKEY hKey;

    fourcc_to_string(fccTypeStr, fccType);
    fccTypeStr[4] = '.';

    /* first, go through the registry entries */
    lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, HKLM_DRIVERS32, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) 
    {
	RegQueryInfoKeyA( hKey, 0, 0, 0, &cnt, 0, 0, 0, 0, 0, 0, 0);
	for (i = 0; i < cnt; i++) 
	{
	    bufLen = sizeof(buf) / sizeof(buf[0]);
	    lRet = RegEnumKeyExA(hKey, i, buf, &bufLen, 0, 0, 0, &lastWrite);
	    if (lRet != ERROR_SUCCESS) continue;
	    if (strncasecmp(buf, fccTypeStr, 5) || buf[9] != '=') continue;
	    if ((result = handler(buf, i, param))) break;
	}
    	RegCloseKey( hKey );
    }
    if (result) return result;

    /* if that didn't work, go through the values in system.ini */
    if (GetPrivateProfileSectionA("drivers32", buf, sizeof(buf), "system.ini")) 
    {
	for (s = buf; *s; cnt++, s += strlen(s) + 1)
	{
	    if (strncasecmp(s, fccTypeStr, 5) || s[9] != '=') continue;
	    if ((result = handler(s, cnt, param))) break;
	}
    }

    return result;
}

/******************************************************************
 *		MSVIDEO_GetHicPtr
 *
 *
 */
WINE_HIC*   MSVIDEO_GetHicPtr(HIC hic)
{
    WINE_HIC*   whic;

    for (whic = MSVIDEO_FirstHic; whic && whic->hic != hic; whic = whic->next);
    return whic;
}

/***********************************************************************
 *		VideoForWindowsVersion		[MSVFW32.2]
 *		VideoForWindowsVersion		[MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI VideoForWindowsVersion(void) 
{
    return 0x040003B6; /* 4.950 */
}

static BOOL ICInfo_enum_handler(const char *drv, int nr, void *param)
{
    ICINFO *lpicinfo = (ICINFO *)param;
    DWORD fccHandler = mmioStringToFOURCCA(drv + 5, 0);

    /* exact match of fccHandler or nth driver found */
    if ((lpicinfo->fccHandler != nr) && (lpicinfo->fccHandler != fccHandler))
	return FALSE;

    lpicinfo->fccType = mmioStringToFOURCCA(drv, 0);
    lpicinfo->fccHandler = fccHandler;
    lpicinfo->dwFlags = 0;
    lpicinfo->dwVersion = 0;
    lpicinfo->dwVersionICM = 0x104;
    lpicinfo->szName[0] = 0;
    lpicinfo->szDescription[0] = 0;
    MultiByteToWideChar(CP_ACP, 0, drv + 10, -1, lpicinfo->szDriver, 
			sizeof(lpicinfo->szDriver)/sizeof(WCHAR));

    return TRUE;
}

/***********************************************************************
 *		ICInfo				[MSVFW32.@]
 * Get information about an installable compressor. Return TRUE if there
 * is one.
 *
 * PARAMS
 *   fccType     [I] type of compressor (e.g. 'vidc')
 *   fccHandler  [I] real fcc for handler or <n>th compressor
 *   lpicinfo    [O] information about compressor
 */
BOOL VFWAPI ICInfo( DWORD fccType, DWORD fccHandler, ICINFO *lpicinfo)
{
    TRACE("(%s,%s/%08lx,%p)\n", 
          wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), fccHandler, lpicinfo);

    lpicinfo->fccHandler = fccHandler;
    return enum_drivers(fccType, ICInfo_enum_handler, lpicinfo);
}

static DWORD IC_HandleRef = 1;

/***********************************************************************
 *		ICInstall			[MSVFW32.@]
 */
BOOL VFWAPI ICInstall(DWORD fccType, DWORD fccHandler, LPARAM lParam, LPSTR szDesc, UINT wFlags) 
{
    reg_driver* driver;
    unsigned len;

    TRACE("(%s,%s,%p,%p,0x%08x)\n", wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), (void*)lParam, szDesc, wFlags);

    /* Check if a driver is already registered */
    for (driver = reg_driver_list; driver; driver = driver->next)
    {
        if (!compare_fourcc(fccType, driver->fccType) &&
            !compare_fourcc(fccHandler, driver->fccHandler))
            break;
    }
    if (driver) return FALSE;

    /* Register the driver */
    driver = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(reg_driver));
    if (!driver) goto oom;
    driver->fccType = fccType;
    driver->fccHandler = fccHandler;

    switch(wFlags)
    {
    case ICINSTALL_FUNCTION:
        driver->proc = (DRIVERPROC)lParam;
	driver->name = NULL;
        break;
    case ICINSTALL_DRIVER:
	driver->proc = NULL;
        len = MultiByteToWideChar(CP_ACP, 0, (char*)lParam, -1, NULL, 0);
        driver->name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!driver->name) goto oom;
        MultiByteToWideChar(CP_ACP, 0, (char*)lParam, -1, driver->name, len);
	break;
    default:
	ERR("Invalid flags!\n");
	HeapFree(GetProcessHeap(), 0, driver);
	return FALSE;
   }

   /* Insert our driver in the list*/
   driver->next = reg_driver_list;
   reg_driver_list = driver;
    
   return TRUE;
 oom:
   if (driver) HeapFree(GetProcessHeap(), 0, driver);
   return FALSE;
}

/***********************************************************************
 *		ICRemove			[MSVFW32.@]
 */
BOOL VFWAPI ICRemove(DWORD fccType, DWORD fccHandler, UINT wFlags) 
{
    reg_driver** pdriver;
    
    TRACE("(%s,%s,0x%08x)\n", wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), wFlags);

    /* Check if a driver is already registered */
    for (pdriver = &reg_driver_list; *pdriver; pdriver = &(*pdriver)->next)
    {
        if (!compare_fourcc(fccType, (*pdriver)->fccType) &&
            !compare_fourcc(fccHandler, (*pdriver)->fccHandler))
            break;
    }
    if (!*pdriver)
        return FALSE;

    /* Remove the driver from the list */
    *pdriver = (*pdriver)->next;
    if ((*pdriver)->name)
        HeapFree(GetProcessHeap(), 0, (*pdriver)->name);
    HeapFree(GetProcessHeap(), 0, *pdriver);
    
    return TRUE;  
}


/***********************************************************************
 *		ICOpen				[MSVFW32.@]
 * Opens an installable compressor. Return special handle.
 */
HIC VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode) 
{
    WCHAR		codecname[10];
    ICOPEN		icopen;
    HDRVR		hdrv;
    WINE_HIC*           whic;
    BOOL                bIs16;
    static const WCHAR  drv32W[] = {'d','r','i','v','e','r','s','3','2','\0'};
    reg_driver*         driver;

    TRACE("(%s,%s,0x%08x)\n", wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), wMode);

    /* Check if there is a registered driver that matches */
    driver = reg_driver_list;
    while(driver)
        if (!compare_fourcc(fccType, driver->fccType) &&
            !compare_fourcc(fccHandler, driver->fccHandler))
	    break;
        else
            driver = driver->next;

    if (driver && driver->proc)
	/* The driver has been registered at runtime with its driverproc */
        return MSVIDEO_OpenFunction(fccType, fccHandler, wMode, (DRIVERPROC)driver->proc, (DWORD)NULL);
  
    /* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the
     * same layout as ICOPEN
     */
    icopen.dwSize		= sizeof(ICOPEN);
    icopen.fccType		= fccType;
    icopen.fccHandler	        = fccHandler;
    icopen.dwVersion            = 0x00001000; /* FIXME */
    icopen.dwFlags		= wMode;
    icopen.dwError              = 0;
    icopen.pV1Reserved          = NULL;
    icopen.pV2Reserved          = NULL;
    icopen.dnDevNode            = 0; /* FIXME */
	
    if (!driver) {
        /* The driver is registered in the registry */
	fourcc_to_string(codecname, fccType);
        codecname[4] = '.';
	fourcc_to_string(codecname + 5, fccHandler);
        codecname[9] = '\0';

        hdrv = OpenDriver(codecname, drv32W, (LPARAM)&icopen);
        if (!hdrv) 
        {
            if (fccType == streamtypeVIDEO) 
            {
		codecname[0] = 'v';
		codecname[1] = 'i';
		codecname[2] = 'd';
		codecname[3] = 'c';

		fccType = ICTYPE_VIDEO;
                hdrv = OpenDriver(codecname, drv32W, (LPARAM)&icopen);
	    }
            if (!hdrv)
                return 0;
	}
    } else {
        /* The driver has been registered at runtime with its name */
        hdrv = OpenDriver(driver->name, NULL, (LPARAM)&icopen);
        if (!hdrv) 
            return 0; 
    }
    bIs16 = GetDriverFlags(hdrv) & 0x10000000; /* undocumented flag: WINE_GDF_16BIT */

    if (bIs16 && !pFnCallTo16)
    {
        FIXME("Got a 16 bit driver, but no 16 bit support in msvfw\n");
        return 0;
    }
    whic = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_HIC));
    if (!whic)
    {
        CloseDriver(hdrv, 0, 0);
        return FALSE;
    }
    whic->hdrv          = hdrv;
    /* FIXME: is the signature the real one ? */
    whic->driverproc    = bIs16 ? (DRIVERPROC)pFnCallTo16 : NULL;
    whic->driverproc16  = 0;
    whic->type          = fccType;
    whic->handler       = fccHandler;
    while (MSVIDEO_GetHicPtr(HIC_32(IC_HandleRef)) != NULL) IC_HandleRef++;
    whic->hic           = HIC_32(IC_HandleRef++);
    whic->next          = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		MSVIDEO_OpenFunction
 */
HIC MSVIDEO_OpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, 
                         DRIVERPROC lpfnHandler, DWORD lpfnHandler16) 
{
    ICOPEN      icopen;
    WINE_HIC*   whic;

    TRACE("(%s,%s,%d,%p,%08lx)\n", 
          wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), wMode, lpfnHandler, lpfnHandler16);

    icopen.dwSize		= sizeof(ICOPEN);
    icopen.fccType		= fccType;
    icopen.fccHandler	        = fccHandler;
    icopen.dwVersion            = 0x00001000; /* FIXME */
    icopen.dwFlags		= wMode;
    icopen.dwError              = 0;
    icopen.pV1Reserved          = NULL;
    icopen.pV2Reserved          = NULL;
    icopen.dnDevNode            = 0; /* FIXME */

    whic = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_HIC));
    if (!whic) return 0;

    whic->driverproc   = lpfnHandler;
    whic->driverproc16 = lpfnHandler16;
    while (MSVIDEO_GetHicPtr(HIC_32(IC_HandleRef)) != NULL) IC_HandleRef++;
    whic->hic          = HIC_32(IC_HandleRef++);
    whic->next         = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    /* Now try opening/loading the driver. Taken from DRIVER_AddToList */
    /* What if the function is used more than once? */

    if (MSVIDEO_SendMessage(whic, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) 
    {
        WARN("DRV_LOAD failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        HeapFree(GetProcessHeap(), 0, whic);
        return 0;
    }
    /* return value is not checked */
    MSVIDEO_SendMessage(whic, DRV_ENABLE, 0L, 0L);

    whic->driverId = (DWORD)MSVIDEO_SendMessage(whic, DRV_OPEN, 0, (DWORD)&icopen);
    /* FIXME: What should we put here? */
    whic->hdrv = (HDRVR)0;
    
    if (whic->driverId == 0) 
    {
        WARN("DRV_OPEN failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        HeapFree(GetProcessHeap(), 0, whic);
        return 0;
    }

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW32.@]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler) 
{
    return MSVIDEO_OpenFunction(fccType, fccHandler, wMode, (DRIVERPROC)lpfnHandler, 0);
}

/***********************************************************************
 *		ICGetInfo			[MSVFW32.@]
 */
LRESULT VFWAPI ICGetInfo(HIC hic, ICINFO *picinfo, DWORD cb) 
{
    LRESULT	ret;
    WINE_HIC*   whic = MSVIDEO_GetHicPtr(hic);

    TRACE("(%p,%p,%ld)\n", hic, picinfo, cb);

    whic = MSVIDEO_GetHicPtr(hic);
    if (!whic) return ICERR_BADHANDLE;
    if (!picinfo) return MMSYSERR_INVALPARAM;

    /* (WS) The field szDriver should be initialized because the driver 
     * is not obliged and often will not do it. Some applications, like
     * VirtualDub, rely on this field and will occasionally crash if it
     * goes unitialized.
     */
    if (cb >= sizeof(ICINFO)) picinfo->szDriver[0] = '\0';

    ret = ICSendMessage(hic, ICM_GETINFO, (DWORD)picinfo, cb);

    /* (WS) When szDriver was not supplied by the driver itself, apparently 
     * Windows will set its value equal to the driver file name. This can
     * be obtained from the registry as we do here.
     */
    if (cb >= sizeof(ICINFO) && picinfo->szDriver[0] == 0)
    {
        ICINFO  ii;

        memset(&ii, 0, sizeof(ii));
        ii.dwSize = sizeof(ii);
        ICInfo(picinfo->fccType, picinfo->fccHandler, &ii);
        lstrcpyW(picinfo->szDriver, ii.szDriver);
    }

    TRACE("	-> 0x%08lx\n", ret);
    return ret;
}

typedef struct {
    DWORD fccType;
    DWORD fccHandler;
    LPBITMAPINFOHEADER lpbiIn;
    LPBITMAPINFOHEADER lpbiOut;
    WORD wMode;
    DWORD querymsg;
    HIC hic;
} driver_info_t;

static HIC try_driver(driver_info_t *info)
{
    HIC   hic;

    if ((hic = ICOpen(info->fccType, info->fccHandler, info->wMode))) 
    {
	if (!ICSendMessage(hic, info->querymsg, (DWORD)info->lpbiIn, (DWORD)info->lpbiOut))
	    return hic;
	ICClose(hic);
    }
    return 0;
}

static BOOL ICLocate_enum_handler(const char *drv, int nr, void *param)
{
    driver_info_t *info = (driver_info_t *)param;
    info->fccHandler = mmioStringToFOURCCA(drv + 5, 0);
    info->hic = try_driver(info);
    return info->hic != 0;
}

/***********************************************************************
 *		ICLocate			[MSVFW32.@]
 */
HIC VFWAPI ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
                    LPBITMAPINFOHEADER lpbiOut, WORD wMode)
{
    driver_info_t info;

    TRACE("(%s,%s,%p,%p,0x%04x)\n", 
          wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), lpbiIn, lpbiOut, wMode);

    info.fccType = fccType;
    info.fccHandler = fccHandler;
    info.lpbiIn = lpbiIn;
    info.lpbiOut = lpbiOut;
    info.wMode = wMode;

    switch (wMode) 
    {
    case ICMODE_FASTCOMPRESS:
    case ICMODE_COMPRESS:
        info.querymsg = ICM_COMPRESS_QUERY;
        break;
    case ICMODE_FASTDECOMPRESS:
    case ICMODE_DECOMPRESS:
        info.querymsg = ICM_DECOMPRESS_QUERY;
        break;
    case ICMODE_DRAW:
        info.querymsg = ICM_DRAW_QUERY;
        break;
    default:
        WARN("Unknown mode (%d)\n", wMode);
        return 0;
    }

    /* Easy case: handler/type match, we just fire a query and return */
    info.hic = try_driver(&info);
    /* If it didn't work, try each driver in turn. 32 bit codecs only. */
    /* FIXME: Move this to an init routine? */
    if (!info.hic) enum_drivers(fccType, ICLocate_enum_handler, &info);

    if (info.hic) 
    {
        TRACE("=> %p\n", info.hic);
	return info.hic;
    }

    if (fccType == streamtypeVIDEO) 
        return ICLocate(ICTYPE_VIDEO, fccHandler, lpbiIn, lpbiOut, wMode);
    
    WARN("(%s,%s,%p,%p,0x%04x) not found!\n",
         wine_dbgstr_fcc(fccType), wine_dbgstr_fcc(fccHandler), lpbiIn, lpbiOut, wMode);
    return 0;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVFW32.@]
 */
HIC VFWAPI ICGetDisplayFormat(
	HIC hic,LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut,
	INT depth,INT dx,INT dy)
{
	HIC	tmphic = hic;

	TRACE("(%p,%p,%p,%d,%d,%d)!\n",hic,lpbiIn,lpbiOut,depth,dx,dy);

	if (!tmphic) {
		tmphic=ICLocate(ICTYPE_VIDEO,0,lpbiIn,NULL,ICMODE_DECOMPRESS);
		if (!tmphic)
			return tmphic;
	}
	if ((dy == lpbiIn->biHeight) && (dx == lpbiIn->biWidth))
		dy = dx = 0; /* no resize needed */

	/* Can we decompress it ? */
	if (ICDecompressQuery(tmphic,lpbiIn,NULL) != 0)
		goto errout; /* no, sorry */

	ICDecompressGetFormat(tmphic,lpbiIn,lpbiOut);

	if (lpbiOut->biCompression != 0) {
	   FIXME("Ooch, how come decompressor outputs compressed data (%ld)??\n",
			 lpbiOut->biCompression);
	}
	if (lpbiOut->biSize < sizeof(*lpbiOut)) {
	   FIXME("Ooch, size of output BIH is too small (%ld)\n",
			 lpbiOut->biSize);
	   lpbiOut->biSize = sizeof(*lpbiOut);
	}
	if (!depth) {
		HDC	hdc;

		hdc = GetDC(0);
		depth = GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES);
		ReleaseDC(0,hdc);
		if (depth==15)	depth = 16;
		if (depth<8)	depth =  8;
	}
	if (lpbiIn->biBitCount == 8)
		depth = 8;

	TRACE("=> %p\n", tmphic);
	return tmphic;
errout:
	if (hic!=tmphic)
		ICClose(tmphic);

	TRACE("=> 0\n");
	return 0;
}

/***********************************************************************
 *		ICCompress			[MSVFW32.@]
 */
DWORD VFWAPIV
ICCompress(
	HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev)
{
	ICCOMPRESS	iccmp;

	TRACE("(%p,%ld,%p,%p,%p,%p,...)\n",hic,dwFlags,lpbiOutput,lpData,lpbiInput,lpBits);

	iccmp.dwFlags		= dwFlags;

	iccmp.lpbiOutput	= lpbiOutput;
	iccmp.lpOutput		= lpData;
	iccmp.lpbiInput		= lpbiInput;
	iccmp.lpInput		= lpBits;

	iccmp.lpckid		= lpckid;
	iccmp.lpdwFlags		= lpdwFlags;
	iccmp.lFrameNum		= lFrameNum;
	iccmp.dwFrameSize	= dwFrameSize;
	iccmp.dwQuality		= dwQuality;
	iccmp.lpbiPrev		= lpbiPrev;
	iccmp.lpPrev		= lpPrev;
	return ICSendMessage(hic,ICM_COMPRESS,(DWORD)&iccmp,sizeof(iccmp));
}

/***********************************************************************
 *		ICDecompress			[MSVFW32.@]
 */
DWORD VFWAPIV  ICDecompress(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,
				LPVOID lpData,LPBITMAPINFOHEADER lpbi,LPVOID lpBits)
{
	ICDECOMPRESS	icd;
	DWORD ret;

	TRACE("(%p,%ld,%p,%p,%p,%p)\n",hic,dwFlags,lpbiFormat,lpData,lpbi,lpBits);

	TRACE("lpBits[0] == %lx\n",((LPDWORD)lpBits)[0]);

	icd.dwFlags	= dwFlags;
	icd.lpbiInput	= lpbiFormat;
	icd.lpInput	= lpData;

	icd.lpbiOutput	= lpbi;
	icd.lpOutput	= lpBits;
	icd.ckid	= 0;
	ret = ICSendMessage(hic,ICM_DECOMPRESS,(DWORD)&icd,sizeof(ICDECOMPRESS));

	TRACE("lpBits[0] == %lx\n",((LPDWORD)lpBits)[0]);

	TRACE("-> %ld\n",ret);

	return ret;
}


/***********************************************************************
 *		ICCompressorChoose   [MSVFW32.@]
 */
BOOL VFWAPI ICCompressorChoose(HWND hwnd, UINT uiFlags, LPVOID pvIn,
                               LPVOID lpData, PCOMPVARS pc, LPSTR lpszTitle)
{
  FIXME("(%p,0x%X,%p,%p,%p,%s),stub!\n",hwnd,uiFlags,pvIn,lpData,pc,lpszTitle);

  if (pc == NULL || pc->cbSize != sizeof(COMPVARS))
    return FALSE;

  if ((pc->dwFlags & ICMF_COMPVARS_VALID) == 0) {
    pc->dwFlags   = 0;
    pc->fccType   = pc->fccHandler = 0;
    pc->hic       = NULL;
    pc->lpbiOut   = NULL;
    pc->lpBitsOut = pc->lpBitsPrev = pc->lpState = NULL;
    pc->lQ        = ICQUALITY_DEFAULT;
    pc->lKey      = -1;
    pc->lDataRate = 300; /* kB */
    pc->lpState   = NULL;
    pc->cbState   = 0;
  }
  if (pc->fccType == 0)
    pc->fccType = ICTYPE_VIDEO;

  /* FIXME */
  
  return FALSE;
}


/***********************************************************************
 *		ICCompressorFree   [MSVFW32.@]
 */
void VFWAPI ICCompressorFree(PCOMPVARS pc)
{
  TRACE("(%p)\n",pc);

  if (pc != NULL && pc->cbSize == sizeof(COMPVARS)) {
    if (pc->hic != NULL) {
      ICClose(pc->hic);
      pc->hic = NULL;
    }
    if (pc->lpbiOut != NULL) {
      GlobalFreePtr(pc->lpbiOut);
      pc->lpbiOut = NULL;
    }
    if (pc->lpBitsOut != NULL) {
      GlobalFreePtr(pc->lpBitsOut);
      pc->lpBitsOut = NULL;
    }
    if (pc->lpBitsPrev != NULL) {
      GlobalFreePtr(pc->lpBitsPrev);
      pc->lpBitsPrev = NULL;
    }
    if (pc->lpState != NULL) {
      GlobalFreePtr(pc->lpBitsPrev);
      pc->lpState = NULL;
    }
    pc->dwFlags = 0;
  }
}


/******************************************************************
 *		MSVIDEO_SendMessage
 *
 *
 */
LRESULT MSVIDEO_SendMessage(WINE_HIC* whic, UINT msg, DWORD lParam1, DWORD lParam2)
{
    LRESULT     ret;
    
#define XX(x) case x: TRACE("(%p,"#x",0x%08lx,0x%08lx)\n",whic,lParam1,lParam2); break;
    
    switch (msg) {
        /* DRV_* */
        XX(DRV_LOAD);
        XX(DRV_ENABLE);
        XX(DRV_OPEN);
        XX(DRV_CLOSE);
        XX(DRV_DISABLE);
        XX(DRV_FREE);
        /* ICM_RESERVED+X */
        XX(ICM_ABOUT);
        XX(ICM_CONFIGURE);
        XX(ICM_GET);
        XX(ICM_GETINFO);
        XX(ICM_GETDEFAULTQUALITY);
        XX(ICM_GETQUALITY);
        XX(ICM_GETSTATE);
        XX(ICM_SETQUALITY);
        XX(ICM_SET);
        XX(ICM_SETSTATE);
        /* ICM_USER+X */
        XX(ICM_COMPRESS_FRAMES_INFO);
        XX(ICM_COMPRESS_GET_FORMAT);
        XX(ICM_COMPRESS_GET_SIZE);
        XX(ICM_COMPRESS_QUERY);
        XX(ICM_COMPRESS_BEGIN);
        XX(ICM_COMPRESS);
        XX(ICM_COMPRESS_END);
        XX(ICM_DECOMPRESS_GET_FORMAT);
        XX(ICM_DECOMPRESS_QUERY);
        XX(ICM_DECOMPRESS_BEGIN);
        XX(ICM_DECOMPRESS);
        XX(ICM_DECOMPRESS_END);
        XX(ICM_DECOMPRESS_SET_PALETTE);
        XX(ICM_DECOMPRESS_GET_PALETTE);
        XX(ICM_DRAW_QUERY);
        XX(ICM_DRAW_BEGIN);
        XX(ICM_DRAW_GET_PALETTE);
        XX(ICM_DRAW_START);
        XX(ICM_DRAW_STOP);
        XX(ICM_DRAW_END);
        XX(ICM_DRAW_GETTIME);
        XX(ICM_DRAW);
        XX(ICM_DRAW_WINDOW);
        XX(ICM_DRAW_SETTIME);
        XX(ICM_DRAW_REALIZE);
        XX(ICM_DRAW_FLUSH);
        XX(ICM_DRAW_RENDERBUFFER);
        XX(ICM_DRAW_START_PLAY);
        XX(ICM_DRAW_STOP_PLAY);
        XX(ICM_DRAW_SUGGESTFORMAT);
        XX(ICM_DRAW_CHANGEPALETTE);
        XX(ICM_GETBUFFERSWANTED);
        XX(ICM_GETDEFAULTKEYFRAMERATE);
        XX(ICM_DECOMPRESSEX_BEGIN);
        XX(ICM_DECOMPRESSEX_QUERY);
        XX(ICM_DECOMPRESSEX);
        XX(ICM_DECOMPRESSEX_END);
        XX(ICM_SET_STATUS_PROC);
    default:
        FIXME("(%p,0x%08lx,0x%08lx,0x%08lx) unknown message\n",whic,(DWORD)msg,lParam1,lParam2);
    }
    
#undef XX
    
    if (whic->driverproc) {
	/* dwDriverId parameter is the value returned by the DRV_OPEN */
        ret = whic->driverproc(whic->driverId, whic->hdrv, msg, lParam1, lParam2);
    } else {
        ret = SendDriverMessage(whic->hdrv, msg, lParam1, lParam2);
    }

    TRACE("	-> 0x%08lx\n", ret);
    return ret;
}

/***********************************************************************
 *		ICSendMessage			[MSVFW32.@]
 */
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2) 
{
    WINE_HIC*   whic = MSVIDEO_GetHicPtr(hic);

    if (!whic) return ICERR_BADHANDLE;
    return MSVIDEO_SendMessage(whic, msg, lParam1, lParam2);
}

/***********************************************************************
 *		ICDrawBegin		[MSVFW32.@]
 */
DWORD VFWAPIV ICDrawBegin(
	HIC                hic,     /* [in] */
	DWORD              dwFlags, /* [in] flags */
	HPALETTE           hpal,    /* [in] palette to draw with */
	HWND               hwnd,    /* [in] window to draw to */
	HDC                hdc,     /* [in] HDC to draw to */
	INT                xDst,    /* [in] destination rectangle */
	INT                yDst,    /* [in] */
	INT                dxDst,   /* [in] */
	INT                dyDst,   /* [in] */
	LPBITMAPINFOHEADER lpbi,    /* [in] format of frame to draw */
	INT                xSrc,    /* [in] source rectangle */
	INT                ySrc,    /* [in] */
	INT                dxSrc,   /* [in] */
	INT                dySrc,   /* [in] */
	DWORD              dwRate,  /* [in] frames/second = (dwRate/dwScale) */
	DWORD              dwScale) /* [in] */
{

	ICDRAWBEGIN	icdb;

	TRACE("(%p,%ld,%p,%p,%p,%u,%u,%u,%u,%p,%u,%u,%u,%u,%ld,%ld)\n",
		  hic, dwFlags, hpal, hwnd, hdc, xDst, yDst, dxDst, dyDst,
		  lpbi, xSrc, ySrc, dxSrc, dySrc, dwRate, dwScale);

	icdb.dwFlags = dwFlags;
	icdb.hpal = hpal;
	icdb.hwnd = hwnd;
	icdb.hdc = hdc;
	icdb.xDst = xDst;
	icdb.yDst = yDst;
	icdb.dxDst = dxDst;
	icdb.dyDst = dyDst;
	icdb.lpbi = lpbi;
	icdb.xSrc = xSrc;
	icdb.ySrc = ySrc;
	icdb.dxSrc = dxSrc;
	icdb.dySrc = dySrc;
	icdb.dwRate = dwRate;
	icdb.dwScale = dwScale;
	return ICSendMessage(hic,ICM_DRAW_BEGIN,(DWORD)&icdb,sizeof(icdb));
}

/***********************************************************************
 *		ICDraw			[MSVFW32.@]
 */
DWORD VFWAPIV ICDraw(HIC hic, DWORD dwFlags, LPVOID lpFormat, LPVOID lpData, DWORD cbData, LONG lTime) {
	ICDRAW	icd;

	TRACE("(%p,%ld,%p,%p,%ld,%ld)\n",hic,dwFlags,lpFormat,lpData,cbData,lTime);

	icd.dwFlags = dwFlags;
	icd.lpFormat = lpFormat;
	icd.lpData = lpData;
	icd.cbData = cbData;
	icd.lTime = lTime;

	return ICSendMessage(hic,ICM_DRAW,(DWORD)&icd,sizeof(icd));
}

/***********************************************************************
 *		ICClose			[MSVFW32.@]
 */
LRESULT WINAPI ICClose(HIC hic)
{
    WINE_HIC* whic = MSVIDEO_GetHicPtr(hic);
    WINE_HIC** p;

    TRACE("(%p)\n",hic);

    if (!whic) return ICERR_BADHANDLE;

    if (whic->driverproc) 
    {
        MSVIDEO_SendMessage(whic, DRV_CLOSE, 0, 0);
        MSVIDEO_SendMessage(whic, DRV_DISABLE, 0, 0);
        MSVIDEO_SendMessage(whic, DRV_FREE, 0, 0);
    }
    else
    {
        CloseDriver(whic->hdrv, 0, 0);
    }

    /* remove whic from list */
    for (p = &MSVIDEO_FirstHic; *p != NULL; p = &((*p)->next))
    {
        if ((*p) == whic)
        {
            *p = whic->next;
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, whic);
    return 0;
}



/***********************************************************************
 *		ICImageCompress	[MSVFW32.@]
 */
HANDLE VFWAPI ICImageCompress(
	HIC hic, UINT uiFlags,
	LPBITMAPINFO lpbiIn, LPVOID lpBits,
	LPBITMAPINFO lpbiOut, LONG lQuality,
	LONG* plSize)
{
	FIXME("(%p,%08x,%p,%p,%p,%ld,%p)\n",
		hic, uiFlags, lpbiIn, lpBits, lpbiOut, lQuality, plSize);

	return NULL;
}

/***********************************************************************
 *		ICImageDecompress	[MSVFW32.@]
 */

HANDLE VFWAPI ICImageDecompress(
	HIC hic, UINT uiFlags, LPBITMAPINFO lpbiIn,
	LPVOID lpBits, LPBITMAPINFO lpbiOut)
{
	HGLOBAL	hMem = NULL;
	BYTE*	pMem = NULL;
	BOOL	bReleaseIC = FALSE;
	BYTE*	pHdr = NULL;
	LONG	cbHdr = 0;
	BOOL	bSucceeded = FALSE;
	BOOL	bInDecompress = FALSE;
	DWORD	biSizeImage;

	TRACE("(%p,%08x,%p,%p,%p)\n",
		hic, uiFlags, lpbiIn, lpBits, lpbiOut);

	if ( hic == NULL )
	{
		hic = ICDecompressOpen( ICTYPE_VIDEO, 0, &lpbiIn->bmiHeader, (lpbiOut != NULL) ? &lpbiOut->bmiHeader : NULL );
		if ( hic == NULL )
		{
			WARN("no handler\n" );
			goto err;
		}
		bReleaseIC = TRUE;
	}
	if ( uiFlags != 0 )
	{
		FIXME( "unknown flag %08x\n", uiFlags );
		goto err;
	}
	if ( lpbiIn == NULL || lpBits == NULL )
	{
		WARN("invalid argument\n");
		goto err;
	}

	if ( lpbiOut != NULL )
	{
		if ( lpbiOut->bmiHeader.biSize != sizeof(BITMAPINFOHEADER) )
			goto err;
		cbHdr = sizeof(BITMAPINFOHEADER);
		if ( lpbiOut->bmiHeader.biCompression == 3 )
			cbHdr += sizeof(DWORD)*3;
		else
		if ( lpbiOut->bmiHeader.biBitCount <= 8 )
		{
			if ( lpbiOut->bmiHeader.biClrUsed == 0 )
				cbHdr += sizeof(RGBQUAD) * (1<<lpbiOut->bmiHeader.biBitCount);
			else
				cbHdr += sizeof(RGBQUAD) * lpbiOut->bmiHeader.biClrUsed;
		}
	}
	else
	{
		TRACE( "get format\n" );

		cbHdr = ICDecompressGetFormatSize(hic,lpbiIn);
		if ( cbHdr < sizeof(BITMAPINFOHEADER) )
			goto err;
		pHdr = (BYTE*)HeapAlloc(GetProcessHeap(),0,cbHdr+sizeof(RGBQUAD)*256);
		if ( pHdr == NULL )
			goto err;
		ZeroMemory( pHdr, cbHdr+sizeof(RGBQUAD)*256 );
		if ( ICDecompressGetFormat( hic, lpbiIn, (BITMAPINFO*)pHdr ) != ICERR_OK )
			goto err;
		lpbiOut = (BITMAPINFO*)pHdr;
		if ( lpbiOut->bmiHeader.biBitCount <= 8 &&
			 ICDecompressGetPalette( hic, lpbiIn, lpbiOut ) != ICERR_OK &&
			 lpbiIn->bmiHeader.biBitCount == lpbiOut->bmiHeader.biBitCount )
		{
			if ( lpbiIn->bmiHeader.biClrUsed == 0 )
				memcpy( lpbiOut->bmiColors, lpbiIn->bmiColors, sizeof(RGBQUAD)*(1<<lpbiOut->bmiHeader.biBitCount) );
			else
				memcpy( lpbiOut->bmiColors, lpbiIn->bmiColors, sizeof(RGBQUAD)*lpbiIn->bmiHeader.biClrUsed );
		}
		if ( lpbiOut->bmiHeader.biBitCount <= 8 &&
			 lpbiOut->bmiHeader.biClrUsed == 0 )
			lpbiOut->bmiHeader.biClrUsed = 1<<lpbiOut->bmiHeader.biBitCount;

		lpbiOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		cbHdr = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*lpbiOut->bmiHeader.biClrUsed;
	}

	biSizeImage = lpbiOut->bmiHeader.biSizeImage;
	if ( biSizeImage == 0 )
		biSizeImage = ((((lpbiOut->bmiHeader.biWidth * lpbiOut->bmiHeader.biBitCount + 7) >> 3) + 3) & (~3)) * abs(lpbiOut->bmiHeader.biHeight);

	TRACE( "call ICDecompressBegin\n" );

	if ( ICDecompressBegin( hic, lpbiIn, lpbiOut ) != ICERR_OK )
		goto err;
	bInDecompress = TRUE;

	TRACE( "cbHdr %ld, biSizeImage %ld\n", cbHdr, biSizeImage );

	hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_ZEROINIT, cbHdr + biSizeImage );
	if ( hMem == NULL )
	{
		WARN( "out of memory\n" );
		goto err;
	}
	pMem = (BYTE*)GlobalLock( hMem );
	if ( pMem == NULL )
		goto err;
	memcpy( pMem, lpbiOut, cbHdr );

	TRACE( "call ICDecompress\n" );
	if ( ICDecompress( hic, 0, &lpbiIn->bmiHeader, lpBits, &lpbiOut->bmiHeader, pMem+cbHdr ) != ICERR_OK )
		goto err;

	bSucceeded = TRUE;
err:
	if ( bInDecompress )
		ICDecompressEnd( hic );
	if ( bReleaseIC )
		ICClose(hic);
	if ( pHdr != NULL )
		HeapFree(GetProcessHeap(),0,pHdr);
	if ( pMem != NULL )
		GlobalUnlock( hMem );
	if ( !bSucceeded && hMem != NULL )
	{
		GlobalFree(hMem); hMem = NULL;
	}

	return (HANDLE)hMem;
}

static BOOL GetFileNamePreview(LPVOID lpofn,BOOL bSave,BOOL bUnicode)
{
  CHAR    szFunctionName[20];
  BOOL    (*fnGetFileName)(LPVOID);
  HMODULE hComdlg32;
  BOOL    ret;

  FIXME("(%p,%d,%d), semi-stub!\n",lpofn,bSave,bUnicode);

  lstrcpyA(szFunctionName, (bSave ? "GetSaveFileName" : "GetOpenFileName"));
  lstrcatA(szFunctionName, (bUnicode ? "W" : "A"));

  hComdlg32 = LoadLibraryA("COMDLG32.DLL");
  if (hComdlg32 == NULL)
    return FALSE;

  fnGetFileName = (LPVOID)GetProcAddress(hComdlg32, szFunctionName);
  if (fnGetFileName == NULL)
    return FALSE;

  /* FIXME: need to add OFN_ENABLEHOOK and our own handler */
  ret = fnGetFileName(lpofn);

  FreeLibrary(hComdlg32);
  return ret;
}

/***********************************************************************
 *		GetOpenFileNamePreviewA	[MSVFW32.@]
 */
BOOL WINAPI GetOpenFileNamePreviewA(LPOPENFILENAMEA lpofn)
{
  FIXME("(%p), semi-stub!\n", lpofn);

  return GetFileNamePreview(lpofn, FALSE, FALSE);
}

/***********************************************************************
 *		GetOpenFileNamePreviewW	[MSVFW32.@]
 */
BOOL WINAPI GetOpenFileNamePreviewW(LPOPENFILENAMEW lpofn)
{
  FIXME("(%p), semi-stub!\n", lpofn);

  return GetFileNamePreview(lpofn, FALSE, TRUE);
}

/***********************************************************************
 *		GetSaveFileNamePreviewA	[MSVFW32.@]
 */
BOOL WINAPI GetSaveFileNamePreviewA(LPOPENFILENAMEA lpofn)
{
  FIXME("(%p), semi-stub!\n", lpofn);

  return GetFileNamePreview(lpofn, TRUE, FALSE);
}

/***********************************************************************
 *		GetSaveFileNamePreviewW	[MSVFW32.@]
 */
BOOL WINAPI GetSaveFileNamePreviewW(LPOPENFILENAMEW lpofn)
{
  FIXME("(%p), semi-stub!\n", lpofn);

  return GetFileNamePreview(lpofn, TRUE, TRUE);
}
