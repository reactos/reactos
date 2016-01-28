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

#include <ipifcons.h>

/* See iptypes.h */
#define MAX_ADAPTER_DESCRIPTION_LENGTH 128

TDI_STATUS InfoTdiQueryGetInterfaceMIB(TDIEntityID ID,
				       PIP_INTERFACE Interface,
				       PNDIS_BUFFER Buffer,
				       PUINT BufferSize) {
    TDI_STATUS Status = TDI_INVALID_REQUEST;
    IFEntry* OutData;
    PLAN_ADAPTER IF;
    PCHAR IFDescr;
    ULONG Size;
    NDIS_STATUS NdisStatus;

    if (!Interface)
        return TDI_INVALID_PARAMETER;

    IF = (PLAN_ADAPTER)Interface->Context;

    TI_DbgPrint(DEBUG_INFO,
		("Getting IFEntry MIB (IF %08x LA %08x) (%04x:%d)\n",
		 Interface, IF, ID.tei_entity, ID.tei_instance));

    OutData = ExAllocatePool( NonPagedPool, FIELD_OFFSET(IFEntry, if_descr[MAX_ADAPTER_DESCRIPTION_LENGTH + 1]));

    if( !OutData ) return TDI_NO_RESOURCES; /* Out of memory */

    RtlZeroMemory( OutData, FIELD_OFFSET(IFEntry, if_descr[MAX_ADAPTER_DESCRIPTION_LENGTH + 1]));

    OutData->if_index = Interface->Index;
    /* viz: tcpip keeps those indices */
    OutData->if_type = Interface ==
        Loopback ? MIB_IF_TYPE_LOOPBACK : MIB_IF_TYPE_ETHERNET;
    OutData->if_mtu = Interface->MTU;
    TI_DbgPrint(DEBUG_INFO,
		("Getting interface speed\n"));
    OutData->if_physaddrlen = Interface->AddressLength;
    OutData->if_adminstatus = MIB_IF_ADMIN_STATUS_UP;
    /* NDIS_HARDWARE_STATUS -> ROUTER_CONNECTION_STATE */
    GetInterfaceConnectionStatus( Interface, &OutData->if_operstatus );

    IFDescr = (PCHAR)&OutData->if_descr[0];

    if( IF ) {
	GetInterfaceSpeed( Interface, (PUINT)&OutData->if_speed );
	TI_DbgPrint(DEBUG_INFO,
		    ("IF Speed = %d * 100bps\n", OutData->if_speed));
	memcpy(OutData->if_physaddr, Interface->Address, Interface->AddressLength);
	TI_DbgPrint(DEBUG_INFO, ("Got HWAddr\n"));

        memcpy(&OutData->if_inoctets, &Interface->Stats, sizeof(SEND_RECV_STATS));

        NdisStatus = NDISCall(IF,
                              NdisRequestQueryInformation,
                              OID_GEN_XMIT_ERROR,
                              &OutData->if_outerrors,
                              sizeof(ULONG));
        if (NdisStatus != NDIS_STATUS_SUCCESS)
            OutData->if_outerrors = 0;

        TI_DbgPrint(DEBUG_INFO, ("OutErrors = %d\n", OutData->if_outerrors));

        NdisStatus = NDISCall(IF,
                              NdisRequestQueryInformation,
                              OID_GEN_RCV_ERROR,
                              &OutData->if_inerrors,
                              sizeof(ULONG));
        if (NdisStatus != NDIS_STATUS_SUCCESS)
            OutData->if_inerrors = 0;

        TI_DbgPrint(DEBUG_INFO, ("InErrors = %d\n", OutData->if_inerrors));
    }

    GetInterfaceName( Interface, IFDescr, MAX_ADAPTER_DESCRIPTION_LENGTH );

    TI_DbgPrint(DEBUG_INFO, ("Copied in name %s\n", IFDescr));
    OutData->if_descrlen = strlen(IFDescr);
    Size = FIELD_OFFSET(IFEntry, if_descr[OutData->if_descrlen + 1]);

    TI_DbgPrint(DEBUG_INFO, ("Finished IFEntry MIB (%04x:%d) size %d\n",
			    ID.tei_entity, ID.tei_instance, Size));

    Status = InfoCopyOut( (PCHAR)OutData, Size, Buffer, BufferSize );
    ExFreePool( OutData );

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
        ArpEntries = ExAllocatePool( NonPagedPool, MemSize );
        if( !ArpEntries ) return STATUS_NO_MEMORY;

        NBCopyNeighbors( Interface, ArpEntries );

        Status = InfoCopyOut( (PVOID)ArpEntries, MemSize, Buffer, BufferSize );

        ExFreePool( ArpEntries );
    }
    else
    {
        Status = InfoCopyOut(NULL, 0, NULL, BufferSize);
    }

    return Status;
}

TDI_STATUS InfoTdiSetArptableMIB(PIP_INTERFACE IF, PVOID Buffer, UINT BufferSize)
{
    PIPARP_ENTRY ArpEntry = Buffer;
    IP_ADDRESS Address;
    PNEIGHBOR_CACHE_ENTRY NCE;

    if (!Buffer || BufferSize < sizeof(IPARP_ENTRY))
        return TDI_INVALID_PARAMETER;

    AddrInitIPv4(&Address, ArpEntry->LogAddr);

    if ((NCE = NBLocateNeighbor(&Address, IF)))
        NBRemoveNeighbor(NCE);
     
    if (NBAddNeighbor(IF,
                      &Address,
                      ArpEntry->PhysAddr,
                      ArpEntry->AddrSize,
                      NUD_PERMANENT,
                      0))
        return TDI_SUCCESS;
    else
        return TDI_INVALID_PARAMETER;
}

VOID InsertTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    AddEntity(IF_ENTITY, Interface, IF_MIB);

    AddEntity(AT_ENTITY, Interface,
              (Interface != Loopback) ? AT_ARP : AT_NULL);

    /* FIXME: This is probably wrong */
    AddEntity(CL_NL_ENTITY, Interface, CL_NL_IP);
}

VOID RemoveTDIInterfaceEntity( PIP_INTERFACE Interface ) {
    /* This removes all of them */
    RemoveEntityByContext(Interface);
}
