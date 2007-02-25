/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

/**********************************************************************/

HANDLE MSACM_hHeap = NULL;
PWINE_ACMDRIVERID MSACM_pFirstACMDriverID = NULL;
PWINE_ACMDRIVERID MSACM_pLastACMDriverID = NULL;

#if 0
/***********************************************************************
 *           MSACM_DumpCache
 */
static	void MSACM_DumpCache(PWINE_ACMDRIVERID padid)
{
    unsigned 	i;

    TRACE("cFilterTags=%lu cFormatTags=%lu fdwSupport=%08lx\n",
	  padid->cFilterTags, padid->cFormatTags, padid->fdwSupport);
    for (i = 0; i < padid->cache->cFormatTags; i++) {
	TRACE("\tdwFormatTag=%lu cbwfx=%lu\n",
	      padid->aFormatTag[i].dwFormatTag, padid->aFormatTag[i].cbwfx);
    }
}
#endif

/***********************************************************************
 *           MSACM_FindFormatTagInCache 		[internal]
 *
 *	Returns TRUE is the format tag fmtTag is present in the cache.
 *	If so, idx is set to its index.
 */
BOOL MSACM_FindFormatTagInCache(WINE_ACMDRIVERID* padid, DWORD fmtTag, LPDWORD idx)
{
    unsigned 	i;

    for (i = 0; i < padid->cFormatTags; i++) {
	if (padid->aFormatTag[i].dwFormatTag == fmtTag) {
	    if (idx) *idx = i;
	    return TRUE;
	}
    }
    return FALSE;
}

/***********************************************************************
 *           MSACM_FillCache
 */
static BOOL MSACM_FillCache(PWINE_ACMDRIVERID padid)
{
    HACMDRIVER		        had = 0;
    int			        ntag;
    ACMDRIVERDETAILSW	        add;
    ACMFORMATTAGDETAILSW        aftd;

    if (acmDriverOpen(&had, (HACMDRIVERID)padid, 0) != 0)
	return FALSE;

    padid->aFormatTag = NULL;
    add.cbStruct = sizeof(add);
    if (MSACM_Message(had, ACMDM_DRIVER_DETAILS, (LPARAM)&add,  0))
	goto errCleanUp;

    if (add.cFormatTags > 0) {
	padid->aFormatTag = HeapAlloc(MSACM_hHeap, HEAP_ZERO_MEMORY,
				      add.cFormatTags * sizeof(padid->aFormatTag[0]));
	if (!padid->aFormatTag) goto errCleanUp;
    }

    padid->cFormatTags = add.cFormatTags;
    padid->cFilterTags = add.cFilterTags;
    padid->fdwSupport  = add.fdwSupport;

    aftd.cbStruct = sizeof(aftd);

    for (ntag = 0; ntag < add.cFormatTags; ntag++) {
	aftd.dwFormatTagIndex = ntag;
	if (MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)&aftd, ACM_FORMATTAGDETAILSF_INDEX)) {
	    TRACE("IIOs (%s)\n", debugstr_w(padid->pszDriverAlias));
	    goto errCleanUp;
	}
	padid->aFormatTag[ntag].dwFormatTag = aftd.dwFormatTag;
	padid->aFormatTag[ntag].cbwfx = aftd.cbFormatSize;
    }

    acmDriverClose(had, 0);

    return TRUE;

errCleanUp:
    if (had) acmDriverClose(had, 0);
    HeapFree(MSACM_hHeap, 0, padid->aFormatTag);
    padid->aFormatTag = NULL;
    return FALSE;
}

/***********************************************************************
 *           MSACM_GetRegistryKey
 */
static	LPWSTR	MSACM_GetRegistryKey(const WINE_ACMDRIVERID* padid)
{
    static const WCHAR	baseKey[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
                                     'A','u','d','i','o','C','o','m','p','r','e','s','s','i','o','n','M','a','n','a','g','e','r','\\',
                                     'D','r','i','v','e','r','C','a','c','h','e','\\','\0'};
    LPWSTR	ret;
    int		len;

    if (!padid->pszDriverAlias) {
	ERR("No alias needed for registry entry\n");
	return NULL;
    }
    len = strlenW(baseKey);
    ret = HeapAlloc(MSACM_hHeap, 0, (len + strlenW(padid->pszDriverAlias) + 1) * sizeof(WCHAR));
    if (!ret) return NULL;

    strcpyW(ret, baseKey);
    strcpyW(ret + len, padid->pszDriverAlias);
    CharLowerW(ret + len);
    return ret;
}

/***********************************************************************
 *           MSACM_ReadCache
 */
static BOOL MSACM_ReadCache(PWINE_ACMDRIVERID padid)
{
    LPWSTR	key = MSACM_GetRegistryKey(padid);
    HKEY	hKey;
    DWORD	type, size;

    if (!key) return FALSE;

    padid->aFormatTag = NULL;

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, key, &hKey))
	goto errCleanUp;

    size = sizeof(padid->cFormatTags);
    if (RegQueryValueExA(hKey, "cFormatTags", 0, &type, (void*)&padid->cFormatTags, &size))
	goto errCleanUp;
    size = sizeof(padid->cFilterTags);
    if (RegQueryValueExA(hKey, "cFilterTags", 0, &type, (void*)&padid->cFilterTags, &size))
	goto errCleanUp;
    size = sizeof(padid->fdwSupport);
    if (RegQueryValueExA(hKey, "fdwSupport", 0, &type, (void*)&padid->fdwSupport, &size))
	goto errCleanUp;

    if (padid->cFormatTags > 0) {
	size = padid->cFormatTags * sizeof(padid->aFormatTag[0]);
	padid->aFormatTag = HeapAlloc(MSACM_hHeap, HEAP_ZERO_MEMORY, size);
	if (!padid->aFormatTag) goto errCleanUp;
	if (RegQueryValueExA(hKey, "aFormatTagCache", 0, &type, (void*)padid->aFormatTag, &size))
	    goto errCleanUp;
    }
    HeapFree(MSACM_hHeap, 0, key);
    return TRUE;

 errCleanUp:
    HeapFree(MSACM_hHeap, 0, key);
    HeapFree(MSACM_hHeap, 0, padid->aFormatTag);
    padid->aFormatTag = NULL;
    RegCloseKey(hKey);
    return FALSE;
}

/***********************************************************************
 *           MSACM_WriteCache
 */
static	BOOL MSACM_WriteCache(PWINE_ACMDRIVERID padid)
{
    LPWSTR	key = MSACM_GetRegistryKey(padid);
    HKEY	hKey;

    if (!key) return FALSE;

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, key, &hKey))
	goto errCleanUp;

    if (RegSetValueExA(hKey, "cFormatTags", 0, REG_DWORD, (void*)&padid->cFormatTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "cFilterTags", 0, REG_DWORD, (void*)&padid->cFilterTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "fdwSupport", 0, REG_DWORD, (void*)&padid->fdwSupport, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "aFormatTagCache", 0, REG_BINARY,
		       (void*)padid->aFormatTag,
		       padid->cFormatTags * sizeof(padid->aFormatTag[0])))
	goto errCleanUp;
    HeapFree(MSACM_hHeap, 0, key);
    return TRUE;

 errCleanUp:
    HeapFree(MSACM_hHeap, 0, key);
    return FALSE;
}

/***********************************************************************
 *           MSACM_RegisterDriver()
 */
PWINE_ACMDRIVERID MSACM_RegisterDriver(LPCWSTR pszDriverAlias, LPCWSTR pszFileName,
				       HINSTANCE hinstModule)
{
    PWINE_ACMDRIVERID	padid;

    TRACE("(%s, %s, %p)\n", 
          debugstr_w(pszDriverAlias), debugstr_w(pszFileName), hinstModule);

    padid = (PWINE_ACMDRIVERID) HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVERID));
    padid->obj.dwType = WINE_ACMOBJ_DRIVERID;
    padid->obj.pACMDriverID = padid;
    padid->pszDriverAlias = NULL;
    if (pszDriverAlias)
    {
        padid->pszDriverAlias = HeapAlloc( MSACM_hHeap, 0, (strlenW(pszDriverAlias)+1) * sizeof(WCHAR) );
        strcpyW( padid->pszDriverAlias, pszDriverAlias );
    }
    padid->pszFileName = NULL;
    if (pszFileName)
    {
        padid->pszFileName = HeapAlloc( MSACM_hHeap, 0, (strlenW(pszFileName)+1) * sizeof(WCHAR) );
        strcpyW( padid->pszFileName, pszFileName );
    }
    padid->hInstModule = hinstModule;

    padid->pACMDriverList = NULL;
    padid->pNextACMDriverID = NULL;
    padid->pPrevACMDriverID = MSACM_pLastACMDriverID;
    if (MSACM_pLastACMDriverID)
	MSACM_pLastACMDriverID->pNextACMDriverID = padid;
    MSACM_pLastACMDriverID = padid;
    if (!MSACM_pFirstACMDriverID)
	MSACM_pFirstACMDriverID = padid;
    /* disable the driver if we cannot load the cache */
    if (!MSACM_ReadCache(padid) && !MSACM_FillCache(padid)) {
	WARN("Couldn't load cache for ACM driver (%s)\n", debugstr_w(pszFileName));
	MSACM_UnregisterDriver(padid);
	return NULL;
    }
    return padid;
}

/***********************************************************************
 *           MSACM_RegisterAllDrivers()
 */
void MSACM_RegisterAllDrivers(void)
{
    static const WCHAR msacm32[] = {'m','s','a','c','m','3','2','.','d','l','l','\0'};
    static const WCHAR msacmW[] = {'M','S','A','C','M','.'};
    static const WCHAR drv32[] = {'d','r','i','v','e','r','s','3','2','\0'};
    static const WCHAR sys[] = {'s','y','s','t','e','m','.','i','n','i','\0'};
    static const WCHAR drvkey[] = {'S','o','f','t','w','a','r','e','\\',
				   'M','i','c','r','o','s','o','f','t','\\',
				   'W','i','n','d','o','w','s',' ','N','T','\\',
				   'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
				   'D','r','i','v','e','r','s','3','2','\0'};
    DWORD i, cnt = 0, bufLen, lRet;
    WCHAR buf[2048], *name, *s;
    FILETIME lastWrite;
    HKEY hKey;

    /* FIXME: What if the user edits system.ini while the program is running?
     * Does Windows handle that?  */
    if (MSACM_pFirstACMDriverID) return;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, drvkey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {
	RegQueryInfoKeyW( hKey, 0, 0, 0, &cnt, 0, 0, 0, 0, 0, 0, 0);
	for (i = 0; i < cnt; i++) {
	    bufLen = sizeof(buf) / sizeof(buf[0]);
	    lRet = RegEnumKeyExW(hKey, i, buf, &bufLen, 0, 0, 0, &lastWrite);
	    if (lRet != ERROR_SUCCESS) continue;
	    if (strncmpiW(buf, msacmW, sizeof(msacmW)/sizeof(msacmW[0]))) continue;
	    if (!(name = strchrW(buf, '='))) continue;
	    *name = 0;
	    MSACM_RegisterDriver(buf, name + 1, 0);
	}
    	RegCloseKey( hKey );
    }

    if (GetPrivateProfileSectionW(drv32, buf, sizeof(buf)/sizeof(buf[0]), sys))
    {
	for(s = buf; *s;  s += strlenW(s) + 1)
	{
	    if (strncmpiW(s, msacmW, sizeof(msacmW)/sizeof(msacmW[0]))) continue;
	    if (!(name = strchrW(s, '='))) continue;
	    *name = 0;
	    MSACM_RegisterDriver(s, name + 1, 0);
	    *name = '=';
	}
    }

    MSACM_RegisterDriver(msacm32, msacm32, 0);
}

/***********************************************************************
 *           MSACM_UnregisterDriver()
 */
PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p)
{
    PWINE_ACMDRIVERID pNextACMDriverID;

    while (p->pACMDriverList)
	acmDriverClose((HACMDRIVER) p->pACMDriverList, 0);

    if (p->pszDriverAlias)
	HeapFree(MSACM_hHeap, 0, p->pszDriverAlias);
    if (p->pszFileName)
	HeapFree(MSACM_hHeap, 0, p->pszFileName);
    HeapFree(MSACM_hHeap, 0, p->aFormatTag);

    if (p == MSACM_pFirstACMDriverID)
	MSACM_pFirstACMDriverID = p->pNextACMDriverID;
    if (p == MSACM_pLastACMDriverID)
	MSACM_pLastACMDriverID = p->pPrevACMDriverID;

    if (p->pPrevACMDriverID)
	p->pPrevACMDriverID->pNextACMDriverID = p->pNextACMDriverID;
    if (p->pNextACMDriverID)
	p->pNextACMDriverID->pPrevACMDriverID = p->pPrevACMDriverID;

    pNextACMDriverID = p->pNextACMDriverID;

    HeapFree(MSACM_hHeap, 0, p);

    return pNextACMDriverID;
}

/***********************************************************************
 *           MSACM_UnregisterAllDrivers()
 */
void MSACM_UnregisterAllDrivers(void)
{
    PWINE_ACMDRIVERID p = MSACM_pFirstACMDriverID;

    while (p) {
	MSACM_WriteCache(p);
	p = MSACM_UnregisterDriver(p);
    }
}

/***********************************************************************
 *           MSACM_GetObj()
 */
PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj, DWORD type)
{
    PWINE_ACMOBJ	pao = (PWINE_ACMOBJ)hObj;

    if (pao == NULL || IsBadReadPtr(pao, sizeof(WINE_ACMOBJ)) ||
	((type != WINE_ACMOBJ_DONTCARE) && (type != pao->dwType)))
	return NULL;
    return pao;
}

/***********************************************************************
 *           MSACM_GetDriverID()
 */
PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID)
{
    return (PWINE_ACMDRIVERID)MSACM_GetObj((HACMOBJ)hDriverID, WINE_ACMOBJ_DRIVERID);
}

/***********************************************************************
 *           MSACM_GetDriver()
 */
PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver)
{
    return (PWINE_ACMDRIVER)MSACM_GetObj((HACMOBJ)hDriver, WINE_ACMOBJ_DRIVER);
}

/***********************************************************************
 *           MSACM_Message()
 */
MMRESULT MSACM_Message(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PWINE_ACMDRIVER	pad = MSACM_GetDriver(had);

    return pad ? SendDriverMessage(pad->hDrvr, uMsg, lParam1, lParam2) : MMSYSERR_INVALHANDLE;
}
