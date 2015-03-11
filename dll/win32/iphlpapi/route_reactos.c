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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include "iphlpapi_private.h"

#define IP_FORWARD_ADD 3
#define IP_FORWARD_DEL 2

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

DWORD createIpForwardEntry( PMIB_IPFORWARDROW pRoute ) {
    HANDLE tcpFile;
    NTSTATUS status = openTcpFile( &tcpFile, FILE_READ_DATA | FILE_WRITE_DATA );
    TCP_REQUEST_SET_INFORMATION_EX_ROUTE_ENTRY req =
        TCP_REQUEST_SET_INFORMATION_INIT;
    IPRouteEntry *rte;
    TDIEntityID   id;
    DWORD         returnSize = 0;

    TRACE("Called.\n");

    if( NT_SUCCESS(status) ) {
        status = getNthIpEntity( tcpFile, pRoute->dwForwardIfIndex, &id );

        if( NT_SUCCESS(status) ) {
            req.Req.ID.toi_class                = INFO_CLASS_PROTOCOL;
            req.Req.ID.toi_type                 = INFO_TYPE_PROVIDER;
            req.Req.ID.toi_id                   = IP_MIB_ARPTABLE_ENTRY_ID;
            req.Req.ID.toi_entity.tei_instance  = id.tei_instance;
            req.Req.ID.toi_entity.tei_entity    = CL_NL_ENTITY;
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

        closeTcpFile( tcpFile );
    }

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
    HANDLE tcpFile;
    NTSTATUS status = openTcpFile( &tcpFile, FILE_READ_DATA | FILE_WRITE_DATA );
    TCP_REQUEST_SET_INFORMATION_EX_ROUTE_ENTRY req =
        TCP_REQUEST_SET_INFORMATION_INIT;
    IPRouteEntry *rte;
    TDIEntityID   id;
    DWORD         returnSize = 0;

    TRACE("Called.\n");

    if( NT_SUCCESS(status) ) {
        status = getNthIpEntity( tcpFile, pRoute->dwForwardIfIndex, &id );

        if( NT_SUCCESS(status) ) {
            req.Req.ID.toi_class                = INFO_CLASS_PROTOCOL;
            req.Req.ID.toi_type                 = INFO_TYPE_PROVIDER;
            req.Req.ID.toi_id                   = IP_MIB_ARPTABLE_ENTRY_ID;
            req.Req.ID.toi_entity.tei_instance  = id.tei_instance;
            req.Req.ID.toi_entity.tei_entity    = CL_NL_ENTITY;
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

        closeTcpFile( tcpFile );
    }

    TRACE("Returning: %08x (IOCTL was %08x)\n", status, IOCTL_TCP_SET_INFORMATION_EX);

    if( NT_SUCCESS(status) )
	return NO_ERROR;
    else
	return status;
}
