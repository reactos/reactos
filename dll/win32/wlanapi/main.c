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

DWORD
WlanRpcStatusToWinError(RPC_STATUS Status)
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
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (pConnectionParameters == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcConnect(hClientHandle, pInterfaceGuid, &pConnectionParameters);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanDisconnect(IN HANDLE hClientHandle,
               IN const GUID *pInterfaceGuid,
               PVOID pReserved)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcDisconnect(hClientHandle, pInterfaceGuid);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanOpenHandle(IN DWORD dwClientVersion,
               PVOID pReserved,
               OUT DWORD *pdwNegotiatedVersion,
               OUT HANDLE *phClientHandle)
{
    DWORD dwResult = ERROR_SUCCESS;
    WCHAR szDummy[] = L"localhost";

    if ((pReserved != NULL) || (pdwNegotiatedVersion == NULL) || (phClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcOpenHandle(szDummy,
                                dwClientVersion,
                                pdwNegotiatedVersion,
                                (WLANSVC_RPC_HANDLE) phClientHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanCloseHandle(IN HANDLE hClientHandle,
                PVOID pReserved)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcCloseHandle(&hClientHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanEnumInterfaces(IN HANDLE hClientHandle,
                   PVOID pReserved,
                   OUT PWLAN_INTERFACE_INFO_LIST *ppInterfaceList)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (ppInterfaceList == NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcEnumInterfaces(hClientHandle, ppInterfaceList);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanScan(IN HANDLE hClientHandle,
         IN const GUID *pInterfaceGuid,
         IN PDOT11_SSID pDot11Ssid,
         IN PWLAN_RAW_DATA pIeData,
         PVOID pReserved)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (pInterfaceGuid == NULL) || (hClientHandle == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcScan(hClientHandle, pInterfaceGuid, pDot11Ssid, pIeData);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanQueryInterface(IN HANDLE hClientHandle,
                   IN const GUID *pInterfaceGuid,
                   IN WLAN_INTF_OPCODE OpCode,
                   PVOID pReserved,
                   OUT PDWORD pdwDataSize,
                   OUT PVOID *ppData,
                   WLAN_OPCODE_VALUE_TYPE *pWlanOpcodeValueType)
{
    if ((pReserved != NULL) || (pInterfaceGuid == NULL) || (hClientHandle == NULL) || (pdwDataSize == NULL) || (ppData == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanGetInterfaceCapability(IN HANDLE hClientHandle,
                           IN const GUID *pInterfaceGuid,
                           PVOID pReserved,
                           OUT PWLAN_INTERFACE_CAPABILITY *ppCapability)
{
    if ((pReserved != NULL) || (pInterfaceGuid == NULL) || (hClientHandle == NULL) || (ppCapability == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD WINAPI WlanRegisterNotification(IN HANDLE hClientHandle,
                                      IN DWORD dwNotifSource,
                                      IN BOOL bIgnoreDuplicate,
                                      WLAN_NOTIFICATION_CALLBACK funcCallback,
                                      PVOID pCallbackContext,
                                      PVOID pReserved,
                                      PDWORD pdwPrevNotifSource)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanReasonCodeToString(IN DWORD dwReasonCode,
                       IN DWORD dwBufferSize,
                       IN PWCHAR pStringBuffer,
                       PVOID pReserved)
{
    if ((pReserved != NULL) || (pStringBuffer == NULL) || (dwBufferSize == 0))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanIhvControl(IN HANDLE hClientHandle,
               IN const GUID *pInterfaceGuid,
               IN WLAN_IHV_CONTROL_TYPE Type,
               IN DWORD dwInBufferSize,
               IN PVOID pInBuffer,
               IN DWORD dwOutBufferSize,
               PVOID pOutBuffer,
               OUT PDWORD pdwBytesReturned)
{
    if ((hClientHandle == NULL) || (pInterfaceGuid == NULL) || (pdwBytesReturned == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanSetSecuritySettings(IN HANDLE hClientHandle,
                        IN WLAN_SECURABLE_OBJECT SecurableObject,
                        IN LPCWSTR strModifiedSDDL)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((hClientHandle == NULL) || (strModifiedSDDL == NULL) || (SecurableObject >= WLAN_SECURABLE_OBJECT_COUNT))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcSetSecuritySettings(hClientHandle, SecurableObject, strModifiedSDDL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = WlanRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanGetAvailableNetworkList(IN HANDLE hClientHandle,
                            IN const GUID *pInterfaceGuid,
                            IN DWORD dwFlags,
                            PVOID pReserved,
                            OUT PWLAN_AVAILABLE_NETWORK_LIST *ppAvailableNetworkList)
{
    if ((pReserved != NULL) || (pInterfaceGuid == NULL) || (hClientHandle == NULL) || (ppAvailableNetworkList == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
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

