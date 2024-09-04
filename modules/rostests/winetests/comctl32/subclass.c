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

#define _WIN32_WINNT 0x0501 /* For SetWindowSubclass/etc */

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/heap.h"
#include "wine/test.h"

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);

#define SEND_NEST   0x01
#define DELETE_SELF 0x02
#define DELETE_PREV 0x04

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
        sequence = heap_alloc( sequence_size * sizeof (struct message) );
    }
    if (sequence_cnt == sequence_size)
    {
        sequence_size *= 2;
        sequence = heap_realloc( sequence, sequence_size * sizeof (struct message) );
    }
    assert(sequence);

    sequence[sequence_cnt].wParam  = msg->wParam;
    sequence[sequence_cnt].procnum = msg->procnum;

    sequence_cnt++;
}

static void flush_sequence(void)
{
    heap_free(sequence);
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
            "%s: in procnum %d expecting wParam 0x%lx got 0x%lx\n",
            context, expected->procnum, expected->wParam, actual->wParam);
        expected++;
        actual++;
    }
    ok(!expected->procnum, "Received fewer messages than expected\n");
    ok(!actual->procnum, "Received more messages than expected\n");
    flush_sequence();
}

static LRESULT WINAPI wnd_proc_1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg;
    
    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = 1;
        add_message(&msg);
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
    struct message msg;
    
    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = uldSubclass;
        add_message(&msg);
        
        if(lParam) {
            if(dwRefData & DELETE_SELF) {
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass);
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass);
            }
            if(dwRefData & DELETE_PREV)
                pRemoveWindowSubclass(hwnd, wnd_proc_sub, uldSubclass-1);
            if(dwRefData & SEND_NEST)
                SendMessageA(hwnd, WM_USER, wParam+1, 0);
        }
    }
    return pDefSubclassProc(hwnd, message, wParam, lParam);
}

static void test_subclass(void)
{
    BOOL ret;
    HWND hwnd = CreateWindowExA(0, "TestSubclass", "Test subclass", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != NULL, "failed to create test subclass wnd\n");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 0);
    SendMessageA(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_BasicTest, "Basic");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, DELETE_SELF);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_DeletedTest, "Deleted");

    SendMessageA(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_AfterDeletedTest, "After Deleted");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 2, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    orig_proc_3 = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc_3);
    SendMessageA(hwnd, WM_USER, 1, 0);
    SendMessageA(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_OldAfterNewTest, "Old after New");

    ret = pSetWindowSubclass(hwnd, wnd_proc_sub, 4, 0);
    ok(ret == TRUE, "Expected TRUE\n");
    SendMessageA(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_MixTest, "Mix");

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
    MAKEFUNC_ORD(RemoveWindowSubclass, 412);
    MAKEFUNC_ORD(DefSubclassProc, 413);
#undef MAKEFUNC_ORD

    if(!pSetWindowSubclass || !pRemoveWindowSubclass || !pDefSubclassProc)
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

START_TEST(subclass)
{
    if(!init_function_pointers()) return;

    if(!register_window_classes()) return;

    test_subclass();
}
