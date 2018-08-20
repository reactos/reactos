/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/scm.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 *                  Eric Kohl
 *                  Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

NTSTATUS
WINAPI
SystemFunction004(
    const struct ustring *in,
    const struct ustring *key,
    struct ustring *out);

NTSTATUS
WINAPI
SystemFunction028(
    IN PVOID ContextHandle,
    OUT LPBYTE SessionKey);

/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
SVCCTL_HANDLEA_bind(SVCCTL_HANDLEA szMachineName)
{
    handle_t hBinding = NULL;
    RPC_CSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEA_bind(%s)\n",
          debugstr_a(szMachineName));

    status = RpcStringBindingComposeA(NULL,
                                      (RPC_CSTR)"ncacn_np",
                                      (RPC_CSTR)szMachineName,
                                      (RPC_CSTR)"\\pipe\\ntsvcs",
                                      NULL,
                                      &pszStringBinding);
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

    TRACE("SVCCTL_HANDLEA_unbind(%s %p)\n",
          debugstr_a(szMachineName), hBinding);

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
    RPC_WSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("SVCCTL_HANDLEW_bind(%s)\n",
          debugstr_w(szMachineName));

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

    TRACE("SVCCTL_HANDLEW_unbind(%s %p)\n",
          debugstr_w(szMachineName), hBinding);

    status = RpcBindingFree(&hBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
}


DWORD
ScmRpcStatusToWinError(RPC_STATUS Status)
{
    TRACE("ScmRpcStatusToWinError(%lx)\n",
          Status);

    switch (Status)
    {
        case STATUS_ACCESS_VIOLATION:
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


static
DWORD
ScmEncryptPassword(
    _In_ PCWSTR pClearTextPassword,
    _Out_ PBYTE *pEncryptedPassword,
    _Out_ PDWORD pEncryptedPasswordSize)
{
    struct ustring inData, keyData, outData;
    BYTE SessionKey[16];
    PBYTE pBuffer;
    NTSTATUS Status;

    /* Get the session key */
    Status = SystemFunction028(NULL,
                               SessionKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("SystemFunction028 failed (Status 0x%08lx)\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    inData.Length = (wcslen(pClearTextPassword) + 1) * sizeof(WCHAR);
    inData.MaximumLength = inData.Length;
    inData.Buffer = (unsigned char *)pClearTextPassword;

    keyData.Length = sizeof(SessionKey);
    keyData.MaximumLength = keyData.Length;
    keyData.Buffer = SessionKey;

    outData.Length = 0;
    outData.MaximumLength = 0;
    outData.Buffer = NULL;

    /* Get the required buffer size */
    Status = SystemFunction004(&inData,
                               &keyData,
                               &outData);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        ERR("SystemFunction004 failed (Status 0x%08lx)\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    /* Allocate a buffer for the encrypted password */
    pBuffer = HeapAlloc(GetProcessHeap(), 0, outData.Length);
    if (pBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    outData.MaximumLength = outData.Length;
    outData.Buffer = pBuffer;

    /* Encrypt the password */
    Status = SystemFunction004(&inData,
                               &keyData,
                               &outData);
    if (!NT_SUCCESS(Status))
    {
        ERR("SystemFunction004 failed (Status 0x%08lx)\n", Status);
        HeapFree(GetProcessHeap(), 0, pBuffer);
        return RtlNtStatusToDosError(Status);
    }

    *pEncryptedPassword = outData.Buffer;
    *pEncryptedPasswordSize = outData.Length;

    return ERROR_SUCCESS;
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

    TRACE("ChangeServiceConfig2A(%p %lu %p)\n",
          hService, dwInfoLevel, lpInfo);

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
        dwError = RChangeServiceConfig2A((SC_RPC_HANDLE)(ULONG_PTR)hService,
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

    TRACE("ChangeServiceConfig2W(%p %lu %p)\n",
          hService, dwInfoLevel, lpInfo);

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
    DWORD dwPasswordSize = 0;
    LPWSTR lpPasswordW = NULL;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("ChangeServiceConfigA(%p %lu %lu %lu %s %s %p %s %s %s %s)\n",
          hService, dwServiceType, dwStartType, dwErrorControl, debugstr_a(lpBinaryPathName),
          debugstr_a(lpLoadOrderGroup), lpdwTagId, debugstr_a(lpDependencies),
          debugstr_a(lpServiceStartName), debugstr_a(lpPassword), debugstr_a(lpDisplayName));

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

    if (lpPassword != NULL)
    {
        /* Convert the password to unicode */
        lpPasswordW = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (strlen(lpPassword) + 1) * sizeof(WCHAR));
        if (lpPasswordW == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpPassword,
                            -1,
                            lpPasswordW,
                            (int)(strlen(lpPassword) + 1));

        /* Encrypt the unicode password */
        dwError = ScmEncryptPassword(lpPasswordW,
                                     &lpEncryptedPassword,
                                     &dwPasswordSize);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    RpcTryExcept
    {
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
                                        dwPasswordSize,
                                        (LPSTR)lpDisplayName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

done:
    if (lpPasswordW != NULL)
    {
        /* Wipe and release the password buffers */
        SecureZeroMemory(lpPasswordW, (wcslen(lpPasswordW) + 1) * sizeof(WCHAR));
        HeapFree(GetProcessHeap(), 0, lpPasswordW);

        if (lpEncryptedPassword != NULL)
        {
            SecureZeroMemory(lpEncryptedPassword, dwPasswordSize);
            HeapFree(GetProcessHeap(), 0, lpEncryptedPassword);
        }
    }

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
    DWORD dwPasswordSize = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("ChangeServiceConfigW(%p %lu %lu %lu %s %s %p %s %s %s %s)\n",
          hService, dwServiceType, dwStartType, dwErrorControl, debugstr_w(lpBinaryPathName),
          debugstr_w(lpLoadOrderGroup), lpdwTagId, debugstr_w(lpDependencies),
          debugstr_w(lpServiceStartName), debugstr_w(lpPassword), debugstr_w(lpDisplayName));

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

    if (lpPassword != NULL)
    {
        dwError = ScmEncryptPassword(lpPassword,
                                     &lpEncryptedPassword,
                                     &dwPasswordSize);
        if (dwError != ERROR_SUCCESS)
        {
            ERR("ScmEncryptPassword failed (Error %lu)\n", dwError);
            goto done;
        }
    }

    RpcTryExcept
    {
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
                                        dwPasswordSize,
                                        (LPWSTR)lpDisplayName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

done:
    if (lpEncryptedPassword != NULL)
    {
        /* Wipe and release the password buffer */
        SecureZeroMemory(lpEncryptedPassword, dwPasswordSize);
        HeapFree(GetProcessHeap(), 0, lpEncryptedPassword);
    }

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

    TRACE("CloseServiceHandle(%p)\n",
          hSCObject);

    if (!hSCObject)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RpcTryExcept
    {
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

    TRACE("ControlService(%p %lu %p)\n",
          hService, dwControl, lpServiceStatus);

    RpcTryExcept
    {
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
    FIXME("ControlServiceEx(%p %lu %lu %p)\n",
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
    DWORD dwPasswordSize = 0;
    LPWSTR lpPasswordW = NULL;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("CreateServiceA(%p %s %s %lx %lu %lu %lu %s %s %p %s %s %s)\n",
          hSCManager, debugstr_a(lpServiceName), debugstr_a(lpDisplayName),
          dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
          debugstr_a(lpBinaryPathName), debugstr_a(lpLoadOrderGroup), lpdwTagId,
          debugstr_a(lpDependencies), debugstr_a(lpServiceStartName), debugstr_a(lpPassword));

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

    if (lpPassword != NULL)
    {
        /* Convert the password to unicode */
        lpPasswordW = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (strlen(lpPassword) + 1) * sizeof(WCHAR));
        if (lpPasswordW == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP,
                            0,
                            lpPassword,
                            -1,
                            lpPasswordW,
                            (int)(strlen(lpPassword) + 1));

        /* Encrypt the password */
        dwError = ScmEncryptPassword(lpPasswordW,
                                     &lpEncryptedPassword,
                                     &dwPasswordSize);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    RpcTryExcept
    {
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
                                  dwPasswordSize,
                                  (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

done:
    if (lpPasswordW != NULL)
    {
        /* Wipe and release the password buffers */
        SecureZeroMemory(lpPasswordW, (wcslen(lpPasswordW) + 1) * sizeof(WCHAR));
        HeapFree(GetProcessHeap(), 0, lpPasswordW);

        if (lpEncryptedPassword != NULL)
        {
            SecureZeroMemory(lpEncryptedPassword, dwPasswordSize);
            HeapFree(GetProcessHeap(), 0, lpEncryptedPassword);
        }
    }

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RCreateServiceA() failed (Error %lu)\n", dwError);
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
    DWORD dwPasswordSize = 0;
    LPBYTE lpEncryptedPassword = NULL;

    TRACE("CreateServiceW(%p %s %s %lx %lu %lu %lu %s %s %p %s %s %s)\n",
          hSCManager, debugstr_w(lpServiceName), debugstr_w(lpDisplayName),
          dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl,
          debugstr_w(lpBinaryPathName), debugstr_w(lpLoadOrderGroup), lpdwTagId,
          debugstr_w(lpDependencies), debugstr_w(lpServiceStartName), debugstr_w(lpPassword));

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

    if (lpPassword != NULL)
    {
        /* Encrypt the password */
        dwError = ScmEncryptPassword(lpPassword,
                                     &lpEncryptedPassword,
                                     &dwPasswordSize);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    RpcTryExcept
    {
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
                                  dwPasswordSize,
                                  (SC_RPC_HANDLE *)&hService);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

done:
    if (lpEncryptedPassword != NULL)
    {
        /* Wipe and release the password buffers */
        SecureZeroMemory(lpEncryptedPassword, dwPasswordSize);
        HeapFree(GetProcessHeap(), 0, lpEncryptedPassword);
    }

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RCreateServiceW() failed (Error %lu)\n", dwError);
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

    TRACE("DeleteService(%p)\n",
          hService);

    RpcTryExcept
    {
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

    TRACE("EnumDependentServicesA(%p %lu %p %lu %p %p)\n",
          hService, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned);

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

    TRACE("EnumDependentServicesW(%p %lu %p %lu %p %p)\n",
          hService, dwServiceState, lpServices, cbBufSize,
          pcbBytesNeeded, lpServicesReturned);

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

    TRACE("EnumServiceGroupW(%p %lu %lu %p %lu %p %p %p %s)\n",
          hSCManager, dwServiceType, dwServiceState, lpServices,
          cbBufSize, pcbBytesNeeded, lpServicesReturned,
          lpResumeHandle, debugstr_w(lpGroup));

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

    TRACE("EnumServicesStatusA(%p %lu %lu %p %lu %p %p %p)\n",
          hSCManager, dwServiceType, dwServiceState, lpServices,
          cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle);

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

    TRACE("EnumServicesStatusW(%p %lu %lu %p %lu %p %p %p)\n",
          hSCManager, dwServiceType, dwServiceState, lpServices,
          cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle);

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

    TRACE("EnumServicesStatusExA(%p %lu %lu %p %lu %p %p %p %s)\n",
          hSCManager, dwServiceType, dwServiceState, lpServices,
          cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle,
          debugstr_a(pszGroupName));

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

    TRACE("EnumServicesStatusExW(%p %lu %lu %p %lu %p %p %p %s)\n",
          hSCManager, dwServiceType, dwServiceState, lpServices,
          cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle,
          debugstr_w(pszGroupName));

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

    TRACE("GetServiceDisplayNameA(%p %s %p %p)\n",
          hSCManager, debugstr_a(lpServiceName), lpDisplayName, lpcchBuffer);

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

    TRACE("GetServiceDisplayNameW(%p %s %p %p)\n",
          hSCManager, debugstr_w(lpServiceName), lpDisplayName, lpcchBuffer);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /*
     * NOTE: A size of 1 character would be enough, but tests show that
     * Windows returns 2 characters instead, certainly due to a WCHAR/bytes
     * mismatch in their code.
     */
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

    TRACE("GetServiceKeyNameA(%p %s %p %p)\n",
          hSCManager, debugstr_a(lpDisplayName), lpServiceName, lpcchBuffer);

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
        /* HACK: because of a problem with rpcrt4, rpcserver is hacked to return 6 for ERROR_SERVICE_DOES_NOT_EXIST */
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

    TRACE("GetServiceKeyNameW(%p %s %p %p)\n",
          hSCManager, debugstr_w(lpDisplayName), lpServiceName, lpcchBuffer);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /*
     * NOTE: A size of 1 character would be enough, but tests show that
     * Windows returns 2 characters instead, certainly due to a WCHAR/bytes
     * mismatch in their code.
     */
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
 *  I_ScGetCurrentGroupStateW
 *
 * @implemented
 */
DWORD WINAPI
I_ScGetCurrentGroupStateW(SC_HANDLE hSCManager,
                          LPWSTR pszGroupName,
                          LPDWORD pdwGroupState)
{
    DWORD dwError;

    TRACE("I_ScGetCurrentGroupStateW(%p %s %p)\n",
          hSCManager, debugstr_w(pszGroupName), pdwGroupState);

    RpcTryExcept
    {
        dwError = RI_ScGetCurrentGroupStateW((SC_RPC_HANDLE)hSCManager,
                                             pszGroupName,
                                             pdwGroupState);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept

    if (dwError != ERROR_SUCCESS)
    {
        TRACE("RI_ScGetCurrentGroupStateW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
    }

    return dwError;
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

    TRACE("LockServiceDatabase(%p)\n",
          hSCManager);

    RpcTryExcept
    {
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

    TRACE("WaitForSCManager()\n");

    /* Try to open the existing event */
    hEvent = OpenEventW(SYNCHRONIZE, FALSE, SCM_START_EVENT);
    if (hEvent == NULL)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            return;

        /* Try to create a new event */
        hEvent = CreateEventW(NULL, TRUE, FALSE, SCM_START_EVENT);
        if (hEvent == NULL)
            return;
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

    TRACE("OpenSCManagerA(%s %s %lx)\n",
          debugstr_a(lpMachineName), debugstr_a(lpDatabaseName), dwDesiredAccess);

    WaitForSCManager();

    RpcTryExcept
    {
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

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenSCManagerA() failed (Error %lu)\n", dwError);
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

    TRACE("OpenSCManagerW(%s %s %lx)\n",
          debugstr_w(lpMachineName), debugstr_w(lpDatabaseName), dwDesiredAccess);

    WaitForSCManager();

    RpcTryExcept
    {
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

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenSCManagerW() failed (Error %lu)\n", dwError);
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

    TRACE("OpenServiceA(%p %s %lx)\n",
           hSCManager, debugstr_a(lpServiceName), dwDesiredAccess);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    RpcTryExcept
    {
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

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenServiceA() failed (Error %lu)\n", dwError);
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

    TRACE("OpenServiceW(%p %s %lx)\n",
           hSCManager, debugstr_w(lpServiceName), dwDesiredAccess);

    if (!hSCManager)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    RpcTryExcept
    {
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

    SetLastError(dwError);
    if (dwError != ERROR_SUCCESS)
    {
        TRACE("ROpenServiceW() failed (Error %lu)\n", dwError);
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

    TRACE("QueryServiceConfigA(%p %p %lu %p)\n",
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

    TRACE("QueryServiceConfigW(%p %p %lu %p)\n",
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

    TRACE("QueryServiceConfig2A(%p %lu %p %lu %p)\n",
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

    if (bUseTempBuffer != FALSE)
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

    TRACE("QueryServiceConfig2W(%p %lu %p %lu %p)\n",
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

    if (bUseTempBuffer != FALSE)
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

    TRACE("QueryServiceLockStatusA(%p %p %lu %p)\n",
          hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

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

    TRACE("QueryServiceLockStatusW(%p %p %lu %p)\n",
          hSCManager, lpLockStatus, cbBufSize, pcbBytesNeeded);

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

    TRACE("QueryServiceObjectSecurity(%p %lu %p)\n",
           hService, dwSecurityInformation, lpSecurityDescriptor);

    RpcTryExcept
    {
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

    TRACE("SetServiceObjectSecurity(%p %lu %p)\n",
          hService, dwSecurityInformation, lpSecurityDescriptor);

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

    TRACE("QueryServiceStatus(%p %p)\n",
          hService, lpServiceStatus);

    if (!hService)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RpcTryExcept
    {
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

    TRACE("QueryServiceStatusEx(%p %lu %p %lu %p)\n",
          hService, InfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

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

    TRACE("StartServiceA(%p %lu %p)\n",
          hService, dwNumServiceArgs, lpServiceArgVectors);

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

    TRACE("StartServiceW(%p %lu %p)\n",
          hService, dwNumServiceArgs, lpServiceArgVectors);

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

    TRACE("UnlockServiceDatabase(%x)\n",
          ScLock);

    RpcTryExcept
    {
        dwError = RUnlockServiceDatabase((LPSC_RPC_LOCK)&ScLock);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError == ERROR_INVALID_HANDLE)
        dwError = ERROR_INVALID_SERVICE_LOCK;

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

    TRACE("NotifyBootConfigStatus(%u)\n",
          BootAcceptable);

    RpcTryExcept
    {
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

DWORD
I_ScQueryServiceTagInfo(PVOID Unused,
                        TAG_INFO_LEVEL dwInfoLevel,
                        PTAG_INFO_NAME_FROM_TAG InOutParams)
{
    SC_HANDLE hScm;
    DWORD dwError;
    PTAG_INFO_NAME_FROM_TAG_IN_PARAMS InParams;
    PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS OutParams;
    LPWSTR lpszName;

    /* We only support one class */
    if (dwInfoLevel != TagInfoLevelNameFromTag)
    {
        return ERROR_RETRY;
    }

    /* Validate input structure */
    if (InOutParams->InParams.dwPid == 0 || InOutParams->InParams.dwTag == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Validate output structure */
    if (InOutParams->OutParams.pszName != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Open service manager */
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hScm == NULL)
    {
        return GetLastError();
    }

    /* Setup call parameters */
    InParams = &InOutParams->InParams;
    OutParams = NULL;

    /* Call SCM to query tag information */
    RpcTryExcept
    {
        dwError = RI_ScQueryServiceTagInfo(hScm, TagInfoLevelNameFromTag, &InParams, &OutParams);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    /* Quit if not a success */
    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    /* OutParams must be set now and we must have a name */
    if (OutParams == NULL ||
        OutParams->pszName == NULL)
    {
        dwError = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    /* Copy back what SCM returned */
    lpszName = LocalAlloc(LPTR,
                          sizeof(WCHAR) * wcslen(OutParams->pszName) + sizeof(UNICODE_NULL));
    if (lpszName == NULL)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    wcscpy(lpszName, OutParams->pszName);
    InOutParams->OutParams.pszName = lpszName;
    InOutParams->OutParams.TagType = OutParams->TagType;

Cleanup:
    CloseServiceHandle(hScm);

    /* Free memory allocated by SCM */
    if (OutParams != NULL)
    {
        if (OutParams->pszName != NULL)
        {
            midl_user_free(OutParams->pszName);
        }

        midl_user_free(OutParams);
    }

    return dwError;
}

/**********************************************************************
 *  I_QueryTagInformation
 *
 * @implemented
 */
DWORD WINAPI
I_QueryTagInformation(PVOID Unused,
                      TAG_INFO_LEVEL dwInfoLevel,
                      PTAG_INFO_NAME_FROM_TAG InOutParams)
{
    /*
     * We only support one information class and it
     * needs parameters
     */
    if (dwInfoLevel != TagInfoLevelNameFromTag ||
        InOutParams == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Validate input structure */
    if (InOutParams->InParams.dwPid == 0 || InOutParams->InParams.dwTag == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Validate output structure */
    if (InOutParams->OutParams.pszName != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Call internal function for the RPC call */
    return I_ScQueryServiceTagInfo(Unused, TagInfoLevelNameFromTag, InOutParams);
}

/* EOF */
