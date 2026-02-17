/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtQueryInformationThread API
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"
#include <internal/ps_i.h>

static
void
Test_ThreadBasicInformationClass(void)
{
    NTSTATUS Status;
    PTHREAD_BASIC_INFORMATION ThreadInfoBasic;
    ULONG ReturnedLength;

    ThreadInfoBasic = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THREAD_BASIC_INFORMATION));
    if (!ThreadInfoBasic)
    {
        skip("Failed to allocate memory for THREAD_BASIC_INFORMATION!\n");
        return;
    }

    /* Everything is NULL */
    Status = NtQueryInformationThread(NULL,
                                      ThreadBasicInformation,
                                      NULL,
                                      0,
                                      NULL);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Don't give a valid thread handle */
    Status = NtQueryInformationThread(NULL,
                                      ThreadBasicInformation,
                                      ThreadInfoBasic,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* The information length is incorrect */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      ThreadInfoBasic,
                                      0,
                                      NULL);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Don't query anything from the function */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      NULL,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* The buffer is misaligned and length information is wrong */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      (PVOID)1,
                                      0,
                                      NULL);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The buffer is misaligned */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      (PVOID)1,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* The buffer is misaligned, try with an alignment size of 2 */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      (PVOID)2,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      NULL);
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* Query the basic information we need from the thread */
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadBasicInformation,
                                      ThreadInfoBasic,
                                      sizeof(THREAD_BASIC_INFORMATION),
                                      &ReturnedLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok(ReturnedLength != 0, "The size of the buffer pointed by ThreadInformation shouldn't be 0!\n");

    /* Output the thread basic information details */
    trace("ReturnedLength = %lu\n", ReturnedLength);
    trace("ThreadInfoBasic->ExitStatus = 0x%08lx\n", ThreadInfoBasic->ExitStatus);
    trace("ThreadInfoBasic->TebBaseAddress = %p\n", ThreadInfoBasic->TebBaseAddress);
    trace("ThreadInfoBasic->ClientId.UniqueProcess = %p\n", ThreadInfoBasic->ClientId.UniqueProcess);
    trace("ThreadInfoBasic->ClientId.UniqueThread = %p\n", ThreadInfoBasic->ClientId.UniqueThread);
    trace("ThreadInfoBasic->AffinityMask = %lu\n", ThreadInfoBasic->AffinityMask);
    trace("ThreadInfoBasic->Priority = %li\n", ThreadInfoBasic->Priority);
    trace("ThreadInfoBasic->BasePriority = %li\n", ThreadInfoBasic->BasePriority);

    HeapFree(GetProcessHeap(), 0, ThreadInfoBasic);
}

static ULONG
NTAPI
DummyThread(
    _In_ PVOID Parameter)
{
    HANDLE hWaitEvent = (HANDLE)Parameter;
    NTSTATUS Status;

    /* Indefinitely wait for the kill signal */
    Status = NtWaitForSingleObject(hWaitEvent, FALSE, NULL);
    ASSERT(NT_SUCCESS(Status));

    NtTerminateThread(NtCurrentThread(), Status);
    return Status;
}

static
void
Test_ThreadHideFromDebuggerClass(void)
{
    NTSTATUS Status;
    HANDLE hWaitEvent = NULL, hThread = NULL;
    BOOLEAN IsThreadHidden;
    ULONG UlongData;
    BOOL IsWow64 = FALSE;

    if (GetNTVersion() < _WIN32_WINNT_VISTA)
    {
        skip("Skipping test as NtQueryInformationThread(ThreadHideFromDebugger) isn't supported prior to Vista\n");
        return;
    }
    IsWow64Process(GetCurrentProcess(), &IsWow64);

    Status = NtCreateEvent(&hWaitEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(hWaitEvent != NULL, "Expected not NULL, got NULL\n");
    if (!NT_SUCCESS(Status))
    {
        skip("Could not create the stop event! (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Create the dummy thread and wait for it to start up */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 DummyThread,
                                 (PVOID)hWaitEvent,
                                 &hThread,
                                 NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(hThread != NULL, "Expected not NULL, got NULL\n");
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create the dummy thread (Status 0x%08lx)\n", Status);
        goto Quit;
    }


    /* Verify that the thread is debuggable by default */
    IsThreadHidden = 0xCC;
    Status = NtQueryInformationThread(hThread,
                                      ThreadHideFromDebugger,
                                      &IsThreadHidden,
                                      sizeof(IsThreadHidden),
                                      NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_bool_false(IsThreadHidden, "IsThreadHidden is");

    /* Make the thread hidden from the debugger -- The wrong way (1) */
    IsThreadHidden = TRUE;
    Status = NtSetInformationThread(hThread,
                                    ThreadHideFromDebugger,
                                    &IsThreadHidden,
                                    sizeof(IsThreadHidden));
    if (IsWow64)
        ok_ntstatus(Status, STATUS_DATATYPE_MISALIGNMENT);
    else
        ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The thread is still debuggable */
    IsThreadHidden = 0xCC;
    Status = NtQueryInformationThread(hThread,
                                      ThreadHideFromDebugger,
                                      &IsThreadHidden,
                                      sizeof(IsThreadHidden),
                                      NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_bool_false(IsThreadHidden, "IsThreadHidden is");

    /* Make the thread hidden from the debugger -- The wrong way (2) */
    UlongData = TRUE;
    Status = NtSetInformationThread(hThread,
                                    ThreadHideFromDebugger,
                                    &UlongData,
                                    sizeof(UlongData));
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The thread is still debuggable */
    IsThreadHidden = 0xCC;
    Status = NtQueryInformationThread(hThread,
                                      ThreadHideFromDebugger,
                                      &IsThreadHidden,
                                      sizeof(IsThreadHidden),
                                      NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_bool_false(IsThreadHidden, "IsThreadHidden is");

    /* Make the thread hidden from the debugger -- The wrong way (3) */
    UlongData = TRUE;
    Status = NtSetInformationThread(hThread,
                                    ThreadHideFromDebugger,
                                    &UlongData,
                                    sizeof(BOOLEAN));
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The thread is still debuggable */
    IsThreadHidden = 0xCC;
    Status = NtQueryInformationThread(hThread,
                                      ThreadHideFromDebugger,
                                      &IsThreadHidden,
                                      sizeof(IsThreadHidden),
                                      NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_bool_false(IsThreadHidden, "IsThreadHidden is");

    /* Make the thread hidden from the debugger -- This way works! */
    Status = NtSetInformationThread(hThread,
                                    ThreadHideFromDebugger,
                                    NULL, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify that the thread is now hidden from the debugger */
    IsThreadHidden = 0xCC;
    Status = NtQueryInformationThread(hThread,
                                      ThreadHideFromDebugger,
                                      &IsThreadHidden,
                                      sizeof(IsThreadHidden),
                                      NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_bool_true(IsThreadHidden, "IsThreadHidden is");


    /* Send the kill signal and wait for the thread to terminate */
    NtSetEvent(hWaitEvent, NULL);
    NtWaitForSingleObject(hThread, FALSE, NULL);
    /* Close the thread */
    NtTerminateThread(hThread, STATUS_SUCCESS);
    NtClose(hThread);
Quit:
    /* Close the event */
    if (hWaitEvent)
        NtClose(hWaitEvent);
}

static
void
Test_ThreadQueryAlignmentProbe(void)
{
    ULONG InfoClass;

    /* Iterate over the process info classes and begin the tests */
    for (InfoClass = 0; InfoClass < _countof(PsThreadInfoClass); InfoClass++)
    {
        /* The buffer is misaligned */
        QuerySetThreadValidator(QUERY,
                                InfoClass,
                                (PVOID)(ULONG_PTR)1,
                                PsThreadInfoClass[InfoClass].RequiredSizeQUERY,
                                STATUS_DATATYPE_MISALIGNMENT);

        /* We query an invalid buffer address */
        QuerySetThreadValidator(QUERY,
                                InfoClass,
                                (PVOID)(ULONG_PTR)PsThreadInfoClass[InfoClass].AlignmentQUERY,
                                PsThreadInfoClass[InfoClass].RequiredSizeQUERY,
                                STATUS_ACCESS_VIOLATION);

        /* The information length is wrong */
        QuerySetThreadValidator(QUERY,
                                InfoClass,
                                (PVOID)(ULONG_PTR)PsThreadInfoClass[InfoClass].AlignmentQUERY,
                                PsThreadInfoClass[InfoClass].RequiredSizeQUERY - 1,
                                STATUS_INFO_LENGTH_MISMATCH);
    }
}

START_TEST(NtQueryInformationThread)
{
    Test_ThreadBasicInformationClass();
    Test_ThreadHideFromDebuggerClass();
    Test_ThreadQueryAlignmentProbe();
}
