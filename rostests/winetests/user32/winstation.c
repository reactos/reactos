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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

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

static void register_class(void)
{
    WNDCLASSA cls;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "WinStationClass";
    RegisterClassA(&cls);
}

static HDESK initial_desktop;

static DWORD CALLBACK thread( LPVOID arg )
{
    HDESK d1, d2;
    HWND hwnd = CreateWindowExA(0,"WinStationClass","test",WS_POPUP,0,0,100,100,GetDesktopWindow(),0,0,0);
    ok( hwnd != 0, "CreateWindow failed\n" );
    d1 = GetThreadDesktop(GetCurrentThreadId());
    trace( "thread %p desktop: %p\n", arg, d1 );
    ok( d1 == initial_desktop, "thread %p doesn't use initial desktop\n", arg );

    SetLastError( 0xdeadbeef );
    ok( !CloseHandle( d1 ), "CloseHandle succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %d\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok( !CloseDesktop( d1 ), "CloseDesktop succeeded\n" );
    ok( GetLastError() == ERROR_BUSY || broken(GetLastError() == 0xdeadbeef), /* wow64 */
        "bad last error %d\n", GetLastError() );
    print_object( d1 );
    d2 = CreateDesktopA( "foobar2", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    trace( "created desktop %p\n", d2 );
    ok( d2 != 0, "CreateDesktop failed\n" );

    SetLastError( 0xdeadbeef );
    ok( !SetThreadDesktop( d2 ), "set thread desktop succeeded with existing window\n" );
    ok( GetLastError() == ERROR_BUSY || broken(GetLastError() == 0xdeadbeef), /* wow64 */
        "bad last error %d\n", GetLastError() );

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
    DWORD id, flags, le;
    ATOM atom;
    char buffer[20];

    /* win stations */

    w1 = GetProcessWindowStation();
    ok( GetProcessWindowStation() == w1, "GetProcessWindowStation returned different handles\n" );
    ok( !CloseWindowStation(w1), "closing process win station succeeded\n" );
    SetLastError( 0xdeadbeef );
    ok( !CloseHandle(w1), "closing process win station handle succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %d\n", GetLastError() );
    print_object( w1 );

    flags = 0;
    ok( GetHandleInformation( w1, &flags ), "GetHandleInformation failed\n" );
    ok( !(flags & HANDLE_FLAG_PROTECT_FROM_CLOSE) ||
        broken(flags & HANDLE_FLAG_PROTECT_FROM_CLOSE), /* set on nt4 */
        "handle %p PROTECT_FROM_CLOSE set\n", w1 );

    ok( DuplicateHandle( GetCurrentProcess(), w1, GetCurrentProcess(), (PHANDLE)&w2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseWindowStation(w2), "closing dup win station failed\n" );

    ok( DuplicateHandle( GetCurrentProcess(), w1, GetCurrentProcess(), (PHANDLE)&w2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseHandle(w2), "closing dup win station handle failed\n" );

    w2 = CreateWindowStationA("WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
    le = GetLastError();
    ok( w2 != 0 || le == ERROR_ACCESS_DENIED, "CreateWindowStation failed (%u)\n", le );
    if (w2 != 0)
    {
        ok( w2 != w1, "CreateWindowStation returned default handle\n" );
        SetLastError( 0xdeadbeef );
        ok( !CloseDesktop( (HDESK)w2 ), "CloseDesktop succeeded on win station\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == 0xdeadbeef), /* wow64 */
            "bad last error %d\n", GetLastError() );
        ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );

        w2 = CreateWindowStationA("WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
        ok( CloseHandle( w2 ), "CloseHandle failed\n" );
    }
    else if (le == ERROR_ACCESS_DENIED)
        win_skip( "Not enough privileges for CreateWindowStation\n" );

    w2 = OpenWindowStationA("winsta0", TRUE, WINSTA_ALL_ACCESS );
    ok( w2 != 0, "OpenWindowStation failed\n" );
    ok( w2 != w1, "OpenWindowStation returned default handle\n" );
    ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );

    w2 = OpenWindowStationA("dummy name", TRUE, WINSTA_ALL_ACCESS );
    ok( !w2, "open dummy win station succeeded\n" );

    CreateMutexA( NULL, 0, "foobar" );
    w2 = CreateWindowStationA("foobar", 0, WINSTA_ALL_ACCESS, NULL );
    le = GetLastError();
    ok( w2 != 0 || le == ERROR_ACCESS_DENIED, "create foobar station failed (%u)\n", le );

    if (w2 != 0)
    {
        w3 = OpenWindowStationA("foobar", TRUE, WINSTA_ALL_ACCESS );
        ok( w3 != 0, "open foobar station failed\n" );
        ok( w3 != w2, "open foobar station returned same handle\n" );
        ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );
        ok( CloseWindowStation( w3 ), "CloseWindowStation failed\n" );

        w3 = OpenWindowStationA("foobar", TRUE, WINSTA_ALL_ACCESS );
        ok( !w3, "open foobar station succeeded\n" );

        w2 = CreateWindowStationA("foobar1", 0, WINSTA_ALL_ACCESS, NULL );
        ok( w2 != 0, "create foobar station failed\n" );
        w3 = CreateWindowStationA("foobar2", 0, WINSTA_ALL_ACCESS, NULL );
        ok( w3 != 0, "create foobar station failed\n" );
        ok( GetHandleInformation( w2, &flags ), "GetHandleInformation failed\n" );
        ok( GetHandleInformation( w3, &flags ), "GetHandleInformation failed\n" );

        SetProcessWindowStation( w2 );
        atom = GlobalAddAtomA("foo");
        ok( GlobalGetAtomNameA( atom, buffer, sizeof(buffer) ) == 3, "GlobalGetAtomName failed\n" );
        ok( !lstrcmpiA( buffer, "foo" ), "bad atom value %s\n", buffer );

        ok( !CloseWindowStation( w2 ), "CloseWindowStation succeeded\n" );
        ok( GetHandleInformation( w2, &flags ), "GetHandleInformation failed\n" );

        SetProcessWindowStation( w3 );
        ok( GetHandleInformation( w2, &flags ), "GetHandleInformation failed\n" );
        ok( CloseWindowStation( w2 ), "CloseWindowStation failed\n" );
        ok( GlobalGetAtomNameA( atom, buffer, sizeof(buffer) ) == 3, "GlobalGetAtomName failed\n" );
        ok( !lstrcmpiA( buffer, "foo" ), "bad atom value %s\n", buffer );
    }
    else if (le == ERROR_ACCESS_DENIED)
        win_skip( "Not enough privileges for CreateWindowStation\n" );

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
    ok( GetLastError() == ERROR_BUSY || broken(GetLastError() == 0xdeadbeef), /* wow64 */
        "bad last error %d\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    if (CloseHandle( d1 ))  /* succeeds on nt4 */
    {
        win_skip( "NT4 desktop handle management is completely different\n" );
        return;
    }
    ok( GetLastError() == ERROR_INVALID_HANDLE, "bad last error %d\n", GetLastError() );

    ok( DuplicateHandle( GetCurrentProcess(), d1, GetCurrentProcess(), (PHANDLE)&d2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseDesktop(d2), "closing dup desktop failed\n" );

    ok( DuplicateHandle( GetCurrentProcess(), d1, GetCurrentProcess(), (PHANDLE)&d2, 0,
                         TRUE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    ok( CloseHandle(d2), "closing dup desktop handle failed\n" );

    d2 = OpenDesktopA( "dummy name", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( !d2, "open dummy desktop succeeded\n" );

    d2 = CreateDesktopA( "foobar", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( d2 != 0, "create foobar desktop failed\n" );
    SetLastError( 0xdeadbeef );
    ok( !CloseWindowStation( (HWINSTA)d2 ), "CloseWindowStation succeeded on desktop\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == 0xdeadbeef), /* wow64 */
        "bad last error %d\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    d3 = CreateDesktopA( "foobar", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( d3 != 0, "create foobar desktop again failed\n" );
    ok( GetLastError() == 0xdeadbeef, "bad last error %d\n", GetLastError() );
    ok( CloseDesktop( d3 ), "CloseDesktop failed\n" );

    d3 = OpenDesktopA( "foobar", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( d3 != 0, "open foobar desktop failed\n" );
    ok( d3 != d2, "open foobar desktop returned same handle\n" );
    ok( CloseDesktop( d2 ), "CloseDesktop failed\n" );
    ok( CloseDesktop( d3 ), "CloseDesktop failed\n" );

    d3 = OpenDesktopA( "foobar", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( !d3, "open foobar desktop succeeded\n" );

    ok( !CloseHandle(d1), "closing thread desktop handle succeeded\n" );
    d2 = GetThreadDesktop(GetCurrentThreadId());
    ok( d1 == d2, "got different handles after close\n" );

    register_class();
    trace( "thread 1 desktop: %p\n", d1 );
    print_object( d1 );
    hthread = CreateThread( NULL, 0, thread, (LPVOID)2, 0, &id );
    Sleep(1000);
    trace( "get other thread desktop: %p\n", GetThreadDesktop(id) );
    WaitForSingleObject( hthread, INFINITE );
    CloseHandle( hthread );

    /* clean side effect */
    SetProcessWindowStation( w1 );
}

/* Enumeration tests */

static BOOL CALLBACK window_station_callbackA(LPSTR winsta, LPARAM lp)
{
    trace("window_station_callbackA called with argument %s\n", winsta);
    return lp;
}

static BOOL CALLBACK open_window_station_callbackA(LPSTR winsta, LPARAM lp)
{
    HWINSTA hwinsta;

    trace("open_window_station_callbackA called with argument %s\n", winsta);
    hwinsta = OpenWindowStationA(winsta, FALSE, WINSTA_ENUMERATE);
    ok(hwinsta != NULL, "Could not open desktop %s!\n", winsta);
    if (hwinsta)
        CloseWindowStation(hwinsta);
    return lp;
}

static void test_enumstations(void)
{
    DWORD ret;
    HWINSTA hwinsta;

    if (0) /* Crashes instead */
    {
        SetLastError(0xbabefeed);
        ret = EnumWindowStationsA(NULL, 0);
        ok(!ret, "EnumWindowStationsA returned successfully!\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "LastError is set to %08x\n", GetLastError());
    }

    hwinsta = CreateWindowStationA("winsta_test", 0, WINSTA_ALL_ACCESS, NULL);
    ret = GetLastError();
    ok(hwinsta != NULL || ret == ERROR_ACCESS_DENIED, "CreateWindowStation failed (%u)\n", ret);
    if (!hwinsta)
    {
        win_skip("Not enough privileges for CreateWindowStation\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = EnumWindowStationsA(open_window_station_callbackA, 0x12345);
    ok(ret == 0x12345, "EnumWindowStationsA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumWindowStationsA(window_station_callbackA, 0);
    ok(!ret, "EnumWindowStationsA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());
}

static BOOL CALLBACK desktop_callbackA(LPSTR desktop, LPARAM lp)
{
    trace("desktop_callbackA called with argument %s\n", desktop);
    return lp;
}

static BOOL CALLBACK open_desktop_callbackA(LPSTR desktop, LPARAM lp)
{
    HDESK hdesk;
    static int once;

    trace("open_desktop_callbackA called with argument %s\n", desktop);
    /* Only try to open one desktop */
    if (once++)
        return lp;

    hdesk = OpenDesktopA(desktop, 0, FALSE, DESKTOP_ENUMERATE);
    ok(hdesk != NULL, "Could not open desktop %s!\n", desktop);
    if (hdesk)
        CloseDesktop(hdesk);
    return lp;
}

static void test_enumdesktops(void)
{
    BOOL ret;

    if (0)  /* Crashes instead */
    {
        SetLastError(0xbabefeed);
        ret = EnumDesktopsA(GetProcessWindowStation(), NULL, 0);
        ok(!ret, "EnumDesktopsA returned successfully!\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "LastError is set to %08x\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = EnumDesktopsA(NULL, desktop_callbackA, 0x12345);
    ok(ret == 0x12345, "EnumDesktopsA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumDesktopsA(GetProcessWindowStation(), open_desktop_callbackA, 0x12345);
    ok(ret == 0x12345, "EnumDesktopsA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumDesktopsA(INVALID_HANDLE_VALUE, desktop_callbackA, 0x12345);
    ok(!ret, "EnumDesktopsA returned %x\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "LastError is set to %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumDesktopsA(GetProcessWindowStation(), desktop_callbackA, 0);
    ok(!ret, "EnumDesktopsA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());
}

/* Miscellaneous tests */

static void test_getuserobjectinformation(void)
{
    HDESK desk;
    WCHAR bufferW[20];
    char buffer[20];
    WCHAR foobarTestW[] = {'f','o','o','b','a','r','T','e','s','t',0};
    WCHAR DesktopW[] = {'D','e','s','k','t','o','p',0};
    DWORD size;
    BOOL ret;

    desk = CreateDesktopA("foobarTest", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    ok(desk != 0, "open foobarTest desktop failed\n");

    strcpy(buffer, "blahblah");

    /** Tests for UOI_NAME **/

    /* Get size, test size and return value/error code */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationA(desk, UOI_NAME, NULL, 0, &size);

    ok(!ret, "GetUserObjectInformationA returned %x\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "LastError is set to %08x\n", GetLastError());
    ok(size == 22, "size is set to %d\n", size); /* Windows returns Unicode length (11*2) */

    /* Get string */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationA(desk, UOI_NAME, buffer, sizeof(buffer), &size);

    ok(ret, "GetUserObjectInformationA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    ok(strcmp(buffer, "foobarTest") == 0, "Buffer is set to '%s'\n", buffer);
    ok(size == 11, "size is set to %d\n", size); /* 11 bytes in 'foobarTest\0' */

    /* Get size, test size and return value/error code (Unicode) */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationW(desk, UOI_NAME, NULL, 0, &size);

    ok(!ret, "GetUserObjectInformationW returned %x\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "LastError is set to %08x\n", GetLastError());
    ok(size == 22, "size is set to %d\n", size);  /* 22 bytes in 'foobarTest\0' in Unicode */

    /* Get string (Unicode) */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationW(desk, UOI_NAME, bufferW, sizeof(bufferW), &size);

    ok(ret, "GetUserObjectInformationW returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    ok(lstrcmpW(bufferW, foobarTestW) == 0, "Buffer is not set to 'foobarTest'\n");
    ok(size == 22, "size is set to %d\n", size);  /* 22 bytes in 'foobarTest\0' in Unicode */

    /** Tests for UOI_TYPE **/

    /* Get size, test size and return value/error code */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationA(desk, UOI_TYPE, NULL, 0, &size);

    ok(!ret, "GetUserObjectInformationA returned %x\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "LastError is set to %08x\n", GetLastError());
    ok(size == 16, "size is set to %d\n", size); /* Windows returns Unicode length (8*2) */

    /* Get string */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationA(desk, UOI_TYPE, buffer, sizeof(buffer), &size);

    ok(ret, "GetUserObjectInformationA returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    ok(strcmp(buffer, "Desktop") == 0, "Buffer is set to '%s'\n", buffer);
    ok(size == 8, "size is set to %d\n", size); /* 8 bytes in 'Desktop\0' */

    /* Get size, test size and return value/error code (Unicode) */
    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetUserObjectInformationW(desk, UOI_TYPE, NULL, 0, &size);

    ok(!ret, "GetUserObjectInformationW returned %x\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "LastError is set to %08x\n", GetLastError());
    ok(size == 16, "size is set to %d\n", size);  /* 16 bytes in 'Desktop\0' in Unicode */

    /* Get string (Unicode) */
    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = GetUserObjectInformationW(desk, UOI_TYPE, bufferW, sizeof(bufferW), &size);

    ok(ret, "GetUserObjectInformationW returned %x\n", ret);
    ok(GetLastError() == 0xdeadbeef, "LastError is set to %08x\n", GetLastError());

    ok(lstrcmpW(bufferW, DesktopW) == 0, "Buffer is not set to 'Desktop'\n");
    ok(size == 16, "size is set to %d\n", size);  /* 16 bytes in 'Desktop\0' in Unicode */

    ok(CloseDesktop(desk), "CloseDesktop failed\n");
}

static void test_inputdesktop(void)
{
    HDESK input_desk, old_input_desk, thread_desk, old_thread_desk, new_desk;
    DWORD ret;
    CHAR name[1024];
    INPUT inputs[1];

    inputs[0].type = INPUT_KEYBOARD;
    U(inputs[0]).ki.wVk = 0;
    U(inputs[0]).ki.wScan = 0x3c0;
    U(inputs[0]).ki.dwFlags = KEYEVENTF_UNICODE;

    /* OpenInputDesktop creates new handles for each calls */
    old_input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(old_input_desk != NULL, "OpenInputDesktop failed!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(old_input_desk, UOI_NAME, name, 1024, NULL);
    ok(ret, "GetUserObjectInformation failed!\n");
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(input_desk != NULL, "OpenInputDesktop failed!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(input_desk, UOI_NAME, name, 1024, NULL);
    ok(ret, "GetUserObjectInformation failed!\n");
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    ok(old_input_desk != input_desk, "returned the same handle!\n");
    ret = CloseDesktop(input_desk);
    ok(ret, "CloseDesktop failed!\n");

    /* by default, GetThreadDesktop is the input desktop, SendInput should succeed. */
    old_thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(old_thread_desk != NULL, "GetThreadDesktop faile!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(old_thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    ok(GetLastError() == 0xdeadbeef, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1, "unexpected return count %d\n", ret);

    /* Set thread desktop to the new desktop, SendInput should fail. */
    new_desk = CreateDesktopA("new_desk", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    ok(new_desk != NULL, "CreateDesktop failed!\n");
    ret = SetThreadDesktop(new_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk == new_desk, "thread desktop doesn't match!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "new_desk"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    if(broken(GetLastError() == 0xdeadbeef))
    {
        SetThreadDesktop(old_thread_desk);
        CloseDesktop(old_input_desk);
        CloseDesktop(input_desk);
        CloseDesktop(new_desk);
        win_skip("Skip tests on NT4\n");
        return;
    }
todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1 || broken(ret == 0) /* Win64 */, "unexpected return count %d\n", ret);

    /* Set thread desktop back to the old thread desktop, SendInput should success. */
    ret = SetThreadDesktop(old_thread_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk == old_thread_desk, "thread desktop doesn't match!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    ok(GetLastError() == 0xdeadbeef, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1, "unexpected return count %d\n", ret);

    /* Set thread desktop to the input desktop, SendInput should success. */
    ret = SetThreadDesktop(old_input_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk == old_input_desk, "thread desktop doesn't match!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    ok(GetLastError() == 0xdeadbeef, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1, "unexpected return count %d\n", ret);

    /* Switch input desktop to the new desktop, SendInput should fail. */
    ret = SwitchDesktop(new_desk);
    ok(ret, "SwitchDesktop failed!\n");
    input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(input_desk != NULL, "OpenInputDesktop failed!\n");
    ok(input_desk != new_desk, "returned the same handle!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(input_desk, UOI_NAME, name, 1024, NULL);
    ok(ret, "GetUserObjectInformation failed!\n");
todo_wine
    ok(!strcmp(name, "new_desk"), "unexpected desktop %s\n", name);
    ret = CloseDesktop(input_desk);
    ok(ret, "CloseDesktop failed!\n");

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1 || broken(ret == 0) /* Win64 */, "unexpected return count %d\n", ret);

    /* Set thread desktop to the new desktop, SendInput should success. */
    ret = SetThreadDesktop(new_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk == new_desk, "thread desktop doesn't match!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "new_desk"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    ok(GetLastError() == 0xdeadbeef, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1, "unexpected return count %d\n", ret);

    /* Switch input desktop to the old input desktop, set thread desktop to the old
     * thread desktop, clean side effects. SendInput should success. */
    ret = SwitchDesktop(old_input_desk);
    input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(input_desk != NULL, "OpenInputDesktop failed!\n");
    ok(input_desk != old_input_desk, "returned the same handle!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(input_desk, UOI_NAME, name, 1024, NULL);
    ok(ret, "GetUserObjectInformation failed!\n");
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    ret = SetThreadDesktop(old_thread_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk == old_thread_desk, "thread desktop doesn't match!\n");
    memset(name, 0, sizeof(name));
    ret = GetUserObjectInformationA(thread_desk, UOI_NAME, name, 1024, NULL);
    ok(!strcmp(name, "Default"), "unexpected desktop %s\n", name);

    SetLastError(0xdeadbeef);
    ret = SendInput(1, inputs, sizeof(INPUT));
    ok(GetLastError() == 0xdeadbeef, "unexpected last error %08x\n", GetLastError());
    ok(ret == 1, "unexpected return count %d\n", ret);

    /* free resources */
    ret = CloseDesktop(input_desk);
    ok(ret, "CloseDesktop failed!\n");
    ret = CloseDesktop(old_input_desk);
    ok(ret, "CloseDesktop failed!\n");
    ret = CloseDesktop(new_desk);
    ok(ret, "CloseDesktop failed!\n");
}

static void test_inputdesktop2(void)
{
    HWINSTA w1, w2;
    HDESK thread_desk, new_desk, input_desk, hdesk;
    DWORD ret;

    thread_desk = GetThreadDesktop(GetCurrentThreadId());
    ok(thread_desk != NULL, "GetThreadDesktop failed!\n");
    w1 = GetProcessWindowStation();
    ok(w1 != NULL, "GetProcessWindowStation failed!\n");
    SetLastError(0xdeadbeef);
    w2 = CreateWindowStationA("winsta_test", 0, WINSTA_ALL_ACCESS, NULL);
    ret = GetLastError();
    ok(w2 != NULL || ret == ERROR_ACCESS_DENIED, "CreateWindowStation failed (%u)\n", ret);
    if (!w2)
    {
        win_skip("Not enough privileges for CreateWindowStation\n");
        return;
    }

    ret = EnumDesktopsA(GetProcessWindowStation(), desktop_callbackA, 0);
    ok(!ret, "EnumDesktopsA failed!\n");
    input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(input_desk != NULL, "OpenInputDesktop failed!\n");
    ret = CloseDesktop(input_desk);
    ok(ret, "CloseDesktop failed!\n");

    ret = SetProcessWindowStation(w2);
    ok(ret, "SetProcessWindowStation failed!\n");
    hdesk = GetThreadDesktop(GetCurrentThreadId());
    ok(hdesk != NULL, "GetThreadDesktop failed!\n");
    ok(hdesk == thread_desk, "thread desktop should not change after winstation changed!\n");
    ret = EnumDesktopsA(GetProcessWindowStation(), desktop_callbackA, 0);

    new_desk = CreateDesktopA("desk_test", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    ok(new_desk != NULL, "CreateDesktop failed!\n");
    ret = EnumDesktopsA(GetProcessWindowStation(), desktop_callbackA, 0);
    ok(!ret, "EnumDesktopsA failed!\n");
    SetLastError(0xdeadbeef);
    input_desk = OpenInputDesktop(0, FALSE, DESKTOP_ALL_ACCESS);
    ok(input_desk == NULL, "OpenInputDesktop should fail on non default winstation!\n");
    ok(GetLastError() == ERROR_INVALID_FUNCTION || broken(GetLastError() == 0xdeadbeef), "last error %08x\n", GetLastError());

    hdesk = OpenDesktopA("desk_test", 0, TRUE, DESKTOP_ALL_ACCESS);
    ok(hdesk != NULL, "OpenDesktop failed!\n");
    SetLastError(0xdeadbeef);
    ret = SwitchDesktop(hdesk);
todo_wine
    ok(!ret, "Switch to desktop belong to non default winstation should fail!\n");
todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED || broken(GetLastError() == 0xdeadbeef), "last error %08x\n", GetLastError());
    ret = SetThreadDesktop(hdesk);
    ok(ret, "SetThreadDesktop failed!\n");

    /* clean side effect */
    ret = SetThreadDesktop(thread_desk);
todo_wine
    ok(ret, "SetThreadDesktop should success even desktop is not belong to process winstation!\n");
    ret = SetProcessWindowStation(w1);
    ok(ret, "SetProcessWindowStation failed!\n");
    ret = SetThreadDesktop(thread_desk);
    ok(ret, "SetThreadDesktop failed!\n");
    ret = CloseWindowStation(w2);
    ok(ret, "CloseWindowStation failed!\n");
    ret = CloseDesktop(new_desk);
    ok(ret, "CloseDesktop failed!\n");
    ret = CloseDesktop(hdesk);
    ok(ret, "CloseDesktop failed!\n");
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        trace("destroying hwnd %p\n", hWnd);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA( hWnd, msg, wParam, lParam );
}

typedef struct tag_wnd_param
{
    const char *wnd_name;
    HWND hwnd;
    HDESK hdesk;
    HANDLE hevent;
} wnd_param;

static DWORD WINAPI create_window(LPVOID param)
{
    wnd_param *param1 = param;
    DWORD ret;
    MSG msg;

    ret = SetThreadDesktop(param1->hdesk);
    ok(ret, "SetThreadDesktop failed!\n");
    param1->hwnd = CreateWindowA("test_class", param1->wnd_name, WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(param1->hwnd != 0, "CreateWindowA failed!\n");
    ret = SetEvent(param1->hevent);
    ok(ret, "SetEvent failed!\n");

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

static DWORD set_foreground(HWND hwnd)
{
    HWND hwnd_fore;
    DWORD set_id, fore_id, ret;
    char win_text[1024];

    hwnd_fore = GetForegroundWindow();
    GetWindowTextA(hwnd_fore, win_text, 1024);
    set_id = GetWindowThreadProcessId(hwnd, NULL);
    fore_id = GetWindowThreadProcessId(hwnd_fore, NULL);
    trace("\"%s\" %p %08x hwnd %p %08x\n", win_text, hwnd_fore, fore_id, hwnd, set_id);
    ret = AttachThreadInput(set_id, fore_id, TRUE);
    trace("AttachThreadInput returned %08x\n", ret);
    ret = ShowWindow(hwnd, SW_SHOWNORMAL);
    trace("ShowWindow returned %08x\n", ret);
    ret = SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
    trace("set topmost returned %08x\n", ret);
    ret = SetWindowPos(hwnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
    trace("set notopmost returned %08x\n", ret);
    ret = SetForegroundWindow(hwnd);
    trace("SetForegroundWindow returned %08x\n", ret);
    Sleep(250);
    AttachThreadInput(set_id, fore_id, FALSE);
    return ret;
}

static void test_foregroundwindow(void)
{
    HWND hwnd, hwnd_test, partners[2], hwnds[2];
    HDESK hdesks[2];
    int thread_desk_id, input_desk_id, hwnd_id;
    WNDCLASSA wclass;
    wnd_param param;
    DWORD ret, timeout, timeout_old;
    char win_text[1024];

#define DESKTOPS 2

    memset( &wclass, 0, sizeof(wclass) );
    wclass.lpszClassName = "test_class";
    wclass.lpfnWndProc   = WndProc;
    RegisterClassA(&wclass);
    param.wnd_name = "win_name";

    hdesks[0] = GetThreadDesktop(GetCurrentThreadId());
    ok(hdesks[0] != NULL, "OpenDesktop failed!\n");
    SetLastError(0xdeadbeef);
    hdesks[1] = CreateDesktopA("desk2", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    ret = GetLastError();
    ok(hdesks[1] != NULL || ret == ERROR_ACCESS_DENIED, "CreateDesktop failed (%u)\n", ret);
    if(!hdesks[1])
    {
        win_skip("Not enough privileges for CreateDesktop\n");
        return;
    }

    ret = SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &timeout_old, 0);
    if(!ret)
    {
        win_skip("Skip tests on NT4\n");
        CloseDesktop(hdesks[1]);
        return;
    }
    trace("old timeout %d\n", timeout_old);
    timeout = 0;
    ret = SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    ok(ret, "set foreground lock timeout failed!\n");
    ret = SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &timeout, 0);
    ok(ret, "get foreground lock timeout failed!\n");
    ok(timeout == 0, "unexpected timeout %d\n", timeout);

    for (thread_desk_id = 0; thread_desk_id < DESKTOPS; thread_desk_id++)
    {
        param.hdesk = hdesks[thread_desk_id];
        param.hevent = CreateEventA(NULL, TRUE, FALSE, NULL);
        CreateThread(NULL, 0, create_window, &param, 0, NULL);
        ret = WaitForSingleObject(param.hevent, INFINITE);
        ok(ret == WAIT_OBJECT_0, "wait failed!\n");
        hwnds[thread_desk_id] = param.hwnd;
    }

    for (thread_desk_id = 0; thread_desk_id < DESKTOPS; thread_desk_id++)
    {
        param.hdesk = hdesks[thread_desk_id];
        param.hevent = CreateEventA(NULL, TRUE, FALSE, NULL);
        CreateThread(NULL, 0, create_window, &param, 0, NULL);
        ret = WaitForSingleObject(param.hevent, INFINITE);
        ok(ret == WAIT_OBJECT_0, "wait failed!\n");
        partners[thread_desk_id] = param.hwnd;
    }

    trace("hwnd0 %p hwnd1 %p partner0 %p partner1 %p\n", hwnds[0], hwnds[1], partners[0], partners[1]);

    for (hwnd_id = 0; hwnd_id < DESKTOPS; hwnd_id++)
        for (thread_desk_id = 0; thread_desk_id < DESKTOPS; thread_desk_id++)
            for (input_desk_id = 0; input_desk_id < DESKTOPS; input_desk_id++)
            {
                trace("testing thread_desk %d input_desk %d hwnd %d\n",
                        thread_desk_id, input_desk_id, hwnd_id);
                hwnd_test = hwnds[hwnd_id];
                ret = SetThreadDesktop(hdesks[thread_desk_id]);
                ok(ret, "set thread desktop failed!\n");
                ret = SwitchDesktop(hdesks[input_desk_id]);
                ok(ret, "switch desktop failed!\n");
                set_foreground(partners[0]);
                set_foreground(partners[1]);
                hwnd = GetForegroundWindow();
                ok(hwnd != hwnd_test, "unexpected foreground window %p\n", hwnd);
                ret = set_foreground(hwnd_test);
                hwnd = GetForegroundWindow();
                GetWindowTextA(hwnd, win_text, 1024);
                trace("hwnd %p name %s\n", hwnd, win_text);
                if (input_desk_id == hwnd_id)
                {
                    if (input_desk_id == thread_desk_id)
                    {
                        ok(ret, "SetForegroundWindow failed!\n");
                        if (hwnd)
                            ok(hwnd == hwnd_test , "unexpected foreground window %p\n", hwnd);
                        else
                            todo_wine ok(hwnd == hwnd_test , "unexpected foreground window %p\n", hwnd);
                    }
                    else
                    {
                        todo_wine ok(ret, "SetForegroundWindow failed!\n");
                        todo_wine ok(hwnd == 0, "unexpected foreground window %p\n", hwnd);
                    }
                }
                else
                {
                    if (input_desk_id == thread_desk_id)
                    {
                        ok(!ret, "SetForegroundWindow should fail!\n");
                        if (hwnd)
                            ok(hwnd == partners[input_desk_id] , "unexpected foreground window %p\n", hwnd);
                        else
                            todo_wine ok(hwnd == partners[input_desk_id] , "unexpected foreground window %p\n", hwnd);
                    }
                    else
                    {
                        todo_wine ok(!ret, "SetForegroundWindow should fail!\n");
                        if (!hwnd)
                            ok(hwnd == 0, "unexpected foreground window %p\n", hwnd);
                        else
                            todo_wine ok(hwnd == 0, "unexpected foreground window %p\n", hwnd);
                    }
                }
            }

    /* Clean up */

    for (thread_desk_id = DESKTOPS - 1; thread_desk_id >= 0; thread_desk_id--)
    {
        ret = SetThreadDesktop(hdesks[thread_desk_id]);
        ok(ret, "set thread desktop failed!\n");
        SendMessageA(hwnds[thread_desk_id], WM_DESTROY, 0, 0);
        SendMessageA(partners[thread_desk_id], WM_DESTROY, 0, 0);
    }

    ret = SwitchDesktop(hdesks[0]);
    ok(ret, "switch desktop failed!\n");
    CloseDesktop(hdesks[1]);

    ret = SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, UlongToPtr(timeout_old), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    ok(ret, "set foreground lock timeout failed!\n");
    ret = SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &timeout, 0);
    ok(ret, "get foreground lock timeout failed!\n");
    ok(timeout == timeout_old, "unexpected timeout %d\n", timeout);
}

START_TEST(winstation)
{
    /* Check whether this platform supports WindowStation calls */

    SetLastError( 0xdeadbeef );
    GetProcessWindowStation();
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("WindowStation calls not supported on this platform\n");
        return;
    }

    test_inputdesktop();
    test_inputdesktop2();
    test_enumstations();
    test_enumdesktops();
    test_handles();
    test_getuserobjectinformation();
    test_foregroundwindow();
}
