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
#include "hardware.h"

class CUSBQueue : public IUSBQueue
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

    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware, PDMA_ADAPTER AdapterObject, IN PDMAMEMORYMANAGER MemManager, IN OPTIONAL PKSPIN_LOCK Lock);
    virtual ULONG GetPendingRequestCount();
    virtual NTSTATUS AddUSBRequest(PURB Urb);
    virtual NTSTATUS AddUSBRequest(IUSBRequest * Request);
    virtual NTSTATUS CancelRequests();
    virtual NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest);
    virtual VOID InterruptCallback(IN NTSTATUS Status, OUT PULONG ShouldRingDoorBell);
    virtual VOID CompleteAsyncRequests();
    virtual NTSTATUS AbortDevicePipe(UCHAR DeviceAddress, IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor);


    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    PKSPIN_LOCK m_Lock;                                                                  // list lock
    PDMA_ADAPTER m_Adapter;                                                             // dma adapter
    PUSBHARDWAREDEVICE m_Hardware;                                                      // stores hardware object
    PQUEUE_HEAD AsyncListQueueHead;                                                     // async queue head
    LIST_ENTRY m_CompletedRequestAsyncList;                                             // completed async request list
    LIST_ENTRY m_PendingRequestAsyncList;                                               // pending async request list
    ULONG m_MaxPeriodicListEntries;                                                     // max perdiodic list entries
    ULONG m_MaxPollingInterval;                                                         // max polling interval
    PHYSICAL_ADDRESS m_SyncFrameListAddr;                                               // physical address of sync frame list
    PULONG m_SyncFrameList;                                                             // virtual address of sync frame list
    PQUEUE_HEAD * m_SyncFrameListQueueHeads;                                            // stores the frame list of queue head

    // queue head manipulation functions
    VOID LinkQueueHead(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    VOID UnlinkQueueHead(PQUEUE_HEAD QueueHead);
    VOID LinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    PQUEUE_HEAD UnlinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, ULONG Count);

    // processes the async list
    VOID ProcessAsyncList(IN NTSTATUS Status, OUT PULONG ShouldRingDoorBell);

    // called for each completed queue head
    VOID QueueHeadCompletion(PQUEUE_HEAD QueueHead, NTSTATUS Status);

    // called when the completion queue is cleaned up
    VOID QueueHeadCleanup(PQUEUE_HEAD QueueHead);

    // intializes the sync schedule
    NTSTATUS InitializeSyncSchedule(IN PUSBHARDWAREDEVICE Hardware, IN PDMAMEMORYMANAGER MemManager);
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
    // initialize device lock
    //
    m_Lock = Lock;

    //
    // Get the AsyncQueueHead
    //
    AsyncListQueueHead = (PQUEUE_HEAD)Hardware->GetAsyncListQueueHead();

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
    // now initialize sync schedule
    //
    Status = InitializeSyncSchedule(Hardware, MemManager);

    //
    // store hardware object
    //
    m_Hardware = Hardware;

    return Status;
}

NTSTATUS
CUSBQueue::InitializeSyncSchedule(
    IN PUSBHARDWAREDEVICE Hardware,
    IN PDMAMEMORYMANAGER MemManager)
{
    PHYSICAL_ADDRESS QueueHeadPhysAddr;
    NTSTATUS Status;
    ULONG Index;
    PQUEUE_HEAD QueueHead;

    //
    // FIXME: check if smaller list sizes are supported
    //
    m_MaxPeriodicListEntries = 1024;

    //
    // use polling scheme of 32ms
    //
    m_MaxPollingInterval = 32;

    //
    // allocate dummy frame list array
    //
    m_SyncFrameListQueueHeads = (PQUEUE_HEAD*)ExAllocatePool(NonPagedPool, m_MaxPollingInterval * sizeof(PQUEUE_HEAD));
    if (!m_SyncFrameListQueueHeads)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

  
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
        ExFreePool(m_SyncFrameListQueueHeads);
        //ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now allocate queue head descriptors for the polling interval
    //
    for(Index = 0; Index < m_MaxPeriodicListEntries; Index++)
    {
        //
        // check if is inside our polling interrupt frequency window
        //
        if (Index < m_MaxPollingInterval)
        {
            //
            // allocate queue head
            //
            Status = MemManager->Allocate(sizeof(QUEUE_HEAD), (PVOID*)&QueueHead, &QueueHeadPhysAddr);

            //
            // initialize queue head
            //
            QueueHead->HorizontalLinkPointer = TERMINATE_POINTER;
            QueueHead->AlternateNextPointer = TERMINATE_POINTER;
            QueueHead->NextPointer = TERMINATE_POINTER;

            //
            // 1 for non high speed, 0 for high speed device
            //
            QueueHead->EndPointCharacteristics.ControlEndPointFlag = 0;
            QueueHead->EndPointCharacteristics.HeadOfReclamation = FALSE;
            QueueHead->EndPointCharacteristics.MaximumPacketLength = 64;

            //
            // Set NakCountReload to max value possible
            //
            QueueHead->EndPointCharacteristics.NakCountReload = 0xF;

            //
            // Get the Initial Data Toggle from the QEDT
            //
            QueueHead->EndPointCharacteristics.QEDTDataToggleControl = FALSE;

            //
            // FIXME: check if High Speed Device
            //
            QueueHead->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;
            QueueHead->EndPointCapabilities.NumberOfTransactionPerFrame = 0x03;
            QueueHead->Token.DWord = 0;
            QueueHead->Token.Bits.InterruptOnComplete = FALSE;
            QueueHead->PhysicalAddr = QueueHeadPhysAddr.LowPart;


            //
            // store in queue head array
            //
            m_SyncFrameListQueueHeads[Index] = QueueHead;
        }
        else
        {
            //
            // get cached entry
            //
            QueueHead = m_SyncFrameListQueueHeads[m_MaxPeriodicListEntries % m_MaxPollingInterval];
        }

        //
        // store entry
        //
        m_SyncFrameList[Index] = (QueueHead->PhysicalAddr | 0x2);
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

ULONG
CUSBQueue::GetPendingRequestCount()
{
    //
    // Loop through the pending list and iterrate one for each QueueHead that
    // has a IRP to complete.
    //

    return 0;
}

NTSTATUS
CUSBQueue::AddUSBRequest(
    IUSBRequest * Request)
{
    PQUEUE_HEAD QueueHead;
    NTSTATUS Status;
    ULONG Type;
    KIRQL OldLevel;

    //
    // sanity check
    //
    ASSERT(Request != NULL);

    //
    // get request type
    //
    Type = Request->GetTransferType();

    //
    // check if supported
    //
    switch(Type)
    {
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
        case USB_ENDPOINT_TYPE_INTERRUPT:
            /* NOT IMPLEMENTED IN QUEUE */
            Status = STATUS_NOT_SUPPORTED;
            break;
        case USB_ENDPOINT_TYPE_BULK:
        case USB_ENDPOINT_TYPE_CONTROL:
            Status = STATUS_SUCCESS;
            break;
        default:
            /* BUG */
            PC_ASSERT(FALSE);
            Status = STATUS_NOT_SUPPORTED;
    }

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // request not supported, please try later
        //
        return Status;
    }

    if (Type == USB_ENDPOINT_TYPE_BULK || Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // get queue head
        //
        Status = Request->GetQueueHead(&QueueHead);

        //
        // check for success
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to get queue head
            //
           return Status;
        }

        DPRINT("Request %p QueueHead %p inserted into AsyncQueue\n", Request, QueueHead);

        //
        // Add it to the pending list
        //
        KeAcquireSpinLock(m_Lock, &OldLevel);
        LinkQueueHead(AsyncListQueueHead, QueueHead);
        KeReleaseSpinLock(m_Lock, OldLevel);

    }


    //
    // add extra reference which is released when the request is completed
    //
    Request->AddRef();


    return STATUS_SUCCESS;
}

NTSTATUS
CUSBQueue::AddUSBRequest(
    PURB Urb)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBQueue::CancelRequests()
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
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
    // Update HLP for Previous QueueHead, which should be the last in list.
    //
    Entry = NewQueueHead->LinkedQueueHeads.Blink;
    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    //ASSERT(LastQueueHead == HeadQueueHead);
    LastQueueHead->HorizontalLinkPointer = (NewQueueHead->PhysicalAddr | QH_TYPE_QH);

    //
    // Update HLP for NewQueueHead to point to next, which should be the HeadQueueHead
    //
    Entry = NewQueueHead->LinkedQueueHeads.Flink;
    NextQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    //ASSERT(NextQueueHead == HeadQueueHead);
    NewQueueHead->HorizontalLinkPointer = (NextQueueHead->PhysicalAddr | QH_TYPE_QH);

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
            DPRINT1("Warnnig; Only %d QueueHeads in HeadQueueHead\n", Index);
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
CUSBQueue::ProcessAsyncList(
    IN NTSTATUS Status,
    OUT PULONG ShouldRingDoorBell)
{
    KIRQL OldLevel;
    PLIST_ENTRY Entry;
    PQUEUE_HEAD QueueHead;
    IUSBRequest * Request;
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
        QueueHead = (PQUEUE_HEAD)CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
        ASSERT(QueueHead);

        //
        // sanity check
        //
        PC_ASSERT(QueueHead->Request);

        //
        // get IUSBRequest interface
        //
        Request = (IUSBRequest*)QueueHead->Request;

        //
        // move to next entry
        //
        Entry = Entry->Flink;

        //
        // check if queue head is complete
        //
        IsQueueHeadComplete = Request->IsQueueHeadComplete(QueueHead);

        DPRINT("Request %p QueueHead %p Complete %d\n", Request, QueueHead, IsQueueHeadComplete);

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
CUSBQueue::InterruptCallback(
    IN NTSTATUS Status, 
    OUT PULONG ShouldRingDoorBell)
{

    DPRINT("CUSBQueue::InterruptCallback\n");

    //
    // iterate asynchronous list
    //
    *ShouldRingDoorBell = FALSE;
    ProcessAsyncList(Status, ShouldRingDoorBell);

    //
    // TODO: implement periodic schedule processing
    //
}

VOID
CUSBQueue::QueueHeadCleanup(
    PQUEUE_HEAD CurrentQH)
{
    PQUEUE_HEAD NewQueueHead;
    IUSBRequest * Request;
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
    Request = (IUSBRequest*)CurrentQH->Request;

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

