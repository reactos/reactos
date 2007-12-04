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

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

handle_t BindingHandle = NULL;

VOID
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
 *  ChangeServiceConfig2A
 *
 * @implemented
 */
BOOL WINAPI
ChangeServiceConfig2A(SC_HANDLE hService,
                      DWORD dwInfoLevel,
                      LPVOID lpInfo)
{
    DWORD lpInfoSize;
    DWORD dwError;

    DPRINT("ChangeServiceConfig2A() called\n");

    /* Determine the length of the lpInfo parameter */
    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            lpInfoSize = sizeof(SERVICE_DESCRIPTIONA);
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            lpInfoSize = sizeof(SERVICE_FAILURE_ACTIONSA);
            break;

        default:
            DPRINT1("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (lpInfo == NULL)
        return TRUE;

    HandleBind();

    dwError = ScmrChangeServiceConfig2A(BindingHandle,
                                        (unsigned int)hService,
                                        dwInfoLevel,
                                        lpInfo,
                                        lpInfoSize);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrChangeServiceConfig2A() failed (Error %lu)\n", dwError);
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
    LPBYTE lpSendData = NULL;
    DWORD dwInfoSize;
    DWORD dwError;

    DPRINT("ChangeServiceConfig2W() called\n");

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
        {
            LPSERVICE_DESCRIPTIONW lpServiceDescription = lpInfo;
            DWORD dwStringSize;

            dwInfoSize = sizeof(SERVICE_DESCRIPTIONW);
            dwStringSize = (wcslen(lpServiceDescription->lpDescription) + 1) * sizeof(WCHAR);
            dwInfoSize += dwStringSize;

            lpSendData = HeapAlloc(GetProcessHeap(), 0, dwInfoSize);
            if (lpSendData)
            {
                LPBYTE pt = lpSendData;

                CopyMemory(pt, lpInfo, sizeof(SERVICE_DESCRIPTIONW));
                pt += sizeof(SERVICE_DESCRIPTIONW);
                CopyMemory(pt, lpServiceDescription->lpDescription, dwStringSize);
            }
            else
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            break;
        }

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            dwInfoSize = sizeof(SERVICE_FAILURE_ACTIONSW);
            break;

        default:
            DPRINT1("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (lpInfo == NULL)
        goto done;

    HandleBind();

    dwError = ScmrChangeServiceConfig2W(BindingHandle,
                                        (unsigned int)hService,
                                        dwInfoLevel,
                                        lpSendData,
                                        dwInfoSize);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrChangeServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

done:
    if (lpSendData != NULL)
        HeapFree(GetProcessHeap(), 0, lpSendData);

    DPRINT("ChangeServiceConfig2W() done\n");

    return TRUE;
}


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
SC_HANDLE STDCALL
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
    int len;
    LPSTR lpStr;

    if (lpServiceName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, NULL, 0);
        lpServiceNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpServiceNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, lpServiceNameW, len);
    }

    if (lpDisplayName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, NULL, 0);
        lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpDisplayNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, lpDisplayNameW, len);
    }

    if (lpBinaryPathName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpBinaryPathName, -1, NULL, 0);
        lpBinaryPathNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpBinaryPathNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, lpBinaryPathNameW, len);
    }

    if (lpLoadOrderGroup)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, NULL, 0);
        lpLoadOrderGroupW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpLoadOrderGroupW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, lpLoadOrderGroupW, len);
    }

    if (lpDependencies)
    {
        lpStr = (LPSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = strlen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;

        lpDependenciesW = HeapAlloc(GetProcessHeap(), 0, dwDependenciesLength * sizeof(WCHAR));
        if (!lpDependenciesW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpDependencies, -1, lpDependenciesW, dwDependenciesLength);
    }

    if (lpServiceStartName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, NULL, 0);
        lpServiceStartName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpServiceStartNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, lpServiceStartNameW, len);
    }

    if (lpPassword)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, NULL, 0);
        lpPasswordW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpPasswordW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, lpPasswordW, len);
    }

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
    if (lpServiceNameW !=NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceNameW);

    if (lpDisplayNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpDisplayNameW);

    if (lpBinaryPathNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpBinaryPathNameW);

    if (lpLoadOrderGroupW != NULL)
        HeapFree(GetProcessHeap(), 0, lpLoadOrderGroupW);

    if (lpDependenciesW != NULL)
        HeapFree(GetProcessHeap(), 0, lpDependenciesW);

    if (lpServiceStartNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceStartNameW);

    if (lpPasswordW != NULL)
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
    DWORD dwDependenciesLength = 0;
    DWORD dwError;
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
                                 (unsigned long *)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrCreateServiceW() failed (Error %lu)\n", dwError);
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
 * @implemented
 */
BOOL STDCALL
EnumDependentServicesA(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSA lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT("EnumServicesStatusA() called\n");

    HandleBind();

    dwError = ScmrEnumDependentServicesA(BindingHandle,
                                         (unsigned int)hService,
                                         dwServiceState,
                                         (unsigned char *)lpServices,
                                         cbBufSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned);

    lpStatusPtr = (LPENUM_SERVICE_STATUSA)lpServices;
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

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrEnumDependentServicesA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("EnumDependentServicesA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesW
 *
 * @implemented
 */
BOOL STDCALL
EnumDependentServicesW(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSW lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT("EnumServicesStatusW() called\n");

    HandleBind();

    dwError = ScmrEnumDependentServicesW(BindingHandle,
                                         (unsigned int)hService,
                                         dwServiceState,
                                         (unsigned char *)lpServices,
                                         cbBufSize,
                                         pcbBytesNeeded,
                                         lpServicesReturned);

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
        DPRINT("ScmrEnumDependentServicesW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("EnumDependentServicesW() done\n");

    return TRUE;
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
    LPENUM_SERVICE_STATUSW  lpServices,
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
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusA(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSA lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT("EnumServicesStatusA() called\n");

    HandleBind();

    dwError = ScmrEnumServicesStatusA(BindingHandle,
                                      (unsigned int)hSCManager,
                                      dwServiceType,
                                      dwServiceState,
                                      (unsigned char *)lpServices,
                                      cbBufSize,
                                      pcbBytesNeeded,
                                      lpServicesReturned,
                                      lpResumeHandle);

    lpStatusPtr = (LPENUM_SERVICE_STATUSA)lpServices;
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

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrEnumServicesStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("EnumServicesStatusA() done\n");

    return TRUE;
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

    DPRINT("EnumServicesStatusW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusExA
 *
 * @implemented
 */
BOOL STDCALL
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
    LPENUM_SERVICE_STATUS_PROCESSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    DPRINT("EnumServicesStatusExA() called\n");

    HandleBind();

    dwError = ScmrEnumServicesStatusExA(BindingHandle,
                                        (unsigned int)hSCManager,
                                        (unsigned long)InfoLevel,
                                        dwServiceType,
                                        dwServiceState,
                                        (unsigned char *)lpServices,
                                        cbBufSize,
                                        pcbBytesNeeded,
                                        lpServicesReturned,
                                        lpResumeHandle,
                                        (char *)pszGroupName);

    if (dwError == ERROR_MORE_DATA)
    {
        DPRINT("Required buffer size %ul\n", *pcbBytesNeeded);
        SetLastError(dwError);
        return FALSE;
    }
    else if (dwError == ERROR_SUCCESS)
    {
        lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSA)lpServices;
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
    else
    {
        DPRINT1("ScmrEnumServicesStatusExA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("EnumServicesStatusExA() done\n");

    return TRUE;
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

    DPRINT("EnumServicesStatusExW() called\n");

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

    if (dwError == ERROR_MORE_DATA)
    {
        DPRINT("Required buffer size %ul\n", *pcbBytesNeeded);
        SetLastError(dwError);
        return FALSE;
    }
    else if (dwError == ERROR_SUCCESS)
    {
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
    }
    else
    {
        DPRINT1("ScmrEnumServicesStatusExW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    DPRINT("EnumServicesStatusExW() done\n");

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
                        L"SvcctrlStartEvent_A3752DX");
    if (hEvent == NULL)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            return;

        /* Try to create a new event */
        hEvent = CreateEventW(NULL,
                              TRUE,
                              FALSE,
                              L"SvcctrlStartEvent_A3752DX");
        if (hEvent == NULL)
        {
            /* Try to open the existing event again */
            hEvent = OpenEventW(SYNCHRONIZE,
                                FALSE,
                                L"SvcctrlStartEvent_A3752DX");
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
                                 (unsigned long*)&hScm);
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
                                 (unsigned long*)&hScm);
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
                               (unsigned long*)&hService);
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
                               (unsigned long*)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrOpenServiceW() failed (Error %lu)\n", dwError);
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
    DWORD dwError;

    DPRINT("QueryServiceConfigW(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceConfigW(BindingHandle,
                                      (unsigned int)hService,
                                      (unsigned char *)lpServiceConfig,
                                      cbBufSize,
                                      pcbBytesNeeded);
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
}


/**********************************************************************
 *  QueryServiceConfig2A
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfig2A(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceConfig2A(%p, %lu, %p, %lu, %p)\n",
           hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceConfig2A(BindingHandle,
                                       (unsigned int)hService,
                                       dwInfoLevel,
                                       (unsigned char *)lpBuffer,
                                       cbBufSize,
                                       pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceConfig2A() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONA lpPtr = (LPSERVICE_DESCRIPTIONA)lpBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSA lpPtr = (LPSERVICE_FAILURE_ACTIONSA)lpBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (SC_ACTION*)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpsaActions);
            }
            break;

        default:
            DPRINT1("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    DPRINT("QueryServiceConfig2A() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfig2W
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfig2W(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceConfig2W(%p, %lu, %p, %lu, %p)\n",
           hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceConfig2W(BindingHandle,
                                       (unsigned int)hService,
                                       dwInfoLevel,
                                       (unsigned char *)lpBuffer,
                                       cbBufSize,
                                       pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONW lpPtr = (LPSERVICE_DESCRIPTIONW)lpBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSW lpPtr = (LPSERVICE_FAILURE_ACTIONSW)lpBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (SC_ACTION*)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpsaActions);
            }
            break;

        default:
            DPRINT1("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    DPRINT("QueryServiceConfig2W() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusA
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceLockStatusA(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceLockStatusA() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceLockStatusA(BindingHandle,
                                          (unsigned int)hSCManager,
                                          (unsigned char *)lpLockStatus,
                                          cbBufSize,
                                          pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceLockStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpLockStatus->lpLockOwner != NULL)
    {
        lpLockStatus->lpLockOwner =
            (LPSTR)((UINT_PTR)lpLockStatus + (UINT_PTR)lpLockStatus->lpLockOwner);
    }

    DPRINT("QueryServiceLockStatusA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusW
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceLockStatusW(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    DPRINT("QueryServiceLockStatusW() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ScmrQueryServiceLockStatusW(BindingHandle,
                                          (unsigned int)hSCManager,
                                          (unsigned char *)lpLockStatus,
                                          cbBufSize,
                                          pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT("ScmrQueryServiceLockStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpLockStatus->lpLockOwner != NULL)
    {
        lpLockStatus->lpLockOwner =
            (LPWSTR)((UINT_PTR)lpLockStatus + (UINT_PTR)lpLockStatus->lpLockOwner);
    }

    DPRINT("QueryServiceLockStatusW() done\n");

    return TRUE;
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
 *  StartServiceA
 *
 * @implemented
 */
BOOL STDCALL
StartServiceA(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCSTR *lpServiceArgVectors)
{
    LPSTR lpBuffer;
    LPSTR lpStr;
    DWORD dwError;
    DWORD dwBufSize;
    DWORD i;

    dwBufSize = 0;
    for (i = 0; i < dwNumServiceArgs; i++)
    {
        dwBufSize += (strlen(lpServiceArgVectors[i]) + 1);
    }
    dwBufSize++;
    DPRINT1("dwBufSize: %lu\n", dwBufSize);

    lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwBufSize);
    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lpStr = lpBuffer;
    for (i = 0; i < dwNumServiceArgs; i++)
    {
        strcpy(lpStr, lpServiceArgVectors[i]);
        lpStr += (strlen(lpServiceArgVectors[i]) + 1);
    }
    *lpStr = 0;

    dwError = ScmrStartServiceA(BindingHandle,
                                (unsigned int)hService,
                                dwNumServiceArgs,
                                (unsigned char *)lpBuffer,
                                dwBufSize);

    HeapFree(GetProcessHeap(), 0, lpBuffer);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrStartServiceA() failed (Error %lu)\n", dwError);
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
BOOL STDCALL
StartServiceW(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCWSTR *lpServiceArgVectors)
{
    LPWSTR lpBuffer;
    LPWSTR lpStr;
    DWORD dwError;
    DWORD dwBufSize;
    DWORD i;

    dwBufSize = 0;
    for (i = 0; i < dwNumServiceArgs; i++)
    {
        dwBufSize += ((wcslen(lpServiceArgVectors[i]) + 1) * sizeof(WCHAR));
    }
    dwBufSize += sizeof(WCHAR);
    DPRINT("dwBufSize: %lu\n", dwBufSize);

    lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwBufSize);
    if (lpBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lpStr = lpBuffer;
    for (i = 0; i < dwNumServiceArgs; i++)
    {
        wcscpy(lpStr, lpServiceArgVectors[i]);
        lpStr += (wcslen(lpServiceArgVectors[i]) + 1);
    }
    *lpStr = 0;

    dwError = ScmrStartServiceW(BindingHandle,
                                (unsigned int)hService,
                                dwNumServiceArgs,
                                (unsigned char *)lpBuffer,
                                dwBufSize);

    HeapFree(GetProcessHeap(), 0, lpBuffer);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("ScmrStartServiceW() failed (Error %lu)\n", dwError);
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

/* EOF */
