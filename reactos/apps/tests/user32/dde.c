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
#include "winbase.h"
#include "winuser.h"
#include "ddeml.h"
#include "winerror.h"

static HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv,
                                     HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
                                     ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    return 0;
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

START_TEST(dde)
{
    DWORD dde_inst, ret;

    dde_inst = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = DdeInitializeW(&dde_inst, DdeCallback, APPCMD_CLIENTONLY, 0);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("Skipping the DDE test on a Win9x platform\n");
        return;
    }

    ok(ret == DMLERR_INVALIDPARAMETER, "DdeInitializeW should fail, but got %04lx instead\n", ret);
    ok(DdeGetLastError(dde_inst) == DMLERR_INVALIDPARAMETER, "expected DMLERR_INVALIDPARAMETER\n");

    dde_inst = 0;
    ret = DdeInitializeW(&dde_inst, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok(ret == DMLERR_NO_ERROR, "DdeInitializeW failed with error %04lx (%08x)\n",
       ret, DdeGetLastError(dde_inst));

    test_DdeCreateStringHandleW(dde_inst, 0);
    test_DdeCreateStringHandleW(dde_inst, CP_WINUNICODE);
    test_DdeCreateStringHandleW(dde_inst, CP_WINANSI);

    ok(DdeUninitialize(dde_inst), "DdeUninitialize failed\n");
}
