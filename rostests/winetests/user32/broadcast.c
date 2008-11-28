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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define _WIN32_WINNT 0x0501

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/test.h"

typedef LONG (WINAPI *PBROADCAST)( DWORD,LPDWORD,UINT,WPARAM,LPARAM );
typedef LONG (WINAPI *PBROADCASTEX)( DWORD,LPDWORD,UINT,WPARAM,LPARAM,PBSMINFO );
static PBROADCAST pBroadcastA;
static PBROADCAST pBroadcastW;
static PBROADCASTEX pBroadcastExA;
static PBROADCASTEX pBroadcastExW;
static HANDLE hevent;

static LRESULT WINAPI main_window_procA(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NULL)
    {
        trace("main_window_procA: Sleeping for %lu ms\n", wparam);
        if (wparam)
        {
            if (WaitForSingleObject(hevent, wparam) == WAIT_TIMEOUT)
                SetEvent(hevent);
        }
        trace("main_window_procA: Returning WM_NULL with parameter %08lx\n", lparam);
        return lparam;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL init_procs(void)
{
    WNDCLASSA cls;
    HANDLE user32 = GetModuleHandle("user32");
    pBroadcastA = (PBROADCAST)GetProcAddress(user32, "BroadcastSystemMessageA");
    if (!pBroadcastA)
        pBroadcastA = (PBROADCAST)GetProcAddress(user32, "BroadcastSystemMessage");
    ok(pBroadcastA != NULL, "No BroadcastSystemMessage found\n");
    if (!pBroadcastA)
        return FALSE;

    pBroadcastW = (PBROADCAST)GetProcAddress(user32, "BroadcastSystemMessageW");
    pBroadcastExA = (PBROADCASTEX)GetProcAddress(user32, "BroadcastSystemMessageExA");
    pBroadcastExW = (PBROADCASTEX)GetProcAddress(user32, "BroadcastSystemMessageExW");

    hevent = CreateEventA(NULL, TRUE, FALSE, "Asynchronous checking event");

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = main_window_procA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MainWindowClass";

    if (!RegisterClassA(&cls))
        return 0;

    if (!CreateWindowExA(0, "MainWindowClass", "Main window", WS_CAPTION | WS_SYSMENU |
                               WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_POPUP, 100, 100, 200,
                               200, 0, 0, GetModuleHandle(0), NULL))
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
        skip("%s is not implemented\n", functionname);
        return;
    }
    ok(!ret || broken(ret), "Returned: %d\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08x\n", GetLastError());

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( 0x80000000, &recips, WM_NULL, 0, 0 );
    ok(!ret || broken(ret), "Returned: %d\n", ret);
    if (!ret) ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08x\n", GetLastError());

#if 0 /* TODO: Check the hang flags */
    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_NOTIMEOUTIFNOTHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_POSTMESSAGE|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0 );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);
#endif

    recips = BSM_APPLICATIONS;
    ResetEvent(hevent);
    ret = broadcast( BSF_POSTMESSAGE|BSF_QUERY, &recips, WM_NULL, 100, 0 );
    ok(ret==1, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);

    SetLastError( 0xdeadbeef );
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_POSTMESSAGE|BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, 0 );
    if (ret)
    {
        ok(ret==1, "Returned: %d\n", ret);
        ok(WaitForSingleObject(hevent, 0) != WAIT_OBJECT_0, "Synchronous message sent instead\n");
        PulseEvent(hevent);

        recips = BSM_APPLICATIONS;
        ret = broadcast( BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY );
        ok(ret==1, "Returned: %d\n", ret);
        ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
        PulseEvent(hevent);

        recips = BSM_APPLICATIONS;
        ret = broadcast( BSF_SENDNOTIFYMESSAGE|BSF_QUERY, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY );
        ok(!ret, "Returned: %d\n", ret);
        ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
        PulseEvent(hevent);
    }
    else  /* BSF_SENDNOTIFYMESSAGE not supported on NT4 */
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "failed with err %u\n", GetLastError() );

    recips = BSM_APPLICATIONS;
    ret = broadcast( 0, &recips, WM_NULL, 100, 0 );
    ok(ret==1, "Returned: %d\n", ret);
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08x\n", GetLastError());
    ok(!ret, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcastex( 0x80000000, &recips, WM_NULL, 0, 0, NULL );
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error: %08x\n", GetLastError());
    ok(!ret, "Returned: %d\n", ret);

#if 0 /* TODO: Check the hang flags */
    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOHANG|BSF_NOTIMEOUTIFNOTHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_QUERY|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);

    SetLastError(0xcafebabe);
    recips = BSM_APPLICATIONS;
    ret = broadcast( BSF_POSTMESSAGE|(BSF_NOTIMEOUTIFNOTHUNG|BSF_FORCEIFHUNG), &recips, WM_NULL, 30000, 0, NULL );
    ok(0, "Last error: %08x\n", GetLastError());
    ok(0, "Returned: %d\n", ret);
#endif

    recips = BSM_APPLICATIONS;
    ResetEvent(hevent);
    ret = broadcastex( BSF_POSTMESSAGE|BSF_QUERY, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_POSTMESSAGE|BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_OBJECT_0, "Synchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_SENDNOTIFYMESSAGE, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY, NULL );
    ok(ret==1, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcastex( BSF_SENDNOTIFYMESSAGE|BSF_QUERY, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY, NULL );
    ok(!ret, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);

    recips = BSM_APPLICATIONS;
    ret = broadcastex( 0, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    PulseEvent(hevent);
}

static BOOL (WINAPI *pOpenProcessToken)(HANDLE, DWORD, HANDLE*);
static BOOL (WINAPI *pAdjustTokenPrivileges)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

static void test_noprivileges(void)
{
    HANDLE advapi32 = GetModuleHandleA("advapi32");
    HANDLE token;
    DWORD recips;
    BOOL ret;

    static const DWORD BSM_ALL_RECIPS = BSM_VXDS | BSM_NETDRIVER |
                                        BSM_INSTALLABLEDRIVERS | BSM_APPLICATIONS;

    pOpenProcessToken = (void *)GetProcAddress(advapi32, "OpenProcessToken");
    pAdjustTokenPrivileges = (void *)GetProcAddress(advapi32, "AdjustTokenPrivileges");
    if (!pOpenProcessToken || !pAdjustTokenPrivileges || !pOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
    {
        skip("Can't open security token for process\n");
        return;
    }
    if (!pAdjustTokenPrivileges(token, TRUE, NULL, 0, NULL, NULL))
    {
        skip("Can't adjust security token for process\n");
        return;
    }

    trace("Trying privileged edition!\n");
    SetLastError(0xcafebabe);
    recips = BSM_ALLDESKTOPS;
    ResetEvent(hevent);
    ret = pBroadcastExW( BSF_QUERY, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d error %u\n", ret, GetLastError());
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    ok(recips == BSM_ALLDESKTOPS ||
       recips == BSM_ALL_RECIPS, /* win2k3 */
       "Received by: %08x\n", recips);
    PulseEvent(hevent);

    SetLastError(0xcafebabe);
    recips = BSM_ALLCOMPONENTS;
    ResetEvent(hevent);
    ret = pBroadcastExW( BSF_QUERY, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d error %u\n", ret, GetLastError());
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    ok(recips == BSM_ALLCOMPONENTS ||
       recips == BSM_ALL_RECIPS, /* win2k3 */
       "Received by: %08x\n", recips);
    PulseEvent(hevent);

    SetLastError(0xcafebabe);
    recips = BSM_ALLDESKTOPS|BSM_APPLICATIONS;
    ResetEvent(hevent);
    ret = pBroadcastExW( BSF_QUERY, &recips, WM_NULL, 100, 0, NULL );
    ok(ret==1, "Returned: %d error %u\n", ret, GetLastError());
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    ok(recips == (BSM_ALLDESKTOPS|BSM_APPLICATIONS) ||
       recips == BSM_APPLICATIONS, /* win2k3 */
       "Received by: %08x\n", recips);
    PulseEvent(hevent);

    SetLastError(0xcafebabe);
    recips = BSM_ALLDESKTOPS|BSM_APPLICATIONS;
    ResetEvent(hevent);
    ret = pBroadcastExW( BSF_QUERY, &recips, WM_NULL, 100, BROADCAST_QUERY_DENY, NULL );
    ok(!ret, "Returned: %d\n", ret);
    ok(WaitForSingleObject(hevent, 0) != WAIT_TIMEOUT, "Asynchronous message sent instead\n");
    ok(recips == (BSM_ALLDESKTOPS|BSM_APPLICATIONS) ||
       recips == BSM_APPLICATIONS, /* win2k3 */
       "Received by: %08x\n", recips);
    PulseEvent(hevent);
}

START_TEST(broadcast)
{
    if (!init_procs())
        return;

    trace("Running BroadcastSystemMessageA tests\n");
    test_parameters(pBroadcastA, "BroadcastSystemMessageA");
    if (pBroadcastW)
    {
        trace("Running BroadcastSystemMessageW tests\n");
        test_parameters(pBroadcastW, "BroadcastSystemMessageW");
    }
    else
        skip("No BroadcastSystemMessageW, skipping\n");
    if (pBroadcastExA)
    {
        trace("Running BroadcastSystemMessageExA tests\n");
        test_parametersEx(pBroadcastExA);
    }
    else
        skip("No BroadcastSystemMessageExA, skipping\n");
    if (pBroadcastExW)
    {
        trace("Running BroadcastSystemMessageExW tests\n");
        test_parametersEx(pBroadcastExW);
        trace("Attempting privileges checking tests\n");
        test_noprivileges();
    }
    else
        skip("No BroadcastSystemMessageExW, skipping\n");
}
