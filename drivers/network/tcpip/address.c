/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: addresses abstraction
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    LIST_ENTRY ListEntry;
    TDI_ADDRESS_IP RemoteAddress;
    PIRP Irp;
    PVOID Buffer;
    ULONG BufferLength;
    PTDI_CONNECTION_INFORMATION ReturnInfo;
} RECEIVE_DATAGRAM_REQUEST;

/* The pool tags we will use for all of our allocation */
#define TAG_ADDRESS_FILE 'FrdA'

/* The list of shared addresses */
static KSPIN_LOCK AddressListLock;
static LIST_ENTRY AddressListHead;

void
TcpIpInitializeAddresses(void)
{
    KeInitializeSpinLock(&AddressListLock);
    InitializeListHead(&AddressListHead);
}

static
BOOLEAN
AddrIsUnspecified(
    _In_ PTDI_ADDRESS_IP Address)
{
    return ((Address->in_addr == 0) || (Address->in_addr == 0xFFFFFFFF));
}

static
BOOLEAN
ReceiveDatagram(
    ADDRESS_FILE* AddressFile,
    struct pbuf *p,
    ip_addr_t *addr,
    u16_t port)
{
    KIRQL OldIrql;
    LIST_ENTRY* ListEntry;
    RECEIVE_DATAGRAM_REQUEST* Request;
    ip_addr_t RequestAddr;
    BOOLEAN Result = FALSE;

    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    DPRINT1("Receiving datagram for addr 0x%08x on port %u.\n", ip4_addr_get_u32(addr), port);

    /* Block any cancellation that could occur */
    IoAcquireCancelSpinLock(&OldIrql);
    KeAcquireSpinLockAtDpcLevel(&AddressFile->RequestLock);

    ListEntry = AddressFile->RequestListHead.Flink;
    while (ListEntry != &AddressFile->RequestListHead)
    {
        Request = CONTAINING_RECORD(ListEntry, RECEIVE_DATAGRAM_REQUEST, ListEntry);
        ListEntry = ListEntry->Flink;

        ip4_addr_set_u32(&RequestAddr, Request->RemoteAddress.in_addr);

        if ((RequestAddr.addr == IPADDR_ANY) ||
                (ip_addr_cmp(&RequestAddr, addr) &&
                        ((Request->RemoteAddress.sin_port == lwip_htons(port)) || !port)))
        {
            PTA_IP_ADDRESS ReturnAddress;
            PIRP Irp;

            DPRINT1("Found a corresponding IRP.\n");

            Irp = Request->Irp;

            /* We found a request for this one */
            IoSetCancelRoutine(Irp, NULL);
            RemoveEntryList(&Request->ListEntry);
            Result = TRUE;

            KeReleaseSpinLockFromDpcLevel(&AddressFile->RequestLock);
            IoReleaseCancelSpinLock(OldIrql);

            /* In case of UDP, lwip provides a pbuf directly pointing to the data.
             * In other case, we must skip the IP header */
            Irp->IoStatus.Information = pbuf_copy_partial(
                p,
                Request->Buffer,
                Request->BufferLength,
                0);
            ReturnAddress = Request->ReturnInfo->RemoteAddress;
            ReturnAddress->Address->AddressLength = TDI_ADDRESS_LENGTH_IP;
            ReturnAddress->Address->AddressType = TDI_ADDRESS_TYPE_IP;
            ReturnAddress->Address->Address->sin_port = lwip_htons(port);
            ReturnAddress->Address->Address->in_addr = ip4_addr_get_u32(addr);
            RtlZeroMemory(ReturnAddress->Address->Address->sin_zero,
                sizeof(ReturnAddress->Address->Address->sin_zero));

            if (Request->BufferLength < p->tot_len)
                Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            else
                Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

            ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);

            /* Start again from the beginning */
            IoAcquireCancelSpinLock(&OldIrql);
            KeAcquireSpinLockAtDpcLevel(&AddressFile->RequestLock);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&AddressFile->RequestLock);
    IoReleaseCancelSpinLock(OldIrql);

    return Result;
}

static
void
lwip_udp_ReceiveDatagram_callback(
    void *arg,
    struct udp_pcb *pcb,
    struct pbuf *p,
    ip_addr_t *addr,
    u16_t port)
{
    UNREFERENCED_PARAMETER(pcb);

    ReceiveDatagram(arg, p, addr, port);
    pbuf_free(p);
}

static
u8_t
lwip_raw_ReceiveDatagram_callback(
    void *arg,
    struct raw_pcb *pcb,
    struct pbuf *p,
    ip_addr_t *addr)
{
    BOOLEAN Result;
    ADDRESS_FILE* AddressFile = arg;

    UNREFERENCED_PARAMETER(pcb);

    /* If this is for ICMP, only process the "echo received" packets.
     * The rest is processed by lwip. */
    if (AddressFile->Protocol == IPPROTO_ICMP)
    {
        /* See icmp_input */
        s16_t hlen;
        struct ip_hdr *iphdr;

        iphdr = (struct ip_hdr *)p->payload;
        hlen = IPH_HL(iphdr) * 4;

        /* Adjust the pbuf to skip the IP header */
        if (pbuf_header(p, -hlen))
            return FALSE;

        if (*((u8_t*)p->payload) != ICMP_ER)
        {
            pbuf_header(p, hlen);
            return FALSE;
        }

        pbuf_header(p, hlen);
    }

    Result = ReceiveDatagram(arg, p, addr, 0);

    if (Result)
        pbuf_free(p);

    return Result;
}

NTSTATUS
TcpIpCreateAddress(
    _Inout_ PIRP Irp,
    _Inout_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ADDRESS_FILE *AddressFile;
    LIST_ENTRY* ListEntry;
    KIRQL OldIrql;
    USHORT Port = 1;

    /* See if this port is already taken, and find a free one if needed. */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);

    ListEntry = AddressListHead.Flink;
    while (ListEntry != &AddressListHead)
    {
        AddressFile = CONTAINING_RECORD(ListEntry, ADDRESS_FILE, ListEntry);

        if (Address->sin_port)
        {
            if ((AddressFile->Protocol == Protocol) &&
                    (AddressFile->Address.sin_port == Address->sin_port))
            {
                if (IrpSp->Parameters.Create.ShareAccess)
                {
                    /* Good, we found the shared address we were looking for */
                    InterlockedIncrement(&AddressFile->RefCount);
                    KeReleaseSpinLock(&AddressListLock, OldIrql);
                    goto Success;
                }

                KeReleaseSpinLock(&AddressListLock, OldIrql);
                return STATUS_ADDRESS_ALREADY_EXISTS;
            }
        }
        else if ((AddressFile->Address.sin_port == lwip_htons(Port))
                && AddressFile->Protocol == Protocol)
        {
            Port++;
            if (Port == 0)
            {
                /* Oh no. Already 65535 ports occupied! */
                DPRINT1("No more free ports for protocol %d!\n", Protocol);
                KeReleaseSpinLock(&AddressListLock, OldIrql);
                return STATUS_TOO_MANY_ADDRESSES;
            }

            /* We must start anew to check again the previous entries in the list */
            ListEntry = &AddressListHead;
        }
        ListEntry = ListEntry->Flink;
    }

    if (!AddrIsUnspecified(Address))
    {
        /* Find the local interface for this address */
        struct netif* lwip_netif = netif_list;
        ip_addr_t IpAddr;

        ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
        while (lwip_netif)
        {
            if (ip_addr_cmp(&IpAddr, &lwip_netif->ip_addr))
            {
                break;
            }
            lwip_netif = lwip_netif->next;
        }

        if (!lwip_netif)
        {
            DPRINT1("Cound not find an interface for address 0x08x\n", AddressFile->Address.in_addr);
            KeReleaseSpinLock(&AddressListLock, OldIrql);
            return STATUS_INVALID_ADDRESS;
        }
    }

    /* Allocate our new address file */
    AddressFile = ExAllocatePoolWithTag(NonPagedPool, sizeof(*AddressFile), TAG_ADDRESS_FILE);
    if (!AddressFile)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(AddressFile, sizeof(*AddressFile));
    AddressFile->RefCount = 1;
    RtlCopyMemory(&AddressFile->Address, Address, sizeof(*Address));
    AddressFile->Protocol = Protocol;
    if (!Address->sin_port)
        AddressFile->Address.sin_port = lwip_htons(Port);

    /* Initialize the datagram request stuff */
    KeInitializeSpinLock(&AddressFile->RequestLock);
    InitializeListHead(&AddressFile->RequestListHead);

    /* Give it an entity ID and open a PCB if needed. */
    switch (Protocol)
    {
        case IPPROTO_TCP:
            InsertEntityInstance(CO_TL_ENTITY, &AddressFile->Instance);
            break;
        case IPPROTO_UDP:
        {
            ip_addr_t IpAddr;
            ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
            InsertEntityInstance(CL_TL_ENTITY, &AddressFile->Instance);
            AddressFile->lwip_udp_pcb = udp_new();
            udp_bind(AddressFile->lwip_udp_pcb, &IpAddr, lwip_ntohs(AddressFile->Address.sin_port));
            ip_set_option(AddressFile->lwip_udp_pcb, SOF_BROADCAST);
            /* Register our recv handler to lwip */
            udp_recv(
                AddressFile->lwip_udp_pcb,
                lwip_udp_ReceiveDatagram_callback,
                AddressFile);
            break;
        }
        default:
        {
            ip_addr_t IpAddr;
            ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
            if (Protocol == IPPROTO_ICMP)
                InsertEntityInstance(ER_ENTITY, &AddressFile->Instance);
            else
                InsertEntityInstance(CL_TL_ENTITY, &AddressFile->Instance);
            AddressFile->lwip_raw_pcb = raw_new(Protocol);
            raw_bind(AddressFile->lwip_raw_pcb, &IpAddr);
            ip_set_option(AddressFile->lwip_raw_pcb, SOF_BROADCAST);
            /* Register our recv handler for lwip */
            raw_recv(
                AddressFile->lwip_raw_pcb,
                lwip_raw_ReceiveDatagram_callback,
                AddressFile);
            break;
        }
    }

    /* Insert it into the list. */
    InsertTailList(&AddressListHead, &AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);

Success:
    IrpSp->FileObject->FsContext = AddressFile;
    IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpCloseAddress(
    _Inout_ ADDRESS_FILE* AddressFile
)
{
    KIRQL OldIrql;

    /* Lock the global address list */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);

    if (InterlockedDecrement(&AddressFile->RefCount) != 0)
    {
        /* There are still some open handles for this address */
        KeReleaseSpinLock(&AddressListLock, OldIrql);
        return STATUS_SUCCESS;
    }

    /* remove the lwip pcb */
    if (AddressFile->Protocol == IPPROTO_UDP)
        udp_remove(AddressFile->lwip_udp_pcb);
    else if (AddressFile->Protocol != IPPROTO_TCP)
        raw_remove(AddressFile->lwip_raw_pcb);

    /* Remove from the list and free the structure */
    RemoveEntryList(&AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);

    RemoveEntityInstance(&AddressFile->Instance);

    ExFreePoolWithTag(AddressFile, TAG_ADDRESS_FILE);


    return STATUS_SUCCESS;
}

static
NTSTATUS
ExtractAddressFromList(
    _In_ PTRANSPORT_ADDRESS AddressList,
    _Out_ PTDI_ADDRESS_IP Address)
{
    PTA_ADDRESS CurrentAddress;
    INT i;

    CurrentAddress = &AddressList->Address[0];

    /* We can only use IP addresses. Search the list until we find one */
    for (i = 0; i < AddressList->TAAddressCount; i++)
    {
        if (CurrentAddress->AddressType == TDI_ADDRESS_TYPE_IP)
        {
            if (CurrentAddress->AddressLength == TDI_ADDRESS_LENGTH_IP)
            {
                /* This is an IPv4 address */
                RtlCopyMemory(Address, &CurrentAddress->Address[0], CurrentAddress->AddressLength);
                return STATUS_SUCCESS;
            }
        }
        CurrentAddress = (PTA_ADDRESS)&CurrentAddress->Address[CurrentAddress->AddressLength];
    }
    return STATUS_INVALID_ADDRESS;
}

static
VOID
NTAPI
CancelReceiveDatagram(
    _Inout_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp)
{
    PIO_STACK_LOCATION IrpSp;
    ADDRESS_FILE* AddressFile;
    RECEIVE_DATAGRAM_REQUEST* Request;
    LIST_ENTRY* ListEntry;
    KIRQL OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AddressFile = IrpSp->FileObject->FsContext;

    /* Find this IRP in the list of requests */
    KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);
    ListEntry = AddressFile->RequestListHead.Flink;
    while (ListEntry != &AddressFile->RequestListHead)
    {
        Request = CONTAINING_RECORD(ListEntry, RECEIVE_DATAGRAM_REQUEST, ListEntry);
        if (Request->Irp == Irp)
            break;
        ListEntry = ListEntry->Flink;
    }

    /* We must have found it */
    NT_ASSERT(ListEntry != &AddressFile->RequestListHead);

    RemoveEntryList(&Request->ListEntry);
    KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);
}


NTSTATUS
TcpIpReceiveDatagram(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ADDRESS_FILE *AddressFile;
    RECEIVE_DATAGRAM_REQUEST* Request = NULL;
    PTDI_REQUEST_KERNEL_RECEIVEDG RequestInfo;
    NTSTATUS Status;
    KIRQL OldIrql;

    /* Check this is really an address file */
    if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
    {
        Status = STATUS_FILE_INVALID;
        goto Failure;
    }

    /* Get the address file */
    AddressFile = IrpSp->FileObject->FsContext;

    if (AddressFile->Protocol == IPPROTO_TCP)
    {
        /* TCP has no such thing as datagrams */
        DPRINT1("Received TDI_RECEIVE_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Failure;
    }

    /* Queue the request */
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_ADDRESS_FILE);
    if (!Request)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    RtlZeroMemory(Request, sizeof(*Request));
    Request->Irp = Irp;

    /* Get details about what we should be receiving */
    RequestInfo = (PTDI_REQUEST_KERNEL_RECEIVEDG)&IrpSp->Parameters;

    /* Get the address */
    if (RequestInfo->ReceiveDatagramInformation->RemoteAddressLength != 0)
    {
        Status = ExtractAddressFromList(
            RequestInfo->ReceiveDatagramInformation->RemoteAddress,
            &Request->RemoteAddress);
        if (!NT_SUCCESS(Status))
            goto Failure;
    }

    DPRINT1("Queuing datagram receive on address 0x%08x, port %u.\n",
        Request->RemoteAddress.in_addr, Request->RemoteAddress.sin_port);

    /* Get the buffer */
    Request->Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    Request->BufferLength = MmGetMdlByteCount(Irp->MdlAddress);

    Request->ReturnInfo = RequestInfo->ReturnDatagramInformation;

    /* Prepare for potential cancellation */
    IoAcquireCancelSpinLock(&OldIrql);
    IoSetCancelRoutine(Irp, CancelReceiveDatagram);
    IoReleaseCancelSpinLock(OldIrql);

    /* Mark pending */
    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);

    /* We're ready to go */
    ExInterlockedInsertTailList(
        &AddressFile->RequestListHead,
        &Request->ListEntry,
        &AddressFile->RequestLock);

    return STATUS_PENDING;

Failure:
    if (Request)
        ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}


NTSTATUS
TcpIpSendDatagram(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ADDRESS_FILE *AddressFile;
    PTDI_REQUEST_KERNEL_SENDDG RequestInfo;
    NTSTATUS Status;
    ip_addr_t IpAddr;
    u16_t Port;
    PVOID Buffer;
    ULONG BufferLength;
    struct pbuf* p = NULL;
    err_t lwip_error;

    /* Check this is really an address file */
    if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
    {
        Status = STATUS_FILE_INVALID;
        goto Finish;
    }

    /* Get the address file */
    AddressFile = IrpSp->FileObject->FsContext;

    if (AddressFile->Protocol == IPPROTO_TCP)
    {
        /* TCP has no such thing as datagrams */
        DPRINT1("Received TDI_SEND_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Finish;
    }

    /* Get details about what we should be receiving */
    RequestInfo = (PTDI_REQUEST_KERNEL_SENDDG)&IrpSp->Parameters;

    /* Get the address */
    if (RequestInfo->SendDatagramInformation->RemoteAddressLength != 0)
    {
        TDI_ADDRESS_IP Address;
        Status = ExtractAddressFromList(
            RequestInfo->SendDatagramInformation->RemoteAddress,
            &Address);
        if (!NT_SUCCESS(Status))
            goto Finish;
        ip4_addr_set_u32(&IpAddr, Address.in_addr);
        Port = lwip_ntohs(Address.sin_port);
    }
    else
    {
        ip_addr_set_any(&IpAddr);
        Port = 0;
    }

    DPRINT1("Sending datagram to address 0x%08x, port %u\n", ip4_addr_get_u32(&IpAddr), lwip_ntohs(Port));

    /* Get the buffer */
    Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    BufferLength = MmGetMdlByteCount(Irp->MdlAddress);
    p = pbuf_alloc(PBUF_RAW, BufferLength, PBUF_REF);
    if (!p)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    p->payload = Buffer;

    /* Send it for real */
    switch (AddressFile->Protocol)
    {
        case IPPROTO_UDP:
            if (((ip4_addr_get_u32(&IpAddr) == IPADDR_ANY) ||
                    (ip4_addr_get_u32(&IpAddr) == IPADDR_BROADCAST)) &&
                    (Port == 67) && (AddressFile->Address.in_addr == 0))
            {
                struct netif* lwip_netif = netif_list;

                /*
                 * This is a DHCP packet for an address file with address 0.0.0.0.
                 * Try to find an ethernet interface with no address set,
                 * and send the packet through it.
                 */
                while (lwip_netif != NULL)
                {
                    if (ip4_addr_get_u32(&lwip_netif->ip_addr) == 0)
                        break;
                    lwip_netif = lwip_netif->next;
                }

                if (lwip_netif == NULL)
                {
                    /* Do a regular send. (This will most likely fail) */
                    lwip_error = udp_sendto(AddressFile->lwip_udp_pcb, p, &IpAddr, Port);
                }
                else
                {
                    /* We found an interface with address being 0.0.0.0 */
                    lwip_error = udp_sendto_if(AddressFile->lwip_udp_pcb, p, &IpAddr, Port, lwip_netif);
                }
            }
            else
            {
                lwip_error = udp_sendto(AddressFile->lwip_udp_pcb, p, &IpAddr, Port);
            }
            break;
        default:
            lwip_error = raw_sendto(AddressFile->lwip_raw_pcb, p, &IpAddr);
            break;
    }

    switch (lwip_error)
    {
        case ERR_OK:
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = BufferLength;
            break;
        case ERR_MEM:
        case ERR_BUF:
            DPRINT1("Received ERR_MEM from lwip.\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        case ERR_RTE:
            DPRINT1("Received ERR_RTE from lwip.\n");
            Status = STATUS_INVALID_ADDRESS;
            break;
        case ERR_VAL:
            DPRINT1("Received ERR_VAL from lwip.\n");
            Status = STATUS_INVALID_PARAMETER;
            break;
        default:
            DPRINT1("Received error %d from lwip.\n", lwip_error);
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
    }

Finish:
    if (p)
        pbuf_free(p);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}

NTSTATUS
AddressSetIpDontFragment(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize)
{
    /* Silently ignore.
     * lwip doesn't have such thing, and already tries to fragment data as less as possible */
    return STATUS_SUCCESS;
}

NTSTATUS
AddressSetTtl(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize)
{
    ADDRESS_FILE* AddressFile;
    TCPIP_INSTANCE* Instance;
    NTSTATUS Status;
    ULONG Value;

    if (BufferSize < sizeof(ULONG))
        return STATUS_BUFFER_TOO_SMALL;

    /* Get the address file */
    Status = GetInstance(ID, &Instance);
    if (!NT_SUCCESS(Status))
        return Status;

    AddressFile = CONTAINING_RECORD(Instance, ADDRESS_FILE, Instance);

    /* Get the value */
    Value = *((ULONG*)InBuffer);

    switch (AddressFile->Protocol)
    {
        case IPPROTO_TCP:
            DPRINT1("TCP not supported yet.\n");
            break;
        case IPPROTO_UDP:
            AddressFile->lwip_udp_pcb->ttl = Value;
            break;
        default:
            AddressFile->lwip_raw_pcb->ttl = Value;
            break;
    }

    return STATUS_SUCCESS;
}
