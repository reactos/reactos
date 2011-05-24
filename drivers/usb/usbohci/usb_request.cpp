/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_request.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID

#include "usbohci.h"
#include "hardware.h"

class CUSBRequest : public IUSBRequest
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

    // IUSBRequest interface functions
    virtual NTSTATUS InitializeWithSetupPacket(IN PDMAMEMORYMANAGER DmaManager, IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, IN UCHAR DeviceAddress, IN OPTIONAL PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN OUT ULONG TransferBufferLength, IN OUT PMDL TransferBuffer);
    virtual NTSTATUS InitializeWithIrp(IN PDMAMEMORYMANAGER DmaManager, IN OUT PIRP Irp);
    virtual BOOLEAN IsRequestComplete();
    virtual ULONG GetTransferType();
    virtual NTSTATUS GetEndpointDescriptor(struct _OHCI_ENDPOINT_DESCRIPTOR ** OutEndpointDescriptor);
    virtual VOID GetResultStatus(OUT OPTIONAL NTSTATUS *NtStatusCode, OUT OPTIONAL PULONG UrbStatusCode);
    virtual BOOLEAN IsRequestInitialized();
    virtual BOOLEAN IsQueueHeadComplete(struct _QUEUE_HEAD * QueueHead);
    virtual VOID CompletionCallback(struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor);

    // local functions
    ULONG InternalGetTransferType();
    UCHAR InternalGetPidDirection();
    UCHAR GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();
    NTSTATUS BuildControlTransferDescriptor(POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor);
    NTSTATUS CreateGeneralTransferDescriptor(POHCI_GENERAL_TD* OutDescriptor, ULONG BufferSize);
    VOID FreeDescriptor(POHCI_GENERAL_TD Descriptor);
    NTSTATUS AllocateEndpointDescriptor(OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor);
    UCHAR GetEndpointAddress();
    USHORT GetMaxPacketSize();

    // constructor / destructor
    CUSBRequest(IUnknown *OuterUnknown){}
    virtual ~CUSBRequest(){}

protected:
    LONG m_Ref;

    //
    // memory manager for allocating setup packet / queue head / transfer descriptors
    //
    PDMAMEMORYMANAGER m_DmaManager;

    //
    // caller provided irp packet containing URB request
    //
    PIRP m_Irp;

    //
    // transfer buffer length
    //
    ULONG m_TransferBufferLength;

    //
    // current transfer length
    //
    ULONG m_TransferBufferLengthCompleted;

    //
    // Total Transfer Length
    //
    ULONG m_TotalBytesTransferred;

    //
    // transfer buffer MDL
    //
    PMDL m_TransferBufferMDL;

    //
    // caller provided setup packet
    //
    PUSB_DEFAULT_PIPE_SETUP_PACKET m_SetupPacket;

    //
    // completion event for callers who initialized request with setup packet
    //
    PKEVENT m_CompletionEvent;

    //
    // device address for callers who initialized it with device address
    //
    UCHAR m_DeviceAddress;

    //
    // store end point address
    //
    PUSB_ENDPOINT_DESCRIPTOR m_EndpointDescriptor;

    //
    // allocated setup packet from the DMA pool
    //
    PUSB_DEFAULT_PIPE_SETUP_PACKET m_DescriptorPacket;
    PHYSICAL_ADDRESS m_DescriptorSetupPacket;

    //
    // stores the result of the operation
    //
    NTSTATUS m_NtStatusCode;
    ULONG m_UrbStatusCode;

};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::InitializeWithSetupPacket(
    IN PDMAMEMORYMANAGER DmaManager,
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    IN UCHAR DeviceAddress,
    IN OPTIONAL PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN OUT ULONG TransferBufferLength,
    IN OUT PMDL TransferBuffer)
{
    //
    // sanity checks
    //
    PC_ASSERT(DmaManager);
    PC_ASSERT(SetupPacket);

    //
    // initialize packet
    //
    m_DmaManager = DmaManager;
    m_SetupPacket = SetupPacket;
    m_TransferBufferLength = TransferBufferLength;
    m_TransferBufferMDL = TransferBuffer;
    m_DeviceAddress = DeviceAddress;
    m_EndpointDescriptor = EndpointDescriptor;
    m_TotalBytesTransferred = 0;

    //
    // Set Length Completed to 0
    //
    m_TransferBufferLengthCompleted = 0;

    //
    // allocate completion event
    //
    m_CompletionEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_USBOHCI);
    if (!m_CompletionEvent)
    {
        //
        // failed to allocate completion event
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize completion event
    //
    KeInitializeEvent(m_CompletionEvent, NotificationEvent, FALSE);

    //
    // done
    //
    return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::InitializeWithIrp(
    IN PDMAMEMORYMANAGER DmaManager,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;

    //
    // sanity checks
    //
    PC_ASSERT(DmaManager);
    PC_ASSERT(Irp);

    m_DmaManager = DmaManager;
    m_TotalBytesTransferred = 0;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity check
    //
    PC_ASSERT(IoStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);
    PC_ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);
    PC_ASSERT(IoStack->Parameters.Others.Argument1 != 0);

    //
    // get urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;

    //
    // store irp
    //
    m_Irp = Irp;

    //
    // check function type
    //
    switch (Urb->UrbHeader.Function)
    {
        //
        // luckily those request have the same structure layout
        //
        case URB_FUNCTION_CLASS_INTERFACE:
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        {
            //
            // bulk interrupt transfer
            //
            if (Urb->UrbBulkOrInterruptTransfer.TransferBufferLength)
            {
                //
                // Check if there is a MDL
                //
                if (!Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL)
                {
                    //
                    // sanity check
                    //
                    PC_ASSERT(Urb->UrbBulkOrInterruptTransfer.TransferBuffer);

                    //
                    // Create one using TransferBuffer
                    //
                    DPRINT("Creating Mdl from Urb Buffer %p Length %lu\n", Urb->UrbBulkOrInterruptTransfer.TransferBuffer, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                    m_TransferBufferMDL = IoAllocateMdl(Urb->UrbBulkOrInterruptTransfer.TransferBuffer,
                                                        Urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
                                                        FALSE,
                                                        FALSE,
                                                        NULL);

                    if (!m_TransferBufferMDL)
                    {
                        //
                        // failed to allocate mdl
                        //
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    //
                    // build mdl for non paged pool
                    // FIXME: Does hub driver already do this when passing MDL?
                    //
                    MmBuildMdlForNonPagedPool(m_TransferBufferMDL);

                    //
                    // Keep that ehci created the MDL and needs to free it.
                    //
                }
                else
                {
                    m_TransferBufferMDL = Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL;
                }

                //
                // save buffer length
                //
                m_TransferBufferLength = Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

                //
                // Set Length Completed to 0
                //
                m_TransferBufferLengthCompleted = 0;

                //
                // get endpoint descriptor
                //
                m_EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Urb->UrbBulkOrInterruptTransfer.PipeHandle;

            }
            break;
        }
        default:
            DPRINT1("URB Function: not supported %x\n", Urb->UrbHeader.Function);
            PC_ASSERT(FALSE);
    }

    //
    // done
    //
    return STATUS_SUCCESS;

}

//----------------------------------------------------------------------------------------
BOOLEAN
CUSBRequest::IsRequestComplete()
{
    //
    // FIXME: check if request was split
    //

    //
    // Check if the transfer was completed, only valid for Bulk Transfers
    //
    if ((m_TransferBufferLengthCompleted < m_TransferBufferLength)
        && (GetTransferType() == USB_ENDPOINT_TYPE_BULK))
    {
        //
        // Transfer not completed
        //
        return FALSE;
    }
    return TRUE;
}
//----------------------------------------------------------------------------------------
ULONG
CUSBRequest::GetTransferType()
{
    //
    // call internal implementation
    //
    return InternalGetTransferType();
}

USHORT
CUSBRequest::GetMaxPacketSize()
{
    if (!m_EndpointDescriptor)
    {
        //
        // control request
        //
        return 0;
    }

    ASSERT(m_Irp);
    ASSERT(m_EndpointDescriptor);

    //
    // return max packet size
    //
    return m_EndpointDescriptor->wMaxPacketSize;
}

UCHAR
CUSBRequest::GetEndpointAddress()
{
    if (!m_EndpointDescriptor)
    {
        //
        // control request
        //
        return 0;
    }

    ASSERT(m_Irp);
    ASSERT(m_EndpointDescriptor);

    //
    // endpoint number is between 1-15
    //
    return (m_EndpointDescriptor->bEndpointAddress & 0xF);
}

//----------------------------------------------------------------------------------------
ULONG
CUSBRequest::InternalGetTransferType()
{
    ULONG TransferType;

    //
    // check if an irp is provided
    //
    if (m_Irp)
    {
        ASSERT(m_EndpointDescriptor);

        //
        // end point is defined in the low byte of bmAttributes
        //
        TransferType = (m_EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK);
    }
    else
    {
        //
        // initialized with setup packet, must be a control transfer
        //
        TransferType = USB_ENDPOINT_TYPE_CONTROL;
    }

    //
    // done
    //
    return TransferType;
}

UCHAR
CUSBRequest::InternalGetPidDirection()
{
    ASSERT(m_Irp);
    ASSERT(m_EndpointDescriptor);

    //
    // end point is defined in the low byte of bEndpointAddress
    //
    return (m_EndpointDescriptor->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) >> 7;
}


//----------------------------------------------------------------------------------------
UCHAR
CUSBRequest::GetDeviceAddress()
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUSBDEVICE UsbDevice;

    //
    // check if there is an irp provided
    //
    if (!m_Irp)
    {
        //
        // used provided address
        //
        return m_DeviceAddress;
    }

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(m_Irp);

    //
    // get contained urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;

    //
    // check if there is a pipe handle provided
    //
    if (Urb->UrbHeader.UsbdDeviceHandle)
    {
        //
        // there is a device handle provided
        //
        UsbDevice = (PUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle;

        //
        // return device address
        //
        return UsbDevice->GetDeviceAddress();
    }

    //
    // no device handle provided, it is the host root bus
    //
    return 0;
}

VOID
CUSBRequest::FreeDescriptor(
    POHCI_GENERAL_TD Descriptor)
{
    if (Descriptor->BufferSize)
    {
        //
        // free buffer
        //
        m_DmaManager->Release(Descriptor->BufferLogical, Descriptor->BufferSize);
    }

    //
    // release descriptor
    //
    m_DmaManager->Release(Descriptor, sizeof(OHCI_GENERAL_TD));

}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::CreateGeneralTransferDescriptor(
    POHCI_GENERAL_TD* OutDescriptor, 
    ULONG BufferSize)
{
    POHCI_GENERAL_TD Descriptor;
    PHYSICAL_ADDRESS DescriptorAddress;
    NTSTATUS Status;

    //
    // allocate transfer descriptor
    //
    Status = m_DmaManager->Allocate(sizeof(OHCI_GENERAL_TD), (PVOID*)&Descriptor, &DescriptorAddress);
    if (!NT_SUCCESS(Status))
    {
         //
         // no memory
         //
         return Status;
    }

    //
    // initialize descriptor, hardware part
    //
    Descriptor->Flags = 0;
    Descriptor->BufferPhysical = 0;
    Descriptor->NextPhysicalDescriptor = 0;
    Descriptor->LastPhysicalByteAddress = 0;

    //
    // software part
    //
    Descriptor->PhysicalAddress.QuadPart = DescriptorAddress.QuadPart;
    Descriptor->BufferSize = BufferSize;

    if (BufferSize > 0)
    {
        //
        // allocate buffer from dma
        //
        Status = m_DmaManager->Allocate(BufferSize, &Descriptor->BufferLogical, &DescriptorAddress);
        if (!NT_SUCCESS(Status))
        {
             //
             // no memory
             //
             m_DmaManager->Release(Descriptor, sizeof(OHCI_GENERAL_TD));
             return Status;
        }

        //
        // set physical address of buffer 
        //
        Descriptor->BufferPhysical = DescriptorAddress.LowPart;
        Descriptor->LastPhysicalByteAddress = Descriptor->BufferPhysical + BufferSize - 1;
    }

    //
    // store result
    //
    *OutDescriptor = Descriptor;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBRequest::AllocateEndpointDescriptor(
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor)
{
    POHCI_ENDPOINT_DESCRIPTOR Descriptor;
    PHYSICAL_ADDRESS DescriptorAddress;
    NTSTATUS Status;

    //
    // allocate descriptor
    //
    Status = m_DmaManager->Allocate(sizeof(OHCI_ENDPOINT_DESCRIPTOR), (PVOID*)&Descriptor, &DescriptorAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor
        //
        return Status;
    }

    //
    // intialize descriptor
    //
    Descriptor->Flags = OHCI_ENDPOINT_SKIP;

    //
    // append device address and endpoint number
    //
    Descriptor->Flags |= OHCI_ENDPOINT_SET_DEVICE_ADDRESS(m_DeviceAddress);
    Descriptor->Flags |= OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(GetEndpointAddress());
    Descriptor->Flags |= OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(GetMaxPacketSize());

    //
    // FIXME: detect type
    //
    Descriptor->Flags |= OHCI_ENDPOINT_FULL_SPEED;

    Descriptor->HeadPhysicalDescriptor = 0;
    Descriptor->NextPhysicalEndpoint = 0;
    Descriptor->TailPhysicalDescriptor = 0;
    Descriptor->PhysicalAddress.QuadPart = DescriptorAddress.QuadPart;

    //
    // store result
    //
    *OutDescriptor = Descriptor;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBRequest::BuildControlTransferDescriptor(
    POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor)
{
    POHCI_GENERAL_TD SetupDescriptor, StatusDescriptor, DataDescriptor = NULL, LastDescriptor;
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    NTSTATUS Status;

    DPRINT1("CUSBRequest::BuildControlTransferDescriptor\n");

    //
    // allocate endpoint descriptor
    //
    Status = AllocateEndpointDescriptor(&EndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create setup descriptor
        //
        return Status;
    }

    //
    // first allocate setup descriptor
    //
    Status = CreateGeneralTransferDescriptor(&SetupDescriptor, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create setup descriptor
        //
        m_DmaManager->Release(EndpointDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));
        return Status;
    }

    //
    // now create the status descriptor
    //
    Status = CreateGeneralTransferDescriptor(&StatusDescriptor, 0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create status descriptor
        //
        FreeDescriptor(SetupDescriptor);
        m_DmaManager->Release(EndpointDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));
        return Status;
    }

    //
    // finally create the last descriptor
    //
    Status = CreateGeneralTransferDescriptor(&LastDescriptor, 0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create status descriptor
        //
        FreeDescriptor(SetupDescriptor);
        FreeDescriptor(StatusDescriptor);
        m_DmaManager->Release(EndpointDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));
        return Status;
    }


    if (m_TransferBufferLength)
    {
        //
        // FIXME: support more than one data descriptor
        //
        ASSERT(m_TransferBufferLength < 8192);

        //
        // now create the data descriptor
        //
        Status = CreateGeneralTransferDescriptor(&DataDescriptor, 0);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to create status descriptor
            //
            m_DmaManager->Release(EndpointDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));
            FreeDescriptor(SetupDescriptor);
            FreeDescriptor(StatusDescriptor);
            FreeDescriptor(LastDescriptor);
            return Status;
        }

        //
        // initialize data descriptor
        //
        DataDescriptor->Flags = OHCI_TD_BUFFER_ROUNDING| OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE) | OHCI_TD_TOGGLE_CARRY | OHCI_TD_DIRECTION_PID_IN;

        //
        // store physical address of buffer
        //
        DataDescriptor->BufferPhysical = MmGetPhysicalAddress(MmGetMdlVirtualAddress(m_TransferBufferMDL)).LowPart;
        DataDescriptor->LastPhysicalByteAddress = DataDescriptor->BufferPhysical + m_TransferBufferLength - 1; 
    }

    //
    // initialize setup descriptor
    //
    SetupDescriptor->Flags = OHCI_TD_DIRECTION_PID_SETUP | OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_TOGGLE_0 | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE);

    if (m_SetupPacket)
    {
        //
        // copy setup packet
        //
        RtlCopyMemory(SetupDescriptor->BufferLogical, m_SetupPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    }
    else
    {
        //
        // generate setup packet from urb
        //
        ASSERT(FALSE);
    }

    //
    // initialize status descriptor
    //
    StatusDescriptor->Flags = OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_TOGGLE_1 | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);
    if (m_TransferBufferLength == 0)
    {
        //
        // input direction is flipped for the status descriptor
        //
        StatusDescriptor->Flags |= OHCI_TD_DIRECTION_PID_IN;
    }
    else
    {
        //
        // output direction is flipped for the status descriptor
        //
        StatusDescriptor->Flags |= OHCI_TD_DIRECTION_PID_OUT;
    }

    //
    // now link the descriptors
    //
    if (m_TransferBufferLength)
    {
         //
         // link setup descriptor to data descriptor
         //
         SetupDescriptor->NextPhysicalDescriptor = DataDescriptor->PhysicalAddress.LowPart;
         SetupDescriptor->NextLogicalDescriptor = DataDescriptor;

         //
         // link data descriptor to status descriptor
         // FIXME: check if there are more data descriptors
         //
         DataDescriptor->NextPhysicalDescriptor = StatusDescriptor->PhysicalAddress.LowPart;
         DataDescriptor->NextLogicalDescriptor = StatusDescriptor;

         //
         // link status descriptor to last descriptor
         //
         StatusDescriptor->NextPhysicalDescriptor = LastDescriptor->PhysicalAddress.LowPart;
         StatusDescriptor->NextLogicalDescriptor = LastDescriptor;
    }
    else
    {
         //
         // link setup descriptor to status descriptor
         //
         SetupDescriptor->NextPhysicalDescriptor = StatusDescriptor->PhysicalAddress.LowPart;
         SetupDescriptor->NextLogicalDescriptor = StatusDescriptor;

         //
         // link status descriptor to last descriptor
         //
         StatusDescriptor->NextPhysicalDescriptor = LastDescriptor->PhysicalAddress.LowPart;
         StatusDescriptor->NextLogicalDescriptor = LastDescriptor;
    }

    //
    // now link descriptor to endpoint
    //
    EndpointDescriptor->HeadPhysicalDescriptor = SetupDescriptor->PhysicalAddress.LowPart;
    EndpointDescriptor->TailPhysicalDescriptor = LastDescriptor->PhysicalAddress.LowPart;
    EndpointDescriptor->HeadLogicalDescriptor = SetupDescriptor;

    //
    // store result
    //
    *OutEndpointDescriptor = EndpointDescriptor;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::GetEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    ULONG TransferType;
    NTSTATUS Status;

    //
    // get transfer type
    //
    TransferType = InternalGetTransferType();

    //
    // build request depending on type
    //
    switch(TransferType)
    {
        case USB_ENDPOINT_TYPE_CONTROL:
            Status = BuildControlTransferDescriptor((POHCI_ENDPOINT_DESCRIPTOR*)OutDescriptor);
            break;
        case USB_ENDPOINT_TYPE_BULK:
            DPRINT1("USB_ENDPOINT_TYPE_BULK not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED; //BuildBulkTransferQueueHead(OutDescriptor);
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
            DPRINT1("USB_ENDPOINT_TYPE_INTERRUPT not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            DPRINT1("USB_ENDPOINT_TYPE_ISOCHRONOUS not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            PC_ASSERT(FALSE);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // store queue head
        //
        //m_QueueHead = *OutDescriptor;

        //
        // store request object
        //
        (*OutDescriptor)->Request = PVOID(this);
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
VOID
CUSBRequest::GetResultStatus(
    OUT OPTIONAL NTSTATUS * NtStatusCode,
    OUT OPTIONAL PULONG UrbStatusCode)
{
    //
    // sanity check
    //
    PC_ASSERT(m_CompletionEvent);

    //
    // wait for the operation to complete
    //
    KeWaitForSingleObject(m_CompletionEvent, Executive, KernelMode, FALSE, NULL);

    //
    // copy status
    //
    if (NtStatusCode)
    {
        *NtStatusCode = m_NtStatusCode;
    }

    //
    // copy urb status
    //
    if (UrbStatusCode)
    {
        *UrbStatusCode = m_UrbStatusCode;
    }

}


VOID
CUSBRequest::CompletionCallback(
    struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor)
{
    POHCI_GENERAL_TD TransferDescriptor, NextTransferDescriptor;

    DPRINT1("CUSBRequest::CompletionCallback\n");

    //
    // set status code
    //
    m_NtStatusCode = STATUS_SUCCESS;
    m_UrbStatusCode = USBD_STATUS_SUCCESS;

    ASSERT(!m_Irp);

    //
    // FIXME: cleanup descriptors
    //

    //
    // get first general transfer descriptor
    //
    TransferDescriptor = (POHCI_GENERAL_TD)OutDescriptor->HeadLogicalDescriptor;

    while(TransferDescriptor)
    {
        //
        // get next
        //
        NextTransferDescriptor = (POHCI_GENERAL_TD)TransferDescriptor->NextLogicalDescriptor;

        //
        // is there a buffer associated
        //
        if (TransferDescriptor->BufferSize)
        {
            //
            // release buffer
            //
            m_DmaManager->Release(TransferDescriptor->BufferLogical, TransferDescriptor->BufferSize);
        }

        //
        // release descriptor
        //
        m_DmaManager->Release(TransferDescriptor, sizeof(OHCI_GENERAL_TD));

        //
        // move to next
        //
        TransferDescriptor = NextTransferDescriptor;
    }

    //
    // release endpoint descriptor
    //
    m_DmaManager->Release(OutDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));

    //
    // signal completion event
    //
    PC_ASSERT(m_CompletionEvent);
    KeSetEvent(m_CompletionEvent, 0, FALSE);
}


//-----------------------------------------------------------------------------------------
BOOLEAN
CUSBRequest::IsRequestInitialized()
{
    if (m_Irp || m_SetupPacket)
    {
        //
        // request is initialized
        //
        return TRUE;
    }

    //
    // request is not initialized
    //
    return FALSE;
}

//-----------------------------------------------------------------------------------------
BOOLEAN
CUSBRequest::IsQueueHeadComplete(
    struct _QUEUE_HEAD * QueueHead)
{
    UNIMPLEMENTED
    return TRUE;
}



//-----------------------------------------------------------------------------------------
NTSTATUS
InternalCreateUSBRequest(
    PUSBREQUEST *OutRequest)
{
    PUSBREQUEST This;

    //
    // allocate requests
    //
    This = new(NonPagedPool, TAG_USBOHCI) CUSBRequest(0);
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
    *OutRequest = (PUSBREQUEST)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}
