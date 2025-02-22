/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RtlQueryProcessBackTraceInformation & RtlLogStackBackTrace
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "loadconfig.h"


// This test serves 2 purposes:
// 1. It tests RtlQueryProcessBackTraceInformation & RtlLogStackBackTrace
// 2. It tests the correct activation of IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG (see also common.c)

NTSTATUS NTAPI RtlQueryProcessBackTraceInformation(IN OUT PRTL_DEBUG_INFORMATION Buffer);


// Seems that this struct is wrong in our sdk?
typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION_32
{
    PVOID SymbolicBackTrace;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[32];
} RTL_PROCESS_BACKTRACE_INFORMATION_32, *PRTL_PROCESS_BACKTRACE_INFORMATION_32;


static PVOID g_Call_Address;
static PVOID g_PreviousReturnAddress;

__declspec(noinline)
PVOID GetFunctionAddress()
{
    return _ReturnAddress();
}

__declspec(noinline)
USHORT Call_Backtrace_2()
{
    USHORT Index;
    Index = RtlLogStackBackTrace();
    ok(Index != 0, "RtlLogStackBackTrace failed\n");
    return Index;
}


__declspec(noinline)
USHORT Call_Backtrace_1()
{
    USHORT Index;
    // It seems that the function calling RtlLogStackBackTrace is skipped,
    // so we wrap it in a deeper nested function to be able to check it
    g_Call_Address = GetFunctionAddress();
    g_PreviousReturnAddress = _ReturnAddress();
    Index = Call_Backtrace_2();
    return Index;
}

static void Check_Stacktrace(PVOID* BackTrace, USHORT Depth)
{
    ok(Depth > 2, "Unexpected Depth: %lu\n", (ULONG)Depth);
    if (Depth > 2)
    {
        ULONG_PTR EndAddress = ((ULONG_PTR)g_Call_Address + 0x20);
        ok(BackTrace[0] >= g_Call_Address && (ULONG_PTR)BackTrace[0] < EndAddress,
           "Unexpected return: %p, expected between %p and %p\n", BackTrace[0], g_Call_Address, (PVOID)EndAddress);
        ok(BackTrace[1] == g_PreviousReturnAddress, "Unexpected return: %p, expected: %p\n",
           BackTrace[1], g_PreviousReturnAddress);
    }
}

static void test_QueryBacktrace(PRTL_DEBUG_INFORMATION Buffer1, PRTL_DEBUG_INFORMATION Buffer2)
{
    NTSTATUS Status;
    USHORT StackTrace;
    PRTL_PROCESS_BACKTRACES Backtraces;
    ULONG OldNumberOfBackTraces, n;
    PRTL_PROCESS_BACKTRACE_INFORMATION_32 FirstBacktrace;
    int found = 0;

    Status = RtlQueryProcessBackTraceInformation(Buffer1);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status != STATUS_SUCCESS)
        return;

    Backtraces = Buffer1->BackTraces;
    ok(Backtraces != NULL, "No BackTraces\n");
    if (!Backtraces)
        return;

    OldNumberOfBackTraces = Backtraces->NumberOfBackTraces;
    // Capture a stacktrace
    StackTrace = Call_Backtrace_1();

    // Show that the old debugbuffer is not changed
    ok(OldNumberOfBackTraces == Backtraces->NumberOfBackTraces, "Debug buffer changed! (%lu => %lu)\n",
       OldNumberOfBackTraces, Backtraces->NumberOfBackTraces);

    // Ask for a new snapshot
    Status = RtlQueryProcessBackTraceInformation(Buffer2);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status != STATUS_SUCCESS)
        return;

    Backtraces = Buffer2->BackTraces;
    ok(Backtraces != NULL, "No BackTraces\n");
    if (!Backtraces)
        return;

    ok(OldNumberOfBackTraces+1 == Backtraces->NumberOfBackTraces, "Stacktrace not added! (%lu => %lu)\n",
       OldNumberOfBackTraces, Backtraces->NumberOfBackTraces);

    FirstBacktrace = (PRTL_PROCESS_BACKTRACE_INFORMATION_32)&Backtraces->BackTraces[0];
    trace("NumberOfBackTraces=%lu\n", Backtraces->NumberOfBackTraces);
    for (n = 0; n < Backtraces->NumberOfBackTraces; ++n)
    {
        PRTL_PROCESS_BACKTRACE_INFORMATION_32 Info = FirstBacktrace + n;
#if 0
        USHORT j;

        trace("BackTraces[%02lu]->SymbolicBackTrace = %p\n", n, Info->SymbolicBackTrace);
        trace("BackTraces[%02lu]->TraceCount = %lu\n", n, Info->TraceCount);
        trace("BackTraces[%02lu]->Index = %lu\n", n, (ULONG)Info->Index);
        trace("BackTraces[%02lu]->Depth = %lu\n", n, (ULONG)Info->Depth);
        for (j = 0; j < Info->Depth; ++j)
        {
            trace("BackTraces[%02lu]->BackTrace[%02u] = %p\n", n, j, Info->BackTrace[j]);
        }
        trace("\n");
#endif
        if (Info->Index == StackTrace)
        {
            found = 1;
            Check_Stacktrace(Info->BackTrace, Info->Depth);
        }
    }
    ok(found, "Stacktrace not found :(\n");
}


START_TEST(stacktrace)
{
    PRTL_DEBUG_INFORMATION Buffer1, Buffer2;

    if (!check_loadconfig())
        return;

    skip("QueryBacktrace not implemented yet\n");
    return;

    Buffer1 = RtlCreateQueryDebugBuffer(0, FALSE);
    ok(Buffer1 != NULL, "Failed!\n");
    if (Buffer1)
    {
        Buffer2 = RtlCreateQueryDebugBuffer(0, FALSE);
        ok(Buffer2 != NULL, "Failed!\n");
        if (Buffer2)
        {
            test_QueryBacktrace(Buffer1, Buffer2);
            RtlDestroyQueryDebugBuffer(Buffer2);
        }

        RtlDestroyQueryDebugBuffer(Buffer1);
    }
}
