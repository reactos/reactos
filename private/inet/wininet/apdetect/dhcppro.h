/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    lproto.h

Abstract:

    This file contains function proto types for the NT specific
    functions.

Author:

    Madan Appiah (madana)  Dec-7-1993

Environment:

    User Mode - Win32

Revision History:


--*/

//
// dhcpreg.c
//

DWORD
DhcpRegQueryInfoKey(
    HKEY KeyHandle,
    LPDHCP_KEY_QUERY_INFO QueryInfo
    );

DWORD
GetRegistryString(
    HKEY Key,
    LPVOID ValueStringName,
    LPTSTR *String,
    LPDWORD StringSize
    );

DWORD
DhcpRegReadParamString(
    LPWSTR     AdapterName,
    LPWSTR     RegKeyLocation,
    LPWSTR     ValueName,
    LPWSTR    *ReturnValue
);

DWORD
RegSetIpAddress(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    DHCP_IP_ADDRESS IpAddress
    );

#if DBG
DWORD
RegSetTimeField(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    time_t Time
    );
#endif

DWORD
DhcpGetRegistryValue(
    LPWSTR RegKey,
    LPWSTR ValueName,
    DWORD ValueType,
    PVOID *Data
    );

DWORD
DhcpSetDNSAddress(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    DHCP_IP_ADDRESS UNALIGNED *Data,
    DWORD DataLength
    );

DWORD
SetDhcpOption(
    LPWSTR AdapterName,
    DHCP_OPTION_ID OptionId,
    LPBOOL DefaultGatewaysSet,
    BOOL LastKnownDefaultGateway
    );

DWORD
DhcpMakeNICList(
    VOID
    );

DWORD
DhcpAddNICtoList(
    LPWSTR AdapterName,
    LPWSTR DeviceName,
    PDHCP_CONTEXT *DhcpContext
    );

#if     defined(_PNP_POWER_)
DWORD
DhcpAddNICtoListEx(
    LPWSTR AdapterName,
    DWORD  ipInterfaceContext,
    PDHCP_CONTEXT *DhcpContext
    );

#endif _PNP_POWER_
BOOL
SetOverRideDefaultGateway(
    LPWSTR AdapterName
    );

BOOL
DhcpGetAddressOption(
    DHCP_IP_ADDRESS **ppDNSServerList,
    DWORD            *pNumberOfServers
    );


BOOL
DhcpRegReadUseMHAsyncDnsFlag(
    VOID
);

DWORD                                             // Win32 status
DhcpInitRegistry(                                 // Initialize registry based globals
    VOID
);

VOID
DhcpCleanupRegistry(                              // undo the effects of InitReg call
    VOID
);


DHCP_IP_ADDRESS                                   // the static ip address of the adapter
DhcpRegReadIpAddress(                             // get the first ip address
    LPWSTR    AdapterName,                        // the adaptor of interest
    LPWSTR    ValueName                           // the ip address value to read
);

DWORD                                             // status
DhcpRegReadIpAddresses(                           // read a set of ip addresses
    IN      DHCPKEY                RegKeyHandle,  // open key handle
    IN      LPWSTR                 ValueName,     // name of value to read frm
    IN      WCHAR                  Separation,    // a MULTI_SZ has L'\0', SZ has L' ' or L',' etc.
    OUT     PDHCP_IP_ADDRESS      *AddressArray,  // an array of addresses
    OUT     LPDWORD                AddressCount   // the output size of above array
);

VOID
DhcpRegInitializeClasses(                         // initialize the classes list
    IN OUT  PDHCP_CONTEXT          DhcpContext    // NULL or adpater context
);

DWORD                                             // status
DhcpGetRegistryValueWithKey(                      // see defn of GetRegistryValue
    IN      HKEY                   KeyHandle,     // keyhandle NOT location
    IN      LPTSTR                 ValueName,     // value to read from registry
    IN      DWORD                  ValueType,     // type of value
    OUT     LPVOID                 Data           // this will be filled in
);

DWORD                                             // status
DhcpRegExpandString(                              // replace '?' with AdapterName
    IN      LPWSTR                 InString,      // input string to expand
    IN      LPWSTR                 AdapterName,   // the adapter name
    OUT     LPWSTR                *OutString,     // the output ptr to store string
    IN OUT  LPWSTR                 Buffer         // the buffer to use if non NULL
);

DWORD                                             // status
DhcpRegReadFromLocation(                          // read from one location
    IN      LPWSTR                 OneLocation,   // value to read from
    IN      LPWSTR                 AdapterName,   // replace '?' with adapternames
    OUT     LPBYTE                *Value,         // output value
    OUT     DWORD                 *ValueType,     // data type of value
    OUT     DWORD                 *ValueSize      // the size in bytes
);

DWORD                                             // status
DhcpRegReadFromAnyLocation(                       // read from one of many locations
    IN      LPWSTR                 MzRegLocation, // multiple locations thru REG_MULTI_MZ
    IN      LPWSTR                 AdapterName,   // may have to replace '?' with AdapterName
    OUT     LPBYTE                *Value,         // data for the value read
    OUT     DWORD                 *ValueType,     // type of the data
    OUT     DWORD                 *ValueSize      // the size of data
);

DWORD                                             // win32 status
DhcpRegFillParams(                                // get the registry config for this adapter
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // adapter context to fill in
    IN      BOOL                   ReadAllInfo    // read EVERYTHING or only some critical info?
);

VOID
DhcpRegReadClassId(                               // Read the class id stuff
    IN      PDHCP_CONTEXT          DhcpContext    // Input context to read for
);

//
// ioctl.c
//

DWORD
IPSetIPAddress(
    DWORD IpInterfaceContext,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask
    );

DWORD
IPAddIPAddress(
    LPWSTR AdapterName,
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS SubnetMask
    );

DWORD
IPResetIPAddress(
    DWORD           dwInterfaceContext,
    DHCP_IP_ADDRESS SubnetMask
    );


DWORD
SetIPAddressAndArp(
    PVOID         pvLocalInformation,
    DWORD         dwAddress,
    DWORD         dwSubnetMask
    );


DWORD
NetBTSetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask
    );

DWORD
NetBTResetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS SubnetMask
    );

DWORD
NetBTNotifyRegChanges(
    LPWSTR DeviceName
    );

DWORD
SetDefaultGateway(
    DWORD Command,
    DHCP_IP_ADDRESS GatewayAddress
    );

HANDLE
DhcpOpenGlobalEvent(
    void
    );

#if     defined(_PNP_POWER_) && !defined(VXD)
DWORD
IPGetIPEventRequest(
    HANDLE  handle,
    HANDLE  event,
    UINT    seqNo,
    PIP_GET_IP_EVENT_RESPONSE  responseBuffer,
    DWORD                responseBufferSize,
    PIO_STATUS_BLOCK     ioStatusBlock
    );

DWORD
IPCancelIPEventRequest(
    HANDLE  handle,
    PIO_STATUS_BLOCK     ioStatusBlock
    );

#endif _PNP_POWER_ && !VXD

//
// api.c
//

DWORD
DhcpApiInit(
    VOID
    );



VOID
DhcpApiCleanup(
    VOID
    );

DWORD
ProcessApiRequest(
    HANDLE PipeHandle,
    LPOVERLAPPED Overlap
    );

//
// util.c
//


PDHCP_CONTEXT
FindDhcpContextOnRenewalList(
    LPWSTR AdapterName,
    DWORD  InterfaceContext
    );

PDHCP_CONTEXT
FindDhcpContextOnNicList(
    LPWSTR AdapterName,
    DWORD  InterfaceContext
    );

BOOL
IsMultiHomeMachine(
    VOID
    );


//
// options.c
//


SERVICE_SPECIFIC_DHCP_OPTION *
FindDhcpOption(
    DHCP_OPTION_ID OptionID
    );


DWORD
ReadGlobalOptionList();

DWORD
LoseAllEnvSpecificOptions(
    PDHCP_CONTEXT   dhcpContext
);


//
// dhcp.c
//

DWORD
SetIpConfigurationForNIC(
    HKEY            KeyHandle,
    PDHCP_CONTEXT   DhcpContext,
    PDHCP_OPTIONS   DhcpOptions,
    DHCP_IP_ADDRESS ServerIpAddress,
    DWORD           dwLeaseTime,
    DWORD           dwT1Time,
    DWORD           dwT2Time,
    BOOL            ObtainedNewAddress
    );

