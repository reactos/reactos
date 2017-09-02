/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_request.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbohci.h"

#define NDEBUG
#include <debug.h>

class CUSBRequest : public IOHCIRequest
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
    IMP_IOHCIREQUEST

    // local functions
    ULONG InternalGetTransferType();
    UCHAR InternalGetPidDirection();
    UCHAR STDMETHODCALLTYPE GetDeviceAddress();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();
    NTSTATUS BuildControlTransferDescriptor(POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor);
    NTSTATUS BuildBulkInterruptEndpoint(POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor);
    NTSTATUS BuildIsochronousEndpoint(POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor);
    NTSTATUS CreateGeneralTransferDescriptor(POHCI_GENERAL_TD* OutDescriptor, ULONG BufferSize);
    VOID FreeDescriptor(POHCI_GENERAL_TD Descriptor);
    NTSTATUS AllocateEndpointDescriptor(OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor);
    NTSTATUS CreateIsochronousTransferDescriptor(OUT POHCI_ISO_TD *OutDescriptor, ULONG FrameCount);
    UCHAR GetEndpointAddress();
    USHORT GetMaxPacketSize();
    VOID CheckError(struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor);
    VOID DumpEndpointDescriptor(struct _OHCI_ENDPOINT_DESCRIPTOR *Descriptor);
    NTSTATUS BuildTransferDescriptorChain(IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode, OUT POHCI_GENERAL_TD * OutFirstDescriptor, OUT POHCI_GENERAL_TD * OutLastDescriptor, OUT PULONG OutTransferBufferOffset);
    VOID InitDescriptor(IN POHCI_GENERAL_TD CurrentDescriptor, IN PVOID TransferBuffer, IN ULONG TransferBufferLength, IN UCHAR PidCode);

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
    // store endpoint descriptor
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
    // device speed
    //
    USB_DEVICE_SPEED m_DeviceSpeed;

    //
    // store urb
    //
    PURB m_Urb;

    //
    // base buffer
    //
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
        ExFreePoolWithTag(m_CompletionEvent, TAG_USBOHCI);
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
    IN OPTIONAL struct _USB_ENDPOINT* EndpointDescriptor,
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
STDMETHODCALLTYPE
CUSBRequest::InitializeWithIrp(
    IN PDMAMEMORYMANAGER DmaManager,
    IN struct IUSBDevice* Device,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

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
    m_Urb = (PURB)IoStack->Parameters.Others.Argument1;

    //
    // store irp
    //
    m_Irp = Irp;

    //
    // store speed
    //
    m_DeviceSpeed = Device->GetSpeed();

    //
    // check function type
    //
    switch (m_Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_ISOCH_TRANSFER:
        {
            //
            // there must be at least one packet
            //
            ASSERT(m_Urb->UrbIsochronousTransfer.NumberOfPackets);

            //
            // is there data to be transferred
            //
            if (m_Urb->UrbIsochronousTransfer.TransferBufferLength)
            {
                //
                // Check if there is a MDL
                //
                if (!m_Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL)
                {
                    //
                    // sanity check
                    //
                    PC_ASSERT(m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer);

                    //
                    // Create one using TransferBuffer
                    //
                    DPRINT("Creating Mdl from Urb Buffer %p Length %lu\n", m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer, m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                    m_TransferBufferMDL = IoAllocateMdl(m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer,
                                                        m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
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
                    m_TransferBufferMDL = m_Urb->UrbIsochronousTransfer.TransferBufferMDL;
                }
            }
 
            //
            // save buffer length
            //
            m_TransferBufferLength = m_Urb->UrbIsochronousTransfer.TransferBufferLength;

            //
            // Set Length Completed to 0
            //
            m_TransferBufferLengthCompleted = 0;

            //
            // get endpoint descriptor
            //
            m_EndpointDescriptor = (PUSB_ENDPOINT)m_Urb->UrbIsochronousTransfer.PipeHandle;

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
            if (m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength)
            {
                //
                // Check if there is a MDL
                //
                if (!m_Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL)
                {
                    //
                    // sanity check
                    //
                    PC_ASSERT(m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer);

                    //
                    // Create one using TransferBuffer
                    //
                    DPRINT("Creating Mdl from Urb Buffer %p Length %lu\n", m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer, m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                    m_TransferBufferMDL = IoAllocateMdl(m_Urb->UrbBulkOrInterruptTransfer.TransferBuffer,
                                                        m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
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
                    m_TransferBufferMDL = m_Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL;
                }

                //
                // save buffer length
                //
                m_TransferBufferLength = m_Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

                //
                // Set Length Completed to 0
                //
                m_TransferBufferLengthCompleted = 0;

                //
                // get endpoint descriptor
                //
                m_EndpointDescriptor = (PUSB_ENDPOINT)m_Urb->UrbBulkOrInterruptTransfer.PipeHandle;

            }
            break;
        }
        default:
            DPRINT1("URB Function: not supported %x\n", m_Urb->UrbHeader.Function);
            PC_ASSERT(FALSE);
    }

    //
    // done
    //
    return STATUS_SUCCESS;

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

USHORT
CUSBRequest::GetMaxPacketSize()
{
    if (!m_EndpointDescriptor)
    {
        if (m_DeviceSpeed == UsbLowSpeed)
        {
            //
            // control pipes use 8 bytes packets
            //
            return 8;
        }
        else
        {
            //
            // must be full speed
            //
            ASSERT(m_DeviceSpeed == UsbFullSpeed);
            return 64;
        }
    }

    ASSERT(m_Irp);
    ASSERT(m_EndpointDescriptor);

    //
    // return max packet size
    //
    return m_EndpointDescriptor->EndPointDescriptor.wMaxPacketSize;
}

UCHAR
STDMETHODCALLTYPE
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
CUSBRequest::CreateIsochronousTransferDescriptor(
    POHCI_ISO_TD* OutDescriptor, 
    ULONG FrameCount)
{
    POHCI_ISO_TD Descriptor;
    PHYSICAL_ADDRESS DescriptorAddress;
    NTSTATUS Status;

    //
    // allocate transfer descriptor
    //
    Status = m_DmaManager->Allocate(sizeof(OHCI_ISO_TD), (PVOID*)&Descriptor, &DescriptorAddress);
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
    Descriptor->Flags = OHCI_ITD_SET_FRAME_COUNT(FrameCount) | OHCI_ITD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE);// |  OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED);
    Descriptor->BufferPhysical = 0;
    Descriptor->NextPhysicalDescriptor = 0;
    Descriptor->LastPhysicalByteAddress = 0;

    //
    // software part
    //
    Descriptor->PhysicalAddress.QuadPart = DescriptorAddress.QuadPart;
    Descriptor->NextLogicalDescriptor = 0;

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
CUSBRequest::BuildIsochronousEndpoint(
    POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor)
{
    POHCI_ISO_TD FirstDescriptor = NULL, PreviousDescriptor = NULL, CurrentDescriptor = NULL;
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    ULONG Index = 0, SubIndex, NumberOfPackets, PageOffset, Page;
    NTSTATUS Status;
    PVOID Buffer;
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PHYSICAL_ADDRESS Address;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(m_Irp);

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
    ASSERT(Urb);

    //
    // allocate endpoint descriptor
    //
    Status = AllocateEndpointDescriptor(&EndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create setup descriptor
        //
        ASSERT(FALSE);
        return Status;
    }

    //
    // get buffer
    //
    Buffer = MmGetSystemAddressForMdlSafe(m_TransferBufferMDL, NormalPagePriority);
    ASSERT(Buffer);

    //
    // FIXME: support requests which spans several pages
    //
    ASSERT(ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(m_TransferBufferMDL), MmGetMdlByteCount(m_TransferBufferMDL)) <= 2);

    Status = m_DmaManager->Allocate(m_TransferBufferLength, &Buffer, &Address);
    ASSERT(Status == STATUS_SUCCESS);


    while(Index < Urb->UrbIsochronousTransfer.NumberOfPackets)
    {
        //
        // get number of packets remaining
        //
        NumberOfPackets = min(Urb->UrbIsochronousTransfer.NumberOfPackets - Index, OHCI_ITD_NOFFSET);
        //
        // allocate iso descriptor
        //
        Status = CreateIsochronousTransferDescriptor(&CurrentDescriptor, NumberOfPackets);
        if (!NT_SUCCESS(Status))
        {
            //
            // FIXME: cleanup
            // failed to allocate descriptor
            //
            ASSERT(FALSE);
            return Status;
        }

        //
        // get physical page (HACK)
        //
        *(volatile char *)Buffer;
        Page = MmGetPhysicalAddress(Buffer).LowPart;

        //
        // get page offset
        //
        PageOffset = BYTE_OFFSET(Page);

        //
        // initialize descriptor
        //
        CurrentDescriptor->BufferPhysical = Page - PageOffset;

        for(SubIndex = 0; SubIndex < NumberOfPackets; SubIndex++)
        {
            //
            // store buffer offset
            //
            CurrentDescriptor->Offset[SubIndex] = Urb->UrbIsochronousTransfer.IsoPacket[Index+SubIndex].Offset + PageOffset;
            DPRINT("Index %lu PacketOffset %lu FinalOffset %lu\n", SubIndex+Index, Urb->UrbIsochronousTransfer.IsoPacket[Index+SubIndex].Offset, CurrentDescriptor->Offset[SubIndex]);
        }

        //
        // increment packet offset
        //
        Index += NumberOfPackets;

        //
        // check if this is the last descriptor
        //
        if (Index == Urb->UrbIsochronousTransfer.NumberOfPackets)
        {
            //
            // end of transfer
            //
            CurrentDescriptor->LastPhysicalByteAddress = CurrentDescriptor->BufferPhysical + PageOffset + m_TransferBufferLength - 1;
        }
        else
        {
            //
            // use start address of next packet - 1
            //
            CurrentDescriptor->LastPhysicalByteAddress = CurrentDescriptor->BufferPhysical + PageOffset + Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset - 1;
        }

        //
        // is there a previous descriptor
        //
        if (PreviousDescriptor)
        {
            //
            // link descriptors
            //
            PreviousDescriptor->NextLogicalDescriptor = CurrentDescriptor;
            PreviousDescriptor->NextPhysicalDescriptor = CurrentDescriptor->PhysicalAddress.LowPart;
        }
        else
        {
            //
            // first descriptor
            //
            FirstDescriptor = CurrentDescriptor;
        }

        //
        // store as previous descriptor
        //
        PreviousDescriptor = CurrentDescriptor;
        DPRINT("Current Descriptor %p Logical %lx StartAddress %x EndAddress %x\n", CurrentDescriptor, CurrentDescriptor->PhysicalAddress.LowPart, CurrentDescriptor->BufferPhysical, CurrentDescriptor->LastPhysicalByteAddress);

    //
    // fire interrupt as soon transfer is finished
    //
    CurrentDescriptor->Flags |= OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);
    }

    //
    // clear interrupt mask for last transfer descriptor
    //
    CurrentDescriptor->Flags &= ~OHCI_TD_INTERRUPT_MASK;

    //
    // fire interrupt as soon transfer is finished
    //
    CurrentDescriptor->Flags |= OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);

    //
    // set isochronous type
    //
    EndpointDescriptor->Flags |= OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;

    //
    // now link descriptor to endpoint
    //
    EndpointDescriptor->HeadPhysicalDescriptor = FirstDescriptor->PhysicalAddress.LowPart;
    EndpointDescriptor->TailPhysicalDescriptor = CurrentDescriptor->PhysicalAddress.LowPart;
    EndpointDescriptor->HeadLogicalDescriptor = FirstDescriptor;

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
    // initialize descriptor
    //
    Descriptor->Flags = OHCI_ENDPOINT_SKIP;

    //
    // append device address and endpoint number
    //
    Descriptor->Flags |= OHCI_ENDPOINT_SET_DEVICE_ADDRESS(GetDeviceAddress());
    Descriptor->Flags |= OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(GetEndpointAddress());
    Descriptor->Flags |= OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(GetMaxPacketSize());

    DPRINT("Flags %x DeviceAddress %x EndpointAddress %x PacketSize %lu\n", Descriptor->Flags, GetDeviceAddress(), GetEndpointAddress(), GetMaxPacketSize());

    //
    // is there an endpoint descriptor
    //
    if (m_EndpointDescriptor)
    {
        //
        // check direction
        //
        if (USB_ENDPOINT_DIRECTION_OUT(m_EndpointDescriptor->EndPointDescriptor.bEndpointAddress))
        {
            //
            // direction out
            //
            Descriptor->Flags |= OHCI_ENDPOINT_DIRECTION_OUT;
        }
        else
        {
            //
            // direction in
            //
            Descriptor->Flags |= OHCI_ENDPOINT_DIRECTION_IN;
        }
    }
    else
    {
        //
        // get it from transfer descriptor
        //
        Descriptor->Flags |= OHCI_ENDPOINT_DIRECTION_DESCRIPTOR;
    }

    //
    // set type
    //
    if (m_DeviceSpeed == UsbFullSpeed)
    {
        //
        // device is full speed
        //
        Descriptor->Flags |= OHCI_ENDPOINT_FULL_SPEED;
    }
    else if (m_DeviceSpeed == UsbLowSpeed)
    {
        //
        // device is full speed
        //
        Descriptor->Flags |= OHCI_ENDPOINT_LOW_SPEED;
    }
    else
    {
        //
        // error
        //
        ASSERT(FALSE);
    }

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

VOID
CUSBRequest::InitDescriptor(
    IN POHCI_GENERAL_TD CurrentDescriptor,
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferLength,
    IN UCHAR PidDirection)
{
    ULONG Direction;

    if (PidDirection)
    {
        //
        // input direction
        //
        Direction = OHCI_TD_DIRECTION_PID_IN;
    }
    else
    {
        //
        // output direction
        //
        Direction = OHCI_TD_DIRECTION_PID_OUT;
    }


    //
    // initialize descriptor
    //
    CurrentDescriptor->Flags = Direction | OHCI_TD_BUFFER_ROUNDING | OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE) | OHCI_TD_TOGGLE_CARRY;

    //
    // store physical address of buffer
    //
    CurrentDescriptor->BufferPhysical = MmGetPhysicalAddress(TransferBuffer).LowPart;
    CurrentDescriptor->LastPhysicalByteAddress = CurrentDescriptor->BufferPhysical + TransferBufferLength - 1; 

    DPRINT("CurrentDescriptor %p Addr %x  TransferBufferLength %lu\n", CurrentDescriptor, CurrentDescriptor->PhysicalAddress.LowPart, TransferBufferLength);
}

NTSTATUS
CUSBRequest::BuildTransferDescriptorChain(
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferLength,
    IN UCHAR PidDirection,
    OUT POHCI_GENERAL_TD * OutFirstDescriptor,
    OUT POHCI_GENERAL_TD * OutLastDescriptor,
    OUT PULONG OutTransferBufferOffset)
{
    POHCI_GENERAL_TD FirstDescriptor = NULL, CurrentDescriptor, LastDescriptor = NULL;
    NTSTATUS Status;
    ULONG MaxLengthInPage, TransferBufferOffset  = 0;
    ULONG MaxPacketSize = 0, TransferSize, CurrentSize;

    //
    // for now use one page as maximum size
    //
    MaxPacketSize = PAGE_SIZE;

    do
    {
        //
        // allocate transfer descriptor
        //
        Status = CreateGeneralTransferDescriptor(&CurrentDescriptor, 0);
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
        // get page offset
        //
        MaxLengthInPage = PAGE_SIZE - BYTE_OFFSET(TransferBuffer);

        //
        // get minimum from current page size
        //
        CurrentSize = min(TransferSize, MaxLengthInPage);
        ASSERT(CurrentSize);

        //
        // now init the descriptor
        //
        InitDescriptor(CurrentDescriptor, 
                       (PVOID)((ULONG_PTR)TransferBuffer + TransferBufferOffset),
                       CurrentSize,
                       PidDirection);

        //
        // adjust offset
        //
        TransferBufferOffset += CurrentSize;

        //
        // is there a previous descriptor
        //
        if (LastDescriptor)
        {
            //
            // link descriptors
            //
            LastDescriptor->NextLogicalDescriptor = (PVOID)CurrentDescriptor;
            LastDescriptor->NextPhysicalDescriptor = CurrentDescriptor->PhysicalAddress.LowPart;
        }
        else
        {
            //
            // it is the first descriptor
            //
            FirstDescriptor = CurrentDescriptor;
        }

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


NTSTATUS
CUSBRequest::BuildBulkInterruptEndpoint(
    POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor)
{
    POHCI_GENERAL_TD FirstDescriptor, LastDescriptor;
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    NTSTATUS Status;
    PVOID Base;
    ULONG ChainDescriptorLength;

    //
    // sanity check
    //
    ASSERT(m_EndpointDescriptor);

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

    ASSERT(m_TransferBufferMDL);

    if (m_Base == NULL)
    {
        //
        // get buffer
        //
        m_Base = MmGetSystemAddressForMdlSafe(m_TransferBufferMDL, NormalPagePriority);
    }

    //
    // Increase the size of last transfer, 0 in case this is the first
    //
    Base = (PVOID)((ULONG_PTR)m_Base + m_TransferBufferLengthCompleted);

    //
    // sanity checks
    //
    ASSERT(m_EndpointDescriptor);
    ASSERT(Base);

    //
    // use 2 * PAGE_SIZE at max for each new request
    //
    ULONG MaxTransferLength = min(1 * PAGE_SIZE, m_TransferBufferLength - m_TransferBufferLengthCompleted);
    DPRINT("m_TransferBufferLength %lu m_TransferBufferLengthCompleted %lu DataToggle %x\n", m_TransferBufferLength, m_TransferBufferLengthCompleted, m_EndpointDescriptor->DataToggle);

    //
    // build bulk transfer descriptor chain
    //
    Status = BuildTransferDescriptorChain(Base,
                                          MaxTransferLength,
                                          InternalGetPidDirection(),
                                          &FirstDescriptor,
                                          &LastDescriptor,
                                          &ChainDescriptorLength);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build transfer descriptor chain
        //
        m_DmaManager->Release(EndpointDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));
        return Status;
    }

    //
    // move to next offset
    //
    m_TransferBufferLengthCompleted += ChainDescriptorLength;

    //
    // first descriptor has no carry bit
    //
    FirstDescriptor->Flags &= ~OHCI_TD_TOGGLE_CARRY;

    //
    // apply data toggle
    //
    FirstDescriptor->Flags |= (m_EndpointDescriptor->DataToggle ? OHCI_TD_TOGGLE_1 : OHCI_TD_TOGGLE_0);

    //
    // clear interrupt mask for last transfer descriptor
    //
    LastDescriptor->Flags &= ~OHCI_TD_INTERRUPT_MASK;


    //
    // fire interrupt as soon transfer is finished
    //
    LastDescriptor->Flags |= OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);

    //
    // now link descriptor to endpoint
    //
    EndpointDescriptor->HeadPhysicalDescriptor = FirstDescriptor->PhysicalAddress.LowPart;
    EndpointDescriptor->TailPhysicalDescriptor = (FirstDescriptor == LastDescriptor ? 0 : LastDescriptor->PhysicalAddress.LowPart);
    EndpointDescriptor->HeadLogicalDescriptor = FirstDescriptor;

    //
    // dump descriptor list
    //
    //DumpEndpointDescriptor(EndpointDescriptor);

    //
    // store result
    //
    *OutEndpointDescriptor = EndpointDescriptor;

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
CUSBRequest::DumpEndpointDescriptor(
    POHCI_ENDPOINT_DESCRIPTOR Descriptor)
{
    ULONG Count = 0;
    POHCI_GENERAL_TD GeneralDescriptor;

    DPRINT1("EndpointDescriptor %p Addr %x\n", Descriptor, Descriptor->PhysicalAddress.LowPart);
    DPRINT1("EndpointDescriptor HeadPhysicalDescriptor %x HeadLogicalDescriptor %p\n", Descriptor->HeadPhysicalDescriptor, Descriptor->HeadLogicalDescriptor);
    DPRINT1("EndpointDescriptor TailPhysicalDescriptor %x\n", Descriptor->TailPhysicalDescriptor);
    DPRINT1("EndpointDescriptor NextDescriptor %p\n", Descriptor->NextDescriptor);
    DPRINT1("EndpointDescriptor NextPhysicalEndpoint %x\n", Descriptor->NextPhysicalEndpoint);
    DPRINT1("EndpointDescriptor Flags %x\n", Descriptor->Flags);


    GeneralDescriptor = (POHCI_GENERAL_TD)Descriptor->HeadLogicalDescriptor;
    while(GeneralDescriptor)
    {
        DPRINT1("Descriptor %lu Address %p Addr %x\n", Count, GeneralDescriptor, GeneralDescriptor->PhysicalAddress);
        DPRINT1("Descriptor %lu BufferLogical %p BufferPhysical %x\n", Count, GeneralDescriptor->BufferLogical, GeneralDescriptor->BufferPhysical);
        DPRINT1("Descriptor %lu BufferSize %lu\n", Count, GeneralDescriptor->BufferSize);
        DPRINT1("Descriptor %lu LastPhysicalByteAddress %x\n", Count, GeneralDescriptor->LastPhysicalByteAddress);
        DPRINT1("Descriptor %lu Flags %x\n", Count, GeneralDescriptor->Flags);
        DPRINT1("Descriptor %lu NextLogicalDescriptor %p NextPhysicalDescriptor %x\n", Count, GeneralDescriptor->NextLogicalDescriptor, GeneralDescriptor->NextPhysicalDescriptor);

        Count++;
        GeneralDescriptor = (POHCI_GENERAL_TD)GeneralDescriptor->NextLogicalDescriptor;
    }
}

NTSTATUS
CUSBRequest::BuildControlTransferDescriptor(
    POHCI_ENDPOINT_DESCRIPTOR * OutEndpointDescriptor)
{
    POHCI_GENERAL_TD SetupDescriptor, StatusDescriptor, DataDescriptor = NULL, LastDescriptor;
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    NTSTATUS Status;

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

    //
    // initialize setup descriptor
    //
    SetupDescriptor->Flags = OHCI_TD_BUFFER_ROUNDING | OHCI_TD_DIRECTION_PID_SETUP | OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_TOGGLE_0 | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE);

    //
    // initialize status descriptor
    //
    StatusDescriptor->Flags = OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_TOGGLE_1 | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_IMMEDIATE);

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
            return Status;
        }

        //
        // initialize data descriptor
        //
        DataDescriptor->Flags = OHCI_TD_SET_CONDITION_CODE(OHCI_TD_CONDITION_NOT_ACCESSED) | OHCI_TD_SET_DELAY_INTERRUPT(OHCI_TD_INTERRUPT_NONE) | OHCI_TD_TOGGLE_CARRY | OHCI_TD_TOGGLE_1;

        //
        // setup pid direction
        //
        DataDescriptor->Flags |= InternalGetPidDirection() == TRUE ? OHCI_TD_DIRECTION_PID_IN : OHCI_TD_DIRECTION_PID_OUT;

        //
        // use short packets
        //
        DataDescriptor->Flags |= OHCI_TD_BUFFER_ROUNDING;

        //
        // store physical address of buffer
        //
        DataDescriptor->BufferPhysical = MmGetPhysicalAddress(MmGetMdlVirtualAddress(m_TransferBufferMDL)).LowPart;
        DataDescriptor->LastPhysicalByteAddress = DataDescriptor->BufferPhysical + m_TransferBufferLength - 1;

        //
        // flip status pid direction
        //
        StatusDescriptor->Flags |= InternalGetPidDirection() == TRUE ? OHCI_TD_DIRECTION_PID_OUT : OHCI_TD_DIRECTION_PID_IN;

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
        // status descriptor is always in
        //
        StatusDescriptor->Flags |= OHCI_TD_DIRECTION_PID_IN;

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
    // dump descriptor
    //
    //DumpEndpointDescriptor(EndpointDescriptor);

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
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
        case USB_ENDPOINT_TYPE_INTERRUPT:
            Status = BuildBulkInterruptEndpoint(OutDescriptor);
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            Status = BuildIsochronousEndpoint((POHCI_ENDPOINT_DESCRIPTOR*)OutDescriptor);
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

VOID
STDMETHODCALLTYPE
CUSBRequest::FreeEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor)
{
    POHCI_GENERAL_TD TransferDescriptor, NextTransferDescriptor;
    POHCI_ISO_TD IsoTransferDescriptor, IsoNextTransferDescriptor;
    ULONG Index, PacketCount;

    DPRINT("CUSBRequest::FreeEndpointDescriptor EndpointDescriptor %p Logical %x\n", OutDescriptor, OutDescriptor->PhysicalAddress.LowPart);

    //
    // check for errors
    //
    CheckError(OutDescriptor);


    if (OutDescriptor->Flags & OHCI_ENDPOINT_ISOCHRONOUS_FORMAT)
    {
        //
        // get first iso transfer descriptor
        //
        IsoTransferDescriptor = (POHCI_ISO_TD)OutDescriptor->HeadLogicalDescriptor;

        //
        // release endpoint descriptor
        //
        m_DmaManager->Release(OutDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));

        while(IsoTransferDescriptor)
        {
            //
            // get next
            //
            IsoNextTransferDescriptor = IsoTransferDescriptor->NextLogicalDescriptor;

            //
            // get packet count
            //
            PacketCount = OHCI_ITD_GET_FRAME_COUNT(IsoTransferDescriptor->Flags);

            DPRINT("CUSBRequest::FreeEndpointDescriptor Descriptor %p Logical %x Buffer Physical %x EndAddress %x PacketCount %lu\n", IsoTransferDescriptor, IsoTransferDescriptor->PhysicalAddress.LowPart, IsoTransferDescriptor->BufferPhysical, IsoTransferDescriptor->LastPhysicalByteAddress, PacketCount);

            for(Index = 0; Index < PacketCount; Index++)
            {
                DPRINT("PSW Index %lu Value %x\n", Index, IsoTransferDescriptor->Offset[Index]);
            }

            //
            // release descriptor
            //
            m_DmaManager->Release(IsoTransferDescriptor, sizeof(OHCI_ISO_TD));

            //
            // move to next
            //
            IsoTransferDescriptor = IsoNextTransferDescriptor;
        }
    }
    else
    {
        //
        // get first general transfer descriptor
        //
        TransferDescriptor = (POHCI_GENERAL_TD)OutDescriptor->HeadLogicalDescriptor;

        //
        // release endpoint descriptor
        //
        m_DmaManager->Release(OutDescriptor, sizeof(OHCI_ENDPOINT_DESCRIPTOR));

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

            DPRINT("CUSBRequest::FreeEndpointDescriptor Descriptor %p Logical %x Buffer Physical %x EndAddress %x\n", TransferDescriptor, TransferDescriptor->PhysicalAddress.LowPart, TransferDescriptor->BufferPhysical, TransferDescriptor->LastPhysicalByteAddress);

            //
            // release descriptor
            //
            m_DmaManager->Release(TransferDescriptor, sizeof(OHCI_GENERAL_TD));

            //
            // move to next
            //
            TransferDescriptor = NextTransferDescriptor;
        }
    }

}

VOID
CUSBRequest::CheckError(
    struct _OHCI_ENDPOINT_DESCRIPTOR * OutDescriptor)
{
    POHCI_GENERAL_TD TransferDescriptor;
    ULONG ConditionCode;
    PURB Urb;
    PIO_STACK_LOCATION IoStack;


    //
    // set status code
    //
    m_NtStatusCode = STATUS_SUCCESS;
    m_UrbStatusCode = USBD_STATUS_SUCCESS;


    if (OutDescriptor->Flags & OHCI_ENDPOINT_ISOCHRONOUS_FORMAT)
    {
        //
        // FIXME: handle isochronous support
        //
        ASSERT(FALSE);
    }
    else
    {
        //
        // get first general transfer descriptor
        //
        TransferDescriptor = (POHCI_GENERAL_TD)OutDescriptor->HeadLogicalDescriptor;

        if (m_EndpointDescriptor != NULL)
        {
            //
            // update data toggle
            //
            m_EndpointDescriptor->DataToggle = (OutDescriptor->HeadPhysicalDescriptor & OHCI_ENDPOINT_TOGGLE_CARRY);
        }

        while(TransferDescriptor)
        {
            //
            // get condition code
            //
            ConditionCode = OHCI_TD_GET_CONDITION_CODE(TransferDescriptor->Flags);
            if (ConditionCode != OHCI_TD_CONDITION_NO_ERROR)
            {
                //
                // FIXME status code
                //
                m_NtStatusCode = STATUS_UNSUCCESSFUL;

                switch(ConditionCode)
                {
                    case OHCI_TD_CONDITION_CRC_ERROR:
                        DPRINT1("OHCI_TD_CONDITION_CRC_ERROR detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_CRC;
                        break;
                    case OHCI_TD_CONDITION_BIT_STUFFING:
                        DPRINT1("OHCI_TD_CONDITION_BIT_STUFFING detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_BTSTUFF;
                        break;
                    case OHCI_TD_CONDITION_TOGGLE_MISMATCH:
                        DPRINT1("OHCI_TD_CONDITION_TOGGLE_MISMATCH detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_DATA_TOGGLE_MISMATCH;
                        break;
                    case OHCI_TD_CONDITION_STALL:
                        DPRINT1("OHCI_TD_CONDITION_STALL detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_STALL_PID;
                        break;
                    case OHCI_TD_CONDITION_NO_RESPONSE:
                        DPRINT1("OHCI_TD_CONDITION_NO_RESPONSE detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_DEV_NOT_RESPONDING;
                        break;
                    case OHCI_TD_CONDITION_PID_CHECK_FAILURE:
                        DPRINT1("OHCI_TD_CONDITION_PID_CHECK_FAILURE detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_PID_CHECK_FAILURE;
                        break;
                    case OHCI_TD_CONDITION_UNEXPECTED_PID:
                        DPRINT1("OHCI_TD_CONDITION_UNEXPECTED_PID detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_UNEXPECTED_PID;
                        break;
                    case OHCI_TD_CONDITION_DATA_OVERRUN:
                        DPRINT1("OHCI_TD_CONDITION_DATA_OVERRUN detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_DATA_OVERRUN;
                        break;
                    case OHCI_TD_CONDITION_DATA_UNDERRUN:
                        if (m_Irp)
                        {
                            //
                            // get current irp stack location
                            //
                            IoStack = IoGetCurrentIrpStackLocation(m_Irp);

                            //
                            // get urb
                            //
                            Urb = (PURB)IoStack->Parameters.Others.Argument1;

                            if(Urb->UrbBulkOrInterruptTransfer.TransferFlags & USBD_SHORT_TRANSFER_OK)
                            {
                                //
                                // short packets are ok
                                //
                                ASSERT(Urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER);
                                m_NtStatusCode = STATUS_SUCCESS;
                                break;
                            }
                        }
                        DPRINT1("OHCI_TD_CONDITION_DATA_UNDERRUN detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_DATA_UNDERRUN;
                        break;
                    case OHCI_TD_CONDITION_BUFFER_OVERRUN:
                        DPRINT1("OHCI_TD_CONDITION_BUFFER_OVERRUN detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_BUFFER_OVERRUN;
                        break;
                    case OHCI_TD_CONDITION_BUFFER_UNDERRUN:
                        DPRINT1("OHCI_TD_CONDITION_BUFFER_UNDERRUN detected in TransferDescriptor TransferDescriptor %p\n", TransferDescriptor);
                        m_UrbStatusCode = USBD_STATUS_BUFFER_UNDERRUN;
                        break;
                }
            }

            //
            // get next
            //
            TransferDescriptor = (POHCI_GENERAL_TD)TransferDescriptor->NextLogicalDescriptor;
        }
    }
}

VOID
STDMETHODCALLTYPE
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
