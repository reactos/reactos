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

#include <ndk/iofuncs.h>
#include <ndk/setypes.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

DWORD
ScmLoadDriver(PSERVICE lpService)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasPrivilegeEnabled = FALSE;
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;

    /* Build the driver path */
    /* 52 = wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") */
    pszDriverPath = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              (52 + wcslen(lpService->lpServiceName) + 1) * sizeof(WCHAR));
    if (pszDriverPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(pszDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(pszDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         pszDriverPath);

    DPRINT("  Path: %wZ\n", &DriverPath);

    /* Acquire driver-loading privilege */
    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasPrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        /* We encountered a failure, exit properly */
        DPRINT1("SERVICES: Cannot acquire driver-loading privilege, Status = 0x%08lx\n", Status);
        goto done;
    }

    Status = NtLoadDriver(&DriverPath);

    /* Release driver-loading privilege */
    RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                       WasPrivilegeEnabled,
                       FALSE,
                       &WasPrivilegeEnabled);

done:
    HeapFree(GetProcessHeap(), 0, pszDriverPath);
    return RtlNtStatusToDosError(Status);
}


DWORD
ScmUnloadDriver(PSERVICE lpService)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasPrivilegeEnabled = FALSE;
    PWSTR pszDriverPath;
    UNICODE_STRING DriverPath;

    /* Build the driver path */
    /* 52 = wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") */
    pszDriverPath = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              (52 + wcslen(lpService->lpServiceName) + 1) * sizeof(WCHAR));
    if (pszDriverPath == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy(pszDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(pszDriverPath,
           lpService->lpServiceName);

    RtlInitUnicodeString(&DriverPath,
                         pszDriverPath);

    DPRINT("  Path: %wZ\n", &DriverPath);

    /* Acquire driver-unloading privilege */
    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &WasPrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        /* We encountered a failure, exit properly */
        DPRINT1("SERVICES: Cannot acquire driver-unloading privilege, Status = 0x%08lx\n", Status);
        goto done;
    }

    Status = NtUnloadDriver(&DriverPath);

    /* Release driver-unloading privilege */
    RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                       WasPrivilegeEnabled,
                       FALSE,
                       &WasPrivilegeEnabled);

done:
    HeapFree(GetProcessHeap(), 0, pszDriverPath);
    return RtlNtStatusToDosError(Status);
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

    /* Zero output buffer if any */
    if (lpServiceStatus != NULL)
    {
        memset(lpServiceStatus, 0, sizeof(SERVICE_STATUS));
    }

    /* Select the appropriate object directory based on driver type */
    if (lpService->Status.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        RtlInitUnicodeString(&DirName, L"\\Driver");
    }
    else // if (lpService->Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER)
    {
        ASSERT(lpService->Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER);
        RtlInitUnicodeString(&DirName, L"\\FileSystem");
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               0,
                               NULL,
                               NULL);

    /* Open the object directory where loaded drivers are */
    Status = NtOpenDirectoryObject(&DirHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenDirectoryObject() failed!\n");
        return RtlNtStatusToDosError(Status);
    }

    /* Allocate a buffer big enough for querying the object */
    BufferLength = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                   2 * MAX_PATH * sizeof(WCHAR);
    DirInfo = (OBJECT_DIRECTORY_INFORMATION*) HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        BufferLength);

    /* Now, start browsing entry by entry */
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
        /* End of enumeration, the driver was not found */
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            DPRINT("No more services\n");
            break;
        }

        /* Other error, fail */
        if (!NT_SUCCESS(Status))
            break;

        DPRINT("Comparing: '%S'  '%wZ'\n", lpService->lpServiceName, &DirInfo->Name);

        /* Compare names to check whether it matches our driver */
        if (_wcsicmp(lpService->lpServiceName, DirInfo->Name.Buffer) == 0)
        {
            /* That's our driver, bail out! */
            DPRINT1("Found: '%S'  '%wZ'\n",
                    lpService->lpServiceName, &DirInfo->Name);
            bFound = TRUE;

            break;
        }
    }

    /* Release resources we don't need */
    HeapFree(GetProcessHeap(),
             0,
             DirInfo);
    NtClose(DirHandle);

    /* Only quit if there's a failure
     * Not having found the driver is legit!
     * It means the driver was registered as a service, but not loaded
     * We have not to fail in that situation, but to return proper status
     */
    if (!NT_SUCCESS(Status) && Status != STATUS_NO_MORE_ENTRIES)
    {
        DPRINT1("Status: %lx\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    /* Now, we have two cases:
     * We found the driver: it means it's running
     * We didn't find the driver: it wasn't running
     */
    if ((bFound != FALSE) &&
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
    /* Not found, return it's stopped */
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

    /* Copy service status if required */
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
