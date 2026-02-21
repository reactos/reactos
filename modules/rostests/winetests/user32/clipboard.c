/*
 * Unit test suite for clipboard functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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

#include <stdio.h>
#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

static BOOL (WINAPI *pAddClipboardFormatListener)(HWND hwnd);
static BOOL (WINAPI *pRemoveClipboardFormatListener)(HWND hwnd);
static BOOL (WINAPI *pGetUpdatedClipboardFormats)( UINT *formats, UINT count, UINT *out_count );
static HGLOBAL (WINAPI *pGlobalFree)(HGLOBAL);

static int thread_from_line;
static char *argv0;

#define open_clipboard(hwnd) open_clipboard_(__LINE__, hwnd)
static BOOL open_clipboard_(int line, HWND hwnd)
{
    DWORD start = GetTickCount();
    while (1)
    {
        BOOL ret = OpenClipboard(hwnd);
        if (ret || GetLastError() != ERROR_ACCESS_DENIED)
            return ret;
        if (GetTickCount() - start > 100)
        {
            char classname[256];
            DWORD le = GetLastError();
            HWND clipwnd = GetOpenClipboardWindow();
            /* Provide a hint as to the source of interference:
             * - The class name would typically be CLIPBRDWNDCLASS if the
             *   clipboard was opened by a Windows application using the
             *   ole32 API.
             * - And it would be __wine_clipboard_manager if it was opened in
             *   response to a native application.
             */
            GetClassNameA(clipwnd, classname, ARRAY_SIZE(classname));
            trace_(__FILE__, line)("%p (%s) opened the clipboard\n", clipwnd, classname);
            SetLastError(le);
            return ret;
        }
        Sleep(15);
    }
}

#define has_no_open_wnd() has_no_open_wnd_(__LINE__)
static BOOL has_no_open_wnd_(int line)
{
    DWORD start = GetTickCount();
    DWORD le = GetLastError();
    while (1)
    {
        HWND clipwnd = GetOpenClipboardWindow();
        if (!clipwnd) return TRUE;
        if (GetTickCount() - start > 100)
        {
            char classname[256];
            le = GetLastError();
            /* See open_clipboard() */
            GetClassNameA(clipwnd, classname, ARRAY_SIZE(classname));
            trace_(__FILE__, line)("%p (%s) opened the clipboard\n", clipwnd, classname);
            SetLastError(le);
            return FALSE;
        }
        Sleep(15);
        SetLastError(le);
    }
}

static DWORD WINAPI open_clipboard_thread(LPVOID arg)
{
    HWND hWnd = arg;
    ok(open_clipboard(hWnd), "%u: OpenClipboard failed\n", thread_from_line);
    return 0;
}

static DWORD WINAPI empty_clipboard_thread(LPVOID arg)
{
    SetLastError( 0xdeadbeef );
    ok(!EmptyClipboard(), "%u: EmptyClipboard succeeded\n", thread_from_line );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %lu\n",
        thread_from_line, GetLastError());
    return 0;
}

static DWORD WINAPI open_and_empty_clipboard_thread(LPVOID arg)
{
    HWND hWnd = arg;
    ok(open_clipboard(hWnd), "%u: OpenClipboard failed\n", thread_from_line);
    ok(EmptyClipboard(), "%u: EmptyClipboard failed\n", thread_from_line );
    return 0;
}

static DWORD WINAPI open_and_empty_clipboard_win_thread(LPVOID arg)
{
    HWND hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );
    ok(open_clipboard(hwnd), "%u: OpenClipboard failed\n", thread_from_line);
    ok(EmptyClipboard(), "%u: EmptyClipboard failed\n", thread_from_line );
    return 0;
}

static DWORD WINAPI set_clipboard_data_thread(LPVOID arg)
{
    HWND hwnd = arg;
    HANDLE ret;

    SetLastError( 0xdeadbeef );
    if (GetClipboardOwner() == hwnd)
    {
        SetClipboardData( CF_WAVE, 0 );
        ok( IsClipboardFormatAvailable( CF_WAVE ), "%u: SetClipboardData failed\n", thread_from_line );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "%u: SetClipboardData failed err %lu\n", thread_from_line, GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = GetClipboardData( CF_WAVE );
        ok( !ret, "%u: GetClipboardData succeeded\n", thread_from_line );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %lu\n",
            thread_from_line, GetLastError());
    }
    else
    {
        SetClipboardData( CF_WAVE, 0 );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %lu\n",
            thread_from_line, GetLastError());
        ok( !IsClipboardFormatAvailable( CF_WAVE ), "%u: SetClipboardData succeeded\n", thread_from_line );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( !ret, "%u: SetClipboardData succeeded\n", thread_from_line );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %lu\n",
            thread_from_line, GetLastError());
    }
    return 0;
}

static void set_clipboard_data_process( int arg )
{
    HANDLE ret;

    winetest_push_context("process %u", arg);
    SetLastError( 0xdeadbeef );
    if (arg)
    {
        ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "SetClipboardData failed err %lu\n", GetLastError() );
    }
    else
    {
        SetClipboardData( CF_WAVE, 0 );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %lu\n",
            GetLastError());
        ok( !IsClipboardFormatAvailable( CF_WAVE ), "SetClipboardData succeeded\n" );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( !ret, "SetClipboardData succeeded\n" );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %lu\n",
            GetLastError());
    }
    winetest_pop_context();
}

static void grab_clipboard_process( int arg )
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = open_clipboard( 0 );
    ok( ret, "OpenClipboard failed\n" );
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard failed\n" );
    if (arg)
    {
        HANDLE ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "process %u: SetClipboardData failed err %lu\n", arg, GetLastError() );
    }
}

static void run_thread( LPTHREAD_START_ROUTINE func, void *arg, int line )
{
    DWORD ret;
    HANDLE thread;

    thread_from_line = line;
    thread = CreateThread(NULL, 0, func, arg, 0, NULL);
    ok(thread != NULL, "%u: CreateThread failed with error %ld\n", line, GetLastError());
    for (;;)
    {
        ret = MsgWaitForMultipleObjectsEx( 1, &thread, 1000, QS_ALLINPUT, 0 );
        if (ret == WAIT_OBJECT_0 + 1)
        {
            MSG msg;
            while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        }
        else break;
    }
    ok(ret == WAIT_OBJECT_0, "%u: expected WAIT_OBJECT_0, got %lu\n", line, ret);
    CloseHandle(thread);
}

static void run_process( const char *args )
{
    char cmd[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;

    sprintf( cmd, "%s clipboard %s", argv0, args );
    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    ok( CreateProcessA( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info ),
        "CreateProcess %s failed\n", cmd );

    wait_child_process( info.hProcess );
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
}

static WNDPROC old_proc;
static LRESULT CALLBACK winproc_wrapper( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    static int wm_renderallformats;
    static int wm_drawclipboard;
    static int seqno;
    DWORD msg_flags = InSendMessageEx( NULL );

    if (!seqno) seqno = GetClipboardSequenceNumber();

    trace( "%p msg %04x\n", hwnd, msg );
    if (!wm_renderallformats)
    {
        ok( GetClipboardOwner() == hwnd, "%04x: wrong owner %p/%p\n", msg, GetClipboardOwner(), hwnd );
        ok( seqno == GetClipboardSequenceNumber(), "%04x: seqno changed\n", msg );
    }
    else
    {
        ok( !GetClipboardOwner(), "%04x: wrong owner %p\n", msg, GetClipboardOwner() );
        ok( seqno + 1 == GetClipboardSequenceNumber(), "%04x: seqno unchanged\n", msg );
    }
    ok( GetClipboardViewer() == hwnd, "%04x: wrong viewer %p/%p\n", msg, GetClipboardViewer(), hwnd );
    ok( GetOpenClipboardWindow() == hwnd, "%04x: wrong open win %p/%p\n",
        msg, GetOpenClipboardWindow(), hwnd );

    switch (msg)
    {
    case WM_DESTROY:
        ok( wm_renderallformats, "didn't receive WM_RENDERALLFORMATS before WM_DESTROY\n" );
        ok( wm_drawclipboard, "didn't receive WM_DRAWCLIPBOARD before WM_DESTROY\n" );
        break;
    case WM_DRAWCLIPBOARD:
        ok( msg_flags == ISMEX_NOSEND, "WM_DRAWCLIPBOARD wrong flags %lx\n", msg_flags );
        wm_drawclipboard++;
        break;
    case WM_RENDERALLFORMATS:
        ok( msg_flags == ISMEX_NOSEND, "WM_RENDERALLFORMATS wrong flags %lx\n", msg_flags );
        wm_renderallformats++;
        break;
    }
    return old_proc( hwnd, msg, wp, lp );
}

static void test_ClipboardOwner(void)
{
    HWND hWnd1, hWnd2;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef,
       "could not perform clipboard test: clipboard already owned\n");

    hWnd1 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd1 != 0, "CreateWindowExA error %ld\n", GetLastError());
    trace("hWnd1 = %p\n", hWnd1);

    hWnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd2 != 0, "CreateWindowExA error %ld\n", GetLastError());
    trace("hWnd2 = %p\n", hWnd2);

    SetLastError(0xdeadbeef);
    ok(!CloseClipboard(), "CloseClipboard should fail if clipboard wasn't open\n");
    ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN || broken(GetLastError() == 0xdeadbeef), /* wow64 */
       "wrong error %lu\n", GetLastError());

    ok(open_clipboard(0), "OpenClipboard failed\n");
    ok(!GetClipboardOwner(), "clipboard should still be not owned\n");
    ok(!OpenClipboard(hWnd1), "OpenClipboard should fail since clipboard already opened\n");
    ok(open_clipboard(0), "OpenClipboard again failed\n");
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    ok(open_clipboard(hWnd1), "OpenClipboard failed\n");
    run_thread( open_clipboard_thread, hWnd1, __LINE__ );
    run_thread( empty_clipboard_thread, 0, __LINE__ );
    run_thread( set_clipboard_data_thread, hWnd1, __LINE__ );
    ok( !IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE available\n" );
    ok( !GetClipboardData( CF_WAVE ), "CF_WAVE data available\n" );
    run_process( "set_clipboard_data 0" );
    ok(!CloseClipboard(), "CloseClipboard should fail if clipboard wasn't open\n");
    ok(open_clipboard(hWnd1), "OpenClipboard failed\n");

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should still be not owned\n");
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should be owned by %p, not by %p\n", hWnd1, GetClipboardOwner());
    run_thread( empty_clipboard_thread, 0, __LINE__ );
    run_thread( set_clipboard_data_thread, hWnd1, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 1" );

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %ld\n", GetLastError());

    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should still be owned\n");

    /* any window will do, even from a different process */
    ret = open_clipboard( GetDesktopWindow() );
    ok( ret, "OpenClipboard error %ld\n", GetLastError());
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ok( GetClipboardOwner() == GetDesktopWindow(), "wrong owner %p/%p\n",
        GetClipboardOwner(), GetDesktopWindow() );
    run_thread( set_clipboard_data_thread, GetDesktopWindow(), __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 2" );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    ret = open_clipboard( hWnd1 );
    ok( ret, "OpenClipboard error %ld\n", GetLastError());
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    SetClipboardData( CF_WAVE, 0 );
    SetClipboardViewer( hWnd1 );
    ok( GetClipboardOwner() == hWnd1, "wrong owner %p/%p\n", GetClipboardOwner(), hWnd1 );
    ok( GetClipboardViewer() == hWnd1, "wrong viewer %p/%p\n", GetClipboardViewer(), hWnd1 );
    ok( GetOpenClipboardWindow() == hWnd1, "wrong open win %p/%p\n", GetOpenClipboardWindow(), hWnd1 );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );

    old_proc = (WNDPROC)SetWindowLongPtrA( hWnd1, GWLP_WNDPROC, (LONG_PTR)winproc_wrapper );
    ret = DestroyWindow(hWnd1);
    ok( ret, "DestroyWindow error %ld\n", GetLastError());
    ret = DestroyWindow(hWnd2);
    ok( ret, "DestroyWindow error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should not be owned\n");
    ok(!GetClipboardViewer() && GetLastError() == 0xdeadbeef, "viewer still exists\n");
    ok( has_no_open_wnd() && GetLastError() == 0xdeadbeef, "clipboard should not be open\n");
    ok( !IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE available\n" );

    SetLastError( 0xdeadbeef );
    ret = CloseClipboard();
    ok( !ret, "CloseClipboard succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %lu\n", GetLastError() );

    ret = open_clipboard( 0 );
    ok( ret, "OpenClipboard error %ld\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 3" );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    run_thread( open_and_empty_clipboard_thread, 0, __LINE__ );
    ok( has_no_open_wnd(), "wrong open window\n" );
    ok( !GetClipboardOwner(), "wrong owner window %p\n", GetClipboardOwner() );

    ret = open_clipboard( 0 );
    ok( ret, "OpenClipboard error %ld\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 4" );
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ok( !SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 )),
        "SetClipboardData succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %lu\n", GetLastError() );
    ok( !IsClipboardFormatAvailable( CF_WAVE ), "SetClipboardData succeeded\n" );

    run_thread( open_and_empty_clipboard_thread, GetDesktopWindow(), __LINE__ );
    ok( has_no_open_wnd(), "wrong open window\n" );
    ok( GetClipboardOwner() == GetDesktopWindow(), "wrong owner window %p / %p\n",
        GetClipboardOwner(), GetDesktopWindow() );

    run_thread( open_and_empty_clipboard_win_thread, 0, __LINE__ );
    ok( has_no_open_wnd(), "wrong open window\n" );
    ok( !GetClipboardOwner(), "wrong owner window %p\n", GetClipboardOwner() );
}

static void test_RegisterClipboardFormatA(void)
{
    ATOM atom_id;
    UINT format_id, format_id2;
    char buf[256];
    int len;
    BOOL ret;
    HANDLE handle;

    format_id = RegisterClipboardFormatA("my_cool_clipboard_format");
    ok(format_id > 0xc000 && format_id < 0xffff, "invalid clipboard format id %04x\n", format_id);

    format_id2 = RegisterClipboardFormatA("MY_COOL_CLIPBOARD_FORMAT");
    ok(format_id2 == format_id, "invalid clipboard format id %04x\n", format_id2);

    len = GetClipboardFormatNameA(format_id, buf, ARRAY_SIZE(buf));
    ok(len == lstrlenA("my_cool_clipboard_format"), "wrong format name length %d\n", len);
    ok(!lstrcmpA(buf, "my_cool_clipboard_format"), "wrong format name \"%s\"\n", buf);

    len = GetClipboardFormatNameA(format_id, NULL, 0);
    ok(len == 0, "wrong format name length %d\n", len);

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GetAtomNameA((ATOM)format_id, buf, ARRAY_SIZE(buf));
    ok(len == 0 || lstrcmpA(buf, "my_cool_clipboard_format") != 0,
       "format_id should not be a valid local atom\n");
    ok(len != 0 || GetLastError() == ERROR_INVALID_HANDLE,
       "err %ld\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameA((ATOM)format_id, buf, ARRAY_SIZE(buf));
    todo_wine ok(len == 0 || lstrcmpA(buf, "my_cool_clipboard_format") != 0,
       "format_id should not be a valid global atom\n");
    ok(len != 0 || GetLastError() == ERROR_INVALID_HANDLE,
       "err %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    atom_id = FindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "FindAtomA should fail, but it returned %x (format_id=%x)\n", atom_id, format_id);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "err %ld\n", GetLastError());

    todo_wine
    {
    /* this relies on the clipboard and global atom table being different */
    SetLastError(0xdeadbeef);
    atom_id = GlobalFindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "GlobalFindAtomA should fail, but it returned %x (format_id=%x)\n", atom_id, format_id);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "err %ld\n", GetLastError());
    }

    for (format_id = 0; format_id < 0x10fff; format_id++)
    {
        SetLastError(0xdeadbeef);
        len = GetClipboardFormatNameA(format_id, buf, ARRAY_SIZE(buf));

        if (format_id < 0xc000 || format_id > 0xffff)
            ok(!len, "GetClipboardFormatNameA should fail, but it returned %d (%s)\n", len, buf);
        else if (len && winetest_debug > 1)
            trace("%04x: %s\n", format_id, len ? buf : "");
    }

    ret = open_clipboard(0);
    ok( ret, "OpenClipboard error %ld\n", GetLastError());

    /* try some invalid/unregistered formats */
    SetLastError( 0xdeadbeef );
    handle = SetClipboardData( 0, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( !handle, "SetClipboardData succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %lu\n", GetLastError());
    handle = SetClipboardData( 0x1234, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %ld\n", GetLastError());
    handle = SetClipboardData( 0x123456, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %ld\n", GetLastError());
    handle = SetClipboardData( 0xffff8765, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %ld\n", GetLastError());

    ok( IsClipboardFormatAvailable( 0x1234 ), "format missing\n" );
    ok( IsClipboardFormatAvailable( 0x123456 ), "format missing\n" );
    ok( IsClipboardFormatAvailable( 0xffff8765 ), "format missing\n" );
    ok( !IsClipboardFormatAvailable( 0 ), "format available\n" );
    ok( !IsClipboardFormatAvailable( 0x3456 ), "format available\n" );
    ok( !IsClipboardFormatAvailable( 0x8765 ), "format available\n" );

    trace("# of formats available: %d\n", CountClipboardFormats());

    format_id = 0;
    while ((format_id = EnumClipboardFormats(format_id)))
    {
        ok(IsClipboardFormatAvailable(format_id), "format %04x was listed as available\n", format_id);
        len = GetClipboardFormatNameA(format_id, buf, ARRAY_SIZE(buf));
        trace("%04x: %s\n", format_id, len ? buf : "");
    }

    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ret =CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    if (CountClipboardFormats())
    {
        SetLastError(0xdeadbeef);
        ok(!EnumClipboardFormats(0), "EnumClipboardFormats should fail if clipboard wasn't open\n");
        ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN,
           "Last error should be set to ERROR_CLIPBOARD_NOT_OPEN, not %ld\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ok(!EmptyClipboard(), "EmptyClipboard should fail if clipboard wasn't open\n");
    ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN || broken(GetLastError() == 0xdeadbeef), /* wow64 */
       "Wrong error %lu\n", GetLastError());

    format_id = RegisterClipboardFormatA("#1234");
    ok(format_id == 1234, "invalid clipboard format id %04x\n", format_id);
}

static HGLOBAL create_textA(void)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 10);
    char *p = GlobalLock(h);
    memcpy(p, "test\0\0\0\0\0", 10);
    GlobalUnlock(h);
    return h;
}

static HGLOBAL create_textW(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0,0,0,0,0,0};
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(testW));
    WCHAR *p = GlobalLock(h);
    memcpy(p, testW, sizeof(testW));
    GlobalUnlock(h);
    return h;
}

static HANDLE create_metafile(void)
{
    const RECT rect = {0, 0, 100, 100};
    METAFILEPICT *pict;
    HANDLE ret;
    HMETAFILE mf;
    HDC hdc = CreateMetaFileA( NULL );
    ExtTextOutA( hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL );
    mf = CloseMetaFile( hdc );
    ret = GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(*pict) );
    pict = GlobalLock( ret );
    pict->mm = MM_TEXT;
    pict->xExt = pict->yExt = 100;
    pict->hMF = mf;
    GlobalUnlock( ret );
    return ret;
}

static HENHMETAFILE create_emf(void)
{
    const RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFileA(NULL, NULL, &rect, "HENHMETAFILE Ole Clipboard Test\0Test\0\0");
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static HBITMAP create_bitmap(void)
{
    HDC hdc = GetDC( 0 );
    UINT bpp = GetDeviceCaps( hdc, BITSPIXEL );
    ReleaseDC( 0, hdc );
    return CreateBitmap( 10, 10, 1, bpp, NULL );
}

static HBITMAP create_dib( BOOL v5 )
{
    HANDLE ret;
    BITMAPINFOHEADER *hdr;

    ret = GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT,
                       sizeof(BITMAPV5HEADER) + 256 * sizeof(RGBQUAD) + 16 * 16 * 4 );
    hdr = GlobalLock( ret );
    hdr->biSize = v5 ? sizeof(BITMAPV5HEADER) : sizeof(*hdr);
    hdr->biWidth = 16;
    hdr->biHeight = 16;
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
    if (v5)
    {
        BITMAPV5HEADER *hdr5 = (BITMAPV5HEADER *)hdr;
        hdr5->bV5RedMask = 0x0000ff;
        hdr5->bV5GreenMask = 0x00ff00;
        hdr5->bV5BlueMask = 0xff0000;
        hdr5->bV5AlphaMask = 0xff000000;
    }
    GlobalUnlock( ret );
    return ret;
}

static LRESULT CALLBACK renderer_winproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    static UINT rendered;
    UINT ret;

    switch (msg)
    {
    case WM_RENDERFORMAT:
        if (wp < 32) rendered |= (1 << wp);
        break;
    case WM_USER:
        ret = rendered;
        rendered = 0;
        return ret;
    }
    return DefWindowProcA( hwnd, msg, wp, lp );
}

static void test_synthesized(void)
{
    static const struct test
    {
        UINT format;
        UINT expected[8];
    } tests[] =
    {
/* 0 */ { CF_TEXT, { CF_TEXT, CF_LOCALE, CF_OEMTEXT, CF_UNICODETEXT }},
        { CF_OEMTEXT, { CF_OEMTEXT, CF_LOCALE, CF_TEXT, CF_UNICODETEXT }},
        { CF_UNICODETEXT, { CF_UNICODETEXT, CF_LOCALE, CF_TEXT, CF_OEMTEXT }},
        { CF_ENHMETAFILE, { CF_ENHMETAFILE, CF_METAFILEPICT }},
        { CF_METAFILEPICT, { CF_METAFILEPICT, CF_ENHMETAFILE }},
/* 5 */ { CF_BITMAP, { CF_BITMAP, CF_DIB, CF_DIBV5 }},
        { CF_DIB, { CF_DIB, CF_BITMAP, CF_DIBV5 }},
        { CF_DIBV5, { CF_DIBV5, CF_BITMAP, CF_DIB }},
    };

    HGLOBAL h, htext;
    HENHMETAFILE emf;
    BOOL r;
    UINT cf, i, j, count, rendered, seq, old_seq;
    HANDLE data;
    HWND hwnd;

    hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)renderer_winproc );

    htext = create_textA();
    emf = create_emf();

    r = open_clipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());
    h = SetClipboardData(CF_TEXT, htext);
    ok(h == htext, "got %p\n", h);
    h = SetClipboardData(CF_ENHMETAFILE, emf);
    ok(h == emf, "got %p\n", h);
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    count = CountClipboardFormats();
    ok( count == 6, "count %u\n", count );
    r = IsClipboardFormatAvailable( CF_TEXT );
    ok( r, "CF_TEXT not available err %ld\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_LOCALE );
    ok( r, "CF_LOCALE not available err %ld\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_OEMTEXT );
    ok( r, "CF_OEMTEXT not available err %ld\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_UNICODETEXT );
    ok( r, "CF_UNICODETEXT not available err %ld\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_ENHMETAFILE );
    ok( r, "CF_ENHMETAFILE not available err %ld\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_METAFILEPICT );
    ok( r, "CF_METAFILEPICT not available err %ld\n", GetLastError());

    r = open_clipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    cf = EnumClipboardFormats(0);
    ok(cf == CF_TEXT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_ENHMETAFILE, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_LOCALE, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_OEMTEXT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_UNICODETEXT, "cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_METAFILEPICT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == 0, "cf %08x\n", cf);

    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());

    SetClipboardData( CF_UNICODETEXT, create_textW() );
    SetClipboardData( CF_TEXT, create_textA() );
    SetClipboardData( CF_OEMTEXT, create_textA() );
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    r = open_clipboard( NULL );
    ok(r, "gle %ld\n", GetLastError());
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats(0);
    ok( cf == CF_UNICODETEXT, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats(cf);
    ok( cf == CF_TEXT, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats(cf);
    ok( cf == CF_OEMTEXT, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats(cf);
    ok( cf == CF_LOCALE, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats( cf );
    ok( cf == 0, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    cf = EnumClipboardFormats( 0xdead );
    ok( cf == 0, "cf %08x\n", cf );
    ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );

    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());

    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("%d", i);
        r = open_clipboard(NULL);
        ok(r, "gle %ld\n", GetLastError());
        r = EmptyClipboard();
        ok(r, "gle %ld\n", GetLastError());

        switch (tests[i].format)
        {
        case CF_TEXT:
        case CF_OEMTEXT:
            SetClipboardData( tests[i].format, create_textA() );
            break;
        case CF_UNICODETEXT:
            SetClipboardData( CF_UNICODETEXT, create_textW() );
            break;
        case CF_ENHMETAFILE:
            SetClipboardData( CF_ENHMETAFILE, create_emf() );
            break;
        case CF_METAFILEPICT:
            SetClipboardData( CF_METAFILEPICT, create_metafile() );
            break;
        case CF_BITMAP:
            SetClipboardData( CF_BITMAP, create_bitmap() );
            break;
        case CF_DIB:
        case CF_DIBV5:
            SetClipboardData( tests[i].format, create_dib( tests[i].format == CF_DIBV5 ));
            break;
        }

        count = CountClipboardFormats();
        ok( count == 1, "count %u\n", count );

        r = CloseClipboard();
        ok(r, "gle %ld\n", GetLastError());

        count = CountClipboardFormats();
        for (j = 0; tests[i].expected[j]; j++)
        {
            r = IsClipboardFormatAvailable( tests[i].expected[j] );
            ok( r, "%04x not available\n", tests[i].expected[j] );
        }
        ok( count == j, "count %u instead of %u\n", count, j );

        r = open_clipboard( hwnd );
        ok(r, "gle %ld\n", GetLastError());
        cf = 0;
        for (j = 0; tests[i].expected[j]; j++)
        {
            winetest_push_context("%d", j);
            cf = EnumClipboardFormats( cf );
            ok(cf == tests[i].expected[j], "got %04x instead of %04x\n",
               cf, tests[i].expected[j] );
            if (cf != tests[i].expected[j])
            {
                winetest_pop_context();
                break;
            }
            old_seq = GetClipboardSequenceNumber();
            data = GetClipboardData( cf );
            ok(data != NULL ||
               broken( tests[i].format == CF_DIBV5 && cf == CF_DIB ), /* >= Vista */
               "couldn't get data, cf %04x err %ld\n", cf, GetLastError());
            seq = GetClipboardSequenceNumber();
            ok(seq == old_seq, "sequence changed (test %d)\n", cf);
            switch (cf)
            {
            case CF_LOCALE:
            {
                UINT *ptr = GlobalLock( data );
                DWORD layout = LOWORD( GetKeyboardLayout(0) );
                ok( GlobalSize( data ) == sizeof(*ptr), "size %Iu\n", GlobalSize( data ));
                ok( *ptr == layout ||
                    broken( *ptr == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )),
                    "CF_LOCALE %04x/%04lx\n", *ptr, layout );
                GlobalUnlock( data );
                break;
            }
            case CF_TEXT:
            case CF_OEMTEXT:
                ok( GlobalSize( data ) == 10, "wrong len %Id\n", GlobalSize( data ));
                break;
            case CF_UNICODETEXT:
                ok( GlobalSize( data ) == 10 * sizeof(WCHAR), "wrong len %Id\n", GlobalSize( data ));
                break;
            }
            winetest_pop_context();
        }
        if (!tests[i].expected[j])
        {
            cf = EnumClipboardFormats( cf );
            ok(cf == 0, "cf %04x\n", cf);
        }

        /* now with delayed rendering */

        r = EmptyClipboard();
        ok(r, "gle %ld\n", GetLastError());

        rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
        ok( !rendered, "formats %08x have been rendered\n", rendered );

        SetClipboardData( tests[i].format, 0 );
        rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
        ok( !rendered, "formats %08x have been rendered\n", rendered );

        count = CountClipboardFormats();
        ok( count == 1, "count %u\n", count );

        r = CloseClipboard();
        ok(r, "gle %ld\n", GetLastError());
        rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
        ok( !rendered, "formats %08x have been rendered\n", rendered );

        count = CountClipboardFormats();
        for (j = 0; tests[i].expected[j]; j++)
        {
            r = IsClipboardFormatAvailable( tests[i].expected[j] );
            ok( r, "%04x not available\n", tests[i].expected[j] );
        }
        ok( count == j, "count %u instead of %u\n", count, j );
        rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
        ok( !rendered, "formats %08x have been rendered\n", rendered );

        r = open_clipboard(NULL);
        ok(r, "gle %ld\n", GetLastError());
        cf = 0;
        for (j = 0; tests[i].expected[j]; j++)
        {
            winetest_push_context("%d", j);
            cf = EnumClipboardFormats( cf );
            ok(cf == tests[i].expected[j], "got %04x instead of %04x\n",
               cf, tests[i].expected[j] );
            if (cf != tests[i].expected[j])
            {
                winetest_pop_context();
                break;
            }
            rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
            ok( !rendered, "formats %08x have been rendered\n", rendered );
            data = GetClipboardData( cf );
            rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
            if (cf == CF_LOCALE)
            {
                ok(data != NULL, "CF_LOCALE no data\n");
                ok( !rendered, "formats %08x have been rendered\n", rendered );
            }
            else
            {
                ok(!data, "format %04x got data %p\n", cf, data);
                ok( rendered == (1 << tests[i].format),
                    "formats %08x have been rendered\n", rendered );
                /* try to render a second time */
                data = GetClipboardData( cf );
                rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
                ok( rendered == (1 << tests[i].format),
                    "formats %08x have been rendered\n", rendered );
            }
            winetest_pop_context();
        }
        if (!tests[i].expected[j])
        {
            cf = EnumClipboardFormats( cf );
            ok(cf == 0, "cf %04x\n", cf);
        }
        r = CloseClipboard();
        ok(r, "gle %ld\n", GetLastError());
        rendered = SendMessageA( hwnd, WM_USER, 0, 0 );
        ok( !rendered, "formats %08x have been rendered\n", rendered );
        winetest_pop_context();
    }

    r = open_clipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());
    DestroyWindow( hwnd );

    /* Check what happens to the delayed rendering clipboard formats when the
     * owner window is destroyed.
     */
    hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)renderer_winproc );

    r = open_clipboard(hwnd);
    ok(r, "gle %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %ld\n", GetLastError());
    SetClipboardData( CF_UNICODETEXT, NULL );
    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    r = open_clipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());
    count = CountClipboardFormats();
    ok(count == 4, "count %u\n", count );

    DestroyWindow( hwnd );

    /* CF_UNICODETEXT and derivatives, CF_TEXT + CF_OEMTEXT, should be gone */
    count = CountClipboardFormats();
    ok(count == 1, "count %u\n", count );
    cf = EnumClipboardFormats( 0 );
    ok(cf == CF_LOCALE, "unexpected clipboard format %u\n", cf);

    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());

    r = open_clipboard(NULL);
    ok(r, "gle %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    data = GetClipboardData( CF_TEXT );
    ok(GetLastError() == ERROR_NOT_FOUND /* win11 */ ||
       broken(GetLastError() == 0xdeadbeef),
       "bad last error %ld\n", GetLastError());
    ok(!data, "GetClipboardData() should have returned NULL\n");

    r = CloseClipboard();
    ok(r, "gle %ld\n", GetLastError());
}

static DWORD WINAPI clipboard_render_data_thread(void *param)
{
    HANDLE handle = SetClipboardData( CF_UNICODETEXT, create_textW() );
    ok( handle != 0, "SetClipboardData failed: %ld\n", GetLastError() );
    return 0;
}

static CRITICAL_SECTION clipboard_cs;
static HWND next_wnd;
static UINT wm_drawclipboard;
static UINT wm_clipboardupdate;
static UINT wm_destroyclipboard;
static UINT wm_renderformat;
static UINT nb_formats;
static BOOL cross_thread;
static BOOL do_render_format;
static HANDLE update_event;

static LRESULT CALLBACK clipboard_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT ret;
    DWORD msg_flags = InSendMessageEx( NULL );

    switch(msg) {
    case WM_DRAWCLIPBOARD:
        ok( msg_flags == (cross_thread ? ISMEX_NOTIFY : ISMEX_NOSEND),
            "WM_DRAWCLIPBOARD wrong flags %lx\n", msg_flags );
        EnterCriticalSection(&clipboard_cs);
        wm_drawclipboard++;
        LeaveCriticalSection(&clipboard_cs);
        break;
    case WM_CHANGECBCHAIN:
        ok( msg_flags == (cross_thread ? ISMEX_SEND : ISMEX_NOSEND),
            "WM_CHANGECBCHAIN wrong flags %lx\n", msg_flags );
        if (next_wnd == (HWND)wp)
            next_wnd = (HWND)lp;
        else if (next_wnd)
            SendMessageA(next_wnd, msg, wp, lp);
        break;
    case WM_DESTROYCLIPBOARD:
        ok( msg_flags == (cross_thread ? ISMEX_SEND : ISMEX_NOSEND),
            "WM_DESTROYCLIPBOARD wrong flags %lx\n", msg_flags );
        wm_destroyclipboard++;
        ok( GetClipboardOwner() == hwnd, "WM_DESTROYCLIPBOARD owner %p\n", GetClipboardOwner() );
        nb_formats = CountClipboardFormats();
        break;
    case WM_RENDERFORMAT:
        ok( !wm_renderformat, "multiple WM_RENDERFORMAT %04x / %04Ix\n", wm_renderformat, wp );
        wm_renderformat = wp;

        if (do_render_format)
        {
            UINT seq, old_seq;
            HANDLE handle;

            old_seq = GetClipboardSequenceNumber();
            handle = SetClipboardData( CF_TEXT, create_textA() );
            ok( handle != 0, "SetClipboardData failed: %ld\n", GetLastError() );
            seq = GetClipboardSequenceNumber();
            ok( seq == old_seq, "sequence changed\n" );
            old_seq = seq;

            handle = CreateThread( NULL, 0, clipboard_render_data_thread, NULL, 0, NULL );
            ok( handle != NULL, "CreateThread failed: %ld\n", GetLastError() );
            ok( WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n" );
            CloseHandle( handle );
            seq = GetClipboardSequenceNumber();
            ok( seq == old_seq, "sequence changed\n" );
        }

        break;
    case WM_CLIPBOARDUPDATE:
        ok( msg_flags == ISMEX_NOSEND, "WM_CLIPBOARDUPDATE wrong flags %lx\n", msg_flags );
        EnterCriticalSection(&clipboard_cs);
        wm_clipboardupdate++;
        SetEvent(update_event);
        LeaveCriticalSection(&clipboard_cs);
        break;
    case WM_USER:
        ChangeClipboardChain(hwnd, next_wnd);
        PostQuitMessage(0);
        break;
    case WM_USER+1:
        ret = wm_drawclipboard;
        wm_drawclipboard = 0;
        return ret;
    case WM_USER+2:
        ret = wm_clipboardupdate;
        wm_clipboardupdate = 0;
        return ret;
    case WM_USER+3:
        ret = wm_destroyclipboard;
        wm_destroyclipboard = 0;
        return ret;
    case WM_USER+4:
        ret = wm_renderformat;
        wm_renderformat = 0;
        return ret;
    case WM_USER+5:
        return nb_formats;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void get_clipboard_data_process(void)
{
    HANDLE data;
    BOOL r;

    r = open_clipboard(0);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    data = GetClipboardData( CF_UNICODETEXT );
    ok( data != NULL, "GetClipboardData failed: %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
}

static UINT old_seq;

static void check_messages_(int line, HWND win, UINT seq_diff, UINT draw, UINT update, UINT destroy, UINT render)
{
    MSG msg;
    UINT count, fmt, seq;

    seq = GetClipboardSequenceNumber();
    ok_(__FILE__, line)(seq - old_seq == seq_diff, "sequence diff %d\n", seq - old_seq);
    old_seq = seq;

    if (!cross_thread)
    {
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }

    if (update && !broken(!pAddClipboardFormatListener))
        ok(WaitForSingleObject(update_event, 1000) == WAIT_OBJECT_0, "wait failed\n");

    count = SendMessageA( win, WM_USER + 1, 0, 0 );
    ok_(__FILE__, line)(count == draw, "WM_DRAWCLIPBOARD %sreceived\n", draw ? "not " : "");
    count = SendMessageA( win, WM_USER + 2, 0, 0 );
    ok_(__FILE__, line)(count == update || broken(!pAddClipboardFormatListener),
                        "WM_CLIPBOARDUPDATE %sreceived\n", update ? "not " : "");
    count = SendMessageA( win, WM_USER + 3, 0, 0 );
    ok_(__FILE__, line)(count == destroy, "WM_DESTROYCLIPBOARD %sreceived\n", destroy ? "not " : "");
    fmt = SendMessageA( win, WM_USER + 4, 0, 0 );
    ok_(__FILE__, line)(fmt == render, "WM_RENDERFORMAT received %04x, expected %04x\n", fmt, render);
}
#define check_messages(a,b,c,d,e,f) check_messages_(__LINE__,a,b,c,d,e,f)

static DWORD WINAPI clipboard_thread(void *param)
{
    HWND ret, win = param;
    BOOL r;
    MSG msg;
    HANDLE handle;
    UINT count, fmt, formats;

    cross_thread = (GetWindowThreadProcessId( win, NULL ) != GetCurrentThreadId());
    trace( "%s-threaded test\n", cross_thread ? "multi" : "single" );

    old_seq = GetClipboardSequenceNumber();

    EnterCriticalSection(&clipboard_cs);
    SetLastError(0xdeadbeef);
    next_wnd = SetClipboardViewer(win);
    ok(GetLastError() == 0xdeadbeef, "GetLastError = %ld\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    SetLastError( 0xdeadbeef );
    ret = SetClipboardViewer( (HWND)0xdead );
    ok( !ret, "SetClipboardViewer succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    r = ChangeClipboardChain( win, (HWND)0xdead );
    ok( !r, "ChangeClipboardChain succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    r = ChangeClipboardChain( (HWND)0xdead, next_wnd );
    ok( !r, "ChangeClipboardChain succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );

    if (pAddClipboardFormatListener)
    {
        r = pAddClipboardFormatListener(win);
        ok( r, "AddClipboardFormatListener failed err %ld\n", GetLastError());
        SetLastError( 0xdeadbeef );
        r = pAddClipboardFormatListener( win );
        ok( !r, "AddClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        r = pAddClipboardFormatListener( (HWND)0xdead );
        ok( !r, "AddClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
        r = pAddClipboardFormatListener( GetDesktopWindow() );
        ok( r, "AddClipboardFormatListener failed err %ld\n", GetLastError());
        r = pRemoveClipboardFormatListener( GetDesktopWindow() );
        ok( r, "RemoveClipboardFormatListener failed err %ld\n", GetLastError());
    }

    check_messages(win, 0, 1, 0, 0, 0);

    SetLastError( 0xdeadbeef );
    r = OpenClipboard( (HWND)0xdead );
    ok( !r, "OpenClipboard succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );

    r = open_clipboard(win);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());

    check_messages(win, 0, 0, 0, 0, 0);

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %ld\n", GetLastError());

    check_messages(win, 1, 0, 0, 0, 0);

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %ld\n", GetLastError());
    /* sequence changes again, even though it was already empty */
    check_messages(win, 1, 0, 0, 1, 0);
    count = SendMessageA( win, WM_USER+5, 0, 0 );
    ok( !count, "wrong format count %u on WM_DESTROYCLIPBOARD\n", count );

    handle = SetClipboardData( CF_TEXT, create_textA() );
    ok(handle != 0, "SetClipboardData failed: %ld\n", GetLastError());

    check_messages(win, 1, 0, 0, 0, 0);

    SetClipboardData( CF_UNICODETEXT, 0 );

    check_messages(win, 1, 0, 0, 0, 0);

    SetClipboardData( CF_UNICODETEXT, 0 );  /* same data again */

    check_messages(win, 1, 0, 0, 0, 0);

    ok( IsClipboardFormatAvailable( CF_TEXT ), "CF_TEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_UNICODETEXT ), "CF_UNICODETEXT available\n" );
    ok( !IsClipboardFormatAvailable( CF_OEMTEXT ), "CF_OEMTEXT available\n" );

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    check_messages(win, 2, 1, 1, 0, 0);

    r = open_clipboard(win);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());

    check_messages(win, 0, 0, 0, 0, 0);

    ok( IsClipboardFormatAvailable( CF_TEXT ), "CF_TEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_UNICODETEXT ), "CF_UNICODETEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_OEMTEXT ), "CF_OEMTEXT available\n" );

    ok( GetClipboardOwner() == win, "wrong owner %p\n", GetClipboardOwner());
    handle = GetClipboardData( CF_UNICODETEXT );
    ok( !handle, "got data for CF_UNICODETEXT\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( fmt == CF_UNICODETEXT, "WM_RENDERFORMAT received %04x\n", fmt );

    do_render_format = TRUE;
    handle = GetClipboardData( CF_OEMTEXT );
    ok( handle != NULL, "didn't get data for CF_OEMTEXT\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( fmt == CF_UNICODETEXT, "WM_RENDERFORMAT received %04x\n", fmt );
    do_render_format = FALSE;

    SetClipboardData( CF_WAVE, 0 );
    check_messages(win, 1, 0, 0, 0, 0);

    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    /* no synthesized format, so CloseClipboard doesn't change the sequence */
    check_messages(win, 0, 1, 1, 0, 0);

    r = open_clipboard(win);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    /* nothing changed */
    check_messages(win, 0, 0, 0, 0, 0);

    formats = CountClipboardFormats();
    r = open_clipboard(0);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %ld\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());

    check_messages(win, 1, 1, 1, 1, 0);
    count = SendMessageA( win, WM_USER+5, 0, 0 );
    ok( count == formats, "wrong format count %u on WM_DESTROYCLIPBOARD\n", count );

    r = open_clipboard(win);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    check_messages(win, 1, 0, 0, 0, 0);

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    check_messages(win, 0, 1, 1, 0, 0);

    run_process( "grab_clipboard 0" );

    if (!cross_thread)
    {
        /* in this case we get a cross-thread WM_DRAWCLIPBOARD */
        cross_thread = TRUE;
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        cross_thread = FALSE;
    }
    check_messages(win, 1, 1, 1, 0, 0);

    r = open_clipboard(0);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    check_messages(win, 1, 0, 0, 0, 0);

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    check_messages(win, 0, 1, 1, 0, 0);

    run_process( "grab_clipboard 1" );

    if (!cross_thread)
    {
        /* in this case we get a cross-thread WM_DRAWCLIPBOARD */
        cross_thread = TRUE;
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        cross_thread = FALSE;
    }
    check_messages(win, 2, 1, 1, 0, 0);

    r = open_clipboard(0);
    ok(r, "OpenClipboard failed: %ld\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    check_messages(win, 1, 0, 0, 0, 0);

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %ld\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    check_messages(win, 0, 1, 1, 0, 0);

    if (cross_thread)
    {
        r = open_clipboard( win );
        ok(r, "OpenClipboard failed: %ld\n", GetLastError());
        r = EmptyClipboard();
        ok(r, "EmptyClipboard failed: %ld\n", GetLastError());
        SetClipboardData( CF_TEXT, 0 );
        r = CloseClipboard();
        ok(r, "CloseClipboard failed: %ld\n", GetLastError());

        do_render_format = TRUE;
        old_seq = GetClipboardSequenceNumber();
        run_process( "get_clipboard_data" );
        do_render_format = FALSE;

        check_messages(win, 0, 1, 1, 0, CF_TEXT);
    }

    r = PostMessageA(win, WM_USER, 0, 0);
    ok(r, "PostMessage failed: %ld\n", GetLastError());

    if (pRemoveClipboardFormatListener)
    {
        r = pRemoveClipboardFormatListener(win);
        ok( r, "RemoveClipboardFormatListener failed err %ld\n", GetLastError());
        SetLastError( 0xdeadbeef );
        r = pRemoveClipboardFormatListener(win);
        ok( !r, "RemoveClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        r = pRemoveClipboardFormatListener( (HWND)0xdead );
        ok( !r, "RemoveClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
    }
    return 0;
}

static void test_messages(void)
{
    WNDCLASSA cls;
    HWND win;
    MSG msg;
    HANDLE thread;
    DWORD tid;

    InitializeCriticalSection(&clipboard_cs);
    update_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = clipboard_wnd_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "clipboard_test";
    RegisterClassA(&cls);

    win = CreateWindowA("clipboard_test", NULL, 0, 0, 0, 0, 0, NULL, 0, NULL, 0);
    ok(win != NULL, "CreateWindow failed: %ld\n", GetLastError());

    thread = CreateThread(NULL, 0, clipboard_thread, (void*)win, 0, &tid);
    ok(thread != NULL, "CreateThread failed: %ld\n", GetLastError());

    while(GetMessageA(&msg, NULL, 0, 0))
    {
        ok( msg.message != WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD was posted\n" );
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    DestroyWindow( win );

    /* same tests again but inside a single thread */

    win = CreateWindowA( "clipboard_test", NULL, 0, 0, 0, 0, 0, NULL, 0, NULL, 0 );
    ok( win != NULL, "CreateWindow failed: %ld\n", GetLastError() );

    clipboard_thread( win );
    DestroyWindow( win );

    UnregisterClassA("clipboard_test", GetModuleHandleA(NULL));
    DeleteCriticalSection(&clipboard_cs);
}

static BOOL is_moveable( HANDLE handle )
{
    void *ptr = GlobalLock( handle );
    if (ptr) GlobalUnlock( handle );
    return ptr && ptr != handle;
}

static BOOL is_fixed( HANDLE handle )
{
    void *ptr = GlobalLock( handle );
    if (ptr) GlobalUnlock( handle );
    return ptr && ptr == handle;
}

static BOOL is_freed( HANDLE handle )
{
    return !GlobalSize( handle );
}

static UINT format_id;
static HBITMAP bitmap, bitmap2;
static HPALETTE palette;
static const LOGPALETTE logpalette = { 0x300, 1, {{ 0x12, 0x34, 0x56, 0x78 }}};

static void test_handles( HWND hwnd )
{
    HGLOBAL h, htext, htext2, htext3, htext4, htext5;
    HGLOBAL hfixed, hfixed2, hmoveable, empty_fixed, empty_moveable;
    void *ptr;
    UINT format_id2 = RegisterClipboardFormatA( "another format" );
    BOOL r;
    HANDLE data;
    HBITMAP bitmap_temp;
    DWORD process;
    BOOL is_owner = (GetWindowThreadProcessId( hwnd, &process ) && process == GetCurrentProcessId());

    trace( "hwnd %p\n", hwnd );
    htext = create_textA();
    htext2 = create_textA();
    htext3 = create_textA();
    htext4 = create_textA();
    htext5 = create_textA();
    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );
    bitmap2 = CreateBitmap( 10, 10, 1, 1, NULL );
    palette = CreatePalette( &logpalette );

    hfixed = GlobalAlloc( GMEM_FIXED, 17 );
    hfixed2 = GlobalAlloc( GMEM_FIXED, 17 );
    ok( is_fixed( hfixed ), "expected fixed mem %p\n", hfixed );
    ok( GlobalSize( hfixed ) == 17, "wrong size %Iu\n", GlobalSize( hfixed ));

    hmoveable = GlobalAlloc( GMEM_MOVEABLE, 23 );
    ok( is_moveable( hmoveable ), "expected moveable mem %p\n", hmoveable );
    ok( GlobalSize( hmoveable ) == 23, "wrong size %Iu\n", GlobalSize( hmoveable ));

    empty_fixed = GlobalAlloc( GMEM_FIXED, 0 );
    ok( is_fixed( empty_fixed ), "expected fixed mem %p\n", empty_fixed );

    empty_moveable = GlobalAlloc( GMEM_MOVEABLE, 0 );
    /* discarded handles can't be GlobalLock'ed */
    ok( is_freed( empty_moveable ), "expected free mem %p\n", empty_moveable );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    h = SetClipboardData( CF_TEXT, htext );
    ok( h == htext, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( format_id, htext2 );
    ok( h == htext2, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    bitmap_temp = CreateBitmap( 10, 10, 1, 1, NULL );
    h = SetClipboardData( CF_BITMAP, bitmap_temp );
    ok( h == bitmap_temp, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( h == bitmap, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_DSPBITMAP, bitmap2 );
    ok( h == bitmap2, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_PALETTE, palette );
    ok( h == palette, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    h = SetClipboardData( CF_GDIOBJFIRST + 3, htext3 );
    ok( h == htext3, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( CF_PRIVATEFIRST + 7, htext5 );
    ok( h == htext5, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( format_id2, empty_moveable );
    ok( !h, "got %p\n", h );
    GlobalFree( empty_moveable );

    if (0)  /* crashes on vista64 */
    {
        ptr = HeapAlloc( GetProcessHeap(), 0, 0 );
        h = SetClipboardData( format_id2, ptr );
        ok( !h, "got %p\n", h );
        HeapFree( GetProcessHeap(), 0, ptr );
    }

    h = SetClipboardData( format_id2, empty_fixed );
    ok( h == empty_fixed, "got %p\n", h );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = SetClipboardData( 0xdeadbeef, hfixed2 );
    ok( h == hfixed2, "got %p\n", h );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = SetClipboardData( 0xdeadbabe, hmoveable );
    ok( h == hmoveable, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );

    ptr = HeapAlloc( GetProcessHeap(), 0, 37 );
    h = SetClipboardData( 0xdeadfade, ptr );
    ok( h == ptr || !h, "got %p\n", h );
    if (!h)  /* heap blocks are rejected on >= win8 */
    {
        HeapFree( GetProcessHeap(), 0, ptr );
        ptr = NULL;
    }

    data = GetClipboardData( CF_TEXT );
    ok( data == htext, "wrong data %p\n", data );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( format_id );
    ok( data == htext2, "wrong data %p, cf %08x\n", data, format_id );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( CF_GDIOBJFIRST + 3 );
    ok( data == htext3, "wrong data %p\n", data );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( CF_PRIVATEFIRST + 7 );
    ok( data == htext5, "wrong data %p\n", data );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( format_id2 );
    ok( data == empty_fixed, "wrong data %p\n", data );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );

    data = GetClipboardData( 0xdeadbeef );
    ok( data == hfixed2, "wrong data %p\n", data );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );

    data = GetClipboardData( 0xdeadbabe );
    ok( data == hmoveable, "wrong data %p\n", data );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( 0xdeadfade );
    ok( data == ptr, "wrong data %p\n", data );

    SetLastError( 0xdeadbeef );
    data = GetClipboardData( CF_RIFF );
    ok( GetLastError() == ERROR_NOT_FOUND /* win11 */ ||
        broken(GetLastError() == 0xdeadbeef),
        "unexpected last error %ld\n", GetLastError() );
    ok( !data, "wrong data %p\n", data );

    h = SetClipboardData( CF_PRIVATEFIRST + 7, htext4 );
    ok( h == htext4, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    ok( is_freed( htext5 ), "expected freed mem %p\n", htext5 );

    h = SetClipboardData( 0xdeadbeef, hfixed );
    ok( h == hfixed, "got %p\n", h );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    if (0) /* this test is unreliable / crashes */
        ok( is_freed( hfixed2 ), "expected freed mem %p\n", hfixed2 );

    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    /* data handles are still valid */
    ok( is_moveable( htext ), "expected moveable mem %p\n", htext );
    ok( is_moveable( htext2 ), "expected moveable mem %p\n", htext2 );
    ok( is_moveable( htext3 ), "expected moveable mem %p\n", htext3 );
    ok( is_moveable( htext4 ), "expected moveable mem %p\n", htext4 );
    ok( GetObjectType( bitmap ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap );
    ok( GetObjectType( bitmap2 ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap2 );
    ok( GetObjectType( palette ) == OBJ_PAL, "expected palette %p\n", palette );
    ok( is_fixed( hfixed ), "expected fixed mem %p\n", hfixed );
    ok( is_moveable( hmoveable ), "expected moveable mem %p\n", hmoveable );
    ok( is_fixed( empty_fixed ), "expected fixed mem %p\n", empty_fixed );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );

    /* and now they are freed, unless we are the owner */
    if (!is_owner)
    {
        ok( is_freed( htext ), "expected freed mem %p\n", htext );
        ok( is_freed( htext2 ), "expected freed mem %p\n", htext2 );
        ok( is_freed( htext3 ), "expected freed mem %p\n", htext3 );
        ok( is_freed( htext4 ), "expected freed mem %p\n", htext4 );
        ok( is_freed( hmoveable ), "expected freed mem %p\n", hmoveable );

        data = GetClipboardData( CF_TEXT );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );

        data = GetClipboardData( format_id );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );

        data = GetClipboardData( CF_GDIOBJFIRST + 3 );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );

        data = GetClipboardData( CF_PRIVATEFIRST + 7 );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );

        data = GetClipboardData( format_id2 );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );
        ok( GlobalSize( data ) == 1, "wrong size %Iu\n", GlobalSize( data ));

        data = GetClipboardData( 0xdeadbeef );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );
        ok( GlobalSize( data ) == 17, "wrong size %Iu\n", GlobalSize( data ));

        data = GetClipboardData( 0xdeadbabe );
        ok( is_fixed( data ), "expected fixed mem %p\n", data );
        ok( GlobalSize( data ) == 23, "wrong size %Iu\n", GlobalSize( data ));

        data = GetClipboardData( 0xdeadfade );
        ok( is_fixed( data ) || !ptr, "expected fixed mem %p\n", data );
        if (ptr) ok( GlobalSize( data ) == 37, "wrong size %Iu\n", GlobalSize( data ));
    }
    else
    {
        ok( is_moveable( htext ), "expected moveable mem %p\n", htext );
        ok( is_moveable( htext2 ), "expected moveable mem %p\n", htext2 );
        ok( is_moveable( htext3 ), "expected moveable mem %p\n", htext3 );
        ok( is_moveable( htext4 ), "expected moveable mem %p\n", htext4 );
        ok( is_moveable( hmoveable ), "expected moveable mem %p\n", hmoveable );

        data = GetClipboardData( CF_TEXT );
        ok( data == htext, "wrong data %p\n", data );

        data = GetClipboardData( format_id );
        ok( data == htext2, "wrong data %p, cf %08x\n", data, format_id );

        data = GetClipboardData( CF_GDIOBJFIRST + 3 );
        ok( data == htext3, "wrong data %p\n", data );

        data = GetClipboardData( CF_PRIVATEFIRST + 7 );
        ok( data == htext4, "wrong data %p\n", data );

        data = GetClipboardData( format_id2 );
        ok( data == empty_fixed, "wrong data %p\n", data );

        data = GetClipboardData( 0xdeadbeef );
        ok( data == hfixed, "wrong data %p\n", data );

        data = GetClipboardData( 0xdeadbabe );
        ok( data == hmoveable, "wrong data %p\n", data );

        data = GetClipboardData( 0xdeadfade );
        ok( data == ptr, "wrong data %p\n", data );
    }

    data = GetClipboardData( CF_OEMTEXT );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_UNICODETEXT );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_LOCALE );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_BITMAP );
    ok( data == bitmap, "expected bitmap %p\n", data );
    data = GetClipboardData( CF_DSPBITMAP );
    ok( data == bitmap2, "expected bitmap %p\n", data );
    data = GetClipboardData( CF_PALETTE );
    ok( data == palette, "expected palette %p\n", data );
    data = GetClipboardData( CF_DIB );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_DIBV5 );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );

    ok( GetObjectType( bitmap ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap );
    ok( GetObjectType( bitmap2 ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap2 );
    ok( GetObjectType( palette ) == OBJ_PAL, "expected palette %p\n", palette );
    ok( is_fixed( hfixed ), "expected fixed mem %p\n", hfixed );
    ok( is_fixed( empty_fixed ), "expected fixed mem %p\n", empty_fixed );

    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    /* w2003, w2008 don't seem to free the data here */
    ok( is_freed( htext ) || broken( !is_freed( htext )), "expected freed mem %p\n", htext );
    ok( is_freed( htext2 ) || broken( !is_freed( htext2 )), "expected freed mem %p\n", htext2 );
    ok( is_freed( htext3 ) || broken( !is_freed( htext3 )), "expected freed mem %p\n", htext3 );
    ok( is_freed( htext4 ) || broken( !is_freed( htext4 )), "expected freed mem %p\n", htext4 );
    ok( is_freed( hmoveable ) || broken( !is_freed( hmoveable )), "expected freed mem %p\n", hmoveable );
    ok( is_fixed( empty_fixed ), "expected fixed mem %p\n", empty_fixed );
    ok( is_fixed( hfixed ), "expected fixed mem %p\n", hfixed );

    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );
}

static DWORD WINAPI test_handles_thread( void *arg )
{
    trace( "running from different thread\n" );
    test_handles( (HWND)arg );
    return 0;
}

static DWORD WINAPI test_handles_thread2( void *arg )
{
    BOOL r;
    HANDLE h;
    char *ptr;

    r = open_clipboard( 0 );
    ok( r, "gle %ld\n", GetLastError() );
    h = GetClipboardData( CF_TEXT );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( "test", ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( format_id );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( "test", ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( CF_GDIOBJFIRST + 3 );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( "test", ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    trace( "gdiobj %p\n", h );
    h = GetClipboardData( CF_PRIVATEFIRST + 7 );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( "test", ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    trace( "private %p\n", h );
    h = GetClipboardData( CF_BITMAP );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    ok( h == bitmap, "different bitmap %p / %p\n", h, bitmap );
    trace( "bitmap %p\n", h );
    h = GetClipboardData( CF_DSPBITMAP );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    ok( h == bitmap2, "different bitmap %p / %p\n", h, bitmap2 );
    trace( "bitmap2 %p\n", h );
    h = GetClipboardData( CF_PALETTE );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    ok( h == palette, "different palette %p / %p\n", h, palette );
    trace( "palette %p\n", h );
    h = GetClipboardData( CF_DIB );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = GetClipboardData( CF_DIBV5 );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    return 0;
}

static void test_handles_process( const char *str )
{
    BOOL r;
    HANDLE h;
    char *ptr;
    BITMAP bm;
    PALETTEENTRY entry;
    BYTE buffer[1024];

    format_id = RegisterClipboardFormatA( "my_cool_clipboard_format" );
    r = open_clipboard( 0 );
    ok( r, "gle %ld\n", GetLastError() );
    h = GetClipboardData( CF_TEXT );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( format_id );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( CF_GDIOBJFIRST + 3 );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    trace( "gdiobj %p\n", h );
    h = GetClipboardData( CF_PRIVATEFIRST + 7 );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    trace( "private %p\n", h );
    h = GetClipboardData( CF_BITMAP );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    ok( GetObjectW( h, sizeof(bm), &bm ) == sizeof(bm), "GetObject %p failed\n", h );
    ok( bm.bmWidth == 13 && bm.bmHeight == 17, "wrong bitmap %ux%u\n", bm.bmWidth, bm.bmHeight );
    trace( "bitmap %p\n", h );
    h = GetClipboardData( CF_DSPBITMAP );
    ok( !GetObjectType( h ), "expected invalid object %p\n", h );
    trace( "bitmap2 %p\n", h );
    h = GetClipboardData( CF_PALETTE );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    ok( GetPaletteEntries( h, 0, 1, &entry ) == 1, "GetPaletteEntries %p failed\n", h );
    ok( entry.peRed == 0x12 && entry.peGreen == 0x34 && entry.peBlue == 0x56,
        "wrong color %02x,%02x,%02x\n", entry.peRed, entry.peGreen, entry.peBlue );
    trace( "palette %p\n", h );
    h = GetClipboardData( CF_METAFILEPICT );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ok( GetObjectType( ((METAFILEPICT *)h)->hMF ) == OBJ_METAFILE,
        "wrong object %p\n", ((METAFILEPICT *)h)->hMF );
    trace( "metafile %p\n", h );
    h = GetClipboardData( CF_DSPMETAFILEPICT );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ok( GetObjectType( ((METAFILEPICT *)h)->hMF ) == OBJ_METAFILE,
        "wrong object %p\n", ((METAFILEPICT *)h)->hMF );
    trace( "metafile2 %p\n", h );
    h = GetClipboardData( CF_ENHMETAFILE );
    ok( GetObjectType( h ) == OBJ_ENHMETAFILE, "expected enhmetafile %p\n", h );
    ok( GetEnhMetaFileBits( h, sizeof(buffer), buffer ) > sizeof(ENHMETAHEADER),
        "GetEnhMetaFileBits failed on %p\n", h );
    ok( ((ENHMETAHEADER *)buffer)->nRecords == 3,
        "wrong records %lu\n", ((ENHMETAHEADER *)buffer)->nRecords );
    trace( "enhmetafile %p\n", h );
    h = GetClipboardData( CF_DSPENHMETAFILE );
    ok( GetObjectType( h ) == OBJ_ENHMETAFILE, "expected enhmetafile %p\n", h );
    ok( GetEnhMetaFileBits( h, sizeof(buffer), buffer ) > sizeof(ENHMETAHEADER),
        "GetEnhMetaFileBits failed on %p\n", h );
    ok( ((ENHMETAHEADER *)buffer)->nRecords == 3,
        "wrong records %lu\n", ((ENHMETAHEADER *)buffer)->nRecords );
    trace( "enhmetafile2 %p\n", h );
    h = GetClipboardData( CF_DIB );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = GetClipboardData( CF_DIBV5 );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );
}

static void test_handles_process_open( const char *str )
{
    HANDLE h, text = GlobalAlloc( GMEM_DDESHARE|GMEM_MOVEABLE, strlen(str) + 1 );
    char *ptr = GlobalLock( text );

    strcpy( ptr, str );
    GlobalUnlock( text );

    /* clipboard already open by parent process */
    h = SetClipboardData( CF_TEXT,  text );
    ok( h == text, "wrong mem %p / %p\n", h, text );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
}

static void test_handles_process_dib( const char *str )
{
    BOOL r;
    HANDLE h;

    r = open_clipboard( 0 );
    ok( r, "gle %ld\n", GetLastError() );
    h = GetClipboardData( CF_BITMAP );
    ok( !GetObjectType( h ), "expected invalid object %p\n", h );
    trace( "dibsection %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );
}

static void test_data_handles(void)
{
    BOOL r;
    char *ptr;
    HANDLE h, text, metafile;
    HWND hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );
    BITMAPINFO bmi;
    void *bits;
    PALETTEENTRY entry;
    BYTE buffer[1024];
    HENHMETAFILE emf;
    int result;
    HDC hdc = GetDC( 0 );

    ok( hwnd != 0, "window creation failed\n" );
    format_id = RegisterClipboardFormatA( "my_cool_clipboard_format" );
    test_handles( 0 );
    test_handles( GetDesktopWindow() );
    test_handles( hwnd );
    run_thread( test_handles_thread, hwnd, __LINE__ );

    bitmap = CreateBitmap( 13, 17, 1, 1, NULL );
    bitmap2 = CreateBitmap( 10, 10, 1, 1, NULL );
    palette = CreatePalette( &logpalette );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    h = SetClipboardData( CF_TEXT, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( format_id, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_DSPBITMAP, bitmap2 );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_PALETTE, palette );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    h = SetClipboardData( CF_METAFILEPICT, create_metafile() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    trace( "metafile %p\n", h );
    h = SetClipboardData( CF_DSPMETAFILEPICT, create_metafile() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    trace( "metafile2 %p\n", h );
    h = SetClipboardData( CF_ENHMETAFILE, create_emf() );
    ok( GetObjectType( h ) == OBJ_ENHMETAFILE, "expected enhmetafile %p\n", h );
    trace( "enhmetafile %p\n", h );
    h = SetClipboardData( CF_DSPENHMETAFILE, create_emf() );
    ok( GetObjectType( h ) == OBJ_ENHMETAFILE, "expected enhmetafile %p\n", h );
    trace( "enhmetafile2 %p\n", h );
    h = SetClipboardData( CF_GDIOBJFIRST + 3, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( CF_PRIVATEFIRST + 7, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    run_thread( test_handles_thread2, 0, __LINE__ );
    run_process( "handles test" );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );
    h = GetClipboardData( CF_TEXT );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = GetClipboardData( format_id );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = GetClipboardData( CF_GDIOBJFIRST + 3 );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = GetClipboardData( CF_PRIVATEFIRST + 7 );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );

    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    text = create_textA();
    h = SetClipboardData( CF_TEXT, text );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );

    run_process( "handles_open foobar" );

    ok( is_moveable( text ), "expected moveable mem %p\n", text );
    h = GetClipboardData( CF_TEXT );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ok( is_moveable( text ), "expected moveable mem %p\n", text );
    ptr = GlobalLock( h );
    ok( ptr && !strcmp( ptr, "foobar" ), "wrong data %s\n", wine_dbgstr_a(ptr) );
    GlobalUnlock( h );

    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    ok( is_fixed( h ), "expected free mem %p\n", h );
    ok( is_freed( text ) || broken( is_moveable(text) ), /* w2003, w2008 */
        "expected free mem %p\n", text );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    /* test CF_BITMAP with a DIB section */
    memset( &bmi, 0, sizeof(bmi) );
    bmi.bmiHeader.biSize = sizeof( bmi.bmiHeader );
    bmi.bmiHeader.biWidth = 29;
    bmi.bmiHeader.biHeight = 13;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bitmap = CreateDIBSection( 0, &bmi, DIB_RGB_COLORS, &bits, 0, 0 );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    trace( "dibsection %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    run_process( "handles_dib dummy" );

    r = open_clipboard( hwnd );
    ok( r, "gle %ld\n", GetLastError() );
    ok( GetObjectType( bitmap ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap );
    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    r = open_clipboard( hwnd );
    ok( r, "Open clipboard failed: %#lx.\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "EmptyClipboard failed: %#lx.\n", GetLastError() );

    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( h == bitmap, "Expected bitmap %p, got %p.\n", bitmap, h );
    ok( !!DeleteObject( bitmap ), "DeleteObject failed.\n" );
    h = GetClipboardData( CF_BITMAP );
    ok( h == bitmap, "Expected bitmap %p, got %p.\n", bitmap, h );
    memset( &bmi, 0, sizeof(bmi) );
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    result = GetDIBits( hdc, h, 0, 0, NULL, &bmi, 0 );
    ok( !!result, "GetDIBits failed: %#lx.\n", GetLastError() );

    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );
    h = SetClipboardData( CF_DSPBITMAP, bitmap );
    ok( h == bitmap, "Expected bitmap %p, got %p.\n", bitmap, h );
    ok( !!DeleteObject( bitmap ), "DeleteObject failed.\n" );
    h = GetClipboardData( CF_DSPBITMAP );
    ok( h == bitmap, "Expected bitmap %p, got %p.\n", bitmap, h );
    memset( &bmi, 0, sizeof(bmi) );
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ok( !GetDIBits( hdc, h, 0, 0, 0, &bmi, 0 ), "GetDIBits returned unexpected value.\n" );

    palette = CreatePalette( &logpalette );
    h = SetClipboardData( CF_PALETTE, palette );
    ok( h == palette, "Expected palette %p, got %p.\n", palette, h );
    ok( !!DeleteObject( palette ), "DeleteObject failed.\n" );
    h = GetClipboardData( CF_PALETTE );
    ok( h == palette, "Expected palette %p, got %p.\n", palette, h );
    ok( !!GetPaletteEntries( h, 0, 1, &entry ), "GetPaletteEntries %p failed.\n", h );
    ok( entry.peRed == 0x12 && entry.peGreen == 0x34 && entry.peBlue == 0x56,
            "Got wrong color (%02x, %02x, %02x).\n", entry.peRed, entry.peGreen, entry.peBlue );

    emf = create_emf();
    h = SetClipboardData( CF_ENHMETAFILE, emf );
    ok( h == emf, "Expected enhmetafile %p, got %p.\n", palette, h );
    ok( !!DeleteEnhMetaFile( emf ), "DeleteEnhMetaFile failed.\n" );
    h = GetClipboardData( CF_ENHMETAFILE );
    ok( h == emf, "Expected enhmetafile %p, got %p.\n", palette, h );
    ok( !GetEnhMetaFileBits( h, sizeof(buffer), buffer ), "GetEnhMetaFileBits returned unexpected value.\n" );

    emf = create_emf();
    h = SetClipboardData( CF_DSPENHMETAFILE, emf );
    ok( h == emf, "Expected enhmetafile %p, got %p.\n", emf, h );
    ok( !!DeleteEnhMetaFile( emf ), "DeleteEnhMetaFile failed.\n" );
    h = GetClipboardData( CF_DSPENHMETAFILE );
    ok( h == emf, "Expected enhmetafile %p, got %p.\n", emf, h );
    ok( !GetEnhMetaFileBits( h, sizeof(buffer), buffer ), "GetEnhMetaFileBits returned unexpected value.\n" );

    metafile = create_metafile();
    h = SetClipboardData( CF_METAFILEPICT, metafile );
    ok( h == metafile, "Expected metafilepict %p, got %p.\n", metafile, h );
    ok( !pGlobalFree( metafile ), "GlobalFree failed.\n" );
    h = GetClipboardData( CF_METAFILEPICT );
    ok( h == metafile, "Expected metafile %p, got %p.\n", metafile, h );
    ok( is_freed( h ), "Expected freed mem %p.\n", h );

    metafile = create_metafile();
    h = SetClipboardData( CF_DSPMETAFILEPICT, metafile );
    ok( h == metafile, "Expected metafilepict %p, got %p.\n", metafile, h );
    ok( !pGlobalFree( metafile ), "GlobalFree failed.\n" );
    h = GetClipboardData( CF_DSPMETAFILEPICT );
    ok( h == metafile, "Expected metafile %p, got %p.\n", metafile, h );
    ok( is_freed( h ), "Expected freed mem %p.\n", h );

    r = EmptyClipboard();
    ok( r, "EmptyClipboard failed: %#lx.\n", GetLastError() );
    r = CloseClipboard();
    ok( r, "CloseClipboard failed: %#lx.\n", GetLastError() );

    ReleaseDC( 0, hdc );
    DestroyWindow( hwnd );
}

static void test_GetUpdatedClipboardFormats(void)
{
    BOOL r;
    UINT count, formats[256];

    if (!pGetUpdatedClipboardFormats)
    {
        win_skip( "GetUpdatedClipboardFormats not supported\n" );
        return;
    }

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( NULL, 0, &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( !count, "wrong count %u\n", count );

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( NULL, 256, &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( !count, "wrong count %u\n", count );

    SetLastError( 0xdeadbeef );
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), NULL );
    ok( !r, "succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( !count, "wrong count %u\n", count );

    r = open_clipboard( 0 );
    ok( r, "gle %ld\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( !count, "wrong count %u\n", count );

    SetClipboardData( CF_UNICODETEXT, 0 );

    count = 0xdeadbeef;
    memset( formats, 0xcc, sizeof(formats) );
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( count == 1, "wrong count %u\n", count );
    ok( formats[0] == CF_UNICODETEXT, "wrong format %u\n", formats[0] );
    ok( formats[1] == 0xcccccccc, "wrong format %u\n", formats[1] );

    SetClipboardData( CF_TEXT, 0 );
    count = 0xdeadbeef;
    memset( formats, 0xcc, sizeof(formats) );
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( count == 2, "wrong count %u\n", count );
    ok( formats[0] == CF_UNICODETEXT, "wrong format %u\n", formats[0] );
    ok( formats[1] == CF_TEXT, "wrong format %u\n", formats[1] );
    ok( formats[2] == 0xcccccccc, "wrong format %u\n", formats[2] );

    SetLastError( 0xdeadbeef );
    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( formats, 0, &count );
    ok( !r, "succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );
    ok( count == 2, "wrong count %u\n", count );

    SetLastError( 0xdeadbeef );
    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( formats, 1, &count );
    ok( !r, "succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );
    ok( count == 2, "wrong count %u\n", count );

    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );

    count = 0xdeadbeef;
    memset( formats, 0xcc, sizeof(formats) );
    r = pGetUpdatedClipboardFormats( formats, ARRAY_SIZE(formats), &count );
    ok( r, "gle %ld\n", GetLastError() );
    ok( count == 4, "wrong count %u\n", count );
    ok( formats[0] == CF_UNICODETEXT, "wrong format %u\n", formats[0] );
    ok( formats[1] == CF_TEXT, "wrong format %u\n", formats[1] );
    ok( formats[2] == CF_LOCALE, "wrong format %u\n", formats[2] );
    ok( formats[3] == CF_OEMTEXT, "wrong format %u\n", formats[3] );
    ok( formats[4] == 0xcccccccc, "wrong format %u\n", formats[4] );

    count = 0xdeadbeef;
    memset( formats, 0xcc, sizeof(formats) );
    r = pGetUpdatedClipboardFormats( formats, 2, &count );
    ok( !r, "gle %ld\n", GetLastError() );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );
    ok( count == 4, "wrong count %u\n", count );
    ok( formats[0] == 0xcccccccc, "wrong format %u\n", formats[0] );

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( NULL, 256, &count );
    ok( !r, "gle %ld\n", GetLastError() );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );
    ok( count == 4, "wrong count %u\n", count );

    count = 0xdeadbeef;
    r = pGetUpdatedClipboardFormats( NULL, 256, &count );
    ok( !r, "gle %ld\n", GetLastError() );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );
    ok( count == 4, "wrong count %u\n", count );
}

static const struct
{
    char strA[12];
    WCHAR strW[12];
    UINT  len;
} test_data[] =
{
#if defined(__REACTOS__) && defined(_MSC_VER) && _MSVC_VER < 1930
    { "foo", {0}, 3 },                 /* 0 */
    { "foo", {0}, 4 },
    { "foo\0bar", {0}, 7 },
    { "foo\0bar", {0}, 8 },
    { "", {0}, 0 },
#else
    { "foo", {}, 3 },                 /* 0 */
    { "foo", {}, 4 },
    { "foo\0bar", {}, 7 },
    { "foo\0bar", {}, 8 },
    { "", {}, 0 },
#endif
    { "", {'f'}, 1 },                 /* 5 */
    { "", {'f'}, 2 },
    { "", {0x3b1,0x3b2,0x3b3}, 5 }, /* Alpha, beta, ... */
    { "", {0x3b1,0x3b2,0x3b3}, 6 },
    { "", {0x3b1,0x3b2,0x3b3,0}, 7 }, /* 10 */
    { "", {0x3b1,0x3b2,0x3b3,0}, 8 },
    { "", {0x3b1,0x3b2,0x3b3,0,0x3b4}, 9 },
    { "", {'f','o','o',0,'b','a','r'}, 7 * sizeof(WCHAR) },
    { "", {'f','o','o',0,'b','a','r',0}, 8 * sizeof(WCHAR) },
};

static void test_string_data(void)
{
    UINT i;
    BOOL r;
    HANDLE data, clip;
    char cmd[16];
    char bufferA[12];
    WCHAR bufferW[12];

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
#ifdef _WIN64
        /* Before Windows 11 1-byte Unicode strings crash on Win64
         * because it underflows when 'appending' a 2 bytes NUL character!
         */
        if (!test_data[i].strA[0] && test_data[i].len < sizeof(WCHAR) &&
            strcmp(winetest_platform, "windows") == 0) continue;
#endif
        winetest_push_context("%d", i);
        r = open_clipboard( 0 );
        ok( r, "gle %ld\n", GetLastError() );
        r = EmptyClipboard();
        ok( r, "gle %ld\n", GetLastError() );
        data = GlobalAlloc( GMEM_FIXED, test_data[i].len );
        if (test_data[i].strA[0])
        {
            memcpy( data, test_data[i].strA, test_data[i].len );
            SetClipboardData( CF_TEXT, data );
            memcpy( bufferA, test_data[i].strA, test_data[i].len );
            bufferA[test_data[i].len - 1] = 0;
            ok( !memcmp( data, bufferA, test_data[i].len ),
                "wrong data %.*s\n", test_data[i].len, (char *)data );
        }
        else
        {
            memcpy( data, test_data[i].strW, test_data[i].len );
            clip = SetClipboardData( CF_UNICODETEXT, data );
            if (test_data[i].len >= sizeof(WCHAR))
            {
                ok( clip == data, "SetClipboardData() returned %p != %p\n", clip, data );
                memcpy( bufferW, test_data[i].strW, test_data[i].len );
                *((WCHAR *)((char *)bufferW + test_data[i].len) - 1) = 0;
                ok( !memcmp( data, bufferW, test_data[i].len ),
                    "wrong data %s\n", wine_dbgstr_an( data, test_data[i].len ));
            }
            else
                ok( !clip || broken(clip != NULL), "expected SetClipboardData() to fail\n" );
        }
        r = CloseClipboard();
        ok( r, "gle %ld\n", GetLastError() );
        sprintf( cmd, "string_data %u", i );
        run_process( cmd );
        winetest_pop_context();
    }
}

static void test_string_data_process( int i )
{
    BOOL r;
    HANDLE data;
    UINT len, len2;
    char bufferA[12];
    WCHAR bufferW[12];

    winetest_push_context("%d", i);
    r = open_clipboard( 0 );
    ok( r, "gle %ld\n", GetLastError() );
    if (test_data[i].strA[0])
    {
        data = GetClipboardData( CF_TEXT );
        ok( data != 0, "could not get data\n" );
        len = GlobalSize( data );
        ok( len == test_data[i].len, "wrong size %u / %u\n", len, test_data[i].len );
        memcpy( bufferA, test_data[i].strA, test_data[i].len );
        bufferA[test_data[i].len - 1] = 0;
        ok( !memcmp( data, bufferA, len ), "wrong data %.*s\n", len, (char *)data );
        data = GetClipboardData( CF_UNICODETEXT );
        ok( data != 0, "could not get data\n" );
        len = GlobalSize( data );
        len2 = MultiByteToWideChar( CP_ACP, 0, bufferA, test_data[i].len, bufferW, ARRAY_SIZE(bufferW) );
        ok( len == len2 * sizeof(WCHAR), "wrong size %u / %u\n", len, len2 );
        ok( !memcmp( data, bufferW, len ), "wrong data %s\n", wine_dbgstr_wn( data, len2 ));
    }
    else
    {
        data = GetClipboardData( CF_UNICODETEXT );
        if (!data)
        {
            ok( test_data[i].len < sizeof(WCHAR), "got no data for len=%d\n", test_data[i].len );
            ok( !IsClipboardFormatAvailable( CF_UNICODETEXT ), "unicode available\n" );
        }
        else if (test_data[i].len == 0)
        {
            /* 0-byte string handling is broken on Windows <= 10 */
            len = GlobalSize( data );
            ok( broken(len == 1), "wrong size %u / 0\n", len );
            ok( broken(*((char*)data) == 0), "wrong data %s\n", wine_dbgstr_an( data, len ));
        }
        else
        {
            len = GlobalSize( data );
            ok( len == test_data[i].len, "wrong size %u / %u\n", len, test_data[i].len );
            memcpy( bufferW, test_data[i].strW, test_data[i].len );
            *((WCHAR *)((char *)bufferW + test_data[i].len) - 1) = 0;
            ok( !memcmp( data, bufferW, len ), "wrong data %s\n", wine_dbgstr_an( data, len ));
        }

        data = GetClipboardData( CF_TEXT );
        if (test_data[i].len >= sizeof(WCHAR))
        {
            ok( data != 0, "could not get data\n" );
            len = GlobalSize( data );
            len2 = WideCharToMultiByte( CP_ACP, 0, bufferW, test_data[i].len / sizeof(WCHAR),
                                        bufferA, ARRAY_SIZE(bufferA), NULL, NULL );
            bufferA[len2 - 1] = 0;
            ok( len == len2, "wrong size %u / %u\n", len, len2 );
            ok( !memcmp( data, bufferA, len ), "wrong data %s, expected %s\n",
                wine_dbgstr_an( data, len ), wine_dbgstr_an( bufferA, len2 ));
        }
        else
        {
            BOOL available = IsClipboardFormatAvailable( CF_TEXT );
            ok( !available || broken(available /* win10 */),
                "available text %d\n", available );
            ok( !data, "got data for empty string\n" );
        }
    }
    r = CloseClipboard();
    ok( r, "gle %ld\n", GetLastError() );
    winetest_pop_context();
}

START_TEST(clipboard)
{
    char **argv;
    int argc = winetest_get_mainargs( &argv );
    HMODULE mod = GetModuleHandleA( "user32" );

    argv0 = argv[0];
    pAddClipboardFormatListener = (void *)GetProcAddress( mod, "AddClipboardFormatListener" );
    pRemoveClipboardFormatListener = (void *)GetProcAddress( mod, "RemoveClipboardFormatListener" );
    pGetUpdatedClipboardFormats = (void *)GetProcAddress( mod, "GetUpdatedClipboardFormats" );
    pGlobalFree = (void *)GetProcAddress( GetModuleHandleA( "kernel32" ), "GlobalFree" );

    if (argc == 4 && !strcmp( argv[2], "set_clipboard_data" ))
    {
        set_clipboard_data_process( atoi( argv[3] ));
        return;
    }
    if (argc == 4 && !strcmp( argv[2], "grab_clipboard" ))
    {
        grab_clipboard_process( atoi( argv[3] ));
        return;
    }
    if (argc == 4 && !strcmp( argv[2], "handles" ))
    {
        test_handles_process( argv[3] );
        return;
    }
    if (argc == 4 && !strcmp( argv[2], "handles_open" ))
    {
        test_handles_process_open( argv[3] );
        return;
    }
    if (argc == 4 && !strcmp( argv[2], "handles_dib" ))
    {
        test_handles_process_dib( argv[3] );
        return;
    }
    if (argc == 4 && !strcmp( argv[2], "string_data" ))
    {
        test_string_data_process( atoi( argv[3] ));
        return;
    }
    if (argc == 3 && !strcmp( argv[2], "get_clipboard_data" ))
    {
        get_clipboard_data_process( );
        return;
    }

    test_RegisterClipboardFormatA();
    test_ClipboardOwner();
    test_synthesized();
    test_messages();
    test_data_handles();
    test_GetUpdatedClipboardFormats();
    test_string_data();
}
