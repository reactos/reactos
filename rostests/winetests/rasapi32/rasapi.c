/*
* Unit test suite for rasapi32 functions
*
* Copyright 2008 Austin English
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
#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include "ras.h"
#include "raserror.h"

static HMODULE hmodule;
static DWORD (WINAPI *pRasEnumDevicesA)(LPRASDEVINFOA, LPDWORD, LPDWORD);

#define RASAPI32_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hmodule, #func); \
    if(!p ## func) \
        trace("GetProcAddress(%s) failed\n", #func);

static void InitFunctionPtrs(void)
{
    hmodule = LoadLibraryA("rasapi32.dll");

    RASAPI32_GET_PROC(RasEnumDevicesA)
}

static void test_rasenum(void)
{
    DWORD result;
    DWORD cDevices = 0;
    DWORD bufsize = 0, cb = 0;
    LPRASDEVINFOA rasDevInfo;

    if(!pRasEnumDevicesA) {
        win_skip("Skipping RasEnumDevicesA tests, function not present\n");
        return;
    }

    /* create the return buffer */
    result = pRasEnumDevicesA(NULL, &bufsize, &cDevices);
    if(ERROR_RASMAN_CANNOT_INITIALIZE == result) {
        win_skip("RAS configuration problem\n");
        return;
    }
    if(ERROR_SUCCESS == result) {
        win_skip("RasEnumDevicesA found nothing to enumerate\n");
        return;
    }
    trace("RasEnumDevicesA: returned %d buffersize %d\n", result, bufsize);
    ok(result == ERROR_BUFFER_TOO_SMALL,
    "Expected ERROR_BUFFER_TOO_SMALL, got %08d\n", result);

    rasDevInfo = (LPRASDEVINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                          max(bufsize,sizeof(RASDEVINFOA)));
    if(!rasDevInfo) {
        win_skip("failed to allocate buffer for RasEnumDevicesA tests\n");
        return;
    }

    /* test first parameter */
    cb = bufsize;
    result = pRasEnumDevicesA(NULL, &cb, &cDevices);
    ok(result == ERROR_BUFFER_TOO_SMALL ||
    result == ERROR_INVALID_USER_BUFFER, /* win98 */
    "Expected ERROR_BUFFER_TOO_SMALL, got %08d\n", result);

    rasDevInfo[0].dwSize = 0;
    cb = bufsize;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    todo_wine
    ok(result == ERROR_INVALID_SIZE ||
    result == ERROR_INVALID_USER_BUFFER, /* win98 */
    "Expected ERROR_INVALID_SIZE, got %08d\n", result);

    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA) -1;
    cb = bufsize;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    todo_wine
    ok(result == ERROR_INVALID_SIZE ||
    result == ERROR_INVALID_USER_BUFFER, /* win98 */
    "Expected ERROR_INVALID_SIZE, got %08d\n", result);

    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA) +1;
    cb = bufsize;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    todo_wine
    ok(result == ERROR_INVALID_SIZE ||
    result == ERROR_INVALID_USER_BUFFER, /* win98 */
    "Expected ERROR_INVALID_SIZE, got %08d\n", result);

    /* test second parameter */
    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA);
    result = pRasEnumDevicesA(rasDevInfo, NULL, &cDevices);
    ok(result == ERROR_INVALID_PARAMETER,
    "Expected ERROR_INVALID_PARAMETER, got %08d\n", result);

    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA);
    cb = 0;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    ok(result == ERROR_BUFFER_TOO_SMALL ||
    result == ERROR_INVALID_SIZE, /* vista, 2k8 */
    "Expected ERROR_BUFFER_TOO_SMALL/ERROR_INVALID_SIZE, got %08d\n", result);

    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA);
    cb = bufsize -1;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    ok(result == ERROR_BUFFER_TOO_SMALL,
    "Expected ERROR_BUFFER_TOO_SMALL, got %08d\n", result);

    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA);
    cb = bufsize +1;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    ok(result == ERROR_SUCCESS,
    "Expected ERROR_SUCCESS, got %08d\n", result);

    /* test third parameter */
    rasDevInfo[0].dwSize = sizeof(RASDEVINFOA);
    cb = bufsize;
    result = pRasEnumDevicesA(rasDevInfo, &cb, NULL);
    ok(result == ERROR_INVALID_PARAMETER,
    "Expected ERROR_INVALID_PARAMETER, got %08d\n", result);

    /* test combinations of invalid parameters */
    result = pRasEnumDevicesA(NULL, NULL, &cDevices);
    ok(result == ERROR_INVALID_PARAMETER,
    "Expected ERROR_INVALID_PARAMETER, got %08d\n", result);

    result = pRasEnumDevicesA(NULL, &cb, NULL);
    ok(result == ERROR_INVALID_PARAMETER ||
    result == ERROR_INVALID_USER_BUFFER, /* win98 */
    "Expected ERROR_INVALID_PARAMETER, got %08d\n", result);

    cb = 0;
    rasDevInfo[0].dwSize = 0;
    result = pRasEnumDevicesA(rasDevInfo, &cb, &cDevices);
    todo_wine
    ok(result == ERROR_INVALID_SIZE ||
    broken(result == ERROR_BUFFER_TOO_SMALL), /* win98 */
    "Expected ERROR_INVALID_SIZE, got %08d\n", result);

    HeapFree(GetProcessHeap(), 0, rasDevInfo);
}

START_TEST(rasapi)
{
    InitFunctionPtrs();

    test_rasenum();

    FreeLibrary(hmodule);
}
