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
    virtual NTSTATUS InitializeWithSetupPacket(IN PDMAMEMORYMANAGER DmaManager, IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, IN UCHAR DeviceAddress, IN OPTIONAL PUSB_ENDPOINT EndpointDescriptor, IN OUT ULONG TransferBufferLength, IN OUT PMDL TransferBuffer);
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
    UCHAR InternalGetPidDirection();
    NTSTATUS BuildControlTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS BuildBulkTransferQueueHead(PQUEUE_HEAD * OutHead);
    NTSTATUS CreateDescriptor(PQUEUE_TRANSFER_DESCRIPTOR *OutDescriptor);
    NTSTATUS CreateQueueHead(PQUEUE_HEAD *OutQueueHead);
    UCHAR GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();
    ULONG InternalCalculateTransferLength();
    NTSTATUS BuildTransferDescriptorChain(IN PQUEUE_HEAD QueueHead, IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, IN UCHAR InitialDataToggle, OUT PQUEUE_TRANSFER_DESCRIPTOR * OutFirstDescriptor, OUT PQUEUE_TRANSFER_DESCRIPTOR * OutLastDescriptor, OUT PUCHAR OutDataToggle, OUT PULONG OutTransferBufferOffset);
    VOID InitDescriptor(IN PQUEUE_TRANSFER_DESCRIPTOR CurrentDescriptor, IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, IN UCHAR DataToggle, OUT PULONG OutDescriptorLength);
    VOID DumpQueueHead(IN PQUEUE_HEAD QueueHead);


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
        // check if the request was successfull
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
        DPRINT1("Request Cancelled\n");
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
        ASSERT(m_EndpointDescriptor == FALSE);
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
CUSBRequest::InitDescriptor(
    IN PQUEUE_TRANSFER_DESCRIPTOR CurrentDescriptor,
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferLength,
    IN UCHAR PidCode,
    IN UCHAR DataToggle,
    OUT PULONG OutDescriptorLength)
{
    ULONG Index, Length = 0, PageOffset, BufferLength;

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
        // use physical address
        //
        CurrentDescriptor->BufferPointer[Index] = MmGetPhysicalAddress(TransferBuffer).LowPart;

        //
        // Get the offset from page size
        //
        PageOffset = BYTE_OFFSET(TransferBuffer);

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
        DPRINT1("Index %lu Length %lu Buffer %p\n", Index, Length, TransferBuffer);

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
            ASSERT(FALSE);
            return Status;
        }

        DPRINT1("BuildTransferDescriptorChain TransferBufferLength %lu  TransferBufferOffset %lu\n", TransferBufferLength, TransferBufferOffset);
        //
        // now init the descriptor
        //
        InitDescriptor(CurrentDescriptor, 
                       (PVOID)((ULONG_PTR)TransferBuffer + TransferBufferOffset),
                       TransferBufferLength - TransferBufferOffset,
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
            LastDescriptor->AlternateNextPointer = CurrentDescriptor->PhysicalAddr;
            LastDescriptor->NextPointer = CurrentDescriptor->PhysicalAddr;
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
        break;
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
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // create setup descriptor
    //
    Status = CreateDescriptor(&SetupDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate transfer descriptor
        //
        ASSERT(FALSE);
        return Status;
    }

    //
    // create status descriptor
    //
    Status = CreateDescriptor(&StatusDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate transfer descriptor
        //
        ASSERT(FALSE);
        return Status;
    }

    //
    // now initialize the queue head
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();

    if (m_EndpointDescriptor)
    {
        //
        // set endpoint address and max packet length
        //
        QueueHead->EndPointCharacteristics.EndPointNumber = m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F;
        QueueHead->EndPointCharacteristics.MaximumPacketLength = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
    }

    //
    // init setup descriptor
    //
    SetupDescriptor->Token.Bits.PIDCode = PID_CODE_SETUP_TOKEN;
    SetupDescriptor->Token.Bits.TotalBytesToTransfer = sizeof(USB_DEFAULT_PIPE_SETUP_PACKET);
    SetupDescriptor->Token.Bits.DataToggle = FALSE;
    SetupDescriptor->BufferPointer[0] = (ULONG)PtrToUlong(m_DescriptorSetupPacket.LowPart);
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

        //
        // FIXME handle errors
        //
        ASSERT(Status == STATUS_SUCCESS);
        ASSERT(DescriptorChainLength == m_TransferBufferLength);

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

    DPRINT1("BuildControlTransferQueueHead done\n");
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
CUSBRequest::BuildBulkTransferQueueHead(
    PQUEUE_HEAD * OutHead)
{
#if 0
    NTSTATUS Status;
    PQUEUE_HEAD QueueHead;
    ULONG TransferDescriptorCount, Index;
    ULONG BytesAvailable, BufferIndex;
    PVOID Base;
    ULONG PageOffset, CurrentTransferBufferLength;
    PQUEUE_TRANSFER_DESCRIPTOR m_TransferDescriptors[3];

    //
    // Allocate the queue head
    //
    Status = CreateQueueHead(&QueueHead);

    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue heads
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity checks
    //
    PC_ASSERT(QueueHead);
    PC_ASSERT(m_TransferBufferLength);

    //
    // Max default of 3 descriptors
    //
    TransferDescriptorCount = 3;

    //
    // get virtual base of mdl
    //
    Base = MmGetSystemAddressForMdlSafe(m_TransferBufferMDL, NormalPagePriority);

    //
    // Increase the size of last transfer, 0 in case this is the first
    //
    Base = (PVOID)((ULONG_PTR)Base + m_TransferBufferLengthCompleted);

    PC_ASSERT(m_EndpointDescriptor);
    PC_ASSERT(Base);

    //
    // Get the offset from page size
    //
    PageOffset = BYTE_OFFSET(Base);

    //
    // PageOffset should only be > 0 if this is the  first transfer for this requests
    //
    if ((PageOffset != 0) && (m_TransferBufferLengthCompleted != 0))
    {
        ASSERT(FALSE);
    }

    //
    // Calculate the size of this transfer
    //
    if ((PageOffset != 0) && ((m_TransferBufferLength - m_TransferBufferLengthCompleted) >= (PAGE_SIZE * 4) + PageOffset))
    {
        CurrentTransferBufferLength = (PAGE_SIZE * 4) + PageOffset;
    }
    else if ((m_TransferBufferLength - m_TransferBufferLengthCompleted) >= PAGE_SIZE * 5)
    {
        CurrentTransferBufferLength = PAGE_SIZE * 5;
    }
    else
        CurrentTransferBufferLength = (m_TransferBufferLength - m_TransferBufferLengthCompleted);

    //
    // Add current transfer length to transfer length completed
    //
    m_TransferBufferLengthCompleted += CurrentTransferBufferLength;
    BytesAvailable = CurrentTransferBufferLength;
    DPRINT("CurrentTransferBufferLength %x, m_TransferBufferLengthCompleted %x\n", CurrentTransferBufferLength, m_TransferBufferLengthCompleted);

    DPRINT("EndPointAddress %x\n", m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress);
    DPRINT("EndPointDirection %x\n", USB_ENDPOINT_DIRECTION_IN(m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress));

    DPRINT("Request %p Base Address %p TransferBytesLength %lu MDL %p\n", this, Base, BytesAvailable, m_TransferBufferMDL);
    DPRINT("InternalGetPidDirection() %d EndPointAddress %x\n", InternalGetPidDirection(), m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F);
    DPRINT("Irp %p QueueHead %p\n", m_Irp, QueueHead);

    //PC_ASSERT(InternalGetPidDirection() == USB_ENDPOINT_DIRECTION_IN(m_EndpointDescriptor->bEndpointAddress));

    //
    // Allocated transfer descriptors
    //
    for (Index = 0; Index < TransferDescriptorCount; Index++)
    {
        Status = CreateDescriptor(&m_TransferDescriptors[Index]);
        if (!NT_SUCCESS(Status))
        {
            //
            // Failed to allocate transfer descriptors
            //

            //
            // Free QueueHead
            //
            FreeQueueHead(QueueHead);

            //
            // Free Descriptors
            // FIXME: Implement FreeDescriptors
            //
            return Status;
        }

        //
        // sanity check
        //
        PC_ASSERT(BytesAvailable);

        //
        // now setup transfer buffers
        //
        for(BufferIndex = 0; BufferIndex < 5; BufferIndex++)
        {
            //
            // If this is the first buffer of the first descriptor and there is a PageOffset
            //
            if ((BufferIndex == 0) && (PageOffset != 0) && (Index == 0))
            {
                //
                // use physical address
                //
                m_TransferDescriptors[Index]->BufferPointer[0] = MmGetPhysicalAddress(Base).LowPart;

                //
                // move to next page
                //
                Base = (PVOID)ROUND_TO_PAGES(Base);

                //
                // increment transfer bytes
                //
                if (CurrentTransferBufferLength > PAGE_SIZE - PageOffset)
                    m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer = PAGE_SIZE - PageOffset;
                else
                    m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer = CurrentTransferBufferLength;

                //
                // decrement available byte count
                //
                BytesAvailable -= m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer;

                DPRINT("TransferDescriptor %p BufferPointer %p BufferIndex %lu TotalBytes %lu Remaining %lu\n", m_TransferDescriptors[Index], m_TransferDescriptors[Index]->BufferPointer[BufferIndex],
                    BufferIndex, m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer, BytesAvailable);
            }
            else
            {
                //
                // the following pages always start on byte zero of each page
                //
                PC_ASSERT(((ULONG_PTR)Base & (PAGE_SIZE-1)) == 0);

                if (BytesAvailable >= PAGE_SIZE)
                {
                    //
                    // store address
                    //
                    m_TransferDescriptors[Index]->BufferPointer[BufferIndex] = MmGetPhysicalAddress(Base).LowPart;

                    //
                    // move to next page
                    //
                    Base = (PVOID)((ULONG_PTR)Base + PAGE_SIZE);

                    //
                    // increment transfer descriptor bytes
                    //
                    m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer += PAGE_SIZE;

                    //
                    // decrement available byte count
                    //
                    BytesAvailable -= PAGE_SIZE;

                    DPRINT("TransferDescriptor %p BufferPointer %p BufferIndex %lu TotalBytes %lu Remaining %lu\n", m_TransferDescriptors[Index], m_TransferDescriptors[Index]->BufferPointer[BufferIndex],
                            BufferIndex, m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer, BytesAvailable);
                }
                else
                {
                    PC_ASSERT(BytesAvailable);

                    //
                    // store address
                    //
                    m_TransferDescriptors[Index]->BufferPointer[BufferIndex] = MmGetPhysicalAddress(Base).LowPart;

                    //
                    // increment transfer descriptor bytes
                    //
                    m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer += BytesAvailable;

                    //
                    // decrement available byte count
                    //
                    BytesAvailable -= BytesAvailable;

                    //
                    // done as this is the last partial or full page
                    //
                    DPRINT("TransferDescriptor %p BufferPointer %p BufferIndex %lu TotalBytes %lu Remaining %lu\n", m_TransferDescriptors[Index], m_TransferDescriptors[Index]->BufferPointer[BufferIndex],
                            BufferIndex, m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer, BytesAvailable);

                    break;
                }
            }

            //
            // Check if all bytes have been consumed
            //
            if (BytesAvailable == 0)
                break;
        }

        //
        // store transfer bytes of descriptor
        //
        m_TransferDescriptors[Index]->TotalBytesToTransfer = m_TransferDescriptors[Index]->Token.Bits.TotalBytesToTransfer;

        //
        // Go ahead and link descriptors
        //
        if (Index > 0)
        {
            m_TransferDescriptors[Index - 1]->NextPointer = m_TransferDescriptors[Index]->PhysicalAddr;
        }

        //
        // setup direction
        //
        m_TransferDescriptors[Index]->Token.Bits.PIDCode = InternalGetPidDirection();

        //
        // FIXME: performance penality?
        //
        m_TransferDescriptors[Index]->Token.Bits.InterruptOnComplete = TRUE;

        InsertTailList(&QueueHead->TransferDescriptorListHead, &m_TransferDescriptors[Index]->DescriptorEntry);

        m_TransferDescriptors[Index]->Token.Bits.DataToggle = m_EndpointDescriptor->DataToggle;
        m_EndpointDescriptor->DataToggle = !m_EndpointDescriptor->DataToggle;

        //
        // FIXME need dead queue transfer descriptor?
        //

        //
        // Check if all bytes have been consumed
        //
        if (BytesAvailable == 0)
            break;
    }

    //
    // all bytes should have been consumed
    //
    PC_ASSERT(BytesAvailable == 0);

    //
    // Initialize the QueueHead
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();

    if (m_EndpointDescriptor)
    {
        //
        // Set endpoint address and max packet length
        //
        QueueHead->EndPointCharacteristics.EndPointNumber = m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F;
        QueueHead->EndPointCharacteristics.MaximumPacketLength = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
    }

    QueueHead->Token.Bits.DataToggle = TRUE;

    //
    // link descriptor with queue head
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
#else
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
        // failed to allocate queue heads
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity checks
    //
    PC_ASSERT(QueueHead);
    PC_ASSERT(m_TransferBufferLength);

    //
    // get virtual base of mdl
    //
    Base = MmGetSystemAddressForMdlSafe(m_TransferBufferMDL, NormalPagePriority);

    //
    // Increase the size of last transfer, 0 in case this is the first
    //
    Base = (PVOID)((ULONG_PTR)Base + m_TransferBufferLengthCompleted);

    PC_ASSERT(m_EndpointDescriptor);
    PC_ASSERT(Base);

    //
    // sanity check
    //
    ASSERT(m_EndpointDescriptor);

	DPRINT1("Before EndPoint %p DataToggle %x\n", m_EndpointDescriptor, m_EndpointDescriptor->DataToggle);

    //
    // build bulk transfer descriptor chain
    //
    Status = BuildTransferDescriptorChain(QueueHead,
                                          Base,
                                          m_TransferBufferLength - m_TransferBufferLengthCompleted,
                                          InternalGetPidDirection(),
                                          m_EndpointDescriptor->DataToggle,
                                          &FirstDescriptor,
                                          &LastDescriptor,
                                          &m_EndpointDescriptor->DataToggle,
                                          &ChainDescriptorLength);

    //
    // FIXME: handle errors
    //
    //ASSERT(ChainDescriptorLength == m_TransferBufferLength);
    DPRINT1("Afterwards: EndPoint %p DataToggle %x\n", m_EndpointDescriptor, m_EndpointDescriptor->DataToggle);

    //
    // move to next offset
    //
    m_TransferBufferLengthCompleted += ChainDescriptorLength;


    ASSERT(Status == STATUS_SUCCESS);

    //
    // init queue head
    //
    QueueHead->EndPointCharacteristics.DeviceAddress = GetDeviceAddress();
    QueueHead->EndPointCharacteristics.EndPointNumber = m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress & 0x0F;
    QueueHead->EndPointCharacteristics.MaximumPacketLength = m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
    QueueHead->NextPointer = FirstDescriptor->PhysicalAddr;


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
    DumpQueueHead(QueueHead);

    //
    // done
    //
    return STATUS_SUCCESS;
#endif
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
    QueueHead->EndPointCharacteristics.NakCountReload = 0xF;

    //
    // Get the Initial Data Toggle from the QEDT
    //
    QueueHead->EndPointCharacteristics.QEDTDataToggleControl = TRUE;

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
    PLIST_ENTRY Entry;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;

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

        //
        // add transfer count
        //
        m_TotalBytesTransferred += (Descriptor->TotalBytesToTransfer - Descriptor->Token.Bits.TotalBytesToTransfer);
        DPRINT1("TotalBytes Transferred in Descriptor %p Length %lu\n", Descriptor, Descriptor->TotalBytesToTransfer - Descriptor->Token.Bits.TotalBytesToTransfer);

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
        ASSERT(FALSE);
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

    //
    // no active descriptors found, queue head is finished
    //
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
