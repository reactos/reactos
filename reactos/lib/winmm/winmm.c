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
#include "wownt32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

/******************************************************************
 *		MyUserYield
 *
 * Internal wrapper to call USER.UserYield16 (in fact through a Wine only export from USER32).
 */
static void MyUserYield(void)
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        FARPROC proc = GetProcAddress( mod, "UserYield16" );
        if (proc) proc();
    }
}

void    (WINAPI *pFnReleaseThunkLock)(DWORD*);
void    (WINAPI *pFnRestoreThunkLock)(DWORD);

/* ========================================================================
 *                   G L O B A L   S E T T I N G S
 * ========================================================================*/

LPWINE_MM_IDATA		WINMM_IData /* = NULL */;

/**************************************************************************
 * 			WINMM_CreateIData			[internal]
 */
static	BOOL	WINMM_CreateIData(HINSTANCE hInstDLL)
{
    WINMM_IData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MM_IDATA));

    if (!WINMM_IData)
	return FALSE;
    WINMM_IData->hWinMM32Instance = hInstDLL;
    InitializeCriticalSection(&WINMM_IData->cs);
    WINMM_IData->cs.DebugInfo = (void*)__FILE__ ": WinMM";
    WINMM_IData->psStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    WINMM_IData->psLastEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    TRACE("Created IData (%p)\n", WINMM_IData);
    return TRUE;
}

/**************************************************************************
 * 			WINMM_DeleteIData			[internal]
 */
static	void WINMM_DeleteIData(void)
{
    if (WINMM_IData) {
	TIME_MMTimeStop();

	/* FIXME: should also free content and resources allocated
	 * inside WINMM_IData */
        CloseHandle(WINMM_IData->psStopEvent);
        CloseHandle(WINMM_IData->psLastEvent);
        DeleteCriticalSection(&WINMM_IData->cs);
	HeapFree(GetProcessHeap(), 0, WINMM_IData);
        WINMM_IData = NULL;
    }
}

/******************************************************************
 *             WINMM_LoadMMSystem
 *
 */
static HANDLE (WINAPI *pGetModuleHandle16)(LPCSTR);
static DWORD (WINAPI *pLoadLibrary16)(LPCSTR);

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
        if (!MCI_Init() || !MMDRV_Init()) {
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
static LPWINE_MIXER MIXER_GetDev(HMIXEROBJ hmix, DWORD dwFlags)
{
    LPWINE_MIXER	lpwm = NULL;

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
	FIXME("Unsupported flag (%08lx)\n", dwFlags & 0xF0000000ul);
	break;
    }
    return lpwm;
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
    LPWINE_MLD	wmld;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIXER, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsW(UINT_PTR uDeviceID, LPMIXERCAPSW lpCaps, UINT uSize)
{
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(uDeviceID, &micA, sizeof(micA));

    if (ret == MMSYSERR_NOERROR) {
	MIXERCAPSW micW;
	micW.wMid           = micA.wMid;
	micW.wPid           = micA.wPid;
	micW.vDriverVersion = micA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, micA.szPname, -1, micW.szPname,
                             sizeof(micW.szPname)/sizeof(WCHAR) );
	micW.fdwSupport     = micA.fdwSupport;
	micW.cDestinations  = micA.cDestinations;
	memcpy(lpCaps, &micW, min(uSize, sizeof(micW)));
    }
    return ret;
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
UINT WINAPI mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD_PTR dwCallback,
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

    TRACE("(%p %p %08lx)\n", hmix, lpid, fdwID);

    if ((lpwm = MIXER_GetDev(hmix, fdwID)) == NULL) {
	return MMSYSERR_INVALHANDLE;
    }

    if (lpid)
      *lpid = lpwm->mld.uDeviceID;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mixerGetControlDetailsA		[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA,
				    DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcdA, fdwDetails);

    if ((lpwm = MIXER_GetDev(hmix, fdwDetails)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if (lpmcdA == NULL || lpmcdA->cbStruct != sizeof(*lpmcdA))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETCONTROLDETAILS, (DWORD_PTR)lpmcdA,
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerGetControlDetailsW	[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails)
{
    DWORD			ret = MMSYSERR_NOTENABLED;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcd, fdwDetails);

    if (lpmcd == NULL || lpmcd->cbStruct != sizeof(*lpmcd))
	return MMSYSERR_INVALPARAM;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	/* can savely use W structure as it is, no string inside */
	ret = mixerGetControlDetailsA(hmix, lpmcd, fdwDetails);
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	{
	    MIXERCONTROLDETAILS_LISTTEXTW *pDetailsW = (MIXERCONTROLDETAILS_LISTTEXTW *)lpmcd->paDetails;
            MIXERCONTROLDETAILS_LISTTEXTA *pDetailsA;
	    int size = max(1, lpmcd->cChannels) * sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
            int i;

	    if (lpmcd->u.cMultipleItems != 0) {
		size *= lpmcd->u.cMultipleItems;
	    }
	    pDetailsA = (MIXERCONTROLDETAILS_LISTTEXTA *)HeapAlloc(GetProcessHeap(), 0, size);
            lpmcd->paDetails = pDetailsA;
            lpmcd->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
	    /* set up lpmcd->paDetails */
	    ret = mixerGetControlDetailsA(hmix, lpmcd, fdwDetails);
	    /* copy from lpmcd->paDetails back to paDetailsW; */
            if(ret == MMSYSERR_NOERROR) {
                for(i=0;i<lpmcd->u.cMultipleItems*lpmcd->cChannels;i++) {
                    pDetailsW->dwParam1 = pDetailsA->dwParam1;
                    pDetailsW->dwParam2 = pDetailsA->dwParam2;
                    MultiByteToWideChar( CP_ACP, 0, pDetailsA->szName, -1,
                                         pDetailsW->szName,
                                         sizeof(pDetailsW->szName)/sizeof(WCHAR) );
                    pDetailsA++;
                    pDetailsW++;
                }
                pDetailsA -= lpmcd->u.cMultipleItems*lpmcd->cChannels;
                pDetailsW -= lpmcd->u.cMultipleItems*lpmcd->cChannels;
            }
	    HeapFree(GetProcessHeap(), 0, pDetailsA);
	    lpmcd->paDetails = pDetailsW;
            lpmcd->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
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
    LPWINE_MIXER	lpwm;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmlcA, fdwControls);

    if ((lpwm = MIXER_GetDev(hmix, fdwControls)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if (lpmlcA == NULL || lpmlcA->cbStruct != sizeof(*lpmlcA))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINECONTROLS, (DWORD_PTR)lpmlcA,
			 fdwControls, TRUE);
}

/**************************************************************************
 * 				mixerGetLineControlsW		[WINMM.@]
 */
UINT WINAPI mixerGetLineControlsW(HMIXEROBJ hmix, LPMIXERLINECONTROLSW lpmlcW,
				  DWORD fdwControls)
{
    MIXERLINECONTROLSA	mlcA;
    DWORD		ret;
    int			i;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmlcW, fdwControls);

    if (lpmlcW == NULL || lpmlcW->cbStruct != sizeof(*lpmlcW) ||
	lpmlcW->cbmxctrl != sizeof(MIXERCONTROLW))
	return MMSYSERR_INVALPARAM;

    mlcA.cbStruct = sizeof(mlcA);
    mlcA.dwLineID = lpmlcW->dwLineID;
    mlcA.u.dwControlID = lpmlcW->u.dwControlID;
    mlcA.u.dwControlType = lpmlcW->u.dwControlType;
    mlcA.cControls = lpmlcW->cControls;
    mlcA.cbmxctrl = sizeof(MIXERCONTROLA);
    mlcA.pamxctrl = HeapAlloc(GetProcessHeap(), 0,
			      mlcA.cControls * mlcA.cbmxctrl);

    ret = mixerGetLineControlsA(hmix, &mlcA, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlcW->dwLineID = mlcA.dwLineID;
	lpmlcW->u.dwControlID = mlcA.u.dwControlID;
	lpmlcW->u.dwControlType = mlcA.u.dwControlType;
	lpmlcW->cControls = mlcA.cControls;

	for (i = 0; i < mlcA.cControls; i++) {
	    lpmlcW->pamxctrl[i].cbStruct = sizeof(MIXERCONTROLW);
	    lpmlcW->pamxctrl[i].dwControlID = mlcA.pamxctrl[i].dwControlID;
	    lpmlcW->pamxctrl[i].dwControlType = mlcA.pamxctrl[i].dwControlType;
	    lpmlcW->pamxctrl[i].fdwControl = mlcA.pamxctrl[i].fdwControl;
	    lpmlcW->pamxctrl[i].cMultipleItems = mlcA.pamxctrl[i].cMultipleItems;
            MultiByteToWideChar( CP_ACP, 0, mlcA.pamxctrl[i].szShortName, -1,
                                 lpmlcW->pamxctrl[i].szShortName,
                                 sizeof(lpmlcW->pamxctrl[i].szShortName)/sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, mlcA.pamxctrl[i].szName, -1,
                                 lpmlcW->pamxctrl[i].szName,
                                 sizeof(lpmlcW->pamxctrl[i].szName)/sizeof(WCHAR) );
	    /* sizeof(lpmlcW->pamxctrl[i].Bounds) ==
	     * sizeof(mlcA.pamxctrl[i].Bounds) */
	    memcpy(&lpmlcW->pamxctrl[i].Bounds, &mlcA.pamxctrl[i].Bounds,
		   sizeof(mlcA.pamxctrl[i].Bounds));
	    /* sizeof(lpmlcW->pamxctrl[i].Metrics) ==
	     * sizeof(mlcA.pamxctrl[i].Metrics) */
	    memcpy(&lpmlcW->pamxctrl[i].Metrics, &mlcA.pamxctrl[i].Metrics,
		   sizeof(mlcA.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcA.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfoA		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoA(HMIXEROBJ hmix, LPMIXERLINEA lpmliW, DWORD fdwInfo)
{
    LPWINE_MIXER	lpwm;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmliW, fdwInfo);

    if ((lpwm = MIXER_GetDev(hmix, fdwInfo)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINEINFO, (DWORD_PTR)lpmliW,
			 fdwInfo, TRUE);
}

/**************************************************************************
 * 				mixerGetLineInfoW		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoW(HMIXEROBJ hmix, LPMIXERLINEW lpmliW,
			      DWORD fdwInfo)
{
    MIXERLINEA		mliA;
    UINT		ret;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmliW, fdwInfo);

    if (lpmliW == NULL || lpmliW->cbStruct != sizeof(*lpmliW))
	return MMSYSERR_INVALPARAM;

    mliA.cbStruct = sizeof(mliA);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliA.dwComponentType = lpmliW->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliA.dwDestination = lpmliW->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliA.dwLineID = lpmliW->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliA.dwDestination = lpmliW->dwDestination;
	mliA.dwSource = lpmliW->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliA.Target.dwType = lpmliW->Target.dwType;
	mliA.Target.wMid = lpmliW->Target.wMid;
	mliA.Target.wPid = lpmliW->Target.wPid;
	mliA.Target.vDriverVersion = lpmliW->Target.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, lpmliW->Target.szPname, -1, mliA.Target.szPname, sizeof(mliA.Target.szPname), NULL, NULL);
	break;
    default:
	FIXME("Unsupported fdwControls=0x%08lx\n", fdwInfo);
    }

    ret = mixerGetLineInfoA(hmix, &mliA, fdwInfo);

    lpmliW->dwDestination = mliA.dwDestination;
    lpmliW->dwSource = mliA.dwSource;
    lpmliW->dwLineID = mliA.dwLineID;
    lpmliW->fdwLine = mliA.fdwLine;
    lpmliW->dwUser = mliA.dwUser;
    lpmliW->dwComponentType = mliA.dwComponentType;
    lpmliW->cChannels = mliA.cChannels;
    lpmliW->cConnections = mliA.cConnections;
    lpmliW->cControls = mliA.cControls;
    MultiByteToWideChar( CP_ACP, 0, mliA.szShortName, -1, lpmliW->szShortName,
                         sizeof(lpmliW->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, mliA.szName, -1, lpmliW->szName,
                         sizeof(lpmliW->szName)/sizeof(WCHAR) );
    lpmliW->Target.dwType = mliA.Target.dwType;
    lpmliW->Target.dwDeviceID = mliA.Target.dwDeviceID;
    lpmliW->Target.wMid = mliA.Target.wMid;
    lpmliW->Target.wPid = mliA.Target.wPid;
    lpmliW->Target.vDriverVersion = mliA.Target.vDriverVersion;
    MultiByteToWideChar( CP_ACP, 0, mliA.Target.szPname, -1, lpmliW->Target.szPname,
                         sizeof(lpmliW->Target.szPname)/sizeof(WCHAR) );

    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.@]
 */
UINT WINAPI mixerSetControlDetails(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA,
				   DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;

    TRACE("(%p, %p, %08lx)\n", hmix, lpmcdA, fdwDetails);

    if ((lpwm = MIXER_GetDev(hmix, fdwDetails)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(&lpwm->mld, MXDM_SETCONTROLDETAILS, (DWORD_PTR)lpmcdA,
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerMessage		[WINMM.@]
 */
UINT WINAPI mixerMessage(HMIXER hmix, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
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
UINT WINAPI auxGetDevCapsW(UINT_PTR uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    AUXCAPSA	acA;
    UINT	ret = auxGetDevCapsA(uDeviceID, &acA, sizeof(acA));

    if (ret == MMSYSERR_NOERROR) {
	AUXCAPSW acW;
	acW.wMid           = acA.wMid;
	acW.wPid           = acA.wPid;
	acW.vDriverVersion = acA.vDriverVersion;
	MultiByteToWideChar( CP_ACP, 0, acA.szPname, -1, acW.szPname,
                             sizeof(acW.szPname)/sizeof(WCHAR) );
	acW.wTechnology    = acA.wTechnology;
	acW.dwSupport      = acA.dwSupport;
	memcpy(lpCaps, &acW, min(uSize, sizeof(acW)));
    }
    return ret;
}

/**************************************************************************
 * 				auxGetDevCapsA		[WINMM.@]
 */
UINT WINAPI auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				auxGetVolume		[WINMM.@]
 */
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
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
UINT WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD_PTR dw1, DWORD_PTR dw2)
{
    LPWINE_MLD		wmld;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2, TRUE);
}

/**************************************************************************
 * 				mciGetErrorStringW		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringW(MCIERROR wError, LPWSTR lpstrBuffer, UINT uLength)
{
    char       bufstr[MAXERRORLENGTH];
    BOOL       ret = mciGetErrorStringA(wError, bufstr, MAXERRORLENGTH);

    MultiByteToWideChar( CP_ACP, 0, bufstr, -1, lpstrBuffer, uLength );
    return ret;
}

/**************************************************************************
 * 				mciGetErrorStringA		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringA(MCIERROR dwError, LPSTR lpstrBuffer, UINT uLength)
{
    BOOL		ret = FALSE;

    if (lpstrBuffer != NULL && uLength > 0 &&
	dwError >= MCIERR_BASE && dwError <= MCIERR_CUSTOM_DRIVER_BASE) {

	if (LoadStringA(WINMM_IData->hWinMM32Instance,
			dwError, lpstrBuffer, uLength) > 0) {
	    ret = TRUE;
	}
    }
    return ret;
}

/**************************************************************************
 *			mciDriverNotify				[WINMM.@]
 */
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, MCIDEVICEID wDevID, UINT wStatus)
{
    TRACE("(%p, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageW(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[WINMM.@]
 */
DWORD WINAPI mciGetDriverData(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x)\n", uDeviceID);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return 0L;
    }

    return wmd->dwPrivate;
}

/**************************************************************************
 * 			mciSetDriverData			[WINMM.@]
 */
BOOL WINAPI mciSetDriverData(MCIDEVICEID uDeviceID, DWORD data)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08lx)\n", uDeviceID, data);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->dwPrivate = data;
    return TRUE;
}

/**************************************************************************
 * 				mciSendCommandA			[WINMM.@]
 */
DWORD WINAPI mciSendCommandA(MCIDEVICEID wDevID, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD	dwRet;

    TRACE("(%08x, %s, %08lx, %08lx)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2, TRUE);
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2);
    TRACE("=> %08lx\n", dwRet);
    return dwRet;
}

inline static LPSTR strdupWtoA( LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = HeapAlloc( GetProcessHeap(), 0, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

static int MCI_MapMsgWtoA(UINT msg, DWORD_PTR dwParam1, DWORD_PTR *dwParam2)
{
    switch(msg)
    {
    case MCI_CLOSE:
    case MCI_PLAY:
    case MCI_SEEK:
    case MCI_STOP:
    case MCI_PAUSE:
    case MCI_GETDEVCAPS:
    case MCI_SPIN:
    case MCI_SET:
    case MCI_STEP:
    case MCI_RECORD:
    case MCI_BREAK:
    case MCI_SOUND:
    case MCI_STATUS:
    case MCI_CUE:
    case MCI_REALIZE:
    case MCI_PUT:
    case MCI_WHERE:
    case MCI_FREEZE:
    case MCI_UNFREEZE:
    case MCI_CUT:
    case MCI_COPY:
    case MCI_PASTE:
    case MCI_UPDATE:
    case MCI_RESUME:
    case MCI_DELETE:
        return 0;

    case MCI_OPEN:
        {
            MCI_OPEN_PARMSW *mci_openW = (MCI_OPEN_PARMSW *)*dwParam2;
            MCI_OPEN_PARMSA *mci_openA;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_openA) + sizeof(DWORD_PTR));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_openA = (MCI_OPEN_PARMSA *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_openA->dwCallback = mci_openW->dwCallback;

            if (dwParam1 & MCI_OPEN_TYPE)
            {
                if (dwParam1 & MCI_OPEN_TYPE_ID)
                    mci_openA->lpstrDeviceType = (LPSTR)mci_openW->lpstrDeviceType;
                else
                    mci_openA->lpstrDeviceType = strdupWtoA(mci_openW->lpstrDeviceType);
            }
            if (dwParam1 & MCI_OPEN_ELEMENT)
            {
                if (dwParam1 & MCI_OPEN_ELEMENT_ID)
                    mci_openA->lpstrElementName = (LPSTR)mci_openW->lpstrElementName;
                else
                    mci_openA->lpstrElementName = strdupWtoA(mci_openW->lpstrElementName);
            }
            if (dwParam1 & MCI_OPEN_ALIAS)
                mci_openA->lpstrAlias = strdupWtoA(mci_openW->lpstrAlias);
        }
        return 1;

    case MCI_WINDOW:
        if (dwParam1 & MCI_ANIM_WINDOW_TEXT)
        {
            MCI_ANIM_WINDOW_PARMSW *mci_windowW = (MCI_ANIM_WINDOW_PARMSW *)*dwParam2;
            MCI_ANIM_WINDOW_PARMSA *mci_windowA;

            mci_windowA = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_windowA));
            if (!mci_windowA) return -1;

            *dwParam2 = (DWORD_PTR)mci_windowA;

            mci_windowA->lpstrText = strdupWtoA(mci_windowW->lpstrText);

            if (dwParam1 & MCI_NOTIFY)
                mci_windowA->dwCallback = mci_windowW->dwCallback;
            if (dwParam1 & MCI_ANIM_WINDOW_HWND)
                mci_windowA->hWnd = mci_windowW->hWnd;
            if (dwParam1 & MCI_ANIM_WINDOW_STATE)
                mci_windowA->nCmdShow = mci_windowW->nCmdShow;

            return 1;
        }
        return 0;

    case MCI_SYSINFO:
        {
            MCI_SYSINFO_PARMSW *mci_sysinfoW = (MCI_SYSINFO_PARMSW *)*dwParam2;
            MCI_SYSINFO_PARMSA *mci_sysinfoA;
            DWORD_PTR *ptr;

            ptr = HeapAlloc(GetProcessHeap(), 0, sizeof(*mci_sysinfoA) + sizeof(DWORD_PTR));
            if (!ptr) return -1;

            *ptr++ = *dwParam2; /* save the previous pointer */
            *dwParam2 = (DWORD_PTR)ptr;
            mci_sysinfoA = (MCI_SYSINFO_PARMSA *)ptr;

            if (dwParam1 & MCI_NOTIFY)
                mci_sysinfoA->dwCallback = mci_sysinfoW->dwCallback;

            mci_sysinfoA->dwRetSize = mci_sysinfoW->dwRetSize; /* FIXME */
            mci_sysinfoA->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mci_sysinfoA->dwRetSize);

            return 1;
        }

    case MCI_INFO:
    case MCI_SAVE:
    case MCI_LOAD:
    case MCI_ESCAPE:
    default:
        FIXME("Message 0x%04x needs translation\n", msg);
        return -1;
    }
    return 0;
}

static DWORD MCI_UnmapMsgWtoA(UINT msg, DWORD_PTR dwParam1, DWORD_PTR dwParam2,
                              DWORD result)
{
    switch(msg)
    {
    case MCI_OPEN:
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_OPEN_PARMSW *mci_openW = (MCI_OPEN_PARMSW *)*ptr;
            MCI_OPEN_PARMSA *mci_openA = (MCI_OPEN_PARMSA *)(ptr + 1);

            mci_openW->wDeviceID = mci_openA->wDeviceID;

            if (dwParam1 & MCI_OPEN_TYPE)
            {
                if (!(dwParam1 & MCI_OPEN_TYPE_ID))
                    HeapFree(GetProcessHeap(), 0, mci_openA->lpstrDeviceType);
            }
            if (dwParam1 & MCI_OPEN_ELEMENT)
            {
                if (!(dwParam1 & MCI_OPEN_ELEMENT_ID))
                    HeapFree(GetProcessHeap(), 0, mci_openA->lpstrElementName);
            }
            if (dwParam1 & MCI_OPEN_ALIAS)
                HeapFree(GetProcessHeap(), 0, mci_openA->lpstrAlias);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;

    case MCI_WINDOW:
        if (dwParam1 & MCI_ANIM_WINDOW_TEXT)
        {
            MCI_ANIM_WINDOW_PARMSA *mci_windowA = (MCI_ANIM_WINDOW_PARMSA *)dwParam2;

            HeapFree(GetProcessHeap(), 0, (void *)mci_windowA->lpstrText);
            HeapFree(GetProcessHeap(), 0, mci_windowA);
        }
        break;

    case MCI_SYSINFO:
        {
            DWORD_PTR *ptr = (DWORD_PTR *)dwParam2 - 1;
            MCI_SYSINFO_PARMSW *mci_sysinfoW = (MCI_SYSINFO_PARMSW *)*ptr;
            MCI_SYSINFO_PARMSA *mci_sysinfoA = (MCI_SYSINFO_PARMSA *)(ptr + 1);

            if (!result)
            {
                mci_sysinfoW->dwNumber = mci_sysinfoA->dwNumber;
                mci_sysinfoW->wDeviceType = mci_sysinfoA->wDeviceType;
                MultiByteToWideChar(CP_ACP, 0,
                                    mci_sysinfoA->lpstrReturn, mci_sysinfoA->dwRetSize,
                                    mci_sysinfoW->lpstrReturn, mci_sysinfoW->dwRetSize);
            }

            HeapFree(GetProcessHeap(), 0, mci_sysinfoA->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, ptr);
        }
        break;

    default:
        FIXME("Message 0x%04x needs unmapping\n", msg);
        break;
    }

    return result;
}


/**************************************************************************
 * 				mciSendCommandW			[WINMM.@]
 *
 * FIXME: we should do the things other way around, but since our
 * MM subsystem is not unicode aware...
 */
DWORD WINAPI mciSendCommandW(MCIDEVICEID wDevID, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    DWORD ret;
    int mapped;

    TRACE("(%08x, %s, %08lx, %08lx)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    mapped = MCI_MapMsgWtoA(wMsg, dwParam1, &dwParam2);
    if (mapped == -1)
    {
        FIXME("message %04x mapping failed\n", wMsg);
        return MMSYSERR_NOMEM;
    }
    ret = mciSendCommandA(wDevID, wMsg, dwParam1, dwParam2);
    if (mapped)
        MCI_UnmapMsgWtoA(wMsg, dwParam1, dwParam2, ret);
    return ret;
}

/**************************************************************************
 * 				mciGetDeviceIDA    		[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDA(LPCSTR lpstrName)
{
    return MCI_GetDriverFromString(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDW		       	[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDW(LPCWSTR lpwstrName)
{
    LPSTR 	lpstrName = NULL;
    UINT	ret;
    INT         len;

    if (lpwstrName) {
        len = WideCharToMultiByte( CP_ACP, 0, lpwstrName, -1, NULL, 0, NULL, NULL );
        lpstrName = HeapAlloc( GetProcessHeap(), 0, len );
        if (lpstrName) WideCharToMultiByte( CP_ACP, 0, lpwstrName, -1, lpstrName, len, NULL, NULL );
    }
    ret = MCI_GetDriverFromString(lpstrName);
    HeapFree(GetProcessHeap(), 0, lpstrName);
    return ret;
}

/**************************************************************************
 * 				MCI_DefYieldProc	       	[internal]
 */
UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data)
{
    INT16	ret;

    TRACE("(0x%04x, 0x%08lx)\n", wDevID, data);

    if ((HIWORD(data) != 0 && HWND_16(GetActiveWindow()) != HIWORD(data)) ||
	(GetAsyncKeyState(LOWORD(data)) & 1) == 0) {
	MyUserYield();
	ret = 0;
    } else {
	MSG		msg;

	msg.hwnd = HWND_32(HIWORD(data));
	while (!PeekMessageA(&msg, msg.hwnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
	ret = -1;
    }
    return ret;
}

/**************************************************************************
 * 				mciSetYieldProc			[WINMM.@]
 */
BOOL WINAPI mciSetYieldProc(MCIDEVICEID uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->lpfnYieldProc = fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = TRUE;

    return TRUE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementIDW	[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDFromElementIDW(DWORD dwElementID, LPCWSTR lpstrType)
{
    /* FIXME: that's rather strange, there is no
     * mciGetDeviceIDFromElementID32A in winmm.spec
     */
    FIXME("(%lu, %p) stub\n", dwElementID, lpstrType);
    return 0;
}

/**************************************************************************
 * 				mciGetYieldProc			[WINMM.@]
 */
YIELDPROC WINAPI mciGetYieldProc(MCIDEVICEID uDeviceID, DWORD* lpdwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return NULL;
    }
    if (!wmd->lpfnYieldProc) {
	WARN("No proc set\n");
	return NULL;
    }
    if (!wmd->bIs32) {
	WARN("Proc is 32 bit\n");
	return NULL;
    }
    return wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetCreatorTask		[WINMM.@]
 */
HTASK WINAPI mciGetCreatorTask(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    HTASK ret = 0;

    if ((wmd = MCI_GetDriver(uDeviceID))) ret = (HTASK)wmd->CreatorThread;

    TRACE("(%u) => %p\n", uDeviceID, ret);
    return ret;
}

/**************************************************************************
 * 			mciDriverYield				[WINMM.@]
 */
UINT WINAPI mciDriverYield(MCIDEVICEID uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    UINT		ret = 0;

    TRACE("(%04x)\n", uDeviceID);

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc || !wmd->bIs32) {
	MyUserYield();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
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
    MIDIOUTCAPSA	mocA;
    UINT		ret = midiOutGetDevCapsA(uDeviceID, &mocA, sizeof(mocA));

    if (ret == MMSYSERR_NOERROR) {
	MIDIOUTCAPSW mocW;
	mocW.wMid		= mocA.wMid;
	mocW.wPid		= mocA.wPid;
	mocW.vDriverVersion	= mocA.vDriverVersion;
	MultiByteToWideChar( CP_ACP, 0, mocA.szPname, -1, mocW.szPname,
                             sizeof(mocW.szPname)/sizeof(WCHAR) );
	mocW.wTechnology        = mocA.wTechnology;
	mocW.wVoices		= mocA.wVoices;
	mocW.wNotes		= mocA.wNotes;
	mocW.wChannelMask	= mocA.wChannelMask;
	mocW.dwSupport	        = mocA.dwSupport;
	memcpy(lpCaps, &mocW, min(uSize, sizeof(mocW)));
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.@]
 */
UINT WINAPI midiOutGetDevCapsA(UINT_PTR uDeviceID, LPMIDIOUTCAPSA lpCaps,
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
 * 				MIDI_GetErrorText       	[internal]
 */
static	UINT16	MIDI_GetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    UINT16		ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) {
	ret = MMSYSERR_INVALPARAM;
    } else if (uSize == 0) {
	ret = MMSYSERR_NOERROR;
    } else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= MIDIERR_BASE  && uError <= MIDIERR_LASTERROR)) {

	if (LoadStringA(WINMM_IData->hWinMM32Instance,
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret;

    ret = MIDI_GetErrorText(uError, xstr, uSize);
    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, xstr);
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
UINT WINAPI midiOutOpen(LPHMIDIOUT lphMidiOut, UINT uDeviceID,
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
UINT WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage,
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
UINT WINAPI midiInGetDevCapsW(UINT_PTR uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    MIDIINCAPSA		micA;
    UINT		ret = midiInGetDevCapsA(uDeviceID, &micA, sizeof(micA));

    if (ret == MMSYSERR_NOERROR) {
	MIDIINCAPSW micW;
	micW.wMid           = micA.wMid;
	micW.wPid           = micA.wPid;
	micW.vDriverVersion = micA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, micA.szPname, -1, micW.szPname,
                             sizeof(micW.szPname)/sizeof(WCHAR) );
	micW.dwSupport      = micA.dwSupport;
	memcpy(lpCaps, &micW, min(uSize, sizeof(micW)));
    }
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCapsA	[WINMM.@]
 */
UINT WINAPI midiInGetDevCapsA(UINT_PTR uDeviceID, LPMIDIINCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%d, %p, %d);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

   return MMDRV_Message(wmld, MIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				midiInGetErrorTextW 		[WINMM.@]
 */
UINT WINAPI midiInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = MIDI_GetErrorText(uError, xstr, uSize);

    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorTextA 		[WINMM.@]
 */
UINT WINAPI midiInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

UINT MIDI_InOpen(HMIDIIN* lphMidiIn, UINT uDeviceID, DWORD dwCallback,
                 DWORD dwInstance, DWORD dwFlags, BOOL bFrom32)
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
UINT WINAPI midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID,
		       DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
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
UINT WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage,
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
                         DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen, 
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
    lpMidiStrm->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    lpMidiStrm->wFlags = HIWORD(fdwOpen);

    lpMidiStrm->hThread = CreateThread(NULL, 0, MMSYSTEM_MidiStream_Player,
				       lpMidiStrm, 0, &(lpMidiStrm->dwThreadID));

    if (!lpMidiStrm->hThread) {
	midiStreamClose((HMIDISTRM)hMidiOut);
	return MMSYSERR_NOMEM;
    }

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
			       DWORD cMidi, DWORD dwCallback,
			       DWORD dwInstance, DWORD fdwOpen)
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
               const LPWAVEFORMATEX lpFormat, DWORD dwCallback, 
               DWORD dwInstance, DWORD dwFlags, BOOL bFrom32)
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
    wod.lpFormat = lpFormat;  /* should the struct be copied iso pointer? */
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
UINT WINAPI waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA lpCaps,
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
 * 				waveOutGetDevCapsW		[WINMM.@]
 */
UINT WINAPI waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW lpCaps,
			       UINT uSize)
{
    WAVEOUTCAPSA	wocA;
    UINT 		ret = waveOutGetDevCapsA(uDeviceID, &wocA, sizeof(wocA));

    if (ret == MMSYSERR_NOERROR) {
	WAVEOUTCAPSW wocW;
	wocW.wMid           = wocA.wMid;
	wocW.wPid           = wocA.wPid;
	wocW.vDriverVersion = wocA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, wocA.szPname, -1, wocW.szPname,
                             sizeof(wocW.szPname)/sizeof(WCHAR) );
	wocW.dwFormats      = wocA.dwFormats;
	wocW.wChannels      = wocA.wChannels;
	wocW.dwSupport      = wocA.dwSupport;
	memcpy(lpCaps, &wocW, min(uSize, sizeof(wocW)));
    }
    return ret;
}

/**************************************************************************
 * 				WAVE_GetErrorText       	[internal]
 */
static	UINT16	WAVE_GetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    UINT16		ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) {
	ret = MMSYSERR_INVALPARAM;
    } else if (uSize == 0) {
	ret = MMSYSERR_NOERROR;
    } else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */uError <= MMSYSERR_LASTERROR) ||
	       (uError >= WAVERR_BASE  && uError <= WAVERR_LASTERROR)) {

	if (LoadStringA(WINMM_IData->hWinMM32Instance,
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, xstr, uSize);

    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.@]
 * All the args/structs have the same layout as the win16 equivalents
 */
UINT WINAPI waveOutOpen(LPHWAVEOUT lphWaveOut, UINT uDeviceID,
                       const LPWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
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

    TRACE("(%p, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_PREPARE, (DWORD_PTR)lpWaveOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
				   LPWAVEHDR lpWaveOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;
    
    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_UNPREPARE, (DWORD_PTR)lpWaveOutHdr, uSize, TRUE);
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
UINT WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage,
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
UINT WINAPI waveInGetDevCapsW(UINT_PTR uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    WAVEINCAPSA		wicA;
    UINT		ret = waveInGetDevCapsA(uDeviceID, &wicA, sizeof(wicA));

    if (ret == MMSYSERR_NOERROR) {
	WAVEINCAPSW wicW;
	wicW.wMid           = wicA.wMid;
	wicW.wPid           = wicA.wPid;
	wicW.vDriverVersion = wicA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, wicA.szPname, -1, wicW.szPname,
                             sizeof(wicW.szPname)/sizeof(WCHAR) );
	wicW.dwFormats      = wicA.dwFormats;
	wicW.wChannels      = wicA.wChannels;
	memcpy(lpCaps, &wicW, min(uSize, sizeof(wicW)));
    }
    return ret;
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.@]
 */
UINT WINAPI waveInGetDevCapsA(UINT_PTR uDeviceID, LPWAVEINCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_WAVEIN, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, WIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				waveInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR txt = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, txt, uSize);

    MultiByteToWideChar( CP_ACP, 0, txt, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, txt);
    return ret;
}

/**************************************************************************
 * 				waveInOpen			[WINMM.@]
 */
UINT WINAPI waveInOpen(HWAVEIN* lphWaveIn, UINT uDeviceID,
		       const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
		       DWORD dwInstance, DWORD dwFlags)
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

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    lpWaveInHdr->dwBytesRecorded = 0;

    return MMDRV_Message(wmld, WIDM_PREPARE, (DWORD_PTR)lpWaveInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr,
				  UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD_PTR)lpWaveInHdr, uSize, TRUE);
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
UINT WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
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
