/*
 * Unit tests for BroadcastSystemMessage
 *
 * Copyright 2008 Maarten Lankhorst
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/test.h"

typedef LONG (WINAPI *PBROADCAST)( DWORD,LPDWORD,UINT,WPARAM,LPARAM );
typedef LONG (WINAPI *PBROADCASTEX)( DWORD,LPDWORD,UINT,WPARAM,LPARAM,PBSMINFO );
static HANDLE hevent;

static LRESULT WINAPI main_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NULL)
    {
        trace("main_window_procA: Sleeping for %Iu ms\n", wparam);
        if (wparam)
        {
            if (WaitForSingleObject(hevent, wparam) == WAIT_TIMEOUT)
                SetEvent(hevent);
        }
        trace("main_window_procA: Returning WM_NULL with parameter %08Ix\n", lparam);
        return lparam;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL init_procs(void)
{
    WNDCLASSA cls;

    hevent = CreateEventA(NULL, TRUE, FALSE, "Asynchronous checking event");

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = main_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MainWindowClass";

    if (!RegisterClassA(&cls))
        return FALSE;

    if (!CreateWindowExA(0, "MainWindowClass", "Main window", WS_CAPTION | WS_SYSMENU |
                               WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP, 100, 100, 200,
                               200, 0, 0, GetModuleHandleA(NULL), NULL))
        return FALSE;
    return TRUE;
}

static void test_parameters(PBROADCAST broadcast, const char *functionname)
{
    LONG ret;
    DWORD recips;

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( 0x80000000, &recips, WM_NULL, 0, 0 );
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("%s is not implemented\n", functionname);
        return;
    }
    ok(!ret || broken(ret), "Returned: %ld\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08lx\n", GetLastError());

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( 0x80000000, &recips, WM_NULL, 0, 0 );
    ok(!ret || broken(ret), "Returned: %ld\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08lx\n", GetLastError());

if (0) /* TODO: Check the hang flags */
{
    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_NOTIMEOUTIFNOTHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_POSTMESSAGE|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);
}

    SetLastError( 0xdeadbeef );
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_POSTMESSAGE|BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, 0 );
    ok(ret==1, "Returned: %ld\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_OBJECT_0, "Synchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY );
    ok(ret==1, "Returned: %ld\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);
}

/* BSF_SENDNOTIFYMESSAGE and BSF_QUERY are both synchronous within the same process
 * However you should be able to distinguish them by sending the BROADCAST_QUERY_DENY flag
 */

static void test_parametersEx(PBROADCASTEX broadcastex)
{
    LONG ret;
    DWORD recips;

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( 0x80000000, &recips, WM_NULL, 0, 0, NULL );
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08lx\n", GetLastError());
    ok(!ret, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( 0x80000000, &recips, WM_NULL, 0, 0, NULL );
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08lx\n", GetLastError());
    ok(!ret, "Returned: %ld\n", ret);

if (0) /* TODO: Check the hang flags */
{
    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_QUERY|(BSF_NOHANG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_QUERY|(BSF_NOHANG|BSF_NOTIMEOUTIFNOTHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_QUERY|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_POSTMESSAGE|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08lx\n", GetLastError());
    ok(0, "Returned: %ld\n", ret);
}

    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_POSTMESSAGE|BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %ld\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_OBJECT_0, "Synchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY, NULL );
    ok(ret==1, "Returned: %ld\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);
}

START_TEST(broadcast)
{
    if (!init_procs())
        return;

    trace("Running BroadcastSystemMessageA tests\n");
    test_parameters(BroadcastSystemMessageA, "BroadcastSystemMessageA");

    trace("Running BroadcastSystemMessageW tests\n");
    test_parameters(BroadcastSystemMessageW, "BroadcastSystemMessageW");

    trace("Running BroadcastSystemMessageExA tests\n");
    test_parametersEx(BroadcastSystemMessageExA);

    trace("Running BroadcastSystemMessageExW tests\n");
    test_parametersEx(BroadcastSystemMessageExW);
}
