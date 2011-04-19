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

    NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware, PDMA_ADAPTER AdapterObject, IN OPTIONAL PKSPIN_LOCK Lock);
    ULONG GetPendingRequestCount();
    NTSTATUS AddUSBRequest(PURB Urb);
    NTSTATUS AddUSBRequest(IUSBRequest * Request);
    NTSTATUS CancelRequests();
    NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest);

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;
    KSPIN_LOCK m_Lock;
    PDMA_ADAPTER m_Adapter;
    PQUEUE_HEAD AsyncQueueHead;
    PQUEUE_HEAD PendingQueueHead;

    VOID LinkQueueHead(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    VOID UnlinkQueueHead(PQUEUE_HEAD QueueHead);
    VOID LinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, PQUEUE_HEAD NewQueueHead);
    PQUEUE_HEAD UnlinkQueueHeadChain(PQUEUE_HEAD HeadQueueHead, ULONG Count);
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
    PDMA_ADAPTER AdapterObject,
    IN OPTIONAL PKSPIN_LOCK Lock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT1("CUSBQueue::Initialize()\n");

    ASSERT(Hardware);

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // FIXME: Need to set AsyncRegister with a QUEUEHEAD
    //
    return Status;
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
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
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
    InsertTailList(&HeadQueueHead->LinkedQueueHeads, &NewQueueHead->LinkedQueueHeads);

    //
    // Update HLP for Previous QueueHead, which should be the last in list.
    //
    Entry = NewQueueHead->LinkedQueueHeads.Blink;
    LastQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    LastQueueHead->HorizontalLinkPointer = (NewQueueHead->PhysicalAddr | QH_TYPE_QH);

    //
    // Update HLP for NewQueueHead to point to next, which should be the HeadQueueHead
    //
    Entry = NewQueueHead->LinkedQueueHeads.Flink;
    NextQueueHead = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    ASSERT(NextQueueHead == HeadQueueHead);
    NewQueueHead->HorizontalLinkPointer = NextQueueHead->PhysicalAddr;
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

    Entry = QueueHead->LinkedQueueHeads.Blink;
    PreviousQH = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    Entry = QueueHead->LinkedQueueHeads.Flink;
    NextQH = CONTAINING_RECORD(Entry, QUEUE_HEAD, LinkedQueueHeads);
    ASSERT(QueueHead->HorizontalLinkPointer == (NextQH->PhysicalAddr | QH_TYPE_QH));
    PreviousQH->HorizontalLinkPointer = NextQH->PhysicalAddr | QH_TYPE_QH;

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
