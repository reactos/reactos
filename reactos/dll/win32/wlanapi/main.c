/*
 * Wireless LAN API (wlanapi.dll)
 *
 * Copyright 2009 Christoph von Wittich (Christoph@ApiViewer.de)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


/* INCLUDES ****************************************************************/
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wlansvc_c.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wlanapi);

handle_t __RPC_USER
WLANSVC_HANDLE_bind(WLANSVC_HANDLE szMachineName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS Status;

    TRACE("RPC_SERVICE_STATUS_HANDLE_bind() called\n");

    Status = RpcStringBindingComposeW(NULL,
                                      L"ncalrpc",
                                      szMachineName,
                                      L"wlansvc",
                                      NULL,
                                      &pszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", Status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    Status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", Status);
    }

    Status = RpcStringFreeW(&pszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringFree returned 0x%x\n", Status);
    }

    return hBinding;
}

void __RPC_USER
WLANSVC_HANDLE_unbind(WLANSVC_HANDLE szMachineName,
                                 handle_t hBinding)
{
    RPC_STATUS Status;

    TRACE("WLANSVC_HANDLE_unbind() called\n");

    Status = RpcBindingFree(&hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", Status);
    }
}

PVOID
WINAPI
WlanAllocateMemory(IN DWORD dwSize)
{
    return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

VOID
WINAPI
WlanFreeMemory(IN PVOID pMem)
{
    HeapFree(GetProcessHeap(), 0, pMem);
}

DWORD
WINAPI
WlanConnect(IN HANDLE hClientHandle,
            IN const GUID *pInterfaceGuid,
            IN const PWLAN_CONNECTION_PARAMETERS pConnectionParameters,
            PVOID pReserved)
{
    DWORD dwError = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        _RpcConnect(hClientHandle, pInterfaceGuid, &pConnectionParameters);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

DWORD
WINAPI
WlanDisconnect(IN HANDLE hClientHandle,
               IN const GUID *pInterfaceGuid,
               PVOID pReserved)
{
    DWORD dwError = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        _RpcDisconnect(hClientHandle, pInterfaceGuid);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

DWORD
WINAPI
WlanOpenHandle(IN DWORD dwClientVersion,
               PVOID pReserved,
               OUT DWORD *pdwNegotiatedVersion,
               OUT HANDLE *phClientHandle)
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szDummy[] = L"localhost";

    if ((pReserved != NULL) || (pdwNegotiatedVersion == NULL) || (phClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwError = _RpcOpenHandle(szDummy,
                                dwClientVersion,
                                pdwNegotiatedVersion,
                                (WLANSVC_RPC_HANDLE) phClientHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

DWORD
WINAPI
WlanCloseHandle(IN HANDLE hClientHandle,
                PVOID pReserved)
{
    DWORD dwError = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        _RpcCloseHandle(hClientHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

DWORD
WINAPI
WlanEnumInterfaces(IN HANDLE hClientHandle,
                   PVOID pReserved,
                   OUT PWLAN_INTERFACE_INFO_LIST *ppInterfaceList)
{
    DWORD dwError = ERROR_SUCCESS;

    if ((pReserved != NULL) || (ppInterfaceList == NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        _RpcEnumInterfaces(hClientHandle, ppInterfaceList);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

DWORD
WINAPI
WlanScan(IN HANDLE hClientHandle,
         IN const GUID *pInterfaceGuid,
         IN PDOT11_SSID pDot11Ssid,
         IN PWLAN_RAW_DATA pIeData,
         PVOID pReserved)
{
    DWORD dwError = ERROR_SUCCESS;

    if ((pReserved != NULL) || (pInterfaceGuid == NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        _RpcScan(hClientHandle, pInterfaceGuid, pDot11Ssid, pIeData);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwError;
}

void __RPC_FAR * __RPC_USER
midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER
midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

