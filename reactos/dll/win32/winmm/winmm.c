/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * WINMM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2002 Eric Pouech
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

/*
 * Eric POUECH :
 * 	98/9	added Win32 MCI support
 *  	99/4	added midiStream support
 *      99/9	added support for loadable low level drivers
 */

/* TODO
 *      + it seems that some programs check what's installed in
 *        registry against the value returned by drivers. Wine is
 *        currently broken regarding this point.
 *      + check thread-safeness for MMSYSTEM and WINMM entry points
 *      + unicode entry points are badly supported (would require
 *        moving 32 bit drivers as Unicode as they are supposed to be)
 *      + allow joystick and timer external calls as we do for wave,
 *        midi, mixer and aux
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

void    (WINAPI *pFnReleaseThunkLock)(DWORD*);
void    (WINAPI *pFnRestoreThunkLock)(DWORD);

/* ========================================================================
 *                   G L O B A L   S E T T I N G S
 * ========================================================================*/

WINE_MM_IDATA WINMM_IData;

/**************************************************************************
 * 			WINMM_CreateIData			[internal]
 */
static	BOOL	WINMM_CreateIData(HINSTANCE hInstDLL)
{
    memset( &WINMM_IData, 0, sizeof WINMM_IData );

    WINMM_IData.hWinMM32Instance = hInstDLL;
    InitializeCriticalSection(&WINMM_IData.cs);
/* FIXME crashes in ReactOS
    WINMM_IData.cs.DebugInfo->Spare[1] = (DWORD)"WINMM_IData";
*/
    WINMM_IData.psStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    WINMM_IData.psLastEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    TRACE("Initialized IData (%p)\n", &WINMM_IData);
    return TRUE;
}

/**************************************************************************
 * 			WINMM_DeleteIData			[internal]
 */
static	void WINMM_DeleteIData(void)
{
    TIME_MMTimeStop();

    /* FIXME: should also free content and resources allocated
     * inside WINMM_IData */
    CloseHandle(WINMM_IData.psStopEvent);
    CloseHandle(WINMM_IData.psLastEvent);
    DeleteCriticalSection(&WINMM_IData.cs);
}

/******************************************************************
 *             WINMM_LoadMMSystem
 *
 */
#ifndef __REACTOS__
static HANDLE (WINAPI *pGetModuleHandle16)(LPCSTR);
static DWORD (WINAPI *pLoadLibrary16)(LPCSTR);
#endif

BOOL WINMM_CheckForMMSystem(void)
{
    /* 0 is not checked yet, -1 is not present, 1 is present */
    static      int    loaded /* = 0 */;

    if (loaded == 0)
    {
        HANDLE      h = GetModuleHandleA("kernel32");
        loaded = -1;
        if (h)
        {
#ifndef __REACTOS__
            pGetModuleHandle16 = (void*)GetProcAddress(h, "GetModuleHandle16");
            pLoadLibrary16 = (void*)GetProcAddress(h, "LoadLibrary16");
            if (pGetModuleHandle16 && pLoadLibrary16 &&
                (pGetModuleHandle16("MMSYSTEM.DLL") || pLoadLibrary16("MMSYSTEM.DLL")))
#endif /* __REACTOS__ */
                loaded = 1;
        }
    }
    return loaded > 0;
}

/******************************************************************
 *             WINMM_ErrorToString
 */
const char* WINMM_ErrorToString(MMRESULT error)
{
#define ERR_TO_STR(dev) case dev: return #dev
    static char unknown[32];
    switch (error) {
    ERR_TO_STR(MMSYSERR_NOERROR);
    ERR_TO_STR(MMSYSERR_ERROR);
    ERR_TO_STR(MMSYSERR_BADDEVICEID);
    ERR_TO_STR(MMSYSERR_NOTENABLED);
    ERR_TO_STR(MMSYSERR_ALLOCATED);
    ERR_TO_STR(MMSYSERR_INVALHANDLE);
    ERR_TO_STR(MMSYSERR_NODRIVER);
    ERR_TO_STR(MMSYSERR_NOMEM);
    ERR_TO_STR(MMSYSERR_NOTSUPPORTED);
    ERR_TO_STR(MMSYSERR_BADERRNUM);
    ERR_TO_STR(MMSYSERR_INVALFLAG);
    ERR_TO_STR(MMSYSERR_INVALPARAM);
    ERR_TO_STR(MMSYSERR_HANDLEBUSY);
    ERR_TO_STR(MMSYSERR_INVALIDALIAS);
    ERR_TO_STR(MMSYSERR_BADDB);
    ERR_TO_STR(MMSYSERR_KEYNOTFOUND);
    ERR_TO_STR(MMSYSERR_READERROR);
    ERR_TO_STR(MMSYSERR_WRITEERROR);
    ERR_TO_STR(MMSYSERR_DELETEERROR);
    ERR_TO_STR(MMSYSERR_VALNOTFOUND);
    ERR_TO_STR(MMSYSERR_NODRIVERCB);
    ERR_TO_STR(WAVERR_BADFORMAT);
    ERR_TO_STR(WAVERR_STILLPLAYING);
    ERR_TO_STR(WAVERR_UNPREPARED);
    ERR_TO_STR(WAVERR_SYNC);
    }
    sprintf(unknown, "Unknown(0x%08x)", error);
    return unknown;
#undef ERR_TO_STR
}

/**************************************************************************
 *		DllMain (WINMM.init)
 *
 * WINMM DLL entry point
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);

	if (!WINMM_CreateIData(hInstDLL))
	    return FALSE;
        if (!MMDRV_Init()) {
            WINMM_DeleteIData();
            return FALSE;
	}
	break;
    case DLL_PROCESS_DETACH:
        /* close all opened MCI drivers */
        MCI_SendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0L, TRUE);
        MMDRV_Exit();
        /* now unload all remaining drivers... */
        DRIVER_UnloadAll();

	WINMM_DeleteIData();
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 	Mixer devices. New to Win95
 */

/**************************************************************************
 * find out the real mixer ID depending on hmix (depends on dwFlags)
 */
static UINT MIXER_GetDev(HMIXEROBJ hmix, DWORD dwFlags, LPWINE_MIXER * lplpwm)
{
    LPWINE_MIXER	lpwm = NULL;
    UINT		uRet = MMSYSERR_NOERROR;

    switch (dwFlags & 0xF0000000ul) {
    case MIXER_OBJECTF_MIXER:
	lpwm = (LPWINE_MIXER)MMDRV_Get(hmix, MMDRV_MIXER, TRUE);
	break;
    case MIXER_OBJECTF_HMIXER:
	lpwm = (LPWINE_MIXER)MMDRV_Get(hmix, MMDRV_MIXER, FALSE);
	break;
    case MIXER_OBJECTF_WAVEOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEOUT, TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HWAVEOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEOUT, FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_WAVEIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEIN,  TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HWAVEIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEIN,  FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_MIDIOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIOUT, TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HMIDIOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIOUT, FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_MIDIIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIIN,  TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HMIDIIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIIN,  FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_AUX:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_AUX,     TRUE,  MMDRV_MIXER);
	break;
    default:
	WARN("Unsupported flag (%08lx)\n", dwFlags & 0xF0000000ul);
        lpwm = 0;
        uRet = MMSYSERR_INVALFLAG;
	break;
    }
    *lplpwm = lpwm;
    if (lpwm == 0 && uRet == MMSYSERR_NOERROR)
        uRet = MMSYSERR_INVALPARAM;
    return uRet;
}

/**************************************************************************
 * 				mixerGetNumDevs			[WINMM.@]
 */
UINT WINAPI mixerGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIXER);
}

/**************************************************************************
 * 				mixerGetDevCapsA		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsA(UINT_PTR uDeviceID, LPMIXERCAPSA lpCaps, UINT uSize)
{
    MIXERCAPSW micW;
    UINT       ret;

    if (lpCaps == NULL) return MMSYSERR_INVALPARAM;

    ret = mixerGetDevCapsW(uDeviceID, &micW, sizeof(micW));

    if (ret == MMSYSERR_NOERROR) {
        MIXERCAPSA micA;
        micA.wMid           = micW.wMid;
        micA.wPid           = micW.wPid;
        micA.vDriverVersion = micW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, micW.szPname, -1, micA.szPname,
                             sizeof(micA.szPname), NULL, NULL );
        micA.fdwSupport     = micW.fdwSupport;
        micA.cDestinations  = micW.cDestinations;
        memcpy(lpCaps, &micA, min(uSize, sizeof(micA)));
    }
    return ret;
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsW(UINT_PTR uDeviceID, LPMIXERCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD wmld;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIXER, TRUE)) == NULL)
        return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

UINT  MIXER_Open(LPHMIXER lphMix, UINT uDeviceID, DWORD_PTR dwCallback,
                 DWORD_PTR dwInstance, DWORD fdwOpen, BOOL bFrom32)
{
    HANDLE		hMix;
    LPWINE_MLD		wmld;
    DWORD		dwRet = 0;
    MIXEROPENDESC	mod;

    TRACE("(%p, %d, %08lx, %08lx, %08lx)\n",
	  lphMix, uDeviceID, dwCallback, dwInstance, fdwOpen);

    wmld = MMDRV_Alloc(sizeof(WINE_MIXER), MMDRV_MIXER, &hMix, &fdwOpen,
		       &dwCallback, &dwInstance, bFrom32);

    wmld->uDeviceID = uDeviceID;
    mod.hmx = (HMIXEROBJ)hMix;
    mod.dwCallback = dwCallback;
    mod.dwInstance = dwInstance;

    dwRet = MMDRV_Open(wmld, MXDM_OPEN, (DWORD)&mod, fdwOpen);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMix, wmld);
	hMix = 0;
    }
    if (lphMix) *lphMix = hMix;
    TRACE("=> %ld hMixer=%p\n", dwRet, hMix);

    return dwRet;
}

/**************************************************************************
 * 				mixerOpen			[WINMM.@]
 */
MMRESULT WINAPI mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD_PTR dwCallback,
                     DWORD_PTR dwInstance, DWORD fdwOpen)
{
    return MIXER_Open(lphMix, uDeviceID, dwCallback, dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				mixerClose			[WINMM.@]
 */
UINT WINAPI mixerClose(HMIXER hMix)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMix);

    if ((wmld = MMDRV_Get(hMix, MMDRV_MIXER, FALSE)) == NULL) return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MXDM_CLOSE);
    MMDRV_Free(hMix, wmld);

    return dwRet;
}

/**************************************************************************
 * 				mixerGetID			[WINMM.@]
 */
UINT WINAPI mixerGetID(HMIXEROBJ hmix, LPUINT lpid, DWORD fdwID)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p %p %08lx)\n", hmix, lpid, fdwID);

    if ((uRet = MIXER_GetDev(hmix, fdwID, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpid)
      *lpid = lpwm->mld.uDeviceID;

    return uRet;
}

/**************************************************************************
 * 				mixerGetControlDetailsW		[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdW,
				    DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcdW, fdwDetails);

    if ((uRet = MIXER_GetDev(hmix, fdwDetails, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpmcdW == NULL || lpmcdW->cbStruct != sizeof(*lpmcdW))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETCONTROLDETAILS, (DWORD_PTR)lpmcdW,
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerGetControlDetailsA	[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA,
                                    DWORD fdwDetails)
{
    DWORD			ret = MMSYSERR_NOTENABLED;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcdA, fdwDetails);

    if (lpmcdA == NULL || lpmcdA->cbStruct != sizeof(*lpmcdA))
	return MMSYSERR_INVALPARAM;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	/* can savely use A structure as it is, no string inside */
	ret = mixerGetControlDetailsW(hmix, lpmcdA, fdwDetails);
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	{
	    MIXERCONTROLDETAILS_LISTTEXTA *pDetailsA = (MIXERCONTROLDETAILS_LISTTEXTA *)lpmcdA->paDetails;
            MIXERCONTROLDETAILS_LISTTEXTW *pDetailsW;
	    int size = max(1, lpmcdA->cChannels) * sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
            unsigned int i;

	    if (lpmcdA->u.cMultipleItems != 0) {
		size *= lpmcdA->u.cMultipleItems;
	    }
	    pDetailsW = HeapAlloc(GetProcessHeap(), 0, size);
            lpmcdA->paDetails = pDetailsW;
            lpmcdA->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
	    /* set up lpmcd->paDetails */
	    ret = mixerGetControlDetailsW(hmix, lpmcdA, fdwDetails);
	    /* copy from lpmcd->paDetails back to paDetailsW; */
            if (ret == MMSYSERR_NOERROR) {
                for (i = 0; i < lpmcdA->u.cMultipleItems * lpmcdA->cChannels; i++) {
                    pDetailsA->dwParam1 = pDetailsW->dwParam1;
                    pDetailsA->dwParam2 = pDetailsW->dwParam2;
                    WideCharToMultiByte( CP_ACP, 0, pDetailsW->szName, -1,
                                         pDetailsA->szName,
                                         sizeof(pDetailsA->szName), NULL, NULL );
                    pDetailsA++;
                    pDetailsW++;
                }
                pDetailsA -= lpmcdA->u.cMultipleItems * lpmcdA->cChannels;
                pDetailsW -= lpmcdA->u.cMultipleItems * lpmcdA->cChannels;
            }
	    HeapFree(GetProcessHeap(), 0, pDetailsW);
	    lpmcdA->paDetails = pDetailsA;
            lpmcdA->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
	}
	break;
    default:
	ERR("Unsupported fdwDetails=0x%08lx\n", fdwDetails);
    }

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.@]
 */
UINT WINAPI mixerGetLineControlsA(HMIXEROBJ hmix, LPMIXERLINECONTROLSA lpmlcA,
				  DWORD fdwControls)
{
    MIXERLINECONTROLSW	mlcW;
    DWORD		ret;
    unsigned int	i;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmlcA, fdwControls);

    if (lpmlcA == NULL || lpmlcA->cbStruct != sizeof(*lpmlcA) ||
	lpmlcA->cbmxctrl != sizeof(MIXERCONTROLA))
	return MMSYSERR_INVALPARAM;

    mlcW.cbStruct = sizeof(mlcW);
    mlcW.dwLineID = lpmlcA->dwLineID;
    mlcW.u.dwControlID = lpmlcA->u.dwControlID;
    mlcW.u.dwControlType = lpmlcA->u.dwControlType;

    /* Debugging on Windows shows for MIXER_GETLINECONTROLSF_ONEBYTYPE only,
       the control count is assumed to be 1 - This is relied upon by a game,
       "Dynomite Deluze"                                                    */
    if (MIXER_GETLINECONTROLSF_ONEBYTYPE == fdwControls) {
        mlcW.cControls = 1;
    } else {
        mlcW.cControls = lpmlcA->cControls;
    }
    mlcW.cbmxctrl = sizeof(MIXERCONTROLW);
    mlcW.pamxctrl = HeapAlloc(GetProcessHeap(), 0,
			      mlcW.cControls * mlcW.cbmxctrl);

    ret = mixerGetLineControlsW(hmix, &mlcW, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlcA->dwLineID = mlcW.dwLineID;
	lpmlcA->u.dwControlID = mlcW.u.dwControlID;
	lpmlcA->u.dwControlType = mlcW.u.dwControlType;
	lpmlcA->cControls = mlcW.cControls;

	for (i = 0; i < mlcW.cControls; i++) {
	    lpmlcA->pamxctrl[i].cbStruct = sizeof(MIXERCONTROLA);
	    lpmlcA->pamxctrl[i].dwControlID = mlcW.pamxctrl[i].dwControlID;
	    lpmlcA->pamxctrl[i].dwControlType = mlcW.pamxctrl[i].dwControlType;
	    lpmlcA->pamxctrl[i].fdwControl = mlcW.pamxctrl[i].fdwControl;
	    lpmlcA->pamxctrl[i].cMultipleItems = mlcW.pamxctrl[i].cMultipleItems;
            WideCharToMultiByte( CP_ACP, 0, mlcW.pamxctrl[i].szShortName, -1,
                                 lpmlcA->pamxctrl[i].szShortName,
                                 sizeof(lpmlcA->pamxctrl[i].szShortName), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, mlcW.pamxctrl[i].szName, -1,
                                 lpmlcA->pamxctrl[i].szName,
                                 sizeof(lpmlcA->pamxctrl[i].szName), NULL, NULL );
	    /* sizeof(lpmlcA->pamxctrl[i].Bounds) ==
	     * sizeof(mlcW.pamxctrl[i].Bounds) */
	    memcpy(&lpmlcA->pamxctrl[i].Bounds, &mlcW.pamxctrl[i].Bounds,
		   sizeof(mlcW.pamxctrl[i].Bounds));
	    /* sizeof(lpmlcA->pamxctrl[i].Metrics) ==
	     * sizeof(mlcW.pamxctrl[i].Metrics) */
	    memcpy(&lpmlcA->pamxctrl[i].Metrics, &mlcW.pamxctrl[i].Metrics,
		   sizeof(mlcW.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcW.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsW		[WINMM.@]
 */
UINT WINAPI mixerGetLineControlsW(HMIXEROBJ hmix, LPMIXERLINECONTROLSW lpmlcW,
				  DWORD fdwControls)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmlcW, fdwControls);

    if ((uRet = MIXER_GetDev(hmix, fdwControls, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpmlcW == NULL || lpmlcW->cbStruct != sizeof(*lpmlcW))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINECONTROLS, (DWORD_PTR)lpmlcW,
			 fdwControls, TRUE);
}

/**************************************************************************
 * 				mixerGetLineInfoW		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoW(HMIXEROBJ hmix, LPMIXERLINEW lpmliW, DWORD fdwInfo)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmliW, fdwInfo);

    if ((uRet = MIXER_GetDev(hmix, fdwInfo, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINEINFO, (DWORD_PTR)lpmliW,
			 fdwInfo, TRUE);
}

/**************************************************************************
 * 				mixerGetLineInfoA		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoA(HMIXEROBJ hmix, LPMIXERLINEA lpmliA,
			      DWORD fdwInfo)
{
    MIXERLINEW		mliW;
    UINT		ret;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmliA, fdwInfo);

    if (lpmliA == NULL || lpmliA->cbStruct != sizeof(*lpmliA))
	return MMSYSERR_INVALPARAM;

    mliW.cbStruct = sizeof(mliW);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliW.dwComponentType = lpmliA->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliW.dwDestination = lpmliA->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliW.dwLineID = lpmliA->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliW.dwDestination = lpmliA->dwDestination;
	mliW.dwSource = lpmliA->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliW.Target.dwType = lpmliA->Target.dwType;
	mliW.Target.wMid = lpmliA->Target.wMid;
	mliW.Target.wPid = lpmliA->Target.wPid;
	mliW.Target.vDriverVersion = lpmliA->Target.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, lpmliA->Target.szPname, -1, mliW.Target.szPname, sizeof(mliW.Target.szPname));
	break;
    default:
	WARN("Unsupported fdwControls=0x%08lx\n", fdwInfo);
        return MMSYSERR_INVALFLAG;
    }

    ret = mixerGetLineInfoW(hmix, &mliW, fdwInfo);

    lpmliA->dwDestination = mliW.dwDestination;
    lpmliA->dwSource = mliW.dwSource;
    lpmliA->dwLineID = mliW.dwLineID;
    lpmliA->fdwLine = mliW.fdwLine;
    lpmliA->dwUser = mliW.dwUser;
    lpmliA->dwComponentType = mliW.dwComponentType;
    lpmliA->cChannels = mliW.cChannels;
    lpmliA->cConnections = mliW.cConnections;
    lpmliA->cControls = mliW.cControls;
    WideCharToMultiByte( CP_ACP, 0, mliW.szShortName, -1, lpmliA->szShortName,
                         sizeof(lpmliA->szShortName), NULL, NULL);
    WideCharToMultiByte( CP_ACP, 0, mliW.szName, -1, lpmliA->szName,
                         sizeof(lpmliA->szName), NULL, NULL );
    lpmliA->Target.dwType = mliW.Target.dwType;
    lpmliA->Target.dwDeviceID = mliW.Target.dwDeviceID;
    lpmliA->Target.wMid = mliW.Target.wMid;
    lpmliA->Target.wPid = mliW.Target.wPid;
    lpmliA->Target.vDriverVersion = mliW.Target.vDriverVersion;
    WideCharToMultiByte( CP_ACP, 0, mliW.Target.szPname, -1, lpmliA->Target.szPname,
                         sizeof(lpmliA->Target.szPname), NULL, NULL );

    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.@]
 */
UINT WINAPI mixerSetControlDetails(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd,
				   DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcd, fdwDetails);

    if ((uRet = MIXER_GetDev(hmix, fdwDetails, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    return MMDRV_Message(&lpwm->mld, MXDM_SETCONTROLDETAILS, (DWORD_PTR)lpmcd,
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerMessage		[WINMM.@]
 */
DWORD WINAPI mixerMessage(HMIXER hmix, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04lx, %d, %08lx, %08lx): semi-stub?\n",
	  (DWORD)hmix, uMsg, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hmix, MMDRV_MIXER, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMsg, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.@]
 */
UINT WINAPI auxGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_AUX);
}

/**************************************************************************
 * 				auxGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI auxGetDevCapsW(UINT_PTR uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				auxGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT uSize)
{
    AUXCAPSW	acW;
    UINT	ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = auxGetDevCapsW(uDeviceID, &acW, sizeof(acW));

    if (ret == MMSYSERR_NOERROR) {
	AUXCAPSA acA;
	acA.wMid           = acW.wMid;
	acA.wPid           = acW.wPid;
	acA.vDriverVersion = acW.vDriverVersion;
	WideCharToMultiByte( CP_ACP, 0, acW.szPname, -1, acA.szPname,
                             sizeof(acA.szPname), NULL, NULL );
	acA.wTechnology    = acW.wTechnology;
	acA.dwSupport      = acW.dwSupport;
	memcpy(lpCaps, &acA, min(uSize, sizeof(acA)));
    }
    return ret;
}

/**************************************************************************
 * 				auxGetVolume		[WINMM.@]
 */
MMRESULT WINAPI auxGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p) !\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETVOLUME, (DWORD_PTR)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxSetVolume		[WINMM.@]
 */
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %lu) !\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxOutMessage		[WINMM.@]
 */
MMRESULT WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD_PTR dw1, DWORD_PTR dw2)
{
    LPWINE_MLD		wmld;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2, TRUE);
}

/**************************************************************************
 * 				midiOutGetNumDevs	[WINMM.@]
 */
UINT WINAPI midiOutGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIOUT);
}

/**************************************************************************
 * 				midiOutGetDevCapsW	[WINMM.@]
 */
UINT WINAPI midiOutGetDevCapsW(UINT_PTR uDeviceID, LPMIDIOUTCAPSW lpCaps,
			       UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%u, %p, %u);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.@]
 */
UINT WINAPI midiOutGetDevCapsA(UINT_PTR uDeviceID, LPMIDIOUTCAPSA lpCaps,
			       UINT uSize)
{
    MIDIOUTCAPSW	mocW;
    UINT		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = midiOutGetDevCapsW(uDeviceID, &mocW, sizeof(mocW));

    if (ret == MMSYSERR_NOERROR) {
	MIDIOUTCAPSA mocA;
	mocA.wMid		= mocW.wMid;
	mocA.wPid		= mocW.wPid;
	mocA.vDriverVersion	= mocW.vDriverVersion;
	WideCharToMultiByte( CP_ACP, 0, mocW.szPname, -1, mocA.szPname,
                             sizeof(mocA.szPname), NULL, NULL );
	mocA.wTechnology        = mocW.wTechnology;
	mocA.wVoices		= mocW.wVoices;
	mocA.wNotes		= mocW.wNotes;
	mocA.wChannelMask	= mocW.wChannelMask;
	mocA.dwSupport	        = mocW.dwSupport;
	memcpy(lpCaps, &mocA, min(uSize, sizeof(mocA)));
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.@]
 * 				midiInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    UINT	ret;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else
    {
        LPWSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize * sizeof(WCHAR));
        if (!xstr) ret = MMSYSERR_NOMEM;
        else
        {
            ret = midiOutGetErrorTextW(uError, xstr, uSize);
            if (ret == MMSYSERR_NOERROR)
                WideCharToMultiByte(CP_ACP, 0, xstr, -1, lpText, uSize, NULL, NULL);
            HeapFree(GetProcessHeap(), 0, xstr);
        }
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.@]
 * 				midiInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    UINT        ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= MIDIERR_BASE  && uError <= MIDIERR_LASTERROR)) {
	if (LoadStringW(WINMM_IData.hWinMM32Instance,
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				MIDI_OutAlloc    		[internal]
 */
static	LPWINE_MIDI	MIDI_OutAlloc(HMIDIOUT* lphMidiOut, LPDWORD lpdwCallback,
				      LPDWORD lpdwInstance, LPDWORD lpdwFlags,
				      DWORD cIDs, MIDIOPENSTRMID* lpIDs, BOOL bFrom32)
{
    HANDLE	      	hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		size;

    size = sizeof(WINE_MIDI) + (cIDs ? (cIDs-1) : 0) * sizeof(MIDIOPENSTRMID);

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(size, MMDRV_MIDIOUT, &hMidiOut, lpdwFlags,
				    lpdwCallback, lpdwInstance, bFrom32);

    if (lphMidiOut != NULL)
	*lphMidiOut = hMidiOut;

    if (lpwm) {
	lpwm->mod.hMidi = (HMIDI) hMidiOut;
	lpwm->mod.dwCallback = *lpdwCallback;
	lpwm->mod.dwInstance = *lpdwInstance;
	lpwm->mod.dnDevNode = 0;
	lpwm->mod.cIds = cIDs;
	if (cIDs)
	    memcpy(&(lpwm->mod.rgIds), lpIDs, cIDs * sizeof(MIDIOPENSTRMID));
    }
    return lpwm;
}

UINT MIDI_OutOpen(LPHMIDIOUT lphMidiOut, UINT uDeviceID, DWORD_PTR dwCallback,
                  DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32)
{
    HMIDIOUT		hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		dwRet = 0;

    TRACE("(%p, %d, %08lX, %08lX, %08lX);\n",
	  lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiOut != NULL) *lphMidiOut = 0;

    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &dwFlags,
			 0, NULL, bFrom32);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mld.uDeviceID = uDeviceID;

    dwRet = MMDRV_Open((LPWINE_MLD)lpwm, MODM_OPEN, (DWORD)&lpwm->mod, dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiOut, (LPWINE_MLD)lpwm);
	hMidiOut = 0;
    }

    if (lphMidiOut) *lphMidiOut = hMidiOut;
    TRACE("=> %d hMidi=%p\n", dwRet, hMidiOut);

    return dwRet;
}

/**************************************************************************
 * 				midiOutOpen    		[WINMM.@]
 */
MMRESULT WINAPI midiOutOpen(LPHMIDIOUT lphMidiOut, UINT_PTR uDeviceID,
                       DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    return MIDI_OutOpen(lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				midiOutClose		[WINMM.@]
 */
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MODM_CLOSE);
    MMDRV_Free(hMidiOut, wmld);

    return dwRet;
}

/**************************************************************************
 * 				midiOutPrepareHeader	[WINMM.@]
 */
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
				 MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if (lpMidiOutHdr == NULL || uSize < sizeof (MIDIHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_PREPARE, (DWORD_PTR)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.@]
 */
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
				   MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if (lpMidiOutHdr == NULL || uSize < sizeof (MIDIHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, (DWORD_PTR)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.@]
 */
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lX)\n", hMidiOut, dwMsg);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_DATA, dwMsg, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutLongMsg		[WINMM.@]
 */
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
			   MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD_PTR)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.@]
 */
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutGetVolume	[WINMM.@]
 */
UINT WINAPI midiOutGetVolume(HMIDIOUT hMidiOut, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p);\n", hMidiOut, lpdwVolume);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_GETVOLUME, (DWORD_PTR)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutSetVolume	[WINMM.@]
 */
UINT WINAPI midiOutSetVolume(HMIDIOUT hMidiOut, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %ld);\n", hMidiOut, dwVolume);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutCachePatches		[WINMM.@]
 */
UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut, UINT uBank,
				WORD* lpwPatchArray, UINT uFlags)
{
    /* not really necessary to support this */
    FIXME("not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.@]
 */
UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT uPatch,
				    WORD* lpwKeyArray, UINT uFlags)
{
    FIXME("not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.@]
 */
UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p)\n", hMidiOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.@]
 */
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage,
                           DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %04X, %08lX, %08lX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) {
	/* HACK... */
	if (uMessage == 0x0001) {
	    *(LPDWORD)dwParam1 = 1;
	    return 0;
	}
	if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.@]
 */
UINT WINAPI midiInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIIN);
}

/**************************************************************************
 * 				midiInGetDevCapsW	[WINMM.@]
 */
MMRESULT WINAPI midiInGetDevCapsW(UINT_PTR uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%d, %p, %d);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

   return MMDRV_Message(wmld, MIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				midiInGetDevCapsA	[WINMM.@]
 */
MMRESULT WINAPI midiInGetDevCapsA(UINT_PTR uDeviceID, LPMIDIINCAPSA lpCaps, UINT uSize)
{
    MIDIINCAPSW		micW;
    UINT		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = midiInGetDevCapsW(uDeviceID, &micW, sizeof(micW));

    if (ret == MMSYSERR_NOERROR) {
	MIDIINCAPSA micA;
	micA.wMid           = micW.wMid;
	micA.wPid           = micW.wPid;
	micA.vDriverVersion = micW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, micW.szPname, -1, micA.szPname,
                             sizeof(micA.szPname), NULL, NULL );
	micA.dwSupport      = micW.dwSupport;
	memcpy(lpCaps, &micA, min(uSize, sizeof(micA)));
    }
    return ret;
}

UINT MIDI_InOpen(HMIDIIN* lphMidiIn, UINT uDeviceID, DWORD_PTR dwCallback,
                 DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32)
{
    HANDLE		hMidiIn;
    LPWINE_MIDI		lpwm;
    DWORD		dwRet = 0;

    TRACE("(%p, %d, %08lX, %08lX, %08lX);\n",
	  lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiIn != NULL) *lphMidiIn = 0;

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(sizeof(WINE_MIDI), MMDRV_MIDIIN, &hMidiIn,
				    &dwFlags, &dwCallback, &dwInstance, bFrom32);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mod.hMidi = (HMIDI) hMidiIn;
    lpwm->mod.dwCallback = dwCallback;
    lpwm->mod.dwInstance = dwInstance;

    lpwm->mld.uDeviceID = uDeviceID;
    dwRet = MMDRV_Open(&lpwm->mld, MIDM_OPEN, (DWORD)&lpwm->mod, dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiIn, &lpwm->mld);
	hMidiIn = 0;
    }
    if (lphMidiIn != NULL) *lphMidiIn = hMidiIn;
    TRACE("=> %ld hMidi=%p\n", dwRet, hMidiIn);

    return dwRet;
}

/**************************************************************************
 * 				midiInOpen		[WINMM.@]
 */
MMRESULT WINAPI midiInOpen(HMIDIIN* lphMidiIn, UINT_PTR uDeviceID,
		       DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    return MIDI_InOpen(lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				midiInClose		[WINMM.@]
 */
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MIDM_CLOSE);
    MMDRV_Free(hMidiIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				midiInPrepareHeader	[WINMM.@]
 */
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
				MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if (lpMidiInHdr == NULL || uSize < sizeof (MIDIHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, (DWORD_PTR)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[WINMM.@]
 */
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
				  MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if (lpMidiInHdr == NULL || uSize < sizeof (MIDIHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, (DWORD_PTR)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInAddBuffer		[WINMM.@]
 */
UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
			    MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD_PTR)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInStart			[WINMM.@]
 */
UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_START, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInStop			[WINMM.@]
 */
UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_STOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInReset			[WINMM.@]
 */
UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInGetID			[WINMM.@]
 */
UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p)\n", hMidiIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[WINMM.@]
 */
DWORD WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %04X, %08lX, %08lX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MIDM_OPEN:
    case MIDM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

typedef struct WINE_MIDIStream {
    HMIDIOUT			hDevice;
    HANDLE			hThread;
    DWORD			dwThreadID;
    DWORD			dwTempo;
    DWORD			dwTimeDiv;
    DWORD			dwPositionMS;
    DWORD			dwPulses;
    DWORD			dwStartTicks;
    WORD			wFlags;
    HANDLE			hEvent;
    LPMIDIHDR			lpMidiHdr;
} WINE_MIDIStream;

#define WINE_MSM_HEADER		(WM_USER+0)
#define WINE_MSM_STOP		(WM_USER+1)

/**************************************************************************
 * 				MMSYSTEM_GetMidiStream		[internal]
 */
static	BOOL	MMSYSTEM_GetMidiStream(HMIDISTRM hMidiStrm, WINE_MIDIStream** lpMidiStrm, WINE_MIDI** lplpwm)
{
    WINE_MIDI* lpwm = (LPWINE_MIDI)MMDRV_Get(hMidiStrm, MMDRV_MIDIOUT, FALSE);

    if (lplpwm)
	*lplpwm = lpwm;

    if (lpwm == NULL) {
	return FALSE;
    }

    *lpMidiStrm = (WINE_MIDIStream*)lpwm->mod.rgIds.dwStreamID;

    return *lpMidiStrm != NULL;
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Convert	[internal]
 */
static	DWORD	MMSYSTEM_MidiStream_Convert(WINE_MIDIStream* lpMidiStrm, DWORD pulse)
{
    DWORD	ret = 0;

    if (lpMidiStrm->dwTimeDiv == 0) {
	FIXME("Shouldn't happen. lpMidiStrm->dwTimeDiv = 0\n");
    } else if (lpMidiStrm->dwTimeDiv > 0x8000) { /* SMPTE, unchecked FIXME? */
	int	nf = -(char)HIBYTE(lpMidiStrm->dwTimeDiv);	/* number of frames     */
	int	nsf = LOBYTE(lpMidiStrm->dwTimeDiv);		/* number of sub-frames */
	ret = (pulse * 1000) / (nf * nsf);
    } else {
	ret = (DWORD)((double)pulse * ((double)lpMidiStrm->dwTempo / 1000) /
		      (double)lpMidiStrm->dwTimeDiv);
    }

    return ret;
}

/**************************************************************************
 * 			MMSYSTEM_MidiStream_MessageHandler	[internal]
 */
static	BOOL	MMSYSTEM_MidiStream_MessageHandler(WINE_MIDIStream* lpMidiStrm, LPWINE_MIDI lpwm, LPMSG msg)
{
    LPMIDIHDR	lpMidiHdr;
    LPMIDIHDR*	lpmh;
    LPBYTE	lpData;

    switch (msg->message) {
    case WM_QUIT:
	SetEvent(lpMidiStrm->hEvent);
	return FALSE;
    case WINE_MSM_STOP:
	TRACE("STOP\n");
	/* this is not quite what MS doc says... */
	midiOutReset(lpMidiStrm->hDevice);
	/* empty list of already submitted buffers */
	for (lpMidiHdr = lpMidiStrm->lpMidiHdr; lpMidiHdr; lpMidiHdr = (LPMIDIHDR)lpMidiHdr->lpNext) {
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;

	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	}
	lpMidiStrm->lpMidiHdr = 0;
	SetEvent(lpMidiStrm->hEvent);
	break;
    case WINE_MSM_HEADER:
	/* sets initial tick count for first MIDIHDR */
	if (!lpMidiStrm->dwStartTicks)
	    lpMidiStrm->dwStartTicks = GetTickCount();

	/* FIXME(EPP): "I don't understand the content of the first MIDIHDR sent
	 * by native mcimidi, it doesn't look like a correct one".
	 * this trick allows to throw it away... but I don't like it.
	 * It looks like part of the file I'm trying to play and definitively looks
	 * like raw midi content
	 * I'd really like to understand why native mcimidi sends it. Perhaps a bad
	 * synchronization issue where native mcimidi is still processing raw MIDI
	 * content before generating MIDIEVENTs ?
	 *
	 * 4c 04 89 3b 00 81 7c 99 3b 43 00 99 23 5e 04 89 L..;..|.;C..#^..
	 * 3b 00 00 89 23 00 7c 99 3b 45 00 99 28 62 04 89 ;...#.|.;E..(b..
	 * 3b 00 00 89 28 00 81 7c 99 3b 4e 00 99 23 5e 04 ;...(..|.;N..#^.
	 * 89 3b 00 00 89 23 00 7c 99 3b 45 00 99 23 78 04 .;...#.|.;E..#x.
	 * 89 3b 00 00 89 23 00 81 7c 99 3b 48 00 99 23 5e .;...#..|.;H..#^
	 * 04 89 3b 00 00 89 23 00 7c 99 3b 4e 00 99 28 62 ..;...#.|.;N..(b
	 * 04 89 3b 00 00 89 28 00 81 7c 99 39 4c 00 99 23 ..;...(..|.9L..#
	 * 5e 04 89 39 00 00 89 23 00 82 7c 99 3b 4c 00 99 ^..9...#..|.;L..
	 * 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b 48 00 99 #^..;...#.|.;H..
	 * 28 62 04 89 3b 00 00 89 28 00 81 7c 99 3b 3f 04 (b..;...(..|.;?.
	 * 89 3b 00 1c 99 23 5e 04 89 23 00 5c 99 3b 45 00 .;...#^..#.\.;E.
	 * 99 23 78 04 89 3b 00 00 89 23 00 81 7c 99 3b 46 .#x..;...#..|.;F
	 * 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b 48 ..#^..;...#.|.;H
	 * 00 99 28 62 04 89 3b 00 00 89 28 00 81 7c 99 3b ..(b..;...(..|.;
	 * 46 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b F..#^..;...#.|.;
	 * 48 00 99 23 78 04 89 3b 00 00 89 23 00 81 7c 99 H..#x..;...#..|.
	 * 3b 4c 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 ;L..#^..;...#.|.
	 */
	lpMidiHdr = (LPMIDIHDR)msg->lParam;
	lpData = lpMidiHdr->lpData;
	TRACE("Adding %s lpMidiHdr=%p [lpData=0x%08lx dwBufferLength=%lu/%lu dwFlags=0x%08lx size=%u]\n",
	      (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular", lpMidiHdr,
	      (DWORD)lpMidiHdr, lpMidiHdr->dwBufferLength, lpMidiHdr->dwBytesRecorded,
	      lpMidiHdr->dwFlags, msg->wParam);
#if 0
	/* dumps content of lpMidiHdr->lpData
	 * FIXME: there should be a debug routine somewhere that already does this
	 * I hate spreading this type of shit all around the code
	 */
	for (dwToGo = 0; dwToGo < lpMidiHdr->dwBufferLength; dwToGo += 16) {
	    DWORD	i;
	    BYTE	ch;

	    for (i = 0; i < min(16, lpMidiHdr->dwBufferLength - dwToGo); i++)
		printf("%02x ", lpData[dwToGo + i]);
	    for (; i < 16; i++)
		printf("   ");
	    for (i = 0; i < min(16, lpMidiHdr->dwBufferLength - dwToGo); i++) {
		ch = lpData[dwToGo + i];
		printf("%c", (ch >= 0x20 && ch <= 0x7F) ? ch : '.');
	    }
	    printf("\n");
	}
#endif
	if (((LPMIDIEVENT)lpData)->dwStreamID != 0 &&
	    ((LPMIDIEVENT)lpData)->dwStreamID != 0xFFFFFFFF &&
	    ((LPMIDIEVENT)lpData)->dwStreamID != (DWORD)lpMidiStrm) {
	    FIXME("Dropping bad %s lpMidiHdr (streamID=%08lx)\n",
		  (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular",
		  ((LPMIDIEVENT)lpData)->dwStreamID);
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;

	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	    break;
	}

	for (lpmh = &lpMidiStrm->lpMidiHdr; *lpmh; lpmh = (LPMIDIHDR*)&((*lpmh)->lpNext));
	*lpmh = lpMidiHdr;
	lpMidiHdr = (LPMIDIHDR)msg->lParam;
	lpMidiHdr->lpNext = 0;
	lpMidiHdr->dwFlags |= MHDR_INQUEUE;
	lpMidiHdr->dwFlags &= ~MHDR_DONE;
	lpMidiHdr->dwOffset = 0;

	break;
    default:
	FIXME("Unknown message %d\n", msg->message);
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Player	[internal]
 */
static	DWORD	CALLBACK	MMSYSTEM_MidiStream_Player(LPVOID pmt)
{
    WINE_MIDIStream* 	lpMidiStrm = pmt;
    WINE_MIDI*		lpwm;
    MSG			msg;
    DWORD		dwToGo;
    DWORD		dwCurrTC;
    LPMIDIHDR		lpMidiHdr;
    LPMIDIEVENT 	me;
    LPBYTE		lpData = 0;

    TRACE("(%p)!\n", lpMidiStrm);

    if (!lpMidiStrm ||
	(lpwm = (LPWINE_MIDI)MMDRV_Get(lpMidiStrm->hDevice, MMDRV_MIDIOUT, FALSE)) == NULL)
	goto the_end;

    /* force thread's queue creation */
    /* Used to be InitThreadInput16(0, 5); */
    /* but following works also with hack in midiStreamOpen */
    PeekMessageA(&msg, 0, 0, 0, 0);

    /* FIXME: this next line must be called before midiStreamOut or midiStreamRestart are called */
    SetEvent(lpMidiStrm->hEvent);
    TRACE("Ready to go 1\n");
    /* thread is started in paused mode */
    SuspendThread(lpMidiStrm->hThread);
    TRACE("Ready to go 2\n");

    lpMidiStrm->dwStartTicks = 0;
    lpMidiStrm->dwPulses = 0;

    lpMidiStrm->lpMidiHdr = 0;

    for (;;) {
	lpMidiHdr = lpMidiStrm->lpMidiHdr;
	if (!lpMidiHdr) {
	    /* for first message, block until one arrives, then process all that are available */
	    GetMessageA(&msg, 0, 0, 0);
	    do {
		if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
		    goto the_end;
	    } while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
	    lpData = 0;
	    continue;
	}

	if (!lpData)
	    lpData = lpMidiHdr->lpData;

	me = (LPMIDIEVENT)(lpData + lpMidiHdr->dwOffset);

	/* do we have to wait ? */
	if (me->dwDeltaTime) {
	    lpMidiStrm->dwPositionMS += MMSYSTEM_MidiStream_Convert(lpMidiStrm, me->dwDeltaTime);
	    lpMidiStrm->dwPulses += me->dwDeltaTime;

	    dwToGo = lpMidiStrm->dwStartTicks + lpMidiStrm->dwPositionMS;

	    TRACE("%ld/%ld/%ld\n", dwToGo, GetTickCount(), me->dwDeltaTime);
	    while ((dwCurrTC = GetTickCount()) < dwToGo) {
		if (MsgWaitForMultipleObjects(0, NULL, FALSE, dwToGo - dwCurrTC, QS_ALLINPUT) == WAIT_OBJECT_0) {
		    /* got a message, handle it */
		    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
			    goto the_end;
		    }
		    lpData = 0;
		} else {
		    /* timeout, so me->dwDeltaTime is elapsed, can break the while loop */
		    break;
		}
	    }
	}
	switch (MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK)) {
	case MEVT_COMMENT:
	    FIXME("NIY: MEVT_COMMENT\n");
	    /* do nothing, skip bytes */
	    break;
	case MEVT_LONGMSG:
	    FIXME("NIY: MEVT_LONGMSG, aka sending Sysex event\n");
	    break;
	case MEVT_NOP:
	    break;
	case MEVT_SHORTMSG:
	    midiOutShortMsg(lpMidiStrm->hDevice, MEVT_EVENTPARM(me->dwEvent));
	    break;
	case MEVT_TEMPO:
	    lpMidiStrm->dwTempo = MEVT_EVENTPARM(me->dwEvent);
	    break;
	case MEVT_VERSION:
	    break;
	default:
	    FIXME("Unknown MEVT (0x%02x)\n", MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK));
	    break;
	}
	if (me->dwEvent & MEVT_F_CALLBACK) {
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_POSITIONCB,
			   lpwm->mod.dwInstance, (LPARAM)lpMidiHdr, 0L);
	}
	lpMidiHdr->dwOffset += sizeof(MIDIEVENT) - sizeof(me->dwParms);
	if (me->dwEvent & MEVT_F_LONG)
	    lpMidiHdr->dwOffset += (MEVT_EVENTPARM(me->dwEvent) + 3) & ~3;
	if (lpMidiHdr->dwOffset >= lpMidiHdr->dwBufferLength) {
	    /* done with this header */
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;

	    lpMidiStrm->lpMidiHdr = (LPMIDIHDR)lpMidiHdr->lpNext;
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	    lpData = 0;
	}
    }
the_end:
    TRACE("End of thread\n");
    ExitThread(0);
    return 0;	/* for removing the warning, never executed */
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_PostMessage	[internal]
 */
static	BOOL MMSYSTEM_MidiStream_PostMessage(WINE_MIDIStream* lpMidiStrm, WORD msg, DWORD pmt1, DWORD pmt2)
{
    if (PostThreadMessageA(lpMidiStrm->dwThreadID, msg, pmt1, pmt2)) {
	DWORD	count;

	if (pFnReleaseThunkLock) pFnReleaseThunkLock(&count);
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	if (pFnRestoreThunkLock) pFnRestoreThunkLock(count);
    } else {
	WARN("bad PostThreadMessageA\n");
	return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 * 				midiStreamClose			[WINMM.@]
 */
MMRESULT WINAPI midiStreamClose(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL))
	return MMSYSERR_INVALHANDLE;

    midiStreamStop(hMidiStrm);
    MMSYSTEM_MidiStream_PostMessage(lpMidiStrm, WM_QUIT, 0, 0);
    HeapFree(GetProcessHeap(), 0, lpMidiStrm);
    CloseHandle(lpMidiStrm->hEvent);

    return midiOutClose((HMIDIOUT)hMidiStrm);
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Open	[internal]
 */
MMRESULT MIDI_StreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID, DWORD cMidi,
                         DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen,
                         BOOL bFrom32)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret;
    MIDIOPENSTRMID	mosm;
    LPWINE_MIDI		lpwm;
    HMIDIOUT		hMidiOut;

    TRACE("(%p, %p, %ld, 0x%08lx, 0x%08lx, 0x%08lx)!\n",
	  lphMidiStrm, lpuDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);

    if (cMidi != 1 || lphMidiStrm == NULL || lpuDeviceID == NULL)
	return MMSYSERR_INVALPARAM;

    lpMidiStrm = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_MIDIStream));
    if (!lpMidiStrm)
	return MMSYSERR_NOMEM;

    lpMidiStrm->dwTempo = 500000;
    lpMidiStrm->dwTimeDiv = 480; 	/* 480 is 120 quater notes per minute *//* FIXME ??*/
    lpMidiStrm->dwPositionMS = 0;

    mosm.dwStreamID = (DWORD)lpMidiStrm;
    /* FIXME: the correct value is not allocated yet for MAPPER */
    mosm.wDeviceID  = *lpuDeviceID;
    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &fdwOpen, 1, &mosm, bFrom32);
    lpMidiStrm->hDevice = hMidiOut;
    if (lphMidiStrm)
	*lphMidiStrm = (HMIDISTRM)hMidiOut;

    lpwm->mld.uDeviceID = *lpuDeviceID;

    ret = MMDRV_Open(&lpwm->mld, MODM_OPEN, (DWORD)&lpwm->mod, fdwOpen);
    lpMidiStrm->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    lpMidiStrm->wFlags = HIWORD(fdwOpen);

    lpMidiStrm->hThread = CreateThread(NULL, 0, MMSYSTEM_MidiStream_Player,
				       lpMidiStrm, 0, &(lpMidiStrm->dwThreadID));

    if (!lpMidiStrm->hThread) {
	midiStreamClose((HMIDISTRM)hMidiOut);
	return MMSYSERR_NOMEM;
    }
    SetThreadPriority(lpMidiStrm->hThread, THREAD_PRIORITY_TIME_CRITICAL);

    /* wait for thread to have started, and for its queue to be created */
    {
	DWORD	count;

	/* (Release|Restore)ThunkLock() is needed when this method is called from 16 bit code,
	 * (meaning the Win16Lock is set), so that it's released and the 32 bit thread running
	 * MMSYSTEM_MidiStreamPlayer can acquire Win16Lock to create its queue.
	 */
	if (pFnReleaseThunkLock) pFnReleaseThunkLock(&count);
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	if (pFnRestoreThunkLock) pFnRestoreThunkLock(count);
    }

    TRACE("=> (%u/%d) hMidi=%p ret=%d lpMidiStrm=%p\n",
	  *lpuDeviceID, lpwm->mld.uDeviceID, *lphMidiStrm, ret, lpMidiStrm);
    return ret;
}

/**************************************************************************
 * 				midiStreamOpen			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID,
			       DWORD cMidi, DWORD_PTR dwCallback,
			       DWORD_PTR dwInstance, DWORD fdwOpen)
{
    return MIDI_StreamOpen(lphMidiStrm, lpuDeviceID, cMidi, dwCallback,
                           dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				midiStreamOut			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOut(HMIDISTRM hMidiStrm, LPMIDIHDR lpMidiHdr,
			      UINT cbMidiHdr)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %u)!\n", hMidiStrm, lpMidiHdr, cbMidiHdr);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (!lpMidiHdr) {
        ret = MMSYSERR_INVALPARAM;
    } else {
	if (!PostThreadMessageA(lpMidiStrm->dwThreadID,
                                WINE_MSM_HEADER, cbMidiHdr,
                                (DWORD)lpMidiHdr)) {
	    WARN("bad PostThreadMessageA\n");
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamPause			[WINMM.@]
 */
MMRESULT WINAPI midiStreamPause(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	if (SuspendThread(lpMidiStrm->hThread) == 0xFFFFFFFF) {
	    WARN("bad Suspend (%ld)\n", GetLastError());
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamPosition		[WINMM.@]
 */
MMRESULT WINAPI midiStreamPosition(HMIDISTRM hMidiStrm, LPMMTIME lpMMT, UINT cbmmt)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %u)!\n", hMidiStrm, lpMMT, cbmmt);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (lpMMT == NULL || cbmmt != sizeof(MMTIME)) {
	ret = MMSYSERR_INVALPARAM;
    } else {
	switch (lpMMT->wType) {
	case TIME_MS:
	    lpMMT->u.ms = lpMidiStrm->dwPositionMS;
	    TRACE("=> %ld ms\n", lpMMT->u.ms);
	    break;
	case TIME_TICKS:
	    lpMMT->u.ticks = lpMidiStrm->dwPulses;
	    TRACE("=> %ld ticks\n", lpMMT->u.ticks);
	    break;
	default:
	    WARN("Unsupported time type %d\n", lpMMT->wType);
	    lpMMT->wType = TIME_MS;
	    ret = MMSYSERR_INVALPARAM;
	    break;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[WINMM.@]
 */
MMRESULT WINAPI midiStreamProperty(HMIDISTRM hMidiStrm, LPBYTE lpPropData, DWORD dwProperty)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %lx)\n", hMidiStrm, lpPropData, dwProperty);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if ((dwProperty & (MIDIPROP_GET|MIDIPROP_SET)) == 0) {
	ret = MMSYSERR_INVALPARAM;
    } else if (dwProperty & MIDIPROP_TEMPO) {
	MIDIPROPTEMPO*	mpt = (MIDIPROPTEMPO*)lpPropData;

	if (sizeof(MIDIPROPTEMPO) != mpt->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    lpMidiStrm->dwTempo = mpt->dwTempo;
	    TRACE("Setting tempo to %ld\n", mpt->dwTempo);
	} else if (dwProperty & MIDIPROP_GET) {
	    mpt->dwTempo = lpMidiStrm->dwTempo;
	    TRACE("Getting tempo <= %ld\n", mpt->dwTempo);
	}
    } else if (dwProperty & MIDIPROP_TIMEDIV) {
	MIDIPROPTIMEDIV*	mptd = (MIDIPROPTIMEDIV*)lpPropData;

	if (sizeof(MIDIPROPTIMEDIV) != mptd->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    lpMidiStrm->dwTimeDiv = mptd->dwTimeDiv;
	    TRACE("Setting time div to %ld\n", mptd->dwTimeDiv);
	} else if (dwProperty & MIDIPROP_GET) {
	    mptd->dwTimeDiv = lpMidiStrm->dwTimeDiv;
	    TRACE("Getting time div <= %ld\n", mptd->dwTimeDiv);
	}
    } else {
	ret = MMSYSERR_INVALPARAM;
    }

    return ret;
}

/**************************************************************************
 * 				midiStreamRestart		[WINMM.@]
 */
MMRESULT WINAPI midiStreamRestart(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	DWORD	ret;

	/* since we increase the thread suspend count on each midiStreamPause
	 * there may be a need for several midiStreamResume
	 */
	do {
	    ret = ResumeThread(lpMidiStrm->hThread);
	} while (ret != 0xFFFFFFFF && ret != 0);
	if (ret == 0xFFFFFFFF) {
	    WARN("bad Resume (%ld)\n", GetLastError());
	    ret = MMSYSERR_ERROR;
	} else {
	    lpMidiStrm->dwStartTicks = GetTickCount() - lpMidiStrm->dwPositionMS;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamStop			[WINMM.@]
 */
MMRESULT WINAPI midiStreamStop(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	/* in case stream has been paused... FIXME is the current state correct ? */
	midiStreamRestart(hMidiStrm);
	MMSYSTEM_MidiStream_PostMessage(lpMidiStrm, WINE_MSM_STOP, 0, 0);
    }
    return ret;
}

UINT WAVE_Open(HANDLE* lphndl, UINT uDeviceID, UINT uType,
               LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
               DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32)
{
    HANDLE		handle;
    LPWINE_MLD		wmld;
    DWORD		dwRet = MMSYSERR_NOERROR;
    WAVEOPENDESC	wod;

    TRACE("(%p, %d, %s, %p, %08lX, %08lX, %08lX, %d);\n",
	  lphndl, (int)uDeviceID, (uType==MMDRV_WAVEOUT)?"Out":"In", lpFormat, dwCallback,
	  dwInstance, dwFlags, bFrom32?32:16);

    if (dwFlags & WAVE_FORMAT_QUERY)
        TRACE("WAVE_FORMAT_QUERY requested !\n");

    if (lpFormat == NULL) {
        WARN("bad format\n");
        return WAVERR_BADFORMAT;
    }

    if ((dwFlags & WAVE_MAPPED) && (uDeviceID == (UINT)-1)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    /* may have a PCMWAVEFORMAT rather than a WAVEFORMATEX so don't read cbSize */
    TRACE("wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u\n",
	  lpFormat->wFormatTag, lpFormat->nChannels, lpFormat->nSamplesPerSec,
	  lpFormat->nAvgBytesPerSec, lpFormat->nBlockAlign, lpFormat->wBitsPerSample);

    if ((wmld = MMDRV_Alloc(sizeof(WINE_WAVE), uType, &handle,
			    &dwFlags, &dwCallback, &dwInstance, bFrom32)) == NULL) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    wod.hWave = handle;
    wod.lpFormat = (LPWAVEFORMATEX)lpFormat;  /* should the struct be copied iso pointer? */
    wod.dwCallback = dwCallback;
    wod.dwInstance = dwInstance;
    wod.dnDevNode = 0L;

    TRACE("cb=%08lx\n", wod.dwCallback);

    for (;;) {
        if (dwFlags & WAVE_MAPPED) {
            wod.uMappedDeviceID = uDeviceID;
            uDeviceID = WAVE_MAPPER;
        } else {
            wod.uMappedDeviceID = -1;
        }
        wmld->uDeviceID = uDeviceID;

        dwRet = MMDRV_Open(wmld, (uType == MMDRV_WAVEOUT) ? WODM_OPEN : WIDM_OPEN,
                           (DWORD)&wod, dwFlags);

        TRACE("dwRet = %s\n", WINMM_ErrorToString(dwRet));
        if (dwRet != WAVERR_BADFORMAT ||
            ((dwFlags & (WAVE_MAPPED|WAVE_FORMAT_DIRECT)) != 0) || (uDeviceID == WAVE_MAPPER)) break;
        /* if we ask for a format which isn't supported by the physical driver,
         * let's try to map it through the wave mapper (except, if we already tried
         * or user didn't allow us to use acm codecs or the device is already the mapper)
         */
        dwFlags |= WAVE_MAPPED;
        /* we shall loop only one */
    }

    if ((dwFlags & WAVE_FORMAT_QUERY) || dwRet != MMSYSERR_NOERROR) {
        MMDRV_Free(handle, wmld);
        handle = 0;
    }

    if (lphndl != NULL) *lphndl = handle;
    TRACE("=> %s hWave=%p\n", WINMM_ErrorToString(dwRet), handle);

    return dwRet;
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.@]
 */
UINT WINAPI waveOutGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_WAVEOUT);
}

/**************************************************************************
 * 				waveOutGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA lpCaps,
			       UINT uSize)
{
    WAVEOUTCAPSW	wocW;
    UINT 		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = waveOutGetDevCapsW(uDeviceID, &wocW, sizeof(wocW));

    if (ret == MMSYSERR_NOERROR) {
	WAVEOUTCAPSA wocA;
	wocA.wMid           = wocW.wMid;
	wocA.wPid           = wocW.wPid;
	wocA.vDriverVersion = wocW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, wocW.szPname, -1, wocA.szPname,
                             sizeof(wocA.szPname), NULL, NULL );
	wocA.dwFormats      = wocW.dwFormats;
	wocA.wChannels      = wocW.wChannels;
	wocA.dwSupport      = wocW.dwSupport;
	memcpy(lpCaps, &wocA, min(uSize, sizeof(wocA)));
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetDevCapsW		[WINMM.@]
 */
UINT WINAPI waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW lpCaps,
			       UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_WAVEOUT, TRUE)) == NULL)
        return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, WODM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);

}

/**************************************************************************
 * 				waveOutGetErrorTextA 	[WINMM.@]
 * 				waveInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    UINT	ret;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else
    {
        LPWSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize * sizeof(WCHAR));
        if (!xstr) ret = MMSYSERR_NOMEM;
        else
        {
            ret = waveOutGetErrorTextW(uError, xstr, uSize);
            if (ret == MMSYSERR_NOERROR)
                WideCharToMultiByte(CP_ACP, 0, xstr, -1, lpText, uSize, NULL, NULL);
            HeapFree(GetProcessHeap(), 0, xstr);
        }
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.@]
 * 				waveInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    UINT        ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= WAVERR_BASE  && uError <= WAVERR_LASTERROR)) {
	if (LoadStringW(WINMM_IData.hWinMM32Instance,
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.@]
 * All the args/structs have the same layout as the win16 equivalents
 */
MMRESULT WINAPI waveOutOpen(LPHWAVEOUT lphWaveOut, UINT uDeviceID,
                       LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                       DWORD_PTR dwInstance, DWORD dwFlags)
{
    return WAVE_Open((HANDLE*)lphWaveOut, uDeviceID, MMDRV_WAVEOUT, lpFormat,
                     dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				waveOutClose		[WINMM.@]
 */
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, WODM_CLOSE);
    if (dwRet != WAVERR_STILLPLAYING)
	MMDRV_Free(hWaveOut, wmld);

    return dwRet;
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.@]
 */
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
				 WAVEHDR* lpWaveOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT		result;

    TRACE("(%p, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WODM_PREPARE, (DWORD_PTR)lpWaveOutHdr,
                                uSize, TRUE)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveOutHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveOutHdr->dwFlags |= WHDR_PREPARED;
    lpWaveOutHdr->dwFlags &= ~WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
				   LPWAVEHDR lpWaveOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT		result;

    TRACE("(%p, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WODM_UNPREPARE, (DWORD_PTR)lpWaveOutHdr,
                                uSize, TRUE)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveOutHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;

    lpWaveOutHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveOutHdr->dwFlags |= WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveOutWrite		[WINMM.@]
 */
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, LPWAVEHDR lpWaveOutHdr,
			 UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_WRITE, (DWORD_PTR)lpWaveOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutBreakLoop	[WINMM.@]
 */
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_BREAKLOOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutPause		[WINMM.@]
 */
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_PAUSE, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutReset		[WINMM.@]
 */
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutRestart		[WINMM.@]
 */
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_RESTART, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.@]
 */
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, LPMMTIME lpTime,
			       UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveOut, lpTime, uSize);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_GETPOS, (DWORD_PTR)lpTime, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutGetPitch		[WINMM.@]
 */
UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, (DWORD)lpdw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_GETPITCH, (DWORD_PTR)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetPitch		[WINMM.@]
 */
UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, (DWORD)dw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_SETPITCH, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, (DWORD)lpdw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_GETPLAYBACKRATE, (DWORD_PTR)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, (DWORD)dw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_SETPLAYBACKRATE, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetVolume	[WINMM.@]
 */
UINT WINAPI waveOutGetVolume(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, (DWORD)lpdw);

     if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, TRUE)) == NULL)
        return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_GETVOLUME, (DWORD_PTR)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetVolume	[WINMM.@]
 */
UINT WINAPI waveOutSetVolume(HWAVEOUT hWaveOut, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lx);\n", hWaveOut, dw);

     if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, TRUE)) == NULL)
        return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_SETVOLUME, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetID		[WINMM.@]
 */
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p);\n", hWaveOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutMessage 		[WINMM.@]
 */
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage,
                           DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %u, %ld, %ld)\n", hWaveOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[WINMM.@]
 */
UINT WINAPI waveInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_WAVEIN);
}

/**************************************************************************
 * 				waveInGetDevCapsW 		[WINMM.@]
 */
MMRESULT WINAPI waveInGetDevCapsW(UINT_PTR uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_WAVEIN, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, WIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.@]
 */
UINT WINAPI waveInGetDevCapsA(UINT_PTR uDeviceID, LPWAVEINCAPSA lpCaps, UINT uSize)
{
    WAVEINCAPSW		wicW;
    UINT		ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = waveInGetDevCapsW(uDeviceID, &wicW, sizeof(wicW));

    if (ret == MMSYSERR_NOERROR) {
	WAVEINCAPSA wicA;
	wicA.wMid           = wicW.wMid;
	wicA.wPid           = wicW.wPid;
	wicA.vDriverVersion = wicW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, wicW.szPname, -1, wicA.szPname,
                             sizeof(wicA.szPname), NULL, NULL );
	wicA.dwFormats      = wicW.dwFormats;
	wicA.wChannels      = wicW.wChannels;
	memcpy(lpCaps, &wicA, min(uSize, sizeof(wicA)));
    }
    return ret;
}

/**************************************************************************
 * 				waveInOpen			[WINMM.@]
 */
MMRESULT WINAPI waveInOpen(HWAVEIN* lphWaveIn, UINT uDeviceID,
		       LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
		       DWORD_PTR dwInstance, DWORD dwFlags)
{
    return WAVE_Open((HANDLE*)lphWaveIn, uDeviceID, MMDRV_WAVEIN, lpFormat,
                     dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				waveInClose			[WINMM.@]
 */
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Message(wmld, WIDM_CLOSE, 0L, 0L, TRUE);
    if (dwRet != WAVERR_STILLPLAYING)
	MMDRV_Free(hWaveIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				waveInPrepareHeader		[WINMM.@]
 */
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr,
				UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT                result;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WIDM_PREPARE, (DWORD_PTR)lpWaveInHdr,
                                uSize, TRUE)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
        return WAVERR_STILLPLAYING;

    lpWaveInHdr->dwFlags |= WHDR_PREPARED;
    lpWaveInHdr->dwFlags &= ~WHDR_DONE;
    lpWaveInHdr->dwBytesRecorded = 0;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr,
				  UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT                result;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED))
	return MMSYSERR_NOERROR;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD_PTR)lpWaveInHdr,
                                uSize, TRUE)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
        return WAVERR_STILLPLAYING;

    lpWaveInHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveInHdr->dwFlags |= WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.@]
 */
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
			    WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD_PTR)lpWaveInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveInReset		[WINMM.@]
 */
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInStart		[WINMM.@]
 */
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_START, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInStop		[WINMM.@]
 */
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld,WIDM_STOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInGetPosition	[WINMM.@]
 */
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, LPMMTIME lpTime,
			      UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpTime, uSize);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_GETPOS, (DWORD_PTR)lpTime, uSize, TRUE);
}

/**************************************************************************
 * 				waveInGetID			[WINMM.@]
 */
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p);\n", hWaveIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInMessage 		[WINMM.@]
 */
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %u, %ld, %ld)\n", hWaveIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;


    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

struct mm_starter
{
    LPTASKCALLBACK      cb;
    DWORD               client;
    HANDLE              event;
};

static DWORD WINAPI mmTaskRun(void* pmt)
{
    struct mm_starter mms;

    memcpy(&mms, pmt, sizeof(struct mm_starter));
    HeapFree(GetProcessHeap(), 0, pmt);
    mms.cb(mms.client);
    if (mms.event) SetEvent(mms.event);
    return 0;
}

/******************************************************************
 *		mmTaskCreate (WINMM.@)
 */
MMRESULT WINAPI mmTaskCreate(LPTASKCALLBACK cb, HANDLE* ph, DWORD client)
{
    HANDLE               hThread;
    HANDLE               hEvent = 0;
    struct mm_starter   *mms;

    mms = HeapAlloc(GetProcessHeap(), 0, sizeof(struct mm_starter));
    if (mms == NULL) return TASKERR_OUTOFMEMORY;

    mms->cb = cb;
    mms->client = client;
    if (ph) hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    mms->event = hEvent;

    hThread = CreateThread(0, 0, mmTaskRun, mms, 0, NULL);
    if (!hThread) {
        HeapFree(GetProcessHeap(), 0, mms);
        if (hEvent) CloseHandle(hEvent);
        return TASKERR_OUTOFMEMORY;
    }
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    if (ph) *ph = hEvent;
    CloseHandle(hThread);
    return 0;
}

/******************************************************************
 *		mmTaskBlock (WINMM.@)
 */
void     WINAPI mmTaskBlock(HANDLE tid)
{
    MSG		msg;

    do
    {
	GetMessageA(&msg, 0, 0, 0);
	if (msg.hwnd) DispatchMessageA(&msg);
    } while (msg.message != WM_USER);
}

/******************************************************************
 *		mmTaskSignal (WINMM.@)
 */
BOOL     WINAPI mmTaskSignal(HANDLE tid)
{
    return PostThreadMessageW((DWORD)tid, WM_USER, 0, 0);
}

/******************************************************************
 *		mmTaskYield (WINMM.@)
 */
void     WINAPI mmTaskYield(void) {}

/******************************************************************
 *		mmGetCurrentTask (WINMM.@)
 */
HANDLE   WINAPI mmGetCurrentTask(void)
{
    return (HANDLE)GetCurrentThreadId();
}
