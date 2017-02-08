/*
 * Defines the types and macros used by the ICMP API, see icmpapi.h.
 *
 * Copyright (C) 1999 Francois Gouget
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

#ifndef __WINE_IPEXPORT_H
#define __WINE_IPEXPORT_H

#include <in6addr.h>
#include <inaddr.h>

typedef ULONG IPAddr;
typedef ULONG IPMask;
typedef ULONG IP_STATUS;

struct ip_option_information
{
    unsigned char  Ttl;
    unsigned char  Tos;
    unsigned char  Flags;
    unsigned char  OptionsSize;
    unsigned char* OptionsData;
};

#define IP_FLAG_DF      0x2

#define IP_OPT_EOL      0
#define IP_OPT_NOP      1
#define IP_OPT_SECURITY 0x82
#define IP_OPT_LSRR     0x83
#define IP_OPT_SSRR     0x89
#define IP_OPT_RR       0x7
#define IP_OPT_TS       0x44
#define IP_OPT_SID      0x88

#define MAX_OPT_SIZE    40


struct icmp_echo_reply
{
    IPAddr                       Address;
    ULONG                        Status;
    ULONG                        RoundTripTime;
    unsigned short               DataSize;
    unsigned short               Reserved;
    void*                        Data;
    struct ip_option_information Options;
};

typedef struct ip_option_information IP_OPTION_INFORMATION, *PIP_OPTION_INFORMATION;

typedef struct icmp_echo_reply ICMP_ECHO_REPLY, *PICMP_ECHO_REPLY;


#define IP_STATUS_BASE              11000

#define IP_SUCCESS                  0
#define IP_BUF_TOO_SMALL            (IP_STATUS_BASE + 1)
#define IP_DEST_NET_UNREACHABLE     (IP_STATUS_BASE + 2)
#define IP_DEST_HOST_UNREACHABLE    (IP_STATUS_BASE + 3)
#define IP_DEST_PROT_UNREACHABLE    (IP_STATUS_BASE + 4)
#define IP_DEST_PORT_UNREACHABLE    (IP_STATUS_BASE + 5)
#define IP_NO_RESOURCES             (IP_STATUS_BASE + 6)
#define IP_BAD_OPTION               (IP_STATUS_BASE + 7)
#define IP_HW_ERROR                 (IP_STATUS_BASE + 8)
#define IP_PACKET_TOO_BIG           (IP_STATUS_BASE + 9)
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)
#define IP_BAD_REQ                  (IP_STATUS_BASE + 11)
#define IP_BAD_ROUTE                (IP_STATUS_BASE + 12)
#define IP_TTL_EXPIRED_TRANSIT      (IP_STATUS_BASE + 13)
#define IP_TTL_EXPIRED_REASSEM      (IP_STATUS_BASE + 14)
#define IP_PARAM_PROBLEM            (IP_STATUS_BASE + 15)
#define IP_SOURCE_QUENCH            (IP_STATUS_BASE + 16)
#define IP_OPTION_TOO_BIG           (IP_STATUS_BASE + 17)
#define IP_BAD_DESTINATION          (IP_STATUS_BASE + 18)

#define IP_ADDR_DELETED             (IP_STATUS_BASE + 19)
#define IP_SPEC_MTU_CHANGE          (IP_STATUS_BASE + 20)
#define IP_MTU_CHANGE               (IP_STATUS_BASE + 21)
#define IP_UNLOAD                   (IP_STATUS_BASE + 22)

#define IP_GENERAL_FAILURE          (IP_STATUS_BASE + 50)
#define MAX_IP_STATUS               IP_GENERAL_FAILURE
#define IP_PENDING                  (IP_STATUS_BASE + 255)


#define MAX_ADAPTER_NAME 128

typedef struct _IP_ADAPTER_INDEX_MAP {
  ULONG Index;
  WCHAR Name[MAX_ADAPTER_NAME];
} IP_ADAPTER_INDEX_MAP, *PIP_ADAPTER_INDEX_MAP;

typedef struct _IP_INTERFACE_INFO {
  LONG                 NumAdapters;
  IP_ADAPTER_INDEX_MAP Adapter[1];
} IP_INTERFACE_INFO,*PIP_INTERFACE_INFO;

typedef struct _IP_UNIDIRECTIONAL_ADAPTER_ADDRESS {
  ULONG  NumAdapters;
  IPAddr Address[1];
} IP_UNIDIRECTIONAL_ADAPTER_ADDRESS, *PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS;

/* ReactOS */

typedef struct _IP_ADAPTER_ORDER_MAP {
  ULONG NumAdapters;
  ULONG AdapterOrder[1];
} IP_ADAPTER_ORDER_MAP, *PIP_ADAPTER_ORDER_MAP;

#if (NTDDI_VERSION >= NTDDI_WINXP)

#include <pshpack1.h>
typedef struct _IPV6_ADDRESS_EX {
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  USHORT sin6_addr[8];
  ULONG sin6_scope_id;
} IPV6_ADDRESS_EX, *PIPV6_ADDRESS_EX;
#include <poppack.h>

typedef struct icmpv6_echo_reply_lh {
  IPV6_ADDRESS_EX Address;
  ULONG Status;
  unsigned int RoundTripTime;
} ICMPV6_ECHO_REPLY_LH, *PICMPV6_ECHO_REPLY_LH;

typedef ICMPV6_ECHO_REPLY_LH ICMPV6_ECHO_REPLY;
typedef ICMPV6_ECHO_REPLY_LH *PICMPV6_ECHO_REPLY;

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#endif /* __WINE_IPEXPORT_H */
