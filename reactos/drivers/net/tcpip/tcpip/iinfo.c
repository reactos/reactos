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

TDI_STATUS InfoTdiQueryGetInterfaceMIB(TDIEntityID *ID,
				       PIP_INTERFACE Interface,
				       PNDIS_BUFFER Buffer,
				       PUINT BufferSize) {
    TDI_STATUS Status = TDI_INVALID_REQUEST;
    PIFENTRY OutData;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    PCHAR IFDescr;
    ULONG Size;
    UINT DescrLenMax = MAX_IFDESCR_LEN - 1;

    TI_DbgPrint(MAX_TRACE, 
		("Getting IFEntry MIB (IF %08x LA %08x) (%04x:%d)\n",
		 Interface, IF, ID->tei_entity, ID->tei_instance));

    OutData = 
	(PIFENTRY)ExAllocatePool( NonPagedPool, 
				  sizeof(IFENTRY) + MAX_IFDESCR_LEN );
    
    if( !OutData ) return TDI_INVALID_REQUEST; /* Out of memory */

    RtlZeroMemory( OutData, sizeof(IFENTRY) + MAX_IFDESCR_LEN );

    OutData->Index = Interface->Index;
    /* viz: tcpip keeps those indices */
    OutData->Type = Interface == 
        Loopback ? MIB_IF_TYPE_LOOPBACK : MIB_IF_TYPE_ETHERNET;
    OutData->Mtu = Interface->MTU;
    TI_DbgPrint(MAX_TRACE, 
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
	TI_DbgPrint(MAX_TRACE,
		    ("IF Speed = %d * 100bps\n", OutData->Speed));
	memcpy(OutData->PhysAddr,Interface->Address,Interface->AddressLength);
	TI_DbgPrint(MAX_TRACE, ("Got HWAddr\n"));
	GetInterfaceName( Interface, IFDescr, MAX_IFDESCR_LEN - 1 );
	DescrLenMax = strlen( IFDescr ) + 1;
    }

    IFDescr[DescrLenMax] = 0; /* Terminate ifdescr string */

    TI_DbgPrint(MAX_TRACE, ("Copied in name %s\n", IFDescr));
    OutData->DescrLen = DescrLenMax;
    IFDescr += DescrLenMax;
    Size = IFDescr - (PCHAR)OutData + 1;

    TI_DbgPrint(MAX_TRACE, ("Finished IFEntry MIB (%04x:%d) size %d\n",
			    ID->tei_entity, ID->tei_instance, Size));

    Status = InfoCopyOut( (PCHAR)OutData, Size, Buffer, BufferSize );
    ExFreePool( OutData );

    return Status;
}
    
TDI_STATUS InfoInterfaceTdiQueryEx( UINT InfoClass,
				    UINT InfoType,
				    UINT InfoId,
				    PVOID Context,
				    TDIEntityID *id,
				    PNDIS_BUFFER Buffer,
				    PUINT BufferSize ) {
    if( InfoClass == INFO_CLASS_GENERIC &&
	InfoType == INFO_TYPE_PROVIDER &&
	InfoId == ENTITY_TYPE_ID ) {
	ULONG Temp = IF_MIB;
	return InfoCopyOut( (PCHAR)&Temp, sizeof(Temp), Buffer, BufferSize );
    } else if( InfoClass == INFO_CLASS_PROTOCOL && 
	       InfoType == INFO_TYPE_PROVIDER &&
	       InfoId == IF_MIB_STATS_ID ) {
	return InfoTdiQueryGetInterfaceMIB( id, Context, Buffer, BufferSize );
    } else 
	return TDI_INVALID_REQUEST;
}

TDI_STATUS InfoInterfaceTdiSetEx( UINT InfoClass,
				  UINT InfoType,
				  UINT InfoId,
				  PVOID Context,
				  TDIEntityID *id,
				  PCHAR Buffer,
				  UINT BufferSize ) {
    TI_DbgPrint(MAX_TRACE, ("Got Request: Class %x Type %x Id %x, EntityID %x:%x\n",
                InfoClass, InfoId, id->tei_entity, id->tei_instance));
    return TDI_INVALID_REQUEST;
}
