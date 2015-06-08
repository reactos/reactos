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

static BOOL is_win9x = FALSE;

#define test_last_error(expected_error) \
    do \
    { \
        if (!is_win9x) \
            ok(GetLastError() == expected_error, \
               "Last error should be set to %d, not %d\n", \
                expected_error, GetLastError()); \
    } while (0)

static DWORD WINAPI open_clipboard_thread(LPVOID arg)
{
    HWND hWnd = arg;
    ok(OpenClipboard(hWnd), "OpenClipboard second time in the same hwnd failed\n");
    return 0;
}

static void test_ClipboardOwner(void)
{
    HANDLE thread;
    HWND hWnd1, hWnd2;
    DWORD dwret;
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
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());

    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");
    thread = CreateThread(NULL, 0, open_clipboard_thread, hWnd1, 0, NULL);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    dwret = WaitForSingleObject(thread, 1000);
    ok(dwret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %u\n", dwret);
    CloseHandle(thread);
    ok(OpenClipboard(hWnd1), "OpenClipboard second time in the same hwnd failed\n");

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should still be not owned\n");
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %d\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should be owned by %p, not by %p\n", hWnd1, GetClipboardOwner());

    SetLastError(0xdeadbeef);
    ret = OpenClipboard(hWnd2);
    ok(!ret && (GetLastError() == 0xdeadbeef || GetLastError() == ERROR_ACCESS_DENIED),
       "OpenClipboard should fail without setting last error value, or with ERROR_ACCESS_DENIED, got error %d\n", GetLastError());

    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %d\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should still be owned\n");

    ret = DestroyWindow(hWnd1);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    ret = DestroyWindow(hWnd2);
    ok( ret, "DestroyWindow error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should not be owned\n");
}

static void test_RegisterClipboardFormatA(void)
{
    ATOM atom_id;
    UINT format_id, format_id2;
    char buf[256];
    int len;
    BOOL ret;

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
    test_last_error(ERROR_INVALID_HANDLE);

todo_wine
{
    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameA((ATOM)format_id, buf, 256);
    ok(len == 0, "GlobalGetAtomNameA should fail\n");
    test_last_error(ERROR_INVALID_HANDLE);
}

    SetLastError(0xdeadbeef);
    atom_id = FindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "FindAtomA should fail\n");
    test_last_error(ERROR_FILE_NOT_FOUND);

    if (0)
    {
    /* this relies on the clipboard and global atom table being different */
    SetLastError(0xdeadbeef);
    atom_id = GlobalFindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "GlobalFindAtomA should fail\n");
    test_last_error(ERROR_FILE_NOT_FOUND);
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
    ok(cf == CF_UNICODETEXT ||
       broken(cf == CF_METAFILEPICT), /* win9x and winME has no CF_UNICODETEXT */
       "cf %08x\n", cf);

    if(cf == CF_UNICODETEXT)
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
    switch(msg) {
    case WM_DRAWCLIPBOARD:
        EnterCriticalSection(&clipboard_cs);
        LeaveCriticalSection(&clipboard_cs);
        break;
    case WM_CHANGECBCHAIN:
        if (next_wnd == (HWND)wp)
            next_wnd = (HWND)lp;
        else if (next_wnd)
            SendMessageA(next_wnd, msg, wp, lp);
        break;
    case WM_USER:
        ChangeClipboardChain(hwnd, next_wnd);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI clipboard_thread(void *param)
{
    HWND win = param;
    BOOL r;

    EnterCriticalSection(&clipboard_cs);
    SetLastError(0xdeadbeef);
    next_wnd = SetClipboardViewer(win);
    ok(GetLastError() == 0xdeadbeef, "GetLastError = %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

    r = OpenClipboard(win);
    ok(r, "OpenClipboard failed: %d\n", GetLastError());

    r = EmptyClipboard();
    ok(r, "EmptyClipboard failed: %d\n", GetLastError());

    EnterCriticalSection(&clipboard_cs);
    r = CloseClipboard();
    ok(r, "CloseClipboard failed: %d\n", GetLastError());
    LeaveCriticalSection(&clipboard_cs);

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
    SetLastError(0xdeadbeef);
    FindAtomW(NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) is_win9x = TRUE;

    test_RegisterClipboardFormatA();
    test_ClipboardOwner();
    test_synthesized();
    test_messages();
}
