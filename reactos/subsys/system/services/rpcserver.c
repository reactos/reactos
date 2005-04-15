/*

 */

/* INCLUDES ****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <stdio.h>
#include <windows.h>

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
  PVOID DatabaseEntry; /* FIXME */

  /* FIXME: Insert more data here */

} SERVICE_HANDLE, *PSERVICE_HANDLE;


/* FUNCTIONS ***************************************************************/

VOID
ScmStartRpcServer(VOID)
{
  RPC_STATUS Status;

  DPRINT("ScmStartRpcServer() called");

  Status = RpcServerUseProtseqEp(L"ncacn_np",
                                 10,
                                 L"\\pipe\\ntsvcs",
                                 NULL);
  if (Status != RPC_S_OK)
  {
    DPRINT1("RpcServerUseProtseqEp() failed (Status %lx)\n", Status);
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
                       SC_HANDLE *Handle,
                       DWORD dwDesiredAccess)
{
  PMANAGER_HANDLE Ptr;

  Ptr = GlobalAlloc(GPTR,
                    sizeof(MANAGER_HANDLE) + wcslen(lpDatabaseName) * sizeof(WCHAR));
  if (Ptr == NULL)
    return ERROR_NOT_ENOUGH_MEMORY;

  Ptr->Handle.Tag = MANAGER_TAG;
  Ptr->Handle.RefCount = 1;
  Ptr->Handle.DesiredAccess = dwDesiredAccess;

  /* FIXME: initialize more data here */

  wcscpy(Ptr->DatabaseName, lpDatabaseName);

  *Handle = (SC_HANDLE)Ptr;

  return ERROR_SUCCESS;
}


static DWORD
ScmCreateServiceHandle(LPVOID lpDatabaseEntry,
                       SC_HANDLE *Handle,
                       DWORD dwDesiredAccess)
{
  PMANAGER_HANDLE Ptr;

  Ptr = GlobalAlloc(GPTR,
                    sizeof(SERVICE_HANDLE));
  if (Ptr == NULL)
    return ERROR_NOT_ENOUGH_MEMORY;

  Ptr->Handle.Tag = SERVICE_TAG;
  Ptr->Handle.RefCount = 1;
  Ptr->Handle.DesiredAccess = dwDesiredAccess;

  /* FIXME: initialize more data here */
  // Ptr->DatabaseEntry = lpDatabaseEntry;

  *Handle = (SC_HANDLE)Ptr;

  return ERROR_SUCCESS;
}


/* Service 0 */
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

      GlobalFree(hManager);
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

      GlobalFree(hManager);
    }

    DPRINT("ScmrCloseServiceHandle() done\n");
    return ERROR_SUCCESS;
  }

  DPRINT1("Invalid handle tag (Tag %lx)\n", hManager->Handle.Tag);

  return ERROR_INVALID_HANDLE;
}


/* Service 1 */
#if 0
unsigned long
ScmrControlService(handle_t BindingHandle,
                   unsigned int hService,
                   unsigned long dwControl,
                   LPSERVICE_STATUS lpServiceStatus)
{
  DPRINT("ScmrControlService() called\n");

#if 0
  lpServiceStatus->dwServiceType = 0x12345678;
  lpServiceStatus->dwCurrentState = 0x98765432;
  lpServiceStatus->dwControlsAccepted = 0xdeadbabe;
  lpServiceStatus->dwWin32ExitCode = 0xbaadf00d;
  lpServiceStatus->dwServiceSpecificExitCode = 0xdeadf00d;
  lpServiceStatus->dwCheckPoint = 0xbaadbabe;
  lpServiceStatus->dwWaitHint = 0x2468ACE1;
#endif

  return TRUE;
}
#endif


/* Service 2 */
unsigned long
ScmrDeleteService(handle_t BindingHandle,
                  unsigned int hService)
{
  DPRINT("ScmrDeleteService() called\n");
  return ERROR_SUCCESS;
}


unsigned long
ScmrOpenSCManagerA(handle_t BindingHandle,
                   char *lpMachineName,
                   char *lpDatabaseName,
                   unsigned long dwDesiredAccess,
                   unsigned int *hScm)
{
  DPRINT("ScmrOpenSCManagerA() called\n");
  return ERROR_SUCCESS;
}


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
                                   &hHandle,
                                   dwDesiredAccess);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT1("ScmCreateManagerHandle() failed (Error %lu)\n", dwError);
    return dwError;
  }

  *hScm = (unsigned int)hHandle;
  DPRINT("*hScm = %x\n", *hScm);

  DPRINT("ScmrOpenSCManagerW() done\n");

  return ERROR_SUCCESS;
}


unsigned int
ScmrOpenServiceA(handle_t BindingHandle,
                 unsigned int hSCManager,
                 char *lpServiceName,
                 unsigned long dwDesiredAccess,
                 unsigned int *hService)
{
  DPRINT("ScmrOpenServiceA() called\n");
  return 0;
}


unsigned int
ScmrOpenServiceW(handle_t BindingHandle,
                 unsigned int hSCManager,
                 wchar_t *lpServiceName,
                 unsigned long dwDesiredAccess,
                 unsigned int *hService)
{
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

  /* FIXME: Check desired access */

  /* FIXME: Get service database entry */

  /* Create a service handle */
  dwError = ScmCreateServiceHandle(NULL,
                                   &hHandle,
                                   dwDesiredAccess);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT1("ScmCreateServiceHandle() failed (Error %lu)\n", dwError);
    return dwError;
  }

  *hService = (unsigned int)hHandle;
  DPRINT("*hService = %x\n", *hService);

  DPRINT("ScmrOpenServiceW() done\n");

  return ERROR_SUCCESS;
}



void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
  return GlobalAlloc(GPTR, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
  GlobalFree(ptr);
}

/* EOF */
