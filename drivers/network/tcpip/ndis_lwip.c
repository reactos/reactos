/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/ndis_proto.c
 * PURPOSE:         tcpip.sys: ndis protocol bindings
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

// FIXME: Maybe more in the future ?
static NDIS_MEDIUM NdisMediaArray[] = { NdisMedium802_3 };

/* The handle we got from NDIS for this protocol */
static NDIS_HANDLE NdisHandle;

/* Initializes the various values lwip will need */
static
NDIS_STATUS
InitializeInterface(
    _Inout_ TCPIP_INTERFACE* Interface)
{
    NDIS_STATUS Status;
    NDIS_REQUEST Request;
    UINT MTU, Speed;
    NDIS_OID QueryAddrOid;
    UINT PacketFilter;

    /* Add this interface into the entities DB */
    InsertEntityInstance(CL_NL_ENTITY, &Interface->ClNlInstance);
    InsertEntityInstance(IF_ENTITY, &Interface->IfInstance);
    InsertEntityInstance(AT_ENTITY, &Interface->AtInstance);

    /* Get the MTU from the NIC */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAXIMUM_FRAME_SIZE;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &MTU;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(UINT);
    NdisRequest(&Status, Interface->NdisContext, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not get MTU from the NIC driver!\n");
        return Status;
    }
    Interface->lwip_netif.mtu = MTU;

    /* Setup media type related data. */
    switch (NdisMediaArray[Interface->MediumIndex])
    {
        case NdisMedium802_3:
            Interface->lwip_netif.hwaddr_len = ETHARP_HWADDR_LEN;
            QueryAddrOid = OID_802_3_CURRENT_ADDRESS;
            PacketFilter = NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_MULTICAST;
            break;
        default:
            /* This is currently impossible */
            DPRINT1("Unknown medium!\n");
            NT_ASSERT(FALSE);
            return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    /* Get the address */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = QueryAddrOid;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &Interface->lwip_netif.hwaddr[0];
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = NETIF_MAX_HWADDR_LEN;
    NdisRequest(&Status, Interface->NdisContext, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not get HW address from the NIC driver!\n");
        return Status;
    }

    /* Get the link speed */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_LINK_SPEED;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &Speed;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(UINT);
    NdisRequest(&Status, Interface->NdisContext, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not get link speed NIC driver!\n");
        /* Good old 10Mb/s as default */
        Speed = 100000;
    }
    /* NDIS drivers give it in 100bps unit */
    Speed *= 100;

    /* Initialize lwip SNMP module */
    NETIF_INIT_SNMP(&Interface->lwip_netif, snmp_ifType_ethernet_csmacd, Speed);

    /* Set the packet filter */
    Request.RequestType = NdisRequestSetInformation;
    Request.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
    Request.DATA.SET_INFORMATION.InformationBuffer = &PacketFilter;
    Request.DATA.SET_INFORMATION.InformationBufferLength = sizeof(PacketFilter);
    NdisRequest(&Status, Interface->NdisContext, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not get HW address from the NIC driver!\n");
        return Status;
    }

    /* Initialize the packet pool */
    NdisAllocatePacketPool(&Status, &Interface->PacketPool, TCPIP_PACKETPOOL_SIZE, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not allocate a packet pool.\n");
        return Status;
    }

    /* Initialize the buffer pool */
    NdisAllocateBufferPool(&Status, &Interface->BufferPool, TCPIP_BUFFERPOOL_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not allocate a buffer pool.\n");
        return Status;
    }

    return NDIS_STATUS_SUCCESS;
}

/* The various callbacks we will give to NDIS */
static
VOID
NTAPI
ProtocolOpenAdapterComplete(
    _In_ NDIS_HANDLE ProtocolBindingContext,
    _In_ NDIS_STATUS Status,
    _In_ NDIS_STATUS OpenErrorStatus)
{
    TCPIP_INTERFACE* Interface = (TCPIP_INTERFACE*)ProtocolBindingContext;

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Interface, TAG_INTERFACE);
        return;
    }

    Status = InitializeInterface(Interface);
    if (!NT_SUCCESS(Status))
    {
        /* Unbind the interface and that's all */
        NdisCloseAdapter(&Status, Interface->NdisContext);
    }
}

static
VOID
NTAPI
ProtocolCloseAdapterComplete(
    _In_  NDIS_HANDLE ProtocolBindingContext,
    _In_  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}

static
VOID
NTAPI
ProtocolResetComplete(
    _In_  NDIS_HANDLE ProtocolBindingContext,
    _In_  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}

static
VOID
NTAPI
ProtocolRequestComplete(
    _In_  NDIS_HANDLE ProtocolBindingContext,
    _In_  PNDIS_REQUEST NdisRequest,
    _In_  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}

static
NDIS_STATUS
NTAPI
ProtocolReceive(
    _In_  NDIS_HANDLE ProtocolBindingContext,
    _In_  NDIS_HANDLE MacReceiveContext,
    _In_  PVOID HeaderBuffer,
    _In_  UINT HeaderBufferSize,
    _In_  PVOID LookAheadBuffer,
    _In_  UINT LookaheadBufferSize,
    _In_  UINT PacketSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static
VOID
NTAPI
ProtocolReceiveComplete(
    _In_  NDIS_HANDLE ProtocolBindingContext)
{
    UNIMPLEMENTED
}

static
VOID
NTAPI
ProtocolStatusComplete(
    _In_  NDIS_HANDLE ProtocolBindingContext)
{
    UNIMPLEMENTED
}

static
INT
NTAPI
ProtocolReceivePacket(
    _In_  NDIS_HANDLE ProtocolBindingContext,
    _In_  PNDIS_PACKET Packet)
{
    UNIMPLEMENTED
    return 0;
}

/* bridge between NDIS and lwip: send data to the adapter */
static
err_t
lwip_netif_linkoutput(
    struct netif *netif,
    struct pbuf *p)
{
    TCPIP_INTERFACE* Interface = CONTAINING_RECORD(netif, TCPIP_INTERFACE, lwip_netif);
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    PVOID PayloadCopy = NULL;

    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    /* Allocate a packet */
    NdisAllocatePacket(&Status, &Packet, Interface->PacketPool);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not allocate a packet from packet pool!\n");
        return ERR_MEM;
    }

    /* Map pbuf to a NDIS buffer chain, if possible (== allocated from non paged pool). */
    if ((p->type == PBUF_POOL) || (p->type == PBUF_RAM))
    {
        while (p)
        {
            NdisAllocateBuffer(&Status, &Buffer, Interface->BufferPool, p->payload, p->len);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Could not allocate a buffer!\n");
                return ERR_MEM;
            }
            NdisChainBufferAtBack(Packet, Buffer);
            p = p->next;
        }
    }
    else
    {
        PayloadCopy = ExAllocatePoolWithTag(NonPagedPool, p->tot_len, TAG_INTERFACE);
        if (!PayloadCopy)
        {
            NdisFreePacket(Packet);
            return ERR_MEM;
        }
        pbuf_copy_partial(p, PayloadCopy, p->tot_len, 0);
        NdisAllocateBuffer(&Status, &Buffer, Interface->BufferPool, p->payload, p->len);
        NdisChainBufferAtFront(Packet, Buffer);
    }

    /* Call ndis */
    NdisSend(&Status, Interface->NdisContext, Packet);

    DPRINT1("NdisSend: got status 0x%08x.\n", Status);

    /* Free the buffer chain */
    if (Status != NDIS_STATUS_PENDING)
    {
        NdisUnchainBufferAtFront(Packet, &Buffer);
        while (Buffer)
        {
            NdisFreeBuffer(Buffer);
            NdisUnchainBufferAtFront(Packet, &Buffer);
        }
        NdisFreePacket(Packet);

        if (PayloadCopy)
            ExFreePoolWithTag(PayloadCopy, TAG_INTERFACE);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NdisSend returned status 0x%08x.\n", Status);
        return ERR_CONN;
    }

    return ERR_OK;
}

/* lwip interface initialisation function */
static
err_t
lwip_netif_init(
    struct netif* lwip_netif)
{
    /* Set output callbacks */
    lwip_netif->output = etharp_output;
    lwip_netif->linkoutput = lwip_netif_linkoutput;

    /* We use ARP and broadcasting */
    lwip_netif->flags |= NETIF_FLAG_ETHARP | NETIF_FLAG_BROADCAST;

    /* Let's say we're ethernet */
    lwip_netif->name[0] = 'e';
    lwip_netif->name[1] = 'n';

    return ERR_OK;
}

static
VOID
NTAPI
ProtocolBindAdapter(
  _Out_  PNDIS_STATUS Status,
  _In_   NDIS_HANDLE BindContext,
  _In_   PNDIS_STRING DeviceName,
  _In_   PVOID SystemSpecific1,
  _In_   PVOID SystemSpecific2)
{
    TCPIP_INTERFACE* Interface;
    NDIS_STATUS OpenErrorStatus;
    UNICODE_STRING RealDeviceName;
    struct ip_addr IpAddr, SubnetMask, GatewayAddr;
    err_t lwip_error;

    /* The device name comes in the \Device\AdapterName form */
    RealDeviceName.Buffer = DeviceName->Buffer + 8;
    RealDeviceName.Length = DeviceName->Length - (8 * sizeof(WCHAR));
    RealDeviceName.MaximumLength = DeviceName->MaximumLength - (8 * sizeof(WCHAR));

    /* Allocate an interface for this NIC */
    Interface = ExAllocatePoolWithTag(PagedPool,
        sizeof(*Interface) + RealDeviceName.MaximumLength,
        TAG_INTERFACE);
    if (!Interface)
    {
        DPRINT1("Could not allocate an interface structure!\n");
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    /* Copy the device name */
    RtlInitEmptyUnicodeString(&Interface->DeviceName, (PWSTR)(Interface + 1), RealDeviceName.MaximumLength);
    RtlCopyUnicodeString(&Interface->DeviceName, &RealDeviceName);

    /* Add the interface to the lwip list */
    ip_addr_set_zero(&IpAddr);
    ip_addr_set_zero(&SubnetMask);
    ip_addr_set_zero(&GatewayAddr);
    lwip_error = netifapi_netif_add(
        &Interface->lwip_netif,
        &IpAddr,
        &SubnetMask,
        &GatewayAddr,
        Interface,
        lwip_netif_init,
        ethernet_input);
    if (lwip_error != ERR_OK)
    {
        DPRINT1("netifapi_netif_add failed with error %d.\n", lwip_error);
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    /* Get a adapter handle from NDIS */
    NdisOpenAdapter(
        Status,
        &OpenErrorStatus,
        &Interface->NdisContext,
        &Interface->MediumIndex,
        NdisMediaArray,
        sizeof(NdisMediaArray) / sizeof(NdisMediaArray[0]),
        NdisHandle,
        Interface,
        DeviceName,
        0,
        NULL);
    if (*Status == NDIS_STATUS_PENDING)
    {
        /* Silently return, as the binding will be finished in the async call */
        return;
    }

    if (*Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("NdisOpenAdapter failed with status 0x%08x.\n", *Status);
        ExFreePoolWithTag(Interface, TAG_INTERFACE);
        return;
    }

    /* Finish the bind request in sync */
    *Status = InitializeInterface(Interface);
}

static
VOID
NTAPI
ProtocolUnbindAdapter(
  _Out_  PNDIS_STATUS Status,
  _In_   NDIS_HANDLE ProtocolBindingContext,
  _In_   NDIS_HANDLE UnbindContext)
{
    UNIMPLEMENTED
}

NTSTATUS
TcpIpRegisterNdisProtocol(void)
{
    NDIS_STATUS Status;
    NDIS_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics;

    /* Simply fill in the structure and pass it to the NDIS driver. */
    RtlZeroMemory(&ProtocolCharacteristics, sizeof(ProtocolCharacteristics));
    ProtocolCharacteristics.MajorNdisVersion = NDIS_PROTOCOL_MAJOR_VERSION;
    ProtocolCharacteristics.MinorNdisVersion = NDIS_PROTOCOL_MINOR_VERSION;
    ProtocolCharacteristics.Reserved = 0;
    ProtocolCharacteristics.OpenAdapterCompleteHandler = ProtocolOpenAdapterComplete;
    ProtocolCharacteristics.CloseAdapterCompleteHandler = ProtocolCloseAdapterComplete;
    ProtocolCharacteristics.ResetCompleteHandler = ProtocolResetComplete;
    ProtocolCharacteristics.RequestCompleteHandler = ProtocolRequestComplete;
    ProtocolCharacteristics.ReceiveHandler = ProtocolReceive;
    ProtocolCharacteristics.ReceiveCompleteHandler = ProtocolReceiveComplete;
    ProtocolCharacteristics.StatusCompleteHandler = ProtocolStatusComplete;
    ProtocolCharacteristics.ReceivePacketHandler = ProtocolReceivePacket;
    ProtocolCharacteristics.BindAdapterHandler = ProtocolBindAdapter;
    ProtocolCharacteristics.UnbindAdapterHandler = ProtocolUnbindAdapter;
    RtlInitUnicodeString(&ProtocolCharacteristics.Name, L"TcpIp");

    NdisRegisterProtocol(&Status, &NdisHandle, &ProtocolCharacteristics, sizeof(ProtocolCharacteristics));

    return Status;
}

void
TcpIpUnregisterNdisProtocol(void)
{
    NDIS_STATUS Status;

    NdisDeregisterProtocol(&Status, NdisHandle);
}
