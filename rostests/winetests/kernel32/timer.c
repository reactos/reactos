/*
 * Unit test suite for time functions
 *
 * Copyright 2004 Mike McCormack
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

#define _WIN32_WINNT 0x0501

#include "wine/test.h"
#include "winbase.h"

typedef HANDLE (WINAPI *fnCreateWaitableTimerA)( SECURITY_ATTRIBUTES*, BOOL, LPSTR );
typedef BOOL (WINAPI *fnSetWaitableTimer)(HANDLE, LARGE_INTEGER*, LONG, PTIMERAPCROUTINE, LPVOID, BOOL);


static void test_timer(void)
{
    HMODULE hker = GetModuleHandle("kernel32");
    fnCreateWaitableTimerA pCreateWaitableTimerA;
    fnSetWaitableTimer pSetWaitableTimer;
    HANDLE handle;
    BOOL r;
    LARGE_INTEGER due;

    pCreateWaitableTimerA = (fnCreateWaitableTimerA) GetProcAddress( hker, "CreateWaitableTimerA");
    if( !pCreateWaitableTimerA )
        return;

    pSetWaitableTimer = (fnSetWaitableTimer) GetProcAddress( hker, "SetWaitableTimer");
    if( !pSetWaitableTimer )
        return;

    /* try once with a positive number */
    handle = pCreateWaitableTimerA( NULL, 0, NULL );
    ok( handle != NULL, "failed to create waitable timer with no name\n" );

    due.QuadPart = 10000;
    r = pSetWaitableTimer( handle, &due, 0x1f4, NULL, NULL, FALSE );
    ok( r, "failed to set timer\n");

    CloseHandle( handle );

    /* try once with a negative number */
    handle = pCreateWaitableTimerA( NULL, 0, NULL );
    ok( handle != NULL, "failed to create waitable timer with no name\n" );

    due.QuadPart = -10000;
    r = pSetWaitableTimer( handle, &due, 0x1f4, NULL, NULL, FALSE );
    ok( r, "failed to set timer\n");

    CloseHandle( handle );
}

START_TEST(timer)
{
    test_timer();
}
