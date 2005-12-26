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

#include <advapi32.h>
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
 * @implemented
 */
BOOL STDCALL
ChangeServiceConfigA(SC_HANDLE hService,
                     DWORD dwServiceType,
                     DWORD dwStartType,
                     DWORD dwErrorControl,
                     LPCSTR lpBinaryPathName,
                     LPCSTR lpLoadOrderGroup,
                     LPDWORD lpdwTagId,
                     LPCSTR lpDependencies,
                     LPCSTR lpServiceStartName,
                     LPCSTR lpPassword,
                     LPCSTR lpDisplayName)
{
    DWORD dwError;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPSTR lpStr;

    DPRINT("ChangeServiceConfigA() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = strlen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrChangeServiceConfigA(BindingHandle,
                                       (unsigned int)hService,
                                       dwServiceType,
                                       dwStartType,
                                       dwErrorControl,
                                       (LPSTR)lpBinaryPathName,
                                       (LPSTR)lpLoadOrderGroup,
                                       lpdwTagId,
                                       (LPSTR)lpDependencies,
                                       dwDependenciesLength,
                                       (LPSTR)lpServiceStartName,
                                       NULL,              /* FIXME: lpPassword */
                                       0,                 /* FIXME: dwPasswordLength */
                                       (LPSTR)lpDisplayName);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrChangeServiceConfigA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfigW
 *
 * @implemented
 */
BOOL STDCALL
ChangeServiceConfigW(SC_HANDLE hService,
                     DWORD dwServiceType,
                     DWORD dwStartType,
                     DWORD dwErrorControl,
                     LPCWSTR lpBinaryPathName,
                     LPCWSTR lpLoadOrderGroup,
                     LPDWORD lpdwTagId,
                     LPCWSTR lpDependencies,
                     LPCWSTR lpServiceStartName,
                     LPCWSTR lpPassword,
                     LPCWSTR lpDisplayName)
{
    DWORD dwError;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPWSTR lpStr;

    DPRINT("ChangeServiceConfigW() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPWSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = wcslen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrChangeServiceConfigW(BindingHandle,
                                       (unsigned int)hService,
                                       dwServiceType,
                                       dwStartType,
                                       dwErrorControl,
                                       (LPWSTR)lpBinaryPathName,
                                       (LPWSTR)lpLoadOrderGroup,
                                       lpdwTagId,
                                       (LPWSTR)lpDependencies,
                                       dwDependenciesLength,
                                       (LPWSTR)lpServiceStartName,
                                       NULL,              /* FIXME: lpPassword */
                                       0,                 /* FIXME: dwPasswordLength */
                                       (LPWSTR)lpDisplayName);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrChangeServiceConfigW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
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

    DPRINT("CloseServiceHandle() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrCloseServiceHandle(BindingHandle,
                                     (unsigned int)hSCObject);
    if (dwError)
    {
        DPRINT1("ScmrCloseServiceHandle() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("CloseServiceHandle() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlService
 *
 * @implemented
 */
BOOL STDCALL
ControlService(SC_HANDLE hService,
               DWORD dwControl,
               LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    DPRINT("ControlService(%x, %x, %p)\n",
           hService, dwControl, lpServiceStatus);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrControlService(BindingHandle,
                                 (unsigned int)hService,
                                 dwControl,
                                 lpServiceStatus);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrControlService() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("ControlService() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlServiceEx
 *
 * @unimplemented
 */
BOOL STDCALL
ControlServiceEx(IN SC_HANDLE hService,
                 IN DWORD dwControl,
                 IN DWORD dwInfoLevel,
                 IN OUT PVOID pControlParams)
{
    DPRINT1("ControlServiceEx(0x%p, 0x%x, 0x%x, 0x%p) UNIMPLEMENTED!\n",
            hService, dwControl, dwInfoLevel, pControlParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}



/**********************************************************************
 *  CreateServiceA
 *
 * @implemented
 */
SC_HANDLE
STDCALL
CreateServiceA(SC_HANDLE hSCManager,
               LPCSTR lpServiceName,
               LPCSTR lpDisplayName,
               DWORD dwDesiredAccess,
               DWORD dwServiceType,
               DWORD dwStartType,
               DWORD dwErrorControl,
               LPCSTR lpBinaryPathName,
               LPCSTR lpLoadOrderGroup,
               LPDWORD lpdwTagId,
               LPCSTR lpDependencies,
               LPCSTR lpServiceStartName,
               LPCSTR lpPassword)
{
    SC_HANDLE RetVal = NULL;
    LPWSTR lpServiceNameW = NULL;
    LPWSTR lpDisplayNameW = NULL;
    LPWSTR lpBinaryPathNameW = NULL;
    LPWSTR lpLoadOrderGroupW = NULL;
    LPWSTR lpDependenciesW = NULL;
    LPWSTR lpServiceStartNameW = NULL;
    LPWSTR lpPasswordW = NULL;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPSTR lpStr;

    int len = MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, NULL, 0);
    lpServiceNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpServiceNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, lpServiceNameW, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, NULL, 0);
    lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpDisplayNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, lpDisplayNameW, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpBinaryPathName, -1, NULL, 0);
    lpBinaryPathNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpBinaryPathNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, lpBinaryPathNameW, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, NULL, 0);
    lpLoadOrderGroupW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpLoadOrderGroupW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, lpLoadOrderGroupW, len);

    if (lpDependencies != NULL)
    {
        lpStr = (LPSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = strlen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    lpDependenciesW = HeapAlloc(GetProcessHeap(), 0, dwDependenciesLength * sizeof(WCHAR));
    if (!lpDependenciesW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpDependencies, -1, lpDependenciesW, dwDependenciesLength);

    len = MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, NULL, 0);
    lpServiceStartName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpServiceStartNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, lpServiceStartNameW, len);

    len = MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, NULL, 0);
    lpPasswordW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!lpPasswordW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, lpPasswordW, len);

    RetVal = CreateServiceW(hSCManager,
                            lpServiceNameW,
                            lpDisplayNameW,
                            dwDesiredAccess,
                            dwServiceType,
                            dwStartType,
                            dwErrorControl,
                            lpBinaryPathNameW,
                            lpLoadOrderGroupW,
                            lpdwTagId,
                            lpDependenciesW,
                            lpServiceStartNameW,
                            lpPasswordW);

cleanup:
    HeapFree(GetProcessHeap(), 0, lpServiceNameW);
    HeapFree(GetProcessHeap(), 0, lpDisplayNameW);
    HeapFree(GetProcessHeap(), 0, lpBinaryPathNameW);
    HeapFree(GetProcessHeap(), 0, lpLoadOrderGroupW);
    HeapFree(GetProcessHeap(), 0, lpDependenciesW);
    HeapFree(GetProcessHeap(), 0, lpServiceStartNameW);
    HeapFree(GetProcessHeap(), 0, lpPasswordW);

    return RetVal;
}


/**********************************************************************
 *  CreateServiceW
 *
 * @implemented
 */
SC_HANDLE STDCALL
CreateServiceW(SC_HANDLE hSCManager,
               LPCWSTR lpServiceName,
               LPCWSTR lpDisplayName,
               DWORD dwDesiredAccess,
               DWORD dwServiceType,
               DWORD dwStartType,
               DWORD dwErrorControl,
               LPCWSTR lpBinaryPathName,
               LPCWSTR lpLoadOrderGroup,
               LPDWORD lpdwTagId,
               LPCWSTR lpDependencies,
               LPCWSTR lpServiceStartName,
               LPCWSTR lpPassword)
{
    SC_HANDLE hService = NULL;
    DWORD dwError;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPWSTR lpStr;

    DPRINT("CreateServiceW() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPWSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = wcslen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrCreateServiceW(BindingHandle,
                                 (unsigned int)hSCManager,
                                 (LPWSTR)lpServiceName,
                                 (LPWSTR)lpDisplayName,
                                 dwDesiredAccess,
                                 dwServiceType,
                                 dwStartType,
                                 dwErrorControl,
                                 (LPWSTR)lpBinaryPathName,
                                 (LPWSTR)lpLoadOrderGroup,
                                 lpdwTagId,
                                 (LPWSTR)lpDependencies,
                                 dwDependenciesLength,
                                 (LPWSTR)lpServiceStartName,
                                 NULL,              /* FIXME: lpPassword */
                                 0,                 /* FIXME: dwPasswordLength */
                                 (unsigned int *)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrCreateServiceW(%S) failed (Error %lu)\n", lpServiceName, dwError);
        SetLastError(dwError);
        return NULL;
    }

    return hService;
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

    DPRINT("DeleteService(%x)\n", hService);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrDeleteService(BindingHandle,
                                (unsigned int)hService);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrDeleteService() failed (Error %lu)\n", dwError);
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
EnumDependentServicesW(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSW lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
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
EnumServiceGroupW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSA  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle,
    LPCWSTR                 lpGroup)
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
EnumServicesStatusA(
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
 *  EnumServicesStatusW
 *
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusW(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSW lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT("EnumServicesStatusW() called\n");

    HandleBind();

    dwError = ScmrEnumServicesStatusW(BindingHandle,
                                      (unsigned int)hSCManager,
                                      dwServiceType,
                                      dwServiceState,
                                      (unsigned char *)lpServices,
                                      cbBufSize,
                                      pcbBytesNeeded,
                                      lpServicesReturned,
                                      lpResumeHandle);

    lpStatusPtr = (LPENUM_SERVICE_STATUSW)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrEnumServicesStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("ScmrEnumServicesStatusW() done\n");

    return TRUE;
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
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusExW(SC_HANDLE hSCManager,
                      SC_ENUM_TYPE InfoLevel,
                      DWORD dwServiceType,
                      DWORD dwServiceState,
                      LPBYTE lpServices,
                      DWORD cbBufSize,
                      LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned,
                      LPDWORD lpResumeHandle,
                      LPCWSTR pszGroupName)
{
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT1("EnumServicesStatusExW() called\n");

    HandleBind();

    dwError = ScmrEnumServicesStatusExW(BindingHandle,
                                        (unsigned int)hSCManager,
                                        (unsigned long)InfoLevel,
                                        dwServiceType,
                                        dwServiceState,
                                        (unsigned char *)lpServices,
                                        cbBufSize,
                                        pcbBytesNeeded,
                                        lpServicesReturned,
                                        lpResumeHandle,
                                        (wchar_t *)pszGroupName);

    lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSW)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrEnumServicesStatusExW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT1("ScmrEnumServicesStatusExW() done\n");

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameA
 *
 * @implemented
 */
BOOL STDCALL
GetServiceDisplayNameA(SC_HANDLE hSCManager,
                       LPCSTR lpServiceName,
                       LPSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;

    DPRINT("GetServiceDisplayNameA() called\n");

    HandleBind();

    dwError = ScmrGetServiceDisplayNameA(BindingHandle,
                                         (unsigned int)hSCManager,
                                         (LPSTR)lpServiceName,
                                         lpDisplayName,
                                         lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrGetServiceDisplayNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameW
 *
 * @implemented
 */
BOOL STDCALL
GetServiceDisplayNameW(SC_HANDLE hSCManager,
                       LPCWSTR lpServiceName,
                       LPWSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;

    DPRINT("GetServiceDisplayNameW() called\n");

    HandleBind();

    dwError = ScmrGetServiceDisplayNameW(BindingHandle,
                                         (unsigned int)hSCManager,
                                         (LPWSTR)lpServiceName,
                                         lpDisplayName,
                                         lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrGetServiceDisplayNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameA
 *
 * @implemented
 */
BOOL STDCALL
GetServiceKeyNameA(SC_HANDLE hSCManager,
                   LPCSTR lpDisplayName,
                   LPSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;

    DPRINT("GetServiceKeyNameA() called\n");

    HandleBind();

    dwError = ScmrGetServiceKeyNameA(BindingHandle,
                                     (unsigned int)hSCManager,
                                     (LPSTR)lpDisplayName,
                                     lpServiceName,
                                     lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrGetServiceKeyNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameW
 *
 * @implemented
 */
BOOL STDCALL
GetServiceKeyNameW(SC_HANDLE hSCManager,
                   LPCWSTR lpDisplayName,
                   LPWSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;

    DPRINT("GetServiceKeyNameW() called\n");

    HandleBind();

    dwError = ScmrGetServiceKeyNameW(BindingHandle,
                                     (unsigned int)hSCManager,
                                     (LPWSTR)lpDisplayName,
                                     lpServiceName,
                                     lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrGetServiceKeyNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
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
        DPRINT1("ScmrLockServiceDatabase() failed (Error %lu)\n", dwError);
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
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrOpenSCManagerA() failed (Error %lu)\n", dwError);
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
        DPRINT("ScmrOpenServiceA(%s) failed (Error %lu)\n", lpServiceName, dwError);
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
        DPRINT("ScmrOpenServiceW(%S) failed (Error %lu)\n", lpServiceName, dwError);
        SetLastError(dwError);
        return NULL;
    }

    DPRINT("hService = %p\n", hService);

    return hService;
}


/**********************************************************************
 *  QueryServiceConfigA
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfigA(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGA lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceConfigA(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceConfigA(BindingHandle,
                                      (unsigned int)hService,
                                      (unsigned char *)lpServiceConfig,
                                      cbBufSize,
                                      pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceConfigA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpServiceConfig->lpBinaryPathName)
        lpServiceConfig->lpBinaryPathName =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpBinaryPathName);

    if (lpServiceConfig->lpLoadOrderGroup)
        lpServiceConfig->lpLoadOrderGroup =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpLoadOrderGroup);

    if (lpServiceConfig->lpDependencies)
        lpServiceConfig->lpDependencies =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpDependencies);

    if (lpServiceConfig->lpServiceStartName)
        lpServiceConfig->lpServiceStartName =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpServiceStartName);

    if (lpServiceConfig->lpDisplayName)
        lpServiceConfig->lpDisplayName =
           (LPSTR)((ULONG_PTR)lpServiceConfig +
                   (ULONG_PTR)lpServiceConfig->lpDisplayName);

    DPRINT("QueryServiceConfigA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfigW
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfigW(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
#if 0
    DWORD dwError;

    DPRINT("QueryServiceConfigW(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    CHECKPOINT1;
    dwError = ScmrQueryServiceConfigW(BindingHandle,
                                      (unsigned int)hService,
                                      (unsigned char *)lpServiceConfig,
                                      cbBufSize,
                                      pcbBytesNeeded);
    CHECKPOINT1;
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceConfigW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpServiceConfig->lpBinaryPathName)
        lpServiceConfig->lpBinaryPathName =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpBinaryPathName);

    if (lpServiceConfig->lpLoadOrderGroup)
        lpServiceConfig->lpLoadOrderGroup =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpLoadOrderGroup);

    if (lpServiceConfig->lpDependencies)
        lpServiceConfig->lpDependencies =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpDependencies);

    if (lpServiceConfig->lpServiceStartName)
        lpServiceConfig->lpServiceStartName =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpServiceStartName);

    if (lpServiceConfig->lpDisplayName)
        lpServiceConfig->lpDisplayName =
           (LPWSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpDisplayName);

    DPRINT("QueryServiceConfigW() done\n");

    return TRUE;
#else
    DPRINT1("QueryServiceConfigW is unimplemented\n");
    if (lpServiceConfig && cbBufSize >= sizeof(QUERY_SERVICE_CONFIGW))
    {
        memset(lpServiceConfig, 0, *pcbBytesNeeded);
        return TRUE;
    }
    else
    {
        *pcbBytesNeeded = sizeof(QUERY_SERVICE_CONFIGW);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
#endif
}


/**********************************************************************
 *  QueryServiceConfig2A
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfig2A(
    SC_HANDLE       hService,
    DWORD           dwInfo,
    LPBYTE          lpBuffer,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    DPRINT1("QueryServiceConfig2A is unimplemented\n");
    return FALSE;
}


/**********************************************************************
 *  QueryServiceConfig2W
 *
 * @unimplemented
 */
BOOL
STDCALL
QueryServiceConfig2W(
    SC_HANDLE       hService,
    DWORD           dwInfo,
    LPBYTE          lpBuffer,
    DWORD           cbBufSize,
    LPDWORD         pcbBytesNeeded)
{
    DPRINT1("QueryServiceConfig2W is unimplemented\n");
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
 * @implemented
 */
BOOL STDCALL
QueryServiceObjectSecurity(SC_HANDLE hService,
                           SECURITY_INFORMATION dwSecurityInformation,
                           PSECURITY_DESCRIPTOR lpSecurityDescriptor,
                           DWORD cbBufSize,
                           LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceObjectSecurity(%p, %lu, %p)\n",
           hService, dwSecurityInformation, lpSecurityDescriptor);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceObjectSecurity(BindingHandle,
                                             (unsigned int)hService,
                                             dwSecurityInformation,
                                             (unsigned char *)lpSecurityDescriptor,
                                             cbBufSize,
                                             pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("QueryServiceObjectSecurity() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  QueryServiceStatus
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceStatus(SC_HANDLE hService,
                   LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    DPRINT("QueryServiceStatus(%p, %p)\n",
           hService, lpServiceStatus);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceStatus(BindingHandle,
                                     (unsigned int)hService,
                                     lpServiceStatus);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrQueryServiceStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  QueryServiceStatusEx
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceStatusEx(SC_HANDLE hService,
                     SC_STATUS_TYPE InfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceStatusEx() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceStatusEx(BindingHandle,
                                       (unsigned int)hService,
                                       InfoLevel,
                                       lpBuffer,
                                       cbBufSize,
                                       pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceStatusEx() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  SetServiceObjectSecurity
 *
 * @implemented
 */
BOOL STDCALL
SetServiceObjectSecurity(SC_HANDLE hService,
                         SECURITY_INFORMATION dwSecurityInformation,
                         PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR SelfRelativeSD = NULL;
    ULONG Length;
    NTSTATUS Status;
    DWORD dwError;

    Length = 0;
    Status = RtlMakeSelfRelativeSD(lpSecurityDescriptor,
                                   SelfRelativeSD,
                                   &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SelfRelativeSD = HeapAlloc(GetProcessHeap(), 0, Length);
    if (SelfRelativeSD == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Status = RtlMakeSelfRelativeSD(lpSecurityDescriptor,
                                   SelfRelativeSD,
                                   &Length);
    if (!NT_SUCCESS(Status))
    {
        HeapFree(GetProcessHeap(), 0, SelfRelativeSD);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrSetServiceObjectSecurity(BindingHandle,
                                           (unsigned int)hService,
                                           dwSecurityInformation,
                                           (unsigned char *)SelfRelativeSD,
                                           Length);

    HeapFree(GetProcessHeap(), 0, SelfRelativeSD);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrServiceObjectSecurity() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
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
#if 0
    DWORD dwError;

    DPRINT("StartServiceW()\n", ScLock);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrStartServiceW(BindingHandle,
                                dwNumServiceArgs,
                                lpServiceArgVectors);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrStartServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
#endif
    DPRINT1("StartServiceW is unimplemented, but returns success...\n");
    //SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    //return FALSE;
    return TRUE;
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

    DPRINT("UnlockServiceDatabase(%x)\n", ScLock);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrUnlockServiceDatabase(BindingHandle,
                                        (unsigned int)ScLock);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrUnlockServiceDatabase() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  NotifyBootConfigStatus
 *
 * @implemented
 */
BOOL STDCALL
NotifyBootConfigStatus(BOOL BootAcceptable)
{
    DWORD dwError;

    DPRINT1("NotifyBootConfigStatus()\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrNotifyBootConfigStatus(BindingHandle,
                                         BootAcceptable);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("NotifyBootConfigStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
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
