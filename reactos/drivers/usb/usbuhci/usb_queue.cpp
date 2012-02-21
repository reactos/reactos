/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_queue.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbuhci.h"
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

    // com
    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware, PDMA_ADAPTER AdapterObject, IN PDMAMEMORYMANAGER MemManager, IN OPTIONAL PKSPIN_LOCK Lock);
    virtual ULONG GetPendingRequestCount();
    virtual NTSTATUS AddUSBRequest(IUSBRequest * Request);
    virtual NTSTATUS CancelRequests();
    virtual NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest);

    // local
    VOID LinkQueueHead(PUHCI_QUEUE_HEAD QueueHead, PUHCI_QUEUE_HEAD NextQueueHead);

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    KSPIN_LOCK m_Lock;                                                                  // list lock
    PUSBHARDWAREDEVICE m_Hardware;                                                      // hardware
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
    // initialize spinlock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // store hardware
    //
    m_Hardware = Hardware;

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
    PUHCI_QUEUE_HEAD NewQueueHead, QueueHead = NULL;
    NTSTATUS Status;

    DPRINT("CUSBQueue::AddUSBRequest\n");

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

NTSTATUS
CreateUSBQueue(
    PUSBQUEUE *OutUsbQueue)
{
    PUSBQUEUE This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBOHCI) CUSBQueue(0);
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
