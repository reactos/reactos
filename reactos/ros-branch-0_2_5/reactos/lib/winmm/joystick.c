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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "mmddk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

#define MAXJOYSTICK	(JOYSTICKID2 + 1)
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
    if (dwJoyID >= MAXJOYSTICK)
	return FALSE;
    if (JOY_Sticks[dwJoyID].hDriver)
	return TRUE;

    JOY_Sticks[dwJoyID].hDriver = OpenDriverA("joystick.drv", 0, dwJoyID);
    return (JOY_Sticks[dwJoyID].hDriver != 0);
}

/**************************************************************************
 * 				JOY_Timer		[internal]
 */
static	void	CALLBACK	JOY_Timer(HWND hWnd, UINT wMsg, UINT wTimer, DWORD dwTime)
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
 * 				joyGetNumDevs		[WINMM.@]
 */
UINT WINAPI joyGetNumDevs(void)
{
    UINT	ret = 0;
    int		i;

    for (i = 0; i < MAXJOYSTICK; i++) {
	if (JOY_LoadDriver(i)) {
	    ret += SendDriverMessage(JOY_Sticks[i].hDriver, JDD_GETNUMDEVS, 0L, 0L);
	}
    }
    return ret;
}

/**************************************************************************
 * 				joyGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI joyGetDevCapsA(UINT_PTR wID, LPJOYCAPSA lpCaps, UINT wSize)
{
    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpCaps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
    lpCaps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME (same as MS Joystick Driver) */

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETDEVCAPS, (DWORD)lpCaps, wSize);
}

/**************************************************************************
 * 				joyGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI joyGetDevCapsW(UINT_PTR wID, LPJOYCAPSW lpCaps, UINT wSize)
{
    JOYCAPSA	jca;
    MMRESULT	ret = joyGetDevCapsA(wID, &jca, sizeof(jca));

    if (ret != JOYERR_NOERROR) return ret;
    lpCaps->wMid = jca.wMid;
    lpCaps->wPid = jca.wPid;
    MultiByteToWideChar( CP_ACP, 0, jca.szPname, -1, lpCaps->szPname,
                         sizeof(lpCaps->szPname)/sizeof(WCHAR) );
    lpCaps->wXmin = jca.wXmin;
    lpCaps->wXmax = jca.wXmax;
    lpCaps->wYmin = jca.wYmin;
    lpCaps->wYmax = jca.wYmax;
    lpCaps->wZmin = jca.wZmin;
    lpCaps->wZmax = jca.wZmax;
    lpCaps->wNumButtons = jca.wNumButtons;
    lpCaps->wPeriodMin = jca.wPeriodMin;
    lpCaps->wPeriodMax = jca.wPeriodMax;

    if (wSize >= sizeof(JOYCAPSW)) { /* Win95 extensions ? */
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
        MultiByteToWideChar( CP_ACP, 0, jca.szRegKey, -1, lpCaps->szRegKey,
                         sizeof(lpCaps->szRegKey)/sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, jca.szOEMVxD, -1, lpCaps->szOEMVxD,
                         sizeof(lpCaps->szOEMVxD)/sizeof(WCHAR) );
    }

    return ret;
}

/**************************************************************************
 *                              joyGetPosEx             [WINMM.@]
 */
MMRESULT WINAPI joyGetPosEx(UINT wID, LPJOYINFOEX lpInfo)
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

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOSEX, (DWORD)lpInfo, 0L);
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

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOS, (DWORD)lpInfo, 0L);
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
