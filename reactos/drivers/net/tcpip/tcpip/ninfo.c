/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/ninfo.c
 * PURPOSE:     Network information
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <tcpip.h>
#include <address.h>
#include <info.h>
#include <pool.h>
#include <prefix.h>
#include <ip.h>
#include <route.h>

TDI_STATUS InfoTdiQueryGetAddrTable( PNDIS_BUFFER Buffer, 
				     PUINT BufferSize ) {
    PIP_INTERFACE CurrentIF;
    PLIST_ENTRY CurrentIFEntry;
    TDI_STATUS Status = TDI_INVALID_REQUEST;
    KIRQL OldIrql;
    UINT Count = 1; /* Start adapter indices at 1 */
    UINT IfCount = CountInterfaces();
    PIPADDR_ENTRY IpAddress = 
	ExAllocatePool( NonPagedPool, sizeof( IPADDR_ENTRY ) * IfCount );
    PIPADDR_ENTRY IpCurrent = IpAddress;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));
    
    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead)
    {
	CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);

	IpCurrent->Index     = Count;
	IpCurrent->Addr      = 0;
	IpCurrent->BcastAddr = 0;
	IpCurrent->Mask      = 0;
	
	/* Locate the diffrent addresses and put them the right place */
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_UNICAST,
				 &IpAddress->Addr );
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_MULTICAST,
				 &IpAddress->BcastAddr );
	GetInterfaceIPv4Address( CurrentIF,
				 ADE_ADDRMASK,
				 &IpAddress->Mask );
	IpCurrent++;
	CurrentIFEntry = CurrentIFEntry->Flink;
	Count++;
    }
    
    KeReleaseSpinLock(&InterfaceListLock, OldIrql);

    Status = InfoCopyOut( IpAddress, sizeof(*IpAddress) * Count,
			  Buffer, BufferSize );
    
    ExFreePool( IpAddress );

    TI_DbgPrint(MAX_TRACE, ("Returning %08x\n", Status));

    return Status;
}

/* Get IPRouteEntry s for each of the routes in the system */
TDI_STATUS InfoTdiQueryGetRouteTable( PNDIS_BUFFER Buffer, PUINT BufferSize ) {
    PIP_INTERFACE CurrentIF;
    PLIST_ENTRY CurrentIFEntry;
    TDI_STATUS Status;
    KIRQL OldIrql;
    UINT RtCount = CountRouteNodes( NULL ),
	Size = sizeof( IPROUTE_ENTRY ) * RtCount;
    PROUTE_CACHE_NODE RCache = 
	ExAllocatePool( NonPagedPool, sizeof( ROUTE_CACHE_NODE ) * RtCount ),
	RCacheCur = RCache;
    PIPROUTE_ENTRY RouteEntries = ExAllocatePool( NonPagedPool, Size ),
	RtCurrent = RouteEntries;

    TI_DbgPrint(MAX_TRACE, ("Called, routes = %d, RCache = %08x\n", 
			    RtCount, RCache));

    if( !RCache || !RouteEntries ) {
	if( RCache ) ExFreePool( RCache );
	if( RouteEntries ) ExFreePool( RouteEntries );
	return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( RouteEntries, Size );

    RtCount = CopyRouteNodes( NULL, RCache );
    
    while( RtCurrent < RouteEntries + RtCount ) {
#if 0
	/* We'll see what we're getting where easier this way */
	RtCurrent->Dest =    FOURCC('d','e','s','t');
	RtCurrent->Index =   FOURCC('i','n','d','x');
	RtCurrent->Metric1 = FOURCC('m','e','t','1');
	RtCurrent->Metric2 = FOURCC('m','e','t','2');
	RtCurrent->Metric3 = FOURCC('m','e','t','3');
	RtCurrent->Metric4 = FOURCC('m','e','t','4');
	RtCurrent->Gw =      FOURCC('g','a','t','e');
	RtCurrent->Type =    FOURCC('t','y','p','e');
	RtCurrent->Proto =   FOURCC('p','r','o','t');
	RtCurrent->Age =     FOURCC('a','g','e',' ');
	RtCurrent->Mask =    FOURCC('m','a','s','k');
	RtCurrent->Metric5 = FOURCC('m','e','t','5');
	RtCurrent->Info =    FOURCC('i','n','f','o');
#endif

	/* Copy Desitnation */
	RtlCopyMemory( &RtCurrent->Dest, 
		       &RCacheCur->Destination.Address.IPv4Address,
		       sizeof(RtCurrent->Dest) );
	RtlCopyMemory( &RtCurrent->Gw,
		       &RCacheCur->NTE->Address,
		       sizeof(RtCurrent->Gw) );

	KeAcquireSpinLock(&EntityListLock, &OldIrql);
	for( RtCurrent->Index = EntityCount; 
	     RtCurrent->Index < EntityMax && 
		 RCacheCur->NTE->Interface != 
		 EntityList[RtCurrent->Index].context;
	     RtCurrent->Index++ );
	KeReleaseSpinLock(&EntityListLock, OldIrql);

	if( RCacheCur->NTE && RCacheCur->NTE->PLE ) 
	    RtCurrent->Mask = 
		~((1 << (32 - RCacheCur->NTE->PLE->PrefixLength)) - 1);
	else
	    TI_DbgPrint
		(MIN_TRACE,
		 ("RCacheCur->NTE : %08x, RCacheCur->NTE->PLE : %08x\n",
		  RCacheCur->NTE, RCacheCur->NTE->PLE));

	RtCurrent->Metric1 = 1 /* Need to store + get this */;
	RtCurrent->Type = 2 /* PF_INET */;

	TI_DbgPrint
	    (MAX_TRACE,("RouteEntry %d:\n", RtCurrent - RouteEntries ));
	TI_DbgPrint(MAX_TRACE,("Source Data:\n"));
	TI_DbgPrint(MAX_TRACE,("Destination: %08x\n", 
			       RCacheCur->Destination.Address.IPv4Address));
	TI_DbgPrint(MAX_TRACE,("NTE: %08x\n"));
	TI_DbgPrint(MAX_TRACE,("NCE: %08x\n"));
	TI_DbgPrint(MAX_TRACE,("Target Data:\n"));
	TI_DbgPrint(MAX_TRACE,("Dest: %08x\n", RtCurrent->Dest));
	TI_DbgPrint(MAX_TRACE,("Index: %08x\n", RtCurrent->Index));
	TI_DbgPrint(MAX_TRACE,("Metric1: %08x\n", RtCurrent->Metric1));
	TI_DbgPrint(MAX_TRACE,("Metric2: %08x\n", RtCurrent->Metric2));
	TI_DbgPrint(MAX_TRACE,("Metric3: %08x\n", RtCurrent->Metric3));
	TI_DbgPrint(MAX_TRACE,("Metric4: %08x\n", RtCurrent->Metric4));
	TI_DbgPrint(MAX_TRACE,("Gw: %08x\n", RtCurrent->Gw));
	TI_DbgPrint(MAX_TRACE,("Type: %08x\n", RtCurrent->Type));
	TI_DbgPrint(MAX_TRACE,("Proto: %08x\n", RtCurrent->Proto));
	TI_DbgPrint(MAX_TRACE,("Age: %08x\n", RtCurrent->Age));
	TI_DbgPrint(MAX_TRACE,("Mask: %08x\n", RtCurrent->Mask));
	TI_DbgPrint(MAX_TRACE,("Metric5: %08x\n", RtCurrent->Metric5));
	TI_DbgPrint(MAX_TRACE,("Info: %08x\n", RtCurrent->Info));
	
	RtCurrent++; RCacheCur++;
    }

    Status = InfoCopyOut( RouteEntries, Size, Buffer, BufferSize );

    ExFreePool( RouteEntries );
    ExFreePool( RCache );

    TI_DbgPrint(MAX_TRACE, ("Returning %08x\n", Status));

    return Status;
}
		
TDI_STATUS InfoTdiQueryGetIPSnmpInfo( PNDIS_BUFFER Buffer,
				      PUINT BufferSize ) {
    KIRQL OldIrql;
    PIP_INTERFACE CurrentIF;
    PLIST_ENTRY CurrentIFEntry;
    IPSNMP_INFO SnmpInfo;
    UINT IfCount = CountInterfaces();
    UINT AddrCount = 0;
    UINT RouteCount = CountRouteNodes( NULL );
    TDI_STATUS Status = TDI_INVALID_REQUEST;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    RtlZeroMemory(&SnmpInfo, sizeof(IPSNMP_INFO));
    
    /* Count number of addresses */
    AddrCount = 0;
    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead)
    {
	CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);
	AddrCount += CountInterfaceAddresses( CurrentIF );
	CurrentIFEntry = CurrentIFEntry->Flink;
    }
    
    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
    
    SnmpInfo.NumIf = IfCount;
    SnmpInfo.NumAddr = AddrCount;
    SnmpInfo.NumRoutes = RouteCount;

    Status = InfoCopyOut( &SnmpInfo, sizeof(SnmpInfo), 
			  Buffer, BufferSize );

    TI_DbgPrint(MAX_TRACE, ("Returning %08x\n", Status));

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
    
    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    switch( InfoClass ) {
    case INFO_CLASS_GENERIC:
	if( InfoType == INFO_TYPE_PROVIDER && InfoId == ENTITY_TYPE_ID ) {
	    ULONG Return = CL_NL_IP;
	    Status = InfoCopyOut( &Return, sizeof(Return), 
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

    TI_DbgPrint(MAX_TRACE, ("Returning %08x\n", Status));

    return Status;
}

TDI_STATUS InfoNetworkLayerTdiSetEx( UINT InfoClass,
				     UINT InfoType,
				     UINT InfoId,
				     PVOID Context,
				     TDIEntityID *id,
				     PCHAR Buffer,
				     UINT BufferSize ) {
}
