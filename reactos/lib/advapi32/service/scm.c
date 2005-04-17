/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/scm.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 *                  Eric Kohl
 * UPDATE HISTORY:
 *  19990413 EA created
 *  19990515 EA
 */

/* INCLUDES ******************************************************************/

#include "advapi32.h"
#include "svcctl_c.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

handle_t BindingHandle = NULL;

static VOID
HandleBind(VOID)
{
  LPWSTR pszStringBinding;
  RPC_STATUS status;

  if (BindingHandle != NULL)
    return;

  status = RpcStringBindingComposeW(NULL,
                                    L"ncacn_np",
                                    NULL,
                                    L"\\pipe\\ntsvcs",
                                    NULL,
                                    &pszStringBinding);
  if (status)
  {
    DPRINT1("RpcStringBindingCompose returned 0x%x\n", status);
    return;
  }

  /* Set the binding handle that will be used to bind to the server. */
  status = RpcBindingFromStringBindingW(pszStringBinding,
                                        &BindingHandle);
  if (status)
  {
    DPRINT1("RpcBindingFromStringBinding returned 0x%x\n", status);
  }

  status = RpcStringFreeW(&pszStringBinding);
  if (status)
  {
    DPRINT1("RpcStringFree returned 0x%x\n", status);
  }
}


#if 0
static VOID
HandleUnbind(VOID)
{
  RPC_STATUS status;

  if (BindingHandle == NULL)
    return;

  status = RpcBindingFree(&BindingHandle);
  if (status)
  {
    DPRINT1("RpcBindingFree returned 0x%x\n", status);
  }
}
#endif


/**********************************************************************
 *  ChangeServiceConfigA
 *
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfigA(
    SC_HANDLE   hService,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCSTR      lpBinaryPathName,
    LPCSTR      lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCSTR      lpDependencies,
    LPCSTR      lpServiceStartName,
    LPCSTR      lpPassword,
    LPCSTR      lpDisplayName)
{
    DPRINT1("ChangeServiceConfigA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  ChangeServiceConfigW
 *
 * @unimplemented
 */
BOOL
STDCALL
ChangeServiceConfigW(
    SC_HANDLE   hService,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword,
    LPCWSTR     lpDisplayName)
{
    DPRINT1("ChangeServiceConfigW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  CloseServiceHandle
 *
 * @implemented
 */
BOOL STDCALL
CloseServiceHandle(SC_HANDLE hSCObject)
{
  DWORD dwError;

  DPRINT1("CloseServiceHandle() called\n");

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrCloseServiceHandle(BindingHandle,
                                   (unsigned int)hSCObject);
  DPRINT1("dwError %lu\n", dwError);

  if (dwError)
  {
    SetLastError(dwError);
    return FALSE;
  }

  return TRUE;
}


/**********************************************************************
 *  ControlService
 *
 * @unimplemented
 */
BOOL STDCALL
ControlService(SC_HANDLE        hService,
               DWORD            dwControl,
               LPSERVICE_STATUS lpServiceStatus)
{
#if 0
  DWORD dwError;

  DPRINT1("ControlService(%x, %x, %p)\n",
          hService, dwControl, lpServiceStatus);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrControlService(BindingHandle,
                               (unsigned int)hService,
                               dwControl,
                               lpServiceStatus);
  if (dwError != ERROR_SUCCESS)
  {
    SetLastError(dwError);
    return FALSE;
  }

  DPRINT1("ControlService() done\n");

  return TRUE;
#endif

  DPRINT1("ControlService is unimplemented\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/**********************************************************************
 *  CreateServiceA
 *
 * @unimplemented
 */
SC_HANDLE
STDCALL
CreateServiceA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpServiceName,
    LPCSTR      lpDisplayName,
    DWORD       dwDesiredAccess,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCSTR      lpBinaryPathName,
    LPCSTR      lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCSTR      lpDependencies,
    LPCSTR      lpServiceStartName,
    LPCSTR      lpPassword)
{
    DPRINT1("CreateServiceA is unimplemented, but returning INVALID_HANDLE_VALUE instead of NULL\n");
    return INVALID_HANDLE_VALUE;
}


/**********************************************************************
 *  CreateServiceW
 *
 * @unimplemented
 */
SC_HANDLE
STDCALL
CreateServiceW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    LPCWSTR     lpDisplayName,
    DWORD       dwDesiredAccess,
    DWORD       dwServiceType,
    DWORD       dwStartType,
    DWORD       dwErrorControl,
    LPCWSTR     lpBinaryPathName,
    LPCWSTR     lpLoadOrderGroup,
    LPDWORD     lpdwTagId,
    LPCWSTR     lpDependencies,
    LPCWSTR     lpServiceStartName,
    LPCWSTR     lpPassword)
{
    DPRINT1("CreateServiceW is unimplemented, but returning INVALID_HANDLE_VALUE instead of NULL\n");
    return INVALID_HANDLE_VALUE;
}


/**********************************************************************
 *  DeleteService
 *
 * @implemented
 */
BOOL STDCALL
DeleteService(SC_HANDLE hService)
{
  DWORD dwError;

  DPRINT1("DeleteService(%x)\n", hService);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrDeleteService(BindingHandle,
                              (unsigned int)hService);
  if (dwError != ERROR_SUCCESS)
  {
    SetLastError(dwError);
    return FALSE;
  }

  return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumDependentServicesA(
    SC_HANDLE       hService,
    DWORD           dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded,
    LPDWORD         lpServicesReturned)
{
    DPRINT1("EnumDependentServicesA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumDependentServicesW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumDependentServicesW(
    SC_HANDLE       hService,
    DWORD           dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded,
    LPDWORD         lpServicesReturned)
{
    DPRINT1("EnumDependentServicesW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServiceGroupW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServiceGroupW (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8)
{
    DPRINT1("EnumServiceGroupW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusA (
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle)
{
    DPRINT1("EnumServicesStatusA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusExA
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusExA(SC_HANDLE  hSCManager,
  SC_ENUM_TYPE  InfoLevel,
  DWORD  dwServiceType,
  DWORD  dwServiceState,
  LPBYTE  lpServices,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded,
  LPDWORD  lpServicesReturned,
  LPDWORD  lpResumeHandle,
  LPCSTR  pszGroupName)
{
    DPRINT1("EnumServicesStatusExA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusExW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusExW(SC_HANDLE  hSCManager,
  SC_ENUM_TYPE  InfoLevel,
  DWORD  dwServiceType,
  DWORD  dwServiceState,
  LPBYTE  lpServices,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded,
  LPDWORD  lpServicesReturned,
  LPDWORD  lpResumeHandle,
  LPCWSTR  pszGroupName)
{
    DPRINT1("EnumServicesStatusExW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServicesStatusW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle)
{
    DPRINT1("EnumServicesStatusW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceDisplayNameA
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceDisplayNameA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpServiceName,
    LPSTR       lpDisplayName,
    LPDWORD     lpcchBuffer)
{
    DPRINT1("GetServiceDisplayNameA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceDisplayNameW
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceDisplayNameW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpServiceName,
    LPWSTR      lpDisplayName,
    LPDWORD     lpcchBuffer)
{
    DPRINT1("GetServiceDisplayNameW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceKeyNameA
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceKeyNameA(
    SC_HANDLE   hSCManager,
    LPCSTR      lpDisplayName,
    LPSTR       lpServiceName,
    LPDWORD     lpcchBuffer)
{
    DPRINT1("GetServiceKeyNameA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  GetServiceKeyNameW
 *
 * @unimplemented
 */
BOOL
STDCALL
GetServiceKeyNameW(
    SC_HANDLE   hSCManager,
    LPCWSTR     lpDisplayName,
    LPWSTR      lpServiceName,
    LPDWORD     lpcchBuffer)
{
    DPRINT1("GetServiceKeyNameW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  LockServiceDatabase
 *
 * @implemented
 */
SC_LOCK STDCALL
LockServiceDatabase(SC_HANDLE hSCManager)
{
  SC_LOCK hLock;
  DWORD dwError;

  DPRINT("LockServiceDatabase(%x)\n", hSCManager);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrLockServiceDatabase(BindingHandle,
                                    (unsigned int)hSCManager,
                                    (unsigned int *)&hLock);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT("ScmrLockServiceDatabase() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return NULL;
  }

  DPRINT("hLock = %p\n", hLock);

  return hLock;
}


static VOID
WaitForSCManager(VOID)
{
  HANDLE hEvent;

  DPRINT("WaitForSCManager() called\n");

  /* Try to open the existing event */
  hEvent = OpenEventW(SYNCHRONIZE,
                      FALSE,
                      L"SvcctrlStartEvent_A3725DX");
  if (hEvent == NULL)
  {
    if (GetLastError() != ERROR_FILE_NOT_FOUND)
      return;

    /* Try to create a new event */
    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"SvcctrlStartEvent_A3725DX");
    if (hEvent == NULL)
    {
      /* Try to open the existing event again */
      hEvent = OpenEventW(SYNCHRONIZE,
                          FALSE,
                          L"SvcctrlStartEvent_A3725DX");
      if (hEvent == NULL)
        return;
    }
  }

  /* Wait for 3 minutes */
  WaitForSingleObject(hEvent, 180000);
  CloseHandle(hEvent);

  DPRINT("ScmWaitForSCManager() done\n");
}


/**********************************************************************
 *  OpenSCManagerA
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenSCManagerA(LPCSTR lpMachineName,
               LPCSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
  SC_HANDLE hScm = NULL;
  DWORD dwError;

  DPRINT("OpenSCManagerA(%s, %s, %lx)\n",
         lpMachineName, lpDatabaseName, dwDesiredAccess);

  WaitForSCManager();

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrOpenSCManagerA(BindingHandle,
                               (LPSTR)lpMachineName,
                               (LPSTR)lpDatabaseName,
                               dwDesiredAccess,
                               (unsigned int*)&hScm);
  if (dwError)
  {
    DPRINT("ScmrOpenSCManagerA() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return NULL;
  }

  DPRINT("hScm = %p\n", hScm);

  return hScm;
}


/**********************************************************************
 *  OpenSCManagerW
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenSCManagerW(LPCWSTR lpMachineName,
               LPCWSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
  SC_HANDLE hScm = NULL;
  DWORD dwError;

  DPRINT("OpenSCManagerW(%S, %S, %lx)\n",
         lpMachineName, lpDatabaseName, dwDesiredAccess);

  WaitForSCManager();

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrOpenSCManagerW(BindingHandle,
                               (LPWSTR)lpMachineName,
                               (LPWSTR)lpDatabaseName,
                               dwDesiredAccess,
                               (unsigned int*)&hScm);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT1("ScmrOpenSCManagerW() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return NULL;
  }

  DPRINT("hScm = %p\n", hScm);

  return hScm;
}


/**********************************************************************
 *  OpenServiceA
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenServiceA(SC_HANDLE hSCManager,
             LPCSTR lpServiceName,
             DWORD dwDesiredAccess)
{
  SC_HANDLE hService = NULL;
  DWORD dwError;

  DPRINT("OpenServiceA(%p, %s, %lx)\n",
         hSCManager, lpServiceName, dwDesiredAccess);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrOpenServiceA(BindingHandle,
                             (unsigned int)hSCManager,
                             (LPSTR)lpServiceName,
                             dwDesiredAccess,
                             (unsigned int*)&hService);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT1("ScmrOpenServiceA() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return NULL;
  }

  DPRINT("hService = %p\n", hService);

  return hService;
}


/**********************************************************************
 *  OpenServiceW
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenServiceW(SC_HANDLE hSCManager,
             LPCWSTR lpServiceName,
             DWORD dwDesiredAccess)
{
  SC_HANDLE hService = NULL;
  DWORD dwError;

  DPRINT("OpenServiceW(%p, %S, %lx)\n",
         hSCManager, lpServiceName, dwDesiredAccess);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrOpenServiceW(BindingHandle,
                             (unsigned int)hSCManager,
                             (LPWSTR)lpServiceName,
                             dwDesiredAccess,
                             (unsigned int*)&hService);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT1("ScmrOpenServiceW() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return NULL;
  }

  DPRINT("hService = %p\n", hService);

  return hService;
}


/**********************************************************************
 *  QueryServiceConfigA
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfigA(
    SC_HANDLE       hService,
    LPQUERY_SERVICE_CONFIGA lpServiceConfig,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    DPRINT1("QueryServiceConfigA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceConfigW
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfigW(
    SC_HANDLE       hService,
    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded)
{
    DPRINT1("QueryServiceConfigW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceLockStatusA
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceLockStatusA(
    SC_HANDLE           hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSA    lpLockStatus,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded)
{
    DPRINT1("QueryServiceLockStatusA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceLockStatusW
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceLockStatusW(
    SC_HANDLE           hSCManager,
    LPQUERY_SERVICE_LOCK_STATUSW    lpLockStatus,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded)
{
    DPRINT1("QueryServiceLockStatusW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceObjectSecurity
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceObjectSecurity(
    SC_HANDLE       hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    DPRINT1("QueryServiceObjectSecurity is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceStatus
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceStatus(
    SC_HANDLE       hService,
    LPSERVICE_STATUS    lpServiceStatus)
{
    DPRINT1("QueryServiceStatus is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  QueryServiceStatusEx
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceStatusEx(SC_HANDLE  hService,
  SC_STATUS_TYPE  InfoLevel,
  LPBYTE  lpBuffer,
  DWORD  cbBufSize,
  LPDWORD  pcbBytesNeeded)
{
    DPRINT1("QueryServiceStatusEx is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  StartServiceA
 *
 * @unimplemented
 */
BOOL
STDCALL
StartServiceA(
    SC_HANDLE   hService,
    DWORD       dwNumServiceArgs,
    LPCSTR      *lpServiceArgVectors)
{
    DPRINT1("StartServiceA is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}




/**********************************************************************
 *  StartServiceW
 *
 * @unimplemented
 */
BOOL
STDCALL
StartServiceW(
    SC_HANDLE   hService,
    DWORD       dwNumServiceArgs,
    LPCWSTR     *lpServiceArgVectors)
{
    DPRINT1("StartServiceW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  UnlockServiceDatabase
 *
 * @implemented
 */
BOOL STDCALL
UnlockServiceDatabase(SC_LOCK ScLock)
{
  DWORD dwError;

  DPRINT("UnlockServiceDatabase(%x)\n", hSCManager);

  HandleBind();

  /* Call to services.exe using RPC */
  dwError = ScmrUnlockServiceDatabase(BindingHandle,
                                      (unsigned int)ScLock);
  if (dwError != ERROR_SUCCESS)
  {
    DPRINT("ScmrUnlockServiceDatabase() failed (Error %lu)\n", dwError);
    SetLastError(dwError);
    return FALSE;
  }

  return TRUE;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
  return GlobalAlloc(GPTR,len);
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
  GlobalFree(ptr);
}

/* EOF */
