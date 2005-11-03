/*

 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

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

  Ptr = HeapAlloc(GetProcessHeap(),
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

  Ptr = HeapAlloc(GetProcessHeap(),
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

  DPRINT1("ScmrControlService() called\n");

  hSvc = (PSERVICE_HANDLE)hService;
  if (hSvc->Handle.Tag != SERVICE_TAG)
  {
    DPRINT1("Invalid handle tag!\n");
    return ERROR_INVALID_HANDLE;
  }


  /* FIXME: Check access rights */


  lpService = hSvc->ServiceEntry;
  if (lpService == NULL)
  {
    DPRINT1("lpService == NULL!\n");
    return ERROR_INVALID_HANDLE;
  }


  /* FIXME: Send control code to the service */


  /* Return service status information */
  lpServiceStatus->dwServiceType = lpService->Type;
  lpServiceStatus->dwCurrentState = lpService->CurrentState;
  lpServiceStatus->dwControlsAccepted = lpService->ControlsAccepted;
  lpServiceStatus->dwWin32ExitCode = lpService->Win32ExitCode;
  lpServiceStatus->dwServiceSpecificExitCode = lpService->ServiceSpecificExitCode;
  lpServiceStatus->dwCheckPoint = lpService->CheckPoint;
  lpServiceStatus->dwWaitHint = lpService->WaitHint;

  return ERROR_SUCCESS;
}


/* Function 2 */
unsigned long
ScmrDeleteService(handle_t BindingHandle,
                  unsigned int hService)
{
  PSERVICE_HANDLE hSvc;
  PSERVICE lpService;
  DWORD dwError;

  DPRINT1("ScmrDeleteService() called\n");

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

  /* Mark service for delete */
  dwError = ScmMarkServiceForDelete(lpService);

  /* FIXME: Release service database lock */

  DPRINT1("ScmrDeleteService() done\n");

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

  /* FIXME: Lock the database */
  *hLock = 0x12345678; /* Dummy! */

  return ERROR_SUCCESS;
}


/* Function 4 */
unsigned long
ScmrQueryServiceObjectSecurity(handle_t BindingHandle)
{
  DPRINT1("ScmrQueryServiceSecurity() is unimplemented\n");
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/* Function 5 */
unsigned long
ScmrSetServiceObjectSecurity(handle_t BindingHandle)
{
  DPRINT1("ScmrSetServiceSecurity() is unimplemented\n");
  return ERROR_CALL_NOT_IMPLEMENTED;
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
  lpServiceStatus->dwServiceType = lpService->Type;
  lpServiceStatus->dwCurrentState = lpService->CurrentState;
  lpServiceStatus->dwControlsAccepted = lpService->ControlsAccepted;
  lpServiceStatus->dwWin32ExitCode = lpService->Win32ExitCode;
  lpServiceStatus->dwServiceSpecificExitCode = lpService->ServiceSpecificExitCode;
  lpServiceStatus->dwCheckPoint = lpService->CheckPoint;
  lpServiceStatus->dwWaitHint = lpService->WaitHint;

  return ERROR_SUCCESS;
}


/* Function 7 */
unsigned long
ScmrSetServiceStatus(handle_t BindingHandle)
{
  DPRINT1("ScmrSetServiceStatus() is unimplemented\n");
 /* FIXME */
  return ERROR_CALL_NOT_IMPLEMENTED;
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
  return ERROR_SUCCESS;
}


#if 0
static DWORD
CreateServiceKey(LPWSTR lpServiceName, PHKEY phKey)
{
    HKEY hServicesKey = NULL;
    DWORD dwDisposition;
    DWORD dwError;

    *phKey = NULL;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_WRITE,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegCreateKeyExW(hServicesKey,
                              lpServiceName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              phKey,
                              &dwDisposition);
    if ((dwError == ERROR_SUCCESS) &&
        (dwDisposition == REG_OPENED_EXISTING_KEY))
    {
        RegCloseKey(*phKey);
        *phKey = NULL;
        dwError = ERROR_SERVICE_EXISTS;
    }

    RegCloseKey(hServicesKey);

    return dwError;
}
#endif


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
#if 0
    HKEY hServiceKey = NULL;
    LPWSTR lpImagePath = NULL;
#endif

    DPRINT1("ScmrCreateServiceW() called\n");
    DPRINT1("lpServiceName = %S\n", lpServiceName);
    DPRINT1("lpDisplayName = %S\n", lpDisplayName);
    DPRINT1("dwDesiredAccess = %lx\n", dwDesiredAccess);
    DPRINT1("dwServiceType = %lu\n", dwServiceType);
    DPRINT1("dwStartType = %lu\n", dwStartType);
    DPRINT1("dwErrorControl = %lu\n", dwErrorControl);
    DPRINT1("lpBinaryPathName = %S\n", lpBinaryPathName);
    DPRINT1("lpLoadOrderGroup = %S\n", lpLoadOrderGroup);

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

    /* FIXME: Fail if the service already exists! */

#if 0
    if (dwServiceType & SERVICE_DRIVER)
    {
        /* FIXME: Adjust the image path */
        lpImagePath = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                wcslen(lpBinaryPathName) + sizeof(WCHAR));
        if (lpImagePath == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        wcscpy(lpImagePath, lpBinaryPathName);
    }

    /* FIXME: Allocate and fill a service entry */

//    if (lpdwTagId != NULL)
//        *lpdwTagId = 0;

//    *hService = 0;


    /* Write service data to the registry */
    /* Create the service key */
    dwError = CreateServiceKey(lpServiceName, &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    if ((lpDisplayName != NULL) && (wcslen(lpDisplayName) > 0))
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
                                 REG_SZ,
                                 (LPBYTE)lpBinaryPathName,
                                 (wcslen(lpBinaryPathName) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
            goto done;
    }
    else if (dwServiceType & SERVICE_DRIVER)
    {
        /* FIXME: Adjust the path name */
        dwError = RegSetValueExW(hServiceKey,
                                 L"ImagePath",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)lpImagePath,
                                 (wcslen(lpImagePath) +  1) *sizeof(WCHAR));
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

done:;
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);

    if (lpImagePath != NULL)
        HeapFree(GetProcessHeap(), 0, lpImagePath);
#endif

    DPRINT1("ScmrCreateServiceW() done (Error %lu)\n", dwError);

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
unsigned int
ScmrOpenServiceW(handle_t BindingHandle,
                 unsigned int hSCManager,
                 wchar_t *lpServiceName,
                 unsigned long dwDesiredAccess,
                 unsigned int *hService)
{
  UNICODE_STRING ServiceName;
  PSERVICE lpService;
  PMANAGER_HANDLE hManager;
  SC_HANDLE hHandle;
  DWORD dwError;

  DPRINT("ScmrOpenServiceW() called\n");
  DPRINT("hSCManager = %x\n", hSCManager);
  DPRINT("lpServiceName = %p\n", lpServiceName);
  DPRINT("lpServiceName: %S\n", lpServiceName);
  DPRINT("dwDesiredAccess = %x\n", dwDesiredAccess);

  hManager = (PMANAGER_HANDLE)hSCManager;
  if (hManager->Handle.Tag != MANAGER_TAG)
  {
    DPRINT1("Invalid manager handle!\n");
    return ERROR_INVALID_HANDLE;
  }

  /* FIXME: Lock the service list */

  /* Get service database entry */
  RtlInitUnicodeString(&ServiceName,
                       lpServiceName);

  lpService = ScmGetServiceEntryByName(&ServiceName);
  if (lpService == NULL)
  {
    DPRINT1("Could not find a service!\n");
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



void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
  HeapFree(GetProcessHeap(), 0, ptr);
}

/* EOF */
