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
WINAPI
WlanDeleteProfile(IN HANDLE hClientHandle,
                  IN const GUID *pInterfaceGuid,
                  IN LPCWSTR strProfileName,
                  PVOID pReserved)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strProfileName == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcDeleteProfile(hClientHandle, pInterfaceGuid, strProfileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanRenameProfile(IN HANDLE hClientHandle,
                  IN const GUID *pInterfaceGuid,
                  IN LPCWSTR strOldProfileName,
                  IN LPCWSTR strNewProfileName,
                  PVOID pReserved)
{
    DWORD dwResult = ERROR_SUCCESS;

    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strOldProfileName == NULL) || (strNewProfileName == NULL))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        dwResult = _RpcRenameProfile(hClientHandle, pInterfaceGuid, strOldProfileName, strNewProfileName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwResult = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwResult;
}

DWORD
WINAPI
WlanGetProfile(IN HANDLE hClientHandle,
               IN const GUID *pInterfaceGuid,
               IN LPCWSTR strProfileName,
               PVOID pReserved,
               OUT LPWSTR *pstrProfileXml,
               DWORD *pdwFlags,
               PDWORD pdwGrantedAccess)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (pstrProfileXml  == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanSetProfile(IN HANDLE hClientHandle,
               IN const GUID *pInterfaceGuid,
               IN DWORD dwFlags,
               IN LPCWSTR strProfileXml,
               LPCWSTR strAllUserProfileSecurity,
               IN BOOL bOverwrite,
               PVOID pReserved,
               OUT DWORD *pdwReasonCode)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strProfileXml == NULL) || (pdwReasonCode == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanGetProfileCustomUserData(IN HANDLE hClientHandle,
                             IN const GUID *pInterfaceGuid,
                             IN LPCWSTR strProfileName,
                             PVOID pReserved,
                             OUT DWORD *pdwDataSize,
                             OUT PBYTE *ppData)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strProfileName == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanSetProfileCustomUserData(IN HANDLE hClientHandle,
                             IN const GUID *pInterfaceGuid,
                             IN LPCWSTR strProfileName,
                             IN DWORD dwDataSize,
                             IN const PBYTE pData,
                             PVOID pReserved)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strProfileName == NULL))
        return ERROR_INVALID_PARAMETER;

    if ((dwDataSize != 0) && (pData == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanGetProfileList(IN HANDLE hClientHandle,
                   IN const GUID *pInterfaceGuid,
                   PVOID pReserved,
                   OUT PWLAN_PROFILE_INFO_LIST *ppProfileList)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (ppProfileList  == NULL))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WlanSetProfileList(IN HANDLE hClientHandle,
                   IN const GUID *pInterfaceGuid,
                   DWORD dwItems,
                   IN LPCWSTR *strProfileNames,
                   PVOID pReserved)
{
    if ((pReserved != NULL) || (hClientHandle == NULL) || (pInterfaceGuid == NULL) || (strProfileNames  == NULL) || (dwItems == 0))
        return ERROR_INVALID_PARAMETER;

    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}
