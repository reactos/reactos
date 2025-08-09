/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_NETIOAPI_H
#define __WINE_NETIOAPI_H

#include <ntddndis.h>

#ifndef IPHLPAPI_DLL_LINKAGE
#define IPHLPAPI_DLL_LINKAGE DECLSPEC_IMPORT
#endif

#ifndef ANY_SIZE
#define ANY_SIZE 1
#endif

typedef enum _MIB_IF_TABLE_LEVEL
{
    MibIfTableNormal,
    MibIfTableRaw,
    MibIfTableNormalWithoutStatistics,
} MIB_IF_TABLE_LEVEL, *PMIB_IF_TABLE_LEVEL;

typedef enum _MIB_NOTIFICATION_TYPE
{
    MibParameterNotification,
    MibAddInstance,
    MibDeleteInstance,
    MibInitialNotification,
} MIB_NOTIFICATION_TYPE, *PMIB_NOTIFICATION_TYPE;

typedef struct _MIB_IF_ROW2
{
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    GUID InterfaceGuid;
    WCHAR Alias[IF_MAX_STRING_SIZE + 1];
    WCHAR Description[IF_MAX_STRING_SIZE + 1];
    ULONG PhysicalAddressLength;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    UCHAR PermanentPhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG Mtu;
    IFTYPE Type;
    TUNNEL_TYPE TunnelType;
    NDIS_MEDIUM MediaType;
    NDIS_PHYSICAL_MEDIUM PhysicalMediumType;
    NET_IF_ACCESS_TYPE AccessType;
    NET_IF_DIRECTION_TYPE DirectionType;
    struct
    {
        BOOLEAN HardwareInterface : 1;
        BOOLEAN FilterInterface : 1;
        BOOLEAN ConnectorPresent : 1;
        BOOLEAN NotAuthenticated : 1;
        BOOLEAN NotMediaConnected : 1;
        BOOLEAN Paused : 1;
        BOOLEAN LowPower : 1;
        BOOLEAN EndPointInterface : 1;
    } InterfaceAndOperStatusFlags;
    IF_OPER_STATUS OperStatus;
    NET_IF_ADMIN_STATUS AdminStatus;
    NET_IF_MEDIA_CONNECT_STATE MediaConnectState;
    NET_IF_NETWORK_GUID NetworkGuid;
    NET_IF_CONNECTION_TYPE ConnectionType;
    ULONG64 TransmitLinkSpeed;
    ULONG64 ReceiveLinkSpeed;
    ULONG64 InOctets;
    ULONG64 InUcastPkts;
    ULONG64 InNUcastPkts;
    ULONG64 InDiscards;
    ULONG64 InErrors;
    ULONG64 InUnknownProtos;
    ULONG64 InUcastOctets;
    ULONG64 InMulticastOctets;
    ULONG64 InBroadcastOctets;
    ULONG64 OutOctets;
    ULONG64 OutUcastPkts;
    ULONG64 OutNUcastPkts;
    ULONG64 OutDiscards;
    ULONG64 OutErrors;
    ULONG64 OutUcastOctets;
    ULONG64 OutMulticastOctets;
    ULONG64 OutBroadcastOctets;
    ULONG64 OutQLen;
} MIB_IF_ROW2, *PMIB_IF_ROW2;

typedef struct _MIB_IF_TABLE2
{
    ULONG NumEntries;
    MIB_IF_ROW2 Table[1];
} MIB_IF_TABLE2, *PMIB_IF_TABLE2;

typedef struct _MIB_IPINTERFACE_ROW
{
    ADDRESS_FAMILY Family;
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    ULONG MaxReassemblySize;
    ULONG64 InterfaceIdentifier;
    ULONG MinRouterAdvertisementInterval;
    ULONG MaxRouterAdvertisementInterval;
    BOOLEAN AdvertisingEnabled;
    BOOLEAN ForwardingEnabled;
    BOOLEAN WeakHostSend;
    BOOLEAN WeakHostReceive;
    BOOLEAN UseAutomaticMetric;
    BOOLEAN UseNeighborUnreachabilityDetection;
    BOOLEAN ManagedAddressConfigurationSupported;
    BOOLEAN OtherStatefulConfigurationSupported;
    BOOLEAN AdvertiseDefaultRoute;
    NL_ROUTER_DISCOVERY_BEHAVIOR RouterDiscoveryBehavior;
    ULONG DadTransmits;
    ULONG BaseReachableTime;
    ULONG RetransmitTime;
    ULONG PathMtuDiscoveryTimeout;
    NL_LINK_LOCAL_ADDRESS_BEHAVIOR LinkLocalAddressBehavior;
    ULONG LinkLocalAddressTimeout;
    ULONG ZoneIndices[ScopeLevelCount];
    ULONG SitePrefixLength;
    ULONG Metric;
    ULONG NlMtu;
    BOOLEAN Connected;
    BOOLEAN SupportsWakeUpPatterns;
    BOOLEAN SupportsNeighborDiscovery;
    BOOLEAN SupportsRouterDiscovery;
    ULONG ReachableTime;
    NL_INTERFACE_OFFLOAD_ROD TransmitOffload;
    NL_INTERFACE_OFFLOAD_ROD ReceiveOffload;
    BOOLEAN DisableDefaultRoutes;
} MIB_IPINTERFACE_ROW, *PMIB_IPINTERFACE_ROW;

typedef struct _MIB_IPINTERFACE_TABLE
{
    ULONG NumEntries;
    MIB_IPINTERFACE_ROW Table[ANY_SIZE];
} MIB_IPINTERFACE_TABLE, *PMIB_IPINTERFACE_TABLE;

typedef struct _MIB_UNICASTIPADDRESS_ROW
{
    SOCKADDR_INET       Address;
    NET_LUID            InterfaceLuid;
    NET_IFINDEX         InterfaceIndex;
    NL_PREFIX_ORIGIN    PrefixOrigin;
    NL_SUFFIX_ORIGIN    SuffixOrigin;
    ULONG               ValidLifetime;
    ULONG               PreferredLifetime;
    UINT8               OnLinkPrefixLength;
    BOOLEAN             SkipAsSource;
    NL_DAD_STATE        DadState;
    SCOPE_ID            ScopeId;
    LARGE_INTEGER       CreationTimeStamp;
} MIB_UNICASTIPADDRESS_ROW, *PMIB_UNICASTIPADDRESS_ROW;

typedef struct _MIB_UNICASTIPADDRESS_TABLE
{
    ULONG NumEntries;
    MIB_UNICASTIPADDRESS_ROW Table[1];
} MIB_UNICASTIPADDRESS_TABLE, *PMIB_UNICASTIPADDRESS_TABLE;

typedef struct _MIB_ANYCASTIPADDRESS_ROW
{
    SOCKADDR_INET Address;
    NET_LUID      InterfaceLuid;
    NET_IFINDEX   InterfaceIndex;
    SCOPE_ID      ScopeId;
} MIB_ANYCASTIPADDRESS_ROW, *PMIB_ANYCASTIPADDRESS_ROW;

typedef struct _MIB_ANYCASTIPADDRESS_TABLE
{
    ULONG                    NumEntries;
    MIB_ANYCASTIPADDRESS_ROW Table[ANY_SIZE];
} MIB_ANYCASTIPADDRESS_TABLE, *PMIB_ANYCASTIPADDRESS_TABLE;

typedef struct _IP_ADDRESS_PREFIX
{
    SOCKADDR_INET Prefix;
    UINT8         PrefixLength;
} IP_ADDRESS_PREFIX, *PIP_ADDRESS_PREFIX;

typedef struct _MIB_IPFORWARD_ROW2
{
    NET_LUID          InterfaceLuid;
    NET_IFINDEX       InterfaceIndex;
    IP_ADDRESS_PREFIX DestinationPrefix;
    SOCKADDR_INET     NextHop;
    UCHAR             SitePrefixLength;
    ULONG             ValidLifetime;
    ULONG             PreferredLifetime;
    ULONG             Metric;
    NL_ROUTE_PROTOCOL Protocol;
    BOOLEAN           Loopback;
    BOOLEAN           AutoconfigureAddress;
    BOOLEAN           Publish;
    BOOLEAN           Immortal;
    ULONG             Age;
    NL_ROUTE_ORIGIN   Origin;
} MIB_IPFORWARD_ROW2, *PMIB_IPFORWARD_ROW2;

typedef struct _MIB_IPFORWARD_TABLE2
{
    ULONG              NumEntries;
    MIB_IPFORWARD_ROW2 Table[ANY_SIZE];
} MIB_IPFORWARD_TABLE2, *PMIB_IPFORWARD_TABLE2;

typedef struct _MIB_IPNET_ROW2
{
    SOCKADDR_INET Address;
    NET_IFINDEX InterfaceIndex;
    NET_LUID InterfaceLuid;
    UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    NL_NEIGHBOR_STATE State;

    union
    {
        struct
        {
            BOOLEAN IsRouter : 1;
            BOOLEAN IsUnreachable : 1;
        } DUMMYSTRUCTNAME;
        UCHAR Flags;
    } DUMMYUNIONNAME;

    union
    {
        ULONG LastReachable;
        ULONG LastUnreachable;
    } ReachabilityTime;
} MIB_IPNET_ROW2, *PMIB_IPNET_ROW2;

typedef struct _MIB_IPNET_TABLE2
{
    ULONG NumEntries;
    MIB_IPNET_ROW2 Table[ANY_SIZE];
} MIB_IPNET_TABLE2, *PMIB_IPNET_TABLE2;

typedef VOID (WINAPI *PIPINTERFACE_CHANGE_CALLBACK)(PVOID, PMIB_IPINTERFACE_ROW,
                                                    MIB_NOTIFICATION_TYPE);
typedef VOID (WINAPI *PUNICAST_IPADDRESS_CHANGE_CALLBACK)(PVOID, PMIB_UNICASTIPADDRESS_ROW,
                                                          MIB_NOTIFICATION_TYPE);

typedef VOID (WINAPI *PIPFORWARD_CHANGE_CALLBACK)(VOID*,MIB_IPFORWARD_ROW2*,MIB_NOTIFICATION_TYPE);

IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceAliasToLuid(const WCHAR*,NET_LUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceGuidToLuid(const GUID*,NET_LUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceIndexToLuid(NET_IFINDEX,NET_LUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceLuidToAlias(const NET_LUID*,WCHAR*,SIZE_T);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceLuidToGuid(const NET_LUID*,GUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceLuidToIndex(const NET_LUID*,NET_IFINDEX*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceLuidToNameA(const NET_LUID*,char*,SIZE_T);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceLuidToNameW(const NET_LUID*,WCHAR*,SIZE_T);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceNameToLuidA(const char*,NET_LUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertInterfaceNameToLuidW(const WCHAR*,NET_LUID*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI ConvertLengthToIpv4Mask(ULONG,ULONG*);
IPHLPAPI_DLL_LINKAGE void WINAPI FreeMibTable(void*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetAnycastIpAddressTable(ADDRESS_FAMILY,MIB_ANYCASTIPADDRESS_TABLE**);
IPHLPAPI_DLL_LINKAGE NET_IF_COMPARTMENT_ID WINAPI GetCurrentThreadCompartmentId(void);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIfEntry2(MIB_IF_ROW2*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIfEntry2Ex(MIB_IF_TABLE_LEVEL,MIB_IF_ROW2*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIfTable2(MIB_IF_TABLE2**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIfTable2Ex(MIB_IF_TABLE_LEVEL,MIB_IF_TABLE2**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIpForwardEntry2(MIB_IPFORWARD_ROW2*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIpForwardTable2(ADDRESS_FAMILY,MIB_IPFORWARD_TABLE2**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIpInterfaceTable(ADDRESS_FAMILY,MIB_IPINTERFACE_TABLE**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIpNetEntry2(MIB_IPNET_ROW2*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetIpNetTable2(ADDRESS_FAMILY,MIB_IPNET_TABLE2**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetUnicastIpAddressEntry(MIB_UNICASTIPADDRESS_ROW*);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI GetUnicastIpAddressTable(ADDRESS_FAMILY,MIB_UNICASTIPADDRESS_TABLE**);
IPHLPAPI_DLL_LINKAGE DWORD WINAPI SetCurrentThreadCompartmentId(NET_IF_COMPARTMENT_ID);
IPHLPAPI_DLL_LINKAGE PCHAR WINAPI if_indextoname(NET_IFINDEX,PCHAR);
IPHLPAPI_DLL_LINKAGE NET_IFINDEX WINAPI if_nametoindex(PCSTR);

#endif /* __WINE_NETIOAPI_H */
