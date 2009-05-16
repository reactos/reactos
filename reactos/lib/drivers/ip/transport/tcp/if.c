/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 *
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include "precomp.h"

int if_index = 0;
struct ifaddr **ifnet_addrs;

int	ifqmaxlen = OSK_IFQ_MAXLEN;
struct	ifnet *ifnet;

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */

POSK_IFADDR TCPGetInterfaceData( PIP_INTERFACE IF ) {
    NTSTATUS Status;
    POSK_IFADDR ifaddr = IF->TCPContext;
    ASSERT(ifaddr);

    RtlZeroMemory(ifaddr, sizeof(OSK_IFADDR) + 2 * sizeof( struct sockaddr_in ));

    struct sockaddr_in *addr_in = (struct sockaddr_in *)&ifaddr[1];
    struct sockaddr_in *dstaddr_in = (struct sockaddr_in *)&addr_in[1];

    TI_DbgPrint(DEBUG_TCPIF,("Called\n"));

    ifaddr->ifa_dstaddr = (struct sockaddr *)dstaddr_in;

    /* XXX - Point-to-point interfaces not supported yet */
    memset( &ifaddr->ifa_dstaddr, 0, sizeof( struct sockaddr ) );

    ifaddr->ifa_addr = (struct sockaddr *)addr_in;
    Status = GetInterfaceIPv4Address( IF,
				      ADE_UNICAST,
				      (PULONG)&addr_in->sin_addr.s_addr );

    ASSERT(NT_SUCCESS(Status));

    TI_DbgPrint(DEBUG_TCPIF,("interface %x : addr %x\n",
			   IF, addr_in->sin_addr.s_addr));

    ifaddr->ifa_flags = 0; /* XXX what goes here? */
    ifaddr->ifa_refcnt = 0; /* Anachronistic */
    ifaddr->ifa_metric = 1; /* We can get it like in ninfo.c, if we want */
    ifaddr->ifa_mtu = IF->MTU;

    TI_DbgPrint(DEBUG_TCPIF,("Leaving\n"));

    return ifaddr;
}

POSK_IFADDR TCPFindInterface( void *ClientData,
			      OSK_UINT AddrType,
			      OSK_UINT FindType,
			      OSK_SOCKADDR *ReqAddr,
			      OSK_IFADDR *Interface ) {
    PIP_INTERFACE IF;
    IP_ADDRESS Destination;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)ReqAddr;
    POSK_IFADDR InterfaceData;

    TI_DbgPrint(DEBUG_TCPIF,("called for type %d\n", FindType));

    if( !ReqAddr ) {
	TI_DbgPrint(DEBUG_TCPIF,("no addr or no ifaddr (%x)\n", ReqAddr));
	return NULL;
    }

    Destination.Type = IP_ADDRESS_V4;
    Destination.Address.IPv4Address = addr_in->sin_addr.s_addr;

    TI_DbgPrint(DEBUG_TCPIF,("Address is %x\n", addr_in->sin_addr.s_addr));

    IF = FindOnLinkInterface(&Destination);
    if (!IF) return NULL;

    InterfaceData = TCPGetInterfaceData(IF);

    addr_in = (struct sockaddr_in *)
	InterfaceData->ifa_addr;

    TI_DbgPrint(DEBUG_TCPIF,("returning addr %x\n", addr_in->sin_addr.s_addr));

    return InterfaceData;
}

