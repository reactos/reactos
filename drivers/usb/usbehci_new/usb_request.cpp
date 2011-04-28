/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usb_request.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID

#include "usbehci.h"
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
    virtual NTSTATUS InitializeWithSetupPacket(IN PDMAMEMORYMANAGER DmaManager, IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, IN OUT ULONG TransferBufferLength, IN OUT PMDL TransferBuffer);
    virtual NTSTATUS InitializeWithIrp(IN PDMAMEMORYMANAGER DmaManager, IN OUT PIRP Irp);
    virtual VOID CompletionCallback(IN NTSTATUS NtStatusCode, IN ULONG UrbStatusCode, IN struct _QUEUE_HEAD *QueueHead);
    virtual VOID CancelCallback(IN NTSTATUS NtStatusCode, IN struct _QUEUE_HEAD *QueueHead);
    virtual NTSTATUS GetQueueHead(struct _QUEUE_HEAD ** OutHead);
    virtual BOOLEAN IsRequestComplete();
    virtual ULONG GetTransferType();
    virtual VOID GetResultStatus(OUT OPTIONAL NTSTATUS *NtStatusCode, OUT OPTIONAL PULONG UrbStatusCode);
    virtual BOOLEAN IsRequestInitialized();
    virtual BOOLEAN ShouldReleaseRequestAfterCompletion();
    virtual VOID FreeQueueHead(struct _QUEUE_HEAD * QueueHead);
    virtual VOID GetTransferBuffer(OUT PMDL * OutMDL, OUT PULONG TransferLength);
    virtual BOOLEAN IsQueueHeadComplete(struct _QUEUE_HEAD * QueueHead);


    // local functions
    ULONG InternalGetTransferType();
    NTSTATUS BuildControlTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS BuildBulkTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS CreateDescriptor(PQUEUE_TRANSFER_DESCRIPTOR *OutDescriptor);
    NTSTATUS CreateQueueHead(PQUEUE_HEAD *OutQueueHead);
    ULONG GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();

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
    // DMA queue head
    //
    PQUEUE_HEAD m_QueueHead;

    //
    // DMA transfer descriptors linked to the queue head
    //
    PQUEUE_TRANSFER_DESCRIPTOR m_TransferDescriptors[3];

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

    //
    // allocate completion event
    //
    m_CompletionEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_USBEHCI);
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
            // bulk / interrupt transfer
            //
            if (Urb->UrbBulkOrInterruptTransfer.TransferBufferLength)
            {
                //
                // FIXME: it must have an MDL
                //
                PC_ASSERT(Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL);

                //
                // get mdl buffer
                //
                m_TransferBufferMDL = Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL;
                m_TransferBufferLength = Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
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
VOID
CUSBRequest::CompletionCallback(
    IN NTSTATUS NtStatusCode,
    IN ULONG UrbStatusCode,
    IN struct _QUEUE_HEAD *QueueHead)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;

    //
    // FIXME: support linked queue heads
    //

    //
    // store completion code
    //
    m_NtStatusCode = NtStatusCode;
    m_UrbStatusCode = UrbStatusCode;

    if (m_Irp)
    {
        //
        // set irp completion status
        //
        m_Irp->IoStatus.Status = NtStatusCode;

        //
        // get current irp stack location
        //
        IoStack = IoGetCurrentIrpStackLocation(m_Irp);

        //
        // get urb
        //
        Urb = (PURB)IoStack->Parameters.Others.Argument1;

        //
        // store urb status
        //
        Urb->UrbHeader.Status = UrbStatusCode;

        //
        // check if the request was successfull
        //
        if (!NT_SUCCESS(NtStatusCode))
        {
            //
            // set returned length to zero in case of error
            //
            Urb->UrbHeader.Length = 0;
        }

        //
        // FIXME: check if the transfer was split
        // if yes dont complete irp yet
        //
        IoCompleteRequest(m_Irp, IO_NO_INCREMENT);
    }
    else
    {
        //
        // signal completion event
        //
        PC_ASSERT(m_CompletionEvent);
        KeSetEvent(m_CompletionEvent, 0, FALSE);
    }
}
//----------------------------------------------------------------------------------------
VOID
CUSBRequest::CancelCallback(
    IN NTSTATUS NtStatusCode,
    IN struct _QUEUE_HEAD *QueueHead)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;

    //
    // FIXME: support linked queue heads
    //

    //
    // store cancelleation code
    //
    m_NtStatusCode = NtStatusCode;

    if (m_Irp)
    {
        //
        // set irp completion status
        //
        m_Irp->IoStatus.Status = NtStatusCode;

        //
        // get current irp stack location
        //
        IoStack = IoGetCurrentIrpStackLocation(m_Irp);

        //
        // get urb
        //
        Urb = (PURB)IoStack->Parameters.Others.Argument1;

        //
        // store urb status
        //
        Urb->UrbHeader.Status = USBD_STATUS_CANCELED;
        Urb->UrbHeader.Length = 0;

        //
        // FIXME: check if the transfer was split
        // if yes dont complete irp yet
        //
        IoCompleteRequest(m_Irp, IO_NO_INCREMENT);
    }
    else
    {
        //
        // signal completion event
        //
        PC_ASSERT(m_CompletionEvent);
        KeSetEvent(m_CompletionEvent, 0, FALSE);
    }
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::GetQueueHead(
    struct _QUEUE_HEAD ** OutHead)
{
    ULONG TransferType;
    NTSTATUS Status;

    //
    // first get transfer type
    //
    TransferType = InternalGetTransferType();

    //
    // build request depending on type
    //
    switch(TransferType)
    {
        case USB_ENDPOINT_TYPE_CONTROL:
            Status = BuildControlTransferQueueHead(OutHead);
            break;
        case USB_ENDPOINT_TYPE_BULK:
            Status = BuildBulkTransferQueueHead(OutHead);
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
        m_QueueHead = *OutHead;

        //
        // store request object
        //
        (*OutHead)->Request = PVOID(this);
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
BOOLEAN
CUSBRequest::IsRequestComplete()
{
    //
    // FIXME: check if request was split
    //
    UNIMPLEMENTED
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

//----------------------------------------------------------------------------------------
ULONG
CUSBRequest::InternalGetTransferType()
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    ULONG TransferType;

    //
    // check if an irp is provided
    //
    if (m_Irp)
    {
        //
        // get stack location
        //
        IoStack = IoGetCurrentIrpStackLocation(m_Irp);

        //
        // get urb
        //
        Urb = (PURB)IoStack->Parameters.Others.Argument1;

        //
        // check if there is a handle
        //
        if (Urb->UrbBulkOrInterruptTransfer.PipeHandle)
        {
            //
            // cast to end point
            //
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Urb->UrbBulkOrInterruptTransfer.PipeHandle;

            //
            // end point is defined in the low byte of bmAttributes
            //
            TransferType = (EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK);
        }
        else
        {
            //
            // no pipe handle, assume it is a control transfer
            //
            TransferType = USB_ENDPOINT_TYPE_CONTROL;
        }
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

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::BuildControlTransferQueueHead(
    PQUEUE_HEAD * OutHead)
{
    NTSTATUS Status;
    ULONG NumTransferDescriptors, Index;
    PQUEUE_HEAD QueueHead;

    //
    // first allocate the queue head
    //
    Status  = CreateQueueHead(&QueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity check
    //
    PC_ASSERT(QueueHead);

    //
    // create setup packet
    //
    Status = BuildSetupPacket();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate setup packet
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // calculate num of transfer descriptors
    //
    NumTransferDescriptors = m_TransferBufferMDL != 0 ? 3 : 2;

    //
    // allocate transfer descriptors
    //
    for(Index = 0; Index < NumTransferDescriptors; Index++)
    {
        //
        // allocate transfer descriptor
        //
        Status = CreateDescriptor(&m_TransferDescriptors[Index]);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to allocate transfer descriptor
            //
            return Status;
        }
    }

    //
    // now initialize the queue head
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();
    
	//if (PipeHandle)
    //    QueueHead->EndPointCharacteristics.EndPointNumber = ((PUSB_ENDPOINT_DESCRIPTOR)PipeHandle)->bEndpointAddress & 0x0F;

    QueueHead->Token.Bits.DataToggle = TRUE;

    //
    // setup descriptors
    //
    m_TransferDescriptors[0]->Token.Bits.PIDCode = PID_CODE_SETUP_TOKEN;
    m_TransferDescriptors[0]->Token.Bits.TotalBytesToTransfer = sizeof(USB_DEFAULT_PIPE_SETUP_PACKET);
    m_TransferDescriptors[0]->Token.Bits.DataToggle = FALSE;

    if (m_TransferBufferMDL)
    {
        //
        // setup in descriptor
        //
        m_TransferDescriptors[1]->Token.Bits.PIDCode = PID_CODE_IN_TOKEN;
        m_TransferDescriptors[1]->Token.Bits.TotalBytesToTransfer = m_TransferBufferLength;

        //
        // setup out descriptor
        //
        m_TransferDescriptors[2]->Token.Bits.PIDCode = PID_CODE_OUT_TOKEN;
        m_TransferDescriptors[2]->Token.Bits.TotalBytesToTransfer = 0;

        //
        // link descriptors
        //
        m_TransferDescriptors[0]->NextPointer = m_TransferDescriptors[1]->PhysicalAddr;

        //
        // special case, setup alternative next descriptor in case of error
        // HAIKU links to dead descriptor
        //
        m_TransferDescriptors[0]->AlternateNextPointer = m_TransferDescriptors[2]->PhysicalAddr;
        m_TransferDescriptors[1]->NextPointer = m_TransferDescriptors[2]->PhysicalAddr;
        m_TransferDescriptors[1]->AlternateNextPointer = m_TransferDescriptors[2]->PhysicalAddr;

        //
        // interrupt on completion
        //
        m_TransferDescriptors[2]->Token.Bits.InterruptOnComplete = TRUE;

    }
    else
    {
        //
        // no buffer, setup in descriptor
        //
        m_TransferDescriptors[1]->Token.Bits.PIDCode = PID_CODE_IN_TOKEN;
        m_TransferDescriptors[1]->Token.Bits.TotalBytesToTransfer = 0;

        //
        // link descriptors
        //
        m_TransferDescriptors[0]->NextPointer = m_TransferDescriptors[1]->PhysicalAddr;
        m_TransferDescriptors[0]->AlternateNextPointer = m_TransferDescriptors[1]->PhysicalAddr;

        //
        // interrupt on completion
        //
        m_TransferDescriptors[1]->Token.Bits.InterruptOnComplete = TRUE;
    }

    //
    // link setup packet into buffer - Physical Address!!!
    //
    m_TransferDescriptors[0]->BufferPointer[0] = (ULONG)PtrToUlong(m_DescriptorSetupPacket.LowPart);

    //
    // link transfer descriptors to queue head
    //
    QueueHead->NextPointer = m_TransferDescriptors[0]->PhysicalAddr;

    //
    // store result
    //
    *OutHead = QueueHead;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::BuildBulkTransferQueueHead(
    PQUEUE_HEAD * OutHead)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::CreateDescriptor(
    PQUEUE_TRANSFER_DESCRIPTOR *OutDescriptor)
{
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    PHYSICAL_ADDRESS TransferDescriptorPhysicalAddress;

    //
    // allocate descriptor
    //
    Status = m_DmaManager->Allocate(sizeof(QUEUE_TRANSFER_DESCRIPTOR), (PVOID*)&Descriptor, &TransferDescriptorPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate transfer descriptor
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize transfer descriptor
    //
    Descriptor->NextPointer = TERMINATE_POINTER;
    Descriptor->AlternateNextPointer = TERMINATE_POINTER;
    Descriptor->Token.Bits.DataToggle = TRUE;
    Descriptor->Token.Bits.ErrorCounter = 0x03;
    Descriptor->Token.Bits.Active = TRUE;
    Descriptor->PhysicalAddr = TransferDescriptorPhysicalAddress.LowPart;

    //
    // store result
    //
    *OutDescriptor = Descriptor;

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::CreateQueueHead(
    PQUEUE_HEAD *OutQueueHead)
{
    PQUEUE_HEAD QueueHead;
    PHYSICAL_ADDRESS QueueHeadPhysicalAddress;
    NTSTATUS Status;

    //
    // allocate queue head
    //
    Status = m_DmaManager->Allocate(sizeof(QUEUE_HEAD), (PVOID*)&QueueHead, &QueueHeadPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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

    //
    // FIXME check if that is really needed
    //
    QueueHead->PhysicalAddr = QueueHeadPhysicalAddress.LowPart;

    //
    // output queue head
    //
    *OutQueueHead = QueueHead;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
ULONG
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
        // no irp is provided
        // assume it is for device address 0
        return 0;
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

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::BuildSetupPacket()
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // allocate common buffer setup packet
    //
    Status = m_DmaManager->Allocate(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET), (PVOID*)&m_DescriptorPacket, &PhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // no memory
        //
        return Status;
    }

    if (m_SetupPacket)
    {
        //
        // copy setup packet
        //
        RtlCopyMemory(m_DescriptorPacket, m_SetupPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
        m_DescriptorSetupPacket = PhysicalAddress;
    }
    else
    {
        //
        // build setup packet from urb
        //
        Status = BuildSetupPacketFromURB();
    }

    //
    // done
    //
    return Status;
}


NTSTATUS
CUSBRequest::BuildSetupPacketFromURB()
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    //
    // sanity checks
    //
    PC_ASSERT(m_Irp);
    PC_ASSERT(m_DescriptorPacket);

    //
    // get stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(m_Irp);

    //
    // get urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;

    //
    // zero descriptor packet
    //
    RtlZeroMemory(m_DescriptorPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));


    switch (Urb->UrbHeader.Function)
    {
    /* CLEAR FEATURE */
        case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
        case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
        case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
            UNIMPLEMENTED
            break;

    /* GET CONFIG */
        case URB_FUNCTION_GET_CONFIGURATION:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_CONFIGURATION;
            m_DescriptorPacket->bmRequestType.B = 0x80;
            m_DescriptorPacket->wLength = 1;
            break;

    /* GET DESCRIPTOR */
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_DESCRIPTOR;
            m_DescriptorPacket->wValue.LowByte = Urb->UrbControlDescriptorRequest.Index;
            m_DescriptorPacket->wValue.HiByte = Urb->UrbControlDescriptorRequest.DescriptorType;
            m_DescriptorPacket->wIndex.W = Urb->UrbControlDescriptorRequest.LanguageId;
            m_DescriptorPacket->wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;
            m_DescriptorPacket->bmRequestType.B = 0x80;
            break;

    /* GET INTERFACE */
        case URB_FUNCTION_GET_INTERFACE:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_CONFIGURATION;
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x80;
            m_DescriptorPacket->wLength = 1;
            break;

    /* GET STATUS */
        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x80;
            m_DescriptorPacket->wLength = 2;
            break;

    case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index != 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x81;
            m_DescriptorPacket->wLength = 2;
            break;

    case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
            m_DescriptorPacket->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index != 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x82;
            m_DescriptorPacket->wLength = 2;
            break;

    /* SET ADDRESS */

    /* SET CONFIG */
        case URB_FUNCTION_SELECT_CONFIGURATION:
            m_DescriptorPacket->bRequest = USB_REQUEST_SET_CONFIGURATION;
            m_DescriptorPacket->wValue.W = Urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue;
            m_DescriptorPacket->wIndex.W = 0;
            m_DescriptorPacket->wLength = 0;
            m_DescriptorPacket->bmRequestType.B = 0x00;
            break;

    /* SET DESCRIPTOR */
        case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
            UNIMPLEMENTED
            break;

    /* SET FEATURE */
        case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
            m_DescriptorPacket->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x80;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
            m_DescriptorPacket->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x81;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
            m_DescriptorPacket->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            m_DescriptorPacket->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            m_DescriptorPacket->bmRequestType.B = 0x82;
            break;

    /* SET INTERFACE*/
        case URB_FUNCTION_SELECT_INTERFACE:
            m_DescriptorPacket->bRequest = USB_REQUEST_SET_INTERFACE;
            m_DescriptorPacket->wValue.W = Urb->UrbSelectInterface.Interface.AlternateSetting;
            m_DescriptorPacket->wIndex.W = Urb->UrbSelectInterface.Interface.InterfaceNumber;
            m_DescriptorPacket->wLength = 0;
            m_DescriptorPacket->bmRequestType.B = 0x01;
            break;

    /* SYNC FRAME */
        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
            UNIMPLEMENTED
            break;
        default:
            UNIMPLEMENTED
            break;
    }

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
CUSBRequest::ShouldReleaseRequestAfterCompletion()
{
    if (m_Irp)
    {
        //
        // the request is completed, release it
        //
        return TRUE;
    }
    else
    {
        //
        // created with an setup packet, don't release
        //
        return FALSE;
    }
}

//-----------------------------------------------------------------------------------------
VOID
CUSBRequest::FreeQueueHead(
    IN struct _QUEUE_HEAD * QueueHead)
{
    //
    // FIXME: support chained queue heads
    //
    PC_ASSERT(QueueHead == m_QueueHead);

    //
    // release queue head
    //
    m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));

    //
    // nullify pointer
    //
    m_QueueHead = 0;

    //
    // release transfer descriptors
    //

    if (m_TransferDescriptors[0])
    {
        //
        // release transfer descriptors
        //
        m_DmaManager->Release(m_TransferDescriptors[0], sizeof(QUEUE_TRANSFER_DESCRIPTOR));
        m_TransferDescriptors[0] = 0;
    }

    if (m_TransferDescriptors[1])
    {
        //
        // release transfer descriptors
        //
        m_DmaManager->Release(m_TransferDescriptors[1], sizeof(QUEUE_TRANSFER_DESCRIPTOR));
        m_TransferDescriptors[1] = 0;
    }

    if (m_TransferDescriptors[2])
    {
        //
        // release transfer descriptors
        //
        m_DmaManager->Release(m_TransferDescriptors[2], sizeof(QUEUE_TRANSFER_DESCRIPTOR));
        m_TransferDescriptors[2] = 0;
    }
}

//-----------------------------------------------------------------------------------------
BOOLEAN
CUSBRequest::IsQueueHeadComplete(
    struct _QUEUE_HEAD * QueueHead)
{
    ULONG Index;

    //
    // first check - is the queue head currently active
    //
    if (QueueHead->Token.Bits.Active)
    {
        //
        // queue head is active (currently processed)
        //
        return FALSE;
    }

    //
    // FIXME: support chained queue heads
    //
    for(Index = 0; Index < 3; Index++)
    {
        //
        // check transfer descriptors for completion
        //
        if (m_TransferDescriptors[Index])
        {
            //
            // check for serious error
            //
            PC_ASSERT(m_TransferDescriptors[Index]->Token.Bits.Halted == 0);

            //
            // the transfer descriptor should be in the same state as the queue head
            //
            PC_ASSERT(m_TransferDescriptors[Index]->Token.Bits.Active == 0);
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------------------
VOID
CUSBRequest::GetTransferBuffer(
    OUT PMDL * OutMDL,
    OUT PULONG TransferLength)
{
    // sanity checks
    PC_ASSERT(OutMDL);
    PC_ASSERT(TransferLength);

    *OutMDL = m_TransferBufferMDL;
    *TransferLength = m_TransferBufferLength;
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
    This = new(NonPagedPool, TAG_USBEHCI) CUSBRequest(0);
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
