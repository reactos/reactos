/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Headers
 * FILE:        include/reactos/winsock/nsp_dns.h
 * PURPOSE:     WinSock 2 Shared NSP Header
 */


#ifndef __NSP_DNS_H
#define __NSP_DNS_H

/* Includes */
#include <svcguid.h>
#include <guiddef.h>

/* Globals */
const GUID DECLSPEC_SELECTANY HostnameGuid = SVCID_HOSTNAME;
const GUID DECLSPEC_SELECTANY AddressGuid = SVCID_INET_HOSTADDRBYINETSTRING;
const GUID DECLSPEC_SELECTANY HostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
const GUID DECLSPEC_SELECTANY IANAGuid = SVCID_INET_SERVICEBYNAME;
const GUID DECLSPEC_SELECTANY InetHostName = SVCID_INET_HOSTADDRBYNAME;
const GUID DECLSPEC_SELECTANY Ipv6Guid = SVCID_DNS_TYPE_AAAA;

/* Macros and Defines */
#define RNR_BUFFER_SIZE 512

#endif

