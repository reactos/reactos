/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/lan.c
 * PURPOSE:     Local Area Network media routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

/* Define this to bugcheck on double complete */
/* #define BREAK_ON_DOUBLE_COMPLETE */

UINT TransferDataCalled = 0;
UINT TransferDataCompleteCalled = 0;
UINT LanReceiveWorkerCalled = 0;

#define NGFP(_Packet)                                             \
    {                                                             \
        PVOID _Header;                                            \
        ULONG _ContigSize, _TotalSize;                            \
        PNDIS_BUFFER _NdisBuffer;                                 \
                                                                  \
        TI_DbgPrint(MID_TRACE,("Checking Packet %x\n", _Packet)); \
	NdisGetFirstBufferFromPacket(_Packet,                     \
				     &_NdisBuffer,                \
				     &_Header,                    \
				     &_ContigSize,                \
				     &_TotalSize);                \
        TI_DbgPrint(MID_TRACE,("NdisBuffer: %x\n", _NdisBuffer)); \
        TI_DbgPrint(MID_TRACE,("Header    : %x\n", _Header));     \
        TI_DbgPrint(MID_TRACE,("ContigSize: %x\n", _ContigSize)); \
        TI_DbgPrint(MID_TRACE,("TotalSize : %x\n", _TotalSize));  \
    }

typedef struct _LAN_WQ_ITEM {
    LIST_ENTRY ListEntry;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
} LAN_WQ_ITEM, *PLAN_WQ_ITEM;

NDIS_HANDLE NdisProtocolHandle = (NDIS_HANDLE)NULL;
BOOLEAN ProtocolRegistered     = FALSE;
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;

/* Work around being called back into afd at Dpc level */
KSPIN_LOCK LanWorkLock;
LIST_ENTRY LanWorkList;
WORK_QUEUE_ITEM LanWorkItem;

/* Double complete protection */
KSPIN_LOCK LanSendCompleteLock;
LIST_ENTRY LanSendCompleteList;

VOID LanChainCompletion( PLAN_ADAPTER Adapter, PNDIS_PACKET NdisPacket ) {
    PLAN_WQ_ITEM PendingCompletion = 
	ExAllocatePool( NonPagedPool, sizeof(LAN_WQ_ITEM) );
    
    if( !PendingCompletion ) return;

    PendingCompletion->Packet  = NdisPacket;
    PendingCompletion->Adapter = Adapter;

    ExInterlockedInsertTailList( &LanSendCompleteList,
				 &PendingCompletion->ListEntry,
				 &LanSendCompleteLock );
}

BOOLEAN LanShouldComplete( PLAN_ADAPTER Adapter, PNDIS_PACKET NdisPacket ) {
    PLIST_ENTRY ListEntry;
    PLAN_WQ_ITEM CompleteEntry;
    KIRQL OldIrql;

    KeAcquireSpinLock( &LanSendCompleteLock, &OldIrql );
    for( ListEntry = LanSendCompleteList.Flink;
	 ListEntry != &LanSendCompleteList;
	 ListEntry = ListEntry->Flink ) {
	CompleteEntry = CONTAINING_RECORD(ListEntry, LAN_WQ_ITEM, ListEntry);

	if( CompleteEntry->Adapter == Adapter && 
	    CompleteEntry->Packet  == NdisPacket ) {
	    RemoveEntryList( ListEntry );
	    KeReleaseSpinLock( &LanSendCompleteLock, OldIrql );
	    ExFreePool( CompleteEntry );
	    return TRUE;
	}
    }
    KeReleaseSpinLock( &LanSendCompleteLock, OldIrql );

    TI_DbgPrint(MID_TRACE,("NDIS completed the same send packet twice "
			   "(Adapter %x Packet %x)!!\n", Adapter, NdisPacket));
#ifdef BREAK_ON_DOUBLE_COMPLETE
    KeBugCheck(0);
#endif

    return FALSE;
}

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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

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
    TI_DbgPrint(MID_TRACE, ("Called.\n"));
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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

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
    TI_DbgPrint(DEBUG_DATALINK, ("Calling completion routine\n"));
    if( LanShouldComplete( (PLAN_ADAPTER)BindingContext, Packet ) ) {
	ASSERT_KM_POINTER(Packet);
	ASSERT_KM_POINTER(PC(Packet));
	ASSERT_KM_POINTER(PC(Packet)->DLComplete);
	(*PC(Packet)->DLComplete)( PC(Packet)->Context, Packet, Status);
	TI_DbgPrint(DEBUG_DATALINK, ("Finished\n"));
    }
}

VOID STDCALL LanReceiveWorker( PVOID Context ) {
    UINT PacketType;
    PLIST_ENTRY ListEntry;
    PLAN_WQ_ITEM WorkItem;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
    PNDIS_BUFFER NdisBuffer;
    IP_PACKET IPPacket;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
    
    while( (ListEntry = 
	    ExInterlockedRemoveHeadList( &LanWorkList, &LanWorkLock )) ) {
	WorkItem = CONTAINING_RECORD(ListEntry, LAN_WQ_ITEM, ListEntry);

	Packet = WorkItem->Packet;
	Adapter = WorkItem->Adapter;
	BytesTransferred = WorkItem->BytesTransferred;

	ExFreePool( WorkItem );

        IPPacket.NdisPacket = Packet;
	
        NdisGetFirstBufferFromPacket(Packet,
                                     &NdisBuffer,
                                     &IPPacket.Header,
                                     &IPPacket.ContigSize,
                                     &IPPacket.TotalSize);

	IPPacket.ContigSize = IPPacket.TotalSize = BytesTransferred;
        /* Determine which upper layer protocol that should receive
           this packet and pass it to the correct receive handler */

	TI_DbgPrint(MID_TRACE,
		    ("ContigSize: %d, TotalSize: %d, BytesTransferred: %d\n",
		     IPPacket.ContigSize, IPPacket.TotalSize,
		     BytesTransferred));

        PacketType = PC(IPPacket.NdisPacket)->PacketType;
	IPPacket.Position = 0;

	TI_DbgPrint
		(DEBUG_DATALINK,
		 ("Ether Type = %x ContigSize = %d Total = %d\n",
		  PacketType, IPPacket.ContigSize, IPPacket.TotalSize));
	
        switch (PacketType) {
            case ETYPE_IPv4:
            case ETYPE_IPv6:
		TI_DbgPrint(MID_TRACE,("Received IP Packet\n"));
                IPReceive(Adapter->Context, &IPPacket);
                break;
            case ETYPE_ARP:
		TI_DbgPrint(MID_TRACE,("Received ARP Packet\n"));
                ARPReceive(Adapter->Context, &IPPacket);
            default:
                break;
        }

	FreeNdisPacket( Packet );
    }
}

VOID LanSubmitReceiveWork( 
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET Packet,
    NDIS_STATUS Status,
    UINT BytesTransferred) {
    BOOLEAN WorkStart;
    PLAN_WQ_ITEM WQItem;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;
    KIRQL OldIrql;

    TcpipAcquireSpinLock( &LanWorkLock, &OldIrql );
    
    WQItem = ExAllocatePool( NonPagedPool, sizeof(LAN_WQ_ITEM) );
    if( !WQItem ) {
	TcpipReleaseSpinLock( &LanWorkLock, OldIrql );
	return;
    }

    WorkStart = IsListEmpty( &LanWorkList );
    WQItem->Packet = Packet;
    WQItem->Adapter = Adapter;
    WQItem->BytesTransferred = BytesTransferred;
    InsertTailList( &LanWorkList, &WQItem->ListEntry );
    if( WorkStart )
	ExQueueWorkItem( &LanWorkItem, CriticalWorkQueue );
    TcpipReleaseSpinLock( &LanWorkLock, OldIrql );
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
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    TransferDataCompleteCalled++;
    ASSERT(TransferDataCompleteCalled <= TransferDataCalled);

    if( Status != NDIS_STATUS_SUCCESS ) return;

    LanSubmitReceiveWork( BindingContext, Packet, Status, BytesTransferred );
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
    UINT temp;
    IP_PACKET IPPacket;
    PCHAR BufferData;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;
    PETH_HEADER EHeader  = (PETH_HEADER)HeaderBuffer;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_DATALINK, ("Called. (packetsize %d)\n",PacketSize));

    if (Adapter->State != LAN_STATE_STARTED) {
        TI_DbgPrint(DEBUG_DATALINK, ("Adapter is stopped.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    if (HeaderBufferSize < Adapter->HeaderSize) {
        TI_DbgPrint(DEBUG_DATALINK, ("Runt frame received.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    if (Adapter->Media == NdisMedium802_3) {
        /* Ethernet and IEEE 802.3 frames can be destinguished by
           looking at the IEEE 802.3 length field. This field is
           less than or equal to 1500 for a valid IEEE 802.3 frame
           and larger than 1500 is it's a valid EtherType value.
           See RFC 1122, section 2.3.3 for more information */
        /* FIXME: Test for Ethernet and IEEE 802.3 frame */
        if (((EType = EHeader->EType) != ETYPE_IPv4) && (EType != ETYPE_ARP)) {
            TI_DbgPrint(DEBUG_DATALINK, ("Not IP or ARP frame. EtherType (0x%X).\n", EType));
            return NDIS_STATUS_NOT_ACCEPTED;
        }
        /* We use EtherType constants to destinguish packet types */
        PacketType = EType;
    } else {
        TI_DbgPrint(MIN_TRACE, ("Unsupported media.\n"));
        /* FIXME: Support other medias */
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Get a transfer data packet */

    TI_DbgPrint(DEBUG_DATALINK, ("Adapter: %x (MTU %d)\n", 
				 Adapter, Adapter->MTU));

    TcpipAcquireSpinLock( &LanWorkLock, &OldIrql );

    NdisStatus = AllocatePacketWithBuffer( &NdisPacket, NULL,
                                           PacketSize + HeaderBufferSize );
    if( NdisStatus != NDIS_STATUS_SUCCESS ) {
	TcpipReleaseSpinLock( &LanWorkLock, OldIrql );
	return NDIS_STATUS_NOT_ACCEPTED;
    }

    PC(NdisPacket)->PacketType = PacketType;

    TI_DbgPrint(DEBUG_DATALINK, ("pretransfer LookaheadBufferSize %d packsize %d\n",LookaheadBufferSize,PacketSize));

    GetDataPtr( NdisPacket, 0, &BufferData, &temp );

    IPPacket.NdisPacket = NdisPacket;
    IPPacket.Position = 0;

    TransferDataCalled++;

    if (LookaheadBufferSize == PacketSize)
    {
        /* Optimized code path for packets that are fully contained in
         * the lookahead buffer. */
        NdisCopyLookaheadData(BufferData,
                              LookaheadBuffer,
                              LookaheadBufferSize,
                              Adapter->MacOptions);
    }
    else
    {
	if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
	    ASSERT(PacketSize <= Adapter->MTU);

            NdisTransferData(&NdisStatus, Adapter->NdisHandle,
                             MacReceiveContext, 0, PacketSize, 
			     NdisPacket, &BytesTransferred);
        }
        else
        {
            BytesTransferred = 0;
        }
    }
    TcpipReleaseSpinLock( &LanWorkLock, OldIrql );
    TI_DbgPrint(DEBUG_DATALINK, ("Calling complete\n"));

    if (NdisStatus != NDIS_STATUS_PENDING)
	ProtocolTransferDataComplete(BindingContext,
				     NdisPacket,
				     NdisStatus,
				     PacketSize);

    TI_DbgPrint(DEBUG_DATALINK, ("leaving\n"));

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
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
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
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}


VOID STDCALL ProtocolStatusComplete(
    NDIS_HANDLE NdisBindingContext)
/*
 * FUNCTION: Called by NDIS when a status-change has occurred
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 */
{
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
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
	TI_DbgPrint(DEBUG_DATALINK, ("Called with registry path %wZ for %wZ\n", SystemSpecific1, DeviceName));
	*Status = LANRegisterAdapter(DeviceName, SystemSpecific1);
}


VOID LANTransmit(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    UINT Offset,
    PVOID LinkAddress,
    USHORT Type)
/*
 * FUNCTION: Transmits a packet
 * ARGUMENTS:
 *     Context     = Pointer to context information (LAN_ADAPTER)
 *     NdisPacket  = Pointer to NDIS packet to send
 *     Offset      = Offset in packet where data starts
 *     LinkAddress = Pointer to link address of destination (NULL = broadcast)
 *     Type        = LAN protocol type (LAN_PROTO_*)
 */
{
    NDIS_STATUS NdisStatus;
    PETH_HEADER EHeader;
    PCHAR Data;
    UINT Size;
    KIRQL OldIrql;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)Context;

    TI_DbgPrint(DEBUG_DATALINK, 
		("Called( NdisPacket %x, Offset %d, Adapter %x )\n",
		 NdisPacket, Offset, Adapter));

    TI_DbgPrint(DEBUG_DATALINK,
		("Adapter Address [%02x %02x %02x %02x %02x %02x]\n",
		 Adapter->HWAddress[0] & 0xff,
		 Adapter->HWAddress[1] & 0xff,
		 Adapter->HWAddress[2] & 0xff,
		 Adapter->HWAddress[3] & 0xff,
		 Adapter->HWAddress[4] & 0xff,
		 Adapter->HWAddress[5] & 0xff));

    /* XXX arty -- Handled adjustment in a saner way than before ... 
     * not needed immediately */
    GetDataPtr( NdisPacket, 0, &Data, &Size );

    if (Adapter->State == LAN_STATE_STARTED) {
        switch (Adapter->Media) {
        case NdisMedium802_3:
            EHeader = (PETH_HEADER)Data;

            if (LinkAddress) {
                /* Unicast address */
                RtlCopyMemory(EHeader->DstAddr, LinkAddress, IEEE_802_ADDR_LENGTH);
            } else {
                /* Broadcast address */
                RtlFillMemory(EHeader->DstAddr, IEEE_802_ADDR_LENGTH, 0xFF);
            }

            RtlCopyMemory(EHeader->SrcAddr, Adapter->HWAddress, IEEE_802_ADDR_LENGTH);

            switch (Type) {
                case LAN_PROTO_IPv4:
                    EHeader->EType = ETYPE_IPv4;
                    break;
                case LAN_PROTO_ARP:
                    EHeader->EType = ETYPE_ARP;
                    break;
                case LAN_PROTO_IPv6:
                    EHeader->EType = ETYPE_IPv6;
                    break;
                default:
#ifdef DBG
                    /* Should not happen */
                    TI_DbgPrint(MIN_TRACE, ("Unknown LAN protocol.\n"));

                    ProtocolSendComplete((NDIS_HANDLE)Context,
                                         NdisPacket,
                                         NDIS_STATUS_FAILURE);
#endif
                    return;
            }
            break;

        default:
            /* FIXME: Support other medias */
            break;
        }

	TI_DbgPrint( MID_TRACE, ("LinkAddress: %x\n", LinkAddress));
	if( LinkAddress ) {
	    TI_DbgPrint
		( MID_TRACE, 
		  ("Link Address [%02x %02x %02x %02x %02x %02x]\n", 
		   ((PCHAR)LinkAddress)[0] & 0xff,
		   ((PCHAR)LinkAddress)[1] & 0xff,
		   ((PCHAR)LinkAddress)[2] & 0xff,
		   ((PCHAR)LinkAddress)[3] & 0xff,
		   ((PCHAR)LinkAddress)[4] & 0xff,
		   ((PCHAR)LinkAddress)[5] & 0xff));
	}

	TcpipAcquireSpinLock( &Adapter->Lock, &OldIrql );
	TI_DbgPrint(MID_TRACE, ("NdisSend\n"));
        NdisSend(&NdisStatus, Adapter->NdisHandle, NdisPacket);
	LanChainCompletion( Adapter, NdisPacket );
	TI_DbgPrint(MID_TRACE, ("NdisSend Done\n"));
	TcpipReleaseSpinLock( &Adapter->Lock, OldIrql );

#if 0
        if (NdisStatus != NDIS_STATUS_PENDING)
            ProtocolSendComplete((NDIS_HANDLE)Context, NdisPacket, NdisStatus);
#endif
    } else {
#if 0
        ProtocolSendComplete((NDIS_HANDLE)Context, NdisPacket, NDIS_STATUS_CLOSED);
#endif
    }
}

static NTSTATUS 
OpenRegistryKey( PNDIS_STRING RegistryPath, PHANDLE RegHandle ) {
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    
    InitializeObjectAttributes(&Attributes, RegistryPath, OBJ_CASE_INSENSITIVE, 0, 0);
    Status = ZwOpenKey(RegHandle, GENERIC_READ, &Attributes);
    return Status;
}

static NTSTATUS ReadIPAddressFromRegistry( HANDLE RegHandle,
					   PWCHAR RegistryValue,
					   PIP_ADDRESS Address ) {
    UNICODE_STRING ValueName;
    UNICODE_STRING UnicodeAddress; 
    NTSTATUS Status;
    ULONG ResultLength;
    UCHAR buf[1024];
    PKEY_VALUE_PARTIAL_INFORMATION Information = (PKEY_VALUE_PARTIAL_INFORMATION)buf;
    ANSI_STRING AnsiAddress;
    ULONG AnsiLen;

    RtlInitUnicodeString(&ValueName, RegistryValue);
    Status = 
	ZwQueryValueKey(RegHandle, 
			&ValueName, 
			KeyValuePartialInformation, 
			Information, 
			sizeof(buf), 
			&ResultLength);

    if (!NT_SUCCESS(Status))
	return Status;
    /* IP address is stored as a REG_MULTI_SZ - we only pay attention to the first one though */
    TI_DbgPrint(MIN_TRACE, ("Information DataLength: 0x%x\n", Information->DataLength));
    
    UnicodeAddress.Buffer = (PWCHAR)&Information->Data;
    UnicodeAddress.Length = Information->DataLength;
    UnicodeAddress.MaximumLength = Information->DataLength;
    
    AnsiLen = RtlUnicodeStringToAnsiSize(&UnicodeAddress);
    if(!AnsiLen)
	return STATUS_NO_MEMORY;
    
    AnsiAddress.Buffer = exAllocatePoolWithTag(PagedPool, AnsiLen, 0x01020304);
    if(!AnsiAddress.Buffer)
	return STATUS_NO_MEMORY;

    AnsiAddress.Length = AnsiLen;
    AnsiAddress.MaximumLength = AnsiLen;
    
    Status = RtlUnicodeStringToAnsiString(&AnsiAddress, &UnicodeAddress, FALSE);
    if (!NT_SUCCESS(Status)) {
	exFreePool(AnsiAddress.Buffer);
	return STATUS_UNSUCCESSFUL;
    }
    
    AnsiAddress.Buffer[AnsiAddress.Length] = 0;
    AddrInitIPv4(Address, inet_addr(AnsiAddress.Buffer));

    return STATUS_SUCCESS;
}

static NTSTATUS ReadStringFromRegistry( HANDLE RegHandle,
					PWCHAR RegistryValue,
					PUNICODE_STRING String ) {
    UNICODE_STRING ValueName;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG ResultLength;
    UCHAR buf[1024];
    PKEY_VALUE_PARTIAL_INFORMATION Information = (PKEY_VALUE_PARTIAL_INFORMATION)buf;

    RtlInitUnicodeString(&ValueName, RegistryValue);
    Status = 
	ZwQueryValueKey(RegHandle, 
			&ValueName, 
			KeyValuePartialInformation, 
			Information, 
			sizeof(buf), 
			&ResultLength);

    if (!NT_SUCCESS(Status))
	return Status;
    /* IP address is stored as a REG_MULTI_SZ - we only pay attention to the first one though */
    TI_DbgPrint(MIN_TRACE, ("Information DataLength: 0x%x\n", Information->DataLength));
    
    UnicodeString.Buffer = (PWCHAR)&Information->Data;
    UnicodeString.Length = Information->DataLength;
    UnicodeString.MaximumLength = Information->DataLength;
 
    String->Buffer = 
	(PWCHAR)exAllocatePool( NonPagedPool, 
				UnicodeString.MaximumLength + sizeof(WCHAR) );

    if( !String->Buffer ) return STATUS_NO_MEMORY;

    String->MaximumLength = UnicodeString.MaximumLength;
    RtlCopyUnicodeString( String, &UnicodeString );

    return STATUS_SUCCESS;
}

static VOID GetSimpleAdapterNameFromRegistryPath
( PUNICODE_STRING TargetString,
  PUNICODE_STRING RegistryPath ) {
    PWCHAR i, LastSlash = NULL;
    UINT NewStringLength = 0;

    for( i = RegistryPath->Buffer; 
	 i < RegistryPath->Buffer + 
	     (RegistryPath->Length / sizeof(WCHAR));
	 i++ ) if( *i == '\\' ) LastSlash = i;

    if( LastSlash ) LastSlash++; else LastSlash = RegistryPath->Buffer;

    NewStringLength = RegistryPath->MaximumLength - 
	((LastSlash - RegistryPath->Buffer) * sizeof(WCHAR));

    TargetString->Buffer = 
	(PWCHAR)exAllocatePool( NonPagedPool, NewStringLength );

    if( !TargetString->Buffer ) {
	TargetString->Length = TargetString->MaximumLength = 0;
	return;
    }

    TargetString->Length = TargetString->MaximumLength = NewStringLength;
    RtlCopyMemory( TargetString->Buffer, LastSlash, NewStringLength );
}

VOID BindAdapter(
    PLAN_ADAPTER Adapter,
    PNDIS_STRING RegistryPath)
/*
 * FUNCTION: Binds a LAN adapter to IP layer
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure
 * NOTES:
 *    We set the lookahead buffer size, set the packet filter and
 *    bind the adapter to IP layer
 */
{
    PIP_INTERFACE IF;
    NDIS_STATUS NdisStatus;
    LLIP_BIND_INFO BindInfo;
    IP_ADDRESS DefaultGateway, DefaultMask = { 0 };
    ULONG Lookahead = LOOKAHEAD_SIZE;
    NTSTATUS Status;
    HANDLE RegHandle = 0;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    Adapter->State = LAN_STATE_OPENING;

    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_LOOKAHEAD,
                          &Lookahead,
                          sizeof(ULONG));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MID_TRACE, ("Could not set lookahead buffer size (0x%X).\n", NdisStatus));
        return;
    }

    /* Bind the adapter to IP layer */
    BindInfo.Context       = Adapter;
    BindInfo.HeaderSize    = Adapter->HeaderSize;
    BindInfo.MinFrameSize  = Adapter->MinFrameSize;
    BindInfo.MTU           = Adapter->MTU;
    BindInfo.Address       = (PUCHAR)&Adapter->HWAddress;
    BindInfo.AddressLength = Adapter->HWAddressLength;
    BindInfo.Transmit      = LANTransmit;

    IF = IPCreateInterface(&BindInfo);

    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return;
    }

    /* 
     * Query per-adapter configuration from the registry 
     * In case anyone is curious:  there *is* an Ndis configuration api
     * for this sort of thing, but it doesn't really support things like
     * REG_MULTI_SZ very well, and there is a note in the DDK that says that
     * protocol drivers developed for win2k and above just use the native
     * services (ZwOpenKey, etc).
     */
    
    Status = OpenRegistryKey( RegistryPath, &RegHandle );
	    
    if(NT_SUCCESS(Status))
	Status = ReadIPAddressFromRegistry( RegHandle, L"DefaultGateway",
					    &DefaultGateway );
    if(!NT_SUCCESS(Status)) {
	Status = STATUS_SUCCESS;
	RtlZeroMemory( &DefaultGateway, sizeof(DefaultGateway) );
    }

    if(NT_SUCCESS(Status))
	Status = ReadIPAddressFromRegistry( RegHandle, L"IPAddress",
					    &IF->Unicast );
    if(NT_SUCCESS(Status)) 
	Status = ReadIPAddressFromRegistry( RegHandle, L"SubnetMask",
					    &IF->Netmask );
    if(NT_SUCCESS(Status)) {
	Status = ReadStringFromRegistry( RegHandle, L"DeviceDesc",
					 &IF->Name );

	RtlZeroMemory( &IF->Name, sizeof( IF->Name ) );

	/* I think that not getting a devicedesc is not a fatal error */
	if( !NT_SUCCESS(Status) ) {
	    if( IF->Name.Buffer ) exFreePool( IF->Name.Buffer );
	    GetSimpleAdapterNameFromRegistryPath( &IF->Name, RegistryPath );
	}
	Status = STATUS_SUCCESS;
    }

    if(!NT_SUCCESS(Status))
    {
	TI_DbgPrint(MIN_TRACE, ("Unable to open protocol-specific registry key: 0x%x\n", Status));
	
	/* XXX how do we proceed?  No ip address, no parameters... do we guess? */
	if(RegHandle)  
	    ZwClose(RegHandle);
	IPDestroyInterface(IF);
	return;
    }
    
    TI_DbgPrint
	(MID_TRACE, 
	 ("--> Our IP address on this interface: '%s'\n", 
	  A2S(&IF->Unicast)));
    
    TI_DbgPrint
	(MID_TRACE, 
	 ("--> Our net mask on this interface: '%s'\n", 
	  A2S(&IF->Netmask)));

    if( DefaultGateway.Address.IPv4Address ) {
	TI_DbgPrint
	    (MID_TRACE, 
	     ("--> Our gateway is: '%s'\n", 
	      A2S(&DefaultGateway)));

	/* Create a default route */
	RouterCreateRoute( &DefaultMask, /* Zero */
			   &DefaultMask, /* Zero */
			   &DefaultGateway,
			   IF,
			   1 );
    }

    /* Get maximum link speed */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          &IF->Speed,
                          sizeof(UINT));

    if( !NT_SUCCESS(NdisStatus) )
	IF->Speed = IP_DEFAULT_LINK_SPEED;

    /* Register interface with IP layer */
    IPRegisterInterface(IF);

    /* Set packet filter so we can send and receive packets */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_PACKET_FILTER,
                          &Adapter->PacketFilter,
                          sizeof(UINT));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MID_TRACE, ("Could not set packet filter (0x%X).\n", NdisStatus));
        IPDestroyInterface(IF);
        return;
    }

    Adapter->Context = IF;
    Adapter->State = LAN_STATE_STARTED;
}


VOID UnbindAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Unbinds a LAN adapter from IP layer
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure
 */
{
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    if (Adapter->State == LAN_STATE_STARTED) {
        PIP_INTERFACE IF = Adapter->Context;

        IPUnregisterInterface(IF);

        IPDestroyInterface(IF);
    }
}


NDIS_STATUS LANRegisterAdapter(
    PNDIS_STRING AdapterName,
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
    PLAN_ADAPTER IF;
    NDIS_STATUS NdisStatus;
    NDIS_STATUS OpenStatus;
    UINT MediaIndex;
    NDIS_MEDIUM MediaArray[MAX_MEDIA];
    UINT AddressOID;
    UINT Speed;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    IF = exAllocatePool(NonPagedPool, sizeof(LAN_ADAPTER));
    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(IF, sizeof(LAN_ADAPTER));

    /* Put adapter in stopped state */
    IF->State = LAN_STATE_STOPPED;

    /* Initialize protecting spin lock */
    KeInitializeSpinLock(&IF->Lock);

    KeInitializeEvent(&IF->Event, SynchronizationEvent, FALSE);

    /* Initialize array with media IDs we support */
    MediaArray[MEDIA_ETH] = NdisMedium802_3;

    TI_DbgPrint(DEBUG_DATALINK,("opening adapter %wZ\n", AdapterName));
    /* Open the adapter. */
    NdisOpenAdapter(&NdisStatus,
                    &OpenStatus,
                    &IF->NdisHandle,
                    &MediaIndex,
                    MediaArray,
                    MAX_MEDIA,
                    NdisProtocolHandle,
                    IF,
                    AdapterName,
                    0,
                    NULL);

    /* Wait until the adapter is opened */
    if (NdisStatus == NDIS_STATUS_PENDING)
        KeWaitForSingleObject(&IF->Event, UserRequest, KernelMode, FALSE, NULL);
    else if (NdisStatus != NDIS_STATUS_SUCCESS) {
	exFreePool(IF);
        return NdisStatus;
    }

    IF->Media = MediaArray[MediaIndex];

    /* Fill LAN_ADAPTER structure with some adapter specific information */
    switch (IF->Media) {
    case NdisMedium802_3:
        IF->HWAddressLength = IEEE_802_ADDR_LENGTH;
        IF->BCastMask       = BCAST_ETH_MASK;
        IF->BCastCheck      = BCAST_ETH_CHECK;
        IF->BCastOffset     = BCAST_ETH_OFFSET;
        IF->HeaderSize      = sizeof(ETH_HEADER);
        IF->MinFrameSize    = 60;
        AddressOID          = OID_802_3_CURRENT_ADDRESS;
        IF->PacketFilter    = 
            NDIS_PACKET_TYPE_BROADCAST |
            NDIS_PACKET_TYPE_DIRECTED  |
            NDIS_PACKET_TYPE_MULTICAST;
        break;

    default:
        /* Unsupported media */
        TI_DbgPrint(MIN_TRACE, ("Unsupported media.\n"));
        exFreePool(IF);
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    /* Get maximum frame size */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_FRAME_SIZE,
                          &IF->MTU,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        exFreePool(IF);
        return NdisStatus;
    }

    /* Get maximum packet size */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_TOTAL_SIZE,
                          &IF->MaxPacketSize,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MIN_TRACE, ("Query for maximum packet size failed.\n"));
        exFreePool(IF);
        return NdisStatus;
    }

    /* Get maximum number of packets we can pass to NdisSend(Packets) at one time */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_SEND_PACKETS,
                          &IF->MaxSendPackets,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        /* Legacy NIC drivers may not support this query, if it fails we
           assume it can send at least one packet per call to NdisSend(Packets) */
        IF->MaxSendPackets = 1;

    /* Get current hardware address */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          AddressOID,
                          &IF->HWAddress,
                          IF->HWAddressLength);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MIN_TRACE, ("Query for current hardware address failed.\n"));
        exFreePool(IF);
        return NdisStatus;
    }

    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          &Speed,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(MIN_TRACE, ("Query for maximum link speed failed.\n"));
        exFreePool(IF);
        return NdisStatus;
    }

    /* Convert returned link speed to bps (it is in 100bps increments) */
    IF->Speed = Speed * 100L;

    /* Add adapter to the adapter list */
    ExInterlockedInsertTailList(&AdapterListHead,
                                &IF->ListEntry,
                                &AdapterListLock);

    /* Bind adapter to IP layer */
    BindAdapter(IF, RegistryPath);

    TI_DbgPrint(DEBUG_DATALINK, ("Leaving.\n"));

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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    /* Unlink the adapter from the list */
    RemoveEntryList(&Adapter->ListEntry);

    /* Unbind adapter from IP layer */
    UnbindAdapter(Adapter);

    TcpipAcquireSpinLock(&Adapter->Lock, &OldIrql);
    NdisHandle = Adapter->NdisHandle;
    if (NdisHandle) {
        Adapter->NdisHandle = NULL;
        TcpipReleaseSpinLock(&Adapter->Lock, OldIrql);

        NdisCloseAdapter(&NdisStatus, NdisHandle);
        if (NdisStatus == NDIS_STATUS_PENDING) {
            TcpipWaitForSingleObject(&Adapter->Event,
                                  UserRequest,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            NdisStatus = Adapter->NdisStatus;
        }
    } else
        TcpipReleaseSpinLock(&Adapter->Lock, OldIrql);

    FreeAdapter(Adapter);

    return NDIS_STATUS_SUCCESS;
}


NTSTATUS LANRegisterProtocol(
    PNDIS_STRING Name)
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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    InitializeListHead(&AdapterListHead);
    KeInitializeSpinLock(&AdapterListLock);

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
                         &NdisProtocolHandle,
                         &ProtChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
	 {
		 TI_DbgPrint(MID_TRACE, ("NdisRegisterProtocol failed, status 0x%x\n", NdisStatus));
        return (NTSTATUS)NdisStatus;
	 }

    ProtocolRegistered = TRUE;

    return STATUS_SUCCESS;
}


VOID LANUnregisterProtocol(
    VOID)
/*
 * FUNCTION: Unregisters this protocol driver with NDIS
 * NOTES: Does not care wether we are already registered
 */
{
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    if (ProtocolRegistered) {
        NDIS_STATUS NdisStatus;
        PLIST_ENTRY CurrentEntry;
        PLIST_ENTRY NextEntry;
        PLAN_ADAPTER Current;
        KIRQL OldIrql;

        TcpipAcquireSpinLock(&AdapterListLock, &OldIrql);

        /* Search the list and remove every adapter we find */
        CurrentEntry = AdapterListHead.Flink;
        while (CurrentEntry != &AdapterListHead) {
            NextEntry = CurrentEntry->Flink;
	        Current = CONTAINING_RECORD(CurrentEntry, LAN_ADAPTER, ListEntry);
            /* Unregister it */
            LANUnregisterAdapter(Current);
            CurrentEntry = NextEntry;
        }

        TcpipReleaseSpinLock(&AdapterListLock, OldIrql);

        NdisDeregisterProtocol(&NdisStatus, NdisProtocolHandle);
        ProtocolRegistered = FALSE;
    }
}

VOID LANStartup() {
    InitializeListHead( &LanWorkList );
    InitializeListHead( &LanSendCompleteList );
    KeInitializeSpinLock( &LanSendCompleteLock );
    ExInitializeWorkItem( &LanWorkItem, LanReceiveWorker, NULL );
}

VOID LANShutdown() {
    KIRQL OldIrql;
    PLAN_WQ_ITEM WorkItem;
    PLIST_ENTRY ListEntry;

    TcpipAcquireSpinLock( &LanWorkLock, &OldIrql );
    while( !IsListEmpty( &LanWorkList ) ) {
	ListEntry = RemoveHeadList( &LanWorkList );
	WorkItem = CONTAINING_RECORD(ListEntry, LAN_WQ_ITEM, ListEntry);
	FreeNdisPacket( WorkItem->Packet );
	ExFreePool( WorkItem );
    }
    TcpipReleaseSpinLock( &LanWorkLock, OldIrql );

    KeAcquireSpinLock( &LanSendCompleteLock, &OldIrql );
    while( !IsListEmpty( &LanSendCompleteList ) ) {
	ListEntry = RemoveHeadList( &LanSendCompleteList );
	WorkItem = CONTAINING_RECORD(ListEntry, LAN_WQ_ITEM, ListEntry);
	FreeNdisPacket( WorkItem->Packet );
	ExFreePool( WorkItem );
    }
    KeReleaseSpinLock( &LanSendCompleteLock, OldIrql );
}

/* EOF */
