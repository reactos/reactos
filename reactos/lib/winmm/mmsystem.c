/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2003 Eric Pouech
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
 *  	99/4	added mmTask and mmThread functions support
 */

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winreg.h"
#include "ntstatus.h"
#include "winternl.h"
#include "wownt32.h"
#include "winnls.h"

#include "wine/winuser16.h"
#include "winemm16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

static WINE_MMTHREAD*   WINMM_GetmmThread(HANDLE16);
static LPWINE_DRIVER    DRIVER_OpenDriver16(LPCSTR, LPCSTR, LPARAM);
static LRESULT          DRIVER_CloseDriver16(HDRVR16, LPARAM, LPARAM);
static LRESULT          DRIVER_SendMessage16(HDRVR16, UINT, LPARAM, LPARAM);
static LRESULT          MMIO_Callback16(SEGPTR, LPMMIOINFO, UINT, LPARAM, LPARAM);

/* ###################################################
 * #                  LIBRARY                        #
 * ###################################################
 */

/**************************************************************************
 * 			DllEntryPoint (MMSYSTEM.2046)
 *
 * MMSYSTEM DLL entry point
 *
 */
BOOL WINAPI MMSYSTEM_LibMain(DWORD fdwReason, HINSTANCE hinstDLL, WORD ds,
			     WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    TRACE("%p 0x%lx\n", hinstDLL, fdwReason);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	/* need to load WinMM in order to:
	 * - initiate correctly shared variables (WINMM_Init())
	 */
        if (!GetModuleHandleA("WINMM.DLL") && !LoadLibraryA("WINMM.DLL"))
        {
            ERR("Could not load sibling WinMM.dll\n");
            return FALSE;
	}
	WINMM_IData->hWinMM16Instance = hinstDLL;
        /* hook in our 16 bit function pointers */
        pFnGetMMThread16    = WINMM_GetmmThread;
        pFnOpenDriver16     = DRIVER_OpenDriver16;
        pFnCloseDriver16    = DRIVER_CloseDriver16;
        pFnSendMessage16    = DRIVER_SendMessage16;
        pFnMmioCallback16   = MMIO_Callback16;
        pFnReleaseThunkLock = ReleaseThunkLock;
        pFnRestoreThunkLock = RestoreThunkLock;
        MMDRV_Init16();
	break;
    case DLL_PROCESS_DETACH:
	WINMM_IData->hWinMM16Instance = 0;
        pFnGetMMThread16    = NULL;
        pFnOpenDriver16     = NULL;
        pFnCloseDriver16    = NULL;
        pFnSendMessage16    = NULL;
        pFnMmioCallback16   = NULL;
        pFnReleaseThunkLock = NULL;
        pFnRestoreThunkLock = NULL;
        /* FIXME: add equivalent for MMDRV_Init16() */
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 				MMSYSTEM_WEP			[MMSYSTEM.1]
 */
int WINAPI MMSYSTEM_WEP(HINSTANCE16 hInstance, WORD wDataSeg,
                        WORD cbHeapSize, LPSTR lpCmdLine)
{
    FIXME("STUB: Unloading MMSystem DLL ... hInst=%04X \n", hInstance);
    return TRUE;
}

/* ###################################################
 * #                  PlaySound                      #
 * ###################################################
 */

/**************************************************************************
 * 				PlaySound		[MMSYSTEM.3]
 */
BOOL16 WINAPI PlaySound16(LPCSTR pszSound, HMODULE16 hmod, DWORD fdwSound)
{
    BOOL16	retv;
    DWORD	lc;

    if ((fdwSound & SND_RESOURCE) == SND_RESOURCE)
    {
        HGLOBAL16 handle;
        HRSRC16 res;

        if (!(res = FindResource16( hmod, pszSound, "WAVE" ))) return FALSE;
        if (!(handle = LoadResource16( hmod, res ))) return FALSE;
        pszSound = LockResource16(handle);
        fdwSound = (fdwSound & ~SND_RESOURCE) | SND_MEMORY;
        /* FIXME: FreeResource16 */
    }

    ReleaseThunkLock(&lc);
    retv = PlaySoundA(pszSound, 0, fdwSound);
    RestoreThunkLock(lc);

    return retv;
}

/**************************************************************************
 * 				sndPlaySound		[MMSYSTEM.2]
 */
BOOL16 WINAPI sndPlaySound16(LPCSTR lpszSoundName, UINT16 uFlags)
{
    BOOL16	retv;
    DWORD	lc;

    ReleaseThunkLock(&lc);
    retv = sndPlaySoundA(lpszSoundName, uFlags);
    RestoreThunkLock(lc);

    return retv;
}

/* ###################################################
 * #                    MISC                         #
 * ###################################################
 */

/**************************************************************************
 * 				mmsystemGetVersion	[MMSYSTEM.5]
 *
 */
UINT16 WINAPI mmsystemGetVersion16(void)
{
    return mmsystemGetVersion();
}

/**************************************************************************
 * 				DriverCallback			[MMSYSTEM.31]
 */
BOOL16 WINAPI DriverCallback16(DWORD dwCallBack, UINT16 uFlags, HDRVR16 hDev,
			       WORD wMsg, DWORD dwUser, DWORD dwParam1,
			       DWORD dwParam2)
{
    return DriverCallback(dwCallBack, uFlags, HDRVR_32(hDev), wMsg, dwUser, dwParam1, dwParam2);
}

/**************************************************************************
 * 			OutputDebugStr	 	[MMSYSTEM.30]
 */
void WINAPI OutputDebugStr16(LPCSTR str)
{
    OutputDebugStringA( str );
}


/* ###################################################
 * #                    MIXER                        #
 * ###################################################
 */

/**************************************************************************
 * 	Mixer devices. New to Win95
 */

/**************************************************************************
 * 				mixerGetNumDevs			[MMSYSTEM.800]
 */
UINT16 WINAPI mixerGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_MIXER);
}

/**************************************************************************
 * 				mixerGetDevCaps			[MMSYSTEM.801]
 */
UINT16 WINAPI mixerGetDevCaps16(UINT16 uDeviceID, LPMIXERCAPS16 lpCaps,
				UINT16 uSize)
{
    MIXERCAPSA  micA;
    UINT        ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = mixerGetDevCapsA(uDeviceID, &micA, sizeof(micA));
    if (ret == MMSYSERR_NOERROR) {
	MIXERCAPS16 mic16;
        mic16.wMid           = micA.wMid;
        mic16.wPid           = micA.wPid;
        mic16.vDriverVersion = micA.vDriverVersion;
        strcpy(mic16.szPname, micA.szPname);
        mic16.fdwSupport     = micA.fdwSupport;
        mic16.cDestinations  = micA.cDestinations;
	memcpy(lpCaps, &mic16, min(uSize, sizeof(mic16)));
    }
    return ret;
}

/**************************************************************************
 * 				mixerOpen			[MMSYSTEM.802]
 */
UINT16 WINAPI mixerOpen16(LPHMIXER16 lphmix, UINT16 uDeviceID, DWORD dwCallback,
			  DWORD dwInstance, DWORD fdwOpen)
{
    HMIXER	hmix;
    UINT	ret;

    ret = MIXER_Open(&hmix, uDeviceID, dwCallback, dwInstance, fdwOpen, FALSE);
    if (lphmix) *lphmix = HMIXER_16(hmix);
    return ret;
}

/**************************************************************************
 * 				mixerClose			[MMSYSTEM.803]
 */
UINT16 WINAPI mixerClose16(HMIXER16 hMix)
{
    return mixerClose(HMIXER_32(hMix));
}

/**************************************************************************
 * 				mixerGetID (MMSYSTEM.806)
 */
UINT16 WINAPI mixerGetID16(HMIXEROBJ16 hmix, LPUINT16 lpid, DWORD fdwID)
{
    UINT	xid;
    UINT	ret = mixerGetID(HMIXEROBJ_32(hmix), &xid, fdwID);

    if (lpid)
	*lpid = xid;
    return ret;
}

/**************************************************************************
 * 				mixerGetControlDetails	[MMSYSTEM.808]
 */
UINT16 WINAPI mixerGetControlDetails16(HMIXEROBJ16 hmix,
				       LPMIXERCONTROLDETAILS16 lpmcd,
				       DWORD fdwDetails)
{
    DWORD	ret = MMSYSERR_NOTENABLED;
    SEGPTR	sppaDetails;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcd, fdwDetails);

    if (lpmcd == NULL || lpmcd->cbStruct != sizeof(*lpmcd))
	return MMSYSERR_INVALPARAM;

    sppaDetails = (SEGPTR)lpmcd->paDetails;
    lpmcd->paDetails = MapSL(sppaDetails);
    ret = mixerGetControlDetailsA(HMIXEROBJ_32(hmix),
			         (LPMIXERCONTROLDETAILS)lpmcd, fdwDetails);
    lpmcd->paDetails = (LPVOID)sppaDetails;

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControls		[MMSYSTEM.807]
 */
UINT16 WINAPI mixerGetLineControls16(HMIXEROBJ16 hmix,
				     LPMIXERLINECONTROLS16 lpmlc16,
				     DWORD fdwControls)
{
    MIXERLINECONTROLSA	mlcA;
    DWORD		ret;
    int			i;
    LPMIXERCONTROL16	lpmc16;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmlc16, fdwControls);

    if (lpmlc16 == NULL || lpmlc16->cbStruct != sizeof(*lpmlc16) ||
	lpmlc16->cbmxctrl != sizeof(MIXERCONTROL16))
	return MMSYSERR_INVALPARAM;

    mlcA.cbStruct = sizeof(mlcA);
    mlcA.dwLineID = lpmlc16->dwLineID;
    mlcA.u.dwControlID = lpmlc16->u.dwControlID;
    mlcA.u.dwControlType = lpmlc16->u.dwControlType;
    mlcA.cControls = lpmlc16->cControls;
    mlcA.cbmxctrl = sizeof(MIXERCONTROLA);
    mlcA.pamxctrl = HeapAlloc(GetProcessHeap(), 0,
			      mlcA.cControls * mlcA.cbmxctrl);

    ret = mixerGetLineControlsA(HMIXEROBJ_32(hmix), &mlcA, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlc16->dwLineID = mlcA.dwLineID;
	lpmlc16->u.dwControlID = mlcA.u.dwControlID;
	lpmlc16->u.dwControlType = mlcA.u.dwControlType;
	lpmlc16->cControls = mlcA.cControls;

	lpmc16 = MapSL(lpmlc16->pamxctrl);

	for (i = 0; i < mlcA.cControls; i++) {
	    lpmc16[i].cbStruct = sizeof(MIXERCONTROL16);
	    lpmc16[i].dwControlID = mlcA.pamxctrl[i].dwControlID;
	    lpmc16[i].dwControlType = mlcA.pamxctrl[i].dwControlType;
	    lpmc16[i].fdwControl = mlcA.pamxctrl[i].fdwControl;
	    lpmc16[i].cMultipleItems = mlcA.pamxctrl[i].cMultipleItems;
	    strcpy(lpmc16[i].szShortName, mlcA.pamxctrl[i].szShortName);
	    strcpy(lpmc16[i].szName, mlcA.pamxctrl[i].szName);
	    /* sizeof(lpmc16[i].Bounds) == sizeof(mlcA.pamxctrl[i].Bounds) */
	    memcpy(&lpmc16[i].Bounds, &mlcA.pamxctrl[i].Bounds,
		   sizeof(mlcA.pamxctrl[i].Bounds));
	    /* sizeof(lpmc16[i].Metrics) == sizeof(mlcA.pamxctrl[i].Metrics) */
	    memcpy(&lpmc16[i].Metrics, &mlcA.pamxctrl[i].Metrics,
		   sizeof(mlcA.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcA.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfo	[MMSYSTEM.805]
 */
UINT16 WINAPI mixerGetLineInfo16(HMIXEROBJ16 hmix, LPMIXERLINE16 lpmli16,
				 DWORD fdwInfo)
{
    MIXERLINEA		mliA;
    UINT		ret;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmli16, fdwInfo);

    if (lpmli16 == NULL || lpmli16->cbStruct != sizeof(*lpmli16))
	return MMSYSERR_INVALPARAM;

    mliA.cbStruct = sizeof(mliA);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliA.dwComponentType = lpmli16->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliA.dwDestination = lpmli16->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliA.dwLineID = lpmli16->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliA.dwDestination = lpmli16->dwDestination;
	mliA.dwSource = lpmli16->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliA.Target.dwType = lpmli16->Target.dwType;
	mliA.Target.wMid = lpmli16->Target.wMid;
	mliA.Target.wPid = lpmli16->Target.wPid;
	mliA.Target.vDriverVersion = lpmli16->Target.vDriverVersion;
	strcpy(mliA.Target.szPname, lpmli16->Target.szPname);
	break;
    default:
	FIXME("Unsupported fdwControls=0x%08lx\n", fdwInfo);
    }

    ret = mixerGetLineInfoA(HMIXEROBJ_32(hmix), &mliA, fdwInfo);

    lpmli16->dwDestination     	= mliA.dwDestination;
    lpmli16->dwSource          	= mliA.dwSource;
    lpmli16->dwLineID          	= mliA.dwLineID;
    lpmli16->fdwLine           	= mliA.fdwLine;
    lpmli16->dwUser            	= mliA.dwUser;
    lpmli16->dwComponentType   	= mliA.dwComponentType;
    lpmli16->cChannels         	= mliA.cChannels;
    lpmli16->cConnections      	= mliA.cConnections;
    lpmli16->cControls         	= mliA.cControls;
    strcpy(lpmli16->szShortName, mliA.szShortName);
    strcpy(lpmli16->szName, mliA.szName);
    lpmli16->Target.dwType     	= mliA.Target.dwType;
    lpmli16->Target.dwDeviceID 	= mliA.Target.dwDeviceID;
    lpmli16->Target.wMid       	= mliA.Target.wMid;
    lpmli16->Target.wPid        = mliA.Target.wPid;
    lpmli16->Target.vDriverVersion = mliA.Target.vDriverVersion;
    strcpy(lpmli16->Target.szPname, mliA.Target.szPname);

    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[MMSYSTEM.809]
 */
UINT16 WINAPI mixerSetControlDetails16(HMIXEROBJ16 hmix,
				       LPMIXERCONTROLDETAILS16 lpmcd,
				       DWORD fdwDetails)
{
    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerMessage		[MMSYSTEM.804]
 */
DWORD WINAPI mixerMessage16(HMIXER16 hmix, UINT16 uMsg, DWORD dwParam1,
			     DWORD dwParam2)
{
    return mixerMessage(HMIXER_32(hmix), uMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				auxGetNumDevs		[MMSYSTEM.350]
 */
UINT16 WINAPI auxGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_AUX);
}

/* ###################################################
 * #                     AUX                         #
 * ###################################################
 */

/**************************************************************************
 * 				auxGetDevCaps		[MMSYSTEM.351]
 */
UINT16 WINAPI auxGetDevCaps16(UINT16 uDeviceID, LPAUXCAPS16 lpCaps, UINT16 uSize)
{
    AUXCAPSA  acA;
    UINT      ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = auxGetDevCapsA(uDeviceID, &acA, sizeof(acA));
    if (ret == MMSYSERR_NOERROR) {
	AUXCAPS16 ac16;
	ac16.wMid           = acA.wMid; 
	ac16.wPid           = acA.wPid; 
	ac16.vDriverVersion = acA.vDriverVersion; 
	strcpy(ac16.szPname, acA.szPname); 
	ac16.wTechnology    = acA.wTechnology; 
	ac16.dwSupport      = acA.dwSupport; 
	memcpy(lpCaps, &ac16, min(uSize, sizeof(ac16)));
    }
    return ret;
}

/**************************************************************************
 * 				auxGetVolume		[MMSYSTEM.352]
 */
UINT16 WINAPI auxGetVolume16(UINT16 uDeviceID, LPDWORD lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p) !\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get((HANDLE)(ULONG_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETVOLUME, (DWORD_PTR)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxSetVolume		[MMSYSTEM.353]
 */
UINT16 WINAPI auxSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %lu) !\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get((HANDLE)(ULONG_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage16(UINT16 uDeviceID, UINT16 uMessage, DWORD dw1, DWORD dw2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", uDeviceID, uMessage, dw1, dw2);

    switch (uMessage) {
    case AUXDM_GETNUMDEVS:
    case AUXDM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case AUXDM_GETVOLUME:
	return auxGetVolume16(uDeviceID, MapSL(dw1));
    case AUXDM_GETDEVCAPS:
	return auxGetDevCaps16(uDeviceID, MapSL(dw1), dw2);
    default:
	TRACE("(%04x, %04x, %08lx, %08lx): unhandled message\n",
	      uDeviceID, uMessage, dw1, dw2);
	break;
    }
    if ((wmld = MMDRV_Get((HANDLE)(ULONG_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2, TRUE);
}

/* ###################################################
 * #                     MCI                         #
 * ###################################################
 */

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
    return mciGetErrorStringA(wError, lpstrBuffer, uLength);
}

/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify16(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE("(%04X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageA(HWND_32(hWndCallBack), MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(UINT16 uDeviceID)
{
    return mciGetDriverData(uDeviceID);
}

/**************************************************************************
 * 			mciSetDriverData			[MMSYSTEM.707]
 */
BOOL16 WINAPI mciSetDriverData16(UINT16 uDeviceID, DWORD data)
{
    return mciSetDriverData(uDeviceID, data);
}

/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet;

    TRACE("(%04X, %s, %08lX, %08lX)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2, FALSE);
    dwRet = MCI_CleanUp(dwRet, wMsg, (DWORD)MapSL(dwParam2));
    TRACE("=> %ld\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciGetDeviceID		       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID16(LPCSTR lpstrName)
{
    TRACE("(\"%s\")\n", lpstrName);

    return MCI_GetDriverFromString(lpstrName);
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC16 fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->lpfnYieldProc = (YIELDPROC)fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = FALSE;

    return TRUE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID16(DWORD dwElementID, LPCSTR lpstrType)
{
    FIXME("(%lu, %s) stub\n", dwElementID, lpstrType);
    return 0;
}

/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC16 WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
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
    if (wmd->bIs32) {
	WARN("Proc is 32 bit\n");
	return NULL;
    }
    return (YIELDPROC16)wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetCreatorTask		[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask16(UINT16 uDeviceID)
{
    LPWINE_MCIDRIVER wmd;
    HTASK16 ret = 0;

    if ((wmd = MCI_GetDriver(uDeviceID))) 
        ret = HTASK_16(wmd->CreatorThread);

    TRACE("(%u) => %04x\n", uDeviceID, ret);
    return ret;
}

/**************************************************************************
 * 				mciDriverYield			[MMSYSTEM.710]
 */
UINT16 WINAPI mciDriverYield16(UINT16 uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    UINT16		ret = 0;

    /*    TRACE("(%04x)\n", uDeviceID); */

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc || wmd->bIs32) {
	UserYield16();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}

/* ###################################################
 * #                     MIDI                        #
 * ###################################################
 */

/**************************************************************************
 * 				midiOutGetNumDevs	[MMSYSTEM.201]
 */
UINT16 WINAPI midiOutGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_MIDIOUT);
}

/**************************************************************************
 * 				midiOutGetDevCaps	[MMSYSTEM.202]
 */
UINT16 WINAPI midiOutGetDevCaps16(UINT16 uDeviceID, LPMIDIOUTCAPS16 lpCaps,
				  UINT16 uSize)
{
    MIDIOUTCAPSA	mocA;
    UINT		ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = midiOutGetDevCapsA(uDeviceID, &mocA, sizeof(mocA));
    if (ret == MMSYSERR_NOERROR) {
	MIDIOUTCAPS16 moc16;
	moc16.wMid            = mocA.wMid;
	moc16.wPid            = mocA.wPid;
	moc16.vDriverVersion  = mocA.vDriverVersion;
	strcpy(moc16.szPname, mocA.szPname);
	moc16.wTechnology     = mocA.wTechnology;
	moc16.wVoices         = mocA.wVoices;
	moc16.wNotes          = mocA.wNotes;
	moc16.wChannelMask    = mocA.wChannelMask;
	moc16.dwSupport       = mocA.dwSupport;
	memcpy(lpCaps, &moc16, min(uSize, sizeof(moc16)));
    }
    return ret;
 }

/**************************************************************************
 * 				midiOutGetErrorText 	[MMSYSTEM.203]
 */
UINT16 WINAPI midiOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return midiOutGetErrorTextA(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutOpen    		[MMSYSTEM.204]
 */
UINT16 WINAPI midiOutOpen16(HMIDIOUT16* lphMidiOut, UINT16 uDeviceID,
                            DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIOUT	hmo;
    UINT	ret;

    ret = MIDI_OutOpen(&hmo, uDeviceID, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphMidiOut != NULL) *lphMidiOut = HMIDIOUT_16(hmo);
    return ret;
}

/**************************************************************************
 * 				midiOutClose		[MMSYSTEM.205]
 */
UINT16 WINAPI midiOutClose16(HMIDIOUT16 hMidiOut)
{
    return midiOutClose(HMIDIOUT_32(hMidiOut));
}

/**************************************************************************
 * 				midiOutPrepareHeader	[MMSYSTEM.206]
 */
UINT16 WINAPI midiOutPrepareHeader16(HMIDIOUT16 hMidiOut,         /* [in] */
                                     SEGPTR lpsegMidiOutHdr,      /* [???] */
				     UINT16 uSize)                /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_PREPARE, lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[MMSYSTEM.207]
 */
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16 hMidiOut,         /* [in] */
				       SEGPTR lpsegMidiOutHdr,      /* [???] */
				       UINT16 uSize)                /* [in] */
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiOutHdr = MapSL(lpsegMidiOutHdr);

    TRACE("(%04X, %08lx, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutShortMsg		[MMSYSTEM.208]
 */
UINT16 WINAPI midiOutShortMsg16(HMIDIOUT16 hMidiOut, DWORD dwMsg)
{
    return midiOutShortMsg(HMIDIOUT_32(hMidiOut), dwMsg);
}

/**************************************************************************
 * 				midiOutLongMsg		[MMSYSTEM.209]
 */
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16 hMidiOut,          /* [in] */
                               LPMIDIHDR16 lpsegMidiOutHdr,  /* [???] NOTE: SEGPTR */
			       UINT16 uSize)                 /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD_PTR)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutReset		[MMSYSTEM.210]
 */
UINT16 WINAPI midiOutReset16(HMIDIOUT16 hMidiOut)
{
    return midiOutReset(HMIDIOUT_32(hMidiOut));
}

/**************************************************************************
 * 				midiOutGetVolume	[MMSYSTEM.211]
 */
UINT16 WINAPI midiOutGetVolume16(UINT16 uDeviceID, DWORD* lpdwVolume)
{
    return midiOutGetVolume(HMIDIOUT_32(uDeviceID), lpdwVolume);
}

/**************************************************************************
 * 				midiOutSetVolume	[MMSYSTEM.212]
 */
UINT16 WINAPI midiOutSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    return midiOutSetVolume(HMIDIOUT_32(uDeviceID), dwVolume);
}

/**************************************************************************
 * 				midiOutCachePatches		[MMSYSTEM.213]
 */
UINT16 WINAPI midiOutCachePatches16(HMIDIOUT16 hMidiOut, UINT16 uBank,
                                    WORD* lpwPatchArray, UINT16 uFlags)
{
    return midiOutCachePatches(HMIDIOUT_32(hMidiOut), uBank, lpwPatchArray,
			       uFlags);
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[MMSYSTEM.214]
 */
UINT16 WINAPI midiOutCacheDrumPatches16(HMIDIOUT16 hMidiOut, UINT16 uPatch,
                                        WORD* lpwKeyArray, UINT16 uFlags)
{
    return midiOutCacheDrumPatches(HMIDIOUT_32(hMidiOut), uPatch, lpwKeyArray, uFlags);
}

/**************************************************************************
 * 				midiOutGetID		[MMSYSTEM.215]
 */
UINT16 WINAPI midiOutGetID16(HMIDIOUT16 hMidiOut, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutMessage		[MMSYSTEM.216]
 */
DWORD WINAPI midiOutMessage16(HMIDIOUT16 hMidiOut, UINT16 uMessage,
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;

    case MODM_GETVOLUME:
        return midiOutGetVolume16(hMidiOut, MapSL(dwParam1));
    case MODM_LONGDATA:
        return midiOutLongMsg16(hMidiOut, MapSL(dwParam1), dwParam2);
    case MODM_PREPARE:
        /* lpMidiOutHdr is still a segmented pointer for this function */
        return midiOutPrepareHeader16(hMidiOut, dwParam1, dwParam2);
    case MODM_UNPREPARE:
        return midiOutUnprepareHeader16(hMidiOut, dwParam1, dwParam2);
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiInGetNumDevs	[MMSYSTEM.301]
 */
UINT16 WINAPI midiInGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_MIDIIN);
}

/**************************************************************************
 * 				midiInGetDevCaps	[MMSYSTEM.302]
 */
UINT16 WINAPI midiInGetDevCaps16(UINT16 uDeviceID, LPMIDIINCAPS16 lpCaps,
				 UINT16 uSize)
{
    MIDIINCAPSA		micA;
    UINT		ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = midiInGetDevCapsA(uDeviceID, &micA, uSize);
    if (ret == MMSYSERR_NOERROR) {
	MIDIINCAPS16 mic16;
	mic16.wMid           = micA.wMid;
	mic16.wPid           = micA.wPid;
	mic16.vDriverVersion = micA.vDriverVersion;
	strcpy(mic16.szPname, micA.szPname);
	mic16.dwSupport      = micA.dwSupport;
	memcpy(lpCaps, &mic16, min(uSize, sizeof(mic16)));
    }
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorText 		[MMSYSTEM.303]
 */
UINT16 WINAPI midiInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return midiInGetErrorTextA(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiInOpen		[MMSYSTEM.304]
 */
UINT16 WINAPI midiInOpen16(HMIDIIN16* lphMidiIn, UINT16 uDeviceID,
			   DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIIN	xhmid;
    UINT 	ret;

    ret = MIDI_InOpen(&xhmid, uDeviceID, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphMidiIn) *lphMidiIn = HMIDIIN_16(xhmid);
    return ret;
}

/**************************************************************************
 * 				midiInClose		[MMSYSTEM.305]
 */
UINT16 WINAPI midiInClose16(HMIDIIN16 hMidiIn)
{
    return midiInClose(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInPrepareHeader	[MMSYSTEM.306]
 */
UINT16 WINAPI midiInPrepareHeader16(HMIDIIN16 hMidiIn,         /* [in] */
                                    SEGPTR lpsegMidiInHdr,     /* [???] */
				    UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[MMSYSTEM.307]
 */
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16 hMidiIn,         /* [in] */
                                      SEGPTR lpsegMidiInHdr,     /* [???] */
				      UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiInHdr = MapSL(lpsegMidiInHdr);

    TRACE("(%04X, %08lx, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInAddBuffer		[MMSYSTEM.308]
 */
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16 hMidiIn,         /* [in] */
                                MIDIHDR16* lpsegMidiInHdr, /* [???] NOTE: SEGPTR */
				UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD_PTR)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInStart			[MMSYSTEM.309]
 */
UINT16 WINAPI midiInStart16(HMIDIIN16 hMidiIn)
{
    return midiInStart(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInStop			[MMSYSTEM.310]
 */
UINT16 WINAPI midiInStop16(HMIDIIN16 hMidiIn)
{
    return midiInStop(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInReset			[MMSYSTEM.311]
 */
UINT16 WINAPI midiInReset16(HMIDIIN16 hMidiIn)
{
    return midiInReset(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInGetID			[MMSYSTEM.312]
 */
UINT16 WINAPI midiInGetID16(HMIDIIN16 hMidiIn, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[MMSYSTEM.313]
 */
DWORD WINAPI midiInMessage16(HMIDIIN16 hMidiIn, UINT16 uMessage,
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    switch (uMessage) {
    case MIDM_OPEN:
    case MIDM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;

    case MIDM_GETDEVCAPS:
        return midiInGetDevCaps16(hMidiIn, MapSL(dwParam1), dwParam2);
    case MIDM_PREPARE:
        return midiInPrepareHeader16(hMidiIn, dwParam1, dwParam2);
    case MIDM_UNPREPARE:
        return midiInUnprepareHeader16(hMidiIn, dwParam1, dwParam2);
    case MIDM_ADDBUFFER:
        return midiInAddBuffer16(hMidiIn, MapSL(dwParam1), dwParam2);
    }

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, FALSE);
}

/**************************************************************************
 * 				midiStreamClose			[MMSYSTEM.252]
 */
MMRESULT16 WINAPI midiStreamClose16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamClose(HMIDISTRM_32(hMidiStrm));
}

/**************************************************************************
 * 				midiStreamOpen			[MMSYSTEM.251]
 */
MMRESULT16 WINAPI midiStreamOpen16(HMIDISTRM16* phMidiStrm, LPUINT16 devid,
				   DWORD cMidi, DWORD dwCallback,
				   DWORD dwInstance, DWORD fdwOpen)
{
    HMIDISTRM	hMidiStrm32;
    MMRESULT 	ret;
    UINT	devid32;

    if (!phMidiStrm || !devid)
	return MMSYSERR_INVALPARAM;
    devid32 = *devid;
    ret = MIDI_StreamOpen(&hMidiStrm32, &devid32, cMidi, dwCallback,
                          dwInstance, fdwOpen, FALSE);
    *phMidiStrm = HMIDISTRM_16(hMidiStrm32);
    *devid = devid32;
    return ret;
}

/**************************************************************************
 * 				midiStreamOut			[MMSYSTEM.254]
 */
MMRESULT16 WINAPI midiStreamOut16(HMIDISTRM16 hMidiStrm, LPMIDIHDR16 lpMidiHdr, UINT16 cbMidiHdr)
{
    return midiStreamOut(HMIDISTRM_32(hMidiStrm), (LPMIDIHDR)lpMidiHdr,
		         cbMidiHdr);
}

/**************************************************************************
 * 				midiStreamPause			[MMSYSTEM.255]
 */
MMRESULT16 WINAPI midiStreamPause16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamPause(HMIDISTRM_32(hMidiStrm));
}

/**************************************************************************
 * 				midiStreamPosition		[MMSYSTEM.253]
 */
MMRESULT16 WINAPI midiStreamPosition16(HMIDISTRM16 hMidiStrm, LPMMTIME16 lpmmt16, UINT16 cbmmt)
{
    MMTIME	mmt32;
    MMRESULT	ret;

    if (!lpmmt16)
	return MMSYSERR_INVALPARAM;
    MMSYSTEM_MMTIME16to32(&mmt32, lpmmt16);
    ret = midiStreamPosition(HMIDISTRM_32(hMidiStrm), &mmt32, sizeof(MMTIME));
    MMSYSTEM_MMTIME32to16(lpmmt16, &mmt32);
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[MMSYSTEM.250]
 */
MMRESULT16 WINAPI midiStreamProperty16(HMIDISTRM16 hMidiStrm, LPBYTE lpPropData, DWORD dwProperty)
{
    return midiStreamProperty(HMIDISTRM_32(hMidiStrm), lpPropData, dwProperty);
}

/**************************************************************************
 * 				midiStreamRestart		[MMSYSTEM.256]
 */
MMRESULT16 WINAPI midiStreamRestart16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamRestart(HMIDISTRM_32(hMidiStrm));
}

/**************************************************************************
 * 				midiStreamStop			[MMSYSTEM.257]
 */
MMRESULT16 WINAPI midiStreamStop16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamStop(HMIDISTRM_32(hMidiStrm));
}

/* ###################################################
 * #                     WAVE                        #
 * ###################################################
 */

/**************************************************************************
 * 				waveOutGetNumDevs		[MMSYSTEM.401]
 */
UINT16 WINAPI waveOutGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_WAVEOUT);
}

/**************************************************************************
 * 				waveOutGetDevCaps		[MMSYSTEM.402]
 */
UINT16 WINAPI waveOutGetDevCaps16(UINT16 uDeviceID,
				  LPWAVEOUTCAPS16 lpCaps, UINT16 uSize)
{
    WAVEOUTCAPSA	wocA;
    UINT 		ret;
    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = waveOutGetDevCapsA(uDeviceID, &wocA, sizeof(wocA));
    if (ret == MMSYSERR_NOERROR) {
        WAVEOUTCAPS16 woc16;
        woc16.wMid           = wocA.wMid;
        woc16.wPid           = wocA.wPid;
        woc16.vDriverVersion = wocA.vDriverVersion;
        strcpy(woc16.szPname, wocA.szPname);
        woc16.dwFormats      = wocA.dwFormats;
        woc16.wChannels      = wocA.wChannels;
        woc16.dwSupport      = wocA.dwSupport;
        memcpy(lpCaps, &woc16, min(uSize, sizeof(woc16)));
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorText 	[MMSYSTEM.403]
 */
UINT16 WINAPI waveOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return waveOutGetErrorTextA(uError, lpText, uSize);
}

/**************************************************************************
 *			waveOutOpen			[MMSYSTEM.404]
 */
UINT16 WINAPI waveOutOpen16(HWAVEOUT16* lphWaveOut, UINT16 uDeviceID,
                            const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
			    DWORD dwInstance, DWORD dwFlags)
{
    HANDLE		hWaveOut;
    UINT		ret;

    /* since layout of WAVEFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     * however, we need to promote correctly the wave mapper id
     * (0xFFFFFFFF and not 0x0000FFFF)
     */
    ret = WAVE_Open(&hWaveOut, (uDeviceID == (UINT16)-1) ? (UINT)-1 : uDeviceID,
                    MMDRV_WAVEOUT, lpFormat, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveOut != NULL) *lphWaveOut = HWAVEOUT_16(hWaveOut);
    return ret;
}

/**************************************************************************
 * 				waveOutClose		[MMSYSTEM.405]
 */
UINT16 WINAPI waveOutClose16(HWAVEOUT16 hWaveOut)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutClose(HWAVEOUT_32(hWaveOut));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutPrepareHeader	[MMSYSTEM.406]
 */
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16 hWaveOut,      /* [in] */
                                     SEGPTR lpsegWaveOutHdr,   /* [???] */
				     UINT16 uSize)             /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = MapSL(lpsegWaveOutHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_PREPARE, lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[MMSYSTEM.407]
 */
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16 hWaveOut,       /* [in] */
				       SEGPTR lpsegWaveOutHdr,    /* [???] */
				       UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = MapSL(lpsegWaveOutHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_UNPREPARE, lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT16 WINAPI waveOutWrite16(HWAVEOUT16 hWaveOut,       /* [in] */
			     LPWAVEHDR lpsegWaveOutHdr, /* [???] NOTE: SEGPTR */
			     UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_WRITE, (DWORD_PTR)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutBreakLoop	[MMSYSTEM.419]
 */
UINT16 WINAPI waveOutBreakLoop16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutBreakLoop(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutPause		[MMSYSTEM.409]
 */
UINT16 WINAPI waveOutPause16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutPause(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutReset		[MMSYSTEM.411]
 */
UINT16 WINAPI waveOutReset16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutReset(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutRestart	[MMSYSTEM.410]
 */
UINT16 WINAPI waveOutRestart16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutRestart(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutGetPosition	[MMSYSTEM.412]
 */
UINT16 WINAPI waveOutGetPosition16(HWAVEOUT16 hWaveOut, LPMMTIME16 lpTime,
                                   UINT16 uSize)
{
    UINT	ret;
    MMTIME	mmt;

    mmt.wType = lpTime->wType;
    ret = waveOutGetPosition(HWAVEOUT_32(hWaveOut), &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

/**************************************************************************
 * 				waveOutGetPitch		[MMSYSTEM.413]
 */
UINT16 WINAPI waveOutGetPitch16(HWAVEOUT16 hWaveOut16, LPDWORD lpdw)
{
    return waveOutGetPitch(HWAVEOUT_32(hWaveOut16), lpdw);
}

/**************************************************************************
 * 				waveOutSetPitch		[MMSYSTEM.414]
 */
UINT16 WINAPI waveOutSetPitch16(HWAVEOUT16 hWaveOut16, DWORD dw)
{
    return waveOutSetPitch(HWAVEOUT_32(hWaveOut16), dw);
}

/**************************************************************************
 * 				waveOutGetPlaybackRate	[MMSYSTEM.417]
 */
UINT16 WINAPI waveOutGetPlaybackRate16(HWAVEOUT16 hWaveOut16, LPDWORD lpdw)
{
    return waveOutGetPlaybackRate(HWAVEOUT_32(hWaveOut16), lpdw);
}

/**************************************************************************
 * 				waveOutSetPlaybackRate	[MMSYSTEM.418]
 */
UINT16 WINAPI waveOutSetPlaybackRate16(HWAVEOUT16 hWaveOut16, DWORD dw)
{
    return waveOutSetPlaybackRate(HWAVEOUT_32(hWaveOut16), dw);
}

/**************************************************************************
 * 				waveOutGetVolume	[MMSYSTEM.415]
 */
UINT16 WINAPI waveOutGetVolume16(UINT16 devid, LPDWORD lpdw)
{
    return waveOutGetVolume(HWAVEOUT_32(devid), lpdw);
}

/**************************************************************************
 * 				waveOutSetVolume	[MMSYSTEM.416]
 */
UINT16 WINAPI waveOutSetVolume16(UINT16 devid, DWORD dw)
{
    return waveOutSetVolume(HWAVEOUT_32(devid), dw);
}

/**************************************************************************
 * 				waveOutGetID	 	[MMSYSTEM.420]
 */
UINT16 WINAPI waveOutGetID16(HWAVEOUT16 hWaveOut, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutMessage 		[MMSYSTEM.421]
 */
DWORD WINAPI waveOutMessage16(HWAVEOUT16 hWaveOut, UINT16 uMessage,
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04x, %u, %ld, %ld)\n", hWaveOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, TRUE)) != NULL) {
            if (uMessage == DRV_QUERYDRVENTRY || uMessage == DRV_QUERYDEVNODE)
                dwParam1 = (DWORD)MapSL(dwParam1);
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, FALSE);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[MMSYSTEM.501]
 */
UINT16 WINAPI waveInGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_WAVEIN);
}

/**************************************************************************
 * 				waveInGetDevCaps 		[MMSYSTEM.502]
 */
UINT16 WINAPI waveInGetDevCaps16(UINT16 uDeviceID, LPWAVEINCAPS16 lpCaps,
				 UINT16 uSize)
{
    WAVEINCAPSA	wicA;
    UINT	ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = waveInGetDevCapsA(uDeviceID, &wicA, sizeof(wicA));
    if (ret == MMSYSERR_NOERROR) {
        WAVEINCAPS16 wic16;
        wic16.wMid           = wicA.wMid;
        wic16.wPid           = wicA.wPid;
        wic16.vDriverVersion = wicA.vDriverVersion;
        strcpy(wic16.szPname, wicA.szPname);
        wic16.dwFormats      = wicA.dwFormats;
        wic16.wChannels      = wicA.wChannels;
        memcpy(lpCaps, &wic16, min(uSize, sizeof(wic16)));
    }
    return ret;
}

/**************************************************************************
 * 				waveInGetErrorText 	[MMSYSTEM.503]
 */
UINT16 WINAPI waveInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return waveInGetErrorTextA(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveInOpen			[MMSYSTEM.504]
 */
UINT16 WINAPI waveInOpen16(HWAVEIN16* lphWaveIn, UINT16 uDeviceID,
                           const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
                           DWORD dwInstance, DWORD dwFlags)
{
    HANDLE		hWaveIn;
    UINT		ret;

    /* since layout of WAVEFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     * however, we need to promote correctly the wave mapper id
     * (0xFFFFFFFF and not 0x0000FFFF)
     */
    ret = WAVE_Open(&hWaveIn, (uDeviceID == (UINT16)-1) ? (UINT)-1 : uDeviceID,
                    MMDRV_WAVEIN, lpFormat, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveIn != NULL) *lphWaveIn = HWAVEIN_16(hWaveIn);
    return ret;
}

/**************************************************************************
 * 				waveInClose			[MMSYSTEM.505]
 */
UINT16 WINAPI waveInClose16(HWAVEIN16 hWaveIn)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInClose(HWAVEIN_32(hWaveIn));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInPrepareHeader		[MMSYSTEM.506]
 */
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16 hWaveIn,       /* [in] */
				    SEGPTR lpsegWaveInHdr,   /* [???] */
				    UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = MapSL(lpsegWaveInHdr);
    UINT16		ret;

    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    lpWaveInHdr->dwBytesRecorded = 0;

    ret = MMDRV_Message(wmld, WIDM_PREPARE, lpsegWaveInHdr, uSize, FALSE);
    return ret;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[MMSYSTEM.507]
 */
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16 hWaveIn,       /* [in] */
				      SEGPTR lpsegWaveInHdr,   /* [???] */
				      UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = MapSL(lpsegWaveInHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_UNPREPARE, lpsegWaveInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveInAddBuffer		[MMSYSTEM.508]
 */
UINT16 WINAPI waveInAddBuffer16(HWAVEIN16 hWaveIn,       /* [in] */
				WAVEHDR* lpsegWaveInHdr, /* [???] NOTE: SEGPTR */
				UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpsegWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD_PTR)lpsegWaveInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveInReset		[MMSYSTEM.511]
 */
UINT16 WINAPI waveInReset16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInReset(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInStart		[MMSYSTEM.509]
 */
UINT16 WINAPI waveInStart16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInStart(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInStop		[MMSYSTEM.510]
 */
UINT16 WINAPI waveInStop16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInStop(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInGetPosition	[MMSYSTEM.512]
 */
UINT16 WINAPI waveInGetPosition16(HWAVEIN16 hWaveIn, LPMMTIME16 lpTime,
                                  UINT16 uSize)
{
    UINT	ret;
    MMTIME	mmt;

    mmt.wType = lpTime->wType;
    ret = waveInGetPosition(HWAVEIN_32(hWaveIn), &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

/**************************************************************************
 * 				waveInGetID			[MMSYSTEM.513]
 */
UINT16 WINAPI waveInGetID16(HWAVEIN16 hWaveIn, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInMessage 		[MMSYSTEM.514]
 */
DWORD WINAPI waveInMessage16(HWAVEIN16 hWaveIn, UINT16 uMessage,
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04x, %u, %ld, %ld)\n", hWaveIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, TRUE)) != NULL) {
            if (uMessage == DRV_QUERYDRVENTRY || uMessage == DRV_QUERYDEVNODE)
                dwParam1 = (DWORD)MapSL(dwParam1);
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, FALSE);
}

/* ###################################################
 * #                     TASK                        #
 * ###################################################
 */

/*#define USE_MM_TSK_WINE*/

/**************************************************************************
 * 				mmTaskCreate		[MMSYSTEM.900]
 *
 * Creates a 16 bit MM task. It's entry point is lpFunc, and it should be
 * called upon creation with dwPmt as parameter.
 */
HINSTANCE16 WINAPI mmTaskCreate16(SEGPTR spProc, HINSTANCE16 *lphMmTask, DWORD dwPmt)
{
    HINSTANCE16 	ret;
    HINSTANCE16		handle;
    char cmdline[16];
    DWORD showCmd = 0x40002;
    LOADPARAMS16 lp;

    TRACE("(%08lx, %p, %08lx);\n", spProc, lphMmTask, dwPmt);
    /* This to work requires NE modules to be started with a binary command line
     * which is not currently the case. A patch exists but has never been committed.
     * A workaround would be to integrate code for mmtask.tsk into Wine, but
     * this requires tremendous work (starting with patching tools/build to
     * create NE executables (and not only DLLs) for builtins modules.
     * EP 99/04/25
     */
    FIXME("This is currently broken. It will fail\n");

    cmdline[0] = 0x0d;
    *(LPDWORD)(cmdline + 1) = (DWORD)spProc;
    *(LPDWORD)(cmdline + 5) = dwPmt;
    *(LPDWORD)(cmdline + 9) = 0;

    lp.hEnvironment = 0;
    lp.cmdLine = MapLS(cmdline);
    lp.showCmd = MapLS(&showCmd);
    lp.reserved = 0;

#ifndef USE_MM_TSK_WINE
    handle = LoadModule16("c:\\windows\\system\\mmtask.tsk", &lp);
#else
    handle = LoadModule16("mmtask.tsk", &lp);
#endif
    if (handle < 32) {
	ret = (handle) ? 1 : 2;
	handle = 0;
    } else {
	ret = 0;
    }
    if (lphMmTask)
	*lphMmTask = handle;

    UnMapLS( lp.cmdLine );
    UnMapLS( lp.showCmd );
    TRACE("=> 0x%04x/%d\n", handle, ret);
    return ret;
}

#ifdef USE_MM_TSK_WINE
/* C equivalent to mmtask.tsk binary content */
void	mmTaskEntryPoint16(LPSTR cmdLine, WORD di, WORD si)
{
    int	len = cmdLine[0x80];

    if (len / 2 == 6) {
	void	(*fpProc)(DWORD) = MapSL(*((DWORD*)(cmdLine + 1)));
	DWORD	dwPmt  = *((DWORD*)(cmdLine + 5));

#if 0
	InitTask16(); /* FIXME: pmts / from context ? */
	InitApp(di);
#endif
	if (SetMessageQueue16(0x40)) {
	    WaitEvent16(0);
	    if (HIWORD(fpProc)) {
		OldYield16();
/* EPP 		StackEnter16(); */
		(fpProc)(dwPmt);
	    }
	}
    }
    OldYield16();
    OldYield16();
    OldYield16();
    ExitProcess(0);
}
#endif

/**************************************************************************
 * 				mmTaskBlock		[MMSYSTEM.902]
 */
void	WINAPI	mmTaskBlock16(HINSTANCE16 WINE_UNUSED hInst)
{
    MSG		msg;

    do {
	GetMessageA(&msg, 0, 0, 0);
	if (msg.hwnd) {
	    TranslateMessage(&msg);
	    DispatchMessageA(&msg);
	}
    } while (msg.message < 0x3A0);
}

/**************************************************************************
 * 				mmTaskSignal		[MMSYSTEM.903]
 */
LRESULT	WINAPI mmTaskSignal16(HTASK16 ht)
{
    TRACE("(%04x);\n", ht);
    return PostThreadMessageW( HTASK_32(ht), WM_USER, 0, 0 );
}

/**************************************************************************
 * 				mmGetCurrentTask	[MMSYSTEM.904]
 */
HTASK16 WINAPI mmGetCurrentTask16(void)
{
    return GetCurrentTask();
}

/**************************************************************************
 * 				mmTaskYield		[MMSYSTEM.905]
 */
void	WINAPI	mmTaskYield16(void)
{
    MSG		msg;

    if (PeekMessageA(&msg, 0, 0, 0, 0)) {
	K32WOWYield16();
    }
}

extern DWORD	WINAPI	GetProcessFlags(DWORD);

/******************************************************************
 *		WINMM_GetmmThread
 *
 *
 */
static  WINE_MMTHREAD*	WINMM_GetmmThread(HANDLE16 h)
{
    return (WINE_MMTHREAD*)MapSL( MAKESEGPTR(h, 0) );
}

void WINAPI WINE_mmThreadEntryPoint(DWORD);

/**************************************************************************
 * 				mmThreadCreate		[MMSYSTEM.1120]
 *
 * undocumented
 * Creates a MM thread, calling fpThreadAddr(dwPmt).
 * dwFlags:
 * 	bit.0 set means create a 16 bit task instead of thread calling a 16 bit proc
 *	bit.1 set means to open a VxD for this thread (unsupported)
 */
LRESULT	WINAPI mmThreadCreate16(FARPROC16 fpThreadAddr, LPHANDLE16 lpHndl, DWORD dwPmt, DWORD dwFlags)
{
    HANDLE16		hndl;
    LRESULT		ret;

    TRACE("(%p, %p, %08lx, %08lx)!\n", fpThreadAddr, lpHndl, dwPmt, dwFlags);

    hndl = GlobalAlloc16(sizeof(WINE_MMTHREAD), GMEM_SHARE|GMEM_ZEROINIT);

    if (hndl == 0) {
	ret = 2;
    } else {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);

#if 0
	/* force mmtask routines even if mmthread is required */
	/* this will work only if the patch about binary cmd line and NE tasks
	 * is committed
	 */
	dwFlags |= 1;
#endif

	lpMMThd->dwSignature 	= WINE_MMTHREAD_CREATED;
	lpMMThd->dwCounter   	= 0;
	lpMMThd->hThread     	= 0;
	lpMMThd->dwThreadID  	= 0;
	lpMMThd->fpThread    	= (DWORD)fpThreadAddr;
	lpMMThd->dwThreadPmt 	= dwPmt;
	lpMMThd->dwSignalCount	= 0;
	lpMMThd->hEvent      	= 0;
	lpMMThd->hVxD        	= 0;
	lpMMThd->dwStatus    	= 0;
	lpMMThd->dwFlags     	= dwFlags;
	lpMMThd->hTask       	= 0;

	if ((dwFlags & 1) == 0 && (GetProcessFlags(GetCurrentThreadId()) & 8) == 0) {
	    lpMMThd->hEvent = CreateEventA(0, 0, 1, 0);

	    TRACE("Let's go crazy... trying new MM thread. lpMMThd=%p\n", lpMMThd);
	    if (lpMMThd->dwFlags & 2) {
		/* as long as we don't support MM VxD in wine, we don't need
		 * to care about this flag
		 */
		/* FIXME("Don't know how to properly open VxD handles\n"); */
		/* lpMMThd->hVxD = OpenVxDHandle(lpMMThd->hEvent); */
	    }

	    lpMMThd->hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WINE_mmThreadEntryPoint,
					    (LPVOID)(DWORD)hndl, CREATE_SUSPENDED, &lpMMThd->dwThreadID);
	    if (lpMMThd->hThread == 0) {
		WARN("Couldn't create thread\n");
		/* clean-up(VxDhandle...); devicedirectio... */
		if (lpMMThd->hEvent != 0)
		    CloseHandle(lpMMThd->hEvent);
		ret = 2;
	    } else {
		TRACE("Got a nice thread hndl=%p id=0x%08lx\n", lpMMThd->hThread, lpMMThd->dwThreadID);
		ret = 0;
	    }
	} else {
	    /* get WINE_mmThreadEntryPoint()
	     * 2047 is its ordinal in mmsystem.spec
	     */
	    FARPROC16	fp = GetProcAddress16(GetModuleHandle16("MMSYSTEM"), (LPCSTR)2047);

	    TRACE("farproc seg=0x%08lx lin=%p\n", (DWORD)fp, MapSL((SEGPTR)fp));

	    ret = (fp == 0) ? 2 : mmTaskCreate16((DWORD)fp, 0, hndl);
	}

	if (ret == 0) {
	    if (lpMMThd->hThread && !ResumeThread(lpMMThd->hThread))
		WARN("Couldn't resume thread\n");

	    while (lpMMThd->dwStatus != 0x10) { /* test also HIWORD of dwStatus */
		UserYield16();
	    }
	}
    }

    if (ret != 0) {
	GlobalFree16(hndl);
	hndl = 0;
    }

    if (lpHndl)
	*lpHndl = hndl;

    TRACE("ok => %ld\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadSignal		[MMSYSTEM.1121]
 */
void WINAPI mmThreadSignal16(HANDLE16 hndl)
{
    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);

	lpMMThd->dwCounter++;
	if (lpMMThd->hThread != 0) {
	    InterlockedIncrement(&lpMMThd->dwSignalCount);
	    SetEvent(lpMMThd->hEvent);
	} else {
	    mmTaskSignal16(lpMMThd->hTask);
	}
	lpMMThd->dwCounter--;
    }
}

static	void	MMSYSTEM_ThreadBlock(WINE_MMTHREAD* lpMMThd)
{
    MSG		msg;
    DWORD	ret;

    if (lpMMThd->dwThreadID != GetCurrentThreadId())
	ERR("Not called by thread itself\n");

    for (;;) {
	ResetEvent(lpMMThd->hEvent);
	if (InterlockedDecrement(&lpMMThd->dwSignalCount) >= 0)
	    break;
	InterlockedIncrement(&lpMMThd->dwSignalCount);

	TRACE("S1\n");

	ret = MsgWaitForMultipleObjects(1, &lpMMThd->hEvent, FALSE, INFINITE, QS_ALLINPUT);
	switch (ret) {
	case WAIT_OBJECT_0:	/* Event */
	    TRACE("S2.1\n");
	    break;
	case WAIT_OBJECT_0 + 1:	/* Msg */
	    TRACE("S2.2\n");
	    if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	    }
	    break;
	default:
	    WARN("S2.x unsupported ret val 0x%08lx\n", ret);
	}
	TRACE("S3\n");
    }
}

/**************************************************************************
 * 				mmThreadBlock		[MMSYSTEM.1122]
 */
void	WINAPI mmThreadBlock16(HANDLE16 hndl)
{
    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);

	if (lpMMThd->hThread != 0) {
	    DWORD	lc;

	    ReleaseThunkLock(&lc);
	    MMSYSTEM_ThreadBlock(lpMMThd);
	    RestoreThunkLock(lc);
	} else {
	    mmTaskBlock16(lpMMThd->hTask);
	}
    }
    TRACE("done\n");
}

/**************************************************************************
 * 				mmThreadIsCurrent	[MMSYSTEM.1123]
 */
BOOL16	WINAPI mmThreadIsCurrent16(HANDLE16 hndl)
{
    BOOL16		ret = FALSE;

    TRACE("(%04x)!\n", hndl);

    if (hndl && mmThreadIsValid16(hndl)) {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);
	ret = (GetCurrentThreadId() == lpMMThd->dwThreadID);
    }
    TRACE("=> %d\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadIsValid		[MMSYSTEM.1124]
 */
BOOL16	WINAPI	mmThreadIsValid16(HANDLE16 hndl)
{
    BOOL16		ret = FALSE;

    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);

	if (!IsBadWritePtr(lpMMThd, sizeof(WINE_MMTHREAD)) &&
	    lpMMThd->dwSignature == WINE_MMTHREAD_CREATED &&
	    IsTask16(lpMMThd->hTask)) {
	    lpMMThd->dwCounter++;
	    if (lpMMThd->hThread != 0) {
		DWORD	dwThreadRet;
		if (GetExitCodeThread(lpMMThd->hThread, &dwThreadRet) &&
		    dwThreadRet == STATUS_PENDING) {
		    ret = TRUE;
		}
	    } else {
		ret = TRUE;
	    }
	    lpMMThd->dwCounter--;
	}
    }
    TRACE("=> %d\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadGetTask		[MMSYSTEM.1125]
 */
HANDLE16 WINAPI mmThreadGetTask16(HANDLE16 hndl)
{
    HANDLE16	ret = 0;

    TRACE("(%04x)\n", hndl);

    if (mmThreadIsValid16(hndl)) {
	WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);
	ret = lpMMThd->hTask;
    }
    return ret;
}

/**************************************************************************
 * 			        __wine_mmThreadEntryPoint (MMSYSTEM.2047)
 */
void WINAPI WINE_mmThreadEntryPoint(DWORD _pmt)
{
    HANDLE16		hndl = (HANDLE16)_pmt;
    WINE_MMTHREAD*	lpMMThd = WINMM_GetmmThread(hndl);

    TRACE("(%04x %p)\n", hndl, lpMMThd);

    lpMMThd->hTask = LOWORD(GetCurrentTask());
    TRACE("[10-%p] setting hTask to 0x%08x\n", lpMMThd->hThread, lpMMThd->hTask);
    lpMMThd->dwStatus = 0x10;
    MMSYSTEM_ThreadBlock(lpMMThd);
    TRACE("[20-%p]\n", lpMMThd->hThread);
    lpMMThd->dwStatus = 0x20;
    if (lpMMThd->fpThread) {
	WOWCallback16(lpMMThd->fpThread, lpMMThd->dwThreadPmt);
    }
    lpMMThd->dwStatus = 0x30;
    TRACE("[30-%p]\n", lpMMThd->hThread);
    while (lpMMThd->dwCounter) {
	Sleep(1);
	/* K32WOWYield16();*/
    }
    TRACE("[XX-%p]\n", lpMMThd->hThread);
    /* paranoia */
    lpMMThd->dwSignature = WINE_MMTHREAD_DELETED;
    /* close lpMMThread->hVxD directIO */
    if (lpMMThd->hEvent)
	CloseHandle(lpMMThd->hEvent);
    GlobalFree16(hndl);
    TRACE("done\n");
}

typedef	BOOL16 (WINAPI *MMCPLCALLBACK)(HWND, LPCSTR, LPCSTR, LPCSTR);

/**************************************************************************
 * 			mmShowMMCPLPropertySheet	[MMSYSTEM.1150]
 */
BOOL16	WINAPI	mmShowMMCPLPropertySheet16(HWND hWnd, LPCSTR lpStrDevice,
					   LPCSTR lpStrTab, LPCSTR lpStrTitle)
{
    HANDLE	hndl;
    BOOL16	ret = FALSE;

    TRACE("(%p \"%s\" \"%s\" \"%s\")\n", hWnd, lpStrDevice, lpStrTab, lpStrTitle);

    hndl = LoadLibraryA("MMSYS.CPL");
    if (hndl != 0) {
	MMCPLCALLBACK	fp = (MMCPLCALLBACK)GetProcAddress(hndl, "ShowMMCPLPropertySheet");
	if (fp != NULL) {
	    DWORD	lc;
	    ReleaseThunkLock(&lc);
	    ret = (fp)(hWnd, lpStrDevice, lpStrTab, lpStrTitle);
	    RestoreThunkLock(lc);
	}
	FreeLibrary(hndl);
    }

    return ret;
}

/**************************************************************************
 * 			StackEnter		[MMSYSTEM.32]
 */
void WINAPI StackEnter16(void)
{
#ifdef __i386__
    /* mmsystem.dll from Win 95 does only this: so does Wine */
    __asm__("stc");
#endif
}

/**************************************************************************
 * 			StackLeave		[MMSYSTEM.33]
 */
void WINAPI StackLeave16(void)
{
#ifdef __i386__
    /* mmsystem.dll from Win 95 does only this: so does Wine */
    __asm__("stc");
#endif
}

/**************************************************************************
 * 			WMMMidiRunOnce	 	[MMSYSTEM.8]
 */
void WINAPI WMMMidiRunOnce16(void)
{
    FIXME("(), stub!\n");
}

/* ###################################################
 * #                    DRIVER                       #
 * ###################################################
 */

/**************************************************************************
 *				DRIVER_MapMsg32To16		[internal]
 *
 * Map a 32 bit driver message to a 16 bit driver message.
 */
static WINMM_MapType DRIVER_MapMsg32To16(WORD wMsg, DWORD* lParam1, DWORD* lParam2)
{
    WINMM_MapType       ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
    case DRV_CLOSE:	/* should be 0/0 */
    case DRV_OPEN:	/* pass through */
	/* lParam1 and lParam2 are not used */
	ret = WINMM_MAP_OK;
	break;
    case DRV_CONFIGURE:
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (conf) or to a driver (inst) or not used,
	 * lParam2 is a pointer to DRVCONFIGINFO
	 */
	if (*lParam2) {
            LPDRVCONFIGINFO16 dci16 = HeapAlloc( GetProcessHeap(), 0, sizeof(*dci16) );
            LPDRVCONFIGINFO	dci32 = (LPDRVCONFIGINFO)(*lParam2);

	    if (dci16) {
		LPSTR str1;
                INT len;
		dci16->dwDCISize = sizeof(DRVCONFIGINFO16);

                if (dci32->lpszDCISectionName) {
                    len = WideCharToMultiByte( CP_ACP, 0, dci32->lpszDCISectionName, -1, NULL, 0, NULL, NULL );
                    str1 = HeapAlloc( GetProcessHeap(), 0, len );
                    if (str1) {
                        WideCharToMultiByte( CP_ACP, 0, dci32->lpszDCISectionName, -1, str1, len, NULL, NULL );
                        dci16->lpszDCISectionName = MapLS( str1 );
                    } else {
                        return WINMM_MAP_NOMEM;
                    }
		} else {
		    dci16->lpszDCISectionName = 0L;
		}
                if (dci32->lpszDCIAliasName) {
                    len = WideCharToMultiByte( CP_ACP, 0, dci32->lpszDCIAliasName, -1, NULL, 0, NULL, NULL );
                    str1 = HeapAlloc( GetProcessHeap(), 0, len );
                    if (str1) {
                        WideCharToMultiByte( CP_ACP, 0, dci32->lpszDCIAliasName, -1, str1, len, NULL, NULL );
                        dci16->lpszDCIAliasName = MapLS( str1 );
                    } else {
                        return WINMM_MAP_NOMEM;
                    }
		} else {
		    dci16->lpszDCISectionName = 0L;
		}
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam2 = MapLS( dci16 );
	    ret = WINMM_MAP_OKMEM;
	} else {
	    ret = WINMM_MAP_OK;
	}
	break;
    default:
	if (!((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100))) {
	   FIXME("Unknown message 0x%04x\n", wMsg);
	}
	ret = WINMM_MAP_OK;
    }
    return ret;
}

/**************************************************************************
 *				DRIVER_UnMapMsg32To16		[internal]
 *
 * UnMap a 32 bit driver message to a 16 bit driver message.
 */
static WINMM_MapType DRIVER_UnMapMsg32To16(WORD wMsg, DWORD lParam1, DWORD lParam2)
{
    WINMM_MapType	ret = WINMM_MAP_MSGERROR;

    switch (wMsg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_QUERYCONFIGURE:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
    case DRV_OPEN:
    case DRV_CLOSE:
	/* lParam1 and lParam2 are not used */
	break;
    case DRV_CONFIGURE:
    case DRV_INSTALL:
	/* lParam1 is a handle to a window (or not used), lParam2 is a pointer to DRVCONFIGINFO, lParam2 */
	if (lParam2) {
	    LPDRVCONFIGINFO16	dci16 = MapSL(lParam2);
            HeapFree( GetProcessHeap(), 0, MapSL(dci16->lpszDCISectionName) );
            HeapFree( GetProcessHeap(), 0, MapSL(dci16->lpszDCIAliasName) );
            UnMapLS( lParam2 );
            UnMapLS( dci16->lpszDCISectionName );
            UnMapLS( dci16->lpszDCIAliasName );
            HeapFree( GetProcessHeap(), 0, dci16 );
	}
	ret = WINMM_MAP_OK;
	break;
    default:
	if (!((wMsg >= 0x800 && wMsg < 0x900) || (wMsg >= 0x4000 && wMsg < 0x4100))) {
	    FIXME("Unknown message 0x%04x\n", wMsg);
	}
	ret = WINMM_MAP_OK;
    }
    return ret;
}

/**************************************************************************
 *				DRIVER_TryOpenDriver16		[internal]
 *
 * Tries to load a 16 bit driver whose DLL's (module) name is lpFileName.
 */
static	LPWINE_DRIVER	DRIVER_OpenDriver16(LPCSTR fn, LPCSTR sn, LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    LPCSTR		cause = 0;

    TRACE("(%s, %08lX);\n", debugstr_a(sn), lParam2);

    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));
    if (lpDrv == NULL) {cause = "OOM"; goto exit;}

    /* FIXME: shall we do some black magic here on sn ?
     *	drivers32 => drivers
     *	mci32 => mci
     * ...
     */
    lpDrv->d.d16.hDriver16 = OpenDriver16(fn, sn, lParam2);
    if (lpDrv->d.d16.hDriver16 == 0) {cause = "Not a 16 bit driver"; goto exit;}
    lpDrv->dwFlags = WINE_GDF_16BIT;

    TRACE("=> %p\n", lpDrv);
    return lpDrv;
 exit:
    HeapFree(GetProcessHeap(), 0, lpDrv);
    TRACE("Unable to load 16 bit module %s: %s\n", debugstr_a(fn), cause);
    return NULL;
}

/******************************************************************
 *		DRIVER_SendMessage16
 *
 *
 */
static LRESULT  DRIVER_SendMessage16(HDRVR16 hDrv16, UINT msg, 
                                     LPARAM lParam1, LPARAM lParam2)
{
    LRESULT             ret = 0;
    WINMM_MapType	map;

    TRACE("Before sdm16 call hDrv=%04x wMsg=%04x p1=%08lx p2=%08lx\n",
          hDrv16, msg, lParam1, lParam2);

    switch (map = DRIVER_MapMsg32To16(msg, &lParam1, &lParam2)) {
    case WINMM_MAP_OKMEM:
    case WINMM_MAP_OK:
        ret = SendDriverMessage16(hDrv16, msg, lParam1, lParam2);
        if (map == WINMM_MAP_OKMEM)
            DRIVER_UnMapMsg32To16(msg, lParam1, lParam2);
    default:
        break;
    }
    return ret;
}

/******************************************************************
 *		DRIVER_CloseDriver16
 *
 *
 */
static LRESULT DRIVER_CloseDriver16(HDRVR16 hDrv16, LPARAM lParam1, LPARAM lParam2)
{
    return CloseDriver16(hDrv16, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvOpen	       		[MMSYSTEM.1100]
 */
HDRVR16 WINAPI DrvOpen16(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
    return OpenDriver16(lpDriverName, lpSectionName, lParam);
}

/**************************************************************************
 * 				DrvClose       		[MMSYSTEM.1101]
 */
LRESULT WINAPI DrvClose16(HDRVR16 hDrv, LPARAM lParam1, LPARAM lParam2)
{
    return CloseDriver16(hDrv, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvSendMessage		[MMSYSTEM.1102]
 */
LRESULT WINAPI DrvSendMessage16(HDRVR16 hDrv, WORD msg, LPARAM lParam1,
				LPARAM lParam2)
{
    return SendDriverMessage16(hDrv, msg, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvGetModuleHandle	[MMSYSTEM.1103]
 */
HANDLE16 WINAPI DrvGetModuleHandle16(HDRVR16 hDrv)
{
    return GetDriverModuleHandle16(hDrv);
}

/**************************************************************************
 * 				DrvDefDriverProc	[MMSYSTEM.1104]
 */
LRESULT WINAPI DrvDefDriverProc16(DWORD dwDriverID, HDRVR16 hDrv, WORD wMsg,
				  DWORD dwParam1, DWORD dwParam2)
{
    return DefDriverProc16(dwDriverID, hDrv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DriverProc			[MMSYSTEM.6]
 */
LRESULT WINAPI DriverProc16(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg,
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("dwDevID=%08lx hDrv=%04x wMsg=%04x dwParam1=%08lx dwParam2=%08lx\n",
	  dwDevID, hDrv, wMsg, dwParam1, dwParam2);

    return DrvDefDriverProc16(dwDevID, hDrv, wMsg, dwParam1, dwParam2);
}

/* ###################################################
 * #                     TIME                        #
 * ###################################################
 */

/******************************************************************
 *		MMSYSTEM_MMTIME32to16
 *
 *
 */
void MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16, const MMTIME* mmt32)
{
    mmt16->wType = mmt32->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u, which has padding
     */
    memcpy(&(mmt16->u), &(mmt32->u), sizeof(mmt16->u));
}

/******************************************************************
 *		MMSYSTEM_MMTIME16to32
 *
 *
 */
void MMSYSTEM_MMTIME16to32(LPMMTIME mmt32, const MMTIME16* mmt16)
{
    mmt32->wType = mmt16->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u, which has padding
     */
    memcpy(&(mmt32->u), &(mmt16->u), sizeof(mmt16->u));
}

/**************************************************************************
 * 				timeGetSystemTime	[MMSYSTEM.601]
 */
MMRESULT16 WINAPI timeGetSystemTime16(LPMMTIME16 lpTime, UINT16 wSize)
{
    TRACE("(%p, %u);\n", lpTime, wSize);

    if (wSize >= sizeof(*lpTime)) {
	lpTime->wType = TIME_MS;
	TIME_MMTimeStart();
	lpTime->u.ms = WINMM_SysTimeMS;

	TRACE("=> %lu\n", lpTime->u.ms);
    }

    return 0;
}

/**************************************************************************
 * 				timeSetEvent		[MMSYSTEM.602]
 */
MMRESULT16 WINAPI timeSetEvent16(UINT16 wDelay, UINT16 wResol, LPTIMECALLBACK16 lpFunc,
				 DWORD dwUser, UINT16 wFlags)
{
    if (wFlags & WINE_TIMER_IS32)
	WARN("Unknown windows flag... wine internally used.. ooch\n");

    return TIME_SetEventInternal(wDelay, wResol, (LPTIMECALLBACK)lpFunc,
                                 dwUser, wFlags & ~WINE_TIMER_IS32);
}

/**************************************************************************
 * 				timeKillEvent		[MMSYSTEM.603]
 */
MMRESULT16 WINAPI timeKillEvent16(UINT16 wID)
{
    return timeKillEvent(wID);
}

/**************************************************************************
 * 				timeGetDevCaps		[MMSYSTEM.604]
 */
MMRESULT16 WINAPI timeGetDevCaps16(LPTIMECAPS16 lpCaps, UINT16 wSize)
{
    TIMECAPS    caps;
    MMRESULT    ret;
    TRACE("(%p, %u) !\n", lpCaps, wSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = timeGetDevCaps(&caps, sizeof(caps));
    if (ret == MMSYSERR_NOERROR) {
	TIMECAPS16 tc16;
	tc16.wPeriodMin = caps.wPeriodMin;
	tc16.wPeriodMax = caps.wPeriodMax;
	memcpy(lpCaps, &tc16, min(wSize, sizeof(tc16)));
    }
    return ret;
}

/**************************************************************************
 * 				timeBeginPeriod	[MMSYSTEM.605]
 */
MMRESULT16 WINAPI timeBeginPeriod16(UINT16 wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    return timeBeginPeriod(wPeriod);
}

/**************************************************************************
 * 				timeEndPeriod		[MMSYSTEM.606]
 */
MMRESULT16 WINAPI timeEndPeriod16(UINT16 wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    return timeEndPeriod(wPeriod);
}

/**************************************************************************
 * 				mciSendString			[MMSYSTEM.702]
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrRet,
			     UINT16 uRetLen, HWND16 hwndCallback)
{
    return mciSendStringA(lpstrCommand, lpstrRet, uRetLen, HWND_32(hwndCallback));
}

/**************************************************************************
 *                    	mciLoadCommandResource			[MMSYSTEM.705]
 */
UINT16 WINAPI mciLoadCommandResource16(HINSTANCE16 hInst, LPCSTR resname, UINT16 type)
{
    HRSRC16 res;
    HGLOBAL16 handle;
    void *ptr;

    if (!(res = FindResource16( hInst, resname, (LPSTR)RT_RCDATA))) return MCI_NO_COMMAND_TABLE;
    if (!(handle = LoadResource16( hInst, res ))) return MCI_NO_COMMAND_TABLE;
    ptr = LockResource16(handle);
    return MCI_SetCommandTable(ptr, type);
    /* FIXME: FreeResource */
}

/**************************************************************************
 *                    	mciFreeCommandResource			[MMSYSTEM.713]
 */
BOOL16 WINAPI mciFreeCommandResource16(UINT16 uTable)
{
    TRACE("(%04x)!\n", uTable);

    return mciFreeCommandResource(uTable);
}

/* ###################################################
 * #                     MMIO                        #
 * ###################################################
 */

/****************************************************************
 *       		MMIO_Map32To16			[INTERNAL]
 */
static LRESULT	MMIO_Map32To16(DWORD wMsg, LPARAM* lp1, LPARAM* lp2)
{
    switch (wMsg) {
    case MMIOM_CLOSE:
    case MMIOM_SEEK:
	/* nothing to do */
	break;
    case MMIOM_OPEN:
    case MMIOM_READ:
    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
        *lp1 = MapLS( (void *)*lp1 );
	break;
    case MMIOM_RENAME:
        *lp1 = MapLS( (void *)*lp1 );
        *lp2 = MapLS( (void *)*lp2 );
        break;
    default:
        if (wMsg < MMIOM_USER)
            TRACE("Not a mappable message (%ld)\n", wMsg);
    }
    return MMSYSERR_NOERROR;
}

/****************************************************************
 *       	MMIO_UnMap32To16 			[INTERNAL]
 */
static LRESULT	MMIO_UnMap32To16(DWORD wMsg, LPARAM lParam1, LPARAM lParam2,
				 LPARAM lp1, LPARAM lp2)
{
    switch (wMsg) {
    case MMIOM_CLOSE:
    case MMIOM_SEEK:
	/* nothing to do */
	break;
    case MMIOM_OPEN:
    case MMIOM_READ:
    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
        UnMapLS( lp1 );
	break;
    case MMIOM_RENAME:
        UnMapLS( lp1 );
        UnMapLS( lp2 );
	break;
    default:
        if (wMsg < MMIOM_USER)
            TRACE("Not a mappable message (%ld)\n", wMsg);
    }
    return MMSYSERR_NOERROR;
}

/******************************************************************
 *		MMIO_Callback16
 *
 *
 */
static LRESULT MMIO_Callback16(SEGPTR cb16, LPMMIOINFO lpmmioinfo, UINT uMessage,
                               LPARAM lParam1, LPARAM lParam2)
{
    LRESULT 		result;
    MMIOINFO16          mmioInfo16;
    SEGPTR		segmmioInfo16;
    LPARAM		lp1 = lParam1, lp2 = lParam2;
    WORD args[7];

    memset(&mmioInfo16, 0, sizeof(MMIOINFO16));
    mmioInfo16.lDiskOffset = lpmmioinfo->lDiskOffset;
    mmioInfo16.adwInfo[0]  = lpmmioinfo->adwInfo[0];
    mmioInfo16.adwInfo[1]  = lpmmioinfo->adwInfo[1];
    mmioInfo16.adwInfo[2]  = lpmmioinfo->adwInfo[2];
    mmioInfo16.adwInfo[3]  = lpmmioinfo->adwInfo[3];
    /* map (lParam1, lParam2) into (lp1, lp2) 32=>16 */
    if ((result = MMIO_Map32To16(uMessage, &lp1, &lp2)) != MMSYSERR_NOERROR)
        return result;

    segmmioInfo16 = MapLS(&mmioInfo16);
    args[6] = HIWORD(segmmioInfo16);
    args[5] = LOWORD(segmmioInfo16);
    args[4] = uMessage;
    args[3] = HIWORD(lp1);
    args[2] = LOWORD(lp1);
    args[1] = HIWORD(lp2);
    args[0] = LOWORD(lp2);
    WOWCallback16Ex( cb16, WCB16_PASCAL, sizeof(args), args, &result );
    UnMapLS(segmmioInfo16);
    MMIO_UnMap32To16(uMessage, lParam1, lParam2, lp1, lp2);

    lpmmioinfo->lDiskOffset = mmioInfo16.lDiskOffset;
    lpmmioinfo->adwInfo[0]  = mmioInfo16.adwInfo[0];
    lpmmioinfo->adwInfo[1]  = mmioInfo16.adwInfo[1];
    lpmmioinfo->adwInfo[2]  = mmioInfo16.adwInfo[2];
    lpmmioinfo->adwInfo[3]  = mmioInfo16.adwInfo[3];

    return result;
}

/******************************************************************
 *             MMIO_ResetSegmentedData
 *
 */
static LRESULT     MMIO_SetSegmentedBuffer(HMMIO hmmio, SEGPTR ptr, BOOL release)
{
    LPWINE_MMIO		wm;

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;
    if (release) UnMapLS(wm->segBuffer16);
    wm->segBuffer16 = ptr;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioOpen       		[MMSYSTEM.1210]
 */
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16* lpmmioinfo16,
			  DWORD dwOpenFlags)
{
    HMMIO 	ret;

    if (lpmmioinfo16) {
	MMIOINFO	mmioinfo;

	memset(&mmioinfo, 0, sizeof(mmioinfo));

	mmioinfo.dwFlags     = lpmmioinfo16->dwFlags;
	mmioinfo.fccIOProc   = lpmmioinfo16->fccIOProc;
	mmioinfo.pIOProc     = (LPMMIOPROC)lpmmioinfo16->pIOProc;
	mmioinfo.cchBuffer   = lpmmioinfo16->cchBuffer;
	mmioinfo.pchBuffer   = MapSL((DWORD)lpmmioinfo16->pchBuffer);
        mmioinfo.adwInfo[0]  = lpmmioinfo16->adwInfo[0];
        /* if we don't have a file name, it's likely a passed open file descriptor */
        if (!szFileName) 
            mmioinfo.adwInfo[0] = (DWORD)DosFileHandleToWin32Handle(mmioinfo.adwInfo[0]);
	mmioinfo.adwInfo[1]  = lpmmioinfo16->adwInfo[1];
	mmioinfo.adwInfo[2]  = lpmmioinfo16->adwInfo[2];
	mmioinfo.adwInfo[3]  = lpmmioinfo16->adwInfo[3];

	ret = MMIO_Open(szFileName, &mmioinfo, dwOpenFlags, MMIO_PROC_16);
        MMIO_SetSegmentedBuffer(mmioinfo.hmmio, (SEGPTR)lpmmioinfo16->pchBuffer, FALSE);

	lpmmioinfo16->wErrorRet = mmioinfo.wErrorRet;
        lpmmioinfo16->hmmio     = HMMIO_16(mmioinfo.hmmio);
    } else {
	ret = MMIO_Open(szFileName, NULL, dwOpenFlags, MMIO_PROC_32A);
    }
    return HMMIO_16(ret);
}

/**************************************************************************
 * 				mmioClose      		[MMSYSTEM.1211]
 */
MMRESULT16 WINAPI mmioClose16(HMMIO16 hmmio, UINT16 uFlags)
{
    MMIO_SetSegmentedBuffer(HMMIO_32(hmmio), (SEGPTR)NULL, TRUE);
    return mmioClose(HMMIO_32(hmmio), uFlags);
}

/**************************************************************************
 * 				mmioRead	       	[MMSYSTEM.1212]
 */
LONG WINAPI mmioRead16(HMMIO16 hmmio, HPSTR pch, LONG cch)
{
    return mmioRead(HMMIO_32(hmmio), pch, cch);
}

/**************************************************************************
 * 				mmioWrite      		[MMSYSTEM.1213]
 */
LONG WINAPI mmioWrite16(HMMIO16 hmmio, HPCSTR pch, LONG cch)
{
    return mmioWrite(HMMIO_32(hmmio),pch,cch);
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek16(HMMIO16 hmmio, LONG lOffset, INT16 iOrigin)
{
    return mmioSeek(HMMIO_32(hmmio), lOffset, iOrigin);
}

/**************************************************************************
 * 				mmioGetInfo	       	[MMSYSTEM.1215]
 */
MMRESULT16 WINAPI mmioGetInfo16(HMMIO16 hmmio, MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO            mmioinfo;
    MMRESULT            ret;
    LPWINE_MMIO		wm;

    TRACE("(0x%04x,%p,0x%08x)\n", hmmio, lpmmioinfo, uFlags);

    if ((wm = MMIO_Get(HMMIO_32(hmmio))) == NULL)
	return MMSYSERR_INVALHANDLE;

    ret = mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, uFlags);
    if (ret != MMSYSERR_NOERROR) return ret;

    lpmmioinfo->dwFlags     = mmioinfo.dwFlags;
    lpmmioinfo->fccIOProc   = mmioinfo.fccIOProc;
    lpmmioinfo->pIOProc     = (wm->ioProc->type == MMIO_PROC_16) ?
        (LPMMIOPROC16)wm->ioProc->pIOProc : NULL;
    lpmmioinfo->wErrorRet   = mmioinfo.wErrorRet;
    lpmmioinfo->hTask       = HTASK_16(mmioinfo.hTask);
    lpmmioinfo->cchBuffer   = mmioinfo.cchBuffer;
    lpmmioinfo->pchBuffer   = (void*)wm->segBuffer16;
    lpmmioinfo->pchNext     = (void*)(wm->segBuffer16 + (mmioinfo.pchNext - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndRead  = (void*)(wm->segBuffer16 + (mmioinfo.pchEndRead - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndWrite = (void*)(wm->segBuffer16 + (mmioinfo.pchEndWrite - mmioinfo.pchBuffer));
    lpmmioinfo->lBufOffset  = mmioinfo.lBufOffset;
    lpmmioinfo->lDiskOffset = mmioinfo.lDiskOffset;
    lpmmioinfo->adwInfo[0]  = mmioinfo.adwInfo[0];
    lpmmioinfo->adwInfo[1]  = mmioinfo.adwInfo[1];
    lpmmioinfo->adwInfo[2]  = mmioinfo.adwInfo[2];
    lpmmioinfo->adwInfo[3]  = mmioinfo.adwInfo[3];
    lpmmioinfo->dwReserved1 = 0;
    lpmmioinfo->dwReserved2 = 0;
    lpmmioinfo->hmmio = HMMIO_16(mmioinfo.hmmio);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioSetInfo  		[MMSYSTEM.1216]
 */
MMRESULT16 WINAPI mmioSetInfo16(HMMIO16 hmmio, const MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO            mmioinfo;
    MMRESULT            ret;

    TRACE("(0x%04x,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);

    ret = mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, 0);
    if (ret != MMSYSERR_NOERROR) return ret;

    /* check if seg and lin buffers are the same */
    if (mmioinfo.cchBuffer != lpmmioinfo->cchBuffer  ||
        mmioinfo.pchBuffer != MapSL((DWORD)lpmmioinfo->pchBuffer)) 
	return MMSYSERR_INVALPARAM;

    /* check pointers coherence */
    if (lpmmioinfo->pchNext < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchNext > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer ||
	lpmmioinfo->pchEndRead < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchEndRead > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer ||
	lpmmioinfo->pchEndWrite < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchEndWrite > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer)
	return MMSYSERR_INVALPARAM;

    mmioinfo.pchNext     = mmioinfo.pchBuffer + (lpmmioinfo->pchNext     - lpmmioinfo->pchBuffer);
    mmioinfo.pchEndRead  = mmioinfo.pchBuffer + (lpmmioinfo->pchEndRead  - lpmmioinfo->pchBuffer);
    mmioinfo.pchEndWrite = mmioinfo.pchBuffer + (lpmmioinfo->pchEndWrite - lpmmioinfo->pchBuffer);

    return mmioSetInfo(HMMIO_32(hmmio), &mmioinfo, uFlags);
}

/**************************************************************************
 * 				mmioSetBuffer		[MMSYSTEM.1217]
 */
MMRESULT16 WINAPI mmioSetBuffer16(HMMIO16 hmmio, LPSTR pchBuffer,
                                  LONG cchBuffer, UINT16 uFlags)
{
    MMRESULT    ret = mmioSetBuffer(HMMIO_32(hmmio), MapSL((DWORD)pchBuffer), 
                                    cchBuffer, uFlags);

    if (ret == MMSYSERR_NOERROR)
        MMIO_SetSegmentedBuffer(HMMIO_32(hmmio), (DWORD)pchBuffer, TRUE);
    else
        UnMapLS((DWORD)pchBuffer);
    return ret;
}

/**************************************************************************
 * 				mmioFlush      		[MMSYSTEM.1218]
 */
MMRESULT16 WINAPI mmioFlush16(HMMIO16 hmmio, UINT16 uFlags)
{
    return mmioFlush(HMMIO_32(hmmio), uFlags);
}

/***********************************************************************
 * 				mmioAdvance    		[MMSYSTEM.1219]
 */
MMRESULT16 WINAPI mmioAdvance16(HMMIO16 hmmio, MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO    mmioinfo;
    LRESULT     ret;

    /* WARNING: this heavily relies on mmioAdvance implementation (for choosing which
     * fields to init
     */
    if (lpmmioinfo)
    {
        mmioinfo.pchBuffer = MapSL((DWORD)lpmmioinfo->pchBuffer);
        mmioinfo.pchNext = MapSL((DWORD)lpmmioinfo->pchNext);
        mmioinfo.dwFlags = lpmmioinfo->dwFlags;
        mmioinfo.lBufOffset = lpmmioinfo->lBufOffset;
        ret = mmioAdvance(HMMIO_32(hmmio), &mmioinfo, uFlags);
    }
    else
        ret = mmioAdvance(HMMIO_32(hmmio), NULL, uFlags);
        
    if (ret != MMSYSERR_NOERROR) return ret;

    if (lpmmioinfo)
    {
        lpmmioinfo->dwFlags = mmioinfo.dwFlags;
        lpmmioinfo->pchNext     = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchNext - mmioinfo.pchBuffer));
        lpmmioinfo->pchEndRead  = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchEndRead - mmioinfo.pchBuffer));
        lpmmioinfo->pchEndWrite = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchEndWrite - mmioinfo.pchBuffer));
        lpmmioinfo->lBufOffset  = mmioinfo.lBufOffset;
        lpmmioinfo->lDiskOffset = mmioinfo.lDiskOffset;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioStringToFOURCC	[MMSYSTEM.1220]
 */
FOURCC WINAPI mmioStringToFOURCC16(LPCSTR sz, UINT16 uFlags)
{
    return mmioStringToFOURCCA(sz, uFlags);
}

/**************************************************************************
 *              mmioInstallIOProc    [MMSYSTEM.1221]
 */
LPMMIOPROC16 WINAPI mmioInstallIOProc16(FOURCC fccIOProc, LPMMIOPROC16 pIOProc,
                                        DWORD dwFlags)
{
    return (LPMMIOPROC16)MMIO_InstallIOProc(fccIOProc, (LPMMIOPROC)pIOProc,
                                            dwFlags, MMIO_PROC_16);
}

/**************************************************************************
 * 				mmioSendMessage	[MMSYSTEM.1222]
 */
LRESULT WINAPI mmioSendMessage16(HMMIO16 hmmio, UINT16 uMessage,
				 LPARAM lParam1, LPARAM lParam2)
{
    return MMIO_SendMessage(HMMIO_32(hmmio), uMessage, 
                            lParam1, lParam2, MMIO_PROC_16);
}

/**************************************************************************
 * 				mmioDescend	       	[MMSYSTEM.1223]
 */
MMRESULT16 WINAPI mmioDescend16(HMMIO16 hmmio, LPMMCKINFO lpck,
                                const MMCKINFO* lpckParent, UINT16 uFlags)
{
    return mmioDescend(HMMIO_32(hmmio), lpck, lpckParent, uFlags);
}

/**************************************************************************
 * 				mmioAscend     		[MMSYSTEM.1224]
 */
MMRESULT16 WINAPI mmioAscend16(HMMIO16 hmmio, MMCKINFO* lpck, UINT16 uFlags)
{
    return mmioAscend(HMMIO_32(hmmio),lpck,uFlags);
}

/**************************************************************************
 * 				mmioCreateChunk		[MMSYSTEM.1225]
 */
MMRESULT16 WINAPI mmioCreateChunk16(HMMIO16 hmmio, MMCKINFO* lpck, UINT16 uFlags)
{
    return mmioCreateChunk(HMMIO_32(hmmio), lpck, uFlags);
}

/**************************************************************************
 * 				mmioRename     		[MMSYSTEM.1226]
 */
MMRESULT16 WINAPI mmioRename16(LPCSTR szFileName, LPCSTR szNewFileName,
                               MMIOINFO16* lpmmioinfo, DWORD dwRenameFlags)
{
    BOOL        inst = FALSE;
    MMRESULT    ret;
    MMIOINFO    mmioinfo;

    if (lpmmioinfo != NULL && lpmmioinfo->pIOProc != NULL && 
        lpmmioinfo->fccIOProc == 0) {
        FIXME("Can't handle this case yet\n");
        return MMSYSERR_ERROR;
    }
     
    /* this is a bit hacky, but it'll work if we get a fourCC code or nothing.
     * but a non installed ioproc without a fourcc won't do
     */
    if (lpmmioinfo && lpmmioinfo->fccIOProc && lpmmioinfo->pIOProc) {
        MMIO_InstallIOProc(lpmmioinfo->fccIOProc, (LPMMIOPROC)lpmmioinfo->pIOProc,
                           MMIO_INSTALLPROC, MMIO_PROC_16);
        inst = TRUE;
    }
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    mmioinfo.fccIOProc = lpmmioinfo->fccIOProc;
    ret = mmioRenameA(szFileName, szNewFileName, &mmioinfo, dwRenameFlags);
    if (inst) {
        MMIO_InstallIOProc(lpmmioinfo->fccIOProc, NULL,
                           MMIO_REMOVEPROC, MMIO_PROC_16);
    }
    return ret;
}

/* ###################################################
 * #                     JOYSTICK                    #
 * ###################################################
 */

/**************************************************************************
 * 				joyGetNumDevs		[MMSYSTEM.101]
 */
UINT16 WINAPI joyGetNumDevs16(void)
{
    return joyGetNumDevs();
}

/**************************************************************************
 * 				joyGetDevCaps		[MMSYSTEM.102]
 */
MMRESULT16 WINAPI joyGetDevCaps16(UINT16 wID, LPJOYCAPS16 lpCaps, UINT16 wSize)
{
    JOYCAPSA	jca;
    MMRESULT	ret = joyGetDevCapsA(wID, &jca, sizeof(jca));

    if (ret != JOYERR_NOERROR) return ret;
    lpCaps->wMid = jca.wMid;
    lpCaps->wPid = jca.wPid;
    strcpy(lpCaps->szPname, jca.szPname);
    lpCaps->wXmin = jca.wXmin;
    lpCaps->wXmax = jca.wXmax;
    lpCaps->wYmin = jca.wYmin;
    lpCaps->wYmax = jca.wYmax;
    lpCaps->wZmin = jca.wZmin;
    lpCaps->wZmax = jca.wZmax;
    lpCaps->wNumButtons = jca.wNumButtons;
    lpCaps->wPeriodMin = jca.wPeriodMin;
    lpCaps->wPeriodMax = jca.wPeriodMax;

    if (wSize >= sizeof(JOYCAPS16)) { /* Win95 extensions ? */
	lpCaps->wRmin = jca.wRmin;
	lpCaps->wRmax = jca.wRmax;
	lpCaps->wUmin = jca.wUmin;
	lpCaps->wUmax = jca.wUmax;
	lpCaps->wVmin = jca.wVmin;
	lpCaps->wVmax = jca.wVmax;
	lpCaps->wCaps = jca.wCaps;
	lpCaps->wMaxAxes = jca.wMaxAxes;
	lpCaps->wNumAxes = jca.wNumAxes;
	lpCaps->wMaxButtons = jca.wMaxButtons;
	strcpy(lpCaps->szRegKey, jca.szRegKey);
	strcpy(lpCaps->szOEMVxD, jca.szOEMVxD);
    }

    return ret;
}

/**************************************************************************
 *                              joyGetPosEx           [MMSYSTEM.110]
 */
MMRESULT16 WINAPI joyGetPosEx16(UINT16 wID, LPJOYINFOEX lpInfo)
{
    return joyGetPosEx(wID, lpInfo);
}

/**************************************************************************
 * 				joyGetPos	       	[MMSYSTEM.103]
 */
MMRESULT16 WINAPI joyGetPos16(UINT16 wID, LPJOYINFO16 lpInfo)
{
    JOYINFO	ji;
    MMRESULT	ret;

    TRACE("(%d, %p);\n", wID, lpInfo);

    if ((ret = joyGetPos(wID, &ji)) == JOYERR_NOERROR) {
	lpInfo->wXpos = ji.wXpos;
	lpInfo->wYpos = ji.wYpos;
	lpInfo->wZpos = ji.wZpos;
	lpInfo->wButtons = ji.wButtons;
    }
    return ret;
}

/**************************************************************************
 * 				joyGetThreshold		[MMSYSTEM.104]
 */
MMRESULT16 WINAPI joyGetThreshold16(UINT16 wID, LPUINT16 lpThreshold)
{
    UINT        t;
    MMRESULT    ret;

    ret = joyGetThreshold(wID, &t);
    if (ret == JOYERR_NOERROR)
        *lpThreshold = t;
    return ret;
}

/**************************************************************************
 * 				joyReleaseCapture	[MMSYSTEM.105]
 */
MMRESULT16 WINAPI joyReleaseCapture16(UINT16 wID)
{
    return joyReleaseCapture(wID);
}

/**************************************************************************
 * 				joySetCapture		[MMSYSTEM.106]
 */
MMRESULT16 WINAPI joySetCapture16(HWND16 hWnd, UINT16 wID, UINT16 wPeriod, BOOL16 bChanged)
{
    return joySetCapture16(hWnd, wID, wPeriod, bChanged);
}

/**************************************************************************
 * 				joySetThreshold		[MMSYSTEM.107]
 */
MMRESULT16 WINAPI joySetThreshold16(UINT16 wID, UINT16 wThreshold)
{
    return joySetThreshold16(wID,wThreshold);
}

/**************************************************************************
 * 				joySetCalibration	[MMSYSTEM.109]
 */
MMRESULT16 WINAPI joySetCalibration16(UINT16 wID)
{
    FIXME("(%04X): stub.\n", wID);
    return JOYERR_NOCANDO;
}
