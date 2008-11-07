/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/rpcserver.c
 * PURPOSE:     RPC server interface for the advapi32 calls
 * COPYRIGHT:   Copyright 2005-2006 Eric Kohl
 *              Copyright 2006-2007 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 */

/* INCLUDES ****************************************************************/

#include "services.h"
#include "svcctl_s.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

#define MANAGER_TAG 0x72674D68  /* 'hMgr' */
#define SERVICE_TAG 0x63765368  /* 'hSvc' */

typedef struct _SCMGR_HANDLE
{
    DWORD Tag;
    DWORD RefCount;
    DWORD DesiredAccess;
} SCMGR_HANDLE;


typedef struct _MANAGER_HANDLE
{
    SCMGR_HANDLE Handle;

    /* FIXME: Insert more data here */

    WCHAR DatabaseName[1];
} MANAGER_HANDLE, *PMANAGER_HANDLE;


typedef struct _SERVICE_HANDLE
{
    SCMGR_HANDLE Handle;

    DWORD DesiredAccess;
    PSERVICE ServiceEntry;

    /* FIXME: Insert more data here */

} SERVICE_HANDLE, *PSERVICE_HANDLE;


#define SC_MANAGER_READ \
  (STANDARD_RIGHTS_READ | \
   SC_MANAGER_QUERY_LOCK_STATUS | \
   SC_MANAGER_ENUMERATE_SERVICE)

#define SC_MANAGER_WRITE \
  (STANDARD_RIGHTS_WRITE | \
   SC_MANAGER_MODIFY_BOOT_CONFIG | \
   SC_MANAGER_CREATE_SERVICE)

#define SC_MANAGER_EXECUTE \
  (STANDARD_RIGHTS_EXECUTE | \
   SC_MANAGER_LOCK | \
   SC_MANAGER_ENUMERATE_SERVICE | \
   SC_MANAGER_CONNECT | \
   SC_MANAGER_CREATE_SERVICE)


#define SERVICE_READ \
  (STANDARD_RIGHTS_READ | \
   SERVICE_INTERROGATE | \
   SERVICE_ENUMERATE_DEPENDENTS | \
   SERVICE_QUERY_STATUS | \
   SERVICE_QUERY_CONFIG)

#define SERVICE_WRITE \
  (STANDARD_RIGHTS_WRITE | \
   SERVICE_CHANGE_CONFIG)

#define SERVICE_EXECUTE \
  (STANDARD_RIGHTS_EXECUTE | \
   SERVICE_USER_DEFINED_CONTROL | \
   SERVICE_PAUSE_CONTINUE | \
   SERVICE_STOP | \
   SERVICE_START)


/* VARIABLES ***************************************************************/

static GENERIC_MAPPING
ScmManagerMapping = {SC_MANAGER_READ,
                     SC_MANAGER_WRITE,
                     SC_MANAGER_EXECUTE,
                     SC_MANAGER_ALL_ACCESS};

static GENERIC_MAPPING
ScmServiceMapping = {SERVICE_READ,
                     SERVICE_WRITE,
                     SERVICE_EXECUTE,
                     SC_MANAGER_ALL_ACCESS};


/* FUNCTIONS ***************************************************************/

VOID
ScmStartRpcServer(VOID)
{
    RPC_STATUS Status;

    DPRINT("ScmStartRpcServer() called\n");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\ntsvcs",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(svcctl_v2_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    DPRINT("ScmStartRpcServer() done\n");
}


static DWORD
ScmCreateManagerHandle(LPWSTR lpDatabaseName,
                       SC_HANDLE *Handle)
{
    PMANAGER_HANDLE Ptr;

    if (lpDatabaseName == NULL)
        lpDatabaseName = SERVICES_ACTIVE_DATABASEW;

    if (wcsicmp(lpDatabaseName,SERVICES_FAILED_DATABASEW)==0)
    {
        DPRINT1("Database %S, does not exist\n",lpDatabaseName);
        return ERROR_DATABASE_DOES_NOT_EXIST;
    }
    else if (wcsicmp(lpDatabaseName, SERVICES_ACTIVE_DATABASEW) != 0)
    {
        DPRINT1("Invalid Database name %S.\n",lpDatabaseName);
        return ERROR_INVALID_NAME;
    }

    Ptr = (MANAGER_HANDLE*) HeapAlloc(GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    sizeof(MANAGER_HANDLE) + (wcslen(lpDatabaseName) + 1) * sizeof(WCHAR));
    if (Ptr == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    Ptr->Handle.Tag = MANAGER_TAG;
    Ptr->Handle.RefCount = 1;

    /* FIXME: initialize more data here */

    wcscpy(Ptr->DatabaseName, lpDatabaseName);

    *Handle = (SC_HANDLE)Ptr;

    return ERROR_SUCCESS;
}


static DWORD
ScmCreateServiceHandle(PSERVICE lpServiceEntry,
                       SC_HANDLE *Handle)
{
    PSERVICE_HANDLE Ptr;

    Ptr = (SERVICE_HANDLE*) HeapAlloc(GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    sizeof(SERVICE_HANDLE));
    if (Ptr == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    Ptr->Handle.Tag = SERVICE_TAG;
    Ptr->Handle.RefCount = 1;

    /* FIXME: initialize more data here */
    Ptr->ServiceEntry = lpServiceEntry;

    *Handle = (SC_HANDLE)Ptr;

    return ERROR_SUCCESS;
}


static DWORD
ScmCheckAccess(SC_HANDLE Handle,
               DWORD dwDesiredAccess)
{
    PMANAGER_HANDLE hMgr;

    hMgr = (PMANAGER_HANDLE)Handle;
    if (hMgr->Handle.Tag == MANAGER_TAG)
    {
        RtlMapGenericMask(&dwDesiredAccess,
                          &ScmManagerMapping);

        hMgr->Handle.DesiredAccess = dwDesiredAccess;

        return ERROR_SUCCESS;
    }
    else if (hMgr->Handle.Tag == SERVICE_TAG)
    {
        RtlMapGenericMask(&dwDesiredAccess,
                          &ScmServiceMapping);

        hMgr->Handle.DesiredAccess = dwDesiredAccess;

        return ERROR_SUCCESS;
    }

    return ERROR_INVALID_HANDLE;
}


DWORD
ScmAssignNewTag(PSERVICE lpService)
{
    /* FIXME */
    DPRINT("Assigning new tag to service %S\n", lpService->lpServiceName);
    lpService->dwTag = 0;
    return ERROR_SUCCESS;
}


/* Internal recursive function */
/* Need to search for every dependency on every service */
static DWORD
Int_EnumDependentServicesW(HKEY hServicesKey,
                           PSERVICE lpService,
                           DWORD dwServiceState,
                           PSERVICE *lpServices,
                           LPDWORD pcbBytesNeeded,
                           LPDWORD lpServicesReturned)
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szNameBuf[MAX_PATH];
    WCHAR szValueBuf[MAX_PATH];
    WCHAR *lpszNameBuf = szNameBuf;
    WCHAR *lpszValueBuf = szValueBuf;
    DWORD dwSize;
    DWORD dwNumSubKeys;
    DWORD dwIteration;
    PSERVICE lpCurrentService;
    HKEY hServiceEnumKey;
    DWORD dwCurrentServiceState = SERVICE_ACTIVE;
    DWORD dwDependServiceStrPtr = 0;
    DWORD dwRequiredSize = 0;

    /* Get the number of service keys */
    dwError = RegQueryInfoKeyW(hServicesKey,
                               NULL,
                               NULL,
                               NULL,
                               &dwNumSubKeys,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ERROR! Unable to get number of services keys.\n");
        return dwError;
    }

    /* Iterate the service keys to see if another service depends on the this service */
    for (dwIteration = 0; dwIteration < dwNumSubKeys; dwIteration++)
    {
        dwSize = MAX_PATH;
        dwError = RegEnumKeyExW(hServicesKey,
                                dwIteration,
                                lpszNameBuf,
                                &dwSize,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (dwError != ERROR_SUCCESS)
            return dwError;

        /* Open the Service key */
        dwError = RegOpenKeyExW(hServicesKey,
                                lpszNameBuf,
                                0,
                                KEY_READ,
                                &hServiceEnumKey);
        if (dwError != ERROR_SUCCESS)
            return dwError;

        dwSize = MAX_PATH;

        /* Check for the DependOnService Value */
        dwError = RegQueryValueExW(hServiceEnumKey,
                                   L"DependOnService",
                                   NULL,
                                   NULL,
                                   (LPBYTE)lpszValueBuf,
                                   &dwSize);

        /* FIXME: Handle load order. */

        /* If the service found has a DependOnService value */
        if (dwError == ERROR_SUCCESS)
        {
            dwDependServiceStrPtr = 0;

            /* Can be more than one Dependencies in the DependOnService string */
            while (wcslen(lpszValueBuf + dwDependServiceStrPtr) > 0)
            {
                if (wcsicmp(lpszValueBuf + dwDependServiceStrPtr, lpService->lpServiceName) == 0)
                {
                    /* Get the current enumed service pointer */
                    lpCurrentService = ScmGetServiceEntryByName(lpszNameBuf);

                    /* Check for valid Service */
                    if (!lpCurrentService)
                    {
                        /* This should never happen! */
                        DPRINT1("This should not happen at this point, report to Developer\n");
                        return ERROR_NOT_FOUND;
                    }

                    /* Determine state the service is in */
                    if (lpCurrentService->Status.dwCurrentState == SERVICE_STOPPED)
                        dwCurrentServiceState = SERVICE_INACTIVE;

                    /* If the ServiceState matches that requested or searching for SERVICE_STATE_ALL */
                    if ((dwCurrentServiceState == dwServiceState) ||
                        (dwServiceState == SERVICE_STATE_ALL))
                    {
                        /* Calculate the required size */
                        dwRequiredSize += sizeof(SERVICE_STATUS);
                        dwRequiredSize += ((wcslen(lpCurrentService->lpServiceName) + 1) * sizeof(WCHAR));
                        dwRequiredSize += ((wcslen(lpCurrentService->lpDisplayName) + 1) * sizeof(WCHAR));

                        /* Add the size for service name and display name pointers */
                        dwRequiredSize += (2 * sizeof(PVOID));

                        /* increase the BytesNeeded size */
                        *pcbBytesNeeded = *pcbBytesNeeded + dwRequiredSize;

                        /* Don't fill callers buffer yet, as MSDN read that the last service with dependency
                           comes first */

                        /* Recursive call to check for its dependencies */
                        Int_EnumDependentServicesW(hServicesKey,
                                                   lpCurrentService,
                                                   dwServiceState,
                                                   lpServices,
                                                   pcbBytesNeeded,
                                                   lpServicesReturned);

                        /* If the lpServices is valid set the service pointer */
                        if (lpServices)
                            lpServices[*lpServicesReturned] = lpCurrentService;

                        *lpServicesReturned = *lpServicesReturned + 1;
                    }
                }

                dwDependServiceStrPtr += (wcslen(lpszValueBuf + dwDependServiceStrPtr) + 1);
            }
        }
        else if (*pcbBytesNeeded)
        {
            dwError = ERROR_SUCCESS;
        }

        RegCloseKey(hServiceEnumKey);
    }

    return dwError;
}


/* Function 0 */
DWORD RCloseServiceHandle(
    LPSC_RPC_HANDLE hSCObject)
{
    PMANAGER_HANDLE hManager;
    PSERVICE_HANDLE hService;
    PSERVICE lpService;
    HKEY hServicesKey;
    DWORD dwError;
    DWORD pcbBytesNeeded = 0;
    DWORD dwServicesReturned = 0;

    DPRINT("RCloseServiceHandle() called\n");

    DPRINT("hSCObject = %p\n", *hSCObject);

    if (*hSCObject == 0)
        return ERROR_INVALID_HANDLE;

    hManager = (PMANAGER_HANDLE)*hSCObject;
    hService = (PSERVICE_HANDLE)*hSCObject;
    if (hManager->Handle.Tag == MANAGER_TAG)
    {
        DPRINT("Found manager handle\n");

        hManager->Handle.RefCount--;
        if (hManager->Handle.RefCount == 0)
        {
            /* FIXME: add handle cleanup code */

            HeapFree(GetProcessHeap(), 0, hManager);
            hManager = NULL;
        }

        DPRINT("RCloseServiceHandle() done\n");
        return ERROR_SUCCESS;
    }
    else if (hService->Handle.Tag == SERVICE_TAG)
    {
        DPRINT("Found service handle\n");

        /* Get the pointer to the service record */
        lpService = hService->ServiceEntry;

        ASSERT(hService->Handle.RefCount > 0);

        hService->Handle.RefCount--;
        if (hService->Handle.RefCount == 0)
        {
            /* FIXME: add handle cleanup code */

            /* Free the handle */
            HeapFree(GetProcessHeap(), 0, hService);
            hService = NULL;
        }

        ASSERT(lpService->dwRefCount > 0);

        lpService->dwRefCount--;
        DPRINT("CloseServiceHandle - lpService->dwRefCount %u\n",
               lpService->dwRefCount);

        if (lpService->dwRefCount == 0)
        {
            /* If this service has been marked for deletion */
            if (lpService->bDeleted)
            {
                /* Open the Services Reg key */
                dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                        L"System\\CurrentControlSet\\Services",
                                        0,
                                        KEY_SET_VALUE | KEY_READ,
                                        &hServicesKey);
                if (dwError != ERROR_SUCCESS)
                {
                    DPRINT1("Failed to open services key\n");
                    return dwError;
                }

                /* Call the internal function with NULL, just to get bytes we need */
                Int_EnumDependentServicesW(hServicesKey,
                                           lpService,
                                           SERVICE_ACTIVE,
                                           NULL,
                                           &pcbBytesNeeded,
                                           &dwServicesReturned);

                /* if pcbBytesNeeded returned a value then there are services running that are dependent on this service*/
                if (pcbBytesNeeded)
                {
                    DPRINT1("Deletion failed due to running dependencies.\n");
                    RegCloseKey(hServicesKey);
                    return ERROR_SUCCESS;
                }

                /* There are no references and no runnning dependencies,
                   it is now safe to delete the service */

                /* Delete the Service Key */
                dwError = RegDeleteKey(hServicesKey,
                                       lpService->lpServiceName);

                RegCloseKey(hServicesKey);

                if (dwError != ERROR_SUCCESS)
                {
                    DPRINT1("Failed to Delete the Service Registry key\n");
                    return dwError;
                }

                /* Delete the Service */
                ScmDeleteServiceRecord(lpService);
            }
        }

        DPRINT("RCloseServiceHandle() done\n");
        return ERROR_SUCCESS;
    }

    DPRINT1("Invalid handle tag (Tag %lx)\n", hManager->Handle.Tag);

    return ERROR_INVALID_HANDLE;
}


/* Function 1 */
DWORD RControlService(
    SC_RPC_HANDLE hService,
    DWORD dwControl,
    LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ACCESS_MASK DesiredAccess;
    DWORD dwError = ERROR_SUCCESS;
    DWORD pcbBytesNeeded = 0;
    DWORD dwServicesReturned = 0;
    HKEY hServicesKey = NULL;

    DPRINT("RControlService() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    /* Check the service handle */
    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Check the service entry point */
    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n"); 
        return ERROR_INVALID_HANDLE;
    }

    /* Check access rights */
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            DesiredAccess = SERVICE_STOP;
            break;

        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
            DesiredAccess = SERVICE_PAUSE_CONTINUE;
            break;

        case SERVICE_INTERROGATE:
            DesiredAccess = SERVICE_INTERROGATE;
            break;

        default:
            if (dwControl >= 128 && dwControl <= 255)
                DesiredAccess = SERVICE_USER_DEFINED_CONTROL;
            else
                DesiredAccess = SERVICE_QUERY_CONFIG |
                                SERVICE_CHANGE_CONFIG |
                                SERVICE_QUERY_STATUS |
                                SERVICE_START |
                                SERVICE_PAUSE_CONTINUE;
            break;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  DesiredAccess))
        return ERROR_ACCESS_DENIED;

    if (dwControl == SERVICE_CONTROL_STOP)
    {
        /* Check if the service has dependencies running as windows
           doesn't stop a service that does */

        /* Open the Services Reg key */
        dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"System\\CurrentControlSet\\Services",
                                0,
                                KEY_READ,
                                &hServicesKey);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("Failed to open services key\n");
            return dwError;
        }

        /* Call the internal function with NULL, just to get bytes we need */
        Int_EnumDependentServicesW(hServicesKey,
                                   lpService,
                                   SERVICE_ACTIVE,
                                   NULL,
                                   &pcbBytesNeeded,
                                   &dwServicesReturned);

        RegCloseKey(hServicesKey);

        /* If pcbBytesNeeded is not zero then there are services running that
           are dependent on this service */
        if (pcbBytesNeeded != 0)
        {
            DPRINT("Service has running dependencies. Failed to stop service.\n");
            return ERROR_DEPENDENT_SERVICES_RUNNING;
        }
    }

    if (lpService->Status.dwServiceType & SERVICE_DRIVER)
    {
        /* Send control code to the driver */
        dwError = ScmControlDriver(lpService,
                                   dwControl,
                                   lpServiceStatus);
    }
    else
    {
        /* Send control code to the service */
        dwError = ScmControlService(lpService,
                                    dwControl,
                                    lpServiceStatus);
    }

    if ((dwError == ERROR_SUCCESS) && (pcbBytesNeeded))
        dwError = ERROR_DEPENDENT_SERVICES_RUNNING;

    /* Return service status information */
    RtlCopyMemory(lpServiceStatus,
                  &lpService->Status,
                  sizeof(SERVICE_STATUS));

    return dwError;
}


/* Function 2 */
DWORD RDeleteService(
    SC_RPC_HANDLE hService)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    DWORD dwError;

    DPRINT("RDeleteService() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
        return ERROR_INVALID_HANDLE;

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  DELETE))
        return ERROR_ACCESS_DENIED;

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Acquire service database lock exclusively */

    if (lpService->bDeleted)
    {
        DPRINT1("The service has already been marked for delete!\n");
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    /* Mark service for delete */
    lpService->bDeleted = TRUE;

    dwError = ScmMarkServiceForDelete(lpService);

    /* FIXME: Release service database lock */

    DPRINT("RDeleteService() done\n");

    return dwError;
}


/* Function 3 */
DWORD RLockServiceDatabase(
    SC_RPC_HANDLE hSCManager,
    LPSC_RPC_LOCK lpLock)
{
    PMANAGER_HANDLE hMgr;

    DPRINT("RLockServiceDatabase() called\n");

    *lpLock = 0;

    hMgr = (PMANAGER_HANDLE)hSCManager;
    if (!hMgr || hMgr->Handle.Tag != MANAGER_TAG)
        return ERROR_INVALID_HANDLE;

    if (!RtlAreAllAccessesGranted(hMgr->Handle.DesiredAccess,
                                  SC_MANAGER_LOCK))
        return ERROR_ACCESS_DENIED;

//    return ScmLockDatabase(0, hMgr->0xC, hLock);

    /* FIXME: Lock the database */
    *lpLock = (SC_RPC_LOCK)0x12345678; /* Dummy! */

    return ERROR_SUCCESS;
}


/* Function 4 */
DWORD RQueryServiceObjectSecurity(
    SC_RPC_HANDLE hService,
    SECURITY_INFORMATION dwSecurityInformation,
    LPBYTE lpSecurityDescriptor,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded)
{
#if 0
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ULONG DesiredAccess = 0;
    NTSTATUS Status;
    DWORD dwBytesNeeded;
    DWORD dwError;

    DPRINT("RQueryServiceObjectSecurity() called\n");

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (dwSecurityInformation & (DACL_SECURITY_INFORMATION ||
                                 GROUP_SECURITY_INFORMATION ||
                                 OWNER_SECURITY_INFORMATION))
        DesiredAccess |= READ_CONTROL;

    if (dwSecurityInformation & SACL_SECURITY_INFORMATION)
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  DesiredAccess))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service list */

    Status = RtlQuerySecurityObject(lpService->lpSecurityDescriptor,
                                    dwSecurityInformation,
                                    (PSECURITY_DESCRIPTOR)lpSecurityDescriptor,
                                    dwSecuityDescriptorSize,
                                    &dwBytesNeeded);

    /* FIXME: Unlock the service list */

    if (NT_SUCCESS(Status))
    {
        *pcbBytesNeeded = dwBytesNeeded;
        dwError = STATUS_SUCCESS;
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        *pcbBytesNeeded = dwBytesNeeded;
        dwError = ERROR_INSUFFICIENT_BUFFER;
    }
    else if (Status == STATUS_BAD_DESCRIPTOR_FORMAT)
    {
        dwError = ERROR_GEN_FAILURE;
    }
    else
    {
        dwError = RtlNtStatusToDosError(Status);
    }

    return dwError;
#endif
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 5 */
DWORD RSetServiceObjectSecurity(
    SC_RPC_HANDLE hService,
    DWORD dwSecurityInformation,
    LPBYTE lpSecurityDescriptor,
    DWORD dwSecuityDescriptorSize)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ULONG DesiredAccess = 0;
    HANDLE hToken = NULL;
    HKEY hServiceKey;
    NTSTATUS Status;
    DWORD dwError;

    DPRINT1("RSetServiceObjectSecurity() called\n");

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (dwSecurityInformation == 0 ||
        dwSecurityInformation & ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
        | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION))
        return ERROR_INVALID_PARAMETER;

    if (!RtlValidSecurityDescriptor((PSECURITY_DESCRIPTOR)lpSecurityDescriptor))
        return ERROR_INVALID_PARAMETER;

    if (dwSecurityInformation & SACL_SECURITY_INFORMATION)
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;

    if (dwSecurityInformation & DACL_SECURITY_INFORMATION)
        DesiredAccess |= WRITE_DAC;

    if (dwSecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        DesiredAccess |= WRITE_OWNER;

    if ((dwSecurityInformation & OWNER_SECURITY_INFORMATION) &&
        (((PSECURITY_DESCRIPTOR)lpSecurityDescriptor)->Owner == NULL))
        return ERROR_INVALID_PARAMETER;

    if ((dwSecurityInformation & GROUP_SECURITY_INFORMATION) &&
        (((PSECURITY_DESCRIPTOR)lpSecurityDescriptor)->Group == NULL))
        return ERROR_INVALID_PARAMETER;

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  DesiredAccess))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (lpService->bDeleted)
        return ERROR_SERVICE_MARKED_FOR_DELETE;

    RpcImpersonateClient(NULL);

    Status = NtOpenThreadToken(NtCurrentThread(),
                               8,
                               TRUE,
                               &hToken);
    if (!NT_SUCCESS(Status))
        return RtlNtStatusToDosError(Status);

    RpcRevertToSelf();

    /* FIXME: Lock service database */

#if 0
    Status = RtlSetSecurityObject(dwSecurityInformation,
                                  (PSECURITY_DESCRIPTOR)lpSecurityDescriptor,
                                  &lpService->lpSecurityDescriptor,
                                  &ScmServiceMapping,
                                  hToken);
    if (!NT_SUCCESS(Status))
    {
        dwError = RtlNtStatusToDosError(Status);
        goto Done;
    }
#endif

    dwError = ScmOpenServiceKey(lpService->lpServiceName,
                                READ_CONTROL | KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    UNIMPLEMENTED;
    dwError = ERROR_SUCCESS;
//    dwError = ScmWriteSecurityDescriptor(hServiceKey,
//                                         lpService->lpSecurityDescriptor);

    RegFlushKey(hServiceKey);
    RegCloseKey(hServiceKey);

Done:

    if (hToken != NULL)
        NtClose(hToken);

    /* FIXME: Unlock service database */

    DPRINT("RSetServiceObjectSecurity() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 6 */
DWORD RQueryServiceStatus(
    SC_RPC_HANDLE hService,
    LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;

    DPRINT("RQueryServiceStatus() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_STATUS))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Return service status information */
    RtlCopyMemory(lpServiceStatus,
                  &lpService->Status,
                  sizeof(SERVICE_STATUS));

    return ERROR_SUCCESS;
}


static BOOL
ScmIsValidServiceState(DWORD dwCurrentState)
{
    switch (dwCurrentState)
    {
        case SERVICE_STOPPED:
        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
        case SERVICE_RUNNING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            return TRUE;

        default:
            return FALSE;
    }
}


/* Function 7 */
DWORD RSetServiceStatus(
    RPC_SERVICE_STATUS_HANDLE hServiceStatus,
    LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE lpService;

    DPRINT("RSetServiceStatus() called\n");
    DPRINT("hServiceStatus = %p\n", hServiceStatus);
    DPRINT("dwServiceType = %lu\n", lpServiceStatus->dwServiceType);
    DPRINT("dwCurrentState = %lu\n", lpServiceStatus->dwCurrentState);
    DPRINT("dwControlsAccepted = %lu\n", lpServiceStatus->dwControlsAccepted);
    DPRINT("dwWin32ExitCode = %lu\n", lpServiceStatus->dwWin32ExitCode);
    DPRINT("dwServiceSpecificExitCode = %lu\n", lpServiceStatus->dwServiceSpecificExitCode);
    DPRINT("dwCheckPoint = %lu\n", lpServiceStatus->dwCheckPoint);
    DPRINT("dwWaitHint = %lu\n", lpServiceStatus->dwWaitHint);

    if (hServiceStatus == 0)
    {
        DPRINT1("hServiceStatus == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    lpService = ScmGetServiceEntryByClientHandle(hServiceStatus);
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Check current state */
    if (!ScmIsValidServiceState(lpServiceStatus->dwCurrentState))
    {
        DPRINT1("Invalid service state!\n");
        return ERROR_INVALID_DATA;
    }

    /* Check service type */
    if (!(lpServiceStatus->dwServiceType & SERVICE_WIN32) &&
         (lpServiceStatus->dwServiceType & SERVICE_DRIVER))
    {
        DPRINT1("Invalid service type!\n");
        return ERROR_INVALID_DATA;
    }

    /* Check accepted controls */
    if (lpServiceStatus->dwControlsAccepted & ~0xFF)
    {
        DPRINT1("Invalid controls accepted!\n");
        return ERROR_INVALID_DATA;
    }


    RtlCopyMemory(&lpService->Status,
                  lpServiceStatus,
                  sizeof(SERVICE_STATUS));

    DPRINT("Set %S to %lu\n", lpService->lpDisplayName, lpService->Status.dwCurrentState);
    DPRINT("RSetServiceStatus() done\n");

    return ERROR_SUCCESS;
}


/* Function 8 */
DWORD RUnlockServiceDatabase(
    LPSC_RPC_LOCK Lock)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}


/* Function 9 */
DWORD RNotifyBootConfigStatus(
    SVCCTL_HANDLEW lpMachineName,
    DWORD BootAcceptable)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 10 */
DWORD RI_ScSetServiceBitsW(
    RPC_SERVICE_STATUS_HANDLE hServiceStatus,
    DWORD dwServiceBits,
    int bSetBitsOn,
    int bUpdateImmediately,
    wchar_t *lpString)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 11 */
DWORD RChangeServiceConfigW(
    SC_RPC_HANDLE hService,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPWSTR lpBinaryPathName,
    LPWSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPBYTE lpDependencies,
    DWORD dwDependSize,
    LPWSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPWSTR lpDisplayName)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    LPWSTR lpDisplayNameW = NULL;

    DPRINT("RChangeServiceConfigW() called\n");
    DPRINT("dwServiceType = %lu\n", dwServiceType);
    DPRINT("dwStartType = %lu\n", dwStartType);
    DPRINT("dwErrorControl = %lu\n", dwErrorControl);
    DPRINT("lpBinaryPathName = %S\n", lpBinaryPathName);
    DPRINT("lpLoadOrderGroup = %S\n", lpLoadOrderGroup);
    DPRINT("lpDisplayName = %S\n", lpDisplayName);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_CHANGE_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock database exclusively */

    if (lpService->bDeleted)
    {
        /* FIXME: Unlock database */
        DPRINT1("The service has already been marked for delete!\n");
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    /* Open the service key */
    dwError = ScmOpenServiceKey(lpService->szServiceName,
                                KEY_SET_VALUE,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Write service data to the registry */
    /* Set the display name */
    if (lpDisplayName != NULL && *lpDisplayName != 0)
    {
        RegSetValueExW(hServiceKey,
                       L"DisplayName",
                       0,
                       REG_SZ,
                       (LPBYTE)lpDisplayName,
                       (wcslen(lpDisplayName) + 1) * sizeof(WCHAR));

        /* Update the display name */
        lpDisplayNameW = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                           0,
                                           (wcslen(lpDisplayName) + 1) * sizeof(WCHAR));
        if (lpDisplayNameW == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        if (lpService->lpDisplayName != lpService->lpServiceName)
            HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

        lpService->lpDisplayName = lpDisplayNameW;
    }

    if (dwServiceType != SERVICE_NO_CHANGE)
    {
        /* Set the service type */
        dwError = RegSetValueExW(hServiceKey,
                                 L"Type",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwServiceType,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->Status.dwServiceType = dwServiceType;
    }

    if (dwStartType != SERVICE_NO_CHANGE)
    {
        /* Set the start value */
        dwError = RegSetValueExW(hServiceKey,
                                 L"Start",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwStartType,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->dwStartType = dwStartType;
    }

    if (dwErrorControl != SERVICE_NO_CHANGE)
    {
        /* Set the error control value */
        dwError = RegSetValueExW(hServiceKey,
                                 L"ErrorControl",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwErrorControl,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->dwErrorControl = dwErrorControl;
    }

#if 0
    /* FIXME: set the new ImagePath value */

    /* Set the image path */
    if (dwServiceType & SERVICE_WIN32)
    {
        if (lpBinaryPathName != NULL && *lpBinaryPathName != 0)
        {
            dwError = RegSetValueExW(hServiceKey,
                                     L"ImagePath",
                                     0,
                                     REG_EXPAND_SZ,
                                     (LPBYTE)lpBinaryPathName,
                                     (wcslen(lpBinaryPathName) + 1) * sizeof(WCHAR));
            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }
    else if (dwServiceType & SERVICE_DRIVER)
    {
        if (lpImagePath != NULL && *lpImagePath != 0)
        {
            dwError = RegSetValueExW(hServiceKey,
                                     L"ImagePath",
                                     0,
                                     REG_EXPAND_SZ,
                                     (LPBYTE)lpImagePath,
                                     (wcslen(lpImagePath) + 1) *sizeof(WCHAR));
            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }
#endif

    /* Set the group name */
    if (lpLoadOrderGroup != NULL && *lpLoadOrderGroup != 0)
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"Group",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)lpLoadOrderGroup,
                                 (wcslen(lpLoadOrderGroup) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;
        /* FIXME: Update lpService->lpServiceGroup */
    }

    if (lpdwTagId != NULL)
    {
        dwError = ScmAssignNewTag(lpService);
        if (dwError != ERROR_SUCCESS)
            goto done;

        dwError = RegSetValueExW(hServiceKey,
                                 L"Tag",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&lpService->dwTag,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        *lpdwTagId = lpService->dwTag;
    }

    /* Write dependencies */
    if (lpDependencies != NULL && *lpDependencies != 0)
    {
        dwError = ScmWriteDependencies(hServiceKey,
                                       (LPWSTR)lpDependencies,
                                       dwDependSize);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (lpPassword != NULL)
    {
        /* FIXME: Write password */
    }

    /* FIXME: Unlock database */

done:
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    DPRINT("RChangeServiceConfigW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Create a path suitable for the bootloader out of the full path */
DWORD
ScmConvertToBootPathName(wchar_t *CanonName, wchar_t **RelativeName)
{
    DWORD ServiceNameLen, BufferSize, ExpandedLen;
    WCHAR Dest;
    WCHAR *Expanded;
    UNICODE_STRING NtPathName, SystemRoot, LinkTarget;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE SymbolicLinkHandle;

    DPRINT("ScmConvertToBootPathName %S\n", CanonName);

    ServiceNameLen = wcslen(CanonName);

    /* First check, if it's already good */
    if (ServiceNameLen > 12 &&
        !wcsnicmp(L"\\SystemRoot\\", CanonName, 12))
    {
        *RelativeName = LocalAlloc(LMEM_ZEROINIT, ServiceNameLen * sizeof(WCHAR) + sizeof(WCHAR));
        if (*RelativeName == NULL)
        {
            DPRINT1("Error allocating memory for boot driver name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Copy it */
        wcscpy(*RelativeName, CanonName);

        DPRINT1("Bootdriver name %S\n", *RelativeName);
        return ERROR_SUCCESS;
    }

    /* If it has %SystemRoot% prefix, substitute it to \System*/
    if (ServiceNameLen > 13 &&
        !wcsnicmp(L"%SystemRoot%\\", CanonName, 13))
    {
        /* There is no +sizeof(wchar_t) because the name is less by 1 wchar */
        *RelativeName = LocalAlloc(LMEM_ZEROINIT, ServiceNameLen * sizeof(WCHAR));

        if (*RelativeName == NULL)
        {
            DPRINT1("Error allocating memory for boot driver name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Copy it */
        wcscpy(*RelativeName, L"\\SystemRoot\\");
        wcscat(*RelativeName, CanonName + 13);

        DPRINT1("Bootdriver name %S\n", *RelativeName);
        return ERROR_SUCCESS;
    }

    /* Get buffer size needed for expanding env strings */
    BufferSize = ExpandEnvironmentStringsW(L"%SystemRoot%\\", &Dest, 1);

    if (BufferSize <= 1)
    {
        DPRINT1("Error during a call to ExpandEnvironmentStringsW()\n");
        return ERROR_INVALID_ENVIRONMENT;
    }

    /* Allocate memory, since the size is known now */
    Expanded = LocalAlloc(LMEM_ZEROINIT, BufferSize * sizeof(WCHAR) + sizeof(WCHAR));
    if (!Expanded)
    {
        DPRINT1("Error allocating memory for boot driver name!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Expand it */
    if (ExpandEnvironmentStringsW(L"%SystemRoot%\\", Expanded, BufferSize) >
        BufferSize)
    {
        DPRINT1("Error during a call to ExpandEnvironmentStringsW()\n");
        LocalFree(Expanded);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Convert to NY-style path */
    if (!RtlDosPathNameToNtPathName_U(Expanded, &NtPathName, NULL, NULL))
    {
        DPRINT1("Error during a call to RtlDosPathNameToNtPathName_U()\n");
        return ERROR_INVALID_ENVIRONMENT;
    }

    DPRINT("Converted to NT-style %wZ\n", &NtPathName);

    /* No need to keep the dos-path anymore */
    LocalFree(Expanded);

    /* Copy it to the allocated place */
    Expanded = LocalAlloc(LMEM_ZEROINIT, NtPathName.Length + sizeof(WCHAR));
    if (!Expanded)
    {
            DPRINT1("Error allocating memory for boot driver name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    ExpandedLen = NtPathName.Length / sizeof(WCHAR);
    wcsncpy(Expanded, NtPathName.Buffer, ExpandedLen);
    Expanded[ExpandedLen] = 0;

    if (ServiceNameLen > ExpandedLen &&
        !wcsnicmp(Expanded, CanonName, ExpandedLen))
    {
        /* Only \SystemRoot\ is missing */
        *RelativeName = LocalAlloc(LMEM_ZEROINIT,
            (ServiceNameLen - ExpandedLen) * sizeof(WCHAR) + 13*sizeof(WCHAR));
        if (*RelativeName == NULL)
        {
            DPRINT1("Error allocating memory for boot driver name!\n");
            LocalFree(Expanded);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(*RelativeName, L"\\SystemRoot\\");
        wcscat(*RelativeName, CanonName + ExpandedLen);

        RtlFreeUnicodeString(&NtPathName);
        return ERROR_SUCCESS;
    }

    /* The most complex case starts here */
    RtlInitUnicodeString(&SystemRoot, L"\\SystemRoot");
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open this symlink */
    Status = NtOpenSymbolicLinkObject(&SymbolicLinkHandle, SYMBOLIC_LINK_QUERY, &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        LinkTarget.Length = 0;
        LinkTarget.MaximumLength = 0;

        DPRINT("Opened symbolic link object\n");

        Status = NtQuerySymbolicLinkObject(SymbolicLinkHandle, &LinkTarget, &BufferSize);
        if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* Check if required buffer size is sane */
            if (BufferSize > 0xFFFD)
            {
                DPRINT1("Too large buffer required\n");
                *RelativeName = 0;

                if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                LocalFree(Expanded);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            /* Alloc the string */
            LinkTarget.Buffer = LocalAlloc(LMEM_ZEROINIT, BufferSize + sizeof(WCHAR));
            if (!LinkTarget.Buffer)
            {
                DPRINT1("Unable to alloc buffer\n");
                if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                LocalFree(Expanded);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            /* Do a real query now */
            LinkTarget.Length = BufferSize;
            LinkTarget.MaximumLength = LinkTarget.Length + sizeof(WCHAR);

            Status = NtQuerySymbolicLinkObject(SymbolicLinkHandle, &LinkTarget, &BufferSize);
            if (NT_SUCCESS(Status))
            {
                DPRINT("LinkTarget: %wZ\n", &LinkTarget);

                ExpandedLen = LinkTarget.Length / sizeof(WCHAR);
                if ((ServiceNameLen > ExpandedLen) &&
                    !wcsnicmp(LinkTarget.Buffer, CanonName, ExpandedLen))
                {
                    *RelativeName = LocalAlloc(LMEM_ZEROINIT,
                       (ServiceNameLen - ExpandedLen) * sizeof(WCHAR) + 13*sizeof(WCHAR));

                    if (*RelativeName == NULL)
                    {
                        DPRINT1("Unable to alloc buffer\n");
                        if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                        LocalFree(Expanded);
                        RtlFreeUnicodeString(&NtPathName);
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }

                    /* Copy it over, substituting the first part
                       with SystemRoot */
                    wcscpy(*RelativeName, L"\\SystemRoot\\");
                    wcscat(*RelativeName, CanonName+ExpandedLen+1);

                    /* Cleanup */
                    if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                    LocalFree(Expanded);
                    RtlFreeUnicodeString(&NtPathName);

                    /* Return success */
                    return ERROR_SUCCESS;
                }
                else
                {
                    if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                    LocalFree(Expanded);
                    RtlFreeUnicodeString(&NtPathName);
                    return ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                DPRINT1("Error, Status = %08X\n", Status);
                if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
                LocalFree(Expanded);
                RtlFreeUnicodeString(&NtPathName);
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            DPRINT1("Error, Status = %08X\n", Status);
            if (SymbolicLinkHandle) NtClose(SymbolicLinkHandle);
            LocalFree(Expanded);
            RtlFreeUnicodeString(&NtPathName);
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        DPRINT1("Error, Status = %08X\n", Status);
        LocalFree(Expanded);
        return ERROR_INVALID_PARAMETER;
    }

    /* Failure */
    *RelativeName = NULL;
    return ERROR_INVALID_PARAMETER;
}

DWORD
ScmCanonDriverImagePath(DWORD dwStartType,
                        wchar_t *lpServiceName,
                        wchar_t **lpCanonName)
{
    DWORD ServiceNameLen, Result;
    UNICODE_STRING NtServiceName;
    WCHAR *RelativeName;
    WCHAR *SourceName = lpServiceName;

    /* Calculate the length of the service's name */
    ServiceNameLen = wcslen(lpServiceName);

    /* 12 is wcslen(L"\\SystemRoot\\") */
    if (ServiceNameLen > 12 &&
        !wcsnicmp(L"\\SystemRoot\\", lpServiceName, 12))
    {
        /* SystemRoot prefix is already included */

        *lpCanonName = LocalAlloc(LMEM_ZEROINIT, ServiceNameLen * sizeof(WCHAR) + sizeof(WCHAR));

        if (*lpCanonName == NULL)
        {
            DPRINT1("Error allocating memory for canonized service name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* If it's a boot-time driver, it must be systemroot relative */
        if (dwStartType == SERVICE_BOOT_START)
            SourceName += 12;

        /* Copy it */
        wcscpy(*lpCanonName, SourceName);

        DPRINT("Canonicalized name %S\n", *lpCanonName);
        return NO_ERROR;
    }

    /* Check if it has %SystemRoot% (len=13) */
    if (ServiceNameLen > 13 &&
        !wcsnicmp(L"%%SystemRoot%%\\", lpServiceName, 13))
    {
        /* Substitute %SystemRoot% with \\SystemRoot\\ */
        *lpCanonName = LocalAlloc(LMEM_ZEROINIT, ServiceNameLen * sizeof(WCHAR) + sizeof(WCHAR));

        if (*lpCanonName == NULL)
        {
            DPRINT1("Error allocating memory for canonized service name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* If it's a boot-time driver, it must be systemroot relative */
        if (dwStartType == SERVICE_BOOT_START)
            wcscpy(*lpCanonName, L"\\SystemRoot\\");

        wcscat(*lpCanonName, lpServiceName + 13);

        DPRINT("Canonicalized name %S\n", *lpCanonName);
        return NO_ERROR;
    }

    /* Check if it's a relative path name */
    if (lpServiceName[0] != L'\\' && lpServiceName[1] != L':')
    {
        *lpCanonName = LocalAlloc(LMEM_ZEROINIT, ServiceNameLen * sizeof(WCHAR) + sizeof(WCHAR));

        if (*lpCanonName == NULL)
        {
            DPRINT1("Error allocating memory for canonized service name!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Just copy it over without changing */
        wcscpy(*lpCanonName, lpServiceName);

        return NO_ERROR;
    }

    /* It seems to be a DOS path, convert it */
    if (!RtlDosPathNameToNtPathName_U(lpServiceName, &NtServiceName, NULL, NULL))
    {
        DPRINT1("RtlDosPathNameToNtPathName_U() failed!\n");
        return ERROR_INVALID_PARAMETER;
    }

    *lpCanonName = LocalAlloc(LMEM_ZEROINIT, NtServiceName.Length + sizeof(WCHAR));

    if (*lpCanonName == NULL)
    {
        DPRINT1("Error allocating memory for canonized service name!\n");
        RtlFreeUnicodeString(&NtServiceName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Copy the string */
    wcsncpy(*lpCanonName, NtServiceName.Buffer, NtServiceName.Length / sizeof(WCHAR));

    /* The unicode string is not needed anymore */
    RtlFreeUnicodeString(&NtServiceName);

    if (dwStartType != SERVICE_BOOT_START)
    {
        DPRINT("Canonicalized name %S\n", *lpCanonName);
        return NO_ERROR;
    }

    /* The service is boot-started, so must be relative */
    Result = ScmConvertToBootPathName(*lpCanonName, &RelativeName);
    if (Result)
    {
        /* There is a problem, free name and return */
        LocalFree(*lpCanonName);
        DPRINT1("Error converting named!\n");
        return Result;
    }

    ASSERT(RelativeName);

    /* Copy that string */
    wcscpy(*lpCanonName, RelativeName + 12);

    /* Free the allocated buffer */
    LocalFree(RelativeName);

    DPRINT("Canonicalized name %S\n", *lpCanonName);

    /* Success */
    return NO_ERROR;
}


/* Function 12 */
DWORD RCreateServiceW(
    SC_RPC_HANDLE hSCManager,
    LPWSTR lpServiceName,
    LPWSTR lpDisplayName,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPWSTR lpBinaryPathName,
    LPWSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPBYTE lpDependencies,
    DWORD dwDependSize,
    LPWSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    PMANAGER_HANDLE hManager;
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE lpService = NULL;
    SC_HANDLE hServiceHandle = NULL;
    LPWSTR lpImagePath = NULL;
    HKEY hServiceKey = NULL;

    DPRINT("RCreateServiceW() called\n");
    DPRINT("lpServiceName = %S\n", lpServiceName);
    DPRINT("lpDisplayName = %S\n", lpDisplayName);
    DPRINT("dwDesiredAccess = %lx\n", dwDesiredAccess);
    DPRINT("dwServiceType = %lu\n", dwServiceType);
    DPRINT("dwStartType = %lu\n", dwStartType);
    DPRINT("dwErrorControl = %lu\n", dwErrorControl);
    DPRINT("lpBinaryPathName = %S\n", lpBinaryPathName);
    DPRINT("lpLoadOrderGroup = %S\n", lpLoadOrderGroup);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (!hManager || hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hManager->Handle.DesiredAccess,
                                  SC_MANAGER_CREATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hManager->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    if (wcslen(lpServiceName) == 0)
    {
        return ERROR_INVALID_NAME;
    }

    if (wcslen(lpBinaryPathName) == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwServiceType == (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS)) &&
        (lpServiceStartName))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwServiceType > SERVICE_WIN32_SHARE_PROCESS) &&
        (dwServiceType != (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS)) &&
        (dwServiceType != (SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwStartType > SERVICE_DISABLED)
    {
        return ERROR_INVALID_PARAMETER;
    }

    lpService = ScmGetServiceEntryByName(lpServiceName);
    if (lpService)
    {
        /* check if it is marked for deletion */
        if (lpService->bDeleted)
            return ERROR_SERVICE_MARKED_FOR_DELETE;
        /* Return Error exist */
        return ERROR_SERVICE_EXISTS;
    }

    if (lpDisplayName != NULL &&
        ScmGetServiceEntryByDisplayName(lpDisplayName) != NULL)
        return ERROR_DUPLICATE_SERVICE_NAME;

    if (dwServiceType & SERVICE_DRIVER)
    {
        dwError = ScmCanonDriverImagePath(dwStartType,
                                          lpBinaryPathName,
                                          &lpImagePath);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }
    else
    {
        if (dwStartType == SERVICE_BOOT_START ||
            dwStartType == SERVICE_SYSTEM_START)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    /* Allocate a new service entry */
    dwError = ScmCreateNewServiceRecord(lpServiceName,
                                        &lpService);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Fill the new service entry */
    lpService->Status.dwServiceType = dwServiceType;
    lpService->dwStartType = dwStartType;
    lpService->dwErrorControl = dwErrorControl;

    /* Fill the display name */
    if (lpDisplayName != NULL &&
        *lpDisplayName != 0 &&
        wcsicmp(lpService->lpDisplayName, lpDisplayName) != 0)
    {
        lpService->lpDisplayName = (WCHAR*) HeapAlloc(GetProcessHeap(), 0,
                                             (wcslen(lpDisplayName) + 1) * sizeof(WCHAR));
        if (lpService->lpDisplayName == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        wcscpy(lpService->lpDisplayName, lpDisplayName);
    }

    /* Assign the service to a group */
    if (lpLoadOrderGroup != NULL && *lpLoadOrderGroup != 0)
    {
        dwError = ScmSetServiceGroup(lpService,
                                     lpLoadOrderGroup);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Assign a new tag */
    if (lpdwTagId != NULL)
    {
        dwError = ScmAssignNewTag(lpService);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Write service data to the registry */
    /* Create the service key */
    dwError = ScmCreateServiceKey(lpServiceName,
                                  KEY_WRITE,
                                  &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Set the display name */
    if (lpDisplayName != NULL && *lpDisplayName != 0)
    {
        RegSetValueExW(hServiceKey,
                       L"DisplayName",
                       0,
                       REG_SZ,
                       (LPBYTE)lpDisplayName,
                       (wcslen(lpDisplayName) + 1) * sizeof(WCHAR));
    }

    /* Set the service type */
    dwError = RegSetValueExW(hServiceKey,
                             L"Type",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwServiceType,
                             sizeof(DWORD));
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Set the start value */
    dwError = RegSetValueExW(hServiceKey,
                             L"Start",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwStartType,
                             sizeof(DWORD));
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Set the error control value */
    dwError = RegSetValueExW(hServiceKey,
                             L"ErrorControl",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwErrorControl,
                             sizeof(DWORD));
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Set the image path */
    if (dwServiceType & SERVICE_WIN32)
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"ImagePath",
                                 0,
                                 REG_EXPAND_SZ,
                                 (LPBYTE)lpBinaryPathName,
                                 (wcslen(lpBinaryPathName) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;
    }
    else if (dwServiceType & SERVICE_DRIVER)
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"ImagePath",
                                 0,
                                 REG_EXPAND_SZ,
                                 (LPBYTE)lpImagePath,
                                 (wcslen(lpImagePath) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Set the group name */
    if (lpLoadOrderGroup != NULL && *lpLoadOrderGroup != 0)
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"Group",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)lpLoadOrderGroup,
                                 (wcslen(lpLoadOrderGroup) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (lpdwTagId != NULL)
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"Tag",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&lpService->dwTag,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Write dependencies */
    if (lpDependencies != NULL && *lpDependencies != 0)
    {
        dwError = ScmWriteDependencies(hServiceKey,
                                       (LPWSTR)lpDependencies,
                                       dwDependSize);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* If a non driver and NULL for lpServiceName, write ObjectName as LocalSystem */
    if ((dwServiceType & SERVICE_WIN32) && (!lpServiceName))
    {
        dwError = RegSetValueExW(hServiceKey,
                                 L"ObjectName",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)L"LocalSystem",
                                 24);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (lpPassword != NULL)
    {
        /* FIXME: Write password */
    }

    dwError = ScmCreateServiceHandle(lpService,
                                     &hServiceHandle);
    if (dwError != ERROR_SUCCESS)
        goto done;

    dwError = ScmCheckAccess(hServiceHandle,
                             dwDesiredAccess);
    if (dwError != ERROR_SUCCESS)
        goto done;

    lpService->dwRefCount = 1;
    DPRINT("CreateService - lpService->dwRefCount %u\n", lpService->dwRefCount);

done:;
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    if (dwError == ERROR_SUCCESS)
    {
        DPRINT("hService %p\n", hServiceHandle);
        *lpServiceHandle = (SC_RPC_HANDLE)hServiceHandle;

        if (lpdwTagId != NULL)
            *lpdwTagId = lpService->dwTag;
    }
    else
    {
        /* Release the display name buffer */
        if (lpService->lpServiceName != NULL)
            HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

        if (hServiceHandle)
        {
            /* Remove the service handle */
            HeapFree(GetProcessHeap(), 0, hServiceHandle);
        }

        if (lpService != NULL)
        {
            /* FIXME: remove the service entry */
        }
    }

    if (lpImagePath != NULL)
        HeapFree(GetProcessHeap(), 0, lpImagePath);

    DPRINT("RCreateServiceW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 13 */
DWORD REnumDependentServicesW(
    SC_RPC_HANDLE hService,
    DWORD dwServiceState,
    LPBYTE lpServices,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwServicesReturned = 0;
    DWORD dwServiceCount;
    HKEY hServicesKey = NULL;
    LPSC_RPC_HANDLE hSCObject;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    PSERVICE *lpServicesArray = NULL;
    LPENUM_SERVICE_STATUSW lpServicesPtr = NULL;
    LPWSTR lpStr;

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    DPRINT("REnumDependentServicesW() called\n");

    hSCObject = &hService;
    hSvc = (PSERVICE_HANDLE) *hSCObject;
    lpService = hSvc->ServiceEntry;

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    /* Open the Services Reg key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* First determine the bytes needed and get the number of dependent services */
    dwError = Int_EnumDependentServicesW(hServicesKey,
                                         lpService,
                                         dwServiceState,
                                         NULL,
                                         pcbBytesNeeded,
                                         &dwServicesReturned);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    /* If buffer size is less than the bytes needed or pointer is null */
    if ((!lpServices) || (cbBufSize < *pcbBytesNeeded))
    {
        dwError = ERROR_MORE_DATA;
        goto Done;
    }

    /* Allocate memory for array of service pointers */
    lpServicesArray = HeapAlloc(GetProcessHeap(),
                                0,
                                (dwServicesReturned + 1) * sizeof(PSERVICE));
    if (!lpServicesArray)
    {
        DPRINT1("Could not allocate a buffer!!\n");
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    dwServicesReturned = 0;
    *pcbBytesNeeded = 0;

    dwError = Int_EnumDependentServicesW(hServicesKey,
                                         lpService,
                                         dwServiceState,
                                         lpServicesArray,
                                         pcbBytesNeeded,
                                         &dwServicesReturned);
    if (dwError != ERROR_SUCCESS)
    {
        goto Done;
    }

    lpServicesPtr = (LPENUM_SERVICE_STATUSW) lpServices;
    lpStr = (LPWSTR)(lpServices + (dwServicesReturned * sizeof(ENUM_SERVICE_STATUSW)));

    /* Copy EnumDepenedentService to Buffer */
    for (dwServiceCount = 0; dwServiceCount < dwServicesReturned; dwServiceCount++)
    {
        lpService = lpServicesArray[dwServiceCount];

        /* Copy status info */
        memcpy(&lpServicesPtr->ServiceStatus,
               &lpService->Status,
               sizeof(SERVICE_STATUS));

        /* Copy display name */
        wcscpy(lpStr, lpService->lpDisplayName);
        lpServicesPtr->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServices);
        lpStr += (wcslen(lpService->lpDisplayName) + 1);

        /* Copy service name */
        wcscpy(lpStr, lpService->lpServiceName);
        lpServicesPtr->lpServiceName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServices);
        lpStr += (wcslen(lpService->lpServiceName) + 1);

        lpServicesPtr ++;
    }

    *lpServicesReturned = dwServicesReturned;

Done:
    if (lpServicesArray != NULL)
        HeapFree(GetProcessHeap(), 0, lpServicesArray);

    RegCloseKey(hServicesKey);

    DPRINT("REnumDependentServicesW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 14 */
DWORD REnumServicesStatusW(
    SC_RPC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPBYTE lpBuffer,
    DWORD dwBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned,
    LPBOUNDED_DWORD_256K lpResumeHandle)
{
    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwError = ERROR_SUCCESS;
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    DWORD dwState;
    DWORD dwRequiredSize;
    DWORD dwServiceCount;
    DWORD dwSize;
    DWORD dwLastResumeCount = 0;
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    LPWSTR lpStringPtr;

    DPRINT("REnumServicesStatusW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (!hManager || hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    if ((dwServiceType!=SERVICE_DRIVER) && (dwServiceType!=SERVICE_WIN32))
    {
        DPRINT("Not a valid Service Type!\n");
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwServiceState<SERVICE_ACTIVE) || (dwServiceState>SERVICE_STATE_ALL))
    {
        DPRINT("Not a valid Service State!\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hManager->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hManager->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    if (lpResumeHandle) dwLastResumeCount = *lpResumeHandle;

    /* FIXME: Lock the service list shared */

    lpService = ScmGetServiceEntryByResumeCount(dwLastResumeCount);
    if (lpService == NULL)
    {
        dwError = ERROR_SUCCESS;
        goto Done;
    }

    dwRequiredSize = 0;
    dwServiceCount = 0;

    for (ServiceEntry = &lpService->ServiceListEntry;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        dwSize = sizeof(ENUM_SERVICE_STATUSW) +
                 ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                 ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR));

        if (dwRequiredSize + dwSize > dwBufSize)
        {
            DPRINT("Service name: %S  no fit\n", CurrentService->lpServiceName);
            break;
        }

        DPRINT("Service name: %S  fit\n", CurrentService->lpServiceName);
        dwRequiredSize += dwSize;
        dwServiceCount++;
        dwLastResumeCount = CurrentService->dwResumeCount;
    }

    DPRINT("dwRequiredSize: %lu\n", dwRequiredSize);
    DPRINT("dwServiceCount: %lu\n", dwServiceCount);

    for (;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        dwRequiredSize += (sizeof(ENUM_SERVICE_STATUSW) +
                           ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                           ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR)));

        dwError = ERROR_MORE_DATA;
    }

    DPRINT("*pcbBytesNeeded: %lu\n", dwRequiredSize);

    if (lpResumeHandle) *lpResumeHandle = dwLastResumeCount;
    *lpServicesReturned = dwServiceCount;
    *pcbBytesNeeded = dwRequiredSize;

    lpStatusPtr = (LPENUM_SERVICE_STATUSW)lpBuffer;
    lpStringPtr = (LPWSTR)((ULONG_PTR)lpBuffer +
                           dwServiceCount * sizeof(ENUM_SERVICE_STATUSW));

    dwRequiredSize = 0;
    for (ServiceEntry = &lpService->ServiceListEntry;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        dwSize = sizeof(ENUM_SERVICE_STATUSW) +
                 ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                 ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR));

        if (dwRequiredSize + dwSize > dwBufSize)
            break;

        /* Copy the service name */
        wcscpy(lpStringPtr, CurrentService->lpServiceName);
        lpStatusPtr->lpServiceName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpBuffer);
        lpStringPtr += (wcslen(CurrentService->lpServiceName) + 1);

        /* Copy the display name */
        wcscpy(lpStringPtr, CurrentService->lpDisplayName);
        lpStatusPtr->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpBuffer);
        lpStringPtr += (wcslen(CurrentService->lpDisplayName) + 1);

        /* Copy the status information */
        memcpy(&lpStatusPtr->ServiceStatus,
               &CurrentService->Status,
               sizeof(SERVICE_STATUS));

        lpStatusPtr++;
        dwRequiredSize += dwSize;
    }

    if (dwError == 0) 
    {
        *pcbBytesNeeded = 0;
        if (lpResumeHandle) *lpResumeHandle = 0;
    }

Done:;
    /* FIXME: Unlock the service list */

    DPRINT("REnumServicesStatusW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 15 */
DWORD ROpenSCManagerW(
    LPWSTR lpMachineName,
    LPWSTR lpDatabaseName,
    DWORD dwDesiredAccess,
    LPSC_RPC_HANDLE lpScHandle)
{
    DWORD dwError;
    SC_HANDLE hHandle;

    DPRINT("ROpenSCManagerW() called\n");
    DPRINT("lpMachineName = %p\n", lpMachineName);
    DPRINT("lpMachineName: %S\n", lpMachineName);
    DPRINT("lpDataBaseName = %p\n", lpDatabaseName);
    DPRINT("lpDataBaseName: %S\n", lpDatabaseName);
    DPRINT("dwDesiredAccess = %x\n", dwDesiredAccess);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (!lpScHandle)
        return ERROR_INVALID_PARAMETER;

    dwError = ScmCreateManagerHandle(lpDatabaseName,
                                     &hHandle);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmCreateManagerHandle() failed (Error %lu)\n", dwError);
        return dwError;
    }

    /* Check the desired access */
    dwError = ScmCheckAccess(hHandle,
                             dwDesiredAccess | SC_MANAGER_CONNECT);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmCheckAccess() failed (Error %lu)\n", dwError);
        HeapFree(GetProcessHeap(), 0, hHandle);
        return dwError;
    }

    *lpScHandle = (SC_RPC_HANDLE)hHandle;
    DPRINT("*hScm = %p\n", *lpScHandle);

    DPRINT("ROpenSCManagerW() done\n");

    return ERROR_SUCCESS;
}


/* Function 16 */
DWORD ROpenServiceW(
    SC_RPC_HANDLE hSCManager,
    LPWSTR lpServiceName,
    DWORD dwDesiredAccess,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    PSERVICE lpService;
    PMANAGER_HANDLE hManager;
    SC_HANDLE hHandle;
    DWORD dwError;

    DPRINT("ROpenServiceW() called\n");
    DPRINT("hSCManager = %p\n", hSCManager);
    DPRINT("lpServiceName = %p\n", lpServiceName);
    DPRINT("lpServiceName: %S\n", lpServiceName);
    DPRINT("dwDesiredAccess = %x\n", dwDesiredAccess);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (!lpServiceHandle)
        return ERROR_INVALID_PARAMETER;

    if (!lpServiceName)
        return ERROR_INVALID_ADDRESS;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (!hManager || hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service list */

    /* Get service database entry */
    lpService = ScmGetServiceEntryByName(lpServiceName);
    if (lpService == NULL)
    {
        DPRINT("Could not find a service!\n");
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    /* Create a service handle */
    dwError = ScmCreateServiceHandle(lpService,
                                     &hHandle);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmCreateServiceHandle() failed (Error %lu)\n", dwError);
        return dwError;
    }

    /* Check the desired access */
    dwError = ScmCheckAccess(hHandle,
                             dwDesiredAccess);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmCheckAccess() failed (Error %lu)\n", dwError);
        HeapFree(GetProcessHeap(), 0, hHandle);
        return dwError;
    }

    lpService->dwRefCount++;
    DPRINT("OpenService - lpService->dwRefCount %u\n",lpService->dwRefCount);

    *lpServiceHandle = (SC_RPC_HANDLE)hHandle;
    DPRINT("*hService = %p\n", *lpServiceHandle);

    DPRINT("ROpenServiceW() done\n");

    return ERROR_SUCCESS;
}


/* Function 17 */
DWORD RQueryServiceConfigW(
    SC_RPC_HANDLE hService,
    LPBYTE lpBuf, //LPQUERY_SERVICE_CONFIGW lpServiceConfig,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_8K pcbBytesNeeded)
{
    LPQUERY_SERVICE_CONFIGW lpServiceConfig = (LPQUERY_SERVICE_CONFIGW)lpBuf;
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    LPWSTR lpImagePath = NULL;
    LPWSTR lpServiceStartName = NULL;
    DWORD dwRequiredSize;
    LPQUERY_SERVICE_CONFIGW lpConfig = NULL;
    WCHAR lpEmptyString[] = {0,0};
    LPWSTR lpStr;

    DPRINT("RQueryServiceConfigW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service database shared */

    dwError = ScmOpenServiceKey(lpService->lpServiceName,
                                KEY_READ,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    dwError = ScmReadString(hServiceKey,
                            L"ImagePath",
                            &lpImagePath);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    ScmReadString(hServiceKey,
                  L"ObjectName",
                  &lpServiceStartName);

    dwRequiredSize = sizeof(QUERY_SERVICE_CONFIGW);

    if (lpImagePath != NULL)
        dwRequiredSize += ((wcslen(lpImagePath) + 1) * sizeof(WCHAR));
    else
        dwRequiredSize += 2 * sizeof(WCHAR);

    if (lpService->lpGroup != NULL)
        dwRequiredSize += ((wcslen(lpService->lpGroup->lpGroupName) + 1) * sizeof(WCHAR));
    else
        dwRequiredSize += 2 * sizeof(WCHAR);

    /* FIXME: Add Dependencies length*/

    if (lpServiceStartName != NULL)
        dwRequiredSize += ((wcslen(lpServiceStartName) + 1) * sizeof(WCHAR));
    else
        dwRequiredSize += 2 * sizeof(WCHAR);

    if (lpService->lpDisplayName != NULL)
        dwRequiredSize += ((wcslen(lpService->lpDisplayName) + 1) * sizeof(WCHAR));
    else
        dwRequiredSize += 2 * sizeof(WCHAR);

    if (lpServiceConfig == NULL || cbBufSize < dwRequiredSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        lpConfig = (LPQUERY_SERVICE_CONFIGW)lpServiceConfig;
        lpConfig->dwServiceType = lpService->Status.dwServiceType;
        lpConfig->dwStartType = lpService->dwStartType;
        lpConfig->dwErrorControl = lpService->dwErrorControl;
        lpConfig->dwTagId = lpService->dwTag;

        lpStr = (LPWSTR)(lpConfig + 1);

        if (lpImagePath != NULL)
        {
            wcscpy(lpStr, lpImagePath);
        }
        else
        {
            wcscpy(lpStr, lpEmptyString);
        }

        lpConfig->lpBinaryPathName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (wcslen(lpStr) + 1);

        if (lpService->lpGroup != NULL)
        {
            wcscpy(lpStr, lpService->lpGroup->lpGroupName);
        }
        else
        {
            wcscpy(lpStr, lpEmptyString);
        }

        lpConfig->lpLoadOrderGroup = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (wcslen(lpStr) + 1);

        /* FIXME: Append Dependencies */
        wcscpy(lpStr, lpEmptyString);

        lpStr += (wcslen(lpStr) + 1);
        lpConfig->lpDependencies = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);

        if (lpServiceStartName != NULL)
        {
            wcscpy(lpStr, lpServiceStartName);
        }
        else
        {
            wcscpy(lpStr, lpEmptyString);
        }

        lpConfig->lpServiceStartName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (wcslen(lpStr) + 1);

        if (lpService->lpDisplayName != NULL)
        {
            wcscpy(lpStr, lpService->lpDisplayName);
        }
        else
        {
            wcscpy(lpStr, lpEmptyString);
        }

        lpConfig->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
    }

    if (pcbBytesNeeded != NULL)
        *pcbBytesNeeded = dwRequiredSize;

Done:;
    if (lpImagePath != NULL)
        HeapFree(GetProcessHeap(), 0, lpImagePath);

    if (lpServiceStartName != NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceStartName);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    /* FIXME: Unlock the service database */

    DPRINT("RQueryServiceConfigW() done\n");

    return dwError;
}


/* Function 18 */
DWORD RQueryServiceLockStatusW(
    SC_RPC_HANDLE hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_4K pcbBytesNeeded)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 19 */
DWORD RStartServiceW(
    SC_RPC_HANDLE hService,
    DWORD argc,
    LPSTRING_PTRSW argv)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;

    DPRINT("RStartServiceW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_START))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (lpService->dwStartType == SERVICE_DISABLED)
        return ERROR_SERVICE_DISABLED;

    if (lpService->bDeleted)
        return ERROR_SERVICE_MARKED_FOR_DELETE;

    if (argv) {
        UNIMPLEMENTED;
        argv = NULL;
    }

    /* Start the service */
    dwError = ScmStartService(lpService, argc, (LPWSTR *)argv);

    return dwError;
}


/* Function 20 */
DWORD RGetServiceDisplayNameW(
    SC_RPC_HANDLE hSCManager,
    LPWSTR lpServiceName,
    LPWSTR lpDisplayName,
    DWORD *lpcchBuffer)
{
//    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;

    DPRINT("RGetServiceDisplayNameW() called\n");
    DPRINT("hSCManager = %p\n", hSCManager);
    DPRINT("lpServiceName: %S\n", lpServiceName);
    DPRINT("lpDisplayName: %p\n", lpDisplayName);
    DPRINT("*lpcchBuffer: %lu\n", *lpcchBuffer);

//    hManager = (PMANAGER_HANDLE)hSCManager;
//    if (hManager->Handle.Tag != MANAGER_TAG)
//    {
//        DPRINT1("Invalid manager handle!\n");
//        return ERROR_INVALID_HANDLE;
//    }

    /* Get service database entry */
    lpService = ScmGetServiceEntryByName(lpServiceName);
    if (lpService == NULL)
    {
        DPRINT1("Could not find a service!\n");

        /* If the service could not be found and lpcchBuffer is 0, windows
           puts null in lpDisplayName and puts 1 in lpcchBuffer */
        if (*lpcchBuffer == 0)
        {
            *lpcchBuffer = 1;
            *lpDisplayName = '\0';
        }

        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    if (!lpService->lpDisplayName)
    {
        dwLength = wcslen(lpService->lpServiceName);

        if (lpServiceName != NULL &&
            *lpcchBuffer > dwLength)
        {
            wcscpy(lpDisplayName, lpService->lpServiceName);
        }
    }
    else
    {
        dwLength = wcslen(lpService->lpDisplayName);

        if (lpDisplayName != NULL &&
            *lpcchBuffer > dwLength)
        {
            wcscpy(lpDisplayName, lpService->lpDisplayName);
        }
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength;

    return dwError;
}


/* Function 21 */
DWORD RGetServiceKeyNameW(
    SC_RPC_HANDLE hSCManager,
    LPWSTR lpDisplayName,
    LPWSTR lpServiceName,
    DWORD *lpcchBuffer)
{
//    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;

    DPRINT("RGetServiceKeyNameW() called\n");
    DPRINT("hSCManager = %p\n", hSCManager);
    DPRINT("lpDisplayName: %S\n", lpDisplayName);
    DPRINT("lpServiceName: %p\n", lpServiceName);
    DPRINT("*lpcchBuffer: %lu\n", *lpcchBuffer);

//    hManager = (PMANAGER_HANDLE)hSCManager;
//    if (hManager->Handle.Tag != MANAGER_TAG)
//    {
//        DPRINT1("Invalid manager handle!\n");
//        return ERROR_INVALID_HANDLE;
//    }

    /* Get service database entry */
    lpService = ScmGetServiceEntryByDisplayName(lpDisplayName);
    if (lpService == NULL)
    {
        DPRINT1("Could not find a service!\n");

        /* If the service could not be found and lpcchBuffer is 0, windows
           puts null in lpDisplayName and puts 2 in lpcchBuffer */
        if (*lpcchBuffer == 0)
        {
            *lpcchBuffer = 2;
            *lpServiceName = '\0';
        }

        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    dwLength = wcslen(lpService->lpServiceName);

    if (lpServiceName != NULL &&
        *lpcchBuffer > dwLength)
    {
        wcscpy(lpServiceName, lpService->lpServiceName);
        *lpcchBuffer = dwLength;
        return ERROR_SUCCESS;
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength * 2;

    return dwError;
}


/* Function 22 */
DWORD RI_ScSetServiceBitsA(
    RPC_SERVICE_STATUS_HANDLE hServiceStatus,
    DWORD dwServiceBits,
    int bSetBitsOn,
    int bUpdateImmediately,
    char *lpString)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 23 */
DWORD RChangeServiceConfigA(
    SC_RPC_HANDLE hService,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPSTR lpBinaryPathName,
    LPSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPSTR lpDependencies,
    DWORD dwDependSize,
    LPSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPSTR lpDisplayName)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    LPWSTR lpDisplayNameW = NULL;
    // LPWSTR lpBinaryPathNameW = NULL;
    LPWSTR lpLoadOrderGroupW = NULL;
    LPWSTR lpDependenciesW = NULL;
    // LPWSTR lpPasswordW = NULL;

    DPRINT("RChangeServiceConfigA() called\n");
    DPRINT("dwServiceType = %lu\n", dwServiceType);
    DPRINT("dwStartType = %lu\n", dwStartType);
    DPRINT("dwErrorControl = %lu\n", dwErrorControl);
    DPRINT("lpBinaryPathName = %s\n", lpBinaryPathName);
    DPRINT("lpLoadOrderGroup = %s\n", lpLoadOrderGroup);
    DPRINT("lpDisplayName = %s\n", lpDisplayName);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_CHANGE_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock database exclusively */

    if (lpService->bDeleted)
    {
        /* FIXME: Unlock database */
        DPRINT1("The service has already been marked for delete!\n");
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    /* Open the service key */
    dwError = ScmOpenServiceKey(lpService->szServiceName,
                                KEY_SET_VALUE,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Write service data to the registry */

    if (lpDisplayName != NULL && *lpDisplayName != 0)
    {
        /* Set the display name */
        lpDisplayNameW = HeapAlloc(GetProcessHeap(),
                                   0,
                                   (strlen(lpDisplayName) + 1) * sizeof(WCHAR));
        if (lpDisplayNameW == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpDisplayName,
                            -1,
                            lpDisplayNameW,
                            wcslen(lpDisplayNameW) + 1);

        RegSetValueExW(hServiceKey,
                       L"DisplayName",
                       0,
                       REG_SZ,
                       (LPBYTE)lpDisplayNameW,
                       (wcslen(lpDisplayNameW) + 1) * sizeof(WCHAR));

        /* Update lpService->lpDisplayName */
        if (lpService->lpDisplayName)
            HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

        lpService->lpDisplayName = lpDisplayNameW;
    }

    if (dwServiceType != SERVICE_NO_CHANGE)
    {
        /* Set the service type */
        dwError = RegSetValueExW(hServiceKey,
                                 L"Type",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwServiceType,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->Status.dwServiceType = dwServiceType;
    }

    if (dwStartType != SERVICE_NO_CHANGE)
    {
        /* Set the start value */
        dwError = RegSetValueExW(hServiceKey,
                                 L"Start",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwStartType,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->dwStartType = dwStartType;
    }

    if (dwErrorControl != SERVICE_NO_CHANGE)
    {
        /* Set the error control value */
        dwError = RegSetValueExW(hServiceKey,
                                 L"ErrorControl",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&dwErrorControl,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        lpService->dwErrorControl = dwErrorControl;
    }

#if 0
    /* FIXME: set the new ImagePath value */

    /* Set the image path */
    if (dwServiceType & SERVICE_WIN32)
    {
        if (lpBinaryPathName != NULL && *lpBinaryPathName != 0)
        {
            lpBinaryPathNameW=HeapAlloc(GetProcessHeap(),0, (strlen(lpBinaryPathName)+1) * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, lpBinaryPathName, -1, lpBinaryPathNameW, wcslen(lpBinaryPathNameW)+1);
            dwError = RegSetValueExW(hServiceKey,
                                     L"ImagePath",
                                     0,
                                     REG_EXPAND_SZ,
                                     (LPBYTE)lpBinaryPathNameW,
                                     (wcslen(lpBinaryPathNameW) + 1) * sizeof(WCHAR));
            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }
    else if (dwServiceType & SERVICE_DRIVER)
    {
        if (lpImagePath != NULL && *lpImagePath != 0)
        {
            dwError = RegSetValueExW(hServiceKey,
                                     L"ImagePath",
                                     0,
                                     REG_EXPAND_SZ,
                                     (LPBYTE)lpImagePath,
                                     (wcslen(lpImagePath) + 1) *sizeof(WCHAR));
            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }
#endif

    /* Set the group name */
    if (lpLoadOrderGroup != NULL && *lpLoadOrderGroup != 0)
    {
        lpLoadOrderGroupW = HeapAlloc(GetProcessHeap(),
                                      0,
                                      (strlen(lpLoadOrderGroup)+1) * sizeof(WCHAR));
        if (lpLoadOrderGroupW == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpLoadOrderGroup,
                            -1,
                            lpLoadOrderGroupW,
                            wcslen(lpLoadOrderGroupW) + 1);

        dwError = RegSetValueExW(hServiceKey,
                                 L"Group",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)lpLoadOrderGroupW,
                                 (wcslen(lpLoadOrderGroupW) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;

        /* FIXME: Update lpService->lpServiceGroup */

        HeapFree(GetProcessHeap(), 0, lpLoadOrderGroupW);
    }

    if (lpdwTagId != NULL)
    {
        dwError = ScmAssignNewTag(lpService);
        if (dwError != ERROR_SUCCESS)
            goto done;

        dwError = RegSetValueExW(hServiceKey,
                                 L"Tag",
                                 0,
                                 REG_DWORD,
                                 (LPBYTE)&lpService->dwTag,
                                 sizeof(DWORD));
        if (dwError != ERROR_SUCCESS)
            goto done;

        *lpdwTagId = lpService->dwTag;
    }

    /* Write dependencies */
    if (lpDependencies != NULL && *lpDependencies != 0)
    {
        lpDependenciesW = HeapAlloc(GetProcessHeap(),
                                    0,
                                    (strlen(lpDependencies)+1) * sizeof(WCHAR));
        if (lpDependenciesW == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpDependencies,
                            dwDependSize,
                            lpDependenciesW,
                            wcslen(lpDependenciesW)+1);

        dwError = ScmWriteDependencies(hServiceKey,
                                       (LPWSTR)lpDependenciesW,
                                       dwDependSize);

        HeapFree(GetProcessHeap(), 0, lpDependenciesW);
    }

    if (lpPassword != NULL)
    {
        /* FIXME: Write password */
    }

    /* FIXME: Unlock database */

done:
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    DPRINT("RChangeServiceConfigA() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 24 */
DWORD RCreateServiceA(
    SC_RPC_HANDLE hSCManager,
    LPSTR lpServiceName,
    LPSTR lpDisplayName,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPSTR lpBinaryPathName,
    LPSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPBYTE lpDependencies,
    DWORD dwDependSize,
    LPSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 25 */
DWORD REnumDependentServicesA(
    SC_RPC_HANDLE hService,
    DWORD dwServiceState,
    LPBYTE lpServices,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwServicesReturned = 0;
    DWORD dwServiceCount;
    HKEY hServicesKey = NULL;
    LPSC_RPC_HANDLE hSCObject;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    PSERVICE *lpServicesArray = NULL;
    LPENUM_SERVICE_STATUSA lpServicesPtr = NULL;
    LPSTR lpStr;

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    DPRINT("REnumDependentServicesA() called\n");

    hSCObject = &hService;
    hSvc = (PSERVICE_HANDLE) *hSCObject;
    lpService = hSvc->ServiceEntry;

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    /* Open the Services Reg key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);

    if (dwError != ERROR_SUCCESS) return dwError;

    /* NOTE: Windows calculates the pcbBytesNeeded based on WCHAR strings for
             both EnumDependentServicesA and EnumDependentServicesW. So returned pcbBytesNeeded
             are the same for both. Verified in WINXP. */

    /* First determine the bytes needed and get the number of dependent services*/
    dwError = Int_EnumDependentServicesW(hServicesKey,
                                         lpService,
                                         dwServiceState,
                                         NULL,
                                         pcbBytesNeeded,
                                         &dwServicesReturned);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    /* If buffer size is less than the bytes needed or pointer is null*/
    if ((!lpServices) || (cbBufSize < *pcbBytesNeeded))
    {
        dwError = ERROR_MORE_DATA;
        goto Done;
    }

    /* Allocate memory for array of service pointers */
    lpServicesArray = HeapAlloc(GetProcessHeap(),
                                0,
                                (dwServicesReturned + 1) * sizeof(PSERVICE));
    if (!lpServicesArray)
    {
        DPRINT1("Could not allocate a buffer!!\n");
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    dwServicesReturned = 0;
    *pcbBytesNeeded = 0;

    dwError = Int_EnumDependentServicesW(hServicesKey,
                                         lpService,
                                         dwServiceState,
                                         lpServicesArray,
                                         pcbBytesNeeded,
                                         &dwServicesReturned);
    if (dwError != ERROR_SUCCESS)
    {
        goto Done;
    }

    lpServicesPtr = (LPENUM_SERVICE_STATUSA)lpServices;
    lpStr = (LPSTR)(lpServices + (dwServicesReturned * sizeof(ENUM_SERVICE_STATUSA)));

    /* Copy EnumDepenedentService to Buffer */
    for (dwServiceCount = 0; dwServiceCount < dwServicesReturned; dwServiceCount++)
    {
        lpService = lpServicesArray[dwServiceCount];

        /* Copy the status info */
        memcpy(&lpServicesPtr->ServiceStatus,
               &lpService->Status,
               sizeof(SERVICE_STATUS));

        /* Copy display name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpService->lpDisplayName,
                            -1,
                            lpStr,
                            wcslen(lpService->lpDisplayName),
                            0,
                            0);
        lpServicesPtr->lpDisplayName = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServices);
        lpStr += strlen(lpStr) + 1;

        /* Copy service name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpService->lpServiceName,
                            -1,
                            lpStr,
                            wcslen(lpService->lpServiceName),
                            0,
                            0);
        lpServicesPtr->lpServiceName = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServices);
        lpStr += strlen(lpStr) + 1;

        lpServicesPtr ++;
    }

    *lpServicesReturned = dwServicesReturned;

Done:
    if (lpServicesArray)
        HeapFree(GetProcessHeap(), 0, lpServicesArray);

    RegCloseKey(hServicesKey);

    DPRINT("REnumDependentServicesA() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 26 */
DWORD REnumServicesStatusA(
    SC_RPC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPBYTE lpBuffer,
    DWORD dwBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned,
    LPBOUNDED_DWORD_256K lpResumeHandle)
{
    LPENUM_SERVICE_STATUSW lpStatusPtrW = NULL;
    LPENUM_SERVICE_STATUSA lpStatusPtrA = NULL;
    LPWSTR lpStringPtrW;
    LPSTR lpStringPtrA;
    DWORD dwError;
    DWORD dwServiceCount;

    DPRINT("REnumServicesStatusA() called\n");

    if ((dwBufSize > 0) && (lpBuffer))
    {
        lpStatusPtrW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufSize);
        if (!lpStatusPtrW)
        {
            DPRINT1("Failed to allocate buffer!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    dwError = REnumServicesStatusW(//BindingHandle,
                                   hSCManager,
                                   dwServiceType,
                                   dwServiceState,
                                   (LPBYTE)lpStatusPtrW,
                                   dwBufSize,
                                   pcbBytesNeeded,
                                   lpServicesReturned,
                                   lpResumeHandle);

    /* if no services were returned then we are Done */
    if (*lpServicesReturned == 0) goto Done;

    lpStatusPtrA = (LPENUM_SERVICE_STATUSA)lpBuffer;
    lpStringPtrA = (LPSTR)((ULONG_PTR)lpBuffer +
                  *lpServicesReturned * sizeof(ENUM_SERVICE_STATUSA));
    lpStringPtrW = (LPWSTR)((ULONG_PTR)lpStatusPtrW + 
                  *lpServicesReturned * sizeof(ENUM_SERVICE_STATUSW));

    for (dwServiceCount = 0; dwServiceCount < *lpServicesReturned; dwServiceCount++)
    {
        /* Copy the service name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpStringPtrW,
                            -1,
                            lpStringPtrA,
                            wcslen(lpStringPtrW),
                            0,
                            0);

        lpStatusPtrA->lpServiceName = (LPSTR)((ULONG_PTR)lpStringPtrA - (ULONG_PTR)lpBuffer);
        lpStringPtrA += wcslen(lpStringPtrW) + 1;
        lpStringPtrW += wcslen(lpStringPtrW) + 1;

        /* Copy the display name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpStringPtrW,
                            -1,
                            lpStringPtrA,
                            wcslen(lpStringPtrW),
                            0,
                            0);

        lpStatusPtrA->lpDisplayName = (LPSTR)((ULONG_PTR)lpStringPtrA - (ULONG_PTR)lpBuffer);
        lpStringPtrA += wcslen(lpStringPtrW) + 1;
        lpStringPtrW += wcslen(lpStringPtrW) + 1;

        /* Copy the status information */
        memcpy(&lpStatusPtrA->ServiceStatus,
               &lpStatusPtrW->ServiceStatus,
               sizeof(SERVICE_STATUS));

        lpStatusPtrA++;
    }

Done:;
    if (lpStatusPtrW) HeapFree(GetProcessHeap(), 0, lpStatusPtrW);

    DPRINT("REnumServicesStatusA() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 27 */
DWORD ROpenSCManagerA(
    LPSTR lpMachineName,
    LPSTR lpDatabaseName,
    DWORD dwDesiredAccess,
    LPSC_RPC_HANDLE lpScHandle)
{
    UNICODE_STRING MachineName;
    UNICODE_STRING DatabaseName;
    DWORD dwError;

    DPRINT("ROpenSCManagerA() called\n");

    if (lpMachineName)
        RtlCreateUnicodeStringFromAsciiz(&MachineName,
                                         lpMachineName);

    if (lpDatabaseName)
        RtlCreateUnicodeStringFromAsciiz(&DatabaseName,
                                         lpDatabaseName);

    dwError = ROpenSCManagerW(//BindingHandle,
                              lpMachineName ? MachineName.Buffer : NULL,
                              lpDatabaseName ? DatabaseName.Buffer : NULL,
                              dwDesiredAccess,
                              lpScHandle);

    if (lpMachineName)
        RtlFreeUnicodeString(&MachineName);

    if (lpDatabaseName)
        RtlFreeUnicodeString(&DatabaseName);

    return dwError;
}


/* Function 28 */
DWORD ROpenServiceA(
    SC_RPC_HANDLE hSCManager,
    LPSTR lpServiceName,
    DWORD dwDesiredAccess,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    UNICODE_STRING ServiceName;
    DWORD dwError;

    DPRINT("ROpenServiceA() called\n");

    if (lpServiceName)
        RtlCreateUnicodeStringFromAsciiz(&ServiceName,
                                         lpServiceName);

    dwError = ROpenServiceW(//BindingHandle,
                            hSCManager,
                            lpServiceName ? ServiceName.Buffer : NULL,
                            dwDesiredAccess,
                            lpServiceHandle);

    if (lpServiceName)
        RtlFreeUnicodeString(&ServiceName);

    return dwError;
}


/* Function 29 */
DWORD RQueryServiceConfigA(
    SC_RPC_HANDLE hService,
    LPBYTE lpBuf, //LPQUERY_SERVICE_CONFIGA lpServiceConfig,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_8K pcbBytesNeeded)
{
    LPQUERY_SERVICE_CONFIGA lpServiceConfig = (LPQUERY_SERVICE_CONFIGA)lpBuf;
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    LPWSTR lpImagePath = NULL;
    LPWSTR lpServiceStartName = NULL;
    DWORD dwRequiredSize;
    LPQUERY_SERVICE_CONFIGA lpConfig = NULL;
    CHAR lpEmptyString[]={0,0};
    LPSTR lpStr;

    DPRINT("RQueryServiceConfigA() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service database shared */

    dwError = ScmOpenServiceKey(lpService->lpServiceName,
                                KEY_READ,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    dwError = ScmReadString(hServiceKey,
                            L"ImagePath",
                            &lpImagePath);
    if (dwError != ERROR_SUCCESS)
        goto Done;

    ScmReadString(hServiceKey,
                  L"ObjectName",
                  &lpServiceStartName);

    dwRequiredSize = sizeof(QUERY_SERVICE_CONFIGW);

    if (lpImagePath != NULL)
        dwRequiredSize += wcslen(lpImagePath) + 1;
    else
        dwRequiredSize += 2;

    if (lpService->lpGroup != NULL)
        dwRequiredSize += wcslen(lpService->lpGroup->lpGroupName) + 1;
    else
        dwRequiredSize += 2;

    /* FIXME: Add Dependencies length*/
    dwRequiredSize += 2;

    if (lpServiceStartName != NULL)
        dwRequiredSize += wcslen(lpServiceStartName) + 1;
    else
        dwRequiredSize += 2;

    if (lpService->lpDisplayName != NULL)
        dwRequiredSize += wcslen(lpService->lpDisplayName) + 1;
    else
        dwRequiredSize += 2;

    if (lpServiceConfig == NULL || cbBufSize < dwRequiredSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        lpConfig = (LPQUERY_SERVICE_CONFIGA)lpServiceConfig;
        lpConfig->dwServiceType = lpService->Status.dwServiceType;
        lpConfig->dwStartType = lpService->dwStartType;
        lpConfig->dwErrorControl = lpService->dwErrorControl;
        lpConfig->dwTagId = lpService->dwTag;

        lpStr = (LPSTR)(lpServiceConfig + 1);

        /* NOTE: Strings that are NULL for QUERY_SERVICE_CONFIG are pointers to empty strings.
          Verified in WINXP*/

        if (lpImagePath)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpImagePath,
                                -1,
                                lpStr,
                                wcslen(lpImagePath),
                                0,
                                0);
        }
        else
        {
            strcpy(lpStr, lpEmptyString);
        }

        lpConfig->lpBinaryPathName = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (strlen((LPSTR)lpStr) + 1);

        if (lpService->lpGroup)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpService->lpGroup->lpGroupName,
                                -1,
                                lpStr,
                                wcslen(lpService->lpGroup->lpGroupName),
                                0,
                                0);
        }
        else
        {
            strcpy(lpStr, lpEmptyString);
        }

        lpConfig->lpLoadOrderGroup = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (strlen(lpStr) + 1);

        /* FIXME: Append Dependencies */
        strcpy(lpStr, lpEmptyString);

        lpConfig->lpDependencies = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (strlen(lpStr) + 1);

        if (lpServiceStartName)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpServiceStartName,
                                -1,
                                lpStr,
                                wcslen(lpServiceStartName),
                                0,
                                0);
        }
        else
        {
            strcpy(lpStr, lpEmptyString);
        }

        lpConfig->lpServiceStartName = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        lpStr += (strlen(lpStr) + 1);

        if (lpService->lpDisplayName)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpService->lpDisplayName,
                                -1,
                                lpStr,
                                wcslen(lpService->lpDisplayName),
                                0,
                                0);
        }
        else
        {
            strcpy(lpStr, lpEmptyString);
        }

        lpConfig->lpDisplayName = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
    }

    if (pcbBytesNeeded != NULL)
        *pcbBytesNeeded = dwRequiredSize;

Done:;
    if (lpImagePath != NULL)
        HeapFree(GetProcessHeap(), 0, lpImagePath);

    if (lpServiceStartName != NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceStartName);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    /* FIXME: Unlock the service database */

    DPRINT("RQueryServiceConfigA() done\n");

    return dwError;
}


/* Function 30 */
DWORD RQueryServiceLockStatusA(
    SC_RPC_HANDLE hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_4K pcbBytesNeeded)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 31 */
DWORD RStartServiceA(
    SC_RPC_HANDLE hService,
    DWORD argc,
    LPSTRING_PTRSA argv)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;

    DPRINT1("RStartServiceA() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_START))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (lpService->dwStartType == SERVICE_DISABLED)
        return ERROR_SERVICE_DISABLED;

    if (lpService->bDeleted)
        return ERROR_SERVICE_MARKED_FOR_DELETE;

    /* FIXME: Convert argument vector to Unicode */

    /* Start the service */
    dwError = ScmStartService(lpService, 0, NULL);

    /* FIXME: Free argument vector */

    return dwError;
}


/* Function 32 */
DWORD RGetServiceDisplayNameA(
    SC_RPC_HANDLE hSCManager,
    LPSTR lpServiceName,
    LPSTR lpDisplayName,
    LPBOUNDED_DWORD_4K lpcchBuffer)
{
//    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;
    LPWSTR lpServiceNameW;

    DPRINT("RGetServiceDisplayNameA() called\n");
    DPRINT("hSCManager = %p\n", hSCManager);
    DPRINT("lpServiceName: %s\n", lpServiceName);
    DPRINT("lpDisplayName: %p\n", lpDisplayName);
    DPRINT("*lpcchBuffer: %lu\n", *lpcchBuffer);

//    hManager = (PMANAGER_HANDLE)hSCManager;
//    if (hManager->Handle.Tag != MANAGER_TAG)
//    {
//        DPRINT1("Invalid manager handle!\n");
//        return ERROR_INVALID_HANDLE;
//    }

    dwLength = strlen(lpServiceName) + 1;
    lpServiceNameW = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               dwLength * sizeof(WCHAR));
    if (!lpServiceNameW)
        return ERROR_NOT_ENOUGH_MEMORY;

    MultiByteToWideChar(CP_ACP,
                        0,
                        lpServiceName,
                        strlen(lpServiceName),
                        lpServiceNameW,
                        dwLength);

    lpService = ScmGetServiceEntryByName(lpServiceNameW);

    HeapFree(GetProcessHeap(), 0, lpServiceNameW);

    if (lpService == NULL)
    {
        DPRINT1("Could not find a service!\n");

        /* If the service could not be found and lpcchBuffer is 0, windows
           puts null in lpDisplayName and puts 1 in lpcchBuffer */
        if (*lpcchBuffer == 0)
        {
            *lpcchBuffer = 1;
            *lpDisplayName = '\0';
        }
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    if (!lpService->lpDisplayName)
    {
        dwLength = wcslen(lpService->lpServiceName);
        if (lpServiceName != NULL &&
            *lpcchBuffer > dwLength)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpService->lpServiceName,
                                wcslen(lpService->lpServiceName),
                                lpDisplayName,
                                *lpcchBuffer,
                                NULL,
                                NULL);
            return ERROR_SUCCESS;
        }
    }
    else
    {
        dwLength = wcslen(lpService->lpDisplayName);
        if (lpDisplayName != NULL &&
            *lpcchBuffer > dwLength)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpService->lpDisplayName,
                                wcslen(lpService->lpDisplayName),
                                lpDisplayName,
                                *lpcchBuffer,
                                NULL,
                                NULL);
            return ERROR_SUCCESS;
        }
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength * 2;

    return dwError;
}


/* Function 33 */
DWORD RGetServiceKeyNameA(
    SC_RPC_HANDLE hSCManager,
    LPSTR lpDisplayName,
    LPSTR lpServiceName,
    LPBOUNDED_DWORD_4K lpcchBuffer)
{
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;
    LPWSTR lpDisplayNameW;

    DPRINT("RGetServiceKeyNameA() called\n");
    DPRINT("hSCManager = %p\n", hSCManager);
    DPRINT("lpDisplayName: %s\n", lpDisplayName);
    DPRINT("lpServiceName: %p\n", lpServiceName);
    DPRINT("*lpcchBuffer: %lu\n", *lpcchBuffer);

    dwLength = strlen(lpDisplayName) + 1;
    lpDisplayNameW = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               dwLength * sizeof(WCHAR));
    if (!lpDisplayNameW)
        return ERROR_NOT_ENOUGH_MEMORY;

    MultiByteToWideChar(CP_ACP,
                        0,
                        lpDisplayName,
                        strlen(lpDisplayName),
                        lpDisplayNameW,
                        dwLength);

    lpService = ScmGetServiceEntryByDisplayName(lpDisplayNameW);

    HeapFree(GetProcessHeap(), 0, lpDisplayNameW);

    if (lpService == NULL)
    {
        DPRINT1("Could not find the service!\n");

        /* If the service could not be found and lpcchBuffer is 0,
           put null in lpDisplayName and puts 1 in lpcchBuffer, verified WINXP. */
        if (*lpcchBuffer == 0)
        {
            *lpcchBuffer = 1;
            *lpServiceName = '\0';
        }

        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    dwLength = wcslen(lpService->lpServiceName);
    if (lpService != NULL &&
        *lpcchBuffer > dwLength)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpService->lpServiceName,
                            wcslen(lpService->lpServiceName),
                            lpServiceName,
                            dwLength,
                            NULL,
                            NULL);
        return ERROR_SUCCESS;
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength * 2;

    return dwError;
}


/* Function 34 */
DWORD RI_ScGetCurrentGroupStateW(
    SC_RPC_HANDLE hSCManager,
    LPWSTR lpLoadOrderGroup,
    LPDWORD lpState)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 35 */
DWORD REnumServiceGroupW(
    SC_RPC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned,
    LPBOUNDED_DWORD_256K lpResumeIndex,
    LPCWSTR pszGroupName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 36 */
DWORD RChangeServiceConfig2A(
    SC_RPC_HANDLE hService,
    SC_RPC_CONFIG_INFOA Info)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 37 */
DWORD RChangeServiceConfig2W(
    SC_RPC_HANDLE hService,
    SC_RPC_CONFIG_INFOW Info)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;

    DPRINT("RChangeServiceConfig2W() called\n");
    DPRINT("dwInfoLevel = %lu\n", Info.dwInfoLevel);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_CHANGE_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock database exclusively */

    if (lpService->bDeleted)
    {
        /* FIXME: Unlock database */
        DPRINT1("The service has already been marked for delete!\n");
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    /* Open the service key */
    dwError = ScmOpenServiceKey(lpService->szServiceName,
                                KEY_SET_VALUE,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    if (Info.dwInfoLevel & SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONW lpServiceDescription;

        lpServiceDescription = (LPSERVICE_DESCRIPTIONW)&Info;
        lpServiceDescription->lpDescription = (LPWSTR)(&Info + sizeof(LPSERVICE_DESCRIPTIONW));

        if (lpServiceDescription != NULL &&
            lpServiceDescription->lpDescription != NULL)
        {
            RegSetValueExW(hServiceKey,
                           L"Description",
                           0,
                           REG_SZ,
                           (LPBYTE)lpServiceDescription->lpDescription,
                           (wcslen(lpServiceDescription->lpDescription) + 1) * sizeof(WCHAR));

            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }
    else if (Info.dwInfoLevel & SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        UNIMPLEMENTED;
        dwError = ERROR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

done:
    /* FIXME: Unlock database */
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    DPRINT("RChangeServiceConfig2W() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 38 */
DWORD RQueryServiceConfig2A(
    SC_RPC_HANDLE hService,
    DWORD dwInfoLevel,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_8K pcbBytesNeeded)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    DWORD dwRequiredSize;
    LPWSTR lpDescriptionW = NULL;
    LPSTR lpDescription = NULL;

    DPRINT("RQueryServiceConfig2W() called\n");

    if (!lpBuffer)
        return ERROR_INVALID_ADDRESS;

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service database shared */

    dwError = ScmOpenServiceKey(lpService->lpServiceName,
                                KEY_READ,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    if (dwInfoLevel & SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONA lpServiceDescription = (LPSERVICE_DESCRIPTIONA)lpBuffer;
        LPSTR lpStr;

        dwError = ScmReadString(hServiceKey,
                                L"Description",
                                &lpDescriptionW);
        if (dwError != ERROR_SUCCESS)
            goto done;

        dwRequiredSize = sizeof(SERVICE_DESCRIPTIONA) + ((wcslen(lpDescriptionW) + 1));

        if (cbBufSize < dwRequiredSize)
        {
            *pcbBytesNeeded = dwRequiredSize;
            dwError = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        lpStr = (LPSTR)(lpServiceDescription + 1);

        WideCharToMultiByte(CP_ACP,
                            0,
                            lpDescriptionW,
                            -1,
                            lpStr,
                            wcslen(lpDescriptionW),
                            NULL,
                            NULL);
        lpServiceDescription->lpDescription = (LPSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServiceDescription);
    }
    else if (dwInfoLevel & SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        UNIMPLEMENTED;
        dwError = ERROR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

done:
    if (lpDescription != NULL)
        HeapFree(GetProcessHeap(), 0, lpDescription);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    /* FIXME: Unlock database */

    DPRINT("RQueryServiceConfig2W() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 39 */
DWORD RQueryServiceConfig2W(
    SC_RPC_HANDLE hService,
    DWORD dwInfoLevel,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_8K pcbBytesNeeded)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    DWORD dwRequiredSize;
    LPWSTR lpDescription = NULL;

    DPRINT("RQueryServiceConfig2W() called\n");

    if (!lpBuffer)
        return ERROR_INVALID_ADDRESS;

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_CONFIG))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* FIXME: Lock the service database shared */

    dwError = ScmOpenServiceKey(lpService->lpServiceName,
                                KEY_READ,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    if (dwInfoLevel & SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONW lpServiceDescription = (LPSERVICE_DESCRIPTIONW)lpBuffer;
        LPWSTR lpStr;

        dwError = ScmReadString(hServiceKey,
                                L"Description",
                                &lpDescription);
        if (dwError != ERROR_SUCCESS)
            goto done;

        dwRequiredSize = sizeof(SERVICE_DESCRIPTIONW) + ((wcslen(lpDescription) + 1) * sizeof(WCHAR));

        if (cbBufSize < dwRequiredSize)
        {
            *pcbBytesNeeded = dwRequiredSize;
            dwError = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        lpStr = (LPWSTR)(lpServiceDescription + 1);
        wcscpy(lpStr, lpDescription);
        lpServiceDescription->lpDescription = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServiceDescription);
    }
    else if (dwInfoLevel & SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        UNIMPLEMENTED;
        dwError = ERROR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

done:
    if (lpDescription != NULL)
        HeapFree(GetProcessHeap(), 0, lpDescription);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    /* FIXME: Unlock database */

    DPRINT("RQueryServiceConfig2W() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 40 */
DWORD RQueryServiceStatusEx(
    SC_RPC_HANDLE hService,
    SC_STATUS_TYPE InfoLevel,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_8K pcbBytesNeeded)
{
    LPSERVICE_STATUS_PROCESS lpStatus;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;

    DPRINT("RQueryServiceStatusEx() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
        return ERROR_INVALID_LEVEL;

    *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);

    if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
        return ERROR_INSUFFICIENT_BUFFER;

    hSvc = (PSERVICE_HANDLE)hService;
    if (!hSvc || hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
        return ERROR_INVALID_HANDLE;
    }

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  SERVICE_QUERY_STATUS))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n", hSvc->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    lpStatus = (LPSERVICE_STATUS_PROCESS)lpBuffer;

    /* Return service status information */
    RtlCopyMemory(lpStatus,
                  &lpService->Status,
                  sizeof(SERVICE_STATUS));

    lpStatus->dwProcessId = lpService->ProcessId;	/* FIXME */
    lpStatus->dwServiceFlags = 0;			/* FIXME */

    return ERROR_SUCCESS;
}


/* Function 41 */
DWORD REnumServicesStatusExA(
    SC_RPC_HANDLE hSCManager,
    SC_ENUM_TYPE InfoLevel,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned,
    LPBOUNDED_DWORD_256K lpResumeIndex,
    LPCSTR pszGroupName)
{
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtrW = NULL;
    LPENUM_SERVICE_STATUS_PROCESSA lpStatusPtrA = NULL;
    LPWSTR lpStringPtrW;
    LPSTR lpStringPtrA;
    LPWSTR pszGroupNameW = NULL;
    DWORD dwError;
    DWORD dwServiceCount;

    DPRINT("REnumServicesStatusExA() called\n");

    if (pszGroupName)
    {
        pszGroupNameW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (strlen(pszGroupName) + 1) * sizeof(WCHAR));
        if (!pszGroupNameW)
        {
             DPRINT1("Failed to allocate buffer!\n");
             return ERROR_NOT_ENOUGH_MEMORY;
        }
        MultiByteToWideChar(CP_ACP,
                            0,
                            pszGroupName,
                            -1,
                            pszGroupNameW,
                            strlen(pszGroupName) + 1);
    }

    if ((cbBufSize > 0) && (lpBuffer))
    {
        lpStatusPtrW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBufSize);
        if (!lpStatusPtrW)
        {
            DPRINT1("Failed to allocate buffer!\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    dwError = REnumServicesStatusExW(hSCManager,
                                     InfoLevel,
                                     dwServiceType,
                                     dwServiceState,
                                     (LPBYTE)lpStatusPtrW,
                                     cbBufSize,
                                     pcbBytesNeeded,
                                     lpServicesReturned,
                                     lpResumeIndex,
                                     pszGroupNameW);

    /* if no services were returned then we are Done */
    if (*lpServicesReturned == 0) goto Done;

    lpStatusPtrA = (LPENUM_SERVICE_STATUS_PROCESSA)lpBuffer;
    lpStringPtrA = (LPSTR)((ULONG_PTR)lpBuffer +
                  *lpServicesReturned * sizeof(ENUM_SERVICE_STATUS_PROCESSA));
    lpStringPtrW = (LPWSTR)((ULONG_PTR)lpStatusPtrW + 
                  *lpServicesReturned * sizeof(ENUM_SERVICE_STATUS_PROCESSW));

    for (dwServiceCount = 0; dwServiceCount < *lpServicesReturned; dwServiceCount++)
    {
        /* Copy the service name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpStringPtrW,
                            -1,
                            lpStringPtrA,
                            wcslen(lpStringPtrW),
                            0,
                            0);

        lpStatusPtrA->lpServiceName = (LPSTR)((ULONG_PTR)lpStringPtrA - (ULONG_PTR)lpBuffer);
        lpStringPtrA += wcslen(lpStringPtrW) + 1;
        lpStringPtrW += wcslen(lpStringPtrW) + 1;

        /* Copy the display name */
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpStringPtrW,
                            -1,
                            lpStringPtrA,
                            wcslen(lpStringPtrW),
                            0,
                            0);

        lpStatusPtrA->lpDisplayName = (LPSTR)((ULONG_PTR)lpStringPtrA - (ULONG_PTR)lpBuffer);
        lpStringPtrA += wcslen(lpStringPtrW) + 1;
        lpStringPtrW += wcslen(lpStringPtrW) + 1;

        /* Copy the status information */
        memcpy(&lpStatusPtrA->ServiceStatusProcess,
               &lpStatusPtrW->ServiceStatusProcess,
               sizeof(SERVICE_STATUS));

        lpStatusPtrA->ServiceStatusProcess.dwProcessId = lpStatusPtrW->ServiceStatusProcess.dwProcessId; /* FIXME */
        lpStatusPtrA->ServiceStatusProcess.dwServiceFlags = 0; /* FIXME */
        lpStatusPtrA++;
    }

Done:;
    if (pszGroupNameW) HeapFree(GetProcessHeap(), 0, pszGroupNameW);

    if (lpStatusPtrW) HeapFree(GetProcessHeap(), 0, lpStatusPtrW);

    DPRINT("REnumServicesStatusExA() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 42 */
DWORD REnumServicesStatusExW(
    SC_RPC_HANDLE hSCManager,
    SC_ENUM_TYPE InfoLevel,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPBYTE lpBuffer,
    DWORD cbBufSize,
    LPBOUNDED_DWORD_256K pcbBytesNeeded,
    LPBOUNDED_DWORD_256K lpServicesReturned,
    LPBOUNDED_DWORD_256K lpResumeIndex,
    LPCWSTR pszGroupName)
{
    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwError = ERROR_SUCCESS;
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    DWORD dwState;
    DWORD dwRequiredSize;
    DWORD dwServiceCount;
    DWORD dwSize;
    DWORD dwLastResumeCount = 0;
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtr;
    LPWSTR lpStringPtr;

    DPRINT("REnumServicesStatusExW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (InfoLevel != SC_ENUM_PROCESS_INFO)
        return ERROR_INVALID_LEVEL;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (!hManager || hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    if ((dwServiceType!=SERVICE_DRIVER) && (dwServiceType!=SERVICE_WIN32))
    {
        DPRINT1("Not a valid Service Type!\n");
        return ERROR_INVALID_PARAMETER;
    }

    if ((dwServiceState<SERVICE_ACTIVE) || (dwServiceState>SERVICE_STATE_ALL))
    {
        DPRINT1("Not a valid Service State!\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hManager->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hManager->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    if (lpResumeIndex) dwLastResumeCount = *lpResumeIndex;

    /* Lock the service list shared */

    lpService = ScmGetServiceEntryByResumeCount(dwLastResumeCount);
    if (lpService == NULL)
    {
        dwError = ERROR_SUCCESS;
        goto Done;
    }

    dwRequiredSize = 0;
    dwServiceCount = 0;

    for (ServiceEntry = &lpService->ServiceListEntry;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        if (pszGroupName)
        {
            if (*pszGroupName == 0)
            {
                if (CurrentService->lpGroup != NULL)
                    continue;
            }
            else
            {
                if ((CurrentService->lpGroup == NULL) ||
                    _wcsicmp(pszGroupName, CurrentService->lpGroup->lpGroupName))
                    continue;
            }
        }

        dwSize = sizeof(ENUM_SERVICE_STATUS_PROCESSW) +
                 ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                 ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR));

        if (dwRequiredSize + dwSize <= cbBufSize)
        {
            DPRINT("Service name: %S  fit\n", CurrentService->lpServiceName);
            dwRequiredSize += dwSize;
            dwServiceCount++;
            dwLastResumeCount = CurrentService->dwResumeCount;
        }
        else
        {
            DPRINT("Service name: %S  no fit\n", CurrentService->lpServiceName);
            break;
        }

    }

    DPRINT("dwRequiredSize: %lu\n", dwRequiredSize);
    DPRINT("dwServiceCount: %lu\n", dwServiceCount);

    for (;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        if (pszGroupName)
        {
            if (*pszGroupName == 0)
            {
                if (CurrentService->lpGroup != NULL)
                    continue;
            }
            else
            {
                if ((CurrentService->lpGroup == NULL) ||
                    _wcsicmp(pszGroupName, CurrentService->lpGroup->lpGroupName))
                    continue;
            }
        }

        dwRequiredSize += (sizeof(ENUM_SERVICE_STATUS_PROCESSW) +
                           ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                           ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR)));

        dwError = ERROR_MORE_DATA;
    }

    DPRINT("*pcbBytesNeeded: %lu\n", dwRequiredSize);

    if (lpResumeIndex) *lpResumeIndex = dwLastResumeCount;
    *lpServicesReturned = dwServiceCount;
    *pcbBytesNeeded = dwRequiredSize;

    /* If there was no services that matched */
    if ((!dwServiceCount) && (dwError != ERROR_MORE_DATA))
    {
        dwError = ERROR_SERVICE_DOES_NOT_EXIST;
        goto Done;
    }

    lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSW)lpBuffer;
    lpStringPtr = (LPWSTR)((ULONG_PTR)lpBuffer +
                           dwServiceCount * sizeof(ENUM_SERVICE_STATUS_PROCESSW));

    dwRequiredSize = 0;
    for (ServiceEntry = &lpService->ServiceListEntry;
         ServiceEntry != &ServiceListHead;
         ServiceEntry = ServiceEntry->Flink)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        if ((CurrentService->Status.dwServiceType & dwServiceType) == 0)
            continue;

        dwState = SERVICE_ACTIVE;
        if (CurrentService->Status.dwCurrentState == SERVICE_STOPPED)
            dwState = SERVICE_INACTIVE;

        if ((dwState & dwServiceState) == 0)
            continue;

        if (pszGroupName)
        {
            if (*pszGroupName == 0)
            {
                if (CurrentService->lpGroup != NULL)
                    continue;
            }
            else
            {
                if ((CurrentService->lpGroup == NULL) ||
                    _wcsicmp(pszGroupName, CurrentService->lpGroup->lpGroupName))
                    continue;
            }
        }

        dwSize = sizeof(ENUM_SERVICE_STATUS_PROCESSW) +
                 ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                 ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR));

        if (dwRequiredSize + dwSize <= cbBufSize)
        {
            /* Copy the service name */
            wcscpy(lpStringPtr,
                   CurrentService->lpServiceName);
            lpStatusPtr->lpServiceName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpBuffer);
            lpStringPtr += (wcslen(CurrentService->lpServiceName) + 1);

            /* Copy the display name */
            wcscpy(lpStringPtr,
                   CurrentService->lpDisplayName);
            lpStatusPtr->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpBuffer);
            lpStringPtr += (wcslen(CurrentService->lpDisplayName) + 1);

            /* Copy the status information */
            memcpy(&lpStatusPtr->ServiceStatusProcess,
                   &CurrentService->Status,
                   sizeof(SERVICE_STATUS));
            lpStatusPtr->ServiceStatusProcess.dwProcessId = CurrentService->ProcessId; /* FIXME */
            lpStatusPtr->ServiceStatusProcess.dwServiceFlags = 0; /* FIXME */

            lpStatusPtr++;
            dwRequiredSize += dwSize;
        }
        else
        {
            break;
        }
    }

    if (dwError == 0) 
    {
        *pcbBytesNeeded = 0;
        if (lpResumeIndex) *lpResumeIndex = 0;
    }

Done:;
    /* Unlock the service list */

    DPRINT("REnumServicesStatusExW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 43 */
DWORD RSendTSMessage(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 44 */
DWORD RCreateServiceWOW64A(
    handle_t BindingHandle,
    LPSTR lpServiceName,
    LPSTR lpDisplayName,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPSTR lpBinaryPathName,
    LPSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPBYTE lpDependencies,
    DWORD dwDependSize,
    LPSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 45 */
DWORD RCreateServiceWOW64W(
    handle_t BindingHandle,
    LPWSTR lpServiceName,
    LPWSTR lpDisplayName,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPWSTR lpBinaryPathName,
    LPWSTR lpLoadOrderGroup,
    LPDWORD lpdwTagId,
    LPBYTE lpDependencies,
    DWORD dwDependSize,
    LPWSTR lpServiceStartName,
    LPBYTE lpPassword,
    DWORD dwPwSize,
    LPSC_RPC_HANDLE lpServiceHandle)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 46 */
DWORD RQueryServiceTagInfo(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 47 */
DWORD RNotifyServiceStatusChange(
    SC_RPC_HANDLE hService,
    SC_RPC_NOTIFY_PARAMS NotifyParams,
    GUID *pClientProcessGuid,
    GUID *pSCMProcessGuid,
    PBOOL pfCreateRemoteQueue,
    LPSC_NOTIFY_RPC_HANDLE phNotify)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 48 */
DWORD RGetNotifyResults(
    SC_NOTIFY_RPC_HANDLE hNotify,
    PSC_RPC_NOTIFY_PARAMS_LIST *ppNotifyParams)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 49 */
DWORD RCloseNotifyHandle(
    LPSC_NOTIFY_RPC_HANDLE phNotify,
    PBOOL pfApcFired)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 50 */
DWORD RControlServiceExA(
    SC_RPC_HANDLE hService,
    DWORD dwControl,
    DWORD dwInfoLevel)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 51 */
DWORD RControlServiceExW(
    SC_RPC_HANDLE hService,
    DWORD dwControl,
    DWORD dwInfoLevel)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 52 */
DWORD RSendPnPMessage(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 53 */
DWORD RValidatePnPService(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 54 */
DWORD ROpenServiceStatusHandle(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 55 */
DWORD RFunction55(
    handle_t BindingHandle)  /* FIXME */
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER SC_RPC_HANDLE_rundown(SC_RPC_HANDLE hSCObject)
{
}


void __RPC_USER SC_RPC_LOCK_rundown(SC_RPC_LOCK Lock)
{
}


void __RPC_USER SC_NOTIFY_RPC_HANDLE_rundown(SC_NOTIFY_RPC_HANDLE hNotify)
{
}

/* EOF */
