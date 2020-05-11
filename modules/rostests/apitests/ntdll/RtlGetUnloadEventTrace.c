/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RtlGetUnloadEventTrace
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

PRTL_UNLOAD_EVENT_TRACE
NTAPI
RtlGetUnloadEventTrace(VOID);

#ifndef _WIN64
C_ASSERT(sizeof(RTL_UNLOAD_EVENT_TRACE) == 84);
C_ASSERT(sizeof(RTL_UNLOAD_EVENT_TRACE) * RTL_UNLOAD_EVENT_TRACE_NUMBER == 0x540);
#endif

static void Test_Dump()
{
    PRTL_UNLOAD_EVENT_TRACE TraceHead, Trace;
    UINT n;

    TraceHead = RtlGetUnloadEventTrace();
    for (n = 0; n < RTL_UNLOAD_EVENT_TRACE_NUMBER; ++n)
    {
        ULONG ExpectSequence = n ? n : RTL_UNLOAD_EVENT_TRACE_NUMBER;

        Trace = TraceHead + n;

        ok(Trace->BaseAddress != NULL, "Got no BaseAddress for %u\n", n);
        ok(Trace->SizeOfImage != 0, "Got no SizeOfImage for %u\n", n);
        ok(Trace->Sequence == ExpectSequence,
           "Wrong Sequence: %lu instead of %lu for %u\n", Trace->Sequence, ExpectSequence, n);
        ok(Trace->TimeDateStamp != 0, "Got no TimeDateStamp for %u\n", n);
        ok(Trace->CheckSum != 0, "Got no CheckSum for %u\n", n);
        ok(!wcscmp(Trace->ImageName, L"GetUName.dLl"), "Wrong ImageName for %u: %S\n", n, Trace->ImageName);
    }
}

#define TESTDLL "GetUName.dLl"
static void Test_LoadUnload()
{
    HMODULE mod;
    static char Buffer[MAX_PATH] = {0};

    mod = GetModuleHandleA(TESTDLL);
    ok(mod == NULL, "ERROR, %s already loaded\n", TESTDLL);

    mod = LoadLibraryA(Buffer[0] ? Buffer :TESTDLL);
    ok(mod != NULL, "ERROR, %s not loaded\n", TESTDLL);

    if (!Buffer[0])
    {
        GetModuleFileNameA(mod, Buffer, _countof(Buffer));
    }
    else
    {
        Buffer[0] = '\0';
    }

    FreeLibrary(mod);

    mod = GetModuleHandleA(TESTDLL);
    ok(mod == NULL, "ERROR, %s still loaded\n", TESTDLL);
}

START_TEST(RtlGetUnloadEventTrace)
{
    int n;
    HMODULE Ignore;

    Ignore = LoadLibrary("user32.dll");

    for (n = 0; n <= RTL_UNLOAD_EVENT_TRACE_NUMBER; ++n)
    {
        trace("Num: %u\n", n);
        Test_LoadUnload();
    }
    Test_Dump();

    FreeLibrary(Ignore);
}
