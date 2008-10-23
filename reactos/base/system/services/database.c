/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/database.c
 * PURPOSE:     Database control interface
 * COPYRIGHT:   Copyright 2002-2006 Eric Kohl
 *              Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *                             Gregor Brunmar <gregor.brunmar@home.se>
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

LIST_ENTRY ServiceListHead;

static RTL_RESOURCE DatabaseLock;
static DWORD dwResumeCount = 1;


/* FUNCTIONS *****************************************************************/


PSERVICE
ScmGetServiceEntryByName(LPWSTR lpServiceName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpServiceName, lpServiceName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpServiceName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByDisplayName(LPWSTR lpDisplayName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByDisplayName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpDisplayName, lpDisplayName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByResumeCount(DWORD dwResumeCount)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByResumeCount() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (CurrentService->dwResumeCount > dwResumeCount)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByClientHandle(HANDLE Handle)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByClientHandle() called\n");
    DPRINT("looking for %lu\n", Handle);

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if (CurrentService->hClient == Handle)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


DWORD
ScmCreateNewServiceRecord(LPWSTR lpServiceName,
                          PSERVICE *lpServiceRecord)
{
    PSERVICE lpService = NULL;

    DPRINT("Service: '%S'\n", lpServiceName);

    /* Allocate service entry */
    lpService = (SERVICE*) HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof(SERVICE) + ((wcslen(lpServiceName) + 1) * sizeof(WCHAR)));
    if (lpService == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    *lpServiceRecord = lpService;

    /* Copy service name */
    wcscpy(lpService->szServiceName, lpServiceName);
    lpService->lpServiceName = lpService->szServiceName;
    lpService->lpDisplayName = lpService->lpServiceName;

    /* Set the resume count */
    lpService->dwResumeCount = dwResumeCount++;

    /* Append service record */
    InsertTailList(&ServiceListHead,
                   &lpService->ServiceListEntry);

    /* Initialize the service status */
    lpService->Status.dwCurrentState = SERVICE_STOPPED;
    lpService->Status.dwControlsAccepted = 0;
    lpService->Status.dwWin32ExitCode = ERROR_SERVICE_NEVER_STARTED;
    lpService->Status.dwServiceSpecificExitCode = 0;
    lpService->Status.dwCheckPoint = 0;
    lpService->Status.dwWaitHint = 2000; /* 2 seconds */

    return ERROR_SUCCESS;
}


VOID
ScmDeleteServiceRecord(PSERVICE lpService)
{
    DPRINT1("Deleting Service %S\n", lpService->lpServiceName);

    /* Delete the display name */
    if (lpService->lpDisplayName != NULL &&
        lpService->lpDisplayName != lpService->lpServiceName)
        HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

    /* Decrement the image reference counter */
    if (lpService->lpImage)
        lpService->lpImage->dwServiceRefCount--;

    /* Decrement the group reference counter */
    if (lpService->lpGroup)
        lpService->lpGroup->dwRefCount--;

    /* FIXME: SecurityDescriptor */

    /* Close the control pipe */
    if (lpService->ControlPipeHandle != INVALID_HANDLE_VALUE)
        CloseHandle(lpService->ControlPipeHandle);

    /* Remove the Service from the List */
    RemoveEntryList(&lpService->ServiceListEntry);

    DPRINT1("Deleted Service %S\n", lpService->lpServiceName);

    /* Delete the service record */
    HeapFree(GetProcessHeap(), 0, lpService);

    DPRINT1("Done\n");
}


static DWORD
CreateServiceListEntry(LPWSTR lpServiceName,
                       HKEY hServiceKey)
{
    PSERVICE lpService = NULL;
    LPWSTR lpDisplayName = NULL;
    LPWSTR lpGroup = NULL;
    DWORD dwSize;
    DWORD dwError;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    DWORD dwTagId;

    DPRINT("Service: '%S'\n", lpServiceName);
    if (*lpServiceName == L'{')
        return ERROR_SUCCESS;

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"Type",
                               NULL,
                               NULL,
                               (LPBYTE)&dwServiceType,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    if (((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_OWN_PROCESS) &&
        ((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_SHARE_PROCESS) &&
        (dwServiceType != SERVICE_KERNEL_DRIVER) &&
        (dwServiceType != SERVICE_FILE_SYSTEM_DRIVER))
        return ERROR_SUCCESS;

    DPRINT("Service type: %lx\n", dwServiceType);

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"Start",
                               NULL,
                               NULL,
                               (LPBYTE)&dwStartType,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Start type: %lx\n", dwStartType);

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hServiceKey,
                               L"ErrorControl",
                               NULL,
                               NULL,
                               (LPBYTE)&dwErrorControl,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Error control: %lx\n", dwErrorControl);

    dwError = RegQueryValueExW(hServiceKey,
                               L"Tag",
                               NULL,
                               NULL,
                               (LPBYTE)&dwTagId,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        dwTagId = 0;

    DPRINT("Tag: %lx\n", dwTagId);

    dwError = ScmReadString(hServiceKey,
                            L"Group",
                            &lpGroup);
    if (dwError != ERROR_SUCCESS)
        lpGroup = NULL;

    DPRINT("Group: %S\n", lpGroup);

    dwError = ScmReadString(hServiceKey,
                            L"DisplayName",
                            &lpDisplayName);
    if (dwError != ERROR_SUCCESS)
        lpDisplayName = NULL;

    DPRINT("Display name: %S\n", lpDisplayName);

    dwError = ScmCreateNewServiceRecord(lpServiceName,
                                        &lpService);
    if (dwError != ERROR_SUCCESS)
        goto done;

    lpService->Status.dwServiceType = dwServiceType;
    lpService->dwStartType = dwStartType;
    lpService->dwErrorControl = dwErrorControl;
    lpService->dwTag = dwTagId;

    if (lpGroup != NULL)
    {
        dwError = ScmSetServiceGroup(lpService, lpGroup);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (lpDisplayName != NULL)
    {
        lpService->lpDisplayName = lpDisplayName;
        lpDisplayName = NULL;
    }

    DPRINT("ServiceName: '%S'\n", lpService->lpServiceName);
    if (lpService->lpGroup != NULL)
    {
        DPRINT("Group: '%S'\n", lpService->lpGroup->lpGroupName);
    }
    DPRINT("Start %lx  Type %lx  Tag %lx  ErrorControl %lx\n",
           lpService->dwStartType,
           lpService->Status.dwServiceType,
           lpService->dwTag,
           lpService->dwErrorControl);

    if (ScmIsDeleteFlagSet(hServiceKey))
        lpService->bDeleted = TRUE;

done:;
    if (lpGroup != NULL)
        HeapFree(GetProcessHeap(), 0, lpGroup);

    if (lpDisplayName != NULL)
        HeapFree(GetProcessHeap(), 0, lpDisplayName);

    return dwError;
}


DWORD
ScmDeleteRegKey(HKEY hKey, LPCWSTR lpszSubKey)
{
    DWORD dwRet, dwMaxSubkeyLen = 0, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = 0;

    dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
    if (!dwRet)
    {
        /* Find the maximum subkey length so that we can allocate a buffer */
        dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
                                                         &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
        if (!dwRet)
        {
            dwMaxSubkeyLen++;
            if (dwMaxSubkeyLen > sizeof(szNameBuf)/sizeof(WCHAR))
                /* Name too big: alloc a buffer for it */
                lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(WCHAR));

            if(!lpszName)
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                while (dwRet == ERROR_SUCCESS)
                {
                    dwSize = dwMaxSubkeyLen;
                    dwRet = RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL, NULL, NULL, NULL);
                    if (dwRet == ERROR_SUCCESS || dwRet == ERROR_MORE_DATA)
                        dwRet = ScmDeleteRegKey(hSubKey, lpszName);
                }
                if (dwRet == ERROR_NO_MORE_ITEMS)
                    dwRet = ERROR_SUCCESS;

                if (lpszName != szNameBuf)
                    HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
            }
        }

        RegCloseKey(hSubKey);
        if (!dwRet)
            dwRet = RegDeleteKeyW(hKey, lpszSubKey);
    }
    return dwRet;
}


VOID
ScmDeleteMarkedServices(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    HKEY hServicesKey;
    DWORD dwError;

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        ServiceEntry = ServiceEntry->Flink;

        if (CurrentService->bDeleted == TRUE)
        {
            dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    L"System\\CurrentControlSet\\Services",
                                    0,
                                    DELETE,
                                    &hServicesKey);
            if (dwError == ERROR_SUCCESS)
            {
                dwError = ScmDeleteRegKey(hServicesKey, CurrentService->lpServiceName);
                RegCloseKey(hServicesKey);
                if (dwError == ERROR_SUCCESS)
                {
                    RemoveEntryList(&CurrentService->ServiceListEntry);
                    HeapFree(GetProcessHeap(), 0, CurrentService);
                }
            }
            
            if (dwError != ERROR_SUCCESS)
                DPRINT1("Delete service failed: %S\n", CurrentService->lpServiceName);
        }
    }
}


DWORD
ScmCreateServiceDatabase(VOID)
{
    WCHAR szSubKey[MAX_PATH];
    HKEY hServicesKey;
    HKEY hServiceKey;
    DWORD dwSubKey;
    DWORD dwSubKeyLength;
    FILETIME ftLastChanged;
    DWORD dwError;

    DPRINT("ScmCreateServiceDatabase() called\n");

    dwError = ScmCreateGroupList();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Initialize basic variables */
    InitializeListHead(&ServiceListHead);

    /* Initialize the database lock */
    RtlInitializeResource(&DatabaseLock);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwSubKey = 0;
    for (;;)
    {
        dwSubKeyLength = MAX_PATH;
        dwError = RegEnumKeyExW(hServicesKey,
                                dwSubKey,
                                szSubKey,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                &ftLastChanged);
        if (dwError == ERROR_SUCCESS &&
            szSubKey[0] != L'{')
        {
            DPRINT("SubKeyName: '%S'\n", szSubKey);

            dwError = RegOpenKeyExW(hServicesKey,
                                    szSubKey,
                                    0,
                                    KEY_READ,
                                    &hServiceKey);
            if (dwError == ERROR_SUCCESS)
            {
                dwError = CreateServiceListEntry(szSubKey,
                                                 hServiceKey);

                RegCloseKey(hServiceKey);
            }
        }

        if (dwError != ERROR_SUCCESS)
            break;

        dwSubKey++;
    }

    RegCloseKey(hServicesKey);

    /* Delete services that are marked for delete */
    ScmDeleteMarkedServices();

    DPRINT("ScmCreateServiceDatabase() done\n");

    return ERROR_SUCCESS;
}


VOID
ScmShutdownServiceDatabase(VOID)
{
    DPRINT("ScmShutdownServiceDatabase() called\n");

    ScmDeleteMarkedServices();
    RtlDeleteResource(&DatabaseLock);

    DPRINT("ScmShutdownServiceDatabase() done\n");
}


static NTSTATUS
ScmCheckDriver(PSERVICE Service)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE DirHandle;
    NTSTATUS Status;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG BufferLength;
    ULONG DataLength;
    ULONG Index;

    DPRINT("ScmCheckDriver(%S) called\n", Service->lpServiceName);

    if (Service->Status.dwServiceType == SERVICE_KERNEL_DRIVER)
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
        return Status;
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
            /* FIXME: Add current service to 'failed service' list */
            DPRINT("Service '%S' failed\n", Service->lpServiceName);
            break;
        }

        if (!NT_SUCCESS(Status))
            break;

        DPRINT("Comparing: '%S'  '%wZ'\n", Service->lpServiceName, &DirInfo->Name);

        if (_wcsicmp(Service->lpServiceName, DirInfo->Name.Buffer) == 0)
        {
            DPRINT("Found: '%S'  '%wZ'\n",
                   Service->lpServiceName, &DirInfo->Name);

            /* Mark service as 'running' */
            Service->Status.dwCurrentState = SERVICE_RUNNING;

            /* Mark the service group as 'running' */
            if (Service->lpGroup != NULL)
            {
                Service->lpGroup->ServicesRunning = TRUE;
            }

            break;
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             DirInfo);
    NtClose(DirHandle);

    return STATUS_SUCCESS;
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetBootAndSystemDriverState() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if (CurrentService->dwStartType == SERVICE_BOOT_START ||
            CurrentService->dwStartType == SERVICE_SYSTEM_START)
        {
            /* Check driver */
            DPRINT("  Checking service: %S\n", CurrentService->lpServiceName);

            ScmCheckDriver(CurrentService);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("ScmGetBootAndSystemDriverState() done\n");
}


DWORD
ScmControlService(PSERVICE Service,
                  DWORD dwControl,
                  LPSERVICE_STATUS lpServiceStatus)
{
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD Count;
    DWORD TotalLength;

    DPRINT("ScmControlService() called\n");

    TotalLength = wcslen(Service->lpServiceName) + 1;

    ControlPacket = (SCM_CONTROL_PACKET*)HeapAlloc(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof(SCM_CONTROL_PACKET) + (TotalLength * sizeof(WCHAR)));
    if (ControlPacket == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    ControlPacket->dwControl = dwControl;
    ControlPacket->hClient = Service->hClient;
    ControlPacket->dwSize = TotalLength;
    wcscpy(&ControlPacket->szArguments[0], Service->lpServiceName);

    /* Send the start command */
    WriteFile(Service->ControlPipeHandle,
              ControlPacket,
              sizeof(SCM_CONTROL_PACKET) + (TotalLength * sizeof(WCHAR)),
              &Count,
              NULL);

    /* FIXME: Read the reply */

    /* Release the contol packet */
    HeapFree(GetProcessHeap(),
             0,
             ControlPacket);

    RtlCopyMemory(lpServiceStatus,
                  &Service->Status,
                  sizeof(SERVICE_STATUS));

    DPRINT("ScmControlService) done\n");

    return ERROR_SUCCESS;
}


static DWORD
ScmSendStartCommand(PSERVICE Service,
                    DWORD argc,
                    LPWSTR *argv)
{
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD TotalLength;
    DWORD ArgsLength = 0;
    DWORD Length;
    PWSTR Ptr;
    DWORD Count;

    DPRINT("ScmSendStartCommand() called\n");

    /* Calculate the total length of the start command line */
    TotalLength = wcslen(Service->lpServiceName) + 1;
    if (argc > 0)
    {
        for (Count = 0; Count < argc; Count++)
        {
            DPRINT("Arg: %S\n", argv[Count]);
            Length = wcslen(argv[Count]) + 1;
            TotalLength += Length;
            ArgsLength += Length;
        }
    }
    TotalLength++;
    DPRINT("ArgsLength: %ld TotalLength: %ld\n", ArgsLength, TotalLength);

    /* Allocate a control packet */
    ControlPacket = (SCM_CONTROL_PACKET*)HeapAlloc(GetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof(SCM_CONTROL_PACKET) + (TotalLength - 1) * sizeof(WCHAR));
    if (ControlPacket == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    ControlPacket->dwControl = SERVICE_CONTROL_START;
    ControlPacket->hClient = Service->hClient;
    ControlPacket->dwSize = TotalLength;
    Ptr = &ControlPacket->szArguments[0];
    wcscpy(Ptr, Service->lpServiceName);
    Ptr += (wcslen(Service->lpServiceName) + 1);

    /* Copy argument list */
    if (argc > 0)
    {
        UNIMPLEMENTED;
        DPRINT1("Arguments sent to service ignored!\n");
#if 0
        memcpy(Ptr, Arguments, ArgsLength);
        Ptr += ArgsLength;
#endif
    }

    /* Terminate the argument list */
    *Ptr = 0;

    /* Send the start command */
    WriteFile(Service->ControlPipeHandle,
              ControlPacket,
              sizeof(SCM_CONTROL_PACKET) + (TotalLength - 1) * sizeof(WCHAR),
              &Count,
              NULL);

    /* FIXME: Read the reply */

    /* Release the contol packet */
    HeapFree(GetProcessHeap(),
             0,
             ControlPacket);

    DPRINT("ScmSendStartCommand() done\n");

    return ERROR_SUCCESS;
}


static DWORD
ScmStartUserModeService(PSERVICE Service,
                        DWORD argc,
                        LPWSTR *argv)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    UNICODE_STRING ImagePath;
    ULONG Type;
    DWORD ServiceCurrent = 0;
    BOOL Result;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;
    WCHAR NtControlPipeName[MAX_PATH + 1];
    HKEY hServiceCurrentKey = INVALID_HANDLE_VALUE;
    DWORD KeyDisposition;

    RtlInitUnicodeString(&ImagePath, NULL);

    /* Get service data */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"Type";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].EntryContext = &Type;

    QueryTable[1].Name = L"ImagePath";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].EntryContext = &ImagePath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    Service->lpServiceName,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
        return RtlNtStatusToDosError(Status);
    }
    DPRINT("ImagePath: '%S'\n", ImagePath.Buffer);
    DPRINT("Type: %lx\n", Type);

    /* Get the service number */
    /* TODO: Create registry entry with correct write access */
    Status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control\\ServiceCurrent", 0, NULL,
                            REG_OPTION_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &hServiceCurrentKey,
                            &KeyDisposition);

    if (ERROR_SUCCESS != Status)
    {
        DPRINT1("RegCreateKeyEx() failed with status %u\n", Status);
        return Status;
    }

    if (REG_OPENED_EXISTING_KEY == KeyDisposition)
    {
        DWORD KeySize = sizeof(ServiceCurrent);
        Status = RegQueryValueExW(hServiceCurrentKey, L"", 0, NULL, (BYTE*)&ServiceCurrent, &KeySize);

        if (ERROR_SUCCESS != Status)
        {
            RegCloseKey(hServiceCurrentKey);
            DPRINT1("RegQueryValueEx() failed with status %u\n", Status);
            return Status;
        }

        ServiceCurrent++;
    }

    Status = RegSetValueExW(hServiceCurrentKey, L"", 0, REG_DWORD, (BYTE*)&ServiceCurrent, sizeof(ServiceCurrent));

    RegCloseKey(hServiceCurrentKey);

    if (ERROR_SUCCESS != Status)
    {
        DPRINT1("RegSetValueExW() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create '\\.\pipe\net\NtControlPipeXXX' instance */
    swprintf(NtControlPipeName, L"\\\\.\\pipe\\net\\NtControlPipe%u", ServiceCurrent);
    Service->ControlPipeHandle = CreateNamedPipeW(NtControlPipeName,
                                                  PIPE_ACCESS_DUPLEX,
                                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                                  100,
                                                  8000,
                                                  4,
                                                  30000,
                                                  NULL);
    DPRINT("CreateNamedPipeW(%S) done\n", NtControlPipeName);
    if (Service->ControlPipeHandle == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Failed to create control pipe!\n");
        return GetLastError();
    }

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;

    Result = CreateProcessW(NULL,
                            ImagePath.Buffer,
                            NULL,
                            NULL,
                            FALSE,
                            DETACHED_PROCESS | CREATE_SUSPENDED,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation);
    RtlFreeUnicodeString(&ImagePath);

    if (!Result)
    {
        dwError = GetLastError();
        /* Close control pipe */
        CloseHandle(Service->ControlPipeHandle);
        Service->ControlPipeHandle = INVALID_HANDLE_VALUE;

        DPRINT1("Starting '%S' failed!\n", Service->lpServiceName);
        return dwError;
    }

    DPRINT("Process Id: %lu  Handle %lx\n",
           ProcessInformation.dwProcessId,
           ProcessInformation.hProcess);
    DPRINT("Thread Id: %lu  Handle %lx\n",
           ProcessInformation.dwThreadId,
           ProcessInformation.hThread);

    /* Get process and thread ids */
    Service->ProcessId = ProcessInformation.dwProcessId;
    Service->ThreadId = ProcessInformation.dwThreadId;

    /* Resume Thread */
    ResumeThread(ProcessInformation.hThread);

    /* Connect control pipe */
    if (ConnectNamedPipe(Service->ControlPipeHandle, NULL) ?
        TRUE : (dwError = GetLastError()) == ERROR_PIPE_CONNECTED)
    {
        DWORD dwRead = 0;

        DPRINT("Control pipe connected!\n");

        /* Read SERVICE_STATUS_HANDLE from pipe */
        if (!ReadFile(Service->ControlPipeHandle,
                      (LPVOID)&Service->hClient,
                      sizeof(DWORD),
                      &dwRead,
                      NULL))
        {
            dwError = GetLastError();
            DPRINT1("Reading the service control pipe failed (Error %lu)\n",
                    dwError);
        }
        else
        {
            DPRINT("Received service status %lu\n", Service->hClient);

            /* Send start command */
            dwError = ScmSendStartCommand(Service, argc, argv);
        }
    }
    else
    {
        DPRINT1("Connecting control pipe failed! (Error %lu)\n", dwError);

        /* Close control pipe */
        CloseHandle(Service->ControlPipeHandle);
        Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
        Service->ProcessId = 0;
        Service->ThreadId = 0;
    }

    /* Close process and thread handle */
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return dwError;
}


DWORD
ScmStartService(PSERVICE Service, DWORD argc, LPWSTR *argv)
{
    PSERVICE_GROUP Group = Service->lpGroup;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("ScmStartService() called\n");

    Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
    DPRINT("Service->Type: %lu\n", Service->Status.dwServiceType);

    if (Service->Status.dwServiceType & SERVICE_DRIVER)
    {
        /* Load driver */
        dwError = ScmLoadDriver(Service);
        if (dwError == ERROR_SUCCESS)
            Service->Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }
    else
    {
        /* Start user-mode service */
        dwError = ScmStartUserModeService(Service, argc, argv);
    }

    DPRINT("ScmStartService() done (Error %lu)\n", dwError);

    if (dwError == ERROR_SUCCESS)
    {
        if (Group != NULL)
        {
            Group->ServicesRunning = TRUE;
        }
        Service->Status.dwCurrentState = SERVICE_RUNNING;
    }
#if 0
    else
    {
        switch (Service->ErrorControl)
        {
            case SERVICE_ERROR_NORMAL:
                /* FIXME: Log error */
                break;

            case SERVICE_ERROR_SEVERE:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                break;

            case SERVICE_ERROR_CRITICAL:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                else
                {
                    /* FIXME: BSOD! */
                }
                break;
        }
    }
#endif

    return dwError;
}


VOID
ScmAutoStartServices(VOID)
{
    PLIST_ENTRY GroupEntry;
    PLIST_ENTRY ServiceEntry;
    PSERVICE_GROUP CurrentGroup;
    PSERVICE CurrentService;
    ULONG i;

    /* Clear 'ServiceVisited' flag */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
      CurrentService->ServiceVisited = FALSE;
      ServiceEntry = ServiceEntry->Flink;
    }

    /* Start all services which are members of an existing group */
    GroupEntry = GroupListHead.Flink;
    while (GroupEntry != &GroupListHead)
    {
        CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

        DPRINT("Group '%S'\n", CurrentGroup->lpGroupName);

        /* Start all services witch have a valid tag */
        for (i = 0; i < CurrentGroup->TagCount; i++)
        {
            ServiceEntry = ServiceListHead.Flink;
            while (ServiceEntry != &ServiceListHead)
            {
                CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

                if ((CurrentService->lpGroup == CurrentGroup) &&
                    (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                    (CurrentService->ServiceVisited == FALSE) &&
                    (CurrentService->dwTag == CurrentGroup->TagArray[i]))
                {
                    CurrentService->ServiceVisited = TRUE;
                    ScmStartService(CurrentService, 0, NULL);
                }

                ServiceEntry = ServiceEntry->Flink;
             }
        }

        /* Start all services which have an invalid tag or which do not have a tag */
        ServiceEntry = ServiceListHead.Flink;
        while (ServiceEntry != &ServiceListHead)
        {
            CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

            if ((CurrentService->lpGroup == CurrentGroup) &&
                (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                (CurrentService->ServiceVisited == FALSE))
            {
                CurrentService->ServiceVisited = TRUE;
                ScmStartService(CurrentService, 0, NULL);
            }

            ServiceEntry = ServiceEntry->Flink;
        }

        GroupEntry = GroupEntry->Flink;
    }

    /* Start all services which are members of any non-existing group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if ((CurrentService->lpGroup != NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            CurrentService->ServiceVisited = TRUE;
            ScmStartService(CurrentService, 0, NULL);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* Start all services which are not a member of any group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if ((CurrentService->lpGroup == NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            CurrentService->ServiceVisited = TRUE;
            ScmStartService(CurrentService, 0, NULL);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* Clear 'ServiceVisited' flag again */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
        CurrentService->ServiceVisited = FALSE;
        ServiceEntry = ServiceEntry->Flink;
    }
}


VOID
ScmAutoShutdownServices(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    SERVICE_STATUS ServiceStatus;

    DPRINT("ScmAutoShutdownServices() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if (CurrentService->Status.dwCurrentState == SERVICE_RUNNING ||
            CurrentService->Status.dwCurrentState == SERVICE_START_PENDING)
        {
            /* shutdown service */
            ScmControlService(CurrentService, SERVICE_CONTROL_STOP, &ServiceStatus);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("ScmGetBootAndSystemDriverState() done\n");
}

/* EOF */
