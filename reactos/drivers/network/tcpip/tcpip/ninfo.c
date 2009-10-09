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



/* Get IPRouteEntry s for each of the routes in the system */
TDI_STATUS InfoTdiQueryGetRouteTable( PNDIS_BUFFER Buffer, PUINT BufferSize ) {
    TDI_STATUS Status;
    KIRQL OldIrql;
    UINT RtCount = CountFIBs();
    UINT Size = sizeof( IPROUTE_ENTRY ) * RtCount;
    PFIB_ENTRY RCache =
	exAllocatePool( NonPagedPool, sizeof( FIB_ENTRY ) * RtCount ),
	RCacheCur = RCache;
    PIPROUTE_ENTRY RouteEntries = exAllocatePool( NonPagedPool, Size ),
	RtCurrent = RouteEntries;

    TI_DbgPrint(DEBUG_INFO, ("Called, routes = %d, RCache = %08x\n",
			    RtCount, RCache));

    if( !RCache || !RouteEntries ) {
	if( RCache ) exFreePool( RCache );
	if( RouteEntries ) exFreePool( RouteEntries );
	return TDI_NO_RESOURCES;
    }

    RtlZeroMemory( RouteEntries, Size );

    RtCount = CopyFIBs( RCache );

    while( RtCurrent < RouteEntries + RtCount ) {
	ASSERT(RCacheCur->Router);

	RtlCopyMemory( &RtCurrent->Dest,
		       &RCacheCur->NetworkAddress.Address,
		       sizeof(RtCurrent->Dest) );
	RtlCopyMemory( &RtCurrent->Mask,
		       &RCacheCur->Netmask.Address,
		       sizeof(RtCurrent->Mask) );
	RtlCopyMemory( &RtCurrent->Gw,
		       &RCacheCur->Router->Address.Address,
		       sizeof(RtCurrent->Gw) );

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

    exFreePool( RouteEntries );
    exFreePool( RCache );

    TI_DbgPrint(DEBUG_INFO, ("Returning %08x\n", Status));

    return Status;
}

TDI_STATUS InfoTdiQueryGetAddrTable(TDIEntityID ID,
				    PNDIS_BUFFER Buffer,
				    PUINT BufferSize)
{
    KIRQL OldIrql;
    PIPADDR_ENTRY IPEntry;
    PIP_INTERFACE CurrentIF;
    UINT i;

    TI_DbgPrint(DEBUG_INFO, ("Called.\n"));


    TcpipAcquireSpinLock(&EntityListLock, &OldIrql);

    for (i = 0; i < EntityCount; i++)
    {
        if (EntityList[i].tei_entity == ID.tei_entity &&
            EntityList[i].tei_instance == ID.tei_instance)
            break;
    }

    if (i == EntityCount)
    {
        TcpipReleaseSpinLock(&EntityListLock, OldIrql);
        return TDI_INVALID_PARAMETER;
    }

    IPEntry = exAllocatePool(NonPagedPool, sizeof(IPADDR_ENTRY));
    if (!IPEntry)
    {
        TcpipReleaseSpinLock(&EntityListLock, OldIrql);
        return TDI_NO_RESOURCES;
    }

    CurrentIF = EntityList[i].context;

    IPEntry->Index = CurrentIF->Index;
    GetInterfaceIPv4Address(CurrentIF,
			    ADE_UNICAST,
			    &IPEntry->Addr);
    GetInterfaceIPv4Address(CurrentIF,
			    ADE_ADDRMASK,
			    &IPEntry->Mask);
    GetInterfaceIPv4Address(CurrentIF,
			    ADE_BROADCAST,
			    &IPEntry->BcastAddr);

    TcpipReleaseSpinLock(&EntityListLock, OldIrql);

    InfoCopyOut((PCHAR)IPEntry, sizeof(IPADDR_ENTRY),
		Buffer, BufferSize);

    exFreePool(IPEntry);

    return TDI_SUCCESS;
}

TDI_STATUS InfoTdiQueryGetIPSnmpInfo( TDIEntityID ID,
				      PNDIS_BUFFER Buffer,
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

TDI_STATUS InfoTdiSetRoute(PIPROUTE_ENTRY Route)
{
    IP_ADDRESS Address, Netmask, Router;
    PNEIGHBOR_CACHE_ENTRY NCE;

    AddrInitIPv4( &Address, Route->Dest );
    AddrInitIPv4( &Netmask, Route->Mask );
    AddrInitIPv4( &Router,  Route->Gw );

    if( Route->Type == IP_ROUTE_TYPE_ADD ) { /* Add the route */
        TI_DbgPrint(DEBUG_INFO,("Adding route (%s)\n", A2S(&Address)));
        /* Find the existing route this belongs to */
        NCE = RouterGetRoute( &Router );
        if (!NCE) return TDI_INVALID_PARAMETER;

        /* Really add the route */
	if (!RouterCreateRoute( &Address, &Netmask, &Router,
			       NCE->Interface, Route->Metric1))
	    return TDI_NO_RESOURCES;

        return TDI_SUCCESS;
     } else if( Route->Type == IP_ROUTE_TYPE_DEL ) {
	TI_DbgPrint(DEBUG_INFO,("Removing route (%s)\n", A2S(&Address)));
	if (NT_SUCCESS(RouterRemoveRoute( &Address, &Router )))
            return TDI_SUCCESS;
        else
            return TDI_INVALID_PARAMETER;
     }

     return TDI_INVALID_REQUEST;
}

