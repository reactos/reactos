/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM low level drivers handling functions
 *
 * Copyright 1999 Eric Pouech
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
#include <stdio.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winver.h"
#include "winemm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

LRESULT         (*pFnCallMMDrvFunc16)(DWORD,WORD,WORD,LONG,LONG,LONG) /* = NULL */;
unsigned        (*pFnLoadMMDrvFunc16)(LPCSTR,LPWINE_DRIVER, LPWINE_MM_DRIVER) /* = NULL */;

/* each known type of driver has an instance of this structure */
typedef struct tagWINE_LLTYPE {
    /* those attributes depend on the specification of the type */
    LPCSTR		typestr;	/* name (for debugging) */
    BOOL		bSupportMapper;	/* if type is allowed to support mapper */
    MMDRV_MAPFUNC	Map16To32A;	/* those are function pointers to handle */
    MMDRV_UNMAPFUNC	UnMap16To32A;	/*   the parameter conversion (16 vs 32 bit) */
    MMDRV_MAPFUNC	Map32ATo16; 	/*   when hi-func (in mmsystem or winmm) and */
    MMDRV_UNMAPFUNC	UnMap32ATo16;	/*   low-func (in .drv) do not match */
    LPDRVCALLBACK	Callback;       /* handles callback for a specified type */
    /* those attributes reflect the loaded/current situation for the type */
    UINT		wMaxId;		/* number of loaded devices (sum across all loaded drivers */
    LPWINE_MLD		lpMlds;		/* "static" mlds to access the part though device IDs */
    int			nMapper;	/* index to mapper */
} WINE_LLTYPE;

static int		MMDrvsHi /* = 0 */;
static WINE_MM_DRIVER	MMDrvs[8];
static LPWINE_MLD	MM_MLDrvs[40];
#define MAX_MM_MLDRVS	(sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0]))

#define A(_x,_y) {#_y, _x, NULL, NULL, NULL, NULL, NULL, 0, NULL, -1}
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

/******************************************************************
 *		MMDRV_InstallMap
 *
 *
 */
void    MMDRV_InstallMap(unsigned int drv, 
                         MMDRV_MAPFUNC mp1632, MMDRV_UNMAPFUNC um1632,
                         MMDRV_MAPFUNC mp3216, MMDRV_UNMAPFUNC um3216,
                         LPDRVCALLBACK cb)
{
    assert(drv < MMDRV_MAX);
    llTypes[drv].Map16To32A   = mp1632;
    llTypes[drv].UnMap16To32A = um1632;
    llTypes[drv].Map32ATo16   = mp3216;
    llTypes[drv].UnMap32ATo16 = um1632;
    llTypes[drv].Callback     = cb;
}

/******************************************************************
 *		MMDRV_Is32
 *
 */
BOOL            MMDRV_Is32(unsigned int idx)
{
    TRACE("(%d)\n", idx);
    return MMDrvs[idx].bIs32;
}

/**************************************************************************
 * 				MMDRV_GetDescription32		[internal]
 */
static	BOOL	MMDRV_GetDescription32(const char* fname, char* buf, int buflen)
{
    OFSTRUCT   	ofs;
    DWORD	h;
    LPVOID	ptr = 0;
    LPVOID	val;
    DWORD	dw;
    BOOL	ret = FALSE;
    UINT	u;
    FARPROC pGetFileVersionInfoSizeA;
    FARPROC pGetFileVersionInfoA;
    FARPROC pVerQueryValueA;
    HMODULE hmodule = 0;
    TRACE("(%p, %p, %d)\n", fname, buf, buflen);

#define E(_x)	do {TRACE _x;goto theEnd;} while(0)

    if (OpenFile(fname, &ofs, OF_EXIST)==HFILE_ERROR)		E(("Can't find file %s\n", fname));

    if (!(hmodule = LoadLibraryA( "version.dll" ))) goto theEnd;
    if (!(pGetFileVersionInfoSizeA = GetProcAddress( hmodule, "GetFileVersionInfoSizeA" )))
        goto theEnd;
    if (!(pGetFileVersionInfoA = GetProcAddress( hmodule, "GetFileVersionInfoA" )))
        goto theEnd;
    if (!(pVerQueryValueA = GetProcAddress( hmodule, "VerQueryValueA" )))
        goto theEnd;

    if (!(dw = pGetFileVersionInfoSizeA(ofs.szPathName, &h)))	E(("Can't get FVIS\n"));
    if (!(ptr = HeapAlloc(GetProcessHeap(), 0, dw)))		E(("OOM\n"));
    if (!pGetFileVersionInfoA(ofs.szPathName, h, dw, ptr))	E(("Can't get FVI\n"));

#define	A(_x) if (pVerQueryValueA(ptr, "\\StringFileInfo\\040904B0\\" #_x, &val, &u)) \
                  TRACE(#_x " => %s\n", (LPSTR)val); else TRACE(#_x " @\n")

    A(CompanyName);
    A(FileDescription);
    A(FileVersion);
    A(InternalName);
    A(LegalCopyright);
    A(OriginalFilename);
    A(ProductName);
    A(ProductVersion);
    A(Comments);
    A(LegalTrademarks);
    A(PrivateBuild);
    A(SpecialBuild);
#undef A

    if (!pVerQueryValueA(ptr, "\\StringFileInfo\\040904B0\\ProductName", &val, &u)) E(("Can't get product name\n"));
    lstrcpynA(buf, val, buflen);

#undef E
    ret = TRUE;
theEnd:
    HeapFree(GetProcessHeap(), 0, ptr);
    if (hmodule) FreeLibrary( hmodule );
    return ret;
}

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
                     DWORD_PTR dwParam2, BOOL bFrom32)
{
    LPWINE_MM_DRIVER 		lpDrv;
    DWORD			ret;
    WINE_MM_DRIVER_PART*	part;
    WINE_LLTYPE*		llType = &llTypes[mld->type];
    WINMM_MapType		map;
    int				devID;

    TRACE("(%s %u %u 0x%08lx 0x%08lx 0x%08lx %c)\n",
	  llTypes[mld->type].typestr, mld->uDeviceID, wMsg,
	  mld->dwDriverInstance, dwParam1, dwParam2, bFrom32?'Y':'N');

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

    if (lpDrv->bIs32) {
	assert(part->u.fnMessage32);

	if (bFrom32) {
	    TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx)\n",
		  mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
            ret = part->u.fnMessage32(mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
	    TRACE("=> %s\n", WINMM_ErrorToString(ret));
	} else {
	    map = llType->Map16To32A(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
	    switch (map) {
	    case WINMM_MAP_NOMEM:
		ret = MMSYSERR_NOMEM;
		break;
	    case WINMM_MAP_MSGERROR:
		FIXME("NIY: no conversion yet 16->32 (%u)\n", wMsg);
		ret = MMSYSERR_ERROR;
		break;
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx)\n",
		      mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
		ret = part->u.fnMessage32(mld->uDeviceID, wMsg, mld->dwDriverInstance,
					  dwParam1, dwParam2);
	        TRACE("=> %s\n", WINMM_ErrorToString(ret));
		if (map == WINMM_MAP_OKMEM)
		    llType->UnMap16To32A(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2, ret);
		break;
	    default:
		FIXME("NIY\n");
		ret = MMSYSERR_NOTSUPPORTED;
		break;
	    }
	}
    } else {
	assert(part->u.fnMessage16 && pFnCallMMDrvFunc16);
        
	if (bFrom32) {
	    map = llType->Map32ATo16(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2);
	    switch (map) {
	    case WINMM_MAP_NOMEM:
		ret = MMSYSERR_NOMEM;
		break;
	    case WINMM_MAP_MSGERROR:
		FIXME("NIY: no conversion yet 32->16 (%u)\n", wMsg);
		ret = MMSYSERR_ERROR;
		break;
	    case WINMM_MAP_OK:
	    case WINMM_MAP_OKMEM:
		TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx)\n",
		      mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
		ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16, 
                                         mld->uDeviceID, wMsg, mld->dwDriverInstance, 
                                         dwParam1, dwParam2);
	        TRACE("=> %s\n", WINMM_ErrorToString(ret));
		if (map == WINMM_MAP_OKMEM)
		    llType->UnMap32ATo16(wMsg, &mld->dwDriverInstance, &dwParam1, &dwParam2, ret);
		break;
	    default:
		FIXME("NIY\n");
		ret = MMSYSERR_NOTSUPPORTED;
		break;
	    }
	} else {
	    TRACE("Calling message(dev=%u msg=%u usr=0x%08lx p1=0x%08lx p2=0x%08lx)\n",
		  mld->uDeviceID, wMsg, mld->dwDriverInstance, dwParam1, dwParam2);
            ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16, 
                                     mld->uDeviceID, wMsg, mld->dwDriverInstance, 
                                     dwParam1, dwParam2);
	    TRACE("=> %s\n", WINMM_ErrorToString(ret));
	}
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_Alloc			[internal]
 */
LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags,
			    DWORD* dwCallback, DWORD* dwInstance, BOOL bFrom32)
{
    LPWINE_MLD	mld;
    UINT i;
    TRACE("(%d, %04x, %p, %p, %p, %p, %c)\n",
          size, type, hndl, dwFlags, dwCallback, dwInstance, bFrom32?'Y':'N');

    mld = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!mld)	return NULL;

    /* find an empty slot in MM_MLDrvs table */
    for (i = 0; i < MAX_MM_MLDRVS; i++) if (!MM_MLDrvs[i]) break;

    if (i == MAX_MM_MLDRVS) {
	/* the MM_MLDrvs table could be made growable in the future if needed */
	ERR("Too many open drivers\n");
	return NULL;
    }
    MM_MLDrvs[i] = mld;
    *hndl = (HANDLE)(i | 0x8000);

    mld->type = type;
    if ((UINT)*hndl < MMDRV_GetNum(type) || HIWORD(*hndl) != 0) {
	/* FIXME: those conditions must be fulfilled so that:
	 * - we can distinguish between device IDs and handles
	 * - we can use handles as 16 or 32 bit entities
	 */
	ERR("Shouldn't happen. Bad allocation scheme\n");
    }

    mld->bFrom32 = bFrom32;
    mld->dwFlags = HIWORD(*dwFlags);
    mld->dwCallback = *dwCallback;
    mld->dwClientInstance = *dwInstance;

    if (llTypes[type].Callback)
    {
        *dwFlags = LOWORD(*dwFlags) | CALLBACK_FUNCTION;
        *dwCallback = (DWORD)llTypes[type].Callback;
        *dwInstance = (DWORD)mld; /* FIXME: wouldn't some 16 bit drivers only use the loword ? */
    }

    return mld;
}

/**************************************************************************
 * 				MMDRV_Free			[internal]
 */
void	MMDRV_Free(HANDLE hndl, LPWINE_MLD mld)
{
    TRACE("(%p, %p)\n", hndl, mld);

    if ((UINT)hndl & 0x8000) {
	unsigned idx = (UINT)hndl & ~0x8000;
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
DWORD	MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD dwParam1, DWORD dwFlags)
{
    DWORD		dwRet = MMSYSERR_BADDEVICEID;
    DWORD		dwInstance;
    WINE_LLTYPE*	llType = &llTypes[mld->type];
    TRACE("(%p, %04x, 0x%08lx, 0x%08lx)\n", mld, wMsg, dwParam1, dwFlags);

    mld->dwDriverInstance = (DWORD)&dwInstance;

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
		dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags, TRUE);
	    }
	}
    } else {
	if (mld->uDeviceID < llType->wMaxId) {
	    mld->mmdIndex = llType->lpMlds[mld->uDeviceID].mmdIndex;
	    TRACE("Setting mmdIndex to %u\n", mld->mmdIndex);
	    dwRet = MMDRV_Message(mld, wMsg, dwParam1, dwFlags, TRUE);
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
    return MMDRV_Message(mld, wMsg, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				MMDRV_GetByID			[internal]
 */
LPWINE_MLD	MMDRV_GetByID(UINT uDevID, UINT type)
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
    UINT        hndl = (UINT)_hndl;
    TRACE("(%p, %04x, %c)\n", _hndl, type, bCanBeID ? 'Y' : 'N');

    assert(type < MMDRV_MAX);

    if (hndl >= llTypes[type].wMaxId &&
	hndl != (UINT16)-1 && hndl != (UINT)-1) {
	if (hndl & 0x8000) {
	    hndl = hndl & ~0x8000;
	    if (hndl < sizeof(MM_MLDrvs) / sizeof(MM_MLDrvs[0])) {
		mld = MM_MLDrvs[hndl];
		if (!mld || !HeapValidate(GetProcessHeap(), 0, mld) || mld->type != type)
		    mld = NULL;
	    }
	    hndl = hndl | 0x8000;
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
UINT	MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg, DWORD dwParam1,
			       DWORD dwParam2)
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
        break;

    case DRV_QUERYDEVICEINTERFACE:
    case DRV_QUERYDEVICEINTERFACESIZE:
        return MMDRV_Message(mld, uMsg, dwParam1, dwParam2, TRUE);

#ifdef __WINESRC__
    case DRV_QUERYDSOUNDIFACE: /* Wine-specific: Retrieve DirectSound interface */
    case DRV_QUERYDSOUNDDESC: /* Wine-specific: Retrieve DirectSound driver description*/
    case DRV_QUERYDSOUNDGUID: /* Wine-specific: Retrieve DirectSound driver GUID */
	return MMDRV_Message(mld, uMsg, dwParam1, dwParam2, TRUE);
#endif

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

    if (lpDrv->bIs32 && part->u.fnMessage32) {
	ret = part->u.fnMessage32(0, DRVM_INIT, 0L, 0L, 0L);
	TRACE("DRVM_INIT => %s\n", WINMM_ErrorToString(ret));
#if 0
	ret = part->u.fnMessage32(0, DRVM_ENABLE, 0L, 0L, 0L);
	TRACE("DRVM_ENABLE => %08lx\n", ret);
#endif
        count = part->u.fnMessage32(0, wMsg, 0L, 0L, 0L);
    } else if (!lpDrv->bIs32 && part->u.fnMessage16 && pFnCallMMDrvFunc16) {
        ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16,
                                 0, DRVM_INIT, 0L, 0L, 0L);
	TRACE("DRVM_INIT => %s\n", WINMM_ErrorToString(ret));
#if 0
	ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16,
                                 0, DRVM_ENABLE, 0L, 0L, 0L);
	TRACE("DRVM_ENABLE => %08lx\n", ret);
#endif
        count = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16,
                                   0, wMsg, 0L, 0L, 0L);
    } else {
	return FALSE;
    }

    TRACE("Got %u dev for (%s:%s)\n", count, lpDrv->drvname, llTypes[type].typestr);

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
static	BOOL	MMDRV_Install(LPCSTR drvRegName, LPCSTR drvFileName, BOOL bIsMapper)
{
    int			i, count = 0;
    LPWINE_MM_DRIVER	lpDrv = &MMDrvs[MMDrvsHi];
    LPWINE_DRIVER	d;

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
    lpDrv->bIs32 = (d->dwFlags & WINE_GDF_16BIT) ? FALSE : TRUE;

    /* Then look for xxxMessage functions */
#define	AA(_h,_w,_x,_y,_z)					\
    func = (WINEMM_msgFunc##_y) _z ((_h), #_x);			\
    if (func != NULL) 						\
        { lpDrv->parts[_w].u.fnMessage##_y = func; count++; 	\
          TRACE("Got %d bit func '%s'\n", _y, #_x);         }

    if (lpDrv->bIs32) {
	WINEMM_msgFunc32	func;
        char    		buffer[128];

	if (d->d.d32.hModule) {
#define A(_x,_y)	AA(d->d.d32.hModule,_x,_y,32,GetProcAddress)
	    A(MMDRV_AUX,	auxMessage);
	    A(MMDRV_MIXER,	mxdMessage);
	    A(MMDRV_MIDIIN,	midMessage);
	    A(MMDRV_MIDIOUT, 	modMessage);
	    A(MMDRV_WAVEIN,	widMessage);
	    A(MMDRV_WAVEOUT,	wodMessage);
#undef A
	}
        if (TRACE_ON(winmm)) {
            if (MMDRV_GetDescription32(drvFileName, buffer, sizeof(buffer)))
		TRACE("%s => %s\n", drvFileName, buffer);
	    else
		TRACE("%s => No description\n", drvFileName);
        }
    } else if (WINMM_CheckForMMSystem() && pFnLoadMMDrvFunc16) {
        count += pFnLoadMMDrvFunc16(drvFileName, d, lpDrv);
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
 * 				MMDRV_InitFromRegistry		[internal]
 */
static BOOL	MMDRV_InitFromRegistry(void)
{
    HKEY	hKey;
    char	buffer[256];
    char*	p1;
    char*	p2;
    DWORD	type, size;
    BOOL	ret = FALSE;
    TRACE("()\n");

    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\WinMM", &hKey)) {
	TRACE("Cannot open WinMM config key\n");
	return FALSE;
    }

    size = sizeof(buffer);
    if (!RegQueryValueExA(hKey, "Drivers", 0, &type, (LPVOID)buffer, &size)) {
	p1 = buffer;
	while (p1) {
	    p2 = strchr(p1, ';');
	    if (p2) *p2++ = '\0';
	    ret |= MMDRV_Install(p1, p1, FALSE);
	    p1 = p2;
	}
    }

    /* finish with mappers */
    size = sizeof(buffer);
    if (!RegQueryValueExA(hKey, "WaveMapper", 0, &type, (LPVOID)buffer, &size))
	ret |= MMDRV_Install("wavemapper", buffer, TRUE);
    size = sizeof(buffer);
    if (!RegQueryValueExA(hKey, "MidiMapper", 0, &type, (LPVOID)buffer, &size))
	ret |= MMDRV_Install("midimapper", buffer, TRUE);

    RegCloseKey(hKey);

    return ret;
}

/**************************************************************************
 * 				MMDRV_InitHardcoded		[internal]
 */
static BOOL	MMDRV_InitHardcoded(void)
{
    TRACE("()\n");
    /* first load hardware drivers */
#ifndef __REACTOS__
    MMDRV_Install("wineoss.drv",   	"wineoss.drv",	FALSE);
#endif /* __REACTOS__ */

#ifdef __REACTOS__
    // AG: TESTING:
    MMDRV_Install("mmdrv.dll", "mmdrv.dll", FALSE);
#endif

    /* finish with mappers */
    MMDRV_Install("wavemapper",     "msacm32.dll",    TRUE);
    MMDRV_Install("midimapper",     "midimap.dll",  TRUE);

    return TRUE;
}

/**************************************************************************
 * 				MMDRV_Init			[internal]
 */
BOOL	MMDRV_Init(void)
{
    TRACE("()\n");
    /* FIXME: MMDRV_InitFromRegistry shall be MMDRV_Init in a near future */
    return MMDRV_InitFromRegistry() || MMDRV_InitHardcoded();
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

    if (lpDrv->bIs32 && part->u.fnMessage32) {
#if 0
	ret = part->u.fnMessage32(0, DRVM_DISABLE, 0L, 0L, 0L);
	TRACE("DRVM_DISABLE => %08lx\n", ret);
#endif
	ret = part->u.fnMessage32(0, DRVM_EXIT, 0L, 0L, 0L);
	TRACE("DRVM_EXIT => %s\n", WINMM_ErrorToString(ret));
    } else if (!lpDrv->bIs32 && part->u.fnMessage16 && pFnCallMMDrvFunc16) {
#if 0
	ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16,
                                 0, DRVM_DISABLE, 0L, 0L, 0L);
	TRACE("DRVM_DISABLE => %08lx\n", ret);
#endif
        ret = pFnCallMMDrvFunc16((DWORD)part->u.fnMessage16,
                                 0, DRVM_EXIT, 0L, 0L, 0L);
	TRACE("DRVM_EXIT => %s\n", WINMM_ErrorToString(ret));
    } else {
	return FALSE;
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
    int i;
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
    for (i = sizeof(MMDrvs) / sizeof(MMDrvs[0]) - 1; i >= 0; i--)
    {
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_AUX);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIXER);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIDIIN);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_MIDIOUT);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_WAVEIN);
        MMDRV_ExitPerType(&MMDrvs[i], MMDRV_WAVEOUT);
        CloseDriver(MMDrvs[i].hDriver, 0, 0);
    }
}
