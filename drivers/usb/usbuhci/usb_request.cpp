/*
 * PROJECT:     ReactOS Universal Serial Bus Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbuhci/usb_request.cpp
 * PURPOSE:     USB UHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbuhci.h"

#define NDEBUG
#include <debug.h>

class CUSBRequest : public IUHCIRequest
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
    IMP_IUSBREQUEST
    IMP_IUHCIREQUEST

    // local functions
    ULONG InternalGetTransferType();
    UCHAR InternalGetPidDirection();
    UCHAR GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();
    UCHAR GetEndpointAddress();
    USHORT GetMaxPacketSize();
    NTSTATUS CreateDescriptor(PUHCI_TRANSFER_DESCRIPTOR *OutDescriptor, IN UCHAR PidCode, ULONG BufferLength);
    NTSTATUS BuildControlTransferDescriptor(IN PUHCI_QUEUE_HEAD * OutQueueHead);
    NTSTATUS BuildBulkInterruptTransferDescriptor(IN PUHCI_QUEUE_HEAD * OutQueueHead);
    NTSTATUS BuildQueueHead(OUT PUHCI_QUEUE_HEAD *OutQueueHead);
    VOID FreeDescriptor(IN PUHCI_TRANSFER_DESCRIPTOR Descriptor);
    NTSTATUS BuildTransferDescriptorChain(IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, IN UCHAR InitialDataToggle, OUT PUHCI_TRANSFER_DESCRIPTOR * OutFirstDescriptor, OUT PUHCI_TRANSFER_DESCRIPTOR * OutLastDescriptor, OUT PULONG OutTransferBufferOffset, OUT PUCHAR OutDataToggle);

    // constructor / destructor
    CUSBRequest(IUnknown *OuterUnknown);
    virtual ~CUSBRequest();

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
    PUSB_ENDPOINT m_EndpointDescriptor;

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

    //
    // store device speed
    //
    USB_DEVICE_SPEED m_DeviceSpeed;

    // base
    PVOID m_Base;

};

//----------------------------------------------------------------------------------------
CUSBRequest::CUSBRequest(IUnknown *OuterUnknown) :
    m_CompletionEvent(NULL)
{
    UNREFERENCED_PARAMETER(OuterUnknown);
}

//----------------------------------------------------------------------------------------
CUSBRequest::~CUSBRequest()
{
    if (m_CompletionEvent != NULL)
    {
        ExFreePoolWithTag(m_CompletionEvent, TAG_USBUHCI);
    }
}

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
    IN PUSBDEVICE Device,
    IN OPTIONAL struct _USB_ENDPOINT * EndpointDescriptor,
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
    m_DeviceAddress = Device->GetDeviceAddress();
    m_EndpointDescriptor = EndpointDescriptor;
    m_TotalBytesTransferred = 0;
    m_DeviceSpeed = Device->GetSpeed();

    //
    // Set Length Completed to 0
    //
    m_TransferBufferLengthCompleted = 0;

    //
    // allocate completion event
    //
    m_CompletionEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_USBUHCI);
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
    IN PUSBDEVICE Device,
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
    m_DeviceSpeed = Device->GetSpeed();

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
        case URB_FUNCTION_ISOCH_TRANSFER:
        {
            //
            // there must be at least one packet
            //
            ASSERT(Urb->UrbIsochronousTransfer.NumberOfPackets);

            //
            // is there data to be transferred
            //
            if (Urb->UrbIsochronousTransfer.TransferBufferLength)
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
                }
                else
                {
                    //
                    // use provided mdl
                    //
                    m_TransferBufferMDL = Urb->UrbIsochronousTransfer.TransferBufferMDL;
                }
            }
 
            //
            // save buffer length
            //
            m_TransferBufferLength = Urb->UrbIsochronousTransfer.TransferBufferLength;

            //
            // Set Length Completed to 0
            //
            m_TransferBufferLengthCompleted = 0;

            //
            // get endpoint descriptor
            //
            m_EndpointDescriptor = (PUSB_ENDPOINT)Urb->UrbIsochronousTransfer.PipeHandle;

            //
            // completed initialization
            //
            break;
        }
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
                m_EndpointDescriptor = (PUSB_ENDPOINT)Urb->UrbBulkOrInterruptTransfer.PipeHandle;

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
    return m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
}

UCHAR
CUSBRequest::GetInterval()
{
    ASSERT(m_EndpointDescriptor);
    ASSERT((m_EndpointDescriptor->EndPointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT);

    //
    // return interrupt interval
    //
    return m_EndpointDescriptor->EndPointDescriptor.bInterval;
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
    return (m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0xF);
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
        TransferType = (m_EndpointDescriptor->EndPointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK);
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
    if (m_EndpointDescriptor)
    {
        //
        // end point direction is highest bit in bEndpointAddress
        //
        return (m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) >> 7;
    }
    else
    {
        //
        // request arrives on the control pipe, extract direction from setup packet
        //
        ASSERT(m_SetupPacket);
        return (m_SetupPacket->bmRequestType.B >> 7);
    }
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

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::GetEndpointDescriptor(
    struct _UHCI_QUEUE_HEAD ** OutQueueHead)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG TransferType;

    // get transfer type
    TransferType = InternalGetTransferType();

    if (TransferType == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // build queue head
        //
        Status = BuildControlTransferDescriptor(OutQueueHead);
    }
    else if (TransferType == USB_ENDPOINT_TYPE_INTERRUPT || TransferType == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // build queue head
        //
        Status = BuildBulkInterruptTransferDescriptor(OutQueueHead);
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    //
    // store result
    //
    (*OutQueueHead)->Request = PVOID(this);

    //
    // done
    //
    return STATUS_SUCCESS;
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
NTSTATUS
CUSBRequest::CreateDescriptor(
    OUT PUHCI_TRANSFER_DESCRIPTOR *OutDescriptor,
    IN UCHAR PidCode,
    ULONG BufferLength)
{
    PUHCI_TRANSFER_DESCRIPTOR Descriptor;
    PHYSICAL_ADDRESS Address;
    NTSTATUS Status;

    //
    // allocate descriptor
    //
    Status = m_DmaManager->Allocate(sizeof(UHCI_TRANSFER_DESCRIPTOR), (PVOID*)&Descriptor, &Address);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[USBUHCI] Failed to allocate descriptor\n");
        return Status;
    }

    //
    // init descriptor
    //
    Descriptor->PhysicalAddress = Address.LowPart;
    Descriptor->Status = TD_STATUS_ACTIVE;

    if (InternalGetTransferType() == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        //
        // isochronous transfer descriptor
        //
        Descriptor->Status |= TD_CONTROL_ISOCHRONOUS;
    }
    else
    {
        //
        // error count
        //
        Descriptor->Status |= TD_CONTROL_3_ERRORS;

        if (PidCode == TD_TOKEN_IN && (InternalGetTransferType() != USB_ENDPOINT_TYPE_CONTROL))
        {
            //
            // enable short packet detect for bulk & interrupt
            //
            Descriptor->Status |= TD_CONTROL_SPD;
        }
    }

    //
    // is it low speed device
    //
    if (m_DeviceSpeed == UsbLowSpeed)
    {
        //
        // low speed device
        //
        Descriptor->Status |= TD_CONTROL_LOWSPEED;
    }

    //
    // store buffer size
    //
    Descriptor->BufferSize = BufferLength;

    //
    // is there a buffer
    //
    if(BufferLength)
    {
        //
        // store buffer length
        //
        Descriptor->Token = (BufferLength - 1) << TD_TOKEN_MAXLEN_SHIFT;
    }
    else
    {
        //
        // no buffer magic constant
        //
        Descriptor->Token = TD_TOKEN_NULL_DATA;
    }

    //
    // store address & endpoint number
    //
    Descriptor->Token |= GetEndpointAddress() << TD_TOKEN_ENDPTADDR_SHIFT;
    Descriptor->Token |= GetDeviceAddress() << TD_TOKEN_DEVADDR_SHIFT | PidCode;

    if (BufferLength)
    {
        //
        // allocate buffer for descriptor
        //
        Status = m_DmaManager->Allocate(BufferLength, (PVOID*)&Descriptor->BufferLogical, &Address);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("[USBUHCI] Failed to allocate descriptor buffer length %lu\n", BufferLength);
            m_DmaManager->Release(Descriptor, sizeof(UHCI_TRANSFER_DESCRIPTOR));
            return Status;
        }

        //
        // store address
        //
        Descriptor->BufferPhysical = Address.LowPart;
    }

    //
    // done
    //
    *OutDescriptor = Descriptor;
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBRequest::BuildTransferDescriptorChain(
    IN PVOID TransferBuffer, 
    IN ULONG TransferBufferLength, 
    IN UCHAR PidCode,
    IN UCHAR InitialDataToggle,
    OUT PUHCI_TRANSFER_DESCRIPTOR * OutFirstDescriptor, 
    OUT PUHCI_TRANSFER_DESCRIPTOR * OutLastDescriptor, 
    OUT PULONG OutTransferBufferOffset,
    OUT PUCHAR OutDataToggle)
{
    PUHCI_TRANSFER_DESCRIPTOR FirstDescriptor = NULL, CurrentDescriptor, LastDescriptor = NULL;
    ULONG TransferBufferOffset = 0;
    NTSTATUS Status;
    ULONG MaxPacketSize, CurrentBufferSize;

    //
    // FIXME FIXME FIXME FIXME FIXME 
    //
    if (GetDeviceSpeed() == UsbLowSpeed)
    {
        //
        // low speed use max 8 bytes
        //
        MaxPacketSize = 8;
    }
    else
    {
        if (m_EndpointDescriptor)
        {
            //
            // use endpoint size
            //
            MaxPacketSize = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
        }
        else
        {
            //
            // use max 64 bytes
            //
            MaxPacketSize = 64;
        }
    }

    do
    {
        //
        // determine current packet size
        //
        CurrentBufferSize = min(MaxPacketSize, TransferBufferLength - TransferBufferOffset);

        //
        // allocate descriptor
        //
        Status = CreateDescriptor(&CurrentDescriptor, PidCode, CurrentBufferSize);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to allocate queue head
            //
            DPRINT1("[UHCI] Failed to create descriptor\n");
            ASSERT(FALSE);
            return Status;
        }

        if (PidCode == TD_TOKEN_OUT)
        {
             //
             // copy buffer
             //
             RtlCopyMemory(CurrentDescriptor->BufferLogical, (PVOID)((ULONG_PTR)TransferBuffer + TransferBufferOffset), CurrentBufferSize);
        }
        else
        {
            //
            // store user buffer
            //
            CurrentDescriptor->UserBuffer = (PVOID)((ULONG_PTR)TransferBuffer + TransferBufferOffset);
        }

        if (!FirstDescriptor)
        {
            //
            // first descriptor
            //
            FirstDescriptor = CurrentDescriptor;
        }
        else
        {
            //
            // link descriptor
            //
            LastDescriptor->LinkPhysical = CurrentDescriptor->PhysicalAddress | TD_DEPTH_FIRST;
            LastDescriptor->NextLogicalDescriptor = (PVOID)CurrentDescriptor;
        }

        if (InitialDataToggle)
        {
            //
            // apply data toggle
            //
            CurrentDescriptor->Token |= TD_TOKEN_DATA1;
        }

        //
        // re-run
        //
        LastDescriptor = CurrentDescriptor;
        TransferBufferOffset += CurrentBufferSize;
        InitialDataToggle = !InitialDataToggle;

    }while(TransferBufferOffset < TransferBufferLength);

    if (OutTransferBufferOffset)
    {
        //
        // store transfer buffer length
        //
        *OutTransferBufferOffset = TransferBufferOffset;
    }

    if (OutFirstDescriptor)
    {
        //
        // store first descriptor
        //
        *OutFirstDescriptor = FirstDescriptor;
    }

    if (OutLastDescriptor)
    {
        //
        // store last descriptor
        //
        *OutLastDescriptor = CurrentDescriptor;
    }

    if (OutDataToggle)
    {
        //
        // store data toggle
        //
        *OutDataToggle = InitialDataToggle;
    }

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBRequest::BuildQueueHead(
    OUT PUHCI_QUEUE_HEAD *OutQueueHead)
{
    PUHCI_QUEUE_HEAD QueueHead;
    NTSTATUS Status;
    PHYSICAL_ADDRESS Address;

    //
    // allocate queue head
    //
    Status = m_DmaManager->Allocate(sizeof(UHCI_QUEUE_HEAD), (PVOID*)&QueueHead, &Address);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        DPRINT1("[UHCI] Failed to create queue head\n");
        return Status;
    }

    //
    // store address
    //
    QueueHead->PhysicalAddress = Address.LowPart;
    QueueHead->ElementPhysical = Address.LowPart;

    //
    // store result
    //
    *OutQueueHead = QueueHead;
    return STATUS_SUCCESS;
}

VOID
CUSBRequest::FreeDescriptor(
    IN PUHCI_TRANSFER_DESCRIPTOR Descriptor)
{
    if (Descriptor->BufferLogical)
    {
        //
        // free buffer
        //
        m_DmaManager->Release(Descriptor->BufferLogical, Descriptor->BufferSize);
    }

    //
    // free descriptors
    //
    m_DmaManager->Release(Descriptor, sizeof(UHCI_TRANSFER_DESCRIPTOR));
}

NTSTATUS
CUSBRequest::BuildBulkInterruptTransferDescriptor(
    IN PUHCI_QUEUE_HEAD * OutQueueHead)
{
    NTSTATUS Status;
    PUHCI_QUEUE_HEAD QueueHead;
    PUHCI_TRANSFER_DESCRIPTOR FirstDescriptor, LastDescriptor;
    ULONG ChainDescriptorLength;
    BOOLEAN Direction;
    PVOID Buffer;
    ULONG BufferSize;

    // create queue head
    Status = BuildQueueHead(&QueueHead);
    if (!NT_SUCCESS(Status))
    {
        // failed to allocate queue head
        DPRINT1("[UHCI] Failed to create queue head\n");
        return Status;
    }

    // get direction
    Direction = InternalGetPidDirection();

    if (!m_Base)
    {
        // get buffer base
        m_Base = MmGetMdlVirtualAddress(m_TransferBufferMDL);

        // sanity check
        ASSERT(m_Base != NULL);
    }

    // get new buffer offset
    Buffer = (PVOID)((ULONG_PTR)m_Base + m_TransferBufferLengthCompleted);

    // FIXME determine buffer limit
    BufferSize = min(m_TransferBufferLength - m_TransferBufferLengthCompleted, PAGE_SIZE);

    // create descriptor chain
    Status = BuildTransferDescriptorChain(Buffer,
                                          BufferSize,
                                          Direction ? TD_TOKEN_IN : TD_TOKEN_OUT,
                                          m_EndpointDescriptor->DataToggle,
                                          &FirstDescriptor,
                                          &LastDescriptor,
                                          &ChainDescriptorLength,
                                           NULL);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor
        //
        DPRINT1("[UHCI] Failed to create descriptor chain\n");
        m_DmaManager->Release(QueueHead, sizeof(UHCI_QUEUE_HEAD));
        return Status;
    }

    // adjust buffer offset
    m_TransferBufferLengthCompleted += ChainDescriptorLength;

    // fire interrupt when the last descriptor is complete
    LastDescriptor->Status |= TD_CONTROL_IOC;
    LastDescriptor->LinkPhysical = TD_TERMINATE;
    LastDescriptor->NextLogicalDescriptor = NULL;

    // link queue head with first data descriptor descriptor
    QueueHead->NextElementDescriptor = (PVOID)FirstDescriptor;
    QueueHead->ElementPhysical = FirstDescriptor->PhysicalAddress;

    // store result
    *OutQueueHead = QueueHead;
    return STATUS_SUCCESS;
}


NTSTATUS
CUSBRequest::BuildControlTransferDescriptor(
    IN PUHCI_QUEUE_HEAD * OutQueueHead)
{
    PUHCI_TRANSFER_DESCRIPTOR SetupDescriptor, StatusDescriptor, FirstDescriptor, LastDescriptor;
    PUHCI_QUEUE_HEAD QueueHead;
    BOOLEAN Direction;
    NTSTATUS Status;
    ULONG ChainDescriptorLength;

    //
    // create queue head
    //
    Status = BuildQueueHead(&QueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        DPRINT1("[UHCI] Failed to create queue head\n");
        return Status;
    }

    //
    // get direction
    //
    Direction = InternalGetPidDirection();

    //
    // build setup descriptor
    //
    Status = CreateDescriptor(&SetupDescriptor,
                              TD_TOKEN_SETUP,
                              sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor
        //
        DPRINT1("[UHCI] Failed to create setup descriptor\n");
        m_DmaManager->Release(QueueHead, sizeof(UHCI_QUEUE_HEAD));
        return Status;
    }

    //
    // build status descriptor
    //
    Status = CreateDescriptor(&StatusDescriptor,
                              Direction ? TD_TOKEN_OUT : TD_TOKEN_IN,
                              0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor
        //
        DPRINT1("[UHCI] Failed to create status descriptor\n");
        FreeDescriptor(SetupDescriptor);
        m_DmaManager->Release(QueueHead, sizeof(UHCI_QUEUE_HEAD));
        return Status;
    }

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
    // init status descriptor
    //
    StatusDescriptor->Status |= TD_CONTROL_IOC;
    StatusDescriptor->Token |= TD_TOKEN_DATA1;
    StatusDescriptor->LinkPhysical = TD_TERMINATE;
    StatusDescriptor->NextLogicalDescriptor = NULL;

    if (m_TransferBufferLength)
    {
        //
        // create descriptor chain
        //
        Status = BuildTransferDescriptorChain(MmGetMdlVirtualAddress(m_TransferBufferMDL),
                                              m_TransferBufferLength,
                                              Direction ? TD_TOKEN_IN : TD_TOKEN_OUT,
                                              TRUE,
                                              &FirstDescriptor,
                                              &LastDescriptor,
                                              &ChainDescriptorLength,
                                              NULL);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to allocate descriptor
            //
            DPRINT1("[UHCI] Failed to create descriptor chain\n");
            FreeDescriptor(SetupDescriptor);
            FreeDescriptor(StatusDescriptor);
            m_DmaManager->Release(QueueHead, sizeof(UHCI_QUEUE_HEAD));
            return Status;
        }

        //
        // link setup descriptor to first data descriptor
        //
        SetupDescriptor->LinkPhysical = FirstDescriptor->PhysicalAddress | TD_DEPTH_FIRST;
        SetupDescriptor->NextLogicalDescriptor = (PVOID)FirstDescriptor;

        //
        // link last data descriptor to status descriptor
        //
        LastDescriptor->LinkPhysical = StatusDescriptor->PhysicalAddress | TD_DEPTH_FIRST;
        LastDescriptor->NextLogicalDescriptor = (PVOID)StatusDescriptor;
    }
    else
    {
        //
        // directly link setup to status descriptor
        //
        SetupDescriptor->LinkPhysical = StatusDescriptor->PhysicalAddress | TD_DEPTH_FIRST;
        SetupDescriptor->NextLogicalDescriptor = (PVOID)StatusDescriptor;
    }

    //
    // link queue head with setup descriptor
    //
    QueueHead->NextElementDescriptor = (PVOID)SetupDescriptor;
    QueueHead->ElementPhysical = SetupDescriptor->PhysicalAddress;

    //
    // store result
    //
    *OutQueueHead = QueueHead;
    return STATUS_SUCCESS;
}
USB_DEVICE_SPEED
CUSBRequest::GetDeviceSpeed()
{
    return m_DeviceSpeed;
}

VOID
CUSBRequest::FreeEndpointDescriptor(
     struct _UHCI_QUEUE_HEAD * OutDescriptor)
{
    PUHCI_TRANSFER_DESCRIPTOR Descriptor, NextDescriptor;
    ULONG ErrorCount;
    UCHAR DataToggle = 0;
    ULONG Index = 0;

    //
    // grab first transfer descriptor
    //
    Descriptor = (PUHCI_TRANSFER_DESCRIPTOR)OutDescriptor->NextElementDescriptor;
    while(Descriptor)
    {
        // get data toggle
        DataToggle = (Descriptor->Token >> TD_TOKEN_DATA_TOGGLE_SHIFT) & 0x01;

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
                 m_NtStatusCode = STATUS_UNSUCCESSFUL;

                 if (Descriptor->Status & TD_STATUS_ERROR_BUFFER)
                 {
                     DPRINT1("[USBUHCI] Buffer Error detected in descriptor %p Index %lu\n", Descriptor, Index);
                     m_UrbStatusCode = USBD_STATUS_DATA_BUFFER_ERROR;
                 }
                 else if (Descriptor->Status & TD_STATUS_ERROR_TIMEOUT)
                 {
                     DPRINT1("[USBUHCI] Timeout detected in descriptor %p Index %lu\n", Descriptor, Index);
                     m_UrbStatusCode = USBD_STATUS_TIMEOUT;
                 }
                 else if (Descriptor->Status & TD_STATUS_ERROR_NAK)
                 {
                     DPRINT1("[USBUHCI] Unexpected pid detected in descriptor %p Index %lu\n", Descriptor, Index);
                     m_UrbStatusCode = USBD_STATUS_UNEXPECTED_PID;
                 }
                 else if (Descriptor->Status & TD_STATUS_ERROR_BITSTUFF)
                 {
                     DPRINT1("[USBUHCI] BitStuff detected in descriptor %p Index %lu\n", Descriptor, Index);
                     m_UrbStatusCode = USBD_STATUS_BTSTUFF;
                 }
            }
            else if (Descriptor->Status & TD_STATUS_ERROR_BABBLE)
            {
                 //
                 // babble error
                 //
                 DPRINT1("[USBUHCI] Babble detected in descriptor %p Index %lu\n", Descriptor, Index);
                 m_UrbStatusCode = USBD_STATUS_BABBLE_DETECTED;
            }
            else
            {
                //
                // stall detected
                //
                DPRINT1("[USBUHCI] Stall detected Descriptor %p Index %lu\n", Descriptor, Index);
                m_UrbStatusCode = USBD_STATUS_STALL_PID;
            }
        }
        else
        {
            //
            // FIXME detect actual length
            //
            if (Descriptor->UserBuffer)
            {
                //
                // copy contents back
                //
                RtlCopyMemory(Descriptor->UserBuffer, Descriptor->BufferLogical, Descriptor->BufferSize);
            }
        }
        //
        // move to next descriptor
        //
        NextDescriptor = (PUHCI_TRANSFER_DESCRIPTOR)Descriptor->NextLogicalDescriptor;

        //
        // free endpoint descriptor
        //
        FreeDescriptor(Descriptor);

        //
        // move to next
        //
        Descriptor = NextDescriptor;
        Index++;
    }

    //
    // now free queue head
    //
    m_DmaManager->Release(OutDescriptor, sizeof(UHCI_QUEUE_HEAD));

    // is there an endpoint descriptor
    if (m_EndpointDescriptor)
    {
        // invert last data toggle
        m_EndpointDescriptor->DataToggle = (DataToggle == 0);
    }
}

VOID
CUSBRequest::CompletionCallback()
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;

    DPRINT("CUSBRequest::CompletionCallback\n");

    if (m_Irp)
    {
        //
        // set irp completion status
        //
        m_Irp->IoStatus.Status = m_NtStatusCode;

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
        Urb->UrbHeader.Status = m_UrbStatusCode;

        //
        // Check if the MDL was created
        //
        if (!Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL)
        {
            //
            // Free Mdl
            //
            IoFreeMdl(m_TransferBufferMDL);
        }

        //
        // FIXME calculate length
        //

        //
        // complete request
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


//-----------------------------------------------------------------------------------------
NTSTATUS
NTAPI
InternalCreateUSBRequest(
    PUSBREQUEST *OutRequest)
{
    PUSBREQUEST This;

    //
    // allocate requests
    //
    This = new(NonPagedPool, TAG_USBUHCI) CUSBRequest(0);
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
