/*
 * Unit tests for DDE functions
 *
 * Copyright (c) 2004 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>

#include "wine/test.h"
#include "windows.h"

static const WCHAR TEST_DDE_SERVICE[] = {'T','e','s','t','D','D','E','S','e','r','v','i','c','e',0};

static const char exec_cmdA[] = "ANSI dde command";
static const WCHAR exec_cmdW[] = {'u','n','i','c','o','d','e',' ','d','d','e',' ','c','o','m','m','a','n','d',0};

static WNDPROC old_dde_client_wndproc;

LRESULT WINAPI hook_dde_client_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UINT_PTR lo, hi;

    trace("hook_dde_client_wndproc: %p %04x %08x %08lx\n", hwnd, msg, wparam, lparam);

    switch (msg)
    {
    case WM_DDE_ACK:
        UnpackDDElParam(WM_DDE_ACK, lparam, &lo, &hi);
        trace("WM_DDE_ACK: status %04x hglobal %p\n", lo, (HGLOBAL)hi);
        break;

    default:
        break;
    }
    return CallWindowProcA(old_dde_client_wndproc, hwnd, msg, wparam, lparam);
}

static LRESULT WINAPI dde_server_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    trace("dde_server_wndproc: %p %04x %08x %08lx\n", hwnd, msg, wparam, lparam);

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
        trace("%08lx => lo %04x hi %04x\n", lparam, lo, hi);

        ack.bAppReturnCode = 0;
        ack.reserved = 0;
        ack.fBusy = 0;

        cmd = GlobalLock((HGLOBAL)hi);

        if (!cmd || (lstrcmpW((LPCWSTR)cmd, exec_cmdW) && lstrcmpA(cmd, exec_cmdA)))
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

static BOOL create_dde_windows(HWND *hwnd_client, HWND *hwnd_server)
{
    WNDCLASSA wcA;
    WNDCLASSW wcW;
    static const WCHAR server_class_name[] = {'d','d','e','_','s','e','r','v','e','r','_','w','i','n','d','o','w',0};
    static const char client_class_name[] = "dde_client_window";

    memset(&wcW, 0, sizeof(wcW));
    wcW.lpfnWndProc = dde_server_wndproc;
    wcW.lpszClassName = server_class_name;
    wcW.hInstance = GetModuleHandleA(0);
    if (!RegisterClassW(&wcW)) return FALSE;

    memset(&wcA, 0, sizeof(wcA));
    wcA.lpfnWndProc = dde_client_wndproc;
    wcA.lpszClassName = client_class_name;
    wcA.hInstance = GetModuleHandleA(0);
    assert(RegisterClassA(&wcA));

    *hwnd_server = CreateWindowExW(0, server_class_name, NULL,
                                   WS_POPUP,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandleA(0), NULL);
    assert(*hwnd_server);

    *hwnd_client = CreateWindowExA(0, client_class_name, NULL,
                                   WS_POPUP,
                                   100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                   GetDesktopWindow(), 0,
                                   GetModuleHandleA(0), NULL);
    assert(*hwnd_client);

    trace("server hwnd %p, client hwnd %p\n", *hwnd_server, *hwnd_client);

    ok(IsWindowUnicode(*hwnd_server), "server has to be a unicode window\n");
    ok(!IsWindowUnicode(*hwnd_client), "client has to be an ANSI window\n");

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
    cmd_name = (type >= 0 && type <= 14) ? cmd_type[type] : "unknown";

    trace("client_dde_callback: %04x (%s) %d %p %p %p %p %08lx %08lx\n",
          uType, cmd_name, uFmt, hconv, hsz1, hsz2, hdata, dwData1, dwData2);
    return 0;
}

static void test_dde_transaction(void)
{
    HSZ hsz_server;
    DWORD dde_inst, ret, err;
    HCONV hconv;
    HWND hwnd_client, hwnd_server;
    CONVINFO info;
    HDDEDATA hdata;
    static const char test_cmd[] = "test dde command";

    /* server: unicode, client: ansi */
    if (!create_dde_windows(&hwnd_client, &hwnd_server)) return;

    dde_inst = 0;
    ret = DdeInitializeA(&dde_inst, client_dde_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04lx (%x)\n",
       ret, DdeGetLastError(dde_inst));

    hsz_server = DdeCreateStringHandleW(dde_inst, TEST_DDE_SERVICE, CP_WINUNICODE);

    hconv = DdeConnect(dde_inst, hsz_server, 0, NULL);
    ok(hconv != 0, "DdeConnect error %x\n", DdeGetLastError(dde_inst));
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %lx\n", err);

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(ret, "wrong info size %ld, DdeQueryConvInfo error %x\n", ret, DdeGetLastError(dde_inst));
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
    ok(ret == DDE_FNOTPROCESSED, "wrong status code %04lx\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NOTPROCESSED, "wrong dde error %lx\n", err);

    trace("sending ANSI client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdA, lstrlenA(exec_cmdA) + 1, hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04lx\n", ret);

    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %lx\n", err);

    trace("sending unicode client transaction command\n");
    ret = 0xdeadbeef;
    hdata = DdeClientTransaction((LPBYTE)exec_cmdW, (lstrlenW(exec_cmdW) + 1) * sizeof(WCHAR), hconv, 0, 0, XTYP_EXECUTE, 1000, &ret);
    ok(hdata != 0, "DdeClientTransaction returned %p, error %x\n", hdata, DdeGetLastError(dde_inst));
    ok(ret == DDE_FACK, "wrong status code %04lx\n", ret);
    err = DdeGetLastError(dde_inst);
    ok(err == DMLERR_NO_ERROR, "wrong dde error %lx\n", err);

    ok(DdeDisconnect(hconv), "DdeDisconnect error %x\n", DdeGetLastError(dde_inst));

    info.cb = sizeof(info);
    ret = DdeQueryConvInfo(hconv, QID_SYNC, &info);
    ok(!ret, "DdeQueryConvInfo should fail\n");
    err = DdeGetLastError(dde_inst);
todo_wine {
    ok(err == DMLERR_INVALIDPARAMETER, "wrong dde error %lx\n", err);
}

    ok(DdeFreeStringHandle(dde_inst, hsz_server), "DdeFreeStringHandle error %x\n", DdeGetLastError(dde_inst));

    /* This call hangs on win2k SP4 and XP SP1.
    DdeUninitialize(dde_inst);*/

    DestroyWindow(hwnd_client);
    DestroyWindow(hwnd_server);
}

static void test_DdeCreateStringHandleW(DWORD dde_inst, int codepage)
{
    static const WCHAR dde_string[] = {'D','D','E',' ','S','t','r','i','n','g',0};
    HSZ str_handle;
    WCHAR bufW[256];
    char buf[256];
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

    ok(DdeFreeStringHandle(dde_inst, str_handle), "DdeFreeStringHandle failed\n");
}

static void test_DdeCreateStringHandle(void)
{
    DWORD dde_inst, ret;

    dde_inst = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = DdeInitializeW(&dde_inst, client_dde_callback, APPCMD_CLIENTONLY, 0);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("Skipping the DDE test on a Win9x platform\n");
        return;
    }

    ok(ret == DMLERR_INVALIDPARAMETER, "DdeInitializeW should fail, but got %04lx instead\n", ret);
    ok(DdeGetLastError(dde_inst) == DMLERR_INVALIDPARAMETER, "expected DMLERR_INVALIDPARAMETER\n");

    dde_inst = 0;
    ret = DdeInitializeW(&dde_inst, client_dde_callback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04lx (%08x)\n",
       ret, DdeGetLastError(dde_inst));

    test_DdeCreateStringHandleW(dde_inst, 0);
    test_DdeCreateStringHandleW(dde_inst, CP_WINUNICODE);
    test_DdeCreateStringHandleW(dde_inst, CP_WINANSI);

    ok(DdeUninitialize(dde_inst), "DdeUninitialize failed\n");
}

START_TEST(dde)
{
   test_DdeCreateStringHandle();
   test_dde_transaction();
}
