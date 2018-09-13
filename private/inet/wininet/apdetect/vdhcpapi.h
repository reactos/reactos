/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    vdhcpapi.h

Abstract:

    Worker functions for VDHCP APIs.

Author:

    Madan Appiah (madana)  28-May-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef VDHCPAPI_H_
#define VDHCPAPI_H_

#define DHCP_QUERY_INFO             1
#define DHCP_RENEW_IPADDRESS        2
#define DHCP_RELEASE_IPADDRESS      3

typedef struct _DHCP_NIC_INFO {
    DWORD OffsetHardwareAddress;
    DWORD HardwareLength;
    DWORD IpAddress;
    DWORD Lease;
    DWORD LeaseObtainedTime;
    DWORD LeaseExpiresTime;
    DWORD DhcpServerAddress;
    DWORD DNSServersLen;
    DWORD OffsetDNSServers;
    DWORD DomainNameLen;
    DWORD OffsetDomainName;
} DHCP_NIC_INFO, *LPDHCP_NIC_INFO;

typedef struct _DHCP_HW_INFO {
    DWORD OffsetHardwareAddress;
    DWORD HardwareLength;
} DHCP_HW_INFO, *LPDHCP_HW_INFO;

typedef struct _DHCP_QUERYINFO {
    DWORD NumNICs;
    DHCP_NIC_INFO NicInfo[1];
} DHCP_QUERYINFO, *LPDHCP_QUERYINFO;

#endif