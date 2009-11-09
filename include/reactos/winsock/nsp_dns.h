/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Headers
 * FILE:        include/winsock/nsp_dns.h
 * PURPOSE:     WinSock 2 Shared NSP Header
 */


#ifndef __NSP_DNS_H
#define __NSP_DNS_H

/* Includes */
#include <svcguid.h>

/* Globals */
GUID HostnameGuid = SVCID_HOSTNAME;
GUID AddressGuid = SVCID_INET_HOSTADDRBYINETSTRING;
GUID HostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID IANAGuid = SVCID_INET_SERVICEBYNAME;
GUID InetHostName = SVCID_INET_HOSTADDRBYNAME;
GUID Ipv6Guid = SVCID_DNS_TYPE_AAAA;

/* Macros and Defines */
#define RNR_BUFFER_SIZE 512

#endif

