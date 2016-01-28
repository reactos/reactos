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

#include <ntifs.h>
#include <receive.h>
#include <wait.h>

UINT TransferDataCalled = 0;
UINT TransferDataCompleteCalled = 0;

#define CCS_ROOT L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet"
#define TCPIP_GUID L"{4D36E972-E325-11CE-BFC1-08002BE10318}"

typedef struct _LAN_WQ_ITEM {
    LIST_ENTRY ListEntry;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
    BOOLEAN LegacyReceive;
} LAN_WQ_ITEM, *PLAN_WQ_ITEM;

typedef struct _RECONFIGURE_CONTEXT {
    ULONG State;
    PLAN_ADAPTER Adapter;
} RECONFIGURE_CONTEXT, *PRECONFIGURE_CONTEXT;

NDIS_HANDLE NdisProtocolHandle = (NDIS_HANDLE)NULL;
BOOLEAN ProtocolRegistered     = FALSE;
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;

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

/* Used by legacy ProtocolReceive for packet type */
NDIS_STATUS
GetPacketTypeFromHeaderBuffer(PLAN_ADAPTER Adapter,
                              PVOID HeaderBuffer,
                              ULONG HeaderBufferSize,
                              PULONG PacketType)
{
    PETH_HEADER EthHeader = HeaderBuffer;

    if (HeaderBufferSize < Adapter->HeaderSize)
    {
        TI_DbgPrint(DEBUG_DATALINK, ("Runt frame (size %d).\n", HeaderBufferSize));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    switch (Adapter->Media)
    {
        case NdisMedium802_3:
            /* Ethernet and IEEE 802.3 frames can be destinguished by
               looking at the IEEE 802.3 length field. This field is
               less than or equal to 1500 for a valid IEEE 802.3 frame
               and larger than 1500 is it's a valid EtherType value.
               See RFC 1122, section 2.3.3 for more information */

            *PacketType = EthHeader->EType;
            break;

        default:
            TI_DbgPrint(MIN_TRACE, ("Unsupported media.\n"));

            /* FIXME: Support other medias */
            return NDIS_STATUS_NOT_ACCEPTED;
    }
    
    TI_DbgPrint(DEBUG_DATALINK, ("EtherType (0x%X).\n", *PacketType));
    
    return NDIS_STATUS_SUCCESS;
}

/* Used by ProtocolReceivePacket for packet type */
NDIS_STATUS
GetPacketTypeFromNdisPacket(PLAN_ADAPTER Adapter,
                            PNDIS_PACKET NdisPacket,
                            PULONG PacketType)
{
    PVOID HeaderBuffer;
    ULONG BytesCopied;
    NDIS_STATUS Status;
    
    HeaderBuffer = ExAllocatePool(NonPagedPool,
                                  Adapter->HeaderSize);
    if (!HeaderBuffer)
        return NDIS_STATUS_RESOURCES;
    
    /* Copy the media header */
    BytesCopied = CopyPacketToBuffer(HeaderBuffer,
                                     NdisPacket,
                                     0,
                                     Adapter->HeaderSize);
    if (BytesCopied != Adapter->HeaderSize)
    {
        /* Runt frame */
        ExFreePool(HeaderBuffer);
        TI_DbgPrint(DEBUG_DATALINK, ("Runt frame (size %d).\n", BytesCopied));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    Status = GetPacketTypeFromHeaderBuffer(Adapter,
                                           HeaderBuffer,
                                           BytesCopied,
                                           PacketType);
    
    ExFreePool(HeaderBuffer);
    
    return Status;
}


VOID FreeAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Frees memory for a LAN_ADAPTER structure
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure to free
 */
{
    ExFreePoolWithTag(Adapter, LAN_ADAPTER_TAG);
}


NTSTATUS TcpipLanGetDwordOid
( PIP_INTERFACE Interface,
  NDIS_OID Oid,
  PULONG Result ) {
    /* Get maximum frame size */
    if( Interface->Context ) {
        return NDISCall((PLAN_ADAPTER)Interface->Context,
                        NdisRequestQueryInformation,
                        Oid,
                        Result,
                        sizeof(ULONG));
    } else switch( Oid ) { /* Loopback Case */
    case OID_GEN_HARDWARE_STATUS:
        *Result = NdisHardwareStatusReady;
        return STATUS_SUCCESS;
    case OID_GEN_MEDIA_CONNECT_STATUS:
        *Result = NdisMediaStateConnected;
        return STATUS_SUCCESS;
    default:
        return STATUS_INVALID_PARAMETER;
    }
}


VOID NTAPI ProtocolOpenAdapterComplete(
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

    Adapter->NdisStatus = Status;

    KeSetEvent(&Adapter->Event, 0, FALSE);
}


VOID NTAPI ProtocolCloseAdapterComplete(
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


VOID NTAPI ProtocolResetComplete(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS Status)
/*
 * FUNCTION: Called by NDIS to complete resetting an adapter
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


VOID NTAPI ProtocolRequestComplete(
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


VOID NTAPI ProtocolSendComplete(
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
    FreeNdisPacket(Packet);
}

VOID LanReceiveWorker( PVOID Context ) {
    ULONG PacketType;
    PLAN_WQ_ITEM WorkItem = (PLAN_WQ_ITEM)Context;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
    IP_PACKET IPPacket;
    BOOLEAN LegacyReceive;
    PIP_INTERFACE Interface;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    Packet = WorkItem->Packet;
    Adapter = WorkItem->Adapter;
    BytesTransferred = WorkItem->BytesTransferred;
    LegacyReceive = WorkItem->LegacyReceive;

    ExFreePoolWithTag(WorkItem, WQ_CONTEXT_TAG);

    Interface = Adapter->Context;

    IPInitializePacket(&IPPacket, 0);

    IPPacket.NdisPacket = Packet;
    IPPacket.ReturnPacket = !LegacyReceive;

    if (LegacyReceive)
    {
        /* Packet type is precomputed */
        PacketType = PC(IPPacket.NdisPacket)->PacketType;

        /* Data is at position 0 */
        IPPacket.Position = 0;

        /* Packet size is determined by bytes transferred */
        IPPacket.TotalSize = BytesTransferred;
    }
    else
    {
        /* Determine packet type from media header */
        if (GetPacketTypeFromNdisPacket(Adapter,
                                        IPPacket.NdisPacket,
                                        &PacketType) != NDIS_STATUS_SUCCESS)
        {
            /* Bad packet */
            IPPacket.Free(&IPPacket);
            return;
        }

        /* Data is at the end of the media header */
        IPPacket.Position = Adapter->HeaderSize;

        /* Calculate packet size (excluding media header) */
        NdisQueryPacketLength(IPPacket.NdisPacket, &IPPacket.TotalSize);
    }

    TI_DbgPrint
	(DEBUG_DATALINK,
	 ("Ether Type = %x Total = %d\n",
	  PacketType, IPPacket.TotalSize));

    /* Update interface stats */
    Interface->Stats.InBytes += IPPacket.TotalSize + Adapter->HeaderSize;

    /* NDIS packet is freed in all of these cases */
    switch (PacketType) {
        case ETYPE_IPv4:
        case ETYPE_IPv6:
            TI_DbgPrint(MID_TRACE,("Received IP Packet\n"));
            IPReceive(Adapter->Context, &IPPacket);
            break;
        case ETYPE_ARP:
            TI_DbgPrint(MID_TRACE,("Received ARP Packet\n"));
            ARPReceive(Adapter->Context, &IPPacket);
            break;
        default:
            IPPacket.Free(&IPPacket);
            break;
    }
}

VOID LanSubmitReceiveWork(
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET Packet,
    UINT BytesTransferred,
    BOOLEAN LegacyReceive) {
    PLAN_WQ_ITEM WQItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(LAN_WQ_ITEM),
                                                WQ_CONTEXT_TAG);
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    TI_DbgPrint(DEBUG_DATALINK,("called\n"));

    if (!WQItem) return;

    WQItem->Packet = Packet;
    WQItem->Adapter = Adapter;
    WQItem->BytesTransferred = BytesTransferred;
    WQItem->LegacyReceive = LegacyReceive;

    if (!ChewCreate( LanReceiveWorker, WQItem ))
        ExFreePoolWithTag(WQItem, WQ_CONTEXT_TAG);
}

VOID NTAPI ProtocolTransferDataComplete(
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

    TI_DbgPrint(DEBUG_DATALINK,("called\n"));

    TransferDataCompleteCalled++;
    ASSERT(TransferDataCompleteCalled <= TransferDataCalled);

    if( Status != NDIS_STATUS_SUCCESS ) return;

    LanSubmitReceiveWork(BindingContext,
                         Packet,
                         BytesTransferred,
                         TRUE);
}

INT NTAPI ProtocolReceivePacket(
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET NdisPacket)
{
    PLAN_ADAPTER Adapter = BindingContext;

    if (Adapter->State != LAN_STATE_STARTED) {
        TI_DbgPrint(DEBUG_DATALINK, ("Adapter is stopped.\n"));
        return 0;
    }

    LanSubmitReceiveWork(BindingContext,
                         NdisPacket,
                         0, /* Unused */
                         FALSE);

    /* Hold 1 reference on this packet */
    return 1;
}

NDIS_STATUS NTAPI ProtocolReceive(
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
    ULONG PacketType;
    UINT BytesTransferred;
    PCHAR BufferData;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    TI_DbgPrint(DEBUG_DATALINK, ("Called. (packetsize %d)\n",PacketSize));

    if (Adapter->State != LAN_STATE_STARTED) {
        TI_DbgPrint(DEBUG_DATALINK, ("Adapter is stopped.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    if (HeaderBufferSize < Adapter->HeaderSize) {
        TI_DbgPrint(DEBUG_DATALINK, ("Runt frame received.\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    NdisStatus = GetPacketTypeFromHeaderBuffer(Adapter,
                                               HeaderBuffer,
                                               HeaderBufferSize,
                                               &PacketType);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return NDIS_STATUS_NOT_ACCEPTED;

    TI_DbgPrint(DEBUG_DATALINK, ("Adapter: %x (MTU %d)\n",
				 Adapter, Adapter->MTU));

    /* Get a transfer data packet */
    NdisStatus = AllocatePacketWithBuffer( &NdisPacket, NULL,
                                           PacketSize );
    if( NdisStatus != NDIS_STATUS_SUCCESS ) {
	return NDIS_STATUS_NOT_ACCEPTED;
    }

    PC(NdisPacket)->PacketType = PacketType;

    TI_DbgPrint(DEBUG_DATALINK, ("pretransfer LookaheadBufferSize %d packsize %d\n",LookaheadBufferSize,PacketSize));

    GetDataPtr( NdisPacket, 0, &BufferData, &PacketSize );

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
        NdisTransferData(&NdisStatus, Adapter->NdisHandle,
                         MacReceiveContext, 0, PacketSize,
			 NdisPacket, &BytesTransferred);
    }
    TI_DbgPrint(DEBUG_DATALINK, ("Calling complete\n"));

    if (NdisStatus != NDIS_STATUS_PENDING)
	ProtocolTransferDataComplete(BindingContext,
				     NdisPacket,
				     NdisStatus,
				     PacketSize);

    TI_DbgPrint(DEBUG_DATALINK, ("leaving\n"));

    return NDIS_STATUS_SUCCESS;
}


VOID NTAPI ProtocolReceiveComplete(
    NDIS_HANDLE BindingContext)
/*
 * FUNCTION: Called by NDIS when we're done receiving data
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 */
{
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}

BOOLEAN ReadIpConfiguration(PIP_INTERFACE Interface)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParameterHandle;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    WCHAR Buffer[150];
    UNICODE_STRING IPAddress = RTL_CONSTANT_STRING(L"IPAddress");
    UNICODE_STRING Netmask = RTL_CONSTANT_STRING(L"SubnetMask");
    UNICODE_STRING Gateway = RTL_CONSTANT_STRING(L"DefaultGateway");
    UNICODE_STRING EnableDhcp = RTL_CONSTANT_STRING(L"EnableDHCP");
    UNICODE_STRING Prefix = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\");
    UNICODE_STRING TcpipRegistryPath;
    UNICODE_STRING RegistryDataU;
    ANSI_STRING RegistryDataA;
    ULONG Unused;
    NTSTATUS Status;
    IP_ADDRESS DefaultMask, Router;
    
    AddrInitIPv4(&DefaultMask, 0);

    TcpipRegistryPath.MaximumLength = sizeof(WCHAR) * 150;
    TcpipRegistryPath.Length = 0;
    TcpipRegistryPath.Buffer = Buffer;
    
    /* Build the registry path */
    RtlAppendUnicodeStringToString(&TcpipRegistryPath, &Prefix);
    RtlAppendUnicodeStringToString(&TcpipRegistryPath, &Interface->Name);
    
    InitializeObjectAttributes(&ObjectAttributes,
                               &TcpipRegistryPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               0,
                               NULL);
    
    /* Open a handle to the adapter parameters */
    Status = ZwOpenKey(&ParameterHandle, KEY_READ, &ObjectAttributes);
    
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }
    else
    {
        KeyValueInfo = ExAllocatePool(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 16 * sizeof(WCHAR));
        if (!KeyValueInfo)
        {
            ZwClose(ParameterHandle);
            return FALSE;
        }
        
        /* Read the EnableDHCP entry */
        Status = ZwQueryValueKey(ParameterHandle,
                                 &EnableDhcp,
                                 KeyValuePartialInformation,
                                 KeyValueInfo,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG),
                                 &Unused);
        if (NT_SUCCESS(Status) && KeyValueInfo->DataLength == sizeof(ULONG) && (*(PULONG)KeyValueInfo->Data) == 0)
        {
            RegistryDataU.MaximumLength = 16 + sizeof(WCHAR);
            RegistryDataU.Buffer = (PWCHAR)KeyValueInfo->Data;
            
            /* Read the IP address */
            Status = ZwQueryValueKey(ParameterHandle,
                                     &IPAddress,
                                     KeyValuePartialInformation,
                                     KeyValueInfo,
                                     sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 16 * sizeof(WCHAR),
                                     &Unused);
            if (NT_SUCCESS(Status))
            {
                RegistryDataU.Length = KeyValueInfo->DataLength;
                
                RtlUnicodeStringToAnsiString(&RegistryDataA,
                                             &RegistryDataU,
                                             TRUE);
                
                AddrInitIPv4(&Interface->Unicast, inet_addr(RegistryDataA.Buffer));
                
                RtlFreeAnsiString(&RegistryDataA);
            }

            Status = ZwQueryValueKey(ParameterHandle,
                                     &Netmask,
                                     KeyValuePartialInformation,
                                     KeyValueInfo,
                                     sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 16 * sizeof(WCHAR),
                                     &Unused);
            if (NT_SUCCESS(Status))
            {
                RegistryDataU.Length = KeyValueInfo->DataLength;
                
                RtlUnicodeStringToAnsiString(&RegistryDataA,
                                             &RegistryDataU,
                                             TRUE);
                
                AddrInitIPv4(&Interface->Netmask, inet_addr(RegistryDataA.Buffer));
                
                RtlFreeAnsiString(&RegistryDataA);
            }
            
            /* We have to wait until both IP address and subnet mask
             * are read to add the interface route, but we must do it
             * before we add the default gateway */
            if (!AddrIsUnspecified(&Interface->Unicast) &&
                !AddrIsUnspecified(&Interface->Netmask))
                IPAddInterfaceRoute(Interface);
            
            /* Read default gateway info */
            Status = ZwQueryValueKey(ParameterHandle,
                                     &Gateway,
                                     KeyValuePartialInformation,
                                     KeyValueInfo,
                                     sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 16 * sizeof(WCHAR),
                                     &Unused);
            if (NT_SUCCESS(Status))
            {
                RegistryDataU.Length = KeyValueInfo->DataLength;
                
                RtlUnicodeStringToAnsiString(&RegistryDataA,
                                             &RegistryDataU,
                                             TRUE);
                
                AddrInitIPv4(&Router, inet_addr(RegistryDataA.Buffer));
                
                if (!AddrIsUnspecified(&Router))
                    RouterCreateRoute(&DefaultMask, &DefaultMask, &Router, Interface, 1);
                
                RtlFreeAnsiString(&RegistryDataA);
            }
        }
        
        ZwClose(ParameterHandle);
    }
    
    return TRUE;
}

BOOLEAN ReconfigureAdapter(PRECONFIGURE_CONTEXT Context)
{
    PLAN_ADAPTER Adapter = Context->Adapter;
    PIP_INTERFACE Interface = Adapter->Context;
    //NDIS_STATUS NdisStatus;
    IP_ADDRESS DefaultMask;

    /* Initialize the default unspecified address (0.0.0.0) */
    AddrInitIPv4(&DefaultMask, 0);
    if (Context->State == LAN_STATE_STARTED &&
        !Context->Adapter->CompletingReset)
    {
        /* Read the IP configuration */
        ReadIpConfiguration(Interface);

        /* Compute the broadcast address */
        Interface->Broadcast.Type = IP_ADDRESS_V4;
        Interface->Broadcast.Address.IPv4Address = Interface->Unicast.Address.IPv4Address |
                                                  ~Interface->Netmask.Address.IPv4Address;
    }
    else if (!Context->Adapter->CompletingReset)
    {
        /* Clear IP configuration */
        Interface->Unicast = DefaultMask;
        Interface->Netmask = DefaultMask;
        Interface->Broadcast = DefaultMask;
        
        /* Remove all interface routes */
        RouterRemoveRoutesForInterface(Interface);
        
        /* Destroy all cached neighbors */
        NBDestroyNeighborsForInterface(Interface);
    }
    
    Context->Adapter->CompletingReset = FALSE;

    /* Update the IP and link status information cached in TCP */
    TCPUpdateInterfaceIPInformation(Interface);
    TCPUpdateInterfaceLinkStatus(Interface);

    /* We're done here if the adapter isn't connected */
    if (Context->State != LAN_STATE_STARTED)
    {
        Adapter->State = Context->State;
        return TRUE;
    }
    
    /* NDIS Bug! */
#if 0
    /* Get maximum link speed */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          &Interface->Speed,
                          sizeof(UINT));
    
    if (!NT_SUCCESS(NdisStatus))
        Interface->Speed = IP_DEFAULT_LINK_SPEED;

    Adapter->Speed = Interface->Speed * 100L;
    
    /* Get maximum frame size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_FRAME_SIZE,
                          &Adapter->MTU,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return FALSE;
    
    Interface->MTU = Adapter->MTU;
    
    /* Get maximum packet size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_TOTAL_SIZE,
                          &Adapter->MaxPacketSize,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return FALSE;
#endif

    Adapter->State = Context->State;
    
    return TRUE;
}

VOID ReconfigureAdapterWorker(PVOID Context)
{
    PRECONFIGURE_CONTEXT ReconfigureContext = Context;
    
    /* Complete the reconfiguration asynchronously */
    ReconfigureAdapter(ReconfigureContext);
    
    /* Free the context */
    ExFreePool(ReconfigureContext);
}

VOID NTAPI ProtocolStatus(
    NDIS_HANDLE BindingContext,
    NDIS_STATUS GeneralStatus,
    PVOID StatusBuffer,
    UINT StatusBufferSize)
/*
 * FUNCTION: Called by NDIS when the underlying driver has changed state
 * ARGUMENTS:
 *     BindingContext   = Pointer to a device context (LAN_ADAPTER)
 *     GeneralStatus    = A general status code
 *     StatusBuffer     = Pointer to a buffer with medium-specific data
 *     StatusBufferSize = Number of bytes in StatusBuffer
 */
{
    PLAN_ADAPTER Adapter = BindingContext;
    PRECONFIGURE_CONTEXT Context;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    /* Ignore the status indication if we have no context yet. We'll get another later */
    if (!Adapter->Context)
        return;
    
    Context = ExAllocatePool(NonPagedPool, sizeof(RECONFIGURE_CONTEXT));
    if (!Context)
        return;
    
    Context->Adapter = Adapter;

    switch(GeneralStatus)
    {
        case NDIS_STATUS_MEDIA_CONNECT:
            DbgPrint("NDIS_STATUS_MEDIA_CONNECT\n");

            if (Adapter->State == LAN_STATE_STARTED)
            {
                ExFreePool(Context);
                return;
            }

            Context->State = LAN_STATE_STARTED;
            break;
            
        case NDIS_STATUS_MEDIA_DISCONNECT:
            DbgPrint("NDIS_STATUS_MEDIA_DISCONNECT\n");
            
            if (Adapter->State == LAN_STATE_STOPPED)
            {
                ExFreePool(Context);
                return;
            }
            
            Context->State = LAN_STATE_STOPPED;
            break;

        case NDIS_STATUS_RESET_START:
            Adapter->OldState = Adapter->State;
            Adapter->State = LAN_STATE_RESETTING;
            /* Nothing else to do here */
            ExFreePool(Context);
            return;

        case NDIS_STATUS_RESET_END:
            Adapter->CompletingReset = TRUE;
            Context->State = Adapter->OldState;
            break;

        default:
            DbgPrint("Unhandled status: %x", GeneralStatus);
            ExFreePool(Context);
            return;
    }

    /* Queue the work item */
    if (!ChewCreate(ReconfigureAdapterWorker, Context))
        ExFreePool(Context);
}

VOID NTAPI ProtocolStatusComplete(NDIS_HANDLE NdisBindingContext)
/*
 * FUNCTION: Called by NDIS when a status-change has occurred
 * ARGUMENTS:
 *     BindingContext = Pointer to a device context (LAN_ADAPTER)
 */
{
    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));
}

NDIS_STATUS NTAPI
ProtocolPnPEvent(
    NDIS_HANDLE NdisBindingContext,
    PNET_PNP_EVENT PnPEvent)
{
    switch(PnPEvent->NetEvent)
    {
      case NetEventSetPower:
         DbgPrint("Device transitioned to power state %ld\n", PnPEvent->Buffer);
         return NDIS_STATUS_SUCCESS;

      case NetEventQueryPower:
         DbgPrint("Device wants to go into power state %ld\n", PnPEvent->Buffer);
         return NDIS_STATUS_SUCCESS;

      case NetEventQueryRemoveDevice:
         DbgPrint("Device is about to be removed\n");
         return NDIS_STATUS_SUCCESS;

      case NetEventCancelRemoveDevice:
         DbgPrint("Device removal cancelled\n");
         return NDIS_STATUS_SUCCESS;

      default:
         DbgPrint("Unhandled event type: %ld\n", PnPEvent->NetEvent);
         return NDIS_STATUS_SUCCESS;
    }
}

VOID NTAPI ProtocolBindAdapter(
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
    PCHAR Data, OldData;
    UINT Size, OldSize;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)Context;
    KIRQL OldIrql;
    PNDIS_PACKET XmitPacket;
    PIP_INTERFACE Interface = Adapter->Context;

    TI_DbgPrint(DEBUG_DATALINK,
		("Called( NdisPacket %x, Offset %d, Adapter %x )\n",
		 NdisPacket, Offset, Adapter));

    if (Adapter->State != LAN_STATE_STARTED) {
        (*PC(NdisPacket)->DLComplete)(PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_NOT_ACCEPTED);
        return;
    }

    TI_DbgPrint(DEBUG_DATALINK,
		("Adapter Address [%02x %02x %02x %02x %02x %02x]\n",
		 Adapter->HWAddress[0] & 0xff,
		 Adapter->HWAddress[1] & 0xff,
		 Adapter->HWAddress[2] & 0xff,
		 Adapter->HWAddress[3] & 0xff,
		 Adapter->HWAddress[4] & 0xff,
		 Adapter->HWAddress[5] & 0xff));

    GetDataPtr( NdisPacket, 0, &OldData, &OldSize );

    NdisStatus = AllocatePacketWithBuffer(&XmitPacket, NULL, OldSize + Adapter->HeaderSize);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        (*PC(NdisPacket)->DLComplete)(PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_RESOURCES);
        return;
    }

    GetDataPtr(XmitPacket, 0, &Data, &Size);

    RtlCopyMemory(Data + Adapter->HeaderSize, OldData, OldSize);

    (*PC(NdisPacket)->DLComplete)(PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_SUCCESS);

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
                    ASSERT(FALSE);
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

    if (Adapter->MTU < Size) {
        /* This is NOT a pointer. MSDN explicitly says so. */
        NDIS_PER_PACKET_INFO_FROM_PACKET(NdisPacket,
                                         TcpLargeSendPacketInfo) = (PVOID)((ULONG_PTR)Adapter->MTU);
    }

    /* Update interface stats */
    Interface->Stats.OutBytes += Size;

	TcpipAcquireSpinLock( &Adapter->Lock, &OldIrql );
	TI_DbgPrint(MID_TRACE, ("NdisSend\n"));
	NdisSend(&NdisStatus, Adapter->NdisHandle, XmitPacket);
	TI_DbgPrint(MID_TRACE, ("NdisSend %s\n",
				NdisStatus == NDIS_STATUS_PENDING ?
				"Pending" : "Complete"));
	TcpipReleaseSpinLock( &Adapter->Lock, OldIrql );

	/* I had a talk with vizzini: these really ought to be here.
	 * we're supposed to see these completed by ndis *only* when
	 * status_pending is returned.  Note that this is different from
	 * the situation with IRPs. */
        if (NdisStatus != NDIS_STATUS_PENDING)
            ProtocolSendComplete((NDIS_HANDLE)Context, XmitPacket, NdisStatus);
}

static NTSTATUS
OpenRegistryKey( PNDIS_STRING RegistryPath, PHANDLE RegHandle ) {
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    InitializeObjectAttributes(&Attributes, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
    Status = ZwOpenKey(RegHandle, KEY_ALL_ACCESS, &Attributes);
    return Status;
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
    UnicodeString.Length = Information->DataLength - sizeof(WCHAR);
    UnicodeString.MaximumLength = Information->DataLength;

    String->Buffer =
	(PWCHAR)ExAllocatePool( NonPagedPool,
				UnicodeString.MaximumLength + sizeof(WCHAR) );

    if( !String->Buffer ) return STATUS_NO_MEMORY;

    String->MaximumLength = UnicodeString.MaximumLength;
    RtlCopyUnicodeString( String, &UnicodeString );

    return STATUS_SUCCESS;
}

/*
 * Utility to copy and append two unicode strings.
 *
 * IN OUT PUNICODE_STRING ResultFirst -> First string and result
 * IN     PUNICODE_STRING Second      -> Second string to append
 * IN     BOOL            Deallocate  -> TRUE: Deallocate First string before
 *                                       overwriting.
 *
 * Returns NTSTATUS.
 */

NTSTATUS NTAPI AppendUnicodeString(PUNICODE_STRING ResultFirst,
				   PUNICODE_STRING Second,
				   BOOLEAN Deallocate) {
    NTSTATUS Status;
    UNICODE_STRING Ustr = *ResultFirst;
    PWSTR new_string = ExAllocatePool
        (PagedPool,
         (ResultFirst->Length + Second->Length + sizeof(WCHAR)));
    if( !new_string ) {
	return STATUS_NO_MEMORY;
    }
    memcpy( new_string, ResultFirst->Buffer, ResultFirst->Length );
    memcpy( new_string + ResultFirst->Length / sizeof(WCHAR),
	    Second->Buffer, Second->Length );
    if( Deallocate ) RtlFreeUnicodeString(ResultFirst);
    ResultFirst->Length = Ustr.Length + Second->Length;
    ResultFirst->MaximumLength = ResultFirst->Length;
    new_string[ResultFirst->Length / sizeof(WCHAR)] = 0;
    Status = RtlCreateUnicodeString(ResultFirst,new_string) ?
	STATUS_SUCCESS : STATUS_NO_MEMORY;
    ExFreePool(new_string);
    return Status;
}

static NTSTATUS CheckForDeviceDesc( PUNICODE_STRING EnumKeyName,
                                    PUNICODE_STRING TargetKeyName,
                                    PUNICODE_STRING Name,
                                    PUNICODE_STRING DeviceDesc ) {
    UNICODE_STRING RootDevice = { 0, 0, NULL }, LinkageKeyName = { 0, 0, NULL };
    UNICODE_STRING DescKeyName = { 0, 0, NULL }, Linkage = { 0, 0, NULL };
    UNICODE_STRING BackSlash = { 0, 0, NULL };
    HANDLE DescKey = NULL, LinkageKey = NULL;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_DATALINK,("EnumKeyName %wZ\n", EnumKeyName));

    RtlInitUnicodeString(&BackSlash, L"\\");
    RtlInitUnicodeString(&Linkage, L"\\Linkage");

    RtlInitUnicodeString(&DescKeyName, L"");
    AppendUnicodeString( &DescKeyName, EnumKeyName, FALSE );
    AppendUnicodeString( &DescKeyName, &BackSlash, TRUE );
    AppendUnicodeString( &DescKeyName, TargetKeyName, TRUE );

    RtlInitUnicodeString(&LinkageKeyName, L"");
    AppendUnicodeString( &LinkageKeyName, &DescKeyName, FALSE );
    AppendUnicodeString( &LinkageKeyName, &Linkage, TRUE );

    Status = OpenRegistryKey( &LinkageKeyName, &LinkageKey );
    if( !NT_SUCCESS(Status) ) goto cleanup;

    Status = ReadStringFromRegistry( LinkageKey, L"RootDevice", &RootDevice );
    if( !NT_SUCCESS(Status) ) goto cleanup;

    if( RtlCompareUnicodeString( &RootDevice, Name, TRUE ) == 0 ) {
        Status = OpenRegistryKey( &DescKeyName, &DescKey );
        if( !NT_SUCCESS(Status) ) goto cleanup;

        Status = ReadStringFromRegistry( DescKey, L"DriverDesc", DeviceDesc );
        if( !NT_SUCCESS(Status) ) goto cleanup;

        TI_DbgPrint(DEBUG_DATALINK,("ADAPTER DESC: %wZ\n", DeviceDesc));
    } else Status = STATUS_UNSUCCESSFUL;

cleanup:
    RtlFreeUnicodeString( &RootDevice );
    RtlFreeUnicodeString( &LinkageKeyName );
    RtlFreeUnicodeString( &DescKeyName );
    if( LinkageKey ) ZwClose( LinkageKey );
    if( DescKey ) ZwClose( DescKey );

    TI_DbgPrint(DEBUG_DATALINK,("Returning %x\n", Status));

    return Status;
}

static NTSTATUS FindDeviceDescForAdapter( PUNICODE_STRING Name,
                                          PUNICODE_STRING DeviceDesc ) {
    UNICODE_STRING EnumKeyName, TargetKeyName;
    HANDLE EnumKey;
    NTSTATUS Status;
    ULONG i;
    KEY_BASIC_INFORMATION *Kbio =
        ExAllocatePool(NonPagedPool, sizeof(KEY_BASIC_INFORMATION));
    ULONG KbioLength = sizeof(KEY_BASIC_INFORMATION), ResultLength;

    RtlInitUnicodeString( DeviceDesc, NULL );

    if( !Kbio ) return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitUnicodeString
        (&EnumKeyName, CCS_ROOT L"\\Control\\Class\\" TCPIP_GUID);

    Status = OpenRegistryKey( &EnumKeyName, &EnumKey );

    if( !NT_SUCCESS(Status) ) {
        TI_DbgPrint(DEBUG_DATALINK,("Couldn't open Enum key %wZ: %x\n",
                                    &EnumKeyName, Status));
        ExFreePool( Kbio );
        return Status;
    }

    for( i = 0; NT_SUCCESS(Status); i++ ) {
        Status = ZwEnumerateKey( EnumKey, i, KeyBasicInformation,
                                 Kbio, KbioLength, &ResultLength );

        if( Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW ) {
            ExFreePool( Kbio );
            KbioLength = ResultLength;
            Kbio = ExAllocatePool( NonPagedPool, KbioLength );
            if( !Kbio ) {
                TI_DbgPrint(DEBUG_DATALINK,("Failed to allocate memory\n"));
                ZwClose( EnumKey );
                return STATUS_NO_MEMORY;
            }

            Status = ZwEnumerateKey( EnumKey, i, KeyBasicInformation,
                                     Kbio, KbioLength, &ResultLength );

            if( !NT_SUCCESS(Status) ) {
                TI_DbgPrint(DEBUG_DATALINK,("Couldn't enum key child %d\n", i));
                ZwClose( EnumKey );
                ExFreePool( Kbio );
                return Status;
            }
        }

        if( NT_SUCCESS(Status) ) {
            TargetKeyName.Length = TargetKeyName.MaximumLength =
                Kbio->NameLength;
            TargetKeyName.Buffer = Kbio->Name;

            Status = CheckForDeviceDesc
                ( &EnumKeyName, &TargetKeyName, Name, DeviceDesc );
            if( NT_SUCCESS(Status) ) {
                ZwClose( EnumKey );
                ExFreePool( Kbio );
                return Status;
            } else Status = STATUS_SUCCESS;
        }
    }

    ZwClose( EnumKey );
    ExFreePool( Kbio );
    return STATUS_UNSUCCESSFUL;
}

VOID GetName( PUNICODE_STRING RegistryKey,
              PUNICODE_STRING OutName ) {
    PWCHAR Ptr;
    UNICODE_STRING PartialRegistryKey;

    PartialRegistryKey.Buffer =
        RegistryKey->Buffer + wcslen(CCS_ROOT L"\\Services\\");
    Ptr = PartialRegistryKey.Buffer;

    while( *Ptr != L'\\' &&
           ((PCHAR)Ptr) < ((PCHAR)RegistryKey->Buffer) + RegistryKey->Length )
        Ptr++;

    PartialRegistryKey.Length = PartialRegistryKey.MaximumLength =
        (Ptr - PartialRegistryKey.Buffer) * sizeof(WCHAR);

    RtlInitUnicodeString( OutName, L"" );
    AppendUnicodeString( OutName, &PartialRegistryKey, FALSE );
}

BOOLEAN BindAdapter(
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
    ULONG Lookahead = LOOKAHEAD_SIZE;
    NTSTATUS Status;
    NDIS_MEDIA_STATE MediaState;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    Adapter->State = LAN_STATE_OPENING;

    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_LOOKAHEAD,
                          &Lookahead,
                          sizeof(ULONG));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(DEBUG_DATALINK, ("Could not set lookahead buffer size (0x%X).\n", NdisStatus));
        return FALSE;
    }

    /* Bind the adapter to IP layer */
    BindInfo.Context       = Adapter;
    BindInfo.HeaderSize    = Adapter->HeaderSize;
    BindInfo.MinFrameSize  = Adapter->MinFrameSize;
    BindInfo.Address       = (PUCHAR)&Adapter->HWAddress;
    BindInfo.AddressLength = Adapter->HWAddressLength;
    BindInfo.Transmit      = LANTransmit;

    IF = IPCreateInterface(&BindInfo);

    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return FALSE;
    }

    /*
     * Query per-adapter configuration from the registry
     * In case anyone is curious:  there *is* an Ndis configuration api
     * for this sort of thing, but it doesn't really support things like
     * REG_MULTI_SZ very well, and there is a note in the DDK that says that
     * protocol drivers developed for win2k and above just use the native
     * services (ZwOpenKey, etc).
     */

    GetName( RegistryPath, &IF->Name );

    Status = FindDeviceDescForAdapter( &IF->Name, &IF->Description );
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to get device description.\n"));
        IPDestroyInterface(IF);
        return FALSE;
    }

    TI_DbgPrint(DEBUG_DATALINK,("Adapter Description: %wZ\n",
                &IF->Description));
    
    /* Get maximum link speed */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          &IF->Speed,
                          sizeof(UINT));
    
    if (!NT_SUCCESS(NdisStatus))
        IF->Speed = IP_DEFAULT_LINK_SPEED;
    
    Adapter->Speed = IF->Speed * 100L;
    
    /* Get maximum frame size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_FRAME_SIZE,
                          &Adapter->MTU,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return FALSE;
    
    IF->MTU = Adapter->MTU;
    
    /* Get maximum packet size */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_TOTAL_SIZE,
                          &Adapter->MaxPacketSize,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return FALSE;

    /* Register interface with IP layer */
    IPRegisterInterface(IF);

    /* Store adapter context */
    Adapter->Context = IF;

    /* Get the media state */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestQueryInformation,
                          OID_GEN_MEDIA_CONNECT_STATUS,
                          &MediaState,
                          sizeof(MediaState));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(DEBUG_DATALINK, ("Could not query media status (0x%X).\n", NdisStatus));
        IPUnregisterInterface(IF);
        IPDestroyInterface(IF);
        return FALSE;
    }

    /* Indicate the current media state */
    ProtocolStatus(Adapter,
                   (MediaState == NdisMediaStateConnected) ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
                   NULL, 0);

    /* Set packet filter so we can send and receive packets */
    NdisStatus = NDISCall(Adapter,
                          NdisRequestSetInformation,
                          OID_GEN_CURRENT_PACKET_FILTER,
                          &Adapter->PacketFilter,
                          sizeof(UINT));

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TI_DbgPrint(DEBUG_DATALINK, ("Could not set packet filter (0x%X).\n", NdisStatus));
        IPUnregisterInterface(IF);
        IPDestroyInterface(IF);
        return FALSE;
    }

    return TRUE;
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

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    IF = ExAllocatePoolWithTag(NonPagedPool, sizeof(LAN_ADAPTER), LAN_ADAPTER_TAG);
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
	TI_DbgPrint(DEBUG_DATALINK,("denying adapter %wZ\n", AdapterName));
	ExFreePoolWithTag(IF, LAN_ADAPTER_TAG);
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
        ExFreePoolWithTag(IF, LAN_ADAPTER_TAG);
        return NDIS_STATUS_NOT_SUPPORTED;
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
        ExFreePoolWithTag(IF, LAN_ADAPTER_TAG);
        return NdisStatus;
    }

    /* Bind adapter to IP layer */
    if( !BindAdapter(IF, RegistryPath) ) {
	TI_DbgPrint(DEBUG_DATALINK,("denying adapter %wZ (BindAdapter)\n", AdapterName));
	ExFreePoolWithTag(IF, LAN_ADAPTER_TAG);
	return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Add adapter to the adapter list */
    ExInterlockedInsertTailList(&AdapterListHead,
                                &IF->ListEntry,
                                &AdapterListLock);

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

    return NdisStatus;
}

VOID 
NTAPI
LANUnregisterProtocol(VOID)
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

VOID
NTAPI
ProtocolUnbindAdapter(
    PNDIS_STATUS Status,
    NDIS_HANDLE ProtocolBindingContext,
    NDIS_HANDLE UnbindContext)
{
    /* We don't pend any unbinding so we can just ignore UnbindContext */
    *Status = LANUnregisterAdapter((PLAN_ADAPTER)ProtocolBindingContext);
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
    ProtChars.ReceivePacketHandler           = ProtocolReceivePacket;
    ProtChars.ReceiveHandler                 = ProtocolReceive;
    ProtChars.ReceiveCompleteHandler         = ProtocolReceiveComplete;
    ProtChars.StatusHandler                  = ProtocolStatus;
    ProtChars.StatusCompleteHandler          = ProtocolStatusComplete;
    ProtChars.BindAdapterHandler             = ProtocolBindAdapter;
    ProtChars.PnPEventHandler                = ProtocolPnPEvent;
    ProtChars.UnbindAdapterHandler           = ProtocolUnbindAdapter;
    ProtChars.UnloadHandler                  = LANUnregisterProtocol;

    /* Try to register protocol */
    NdisRegisterProtocol(&NdisStatus,
                         &NdisProtocolHandle,
                         &ProtChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TI_DbgPrint(DEBUG_DATALINK, ("NdisRegisterProtocol failed, status 0x%x\n", NdisStatus));
        return (NTSTATUS)NdisStatus;
    }

    ProtocolRegistered = TRUE;

    return STATUS_SUCCESS;
}

/* EOF */
