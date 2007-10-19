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

#define _WIN32_WINNT 0x0501 /* For SetWindowSubclass/etc */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

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
        sequence = HeapAlloc( GetProcessHeap(), 0, sequence_size * sizeof (struct message) );
    }
    if (sequence_cnt == sequence_size)
    {
        sequence_size *= 2;
        sequence = HeapReAlloc( GetProcessHeap(), 0, sequence, sequence_size * sizeof (struct message) );
    }
    assert(sequence);

    sequence[sequence_cnt].wParam  = msg->wParam;
    sequence[sequence_cnt].procnum = msg->procnum;

    sequence_cnt++;
}

static void flush_sequence(void)
{
    HeapFree(GetProcessHeap(), 0, sequence);
    sequence = 0;
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
            "%s: in procnum %d expecting wParam 0x%x got 0x%x\n",
            context, expected->procnum, expected->wParam, actual->wParam);
        expected++;
        actual++;
    }
    ok(!expected->procnum, "Received fewer messages than expected\n");
    ok(!actual->procnum, "Received more messages than expected\n");
    flush_sequence();
}

static LRESULT WINAPI WndProc1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg;

    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = 1;
        add_message(&msg);
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


static WNDPROC origProc3;
static LRESULT WINAPI WndProc3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg;

    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = 3;
        add_message(&msg);
    }
    return CallWindowProc(origProc3, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI WndProcSub(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uldSubclass, DWORD_PTR dwRefData)
{
    struct message msg;

    if(message == WM_USER) {
        msg.wParam = wParam;
        msg.procnum = uldSubclass;
        add_message(&msg);

        if(lParam) {
            if(dwRefData & DELETE_SELF) {
                pRemoveWindowSubclass(hwnd, WndProcSub, uldSubclass);
                pRemoveWindowSubclass(hwnd, WndProcSub, uldSubclass);
            }
            if(dwRefData & DELETE_PREV)
                pRemoveWindowSubclass(hwnd, WndProcSub, uldSubclass-1);
            if(dwRefData & SEND_NEST)
                SendMessage(hwnd, WM_USER, wParam+1, 0);
        }
    }
    return pDefSubclassProc(hwnd, message, wParam, lParam);
}

static void test_subclass(void)
{
    HWND hwnd = CreateWindowExA(0, "TestSubclass", "Test subclass", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    assert(hwnd);

    pSetWindowSubclass(hwnd, WndProcSub, 2, 0);
    SendMessage(hwnd, WM_USER, 1, 0);
    SendMessage(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_BasicTest, "Basic");

    pSetWindowSubclass(hwnd, WndProcSub, 2, DELETE_SELF);
    SendMessage(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_DeletedTest, "Deleted");

    SendMessage(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_AfterDeletedTest, "After Deleted");

    pSetWindowSubclass(hwnd, WndProcSub, 2, 0);
    origProc3 = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)WndProc3);
    SendMessage(hwnd, WM_USER, 1, 0);
    SendMessage(hwnd, WM_USER, 2, 0);
    ok_sequence(Sub_OldAfterNewTest, "Old after New");

    pSetWindowSubclass(hwnd, WndProcSub, 4, 0);
    SendMessage(hwnd, WM_USER, 1, 0);
    ok_sequence(Sub_MixTest, "Mix");

    /* Now the fun starts */
    pSetWindowSubclass(hwnd, WndProcSub, 4, SEND_NEST);
    SendMessage(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixAndNestTest, "Mix and nest");

    pSetWindowSubclass(hwnd, WndProcSub, 4, SEND_NEST | DELETE_SELF);
    SendMessage(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixNestDelTest, "Mix, nest, del");

    pSetWindowSubclass(hwnd, WndProcSub, 4, 0);
    pSetWindowSubclass(hwnd, WndProcSub, 5, DELETE_PREV);
    SendMessage(hwnd, WM_USER, 1, 1);
    ok_sequence(Sub_MixDelPrevTest, "Mix and del prev");

    DestroyWindow(hwnd);
}

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = WndProc1;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestSubclass";
    if(!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

START_TEST(subclass)
{
    HMODULE hdll;

    hdll = GetModuleHandleA("comctl32.dll");
    assert(hdll);
    pSetWindowSubclass = (void*)GetProcAddress(hdll, "SetWindowSubclass");
    pRemoveWindowSubclass = (void*)GetProcAddress(hdll, "RemoveWindowSubclass");
    pDefSubclassProc = (void*)GetProcAddress(hdll, "DefSubclassProc");

    if(!pSetWindowSubclass || !pRemoveWindowSubclass || !pDefSubclassProc)
        return;

    if(!RegisterWindowClasses()) assert(0);

    test_subclass();
}
