/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for RTL_DEBUG_INFORMATION
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

#ifndef _WIN64
C_ASSERT(sizeof(RTL_DEBUG_INFORMATION) == 0x68);
#endif

static void Test_Buffersizes()
{
    PRTL_DEBUG_INFORMATION Buffer;
    NTSTATUS Status;

    Buffer = RtlCreateQueryDebugBuffer(0, FALSE);
    ok(Buffer != NULL, "Unable to create default buffer\n");
    if (!Buffer)
        return;

    ok_ptr(Buffer->ViewBaseClient, Buffer);
    ok_hex(Buffer->Flags, 0);
    ok(Buffer->OffsetFree == sizeof(*Buffer) || Buffer->OffsetFree == 0x60, "Expected %u or %u, got %lu\n", sizeof(*Buffer), 0x60, Buffer->OffsetFree);
    ok_hex(Buffer->CommitSize, 0x1000);
    ok_hex(Buffer->ViewSize, 0x400000);

    Status = RtlDestroyQueryDebugBuffer(Buffer);
    ok_hex(Status, STATUS_SUCCESS);

    Buffer = RtlCreateQueryDebugBuffer(1, FALSE);
    ok(Buffer != NULL, "Unable to create default buffer\n");
    if (!Buffer)
        return;

    ok_ptr(Buffer->ViewBaseClient, Buffer);
    ok_hex(Buffer->Flags, 0);
    ok(Buffer->OffsetFree == sizeof(*Buffer) || Buffer->OffsetFree == 0x60, "Expected %u or %u, got %lu\n", sizeof(*Buffer), 0x60, Buffer->OffsetFree);
    ok_hex(Buffer->CommitSize, 0x1000);
    ok_hex(Buffer->ViewSize, 0x1000);

    Status = RtlDestroyQueryDebugBuffer(Buffer);
    ok_hex(Status, STATUS_SUCCESS);

    Buffer = RtlCreateQueryDebugBuffer(0x1000, FALSE);
    ok(Buffer != NULL, "Unable to create default buffer\n");
    if (!Buffer)
        return;

    ok_ptr(Buffer->ViewBaseClient, Buffer);
    ok_hex(Buffer->Flags, 0);
    ok(Buffer->OffsetFree == sizeof(*Buffer) || Buffer->OffsetFree == 0x60, "Expected %u or %u, got %lu\n", sizeof(*Buffer), 0x60, Buffer->OffsetFree);
    ok_hex(Buffer->CommitSize, 0x1000);
    ok_hex(Buffer->ViewSize, 0x1000);

    Status = RtlDestroyQueryDebugBuffer(Buffer);
    ok_hex(Status, STATUS_SUCCESS);

    Buffer = RtlCreateQueryDebugBuffer(0x1001, FALSE);
    ok(Buffer != NULL, "Unable to create default buffer\n");
    if (!Buffer)
        return;

    ok_ptr(Buffer->ViewBaseClient, Buffer);
    ok_hex(Buffer->Flags, 0);
    ok(Buffer->OffsetFree == sizeof(*Buffer) || Buffer->OffsetFree == 0x60, "Expected %u or %u, got %lu\n", sizeof(*Buffer), 0x60, Buffer->OffsetFree);
    ok_hex(Buffer->CommitSize, 0x1000);
    ok_hex(Buffer->ViewSize, 0x2000);

    Status = RtlDestroyQueryDebugBuffer(Buffer);
    ok_hex(Status, STATUS_SUCCESS);

    Buffer = RtlCreateQueryDebugBuffer(0x7fffffff, FALSE);
    ok(Buffer == NULL, "Got a valid thing?\n");
    if (Buffer)
    {
        Status = RtlDestroyQueryDebugBuffer(Buffer);
        ok_hex(Status, STATUS_SUCCESS);
    }
}


static void Test_ProcessModules(void)
{
    PRTL_DEBUG_INFORMATION Buffer;
    NTSTATUS Status;
    ULONG RequiredSize = 0;
    PRTL_PROCESS_MODULES ExpectedModules;

    Buffer = RtlCreateQueryDebugBuffer(0, FALSE);
    ok(Buffer != NULL, "Unable to create default buffer\n");
    if (!Buffer)
        return;

    Status = LdrQueryProcessModuleInformation(NULL, 0, &RequiredSize);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    RequiredSize;
    ExpectedModules = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RequiredSize);

    Status = LdrQueryProcessModuleInformation(ExpectedModules, RequiredSize, &RequiredSize);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {

        Status = RtlQueryProcessDebugInformation(GetCurrentProcessId(), RTL_DEBUG_QUERY_MODULES, Buffer);
        ok_hex(Status, STATUS_SUCCESS);
        if (SUCCEEDED(Status))
        {
            ok(!memcmp(ExpectedModules, Buffer->Modules, RequiredSize), "Unexpected difference!\n");
        }
    }
    if (ExpectedModules)
        HeapFree(GetProcessHeap(), 0, ExpectedModules);

    Status = RtlDestroyQueryDebugBuffer(Buffer);
    ok_hex(Status, STATUS_SUCCESS);
}


START_TEST(RtlDebugInformation)
{
    Test_Buffersizes();
    Test_ProcessModules();
}
