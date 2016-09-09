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

#include "wineacm.h"

/***********************************************************************
 *           acmDriverAddA (MSACM32.@)
 */
MMRESULT WINAPI acmDriverAddA(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    MMRESULT resultW;
    WCHAR * driverW = NULL;
    LPARAM lParamW = lParam;

    TRACE("(%p, %p, %08lx, %08x, %08x)\n",
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

    /* A->W translation of name */
    if ((fdwAdd & ACM_DRIVERADDF_TYPEMASK) == ACM_DRIVERADDF_NAME) {
        INT len;

        if (lParam == 0) return MMSYSERR_INVALPARAM;
        len = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lParam, -1, NULL, 0);
        driverW = HeapAlloc(MSACM_hHeap, 0, len * sizeof(WCHAR));
        if (!driverW) return MMSYSERR_NOMEM;
        MultiByteToWideChar(CP_ACP, 0, (LPSTR)lParam, -1, driverW, len);
        lParamW = (LPARAM)driverW;
    }

    resultW = acmDriverAddW(phadid, hinstModule, lParamW, dwPriority, fdwAdd);
    HeapFree(MSACM_hHeap, 0, driverW);
    return resultW;
}

/***********************************************************************
 *           acmDriverAddW (MSACM32.@)
 *
 */
MMRESULT WINAPI acmDriverAddW(PHACMDRIVERID phadid, HINSTANCE hinstModule,
			      LPARAM lParam, DWORD dwPriority, DWORD fdwAdd)
{
    PWINE_ACMLOCALDRIVER pLocalDrv = NULL;

    TRACE("(%p, %p, %08lx, %08x, %08x)\n",
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

    switch (fdwAdd & ACM_DRIVERADDF_TYPEMASK) {
    case ACM_DRIVERADDF_NAME:
        /*
                hInstModule     (unused)
                lParam          name of value in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32
                dwPriority      (unused, set to 0)
         */
        *phadid = (HACMDRIVERID) MSACM_RegisterDriverFromRegistry((LPCWSTR)lParam);        
        if (!*phadid) {
            ERR("Unable to register driver via ACM_DRIVERADDF_NAME\n");
            return MMSYSERR_INVALPARAM;
        }
        break;
    case ACM_DRIVERADDF_FUNCTION:
        /*
                hInstModule     Handle of module which contains driver entry proc
                lParam          Driver function address
                dwPriority      (unused, set to 0)
         */
        fdwAdd &= ~ACM_DRIVERADDF_TYPEMASK;
        /* FIXME: fdwAdd ignored */
        /* Application-supplied acmDriverProc's are placed at the top of the priority unless
           fdwAdd indicates ACM_DRIVERADDF_GLOBAL
         */
        pLocalDrv = MSACM_RegisterLocalDriver(hinstModule, (DRIVERPROC)lParam);
        *phadid = pLocalDrv ? (HACMDRIVERID) MSACM_RegisterDriver(NULL, NULL, pLocalDrv) : NULL;
        if (!*phadid) {
            ERR("Unable to register driver via ACM_DRIVERADDF_FUNCTION\n");
            return MMSYSERR_INVALPARAM;
        }
        break;
    case ACM_DRIVERADDF_NOTIFYHWND:
        /*
                hInstModule     (unused)
                lParam          Handle of notification window
                dwPriority      Window message to send for notification broadcasts
         */
        *phadid = (HACMDRIVERID) MSACM_RegisterNotificationWindow((HWND)lParam, dwPriority);
        if (!*phadid) {
            ERR("Unable to register driver via ACM_DRIVERADDF_NOTIFYHWND\n");
            return MMSYSERR_INVALPARAM;
        }
        break;
    default:
        ERR("invalid flag value 0x%08x for fdwAdd\n", fdwAdd);
        return MMSYSERR_INVALFLAG;
    }

    MSACM_BroadcastNotification();
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmDriverClose (MSACM32.@)
 */
MMRESULT WINAPI acmDriverClose(HACMDRIVER had, DWORD fdwClose)
{
    PWINE_ACMDRIVER	pad;
    PWINE_ACMDRIVERID	padid;
    PWINE_ACMDRIVER*	tpad;

    TRACE("(%p, %08x)\n", had, fdwClose);

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
    for (tpad = &(padid->pACMDriverList); *tpad; tpad = &((*tpad)->pNextACMDriver)) {
	if (*tpad == pad) {
	    *tpad = (*tpad)->pNextACMDriver;
	    break;
	}
    }

    /* close driver if it has been opened */
    if (pad->hDrvr && !pad->pLocalDrvrInst)
	CloseDriver(pad->hDrvr, 0, 0);
    else if (pad->pLocalDrvrInst)
        MSACM_CloseLocalDriver(pad->pLocalDrvrInst);

    pad->obj.dwType = 0;
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

    TRACE("(%p, %p, %08x)\n", hadid, padd, fdwDetails);

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
        padda.cbStruct = min(padd->cbStruct, sizeof(*padd));
        memcpy(padd, &padda, padda.cbStruct);
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

    TRACE("(%p, %p, %08x)\n", hadid, padd, fdwDetails);

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
        paddw.cbStruct = sizeof(paddw);
        mmr = MSACM_Message(acmDrvr, ACMDM_DRIVER_DETAILS, (LPARAM)&paddw,  0);

	acmDriverClose(acmDrvr, 0);
        paddw.cbStruct = min(padd->cbStruct, sizeof(*padd));
        memcpy(padd, &paddw, paddw.cbStruct);
    }
    else if (mmr == MMSYSERR_NODRIVER)
        return MMSYSERR_NOTSUPPORTED;

    return mmr;
}

/***********************************************************************
 *           acmDriverEnum (MSACM32.@)
 */
MMRESULT WINAPI acmDriverEnum(ACMDRIVERENUMCB fnCallback, DWORD_PTR dwInstance,
                              DWORD fdwEnum)
{
    PWINE_ACMDRIVERID	padid;
    DWORD		fdwSupport;

    TRACE("(%p, %08lx, %08x)\n", fnCallback, dwInstance, fdwEnum);

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

    TRACE("(%p, %p, %08x)\n", hao, phadid, fdwDriverID);

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
 * Note: MSDN documentation (July 2001) is incomplete. This function
 * accepts sending messages to an HACMDRIVERID in addition to the
 * documented HACMDRIVER. In fact, for DRV_QUERYCONFIGURE and DRV_CONFIGURE, 
 * this might actually be the required mode of operation.
 *
 * Note: For DRV_CONFIGURE, msacm supplies its own DRVCONFIGINFO structure
 * when the application fails to supply one. Some native drivers depend on
 * this and refuse to display unless a valid DRVCONFIGINFO structure is
 * built and supplied.
 */
LRESULT WINAPI acmDriverMessage(HACMDRIVER had, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p, %04x, %08lx, %08lx\n", had, uMsg, lParam1, lParam2);

    if ((uMsg >= ACMDM_USER && uMsg < ACMDM_RESERVED_LOW) ||
	uMsg == ACMDM_DRIVER_ABOUT ||
	uMsg == DRV_QUERYCONFIGURE ||
	uMsg == DRV_CONFIGURE)
    {
        PWINE_ACMDRIVERID padid;
        LRESULT lResult;
        LPDRVCONFIGINFO pConfigInfo = NULL;
        LPWSTR section_name = NULL;
        LPWSTR alias_name = NULL;

        /* Check whether handle is an HACMDRIVERID */
        padid  = MSACM_GetDriverID((HACMDRIVERID)had);
        
        /* If the message is DRV_CONFIGURE, and the application provides no
           DRVCONFIGINFO structure, msacm must supply its own.
         */
        if (uMsg == DRV_CONFIGURE && lParam2 == 0) {
            LPWSTR pAlias;
            
            /* Get the alias from the HACMDRIVERID */
            if (padid) {
                pAlias = padid->pszDriverAlias;
                if (pAlias == NULL) {
                    WARN("DRV_CONFIGURE: no alias for this driver, cannot self-supply alias\n");
                }
            } else {
                FIXME("DRV_CONFIGURE: reverse lookup HACMDRIVER -> HACMDRIVERID not implemented\n");
                pAlias = NULL;
            }
        
            if (pAlias != NULL) {
                /* DRVCONFIGINFO is only 12 bytes long, but native msacm
                 * reports a 16-byte structure to codecs, so allocate 16 bytes,
                 * just to be on the safe side.
                 */
                const unsigned int iStructSize = 16;
                pConfigInfo = HeapAlloc(MSACM_hHeap, 0, iStructSize);
                if (!pConfigInfo) {
                    ERR("OOM while supplying DRVCONFIGINFO for DRV_CONFIGURE, using NULL\n");
                } else {
                    static const WCHAR drivers32[] = {'D','r','i','v','e','r','s','3','2','\0'};

                    pConfigInfo->dwDCISize = iStructSize;

                    section_name = HeapAlloc(MSACM_hHeap, 0, (strlenW(drivers32) + 1) * sizeof(WCHAR));
                    if (section_name) strcpyW(section_name, drivers32);
                    pConfigInfo->lpszDCISectionName = section_name;
                    alias_name = HeapAlloc(MSACM_hHeap, 0, (strlenW(pAlias) + 1) * sizeof(WCHAR));
                    if (alias_name) strcpyW(alias_name, pAlias);
                    pConfigInfo->lpszDCIAliasName = alias_name;

                    if (pConfigInfo->lpszDCISectionName == NULL || pConfigInfo->lpszDCIAliasName == NULL) {
                        HeapFree(MSACM_hHeap, 0, alias_name);
                        HeapFree(MSACM_hHeap, 0, section_name);
                        HeapFree(MSACM_hHeap, 0, pConfigInfo);
                        pConfigInfo = NULL;
                        ERR("OOM while supplying DRVCONFIGINFO for DRV_CONFIGURE, using NULL\n");
                    }
                }
            }
            
            lParam2 = (LPARAM)pConfigInfo;
        }
        
        if (padid) {
            /* Handle is really an HACMDRIVERID, must have an open session to get an HACMDRIVER */
            if (padid->pACMDriverList != NULL) {
                lResult = MSACM_Message((HACMDRIVER)padid->pACMDriverList, uMsg, lParam1, lParam2);
            } else {
                MMRESULT mmr = acmDriverOpen(&had, (HACMDRIVERID)padid, 0);
                if (mmr != MMSYSERR_NOERROR) {
                    lResult = MMSYSERR_INVALPARAM;
                } else {
                    lResult = acmDriverMessage(had, uMsg, lParam1, lParam2);
                    acmDriverClose(had, 0);
                }
            }
        } else {
            lResult = MSACM_Message(had, uMsg, lParam1, lParam2);
        }
        if (pConfigInfo) {
            HeapFree(MSACM_hHeap, 0, alias_name);
            HeapFree(MSACM_hHeap, 0, section_name);
            HeapFree(MSACM_hHeap, 0, pConfigInfo);
        }
        return lResult;
    }
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

    TRACE("(%p, %p, %08u)\n", phad, hadid, fdwOpen);

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
    pad->hDrvr = 0;
    pad->pLocalDrvrInst = NULL;

    if (padid->pLocalDriver == NULL)
    {
        ACMDRVOPENDESCW	adod;
        int		len;
        LPWSTR		section_name;

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
        section_name = HeapAlloc(MSACM_hHeap, 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, "Drivers32", -1, section_name, len);
        adod.pszSectionName = section_name;
        adod.pszAliasName = padid->pszDriverAlias;
        adod.dnDevNode = 0;

        pad->hDrvr = OpenDriver(padid->pszDriverAlias, NULL, (DWORD_PTR)&adod);

        HeapFree(MSACM_hHeap, 0, section_name);
        if (!pad->hDrvr)
        {
            ret = adod.dwError;
            if (ret == MMSYSERR_NOERROR)
                ret = MMSYSERR_NODRIVER;
            goto gotError;
        }
    }
    else
    {
        ACMDRVOPENDESCW	adod;

        pad->hDrvr = NULL;

        adod.cbStruct = sizeof(adod);
        adod.fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
        adod.fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
        adod.dwVersion = acmGetVersion();
        adod.dwFlags = fdwOpen;
        adod.dwError = 0;
        adod.pszSectionName = NULL;
        adod.pszAliasName = NULL;
        adod.dnDevNode = 0;

        pad->pLocalDrvrInst = MSACM_OpenLocalDriver(padid->pLocalDriver, (DWORD_PTR)&adod);
        if (!pad->pLocalDrvrInst)
        {
            ret = adod.dwError;
            if (ret == MMSYSERR_NOERROR)
                ret = MMSYSERR_NODRIVER;
            goto gotError;
        }
    }

    /* insert new pad at beg of list */
    pad->pNextACMDriver = padid->pACMDriverList;
    padid->pACMDriverList = pad;

    /* FIXME: Create a WINE_ACMDRIVER32 */
    *phad = (HACMDRIVER)pad;
    TRACE("%s => %p\n", debugstr_w(padid->pszDriverAlias), pad);

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

    TRACE("(%p, %08x, %08x)\n", hadid, dwPriority, fdwPriority);

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
    
    /* According to MSDN, ACM_DRIVERPRIORITYF_BEGIN and ACM_DRIVERPRIORITYF_END 
       may only appear by themselves, and in addition, hadid and dwPriority must
       both be zero */
    if ((fdwPriority & ACM_DRIVERPRIORITYF_BEGIN) ||
	(fdwPriority & ACM_DRIVERPRIORITYF_END)) {
	if (fdwPriority & ~(ACM_DRIVERPRIORITYF_BEGIN|ACM_DRIVERPRIORITYF_END)) {
	    WARN("ACM_DRIVERPRIORITYF_[BEGIN|END] cannot be used with any other flags\n");
	    return MMSYSERR_INVALPARAM;
	}
	if (dwPriority) {
	    WARN("priority invalid with ACM_DRIVERPRIORITYF_[BEGIN|END]\n");
	    return MMSYSERR_INVALPARAM;
	}
	if (hadid) {
	    WARN("non-null hadid invalid with ACM_DRIVERPRIORITYF_[BEGIN|END]\n");
	    return MMSYSERR_INVALPARAM;
	}
	/* FIXME: MSDN wording suggests that deferred notification should be 
	   implemented as a system-wide lock held by a calling task, and that 
	   re-enabling notifications should broadcast them across all processes.
	   This implementation uses a simple DWORD counter. One consequence of the
	   current implementation is that applications will never see 
	   MMSYSERR_ALLOCATED as a return error.
	 */
	if (fdwPriority & ACM_DRIVERPRIORITYF_BEGIN) {
	    MSACM_DisableNotifications();
	} else if (fdwPriority & ACM_DRIVERPRIORITYF_END) {
	    MSACM_EnableNotifications();
	}
	return MMSYSERR_NOERROR;
    } else {
        PWINE_ACMDRIVERID padid;
        PWINE_ACMNOTIFYWND panwnd;
        BOOL bPerformBroadcast = FALSE;

        /* Fetch driver ID */
        padid = MSACM_GetDriverID(hadid);
        panwnd = MSACM_GetNotifyWnd(hadid);
        if (!padid && !panwnd) {
            WARN("invalid handle\n");
	    return MMSYSERR_INVALHANDLE;
        }
        
        if (padid) {
            /* Check whether driver ID is appropriate for requested op */
            if (dwPriority) {
                if (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_LOCAL) {
                    return MMSYSERR_NOTSUPPORTED;
                }
                if (dwPriority != 1 && dwPriority != (DWORD)-1) {
                    FIXME("unexpected priority %d, using sign only\n", dwPriority);
                    if ((signed)dwPriority < 0) dwPriority = (DWORD)-1;
                    if (dwPriority > 0) dwPriority = 1;
                }
                
                if (dwPriority == 1 && (padid->pPrevACMDriverID == NULL || 
                    (padid->pPrevACMDriverID->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_LOCAL))) {
                    /* do nothing - driver is first of list, or first after last
                       local driver */
                } else if (dwPriority == (DWORD)-1 && padid->pNextACMDriverID == NULL) {
                    /* do nothing - driver is last of list */
                } else {
                    MSACM_RePositionDriver(padid, dwPriority);
                    bPerformBroadcast = TRUE;
                }
            }

            /* Check whether driver ID should be enabled or disabled */
            if (fdwPriority & ACM_DRIVERPRIORITYF_DISABLE) {
                if (!(padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED)) {
                    padid->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                    bPerformBroadcast = TRUE;
                }
            } else if (fdwPriority & ACM_DRIVERPRIORITYF_ENABLE) {
                if (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) {
                    padid->fdwSupport &= ~ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                    bPerformBroadcast = TRUE;
                }
            }
        }
        
        if (panwnd) {
            if (dwPriority) {
                return MMSYSERR_NOTSUPPORTED;
            }
        
            /* Check whether notify window should be enabled or disabled */
            if (fdwPriority & ACM_DRIVERPRIORITYF_DISABLE) {
                if (!(panwnd->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED)) {
                    panwnd->fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                    bPerformBroadcast = TRUE;
                }
            } else if (fdwPriority & ACM_DRIVERPRIORITYF_ENABLE) {
                if (panwnd->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) {
                    panwnd->fdwSupport &= ~ACMDRIVERDETAILS_SUPPORTF_DISABLED;
                    bPerformBroadcast = TRUE;
                }
            }
        }
        
        /* Perform broadcast of changes */
        if (bPerformBroadcast) {
            MSACM_WriteCurrentPriorities();
            MSACM_BroadcastNotification();
        }
        return MMSYSERR_NOERROR;
    }
}

/***********************************************************************
 *           acmDriverRemove (MSACM32.@)
 */
MMRESULT WINAPI acmDriverRemove(HACMDRIVERID hadid, DWORD fdwRemove)
{
    PWINE_ACMDRIVERID padid;
    PWINE_ACMNOTIFYWND panwnd;

    TRACE("(%p, %08x)\n", hadid, fdwRemove);

    padid = MSACM_GetDriverID(hadid);
    panwnd = MSACM_GetNotifyWnd(hadid);
    if (!padid && !panwnd) {
        WARN("invalid handle\n");
	return MMSYSERR_INVALHANDLE;
    }

    if (fdwRemove) {
        WARN("invalid flag\n");
	return MMSYSERR_INVALFLAG;
    }

    if (padid) MSACM_UnregisterDriver(padid);
    if (panwnd) MSACM_UnRegisterNotificationWindow(panwnd);
    MSACM_BroadcastNotification();

    return MMSYSERR_NOERROR;
}
