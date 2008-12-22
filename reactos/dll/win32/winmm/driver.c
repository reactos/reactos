/*
 * WINE Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1998 Marcus Meissner
 * Copyright 1999 Eric Pouech
 *
 * Reformatting and additional comments added by Andrew Greenwood.
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
 */

#include <string.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "mmddk.h"
#include "winemm.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

static LPWINE_DRIVER   lpDrvItemList  /* = NULL */;
static const WCHAR HKLM_BASE[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                  'W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};

WINE_MMTHREAD*  (*pFnGetMMThread16)(UINT16 h) /* = NULL */;
LPWINE_DRIVER   (*pFnOpenDriver16)(LPCWSTR,LPCWSTR,LPARAM) /* = NULL */;
LRESULT         (*pFnCloseDriver16)(UINT16,LPARAM,LPARAM) /* = NULL */;
LRESULT         (*pFnSendMessage16)(UINT16,UINT,LPARAM,LPARAM) /* = NULL */;

/**************************************************************************
 *			DRIVER_GetNumberOfModuleRefs		[internal]
 *
 * Returns the number of open drivers which share the same module.
 */
static	unsigned DRIVER_GetNumberOfModuleRefs(HMODULE hModule, WINE_DRIVER** found)
{
    LPWINE_DRIVER	lpDrv;
    unsigned		count = 0;

    if (found) *found = NULL;
    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem)
    {
	if (!(lpDrv->dwFlags & WINE_GDF_16BIT) && lpDrv->d.d32.hModule == hModule)
        {
            if (found && !*found) *found = lpDrv;
	    count++;
	}
    }
    return count;
}

/**************************************************************************
 *				DRIVER_FindFromHDrvr		[internal]
 *
 * From a hDrvr being 32 bits, returns the WINE internal structure.
 */
LPWINE_DRIVER	DRIVER_FindFromHDrvr(HDRVR hDrvr)
{
    LPWINE_DRIVER	d = (LPWINE_DRIVER)hDrvr;

    if (hDrvr && HeapValidate(GetProcessHeap(), 0, d) && d->dwMagic == WINE_DI_MAGIC) {
	return d;
    }
    return NULL;
}

/**************************************************************************
 *				DRIVER_SendMessage		[internal]
 */
static LRESULT inline DRIVER_SendMessage(LPWINE_DRIVER lpDrv, UINT msg,
                                         LPARAM lParam1, LPARAM lParam2)
{
    LRESULT		ret = 0;

    if (lpDrv->dwFlags & WINE_GDF_16BIT) {
        /* no need to check mmsystem presence: the driver must have been opened as a 16 bit one,
         */
        if (pFnSendMessage16)
            ret = pFnSendMessage16(lpDrv->d.d16.hDriver16, msg, lParam1, lParam2);
    } else {
        TRACE("Before call32 proc=%p drvrID=%08lx hDrv=%p wMsg=%04x p1=%08lx p2=%08lx\n",
              lpDrv->d.d32.lpDrvProc, lpDrv->d.d32.dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);
        ret = lpDrv->d.d32.lpDrvProc(lpDrv->d.d32.dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);
        TRACE("After  call32 proc=%p drvrID=%08lx hDrv=%p wMsg=%04x p1=%08lx p2=%08lx => %08lx\n",
              lpDrv->d.d32.lpDrvProc, lpDrv->d.d32.dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2, ret);
    }
    return ret;
}

/**************************************************************************
 *				SendDriverMessage		[WINMM.@]
 *				DrvSendMessage			[WINMM.@]
 */
LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT msg, LONG lParam1,
				 LONG lParam2)
{
    LPWINE_DRIVER	lpDrv;
    LRESULT 		retval = 0;

    TRACE("(%p, %04X, %08lX, %08lX)\n", hDriver, msg, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDriver)) != NULL) {
	retval = DRIVER_SendMessage(lpDrv, msg, lParam1, lParam2);
    } else {
	WARN("Bad driver handle %p\n", hDriver);
    }
    TRACE("retval = %ld\n", retval);

    return retval;
}

/**************************************************************************
 *				DRIVER_RemoveFromList		[internal]
 *
 * Generates all the logic to handle driver closure / deletion
 * Removes a driver struct to the list of open drivers.
 */
static	BOOL	DRIVER_RemoveFromList(LPWINE_DRIVER lpDrv)
{
    if (!(lpDrv->dwFlags & WINE_GDF_16BIT)) {
        /* last of this driver in list ? */
	if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, NULL) == 1) {
	    DRIVER_SendMessage(lpDrv, DRV_DISABLE, 0L, 0L);
	    DRIVER_SendMessage(lpDrv, DRV_FREE,    0L, 0L);
	}
    }

    if (lpDrv->lpPrevItem)
	lpDrv->lpPrevItem->lpNextItem = lpDrv->lpNextItem;
    else
	lpDrvItemList = lpDrv->lpNextItem;
    if (lpDrv->lpNextItem)
	lpDrv->lpNextItem->lpPrevItem = lpDrv->lpPrevItem;
    /* trash magic number */
    lpDrv->dwMagic ^= 0xa5a5a5a5;

    return TRUE;
}

/**************************************************************************
 *				DRIVER_AddToList		[internal]
 *
 * Adds a driver struct to the list of open drivers.
 * Generates all the logic to handle driver creation / open.
 */
static	BOOL	DRIVER_AddToList(LPWINE_DRIVER lpNewDrv, LPARAM lParam1, LPARAM lParam2)
{
    lpNewDrv->dwMagic = WINE_DI_MAGIC;
    /* First driver to be loaded for this module, need to load correctly the module */
    if (!(lpNewDrv->dwFlags & WINE_GDF_16BIT)) {
        /* first of this driver in list ? */
	if (DRIVER_GetNumberOfModuleRefs(lpNewDrv->d.d32.hModule, NULL) == 0) {
	    if (DRIVER_SendMessage(lpNewDrv, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) {
		TRACE("DRV_LOAD failed on driver %p\n", lpNewDrv);
		return FALSE;
	    }
	    /* returned value is not checked */
	    DRIVER_SendMessage(lpNewDrv, DRV_ENABLE, 0L, 0L);
	}
    }

    lpNewDrv->lpNextItem = NULL;
    if (lpDrvItemList == NULL) {
	lpDrvItemList = lpNewDrv;
	lpNewDrv->lpPrevItem = NULL;
    } else {
	LPWINE_DRIVER	lpDrv = lpDrvItemList;	/* find end of list */
	while (lpDrv->lpNextItem != NULL)
	    lpDrv = lpDrv->lpNextItem;

	lpDrv->lpNextItem = lpNewDrv;
	lpNewDrv->lpPrevItem = lpDrv;
    }

    if (!(lpNewDrv->dwFlags & WINE_GDF_16BIT)) {
	/* Now just open a new instance of a driver on this module */
	lpNewDrv->d.d32.dwDriverID = DRIVER_SendMessage(lpNewDrv, DRV_OPEN, lParam1, lParam2);

	if (lpNewDrv->d.d32.dwDriverID == 0) {
	    TRACE("DRV_OPEN failed on driver %p\n", lpNewDrv);
	    DRIVER_RemoveFromList(lpNewDrv);
	    return FALSE;
	}
    }
    return TRUE;
}

/**************************************************************************
 *				DRIVER_GetLibName		[internal]
 *
 */
BOOL	DRIVER_GetLibName(LPCWSTR keyName, LPCWSTR sectName, LPWSTR buf, int sz)
{
    HKEY	hKey, hSecKey;
    DWORD	bufLen, lRet;
    static const WCHAR wszSystemIni[] = {'S','Y','S','T','E','M','.','I','N','I',0};
    WCHAR       wsznull = '\0';

    /* This takes us as far as Windows NT\CurrentVersion */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, HKLM_BASE, 0, KEY_QUERY_VALUE, &hKey);

    if (lRet == ERROR_SUCCESS)
    {
        /* Now we descend into the section name that we were given */
	    lRet = RegOpenKeyExW(hKey, sectName, 0, KEY_QUERY_VALUE, &hSecKey);

	    if (lRet == ERROR_SUCCESS)
        {
            /* Retrieve the desired value - this is the filename of the lib */
            bufLen = sz;
	        lRet = RegQueryValueExW(hSecKey, keyName, 0, 0, (void*)buf, &bufLen);
	        RegCloseKey( hSecKey );
	    }

        RegCloseKey( hKey );
    }

    /* Finish up if we've got what we want from the registry */
    if (lRet == ERROR_SUCCESS)
        return TRUE;

    /* default to system.ini if we can't find it in the registry,
     * to support native installations where system.ini is still used */
    return GetPrivateProfileStringW(sectName, keyName, &wsznull, buf, sz / sizeof(WCHAR), wszSystemIni);
}

/**************************************************************************
 *				DRIVER_TryOpenDriver32		[internal]
 *
 * Tries to load a 32 bit driver whose DLL's (module) name is fn
 */
LPWINE_DRIVER	DRIVER_TryOpenDriver32(LPCWSTR fn, LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    HMODULE		hModule = 0;
    LPWSTR		ptr;
    LPCSTR		cause = 0;

    TRACE("(%s, %08lX);\n", debugstr_w(fn), lParam2);

    if ((ptr = strchrW(fn, ' ')) != NULL)
    {
        *ptr++ = '\0';

        while (*ptr == ' ')
            ptr++;

        if (*ptr == '\0')
            ptr = NULL;
    }

    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));

    if (lpDrv == NULL)
    {
        cause = "OOM";
        goto exit;
    }

    if ((hModule = LoadLibraryW(fn)) == 0)
    {
        cause = "Not a 32 bit lib";
        goto exit;
    }

    lpDrv->d.d32.lpDrvProc = (DRIVERPROC)GetProcAddress(hModule, "DriverProc");

    if (lpDrv->d.d32.lpDrvProc == NULL)
    {
        cause = "no DriverProc";
        goto exit;
    }

    lpDrv->dwFlags          = 0;
    lpDrv->d.d32.hModule    = hModule;
    lpDrv->d.d32.dwDriverID = 0;

    /* Win32 installable drivers must support a two phase opening scheme:
     * + first open with NULL as lParam2 (session instance),
     * + then do a second open with the real non null lParam2)
     */
    if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, NULL) == 0 && lParam2)
    {
        LPWINE_DRIVER   ret;

        if (!DRIVER_AddToList(lpDrv, (LPARAM)ptr, 0L))
        {
            cause = "load0 failed";
            goto exit;
        }
        ret = DRIVER_TryOpenDriver32(fn, lParam2);
        if (!ret)
        {
            CloseDriver((HDRVR)lpDrv, 0L, 0L);
            cause = "load1 failed";
            goto exit;
        }
        return ret;
    }

    if (!DRIVER_AddToList(lpDrv, (LPARAM)ptr, lParam2))
    {
        cause = "load failed";
        goto exit;
    }

    TRACE("=> %p\n", lpDrv);
    return lpDrv;

 exit:
    FreeLibrary(hModule);
    HeapFree(GetProcessHeap(), 0, lpDrv);
    TRACE("Unable to load 32 bit module %s: %s\n", debugstr_w(fn), cause);
    return NULL;
}

/**************************************************************************
 *				OpenDriverA		        [WINMM.@]
 *				DrvOpenA			[WINMM.@]
 * (0,1,DRV_LOAD  ,0       ,0)
 * (0,1,DRV_ENABLE,0       ,0)
 * (0,1,DRV_OPEN  ,buf[256],0)
 */
HDRVR WINAPI OpenDriverA(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam)
{
    INT                 len;
    LPWSTR 		dn = NULL;
    LPWSTR 		sn = NULL;
    HDRVR		ret = 0;

    if (lpDriverName)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpDriverName, -1, NULL, 0 );
        dn = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (!dn) goto done;
        MultiByteToWideChar( CP_ACP, 0, lpDriverName, -1, dn, len );
    }

    if (lpSectionName)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpSectionName, -1, NULL, 0 );
        sn = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (!sn) goto done;
        MultiByteToWideChar( CP_ACP, 0, lpSectionName, -1, sn, len );
    }

    ret = OpenDriver(dn, sn, lParam);

done:
    HeapFree(GetProcessHeap(), 0, dn);
    HeapFree(GetProcessHeap(), 0, sn);
    return ret;
}

/**************************************************************************
 *				OpenDriver 		        [WINMM.@]
 *				DrvOpen				[WINMM.@]
 */
HDRVR WINAPI OpenDriver(LPCWSTR lpDriverName, LPCWSTR lpSectionName, LONG lParam)
{
    LPWINE_DRIVER	lpDrv = NULL;
    WCHAR 		libName[128];
    LPCWSTR		lsn = lpSectionName;

    TRACE("(%s, %s, 0x%08lx);\n",
          debugstr_w(lpDriverName), debugstr_w(lpSectionName), lParam);

    /* If no section name is specified, either the caller is intending on
       opening a driver by filename, or wants to open a user-installable
       driver that has an entry in the Drivers32 key in the registry */
    if (lsn == NULL)
    {
        /* Default registry key */
        static const WCHAR wszDrivers32[] = {'D','r','i','v','e','r','s','3','2',0};

        lstrcpynW(libName, lpDriverName, sizeof(libName) / sizeof(WCHAR));

        /* Try and open the driver by filename */
        if ( (lpDrv = DRIVER_TryOpenDriver32(libName, lParam)) )
            goto the_end;

        /* If we got here, the file wasn't found. So we assume the caller
           wanted a driver specified under the Drivers32 registry key */
        lsn = wszDrivers32;
    }

    /* Attempt to locate the driver filename in the registry */
    if ( DRIVER_GetLibName(lpDriverName, lsn, libName, sizeof(libName)) )
    {
        /* Now we have the filename, we can try and load it */
        if ( (lpDrv = DRIVER_TryOpenDriver32(libName, lParam)) )
            goto the_end;
    }

    /* now we will try a 16 bit driver (and add all the glue to make it work... which
     * is located in our mmsystem implementation)
     * so ensure, we can load our mmsystem, otherwise just fail
     */
    WINMM_CheckForMMSystem();
    if (pFnOpenDriver16 &&
        (lpDrv = pFnOpenDriver16(lpDriverName, lpSectionName, lParam)))
    {
        if (DRIVER_AddToList(lpDrv, 0, lParam)) goto the_end;
        HeapFree(GetProcessHeap(), 0, lpDrv);
    }
    TRACE("Failed to open driver %s from system.ini file, section %s\n",
          debugstr_w(lpDriverName), debugstr_w(lpSectionName));
    return 0;

 the_end:
    if (lpDrv)	TRACE("=> %p\n", lpDrv);
    return (HDRVR)lpDrv;
}

/**************************************************************************
 *			CloseDriver				[WINMM.@]
 *			DrvClose				[WINMM.@]
 */
LRESULT WINAPI CloseDriver(HDRVR hDrvr, LONG lParam1, LONG lParam2)
{
    LPWINE_DRIVER	lpDrv;

    TRACE("(%p, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL)
    {
	if (lpDrv->dwFlags & WINE_GDF_16BIT)
        {
            if (pFnCloseDriver16)
                pFnCloseDriver16(lpDrv->d.d16.hDriver16, lParam1, lParam2);
        }
	else
        {
	    DRIVER_SendMessage(lpDrv, DRV_CLOSE, lParam1, lParam2);
            lpDrv->d.d32.dwDriverID = 0;
        }
	if (DRIVER_RemoveFromList(lpDrv)) {
            if (!(lpDrv->dwFlags & WINE_GDF_16BIT))
            {
                LPWINE_DRIVER       lpDrv0;

                /* if driver has an opened session instance, we have to close it too */
                if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, &lpDrv0) == 1)
                {
                    DRIVER_SendMessage(lpDrv0, DRV_CLOSE, 0L, 0L);
                    lpDrv0->d.d32.dwDriverID = 0;
                    DRIVER_RemoveFromList(lpDrv0);
                    FreeLibrary(lpDrv0->d.d32.hModule);
                    HeapFree(GetProcessHeap(), 0, lpDrv0);
                }
                FreeLibrary(lpDrv->d.d32.hModule);
            }
            HeapFree(GetProcessHeap(), 0, lpDrv);
            return TRUE;
        }
    }
    WARN("Failed to close driver\n");
    return FALSE;
}

/**************************************************************************
 *				GetDriverFlags		[WINMM.@]
 * [in] hDrvr handle to the driver
 *
 * Returns:
 *	0x00000000 if hDrvr is an invalid handle
 *	0x80000000 if hDrvr is a valid 32 bit driver
 *	0x90000000 if hDrvr is a valid 16 bit driver
 *
 * native WINMM doesn't return those flags
 *	0x80000000 for a valid 32 bit driver and that's it
 *	(I may have mixed up the two flags :-(
 */
DWORD	WINAPI GetDriverFlags(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    DWORD		ret = 0;

    TRACE("(%p)\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL) {
	ret = WINE_GDF_EXIST | lpDrv->dwFlags;
    }
    return ret;
}

/**************************************************************************
 *				GetDriverModuleHandle	[WINMM.@]
 *				DrvGetModuleHandle	[WINMM.@]
 */
HMODULE WINAPI GetDriverModuleHandle(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    HMODULE		hModule = 0;

    TRACE("(%p);\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL) {
	if (!(lpDrv->dwFlags & WINE_GDF_16BIT))
	    hModule = lpDrv->d.d32.hModule;
    }
    TRACE("=> %p\n", hModule);
    return hModule;
}

/**************************************************************************
 * 				DefDriverProc			  [WINMM.@]
 * 				DrvDefDriverProc		  [WINMM.@]
 */
LONG WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hDrv,
			     UINT Msg, LONG lParam1, LONG lParam2)
{
    switch (Msg) {
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_ENABLE:
    case DRV_DISABLE:
        return 1;
    case DRV_INSTALL:
    case DRV_REMOVE:
        return DRV_SUCCESS;
    default:
        return 0;
    }
}

/**************************************************************************
 * 				DriverCallback			[WINMM.@]
 */
BOOL WINAPI DriverCallback(DWORD dwCallBack, UINT uFlags, HDRVR hDev,
			   UINT wMsg, DWORD dwUser, DWORD dwParam1,
			   DWORD dwParam2)
{
    TRACE("(%08lX, %04X, %p, %04X, %08lX, %08lX, %08lX); !\n",
	  dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);

    switch (uFlags & DCB_TYPEMASK) {
    case DCB_NULL:
	TRACE("Null !\n");
	if (dwCallBack)
	    WARN("uFlags=%04X has null DCB value, but dwCallBack=%08lX is not null !\n", uFlags, dwCallBack);
	break;
    case DCB_WINDOW:
	TRACE("Window(%04lX) handle=%p!\n", dwCallBack, hDev);
	PostMessageA((HWND)dwCallBack, wMsg, (WPARAM)hDev, dwParam1);
	break;
    case DCB_TASK: /* aka DCB_THREAD */
	TRACE("Task(%04lx) !\n", dwCallBack);
	PostThreadMessageA(dwCallBack, wMsg, (WPARAM)hDev, dwParam1);
	break;
    case DCB_FUNCTION:
	TRACE("Function (32 bit) !\n");
	((LPDRVCALLBACK)dwCallBack)(hDev, wMsg, dwUser, dwParam1, dwParam2);
	break;
    case DCB_EVENT:
	TRACE("Event(%08lx) !\n", dwCallBack);
	SetEvent((HANDLE)dwCallBack);
	break;
    case 6: /* I would dub it DCB_MMTHREADSIGNAL */
	/* this is an undocumented DCB_ value used for mmThreads
	 * loword of dwCallBack contains the handle of the lpMMThd block
	 * which dwSignalCount has to be incremented
	 */
        if (pFnGetMMThread16)
	{
	    WINE_MMTHREAD*	lpMMThd = pFnGetMMThread16(LOWORD(dwCallBack));

	    TRACE("mmThread (%04x, %p) !\n", LOWORD(dwCallBack), lpMMThd);
	    /* same as mmThreadSignal16 */
	    InterlockedIncrement(&lpMMThd->dwSignalCount);
	    SetEvent(lpMMThd->hEvent);
	    /* some other stuff on lpMMThd->hVxD */
	}
	break;
#if 0
    case 4:
	/* this is an undocumented DCB_ value for... I don't know */
	break;
#endif
    default:
	WARN("Unknown callback type %d\n", uFlags & DCB_TYPEMASK);
	return FALSE;
    }
    TRACE("Done\n");
    return TRUE;
}

/******************************************************************
 *		DRIVER_UnloadAll
 *
 *
 */
void    DRIVER_UnloadAll(void)
{
    LPWINE_DRIVER 	lpDrv;
    LPWINE_DRIVER 	lpNextDrv = NULL;
    unsigned            count = 0;

    for (lpDrv = lpDrvItemList; lpDrv != NULL; lpDrv = lpNextDrv)
    {
        lpNextDrv = lpDrv->lpNextItem;
        CloseDriver((HDRVR)lpDrv, 0, 0);
        count++;
    }
    TRACE("Unloaded %u drivers\n", count);
}
