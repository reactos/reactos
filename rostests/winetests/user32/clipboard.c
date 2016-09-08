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

#define WM_CLIPBOARDUPDATE 0x031D

static BOOL (WINAPI *pAddClipboardFormatListener)(HWND hwnd);
static BOOL (WINAPI *pRemoveClipboardFormatListener)(HWND hwnd);
static DWORD (WINAPI *pGetClipboardSequenceNumber)(void);

static const BOOL is_win64 = sizeof(void *) > sizeof(int);
static int thread_from_line;
static char *argv0;

static DWORD WINAPI open_clipboard_thread(LPVOID arg)
{
    HWND hWnd = arg;
    ok(OpenClipboard(hWnd), "%u: OpenClipboard failed\n", thread_from_line);
    return 0;
}

static DWORD WINAPI empty_clipboard_thread(LPVOID arg)
{
    SetLastError( 0xdeadbeef );
    ok(!EmptyClipboard(), "%u: EmptyClipboard succeeded\n", thread_from_line );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
        thread_from_line, GetLastError());
    return 0;
}

static DWORD WINAPI open_and_empty_clipboard_thread(LPVOID arg)
{
    HWND hWnd = arg;
    ok(OpenClipboard(hWnd), "%u: OpenClipboard failed\n", thread_from_line);
    ok(EmptyClipboard(), "%u: EmptyClipboard failed\n", thread_from_line );
    return 0;
}

static DWORD WINAPI open_and_empty_clipboard_win_thread(LPVOID arg)
{
    HWND hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );
    ok(OpenClipboard(hwnd), "%u: OpenClipboard failed\n", thread_from_line);
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
        ok( ret != 0, "%u: SetClipboardData failed err %u\n", thread_from_line, GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = GetClipboardData( CF_WAVE );
        ok( !ret, "%u: GetClipboardData succeeded\n", thread_from_line );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
            thread_from_line, GetLastError());
    }
    else
    {
        SetClipboardData( CF_WAVE, 0 );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
            thread_from_line, GetLastError());
        ok( !IsClipboardFormatAvailable( CF_WAVE ), "%u: SetClipboardData succeeded\n", thread_from_line );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( !ret, "%u: SetClipboardData succeeded\n", thread_from_line );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
            thread_from_line, GetLastError());
    }
    return 0;
}

static void set_clipboard_data_process( int arg )
{
    HANDLE ret;

    SetLastError( 0xdeadbeef );
    if (arg)
    {
        todo_wine_if( arg == 1 || arg == 3 )
        ok( IsClipboardFormatAvailable( CF_WAVE ), "process %u: CF_WAVE not available\n", arg );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "process %u: SetClipboardData failed err %u\n", arg, GetLastError() );
    }
    else
    {
        SetClipboardData( CF_WAVE, 0 );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "process %u: wrong error %u\n",
            arg, GetLastError());
        ok( !IsClipboardFormatAvailable( CF_WAVE ), "process %u: SetClipboardData succeeded\n", arg );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( !ret, "process %u: SetClipboardData succeeded\n", arg );
        ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "process %u: wrong error %u\n",
            arg, GetLastError());
    }
}

static void grab_clipboard_process( int arg )
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = OpenClipboard( 0 );
    ok( ret, "OpenClipboard failed\n" );
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard failed\n" );
    if (arg)
    {
        HANDLE ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "process %u: SetClipboardData failed err %u\n", arg, GetLastError() );
    }
}

static void run_thread( LPTHREAD_START_ROUTINE func, void *arg, int line )
{
    DWORD ret;
    HANDLE thread;

    thread_from_line = line;
    thread = CreateThread(NULL, 0, func, arg, 0, NULL);
    ok(thread != NULL, "%u: CreateThread failed with error %d\n", line, GetLastError());
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
    ok(ret == WAIT_OBJECT_0, "%u: expected WAIT_OBJECT_0, got %u\n", line, ret);
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

    winetest_wait_child_process( info.hProcess );
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
        todo_wine ok( wm_drawclipboard, "didn't receive WM_DRAWCLIPBOARD before WM_DESTROY\n" );
        break;
    case WM_DRAWCLIPBOARD:
        ok( msg_flags == ISMEX_NOSEND, "WM_DRAWCLIPBOARD wrong flags %x\n", msg_flags );
        wm_drawclipboard++;
        break;
    case WM_RENDERALLFORMATS:
        ok( msg_flags == ISMEX_NOSEND, "WM_RENDERALLFORMATS wrong flags %x\n", msg_flags );
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
    ok(hWnd1 != 0, "CreateWindowExA error %d\n", GetLastError());
    trace("hWnd1 = %p\n", hWnd1);

    hWnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd2 != 0, "CreateWindowExA error %d\n", GetLastError());
    trace("hWnd2 = %p\n", hWnd2);

    SetLastError(0xdeadbeef);
    ok(!CloseClipboard(), "CloseClipboard should fail if clipboard wasn't open\n");
    ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN || broken(GetLastError() == 0xdeadbeef), /* wow64 */
       "wrong error %u\n", GetLastError());

    ok(OpenClipboard(0), "OpenClipboard failed\n");
    ok(!GetClipboardOwner(), "clipboard should still be not owned\n");
    ok(!OpenClipboard(hWnd1), "OpenClipboard should fail since clipboard already opened\n");
    ok(OpenClipboard(0), "OpenClipboard again failed\n");
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");
    run_thread( open_clipboard_thread, hWnd1, __LINE__ );
    run_thread( empty_clipboard_thread, 0, __LINE__ );
    run_thread( set_clipboard_data_thread, hWnd1, __LINE__ );
    ok( !IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE available\n" );
    ok( !GetClipboardData( CF_WAVE ), "CF_WAVE data available\n" );
    run_process( "set_clipboard_data 0" );
    ok(!CloseClipboard(), "CloseClipboard should fail if clipboard wasn't open\n");
    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should still be not owned\n");
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should be owned by %p, not by %p\n", hWnd1, GetClipboardOwner());
    run_thread( empty_clipboard_thread, 0, __LINE__ );
    run_thread( set_clipboard_data_thread, hWnd1, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 1" );

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %d\n", GetLastError());

    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should still be owned\n");

    /* any window will do, even from a different process */
    ret = OpenClipboard( GetDesktopWindow() );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ok( GetClipboardOwner() == GetDesktopWindow(), "wrong owner %p/%p\n",
        GetClipboardOwner(), GetDesktopWindow() );
    run_thread( set_clipboard_data_thread, GetDesktopWindow(), __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 2" );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    ret = OpenClipboard( hWnd1 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    SetClipboardData( CF_WAVE, 0 );
    SetClipboardViewer( hWnd1 );
    ok( GetClipboardOwner() == hWnd1, "wrong owner %p/%p\n", GetClipboardOwner(), hWnd1 );
    ok( GetClipboardViewer() == hWnd1, "wrong viewer %p/%p\n", GetClipboardViewer(), hWnd1 );
    ok( GetOpenClipboardWindow() == hWnd1, "wrong open win %p/%p\n", GetOpenClipboardWindow(), hWnd1 );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );

    old_proc = (WNDPROC)SetWindowLongPtrA( hWnd1, GWLP_WNDPROC, (LONG_PTR)winproc_wrapper );
    ret = DestroyWindow(hWnd1);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    ret = DestroyWindow(hWnd2);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should not be owned\n");
    ok(!GetClipboardViewer() && GetLastError() == 0xdeadbeef, "viewer still exists\n");
    ok(!GetOpenClipboardWindow() && GetLastError() == 0xdeadbeef, "clipboard should not be open\n");
    todo_wine ok( !IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE available\n" );

    SetLastError( 0xdeadbeef );
    ret = CloseClipboard();
    ok( !ret, "CloseClipboard succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %u\n", GetLastError() );

    ret = OpenClipboard( 0 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 3" );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    run_thread( open_and_empty_clipboard_thread, 0, __LINE__ );
    ok( !GetOpenClipboardWindow(), "wrong open window %p\n", GetOpenClipboardWindow() );
    ok( !GetClipboardOwner(), "wrong owner window %p\n", GetClipboardOwner() );

    ret = OpenClipboard( 0 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ok( IsClipboardFormatAvailable( CF_WAVE ), "CF_WAVE not available\n" );
    ok( GetClipboardData( CF_WAVE ) != 0, "CF_WAVE data not available\n" );
    run_process( "set_clipboard_data 4" );
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ok( !SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 )),
        "SetClipboardData succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %u\n", GetLastError() );
    ok( !IsClipboardFormatAvailable( CF_WAVE ), "SetClipboardData succeeded\n" );

    run_thread( open_and_empty_clipboard_thread, GetDesktopWindow(), __LINE__ );
    ok( !GetOpenClipboardWindow(), "wrong open window %p\n", GetOpenClipboardWindow() );
    ok( GetClipboardOwner() == GetDesktopWindow(), "wrong owner window %p / %p\n",
        GetClipboardOwner(), GetDesktopWindow() );

    run_thread( open_and_empty_clipboard_win_thread, 0, __LINE__ );
    ok( !GetOpenClipboardWindow(), "wrong open window %p\n", GetOpenClipboardWindow() );
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

    len = GetClipboardFormatNameA(format_id, buf, 256);
    ok(len == lstrlenA("my_cool_clipboard_format"), "wrong format name length %d\n", len);
    ok(!lstrcmpA(buf, "my_cool_clipboard_format"), "wrong format name \"%s\"\n", buf);

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GetAtomNameA((ATOM)format_id, buf, 256);
    ok(len == 0, "GetAtomNameA should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "err %d\n", GetLastError());

todo_wine
{
    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameA((ATOM)format_id, buf, 256);
    ok(len == 0, "GlobalGetAtomNameA should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "err %d\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    atom_id = FindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "FindAtomA should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "err %d\n", GetLastError());

    if (0)
    {
    /* this relies on the clipboard and global atom table being different */
    SetLastError(0xdeadbeef);
    atom_id = GlobalFindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "GlobalFindAtomA should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "err %d\n", GetLastError());
    }

    for (format_id = 0; format_id < 0xffff; format_id++)
    {
        SetLastError(0xdeadbeef);
        len = GetClipboardFormatNameA(format_id, buf, 256);

        if (format_id < 0xc000)
            ok(!len, "GetClipboardFormatNameA should fail, but it returned %d (%s)\n", len, buf);
        else if (len && winetest_debug > 1)
            trace("%04x: %s\n", format_id, len ? buf : "");
    }

    ret = OpenClipboard(0);
    ok( ret, "OpenClipboard error %d\n", GetLastError());

    /* try some invalid/unregistered formats */
    SetLastError( 0xdeadbeef );
    handle = SetClipboardData( 0, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( !handle, "SetClipboardData succeeded\n" );
    ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %u\n", GetLastError());
    handle = SetClipboardData( 0x1234, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %d\n", GetLastError());
    handle = SetClipboardData( 0x123456, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %d\n", GetLastError());
    handle = SetClipboardData( 0xffff8765, GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, 1 ));
    ok( handle != 0, "SetClipboardData failed err %d\n", GetLastError());

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
        len = GetClipboardFormatNameA(format_id, buf, 256);
        trace("%04x: %s\n", format_id, len ? buf : "");
    }

    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ret =CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    if (CountClipboardFormats())
    {
        SetLastError(0xdeadbeef);
        ok(!EnumClipboardFormats(0), "EnumClipboardFormats should fail if clipboard wasn't open\n");
        ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN,
           "Last error should be set to ERROR_CLIPBOARD_NOT_OPEN, not %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ok(!EmptyClipboard(), "EmptyClipboard should fail if clipboard wasn't open\n");
    ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN || broken(GetLastError() == 0xdeadbeef), /* wow64 */
       "Wrong error %u\n", GetLastError());

    format_id = RegisterClipboardFormatA("#1234");
    ok(format_id == 1234, "invalid clipboard format id %04x\n", format_id);
}

static HGLOBAL create_textA(void)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 5);
    char *p = GlobalLock(h);
    strcpy(p, "test");
    GlobalUnlock(h);
    return h;
}

static HGLOBAL create_textW(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 5 * sizeof(WCHAR));
    WCHAR *p = GlobalLock(h);
    lstrcpyW(p, testW);
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

static void test_synthesized(void)
{
    static const struct test
    {
        UINT format;
        UINT expected[8];
        UINT todo;
    } tests[] =
    {
/* 0 */ { CF_TEXT, { CF_TEXT, CF_LOCALE, CF_OEMTEXT, CF_UNICODETEXT }, 1 << 1 },
        { CF_OEMTEXT, { CF_OEMTEXT, CF_LOCALE, CF_TEXT, CF_UNICODETEXT }, 1 << 1 },
        { CF_UNICODETEXT, { CF_UNICODETEXT, CF_LOCALE, CF_TEXT, CF_OEMTEXT }, 1 << 1 },
        { CF_ENHMETAFILE, { CF_ENHMETAFILE, CF_METAFILEPICT }},
        { CF_METAFILEPICT, { CF_METAFILEPICT, CF_ENHMETAFILE }},
/* 5 */ { CF_BITMAP, { CF_BITMAP, CF_DIB, CF_DIBV5 }, 1 << 2 },
        { CF_DIB, { CF_DIB, CF_BITMAP, CF_DIBV5 }, 1 << 2 },
        { CF_DIBV5, { CF_DIBV5, CF_BITMAP, CF_DIB }, (1 << 1) | (1 << 2) },
    };

    HGLOBAL h, htext;
    HENHMETAFILE emf;
    BOOL r;
    UINT cf, i, j, count;
    HANDLE data;

    htext = create_textA();
    emf = create_emf();

    r = OpenClipboard(NULL);
    ok(r, "gle %d\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %d\n", GetLastError());
    h = SetClipboardData(CF_TEXT, htext);
    ok(h == htext, "got %p\n", h);
    h = SetClipboardData(CF_ENHMETAFILE, emf);
    ok(h == emf, "got %p\n", h);
    r = CloseClipboard();
    ok(r, "gle %d\n", GetLastError());

    count = CountClipboardFormats();
    todo_wine ok( count == 6, "count %u\n", count );
    r = IsClipboardFormatAvailable( CF_TEXT );
    ok( r, "CF_TEXT not available err %d\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_LOCALE );
    todo_wine ok( r, "CF_LOCALE not available err %d\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_OEMTEXT );
    ok( r, "CF_OEMTEXT not available err %d\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_UNICODETEXT );
    ok( r, "CF_UNICODETEXT not available err %d\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_ENHMETAFILE );
    ok( r, "CF_ENHMETAFILE not available err %d\n", GetLastError());
    r = IsClipboardFormatAvailable( CF_METAFILEPICT );
    ok( r, "CF_METAFILEPICT not available err %d\n", GetLastError());

    r = OpenClipboard(NULL);
    ok(r, "gle %d\n", GetLastError());
    cf = EnumClipboardFormats(0);
    ok(cf == CF_TEXT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_ENHMETAFILE, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    todo_wine ok(cf == CF_LOCALE, "cf %08x\n", cf);
    if(cf == CF_LOCALE)
    {
        data = GetClipboardData(cf);
        ok(data != NULL, "couldn't get data, cf %08x\n", cf);
        cf = EnumClipboardFormats(cf);
    }
    ok(cf == CF_OEMTEXT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_UNICODETEXT, "cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == CF_METAFILEPICT, "cf %08x\n", cf);
    data = GetClipboardData(cf);
    todo_wine ok(data != NULL, "couldn't get data, cf %08x\n", cf);

    cf = EnumClipboardFormats(cf);
    ok(cf == 0, "cf %08x\n", cf);

    r = EmptyClipboard();
    ok(r, "gle %d\n", GetLastError());

    r = CloseClipboard();
    ok(r, "gle %d\n", GetLastError());

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        r = OpenClipboard(NULL);
        ok(r, "%u: gle %d\n", i, GetLastError());
        r = EmptyClipboard();
        ok(r, "%u: gle %d\n", i, GetLastError());

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
        ok( count == 1, "%u: count %u\n", i, count );

        r = CloseClipboard();
        ok(r, "%u: gle %d\n", i, GetLastError());

        count = CountClipboardFormats();
        for (j = 0; tests[i].expected[j]; j++)
        {
            r = IsClipboardFormatAvailable( tests[i].expected[j] );
            todo_wine_if (tests[i].todo & (1 << j))
            ok( r, "%u: %04x not available\n", i, tests[i].expected[j] );
        }
        todo_wine_if (tests[i].todo)
        ok( count == j, "%u: count %u instead of %u\n", i, count, j );

        r = OpenClipboard(NULL);
        ok(r, "%u: gle %d\n", i, GetLastError());
        cf = 0;
        for (j = 0; tests[i].expected[j]; j++)
        {
            cf = EnumClipboardFormats( cf );
            todo_wine_if (tests[i].todo & (1 << j))
            ok(cf == tests[i].expected[j], "%u.%u: got %04x instead of %04x\n",
               i, j, cf, tests[i].expected[j] );
            if (cf != tests[i].expected[j]) break;
            data = GetClipboardData( cf );
            todo_wine_if (j && cf == CF_METAFILEPICT)
            ok(data != NULL ||
               broken( tests[i].format == CF_DIBV5 && cf == CF_DIB ), /* >= Vista */
               "%u: couldn't get data, cf %04x err %d\n", i, cf, GetLastError());
            if (cf == CF_LOCALE)
            {
                UINT *ptr = GlobalLock( data );
                ok( GlobalSize( data ) == sizeof(*ptr), "%u: size %lu\n", i, GlobalSize( data ));
                ok( *ptr == GetUserDefaultLCID() ||
                    broken( *ptr == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )),
                    "%u: CF_LOCALE %08x/%08x\n", i, *ptr, GetUserDefaultLCID() );
                GlobalUnlock( data );
            }
        }
        if (!tests[i].expected[j])
        {
            cf = EnumClipboardFormats( cf );
            ok(cf == 0, "%u: cf %04x\n", i, cf);
        }

        /* now with delayed rendering */

        r = EmptyClipboard();
        ok(r, "%u: gle %d\n", i, GetLastError());

        SetClipboardData( tests[i].format, 0 );

        count = CountClipboardFormats();
        ok( count == 1, "%u: count %u\n", i, count );

        r = CloseClipboard();
        ok(r, "%u: gle %d\n", i, GetLastError());

        count = CountClipboardFormats();
        for (j = 0; tests[i].expected[j]; j++)
        {
            r = IsClipboardFormatAvailable( tests[i].expected[j] );
            todo_wine_if (tests[i].todo & (1 << j))
            ok( r, "%u: %04x not available\n", i, tests[i].expected[j] );
        }
        todo_wine_if (tests[i].todo)
        ok( count == j, "%u: count %u instead of %u\n", i, count, j );

        r = OpenClipboard(NULL);
        ok(r, "%u: gle %d\n", i, GetLastError());
        cf = 0;
        for (j = 0; tests[i].expected[j]; j++)
        {
            cf = EnumClipboardFormats( cf );
            todo_wine_if (tests[i].todo & (1 << j))
            ok(cf == tests[i].expected[j], "%u.%u: got %04x instead of %04x\n",
               i, j, cf, tests[i].expected[j] );
            if (cf != tests[i].expected[j]) break;
            data = GetClipboardData( cf );
            if (cf == CF_LOCALE)
                ok(data != NULL, "%u: CF_LOCALE no data\n", i);
            else
                ok(!data, "%u: format %04x got data %p\n", i, cf, data);
        }
        if (!tests[i].expected[j])
        {
            cf = EnumClipboardFormats( cf );
            ok(cf == 0, "%u: cf %04x\n", i, cf);
        }
        r = CloseClipboard();
        ok(r, "%u: gle %d\n", i, GetLastError());
    }

    r = OpenClipboard(NULL);
    ok(r, "gle %d\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "gle %d\n", GetLastError());
    r = CloseClipboard();
    ok(r, "gle %d\n", GetLastError());
}

static CRITICAL_SECTION clipboard_cs;
static HWND next_wnd;
static UINT wm_drawclipboard;
static UINT wm_clipboardupdate;
static UINT wm_destroyclipboard;
static UINT wm_renderformat;
static UINT nb_formats;
static BOOL cross_thread;

static LRESULT CALLBACK clipboard_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT ret;
    DWORD msg_flags = InSendMessageEx( NULL );

    switch(msg) {
    case WM_DRAWCLIPBOARD:
        ok( msg_flags == (cross_thread ? ISMEX_NOTIFY : ISMEX_NOSEND),
            "WM_DRAWCLIPBOARD wrong flags %x\n", msg_flags );
        EnterCriticalSection(&clipboard_cs);
        wm_drawclipboard++;
        LeaveCriticalSection(&clipboard_cs);
        break;
    case WM_CHANGECBCHAIN:
        ok( msg_flags == (cross_thread ? ISMEX_SEND : ISMEX_NOSEND),
            "WM_CHANGECBCHAIN wrong flags %x\n", msg_flags );
        if (next_wnd == (HWND)wp)
            next_wnd = (HWND)lp;
        else if (next_wnd)
            SendMessageA(next_wnd, msg, wp, lp);
        break;
    case WM_DESTROYCLIPBOARD:
        ok( msg_flags == (cross_thread ? ISMEX_SEND : ISMEX_NOSEND),
            "WM_DESTROYCLIPBOARD wrong flags %x\n", msg_flags );
        wm_destroyclipboard++;
        ok( GetClipboardOwner() == hwnd, "WM_DESTROYCLIPBOARD owner %p\n", GetClipboardOwner() );
        nb_formats = CountClipboardFormats();
        break;
    case WM_RENDERFORMAT:
        ok( !wm_renderformat, "multiple WM_RENDERFORMAT %04x / %04lx\n", wm_renderformat, wp );
        wm_renderformat = wp;
        break;
    case WM_CLIPBOARDUPDATE:
        ok( msg_flags == ISMEX_NOSEND, "WM_CLIPBOARDUPDATE wrong flags %x\n", msg_flags );
        EnterCriticalSection(&clipboard_cs);
        wm_clipboardupdate++;
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

static DWORD WINAPI clipboard_thread(void *param)
{
    HWND ret, win = param;
    BOOL r;
    MSG msg;
    HANDLE handle;
    UINT count, fmt, formats, old_seq = 0, seq;

    cross_thread = (GetWindowThreadProcessId( win, NULL ) != GetCurrentThreadId());
    trace( "%s-threaded test\n", cross_thread ? "multi" : "single" );

    if (pGetClipboardSequenceNumber) old_seq = pGetClipboardSequenceNumber();

    EnterCriticalSection(&clipboard_cs);
    SetLastError(0xdeadbeef);
    next_wnd = SetClipboardViewer(win);
    ok(GetLastError() == 0xdeadbeef, "GetLastError = %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    SetLastError( 0xdeadbeef );
    ret = SetClipboardViewer( (HWND)0xdead );
    ok( !ret, "SetClipboardViewer succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    r = ChangeClipboardChain( win, (HWND)0xdead );
    ok( !r, "ChangeClipboardChain succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    r = ChangeClipboardChain( (HWND)0xdead, next_wnd );
    ok( !r, "ChangeClipboardChain succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );

    if (pAddClipboardFormatListener)
    {
        r = pAddClipboardFormatListener(win);
        ok( r, "AddClipboardFormatListener failed err %d\n", GetLastError());
        SetLastError( 0xdeadbeef );
        r = pAddClipboardFormatListener( win );
        ok( !r, "AddClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        r = pAddClipboardFormatListener( (HWND)0xdead );
        ok( !r, "AddClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );
        r = pAddClipboardFormatListener( GetDesktopWindow() );
        ok( r, "AddClipboardFormatListener failed err %d\n", GetLastError());
        r = pRemoveClipboardFormatListener( GetDesktopWindow() );
        ok( r, "RemoveClipboardFormatListener failed err %d\n", GetLastError());
    }

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER + 1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    SetLastError( 0xdeadbeef );
    r = OpenClipboard( (HWND)0xdead );
    ok( !r, "OpenClipboard succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( !count, "WM_DESTROYCLIPBOARD received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());
    /* sequence changes again, even though it was already empty */
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( count, "WM_DESTROYCLIPBOARD not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );
    count = SendMessageA( win, WM_USER+5, 0, 0 );
    ok( !count, "wrong format count %u on WM_DESTROYCLIPBOARD\n", count );

    handle = SetClipboardData( CF_TEXT, create_textA() );
    ok(handle != 0, "SetClipboardData failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    SetClipboardData( CF_UNICODETEXT, 0 );

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    SetClipboardData( CF_UNICODETEXT, 0 );  /* same data again */

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    ok( IsClipboardFormatAvailable( CF_TEXT ), "CF_TEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_UNICODETEXT ), "CF_UNICODETEXT available\n" );
    ok( !IsClipboardFormatAvailable( CF_OEMTEXT ), "CF_OEMTEXT available\n" );

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    ok( IsClipboardFormatAvailable( CF_TEXT ), "CF_TEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_UNICODETEXT ), "CF_UNICODETEXT available\n" );
    ok( IsClipboardFormatAvailable( CF_OEMTEXT ), "CF_OEMTEXT available\n" );

    ok( GetClipboardOwner() == win, "wrong owner %p\n", GetClipboardOwner());
    handle = GetClipboardData( CF_UNICODETEXT );
    ok( !handle, "got data for CF_UNICODETEXT\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( fmt == CF_UNICODETEXT, "WM_RENDERFORMAT received %04x\n", fmt );

    handle = GetClipboardData( CF_OEMTEXT );
    ok( !handle, "got data for CF_OEMTEXT\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( fmt == CF_UNICODETEXT, "WM_RENDERFORMAT received %04x\n", fmt );

    SetClipboardData( CF_WAVE, 0 );
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );

    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    if (pGetClipboardSequenceNumber)
    {
        /* no synthesized format, so CloseClipboard doesn't change the sequence */
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( seq == old_seq, "sequence changed\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    /* nothing changed */
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( !count, "WM_DESTROYCLIPBOARD received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );

    formats = CountClipboardFormats();
    r = OpenClipboard(0);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());

    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( count == 1, "WM_DESTROYCLIPBOARD not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );
    count = SendMessageA( win, WM_USER+5, 0, 0 );
    ok( count == formats, "wrong format count %u on WM_DESTROYCLIPBOARD\n", count );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( !count, "WM_DESTROYCLIPBOARD received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received %04x\n", fmt );

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( seq == old_seq, "sequence changed\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    run_process( "grab_clipboard 0" );

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        /* in this case we get a cross-thread WM_DRAWCLIPBOARD */
        cross_thread = TRUE;
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        cross_thread = FALSE;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    r = OpenClipboard(0);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( seq == old_seq, "sequence changed\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    run_process( "grab_clipboard 1" );

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        /* in this case we get a cross-thread WM_DRAWCLIPBOARD */
        cross_thread = TRUE;
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
        cross_thread = FALSE;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    r = OpenClipboard(0);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_FIXED, 1 ));
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( !wm_drawclipboard, "WM_DRAWCLIPBOARD received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( seq == old_seq, "sequence changed\n" );
        old_seq = seq;
    }
    if (!cross_thread)
    {
        ok( wm_drawclipboard == 1, "WM_DRAWCLIPBOARD not received\n" );
        ok( !wm_clipboardupdate, "WM_CLIPBOARDUPDATE received\n" );
        ok( !wm_renderformat, "WM_RENDERFORMAT received\n" );
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageW( &msg );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count == 1, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( count == 1 || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );
    fmt = SendMessageA( win, WM_USER+4, 0, 0 );
    ok( !fmt, "WM_RENDERFORMAT received\n" );

    r = PostMessageA(win, WM_USER, 0, 0);
    ok(r, "PostMessage failed: %d\n", GetLastError());

    if (pRemoveClipboardFormatListener)
    {
        r = pRemoveClipboardFormatListener(win);
        ok( r, "RemoveClipboardFormatListener failed err %d\n", GetLastError());
        SetLastError( 0xdeadbeef );
        r = pRemoveClipboardFormatListener(win);
        ok( !r, "RemoveClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        r = pRemoveClipboardFormatListener( (HWND)0xdead );
        ok( !r, "RemoveClipboardFormatListener succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %u\n", GetLastError() );
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

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = clipboard_wnd_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = "clipboard_test";
    RegisterClassA(&cls);

    win = CreateWindowA("clipboard_test", NULL, 0, 0, 0, 0, 0, NULL, 0, NULL, 0);
    ok(win != NULL, "CreateWindow failed: %d\n", GetLastError());

    thread = CreateThread(NULL, 0, clipboard_thread, (void*)win, 0, &tid);
    ok(thread != NULL, "CreateThread failed: %d\n", GetLastError());

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
    ok( win != NULL, "CreateWindow failed: %d\n", GetLastError() );

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
    void *ptr = GlobalLock( handle );
    if (ptr) GlobalUnlock( handle );
    return !ptr;
}

static UINT format_id;
static HBITMAP bitmap, bitmap2;
static HPALETTE palette;
static HPEN pen;
static const LOGPALETTE logpalette = { 0x300, 1 };

static void test_handles( HWND hwnd )
{
    HGLOBAL h, htext, htext2;
    BOOL r;
    HANDLE data;
    DWORD process;
    BOOL is_owner = (GetWindowThreadProcessId( hwnd, &process ) && process == GetCurrentProcessId());

    trace( "hwnd %p\n", hwnd );
    htext = create_textA();
    htext2 = create_textA();
    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );
    bitmap2 = CreateBitmap( 10, 10, 1, 1, NULL );
    palette = CreatePalette( &logpalette );
    pen = CreatePen( PS_SOLID, 1, 0 );

    r = OpenClipboard( hwnd );
    ok( r, "gle %d\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %d\n", GetLastError() );

    h = SetClipboardData( CF_TEXT, htext );
    ok( h == htext, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( format_id, htext2 );
    ok( h == htext2, "got %p\n", h );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( h == bitmap, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_PALETTE, palette );
    ok( h == palette, "got %p\n", h );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    /* setting custom GDI formats crashes on 64-bit Windows */
    if (!is_win64)
    {
        h = SetClipboardData( CF_GDIOBJFIRST + 1, bitmap2 );
        ok( h == bitmap2, "got %p\n", h );
        ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
        h = SetClipboardData( CF_GDIOBJFIRST + 2, pen );
        ok( h == pen, "got %p\n", h );
        ok( GetObjectType( h ) == OBJ_PEN, "expected pen %p\n", h );
    }

    data = GetClipboardData( CF_TEXT );
    ok( data == htext, "wrong data %p\n", data );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    data = GetClipboardData( format_id );
    ok( data == htext2, "wrong data %p, cf %08x\n", data, format_id );
    ok( is_moveable( data ), "expected moveable mem %p\n", data );

    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );

    /* data handles are still valid */
    ok( is_moveable( htext ), "expected moveable mem %p\n", htext );
    ok( is_moveable( htext2 ), "expected moveable mem %p\n", htext );
    ok( GetObjectType( bitmap ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap );
    ok( GetObjectType( bitmap2 ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap2 );
    ok( GetObjectType( palette ) == OBJ_PAL, "expected palette %p\n", palette );
    ok( GetObjectType( pen ) == OBJ_PEN, "expected pen %p\n", pen );

    r = OpenClipboard( hwnd );
    ok( r, "gle %d\n", GetLastError() );

    /* and now they are freed, unless we are the owner */
    if (!is_owner)
    {
        todo_wine ok( is_freed( htext ), "expected freed mem %p\n", htext );
        todo_wine ok( is_freed( htext2 ), "expected freed mem %p\n", htext );

        data = GetClipboardData( CF_TEXT );
        todo_wine ok( is_fixed( data ), "expected fixed mem %p\n", data );

        data = GetClipboardData( format_id );
        todo_wine ok( is_fixed( data ), "expected fixed mem %p\n", data );
    }
    else
    {
        ok( is_moveable( htext ), "expected moveable mem %p\n", htext );
        ok( is_moveable( htext2 ), "expected moveable mem %p\n", htext );

        data = GetClipboardData( CF_TEXT );
        ok( data == htext, "wrong data %p\n", data );

        data = GetClipboardData( format_id );
        ok( data == htext2, "wrong data %p, cf %08x\n", data, format_id );
    }

    data = GetClipboardData( CF_OEMTEXT );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_UNICODETEXT );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_BITMAP );
    ok( data == bitmap, "expected bitmap %p\n", data );
    data = GetClipboardData( CF_PALETTE );
    ok( data == palette, "expected palette %p\n", data );
    if (!is_win64)
    {
        data = GetClipboardData( CF_GDIOBJFIRST + 1 );
        ok( data == bitmap2, "expected bitmap2 %p\n", data );
        data = GetClipboardData( CF_GDIOBJFIRST + 2 );
        ok( data == pen, "expected pen %p\n", data );
    }
    data = GetClipboardData( CF_DIB );
    ok( is_fixed( data ), "expected fixed mem %p\n", data );
    data = GetClipboardData( CF_DIBV5 );
    todo_wine ok( is_fixed( data ), "expected fixed mem %p\n", data );

    ok( GetObjectType( bitmap ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap );
    ok( GetObjectType( bitmap2 ) == OBJ_BITMAP, "expected bitmap %p\n", bitmap2 );
    ok( GetObjectType( palette ) == OBJ_PAL, "expected palette %p\n", palette );
    ok( GetObjectType( pen ) == OBJ_PEN, "expected pen %p\n", pen );

    r = EmptyClipboard();
    ok( r, "gle %d\n", GetLastError() );

    /* w2003, w2008 don't seem to free the data here */
    ok( is_freed( htext ) || broken( !is_freed( htext )), "expected freed mem %p\n", htext );
    ok( is_freed( htext2 ) || broken( !is_freed( htext2 )), "expected freed mem %p\n", htext );
    ok( !GetObjectType( bitmap ), "expected freed handle %p\n", bitmap );
    ok( !GetObjectType( palette ), "expected freed handle %p\n", palette );
    ok( GetObjectType( bitmap2 ) == OBJ_BITMAP, "expected bitmap2 %p\n", bitmap2 );
    ok( GetObjectType( pen ) == OBJ_PEN, "expected pen %p\n", pen );

    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );
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

    r = OpenClipboard( 0 );
    ok( r, "gle %d\n", GetLastError() );
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
    h = GetClipboardData( CF_BITMAP );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    ok( h == bitmap, "different bitmap %p / %p\n", h, bitmap );
    trace( "bitmap %p\n", h );
    h = GetClipboardData( CF_PALETTE );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    ok( h == palette, "different palette %p / %p\n", h, palette );
    trace( "palette %p\n", h );
    if (!is_win64)
    {
        h = GetClipboardData( CF_GDIOBJFIRST + 1 );
        ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
        ok( h == bitmap2, "different bitmap %p / %p\n", h, bitmap2 );
        trace( "bitmap2 %p\n", h );
        h = GetClipboardData( CF_GDIOBJFIRST + 2 );
        ok( GetObjectType( h ) == OBJ_PEN, "expected pen %p\n", h );
        ok( h == pen, "different pen %p / %p\n", h, pen );
        trace( "pen %p\n", h );
    }
    h = GetClipboardData( CF_DIB );
    ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = GetClipboardData( CF_DIBV5 );
    todo_wine ok( is_fixed( h ), "expected fixed mem %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );
    return 0;
}

static void test_handles_process( const char *str )
{
    BOOL r;
    HANDLE h;
    char *ptr;

    format_id = RegisterClipboardFormatA( "my_cool_clipboard_format" );
    r = OpenClipboard( 0 );
    ok( r, "gle %d\n", GetLastError() );
    h = GetClipboardData( CF_TEXT );
    todo_wine_if( !h ) ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) todo_wine ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( format_id );
    todo_wine ok( is_fixed( h ), "expected fixed mem %p\n", h );
    ptr = GlobalLock( h );
    if (ptr) ok( !strcmp( str, ptr ), "wrong data '%.5s'\n", ptr );
    GlobalUnlock( h );
    h = GetClipboardData( CF_BITMAP );
    todo_wine ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    trace( "bitmap %p\n", h );
    h = GetClipboardData( CF_PALETTE );
    todo_wine ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    trace( "palette %p\n", h );
    h = GetClipboardData( CF_GDIOBJFIRST + 1 );
    ok( !GetObjectType( h ), "expected invalid %p\n", h );
    trace( "bitmap2 %p\n", h );
    h = GetClipboardData( CF_GDIOBJFIRST + 2 );
    ok( !GetObjectType( h ), "expected invalid %p\n", h );
    trace( "pen %p\n", h );
    h = GetClipboardData( CF_DIB );
    todo_wine ok( is_fixed( h ), "expected fixed mem %p\n", h );
    h = GetClipboardData( CF_DIBV5 );
    todo_wine ok( is_fixed( h ), "expected fixed mem %p\n", h );
    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );
}

static void test_data_handles(void)
{
    BOOL r;
    HANDLE h;
    HWND hwnd = CreateWindowA( "static", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL );

    ok( hwnd != 0, "window creation failed\n" );
    format_id = RegisterClipboardFormatA( "my_cool_clipboard_format" );
    test_handles( 0 );
    test_handles( GetDesktopWindow() );
    test_handles( hwnd );
    run_thread( test_handles_thread, hwnd, __LINE__ );

    bitmap = CreateBitmap( 10, 10, 1, 1, NULL );
    bitmap2 = CreateBitmap( 10, 10, 1, 1, NULL );
    palette = CreatePalette( &logpalette );
    pen = CreatePen( PS_SOLID, 1, 0 );

    r = OpenClipboard( hwnd );
    ok( r, "gle %d\n", GetLastError() );
    r = EmptyClipboard();
    ok( r, "gle %d\n", GetLastError() );
    h = SetClipboardData( CF_TEXT, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( format_id, create_textA() );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = SetClipboardData( CF_BITMAP, bitmap );
    ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
    h = SetClipboardData( CF_PALETTE, palette );
    ok( GetObjectType( h ) == OBJ_PAL, "expected palette %p\n", h );
    if (!is_win64)
    {
        h = SetClipboardData( CF_GDIOBJFIRST + 1, bitmap2 );
        ok( GetObjectType( h ) == OBJ_BITMAP, "expected bitmap %p\n", h );
        h = SetClipboardData( CF_GDIOBJFIRST + 2, pen );
        ok( GetObjectType( h ) == OBJ_PEN, "expected pen %p\n", h );
    }
    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );

    run_thread( test_handles_thread2, 0, __LINE__ );
    run_process( "handles test" );

    r = OpenClipboard( hwnd );
    ok( r, "gle %d\n", GetLastError() );
    h = GetClipboardData( CF_TEXT );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    h = GetClipboardData( format_id );
    ok( is_moveable( h ), "expected moveable mem %p\n", h );
    r = EmptyClipboard();
    ok( r, "gle %d\n", GetLastError() );
    r = CloseClipboard();
    ok( r, "gle %d\n", GetLastError() );

    DestroyWindow( hwnd );
}

START_TEST(clipboard)
{
    char **argv;
    int argc = winetest_get_mainargs( &argv );
    HMODULE mod = GetModuleHandleA( "user32" );

    argv0 = argv[0];
    pAddClipboardFormatListener = (void *)GetProcAddress( mod, "AddClipboardFormatListener" );
    pRemoveClipboardFormatListener = (void *)GetProcAddress( mod, "RemoveClipboardFormatListener" );
    pGetClipboardSequenceNumber = (void *)GetProcAddress( mod, "GetClipboardSequenceNumber" );

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

    test_RegisterClipboardFormatA();
    test_ClipboardOwner();
    test_synthesized();
    test_messages();
    test_data_handles();
}
