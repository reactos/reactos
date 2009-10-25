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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"

#include "winemm.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmtime);

typedef struct tagWINE_TIMERENTRY {
    struct list                 entry;
    UINT                        wDelay;
    UINT                        wResol;
    LPTIMECALLBACK              lpFunc; /* can be lots of things */
    DWORD_PTR                   dwUser;
    UINT16                      wFlags;
    UINT16                      wTimerID;
    DWORD                       dwTriggerTime;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

static struct list timer_list = LIST_INIT(timer_list);

static CRITICAL_SECTION TIME_cbcrst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &TIME_cbcrst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": TIME_cbcrst") }
};
static CRITICAL_SECTION TIME_cbcrst = { &critsect_debug, -1, 0, 0, 0, 0 };

static    HANDLE                TIME_hMMTimer;
static    BOOL                  TIME_TimeToDie = TRUE;
static    int                   TIME_fdWake[2] = { -1, -1 };

/* link timer at the appropriate spot in the list */
static inline void link_timer( WINE_TIMERENTRY *timer )
{
    WINE_TIMERENTRY *next;

    LIST_FOR_EACH_ENTRY( next, &timer_list, WINE_TIMERENTRY, entry )
        if ((int)(next->dwTriggerTime - timer->dwTriggerTime) >= 0) break;

    list_add_before( &next->entry, &timer->entry );
}

/*
 * Some observations on the behavior of winmm on Windows.
 * First, the call to timeBeginPeriod(xx) can never be used
 * to raise the timer resolution, only lower it.
 *
 * Second, a brief survey of a variety of Win 2k and Win X
 * machines showed that a 'standard' (aka default) timer
 * resolution was 1 ms (Win9x is documented as being 1).  However, one 
 * machine had a standard timer resolution of 10 ms.
 *
 * Further, if we set our default resolution to 1,
 * the implementation of timeGetTime becomes GetTickCount(),
 * and we can optimize the code to reduce overhead.
 *
 * Additionally, a survey of Event behaviors shows that
 * if we request a Periodic event every 50 ms, then Windows
 * makes sure to trigger that event 20 times in the next
 * second.  If delays prevent that from happening on exact
 * schedule, Windows will trigger the events as close
 * to the original schedule as is possible, and will eventually
 * bring the event triggers back onto a schedule that is
 * consistent with what would have happened if there were
 * no delays.
 *
 *   Jeremy White, October 2004
 */
#define MMSYSTIME_MININTERVAL (1)
#define MMSYSTIME_MAXINTERVAL (65535)

#ifdef HAVE_POLL

/**************************************************************************
 *           TIME_MMSysTimeCallback
 */
static int TIME_MMSysTimeCallback(void)
{
    WINE_TIMERENTRY *timer, *to_free;
    int delta_time;

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
     */

    EnterCriticalSection(&WINMM_cs);
    for (;;)
    {
        struct list *ptr = list_head( &timer_list );
        if (!ptr)
        {
            delta_time = -1;
            break;
        }

        timer = LIST_ENTRY( ptr, WINE_TIMERENTRY, entry );
        delta_time = timer->dwTriggerTime - GetTickCount();
        if (delta_time > 0) break;

        list_remove( &timer->entry );
        if (timer->wFlags & TIME_PERIODIC)
        {
            timer->dwTriggerTime += timer->wDelay;
            link_timer( timer );  /* restart it */
            to_free = NULL;
        }
        else to_free = timer;

        switch(timer->wFlags & (TIME_CALLBACK_EVENT_SET|TIME_CALLBACK_EVENT_PULSE))
        {
        case TIME_CALLBACK_EVENT_SET:
            SetEvent(timer->lpFunc);
            break;
        case TIME_CALLBACK_EVENT_PULSE:
            PulseEvent(timer->lpFunc);
            break;
        case TIME_CALLBACK_FUNCTION:
            {
                DWORD_PTR user = timer->dwUser;
                UINT16 id = timer->wTimerID;
                UINT16 flags = timer->wFlags;
                LPTIMECALLBACK func = timer->lpFunc;

                if (flags & TIME_KILL_SYNCHRONOUS) EnterCriticalSection(&TIME_cbcrst);
                LeaveCriticalSection(&WINMM_cs);

                func(id, 0, user, 0, 0);

                EnterCriticalSection(&WINMM_cs);
                if (flags & TIME_KILL_SYNCHRONOUS) LeaveCriticalSection(&TIME_cbcrst);
            }
            break;
        }
        HeapFree( GetProcessHeap(), 0, to_free );
    }
    LeaveCriticalSection(&WINMM_cs);
    return delta_time;
}

/**************************************************************************
 *           TIME_MMSysTimeThread
 */
static DWORD CALLBACK TIME_MMSysTimeThread(LPVOID arg)
{
    int sleep_time, ret;
    char readme[16];
    struct pollfd pfd;

    pfd.fd = TIME_fdWake[0];
    pfd.events = POLLIN;

    TRACE("Starting main winmm thread\n");

    /* FIXME:  As an optimization, we could have
               this thread die when there are no more requests
               pending, and then get recreated on the first
               new event; it's not clear if that would be worth
               it or not.                 */

    while (! TIME_TimeToDie) 
    {
        sleep_time = TIME_MMSysTimeCallback();

        if (sleep_time == 0)
            continue;

        if ((ret = poll(&pfd, 1, sleep_time)) < 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                ERR("Unexpected error in poll: %s(%d)\n", strerror(errno), errno);
                break;
            }
         }

        while (ret > 0) ret = read(TIME_fdWake[0], readme, sizeof(readme));
    }
    TRACE("Exiting main winmm thread\n");
    return 0;
}

/**************************************************************************
 * 				TIME_MMTimeStart
 */
static void TIME_MMTimeStart(void)
{
    if (!TIME_hMMTimer) {
        if (pipe(TIME_fdWake) < 0)
        {
            TIME_fdWake[0] = TIME_fdWake[1] = -1;
            ERR("Cannot create pipe: %s\n", strerror(errno));
        } else {
            fcntl(TIME_fdWake[0], F_SETFL, O_NONBLOCK);
            fcntl(TIME_fdWake[1], F_SETFL, O_NONBLOCK);
        }
        TIME_TimeToDie = FALSE;
        TIME_hMMTimer = CreateThread(NULL, 0, TIME_MMSysTimeThread, NULL, 0, NULL);
        SetThreadPriority(TIME_hMMTimer, THREAD_PRIORITY_TIME_CRITICAL);
    }
}

#else  /* HAVE_POLL */

static void TIME_MMTimeStart(void)
{
    FIXME( "not starting system thread\n" );
}

#endif  /* HAVE_POLL */

/**************************************************************************
 * 				TIME_MMTimeStop
 */
void	TIME_MMTimeStop(void)
{
    if (TIME_hMMTimer) {
        const char a='a';

        TIME_TimeToDie = TRUE;
        write(TIME_fdWake[1], &a, sizeof(a));

        WaitForSingleObject(TIME_hMMTimer, INFINITE);
        close(TIME_fdWake[0]);
        close(TIME_fdWake[1]);
        TIME_fdWake[0] = TIME_fdWake[1] = -1;
        CloseHandle(TIME_hMMTimer);
        TIME_hMMTimer = 0;
        DeleteCriticalSection(&TIME_cbcrst);
    }
}

/**************************************************************************
 * 				timeGetSystemTime	[WINMM.@]
 */
MMRESULT WINAPI timeGetSystemTime(LPMMTIME lpTime, UINT wSize)
{

    if (wSize >= sizeof(*lpTime)) {
	lpTime->wType = TIME_MS;
	lpTime->u.ms = GetTickCount();

    }

    return 0;
}

/**************************************************************************
 * 				timeSetEvent		[WINMM.@]
 */
MMRESULT WINAPI timeSetEvent(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                            DWORD_PTR dwUser, UINT wFlags)
{
    WORD 		wNewID = 0;
    LPWINE_TIMERENTRY	lpNewTimer;
    LPWINE_TIMERENTRY	lpTimer;
    const char c = 'c';

    TRACE("(%u, %u, %p, %08lX, %04X);\n", wDelay, wResol, lpFunc, dwUser, wFlags);

    if (wDelay < MMSYSTIME_MININTERVAL || wDelay > MMSYSTIME_MAXINTERVAL)
	return 0;

    lpNewTimer = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_TIMERENTRY));
    if (lpNewTimer == NULL)
	return 0;

    lpNewTimer->wDelay = wDelay;
    lpNewTimer->dwTriggerTime = GetTickCount() + wDelay;

    /* FIXME - wResol is not respected, although it is not clear
               that we could change our precision meaningfully  */
    lpNewTimer->wResol = wResol;
    lpNewTimer->lpFunc = lpFunc;
    lpNewTimer->dwUser = dwUser;
    lpNewTimer->wFlags = wFlags;

    EnterCriticalSection(&WINMM_cs);

    LIST_FOR_EACH_ENTRY( lpTimer, &timer_list, WINE_TIMERENTRY, entry )
        wNewID = max(wNewID, lpTimer->wTimerID);

    link_timer( lpNewTimer );
    lpNewTimer->wTimerID = wNewID + 1;

    TIME_MMTimeStart();

    LeaveCriticalSection(&WINMM_cs);

    /* Wake the service thread in case there is work to be done */
    write(TIME_fdWake[1], &c, sizeof(c));

    TRACE("=> %u\n", wNewID + 1);

    return wNewID + 1;
}

/**************************************************************************
 * 				timeKillEvent		[WINMM.@]
 */
MMRESULT WINAPI timeKillEvent(UINT wID)
{
    WINE_TIMERENTRY *lpSelf = NULL, *lpTimer;
    DWORD wFlags;

    TRACE("(%u)\n", wID);
    EnterCriticalSection(&WINMM_cs);
    /* remove WINE_TIMERENTRY from list */
    LIST_FOR_EACH_ENTRY( lpTimer, &timer_list, WINE_TIMERENTRY, entry )
    {
	if (wID == lpTimer->wTimerID) {
            lpSelf = lpTimer;
            list_remove( &lpTimer->entry );
	    break;
	}
    }
    LeaveCriticalSection(&WINMM_cs);

    if (!lpSelf)
    {
        WARN("wID=%u is not a valid timer ID\n", wID);
        return MMSYSERR_INVALPARAM;
    }
    wFlags = lpSelf->wFlags;
    if (wFlags & TIME_KILL_SYNCHRONOUS)
        EnterCriticalSection(&TIME_cbcrst);
    HeapFree(GetProcessHeap(), 0, lpSelf);
    if (wFlags & TIME_KILL_SYNCHRONOUS)
        LeaveCriticalSection(&TIME_cbcrst);
    return TIMERR_NOERROR;
}

/**************************************************************************
 * 				timeGetDevCaps		[WINMM.@]
 */
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize)
{
    TRACE("(%p, %u)\n", lpCaps, wSize);

    if (lpCaps == 0) {
        WARN("invalid lpCaps\n");
        return TIMERR_NOCANDO;
    }

    if (wSize < sizeof(TIMECAPS)) {
        WARN("invalid wSize\n");
        return TIMERR_NOCANDO;
    }

    lpCaps->wPeriodMin = MMSYSTIME_MININTERVAL;
    lpCaps->wPeriodMax = MMSYSTIME_MAXINTERVAL;
    return TIMERR_NOERROR;
}

/**************************************************************************
 * 				timeBeginPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeBeginPeriod(UINT wPeriod)
{
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;

    if (wPeriod > MMSYSTIME_MININTERVAL)
    {
        WARN("Stub; we set our timer resolution at minimum\n");
    }

    return 0;
}

/**************************************************************************
 * 				timeEndPeriod		[WINMM.@]
 */
MMRESULT WINAPI timeEndPeriod(UINT wPeriod)
{
    if (wPeriod < MMSYSTIME_MININTERVAL || wPeriod > MMSYSTIME_MAXINTERVAL)
	return TIMERR_NOCANDO;

    if (wPeriod > MMSYSTIME_MININTERVAL)
    {
        WARN("Stub; we set our timer resolution at minimum\n");
    }
    return 0;
}

/**************************************************************************
 * 				timeGetTime    [WINMM.@]
 */
DWORD WINAPI timeGetTime(void)
{
#if defined(COMMENTOUTPRIORTODELETING)
    DWORD       count;

    /* FIXME: releasing the win16 lock here is a temporary hack (I hope)
     * that lets mciavi32.dll run correctly
     */
    if (pFnReleaseThunkLock) pFnReleaseThunkLock(&count);
    if (pFnRestoreThunkLock) pFnRestoreThunkLock(count);
#endif

    return GetTickCount();
}
