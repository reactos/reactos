/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_queue.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbohci.h"
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
    virtual VOID TransferDescriptorCompletionCallback(ULONG TransferDescriptorLogicalAddress);

    // local functions
    BOOLEAN IsTransferDescriptorInEndpoint(IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN ULONG TransferDescriptorLogicalAddress);
    NTSTATUS FindTransferDescriptorInEndpoint(IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN ULONG TransferDescriptorLogicalAddress, OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor, OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor);

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    KSPIN_LOCK m_Lock;                                                                  // list lock
    PUSBHARDWAREDEVICE m_Hardware;                                                      // hardware
    POHCI_ENDPOINT_DESCRIPTOR m_BulkHeadEndpointDescriptor;                             // bulk head descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_ControlHeadEndpointDescriptor;                          // control head descriptor
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
    // get bulk endpoint descriptor
    //
    Hardware->GetBulkHeadEndpointDescriptor(&m_BulkHeadEndpointDescriptor);

    //
    // get control endpoint descriptor
    //
    Hardware->GetControlHeadEndpointDescriptor(&m_ControlHeadEndpointDescriptor);

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
    NTSTATUS Status;
    ULONG Type;
    KIRQL OldLevel;
    POHCI_ENDPOINT_DESCRIPTOR HeadDescriptor;
    POHCI_ENDPOINT_DESCRIPTOR Descriptor;

    DPRINT1("CUSBQueue::AddUSBRequest\n");

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

    //
    // add extra reference which is released when the request is completed
    //
    Request->AddRef();

    //
    // get transfer descriptors
    //
    Status = Request->GetEndpointDescriptor(&Descriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get transfer descriptor
        //
        DPRINT1("CUSBQueue::AddUSBRequest GetEndpointDescriptor failed with %x\n", Status);

        //
        // release reference
        //
        Request->Release();
        return Status;
    }

    //
    // check type
    //
    if (Type == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = m_BulkHeadEndpointDescriptor;
    }
    else if (Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = m_ControlHeadEndpointDescriptor;
    }

    //
    // link endpoints
    //
    Descriptor->NextPhysicalEndpoint = HeadDescriptor->NextPhysicalEndpoint;
    Descriptor->NextDescriptor = HeadDescriptor->NextDescriptor;

    HeadDescriptor->NextPhysicalEndpoint = Descriptor->PhysicalAddress.LowPart;
    HeadDescriptor->NextDescriptor = Descriptor;

    //
    // set descriptor active
    //
    Descriptor->Flags &= ~OHCI_ENDPOINT_SKIP;
    //HeadDescriptor->Flags &= ~OHCI_ENDPOINT_SKIP;

    //
    // notify hardware of our request
    //
    m_Hardware->HeadEndpointDescriptorModified(Type);

    DPRINT1("Request %x %x added to queue\n", Descriptor, Descriptor->PhysicalAddress);


    return STATUS_SUCCESS;
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
CUSBQueue::FindTransferDescriptorInEndpoint(
    IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG TransferDescriptorLogicalAddress,
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor,
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor)
{
    POHCI_ENDPOINT_DESCRIPTOR LastDescriptor = EndpointDescriptor;


    //
    // skip first endpoint head
    //
    EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;

    while(EndpointDescriptor)
    {
        //
        // check if the transfer descriptor is inside the list
        //
        if (IsTransferDescriptorInEndpoint(EndpointDescriptor, TransferDescriptorLogicalAddress))
        {
            //
            // found endpoint
            //
            *OutEndpointDescriptor = EndpointDescriptor;
            *OutPreviousEndpointDescriptor = LastDescriptor;

            //
            // done
            //
            return STATUS_SUCCESS;
        }

        //
        // store last endpoint
        //
        LastDescriptor = EndpointDescriptor;

        //
        // move to next
        //
        EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;
    }

    //
    // failed to endpoint
    //
    return STATUS_NOT_FOUND;
}


BOOLEAN
CUSBQueue::IsTransferDescriptorInEndpoint(
    IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG TransferDescriptorLogicalAddress)
{
    POHCI_GENERAL_TD Descriptor;

    //
    // get first general transfer descriptor
    //
    Descriptor = (POHCI_GENERAL_TD)EndpointDescriptor->HeadLogicalDescriptor;

    //
    // sanity check
    //
    ASSERT(Descriptor);

    do
    {
        if (Descriptor->PhysicalAddress.LowPart == TransferDescriptorLogicalAddress)
        {
            //
            // found descriptor
            //
            return TRUE;
        }

        //
        // move to next
        //
        Descriptor = (POHCI_GENERAL_TD)Descriptor->NextLogicalDescriptor;
    }while(Descriptor);


    //
    // no descriptor found
    //
    return FALSE;
}


VOID
CUSBQueue::TransferDescriptorCompletionCallback(
    ULONG TransferDescriptorLogicalAddress)
{
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, PreviousEndpointDescriptor;
    NTSTATUS Status;
    PUSBREQUEST Request;

    //
    // find transfer descriptor in control list
    //
    Status = FindTransferDescriptorInEndpoint(m_ControlHeadEndpointDescriptor, TransferDescriptorLogicalAddress, &EndpointDescriptor, &PreviousEndpointDescriptor);
    if (NT_SUCCESS(Status))
    {
        //
        // FIXME: make sure this is ok
        // unlink descriptor
        //
        PreviousEndpointDescriptor->NextDescriptor = EndpointDescriptor->NextDescriptor;
        PreviousEndpointDescriptor->NextPhysicalEndpoint = EndpointDescriptor->NextPhysicalEndpoint;

        //
        // get corresponding request
        //
        Request = PUSBREQUEST(EndpointDescriptor->Request);

        //
        // notify of completion
        //
        Request->CompletionCallback(EndpointDescriptor);

        //
        // FIXME: check if complete
        //

        //
        // release request
        //
        Request->Release();
    }

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
