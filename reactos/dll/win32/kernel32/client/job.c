/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/process/job.c
 * PURPOSE:         Job functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *                  Created 9/23/2004
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateJobObjectA(IN LPSECURITY_ATTRIBUTES lpJobAttributes,
                 IN LPCSTR lpName)
{
    /* Call the W(ide) function */
    ConvertWin32AnsiObjectApiToUnicodeApi(JobObject, lpName, lpJobAttributes);
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateJobObjectW(IN LPSECURITY_ATTRIBUTES lpJobAttributes,
                 IN LPCWSTR lpName)
{
    /* Create the NT object */
    CreateNtObjectFromWin32Api(JobObject, JobObject, JOB, lpJobAttributes, lpName);
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenJobObjectW(DWORD dwDesiredAccess,
               BOOL bInheritHandle,
               LPCWSTR lpName)
{
    /* Open the NT object */
    OpenNtObjectFromWin32Api(JobObject, dwDesiredAccess, bInheritHandle, lpName);
}


/*
 * @implemented
 */
HANDLE
WINAPI
OpenJobObjectA(DWORD dwDesiredAccess,
               BOOL bInheritHandle,
               LPCSTR lpName)
{
    /* Call the W(ide) function */
    ConvertOpenWin32AnsiObjectApiToUnicodeApi(JobObject, dwDesiredAccess, bInheritHandle, lpName);
}


/*
 * @implemented
 */
BOOL
WINAPI
IsProcessInJob(HANDLE ProcessHandle,
               HANDLE JobHandle,
               PBOOL Result)
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
AssignProcessToJobObject(HANDLE hJob,
                         HANDLE hProcess)
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
QueryInformationJobObject(HANDLE hJob,
                          JOBOBJECTINFOCLASS JobObjectInformationClass,
                          LPVOID lpJobObjectInformation,
                          DWORD cbJobObjectInformationLength,
                          LPDWORD lpReturnLength)
{
    NTSTATUS Status;

    Status = NtQueryInformationJobObject(hJob,
                                         JobObjectInformationClass,
                                         lpJobObjectInformation,
                                         cbJobObjectInformationLength,
                                         lpReturnLength);
    if (NT_SUCCESS(Status))
    {
        PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicInfo;

        switch (JobObjectInformationClass)
        {
            case JobObjectBasicLimitInformation:
                BasicInfo = (PJOBOBJECT_BASIC_LIMIT_INFORMATION)lpJobObjectInformation;
                break;

            case JobObjectExtendedLimitInformation:
                BasicInfo = &((PJOBOBJECT_EXTENDED_LIMIT_INFORMATION)lpJobObjectInformation)->BasicLimitInformation;
                break;

            default:
                BasicInfo = NULL;
                break;
        }

        if (BasicInfo != NULL)
        {
            /* we need to convert the process priority classes in the
               JOBOBJECT_BASIC_LIMIT_INFORMATION structure the same way as
               GetPriorityClass() converts it! */
            switch (BasicInfo->PriorityClass)
            {
                case PROCESS_PRIORITY_CLASS_IDLE:
                    BasicInfo->PriorityClass = IDLE_PRIORITY_CLASS;
                    break;

                case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
                    BasicInfo->PriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
                    break;

                case PROCESS_PRIORITY_CLASS_NORMAL:
                    BasicInfo->PriorityClass = NORMAL_PRIORITY_CLASS;
                    break;

                case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
                    BasicInfo->PriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
                    break;

                case PROCESS_PRIORITY_CLASS_HIGH:
                    BasicInfo->PriorityClass = HIGH_PRIORITY_CLASS;
                    break;

                case PROCESS_PRIORITY_CLASS_REALTIME:
                    BasicInfo->PriorityClass = REALTIME_PRIORITY_CLASS;
                    break;

                default:
                    BasicInfo->PriorityClass = NORMAL_PRIORITY_CLASS;
                    break;
            }
        }

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
SetInformationJobObject(HANDLE hJob,
                        JOBOBJECTINFOCLASS JobObjectInformationClass,
                        LPVOID lpJobObjectInformation,
                        DWORD cbJobObjectInformationLength)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo;
    PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicInfo;
    PVOID ObjectInfo;
    NTSTATUS Status;

    switch (JobObjectInformationClass)
    {
        case JobObjectBasicLimitInformation:
            if (cbJobObjectInformationLength != sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION))
            {
                SetLastError(ERROR_BAD_LENGTH);
                return FALSE;
            }

            ObjectInfo = &ExtendedLimitInfo.BasicLimitInformation;
            BasicInfo = (PJOBOBJECT_BASIC_LIMIT_INFORMATION)ObjectInfo;
            RtlCopyMemory(ObjectInfo, lpJobObjectInformation, cbJobObjectInformationLength);
            break;

        case JobObjectExtendedLimitInformation:
            if (cbJobObjectInformationLength != sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION))
            {
                SetLastError(ERROR_BAD_LENGTH);
                return FALSE;
            }

            ObjectInfo = &ExtendedLimitInfo;
            BasicInfo = &ExtendedLimitInfo.BasicLimitInformation;
            RtlCopyMemory(ObjectInfo, lpJobObjectInformation, cbJobObjectInformationLength);
            break;

        default:
            ObjectInfo = lpJobObjectInformation;
            BasicInfo = NULL;
            break;
    }

    if (BasicInfo != NULL)
    {
        /* we need to convert the process priority classes in the
           JOBOBJECT_BASIC_LIMIT_INFORMATION structure the same way as
           SetPriorityClass() converts it! */
        switch(BasicInfo->PriorityClass)
        {
            case IDLE_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
                break;

            case BELOW_NORMAL_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
                break;

            case NORMAL_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
                break;

            case ABOVE_NORMAL_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
                break;

            case HIGH_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
                break;

            case REALTIME_PRIORITY_CLASS:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;
                break;

            default:
                BasicInfo->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
                break;
        }
    }

    Status = NtSetInformationJobObject(hJob,
                                       JobObjectInformationClass,
                                       ObjectInfo,
                                       cbJobObjectInformationLength);
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
TerminateJobObject(HANDLE hJob,
                   UINT uExitCode)
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
 * @unimplemented
 */
BOOL
WINAPI
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
    STUB;
    return 0;
}

/* EOF */
