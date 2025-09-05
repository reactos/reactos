/* WINE iptypes.h
 * Copyright (C) 2003 Juan Lang
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

#ifndef WINE_IPTYPES_H_
#define WINE_IPTYPES_H_

#include <time.h>
#include <ifdef.h>
#include <nldef.h>

#define MAX_ADAPTER_DESCRIPTION_LENGTH  128
#define MAX_ADAPTER_NAME_LENGTH         256
#define MAX_ADAPTER_ADDRESS_LENGTH      8
#define MAX_HOSTNAME_LEN                128
#define MAX_DOMAIN_NAME_LEN             128
#define MAX_SCOPE_ID_LEN                256
#define MAX_DHCPV6_DUID_LENGTH          130
#define MAX_DNS_SUFFIX_STRING_LENGTH    256

#define BROADCAST_NODETYPE              1
#define PEER_TO_PEER_NODETYPE           2
#define MIXED_NODETYPE                  4
#define HYBRID_NODETYPE                 8

typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

typedef struct _IP_ADDR_STRING {
    struct _IP_ADDR_STRING* Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;

typedef struct _IP_ADAPTER_INFO {
    struct _IP_ADAPTER_INFO* Next;
    DWORD ComboIndex;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
} IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;

typedef struct _IP_PER_ADAPTER_INFO {
    UINT AutoconfigEnabled;
    UINT AutoconfigActive;
    PIP_ADDR_STRING CurrentDnsServer;
    IP_ADDR_STRING DnsServerList;
} IP_PER_ADAPTER_INFO, *PIP_PER_ADAPTER_INFO;

typedef struct {
    char HostName[MAX_HOSTNAME_LEN + 4] ;
    char DomainName[MAX_DOMAIN_NAME_LEN + 4];
    PIP_ADDR_STRING CurrentDnsServer;
    IP_ADDR_STRING DnsServerList;
    UINT NodeType;
    char ScopeId[MAX_SCOPE_ID_LEN + 4];
    UINT EnableRouting;
    UINT EnableProxy;
    UINT EnableDns;
} FIXED_INFO, *PFIXED_INFO;

typedef NL_PREFIX_ORIGIN IP_PREFIX_ORIGIN;
typedef NL_SUFFIX_ORIGIN IP_SUFFIX_ORIGIN;
typedef NL_DAD_STATE IP_DAD_STATE;

#ifdef _WINSOCK2API_

#define IP_ADAPTER_ADDRESS_DNS_ELIGIBLE 0x00000001
#define IP_ADAPTER_ADDRESS_TRANSIENT    0x00000002

typedef struct _IP_ADAPTER_UNICAST_ADDRESS_LH {
    union {
        struct {
            ULONG Length;
            DWORD Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_UNICAST_ADDRESS_LH   *Next;
    SOCKET_ADDRESS                          Address;
    IP_PREFIX_ORIGIN                        PrefixOrigin;
    IP_SUFFIX_ORIGIN                        SuffixOrigin;
    IP_DAD_STATE                            DadState;
    ULONG                                   ValidLifetime;
    ULONG                                   PreferredLifetime;
    ULONG                                   LeaseLifetime;
    UINT8                                   OnLinkPrefixLength;
} IP_ADAPTER_UNICAST_ADDRESS_LH, *PIP_ADAPTER_UNICAST_ADDRESS_LH;

typedef struct _IP_ADAPTER_UNICAST_ADDRESS_XP {
    union {
        struct {
            ULONG Length;
            DWORD Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_UNICAST_ADDRESS_XP   *Next;
    SOCKET_ADDRESS                          Address;
    IP_PREFIX_ORIGIN                        PrefixOrigin;
    IP_SUFFIX_ORIGIN                        SuffixOrigin;
    IP_DAD_STATE                            DadState;
    ULONG                                   ValidLifetime;
    ULONG                                   PreferredLifetime;
    ULONG                                   LeaseLifetime;
} IP_ADAPTER_UNICAST_ADDRESS_XP, *PIP_ADAPTER_UNICAST_ADDRESS_XP;

typedef IP_ADAPTER_UNICAST_ADDRESS_LH IP_ADAPTER_UNICAST_ADDRESS;
typedef IP_ADAPTER_UNICAST_ADDRESS_LH *PIP_ADAPTER_UNICAST_ADDRESS;

typedef struct _IP_ADAPTER_ANYCAST_ADDRESS {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_ANYCAST_ADDRESS *Next;
    SOCKET_ADDRESS                      Address;
} IP_ADAPTER_ANYCAST_ADDRESS, *PIP_ADAPTER_ANYCAST_ADDRESS;

typedef struct _IP_ADAPTER_MULTICAST_ADDRESS {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_MULTICAST_ADDRESS *Next;
    SOCKET_ADDRESS                       Address;
} IP_ADAPTER_MULTICAST_ADDRESS, *PIP_ADAPTER_MULTICAST_ADDRESS;

typedef struct _IP_ADAPTER_DNS_SERVER_ADDRESS {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Reserved;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_DNS_SERVER_ADDRESS *Next;
    SOCKET_ADDRESS                         Address;
} IP_ADAPTER_DNS_SERVER_ADDRESS, *PIP_ADAPTER_DNS_SERVER_ADDRESS;

typedef struct _IP_ADAPTER_PREFIX {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_PREFIX *Next;
    SOCKET_ADDRESS             Address;
    ULONG                      PrefixLength;
} IP_ADAPTER_PREFIX, *PIP_ADAPTER_PREFIX;

typedef struct _IP_ADAPTER_WINS_SERVER_ADDRESS_LH {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Reserved;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_WINS_SERVER_ADDRESS_LH *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_WINS_SERVER_ADDRESS_LH, *PIP_ADAPTER_WINS_SERVER_ADDRESS_LH;
typedef IP_ADAPTER_WINS_SERVER_ADDRESS_LH IP_ADAPTER_WINS_SERVER_ADDRESS;
typedef IP_ADAPTER_WINS_SERVER_ADDRESS_LH *PIP_ADAPTER_WINS_SERVER_ADDRESS;

typedef struct _IP_ADAPTER_GATEWAY_ADDRESS_LH {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD Reserved;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_GATEWAY_ADDRESS_LH *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_GATEWAY_ADDRESS_LH, *PIP_ADAPTER_GATEWAY_ADDRESS_LH;
typedef IP_ADAPTER_GATEWAY_ADDRESS_LH IP_ADAPTER_GATEWAY_ADDRESS;
typedef IP_ADAPTER_GATEWAY_ADDRESS_LH *PIP_ADAPTER_GATEWAY_ADDRESS;

typedef struct _IP_ADAPTER_DNS_SUFFIX {
    struct _IP_ADAPTER_DNS_SUFFIX *Next;
    WCHAR String[MAX_DNS_SUFFIX_STRING_LENGTH];
} IP_ADAPTER_DNS_SUFFIX, *PIP_ADAPTER_DNS_SUFFIX;

#define IP_ADAPTER_DDNS_ENABLED               0x1
#define IP_ADAPTER_REGISTER_ADAPTER_SUFFIX    0x2
#define IP_ADAPTER_DHCP_ENABLED               0x4
#define IP_ADAPTER_RECEIVE_ONLY               0x8
#define IP_ADAPTER_NO_MULTICAST               0x10
#define IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG 0x20
#define IP_ADAPTER_NETBIOS_OVER_TCPIP_ENABLED 0x40
#define IP_ADAPTER_IPV4_ENABLED               0x80
#define IP_ADAPTER_IPV6_ENABLED               0x100
#define IP_ADAPTER_IPV6_MANAGE_ADDRESS_CONFIG 0x200

typedef struct _IP_ADAPTER_ADDRESSES_LH {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD IfIndex;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_ADDRESSES_LH *Next;
    PCHAR                           AdapterName;
    PIP_ADAPTER_UNICAST_ADDRESS     FirstUnicastAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS     FirstAnycastAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS   FirstMulticastAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS  FirstDnsServerAddress;
    PWCHAR                          DnsSuffix;
    PWCHAR                          Description;
    PWCHAR                          FriendlyName;
    BYTE                            PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD                           PhysicalAddressLength;
    union {
        DWORD                       Flags;
        struct {
            DWORD                   DdnsEnabled : 1;
            DWORD                   RegisterAdapterSuffix : 1;
            DWORD                   Dhcpv4Enabled : 1;
            DWORD                   ReceiveOnly : 1;
            DWORD                   NoMulticast : 1;
            DWORD                   Ipv6OtherStatefulConfig : 1;
            DWORD                   NetbiosOverTcpipEnabled : 1;
            DWORD                   Ipv4Enabled : 1;
            DWORD                   Ipv6Enabled : 1;
            DWORD                   Ipv6ManagedAddressConfigurationSupported : 1;
        } DUMMYSTRUCTNAME1;
    } DUMMYUNIONNAME1;
    DWORD                           Mtu;
    DWORD                           IfType;
    IF_OPER_STATUS                  OperStatus;
    DWORD                           Ipv6IfIndex;
    DWORD                           ZoneIndices[16];
    PIP_ADAPTER_PREFIX              FirstPrefix;
    ULONG64                         TransmitLinkSpeed;
    ULONG64                         ReceiveLinkSpeed;
    PIP_ADAPTER_WINS_SERVER_ADDRESS_LH FirstWinsServerAddress;
    PIP_ADAPTER_GATEWAY_ADDRESS_LH  FirstGatewayAddress;
    ULONG                           Ipv4Metric;
    ULONG                           Ipv6Metric;
    IF_LUID                         Luid;
    SOCKET_ADDRESS                  Dhcpv4Server;
    NET_IF_COMPARTMENT_ID           CompartmentId;
    NET_IF_NETWORK_GUID             NetworkGuid;
    NET_IF_CONNECTION_TYPE          ConnectionType;
    TUNNEL_TYPE                     TunnelType;
    SOCKET_ADDRESS                  Dhcpv6Server;
    BYTE                            Dhcpv6ClientDuid[MAX_DHCPV6_DUID_LENGTH];
    ULONG                           Dhcpv6ClientDuidLength;
    ULONG                           Dhcpv6Iaid;
    PIP_ADAPTER_DNS_SUFFIX          FirstDnsSuffix;
} IP_ADAPTER_ADDRESSES_LH, *PIP_ADAPTER_ADDRESSES_LH;

typedef struct _IP_ADAPTER_ADDRESSES_XP {
    union {
        ULONGLONG Alignment;
        struct {
            ULONG Length;
            DWORD IfIndex;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    struct _IP_ADAPTER_ADDRESSES_XP *Next;
    PCHAR                           AdapterName;
    PIP_ADAPTER_UNICAST_ADDRESS     FirstUnicastAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS     FirstAnycastAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS   FirstMulticastAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS  FirstDnsServerAddress;
    PWCHAR                          DnsSuffix;
    PWCHAR                          Description;
    PWCHAR                          FriendlyName;
    BYTE                            PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD                           PhysicalAddressLength;
    DWORD                           Flags;
    DWORD                           Mtu;
    DWORD                           IfType;
    IF_OPER_STATUS                  OperStatus;
    DWORD                           Ipv6IfIndex;
    DWORD                           ZoneIndices[16];
    PIP_ADAPTER_PREFIX              FirstPrefix;
} IP_ADAPTER_ADDRESSES_XP, *PIP_ADAPTER_ADDRESSES_XP;

typedef IP_ADAPTER_ADDRESSES_LH IP_ADAPTER_ADDRESSES;
typedef IP_ADAPTER_ADDRESSES_LH *PIP_ADAPTER_ADDRESSES;

#define GAA_FLAG_SKIP_UNICAST                0x00000001
#define GAA_FLAG_SKIP_ANYCAST                0x00000002
#define GAA_FLAG_SKIP_MULTICAST              0x00000004
#define GAA_FLAG_SKIP_DNS_SERVER             0x00000008
#define GAA_FLAG_INCLUDE_PREFIX              0x00000010
#define GAA_FLAG_SKIP_FRIENDLY_NAME          0x00000020
#define GAA_FLAG_INCLUDE_WINS_INFO           0x00000040
#define GAA_FLAG_INCLUDE_GATEWAYS            0x00000080
#define GAA_FLAG_INCLUDE_ALL_INTERFACES      0x00000100
#define GAA_FLAG_INCLUDE_ALL_COMPARTMENTS    0x00000200
#define GAA_FLAG_INCLUDE_TUNNEL_BINDINGORDER 0x00000400

#endif /* _WINSOCK2API_ */

#endif /* WINE_IPTYPES_H_*/
