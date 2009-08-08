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
STDCALL
CreateJobObjectA(LPSECURITY_ATTRIBUTES lpJobAttributes,
                 LPCSTR lpName)
{
  HANDLE hJob;
  ANSI_STRING AnsiName;
  UNICODE_STRING UnicodeName;

  if(lpName != NULL)
  {
    NTSTATUS Status;

    RtlInitAnsiString(&AnsiName, lpName);
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, TRUE);
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }
  }

  hJob = CreateJobObjectW(lpJobAttributes,
                          ((lpName != NULL) ? UnicodeName.Buffer : NULL));

  if(lpName != NULL)
  {
    RtlFreeUnicodeString(&UnicodeName);
  }
  return hJob;
}


/*
 * @implemented
 */
HANDLE
STDCALL
CreateJobObjectW(LPSECURITY_ATTRIBUTES lpJobAttributes,
                 LPCWSTR lpName)
{
  UNICODE_STRING JobName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  ULONG Attributes = 0;
  PVOID SecurityDescriptor;
  HANDLE hJob;
  NTSTATUS Status;

  if(lpName != NULL)
  {
    RtlInitUnicodeString(&JobName, lpName);
  }

  if(lpJobAttributes != NULL)
  {
    if(lpJobAttributes->bInheritHandle)
    {
      Attributes |= OBJ_INHERIT;
    }
    SecurityDescriptor = lpJobAttributes->lpSecurityDescriptor;
  }
  else
  {
    SecurityDescriptor = NULL;
  }

  InitializeObjectAttributes(&ObjectAttributes,
                             ((lpName != NULL) ? &JobName : NULL),
                             Attributes,
                             NULL,
                             SecurityDescriptor);

  Status = NtCreateJobObject(&hJob,
                             JOB_OBJECT_ALL_ACCESS,
                             &ObjectAttributes);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return NULL;
  }

  return hJob;
}


/*
 * @implemented
 */
HANDLE
STDCALL
OpenJobObjectW(DWORD dwDesiredAccess,
               BOOL bInheritHandle,
               LPCWSTR lpName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING JobName;
  HANDLE hJob;
  NTSTATUS Status;

  if(lpName == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }

  RtlInitUnicodeString(&JobName, lpName);

  InitializeObjectAttributes(&ObjectAttributes,
                             &JobName,
                             (bInheritHandle ? OBJ_INHERIT : 0),
                             NULL,
                             NULL);

  Status = NtOpenJobObject(&hJob,
                           dwDesiredAccess,
                           &ObjectAttributes);

  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return NULL;
  }

  return hJob;
}


/*
 * @implemented
 */
HANDLE
STDCALL
OpenJobObjectA(DWORD dwDesiredAccess,
               BOOL bInheritHandle,
               LPCSTR lpName)
{
  ANSI_STRING AnsiName;
  UNICODE_STRING UnicodeName;
  HANDLE hJob;
  NTSTATUS Status;

  if(lpName == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }

  RtlInitAnsiString(&AnsiName, lpName);
  Status = RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, TRUE);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  hJob = OpenJobObjectW(dwDesiredAccess,
                        bInheritHandle,
                        UnicodeName.Buffer);

  RtlFreeUnicodeString(&UnicodeName);
  return hJob;
}


/*
 * @implemented
 */
BOOL
STDCALL
IsProcessInJob(HANDLE ProcessHandle,
               HANDLE JobHandle,
               PBOOL Result)
{
  NTSTATUS Status;

  Status = NtIsProcessInJob(ProcessHandle, JobHandle);
  if(NT_SUCCESS(Status))
  {
    *Result = (Status == STATUS_PROCESS_IN_JOB);
    return TRUE;
  }

  SetLastErrorByStatus(Status);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
AssignProcessToJobObject(HANDLE hJob,
                         HANDLE hProcess)
{
  NTSTATUS Status;

  Status = NtAssignProcessToJobObject(hJob, hProcess);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
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
  if(NT_SUCCESS(Status))
  {
    PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicInfo;
    switch(JobObjectInformationClass)
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

    if(BasicInfo != NULL)
    {
      /* we need to convert the process priority classes in the
         JOBOBJECT_BASIC_LIMIT_INFORMATION structure the same way as
         GetPriorityClass() converts it! */
      switch(BasicInfo->PriorityClass)
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

  SetLastErrorByStatus(Status);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetInformationJobObject(HANDLE hJob,
                        JOBOBJECTINFOCLASS JobObjectInformationClass,
                        LPVOID lpJobObjectInformation,
                        DWORD cbJobObjectInformationLength)
{
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo;
  PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicInfo;
  PVOID ObjectInfo;
  NTSTATUS Status;

  switch(JobObjectInformationClass)
  {
    case JobObjectBasicLimitInformation:
      if(cbJobObjectInformationLength != sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION))
      {
        SetLastError(ERROR_BAD_LENGTH);
        return FALSE;
      }
      ObjectInfo = &ExtendedLimitInfo.BasicLimitInformation;
      BasicInfo = (PJOBOBJECT_BASIC_LIMIT_INFORMATION)ObjectInfo;
      RtlCopyMemory(ObjectInfo, lpJobObjectInformation, cbJobObjectInformationLength);
      break;

    case JobObjectExtendedLimitInformation:
      if(cbJobObjectInformationLength != sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION))
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

  if(BasicInfo != NULL)
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
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
TerminateJobObject(HANDLE hJob,
                   UINT uExitCode)
{
  NTSTATUS Status;

  Status = NtTerminateJobObject(hJob, uExitCode);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/* EOF */
