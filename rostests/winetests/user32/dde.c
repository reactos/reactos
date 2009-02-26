/*
 * Unit tests for DDE functions
 *
 * Copyright (c) 2004 Dmitry Timoshkov
 * Copyright (c) 2007 James Hawkins
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "dde.h"
#include "ddeml.h"
#include "winerror.h"

#include "wine/test.h"

static const WCHAR TEST_DDE_SERVICE[] = {'T','e','s','t','D','D','E','S','e','r','v','i','c','e',0};

static char exec_cmdA[] = "ANSI dde command";
static WCHAR exec_cmdW[] = {'u','n','i','c','o','d','e',' ','d','d','e',' ','c','o','m','m','a','n','d',0};

static WNDPROC old_dde_client_wndproc;

static const DWORD default_timeout = 200;

static void flush_events(void)
{
    MSG msg;
    int diff = default_timeout;
    int min_timeout = 50;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessage( &msg );
        diff = time - GetTickCount();
        min_timeout = 10;
    }
}

static void create_dde_window(HWND *hwnd, LPCSTR name, WNDPROC wndproc)
{
    WNDCLASSA wcA;

    memset(&wcA, 0, sizeof(wcA));
    wcA.lpfnWndProc = wndproc;
    wcA.lpszClassName = name;
    wcA.hInstance = GetModuleHandleA(0);
    assert(RegisterClassA(&wcA));

    *hwnd = CreateWindowExA(0, name, NULL, WS_POPUP,
                            500, 500, CW_USEDEFAULT, CW_USEDEFAULT,
                            GetDesktopWindow(), 0, GetModuleHandleA(0), NULL);
    assert(*hwnd);
}

static void destroy_dde_window(HWND *hwnd, LPCSTR name)
{
    DestroyWindow(*hwnd);
    UnregisterClass(name, GetModuleHandleA(0));
}

static LRESULT WINAPI dde_server_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT_PTR lo, hi;
    char str[MAX_PATH], *ptr;
    HGLOBAL hglobal;
    DDEDATA *data;
    DDEPOKE *poke;
    DWORD size;

    static int msg_index = 0;
    static HWND client = 0;
    static BOOL executed = FALSE;

    if (msg < WM_DDE_FIRST || msg > WM_DDE_LAST)
        return DefWindowProcA(hwnd, msg, wparam, lparam);

    msg_index++;

    switch (msg)
    {
    case WM_DDE_INITIATE:
    {
        client = (HWND)wparam;
        ok(msg_index == 1, "Expected 1, got %d\n", msg_index);

        GlobalGetAtomNameA(LOWORD(lparam), str, MAX_PATH);
        ok(!lstrcmpA(str, "TestDDEService"), "Expected TestDDEService, got %s\n", str);

        GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);

        SendMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_REQUEST:
    {
        ok((msg_index >= 2 && msg_index <= 4) ||
           (msg_index >= 7 && msg_index <= 8),
           "Expected 2, 3, 4, 7 or 8, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);
        ok(LOWORD(lparam) == CF_TEXT, "Expected CF_TEXT, got %d\n", LOWORD(lparam));

        GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
        if (msg_index < 8)
            ok(!lstrcmpA(str, "request"), "Expected request, got %s\n", str);
        else
            ok(!lstrcmpA(str, "executed"), "Expected executed, got %s\n", str);

        if (msg_index == 8)
        {
            if (executed)
                lstrcpyA(str, "command executed\r\n");
            else
                lstrcpyA(str, "command not executed\r\n");
        }
        else
            lstrcpyA(str, "requested data\r\n");

        size = sizeof(DDEDATA) + lstrlenA(str) + 1;
        hglobal = GlobalAlloc(GMEM_MOVEABLE, size);
        ok(hglobal != NULL, "Expected non-NULL hglobal\n");

        data = GlobalLock(hglobal);
        ZeroMemory(data, size);

        /* setting fResponse to FALSE at this point destroys
         * the internal messaging state of native dde
         */
        data->fResponse = TRUE;

        if (msg_index == 2)
            data->fRelease = TRUE;
        else if (msg_index == 3)
            data->fAckReq = TRUE;

        data->cfFormat = CF_TEXT;
        lstrcpyA((LPSTR)data->Value, str);
        GlobalUnlock(hglobal);

        lparam = PackDDElParam(WM_DDE_ACK, (UINT_PTR)hglobal, HIWORD(lparam));
        PostMessageA(client, WM_DDE_DATA, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_POKE:
    {
        ok(msg_index == 5 || msg_index == 6, "Expected 5 or 6, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);

        UnpackDDElParam(WM_DDE_POKE, lparam, &lo, &hi);

        GlobalGetAtomNameA(hi, str, MAX_PATH);
        ok(!lstrcmpA(str, "poker"), "Expected poker, got %s\n", str);

        poke = GlobalLock((HGLOBAL)lo);
        ok(poke != NULL, "Expected non-NULL poke\n");
        ok(poke->fReserved == 0, "Expected 0, got %d\n", poke->fReserved);
        ok(poke->unused == 0, "Expected 0, got %d\n", poke->unused);
        ok(poke->fRelease == TRUE, "Expected TRUE, got %d\n", poke->fRelease);
        ok(poke->cfFormat == CF_TEXT, "Expected CF_TEXT, got %d\n", poke->cfFormat);

        if (msg_index == 5)
        {
            size = GlobalSize((HGLOBAL)lo);
            ok(size == 4, "got %d\n", size);
        }
        else
            ok(!lstrcmpA((LPSTR)poke->Value, "poke data\r\n"),
               "Expected 'poke data\\r\\n', got %s\n", poke->Value);

        GlobalUnlock((HGLOBAL)lo);

        lparam = PackDDElParam(WM_DDE_ACK, DDE_FACK, hi);
        PostMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_EXECUTE:
    {
        ok(msg_index == 7, "Expected 7, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);

        ptr = GlobalLock((HGLOBAL)lparam);
        ok(!lstrcmpA(ptr, "[Command(Var)]"), "Expected [Command(Var)], got %s\n", ptr);
        GlobalUnlock((HGLOBAL)lparam);

        executed = TRUE;

        lparam = ReuseDDElParam(lparam, WM_DDE_EXECUTE, WM_DDE_ACK, DDE_FACK, HIWORD(lparam));
        PostMessageA(client, WM_DDE_ACK, (WPARAM)hwnd, lparam);

        break;
    }

    case WM_DDE_TERMINATE:
    {
        ok(msg_index == 9, "Expected 9, got %d\n", msg_index);
        ok(wparam == (WPARAM)client, "Expected client hwnd, got %08lx\n", wparam);
        ok(lparam == 0, "Expected 0, got %08lx\n", lparam);

        PostMessageA(client, WM_DDE_TERMINATE, (WPARAM)hwnd, 0);

        break;
    }

    default:
        ok(FALSE, "Unhandled msg: %08x\n", msg);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void test_msg_server(HANDLE hproc, HANDLE hthread)
{
    MSG msg;
    HWND hwnd;
    DWORD res;

    create_dde_window(&hwnd, "dde_server", dde_server_wndproc);
    ResumeThread( hthread );

    while (MsgWaitForMultipleObjects( 1, &hproc, FALSE, INFINITE, QS_ALLINPUT ) != 0)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }

    destroy_dde_window(&hwnd, "dde_server");
    GetExitCodeProcess( hproc, &res );
    ok( !res, "client failed with %u error(s)\n", res );
}

static HDDEDATA CALLBACK client_ddeml_callback(UINT uType, UINT uFmt, HCONV hconv,
                                               HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    ok(FALSE, "Unhandled msg: %08x\n", uType);
    return 0;
}

static void test_ddeml_client(void)
{
    UINT ret;
    char buffer[32];
    LPSTR str;
    DWORD size, res;
    HDDEDATA hdata, op;
    HSZ server, topic, item;
    DWORD client_pid;
    HCONV conversation;

    client_pid = 0;
    ret = DdeInitializeA(&client_pid, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    /* FIXME: make these atoms global and check them in the server */

    server = DdeCreateStringHandleA(client_pid, "TestDDEService", CP_WINANSI);
    topic = DdeCreateStringHandleA(client_pid, "TestDDETopic", CP_WINANSI);

    DdeGetLastError(client_pid);
    conversation = DdeConnect(client_pid, server, topic, NULL);
    ok(conversation != NULL, "Expected non-NULL conversation\n");
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    DdeFreeStringHandle(client_pid, server);

    item = DdeCreateStringHandleA(client_pid, "request", CP_WINANSI);

    /* XTYP_REQUEST, fRelease = TRUE */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %08x\n", res);
    if (hdata == NULL)
        ok(FALSE, "hdata is NULL\n");
    else
    {
        str = (LPSTR)DdeAccessData(hdata, &size);
        ok(!lstrcmpA(str, "requested data\r\n"), "Expected 'requested data\\r\\n', got %s\n", str);
        ok(size == 19, "Expected 19, got %d\n", size);

        ret = DdeUnaccessData(hdata);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }

    /* XTYP_REQUEST, fAckReq = TRUE */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
todo_wine
    ok(ret == DMLERR_MEMORY_ERROR, "Expected DMLERR_MEMORY_ERROR, got %d\n", ret);
    if (hdata == NULL)
        ok(FALSE, "hdata is NULL\n");
    else
    {
        str = (LPSTR)DdeAccessData(hdata, &size);
        ok(!lstrcmpA(str, "requested data\r\n"), "Expected 'requested data\\r\\n', got %s\n", str);
        ok(size == 19, "Expected 19, got %d\n", size);

        ret = DdeUnaccessData(hdata);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }

    /* XTYP_REQUEST, all params normal */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
    if (hdata == NULL)
        ok(FALSE, "hdata is NULL\n");
    else
    {
        str = (LPSTR)DdeAccessData(hdata, &size);
        ok(!lstrcmpA(str, "requested data\r\n"), "Expected 'requested data\\r\\n', got %s\n", str);
        ok(size == 19, "Expected 19, got %d\n", size);

        ret = DdeUnaccessData(hdata);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }

    /* XTYP_REQUEST, no item */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, 0, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(hdata == NULL, "Expected NULL hdata, got %p\n", hdata);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %08x\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    DdeFreeStringHandle(client_pid, item);

    item = DdeCreateStringHandleA(client_pid, "poker", CP_WINANSI);

    lstrcpyA(buffer, "poke data\r\n");
    hdata = DdeCreateDataHandle(client_pid, (LPBYTE)buffer, lstrlenA(buffer) + 1,
                                0, item, CF_TEXT, 0);
    ok(hdata != NULL, "Expected non-NULL hdata\n");

    /* XTYP_POKE, no item */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, 0, CF_TEXT, XTYP_POKE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    /* XTYP_POKE, no data */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, 0, CF_TEXT, XTYP_POKE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    /* XTYP_POKE, wrong size */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, 0, conversation, item, CF_TEXT, XTYP_POKE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
    ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    /* XTYP_POKE, correct params */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, item, CF_TEXT, XTYP_POKE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
    ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);

    DdeFreeDataHandle(hdata);

    lstrcpyA(buffer, "[Command(Var)]");
    hdata = DdeCreateDataHandle(client_pid, (LPBYTE)buffer, lstrlenA(buffer) + 1,
                                0, NULL, CF_TEXT, 0);
    ok(hdata != NULL, "Expected non-NULL hdata\n");

    /* XTYP_EXECUTE, correct params */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction((LPBYTE)hdata, -1, conversation, NULL, 0, XTYP_EXECUTE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    ok(op == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", op);
    ok(res == DDE_FACK, "Expected DDE_FACK, got %d\n", res);

    /* XTYP_EXECUTE, no data */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, NULL, 0, XTYP_EXECUTE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_MEMORY_ERROR, "Expected DMLERR_MEMORY_ERROR, got %d\n", ret);

    /* XTYP_EXECUTE, no data, -1 size */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, -1, conversation, NULL, 0, XTYP_EXECUTE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    DdeFreeStringHandle(client_pid, topic);
    DdeFreeDataHandle(hdata);

    item = DdeCreateStringHandleA(client_pid, "executed", CP_WINANSI);

    /* verify the execute */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);
    ok(res == DDE_FNOTPROCESSED, "Expected DDE_FNOTPROCESSED, got %d\n", res);
    if (hdata == NULL)
        ok(FALSE, "hdata is NULL\n");
    else
    {
        str = (LPSTR)DdeAccessData(hdata, &size);
        ok(!lstrcmpA(str, "command executed\r\n"), "Expected 'command executed\\r\\n', got %s\n", str);
        ok(size == 21, "Expected 21, got %d\n", size);

        ret = DdeUnaccessData(hdata);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }

    /* invalid transactions */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_ADVREQ, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_CONNECT, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_CONNECT_CONFIRM, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_DISCONNECT, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_ERROR, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_MONITOR, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REGISTER, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_UNREGISTER, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_WILDCONNECT, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    op = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_XACT_COMPLETE, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
    ok(op == NULL, "Expected NULL, got %p\n", op);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", res);
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);

    DdeFreeStringHandle(client_pid, item);

    ret = DdeDisconnect(conversation);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeUninitialize(client_pid);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
}

static DWORD server_pid;

static HDDEDATA CALLBACK server_ddeml_callback(UINT uType, UINT uFmt, HCONV hconv,
                                               HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    char str[MAX_PATH], *ptr;
    HDDEDATA ret = NULL;
    DWORD size;

    static int msg_index = 0;
    static HCONV conversation = 0;

    msg_index++;

    switch (uType)
    {
    case XTYP_REGISTER:
    {
        ok(msg_index == 1, "Expected 1, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv == 0, "Expected 0, got %p\n", hconv);
        ok(hdata == 0, "Expected 0, got %p\n", hdata);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == 0, "Expected 0, got %08lx\n", dwData2);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDEServer"), "Expected TestDDEServer, got %s\n", str);
        ok(size == 13, "Expected 13, got %d\n", size);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);
        ok(!strncmp(str, "TestDDEServer(", 14), "Expected TestDDEServer(, got %s\n", str);
        ok(str[size - 1] == ')', "Expected ')', got %c\n", str[size - 1]);
        ok(size == 25, "Expected 25, got %d\n", size);

        return (HDDEDATA)TRUE;
    }

    case XTYP_CONNECT:
    {
        ok(msg_index == 2, "Expected 2, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv == 0, "Expected 0, got %p\n", hconv);
        ok(hdata == 0, "Expected 0, got %p\n", hdata);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == FALSE, "Expected FALSE, got %08lx\n", dwData2);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDEServer"), "Expected TestDDEServer, got %s\n", str);
        ok(size == 13, "Expected 13, got %d\n", size);

        return (HDDEDATA)TRUE;
    }

    case XTYP_CONNECT_CONFIRM:
    {
        conversation = hconv;

        ok(msg_index == 3, "Expected 3, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv != NULL, "Expected non-NULL hconv\n");
        ok(hdata == 0, "Expected 0, got %p\n", hdata);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == FALSE, "Expected FALSE, got %08lx\n", dwData2);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDEServer"), "Expected TestDDEServer, got %s\n", str);
        ok(size == 13, "Expected 13, got %d\n", size);

        return (HDDEDATA)TRUE;
    }

    case XTYP_REQUEST:
    {
        ok(msg_index == 4 || msg_index == 5 || msg_index == 6,
           "Expected 4, 5 or 6, got %d\n", msg_index);
        ok(hconv == conversation, "Expected conversation handle, got %p\n", hconv);
        ok(hdata == 0, "Expected 0, got %p\n", hdata);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == 0, "Expected 0, got %08lx\n", dwData2);

        if (msg_index == 4)
            ok(uFmt == 0xbeef, "Expected 0xbeef, got %08x\n", uFmt);
        else
            ok(uFmt == CF_TEXT, "Expected CF_TEXT, got %08x\n", uFmt);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);

        if (msg_index == 5)
        {
            {
                ok(!lstrcmpA(str, ""), "Expected empty string, got %s\n", str);
                ok(size == 1, "Expected 1, got %d\n", size);
            }
        }
        else if (msg_index == 6)
        {
            ok(!lstrcmpA(str, "request"), "Expected request, got %s\n", str);
            ok(size == 7, "Expected 7, got %d\n", size);
        }

        if (msg_index == 6)
        {
            lstrcpyA(str, "requested data\r\n");
            return DdeCreateDataHandle(server_pid, (LPBYTE)str, lstrlenA(str) + 1,
                                        0, hsz2, CF_TEXT, 0);
        }

        return NULL;
    }

    case XTYP_POKE:
    {
        ok(msg_index == 7 || msg_index == 8, "Expected 7 or 8, got %d\n", msg_index);
        ok(uFmt == CF_TEXT, "Expected CF_TEXT, got %d\n", uFmt);
        ok(hconv == conversation, "Expected conversation handle, got %p\n", hconv);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == 0, "Expected 0, got %08lx\n", dwData2);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        ptr = (LPSTR)DdeAccessData(hdata, &size);
        ok(!lstrcmpA(ptr, "poke data\r\n"), "Expected 'poke data\\r\\n', got %s\n", ptr);
        ok(size == 12, "Expected 12, got %d\n", size);
        DdeUnaccessData(hdata);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);
        if (msg_index == 7)
        {
            {
                ok(!lstrcmpA(str, ""), "Expected empty string, got %s\n", str);
                ok(size == 1, "Expected 1, got %d\n", size);
            }
        }
        else
        {
            ok(!lstrcmpA(str, "poke"), "Expected poke, got %s\n", str);
            ok(size == 4, "Expected 4, got %d\n", size);
        }

        return (HDDEDATA)DDE_FACK;
    }

    case XTYP_EXECUTE:
    {
        ok(msg_index >= 9 && msg_index <= 11, "Expected 9 or 11, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv == conversation, "Expected conversation handle, got %p\n", hconv);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == 0, "Expected 0, got %08lx\n", dwData2);
        ok(hsz2 == 0, "Expected 0, got %p\n", hsz2);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        if (msg_index == 9 || msg_index == 11)
        {
            ptr = (LPSTR)DdeAccessData(hdata, &size);

            if (msg_index == 9)
            {
                ok(!lstrcmpA(ptr, "[Command(Var)]"), "Expected '[Command(Var)]', got %s\n", ptr);
                ok(size == 15, "Expected 15, got %d\n", size);
                ret = (HDDEDATA)DDE_FACK;
            }
            else
            {
                ok(!lstrcmpA(ptr, "[BadCommand(Var)]"), "Expected '[BadCommand(Var)]', got %s\n", ptr);
                ok(size == 18, "Expected 18, got %d\n", size);
                ret = DDE_FNOTPROCESSED;
            }

            DdeUnaccessData(hdata);
        }
        else if (msg_index == 10)
        {
            DWORD rsize = 0;
            size = 0;

            size = DdeGetData(hdata, NULL, 0, 0);
            ok(size == 17, "DdeGetData should have returned 17 not %d\n", size);
            ptr = HeapAlloc(GetProcessHeap(), 0, size);
            ok(ptr != NULL,"HeapAlloc should have returned ptr not NULL\n");
            rsize = DdeGetData(hdata, (LPBYTE)ptr, size, 0);
            ok(rsize == size, "DdeGetData did not return %d bytes but %d\n", size, rsize);

            ok(!lstrcmpA(ptr, "[Command-2(Var)]"), "Expected '[Command-2(Var)]' got %s\n", ptr);
            ok(size == 17, "Expected 17, got %d\n", size);
            ret = (HDDEDATA)DDE_FACK;

            HeapFree(GetProcessHeap(), 0, ptr);
        }

        return ret;
    }

    case XTYP_DISCONNECT:
    {
        ok(msg_index == 12, "Expected 12, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv == conversation, "Expected conversation handle, got %p\n", hconv);
        ok(dwData1 == 0, "Expected 0, got %08lx\n", dwData1);
        ok(dwData2 == 0, "Expected 0, got %08lx\n", dwData2);
        ok(hsz1 == 0, "Expected 0, got %p\n", hsz2);
        ok(hsz2 == 0, "Expected 0, got %p\n", hsz2);

        return 0;
    }

    default:
        ok(FALSE, "Unhandled msg: %08x\n", uType);
    }

    return 0;
}

static void test_ddeml_server(HANDLE hproc)
{
    MSG msg;
    UINT res;
    BOOL ret;
    HSZ server;
    HDDEDATA hdata;

    /* set up DDE server */
    server_pid = 0;
    res = DdeInitialize(&server_pid, server_ddeml_callback, APPCLASS_STANDARD, 0);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);

    server = DdeCreateStringHandle(server_pid, "TestDDEServer", CP_WINANSI);
    ok(server != NULL, "Expected non-NULL string handle\n");

    hdata = DdeNameService(server_pid, server, 0, DNS_REGISTER);
    ok(hdata == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", hdata);

    while (MsgWaitForMultipleObjects( 1, &hproc, FALSE, INFINITE, QS_ALLINPUT ) != 0)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    ret = DdeUninitialize(server_pid);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    GetExitCodeProcess( hproc, &res );
    ok( !res, "client failed with %u error(s)\n", res );
}

static HWND client_hwnd, server_hwnd;
static ATOM server, topic, item;
static HGLOBAL execute_hglobal;

static LRESULT WINAPI dde_msg_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    char str[MAX_PATH];
    UINT_PTR lo, hi;
    DDEDATA *data;
    DDEACK *ack;
    DWORD size;
    LPSTR ptr;

    static int msg_index = 0;

    if (msg < WM_DDE_FIRST || msg > WM_DDE_LAST)
        return DefWindowProcA(hwnd, msg, wparam, lparam);

    msg_index++;

    switch (msg)
    {
    case WM_DDE_INITIATE:
    {
        ok(msg_index == 1, "Expected 1, got %d\n", msg_index);
        ok(wparam == (WPARAM)client_hwnd, "Expected client hwnd, got %08lx\n", wparam);

        size = GlobalGetAtomNameA(LOWORD(lparam), str, MAX_PATH);
        ok(LOWORD(lparam) == server, "Expected server atom, got %08x\n", LOWORD(lparam));
        ok(!lstrcmpA(str, "TestDDEServer"), "Expected TestDDEServer, got %s\n", str);
        ok(size == 13, "Expected 13, got %d\n", size);

        size = GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
        ok(HIWORD(lparam) == topic, "Expected topic atom, got %08x\n", HIWORD(lparam));
        ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
        ok(size == 12, "Expected 12, got %d\n", size);

        break;
    }

    case WM_DDE_ACK:
    {
        ok((msg_index >= 2 && msg_index <= 4) || (msg_index >= 6 && msg_index <= 11),
           "Expected 2, 3, 4, 6, 7, 8, 9, 10 or 11, got %d\n", msg_index);

        if (msg_index == 2)
        {
            server_hwnd = (HWND)wparam;
            ok(wparam != 0, "Expected non-NULL wparam, got %08lx\n", wparam);

            size = GlobalGetAtomNameA(LOWORD(lparam), str, MAX_PATH);
            ok(LOWORD(lparam) == server, "Expected server atom, got %08x\n", LOWORD(lparam));
            ok(!lstrcmpA(str, "TestDDEServer"), "Expected TestDDEServer, got %s\n", str);
            ok(size == 13, "Expected 13, got %d\n", size);

            size = GlobalGetAtomNameA(HIWORD(lparam), str, MAX_PATH);
            ok(HIWORD(lparam) == topic, "Expected topic atom, got %08x\n", HIWORD(lparam));
            ok(!lstrcmpA(str, "TestDDETopic"), "Expected TestDDETopic, got %s\n", str);
            ok(size == 12, "Expected 12, got %d\n", size);
        }
        else if (msg_index >= 9 && msg_index <= 11)
        {
            ok(wparam == (WPARAM)server_hwnd, "Expected server hwnd, got %08lx\n", wparam);

            UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);

            ack = (DDEACK *)&lo;
            ok(ack->bAppReturnCode == 0, "Expected 0, got %d\n", ack->bAppReturnCode);
            ok(ack->reserved == 0, "Expected 0, got %d\n", ack->reserved);
            ok(ack->fBusy == FALSE, "Expected FALSE, got %d\n", ack->fBusy);

            ok(hi == (UINT_PTR)execute_hglobal, "Execpted execute hglobal, got %08lx\n", hi);
            ptr = GlobalLock((HGLOBAL)hi);

            if (msg_index == 9)
            {
                ok(ack->fAck == TRUE, "Expected TRUE, got %d\n", ack->fAck);
                ok(!lstrcmpA(ptr, "[Command(Var)]"), "Expected '[Command(Var)]', got %s\n", ptr);
            } else if (msg_index == 10)
            {
                ok(ack->fAck == TRUE, "Expected TRUE, got %d\n", ack->fAck);
                ok(!lstrcmpA(ptr, "[Command-2(Var)]"), "Expected '[Command-2(Var)]', got %s\n", ptr);
            }
            else
            {
                ok(ack->fAck == FALSE, "Expected FALSE, got %d\n", ack->fAck);
                ok(!lstrcmpA(ptr, "[BadCommand(Var)]"), "Expected '[BadCommand(Var)]', got %s\n", ptr);
            }

            GlobalUnlock((HGLOBAL)hi);
        }
        else
        {
            ok(wparam == (WPARAM)server_hwnd, "Expected server hwnd, got %08lx\n", wparam);

            UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);

            ack = (DDEACK *)&lo;
            ok(ack->bAppReturnCode == 0, "Expected 0, got %d\n", ack->bAppReturnCode);
            ok(ack->reserved == 0, "Expected 0, got %d\n", ack->reserved);
            ok(ack->fBusy == FALSE, "Expected FALSE, got %d\n", ack->fBusy);

            if (msg_index >= 7)
                ok(ack->fAck == TRUE, "Expected TRUE, got %d\n", ack->fAck);
            else
            {
                if (msg_index == 6) todo_wine
                ok(ack->fAck == FALSE, "Expected FALSE, got %d\n", ack->fAck);
            }

            size = GlobalGetAtomNameA(hi, str, MAX_PATH);
            if (msg_index == 3)
            {
                ok(hi == item, "Expected item atom, got %08lx\n", hi);
                ok(!lstrcmpA(str, "request"), "Expected request, got %s\n", str);
                ok(size == 7, "Expected 7, got %d\n", size);
            }
            else if (msg_index == 4 || msg_index == 7)
            {
                ok(hi == 0, "Expected 0, got %08lx\n", hi);
                ok(size == 0, "Expected empty string, got %d\n", size);
            }
            else
            {
                ok(hi == item, "Expected item atom, got %08lx\n", hi);
                if (msg_index == 6) todo_wine
                {
                    ok(!lstrcmpA(str, "poke"), "Expected poke, got %s\n", str);
                    ok(size == 4, "Expected 4, got %d\n", size);
                }
            }
        }

        break;
    }

    case WM_DDE_DATA:
    {
        ok(msg_index == 5, "Expected 5, got %d\n", msg_index);
        ok(wparam == (WPARAM)server_hwnd, "Expected server hwnd, got %08lx\n", wparam);

        UnpackDDElParam(WM_DDE_DATA, lparam, &lo, &hi);

        data = GlobalLock((HGLOBAL)lo);
        ok(data->unused == 0, "Expected 0, got %d\n", data->unused);
        ok(data->fResponse == TRUE, "Expected TRUE, got %d\n", data->fResponse);
        todo_wine
        {
            ok(data->fRelease == TRUE, "Expected TRUE, got %d\n", data->fRelease);
        }
        ok(data->fAckReq == 0, "Expected 0, got %d\n", data->fAckReq);
        ok(data->cfFormat == CF_TEXT, "Expected CF_TEXT, got %d\n", data->cfFormat);
        ok(!lstrcmpA((LPSTR)data->Value, "requested data\r\n"),
           "Expeted 'requested data\\r\\n', got %s\n", data->Value);
        GlobalUnlock((HGLOBAL)lo);

        size = GlobalGetAtomNameA(hi, str, MAX_PATH);
        ok(hi == item, "Expected item atom, got %08x\n", HIWORD(lparam));
        ok(!lstrcmpA(str, "request"), "Expected request, got %s\n", str);
        ok(size == 7, "Expected 7, got %d\n", size);

        GlobalFree((HGLOBAL)lo);
        GlobalDeleteAtom(hi);

        break;
    }

    default:
        ok(FALSE, "Unhandled msg: %08x\n", msg);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static HGLOBAL create_poke(void)
{
    HGLOBAL hglobal;
    DDEPOKE *poke;
    DWORD size;

    size = FIELD_OFFSET(DDEPOKE, Value[sizeof("poke data\r\n")]);
    hglobal = GlobalAlloc(GMEM_DDESHARE, size);
    ok(hglobal != 0, "Expected non-NULL hglobal\n");

    poke = GlobalLock(hglobal);
    poke->unused = 0;
    poke->fRelease = TRUE;
    poke->fReserved = TRUE;
    poke->cfFormat = CF_TEXT;
    lstrcpyA((LPSTR)poke->Value, "poke data\r\n");
    GlobalUnlock(hglobal);

    return hglobal;
}

static HGLOBAL create_execute(LPCSTR command)
{
    HGLOBAL hglobal;
    LPSTR ptr;

    hglobal = GlobalAlloc(GMEM_DDESHARE, lstrlenA(command) + 1);
    ok(hglobal != 0, "Expected non-NULL hglobal\n");

    ptr = GlobalLock(hglobal);
    lstrcpyA(ptr, command);
    GlobalUnlock(hglobal);

    return hglobal;
}

static void test_msg_client(void)
{
    HGLOBAL hglobal;
    LPARAM lparam;

    create_dde_window(&client_hwnd, "dde_client", dde_msg_client_wndproc);

    server = GlobalAddAtomA("TestDDEServer");
    ok(server != 0, "Expected non-NULL server\n");

    topic = GlobalAddAtomA("TestDDETopic");
    ok(topic != 0, "Expected non-NULL topic\n");

    SendMessageA(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)client_hwnd, MAKELONG(server, topic));

    GlobalDeleteAtom(server);
    GlobalDeleteAtom(topic);

    flush_events();

    item = GlobalAddAtom("request");
    ok(item != 0, "Expected non-NULL item\n");

    /* WM_DDE_REQUEST, bad clipboard format */
    lparam = PackDDElParam(WM_DDE_REQUEST, 0xdeadbeef, item);
    PostMessageA(server_hwnd, WM_DDE_REQUEST, (WPARAM)client_hwnd, lparam);

    flush_events();

    /* WM_DDE_REQUEST, no item */
    lparam = PackDDElParam(WM_DDE_REQUEST, CF_TEXT, 0);
    PostMessageA(server_hwnd, WM_DDE_REQUEST, (WPARAM)client_hwnd, lparam);

    flush_events();

    /* WM_DDE_REQUEST, no client hwnd */
    lparam = PackDDElParam(WM_DDE_REQUEST, CF_TEXT, item);
    PostMessageA(server_hwnd, WM_DDE_REQUEST, 0, lparam);

    flush_events();

    /* WM_DDE_REQUEST, correct params */
    lparam = PackDDElParam(WM_DDE_REQUEST, CF_TEXT, item);
    PostMessageA(server_hwnd, WM_DDE_REQUEST, (WPARAM)client_hwnd, lparam);

    flush_events();

    GlobalDeleteAtom(item);
    item = GlobalAddAtomA("poke");
    ok(item != 0, "Expected non-NULL item\n");

    hglobal = create_poke();

    /* WM_DDE_POKE, no ddepoke */
    lparam = PackDDElParam(WM_DDE_POKE, 0, item);
    /* win9x returns 0 here and crashes in PostMessageA */
    if (lparam) {
        PostMessageA(server_hwnd, WM_DDE_POKE, (WPARAM)client_hwnd, lparam);
        flush_events();
    }
    else
        win_skip("no lparam for WM_DDE_POKE\n");


    /* WM_DDE_POKE, no item */
    lparam = PackDDElParam(WM_DDE_POKE, (UINT_PTR)hglobal, 0);
    PostMessageA(server_hwnd, WM_DDE_POKE, (WPARAM)client_hwnd, lparam);

    flush_events();

    hglobal = create_poke();

    /* WM_DDE_POKE, no client hwnd */
    lparam = PackDDElParam(WM_DDE_POKE, (UINT_PTR)hglobal, item);
    PostMessageA(server_hwnd, WM_DDE_POKE, 0, lparam);

    flush_events();

    /* WM_DDE_POKE, all params correct */
    lparam = PackDDElParam(WM_DDE_POKE, (UINT_PTR)hglobal, item);
    PostMessageA(server_hwnd, WM_DDE_POKE, (WPARAM)client_hwnd, lparam);

    flush_events();

    execute_hglobal = create_execute("[Command(Var)]");

    /* WM_DDE_EXECUTE, no lparam */
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, (WPARAM)client_hwnd, 0);

    flush_events();

    /* WM_DDE_EXECUTE, no hglobal */
    lparam = PackDDElParam(WM_DDE_EXECUTE, 0, 0);
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, (WPARAM)client_hwnd, lparam);

    flush_events();

    /* WM_DDE_EXECUTE, no client hwnd */
    lparam = PackDDElParam(WM_DDE_EXECUTE, 0, (UINT_PTR)execute_hglobal);
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, 0, lparam);

    flush_events();

    /* WM_DDE_EXECUTE, all params correct */
    lparam = PackDDElParam(WM_DDE_EXECUTE, 0, (UINT_PTR)execute_hglobal);
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, (WPARAM)client_hwnd, lparam);

    flush_events();

    GlobalFree(execute_hglobal);
    execute_hglobal = create_execute("[Command-2(Var)]");

    /* WM_DDE_EXECUTE, all params correct */
    lparam = PackDDElParam(WM_DDE_EXECUTE, 0, (UINT_PTR)execute_hglobal);
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, (WPARAM)client_hwnd, lparam);

    flush_events();

    GlobalFree(execute_hglobal);
    execute_hglobal = create_execute("[BadCommand(Var)]");

    /* WM_DDE_EXECUTE that will get rejected */
    lparam = PackDDElParam(WM_DDE_EXECUTE, 0, (UINT_PTR)execute_hglobal);
    PostMessageA(server_hwnd, WM_DDE_EXECUTE, (WPARAM)client_hwnd, lparam);

    flush_events();

    destroy_dde_window(&client_hwnd, "dde_client");
}

static LRESULT WINAPI hook_dde_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT_PTR lo, hi;

    trace("hook_dde_client_wndproc: %p %04x %08lx %08lx\n", hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_DDE_ACK:
        UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);
        trace("WM_DDE_ACK: status %04lx hglobal %p\n", lo, (HGLOBAL)hi);
        break;

    default:
        break;
    }
    return CallWindowProcA(old_dde_client_wndproc, hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI dde_server_wndprocW(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    trace("dde_server_wndprocW: %p %04x %08lx %08lx\n", hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_DDE_INITIATE:
    {
        ATOM aService = GlobalAddAtomW(TEST_DDE_SERVICE);

        trace("server: got WM_DDE_INITIATE from %p with %08lx\n", (HWND)wparam, lparam);

        if (LOWORD(lparam) == aService)
        {
            ok(!IsWindowUnicode((HWND)wparam), "client should be an ANSI window\n");
            old_dde_client_wndproc = (WNDPROC)SetWindowLongPtrA((HWND)wparam, GWLP_WNDPROC, (ULONG_PTR)hook_dde_client_wndproc);
            trace("server: sending WM_DDE_ACK to %p\n", (HWND)wparam);
            SendMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, MAKELPARAM(aService, 0));
        }
        else
            GlobalDeleteAtom(aService);
        return 0;
    }

    case WM_DDE_EXECUTE:
    {
        DDEACK ack;
        WORD status;
        LPCSTR cmd;
        UINT_PTR lo, hi;

        trace("server: got WM_DDE_EXECUTE from %p with %08lx\n", (HWND)wparam, lparam);

        UnpackDDElParam(WM_DDE_EXECUTE, lparam, &lo, &hi);
        trace("%08lx => lo %04lx hi %04lx\n", lparam, lo, hi);

        ack.bAppReturnCode = 0;
        ack.reserved = 0;
        ack.fBusy = 0;

        cmd = GlobalLock((HGLOBAL)hi);
        if (!cmd || (lstrcmpA(cmd, exec_cmdA) && lstrcmpW((LPCWSTR)cmd, exec_cmdW)))
        {
            trace("ignoring unknown WM_DDE_EXECUTE command\n");
            /* We have to send a negative acknowledge even if we don't
             * accept the command, otherwise Windows goes mad and next time
             * we send an acknowledge DDEML drops the connection.
             * Not sure how to call it: a bug or a feature.
             */
            ack.fAck = 0;
        }
        else
            ack.fAck = 1;
        GlobalUnlock((HGLOBAL)hi);

        trace("server: posting %s WM_DDE_ACK to %p\n", ack.fAck ? "POSITIVE" : "NEGATIVE", (HWND)wparam);

        status = *((WORD *)&ack);
        lparam = ReuseDDElParam(lparam, WM_DDE_EXECUTE, WM_DDE_ACK, status, hi);

        PostMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, lparam);
        return 0;
    }

    case WM_DDE_TERMINATE:
    {
        DDEACK ack;
        WORD status;

        trace("server: got WM_DDE_TERMINATE from %p with %08lx\n", (HWND)wparam, lparam);

        ack.bAppReturnCode = 0;
        ack.reserved = 0;
        ack.fBusy = 0;
        ack.fAck = 1;

        trace("server: posting %s WM_DDE_ACK to %p\n", ack.fAck ? "POSITIVE" : "NEGATIVE", (HWND)wparam);

        status = *((WORD *)&ack);
        lparam = PackDDElParam(WM_DDE_ACK, status, 0);

        PostMessageW((HWND)wparam, WM_DDE_ACK, (WPARAM)hwnd, lparam);
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI dde_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL create_dde_windows(HWND *client, HWND *server)
{
    WNDCLASSA wcA;
    WNDCLASSW wcW;
    static const WCHAR server_class_name[] = {'d','d','e','_','s','e','r','v','e','r','_','w','i','n','d','o','w',0};
    static const char client_class_name[] = "dde_client_window";

    memset(&wcW, 0, sizeof(wcW));
    wcW.lpfnWndProc = dde_server_wndprocW;
    wcW.lpszClassName = server_class_name;
    wcW.hInstance = GetModuleHandleA(0);
    if (!RegisterClassW(&wcW)) return FALSE;

    memset(&wcA, 0, sizeof(wcA));
    wcA.lpfnWndProc = dde_client_wndproc;
    wcA.lpszClassName = client_class_name;
    wcA.hInstance = GetModuleHandleA(0);
    assert(RegisterClassA(&wcA));

    *server = CreateWindowExW(0, server_class_name, NULL,
                              WS_POPUP,
                              100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                              GetDesktopWindow(), 0,
                              GetModuleHandleA(0), NULL);
    assert(*server);

    *client = CreateWindowExA(0, client_class_name, NULL,
                              WS_POPUP,
                              100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                              GetDesktopWindow(), 0,
                              GetModuleHandleA(0), NULL);
    assert(*client);

    trace("server hwnd %p, client hwnd %p\n", *server, *client);

    ok(IsWindowUnicode(*server), "server has to be a unicode window\n");
    ok(!IsWindowUnicode(*client), "client has to be an ANSI window\n");

    return TRUE;
}

static HDDEDATA CALLBACK client_dde_callback(UINT uType, UINT uFmt, HCONV hconv,
                                     HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                     ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    static const char * const cmd_type[15] = {
        "XTYP_ERROR", "XTYP_ADVDATA", "XTYP_ADVREQ", "XTYP_ADVSTART",
        "XTYP_ADVSTOP", "XTYP_EXECUTE", "XTYP_CONNECT", "XTYP_CONNECT_CONFIRM",
        "XTYP_XACT_COMPLETE", "XTYP_POKE", "XTYP_REGISTER", "XTYP_REQUEST",
        "XTYP_DISCONNECT", "XTYP_UNREGISTER", "XTYP_WILDCONNECT" };
    UINT type;
    const char *cmd_name;

    type = (uType & XTYP_MASK) >> XTYP_SHIFT;
    cmd_name = (type <= 14) ? cmd_type[type] : "unknown";

    trace("client_dde_callback: %04x (%s) %d %p %p %p %p %08lx %08lx\n",
          uType, cmd_name, uFmt, hconv, hsz1, hsz2, hdata, dwData1, dwData2);
    return 0;
}

static void test_dde_aw_transaction(void)
{
    HSZ hsz_server;
    DWORD dde_inst, ret, err;
    HCONV hconv;
    HWND hwnd_client, hwnd_server;
    CONVINFO info;
    HDDEDATA hdata;
    static char test_cmd[] = "test dde command";

    /* server: unicode, client: ansi */
    if (!create_dde_windows(&hwnd_client, &hwnd_server)) return;

    dde_inst = 0;
    ret = DdeInitializeA(&dde_inst, client_dde_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeA failed with error %04x (%x)\n",
       ret, DdeGetLastError(dde_inst));

    hsz_server = DdeCreateStringHandleW(dde_inst, TEST_DDE_SERVICE, CP_WINUNICODE);

    hconv = DdeConnect(dde_inst, hsz_server, 0, NULL);
    ok(hconv != 0, "DdeConnect error %x\n", DdeGetLastError(dde_inst));
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(ret, "wrong info size %d, DdeQueryConvInfo error %x\n", ret, DdeGetLastError(dde_inst));
    /* should be CP_WINANSI since we used DdeInitializeA */
    ok(info.ConvCtxt.iCodePage == CP_WINANSI, "wrong iCodePage %d\n", info.ConvCtxt.iCodePage);
    ok(!info.hConvPartner, "unexpected info.hConvPartner: %p\n", info.hConvPartner);
todo_wine {
    ok((info.wStatus & DDE_FACK), "unexpected info.wStatus: %04x\n", info.wStatus);
}
    ok((info.wStatus & (ST_CONNECTED | ST_CLIENT)) == (ST_CONNECTED | ST_CLIENT), "unexpected info.wStatus: %04x\n", info.wStatus);
    ok(info.wConvst == XST_CONNECTED, "unexpected info.wConvst: %04x\n", info.wConvst);
    ok(info.wType == 0, "unexpected info.wType: %04x\n", info.wType);

    trace("hwnd %p, hwndPartner %p\n", info.hwnd, info.hwndPartner);

    trace("sending test client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)test_cmd, strlen(test_cmd) + 1, hconv, (HSZ)0xdead, 0xbeef, XTYP_EXECUTE, 1000, &ret);
    ok(!hdata, "DdeClientTransaction succeeded\n");
    ok(ret == DDE_FNOTPROCESSED, "wrong status code %04x\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NOTPROCESSED, "wrong dde error %x\n", err);

    trace("sending ANSI client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdA, lstrlenA(exec_cmdA) + 1, hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04x\n", ret);

    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    trace("sending unicode client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdW, (lstrlenW(exec_cmdW) + 1) * sizeof(WCHAR), hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04x\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    ok(DdeDisconnect(hconv), "DdeDisconnect error %x\n", DdeGetLastError(dde_inst));

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(!ret, "DdeQueryConvInfo should fail\n");
    err = DdeGetLastError(dde_inst);
todo_wine {
    ok(err == DMLERR_INVALIDPARAMETER, "wrong dde error %x\n", err);
}

    ok(DdeFreeStringHandle(dde_inst, hsz_server), "DdeFreeStringHandle error %x\n", DdeGetLastError(dde_inst));

    /* This call hangs on win2k SP4 and XP SP1.
    DdeUninitialize(dde_inst);*/

    DestroyWindow(hwnd_client);
    DestroyWindow(hwnd_server);
}

static void test_initialisation(void)
{
    UINT ret;
    DWORD res;
    HDDEDATA hdata;
    HSZ server, topic, item;
    DWORD client_pid;
    HCONV conversation;

    /* Initialise without a valid server window. */
    client_pid = 0;
    ret = DdeInitializeA(&client_pid, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", ret);


    server = DdeCreateStringHandleA(client_pid, "TestDDEService", CP_WINANSI);
    topic = DdeCreateStringHandleA(client_pid, "TestDDETopic", CP_WINANSI);

    DdeGetLastError(client_pid);

    /* There is no server window so no conversation can be extracted */
    conversation = DdeConnect(client_pid, server, topic, NULL);
    ok(conversation == NULL, "Expected NULL conversation, %p\n", conversation);
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_CONV_ESTABLISHED, "Expected DMLERR_NO_CONV_ESTABLISHED, got %d\n", ret);

    DdeFreeStringHandle(client_pid, server);

    item = DdeCreateStringHandleA(client_pid, "request", CP_WINANSI);

    /* There is no converstation so an invalild parameter results */
    res = 0xdeadbeef;
    DdeGetLastError(client_pid);
    hdata = DdeClientTransaction(NULL, 0, conversation, item, CF_TEXT, XTYP_REQUEST, default_timeout, &res);
    ret = DdeGetLastError(client_pid);
todo_wine
    ok(ret == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", ret);
    ok(res == 0xdeadbeef, "Expected 0xdeadbeef, got %08x\n", res);

    DdeFreeStringHandle(client_pid, server);
    ret = DdeDisconnect(conversation);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);

    ret = DdeUninitialize(client_pid);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
}

static void test_DdeCreateStringHandleW(DWORD dde_inst, int codepage)
{
    static const WCHAR dde_string[] = {'D','D','E',' ','S','t','r','i','n','g',0};
    HSZ str_handle;
    WCHAR bufW[256];
    char buf[256];
    ATOM atom;
    int ret;

    str_handle = DdeCreateStringHandleW(dde_inst, dde_string, codepage);
    ok(str_handle != 0, "DdeCreateStringHandleW failed with error %08x\n",
       DdeGetLastError(dde_inst));

    ret = DdeQueryStringW(dde_inst, str_handle, NULL, 0, codepage);
    if (codepage == CP_WINANSI)
        ok(ret == 1, "DdeQueryStringW returned wrong length %d\n", ret);
    else
        ok(ret == lstrlenW(dde_string), "DdeQueryStringW returned wrong length %d\n", ret);

    ret = DdeQueryStringW(dde_inst, str_handle, bufW, 256, codepage);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringW returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", (LPCSTR)bufW), "DdeQueryStringW returned wrong string\n");
    }
    else
    {
        ok(ret == lstrlenW(dde_string), "DdeQueryStringW returned wrong length %d\n", ret);
        ok(!lstrcmpW(dde_string, bufW), "DdeQueryStringW returned wrong string\n");
    }

    ret = DdeQueryStringA(dde_inst, str_handle, buf, 256, CP_WINANSI);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", buf), "DdeQueryStringW returned wrong string\n");
    }
    else
    {
        ok(ret == lstrlenA("DDE String"), "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("DDE String", buf), "DdeQueryStringA returned wrong string %s\n", buf);
    }

    ret = DdeQueryStringA(dde_inst, str_handle, buf, 256, CP_WINUNICODE);
    if (codepage == CP_WINANSI)
    {
        ok(ret == 1, "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpA("D", buf), "DdeQueryStringA returned wrong string %s\n", buf);
    }
    else
    {
        ok(ret == lstrlenA("DDE String"), "DdeQueryStringA returned wrong length %d\n", ret);
        ok(!lstrcmpW(dde_string, (LPCWSTR)buf), "DdeQueryStringW returned wrong string\n");
    }

    if (codepage == CP_WINANSI)
    {
        atom = FindAtomA((LPSTR)dde_string);
        ok(atom != 0, "Expected a valid atom\n");

        SetLastError(0xdeadbeef);
        atom = GlobalFindAtomA((LPSTR)dde_string);
        ok(atom == 0, "Expected 0, got %d\n", atom);
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }
    else
    {
        atom = FindAtomW(dde_string);
        ok(atom != 0, "Expected a valid atom\n");

        SetLastError(0xdeadbeef);
        atom = GlobalFindAtomW(dde_string);
        ok(atom == 0, "Expected 0, got %d\n", atom);
        ok(GetLastError() == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    }

    ok(DdeFreeStringHandle(dde_inst, str_handle), "DdeFreeStringHandle failed\n");
}

static void test_DdeCreateDataHandle(void)
{
    HDDEDATA hdata;
    DWORD dde_inst, dde_inst2;
    DWORD size;
    UINT res, err;
    BOOL ret;
    HSZ item;
    LPBYTE ptr;
    WCHAR item_str[] = {'i','t','e','m',0};

    dde_inst = 0;
    dde_inst2 = 0;
    res = DdeInitializeA(&dde_inst, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);

    res = DdeInitializeA(&dde_inst2, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);

    /* 0 instance id
     * This block tests an invalid instance Id.  The correct behaviour is that if the instance Id
     * is invalid then the lastError of all instances is set to the error.  There are two instances
     * created, lastError is cleared, an error is generated and then both instances are checked to
     * ensure that they both have the same error set
     */
    item = DdeCreateStringHandleA(0, "item", CP_WINANSI);
    ok(item == NULL, "Expected NULL hsz got %p\n", item);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);
    err = DdeGetLastError(dde_inst2);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);
    item = DdeCreateStringHandleW(0, item_str, CP_WINUNICODE);
    ok(item == NULL, "Expected NULL hsz got %p\n", item);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);
    err = DdeGetLastError(dde_inst2);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);

    item = DdeCreateStringHandleA(dde_inst, "item", CP_WINANSI);
    ok(item != NULL, "Expected non-NULL hsz\n");
    item = DdeCreateStringHandleA(dde_inst2, "item", CP_WINANSI);
    ok(item != NULL, "Expected non-NULL hsz\n");

    if (0) {
        /* do not test with an invalid instance id: that crashes on win9x */
        hdata = DdeCreateDataHandle(0xdeadbeef, (LPBYTE)"data", MAX_PATH, 0, item, CF_TEXT, 0);
    }

    /* 0 instance id
     * This block tests an invalid instance Id.  The correct behaviour is that if the instance Id
     * is invalid then the lastError of all instances is set to the error.  There are two instances
     * created, lastError is cleared, an error is generated and then both instances are checked to
     * ensure that they both have the same error set
     */
    DdeGetLastError(dde_inst);
    DdeGetLastError(dde_inst2);
    hdata = DdeCreateDataHandle(0, (LPBYTE)"data", MAX_PATH, 0, item, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata == NULL, "Expected NULL, got %p\n", hdata);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);
    err = DdeGetLastError(dde_inst2);
    ok(err == DMLERR_INVALIDPARAMETER, "Expected DMLERR_INVALIDPARAMETER, got %d\n", err);

    ret = DdeUninitialize(dde_inst2);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);


    /* NULL pSrc */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, NULL, MAX_PATH, 0, item, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(size == 260, "Expected 260, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* cb is zero */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, (LPBYTE)"data", 0, 0, item, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(size == 0, "Expected 0, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* cbOff is non-zero */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, (LPBYTE)"data", MAX_PATH, 2, item, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(size == 262, "Expected 262, got %d\n", size);
    todo_wine
    {
        ok(lstrlenA((LPSTR)ptr) == 0, "Expected 0, got %d\n", lstrlenA((LPSTR)ptr));
    }

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* NULL item */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, (LPBYTE)"data", MAX_PATH, 0, 0, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(!lstrcmpA((LPSTR)ptr, "data"), "Expected data, got %s\n", ptr);
    ok(size == 260, "Expected 260, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* NULL item */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, (LPBYTE)"data", MAX_PATH, 0, (HSZ)0xdeadbeef, CF_TEXT, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(!lstrcmpA((LPSTR)ptr, "data"), "Expected data, got %s\n", ptr);
    ok(size == 260, "Expected 260, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* invalid clipboard format */
    DdeGetLastError(dde_inst);
    hdata = DdeCreateDataHandle(dde_inst, (LPBYTE)"data", MAX_PATH, 0, item, 0xdeadbeef, 0);
    err = DdeGetLastError(dde_inst);
    ok(hdata != NULL, "Expected non-NULL hdata\n");
    ok(err == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", err);

    ptr = GlobalLock(hdata);
    todo_wine
    {
        ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    }

    ptr = DdeAccessData(hdata, &size);
    ok(ptr != NULL, "Expected non-NULL ptr\n");
    ok(!lstrcmpA((LPSTR)ptr, "data"), "Expected data, got %s\n", ptr);
    ok(size == 260, "Expected 260, got %d\n", size);

    ret = DdeUnaccessData(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeFreeDataHandle(hdata);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    ret = DdeUninitialize(dde_inst);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);
}

static void test_DdeCreateStringHandle(void)
{
    DWORD dde_inst, ret;

    dde_inst = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = DdeInitializeW(&dde_inst, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("DdeInitializeW is unimplemented\n");
        return;
    }

    ok(ret == DMLERR_INVALIDPARAMETER, "DdeInitializeW should fail, but got %04x instead\n", ret);
    ok(DdeGetLastError(dde_inst) == DMLERR_INVALIDPARAMETER, "expected DMLERR_INVALIDPARAMETER\n");

    dde_inst = 0;
    ret = DdeInitializeW(&dde_inst, client_ddeml_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04x (%08x)\n",
       ret, DdeGetLastError(dde_inst));

    test_DdeCreateStringHandleW(dde_inst, 0);
    test_DdeCreateStringHandleW(dde_inst, CP_WINUNICODE);
    test_DdeCreateStringHandleW(dde_inst, CP_WINANSI);

    ok(DdeUninitialize(dde_inst), "DdeUninitialize failed\n");
}

static void test_FreeDDElParam(void)
{
    HGLOBAL val, hglobal;
    BOOL ret;

    ret = FreeDDElParam(WM_DDE_INITIATE, 0);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_INITIATE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_ADVISE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_UNADVISE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_ACK, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_DATA, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_REQUEST, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_POKE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == hglobal, "Expected hglobal, got %p\n", val);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    hglobal = GlobalAlloc(GMEM_DDESHARE, 100);
    ret = FreeDDElParam(WM_DDE_EXECUTE, (LPARAM)hglobal);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    val = GlobalFree(hglobal);
    ok(val == NULL, "Expected NULL, got %p\n", val);
}

static void test_PackDDElParam(void)
{
    UINT_PTR lo, hi, *ptr;
    HGLOBAL hglobal;
    LPARAM lparam;
    BOOL ret;

    lparam = PackDDElParam(WM_DDE_INITIATE, 0xcafe, 0xbeef);
    /* value gets sign-extended despite being an LPARAM */
    ok(lparam == (int)0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_INITIATE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_INITIATE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_TERMINATE, 0xcafe, 0xbeef);
    ok(lparam == (int)0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_TERMINATE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_TERMINATE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_ADVISE, 0xcafe, 0xbeef);
    /* win9x returns 0 here */
    if (lparam) {
        ptr = GlobalLock((HGLOBAL)lparam);
        ok(ptr != NULL, "Expected non-NULL ptr\n");
        ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
        ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

        ret = GlobalUnlock((HGLOBAL)lparam);
        ok(ret == 1, "Expected 1, got %d\n", ret);

        lo = hi = 0;
        ret = UnpackDDElParam(WM_DDE_ADVISE, lparam, &lo, &hi);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
        ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
        ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);
    }
    else
        win_skip("no lparam for WM_DDE_ADVISE\n");

    ret = FreeDDElParam(WM_DDE_ADVISE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_UNADVISE, 0xcafe, 0xbeef);
    ok(lparam == (int)0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_UNADVISE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_UNADVISE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_ACK, 0xcafe, 0xbeef);
    /* win9x returns the input (0xbeef<<16 | 0xcafe) here */
    if (lparam != (int)0xbeefcafe) {
        ptr = GlobalLock((HGLOBAL)lparam);
        ok(ptr != NULL, "Expected non-NULL ptr\n");
        ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
        ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

        ret = GlobalUnlock((HGLOBAL)lparam);
        ok(ret == 1, "Expected 1, got %d\n", ret);

        lo = hi = 0;
        ret = UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
        ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
        ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

        ret = FreeDDElParam(WM_DDE_ACK, lparam);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

        hglobal = GlobalFree((HGLOBAL)lparam);
        ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
        ok(GetLastError() == ERROR_INVALID_HANDLE,
           "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    }
    else
        win_skip("got lparam 0x%lx for WM_DDE_ACK\n", lparam);

    lparam = PackDDElParam(WM_DDE_DATA, 0xcafe, 0xbeef);
    /* win9x returns 0 here */
    if (lparam) {
        ptr = GlobalLock((HGLOBAL)lparam);
        ok(ptr != NULL, "Expected non-NULL ptr\n");
        ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
        ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

        ret = GlobalUnlock((HGLOBAL)lparam);
        ok(ret == 1, "Expected 1, got %d\n", ret);

        lo = hi = 0;
        ret = UnpackDDElParam(WM_DDE_DATA, lparam, &lo, &hi);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
        ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
        ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);
    }
    else
        win_skip("no lparam for WM_DDE_DATA\n");

    ret = FreeDDElParam(WM_DDE_DATA, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_REQUEST, 0xcafe, 0xbeef);
    ok(lparam == (int)0xbeefcafe, "Expected 0xbeefcafe, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_REQUEST, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_REQUEST, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    lparam = PackDDElParam(WM_DDE_POKE, 0xcafe, 0xbeef);
    /* win9x returns 0 here */
    if (lparam) {
        ptr = GlobalLock((HGLOBAL)lparam);
        ok(ptr != NULL, "Expected non-NULL ptr\n");
        ok(ptr[0] == 0xcafe, "Expected 0xcafe, got %08lx\n", ptr[0]);
        ok(ptr[1] == 0xbeef, "Expected 0xbeef, got %08lx\n", ptr[1]);

        ret = GlobalUnlock((HGLOBAL)lparam);
        ok(ret == 1, "Expected 1, got %d\n", ret);

        lo = hi = 0;
        ret = UnpackDDElParam(WM_DDE_POKE, lparam, &lo, &hi);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
        ok(lo == 0xcafe, "Expected 0xcafe, got %08lx\n", lo);
        ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);
    }
    else
        win_skip("no lparam for WM_DDE_POKE\n");

    ret = FreeDDElParam(WM_DDE_POKE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    hglobal = GlobalFree((HGLOBAL)lparam);
    ok(hglobal == (HGLOBAL)lparam, "Expected lparam, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lparam = PackDDElParam(WM_DDE_EXECUTE, 0xcafe, 0xbeef);
    ok(lparam == 0xbeef, "Expected 0xbeef, got %08lx\n", lparam);
    ok(GlobalLock((HGLOBAL)lparam) == NULL,
       "Expected NULL, got %p\n", GlobalLock((HGLOBAL)lparam));
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    lo = hi = 0;
    ret = UnpackDDElParam(WM_DDE_EXECUTE, lparam, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    ret = FreeDDElParam(WM_DDE_EXECUTE, lparam);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
}

static void test_UnpackDDElParam(void)
{
    UINT_PTR lo, hi, *ptr;
    HGLOBAL hglobal;
    BOOL ret;

    /* NULL lParam */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);

    /* NULL lo */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, NULL, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xdead, "Expected 0xdead, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    /* NULL hi */
    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, &lo, NULL);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xbeef, "Expected 0xbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_INITIATE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_TERMINATE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, 0, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0 ||
       broken(lo == 0xdead), /* win2k */
       "Expected 0, got %08lx\n", lo);
    ok(hi == 0 ||
       broken(hi == 0xbeef), /* win2k */
       "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0 ||
       broken(lo == 0xdead), /* win2k */
       "Expected 0, got %08lx\n", lo);
    ok(hi == 0 ||
       broken(hi == 0xbeef), /* win2k */
       "Expected 0, got %08lx\n", hi);

    hglobal = GlobalAlloc(GMEM_DDESHARE, 2);
    ptr = GlobalLock(hglobal);
    ptr[0] = 0xcafebabe;
    ptr[1] = 0xdeadbeef;
    GlobalUnlock(hglobal);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ADVISE, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_UNADVISE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ACK, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0 ||
       broken(lo == 0xdead), /* win2k */
       "Expected 0, got %08lx\n", lo);
    ok(hi == 0 ||
       broken(hi == 0xbeef), /* win2k */
       "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_ACK, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_DATA, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0 ||
       broken(lo == 0xdead), /* win2k */
       "Expected 0, got %08lx\n", lo);
    ok(hi == 0 ||
       broken(hi == 0xbeef), /* win2k */
       "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_DATA, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_REQUEST, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xbabe, "Expected 0xbabe, got %08lx\n", lo);
    ok(hi == 0xcafe, "Expected 0xcafe, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_POKE, 0xcafebabe, &lo, &hi);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(lo == 0 ||
       broken(lo == 0xdead), /* win2k */
       "Expected 0, got %08lx\n", lo);
    ok(hi == 0 ||
       broken(hi == 0xbeef), /* win2k */
       "Expected 0, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_POKE, (LPARAM)hglobal, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", lo);
    ok(hi == 0xdeadbeef, "Expected 0xdeadbeef, got %08lx\n", hi);

    lo = 0xdead;
    hi = 0xbeef;
    ret = UnpackDDElParam(WM_DDE_EXECUTE, 0xcafebabe, &lo, &hi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);
    ok(hi == 0xcafebabe, "Expected 0xcafebabe, got %08lx\n", hi);
}

static HDDEDATA CALLBACK server_end_to_end_callback(UINT uType, UINT uFmt, HCONV hconv,
                                               HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    DWORD size, rsize;
    char str[MAX_PATH];
    static int msg_index = 0;
    static HCONV conversation = 0;
    static char test_cmd_w_to_a[] = "test dde command";
    static char test_cmd_a_to_a[] = "Test dde command";
    static WCHAR test_cmd_w_to_w[] = {'t','e','s','t',' ','d','d','e',' ','c','o','m','m','a','n','d',0};
    static WCHAR test_cmd_a_to_w[] = {'T','e','s','t',' ','d','d','e',' ','c','o','m','m','a','n','d',0};
    static char test_service [] = "TestDDEService";
    static char test_topic [] = "TestDDETopic";

    msg_index++;

    switch (uType)
    {
    case XTYP_REGISTER:
    {
        ok(msg_index == 1 || msg_index == 7 || msg_index == 13 || msg_index == 19,
                             "Expected 1, 7, 13 or 19, got %d\n", msg_index);
        return (HDDEDATA)TRUE;
    }

    case XTYP_CONNECT:
    {
        ok(msg_index == 2 || msg_index == 8 || msg_index == 14 || msg_index == 20,
                             "Expected 2, 8, 14 or 20, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d, msg_index=%d\n", uFmt, msg_index);
        ok(hconv == 0, "Expected 0, got %p, msg_index=%d\n", hconv, msg_index);
        ok(hdata == 0, "Expected 0, got %p, msg_index=%d\n", hdata, msg_index);
        ok(dwData1 != 0, "Expected not 0, got %08lx, msg_index=%d\n", dwData1, msg_index);
        ok(dwData2 == FALSE, "Expected FALSE, got %08lx, msg_index=%d\n", dwData2, msg_index);

        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, test_topic), "Expected %s, got %s, msg_index=%d\n",
                             test_topic, str, msg_index);
        ok(size == 12, "Expected 12, got %d, msg_index=%d\n", size, msg_index);

        size = DdeQueryStringA(server_pid, hsz2, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, test_service), "Expected %s, got %s, msg_index=%d\n",
                             test_service, str, msg_index);
        ok(size == 14, "Expected 14, got %d, msg_index=%d\n", size, msg_index);

        return (HDDEDATA) TRUE;
    }
    case XTYP_CONNECT_CONFIRM:
    {
        ok(msg_index == 3 || msg_index == 9  ||  msg_index == 15 ||  msg_index == 21,
                             "Expected 3, 9, 15 or 21 got %d\n", msg_index);
        conversation = hconv;
        return (HDDEDATA) TRUE;
    }
    case XTYP_EXECUTE:
    {
        BYTE *buffer = NULL;

        ok(msg_index == 4 || msg_index == 5 || msg_index == 10 || msg_index == 11 ||
           msg_index == 16 || msg_index == 17 || msg_index == 22 || msg_index == 23,
           "Expected 4, 5, 10, 11, 16, 17, 22 or 23, got %d\n", msg_index);
        ok(uFmt == 0, "Expected 0, got %d\n", uFmt);
        ok(hconv == conversation, "Expected conversation handle, got %p, msg_index=%d\n",
                             hconv, msg_index);
        ok(dwData1 == 0, "Expected 0, got %08lx, msg_index=%d\n", dwData1, msg_index);
        ok(dwData2 == 0, "Expected 0, got %08lx, msg_index=%d\n", dwData2, msg_index);
        ok(hsz2 == 0, "Expected 0, got %p, msg_index=%d\n", hsz2, msg_index);
        size = DdeQueryStringA(server_pid, hsz1, str, MAX_PATH, CP_WINANSI);
        ok(!lstrcmpA(str, test_topic), "Expected %s, got %s, msg_index=%d\n",
                             test_topic, str, msg_index);
        ok(size == 12, "Expected 12, got %d, msg_index=%d\n", size, msg_index);
        ok(size == 12, "Expected 12, got %d, msg_index=%d\n", size, msg_index);

        size = DdeGetData(hdata, NULL, 0, 0);
        if (msg_index == 10 || msg_index ==11 || msg_index == 16 || msg_index ==17)
          if (msg_index == 10 || msg_index == 16)
          todo_wine
            ok(size == 34, "Expected that size should be 34 not %d, msg_index=%d\n",
                             size, msg_index);
          else
            ok(size == 34, "Expected that size should be 34 not %d, msg_index=%d\n",
                             size, msg_index);
        else
        if (msg_index ==22)
            ok(size == 8 || size == 9, "Expected that size should be 8 or 9 not %d, msg_index=%d\n",
                             size, msg_index);
        else
          if (msg_index == 5)
          todo_wine
            ok(size == 17, "Expected that size should be 17 not %d, msg_index=%d\n",
                             size, msg_index);
          else
            ok(size == 17, "Expected that size should be 17 not %d, msg_index=%d\n",
                             size, msg_index);
        ok((buffer = HeapAlloc(GetProcessHeap(), 0, size)) != NULL, "should not be null\n");
        rsize = DdeGetData(hdata, buffer, size, 0);
        if (msg_index == 10 || msg_index == 11 || msg_index == 16 || msg_index ==17)
        {
            ok(rsize == size, "Incorrect size returned, expected %d got %d, msg_index=%d\n",
                             size, rsize, msg_index);
          if (msg_index == 10 || msg_index == 16)
          todo_wine {
            ok(!lstrcmpW((WCHAR*)buffer, test_cmd_a_to_w),
                             "Expected \"Test dde command\", msg_index=%d\n",
                             msg_index);
            ok(size == 34, "Expected 34, got %d, msg_index=%d\n", size, msg_index);
          } else
          {
            ok(!lstrcmpW((WCHAR*)buffer, test_cmd_w_to_w),
                             "Expected \"test dde command\", msg_index=%d\n",
                             msg_index);
            ok(size == 34, "Expected 34, got %d, msg_index=%d\n", size, msg_index);
          }
        }else if (msg_index == 22)
        {
            ok(rsize == size, "Incorrect size returned, expected %d got %d, msg_index=%d\n",
                             size, rsize, msg_index);
        } else
        {
            ok(rsize == size, "Incorrect size returned, expected %d got %d, msg_index=%d\n",
                             size, rsize, msg_index);
          if (msg_index == 5)
          todo_wine {
            ok(!lstrcmpA((CHAR*)buffer, test_cmd_w_to_a), "Expected %s, got %s, msg_index=%d\n",
                             test_cmd_w_to_a, buffer, msg_index);
            ok(size == 17, "Expected size should be 17, got %d, msg_index=%d\n", size, msg_index);
          }
          else if (msg_index == 23)
          {
            ok(!lstrcmpA((CHAR*)buffer, test_cmd_w_to_a), "Expected %s, got %s, msg_index=%d\n",
                             test_cmd_w_to_a, buffer, msg_index);
            ok(size == 17, "Expected size should be 17, got %d, msg_index=%d\n", size, msg_index);
          }
            else
            {
              ok(!lstrcmpA((CHAR*)buffer, test_cmd_a_to_a), "Expected %s, got %s, msg_index=%d\n",
                               test_cmd_a_to_a, buffer, msg_index);
              ok(size == 17, "Expected size should be 17, got %d, msg_index=%d\n", size, msg_index);
            }

        }

        return (HDDEDATA) DDE_FACK;
    }
    case XTYP_DISCONNECT:
        return (HDDEDATA) TRUE;

    default:
        ok(FALSE, "Unhandled msg: %08x, msg_index=%d\n", uType, msg_index);
    }

    return NULL;
}

static HDDEDATA CALLBACK client_end_to_end_callback(UINT uType, UINT uFmt, HCONV hconv,
                                               HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    switch (uType)
    {
    case XTYP_DISCONNECT:
        return (HDDEDATA) TRUE;

    default:
        ok(FALSE, "Unhandled msg: %08x\n", uType);
    }

    return NULL;
}

static void test_end_to_end_client(BOOL type_a)
{
    DWORD  ret, err;
    DWORD client_pid = 0;
    HSZ server, topic;
    HCONV hconv;
    HDDEDATA hdata;
    static char test_cmd[] = "Test dde command";
    static WCHAR test_cmd_w[] = {'t','e','s','t',' ','d','d','e',' ','c','o','m','m','a','n','d',0};
    static char test_service[] = "TestDDEService";
    static WCHAR test_service_w[] = {'T','e','s','t','D','D','E','S','e','r','v','i','c','e',0};
    static char test_topic[] = "TestDDETopic";
    static WCHAR test_topic_w[] = {'T','e','s','t','D','D','E','T','o','p','i','c',0};

    trace("Start end to end client %s\n", type_a ? "ASCII" : "UNICODE");

    if (type_a)
        ret = DdeInitializeA(&client_pid, client_end_to_end_callback, APPCMD_CLIENTONLY, 0);
    else
        ret = DdeInitializeW(&client_pid, client_end_to_end_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %x\n", ret);

    if (type_a)
    {
        server = DdeCreateStringHandleA(client_pid, test_service, CP_WINANSI);
        topic = DdeCreateStringHandleA(client_pid, test_topic, CP_WINANSI);
    }
    else {
        server = DdeCreateStringHandleW(client_pid, test_service_w, CP_WINUNICODE);
        topic = DdeCreateStringHandleW(client_pid, test_topic_w, CP_WINUNICODE);
    }

    DdeGetLastError(client_pid);
    hconv = DdeConnect(client_pid, server, topic, NULL);
    ok(hconv != NULL, "Expected non-NULL conversation\n");
    ret = DdeGetLastError(client_pid);
    ok(ret == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %x\n", ret);
    DdeFreeStringHandle(client_pid, server);

    /* Test both A and W data being passed to DdeClientTransaction */
    hdata = DdeClientTransaction((LPBYTE)test_cmd, strlen(test_cmd) + 1,
            hconv, (HSZ)0xdead, 0xbeef, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != NULL, "DdeClientTransaction failed\n");
    ok(ret == DDE_FACK, "wrong status code %x\n", ret);
    err = DdeGetLastError(client_pid);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    hdata = DdeClientTransaction((LPBYTE)test_cmd_w, lstrlenW(test_cmd_w) * sizeof(WCHAR) + 2,
            hconv, (HSZ)0xdead, 0xbeef, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != NULL, "DdeClientTransaction failed\n");
    ok(ret == DDE_FACK, "wrong status code %x\n", ret);
    err = DdeGetLastError(client_pid);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %x\n", err);

    DdeFreeStringHandle(client_pid, topic);
    ret = DdeDisconnect(hconv);
    ok(ret == TRUE, "Expected TRUE, got %x\n", ret);

    ret = DdeUninitialize(client_pid);
    ok(ret == TRUE, "Expected TRUE, got %x\n", ret);

}

static void test_end_to_end_server(HANDLE hproc, HANDLE hthread, BOOL type_a)
{
    MSG msg;
    HSZ server;
    BOOL ret;
    DWORD res;
    HDDEDATA hdata;
    static CHAR test_service[] = "TestDDEService";

    trace("start end to end server %s\n", type_a ? "ASCII" : "UNICODE");
    server_pid = 0;

    if (type_a)
        res = DdeInitializeA(&server_pid, server_end_to_end_callback, APPCLASS_STANDARD, 0);
    else
        res = DdeInitializeW(&server_pid, server_end_to_end_callback, APPCLASS_STANDARD, 0);
    ok(res == DMLERR_NO_ERROR, "Expected DMLERR_NO_ERROR, got %d\n", res);

    server = DdeCreateStringHandleA(server_pid, test_service, CP_WINANSI);
    ok(server != NULL, "Expected non-NULL string handle\n");

    hdata = DdeNameService(server_pid, server, 0, DNS_REGISTER);
    ok(hdata == (HDDEDATA)TRUE, "Expected TRUE, got %p\n", hdata);
    ResumeThread( hthread );


    while (MsgWaitForMultipleObjects( 1, &hproc, FALSE, INFINITE, QS_ALLINPUT ) != 0)
    {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }

    ret = DdeUninitialize(server_pid);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    GetExitCodeProcess( hproc, &res );
    ok( !res, "client failed with %u error(s)\n", res );
}

START_TEST(dde)
{
    int argc;
    char **argv;
    char buffer[MAX_PATH];
    STARTUPINFO startup;
    PROCESS_INFORMATION proc;

    argc = winetest_get_mainargs(&argv);
    if (argc == 3)
    {
        if (!lstrcmpA(argv[2], "ddeml"))
            test_ddeml_client();
        else if (!lstrcmpA(argv[2], "msg"))
            test_msg_client();
        else if (!lstrcmpA(argv[2], "enda"))
            test_end_to_end_client(TRUE);
        else if (!lstrcmpA(argv[2], "endw"))
            test_end_to_end_client(FALSE);

        return;
    }

    test_initialisation();

    ZeroMemory(&startup, sizeof(STARTUPINFO));
    sprintf(buffer, "%s dde ddeml", argv[0]);
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   CREATE_SUSPENDED, NULL, NULL, &startup, &proc);

    test_msg_server(proc.hProcess, proc.hThread);

    sprintf(buffer, "%s dde msg", argv[0]);
    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   0, NULL, NULL, &startup, &proc);

    test_ddeml_server(proc.hProcess);

    /* Test the combinations of A and W interfaces with A and W data
       end to end to ensure that data conversions are accurate */
    sprintf(buffer, "%s dde enda", argv[0]);
    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   CREATE_SUSPENDED, NULL, NULL, &startup, &proc);

    test_end_to_end_server(proc.hProcess, proc.hThread, TRUE);

    sprintf(buffer, "%s dde endw", argv[0]);
    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   CREATE_SUSPENDED, NULL, NULL, &startup, &proc);

    test_end_to_end_server(proc.hProcess, proc.hThread, FALSE);

    sprintf(buffer, "%s dde enda", argv[0]);
    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   CREATE_SUSPENDED, NULL, NULL, &startup, &proc);

    test_end_to_end_server(proc.hProcess, proc.hThread, FALSE);

    sprintf(buffer, "%s dde endw", argv[0]);
    CreateProcessA(NULL, buffer, NULL, NULL, FALSE,
                   CREATE_SUSPENDED, NULL, NULL, &startup, &proc);

    test_end_to_end_server(proc.hProcess, proc.hThread, TRUE);

    test_dde_aw_transaction();

    test_DdeCreateDataHandle();
    test_DdeCreateStringHandle();
    test_FreeDDElParam();
    test_PackDDElParam();
    test_UnpackDDElParam();
}
