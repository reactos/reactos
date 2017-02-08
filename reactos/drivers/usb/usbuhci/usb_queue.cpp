/*
 * PROJECT:     ReactOS Universal Serial Bus Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbuhci/usb_queue.cpp
 * PURPOSE:     USB UHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbuhci.h"

#define NDEBUG
#include <debug.h>

class CUSBQueue : public IUHCIQueue
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

    // com
    IMP_IUSBQUEUE
    IMP_IUHCIQUEUE

    // local
    VOID LinkQueueHead(PUHCI_QUEUE_HEAD QueueHead, PUHCI_QUEUE_HEAD NextQueueHead);
    VOID UnLinkQueueHead(PUHCI_QUEUE_HEAD PreviousQueueHead, PUHCI_QUEUE_HEAD NextQueueHead);
    BOOLEAN IsQueueHeadComplete(PUHCI_QUEUE_HEAD QueueHead);
    NTSTATUS AddQueueHead(PUHCI_QUEUE_HEAD NewQueueHead);
    VOID QueueHeadCleanup(IN PUHCI_QUEUE_HEAD QueueHead, IN PUHCI_QUEUE_HEAD PreviousQueueHead, OUT PUHCI_QUEUE_HEAD *NextQueueHead);


    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    KSPIN_LOCK m_Lock;                                                                  // list lock
    PUHCIHARDWAREDEVICE m_Hardware;                                                     // hardware
    
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

    //
    // store hardware
    //
    m_Hardware = PUHCIHARDWAREDEVICE(Hardware);

    //
    // initialize spinlock
    //
    KeInitializeSpinLock(&m_Lock);
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBQueue::AddQueueHead(
    PUHCI_QUEUE_HEAD NewQueueHead)
{
    PUHCIREQUEST Request;
    PUHCI_QUEUE_HEAD QueueHead = NULL;


    //
    // get request
    //
    Request = (PUHCIREQUEST)NewQueueHead->Request;
    if (!Request)
    {
        //
        // no request
        //
        return STATUS_INVALID_PARAMETER;
    }

    if (Request->GetTransferType() == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // get device speed
        //
        if (Request->GetDeviceSpeed() == UsbLowSpeed)
        {
            //
            // use low speed queue
            //
            m_Hardware->GetQueueHead(UHCI_LOW_SPEED_CONTROL_QUEUE, &QueueHead);
        }
        else
        {
            //
            // use full speed queue
            //
            m_Hardware->GetQueueHead(UHCI_FULL_SPEED_CONTROL_QUEUE, &QueueHead);
        }
    }
    else if (Request->GetTransferType() == USB_ENDPOINT_TYPE_BULK)
    {
         //
         // use full speed queue
         //
         m_Hardware->GetQueueHead(UHCI_BULK_QUEUE, &QueueHead);
    }
    else if (Request->GetTransferType() == USB_ENDPOINT_TYPE_INTERRUPT)
    {
         //
         // use full speed queue
         //
         m_Hardware->GetQueueHead(UHCI_INTERRUPT_QUEUE, &QueueHead);
    }

    //
    // FIXME support isochronous
    //
    ASSERT(QueueHead);

    //
    // add reference
    //
    Request->AddRef();

    //
    // now link the new queue head
    //
    LinkQueueHead(QueueHead, NewQueueHead);
    return STATUS_SUCCESS;

}

NTSTATUS
CUSBQueue::AddUSBRequest(
    IUSBRequest * Req)
{
    PUHCI_QUEUE_HEAD NewQueueHead;
    NTSTATUS Status;
    PUHCIREQUEST Request;

    // get request
    Request = (PUHCIREQUEST)Req;

    //
    // get queue head
    //
    Status = Request->GetEndpointDescriptor(&NewQueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create queue head
        //
        DPRINT1("[USBUHCI] Failed to create queue head %x\n", Status);
        return Status;
    }

    //
    // sanity check
    //
    ASSERT(PVOID(Request) == NewQueueHead->Request);

    //
    // add queue head
    //
    DPRINT("AddUSBRequest Request %p\n", Request);
    DPRINT("NewQueueHead %p\n", NewQueueHead);
    return AddQueueHead(NewQueueHead);
}

VOID
CUSBQueue::LinkQueueHead(
    IN PUHCI_QUEUE_HEAD QueueHead,
    IN PUHCI_QUEUE_HEAD NextQueueHead)
{
    NextQueueHead->LinkPhysical = QueueHead->LinkPhysical;
    NextQueueHead->NextLogicalDescriptor = QueueHead->NextLogicalDescriptor;

    QueueHead->LinkPhysical = NextQueueHead->PhysicalAddress | QH_NEXT_IS_QH;
    QueueHead->NextLogicalDescriptor = (PVOID)NextQueueHead;
}


VOID
CUSBQueue::UnLinkQueueHead(
    PUHCI_QUEUE_HEAD QueueHeadToRemove,
    PUHCI_QUEUE_HEAD PreviousQueueHead)
{
    PreviousQueueHead->LinkPhysical = QueueHeadToRemove->LinkPhysical;
    PreviousQueueHead->NextLogicalDescriptor = QueueHeadToRemove->NextLogicalDescriptor;
}

NTSTATUS
CUSBQueue::AbortDevicePipe(
    IN UCHAR DeviceAddress,
    IN PUSB_ENDPOINT_DESCRIPTOR EndDescriptor)
{
    KIRQL OldLevel;
    PUHCI_TRANSFER_DESCRIPTOR Descriptor;
    PUHCI_QUEUE_HEAD QueueHead, PreviousQueueHead = NULL;
    UCHAR EndpointAddress, EndpointDeviceAddress;
    PUSB_ENDPOINT EndpointDescriptor;

    // get descriptor
    EndpointDescriptor = (PUSB_ENDPOINT)EndDescriptor;

    // acquire lock
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    // get queue head
    m_Hardware->GetQueueHead(UHCI_INTERRUPT_QUEUE, &QueueHead);

    while(QueueHead)
    {
        // get descriptor
        Descriptor = (PUHCI_TRANSFER_DESCRIPTOR)QueueHead->NextElementDescriptor;

        if (Descriptor)
        {
            // extract endpoint address
            EndpointAddress = (Descriptor->Token >> TD_TOKEN_ENDPTADDR_SHIFT) & 0x0F;

            // extract device address
            EndpointDeviceAddress = (Descriptor->Token >> TD_TOKEN_DEVADDR_SHIFT) & 0x7F;

            // check if they match
            if (EndpointAddress == (EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F) &&
                DeviceAddress == EndpointDeviceAddress)
            {
                // cleanup queue head
                QueueHeadCleanup(QueueHead, PreviousQueueHead, &QueueHead);
                continue;
            }
        }

        // move to next queue head
        PreviousQueueHead = QueueHead;
        QueueHead = (PUHCI_QUEUE_HEAD)QueueHead->NextLogicalDescriptor;
    }

    // release lock
    KeReleaseSpinLock(&m_Lock, OldLevel);
    return STATUS_SUCCESS;
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

BOOLEAN
CUSBQueue::IsQueueHeadComplete(
    IN PUHCI_QUEUE_HEAD QueueHead)
{
    PUHCI_TRANSFER_DESCRIPTOR Descriptor;
    ULONG ErrorCount;

    if (QueueHead->NextElementDescriptor == NULL)
    {
        //
        // empty queue head
        //
        DPRINT("QueueHead %p empty element physical\n", QueueHead);
        return FALSE;
    }

    //
    // check all descriptors
    //
    Descriptor = (PUHCI_TRANSFER_DESCRIPTOR)QueueHead->NextElementDescriptor;
    while(Descriptor)
    {
        if (Descriptor->Status & TD_STATUS_ACTIVE)
        {
            //
            // descriptor is still active
            //
            DPRINT("Descriptor %p is active Status %x BufferSize %lu\n", Descriptor, Descriptor->Status, Descriptor->BufferSize);
            return FALSE;
        }

        if (Descriptor->Status & TD_ERROR_MASK)
        {
            //
            // error happened
            //
            DPRINT1("[USBUHCI] Error detected at descriptor %p Physical %x\n", Descriptor, Descriptor->PhysicalAddress);

            //
            // get error count
            //
            ErrorCount = (Descriptor->Status >> TD_ERROR_COUNT_SHIFT) & TD_ERROR_COUNT_MASK;
            if (ErrorCount == 0)
            {
                 //
                 // error retry count elapsed
                 //
                 DPRINT1("[USBUHCI] ErrorBuffer %x TimeOut %x Nak %x BitStuff %x\n",
                         Descriptor->Status & TD_STATUS_ERROR_BUFFER,
                         Descriptor->Status & TD_STATUS_ERROR_TIMEOUT,
                         Descriptor->Status & TD_STATUS_ERROR_NAK,
                         Descriptor->Status & TD_STATUS_ERROR_BITSTUFF);
                 return TRUE;
            }
            else if (Descriptor->Status & TD_STATUS_ERROR_BABBLE)
            {
                 //
                 // babble error
                 //
                 DPRINT1("[USBUHCI] Babble detected\n");
                 return TRUE;
            }
            else
            {
                //
                // stall detected
                //
                DPRINT1("[USBUHCI] Stall detected\n");
            }
        }

        //
        // move to next descriptor
        //
        Descriptor = (PUHCI_TRANSFER_DESCRIPTOR)Descriptor->NextLogicalDescriptor;
    }

    //
    // request is complete
    //
    return TRUE;
}

VOID
CUSBQueue::QueueHeadCleanup(
    IN PUHCI_QUEUE_HEAD QueueHead,
    IN PUHCI_QUEUE_HEAD PreviousQueueHead,
    OUT PUHCI_QUEUE_HEAD *NextQueueHead)
{
    PUHCIREQUEST Request;
    PUHCI_QUEUE_HEAD NewQueueHead;
    NTSTATUS Status;

    //
    // unlink queue head
    //
    UnLinkQueueHead(QueueHead, PreviousQueueHead);

    //
    // get next queue head
    //
    *NextQueueHead = (PUHCI_QUEUE_HEAD)PreviousQueueHead->NextLogicalDescriptor;
    ASSERT(*NextQueueHead != QueueHead);

    //
    // the queue head is complete, is the transfer now completed?
    //
    Request = (PUHCIREQUEST)QueueHead->Request;
    ASSERT(Request);

    //
    // free queue head
    //
    DPRINT("Request %p\n", Request);
    Request->FreeEndpointDescriptor(QueueHead);

    //
    // check if transfer is complete
    //
    if (Request->IsRequestComplete())
    {
        //
        // the transfer is complete
        //
        Request->CompletionCallback();
        Request->Release();
        return;
    }

    //
    // grab new queue head
    //
    Status = Request->GetEndpointDescriptor(&NewQueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get new queue head
        //
        DPRINT1("[USBUHCI] Failed to get new queue head with %x\n", Status);
        Request->CompletionCallback();
        Request->Release();
        return;
    }

    //
    // Link queue head
    //
    Status = AddQueueHead(NewQueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get new queue head
        //
        DPRINT1("[USBUHCI] Failed to add queue head with %x\n", Status);
        Request->CompletionCallback();
        Request->Release();
        return;
    }

}

VOID
CUSBQueue::TransferInterrupt(
    UCHAR ErrorInterrupt)
{
    KIRQL OldLevel;
    PUHCI_QUEUE_HEAD QueueHead, PreviousQueueHead = NULL;
    BOOLEAN IsComplete;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // get queue head
    //
    m_Hardware->GetQueueHead(UHCI_INTERRUPT_QUEUE, &QueueHead);

    while(QueueHead)
    {
        //
        // is queue head complete
        //
        DPRINT("QueueHead %p\n", QueueHead);
        IsComplete = IsQueueHeadComplete(QueueHead);
        if (IsComplete)
        {
            //
            // cleanup queue head
            //
            QueueHeadCleanup(QueueHead, PreviousQueueHead, &QueueHead);
            continue;
        }

        //
        // backup previous queue head
        //
        PreviousQueueHead = QueueHead;

        //
        // get next queue head
        //
        QueueHead = (PUHCI_QUEUE_HEAD)QueueHead->NextLogicalDescriptor;
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);
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
    This = new(NonPagedPool, TAG_USBUHCI) CUSBQueue(0);
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
