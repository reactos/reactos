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
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
SVCCTL_HANDLEA_bind(SVCCTL_HANDLEA szMachineName)
{
    handle_t hBinding = NULL;
    UCHAR *pszStringBinding;
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEA_bind() called\n");

    status = RpcStringBindingComposeA(NULL,
                                      (UCHAR *)"ncacn_np",
                                      (UCHAR *)szMachineName,
                                      (UCHAR *)"\\pipe\\ntsvcs",
                                      NULL,
                                      (UCHAR **)&pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingA(pszStringBinding,
                                          &hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeA(&pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
SVCCTL_HANDLEA_unbind(SVCCTL_HANDLEA szMachineName,
                      handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEA_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
SVCCTL_HANDLEW_bind(SVCCTL_HANDLEW szMachineName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEW_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      szMachineName,
                                      L"\\pipe\\ntsvcs",
                                      NULL,
                                      &pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
SVCCTL_HANDLEW_unbind(SVCCTL_HANDLEW szMachineName,
                      handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEW_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


DWORD
ScmRpcStatusToWinError(RPC_STATUS Status)
{
    switch (Status)
    {
        case RPC_S_INVALID_BINDING:
        case RPC_X_SS_IN_NULL_CONTEXT:
            return ERROR_INVALID_HANDLE;

        case RPC_X_ENUM_VALUE_OUT_OF_RANGE:
        case RPC_X_BYTE_COUNT_TOO_SMALL:
            return ERROR_INVALID_PARAMETER;

        case RPC_X_NULL_REF_POINTER:
            return ERROR_INVALID_ADDRESS;

        default:
            return (DWORD)Status;
    }
}


/**********************************************************************
 *  ChangeServiceConfig2A
 *
 * @implemented
 */
BOOL WINAPI
ChangeServiceConfig2A(SC_HANDLE hService,
                      DWORD dwInfoLevel,
                      LPVOID lpInfo)
{
    SC_RPC_CONFIG_INFOA Info;
    DWORD dwError;

    TRACE("ChangeServiceConfig2A() called\n");

    if (lpInfo == NULL) return TRUE;

    /* Fill relevant field of the Info structure */
    Info.dwInfoLevel = dwInfoLevel;
    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            Info.psd = lpInfo;
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            Info.psfa = lpInfo;
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    RpcTryExcept
    {
        dwError = RChangeServiceConfig2A((SC_RPC_HANDLE)hService,
                                         Info);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RChangeServiceConfig2A() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfig2W
 *
 * @implemented
 */
BOOL WINAPI
ChangeServiceConfig2W(SC_HANDLE hService,
                      DWORD dwInfoLevel,
                      LPVOID lpInfo)
{
    SC_RPC_CONFIG_INFOW Info;
    DWORD dwError;

    TRACE("ChangeServiceConfig2W() called\n");

    if (lpInfo == NULL) return TRUE;

    /* Fill relevant field of the Info structure */
    Info.dwInfoLevel = dwInfoLevel;
    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            Info.psd = lpInfo;
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            Info.psfa = lpInfo;
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    RpcTryExcept
    {
        dwError = RChangeServiceConfig2W((SC_RPC_HANDLE)hService,
                                         Info);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RChangeServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfigA
 *
 * @implemented
 */
BOOL WINAPI
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
    SIZE_T cchLength;
    LPCSTR lpStr;
    DWORD dwPasswordLength = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("ChangeServiceConfigA() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = lpDependencies;
        while (*lpStr)
        {
            cchLength = strlen(lpStr) + 1;
            dwDependenciesLength += (DWORD)cchLength;
            lpStr = lpStr + cchLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */
    lpEncryptedPassword = (LPBYTE)lpPassword;
    dwPasswordLength = (DWORD)(lpPassword ? (strlen(lpPassword) + 1) * sizeof(CHAR) : 0);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RChangeServiceConfigA((SC_RPC_HANDLE)hService,
                                        dwServiceType,
                                        dwStartType,
                                        dwErrorControl,
                                        (LPSTR)lpBinaryPathName,
                                        (LPSTR)lpLoadOrderGroup,
                                        lpdwTagId,
                                        (LPBYTE)lpDependencies,
                                        dwDependenciesLength,
                                        (LPSTR)lpServiceStartName,
                                        lpEncryptedPassword,
                                        dwPasswordLength,
                                        (LPSTR)lpDisplayName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RChangeServiceConfigA() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
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
    SIZE_T cchLength;
    LPCWSTR lpStr;
    DWORD dwPasswordLength = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("ChangeServiceConfigW() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = lpDependencies;
        while (*lpStr)
        {
            cchLength = wcslen(lpStr) + 1;
            dwDependenciesLength += (DWORD)cchLength;
            lpStr = lpStr + cchLength;
        }
        dwDependenciesLength++;
        dwDependenciesLength *= sizeof(WCHAR);
    }

    /* FIXME: Encrypt the password */
    lpEncryptedPassword = (LPBYTE)lpPassword;
    dwPasswordLength = (lpPassword ? (wcslen(lpPassword) + 1) * sizeof(WCHAR) : 0);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RChangeServiceConfigW((SC_RPC_HANDLE)hService,
                                        dwServiceType,
                                        dwStartType,
                                        dwErrorControl,
                                        (LPWSTR)lpBinaryPathName,
                                        (LPWSTR)lpLoadOrderGroup,
                                        lpdwTagId,
                                        (LPBYTE)lpDependencies,
                                        dwDependenciesLength,
                                        (LPWSTR)lpServiceStartName,
                                        lpEncryptedPassword,
                                        dwPasswordLength,
                                        (LPWSTR)lpDisplayName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RChangeServiceConfigW() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
CloseServiceHandle(SC_HANDLE hSCObject)
{
    DWORD dwError;

    TRACE("CloseServiceHandle() called\n");

    if (!hSCObject)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RCloseServiceHandle((LPSC_RPC_HANDLE)&hSCObject);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError)
    {
        TRACE("RCloseServiceHandle() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("CloseServiceHandle() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlService
 *
 * @implemented
 */
BOOL WINAPI
ControlService(SC_HANDLE hService,
               DWORD dwControl,
               LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("ControlService(%x, %x, %p)\n",
           hService, dwControl, lpServiceStatus);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RControlService((SC_RPC_HANDLE)hService,
                                  dwControl,
                                  lpServiceStatus);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RControlService() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("ControlService() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlServiceEx
 *
 * @unimplemented
 */
BOOL WINAPI
ControlServiceEx(IN SC_HANDLE hService,
                 IN DWORD dwControl,
                 IN DWORD dwInfoLevel,
                 IN OUT PVOID pControlParams)
{
    FIXME("ControlServiceEx(0x%p, 0x%x, 0x%x, 0x%p) UNIMPLEMENTED!\n",
            hService, dwControl, dwInfoLevel, pControlParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  CreateServiceA
 *
 * @implemented
 */
SC_HANDLE WINAPI
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
    SC_HANDLE hService = NULL;
    DWORD dwDependenciesLength = 0;
    DWORD dwError;
    SIZE_T cchLength;
    LPCSTR lpStr;
    DWORD dwPasswordLength = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("CreateServiceA() called\n");
    TRACE("%p %s %s\n", hSCManager,
          lpServiceName, lpDisplayName);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    /* Calculate the Dependencies length */
    if (lpDependencies != NULL)
    {
        lpStr = lpDependencies;
        while (*lpStr)
        {
            cchLength = strlen(lpStr) + 1;
            dwDependenciesLength += (DWORD)cchLength;
            lpStr = lpStr + cchLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */
    lpEncryptedPassword = (LPBYTE)lpPassword;
    dwPasswordLength = (DWORD)(lpPassword ? (strlen(lpPassword) + 1) * sizeof(CHAR) : 0);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RCreateServiceA((SC_RPC_HANDLE)hSCManager,
                                  (LPSTR)lpServiceName,
                                  (LPSTR)lpDisplayName,
                                  dwDesiredAccess,
                                  dwServiceType,
                                  dwStartType,
                                  dwErrorControl,
                                  (LPSTR)lpBinaryPathName,
                                  (LPSTR)lpLoadOrderGroup,
                                  lpdwTagId,
                                  (LPBYTE)lpDependencies,
                                  dwDependenciesLength,
                                  (LPSTR)lpServiceStartName,
                                  lpEncryptedPassword,
                                  dwPasswordLength,
                                  (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RCreateServiceA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    return hService;
}


/**********************************************************************
 *  CreateServiceW
 *
 * @implemented
 */
SC_HANDLE WINAPI
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
    DWORD dwDependenciesLength = 0;
    DWORD dwError;
    SIZE_T cchLength;
    LPCWSTR lpStr;
    DWORD dwPasswordLength = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("CreateServiceW() called\n");
    TRACE("%p %S %S\n", hSCManager,
          lpServiceName, lpDisplayName);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    /* Calculate the Dependencies length */
    if (lpDependencies != NULL)
    {
        lpStr = lpDependencies;
        while (*lpStr)
        {
            cchLength = wcslen(lpStr) + 1;
            dwDependenciesLength += (DWORD)cchLength;
            lpStr = lpStr + cchLength;
        }
        dwDependenciesLength++;
        dwDependenciesLength *= sizeof(WCHAR);
    }

    /* FIXME: Encrypt the password */
    lpEncryptedPassword = (LPBYTE)lpPassword;
    dwPasswordLength = (DWORD)(lpPassword ? (wcslen(lpPassword) + 1) * sizeof(WCHAR) : 0);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RCreateServiceW((SC_RPC_HANDLE)hSCManager,
                                  lpServiceName,
                                  lpDisplayName,
                                  dwDesiredAccess,
                                  dwServiceType,
                                  dwStartType,
                                  dwErrorControl,
                                  lpBinaryPathName,
                                  lpLoadOrderGroup,
                                  lpdwTagId,
                                  (LPBYTE)lpDependencies,
                                  dwDependenciesLength,
                                  lpServiceStartName,
                                  lpEncryptedPassword,
                                  dwPasswordLength,
                                  (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RCreateServiceW() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
DeleteService(SC_HANDLE hService)
{
    DWORD dwError;

    TRACE("DeleteService(%x)\n", hService);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RDeleteService((SC_RPC_HANDLE)hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RDeleteService() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesA
 *
 * @implemented
 */
BOOL WINAPI
EnumDependentServicesA(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSA lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    ENUM_SERVICE_STATUSA ServiceStatus;
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumDependentServicesA() called\n");

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUSA))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUSA);
    }
    else
    {
        lpStatusPtr = lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumDependentServicesA((SC_RPC_HANDLE)hService,
                                          dwServiceState,
                                          (LPBYTE)lpStatusPtr,
                                          dwBufferSize,
                                          pcbBytesNeeded,
                                          lpServicesReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
        for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
        {
            if (lpStatusPtr->lpServiceName)
                lpStatusPtr->lpServiceName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

            if (lpStatusPtr->lpDisplayName)
                lpStatusPtr->lpDisplayName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

            lpStatusPtr++;
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumDependentServicesA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumDependentServicesA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesW
 *
 * @implemented
 */
BOOL WINAPI
EnumDependentServicesW(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSW lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    ENUM_SERVICE_STATUSW ServiceStatus;
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumDependentServicesW() called\n");

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUSW))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUSW);
    }
    else
    {
        lpStatusPtr = lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumDependentServicesW((SC_RPC_HANDLE)hService,
                                          dwServiceState,
                                          (LPBYTE)lpStatusPtr,
                                          dwBufferSize,
                                          pcbBytesNeeded,
                                          lpServicesReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
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
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumDependentServicesW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumDependentServicesW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServiceGroupW
 *
 * @implemented
 */
BOOL WINAPI
EnumServiceGroupW(SC_HANDLE hSCManager,
                  DWORD dwServiceType,
                  DWORD dwServiceState,
                  LPENUM_SERVICE_STATUSW lpServices,
                  DWORD cbBufSize,
                  LPDWORD pcbBytesNeeded,
                  LPDWORD lpServicesReturned,
                  LPDWORD lpResumeHandle,
                  LPCWSTR lpGroup)
{
    ENUM_SERVICE_STATUSW ServiceStatus;
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumServiceGroupW() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pcbBytesNeeded == NULL || lpServicesReturned == NULL)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUSW))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUSW);
    }
    else
    {
        lpStatusPtr = lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        if (lpGroup == NULL)
        {
            dwError = REnumServicesStatusW((SC_RPC_HANDLE)hSCManager,
                                           dwServiceType,
                                           dwServiceState,
                                           (LPBYTE)lpStatusPtr,
                                           dwBufferSize,
                                           pcbBytesNeeded,
                                           lpServicesReturned,
                                           lpResumeHandle);
        }
        else
        {
            dwError = REnumServiceGroupW((SC_RPC_HANDLE)hSCManager,
                                         dwServiceType,
                                         dwServiceState,
                                         (LPBYTE)lpStatusPtr,
                                         dwBufferSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned,
                                         lpResumeHandle,
                                         lpGroup);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
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
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumServiceGroupW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServiceGroupW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusA
 *
 * @implemented
 */
BOOL WINAPI
EnumServicesStatusA(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSA lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    ENUM_SERVICE_STATUSA ServiceStatus;
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumServicesStatusA() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pcbBytesNeeded == NULL || lpServicesReturned == NULL)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUSA))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUSA);
    }
    else
    {
        lpStatusPtr = lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumServicesStatusA((SC_RPC_HANDLE)hSCManager,
                                       dwServiceType,
                                       dwServiceState,
                                       (LPBYTE)lpStatusPtr,
                                       dwBufferSize,
                                       pcbBytesNeeded,
                                       lpServicesReturned,
                                       lpResumeHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
        for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
        {
            if (lpStatusPtr->lpServiceName)
                lpStatusPtr->lpServiceName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

            if (lpStatusPtr->lpDisplayName)
                lpStatusPtr->lpDisplayName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

            lpStatusPtr++;
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumServicesStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusW
 *
 * @implemented
 */
BOOL WINAPI
EnumServicesStatusW(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSW lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    ENUM_SERVICE_STATUSW ServiceStatus;
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumServicesStatusW() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pcbBytesNeeded == NULL || lpServicesReturned == NULL)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUSW))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUSW);
    }
    else
    {
        lpStatusPtr = lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumServicesStatusW((SC_RPC_HANDLE)hSCManager,
                                       dwServiceType,
                                       dwServiceState,
                                       (LPBYTE)lpStatusPtr,
                                       dwBufferSize,
                                       pcbBytesNeeded,
                                       lpServicesReturned,
                                       lpResumeHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
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
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumServicesStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusExA
 *
 * @implemented
 */
BOOL WINAPI
EnumServicesStatusExA(SC_HANDLE hSCManager,
                      SC_ENUM_TYPE InfoLevel,
                      DWORD dwServiceType,
                      DWORD dwServiceState,
                      LPBYTE lpServices,
                      DWORD cbBufSize,
                      LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned,
                      LPDWORD lpResumeHandle,
                      LPCSTR pszGroupName)
{
    ENUM_SERVICE_STATUS_PROCESSA ServiceStatus;
    LPENUM_SERVICE_STATUS_PROCESSA lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumServicesStatusExA() called\n");

    if (InfoLevel != SC_ENUM_PROCESS_INFO)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pcbBytesNeeded == NULL || lpServicesReturned == NULL)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUS_PROCESSA))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUS_PROCESSA);
    }
    else
    {
        lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSA)lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumServicesStatusExA((SC_RPC_HANDLE)hSCManager,
                                         InfoLevel,
                                         dwServiceType,
                                         dwServiceState,
                                         (LPBYTE)lpStatusPtr,
                                         dwBufferSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned,
                                         lpResumeHandle,
                                         (LPSTR)pszGroupName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
        if (InfoLevel == SC_ENUM_PROCESS_INFO)
        {
            for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
            {
                if (lpStatusPtr->lpServiceName)
                    lpStatusPtr->lpServiceName =
                        (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

                if (lpStatusPtr->lpDisplayName)
                    lpStatusPtr->lpDisplayName =
                        (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

                lpStatusPtr++;
            }
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumServicesStatusExA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusExA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusExW
 *
 * @implemented
 */
BOOL WINAPI
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
    ENUM_SERVICE_STATUS_PROCESSW ServiceStatus;
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;
    DWORD dwCount;

    TRACE("EnumServicesStatusExW() called\n");

    if (InfoLevel != SC_ENUM_PROCESS_INFO)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pcbBytesNeeded == NULL || lpServicesReturned == NULL)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    if (lpServices == NULL || cbBufSize < sizeof(ENUM_SERVICE_STATUS_PROCESSW))
    {
        lpStatusPtr = &ServiceStatus;
        dwBufferSize = sizeof(ENUM_SERVICE_STATUS_PROCESSW);
    }
    else
    {
        lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSW)lpServices;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        dwError = REnumServicesStatusExW((SC_RPC_HANDLE)hSCManager,
                                         InfoLevel,
                                         dwServiceType,
                                         dwServiceState,
                                         (LPBYTE)lpStatusPtr,
                                         dwBufferSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned,
                                         lpResumeHandle,
                                         (LPWSTR)pszGroupName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_SUCCESS || dwError == ERROR_MORE_DATA)
    {
        if (InfoLevel == SC_ENUM_PROCESS_INFO)
        {
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
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("REnumServicesStatusExW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusExW() done\n");

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameA
 *
 * @implemented
 */
BOOL WINAPI
GetServiceDisplayNameA(SC_HANDLE hSCManager,
                       LPCSTR lpServiceName,
                       LPSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;
    LPSTR lpNameBuffer;
    CHAR szEmptyName[] = "";

    TRACE("GetServiceDisplayNameA() called\n");
    TRACE("%p %s %p %p\n", hSCManager,
          debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpDisplayName || *lpcchBuffer < sizeof(CHAR))
    {
        lpNameBuffer = szEmptyName;
        *lpcchBuffer = sizeof(CHAR);
    }
    else
    {
        lpNameBuffer = lpDisplayName;
    }

    RpcTryExcept
    {
        dwError = RGetServiceDisplayNameA((SC_RPC_HANDLE)hSCManager,
                                          lpServiceName,
                                          lpNameBuffer,
                                          lpcchBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        /* HACK: because of a problem with rpcrt4, rpcserver is hacked to return 6 for ERROR_SERVICE_DOES_NOT_EXIST */
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RGetServiceDisplayNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameW
 *
 * @implemented
 */
BOOL WINAPI
GetServiceDisplayNameW(SC_HANDLE hSCManager,
                       LPCWSTR lpServiceName,
                       LPWSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;
    LPWSTR lpNameBuffer;
    WCHAR szEmptyName[] = L"";

    TRACE("GetServiceDisplayNameW() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpDisplayName || *lpcchBuffer < sizeof(WCHAR))
    {
        lpNameBuffer = szEmptyName;
        *lpcchBuffer = sizeof(WCHAR);
    }
    else
    {
        lpNameBuffer = lpDisplayName;
    }

    RpcTryExcept
    {
        dwError = RGetServiceDisplayNameW((SC_RPC_HANDLE)hSCManager,
                                          lpServiceName,
                                          lpNameBuffer,
                                          lpcchBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RGetServiceDisplayNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameA
 *
 * @implemented
 */
BOOL WINAPI
GetServiceKeyNameA(SC_HANDLE hSCManager,
                   LPCSTR lpDisplayName,
                   LPSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;
    LPSTR lpNameBuffer;
    CHAR szEmptyName[] = "";

    TRACE("GetServiceKeyNameA() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpServiceName || *lpcchBuffer < sizeof(CHAR))
    {
        lpNameBuffer = szEmptyName;
        *lpcchBuffer = sizeof(CHAR);
    }
    else
    {
        lpNameBuffer = lpServiceName;
    }

    RpcTryExcept
    {
        dwError = RGetServiceKeyNameA((SC_RPC_HANDLE)hSCManager,
                                      lpDisplayName,
                                      lpNameBuffer,
                                      lpcchBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RGetServiceKeyNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameW
 *
 * @implemented
 */
BOOL WINAPI
GetServiceKeyNameW(SC_HANDLE hSCManager,
                   LPCWSTR lpDisplayName,
                   LPWSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;
    LPWSTR lpNameBuffer;
    WCHAR szEmptyName[] = L"";

    TRACE("GetServiceKeyNameW() called\n");

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!lpServiceName || *lpcchBuffer < sizeof(WCHAR))
    {
        lpNameBuffer = szEmptyName;
        *lpcchBuffer = sizeof(WCHAR);
    }
    else
    {
        lpNameBuffer = lpServiceName;
    }

    RpcTryExcept
    {
        dwError = RGetServiceKeyNameW((SC_RPC_HANDLE)hSCManager,
                                      lpDisplayName,
                                      lpNameBuffer,
                                      lpcchBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RGetServiceKeyNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  LockServiceDatabase
 *
 * @implemented
 */
SC_LOCK WINAPI
LockServiceDatabase(SC_HANDLE hSCManager)
{
    SC_LOCK hLock;
    DWORD dwError;

    TRACE("LockServiceDatabase(%x)\n", hSCManager);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RLockServiceDatabase((SC_RPC_HANDLE)hSCManager,
                                       (SC_RPC_LOCK *)&hLock);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RLockServiceDatabase() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hLock = %p\n", hLock);

    return hLock;
}


static VOID
WaitForSCManager(VOID)
{
    HANDLE hEvent;

    TRACE("WaitForSCManager() called\n");

    /* Try to open the existing event */
    hEvent = OpenEventW(SYNCHRONIZE, FALSE, SCM_START_EVENT);
    if (hEvent == NULL)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) return;

        /* Try to create a new event */
        hEvent = CreateEventW(NULL, TRUE, FALSE, SCM_START_EVENT);
        if (hEvent == NULL) return;
    }

    /* Wait for 3 minutes */
    WaitForSingleObject(hEvent, 180000);
    CloseHandle(hEvent);

    TRACE("ScmWaitForSCManager() done\n");
}


/**********************************************************************
 *  OpenSCManagerA
 *
 * @implemented
 */
SC_HANDLE WINAPI
OpenSCManagerA(LPCSTR lpMachineName,
               LPCSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
    SC_HANDLE hScm = NULL;
    DWORD dwError;

    TRACE("OpenSCManagerA(%s, %s, %lx)\n",
           lpMachineName, lpDatabaseName, dwDesiredAccess);

    WaitForSCManager();

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = ROpenSCManagerA((LPSTR)lpMachineName,
                                  (LPSTR)lpDatabaseName,
                                  dwDesiredAccess,
                                  (SC_RPC_HANDLE *)&hScm);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenSCManagerA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hScm = %p\n", hScm);

    return hScm;
}


/**********************************************************************
 *  OpenSCManagerW
 *
 * @implemented
 */
SC_HANDLE WINAPI
OpenSCManagerW(LPCWSTR lpMachineName,
               LPCWSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
    SC_HANDLE hScm = NULL;
    DWORD dwError;

    TRACE("OpenSCManagerW(%S, %S, %lx)\n",
           lpMachineName, lpDatabaseName, dwDesiredAccess);

    WaitForSCManager();

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = ROpenSCManagerW((LPWSTR)lpMachineName,
                                  (LPWSTR)lpDatabaseName,
                                  dwDesiredAccess,
                                  (SC_RPC_HANDLE *)&hScm);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenSCManagerW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hScm = %p\n", hScm);

    return hScm;
}


/**********************************************************************
 *  OpenServiceA
 *
 * @implemented
 */
SC_HANDLE WINAPI
OpenServiceA(SC_HANDLE hSCManager,
             LPCSTR lpServiceName,
             DWORD dwDesiredAccess)
{
    SC_HANDLE hService = NULL;
    DWORD dwError;

    TRACE("OpenServiceA(%p, %s, %lx)\n",
           hSCManager, lpServiceName, dwDesiredAccess);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = ROpenServiceA((SC_RPC_HANDLE)hSCManager,
                                (LPSTR)lpServiceName,
                                dwDesiredAccess,
                                (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenServiceA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hService = %p\n", hService);

    return hService;
}


/**********************************************************************
 *  OpenServiceW
 *
 * @implemented
 */
SC_HANDLE WINAPI
OpenServiceW(SC_HANDLE hSCManager,
             LPCWSTR lpServiceName,
             DWORD dwDesiredAccess)
{
    SC_HANDLE hService = NULL;
    DWORD dwError;

    TRACE("OpenServiceW(%p, %S, %lx)\n",
           hSCManager, lpServiceName, dwDesiredAccess);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = ROpenServiceW((SC_RPC_HANDLE)hSCManager,
                                (LPWSTR)lpServiceName,
                                dwDesiredAccess,
                                (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hService = %p\n", hService);

    return hService;
}


/**********************************************************************
 *  QueryServiceConfigA
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceConfigA(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGA lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
    QUERY_SERVICE_CONFIGA ServiceConfig;
    LPQUERY_SERVICE_CONFIGA lpConfigPtr;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceConfigA(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    if (lpServiceConfig == NULL ||
        cbBufSize < sizeof(QUERY_SERVICE_CONFIGA))
    {
        lpConfigPtr = &ServiceConfig;
        dwBufferSize = sizeof(QUERY_SERVICE_CONFIGA);
    }
    else
    {
        lpConfigPtr = lpServiceConfig;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceConfigA((SC_RPC_HANDLE)hService,
                                       (LPBYTE)lpConfigPtr,
                                       dwBufferSize,
                                       pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceConfigA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpConfigPtr->lpBinaryPathName)
        lpConfigPtr->lpBinaryPathName =
            (LPSTR)((ULONG_PTR)lpConfigPtr +
                    (ULONG_PTR)lpConfigPtr->lpBinaryPathName);

    if (lpConfigPtr->lpLoadOrderGroup)
        lpConfigPtr->lpLoadOrderGroup =
            (LPSTR)((ULONG_PTR)lpConfigPtr +
                    (ULONG_PTR)lpConfigPtr->lpLoadOrderGroup);

    if (lpConfigPtr->lpDependencies)
        lpConfigPtr->lpDependencies =
            (LPSTR)((ULONG_PTR)lpConfigPtr +
                    (ULONG_PTR)lpConfigPtr->lpDependencies);

    if (lpConfigPtr->lpServiceStartName)
        lpConfigPtr->lpServiceStartName =
            (LPSTR)((ULONG_PTR)lpConfigPtr +
                    (ULONG_PTR)lpConfigPtr->lpServiceStartName);

    if (lpConfigPtr->lpDisplayName)
        lpConfigPtr->lpDisplayName =
           (LPSTR)((ULONG_PTR)lpConfigPtr +
                   (ULONG_PTR)lpConfigPtr->lpDisplayName);

    TRACE("QueryServiceConfigA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfigW
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceConfigW(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
    QUERY_SERVICE_CONFIGW ServiceConfig;
    LPQUERY_SERVICE_CONFIGW lpConfigPtr;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceConfigW(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    if (lpServiceConfig == NULL ||
        cbBufSize < sizeof(QUERY_SERVICE_CONFIGW))
    {
        lpConfigPtr = &ServiceConfig;
        dwBufferSize = sizeof(QUERY_SERVICE_CONFIGW);
    }
    else
    {
        lpConfigPtr = lpServiceConfig;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceConfigW((SC_RPC_HANDLE)hService,
                                       (LPBYTE)lpConfigPtr,
                                       dwBufferSize,
                                       pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceConfigW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpConfigPtr->lpBinaryPathName)
        lpConfigPtr->lpBinaryPathName =
            (LPWSTR)((ULONG_PTR)lpConfigPtr +
                     (ULONG_PTR)lpConfigPtr->lpBinaryPathName);

    if (lpConfigPtr->lpLoadOrderGroup)
        lpConfigPtr->lpLoadOrderGroup =
            (LPWSTR)((ULONG_PTR)lpConfigPtr +
                     (ULONG_PTR)lpConfigPtr->lpLoadOrderGroup);

    if (lpConfigPtr->lpDependencies)
        lpConfigPtr->lpDependencies =
            (LPWSTR)((ULONG_PTR)lpConfigPtr +
                     (ULONG_PTR)lpConfigPtr->lpDependencies);

    if (lpConfigPtr->lpServiceStartName)
        lpConfigPtr->lpServiceStartName =
            (LPWSTR)((ULONG_PTR)lpConfigPtr +
                     (ULONG_PTR)lpConfigPtr->lpServiceStartName);

    if (lpConfigPtr->lpDisplayName)
        lpConfigPtr->lpDisplayName =
           (LPWSTR)((ULONG_PTR)lpConfigPtr +
                    (ULONG_PTR)lpConfigPtr->lpDisplayName);

    TRACE("QueryServiceConfigW() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfig2A
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceConfig2A(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    SERVICE_DESCRIPTIONA ServiceDescription;
    SERVICE_FAILURE_ACTIONSA ServiceFailureActions;
    LPBYTE lpTempBuffer;
    BOOL bUseTempBuffer = FALSE;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceConfig2A(hService %p, dwInfoLevel %lu, lpBuffer %p, cbBufSize %lu, pcbBytesNeeded %p)\n",
          hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    lpTempBuffer = lpBuffer;
    dwBufferSize = cbBufSize;

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            if ((lpBuffer == NULL) || (cbBufSize < sizeof(SERVICE_DESCRIPTIONA)))
            {
                lpTempBuffer = (LPBYTE)&ServiceDescription;
                dwBufferSize = sizeof(SERVICE_DESCRIPTIONA);
                bUseTempBuffer = TRUE;
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            if ((lpBuffer == NULL) || (cbBufSize < sizeof(SERVICE_FAILURE_ACTIONSA)))
            {
                lpTempBuffer = (LPBYTE)&ServiceFailureActions;
                dwBufferSize = sizeof(SERVICE_FAILURE_ACTIONSA);
                bUseTempBuffer = TRUE;
            }
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceConfig2A((SC_RPC_HANDLE)hService,
                                        dwInfoLevel,
                                        lpTempBuffer,
                                        dwBufferSize,
                                        pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceConfig2A() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (bUseTempBuffer == TRUE)
    {
        TRACE("RQueryServiceConfig2A() returns ERROR_INSUFFICIENT_BUFFER\n");
        *pcbBytesNeeded = dwBufferSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONA lpPtr = (LPSERVICE_DESCRIPTIONA)lpTempBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSA lpPtr = (LPSERVICE_FAILURE_ACTIONSA)lpTempBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (LPSC_ACTION)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpsaActions);
            }
            break;
    }

    TRACE("QueryServiceConfig2A() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfig2W
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceConfig2W(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    SERVICE_DESCRIPTIONW ServiceDescription;
    SERVICE_FAILURE_ACTIONSW ServiceFailureActions;
    LPBYTE lpTempBuffer;
    BOOL bUseTempBuffer = FALSE;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceConfig2W(%p, %lu, %p, %lu, %p)\n",
           hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    lpTempBuffer = lpBuffer;
    dwBufferSize = cbBufSize;

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            if ((lpBuffer == NULL) || (cbBufSize < sizeof(SERVICE_DESCRIPTIONW)))
            {
                lpTempBuffer = (LPBYTE)&ServiceDescription;
                dwBufferSize = sizeof(SERVICE_DESCRIPTIONW);
                bUseTempBuffer = TRUE;
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            if ((lpBuffer == NULL) || (cbBufSize < sizeof(SERVICE_FAILURE_ACTIONSW)))
            {
                lpTempBuffer = (LPBYTE)&ServiceFailureActions;
                dwBufferSize = sizeof(SERVICE_FAILURE_ACTIONSW);
                bUseTempBuffer = TRUE;
            }
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceConfig2W((SC_RPC_HANDLE)hService,
                                        dwInfoLevel,
                                        lpTempBuffer,
                                        dwBufferSize,
                                        pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (bUseTempBuffer == TRUE)
    {
        TRACE("RQueryServiceConfig2W() returns ERROR_INSUFFICIENT_BUFFER\n");
        *pcbBytesNeeded = dwBufferSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONW lpPtr = (LPSERVICE_DESCRIPTIONW)lpTempBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPWSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSW lpPtr = (LPSERVICE_FAILURE_ACTIONSW)lpTempBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPWSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPWSTR)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (LPSC_ACTION)((ULONG_PTR)lpPtr + (ULONG_PTR)lpPtr->lpsaActions);
            }
            break;
    }

    TRACE("QueryServiceConfig2W() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusA
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceLockStatusA(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    QUERY_SERVICE_LOCK_STATUSA LockStatus;
    LPQUERY_SERVICE_LOCK_STATUSA lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceLockStatusA() called\n");

    if (lpLockStatus == NULL || cbBufSize < sizeof(QUERY_SERVICE_LOCK_STATUSA))
    {
        lpStatusPtr = &LockStatus;
        dwBufferSize = sizeof(QUERY_SERVICE_LOCK_STATUSA);
    }
    else
    {
        lpStatusPtr = lpLockStatus;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceLockStatusA((SC_RPC_HANDLE)hSCManager,
                                           (LPBYTE)lpStatusPtr,
                                           dwBufferSize,
                                           pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceLockStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpStatusPtr->lpLockOwner != NULL)
    {
        lpStatusPtr->lpLockOwner =
            (LPSTR)((ULONG_PTR)lpStatusPtr + (ULONG_PTR)lpStatusPtr->lpLockOwner);
    }

    TRACE("QueryServiceLockStatusA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusW
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceLockStatusW(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    QUERY_SERVICE_LOCK_STATUSW LockStatus;
    LPQUERY_SERVICE_LOCK_STATUSW lpStatusPtr;
    DWORD dwBufferSize;
    DWORD dwError;

    TRACE("QueryServiceLockStatusW() called\n");

    if (lpLockStatus == NULL || cbBufSize < sizeof(QUERY_SERVICE_LOCK_STATUSW))
    {
        lpStatusPtr = &LockStatus;
        dwBufferSize = sizeof(QUERY_SERVICE_LOCK_STATUSW);
    }
    else
    {
        lpStatusPtr = lpLockStatus;
        dwBufferSize = cbBufSize;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceLockStatusW((SC_RPC_HANDLE)hSCManager,
                                           (LPBYTE)lpStatusPtr,
                                           dwBufferSize,
                                           pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceLockStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpStatusPtr->lpLockOwner != NULL)
    {
        lpStatusPtr->lpLockOwner =
            (LPWSTR)((ULONG_PTR)lpStatusPtr + (ULONG_PTR)lpStatusPtr->lpLockOwner);
    }

    TRACE("QueryServiceLockStatusW() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceObjectSecurity
 *
 * @implemented
 */
BOOL WINAPI
QueryServiceObjectSecurity(SC_HANDLE hService,
                           SECURITY_INFORMATION dwSecurityInformation,
                           PSECURITY_DESCRIPTOR lpSecurityDescriptor,
                           DWORD cbBufSize,
                           LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceObjectSecurity(%p, %lu, %p)\n",
           hService, dwSecurityInformation, lpSecurityDescriptor);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceObjectSecurity((SC_RPC_HANDLE)hService,
                                              dwSecurityInformation,
                                              (LPBYTE)lpSecurityDescriptor,
                                              cbBufSize,
                                              pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("QueryServiceObjectSecurity() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
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

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RSetServiceObjectSecurity((SC_RPC_HANDLE)hService,
                                            dwSecurityInformation,
                                            (LPBYTE)SelfRelativeSD,
                                            Length);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    HeapFree(GetProcessHeap(), 0, SelfRelativeSD);

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RServiceObjectSecurity() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
QueryServiceStatus(SC_HANDLE hService,
                   LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("QueryServiceStatus(%p, %p)\n",
           hService, lpServiceStatus);

    if (!hService)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceStatus((SC_RPC_HANDLE)hService,
                                      lpServiceStatus);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceStatus() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
QueryServiceStatusEx(SC_HANDLE hService,
                     SC_STATUS_TYPE InfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceStatusEx() called\n");

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
    {
        *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RQueryServiceStatusEx((SC_RPC_HANDLE)hService,
                                        InfoLevel,
                                        lpBuffer,
                                        cbBufSize,
                                        pcbBytesNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RQueryServiceStatusEx() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  StartServiceA
 *
 * @implemented
 */
BOOL WINAPI
StartServiceA(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCSTR *lpServiceArgVectors)
{
    DWORD dwError;

    RpcTryExcept
    {
        dwError = RStartServiceA((SC_RPC_HANDLE)hService,
                                 dwNumServiceArgs,
                                 (LPSTRING_PTRSA)lpServiceArgVectors);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        ERR("RStartServiceA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  StartServiceW
 *
 * @implemented
 */
BOOL WINAPI
StartServiceW(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCWSTR *lpServiceArgVectors)
{
    DWORD dwError;

    RpcTryExcept
    {
        dwError = RStartServiceW((SC_RPC_HANDLE)hService,
                                 dwNumServiceArgs,
                                 (LPSTRING_PTRSW)lpServiceArgVectors);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        ERR("RStartServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  UnlockServiceDatabase
 *
 * @implemented
 */
BOOL WINAPI
UnlockServiceDatabase(SC_LOCK ScLock)
{
    DWORD dwError;

    TRACE("UnlockServiceDatabase(%x)\n", ScLock);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RUnlockServiceDatabase((LPSC_RPC_LOCK)&ScLock);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RUnlockServiceDatabase() failed (Error %lu)\n", dwError);
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
BOOL WINAPI
NotifyBootConfigStatus(BOOL BootAcceptable)
{
    DWORD dwError;

    TRACE("NotifyBootConfigStatus()\n");

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
        dwError = RNotifyBootConfigStatus(NULL,
                                          BootAcceptable);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("NotifyBootConfigStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
