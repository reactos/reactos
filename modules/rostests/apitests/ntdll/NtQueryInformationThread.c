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
    Test_ThreadQueryAlignmentProbe();
}
