/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/lan.c
 * PURPOSE:     Local Area Network media routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   arty -- Separate service 09/2004
 */

#include "precomp.h"

ULONG DebugTraceLevel = 0x7ffffff;
PDEVICE_OBJECT LanDeviceObject  = NULL;

NDIS_STATUS NDISCall(
    PLAN_ADAPTER Adapter,
    NDIS_REQUEST_TYPE Type,
    NDIS_OID OID,
    PVOID Buffer,
    UINT Length)
/*
 * FUNCTION: Send a request to NDIS
 * ARGUMENTS:
 *     Adapter     = Pointer to a LAN_ADAPTER structure
 *     Type        = Type of request (Set or Query)
 *     OID         = Value to be set/queried for
 *     Buffer      = Pointer to a buffer to use
 *     Length      = Number of bytes in Buffer
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_REQUEST Request;
    NDIS_STATUS NdisStatus;

    Request.RequestType = Type;
    if (Type == NdisRequestSetInformation) {
        Request.DATA.SET_INFORMATION.Oid                     = OID;
        Request.DATA.SET_INFORMATION.InformationBuffer       = Buffer;
        Request.DATA.SET_INFORMATION.InformationBufferLength = Length;
    } else {
        Request.DATA.QUERY_INFORMATION.Oid                     = OID;
        Request.DATA.QUERY_INFORMATION.InformationBuffer       = Buffer;
        Request.DATA.QUERY_INFORMATION.InformationBufferLength = Length;
    }

    if (Adapter->State != LAN_STATE_RESETTING) {
        NdisRequest(&NdisStatus, Adapter->NdisHandle, &Request);
    } else {
        NdisStatus = NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Wait for NDIS to complete the request */
    if (NdisStatus == NDIS_STATUS_PENDING) {
        KeWaitForSingleObject(&Adapter->Event,
                              UserRequest,
                              KernelMode,
                              FALSE,
                              NULL);
        NdisStatus = Adapter->NdisStatus;
    }

    return NdisStatus;
}


VOID FreeAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Frees memory for a LAN_ADAPTER structure
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure to free
 */
{
    exFreePool(Adapter);
}


VOID STDCALL ProtocolOpenAdapterComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status,
    NDIS_STATUS OpenErrorStatus)
/*
 * FUNCTION: Called by NDIS to complete opening of an adapter
 * ARGUMENTS:
 *     BindingContext  = Pointer to a device context (LAN_ADAPTER)
 *     Status          = Status of the operation
 *     OpenErrorStatus = Additional status information
 */
{
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    KeSetEvent(&Adapter->Event, 0, FALSE);
}


VOID STDCALL ProtocolCloseAdapterComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status)
/*
 * FUNCTION: Called by NDIS to complete closing an adapter
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 *     Status         = Status of the operation
 */
{
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    Adapter->NdisStatus = Status;

    KeSetEvent(&Adapter->Event, 0, FALSE);
}


VOID STDCALL ProtocolResetComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status)
/*
 * FUNCTION: Called by NDIS to complete resetting an adapter
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 *     Status         = Status of the operation
 */
{
    LA_DbgPrint(MID_TRACE, ("Called.\n"));
}


VOID STDCALL ProtocolRequestComplete(
    NDIS_HANDLE BindingContext,
    PNDIS_REQUEST NdisRequest,
    NDIS_STATUS Status)
/*
 * FUNCTION: Called by NDIS to complete a request
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 *     NdisRequest    = Pointer to an object describing the request
 *     Status         = Status of the operation
 */
{
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    /* Save status of request and signal an event */
    Adapter->NdisStatus = Status;

    KeSetEvent(&Adapter->Event, 0, FALSE);
}


VOID STDCALL ProtocolSendComplete(
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET Packet,
    NDIS_STATUS Status)
/*
 * FUNCTION: Called by NDIS to complete sending process
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 *     Packet         = Pointer to a packet descriptor
 *     Status         = Status of the operation
 */
{
    /*PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;*/

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
    /*(*PC(Packet)->DLComplete)(Adapter->Context, Packet, Status);*/
    LA_DbgPrint(DEBUG_DATALINK, ("Finished\n"));
}


VOID STDCALL ProtocolTransferDataComplete(
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET Packet,
    NDIS_STATUS Status,
    UINT BytesTransferred)
/*
 * FUNCTION: Called by NDIS to complete reception of data
 * ARGUMENTS:
 *     BindingContext   = Pointer to a device context (LAN_ADAPTER)
 *     Packet           = Pointer to a packet descriptor
 *     Status           = Status of the operation
 *     BytesTransferred = Number of bytes transferred
 * NOTES:
 *     If the packet was successfully received, determine the protocol
 *     type and pass it to the correct receive handler
 */
{
    PLIST_ENTRY ListEntry, ReadListEntry;
    PLAN_PROTOCOL Proto;
    PLAN_PACKET_HEADER Header;
    PLAN_DEVICE_EXT DeviceExt = LanDeviceObject->DeviceExtension;
    UINT i;
    UINT PacketType;
    UINT ContigSize;
    PIRP ReadIrp;
    KIRQL OldIrql;
    LAN_PACKET LPPacket;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    if (Status == NDIS_STATUS_SUCCESS) {
        PNDIS_BUFFER NdisBuffer;

        NdisGetFirstBufferFromPacket(Packet,
                                     &NdisBuffer,
                                     &LPPacket.EthHeader,
                                     &ContigSize,
                                     &LPPacket.TotalSize);

	LPPacket.TotalSize = BytesTransferred;

        /* Determine which upper layer protocol that should receive
           this packet and pass it to the correct receive handler */

        /*OskitDumpBuffer( IPPacket.Header, BytesTransferred );*/

        PacketType = LPPacket.EthHeader->EType;

	LA_DbgPrint
	    (DEBUG_DATALINK,
	     ("Ether Type = %x Total = %d Packet %x Payload %x\n",
	      PacketType, LPPacket.TotalSize, LPPacket.EthHeader,
	      LPPacket.EthHeader + 1));

	NdisBuffer->Next = NULL;

	for( ListEntry = DeviceExt->ProtocolListHead.Flink;
	     ListEntry != &DeviceExt->ProtocolListHead;
	     ListEntry = ListEntry->Flink ) {
	    Proto = CONTAINING_RECORD(ListEntry, LAN_PROTOCOL, ListEntry);
	    LA_DbgPrint(MID_TRACE,("Examining protocol %x\n", Proto));
	    for( i = 0; i < Proto->NumEtherTypes; i++ ) {
		LA_DbgPrint(MID_TRACE,(".Accepts proto %x\n",
				       Proto->EtherType[i]));
		if( Proto->EtherType[i] == PacketType &&
		    !IsListEmpty( &Proto->ReadIrpListHead ) ) {
		    ReadListEntry = RemoveHeadList( &Proto->ReadIrpListHead );
		    ReadIrp = CONTAINING_RECORD(ReadListEntry, IRP,
						Tail.Overlay.ListEntry );
		    LA_DbgPrint(MID_TRACE,("..Irp %x\n", ReadIrp));
		    _SEH_TRY {
			Header = ReadIrp->AssociatedIrp.SystemBuffer;
			LA_DbgPrint
			    (MID_TRACE,
			     ("Writing packet at %x\n", Header));
			Header->Fixed.Adapter = Adapter->Index;
			Header->Fixed.AddressType = Adapter->Media;
			Header->Fixed.AddressLen = IEEE_802_ADDR_LENGTH;
			Header->Fixed.PacketType = PacketType;
			RtlCopyMemory( Header->Address,
				       LPPacket.EthHeader->SrcAddr,
				       IEEE_802_ADDR_LENGTH );
			if( Proto->Buffered ) {
			    LA_DbgPrint(MID_TRACE,("Buffered copy\n"));
			    RtlCopyMemory
				( Header->Address +
				  IEEE_802_ADDR_LENGTH,
				  LPPacket.EthHeader + 1,
				  LPPacket.TotalSize -
				  sizeof(*LPPacket.EthHeader) );
			    Header->Fixed.Mdl = NULL;
			} else
			    Header->Fixed.Mdl = NdisBuffer;

			ReadIrp->IoStatus.Status = 0;
			ReadIrp->IoStatus.Information =
			    (Header->Address + IEEE_802_ADDR_LENGTH +
			     LPPacket.TotalSize -
			     sizeof(*LPPacket.EthHeader)) -
			    (PCHAR)Header;

			LA_DbgPrint(MID_TRACE,("Bytes returned %d\n",
					       ReadIrp->IoStatus.Information));

			IoCompleteRequest( ReadIrp, IO_NETWORK_INCREMENT );
		    } _SEH_HANDLE {
			LA_DbgPrint
			    (MIN_TRACE,
			     ("Failed write to packet in client\n"));
			ReadIrp->IoStatus.Status = STATUS_ACCESS_VIOLATION;
			ReadIrp->IoStatus.Information = 0;
			IoCompleteRequest( ReadIrp, IO_NETWORK_INCREMENT );
		    } _SEH_END;
		    break;
		}
	    }
	}
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    FreeNdisPacket( Packet );
}


NDIS_STATUS STDCALL ProtocolReceive(
    NDIS_HANDLE BindingContext,
    NDIS_HANDLE MacReceiveContext,
    PVOID HeaderBuffer,
    UINT HeaderBufferSize,
    PVOID LookaheadBuffer,
    UINT LookaheadBufferSize,
    UINT PacketSize)
/*
 * FUNCTION: Called by NDIS when a packet has been received on the physical link
 * ARGUMENTS:
 *     BindingContext      = Pointer to a device context (LAN_ADAPTER)
 *     MacReceiveContext   = Handle used by underlying NIC driver
 *     HeaderBuffer        = Pointer to a buffer containing the packet header
 *     HeaderBufferSize    = Number of bytes in HeaderBuffer
 *     LookaheadBuffer     = Pointer to a buffer containing buffered packet data
 *     LookaheadBufferSize = Size of LookaheadBuffer. May be less than asked for
 *     PacketSize          = Overall size of the packet (not including header)
 * RETURNS:
 *     Status of operation
 */
{
    USHORT EType;
    UINT PacketType, BytesTransferred;
    PCHAR BufferData;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    LA_DbgPrint(DEBUG_DATALINK, ("Called. (packetsize %d)\n",PacketSize));

    if (Adapter->State != LAN_STATE_STARTED) {
        LA_DbgPrint(DEBUG_DATALINK, ("Adapter is stopped.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    if (HeaderBufferSize < Adapter->HeaderSize) {
        LA_DbgPrint(DEBUG_DATALINK, ("Runt frame received.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    PacketType = EType;

    /* Get a transfer data packet */
    KeAcquireSpinLockAtDpcLevel(&Adapter->Lock);
    NdisStatus = AllocatePacketWithBuffer( &NdisPacket, NULL, Adapter->MTU );
    if( NdisStatus != NDIS_STATUS_SUCCESS ) {
        KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);
        return NDIS_STATUS_NOT_ACCEPTED;
    }
    LA_DbgPrint(DEBUG_DATALINK, ("pretransfer LookaheadBufferSize %d packsize %d\n",LookaheadBufferSize,PacketSize));
    {
	UINT temp;
	temp = PacketSize;
	GetDataPtr( NdisPacket, 0, &BufferData, &temp );
    }

    LA_DbgPrint(DEBUG_DATALINK, ("pretransfer LookaheadBufferSize %d HeaderBufferSize %d packsize %d\n",LookaheadBufferSize,HeaderBufferSize,PacketSize));
    /* Get the data */
    NdisTransferData(&NdisStatus,
		     Adapter->NdisHandle,
		     MacReceiveContext,
		     0,
		     PacketSize + HeaderBufferSize,
		     NdisPacket,
		     &BytesTransferred);

    LA_DbgPrint(DEBUG_DATALINK, ("Calling complete\n"));

    if (NdisStatus != NDIS_STATUS_PENDING)
	ProtocolTransferDataComplete(BindingContext,
				     NdisPacket,
				     NdisStatus,
				     PacketSize + HeaderBufferSize);

    /* Release the packet descriptor */
    KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);
    LA_DbgPrint(DEBUG_DATALINK, ("leaving\n"));

    return NDIS_STATUS_SUCCESS;
}


VOID STDCALL ProtocolReceiveComplete(
    NDIS_HANDLE BindingContext)
/*
 * FUNCTION: Called by NDIS when we're done receiving data
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 */
{
    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}


VOID STDCALL ProtocolStatus(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS GenerelStatus,
    PVOID StatusBuffer,
    UINT StatusBufferSize)
/*
 * FUNCTION: Called by NDIS when the underlying driver has changed state
 * ARGUMENTS:
 *     BindingContext   = Pointer to a device context (LAN_ADAPTER)
 *     GenerelStatus    = A generel status code
 *     StatusBuffer     = Pointer to a buffer with medium-specific data
 *     StatusBufferSize = Number of bytes in StatusBuffer
 */
{
    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}


VOID STDCALL ProtocolStatusComplete(
    NDIS_HANDLE NdisBindingContext)
/*
 * FUNCTION: Called by NDIS when a status-change has occurred
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 */
{
    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}

VOID STDCALL ProtocolBindAdapter(
    OUT PNDIS_STATUS   Status,
    IN  NDIS_HANDLE    BindContext,
    IN  PNDIS_STRING   DeviceName,
    IN  PVOID          SystemSpecific1,
    IN  PVOID          SystemSpecific2)
/*
 * FUNCTION: Called by NDIS during NdisRegisterProtocol to set up initial
 *           bindings, and periodically thereafer as new adapters come online
 * ARGUMENTS:
 *     Status: Return value to NDIS
 *     BindContext: Handle provided by NDIS to track pending binding operations
 *     DeviceName: Name of the miniport device to bind to
 *     SystemSpecific1: Pointer to a registry path with protocol-specific configuration information
 *     SystemSpecific2: Unused & must not be touched
 */
{
    /* XXX confirm that this is still true, or re-word the following comment */
    /* we get to ignore BindContext because we will never pend an operation with NDIS */
    LA_DbgPrint(DEBUG_DATALINK, ("Called with registry path %wZ\n", SystemSpecific1));
    *Status = LANRegisterAdapter(DeviceName, SystemSpecific1);
}


VOID LANTransmit(
    PLAN_ADAPTER Adapter,
    PNDIS_PACKET NdisPacket,
    PVOID LinkAddress,
    USHORT Type)
/*
 * FUNCTION: Transmits a packet
 * ARGUMENTS:
 *     Context     = Pointer to context information (LAN_ADAPTER)
 *     NdisPacket  = Pointer to NDIS packet to send
 *     LinkAddress = Pointer to link address of destination (NULL = broadcast)
 *     Type        = LAN protocol type (LAN_PROTO_*)
 */
{
    NDIS_STATUS NdisStatus;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    if (Adapter->State == LAN_STATE_STARTED) {
        NdisSend(&NdisStatus, Adapter->NdisHandle, NdisPacket);
        if (NdisStatus != NDIS_STATUS_PENDING)
            ProtocolSendComplete((NDIS_HANDLE)Adapter, NdisPacket, NdisStatus);
    } else {
        ProtocolSendComplete((NDIS_HANDLE)Adapter, NdisPacket, NDIS_STATUS_CLOSED);
    }
}

/* For use internally */
UINT LANTransmitInternal(PLAN_PACKET_HEADER ToWrite, UINT OverallLength) {
    NDIS_STATUS NdisStatus;
    PLAN_DEVICE_EXT DeviceExt = LanDeviceObject->DeviceExtension;
    PLAN_ADAPTER Adapter;
    PETH_HEADER EthHeader;
    KIRQL OldIrql;
    PNDIS_PACKET NdisPacket;
    UINT Size, PayloadSize = OverallLength -
	((ToWrite->Address + ToWrite->Fixed.AddressLen) - (PCHAR)ToWrite);

    NdisStatus = AllocatePacketWithBuffer( &NdisPacket, NULL,
					   PayloadSize + sizeof(ETH_HEADER) );

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    if( !NT_SUCCESS(NdisStatus) ) goto end;

    Adapter = FindAdapterByIndex( DeviceExt, ToWrite->Fixed.Adapter );

    if( !Adapter ) goto end;

    GetDataPtr( NdisPacket, 0, (PCHAR *)&EthHeader, &Size );
    if( !EthHeader ) goto end;

    LA_DbgPrint(MID_TRACE,("Writing %d bytes of Dst\n",
			   ToWrite->Fixed.AddressLen));

    /* Handle broadcast for other media types here */
    if( ToWrite->Fixed.AddressLen )
	RtlCopyMemory( EthHeader->DstAddr,
		       ToWrite->Address,
		       ToWrite->Fixed.AddressLen );
    else
	memset( EthHeader->DstAddr, -1, sizeof(EthHeader->DstAddr) );

    LA_DbgPrint(MID_TRACE,("Writing %d bytes of Src\n", Adapter->HWAddressLength));
    RtlCopyMemory( EthHeader->SrcAddr,
		   Adapter->HWAddress,
		   Adapter->HWAddressLength );
    LA_DbgPrint(MID_TRACE,("Writing %d bytes of payload\n", PayloadSize));
    EthHeader->EType = ToWrite->Fixed.PacketType;
    RtlCopyMemory( EthHeader + 1,
		   ToWrite->Address + ToWrite->Fixed.AddressLen,
		   PayloadSize );

    LANTransmit( Adapter, NdisPacket, ToWrite->Address,
		 ToWrite->Fixed.PacketType );

end:
    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    return OverallLength;
}

VOID BindAdapter(PLAN_ADAPTER Adapter, PNDIS_STRING RegistryPath)
/*
 * FUNCTION: Binds a LAN adapter to IP layer
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure
 * NOTES:
 *    We set the lookahead buffer size, set the packet filter and
 *    bind the adapter to IP layer
 */
{
    /*NDIS_STATUS NdisStatus;*/
    /*ULONG Lookahead = LOOKAHEAD_SIZE;*/
    /*NTSTATUS Status;*/
    /*HANDLE RegHandle = 0;*/

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

}

NDIS_STATUS LANRegisterAdapter( PNDIS_STRING AdapterName,
				PNDIS_STRING RegistryPath)
/*
 * FUNCTION: Registers protocol with an NDIS adapter
 * ARGUMENTS:
 *     AdapterName = Pointer to string with name of adapter to register
 *     Adapter     = Address of pointer to a LAN_ADAPTER structure
 * RETURNS:
 *     Status of operation
 */
{
    PLAN_ADAPTER Adapter;
    NDIS_MEDIUM MediaArray[MAX_MEDIA];
    NDIS_STATUS NdisStatus;
    NDIS_STATUS OpenStatus;
    UINT MediaIndex;
    UINT AddressOID;
    UINT Speed;
    PLAN_DEVICE_EXT DeviceExt = LanDeviceObject->DeviceExtension;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    Adapter = exAllocatePool(NonPagedPool, sizeof(LAN_ADAPTER));
    if (!Adapter) {
        LA_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(Adapter, sizeof(LAN_ADAPTER));

    /* Put adapter in stopped state */
    Adapter->State = LAN_STATE_STOPPED;
    Adapter->Index = DeviceExt->AdapterId++;

    InitializeListHead( &Adapter->AddressList );
    InitializeListHead( &Adapter->ForeignList );

    /* Initialize protecting spin lock */
    KeInitializeSpinLock(&Adapter->Lock);

    KeInitializeEvent(&Adapter->Event, SynchronizationEvent, FALSE);

    /* Initialize array with media IDs we support */
    MediaArray[MEDIA_ETH] = NdisMedium802_3;

    LA_DbgPrint(DEBUG_DATALINK,("opening adapter %wZ\n", AdapterName));
    /* Open the adapter. */
    NdisOpenAdapter(&NdisStatus,
                    &OpenStatus,
                    &Adapter->NdisHandle,
                    &MediaIndex,
                    MediaArray,
                    MAX_MEDIA,
                    DeviceExt->NdisProtocolHandle,
                    Adapter,
                    AdapterName,
                    0,
                    NULL);

    /* Wait until the adapter is opened */
    if (NdisStatus == NDIS_STATUS_PENDING)
        KeWaitForSingleObject(&Adapter->Event, UserRequest, KernelMode, FALSE, NULL);
    else if (NdisStatus != NDIS_STATUS_SUCCESS) {
	exFreePool(Adapter);
        return NdisStatus;
    }

    Adapter->Media = MediaArray[MediaIndex];

    /* Fill LAN_ADAPTER structure with some adapter specific information */
    switch (Adapter->Media) {
    case NdisMedium802_3:
        Adapter->HWAddressLength = IEEE_802_ADDR_LENGTH;
        Adapter->BCastMask       = BCAST_ETH_MASK;
        Adapter->BCastCheck      = BCAST_ETH_CHECK;
        Adapter->BCastOffset     = BCAST_ETH_OFFSET;
        Adapter->HeaderSize      = sizeof(ETH_HEADER);
        Adapter->MinFrameSize    = 60;
        AddressOID          = OID_802_3_CURRENT_ADDRESS;
        Adapter->PacketFilter    =
            NDIS_PACKET_TYPE_BROADCAST |
            NDIS_PACKET_TYPE_DIRECTED  |
            NDIS_PACKET_TYPE_MULTICAST;
        break;

    default:
        /* Unsupported media */
        LA_DbgPrint(MIN_TRACE, ("Unsupported media.\n"));
        exFreePool(Adapter);
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    /* Get maximum frame size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_FRAME_SIZE,
                          &Adapter->MTU,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        exFreePool(Adapter);
        return NdisStatus;
    }

    /* Get maximum packet size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_TOTAL_SIZE,
                          &Adapter->MaxPacketSize,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        LA_DbgPrint(MIN_TRACE, ("Query for maximum packet size failed.\n"));
        exFreePool(Adapter);
        return NdisStatus;
    }

    /* Get maximum number of packets we can pass to NdisSend(Packets) at one time */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_SEND_PACKETS,
                          &Adapter->MaxSendPackets,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        /* Legacy NIC drivers may not support this query, if it fails we
           assume it can send at least one packet per call to NdisSend(Packets) */
        Adapter->MaxSendPackets = 1;

    /* Get current hardware address */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          AddressOID,
                          Adapter->HWAddress,
                          Adapter->HWAddressLength);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        LA_DbgPrint(MIN_TRACE, ("Query for current hardware address failed.\n"));
        exFreePool(Adapter);
        return NdisStatus;
    }

    /* Get maximum link speed */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          &Speed,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        LA_DbgPrint(MIN_TRACE, ("Query for maximum link speed failed.\n"));
        exFreePool(Adapter);
        return NdisStatus;
    }

    /* Convert returned link speed to bps (it is in 100bps increments) */
    Adapter->Speed = Speed * 100L;

    /* Add adapter to the adapter list */
    ExInterlockedInsertTailList(&DeviceExt->AdapterListHead,
                                &Adapter->ListEntry,
                                &DeviceExt->Lock);

    Adapter->RegistryPath.Buffer =
	ExAllocatePool( NonPagedPool, RegistryPath->MaximumLength );
    if( !Adapter->RegistryPath.Buffer )
	return NDIS_STATUS_RESOURCES;

    RtlCopyUnicodeString( &Adapter->RegistryPath,
			  RegistryPath );

    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_LOOKAHEAD,
                          &Adapter->Lookahead,
                          sizeof(ULONG));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        LA_DbgPrint(MID_TRACE,
		    ("Could not set lookahead buffer size (0x%X).\n",
		     NdisStatus));
        return NdisStatus;
    }

    /* Set packet filter so we can send and receive packets */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_PACKET_FILTER,
                          &Adapter->PacketFilter,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        LA_DbgPrint(MID_TRACE, ("Could not set packet filter (0x%X).\n",
				NdisStatus));
        return NdisStatus;
    }

    Adapter->State = LAN_STATE_STARTED;

    LA_DbgPrint(DEBUG_DATALINK, ("Leaving.\n"));

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS LANUnregisterAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Unregisters protocol with NDIS adapter
 * ARGUMENTS:
 *     Adapter = Pointer to a LAN_ADAPTER structure
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;
    NDIS_HANDLE NdisHandle;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    /* Unlink the adapter from the list */
    RemoveEntryList(&Adapter->ListEntry);

    KeAcquireSpinLock(&Adapter->Lock, &OldIrql);
    NdisHandle = Adapter->NdisHandle;
    if (NdisHandle) {
        Adapter->NdisHandle = NULL;
        KeReleaseSpinLock(&Adapter->Lock, OldIrql);

        NdisCloseAdapter(&NdisStatus, NdisHandle);
        if (NdisStatus == NDIS_STATUS_PENDING) {
            KeWaitForSingleObject(&Adapter->Event,
                                  UserRequest,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            NdisStatus = Adapter->NdisStatus;
        }
    } else
        KeReleaseSpinLock(&Adapter->Lock, OldIrql);

    FreeAdapter(Adapter);

    return NDIS_STATUS_SUCCESS;
}

NTSTATUS LANRegisterProtocol(PNDIS_STRING Name)
/*
 * FUNCTION: Registers this protocol driver with NDIS
 * ARGUMENTS:
 *     Name = Name of this protocol driver
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS NdisStatus;
    NDIS_PROTOCOL_CHARACTERISTICS ProtChars;
    PLAN_DEVICE_EXT DeviceExt = LanDeviceObject->DeviceExtension;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    InitializeListHead(&DeviceExt->AdapterListHead);
    InitializeListHead(&DeviceExt->ProtocolListHead);

    /* Set up protocol characteristics */
    RtlZeroMemory(&ProtChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    ProtChars.MajorNdisVersion               = NDIS_VERSION_MAJOR;
    ProtChars.MinorNdisVersion               = NDIS_VERSION_MINOR;
    ProtChars.Name.Length                    = Name->Length;
    ProtChars.Name.Buffer                    = Name->Buffer;
    ProtChars.Name.MaximumLength             = Name->MaximumLength;
    ProtChars.OpenAdapterCompleteHandler     = ProtocolOpenAdapterComplete;
    ProtChars.CloseAdapterCompleteHandler    = ProtocolCloseAdapterComplete;
    ProtChars.ResetCompleteHandler           = ProtocolResetComplete;
    ProtChars.RequestCompleteHandler         = ProtocolRequestComplete;
    ProtChars.SendCompleteHandler            = ProtocolSendComplete;
    ProtChars.TransferDataCompleteHandler    = ProtocolTransferDataComplete;
    ProtChars.ReceiveHandler                 = ProtocolReceive;
    ProtChars.ReceiveCompleteHandler         = ProtocolReceiveComplete;
    ProtChars.StatusHandler                  = ProtocolStatus;
    ProtChars.StatusCompleteHandler          = ProtocolStatusComplete;
    ProtChars.BindAdapterHandler             = ProtocolBindAdapter;

    /* Try to register protocol */
    NdisRegisterProtocol(&NdisStatus,
                         &DeviceExt->NdisProtocolHandle,
                         &ProtChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
	LA_DbgPrint(MID_TRACE, ("NdisRegisterProtocol failed, status 0x%x\n", NdisStatus));
        return (NTSTATUS)NdisStatus;
    }

    return STATUS_SUCCESS;
}


VOID LANUnregisterProtocol(VOID)
/*
 * FUNCTION: Unregisters this protocol driver with NDIS
 * NOTES: Does not care wether we are already registered
 */
{
    PLAN_DEVICE_EXT DeviceExt = LanDeviceObject->DeviceExtension;

    LA_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    NDIS_STATUS NdisStatus;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PLAN_ADAPTER Current;
    KIRQL OldIrql;

    KeAcquireSpinLock(&DeviceExt->Lock, &OldIrql);

    /* Search the list and remove every adapter we find */
    CurrentEntry = DeviceExt->AdapterListHead.Flink;
    while (CurrentEntry != &DeviceExt->AdapterListHead) {
	NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, LAN_ADAPTER, ListEntry);
	/* Unregister it */
	LANUnregisterAdapter(Current);
	CurrentEntry = NextEntry;
    }

    NdisDeregisterProtocol(&NdisStatus, DeviceExt->NdisProtocolHandle);
}

NTSTATUS STDCALL
LanCreateProtocol( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		   PIO_STACK_LOCATION IrpSp ) {
    PLAN_PROTOCOL Proto;
    PFILE_FULL_EA_INFORMATION EaInfo;
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PCHAR ProtoNumbersToMatch;
    UINT Size = sizeof( *Proto );
    NTSTATUS Status = STATUS_SUCCESS;

    EaInfo = Irp->AssociatedIrp.SystemBuffer;
    Size += EaInfo->EaValueLength;
    Proto = ExAllocatePool( NonPagedPool, Size );

    if( !Proto ) {
	Status = Irp->IoStatus.Status = STATUS_NO_MEMORY;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	return Status;
    }

    RtlZeroMemory( Proto, Size );

    Proto->Id = DeviceExt->ProtoId++;
    Proto->NumEtherTypes = EaInfo->EaValueLength / sizeof(USHORT);
    ProtoNumbersToMatch = EaInfo->EaName + EaInfo->EaNameLength + 1;

    LA_DbgPrint(MID_TRACE,("NumEtherTypes: %d\n", Proto->NumEtherTypes));

    RtlCopyMemory( Proto->EtherType,
		   ProtoNumbersToMatch,
		   sizeof(USHORT) * Proto->NumEtherTypes );

    InitializeListHead( &Proto->ReadIrpListHead );

    FileObject->FsContext = Proto;

    LA_DbgPrint(MID_TRACE,("DeviceExt: %x, Proto %x\n", DeviceExt, Proto));

    ExInterlockedInsertTailList( &DeviceExt->ProtocolListHead,
				 &Proto->ListEntry,
				 &DeviceExt->Lock );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    LA_DbgPrint(MID_TRACE,("Status %x\n", Irp->IoStatus.Status));

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
LanCloseProtocol( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		  PIO_STACK_LOCATION IrpSp ) {
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PLAN_PROTOCOL Proto = FileObject->FsContext;
    KIRQL OldIrql;
    PLIST_ENTRY ReadIrpListEntry;
    PIRP ReadIrp;
    NTSTATUS Status;

    LA_DbgPrint(MID_TRACE,("Called\n"));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    while( !IsListEmpty( &Proto->ReadIrpListHead ) ) {
	ReadIrpListEntry = RemoveHeadList( &Proto->ReadIrpListHead );

	ReadIrp = CONTAINING_RECORD( ReadIrpListEntry, IRP,
				     Tail.Overlay.ListEntry );
	ReadIrp->IoStatus.Information = 0;
	ReadIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest( ReadIrp, IO_NO_INCREMENT );
    }

    RemoveEntryList( &Proto->ListEntry );

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    LA_DbgPrint(MID_TRACE,("Deleting %x\n"));

    ExFreePool( Proto );

    Status = Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}

PLAN_ADAPTER FindAdapterByIndex( PLAN_DEVICE_EXT DeviceExt, UINT Index ) {
    PLIST_ENTRY ListEntry;
    PLAN_ADAPTER Current, Target = NULL;

    for( ListEntry = DeviceExt->AdapterListHead.Flink;
	 ListEntry != &DeviceExt->AdapterListHead;
	 ListEntry = ListEntry->Flink ) {
	Current = CONTAINING_RECORD(ListEntry, LAN_ADAPTER, ListEntry);
	if( Current->Index == Index ) {
	    Target = Current;
	    break;
	}
    }

    return Target;
}

/* Write data to an adapter:
 * |<-              16               >| |<-- variable ... -->|
 * [indx] [addrtype] [addrlen ] [ptype] [packet-data ...]
 */
NTSTATUS STDCALL
LanWriteData( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	      PIO_STACK_LOCATION IrpSp ) {
    PLAN_PACKET_HEADER ToWrite = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS Status = STATUS_SUCCESS;

    LA_DbgPrint(MID_TRACE,("Called\n"));

    Irp->IoStatus.Information =
	LANTransmitInternal( ToWrite, IrpSp->Parameters.Write.Length );
    Irp->IoStatus.Status = Status;

    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    return Status;
}

NTSTATUS STDCALL
LanReadData( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	     PIO_STACK_LOCATION IrpSp ) {
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PLAN_PROTOCOL Proto = FileObject->FsContext;

    LA_DbgPrint(MID_TRACE,("Called on %x (%x)\n", Proto, Irp));

    ExInterlockedInsertTailList( &Proto->ReadIrpListHead,
				 &Irp->Tail.Overlay.ListEntry,
				 &DeviceExt->Lock );

    LA_DbgPrint(MID_TRACE,("List: %x %x\n",
			   Proto->ReadIrpListHead.Flink,
			   Irp->Tail.Overlay.ListEntry.Flink));

    IoMarkIrpPending( Irp );
    return STATUS_PENDING;
}

NTSTATUS STDCALL
LanEnumAdapters( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		 PIO_STACK_LOCATION IrpSp ) {
    PLIST_ENTRY ListEntry;
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    PLAN_ADAPTER Adapter;
    UINT AdapterCount = 0;
    PUINT Output = Irp->AssociatedIrp.SystemBuffer;
    KIRQL OldIrql;

    LA_DbgPrint(MID_TRACE,("Called\n"));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    for( ListEntry = DeviceExt->AdapterListHead.Flink;
	 ListEntry != &DeviceExt->AdapterListHead;
	 ListEntry = ListEntry->Flink ) AdapterCount++;

    if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
	AdapterCount * sizeof(UINT) ) {
	for( ListEntry = DeviceExt->AdapterListHead.Flink;
	     ListEntry != &DeviceExt->AdapterListHead;
	     ListEntry = ListEntry->Flink ) {
	    Adapter = CONTAINING_RECORD(ListEntry, LAN_ADAPTER, ListEntry);
	    *Output++ = Adapter->Index;
	}
    } else Status = STATUS_BUFFER_TOO_SMALL;

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    LA_DbgPrint(MID_TRACE,("Ending\n"));

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (PCHAR)Output -
	(PCHAR)Irp->AssociatedIrp.SystemBuffer;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

NTSTATUS STDCALL
LanAdapterInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp ) {
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    PLAN_ADAPTER Adapter;
    PLAN_ADDRESS_C Address;
    PUINT AdapterIndexPtr = Irp->AssociatedIrp.SystemBuffer;
    PLIST_ENTRY ListEntry;
    UINT BytesNeeded = sizeof( LAN_ADAPTER_INFO ), AddrSize;
    NTSTATUS Status = STATUS_SUCCESS;
    PCHAR Writing = Irp->AssociatedIrp.SystemBuffer;
    PLAN_ADAPTER_INFO_S Info;
    KIRQL OldIrql;

    LA_DbgPrint(MID_TRACE,("Called\n"));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
	sizeof(*AdapterIndexPtr) )
	Adapter = NULL;
    else
	Adapter = FindAdapterByIndex( DeviceExt, *AdapterIndexPtr );

    if( Adapter ) {
	/* Local Addresses */
	for( ListEntry = Adapter->AddressList.Flink;
	     ListEntry != &Adapter->AddressList;
	     ListEntry = ListEntry->Flink ) {
	    Address = CONTAINING_RECORD(ListEntry, LAN_ADDRESS_C, ListEntry);
	    BytesNeeded += LAN_ADDR_SIZE(Address->ClientPart.AddressLen,
					 Address->ClientPart.HWAddressLen);
	}

	/* Foreign Addresses */
	for( ListEntry = Adapter->ForeignList.Flink;
	     ListEntry != &Adapter->ForeignList;
	     ListEntry = ListEntry->Flink ) {
	    Address = CONTAINING_RECORD(ListEntry, LAN_ADDRESS_C, ListEntry);
	    BytesNeeded += LAN_ADDR_SIZE(Address->ClientPart.AddressLen,
					 Address->ClientPart.HWAddressLen);
	}
	BytesNeeded += Adapter->RegistryPath.Length;

	if( IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
	    BytesNeeded ) {
	    /* Write common info */
	    Info = (PLAN_ADAPTER_INFO_S)Writing;
	    Info->Index      = Adapter->Index;
	    Info->Media      = Adapter->Media;
	    Info->Speed      = Adapter->Speed;
	    /* Ethernet specific XXX */
	    Info->AddressLen = IEEE_802_ADDR_LENGTH;
	    Info->Overhead   = Adapter->HeaderSize;
	    Info->MTU        = Adapter->MTU;
	    Info->RegKeySize = Adapter->RegistryPath.Length;

	    /* Copy the name */
	    Writing += sizeof(*Info);
	    RtlCopyMemory( Adapter->RegistryPath.Buffer,
			   Writing,
			   Adapter->RegistryPath.Length );

	    /* Write the address info */
	    Writing += Adapter->RegistryPath.Length;

	    for( ListEntry = Adapter->AddressList.Flink;
		 ListEntry != &Adapter->AddressList;
		 ListEntry = ListEntry->Flink ) {
		Address = CONTAINING_RECORD(ListEntry, LAN_ADDRESS_C,
					    ListEntry);
		AddrSize = LAN_ADDR_SIZE(Address->ClientPart.AddressLen,
					 Address->ClientPart.HWAddressLen);
		RtlCopyMemory( Writing, &Address->ClientPart, AddrSize );
		Writing += AddrSize;
	    }

	    for( ListEntry = Adapter->ForeignList.Flink;
		 ListEntry != &Adapter->ForeignList;
		 ListEntry = ListEntry->Flink ) {
		Address = CONTAINING_RECORD(ListEntry, LAN_ADDRESS_C,
					    ListEntry);
		AddrSize = LAN_ADDR_SIZE(Address->ClientPart.AddressLen,
					 Address->ClientPart.HWAddressLen);
		RtlCopyMemory( Writing, &Address->ClientPart, AddrSize );
		Writing += AddrSize;
	    }

	    ASSERT( BytesNeeded == Writing - Irp->AssociatedIrp.SystemBuffer );
	} else Status = STATUS_BUFFER_TOO_SMALL;
    } else Status = STATUS_NO_SUCH_DEVICE;

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    LA_DbgPrint(MID_TRACE,("Ending (%d bytes)\n", BytesNeeded));

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesNeeded;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

NTSTATUS STDCALL
LanSetBufferedMode( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		    PIO_STACK_LOCATION IrpSp ) {
    PLAN_DEVICE_EXT DeviceExt =
	(PLAN_DEVICE_EXT)DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PLAN_PROTOCOL Proto = FileObject->FsContext;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;

    LA_DbgPrint(MID_TRACE,("Called %x\n", Proto));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    if( IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
	sizeof(Proto->Buffered) )
	RtlCopyMemory( &Proto->Buffered, Irp->AssociatedIrp.SystemBuffer,
		       sizeof(Proto->Buffered) );
    else
	Status = STATUS_INVALID_PARAMETER;

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    LA_DbgPrint(MID_TRACE,("Set buffered for %x to %d\n", Proto->Buffered));

    Status = Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    return Status;
}

NTSTATUS STDCALL
LanDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;

    LA_DbgPrint(MID_TRACE,("LanDispatch: %d\n", IrpSp->MajorFunction));
    if( IrpSp->MajorFunction != IRP_MJ_CREATE) {
	LA_DbgPrint(MID_TRACE,("FO %x, IrpSp->FO %x\n",
			       FileObject, IrpSp->FileObject));
	ASSERT(FileObject == IrpSp->FileObject);
    }

    switch(IrpSp->MajorFunction)
    {
	/* opening and closing handles to the device */
    case IRP_MJ_CREATE:
	/* Mostly borrowed from the named pipe file system */
	return LanCreateProtocol(DeviceObject, Irp, IrpSp);

    case IRP_MJ_CLOSE:
	/* Ditto the borrowing */
	return LanCloseProtocol(DeviceObject, Irp, IrpSp);

	/* write data */
    case IRP_MJ_WRITE:
	return LanWriteData( DeviceObject, Irp, IrpSp );

	/* read data */
    case IRP_MJ_READ:
	return LanReadData( DeviceObject, Irp, IrpSp );

    case IRP_MJ_DEVICE_CONTROL:
    {
	LA_DbgPrint(MID_TRACE,("DeviceIoControl: %x\n",
			       IrpSp->Parameters.DeviceIoControl.
			       IoControlCode));
	switch( IrpSp->Parameters.DeviceIoControl.IoControlCode ) {
	case IOCTL_IF_ENUM_ADAPTERS:
	    return LanEnumAdapters( DeviceObject, Irp, IrpSp );

	case IOCTL_IF_BUFFERED_MODE:
	    return LanSetBufferedMode( DeviceObject, Irp, IrpSp );

	case IOCTL_IF_ADAPTER_INFO:
	    return LanAdapterInfo( DeviceObject, Irp, IrpSp );

	default:
	    Status = STATUS_NOT_IMPLEMENTED;
	    Irp->IoStatus.Information = 0;
	    LA_DbgPrint(MIN_TRACE, ("Unknown IOCTL (0x%x)\n",
				    IrpSp->Parameters.DeviceIoControl.
				    IoControlCode));
	    break;
	}
	break;
    }

    /* unsupported operations */
    default:
    {
	Status = STATUS_NOT_IMPLEMENTED;
	LA_DbgPrint(MIN_TRACE,
		    ("Irp: Unknown Major code was %x\n",
		     IrpSp->MajorFunction));
	break;
    }
    }

    LA_DbgPrint(MID_TRACE, ("Returning %x\n", Status));
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return (Status);
}

/* Do i need a global here?  I think i need to do this a different way XXX */
VOID STDCALL LanUnload(PDRIVER_OBJECT DriverObject) {
    LANUnregisterProtocol();
    CloseNdisPools();
}

NTSTATUS STDCALL DriverEntry( PDRIVER_OBJECT DriverObject,
			      PUNICODE_STRING RegsitryPath ) {
    PDEVICE_OBJECT DeviceObject;
    PLAN_DEVICE_EXT DeviceExt;
    UNICODE_STRING wstrDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Lan");
    UNICODE_STRING LanString = RTL_CONSTANT_STRING(L"LAN");
    NTSTATUS Status;

    InitNdisPools();

    /* register driver routines */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = LanDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = LanDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = LanDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = LanDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LanDispatch;
    DriverObject->DriverUnload = LanUnload;

    /* create lan device */
    Status = IoCreateDevice
	( DriverObject,
	  sizeof(LAN_DEVICE_EXT),
	  &wstrDeviceName,
	  FILE_DEVICE_NAMED_PIPE,
	  0,
	  FALSE,
	  &DeviceObject );

    /* failure */
    if(!NT_SUCCESS(Status))
    {
	return (Status);
    }

    LanDeviceObject = DeviceObject;
    DeviceExt = DeviceObject->DeviceExtension;
    RtlZeroMemory( DeviceExt, sizeof(*DeviceExt) );
    InitializeListHead( &DeviceExt->AdapterListHead );
    InitializeListHead( &DeviceExt->ProtocolListHead );
    KeInitializeSpinLock( &DeviceExt->Lock );

    LANRegisterProtocol( &LanString );

    DeviceObject->Flags |= DO_BUFFERED_IO;

    LA_DbgPrint(MID_TRACE,("Device created: object %x ext %x\n",
			   DeviceObject, DeviceExt));

    return (Status);
}

/* EOF */
