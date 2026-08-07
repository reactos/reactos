/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/job.c
 * PURPOSE:         Job functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *                  Created 9/23/2004
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#include <winspool.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateJobObjectA(_In_ LPSECURITY_ATTRIBUTES lpJobAttributes,
                 _In_ LPCSTR lpName)
{
    /* Call the W(ide) function */
    ConvertWin32AnsiObjectApiToUnicodeApi(JobObject, lpName, lpJobAttributes);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateJobObjectW(_In_ LPSECURITY_ATTRIBUTES lpJobAttributes,
                 _In_ LPCWSTR lpName)
{
    /* Create the NT object */
    CreateNtObjectFromWin32Api(JobObject, JobObject, JOB_OBJECT_ALL_ACCESS, lpJobAttributes, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenJobObjectW(_In_ DWORD dwDesiredAccess,
               _In_ BOOL bInheritHandle,
               _In_ LPCWSTR lpName)
{
    /* Open the NT object */
    OpenNtObjectFromWin32Api(JobObject, dwDesiredAccess, bInheritHandle, lpName);
}


/*
 * @implemented
 */
HANDLE
WINAPI
OpenJobObjectA(_In_ DWORD dwDesiredAccess,
               _In_ BOOL bInheritHandle,
               _In_ LPCSTR lpName)
{
    /* Call the W(ide) function */
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(JobObject, dwDesiredAccess, bInheritHandle, lpName);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsProcessInJob(_In_ HANDLE ProcessHandle,
               _In_opt_ HANDLE JobHandle,
               _Out_ PBOOL Result)
{
    NTSTATUS Status;

    Status = NtIsProcessInJob(ProcessHandle, JobHandle);
    if (NT_SUCCESS(Status))
    {
        *Result = (Status == STATUS_PROCESS_IN_JOB);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AssignProcessToJobObject(_In_ HANDLE hJob,
                         _In_ HANDLE hProcess)
{
    NTSTATUS Status;

    Status = NtAssignProcessToJobObject(hJob, hProcess);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
QueryInformationJobObject(
    _In_opt_ HANDLE hJob,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _Out_writes_bytes_to_(cbJobObjectInformationLength, *lpReturnLength) LPVOID
    lpJobObjectInformation,
    _In_ DWORD cbJobObjectInformationLength,
    _Out_opt_ LPDWORD lpReturnLength)
{
    NTSTATUS Status;
    PVOID JobInfo;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION LocalInfo;
    ULONG ExpectedSize;

    if (JobObjectInformationClass == JobObjectBasicLimitInformation)
    {
        ExpectedSize = sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION);
        JobInfo = &LocalInfo;
    }
    else if (JobObjectInformationClass == JobObjectExtendedLimitInformation)
    {
        ExpectedSize = sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION);
        JobInfo = &LocalInfo;
    }
    else
    {
        ExpectedSize = cbJobObjectInformationLength;
        JobInfo = lpJobObjectInformation;
    }

    if (cbJobObjectInformationLength != ExpectedSize)
    {
        BaseSetLastNTError(STATUS_INFO_LENGTH_MISMATCH);
        return FALSE;
    }

    Status = NtQueryInformationJobObject(hJob,
                                         JobObjectInformationClass,
                                         JobInfo,
                                         ExpectedSize,
                                         lpReturnLength);
    if (NT_SUCCESS(Status))
    {
        if (JobInfo != &LocalInfo) return TRUE;

        switch (LocalInfo.BasicLimitInformation.PriorityClass)
        {
            case PROCESS_PRIORITY_CLASS_IDLE:
                LocalInfo.BasicLimitInformation.PriorityClass =
                IDLE_PRIORITY_CLASS;
                break;

            case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
                LocalInfo.BasicLimitInformation.PriorityClass =
                BELOW_NORMAL_PRIORITY_CLASS;
                break;

            case PROCESS_PRIORITY_CLASS_NORMAL:
                LocalInfo.BasicLimitInformation.PriorityClass =
                NORMAL_PRIORITY_CLASS;
                break;

            case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
                LocalInfo.BasicLimitInformation.PriorityClass =
                ABOVE_NORMAL_PRIORITY_CLASS;
                break;

            case PROCESS_PRIORITY_CLASS_HIGH:
                LocalInfo.BasicLimitInformation.PriorityClass =
                HIGH_PRIORITY_CLASS;
                break;

            case PROCESS_PRIORITY_CLASS_REALTIME:
                LocalInfo.BasicLimitInformation.PriorityClass =
                REALTIME_PRIORITY_CLASS;
                break;

            default:
                LocalInfo.BasicLimitInformation.PriorityClass =
                NORMAL_PRIORITY_CLASS;
                break;
        }

        RtlCopyMemory(lpJobObjectInformation, &LocalInfo, ExpectedSize);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetInformationJobObject(
    _In_ HANDLE hJob,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _In_reads_bytes_(cbJobObjectInformationLength) LPVOID lpJobObjectInformation,
    _In_ DWORD cbJobObjectInformationLength)
{
    NTSTATUS Status;
    PVOID JobInfo;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION LocalInfo;
    ULONG ExpectedSize;
    PVOID State = NULL;
    ULONG Privilege = SE_INC_BASE_PRIORITY_PRIVILEGE;

    if (JobObjectInformationClass == JobObjectBasicLimitInformation)
    {
        ExpectedSize = sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION);
        JobInfo = &LocalInfo;
    }
    else if (JobObjectInformationClass == JobObjectExtendedLimitInformation)
    {
        ExpectedSize = sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION);
        JobInfo = &LocalInfo;
    }
    else
    {
        ExpectedSize = cbJobObjectInformationLength;
        JobInfo = lpJobObjectInformation;
    }

    if (cbJobObjectInformationLength != ExpectedSize)
    {
        BaseSetLastNTError(STATUS_INFO_LENGTH_MISMATCH);
        return FALSE;
    }

    if (JobInfo == &LocalInfo)
    {
        RtlCopyMemory(&LocalInfo, lpJobObjectInformation, ExpectedSize);

        if (LocalInfo.BasicLimitInformation.LimitFlags &
            JOB_OBJECT_LIMIT_PRIORITY_CLASS)
        {
            switch (LocalInfo.BasicLimitInformation.PriorityClass)
            {
                case IDLE_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_IDLE;
                    break;

                case BELOW_NORMAL_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
                    break;

                case NORMAL_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_NORMAL;
                    break;

                case ABOVE_NORMAL_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
                    break;

                case HIGH_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_HIGH;
                    break;

                case REALTIME_PRIORITY_CLASS:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_REALTIME;
                    break;

                default:
                    LocalInfo.BasicLimitInformation.PriorityClass =
                    PROCESS_PRIORITY_CLASS_NORMAL;
                    break;
            }
        }

        if (LocalInfo.BasicLimitInformation.LimitFlags &
            JOB_OBJECT_LIMIT_WORKINGSET)
        {
            Status = RtlAcquirePrivilege(&Privilege, 1, 0, &State);
        }
    }

    Status = NtSetInformationJobObject(hJob,
                                       JobObjectInformationClass,
                                       JobInfo,
                                       ExpectedSize);
    if (NT_SUCCESS(Status))
    {
        if (State != NULL) RtlReleasePrivilege(State);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
TerminateJobObject(_In_ HANDLE hJob,
                   _In_ UINT uExitCode)
{
    NTSTATUS Status;

    Status = NtTerminateJobObject(hJob, uExitCode);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateJobSet(IN ULONG NumJob,
             IN PJOB_SET_ARRAY UserJobSet,
             IN ULONG Flags)
{
    NTSTATUS Status;

    Status = NtCreateJobSet(NumJob, UserJobSet, Flags);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
