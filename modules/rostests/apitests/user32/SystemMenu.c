/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for system menu messages, based on msg.c Wine test
 * COPYRIGHT:   Copyright 1999 Ove Kaaven <ovek@transgaming.com>
 *              Copyright 2003 Dimitrie O. Paun <dimi@lattica.com>
 *              Copyright 2004-2005, 2016 Dmitry Timoshkov <dmitry@baikal.ru>
 *              Copyright 2023 Egor Ananyin <ananinegor@gmail.com>
 */

#include "precomp.h"

#define WND_PARENT_ID  1
#define WND_POPUP_ID   2

static BOOL (WINAPI *pGetCurrentActCtx)(HANDLE *);
static BOOL (WINAPI *pQueryActCtxW)(DWORD, HANDLE, void *, ULONG, void *, SIZE_T, SIZE_T *);

static BOOL test_DestroyWindow_flag;
static HWINEVENTHOOK hEvent_hook;
static HHOOK hKBD_hook;
static HHOOK hCBT_hook;
static DWORD cbt_hook_thread_id;

typedef enum
{
    sent = 0x1,
    posted = 0x2,
    parent = 0x4,
    wparam = 0x8,
    lparam = 0x10,
    defwinproc = 0x20,
    beginpaint = 0x40,
    optional = 0x80,
    hook = 0x100,
    winevent_hook = 0x200,
    kbd_hook = 0x400
} msg_flags_t;

struct message
{
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    WPARAM wParam;         /* expected value of wParam */
    LPARAM lParam;         /* expected value of lParam */
    WPARAM wp_mask;        /* mask for wParam checks */
    LPARAM lp_mask;        /* mask for lParam checks */
};

struct recvd_message
{
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    HWND hwnd;             /* window that received the message */
    WPARAM wParam;         /* expected value of wParam */
    LPARAM lParam;         /* expected value of lParam */
    int line;              /* source line where logged */
    const char *descr;     /* description for trace output */
    char output[512];      /* trace output */
};

static int sequence_cnt, sequence_size;
static struct recvd_message* sequence;
static CRITICAL_SECTION sequence_cs;

/* user32 functions */
static HWND (WINAPI *pGetAncestor)(HWND, UINT);
static BOOL (WINAPI *pUnhookWinEvent)(HWINEVENTHOOK);

static void init_procs(void)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");

#define GET_PROC(dll, func) \
    p ## func = (void*)GetProcAddress(dll, #func); \
    if (!p ## func) { \
        trace("GetProcAddress(%s) failed\n", #func); \
    }

    GET_PROC(user32, GetAncestor)
    GET_PROC(user32, UnhookWinEvent)

#undef GET_PROC
}

static BOOL ignore_message(UINT message)
{
    /* these are always ignored */
    return (message >= 0xc000 ||
            message == WM_GETICON ||
            message == WM_GETOBJECT ||
            message == WM_TIMECHANGE ||
            message == WM_DISPLAYCHANGE ||
            message == WM_DEVICECHANGE ||
            message == WM_DWMNCRENDERINGCHANGED);
}

#define add_message(msg) add_message_(__LINE__, msg);
static void add_message_(int line, const struct recvd_message *msg)
{
    struct recvd_message *seq;

    EnterCriticalSection(&sequence_cs);
    if (!sequence)
    {
        sequence_size = 10;
        sequence = HeapAlloc(GetProcessHeap(), 0, sequence_size * sizeof(*sequence));
    }
    if (sequence_cnt == sequence_size) 
    {
        sequence_size *= 2;
        sequence = HeapReAlloc(GetProcessHeap(), 0, sequence, sequence_size * sizeof(*sequence));
    }
    assert(sequence);

    seq = &sequence[sequence_cnt++];
    seq->hwnd = msg->hwnd;
    seq->message = msg->message;
    seq->flags = msg->flags;
    seq->wParam = msg->wParam;
    seq->lParam = msg->lParam;
    seq->line   = line;
    seq->descr  = msg->descr;
    seq->output[0] = 0;
    LeaveCriticalSection(&sequence_cs);

    if (msg->descr)
    {
        if (msg->flags & hook)
        {
            static const char * const CBT_code_name[10] =
            {
                "HCBT_MOVESIZE",
                "HCBT_MINMAX",
                "HCBT_QS",
                "HCBT_CREATEWND",
                "HCBT_DESTROYWND",
                "HCBT_ACTIVATE",
                "HCBT_CLICKSKIPPED",
                "HCBT_KEYSKIPPED",
                "HCBT_SYSCOMMAND",
                "HCBT_SETFOCUS"
            };
            const char *code_name = (msg->message <= HCBT_SETFOCUS ? CBT_code_name[msg->message] : "Unknown");

            sprintf(seq->output, "%s: hook %d (%s) wp %08Ix lp %08Ix",
                    msg->descr, msg->message, code_name, msg->wParam, msg->lParam);
        }
        else if (msg->flags & winevent_hook)
        {
            sprintf(seq->output, "%s: winevent %p %08x %08Ix %08Ix",
                    msg->descr, msg->hwnd, msg->message, msg->wParam, msg->lParam);
        }
        else
        {
            if (msg->message >= 0xc000)
                return;  /* ignore registered messages */
            sprintf(seq->output, "%s: %p %04x wp %08Ix lp %08Ix",
                    msg->descr, msg->hwnd, msg->message, msg->wParam, msg->lParam);
            if (msg->flags & (sent | posted | parent | defwinproc | beginpaint))
                sprintf(seq->output + strlen(seq->output), " (flags %x)", msg->flags);
        }
    }
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static void flush_sequence(void)
{
    EnterCriticalSection(&sequence_cs);
    HeapFree(GetProcessHeap(), 0, sequence);
    sequence = 0;
    sequence_cnt = sequence_size = 0;
    LeaveCriticalSection(&sequence_cs);
}

static void dump_sequence(const struct message *expected, const char *context, const char *file, int line)
{
    const struct recvd_message *actual = sequence;
    unsigned int count = 0;

    trace_(file, line)("Failed sequence %s:\n", context);

    while (expected->message && actual->message)
    {
        if (actual->output[0])
        {
            if (expected->flags & hook)
            {
                trace_(file, line)("  %u: expected: hook %04x - actual: %s\n",
                                   count, expected->message, actual->output);
            }
            else if (expected->flags & winevent_hook)
            {
                trace_(file, line)("  %u: expected: winevent %04x - actual: %s\n",
                                   count, expected->message, actual->output);
            }
            else if (expected->flags & kbd_hook)
            {
                trace_(file, line)("  %u: expected: kbd %04x - actual: %s\n",
                                   count, expected->message, actual->output);
            }
            else
            {
                trace_(file, line)("  %u: expected: msg %04x - actual: %s\n",
                                   count, expected->message, actual->output);
            }
        }

        if (expected->message == actual->message)
        {
            if ((expected->flags & defwinproc) != (actual->flags & defwinproc) &&
                    (expected->flags & optional))
            {
                /* don't match messages if their defwinproc status differs */
                expected++;
            }
            else
            {
                expected++;
                actual++;
            }
        }
        /* silently drop winevent messages if there is no support for them */
        else if ((expected->flags & optional) || ((expected->flags & winevent_hook) && !hEvent_hook))
            expected++;
        else
        {
            expected++;
            actual++;
        }

        count++;
    }

    /* optional trailing messages */
    while (expected->message && ((expected->flags & optional) ||
        ((expected->flags & winevent_hook) && !hEvent_hook)))
    {
        trace_(file, line)("  %u: expected: msg %04x - actual: nothing\n", count, expected->message);
        expected++;
        count++;
    }

    if (expected->message)
    {
        trace_(file, line)("  %u: expected: msg %04x - actual: nothing\n", count, expected->message);
        return;
    }

    while (actual->message && actual->output[0])
    {
        trace_(file, line)("  %u: expected: nothing - actual: %s\n", count, actual->output);
        actual++;
        count++;
    }
}

#define ok_sequence(exp, contx, todo) \
        ok_sequence_((exp), (contx), (todo), __FILE__, __LINE__)


static void ok_sequence_(const struct message *expected_list, const char *context, BOOL todo,
                         const char *file, int line)
{
    static const struct recvd_message end_of_sequence;
    const struct message *expected = expected_list;
    const struct recvd_message *actual;
    int failcount = 0, dump = 0;
    unsigned int count = 0;

    add_message(&end_of_sequence);

    actual = sequence;

    while (expected->message && actual->message)
    {
        if (expected->message == actual->message &&
            !((expected->flags ^ actual->flags) & (hook | winevent_hook | kbd_hook)))
        {
            if (expected->flags & wparam)
            {
                if (((expected->wParam ^ actual->wParam) & ~expected->wp_mask) && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        if (strcmp(winetest_platform, "wine"))
                            dump++;
                        ok_( file, line) (FALSE,
                            "%s: %u: in msg 0x%04x expecting wParam 0x%x got 0x%x\n",
                            context, count, expected->message, expected->wParam, actual->wParam);
                    }
                }
                else
                {
                    ok_( file, line)(((expected->wParam ^ actual->wParam) & ~expected->wp_mask) == 0,
                                     "%s: %u: in msg 0x%04x expecting wParam 0x%x got 0x%x\n",
                                     context, count, expected->message, expected->wParam, actual->wParam);
                    if ((expected->wParam ^ actual->wParam) & ~expected->wp_mask)
                        dump++;
                }
            }

            if (expected->flags & lparam)
            {
                if (((expected->lParam ^ actual->lParam) & ~expected->lp_mask) && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        if (strcmp(winetest_platform, "wine"))
                            dump++;
                        ok_( file, line) (FALSE,
                            "%s: %u: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                            context, count, expected->message, expected->lParam, actual->lParam);
                    }
                }
                else
                {
                    ok_( file, line)(((expected->lParam ^ actual->lParam) & ~expected->lp_mask) == 0,
                                     "%s: %u: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                                     context, count, expected->message, expected->lParam, actual->lParam);
                    if ((expected->lParam ^ actual->lParam) & ~expected->lp_mask)
                        dump++;
                }
            }
            if ((expected->flags & optional) &&
                ((expected->flags ^ actual->flags) & (defwinproc | parent)))
            {
                /* don't match optional messages if their defwinproc or parent status differs */
                expected++;
                count++;
                continue;
            }
            if ((expected->flags & defwinproc) != (actual->flags & defwinproc) && todo)
            {
                todo_wine
                {
                    failcount++;
                    if (strcmp(winetest_platform, "wine"))
                        dump++;
                    ok_(file, line) (FALSE,
                        "%s: %u: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                        context, count, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                }
            }
            else
            {
                ok_(file, line) ((expected->flags & defwinproc) == (actual->flags & defwinproc),
                    "%s: %u: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                    context, count, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                if ((expected->flags & defwinproc) != (actual->flags & defwinproc))
                    dump++;
            }

            ok_(file, line) ((expected->flags & beginpaint) == (actual->flags & beginpaint),
                "%s: %u: the msg 0x%04x should %shave been sent by BeginPaint\n",
                context, count, expected->message, (expected->flags & beginpaint) ? "" : "NOT ");
            if ((expected->flags & beginpaint) != (actual->flags & beginpaint))
                dump++;

            ok_(file, line) ((expected->flags & (sent | posted)) == (actual->flags & (sent | posted)),
                "%s: %u: the msg 0x%04x should have been %s\n",
                context, count, expected->message, (expected->flags & posted) ? "posted" : "sent");
            if ((expected->flags & (sent | posted)) != (actual->flags & (sent | posted)))
                dump++;

            ok_(file, line) ((expected->flags & parent) == (actual->flags & parent),
                "%s: %u: the msg 0x%04x was expected in %s\n",
                context, count, expected->message, (expected->flags & parent) ? "parent" : "child");
            if ((expected->flags & parent) != (actual->flags & parent))
                dump++;

            ok_(file, line) ((expected->flags & hook) == (actual->flags & hook),
                "%s: %u: the msg 0x%04x should have been sent by a hook\n",
                context, count, expected->message);
            if ((expected->flags & hook) != (actual->flags & hook))
                dump++;

            ok_(file, line) ((expected->flags & winevent_hook) == (actual->flags & winevent_hook),
                "%s: %u: the msg 0x%04x should have been sent by a winevent hook\n",
                context, count, expected->message);
            if ((expected->flags & winevent_hook) != (actual->flags & winevent_hook))
                dump++;

            ok_(file, line) ((expected->flags & kbd_hook) == (actual->flags & kbd_hook),
                "%s: %u: the msg 0x%04x should have been sent by a keyboard hook\n",
                context, count, expected->message);
            if ((expected->flags & kbd_hook) != (actual->flags & kbd_hook))
                dump++;

            expected++;
            actual++;
        }
        /* silently drop hook messages if there is no support for them */
        else if ((expected->flags & optional) ||
                 ((expected->flags & hook) && !hCBT_hook) ||
                 ((expected->flags & winevent_hook) && !hEvent_hook) ||
                 ((expected->flags & kbd_hook) && !hKBD_hook))
            expected++;
        else if (todo)
        {
            failcount++;
            todo_wine
            {
                if (strcmp(winetest_platform, "wine"))
                    dump++;
                ok_(file, line) (FALSE, "%s: %u: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                                 context, count, expected->message, actual->message);
            }
            goto done;
        }
        else
        {
            ok_(file, line) (FALSE, "%s: %u: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                             context, count, expected->message, actual->message);
            dump++;
            expected++;
            actual++;
        }
        count++;
    }

    /* skip all optional trailing messages */
    while (expected->message && ((expected->flags & optional) ||
                                 ((expected->flags & hook) && !hCBT_hook) ||
                                 ((expected->flags & winevent_hook) && !hEvent_hook)))
        expected++;

    if (todo)
    {
        todo_wine
        {
            if (expected->message || actual->message)
            {
                failcount++;
                if (strcmp(winetest_platform, "wine"))
                    dump++;
                ok_(file, line) (FALSE, "%s: %u: the msg sequence is not complete: expected %04x - actual %04x\n",
                                 context, count, expected->message, actual->message);
            }
        }
    }
    else
    {
        if (expected->message || actual->message)
        {
            dump++;
            ok_(file, line) (FALSE, "%s: %u: the msg sequence is not complete: expected %04x - actual %04x\n",
                             context, count, expected->message, actual->message);
        }
    }
    if (todo && !failcount) /* succeeded yet marked todo */
        todo_wine
        {
            if (!strcmp(winetest_platform, "wine"))
                dump++;
            ok_(file, line) (TRUE, "%s: marked \"todo_wine\" but succeeds\n", context);
        }

done:
    if (dump) dump_sequence(expected_list, context, file, line);
    flush_sequence();
}

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))

/************* window procedures ********************/

static LRESULT MsgCheckProc(BOOL unicode, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    static LONG beginpaint_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message(message)) return 0;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    if (beginpaint_counter) msg.flags |= beginpaint;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "MsgCheckProc";
    add_message(&msg);

    defwndproc_counter++;
    ret = unicode ? DefWindowProcW(hwnd, message, wParam, lParam) 
                  : DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI MsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return MsgCheckProc(FALSE, hwnd, message, wParam, lParam);
}

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = MsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestWindowClass";
    if (!RegisterClassA(&cls)) return FALSE;

    return TRUE;
}

static BOOL is_our_logged_class(HWND hwnd)
{
    char buf[256];

    if (GetClassNameA(hwnd, buf, sizeof(buf)))
    {
        if (!lstrcmpiA(buf, "TestWindowClass") ||
            !lstrcmpiA(buf, "ShowWindowClass") ||
            !lstrcmpiA(buf, "RecursiveActivationClass") ||
            !lstrcmpiA(buf, "TestParentClass") ||
            !lstrcmpiA(buf, "TestPopupClass") ||
            !lstrcmpiA(buf, "SimpleWindowClass") ||
            !lstrcmpiA(buf, "TestDialogClass") ||
            !lstrcmpiA(buf, "MDI_frame_class") ||
            !lstrcmpiA(buf, "MDI_client_class") ||
            !lstrcmpiA(buf, "MDI_child_class") ||
            !lstrcmpiA(buf, "my_button_class") ||
            !lstrcmpiA(buf, "my_edit_class") ||
            !lstrcmpiA(buf, "static") ||
            !lstrcmpiA(buf, "ListBox") ||
            !lstrcmpiA(buf, "ComboBox") ||
            !lstrcmpiA(buf, "MyDialogClass") ||
            !lstrcmpiA(buf, "#32770") ||
            !lstrcmpiA(buf, "#32768"))
        return TRUE;
    }
    return FALSE;
}

static LRESULT CALLBACK cbt_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    HWND hwnd;

    ok(cbt_hook_thread_id == GetCurrentThreadId(), "we didn't ask for events from other threads\n");

    if (nCode == HCBT_CLICKSKIPPED)
    {
        /* ignore this event, XP sends it a lot when switching focus between windows */
        return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
    }

    if (nCode == HCBT_SYSCOMMAND || nCode == HCBT_KEYSKIPPED)
    {
        struct recvd_message msg;

        msg.hwnd = 0;
        msg.message = nCode;
        msg.flags = hook | wparam | lparam;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "CBT";
        add_message(&msg);

        return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
    }

    if (nCode == HCBT_DESTROYWND)
    {
        if (test_DestroyWindow_flag)
        {
            DWORD style = GetWindowLongA((HWND)wParam, GWL_STYLE);
            if (style & WS_CHILD)
                lParam = GetWindowLongPtrA((HWND)wParam, GWLP_ID);
            else if (style & WS_POPUP)
                lParam = WND_POPUP_ID;
            else
                lParam = WND_PARENT_ID;
        }
    }

    /* Log also SetFocus(0) calls */
    hwnd = wParam ? (HWND)wParam : (HWND)lParam;

    if (is_our_logged_class(hwnd))
    {
        struct recvd_message msg;

        msg.hwnd = hwnd;
        msg.message = nCode;
        msg.flags = hook | wparam | lparam;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "CBT";
        add_message(&msg);
    }
    return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
}

/*************************** Menu test ******************************/

static const struct message wm_popup_menu_4[] =
{
    { HCBT_CREATEWND, hook },
    { WM_ENTERMENULOOP, sent | wparam | lparam, 0, 0 },
    { WM_INITMENU, sent | lparam, 0, 0 },
    { WM_INITMENUPOPUP, sent | lparam, 0, 0x10000 },
    { HCBT_KEYSKIPPED, hook | wparam | lparam | optional, VK_DOWN, 0x10000001 },
    { WM_MENUSELECT, sent, MAKEWPARAM(0,0xffff), 0 },
    { HCBT_KEYSKIPPED, hook | wparam | lparam | optional, VK_DOWN, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook | wparam | lparam | optional, VK_ESCAPE, 0x10000001 },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent | lparam, 0, 0x20000000 },
    { WM_MENUSELECT, sent, MAKEWPARAM(0,0xffff), 0 },
    { WM_EXITMENULOOP, sent | wparam | lparam, 0, 0 },
    { HCBT_KEYSKIPPED, hook | wparam | lparam | optional, VK_ESCAPE, 0xc0000001 },
    { WM_KEYUP, sent | wparam | lparam, VK_ESCAPE, 0xc0000001 },
    { 0 }
};

static LRESULT WINAPI parent_menu_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_ENTERIDLE ||
        message == WM_INITMENU ||
        message == WM_INITMENUPOPUP ||
        message == WM_MENUSELECT ||
        message == WM_PARENTNOTIFY ||
        message == WM_ENTERMENULOOP ||
        message == WM_EXITMENULOOP ||
        message == WM_UNINITMENUPOPUP ||
        message == WM_KEYDOWN ||
        message == WM_KEYUP ||
        message == WM_CHAR ||
        message == WM_SYSKEYDOWN ||
        message == WM_SYSKEYUP ||
        message == WM_SYSCHAR ||
        message == WM_COMMAND ||
        message == WM_MENUCOMMAND)
    {
        struct recvd_message msg;

        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent | wparam | lparam;
        msg.wParam = wp;
        msg.lParam = lp;
        msg.descr = "parent_menu_proc";
        add_message(&msg);
    }

    return DefWindowProcA(hwnd, message, wp, lp);
}

static void test_menu_messages(void)
{
    MSG msg;
    WNDCLASSA cls;
    HWND hwnd;
    RECT rect;

    cls.style = 0;
    cls.lpfnWndProc = parent_menu_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestMenuClass";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls))
        assert(0);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "TestMenuClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "LoadMenuA error %lu\n", GetLastError());

    SetForegroundWindow(hwnd);
    flush_events();

    trace("testing system menu\n");
    GetWindowRect(hwnd, &rect);
    SetCursorPos(rect.left + 30, rect.top + 10);
    flush_sequence();
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
    keybd_event(VK_DOWN, 0, 0, 0);
    keybd_event(VK_DOWN, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_ESCAPE, 0, 0, 0);
    keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok_sequence(wm_popup_menu_4, "system menu command", FALSE);

    DestroyWindow(hwnd);
}

static void init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f)
    X(GetCurrentActCtx);
    X(QueryActCtxW);
#undef X
}

static void init_tests()
{
    init_funcs();

    InitializeCriticalSection(&sequence_cs);
    init_procs();

    if (!RegisterWindowClasses())
        assert(0);

    cbt_hook_thread_id = GetCurrentThreadId();
    hCBT_hook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    if (!hCBT_hook)
        win_skip("cannot set global hook, will skip hook tests\n");
}

static void cleanup_tests()
{
    BOOL ret;
    UnhookWindowsHookEx(hCBT_hook);
    if (pUnhookWinEvent && hEvent_hook)
    {
        ret = pUnhookWinEvent(hEvent_hook);
        ok(ret, "UnhookWinEvent error %ld\n", GetLastError());
        SetLastError(0xdeadbeef);
        ok(!pUnhookWinEvent(hEvent_hook), "UnhookWinEvent succeeded\n");
        ok(GetLastError() == ERROR_INVALID_HANDLE || /* Win2k */
           GetLastError() == 0xdeadbeef, /* Win9x */
           "unexpected error %ld\n", GetLastError());
    }
    DeleteCriticalSection(&sequence_cs);
}

START_TEST(SystemMenu)
{
    init_tests();
    test_menu_messages();
    cleanup_tests();
}
