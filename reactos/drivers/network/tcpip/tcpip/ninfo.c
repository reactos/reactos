/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/ninfo.c
 * PURPOSE:     Network information
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

#define IP_ROUTE_TYPE_ADD 3
#define IP_ROUTE_TYPE_DEL 2

TDI_STATUS InfoTdiQueryGetAddrTable( PNDIS_BUFFER Buffer,
				     PUINT BufferSize ) {

    TDI_STATUS Status = TDI_INVALID_REQUEST;
    KIRQL OldIrql;
    UINT Count = 0;
    UINT IfCount = CountInterfaces();
    PIPADDR_ENTRY IpAddress =
	ExAllocatePool( NonPagedPool, sizeof( IPADDR_ENTRY ) * IfCount );
    PIPADDR_ENTRY IpCurrent = IpAddress;
    IF_LIST_ITER(CurrentIF);

    TI_DbgPrint(DEBUG_INFO, ("Called.\n"));

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    ForEachInterface(CurrentIF) {
	IpCurrent->Index     = CurrentIF->Index;
	IpCurrent->Addr      = 0;
	IpCurrent->BcastAddr = 0;
	IpCurrent->Mask      = 0;

	/* Locate the diffrent addresses and put them the right place */
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_UNICAST,
				 &IpCurrent->Addr );
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_BROADCAST,
				 &IpCurrent->BcastAddr );
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_ADDRMASK,
				 &IpCurrent->Mask );
	IpCurrent++;
	Count++;
    } EndFor(CurrentIF);
    ASSERT( Count == IfCount );

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    Status = InfoCopyOut( (PCHAR)IpAddress, sizeof(*IpAddress) * IfCount,
			  Buffer, BufferSize );

    ExFreePool( IpAddress );

    TI_DbgPrint(DEBUG_INFO, ("Returning %08x\n", Status));

    return Status;
}

/* Get IPRouteEntry s for each of the routes in the system */
TDI_STATUS InfoTdiQueryGetRouteTable( PNDIS_BUFFER Buffer, PUINT BufferSize ) {
    TDI_STATUS Status;
    KIRQL OldIrql;
    UINT RtCount = CountFIBs();
    UINT Size = sizeof( IPROUTE_ENTRY ) * RtCount;
    PFIB_ENTRY RCache =
	ExAllocatePool( NonPagedPool, sizeof( FIB_ENTRY ) * RtCount ),
	RCacheCur = RCache;
    PIPROUTE_ENTRY RouteEntries = ExAllocatePool( NonPagedPool, Size ),
	RtCurrent = RouteEntries;

    TI_DbgPrint(DEBUG_INFO, ("Called, routes = %d, RCache = %08x\n",
			    RtCount, RCache));

    if( !RCache || !RouteEntries ) {
	if( RCache ) ExFreePool( RCache );
	if( RouteEntries ) ExFreePool( RouteEntries );
	return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( RouteEntries, Size );

    RtCount = CopyFIBs( RCache );

    while( RtCurrent < RouteEntries + RtCount ) {
	/* Copy Desitnation */
	RtlCopyMemory( &RtCurrent->Dest,
		       &RCacheCur->NetworkAddress.Address,
		       sizeof(RtCurrent->Dest) );
	RtlCopyMemory( &RtCurrent->Mask,
		       &RCacheCur->Netmask.Address,
		       sizeof(RtCurrent->Mask) );

	if( RCacheCur->Router )
	    RtlCopyMemory( &RtCurrent->Gw,
			   &RCacheCur->Router->Address.Address,
			   sizeof(RtCurrent->Gw) );
	else
	    RtlZeroMemory( &RtCurrent->Gw, sizeof(RtCurrent->Gw) );

	RtCurrent->Metric1 = RCacheCur->Metric;
	RtCurrent->Type = TDI_ADDRESS_TYPE_IP;

	TI_DbgPrint
	    (DEBUG_INFO,
	     ("%d: NA %08x NM %08x GW %08x MT %x\n",
	      RtCurrent - RouteEntries,
	      RtCurrent->Dest,
	      RtCurrent->Mask,
	      RtCurrent->Gw,
	      RtCurrent->Metric1 ));

	TcpipAcquireSpinLock(&EntityListLock, &OldIrql);
	for( RtCurrent->Index = EntityCount;
	     RtCurrent->Index > 0 &&
		 RCacheCur->Router->Interface !=
		 EntityList[RtCurrent->Index - 1].context;
	     RtCurrent->Index-- );

        RtCurrent->Index = EntityList[RtCurrent->Index - 1].tei_instance;
	TcpipReleaseSpinLock(&EntityListLock, OldIrql);

	RtCurrent++; RCacheCur++;
    }

    Status = InfoCopyOut( (PCHAR)RouteEntries, Size, Buffer, BufferSize );

    ExFreePool( RouteEntries );
    ExFreePool( RCache );

    TI_DbgPrint(DEBUG_INFO, ("Returning %08x\n", Status));

    return Status;
}

TDI_STATUS InfoTdiQueryGetIPSnmpInfo( PNDIS_BUFFER Buffer,
				      PUINT BufferSize ) {
    IPSNMP_INFO SnmpInfo;
    UINT IfCount = CountInterfaces();
    UINT RouteCount = CountFIBs();
    TDI_STATUS Status = TDI_INVALID_REQUEST;

    TI_DbgPrint(DEBUG_INFO, ("Called.\n"));

    RtlZeroMemory(&SnmpInfo, sizeof(IPSNMP_INFO));

    SnmpInfo.NumIf = IfCount;
    SnmpInfo.NumAddr = 1;
    SnmpInfo.NumRoutes = RouteCount;

    Status = InfoCopyOut( (PCHAR)&SnmpInfo, sizeof(SnmpInfo),
			  Buffer, BufferSize );

    TI_DbgPrint(DEBUG_INFO, ("Returning %08x\n", Status));

    return Status;
}

TDI_STATUS InfoNetworkLayerTdiQueryEx( UINT InfoClass,
				       UINT InfoType,
				       UINT InfoId,
				       PVOID Context,
				       TDIEntityID *id,
				       PNDIS_BUFFER Buffer,
				       PUINT BufferSize ) {
    TDI_STATUS Status = TDI_INVALID_REQUEST;

    TI_DbgPrint(DEBUG_INFO, ("Called.\n"));

    switch( InfoClass ) {
    case INFO_CLASS_GENERIC:
	if( InfoType == INFO_TYPE_PROVIDER && InfoId == ENTITY_TYPE_ID ) {
	    ULONG Return = CL_NL_IP;
	    Status = InfoCopyOut( (PCHAR)&Return, sizeof(Return),
				  Buffer, BufferSize );
	}
	break;

    case INFO_CLASS_PROTOCOL:
	switch( InfoType ) {
	case INFO_TYPE_PROVIDER:
	    switch( InfoId ) {
	    case IP_MIB_ADDRTABLE_ENTRY_ID:
		Status = InfoTdiQueryGetAddrTable( Buffer, BufferSize );
		break;

	    case IP_MIB_ROUTETABLE_ENTRY_ID:
		Status = InfoTdiQueryGetRouteTable( Buffer, BufferSize );
		break;

	    case IP_MIB_STATS_ID:
		Status = InfoTdiQueryGetIPSnmpInfo( Buffer, BufferSize );
		break;
	    }
	    break;
	}
    }

    TI_DbgPrint(DEBUG_INFO, ("Returning %08x\n", Status));

    return Status;
}

TDI_STATUS InfoNetworkLayerTdiSetEx( UINT InfoClass,
				     UINT InfoType,
				     UINT InfoId,
				     PVOID Context,
				     TDIEntityID *id,
				     PCHAR Buffer,
				     UINT BufferSize ) {
    NTSTATUS Status = TDI_INVALID_REQUEST;
    IP_ADDRESS Address;
    IP_ADDRESS Netmask;
    IP_ADDRESS Router;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_INFO,("Called\n"));

    //OskitDumpBuffer( (OSK_PCHAR)Buffer, BufferSize );

    if( InfoClass == INFO_CLASS_PROTOCOL &&
	InfoType == INFO_TYPE_PROVIDER &&
	InfoId == IP_MIB_ROUTETABLE_ENTRY_ID &&
	id->tei_entity == CL_NL_ENTITY ) { /* Add or delete a route */
	PIPROUTE_ENTRY Route = (PIPROUTE_ENTRY)Buffer;
	AddrInitIPv4( &Address, Route->Dest );
	AddrInitIPv4( &Netmask, Route->Mask );
	AddrInitIPv4( &Router,  Route->Gw );

	if( Route->Type == IP_ROUTE_TYPE_ADD ) { /* Add the route */
	    TI_DbgPrint(DEBUG_INFO,("Adding route (%s)\n", A2S(&Address)));
	    /* Find the existing route this belongs to */
	    NCE = RouterGetRoute( &Router );
	    /* Really add the route */
	    if( NCE &&
		RouterCreateRoute( &Address, &Netmask, &Router,
				   NCE->Interface, Route->Metric1 ) )
		Status = STATUS_SUCCESS;
	    else
		Status = STATUS_UNSUCCESSFUL;
	} else if( Route->Type == IP_ROUTE_TYPE_DEL ) {
	    TI_DbgPrint(DEBUG_INFO,("Removing route (%s)\n", A2S(&Address)));
	    Status = RouterRemoveRoute( &Address, &Router );
	} else Status = TDI_INVALID_REQUEST;
    }

    TI_DbgPrint(DEBUG_INFO,("Returning %x\n", Status));

    return Status;
}
