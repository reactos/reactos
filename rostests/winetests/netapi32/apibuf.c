/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the network buffer function.
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

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>

static NET_API_STATUS (WINAPI *pNetApiBufferAllocate)(DWORD,LPVOID*)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferFree)(LPVOID)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferReallocate)(LPVOID,DWORD,LPVOID*)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferSize)(LPVOID,LPDWORD)=NULL;


static void run_apibuf_tests(void)
{
    VOID *p;
    DWORD dwSize;
    NET_API_STATUS res;

    /* test normal logic */
    ok(pNetApiBufferAllocate(1024, (LPVOID *)&p) == NERR_Success,
       "Reserved memory\n");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize >= 1024, "The size is correct\n");

    ok(pNetApiBufferReallocate(p, 1500, (LPVOID *) &p) == NERR_Success,
       "Reallocated\n");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize >= 1500, "The size is correct\n");

    ok(pNetApiBufferFree(p) == NERR_Success, "Freed\n");

    /* test errors handling */
    ok(pNetApiBufferFree(p) == NERR_Success, "Freed\n");

    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(pNetApiBufferSize(NULL, &dwSize) == ERROR_INVALID_PARAMETER, "Error for NULL pointer\n");

    /* border reallocate cases */
    ok(pNetApiBufferReallocate(0, 1500, (LPVOID *) &p) == NERR_Success, "Reallocate with OldBuffer = NULL failed\n");
    ok(p != NULL, "No memory got allocated\n");
    ok(pNetApiBufferAllocate(1024, (LPVOID *)&p) == NERR_Success, "Memory not reserved\n");
    ok(pNetApiBufferReallocate(p, 0, (LPVOID *) &p) == NERR_Success, "Not freed\n");
    ok(p == NULL, "Pointer not cleared\n");
    
    /* 0-length buffer */
    ok(pNetApiBufferAllocate(0, (LPVOID *)&p) == NERR_Success,
       "Reserved memory\n");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize < 0xFFFFFFFF, "The size of the 0-length buffer\n");
    ok(pNetApiBufferFree(p) == NERR_Success, "Freed\n");

    /* NULL-Pointer */
    /* NT: ERROR_INVALID_PARAMETER, lasterror is untouched) */
    SetLastError(0xdeadbeef);
    res = pNetApiBufferAllocate(0, (LPVOID *)NULL);
    ok( (res == ERROR_INVALID_PARAMETER) && (GetLastError() == 0xdeadbeef),
        "returned %d with 0x%x (expected ERROR_INVALID_PARAMETER with "
        "0xdeadbeef)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pNetApiBufferAllocate(1024, (LPVOID *)NULL);    
    ok( (res == ERROR_INVALID_PARAMETER) && (GetLastError() == 0xdeadbeef),
        "returned %d with 0x%x (expected ERROR_INVALID_PARAMETER with "
        "0xdeadbeef)\n", res, GetLastError());
}

START_TEST(apibuf)
{
    HMODULE hnetapi32=LoadLibraryA("netapi32.dll");

    pNetApiBufferAllocate=(void*)GetProcAddress(hnetapi32,"NetApiBufferAllocate");
    pNetApiBufferFree=(void*)GetProcAddress(hnetapi32,"NetApiBufferFree");
    pNetApiBufferReallocate=(void*)GetProcAddress(hnetapi32,"NetApiBufferReallocate");
    pNetApiBufferSize=(void*)GetProcAddress(hnetapi32,"NetApiBufferSize");

    if (pNetApiBufferAllocate && pNetApiBufferFree && pNetApiBufferReallocate && pNetApiBufferSize)
        run_apibuf_tests();
    else
        skip("Needed functions are not available\n");

    FreeLibrary(hnetapi32);
}
