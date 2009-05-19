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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
static PWINE_ACMDRIVERID MSACM_pLastACMDriverID;

static DWORD MSACM_suspendBroadcastCount = 0;
static BOOL MSACM_pendingBroadcast = FALSE;
static PWINE_ACMNOTIFYWND MSACM_pFirstACMNotifyWnd = NULL;
static PWINE_ACMNOTIFYWND MSACM_pLastACMNotifyWnd = NULL;

static void MSACM_ReorderDriversByPriority(void);

/***********************************************************************
 *           MSACM_RegisterDriverFromRegistry()
 */
PWINE_ACMDRIVERID MSACM_RegisterDriverFromRegistry(LPCWSTR pszRegEntry)
{
    static const WCHAR msacmW[] = {'M','S','A','C','M','.'};
    static const WCHAR drvkey[] = {'S','o','f','t','w','a','r','e','\\',
				   'M','i','c','r','o','s','o','f','t','\\',
				   'W','i','n','d','o','w','s',' ','N','T','\\',
				   'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
				   'D','r','i','v','e','r','s','3','2','\0'};
    WCHAR buf[2048];
    DWORD bufLen, lRet;
    HKEY hKey;
    PWINE_ACMDRIVERID padid = NULL;
    
    /* The requested registry entry must have the format msacm.XXXXX in order to
       be recognized in any future sessions of msacm
     */
    if (0 == strncmpiW(pszRegEntry, msacmW, sizeof(msacmW)/sizeof(WCHAR))) {
        lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, drvkey, 0, KEY_QUERY_VALUE, &hKey);
        if (lRet != ERROR_SUCCESS) {
            WARN("unable to open registry key - 0x%08x\n", lRet);
        } else {
            bufLen = sizeof(buf);
            lRet = RegQueryValueExW(hKey, pszRegEntry, NULL, NULL, (LPBYTE)buf, &bufLen);
            if (lRet != ERROR_SUCCESS) {
                WARN("unable to query requested subkey %s - 0x%08x\n", debugstr_w(pszRegEntry), lRet);
            } else {
                MSACM_RegisterDriver(pszRegEntry, buf, 0);
            }
            RegCloseKey( hKey );
        }
    }
    return padid;
}

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
BOOL MSACM_FindFormatTagInCache(const WINE_ACMDRIVERID* padid, DWORD fmtTag, LPDWORD idx)
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
    unsigned int		        ntag;
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
static	BOOL MSACM_WriteCache(const WINE_ACMDRIVERID *padid)
{
    LPWSTR	key = MSACM_GetRegistryKey(padid);
    HKEY	hKey;

    if (!key) return FALSE;

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, key, &hKey))
	goto errCleanUp;

    if (RegSetValueExA(hKey, "cFormatTags", 0, REG_DWORD, (const void*)&padid->cFormatTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "cFilterTags", 0, REG_DWORD, (const void*)&padid->cFilterTags, sizeof(DWORD)))
	goto errCleanUp;
    if (RegSetValueExA(hKey, "fdwSupport", 0, REG_DWORD, (const void*)&padid->fdwSupport, sizeof(DWORD)))
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
				       PWINE_ACMLOCALDRIVER pLocalDriver)
{
    PWINE_ACMDRIVERID	padid;

    TRACE("(%s, %s, %p)\n", 
          debugstr_w(pszDriverAlias), debugstr_w(pszFileName), pLocalDriver);

    padid = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVERID));
    if (!padid)
        return NULL;
    padid->obj.dwType = WINE_ACMOBJ_DRIVERID;
    padid->obj.pACMDriverID = padid;
    padid->pszDriverAlias = NULL;
    if (pszDriverAlias)
    {
        padid->pszDriverAlias = HeapAlloc( MSACM_hHeap, 0, (strlenW(pszDriverAlias)+1) * sizeof(WCHAR) );
        if (!padid->pszDriverAlias) {
            HeapFree(MSACM_hHeap, 0, padid);
            return NULL;
        }
        strcpyW( padid->pszDriverAlias, pszDriverAlias );
    }
    padid->pszFileName = NULL;
    if (pszFileName)
    {
        padid->pszFileName = HeapAlloc( MSACM_hHeap, 0, (strlenW(pszFileName)+1) * sizeof(WCHAR) );
        if (!padid->pszFileName) {
            HeapFree(MSACM_hHeap, 0, padid->pszDriverAlias);
            HeapFree(MSACM_hHeap, 0, padid);
            return NULL;
        }
        strcpyW( padid->pszFileName, pszFileName );
    }
    padid->pLocalDriver = pLocalDriver;

    padid->pACMDriverList = NULL;
    
    if (pLocalDriver) {
        padid->pPrevACMDriverID = NULL;
        padid->pNextACMDriverID = MSACM_pFirstACMDriverID;
        if (MSACM_pFirstACMDriverID)
            MSACM_pFirstACMDriverID->pPrevACMDriverID = padid;
        MSACM_pFirstACMDriverID = padid;
        if (!MSACM_pLastACMDriverID)
            MSACM_pLastACMDriverID = padid;
    } else {
        padid->pNextACMDriverID = NULL;
        padid->pPrevACMDriverID = MSACM_pLastACMDriverID;
        if (MSACM_pLastACMDriverID)
	    MSACM_pLastACMDriverID->pNextACMDriverID = padid;
        MSACM_pLastACMDriverID = padid;
        if (!MSACM_pFirstACMDriverID)
	    MSACM_pFirstACMDriverID = padid;
    }
    /* disable the driver if we cannot load the cache */
    if ((!padid->pszDriverAlias || !MSACM_ReadCache(padid)) && !MSACM_FillCache(padid)) {
	WARN("Couldn't load cache for ACM driver (%s)\n", debugstr_w(pszFileName));
	MSACM_UnregisterDriver(padid);
	return NULL;
    }

    if (pLocalDriver) padid->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_LOCAL;
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
    MSACM_ReorderDriversByPriority();
    MSACM_RegisterDriver(msacm32, msacm32, 0);
}

/***********************************************************************
 *           MSACM_RegisterNotificationWindow()
 */
PWINE_ACMNOTIFYWND MSACM_RegisterNotificationWindow(HWND hNotifyWnd, DWORD dwNotifyMsg)
{
    PWINE_ACMNOTIFYWND	panwnd;

    TRACE("(%p,0x%08x)\n", hNotifyWnd, dwNotifyMsg);

    panwnd = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMNOTIFYWND));
    panwnd->obj.dwType = WINE_ACMOBJ_NOTIFYWND;
    panwnd->obj.pACMDriverID = 0;
    panwnd->hNotifyWnd = hNotifyWnd;
    panwnd->dwNotifyMsg = dwNotifyMsg;
    panwnd->fdwSupport = 0;
    
    panwnd->pNextACMNotifyWnd = NULL;
    panwnd->pPrevACMNotifyWnd = MSACM_pLastACMNotifyWnd;
    if (MSACM_pLastACMNotifyWnd)
        MSACM_pLastACMNotifyWnd->pNextACMNotifyWnd = panwnd;
    MSACM_pLastACMNotifyWnd = panwnd;
    if (!MSACM_pFirstACMNotifyWnd)
        MSACM_pFirstACMNotifyWnd = panwnd;

    return panwnd;
}

/***********************************************************************
 *           MSACM_BroadcastNotification()
 */
void MSACM_BroadcastNotification(void)
{
    if (MSACM_suspendBroadcastCount <= 0) {
        PWINE_ACMNOTIFYWND panwnd;

        for (panwnd = MSACM_pFirstACMNotifyWnd; panwnd; panwnd = panwnd->pNextACMNotifyWnd) 
        if (!(panwnd->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED))
            SendMessageW(panwnd->hNotifyWnd, panwnd->dwNotifyMsg, 0, 0);
    } else {
        MSACM_pendingBroadcast = TRUE;
    }
}

/***********************************************************************
 *           MSACM_DisableNotifications()
 */
void MSACM_DisableNotifications(void)
{
    MSACM_suspendBroadcastCount++;
}

/***********************************************************************
 *           MSACM_EnableNotifications()
 */
void MSACM_EnableNotifications(void)
{
    if (MSACM_suspendBroadcastCount > 0) {
        MSACM_suspendBroadcastCount--;
        if (MSACM_suspendBroadcastCount == 0 && MSACM_pendingBroadcast) {
            MSACM_pendingBroadcast = FALSE;
            MSACM_BroadcastNotification();
        }
    }
}

/***********************************************************************
 *           MSACM_UnRegisterNotificationWindow()
 */
PWINE_ACMNOTIFYWND MSACM_UnRegisterNotificationWindow(const WINE_ACMNOTIFYWND *panwnd)
{
    PWINE_ACMNOTIFYWND p;

    for (p = MSACM_pFirstACMNotifyWnd; p; p = p->pNextACMNotifyWnd) {
        if (p == panwnd) {
            PWINE_ACMNOTIFYWND pNext = p->pNextACMNotifyWnd;

            if (p->pPrevACMNotifyWnd) p->pPrevACMNotifyWnd->pNextACMNotifyWnd = p->pNextACMNotifyWnd;
            if (p->pNextACMNotifyWnd) p->pNextACMNotifyWnd->pPrevACMNotifyWnd = p->pPrevACMNotifyWnd;
            if (MSACM_pFirstACMNotifyWnd == p) MSACM_pFirstACMNotifyWnd = p->pNextACMNotifyWnd;
            if (MSACM_pLastACMNotifyWnd == p) MSACM_pLastACMNotifyWnd = p->pPrevACMNotifyWnd;
            HeapFree(MSACM_hHeap, 0, p);
            
            return pNext;
        }
    }
    return NULL;
}

/***********************************************************************
 *           MSACM_RePositionDriver()
 */
void MSACM_RePositionDriver(PWINE_ACMDRIVERID padid, DWORD dwPriority)
{
    PWINE_ACMDRIVERID pTargetPosition = NULL;
                
    /* Remove selected driver from linked list */
    if (MSACM_pFirstACMDriverID == padid) {
        MSACM_pFirstACMDriverID = padid->pNextACMDriverID;
    }
    if (MSACM_pLastACMDriverID == padid) {
        MSACM_pLastACMDriverID = padid->pPrevACMDriverID;
    }
    if (padid->pPrevACMDriverID != NULL) {
        padid->pPrevACMDriverID->pNextACMDriverID = padid->pNextACMDriverID;
    }
    if (padid->pNextACMDriverID != NULL) {
        padid->pNextACMDriverID->pPrevACMDriverID = padid->pPrevACMDriverID;
    }
    
    /* Look up position where selected driver should be */
    if (dwPriority == 1) {
        pTargetPosition = padid->pPrevACMDriverID;
        while (pTargetPosition->pPrevACMDriverID != NULL &&
            !(pTargetPosition->pPrevACMDriverID->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_LOCAL)) {
            pTargetPosition = pTargetPosition->pPrevACMDriverID;
        }
    } else if (dwPriority == -1) {
        pTargetPosition = padid->pNextACMDriverID;
        while (pTargetPosition->pNextACMDriverID != NULL) {
            pTargetPosition = pTargetPosition->pNextACMDriverID;
        }
    }
    
    /* Place selected driver in selected position */
    padid->pPrevACMDriverID = pTargetPosition->pPrevACMDriverID;
    padid->pNextACMDriverID = pTargetPosition;
    if (padid->pPrevACMDriverID != NULL) {
        padid->pPrevACMDriverID->pNextACMDriverID = padid;
    } else {
        MSACM_pFirstACMDriverID = padid;
    }
    if (padid->pNextACMDriverID != NULL) {
        padid->pNextACMDriverID->pPrevACMDriverID = padid;
    } else {
        MSACM_pLastACMDriverID = padid;
    }
}

/***********************************************************************
 *           MSACM_ReorderDriversByPriority()
 * Reorders all drivers based on the priority list indicated by the registry key:
 * HKCU\\Software\\Microsoft\\Multimedia\\Audio Compression Manager\\Priority v4.00
 */
static void MSACM_ReorderDriversByPriority(void)
{
    PWINE_ACMDRIVERID	padid;
    unsigned int iNumDrivers;
    PWINE_ACMDRIVERID * driverList = NULL;
    HKEY hPriorityKey = NULL;
    
    TRACE("\n");
    
    /* Count drivers && alloc corresponding memory for list */
    iNumDrivers = 0;
    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) iNumDrivers++;
    if (iNumDrivers > 1)
    {
        LONG lError;
        static const WCHAR basePriorityKey[] = {
            'S','o','f','t','w','a','r','e','\\',
            'M','i','c','r','o','s','o','f','t','\\',
            'M','u','l','t','i','m','e','d','i','a','\\',
            'A','u','d','i','o',' ','C','o','m','p','r','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
            'P','r','i','o','r','i','t','y',' ','v','4','.','0','0','\0'
        };
        unsigned int i;
        LONG lBufferLength;
        WCHAR szBuffer[256];
        
        driverList = HeapAlloc(MSACM_hHeap, 0, iNumDrivers * sizeof(PWINE_ACMDRIVERID));
        if (!driverList)
        {
            ERR("out of memory\n");
            goto errCleanUp;
        }

        lError = RegOpenKeyW(HKEY_CURRENT_USER, basePriorityKey, &hPriorityKey);
        if (lError != ERROR_SUCCESS) {
            TRACE("RegOpenKeyW failed, possibly key does not exist yet\n");
            hPriorityKey = NULL;
            goto errCleanUp;
        } 
            
        /* Copy drivers into list to simplify linked list modification */
        for (i = 0, padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID, i++)
        {
            driverList[i] = padid;
        }

        /* Query each of the priorities in turn. Alias key is in lowercase. 
            The general form of the priority record is the following:
            "PriorityN" --> "1, msacm.driveralias"
            where N is an integer, and the value is a string of the driver
            alias, prefixed by "1, " for an enabled driver, or "0, " for a
            disabled driver.
            */
        for (i = 0; i < iNumDrivers; i++)
        {
            static const WCHAR priorityTmpl[] = {'P','r','i','o','r','i','t','y','%','l','d','\0'};
            WCHAR szSubKey[17];
            unsigned int iTargetPosition;
            unsigned int iCurrentPosition;
            WCHAR * pAlias;
            static const WCHAR sPrefix[] = {'m','s','a','c','m','.','\0'};
            
            /* Build expected entry name */
            snprintfW(szSubKey, 17, priorityTmpl, i + 1);
            lBufferLength = sizeof(szBuffer);
            lError = RegQueryValueExW(hPriorityKey, szSubKey, NULL, NULL, (LPBYTE)szBuffer, (LPDWORD)&lBufferLength);
            if (lError != ERROR_SUCCESS) continue;

            /* Recovered driver alias should be at this position */
            iTargetPosition = i;
            
            /* Locate driver alias in driver list */
            pAlias = strstrW(szBuffer, sPrefix);
            if (pAlias == NULL) continue;
            
            for (iCurrentPosition = 0; iCurrentPosition < iNumDrivers; iCurrentPosition++) {
                if (strcmpiW(driverList[iCurrentPosition]->pszDriverAlias, pAlias) == 0) 
                    break;
            }
            if (iCurrentPosition < iNumDrivers && iTargetPosition != iCurrentPosition) {
                padid = driverList[iTargetPosition];
                driverList[iTargetPosition] = driverList[iCurrentPosition];
                driverList[iCurrentPosition] = padid;

                /* Locate enabled status */
                if (szBuffer[0] == '1') {
                    driverList[iTargetPosition]->fdwSupport &= ~ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                } else if (szBuffer[0] == '0') {
                    driverList[iTargetPosition]->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                }
            }
        }
        
        /* Re-assign pointers so that linked list traverses the ordered array */
        for (i = 0; i < iNumDrivers; i++) {
            driverList[i]->pPrevACMDriverID = (i > 0) ? driverList[i - 1] : NULL;
            driverList[i]->pNextACMDriverID = (i < iNumDrivers - 1) ? driverList[i + 1] : NULL;
        }
        MSACM_pFirstACMDriverID = driverList[0];
        MSACM_pLastACMDriverID = driverList[iNumDrivers - 1];
    }
    
errCleanUp:
    if (hPriorityKey != NULL) RegCloseKey(hPriorityKey);
    HeapFree(MSACM_hHeap, 0, driverList);
}

/***********************************************************************
 *           MSACM_WriteCurrentPriorities()
 * Writes out current order of driver priorities to registry key:
 * HKCU\\Software\\Microsoft\\Multimedia\\Audio Compression Manager\\Priority v4.00
 */
void MSACM_WriteCurrentPriorities(void)
{
    LONG lError;
    HKEY hPriorityKey;
    static const WCHAR basePriorityKey[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'M','u','l','t','i','m','e','d','i','a','\\',
        'A','u','d','i','o',' ','C','o','m','p','r','e','s','s','i','o','n',' ','M','a','n','a','g','e','r','\\',
        'P','r','i','o','r','i','t','y',' ','v','4','.','0','0','\0'
    };
    PWINE_ACMDRIVERID padid;
    DWORD dwPriorityCounter;
    static const WCHAR priorityTmpl[] = {'P','r','i','o','r','i','t','y','%','l','d','\0'};
    static const WCHAR valueTmpl[] = {'%','c',',',' ','%','s','\0'};
    static const WCHAR converterAlias[] = {'I','n','t','e','r','n','a','l',' ','P','C','M',' ','C','o','n','v','e','r','t','e','r','\0'};
    WCHAR szSubKey[17];
    WCHAR szBuffer[256];

    /* Delete ACM priority key and create it anew */
    lError = RegDeleteKeyW(HKEY_CURRENT_USER, basePriorityKey);
    if (lError != ERROR_SUCCESS && lError != ERROR_FILE_NOT_FOUND) {
        ERR("unable to remove current key %s (0x%08x) - priority changes won't persist past application end.\n",
            debugstr_w(basePriorityKey), lError);
        return;
    }
    lError = RegCreateKeyW(HKEY_CURRENT_USER, basePriorityKey, &hPriorityKey);
    if (lError != ERROR_SUCCESS) {
        ERR("unable to create key %s (0x%08x) - priority changes won't persist past application end.\n",
            debugstr_w(basePriorityKey), lError);
        return;
    }
    
    /* Write current list of priorities */
    for (dwPriorityCounter = 0, padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {        
        if (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_LOCAL) continue;
        if (padid->pszDriverAlias == NULL) continue;    /* internal PCM converter is last */

        /* Build required value name */
        dwPriorityCounter++;
        snprintfW(szSubKey, 17, priorityTmpl, dwPriorityCounter);
        
        /* Value has a 1 in front for enabled drivers and 0 for disabled drivers */
        snprintfW(szBuffer, 256, valueTmpl, (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) ? '0' : '1', padid->pszDriverAlias);
        strlwrW(szBuffer);
        
        lError = RegSetValueExW(hPriorityKey, szSubKey, 0, REG_SZ, (BYTE *)szBuffer, (strlenW(szBuffer) + 1) * sizeof(WCHAR));
        if (lError != ERROR_SUCCESS) {
            ERR("unable to write value for %s under key %s (0x%08x)\n",
                debugstr_w(padid->pszDriverAlias), debugstr_w(basePriorityKey), lError);
        }
    }
    
    /* Build required value name */
    dwPriorityCounter++;
    snprintfW(szSubKey, 17, priorityTmpl, dwPriorityCounter);
        
    /* Value has a 1 in front for enabled drivers and 0 for disabled drivers */
    snprintfW(szBuffer, 256, valueTmpl, '1', converterAlias);
        
    lError = RegSetValueExW(hPriorityKey, szSubKey, 0, REG_SZ, (BYTE *)szBuffer, (strlenW(szBuffer) + 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS) {
        ERR("unable to write value for %s under key %s (0x%08x)\n",
            debugstr_w(converterAlias), debugstr_w(basePriorityKey), lError);
    }
    RegCloseKey(hPriorityKey);
}

static PWINE_ACMLOCALDRIVER MSACM_pFirstACMLocalDriver;
static PWINE_ACMLOCALDRIVER MSACM_pLastACMLocalDriver;

static PWINE_ACMLOCALDRIVER MSACM_UnregisterLocalDriver(PWINE_ACMLOCALDRIVER paldrv)
{
    PWINE_ACMLOCALDRIVER pNextACMLocalDriver;

    if (paldrv->pACMInstList) {
        ERR("local driver instances still present after closing all drivers - memory leak\n");
        return NULL;
    }

    if (paldrv == MSACM_pFirstACMLocalDriver)
        MSACM_pFirstACMLocalDriver = paldrv->pNextACMLocalDrv;
    if (paldrv == MSACM_pLastACMLocalDriver)
        MSACM_pLastACMLocalDriver = paldrv->pPrevACMLocalDrv;

    if (paldrv->pPrevACMLocalDrv)
        paldrv->pPrevACMLocalDrv->pNextACMLocalDrv = paldrv->pNextACMLocalDrv;
    if (paldrv->pNextACMLocalDrv)
        paldrv->pNextACMLocalDrv->pPrevACMLocalDrv = paldrv->pPrevACMLocalDrv;

    pNextACMLocalDriver = paldrv->pNextACMLocalDrv;

    HeapFree(MSACM_hHeap, 0, paldrv);

    return pNextACMLocalDriver;
}

/***********************************************************************
 *           MSACM_UnregisterDriver()
 */
PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p)
{
    PWINE_ACMDRIVERID pNextACMDriverID;

    while (p->pACMDriverList)
	acmDriverClose((HACMDRIVER) p->pACMDriverList, 0);

    HeapFree(MSACM_hHeap, 0, p->pszDriverAlias);
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

    if (p->pLocalDriver) MSACM_UnregisterLocalDriver(p->pLocalDriver);
    HeapFree(MSACM_hHeap, 0, p);

    return pNextACMDriverID;
}

/***********************************************************************
 *           MSACM_UnregisterAllDrivers()
 */
void MSACM_UnregisterAllDrivers(void)
{
    PWINE_ACMNOTIFYWND panwnd = MSACM_pFirstACMNotifyWnd;
    PWINE_ACMDRIVERID p = MSACM_pFirstACMDriverID;

    while (p) {
	MSACM_WriteCache(p);
	p = MSACM_UnregisterDriver(p);
    }
    
    while (panwnd) {
	panwnd = MSACM_UnRegisterNotificationWindow(panwnd);
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
 *           MSACM_GetNotifyWnd()
 */
PWINE_ACMNOTIFYWND MSACM_GetNotifyWnd(HACMDRIVERID hDriver)
{
    return (PWINE_ACMNOTIFYWND)MSACM_GetObj((HACMOBJ)hDriver, WINE_ACMOBJ_NOTIFYWND);
}

/***********************************************************************
 *           MSACM_GetLocalDriver()
 */
/* 
PWINE_ACMLOCALDRIVER MSACM_GetLocalDriver(HACMDRIVER hDriver)
{
    return (PWINE_ACMLOCALDRIVER)MSACM_GetObj((HACMOBJ)hDriver, WINE_ACMOBJ_LOCALDRIVER);
}
*/
#define MSACM_DRIVER_SendMessage(PDRVRINST, msg, lParam1, lParam2) \
        (PDRVRINST)->pLocalDriver->lpDrvProc((PDRVRINST)->dwDriverID, (HDRVR)(PDRVRINST), msg, lParam1, lParam2)

/***********************************************************************
 *           MSACM_Message()
 */
MMRESULT MSACM_Message(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PWINE_ACMDRIVER	pad = MSACM_GetDriver(had);

    if (!pad) return MMSYSERR_INVALHANDLE;
    if (pad->hDrvr) return SendDriverMessage(pad->hDrvr, uMsg, lParam1, lParam2);
    if (pad->pLocalDrvrInst) return MSACM_DRIVER_SendMessage(pad->pLocalDrvrInst, uMsg, lParam1, lParam2);

    return MMSYSERR_INVALHANDLE;
}

PWINE_ACMLOCALDRIVER MSACM_RegisterLocalDriver(HMODULE hModule, DRIVERPROC lpDriverProc)
{
    PWINE_ACMLOCALDRIVER paldrv;

    TRACE("(%p, %p)\n", hModule, lpDriverProc);
    if (!hModule || !lpDriverProc) return NULL;
    
    /* look up previous instance of local driver module */
    for (paldrv = MSACM_pFirstACMLocalDriver; paldrv; paldrv = paldrv->pNextACMLocalDrv)
    {
        if (paldrv->hModule == hModule && paldrv->lpDrvProc == lpDriverProc) return paldrv;
    }

    paldrv = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMLOCALDRIVER));
    paldrv->obj.dwType = WINE_ACMOBJ_LOCALDRIVER;
    paldrv->obj.pACMDriverID = 0;
    paldrv->hModule = hModule;
    paldrv->lpDrvProc = lpDriverProc;
    paldrv->pACMInstList = NULL;

    paldrv->pNextACMLocalDrv = NULL;
    paldrv->pPrevACMLocalDrv = MSACM_pLastACMLocalDriver;
    if (MSACM_pLastACMLocalDriver)
	MSACM_pLastACMLocalDriver->pNextACMLocalDrv = paldrv;
    MSACM_pLastACMLocalDriver = paldrv;
    if (!MSACM_pFirstACMLocalDriver)
	MSACM_pFirstACMLocalDriver = paldrv;

    return paldrv;
}

/**************************************************************************
 *			MSACM_GetNumberOfModuleRefs		[internal]
 *
 * Returns the number of open drivers which share the same module.
 * Inspired from implementation in dlls/winmm/driver.c
 */
static unsigned MSACM_GetNumberOfModuleRefs(HMODULE hModule, DRIVERPROC lpDrvProc, WINE_ACMLOCALDRIVERINST ** found)
{
    PWINE_ACMLOCALDRIVER lpDrv;
    unsigned		count = 0;

    if (found) *found = NULL;
    for (lpDrv = MSACM_pFirstACMLocalDriver; lpDrv; lpDrv = lpDrv->pNextACMLocalDrv)
    {
	if (lpDrv->hModule == hModule && lpDrv->lpDrvProc == lpDrvProc)
        {
            PWINE_ACMLOCALDRIVERINST pInst = lpDrv->pACMInstList;
        
	    while (pInst) {
                if (found && !*found) *found = pInst;
	        count++;
	        pInst = pInst->pNextACMInst;
	    }
	}
    }
    return count;
}

/**************************************************************************
 *				MSACM_RemoveFromList		[internal]
 *
 * Generates all the logic to handle driver closure / deletion
 * Removes a driver struct to the list of open drivers.
 */
static	BOOL	MSACM_RemoveFromList(PWINE_ACMLOCALDRIVERINST lpDrv)
{
    PWINE_ACMLOCALDRIVER pDriverBase = lpDrv->pLocalDriver;
    PWINE_ACMLOCALDRIVERINST pPrevInst;

    /* last of this driver in list ? */
    if (MSACM_GetNumberOfModuleRefs(pDriverBase->hModule, pDriverBase->lpDrvProc, NULL) == 1) {
        MSACM_DRIVER_SendMessage(lpDrv, DRV_DISABLE, 0L, 0L);
        MSACM_DRIVER_SendMessage(lpDrv, DRV_FREE,    0L, 0L);
    }
    
    pPrevInst = NULL;
    if (pDriverBase->pACMInstList != lpDrv) {
        pPrevInst = pDriverBase->pACMInstList;
        while (pPrevInst && pPrevInst->pNextACMInst != lpDrv)
            pPrevInst = pPrevInst->pNextACMInst;
        if (!pPrevInst) {
            ERR("requested to remove invalid instance %p\n", pPrevInst);
            return FALSE;
        }
    }
    if (!pPrevInst) {
        /* first driver instance on list */
        pDriverBase->pACMInstList = lpDrv->pNextACMInst;
    } else {
        pPrevInst->pNextACMInst = lpDrv->pNextACMInst;
    }
    return TRUE;
}

/**************************************************************************
 *				MSACM_AddToList		[internal]
 *
 * Adds a driver struct to the list of open drivers.
 * Generates all the logic to handle driver creation / open.
 */
static	BOOL	MSACM_AddToList(PWINE_ACMLOCALDRIVERINST lpNewDrv, LPARAM lParam2)
{
    PWINE_ACMLOCALDRIVER pDriverBase = lpNewDrv->pLocalDriver;

    /* first of this driver in list ? */
    if (MSACM_GetNumberOfModuleRefs(pDriverBase->hModule, pDriverBase->lpDrvProc, NULL) == 0) {
        if (MSACM_DRIVER_SendMessage(lpNewDrv, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) {
            FIXME("DRV_LOAD failed on driver %p\n", lpNewDrv);
            return FALSE;
        }
        /* returned value is not checked */
        MSACM_DRIVER_SendMessage(lpNewDrv, DRV_ENABLE, 0L, 0L);
    }

    lpNewDrv->pNextACMInst = NULL;
    if (pDriverBase->pACMInstList == NULL) {
	pDriverBase->pACMInstList = lpNewDrv;
    } else {
        PWINE_ACMLOCALDRIVERINST lpDrvInst = pDriverBase->pACMInstList;
    
        while (lpDrvInst->pNextACMInst != NULL)
            lpDrvInst = lpDrvInst->pNextACMInst;

	lpDrvInst->pNextACMInst = lpNewDrv;
    }

    /* Now just open a new instance of a driver on this module */
    lpNewDrv->dwDriverID = MSACM_DRIVER_SendMessage(lpNewDrv, DRV_OPEN, 0, lParam2);

    if (lpNewDrv->dwDriverID == 0) {
        FIXME("DRV_OPEN failed on driver %p\n", lpNewDrv);
        MSACM_RemoveFromList(lpNewDrv);
        return FALSE;
    }
    return TRUE;
}

PWINE_ACMLOCALDRIVERINST MSACM_OpenLocalDriver(PWINE_ACMLOCALDRIVER paldrv, LPARAM lParam2)
{
    PWINE_ACMLOCALDRIVERINST pDrvInst;
    
    pDrvInst = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMLOCALDRIVERINST));
    pDrvInst->pLocalDriver = paldrv;
    pDrvInst->dwDriverID = 0;
    pDrvInst->pNextACMInst = NULL;
    pDrvInst->bSession = FALSE;
    
    /* Win32 installable drivers must support a two phase opening scheme:
     * + first open with NULL as lParam2 (session instance),
     * + then do a second open with the real non null lParam2)
     */
    if (MSACM_GetNumberOfModuleRefs(paldrv->hModule, paldrv->lpDrvProc, NULL) == 0 && lParam2)
    {
        PWINE_ACMLOCALDRIVERINST   ret;

        if (!MSACM_AddToList(pDrvInst, 0L))
        {
            ERR("load0 failed\n");
            goto exit;
        }
        ret = MSACM_OpenLocalDriver(paldrv, lParam2);
        if (!ret)
        {
            MSACM_CloseLocalDriver(pDrvInst);
            ERR("load1 failed\n");
            goto exit;
        }
        pDrvInst->bSession = TRUE;
        return ret;
    }
    
    if (!MSACM_AddToList(pDrvInst, lParam2))
    {
        ERR("load failed\n");
        goto exit;
    }

    TRACE("=> %p\n", pDrvInst);
    return pDrvInst;
exit:
    HeapFree(MSACM_hHeap, 0, pDrvInst);
    return NULL;
}

LRESULT MSACM_CloseLocalDriver(PWINE_ACMLOCALDRIVERINST paldrv)
{
    if (MSACM_RemoveFromList(paldrv)) {
        PWINE_ACMLOCALDRIVERINST lpDrv0;
        PWINE_ACMLOCALDRIVER pDriverBase = paldrv->pLocalDriver;
    
        MSACM_DRIVER_SendMessage(paldrv, DRV_CLOSE, 0, 0);
        paldrv->dwDriverID = 0;
    
        if (paldrv->bSession)
            ERR("should not directly close session instance (%p)\n", paldrv);

        /* if driver has an opened session instance, we have to close it too */
        if (MSACM_GetNumberOfModuleRefs(pDriverBase->hModule, pDriverBase->lpDrvProc, &lpDrv0) == 1 &&
                lpDrv0->bSession)
        {
            MSACM_DRIVER_SendMessage(lpDrv0, DRV_CLOSE, 0L, 0L);
            lpDrv0->dwDriverID = 0;
            MSACM_RemoveFromList(lpDrv0);
            HeapFree(GetProcessHeap(), 0, lpDrv0);
        }

        HeapFree(MSACM_hHeap, 0, paldrv);
        return TRUE;
    }
    ERR("unable to close driver instance\n");
    return FALSE;
}
