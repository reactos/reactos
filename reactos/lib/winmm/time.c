/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYSTEM time functions
 *
 * Copyright 1993 Martin Ayotte
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
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

static    HANDLE                TIME_hMMTimer;
static    LPWINE_TIMERENTRY 	TIME_TimersList;
static    HANDLE                TIME_hKillEvent;
DWORD		                WINMM_SysTimeMS;

/*
 * FIXME
 * We're using "1" as the mininum resolution to the timer,
 * as Windows 95 does, according to the docs. Maybe it should
 * depend on the computers resources!
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)

#define MMSYSTIME_STDINTERVAL (10) /* reasonable value? */

static	void	TIME_TriggerCallBack(LPWINE_TIMERENTRY lpTimer)
{
    TRACE("before CallBack => lpFunc=%p wTimerID=%04X dwUser=%08lX !\n",
	  lpTimer->lpFunc, lpTimer->wTimerID, lpTimer->dwUser);

    /* - TimeProc callback that is called here is something strange, under Windows 3.1x it is called
     * 		during interrupt time,  is allowed to execute very limited number of API calls (like
     *	    	PostMessage), and must reside in DLL (therefore uses stack of active application). So I
     *       guess current implementation via SetTimer has to be improved upon.
     */
    switch (lpTimer->wFlags & 0x30) {
    case TIME_CALLBACK_FUNCTION:
	if (lpTimer->wFlags & WINE_TIMER_IS32)
	    (lpTimer->lpFunc)(lpTimer->wTimerID, 0, lpTimer->dwUser, 0, 0);
	else if (pFnCallMMDrvFunc16)
	    pFnCallMMDrvFunc16((DWORD)lpTimer->lpFunc, lpTimer->wTimerID, 0,
                               lpTimer->dwUser, 0, 0);
	break;
    case TIME_CALLBACK_EVENT_SET:
	SetEvent((HANDLE)lpTimer->lpFunc);
	break;
    case TIME_CALLBACK_EVENT_PULSE:
	PulseEvent((HANDLE)lpTimer->lpFunc);
	break;
    default:
	FIXME("Unknown callback type 0x%04x for mmtime callback (%p), ignored.\n",
	      lpTimer->wFlags, lpTimer->lpFunc);
	break;
    }
    TRACE("after CallBack !\n");
}

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static void CALLBACK TIME_MMSysTimeCallback(LPWINE_MM_IDATA iData)
{
static    int				nSizeLpTimers;
static    LPWINE_TIMERENTRY		lpTimers;

    LPWINE_TIMERENTRY   timer, *ptimer, *next_ptimer;
    DWORD		delta = GetTickCount() - WINMM_SysTimeMS;
    int			idx;

    TRACE("Time delta: %ld\n", delta);

    while (delta >= MMSYSTIME_MININTERVAL) {
	delta -= MMSYSTIME_MININTERVAL;
	WINMM_SysTimeMS += MMSYSTIME_MININTERVAL;

	/* since timeSetEvent() and timeKillEvent() can be called
	 * from 16 bit code, there are cases where win16 lock is
	 * locked upon entering timeSetEvent(), and then the mm timer
	 * critical section is locked. This function cannot call the
	 * timer callback with the crit sect locked (because callback
	 * may need to acquire Win16 lock, thus providing a deadlock
	 * situation).
	 * To cope with that, we just copy the WINE_TIMERENTRY struct
	 * that need to trigger the callback, and call it without the
	 * mm timer crit sect locked.
         * the hKillTimeEvent is used to mark the section where we 
         * handle the callbacks so we can do synchronous kills.
	 * EPP 99/07/13, updated 04/01/10
	 */
	idx = 0;

	EnterCriticalSection(&iData->cs);
	for (ptimer = &TIME_TimersList; *ptimer != NULL; ) {
            timer = *ptimer;
	    next_ptimer = &timer->lpNext;
	    if (timer->uCurTime < MMSYSTIME_MININTERVAL) {
		/* since lpTimer->wDelay is >= MININTERVAL, wCurTime value
		 * shall be correct (>= 0)
		 */
		timer->uCurTime += timer->wDelay - MMSYSTIME_MININTERVAL;
		if (timer->lpFunc) {
		    if (idx == nSizeLpTimers) {
			if (lpTimers) 
			    lpTimers = (LPWINE_TIMERENTRY)
                                HeapReAlloc(GetProcessHeap(), 0, lpTimers,
                                            ++nSizeLpTimers * sizeof(WINE_TIMERENTRY));
			else 
			    lpTimers = (LPWINE_TIMERENTRY)
			    HeapAlloc(GetProcessHeap(), 0,
                                      ++nSizeLpTimers * sizeof(WINE_TIMERENTRY));
		    }
		    lpTimers[idx++] = *timer;
		}
		/* TIME_ONESHOT is defined as 0 */
		if (!(timer->wFlags & TIME_PERIODIC))
                {
                    /* unlink timer from timers list */
                    *ptimer = *next_ptimer;
                    HeapFree(GetProcessHeap(), 0, timer);
                }
	    } else {
		timer->uCurTime -= MMSYSTIME_MININTERVAL;
	    }
	    ptimer = next_ptimer;
	}
        if (TIME_hKillEvent) ResetEvent(TIME_hKillEvent);
	LeaveCriticalSection(&iData->cs);

	while (idx > 0) TIME_TriggerCallBack(&lpTimers[--idx]);
        if (TIME_hKillEvent) SetEvent(TIME_hKillEvent);
    }
}

/**************************************************************************
 *           TIME_MMSysTimeThread
 */
static DWORD CALLBACK TIME_MMSysTimeThread(LPVOID arg)
{
    LPWINE_MM_IDATA iData = (LPWINE_MM_IDATA)arg;
    volatile HANDLE *pActive = (volatile HANDLE *)&TIME_hMMTimer;
    DWORD last_time, cur_time;

#ifndef __REACTOS__
    usleep(MMSYSTIME_STDINTERVAL * 1000);
#endif /* __REACTOS__ */

    last_time = GetTickCount();
    while (*pActive) {
	TIME_MMSysTimeCallback(iData);
	cur_time = GetTickCount();
	while (last_time < cur_time)
	    last_time += MMSYSTIME_STDINTERVAL;
#ifndef __REACTOS__
	usleep((last_time - cur_time) * 1000);
#endif /* __REACTOS__ */
    }
    return 0;
}

/**************************************************************************
 * 				TIME_MMTimeStart
 */
void	TIME_MMTimeStart(void)
{
    /* one could think it's possible to stop the service thread activity when no more
     * mm timers are active, but this would require to keep mmSysTimeMS up-to-date
     * without being incremented within the service thread callback.
     */
    if (!TIME_hMMTimer) {
	WINMM_SysTimeMS = GetTickCount();
	TIME_TimersList = NULL;
	TIME_hMMTimer = CreateThread(NULL, 0, TIME_MMSysTimeThread, WINMM_IData, 0, NULL);
    }
}

/**************************************************************************
 * 				TIME_MMTimeStop
 */
void	TIME_MMTimeStop(void)
{
    /* FIXME: in the worst case, we're going to wait 65 seconds here :-( */
    if (TIME_hMMTimer) {
	HANDLE hMMTimer = TIME_hMMTimer;
	TIME_hMMTimer = 0;
	WaitForSingleObject(hMMTimer, INFINITE);
	CloseHandle(hMMTimer);
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.@]
 */
MMRESULT WINAPI timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{
    TRACE("(%p, %u);\n", lpTime, wSize);

    if (wSize >= sizeof(*lpTime)) {
	TIME_MMTimeStart();
	lpTime->wType = TIME_MS;
	lpTime->u.ms = WINMM_SysTimeMS;

	TRACE("=> %lu\n", lpTime->u.ms);
    }

    return 0;
}

/**************************************************************************
 * 				TIME_SetEventInternal	[internal]
 */
WORD	TIME_SetEventInternal(UINT wDelay, UINT wResol,
                              LPTIMECALLBACK lpFunc, DWORD dwUser, UINT wFlags)
{
    WORD 		wNewID = 0;
    LPWINE_TIMERENTRY	lpNewTimer;
    LPWINE_TIMERENTRY	lpTimer;

    TRACE("(%u, %u, %p, %08lX, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    lpNewTimer = (LPWINE_TIMERENTRY)HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;

    if (wDelay < MMSYSTIME_MININTERVAL || wDelay > MMSYSTIME_MAXINTERVAL)
	return 0;

    TIME_MMTimeStart();

    lpNewTimer->uCurTime = wDelay;
    lpNewTimer->wDelay = wDelay;
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;

    EnterCriticalSection(&WINMM_IData->cs);

    if ((wFlags & TIME_KILL_SYNCHRONOUS) && !TIME_hKillEvent)
        TIME_hKillEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

    for (lpTimer = TIME_TimersList; lpTimer != NULL; lpTimer = lpTimer->lpNext) {
	wNewID = max(wNewID, lpTimer->wTimerID);
    }

    lpNewTimer->lpNext = TIME_TimersList;
    TIME_TimersList = lpNewTimer;
    lpNewTimer->wTimerID = wNewID + 1;

    LeaveCriticalSection(&WINMM_IData->cs);

    TRACE("=> %u\n", wNewID + 1);

    return wNewID + 1;
}

/**************************************************************************
 * 				timeSetEvent		[WINMM.@]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                            DWORD_PTR dwUser, UINT wFlags)
{
    if (wFlags & WINE_TIMER_IS32)
	WARN("Unknown windows flag... wine internally used.. ooch\n");

    return TIME_SetEventInternal(wDelay, wResol, lpFunc,
                                 dwUser, wFlags|WINE_TIMER_IS32);
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.@]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    LPWINE_TIMERENTRY   lpSelf = NULL, *lpTimer;

    TRACE("(%u)\n", wID);
    EnterCriticalSection(&WINMM_IData->cs);
    /* remove WINE_TIMERENTRY from list */
    for (lpTimer = &TIME_TimersList; *lpTimer; lpTimer = &(*lpTimer)->lpNext) {
	if (wID == (*lpTimer)->wTimerID) {
            lpSelf = *lpTimer;
            /* unlink timer of id 'wID' */
            *lpTimer = (*lpTimer)->lpNext;
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_IData->cs);

    if (!lpSelf)
    {
        WARN("wID=%u is not a valid timer ID\n", wID);
        return MMSYSERR_INVALPARAM;
    }
    if (lpSelf->wFlags & TIME_KILL_SYNCHRONOUS)
        WaitForSingleObject(TIME_hKillEvent, INFINITE);
    HeapFree(GetProcessHeap(), 0, lpSelf);
    return TIMERR_NOERROR;
}

/**************************************************************************
 * 				timeGetDevCaps		[WINMM.@]
 */
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize)
{
    TRACE("(%p, %u) !\n", lpCaps, wSize);

    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return 0;
}

/**************************************************************************
 * 				timeBeginPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeBeginPeriod(UINT wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeEndPeriod(UINT wPeriod)
{
    TRACE("(%u) !\n", wPeriod);

    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;
    return 0;
}

/**************************************************************************
 * 				timeGetTime    [MMSYSTEM.607]
 * 				timeGetTime    [WINMM.@]
 */
DWORD WINAPI timeGetTime(void)
{
    DWORD       count;
    /* FIXME: releasing the win16 lock here is a temporary hack (I hope)
     * that lets mciavi.drv run correctly
     */
    if (pFnReleaseThunkLock) pFnReleaseThunkLock(&count);
    TIME_MMTimeStart();
    if (pFnRestoreThunkLock) pFnRestoreThunkLock(count);
    return WINMM_SysTimeMS;
}
