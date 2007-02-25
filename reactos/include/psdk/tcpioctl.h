/*
 * tcpioctl.h
 *
 * Set and query ioctl constants for tcpip.sys
 *
 * Contributors:
 *   Created by Art Yerkes (ayerkes@speakeasy.net) from 
 *    drivers/net/tcpip/include/ticonsts.h
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _TCPIOCTL_H
#define _TCPIOCTL_H

#define DD_TCP_DEVICE_NAME L"\\Device\\Tcp"

/* TCP/UDP/RawIP IOCTL code definitions */

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(Function, Method, Access) \
    CTL_CODE(FSCTL_TCP_BASE, Function, Method, Access)

#define IOCTL_TCP_QUERY_INFORMATION_EX \
    _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX \
    _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SET_IP_ADDRESS \
    _TCP_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_DELETE_IP_ADDRESS \
    _TCP_CTL_CODE(16, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IF_MIB_STATS_ID                 1
#define IP_MIB_STATS_ID                 1
#define IP_MIB_ARPTABLE_ENTRY_ID        0x101
#define IP_MIB_ADDRTABLE_ENTRY_ID       0x102
#define IP_INTFC_INFO_ID                0x103
#define MAX_PHYSADDR_SIZE               8

typedef struct IFEntry
{
    ULONG if_index;
    ULONG if_type;
    ULONG if_mtu;
    ULONG if_speed;
    ULONG if_physaddrlen;
    UCHAR if_physaddr[MAX_PHYSADDR_SIZE];
    ULONG if_adminstatus;
    ULONG if_operstatus;
    ULONG if_lastchange;
    ULONG if_inoctets;
    ULONG if_inucastpkts;
    ULONG if_innucastpkts;
    ULONG if_indiscards;
    ULONG if_inerrors;
    ULONG if_inunknownprotos;
    ULONG if_outoctets;
    ULONG if_outucastpkts;
    ULONG if_outnucastpkts;
    ULONG if_outdiscards;
    ULONG if_outerrors;
    ULONG if_outqlen;
    ULONG if_descrlen;
    UCHAR if_descr[1];
} IFEntry;

typedef struct IPSNMPInfo
{
    ULONG ipsi_forwarding;
    ULONG ipsi_defaultttl;
    ULONG ipsi_inreceives;
    ULONG ipsi_inhdrerrors;
    ULONG ipsi_inaddrerrors;
    ULONG ipsi_forwdatagrams;
    ULONG ipsi_inunknownprotos;
    ULONG ipsi_indiscards;
    ULONG ipsi_indelivers;
    ULONG ipsi_outrequests;
    ULONG ipsi_routingdiscards;
    ULONG ipsi_outdiscards;
    ULONG ipsi_outnoroutes;
    ULONG ipsi_reasmtimeout;
    ULONG ipsi_reasmreqds;
    ULONG ipsi_reasmoks;
    ULONG ipsi_reasmfails;
    ULONG ipsi_fragoks;
    ULONG ipsi_fragfails;
    ULONG ipsi_fragcreates;
    ULONG ipsi_numif;
    ULONG ipsi_numaddr;
    ULONG ipsi_numroutes;
} IPSNMPInfo;

typedef struct IPAddrEntry
{
    ULONG iae_addr;
    ULONG iae_index;
    ULONG iae_mask;
    ULONG iae_bcastaddr;
    ULONG iae_reasmsize;
    USHORT iae_context;
    USHORT iae_pad;
} IPAddrEntry;

typedef struct IPInterfaceInfo
{
    ULONG iii_flags;
    ULONG iii_mtu;
    ULONG iii_speed;
    ULONG iii_addrlength;
    UCHAR iii_addr[1];
} IPInterfaceInfo;

#endif/*_TCPIOCTL_H*/
