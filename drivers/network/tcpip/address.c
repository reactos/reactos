/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: addresses abstraction
 */

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

#ifndef NDEBUG
/* Debug global variables, for convenience */
volatile long int AddrFileCount;
volatile long int GlContextCount;
volatile long int PcbCount;

PADDRESS_FILE AddrFileArray[16];
PTCP_CONTEXT ContextArray[16];
struct tcp_pcb *PCBArray[16];

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
void lwip_tcp_err_callback(void *arg, err_t err);
static err_t lwip_tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t lwip_tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t lwip_tcp_receive_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t lwip_tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);

/* Forward-declare helper function */
VOID CloseAddress(PADDRESS_FILE AddressFile);
NTSTATUS DisassociateAddress(PTCP_CONTEXT Context);

#define AddrIsUnspecified(Address) ((Address->in_addr == 0) || (Address->in_addr == 0xFFFFFFFF))

#define TCP_SET_STATE(State,Context) \
    DPRINT("Setting Context %p State to %s\n", Context, #State); \
    Context->TcpState = State
#define TCP_ADD_STATE(State,Context) \
    DPRINT("Adding State %s to Context %p\n", #State, Context); \
    Context->TcpState |= State
#define TCP_RMV_STATE(State,Context) \
    DPRINT("Removing State %s from Context %p\n", #State, Context); \
    Context->TcpState &= ~State

/**
 * Recursive mutex guarding a TCP_CONTEXT using a KSPIN_LOCK residing in the Context's associated
 * ADDRESS_FILE. This mutex guards against concurrent access from multiple execution contexts if and
 * only if the Context is associated with an Address File. If TcpIpAssociateAddress has been called
 * on a TCP_CONTEXT struct, this mutex should be held when reading from or writing to any and all
 * fields in the struct until TcpIpDisassociateAddress is call on the same struct. 
 * 
 * Mutex acquisition returns TRUE if the mutex has been acquired. Returns FALSE if the mutex could
 * not be acquired due to the Context having no association to any Address File.
 * 
 * A disassociated context should be inherently safe from concurrent access because the only
 * remaining reference to it should reside in an lwIP Protocol Control Block as the callback
 * argument. Thus, lwIP callback functions can safely ignore the return value while all other
 * functions should treat a return value of FALSE as a non-recoverable error.
 **/
VOID
InitializeContextMutex(
    PTCP_CONTEXT Context
)
{
    Context->MutexOwner = NULL;
    Context->MutexDepth = 0;
}

BOOLEAN
GetExclusiveContextAccess(
    PTCP_CONTEXT Context
)
{
    HANDLE Thread;
    KIRQL OldIrql;

    PADDRESS_FILE AddressFile;
    PKSPIN_LOCK Lock;

#ifndef NDEBUG
    Thread = PsGetCurrentThreadId();
    DPRINT("Thread %p acquiring lock on Context %p\n", Thread, Context);
#endif
    
    AddressFile = Context->AddressFile;
    if (AddressFile == NULL)
    {
        /* Context has been disassociated. There is no lock to acquire. */
        InterlockedIncrement(&Context->MutexDepth);
        return FALSE;
    }
    Lock = &AddressFile->AssociatedContextsLock;

    Thread = PsGetCurrentThreadId();

AGAIN:
    /* Start mutex acquisition */
    KeAcquireSpinLock(Lock, &OldIrql);
    if (Context->AddressFile == NULL)
    {
        /* Context was disassociated while we tried to acquire lock. IRP processing should stop. */
        KeReleaseSpinLock(Lock, OldIrql);
        InterlockedIncrement(&Context->MutexDepth);
        return FALSE;
    }
    if (Context->MutexOwner != NULL && Context->MutexOwner != Thread)
    {
        /* Context was acquired by another thread, try again */
        KeReleaseSpinLock(Lock, OldIrql);
        goto AGAIN;
    }

    /* We passed all tests, we have exclusive access to this Context. */
    Context->MutexOwner = Thread;
    KeReleaseSpinLock(Lock, OldIrql);
    InterlockedIncrement(&Context->MutexDepth);

    return TRUE;
}

VOID
ReleaseExclusiveContextAccess(
    PTCP_CONTEXT Context
)
{
    HANDLE Thread;
    
    Thread = PsGetCurrentThreadId();
    DPRINT("Thread %p releasing lock on Context %p\n", Thread, Context);
    
    /* If the releasing call came from the mutex owner, do release. Otherwise, do nothing. */
    if (Context->MutexOwner == Thread)
    {
        if (InterlockedDecrement(&Context->MutexDepth) == 0)
        {
            Context->MutexOwner = NULL;
        }
    }
    else
    {
        DPRINT1("Release mutex from non-owner\n");
    }
}

/* Get exclusive access to the corresponding TCP Context before calling */
NTSTATUS
EnqueueRequest(
    PVOID Payload,
    PTCP_CONTEXT Context,
    UCHAR CancelMode,
    UCHAR PendingMode,
    UCHAR PayloadType
)
{
    PTCP_REQUEST Request;

    /* Allocate and initialize TCP Request */
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
    if (Request == NULL)
    {
        DPRINT1("Not enough resources\n");
        return STATUS_NO_MEMORY;
    }
    Request->Payload.PendingIrp = Payload;
    Request->CancelMode = CancelMode;
    Request->PendingMode = PendingMode;
    Request->PayloadType = PayloadType;

    /* Enqueue request into Context's request list */
    InsertTailList(&Context->RequestListHead, &Request->ListEntry);

    return STATUS_PENDING;
}

/* Get exclusive access to the corresponding TCP Context before calling */
NTSTATUS
PrepareIrpForCancel(
    PIRP Irp,
    PTCP_CONTEXT Context,
    PDRIVER_CANCEL CancelRoutine,
    UCHAR CancelMode,
    UCHAR PendingMode
)
{
    NTSTATUS Status;

    /* Check that the IRP was not already cancelled */
    if (Irp->Cancel)
    {
        DPRINT("IRP already cancelled\n");
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        return STATUS_CANCELLED;
    }

    /* Create and enqueue TCP Request */
    Status = EnqueueRequest(Irp, Context, CancelMode, PendingMode, TCP_REQUEST_PAYLOAD_IRP);

    /* Set the IRP's Cancel routine */
    IoSetCancelRoutine(Irp, CancelRoutine);

    return Status;
}

/* Dequeue Request, set Irp->CancelRoutine to NULL, get exclusive access to the Context, and set
 * Irp->IoStatus.Information if needed before calling. This function deallocates the Request struct
 * and completes the IRP. This function may also deallocate the lwIP PCB and set the PCB pointer in
 * the Context struct to NULL. */
VOID
CleanupRequest(
    PTCP_REQUEST Request,
    NTSTATUS Status,
    PTCP_CONTEXT Context
)
{
#ifndef NDEBUG
    KIRQL OldIrql;
    INT i;
    PIO_STACK_LOCATION IrpSp;
#endif
    LONG ContextCount;

    PIRP Irp;
    PTCP_CONTEXT AcceptingContext;

    /* If this was just a request completion, skip lwIP PCB cleanup and go straight to IRP
     * completion */
    if (Status == STATUS_SUCCESS)
    {
        goto COMPLETE_IRP;
    }

    /* This is not a simple completion. Perform cleanup depending on Request payload type. */
    switch (Request->PayloadType)
    {
        case TCP_REQUEST_PAYLOAD_IRP :
            /* The Request holds a pending IRP */
            goto CLEANUP_PCB;
        case TCP_REQUEST_PAYLOAD_CONTEXT :
            /* The Request holds a dummy Context meant for incomming newly accepted connections */
            AcceptingContext = Request->Payload.Context;

            /* If an incomming connection has already been accepted, terminate it. */
            if (Request->PendingMode == TCP_REQUEST_PENDING_ACCEPTED_CONNECTION)
            {
#ifndef NDEBUG
                REMOVE_PCB(AcceptingContext->lwip_tcp_pcb);
#endif
                ACQUIRE_SERIAL_MUTEX();
                tcp_close(AcceptingContext->lwip_tcp_pcb);
                RELEASE_SERIAL_MUTEX();
            }

            /* If the Context does not have an upper-level reference, deallocate it. */
            if (AcceptingContext->ReferencedByUpperLayer == FALSE)
            {
                /* If removing this Context removes the last reference to its Address File, also
                 * deallocate the Address File. */
                ContextCount = InterlockedDecrement(&AcceptingContext->AddressFile->ContextCount);
                if (ContextCount < 1)
                {
                    CloseAddress(AcceptingContext->AddressFile);
                }
#ifndef NDEBUG
                REMOVE_CONTEXT(AcceptingContext);
#endif
                ExFreePoolWithTag(AcceptingContext, TAG_TCP_CONTEXT);
            }

            /* Skip IRP completion, because there is no IRP to complete. */
            goto FINISH;
        default :
            DPRINT1("We should never reach here. Something is wrong.\n");
            goto FINISH;
    }

CLEANUP_PCB:
    /* If cancelling, clean up the lwIP PCB according to request type */
    switch (Request->CancelMode)
    {
        case TCP_REQUEST_CANCEL_MODE_CLOSE :
            TCP_SET_STATE(TCP_STATE_CLOSED, Context);
            if (Context->lwip_tcp_pcb != NULL)
            {
#ifndef NDEBUG
                REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
                ACQUIRE_SERIAL_MUTEX();
                tcp_close(Context->lwip_tcp_pcb);
                RELEASE_SERIAL_MUTEX();
                Context->lwip_tcp_pcb = NULL;
            }
            goto COMPLETE_IRP;
        case TCP_REQUEST_CANCEL_MODE_ABORT :
            TCP_SET_STATE(TCP_STATE_ABORTED, Context);
            if (Context->lwip_tcp_pcb != NULL)
            {
#ifndef NDEBUG
                REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
                ACQUIRE_SERIAL_MUTEX();
                tcp_abort(Context->lwip_tcp_pcb);
                RELEASE_SERIAL_MUTEX();
                Context->lwip_tcp_pcb = NULL;
            }
            goto COMPLETE_IRP;
        case TCP_REQUEST_CANCEL_MODE_PRESERVE :
            /* For requests that do not deallocate the PCB when cancelled, determine and clear the
             * appropriate TCP State bit */
            switch (Request->PendingMode)
            {
                case TCP_REQUEST_PENDING_SEND :
                    TCP_RMV_STATE(TCP_STATE_SENDING, Context);
                    break;
                case TCP_REQUEST_PENDING_RECEIVE :
                    TCP_RMV_STATE(TCP_STATE_RECEIVING, Context);
                    break;
                default :
                    DPRINT1("We should never reach here. Something is wrong.\n");
                    goto FINISH;
            }
            goto COMPLETE_IRP;
        default :
            DPRINT1("We should never reach here. Something is wrong.\n");
            goto FINISH;
    }

COMPLETE_IRP:
    /* Complete the IRP */
    Irp = Request->Payload.PendingIrp;
    Irp->IoStatus.Status = Status;
#ifndef NDEBUG
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    REMOVE_IRP(Irp);
    REMOVE_IRPSP(IrpSp);
#endif
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

FINISH:
    /* Deallocate the TCP Request */
    ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
}

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
    
    PIO_STACK_LOCATION IrpSp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;

    /* Block potential repeated cancellations */
    IoSetCancelRoutine(Irp, NULL);

    /* This function is always called with the Cancel lock held */
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /* The file types distinguishes between some protocols */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    switch ((ULONG)IrpSp->FileObject->FsContext2)
    {
        case TDI_TRANSPORT_ADDRESS_FILE :
            goto DGRAM_CANCEL;
        case TDI_CONNECTION_FILE :
            goto TCP_CANCEL;
        default :
            DPRINT1("IRP does not contain a valid FileObject\n");
            goto FINISH;
    }

DGRAM_CANCEL:
    DPRINT1("Datagram cancel not yet implemented\n");
    goto FINISH;

TCP_CANCEL:
    Context = IrpSp->FileObject->FsContext;
    GetExclusiveContextAccess(Context);
    
    /* If this is a cancellation on a TDI_LISTEN request, we need to disassociate and close the
     * Context. AFD does not know about the extra Context we keep for listening sockets. AFD only
     * cares about actual connection endpoints. */
    if (IrpSp->MinorFunction == TDI_LISTEN)
    {
        /* Disassociate the listening Context, which we grab from the AddressFile. */
        Context = Context->AddressFile->Listener;
        DisassociateAddress(Context);
        
        /* If the lwIP PCB still exists, close it. Since we are deallocating the PCB's associated
         * Context, we also need to clear the Context pointer in the PCB. */
        if (Context->lwip_tcp_pcb)
        {
#ifndef NDEBUG
            REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
            ACQUIRE_SERIAL_MUTEX();
            tcp_arg(Context->lwip_tcp_pcb, NULL);
            tcp_close(Context->lwip_tcp_pcb);
            RELEASE_SERIAL_MUTEX();
        }

        /* Deallocate the Context. The corresponding Address File's ContextCount should have been
         * decremented when the Context was disassociated. */
#ifndef NDEBUG
        REMOVE_CONTEXT(Context);
#endif
        ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);
        
        return;
    }

    /* Walk the TCP Context's list of requests to find one with this IRP */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);

        if (Request->Payload.PendingIrp == Irp)
        {
            /* Immediately remove the request from the queue before processing */
            RemoveEntryList(&Request->ListEntry);
            Irp->IoStatus.Information = 0;
            CleanupRequest(Request, STATUS_CANCELLED, Context);
            ReleaseExclusiveContextAccess(Context);
            return;
        }

        Entry = Entry->Flink;
    }

    ReleaseExclusiveContextAccess(Context);

    DPRINT1("Did not find a matching TCP Request, we may leave a dead IRP pointer somewhere\n");

FINISH:
#ifndef NDEBUG
    REMOVE_IRP(Irp);
    REMOVE_IRPSP(IrpSp);
#endif
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return;
}

void
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

/* Acquire AddressListLock before calling */
NTSTATUS
CreateAddressAndEnlist(
    _In_ struct netif *lwip_netif,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol,
    _In_ USHORT ShareAccess,
    _Out_ PADDRESS_FILE *_AddressFile
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
    KeInitializeSpinLock(&AddressFile->AssociatedContextsLock);

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
                    ACQUIRE_SERIAL_MUTEX();
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

VOID
CloseAddress(
    PADDRESS_FILE AddressFile
)
{
#ifndef NDEBUG
    INT i;
#endif
    KIRQL OldIrql;

    /* Remove the Address File from global lists before further processing */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);
    RemoveEntryList(&AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);
    RemoveEntityInstance(&AddressFile->Instance);

    /* For ICMP, RAW, UDP addresses, we need to deallocate the lwIP PCB */
    switch (AddressFile->Protocol)
    {
        case IPPROTO_ICMP :
        case IPPROTO_RAW :
            raw_remove(AddressFile->lwip_raw_pcb);
            break;
        case IPPROTO_UDP :
            udp_remove(AddressFile->lwip_udp_pcb);
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
    InitializeContextMutex(Context);

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
TcpIpCloseContext(
    _In_ PTCP_CONTEXT Context
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif

    /* Sanity check */
    if (Context->AddressFile != NULL)
    {
        DPRINT1("Context retains address association\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* If the lwIP PCB still exists, close it. Since we are deallocating the PCB's associated
     * Context, we also need to clear the Context pointer in the PCB. */
    if (Context->lwip_tcp_pcb)
    {
#ifndef NDEBUG
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        ACQUIRE_SERIAL_MUTEX();
        tcp_arg(Context->lwip_tcp_pcb, NULL);
        tcp_close(Context->lwip_tcp_pcb);
        RELEASE_SERIAL_MUTEX();
    }

    /* Deallocate the Context. The corresponding Address File's ContextCount should have been
     * decremented when the Context was disassociated. */
#ifndef NDEBUG
    REMOVE_CONTEXT(Context);
#endif
    ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);

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
        DPRINT1("We should be associating a new TCP Context with a TCP Address File\n");
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
    ACQUIRE_SERIAL_MUTEX();
    tcp_arg(Context->lwip_tcp_pcb, Context);
    tcp_err(Context->lwip_tcp_pcb, lwip_tcp_err_callback);

    /* Attempt to bind the PCB. lwIP internally handles INADDR_ANY. */
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
            DPRINT1("lwIP ERR_BUFF\n");
            Status = STATUS_NO_MEMORY;
            goto FINISH;
        }
        case (ERR_VAL) :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP ERR_VAL\n");
            Status = STATUS_INVALID_PARAMETER;
            goto FINISH;
        }
        case (ERR_USE) :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP ERR_USE\n");
            Status = STATUS_ADDRESS_ALREADY_EXISTS;
            goto FINISH;
        }
        default :
        {
            RELEASE_SERIAL_MUTEX();
            DPRINT1("lwIP unexpected error\n");
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

/* This function does not require the Context mutex to be held. Holding the mutex while calling will
 * succeed but will decrease performance, since the mutex will be acquired recursively. */
NTSTATUS
DisassociateAddress(
    PTCP_CONTEXT Context
)
{
    LONG ContextCount;

    PADDRESS_FILE AddressFile;
    PIRP PendingIrp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_REQUEST Request;
    
    /* Immediately remove the Context's association with its Address File */
    if (GetExclusiveContextAccess(Context) == FALSE)
    {
        ReleaseExclusiveContextAccess(Context);
        return STATUS_SUCCESS;
    }
    AddressFile = Context->AddressFile;
    Context->AddressFile = NULL;
    ReleaseExclusiveContextAccess(Context);
    
    /* Walk the Context's Request list to finish all outstanding IRPs. */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        /* Remove the Request from the list and increment list walk */
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        Entry = Entry->Flink;
        RemoveEntryList(&Request->ListEntry);

        /* If this is an IRP, block potential cancellations. */
        if (Request->PayloadType == TCP_REQUEST_PAYLOAD_IRP)
        {
            PendingIrp = Request->Payload.PendingIrp;
            IoSetCancelRoutine(PendingIrp, NULL);
        }

        /* Clean up the Request */
        CleanupRequest(Request, STATUS_CANCELLED, Context);
    }

    /* Decrement the corresponding Address File's ContextCount, remove the Address File if needed */
    if (AddressFile != NULL)
    {
        ContextCount = InterlockedDecrement(&AddressFile->ContextCount);
        if (ContextCount < 1)
        {
            CloseAddress(AddressFile);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpDisassociateAddress(
    _Inout_ PIRP Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Sanity checks */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Disassociating something that is not a TCP Context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;

    return DisassociateAddress(Context);
}

NTSTATUS
TcpIpListen(
    _Inout_ PIRP Irp
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    NTSTATUS Status;
    LONG ContextCount;

    PADDRESS_FILE AddressFile;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    PTCP_CONTEXT ListenContext;
    PTCP_REQUEST Request;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Grab TCP Context from IRP */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Not a connection context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (GetExclusiveContextAccess(Context) == FALSE)
    {
        DPRINT("Context has been disassociated\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ADDRESS_CLOSED;
    }
    if (Context->TcpState != TCP_STATE_BOUND)
    {
        DPRINT1("Context is not a bound context\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_INVALID_PARAMETER;
    }

    AddressFile = Context->AddressFile;
    /* If there is already a listener on the address, this context is a dummy connection endpoint
     * used to poll/wait for new accepted connections. Check for queued accepted connections. If
     * there are none, queue the new Context and the Listen Request. */
    if (AddressFile->HasListener == TRUE)
    {
        ListenContext = AddressFile->Listener;
        if (GetExclusiveContextAccess(ListenContext) == FALSE)
        {
            DPRINT1("TDI_LISTEN on disassociated Listen Context? SNAFU.\n");
            ReleaseExclusiveContextAccess(ListenContext);
            ReleaseExclusiveContextAccess(Context);
            return STATUS_ADDRESS_CLOSED;
        }

        /* Check for queued accepted connections in the listener's Request queue */
        Head = &ListenContext->RequestListHead;
        Entry = Head->Flink;
        while (Entry != Head)
        {
            Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
            if (Request->PendingMode == TCP_REQUEST_PENDING_ACCEPTED_CONNECTION)
            {
                RemoveEntryList(&Request->ListEntry);
                ReleaseExclusiveContextAccess(ListenContext);
                goto CONNECTION_AVAILABLE;
            }
        }

        /* If there is no queued accepted connection, enqueue a Request with the new Context as the
         * payload, then enqueue the IRP. Finding a queued accepted connection jumps beyond here. */
        EnqueueRequest(
            Context,
            ListenContext,
            TCP_REQUEST_CANCEL_MODE_PRESERVE,
            TCP_REQUEST_PENDING_LISTEN_POLL,
            TCP_REQUEST_PAYLOAD_CONTEXT);

        /* Enqueue the IRP */
        Status = PrepareIrpForCancel(
            Irp,
            ListenContext,
            CancelRequestRoutine,
            TCP_REQUEST_CANCEL_MODE_CLOSE,
            TCP_REQUEST_PENDING_LISTEN);

        ReleaseExclusiveContextAccess(ListenContext);
        ReleaseExclusiveContextAccess(Context);
        return Status;

CONNECTION_AVAILABLE:
        /* If there is a queued accepted connection, deallocate the Context that came with the IRP
         * and return the Context that is in the queue with IRP. */
#ifndef NDEBUG
        REMOVE_CONTEXT(Context);
#endif
        ContextCount = InterlockedDecrement(&Context->AddressFile->ContextCount);
        if (ContextCount < 1)
        {
            CloseAddress(Context->AddressFile);
        }
        Context->AddressFile = NULL;
        ReleaseExclusiveContextAccess(Context);
        ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);

        /* Set the IRP's FileObject to point to the queued Context. We also need to mark this
         * Context as created through a request by an upper level driver, since it replaces one that
         * actually was created that way. */
        Context = Request->Payload.Context;
        TCP_SET_STATE(TCP_STATE_CONNECTED, Context);
        IrpSp->FileObject->FsContext = Context;
        Context->ReferencedByUpperLayer = TRUE;

        /* Inform lwIP that we accepted the connection */
        ACQUIRE_SERIAL_MUTEX();
        tcp_accepted(ListenContext->lwip_tcp_pcb);
        RELEASE_SERIAL_MUTEX();

        ExFreePoolWithTag(Request, TAG_TCP_REQUEST);

        /* main.c will call IoCompleteRequest() on this IRP */
        return STATUS_SUCCESS;
    }

    /* If there is not already a listener on the address, create a Listen Context and enqueue the
     * existing Context as a dummy Context. We will later perform our own disassociation and closure
     * on the Listen Context. */
    /* Create context */
    ListenContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ListenContext), TAG_TCP_CONTEXT);
    if (ListenContext == NULL)
    {
        DPRINT1("Not enough resources\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_NO_MEMORY;
    }
#ifndef NDEBUG
    ADD_CONTEXT(ListenContext);
#endif

    /* Set initial values */
    ListenContext->AddressFile = AddressFile;
    InitializeListHead(&ListenContext->RequestListHead);
    ListenContext->ReferencedByUpperLayer = FALSE;
    InitializeContextMutex(ListenContext);
    InterlockedIncrement(&AddressFile->ContextCount);
    
    /* Call into lwIP to initiate Listen */
#ifndef NDEBUG
    REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
    ACQUIRE_SERIAL_MUTEX();
    ListenContext->lwip_tcp_pcb = tcp_listen(Context->lwip_tcp_pcb);
    RELEASE_SERIAL_MUTEX();
    if (ListenContext->lwip_tcp_pcb == NULL)
    {
        DPRINT("Bind failed\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_INVALID_ADDRESS;
    }
    Context->lwip_tcp_pcb = NULL;
#ifndef NDEBUG
    ADD_PCB(ListenContext->lwip_tcp_pcb);
#endif

    /* Set lwIP callback information for new PCB */
    ACQUIRE_SERIAL_MUTEX();
    tcp_arg(ListenContext->lwip_tcp_pcb, ListenContext);
    tcp_accept(ListenContext->lwip_tcp_pcb, lwip_tcp_accept_callback);
    RELEASE_SERIAL_MUTEX();

    /* Mark the Address File as having a listener */
    AddressFile->HasListener = TRUE;
    AddressFile->Listener = ListenContext;
    
    /* Enqueue the dummy Context to receive new connections */
    EnqueueRequest(
        Context,
        ListenContext,
        TCP_REQUEST_CANCEL_MODE_PRESERVE,
        TCP_REQUEST_PENDING_LISTEN_POLL,
        TCP_REQUEST_PAYLOAD_CONTEXT);
    
    /* Mark IRP as pending and enqueue the Listen Request on the Listen Context */
    Status = PrepareIrpForCancel(
        Irp,
        ListenContext,
        CancelRequestRoutine,
        TCP_REQUEST_CANCEL_MODE_CLOSE,
        TCP_REQUEST_PENDING_LISTEN);
    TCP_SET_STATE(TCP_STATE_LISTENING, ListenContext);

    ReleaseExclusiveContextAccess(Context);
    return Status;
}

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

    /* Sanity checks */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("File object is not a connection context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (GetExclusiveContextAccess(Context) == FALSE)
    {
        DPRINT1("Context has been disassociated\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ADDRESS_CLOSED;
    }
    if (Context->TcpState != TCP_STATE_BOUND)
    {
        DPRINT1("Connecting from unbound socket\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_INVALID_PARAMETER;
    }

    /* Extract remote address to connect to */
    RequestInfo = (PTDI_REQUEST_KERNEL_CONNECT)&IrpSp->Parameters;
    RemoteTransportAddress = RequestInfo->RequestConnectionInformation->RemoteAddress;
    SocketAddressRemote = (struct sockaddr *)&RemoteTransportAddress->Address[0];
    SocketAddressInRemote = (struct sockaddr_in *)&SocketAddressRemote->sa_data;

    /* Call into lwIP to initiate Connect */
    ACQUIRE_SERIAL_MUTEX();
    lwip_err = tcp_connect(Context->lwip_tcp_pcb,
        (ip_addr_t*)&SocketAddressInRemote->sin_addr.s_addr,
        SocketAddressInRemote->sin_port,
        lwip_tcp_connected_callback);
    RELEASE_SERIAL_MUTEX();
    switch (lwip_err)
    {
        case ERR_OK :
            /* If successful, enqueue the IRP, set the TCP State variable, and return */
            Status = PrepareIrpForCancel(
                Irp,
                Context,
                CancelRequestRoutine,
                TCP_REQUEST_CANCEL_MODE_ABORT,
                TCP_REQUEST_PENDING_CONNECT);
            TCP_SET_STATE(TCP_STATE_CONNECTING, Context);
            goto FINISH;
        case ERR_VAL :
            DPRINT1("lwip ERR_VAL\n");
            Status = STATUS_INVALID_PARAMETER;
            goto FINISH;
        case ERR_ISCONN :
            DPRINT1("lwip ERR_ISCONN\n");
            Status = STATUS_CONNECTION_ACTIVE;
            goto FINISH;
        case ERR_RTE :
            DPRINT1("lwip ERR_RTE\n");
            Status = STATUS_NETWORK_UNREACHABLE;
            goto FINISH;
        case ERR_BUF :
            /* Use correct error once NDIS errors are included.
             * This return value means local port unavailable. */
            DPRINT1("lwip ERR_BUF\n");
            Status = STATUS_ADDRESS_ALREADY_EXISTS;
            goto FINISH;
        case ERR_USE :
            DPRINT1("lwip ERR_USE\n");
            Status = STATUS_CONNECTION_ACTIVE;
            goto FINISH;
        case ERR_MEM :
            DPRINT1("lwip ERR_MEM\n");
            Status = STATUS_NO_MEMORY;
            goto FINISH;
        default :
            /* unknown return value */
            DPRINT1("lwip unknown return code\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto FINISH;
    }

FINISH:
    ReleaseExclusiveContextAccess(Context);
    return Status;
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
    
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;
    
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    /* Sanity checks. We do not acquire the Context mutex because TDI_DISCONNECT results from an
     * IoCompleteRequest() called with the Context mutex held. */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Disconnection on something that is not a TCP Context\n");
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Mark the Context for disconnect. Do not shut down the PCB, because this function is called
     * while another thread context holds the global MTSerialMutex. */
    Context = IrpSp->FileObject->FsContext;
    TCP_SET_STATE(TCP_STATE_CLOSED, Context);
    
    return STATUS_SUCCESS;
}

NTSTATUS
TcpIpReceive(
    _Inout_ PIRP Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    PTCP_CONTEXT Context;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Sanity checks */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Received TDI_RECEIVE for something that is not a TCP Context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (GetExclusiveContextAccess(Context) == FALSE)
    {
        DPRINT("Context has been disassociated\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ADDRESS_CLOSED;
    }
    if (!(Context->TcpState & TCP_STATE_CONNECTED))
    {
        DPRINT1("TCP Context %p is in the wrong state for receiving data: %08x\n",
            Context,
            Context->TcpState);
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ADDRESS_CLOSED;
    }

    /* No need to call into lwIP. The Receive callback should have been set when the connection was
     * established. */

    /* Mark IRP as pending, and the TCP Context as Receiving */
    PrepareIrpForCancel(
        Irp,
        Context,
        CancelRequestRoutine,
        TCP_REQUEST_CANCEL_MODE_PRESERVE,
        TCP_REQUEST_PENDING_RECEIVE);
    TCP_ADD_STATE(TCP_STATE_RECEIVING, Context);

    ReleaseExclusiveContextAccess(Context);
    return STATUS_PENDING;
}

NTSTATUS
TcpIpSend(
    _Inout_ PIRP Irp
)
{
    NTSTATUS Status;
    UINT SendBytes;

    PIO_STACK_LOCATION IrpSp;
    PTDI_REQUEST_KERNEL_SEND RequestInfo;
    PTCP_CONTEXT Context;
    PVOID Buffer;

    err_t lwip_err;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Sanity checks */
    if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
    {
        DPRINT1("Received TDI_SEND for something that is not a TCP Context\n");
        return STATUS_INVALID_PARAMETER;
    }
    Context = IrpSp->FileObject->FsContext;
    if (GetExclusiveContextAccess(Context) == FALSE)
    {
        DPRINT1("Context has been disassociated\n");
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ADDRESS_CLOSED;
    }
    if (!(Context->TcpState & TCP_STATE_CONNECTED))
    {
        DPRINT1("Attempting to send on Context at %p without an established connection\n", Context);
        ReleaseExclusiveContextAccess(Context);
        return STATUS_ONLY_IF_CONNECTED;
    }

    /* Get send buffer and length */
    NdisQueryBuffer(Irp->MdlAddress, &Buffer, &SendBytes);
    RequestInfo = (PTDI_REQUEST_KERNEL_SEND)&IrpSp->Parameters;

    /* If the Context is already servicing a Send request, do not initiate another one right now */
    if (Context->TcpState & TCP_STATE_SENDING)
    {
        goto WAIT_TO_SEND;
    }

    /* Call into lwIP to initiate send */
    ACQUIRE_SERIAL_MUTEX();
    lwip_err = tcp_write(Context->lwip_tcp_pcb, Buffer, RequestInfo->SendLength, 0);
    RELEASE_SERIAL_MUTEX();
    switch (lwip_err)
    {
        case ERR_OK:
            /* If the lwIP call succeeded, set the TCP State variable */
            TCP_ADD_STATE(TCP_STATE_SENDING, Context);
            break;
        case ERR_MEM:
            DPRINT1("lwIP ERR_MEM\n");
            Status = STATUS_NO_MEMORY;
            goto FINISH;
        case ERR_ARG:
            DPRINT1("lwIP ERR_ARG\n");
            Status = STATUS_INVALID_PARAMETER;
            goto FINISH;
        case ERR_CONN:
            DPRINT1("lwIP ERR_CONN\n");
            Status = STATUS_CONNECTION_ACTIVE;
            goto FINISH;
        default:
            DPRINT1("Unknwon lwIP Error: %d\n", lwip_err);
            Status = STATUS_NOT_IMPLEMENTED;
            goto FINISH;
    }

WAIT_TO_SEND:
    Status = PrepareIrpForCancel(
        Irp,
        Context,
        CancelRequestRoutine,
        TCP_REQUEST_CANCEL_MODE_PRESERVE,
        TCP_REQUEST_PENDING_SEND);

FINISH:
    ReleaseExclusiveContextAccess(Context);
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
 * Datagram Functions
 */

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
#ifndef NDEBUG
    INT i;
#endif
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

#ifndef NDEBUG
    REMOVE_IRP(Irp);
    REMOVE_IRPSP(IrpSp);
#endif
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    ExFreePoolWithTag(Request, TAG_DGRAM_REQST);
}

NTSTATUS
TcpIpReceiveDatagram(
    _Inout_ PIRP Irp)
{
#ifndef NDEBUG
    KIRQL OldIrql;
    INT i;
#endif
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
#ifndef NDEBUG
    REMOVE_IRP(Irp);
    REMOVE_IRPSP(IrpSp);
#endif
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}

NTSTATUS
TcpIpSendDatagram(
    _Inout_ PIRP Irp)
{
#ifndef NDEBUG
    KIRQL OldIrql;
    INT i;
#endif
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
#ifndef NDEBUG
    REMOVE_IRP(Irp);
    REMOVE_IRPSP(IrpSp);
#endif
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}

/* Acquire Context mutex before calling. If the PCB is deallocated, also set the PCB pointer to NULL
 * before calling. */
VOID
ProcessPCBError(
    PTCP_CONTEXT Context,
    ULONG TcpState)
{
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_REQUEST Request;

    /* Set the Context's State to indicate it is no longer active */
    TCP_SET_STATE(TcpState, Context);

    /* Walk the Context's list of pending requests and finish them */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        /* Extract Request */
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);

        /* Block potential cancellations */
        Irp = Request->Payload.PendingIrp;
        IoSetCancelRoutine(Irp, NULL);

        /* Dequeue Request and increment list walk */
        Entry = Entry->Flink;
        RemoveEntryList(&Request->ListEntry);

        /* Complete the IRP, deallocate the Request, and deallocate the lwIP PCB if necessary */
        Irp->IoStatus.Information = 0;
        CleanupRequest(Request, STATUS_CANCELLED, Context);
    }
}

void
lwip_tcp_err_callback(
    void *arg,
    err_t err
)
{
    ULONG TcpState;

    PTCP_CONTEXT Context;

    /* Interpret lwIP error */
    // TODO: detailed NTSTATUS codes
    switch (err)
    {
        case ERR_ABRT :
            DPRINT1("lwIP socket aborted\n");
            TcpState = TCP_STATE_ABORTED;
            break;
        case ERR_RST :
            /* This is the only case that indicates the lwIP PCB still exists */
            DPRINT1("lwIP socket reset\n");
            TcpState = TCP_STATE_CLOSED;
            goto RETAIN_PCB;
        case ERR_CLSD :
            DPRINT1("lwIP socket closed\n");
            TcpState = TCP_STATE_CLOSED;
            break;
        case ERR_CONN :
            DPRINT1("lwIP connection failed\n");
            TcpState = TCP_STATE_CLOSED;
            break;
        case ERR_ARG :
            DPRINT1("lwIP invalid arguments\n");
            TcpState = TCP_STATE_ABORTED;
            break;
        case ERR_IF :
            DPRINT1("Low-level error\n");
            TcpState = TCP_STATE_ABORTED;
            break;
        default :
            DPRINT1("Unsupported lwIP error code: %d\n", err);
            TcpState = TCP_STATE_ABORTED;
            break;
    }

    Context = (PTCP_CONTEXT)arg;
    if (Context == NULL)
    {
        return;
    }

    GetExclusiveContextAccess(Context);
    Context->lwip_tcp_pcb = NULL;
RETAIN_PCB:
    ProcessPCBError(Context, TcpState);
    ReleaseExclusiveContextAccess(Context);
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

    Context = (PTCP_CONTEXT)arg;
    GetExclusiveContextAccess(Context);

    /* Sanity checks */
    if (Context->TcpState != TCP_STATE_CONNECTING)
    {
        DPRINT1("Callback on a context that did not initiate a connection\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }
    if (Context->lwip_tcp_pcb != tpcb)
    {
        DPRINT1("Connected PCB mismatch\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }
    /* The Connect request should be the first request */
    Entry = RemoveHeadList(&Context->RequestListHead);
    if (Entry == &Context->RequestListHead)
    {
        DPRINT1("No matching Connect Request found\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }

    Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
    Irp = Request->Payload.PendingIrp;

    /* Block cancellations */
    IoSetCancelRoutine(Irp, NULL);

    /* One last sanity check */
    if (Request->PendingMode != TCP_REQUEST_PENDING_CONNECT)
    {
        DPRINT1("Pending Request is not a Connect request. This should never happen.\n");
        Irp->IoStatus.Information = 0;
        CleanupRequest(Request, STATUS_CANCELLED, Context);
        ReleaseExclusiveContextAccess(Context);
        return ERR_CONN;
    }

    /* Complete the Request, set TCP State variable, and set callback information */
    TCP_SET_STATE(TCP_STATE_CONNECTED, Context);
    tcp_sent(Context->lwip_tcp_pcb, lwip_tcp_sent_callback);
    tcp_recv(Context->lwip_tcp_pcb, lwip_tcp_receive_callback);
    CleanupRequest(Request, STATUS_SUCCESS, Context);

    ReleaseExclusiveContextAccess(Context);
    return ERR_OK;
}

static
err_t
lwip_tcp_accept_callback(
    void *arg,
    struct tcp_pcb *newpcb,
    err_t err
)
{
#ifndef NDEBUG
    INT i;
    KIRQL OldIrql;
#endif
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_CONTEXT NewContext;
    PTCP_REQUEST CurrentRequest;
    PTCP_REQUEST DummyRequest;
    PTCP_REQUEST Request;

    /* lwIP currently never sends anything other than ERR_OK here */

    Context = (PTCP_CONTEXT)arg;
    GetExclusiveContextAccess(Context);

    /* Sanity check */
    if (!(Context->TcpState & TCP_STATE_LISTENING))
    {
        DPRINT1("lwIP sending Accept event to non-listening TCP Context\n");
#ifndef NDEBUG
        REMOVE_PCB(Context->lwip_tcp_pcb);
#endif
        tcp_close(Context->lwip_tcp_pcb);
        Context->lwip_tcp_pcb = NULL;
        ReleaseExclusiveContextAccess(Context);
        return ERR_CLSD;
    }

    /* Look for a Listen request and an available dummy Context */
    Head = &Context->RequestListHead;
    NewContext = NULL;
    Request = NULL;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        // TODO: optimize checking logic

        CurrentRequest = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        if ((CurrentRequest->PendingMode == TCP_REQUEST_PENDING_LISTEN) && (Request == NULL))
        {
            /* This is the Listen request we are looking for. Immediately block cancellations, then
             * dequeue the Request. */
            Irp = CurrentRequest->Payload.PendingIrp;
            IoSetCancelRoutine(Irp, NULL);
            RemoveEntryList(&CurrentRequest->ListEntry);
            Request = CurrentRequest;

            /* If we have already found a dummy Context, associate it with the Established PCB. */
            if (NewContext != NULL)
            {
                RemoveEntryList(&DummyRequest->ListEntry);
                ExFreePoolWithTag(DummyRequest, TAG_TCP_REQUEST);
                goto CONTEXT_FOUND;
            }
        }
        else if ((CurrentRequest->PendingMode == TCP_REQUEST_PENDING_LISTEN_POLL)
            && (NewContext == NULL))
        {
            /* This is a dummy Context we can use instead of creating a new one. Store a reference
             * to it. No need to acquire exclusive access to this Context because this is the only
             * existing pointer to it at this moment. */
            NewContext = CurrentRequest->Payload.Context;
            DummyRequest = CurrentRequest;

            /* If we have already found a Listen request, we still must associate this Context with
             * the Established PCB before completing the Request. */
            if (Request != NULL)
            {
                RemoveEntryList(&DummyRequest->ListEntry);
                ExFreePoolWithTag(DummyRequest, TAG_TCP_REQUEST);
                goto CONTEXT_FOUND;
            }
        }
        Entry = Entry->Flink;
    }

    /* We need a Context to store the Established PCB in. If the list does not contain a dummy
     * Context, we need to create a new one. */
    if (NewContext == NULL)
    {
        NewContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*NewContext), TAG_TCP_CONTEXT);
        if (NewContext == NULL)
        {
            DPRINT1("Not enough resources\n");
            ReleaseExclusiveContextAccess(Context);
            return ERR_MEM;
        }
#ifndef NDEBUG
        ADD_CONTEXT(NewContext);
#endif
        NewContext->AddressFile = Context->AddressFile;
        InitializeListHead(&NewContext->RequestListHead);
        NewContext->ReferencedByUpperLayer = FALSE;
        InitializeContextMutex(NewContext);

        /* Increment the Address File's Context reference count */
        InterlockedIncrement(&NewContext->AddressFile->ContextCount);
    }

CONTEXT_FOUND:
    /* Associate the new Context and the Established PCB with each other  */
#ifndef NDEBUG
    ADD_PCB(newpcb);
#endif
    NewContext->lwip_tcp_pcb = newpcb;
    tcp_arg(NewContext->lwip_tcp_pcb, NewContext);
    tcp_err(NewContext->lwip_tcp_pcb, lwip_tcp_err_callback);
    tcp_sent(NewContext->lwip_tcp_pcb, lwip_tcp_sent_callback);
    tcp_recv(NewContext->lwip_tcp_pcb, lwip_tcp_receive_callback);

    /* If we found a Listen request, complete it. */
    if (Request != NULL)
    {
        /* Store new Context information */
        TCP_SET_STATE(TCP_STATE_CONNECTED, NewContext);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        IrpSp->FileObject->FsContext = NewContext;
        IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;

        /* Finish the Request */
        NewContext->ReferencedByUpperLayer = TRUE;
        CleanupRequest(Request, STATUS_SUCCESS, Context);

        /* Notify the listening lwIP PCB that we accepted the connection */
        tcp_accepted(Context->lwip_tcp_pcb);

        ReleaseExclusiveContextAccess(Context);
        return ERR_OK;
    }

    /* If we did not find a Listen request, we need to enqueue the new Context. The next TDI_LISTEN
     * will find it and complete without pending. */
    TCP_SET_STATE(TCP_STATE_BOUND, NewContext);
    Status = EnqueueRequest(
        NewContext,
        Context,
        TCP_REQUEST_CANCEL_MODE_PRESERVE,
        TCP_REQUEST_PENDING_ACCEPTED_CONNECTION,
        TCP_REQUEST_PAYLOAD_CONTEXT);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("Ran out of resources trying to enqueue a connected Context\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_MEM;
    }

    ReleaseExclusiveContextAccess(Context);
    return ERR_OK;
}

static
err_t
lwip_tcp_receive_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err
)
{
    INT CopiedLength;
    INT RemainingDestBytes;
    UCHAR *CurrentDestLocation;

    PIRP Irp;
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;
    PNDIS_BUFFER Buffer;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;

    struct pbuf *next;

    /* lwIP currently never sends anything other than ERR_OK here */

    Context = (PTCP_CONTEXT)arg;

    /* Get exclusive access to the Context */
    GetExclusiveContextAccess(Context);

    /* If the buffer is NULL, the PCB has been closed */
    if (p == NULL)
    {
        Context->lwip_tcp_pcb = NULL;
        ProcessPCBError(Context, TCP_STATE_CLOSED);
        ReleaseExclusiveContextAccess(Context);
        return ERR_OK;
    }

    /* Sanity checks */
    if (!(Context->TcpState & TCP_STATE_RECEIVING))
    {
        DPRINT1("Receive callback on Context that is not currently receiving\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }
    if (Context->lwip_tcp_pcb != tpcb)
    {
        DPRINT1("Receive PCB mismatch\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }

    /* Walk the Request list for the matching Receive request */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
        if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
        {
            /* Found a matching request. Block cancellations, dequeue,
             * and break out of the list walk. */
            Irp = Request->Payload.PendingIrp;
            IoSetCancelRoutine(Irp, NULL);
            RemoveEntryList(&Request->ListEntry);
            goto FOUND;
        }
        Entry = Entry->Flink;
    }
    DPRINT1("Failed to find a pending Receive\n");
    TCP_RMV_STATE(TCP_STATE_RECEIVING, Context);
    ReleaseExclusiveContextAccess(Context);
    return ERR_ARG;

FOUND:
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
     * Otherwise, leave the state variable alone. */
    Entry = Head->Flink;
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
    /* Clean up the Request struct and the IRP */
    Irp->IoStatus.Information = CopiedLength;
    CleanupRequest(Request, STATUS_SUCCESS, Context);

    ReleaseExclusiveContextAccess(Context);
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
    PIRP Irp;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Head;
    PTCP_CONTEXT Context;
    PTCP_REQUEST Request;

    Context = (PTCP_CONTEXT)arg;

    /* Get exclusive access to the Context */
    GetExclusiveContextAccess(Context);

    /* Sanity check */
    if (!(Context->TcpState & TCP_STATE_SENDING))
    {
        DPRINT1("Callback on a connection that is not sending anything\n");
        ReleaseExclusiveContextAccess(Context);
        return ERR_ARG;
    }

    /* Walk Request list for the first Send request */
    Head = &Context->RequestListHead;
    Entry = Head->Flink;
    while (Entry != Head)
    {
        Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);

        /* Jump to handler when Request found */
        if (Request->PendingMode == TCP_REQUEST_PENDING_SEND)
        {
            /* Immediately block any cancellations */
            Irp = Request->Payload.PendingIrp;
            IoSetCancelRoutine(Irp, NULL);

            /* Dequeue the entry and jump to handler */
            RemoveEntryList(&Request->ListEntry);
            goto FOUND;
        }

        Entry = Entry->Flink;
    }

    /* Being here means we walked the entire list without finding a Send request. We should clear
     * the TCP State variable SENDING bit. */
    DPRINT1("Callback on Context with no outstanding Send requests\n");
    TCP_RMV_STATE(TCP_STATE_SENDING, Context);
    ReleaseExclusiveContextAccess(Context);
    return ERR_ARG;

FOUND:
    /* Complete the Request */
    Irp->IoStatus.Information = len;
    CleanupRequest(Request, STATUS_SUCCESS, Context);

    ReleaseExclusiveContextAccess(Context);
    return ERR_OK;
}