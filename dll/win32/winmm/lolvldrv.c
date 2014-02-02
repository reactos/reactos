/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYSTEM low level drivers handling functions
 *
 * Copyright 1999 Eric Pouech
 * Modified for use with ReactOS by Andrew Greenwood, 2007
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

#include "winemm.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

/* each known type of driver has an instance of this structure */
typedef struct tagWINE_LLTYPE {
    /* those attributes depend on the specification of the type */
    LPCSTR		typestr;	/* name (for debugging) */
    BOOL		bSupportMapper;	/* if type is allowed to support mapper */
    /* those attributes reflect the loaded/current situation for the type */
    UINT		wMaxId;		/* number of loaded devices (sum across all loaded drivers) */
    LPWINE_MLD		lpMlds;		/* "static" mlds to access the part though device IDs */
    int			nMapper;	/* index to mapper */
} WINE_LLTYPE;

static int		MMDrvsHi /* = 0 */;
static WINE_MM_DRIVER	MMDrvs[8];
static LPWINE_MLD	MM_MLDrvs[40];
#define MAX_MM_MLDRVS	(sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0]))

#define A(_x,_y) {#_y, _x, 0, NULL, -1}
/* Note: the indices of this array must match the definitions
 *	 of the MMDRV_???? manifest constants
 */
static WINE_LLTYPE	llTypes[MMDRV_MAX] = {
    A(TRUE,  Aux),
    A(FALSE, Mixer),
    A(TRUE,  MidiIn),
    A(TRUE,  MidiOut),
    A(TRUE,  WaveIn),
    A(TRUE,  WaveOut),
};
#undef A

/**************************************************************************
 * 			MMDRV_GetNum				[internal]
 */
UINT	MMDRV_GetNum(UINT type)
{
    TRACE("(%04x)\n", type);
    assert(type < MMDRV_MAX);
    return llTypes[type].wMaxId;
}

/**************************************************************************
 * 				MMDRV_Message			[internal]
 */
DWORD  MMDRV_Message(LPWINE_MLD mld, UINT wMsg, DWORD_PTR dwParam1,
                     DWORD_PTR dwParam2)
{
    LPWINE_MM_DRIVER 		lpDrv;
    DWORD			ret;
    WINE_MM_DRIVER_PART*	part;
    WINE_LLTYPE*		llType = &llTypes[mld->type];
    int				devID;

    TRACE("(%s %u %u 0x%08lx 0x%08lx 0x%08lx)\n",
	  llTypes[mld->type].typestr, mld->uDeviceID, wMsg,
	  mld->dwDriverInstance, dwParam1, dwParam2);

    if (mld->uDeviceID == (UINT16)-1) {
	if (!llType->bSupportMapper) {
	    WARN("uDev=-1 requested on non-mappable ll type %s\n",
		 llTypes[mld->type].typestr);
	    return MMSYSERR_BADDEVICEID;
	}
	devID = -1;
    } else {
	if (mld->uDeviceID >= llType->wMaxId) {
	    WARN("uDev(%u) requested >= max (%d)\n", mld->uDeviceID, llType->wMaxId);
	    return MMSYSERR_BADDEVICEID;
	}
	devID = mld->uDeviceID;
    }

    lpDrv = &MMDrvs[mld->mmdIndex];
    part = &lpDrv->parts[mld->type];

#if 0
    /* some sanity checks */
    if (!(part->nIDMin <= devID))
	ERR("!(part->nIDMin(%d) <= devID(%d))\n", part->nIDMin, devID);
    if (!(devID < part->nIDMax))
	ERR("!(devID(%d) < part->nIDMax(%d))\n", devID, part->nIDMax);
#endif

    assert(part->fnMessage32);

    TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx)\n",
          mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
    ret = part->fnMessage32(mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
    TRACE("=> %s\n", WINMM_ErrorToString(ret));

    return ret;
}

/**************************************************************************
 * 				MMDRV_Alloc			[internal]
 */
LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags,
			    DWORD_PTR* dwCallback, DWORD_PTR* dwInstance)
{
    LPWINE_MLD	mld;
    UINT_PTR i;
    TRACE("(%d, %04x, %p, %p, %p, %p)\n",
          size, type, hndl, dwFlags, dwCallback, dwInstance);

    mld = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!mld)	return NULL;

    /* find an empty slot in MM_MLDrvs table */
    for (i = 0; i < MAX_MM_MLDRVS; i++) if (!MM_MLDrvs[i]) break;

    if (i == MAX_MM_MLDRVS) {
	/* the MM_MLDrvs table could be made growable in the future if needed */
	ERR("Too many open drivers\n");
        HeapFree(GetProcessHeap(), 0, mld);
	return NULL;
    }
    MM_MLDrvs[i] = mld;
    *hndl = (HANDLE)(i | 0x8000);

    mld->type = type;
    if ((UINT_PTR)*hndl < MMDRV_GetNum(type) || ((UINT_PTR)*hndl >> 16)) {
	/* FIXME: those conditions must be fulfilled so that:
	 * - we can distinguish between device IDs and handles
	 * - we can use handles as 16 or 32 bit entities
	 */
	ERR("Shouldn't happen. Bad allocation scheme\n");
    }

    mld->dwFlags = HIWORD(*dwFlags);
    mld->dwCallback = *dwCallback;
    mld->dwClientInstance = *dwInstance;

    return mld;
}

/**************************************************************************
 * 				MMDRV_Free			[internal]
 */
void	MMDRV_Free(HANDLE hndl, LPWINE_MLD mld)
{
    TRACE("(%p, %p)\n", hndl, mld);

    if ((UINT_PTR)hndl & 0x8000) {
	UINT_PTR idx = (UINT_PTR)hndl & ~0x8000;
	if (idx < sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0])) {
	    MM_MLDrvs[idx] = NULL;
	    HeapFree(GetProcessHeap(), 0, mld);
	    return;
	}
    }
    ERR("Bad Handle %p at %p (not freed)\n", hndl, mld);
}

/**************************************************************************
 * 				MMDRV_Open			[internal]
 */
DWORD MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD_PTR dwParam1, DWORD dwFlags)
{
    DWORD		dwRet = MMSYSERR_BADDEVICEID;
    DWORD_PTR		dwInstance;
    WINE_LLTYPE*	llType = &llTypes[mld->type];
    TRACE("(%p, %04x, 0x%08lx, 0x%08x)\n", mld, wMsg, dwParam1, dwFlags);

    mld->dwDriverInstance = (DWORD_PTR)&dwInstance;

    if (mld->uDeviceID == (UINT)-1 || mld->uDeviceID == (UINT16)-1) {
	TRACE("MAPPER mode requested !\n");
	/* check if mapper is supported by type */
	if (llType->bSupportMapper) {
	    if (llType->nMapper == -1) {
		/* no driver for mapper has been loaded, try a dumb implementation */
		TRACE("No mapper loaded, doing it by hand\n");
		for (mld->uDeviceID = 0; mld->uDeviceID < llType->wMaxId; mld->uDeviceID++) {
		    if ((dwRet = MMDRV_Open(mld, wMsg, dwParam1, dwFlags)) == MMSYSERR_NOERROR) {
			/* to share this function epilog */
			dwInstance = mld->dwDriverInstance;
			break;
		    }
		}
	    } else {
		mld->uDeviceID = (UINT16)-1;
		mld->mmdIndex = llType->lpMlds[-1].mmdIndex;
		TRACE("Setting mmdIndex to %u\n", mld->mmdIndex);
		dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags);
	    }
	}
    } else {
	if (mld->uDeviceID < llType->wMaxId) {
	    mld->mmdIndex = llType->lpMlds[mld->uDeviceID].mmdIndex;
	    TRACE("Setting mmdIndex to %u\n", mld->mmdIndex);
	    dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags);
	}
    }
    if (dwRet == MMSYSERR_NOERROR)
	mld->dwDriverInstance = dwInstance;
    return dwRet;
}

/**************************************************************************
 * 				MMDRV_Close			[internal]
 */
DWORD	MMDRV_Close(LPWINE_MLD mld, UINT wMsg)
{
    TRACE("(%p, %04x)\n", mld, wMsg);
    return MMDRV_Message(mld, wMsg, 0L, 0L);
}

/**************************************************************************
 * 				MMDRV_GetByID			[internal]
 */
static LPWINE_MLD MMDRV_GetByID(UINT uDevID, UINT type)
{
    TRACE("(%04x, %04x)\n", uDevID, type);
    if (uDevID < llTypes[type].wMaxId)
	return &llTypes[type].lpMlds[uDevID];
    if ((uDevID == (UINT16)-1 || uDevID == (UINT)-1) && llTypes[type].nMapper != -1)
	return &llTypes[type].lpMlds[-1];
    return NULL;
}

/**************************************************************************
 * 				MMDRV_Get			[internal]
 */
LPWINE_MLD	MMDRV_Get(HANDLE _hndl, UINT type, BOOL bCanBeID)
{
    LPWINE_MLD	mld = NULL;
    UINT_PTR    hndl = (UINT_PTR)_hndl;
    TRACE("(%p, %04x, %c)\n", _hndl, type, bCanBeID ? 'Y' : 'N');

    assert(type < MMDRV_MAX);

    if (hndl >= llTypes[type].wMaxId &&
	hndl != (UINT16)-1 && hndl != (UINT)-1) {
	if (hndl & 0x8000) {
	    UINT idx = hndl & ~0x8000;
	    if (idx < sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0])) {
                __TRY
                {
                    mld = MM_MLDrvs[idx];
                    if (mld && mld->type != type) mld = NULL;
                }
                __EXCEPT_PAGE_FAULT
                {
                    mld = NULL;
                }
                __ENDTRY;
	    }
	}
    }
    if (mld == NULL && bCanBeID) {
	mld = MMDRV_GetByID(hndl, type);
    }
    return mld;
}

/**************************************************************************
 * 				MMDRV_GetRelated		[internal]
 */
LPWINE_MLD	MMDRV_GetRelated(HANDLE hndl, UINT srcType,
				 BOOL bSrcCanBeID, UINT dstType)
{
    LPWINE_MLD		mld;
    TRACE("(%p, %04x, %c, %04x)\n",
          hndl, srcType, bSrcCanBeID ? 'Y' : 'N', dstType);

    if ((mld = MMDRV_Get(hndl, srcType, bSrcCanBeID)) != NULL) {
	WINE_MM_DRIVER_PART*	part = &MMDrvs[mld->mmdIndex].parts[dstType];
	if (part->nIDMin < part->nIDMax)
	    return MMDRV_GetByID(part->nIDMin, dstType);
    }
    return NULL;
}

/**************************************************************************
 * 				MMDRV_PhysicalFeatures		[internal]
 */
UINT	MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg,
                               DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    WINE_MM_DRIVER*	lpDrv = &MMDrvs[mld->mmdIndex];

    TRACE("(%p, %04x, %08lx, %08lx)\n", mld, uMsg, dwParam1, dwParam2);

    /* all those function calls are undocumented */
    switch (uMsg) {
    case DRV_QUERYDRVENTRY:
        lstrcpynA((LPSTR)dwParam1, lpDrv->drvname, LOWORD(dwParam2));
	break;
    case DRV_QUERYDEVNODE:
        *(LPDWORD)dwParam1 = 0L; /* should be DevNode */
	break;
    case DRV_QUERYNAME:
	WARN("NIY QueryName\n");
	break;
    case DRV_QUERYDRIVERIDS:
	WARN("NIY call VxD\n");
	/* should call VxD MMDEVLDR with (DevNode, dwParam1 and dwParam2) as pmts
	 * dwParam1 is buffer and dwParam2 is sizeof(buffer)
	 * I don't know where the result is stored though
	 */
	break;
    case DRV_QUERYMAPPABLE:
	return (lpDrv->bIsMapper) ? 2 : 0;

    case DRVM_MAPPER_PREFERRED_GET:
	/* FIXME: get from registry someday */
        *((LPDWORD)dwParam1) = -1;      /* No preferred device */
        *((LPDWORD)dwParam2) = 0;
        break;

    case DRV_QUERYDEVICEINTERFACE:
    case DRV_QUERYDEVICEINTERFACESIZE:
        return MMDRV_Message(mld, uMsg, dwParam1, dwParam2);

    case DRV_QUERYDSOUNDIFACE: /* Wine-specific: Retrieve DirectSound interface */
    case DRV_QUERYDSOUNDDESC: /* Wine-specific: Retrieve DirectSound driver description*/
	return MMDRV_Message(mld, uMsg, dwParam1, dwParam2);

    default:
	WARN("Unknown call %04x\n", uMsg);
	return MMSYSERR_INVALPARAM;
    }
    return 0L;
}

/**************************************************************************
 * 				MMDRV_InitPerType		[internal]
 */
static  BOOL	MMDRV_InitPerType(LPWINE_MM_DRIVER lpDrv, UINT type, UINT wMsg)
{
    WINE_MM_DRIVER_PART*	part = &lpDrv->parts[type];
    DWORD			ret;
    UINT			count = 0;
    int				i, k;
    TRACE("(%p, %04x, %04x)\n", lpDrv, type, wMsg);

    part->nIDMin = part->nIDMax = 0;

    /* for DRVM_INIT and DRVM_ENABLE, dwParam2 should be PnP node */
    /* the DRVM_ENABLE is only required when the PnP node is non zero */
    if (part->fnMessage32) {
        ret = part->fnMessage32(0, DRVM_INIT, 0L, 0L, 0L);
        TRACE("DRVM_INIT => %s\n", WINMM_ErrorToString(ret));
#if 0
        ret = part->fnMessage32(0, DRVM_ENABLE, 0L, 0L, 0L);
        TRACE("DRVM_ENABLE => %08lx\n", ret);
#endif
        count = part->fnMessage32(0, wMsg, 0L, 0L, 0L);
    }
    else return FALSE;

    TRACE("Got %u dev for (%s:%s)\n", count, lpDrv->drvname, llTypes[type].typestr);
    
    if (HIWORD(count))
        return FALSE;

    /* got some drivers */
    if (lpDrv->bIsMapper) {
	/* it seems native mappers return 0 devices :-( */
	if (llTypes[type].nMapper != -1)
	    ERR("Two mappers for type %s (%d, %s)\n",
		llTypes[type].typestr, llTypes[type].nMapper, lpDrv->drvname);
	if (count > 1)
	    ERR("Strange: mapper with %d > 1 devices\n", count);
	llTypes[type].nMapper = MMDrvsHi;
    } else {
	if (count == 0)
	    return FALSE;
	part->nIDMin = llTypes[type].wMaxId;
	llTypes[type].wMaxId += count;
	part->nIDMax = llTypes[type].wMaxId;
    }
    TRACE("Setting min=%d max=%d (ttop=%d) for (%s:%s)\n",
	  part->nIDMin, part->nIDMax, llTypes[type].wMaxId,
	  lpDrv->drvname, llTypes[type].typestr);
    /* realloc translation table */
    if (llTypes[type].lpMlds)
        llTypes[type].lpMlds = (LPWINE_MLD)
	HeapReAlloc(GetProcessHeap(), 0, llTypes[type].lpMlds - 1,
		    sizeof(WINE_MLD) * (llTypes[type].wMaxId + 1)) + 1;
    else
        llTypes[type].lpMlds = (LPWINE_MLD)
	HeapAlloc(GetProcessHeap(), 0,
		    sizeof(WINE_MLD) * (llTypes[type].wMaxId + 1)) + 1;

    /* re-build the translation table */
    if (llTypes[type].nMapper != -1) {
	TRACE("%s:Trans[%d] -> %s\n", llTypes[type].typestr, -1, MMDrvs[llTypes[type].nMapper].drvname);
	llTypes[type].lpMlds[-1].uDeviceID = (UINT16)-1;
	llTypes[type].lpMlds[-1].type = type;
	llTypes[type].lpMlds[-1].mmdIndex = llTypes[type].nMapper;
	llTypes[type].lpMlds[-1].dwDriverInstance = 0;
    }
    for (i = k = 0; i <= MMDrvsHi; i++) {
	while (MMDrvs[i].parts[type].nIDMin <= k && k < MMDrvs[i].parts[type].nIDMax) {
	    TRACE("%s:Trans[%d] -> %s\n", llTypes[type].typestr, k, MMDrvs[i].drvname);
	    llTypes[type].lpMlds[k].uDeviceID = k;
	    llTypes[type].lpMlds[k].type = type;
	    llTypes[type].lpMlds[k].mmdIndex = i;
	    llTypes[type].lpMlds[k].dwDriverInstance = 0;
	    k++;
	}
    }
    return TRUE;
}

/**************************************************************************
 * 				MMDRV_Install			[internal]
 */
BOOL	MMDRV_Install(LPCSTR drvRegName, LPCSTR drvFileName, BOOL bIsMapper)
{
    int			i, count = 0;
    LPWINE_MM_DRIVER	lpDrv = &MMDrvs[MMDrvsHi];
    LPWINE_DRIVER	d;
    WINEMM_msgFunc32	func;

    TRACE("('%s', '%s', mapper=%c);\n", drvRegName, drvFileName, bIsMapper ? 'Y' : 'N');

    for (i = 0; i < MMDrvsHi; i++) {
        if (!strcmp(drvRegName, MMDrvs[i].drvname)) return FALSE;
    }

    /* Be sure that size of MMDrvs matches the max number of loadable
     * drivers !!
     * If not just increase size of MMDrvs
     */
    assert(MMDrvsHi <= sizeof(MMDrvs)/sizeof(MMDrvs[0]));

    memset(lpDrv, 0, sizeof(*lpDrv));

    if (!(lpDrv->hDriver = OpenDriverA(drvFileName, 0, 0))) {
	WARN("Couldn't open driver '%s'\n", drvFileName);
	return FALSE;
    }

    d = DRIVER_FindFromHDrvr(lpDrv->hDriver);

    if (!(d = DRIVER_FindFromHDrvr(lpDrv->hDriver))) {
	CloseDriver(lpDrv->hDriver, 0, 0);
	WARN("Couldn't get the WINE internal structure for driver '%s'\n", drvFileName);
	return FALSE;
    }

    /* Then look for xxxMessage functions */
#define	AA(_h,_w,_x,_y,_z)					\
    func = (WINEMM_msgFunc##_y) _z ((_h), #_x);			\
    if (func != NULL) 						\
        { lpDrv->parts[_w].fnMessage##_y = func; count++; 	\
          TRACE("Got %d bit func '%s'\n", _y, #_x);         }

    if (d->hModule) {
#define A(_x,_y)	AA(d->hModule,_x,_y,32,GetProcAddress)
	    A(MMDRV_AUX,	auxMessage);
	    A(MMDRV_MIXER,	mxdMessage);
	    A(MMDRV_MIDIIN,	midMessage);
	    A(MMDRV_MIDIOUT, 	modMessage);
	    A(MMDRV_WAVEIN,	widMessage);
	    A(MMDRV_WAVEOUT,	wodMessage);
#undef A
    }
#undef AA

    if (!count) {
	CloseDriver(lpDrv->hDriver, 0, 0);
	WARN("No message functions found\n");
	return FALSE;
    }

    /* FIXME: being a mapper or not should be known by another way */
    /* it's known for NE drvs (the description is of the form '*mapper: *'
     * I don't have any clue for PE drvs
     */
    lpDrv->bIsMapper = bIsMapper;
    lpDrv->drvname = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(drvRegName) + 1), drvRegName);

    /* Finish init and get the count of the devices */
    i = 0;
    if (MMDRV_InitPerType(lpDrv, MMDRV_AUX,     AUXDM_GETNUMDEVS))	i = 1;
    if (MMDRV_InitPerType(lpDrv, MMDRV_MIXER,   MXDM_GETNUMDEVS))	i = 1;
    if (MMDRV_InitPerType(lpDrv, MMDRV_MIDIIN,  MIDM_GETNUMDEVS))	i = 1;
    if (MMDRV_InitPerType(lpDrv, MMDRV_MIDIOUT, MODM_GETNUMDEVS))	i = 1;
    if (MMDRV_InitPerType(lpDrv, MMDRV_WAVEIN,  WIDM_GETNUMDEVS))	i = 1;
    if (MMDRV_InitPerType(lpDrv, MMDRV_WAVEOUT, WODM_GETNUMDEVS))	i = 1;
    /* if all those func calls return FALSE, then the driver must be unloaded */
    if (!i) {
	CloseDriver(lpDrv->hDriver, 0, 0);
	HeapFree(GetProcessHeap(), 0, lpDrv->drvname);
	WARN("Driver initialization failed\n");
	return FALSE;
    }

    MMDrvsHi++;

    return TRUE;
}

/**************************************************************************
 * 				MMDRV_Init
 */
BOOL	MMDRV_Init(void)
{
/* Redundant code, keeping this for reference only (for now) */
#if 0
    HKEY	hKey;
    char	driver_buffer[256];
    char	mapper_buffer[256];
    char	midi_buffer[256];
    char*	p;
    DWORD	type, size;
    BOOL	ret = FALSE;
    TRACE("()\n");

    strcpy(driver_buffer, WINE_DEFAULT_WINMM_DRIVER);
    strcpy(mapper_buffer, WINE_DEFAULT_WINMM_MAPPER);
    strcpy(midi_buffer, WINE_DEFAULT_WINMM_MIDI);

    /* @@ Wine registry key: HKCU\Software\Wine\Drivers */
    if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Drivers", &hKey))
    {
        size = sizeof(driver_buffer);
        if (RegQueryValueExA(hKey, "Audio", 0, &type, (LPVOID)driver_buffer, &size))
            strcpy(driver_buffer, WINE_DEFAULT_WINMM_DRIVER);
    }

    p = driver_buffer;
    while (p)
    {
        char filename[sizeof(driver_buffer)+10];
        char *next = strchr(p, ',');
        if (next) *next++ = 0;
        sprintf( filename, "wine%s.drv", p );
        if ((ret = MMDRV_Install( filename, filename, FALSE ))) break;
        p = next;
    }

    ret |= MMDRV_Install("beepmidi.dll", "beepmidi.dll", FALSE);

    ret |= MMDRV_Install("wavemapper", WINE_DEFAULT_WINMM_MAPPER, TRUE);
    ret |= MMDRV_Install("midimapper", WINE_DEFAULT_WINMM_MIDI, TRUE);
    return ret;
#else
    INT driver_count = 0;

    driver_count += LoadRegistryMMEDrivers(NT_MME_DRIVERS_KEY);
    driver_count += LoadRegistryMMEDrivers(NT_MME_DRIVERS32_KEY);

    /* Explorer doesn't like us failing */
    return TRUE;
//    return ( driver_count > 0 );
#endif
}

/******************************************************************
 *		ExitPerType
 *
 *
 */
static  BOOL	MMDRV_ExitPerType(LPWINE_MM_DRIVER lpDrv, UINT type)
{
    WINE_MM_DRIVER_PART*	part = &lpDrv->parts[type];
    DWORD			ret;
    TRACE("(%p, %04x)\n", lpDrv, type);

    if (part->fnMessage32) {
#if 0
        ret = part->fnMessage32(0, DRVM_DISABLE, 0L, 0L, 0L);
        TRACE("DRVM_DISABLE => %08lx\n", ret);
#endif
        ret = part->fnMessage32(0, DRVM_EXIT, 0L, 0L, 0L);
        TRACE("DRVM_EXIT => %s\n", WINMM_ErrorToString(ret));
    }

    return TRUE;
}

/******************************************************************
 *		Exit
 *
 *
 */
void    MMDRV_Exit(void)
{
    unsigned int i;
    TRACE("()\n");

    for (i = 0; i < sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0]); i++)
    {
        if (MM_MLDrvs[i] != NULL)
        {
            FIXME("Closing while ll-driver open\n");
#if 0
            /* FIXME: should generate a message depending on type */
            MMDRV_Free((HANDLE)(i | 0x8000), MM_MLDrvs[i]);
#endif
        }
    }

    /* unload driver, in reverse order of loading */
    i = MMDrvsHi;
    while (i-- > 0)
    {
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_AUX);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIXER);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIDIIN);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIDIOUT);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_WAVEIN);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_WAVEOUT);
        CloseDriver(MMDrvs[i].hDriver, 0, 0);
    }
    if (llTypes[MMDRV_AUX].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_AUX].lpMlds - 1);
    if (llTypes[MMDRV_MIXER].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_MIXER].lpMlds - 1);
    if (llTypes[MMDRV_MIDIIN].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_MIDIIN].lpMlds - 1);
    if (llTypes[MMDRV_MIDIOUT].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_MIDIOUT].lpMlds - 1);
    if (llTypes[MMDRV_WAVEIN].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_WAVEIN].lpMlds - 1);
    if (llTypes[MMDRV_WAVEOUT].lpMlds)
        HeapFree(GetProcessHeap(), 0, llTypes[MMDRV_WAVEOUT].lpMlds - 1);
}
