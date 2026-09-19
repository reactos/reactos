/*
 * Copyright 1998 Marcus Meissner
 * Copyright 2000 Bradley Baetz
 * Copyright 2003 Michael Günnewig
 * Copyright 2005 Dmitry Timoshkov
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
 * FIXME: This all assumes 32 bit codecs
 *		Win95 appears to prefer 32 bit codecs, even from 16 bit code.
 *		There is the ICOpenFunction16 to worry about still, though.
 *      
 * TODO
 *      - no thread safety
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wingdi.h"
#include "winternl.h"
#include "winuser.h"
#include "commdlg.h"
#include "vfw.h"
#include "msvideo_private.h"
#include "wine/debug.h"
#include "wine/list.h"

/* Drivers32 settings */
#define HKLM_DRIVERS32 "Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

/* This one is a macro in order to work for both ASCII and Unicode */
#define fourcc_to_string(str, fcc) do { \
	(str)[0] = LOBYTE(LOWORD(fcc)); \
	(str)[1] = HIBYTE(LOWORD(fcc)); \
	(str)[2] = LOBYTE(HIWORD(fcc)); \
	(str)[3] = HIBYTE(HIWORD(fcc)); \
	} while(0)

static const char *wine_dbgstr_icerr( int ret )
{
    const char *str;
    if (ret <= ICERR_CUSTOM)
        return wine_dbg_sprintf("ICERR_CUSTOM (%d)", ret);
#define XX(x) case (x): str = #x; break
    switch (ret)
    {
        XX(ICERR_OK);
        XX(ICERR_DONTDRAW);
        XX(ICERR_NEWPALETTE);
        XX(ICERR_GOTOKEYFRAME);
        XX(ICERR_STOPDRAWING);
        XX(ICERR_UNSUPPORTED);
        XX(ICERR_BADFORMAT);
        XX(ICERR_MEMORY);
        XX(ICERR_INTERNAL);
        XX(ICERR_BADFLAGS);
        XX(ICERR_BADPARAM);
        XX(ICERR_BADSIZE);
        XX(ICERR_BADHANDLE);
        XX(ICERR_CANTUPDATE);
        XX(ICERR_ABORT);
        XX(ICERR_ERROR);
        XX(ICERR_BADBITDEPTH);
        XX(ICERR_BADIMAGESIZE);
        default: str = wine_dbg_sprintf("UNKNOWN (%d)", ret);
    }
#undef XX
    return str;
}

static WINE_HIC*        MSVIDEO_FirstHic /* = NULL */;

struct reg_driver
{
    DWORD       fccType;
    DWORD       fccHandler;
    DRIVERPROC  proc;
    struct list entry;
};

static struct list reg_driver_list = LIST_INIT(reg_driver_list);

HMODULE MSVFW32_hModule;

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p,%lx,%p\n", hinst, reason, reserved);

    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            MSVFW32_hModule = hinst;
            break;
    }
    return TRUE;
}

/******************************************************************
 *		MSVIDEO_SendMessage
 *
 *
 */
static LRESULT MSVIDEO_SendMessage(WINE_HIC* whic, UINT msg, DWORD_PTR lParam1, DWORD_PTR lParam2)
{
    LRESULT     ret;

#define XX(x) case x: TRACE("(%p,"#x",0x%08Ix,0x%08Ix)\n",whic,lParam1,lParam2); break

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
        FIXME("(%p,0x%08x,0x%08Ix,0x%08Ix) unknown message\n",whic,msg,lParam1,lParam2);
    }

#undef XX

    if (whic->driverproc) {
	/* dwDriverId parameter is the value returned by the DRV_OPEN */
        ret = whic->driverproc(whic->driverId, whic->hdrv, msg, lParam1, lParam2);
    } else {
        ret = SendDriverMessage(whic->hdrv, msg, lParam1, lParam2);
    }

    TRACE("	-> %s\n", wine_dbgstr_icerr(ret));
    return ret;
}

static int compare_fourcc(DWORD fcc1, DWORD fcc2)
{
  char fcc_str1[4];
  char fcc_str2[4];
  fourcc_to_string(fcc_str1, fcc1);
  fourcc_to_string(fcc_str2, fcc2);
  return _strnicmp(fcc_str1, fcc_str2, 4);
}

static DWORD get_size_image(LONG width, LONG height, WORD depth)
{
    DWORD ret = width * depth;
    ret = (ret + 7) / 8;    /* divide by byte size, rounding up */
    ret = (ret + 3) & ~3;   /* align to 4 bytes */
    ret *= abs(height);
    return ret;
}

/******************************************************************
 *		MSVIDEO_GetHicPtr
 *
 *
 */
static WINE_HIC*   MSVIDEO_GetHicPtr(HIC hic)
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
BOOL VFWAPI ICInfo(DWORD type, DWORD handler, ICINFO *info)
{
    char name_buf[10], buf[2048];
    DWORD ret_type, ret_handler;
    struct reg_driver *driver;
    DWORD i, count = 0;
    LONG res;
    HKEY key;

    TRACE("type %s, handler %s, info %p.\n",
            debugstr_fourcc(type), debugstr_fourcc(handler), info);

    memset(info, 0, sizeof(*info));
    info->dwSize = sizeof(*info);
    info->dwVersionICM = ICVERSION;

    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE, HKLM_DRIVERS32, 0, KEY_QUERY_VALUE, &key))
    {
        i = 0;
        for (;;)
        {
            DWORD name_len = ARRAY_SIZE(name_buf), driver_len = ARRAY_SIZE(info->szDriver);

            res = RegEnumValueA(key, i++, name_buf, &name_len, 0, 0, (BYTE *)buf, &driver_len);
            if (res == ERROR_NO_MORE_ITEMS) break;

            if (name_len != 9 || name_buf[4] != '.') continue;
            ret_type = mmioStringToFOURCCA(name_buf, 0);
            ret_handler = mmioStringToFOURCCA(name_buf + 5, 0);
            if (type && compare_fourcc(type, ret_type)) continue;
            if (compare_fourcc(handler, ret_handler) && handler != count++) continue;

            info->fccType = ret_type;
            info->fccHandler = ret_handler;
            MultiByteToWideChar(CP_ACP, 0, buf, -1, info->szDriver, ARRAY_SIZE(info->szDriver));
            TRACE("Returning codec %s, driver %s.\n", debugstr_a(name_buf), debugstr_a(buf));
            return TRUE;
        }
        RegCloseKey(key);
    }

    if (GetPrivateProfileSectionA("drivers32", buf, sizeof(buf), "system.ini"))
    {
        char *s;
        for (s = buf; *s; s += strlen(s) + 1)
        {
            if (s[4] != '.' || s[9] != '=') continue;
            ret_type = mmioStringToFOURCCA(s, 0);
            ret_handler = mmioStringToFOURCCA(s + 5, 0);
            if (type && compare_fourcc(type, ret_type)) continue;
            if (compare_fourcc(handler, ret_handler) && handler != count++) continue;

            info->fccType = ret_type;
            info->fccHandler = ret_handler;
            MultiByteToWideChar(CP_ACP, 0, s + 10, -1, info->szDriver, ARRAY_SIZE(info->szDriver));
            TRACE("Returning codec %s, driver %s.\n", debugstr_an(s, 9), debugstr_a(s + 10));
            return TRUE;
        }
    }

    LIST_FOR_EACH_ENTRY(driver, &reg_driver_list, struct reg_driver, entry)
    {
        if (type && compare_fourcc(type, driver->fccType)) continue;
        if (compare_fourcc(handler, driver->fccHandler) && handler != count++) continue;
        if (driver->proc(0, NULL, ICM_GETINFO, (DWORD_PTR)info, sizeof(*info)) == sizeof(*info))
            return TRUE;
    }

    info->fccType = type;
    info->fccHandler = handler;
    WARN("No driver found for codec %s.%s.\n", debugstr_fourcc(type), debugstr_fourcc(handler));
    return FALSE;
}

static DWORD IC_HandleRef = 1;

/***********************************************************************
 *              ICInstall                       [MSVFW32.@]
 */
BOOL VFWAPI ICInstall(DWORD type, DWORD handler, LPARAM lparam, char *desc, UINT flags)
{
    struct reg_driver *driver;

    TRACE("type %s, handler %s, lparam %#Ix, desc %s, flags %#x.\n",
            debugstr_fourcc(type), debugstr_fourcc(handler), lparam, debugstr_a(desc), flags);

    LIST_FOR_EACH_ENTRY(driver, &reg_driver_list, struct reg_driver, entry)
    {
        if (!compare_fourcc(type, driver->fccType)
                && !compare_fourcc(handler, driver->fccHandler))
        {
            return FALSE;
        }
    }

    switch (flags)
    {
    case ICINSTALL_FUNCTION:
        if (!(driver = calloc(1, sizeof(*driver))))
            return FALSE;
        driver->fccType = type;
        driver->fccHandler = handler;
        driver->proc = (DRIVERPROC)lparam;
        list_add_tail(&reg_driver_list, &driver->entry);
        return TRUE;
    case ICINSTALL_DRIVER:
    {
        const char *driver = (const char *)lparam;
        char value[10];
        HKEY key;
        LONG res;

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, HKLM_DRIVERS32, 0, KEY_SET_VALUE, &key))
            return FALSE;
        fourcc_to_string(value, type);
        value[4] = '.';
        fourcc_to_string(value + 5, handler);
        value[9] = 0;
        res = RegSetValueExA(key, value, 0, REG_SZ, (const BYTE *)driver, strlen(driver) + 1);
        RegCloseKey(key);
        return !res;
    }
    default:
        FIXME("Unhandled flags %#x.\n", flags);
        return FALSE;
    }
}

/***********************************************************************
 *              ICRemove                        [MSVFW32.@]
 */
BOOL VFWAPI ICRemove(DWORD type, DWORD handler, UINT flags)
{
    struct reg_driver *driver;
    char value[10];
    HKEY key;
    LONG res;

    TRACE("type %s, handler %s, flags %#x.\n",
            debugstr_fourcc(type), debugstr_fourcc(handler), flags);

    LIST_FOR_EACH_ENTRY(driver, &reg_driver_list, struct reg_driver, entry)
    {
        if (!compare_fourcc(type, driver->fccType)
                && !compare_fourcc(handler, driver->fccHandler))
        {
            list_remove(&driver->entry);
            free(driver);
            return TRUE;
        }
    }

    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE, HKLM_DRIVERS32, 0, KEY_SET_VALUE, &key))
    {
        fourcc_to_string(value, type);
        value[4] = '.';
        fourcc_to_string(value + 5, handler);
        value[9] = 0;
        res = RegDeleteValueA(key, value);
        RegCloseKey(key);
        return !res;
    }

    return FALSE;
}


/***********************************************************************
 *		ICOpen				[MSVFW32.@]
 * Opens an installable compressor. Return special handle.
 */
HIC VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode) 
{
    WCHAR		codecname[10];
    ICOPEN		icopen;
    WINE_HIC*           whic;
    static const WCHAR  drv32W[] = {'d','r','i','v','e','r','s','3','2','\0'};
    struct reg_driver *driver;
    HDRVR hdrv = NULL;

    TRACE("(%s,%s,0x%08x)\n", debugstr_fourcc(fccType), debugstr_fourcc(fccHandler), wMode);

    if (!fccHandler) /* No specific handler, return the first valid for wMode */
    {
        HIC local;
        ICINFO info;
        DWORD loop = 0;
        info.dwSize = sizeof(info);
        while(ICInfo(fccType, loop++, &info))
        {
            /* Ensure fccHandler is not 0x0 because we will recurse on ICOpen */
            if(!info.fccHandler)
                continue;
            local = ICOpen(fccType, info.fccHandler, wMode);
            if (local != 0)
            {
                TRACE("Returning %s as default handler for %s\n",
                      debugstr_fourcc(info.fccHandler), debugstr_fourcc(fccType));
                return local;
            }
        }
    }

    LIST_FOR_EACH_ENTRY(driver, &reg_driver_list, struct reg_driver, entry)
    {
        if (!compare_fourcc(fccType, driver->fccType)
                && !compare_fourcc(fccHandler, driver->fccHandler))
        {
            return ICOpenFunction(driver->fccType, driver->fccHandler, wMode, driver->proc);
        }
    }

    /* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the
     * same layout as ICOPEN
     */
    icopen.dwSize      = sizeof(ICOPEN);
    icopen.fccType     = fccType;
    icopen.fccHandler  = fccHandler;
    icopen.dwVersion   = 0x00001000; /* FIXME */
    icopen.dwFlags     = wMode;
    icopen.dwError     = 0;
    icopen.pV1Reserved = NULL;
    icopen.pV2Reserved = NULL;
    icopen.dnDevNode   = 0; /* FIXME */

    if (!hdrv)
    {
        /* normalize to lower case as in 'vidc' */
        ((char*)&fccType)[0] = tolower(((char*)&fccType)[0]);
        ((char*)&fccType)[1] = tolower(((char*)&fccType)[1]);
        ((char*)&fccType)[2] = tolower(((char*)&fccType)[2]);
        ((char*)&fccType)[3] = tolower(((char*)&fccType)[3]);
        icopen.fccType = fccType;
        /* Seek the driver in the registry */
        fourcc_to_string(codecname, fccType);
        codecname[4] = '.';
        fourcc_to_string(codecname + 5, fccHandler);
        codecname[9] = '\0';

        hdrv = OpenDriver(codecname, drv32W, (LPARAM)&icopen);
        if (!hdrv)
            return 0;
    }

    if (!(whic = malloc(sizeof(*whic))))
    {
        CloseDriver(hdrv, 0, 0);
        return FALSE;
    }
    whic->hdrv          = hdrv;
    whic->driverproc    = NULL;
    whic->type          = fccType;
    whic->handler       = fccHandler;
    while (MSVIDEO_GetHicPtr((HIC)(ULONG_PTR)IC_HandleRef) != NULL) IC_HandleRef++;
    whic->hic           = (HIC)(ULONG_PTR)IC_HandleRef++;
    whic->next          = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW32.@]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, DRIVERPROC lpfnHandler)
{
    ICOPEN      icopen;
    WINE_HIC*   whic;

    TRACE("(%s,%s,%d,%p)\n",
          debugstr_fourcc(fccType), debugstr_fourcc(fccHandler), wMode, lpfnHandler);

    icopen.dwSize      = sizeof(ICOPEN);
    icopen.fccType     = fccType;
    icopen.fccHandler  = fccHandler;
    icopen.dwVersion   = ICVERSION;
    icopen.dwFlags     = wMode;
    icopen.dwError     = 0;
    icopen.pV1Reserved = NULL;
    icopen.pV2Reserved = NULL;
    icopen.dnDevNode   = 0; /* FIXME */

    if (!(whic = malloc(sizeof(*whic))))
        return NULL;

    whic->driverproc   = lpfnHandler;
    while (MSVIDEO_GetHicPtr((HIC)(ULONG_PTR)IC_HandleRef) != NULL) IC_HandleRef++;
    whic->hic          = (HIC)(ULONG_PTR)IC_HandleRef++;
    whic->next         = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    /* Now try opening/loading the driver. Taken from DRIVER_AddToList */
    /* What if the function is used more than once? */

    if (MSVIDEO_SendMessage(whic, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) 
    {
        WARN("DRV_LOAD failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        free(whic);
        return 0;
    }
    /* return value is not checked */
    MSVIDEO_SendMessage(whic, DRV_ENABLE, 0L, 0L);

    whic->driverId = (DWORD)MSVIDEO_SendMessage(whic, DRV_OPEN, 0, (DWORD_PTR)&icopen);
    /* FIXME: What should we put here? */
    whic->hdrv = NULL;
    
    if (whic->driverId == 0) 
    {
        WARN("DRV_OPEN failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        free(whic);
        return 0;
    }

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		ICGetInfo			[MSVFW32.@]
 */
LRESULT VFWAPI ICGetInfo(HIC hic, ICINFO *picinfo, DWORD cb) 
{
    LRESULT	ret;
    WINE_HIC*   whic = MSVIDEO_GetHicPtr(hic);

    TRACE("(%p,%p,%ld)\n", hic, picinfo, cb);

    if (!whic) return ICERR_BADHANDLE;
    if (!picinfo) return MMSYSERR_INVALPARAM;

    /* (WS) The field szDriver should be initialized because the driver 
     * is not obliged and often will not do it. Some applications, like
     * VirtualDub, rely on this field and will occasionally crash if it
     * goes uninitialized.
     */
    if (cb >= sizeof(ICINFO)) picinfo->szDriver[0] = '\0';

    ret = ICSendMessage(hic, ICM_GETINFO, (DWORD_PTR)picinfo, cb);

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

    return ret;
}

/***********************************************************************
 *              ICLocate                        [MSVFW32.@]
 */
HIC VFWAPI ICLocate(DWORD type, DWORD handler, BITMAPINFOHEADER *in,
        BITMAPINFOHEADER *out, WORD mode)
{
    ICINFO info = {sizeof(info)};
    UINT msg;
    HIC hic;
    DWORD i;

    TRACE("type %s, handler %s, in %p, out %p, mode %u.\n",
            debugstr_fourcc(type), debugstr_fourcc(handler), in, out, mode);

    switch (mode)
    {
    case ICMODE_FASTCOMPRESS:
    case ICMODE_COMPRESS:
        msg = ICM_COMPRESS_QUERY;
        break;
    case ICMODE_FASTDECOMPRESS:
    case ICMODE_DECOMPRESS:
        msg = ICM_DECOMPRESS_QUERY;
        break;
    case ICMODE_DRAW:
        msg = ICM_DRAW_QUERY;
        break;
    default:
        FIXME("Unhandled mode %#x.\n", mode);
        return 0;
    }

    if ((hic = ICOpen(type, handler, mode)))
    {
        if (!ICSendMessage(hic, msg, (DWORD_PTR)in, (DWORD_PTR)out))
        {
            TRACE("Found codec %s.%s.\n", debugstr_fourcc(type),
                    debugstr_fourcc(handler));
            return hic;
        }
        ICClose(hic);
    }

    for (i = 0; ICInfo(type, i, &info); ++i)
    {
        if ((hic = ICOpen(info.fccType, info.fccHandler, mode)))
        {
            if (!ICSendMessage(hic, msg, (DWORD_PTR)in, (DWORD_PTR)out))
            {
                TRACE("Found codec %s.%s.\n", debugstr_fourcc(info.fccType),
                        debugstr_fourcc(info.fccHandler));
                return hic;
            }
            ICClose(hic);
        }
    }

    if (type == streamtypeVIDEO)
        return ICLocate(ICTYPE_VIDEO, handler, in, out, mode);

    WARN("Could not find a driver for codec %s.%s.\n",
            debugstr_fourcc(type), debugstr_fourcc(handler));

    return 0;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVFW32.@]
 */
HIC VFWAPI ICGetDisplayFormat(HIC hic, BITMAPINFOHEADER *in, BITMAPINFOHEADER *out,
                              int depth, int width, int height)
{
    HIC tmphic = hic;

    TRACE("(%p, %p, %p, %d, %d, %d)\n", hic, in, out, depth, width, height);

    if (!tmphic)
    {
        tmphic = ICLocate(ICTYPE_VIDEO, 0, in, NULL, ICMODE_DECOMPRESS);
        if (!tmphic)
            return NULL;
    }

    if (ICDecompressQuery(tmphic, in, NULL))
        goto err;

    if (width <= 0 || height <= 0)
    {
        width = in->biWidth;
        height = in->biHeight;
    }

    if (!depth)
        depth = 32;

    *out = *in;
    out->biSize = sizeof(*out);
    out->biWidth = width;
    out->biHeight = height;
    out->biCompression = BI_RGB;
    out->biSizeImage = get_size_image(width, height, depth);

    /* first try the given depth */
    out->biBitCount = depth;
    out->biSizeImage = get_size_image(width, height, out->biBitCount);
    if (!ICDecompressQuery(tmphic, in, out))
    {
        if (depth == 8)
            ICDecompressGetPalette(tmphic, in, out);
        return tmphic;
    }

    /* then try 16, both with BI_RGB and BI_BITFIELDS */
    if (depth <= 16)
    {
        out->biBitCount = 16;
        out->biSizeImage = get_size_image(width, height, out->biBitCount);
        if (!ICDecompressQuery(tmphic, in, out))
            return tmphic;

        out->biCompression = BI_BITFIELDS;
        if (!ICDecompressQuery(tmphic, in, out))
            return tmphic;
        out->biCompression = BI_RGB;
    }

    /* then try 24 */
    if (depth <= 24)
    {
        out->biBitCount = 24;
        out->biSizeImage = get_size_image(width, height, out->biBitCount);
        if (!ICDecompressQuery(tmphic, in, out))
            return tmphic;
    }

    /* then try 32 */
    if (depth <= 32)
    {
        out->biBitCount = 32;
        out->biSizeImage = get_size_image(width, height, out->biBitCount);
        if (!ICDecompressQuery(tmphic, in, out))
            return tmphic;
    }

    /* as a last resort, try 32 bpp with the original width and height */
    out->biWidth = in->biWidth;
    out->biHeight = in->biHeight;
    out->biBitCount = 32;
    out->biSizeImage = get_size_image(out->biWidth, out->biHeight, out->biBitCount);
    if (!ICDecompressQuery(tmphic, in, out))
        return tmphic;

    /* finally, ask the compressor for its default output format */
    if (!ICSendMessage(tmphic, ICM_DECOMPRESS_GET_FORMAT, (DWORD_PTR)in, (DWORD_PTR)out))
        return tmphic;

err:
    if (hic != tmphic)
        ICClose(tmphic);

    return NULL;
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
	return ICSendMessage(hic,ICM_COMPRESS,(DWORD_PTR)&iccmp,sizeof(iccmp));
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

	icd.dwFlags	= dwFlags;
	icd.lpbiInput	= lpbiFormat;
	icd.lpInput	= lpData;

	icd.lpbiOutput	= lpbi;
	icd.lpOutput	= lpBits;
	icd.ckid	= 0;
	ret = ICSendMessage(hic,ICM_DECOMPRESS,(DWORD_PTR)&icd,sizeof(ICDECOMPRESS));

	return ret;
}


struct choose_compressor
{
    UINT flags;
    LPCSTR title;
    COMPVARS cv;
};

struct codec_info
{
    HIC hic;
    ICINFO icinfo;
};

static BOOL enum_compressors(HWND list, COMPVARS *pcv, BOOL enum_all)
{
    UINT id, total = 0;
    ICINFO icinfo;

    id = 0;

    while (ICInfo(pcv->fccType, id, &icinfo))
    {
        struct codec_info *ic;
        DWORD idx;
        HIC hic;

        id++;

        hic = ICOpen(icinfo.fccType, icinfo.fccHandler, ICMODE_COMPRESS);

        if (hic)
        {
            /* for unknown reason fccHandler reported by the driver
             * doesn't always work, use the one returned by ICInfo instead.
             */
            DWORD fccHandler = icinfo.fccHandler;

            if (!enum_all && pcv->lpbiIn)
            {
                if (ICCompressQuery(hic, pcv->lpbiIn, NULL) != ICERR_OK)
                {
                    TRACE("fccHandler %s doesn't support input DIB format %ld\n",
                          debugstr_fourcc(icinfo.fccHandler), pcv->lpbiIn->bmiHeader.biCompression);
                    ICClose(hic);
                    continue;
                }
            }

            ICGetInfo(hic, &icinfo, sizeof(icinfo));
            icinfo.fccHandler = fccHandler;

            idx = SendMessageW(list, CB_ADDSTRING, 0, (LPARAM)icinfo.szDescription);

            ic = malloc(sizeof(*ic));
            ic->icinfo = icinfo;
            ic->hic = hic;
            SendMessageW(list, CB_SETITEMDATA, idx, (LPARAM)ic);
        }
        total++;
    }

    return total != 0;
}

static INT_PTR CALLBACK icm_choose_compressor_dlgproc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        struct codec_info *ic;
        WCHAR buf[128];
        struct choose_compressor *choose_comp = (struct choose_compressor *)lparam;

        SetWindowLongPtrW(hdlg, DWLP_USER, lparam);

        /* FIXME */
        choose_comp->flags &= ~(ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME);

        if (choose_comp->title)
            SetWindowTextA(hdlg, choose_comp->title);

        if (!(choose_comp->flags & ICMF_CHOOSE_DATARATE))
        {
            ShowWindow(GetDlgItem(hdlg, IDC_DATARATE_CHECKBOX), SW_HIDE);
            ShowWindow(GetDlgItem(hdlg, IDC_DATARATE), SW_HIDE);
            ShowWindow(GetDlgItem(hdlg, IDC_DATARATE_KB), SW_HIDE);
        }

        if (!(choose_comp->flags & ICMF_CHOOSE_KEYFRAME))
        {
            ShowWindow(GetDlgItem(hdlg, IDC_KEYFRAME_CHECKBOX), SW_HIDE);
            ShowWindow(GetDlgItem(hdlg, IDC_KEYFRAME), SW_HIDE);
            ShowWindow(GetDlgItem(hdlg, IDC_KEYFRAME_FRAMES), SW_HIDE);
        }

        /* FIXME */
        EnableWindow(GetDlgItem(hdlg, IDC_QUALITY_SCROLL), FALSE);
        EnableWindow(GetDlgItem(hdlg, IDC_QUALITY_TXT), FALSE);

        /*if (!(choose_comp->flags & ICMF_CHOOSE_PREVIEW))
            ShowWindow(GetDlgItem(hdlg, IDC_PREVIEW), SW_HIDE);*/

        LoadStringW(MSVFW32_hModule, IDS_FULLFRAMES, buf, 128);
        SendDlgItemMessageW(hdlg, IDC_COMP_LIST, CB_ADDSTRING, 0, (LPARAM)buf);

        ic = malloc(sizeof(*ic));
        ic->icinfo.fccType = streamtypeVIDEO;
        ic->icinfo.fccHandler = comptypeDIB;
        ic->hic = 0;
        SendDlgItemMessageW(hdlg, IDC_COMP_LIST, CB_SETITEMDATA, 0, (LPARAM)ic);

        enum_compressors(GetDlgItem(hdlg, IDC_COMP_LIST), &choose_comp->cv, choose_comp->flags & ICMF_CHOOSE_ALLCOMPRESSORS);

        SendDlgItemMessageW(hdlg, IDC_COMP_LIST, CB_SETCURSEL, 0, 0);
        SetFocus(GetDlgItem(hdlg, IDC_COMP_LIST));

        SetWindowLongPtrW(hdlg, DWLP_USER, (ULONG_PTR)choose_comp);
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_COMP_LIST:
        {
            INT cur_sel;
            struct codec_info *ic;
            BOOL can_configure = FALSE, can_about = FALSE;
            struct choose_compressor *choose_comp;

            if (HIWORD(wparam) != CBN_SELCHANGE && HIWORD(wparam) != CBN_SETFOCUS)
                break;

            choose_comp = (struct choose_compressor *)GetWindowLongPtrW(hdlg, DWLP_USER);

            cur_sel = SendMessageW((HWND)lparam, CB_GETCURSEL, 0, 0);

            ic = (struct codec_info *)SendMessageW((HWND)lparam, CB_GETITEMDATA, cur_sel, 0);
            if (ic && ic->hic)
            {
                if (ICQueryConfigure(ic->hic) == DRVCNF_OK)
                    can_configure = TRUE;
                if (ICQueryAbout(ic->hic) == DRVCNF_OK)
                    can_about = TRUE;
            }
            EnableWindow(GetDlgItem(hdlg, IDC_CONFIGURE), can_configure);
            EnableWindow(GetDlgItem(hdlg, IDC_ABOUT), can_about);

            if (choose_comp->flags & ICMF_CHOOSE_DATARATE)
            {
                /* FIXME */
            }
            if (choose_comp->flags & ICMF_CHOOSE_KEYFRAME)
            {
                /* FIXME */
            }

            break;
        }

        case IDC_CONFIGURE:
        case IDC_ABOUT:
        {
            HWND list = GetDlgItem(hdlg, IDC_COMP_LIST);
            INT cur_sel;
            struct codec_info *ic;

            if (HIWORD(wparam) != BN_CLICKED)
                break;

            cur_sel = SendMessageW(list, CB_GETCURSEL, 0, 0);

            ic = (struct codec_info *)SendMessageW(list, CB_GETITEMDATA, cur_sel, 0);
            if (ic && ic->hic)
            {
                if (LOWORD(wparam) == IDC_CONFIGURE)
                    ICConfigure(ic->hic, hdlg);
                else
                    ICAbout(ic->hic, hdlg);
            }

            break;
        }

        case IDOK:
        {
            HWND list = GetDlgItem(hdlg, IDC_COMP_LIST);
            INT cur_sel;
            struct codec_info *ic;

            if (HIWORD(wparam) != BN_CLICKED)
                break;

            cur_sel = SendMessageW(list, CB_GETCURSEL, 0, 0);
            ic = (struct codec_info *)SendMessageW(list, CB_GETITEMDATA, cur_sel, 0);
            if (ic)
            {
                struct choose_compressor *choose_comp = (struct choose_compressor *)GetWindowLongPtrW(hdlg, DWLP_USER);

                choose_comp->cv.hic = ic->hic;
                choose_comp->cv.fccType = ic->icinfo.fccType;
                choose_comp->cv.fccHandler = ic->icinfo.fccHandler;
                /* FIXME: fill everything else */

                /* prevent closing the codec handle below */
                ic->hic = 0;
            }
        }
        /* fall through */
        case IDCANCEL:
        {
            HWND list = GetDlgItem(hdlg, IDC_COMP_LIST);
            INT idx = 0;

            if (HIWORD(wparam) != BN_CLICKED)
                break;

            while (1)
            {
                struct codec_info *ic;
    
                ic = (struct codec_info *)SendMessageW(list, CB_GETITEMDATA, idx++, 0);

                if (!ic || (LONG_PTR)ic == CB_ERR) break;

                if (ic->hic) ICClose(ic->hic);
                free(ic);
            }

            EndDialog(hdlg, LOWORD(wparam) == IDOK);
            break;
        }

        default:
            break;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

/***********************************************************************
 *		ICCompressorChoose   [MSVFW32.@]
 */
BOOL VFWAPI ICCompressorChoose(HWND hwnd, UINT uiFlags, LPVOID pvIn,
                               LPVOID lpData, PCOMPVARS pc, LPSTR lpszTitle)
{
    struct choose_compressor choose_comp;
    BOOL ret;

    TRACE("(%p,%08x,%p,%p,%p,%s)\n", hwnd, uiFlags, pvIn, lpData, pc, lpszTitle);

    if (!pc || pc->cbSize != sizeof(COMPVARS))
        return FALSE;

    if (!(pc->dwFlags & ICMF_COMPVARS_VALID))
    {
        pc->dwFlags   = 0;
        pc->fccType   = pc->fccHandler = 0;
        pc->hic       = NULL;
        pc->lpbiIn    = NULL;
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

    choose_comp.cv = *pc;
    choose_comp.flags = uiFlags;
    choose_comp.title = lpszTitle;

    ret = DialogBoxParamW(MSVFW32_hModule, MAKEINTRESOURCEW(ICM_CHOOSE_COMPRESSOR), hwnd,
                          icm_choose_compressor_dlgproc, (LPARAM)&choose_comp);

    if (ret)
    {
        *pc = choose_comp.cv;
        pc->dwFlags |= ICMF_COMPVARS_VALID;
    }

    return ret;
}


/***********************************************************************
 *		ICCompressorFree   [MSVFW32.@]
 */
void VFWAPI ICCompressorFree(PCOMPVARS pc)
{
    TRACE("(%p)\n", pc);

    if (pc && pc->cbSize == sizeof(COMPVARS))
    {
        if (pc->hic)
        {
            ICClose(pc->hic);
            pc->hic = NULL;
        }
        free(pc->lpbiIn);
        pc->lpbiIn = NULL;
        free(pc->lpBitsOut);
        pc->lpBitsOut = NULL;
        free(pc->lpBitsPrev);
        pc->lpBitsPrev = NULL;
        free(pc->lpState);
        pc->lpState = NULL;
        pc->dwFlags = 0;
    }
}

/***********************************************************************
 *		ICSendMessage			[MSVFW32.@]
 */
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD_PTR lParam1, DWORD_PTR lParam2) 
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
	return ICSendMessage(hic,ICM_DRAW_BEGIN,(DWORD_PTR)&icdb,sizeof(icdb));
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

	return ICSendMessage(hic,ICM_DRAW,(DWORD_PTR)&icd,sizeof(icd));
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

    free(whic);
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
	ULONG	cbHdr = 0;
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
		if (!(pHdr = calloc(1, cbHdr + sizeof(RGBQUAD) * 256)))
			goto err;
		if ( ICDecompressGetFormat( hic, lpbiIn, pHdr ) != ICERR_OK )
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
		biSizeImage = get_size_image(lpbiOut->bmiHeader.biWidth, lpbiOut->bmiHeader.biHeight, lpbiOut->bmiHeader.biBitCount);

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
	pMem = GlobalLock( hMem );
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
        free(pHdr);
	if ( pMem != NULL )
		GlobalUnlock( hMem );
	if ( !bSucceeded && hMem != NULL )
	{
		GlobalFree(hMem); hMem = NULL;
	}

	return hMem;
}

/***********************************************************************
 *      ICSeqCompressFrame   [MSVFW32.@]
 */
LPVOID VFWAPI ICSeqCompressFrame(PCOMPVARS pc, UINT uiFlags, LPVOID lpBits, BOOL *pfKey, LONG *plSize)
{
    ICCOMPRESS* icComp = pc->lpState;
    DWORD ret;
    TRACE("(%p, 0x%08x, %p, %p, %p)\n", pc, uiFlags, lpBits, pfKey, plSize);

    if (pc->cbState != sizeof(ICCOMPRESS))
    {
       ERR("Invalid cbState %li\n", pc->cbState);
       return NULL;
    }

    if (!pc->lKeyCount++)
       icComp->dwFlags = ICCOMPRESS_KEYFRAME;
    else
    {
        if (pc->lKey && pc->lKeyCount == (pc->lKey - 1))
        /* No key frames if pc->lKey == 0 */
           pc->lKeyCount = 0;
        icComp->dwFlags = 0;
    }

    icComp->lpInput = lpBits;
    icComp->lFrameNum = pc->lFrame++;
    icComp->lpOutput = pc->lpBitsOut;
    icComp->lpPrev = pc->lpBitsPrev;
    ret = ICSendMessage(pc->hic, ICM_COMPRESS, (DWORD_PTR)icComp, sizeof(*icComp));

    if (ret == ICERR_OK)
    {
        LPVOID oldprev, oldout;

        if (icComp->dwFlags & AVIIF_KEYFRAME)
        {
            pc->lKeyCount = 1;
            *pfKey = TRUE;
            TRACE("Key frame\n");
        }
        else
            *pfKey = FALSE;

        *plSize = icComp->lpbiOutput->biSizeImage;

        /* We shift Prev and Out, so we don't have to allocate and release memory */
        oldprev = pc->lpBitsPrev;
        oldout = pc->lpBitsOut;
        pc->lpBitsPrev = oldout;
        pc->lpBitsOut = oldprev;

        TRACE("returning: %p, compressed frame size %lu\n", icComp->lpOutput, *plSize);
        return icComp->lpOutput;
    }
    return NULL;
}

static void clear_compvars(PCOMPVARS pc)
{
    free(pc->lpbiIn);
    free(pc->lpBitsPrev);
    free(pc->lpBitsOut);
    free(pc->lpState);
    pc->lpbiIn = pc->lpBitsPrev = pc->lpBitsOut = pc->lpState = NULL;
    if (pc->dwFlags & 0x80000000)
    {
        free(pc->lpbiOut);
        pc->lpbiOut = NULL;
        pc->dwFlags &= ~0x80000000;
    }
}

/***********************************************************************
 *      ICSeqCompressFrameEnd   [MSVFW32.@]
 */
void VFWAPI ICSeqCompressFrameEnd(PCOMPVARS pc)
{
    TRACE("(%p)\n", pc);
    ICSendMessage(pc->hic, ICM_COMPRESS_END, 0, 0);
    clear_compvars(pc);
}

static BITMAPINFO *copy_bitmapinfo(const BITMAPINFO *src)
{
    int num_colors;
    unsigned int size;
    BITMAPINFO *dst;

    if (src->bmiHeader.biClrUsed)
        num_colors = min(src->bmiHeader.biClrUsed, 256);
    else
        num_colors = src->bmiHeader.biBitCount > 8 ? 0 : 1 << src->bmiHeader.biBitCount;

    size = FIELD_OFFSET(BITMAPINFO, bmiColors[num_colors]);
    if (src->bmiHeader.biCompression == BI_BITFIELDS)
        size += 3 * sizeof(DWORD);

    if (!(dst = malloc(size)))
        return NULL;

    memcpy(dst, src, size);
    return dst;
}

/***********************************************************************
 *      ICSeqCompressFrameStart [MSVFW32.@]
 */
BOOL VFWAPI ICSeqCompressFrameStart(PCOMPVARS pc, LPBITMAPINFO lpbiIn)
{
    /* I'm ignoring bmiColors as I don't know what to do with it,
     * it doesn't appear to be used though
     */
    DWORD ret;
    ICCOMPRESS* icComp;

    if (!(pc->lpbiIn = copy_bitmapinfo(lpbiIn)))
        return FALSE;

    if (!(pc->lpState = malloc(sizeof(ICCOMPRESS) + sizeof(*icComp->lpckid) + sizeof(*icComp->lpdwFlags))))
        goto error;

    pc->cbState = sizeof(ICCOMPRESS);

    if (!pc->lpbiOut)
    {
        /* Ask compressor for needed header size */
        int size = ICSendMessage(pc->hic, ICM_COMPRESS_GET_FORMAT,
                                 (DWORD_PTR)pc->lpbiIn, 0);
        if (size <= 0)
            goto error;

        if (!(pc->lpbiOut = calloc(1, size)))
            goto error;
        /* Flag to show that we allocated lpbiOut for proper cleanup */
        pc->dwFlags |= 0x80000000;

        ret = ICSendMessage(pc->hic, ICM_COMPRESS_GET_FORMAT,
                            (DWORD_PTR)pc->lpbiIn, (DWORD_PTR)pc->lpbiOut);
        if (ret != ICERR_OK)
        {
            ERR("Could not get output format from compressor\n");
            goto error;
        }
        if (!pc->lpbiOut->bmiHeader.biSizeImage)
        {
            /* If we can't know the output frame size for sure at least allocate
             * the same size of the input frame and also at least 8Kb to be sure
             * that poor compressors will have enough memory to work if the input
             * frame is too small.
             */
            pc->lpbiOut->bmiHeader.biSizeImage = max(8192, pc->lpbiIn->bmiHeader.biSizeImage);
            ERR("Bad codec! Invalid output frame size, guessing from input\n");
        }
    }

    TRACE("Input: %lux%lu, fcc %s, bpp %u, size %lu\n",
          pc->lpbiIn->bmiHeader.biWidth, pc->lpbiIn->bmiHeader.biHeight,
          debugstr_fourcc(pc->lpbiIn->bmiHeader.biCompression),
          pc->lpbiIn->bmiHeader.biBitCount,
          pc->lpbiIn->bmiHeader.biSizeImage);
    TRACE("Output: %lux%lu, fcc %s, bpp %u, size %lu\n",
          pc->lpbiOut->bmiHeader.biWidth, pc->lpbiOut->bmiHeader.biHeight,
          debugstr_fourcc(pc->lpbiOut->bmiHeader.biCompression),
          pc->lpbiOut->bmiHeader.biBitCount,
          pc->lpbiOut->bmiHeader.biSizeImage);

    /* Buffer for compressed frame data */
    if (!(pc->lpBitsOut = malloc(pc->lpbiOut->bmiHeader.biSizeImage)))
        goto error;

    /* Buffer for previous compressed frame data */
    if (!(pc->lpBitsPrev = malloc(pc->lpbiOut->bmiHeader.biSizeImage)))
        goto error;

    TRACE("Compvars:\n"
          "\tsize: %lu\n"
          "\tflags: 0x%lx\n"
          "\thic: %p\n"
          "\ttype: %s\n"
          "\thandler: %s\n"
          "\tin/out: %p/%p\n"
          "\tkey/data/quality: %li/%li/%li\n",
    pc->cbSize, pc->dwFlags, pc->hic, debugstr_fourcc(pc->fccType),
    debugstr_fourcc(pc->fccHandler), pc->lpbiIn, pc->lpbiOut, pc->lKey,
    pc->lDataRate, pc->lQ);

    ret = ICSendMessage(pc->hic, ICM_COMPRESS_BEGIN, (DWORD_PTR)pc->lpbiIn, (DWORD_PTR)pc->lpbiOut);
    if (ret == ICERR_OK)
    {
        icComp = pc->lpState;
        /* Initialise some variables */
        pc->lFrame = 0; pc->lKeyCount = 0;

        icComp->lpbiOutput = &pc->lpbiOut->bmiHeader;
        icComp->lpbiInput = &pc->lpbiIn->bmiHeader;
        icComp->lpckid = (DWORD *)(icComp + 1);
        *icComp->lpckid = 0;
        icComp->lpdwFlags = (DWORD *)((char *)(icComp + 1) + sizeof(*icComp->lpckid));
        *icComp->lpdwFlags = 0;
        icComp->dwFrameSize = 0;
        icComp->dwQuality = pc->lQ;
        icComp->lpbiPrev = &pc->lpbiIn->bmiHeader;
        return TRUE;
    }
error:
    clear_compvars(pc);
    return FALSE;
}

/***********************************************************************
 *      GetFileNamePreview   [MSVFW32.@]
 */
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
  {
    FreeLibrary(hComdlg32);
    return FALSE;
  }

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
