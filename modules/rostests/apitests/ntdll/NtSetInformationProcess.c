/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetInformationProcess API
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
void
Test_ProcBasePriorityClass(void)
{
    NTSTATUS Status;

    /*
     * Assign a priority of HIGH_PRIORITY (see pstypes.h).
     * The function will fail with a status of STATUS_PRIVILEGE_NOT_HELD
     * as the process who executed the caller doesn't have the requested
     * privileges to perform the operation.
    */
    KPRIORITY BasePriority = HIGH_PRIORITY;

    /* Everything is NULL */
    Status = NtSetInformationProcess(NULL,
                                     ProcessBasePriority,
                                     NULL,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Set the base priority to an invalid process handle */
    Status = NtSetInformationProcess(NULL,
                                     ProcessBasePriority,
                                     &BasePriority,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* Don't input anything to the caller but the length information is valid */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     NULL,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* Give the base priority input to the caller but length is invalid */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     &BasePriority,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The input buffer is misaligned and length information invalid */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     (PVOID)1,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The input buffer is misaligned */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     (PVOID)1,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* The input buffer is misaligned (try with an alignment of 2) */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     (PVOID)2,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* Set the base priority but we have lack privileges to do so */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     &BasePriority,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_PRIVILEGE_NOT_HELD);

    /*
     * Assign a random priority value this time and
     * set the base priority to the current process.
    */
    BasePriority = 8;
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     &BasePriority,
                                     sizeof(KPRIORITY));
    ok_hex(Status, STATUS_SUCCESS);
}

static
void
Test_ProcRaisePriorityClass(void)
{
    NTSTATUS Status;

    /* Raise the priority as much as possible */
    ULONG RaisePriority = MAXIMUM_PRIORITY;

    /* Everything is NULL */
    Status = NtSetInformationProcess(NULL,
                                     ProcessRaisePriority,
                                     NULL,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* A invalid handle to process is given */
    Status = NtSetInformationProcess(NULL,
                                     ProcessRaisePriority,
                                     &RaisePriority,
                                     sizeof(ULONG));
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* The input buffer is misaligned and length information invalid */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessRaisePriority,
                                     (PVOID)1,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* A NULL buffer has been accessed */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessRaisePriority,
                                     NULL,
                                     sizeof(ULONG));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* The input buffer is misaligned */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessRaisePriority,
                                     (PVOID)1,
                                     sizeof(ULONG));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* The input buffer is misaligned -- try with an alignment size of 2 */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessRaisePriority,
                                     (PVOID)2,
                                     sizeof(ULONG));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* Raise the priority of the given current process */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessRaisePriority,
                                     &RaisePriority,
                                     sizeof(ULONG));
    ok_hex(Status, STATUS_SUCCESS);
}

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

static
void
Test_ProcessWx86InformationClass(void)
{
    NTSTATUS Status;
    ULONG VdmPower = 1;

    /* Everything is NULL */
    Status = NtSetInformationProcess(NULL,
                                     ProcessWx86Information,
                                     NULL,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Don't set anything to the process */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessWx86Information,
                                     NULL,
                                     sizeof(VdmPower));
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* The buffer is misaligned and information length is wrong */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessWx86Information,
                                     (PVOID)1,
                                     0);
    ok_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* The buffer is misaligned */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessWx86Information,
                                     (PVOID)1,
                                     sizeof(VdmPower));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* The buffer is misaligned -- try with an alignment size of 2 */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessWx86Information,
                                     (PVOID)2,
                                     sizeof(VdmPower));
    ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

    /* We do not have privileges to set the VDM power */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessWx86Information,
                                     &VdmPower,
                                     sizeof(VdmPower));
    ok_hex(Status, STATUS_PRIVILEGE_NOT_HELD);
}

START_TEST(NtSetInformationProcess)
{
    Test_ProcForegroundBackgroundClass();
    Test_ProcBasePriorityClass();
    Test_ProcRaisePriorityClass();
    Test_ProcessWx86InformationClass();
}
