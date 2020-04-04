/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetInformationProcess API
 * COPYRIGHT:       Copyright 2020 Bișoc George <fraizeraust99 at gmail dot com>
 */

#include "precomp.h"

static
void
Test_ProcForegroundBackgroundClass(void)
{
    NTSTATUS Status;
    PPROCESS_FOREGROUND_BACKGROUND ProcForeground;

    ProcForeground = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROCESS_FOREGROUND_BACKGROUND));
    if (ProcForeground == NULL)
    {
        skip("Failed to allocate memory block from heap for PROCESS_FOREGROUND_BACKGROUND!\n");
        return;
    }

    /* As a test, set the foreground of the retrieved current process to FALSE */
    ProcForeground->Foreground = FALSE;

    /* Set everything NULL for the caller */
    Status = NtSetInformationProcess(NULL,
                                     ProcessForegroundInformation,
                                     NULL,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Give an invalid process handle (but the input buffer and length are correct) */
    Status = NtSetInformationProcess(NULL,
                                     ProcessForegroundInformation,
                                     ProcForeground,
                                     sizeof(PROCESS_FOREGROUND_BACKGROUND));
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* Give a buffer data to the argument input as NULL */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessForegroundInformation,
                                     NULL,
                                     sizeof(PROCESS_FOREGROUND_BACKGROUND));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* Set the information process foreground with an incorrect buffer alignment and zero buffer length */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessForegroundInformation,
                                     (PVOID)1,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /*
     * Set the information process foreground with an incorrect buffer alignment but correct size length.
     * The function will return STATUS_ACCESS_VIOLATION as the alignment probe requirement is not performed
     * in this class.
     */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessForegroundInformation,
                                     (PVOID)1,
                                     sizeof(PROCESS_FOREGROUND_BACKGROUND));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* Set the foreground information to the current given process, we must expect the function should succeed */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessForegroundInformation,
                                     ProcForeground,
                                     sizeof(PROCESS_FOREGROUND_BACKGROUND));
    ok_hex(Status, STATUS_SUCCESS);

    /* Clear all the stuff */
    HeapFree(GetProcessHeap(), 0, ProcForeground);
}

START_TEST(NtSetInformationProcess)
{
    Test_ProcForegroundBackgroundClass();
}
