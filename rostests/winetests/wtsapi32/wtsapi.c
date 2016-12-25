/*
 * Copyright 2014 Stefan Leichter
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wine/winternl.h>
#include <wtsapi32.h>

#include "wine/test.h"

static void test_WTSEnumerateProcessesW(void)
{
    BOOL found = FALSE, ret;
    DWORD count, i;
    PWTS_PROCESS_INFOW info;
    WCHAR *pname, nameW[MAX_PATH];

    GetModuleFileNameW(NULL, nameW, MAX_PATH);
    for (pname = nameW + lstrlenW(nameW); pname > nameW; pname--)
    {
        if(*pname == '/' || *pname == '\\')
        {
            pname++;
            break;
        }
    }

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 1, 1, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    if (info) WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 0, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    if (info) WTSFreeMemory(info);

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 2, &info, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    if (info) WTSFreeMemory(info);

    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, NULL, &count);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());

    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, NULL);
    ok(!ret, "expected WTSEnumerateProcessesW to fail\n");
    ok(GetLastError()== ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got: %d\n", GetLastError());
    if (info) WTSFreeMemory(info);

    count = 0;
    info = NULL;
    SetLastError(0xdeadbeef);
    ret = WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &info, &count);
    ok(ret || broken(!ret), /* fails on Win2K with error ERROR_APP_WRONG_OS */
        "expected WTSEnumerateProcessesW to succeed; failed with %d\n", GetLastError());
    for(i = 0; ret && i < count; i++)
    {
        found = found || !lstrcmpW(pname, info[i].pProcessName);
    }
    ok(found || broken(!ret), "process name %s not found\n", wine_dbgstr_w(pname));
    if (info) WTSFreeMemory(info);
}

START_TEST (wtsapi)
{
    test_WTSEnumerateProcessesW();
}
