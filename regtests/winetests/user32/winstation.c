/*
 * Unit tests for window stations and desktops
 *
 * Copyright 2002 Alexandre Julliard
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

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#define DESKTOP_ALL_ACCESS 0x01ff

static void print_object( HANDLE obj )
{
    char buffer[100];
    DWORD size;

    strcpy( buffer, "foobar" );
    if (!GetUserObjectInformationA( obj, UOI_NAME, buffer, sizeof(buffer), &size ))
        trace( "could not get info for %p\n", obj );
    else
        trace( "obj %p name '%s'\n", obj, buffer );
    strcpy( buffer, "foobar" );
    if (!GetUserObjectInformationA( obj, UOI_TYPE, buffer, sizeof(buffer), &size ))
        trace( "could not get type for %p\n", obj );
    else
        trace( "obj %p type '%s'\n", obj, buffer );
}

static HDESK initial_desktop;

static DWORD CALLBACK thread( LPVOID arg )
{
    HDESK d1, d2;
    HWND hwnd = CreateWindowExA(0,"BUTTON","test",WS_POPUP,0,0,100,100,GetDesktopWindow(),0,0,0);
    ok( hwnd != 0, "CreateWindow failed\n" );
    d1 = GetThreadDesktop(GetCurrentThreadId());
    trace( "thread %p desktop: %p\n", arg, d1 );
    ok( d1 == initial_desktop, "thread %p doesn't use initial desktop\n", arg );

    SetLastError( 0xdeadbeef );
    ok( !CloseHandle( d1 ), "CloseHandle succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %ld\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok( !CloseDesktop( d1 ), "CloseDesktop succeeded\n" );
    ok( GetLastError() == ERROR_BUSY, "bad last error %ld\n", GetLastError() );
    print_object( d1 );
    d2 = CreateDesktop( "foobar2", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    trace( "created desktop %p\n", d2 );
    ok( d2 != 0, "CreateDesktop failed\n" );

    SetLastError( 0xdeadbeef );
    ok( !SetThreadDesktop( d2 ), "set thread desktop succeeded with existing window\n" );
    ok( GetLastError() == ERROR_BUSY, "bad last error %ld\n", GetLastError() );

    DestroyWindow( hwnd );
    ok( SetThreadDesktop( d2 ), "set thread desktop failed\n" );
    d1 = GetThreadDesktop(GetCurrentThreadId());
    ok( d1 == d2, "GetThreadDesktop did not return set desktop %p/%p\n", d1, d2 );
    print_object( d2 );
    if (arg < (LPVOID)5)
    {
        HANDLE hthread = CreateThread( NULL, 0, thread, (char *)arg + 1, 0, NULL );
        Sleep(1000);
        WaitForSingleObject( hthread, INFINITE );
        CloseHandle( hthread );
    }
    return 0;
}

static void test_handles(void)
{
    HWINSTA w1, w2, w3;
    HDESK d1, d2, d3;
    HANDLE hthread;
    DWORD id, flags;

    /* win stations */

    w1 = GetProcessWindowStation();
    ok( GetProcessWindowStation() == w1, "GetProcessWindowStation returned different handles\n" );
    ok( !CloseWindowStation(w1), "closing process win station succeeded\n" );
    SetLastError( 0xdeadbeef );
    ok( !CloseHandle(w1), "closing process win station handle succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %ld\n", GetLastError() );
    print_object( w1 );

    flags = 0;
    ok( GetHandleInformation( w1, &flags ), "GetHandleInformation failed\n" );
    ok( !(flags & HANDLE_FLAG_PROTECT_FROM_CLOSE), "handle %p PROTECT_FROM_CLOSE set\n", w1 );

    ok( DuplicateHandle( GetCurrentProcess(), w1, GetCurrentProcess(), (PHANDLE)&w2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseWindowStation(w2), "closing dup win station failed\n" );

    ok( DuplicateHandle( GetCurrentProcess(), w1, GetCurrentProcess(), (PHANDLE)&w2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseHandle(w2), "closing dup win station handle failed\n" );

    w2 = CreateWindowStation("WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
    ok( w2 != 0, "CreateWindowStation failed\n" );
    ok( w2 != w1, "CreateWindowStation returned default handle\n" );
    SetLastError( 0xdeadbeef );
    ok( !CloseDesktop( (HDESK)w2 ), "CloseDesktop succeeded on win station\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %ld\n", GetLastError() );
    ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );

    w2 = CreateWindowStation("WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
    ok( CloseHandle( w2 ), "CloseHandle failed\n" );

    w2 = OpenWindowStation("winsta0", TRUE, WINSTA_ALL_ACCESS );
    ok( w2 != 0, "OpenWindowStation failed\n" );
    ok( w2 != w1, "OpenWindowStation returned default handle\n" );
    ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );

    w2 = OpenWindowStation("dummy name", TRUE, WINSTA_ALL_ACCESS );
    ok( !w2, "open dummy win station succeeded\n" );

    CreateMutexA( NULL, 0, "foobar" );
    w2 = CreateWindowStation("foobar", 0, WINSTA_ALL_ACCESS, NULL );
    ok( w2 != 0, "create foobar station failed\n" );

    w3 = OpenWindowStation("foobar", TRUE, WINSTA_ALL_ACCESS );
    ok( w3 != 0, "open foobar station failed\n" );
    ok( w3 != w2, "open foobar station returned same handle\n" );
    ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );
    ok( CloseWindowStation( w3 ), "CloseWindowStation failed\n" );

    w3 = OpenWindowStation("foobar", TRUE, WINSTA_ALL_ACCESS );
    ok( !w3, "open foobar station succeeded\n" );

    /* desktops */
    d1 = GetThreadDesktop(GetCurrentThreadId());
    initial_desktop = d1;
    ok( GetThreadDesktop(GetCurrentThreadId()) == d1,
        "GetThreadDesktop returned different handles\n" );

    flags = 0;
    ok( GetHandleInformation( d1, &flags ), "GetHandleInformation failed\n" );
    ok( !(flags & HANDLE_FLAG_PROTECT_FROM_CLOSE), "handle %p PROTECT_FROM_CLOSE set\n", d1 );

    SetLastError( 0xdeadbeef );
    ok( !CloseDesktop(d1), "closing thread desktop succeeded\n" );
    ok( GetLastError() == ERROR_BUSY, "bad last error %ld\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !CloseHandle(d1), "closing thread desktop handle failed\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %ld\n", GetLastError() );

    ok( DuplicateHandle( GetCurrentProcess(), d1, GetCurrentProcess(), (PHANDLE)&d2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseDesktop(d2), "closing dup desktop failed\n" );

    ok( DuplicateHandle( GetCurrentProcess(), d1, GetCurrentProcess(), (PHANDLE)&d2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseHandle(d2), "closing dup desktop handle failed\n" );

    d2 = OpenDesktop( "dummy name", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( !d2, "open dummy desktop succeeded\n" );

    d2 = CreateDesktop( "foobar", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( d2 != 0, "create foobar desktop failed\n" );
    SetLastError( 0xdeadbeef );
    ok( !CloseWindowStation( (HWINSTA)d2 ), "CloseWindowStation succeeded on desktop\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %ld\n", GetLastError() );

    d3 = OpenDesktop( "foobar", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( d3 != 0, "open foobar desktop failed\n" );
    ok( d3 != d2, "open foobar desktop returned same handle\n" );
    ok( CloseDesktop( d2 ), "CloseDesktop failed\n" );
    ok( CloseDesktop( d3 ), "CloseDesktop failed\n" );

    d3 = OpenDesktop( "foobar", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( !d3, "open foobar desktop succeeded\n" );

    ok( !CloseHandle(d1), "closing thread desktop handle succeeded\n" );
    d2 = GetThreadDesktop(GetCurrentThreadId());
    ok( d1 == d2, "got different handles after close\n" );

    trace( "thread 1 desktop: %p\n", d1 );
    print_object( d1 );
    hthread = CreateThread( NULL, 0, thread, (LPVOID)2, 0, &id );
    Sleep(1000);
    trace( "get other thread desktop: %p\n", GetThreadDesktop(id) );
    WaitForSingleObject( hthread, INFINITE );
    CloseHandle( hthread );
}

START_TEST(winstation)
{
    /* Check whether this platform supports WindowStation calls */

    SetLastError( 0xdeadbeef );
    GetProcessWindowStation();
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("WindowStation calls not supported on this platform\n");
        return;
    }

    test_handles();
}
