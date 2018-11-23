/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * joystick functions
 *
 * Copyright 1997 Andreas Mohr
 *	     2000 Wolfgang Schwotzer
 *                Eric Pouech
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdlib.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

#define MAXJOYSTICK (JOYSTICKID2 + 30)
#define JOY_PERIOD_MIN	(10)	/* min Capture time period */
#define JOY_PERIOD_MAX	(1000)	/* max Capture time period */

typedef struct tagWINE_JOYSTICK {
    JOYINFO	ji;
    HWND	hCapture;
    UINT	wTimer;
    DWORD	threshold;
    BOOL	bChanged;
    HDRVR	hDriver;
} WINE_JOYSTICK;

static	WINE_JOYSTICK	JOY_Sticks[MAXJOYSTICK];

/**************************************************************************
 * 				JOY_LoadDriver		[internal]
 */
static	BOOL JOY_LoadDriver(DWORD dwJoyID)
{
    static BOOL winejoystick_missing = FALSE;

    if (dwJoyID >= MAXJOYSTICK || winejoystick_missing)
	return FALSE;
    if (JOY_Sticks[dwJoyID].hDriver)
	return TRUE;

    JOY_Sticks[dwJoyID].hDriver = OpenDriverA("winejoystick.drv", 0, dwJoyID);

    if (!JOY_Sticks[dwJoyID].hDriver)
    {
        /* The default driver is missing, don't attempt to load it again */
        winejoystick_missing = TRUE;
    }

    return (JOY_Sticks[dwJoyID].hDriver != 0);
}

/**************************************************************************
 * 				JOY_Timer		[internal]
 */
static	void	CALLBACK	JOY_Timer(HWND hWnd, UINT wMsg, UINT_PTR wTimer, DWORD dwTime)
{
    int			i;
    WINE_JOYSTICK*	joy;
    JOYINFO		ji;
    LONG		pos;
    unsigned 		buttonChange;

    for (i = 0; i < MAXJOYSTICK; i++) {
	joy = &JOY_Sticks[i];

	if (joy->hCapture != hWnd) continue;

	joyGetPos(i, &ji);
	pos = MAKELONG(ji.wXpos, ji.wYpos);

	if (!joy->bChanged ||
	    abs(joy->ji.wXpos - ji.wXpos) > joy->threshold ||
	    abs(joy->ji.wYpos - ji.wYpos) > joy->threshold) {
	    SendMessageA(joy->hCapture, MM_JOY1MOVE + i, ji.wButtons, pos);
	    joy->ji.wXpos = ji.wXpos;
	    joy->ji.wYpos = ji.wYpos;
	}
	if (!joy->bChanged ||
	    abs(joy->ji.wZpos - ji.wZpos) > joy->threshold) {
	    SendMessageA(joy->hCapture, MM_JOY1ZMOVE + i, ji.wButtons, pos);
	    joy->ji.wZpos = ji.wZpos;
	}
	if ((buttonChange = joy->ji.wButtons ^ ji.wButtons) != 0) {
	    if (ji.wButtons & buttonChange)
		SendMessageA(joy->hCapture, MM_JOY1BUTTONDOWN + i,
			     (buttonChange << 8) | (ji.wButtons & buttonChange), pos);
	    if (joy->ji.wButtons & buttonChange)
		SendMessageA(joy->hCapture, MM_JOY1BUTTONUP + i,
			     (buttonChange << 8) | (joy->ji.wButtons & buttonChange), pos);
	    joy->ji.wButtons = ji.wButtons;
	}
    }
}

/**************************************************************************
 *                              joyConfigChanged        [WINMM.@]
 */
MMRESULT WINAPI joyConfigChanged(DWORD flags)
{
    FIXME("(%x) - stub\n", flags);

    if (flags)
	return JOYERR_PARMS;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetNumDevs		[WINMM.@]
 */
UINT WINAPI DECLSPEC_HOTPATCH joyGetNumDevs(void)
{
    UINT	ret = 0;
    int		i;

    for (i = 0; i < MAXJOYSTICK; i++) {
	if (JOY_LoadDriver(i)) {
            ret += SendDriverMessage(JOY_Sticks[i].hDriver, JDD_GETNUMDEVS, 0, 0);
	}
    }
    return ret;
}

/**************************************************************************
 * 				joyGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsW(UINT_PTR wID, LPJOYCAPSW lpCaps, UINT wSize)
{
    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpCaps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
    lpCaps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME (same as MS Joystick Driver) */

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETDEVCAPS, (LPARAM)lpCaps, wSize);
}

/**************************************************************************
 * 				joyGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsA(UINT_PTR wID, LPJOYCAPSA lpCaps, UINT wSize)
{
    JOYCAPSW	jcw;
    MMRESULT	ret;

    if (lpCaps == NULL) return MMSYSERR_INVALPARAM;

    ret = joyGetDevCapsW(wID, &jcw, sizeof(jcw));

    if (ret == JOYERR_NOERROR)
    {
        lpCaps->wMid = jcw.wMid;
        lpCaps->wPid = jcw.wPid;
        WideCharToMultiByte( CP_ACP, 0, jcw.szPname, -1, lpCaps->szPname,
                             sizeof(lpCaps->szPname), NULL, NULL );
        lpCaps->wXmin = jcw.wXmin;
        lpCaps->wXmax = jcw.wXmax;
        lpCaps->wYmin = jcw.wYmin;
        lpCaps->wYmax = jcw.wYmax;
        lpCaps->wZmin = jcw.wZmin;
        lpCaps->wZmax = jcw.wZmax;
        lpCaps->wNumButtons = jcw.wNumButtons;
        lpCaps->wPeriodMin = jcw.wPeriodMin;
        lpCaps->wPeriodMax = jcw.wPeriodMax;

        if (wSize >= sizeof(JOYCAPSA)) { /* Win95 extensions ? */
            lpCaps->wRmin = jcw.wRmin;
            lpCaps->wRmax = jcw.wRmax;
            lpCaps->wUmin = jcw.wUmin;
            lpCaps->wUmax = jcw.wUmax;
            lpCaps->wVmin = jcw.wVmin;
            lpCaps->wVmax = jcw.wVmax;
            lpCaps->wCaps = jcw.wCaps;
            lpCaps->wMaxAxes = jcw.wMaxAxes;
            lpCaps->wNumAxes = jcw.wNumAxes;
            lpCaps->wMaxButtons = jcw.wMaxButtons;
            WideCharToMultiByte( CP_ACP, 0, jcw.szRegKey, -1, lpCaps->szRegKey,
                                 sizeof(lpCaps->szRegKey), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, jcw.szOEMVxD, -1, lpCaps->szOEMVxD,
                                 sizeof(lpCaps->szOEMVxD), NULL, NULL );
        }
    }

    return ret;
}

/**************************************************************************
 *                              joyGetPosEx             [WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetPosEx(UINT wID, LPJOYINFOEX lpInfo)
{
    TRACE("(%d, %p);\n", wID, lpInfo);

    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpInfo->dwXpos = 0;
    lpInfo->dwYpos = 0;
    lpInfo->dwZpos = 0;
    lpInfo->dwRpos = 0;
    lpInfo->dwUpos = 0;
    lpInfo->dwVpos = 0;
    lpInfo->dwButtons = 0;
    lpInfo->dwButtonNumber = 0;
    lpInfo->dwPOV = 0;
    lpInfo->dwReserved1 = 0;
    lpInfo->dwReserved2 = 0;

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOSEX, (LPARAM)lpInfo, 0);
}

/**************************************************************************
 * 				joyGetPos	       	[WINMM.@]
 */
MMRESULT WINAPI joyGetPos(UINT wID, LPJOYINFO lpInfo)
{
    TRACE("(%d, %p);\n", wID, lpInfo);

    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpInfo->wXpos = 0;
    lpInfo->wYpos = 0;
    lpInfo->wZpos = 0;
    lpInfo->wButtons = 0;

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOS, (LPARAM)lpInfo, 0);
}

/**************************************************************************
 * 				joyGetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joyGetThreshold(UINT wID, LPUINT lpThreshold)
{
    TRACE("(%04X, %p);\n", wID, lpThreshold);

    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;

    *lpThreshold = JOY_Sticks[wID].threshold;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyReleaseCapture	[WINMM.@]
 */
MMRESULT WINAPI joyReleaseCapture(UINT wID)
{
    TRACE("(%04X);\n", wID);

    if (wID >= MAXJOYSTICK)		return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))		return MMSYSERR_NODRIVER;
    if (!JOY_Sticks[wID].hCapture)	return JOYERR_NOCANDO;

    KillTimer(JOY_Sticks[wID].hCapture, JOY_Sticks[wID].wTimer);
    JOY_Sticks[wID].hCapture = 0;
    JOY_Sticks[wID].wTimer = 0;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetCapture		[WINMM.@]
 */
MMRESULT WINAPI joySetCapture(HWND hWnd, UINT wID, UINT wPeriod, BOOL bChanged)
{
    TRACE("(%p, %04X, %d, %d);\n",  hWnd, wID, wPeriod, bChanged);

    if (wID >= MAXJOYSTICK || hWnd == 0) return JOYERR_PARMS;
    if (wPeriod<JOY_PERIOD_MIN || wPeriod>JOY_PERIOD_MAX) return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID)) return MMSYSERR_NODRIVER;

    if (JOY_Sticks[wID].hCapture || !IsWindow(hWnd))
	return JOYERR_NOCANDO; /* FIXME: what should be returned ? */

    if (joyGetPos(wID, &JOY_Sticks[wID].ji) != JOYERR_NOERROR)
	return JOYERR_UNPLUGGED;

    if ((JOY_Sticks[wID].wTimer = SetTimer(hWnd, 0, wPeriod, JOY_Timer)) == 0)
	return JOYERR_NOCANDO;

    JOY_Sticks[wID].hCapture = hWnd;
    JOY_Sticks[wID].bChanged = bChanged;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joySetThreshold(UINT wID, UINT wThreshold)
{
    TRACE("(%04X, %d);\n", wID, wThreshold);

    if (wID >= MAXJOYSTICK) return MMSYSERR_INVALPARAM;

    JOY_Sticks[wID].threshold = wThreshold;

    return JOYERR_NOERROR;
}
