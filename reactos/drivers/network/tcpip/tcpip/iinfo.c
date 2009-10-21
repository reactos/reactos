/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/iinfo.c
 * PURPOSE:     Per-interface information.
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

TDI_STATUS InfoTdiQueryGetInterfaceMIB(TDIEntityID ID,
				       PIP_INTERFACE Interface,
				       PNDIS_BUFFER Buffer,
				       PUINT BufferSize) {
    TDI_STATUS Status = TDI_INVALID_REQUEST;
    PIFENTRY OutData;
    PLAN_ADAPTER IF;
    PCHAR IFDescr;
    ULONG Size;
    UINT DescrLenMax = MAX_IFDESCR_LEN - 1;
    NDIS_STATUS NdisStatus;

    if (!Interface)
        return TDI_INVALID_PARAMETER;

    IF = (PLAN_ADAPTER)Interface->Context;

    TI_DbgPrint(DEBUG_INFO,
		("Getting IFEntry MIB (IF %08x LA %08x) (%04x:%d)\n",
		 Interface, IF, ID.tei_entity, ID.tei_instance));

    OutData =
	(PIFENTRY)exAllocatePool( NonPagedPool,
				  sizeof(IFENTRY) + MAX_IFDESCR_LEN );

    if( !OutData ) return TDI_NO_RESOURCES; /* Out of memory */

    RtlZeroMemory( OutData, sizeof(IFENTRY) + MAX_IFDESCR_LEN );

    OutData->Index = Interface->Index;
    /* viz: tcpip keeps those indices */
    OutData->Type = Interface ==
        Loopback ? MIB_IF_TYPE_LOOPBACK : MIB_IF_TYPE_ETHERNET;
    OutData->Mtu = Interface->MTU;
    TI_DbgPrint(DEBUG_INFO,
		("Getting interface speed\n"));
    OutData->PhysAddrLen = Interface->AddressLength;
    OutData->AdminStatus = MIB_IF_ADMIN_STATUS_UP;
    /* NDIS_HARDWARE_STATUS -> ROUTER_CONNECTION_STATE */
    Status = GetInterfaceConnectionStatus( Interface, &OutData->OperStatus );

    /* Not sure what to do here, but not ready seems a safe bet on failure */
    if( !NT_SUCCESS(Status) )
        OutData->OperStatus = NdisHardwareStatusNotReady;

    IFDescr = (PCHAR)&OutData[1];

    if( IF ) {
	GetInterfaceSpeed( Interface, (PUINT)&OutData->Speed );
	TI_DbgPrint(DEBUG_INFO,
		    ("IF Speed = %d * 100bps\n", OutData->Speed));
	memcpy(OutData->PhysAddr,Interface->Address,Interface->AddressLength);
	TI_DbgPrint(DEBUG_INFO, ("Got HWAddr\n"));

        memcpy(&OutData->InOctets, &Interface->Stats, sizeof(SEND_RECV_STATS));

        NdisStatus = NDISCall(IF,
                              NdisRequestQueryInformation,
                              OID_GEN_XMIT_ERROR,
                              &OutData->OutErrors,
                              sizeof(ULONG));
        if (NdisStatus != NDIS_STATUS_SUCCESS)
            OutData->OutErrors = 0;

        TI_DbgPrint(DEBUG_INFO, ("OutErrors = %d\n", OutData->OutErrors));

        NdisStatus = NDISCall(IF,
                              NdisRequestQueryInformation,
                              OID_GEN_RCV_ERROR,
                              &OutData->InErrors,
                              sizeof(ULONG));
        if (NdisStatus != NDIS_STATUS_SUCCESS)
            OutData->InErrors = 0;

        TI_DbgPrint(DEBUG_INFO, ("InErrors = %d\n", OutData->InErrors));
    }

    GetInterfaceName( Interface, IFDescr, MAX_IFDESCR_LEN - 1 );
    DescrLenMax = strlen( IFDescr ) + 1;

    TI_DbgPrint(DEBUG_INFO, ("Copied in name %s\n", IFDescr));
    OutData->DescrLen = DescrLenMax;
    IFDescr += DescrLenMax;
    Size = IFDescr - (PCHAR)OutData + 1;

    TI_DbgPrint(DEBUG_INFO, ("Finished IFEntry MIB (%04x:%d) size %d\n",
			    ID.tei_entity, ID.tei_instance, Size));

    Status = InfoCopyOut( (PCHAR)OutData, Size, Buffer, BufferSize );
    exFreePool( OutData );

    TI_DbgPrint(DEBUG_INFO,("Returning %x\n", Status));

    return Status;
}

TDI_STATUS InfoTdiQueryGetArptableMIB(TDIEntityID ID,
				      PIP_INTERFACE Interface,
				      PNDIS_BUFFER Buffer,
				      PUINT BufferSize) {
    NTSTATUS Status;
    ULONG NumNeighbors = NBCopyNeighbors( Interface, NULL );
    ULONG MemSize = NumNeighbors * sizeof(IPARP_ENTRY);
    PIPARP_ENTRY ArpEntries;

    if (MemSize != 0)
    {
        ArpEntries = exAllocatePoolWithTag( NonPagedPool, MemSize, FOURCC('A','R','P','t') );
        if( !ArpEntries ) return STATUS_NO_MEMORY;

        NBCopyNeighbors( Interface, ArpEntries );

        Status = InfoCopyOut( (PVOID)ArpEntries, MemSize, Buffer, BufferSize );

        exFreePool( ArpEntries );
    }
    else
    {
        Status = InfoCopyOut(NULL, 0, NULL, BufferSize);
    }

    return Status;
}

VOID InsertTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    KIRQL OldIrql;
    UINT IFCount = 0, CLNLCount = 0, CLTLCount = 0, COTLCount = 0, ATCount = 0, ERCount = 0, i;

    TI_DbgPrint(DEBUG_INFO,
		("Inserting interface %08x (%d entities already)\n",
		 Interface, EntityCount));

    TcpipAcquireSpinLock( &EntityListLock, &OldIrql );

    /* Count IP Entities */
    for( i = 0; i < EntityCount; i++ )
	switch( EntityList[i].tei_entity )
        {
           case IF_ENTITY:
              IFCount++;
              break;

           case CL_NL_ENTITY:
              CLNLCount++;
              break;

           case CL_TL_ENTITY:
              CLTLCount++;
              break;

           case CO_TL_ENTITY:
              COTLCount++;
              break;

           case AT_ENTITY:
              ATCount++;
              break;

           case ER_ENTITY:
              ERCount++;
              break;

           default:
              break;
       }

    EntityList[EntityCount].tei_entity   = IF_ENTITY;
    EntityList[EntityCount].tei_instance = IFCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = IF_MIB;
    EntityCount++;
    EntityList[EntityCount].tei_entity   = CL_NL_ENTITY;
    EntityList[EntityCount].tei_instance = CLNLCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = CL_NL_IP;
    EntityCount++;
    EntityList[EntityCount].tei_entity   = CL_TL_ENTITY;
    EntityList[EntityCount].tei_instance = CLTLCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = CL_TL_UDP;
    EntityCount++;
    EntityList[EntityCount].tei_entity   = CO_TL_ENTITY;
    EntityList[EntityCount].tei_instance = COTLCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = CO_TL_TCP;
    EntityCount++;
    EntityList[EntityCount].tei_entity   = ER_ENTITY;
    EntityList[EntityCount].tei_instance = ERCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = ER_ICMP;
    EntityCount++;
    EntityList[EntityCount].tei_entity   = AT_ENTITY;
    EntityList[EntityCount].tei_instance = ATCount;
    EntityList[EntityCount].context      = Interface;
    EntityList[EntityCount].flags        = (Interface != Loopback) ? AT_ARP : AT_NULL;
    EntityCount++;

    TcpipReleaseSpinLock( &EntityListLock, OldIrql );
}

VOID RemoveTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    KIRQL OldIrql;
    UINT i;

    TI_DbgPrint(DEBUG_INFO,("Removing TDI entry 0x%x\n", Interface));

    TcpipAcquireSpinLock( &EntityListLock, &OldIrql );

    /* Remove entities that have this interface as context
     * In the future, this might include AT_ENTITY types, too
     */
    for( i = 0; i < EntityCount; i++ ) {
	TI_DbgPrint(DEBUG_INFO,("--> examining TDI entry 0x%x\n", EntityList[i].context));
	if( EntityList[i].context == Interface ) {
	    if( i != EntityCount-1 ) {
		memcpy( &EntityList[i],
			&EntityList[--EntityCount],
			sizeof(EntityList[i]) );
	    } else {
		EntityCount--;
	    }
	}
    }

    TcpipReleaseSpinLock( &EntityListLock, OldIrql );
}

