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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

/***********************************************************************
 *           acmDriverAddA (MSACM32.@)
 */
MMRESULT WINAPI acmDriverAddA(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    TRACE("(%p, %p, %08lx, %08lx, %08lx)\n",
          phadid, hinstModule, lParam, dwPriority, fdwAdd);

    if (!phadid) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    /* Check if any unknown flags */
    if (fdwAdd &
	~(ACM_DRIVERADDF_FUNCTION|ACM_DRIVERADDF_NOTIFYHWND|
	  ACM_DRIVERADDF_GLOBAL)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    /* Check if any incompatible flags */
    if ((fdwAdd & ACM_DRIVERADDF_FUNCTION) &&
	(fdwAdd & ACM_DRIVERADDF_NOTIFYHWND)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    /* FIXME: in fact, should GetModuleFileName(hinstModule) and do a
     * LoadDriver on it, to be sure we can call SendDriverMessage on the
     * hDrvr handle.
     */
    *phadid = (HACMDRIVERID) MSACM_RegisterDriver(NULL, NULL, hinstModule);

    /* FIXME: lParam, dwPriority and fdwAdd ignored */

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverAddW (MSACM32.@)
 * FIXME
 *   Not implemented
 */
MMRESULT WINAPI acmDriverAddW(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    FIXME("(%p, %p, %ld, %ld, %ld): stub\n",
	  phadid, hinstModule, lParam, dwPriority, fdwAdd);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverClose (MSACM32.@)
 */
MMRESULT WINAPI acmDriverClose(HACMDRIVER had, DWORD fdwClose)
{
    PWINE_ACMDRIVER	pad;
    PWINE_ACMDRIVERID	padid;
    PWINE_ACMDRIVER*	tpad;

    TRACE("(%p, %08lx)\n", had, fdwClose);

    if (fdwClose) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    pad = MSACM_GetDriver(had);
    if (!pad) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    padid = pad->obj.pACMDriverID;

    /* remove driver from list */
    for (tpad = &(padid->pACMDriverList); *tpad; *tpad = (*tpad)->pNextACMDriver) {
	if (*tpad == pad) {
	    *tpad = (*tpad)->pNextACMDriver;
	    break;
	}
    }

    /* close driver if it has been opened */
    if (pad->hDrvr && !padid->hInstModule)
	CloseDriver(pad->hDrvr, 0, 0);

    HeapFree(MSACM_hHeap, 0, pad);

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverDetailsA (MSACM32.@)
 */
MMRESULT WINAPI acmDriverDetailsA(HACMDRIVERID hadid, PACMDRIVERDETAILSA padd, DWORD fdwDetails)
{
    MMRESULT mmr;
    ACMDRIVERDETAILSW	addw;

    TRACE("(%p, %p, %08lx)\n", hadid, padd, fdwDetails);

    if (!padd) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (padd->cbStruct < 4) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    addw.cbStruct = sizeof(addw);
    mmr = acmDriverDetailsW(hadid, &addw, fdwDetails);
    if (mmr == 0) {
        ACMDRIVERDETAILSA padda;

        padda.fccType = addw.fccType;
        padda.fccComp = addw.fccComp;
        padda.wMid = addw.wMid;
        padda.wPid = addw.wPid;
        padda.vdwACM = addw.vdwACM;
        padda.vdwDriver = addw.vdwDriver;
        padda.fdwSupport = addw.fdwSupport;
        padda.cFormatTags = addw.cFormatTags;
        padda.cFilterTags = addw.cFilterTags;
        padda.hicon = addw.hicon;
        WideCharToMultiByte( CP_ACP, 0, addw.szShortName, -1, padda.szShortName,
                             sizeof(padda.szShortName), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, addw.szLongName, -1, padda.szLongName,
                             sizeof(padda.szLongName), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, addw.szCopyright, -1, padda.szCopyright,
                             sizeof(padda.szCopyright), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, addw.szLicensing, -1, padda.szLicensing,
                             sizeof(padda.szLicensing), NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, addw.szFeatures, -1, padda.szFeatures,
                             sizeof(padda.szFeatures), NULL, NULL );
        memcpy(padd, &padda, min(padd->cbStruct, sizeof(*padd)));
    }
    return mmr;
}

/***********************************************************************
 *           acmDriverDetailsW (MSACM32.@)
 */
MMRESULT WINAPI acmDriverDetailsW(HACMDRIVERID hadid, PACMDRIVERDETAILSW padd, DWORD fdwDetails)
{
    HACMDRIVER acmDrvr;
    MMRESULT mmr;

    TRACE("(%p, %p, %08lx)\n", hadid, padd, fdwDetails);

    if (!padd) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (padd->cbStruct < 4) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (fdwDetails) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    mmr = acmDriverOpen(&acmDrvr, hadid, 0);
    if (mmr == MMSYSERR_NOERROR) {
        ACMDRIVERDETAILSW paddw;
        mmr = (MMRESULT)MSACM_Message(acmDrvr, ACMDM_DRIVER_DETAILS, (LPARAM)&paddw,  0);

	acmDriverClose(acmDrvr, 0);
        memcpy(padd, &paddw, min(padd->cbStruct, sizeof(*padd)));
    }

    return mmr;
}

/***********************************************************************
 *           acmDriverEnum (MSACM32.@)
 */
MMRESULT WINAPI acmDriverEnum(ACMDRIVERENUMCB fnCallback, DWORD dwInstance, DWORD fdwEnum)
{
    PWINE_ACMDRIVERID	padid;
    DWORD		fdwSupport;

    TRACE("(%p, %08lx, %08lx)\n", fnCallback, dwInstance, fdwEnum);

    if (!fnCallback) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    if (fdwEnum & ~(ACM_DRIVERENUMF_NOLOCAL|ACM_DRIVERENUMF_DISABLED)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
	fdwSupport = padid->fdwSupport;

	if (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) {
	    if (fdwEnum & ACM_DRIVERENUMF_DISABLED)
		fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
	    else
		continue;
	}
	if (!(*fnCallback)((HACMDRIVERID)padid, dwInstance, fdwSupport))
	    break;
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverID (MSACM32.@)
 */
MMRESULT WINAPI acmDriverID(HACMOBJ hao, PHACMDRIVERID phadid, DWORD fdwDriverID)
{
    PWINE_ACMOBJ pao;

    TRACE("(%p, %p, %08lx)\n", hao, phadid, fdwDriverID);

    if (fdwDriverID) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    pao = MSACM_GetObj(hao, WINE_ACMOBJ_DONTCARE);
    if (!pao) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    if (!phadid) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    *phadid = (HACMDRIVERID) pao->pACMDriverID;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverMessage (MSACM32.@)
 *
 */
LRESULT WINAPI acmDriverMessage(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p, %04x, %08lx, %08lx\n", had, uMsg, lParam1, lParam2);

    if ((uMsg >= ACMDM_USER && uMsg < ACMDM_RESERVED_LOW) ||
	uMsg == ACMDM_DRIVER_ABOUT ||
	uMsg == DRV_QUERYCONFIGURE ||
	uMsg == DRV_CONFIGURE)
	return MSACM_Message(had, uMsg, lParam1, lParam2);

    WARN("invalid parameter\n");
    return MMSYSERR_INVALPARAM;
}

/***********************************************************************
 *           acmDriverOpen (MSACM32.@)
 */
MMRESULT WINAPI acmDriverOpen(PHACMDRIVER phad, HACMDRIVERID hadid, DWORD fdwOpen)
{
    PWINE_ACMDRIVERID	padid;
    PWINE_ACMDRIVER	pad = NULL;
    MMRESULT		ret;

    TRACE("(%p, %p, %08lu)\n", phad, hadid, fdwOpen);

    if (!phad) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    if (fdwOpen) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    padid = MSACM_GetDriverID(hadid);
    if (!padid) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    pad = HeapAlloc(MSACM_hHeap, 0, sizeof(WINE_ACMDRIVER));
    if (!pad) {
        WARN("no memory\n");
        return MMSYSERR_NOMEM;
    }

    pad->obj.dwType = WINE_ACMOBJ_DRIVER;
    pad->obj.pACMDriverID = padid;

    if (!(pad->hDrvr = (HDRVR)padid->hInstModule))
    {
        ACMDRVOPENDESCW	adod;
        int		len;

	/* this is not an externally added driver... need to actually load it */
	if (!padid->pszDriverAlias)
        {
            ret = MMSYSERR_ERROR;
            goto gotError;
        }

        adod.cbStruct = sizeof(adod);
        adod.fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
        adod.fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
        adod.dwVersion = acmGetVersion();
        adod.dwFlags = fdwOpen;
        adod.dwError = 0;
        len = strlen("Drivers32") + 1;
        adod.pszSectionName = HeapAlloc(MSACM_hHeap, 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, "Drivers32", -1, (LPWSTR)adod.pszSectionName, len);
        adod.pszAliasName = padid->pszDriverAlias;
        adod.dnDevNode = 0;

        pad->hDrvr = OpenDriver(padid->pszDriverAlias, NULL, (DWORD)&adod);

        HeapFree(MSACM_hHeap, 0, (LPWSTR)adod.pszSectionName);
        if (!pad->hDrvr)
        {
            ret = adod.dwError;
            goto gotError;
        }
    }

    /* insert new pad at beg of list */
    pad->pNextACMDriver = padid->pACMDriverList;
    padid->pACMDriverList = pad;

    /* FIXME: Create a WINE_ACMDRIVER32 */
    *phad = (HACMDRIVER)pad;
    TRACE("'%s' => %08lx\n", debugstr_w(padid->pszDriverAlias), (DWORD)pad);

    return MMSYSERR_NOERROR;
 gotError:
    WARN("failed: ret = %08x\n", ret);
    if (pad && !pad->hDrvr)
	HeapFree(MSACM_hHeap, 0, pad);
    return ret;
}

/***********************************************************************
 *           acmDriverPriority (MSACM32.@)
 */
MMRESULT WINAPI acmDriverPriority(HACMDRIVERID hadid, DWORD dwPriority, DWORD fdwPriority)
{
    PWINE_ACMDRIVERID padid;
    CHAR szSubKey[17];
    CHAR szBuffer[256];
    LONG lBufferLength = sizeof(szBuffer);
    LONG lError;
    HKEY hPriorityKey;
    DWORD dwPriorityCounter;

    TRACE("(%p, %08lx, %08lx)\n", hadid, dwPriority, fdwPriority);

    padid = MSACM_GetDriverID(hadid);
    if (!padid) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    /* Check for unknown flags */
    if (fdwPriority &
	~(ACM_DRIVERPRIORITYF_ENABLE|ACM_DRIVERPRIORITYF_DISABLE|
	  ACM_DRIVERPRIORITYF_BEGIN|ACM_DRIVERPRIORITYF_END)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    /* Check for incompatible flags */
    if ((fdwPriority & ACM_DRIVERPRIORITYF_ENABLE) &&
	(fdwPriority & ACM_DRIVERPRIORITYF_DISABLE)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    /* Check for incompatible flags */
    if ((fdwPriority & ACM_DRIVERPRIORITYF_BEGIN) &&
	(fdwPriority & ACM_DRIVERPRIORITYF_END)) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    lError = RegOpenKeyA(HKEY_CURRENT_USER,
			 "Software\\Microsoft\\Multimedia\\"
			 "Audio Compression Manager\\Priority v4.00",
			 &hPriorityKey
			 );
    /* FIXME: Create key */
    if (lError != ERROR_SUCCESS) {
        WARN("RegOpenKeyA failed\n");
	return MMSYSERR_ERROR;
    }

    for (dwPriorityCounter = 1; ; dwPriorityCounter++)	{
	snprintf(szSubKey, 17, "Priority%ld", dwPriorityCounter);
	lError = RegQueryValueA(hPriorityKey, szSubKey, szBuffer, &lBufferLength);
	if (lError != ERROR_SUCCESS)
	    break;

	FIXME("(%p, %ld, %ld): stub (partial)\n",
	      hadid, dwPriority, fdwPriority);
	break;
    }

    RegCloseKey(hPriorityKey);

    WARN("RegQueryValueA failed\n");
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmDriverRemove (MSACM32.@)
 */
MMRESULT WINAPI acmDriverRemove(HACMDRIVERID hadid, DWORD fdwRemove)
{
    PWINE_ACMDRIVERID padid;

    TRACE("(%p, %08lx)\n", hadid, fdwRemove);

    padid = MSACM_GetDriverID(hadid);
    if (!padid) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    if (fdwRemove) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    MSACM_UnregisterDriver(padid);

    return MMSYSERR_NOERROR;
}
