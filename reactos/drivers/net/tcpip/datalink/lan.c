/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/lan.c
 * PURPOSE:     Local Area Network media routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <lan.h>
#include <address.h>
#include <routines.h>
#include <transmit.h>
#include <receive.h>
#include <arp.h>

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


PNDIS_PACKET AllocateTDPacket(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Allocates an NDIS packet for NdisTransferData
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure
 * RETURNS:
 *     Pointer to NDIS packet or NULL if there was not enough free
 *     non-paged memory
 */
{
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER Buffer;
    PVOID Data;

    NdisAllocatePacket(&NdisStatus, &NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return NULL;

    Data = ExAllocatePool(NonPagedPool, Adapter->MTU);
    if (!Data) {
        NdisFreePacket(NdisPacket);
        return NULL;
    }
        
    NdisAllocateBuffer(&NdisStatus,
                      &Buffer,
                      GlobalBufferPool,
                      Data,
                      Adapter->MTU);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NdisFreePacket(NdisPacket);
        ExFreePool(Data);
        return NULL;
    }

    NdisChainBufferAtFront(NdisPacket, Buffer);

    PC(NdisPacket)->Context = NULL; /* End of list */

    return NdisPacket;
}


VOID FreeTDPackets(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Frees transfer data packets
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure
 */
{
    PNDIS_PACKET NdisPacket, Next;

    /* Release transfer data packets */
    NdisPacket = Adapter->TDPackets;
    while (NdisPacket) {
        Next = PC(NdisPacket)->Context;
        FreeNdisPacket(NdisPacket);
        NdisPacket = Next;
    }
    Adapter->TDPackets = NULL;
}


VOID FreeAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Frees memory for a LAN_ADAPTER structure
 * ARGUMENTS:
 *     Adapter = Pointer to LAN_ADAPTER structure to free
 */
{
    FreeTDPackets(Adapter);
    ExFreePool(Adapter);
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
	PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    AdjustPacket(Packet, Adapter->HeaderSize, PC(Packet)->DLOffset);

    (*PC(Packet)->DLComplete)(Adapter->Context, Packet, Status);
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
    UINT PacketType;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    if (Status == NDIS_STATUS_SUCCESS) {
        PNDIS_BUFFER NdisBuffer;
        IP_PACKET IPPacket;

        IPPacket.NdisPacket = Packet;

        NdisGetFirstBufferFromPacket(Packet,
                                     &NdisBuffer,
                                     &IPPacket.Header,
                                     &IPPacket.ContigSize,
                                     &IPPacket.TotalSize);

        /* Determine which upper layer protocol that should receive
           this packet and pass it to the correct receive handler */
        PacketType = ((PETH_HEADER)IPPacket.Header)->EType;
        switch (PacketType) {
            case ETYPE_IPv4:
            case ETYPE_IPv6:
                IPReceive(Adapter->Context, &IPPacket);
                break;
            case ETYPE_ARP:
                ARPReceive(Adapter->Context, &IPPacket);
            default:
                break;
        }
    }

    /* Release the packet descriptor */
    KeAcquireSpinLockAtDpcLevel(&Adapter->Lock);

    PC(Packet)->Context = Adapter->TDPackets;
    Adapter->TDPackets  = Packet;

    KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);
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
    UINT PacketType;
    IP_PACKET IPPacket;
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER NdisBuffer;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;
    PETH_HEADER EHeader  = (PETH_HEADER)HeaderBuffer;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

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

    KeAcquireSpinLockAtDpcLevel(&Adapter->Lock);

    NdisPacket = Adapter->TDPackets;
    if (NdisPacket == (PNDIS_PACKET)NULL) {
        TI_DbgPrint(DEBUG_DATALINK, ("No available packet descriptors.\n"));
        /* We don't have a free packet descriptor. Drop the packet */
        KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);
        return NDIS_STATUS_SUCCESS;
    }
    Adapter->TDPackets = PC(NdisPacket)->Context;

    KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);

    if (LookaheadBufferSize < PacketSize) {
        NDIS_STATUS NdisStatus;
        UINT BytesTransferred;

        /* Get the data */
        NdisTransferData(&NdisStatus,
                         Adapter->NdisHandle,
                         MacReceiveContext,
                         0,
                         PacketSize,
                         NdisPacket,
                         &BytesTransferred);
        if (NdisStatus != NDIS_STATUS_PENDING)
            ProtocolTransferDataComplete(BindingContext,
                                         NdisPacket,
                                         NdisStatus,
                                         BytesTransferred);

        return NDIS_STATUS_SUCCESS;
    }

    /* We got all the data in the lookahead buffer */

    IPPacket.NdisPacket = NdisPacket;

    NdisGetFirstBufferFromPacket(NdisPacket,
                                 &NdisBuffer,
                                 &IPPacket.Header,
                                 &IPPacket.ContigSize,
                                 &IPPacket.TotalSize);

    RtlCopyMemory(IPPacket.Header, LookaheadBuffer, PacketSize);

    switch (PacketType) {
        case ETYPE_IPv4:
        case ETYPE_IPv6:
            IPReceive(Adapter->Context, &IPPacket);
            break;
        case ETYPE_ARP:
            ARPReceive(Adapter->Context, &IPPacket);
            break;
        default:
            break;
    }

    /* Release the packet descriptor */
    KeAcquireSpinLockAtDpcLevel(&Adapter->Lock);

    PC(NdisPacket)->Context = Adapter->TDPackets;
    Adapter->TDPackets      = NdisPacket;

    KeReleaseSpinLockFromDpcLevel(&Adapter->Lock);

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
	TI_DbgPrint(DEBUG_DATALINK, ("Called with registry path %wZ\n", SystemSpecific1));
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
    PVOID Data;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)Context;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    /* NDIS send routines don't have an offset argument so we
       must offset the data in upper layers and adjust the
       packet here. We save the offset in the packet context
       area so it can be undone before we release the packet */
    Data = AdjustPacket(NdisPacket, Offset, Adapter->HeaderSize);
    PC(NdisPacket)->DLOffset = Offset;

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

        NdisSend(&NdisStatus, Adapter->NdisHandle, NdisPacket);
        if (NdisStatus != NDIS_STATUS_PENDING)
            ProtocolSendComplete((NDIS_HANDLE)Context, NdisPacket, NdisStatus);
    } else {
        ProtocolSendComplete((NDIS_HANDLE)Context, NdisPacket, NDIS_STATUS_CLOSED);
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
					   PIP_ADDRESS *Address ) {
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
    
    AnsiAddress.Buffer = ExAllocatePoolWithTag(PagedPool, AnsiLen, 0x01020304);
    if(!AnsiAddress.Buffer)
	return STATUS_NO_MEMORY;

    AnsiAddress.Length = AnsiLen;
    AnsiAddress.MaximumLength = AnsiLen;
    
    Status = RtlUnicodeStringToAnsiString(&AnsiAddress, &UnicodeAddress, FALSE);
    if (!NT_SUCCESS(Status)) {
	ExFreePool(AnsiAddress.Buffer);
	return STATUS_UNSUCCESSFUL;
    }
    
    AnsiAddress.Buffer[AnsiAddress.Length] = 0;
    *Address = AddrBuildIPv4(inet_addr(AnsiAddress.Buffer));
    if (!Address) {
	ExFreePool(AnsiAddress.Buffer);
	return STATUS_UNSUCCESSFUL;
    }

    return *Address ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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
    INT i;
    PIP_INTERFACE IF;
    PIP_ADDRESS Address = 0;
    PIP_ADDRESS Netmask = 0;
    PNDIS_PACKET Packet;
    NDIS_STATUS NdisStatus;
    LLIP_BIND_INFO BindInfo;
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

    /* Allocate packets for NdisTransferData */
    /* FIXME: How many should we allocate? */
    Adapter->TDPackets = NULL;
    for (i = 0; i < 2; i++) {
        Packet              = AllocateTDPacket(Adapter);
        if (!Packet) {
            TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            FreeTDPackets(Adapter);
            return;
        }
        PC(Packet)->Context = Adapter->TDPackets;
        Adapter->TDPackets  = Packet;
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
        FreeTDPackets(Adapter);
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
	Status = ReadIPAddressFromRegistry( RegHandle, L"IPAddress",
					    &Address );
    if(NT_SUCCESS(Status)) 
	Status = ReadIPAddressFromRegistry( RegHandle, L"SubnetMask",
					    &Netmask );
    
    if(!NT_SUCCESS(Status) || !Address || !Netmask)
    {
	TI_DbgPrint(MIN_TRACE, ("Unable to open protocol-specific registry key: 0x%x\n", Status));
	
	/* XXX how do we proceed?  No ip address, no parameters... do we guess? */
	if(RegHandle)  
	    ZwClose(RegHandle);
	if(Address) Address->Free(Address);
	if(Netmask) Netmask->Free(Netmask);
	FreeTDPackets(Adapter);
	IPDestroyInterface(IF);
	return;
    }
    
    TI_DbgPrint
	(MID_TRACE, 
	 ("--> Our IP address on this interface: '%s'\n", 
	  A2S(Address)));
    
    TI_DbgPrint
	(MID_TRACE, 
	 ("--> Our net mask on this interface: '%s'\n", 
	  A2S(Netmask)));

    /* Create a net table entry for this interface */
    if (!IPCreateNTE(IF, Address, AddrCountPrefixBits(Netmask))) {
	Netmask->Free(Netmask);
        TI_DbgPrint(MIN_TRACE, ("IPCreateNTE() failed.\n"));
        FreeTDPackets(Adapter);
        IPDestroyInterface(IF);
        return;
    }

    Netmask->Free(Netmask);

    /* Reference the interface for the NTE. The reference
       for the address is just passed on to the NTE */
    ReferenceObject(IF);

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
        FreeTDPackets(Adapter);
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

        /* Free transfer data packets */
        FreeTDPackets(Adapter);
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
		PNDIS_CONFIGURATION_PARAMETER Parameter;
    UINT MediaIndex;
    NDIS_MEDIUM MediaArray[MAX_MEDIA];
    UINT AddressOID;
    UINT Speed;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    IF = ExAllocatePool(NonPagedPool, sizeof(LAN_ADAPTER));
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
        ExFreePool(IF);
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
        ExFreePool(IF);
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    /* Get maximum frame size */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_MAXIMUM_FRAME_SIZE,
                          &IF->MTU,
                          sizeof(UINT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ExFreePool(IF);
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
        ExFreePool(IF);
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
        ExFreePool(IF);
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
        ExFreePool(IF);
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

        KeAcquireSpinLock(&AdapterListLock, &OldIrql);

        /* Search the list and remove every adapter we find */
        CurrentEntry = AdapterListHead.Flink;
        while (CurrentEntry != &AdapterListHead) {
            NextEntry = CurrentEntry->Flink;
	        Current = CONTAINING_RECORD(CurrentEntry, LAN_ADAPTER, ListEntry);
            /* Unregister it */
            LANUnregisterAdapter(Current);
            CurrentEntry = NextEntry;
        }

        KeReleaseSpinLock(&AdapterListLock, OldIrql);

        NdisDeregisterProtocol(&NdisStatus, NdisProtocolHandle);
        ProtocolRegistered = FALSE;
    }
}

/* EOF */
