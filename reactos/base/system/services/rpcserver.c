/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/rpcserver.c
 * PURPOSE:     RPC server interface for the advapi32 calls
 * COPYRIGHT:   Copyright 2005-2006 Eric Kohl
 *              Copyright 2006-2007 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
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

    DPRINT("ScmStartRpcServer() called");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\ntsvcs",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT1("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(svcctl_ServerIfHandle,
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

    DPRINT("ScmStartRpcServer() done");
}


static DWORD
ScmCreateManagerHandle(LPWSTR lpDatabaseName,
                       SC_HANDLE *Handle)
{
    PMANAGER_HANDLE Ptr;

    if (lpDatabaseName == NULL)
        lpDatabaseName = SERVICES_ACTIVE_DATABASEW;

    Ptr = (MANAGER_HANDLE*) HeapAlloc(GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    sizeof(MANAGER_HANDLE) + wcslen(lpDatabaseName) * sizeof(WCHAR));
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


/* Function 0 */
unsigned long
ScmrCloseServiceHandle(handle_t BindingHandle,
                       unsigned int hScObject)
{
    PMANAGER_HANDLE hManager;

    DPRINT("ScmrCloseServiceHandle() called\n");

    DPRINT("hScObject = %X\n", hScObject);

    if (hScObject == 0)
        return ERROR_INVALID_HANDLE;

    hManager = (PMANAGER_HANDLE)hScObject;
    if (hManager->Handle.Tag == MANAGER_TAG)
    {
        DPRINT("Found manager handle\n");

        hManager->Handle.RefCount--;
        if (hManager->Handle.RefCount == 0)
        {
            /* FIXME: add cleanup code */

            HeapFree(GetProcessHeap(), 0, hManager);
        }

        DPRINT("ScmrCloseServiceHandle() done\n");
        return ERROR_SUCCESS;
    }
    else if (hManager->Handle.Tag == SERVICE_TAG)
    {
        DPRINT("Found service handle\n");

        hManager->Handle.RefCount--;
        if (hManager->Handle.RefCount == 0)
        {
            /* FIXME: add cleanup code */

            HeapFree(GetProcessHeap(), 0, hManager);
        }

        DPRINT("ScmrCloseServiceHandle() done\n");
        return ERROR_SUCCESS;
    }

    DPRINT1("Invalid handle tag (Tag %lx)\n", hManager->Handle.Tag);

    return ERROR_INVALID_HANDLE;
}


/* Function 1 */
unsigned long
ScmrControlService(handle_t BindingHandle,
                   unsigned int hService,
                   unsigned long dwControl,
                   LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ACCESS_MASK DesiredAccess;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("ScmrControlService() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    /* Check the service handle */
    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
    {
        DPRINT1("Invalid handle tag!\n");
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

    /* Check the service entry point */
    lpService = hSvc->ServiceEntry;
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
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

    /* Return service status information */
    RtlCopyMemory(lpServiceStatus,
                  &lpService->Status,
                  sizeof(SERVICE_STATUS));

    return dwError;
}


/* Function 2 */
unsigned long
ScmrDeleteService(handle_t BindingHandle,
                  unsigned int hService)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    DWORD dwError;

    DPRINT("ScmrDeleteService() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
        return ERROR_INVALID_HANDLE;

    if (!RtlAreAllAccessesGranted(hSvc->Handle.DesiredAccess,
                                  STANDARD_RIGHTS_REQUIRED))
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

    DPRINT("ScmrDeleteService() done\n");

    return dwError;
}


/* Function 3 */
unsigned long
ScmrLockServiceDatabase(handle_t BindingHandle,
                        unsigned int hSCManager,
                        unsigned int *hLock)
{
    PMANAGER_HANDLE hMgr;

    DPRINT("ScmrLockServiceDatabase() called\n");

    *hLock = 0;

    hMgr = (PMANAGER_HANDLE)hSCManager;
    if (hMgr->Handle.Tag != MANAGER_TAG)
        return ERROR_INVALID_HANDLE;

    if (!RtlAreAllAccessesGranted(hMgr->Handle.DesiredAccess,
                                  SC_MANAGER_LOCK))
        return ERROR_ACCESS_DENIED;

//    return ScmLockDatabase(0, hMgr->0xC, hLock);

    /* FIXME: Lock the database */
    *hLock = 0x12345678; /* Dummy! */

    return ERROR_SUCCESS;
}


/* Function 4 */
unsigned long
ScmrQueryServiceObjectSecurity(handle_t BindingHandle,
                               unsigned int hService,
                               unsigned long dwSecurityInformation,
                               unsigned char *lpSecurityDescriptor,
                               unsigned long dwSecuityDescriptorSize,
                               unsigned long *pcbBytesNeeded)
{
#if 0
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ULONG DesiredAccess = 0;
    NTSTATUS Status;
    DWORD dwBytesNeeded;
    DWORD dwError;

    DPRINT("ScmrQueryServiceObjectSecurity() called\n");

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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
    DPRINT1("ScmrQueryServiceObjectSecurity() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 5 */
unsigned long
ScmrSetServiceObjectSecurity(handle_t BindingHandle,
                             unsigned int hService,
                             unsigned long dwSecurityInformation,
                             unsigned char *lpSecurityDescriptor,
                             unsigned long dwSecuityDescriptorSize)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;
    ULONG DesiredAccess = 0;
    HANDLE hToken = NULL;
    HKEY hServiceKey;
    NTSTATUS Status;
    DWORD dwError;

    DPRINT1("ScmrSetServiceObjectSecurity() called\n");

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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

    DPRINT1("Stub: ScmrSetServiceObjectSecurity() is unimplemented\n");
    dwError = ERROR_SUCCESS;
//    dwError = ScmWriteSecurityDescriptor(hServiceKey,
//                                         lpService->lpSecurityDescriptor);

    RegFlushKey(hServiceKey);
    RegCloseKey(hServiceKey);

Done:

    if (hToken != NULL)
        NtClose(hToken);

    /* FIXME: Unlock service database */

    DPRINT("ScmrSetServiceObjectSecurity() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 6 */
unsigned long
ScmrQueryServiceStatus(handle_t BindingHandle,
                       unsigned int hService,
                       LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;

    DPRINT("ScmrQueryServiceStatus() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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


/* Function 7 */
unsigned long
ScmrSetServiceStatus(handle_t BindingHandle,
                     unsigned long hServiceStatus,
                     LPSERVICE_STATUS lpServiceStatus)
{
    PSERVICE lpService;

    DPRINT("ScmrSetServiceStatus() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    lpService = ScmGetServiceEntryByThreadId((ULONG)hServiceStatus);
    if (lpService == NULL)
    {
        DPRINT1("lpService == NULL!\n");
        return ERROR_INVALID_HANDLE;
    }

    RtlCopyMemory(&lpService->Status,
                  lpServiceStatus,
                  sizeof(SERVICE_STATUS));

    DPRINT("Set %S to %lu\n", lpService->lpDisplayName, lpService->Status.dwCurrentState);
    DPRINT("ScmrSetServiceStatus() done\n");

    return ERROR_SUCCESS;
}


/* Function 8 */
unsigned long
ScmrUnlockServiceDatabase(handle_t BindingHandle,
                          unsigned int hLock)
{
    DPRINT1("ScmrUnlockServiceDatabase() called\n");
    /* FIXME */
    return ERROR_SUCCESS;
}


/* Function 9 */
unsigned long
ScmrNotifyBootConfigStatus(handle_t BindingHandle,
                           unsigned long BootAcceptable)
{
    DPRINT1("ScmrNotifyBootConfigStatus() called\n");
    /* FIXME */
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 10 */
unsigned long
ScmrSetServiceBitsW(handle_t BindingHandle,
                    unsigned long hServiceStatus,
                    unsigned long dwServiceBits,
                    unsigned long bSetBitsOn,
                    unsigned long bUpdateImmediately,
                    wchar_t *lpString)
{
    DPRINT1("ScmrSetServiceBitsW() called\n");
    /* FIXME */
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 11 */
unsigned long
ScmrChangeServiceConfigW(handle_t BiningHandle,
                         unsigned int hService,
                         unsigned long dwServiceType,
                         unsigned long dwStartType,
                         unsigned long dwErrorControl,
                         wchar_t *lpBinaryPathName,
                         wchar_t *lpLoadOrderGroup,
                         unsigned long *lpdwTagId, /* in, out, unique */
                         wchar_t *lpDependencies,
                         unsigned long dwDependenciesLength,
                         wchar_t *lpServiceStartName,
                         wchar_t *lpPassword,
                         unsigned long dwPasswordLength,
                         wchar_t *lpDisplayName)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;

    DPRINT("ScmrChangeServiceConfigW() called\n");
    DPRINT("dwServiceType = %lu\n", dwServiceType);
    DPRINT("dwStartType = %lu\n", dwStartType);
    DPRINT("dwErrorControl = %lu\n", dwErrorControl);
    DPRINT("lpBinaryPathName = %S\n", lpBinaryPathName);
    DPRINT("lpLoadOrderGroup = %S\n", lpLoadOrderGroup);
    DPRINT("lpDisplayName = %S\n", lpDisplayName);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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
        /* FIXME: update lpService->lpDisplayName */
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
        /* FIXME: update lpService->lpServiceGroup */
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
                                       lpDependencies,
                                       dwDependenciesLength);
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

    DPRINT("ScmrChangeServiceConfigW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 12 */
unsigned long
ScmrCreateServiceW(handle_t BindingHandle,
                   unsigned int hSCManager,
                   wchar_t *lpServiceName,
                   wchar_t *lpDisplayName,
                   unsigned long dwDesiredAccess,
                   unsigned long dwServiceType,
                   unsigned long dwStartType,
                   unsigned long dwErrorControl,
                   wchar_t *lpBinaryPathName,
                   wchar_t *lpLoadOrderGroup,
                   unsigned long *lpdwTagId, /* in, out */
                   wchar_t *lpDependencies,
                   unsigned long dwDependenciesLength,
                   wchar_t *lpServiceStartName,
                   wchar_t *lpPassword,
                   unsigned long dwPasswordLength,
                   unsigned int *hService) /* out */
{
    PMANAGER_HANDLE hManager;
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE lpService = NULL;
    SC_HANDLE hServiceHandle = NULL;
    LPWSTR lpImagePath = NULL;
    HKEY hServiceKey = NULL;

    DPRINT("ScmrCreateServiceW() called\n");
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
    if (hManager->Handle.Tag != MANAGER_TAG)
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

    /* Fail if the service already exists! */
    if (ScmGetServiceEntryByName(lpServiceName) != NULL)
        return ERROR_SERVICE_EXISTS;

    if (dwServiceType & SERVICE_DRIVER)
    {
        /* FIXME: Adjust the image path
         * Following line is VERY BAD, because it assumes that the
         * first part of full file name is the OS directory */
        if (lpBinaryPathName[1] == ':') lpBinaryPathName += GetWindowsDirectoryW(NULL, 0);

        lpImagePath = (WCHAR*) HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (wcslen(lpBinaryPathName) + 1) * sizeof(WCHAR));
        if (lpImagePath == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        wcscpy(lpImagePath, lpBinaryPathName);
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
                                 (wcslen(lpImagePath) + 1) *sizeof(WCHAR));
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
                                       lpDependencies,
                                       dwDependenciesLength);
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

done:;
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    if (dwError == ERROR_SUCCESS)
    {
        DPRINT("hService %lx\n", hServiceHandle);
        *hService = (unsigned int)hServiceHandle;

        if (lpdwTagId != NULL)
            *lpdwTagId = lpService->dwTag;
    }
    else
    {
        /* Release the display name buffer */
        if (lpService->lpServiceName != NULL)
            HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

        if (hServiceHandle != NULL)
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

    DPRINT("ScmrCreateServiceW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 13 */
unsigned long
ScmrEnumDependentServicesW(handle_t BindingHandle,
                           unsigned int hService,
                           unsigned long dwServiceState,
                           unsigned char *lpServices,
                           unsigned long cbBufSize,
                           unsigned long *pcbBytesNeeded,
                           unsigned long *lpServicesReturned)
{
    DWORD dwError = ERROR_SUCCESS;

    DPRINT1("ScmrEnumDependentServicesW() called\n");
    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    DPRINT1("ScmrEnumDependentServicesW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 14 */
unsigned long
ScmrEnumServicesStatusW(handle_t BindingHandle,
                        unsigned int hSCManager,
                        unsigned long dwServiceType,
                        unsigned long dwServiceState,
                        unsigned char *lpServices,
                        unsigned long dwBufSize,
                        unsigned long *pcbBytesNeeded,
                        unsigned long *lpServicesReturned,
                        unsigned long *lpResumeHandle)
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
    DWORD dwLastResumeCount;
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    LPWSTR lpStringPtr;

    DPRINT("ScmrEnumServicesStatusW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hManager->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hManager->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    dwLastResumeCount = *lpResumeHandle;

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

        if (dwRequiredSize + dwSize <= dwBufSize)
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

        dwRequiredSize += (sizeof(ENUM_SERVICE_STATUSW) +
                           ((wcslen(CurrentService->lpServiceName) + 1) * sizeof(WCHAR)) +
                           ((wcslen(CurrentService->lpDisplayName) + 1) * sizeof(WCHAR)));

        dwError = ERROR_MORE_DATA;
    }

    DPRINT("*pcbBytesNeeded: %lu\n", dwRequiredSize);

    *lpResumeHandle = dwLastResumeCount;
    *lpServicesReturned = dwServiceCount;
    *pcbBytesNeeded = dwRequiredSize;

    lpStatusPtr = (LPENUM_SERVICE_STATUSW)lpServices;
    lpStringPtr = (LPWSTR)((ULONG_PTR)lpServices +
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

        if (dwRequiredSize + dwSize <= dwBufSize)
        {
            /* Copy the service name */
            wcscpy(lpStringPtr,
                   CurrentService->lpServiceName);
            lpStatusPtr->lpServiceName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpServices);
            lpStringPtr += (wcslen(CurrentService->lpServiceName) + 1);

            /* Copy the display name */
            wcscpy(lpStringPtr,
                   CurrentService->lpDisplayName);
            lpStatusPtr->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpServices);
            lpStringPtr += (wcslen(CurrentService->lpDisplayName) + 1);

            /* Copy the status information */
            memcpy(&lpStatusPtr->ServiceStatus,
                   &CurrentService->Status,
                   sizeof(SERVICE_STATUS));

            lpStatusPtr++;
            dwRequiredSize += dwSize;
        }
        else
        {
            break;
        }

    }

Done:;
    /* FIXME: Unlock the service list */

    DPRINT("ScmrEnumServicesStatusW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 15 */
unsigned long
ScmrOpenSCManagerW(handle_t BindingHandle,
                   wchar_t *lpMachineName,
                   wchar_t *lpDatabaseName,
                   unsigned long dwDesiredAccess,
                   unsigned int *hScm)
{
    DWORD dwError;
    SC_HANDLE hHandle;

    DPRINT("ScmrOpenSCManagerW() called\n");
    DPRINT("lpMachineName = %p\n", lpMachineName);
    DPRINT("lpMachineName: %S\n", lpMachineName);
    DPRINT("lpDataBaseName = %p\n", lpDatabaseName);
    DPRINT("lpDataBaseName: %S\n", lpDatabaseName);
    DPRINT("dwDesiredAccess = %x\n", dwDesiredAccess);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

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

    *hScm = (unsigned int)hHandle;
    DPRINT("*hScm = %x\n", *hScm);

    DPRINT("ScmrOpenSCManagerW() done\n");

    return ERROR_SUCCESS;
}


/* Function 16 */
unsigned long
ScmrOpenServiceW(handle_t BindingHandle,
                 unsigned int hSCManager,
                 wchar_t *lpServiceName,
                 unsigned long dwDesiredAccess,
                 unsigned int *hService)
{
    PSERVICE lpService;
    PMANAGER_HANDLE hManager;
    SC_HANDLE hHandle;
    DWORD dwError;

    DPRINT("ScmrOpenServiceW() called\n");
    DPRINT("hSCManager = %x\n", hSCManager);
    DPRINT("lpServiceName = %p\n", lpServiceName);
    DPRINT("lpServiceName: %S\n", lpServiceName);
    DPRINT("dwDesiredAccess = %x\n", dwDesiredAccess);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (hManager->Handle.Tag != MANAGER_TAG)
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

    *hService = (unsigned int)hHandle;
    DPRINT("*hService = %x\n", *hService);

    DPRINT("ScmrOpenServiceW() done\n");

    return ERROR_SUCCESS;
}


/* Function 17 */
unsigned long
ScmrQueryServiceConfigW(handle_t BindingHandle,
                        unsigned int hService,
                        unsigned char *lpServiceConfig,
                        unsigned long cbBufSize,
                        unsigned long *pcbBytesNeeded)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    LPWSTR lpImagePath = NULL;
    LPWSTR lpServiceStartName = NULL;
    DWORD dwRequiredSize;
    LPQUERY_SERVICE_CONFIGW lpConfig;
    LPWSTR lpStr;

    DPRINT("ScmrQueryServiceConfigW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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

    if (lpService->lpGroup != NULL)
        dwRequiredSize += ((wcslen(lpService->lpGroup->lpGroupName) + 1) * sizeof(WCHAR));

    /* FIXME: Add Dependencies length*/

    if (lpServiceStartName != NULL)
        dwRequiredSize += ((wcslen(lpServiceStartName) + 1) * sizeof(WCHAR));

    if (lpService->lpDisplayName != NULL)
        dwRequiredSize += ((wcslen(lpService->lpDisplayName) + 1) * sizeof(WCHAR));

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
            lpConfig->lpBinaryPathName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
            lpStr += (wcslen(lpImagePath) + 1);
        }
        else
        {
            lpConfig->lpBinaryPathName = NULL;
        }

        if (lpService->lpGroup != NULL)
        {
            wcscpy(lpStr, lpService->lpGroup->lpGroupName);
            lpConfig->lpLoadOrderGroup = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
            lpStr += (wcslen(lpService->lpGroup->lpGroupName) + 1);
        }
        else
        {
            lpConfig->lpLoadOrderGroup = NULL;
        }

        /* FIXME: Append Dependencies */
        lpConfig->lpDependencies = NULL;

        if (lpServiceStartName != NULL)
        {
            wcscpy(lpStr, lpServiceStartName);
            lpConfig->lpServiceStartName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
            lpStr += (wcslen(lpServiceStartName) + 1);
        }
        else
        {
            lpConfig->lpServiceStartName = NULL;
        }

        if (lpService->lpDisplayName != NULL)
        {
            wcscpy(lpStr, lpService->lpDisplayName);
            lpConfig->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpConfig);
        }
        else
        {
            lpConfig->lpDisplayName = NULL;
        }
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

    DPRINT("ScmrQueryServiceConfigW() done\n");

    return dwError;
}


/* Function 18 */
unsigned long
ScmrQueryServiceLockStatusW(handle_t BindingHandle,
                            unsigned int hSCManager,
                            unsigned char *lpLockStatus,   /* [out, unique, size_is(cbBufSize)] */
                            unsigned long cbBufSize,       /* [in] */
                            unsigned long *pcbBytesNeeded) /* [out] */
{
    DPRINT1("ScmrQueryServiceLockStatusW() called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 19 */
unsigned long
ScmrStartServiceW(handle_t BindingHandle,
                  unsigned int hService,
                  unsigned long dwNumServiceArgs,
                  unsigned char *lpServiceArgBuffer,
                  unsigned long cbBufSize)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;

    DPRINT("ScmrStartServiceW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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

    /* Start the service */
    dwError = ScmStartService(lpService, (LPWSTR)lpServiceArgBuffer);

    return dwError;
}


/* Function 20 */
unsigned long
ScmrGetServiceDisplayNameW(handle_t BindingHandle,
                           unsigned int hSCManager,
                           wchar_t *lpServiceName,
                           wchar_t *lpDisplayName, /* [out, unique] */
                           unsigned long *lpcchBuffer)
{
//    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;

    DPRINT("ScmrGetServiceDisplayNameW() called\n");
    DPRINT("hSCManager = %x\n", hSCManager);
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
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    dwLength = wcslen(lpService->lpDisplayName) + 1;

    if (lpDisplayName != NULL &&
        *lpcchBuffer >= dwLength)
    {
        wcscpy(lpDisplayName, lpService->lpDisplayName);
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength;

    return dwError;
}


/* Function 21 */
unsigned long
ScmrGetServiceKeyNameW(handle_t BindingHandle,
                       unsigned int hSCManager,
                       wchar_t *lpDisplayName,
                       wchar_t *lpServiceName, /* [out, unique] */
                       unsigned long *lpcchBuffer)
{
//    PMANAGER_HANDLE hManager;
    PSERVICE lpService;
    DWORD dwLength;
    DWORD dwError;

    DPRINT("ScmrGetServiceKeyNameW() called\n");
    DPRINT("hSCManager = %x\n", hSCManager);
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
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    dwLength = wcslen(lpService->lpServiceName) + 1;

    if (lpServiceName != NULL &&
        *lpcchBuffer >= dwLength)
    {
        wcscpy(lpServiceName, lpService->lpServiceName);
    }

    dwError = (*lpcchBuffer > dwLength) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;

    *lpcchBuffer = dwLength;

    return dwError;
}


/* Function 22 */
unsigned long
ScmrSetServiceBitsA(handle_t BindingHandle,
                    unsigned long hServiceStatus,
                    unsigned long dwServiceBits,
                    unsigned long bSetBitsOn,
                    unsigned long bUpdateImmediately,
                    char *lpString)
{
    DPRINT1("ScmrSetServiceBitsA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 23 */
unsigned long
ScmrChangeServiceConfigA(handle_t BiningHandle,
                         unsigned int hService,
                         unsigned long dwServiceType,
                         unsigned long dwStartType,
                         unsigned long dwErrorControl,
                         char *lpBinaryPathName,
                         char *lpLoadOrderGroup,
                         unsigned long *lpdwTagId,
                         char *lpDependencies,
                         unsigned long dwDependenciesLength,
                         char *lpServiceStartName,
                         char *lpPassword,
                         unsigned long dwPasswordLength,
                         char *lpDisplayName)
{
    DPRINT1("ScmrChangeServiceConfigA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 24 */
unsigned long
ScmrCreateServiceA(handle_t BindingHandle,
                   unsigned int hSCManager,
                   char *lpServiceName,
                   char *lpDisplayName,
                   unsigned long dwDesiredAccess,
                   unsigned long dwServiceType,
                   unsigned long dwStartType,
                   unsigned long dwErrorControl,
                   char *lpBinaryPathName,
                   char *lpLoadOrderGroup,
                   unsigned long *lpdwTagId, /* in, out */
                   char *lpDependencies,
                   unsigned long dwDependenciesLength,
                   char *lpServiceStartName,
                   char *lpPassword,
                   unsigned long dwPasswordLength,
                   unsigned int *hService) /* out */
{
    DPRINT1("ScmrCreateServiceA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 25 */
unsigned long
ScmrEnumDependentServicesA(handle_t BindingHandle,
                           unsigned int hService,
                           unsigned long dwServiceState,
                           unsigned char *lpServices,
                           unsigned long cbBufSize,
                           unsigned long *pcbBytesNeeded,
                           unsigned long *lpServicesReturned)
{
    DPRINT1("ScmrEnumDependentServicesA() is unimplemented\n");
    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 26 */
unsigned long
ScmrEnumServicesStatusA(handle_t BindingHandle,
                        unsigned int hSCManager,
                        unsigned long dwServiceType,
                        unsigned long dwServiceState,
                        unsigned char *lpServices,
                        unsigned long dwBufSize,
                        unsigned long *pcbBytesNeeded,
                        unsigned long *lpServicesReturned,
                        unsigned long *lpResumeHandle)
{
    DPRINT1("ScmrEnumServicesAtatusA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 27 */
unsigned long
ScmrOpenSCManagerA(handle_t BindingHandle,
                   char *lpMachineName,
                   char *lpDatabaseName,
                   unsigned long dwDesiredAccess,
                   unsigned int *hScm)
{
    UNICODE_STRING MachineName;
    UNICODE_STRING DatabaseName;
    DWORD dwError;

    DPRINT("ScmrOpenSCManagerA() called\n");

    if (lpMachineName)
        RtlCreateUnicodeStringFromAsciiz(&MachineName,
                                         lpMachineName);

    if (lpDatabaseName)
        RtlCreateUnicodeStringFromAsciiz(&DatabaseName,
                                         lpDatabaseName);

    dwError = ScmrOpenSCManagerW(BindingHandle,
                                 lpMachineName ? MachineName.Buffer : NULL,
                                 lpDatabaseName ? DatabaseName.Buffer : NULL,
                                 dwDesiredAccess,
                                 hScm);

    if (lpMachineName)
        RtlFreeUnicodeString(&MachineName);

    if (lpDatabaseName)
        RtlFreeUnicodeString(&DatabaseName);

    return dwError;
}


/* Function 28 */
unsigned int
ScmrOpenServiceA(handle_t BindingHandle,
                 unsigned int hSCManager,
                 char *lpServiceName,
                 unsigned long dwDesiredAccess,
                 unsigned int *hService)
{
    UNICODE_STRING ServiceName;
    DWORD dwError;

    DPRINT("ScmrOpenServiceA() called\n");

    RtlCreateUnicodeStringFromAsciiz(&ServiceName,
                                     lpServiceName);

    dwError = ScmrOpenServiceW(BindingHandle,
                               hSCManager,
                               ServiceName.Buffer,
                               dwDesiredAccess,
                               hService);

    RtlFreeUnicodeString(&ServiceName);

    return dwError;
}


/* Function 29 */
unsigned long
ScmrQueryServiceConfigA(handle_t BindingHandle,
                        unsigned int hService,
                        unsigned char *lpServiceConfig,
                        unsigned long cbBufSize,
                        unsigned long *pcbBytesNeeded)
{
    DPRINT1("ScmrQueryServiceConfigA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 30 */
unsigned long
ScmrQueryServiceLockStatusA(handle_t BindingHandle,
                            unsigned int hSCManager,
                            unsigned char *lpLockStatus,   /* [out, unique, size_is(cbBufSize)] */
                            unsigned long cbBufSize,       /* [in] */
                            unsigned long *pcbBytesNeeded) /* [out] */
{
    DPRINT1("ScmrQueryServiceLockStatusA() called\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 31 */
unsigned long
ScmrStartServiceA(handle_t BindingHandle,
                  unsigned int hService,
                  unsigned long dwNumServiceArgs,
                  unsigned char *lpServiceArgBuffer,
                  unsigned long cbBufSize)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;

    DPRINT1("ScmrStartServiceA() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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
    dwError = ScmStartService(lpService, NULL);

    /* FIXME: Free argument vector */

    return dwError;
}


/* Function 32 */
unsigned long
ScmrGetServiceDisplayNameA(handle_t BindingHandle,
                           unsigned int hSCManager,
                           char *lpServiceName,
                           char *lpDisplayName, /* [out, unique] */
                           unsigned long *lpcchBuffer)
{
    DPRINT1("ScmrGetServiceDisplayNameA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 33 */
unsigned long
ScmrGetServiceKeyNameA(handle_t BindingHandle,
                       unsigned int hSCManager,
                       char *lpDisplayName,
                       char *lpServiceName, /* [out, unique] */
                       unsigned long *lpcchBuffer)
{
    DPRINT1("ScmrGetServiceKeyNameA() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 34 */
unsigned long
ScmrGetCurrentGroupStateW(handle_t BindingHandle)
{
    DPRINT1("ScmrGetCurrentGroupStateW() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 35 */
unsigned long
ScmrEnumServiceGroupW(handle_t BindingHandle)
{
    DPRINT1("ScmrEnumServiceGroupW() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 36 */
unsigned long
ScmrChangeServiceConfig2A(handle_t BindingHandle,
                          unsigned int hService,
                          unsigned long dwInfoLevel,
                          unsigned char *lpInfo,
                          unsigned long dwInfoSize)
{
    DPRINT1("ScmrChangeServiceConfig2A() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 37 */
unsigned long
ScmrChangeServiceConfig2W(handle_t BindingHandle,
                          unsigned int hService,
                          unsigned long dwInfoLevel,
                          unsigned char *lpInfo,
                          unsigned long dwInfoSize)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;

    DPRINT("ScmrChangeServiceConfig2W() called\n");
    DPRINT("dwInfoLevel = %lu\n", dwInfoLevel);
    DPRINT("dwInfoSize = %lu\n", dwInfoSize);

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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

    if (dwInfoLevel & SERVICE_CONFIG_DESCRIPTION)
    {
        LPSERVICE_DESCRIPTIONW lpServiceDescription = (LPSERVICE_DESCRIPTIONW)lpInfo;

        if (dwInfoSize != sizeof(*lpServiceDescription))
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto done;
        }

        if (lpServiceDescription != NULL && lpServiceDescription->lpDescription != NULL)
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
    else if (dwInfoLevel & SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        DPRINT1("SERVICE_CONFIG_FAILURE_ACTIONS not implemented\n");
        dwError = ERROR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

done:
    /* FIXME: Unlock database */
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    DPRINT("ScmrChangeServiceConfigW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 38 */
unsigned long
ScmrQueryServiceConfig2A(handle_t BindingHandle,
                         unsigned int hService,
                         unsigned long dwInfoLevel,
                         unsigned char *lpBuffer,
                         unsigned long cbBufSize,
                         unsigned long *pcbBytesNeeded)
{
    DPRINT1("ScmrQueryServiceConfig2A() is unimplemented\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 39 */
unsigned long
ScmrQueryServiceConfig2W(handle_t BindingHandle,
                         unsigned int hService,
                         unsigned long dwInfoLevel,
                         unsigned char *lpBuffer,
                         unsigned long cbBufSize,
                         unsigned long *pcbBytesNeeded)
{
    DWORD dwError = ERROR_SUCCESS;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService = NULL;
    HKEY hServiceKey = NULL;
    DWORD dwRequiredSize;
    LPWSTR lpDescription = NULL;

    DPRINT("ScmrQueryServiceConfig2W() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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
        else
        {
            lpStr = (LPWSTR)(lpServiceDescription + 1);
            wcscpy(lpStr, lpDescription);
            lpServiceDescription->lpDescription = (LPWSTR)((ULONG_PTR)lpStr - (ULONG_PTR)lpServiceDescription);
        }
    }
    else if (dwInfoLevel & SERVICE_CONFIG_FAILURE_ACTIONS)
    {
        DPRINT1("SERVICE_CONFIG_FAILURE_ACTIONS not implemented\n");
        dwError = ERROR_CALL_NOT_IMPLEMENTED;
        goto done;
    }

done:
    if (lpDescription != NULL)
        HeapFree(GetProcessHeap(), 0, lpDescription);

    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    /* FIXME: Unlock database */

    DPRINT("ScmrQueryServiceConfig2W() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 40 */
unsigned long
ScmrQueryServiceStatusEx(handle_t BindingHandle,
                         unsigned int hService,
                         unsigned long InfoLevel,
                         unsigned char *lpBuffer, /* out */
                         unsigned long cbBufSize,
                         unsigned long *pcbBytesNeeded) /* out */
{
    LPSERVICE_STATUS_PROCESS lpStatus;
    PSERVICE_HANDLE hSvc;
    PSERVICE lpService;

    DPRINT("ScmrQueryServiceStatusEx() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
        return ERROR_INVALID_LEVEL;

    *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);

    if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
        return ERROR_INSUFFICIENT_BUFFER;

    hSvc = (PSERVICE_HANDLE)hService;
    if (hSvc->Handle.Tag != SERVICE_TAG)
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
unsigned long
ScmrEnumServicesStatusExA(handle_t BindingHandle,
                          unsigned int hSCManager,
                          unsigned long InfoLevel,
                          unsigned long dwServiceType,
                          unsigned long dwServiceState,
                          unsigned char *lpServices,
                          unsigned long dwBufSize,
                          unsigned long *pcbBytesNeeded,
                          unsigned long *lpServicesReturned,
                          unsigned long *lpResumeHandle,
                          char *pszGroupName)
{
    DPRINT1("ScmrEnumServicesStatusExA() is unimplemented\n");
    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 42 */
unsigned long
ScmrEnumServicesStatusExW(handle_t BindingHandle,
                          unsigned int hSCManager,
                          unsigned long InfoLevel,
                          unsigned long dwServiceType,
                          unsigned long dwServiceState,
                          unsigned char *lpServices,
                          unsigned long dwBufSize,
                          unsigned long *pcbBytesNeeded,
                          unsigned long *lpServicesReturned,
                          unsigned long *lpResumeHandle,
                          wchar_t *pszGroupName)
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
    DWORD dwLastResumeCount;
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtr;
    LPWSTR lpStringPtr;

    DPRINT("ScmrEnumServicesStatusExW() called\n");

    if (ScmShutdown)
        return ERROR_SHUTDOWN_IN_PROGRESS;

    if (InfoLevel != SC_ENUM_PROCESS_INFO)
        return ERROR_INVALID_LEVEL;

    hManager = (PMANAGER_HANDLE)hSCManager;
    if (hManager->Handle.Tag != MANAGER_TAG)
    {
        DPRINT1("Invalid manager handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    /* Check access rights */
    if (!RtlAreAllAccessesGranted(hManager->Handle.DesiredAccess,
                                  SC_MANAGER_ENUMERATE_SERVICE))
    {
        DPRINT1("Insufficient access rights! 0x%lx\n",
                hManager->Handle.DesiredAccess);
        return ERROR_ACCESS_DENIED;
    }

    *pcbBytesNeeded = 0;
    *lpServicesReturned = 0;

    dwLastResumeCount = *lpResumeHandle;

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

        if (dwRequiredSize + dwSize <= dwBufSize)
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

    *lpResumeHandle = dwLastResumeCount;
    *lpServicesReturned = dwServiceCount;
    *pcbBytesNeeded = dwRequiredSize;

    lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSW)lpServices;
    lpStringPtr = (LPWSTR)((ULONG_PTR)lpServices +
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

        if (dwRequiredSize + dwSize <= dwBufSize)
        {
            /* Copy the service name */
            wcscpy(lpStringPtr,
                   CurrentService->lpServiceName);
            lpStatusPtr->lpServiceName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpServices);
            lpStringPtr += (wcslen(CurrentService->lpServiceName) + 1);

            /* Copy the display name */
            wcscpy(lpStringPtr,
                   CurrentService->lpDisplayName);
            lpStatusPtr->lpDisplayName = (LPWSTR)((ULONG_PTR)lpStringPtr - (ULONG_PTR)lpServices);
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

Done:;
    /* Unlock the service list */

    DPRINT("ScmrEnumServicesStatusExW() done (Error %lu)\n", dwError);

    return dwError;
}


/* Function 43 */
/* ScmrSendTSMessage */


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

/* EOF */
