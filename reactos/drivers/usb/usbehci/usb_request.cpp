/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usb_request.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"

#define NDEBUG
#include <debug.h>

class CUSBRequest : public IEHCIRequest
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
    IMP_IUSBREQUEST
    // IEHCI Request interface functions
    IMP_IEHCIREQUEST

    // local functions
    ULONG InternalGetTransferType();
    UCHAR InternalGetPidDirection();
    NTSTATUS BuildControlTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS BuildBulkInterruptTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS STDMETHODCALLTYPE CreateDescriptor(PQUEUE_TRANSFER_DESCRIPTOR *OutDescriptor);
    NTSTATUS CreateQueueHead(PQUEUE_HEAD *OutQueueHead);
    UCHAR STDMETHODCALLTYPE GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();
    ULONG InternalCalculateTransferLength();
    NTSTATUS STDMETHODCALLTYPE BuildTransferDescriptorChain(IN PQUEUE_HEAD QueueHead, IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, IN UCHAR InitialDataToggle, OUT PQUEUE_TRANSFER_DESCRIPTOR * OutFirstDescriptor, OUT PQUEUE_TRANSFER_DESCRIPTOR * OutLastDescriptor, OUT PUCHAR OutDataToggle, OUT PULONG OutTransferBufferOffset);
    VOID STDMETHODCALLTYPE InitDescriptor(IN PQUEUE_TRANSFER_DESCRIPTOR CurrentDescriptor, IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, IN UCHAR DataToggle, OUT PULONG OutDescriptorLength);
    VOID DumpQueueHead(IN PQUEUE_HEAD QueueHead);

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
    // DMA queue head
    //
    PQUEUE_HEAD m_QueueHead;

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

    // buffer base address
    PVOID m_Base;

    // device speed
    USB_DEVICE_SPEED m_Speed;

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
        ExFreePoolWithTag(m_CompletionEvent, TAG_USBEHCI);
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
STDMETHODCALLTYPE
CUSBRequest::InitializeWithSetupPacket(
    IN PDMAMEMORYMANAGER DmaManager,
    IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    IN PUSBDEVICE Device,
    IN OPTIONAL PUSB_ENDPOINT EndpointDescriptor,
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
    m_Speed = Device->GetSpeed();
    m_EndpointDescriptor = EndpointDescriptor;
    m_TotalBytesTransferred = 0;

    //
    // Set Length Completed to 0
    //
    m_TransferBufferLengthCompleted = 0;

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
STDMETHODCALLTYPE
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
    m_Speed = Device->GetSpeed();

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
                m_EndpointDescriptor = (PUSB_ENDPOINT)Urb->UrbBulkOrInterruptTransfer.PipeHandle;

            }
            break;
        }
        default:
            DPRINT1("URB Function: not supported %x\n", Urb->UrbHeader.Function);
            //ASSERT(FALSE);
    }

    //
    // done
    //
    return STATUS_SUCCESS;

}

//----------------------------------------------------------------------------------------
VOID
STDMETHODCALLTYPE
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
        // check if the request was successful
        //
        if (!NT_SUCCESS(NtStatusCode))
        {
            //
            // set returned length to zero in case of error
            //
            Urb->UrbHeader.Length = 0;
        }
        else
        {
            //
            // calculate transfer length
            //
            Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = InternalCalculateTransferLength();
        }

        DPRINT("Request %p Completing Irp %p NtStatusCode %x UrbStatusCode %x Transferred Length %lu\n", this, m_Irp, NtStatusCode, UrbStatusCode, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);

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
STDMETHODCALLTYPE
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
        case USB_ENDPOINT_TYPE_INTERRUPT:
        case USB_ENDPOINT_TYPE_BULK:
            Status = BuildBulkInterruptTransferQueueHead(OutHead);
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
STDMETHODCALLTYPE
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
STDMETHODCALLTYPE
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
        ASSERT(m_EndpointDescriptor == NULL);
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
        ASSERT(m_DescriptorPacket);
        return (m_DescriptorPacket->bmRequestType.B >> 7);
    }
}

VOID
STDMETHODCALLTYPE
CUSBRequest::InitDescriptor(
    IN PQUEUE_TRANSFER_DESCRIPTOR CurrentDescriptor,
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferLength,
    IN UCHAR PidCode,
    IN UCHAR DataToggle,
    OUT PULONG OutDescriptorLength)
{
    ULONG Index, Length = 0, PageOffset, BufferLength;
    PHYSICAL_ADDRESS Address;

    //
    // init transfer descriptor
    //
    CurrentDescriptor->Token.Bits.PIDCode = PidCode;
    CurrentDescriptor->Token.Bits.TotalBytesToTransfer = 0;
    CurrentDescriptor->Token.Bits.DataToggle = DataToggle;

    //
    // sanity check
    //
    ASSERT(TransferBufferLength);

    //
    // store buffers
    //
    Index = 0;
    do
    {
        //
        // get address (HACK)
        //
        *(volatile char *)TransferBuffer;
        Address = MmGetPhysicalAddress(TransferBuffer);

        //
        // use physical address
        //
        CurrentDescriptor->BufferPointer[Index] = Address.LowPart;
        CurrentDescriptor->ExtendedBufferPointer[Index] = Address.HighPart;

        //
        // Get the offset from page size
        //
        PageOffset = BYTE_OFFSET(CurrentDescriptor->BufferPointer[Index]);
        if (PageOffset != 0)
        {
           //
           // move to next page
           //
           TransferBuffer = (PVOID)ROUND_TO_PAGES(TransferBuffer);
        }
        else
        {
            //
            // move to next page
            //
            TransferBuffer = (PVOID)((ULONG_PTR)TransferBuffer + PAGE_SIZE);
        }

        //
        // calculate buffer length
        //
        BufferLength = min(TransferBufferLength, PAGE_SIZE - PageOffset);

        //
        // increment transfer bytes
        //
        CurrentDescriptor->Token.Bits.TotalBytesToTransfer += BufferLength;
        CurrentDescriptor->TotalBytesToTransfer += BufferLength;
        Length += BufferLength;
        DPRINT("Index %lu TransferBufferLength %lu PageOffset %x BufferLength %lu Buffer Phy %p TransferBuffer %p\n", Index, TransferBufferLength, PageOffset, BufferLength, CurrentDescriptor->BufferPointer[Index], TransferBuffer);

        //
        // decrement available byte count
        //
        TransferBufferLength -= BufferLength;
        if (TransferBufferLength == 0)
        {
            //
            // end reached
            //
            break;
        }

        //
        // sanity check
        //
        if (Index > 1)
        {
            //
            // no equal buffers
            //
            ASSERT(CurrentDescriptor->BufferPointer[Index] != CurrentDescriptor->BufferPointer[Index-1]);
        }

        //
        // next descriptor index
        //
        Index++;
    }while(Index < 5);

    //
    // store result
    //
    *OutDescriptorLength = Length;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::BuildTransferDescriptorChain(
    IN PQUEUE_HEAD QueueHead,
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferLength,
    IN UCHAR PidCode,
    IN UCHAR InitialDataToggle,
    OUT PQUEUE_TRANSFER_DESCRIPTOR * OutFirstDescriptor,
    OUT PQUEUE_TRANSFER_DESCRIPTOR * OutLastDescriptor,
    OUT PUCHAR OutDataToggle,
    OUT PULONG OutTransferBufferOffset)
{
    PQUEUE_TRANSFER_DESCRIPTOR FirstDescriptor = NULL, CurrentDescriptor, LastDescriptor = NULL;
    NTSTATUS Status;
    ULONG DescriptorLength, TransferBufferOffset  = 0;
    ULONG MaxPacketSize = 0, TransferSize;

    //
    // is there an endpoint descriptor
    //
    if (m_EndpointDescriptor)
    {
        //
        // use endpoint packet size
        //
        MaxPacketSize = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
    }

    do
    {
        //
        // allocate transfer descriptor
        //
        Status = CreateDescriptor(&CurrentDescriptor);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to allocate transfer descriptor
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (MaxPacketSize)
        {
            //
            // transfer size is minimum available buffer or endpoint size
            //
            TransferSize = min(TransferBufferLength - TransferBufferOffset, MaxPacketSize);
        }
        else
        {
            //
            // use available buffer
            //
            TransferSize = TransferBufferLength - TransferBufferOffset;
        }

        //
        // now init the descriptor
        //
        InitDescriptor(CurrentDescriptor, 
                       (PVOID)((ULONG_PTR)TransferBuffer + TransferBufferOffset),
                       TransferSize,
                       PidCode,
                       InitialDataToggle,
                       &DescriptorLength);

        //
        // insert into queue head
        //
        InsertTailList(&QueueHead->TransferDescriptorListHead, &CurrentDescriptor->DescriptorEntry);

        //
        // adjust offset
        //
        TransferBufferOffset += DescriptorLength;

        if (LastDescriptor)
        {
            //
            // link to current descriptor
            //
            LastDescriptor->NextPointer = CurrentDescriptor->PhysicalAddr;
            LastDescriptor = CurrentDescriptor;
        }
        else
        {
            //
            // first descriptor in chain
            //
            LastDescriptor = FirstDescriptor = CurrentDescriptor;
        }

        //
        // flip data toggle
        //
        InitialDataToggle = !InitialDataToggle;

        if(TransferBufferLength == TransferBufferOffset)
        {
            //
            // end reached
            //
            break;
        }

    }while(TRUE);

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
        // store result data toggle
        //
        *OutDataToggle = InitialDataToggle;
    }

    if (OutTransferBufferOffset)
    {
        //
        // store offset
        //
        *OutTransferBufferOffset = TransferBufferOffset;
    }

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::BuildControlTransferQueueHead(
    PQUEUE_HEAD * OutHead)
{
    NTSTATUS Status;
    ULONG DescriptorChainLength;
    PQUEUE_HEAD QueueHead;
    PQUEUE_TRANSFER_DESCRIPTOR SetupDescriptor, StatusDescriptor, FirstDescriptor, LastDescriptor;

    //
    // first allocate the queue head
    //
    Status = CreateQueueHead(&QueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        DPRINT1("[EHCI] Failed to create queue head\n");
        return Status;
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
        // failed to create setup packet
        DPRINT1("[EHCI] Failed to create setup packet\n");

        // release queue head
        m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));
        return Status;
    }

    //
    // create setup descriptor
    //
    Status = CreateDescriptor(&SetupDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed to create setup transfer descriptor
        DPRINT1("[EHCI] Failed to create setup descriptor\n");

        if (m_DescriptorPacket)
        {
            // release packet descriptor
            m_DmaManager->Release(m_DescriptorPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
        }

        // release queue head
        m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));
        return Status;
    }

    //
    // create status descriptor
    //
    Status = CreateDescriptor(&StatusDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed to create status transfer descriptor
        DPRINT1("[EHCI] Failed to create status descriptor\n");

        // release setup transfer descriptor
        m_DmaManager->Release(SetupDescriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));

        if (m_DescriptorPacket)
        {
            // release packet descriptor
            m_DmaManager->Release(m_DescriptorPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
        }

        // release queue head
        m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));
        return Status;
    }

    //
    // now initialize the queue head
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();

    ASSERT(m_EndpointDescriptor == NULL);

    //
    // init setup descriptor
    //
    SetupDescriptor->Token.Bits.PIDCode = PID_CODE_SETUP_TOKEN;
    SetupDescriptor->Token.Bits.TotalBytesToTransfer = sizeof(USB_DEFAULT_PIPE_SETUP_PACKET);
    SetupDescriptor->Token.Bits.DataToggle = FALSE;
    SetupDescriptor->BufferPointer[0] = m_DescriptorSetupPacket.LowPart;
    SetupDescriptor->ExtendedBufferPointer[0] = m_DescriptorSetupPacket.HighPart;
    InsertTailList(&QueueHead->TransferDescriptorListHead, &SetupDescriptor->DescriptorEntry);


    //
    // init status descriptor
    //
    StatusDescriptor->Token.Bits.TotalBytesToTransfer = 0;
    StatusDescriptor->Token.Bits.DataToggle = TRUE;
    StatusDescriptor->Token.Bits.InterruptOnComplete = TRUE;

    //
    // is there data
    //
    if (m_TransferBufferLength)
    {
        Status = BuildTransferDescriptorChain(QueueHead,
                                              MmGetMdlVirtualAddress(m_TransferBufferMDL),
                                              m_TransferBufferLength,
                                              InternalGetPidDirection(),
                                              TRUE,
                                              &FirstDescriptor,
                                              &LastDescriptor,
                                              NULL,
                                              &DescriptorChainLength);
        if (!NT_SUCCESS(Status))
        {
            // failed to create descriptor chain
            DPRINT1("[EHCI] Failed to create descriptor chain\n");

            // release status transfer descriptor
            m_DmaManager->Release(StatusDescriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));

            // release setup transfer descriptor
            m_DmaManager->Release(SetupDescriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));

            if (m_DescriptorPacket)
            {
                // release packet descriptor
                m_DmaManager->Release(m_DescriptorPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
            }

            // release queue head
            m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));
            return Status;
        }

        if (m_TransferBufferLength != DescriptorChainLength)
        {
            DPRINT1("DescriptorChainLength %x\n", DescriptorChainLength);
            DPRINT1("m_TransferBufferLength %x\n", m_TransferBufferLength);
            ASSERT(FALSE);
        }

        //
        // now link the descriptors
        //
        SetupDescriptor->NextPointer = FirstDescriptor->PhysicalAddr;
        SetupDescriptor->AlternateNextPointer = FirstDescriptor->PhysicalAddr;
        LastDescriptor->NextPointer = StatusDescriptor->PhysicalAddr;
        LastDescriptor->AlternateNextPointer = StatusDescriptor->PhysicalAddr;


        //
        // pid code is flipped for ops with data stage
        //
        StatusDescriptor->Token.Bits.PIDCode = !InternalGetPidDirection();
    }
    else
    {
        //
        // direct link
        //
        SetupDescriptor->NextPointer = StatusDescriptor->PhysicalAddr;
        SetupDescriptor->AlternateNextPointer = StatusDescriptor->PhysicalAddr;

        //
        // retrieve result of operation
        //
        StatusDescriptor->Token.Bits.PIDCode = PID_CODE_IN_TOKEN;
    }

    //
    // insert status descriptor
    //
    InsertTailList(&QueueHead->TransferDescriptorListHead, &StatusDescriptor->DescriptorEntry);


    //
    // link transfer descriptors to queue head
    //
    QueueHead->NextPointer = SetupDescriptor->PhysicalAddr;

    //
    // store result
    //
    *OutHead = QueueHead;

    //
    // displays the current request
    //
    //DumpQueueHead(QueueHead);

    DPRINT("BuildControlTransferQueueHead done\n");
    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
CUSBRequest::DumpQueueHead(
    IN PQUEUE_HEAD QueueHead)
{
    PLIST_ENTRY Entry;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    ULONG Index = 0;

    DPRINT1("QueueHead %p Addr %x\n", QueueHead, QueueHead->PhysicalAddr);
    DPRINT1("QueueHead AlternateNextPointer %x\n", QueueHead->AlternateNextPointer);
    DPRINT1("QueueHead NextPointer %x\n", QueueHead->NextPointer);

    DPRINT1("QueueHead HubAddr %x\n", QueueHead->EndPointCharacteristics.ControlEndPointFlag);
    DPRINT1("QueueHead DeviceAddress %x\n", QueueHead->EndPointCharacteristics.DeviceAddress);
    DPRINT1("QueueHead EndPointNumber %x\n", QueueHead->EndPointCharacteristics.EndPointNumber);
    DPRINT1("QueueHead EndPointSpeed %x\n", QueueHead->EndPointCharacteristics.EndPointSpeed);
    DPRINT1("QueueHead HeadOfReclamation %x\n", QueueHead->EndPointCharacteristics.HeadOfReclamation);
    DPRINT1("QueueHead InactiveOnNextTransaction %x\n", QueueHead->EndPointCharacteristics.InactiveOnNextTransaction);
    DPRINT1("QueueHead MaximumPacketLength %x\n", QueueHead->EndPointCharacteristics.MaximumPacketLength);
    DPRINT1("QueueHead NakCountReload %x\n", QueueHead->EndPointCharacteristics.NakCountReload);
    DPRINT1("QueueHead QEDTDataToggleControl %x\n", QueueHead->EndPointCharacteristics.QEDTDataToggleControl);
    DPRINT1("QueueHead HubAddr %x\n", QueueHead->EndPointCapabilities.HubAddr);
    DPRINT1("QueueHead InterruptScheduleMask %x\n", QueueHead->EndPointCapabilities.InterruptScheduleMask);
    DPRINT1("QueueHead NumberOfTransactionPerFrame %x\n", QueueHead->EndPointCapabilities.NumberOfTransactionPerFrame);
    DPRINT1("QueueHead PortNumber %x\n", QueueHead->EndPointCapabilities.PortNumber);
    DPRINT1("QueueHead SplitCompletionMask %x\n", QueueHead->EndPointCapabilities.SplitCompletionMask);

    Entry = QueueHead->TransferDescriptorListHead.Flink;
    while(Entry != &QueueHead->TransferDescriptorListHead)
    {
        //
        // get transfer descriptor
        //
        Descriptor = (PQUEUE_TRANSFER_DESCRIPTOR)CONTAINING_RECORD(Entry, QUEUE_TRANSFER_DESCRIPTOR, DescriptorEntry);

        DPRINT1("TransferDescriptor %lu Addr %x\n", Index, Descriptor->PhysicalAddr);
        DPRINT1("TransferDescriptor %lu Next %x\n", Index, Descriptor->NextPointer);
        DPRINT1("TransferDescriptor %lu AlternateNextPointer %x\n", Index, Descriptor->AlternateNextPointer);
        DPRINT1("TransferDescriptor %lu Active %lu\n", Index, Descriptor->Token.Bits.Active);
        DPRINT1("TransferDescriptor %lu BabbleDetected %lu\n", Index, Descriptor->Token.Bits.BabbleDetected);
        DPRINT1("TransferDescriptor %lu CurrentPage %lu\n", Index, Descriptor->Token.Bits.CurrentPage);
        DPRINT1("TransferDescriptor %lu DataBufferError %lu\n", Index, Descriptor->Token.Bits.DataBufferError);
        DPRINT1("TransferDescriptor %lu DataToggle %lu\n", Index, Descriptor->Token.Bits.DataToggle);
        DPRINT1("TransferDescriptor %lu ErrorCounter %lu\n", Index, Descriptor->Token.Bits.ErrorCounter);
        DPRINT1("TransferDescriptor %lu Halted %lu\n", Index, Descriptor->Token.Bits.Halted);
        DPRINT1("TransferDescriptor %lu InterruptOnComplete %x\n", Index, Descriptor->Token.Bits.InterruptOnComplete);
        DPRINT1("TransferDescriptor %lu MissedMicroFrame %lu\n", Index, Descriptor->Token.Bits.MissedMicroFrame);
        DPRINT1("TransferDescriptor %lu PIDCode %lu\n", Index, Descriptor->Token.Bits.PIDCode);
        DPRINT1("TransferDescriptor %lu PingState %lu\n", Index, Descriptor->Token.Bits.PingState);
        DPRINT1("TransferDescriptor %lu SplitTransactionState %lu\n", Index, Descriptor->Token.Bits.SplitTransactionState);
        DPRINT1("TransferDescriptor %lu TotalBytesToTransfer %lu\n", Index, Descriptor->Token.Bits.TotalBytesToTransfer);
        DPRINT1("TransferDescriptor %lu TransactionError %lu\n", Index, Descriptor->Token.Bits.TransactionError);

        DPRINT1("TransferDescriptor %lu Buffer Pointer 0 %x\n", Index, Descriptor->BufferPointer[0]);
        DPRINT1("TransferDescriptor %lu Buffer Pointer 1 %x\n", Index, Descriptor->BufferPointer[1]);
        DPRINT1("TransferDescriptor %lu Buffer Pointer 2 %x\n", Index, Descriptor->BufferPointer[2]);
        DPRINT1("TransferDescriptor %lu Buffer Pointer 3 %x\n", Index, Descriptor->BufferPointer[3]);
        DPRINT1("TransferDescriptor %lu Buffer Pointer 4 %x\n", Index, Descriptor->BufferPointer[4]);
        Entry = Entry->Flink;
        Index++;
    }
}


//----------------------------------------------------------------------------------------
NTSTATUS
CUSBRequest::BuildBulkInterruptTransferQueueHead(
    PQUEUE_HEAD * OutHead)
{
    NTSTATUS Status;
    PQUEUE_HEAD QueueHead;
    PVOID Base;
    ULONG ChainDescriptorLength;
    PQUEUE_TRANSFER_DESCRIPTOR FirstDescriptor, LastDescriptor;

    //
    // Allocate the queue head
    //
    Status = CreateQueueHead(&QueueHead);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        DPRINT1("[EHCI] Failed to create queue head\n");
        return Status;
    }

    //
    // sanity checks
    //
    PC_ASSERT(QueueHead);
    PC_ASSERT(m_TransferBufferLength);

    if (!m_Base)
    {
        //
        // get virtual base of mdl
        //
        m_Base = MmGetSystemAddressForMdlSafe(m_TransferBufferMDL, NormalPagePriority);
    }

    //
    // Increase the size of last transfer, 0 in case this is the first
    //
    Base = (PVOID)((ULONG_PTR)m_Base + m_TransferBufferLengthCompleted);

    PC_ASSERT(m_EndpointDescriptor);
    PC_ASSERT(Base);

    //
    // sanity check
    //
    ASSERT(m_EndpointDescriptor);

    //
    // use 4 * PAGE_SIZE at max for each new request
    //
    ULONG MaxTransferLength = min(4 * PAGE_SIZE, m_TransferBufferLength - m_TransferBufferLengthCompleted);

    //
    // build bulk transfer descriptor chain
    //
    Status = BuildTransferDescriptorChain(QueueHead,
                                          Base,
                                          MaxTransferLength,
                                          InternalGetPidDirection(),
                                          m_EndpointDescriptor->DataToggle,
                                          &FirstDescriptor,
                                          &LastDescriptor,
                                          &m_EndpointDescriptor->DataToggle,
                                          &ChainDescriptorLength);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build transfer descriptor chain
        //
        DPRINT1("[EHCI] Failed to create descriptor chain\n");
        m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));
        return Status;
    }

    //
    // move to next offset
    //
    m_TransferBufferLengthCompleted += ChainDescriptorLength;

    //
    // init queue head
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();
    QueueHead->EndPointCharacteristics.EndPointNumber = m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F;
    QueueHead->EndPointCharacteristics.MaximumPacketLength = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
    QueueHead->NextPointer = FirstDescriptor->PhysicalAddr;
    QueueHead->CurrentLinkPointer = FirstDescriptor->PhysicalAddr;
    QueueHead->AlternateNextPointer = TERMINATE_POINTER;

    ASSERT(QueueHead->EndPointCharacteristics.DeviceAddress);
    ASSERT(QueueHead->EndPointCharacteristics.EndPointNumber);
    ASSERT(QueueHead->EndPointCharacteristics.MaximumPacketLength);
    ASSERT(QueueHead->NextPointer);

    //
    // interrupt on last descriptor
    //
    LastDescriptor->Token.Bits.InterruptOnComplete = TRUE;

    //
    // store result
    //
    *OutHead = QueueHead;

    //
    // dump status
    //
    //DumpQueueHead(QueueHead);

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
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
    InitializeListHead(&QueueHead->TransferDescriptorListHead);

    //
    // 1 for non high speed, 0 for high speed device
    //
    QueueHead->EndPointCharacteristics.ControlEndPointFlag = 0;
    QueueHead->EndPointCharacteristics.HeadOfReclamation = FALSE;
    QueueHead->EndPointCharacteristics.MaximumPacketLength = 64;

    //
    // Set NakCountReload to max value possible
    //
    QueueHead->EndPointCharacteristics.NakCountReload = 0x3;

    //
    // Get the Initial Data Toggle from the QEDT
    //
    QueueHead->EndPointCharacteristics.QEDTDataToggleControl = TRUE;

    //
    // FIXME: check if High Speed Device
    //
    QueueHead->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;
    QueueHead->EndPointCapabilities.NumberOfTransactionPerFrame = 0x01;
    QueueHead->Token.DWord = 0;
    QueueHead->Token.Bits.InterruptOnComplete = FALSE;

    //
    // store address
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
UCHAR
STDMETHODCALLTYPE
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
CUSBRequest::BuildSetupPacket()
{
    NTSTATUS Status;

    //
    // allocate common buffer setup packet
    //
    Status = m_DmaManager->Allocate(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET), (PVOID*)&m_DescriptorPacket, &m_DescriptorSetupPacket);
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
            UNIMPLEMENTED;
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
            m_DescriptorPacket->wLength = (USHORT)Urb->UrbControlDescriptorRequest.TransferBufferLength;
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
            UNIMPLEMENTED;
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
            UNIMPLEMENTED;
            break;
        default:
            UNIMPLEMENTED;
            break;
    }

    return Status;
}

//----------------------------------------------------------------------------------------
VOID
STDMETHODCALLTYPE
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
STDMETHODCALLTYPE
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
STDMETHODCALLTYPE
CUSBRequest::FreeQueueHead(
    IN struct _QUEUE_HEAD * QueueHead)
{
    PLIST_ENTRY Entry;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;

    //
    // sanity checks
    //
    ASSERT(m_DmaManager);
    ASSERT(QueueHead);
    ASSERT(!IsListEmpty(&QueueHead->TransferDescriptorListHead));

    do
    {
        //
        // get transfer descriptors
        //
        Entry = RemoveHeadList(&QueueHead->TransferDescriptorListHead);
        ASSERT(Entry);

        //
        // obtain descriptor from entry
        //
        Descriptor = (PQUEUE_TRANSFER_DESCRIPTOR)CONTAINING_RECORD(Entry, QUEUE_TRANSFER_DESCRIPTOR, DescriptorEntry);
        ASSERT(Descriptor);

        //
        // add transfer count
        //
        m_TotalBytesTransferred += (Descriptor->TotalBytesToTransfer - Descriptor->Token.Bits.TotalBytesToTransfer);
        DPRINT("TotalBytes Transferred in Descriptor %p Phys Addr %x TotalBytesSoftware %lu Length %lu\n", Descriptor, Descriptor->PhysicalAddr, Descriptor->TotalBytesToTransfer, Descriptor->TotalBytesToTransfer - Descriptor->Token.Bits.TotalBytesToTransfer);

        //
        // release transfer descriptors
        //
        m_DmaManager->Release(Descriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));

    }while(!IsListEmpty(&QueueHead->TransferDescriptorListHead));

    if (m_DescriptorPacket)
    {
        //
        // release packet descriptor
        //
        m_DmaManager->Release(m_DescriptorPacket, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    }

    //
    // release queue head
    //
    m_DmaManager->Release(QueueHead, sizeof(QUEUE_HEAD));

    //
    // nullify pointers
    //
    m_QueueHead = 0;
    m_DescriptorPacket = 0;
}

//-----------------------------------------------------------------------------------------
BOOLEAN
STDMETHODCALLTYPE
CUSBRequest::IsQueueHeadComplete(
    struct _QUEUE_HEAD * QueueHead)
{
    PLIST_ENTRY Entry;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;

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

    if (QueueHead->Token.Bits.Halted)
    {
        //
        // error occured
        //
        DPRINT1("Found halted queue head %p\n", QueueHead);
        DumpQueueHead(QueueHead);
        //ASSERT(FALSE);
        return TRUE;
    }

    //
    // loop list and see if there are any active descriptors
    //
    Entry = QueueHead->TransferDescriptorListHead.Flink;
    while(Entry != &QueueHead->TransferDescriptorListHead)
    {
        //
        // obtain descriptor from entry
        //
        Descriptor = (PQUEUE_TRANSFER_DESCRIPTOR)CONTAINING_RECORD(Entry, QUEUE_TRANSFER_DESCRIPTOR, DescriptorEntry);
        ASSERT(Descriptor);
        if (Descriptor->Token.Bits.Active)
        {
            //
            // descriptor is still active
            //
            return FALSE;
        }

         //
         // move to next entry
         //
         Entry = Entry->Flink;
    }

    DPRINT("QueueHead %p Addr %x is complete\n", QueueHead, QueueHead->PhysicalAddr);

    //
    // no active descriptors found, queue head is finished
    //
    return TRUE;
}

//-----------------------------------------------------------------------------------------
ULONG
CUSBRequest::InternalCalculateTransferLength()
{
    if (!m_Irp)
    {
        //
        // FIXME: get length for control request
        //
        return m_TransferBufferLength;
    }

    //
    // sanity check
    //
    ASSERT(m_EndpointDescriptor);
    if (USB_ENDPOINT_DIRECTION_IN(m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress))
    {
        //
        // bulk in request
        // HACK: Properly determine transfer length
        //
        return m_TransferBufferLength;//m_TotalBytesTransferred;
    }

    //
    // bulk out transfer
    //
    return m_TransferBufferLength;
}

USB_DEVICE_SPEED
CUSBRequest::GetSpeed()
{
    return m_Speed;
}

UCHAR
CUSBRequest::GetInterval()
{
    if (!m_EndpointDescriptor)
        return 0;

    return m_EndpointDescriptor->EndPointDescriptor.bInterval;
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
