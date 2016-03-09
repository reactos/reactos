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

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"

#define WM_CLIPBOARDUPDATE 0x031D

static BOOL (WINAPI *pAddClipboardFormatListener)(HWND hwnd);
static DWORD (WINAPI *pGetClipboardSequenceNumber)(void);

static int thread_from_line;

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

static DWORD WINAPI set_clipboard_data_thread(LPVOID arg)
{
    HWND hwnd = arg;
    HANDLE ret;

    SetLastError( 0xdeadbeef );
    if (GetClipboardOwner() == hwnd)
    {
        SetClipboardData( CF_WAVE, 0 );
        todo_wine ok( IsClipboardFormatAvailable( CF_WAVE ), "%u: SetClipboardData failed\n", thread_from_line );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        ok( ret != 0, "%u: SetClipboardData failed err %u\n", thread_from_line, GetLastError() );
    }
    else
    {
        SetClipboardData( CF_WAVE, 0 );
        todo_wine ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
                      thread_from_line, GetLastError());
        ok( !IsClipboardFormatAvailable( CF_WAVE ), "%u: SetClipboardData succeeded\n", thread_from_line );
        ret = SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 ));
        todo_wine ok( !ret, "%u: SetClipboardData succeeded\n", thread_from_line );
        todo_wine ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "%u: wrong error %u\n",
                      thread_from_line, GetLastError());
    }
    return 0;
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
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    ret = OpenClipboard( hWnd1 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ok( GetClipboardOwner() == hWnd1, "wrong owner %p/%p\n", GetClipboardOwner(), hWnd1 );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    ret = DestroyWindow(hWnd1);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    ret = DestroyWindow(hWnd2);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should not be owned\n");

    ret = OpenClipboard( 0 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    run_thread( open_and_empty_clipboard_thread, 0, __LINE__ );

    ret = OpenClipboard( 0 );
    ok( ret, "OpenClipboard error %d\n", GetLastError());
    run_thread( set_clipboard_data_thread, 0, __LINE__ );
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    todo_wine ok( !SetClipboardData( CF_WAVE, GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, 100 )),
                  "SetClipboardData succeeded\n" );
    todo_wine ok( GetLastError() == ERROR_CLIPBOARD_NOT_OPEN, "wrong error %u\n", GetLastError() );
    todo_wine ok( !IsClipboardFormatAvailable( CF_WAVE ), "SetClipboardData succeeded\n" );

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

static HGLOBAL create_text(void)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, 5);
    char *p = GlobalLock(h);
    strcpy(p, "test");
    GlobalUnlock(h);
    return h;
}

static HENHMETAFILE create_emf(void)
{
    const RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFileA(NULL, NULL, &rect, "HENHMETAFILE Ole Clipboard Test\0Test\0\0");
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static void test_synthesized(void)
{
    HGLOBAL h, htext;
    HENHMETAFILE emf;
    BOOL r;
    UINT cf;
    HANDLE data;

    htext = create_text();
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
        cf = EnumClipboardFormats(cf);
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
}

static CRITICAL_SECTION clipboard_cs;
static HWND next_wnd;
static LRESULT CALLBACK clipboard_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static UINT wm_drawclipboard;
    static UINT wm_clipboardupdate;
    static UINT wm_destroyclipboard;
    LRESULT ret;

    switch(msg) {
    case WM_DRAWCLIPBOARD:
        EnterCriticalSection(&clipboard_cs);
        wm_drawclipboard++;
        LeaveCriticalSection(&clipboard_cs);
        break;
    case WM_CHANGECBCHAIN:
        if (next_wnd == (HWND)wp)
            next_wnd = (HWND)lp;
        else if (next_wnd)
            SendMessageA(next_wnd, msg, wp, lp);
        break;
    case WM_DESTROYCLIPBOARD:
        wm_destroyclipboard++;
        ok( GetClipboardOwner() == hwnd, "WM_DESTROYCLIPBOARD owner %p\n", GetClipboardOwner() );
        break;
    case WM_CLIPBOARDUPDATE:
        wm_clipboardupdate++;
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
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI clipboard_thread(void *param)
{
    HWND win = param;
    BOOL r;
    HANDLE handle;
    UINT count, old_seq = 0, seq;

    if (pGetClipboardSequenceNumber) old_seq = pGetClipboardSequenceNumber();

    EnterCriticalSection(&clipboard_cs);
    SetLastError(0xdeadbeef);
    next_wnd = SetClipboardViewer(win);
    ok(GetLastError() == 0xdeadbeef, "GetLastError = %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);
    if (pAddClipboardFormatListener)
    {
        r = pAddClipboardFormatListener(win);
        ok( r, "AddClipboardFormatListener failed err %d\n", GetLastError());
    }

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    count = SendMessageA( win, WM_USER + 1, 0, 0 );
    ok( count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( !count, "WM_DESTROYCLIPBOARD received\n" );

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());
    /* sequence changes again, even though it was already empty */
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );
    count = SendMessageA( win, WM_USER+3, 0, 0 );
    ok( count, "WM_DESTROYCLIPBOARD not received\n" );

    handle = SetClipboardData( CF_TEXT, create_text() );
    ok(handle != 0, "SetClipboardData failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    SetClipboardData( CF_UNICODETEXT, 0 );

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    SetClipboardData( CF_UNICODETEXT, 0 );  /* same data again */

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

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
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    todo_wine ok( count || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());

    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        ok( seq == old_seq, "sequence changed\n" );
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    SetClipboardData( CF_WAVE, 0 );
    if (pGetClipboardSequenceNumber)
    {
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( (int)(seq - old_seq) > 0, "sequence unchanged\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    if (pGetClipboardSequenceNumber)
    {
        /* no synthesized format, so CloseClipboard doesn't change the sequence */
        seq = pGetClipboardSequenceNumber();
        todo_wine ok( seq == old_seq, "sequence changed\n" );
        old_seq = seq;
    }
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( count, "WM_DRAWCLIPBOARD not received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    todo_wine ok( count || broken(!pAddClipboardFormatListener), "WM_CLIPBOARDUPDATE not received\n" );

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
    count = SendMessageA( win, WM_USER+1, 0, 0 );
    ok( !count, "WM_DRAWCLIPBOARD received\n" );
    count = SendMessageA( win, WM_USER+2, 0, 0 );
    ok( !count, "WM_CLIPBOARDUPDATE received\n" );

    r = OpenClipboard(0);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());
    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());

    r = PostMessageA(win, WM_USER, 0, 0);
    ok(r, "PostMessage failed: %d\n", GetLastError());
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

    while(GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    UnregisterClassA("clipboard_test", GetModuleHandleA(NULL));
    DeleteCriticalSection(&clipboard_cs);
}

START_TEST(clipboard)
{
    HMODULE mod = GetModuleHandleA( "user32" );

    pAddClipboardFormatListener = (void *)GetProcAddress( mod, "AddClipboardFormatListener" );
    pGetClipboardSequenceNumber = (void *)GetProcAddress( mod, "GetClipboardSequenceNumber" );

    test_RegisterClipboardFormatA();
    test_ClipboardOwner();
    test_synthesized();
    test_messages();
}
