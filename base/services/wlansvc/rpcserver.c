/*
 * PROJECT:     ReactOS WLAN Service
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/services/wlansvc/rpcserver.c
 * PURPOSE:     RPC server interface
 * COPYRIGHT:   Copyright 2009 Christoph von Wittich
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>
//#define GET_IF_ENTRY2_IMPLEMENTED 1

LIST_ENTRY WlanSvcHandleListHead;

DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    InitializeListHead(&WlanSvcHandleListHead);
    
    Status = RpcServerUseProtseqEpW(L"ncalrpc", 20, L"wlansvc", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(wlansvc_interface_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    DPRINT("RpcServerListen finished\n");
    return 0;
}

PWLANSVCHANDLE WlanSvcGetHandleEntry(LPWLANSVC_RPC_HANDLE ClientHandle)
{
    PLIST_ENTRY CurrentEntry;
    PWLANSVCHANDLE lpWlanSvcHandle;

    CurrentEntry = WlanSvcHandleListHead.Flink;
    while (CurrentEntry != &WlanSvcHandleListHead)
    {
        lpWlanSvcHandle = CONTAINING_RECORD(CurrentEntry,
                                        WLANSVCHANDLE,
                                        WlanSvcHandleListEntry);
        CurrentEntry = CurrentEntry->Flink;

        if (lpWlanSvcHandle == (PWLANSVCHANDLE) ClientHandle)
            return lpWlanSvcHandle;
    }

    return NULL;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcOpenHandle(
#else
DWORD _RpcOpenHandle(
#endif
    wchar_t *arg_1,
    DWORD dwClientVersion,
    DWORD *pdwNegotiatedVersion,
    LPWLANSVC_RPC_HANDLE phClientHandle)
{
    PWLANSVCHANDLE lpWlanSvcHandle;

    lpWlanSvcHandle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WLANSVCHANDLE));
    if (lpWlanSvcHandle == NULL)
    {
        DPRINT1("Failed to allocate Heap!\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (dwClientVersion > 2)
        dwClientVersion = 2;
    
    if (dwClientVersion < 1)
        dwClientVersion = 1;
    
    lpWlanSvcHandle->dwClientVersion = dwClientVersion;
    *pdwNegotiatedVersion = dwClientVersion;
    
    InsertTailList(&WlanSvcHandleListHead, &lpWlanSvcHandle->WlanSvcHandleListEntry);
    *phClientHandle = lpWlanSvcHandle;

    return ERROR_SUCCESS;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcCloseHandle(
#else
DWORD _RpcCloseHandle(
#endif
    LPWLANSVC_RPC_HANDLE phClientHandle)
{
    PWLANSVCHANDLE lpWlanSvcHandle;

    lpWlanSvcHandle = WlanSvcGetHandleEntry(*phClientHandle);
    if (!lpWlanSvcHandle)
    {
        return ERROR_INVALID_HANDLE;
    }

    RemoveEntryList(&lpWlanSvcHandle->WlanSvcHandleListEntry);
    HeapFree(GetProcessHeap(), 0, lpWlanSvcHandle);

    return ERROR_SUCCESS;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcEnumInterfaces(
#else
DWORD _RpcEnumInterfaces(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    PWLAN_INTERFACE_INFO_LIST *ppInterfaceList)
{
#if GET_IF_ENTRY2_IMPLEMENTED
    DWORD dwNumInterfaces;
    DWORD dwResult, dwSize;
    DWORD dwIndex;
    MIB_IF_ROW2 IfRow;
    PWLAN_INTERFACE_INFO_LIST InterfaceList;

    dwResult = GetNumberOfInterfaces(&dwNumInterfaces);
    dwSize = sizeof(WLAN_INTERFACE_INFO_LIST);
    if (dwResult != NO_ERROR)
    {
        /* set num interfaces to zero when an error occurs */
        dwNumInterfaces = 0;
    }
    else
    {
        if (dwNumInterfaces > 1)
        {
            /* add extra size for interface */
            dwSize += (dwNumInterfaces-1) * sizeof(WLAN_INTERFACE_INFO);
        }
    }

    /* allocate interface list */
    InterfaceList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (!InterfaceList)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppInterfaceList = InterfaceList;
    if (!dwNumInterfaces)
    {
        return ERROR_SUCCESS;
    }

    for(dwIndex = 0; dwIndex < dwNumInterfaces; dwIndex++)
    {
        ZeroMemory(&IfRow, sizeof(MIB_IF_ROW2));
        IfRow.InterfaceIndex = dwIndex;

        dwResult = GetIfEntry2(&IfRow);
        if (dwResult == NO_ERROR)
        {
            if (IfRow.Type == IF_TYPE_IEEE80211 && IfRow.InterfaceAndOperStatusFlags.HardwareInterface)
            {
                RtlMoveMemory(&InterfaceList->InterfaceInfo[InterfaceList->dwNumberOfItems].InterfaceGuid, &IfRow.InterfaceGuid, sizeof(GUID));
                wcscpy(InterfaceList->InterfaceInfo[InterfaceList->dwNumberOfItems].strInterfaceDescription, IfRow.Description);
                //FIXME set state
                InterfaceList->dwNumberOfItems++;
            }
        }
    }

    return ERROR_SUCCESS;
#else
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
#endif
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetAutoConfigParameter(
#else
DWORD _RpcSetAutoConfigParameter(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    long OpCode,
    DWORD dwDataSize,
    LPBYTE pData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcQueryAutoConfigParameter(
#else
DWORD _RpcQueryAutoConfigParameter(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    DWORD OpCode,
    LPDWORD pdwDataSize,
    char **ppData,
    DWORD *pWlanOpcodeValueType)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetInterfaceCapability(
#else
DWORD _RpcGetInterfaceCapability(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    PWLAN_INTERFACE_CAPABILITY *ppCapability)
{
    PWLANSVCHANDLE lpWlanSvcHandle;

    lpWlanSvcHandle = WlanSvcGetHandleEntry(hClientHandle);
    if (!lpWlanSvcHandle)
    {
        return ERROR_INVALID_HANDLE;
    }

    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetInterface(
#else
DWORD _RpcSetInterface(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    DWORD OpCode,
    DWORD dwDataSize,
    LPBYTE pData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcQueryInterface(
#else
DWORD _RpcQueryInterface(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    long OpCode,
    LPDWORD pdwDataSize,
    LPBYTE *ppData,
    LPDWORD pWlanOpcodeValueType)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcIhvControl(
#else
DWORD _RpcIhvControl(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    DWORD Type,
    DWORD dwInBufferSize,
    LPBYTE pInBuffer,
    DWORD dwOutBufferSize,
    LPBYTE pOutBuffer,
    LPDWORD pdwBytesReturned)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcScan(
#else
DWORD _RpcScan(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    PDOT11_SSID pDot11Ssid,
    PWLAN_RAW_DATA pIeData)
{
    PWLANSVCHANDLE lpWlanSvcHandle;

    lpWlanSvcHandle = WlanSvcGetHandleEntry(hClientHandle);
    if (!lpWlanSvcHandle)
    {
        return ERROR_INVALID_HANDLE;
    }
    
    /*
    DWORD dwBytesReturned;
    HANDLE hDevice;
    ULONG OidCode = OID_802_11_BSSID_LIST_SCAN;
    PNDIS_802_11_BSSID_LIST pBssIDList;

    DeviceIoControl(hDevice,
                    IOCTL_NDIS_QUERY_GLOBAL_STATS,
                    &OidCode,
                    sizeof(ULONG),
                    NULL,
                    0,
                    &dwBytesReturned,
                    NULL);
*/
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetAvailableNetworkList(
#else
DWORD _RpcGetAvailableNetworkList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    DWORD dwFlags,
    WLAN_AVAILABLE_NETWORK_LIST **ppAvailableNetworkList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetNetworkBssList(
#else
DWORD _RpcGetNetworkBssList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    PDOT11_SSID pDot11Ssid,
    short dot11BssType,
    DWORD bSecurityEnabled,
    LPDWORD dwBssListSize,
    LPBYTE *ppWlanBssList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcConnect(
#else
DWORD _RpcConnect(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    const PWLAN_CONNECTION_PARAMETERS *pConnectionParameters)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcDisconnect(
#else
DWORD _RpcDisconnect(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGUID)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcRegisterNotification(
#else
DWORD _RpcRegisterNotification(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    DWORD arg_2,
    LPDWORD pdwPrevNotifSource)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcAsyncGetNotification(
#else
DWORD _RpcAsyncGetNotification(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    PWLAN_NOTIFICATION_DATA *NotificationData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetProfileEapUserData(
#else
DWORD _RpcSetProfileEapUserData(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    EAP_METHOD_TYPE MethodType,
    DWORD dwFlags,
    DWORD dwEapUserDataSize,
    LPBYTE pbEapUserData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetProfile(
#else
DWORD _RpcSetProfile(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    DWORD dwFlags,
    wchar_t *strProfileXml,
    wchar_t *strAllUserProfileSecurity,
    BOOL bOverwrite,
    LPDWORD pdwReasonCode)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetProfile(
#else
DWORD _RpcGetProfile(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    wchar_t **pstrProfileXml,
    LPDWORD pdwFlags,
    LPDWORD pdwGrantedAccess)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcDeleteProfile(
#else
DWORD _RpcDeleteProfile(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    const wchar_t *strProfileName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcRenameProfile(
#else
DWORD _RpcRenameProfile(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    const wchar_t *strOldProfileName,
    const wchar_t *strNewProfileName)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetProfileList(
#else
DWORD _RpcSetProfileList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    DWORD dwItems,
    BYTE **strProfileNames)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetProfileList(
#else
DWORD _RpcGetProfileList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    PWLAN_PROFILE_INFO_LIST *ppProfileList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetProfilePosition(
#else
DWORD _RpcSetProfilePosition(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    DWORD dwPosition)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetProfileCustomUserData(
#else
DWORD _RpcSetProfileCustomUserData(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    DWORD dwDataSize,
    LPBYTE pData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetProfileCustomUserData(
#else
DWORD _RpcGetProfileCustomUserData(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    LPDWORD dwDataSize,
    LPBYTE *pData)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD _stdcall _RpcSetFilterList(
    WLANSVC_RPC_HANDLE hClientHandle,
    short wlanFilterListType,
    PDOT11_NETWORK_LIST pNetworkList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetFilterList(
#else
DWORD _RpcGetFilterList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    short wlanFilterListType,
    PDOT11_NETWORK_LIST *pNetworkList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetPsdIEDataList(
#else
DWORD _RpcSetPsdIEDataList(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    wchar_t *strFormat,
    DWORD dwDataListSize,
    LPBYTE pPsdIEDataList)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSaveTemporaryProfile(
#else
DWORD _RpcSaveTemporaryProfile(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    const GUID *pInterfaceGuid,
    wchar_t *strProfileName,
    wchar_t *strAllUserProfileSecurity,
    DWORD dwFlags,
    BOOL bOverWrite)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcIsUIRequestPending(
#else
DWORD _RpcIsUIRequestPending(
#endif
    wchar_t *arg_1,
    const GUID *pInterfaceGuid,
    struct_C *arg_3,
    LPDWORD arg_4)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetUIForwardingNetworkList(
#else
DWORD _RpcSetUIForwardingNetworkList(
#endif
    wchar_t *arg_1,
    GUID *arg_2,
    DWORD dwSize,
    GUID *arg_4)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcIsNetworkSuppressed(
#else
DWORD _RpcIsNetworkSuppressed(
#endif
    wchar_t *arg_1,
    DWORD arg_2,
    const GUID *pInterfaceGuid,
    LPDWORD arg_4)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcRemoveUIForwardingNetworkList(
#else
DWORD _RpcRemoveUIForwardingNetworkList(
#endif
    wchar_t *arg_1,
    const GUID *pInterfaceGuid)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcQueryExtUIRequest(
#else
DWORD _RpcQueryExtUIRequest(
#endif
    wchar_t *arg_1,
    GUID *arg_2,
    GUID *arg_3,
    short arg_4,
    GUID *pInterfaceGuid,
    struct_C **arg_6)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcUIResponse(
#else
DWORD _RpcUIResponse(
#endif
    wchar_t *arg_1,
    struct_C *arg_2,
    struct_D *arg_3)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetProfileKeyInfo(
#else
DWORD _RpcGetProfileKeyInfo(
#endif
    wchar_t *arg_1,
    DWORD arg_2,
    const GUID *pInterfaceGuid,
    wchar_t *arg_4,
    DWORD arg_5,
    LPDWORD arg_6,
    char *arg_7,
    LPDWORD arg_8)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcAsyncDoPlap(
#else
DWORD _RpcAsyncDoPlap(
#endif
    wchar_t *arg_1,
    const GUID *pInterfaceGuid,
    wchar_t *arg_3,
    DWORD dwSize,
    struct_E arg_5[])
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcQueryPlapCredentials(
#else
DWORD _RpcQueryPlapCredentials(
#endif
    wchar_t *arg_1,
    LPDWORD dwSize,
    struct_E **arg_3,
    wchar_t **arg_4,
    GUID *pInterfaceGuid,
    LPDWORD arg_6,
    LPDWORD arg_7,
    LPDWORD arg_8,
    LPDWORD arg_9)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcCancelPlap(
#else
DWORD _RpcCancelPlap(
#endif
    wchar_t *arg_1,
    const GUID *pInterfaceGuid)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcSetSecuritySettings(
#else
DWORD _RpcSetSecuritySettings(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    WLAN_SECURABLE_OBJECT SecurableObject,
    const wchar_t *strModifiedSDDL)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef __REACTOS__
DWORD RPC_ENTRY _RpcGetSecuritySettings(
#else
DWORD _RpcGetSecuritySettings(
#endif
    WLANSVC_RPC_HANDLE hClientHandle,
    WLAN_SECURABLE_OBJECT SecurableObject,
    WLAN_OPCODE_VALUE_TYPE *pValueType,
    wchar_t **pstrCurrentSDDL,
    LPDWORD pdwGrantedAccess)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


void __RPC_USER WLANSVC_RPC_HANDLE_rundown(WLANSVC_RPC_HANDLE hClientHandle)
{
}

