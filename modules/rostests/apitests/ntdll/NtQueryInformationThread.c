/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for the NtQueryInformationThread API
 * COPYRIGHT:   Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include <internal/ps_i.h>

#define ok_wchar(x, y) ok_hex((unsigned)(unsigned short)(x), (unsigned)(unsigned short)(y))

#define ok_nwstr(str1, str2, count) \
    ok(wcsncmp((PWCHAR)(str1), (PWCHAR)(str2), (count)) == 0, \
       "Wrong string. Expected '%.*S', got '%.*S'\n", (count), (str1), (count), (str2))

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

/**
 * @brief   An NT-equivalent of kernel32!GetThreadDescription().
 **/
static NTSTATUS
ntGetThreadName(
    _In_ HANDLE hThread,
    _Outptr_ PTHREAD_NAME_INFORMATION* ppThreadNameInfo,
    _Out_ PULONG pLength,
    _In_ UCHAR Fill)
{
    PTHREAD_NAME_INFORMATION NameInfo = NULL;
    PVOID ptr;
    NTSTATUS Status;
    ULONG Length;

    *ppThreadNameInfo = NULL;
    *pLength = 0;

    /* Since there may be concurrent SetThreadDescription() invocations being
     * executed, we need to loop over the buffer allocation size until we can
     * successfully capture the thread name. */
    Length = 0;
    Status = NtQueryInformationThread(hThread, ThreadNameInformation, NULL, 0, &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return Status;

    /* Loop again only if we get buffer-too-small types of errors;
     * otherwise, break the loop: any error will be handled below. */
    while (Status == STATUS_INFO_LENGTH_MISMATCH ||
           Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* (Re-)allocate the information buffer */
        ptr = (NameInfo ? RtlReAllocateHeap(RtlGetProcessHeap(), 0, NameInfo, Length)
                        : RtlAllocateHeap(RtlGetProcessHeap(), 0, Length));
        if (!ptr)
        {
            Status = STATUS_NO_MEMORY;
            break;
        }
        RtlFillMemory(ptr, Length, Fill);
        NameInfo = ptr;
        Status = NtQueryInformationThread(hThread, ThreadNameInformation,
                                          NameInfo, Length, &Length);
        // Status should be STATUS_SUCCESS, unless concurrent SetThreadDescription() happens.
    }
    if (NT_SUCCESS(Status))
    {
        *ppThreadNameInfo = NameInfo;
        *pLength = Length;
    }
    else
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NameInfo);
    }

    return Status;
}

static void
DoTest(
    _In_ UINT i,
    _In_ HANDLE hThread,
    _In_opt_ PCWSTR TestName)
{
    BOOL bEmptyName = ((TestName == (PCWSTR)(ULONG_PTR)-1) || !TestName || !*TestName);
    ULONG ExpectedNameLength = (ULONG)(bEmptyName ? 0 : wcslen(TestName) * sizeof(WCHAR));
    ULONG ExpectedLength = sizeof(UNICODE_STRING) + ExpectedNameLength;

    NTSTATUS Status;
    ULONG Length;
    PTHREAD_NAME_INFORMATION pNameInfo;
    // IMPORTANT REMARK: Make the fixed Buffer large enough
    // to hold all the TestName's being tested in this file.
    struct { THREAD_NAME_INFORMATION Info; WCHAR Buffer[MAX_PATH]; } NameInfo;

    ASSERT(sizeof(NameInfo.Buffer) >= ExpectedNameLength);

    winetest_push_context("Test %lu", i);

    /* Set a new thread name only if it's not '-1'; otherwise we
     * just query the existing name without setting it before. */
    if (TestName != (PCWSTR)(ULONG_PTR)-1)
    {
        /* Set a non-empty thread name, to reset it to something different
         * from the previous case and distinguish with the next one. */
        RtlFillMemory(&NameInfo, sizeof(NameInfo), 0xEF);
        NameInfo.Info.ThreadName.Length = NameInfo.Info.ThreadName.MaximumLength = sizeof(NameInfo.Buffer);
        NameInfo.Info.ThreadName.Buffer = NameInfo.Buffer;
        Status = NtSetInformationThread(hThread, ThreadNameInformation,
                                        &NameInfo.Info, sizeof(NameInfo.Info));
        ok_ntstatus(Status, STATUS_SUCCESS);

        /* Now set the thread name to be tested */
        RtlFillMemory(&NameInfo, sizeof(NameInfo), 0xAB);
        RtlInitUnicodeString(&NameInfo.Info.ThreadName, TestName);
        Status = NtSetInformationThread(hThread, ThreadNameInformation,
                                        &NameInfo.Info, sizeof(NameInfo.Info));
        ok_ntstatus(Status, STATUS_SUCCESS);
    }

    /* Verify that ThreadNameInformation call with NULL buffer returns the expected length */
    Length = 0;
    Status = NtQueryInformationThread(hThread, ThreadNameInformation, NULL, 0, &Length);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_long(Length, ExpectedLength);

    pNameInfo = &NameInfo.Info;

    /* Test ThreadNameInformation call with minimally-sized buffer */
    RtlFillMemory(&NameInfo, sizeof(NameInfo), 0xAB);
    Length = 0;
    Status = NtQueryInformationThread(hThread, ThreadNameInformation,
                                      pNameInfo, sizeof(*pNameInfo), &Length);
    if (bEmptyName)
    {
        ok_ntstatus(Status, STATUS_SUCCESS);
        ok_long(Length, ExpectedLength); // == sizeof(UNICODE_STRING);
        ok_long((ULONG)pNameInfo->ThreadName.Length, 0);
    }
    else
    {
        ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
        ok_long(Length, ExpectedLength); // == sizeof(UNICODE_STRING) + ExpectedNameLength;
        ok_long((ULONG)pNameInfo->ThreadName.Length, 0xABAB);
    }
    ok_long((ULONG)pNameInfo->ThreadName.MaximumLength, (ULONG)pNameInfo->ThreadName.Length);
    if (bEmptyName)
    {
        ok(pNameInfo->ThreadName.Buffer == NULL || /* ReactOS will set the Buffer to NULL */
           /* All Win10+ versions erroneously point the Buffer past the end of the data */
           broken(pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1)),
           "Expected NULL pointer, got 0x%p\n", pNameInfo->ThreadName.Buffer);
        // NOTE: pNameInfo->ThreadName.Buffer[0] is invalid.
    }
    else
    {
        ok_ptr(pNameInfo->ThreadName.Buffer, (PVOID)(ULONG_PTR)0xABABABABABABABABULL);
    }

    RtlFillMemory(&NameInfo, sizeof(NameInfo), 0xAB);
    Length = 0;
    if (bEmptyName)
    {
        /* Retest the same with a slightly-larger buffer */
        Status = NtQueryInformationThread(hThread, ThreadNameInformation,
                                          &NameInfo,
                                          sizeof(UNICODE_STRING) + sizeof(UNICODE_NULL),
                                          &Length);
    }
    else
    {
        /* Test direct ThreadNameInformation call with correctly-sized buffer */
        Status = NtQueryInformationThread(hThread, ThreadNameInformation,
                                          &NameInfo,
                                          ExpectedLength,
                                          &Length);
    }
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(Length, ExpectedLength);
    ok_long((ULONG)pNameInfo->ThreadName.Length, ExpectedNameLength);
    ok_long((ULONG)pNameInfo->ThreadName.MaximumLength, (ULONG)pNameInfo->ThreadName.Length);
    if (bEmptyName)
    {
        ok(pNameInfo->ThreadName.Buffer == NULL || /* ReactOS will set the Buffer to NULL */
           /* All Win10+ versions erroneously point the Buffer past the end of the data */
           broken(pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1)),
           "Expected NULL pointer, got 0x%p\n", pNameInfo->ThreadName.Buffer);

        // NOTE: pNameInfo->ThreadName.Buffer[0] should be invalid, but let's
        // check past it to see what data there is (in the "broken" Windows case).
        if (pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1))
            ok_wchar(pNameInfo->ThreadName.Buffer[0], 0xABAB);
    }
    else
    {
        ok_ptr(pNameInfo->ThreadName.Buffer, (PVOID)(&pNameInfo->ThreadName + 1));
        if (pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1))
            ok_nwstr(pNameInfo->ThreadName.Buffer, TestName, ExpectedNameLength / sizeof(WCHAR));
        else
            skip("pNameInfo->ThreadName.Buffer is invalid (0x%p)\n", pNameInfo->ThreadName.Buffer);
    }

    /* Test ThreadNameInformation with dynamically allocated buffer */
    pNameInfo = NULL;
    Length = 0;
    Status = ntGetThreadName(hThread, &pNameInfo, &Length, 0xCD);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(Length, ExpectedLength);
    ok(pNameInfo != NULL, "Expected not NULL, got NULL\n");
    if (pNameInfo)
    {
        ok_long((ULONG)pNameInfo->ThreadName.Length, ExpectedNameLength);
        ok_long((ULONG)pNameInfo->ThreadName.MaximumLength, (ULONG)pNameInfo->ThreadName.Length);
        if (bEmptyName)
        {
            ok(pNameInfo->ThreadName.Buffer == NULL || /* ReactOS will set the Buffer to NULL */
               /* All Win10+ versions erroneously point the Buffer past the end of the data */
               broken(pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1)),
               "Expected NULL pointer, got 0x%p\n", pNameInfo->ThreadName.Buffer);
            // NOTE: pNameInfo->ThreadName.Buffer[0] is invalid.
        }
        else
        {
            ok_ptr(pNameInfo->ThreadName.Buffer, (PVOID)(&pNameInfo->ThreadName + 1));
            if (pNameInfo->ThreadName.Buffer == (PVOID)(&pNameInfo->ThreadName + 1))
                ok_nwstr(pNameInfo->ThreadName.Buffer, TestName, ExpectedNameLength / sizeof(WCHAR));
            else
                skip("pNameInfo->ThreadName.Buffer is invalid (0x%p)\n", pNameInfo->ThreadName.Buffer);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, pNameInfo);
    }
    else
    {
        skip("pNameInfo is NULL\n");
    }

    winetest_pop_context();
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
Test_ThreadNameInformation(void)
{
    ULONG NTDDIVersion;
    NTSTATUS Status;
    HANDLE hWaitEvent = NULL, hThread = NULL;

    OSVERSIONINFOEXW osVerInfo;
    osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
    GetVersionExW((LPOSVERSIONINFOW)&osVerInfo);
    trace("osVerInfo: %lu.%lu.%lu ('%S')\n",
          osVerInfo.dwMajorVersion,
          osVerInfo.dwMinorVersion,
          osVerInfo.dwBuildNumber,
          osVerInfo.szCSDVersion);

    NTDDIVersion = GetNTDDIVersion();
    trace("NTDDIVersion: 0x%08lx\n", NTDDIVersion);
    if (NTDDIVersion < NTDDI_WIN10_RS1)
    {
        trace("Running %s on NT %hu.%hu(.%hu), it may not work!\n",
              __FUNCTION__,
              (NTDDIVersion & 0xFF000000) >> 24,
              (NTDDIVersion & 0x00FF0000) >> 16,
              (NTDDIVersion & 0x000000FF));
    }

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

    /* EXPECTATION: Initial thread name is empty. */
    DoTest(1, hThread, (PCWSTR)(ULONG_PTR)-1);

    /* Set a non-empty thread name.
     * EXPECTATION: Thread name is what we have set. */
    DoTest(2, hThread, L"Hello World!");

    /* Set an empty-string thread name.
     * EXPECTATION: Thread name is empty. */
    DoTest(3, hThread, L"");

    /* Set a NULL-pointer empty-string thread name.
     * EXPECTATION: Thread name is empty. */
    DoTest(4, hThread, NULL);

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

START_TEST(NtQueryInformationThread)
{
    Test_ThreadBasicInformationClass();
    Test_ThreadQueryAlignmentProbe();
    Test_ThreadNameInformation();
}
