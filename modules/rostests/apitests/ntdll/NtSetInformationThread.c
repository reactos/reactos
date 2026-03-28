/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetInformationThread API
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"
#include <internal/ps_i.h>

static
void
Test_ThreadPriorityClass(void)
{
    NTSTATUS Status;
    KPRIORITY *Priority;

    Priority = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KPRIORITY));
    if (Priority == NULL)
    {
        skip("Failed to allocate memory for thread priority variable!\n");
        return;
    }

    /* Assign a priority */
    *Priority = 11;

    /* Everything is NULL */
    Status = NtSetInformationThread(NULL,
                                    ThreadPriority,
                                    NULL,
                                    0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Give an invalid thread handle */
    Status = NtSetInformationThread(NULL,
                                    ThreadPriority,
                                    Priority,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* Don't set any priority to the thread */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    NULL,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* The information length is incorrect */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    Priority,
                                    0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The buffer is misaligned and information length is incorrect */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    (PVOID)1,
                                    0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The buffer is misaligned */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    (PVOID)1,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* The buffer is misaligned -- try with an alignment size of 2 */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    (PVOID)2,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* Set the priority to the current thread */
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    Priority,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_SUCCESS);

    /*
     * Set the priority as LOW_REALTIME_PRIORITY,
     * we don't have privileges to do so.
    */
    *Priority = LOW_REALTIME_PRIORITY;
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadPriority,
                                    Priority,
                                    sizeof(KPRIORITY));
    ok_hex(Status, STATUS_PRIVILEGE_NOT_HELD);

    HeapFree(GetProcessHeap(), 0, Priority);
}

static
void
Test_ThreadSetAlignmentProbe(void)
{
    ULONG InfoClass;

    /* Iterate over the process info classes and begin the tests */
    for (InfoClass = 0; InfoClass < _countof(PsThreadInfoClass); InfoClass++)
    {
        /* The buffer is misaligned */
        QuerySetThreadValidator(SET,
                                InfoClass,
                                (PVOID)(ULONG_PTR)1,
                                PsThreadInfoClass[InfoClass].RequiredSizeSET,
                                STATUS_DATATYPE_MISALIGNMENT);

        /* We set an invalid buffer address */
        QuerySetThreadValidator(SET,
                                InfoClass,
                                (PVOID)(ULONG_PTR)PsThreadInfoClass[InfoClass].AlignmentSET,
                                PsThreadInfoClass[InfoClass].RequiredSizeSET,
                                STATUS_ACCESS_VIOLATION);

        /* The information length is wrong */
        QuerySetThreadValidator(SET,
                                InfoClass,
                                (PVOID)(ULONG_PTR)PsThreadInfoClass[InfoClass].AlignmentSET,
                                PsThreadInfoClass[InfoClass].RequiredSizeSET - 1,
                                STATUS_INFO_LENGTH_MISMATCH);
    }
}

static
void
Test_ThreadNameInformation(void)
{
    NTSTATUS Status;
    THREAD_NAME_INFORMATION NameInfo;
    UCHAR Buffer[sizeof(THREAD_NAME_INFORMATION) + 256 * sizeof(WCHAR)];
    PTHREAD_NAME_INFORMATION QueryInfo = (PTHREAD_NAME_INFORMATION)Buffer;
    ULONG ReturnLength;
    static const WCHAR TestName[] = L"TestThreadName";
    static const WCHAR TestName2[] = L"UpdatedName";

    /* Set a thread name */
    RtlInitUnicodeString(&NameInfo.ThreadName, TestName);
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadNameInformation,
                                    &NameInfo,
                                    sizeof(NameInfo));
    ok_hex(Status, STATUS_SUCCESS);

    /* Query it back */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadNameInformation,
                                      Buffer,
                                      sizeof(Buffer),
                                      &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_long(QueryInfo->ThreadName.Length, (ULONG)(wcslen(TestName) * sizeof(WCHAR)));
    ok(QueryInfo->ThreadName.Buffer != NULL, "Expected non-NULL buffer\n");
    if (QueryInfo->ThreadName.Buffer)
    {
        ok(wcsncmp(QueryInfo->ThreadName.Buffer, TestName,
                   QueryInfo->ThreadName.Length / sizeof(WCHAR)) == 0,
           "Thread name mismatch: got '%.*ls'\n",
           QueryInfo->ThreadName.Length / (int)sizeof(WCHAR),
           QueryInfo->ThreadName.Buffer);
    }

    /* Update the name */
    RtlInitUnicodeString(&NameInfo.ThreadName, TestName2);
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadNameInformation,
                                    &NameInfo,
                                    sizeof(NameInfo));
    ok_hex(Status, STATUS_SUCCESS);

    /* Query the updated name */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadNameInformation,
                                      Buffer,
                                      sizeof(Buffer),
                                      &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_long(QueryInfo->ThreadName.Length, (ULONG)(wcslen(TestName2) * sizeof(WCHAR)));
    if (QueryInfo->ThreadName.Buffer)
    {
        ok(wcsncmp(QueryInfo->ThreadName.Buffer, TestName2,
                   QueryInfo->ThreadName.Length / sizeof(WCHAR)) == 0,
           "Updated name mismatch\n");
    }

    /* Clear the name by setting empty string */
    RtlInitUnicodeString(&NameInfo.ThreadName, NULL);
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadNameInformation,
                                    &NameInfo,
                                    sizeof(NameInfo));
    ok_hex(Status, STATUS_SUCCESS);

    /* Query - should be empty */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadNameInformation,
                                      Buffer,
                                      sizeof(Buffer),
                                      &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_long(QueryInfo->ThreadName.Length, 0);

    /* Query with too-small buffer should fail with STATUS_BUFFER_TOO_SMALL */
    RtlInitUnicodeString(&NameInfo.ThreadName, TestName);
    Status = NtSetInformationThread(GetCurrentThread(),
                                    ThreadNameInformation,
                                    &NameInfo,
                                    sizeof(NameInfo));
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtQueryInformationThread(GetCurrentThread(),
                                      ThreadNameInformation,
                                      Buffer,
                                      sizeof(THREAD_NAME_INFORMATION) - 1,
                                      &ReturnLength);
    ok_hex(Status, STATUS_BUFFER_TOO_SMALL);
    ok_long(ReturnLength, (ULONG)(sizeof(THREAD_NAME_INFORMATION) + wcslen(TestName) * sizeof(WCHAR)));

    /* Clean up */
    RtlInitUnicodeString(&NameInfo.ThreadName, NULL);
    NtSetInformationThread(GetCurrentThread(),
                           ThreadNameInformation,
                           &NameInfo,
                           sizeof(NameInfo));
}

START_TEST(NtSetInformationThread)
{
    Test_ThreadPriorityClass();
    Test_ThreadSetAlignmentProbe();
    Test_ThreadNameInformation();
}
