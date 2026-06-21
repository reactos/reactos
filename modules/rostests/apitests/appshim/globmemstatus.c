/*
 * PROJECT:     appshim_apitest
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for GlobalMemoryStatus2GB shim
 * COPYRIGHT:   Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include "wine/test.h"

#include "appshim_apitest.h"

static HMODULE g_hShimDll;
static tGETHOOKAPIS pGetHookAPIs;

typedef VOID(NTAPI *GLOBALMEMORYSTATUSPROC)(LPMEMORYSTATUS lpBuffer);


VOID NTAPI
my_GlobalMemoryStatus_Low(LPMEMORYSTATUS lpBuffer)
{
    memset(lpBuffer, 0, sizeof(MEMORYSTATUS));
}

VOID NTAPI
my_GlobalMemoryStatus_High(LPMEMORYSTATUS lpBuffer)
{
    memset(lpBuffer, 0xff, sizeof(MEMORYSTATUS));
}

static void
test_GlobalMemoryStatus(PHOOKAPI hook)
{
    GLOBALMEMORYSTATUSPROC proc;

    ok_str(hook->LibraryName, "KERNEL32.DLL");
    hook->OriginalFunction = my_GlobalMemoryStatus_Low;
    proc = hook->ReplacementFunction;


    MEMORYSTATUS ms;
    memset(&ms, 0x33, sizeof(ms));
    proc(&ms);

    ok_hex(ms.dwLength, 0);
    ok_hex(ms.dwMemoryLoad, 0);
    ok_hex(ms.dwTotalPhys, 0);
    ok_hex(ms.dwAvailPhys, 0);
    ok_hex(ms.dwTotalPageFile, 0);
    ok_hex(ms.dwAvailPageFile, 0);
    ok_hex(ms.dwTotalVirtual, 0);
    ok_hex(ms.dwAvailVirtual, 0);


    hook->OriginalFunction = my_GlobalMemoryStatus_High;

    memset(&ms, 0x33, sizeof(ms));
    proc(&ms);
    ok_hex(ms.dwLength, 0xffffffff);     // not touched
    ok_hex(ms.dwMemoryLoad, 0xffffffff); // not touched
    ok_hex(ms.dwTotalPhys, 0x3fffffff);
    ok_hex(ms.dwAvailPhys, 0x3fffffff);
    ok_hex(ms.dwTotalPageFile, 0x7fffffff);
    ok_hex(ms.dwAvailPageFile, 0x3fffffff);
    ok_hex(ms.dwTotalVirtual, 0xffffffff); // not touched
    ok_hex(ms.dwAvailVirtual, 0xffffffff); // not touched
}


START_TEST(globalmemorystatus)
{
    DWORD num_shims = 0, n;
    PHOOKAPI hook;

    if (!LoadShimDLL(L"aclayers.dll", &g_hShimDll, &pGetHookAPIs))
        return;

    hook = pGetHookAPIs("", L"GlobalMemoryStatus2GB", &num_shims);

    ok(hook != NULL, "Expected hook to be a valid pointer\n");
    ok(num_shims == 1, "Expected num_shims to be 1, was: %u\n", num_shims);

    if (!hook || !num_shims)
        return;

    for (n = 0; n < num_shims; ++n)
    {
        if (!_stricmp(hook[n].FunctionName, "GlobalMemoryStatus"))
            test_GlobalMemoryStatus(hook + n);
    }
}
