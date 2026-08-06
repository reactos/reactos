/* Unit tests for subclassed windows.
 *
 * Copyright 2004 Kevin Koltzau
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/test.h"

static BOOL (WINAPI *pGetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR *);
static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);

#define IS_WNDPROC_HANDLE(x) (((ULONG_PTR)(x) >> 16) == (~0u >> 16))

#define SEND_NEST   0x01
#define DELETE_SELF 0x02
#define DELETE_PREV 0x04
#define EXPECT_UNICODE    0x10
#define EXPECT_WNDPROC_1  0x20
#define EXPECT_WNDPROC_3  0x40

struct message {
    int procnum;           /* WndProc id message is expected from */
    WPARAM wParam;         /* expected value of wParam */
};

static int sequence_cnt, sequence_size;
static struct message* sequence;

static const struct message Sub_BasicTest[] = {
    { 2, 1 },
    { 1, 1 },
    { 2, 2 },
    { 1, 2 },
    { 0 }
};

static const struct message Sub_DeletedTest[] = {
    { 2, 1 },
    { 1, 1 },
    { 0 }
};

static const struct message Sub_AfterDeletedTest[] = {
    { 1, 1 },
    { 0 }
};

static const struct message Sub_OldAfterNewTest[] = {
    { 3, 1 },
    { 2, 1 },
    { 1, 1 },
    { 3, 2 },
    { 2, 2 },
    { 1, 2 },
    { 0 }
};

static const struct message Sub_MixTest[] = {
    { 3, 1 },
    { 4, 1 },
    { 2, 1 },
    { 1, 1 },
    { 0 }
};

static const struct message Sub_MixAndNestTest[] = {
    { 3, 1 },
    { 4, 1 },
    { 3, 2 },
    { 4, 2 },
    { 2, 2 },
    { 1, 2 },
    { 2, 1 },
    { 1, 1 },
    { 0 }
};

static const struct message Sub_MixNestDelTest[] = {
    { 3, 1 },
    { 4, 1 },
    { 3, 2 },
    { 2, 2 },
    { 1, 2 },
    { 2, 1 },
    { 1, 1 },
    { 0 }
};

static const struct message Sub_MixDelPrevTest[] = {
    { 3, 1 },
    { 5, 1 },
    { 2, 1 },
    { 1, 1 },
    { 0 }
};

static void add_message(const struct message *msg)
{
    if (!sequence)
    {
        sequence_size = 10;
        sequence = malloc(sequence_size * sizeof(struct message));
    }
    if (sequence_cnt == sequence_size)
    {
        sequence_size *= 2;
        sequence = realloc(sequence, sequence_size * sizeof(struct message));
    }
    assert(sequence);

    sequence[sequence_cnt].wParam  = msg->wParam;
    sequence[sequence_cnt].procnum = msg->procnum;

    sequence_cnt++;
}

static void flush_sequence(void)
{
    free(sequence);
    sequence = NULL;
    sequence_cnt = sequence_size = 0;
}

static void ok_sequence(const struct message *expected, const char *context)
{
    static const struct message end_of_sequence = { 0, 0 };
    const struct message *actual;

    add_message(&end_of_sequence);

    actual = sequence;

    while(expected->procnum && actual->procnum)
    {
        ok(expected->procnum == actual->procnum,
            "%s: the procnum %d was expected, but got procnum %d instead\n",
            context, expected->procnum, actual->procnum);
        ok(expected->wParam == actual->wParam,
            "%s: in procnum %d expecting wParam 0x%Ix got 0x%Ix\n",
            context, expected->procnum, expected->wParam, actual->wParam);
        expected++;
        actual++;
    }
    ok(!expected->procnum, "Received fewer messages than expected\n");
    ok(!actual->procnum, "Received more messages than expected\n");
    flush_sequence();
}

static LRESULT WINAPI wnd_proc_1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI wnd_proc_3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define check_unicode(a, b) check_unicode_(__LINE__, a, b)
static void check_unicode_(int line, HWND hwnd, DWORD flags)
{
    WNDPROC proc;
    BOOL ret;

    ret = IsWindowUnicode(hwnd);
    ok_(__FILE__, line)(ret == !!(flags & EXPECT_UNICODE), "IsWindowUnicode returned %u\n", ret);

    proc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    ok_(__FILE__, line)(IS_WNDPROC_HANDLE(proc) == !(flags & EXPECT_UNICODE), "got proc %p\n", proc);

    proc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    if (flags & EXPECT_UNICODE)
        ok_(__FILE__, line)(IS_WNDPROC_HANDLE(proc), "got proc %p\n", proc);
    else if (flags & EXPECT_WNDPROC_1)
        ok_(__FILE__, line)(proc == wnd_proc_1, "got proc %p\n", proc);
    else if (flags & EXPECT_WNDPROC_3)
        ok_(__FILE__, line)(proc == wnd_proc_3, "got proc %p\n", proc);
    else
        ok_(__FILE__, line)(!IS_WNDPROC_HANDLE(proc), "got proc %p\n", proc);
}

static LRESULT WINAPI wnd_proc_1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD flags = GetWindowLongA(hwnd, GWLP_USERDATA);
    struct message msg;

    check_unicode(hwnd, flags);
    
    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = 1;
        add_message(&msg);
    }
    if (message == WM_CHAR) {
        ok(!(wParam & ~0xff), "got wParam %#Ix\n", wParam);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}


static WNDPROC orig_proc_3;
static LRESULT WINAPI wnd_proc_3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg;
    
    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = 3;
        add_message(&msg);
    }
    return CallWindowProcA(orig_proc_3, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI wnd_proc_sub(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uldSubclass, DWORD_PTR dwRefData)
{
    DWORD flags = GetWindowLongA(hwnd, GWLP_USERDATA);
    struct message msg;

    check_unicode(hwnd, flags);
    
    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = uldSubclass;
        add_message(&msg);
        
        if(lParam) {
            if(dwRefData & DELETE_SELF) {
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass);
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass);
                check_unicode(hwnd, flags);
            }
            if(dwRefData & DELETE_PREV)
            {
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass-1);
                check_unicode(hwnd, flags);
            }
            if(dwRefData & SEND_NEST)
            {
                SendMessageA(hwnd, WM_USER, wParam+1, 0);
                check_unicode(hwnd, flags);
            }
        }
    }
    if (message == WM_CHAR) {
        ok(wParam == 0x30c2, "got wParam %#Ix\n", wParam);
    }
    return pDefSubclassProc(hwnd, message, wParam, lParam);
}

static void test_subclass(void)
{
    BOOL ret;
    HWND hwnd = CreateWindowExA(0, "TestSubclass", "Test subclass", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != NULL, "failed to create test subclass wnd\n");
    check_unicode(hwnd, EXPECT_WNDPROC_1);
    SetWindowLongA(hwnd, GWLP_USERDATA, EXPECT_WNDPROC_1);

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    check_unicode(hwnd, EXPECT_UNICODE);
    SetWindowLongA(hwnd, GWLP_USERDATA, EXPECT_UNICODE);

    SendMessageA(hwnd, WM_USER, 1, 0);
    SendMessageA(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_BasicTest, "Basic");
    SendMessageW(hwnd, WM_CHAR, 0x30c2, 1);

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, DELETE_SELF);
    ok(ret == TRUE, "Expected TRUE\n");
    check_unicode(hwnd, EXPECT_UNICODE);

    SendMessageA(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_DeletedTest, "Deleted");
    check_unicode(hwnd, EXPECT_WNDPROC_1);
    SetWindowLongA(hwnd, GWLP_USERDATA, EXPECT_WNDPROC_1);

    SendMessageA(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_AfterDeletedTest, "After Deleted");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    orig_proc_3 = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc_3);
    check_unicode(hwnd, EXPECT_WNDPROC_3);
    SetWindowLongA(hwnd, GWLP_USERDATA, EXPECT_WNDPROC_3);

    SendMessageA(hwnd, WM_USER, 1, 0);
    SendMessageA(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_OldAfterNewTest, "Old after New");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 4, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    check_unicode(hwnd, EXPECT_WNDPROC_3);

    SendMessageA(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_MixTest, "Mix");

    check_unicode(hwnd, EXPECT_WNDPROC_3);

    /* Now the fun starts */
    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 4, SEND_NEST);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixAndNestTest, "Mix and nest");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 4, SEND_NEST | DELETE_SELF);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixNestDelTest, "Mix, nest, del");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 4, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 5, DELETE_PREV);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixDelPrevTest, "Mix and del prev");

    ret = pSetWindowSubclass(NULL, wnd_proc_sub, 1, 0);
    ok(ret == FALSE, "Expected FALSE\n");

    ret = pSetWindowSubclass(hwnd, NULL, 1, 0);
    ok(ret == FALSE, "Expected FALSE\n");

    pRemoveWindowSubclass(hwnd, wnd_proc_sub, 2);
    pRemoveWindowSubclass(hwnd, wnd_proc_sub, 5);

    check_unicode(hwnd, EXPECT_WNDPROC_3);

    DestroyWindow(hwnd);
}

static BOOL register_window_classes(void)
{
    WNDCLASSA cls;
    ATOM atom;

    cls.style = 0;
    cls.lpfnWndProc = wnd_proc_1;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestSubclass";
    atom = RegisterClassA(&cls);
    ok(atom, "failed to register test class\n");

    return atom != 0;
}

static BOOL init_function_pointers(void)
{
    HMODULE hmod;
    void *ptr;

    hmod = LoadLibraryA("comctl32.dll");
    ok(hmod != NULL, "got %p\n", hmod);

    /* Functions have to be loaded by ordinal. Only XP and W2K3 export
     * them by name.
     */
#define MAKEFUNC_ORD(f, ord) (p##f = (void*)GetProcAddress(hmod, (LPSTR)(ord)))
    MAKEFUNC_ORD(SetWindowSubclass, 410);
    MAKEFUNC_ORD(GetWindowSubclass, 411);
    MAKEFUNC_ORD(RemoveWindowSubclass, 412);
    MAKEFUNC_ORD(DefSubclassProc, 413);
#undef MAKEFUNC_ORD

    if(!pSetWindowSubclass || !pGetWindowSubclass || !pRemoveWindowSubclass || !pDefSubclassProc)
    {
        win_skip("SetWindowSubclass and friends are not available\n");
        return FALSE;
    }

    /* test named exports */
    ptr = GetProcAddress(hmod, "SetWindowSubclass");
    ok(broken(ptr == 0) || ptr != 0, "expected named export for SetWindowSubclass\n");
    if(ptr)
    {
#define TESTNAMED(f) \
    ptr = (void*)GetProcAddress(hmod, #f); \
    ok(ptr != 0, "expected named export for " #f "\n");
        TESTNAMED(RemoveWindowSubclass);
        TESTNAMED(DefSubclassProc);
        /* GetWindowSubclass exported for V6 only */
#undef TESTNAMED
    }

    return TRUE;
}

static void test_GetWindowSubclass(void)
{
    DWORD_PTR data;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowA("TestSubclass", "Test subclass", WS_OVERLAPPEDWINDOW, 100, 100, 200, 200,
                         0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, 7);
    ok(ret, "SetWindowSubclass failed.\n");

    data = 0xdeadbeef;
    ret = pGetWindowSubclass(NULL, wnd_proc_sub, 2, &data);
    ok(!ret, "GetWindowSubclass succeeded.\n");
    ok(data == 0, "Got unexpected data %#Ix.\n", data);

    data = 0xdeadbeef;
    ret = pGetWindowSubclass(hwnd, NULL, 2, &data);
    ok(!ret, "GetWindowSubclass succeeded.\n");
    ok(data == 0, "Got unexpected data %#Ix.\n", data);

    data = 0xdeadbeef;
    ret = pGetWindowSubclass(hwnd, wnd_proc_sub, 0, &data);
    ok(!ret, "GetWindowSubclass succeeded.\n");
    ok(data == 0, "Got unexpected data %#Ix.\n", data);

    ret = pGetWindowSubclass(hwnd, wnd_proc_sub, 2, NULL);
    ok(ret, "GetWindowSubclass failed.\n");

    data = 0xdeadbeef;
    ret = pGetWindowSubclass(hwnd, wnd_proc_sub, 2, &data);
    ok(ret, "GetWindowSubclass failed.\n");
    ok(data == 7, "Got unexpected data %#Ix.\n", data);

    pRemoveWindowSubclass(hwnd, wnd_proc_sub, 2);
    DestroyWindow(hwnd);
}

START_TEST(subclass)
{
    if(!init_function_pointers()) return;

    if(!register_window_classes()) return;

    test_subclass();
    test_GetWindowSubclass();
}
