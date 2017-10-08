/*
 * Test winmm timer
 *
 * Copyright (c) 2005 Robert Reif
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#define NOBITMAP
#include "mmreg.h"

#include "winmm_test.h"

static TIMECAPS tc;

static void test_timeGetDevCaps(void)
{
   MMRESULT rc;

    rc = timeGetDevCaps(&tc, 0);
    ok(rc == TIMERR_NOCANDO || rc == MMSYSERR_INVALPARAM,
       "timeGetDevCaps() returned %s, should have returned TIMERR_NOCANDO "
       "or MMSYSERR_INVALPARAM\n", mmsys_error(rc));

    rc = timeGetDevCaps(0, sizeof(tc));
    ok(rc == TIMERR_NOCANDO || rc == TIMERR_STRUCT,
       "timeGetDevCaps() returned %s, should have returned TIMERR_NOCANDO "
       "or TIMERR_STRUCT\n", mmsys_error(rc));

    rc = timeGetDevCaps(0, 0);
    ok(rc == TIMERR_NOCANDO || rc == MMSYSERR_INVALPARAM,
       "timeGetDevCaps() returned %s, should have returned TIMERR_NOCANDO "
       "or MMSYSERR_INVALPARAM\n", mmsys_error(rc));

    rc = timeGetDevCaps(&tc, sizeof(tc));
    ok(rc == TIMERR_NOERROR, "timeGetDevCaps() returned %s, "
       "should have returned TIMERR_NOERROR\n", mmsys_error(rc));

    if (rc == TIMERR_NOERROR)
        trace("wPeriodMin = %u, wPeriodMax = %u\n",
              tc.wPeriodMin, tc.wPeriodMax);
}

#define NUM_SAMPLES    100

static DWORD count = 0;
static DWORD times[NUM_SAMPLES];

static void CALLBACK testTimeProc(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                                  DWORD_PTR dw1, DWORD_PTR dw2)
{
    if (count < NUM_SAMPLES)
        times[count++] = timeGetTime();
}

static void test_timer(UINT period, UINT resolution)
{
    MMRESULT rc;
    UINT i, id, delta;
    DWORD dwMin = 0xffffffff, dwMax = 0;
    double sum = 0.0;
    double deviation = 0.0;

    count = 0;

    for (i = 0; i < NUM_SAMPLES; i++)
        times[i] = 0;

    rc = timeBeginPeriod(period);
    ok(rc == TIMERR_NOERROR, "timeBeginPeriod(%u) returned %s, "
       "should have returned TIMERR_NOERROR\n", period, mmsys_error(rc));
    if (rc != TIMERR_NOERROR)
        return;

    id = timeSetEvent(period, resolution, testTimeProc, 0, TIME_PERIODIC);
    ok(id != 0, "timeSetEvent(%u, %u, %p, 0, TIME_PERIODIC) returned %d, "
       "should have returned id > 0\n", period, resolution, testTimeProc, id);
    if (id == 0)
        return;

    Sleep((NUM_SAMPLES * period) + (2 * period));

    rc = timeEndPeriod(period);
    ok(rc == TIMERR_NOERROR, "timeEndPeriod(%u) returned %s, "
       "should have returned TIMERR_NOERROR\n", period, mmsys_error(rc));
    if (rc != TIMERR_NOERROR)
        return;

    rc = timeKillEvent(id);
    ok(rc == TIMERR_NOERROR, "timeKillEvent(%u) returned %s, "
       "should have returned TIMERR_NOERROR\n", id, mmsys_error(rc));

    trace("period = %u, resolution = %u\n", period, resolution);

    for (i = 0; i < count; i++)
    {
        if (i == 0)
        {
            if (winetest_debug > 1)
                trace("time[%d] = %u\n", i, times[i]);
        }
        else
        {
            delta = times[i] - times[i - 1];

            if (winetest_debug > 1)
                trace("time[%d] = %u delta = %d\n", i, times[i], delta);

            sum += delta;
            deviation += ((delta - period) * (delta - period));

            if (delta < dwMin)
                dwMin = delta;

            if (delta > dwMax)
                dwMax = delta;
        }
    }

    trace("min = %u, max = %u, average = %f, standard deviation = %f\n",
          dwMin, dwMax, sum / (count - 1), sqrt(deviation / (count - 2)));
}

static const char * get_priority(int priority)
{
    static char     tmp[32];
#define STR(x) case x: return #x
    switch(priority) {
    STR(THREAD_PRIORITY_LOWEST);
    STR(THREAD_PRIORITY_BELOW_NORMAL);
    STR(THREAD_PRIORITY_NORMAL);
    STR(THREAD_PRIORITY_HIGHEST);
    STR(THREAD_PRIORITY_ABOVE_NORMAL);
    STR(THREAD_PRIORITY_TIME_CRITICAL);
    STR(THREAD_PRIORITY_IDLE);
    }
    sprintf(tmp, "UNKNOWN(%d)", priority);
    return tmp;
}

static int priority = 0;
static BOOL fired = FALSE;

static void CALLBACK priorityTimeProc(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                                      DWORD_PTR dw1, DWORD_PTR dw2)
{
    priority = GetThreadPriority(GetCurrentThread());
    ok(priority!=THREAD_PRIORITY_ERROR_RETURN, "GetThreadPriority() failed, GetLastError() = %u\n", GetLastError());
    fired = TRUE;
}

static void test_priority(void)
{
    UINT id;

    id = timeSetEvent(100, 100, priorityTimeProc, 0, TIME_ONESHOT);
    ok(id != 0, "timeSetEvent(100, 100, %p, 0, TIME_ONESHOT) returned %d, "
       "should have returned id > 0\n", priorityTimeProc, id);
    if (id == 0)
        return;

    Sleep(200);

    ok(fired == TRUE, "Callback not called\n");
    if (fired)
    {
        ok(priority == THREAD_PRIORITY_TIME_CRITICAL,
           "thread priority is %s, should be THREAD_PRIORITY_TIME_CRITICAL\n",
           get_priority(priority));
    }
    timeKillEvent(id);
}

START_TEST(timer)
{
    test_timeGetDevCaps();

    if (tc.wPeriodMin <= 1) {
        test_timer(1, 0);
        test_timer(1, 1);
    }

    if (tc.wPeriodMin <= 10) {
        test_timer(10, 0);
        test_timer(10, 1);
        test_timer(10, 10);
    }

    if (tc.wPeriodMin <= 20) {
        test_timer(20, 0);
        test_timer(20, 1);
        test_timer(20, 10);
        test_timer(20, 20);
    }

    test_priority();
}
