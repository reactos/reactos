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
    UINT RtCount = CountFIBs(),
	Size = sizeof( IPROUTE_ENTRY ) * RtCount;
    PFIB_ENTRY RCache = 
	ExAllocatePool( NonPagedPool, sizeof( FIB_ENTRY ) * RtCount ),
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

    RtCount = CopyFIBs( RCache );
    
    while( RtCurrent < RouteEntries + RtCount ) {
	/* Copy Desitnation */
	if( RCacheCur->NetworkAddress && RCacheCur->Netmask && 
	    RCacheCur->Router && RCacheCur->Router->Address ) {
	    TI_DbgPrint(MAX_TRACE, ("%d: NA %08x NM %08x GW %08x MT %d\n",
				    RtCurrent - RouteEntries,
				    RCacheCur->NetworkAddress->Address,
				    RCacheCur->Netmask->Address,
				    RCacheCur->Router->Address->Address,
				    RCacheCur->Metric));
	    
	    RtlCopyMemory( &RtCurrent->Dest, 
			   &RCacheCur->NetworkAddress->Address,
			   sizeof(RtCurrent->Dest) );
	    RtlCopyMemory( &RtCurrent->Mask,
			   &RCacheCur->Netmask->Address,
			   sizeof(RtCurrent->Mask) );
	    /* Currently, this address is stuffed into the pointer.
	     * That probably is not intended. */
	    RtlCopyMemory( &RtCurrent->Gw,
			   &RCacheCur->Router->Address->Address,
			   sizeof(RtCurrent->Gw) );
	    RtCurrent->Metric1 = RCacheCur->Metric;
	    RtCurrent->Type = 2 /* PF_INET */;
	    
	    KeAcquireSpinLock(&EntityListLock, &OldIrql);
	    for( RtCurrent->Index = EntityCount - 1; 
		 RtCurrent->Index >= 0 &&
		     RCacheCur->Router->Interface != 
		     EntityList[RtCurrent->Index].context;
		 RtCurrent->Index-- );
	    RtCurrent->Index = EntityList[RtCurrent->Index].tei_instance;
	    KeReleaseSpinLock(&EntityListLock, OldIrql);
	} else {
	    TI_DbgPrint(MAX_TRACE, ("%d: BAD: NA %08x NM %08x GW %08x MT %d\n",
				    RtCurrent - RouteEntries,
				    RCacheCur->NetworkAddress,
				    RCacheCur->Netmask,
				    RCacheCur->Router,
				    RCacheCur->Router ? 
				    RCacheCur->Router->Address : 0,
				    RCacheCur->Metric));
	}
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
