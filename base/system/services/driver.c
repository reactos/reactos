/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/driver.c
 * PURPOSE:     Driver control interface
 * COPYRIGHT:   Copyright 2005-2006 Eric Kohl
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

DWORD
ScmLoadDriver(PSERVICE lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Build the driver path */
    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    /* FIXME: Acquire privilege */

    DPRINT("  Path: %wZ\n", &DriverPath);
    Status = NtLoadDriver(&DriverPath);

    /* FIXME: Release privilege */

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
    }

    return dwError;
}


DWORD
ScmUnloadDriver(PSERVICE lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;

    /* Build the driver path */
    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    /* FIXME: Acquire privilege */

    Status = NtUnloadDriver(&DriverPath);

    /* FIXME: Release privilege */

    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
    }

    return dwError;
}


DWORD
ScmGetDriverStatus(PSERVICE lpService,
                   LPSERVICE_STATUS lpServiceStatus)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE DirHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG BufferLength;
    ULONG DataLength;
    ULONG Index;
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bFound = FALSE;

    DPRINT1("ScmGetDriverStatus() called\n");

    memset(lpServiceStatus, 0, sizeof(SERVICE_STATUS));

    if (lpService->Status.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        RtlInitUnicodeString(&DirName,
                             L"\\Driver");
    }
    else
    {
        RtlInitUnicodeString(&DirName,
                             L"\\FileSystem");
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DirHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenDirectoryObject() failed!\n");
        return RtlNtStatusToDosError(Status);
    }

    BufferLength = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                   2 * MAX_PATH * sizeof(WCHAR);
    DirInfo = (OBJECT_DIRECTORY_INFORMATION*) HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        BufferLength);

    Index = 0;
    while (TRUE)
    {
        Status = NtQueryDirectoryObject(DirHandle,
                                        DirInfo,
                                        BufferLength,
                                        TRUE,
                                        FALSE,
                                        &Index,
                                        &DataLength);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            DPRINT("No more services\n");
            break;
        }

        if (!NT_SUCCESS(Status))
            break;

        DPRINT("Comparing: '%S'  '%wZ'\n", lpService->lpServiceName, &DirInfo->Name);

        if (_wcsicmp(lpService->lpServiceName, DirInfo->Name.Buffer) == 0)
        {
            DPRINT1("Found: '%S'  '%wZ'\n",
                    lpService->lpServiceName, &DirInfo->Name);
            bFound = TRUE;

            break;
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             DirInfo);
    NtClose(DirHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Status: %lx\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    if ((bFound == TRUE) &&
        (lpService->Status.dwCurrentState != SERVICE_STOP_PENDING))
    {
        if (lpService->Status.dwCurrentState == SERVICE_STOPPED)
        {
            lpService->Status.dwWin32ExitCode = ERROR_SUCCESS;
            lpService->Status.dwServiceSpecificExitCode = ERROR_SUCCESS;
            lpService->Status.dwCheckPoint = 0;
            lpService->Status.dwWaitHint = 0;
            lpService->Status.dwControlsAccepted = 0;
        }
        else
        {
            lpService->Status.dwCurrentState = SERVICE_RUNNING;
            lpService->Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

            if (lpService->Status.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED)
                lpService->Status.dwWin32ExitCode = ERROR_SUCCESS;
        }
    }
    else
    {
        lpService->Status.dwCurrentState = SERVICE_STOPPED;
        lpService->Status.dwControlsAccepted = 0;
        lpService->Status.dwCheckPoint = 0;
        lpService->Status.dwWaitHint = 0;

        if (lpService->Status.dwCurrentState == SERVICE_STOP_PENDING)
            lpService->Status.dwWin32ExitCode = ERROR_SUCCESS;
        else
            lpService->Status.dwWin32ExitCode = ERROR_GEN_FAILURE;
    }

    if (lpServiceStatus != NULL)
    {
        memcpy(lpServiceStatus,
               &lpService->Status,
               sizeof(SERVICE_STATUS));
    }

    DPRINT1("ScmGetDriverStatus() done (Error: %lu)\n", dwError);

    return ERROR_SUCCESS;
}


DWORD
ScmControlDriver(PSERVICE lpService,
                 DWORD dwControl,
                 LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    DPRINT("ScmControlDriver() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            if (lpService->Status.dwCurrentState != SERVICE_RUNNING)
            {
                dwError = ERROR_INVALID_SERVICE_CONTROL;
                goto done;
            }

            dwError = ScmUnloadDriver(lpService);
            if (dwError == ERROR_SUCCESS)
            {
                lpService->Status.dwControlsAccepted = 0;
                lpService->Status.dwCurrentState = SERVICE_STOPPED;
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            dwError = ScmGetDriverStatus(lpService,
                                         lpServiceStatus);
            break;

        default:
            dwError = ERROR_INVALID_SERVICE_CONTROL;
    }

done:;
    DPRINT("ScmControlDriver() done (Erorr: %lu)\n", dwError);

    return dwError;
}

/* EOF */
