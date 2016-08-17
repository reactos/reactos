/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: addresses abstraction
 */

#include "precomp.h"

#ifdef TCPIP_NDEBUG
#define NDEBUG
#endif
#include <debug.h>

#ifndef NDEBUG
/* Debug global variables, for convenience */
volatile long int AddrFileCount;
volatile long int GlContextCount;
volatile long int PcbCount;

PADDRESS_FILE AddrFileArray[128];
PTCP_CONTEXT ContextArray[128];
struct tcp_pcb *PCBArray[128];

KSPIN_LOCK AddrFileArrayLock;
KSPIN_LOCK ContextArrayLock;
KSPIN_LOCK PCBArrayLock;

#define _KeAcquireSpinLock(Lock,Irql) \
    if (KeGetCurrentIrql() != 0) \
    { \
        DPRINT("\n    Acquiring %p %s at IRQL %u\n", Lock, #Lock, KeGetCurrentIrql()); \
    }; \
    KeAcquireSpinLock(Lock, Irql)

#define _KeReleaseSpinLock(Lock,Irql) \
    if (KeGetCurrentIrql() != 2) \
    { \
        DPRINT("\n    Releasing %p %s at IRQL %u\n", Lock, #Lock, KeGetCurrentIrql()); \
    }; \
    KeReleaseSpinLock(Lock, Irql)

#define ADD_ADDR_FILE(AddrFile) \
    DPRINT("Adding Address File %p\n", AddrFile); \
    KeAcquireSpinLock(&AddrFileArrayLock, &OldIrql); \
    AddrFileArray[AddrFileCount] = AddrFile; \
    AddrFileCount++; \
    KeReleaseSpinLock(&AddrFileArrayLock, OldIrql)

#define ADD_ADDR_FILE_DPC(AddrFile) \
    DPRINT("Adding Address File %p\n", AddrFile); \
    KeAcquireSpinLockAtDpcLevel(&AddrFileArrayLock); \
    AddrFileArray[AddrFileCount] = AddrFile; \
    AddrFileCount++; \
    KeReleaseSpinLockFromDpcLevel(&AddrFileArrayLock)

#define REMOVE_ADDR_FILE(AddrFile) \
    DPRINT("Removing Address File %p\n", AddrFile); \
    KeAcquireSpinLock(&AddrFileArrayLock, &OldIrql); \
    for (i = 0; i < AddrFileCount; i++) \
    { \
        if (AddrFile == AddrFileArray[i]) \
        { \
            AddrFileArray[i] = NULL; \
        } \
        if (AddrFileArray[i] == NULL) \
        { \
            AddrFileArray[i] = AddrFileArray[i+1]; \
            AddrFileArray[i+1] = NULL; \
        } \
    } \
    AddrFileCount--; \
    KeReleaseSpinLock(&AddrFileArrayLock, OldIrql)

#define REMOVE_ADDR_FILE_DPC(AddrFile) \
    DPRINT("Removing Address File %p\n", AddrFile); \
    KeAcquireSpinLockAtDpcLevel(&AddrFileArrayLock); \
    for (i = 0; i < AddrFileCount; i++) \
    { \
        if (AddrFile == AddrFileArray[i]) \
        { \
            AddrFileArray[i] = NULL; \
        } \
        if (AddrFileArray[i] == NULL) \
        { \
            AddrFileArray[i] = AddrFileArray[i+1]; \
            AddrFileArray[i+1] = NULL; \
        } \
    } \
    AddrFileCount--; \
    KeReleaseSpinLockFromDpcLevel(&AddrFileArrayLock)

#define ADD_CONTEXT(Context) \
    DPRINT("Adding Context %p\n", Context); \
    KeAcquireSpinLock(&ContextArrayLock, &OldIrql); \
    ContextArray[GlContextCount] = Context; \
    GlContextCount++; \
    KeReleaseSpinLock(&ContextArrayLock, OldIrql)

#define ADD_CONTEXT_DPC(Context) \
    DPRINT("Adding Context %p\n", Context); \
    KeAcquireSpinLockAtDpcLevel(&ContextArrayLock); \
    ContextArray[GlContextCount] = Context; \
    GlContextCount++; \
    KeReleaseSpinLockFromDpcLevel(&ContextArrayLock)

#define REMOVE_CONTEXT(Context) \
    DPRINT("Removing Context %p\n", Context); \
    KeAcquireSpinLock(&ContextArrayLock, &OldIrql); \
    for (i = 0; i < GlContextCount; i++) \
    { \
        if (Context == ContextArray[i]) \
        { \
            ContextArray[i] = NULL; \
        } \
        if (ContextArray[i] == NULL) \
        { \
            ContextArray[i] = ContextArray[i+1]; \
            ContextArray[i+1] = NULL; \
        } \
    } \
    GlContextCount--; \
    KeReleaseSpinLock(&ContextArrayLock, OldIrql)

#define REMOVE_CONTEXT_DPC(Context) \
    DPRINT("Removing Context %p\n", Context); \
    KeAcquireSpinLockAtDpcLevel(&ContextArrayLock); \
    for (i = 0; i < GlContextCount; i++) \
    { \
        if (Context == ContextArray[i]) \
        { \
            ContextArray[i] = NULL; \
        } \
        if (ContextArray[i] == NULL) \
        { \
            ContextArray[i] = ContextArray[i+1]; \
            ContextArray[i+1] = NULL; \
        } \
    } \
    GlContextCount--; \
    KeReleaseSpinLockFromDpcLevel(&ContextArrayLock)

#define ADD_PCB(pcb) \
    DPRINT("Adding PCB %p\n", pcb); \
    KeAcquireSpinLock(&PCBArrayLock, &OldIrql); \
    PCBArray[PcbCount] = pcb; \
    PcbCount++; \
    KeReleaseSpinLock(&PCBArrayLock, OldIrql)

#define ADD_PCB_DPC(pcb) \
    DPRINT("Adding PCB %p\n", pcb); \
    KeAcquireSpinLockAtDpcLevel(&PCBArrayLock); \
    PCBArray[PcbCount] = pcb; \
    PcbCount++; \
    KeReleaseSpinLockFromDpcLevel(&PCBArrayLock)

#define REMOVE_PCB(pcb) \
    DPRINT("Removing PCB %p\n", pcb); \
    KeAcquireSpinLock(&PCBArrayLock, &OldIrql); \
    for (i = 0; i < PcbCount; i++) \
    { \
        if (pcb == PCBArray[i]) \
        { \
            PCBArray[i] = NULL; \
        } \
        if (PCBArray[i] == NULL) \
        { \
            PCBArray[i] = PCBArray[i+1]; \
            PCBArray[i+1] = NULL; \
        } \
    } \
    PcbCount--; \
    KeReleaseSpinLock(&PCBArrayLock, OldIrql)

#define REMOVE_PCB_DPC(pcb) \
    DPRINT("Removing PCB %p\n", pcb); \
    KeAcquireSpinLockAtDpcLevel(&PCBArrayLock); \
    for (i = 0; i < PcbCount; i++) \
    { \
        if (pcb == PCBArray[i]) \
        { \
            PCBArray[i] = NULL; \
        } \
        if (PCBArray[i] == NULL) \
        { \
            PCBArray[i] = PCBArray[i+1]; \
            PCBArray[i+1] = NULL; \
        } \
    } \
    PcbCount--; \
    KeReleaseSpinLockFromDpcLevel(&PCBArrayLock)
#endif

/* The pool tags we will use for all of our allocations */
#define TAG_ADDRESS_FILE 'FrdA'
#define TAG_DGRAM_REQST  'DgRq'
#define TAG_TCP_CONTEXT  'TCPx'
#define TAG_TCP_REQUEST  'TCPr'

typedef struct
{
    LIST_ENTRY ListEntry;
    TDI_ADDRESS_IP RemoteAddress;
    PIRP Irp;
    PVOID Buffer;
    ULONG BufferLength;
    PTDI_CONNECTION_INFORMATION ReturnInfo;
} RECEIVE_DATAGRAM_REQUEST;

/* Global Address File List */
static KSPIN_LOCK AddressListLock;
static LIST_ENTRY AddressListHead;

/* Forward-declare lwIP callback functions */
static err_t lwip_tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t lwip_tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t lwip_tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void lwip_tcp_err_callback(void *arg, err_t err);
static err_t lwip_tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);

/* Forward-declare helper functions */
static VOID CloseAddress(PADDRESS_FILE AddressFile);
static NTSTATUS CreateAddressAndEnlist(struct netif *lwip_netif, PTDI_ADDRESS_IP Address,
    IPPROTO Protocol, USHORT ShareAccess, PADDRESS_FILE *AddressFile);
static NTSTATUS DoSend(PIRP Irp, PTCP_CONTEXT Context);
static NTSTATUS EnqueueIRP(PIRP Irp, PTCP_CONTEXT Context, PDRIVER_CANCEL CancelRoutine,
    UCHAR CancelMode, UCHAR PendingMode);
static NTSTATUS ExtractAddressFromList(PTRANSPORT_ADDRESS AddressList, PTDI_ADDRESS_IP Address);

#define AddrIsUnspecified(Address) ((Address->in_addr == 0) || (Address->in_addr == 0xFFFFFFFF))

#define TCP_SET_STATE(State,Context) \
    Context->TcpState = State
#define TCP_ADD_STATE(State,Context) \
    Context->TcpState |= State
#define TCP_RMV_STATE(State,Context) \
    Context->TcpState &= ~(State)
/*
#define TCP_SET_STATE(State,Context) \
    DPRINT("Setting Context %p State to %s\n", Context, #State); \
    Context->TcpState = State
#define TCP_ADD_STATE(State,Context) \
    DPRINT("Adding State %s to Context %p\n", #State, Context); \
    Context->TcpState |= State
#define TCP_RMV_STATE(State,Context) \
    DPRINT("Removing State %s from Context %p\n", #State, Context); \
    Context->TcpState &= ~(State)
*/
/**
 * Context mutex functions
 **/

VOID
ContextMutexInit(
    PTCP_CONTEXT Context
)
{
    KeInitializeMutex(&Context->Mutex, 0);
}

VOID
ContextMutexAcquire(
    PTCP_CONTEXT Context
)
{
/*
#ifndef NDEBUG
    PKTHREAD Thread;
    Thread = KeGetCurrentThread();
    DPRINT("Thread %p acquiring lock on Context %p\n", Thread, Context);
#endif
*/
    KeWaitForMutexObject(&Context->Mutex, Executive, KernelMode, FALSE, NULL);
/*
#ifndef NDEBUG
    DPRINT("Thread %p acquired lock on Context %p\n", Thread, Context);
#endif
*/
}

VOID
ContextMutexRelease(
    PTCP_CONTEXT Context
)
{
/*
#ifndef NDEBUG
    PKTHREAD Thread;
    Thread = KeGetCurrentThread();
    DPRINT("Thread %p releasing lock on Context %p\n", Thread, Context);
#endif
*/
    KeReleaseMutex(&Context->Mutex, FALSE);
/*
#ifndef NDEBUG
    DPRINT("Thread %p released lock on Context %p\n", Thread, Context);
#endif
*/
}

/**
 * TCP Cancel Routine
 **/

VOID
NTAPI
CancelRequestRoutine(
    _Inout_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    LARGE_INTEGER Timeout;
    
    PADDRESS_FILE AddressFile;
    PIO_STACK_LOCATION IrpSp;
    PIRP PendingIrp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_CONTEXT CurrentContext;
    PTCP_CONTEXT ListenContext;
    PTCP_REQUEST ListWalkRequest;
    PTCP_REQUEST Request;
    
    /* This function is always called with the Cancel lock held, Irp->Cancel set to TRUE, and
     * Irp->CancelRoutine set to NULL. */
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    
    /* This routine currently only handles TCP requests */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT("Invalid file type %08x for TCP cancel\n", IrpSp->FileObject->FsContext2);
        goto FINISH;
    }
    
    Context = IrpSp->FileObject->FsContext;
    ContextMutexAcquire(Context);
    
    /* Walk the Context's Request list to find the TCP_REQUEST containing this IRP */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        if (Request->Payload.PendingIrp == Irp)
        {
            RemoveEntryList(&Request->ListEntry);
            CurrentContext = Context;
            goto FOUND_REQUEST;
        }
        Entry = Entry->Flink;
    }
    
    /* If we could not find a matching IRP, check in the Listener's Request list if it exists. */
    if ((Context->AddressFile != NULL)
        && (Context->AddressFile->HasListener == TRUE)
        && (Context->AddressFile->Listener != Context))
    {
        ContextMutexRelease(Context);
        ListenContext = Context->AddressFile->Listener;
        ContextMutexAcquire(ListenContext);
        Head = &ListenContext->RequestListHead;
        Entry = Head->Flink;
        while (Entry != Head)
        {
            Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
            if (Request->Payload.PendingIrp == Irp)
            {
                RemoveEntryList(&Request->ListEntry);
                CurrentContext = ListenContext;
                goto FOUND_REQUEST;
            }
            Entry = Entry->Flink;
        }
        ContextMutexRelease(ListenContext);
    }
    else
    {
        ContextMutexRelease(Context);
    }
    
    /* We should never go through both lists without finding a match */
    DPRINT1("Cancelling unknown IRP %p for Context %p\n", Irp, Context);
    goto FINISH;
    
FOUND_REQUEST:
    /* Handle PCB deallocation */
    switch (Request->CancelMode)
    {
        case TCP_REQUEST_CANCEL_MODE_ABORT :
            Timeout.QuadPart = -1;
            ACQUIRE_SERIAL_MUTEX(&CurrentContext->Mutex, &Timeout);
            if (CurrentContext->lwip_tcp_pcb != NULL)
            {
                tcp_arg(CurrentContext->lwip_tcp_pcb, NULL);
                tcp_abort(CurrentContext->lwip_tcp_pcb);
#ifndef NDEBUG
                REMOVE_PCB(CurrentContext->lwip_tcp_pcb);
#endif
                CurrentContext->lwip_tcp_pcb = NULL;
            }
            RELEASE_SERIAL_MUTEX();
            break;
        case TCP_REQUEST_CANCEL_MODE_CLOSE :
            Timeout.QuadPart = -1;
            ACQUIRE_SERIAL_MUTEX(&CurrentContext->Mutex, &Timeout);
            if (CurrentContext->lwip_tcp_pcb != NULL)
            {
                tcp_arg(CurrentContext->lwip_tcp_pcb, NULL);
                tcp_close(CurrentContext->lwip_tcp_pcb);
#ifndef NDEBUG
                REMOVE_PCB(CurrentContext->lwip_tcp_pcb);
#endif
                CurrentContext->lwip_tcp_pcb = NULL;
            }
            RELEASE_SERIAL_MUTEX();
            break;
        case TCP_REQUEST_CANCEL_MODE_SHUTDOWN :
            Timeout.QuadPart = -1;
            ACQUIRE_SERIAL_MUTEX(&CurrentContext->Mutex, &Timeout);
            if (CurrentContext->lwip_tcp_pcb != NULL)
            {
                tcp_arg(CurrentContext->lwip_tcp_pcb, NULL);
                tcp_shutdown(CurrentContext->lwip_tcp_pcb, 1, 1);
#ifndef NDEBUG
                REMOVE_PCB(CurrentContext->lwip_tcp_pcb);
#endif
                CurrentContext->lwip_tcp_pcb = NULL;
            }
            RELEASE_SERIAL_MUTEX();
            break;
        case TCP_REQUEST_CANCEL_MODE_PRESERVE :
            break;
        default :
            DPRINT1("Invalid cancel mode %08x for Request on Context %p\n",
                Request->CancelMode, CurrentContext);
            break;
    }
    
    /* Handle TCP State bitmap modification and Listener deallocation */
    switch (Request->PendingMode)
    {
        case TCP_REQUEST_PENDING_SEND :
            Head = &CurrentContext->RequestListHead;
            Entry = Head->Flink;
            while (Entry != Head)
            {
                ListWalkRequest = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
                if (ListWalkRequest->PendingMode == TCP_REQUEST_PENDING_SEND)
                {
                    goto PRESERVE_STATE_SEND;
                }
                Entry = Entry->Flink;
            }
            TCP_RMV_STATE(TCP_STATE_SENDING, CurrentContext);
PRESERVE_STATE_SEND:
            ContextMutexRelease(CurrentContext);
            break;
        case TCP_REQUEST_PENDING_RECEIVE :
            Head = &CurrentContext->RequestListHead;
            Entry = Head->Flink;
            while (Entry != Head)
            {
                ListWalkRequest = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
                if (ListWalkRequest->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
                {
                    goto PRESERVE_STATE_RECEIVE;
                }
                Entry = Entry->Flink;
            }
            TCP_RMV_STATE(TCP_STATE_RECEIVING, CurrentContext);
PRESERVE_STATE_RECEIVE:
            ContextMutexRelease(CurrentContext);
            break;
        case TCP_REQUEST_PENDING_CONNECT :
            TCP_SET_STATE(TCP_STATE_CLOSED, CurrentContext);
            ContextMutexRelease(CurrentContext);
            break;
        case TCP_REQUEST_PENDING_LISTEN :
            /* When cancelling a Listen Request, we need to deallocate the Listen Context that was
             * allocated on the first Listen IRP on a particular address. */
            Timeout.QuadPart = -1;
            /* Acquire the MTSerialMutex to block pending accepted connections from being processed
             * and dying, leaving dead pointers in the Request list. */
            ACQUIRE_SERIAL_MUTEX(&CurrentContext->Mutex, &Timeout);
            /* Walk the Request list to complete IRPs and kill pending accepted connections */
            Head = &CurrentContext->RequestListHead;
            Entry = Entry->Flink;
            while (Entry != Head)
            {
                ListWalkRequest = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
                Entry = Entry->Flink;
                switch (ListWalkRequest->PayloadType)
                {
                    case TCP_REQUEST_PAYLOAD_IRP :
                        PendingIrp = ListWalkRequest->Payload.PendingIrp;
                        PendingIrp->Cancel = TRUE;
                        IoSetCancelRoutine(PendingIrp, NULL);
                        PendingIrp->IoStatus.Status = STATUS_CANCELLED;
                        PendingIrp->IoStatus.Information = 0;
                        IoCompleteRequest(PendingIrp, IO_NETWORK_INCREMENT);
                    case TCP_REQUEST_PAYLOAD_PCB :
                        tcp_arg(ListWalkRequest->Payload.apcb, NULL);
                        tcp_close(ListWalkRequest->Payload.apcb);
#ifndef NDEBUG
                        REMOVE_PCB(ListWalkRequest->Payload.apcb);
#endif
                    default :
                        DPRINT1("Invalid payload type %08x on Context %p\n",
                            ListWalkRequest->PayloadType, CurrentContext);
                        break;
                }
                ExFreePoolWithTag(ListWalkRequest, TAG_TCP_REQUEST);
            }
            RELEASE_SERIAL_MUTEX();
            /* Disassociate this Listen context from its Address File, and mark the Address File as
             * no longer having a Listener. */
            AddressFile = CurrentContext->AddressFile;
            if (AddressFile != NULL)
            {
                AddressFile->HasListener = FALSE;
                AddressFile->Listener = NULL;
                if ((InterlockedDecrement(&AddressFile->ContextCount) < 1)
                    && (AddressFile->RefCount < 1))
                {
                    CloseAddress(AddressFile);
                }
            }
            else
            {
                DPRINT1("Listener %p should have an associated address\n", CurrentContext);
            }
            ContextMutexRelease(CurrentContext);
            ExFreePoolWithTag(CurrentContext, TAG_TCP_CONTEXT);
#ifndef NDEBUG
            REMOVE_CONTEXT(CurrentContext);
#endif
            break;
        default :
            ContextMutexRelease(CurrentContext);
            DPRINT1("Invalid pending mode %08x for cancel on Context %p\n",
                Request->PendingMode, CurrentContext);
            break;
    }
    ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
    
FINISH:
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}

/**
 * Creation and initialization handlers
 **/

VOID
TcpIpInitializeAddresses(void)
{
    KeInitializeSpinLock(&AddressListLock);
    InitializeListHead(&AddressListHead);

#ifndef NDEBUG
    AddrFileCount = 0;
    GlContextCount = 0;
    PcbCount = 0;

    KeInitializeSpinLock(&AddrFileArrayLock);
    KeInitializeSpinLock(&ContextArrayLock);
    KeInitializeSpinLock(&PCBArrayLock);
#endif
}

NTSTATUS
TcpIpCreateAddress(
    _Inout_ PIRP Irp,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PADDRESS_FILE AddressFile;
    PIO_STACK_LOCATION IrpSp;

    struct netif *lwip_netif;
    ip_addr_t IpAddr;

    /* For a specified address, find a matching netif and, if needed, a free port. For unspecified
     * addresses, the lwip_netif is set to NULL. */
    if (!AddrIsUnspecified(Address))
    {
        ip4_addr_set_u32(&IpAddr, Address->in_addr);
        for (lwip_netif = netif_list; lwip_netif != NULL; lwip_netif = lwip_netif->next)
        {
            if (ip_addr_cmp(&IpAddr, &lwip_netif->ip_addr))
            {
                /* Found the local interface */
                break;
            }
        }
        /* List walk did not find a match, so there is no match */
        if (lwip_netif == NULL) {
            DPRINT("Cound not find an interface for address 0x%08x\n", Address->in_addr);
            Status = STATUS_INVALID_ADDRESS;
            goto FAIL;
        }

        /* If port is unspecified, grab a free port from lwIP */
        if (Address->sin_port == 0)
        {
            switch (Protocol)
            {
                case IPPROTO_TCP :
                    ACQUIRE_SERIAL_MUTEX_NO_TO();
                    Address->sin_port = (USHORT)tcp_new_port();
                    RELEASE_SERIAL_MUTEX();
                    break;
                case IPPROTO_UDP :
                    Address->sin_port = (USHORT)udp_new_port();
                    break;
                case IPPROTO_RAW :
                case IPPROTO_ICMP :
                    /* Raw and ICMP do not use ports */
                    goto NOPORT;
                default :
                    DPRINT("Unsupported protocol: %d\n", Protocol);
                    Status = STATUS_INVALID_ADDRESS;
                    goto FAIL;
            }
        }

        /* There are no more free ports. Protocols that don't use ports skip this check */
        if (Address->sin_port == 0) {
            DPRINT("No more free ports for protocol %d\n", Protocol);
            Status = STATUS_TOO_MANY_ADDRESSES;
            goto FAIL;
        }
    }
    else
    {
        lwip_netif = NULL;
    }

NOPORT:
    /* Create Address File and add to all relevant lists */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    KeAcquireSpinLock(&AddressListLock, &OldIrql);
    Status = CreateAddressAndEnlist(
        lwip_netif,
        Address,
        Protocol,
        IrpSp->Parameters.Create.ShareAccess,
        &AddressFile);
    KeReleaseSpinLock(&AddressListLock, OldIrql);
    if (Status != STATUS_SUCCESS)
    {
FAIL:
        IrpSp->FileObject->FsContext = NULL;
        IrpSp->FileObject->FsContext2 = NULL;
    }
    else
    {
        IrpSp->FileObject->FsContext = AddressFile;
        IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
    }

    return Status;
}

NTSTATUS
TcpIpCreateContext(
    _Inout_ PIRP Irp,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
)
{
#ifndef NDEBUG
    KIRQL OldIrql;
#endif
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;

    /* Do not support anything other than TCP right now */
    if (Protocol != IPPROTO_TCP)
    {
        DPRINT1("Creating connection context for non-TCP protocol: %d\n", Protocol);
        return STATUS_INVALID_PARAMETER;
    }

    /* Create context */
    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Context), TAG_TCP_CONTEXT);
    if (Context == NULL)
    {
        DPRINT1("Not enough resources\n");
        return STATUS_NO_MEMORY;
    }
#ifndef NDEBUG
    ADD_CONTEXT(Context);
#endif

    /* Set initial values */
    TCP_SET_STATE(TCP_STATE_CREATED, Context);
    Context->AddressFile = NULL;
    InitializeListHead(&Context->RequestListHead);
    Context->ReferencedByUpperLayer = TRUE;
    ContextMutexInit(Context);

    /* We defer PCB creation until TcpIpAssociateAddress(), since this Context could be a dummy
     * connection endpoint used to poll/wait for new accepted connections */
    Context->lwip_tcp_pcb = NULL;

    /* Deliver Context to caller through the IRP */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp->FileObject->FsContext = Context;
    IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;

    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpAssociateAddress(
    _Inout_ PIRP Irp
)
{
    KIRQL OldIrql;
    NTSTATUS Status;

    PADDRESS_FILE AddressFile;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    PTDI_REQUEST_KERNEL_ASSOCIATE RequestInfo;

    err_t lwip_err;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Get the Address File we are associating to */
    RequestInfo = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;
    Status = ObReferenceObjectByHandle(
        RequestInfo->AddressHandle,
        0,
        *IoFileObjectType,
        KernelMode,
        (PVOID*)&FileObject,
        NULL);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("Failed to dereference FileObject: 0x%08x\n", Status);
        return Status;
    }
    if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
    {
        DPRINT1("File object should be an Address File\n");
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }
    AddressFile = FileObject->FsContext;

    /* Get the TCP Context we are associating */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("File object should be a TCP Context\n");
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;

    /* Sanity checks */
    if ((AddressFile->Protocol != IPPROTO_TCP) || (Context->TcpState != TCP_STATE_CREATED))
    {
        DPRINT1("Context %p has wrong state or Addr File %p has wrong protocol, for association\n",
            Context, AddressFile);
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* If there is already a listener listening on the address, then this is a dummy connection
     * endpoint used to poll/wait for new accepted connections. In this case, we skip creating and
     * binding a new PCB. */
    if (AddressFile->HasListener == TRUE)
    {
        goto NO_PCB;
    }

    /* Create a new lwIP PCB and initialize callback data */
    Context->lwip_tcp_pcb = tcp_new();
#ifndef NDEBUG
    ADD_PCB(Context->lwip_tcp_pcb);
#endif
    tcp_arg(Context->lwip_tcp_pcb, Context);
    tcp_err(Context->lwip_tcp_pcb, lwip_tcp_err_callback);

    /* Attempt to bind the PCB. lwIP internally handles INADDR_ANY. */
    ACQUIRE_SERIAL_MUTEX_NO_TO();
    lwip_err = tcp_bind(
        Context->lwip_tcp_pcb,
        (ip_addr_t *)&AddressFile->Address.in_addr,
        AddressFile->Address.sin_port);
    switch (lwip_err)
    {
        case (ERR_OK) :
        {
            break;
        }
        case (ERR_BUF) :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP ERR_BUFF. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            Status = STATUS_NO_MEMORY;
            goto FINISH;
        }
        case (ERR_VAL) :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP ERR_VAL. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            Status = STATUS_INVALID_PARAMETER;
            goto FINISH;
        }
        case (ERR_USE) :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP ERR_USE. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            Status = STATUS_ADDRESS_ALREADY_EXISTS;
            goto FINISH;
        }
        default :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP unexpected error. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            // TODO: better return code
            Status = STATUS_UNSUCCESSFUL;
            goto FINISH;
        }
    }
    ip_set_option(Context->lwip_tcp_pcb, SOF_BROADCAST);
    RELEASE_SERIAL_MUTEX();

NO_PCB:
    /* Failure would jump us beyond here, so being here means we succeeded in binding */
    TCP_SET_STATE(TCP_STATE_BOUND, Context);
    Context->AddressFile = AddressFile;
    InterlockedIncrement(&AddressFile->ContextCount);

    Status = STATUS_SUCCESS;

FINISH:
    return Status;
}

/**
 * TCP server-side handlers
 **/

/* This handler never acquires the mutex for the incoming Context because the IRP sender should be
 * the only reference holder for that Context. */
NTSTATUS
TcpIpListen(
    _Inout_ PIRP Irp
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    
    PADDRESS_FILE AddressFile;
    PIO_STACK_LOCATION IrpSp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_CONTEXT ListenContext;
    PTCP_REQUEST Request;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity check the incoming TCP Context */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Not a connection context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (Context->TcpState != TCP_STATE_BOUND)
    {
        DPRINT1("Context %p is not a bound context\n", Context);
        return STATUS_INVALID_PARAMETER;
    }
    
    /* If there is already a listener on the address, this context is a dummy endpoint used to poll
     * and wait for new accepted connections. */
    AddressFile = Context->AddressFile;
    if (AddressFile->HasListener == TRUE)
    {
        ListenContext = AddressFile->Listener;
        ContextMutexAcquire(ListenContext);
        
        Head = &ListenContext->RequestListHead;
        Entry = Head->Flink;
        while (Entry != Head)
        {
            Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
            if (Request->PendingMode == TCP_REQUEST_PENDING_ACCEPTED_CONNECTION)
            {
                /* If there is an established PCB available, immediately take ownership of it from
                 * the Listening Context so we can release the mutex on the Listening Context. */
                RemoveEntryList(&Request->ListEntry);
                Timeout.QuadPart = -1;
                
                /* This macro releases &ListenContext->Mutex on timeout to prevent deadlocks. 
                 * We need to make sure the ListenContext was not disassociated during wait. */
                ACQUIRE_SERIAL_MUTEX(&ListenContext->Mutex, &Timeout);
                if (ListenContext->AddressFile == NULL)
                {
                    RELEASE_SERIAL_MUTEX();
                    ContextMutexRelease(ListenContext);
                    return STATUS_ADDRESS_CLOSED;
                }
                
                /* Inform lwIP we accepted the connection */
                tcp_accepted(ListenContext->lwip_tcp_pcb);
                
                RELEASE_SERIAL_MUTEX();
                ContextMutexRelease(ListenContext);
                
                /* Set Context fields */
                Context->lwip_tcp_pcb = Request->Payload.apcb;
                TCP_SET_STATE(TCP_STATE_CONNECTED, Context);
                
                ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
                
                return STATUS_SUCCESS;
            }
            Entry = Entry->Flink;
        }
        
        /* We walked the entire Request list without finding a connection waiting to be accepted. 
         * Now we must enquque a Request and wait for an incomming connection request. */
        DPRINT("Found no connection for Context %p on Listener %p with PCB %p\n",
            Context, ListenContext, ListenContext->lwip_tcp_pcb);
        
        Status = EnqueueIRP(
            Irp,
            ListenContext,
            CancelRequestRoutine,
            TCP_REQUEST_CANCEL_MODE_CLOSE,
            TCP_REQUEST_PENDING_LISTEN);
            
        ContextMutexRelease(ListenContext);
        
        return Status;
    }
    else
    {
        /* If there is not already a Listener on the address, create one and initiate a Listen in
         * lwIP. */
        ListenContext = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(*ListenContext),
            TAG_TCP_CONTEXT);
        if (ListenContext == NULL)
        {
            DPRINT1("Not enough resources\n");
            return STATUS_NO_MEMORY;
        }
#ifndef NDEBUG
        ADD_CONTEXT(ListenContext);
#endif
        
        InterlockedIncrement(&AddressFile->ContextCount);
        
        /* Initialize the Listener */
        TCP_SET_STATE(TCP_STATE_LISTENING, ListenContext);
        ListenContext->AddressFile = AddressFile;
        InitializeListHead(&ListenContext->RequestListHead);
        ListenContext->ReferencedByUpperLayer = FALSE;
        ContextMutexInit(ListenContext);
        
        /* Initiate lwIP listen */
        ACQUIRE_SERIAL_MUTEX_NO_TO();
        ListenContext->lwip_tcp_pcb = tcp_listen(Context->lwip_tcp_pcb);
#ifndef NDEBUG
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        if (ListenContext->lwip_tcp_pcb == NULL)
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("Listen failed on Context %p\n", Context);
            return STATUS_INVALID_ADDRESS;
        }
#ifndef NDEBUG
        ADD_PCB(ListenContext->lwip_tcp_pcb);
#endif
        
        /* Set lwIP callback information */
        tcp_arg(ListenContext->lwip_tcp_pcb, ListenContext);
        tcp_accept(ListenContext->lwip_tcp_pcb, lwip_tcp_accept_callback);
        
        Context->lwip_tcp_pcb = NULL;
        
        /* Mark Address File as having a Listener */
        AddressFile->HasListener = TRUE;
        AddressFile->Listener = ListenContext;
        
        Status = EnqueueIRP(
            Irp,
            ListenContext,
            CancelRequestRoutine,
            TCP_REQUEST_CANCEL_MODE_CLOSE,
            TCP_REQUEST_PENDING_LISTEN);
        
        RELEASE_SERIAL_MUTEX();
        
        return Status;
    }
}

/**
 * TCP client-side handlers
 **/

/* This handler never acquires the mutex for the incoming Context because the IRP sender should be
 * the only reference holder for that Context */
NTSTATUS
TcpIpConnect(
    _Inout_ PIRP Irp
)
{
    NTSTATUS Status;
    
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    PTDI_REQUEST_KERNEL_CONNECT RequestInfo;
    PTRANSPORT_ADDRESS RemoteTransportAddress;
    
    struct sockaddr *SocketAddressRemote;
    struct sockaddr_in * SocketAddressInRemote;
    
    err_t lwip_err;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity check the given Context */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("File object is not a connection context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (Context->TcpState != TCP_STATE_BOUND)
    {
        DPRINT1("Connecting from unbound socket context %p\n", Context);
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Extract remote address to connect to */
    RequestInfo = (PTDI_REQUEST_KERNEL_CONNECT)&IrpSp->Parameters;
    RemoteTransportAddress = RequestInfo->RequestConnectionInformation->RemoteAddress;
    SocketAddressRemote = (struct sockaddr *)&RemoteTransportAddress->Address[0];
    SocketAddressInRemote = (struct sockaddr_in *)&SocketAddressRemote->sa_data;
    
    /* Call into lwIP to initiate Connect */
    ACQUIRE_SERIAL_MUTEX_NO_TO();
    lwip_err = tcp_connect(Context->lwip_tcp_pcb,
        (ip_addr_t*)&SocketAddressInRemote->sin_addr.s_addr,
        SocketAddressInRemote->sin_port,
        lwip_tcp_connected_callback);
    switch (lwip_err)
    {
        case ERR_OK :
            /* If successful, enqueue the IRP, set the TCP State variable, and return */
            Status = EnqueueIRP(
                Irp,
                Context,
                CancelRequestRoutine,
                TCP_REQUEST_CANCEL_MODE_ABORT,
                TCP_REQUEST_PENDING_CONNECT);
            TCP_SET_STATE(TCP_STATE_CONNECTING, Context);
            RELEASE_SERIAL_MUTEX();
            return Status;
        case ERR_VAL :
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_VAL. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_INVALID_PARAMETER;
        case ERR_ISCONN :
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_ISCONN. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_CONNECTION_ACTIVE;
        case ERR_RTE :
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_RTE. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_NETWORK_UNREACHABLE;
        case ERR_BUF :
            /* Use correct error once NDIS errors are included.
             * This return value means local port unavailable. */
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_BUF. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_ADDRESS_ALREADY_EXISTS;
        case ERR_USE :
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_USE. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_CONNECTION_ACTIVE;
        case ERR_MEM :
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip ERR_MEM. Context %p, PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_NO_MEMORY;
        default :
            /* unknown return value */
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwip unknown return code. Context %p, PCB %p\n",
                Context, Context->lwip_tcp_pcb);
            return STATUS_NOT_IMPLEMENTED;
    }
}

/**
 * TCP established connection operations handlers
 **/

NTSTATUS
TcpIpSend(
    _Inout_ PIRP Irp
)
{
    NTSTATUS Status;
    
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity check the incoming Context */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Received TDI_SEND for something that is not a TCP Context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    ACQUIRE_SERIAL_MUTEX_NO_TO();
    ContextMutexAcquire(Context);
    if (!(Context->TcpState & TCP_STATE_CONNECTED))
    {
        ContextMutexRelease(Context);
        RELEASE_SERIAL_MUTEX();
        DPRINT1("Attempting to send on Context at %p without an established connection\n", Context);
        return STATUS_ONLY_IF_CONNECTED;
    }
    
    /* If the Context is already servicing a Send, queue this one to be handled when that is done */
    if (Context->TcpState & TCP_STATE_SENDING)
    {
        goto WAIT_TO_SEND;
    }
    
    Status = DoSend(Irp, Context);
    RELEASE_SERIAL_MUTEX();
    if (Status == STATUS_SUCCESS)
    {
        TCP_ADD_STATE(TCP_STATE_SENDING, Context);
    }
    else
    {
        goto FINISH;
    }
    
WAIT_TO_SEND:
    Status = EnqueueIRP(
        Irp,
        Context,
        CancelRequestRoutine,
        TCP_REQUEST_CANCEL_MODE_PRESERVE,
        TCP_REQUEST_PENDING_SEND);
FINISH:
    ContextMutexRelease(Context);
    return Status;
}

NTSTATUS
TcpIpReceive(
    _Inout_ PIRP Irp
)
{
    NTSTATUS Status;
    
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity check incoming Context */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Receiving not on a connection endpoint\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    ContextMutexAcquire(Context);
    if (!(Context->TcpState & TCP_STATE_CONNECTED))
    {
        DPRINT1("Receiving on TCP Context %p in state %08x with PCB %p\n",
            Context, Context->TcpState, Context->lwip_tcp_pcb);
        ContextMutexRelease(Context);
        return STATUS_ADDRESS_CLOSED;
    }
    
    /* Do not handle pending received data like we do pending accepted connections. lwIP handles
     * refused data for us. Simply enqueue the Request. */
    Status = EnqueueIRP(
        Irp,
        Context,
        CancelRequestRoutine,
        TCP_REQUEST_CANCEL_MODE_SHUTDOWN,
        TCP_REQUEST_PENDING_RECEIVE);
    TCP_ADD_STATE(TCP_STATE_RECEIVING, Context);
    ContextMutexRelease(Context);
    return Status;
}

/**
 * TCP shutdown handlers
 **/

NTSTATUS
TcpIpDisassociateAddress(
    _Inout_ PIRP Irp
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    LARGE_INTEGER Timeout;
    
    PADDRESS_FILE AddressFile;
    PIO_STACK_LOCATION IrpSp;
    PIRP PendingIrp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Obligatory sanity check */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Disassociating invalid object\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    Context = IrpSp->FileObject->FsContext;
    ContextMutexAcquire(Context);
    
    /* Check if the address association is already broken */
    AddressFile = Context->AddressFile;
    if (AddressFile == NULL)
    {
        goto BREAK_LWIP_ASSOCIATION;
    }
    Context->AddressFile = NULL;
    
    /* If the address association remains, break it. */
    if ((InterlockedDecrement(&AddressFile->ContextCount) < 1) && (AddressFile->RefCount < 1))
    {
        CloseAddress(AddressFile);
    }
    
BREAK_LWIP_ASSOCIATION:
    /* If the lwIP association remains, break it. */
    Timeout.QuadPart = -1;
    ACQUIRE_SERIAL_MUTEX(&Context->Mutex, &Timeout);
    if (Context->lwip_tcp_pcb != NULL)
    {
        tcp_arg(Context->lwip_tcp_pcb, NULL);
        tcp_close(Context->lwip_tcp_pcb);
#ifndef NDEBUG
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        Context->lwip_tcp_pcb = NULL;
    }
    RELEASE_SERIAL_MUTEX();
    
    /* Clean up remaining pending requests */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        Entry = Entry->Flink;
        
        /* Immediately block cancellations. We do not have to worry about other payload types, since
         * they only appear on Listening Contexts, which we create and destroy ourselves without
         * IRPs to tell us when to do so. */
        PendingIrp = Request->Payload.PendingIrp;
        PendingIrp->Cancel = TRUE;
        IoSetCancelRoutine(PendingIrp, NULL);
        /* Set completion information and complete */
        PendingIrp->IoStatus.Status = STATUS_CANCELLED;
        PendingIrp->IoStatus.Information = 0;
        IoCompleteRequest(PendingIrp, IO_NETWORK_INCREMENT);

        ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
    }
    
    /* Pending accepted connections have been dealt with, we can release the MTSerialMutex. */
    ContextMutexRelease(Context);
    
    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpDisconnect(
    _Inout_ PIRP Irp
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    LARGE_INTEGER Timeout;
    
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity check incoming Context */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Not disconnecting a valid connection\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    Context = IrpSp->FileObject->FsContext;
    ContextMutexAcquire(Context);
    
    Timeout.QuadPart = -1;
    ACQUIRE_SERIAL_MUTEX(&Context->Mutex, &Timeout);
    
    /* Shut down the connection, if it is still alive. */
    if (Context->lwip_tcp_pcb != NULL)
    {
        tcp_arg(Context->lwip_tcp_pcb, NULL);
        tcp_close(Context->lwip_tcp_pcb);
#ifndef NDEBUG
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        Context->lwip_tcp_pcb = NULL;
    }

    RELEASE_SERIAL_MUTEX();
    
    TCP_SET_STATE(TCP_STATE_CLOSED, Context);
    
    ContextMutexRelease(Context);
    
    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpCloseAddress(
    _In_ PADDRESS_FILE AddressFile
)
{
    /* Check if there are still references to this Address File */
    if (InterlockedDecrement(&AddressFile->RefCount) > 0)
    {
        DPRINT("Closing address with %d open handles\n", AddressFile->RefCount);
        return STATUS_SUCCESS;
    }
    if (AddressFile->ContextCount > 0)
    {
        /* This should not happen if the upper level drivers work correctly */
        DPRINT("Closing address with %d context references\n", AddressFile->ContextCount);
        // TODO: better return code
        return STATUS_UNSUCCESSFUL;
    }

    CloseAddress(AddressFile);

    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpCloseContext(
    _In_ PTCP_CONTEXT Context
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    
    ContextMutexAcquire(Context);
    
    if ((Context->AddressFile != NULL) || (Context->lwip_tcp_pcb != NULL)) {
        DPRINT1("Context %p retains association. AddressFile %p, tcp_pcb %p\n",
            Context,
            Context->AddressFile,
            Context->lwip_tcp_pcb);
        return STATUS_CONNECTION_ACTIVE;
    }
    
    ContextMutexRelease(Context);
    
    ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);
#ifndef NDEBUG
    REMOVE_CONTEXT(Context);
#endif
    
    return STATUS_SUCCESS;
}

/**
 * UDP Fuctions
 **/

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

    ExFreePoolWithTag(Request, TAG_DGRAM_REQST);
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
        DPRINT("Received TDI_RECEIVE_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Failure;
    }

    /* Queue the request */
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_DGRAM_REQST);
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

    DPRINT("Queuing datagram receive on address 0x%08x, port %u.\n",
        Request->RemoteAddress.in_addr, Request->RemoteAddress.sin_port);

    /* Get the buffer */
    Request->Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    Request->BufferLength = MmGetMdlByteCount(Irp->MdlAddress);

    Request->ReturnInfo = RequestInfo->ReturnDatagramInformation;

    /* Mark pending */
    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, CancelReceiveDatagram);

    /* We're ready to go */
    ExInterlockedInsertTailList(
        &AddressFile->RequestListHead,
        &Request->ListEntry,
        &AddressFile->RequestLock);

    return STATUS_PENDING;

Failure:
    if (Request)
        ExFreePoolWithTag(Request, TAG_DGRAM_REQST);
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
        DPRINT("Received TDI_SEND_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Finish;
    }

    /* Get details about what we should be sending */
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

    DPRINT("Sending datagram to address 0x%08x, port %u\n", ip4_addr_get_u32(&IpAddr), lwip_ntohs(Port));

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

/**
 * TCP/IP Stack Functions
 **/

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
            DPRINT("TCP not supported yet.\n");
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

/**
 * Helper Functions
 **/

/* Check AddressFile->ContextCount and AddressFile->RefCount before calling */
static
VOID
CloseAddress(
    PADDRESS_FILE AddressFile
)
{
#ifndef NDEBUG
    INT i;
#endif
    KIRQL OldIrql;
    
    /* Remove the Address File from global lists */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);
    RemoveEntryList(&AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);
    RemoveEntityInstance(&AddressFile->Instance);
    
    /* ICMP, RAW, and UDP addresses store a local PCB */
    switch (AddressFile->Protocol)
    {
        case IPPROTO_ICMP :
        case IPPROTO_RAW :
            //raw_remove(AddressFile->lwip_raw_pcb);
            break;
        case IPPROTO_UDP :
            //udp_remove(AddressFile->lwip_udp_pcb);
            break;
        case IPPROTO_TCP :
            /* Nothing to deallocate for TCP */
            goto NO_REQUESTS;
        default :
            /* We should never reach here */
            DPRINT1("Closing Address File with unknown protocol, %d. This should never happen.\n",
                AddressFile->Protocol);
            break;
    }
    
    /* Finish pending requests for RAW and UDP addresses */

NO_REQUESTS:
    /* Deallocate the Address File */
    ExFreePoolWithTag(AddressFile, TAG_ADDRESS_FILE);
#ifndef NDEBUG
    REMOVE_ADDR_FILE(AddressFile);
#endif
}

static
NTSTATUS
CreateAddressAndEnlist(
    struct netif *lwip_netif,
    PTDI_ADDRESS_IP Address,
    IPPROTO Protocol,
    USHORT ShareAccess,
    PADDRESS_FILE *_AddressFile
)
{
    PADDRESS_FILE AddressFile;
    PLIST_ENTRY Entry;

    /* If a netif is specified, check for AddressFile with matching netif and port in order to
     * detect duplicates */
    if (lwip_netif != NULL)
    {
        Entry = AddressListHead.Flink;
        while (Entry != &AddressListHead)
        {
            AddressFile = CONTAINING_RECORD(Entry, ADDRESS_FILE, ListEntry);
            /* If the requested address' lwIP netif, protocol, and port match that of an existing
             * Address File, then check if we are creating a shared address. */
            if ((AddressFile->NetInterface == lwip_netif) &&
                (AddressFile->Protocol == Protocol) &&
                (AddressFile->Address.sin_port == Address->sin_port))
            {
                /* Matching address file found. */
                if (ShareAccess)
                {
                    /* If shared, increment reference count */
                    InterlockedIncrement(&AddressFile->RefCount);
                    goto FINISH;
                }
                else
                {
                    /* If not shared, return error */
                    return STATUS_ADDRESS_ALREADY_EXISTS;
                }
            }
        }
    }

    /* If no match was found in the Address File list or the netif is not specified,
     * then create a new one */
    AddressFile = ExAllocatePoolWithTag(NonPagedPool, sizeof(*AddressFile), TAG_ADDRESS_FILE);
    if (AddressFile == NULL)
    {
        DPRINT1("Not enough resources\n");
        return STATUS_NO_MEMORY;
    }
#ifndef NDEBUG
    ADD_ADDR_FILE_DPC(AddressFile);
#endif

    /* Initialize Address File fields */
    AddressFile->RefCount = 0;
    AddressFile->ContextCount = 0;
    AddressFile->Protocol = Protocol;
    RtlCopyMemory(&AddressFile->Address, Address, sizeof(*Address));
    AddressFile->NetInterface = lwip_netif;

    /* Entity ID */
    switch (Protocol)
    {
        case IPPROTO_TCP :
            InsertEntityInstance(CO_TL_ENTITY, &AddressFile->Instance);
            break;
        case IPPROTO_ICMP :
            InsertEntityInstance(ER_ENTITY, &AddressFile->Instance);
            break;
        default :
            /* UDP, RAW */
            InsertEntityInstance(CL_TL_ENTITY, &AddressFile->Instance);
            break;
    }

    /* TCP variables */
    AddressFile->HasListener = FALSE;

    /* UDP and RAW variables */
    KeInitializeSpinLock(&AddressFile->RequestLock);
    InitializeListHead(&AddressFile->RequestListHead);
    AddressFile->lwip_raw_pcb = NULL;

    /* Add to master list */
    InsertTailList(&AddressListHead, &AddressFile->ListEntry);

FINISH:
    /* Output the Address File */
    *_AddressFile = AddressFile;

    return STATUS_SUCCESS;
}

static
NTSTATUS
DoSend(
    PIRP Irp,
    PTCP_CONTEXT Context
)
{
    UINT SendBytes;
    PVOID Buffer;
    err_t lwip_err;
    
    /* Get send buffer and length */
    NdisQueryBuffer(Irp->MdlAddress, &Buffer, &SendBytes);
    
    /* Call into lwIP to initiate send */
    lwip_err = tcp_write(Context->lwip_tcp_pcb, Buffer, SendBytes, 0);
    switch (lwip_err)
    {
        case ERR_OK:
            return STATUS_SUCCESS;
        case ERR_MEM:
            DPRINT1("lwIP ERR_MEM. Context %p with PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_NO_MEMORY;
        case ERR_ARG:
            DPRINT1("lwIP ERR_ARG. Context %p with PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_INVALID_PARAMETER;
        case ERR_CONN:
            DPRINT1("lwIP ERR_CONN. Context %p with PCB %p\n", Context, Context->lwip_tcp_pcb);
            return STATUS_CONNECTION_ACTIVE;
        default:
            DPRINT1("Unknwon lwIP Error: %d. Context %p with PCB %p\n",
                lwip_err, Context, Context->lwip_tcp_pcb);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static
NTSTATUS
EnqueueIRP(
    PIRP Irp,
    PTCP_CONTEXT Context,
    PDRIVER_CANCEL CancelRoutine,
    UCHAR CancelMode,
    UCHAR PendingMode
)
{
    PTCP_REQUEST Request;
    
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
    if (Request == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    
    IoSetCancelRoutine(Irp, CancelRoutine);
    Request->Payload.PendingIrp = Irp;
    Request->CancelMode = CancelMode;
    Request->PendingMode = PendingMode;
    Request->PayloadType = TCP_REQUEST_PAYLOAD_IRP;
    
    InsertTailList(&Context->RequestListHead, &Request->ListEntry);
    
    return STATUS_PENDING;
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

/**
 * lwIP Callback Functions
 **/

static
err_t
lwip_tcp_accept_callback(
    void *arg,
    struct tcp_pcb *newpcb,
    err_t err
)
{
#ifndef NDEBUG
    KIRQL OldIrql;
#endif
    
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_CONTEXT ListenContext;
    PTCP_REQUEST Request;
    
    /* lwIP currently never sends anything other than ERR_OK here */
    
    ListenContext = (PTCP_CONTEXT)arg;
    if (ListenContext == NULL)
    {
        DPRINT("No listener.\n");
        return ERR_CLSD;
    }

#ifndef NDEBUG
    ADD_PCB(newpcb);
#endif
    
    /* Do non-dependent PCB setup */
    tcp_err(newpcb, lwip_tcp_err_callback);
    tcp_recv(newpcb, lwip_tcp_recv_callback);
    tcp_sent(newpcb, lwip_tcp_sent_callback);
    
    ContextMutexAcquire(ListenContext);
    
    /* Walk the listener's Request list for a TDI_LISTEN to complete */
    Head = &ListenContext->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        if (Request->PendingMode == TCP_REQUEST_PENDING_LISTEN)
        {
            /* Immediately block cancellation and remove the Request from the list so we can release
             * the Context mutex */
            Irp = Request->Payload.PendingIrp;
            Irp->Cancel = TRUE;
            IoSetCancelRoutine(Irp, NULL);
            RemoveEntryList(&Request->ListEntry);
            ContextMutexRelease(ListenContext);
            
            /* Extract container Context */
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            Context = IrpSp->FileObject->FsContext;
            
            /* Set Context and PCB fields */
            TCP_SET_STATE(TCP_STATE_CONNECTED, Context);
            Context->lwip_tcp_pcb = newpcb;
            tcp_arg(newpcb, Context);
            
            /* Complete the IRP */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
            
            ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
            
            return ERR_OK;
        }
        Entry = Entry->Flink;
    }
    
    /* If the list walk did not find a valid IRP, we need to enqueue the newpcb for the next
     * TDI_LISTEN to find. */
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
    if (Request == NULL)
    {
        ContextMutexRelease(ListenContext);
        DPRINT("Out of resources\n");
        return ERR_MEM;
    }
    
    Request->Payload.apcb = newpcb;
    Request->CancelMode = TCP_REQUEST_CANCEL_MODE_PRESERVE;
    Request->PendingMode = TCP_REQUEST_PENDING_ACCEPTED_CONNECTION;
    Request->PayloadType = TCP_REQUEST_PAYLOAD_PCB;
    
    InsertTailList(&ListenContext->RequestListHead, &Request->ListEntry);
    tcp_arg(newpcb, ListenContext);
    
    return ERR_OK;
}

static
err_t
lwip_tcp_recv_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err
)   
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    INT CopiedLength;
    INT RemainingDestBytes;
    UCHAR *CurrentDestLocation;
    
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PNDIS_BUFFER Buffer;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;
    
    struct pbuf *next;
    
    /* lwIP currently never sends anything other than ERR_OK here */
    
    Context = (PTCP_CONTEXT)arg;
    if (Context == NULL)
    {
        DPRINT("No receiving Context for PCB %p\n", tpcb);
        return ERR_CLSD;
    }
    ContextMutexAcquire(Context);
    
    /* A null buffer means the connection has been closed */
    if (p == NULL)
    {
        /* Call tcp_abort() to keep this PCB from lingering in CLOSE_WAIT */
        tcp_arg(Context->lwip_tcp_pcb, NULL);
        tcp_abort(Context->lwip_tcp_pcb);
#ifndef NDEBUG
        DPRINT("Context %p closed by lwIP\n", Context);
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        Context->lwip_tcp_pcb = NULL;
        TCP_SET_STATE(TCP_STATE_CLOSED, Context);
        ContextMutexRelease(Context);
        return ERR_ABRT;
    }
    
    /* Sanity check the Context */
    if (!(Context->TcpState & (TCP_STATE_CONNECTED|TCP_STATE_RECEIVING)))
    {
        ContextMutexRelease(Context);
        DPRINT("Receiving on unconnected Context %p from PCB %p\n", Context, tpcb);
        return ERR_ARG;
    }
    if (Context->lwip_tcp_pcb != tpcb)
    {
        ContextMutexRelease(Context);
        DPRINT1("Receive PCB mismatch. Context %p has %p, callback has %p\n",
            Context, Context->lwip_tcp_pcb, tpcb);
        return ERR_ARG;
    }
    
    /* Walk the Request list for a matching TDI_RECEIVE */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        Entry = Entry->Flink;
        if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
        {
            /* Found a TDI_RECEIVE. Block cancellations, dequeue, and proceed to data copy. */
            Irp = Request->Payload.PendingIrp;
            Irp->Cancel = TRUE;
            IoSetCancelRoutine(Irp, NULL);
            
            RemoveEntryList(&Request->ListEntry);
            ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
            
            goto COPY_DATA;
        }
    }
    
    /* If we did not find a TDI_RECEIVE, simply return an error. lwIP will save the refused data. */
    TCP_RMV_STATE(TCP_STATE_RECEIVING, Context);
    ContextMutexRelease(Context);
    DPRINT("Did not find a TDI_RECEIVE on Context %p marked as Receiving with PCB %p\n",
        Context, Context->lwip_tcp_pcb);
    return ERR_MEM;
    
COPY_DATA:
    /* Get buffer pointers to write to */
    Buffer = (PNDIS_BUFFER)Irp->MdlAddress;
    NdisQueryBuffer(Buffer, &CurrentDestLocation, &RemainingDestBytes);

    /**
     * Copy the data from the pbuf to the NDIS Buffer
    **/
    CopiedLength = 0;
    /* Copy entire pbuf payloads while there is room in the NDIS Buffer */
    while (RemainingDestBytes > p->len)
    {
        RtlCopyMemory(CurrentDestLocation, p->payload, p->len);

        /* Update pointers and byte count */
        CopiedLength += p->len;
        CurrentDestLocation += p->len;
        RemainingDestBytes -= p->len;

        /* If there is still data left, go to the next pbuf. Otherwise, we are done copying. */
        if (p->next != NULL)
        {
            next = p->next;
            pbuf_free(p);
            p = next;
        }
        else
        {
            pbuf_free(p);
            goto COPY_DONE;
        }
    }
    /* When the remaining room in the NDIS Buffer is no longer larger than the pbuf payload, do one
     * final copy to top off the NDIS Buffer, then update the byte count. */
    RtlCopyMemory(CurrentDestLocation, p->payload, RemainingDestBytes);
    CopiedLength += RemainingDestBytes;

COPY_DONE:
    /* Inform lwIP of how much data we copied */
    tcp_recved(Context->lwip_tcp_pcb, CopiedLength);

    /* Check for other pending Receive requests. If there are none, clear the Receive TCP State bit.
     * Otherwise, leave the state variable alone. We continue from where we left off in the list. */
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        Entry = Entry->Flink;
        /* If there are still pending Receive requests, skip the TCP State change */
        if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
        {
            goto STILL_PENDING;
        }
    }
    TCP_RMV_STATE(TCP_STATE_RECEIVING, Context);
    
STILL_PENDING:
    ContextMutexRelease(Context);
    
    /* Clean up the Request struct and the IRP */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = CopiedLength;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return ERR_OK;
}

static
err_t
lwip_tcp_sent_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    u16_t len
)
{
    NTSTATUS Status;
    
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;
    
    /* Sanity check the Context */
    Context = (PTCP_CONTEXT)arg;
    if (Context == NULL)
    {
        DPRINT("Callack on closed Context from PCB %p\n", tpcb);
        return ERR_CLSD;
    }
    ContextMutexAcquire(Context);
    if (!(Context->TcpState & TCP_STATE_SENDING))
    {
        ContextMutexRelease(Context);
        DPRINT("Callback on Context %p that is not sending for PCB %p\n", Context, tpcb);
        return ERR_ARG;
    }
    if (Context->lwip_tcp_pcb != tpcb)
    {
        ContextMutexRelease(Context);
        DPRINT("Sent PCB mismatch. Context %p has %p, callback has %p\n",
            Context, Context->lwip_tcp_pcb, tpcb);
        return ERR_ARG;
    }
    
    /* Grab the first TDI_SEND */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        Entry = Entry->Flink;
        if (Request->PendingMode == TCP_REQUEST_PENDING_SEND)
        {
            /* Block cancellation and finish request */
            Irp = Request->Payload.PendingIrp;
            Irp->Cancel = TRUE;
            IoSetCancelRoutine(Irp, NULL);
            
            RemoveEntryList(&Request->ListEntry);
            
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = len;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

            ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
            
            goto CHECK_FOR_NEXT_REQUEST;
        }
    }
    
    /* If we didn't find a TDI_SEND, something is wrong. */
    ContextMutexRelease(Context);
    DPRINT("No TDI_SEND on Context %p marked as SENDING with PCB %p\n",
        Context, Context->lwip_tcp_pcb);
    return ERR_ARG;
    
CHECK_FOR_NEXT_REQUEST:
    /* Walk the rest of the list to see if we need to service the next TDI_SEND */
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        if (Request->PendingMode == TCP_REQUEST_PENDING_SEND)
        {
            Status = DoSend(Request->Payload.PendingIrp, Context);
            ContextMutexRelease(Context);
            if (Status == STATUS_SUCCESS)
            {
                return ERR_OK;
            }
            else
            {
                DPRINT1("Not enough resources\n");
                return ERR_MEM;
            }
        }
        Entry = Entry->Flink;
    }
    
    /* If we walk the entirety of the list without finding another TDI_SEND, clear the state bit. */
    TCP_RMV_STATE(TCP_STATE_SENDING, Context);
    ContextMutexRelease(Context);
    return ERR_OK;
}

static
void
lwip_tcp_err_callback(
    void *arg,
    err_t err
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    PTCP_CONTEXT Context;
    
    DPRINT("lwIP closed a socket with arg %08x: %d\n", arg, err);
    
    Context = (PTCP_CONTEXT)arg;
    if (Context == NULL)
    {
        return;
    }
    
    ContextMutexAcquire(Context);
#ifndef NDEBUG
    REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
    Context->lwip_tcp_pcb = NULL;
    ContextMutexRelease(Context);
}

static
err_t
lwip_tcp_connected_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    err_t err
)
{
    PIRP Irp;
    PLIST_ENTRY Entry;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;
    
    /* lwIP currently never sends anything other than ERR_OK here */
    
    /* Sanity check the Context */
    Context = (PTCP_CONTEXT)arg;
    if (Context == NULL)
    {
        DPRINT("No callback Context for PCB %p\n", tpcb);
        return ERR_CLSD;
    }
    ContextMutexAcquire(Context);
    if (Context->TcpState != TCP_STATE_CONNECTING)
    {
        ContextMutexRelease(Context);
        DPRINT("Connection established for Context %p in state %08x on PCB %p\n",
            Context, Context->TcpState, tpcb);
        return ERR_ARG;
    }
    if (Context->lwip_tcp_pcb != tpcb)
    {
        ContextMutexRelease(Context);
        DPRINT("Connected PCB mismatch. Context %p has %p, callback has %p\n",
            Context, Context->lwip_tcp_pcb, tpcb);
        return ERR_ARG;
    }
    
    /* The Connect request should be the first and only Request */
    Entry = RemoveHeadList(&Context->RequestListHead);
    if (Entry == &Context->RequestListHead)
    {
        ContextMutexRelease(Context);
        DPRINT("No Connect request on Context %p\n", Context);
        return ERR_ARG;
    }
    
    Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
    Irp = Request->Payload.PendingIrp;
    
    /* Block cancellations */
    Irp->Cancel = TRUE;
    IoSetCancelRoutine(Irp, NULL);
    
    /* One last sanity check */
    if (Request->PendingMode != TCP_REQUEST_PENDING_CONNECT)
    {
        ContextMutexRelease(Context);
        DPRINT("Pending Request on Connecting Context %p is not a Connect\n", Context);
        ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return ERR_ARG;
    }
    
    ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
    
    /* Set Context fields and callback information */
    TCP_SET_STATE(TCP_STATE_CONNECTED, Context);
    tcp_sent(Context->lwip_tcp_pcb, lwip_tcp_sent_callback);
    tcp_recv(Context->lwip_tcp_pcb, lwip_tcp_recv_callback);
    
    ContextMutexRelease(Context);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    
    return ERR_OK;
}

