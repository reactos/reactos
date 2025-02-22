/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        dll/win32/msafd/include/helpers.h
 * PURPOSE:     Definitions for helper DLL management
 */
#ifndef __HELPERS_H
#define __HELPERS_H

//#include <msafd.h>

typedef struct _HELPER_DATA {
    LIST_ENTRY                      Helpers;
    LONG                            RefCount;
    HANDLE                          hInstance;
    INT                             MinWSAddressLength;
    INT                             MaxWSAddressLength;
    INT                             MinTDIAddressLength;
    INT                             MaxTDIAddressLength;
    BOOLEAN                         UseDelayedAcceptance;
    PWINSOCK_MAPPING                Mapping;
    PWSH_OPEN_SOCKET                WSHOpenSocket;
    PWSH_OPEN_SOCKET2               WSHOpenSocket2;
    PWSH_JOIN_LEAF                  WSHJoinLeaf;
    PWSH_NOTIFY                     WSHNotify;
    PWSH_GET_SOCKET_INFORMATION     WSHGetSocketInformation;
    PWSH_SET_SOCKET_INFORMATION     WSHSetSocketInformation;
    PWSH_GET_SOCKADDR_TYPE          WSHGetSockaddrType;
    PWSH_GET_WILDCARD_SOCKADDR      WSHGetWildcardSockaddr;
    PWSH_GET_BROADCAST_SOCKADDR     WSHGetBroadcastSockaddr;
    PWSH_ADDRESS_TO_STRING          WSHAddressToString;
    PWSH_STRING_TO_ADDRESS          WSHStringToAddress;
    PWSH_IOCTL                      WSHIoctl;
    WCHAR                           TransportName[1];
} HELPER_DATA, *PHELPER_DATA;

int SockLoadHelperDll(
    PWSTR TransportName,
    PWINSOCK_MAPPING Mapping,
    PHELPER_DATA *HelperDllData
);

int SockLoadTransportMapping(
    PWSTR TransportName,
    PWINSOCK_MAPPING *Mapping
);

int SockLoadTransportList(
    PWSTR *TransportList
);

BOOL SockIsTripleInMapping(
    PWINSOCK_MAPPING Mapping,
    INT AddressFamily,
    INT SocketType,
    INT Protocol
);

int SockGetTdiName(
    PINT AddressFamily,
    PINT SocketType,
    PINT Protocol,
    GROUP Group,
    DWORD Flags,
    PUNICODE_STRING TransportName,
    PVOID *HelperDllContext,
    PHELPER_DATA *HelperDllData,
    PDWORD Events
);

#endif /* __HELPERS_H */

/* EOF */
