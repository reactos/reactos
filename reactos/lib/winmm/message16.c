/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM MCI and low level mapping functions
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
#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winver.h"
#include "wownt32.h"
#include "winemm16.h"
#include "digitalv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

/**************************************************************************
 * 				MMDRV_Callback			[internal]
 */
static  void	MMDRV_Callback(LPWINE_MLD mld, HDRVR hDev, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("CB (*%08lx)(%p %08x %08lx %08lx %08lx\n",
	  mld->dwCallback, hDev, uMsg, mld->dwClientInstance, dwParam1, dwParam2);

    if (!mld->bFrom32 && (mld->dwFlags & DCB_TYPEMASK) == DCB_FUNCTION)
    {
        WORD args[8];
	/* 16 bit func, call it */
	TRACE("Function (16 bit) !\n");

        args[7] = HDRVR_16(hDev);
        args[6] = uMsg;
        args[5] = HIWORD(mld->dwClientInstance);
        args[4] = LOWORD(mld->dwClientInstance);
        args[3] = HIWORD(dwParam1);
        args[2] = LOWORD(dwParam1);
        args[1] = HIWORD(dwParam2);
        args[0] = LOWORD(dwParam2);
        WOWCallback16Ex( mld->dwCallback, WCB16_PASCAL, sizeof(args), args, NULL );
    } else {
	DriverCallback(mld->dwCallback, mld->dwFlags, hDev, uMsg,
		       mld->dwClientInstance, dwParam1, dwParam2);
    }
}

/* =================================
 *       A U X    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_Aux_Map16To32A		[internal]
 */
static  WINMM_MapType	MMDRV_Aux_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_UnMap16To32A		[internal]
 */
static  WINMM_MapType	MMDRV_Aux_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_Map32ATo16		[internal]
 */
static  WINMM_MapType	MMDRV_Aux_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_UnMap32ATo16		[internal]
 */
static  WINMM_MapType	MMDRV_Aux_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
#if 0
 case AUXDM_GETDEVCAPS:
    lpCaps->wMid = ac16.wMid;
    lpCaps->wPid = ac16.wPid;
    lpCaps->vDriverVersion = ac16.vDriverVersion;
    strcpy(lpCaps->szPname, ac16.szPname);
    lpCaps->wTechnology = ac16.wTechnology;
    lpCaps->dwSupport = ac16.dwSupport;
#endif
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Aux_Callback		[internal]
 */
static  void	CALLBACK MMDRV_Aux_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    FIXME("NIY\n");
    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *     M I X E R  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				xMMDRV_Mixer_Map16To32A		[internal]
 */
static  WINMM_MapType	MMDRV_Mixer_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_UnMap16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_Mixer_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
#if 0
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));

    if (ret == MMSYSERR_NOERROR) {
	mixcaps->wMid           = micA.wMid;
	mixcaps->wPid           = micA.wPid;
	mixcaps->vDriverVersion = micA.vDriverVersion;
	strcpy(mixcaps->szPname, micA.szPname);
	mixcaps->fdwSupport     = micA.fdwSupport;
	mixcaps->cDestinations  = micA.cDestinations;
    }
    return ret;
#endif
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_Map32ATo16		[internal]
 */
static  WINMM_MapType	MMDRV_Mixer_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_UnMap32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_Mixer_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_Mixer_Callback		[internal]
 */
static  void	CALLBACK MMDRV_Mixer_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    FIXME("NIY\n");
    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   M I D I  I N    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_MidiIn_Map16To32A		[internal]
 */
static  WINMM_MapType	MMDRV_MidiIn_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_UnMap16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_MidiIn_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_Map32ATo16		[internal]
 */
static  WINMM_MapType	MMDRV_MidiIn_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_UnMap32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_MidiIn_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 				MMDRV_MidiIn_Callback		[internal]
 */
static  void	CALLBACK MMDRV_MidiIn_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case MIM_OPEN:
    case MIM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */

    case MIM_DATA:
    case MIM_MOREDATA:
    case MIM_ERROR:
	/* dwParam1 & dwParam2 are are data, nothing to do */
	break;
    case MIM_LONGDATA:
    case MIM_LONGERROR:
	/* dwParam1 points to a MidiHdr, work to be done !!! */
	if (mld->bFrom32 && !MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 32 => 16 */
	    LPMIDIHDR		mh16 = MapSL(dwParam1);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)((LPSTR)mh16 - sizeof(LPMIDIHDR));

	    dwParam1 = (DWORD)mh32;
	    mh32->dwFlags = mh16->dwFlags;
	    mh32->dwBytesRecorded = mh16->dwBytesRecorded;
	    if (mh32->reserved >= sizeof(MIDIHDR))
		mh32->dwOffset = mh16->dwOffset;
	} else if (!mld->bFrom32 && MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 16 => 32 */
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(dwParam1);
	    SEGPTR		segmh16 = *(SEGPTR*)((LPSTR)mh32 - sizeof(LPMIDIHDR));
	    LPMIDIHDR		mh16 = MapSL(segmh16);

	    dwParam1 = (DWORD)segmh16;
	    mh16->dwFlags = mh32->dwFlags;
	    mh16->dwBytesRecorded = mh32->dwBytesRecorded;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;
	}
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    /* case MOM_POSITIONCB: */
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   M I D I  O U T  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_MidiOut_Map16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_MidiOut_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;

    case MODM_OPEN:
    case MODM_CLOSE:
    case MODM_GETVOLUME:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA	moc32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMIDIOUTCAPS16) + sizeof(MIDIOUTCAPSA));
	    LPMIDIOUTCAPS16	moc16 = MapSL(*lpParam1);

	    if (moc32) {
		*(LPMIDIOUTCAPS16*)moc32 = moc16;
		moc32 = (LPMIDIOUTCAPSA)((LPSTR)moc32 + sizeof(LPMIDIOUTCAPS16));
		*lpParam1 = (DWORD)moc32;
		*lpParam2 = sizeof(MIDIOUTCAPSA);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case MODM_PREPARE:
	{
	    LPMIDIHDR		mh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMIDIHDR) + sizeof(MIDIHDR));
	    LPMIDIHDR		mh16 = MapSL(*lpParam1);

	    if (mh32) {
		*(LPMIDIHDR*)mh32 = (LPMIDIHDR)*lpParam1;
		mh32 = (LPMIDIHDR)((LPSTR)mh32 + sizeof(LPMIDIHDR));
		mh32->lpData = MapSL((SEGPTR)mh16->lpData);
		mh32->dwBufferLength = mh16->dwBufferLength;
		mh32->dwBytesRecorded = mh16->dwBytesRecorded;
		mh32->dwUser = mh16->dwUser;
		mh32->dwFlags = mh16->dwFlags;
		/* FIXME: nothing on mh32->lpNext */
		/* could link the mh32->lpNext at this level for memory house keeping */
		mh32->dwOffset = (*lpParam2 >= sizeof(MIDIHDR)) ? ((LPMIDIHDR)mh16)->dwOffset : 0;
		mh16->lpNext = mh32; /* for reuse in unprepare and write */
		/* store size of passed MIDIHDR?? structure to know if dwOffset is available or not */
		mh16->reserved = *lpParam2;
		*lpParam1 = (DWORD)mh32;
		*lpParam2 = sizeof(MIDIHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh16 = MapSL(*lpParam1);
	    LPMIDIHDR		mh32 = (LPMIDIHDR)mh16->lpNext;

	    *lpParam1 = (DWORD)mh32;
	    *lpParam2 = sizeof(MIDIHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == MODM_LONGDATA && mh32->dwBufferLength < mh16->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    mh32->dwBufferLength, mh16->dwBufferLength);
	    } else
                mh32->dwBufferLength = mh16->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;

    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_UnMap16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_MidiOut_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;

    case MODM_OPEN:
    case MODM_CLOSE:
    case MODM_GETVOLUME:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA		moc32 = (LPMIDIOUTCAPSA)(*lpParam1);
	    LPMIDIOUTCAPS16		moc16 = *(LPMIDIOUTCAPS16*)((LPSTR)moc32 - sizeof(LPMIDIOUTCAPS16));

	    moc16->wMid			= moc32->wMid;
	    moc16->wPid			= moc32->wPid;
	    moc16->vDriverVersion	= moc32->vDriverVersion;
	    strcpy(moc16->szPname, moc32->szPname);
	    moc16->wTechnology		= moc32->wTechnology;
	    moc16->wVoices		= moc32->wVoices;
	    moc16->wNotes		= moc32->wNotes;
	    moc16->wChannelMask		= moc32->wChannelMask;
	    moc16->dwSupport		= moc32->dwSupport;
	    HeapFree(GetProcessHeap(), 0, (LPSTR)moc32 - sizeof(LPMIDIOUTCAPS16));
	    ret = WINMM_MAP_OK;
	}
	break;
    case MODM_PREPARE:
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(*lpParam1);
	    LPMIDIHDR		mh16 = MapSL(*(SEGPTR*)((LPSTR)mh32 - sizeof(LPMIDIHDR)));

	    assert(mh16->lpNext == mh32);
	    mh16->dwBufferLength = mh32->dwBufferLength;
	    mh16->dwBytesRecorded = mh32->dwBytesRecorded;
	    mh16->dwUser = mh32->dwUser;
	    mh16->dwFlags = mh32->dwFlags;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;

	    if (wMsg == MODM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)mh32 - sizeof(LPMIDIHDR));
		mh16->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;

    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_Map32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_MidiOut_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_CLOSE:
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;
    case MODM_GETDEVCAPS:
	{
            LPMIDIOUTCAPSA moc32 = (LPMIDIOUTCAPSA)*lpParam1;
            LPSTR ptr = HeapAlloc( GetProcessHeap(), 0, sizeof(LPMIDIOUTCAPSA)+sizeof(MIDIOUTCAPS16));

	    if (ptr) {
		*(LPMIDIOUTCAPSA*)ptr = moc32;
		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	    *lpParam1 = (DWORD)MapLS(ptr) + sizeof(LPMIDIOUTCAPSA);
	    *lpParam2 = sizeof(MIDIOUTCAPS16);
	}
	break;
    case MODM_PREPARE:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)*lpParam1;
	    LPMIDIHDR		mh16;
	    LPVOID ptr = HeapAlloc( GetProcessHeap(), 0,
                                    sizeof(LPMIDIHDR) + sizeof(MIDIHDR) + mh32->dwBufferLength);

	    if (ptr) {
		*(LPMIDIHDR*)ptr = mh32;
		mh16 = (LPMIDIHDR)((LPSTR)ptr + sizeof(LPMIDIHDR));
		*lpParam1 = MapLS(mh16);
		mh16->lpData = (LPSTR)*lpParam1 + sizeof(MIDIHDR);
		/* data will be copied on WODM_WRITE */
		mh16->dwBufferLength = mh32->dwBufferLength;
		mh16->dwBytesRecorded = mh32->dwBytesRecorded;
		mh16->dwUser = mh32->dwUser;
		mh16->dwFlags = mh32->dwFlags;
		/* FIXME: nothing on mh32->lpNext */
		/* could link the mh32->lpNext at this level for memory house keeping */
		mh16->dwOffset = (*lpParam2 >= sizeof(MIDIHDR)) ? mh32->dwOffset : 0;

		mh32->lpNext = (LPMIDIHDR)mh16; /* for reuse in unprepare and write */
		mh32->reserved = *lpParam2;

		TRACE("mh16=%08lx mh16->lpData=%08lx mh32->buflen=%lu mh32->lpData=%08lx\n",
		      *lpParam1, (DWORD)mh16->lpData,
		      mh32->dwBufferLength, (DWORD)mh32->lpData);
		*lpParam2 = sizeof(MIDIHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(*lpParam1);
	    LPMIDIHDR		mh16 = (LPMIDIHDR)mh32->lpNext;
	    LPSTR		ptr = (LPSTR)mh16 - sizeof(LPMIDIHDR);

	    assert(*(LPMIDIHDR*)ptr == mh32);

	    if (wMsg == MODM_LONGDATA)
		memcpy((LPSTR)mh16 + sizeof(MIDIHDR), mh32->lpData, mh32->dwBufferLength);

	    *lpParam1 = MapLS(mh16);
	    *lpParam2 = sizeof(MIDIHDR);
	    TRACE("mh16=%08lx mh16->lpData=%08lx mh32->buflen=%lu mh32->lpData=%08lx\n",
                  *lpParam1, (DWORD)mh16->lpData, mh32->dwBufferLength, (DWORD)mh32->lpData);

	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == MODM_LONGDATA && mh16->dwBufferLength < mh32->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    mh16->dwBufferLength, mh32->dwBufferLength);
	    } else
                mh16->dwBufferLength = mh32->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;
    case MODM_OPEN:
	{
            LPMIDIOPENDESC		mod32 = (LPMIDIOPENDESC)*lpParam1;
	    LPVOID			ptr;
	    LPMIDIOPENDESC16		mod16;

	    /* allocated data are mapped as follows:
	       LPMIDIOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       MIDIOPENDESC16	mod16: openDesc passed to driver
	       MIDIOPENSTRMID	cIds
	    */
            ptr = HeapAlloc( GetProcessHeap(), 0,
                             sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD) + sizeof(MIDIOPENDESC16) +
                             mod32->cIds ? (mod32->cIds - 1) * sizeof(MIDIOPENSTRMID) : 0);

	    if (ptr) {
                SEGPTR segptr = MapLS(ptr);
		*(LPMIDIOPENDESC*)ptr = mod32;
		*(LPDWORD)((char*)ptr + sizeof(LPMIDIOPENDESC)) = *lpdwUser;
		mod16 = (LPMIDIOPENDESC16)((LPSTR)ptr + sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD));

		mod16->hMidi = HMIDI_16(mod32->hMidi);
		mod16->dwCallback = mod32->dwCallback;
		mod16->dwInstance = mod32->dwInstance;
		mod16->dnDevNode = mod32->dnDevNode;
		mod16->cIds = mod32->cIds;
		memcpy(&mod16->rgIds, &mod32->rgIds, mod32->cIds * sizeof(MIDIOPENSTRMID));

		*lpParam1 = (DWORD)segptr + sizeof(LPMIDIOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = (DWORD)segptr + sizeof(LPMIDIOPENDESC) + sizeof(DWORD);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case MODM_GETVOLUME:
    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_UnMap32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_MidiOut_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case MODM_CLOSE:
    case MODM_GETNUMDEVS:
    case MODM_DATA:
    case MODM_RESET:
    case MODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;
    case MODM_GETDEVCAPS:
	{
	    LPMIDIOUTCAPS16		moc16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)moc16 - sizeof(LPMIDIOUTCAPSA);
            LPMIDIOUTCAPSA		moc32 = *(LPMIDIOUTCAPSA*)ptr;

	    moc32->wMid			= moc16->wMid;
	    moc32->wPid			= moc16->wPid;
	    moc32->vDriverVersion	= moc16->vDriverVersion;
	    strcpy(moc32->szPname, moc16->szPname);
	    moc32->wTechnology		= moc16->wTechnology;
	    moc32->wVoices		= moc16->wVoices;
	    moc32->wNotes		= moc16->wNotes;
	    moc32->wChannelMask		= moc16->wChannelMask;
	    moc32->dwSupport		= moc16->dwSupport;
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case MODM_PREPARE:
    case MODM_UNPREPARE:
    case MODM_LONGDATA:
	{
	    LPMIDIHDR		mh16 = MapSL(*lpParam1);
	    LPSTR		ptr = (LPSTR)mh16 - sizeof(LPMIDIHDR);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)ptr;

	    assert(mh32->lpNext == (LPMIDIHDR)mh16);
            UnMapLS( *lpParam1 );
	    mh32->dwBytesRecorded = mh16->dwBytesRecorded;
	    mh32->dwUser = mh16->dwUser;
	    mh32->dwFlags = mh16->dwFlags;

	    if (wMsg == MODM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
                HeapFree( GetProcessHeap(), 0, ptr );
		mh32->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;
    case MODM_OPEN:
	{
	    LPMIDIOPENDESC16		mod16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)mod16 - sizeof(LPMIDIOPENDESC) - 2*sizeof(DWORD);
            UnMapLS( *lpParam1 );
	    **(DWORD**)(ptr + sizeof(LPMIDIOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPMIDIOPENDESC) + sizeof(DWORD));

            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case MODM_GETVOLUME:
    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_MidiOut_Callback		[internal]
 */
static  void	CALLBACK MMDRV_MidiOut_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case MOM_OPEN:
    case MOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case MOM_DONE:
	if (mld->bFrom32 && !MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 32 => 16 */
	    LPMIDIHDR		mh16 = MapSL(dwParam1);
	    LPMIDIHDR		mh32 = *(LPMIDIHDR*)((LPSTR)mh16 - sizeof(LPMIDIHDR));

	    dwParam1 = (DWORD)mh32;
	    mh32->dwFlags = mh16->dwFlags;
	    mh32->dwOffset = mh16->dwOffset;
	    if (mh32->reserved >= sizeof(MIDIHDR))
		mh32->dwOffset = mh16->dwOffset;
	} else if (!mld->bFrom32 && MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 16 => 32 */
	    LPMIDIHDR		mh32 = (LPMIDIHDR)(dwParam1);
	    SEGPTR		segmh16 = *(SEGPTR*)((LPSTR)mh32 - sizeof(LPMIDIHDR));
	    LPMIDIHDR		mh16 = MapSL(segmh16);

	    dwParam1 = (DWORD)segmh16;
	    mh16->dwFlags = mh32->dwFlags;
	    if (mh16->reserved >= sizeof(MIDIHDR))
		mh16->dwOffset = mh32->dwOffset;
	}
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    /* case MOM_POSITIONCB: */
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   W A V E  I N    M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_WaveIn_Map16To32A		[internal]
 */
static  WINMM_MapType	MMDRV_WaveIn_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = WINMM_MAP_OK;
	break;
    case WIDM_OPEN:
    case WIDM_CLOSE:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;
    case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA	wic32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEINCAPS16) + sizeof(WAVEINCAPSA));
	    LPWAVEINCAPS16	wic16 = MapSL(*lpParam1);

	    if (wic32) {
		*(LPWAVEINCAPS16*)wic32 = wic16;
		wic32 = (LPWAVEINCAPSA)((LPSTR)wic32 + sizeof(LPWAVEINCAPS16));
		*lpParam1 = (DWORD)wic32;
		*lpParam2 = sizeof(WAVEINCAPSA);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_GETPOS:
	{
            LPMMTIME		mmt32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMMTIME16) + sizeof(MMTIME));
	    LPMMTIME16		mmt16 = MapSL(*lpParam1);

	    if (mmt32) {
		*(LPMMTIME16*)mmt32 = mmt16;
		mmt32 = (LPMMTIME)((LPSTR)mmt32 + sizeof(LPMMTIME16));

		mmt32->wType = mmt16->wType;
		*lpParam1 = (DWORD)mmt32;
		*lpParam2 = sizeof(MMTIME);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_PREPARE:
	{
	    LPWAVEHDR		wh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEHDR) + sizeof(WAVEHDR));
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);

	    if (wh32) {
		*(LPWAVEHDR*)wh32 = (LPWAVEHDR)*lpParam1;
		wh32 = (LPWAVEHDR)((LPSTR)wh32 + sizeof(LPWAVEHDR));
		wh32->lpData = MapSL((SEGPTR)wh16->lpData);
		wh32->dwBufferLength = wh16->dwBufferLength;
		wh32->dwBytesRecorded = wh16->dwBytesRecorded;
		wh32->dwUser = wh16->dwUser;
		wh32->dwFlags = wh16->dwFlags;
		wh32->dwLoops = wh16->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh16->lpNext = wh32; /* for reuse in unprepare and write */
		*lpParam1 = (DWORD)wh32;
		*lpParam2 = sizeof(WAVEHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);
	    LPWAVEHDR		wh32 = (LPWAVEHDR)wh16->lpNext;

	    *lpParam1 = (DWORD)wh32;
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == WIDM_ADDBUFFER && wh32->dwBufferLength < wh16->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    wh32->dwBufferLength, wh16->dwBufferLength);
	    } else
                wh32->dwBufferLength = wh16->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;
    case WIDM_MAPPER_STATUS:
	/* just a single DWORD */
	*lpParam2 = (DWORD)MapSL(*lpParam2);
	ret = WINMM_MAP_OK;
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_UnMap16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_WaveIn_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
    case WIDM_MAPPER_STATUS:
	ret = WINMM_MAP_OK;
	break;
    case WIDM_OPEN:
    case WIDM_CLOSE:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;
    case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA		wic32 = (LPWAVEINCAPSA)(*lpParam1);
	    LPWAVEINCAPS16		wic16 = *(LPWAVEINCAPS16*)((LPSTR)wic32 - sizeof(LPWAVEINCAPS16));

	    wic16->wMid = wic32->wMid;
	    wic16->wPid = wic32->wPid;
	    wic16->vDriverVersion = wic32->vDriverVersion;
	    strcpy(wic16->szPname, wic32->szPname);
	    wic16->dwFormats = wic32->dwFormats;
	    wic16->wChannels = wic32->wChannels;
	    HeapFree(GetProcessHeap(), 0, (LPSTR)wic32 - sizeof(LPWAVEINCAPS16));
	    ret = WINMM_MAP_OK;
	}
	break;
    case WIDM_GETPOS:
	{
            LPMMTIME		mmt32 = (LPMMTIME)(*lpParam1);
	    LPMMTIME16		mmt16 = *(LPMMTIME16*)((LPSTR)mmt32 - sizeof(LPMMTIME16));

	    MMSYSTEM_MMTIME32to16(mmt16, mmt32);
	    HeapFree(GetProcessHeap(), 0, (LPSTR)mmt32 - sizeof(LPMMTIME16));
	    ret = WINMM_MAP_OK;
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_PREPARE:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = MapSL(*(SEGPTR*)((LPSTR)wh32 - sizeof(LPWAVEHDR)));

	    assert(wh16->lpNext == wh32);
	    wh16->dwBufferLength = wh32->dwBufferLength;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	    wh16->dwUser = wh32->dwUser;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwLoops = wh32->dwLoops;

	    if (wMsg == WIDM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)wh32 - sizeof(LPWAVEHDR));
		wh16->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_Map32ATo16		[internal]
 */
static  WINMM_MapType	MMDRV_WaveIn_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_CLOSE:
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = WINMM_MAP_OK;
	break;

    case WIDM_OPEN:
	{
            LPWAVEOPENDESC		wod32 = (LPWAVEOPENDESC)*lpParam1;
	    int				sz = sizeof(WAVEFORMATEX);
	    LPVOID			ptr;
	    LPWAVEOPENDESC16		wod16;

	    /* allocated data are mapped as follows:
	       LPWAVEOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       WAVEOPENDESC16	wod16: openDesc passed to driver
	       WAVEFORMATEX	openDesc->lpFormat passed to driver
	       xxx		extra bytes to WAVEFORMATEX
	    */
	    if (wod32->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		TRACE("Allocating %u extra bytes (%d)\n", ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize, wod32->lpFormat->wFormatTag);
		sz += ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize;
	    }

            ptr = HeapAlloc( GetProcessHeap(), 0,
                             sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16) + sz);

	    if (ptr) {
                SEGPTR seg_ptr = MapLS( ptr );
		*(LPWAVEOPENDESC*)ptr = wod32;
		*(LPDWORD)((char*)ptr + sizeof(LPWAVEOPENDESC)) = *lpdwUser;
		wod16 = (LPWAVEOPENDESC16)((LPSTR)ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD));

		wod16->hWave = HWAVE_16(wod32->hWave);
		wod16->lpFormat = (LPWAVEFORMATEX)(seg_ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16));
		memcpy(wod16 + 1, wod32->lpFormat, sz);

		wod16->dwCallback = wod32->dwCallback;
		wod16->dwInstance = wod32->dwInstance;
		wod16->uMappedDeviceID = wod32->uMappedDeviceID;
		wod16->dnDevNode = wod32->dnDevNode;

		*lpParam1 = seg_ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = seg_ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_PREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)*lpParam1;
	    LPWAVEHDR		wh16;
	    LPVOID ptr = HeapAlloc( GetProcessHeap(), 0,
                                    sizeof(LPWAVEHDR) + sizeof(WAVEHDR) + wh32->dwBufferLength);

	    if (ptr) {
                SEGPTR seg_ptr = MapLS( ptr );
		*(LPWAVEHDR*)ptr = wh32;
		wh16 = (LPWAVEHDR)((LPSTR)ptr + sizeof(LPWAVEHDR));
		wh16->lpData = (LPSTR)seg_ptr + sizeof(LPWAVEHDR) + sizeof(WAVEHDR);
		/* data will be copied on WODM_WRITE */
		wh16->dwBufferLength = wh32->dwBufferLength;
		wh16->dwBytesRecorded = wh32->dwBytesRecorded;
		wh16->dwUser = wh32->dwUser;
		wh16->dwFlags = wh32->dwFlags;
		wh16->dwLoops = wh32->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh32->lpNext = wh16; /* for reuse in unprepare and write */
		TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n",
		      seg_ptr + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		      wh32->dwBufferLength, (DWORD)wh32->lpData);
		*lpParam1 = seg_ptr + sizeof(LPWAVEHDR);
		*lpParam2 = sizeof(WAVEHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WIDM_ADDBUFFER:
    case WIDM_UNPREPARE:
 	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = wh32->lpNext;
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
            SEGPTR seg_ptr = MapLS( ptr );

	    assert(*(LPWAVEHDR*)ptr == wh32);

	    TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n",
		  seg_ptr + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		  wh32->dwBufferLength, (DWORD)wh32->lpData);

	    if (wMsg == WIDM_ADDBUFFER)
		memcpy((LPSTR)wh16 + sizeof(WAVEHDR), wh32->lpData, wh32->dwBufferLength);

	    *lpParam1 = seg_ptr + sizeof(LPWAVEHDR);
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == WIDM_ADDBUFFER && wh16->dwBufferLength < wh32->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    wh16->dwBufferLength, wh32->dwBufferLength);
	    } else
                wh16->dwBufferLength = wh32->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;
   case WIDM_GETDEVCAPS:
	{
            LPWAVEINCAPSA wic32 = (LPWAVEINCAPSA)*lpParam1;
            LPSTR ptr = HeapAlloc( GetProcessHeap(), 0 ,sizeof(LPWAVEINCAPSA) + sizeof(WAVEINCAPS16));

	    if (ptr) {
		*(LPWAVEINCAPSA*)ptr = wic32;
		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	    *lpParam1 = MapLS(ptr) + sizeof(LPWAVEINCAPSA);
	    *lpParam2 = sizeof(WAVEINCAPS16);
	}
	break;
    case WIDM_GETPOS:
 	{
            LPMMTIME mmt32 = (LPMMTIME)*lpParam1;
            LPSTR ptr = HeapAlloc( GetProcessHeap(), 0, sizeof(LPMMTIME) + sizeof(MMTIME16));
            LPMMTIME16 mmt16 = (LPMMTIME16)(ptr + sizeof(LPMMTIME));

	    if (ptr) {
		*(LPMMTIME*)ptr = mmt32;
		mmt16->wType = mmt32->wType;
		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	    *lpParam1 = MapLS(ptr) + sizeof(LPMMTIME);
	    *lpParam2 = sizeof(MMTIME16);
	}
	break;
    case DRVM_MAPPER_STATUS:
 	{
            LPDWORD p32 = (LPDWORD)*lpParam2;
            *lpParam2 = MapLS(p32);
            ret = WINMM_MAP_OKMEM;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_UnMap32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_WaveIn_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case WIDM_CLOSE:
    case WIDM_GETNUMDEVS:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_STOP:
	ret = WINMM_MAP_OK;
	break;

    case WIDM_OPEN:
	{
	    LPWAVEOPENDESC16		wod16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wod16 - sizeof(LPWAVEOPENDESC) - 2*sizeof(DWORD);
            LPWAVEOPENDESC		wod32 = *(LPWAVEOPENDESC*)ptr;

            UnMapLS( *lpParam1 );
	    wod32->uMappedDeviceID = wod16->uMappedDeviceID;
	    **(DWORD**)(ptr + sizeof(LPWAVEOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD));
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;

    case WIDM_ADDBUFFER:
    case WIDM_PREPARE:
    case WIDM_UNPREPARE:
	{
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)ptr;

	    assert(wh32->lpNext == wh16);
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	    wh32->dwUser = wh16->dwUser;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwLoops = wh16->dwLoops;
            UnMapLS( *lpParam1 );

	    if (wMsg == WIDM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
                HeapFree( GetProcessHeap(), 0, ptr );
		wh32->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;
     case WIDM_GETDEVCAPS:
	{
	    LPWAVEINCAPS16		wic16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wic16 - sizeof(LPWAVEINCAPSA);
            LPWAVEINCAPSA		wic32 = *(LPWAVEINCAPSA*)ptr;

	    wic32->wMid = wic16->wMid;
	    wic32->wPid = wic16->wPid;
	    wic32->vDriverVersion = wic16->vDriverVersion;
	    strcpy(wic32->szPname, wic16->szPname);
	    wic32->dwFormats = wic16->dwFormats;
	    wic32->wChannels = wic16->wChannels;
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case WIDM_GETPOS:
	{
	    LPMMTIME16		mmt16 = MapSL(*lpParam1);
	    LPSTR		ptr   = (LPSTR)mmt16 - sizeof(LPMMTIME);
            LPMMTIME		mmt32 = *(LPMMTIME*)ptr;

	    MMSYSTEM_MMTIME16to32(mmt32, mmt16);
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case DRVM_MAPPER_STATUS:
	{
            UnMapLS( *lpParam2 );
	    ret = WINMM_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveIn_Callback		[internal]
 */
static  void	CALLBACK MMDRV_WaveIn_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WIM_DATA:
	if (mld->bFrom32 && !MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 32 => 16 */
	    LPWAVEHDR		wh16 = MapSL(dwParam1);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)((LPSTR)wh16 - sizeof(LPWAVEHDR));

	    dwParam1 = (DWORD)wh32;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	} else if (!mld->bFrom32 && MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 16 => 32 */
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(dwParam1);
	    SEGPTR		segwh16 = *(SEGPTR*)((LPSTR)wh32 - sizeof(LPWAVEHDR));
	    LPWAVEHDR		wh16 = MapSL(segwh16);

	    dwParam1 = (DWORD)segwh16;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	}
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *   W A V E  O U T  M A P P E R S
 * ================================= */

/**************************************************************************
 * 				MMDRV_WaveOut_Map16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_WaveOut_Map16To32A  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    /* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;

    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
    case WODM_GETVOLUME:
    case WODM_OPEN:
	FIXME("Shouldn't be used: the corresponding 16 bit functions use the 32 bit interface\n");
	break;

    case WODM_GETDEVCAPS:
	{
            LPWAVEOUTCAPSA		woc32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEOUTCAPS16) + sizeof(WAVEOUTCAPSA));
	    LPWAVEOUTCAPS16		woc16 = MapSL(*lpParam1);

	    if (woc32) {
		*(LPWAVEOUTCAPS16*)woc32 = woc16;
		woc32 = (LPWAVEOUTCAPSA)((LPSTR)woc32 + sizeof(LPWAVEOUTCAPS16));
		*lpParam1 = (DWORD)woc32;
		*lpParam2 = sizeof(WAVEOUTCAPSA);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WODM_GETPOS:
	{
            LPMMTIME		mmt32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMMTIME16) + sizeof(MMTIME));
	    LPMMTIME16		mmt16 = MapSL(*lpParam1);

	    if (mmt32) {
		*(LPMMTIME16*)mmt32 = mmt16;
		mmt32 = (LPMMTIME)((LPSTR)mmt32 + sizeof(LPMMTIME16));

		mmt32->wType = mmt16->wType;
		*lpParam1 = (DWORD)mmt32;
		*lpParam2 = sizeof(MMTIME);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WODM_PREPARE:
	{
	    LPWAVEHDR		wh32 = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWAVEHDR) + sizeof(WAVEHDR));
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);

	    if (wh32) {
		*(LPWAVEHDR*)wh32 = (LPWAVEHDR)*lpParam1;
		wh32 = (LPWAVEHDR)((LPSTR)wh32 + sizeof(LPWAVEHDR));
		wh32->lpData = MapSL((SEGPTR)wh16->lpData);
		wh32->dwBufferLength = wh16->dwBufferLength;
		wh32->dwBytesRecorded = wh16->dwBytesRecorded;
		wh32->dwUser = wh16->dwUser;
		wh32->dwFlags = wh16->dwFlags;
		wh32->dwLoops = wh16->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh16->lpNext = wh32; /* for reuse in unprepare and write */
		*lpParam1 = (DWORD)wh32;
		*lpParam2 = sizeof(WAVEHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);
	    LPWAVEHDR		wh32 = (LPWAVEHDR)wh16->lpNext;

	    *lpParam1 = (DWORD)wh32;
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == WODM_WRITE && wh32->dwBufferLength < wh16->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    wh32->dwBufferLength, wh16->dwBufferLength);
	    } else
                wh32->dwBufferLength = wh16->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;
    case WODM_MAPPER_STATUS:
	*lpParam2 = (DWORD)MapSL(*lpParam2);
	ret = WINMM_MAP_OK;
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_UnMap16To32A	[internal]
 */
static  WINMM_MapType	MMDRV_WaveOut_UnMap16To32A(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    /* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
    case WODM_MAPPER_STATUS:
	ret = WINMM_MAP_OK;
	break;

    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
    case WODM_GETVOLUME:
    case WODM_OPEN:
	FIXME("Shouldn't be used: those 16 bit functions use the 32 bit interface\n");
	break;

    case WODM_GETDEVCAPS:
	{
            LPWAVEOUTCAPSA		woc32 = (LPWAVEOUTCAPSA)(*lpParam1);
	    LPWAVEOUTCAPS16		woc16 = *(LPWAVEOUTCAPS16*)((LPSTR)woc32 - sizeof(LPWAVEOUTCAPS16));

	    woc16->wMid = woc32->wMid;
	    woc16->wPid = woc32->wPid;
	    woc16->vDriverVersion = woc32->vDriverVersion;
	    strcpy(woc16->szPname, woc32->szPname);
	    woc16->dwFormats = woc32->dwFormats;
	    woc16->wChannels = woc32->wChannels;
	    woc16->dwSupport = woc32->dwSupport;
	    HeapFree(GetProcessHeap(), 0, (LPSTR)woc32 - sizeof(LPWAVEOUTCAPS16));
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_GETPOS:
	{
            LPMMTIME		mmt32 = (LPMMTIME)(*lpParam1);
	    LPMMTIME16		mmt16 = *(LPMMTIME16*)((LPSTR)mmt32 - sizeof(LPMMTIME16));

	    MMSYSTEM_MMTIME32to16(mmt16, mmt32);
	    HeapFree(GetProcessHeap(), 0, (LPSTR)mmt32 - sizeof(LPMMTIME16));
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_PREPARE:
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = MapSL(*(SEGPTR*)((LPSTR)wh32 - sizeof(LPWAVEHDR)));

	    assert(wh16->lpNext == wh32);
	    wh16->dwBufferLength = wh32->dwBufferLength;
	    wh16->dwBytesRecorded = wh32->dwBytesRecorded;
	    wh16->dwUser = wh32->dwUser;
	    wh16->dwFlags = wh32->dwFlags;
	    wh16->dwLoops = wh32->dwLoops;

	    if (wMsg == WODM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
		HeapFree(GetProcessHeap(), 0, (LPSTR)wh32 - sizeof(LPWAVEHDR));
		wh16->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet for %u [%lx,%lx]\n", wMsg, *lpParam1, *lpParam2);
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_Map32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_WaveOut_Map32ATo16  (UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2)
{
    WINMM_MapType	ret;

    switch (wMsg) {
	/* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;

    case WODM_GETDEVCAPS:
	{
            LPWAVEOUTCAPSA woc32 = (LPWAVEOUTCAPSA)*lpParam1;
            LPSTR ptr = HeapAlloc( GetProcessHeap(), 0,
                                   sizeof(LPWAVEOUTCAPSA) + sizeof(WAVEOUTCAPS16));

	    if (ptr) {
		*(LPWAVEOUTCAPSA*)ptr = woc32;
		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	    *lpParam1 = MapLS(ptr) + sizeof(LPWAVEOUTCAPSA);
	    *lpParam2 = sizeof(WAVEOUTCAPS16);
	}
	break;
    case WODM_GETPITCH:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case WODM_GETPLAYBACKRATE:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case WODM_GETPOS:
	{
            LPMMTIME mmt32 = (LPMMTIME)*lpParam1;
            LPSTR ptr = HeapAlloc( GetProcessHeap(), 0, sizeof(LPMMTIME) + sizeof(MMTIME16));
            LPMMTIME16 mmt16 = (LPMMTIME16)(ptr + sizeof(LPMMTIME));

	    if (ptr) {
		*(LPMMTIME*)ptr = mmt32;
		mmt16->wType = mmt32->wType;
		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	    *lpParam1 = MapLS(ptr) + sizeof(LPMMTIME);
	    *lpParam2 = sizeof(MMTIME16);
	}
	break;
    case WODM_GETVOLUME:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case WODM_OPEN:
	{
            LPWAVEOPENDESC		wod32 = (LPWAVEOPENDESC)*lpParam1;
	    int				sz = sizeof(WAVEFORMATEX);
	    LPVOID			ptr;
	    LPWAVEOPENDESC16		wod16;

	    /* allocated data are mapped as follows:
	       LPWAVEOPENDESC	ptr to orig lParam1
	       DWORD		orig dwUser, which is a pointer to DWORD:driver dwInstance
	       DWORD		dwUser passed to driver
	       WAVEOPENDESC16	wod16: openDesc passed to driver
	       WAVEFORMATEX	openDesc->lpFormat passed to driver
	       xxx		extra bytes to WAVEFORMATEX
	    */
	    if (wod32->lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
		TRACE("Allocating %u extra bytes (%d)\n", ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize, wod32->lpFormat->wFormatTag);
		sz += ((LPWAVEFORMATEX)wod32->lpFormat)->cbSize;
	    }

	    ptr = HeapAlloc( GetProcessHeap(), 0,
                             sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16) + sz);

	    if (ptr) {
                SEGPTR seg_ptr = MapLS( ptr );
		*(LPWAVEOPENDESC*)ptr = wod32;
		*(LPDWORD)((char*)ptr + sizeof(LPWAVEOPENDESC)) = *lpdwUser;
		wod16 = (LPWAVEOPENDESC16)((LPSTR)ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD));

		wod16->hWave = HWAVE_16(wod32->hWave);
		wod16->lpFormat = (LPWAVEFORMATEX)(seg_ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD) + sizeof(WAVEOPENDESC16));
		memcpy(wod16 + 1, wod32->lpFormat, sz);

		wod16->dwCallback = wod32->dwCallback;
		wod16->dwInstance = wod32->dwInstance;
		wod16->uMappedDeviceID = wod32->uMappedDeviceID;
		wod16->dnDevNode = wod32->dnDevNode;

		*lpParam1 = seg_ptr + sizeof(LPWAVEOPENDESC) + 2*sizeof(DWORD);
		*lpdwUser = seg_ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WODM_PREPARE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)*lpParam1;
	    LPWAVEHDR		wh16;
	    LPVOID ptr = HeapAlloc( GetProcessHeap(), 0,
                                    sizeof(LPWAVEHDR) + sizeof(WAVEHDR) + wh32->dwBufferLength);

	    if (ptr) {
                SEGPTR seg_ptr = MapLS( ptr );
		*(LPWAVEHDR*)ptr = wh32;
		wh16 = (LPWAVEHDR)((LPSTR)ptr + sizeof(LPWAVEHDR));
		wh16->lpData = (LPSTR)seg_ptr + sizeof(LPWAVEHDR) + sizeof(WAVEHDR);
		/* data will be copied on WODM_WRITE */
		wh16->dwBufferLength = wh32->dwBufferLength;
		wh16->dwBytesRecorded = wh32->dwBytesRecorded;
		wh16->dwUser = wh32->dwUser;
		wh16->dwFlags = wh32->dwFlags;
		wh16->dwLoops = wh32->dwLoops;
		/* FIXME: nothing on wh32->lpNext */
		/* could link the wh32->lpNext at this level for memory house keeping */
		wh32->lpNext = wh16; /* for reuse in unprepare and write */
		TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n",
		      seg_ptr + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		      wh32->dwBufferLength, (DWORD)wh32->lpData);
		*lpParam1 = seg_ptr + sizeof(LPWAVEHDR);
		*lpParam2 = sizeof(WAVEHDR);

		ret = WINMM_MAP_OKMEM;
	    } else {
		ret = WINMM_MAP_NOMEM;
	    }
	}
	break;
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(*lpParam1);
	    LPWAVEHDR		wh16 = wh32->lpNext;
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
            SEGPTR seg_ptr = MapLS( ptr );

	    assert(*(LPWAVEHDR*)ptr == wh32);

	    TRACE("wh16=%08lx wh16->lpData=%08lx wh32->buflen=%lu wh32->lpData=%08lx\n",
		  seg_ptr + sizeof(LPWAVEHDR), (DWORD)wh16->lpData,
		  wh32->dwBufferLength, (DWORD)wh32->lpData);

	    if (wMsg == WODM_WRITE)
		memcpy((LPSTR)wh16 + sizeof(WAVEHDR), wh32->lpData, wh32->dwBufferLength);

	    *lpParam1 = seg_ptr + sizeof(LPWAVEHDR);
	    *lpParam2 = sizeof(WAVEHDR);
	    /* dwBufferLength can be reduced between prepare & write */
	    if (wMsg == WODM_WRITE && wh16->dwBufferLength < wh32->dwBufferLength) {
		ERR("Size of buffer has been increased from %ld to %ld, keeping initial value\n",
		    wh16->dwBufferLength, wh32->dwBufferLength);
	    } else
                wh16->dwBufferLength = wh32->dwBufferLength;
	    ret = WINMM_MAP_OKMEM;
	}
	break;
    case DRVM_MAPPER_STATUS:
 	{
            LPDWORD p32 = (LPDWORD)*lpParam2;
            *lpParam2 = MapLS(p32);
            ret = WINMM_MAP_OKMEM;
	}
	break;
    default:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_UnMap32ATo16	[internal]
 */
static  WINMM_MapType	MMDRV_WaveOut_UnMap32ATo16(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT fn_ret)
{
    WINMM_MapType	ret;

    switch (wMsg) {
	/* nothing to do */
    case WODM_BREAKLOOP:
    case WODM_CLOSE:
    case WODM_GETNUMDEVS:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_SETVOLUME:
	ret = WINMM_MAP_OK;
	break;

    case WODM_GETDEVCAPS:
	{
	    LPWAVEOUTCAPS16		woc16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)woc16 - sizeof(LPWAVEOUTCAPSA);
            LPWAVEOUTCAPSA		woc32 = *(LPWAVEOUTCAPSA*)ptr;

	    woc32->wMid = woc16->wMid;
	    woc32->wPid = woc16->wPid;
	    woc32->vDriverVersion = woc16->vDriverVersion;
	    strcpy(woc32->szPname, woc16->szPname);
	    woc32->dwFormats = woc16->dwFormats;
	    woc32->wChannels = woc16->wChannels;
	    woc32->dwSupport = woc16->dwSupport;
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_GETPITCH:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case WODM_GETPLAYBACKRATE:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case WODM_GETPOS:
	{
	    LPMMTIME16		mmt16 = MapSL(*lpParam1);
	    LPSTR		ptr   = (LPSTR)mmt16 - sizeof(LPMMTIME);
            LPMMTIME		mmt32 = *(LPMMTIME*)ptr;

	    MMSYSTEM_MMTIME16to32(mmt32, mmt16);
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_OPEN:
	{
	    LPWAVEOPENDESC16		wod16 = MapSL(*lpParam1);
	    LPSTR			ptr   = (LPSTR)wod16 - sizeof(LPWAVEOPENDESC) - 2*sizeof(DWORD);
            LPWAVEOPENDESC		wod32 = *(LPWAVEOPENDESC*)ptr;

	    wod32->uMappedDeviceID = wod16->uMappedDeviceID;
	    **(DWORD**)(ptr + sizeof(LPWAVEOPENDESC)) = *(LPDWORD)(ptr + sizeof(LPWAVEOPENDESC) + sizeof(DWORD));
            UnMapLS( *lpParam1 );
            HeapFree( GetProcessHeap(), 0, ptr );
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_PREPARE:
    case WODM_UNPREPARE:
    case WODM_WRITE:
	{
	    LPWAVEHDR		wh16 = MapSL(*lpParam1);
	    LPSTR		ptr = (LPSTR)wh16 - sizeof(LPWAVEHDR);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)ptr;

	    assert(wh32->lpNext == wh16);
	    wh32->dwBytesRecorded = wh16->dwBytesRecorded;
	    wh32->dwUser = wh16->dwUser;
	    wh32->dwFlags = wh16->dwFlags;
	    wh32->dwLoops = wh16->dwLoops;

            UnMapLS( *lpParam1 );
	    if (wMsg == WODM_UNPREPARE && fn_ret == MMSYSERR_NOERROR) {
                HeapFree( GetProcessHeap(), 0, ptr );
		wh32->lpNext = 0;
	    }
	    ret = WINMM_MAP_OK;
	}
	break;
    case WODM_GETVOLUME:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    case DRVM_MAPPER_STATUS:
	{
            UnMapLS( *lpParam2 );
	    ret = WINMM_MAP_OK;
	}
	break;
    default:
	FIXME("NIY: no conversion yet\n");
	ret = WINMM_MAP_MSGERROR;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				MMDRV_WaveOut_Callback		[internal]
 */
static  void	CALLBACK MMDRV_WaveOut_Callback(HDRVR hDev, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD	mld = (LPWINE_MLD)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WOM_DONE:
	if (mld->bFrom32 && !MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 32 => 16 */
	    LPWAVEHDR		wh16 = MapSL(dwParam1);
	    LPWAVEHDR		wh32 = *(LPWAVEHDR*)((LPSTR)wh16 - sizeof(LPWAVEHDR));

	    dwParam1 = (DWORD)wh32;
	    wh32->dwFlags = wh16->dwFlags;
	} else if (!mld->bFrom32 && MMDRV_Is32(mld->mmdIndex)) {
	    /* initial map is: 16 => 32 */
	    LPWAVEHDR		wh32 = (LPWAVEHDR)(dwParam1);
	    SEGPTR		segwh16 = *(SEGPTR*)((LPSTR)wh32 - sizeof(LPWAVEHDR));
	    LPWAVEHDR		wh16 = MapSL(segwh16);

	    dwParam1 = (DWORD)segwh16;
	    wh16->dwFlags = wh32->dwFlags;
	}
	/* else { 16 => 16 or 32 => 32, nothing to do, same struct is kept }*/
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    MMDRV_Callback(mld, hDev, uMsg, dwParam1, dwParam2);
}

/* =================================
 *  M A P P E R S   H A N D L I N G
 * ================================= */

static  LRESULT    MMDRV_CallMMDrvFunc16(DWORD fp16, WORD dev, WORD msg, LONG instance,
                                         LONG lp1, LONG lp2)
{
    WORD args[8];
    DWORD ret;

    args[7] = dev;
    args[6] = msg;
    args[5] = HIWORD(instance);
    args[4] = LOWORD(instance);
    args[3] = HIWORD(lp1);
    args[2] = LOWORD(lp1);
    args[1] = HIWORD(lp2);
    args[0] = LOWORD(lp2);
    WOWCallback16Ex( fp16, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/**************************************************************************
 * 				MMDRV_GetDescription16		[internal]
 */
static  BOOL	MMDRV_GetDescription16(const char* fname, char* buf, int buflen)
{
    OFSTRUCT   	ofs;
    HFILE	hFile;
    WORD 	w;
    DWORD	dw;
    BOOL	ret = FALSE;

    if ((hFile = OpenFile(fname, &ofs, OF_READ | OF_SHARE_DENY_WRITE)) == HFILE_ERROR) {
	ERR("Can't open file %s (builtin driver ?)\n", fname);
	return FALSE;
    }

#define E(_x)	do {TRACE _x;goto theEnd;} while(0)

    if (_lread(hFile, &w, 2) != 2)			E(("Can't read sig\n"));
    if (w != ('Z' * 256 + 'M')) 			E(("Bad sig %04x\n", w));
    if (_llseek(hFile, 0x3C, SEEK_SET) < 0) 		E(("Can't seek to ext header offset\n"));
    if (_lread(hFile, &dw, 4) != 4)			E(("Can't read ext header offset\n"));
    if (_llseek(hFile, dw + 0x2C, SEEK_SET) < 0) 	E(("Can't seek to ext header.nr table %lu\n", dw+0x2C));
    if (_lread(hFile, &dw, 4) != 4)			E(("Can't read nr table offset\n"));
    if (_llseek(hFile, dw, SEEK_SET) < 0) 		E(("Can't seek to nr table %lu\n", dw));
    if (_lread(hFile, buf, 1) != 1)			E(("Can't read descr length\n"));
    buflen = min((int)(unsigned)(BYTE)buf[0], buflen - 1);
    if (_lread(hFile, buf, buflen) != buflen)		E(("Can't read descr (%d)\n", buflen));
    buf[buflen] = '\0';
    ret = TRUE;
    TRACE("Got '%s' [%d]\n", buf, buflen);
theEnd:
    _lclose(hFile);
    return ret;
}

/******************************************************************
 *		MMDRV_LoadMMDrvFunc16
 *
 */
unsigned   MMDRV_LoadMMDrvFunc16(LPCSTR drvName, LPWINE_DRIVER d, 
                                 LPWINE_MM_DRIVER lpDrv)
{        
    WINEMM_msgFunc16	func;
    unsigned            count = 0;
    char    		buffer[128];
    /*
     * DESCRIPTION 'wave,aux,mixer:Creative Labs Sound Blaster 16 Driver'
     * The beginning of the module description indicates the driver supports
     * waveform, auxiliary, and mixer devices. Use one of the following
     * device-type names, followed by a colon (:) to indicate the type of
     * device your driver supports. If the driver supports more than one
     * type of device, separate each device-type name with a comma (,).
     *
     * wave for waveform audio devices
     * wavemapper for wave mappers
     * midi for MIDI audio devices
     * midimapper for midi mappers
     * aux for auxiliary audio devices
     * mixer for mixer devices
     */

    if (d->d.d16.hDriver16) {
        HMODULE16	hMod16 = GetDriverModuleHandle16(d->d.d16.hDriver16);

#define	AA(_h,_w,_x,_y,_z)					\
    func = (WINEMM_msgFunc##_y) _z ((_h), #_x);			\
    if (func != NULL) 						\
        { lpDrv->parts[_w].u.fnMessage##_y = func; count++; 	\
          TRACE("Got %d bit func '%s'\n", _y, #_x);         }

#define A(_x,_y)	AA(hMod16,_x,_y,16,GetProcAddress16)
        A(MMDRV_AUX,	auxMessage);
        A(MMDRV_MIXER,	mxdMessage);
        A(MMDRV_MIDIIN,	midMessage);
        A(MMDRV_MIDIOUT,modMessage);
        A(MMDRV_WAVEIN,	widMessage);
        A(MMDRV_WAVEOUT,wodMessage);
#undef A
#undef AA
    }
    if (TRACE_ON(winmm)) {
        if (MMDRV_GetDescription16(drvName, buffer, sizeof(buffer)))
	    TRACE("%s => %s\n", drvName, buffer);
	else
	    TRACE("%s => No description\n", drvName);
    }

    return count;
}

/* =================================
 *              M C I
 * ================================= */

/**************************************************************************
 * 			MCI_MapMsg16To32A			[internal]
 */
static WINMM_MapType	MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    if (*lParam == 0)
	return WINMM_MAP_OK;
    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:
    case MCI_PLAY:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESUME:
    case MCI_SEEK:
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:		/* FIXME: is wrong for digital video */
    case MCI_STEP:
    case MCI_STOP:
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	*lParam = (DWORD)MapSL(*lParam);
	return WINMM_MAP_OK;
    case MCI_WINDOW:
	/* in fact, I would also need the dwFlags... to see
	 * which members of lParam are effectively used
	 */
	*lParam = (DWORD)MapSL(*lParam);
	FIXME("Current mapping may be wrong\n");
	break;
    case MCI_BREAK:
	{
            LPMCI_BREAK_PARMS		mbp32 = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_BREAK_PARMS));
	    LPMCI_BREAK_PARMS16		mbp16 = MapSL(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->nVirtKey = mbp16->nVirtKey;
		mbp32->hwndBreak = HWND_32(mbp16->hwndBreak);
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return WINMM_MAP_OKMEM;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSA	mvep32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_VD_ESCAPE_PARMSA));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = MapSL(*lParam);

	    if (mvep32a) {
		mvep32a->dwCallback       = mvep16->dwCallback;
		mvep32a->lpstrCommand     = MapSL(mvep16->lpstrCommand);
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mvep32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_INFO_PARMSA));
	    LPMCI_INFO_PARMS16	mip16  = MapSL(*lParam);

	    /* FIXME this is wrong if device is of type
	     * MCI_DEVTYPE_DIGITAL_VIDEO, some members are not mapped
	     */
	    if (mip32a) {
		mip32a->dwCallback  = mip16->dwCallback;
		mip32a->lpstrReturn = MapSL(mip16->lpstrReturn);
		mip32a->dwRetSize   = mip16->dwRetSize;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mip32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	{
            LPMCI_OPEN_PARMSA	mop32a = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_OPEN_PARMS16) + sizeof(MCI_OPEN_PARMSA) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16  = MapSL(*lParam);

	    if (mop32a) {
		*(LPMCI_OPEN_PARMS16*)(mop32a) = mop16;
		mop32a = (LPMCI_OPEN_PARMSA)((char*)mop32a + sizeof(LPMCI_OPEN_PARMS16));
		mop32a->dwCallback       = mop16->dwCallback;
		mop32a->wDeviceID        = mop16->wDeviceID;
		mop32a->lpstrDeviceType  = MapSL(mop16->lpstrDeviceType);
		mop32a->lpstrElementName = MapSL(mop16->lpstrElementName);
		mop32a->lpstrAlias       = MapSL(mop16->lpstrAlias);
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop32a + 1, mop16 + 1, 2 * sizeof(DWORD));
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mop32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_SYSINFO_PARMSA));
	    LPMCI_SYSINFO_PARMS16	msip16  = MapSL(*lParam);

	    if (msip32a) {
		msip32a->dwCallback       = msip16->dwCallback;
		msip32a->lpstrReturn      = MapSL(msip16->lpstrReturn);
		msip32a->dwRetSize        = msip16->dwRetSize;
		msip32a->dwNumber         = msip16->dwNumber;
		msip32a->wDeviceType      = msip16->wDeviceType;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)msip32a;
	}
	return WINMM_MAP_OKMEM;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return WINMM_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
    }
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32A			[internal]
 */
static  WINMM_MapType	MCI_UnMapMsg16To32A(WORD uDevType, WORD wMsg, DWORD lParam)
{
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:
    case MCI_PLAY:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESUME:
    case MCI_SEEK:
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:
    case MCI_STEP:
    case MCI_STOP:
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	return WINMM_MAP_OK;

    case MCI_WINDOW:
	/* FIXME ?? see Map function */
	return WINMM_MAP_OK;

    case MCI_BREAK:
    case MCI_ESCAPE:
    case MCI_INFO:
    case MCI_SYSINFO:
	HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
	return WINMM_MAP_OK;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	if (lParam) {
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)lParam;
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)((char*)mop32a - sizeof(LPMCI_OPEN_PARMS16));

	    mop16->wDeviceID = mop32a->wDeviceID;
	    if (!HeapFree(GetProcessHeap(), 0, (LPVOID)(lParam - sizeof(LPMCI_OPEN_PARMS16))))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return WINMM_MAP_OK;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return WINMM_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
    }
    return WINMM_MAP_MSGERROR;
}

/*
 * 0000 stop
 * 0001 squeeze   signed 4 bytes to 2 bytes     *( LPINT16)D = ( INT16)*( LPINT16)S; D += 2;     S += 4
 * 0010 squeeze unsigned 4 bytes to 2 bytes     *(LPUINT16)D = (UINT16)*(LPUINT16)S; D += 2;     S += 4
 * 0100
 * 0101
 * 0110 zero 4 bytes                            *(DWORD)D = 0                        D += 4;     S += 4
 * 0111 copy string                             *(LPSTR*)D = seg dup(*(LPSTR*)S)     D += 4;     S += 4
 * 1xxx copy xxx + 1 bytes                      memcpy(D, S, xxx + 1);               D += xxx+1; S += xxx+1
 */

/**************************************************************************
 * 			MCI_MsgMapper32To16_Create		[internal]
 *
 * Helper for MCI_MapMsg32ATo16.
 * Maps the 32 bit pointer (*ptr), of size bytes, to an allocated 16 bit
 * segmented pointer.
 * map contains a list of action to be performed for the mapping (see list
 * above)
 * if keep is TRUE, keeps track of in 32 bit ptr in allocated 16 bit area.
 */
static	WINMM_MapType	MCI_MsgMapper32To16_Create(void** ptr, int size16, DWORD map, BOOLEAN keep)
{
    void*	lp = HeapAlloc( GetProcessHeap(), 0, (keep ? sizeof(void**) : 0) + size16 );
    LPBYTE	p16, p32;

    if (!lp) {
	return WINMM_MAP_NOMEM;
    }
    p32 = (LPBYTE)(*ptr);
    if (keep) {
	*(void**)lp = *ptr;
	p16 = (LPBYTE)lp + sizeof(void**);
	*ptr = (char*)MapLS(lp) + sizeof(void**);
    } else {
	p16 = lp;
	*ptr = (void*)MapLS(lp);
    }

    if (map == 0) {
	memcpy(p16, p32, size16);
    } else {
	unsigned	nibble;
	unsigned	sz;

	while (map & 0xF) {
	    nibble = map & 0xF;
	    if (nibble & 0x8) {
		sz = (nibble & 7) + 1;
		memcpy(p16, p32, sz);
		p16 += sz;
		p32 += sz;
		size16 -= sz;	/* DEBUG only */
	    } else {
		switch (nibble) {
		case 0x1:
                    *(LPINT16)p16 = *(LPINT)p32;
                    p16 += sizeof(INT16);
                    p32 += sizeof(INT);
                    size16 -= sizeof(INT16);
                    break;
		case 0x2:
                    *(LPUINT16)p16 = *(LPUINT)p32;
                    p16 += sizeof(UINT16);
                    p32 += sizeof(UINT);
                    size16 -= sizeof(UINT16);
                    break;
		case 0x6:
                    *(LPDWORD)p16 = 0;
                    p16 += sizeof(DWORD);
                    p32 += sizeof(DWORD);
                    size16 -= sizeof(DWORD);
                    break;
		case 0x7:
                    *(SEGPTR *)p16 = MapLS( *(LPSTR *)p32 );
                    p16 += sizeof(SEGPTR);
                    p32 += sizeof(LPSTR);
                    size16 -= sizeof(SEGPTR);
                    break;
		default:
                    FIXME("Unknown nibble for mapping (%x)\n", nibble);
		}
	    }
	    map >>= 4;
	}
	if (size16 != 0) /* DEBUG only */
	    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
    }
    return WINMM_MAP_OKMEM;
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Destroy		[internal]
 *
 * Helper for MCI_UnMapMsg32ATo16.
 */
static	WINMM_MapType	MCI_MsgMapper32To16_Destroy(void* ptr, int size16, DWORD map, BOOLEAN kept)
{
    if (ptr) {
	void*		msg16 = MapSL((SEGPTR)ptr);
	void*		alloc;
	LPBYTE		p32, p16;
	unsigned	nibble;

        UnMapLS( (SEGPTR)ptr );
	if (kept) {
	    alloc = (char*)msg16 - sizeof(void**);
	    p32 = *(void**)alloc;
	    p16 = msg16;

	    if (map == 0) {
		memcpy(p32, p16, size16);
	    } else {
		while (map & 0xF) {
		    nibble = map & 0xF;
		    if (nibble & 0x8) {
			memcpy(p32, p16, (nibble & 7) + 1);
			p16 += (nibble & 7) + 1;
			p32 += (nibble & 7) + 1;
			size16 -= (nibble & 7) + 1;
		    } else {
			switch (nibble) {
			case 0x1:
                            *(LPINT)p32 = *(LPINT16)p16;
                            p16 += sizeof(INT16);
                            p32 += sizeof(INT);
                            size16 -= sizeof(INT16);
                            break;
			case 0x2:
                            *(LPUINT)p32 = *(LPUINT16)p16;
                            p16 += sizeof(UINT16);
                            p32 += sizeof(UINT);
                            size16 -= sizeof(UINT16);
                            break;
			case 0x6:
                            p16 += sizeof(UINT);
                            p32 += sizeof(UINT);
                            size16 -= sizeof(UINT);
                            break;
			case 0x7:
                            UnMapLS( *(SEGPTR *)p16 );
                            p16 += sizeof(SEGPTR);
                            p32 += sizeof(char*);
                            size16 -= sizeof(SEGPTR);
                            break;
			default:
                            FIXME("Unknown nibble for mapping (%x)\n", nibble);
			}
		    }
		    map >>= 4;
		}
		if (size16 != 0) /* DEBUG only */
		    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
	    }
	} else {
	    alloc = msg16;
	}

        HeapFree( GetProcessHeap(), 0, alloc );
    }
    return WINMM_MAP_OK;
}

/**************************************************************************
 * 			MCI_MapMsg32ATo16			[internal]
 *
 * Map a 32-A bit MCI message to a 16 bit MCI message.
 */
static  WINMM_MapType	MCI_MapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD* lParam)
{
    int		size;
    BOOLEAN     keep = FALSE;
    DWORD	map = 0;

    if (*lParam == 0)
	return WINMM_MAP_OK;

    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
    case MCI_BREAK:
	size = sizeof(MCI_BREAK_PARMS);
	break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
	/* case MCI_COPY: */
    case MCI_CUE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_CUE_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_CUE_PARMS);	break;*/	FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_CUT:*/
    case MCI_DELETE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_DELETE_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_DELETE_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_FREEZE_PARMS);	map = 0x0001111B; 	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_GETDEVCAPS:
	keep = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = (LPMCI_INFO_PARMSA)(*lParam);
	    LPMCI_INFO_PARMS16	mip16;

	    switch (uDevType) {
	    case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_INFO_PARMS16);	break;
	    default:				size = sizeof(MCI_INFO_PARMS16);	break;
	    }
            mip16 = HeapAlloc( GetProcessHeap(), 0, size);
            if (mip16)
            {
		mip16->dwCallback  = mip32a->dwCallback;
		mip16->lpstrReturn = MapLS( mip32a->lpstrReturn );
		mip16->dwRetSize   = mip32a->dwRetSize;
		if (uDevType == MCI_DEVTYPE_DIGITAL_VIDEO) {
		    ((LPMCI_DGV_INFO_PARMS16)mip16)->dwItem = ((LPMCI_DGV_INFO_PARMSA)mip32a)->dwItem;
		}
	    } else {
		return WINMM_MAP_NOMEM;
	    }
            *lParam = MapLS(mip16);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	{
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)(*lParam);
            char* ptr = HeapAlloc( GetProcessHeap(), 0,
                                   sizeof(LPMCI_OPEN_PARMSA) + sizeof(MCI_OPEN_PARMS16) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16;


	    if (ptr) {
		*(LPMCI_OPEN_PARMSA*)(ptr) = mop32a;
		mop16 = (LPMCI_OPEN_PARMS16)(ptr + sizeof(LPMCI_OPEN_PARMSA));
		mop16->dwCallback       = mop32a->dwCallback;
		mop16->wDeviceID        = mop32a->wDeviceID;
		if (dwFlags & MCI_OPEN_TYPE) {
		    if (dwFlags & MCI_OPEN_TYPE_ID) {
			/* dword "transparent" value */
			mop16->lpstrDeviceType = (SEGPTR)mop32a->lpstrDeviceType;
		    } else {
			/* string */
			mop16->lpstrDeviceType = MapLS( mop32a->lpstrDeviceType );
		    }
		} else {
		    /* nuthin' */
		    mop16->lpstrDeviceType = 0;
		}
		if (dwFlags & MCI_OPEN_ELEMENT) {
		    if (dwFlags & MCI_OPEN_ELEMENT_ID) {
			mop16->lpstrElementName = (SEGPTR)mop32a->lpstrElementName;
		    } else {
			mop16->lpstrElementName = MapLS( mop32a->lpstrElementName );
		    }
		} else {
		    mop16->lpstrElementName = 0;
		}
		if (dwFlags & MCI_OPEN_ALIAS) {
		    mop16->lpstrAlias = MapLS( mop32a->lpstrAlias );
		} else {
		    mop16->lpstrAlias = 0;
		}
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop16 + 1, mop32a + 1, 2 * sizeof(DWORD));
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_OPEN_PARMSA);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_PLAY:
	size = sizeof(MCI_PLAY_PARMS);
	break;
    case MCI_PUT:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_REALIZE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_RECORD:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECORD_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_RECORD_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_RECORD_PARMS);	break;
	}
	break;
    case MCI_RESUME:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_SEEK:
	switch (uDevType) {
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SEEK_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_SEEK_PARMS);		break;
	}
	break;
    case MCI_SET:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SET_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SET_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	case MCI_DEVTYPE_SEQUENCER:	size = sizeof(MCI_SEQ_SET_PARMS);	break;
        /* FIXME: normally the 16 and 32 bit structures are byte by byte aligned,
	 * so not doing anything should work...
	 */
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_SET_PARMS);	break;
	default:			size = sizeof(MCI_SET_PARMS);		break;
	}
	break;
    case MCI_SETAUDIO:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SETAUDIO_PARMS16);map = 0x0000077FF;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SETAUDIO_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	size = sizeof(MCI_SET_PARMS);
	break;
    case MCI_STATUS:
	keep = TRUE;
	switch (uDevType) {
	/* FIXME:
	 * don't know if buffer for value is the one passed through lpstrDevice
	 * or is provided by MCI driver.
	 * Assuming solution 2: provided by MCI driver, so zeroing on entry
	 */
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	map = 0x0B6FF;		break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STEP_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STEP_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	case MCI_DEVTYPE_VIDEODISC:	size = sizeof(MCI_VD_STEP_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_STOP:
	size = sizeof(MCI_SET_PARMS);
	break;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA  msip32a = (LPMCI_SYSINFO_PARMSA)(*lParam);
            LPMCI_SYSINFO_PARMS16 msip16;
            char* ptr = HeapAlloc( GetProcessHeap(), 0,
                                   sizeof(LPMCI_SYSINFO_PARMSA) + sizeof(MCI_SYSINFO_PARMS16) );

	    if (ptr) {
		*(LPMCI_SYSINFO_PARMSA*)(ptr) = msip32a;
		msip16 = (LPMCI_SYSINFO_PARMS16)(ptr + sizeof(LPMCI_SYSINFO_PARMSA));

		msip16->dwCallback       = msip32a->dwCallback;
		msip16->lpstrReturn      = MapLS( msip32a->lpstrReturn );
		msip16->dwRetSize        = msip32a->dwRetSize;
		msip16->dwNumber         = msip32a->dwNumber;
		msip16->wDeviceType      = msip32a->wDeviceType;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_SYSINFO_PARMSA);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_UPDATE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_UPDATE_PARMS16);	map = 0x000B1111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7FB;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7FB;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case DRV_OPEN:
	{
            LPMCI_OPEN_DRIVER_PARMSA  modp32a = (LPMCI_OPEN_DRIVER_PARMSA)(*lParam);
            LPMCI_OPEN_DRIVER_PARMS16 modp16;
            char *ptr = HeapAlloc( GetProcessHeap(), 0,
                                  sizeof(LPMCI_OPEN_DRIVER_PARMSA) + sizeof(MCI_OPEN_DRIVER_PARMS16));

	    if (ptr) {
		*(LPMCI_OPEN_DRIVER_PARMSA*)(ptr) = modp32a;
		modp16 = (LPMCI_OPEN_DRIVER_PARMS16)(ptr + sizeof(LPMCI_OPEN_DRIVER_PARMSA));
		modp16->wDeviceID = modp32a->wDeviceID;
		modp16->lpstrParams = MapLS( modp32a->lpstrParams );
		/* other fields are gonna be filled by the driver, don't copy them */
 	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_OPEN_DRIVER_PARMSA);
	}
	return WINMM_MAP_OKMEM;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	return WINMM_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
	return WINMM_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Create((void**)lParam, size, map, keep);
}

/**************************************************************************
 * 			MCI_UnMapMsg32ATo16			[internal]
 */
static  WINMM_MapType	MCI_UnMapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD lParam)
{
    int		size = 0;
    BOOLEAN     kept = FALSE;	/* there is no need to compute size when kept is FALSE */
    DWORD 	map = 0;

    switch (wMsg) {
    case MCI_BREAK:
        break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
	break;
	/* case MCI_COPY: */
    case MCI_CUE:
	break;
	/* case MCI_CUT: */
    case MCI_DELETE:
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	break;
    case MCI_GETDEVCAPS:
	kept = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMS16 mip16  = (LPMCI_INFO_PARMS16)MapSL(lParam);
            UnMapLS( lParam );
            UnMapLS( mip16->lpstrReturn );
            HeapFree( GetProcessHeap(), 0, mip16 );
	}
	return WINMM_MAP_OK;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	if (lParam) {
            LPMCI_OPEN_PARMS16	mop16  = (LPMCI_OPEN_PARMS16)MapSL(lParam);
	    LPMCI_OPEN_PARMSA	mop32a = *(LPMCI_OPEN_PARMSA*)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA));
            UnMapLS( lParam );
	    mop32a->wDeviceID = mop16->wDeviceID;
            if ((dwFlags & MCI_OPEN_TYPE) && !(dwFlags & MCI_OPEN_TYPE_ID))
                UnMapLS( mop16->lpstrDeviceType );
            if ((dwFlags & MCI_OPEN_ELEMENT) && !(dwFlags & MCI_OPEN_ELEMENT_ID))
                UnMapLS( mop16->lpstrElementName );
            if (dwFlags & MCI_OPEN_ALIAS)
                UnMapLS( mop16->lpstrAlias );
            HeapFree( GetProcessHeap(), 0, (char*)mop16 - sizeof(LPMCI_OPEN_PARMSA) );
	}
	return WINMM_MAP_OK;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	break;
    case MCI_PLAY:
	break;
    case MCI_PUT:
	break;
    case MCI_REALIZE:
	break;
    case MCI_RECORD:
	break;
    case MCI_RESUME:
	break;
    case MCI_SEEK:
	break;
    case MCI_SET:
	break;
    case MCI_SETAUDIO:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	map = 0x0000077FF;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SETAUDIO_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	}
	break;
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	break;
    case MCI_STATUS:
	kept = TRUE;
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	if (lParam) {
            LPMCI_DGV_STATUS_PARMS16	mdsp16  = (LPMCI_DGV_STATUS_PARMS16)MapSL(lParam);
	    LPMCI_DGV_STATUS_PARMSA	mdsp32a = *(LPMCI_DGV_STATUS_PARMSA*)((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA));

            UnMapLS( lParam );
	    if (mdsp16) {
		mdsp32a->dwReturn = mdsp16->dwReturn;
		if (dwFlags & MCI_DGV_STATUS_DISKSPACE) {
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%08lx\n", mdsp16->lpstrDrive);
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%s\n", (LPSTR)MapSL(mdsp16->lpstrDrive));
                    UnMapLS( mdsp16->lpstrDrive );
		}
                HeapFree( GetProcessHeap(), 0, (char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA) );
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	}
	return WINMM_MAP_OKMEM;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	break;
    case MCI_STOP:
	break;
    case MCI_SYSINFO:
	if (lParam) {
            LPMCI_SYSINFO_PARMS16	msip16  = (LPMCI_SYSINFO_PARMS16)MapSL(lParam);
	    LPMCI_SYSINFO_PARMSA	msip32a = *(LPMCI_SYSINFO_PARMSA*)((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA));

            UnMapLS( lParam );
	    if (msip16) {
                msip16->dwCallback = msip32a->dwCallback;
                UnMapLS( msip16->lpstrReturn );
                HeapFree( GetProcessHeap(), 0, (char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA) );
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	break;
    case MCI_UPDATE:
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	default:			break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7666;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7666;	break;
	default:			break;
	}
	/* FIXME: see map function */
	break;

    case DRV_OPEN:
	if (lParam) {
            LPMCI_OPEN_DRIVER_PARMS16	modp16  = (LPMCI_OPEN_DRIVER_PARMS16)MapSL(lParam);
	    LPMCI_OPEN_DRIVER_PARMSA	modp32a = *(LPMCI_OPEN_DRIVER_PARMSA*)((char*)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA));

            UnMapLS( lParam );
	    modp32a->wCustomCommandTable = modp16->wCustomCommandTable;
	    modp32a->wType = modp16->wType;
            UnMapLS( modp16->lpstrParams );
            HeapFree( GetProcessHeap(), 0, (char *)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA) );
	}
	return WINMM_MAP_OK;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return WINMM_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
	return WINMM_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Destroy((void*)lParam, size, map, kept);
}

void    MMDRV_Init16(void)
{
#define A(_x,_y) MMDRV_InstallMap(_x, \
MMDRV_##_y##_Map16To32A, MMDRV_##_y##_UnMap16To32A, \
MMDRV_##_y##_Map32ATo16, MMDRV_##_y##_UnMap32ATo16, \
MMDRV_##_y##_Callback)
    A(MMDRV_AUX,        Aux);
    A(MMDRV_MIXER,      Mixer);
    A(MMDRV_MIDIIN,     MidiIn);
    A(MMDRV_MIDIOUT,    MidiOut);
    A(MMDRV_WAVEIN,     WaveIn);
    A(MMDRV_WAVEOUT,    WaveOut);
#undef A

    pFnCallMMDrvFunc16 = MMDRV_CallMMDrvFunc16;
    pFnLoadMMDrvFunc16 = MMDRV_LoadMMDrvFunc16;

    pFnMciMapMsg16To32A   = MCI_MapMsg16To32A;
    pFnMciUnMapMsg16To32A = MCI_UnMapMsg16To32A;
    pFnMciMapMsg32ATo16   = MCI_MapMsg32ATo16;
    pFnMciUnMapMsg32ATo16 = MCI_UnMapMsg32ATo16;
}
