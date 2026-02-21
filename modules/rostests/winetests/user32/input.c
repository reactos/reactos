/* Test Key event to Key message translation
 *
 * Copyright 2003 Rein Klazes
 * Copyright 2019 Remi Bernon for CodeWeavers
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

/* test whether the right type of messages:
 * WM_KEYUP/DOWN vs WM_SYSKEYUP/DOWN  are sent in case of combined
 * keystrokes.
 *
 * For instance <ALT>-X can be accomplished by
 * the sequence ALT-KEY-DOWN, X-KEY-DOWN, ALT-KEY-UP, X-KEY-UP
 * but also X-KEY-DOWN, ALT-KEY-DOWN, X-KEY-UP, ALT-KEY-UP
 * Whether a KEY or a SYSKEY message is sent is not always clear, it is
 * also not the same in WINNT as in WIN9X */

/* NOTE that there will be test failures under WIN9X
 * No applications are known to me that rely on this
 * so I don't fix it */

/* TODO:
 * 1. extend it to the wm_command and wm_syscommand notifications
 * 2. add some more tests with special cases like dead keys or right (alt) key
 * 3. there is some adapted code from input.c in here. Should really
 *    make that code exactly the same.
 * 4. resolve the win9x case when there is a need or the testing frame work
 *    offers a nice way.
 * 5. The test app creates a window, the user should not take the focus
 *    away during its short existence. I could do something to prevent that
 *    if it is a problem.
 *
 */

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winreg.h"
#include "ddk/hidsdi.h"
#include "imm.h"
#include "kbd.h"

#include "wine/test.h"
#ifdef __REACTOS__
/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    static char buffer[256];
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    ret = buffer;
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size) ret[max_size-1] = 0;
    va_end(valist);
    return ret;
}
#endif

#define check_member_( file, line, val, exp, fmt, member )                                         \
    ok_(file, line)( (val).member == (exp).member, "got " #member " " fmt "\n", (val).member )
#define check_member( val, exp, fmt, member )                                                      \
    check_member_( __FILE__, __LINE__, val, exp, fmt, member )

static const char *debugstr_ok( const char *cond )
{
    int c, n = 0;
    /* skip possible casts */
    while ((c = *cond++))
    {
        if (c == '(') n++;
        if (!n) break;
        if (c == ')') n--;
    }
    if (!strchr( cond - 1, '(' )) return wine_dbg_sprintf( "got %s", cond - 1 );
    return wine_dbg_sprintf( "%.*s returned", (int)strcspn( cond - 1, "( " ), cond - 1 );
}

#define ok_eq( e, r, t, f, ... )                                                                   \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v == (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_ne( e, r, t, f, ... )                                                                   \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v != (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_rect( e, r )                                                                            \
    do                                                                                             \
    {                                                                                              \
        RECT v = (r);                                                                              \
        ok( EqualRect( &v, &(e) ), "%s %s\n", debugstr_ok(#r), wine_dbgstr_rect(&v) );             \
    } while (0)
#define ok_point( e, r )                                                                           \
    do                                                                                             \
    {                                                                                              \
        POINT v = (r);                                                                             \
        ok( !memcmp( &v, &(e), sizeof(v) ), "%s %s\n", debugstr_ok(#r), wine_dbgstr_point(&v) );   \
    } while (0)
#define ok_ret( e, r ) ok_eq( e, r, UINT_PTR, "%Iu, error %ld", GetLastError() )

enum user_function
{
    MSG_TEST_WIN = 1,
    LL_HOOK_KEYBD,
    LL_HOOK_MOUSE,
    RAW_INPUT_KEYBOARD,
};

struct user_call
{
    enum user_function func;

    union
    {
        struct
        {
            HWND hwnd;
            UINT msg;
            WPARAM wparam;
            LPARAM lparam;
        } message;
        struct
        {
            UINT msg;
            UINT scan;
            UINT vkey;
            UINT flags;
            UINT_PTR extra;
        } ll_hook_kbd;
        struct
        {
            UINT msg;
            POINT point;
            UINT data;
            UINT flags;
            UINT time;
            UINT_PTR extra;
        } ll_hook_ms;
        struct
        {
            HWND hwnd;
            BYTE code;
            RAWKEYBOARD kbd;
        } raw_input;
    };

    BOOL todo;
    BOOL todo_value;
    BOOL broken;
};

static const struct user_call empty_sequence[] = {{0}};
static struct user_call current_sequence[1024];
static LONG current_sequence_len;

static const char *debugstr_wm( UINT msg )
{
    switch (msg)
    {
    case WM_CHAR: return "WM_CHAR";
    case WM_KEYDOWN: return "WM_KEYDOWN";
    case WM_KEYUP: return "WM_KEYUP";
    case WM_SYSCHAR: return "WM_SYSCHAR";
    case WM_SYSCOMMAND: return "WM_SYSCOMMAND";
    case WM_SYSKEYDOWN: return "WM_SYSKEYDOWN";
    case WM_SYSKEYUP: return "WM_SYSKEYUP";
    case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
    case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
    case WM_LBUTTONUP: return "WM_LBUTTONUP";
    case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
    case WM_NCHITTEST: return "WM_NCHITTEST";
    }
    return wine_dbg_sprintf( "%#x", msg );
}

static const char *debugstr_vk( UINT vkey )
{
    switch (vkey)
    {
    case VK_CONTROL: return "VK_CONTROL";
    case VK_LCONTROL: return "VK_LCONTROL";
    case VK_LMENU: return "VK_LMENU";
    case VK_LSHIFT: return "VK_LSHIFT";
    case VK_MENU: return "VK_MENU";
    case VK_RCONTROL: return "VK_RCONTROL";
    case VK_RMENU: return "VK_RMENU";
    case VK_RSHIFT: return "VK_RSHIFT";
    case VK_SHIFT: return "VK_SHIFT";
    }

    if (vkey >= '0' && vkey <= '9') return wine_dbg_sprintf( "%c", vkey );
    if (vkey >= 'A' && vkey <= 'Z') return wine_dbg_sprintf( "%c", vkey );
    return wine_dbg_sprintf( "%#x", vkey );
}

#define ok_call( a, b ) ok_call_( __FILE__, __LINE__, a, b )
static int ok_call_( const char *file, int line, const struct user_call *expected, const struct user_call *received )
{
    int ret;

    if ((ret = expected->func - received->func)) goto done;

    switch (expected->func)
    {
    case MSG_TEST_WIN:
        if ((ret = expected->message.hwnd - received->message.hwnd)) goto done;
        if ((ret = expected->message.msg - received->message.msg)) goto done;
        if ((ret = (expected->message.wparam - received->message.wparam))) goto done;
        if ((ret = (expected->message.lparam - received->message.lparam))) goto done;
        break;
    case LL_HOOK_KEYBD:
        if ((ret = expected->ll_hook_kbd.msg - received->ll_hook_kbd.msg)) goto done;
        if ((ret = (expected->ll_hook_kbd.scan - received->ll_hook_kbd.scan))) goto done;
        if ((ret = expected->ll_hook_kbd.vkey - received->ll_hook_kbd.vkey)) goto done;
        if ((ret = (expected->ll_hook_kbd.flags - received->ll_hook_kbd.flags))) goto done;
        if ((ret = (expected->ll_hook_kbd.extra - received->ll_hook_kbd.extra))) goto done;
        break;
    case LL_HOOK_MOUSE:
        if ((ret = expected->ll_hook_ms.msg - received->ll_hook_ms.msg)) goto done;
        if ((ret = expected->ll_hook_ms.point.x - received->ll_hook_ms.point.x)) goto done;
        if ((ret = expected->ll_hook_ms.point.y - received->ll_hook_ms.point.y)) goto done;
        if ((ret = (expected->ll_hook_ms.data - received->ll_hook_ms.data))) goto done;
        if ((ret = (expected->ll_hook_ms.flags - received->ll_hook_ms.flags))) goto done;
        if (0 && (ret = expected->ll_hook_ms.time - received->ll_hook_ms.time)) goto done;
        if ((ret = (expected->ll_hook_ms.extra - received->ll_hook_ms.extra))) goto done;
        break;
    case RAW_INPUT_KEYBOARD:
        if ((ret = expected->raw_input.hwnd - received->raw_input.hwnd)) goto done;
        if ((ret = expected->raw_input.code - received->raw_input.code)) goto done;
        if ((ret = expected->raw_input.kbd.MakeCode - received->raw_input.kbd.MakeCode)) goto done;
        if ((ret = expected->raw_input.kbd.Flags - received->raw_input.kbd.Flags)) goto done;
        if ((ret = expected->raw_input.kbd.VKey - received->raw_input.kbd.VKey)) goto done;
        if ((ret = expected->raw_input.kbd.Message - received->raw_input.kbd.Message)) goto done;
        if ((ret = expected->raw_input.kbd.ExtraInformation - received->raw_input.kbd.ExtraInformation)) goto done;
        break;
    }

done:
    if (ret && broken( expected->broken )) return ret;

    switch (received->func)
    {
    case MSG_TEST_WIN:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got MSG_TEST_WIN hwnd %p, msg %s, wparam %#Ix, lparam %#Ix\n", received->message.hwnd,
                         debugstr_wm(received->message.msg), received->message.wparam, received->message.lparam );
        return ret;
    case LL_HOOK_KEYBD:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got LL_HOOK_KEYBD msg %s scan %#x, vkey %s, flags %#x, extra %#Ix\n", debugstr_wm(received->ll_hook_kbd.msg),
                         received->ll_hook_kbd.scan, debugstr_vk(received->ll_hook_kbd.vkey), received->ll_hook_kbd.flags,
                         received->ll_hook_kbd.extra );
        return ret;
    case LL_HOOK_MOUSE:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got LL_HOOK_MOUSE msg %s, point %s, data %#x, flags %#x, time %u, extra %#Ix\n", debugstr_wm(received->ll_hook_ms.msg),
                         wine_dbgstr_point(&received->ll_hook_ms.point), received->ll_hook_ms.data, received->ll_hook_ms.flags, received->ll_hook_ms.time,
                         received->ll_hook_ms.extra );
        return ret;
    case RAW_INPUT_KEYBOARD:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got WM_INPUT key hwnd %p, code %d, make_code %#x, flags %#x, vkey %s, message %s, extra %#lx\n",
                         received->raw_input.hwnd, received->raw_input.code, received->raw_input.kbd.MakeCode,
                         received->raw_input.kbd.Flags, debugstr_vk(received->raw_input.kbd.VKey),
                         debugstr_wm(received->raw_input.kbd.Message), received->raw_input.kbd.ExtraInformation );
        return ret;
    }

    switch (expected->func)
    {
    case MSG_TEST_WIN:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "MSG_TEST_WIN hwnd %p, %s, wparam %#Ix, lparam %#Ix\n", expected->message.hwnd,
                         debugstr_wm(expected->message.msg), expected->message.wparam, expected->message.lparam );
        break;
    case LL_HOOK_KEYBD:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "LL_HOOK_KBD msg %s scan %#x, vkey %s, flags %#x, extra %#Ix\n", debugstr_wm(expected->ll_hook_kbd.msg),
                         expected->ll_hook_kbd.scan, debugstr_vk(expected->ll_hook_kbd.vkey), expected->ll_hook_kbd.flags,
                         expected->ll_hook_kbd.extra );
        break;
    case LL_HOOK_MOUSE:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "LL_HOOK_MOUSE msg %s, point %s, data %#x, flags %#x, time %u, extra %#Ix\n", debugstr_wm(received->ll_hook_ms.msg),
                         wine_dbgstr_point(&received->ll_hook_ms.point), received->ll_hook_ms.data, received->ll_hook_ms.flags, received->ll_hook_ms.time,
                         received->ll_hook_ms.extra );
        return ret;
    case RAW_INPUT_KEYBOARD:
        todo_wine_if( expected->todo || expected->todo_value )
        ok_(file, line)( !ret, "got WM_INPUT key hwnd %p, code %d, make_code %#x, flags %#x, vkey %s, message %s, extra %#lx\n",
                         expected->raw_input.hwnd, expected->raw_input.code, expected->raw_input.kbd.MakeCode,
                         expected->raw_input.kbd.Flags, debugstr_vk(expected->raw_input.kbd.VKey),
                         debugstr_wm(expected->raw_input.kbd.Message), expected->raw_input.kbd.ExtraInformation );
        return ret;
    }

    return 0;
}

#define ok_seq( a ) ok_seq_( __FILE__, __LINE__, a, #a )
static void ok_seq_( const char *file, int line, const struct user_call *expected, const char *context )
{
    const struct user_call *received = current_sequence;
    UINT i = 0, ret;

    while (expected->func || received->func)
    {
        winetest_push_context( "%s %u%s%s", context, i++, !expected->func ? " (spurious)" : "",
                               !received->func ? " (missing)" : "" );
        ret = ok_call_( file, line, expected, received );
        if (ret && expected->todo && expected->func &&
            !strcmp( winetest_platform, "wine" ))
            expected++;
        else if (ret && broken(expected->broken))
            expected++;
        else
        {
            if (expected->func) expected++;
            if (received->func) received++;
        }
        winetest_pop_context();
    }

    memset( current_sequence, 0, sizeof(current_sequence) );
    current_sequence_len = 0;
}

static BOOL append_message_hwnd;
static BOOL (*p_accept_message)( UINT msg );

static void append_ll_hook_kbd( UINT msg, const KBDLLHOOKSTRUCT *info )
{
    struct user_call call =
    {
        .func = LL_HOOK_KEYBD, .ll_hook_kbd =
        {
            .msg = msg, .scan = info->scanCode, .vkey = info->vkCode,
            .flags = info->flags, .extra = info->dwExtraInfo
        }
    };
    if (!p_accept_message || p_accept_message( msg ))
    {
        ULONG index = InterlockedIncrement( &current_sequence_len ) - 1;
        ok( index < ARRAY_SIZE(current_sequence), "got %lu calls\n", index );
        current_sequence[index] = call;
    }
}

static void append_ll_hook_ms( UINT msg, const MSLLHOOKSTRUCT *info )
{
    struct user_call call =
    {
        .func = LL_HOOK_MOUSE, .ll_hook_ms =
        {
            .msg = msg, .point = info->pt, .data = info->mouseData, .flags = info->flags,
            .time = info->time, .extra = info->dwExtraInfo
        }
    };
    if (!p_accept_message || p_accept_message( msg ))
    {
        ULONG index = InterlockedIncrement( &current_sequence_len ) - 1;
        ok( index < ARRAY_SIZE(current_sequence), "got %lu calls\n", index );
        current_sequence[index] = call;
    }
}

static void append_rawinput_message( HWND hwnd, WPARAM wparam, HRAWINPUT handle )
{
    RAWINPUT rawinput;
    UINT size = sizeof(rawinput), ret;

    ret = GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER) );
    ok_ne( ret, (UINT)-1, UINT, "%u" );

    if (rawinput.header.dwType == RIM_TYPEKEYBOARD)
    {
        struct user_call call =
        {
            .func = RAW_INPUT_KEYBOARD,
            .raw_input = {.hwnd = hwnd, .code = GET_RAWINPUT_CODE_WPARAM(wparam), .kbd = rawinput.data.keyboard}
        };
        ULONG index = InterlockedIncrement( &current_sequence_len ) - 1;
        ok( index < ARRAY_SIZE(current_sequence), "got %lu calls\n", index );
        if (!append_message_hwnd) call.message.hwnd = 0;
        current_sequence[index] = call;
    }
}

static void append_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_INPUT) append_rawinput_message( hwnd, wparam, (HRAWINPUT)lparam );
    else if (!p_accept_message || p_accept_message( msg ))
    {
        struct user_call call = {.func = MSG_TEST_WIN, .message = {.hwnd = hwnd, .msg = msg, .wparam = wparam, .lparam = lparam}};
        ULONG index = InterlockedIncrement( &current_sequence_len ) - 1;
        ok( index < ARRAY_SIZE(current_sequence), "got %lu calls\n", index );
        if (!append_message_hwnd) call.message.hwnd = 0;
        current_sequence[index] = call;
    }
}

static LRESULT CALLBACK append_message_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    append_message( hwnd, msg, wparam, lparam );
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

/* globals */
#define DESKTOP_ALL_ACCESS 0x01ff

static BOOL (WINAPI *pEnableMouseInPointer)( BOOL );
static BOOL (WINAPI *pIsMouseInPointerEnabled)(void);
static BOOL (WINAPI *pGetCurrentInputMessageSource)( INPUT_MESSAGE_SOURCE *source );
static BOOL (WINAPI *pGetPointerType)(UINT32, POINTER_INPUT_TYPE*);
static BOOL (WINAPI *pGetPointerInfo)(UINT32, POINTER_INFO*);
static BOOL (WINAPI *pGetPointerInfoHistory)(UINT32, UINT32*, POINTER_INFO*);
static BOOL (WINAPI *pGetPointerFrameInfo)(UINT32, UINT32*, POINTER_INFO*);
static BOOL (WINAPI *pGetPointerFrameInfoHistory)(UINT32, UINT32*, UINT32*, POINTER_INFO*);
static int (WINAPI *pGetMouseMovePointsEx) (UINT, LPMOUSEMOVEPOINT, LPMOUSEMOVEPOINT, int, DWORD);
static UINT (WINAPI *pGetRawInputDeviceList) (PRAWINPUTDEVICELIST, PUINT, UINT);
static UINT (WINAPI *pGetRawInputDeviceInfoW) (HANDLE, UINT, void *, UINT *);
static UINT (WINAPI *pGetRawInputDeviceInfoA) (HANDLE, UINT, void *, UINT *);
static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static HKL (WINAPI *pLoadKeyboardLayoutEx)(HKL, const WCHAR *, UINT);

/**********************adapted from input.c **********************************/

static BOOL is_wow64;

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("user32");

#define GET_PROC(func) \
    if (!(p ## func = (void*)GetProcAddress(hdll, #func))) \
      trace("GetProcAddress(%s) failed\n", #func)

    GET_PROC(EnableMouseInPointer);
    GET_PROC(IsMouseInPointerEnabled);
    GET_PROC(GetCurrentInputMessageSource);
    GET_PROC(GetMouseMovePointsEx);
    GET_PROC(GetPointerInfo);
    GET_PROC(GetPointerInfoHistory);
    GET_PROC(GetPointerFrameInfo);
    GET_PROC(GetPointerFrameInfoHistory);
    GET_PROC(GetPointerType);
    GET_PROC(GetRawInputDeviceList);
    GET_PROC(GetRawInputDeviceInfoW);
    GET_PROC(GetRawInputDeviceInfoA);
    GET_PROC(LoadKeyboardLayoutEx);

    hdll = GetModuleHandleA("kernel32");
    GET_PROC(IsWow64Process);
#undef GET_PROC

    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 ))
        is_wow64 = FALSE;
}

#define run_in_process( a, b ) run_in_process_( __FILE__, __LINE__, a, b )
static void run_in_process_( const char *file, int line, char **argv, const char *args )
{
    STARTUPINFOA startup = {.cb = sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION info = {0};
    char cmdline[MAX_PATH * 2];
    DWORD ret;

    sprintf( cmdline, "%s %s %s", argv[0], argv[1], args );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok_(file, line)( ret, "CreateProcessA failed, error %lu\n", GetLastError() );
    if (!ret) return;

    wait_child_process( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
}

#define run_in_desktop( a, b, c ) run_in_desktop_( __FILE__, __LINE__, a, b, c )
static void run_in_desktop_( const char *file, int line, char **argv,
                             const char *args, BOOL input )
{
    const char *desktop_name = "WineTest Desktop";
    STARTUPINFOA startup = {.cb = sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION info = {0};
    HDESK old_desktop, desktop;
    char cmdline[MAX_PATH * 2];
    DWORD ret;

    old_desktop = OpenInputDesktop( 0, FALSE, DESKTOP_ALL_ACCESS );
    ok_(file, line)( !!old_desktop, "OpenInputDesktop failed, error %lu\n", GetLastError() );
    desktop = CreateDesktopA( desktop_name, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok_(file, line)( !!desktop, "CreateDesktopA failed, error %lu\n", GetLastError() );
    if (input)
    {
        ret = SwitchDesktop( desktop );
        ok_(file, line)( ret, "SwitchDesktop failed, error %lu\n", GetLastError() );
    }

    startup.lpDesktop = (char *)desktop_name;
    sprintf( cmdline, "%s %s %s", argv[0], argv[1], args );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok_(file, line)( ret, "CreateProcessA failed, error %lu\n", GetLastError() );
    if (!ret) return;

    wait_child_process( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );

    if (input)
    {
        ret = SwitchDesktop( old_desktop );
        ok_(file, line)( ret, "SwitchDesktop failed, error %lu\n", GetLastError() );
    }
    ret = CloseDesktop( desktop );
    ok_(file, line)( ret, "CloseDesktop failed, error %lu\n", GetLastError() );
    ret = CloseDesktop( old_desktop );
    ok_(file, line)( ret, "CloseDesktop failed, error %lu\n", GetLastError() );
}

#define wait_messages( a, b ) msg_wait_for_events_( __FILE__, __LINE__, 0, NULL, a, b )
#define msg_wait_for_events( a, b, c ) msg_wait_for_events_( __FILE__, __LINE__, a, b, c, FALSE )
static DWORD msg_wait_for_events_( const char *file, int line, DWORD count, HANDLE *events, DWORD timeout, BOOL append_peeked )
{
    DWORD ret, end = GetTickCount() + min( timeout, 5000 );
    MSG msg;

    while ((ret = MsgWaitForMultipleObjects( count, events, FALSE, min( timeout, 5000 ), QS_ALLINPUT )) <= count)
    {
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            if (append_peeked) append_message( msg.hwnd, msg.message, msg.wParam, msg.lParam );
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        if (ret < count) return ret;
        if (timeout >= 5000) continue;
        if (end <= GetTickCount()) timeout = 0;
        else timeout = end - GetTickCount();
    }

    if (timeout >= 5000) ok_(file, line)( 0, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    else ok_(file, line)( ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    return ret;
}

static inline BOOL is_keyboard_message( UINT message )
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST);
}

static inline BOOL is_mouse_message( UINT message )
{
    return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST);
}

#define create_foreground_window( a ) create_foreground_window_( __FILE__, __LINE__, a, 5 )
HWND create_foreground_window_( const char *file, int line, BOOL fullscreen, UINT retries )
{
    for (;;)
    {
        HWND hwnd;
        BOOL ret;

        hwnd = CreateWindowW( L"static", NULL, WS_POPUP | (fullscreen ? 0 : WS_VISIBLE),
                              100, 100, 200, 200, NULL, NULL, NULL, NULL );
        ok_(file, line)( hwnd != NULL, "CreateWindowW failed, error %lu\n", GetLastError() );

        if (fullscreen)
        {
            HMONITOR hmonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );
            MONITORINFO mi = {.cbSize = sizeof(MONITORINFO)};

            ok_(file, line)( hmonitor != NULL, "MonitorFromWindow failed, error %lu\n", GetLastError() );
            ret = GetMonitorInfoW( hmonitor, &mi );
            ok_(file, line)( ret, "GetMonitorInfoW failed, error %lu\n", GetLastError() );
            ret = SetWindowPos( hwnd, 0, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                                mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW );
            ok_(file, line)( ret, "SetWindowPos failed, error %lu\n", GetLastError() );
        }
        wait_messages( 100, FALSE );

        if (GetForegroundWindow() == hwnd) return hwnd;
        ok_(file, line)( retries > 0, "failed to create foreground window\n" );
        if (!retries--) return hwnd;

        ret = DestroyWindow( hwnd );
        ok_(file, line)( ret, "DestroyWindow failed, error %lu\n", GetLastError() );
        wait_messages( 0, FALSE );
    }
}

/* try to make sure pending X events have been processed before continuing */
static void empty_message_queue(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 50;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (is_keyboard_message(msg.message) || is_mouse_message(msg.message))
                ok(msg.time != 0, "message %#x has time set to 0\n", msg.message);

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        diff = time - GetTickCount();
    }
}

struct send_input_keyboard_test
{
    WORD scan;
    WORD vkey;
    DWORD flags;
    struct user_call expect[8];
    BYTE expect_state[256];
    BOOL todo_state[256];
    BOOL async;
    BYTE expect_async[256];
    BOOL todo_async[256];
};

static LRESULT CALLBACK ll_hook_kbd_proc(int code, WPARAM wparam, LPARAM lparam)
{
    KBDLLHOOKSTRUCT *hook_info = (KBDLLHOOKSTRUCT *)lparam;

    if (code == HC_ACTION)
    {
        append_ll_hook_kbd( wparam, hook_info );

if(0) /* For some reason not stable on Wine */
{
        if (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN)
            ok(!(GetAsyncKeyState(hook_info->vkCode) & 0x8000), "key %lx should be up\n", hook_info->vkCode);
        else if (wparam == WM_KEYUP || wparam == WM_SYSKEYUP)
            ok(GetAsyncKeyState(hook_info->vkCode) & 0x8000, "key %lx should be down\n", hook_info->vkCode);
}

        if (winetest_debug > 1)
            trace("Hook:   w=%Ix vk:%8lx sc:%8lx fl:%8lx %Ix\n", wparam,
                  hook_info->vkCode, hook_info->scanCode, hook_info->flags, hook_info->dwExtraInfo);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

#define check_keyboard_state( a, b ) check_keyboard_state_( __LINE__, a, b )
static void check_keyboard_state_( int line, const BYTE expect_state[256], const BOOL todo_state[256] )
{
    BYTE state[256];
    UINT i;

    ok_ret( 1, GetKeyboardState( state ) );
    for (i = 0; i < ARRAY_SIZE(state); i++)
    {
        todo_wine_if( todo_state[i] )
        ok_(__FILE__, line)( (expect_state[i] & 0x80) == (state[i] & 0x80),
                             "got %s: %#x\n", debugstr_vk( i ), state[i] );
    }
}

#define check_keyboard_async( a, b ) check_keyboard_async_( __LINE__, a, b )
static void check_keyboard_async_( int line, const BYTE expect_state[256], const BOOL todo_state[256] )
{
    UINT i;

    /* TODO: figure out if the async state for vkey 0 provides any information and
     * add it to the check. */
    for (i = 1; i < 256; i++)
    {
        BYTE state = GetAsyncKeyState(i) >> 8;
        todo_wine_if( todo_state[i] )
        ok_(__FILE__, line)( (expect_state[i] & 0x80) == (state & 0x80),
                             "async got %s: %#x\n", debugstr_vk( i ), state );
    }
}

static void clear_keyboard_state( void )
{
    static BYTE empty_state[256] = {0};
    INPUT input = {.type = INPUT_KEYBOARD};
    BYTE lock_keys[] = {VK_NUMLOCK, VK_CAPITAL, VK_SCROLL};
    UINT i;

    for (i = 0; i < ARRAY_SIZE(lock_keys); ++i)
    {
        if (GetKeyState( lock_keys[i] ) & 0x0001)
        {
            input.ki.wVk = lock_keys[i];
            SendInput( 1, &input, sizeof(input) );
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput( 1, &input, sizeof(input) );
            wait_messages( 5, FALSE );
            memset( current_sequence, 0, sizeof(current_sequence) );
            current_sequence_len = 0;
        }
    }

    SetKeyboardState( empty_state );
}

#define check_send_input_keyboard_test( a, b ) check_send_input_keyboard_test_( a, #a, b )
static void check_send_input_keyboard_test_( const struct send_input_keyboard_test *test, const char *context, BOOL peeked )
{
    INPUT input = {.type = INPUT_KEYBOARD};
    UINT i;

    winetest_push_context( "%s", context );
    clear_keyboard_state();

    for (i = 0; test->vkey || test->scan; i++, test++)
    {
        winetest_push_context( "%u", i );

        input.ki.wScan = test->flags & (KEYEVENTF_SCANCODE | KEYEVENTF_UNICODE) ? test->scan : (i + 1);
        input.ki.dwFlags = test->flags;
        input.ki.wVk = test->vkey;
        ok_ret( 1, SendInput( 1, &input, sizeof(input) ) );
        wait_messages( 5, peeked );

        ok_seq( test->expect );
        check_keyboard_state( test->expect_state, test->todo_state );
        if (test->async) check_keyboard_async( test->expect_async, test->todo_async );

        winetest_pop_context();
    }

    clear_keyboard_state();
    winetest_pop_context();
}

static BOOL accept_keyboard_messages_syscommand( UINT msg )
{
    return is_keyboard_message( msg ) || msg == WM_SYSCOMMAND;
}

static BOOL keyboard_layout_has_altgr(void)
{
    typedef struct
    {
        UINT64 pCharModifiers;
        UINT64 pVkToWcharTable;
        UINT64 pDeadKey;
        UINT64 pKeyNames;
        UINT64 pKeyNamesExt;
        UINT64 pKeyNamesDead;
        UINT64 pusVSCtoVK;
        BYTE bMaxVSCtoVK;
        UINT64 pVSCtoVK_E0;
        UINT64 pVSCtoVK_E1;
        DWORD fLocaleFlags;
    } KBDTABLES64;

    WCHAR layout_path[MAX_PATH] = {L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\"}, value[MAX_PATH];
    KBDTABLES *(CDECL *pKbdLayerDescriptor)(void);
    DWORD value_size = sizeof(value), flags;
    KBDTABLES *tables;
    HMODULE module;

    ok_ret( 1, GetKeyboardLayoutNameW( layout_path + wcslen(layout_path) ) );
    todo_wine
    ok_ret( 0, RegGetValueW( HKEY_LOCAL_MACHINE, layout_path, L"Layout File", RRF_RT_REG_SZ,
                             NULL, (void *)&value, &value_size) );

    module = LoadLibraryW( value );
    todo_wine
    ok_ne( NULL, module, HMODULE, "%p" );
    pKbdLayerDescriptor = (void *)GetProcAddress( module, "KbdLayerDescriptor" );
    todo_wine
    ok_ne( NULL, pKbdLayerDescriptor, void *, "%p" );
    /* FIXME: Wine doesn't implement ALTGR behavior */
    if (!pKbdLayerDescriptor) return FALSE;
    tables = pKbdLayerDescriptor();
    ok_ne( NULL, tables, KBDTABLES *, "%p" );
    flags = is_wow64 ? ((KBDTABLES64 *)tables)->fLocaleFlags : tables->fLocaleFlags;
    ok_ret( 1, FreeLibrary( module ) );

    trace( "%s flags %#lx\n", debugstr_w(value), flags );
    return !!(flags & KLLF_ALTGR);
}

static void get_test_scan( WORD vkey, WORD *scan, WCHAR *wch, WCHAR *wch_shift )
{
    HKL hkl = GetKeyboardLayout( 0 );
    BYTE state[256] = {0};

    *scan = MapVirtualKeyExW( vkey, MAPVK_VK_TO_VSC_EX, hkl );
    ok_ne( 0, *scan, WORD, "%#x" );
    ok_ret( 1, ToUnicodeEx( vkey, *scan, state, wch, 1, 0, hkl ) );
    state[VK_SHIFT] = 0x80;
    ok_ret( 1, ToUnicodeEx( vkey, *scan, state, wch_shift, 1, 0, hkl ) );
}

static void test_SendInput_keyboard_messages( WORD vkey, WORD scan, WCHAR wch, WCHAR wch_shift, WCHAR wch_control, HKL hkl )
{
#define WIN_MSG(m, w, l, ...) {.func = MSG_TEST_WIN, .message = {.msg = m, .wparam = w, .lparam = l}, ## __VA_ARGS__}
#define KBD_HOOK(m, s, v, f, ...) {.func = LL_HOOK_KEYBD, .ll_hook_kbd = {.msg = m, .scan = s, .vkey = v, .flags = f}, ## __VA_ARGS__}

#define KEY_HOOK_(m, s, v, f, ...) KBD_HOOK( m, s, v, LLKHF_INJECTED | (m == WM_KEYUP || m == WM_SYSKEYUP ? LLKHF_UP : 0) | (f), ## __VA_ARGS__ )
#define KEY_HOOK(m, s, v, ...) KEY_HOOK_( m, s, v, 0, ## __VA_ARGS__ )

#define KEY_MSG_(m, s, v, f, ...) WIN_MSG( m, v, MAKELONG(1, (s) | (m == WM_KEYUP || m == WM_SYSKEYUP ? (KF_UP | KF_REPEAT) : 0) | (f)), ## __VA_ARGS__ )
#define KEY_MSG(m, s, v, ...) KEY_MSG_( m, s, v, 0, ## __VA_ARGS__ )

    struct send_input_keyboard_test lmenu_vkey[] =
    {
        {.vkey = VK_LMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {
            .vkey = vkey, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, /*[vkey] = 0x80*/},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 2, vkey, LLKHF_ALTDOWN, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYDOWN, 2, vkey, KF_ALTDOWN),
                WIN_MSG(WM_SYSCHAR, wch, MAKELONG(1, 2|KF_ALTDOWN)),
                WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, wch),
                {0}
            }
        },
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYUP, 3, vkey, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 3, vkey, KF_ALTDOWN), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LMENU), KEY_MSG(WM_KEYUP, 4, VK_MENU), {0}}},
        {0},
    };

    struct send_input_keyboard_test lmenu_vkey_peeked[] =
    {
        {.vkey = VK_LMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {
            .vkey = vkey, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, /*[vkey] = 0x80*/},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 2, vkey, LLKHF_ALTDOWN, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYDOWN, 2, vkey, KF_ALTDOWN),
                WIN_MSG(WM_SYSCHAR, wch, MAKELONG(1, 2|KF_ALTDOWN)),
                {0}
            }
        },
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYUP, 3, vkey, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 3, vkey, KF_ALTDOWN), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LMENU), KEY_MSG(WM_KEYUP, 4, VK_MENU), {0}}},
        {0},
    };

    struct send_input_keyboard_test lcontrol_vkey[] =
    {
        {.vkey = VK_LCONTROL, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_LCONTROL), KEY_MSG(WM_KEYDOWN, 1, VK_CONTROL), {0}}},
        {.vkey = vkey, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80, /*[vkey] = 0x80*/},
         .expect = {KEY_HOOK(WM_KEYDOWN, 2, vkey), KEY_MSG(WM_KEYDOWN, 2, vkey), WIN_MSG(WM_CHAR, wch_control, MAKELONG(1, 2)), {0}}},
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYUP, 3, vkey), KEY_MSG(WM_KEYUP, 3, vkey), {0}}},
        {.vkey = VK_LCONTROL, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LCONTROL), KEY_MSG(WM_KEYUP, 4, VK_CONTROL), {0}}},
        {0},
    };

    struct send_input_keyboard_test lmenu_lcontrol_vkey[] =
    {
        {.vkey = VK_LMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {.vkey = VK_LCONTROL, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 2, VK_LCONTROL), KEY_MSG_(WM_KEYDOWN, 2, VK_CONTROL, KF_ALTDOWN), {0}}},
        {.vkey = vkey, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80, /*[vkey] = 0x80*/},
         .expect = {KEY_HOOK(WM_KEYDOWN, 3, vkey), KEY_MSG_(WM_KEYDOWN, 3, vkey, KF_ALTDOWN), {0}}},
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYUP, 4, vkey), KEY_MSG_(WM_KEYUP, 4, vkey, KF_ALTDOWN), {0}}},
        {.vkey = VK_LCONTROL, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYUP, 5, VK_LCONTROL, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 5, VK_CONTROL, KF_ALTDOWN), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 6, VK_LMENU), KEY_MSG(WM_KEYUP, 6, VK_MENU), {0}}},
        {0},
    };

    struct send_input_keyboard_test shift_vkey[] =
    {
        {.vkey = VK_LSHIFT, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_LSHIFT), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = vkey, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80, /*[vkey] = 0x80*/},
         .expect = {KEY_HOOK(WM_KEYDOWN, 2, vkey), KEY_MSG(WM_KEYDOWN, 2, vkey), WIN_MSG(WM_CHAR, wch_shift, MAKELONG(1, 2)), {0}}},
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYUP, 3, vkey), KEY_MSG(WM_KEYUP, 3, vkey), {0}}},
        {.vkey = VK_LSHIFT, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LSHIFT), KEY_MSG(WM_KEYUP, 4, VK_SHIFT), {0}}},
        {0},
    };

    static const struct send_input_keyboard_test rshift[] =
    {
        {.vkey = VK_RSHIFT, .expect_state = {[VK_SHIFT] = 0x80, [VK_RSHIFT] = 0x80}, .todo_state = {[VK_RSHIFT] = TRUE, [VK_LSHIFT] = TRUE},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_RSHIFT, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_RSHIFT, .flags = KEYEVENTF_KEYUP, .expect_state = {0},
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RSHIFT, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYUP, 2, VK_SHIFT), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test lshift_ext[] =
    {
        {.vkey = VK_LSHIFT, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_SHIFT] = 0x80, [VK_RSHIFT] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_LSHIFT, LLKHF_EXTENDED), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_LSHIFT, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_LSHIFT, LLKHF_EXTENDED), KEY_MSG(WM_KEYUP, 2, VK_SHIFT), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test rshift_ext[] =
    {
        {.vkey = VK_RSHIFT, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_SHIFT] = 0x80, [VK_RSHIFT] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_RSHIFT, LLKHF_EXTENDED), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_RSHIFT, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, .expect_state = {0},
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RSHIFT, LLKHF_EXTENDED), KEY_MSG(WM_KEYUP, 2, VK_SHIFT), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test shift[] =
    {
        {.vkey = VK_SHIFT, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_LSHIFT), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_SHIFT, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_LSHIFT), KEY_MSG(WM_KEYUP, 2, VK_SHIFT), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test shift_ext[] =
    {
        {.vkey = VK_SHIFT, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_SHIFT] = 0x80, [VK_RSHIFT] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_LSHIFT, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_SHIFT, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_LSHIFT, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYUP, 2, VK_SHIFT), {0}}},
        {0},
    };

    static const struct send_input_keyboard_test rcontrol[] =
    {
        {.vkey = VK_RCONTROL, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_RCONTROL), KEY_MSG(WM_KEYDOWN, 1, VK_CONTROL), {0}}},
        {.vkey = VK_RCONTROL, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_RCONTROL), KEY_MSG(WM_KEYUP, 2, VK_CONTROL), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test lcontrol_ext[] =
    {
        {.vkey = VK_LCONTROL, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_CONTROL] = 0x80, [VK_RCONTROL] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_LCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYDOWN, 1, VK_CONTROL, KF_EXTENDED), {0}}},
        {.vkey = VK_LCONTROL, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_LCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYUP, 2, VK_CONTROL, KF_EXTENDED), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test rcontrol_ext[] =
    {
        {.vkey = VK_RCONTROL, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_CONTROL] = 0x80, [VK_RCONTROL] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_RCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYDOWN, 1, VK_CONTROL, KF_EXTENDED), {0}}},
        {.vkey = VK_RCONTROL, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYUP, 2, VK_CONTROL, KF_EXTENDED), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test control[] =
    {
        {.vkey = VK_CONTROL, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_LCONTROL), KEY_MSG(WM_KEYDOWN, 1, VK_CONTROL), {0}}},
        {.vkey = VK_CONTROL, .flags = KEYEVENTF_KEYUP, .expect_state = {0},
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_LCONTROL), KEY_MSG(WM_KEYUP, 2, VK_CONTROL), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test control_ext[] =
    {
        {.vkey = VK_CONTROL, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_CONTROL] = 0x80, [VK_RCONTROL] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 1, VK_RCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYDOWN, 1, VK_CONTROL, KF_EXTENDED), {0}}},
        {.vkey = VK_CONTROL, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RCONTROL, LLKHF_EXTENDED), KEY_MSG_(WM_KEYUP, 2, VK_CONTROL, KF_EXTENDED), {0}}},
        {0},
    };

    static const struct send_input_keyboard_test rmenu[] =
    {
        {.vkey = VK_RMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {.vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_RMENU, .todo_value = TRUE), KEY_MSG(WM_SYSKEYUP, 2, VK_MENU), WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test lmenu_ext[] =
    {
        {.vkey = VK_LMENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_LMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test rmenu_ext[] =
    {
        {.vkey = VK_RMENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test menu[] =
    {
        {.vkey = VK_MENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {.vkey = VK_MENU, .flags = KEYEVENTF_KEYUP, .expect_state = {0},
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_LMENU, .todo_value = TRUE), KEY_MSG(WM_SYSKEYUP, 2, VK_MENU), WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test menu_ext[] =
    {
        {.vkey = VK_MENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_MENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0), {0}}},
        {0},
    };

    static const struct send_input_keyboard_test rmenu_peeked[] =
    {
        {.vkey = VK_RMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {.vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_RMENU, .todo_value = TRUE), KEY_MSG(WM_SYSKEYUP, 2, VK_MENU), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test lmenu_ext_peeked[] =
    {
        {.vkey = VK_LMENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_LMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test rmenu_ext_peeked[] =
    {
        {.vkey = VK_RMENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test menu_peeked[] =
    {
        {.vkey = VK_MENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {.vkey = VK_MENU, .flags = KEYEVENTF_KEYUP, .expect_state = {0},
         .expect = {KEY_HOOK(WM_KEYUP, 2, VK_LMENU, .todo_value = TRUE), KEY_MSG(WM_SYSKEYUP, 2, VK_MENU), {0}}},
        {0},
    };
    static const struct send_input_keyboard_test menu_ext_peeked[] =
    {
        {.vkey = VK_MENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED), {0}}},
        {.vkey = VK_MENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
         .expect = {KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 2, VK_MENU, KF_EXTENDED), {0}}},
        {0},
    };

    struct send_input_keyboard_test rmenu_altgr[] =
    {
        {
            .vkey = VK_RMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 0x21d, VK_LCONTROL, LLKHF_ALTDOWN),
                KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN, .todo_value = TRUE),
                KEY_MSG(WM_KEYDOWN, 0x1d, VK_CONTROL),
                KEY_MSG_(WM_KEYDOWN, 1, VK_MENU, KF_ALTDOWN),
                {0}
            }
        },
        {
            .vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP,
            .expect =
            {
                KEY_HOOK(WM_KEYUP, 0x21d, VK_LCONTROL),
                KEY_HOOK(WM_KEYUP, 2, VK_RMENU),
                KEY_MSG_(WM_SYSKEYUP, 0x1d, VK_CONTROL, KF_ALTDOWN),
                KEY_MSG(WM_KEYUP, 2, VK_MENU),
                {0}
            }
        },
        {0},
    };
    struct send_input_keyboard_test rmenu_ext_altgr[] =
    {
        {
            .vkey = VK_RMENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 0x21d, VK_LCONTROL, LLKHF_ALTDOWN),
                KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE),
                KEY_MSG(WM_KEYDOWN, 0x1d, VK_CONTROL),
                KEY_MSG_(WM_KEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED),
                {0}
            }
        },
        {
            .vkey = VK_RMENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
            .expect =
            {
                KEY_HOOK(WM_KEYUP, 0x21d, VK_LCONTROL),
                KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYUP, 0x1d, VK_CONTROL, KF_ALTDOWN),
                KEY_MSG_(WM_KEYUP, 2, VK_MENU, KF_EXTENDED),
                {0}
            }
        },
        {0},
    };
    struct send_input_keyboard_test menu_ext_altgr[] =
    {
        {
            .vkey = VK_MENU, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_MENU] = 0x80, [VK_RMENU] = 0x80, [VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 0x21d, VK_LCONTROL, LLKHF_ALTDOWN),
                KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_RMENU, LLKHF_ALTDOWN|LLKHF_EXTENDED, .todo_value = TRUE),
                KEY_MSG(WM_KEYDOWN, 0x1d, VK_CONTROL),
                KEY_MSG_(WM_KEYDOWN, 1, VK_MENU, KF_ALTDOWN|KF_EXTENDED),
                {0}
            }
        },
        {
            .vkey = VK_MENU, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY,
            .expect =
            {
                KEY_HOOK(WM_KEYUP, 0x21d, VK_LCONTROL),
                KEY_HOOK_(WM_KEYUP, 2, VK_RMENU, LLKHF_EXTENDED, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYUP, 0x1d, VK_CONTROL, KF_ALTDOWN),
                KEY_MSG_(WM_KEYUP, 2, VK_MENU, KF_EXTENDED),
                {0}
            }
        },
        {0},
    };

    static const struct send_input_keyboard_test lrshift_ext[] =
    {
        {.vkey = VK_LSHIFT, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 1, VK_LSHIFT), KEY_MSG(WM_KEYDOWN, 1, VK_SHIFT), {0}}},
        {.vkey = VK_RSHIFT, .flags = KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80, [VK_RSHIFT] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 2, VK_RSHIFT, LLKHF_EXTENDED), KEY_MSG_(WM_KEYDOWN, 2, VK_SHIFT, KF_REPEAT, .todo_value = TRUE), {0}}},
        {.vkey = VK_RSHIFT, .flags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYUP, 3, VK_RSHIFT, LLKHF_EXTENDED), {0, .todo = TRUE}, {0}}},
        {.vkey = VK_LSHIFT, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LSHIFT), KEY_MSG(WM_KEYUP, 4, VK_SHIFT), {0}}},
        {0},
    };

    struct send_input_keyboard_test rshift_scan[] =
    {
        {.scan = 0x36, .flags = KEYEVENTF_SCANCODE, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0x36, VK_RSHIFT), KEY_MSG(WM_KEYDOWN, 0x36, VK_SHIFT), {0}}},
        {.scan = scan, .flags = KEYEVENTF_SCANCODE, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80, /*[vkey] = 0x80*/},
         .expect = {KEY_HOOK(WM_KEYDOWN, scan, vkey), KEY_MSG(WM_KEYDOWN, scan, vkey), WIN_MSG(WM_CHAR, wch_shift, MAKELONG(1, scan)), {0}}},
        {.scan = scan, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, .expect_state = {[VK_SHIFT] = 0x80, [VK_LSHIFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYUP, scan, vkey), KEY_MSG(WM_KEYUP, scan, vkey), {0}}},
        {.scan = 0x36, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0x36, VK_RSHIFT), KEY_MSG(WM_KEYUP, 0x36, VK_SHIFT), {0}}},
        {0},
    };

    struct send_input_keyboard_test rctrl_scan[] =
    {
        {.scan = 0xe01d, .flags = KEYEVENTF_SCANCODE, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0x1d, VK_LCONTROL), KEY_MSG(WM_KEYDOWN, 0x1d, VK_CONTROL), {0}}},
        {.scan = 0xe01d, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0x1d, VK_LCONTROL), KEY_MSG(WM_KEYUP, 0x1d, VK_CONTROL), {0}}},
        {.scan = 0x1d, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY, .expect_state = {[VK_CONTROL] = 0x80, [VK_RCONTROL] = 0x80},
         .expect = {KEY_HOOK_(WM_KEYDOWN, 0x1d, VK_RCONTROL, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYDOWN, 0x11d, VK_CONTROL), {0}}},
        {.scan = 0x1d, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK_(WM_KEYUP, 0x1d, VK_RCONTROL, LLKHF_EXTENDED, .todo_value = TRUE), KEY_MSG(WM_KEYUP, 0x11d, VK_CONTROL), {0}}},
        {0},
    };

    struct send_input_keyboard_test unicode[] =
    {
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_PACKET] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0x3c0, VK_PACKET), KEY_MSG(WM_KEYDOWN, 0, VK_PACKET, .todo_value = TRUE), WIN_MSG(WM_CHAR, 0x3c0, 1), {0}}},
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0x3c0, VK_PACKET, .todo_value = TRUE), KEY_MSG(WM_KEYUP, 0, VK_PACKET, .todo_value = TRUE), {0}}},
        {0},
    };

    struct send_input_keyboard_test lmenu_unicode[] =
    {
        {.vkey = VK_LMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {
            .scan = 0x3c0, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_PACKET] = 0x80},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 0x3c0, VK_PACKET, LLKHF_ALTDOWN, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYDOWN, 0, VK_PACKET, KF_ALTDOWN, .todo_value = TRUE),
                WIN_MSG(WM_SYSCHAR, 0x3c0, MAKELONG(1, KF_ALTDOWN), .todo_value = TRUE),
                WIN_MSG(WM_SYSCOMMAND, SC_KEYMENU, 0x3c0, .todo = TRUE),
                {0},
            },
        },
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYUP, 0x3c0, VK_PACKET, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 0, VK_PACKET, KF_ALTDOWN, .todo_value = TRUE), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LMENU), KEY_MSG(WM_KEYUP, 4, VK_MENU), {0}}},
        {0},
    };
    struct send_input_keyboard_test lmenu_unicode_peeked[] =
    {
        {.vkey = VK_LMENU, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYDOWN, 1, VK_LMENU, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYDOWN, 1, VK_MENU, KF_ALTDOWN), {0}}},
        {
            .scan = 0x3c0, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80, [VK_PACKET] = 0x80},
            .expect =
            {
                KEY_HOOK_(WM_SYSKEYDOWN, 0x3c0, VK_PACKET, LLKHF_ALTDOWN, .todo_value = TRUE),
                KEY_MSG_(WM_SYSKEYDOWN, 0, VK_PACKET, KF_ALTDOWN, .todo_value = TRUE),
                WIN_MSG(WM_SYSCHAR, 0x3c0, MAKELONG(1, KF_ALTDOWN), .todo_value = TRUE),
                {0},
            },
        },
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, .expect_state = {[VK_MENU] = 0x80, [VK_LMENU] = 0x80},
         .expect = {KEY_HOOK_(WM_SYSKEYUP, 0x3c0, VK_PACKET, LLKHF_ALTDOWN, .todo_value = TRUE), KEY_MSG_(WM_SYSKEYUP, 0, VK_PACKET, KF_ALTDOWN, .todo_value = TRUE), {0}}},
        {.vkey = VK_LMENU, .flags = KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 4, VK_LMENU), KEY_MSG(WM_KEYUP, 4, VK_MENU), {0}}},
        {0},
    };

    struct send_input_keyboard_test unicode_vkey[] =
    {
#if defined(__REACTOS__) && defined(_MSC_VER)
        {.scan = 0x3c0, .vkey = vkey, .flags = KEYEVENTF_UNICODE, .expect_state = { 0 /*[vkey] = 0x80*/},
#else
        {.scan = 0x3c0, .vkey = vkey, .flags = KEYEVENTF_UNICODE, .expect_state = {/*[vkey] = 0x80*/},
#endif
         .expect = {KEY_HOOK(WM_KEYDOWN, 0xc0, vkey), KEY_MSG(WM_KEYDOWN, 0xc0, vkey), WIN_MSG(WM_CHAR, wch, MAKELONG(1, 0xc0)), {0}}},
        {.scan = 0x3c0, .vkey = vkey, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0xc0, vkey), KEY_MSG(WM_KEYUP, 0xc0, vkey), {0}}},
        {0},
    };

    struct send_input_keyboard_test unicode_vkey_ctrl[] =
    {
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0xc0, VK_LCONTROL), KEY_MSG(WM_KEYDOWN, 0xc0, VK_CONTROL), {0}}},
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0xc0, VK_LCONTROL), KEY_MSG(WM_KEYUP, 0xc0, VK_CONTROL), {0}}},
        {0},
    };

    struct send_input_keyboard_test unicode_vkey_packet[] =
    {
        {.scan = 0x3c0, .vkey = VK_PACKET, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_PACKET] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0xc0, VK_PACKET), KEY_MSG(WM_KEYDOWN, 0, VK_PACKET, .todo_value = TRUE), WIN_MSG(WM_CHAR, 0xc0, 1), {0}}},
        {.scan = 0x3c0, .vkey = VK_PACKET, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0xc0, VK_PACKET), KEY_MSG(WM_KEYUP, 0, VK_PACKET, .todo_value = TRUE), {0}}},
        {0},
    };

    struct send_input_keyboard_test numpad_scan[] =
    {
        {.scan = 0x4b, .flags = KEYEVENTF_SCANCODE, .expect_state = {[VK_LEFT] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0x4b, VK_LEFT), KEY_MSG(WM_KEYDOWN, 0x4b, VK_LEFT), {0}}},
        {.scan = 0x4b, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP,
         .expect = {KEY_HOOK(WM_KEYUP, 0x4b, VK_LEFT), KEY_MSG(WM_KEYUP, 0x4b, VK_LEFT), {0}}},
        {0},
    };

    struct send_input_keyboard_test numpad_scan_numlock[] =
    {
        {.scan = 0x45, .flags = KEYEVENTF_SCANCODE, .expect_state = {[VK_NUMLOCK] = 0x80},
         .expect = {KEY_HOOK(WM_KEYDOWN, 0x45, VK_NUMLOCK), KEY_MSG(WM_KEYDOWN, 0x45, VK_NUMLOCK), {0}}},
        {.scan = 0x45, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, .expect_state = {[VK_NUMLOCK] = 0x01},
         .expect = {KEY_HOOK(WM_KEYUP, 0x45, VK_NUMLOCK), KEY_MSG(WM_KEYUP, 0x45, VK_NUMLOCK), {0}}},
        {
            .scan = 0x4b, .flags = KEYEVENTF_SCANCODE,
            .expect_state = {[VK_NUMPAD4] = 0x80, [VK_NUMLOCK] = 0x01},
            .todo_state = {[VK_NUMPAD4] = TRUE, [VK_LEFT] = TRUE},
            .expect =
            {
                KEY_HOOK(WM_KEYDOWN, 0x4b, VK_NUMPAD4, .todo_value = TRUE),
                KEY_MSG(WM_KEYDOWN, 0x4b, VK_NUMPAD4, .todo_value = TRUE),
                WIN_MSG(WM_CHAR, '4', MAKELONG(1, 0x4b), .todo_value = TRUE),
                {0}
            }
        },
        {
            .scan = 0x4b, .flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, .expect_state = {[VK_NUMLOCK] = 0x01},
            .expect =
            {
                KEY_HOOK(WM_KEYUP, 0x4b, VK_NUMPAD4, .todo_value = TRUE),
                KEY_MSG(WM_KEYUP, 0x4b, VK_NUMPAD4, .todo_value = TRUE),
                {0}
            }
        },
        {0},
    };

#undef WIN_MSG
#undef KBD_HOOK
#undef KEY_HOOK_
#undef KEY_HOOK
#undef KEY_MSG_
#undef KEY_MSG

    BOOL altgr = keyboard_layout_has_altgr(), skip_altgr = FALSE;
    LONG_PTR old_proc;
    HHOOK hook;
    HWND hwnd;

    /* on 32-bit with ALTGR keyboard, the CONTROL key is sent to the hooks without the
     * LLKHF_INJECTED flag, skip the tests to keep it simple */
    if (altgr && sizeof(void *) == 4 && !is_wow64) skip_altgr = TRUE;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );

    /* If we have had a spurious layout change, wch(_shift) may be incorrect. */
    if (GetKeyboardLayout( 0 ) != hkl)
    {
        win_skip( "Spurious keyboard layout changed detected (expected: %p got: %p)\n",
                  hkl, GetKeyboardLayout( 0 ) );
        ok_ret( 1, DestroyWindow( hwnd ) );
        wait_messages( 100, FALSE );
        ok_seq( empty_sequence );
        return;
    }

    hook = SetWindowsHookExW( WH_KEYBOARD_LL, ll_hook_kbd_proc, GetModuleHandleW( NULL ), 0 );
    ok_ne( NULL, hook, HHOOK, "%p" );

    p_accept_message = accept_keyboard_messages_syscommand;
    ok_seq( empty_sequence );

    lmenu_vkey_peeked[1].expect_state[vkey] = 0x80;
    lmenu_vkey[1].expect_state[vkey] = 0x80;
    lcontrol_vkey[1].expect_state[vkey] = 0x80;
    lmenu_lcontrol_vkey[2].expect_state[vkey] = 0x80;
    shift_vkey[1].expect_state[vkey] = 0x80;
    rshift_scan[1].expect_state[vkey] = 0x80;
    unicode_vkey[0].expect_state[vkey] = 0x80;

    /* test peeked messages */
    winetest_push_context( "peek" );
    check_send_input_keyboard_test( lmenu_vkey_peeked, TRUE );
    check_send_input_keyboard_test( lcontrol_vkey, TRUE );
    check_send_input_keyboard_test( lmenu_lcontrol_vkey, TRUE );
    check_send_input_keyboard_test( shift_vkey, TRUE );
    check_send_input_keyboard_test( rshift, TRUE );
    check_send_input_keyboard_test( lshift_ext, TRUE );
    check_send_input_keyboard_test( rshift_ext, TRUE );
    check_send_input_keyboard_test( shift, TRUE );
    check_send_input_keyboard_test( shift_ext, TRUE );
    check_send_input_keyboard_test( rcontrol, TRUE );
    check_send_input_keyboard_test( lcontrol_ext, TRUE );
    check_send_input_keyboard_test( rcontrol_ext, TRUE );
    check_send_input_keyboard_test( control, TRUE );
    check_send_input_keyboard_test( control_ext, TRUE );
    if (skip_altgr) skip( "skipping rmenu_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( rmenu_altgr, TRUE );
    else check_send_input_keyboard_test( rmenu_peeked, TRUE );
    check_send_input_keyboard_test( lmenu_ext_peeked, TRUE );
    if (skip_altgr) skip( "skipping rmenu_ext_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( rmenu_ext_altgr, TRUE );
    else check_send_input_keyboard_test( rmenu_ext_peeked, TRUE );
    check_send_input_keyboard_test( menu_peeked, TRUE );
    if (skip_altgr) skip( "skipping menu_ext_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( menu_ext_altgr, TRUE );
    else check_send_input_keyboard_test( menu_ext_peeked, TRUE );
    check_send_input_keyboard_test( lrshift_ext, TRUE );
    check_send_input_keyboard_test( rshift_scan, TRUE );
    /* Skip on Korean layouts since they map the right control key to VK_HANJA */
    if (hkl == (HKL)0x04120412) skip( "skipping rctrl_scan test on Korean layout" );
    else check_send_input_keyboard_test( rctrl_scan, TRUE );
    check_send_input_keyboard_test( unicode, TRUE );
    check_send_input_keyboard_test( lmenu_unicode_peeked, TRUE );
    check_send_input_keyboard_test( unicode_vkey, TRUE );
    check_send_input_keyboard_test( unicode_vkey_ctrl, TRUE );
    check_send_input_keyboard_test( unicode_vkey_packet, TRUE );
    check_send_input_keyboard_test( numpad_scan, TRUE );
    check_send_input_keyboard_test( numpad_scan_numlock, TRUE );
    winetest_pop_context();

    wait_messages( 100, FALSE );
    ok_seq( empty_sequence );

    /* test received messages */
    old_proc = SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_proc, LONG_PTR, "%#Ix" );

    winetest_push_context( "receive" );
    check_send_input_keyboard_test( lmenu_vkey, FALSE );
    check_send_input_keyboard_test( lcontrol_vkey, FALSE );
    check_send_input_keyboard_test( lmenu_lcontrol_vkey, FALSE );
    check_send_input_keyboard_test( shift_vkey, FALSE );
    check_send_input_keyboard_test( rshift, FALSE );
    check_send_input_keyboard_test( lshift_ext, FALSE );
    check_send_input_keyboard_test( rshift_ext, FALSE );
    check_send_input_keyboard_test( shift, FALSE );
    check_send_input_keyboard_test( shift_ext, FALSE );
    check_send_input_keyboard_test( rcontrol, FALSE );
    check_send_input_keyboard_test( lcontrol_ext, FALSE );
    check_send_input_keyboard_test( rcontrol_ext, FALSE );
    check_send_input_keyboard_test( control, FALSE );
    check_send_input_keyboard_test( control_ext, FALSE );
    if (skip_altgr) skip( "skipping rmenu_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( rmenu_altgr, FALSE );
    else check_send_input_keyboard_test( rmenu, FALSE );
    check_send_input_keyboard_test( lmenu_ext, FALSE );
    if (skip_altgr) skip( "skipping rmenu_ext_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( rmenu_ext_altgr, FALSE );
    else check_send_input_keyboard_test( rmenu_ext, FALSE );
    check_send_input_keyboard_test( menu, FALSE );
    if (skip_altgr) skip( "skipping menu_ext_altgr test\n" );
    else if (altgr) check_send_input_keyboard_test( menu_ext_altgr, FALSE );
    else check_send_input_keyboard_test( menu_ext, FALSE );
    check_send_input_keyboard_test( lrshift_ext, FALSE );
    check_send_input_keyboard_test( rshift_scan, FALSE );
    /* Skip on Korean layouts since they map the right control key to VK_HANJA */
    if (hkl == (HKL)0x04120412) skip( "skipping rctrl_scan test on Korean layout" );
    else check_send_input_keyboard_test( rctrl_scan, FALSE );
    check_send_input_keyboard_test( unicode, FALSE );
    check_send_input_keyboard_test( lmenu_unicode, FALSE );
    check_send_input_keyboard_test( unicode_vkey, FALSE );
    check_send_input_keyboard_test( unicode_vkey_ctrl, FALSE );
    check_send_input_keyboard_test( unicode_vkey_packet, FALSE );
    check_send_input_keyboard_test( numpad_scan, FALSE );
    check_send_input_keyboard_test( numpad_scan_numlock, FALSE );
    winetest_pop_context();

    ok_ret( 1, DestroyWindow( hwnd ) );
    ok_ret( 1, UnhookWindowsHookEx( hook ) );

    wait_messages( 100, FALSE );
    ok_seq( empty_sequence );
    p_accept_message = NULL;
}

static void test_keynames(void)
{
    int i, len;
    char buff[256];

    for (i = 0; i < 512; i++)
    {
        strcpy(buff, "----");
        len = GetKeyNameTextA(i << 16, buff, sizeof(buff));
        ok(len || !buff[0], "%d: Buffer is not zeroed\n", i);
    }
}

static BOOL accept_keyboard_messages_raw( UINT msg )
{
    return is_keyboard_message( msg ) || msg == WM_INPUT;
}

static void test_SendInput_raw_key_messages( WORD vkey, WORD wch, HKL hkl )
{
#define WIN_MSG(m, w, l, ...) {.func = MSG_TEST_WIN, .message = {.msg = m, .wparam = w, .lparam = l}, ## __VA_ARGS__}
#define RAW_KEY(s, f, v, m, ...) {.func = RAW_INPUT_KEYBOARD, .raw_input.kbd = {.MakeCode = s, .Flags = f, .VKey = v, .Message = m}, ## __VA_ARGS__}
#define KEY_MSG(m, s, v,  ...) WIN_MSG( m, v, MAKELONG(1, (s) | (m == WM_KEYUP || m == WM_SYSKEYUP ? (KF_UP | KF_REPEAT) : 0)), ## __VA_ARGS__ )
    struct send_input_keyboard_test raw_legacy[] =
    {
        {.vkey = vkey,
         .expect = {RAW_KEY(1, RI_KEY_MAKE, vkey, WM_KEYDOWN), KEY_MSG(WM_KEYDOWN, 1, vkey), WIN_MSG(WM_CHAR, wch, MAKELONG(1, 1)), {0}}},
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP,
         .expect = {RAW_KEY(2, RI_KEY_BREAK, vkey, WM_KEYUP), KEY_MSG(WM_KEYUP, 2, vkey), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_nolegacy[] =
    {
        {.vkey = vkey, .async = TRUE, .expect = {RAW_KEY(1, RI_KEY_MAKE, vkey, WM_KEYDOWN), {0}}},
        {.vkey = vkey, .flags = KEYEVENTF_KEYUP, .expect = {RAW_KEY(2, RI_KEY_BREAK, vkey, WM_KEYUP), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_vk_packet_legacy[] =
    {
        {.vkey = VK_PACKET, .expect_state = {[VK_PACKET] = 0x80},
         .expect = {RAW_KEY(1, RI_KEY_MAKE, VK_PACKET, WM_KEYDOWN), KEY_MSG(WM_KEYDOWN, 1, VK_PACKET), {0, .todo = TRUE}}},
        {.vkey = VK_PACKET, .flags = KEYEVENTF_KEYUP,
         .expect = {RAW_KEY(2, RI_KEY_BREAK, VK_PACKET, WM_KEYUP), KEY_MSG(WM_KEYUP, 2, VK_PACKET), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_vk_packet_nolegacy[] =
    {
        {.vkey = VK_PACKET, .async = TRUE, .expect_async = {[VK_PACKET] = 0x80},
         .expect = {RAW_KEY(1, RI_KEY_MAKE, VK_PACKET, WM_KEYDOWN), {0}}},
        {.vkey = VK_PACKET, .flags = KEYEVENTF_KEYUP, .expect = {RAW_KEY(2, RI_KEY_BREAK, VK_PACKET, WM_KEYUP), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_unicode_legacy[] =
    {
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE, .expect_state = {[VK_PACKET] = 0x80},
         .expect = {KEY_MSG(WM_KEYDOWN, 0, VK_PACKET, .todo_value = TRUE), WIN_MSG(WM_CHAR, 0x3c0, 1), {0}}},
        {.scan = 0x3c0, .flags = KEYEVENTF_KEYUP | KEYEVENTF_UNICODE,
         .expect = {KEY_MSG(WM_KEYUP, 0, VK_PACKET, .todo_value = TRUE), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_unicode_nolegacy[] =
    {
        {.scan = 0x3c0, .flags = KEYEVENTF_UNICODE, .async = TRUE, .expect_async = {[VK_PACKET] = 0x80}},
        {.scan = 0x3c0, .flags = KEYEVENTF_KEYUP | KEYEVENTF_UNICODE},
        {0},
    };
    struct send_input_keyboard_test raw_unicode_vkey_ctrl_legacy[] =
    {
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE,
         .expect_state = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80},
         .expect = {KEY_MSG(WM_KEYDOWN, 0xc0, VK_CONTROL), {0}}},
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP,
         .expect = {KEY_MSG(WM_KEYUP, 0xc0, VK_CONTROL), {0}}},
        {0},
    };
    struct send_input_keyboard_test raw_unicode_vkey_ctrl_nolegacy[] =
    {
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE, .async = TRUE,
         .expect_async = {[VK_CONTROL] = 0x80, [VK_LCONTROL] = 0x80}},
        {.scan = 0x3c0, .vkey = VK_CONTROL, .flags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP},
        {0},
    };
#undef WIN_MSG
#undef RAW_KEY
#undef KEY_MSG
    RAWINPUTDEVICE rid = {.usUsagePage = HID_USAGE_PAGE_GENERIC, .usUsage = HID_USAGE_GENERIC_KEYBOARD};
    int receive;
    HWND hwnd;

    raw_legacy[0].expect_state[vkey] = 0x80;
    raw_nolegacy[0].expect_async[vkey] = 0x80;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );

    /* If we have had a spurious layout change, wch may be incorrect. */
    if (GetKeyboardLayout( 0 ) != hkl)
    {
        win_skip( "Spurious keyboard layout changed detected (expected: %p got: %p)\n",
                  hkl, GetKeyboardLayout( 0 ) );
        ok_ret( 1, DestroyWindow( hwnd ) );
        wait_messages( 100, FALSE );
        ok_seq( empty_sequence );
        return;
    }

    p_accept_message = accept_keyboard_messages_raw;

    for (receive = 0; receive <= 1; receive++)
    {
        winetest_push_context( receive ? "receive" : "peek" );

        if (receive)
        {
            /* test received messages */
            LONG_PTR old_proc = SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
            ok_ne( 0, old_proc, LONG_PTR, "%#Ix" );
        }

        rid.dwFlags = 0;
        ok_ret( 1, RegisterRawInputDevices( &rid, 1, sizeof(rid) ) );

        /* get both WM_INPUT and legacy messages */
        check_send_input_keyboard_test( raw_legacy, !receive );
        check_send_input_keyboard_test( raw_vk_packet_legacy, !receive );
        /* no WM_INPUT message for unicode */
        check_send_input_keyboard_test( raw_unicode_legacy, !receive );
        check_send_input_keyboard_test( raw_unicode_vkey_ctrl_legacy, !receive );

        rid.dwFlags = RIDEV_REMOVE;
        ok_ret( 1, RegisterRawInputDevices( &rid, 1, sizeof(rid) ) );

        rid.dwFlags = RIDEV_NOLEGACY;
        ok_ret( 1, RegisterRawInputDevices( &rid, 1, sizeof(rid) ) );

        /* get only WM_INPUT messages */
        check_send_input_keyboard_test( raw_nolegacy, !receive );
        check_send_input_keyboard_test( raw_vk_packet_nolegacy, !receive );
        /* no WM_INPUT message for unicode */
        check_send_input_keyboard_test( raw_unicode_nolegacy, !receive );
        check_send_input_keyboard_test( raw_unicode_vkey_ctrl_nolegacy, !receive );

        rid.dwFlags = RIDEV_REMOVE;
        ok_ret( 1, RegisterRawInputDevices( &rid, 1, sizeof(rid) ) );

        winetest_pop_context();
    }

    ok_ret( 1, DestroyWindow( hwnd ) );
    wait_messages( 100, FALSE );
    ok_seq( empty_sequence );

    p_accept_message = NULL;
}

static void test_GetMouseMovePointsEx( char **argv )
{
#define BUFLIM  64
#define MYERROR 0xdeadbeef
    int i, count, retval;
    MOUSEMOVEPOINT in;
    MOUSEMOVEPOINT out[200];
    POINT point;
    INPUT input;

    /* Get a valid content for the input struct */
    if(!GetCursorPos(&point)) {
        win_skip("GetCursorPos() failed with error %lu\n", GetLastError());
        return;
    }
    memset(&in, 0, sizeof(MOUSEMOVEPOINT));
    in.x = point.x;
    in.y = point.y;

    /* test first parameter
     * everything different than sizeof(MOUSEMOVEPOINT)
     * is expected to fail with ERROR_INVALID_PARAMETER
     */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(0, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    if (retval == ERROR_INVALID_PARAMETER)
    {
        win_skip( "GetMouseMovePointsEx broken on WinME\n" );
        return;
    }
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)+1, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* test second and third parameter
     */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_NOACCESS || GetLastError() == MYERROR,
       "expected error ERROR_NOACCESS, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(ERROR_NOACCESS == GetLastError(),
       "expected error ERROR_NOACCESS, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(ERROR_NOACCESS == GetLastError(),
       "expected error ERROR_NOACCESS, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    count = 0;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %lu\n", GetLastError());
    else
        ok(retval == count, "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    /* test fourth parameter
     * a value higher than 64 is expected to fail with ERROR_INVALID_PARAMETER
     */
    SetLastError(MYERROR);
    count = -1;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    ok(retval == count, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    count = 0;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %lu\n", GetLastError());
    else
        ok(retval == count, "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    SetLastError(MYERROR);
    count = BUFLIM;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %lu\n", GetLastError());
    else
        ok((0 <= retval) && (retval <= count), "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* it was not possible to force an error with the fifth parameter on win2k */

    /* test combinations of wrong parameters to see which error wins */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, NULL, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, &in, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, out, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    /* more than 64 to be sure we wrap around */
    for (i = 0; i < 67; i++)
    {
        in.x = i;
        in.y = i*2;
        SetCursorPos( in.x, in.y );
    }

    SetLastError( MYERROR );
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( GetLastError() == MYERROR, "expected error to stay %x, got %lx\n", MYERROR, GetLastError() );

    for (i = 0; i < retval; i++)
    {
        ok( out[i].x == in.x && out[i].y == in.y, "wrong position %d, expected %dx%d got %dx%d\n", i, in.x, in.y, out[i].x, out[i].y );
        in.x--;
        in.y -= 2;
    }

    in.x = 1500;
    in.y = 1500;
    SetLastError( MYERROR );
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == -1, "expected to get -1 but got %d\n", retval );
    ok( GetLastError() == ERROR_POINT_NOT_FOUND, "expected error to be set to %x, got %lx\n", ERROR_POINT_NOT_FOUND, GetLastError() );

    /* make sure there's no deduplication */
    in.x = 6;
    in.y = 6;
    SetCursorPos( in.x, in.y );
    SetCursorPos( in.x, in.y );
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( out[0].x == 6 && out[0].y == 6, "expected cursor position to be 6x6 but got %d %d\n", out[0].x, out[0].y );
    ok( out[1].x == 6 && out[1].y == 6, "expected cursor position to be 6x6 but got %d %d\n", out[1].x, out[1].y );

    /* make sure 2 events are distinguishable by their timestamps */
    in.x = 150;
    in.y = 75;
    SetCursorPos( 30, 30 );
    SetCursorPos( in.x, in.y );
    SetCursorPos( 150, 150 );
    Sleep( 3 );
    SetCursorPos( in.x, in.y );

    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( out[0].x == 150 && out[0].y == 75, "expected cursor position to be 150x75 but got %d %d\n", out[0].x, out[0].y );
    ok( out[1].x == 150 && out[1].y == 150, "expected cursor position to be 150x150 but got %d %d\n", out[1].x, out[1].y );
    ok( out[2].x == 150 && out[2].y == 75, "expected cursor position to be 150x75 but got %d %d\n", out[2].x, out[2].y );

    in.time = out[2].time;
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 62, "expected to get 62 mouse move points but got %d\n", retval );
    ok( out[0].x == 150 && out[0].y == 75, "expected cursor position to be 150x75 but got %d %d\n", out[0].x, out[0].y );
    ok( out[1].x == 30 && out[1].y == 30, "expected cursor position to be 30x30 but got %d %d\n", out[1].x, out[1].y );

    /* events created through other means should also be on the list with correct extra info */
    mouse_event( MOUSEEVENTF_MOVE, -13, 17, 0, 0xcafecafe );
    ok( GetCursorPos( &point ), "failed to get cursor position\n" );
    ok( in.x != point.x && in.y != point.y, "cursor didn't change position after mouse_event()\n" );
    in.time = 0;
    in.x = point.x;
    in.y = point.y;
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( out[0].dwExtraInfo == 0xcafecafe, "wrong extra info, got 0x%Ix expected 0xcafecafe\n", out[0].dwExtraInfo );

    input.type = INPUT_MOUSE;
    memset( &input, 0, sizeof(input) );
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dwExtraInfo = 0xdeadbeef;
    input.mi.dx = -17;
    input.mi.dy = 13;
    SendInput( 1, (INPUT *)&input, sizeof(INPUT) );
    ok( GetCursorPos( &point ), "failed to get cursor position\n" );
    ok( in.x != point.x && in.y != point.y, "cursor didn't change position after mouse_event()\n" );
    in.time = 0;
    in.x = point.x;
    in.y = point.y;
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( out[0].dwExtraInfo == 0xdeadbeef, "wrong extra info, got 0x%Ix expected 0xdeadbeef\n", out[0].dwExtraInfo );

    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM, GMMP_USE_HIGH_RESOLUTION_POINTS );
    todo_wine ok( retval == 64, "expected to get 64 high resolution mouse move points but got %d\n", retval );

    run_in_process( argv, "test_GetMouseMovePointsEx_process" );
#undef BUFLIM
#undef MYERROR
}

static void test_GetMouseMovePointsEx_process(void)
{
    int retval;
    MOUSEMOVEPOINT in;
    MOUSEMOVEPOINT out[64], out2[64];
    POINT point;
    HDESK desk0, desk1;
    HWINSTA winstation0, winstation1;

    memset( out, 0, sizeof(out) );
    memset( out2, 0, sizeof(out2) );

    /* move point history is shared between desktops within the same windowstation */
    ok( GetCursorPos( &point ), "failed to get cursor position\n" );
    in.time = 0;
    in.x = point.x;
    in.y = point.y;
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, ARRAY_SIZE(out), GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );

    desk0 = OpenInputDesktop( 0, FALSE, DESKTOP_ALL_ACCESS );
    ok( desk0 != NULL, "OpenInputDesktop has failed with %ld\n", GetLastError() );
    desk1 = CreateDesktopA( "getmousemovepointsex_test_desktop", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( desk1 != NULL, "CreateDesktopA failed with %ld\n", GetLastError() );

    ok( SetThreadDesktop( desk1 ), "SetThreadDesktop failed!\n" );
    ok( SwitchDesktop( desk1 ), "SwitchDesktop failed\n" );

    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out2, ARRAY_SIZE(out2), GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );

    ok( memcmp( out, out2, sizeof(out2) ) == 0, "expected to get exact same history on the new desktop\n" );

    in.time = 0;
    in.x = 38;
    in.y = 27;
    SetCursorPos( in.x, in.y );

    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out2, ARRAY_SIZE(out2), GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );

    ok( SetThreadDesktop( desk0 ), "SetThreadDesktop failed!\n" );
    ok( SwitchDesktop( desk0 ), "SwitchDesktop failed\n" );

    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, ARRAY_SIZE(out), GMMP_USE_DISPLAY_POINTS );
    ok( retval == 64, "expected to get 64 mouse move points but got %d\n", retval );
    ok( memcmp( out, out2, sizeof( out2 ) ) == 0, "expected to get exact same history on the old desktop\n" );

    CloseDesktop( desk1 );
    CloseDesktop( desk0 );

    /* non-default windowstations are non-interactive */
    winstation0 = GetProcessWindowStation();
    ok( winstation0 != NULL, "GetProcessWindowStation has failed with %ld\n", GetLastError() );
    desk0 = OpenInputDesktop( 0, FALSE, DESKTOP_ALL_ACCESS );
    ok( desk0 != NULL, "OpenInputDesktop has failed with %ld\n", GetLastError() );
    winstation1 = CreateWindowStationA( "test_winstation", 0, WINSTA_ALL_ACCESS, NULL );

    if (winstation1 == NULL && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip("not enough privileges for CreateWindowStation\n");
        CloseDesktop( desk0 );
        CloseWindowStation( winstation0 );
        return;
    }

    ok( winstation1 != NULL, "CreateWindowStationA has failed with %ld\n", GetLastError() );
    ok( SetProcessWindowStation( winstation1 ), "SetProcessWindowStation has failed\n" );

    desk1 = CreateDesktopA( "getmousemovepointsex_test_desktop", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( desk1 != NULL, "CreateDesktopA failed with %ld\n", GetLastError() );
    ok( SetThreadDesktop( desk1 ), "SetThreadDesktop failed!\n" );

    SetLastError( 0xDEADBEEF );
    retval = pGetMouseMovePointsEx( sizeof(MOUSEMOVEPOINT), &in, out, ARRAY_SIZE(out), GMMP_USE_DISPLAY_POINTS );
    todo_wine ok( retval == -1, "expected to get -1 mouse move points but got %d\n", retval );
    todo_wine ok( GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED got %ld\n", GetLastError() );

    ok( SetProcessWindowStation( winstation0 ), "SetProcessWindowStation has failed\n" );
    ok( SetThreadDesktop( desk0 ), "SetThreadDesktop failed!\n" );
    CloseDesktop( desk1 );
    CloseWindowStation( winstation1 );
    CloseDesktop( desk0 );
    CloseWindowStation( winstation0 );
}

static void test_GetRawInputDeviceList(void)
{
    RAWINPUTDEVICELIST devices[32];
    UINT ret, oret, devcount, odevcount, i;
    DWORD err;
    BOOLEAN br;

    SetLastError(0xdeadbeef);
    ret = pGetRawInputDeviceList(NULL, NULL, 0);
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_INVALID_PARAMETER, "expected 87, got %ld\n", err);

    SetLastError(0xdeadbeef);
    ret = pGetRawInputDeviceList(NULL, NULL, sizeof(devices[0]));
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_NOACCESS, "expected 998, got %ld\n", err);

    devcount = 0;
    ret = pGetRawInputDeviceList(NULL, &devcount, sizeof(devices[0]));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(devcount > 0, "expected non-zero\n");

    SetLastError(0xdeadbeef);
    devcount = 0;
    ret = pGetRawInputDeviceList(devices, &devcount, sizeof(devices[0]));
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_INSUFFICIENT_BUFFER, "expected 122, got %ld\n", err);
    ok(devcount > 0, "expected non-zero\n");

    /* devcount contains now the correct number of devices */
    ret = pGetRawInputDeviceList(devices, &devcount, sizeof(devices[0]));
    ok(ret > 0, "expected non-zero\n");

    if (devcount)
    {
        RID_DEVICE_INFO info;
        UINT size;

        SetLastError( 0xdeadbeef );
        ret = pGetRawInputDeviceInfoW( UlongToHandle( 0xdeadbeef ), RIDI_DEVICEINFO, NULL, NULL );
        ok( ret == ~0U, "GetRawInputDeviceInfoW returned %#x, expected ~0.\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "GetRawInputDeviceInfoW last error %#lx, expected 0xdeadbeef.\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        size = 0xdeadbeef;
        ret = pGetRawInputDeviceInfoW( UlongToHandle( 0xdeadbeef ), RIDI_DEVICEINFO, NULL, &size );
        ok( ret == ~0U, "GetRawInputDeviceInfoW returned %#x, expected ~0.\n", ret );
        ok( size == 0xdeadbeef, "GetRawInputDeviceInfoW returned size %#x, expected 0.\n", size );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "GetRawInputDeviceInfoW last error %#lx, expected 0xdeadbeef.\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        size = 0xdeadbeef;
        ret = pGetRawInputDeviceInfoW( devices[0].hDevice, 0xdeadbeef, NULL, &size );
        ok( ret == ~0U, "GetRawInputDeviceInfoW returned %#x, expected ~0.\n", ret );
        ok( size == 0xdeadbeef, "GetRawInputDeviceInfoW returned size %#x, expected 0.\n", size );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "GetRawInputDeviceInfoW last error %#lx, expected 0xdeadbeef.\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pGetRawInputDeviceInfoW( devices[0].hDevice, RIDI_DEVICEINFO, &info, NULL );
        ok( ret == ~0U, "GetRawInputDeviceInfoW returned %#x, expected ~0.\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "GetRawInputDeviceInfoW last error %#lx, expected 0xdeadbeef.\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        size = 0;
        ret = pGetRawInputDeviceInfoW( devices[0].hDevice, RIDI_DEVICEINFO, &info, &size );
        ok( ret == ~0U, "GetRawInputDeviceInfoW returned %#x, expected ~0.\n", ret );
        ok( size == sizeof(info), "GetRawInputDeviceInfoW returned size %#x, expected %#Ix.\n", size, sizeof(info) );
        ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetRawInputDeviceInfoW last error %#lx, expected 0xdeadbeef.\n", GetLastError() );
    }

    for(i = 0; i < devcount; ++i)
    {
        WCHAR name[128];
        char nameA[128];
        UINT sz, len;
        RID_DEVICE_INFO info;
        HANDLE file;
        char *ppd;

        /* get required buffer size */
        name[0] = '\0';
        sz = 5;
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, name, &sz);
        ok(ret == -1, "GetRawInputDeviceInfo gave wrong failure: %ld\n", err);
        ok(sz > 5 && sz < ARRAY_SIZE(name), "Size should have been set and not too large (got: %u)\n", sz);

        /* buffer size for RIDI_DEVICENAME is in CHARs, not BYTEs */
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, name, &sz);
        ok(ret == sz, "GetRawInputDeviceInfo gave wrong return: %ld\n", err);
        len = lstrlenW(name);
        ok(len + 1 == ret, "GetRawInputDeviceInfo returned wrong length (name: %u, ret: %u)\n", len + 1, ret);

        /* test A variant with same size */
        ret = pGetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICENAME, nameA, &sz);
        ok(ret == sz, "GetRawInputDeviceInfoA gave wrong return: %ld\n", err);
        len = strlen(nameA);
        ok(len + 1 == ret, "GetRawInputDeviceInfoA returned wrong length (name: %u, ret: %u)\n", len + 1, ret);

        /* buffer size for RIDI_DEVICEINFO is in BYTEs */
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        sz = sizeof(info) - 1;
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == -1, "GetRawInputDeviceInfo gave wrong failure: %ld\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");

        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == sizeof(info), "GetRawInputDeviceInfo gave wrong return: %ld\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");
        ok(info.dwType == devices[i].dwType, "GetRawInputDeviceInfo set wrong type: 0x%lx\n", info.dwType);

        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        ret = pGetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == sizeof(info), "GetRawInputDeviceInfo gave wrong return: %ld\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");
        ok(info.dwType == devices[i].dwType, "GetRawInputDeviceInfo set wrong type: 0x%lx\n", info.dwType);

        /* setupapi returns an NT device path, but CreateFile() < Vista can't
         * understand that; so use the \\?\ prefix instead */
        name[1] = '\\';
        file = CreateFileW(name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu\n", wine_dbgstr_w(name), GetLastError());

        sz = 0;
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_PREPARSEDDATA, NULL, &sz);
        ok(ret == 0, "GetRawInputDeviceInfo gave wrong return: %u\n", ret);
        ok((info.dwType == RIM_TYPEHID && sz != 0) ||
                (info.dwType != RIM_TYPEHID && sz == 0),
                "Got wrong PPD size for type 0x%lx: %u\n", info.dwType, sz);

        ppd = malloc(sz);
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_PREPARSEDDATA, ppd, &sz);
        ok(ret == sz, "GetRawInputDeviceInfo gave wrong return: %u, should be %u\n", ret, sz);

        if (file != INVALID_HANDLE_VALUE && ret == sz)
        {
            PHIDP_PREPARSED_DATA preparsed;

            if (info.dwType == RIM_TYPEHID)
            {
                br = HidD_GetPreparsedData(file, &preparsed);
                ok(br == TRUE, "HidD_GetPreparsedData failed\n");

                if (br)
                    ok(!memcmp(preparsed, ppd, sz), "Expected to get same preparsed data\n");
            }
            else
            {
                /* succeeds on hardware, fails in some VMs */
                br = HidD_GetPreparsedData(file, &preparsed);
                ok(br == TRUE || broken(br == FALSE), "HidD_GetPreparsedData failed\n");
            }

            if (br)
                HidD_FreePreparsedData(preparsed);
        }

        free(ppd);

        CloseHandle(file);
    }

    /* check if variable changes from larger to smaller value */
    devcount = odevcount = ARRAY_SIZE(devices);
    oret = ret = pGetRawInputDeviceList(devices, &odevcount, sizeof(devices[0]));
    ok(ret > 0, "expected non-zero\n");
    ok(devcount == odevcount, "expected %d, got %d\n", devcount, odevcount);
    devcount = odevcount;
    odevcount = ARRAY_SIZE(devices);
    ret = pGetRawInputDeviceList(NULL, &odevcount, sizeof(devices[0]));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(odevcount == oret, "expected %d, got %d\n", oret, odevcount);
}

static void test_GetRawInputData(void)
{
    UINT size = 0;

    /* Null raw input handle */
    SetLastError( 0xdeadbeef );
    ok_ret( (UINT)-1, GetRawInputData( NULL, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_ret( ERROR_INVALID_HANDLE, GetLastError() );
}

static void test_RegisterRawInputDevices(void)
{
    HWND hwnd;
    RAWINPUTDEVICE raw_devices[2] = {0};
    UINT count, raw_devices_count;
    MSG msg;

    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    raw_devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;

    hwnd = create_foreground_window( FALSE );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, RegisterRawInputDevices( NULL, 0, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );

    SetLastError( 0xdeadbeef );
    ok_ret( (UINT)-1, GetRegisteredRawInputDevices( NULL, NULL, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok_ret( (UINT)-1, GetRegisteredRawInputDevices( NULL, &raw_devices_count, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    raw_devices_count = 0;
    ok_ret( 0, GetRegisteredRawInputDevices( NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE) ) );
    ok_eq( 2, raw_devices_count, UINT, "%u" );

    SetLastError( 0xdeadbeef );
    raw_devices_count = 0;
    count = GetRegisteredRawInputDevices( raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE) );
    if (broken(count == 0) /* depends on windows versions */)
        win_skip( "Ignoring GetRegisteredRawInputDevices success\n" );
    else
    {
        ok_eq( -1, count, int, "%d" );
        ok_eq( 0, raw_devices_count, UINT, "%u" );
        ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    raw_devices_count = 1;
    ok_ret( (UINT)-1, GetRegisteredRawInputDevices( raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE) ) );
    ok_eq( 2, raw_devices_count, UINT, "%u" );
    ok_ret( ERROR_INSUFFICIENT_BUFFER, GetLastError() );

    memset( raw_devices, 0, sizeof(raw_devices) );
    raw_devices_count = ARRAY_SIZE(raw_devices);
    ok_ret( 2, GetRegisteredRawInputDevices( raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE) ) );
    ok_eq( 2, raw_devices_count, UINT, "%u" );
    ok_eq( HID_USAGE_PAGE_GENERIC, raw_devices[0].usUsagePage, USHORT, "%#x" );
    ok_eq( HID_USAGE_GENERIC_JOYSTICK, raw_devices[0].usUsage, USHORT, "%#x" );
    ok_eq( HID_USAGE_PAGE_GENERIC, raw_devices[1].usUsagePage, USHORT, "%#x" );
    ok_eq( HID_USAGE_GENERIC_GAMEPAD, raw_devices[1].usUsage, USHORT, "%#x" );

    /* RIDEV_INPUTSINK requires hwndTarget != NULL */
    raw_devices[0].dwFlags = RIDEV_INPUTSINK;
    raw_devices[0].hwndTarget = 0;
    raw_devices[1].dwFlags = RIDEV_INPUTSINK;
    raw_devices[1].hwndTarget = 0;

    SetLastError(0xdeadbeef);
    ok_ret( 0, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    raw_devices[0].hwndTarget = hwnd;
    raw_devices[1].hwndTarget = hwnd;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );

    /* RIDEV_DEVNOTIFY send messages for any pre-existing device */
    raw_devices[0].dwFlags = RIDEV_DEVNOTIFY;
    raw_devices[0].hwndTarget = 0;
    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_devices[1].dwFlags = RIDEV_DEVNOTIFY;
    raw_devices[1].hwndTarget = 0;
    raw_devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;

    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );
    while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
    {
        ok_ne( WM_INPUT_DEVICE_CHANGE, msg.message, UINT, "%#x" );
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    /* RIDEV_DEVNOTIFY send messages only to hwndTarget */
    raw_devices[0].hwndTarget = hwnd;
    raw_devices[1].hwndTarget = hwnd;

    count = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );
    while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message == WM_INPUT_DEVICE_CHANGE) count++;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
    ok( count >= 2, "got %u messages\n", count );

    count = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );
    while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message == WM_INPUT_DEVICE_CHANGE) count++;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
    ok( count >= 2, "got %u messages\n", count );

    /* RIDEV_REMOVE requires hwndTarget == NULL */
    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = hwnd;
    raw_devices[1].dwFlags = RIDEV_REMOVE;
    raw_devices[1].hwndTarget = hwnd;

    SetLastError(0xdeadbeef);
    ok_ret( 0, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    raw_devices[0].hwndTarget = 0;
    raw_devices[1].hwndTarget = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );

    ok_ret( 1, DestroyWindow( hwnd ) );
}

static int rawinputbuffer_wndproc_count;

typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    ULONG hDevice;
    ULONG wParam;
} RAWINPUTHEADER32;

#ifdef _WIN64
typedef RAWINPUTHEADER RAWINPUTHEADER64;
typedef RAWINPUT RAWINPUT64;
#else
typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    ULONGLONG hDevice;
    ULONGLONG wParam;
} RAWINPUTHEADER64;

typedef struct
{
    RAWINPUTHEADER64 header;
    union {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT64;
#endif

static LRESULT CALLBACK rawinputbuffer_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INPUT)
    {
        UINT i, size, rawinput_size, iteration = rawinputbuffer_wndproc_count++;
        char buffer[16 * sizeof(RAWINPUT64)];
        RAWINPUT64 *rawbuffer64 = (RAWINPUT64 *)buffer;
        RAWINPUT *rawbuffer = (RAWINPUT *)buffer;
        HRAWINPUT handle = (HRAWINPUT)lparam;
        RAWINPUT rawinput = {{0}};

        winetest_push_context( "%u", iteration );

        if (is_wow64) rawinput_size = sizeof(RAWINPUTHEADER64) + sizeof(RAWKEYBOARD);
        else rawinput_size = sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD);

        size = sizeof(buffer);
        memset( buffer, 0, sizeof(buffer) );
        ok_ret( 3, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER))  );
        ok_eq( sizeof(buffer), size, UINT, "%u" );

        for (i = 0; i < 3; ++i)
        {
            winetest_push_context( "%u", i );
            if (is_wow64)
            {
                RAWINPUT64 *rawinput = (RAWINPUT64 *)(buffer + i * rawinput_size);
                ok_eq( RIM_TYPEKEYBOARD, rawinput->header.dwType, UINT, "%u" );
                ok_eq( rawinput_size, rawinput->header.dwSize, UINT, "%u" );
                ok_eq( wparam, rawinput->header.wParam, WPARAM, "%Iu" );
                ok_eq( i + 2, rawinput->data.keyboard.MakeCode, WPARAM, "%Iu" );
            }
            else
            {
                RAWINPUT *rawinput = (RAWINPUT *)(buffer + i * rawinput_size);
                ok_eq( RIM_TYPEKEYBOARD, rawinput->header.dwType, UINT, "%u" );
                ok_eq( rawinput_size, rawinput->header.dwSize, UINT, "%u" );
                ok_eq( wparam, rawinput->header.wParam, WPARAM, "%Iu" );
                ok_eq( i + 2, rawinput->data.keyboard.MakeCode, WPARAM, "%Iu" );
            }
            winetest_pop_context();
        }

        /* the first event should be removed by the next GetRawInputBuffer call
         * and the others should do another round through the message loop but not more */
        if (iteration == 0)
        {
            keybd_event( 'X', 5, 0, 0 );
            keybd_event( 'X', 6, KEYEVENTF_KEYUP, 0 );
            keybd_event( 'X', 2, KEYEVENTF_KEYUP, 0 );
            keybd_event( 'X', 3, 0, 0 );
            keybd_event( 'X', 4, KEYEVENTF_KEYUP, 0 );

            /* even though rawinput_size is the minimum required size,
             * it needs one more byte to return success */
            size = sizeof(rawinput) + 1;
            memset( buffer, 0, sizeof(buffer) );
            ok_ret( 1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
            if (is_wow64) ok_eq( 5, rawbuffer64->data.keyboard.MakeCode, WPARAM, "%Iu" );
            else ok_eq( 5, rawbuffer->data.keyboard.MakeCode, WPARAM, "%Iu" );

            /* peek the messages now, they should still arrive in the correct order */
            winetest_pop_context();
            wait_messages( 0, FALSE );
            winetest_push_context( "%u", iteration );

            /* reading the message data now should fail, the data
             * from the first message has been overwritten. */
            SetLastError( 0xdeadbeef );
            size = sizeof(rawinput);
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_HEADER, &rawinput, &size, sizeof(RAWINPUTHEADER) ) );
            ok_ret( ERROR_INVALID_HANDLE, GetLastError() );
        }
        else
        {
            SetLastError( 0xdeadbeef );
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_HEADER, &rawinput, &size, 0) );
            ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

            SetLastError( 0xdeadbeef );
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_HEADER, &rawinput, &size, sizeof(RAWINPUTHEADER) + 1 ) );
            ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

            size = sizeof(RAWINPUTHEADER);
            rawinput_size = sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD);
            ok_ret( sizeof(RAWINPUTHEADER), GetRawInputData( handle, RID_HEADER, &rawinput, &size, sizeof(RAWINPUTHEADER) ) );
            ok_eq( rawinput_size, rawinput.header.dwSize, UINT, "%u" );
            ok_eq( RIM_TYPEKEYBOARD, rawinput.header.dwType, UINT, "%u" );


            SetLastError( 0xdeadbeef );
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_INPUT, &rawinput, &size, 0) );
            ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

            SetLastError( 0xdeadbeef );
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER) + 1 ) );
            ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

            SetLastError( 0xdeadbeef );
            size = 0;
            ok_ret( (UINT)-1, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER) ) );
            ok_ret( ERROR_INSUFFICIENT_BUFFER, GetLastError() );

            SetLastError( 0xdeadbeef );
            size = sizeof(rawinput);
            ok_ret( (UINT)-1, GetRawInputData( handle, 0, &rawinput, &size, sizeof(RAWINPUTHEADER) ) );
            ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

            SetLastError( 0xdeadbeef );
            size = sizeof(rawinput);
            ok_ret( rawinput_size, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER) ) );
            ok_eq( 6, rawinput.data.keyboard.MakeCode, UINT, "%u" );

            SetLastError( 0xdeadbeef );
            size = sizeof(buffer);
            if (sizeof(void *) == 8)
            {
                ok_ret( (UINT)-1, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER32) ) );
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetRawInputData returned %08lx\n", GetLastError());
            }
            else if (is_wow64)
            {
                todo_wine
                ok_ret( rawinput_size, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER64) ) );
                ok_eq( 6, rawinput.data.keyboard.MakeCode, UINT, "%u" );
                todo_wine
                ok_ret( 0xdeadbeef, GetLastError() );
            }
            else
            {
                ok_ret( (UINT)-1, GetRawInputData( handle, RID_INPUT, &rawinput, &size, sizeof(RAWINPUTHEADER64) ) );
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetRawInputData returned %08lx\n", GetLastError());
            }
        }

        winetest_pop_context();
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void test_GetRawInputBuffer(void)
{
    unsigned int size, rawinput_size, header_size;
    RAWINPUTDEVICE raw_devices[1];
    char buffer[16 * sizeof(RAWINPUT64)];
    RAWINPUT64 *rawbuffer64 = (RAWINPUT64 *)buffer;
    RAWINPUT *rawbuffer = (RAWINPUT *)buffer;
    DWORD t1, t2, t3, now, pos1, pos2;
    LPARAM extra_info1, extra_info2;
    INPUT_MESSAGE_SOURCE source;
    POINT pt;
    HWND hwnd;
    BOOL ret;

    if (is_wow64) rawinput_size = sizeof(RAWINPUTHEADER64) + sizeof(RAWMOUSE);
    else rawinput_size = sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE);

    hwnd = create_foreground_window( TRUE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)rawinputbuffer_wndproc );

    if (pGetCurrentInputMessageSource)
    {
        ret = pGetCurrentInputMessageSource( &source );
        ok( ret, "got error %lu.\n", GetLastError() );
        ok( !source.deviceType, "got %#x.\n", source.deviceType );
        ok( !source.originId, "got %#x.\n", source.originId );
    }

    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_devices[0].dwFlags = RIDEV_INPUTSINK;
    raw_devices[0].hwndTarget = hwnd;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE( raw_devices ), sizeof(RAWINPUTDEVICE) ) );

    /* check basic error cases */

    SetLastError( 0xdeadbeef );
    ok_ret( (UINT)-1, GetRawInputBuffer( NULL, NULL, sizeof(RAWINPUTHEADER) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    size = sizeof(buffer);
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    /* valid calls, but no input */
    t1 = GetMessageTime();
    pos1 = GetMessagePos();
    extra_info1 = GetMessageExtraInfo();
    now = GetTickCount();
    ok( t1 <= now, "got %lu, %lu.\n", t1, now );

    size = sizeof(buffer);
    ok_ret( 0, GetRawInputBuffer( NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );
    size = 0;
    ok_ret( 0, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );
    size = sizeof(buffer);
    ok_ret( 0, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );


    Sleep( 20 );
    mouse_event( MOUSEEVENTF_MOVE, 5, 0, 0, 0xdeadbeef );
    t2 = GetMessageTime();
    ok( t2 == t1, "got %lu, %lu.\n", t1, t2 );
    /* invalid calls with input */

    SetLastError( 0xdeadbeef );
    size = 0;
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size, size, UINT, "%u" );
    ok_ret( ERROR_INSUFFICIENT_BUFFER, GetLastError() );
    t2 = GetMessageTime();
    ok( t2 == t1, "got %lu, %lu.\n", t1, t2 );

    SetLastError( 0xdeadbeef );
    size = sizeof(buffer);
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    size = sizeof(buffer);
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) + 1 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    /* the function returns 64-bit RAWINPUT structures on WoW64, but still
     * forbids sizeof(RAWINPUTHEADER) from the wrong architecture */
    SetLastError( 0xdeadbeef );
    size = sizeof(buffer);
    header_size = (sizeof(void *) == 8 ? sizeof(RAWINPUTHEADER32) : sizeof(RAWINPUTHEADER64));
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, header_size ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );


    /* NOTE: calling with size == rawinput_size returns an error, */
    /* BUT it fills the buffer nonetheless and empties the internal buffer (!!) */

    Sleep( 20 );
    t2 = GetTickCount();
    size = 0;
    ok_ret( 0, GetRawInputBuffer( NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size, size, UINT, "%u" );

    SetLastError( 0xdeadbeef );
    size = rawinput_size;
    memset( buffer, 0, sizeof(buffer) );
    ok_ret( (UINT)-1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_ret( ERROR_INSUFFICIENT_BUFFER, GetLastError() );
    if (is_wow64) ok_eq( 5, rawbuffer64->data.mouse.lLastX, UINT, "%u" );
    else ok_eq( 5, rawbuffer->data.mouse.lLastX, UINT, "%u" );

    t3 = GetMessageTime();
    pos2 = GetMessagePos();
    extra_info2 = GetMessageExtraInfo();
    ok( extra_info2 == extra_info1, "got %#Ix, %#Ix.\n", extra_info1, extra_info2 );
    GetCursorPos( &pt );
    ok( t3 > t1, "got %lu, %lu.\n", t1, t3 );
    ok( t3 < t2, "got %lu, %lu.\n", t2, t3 );
    ok( pos1 == pos2, "got pos1 (%ld, %ld), pos2 (%ld, %ld), pt (%ld %ld).\n",
        pos1 & 0xffff, pos1 >> 16, pos2 & 0xffff, pos2 >> 16, pt.x, pt.y );
    if (pGetCurrentInputMessageSource)
    {
        ret = pGetCurrentInputMessageSource( &source );
        ok( ret, "got error %lu.\n", GetLastError() );
        ok( !source.deviceType, "got %#x.\n", source.deviceType );
        ok( !source.originId, "got %#x.\n", source.originId );
    }

    /* no more data to read */

    size = sizeof(buffer);
    ok_ret( 0, GetRawInputBuffer( NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );


    /* rawinput_size + 1 succeeds */
    t1 = GetMessageTime();
    pos1 = GetMessagePos();
    extra_info1 = GetMessageExtraInfo();
    now = GetTickCount();
    ok( t1 <= now, "got %lu, %lu.\n", t1, now );

    Sleep( 20 );
    mouse_event( MOUSEEVENTF_MOVE, 5, 0, 0, 0xfeedcafe );

    t2 = GetMessageTime();
    ok( t2 == t1, "got %lu, %lu.\n", t1, t2 );

    Sleep( 20 );
    t2 = GetTickCount();

    size = rawinput_size + 1;
    memset( buffer, 0, sizeof(buffer) );
    ok_ret( 1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size + 1, size, UINT, "%u" );
    if (is_wow64)
    {
        ok_eq( RIM_TYPEMOUSE, rawbuffer64->header.dwType, UINT, "%#x" );
        ok_eq( 0, rawbuffer64->header.wParam, WPARAM, "%Iu" );
        ok_eq( 5, rawbuffer64->data.mouse.lLastX, UINT, "%u" );
    }
    else
    {
        ok_eq( RIM_TYPEMOUSE, rawbuffer->header.dwType, UINT, "%#x" );
        ok_eq( 0, rawbuffer->header.wParam, WPARAM, "%Iu" );
        ok_eq( 5, rawbuffer->data.mouse.lLastX, UINT, "%u" );
    }

    size = sizeof(buffer);
    ok_ret( 0, GetRawInputBuffer( NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );

    t3 = GetMessageTime();
    pos2 = GetMessagePos();
    extra_info2 = GetMessageExtraInfo();
    GetCursorPos(&pt);
    ok( extra_info2 == extra_info1, "got %#Ix, %#Ix.\n", extra_info1, extra_info2 );
    ok( t3 > t1, "got %lu, %lu.\n", t1, t3 );
    ok( t3 < t2, "got %lu, %lu.\n", t2, t3 );
    ok( pos1 == pos2, "got pos1 (%ld, %ld), pos2 (%ld, %ld), pt (%ld %ld).\n",
        pos1 & 0xffff, pos1 >> 16, pos2 & 0xffff, pos2 >> 16, pt.x, pt.y );

    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE) ) );


    /* some keyboard tests to better check fields under wow64 */
    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    raw_devices[0].dwFlags = RIDEV_INPUTSINK;
    raw_devices[0].hwndTarget = hwnd;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE) ) );

    keybd_event( 'X', 0x2d, 0, 0 );
    keybd_event( 'X', 0x2d, KEYEVENTF_KEYUP, 0 );

    if (is_wow64) rawinput_size = sizeof(RAWINPUTHEADER64) + sizeof(RAWKEYBOARD);
    else rawinput_size = sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD);

    size = 0;
    ok_ret( 0, GetRawInputBuffer( NULL, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size, size, UINT, "%u" );

    size = sizeof(buffer);
    memset( buffer, 0, sizeof(buffer) );
    ok_ret( 2, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( sizeof(buffer), size, UINT, "%u" );
    if (is_wow64)
    {
        ok_eq( RIM_TYPEKEYBOARD, rawbuffer64->header.dwType, UINT, "%u" );
        ok_eq( 0, rawbuffer64->header.wParam, UINT, "%u" );
        ok_eq( 0x2d, rawbuffer64->data.keyboard.MakeCode, UINT, "%#x" );
    }
    else
    {
        ok_eq( RIM_TYPEKEYBOARD, rawbuffer->header.dwType, UINT, "%u" );
        ok_eq( 0, rawbuffer->header.wParam, UINT, "%u" );
        ok_eq( 0x2d, rawbuffer->data.keyboard.MakeCode, UINT, "%#x" );
    }


    /* test GetRawInputBuffer interaction with WM_INPUT messages */

    rawinputbuffer_wndproc_count = 0;
    keybd_event( 'X', 1, 0, 0 );
    keybd_event( 'X', 2, KEYEVENTF_KEYUP, 0 );
    keybd_event( 'X', 3, 0, 0 );
    keybd_event( 'X', 4, KEYEVENTF_KEYUP, 0 );
    wait_messages( 100, FALSE );
    ok_eq( 2, rawinputbuffer_wndproc_count, UINT, "%u" );

    keybd_event( 'X', 3, 0, 0 );
    keybd_event( 'X', 4, KEYEVENTF_KEYUP, 0 );

    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE) ) );


    /* rawinput buffer survives registered device changes */

    size = rawinput_size + 1;
    ok_ret( 1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size + 1, size, UINT, "%u" );

    raw_devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    raw_devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    raw_devices[0].dwFlags = RIDEV_INPUTSINK;
    raw_devices[0].hwndTarget = hwnd;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE) ) );

    if (is_wow64) rawinput_size = sizeof(RAWINPUTHEADER64) + sizeof(RAWMOUSE);
    else rawinput_size = sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE);

    size = rawinput_size + 1;
    ok_ret( 1, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( rawinput_size + 1, size, UINT, "%u" );
    size = sizeof(buffer);
    ok_ret( 0, GetRawInputBuffer( rawbuffer, &size, sizeof(RAWINPUTHEADER) ) );
    ok_eq( 0, size, UINT, "%u" );

    raw_devices[0].dwFlags = RIDEV_REMOVE;
    raw_devices[0].hwndTarget = 0;
    ok_ret( 1, RegisterRawInputDevices( raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE) ) );


    ok_ret( 1, DestroyWindow( hwnd ) );
}

static BOOL rawinput_test_received_legacy;
static BOOL rawinput_test_received_raw;
static BOOL rawinput_test_received_rawfg;

static LRESULT CALLBACK rawinput_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT ret, raw_size;
    RAWINPUT raw;

    if (msg == WM_INPUT)
    {
        todo_wine_if(rawinput_test_received_raw)
        ok(!rawinput_test_received_raw, "Unexpected spurious WM_INPUT message.\n");
        ok(wparam == RIM_INPUT || wparam == RIM_INPUTSINK, "Unexpected wparam: %Iu\n", wparam);

        rawinput_test_received_raw = TRUE;
        if (wparam == RIM_INPUT) rawinput_test_received_rawfg = TRUE;

        ret = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &raw_size, sizeof(RAWINPUTHEADER));
        ok(ret == 0, "GetRawInputData failed\n");
        ok(raw_size <= sizeof(raw), "Unexpected rawinput data size: %u\n", raw_size);

        raw_size = sizeof(raw);
        ret = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, &raw_size, sizeof(RAWINPUTHEADER));
        ok(ret > 0 && ret != (UINT)-1, "GetRawInputData failed\n");
        ok(raw.header.dwType == RIM_TYPEMOUSE, "Unexpected rawinput type: %lu\n", raw.header.dwType);
        ok(raw.header.dwSize == raw_size, "Expected size %u, got %lu\n", raw_size, raw.header.dwSize);
        todo_wine_if (wparam)
            ok(raw.header.wParam == wparam, "Expected wparam %Iu, got %Iu\n", wparam, raw.header.wParam);

        ok(!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE), "Unexpected absolute rawinput motion\n");
        ok(!(raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP), "Unexpected virtual desktop rawinput motion\n");
    }

    if (msg == WM_MOUSEMOVE) rawinput_test_received_legacy = TRUE;

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

struct rawinput_test
{
    BOOL register_device;
    BOOL register_window;
    DWORD register_flags;
    BOOL expect_legacy;
    BOOL expect_raw;
    BOOL expect_rawfg;
    BOOL todo_legacy;
    BOOL todo_raw;
    BOOL todo_rawfg;
};

struct rawinput_test rawinput_tests[] =
{
    { FALSE, FALSE, 0,                TRUE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  FALSE, 0,                TRUE,  TRUE,  TRUE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  0,                TRUE,  TRUE,  TRUE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_NOLEGACY,  FALSE,  TRUE,  TRUE, /* todos: */ FALSE, FALSE, FALSE },

    /* same-process foreground tests */
    { TRUE,  FALSE, 0,               FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  0,               FALSE,  TRUE,  TRUE, /* todos: */ FALSE, FALSE, FALSE },

    /* cross-process foreground tests */
    { TRUE,  TRUE,  0,               FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_INPUTSINK, FALSE,  TRUE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  0,               FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },

    /* multi-process rawinput tests */
    { TRUE,  TRUE,  0,               FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_INPUTSINK, FALSE,  TRUE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_INPUTSINK, FALSE,  TRUE, FALSE, /* todos: */ FALSE, FALSE, FALSE },

    { TRUE,  TRUE,  RIDEV_EXINPUTSINK, FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_EXINPUTSINK, FALSE,  TRUE, FALSE, /* todos: */ FALSE,  TRUE, FALSE },

    /* cross-desktop foreground tests */
    { TRUE,  FALSE, 0,               FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  0,               FALSE,  TRUE,  TRUE, /* todos: */ FALSE, FALSE, FALSE },
    { TRUE,  TRUE,  RIDEV_INPUTSINK, FALSE, FALSE, FALSE, /* todos: */ FALSE, FALSE, FALSE },
};

static void rawinput_test_process(void)
{
    RAWINPUTDEVICE raw_devices[1];
    HANDLE ready, start, done;
    DWORD ret;
    POINT pt;
    HWND hwnd = NULL;
    MSG msg;
    int i;

    ready = OpenEventA(EVENT_ALL_ACCESS, FALSE, "rawinput_test_process_ready");
    ok(ready != 0, "OpenEventA failed, error: %lu\n", GetLastError());

    start = OpenEventA(EVENT_ALL_ACCESS, FALSE, "rawinput_test_process_start");
    ok(start != 0, "OpenEventA failed, error: %lu\n", GetLastError());

    done = OpenEventA(EVENT_ALL_ACCESS, FALSE, "rawinput_test_process_done");
    ok(done != 0, "OpenEventA failed, error: %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(rawinput_tests); ++i)
    {
        WaitForSingleObject(ready, INFINITE);
        ResetEvent(ready);

        switch (i)
        {
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 16:
            GetCursorPos(&pt);

            hwnd = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
                                 pt.x - 50, pt.y - 50, 100, 100, 0, NULL, NULL, NULL);
            SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)rawinput_wndproc);
            ok(hwnd != 0, "CreateWindow failed\n");
            empty_message_queue();

            /* FIXME: Try to workaround X11/Win32 focus inconsistencies and
             * make the window visible and foreground as hard as possible. */
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
            SetForegroundWindow(hwnd);
            UpdateWindow(hwnd);
            empty_message_queue();

            if (i == 9 || i == 10 || i == 11 || i == 12)
            {
                raw_devices[0].usUsagePage = 0x01;
                raw_devices[0].usUsage = 0x02;
                raw_devices[0].dwFlags = i == 11 ? RIDEV_INPUTSINK : 0;
                raw_devices[0].hwndTarget = i == 11 ? hwnd : 0;

                SetLastError(0xdeadbeef);
                ret = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
                ok(ret, "%d: RegisterRawInputDevices failed\n", i);
                ok(GetLastError() == 0xdeadbeef, "%d: RegisterRawInputDevices returned %08lx\n", i, GetLastError());
            }

            rawinput_test_received_legacy = FALSE;
            rawinput_test_received_raw = FALSE;
            rawinput_test_received_rawfg = FALSE;

            /* fallthrough */
        case 14:
        case 15:
            if (i != 8) mouse_event(MOUSEEVENTF_MOVE, 5, 0, 0, 0);
            empty_message_queue();
            break;
        }

        SetEvent(start);

        while (MsgWaitForMultipleObjects(1, &done, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
            while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

        ResetEvent(done);

        if (i == 9 || i == 10 || i == 11 || i == 12)
        {
            raw_devices[0].dwFlags = RIDEV_REMOVE;
            raw_devices[0].hwndTarget = 0;

            flaky_wine
            ok(rawinput_test_received_legacy, "%d: foreground process expected WM_MOUSEMOVE message\n", i);
            ok(rawinput_test_received_raw, "%d: foreground process expected WM_INPUT message\n", i);
            ok(rawinput_test_received_rawfg, "%d: foreground process expected RIM_INPUT message\n", i);

            SetLastError(0xdeadbeef);
            ret = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
            ok(ret, "%d: RegisterRawInputDevices failed\n", i);
            ok(GetLastError() == 0xdeadbeef, "%d: RegisterRawInputDevices returned %08lx\n", i, GetLastError());
        }

        if (hwnd) DestroyWindow(hwnd);
    }

    WaitForSingleObject(ready, INFINITE);
    CloseHandle(done);
    CloseHandle(start);
    CloseHandle(ready);
}

struct rawinput_test_thread_params
{
    HDESK desk;
    HANDLE ready;
    HANDLE start;
    HANDLE done;
};

static DWORD WINAPI rawinput_test_desk_thread(void *arg)
{
    struct rawinput_test_thread_params *params = arg;
    RAWINPUTDEVICE raw_devices[1];
    DWORD ret;
    POINT pt;
    HWND hwnd = NULL;
    MSG msg;
    int i;

    ok( SetThreadDesktop( params->desk ), "SetThreadDesktop failed\n" );

    for (i = 14; i < ARRAY_SIZE(rawinput_tests); ++i)
    {
        WaitForSingleObject(params->ready, INFINITE);
        ResetEvent(params->ready);

        switch (i)
        {
        case 14:
        case 15:
        case 16:
            GetCursorPos(&pt);

            hwnd = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
                                 pt.x - 50, pt.y - 50, 100, 100, 0, NULL, NULL, NULL);
            ok(hwnd != 0, "CreateWindow failed\n");
            SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)rawinput_wndproc);
            empty_message_queue();

            /* FIXME: Try to workaround X11/Win32 focus inconsistencies and
             * make the window visible and foreground as hard as possible. */
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
            SetForegroundWindow(hwnd);
            UpdateWindow(hwnd);
            empty_message_queue();

            raw_devices[0].usUsagePage = 0x01;
            raw_devices[0].usUsage = 0x02;
            raw_devices[0].dwFlags = rawinput_tests[i].register_flags;
            raw_devices[0].hwndTarget = rawinput_tests[i].register_window ? hwnd : 0;

            rawinput_test_received_legacy = FALSE;
            rawinput_test_received_raw = FALSE;
            rawinput_test_received_rawfg = FALSE;

            SetLastError(0xdeadbeef);
            ret = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
            ok(ret, "%d: RegisterRawInputDevices failed\n", i);
            ok(GetLastError() == 0xdeadbeef, "%d: RegisterRawInputDevices returned %08lx\n", i, GetLastError());
            break;
        }

        SetEvent(params->start);

        while (MsgWaitForMultipleObjects(1, &params->done, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
            while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

        ResetEvent(params->done);
        if (hwnd) DestroyWindow(hwnd);
    }

    return 0;
}

static DWORD WINAPI rawinput_test_thread(void *arg)
{
    struct rawinput_test_thread_params *params = arg;
    HANDLE thread;
    POINT pt;
    HWND hwnd = NULL;
    int i;

    for (i = 0; i < 14; ++i)
    {
        WaitForSingleObject(params->ready, INFINITE);
        ResetEvent(params->ready);

        switch (i)
        {
        case 4:
        case 5:
            GetCursorPos(&pt);

            hwnd = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
                                 pt.x - 50, pt.y - 50, 100, 100, 0, NULL, NULL, NULL);
            ok(hwnd != 0, "CreateWindow failed\n");
            empty_message_queue();

            /* FIXME: Try to workaround X11/Win32 focus inconsistencies and
             * make the window visible and foreground as hard as possible. */
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
            SetForegroundWindow(hwnd);
            UpdateWindow(hwnd);
            empty_message_queue();

            mouse_event(MOUSEEVENTF_MOVE, 5, 0, 0, 0);
            empty_message_queue();
            break;
        }

        SetEvent(params->start);

        WaitForSingleObject(params->done, INFINITE);
        ResetEvent(params->done);
        if (hwnd) DestroyWindow(hwnd);
    }

    thread = CreateThread(NULL, 0, rawinput_test_desk_thread, params, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    return 0;
}

static void test_rawinput(const char* argv0)
{
    struct rawinput_test_thread_params params;
    PROCESS_INFORMATION process_info;
    RAWINPUTDEVICE raw_devices[1];
    STARTUPINFOA startup_info;
    HANDLE thread, process_ready, process_start, process_done;
    DWORD ret;
    POINT pt, newpt;
    HWND hwnd;
    BOOL skipped;
    char path[MAX_PATH];
    int i;

    params.desk = CreateDesktopA( "rawinput_test_desktop", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( params.desk != NULL, "CreateDesktopA failed, last error: %lu\n", GetLastError() );

    params.ready = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(params.ready != NULL, "CreateEvent failed\n");

    params.start = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(params.start != NULL, "CreateEvent failed\n");

    params.done = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(params.done != NULL, "CreateEvent failed\n");

    thread = CreateThread(NULL, 0, rawinput_test_thread, &params, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");

    process_ready = CreateEventA(NULL, FALSE, FALSE, "rawinput_test_process_ready");
    ok(process_ready != NULL, "CreateEventA failed\n");

    process_start = CreateEventA(NULL, FALSE, FALSE, "rawinput_test_process_start");
    ok(process_start != NULL, "CreateEventA failed\n");

    process_done = CreateEventA(NULL, FALSE, FALSE, "rawinput_test_process_done");
    ok(process_done != NULL, "CreateEventA failed\n");

    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_SHOWNORMAL;

    sprintf(path, "%s input rawinput_test", argv0);
    ret = CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info );
    ok(ret, "CreateProcess \"%s\" failed err %lu.\n", path, GetLastError());

    SetCursorPos(100, 100);
    empty_message_queue();

    for (i = 0; i < ARRAY_SIZE(rawinput_tests); ++i)
    {
        GetCursorPos(&pt);

        hwnd = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
                             pt.x - 50, pt.y - 50, 100, 100, 0, NULL, NULL, NULL);
        ok(hwnd != 0, "CreateWindow failed\n");
        if (i != 14 && i != 15 && i != 16)
            SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)rawinput_wndproc);
        empty_message_queue();

        /* FIXME: Try to workaround X11/Win32 focus inconsistencies and
         * make the window visible and foreground as hard as possible. */
        ShowWindow(hwnd, SW_SHOW);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
        SetForegroundWindow(hwnd);
        UpdateWindow(hwnd);
        empty_message_queue();

        rawinput_test_received_legacy = FALSE;
        rawinput_test_received_raw = FALSE;
        rawinput_test_received_rawfg = FALSE;

        raw_devices[0].usUsagePage = 0x01;
        raw_devices[0].usUsage = 0x02;
        raw_devices[0].dwFlags = rawinput_tests[i].register_flags;
        raw_devices[0].hwndTarget = rawinput_tests[i].register_window ? hwnd : 0;

        if (!rawinput_tests[i].register_device)
            skipped = FALSE;
        else
        {
            SetLastError(0xdeadbeef);
            skipped = !RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
            if (rawinput_tests[i].register_flags == RIDEV_EXINPUTSINK && skipped)
                win_skip("RIDEV_EXINPUTSINK not supported\n");
            else
                ok(!skipped, "%d: RegisterRawInputDevices failed: %lu\n", i, GetLastError());
        }

        SetEvent(params.ready);
        WaitForSingleObject(params.start, INFINITE);
        ResetEvent(params.start);

        /* we need the main window to be over the other thread window, as although
         * it is in another desktop, it will receive the messages directly otherwise */
        switch (i)
        {
        case 14:
        case 15:
            DestroyWindow(hwnd);
            hwnd = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
                                 pt.x - 50, pt.y - 50, 100, 100, 0, NULL, NULL, NULL);
            ok(hwnd != 0, "CreateWindow failed\n");
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
            SetForegroundWindow(hwnd);
            empty_message_queue();

            rawinput_test_received_legacy = FALSE;
            rawinput_test_received_raw = FALSE;
            rawinput_test_received_rawfg = FALSE;
            break;
        }

        SetEvent(process_ready);
        WaitForSingleObject(process_start, INFINITE);
        ResetEvent(process_start);

        if (i <= 3 || i == 8) mouse_event(MOUSEEVENTF_MOVE, 5, 0, 0, 0);
        empty_message_queue();

        SetEvent(process_done);
        SetEvent(params.done);

        flaky_wine
        if (!skipped)
        {
            todo_wine_if(rawinput_tests[i].todo_legacy)
            ok(rawinput_test_received_legacy == rawinput_tests[i].expect_legacy,
               "%d: %sexpected WM_MOUSEMOVE message\n", i, rawinput_tests[i].expect_legacy ? "" : "un");
            todo_wine_if(rawinput_tests[i].todo_raw)
            ok(rawinput_test_received_raw == rawinput_tests[i].expect_raw,
               "%d: %sexpected WM_INPUT message\n", i, rawinput_tests[i].expect_raw ? "" : "un");
            todo_wine_if(rawinput_tests[i].todo_rawfg)
            ok(rawinput_test_received_rawfg == rawinput_tests[i].expect_rawfg,
               "%d: %sexpected RIM_INPUT message\n", i, rawinput_tests[i].expect_rawfg ? "" : "un");
        }

        GetCursorPos(&newpt);
        ok((newpt.x - pt.x) == 5 || (newpt.x - pt.x) == 4, "%d: Unexpected cursor movement\n", i);

        if (rawinput_tests[i].register_device)
        {
            raw_devices[0].dwFlags = RIDEV_REMOVE;
            raw_devices[0].hwndTarget = 0;

            SetLastError(0xdeadbeef);
            ret = RegisterRawInputDevices(raw_devices, ARRAY_SIZE(raw_devices), sizeof(RAWINPUTDEVICE));
            ok(ret, "%d: RegisterRawInputDevices failed: %lu\n", i, GetLastError());
        }

        DestroyWindow(hwnd);
    }

    SetEvent(process_ready);
    winetest_wait_child_process(process_info.hProcess);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    CloseHandle(process_done);
    CloseHandle(process_start);
    CloseHandle(process_ready);

    WaitForSingleObject(thread, INFINITE);

    CloseHandle(params.done);
    CloseHandle(params.start);
    CloseHandle(params.ready);
    CloseHandle(thread);

    CloseDesktop(params.desk);
}

static void test_DefRawInputProc(void)
{
    LRESULT ret;

    SetLastError(0xdeadbeef);
    ret = DefRawInputProc(NULL, 0, sizeof(RAWINPUTHEADER));
    ok(!ret, "got %Id\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %ld\n", GetLastError());
    ret = DefRawInputProc(LongToPtr(0xcafe), 0xbeef, sizeof(RAWINPUTHEADER));
    ok(!ret, "got %Id\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %ld\n", GetLastError());
    ret = DefRawInputProc(NULL, 0, sizeof(RAWINPUTHEADER) - 1);
    ok(ret == -1, "got %Id\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %ld\n", GetLastError());
    ret = DefRawInputProc(NULL, 0, sizeof(RAWINPUTHEADER) + 1);
    ok(ret == -1, "got %Id\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %ld\n", GetLastError());
}

static void test_key_map(void)
{
    HKL kl = GetKeyboardLayout(0);
    UINT kL, kR, s, sL;
    int i;
    static const UINT numpad_collisions[][2] = {
        { VK_NUMPAD0, VK_INSERT },
        { VK_NUMPAD1, VK_END },
        { VK_NUMPAD2, VK_DOWN },
        { VK_NUMPAD3, VK_NEXT },
        { VK_NUMPAD4, VK_LEFT },
        { VK_NUMPAD6, VK_RIGHT },
        { VK_NUMPAD7, VK_HOME },
        { VK_NUMPAD8, VK_UP },
        { VK_NUMPAD9, VK_PRIOR },
    };

    s  = MapVirtualKeyExA(VK_SHIFT,  MAPVK_VK_TO_VSC, kl);
    ok(s != 0, "MapVirtualKeyEx(VK_SHIFT) should return non-zero\n");
    sL = MapVirtualKeyExA(VK_LSHIFT, MAPVK_VK_TO_VSC, kl);
    ok(s == sL || broken(sL == 0), /* win9x */
       "%x != %x\n", s, sL);

    kL = MapVirtualKeyExA(0x2a, MAPVK_VSC_TO_VK, kl);
    ok(kL == VK_SHIFT, "Scan code -> vKey = %x (not VK_SHIFT)\n", kL);
    kR = MapVirtualKeyExA(0x36, MAPVK_VSC_TO_VK, kl);
    ok(kR == VK_SHIFT, "Scan code -> vKey = %x (not VK_SHIFT)\n", kR);

    kL = MapVirtualKeyExA(0x2a, MAPVK_VSC_TO_VK_EX, kl);
    ok(kL == VK_LSHIFT || broken(kL == 0), /* win9x */
       "Scan code -> vKey = %x (not VK_LSHIFT)\n", kL);
    kR = MapVirtualKeyExA(0x36, MAPVK_VSC_TO_VK_EX, kl);
    ok(kR == VK_RSHIFT || broken(kR == 0), /* win9x */
       "Scan code -> vKey = %x (not VK_RSHIFT)\n", kR);

    /* test that MAPVK_VSC_TO_VK prefers the non-numpad vkey if there's ambiguity */
    for (i = 0; i < ARRAY_SIZE(numpad_collisions); i++)
    {
        UINT numpad_scan = MapVirtualKeyExA(numpad_collisions[i][0],  MAPVK_VK_TO_VSC, kl);
        UINT other_scan  = MapVirtualKeyExA(numpad_collisions[i][1],  MAPVK_VK_TO_VSC, kl);

        /* do they really collide for this layout? */
        if (numpad_scan && other_scan == numpad_scan)
        {
            UINT vkey = MapVirtualKeyExA(numpad_scan, MAPVK_VSC_TO_VK, kl);
            ok(vkey != numpad_collisions[i][0],
               "Got numpad vKey %x for scan code %x when there was another choice\n",
               vkey, numpad_scan);
        }
    }

    /* test the scan code prefixes of the right variant of a keys */
    s = MapVirtualKeyExA(VK_RCONTROL, MAPVK_VK_TO_VSC, kl);
    ok(s >> 8 == 0x00, "Scan code prefixes should not be returned when not using MAPVK_VK_TO_VSC_EX %#1x\n", s >> 8);
    s = MapVirtualKeyExA(VK_RCONTROL, MAPVK_VK_TO_VSC_EX, kl);
    ok(s >> 8 == 0xE0 || broken(s == 0), "Scan code prefix for VK_RCONTROL should be 0xE0 when MAPVK_VK_TO_VSC_EX is set, was %#1x\n", s >> 8);
    s = MapVirtualKeyExA(VK_RMENU, MAPVK_VK_TO_VSC_EX, kl);
    ok(s >> 8 == 0xE0 || broken(s == 0), "Scan code prefix for VK_RMENU should be 0xE0 when MAPVK_VK_TO_VSC_EX is set, was %#1x\n", s >> 8);
    s = MapVirtualKeyExA(VK_RSHIFT, MAPVK_VK_TO_VSC_EX, kl);
    ok(s >> 8 == 0x00 || broken(s == 0), "The scan code shouldn't have a prefix, got %#1x\n", s >> 8);
}

#define shift 1
#define ctrl  2
#define menu  4

static const struct tounicode_tests
{
    UINT vk;
    DWORD modifiers;
    WCHAR chr; /* if vk is 0, lookup vk using this char */
    int expect_ret;
    WCHAR expect_buf[4];
} utests[] =
{
    { 0, 0, 'a', 1, {'a',0}},
    { 0, shift, 'a', 1, {'A',0}},
    { 0, menu, 'a', 1, {'a',0}},
    { 0, shift|menu, 'a', 1, {'A',0}},
#if defined(__REACTOS__) && defined(_MSC_VER)
    { 0, shift|ctrl|menu, 'a', 0, {0}},
#else
    { 0, shift|ctrl|menu, 'a', 0, {}},
#endif
    { 0, ctrl, 'a', 1, {1, 0}},
    { 0, shift|ctrl, 'a', 1, {1, 0}},
#if defined(__REACTOS__) && defined(_MSC_VER)
    { VK_TAB, ctrl, 0, 0, {0}},
    { VK_TAB, shift|ctrl, 0, 0, {0}},
#else
    { VK_TAB, ctrl, 0, 0, {}},
    { VK_TAB, shift|ctrl, 0, 0, {}},
#endif
    { VK_RETURN, ctrl, 0, 1, {'\n', 0}},
#if defined(__REACTOS__) && defined(_MSC_VER)
    { VK_RETURN, shift|ctrl, 0, 0, {0}},
    { 0, ctrl, '4', 0, {0}},
    { 0, shift|ctrl, '4', 0, {0}},
    { 0, ctrl, '!', 0, {0}},
    { 0, ctrl, '\"', 0, {0}},
    { 0, ctrl, '#', 0, {0}},
    { 0, ctrl, '$', 0, {0}},
    { 0, ctrl, '%', 0, {0}},
    { 0, ctrl, '\'', 0, {0}},
    { 0, ctrl, '(', 0, {0}},
    { 0, ctrl, ')', 0, {0}},
    { 0, ctrl, '*', 0, {0}},
    { 0, ctrl, '+', 0, {0}},
    { 0, ctrl, ',', 0, {0}},
    { 0, ctrl, '-', 0, {0}},
    { 0, ctrl, '.', 0, {0}},
    { 0, ctrl, '/', 0, {0}},
    { 0, ctrl, ':', 0, {0}},
    { 0, ctrl, ';', 0, {0}},
    { 0, ctrl, '<', 0, {0}},
    { 0, ctrl, '=', 0, {0}},
    { 0, ctrl, '>', 0, {0}},
    { 0, ctrl, '?', 0, {0}},
#else
    { VK_RETURN, shift|ctrl, 0, 0, {}},
    { 0, ctrl, '4', 0, {}},
    { 0, shift|ctrl, '4', 0, {}},
    { 0, ctrl, '!', 0, {}},
    { 0, ctrl, '\"', 0, {}},
    { 0, ctrl, '#', 0, {}},
    { 0, ctrl, '$', 0, {}},
    { 0, ctrl, '%', 0, {}},
    { 0, ctrl, '\'', 0, {}},
    { 0, ctrl, '(', 0, {}},
    { 0, ctrl, ')', 0, {}},
    { 0, ctrl, '*', 0, {}},
    { 0, ctrl, '+', 0, {}},
    { 0, ctrl, ',', 0, {}},
    { 0, ctrl, '-', 0, {}},
    { 0, ctrl, '.', 0, {}},
    { 0, ctrl, '/', 0, {}},
    { 0, ctrl, ':', 0, {}},
    { 0, ctrl, ';', 0, {}},
    { 0, ctrl, '<', 0, {}},
    { 0, ctrl, '=', 0, {}},
    { 0, ctrl, '>', 0, {}},
    { 0, ctrl, '?', 0, {}},
#endif
    { 0, ctrl, '@', 1, {0}},
    { 0, ctrl, '[', 1, {0x1b}},
    { 0, ctrl, '\\', 1, {0x1c}},
    { 0, ctrl, ']', 1, {0x1d}},
    { 0, ctrl, '^', 1, {0x1e}},
    { 0, ctrl, '_', 1, {0x1f}},
#if defined(__REACTOS__) && defined(_MSC_VER)
    { 0, ctrl, '`', 0, {0}},
#else
    { 0, ctrl, '`', 0, {}},
#endif
    { VK_SPACE, 0, 0, 1, {' ',0}},
    { VK_SPACE, shift, 0, 1, {' ',0}},
    { VK_SPACE, ctrl, 0, 1, {' ',0}},
};

static void test_ToUnicode(void)
{
    WCHAR wStr[4];
    BYTE state[256];
    const BYTE SC_RETURN = 0x1c, SC_TAB = 0x0f, SC_A = 0x1e;
    const BYTE HIGHEST_BIT = 0x80;
    int i, ret;
    BOOL us_kbd = (GetKeyboardLayout(0) == (HKL)(ULONG_PTR)0x04090409);

    for(i=0; i<256; i++)
        state[i]=0;

    wStr[1] = 0xAA;
    SetLastError(0xdeadbeef);
    ret = ToUnicode(VK_RETURN, SC_RETURN, state, wStr, 4, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ToUnicode is not implemented\n");
        return;
    }

    ok(ret == 1, "ToUnicode for Return key didn't return 1 (was %i)\n", ret);
    if(ret == 1)
    {
        ok(wStr[0]=='\r', "ToUnicode for CTRL + Return was %i (expected 13)\n", wStr[0]);
        ok(wStr[1]==0 || broken(wStr[1]!=0) /* nt4 */,
           "ToUnicode didn't null-terminate the buffer when there was room.\n");
    }

    for (i = 0; i < ARRAY_SIZE(utests); i++)
    {
        UINT vk = utests[i].vk, mod = utests[i].modifiers, scan;

        if(!vk)
        {
            short vk_ret;

            if (!us_kbd) continue;
            vk_ret = VkKeyScanW(utests[i].chr);
            if (vk_ret == -1) continue;
            vk = vk_ret & 0xff;
            if (vk_ret & 0x100) mod |= shift;
            if (vk_ret & 0x200) mod |= ctrl;
        }
        scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

        state[VK_SHIFT]   = state[VK_LSHIFT]   = (mod & shift) ? HIGHEST_BIT : 0;
        state[VK_CONTROL] = state[VK_LCONTROL] = (mod & ctrl) ? HIGHEST_BIT : 0;
        state[VK_MENU]    = state[VK_LMENU]    = (mod & menu) ? HIGHEST_BIT : 0;

        ret = ToUnicode(vk, scan, state, wStr, 4, 0);
        ok(ret == utests[i].expect_ret, "%d: got %d expected %d\n", i, ret, utests[i].expect_ret);
        if (ret)
            ok(!lstrcmpW(wStr, utests[i].expect_buf), "%d: got %s expected %s\n", i, wine_dbgstr_w(wStr),
                wine_dbgstr_w(utests[i].expect_buf));

    }
    state[VK_SHIFT]   = state[VK_LSHIFT]   = 0;
    state[VK_CONTROL] = state[VK_LCONTROL] = 0;

    ret = ToUnicode(VK_TAB, SC_TAB, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicode(VK_RETURN, SC_RETURN, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicode('A', SC_A, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx(VK_TAB, SC_TAB, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx(VK_RETURN, SC_RETURN, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx('A', SC_A, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
}

static void test_ToAscii(void)
{
    WCHAR wstr[16];
    char str[16];
    WORD character;
    BYTE state[256];
    const BYTE SC_RETURN = 0x1c, SC_A = 0x1e;
    const BYTE HIGHEST_BIT = 0x80;
    int ret, len;
    DWORD gle;

    memset(state, 0, sizeof(state));

    character = 0;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 1, "ToAscii for Return key didn't return 1 (was %i)\n", ret);
    ok(character == '\r', "ToAscii for Return was %i (expected 13)\n", character);

    wstr[0] = 0;
    ret = ToUnicode('A', SC_A, state, wstr, ARRAY_SIZE(wstr), 0);
    ok(ret == 1, "ToUnicode(A) returned %i, expected 1\n", ret);

    str[0] = '\0';
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, sizeof(str), NULL, NULL);
    gle = GetLastError();
    ok(len > 0, "Could not convert %s (gle %lu)\n", wine_dbgstr_w(wstr), gle);

    character = 0;
    ret = ToAscii('A', SC_A, state, &character, 0);
    if (len == 1 || len == 2)
        ok(ret == 1, "ToAscii(A) returned %i, expected 1\n", ret);
    else
        /* ToAscii() can only return 2 chars => it fails if len > 2 */
        ok(ret == 0, "ToAscii(A) returned %i, expected 0\n", ret);
    switch ((ULONG_PTR)GetKeyboardLayout(0))
    {
    case 0x04090409: /* Qwerty */
    case 0x04070407: /* Qwertz */
    case 0x040c040c: /* Azerty */
        ok(lstrcmpW(wstr, L"a") == 0, "ToUnicode(A) returned %s\n", wine_dbgstr_w(wstr));
        ok(character == 'a', "ToAscii(A) returned char=%i, expected %i\n", character, 'a');
        break;
    /* Other keyboard layouts may or may not return 'a' */
    }

    state[VK_CONTROL] |= HIGHEST_BIT;
    state[VK_LCONTROL] |= HIGHEST_BIT;
    character = 0;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 1, "ToAscii for CTRL + Return key didn't return 1 (was %i)\n", ret);
    ok(character == '\n', "ToAscii for CTRL + Return was %i (expected 10)\n", character);

    character = 0;
    ret = ToAscii('A', SC_A, state, &character, 0);
    ok(ret == 1, "ToAscii for CTRL + character 'A' didn't return 1 (was %i)\n", ret);
    ok(character == 1, "ToAscii for CTRL + character 'A' was %i (expected 1)\n", character);

    state[VK_SHIFT] |= HIGHEST_BIT;
    state[VK_LSHIFT] |= HIGHEST_BIT;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 0, "ToAscii for CTRL + Shift + Return key didn't return 0 (was %i)\n", ret);

    ret = ToAscii(VK_RETURN, SC_RETURN, NULL, &character, 0);
    ok(ret == 0, "ToAscii for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAscii('A', SC_A, NULL, &character, 0);
    ok(ret == 0, "ToAscii for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAsciiEx(VK_RETURN, SC_RETURN, NULL, &character, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToAsciiEx for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAsciiEx('A', SC_A, NULL, &character, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToAsciiEx for NULL keystate didn't return 0 (was %i)\n", ret);
}

static void test_get_async_key_state(void)
{
    /* input value sanity checks */
    ok(0 == GetAsyncKeyState(1000000), "GetAsyncKeyState did not return 0\n");
    ok(0 == GetAsyncKeyState(-1000000), "GetAsyncKeyState did not return 0\n");
}

static HKL *get_keyboard_layouts( UINT *count )
{
    HKL *layouts;

    *count = GetKeyboardLayoutList( 0, NULL );
    ok_ne( 0, *count, UINT, "%u" );
    layouts = malloc( *count * sizeof(HKL) );
    ok_ne( NULL, layouts, void *, "%p" );
    *count = GetKeyboardLayoutList( *count, layouts );
    ok_ne( 0, *count, UINT, "%u" );

    return layouts;
}

static void test_keyboard_layout_name(void)
{
    WCHAR klid[KL_NAMELENGTH], tmpklid[KL_NAMELENGTH], layout_path[MAX_PATH], value[5];
    HKL layout, tmplayout, *layouts, *layouts_preload;
    DWORD status, value_size, klid_size, type, id;
    int i, j;
    HKEY hkey;
    UINT len;
    BOOL ret;

    if (0) /* crashes on native system */
        ret = GetKeyboardLayoutNameA(NULL);

    SetLastError(0xdeadbeef);
    ret = GetKeyboardLayoutNameW(NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_NOACCESS, "got %ld\n", GetLastError());

    layout = GetKeyboardLayout(0);
    if (broken( layout == (HKL)0x040a0c0a ))
    {
        /* The testbot w7u_es has a broken layout configuration, its active layout is 040a:0c0a,
         * with 0c0a its user locale and 040a its layout langid. Its layout preload list contains
         * a 00000c0a layout but the system layouts OTOH only contains the standard 0000040a layout.
         * Later, after activating 0409:0409 layout, GetKeyboardLayoutNameW returns 00000c0a.
         */
        win_skip( "broken keyboard layout, skipping tests\n" );
        return;
    }

    layouts = get_keyboard_layouts( &len );
    ok(layouts != NULL, "Could not allocate memory\n");

    layouts_preload = calloc(1, sizeof(HKL));
    ok(layouts_preload != NULL, "Could not allocate memory\n");

    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", &hkey ))
    {
        i = 0;
        type = REG_SZ;
        klid_size = sizeof(klid);
        value_size = ARRAY_SIZE(value);
        while (!RegEnumValueW( hkey, i++, value, &value_size, NULL, &type, (void *)&klid, &klid_size ))
        {
            klid_size = sizeof(klid);
            value_size = ARRAY_SIZE(value);
            layouts_preload = realloc( layouts_preload, (i + 1) * sizeof(*layouts_preload) );
            ok(layouts_preload != NULL, "Could not allocate memory\n");
            layouts_preload[i - 1] = UlongToHandle( wcstoul( klid, NULL, 16 ) );
            layouts_preload[i] = 0;

            id = (DWORD_PTR)layouts_preload[i - 1];
            if (id & 0x80000000) todo_wine_if(HIWORD(id) == 0xe001) ok((id & 0xf0000000) == 0xd0000000, "Unexpected preloaded keyboard layout high bits %#lx\n", id);
            else ok(!(id & 0xf0000000), "Unexpected preloaded keyboard layout high bits %#lx\n", id);
        }

        RegCloseKey( hkey );
    }

    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", &hkey ))
    {
        for (i = 0; layouts_preload[i]; ++i)
        {
            type = REG_SZ;
            klid_size = sizeof(klid);
            swprintf( tmpklid, KL_NAMELENGTH, L"%08x", HandleToUlong( layouts_preload[i] ) );
            if (!RegQueryValueExW( hkey, tmpklid, NULL, &type, (void *)&klid, &klid_size ))
            {
                layouts_preload[i] = UlongToHandle( wcstoul( klid, NULL, 16 ) );

                /* Substitute should contain the keyboard layout id, not the HKL high word */
                id = (DWORD_PTR)layouts_preload[i];
                ok(!(id & 0xf0000000), "Unexpected substitute keyboard layout high bits %#lx\n", id);
            }
            else
            {
                id = (DWORD_PTR)layouts_preload[i];
                ok(!(id & 0xf0000000), "Unexpected preloaded keyboard layout high bits %#lx\n", id);
            }
        }

        RegCloseKey( hkey );
    }

    for (i = len - 1; i >= 0; --i)
    {
        id = (DWORD_PTR)layouts[i];

        winetest_push_context( "%08lx", id );

        ActivateKeyboardLayout(layouts[i], 0);

        tmplayout = GetKeyboardLayout(0);
        todo_wine_if(tmplayout != layouts[i])
        ok( tmplayout == layouts[i], "Failed to activate keyboard layout\n");
        if (tmplayout != layouts[i])
        {
            winetest_pop_context();
            continue;
        }

        GetKeyboardLayoutNameW(klid);

        for (j = 0; layouts_preload[j]; ++j)
        {
            swprintf( tmpklid, KL_NAMELENGTH, L"%08X", layouts_preload[j] );
            if (!wcscmp( tmpklid, klid )) break;
        }
        ok(j < len, "Could not find keyboard layout %s in preload list\n", wine_dbgstr_w(klid));

        if (id & 0x80000000)
        {
            todo_wine ok((id >> 28) == 0xf, "hkl high bits %#lx, expected 0xf\n", id >> 28);

            value_size = sizeof(value);
            wcscpy(layout_path, L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\");
            wcscat(layout_path, klid);
            status = RegGetValueW(HKEY_LOCAL_MACHINE, layout_path, L"Layout Id", RRF_RT_REG_SZ, NULL, (void *)&value, &value_size);
            todo_wine ok(!status, "RegGetValueW returned %lx\n", status);
            ok(value_size == 5 * sizeof(WCHAR), "RegGetValueW returned size %ld\n", value_size);

            swprintf(tmpklid, KL_NAMELENGTH, L"%04X", (id >> 16) & 0x0fff);
            todo_wine ok(!wcsicmp(value, tmpklid), "RegGetValueW returned %s, expected %s\n", debugstr_w(value), debugstr_w(tmpklid));
        }
        else
        {
            swprintf(tmpklid, KL_NAMELENGTH, L"%08X", id >> 16);
            ok(!wcsicmp(klid, tmpklid), "GetKeyboardLayoutNameW returned %s, expected %s\n", debugstr_w(klid), debugstr_w(tmpklid));
        }

        ActivateKeyboardLayout(layout, 0);
        tmplayout = LoadKeyboardLayoutW(klid, KLF_ACTIVATE);

        /* The low word of HKL is the selected user lang and may be different as LoadKeyboardLayoutW also selects the default lang from the layout */
        ok(((UINT_PTR)tmplayout & ~0xffff) == ((UINT_PTR)layouts[i] & ~0xffff), "LoadKeyboardLayoutW returned %p, expected %p\n", tmplayout, layouts[i]);

        /* The layout name only depends on the keyboard layout: the high word of HKL. */
        GetKeyboardLayoutNameW(tmpklid);
        ok(!wcsicmp(klid, tmpklid), "GetKeyboardLayoutNameW returned %s, expected %s\n", debugstr_w(tmpklid), debugstr_w(klid));

        winetest_pop_context();
    }

    ActivateKeyboardLayout(layout, 0);

    free(layouts);
    free(layouts_preload);
}

static HKL expect_hkl;
static HKL change_hkl;
static int got_setfocus;

static LRESULT CALLBACK test_ActivateKeyboardLayout_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    ok( msg != WM_INPUTLANGCHANGEREQUEST, "got WM_INPUTLANGCHANGEREQUEST\n" );

    if (msg == WM_SETFOCUS) got_setfocus = 1;
    if (msg == WM_INPUTLANGCHANGE)
    {
        HKL layout = GetKeyboardLayout( 0 );
        CHARSETINFO info;
        WCHAR klidW[64];
        UINT codepage;
        LCID lcid;

        /* get keyboard layout lcid from its name, as the HKL might be aliased */
        GetKeyboardLayoutNameW( klidW );
        swscanf( klidW, L"%x", &lcid );
        lcid = LOWORD(lcid);

        if (!(HIWORD(layout) & 0x8000)) ok( lcid == HIWORD(layout), "got lcid %#lx\n", lcid );

        GetLocaleInfoA( lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                        (char *)&codepage, sizeof(codepage) );
        TranslateCharsetInfo( UlongToPtr( codepage ), &info, TCI_SRCCODEPAGE );

        ok( !got_setfocus, "got WM_SETFOCUS before WM_INPUTLANGCHANGE\n" );
        ok( layout == expect_hkl, "got layout %p\n", layout );
        ok( wparam == info.ciCharset || broken(wparam == 0 && (HIWORD(layout) & 0x8000)),
            "got wparam %#Ix\n", wparam );
        ok( lparam == (LPARAM)expect_hkl, "got lparam %#Ix\n", lparam );
        change_hkl = (HKL)lparam;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static DWORD CALLBACK test_ActivateKeyboardLayout_thread_proc( void *arg )
{
    ActivateKeyboardLayout( arg, 0 );
    return 0;
}

static void test_ActivateKeyboardLayout( char **argv )
{
    HKL layout, tmp_layout, *layouts;
    HWND hwnd1, hwnd2;
    HANDLE thread;
    UINT i, count;
    DWORD ret;

    layout = GetKeyboardLayout( 0 );
    if (broken( layout == (HKL)0x040a0c0a ))
    {
        /* The testbot w7u_es has a broken layout configuration, see test_keyboard_layout_name above. */
        win_skip( "broken keyboard layout, skipping tests\n" );
        return;
    }

    count = GetKeyboardLayoutList( 0, NULL );
    ok( count > 0, "GetKeyboardLayoutList returned %d\n", count );
    layouts = malloc( count * sizeof(HKL) );
    ok( layouts != NULL, "Could not allocate memory\n" );
    count = GetKeyboardLayoutList( count, layouts );
    ok( count > 0, "GetKeyboardLayoutList returned %d\n", count );

    hwnd1 = CreateWindowA( "static", "static", WS_VISIBLE | WS_POPUP,
                           100, 100, 100, 100, 0, NULL, NULL, NULL );
    ok( !!hwnd1, "CreateWindow failed, error %lu\n", GetLastError() );
    empty_message_queue();

    SetWindowLongPtrA( hwnd1, GWLP_WNDPROC, (LONG_PTR)test_ActivateKeyboardLayout_window_proc );

    for (i = 0; i < count; ++i)
    {
        BOOL broken_focus_activate = FALSE;
        HKL other_layout = layouts[i];

        winetest_push_context( "%08x / %08x", (UINT)(UINT_PTR)layout, (UINT)(UINT_PTR)other_layout );

        /* test WM_INPUTLANGCHANGE message */

        change_hkl = 0;
        expect_hkl = other_layout;
        got_setfocus = 0;
        ActivateKeyboardLayout( other_layout, 0 );
        if (other_layout == layout) ok( change_hkl == 0, "got change_hkl %p\n", change_hkl );
        else todo_wine ok( change_hkl == other_layout, "got change_hkl %p\n", change_hkl );
        change_hkl = expect_hkl = 0;

        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == other_layout, "got tmp_layout %p\n", tmp_layout );

        /* changing the layout from another thread doesn't send the message */

        thread = CreateThread( NULL, 0, test_ActivateKeyboardLayout_thread_proc, layout, 0, 0 );
        ret = WaitForSingleObject( thread, 1000 );
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
        CloseHandle( thread );

        /* and has no immediate effect */

        empty_message_queue();
        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == other_layout, "got tmp_layout %p\n", tmp_layout );

        /* but the change only takes effect after focus changes */

        hwnd2 = CreateWindowA( "static", "static", WS_VISIBLE | WS_POPUP,
                               100, 100, 100, 100, 0, NULL, NULL, NULL );
        ok( !!hwnd2, "CreateWindow failed, error %lu\n", GetLastError() );

        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == layout || broken(layout != other_layout && tmp_layout == other_layout) /* w7u */,
            "got tmp_layout %p\n", tmp_layout );
        if (broken(layout != other_layout && tmp_layout == other_layout))
        {
            win_skip( "Broken layout activation on focus change, skipping some tests\n" );
            broken_focus_activate = TRUE;
        }
        empty_message_queue();

        /* only the focused window receives the WM_INPUTLANGCHANGE message */

        ActivateKeyboardLayout( other_layout, 0 );
        ok( change_hkl == 0, "got change_hkl %p\n", change_hkl );

        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == other_layout, "got tmp_layout %p\n", tmp_layout );

        thread = CreateThread( NULL, 0, test_ActivateKeyboardLayout_thread_proc, layout, 0, 0 );
        ret = WaitForSingleObject( thread, 1000 );
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
        CloseHandle( thread );

        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == other_layout, "got tmp_layout %p\n", tmp_layout );

        /* changing focus is enough for the layout change to take effect */

        change_hkl = 0;
        expect_hkl = layout;
        got_setfocus = 0;
        SetFocus( hwnd1 );

        if (broken_focus_activate)
        {
            ok( got_setfocus == 1, "got got_setfocus %d\n", got_setfocus );
            ok( change_hkl == 0, "got change_hkl %p\n", change_hkl );
            got_setfocus = 0;
            ActivateKeyboardLayout( layout, 0 );
        }

        if (other_layout == layout) ok( change_hkl == 0, "got change_hkl %p\n", change_hkl );
        else todo_wine ok( change_hkl == layout, "got change_hkl %p\n", change_hkl );
        change_hkl = expect_hkl = 0;

        tmp_layout = GetKeyboardLayout( 0 );
        todo_wine_if(layout != other_layout)
        ok( tmp_layout == layout, "got tmp_layout %p\n", tmp_layout );

        DestroyWindow( hwnd2 );
        empty_message_queue();

        winetest_pop_context();
    }

    DestroyWindow( hwnd1 );

    free( layouts );
}

static void test_key_names(void)
{
    char buffer[40];
    WCHAR bufferW[40];
    int ret, prev;
    LONG lparam = 0x1d << 16;

    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetKeyNameTextA( lparam, buffer, sizeof(buffer) );
    ok( ret > 0, "wrong len %u for '%s'\n", ret, buffer );
    ok( ret == strlen(buffer), "wrong len %u for '%s'\n", ret, buffer );

    memset( buffer, 0xcc, sizeof(buffer) );
    prev = ret;
    ret = GetKeyNameTextA( lparam, buffer, prev );
    ok( ret == prev - 1, "wrong len %u for '%s'\n", ret, buffer );
    ok( ret == strlen(buffer), "wrong len %u for '%s'\n", ret, buffer );

    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetKeyNameTextA( lparam, buffer, 0 );
    ok( ret == 0, "wrong len %u for '%s'\n", ret, buffer );
    ok( buffer[0] == 0, "wrong string '%s'\n", buffer );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    ret = GetKeyNameTextW( lparam, bufferW, ARRAY_SIZE(bufferW));
    ok( ret > 0, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( ret == lstrlenW(bufferW), "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    prev = ret;
    ret = GetKeyNameTextW( lparam, bufferW, prev );
    ok( ret == prev - 1, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( ret == lstrlenW(bufferW), "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    ret = GetKeyNameTextW( lparam, bufferW, 0 );
    ok( ret == 0, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( bufferW[0] == 0xcccc, "wrong string %s\n", wine_dbgstr_w(bufferW) );
}

static void simulate_click(BOOL left, int x, int y)
{
    INPUT input[2];
    UINT events_no;

    SetCursorPos(x, y);
    memset(input, 0, sizeof(input));
    input[0].type = INPUT_MOUSE;
    input[0].mi.dx = x;
    input[0].mi.dy = y;
    input[0].mi.dwFlags = left ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dx = x;
    input[1].mi.dy = y;
    input[1].mi.dwFlags = left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
    events_no = SendInput(2, input, sizeof(input[0]));
    ok(events_no == 2, "SendInput returned %d\n", events_no);
}

static BOOL wait_for_event(HANDLE event, int timeout)
{
    DWORD end_time = GetTickCount() + timeout;
    MSG msg;

    do {
        if(MsgWaitForMultipleObjects(1, &event, FALSE, timeout, QS_ALLINPUT) == WAIT_OBJECT_0)
            return TRUE;
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        timeout = end_time - GetTickCount();
    }while(timeout > 0);

    return FALSE;
}

static LRESULT CALLBACK httransparent_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    append_message( hwnd, msg, wparam, lparam );
    if (msg == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

struct create_transparent_window_params
{
    HANDLE start_event;
    HANDLE end_event;
    HWND hwnd;
};

static DWORD WINAPI create_transparent_window_thread(void *arg)
{
    struct create_transparent_window_params *params = arg;
    ULONG_PTR old_proc;

    params->hwnd = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, params->hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    old_proc = SetWindowLongPtrW( params->hwnd, GWLP_WNDPROC, (LONG_PTR)httransparent_wndproc );
    ok_ne( 0, old_proc, LONG_PTR, "%#Ix" );

    ok_ret( 1, SetEvent( params->start_event ) );
    wait_for_event( params->end_event, 5000 );

    ok_ret( 1, DestroyWindow( params->hwnd ) );
    wait_messages( 100, FALSE );
    return 0;
}

static LRESULT CALLBACK mouse_move_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static DWORD last_x = 50, expect_x = 60;

    if (msg == WM_MOUSEMOVE)
    {
        POINT pt = {LOWORD(lparam), HIWORD(lparam)};
        MapWindowPoints(hwnd, NULL, &pt, 1);

        flaky
        if (pt.x != last_x) ok( pt.x == expect_x, "got unexpected WM_MOUSEMOVE x %ld, expected %ld\n", pt.x, expect_x );

        expect_x = pt.x == 50 ? 60 : 50;
        last_x = pt.x;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK mouse_layered_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    append_message( hwnd, msg, wparam, lparam );

    if (msg == WM_PAINT)
    {
        HBRUSH brush = CreateSolidBrush( RGB(50, 100, 255) );
        PAINTSTRUCT paint;
        RECT client_rect;

        HDC hdc = BeginPaint( hwnd, &paint );
        GetClientRect( hwnd, &client_rect );
        FillRect( hdc, &client_rect, brush );
        EndPaint( hwnd, &paint );

        DeleteObject( brush );
        return 0;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

struct send_input_mouse_test
{
    BOOL set_cursor_pos;
    POINT cursor_pos;
    UINT data;
    UINT flags;
    ULONG_PTR extra;
    struct user_call expect[8];
    BYTE expect_state[256];
    BOOL todo_state[256];
};

static LRESULT CALLBACK ll_hook_ms_proc( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook_info = (MSLLHOOKSTRUCT *)lparam;
    if (code == HC_ACTION) append_ll_hook_ms( wparam, hook_info );
    return CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK ll_hook_setpos_proc( int code, WPARAM wparam, LPARAM lparam )
{
    if (code == HC_ACTION)
    {
        MSLLHOOKSTRUCT *hook_info = (MSLLHOOKSTRUCT *)lparam;
        POINT pos, hook_pos = {40, 40};

        ok( abs( hook_info->pt.x - 51 ) <= 1, "got x %ld\n", hook_info->pt.x );
        ok( abs( hook_info->pt.y - 49 ) <= 1, "got y %ld\n", hook_info->pt.y );

        /* spuriously moves by 0 or 1 pixels on Windows */
        ok_ret( 1, GetCursorPos( &pos ) );
        ok( abs( pos.x - 51 ) <= 1, "got x %ld\n", pos.x );
        ok( abs( pos.y - 49 ) <= 1, "got y %ld\n", pos.y );

        ok_ret( 1, SetCursorPos( 60, 60 ) );
        hook_info->pt = hook_pos;
    }

    return CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK ll_hook_getpos_proc( int code, WPARAM wparam, LPARAM lparam )
{
    if (code == HC_ACTION)
    {
        MSLLHOOKSTRUCT *hook_info = (MSLLHOOKSTRUCT *)lparam;
        POINT pos, expect_pos = {60, 60}, hook_pos = {40, 40};
        ok_point( hook_pos, hook_info->pt );
        ok_ret( 1, GetCursorPos( &pos ) );
        ok_point( expect_pos, pos );
    }

    return CallNextHookEx( 0, code, wparam, lparam );
}

static BOOL accept_mouse_messages_nomove( UINT msg )
{
    return is_mouse_message( msg ) && msg != WM_MOUSEMOVE;
}

static void test_SendInput_mouse_messages(void)
{
#define WIN_MSG(m, h, w, l, ...) {.func = MSG_TEST_WIN, .message = {.msg = m, .hwnd = h, .wparam = w, .lparam = l}, ## __VA_ARGS__}
#define MS_HOOK(m, x, y, ...) {.func = LL_HOOK_MOUSE, .ll_hook_ms = {.msg = m, .point = {x, y}, .flags = 1}, ## __VA_ARGS__}
    struct user_call mouse_move[] =
    {
        MS_HOOK(WM_MOUSEMOVE, 40, 40),
        {0},
    };
    struct user_call button_down_hwnd[] =
    {
        MS_HOOK(WM_LBUTTONDOWN, 50, 50),
        WIN_MSG(WM_LBUTTONDOWN, (HWND)-1/*hwnd*/, 0x1, MAKELONG(50, 50)),
        {0},
    };
    struct user_call button_down_hwnd_todo[] =
    {
        MS_HOOK(WM_LBUTTONDOWN, 50, 50),
        WIN_MSG(WM_LBUTTONDOWN, (HWND)-1/*hwnd*/, 0x1, MAKELONG(50, 50), .todo = TRUE),
        {0/* placeholder for Wine spurious messages */},
        {0},
    };
    struct user_call button_up_hwnd[] =
    {
        MS_HOOK(WM_LBUTTONUP, 50, 50),
        WIN_MSG(WM_LBUTTONUP, (HWND)-1/*hwnd*/, 0, MAKELONG(50, 50)),
        {0},
    };
    struct user_call button_up_hwnd_todo[] =
    {
        MS_HOOK(WM_LBUTTONUP, 50, 50),
        WIN_MSG(WM_LBUTTONUP, (HWND)-1/*hwnd*/, 0, MAKELONG(50, 50), .todo = TRUE),
        {0/* placeholder for Wine spurious messages */},
        {0},
    };
    struct user_call button_down_no_message[] =
    {
        MS_HOOK(WM_LBUTTONDOWN, 50, 50),
        {.todo = TRUE /* spurious message on Wine */},
        {0},
    };
    struct user_call button_up_no_message[] =
    {
        MS_HOOK(WM_LBUTTONUP, 50, 50),
        {.todo = TRUE /* spurious message on Wine */},
        {0},
    };
#undef WIN_MSG
#undef MS_HOOK
    static const POINT expect_60x50 = {60, 50}, expect_50x50 = {50, 50};

    static const struct layered_test
    {
        UINT color;
        UINT alpha;
        UINT flags;
        BOOL expect_click;
    }
    layered_tests[] =
    {
        {.flags = LWA_ALPHA, .expect_click = FALSE},
        {.alpha = 1, .flags = LWA_ALPHA, .expect_click = TRUE},
        {.color = RGB(0, 255, 0), .flags = LWA_COLORKEY, .expect_click = TRUE},
        {.color = RGB(50, 100, 255), .flags = LWA_COLORKEY, .expect_click = TRUE},
        {.color = RGB(0, 255, 0), .flags = LWA_COLORKEY | LWA_ALPHA, .expect_click = FALSE},
        {.color = RGB(0, 255, 0), .alpha = 1, .flags = LWA_COLORKEY | LWA_ALPHA, .expect_click = TRUE},
        {.color = RGB(50, 100, 255), .alpha = 1, .flags = LWA_COLORKEY | LWA_ALPHA, .expect_click = TRUE},
    };

    struct create_transparent_window_params params = {0};
    ULONG_PTR old_proc, old_other_proc;
    RECT clip_rect = {55, 55, 55, 55};
    UINT dblclk_time, i;
    HWND hwnd, other;
    DWORD thread_id;
    HANDLE thread;
    HHOOK hook, hook_setpos, hook_getpos;
    HRGN hregion;
    RECT region;
    POINT pt;

    params.start_event = CreateEventA( NULL, FALSE, FALSE, NULL );
    ok( !!params.start_event, "CreateEvent failed\n" );
    params.end_event = CreateEventA( NULL, FALSE, FALSE, NULL );
    ok( !!params.end_event, "CreateEvent failed\n" );


    dblclk_time = GetDoubleClickTime();
    ok_eq( 500, dblclk_time, UINT, "%u" );

    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, SetDoubleClickTime( 1 ) );

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    trace( "hwnd %p\n", hwnd );

    hook = SetWindowsHookExW( WH_MOUSE_LL, ll_hook_ms_proc, GetModuleHandleW( NULL ), 0 );
    ok_ne( NULL, hook, HHOOK, "%p" );

    /* test hooks only, WM_MOUSEMOVE messages are very brittle */
    p_accept_message = is_mouse_message;
    ok_seq( empty_sequence );

    /* SetCursorPos or ClipCursor don't call mouse ll hooks */
    ok_ret( 1, SetCursorPos( 60, 60 ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, ClipCursor( &clip_rect ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, ClipCursor( NULL ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    wait_messages( 100, FALSE );
    todo_wine ok_seq( empty_sequence );

    hook_getpos = SetWindowsHookExW( WH_MOUSE_LL, ll_hook_getpos_proc, GetModuleHandleW( NULL ), 0 );
    ok_ne( NULL, hook_getpos, HHOOK, "%p" );
    hook_setpos = SetWindowsHookExW( WH_MOUSE_LL, ll_hook_setpos_proc, GetModuleHandleW( NULL ), 0 );
    ok_ne( NULL, hook_setpos, HHOOK, "%p" );

    mouse_event( MOUSEEVENTF_MOVE, 0, 0, 0, 0 );
    /* recent Windows versions don't call the hooks with no movement */
    if (current_sequence_len)
    {
        ok_seq( mouse_move );
        ok_ret( 1, SetCursorPos( 50, 50 ) );
    }
    ok_seq( empty_sequence );

    /* WH_MOUSE_LL hook is called even with 1 pixel moves */
    mouse_event( MOUSEEVENTF_MOVE, +1, -1, 0, 0 );
    ok_seq( mouse_move );

    ok_ret( 1, UnhookWindowsHookEx( hook_getpos ) );
    ok_ret( 1, UnhookWindowsHookEx( hook_setpos ) );


    append_message_hwnd = TRUE;
    p_accept_message = accept_mouse_messages_nomove;
    ok_seq( empty_sequence );


    /* basic button messages */

    old_proc = SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_proc, LONG_PTR, "%#Ix" );

    ok_ret( 1, SetCursorPos( 50, 50 ) );
    wait_messages( 100, FALSE );
    todo_wine ok_seq( empty_sequence );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd );

    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );


    /* click through top-level window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = other;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = other;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );


    /* click through child window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 100, 100, hwnd, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = other;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = other;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );


    /* click through HTTRANSPARENT top-level window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)httransparent_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd_todo[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd_todo );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd_todo[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd_todo );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );


    /* click through HTTRANSPARENT child window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 100, 100, hwnd, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)httransparent_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );


    /* click on HTTRANSPARENT top-level window that belongs to other thread */

    thread = CreateThread( NULL, 0, create_transparent_window_thread, &params, 0, NULL );
    ok_ne( NULL, thread, HANDLE, "%p" );

    ok_ret( 0, WaitForSingleObject( params.start_event, 5000 ) );
    ok_ret( 0, SendMessageW( params.hwnd, WM_USER, 0, 0 ) );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    ok_seq( button_down_no_message );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    ok_seq( button_up_no_message );

    ok_ret( 1, SetEvent( params.end_event ) );
    ok_ret( 0, WaitForSingleObject( thread, 5000 ) );
    ok_ret( 1, CloseHandle( thread ) );
    ResetEvent( params.start_event );
    ResetEvent( params.end_event );


    /* click on HTTRANSPARENT top-level window that belongs to other thread,
     * thread input queues are attached */

    thread = CreateThread( NULL, 0, create_transparent_window_thread, &params, 0, &thread_id );
    ok_ne( NULL, thread, HANDLE, "%p" );

    ok_ret( 0, WaitForSingleObject( params.start_event, 5000 ) );
    ok_ret( 1, AttachThreadInput( thread_id, GetCurrentThreadId(), TRUE ) );
    ok_ret( 0, SendMessageW( params.hwnd, WM_USER, 0, 0 ) );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd_todo[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd_todo );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );

    ok_ret( 1, AttachThreadInput( thread_id, GetCurrentThreadId(), FALSE ) );
    ok_ret( 1, SetEvent( params.end_event ) );
    ok_ret( 0, WaitForSingleObject( thread, 5000 ) );
    ok_ret( 1, CloseHandle( thread ) );
    ResetEvent( params.start_event );
    ResetEvent( params.end_event );


    /* click on top-level window with SetCapture called for the underlying window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    SetCapture( hwnd );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );

    SetCapture( 0 );


    /* click through child window with SetCapture called for the underlying window */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_CHILD, 0, 0, 100, 100, hwnd, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    SetCapture( hwnd );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );

    SetCapture( 0 );



    /* click through layered window with alpha channel / color key */

    for (i = 0; i < ARRAY_SIZE(layered_tests); i++)
    {
        const struct layered_test *test = layered_tests + i;
        BOOL ret;

        winetest_push_context( "layered %u", i );

        other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
        ok_ne( NULL, other, HWND, "%p" );
        old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)mouse_layered_wndproc );
        ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );
        wait_messages( 100, FALSE );

        ok_ret( ERROR, GetWindowRgnBox( other, &region ) );


        SetWindowLongW( other, GWL_EXSTYLE, GetWindowLongW( other, GWL_EXSTYLE ) | WS_EX_LAYERED );
        ret = SetLayeredWindowAttributes( other, test->color, test->alpha, test->flags );
        if (broken(!ret)) /* broken on Win7 probably because of the separate desktop */
        {
            win_skip("Skipping broken SetLayeredWindowAttributes tests\n");
            winetest_pop_context();
            DestroyWindow( other );
            break;
        }

        ok_ret( 1, RedrawWindow( other, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN ) );
        wait_messages( 5, FALSE );
        current_sequence_len = 0;

        mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
        wait_messages( 5, FALSE );
        button_down_hwnd_todo[1].message.hwnd = test->expect_click ? other : hwnd;
        button_down_hwnd_todo[1].todo = !test->expect_click;
        button_down_hwnd_todo[2].todo = !test->alpha && (test->flags & LWA_ALPHA);
        ok_seq( button_down_hwnd_todo );
        mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
        wait_messages( 5, FALSE );
        button_up_hwnd_todo[1].message.hwnd = test->expect_click ? other : hwnd;
        button_up_hwnd_todo[1].todo = !test->expect_click;
        button_up_hwnd_todo[2].todo = !test->alpha && (test->flags & LWA_ALPHA);
        ok_seq( button_up_hwnd_todo );


        /* removing the attribute isn't enough to get mouse input again? */

        SetWindowLongW( other, GWL_EXSTYLE, GetWindowLongW( other, GWL_EXSTYLE ) & ~WS_EX_LAYERED );
        ok_ret( 1, RedrawWindow( other, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN ) );
        wait_messages( 5, FALSE );

        mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
        wait_messages( 5, FALSE );
        button_down_hwnd_todo[1].message.hwnd = test->expect_click ? other : hwnd;
        button_down_hwnd_todo[1].todo = !test->expect_click;
        button_down_hwnd_todo[2].todo = !test->alpha && (test->flags & LWA_ALPHA);
        ok_seq( button_down_hwnd_todo );
        mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
        wait_messages( 5, FALSE );
        button_up_hwnd_todo[1].message.hwnd = test->expect_click ? other : hwnd;
        button_up_hwnd_todo[1].todo = !test->expect_click;
        button_up_hwnd_todo[2].todo = !test->alpha && (test->flags & LWA_ALPHA);
        ok_seq( button_up_hwnd_todo );

        ok_ret( 1, DestroyWindow( other ) );

        winetest_pop_context();
    }


    /* click on top-level window with SetWindowRgn called */

    other = CreateWindowW( L"static", NULL, WS_VISIBLE | WS_POPUP, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    current_sequence_len = 0;

    old_other_proc = SetWindowLongPtrW( other, GWLP_WNDPROC, (LONG_PTR)append_message_wndproc );
    ok_ne( 0, old_other_proc, LONG_PTR, "%#Ix" );

    hregion = CreateRectRgn( 0, 0, 10, 10 );
    ok_ne( NULL, hregion, HRGN, "%p" );
    ok_ret( 1, SetWindowRgn( other, hregion, TRUE ) );
    DeleteObject( hregion );
    ok_ret( SIMPLEREGION, GetWindowRgnBox( other, &region ) );

    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_down_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_down_hwnd );
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    wait_messages( 5, FALSE );
    button_up_hwnd[1].message.hwnd = hwnd;
    ok_seq( button_up_hwnd );

    ok_ret( 1, DestroyWindow( other ) );
    wait_messages( 0, FALSE );


    /* warm up test case by moving cursor and window a bit first */
    ok_ret( 1, SetCursorPos( 60, 50 ) );
    ok_ret( 1, SetWindowPos( hwnd, NULL, 10, 0, 0, 0, SWP_NOSIZE ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, SetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE ) );
    wait_messages( 5, FALSE );
    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)mouse_move_wndproc );

    ok_ret( 1, SetCursorPos( 60, 50 ) );
    ok_ret( 1, SetWindowPos( hwnd, NULL, 10, 0, 0, 0, SWP_NOSIZE ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, GetCursorPos( &pt ) );
    ok_point( expect_60x50, pt );

    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, SetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE ) );
    wait_messages( 5, FALSE );
    ok_ret( 1, GetCursorPos( &pt ) );
    ok_point( expect_50x50, pt );


    ok_ret( 1, UnhookWindowsHookEx( hook ) );

    wait_messages( 100, FALSE );
    todo_wine ok_seq( empty_sequence );
    p_accept_message = NULL;
    append_message_hwnd = FALSE;


    ok_ret( 1, DestroyWindow( hwnd ) );

    CloseHandle( params.start_event );
    CloseHandle( params.end_event );


    ok_ret( 1, SetDoubleClickTime( dblclk_time ) );
}


static LRESULT WINAPI MsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_USER+1)
    {
        HWND hwnd = (HWND)lParam;
        ok(GetFocus() == hwnd, "thread expected focus %p, got %p\n", hwnd, GetFocus());
        ok(GetActiveWindow() == hwnd, "thread expected active %p, got %p\n", hwnd, GetActiveWindow());
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

struct wnd_event
{
    HWND hwnd;
    HANDLE wait_event;
    HANDLE start_event;
    DWORD attach_from;
    DWORD attach_to;
    BOOL setWindows;
};

static DWORD WINAPI thread_proc(void *param)
{
    MSG msg;
    struct wnd_event *wnd_event = param;
    BOOL ret;

    if (wnd_event->wait_event)
    {
        ok(WaitForSingleObject(wnd_event->wait_event, INFINITE) == WAIT_OBJECT_0,
           "WaitForSingleObject failed\n");
        CloseHandle(wnd_event->wait_event);
    }

    if (wnd_event->attach_from)
    {
        ret = AttachThreadInput(wnd_event->attach_from, GetCurrentThreadId(), TRUE);
        ok(ret, "AttachThreadInput error %ld\n", GetLastError());
    }

    if (wnd_event->attach_to)
    {
        ret = AttachThreadInput(GetCurrentThreadId(), wnd_event->attach_to, TRUE);
        ok(ret, "AttachThreadInput error %ld\n", GetLastError());
    }

    wnd_event->hwnd = CreateWindowExA(0, "TestWindowClass", "window caption text", WS_OVERLAPPEDWINDOW,
                                      100, 100, 200, 200, 0, 0, 0, NULL);
    ok(wnd_event->hwnd != 0, "Failed to create overlapped window\n");

    if (wnd_event->setWindows)
    {
        SetFocus(wnd_event->hwnd);
        SetActiveWindow(wnd_event->hwnd);
    }

    SetEvent(wnd_event->start_event);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

static void test_attach_input(void)
{
    HANDLE hThread;
    HWND ourWnd, Wnd2;
    DWORD ret, tid;
    struct wnd_event wnd_event;
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = MsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorW( NULL, (LPCWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&cls)) return;

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = FALSE;
    if (!wnd_event.start_event)
    {
        win_skip("skipping interthread message test under win9x\n");
        return;
    }

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ourWnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL);
    ok(ourWnd!= 0, "failed to create ourWnd window\n");

    Wnd2 = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL);
    ok(Wnd2!= 0, "failed to create Wnd2 window\n");

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());
    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());
    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)ourWnd);

    SetActiveWindow(Wnd2);
    SetFocus(Wnd2);
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)Wnd2);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %ld\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = TRUE;

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == 0, "expected active 0, got %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    SetFocus(Wnd2);
    SetActiveWindow(Wnd2);
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)Wnd2);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %ld\n", GetLastError());

    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %ld\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = TRUE;

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(!ret, "AttachThreadInput succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef) /* <= Win XP */,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(tid, GetCurrentThreadId(), TRUE);
    ok(!ret, "AttachThreadInput succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef) /* <= Win XP */,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetEvent(wnd_event.wait_event);

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %ld\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = GetCurrentThreadId();
    wnd_event.attach_to = 0;
    wnd_event.setWindows = FALSE;

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %ld\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = GetCurrentThreadId();
    wnd_event.setWindows = FALSE;

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %ld\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %ld\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);
    DestroyWindow(ourWnd);
    DestroyWindow(Wnd2);
}

struct get_key_state_test_desc
{
    BOOL peek_message;
    BOOL peek_message_main;
    BOOL set_keyboard_state_main;
    BOOL set_keyboard_state;
};

struct get_key_state_test_desc get_key_state_tests[] =
{
    /* 0: not peeking in thread, no msg queue: GetKeyState / GetKeyboardState miss key press */
    {FALSE,  TRUE, FALSE, FALSE},
    /* 1: peeking on thread init, not in main: GetKeyState / GetKeyboardState catch key press */
    /*    - GetKeyboardState misses key press if called before GetKeyState */
    /*    - GetKeyboardState catches key press, if called after GetKeyState */
    { TRUE, FALSE, FALSE, FALSE},
    /* 2: peeking on thread init, and in main: GetKeyState / GetKeyboardState catch key press */
    { TRUE,  TRUE, FALSE, FALSE},

    /* same tests but with SetKeyboardState called in main thread */
    /* 3: not peeking in thread, no msg queue: GetKeyState / GetKeyboardState miss key press */
    {FALSE,  TRUE,  TRUE, FALSE},
    /* 4: peeking on thread init, not in main: GetKeyState / GetKeyboardState catch key press */
    /*    - GetKeyboardState misses key press if called before GetKeyState */
    /*    - GetKeyboardState catches key press, if called after GetKeyState */
    { TRUE, FALSE,  TRUE, FALSE},
    /* 5: peeking on thread init, and in main: GetKeyState / GetKeyboardState catch key press */
    { TRUE,  TRUE,  TRUE, FALSE},

    /* same tests but with SetKeyboardState called in other thread */
    /* 6: not peeking in thread, no msg queue: GetKeyState / GetKeyboardState miss key press */
    {FALSE,  TRUE,  TRUE,  TRUE},
    /* 7: peeking on thread init, not in main: GetKeyState / GetKeyboardState catch key press */
    /*    - GetKeyboardState misses key press if called before GetKeyState */
    /*    - GetKeyboardState catches key press, if called after GetKeyState */
    { TRUE, FALSE,  TRUE,  TRUE},
    /* 8: peeking on thread init, and in main: GetKeyState / GetKeyboardState catch key press */
    { TRUE,  TRUE,  TRUE,  TRUE},
};

struct get_key_state_thread_params
{
    HANDLE semaphores[2];
    int index;
};

#define check_get_keyboard_state(i, j, c, x) check_get_keyboard_state_(i, j, c, x, __LINE__)
static void check_get_keyboard_state_(int i, int j, int c, int x, int line)
{
    unsigned char keystate[256];
    BOOL ret;

    memset(keystate, 0, sizeof(keystate));
    ret = GetKeyboardState(keystate);
    ok_(__FILE__, line)(ret, "GetKeyboardState failed, %lu\n", GetLastError());
    ok_(__FILE__, line)(!(keystate['X'] & 0x80) == !x, "%d:%d: expected that X keystate is %s\n", i, j, x ? "set" : "unset");
    ok_(__FILE__, line)(!(keystate['C'] & 0x80) == !c, "%d:%d: expected that C keystate is %s\n", i, j, c ? "set" : "unset");

    /* calling it twice shouldn't change */
    memset(keystate, 0, sizeof(keystate));
    ret = GetKeyboardState(keystate);
    ok_(__FILE__, line)(ret, "GetKeyboardState failed, %lu\n", GetLastError());
    ok_(__FILE__, line)(!(keystate['X'] & 0x80) == !x, "%d:%d: expected that X keystate is %s\n", i, j, x ? "set" : "unset");
    ok_(__FILE__, line)(!(keystate['C'] & 0x80) == !c, "%d:%d: expected that C keystate is %s\n", i, j, c ? "set" : "unset");
}

#define check_get_key_state(i, j, c, x) check_get_key_state_(i, j, c, x, __LINE__)
static void check_get_key_state_(int i, int j, int c, int x, int line)
{
    SHORT state;

    state = GetKeyState('X');
    ok_(__FILE__, line)(!(state & 0x8000) == !x, "%d:%d: expected that X highest bit is %s, got %#x\n", i, j, x ? "set" : "unset", state);
    ok_(__FILE__, line)(!(state & 0x007e), "%d:%d: expected that X undefined bits are unset, got %#x\n", i, j, state);

    state = GetKeyState('C');
    ok_(__FILE__, line)(!(state & 0x8000) == !c, "%d:%d: expected that C highest bit is %s, got %#x\n", i, j, c ? "set" : "unset", state);
    ok_(__FILE__, line)(!(state & 0x007e), "%d:%d: expected that C undefined bits are unset, got %#x\n", i, j, state);
}

static DWORD WINAPI get_key_state_thread(void *arg)
{
    struct get_key_state_thread_params *params = arg;
    struct get_key_state_test_desc* test;
    HANDLE *semaphores = params->semaphores;
    DWORD result;
    BYTE keystate[256] = {0};
    BOOL has_queue;
    BOOL expect_x, expect_c;
    MSG msg;
    int i = params->index, j;

    test = get_key_state_tests + i;
    has_queue = test->peek_message || test->set_keyboard_state;

    if (test->peek_message)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            ok(!is_keyboard_message(msg.message), "%d: PeekMessageA got keyboard message.\n", i);
    }

    for (j = 0; j < 4; ++j)
    {
        /* initialization */
        ReleaseSemaphore(semaphores[0], 1, NULL);
        result = WaitForSingleObject(semaphores[1], 1000);
        ok(result == WAIT_OBJECT_0, "%d:%d: WaitForSingleObject returned %lu\n", i, j, result);

        if (test->set_keyboard_state)
        {
            keystate['C'] = 0xff;
            SetKeyboardState(keystate);
        }

        /* key pressed */
        ReleaseSemaphore(semaphores[0], 1, NULL);
        result = WaitForSingleObject(semaphores[1], 1000);
        ok(result == WAIT_OBJECT_0, "%d:%d: WaitForSingleObject returned %lu\n", i, j, result);

        if (test->set_keyboard_state) expect_x = TRUE;
        else if (!has_queue && j == 0) expect_x = FALSE;
        else expect_x = TRUE;

        if (test->set_keyboard_state) expect_c = TRUE;
        else expect_c = FALSE;

        check_get_keyboard_state(i, j, expect_c, FALSE);
        check_get_key_state(i, j, expect_c, expect_x);
        check_get_keyboard_state(i, j, expect_c, expect_x);

        /* key released */
        ReleaseSemaphore(semaphores[0], 1, NULL);
        result = WaitForSingleObject(semaphores[1], 1000);
        ok(result == WAIT_OBJECT_0, "%d: WaitForSingleObject returned %lu\n", i, result);

        check_get_keyboard_state(i, j, expect_c, expect_x);
        check_get_key_state(i, j, expect_c, FALSE);
        check_get_keyboard_state(i, j, expect_c, FALSE);
    }

    return 0;
}

static void test_GetKeyState(void)
{
    struct get_key_state_thread_params params;
    HANDLE thread;
    DWORD result;
    BYTE keystate[256] = {0};
    BOOL expect_x, expect_c;
    HWND hwnd;
    MSG msg;
    int i, j;

    BOOL us_kbd = (GetKeyboardLayout(0) == (HKL)(ULONG_PTR)0x04090409);
    if (!us_kbd)
    {
        skip("skipping test with inconsistent results on non-us keyboard\n");
        return;
    }

    params.semaphores[0] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(params.semaphores[0] != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());
    params.semaphores[1] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(params.semaphores[1] != NULL, "CreateSemaphoreA failed %lu\n", GetLastError());

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowA failed %lu\n", GetLastError());
    empty_message_queue();

    for (i = 0; i < ARRAY_SIZE(get_key_state_tests); ++i)
    {
        struct get_key_state_test_desc* test = get_key_state_tests + i;

        params.index = i;
        thread = CreateThread(NULL, 0, get_key_state_thread, &params, 0, NULL);
        ok(thread != NULL, "CreateThread failed %lu\n", GetLastError());

        for (j = 0; j < 4; ++j)
        {
            /* initialization */
            result = WaitForSingleObject(params.semaphores[0], 1000);
            ok(result == WAIT_OBJECT_0, "%d:%d: WaitForSingleObject returned %lu\n", i, j, result);

            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            empty_message_queue();

            ReleaseSemaphore(params.semaphores[1], 1, NULL);

            /* key pressed */
            result = WaitForSingleObject(params.semaphores[0], 1000);
            ok(result == WAIT_OBJECT_0, "%d:%d: WaitForSingleObject returned %lu\n", i, j, result);

            keybd_event('X', 0, 0, 0);
            if (test->set_keyboard_state_main)
            {
                expect_c = TRUE;
                keystate['C'] = 0xff;
                SetKeyboardState(keystate);
            }
            else expect_c = FALSE;

            check_get_keyboard_state(i, j, expect_c, FALSE);
            check_get_key_state(i, j, expect_c, FALSE);
            check_get_keyboard_state(i, j, expect_c, FALSE);

            if (test->peek_message_main) while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

            if (test->peek_message_main) expect_x = TRUE;
            else expect_x = FALSE;

            check_get_keyboard_state(i, j, expect_c, expect_x);
            check_get_key_state(i, j, expect_c, expect_x);
            check_get_keyboard_state(i, j, expect_c, expect_x);

            ReleaseSemaphore(params.semaphores[1], 1, NULL);

            /* key released */
            result = WaitForSingleObject(params.semaphores[0], 1000);
            ok(result == WAIT_OBJECT_0, "%d:%d: WaitForSingleObject returned %lu\n", i, j, result);

            keybd_event('X', 0, KEYEVENTF_KEYUP, 0);
            if (test->set_keyboard_state_main)
            {
                expect_x = FALSE;
                keystate['C'] = 0x00;
                SetKeyboardState(keystate);
            }

            check_get_keyboard_state(i, j, FALSE, expect_x);
            check_get_key_state(i, j, FALSE, expect_x);
            check_get_keyboard_state(i, j, FALSE, expect_x);

            if (test->peek_message_main) while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);

            check_get_keyboard_state(i, j, FALSE, FALSE);
            check_get_key_state(i, j, FALSE, FALSE);
            check_get_keyboard_state(i, j, FALSE, FALSE);

            ReleaseSemaphore(params.semaphores[1], 1, NULL);
        }

        result = WaitForSingleObject(thread, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", result);
        CloseHandle(thread);
    }

    DestroyWindow(hwnd);
    CloseHandle(params.semaphores[0]);
    CloseHandle(params.semaphores[1]);
}

static void test_OemKeyScan(void)
{
    DWORD ret, expect, vkey, scan;
    WCHAR oem, wchr;
    char oem_char;

    BOOL us_kbd = (GetKeyboardLayout(0) == (HKL)(ULONG_PTR)0x04090409);
    if (!us_kbd)
    {
        skip("skipping test with inconsistent results on non-us keyboard\n");
        return;
    }

    for (oem = 0; oem < 0x200; oem++)
    {
        ret = OemKeyScan( oem );

        oem_char = LOBYTE( oem );
        /* OemKeyScan returns -1 for any character that cannot be mapped,
         * whereas OemToCharBuff changes unmappable characters to question
         * marks. The ASCII characters 0-127, including the real question mark
         * character, are all mappable and are the same in all OEM codepages. */
        if (!OemToCharBuffW( &oem_char, &wchr, 1 ) || (wchr == '?' && oem_char < 0))
            expect = -1;
        else
        {
            vkey = VkKeyScanW( wchr );
            scan = MapVirtualKeyW( LOBYTE( vkey ), MAPVK_VK_TO_VSC );
            if (!scan)
                expect = -1;
            else
            {
                vkey &= 0xff00;
                vkey <<= 8;
                expect = vkey | scan;
            }
        }
        ok( ret == expect, "%04x: got %08lx expected %08lx\n", oem, ret, expect );
    }
}

static INPUT_MESSAGE_SOURCE expect_src;

static LRESULT WINAPI msg_source_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    INPUT_MESSAGE_SOURCE source;
    MSG msg;

    ok( pGetCurrentInputMessageSource( &source ), "GetCurrentInputMessageSource failed\n" );
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        ok( source.deviceType == expect_src.deviceType || /* also accept system-generated WM_MOUSEMOVE */
            (message == WM_MOUSEMOVE && source.deviceType == IMDT_UNAVAILABLE),
            "%x: wrong deviceType %x/%x\n", message, source.deviceType, expect_src.deviceType );
        ok( source.originId == expect_src.originId ||
            (message == WM_MOUSEMOVE && source.originId == IMO_SYSTEM),
            "%x: wrong originId %x/%x\n", message, source.originId, expect_src.originId );
        SendMessageA( hwnd, WM_USER, 0, 0 );
        PostMessageA( hwnd, WM_USER, 0, 0 );
        if (PeekMessageW( &msg, hwnd, WM_USER, WM_USER, PM_REMOVE )) DispatchMessageW( &msg );
        ok( source.deviceType == expect_src.deviceType || /* also accept system-generated WM_MOUSEMOVE */
            (message == WM_MOUSEMOVE && source.deviceType == IMDT_UNAVAILABLE),
            "%x: wrong deviceType %x/%x\n", message, source.deviceType, expect_src.deviceType );
        ok( source.originId == expect_src.originId ||
            (message == WM_MOUSEMOVE && source.originId == IMO_SYSTEM),
            "%x: wrong originId %x/%x\n", message, source.originId, expect_src.originId );
        break;
    default:
        ok( source.deviceType == IMDT_UNAVAILABLE, "%x: wrong deviceType %x\n",
            message, source.deviceType );
        ok( source.originId == 0, "%x: wrong originId %x\n", message, source.originId );
        break;
    }

    return DefWindowProcA( hwnd, message, wp, lp );
}

static void test_input_message_source(void)
{
    WNDCLASSA cls;
    INPUT inputs[2];
    HWND hwnd;
    RECT rc;
    MSG msg;

    cls.style = 0;
    cls.lpfnWndProc = msg_source_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = 0;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "message source class";
    RegisterClassA(&cls);
    hwnd = CreateWindowA( cls.lpszClassName, "test", WS_OVERLAPPED, 0, 0, 100, 100,
                          0, 0, 0, 0 );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    UpdateWindow( hwnd );
    SetForegroundWindow( hwnd );
    SetFocus( hwnd );

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.dwExtraInfo = 0;
    inputs[0].ki.time = 0;
    inputs[0].ki.wVk = 0;
    inputs[0].ki.wScan = 0x3c0;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_UNAVAILABLE;
    SendMessageA( hwnd, WM_KEYDOWN, 0, 0 );
    SendMessageA( hwnd, WM_MOUSEMOVE, 0, 0 );

    SendInput( 2, inputs, sizeof(INPUT) );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        expect_src.deviceType = IMDT_KEYBOARD;
        expect_src.originId = IMO_INJECTED;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
    GetWindowRect( hwnd, &rc );
    simulate_click( TRUE, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 );
    simulate_click( FALSE, (rc.left + rc.right) / 2 + 1, (rc.top + rc.bottom) / 2 + 1 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        expect_src.deviceType = IMDT_MOUSE;
        expect_src.originId = IMO_INJECTED;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_UNAVAILABLE;
    SendMessageA( hwnd, WM_KEYDOWN, 0, 0 );
    SendMessageA( hwnd, WM_LBUTTONDOWN, 0, 0 );
    PostMessageA( hwnd, WM_KEYUP, 0, 0 );
    PostMessageA( hwnd, WM_LBUTTONUP, 0, 0 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_SYSTEM;
    SetCursorPos( (rc.left + rc.right) / 2 - 1, (rc.top + rc.bottom) / 2 - 1 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    DestroyWindow( hwnd );
    UnregisterClassA( cls.lpszClassName, GetModuleHandleA(0) );
}

static void test_UnregisterDeviceNotification(void)
{
    BOOL ret = UnregisterDeviceNotification(NULL);
    ok(ret == FALSE, "Unregistering NULL Device Notification returned: %d\n", ret);
}

static void test_SendInput( WORD vkey, WCHAR wch, HKL hkl )
{
    const struct user_call broken_sequence[] =
    {
        {.func = MSG_TEST_WIN, .message = {.msg = WM_KEYDOWN, .wparam = vkey, .lparam = MAKELONG(1, 0)}},
        {.func = MSG_TEST_WIN, .message = {.msg = WM_CHAR, .wparam = wch, .lparam = MAKELONG(1, 0)}},
        {.func = MSG_TEST_WIN, .message = {.msg = WM_KEYUP, .wparam = vkey, .lparam = MAKELONG(1, KF_UP | KF_REPEAT)}},
        {0}
    };

    INPUT input[16];
    UINT res, i;
    HWND hwnd;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );

    /* If we have had a spurious layout change, wch may be incorrect. */
    if (GetKeyboardLayout( 0 ) != hkl)
    {
        win_skip( "Spurious keyboard layout changed detected (expected: %p got: %p)\n",
                  hkl, GetKeyboardLayout( 0 ) );
        ok_ret( 1, DestroyWindow( hwnd ) );
        wait_messages( 100, FALSE );
        ok_seq( empty_sequence );
        return;
    }

    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 0, NULL, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 1, NULL, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 1, NULL, sizeof(*input) ) );
    ok( GetLastError() == ERROR_NOACCESS || GetLastError() == ERROR_INVALID_PARAMETER,
        "GetLastError returned %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 0, input, sizeof(*input) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 0, NULL, sizeof(*input) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    memset( input, 0, sizeof(input) );
    SetLastError( 0xdeadbeef );
    ok_ret( 1, SendInput( 1, input, sizeof(*input) ) );
    ok_ret( 0xdeadbeef, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 16, SendInput( 16, input, sizeof(*input) ) );
    ok_ret( 0xdeadbeef, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 1, input, 0 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 1, input, sizeof(*input) + 1 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 1, input, sizeof(*input) - 1 ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    for (i = 0; i < ARRAY_SIZE(input); ++i) input[i].type = INPUT_KEYBOARD;
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 16, input, offsetof( INPUT, ki ) + sizeof(KEYBDINPUT) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );
    SetLastError( 0xdeadbeef );
    ok_ret( 16, SendInput( 16, input, sizeof(*input) ) );
    ok_ret( 0xdeadbeef, GetLastError() );

    for (i = 0; i < ARRAY_SIZE(input); ++i) input[i].type = INPUT_HARDWARE;
    SetLastError( 0xdeadbeef );
    ok_ret( 0, SendInput( 16, input, offsetof( INPUT, hi ) + sizeof(HARDWAREINPUT) ) );
    ok_ret( ERROR_INVALID_PARAMETER, GetLastError() );

    wait_messages( 100, FALSE );
    ok_seq( empty_sequence );
    p_accept_message = is_keyboard_message;

    input[0].hi.uMsg = WM_KEYDOWN;
    input[0].hi.wParamL = 0;
    input[0].hi.wParamH = 'A';
    input[1].hi.uMsg = WM_KEYUP;
    input[1].hi.wParamL = 0;
    input[1].hi.wParamH = 'A' | 0xc000;
    SetLastError( 0xdeadbeef );
    res = SendInput( 16, input, sizeof(*input) );
    ok( (res == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) ||
        broken(res == 16 && GetLastError() == 0xdeadbeef) /* 32bit */,
        "SendInput returned %u, error %#lx\n", res, GetLastError() );
    wait_messages( 100, TRUE );
    ok_seq( empty_sequence );

    memset( input, 0, sizeof(input) );
    input[0].type = INPUT_HARDWARE;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = vkey;
    input[1].ki.dwFlags = 0;
    input[2].type = INPUT_KEYBOARD;
    input[2].ki.wVk = vkey;
    input[2].ki.dwFlags = KEYEVENTF_KEYUP;
    SetLastError( 0xdeadbeef );
    res = SendInput( 16, input, sizeof(*input) );
    ok( (res == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) ||
        broken(res == 16 && GetLastError() == 0xdeadbeef),
        "SendInput returned %u, error %#lx\n", res, GetLastError() );
    wait_messages( 100, TRUE );
    if (broken(res == 16)) ok_seq( broken_sequence );
    else ok_seq( empty_sequence );

    for (i = 0; i < ARRAY_SIZE(input); ++i) input[i].type = INPUT_HARDWARE + 1;
    SetLastError( 0xdeadbeef );
    ok_ret( 16, SendInput( 16, input, sizeof(*input) ) );
    ok_ret( 0xdeadbeef, GetLastError() );
    wait_messages( 100, TRUE );
    ok_seq( empty_sequence );

    ok_ret( 1, DestroyWindow( hwnd ) );
    wait_messages( 100, FALSE );
    ok_seq( empty_sequence );
    p_accept_message = NULL;
}

#define check_pointer_info( a, b ) check_pointer_info_( __LINE__, a, b )
static void check_pointer_info_( int line, const POINTER_INFO *actual, const POINTER_INFO *expected )
{
    check_member( *actual, *expected, "%#lx", pointerType );
    check_member( *actual, *expected, "%#x", pointerId );
    check_member( *actual, *expected, "%#x", frameId );
    check_member( *actual, *expected, "%#x", pointerFlags );
    check_member( *actual, *expected, "%p", sourceDevice );
    check_member( *actual, *expected, "%p", hwndTarget );
    check_member( *actual, *expected, "%+ld", ptPixelLocation.x );
    check_member( *actual, *expected, "%+ld", ptPixelLocation.y );
    check_member( *actual, *expected, "%+ld", ptHimetricLocation.x );
    check_member( *actual, *expected, "%+ld", ptHimetricLocation.y );
    check_member( *actual, *expected, "%+ld", ptPixelLocationRaw.x );
    check_member( *actual, *expected, "%+ld", ptPixelLocationRaw.y );
    check_member( *actual, *expected, "%+ld", ptHimetricLocationRaw.x );
    check_member( *actual, *expected, "%+ld", ptHimetricLocationRaw.y );
    check_member( *actual, *expected, "%lu", dwTime );
    check_member( *actual, *expected, "%u", historyCount );
    check_member( *actual, *expected, "%#x", InputData );
    check_member( *actual, *expected, "%#lx", dwKeyStates );
    check_member( *actual, *expected, "%I64u", PerformanceCount );
    check_member( *actual, *expected, "%#x", ButtonChangeType );
}

static DWORD CALLBACK test_GetPointerInfo_thread( void *arg )
{
    POINTER_INFO pointer_info;
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowW( L"test", L"test name", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200,
                          200, 0, 0, NULL, 0 );

    memset( &pointer_info, 0xcd, sizeof(pointer_info) );
    ret = pGetPointerInfo( 1, &pointer_info );
    ok( !ret, "GetPointerInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    DestroyWindow( hwnd );

    return 0;
}

static void test_GetPointerInfo( BOOL mouse_in_pointer_enabled )
{
    POINTER_INFO pointer_info[4], expect_pointer;
    void *invalid_ptr = (void *)0xdeadbeef;
    UINT32 entry_count, pointer_count;
    POINTER_INPUT_TYPE type;
    WNDCLASSW cls =
    {
        .lpfnWndProc   = DefWindowProcW,
        .hInstance     = GetModuleHandleW( NULL ),
        .hbrBackground = GetStockObject( WHITE_BRUSH ),
        .lpszClassName = L"test",
    };
    HANDLE thread;
    ATOM class;
    DWORD res;
    HWND hwnd;
    BOOL ret;

    if (!pGetPointerType)
    {
        todo_wine
        win_skip( "GetPointerType not found, skipping tests\n" );
        return;
    }

    SetLastError( 0xdeadbeef );
    ret = pGetPointerType( 1, NULL );
    ok( !ret, "GetPointerType succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pGetPointerType( 0xdead, &type );
    todo_wine
    ok( !ret, "GetPointerType succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    ret = pGetPointerType( 1, &type );
    ok( ret, "GetPointerType failed, error %lu\n", GetLastError() );
    ok( type == PT_MOUSE, " type %ld\n", type );

    if (!pGetPointerInfo)
    {
        todo_wine
        win_skip( "GetPointerInfo not found, skipping tests\n" );
        return;
    }

    class = RegisterClassW( &cls );
    ok( class, "RegisterClassW failed: %lu\n", GetLastError() );

    ret = pGetPointerInfo( 1, invalid_ptr );
    ok( !ret, "GetPointerInfo succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS || broken(GetLastError() == ERROR_INVALID_PARAMETER) /* w10 32bit */,
        "got error %lu\n", GetLastError() );

    memset( pointer_info, 0xcd, sizeof(pointer_info) );
    ret = pGetPointerInfo( 1, pointer_info );
    ok( !ret, "GetPointerInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    SetCursorPos( 500, 500 );  /* avoid generating mouse message on window creation */

    hwnd = CreateWindowW( L"test", L"test name", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200,
                          200, 0, 0, NULL, 0 );
    empty_message_queue();

    memset( pointer_info, 0xcd, sizeof(pointer_info) );
    ret = pGetPointerInfo( 1, pointer_info );
    ok( !ret, "GetPointerInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    SetCursorPos( 200, 200 );
    empty_message_queue();
    mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    empty_message_queue();
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );
    empty_message_queue();
    mouse_event( MOUSEEVENTF_MOVE, 10, 10, 0, 0 );
    empty_message_queue();

    memset( pointer_info, 0xcd, sizeof(pointer_info) );
    ret = pGetPointerInfo( 0xdead, pointer_info );
    ok( !ret, "GetPointerInfo succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );

    memset( pointer_info, 0xcd, sizeof(pointer_info) );
    ret = pGetPointerInfo( 1, pointer_info );
    todo_wine_if(mouse_in_pointer_enabled)
    ok( ret == mouse_in_pointer_enabled, "GetPointerInfo failed, error %lu\n", GetLastError() );
    if (!mouse_in_pointer_enabled)
    {
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
        return;
    }

    todo_wine
    ok( pointer_info[0].pointerType == PT_MOUSE, "got pointerType %lu\n", pointer_info[0].pointerType );
    todo_wine
    ok( pointer_info[0].pointerId == 1, "got pointerId %u\n", pointer_info[0].pointerId );
    ok( !!pointer_info[0].frameId, "got frameId %u\n", pointer_info[0].frameId );
    todo_wine
    ok( pointer_info[0].pointerFlags == (0x20000 | POINTER_MESSAGE_FLAG_INRANGE | POINTER_MESSAGE_FLAG_PRIMARY),
        "got pointerFlags %#x\n", pointer_info[0].pointerFlags );
    todo_wine
    ok( pointer_info[0].sourceDevice == INVALID_HANDLE_VALUE || broken(!!pointer_info[0].sourceDevice) /* < w10 & 32bit */,
        "got sourceDevice %p\n", pointer_info[0].sourceDevice );
    todo_wine
    ok( pointer_info[0].hwndTarget == hwnd, "got hwndTarget %p\n", pointer_info[0].hwndTarget );
    ok( !!pointer_info[0].ptPixelLocation.x, "got ptPixelLocation %s\n", wine_dbgstr_point( &pointer_info[0].ptPixelLocation ) );
    ok( !!pointer_info[0].ptPixelLocation.y, "got ptPixelLocation %s\n", wine_dbgstr_point( &pointer_info[0].ptPixelLocation ) );
    ok( !!pointer_info[0].ptHimetricLocation.x, "got ptHimetricLocation %s\n", wine_dbgstr_point( &pointer_info[0].ptHimetricLocation ) );
    ok( !!pointer_info[0].ptHimetricLocation.y, "got ptHimetricLocation %s\n", wine_dbgstr_point( &pointer_info[0].ptHimetricLocation ) );
    ok( !!pointer_info[0].ptPixelLocationRaw.x, "got ptPixelLocationRaw %s\n", wine_dbgstr_point( &pointer_info[0].ptPixelLocationRaw ) );
    ok( !!pointer_info[0].ptPixelLocationRaw.y, "got ptPixelLocationRaw %s\n", wine_dbgstr_point( &pointer_info[0].ptPixelLocationRaw ) );
    ok( !!pointer_info[0].ptHimetricLocationRaw.x, "got ptHimetricLocationRaw %s\n", wine_dbgstr_point( &pointer_info[0].ptHimetricLocationRaw ) );
    ok( !!pointer_info[0].ptHimetricLocationRaw.y, "got ptHimetricLocationRaw %s\n", wine_dbgstr_point( &pointer_info[0].ptHimetricLocationRaw ) );
    ok( !!pointer_info[0].dwTime, "got dwTime %lu\n", pointer_info[0].dwTime );
    todo_wine
    ok( pointer_info[0].historyCount == 1, "got historyCount %u\n", pointer_info[0].historyCount );
    todo_wine
    ok( pointer_info[0].InputData == 0, "got InputData %u\n", pointer_info[0].InputData );
    todo_wine
    ok( pointer_info[0].dwKeyStates == 0, "got dwKeyStates %lu\n", pointer_info[0].dwKeyStates );
    ok( !!pointer_info[0].PerformanceCount, "got PerformanceCount %I64u\n", pointer_info[0].PerformanceCount );
    todo_wine
    ok( pointer_info[0].ButtonChangeType == 0, "got ButtonChangeType %u\n", pointer_info[0].ButtonChangeType );

    thread = CreateThread( NULL, 0, test_GetPointerInfo_thread, NULL, 0, NULL );
    res = WaitForSingleObject( thread, 5000 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    expect_pointer = pointer_info[0];

    memset( pointer_info, 0xa5, sizeof(pointer_info) );
    entry_count = pointer_count = 2;
    if (!pGetPointerFrameInfo) ret = FALSE;
    else ret = pGetPointerFrameInfo( 1, &pointer_count, pointer_info );
    todo_wine_if(!pGetPointerFrameInfo)
    ok( ret, "GetPointerFrameInfo failed, error %lu\n", GetLastError() );
    todo_wine_if(!pGetPointerFrameInfo)
    ok( pointer_count == 1, "got pointer_count %u\n", pointer_count );
    todo_wine_if(!pGetPointerFrameInfo)
    check_pointer_info( &pointer_info[0], &expect_pointer );
    memset( pointer_info, 0xa5, sizeof(pointer_info) );
    entry_count = pointer_count = 2;
    if (!pGetPointerInfoHistory) ret = FALSE;
    else ret = pGetPointerInfoHistory( 1, &entry_count, pointer_info );
    todo_wine_if(!pGetPointerInfoHistory)
    ok( ret, "GetPointerInfoHistory failed, error %lu\n", GetLastError() );
    todo_wine_if(!pGetPointerInfoHistory)
    ok( entry_count == 1, "got entry_count %u\n", entry_count );
    todo_wine_if(!pGetPointerInfoHistory)
    check_pointer_info( &pointer_info[0], &expect_pointer );
    memset( pointer_info, 0xa5, sizeof(pointer_info) );
    entry_count = pointer_count = 2;
    if (!pGetPointerFrameInfoHistory) ret = FALSE;
    else ret = pGetPointerFrameInfoHistory( 1, &entry_count, &pointer_count, pointer_info );
    todo_wine_if(!pGetPointerFrameInfoHistory)
    ok( ret, "GetPointerFrameInfoHistory failed, error %lu\n", GetLastError() );
    todo_wine_if(!pGetPointerFrameInfoHistory)
    ok( entry_count == 1, "got pointer_count %u\n", pointer_count );
    todo_wine_if(!pGetPointerFrameInfoHistory)
    ok( pointer_count == 1, "got pointer_count %u\n", pointer_count );
    todo_wine_if(!pGetPointerFrameInfoHistory)
    check_pointer_info( &pointer_info[0], &expect_pointer );

    DestroyWindow( hwnd );

    ret = UnregisterClassW( L"test", GetModuleHandleW( NULL ) );
    ok( ret, "UnregisterClassW failed: %lu\n", GetLastError() );
}

static void test_EnableMouseInPointer( const char *arg )
{
    DWORD enable = strtoul( arg, 0, 10 );
    BOOL ret;

    winetest_push_context( "enable %lu", enable );

    ret = pEnableMouseInPointer( enable );
    todo_wine
    ok( ret, "EnableMouseInPointer failed, error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pEnableMouseInPointer( !enable );
    ok( !ret, "EnableMouseInPointer succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_ACCESS_DENIED, "got error %lu\n", GetLastError() );
    ret = pIsMouseInPointerEnabled();
    todo_wine_if(enable)
    ok( ret == enable, "IsMouseInPointerEnabled returned %u, error %lu\n", ret, GetLastError() );

    ret = pEnableMouseInPointer( enable );
    todo_wine
    ok( ret, "EnableMouseInPointer failed, error %lu\n", GetLastError() );
    ret = pIsMouseInPointerEnabled();
    todo_wine_if(enable)
    ok( ret == enable, "IsMouseInPointerEnabled returned %u, error %lu\n", ret, GetLastError() );

    test_GetPointerInfo( enable );

    winetest_pop_context();
}

static BOOL CALLBACK get_virtual_screen_proc( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    RECT *virtual_rect = (RECT *)lp;
    UnionRect( virtual_rect, virtual_rect, rect );
    return TRUE;
}

RECT get_virtual_screen_rect(void)
{
    RECT rect = {0};
    EnumDisplayMonitors( 0, NULL, get_virtual_screen_proc, (LPARAM)&rect );
    return rect;
}

static void test_ClipCursor_dirty( const char *arg )
{
    RECT rect, expect_rect = {1, 2, 3, 4};

    /* check leaked clip rect from another desktop or process */
    ok_ret( 1, GetClipCursor( &rect ) );
    todo_wine_if( !strcmp( arg, "desktop" ) )
    ok_rect( expect_rect, rect );

    /* intentionally leaking clipping rect */
}

static DWORD CALLBACK test_ClipCursor_thread( void *arg )
{
    RECT rect, clip_rect, virtual_rect = get_virtual_screen_rect();
    HWND hwnd;

    clip_rect.left = clip_rect.right = (virtual_rect.left + virtual_rect.right) / 2;
    clip_rect.top = clip_rect.bottom = (virtual_rect.top + virtual_rect.bottom) / 2;

    /* creating a window doesn't reset clipping rect */
    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                          NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* setting a window foreground does, even from the same process */
    ok_ret( 1, SetForegroundWindow( hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( virtual_rect, rect );

    /* destroying the window doesn't reset the clipping rect */
    InflateRect( &clip_rect, +1, +1 );
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* intentionally leaking clipping rect */
    return 0;
}

static void test_ClipCursor_process(void)
{
    RECT rect, clip_rect, virtual_rect = get_virtual_screen_rect();
    HWND hwnd, tmp_hwnd;
    HANDLE thread;

    clip_rect.left = clip_rect.right = (virtual_rect.left + virtual_rect.right) / 2;
    clip_rect.top = clip_rect.bottom = (virtual_rect.top + virtual_rect.bottom) / 2;

    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* creating an invisible window doesn't reset clip cursor */
    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                          NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    ok_ret( 1, DestroyWindow( hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* setting a window foreground, even invisible, resets it */
    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                          NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    ok_ret( 1, SetForegroundWindow( hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( virtual_rect, rect );

    ok_ret( 1, ClipCursor( &clip_rect ) );

    /* creating and setting another window foreground doesn't reset it */
    tmp_hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                              NULL, NULL, NULL, NULL );
    ok( !!tmp_hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    ok_ret( 1, SetForegroundWindow( tmp_hwnd ) );
    ok_ret( 1, DestroyWindow( tmp_hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* but changing foreground to another thread in the same process reset it */
    thread = CreateThread( NULL, 0, test_ClipCursor_thread, NULL, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );
    msg_wait_for_events( 1, &thread, 5000 );

    /* thread exit and foreground window destruction doesn't reset the clipping rect */
    InflateRect( &clip_rect, +1, +1 );
    ok_ret( 1, DestroyWindow( hwnd ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* intentionally leaking clipping rect */
}

static void test_ClipCursor_desktop( char **argv )
{
    RECT rect, clip_rect, virtual_rect = get_virtual_screen_rect();

    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( virtual_rect, rect );

    /* ClipCursor clips rectangle to the virtual screen rect */
    clip_rect = virtual_rect;
    InflateRect( &clip_rect, +1, +1 );
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( virtual_rect, rect );

    clip_rect = virtual_rect;
    InflateRect( &clip_rect, -1, -1 );
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* ClipCursor(NULL) resets to the virtual screen rect */
    ok_ret( 1, ClipCursor( NULL ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( virtual_rect, rect );

    clip_rect.left = clip_rect.right = (virtual_rect.left + virtual_rect.right) / 2;
    clip_rect.top = clip_rect.bottom = (virtual_rect.top + virtual_rect.bottom) / 2;
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* ClipCursor rejects invalid rectangles */
    clip_rect.right -= 1;
    clip_rect.bottom -= 1;
    SetLastError( 0xdeadbeef );
    ok_ret( 0, ClipCursor( &clip_rect ) );
    todo_wine
    ok_ret( ERROR_ACCESS_DENIED, GetLastError() );

    /* which doesn't reset the previous clip rect */
    clip_rect.right += 1;
    clip_rect.bottom += 1;
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* running a process causes it to leak until foreground actually changes */
    run_in_process( argv, "test_ClipCursor_process" );

    /* as foreground window is now transient, cursor clipping isn't reset */
    InflateRect( &clip_rect, +1, +1 );
    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* intentionally leaking clipping rect */
}

static void test_ClipCursor( char **argv )
{
    RECT rect, clip_rect = {1, 2, 3, 4}, virtual_rect = get_virtual_screen_rect();

    ok_ret( 1, ClipCursor( &clip_rect ) );

    /* running a new process doesn't reset clipping rectangle */
    run_in_process( argv, "test_ClipCursor_dirty process" );

    /* running in a separate desktop, without switching desktop as well */
    run_in_desktop( argv, "test_ClipCursor_dirty desktop", 0 );

    ok_ret( 1, GetClipCursor( &rect ) );
    ok_rect( clip_rect, rect );

    /* running in a desktop and switching input resets the clipping rect */
    run_in_desktop( argv, "test_ClipCursor_desktop", 1 );

    ok_ret( 1, GetClipCursor( &rect ) );
    todo_wine
    ok_rect( virtual_rect, rect );
    if (!EqualRect( &rect, &virtual_rect )) ok_ret( 1, ClipCursor( NULL ) );
}

static void test_SetCursorPos(void)
{
    RECT clip_rect = {50, 50, 51, 51};
    POINT pos, expect_pos = {50, 50};

    ok_ret( 0, GetCursorPos( NULL ) );
    todo_wine ok_ret( ERROR_NOACCESS, GetLastError() );

    /* immediate cursor position updates */
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );

    /* without MOUSEEVENTF_MOVE cursor doesn't move */
    mouse_event( MOUSEEVENTF_LEFTUP, 123, 456, 0, 0 );
    mouse_event( MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE, 123, 456, 0, 0 );
    mouse_event( MOUSEEVENTF_RIGHTUP, 456, 123, 0, 0 );
    mouse_event( MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_ABSOLUTE, 456, 123, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );

    /* need to move by at least 3 pixels to update, but not consistent */
    mouse_event( MOUSEEVENTF_MOVE, -1, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
#ifdef __REACTOS__
    ok( abs( expect_pos.x - pos.x ) <= 2, "got pos %ld\n", pos.x );
    ok( abs( expect_pos.y - pos.y ) <= 2, "got pos %ld\n", pos.y );
#else
    todo_wine ok_point( expect_pos, pos );
#endif
    mouse_event( MOUSEEVENTF_MOVE, +1, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
#ifdef __REACTOS__
    ok( abs( expect_pos.x - pos.x ) <= 2, "got pos %ld\n", pos.x );
    ok( abs( expect_pos.y - pos.y ) <= 2, "got pos %ld\n", pos.y );
#else
    ok_point( expect_pos, pos );
#endif

    /* spuriously moves by 1 or 2 pixels on Windows */
    expect_pos.x -= 2;
    mouse_event( MOUSEEVENTF_MOVE, -4, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
#ifdef __REACTOS__
    todo_wine ok( abs( expect_pos.x - pos.x ) <= 2, "got pos %ld\n", pos.x );
#else
    todo_wine ok( abs( expect_pos.x - pos.x ) <= 1, "got pos %ld\n", pos.x );
#endif
    expect_pos.x += 2;
    mouse_event( MOUSEEVENTF_MOVE, +4, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
#ifdef __REACTOS__
    ok( abs( expect_pos.x - pos.x ) <= 2, "got pos %ld\n", pos.x );
#else
    ok( abs( expect_pos.x - pos.x ) <= 1, "got pos %ld\n", pos.x );
#endif

    /* test ClipCursor holding the cursor in place */
    expect_pos.x = expect_pos.y = 50;
    ok_ret( 1, SetCursorPos( 49, 51 ) );
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 49, 49 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 48, 48 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 51, 51 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 52, 52 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 49, 51 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 51, 49 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );

    mouse_event( MOUSEEVENTF_MOVE, -1, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    mouse_event( MOUSEEVENTF_MOVE, 0, +1, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    mouse_event( MOUSEEVENTF_MOVE, +1, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    mouse_event( MOUSEEVENTF_MOVE, 0, -1, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );

    /* weird behavior when ClipCursor rect is empty */
    clip_rect.right = clip_rect.bottom = 50;
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    expect_pos.x = expect_pos.y = 49;
    ok_ret( 1, ClipCursor( &clip_rect ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = expect_pos.y = 50;
    ok_ret( 1, SetCursorPos( 49, 49 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    expect_pos.x = expect_pos.y = 49;
    ok_ret( 1, SetCursorPos( 50, 50 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = expect_pos.y = 50;
    ok_ret( 1, SetCursorPos( 48, 48 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );
    expect_pos.x = expect_pos.y = 49;
    ok_ret( 1, SetCursorPos( 51, 51 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    ok_ret( 1, SetCursorPos( 52, 52 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = 50;
    expect_pos.y = 49;
    ok_ret( 1, SetCursorPos( 49, 51 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = 49;
    expect_pos.y = 50;
    ok_ret( 1, SetCursorPos( 51, 49 ) );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );

    expect_pos.x = 50;
    expect_pos.y = 49;
    mouse_event( MOUSEEVENTF_MOVE, -10, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = 49;
    expect_pos.y = 49;
    mouse_event( MOUSEEVENTF_MOVE, 0, +10, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = 49;
    expect_pos.y = 50;
    mouse_event( MOUSEEVENTF_MOVE, +10, 0, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    todo_wine ok_point( expect_pos, pos );
    expect_pos.x = 50;
    expect_pos.y = 50;
    mouse_event( MOUSEEVENTF_MOVE, 0, -10, 0, 0 );
    ok_ret( 1, GetCursorPos( &pos ) );
    ok_point( expect_pos, pos );

    ok_ret( 1, ClipCursor( NULL ) );
}

static HANDLE ll_keyboard_event;

static LRESULT CALLBACK ll_keyboard_event_wait(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HC_ACTION)
    {
        ok_ret( WAIT_TIMEOUT, WaitForSingleObject( ll_keyboard_event, 100 ) );
        return -123;
    }

    return CallNextHookEx( 0, code, wparam, lparam );
}

static void test_keyboard_ll_hook_blocking(void)
{
    INPUT input = {.type = INPUT_KEYBOARD, .ki = {.wVk = VK_RETURN}};
    HHOOK hook;
    HWND hwnd;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL );
    ok_ne( NULL, hwnd, HWND, "%p" );
    wait_messages( 100, FALSE );
    hook = SetWindowsHookExW( WH_KEYBOARD_LL, ll_keyboard_event_wait, GetModuleHandleW( NULL ), 0 );
    ok_ne( NULL, hook, HHOOK, "%p" );
    ll_keyboard_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_ne( NULL, ll_keyboard_event, HANDLE, "%p" );

    ok_ret( 1, SendInput( 1, &input, sizeof(input) ) );
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    ok_ret( 1, SendInput( 1, &input, sizeof(input) ) );
    ok_ret( 1, SetEvent( ll_keyboard_event ) );

    ok_ret( 1, CloseHandle( ll_keyboard_event ) );
    ok_ret( 1, UnhookWindowsHookEx( hook ) );
    ok_ret( 1, DestroyWindow( hwnd ) );
}

static void test_LoadKeyboardLayoutEx( HKL orig_hkl )
{
    static const WCHAR test_layout_name[] = L"00000429";
    static const HKL test_hkl = (HKL)0x04290429;

    HKL *new_layouts, *layouts, old_hkl, hkl;
    UINT i, j, len, new_len;
    WCHAR layout_name[64];

    old_hkl = GetKeyboardLayout( 0 );
    ok_ne( 0, old_hkl, HKL, "%p" );

    /* If we are dealing with a testbot setup that is prone to spurious
     * layout changes, layout activations in this test are likely to
     * not have the expected effect, invalidating the test assumptions. */
    if (orig_hkl != old_hkl)
    {
        win_skip( "Spurious keyboard layout changed detected (expected: %p got: %p)\n",
                  orig_hkl, old_hkl );
        return;
    }

    hkl = pLoadKeyboardLayoutEx( NULL, test_layout_name, 0 );
    ok_eq( 0, hkl, HKL, "%p" );

    layouts = get_keyboard_layouts( &len );
    for (i = 0; i < len; i++) if (layouts[i] == test_hkl) break;
    if (i != len)
    {
        skip( "Test HKL is already loaded, skipping tests\n" );
        free( layouts );
        return;
    }

    /* LoadKeyboardLayoutEx replaces loaded layouts, but will lose mixed layout / locale */
    for (i = 0, j = len; i < len; i++)
    {
        if (HIWORD(layouts[i]) != LOWORD(layouts[i])) continue;
        if (j == len) j = i;
        else break;
    }
    if (i == len) i = j;
    if (i == len)
    {
        skip( "Failed to find appropriate layouts, skipping tests\n" );
        free( layouts );
        return;
    }

    trace( "using layouts %p / %p\n", layouts[i], layouts[j] );

    ActivateKeyboardLayout( layouts[i], 0 );
    ok_eq( layouts[i], GetKeyboardLayout( 0 ), HKL, "%p" );
    ok_ret( 1, GetKeyboardLayoutNameW( layout_name ) );

    /* LoadKeyboardLayoutEx replaces a currently loaded layout */
    hkl = pLoadKeyboardLayoutEx( layouts[i], test_layout_name, 0 );
    todo_wine
    ok_eq( test_hkl, hkl, HKL, "%p" );
    new_layouts = get_keyboard_layouts( &new_len );
    ok_eq( len, new_len, UINT, "%u" );
    todo_wine
    ok_eq( test_hkl, new_layouts[i], HKL, "%p" );
    new_layouts[i] = layouts[i];
    ok( !memcmp( new_layouts, layouts, len * sizeof(*layouts) ), "keyboard layouts changed\n" );
    free( new_layouts );

    hkl = pLoadKeyboardLayoutEx( test_hkl, layout_name, 0 );
    ok_eq( layouts[i], hkl, HKL, "%p" );
    new_layouts = get_keyboard_layouts( &new_len );
    ok_eq( len, new_len, UINT, "%u" );
    ok( !memcmp( new_layouts, layouts, len * sizeof(*layouts) ), "keyboard layouts changed\n" );
    free( new_layouts );

    if (j == i) skip( "Only one layout found, skipping tests\n" );
    else
    {
        /* it also works if a different layout is active */
        ActivateKeyboardLayout( layouts[j], 0 );
        ok_eq( layouts[j], GetKeyboardLayout( 0 ), HKL, "%p" );

        hkl = pLoadKeyboardLayoutEx( layouts[i], test_layout_name, 0 );
        todo_wine
        ok_eq( test_hkl, hkl, HKL, "%p" );
        new_layouts = get_keyboard_layouts( &new_len );
        ok_eq( len, new_len, UINT, "%u" );
        todo_wine
        ok_eq( test_hkl, new_layouts[i], HKL, "%p" );
        new_layouts[i] = layouts[i];
        ok( !memcmp( new_layouts, layouts, len * sizeof(*layouts) ), "keyboard layouts changed\n" );
        free( new_layouts );

        hkl = pLoadKeyboardLayoutEx( test_hkl, layout_name, 0 );
        ok_eq( layouts[i], hkl, HKL, "%p" );
        new_layouts = get_keyboard_layouts( &new_len );
        ok_eq( len, new_len, UINT, "%u" );
        ok( !memcmp( new_layouts, layouts, len * sizeof(*layouts) ), "keyboard layouts changed\n" );
        free( new_layouts );
    }

    free( layouts );
    ActivateKeyboardLayout( old_hkl, 0 );
    ok_eq( old_hkl, GetKeyboardLayout( 0 ), HKL, "%p" );
}

/* run the tests in a separate desktop to avoid interaction with other
 * tests, current desktop state, or user actions. */
static void test_input_desktop( char **argv )
{
    HKL hkl = GetKeyboardLayout( 0 );
    WCHAR wch, wch_shift;
    POINT pos;
    WORD scan;

    trace( "hkl %p\n", hkl );
    ok_ret( 1, GetCursorPos( &pos ) );
    test_SetCursorPos();

    get_test_scan( 'F', &scan, &wch, &wch_shift );
    test_SendInput( 'F', wch, hkl );
    test_SendInput_keyboard_messages( 'F', scan, wch, wch_shift, '\x06', hkl );
    test_SendInput_mouse_messages();

    test_keyboard_ll_hook_blocking();

    test_RegisterRawInputDevices();
    test_GetRawInputData();
    test_GetRawInputBuffer();
    test_SendInput_raw_key_messages( 'F', wch, hkl );

    test_LoadKeyboardLayoutEx( hkl );

    ok_ret( 1, SetCursorPos( pos.x, pos.y ) );
}

static void test_keyboard_layout(void)
{
    const CHAR *layout_name;
    LANGID lang_id;
    HKL hkl;

    /* Test that the high word of the keyboard layout in CJK locale is the same as the low word,
     * even when IME is on */
    lang_id = PRIMARYLANGID(GetUserDefaultLCID());
    if (lang_id == LANG_CHINESE || lang_id == LANG_JAPANESE || lang_id == LANG_KOREAN)
    {
        hkl = GetKeyboardLayout(0);
        ok(HIWORD(hkl) == LOWORD(hkl), "Got unexpected hkl %p.\n", hkl);

        if (lang_id == LANG_CHINESE)
            layout_name = "00000804";
        else if (lang_id == LANG_JAPANESE)
            layout_name = "00000411";
        else if (lang_id == LANG_KOREAN)
            layout_name = "00000412";
        hkl = LoadKeyboardLayoutA(layout_name, 0);
        ok(HIWORD(hkl) == LOWORD(hkl), "Got unexpected hkl %p.\n", hkl);
    }
}

START_TEST(input)
{
    char **argv;
    int argc;
    POINT pos;

    init_function_pointers();
    GetCursorPos( &pos );

#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_VISTA && !is_reactos()) {
        skip("The user32:input tests causes persistent input issues on WS03!\n");
        return;
    }
#endif
    argc = winetest_get_mainargs(&argv);
    if (argc >= 3 && !strcmp( argv[2], "rawinput_test" ))
        return rawinput_test_process();
    if (argc >= 3 && !strcmp( argv[2], "test_GetMouseMovePointsEx_process" ))
        return test_GetMouseMovePointsEx_process();
    if (argc >= 4 && !strcmp( argv[2], "test_EnableMouseInPointer" ))
        return test_EnableMouseInPointer( argv[3] );
    if (argc >= 4 && !strcmp( argv[2], "test_ClipCursor_dirty" ))
        return test_ClipCursor_dirty( argv[3] );
    if (argc >= 3 && !strcmp( argv[2], "test_ClipCursor_process" ))
        return test_ClipCursor_process();
    if (argc >= 3 && !strcmp( argv[2], "test_ClipCursor_desktop" ))
        return test_ClipCursor_desktop( argv );
    if (argc >= 3 && !strcmp( argv[2], "test_input_desktop" ))
        return test_input_desktop( argv );

    run_in_desktop( argv, "test_input_desktop", 1 );
    test_keynames();
    test_key_map();
    test_ToUnicode();
    test_ToAscii();
    test_get_async_key_state();
    test_keyboard_layout();
    test_keyboard_layout_name();
    test_ActivateKeyboardLayout( argv );
    test_key_names();
    test_attach_input();
    test_GetKeyState();
    test_OemKeyScan();
    test_rawinput(argv[0]);
    test_DefRawInputProc();

    if(pGetMouseMovePointsEx)
        test_GetMouseMovePointsEx( argv );
    else
        win_skip("GetMouseMovePointsEx is not available\n");

    if(pGetRawInputDeviceList)
        test_GetRawInputDeviceList();
    else
        win_skip("GetRawInputDeviceList is not available\n");

    if (pGetCurrentInputMessageSource)
        test_input_message_source();
    else
        win_skip("GetCurrentInputMessageSource is not available\n");

    SetCursorPos( pos.x, pos.y );

    if (pGetPointerType)
        test_GetPointerInfo( FALSE );
    else
        win_skip( "GetPointerType is not available\n" );

    test_UnregisterDeviceNotification();

    if (!pEnableMouseInPointer)
        win_skip( "EnableMouseInPointer not found, skipping tests\n" );
    else
    {
        run_in_process( argv, "test_EnableMouseInPointer 0" );
        run_in_process( argv, "test_EnableMouseInPointer 1" );
    }

    test_ClipCursor( argv );
}
