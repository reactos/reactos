/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/helpers.h
 * PURPOSE:     Definitions for helper DLL management
 */
#ifndef __HELPERS_H
#define __HELPERS_H

#include <msafd.h>

typedef struct _WSHELPER_DLL_ENTRIES {
    PWSH_ADDRESS_TO_STRING      lpWSHAddressToString;
    PWSH_ENUM_PROTOCOLS         lpWSHEnumProtocols;
    PWSH_GET_BROADCAST_SOCKADDR lpWSHGetBroadcastSockaddr;
    PWSH_GET_PROVIDER_GUID      lpWSHGetProviderGuid;
    PWSH_GET_SOCKADDR_TYPE      lpWSHGetSockaddrType;
    PWSH_GET_SOCKET_INFORMATION lpWSHGetSocketInformation;
    PWSH_GET_WILDCARD_SOCKEADDR lpWSHGetWildcardSockaddr;
    PWSH_GET_WINSOCK_MAPPING    lpWSHGetWinsockMapping;
    PWSH_GET_WSAPROTOCOL_INFO   lpWSHGetWSAProtocolInfo;
    PWSH_IOCTL                  lpWSHIoctl;
    PWSH_JOIN_LEAF              lpWSHJoinLeaf;
    PWSH_NOTIFY                 lpWSHNotify;
    PWSH_OPEN_SOCKET            lpWSHOpenSocket;
    PWSH_OPEN_SOCKET2           lpWSHOpenSocket2;
    PWSH_SET_SOCKET_INFORMATION lpWSHSetSocketInformation;
    PWSH_STRING_TO_ADDRESS      lpWSHStringToAddress;
} WSHELPER_DLL_ENTRIES, *PWSHELPER_DLL_ENTRIES;

typedef struct _WSHELPER_DLL {
    LIST_ENTRY ListEntry;
    CRITICAL_SECTION Lock;
    WCHAR LibraryName[MAX_PATH];
    HMODULE hModule;
    WSHELPER_DLL_ENTRIES EntryTable;
    PWINSOCK_MAPPING Mapping;
} WSHELPER_DLL, *PWSHELPER_DLL;


PWSHELPER_DLL CreateHelperDLL(
    LPWSTR LibraryName);

INT DestroyHelperDLL(
    PWSHELPER_DLL HelperDLL);

PWSHELPER_DLL LocateHelperDLL(
    LPWSAPROTOCOL_INFOW lpProtocolInfo);

INT LoadHelperDLL(
    PWSHELPER_DLL HelperDLL);

INT UnloadHelperDLL(
    PWSHELPER_DLL HelperDLL);

VOID CreateHelperDLLDatabase(VOID);

VOID DestroyHelperDLLDatabase(VOID);

#endif /* __HELPERS_H */

/* EOF */
