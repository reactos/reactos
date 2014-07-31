/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usb_queue.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"

#define NDEBUG
#include <debug.h>

class CUSBQueue : public IEHCIQueue
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    // IUSBQueue functions
    IMP_IUSBQUEUE

    // IEHCIQueue functions
    IMP_IEHCIQUEUE

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    PKSPIN_LOCK m_Lock;                                                                  // list lock
    PDMA_ADAPTER m_Adapter;                                                             // dma adapter
    PEHCIHARDWAREDEVICE m_Hardware;                                                     // stores hardware object
    PQUEUE_HEAD AsyncListQueueHead;                                                     // async queue head
    LIST_ENTRY m_CompletedRequestAsyncList;                                             // completed async request list
    LIST_ENTRY m_PendingRequestAsyncList;                                               // pending async request list
    ULONG m_MaxPeriodicListEntries;                                                     // max perdiodic list entries
    ULONG m_MaxPollingInterval;                                                         // max polling interval
    PHYSICAL_ADDRESS m_SyncFrameListAddr;                                               // physical address of sync frame list
    PULONG m_SyncFrameList;                                                             // virtual address of sync frame list

    // queue head manipulation functions
    VOID LinkQueueHead(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    VOID UnlinkQueueHead(PQUEUE_HEAD QueueHead);
    VOID LinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    PQUEUE_HEAD UnlinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, ULONG Count);

    // processes the async list
    VOID ProcessAsyncList(IN NTSTATUS Status, OUT PULONG ShouldRingDoorBell);

    // processes the async list
    VOID ProcessPeriodicSchedule(IN NTSTATUS Status, OUT PULONG ShouldRingDoorBell);

    // called for each completed queue head
    VOID QueueHeadCompletion(PQUEUE_HEAD QueueHead, NTSTATUS Status);

    // called for each completed queue head
    VOID QueueHeadInterruptCompletion(PQUEUE_HEAD QueueHead, NTSTATUS Status);

    // called when the completion queue is cleaned up
    VOID QueueHeadCleanup(PQUEUE_HEAD QueueHead);

    // intializes the sync schedule
    NTSTATUS InitializeSyncSchedule(IN PEHCIHARDWAREDEVICE Hardware, IN PDMAMEMORYMANAGER MemManager);

    // links interrupt queue head
    VOID LinkInterruptQueueHead(PQUEUE_HEAD QueueHead);

    // interval index
    UCHAR GetIntervalIndex(UCHAR Interval);


    // interrupt queue heads
    PQUEUE_HEAD m_InterruptQueueHeads[EHCI_INTERRUPT_ENTRIES_COUNT];

    // contains the periodic queue heads
    LIST_ENTRY m_PeriodicQueueHeads;
};

//=================================================================================================
// COM
//
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::Initialize(
    IN PUSBHARDWAREDEVICE Hardware,
    IN PDMA_ADAPTER AdapterObject,
    IN PDMAMEMORYMANAGER MemManager,
    IN OPTIONAL PKSPIN_LOCK Lock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CUSBQueue::Initialize()\n");

    ASSERT(Hardware);

    //
    // store device lock
    //
    m_Lock = Lock;

    //
    // store hardware object
    //
    m_Hardware = PEHCIHARDWAREDEVICE(Hardware);


    //
    // Get the AsyncQueueHead
    //
    AsyncListQueueHead = (PQUEUE_HEAD)m_Hardware->GetAsyncListQueueHead();

    //
    // Initialize the List Head
    //
    InitializeListHead(&AsyncListQueueHead->LinkedQueueHeads);

    //
    // Initialize completed async list head
    //
    InitializeListHead(&m_CompletedRequestAsyncList);

    //
    // Initialize pending async list head
    //
    InitializeListHead(&m_PendingRequestAsyncList);

    //
    // initialize periodic queue heads
    //
    InitializeListHead(&m_PeriodicQueueHeads);

    //
    // now initialize sync schedule
    //
    Status = InitializeSyncSchedule(m_Hardware, MemManager);


    return Status;
}

NTSTATUS
CUSBQueue::InitializeSyncSchedule(
    IN PEHCIHARDWAREDEVICE Hardware,
    IN PDMAMEMORYMANAGER MemManager)
{
    PHYSICAL_ADDRESS QueueHeadPhysAddr;
    NTSTATUS Status;
    ULONG Index, Interval, IntervalIndex;
    PQUEUE_HEAD QueueHead;

    //
    // FIXME: check if smaller list sizes are supported
    //
    m_MaxPeriodicListEntries = 1024;

    //
    // use polling scheme of 512ms
    //
    m_MaxPollingInterval = 512;

    //
    // first allocate a page to hold the queue array
    //
    Status = MemManager->Allocate(m_MaxPeriodicListEntries * sizeof(PVOID), (PVOID*)&m_SyncFrameList, &m_SyncFrameListAddr);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate sync frame list array
        //
        DPRINT1("Failed to allocate sync frame list\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for(Index = 0; Index < EHCI_INTERRUPT_ENTRIES_COUNT; Index++)
    {
        //
        // allocate queue head
        //
        Status = MemManager->Allocate(sizeof(QUEUE_HEAD), (PVOID*)&QueueHead, &QueueHeadPhysAddr);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to create queue head
            //
            DPRINT1("Failed to create queue head\n");
            return Status;
        }

        //
        // initialize queue head
        //
        QueueHead->HorizontalLinkPointer = TERMINATE_POINTER;
        QueueHead->AlternateNextPointer = TERMINATE_POINTER;
        QueueHead->NextPointer = TERMINATE_POINTER;
        QueueHead->EndPointCharacteristics.MaximumPacketLength = 64;
        QueueHead->EndPointCharacteristics.NakCountReload = 0x3;
        QueueHead->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;
        QueueHead->EndPointCapabilities.NumberOfTransactionPerFrame = 0x01;
        QueueHead->PhysicalAddr = QueueHeadPhysAddr.LowPart;
        QueueHead->Token.Bits.Halted = TRUE; //FIXME
        m_InterruptQueueHeads[Index]= QueueHead;

        if (Index > 0)
        {
             // link all to the first queue head
             QueueHead->HorizontalLinkPointer = m_InterruptQueueHeads[0]->PhysicalAddr | QH_TYPE_QH;
             QueueHead->NextQueueHead = m_InterruptQueueHeads[0];
        }
    }

    //
    // build interrupt tree
    //
    Interval = EHCI_FRAMELIST_ENTRIES_COUNT;
    IntervalIndex = EHCI_INTERRUPT_ENTRIES_COUNT - 1;
    while (Interval > 1)
    {
        for (Index = Interval / 2; Index < EHCI_FRAMELIST_ENTRIES_COUNT; Index += Interval) 
        {
            DPRINT("Index %lu IntervalIndex %lu\n", Index, IntervalIndex);
            m_SyncFrameList[Index] = m_InterruptQueueHeads[IntervalIndex]->PhysicalAddr | QH_TYPE_QH;
        }
        IntervalIndex--;
        Interval /= 2;
    }

    //
    // now set the sync base
    //
    Hardware->SetPeriodicListRegister(m_SyncFrameListAddr.LowPart);

    //
    // sync frame list initialized
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::AddUSBRequest(
    IUSBRequest * Req)
{
    PQUEUE_HEAD QueueHead;
    NTSTATUS Status;
    ULONG Type;
    KIRQL OldLevel;
    PEHCIREQUEST Request;

    // sanity check
    ASSERT(Req != NULL);

    // get internal req
    Request = PEHCIREQUEST(Req);

    // get request type
    Type = Request->GetTransferType();

    // check if supported
    switch(Type)
    {
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            /* NOT IMPLEMENTED IN QUEUE */
            Status = STATUS_NOT_SUPPORTED;
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
        case USB_ENDPOINT_TYPE_BULK:
        case USB_ENDPOINT_TYPE_CONTROL:
            Status = STATUS_SUCCESS;
            break;
        default:
            /* BUG */
            PC_ASSERT(FALSE);
            Status = STATUS_NOT_SUPPORTED;
    }

    // check for success
    if (!NT_SUCCESS(Status))
    {
        // request not supported, please try later
        return Status;
    }

    // get queue head
    Status = Request->GetQueueHead(&QueueHead);

    // check for success
    if (!NT_SUCCESS(Status))
    {
        // failed to get queue head
        return Status;
    }

    // acquire lock
    KeAcquireSpinLock(m_Lock, &OldLevel);

    if (Type == USB_ENDPOINT_TYPE_BULK || Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        // Add to list
        LinkQueueHead(AsyncListQueueHead, QueueHead);
    }
    else if (Type == USB_ENDPOINT_TYPE_INTERRUPT)
    {
        // get interval
        LinkInterruptQueueHead(QueueHead);
    }

    // release lock
    KeReleaseSpinLock(m_Lock, OldLevel);


    // add extra reference which is released when the request is completed
    Request->AddRef();

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::CreateUSBRequest(
    IUSBRequest **OutRequest)
{
    PUSBREQUEST UsbRequest;
    NTSTATUS Status;

    *OutRequest = NULL;
    Status = InternalCreateUSBRequest(&UsbRequest);

    if (NT_SUCCESS(Status))
    {
        *OutRequest = UsbRequest;
    }

    return Status;
}

UCHAR
CUSBQueue::GetIntervalIndex(
    UCHAR Interval)
{
    UCHAR IntervalIndex;

    ASSERT(Interval != 0);
    if (Interval == 1)
        IntervalIndex = 1;
    else if (Interval == 2)
        IntervalIndex = 2;
    else if (Interval <= 4)
        IntervalIndex = 3;
    else if (Interval <= 8)
        IntervalIndex = 4;
    else if (Interval <= 16)
        IntervalIndex = 5;
    else if (Interval <= 32)
        IntervalIndex = 6;
    else if (Interval <= 64)
        IntervalIndex = 7;
    else if (Interval <= 128)
        IntervalIndex = 8;
    else
        IntervalIndex = 9;

    ASSERT(IntervalIndex < EHCI_INTERRUPT_ENTRIES_COUNT);
    return IntervalIndex;
}

VOID
CUSBQueue::LinkInterruptQueueHead(
    PQUEUE_HEAD QueueHead)
{
    PEHCIREQUEST Request;
    UCHAR Interval, IntervalIndex;
    USB_DEVICE_SPEED DeviceSpeed;
    PQUEUE_HEAD InterruptQueueHead;

    // get internal req
    Request = PEHCIREQUEST(QueueHead->Request);
    ASSERT(Request);

    // get interval
    Interval = Request->GetInterval();

    // get device speed
    DeviceSpeed = Request->GetSpeed();
    if (DeviceSpeed == UsbHighSpeed)
    {
        // interrupt queue head can be scheduled on each possible micro frame
        QueueHead->EndPointCapabilities.InterruptScheduleMask = 0xFF;
    }
    else
    {
        // As we do not yet support FSTNs to correctly reference low/full
        // speed interrupt transfers, we simply put them into the 1 interval
        // queue. This way we ensure that we reach them on every micro frame
        // and can do the corresponding start/complete split transactions.
        // ToDo: use FSTNs to correctly link non high speed interrupt transfers
        Interval = 1;

        // For now we also force start splits to be in micro frame 0 and
        // complete splits to be in micro frame 2, 3 and 4.
        QueueHead->EndPointCapabilities.InterruptScheduleMask = 0x01;
        QueueHead->EndPointCapabilities.SplitCompletionMask = 0x1C;
    }

    // sanitize interrupt interval
    Interval = max(1, Interval);

    // get interval index
    IntervalIndex = GetIntervalIndex(Interval);


    // get interrupt queue head
    InterruptQueueHead = m_InterruptQueueHeads[IntervalIndex];

    // link queue head
    QueueHead->HorizontalLinkPointer = InterruptQueueHead->HorizontalLinkPointer;
    QueueHead->NextQueueHead = InterruptQueueHead->NextQueueHead;

    InterruptQueueHead->HorizontalLinkPointer = QueueHead->PhysicalAddr | QH_TYPE_QH;
    InterruptQueueHead->NextQueueHead = QueueHead;

    // store in periodic list
    InsertTailList(&m_PeriodicQueueHeads, &QueueHead->LinkedQueueHeads);
}

//
// LinkQueueHead - Links one QueueHead to the end of HeadQueueHead list, updating HorizontalLinkPointer.
//
VOID
CUSBQueue::LinkQueueHead(
    PQUEUE_HEAD HeadQueueHead,
    PQUEUE_HEAD NewQueueHead)
{
    PQUEUE_HEAD LastQueueHead, NextQueueHead;
    PLIST_ENTRY Entry;
    ASSERT(HeadQueueHead);
    ASSERT(NewQueueHead);

    //
    // Link the LIST_ENTRYs
    //
    //ASSERT(IsListEmpty(&HeadQueueHead->LinkedQueueHeads));
    InsertTailList(&HeadQueueHead->LinkedQueueHeads, &NewQueueHead->LinkedQueueHeads);

    //
    // Update HLP for NewQueueHead to point to next, which should be the HeadQueueHead
    //
    Entry = NewQueueHead->LinkedQueueHeads.Flink;
    NextQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    //ASSERT(NextQueueHead == HeadQueueHead);
    NewQueueHead->HorizontalLinkPointer = (NextQueueHead->PhysicalAddr | QH_TYPE_QH);

    _ReadWriteBarrier();

    //
    // Update HLP for Previous QueueHead, which should be the last in list.
    //
    Entry = NewQueueHead->LinkedQueueHeads.Blink;
    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    //ASSERT(LastQueueHead == HeadQueueHead);
    LastQueueHead->HorizontalLinkPointer = (NewQueueHead->PhysicalAddr | QH_TYPE_QH);

    //
    // head queue head must be halted
    //
    //PC_ASSERT(HeadQueueHead->Token.Bits.Halted == TRUE);
}

//
// UnlinkQueueHead - Unlinks one QueueHead, updating HorizontalLinkPointer.
//
VOID
CUSBQueue::UnlinkQueueHead(
    PQUEUE_HEAD QueueHead)
{
    PQUEUE_HEAD PreviousQH, NextQH;
    PLIST_ENTRY Entry;

    //
    // sanity check: there must be at least one queue head with halted bit set
    //
    //PC_ASSERT(QueueHead->Token.Bits.Halted == 0);

    //
    // get previous link
    //
    Entry = QueueHead->LinkedQueueHeads.Blink;

    //
    // get queue head structure
    //
    PreviousQH = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

    //
    // get next link
    //
    Entry = QueueHead->LinkedQueueHeads.Flink;

    //
    // get queue head structure
    //
    NextQH = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

    //
    // sanity check
    //
    ASSERT(QueueHead->HorizontalLinkPointer == (NextQH->PhysicalAddr | QH_TYPE_QH));

    //
    // remove queue head from linked list
    //
    PreviousQH->HorizontalLinkPointer = NextQH->PhysicalAddr | QH_TYPE_QH;

    //
    // remove software link
    //
    RemoveEntryList(&QueueHead->LinkedQueueHeads);
}

//
// LinkQueueHeadChain - Links a list of QueueHeads to the HeadQueueHead list, updating HorizontalLinkPointer.
//
VOID
CUSBQueue::LinkQueueHeadChain(
    PQUEUE_HEAD HeadQueueHead,
    PQUEUE_HEAD NewQueueHead)
{
    PQUEUE_HEAD LastQueueHead;
    PLIST_ENTRY Entry;
    ASSERT(HeadQueueHead);
    ASSERT(NewQueueHead);

    //
    // Find the last QueueHead in NewQueueHead
    //
    Entry = NewQueueHead->LinkedQueueHeads.Blink;
    ASSERT(Entry != NewQueueHead->LinkedQueueHeads.Flink);
    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

    //
    // Set the LinkPointer and Flink
    //
    LastQueueHead->HorizontalLinkPointer = HeadQueueHead->PhysicalAddr | QH_TYPE_QH;
    LastQueueHead->LinkedQueueHeads.Flink = &HeadQueueHead->LinkedQueueHeads;

    //
    // Fine the last QueueHead in HeadQueueHead
    //
    Entry = HeadQueueHead->LinkedQueueHeads.Blink;
    HeadQueueHead->LinkedQueueHeads.Blink = &LastQueueHead->LinkedQueueHeads;
    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    LastQueueHead->LinkedQueueHeads.Flink = &NewQueueHead->LinkedQueueHeads;
    LastQueueHead->HorizontalLinkPointer = NewQueueHead->PhysicalAddr | QH_TYPE_QH;
}

//
// UnlinkQueueHeadChain - Unlinks a list number of QueueHeads from HeadQueueHead list, updating HorizontalLinkPointer.
// returns the chain of QueueHeads removed from HeadQueueHead.
//
PQUEUE_HEAD
CUSBQueue::UnlinkQueueHeadChain(
    PQUEUE_HEAD HeadQueueHead,
    ULONG Count)
{
    PQUEUE_HEAD LastQueueHead, FirstQueueHead;
    PLIST_ENTRY Entry;
    ULONG Index;

    //
    // Find the last QueueHead in NewQueueHead
    //
    Entry = &HeadQueueHead->LinkedQueueHeads;
    FirstQueueHead = CONTAINING_RECORD(Entry->Flink, QUEUE_HEAD, LinkedQueueHeads);

    for (Index = 0; Index < Count; Index++)
    {
        Entry = Entry->Flink;

        if (Entry == &HeadQueueHead->LinkedQueueHeads)
        {
            DPRINT1("Warning; Only %lu QueueHeads in HeadQueueHead\n", Index);
            Count = Index + 1;
            break;
        }
    }

    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    HeadQueueHead->LinkedQueueHeads.Flink = LastQueueHead->LinkedQueueHeads.Flink;
    if (Count + 1 == Index)
    {
        HeadQueueHead->LinkedQueueHeads.Blink = &HeadQueueHead->LinkedQueueHeads;
    }
    else
        HeadQueueHead->LinkedQueueHeads.Blink = LastQueueHead->LinkedQueueHeads.Flink;

    FirstQueueHead->LinkedQueueHeads.Blink = &LastQueueHead->LinkedQueueHeads;
    LastQueueHead->LinkedQueueHeads.Flink = &FirstQueueHead->LinkedQueueHeads;
    LastQueueHead->HorizontalLinkPointer = TERMINATE_POINTER;
    return FirstQueueHead;
}

VOID
CUSBQueue::QueueHeadInterruptCompletion(
    PQUEUE_HEAD QueueHead,
    NTSTATUS Status)
{
    PEHCIREQUEST Request;
    UCHAR Interval, IntervalIndex;
    PQUEUE_HEAD InterruptQueueHead, LastQueueHead = NULL;


    //
    // sanity check
    //
    PC_ASSERT(QueueHead->Request);

    //
    // get IUSBRequest interface
    //
    Request = (PEHCIREQUEST)QueueHead->Request;

    // get interval
    Interval = Request->GetInterval();

    // sanitize interval
    Interval = max(1, Interval);

    // get interval index
    IntervalIndex = GetIntervalIndex(Interval);

    // get interrupt queue head from index
    InterruptQueueHead = m_InterruptQueueHeads[IntervalIndex];

    while(InterruptQueueHead != NULL)
    {
        if (InterruptQueueHead == QueueHead)
            break;

        // move to next queue head
        LastQueueHead = InterruptQueueHead;
        InterruptQueueHead = (PQUEUE_HEAD)InterruptQueueHead->NextQueueHead;
    }

    if (InterruptQueueHead != QueueHead)
    {
        // queue head not in list
        ASSERT(FALSE);
        return;
    }

    // now unlink queue head
    LastQueueHead->HorizontalLinkPointer = QueueHead->HorizontalLinkPointer;
    LastQueueHead->NextQueueHead = QueueHead->NextQueueHead;

    DPRINT1("Periodic QueueHead %p Addr %x unlinked\n", QueueHead, QueueHead->PhysicalAddr);

    // insert into completed list
    InsertTailList(&m_CompletedRequestAsyncList, &QueueHead->LinkedQueueHeads);
}



VOID
CUSBQueue::QueueHeadCompletion(
    PQUEUE_HEAD CurrentQH,
    NTSTATUS Status)
{
    //
    // now unlink the queue head
    // FIXME: implement chained queue heads
    // no need to acquire locks, as it is called with locks held
    //

    //
    // unlink queue head
    //
    UnlinkQueueHead(CurrentQH);

    //
    // insert into completed list
    //
    InsertTailList(&m_CompletedRequestAsyncList, &CurrentQH->LinkedQueueHeads);
}


VOID
CUSBQueue::ProcessPeriodicSchedule(
    IN NTSTATUS Status,
    OUT PULONG ShouldRingDoorBell)
{
    KIRQL OldLevel;
    PLIST_ENTRY Entry;
    PQUEUE_HEAD QueueHead;
    PEHCIREQUEST Request;
    BOOLEAN IsQueueHeadComplete;

    //
    // lock completed async list
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    //
    // walk async list 
    //
    ASSERT(AsyncListQueueHead);
    Entry = m_PeriodicQueueHeads.Flink;

    while(Entry != &m_PeriodicQueueHeads)
    {
        //
        // get queue head structure
        //
        QueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

        //
        // sanity check
        //
        PC_ASSERT(QueueHead->Request);

        //
        // get IUSBRequest interface
        //
        Request = (PEHCIREQUEST)QueueHead->Request;

        //
        // move to next entry
        //
        Entry = Entry->Flink;

        //
        // check if queue head is complete
        //
        IsQueueHeadComplete = Request->IsQueueHeadComplete(QueueHead);

        DPRINT("Request %p QueueHead %p Complete %c\n", Request, QueueHead, IsQueueHeadComplete);

        //
        // check if queue head is complete
        //
        if (IsQueueHeadComplete)
        {
            //
            // current queue head is complete
            //
            QueueHeadInterruptCompletion(QueueHead, Status);

            //
            // ring door bell is going to be necessary
            //
            *ShouldRingDoorBell = TRUE;
        }
    }

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);

}

VOID
CUSBQueue::ProcessAsyncList(
    IN NTSTATUS Status,
    OUT PULONG ShouldRingDoorBell)
{
    KIRQL OldLevel;
    PLIST_ENTRY Entry;
    PQUEUE_HEAD QueueHead;
    PEHCIREQUEST Request;
    BOOLEAN IsQueueHeadComplete;

    //
    // lock completed async list
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    //
    // walk async list 
    //
    ASSERT(AsyncListQueueHead);
    Entry = AsyncListQueueHead->LinkedQueueHeads.Flink;

    while(Entry != &AsyncListQueueHead->LinkedQueueHeads)
    {
        //
        // get queue head structure
        //
        QueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

        //
        // sanity check
        //
        PC_ASSERT(QueueHead->Request);

        //
        // get IUSBRequest interface
        //
        Request = (PEHCIREQUEST)QueueHead->Request;

        //
        // move to next entry
        //
        Entry = Entry->Flink;

        //
        // check if queue head is complete
        //
        IsQueueHeadComplete = Request->IsQueueHeadComplete(QueueHead);

        DPRINT("Request %p QueueHead %p Complete %c\n", Request, QueueHead, IsQueueHeadComplete);

        //
        // check if queue head is complete
        //
        if (IsQueueHeadComplete)
        {
            //
            // current queue head is complete
            //
            QueueHeadCompletion(QueueHead, Status);

            //
            // ring door bell is going to be necessary
            //
            *ShouldRingDoorBell = TRUE;
        }
    }

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);
}


VOID
STDMETHODCALLTYPE
CUSBQueue::InterruptCallback(
    IN NTSTATUS Status, 
    OUT PULONG ShouldRingDoorBell)
{
    DPRINT("CUSBQueue::InterruptCallback\n");

    //
    // process periodic schedule
    //
    ProcessPeriodicSchedule(Status, ShouldRingDoorBell);

    //
    // iterate asynchronous list
    //
    *ShouldRingDoorBell = FALSE;
    ProcessAsyncList(Status, ShouldRingDoorBell);
}

VOID
CUSBQueue::QueueHeadCleanup(
    PQUEUE_HEAD CurrentQH)
{
    PQUEUE_HEAD NewQueueHead;
    PEHCIREQUEST Request;
    BOOLEAN ShouldReleaseWhenDone;
    USBD_STATUS UrbStatus;
    KIRQL OldLevel;

    //
    // sanity checks
    //
    PC_ASSERT(CurrentQH->Token.Bits.Active == 0);
    PC_ASSERT(CurrentQH->Request);


    //
    // get request
    //
    Request = (PEHCIREQUEST)CurrentQH->Request;

    //
    // sanity check
    //
    PC_ASSERT(Request);

    //
    // check if the queue head was completed with errors
    //
    if (CurrentQH->Token.Bits.Halted)
    {
        if (CurrentQH->Token.Bits.DataBufferError)
        {
            //
            // data buffer error
            //
            UrbStatus = USBD_STATUS_DATA_BUFFER_ERROR;
        }
        else if (CurrentQH->Token.Bits.BabbleDetected)
        {
            //
            // babble detected
            //
            UrbStatus = USBD_STATUS_BABBLE_DETECTED;
        }
        else
        {
            //
            // stall pid
            //
            UrbStatus = USBD_STATUS_STALL_PID;
        }
    }
    else
    {
        //
        // well done ;)
        //
        UrbStatus = USBD_STATUS_SUCCESS;
    }

    //
    // Check if the transfer was completed and if UrbStatus is ok
    //
    if ((Request->IsRequestComplete() == FALSE) && (UrbStatus == USBD_STATUS_SUCCESS))
    {
        //
        // request is incomplete, get new queue head
        //
        if (Request->GetQueueHead(&NewQueueHead) == STATUS_SUCCESS)
        {
            //
            // let IUSBRequest free the queue head
            //
            Request->FreeQueueHead(CurrentQH);

            //
            // first acquire request lock
            //
            KeAcquireSpinLock(m_Lock, &OldLevel);

            //
            // add to pending list
            //
            InsertTailList(&m_PendingRequestAsyncList, &NewQueueHead->LinkedQueueHeads);

            //
            // release queue head
            //
            KeReleaseSpinLock(m_Lock, OldLevel);

            //
            // Done for now
            //
            return;
        }
        DPRINT1("Unable to create a new QueueHead\n");
        //ASSERT(FALSE);

        //
        // Else there was a problem
        // FIXME: Find better return
        UrbStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

    if (UrbStatus != USBD_STATUS_SUCCESS)
    {
        DPRINT1("URB failed with status 0x%x\n", UrbStatus);
        //PC_ASSERT(FALSE);
    }

    //
    // notify request that a transfer has completed
    //
    Request->CompletionCallback(UrbStatus != USBD_STATUS_SUCCESS ?  STATUS_UNSUCCESSFUL : STATUS_SUCCESS,
                                UrbStatus,
                                CurrentQH);

    //
    // let IUSBRequest free the queue head
    //
    Request->FreeQueueHead(CurrentQH);

    //
    // check if we should release request when done
    //
    ShouldReleaseWhenDone = Request->ShouldReleaseRequestAfterCompletion();

    //
    // release reference when the request was added
    //
    Request->Release();

    //
    // check if the operation was asynchronous
    //
    if (ShouldReleaseWhenDone)
    {
        //
        // release outstanding reference count
        //
        Request->Release();
    }

    //
    // request is now released
    //
}

VOID
STDMETHODCALLTYPE
CUSBQueue::CompleteAsyncRequests()
{
    KIRQL OldLevel;
    PLIST_ENTRY Entry;
    PQUEUE_HEAD CurrentQH;

    DPRINT("CUSBQueue::CompleteAsyncRequests\n");

    //
    // first acquire request lock
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    //
    // the list should not be empty
    //
    PC_ASSERT(!IsListEmpty(&m_CompletedRequestAsyncList));

    while(!IsListEmpty(&m_CompletedRequestAsyncList))
    {
        //
        // remove first entry
        //
        Entry = RemoveHeadList(&m_CompletedRequestAsyncList);

        //
        // get queue head structure
        //
        CurrentQH = (PQUEUE_HEAD)CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

        //
        // release lock
        //
        KeReleaseSpinLock(m_Lock, OldLevel);

        //
        // complete request now
        //
        QueueHeadCleanup(CurrentQH);

        //
        // first acquire request lock
        //
        KeAcquireSpinLock(m_Lock, &OldLevel);
    }

    //
    // is there a pending async entry
    //
    if (!IsListEmpty(&m_PendingRequestAsyncList))
    {
        //
        // remove first entry
        //
        Entry = RemoveHeadList(&m_PendingRequestAsyncList);

        //
        // get queue head structure
        //
        CurrentQH = (PQUEUE_HEAD)CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);

        //
        // Add it to the AsyncList list
        //
        LinkQueueHead(AsyncListQueueHead, CurrentQH);
    }

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::AbortDevicePipe(
    IN UCHAR DeviceAddress,
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
    KIRQL OldLevel;
    PLIST_ENTRY Entry;
    PQUEUE_HEAD QueueHead;
    LIST_ENTRY ListHead;

    //
    // lock completed async list
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    DPRINT1("AbortDevicePipe DeviceAddress %x EndpointDescriptor %p Addr %x\n", DeviceAddress, EndpointDescriptor, EndpointDescriptor->bEndpointAddress);

    //
    // init list head	
    //
    InitializeListHead(&ListHead);


    //
    // walk async list 
    //
    ASSERT(AsyncListQueueHead);
    Entry = AsyncListQueueHead->LinkedQueueHeads.Flink;

    while(Entry != &AsyncListQueueHead->LinkedQueueHeads)
    {
        //
        // get queue head structure
        //
        QueueHead = (PQUEUE_HEAD)CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
        ASSERT(QueueHead);

        //
        // move to next entry
        //
        Entry = Entry->Flink;

        if (QueueHead->EndPointCharacteristics.DeviceAddress == DeviceAddress &&
            QueueHead->EndPointCharacteristics.EndPointNumber == (EndpointDescriptor->bEndpointAddress & 0xF) && QueueHead->Token.Bits.Halted)
        {
            //
            // unlink queue head
            //
            UnlinkQueueHead(QueueHead);

            //
            // add to temp list
            //
            InsertTailList(&ListHead, &QueueHead->LinkedQueueHeads);
        }
    }

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);

    while(!IsListEmpty(&ListHead))
    {
        //
        // remove entry
        //
        Entry = RemoveHeadList(&ListHead);

        //
        // get queue head structure
        //
        QueueHead = (PQUEUE_HEAD)CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
        ASSERT(QueueHead);

        //
        // cleanup queue head
        //
        QueueHeadCleanup(QueueHead);
    }
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CreateUSBQueue(
    PUSBQUEUE *OutUsbQueue)
{
    PUSBQUEUE This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBEHCI) CUSBQueue(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutUsbQueue = (PUSBQUEUE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

