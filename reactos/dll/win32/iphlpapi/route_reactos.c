/*
 * iphlpapi dll implementation -- Setting and storing route information
 *
 * These are stubs for functions that set routing information on the target
 * operating system.  They are grouped here because their implementation will
 * vary widely by operating system.
 *
 * Copyright (C) 2004 Art Yerkes
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "config.h"
#include "iphlpapi_private.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "resinfo.h"
#include "iphlpapi.h"
#include "wine/debug.h"

#define IP_FORWARD_ADD 3
#define IP_FORWARD_DEL 2

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

DWORD createIpForwardEntry( PMIB_IPFORWARDROW pRoute ) {
    HANDLE tcpFile = INVALID_HANDLE_VALUE;
    NTSTATUS status = openTcpFile( &tcpFile );
    TCP_REQUEST_SET_INFORMATION_EX_SAFELY_SIZED req = 
        TCP_REQUEST_SET_INFORMATION_INIT;
    IPRouteEntry *rte;
    TDIEntityID   id;
    DWORD         returnSize = 0;
    
    TRACE("Called.\n");

    if( NT_SUCCESS(status) )
        status = getNthIpEntity( tcpFile, 0, &id );

    if( NT_SUCCESS(status) ) {
        req.Req.ID.toi_class                = INFO_CLASS_PROTOCOL;
        req.Req.ID.toi_type                 = INFO_TYPE_PROVIDER;
        req.Req.ID.toi_id                   = IP_MIB_ROUTETABLE_ENTRY_ID;
        req.Req.ID.toi_entity               = id;
        req.Req.BufferSize                  = sizeof(*rte);
        rte                                 = 
            (IPRouteEntry *)&req.Req.Buffer[0];

	// dwForwardPolicy
	// dwForwardNextHopAS
	rte->ire_dest    = pRoute->dwForwardDest;
	rte->ire_mask    = pRoute->dwForwardMask;
	rte->ire_gw      = pRoute->dwForwardNextHop;
	rte->ire_index   = pRoute->dwForwardIfIndex;
	rte->ire_type    = IP_FORWARD_ADD;
	rte->ire_proto   = pRoute->dwForwardProto;
	rte->ire_age     = pRoute->dwForwardAge;
	rte->ire_metric1 = pRoute->dwForwardMetric1;
	rte->ire_metric2 = pRoute->dwForwardMetric2;
	rte->ire_metric3 = pRoute->dwForwardMetric3;
	rte->ire_metric4 = pRoute->dwForwardMetric4;
	rte->ire_metric5 = pRoute->dwForwardMetric5;

        status = DeviceIoControl( tcpFile,
                                  IOCTL_TCP_SET_INFORMATION_EX,
                                  &req,
                                  sizeof(req),
                                  NULL,
                                  0,
                                  &returnSize,
                                  NULL );
    }

    if( tcpFile != INVALID_HANDLE_VALUE )
        closeTcpFile( tcpFile );

    TRACE("Returning: %08x (IOCTL was %08x)\n", status, IOCTL_TCP_SET_INFORMATION_EX);

    if( NT_SUCCESS(status) )
	return NO_ERROR;
    else
	return status;
}

DWORD setIpForwardEntry( PMIB_IPFORWARDROW pRoute ) {
    FIXME(":stub\n");
    return (DWORD) 0;
}

DWORD deleteIpForwardEntry( PMIB_IPFORWARDROW pRoute ) {
    HANDLE tcpFile = INVALID_HANDLE_VALUE;
    NTSTATUS status = openTcpFile( &tcpFile );
    TCP_REQUEST_SET_INFORMATION_EX_SAFELY_SIZED req = 
        TCP_REQUEST_SET_INFORMATION_INIT;
    IPRouteEntry *rte;
    TDIEntityID   id;
    DWORD         returnSize = 0;
    
    TRACE("Called.\n");

    if( NT_SUCCESS(status) )
        status = getNthIpEntity( tcpFile, 0, &id );

    if( NT_SUCCESS(status) ) {
        req.Req.ID.toi_class                = INFO_CLASS_PROTOCOL;
        req.Req.ID.toi_type                 = INFO_TYPE_PROVIDER;
        req.Req.ID.toi_id                   = IP_MIB_ROUTETABLE_ENTRY_ID;
        req.Req.ID.toi_entity               = id;
        req.Req.BufferSize                  = sizeof(*rte);
        rte                                 = 
            (IPRouteEntry *)&req.Req.Buffer[0];

	// dwForwardPolicy
	// dwForwardNextHopAS
	rte->ire_dest    = pRoute->dwForwardDest;
	rte->ire_mask    = INADDR_NONE;
	rte->ire_gw      = pRoute->dwForwardNextHop;
	rte->ire_index   = pRoute->dwForwardIfIndex;
	rte->ire_type    = IP_FORWARD_DEL;
	rte->ire_proto   = pRoute->dwForwardProto;
	rte->ire_age     = pRoute->dwForwardAge;
	rte->ire_metric1 = pRoute->dwForwardMetric1;
	rte->ire_metric2 = INADDR_NONE;
	rte->ire_metric3 = INADDR_NONE;
	rte->ire_metric4 = INADDR_NONE;
	rte->ire_metric5 = INADDR_NONE;

        status = DeviceIoControl( tcpFile,
                                  IOCTL_TCP_SET_INFORMATION_EX,
                                  &req,
                                  sizeof(req),
                                  NULL,
                                  0,
                                  &returnSize,
                                  NULL );
    }

    if( tcpFile != INVALID_HANDLE_VALUE )
        closeTcpFile( tcpFile );

    TRACE("Returning: %08x (IOCTL was %08x)\n", status, IOCTL_TCP_SET_INFORMATION_EX);

    if( NT_SUCCESS(status) )
	return NO_ERROR;
    else
	return status;
}
