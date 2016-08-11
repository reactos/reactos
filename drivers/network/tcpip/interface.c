/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/interface.c
 * PURPOSE:         tcpip.sys: ndis <-> lwip bridge implementation
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

TCPIP_INTERFACE* LoopbackInterface;

static
VOID
GetInterfaceOperStatus(
    _In_ TCPIP_INTERFACE* Interface,
    _Out_ ULONG* OperStatus)
{
    NDIS_STATUS Status;
    NDIS_REQUEST Request;
    NDIS_MEDIA_STATE MediaState;

    if (Interface->NdisContext == NULL)
    {
        /* This is the looback interface */
        *OperStatus = MIB_IF_OPER_STATUS_CONNECTED;
        return;
    }

    /* Get the connection status from NDIS */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MEDIA_CONNECT_STATUS;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &MediaState;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);
    NdisRequest(&Status, Interface->NdisContext, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not get connection status from the NIC driver. Status 0x%08x\n", Status);
        *OperStatus = MIB_IF_OPER_STATUS_NON_OPERATIONAL;
        return;
    }

    switch(MediaState)
    {
        case NdisMediaStateConnected:
            *OperStatus = MIB_IF_OPER_STATUS_CONNECTED;
            break;
        case NdisMediaStateDisconnected:
            *OperStatus = MIB_IF_OPER_STATUS_DISCONNECTED;
            break;
        default:
            DPRINT1("Got unknown media state from NIC driver: %d.\n", MediaState);
            *OperStatus = MIB_IF_OPER_STATUS_NON_OPERATIONAL;
    }
}

NTSTATUS
QueryInterfaceEntry(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize)
{
    TCPIP_INSTANCE* Instance;
    TCPIP_INTERFACE* Interface;
    NTSTATUS Status;
    IFEntry* IfEntry = OutBuffer;
    ULONG NeededSize;

    NT_ASSERT(ID.tei_entity == IF_ENTITY);

    Status = GetInstance(ID, &Instance);
    if (!NT_SUCCESS(Status))
        return Status;

    Interface = CONTAINING_RECORD(Instance, TCPIP_INTERFACE, IfInstance);

    NeededSize = FIELD_OFFSET(IFEntry, if_descr) +
            RTL_FIELD_SIZE(IFEntry, if_descr[0]) * (Interface->DeviceName.Length / 2 + 1);

    if (!OutBuffer)
    {
        *BufferSize = NeededSize;
        return STATUS_SUCCESS;
    }

    if (*BufferSize < NeededSize)
    {
        *BufferSize = NeededSize;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Fill in the data from our interface */
    IfEntry->if_index = Interface->lwip_netif.num;
    IfEntry->if_type = (Interface->lwip_netif.flags & NETIF_FLAG_ETHARP) ?
            IF_TYPE_ETHERNET_CSMACD : IF_TYPE_SOFTWARE_LOOPBACK;
    IfEntry->if_mtu = Interface->lwip_netif.mtu;
    IfEntry->if_speed = Interface->Speed;
    IfEntry->if_physaddrlen = Interface->lwip_netif.hwaddr_len;
    RtlCopyMemory(IfEntry->if_physaddr, Interface->lwip_netif.hwaddr, IfEntry->if_physaddrlen);
    IfEntry->if_adminstatus = MIB_IF_ADMIN_STATUS_UP;
    GetInterfaceOperStatus(Interface, &IfEntry->if_operstatus);

    // FIXME: Fill those
    IfEntry->if_lastchange = 0;
    IfEntry->if_inoctets = 0;
    IfEntry->if_inucastpkts = 0;
    IfEntry->if_innucastpkts = 0;
    IfEntry->if_inerrors = 0;
    IfEntry->if_inunknownprotos = 0;
    IfEntry->if_outoctets = 0;
    IfEntry->if_outucastpkts = 0;
    IfEntry->if_outnucastpkts = 0;
    IfEntry->if_outdiscards = 0;
    IfEntry->if_outerrors = 0;
    IfEntry->if_outqlen = 0;

    /* Set name */
    RtlUnicodeToMultiByteN(
        (PCHAR)&IfEntry->if_descr[0],
        *BufferSize - FIELD_OFFSET(IFEntry, if_descr),
        &IfEntry->if_descrlen,
        Interface->DeviceName.Buffer,
        Interface->DeviceName.Length);

    *BufferSize = NeededSize;

    return STATUS_SUCCESS;
}

NTSTATUS
QueryInterfaceAddrTable(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize)
{
    TCPIP_INSTANCE* Instance;
    TCPIP_INTERFACE* Interface;
    NTSTATUS Status;
    IPAddrEntry* AddrEntry = OutBuffer;

    NT_ASSERT(ID.tei_entity == CL_NL_ENTITY);

    Status = GetInstance(ID, &Instance);
    if (!NT_SUCCESS(Status))
        return Status;

    Interface = CONTAINING_RECORD(Instance, TCPIP_INTERFACE, ClNlInstance);

    // FIXME: return more than 'one' address
    if (!OutBuffer)
    {
        *BufferSize = sizeof(IPAddrEntry);
        return STATUS_SUCCESS;
    }

    if (*BufferSize < sizeof(IPAddrEntry))
    {
        *BufferSize = sizeof(IPAddrEntry);
        return STATUS_BUFFER_OVERFLOW;
    }

    AddrEntry->iae_addr = Interface->lwip_netif.ip_addr.addr;
    AddrEntry->iae_index = Interface->lwip_netif.num;
    AddrEntry->iae_mask = Interface->lwip_netif.netmask.addr;
    _BitScanReverse(&AddrEntry->iae_bcastaddr, AddrEntry->iae_addr | ~AddrEntry->iae_mask);
    /* FIXME: set those */
    AddrEntry->iae_reasmsize = 0;
    AddrEntry->iae_context = 0;
    AddrEntry->iae_pad = 0;

    return STATUS_SUCCESS;
}

/* callback provided to lwip for initializing the loopback interface */
static
err_t
lwip_netif_loopback_init(struct netif *netif)
{
    NETIF_INIT_SNMP(netif, snmp_ifType_softwareLoopback, 0);

    netif->name[0] = 'l';
    netif->name[1] = 'o';
    netif->output = netif_loop_output;
    return ERR_OK;
}

NTSTATUS
TcpIpCreateLoopbackInterface(void)
{
    err_t lwip_error;
    struct ip_addr IpAddr, SubnetMask, GatewayAddr;

    LoopbackInterface = ExAllocatePoolWithTag(NonPagedPool, sizeof(*LoopbackInterface), TAG_INTERFACE);
    if (!LoopbackInterface)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(LoopbackInterface, sizeof(*LoopbackInterface));

    /* Add it to lwip stack */
    IP4_ADDR(&GatewayAddr, 127,0,0,1);
    IP4_ADDR(&IpAddr, 127,0,0,1);
    IP4_ADDR(&SubnetMask, 255,0,0,0);
    ACQUIRE_SERIAL_MUTEX_NO_TO();
    lwip_error = netifapi_netif_add(
        &LoopbackInterface->lwip_netif,
        &IpAddr,
        &SubnetMask,
        &GatewayAddr,
        LoopbackInterface,
        lwip_netif_loopback_init,
        tcpip_input);
    if (lwip_error != ERR_OK)
    {
        ExFreePoolWithTag(LoopbackInterface, TAG_INTERFACE);
        RELEASE_SERIAL_MUTEX();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    netifapi_netif_set_up(&LoopbackInterface->lwip_netif);
    RELEASE_SERIAL_MUTEX();

    /* Add this interface into the entities DB */
    InsertEntityInstance(CL_NL_ENTITY, &LoopbackInterface->ClNlInstance);
    InsertEntityInstance(IF_ENTITY, &LoopbackInterface->IfInstance);
    InsertEntityInstance(AT_ENTITY, &LoopbackInterface->AtInstance);

    RtlInitUnicodeString(&LoopbackInterface->DeviceName, L"Loopback");

    return STATUS_SUCCESS;
}
