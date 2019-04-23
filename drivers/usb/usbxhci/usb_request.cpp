/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Extensible Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbxhci/usb_request.cpp
 * PURPOSE:     USB XHCI device driver(based on Haiku XHCI driver and ReactOS EHCI)
 * PROGRAMMERS: lesanilie@gmail
 */

#include "usbxhci.h"

#define YDEBUG
#include <debug.h>

class CUSBRequest : public IXHCIRequest
{
public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
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

    // com interfaces
    IMP_IUSBREQUEST
    IMP_IXHCIREQUEST

    // local functions
    ULONG InternalGetTransferType();
    UCHAR STDMETHODCALLTYPE GetDeviceAddress();
    NTSTATUS STDMETHODCALLTYPE InitCommandDescriptor(PCOMMAND_DESCRIPTOR CommandDescriptor);
    NTSTATUS BuildCommandInformation();
    NTSTATUS BuildSetupPacket();
    NTSTATUS BuildSetupPacketFromURB();

    CUSBRequest(IUnknown *OuterUnknown) {}
    virtual ~CUSBRequest() {}
protected:
    LONG m_Ref;
    PDMAMEMORYMANAGER m_DmaManager;                   // memory manager for allocating setup packet / command & transfer descriptors
    PIRP m_Irp;                                       // caller provided irp packet containing URB request
    ULONG m_TransferBufferLength;                     // transfer buffer length
    ULONG m_TransferBufferLengthCompleted;            //current transfer length
    ULONG m_TotalBytesTransferred;                    // Total Transfer Length
    PMDL m_TransferBufferMDL;                         // transfer buffer MDL
    PUSB_DEFAULT_PIPE_SETUP_PACKET m_SetupPacket;     // caller provided setup packet
    PKEVENT m_CompletionEvent;                        // completion event for callers who initialized request with setup packet
    UCHAR m_DeviceAddress;                            // device address for callers who initialized it with device address
    PUSB_ENDPOINT m_EndpointDescriptor;               // store end point address
    PUSB_DEFAULT_PIPE_SETUP_PACKET m_DescriptorPacket;// allocated setup packet from the DMA pool
    PHYSICAL_ADDRESS m_DescriptorSetupPacket;         // setup descriptor logical address
    NTSTATUS m_NtStatusCode;                          // stores the result of the operation
    ULONG m_UrbStatusCode;                            // URB completion status
    PVOID m_Base;                                     // buffer base address
    USB_DEVICE_SPEED m_Speed;                         // device speed
    PCOMMAND_INFORMATION m_CommandInformation;        // device command
    PTRB m_CommandTransferRequestBlock;               // command trb
    PDEVICE_INFORMATION m_DeviceInformation;          // device information
};

//
// UNKNOWN
//
NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::QueryInterface(
    IN REFIID reffid,
    OUT PVOID *Output)
{
    return STATUS_UNSUCCESSFUL;
}

//
// IMP_IUSBREQUEST
//
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
    m_DmaManager                    = DmaManager;
    m_SetupPacket                   = SetupPacket;
    m_TransferBufferLength          = TransferBufferLength;
    m_TransferBufferMDL             = TransferBuffer;
    m_DeviceAddress                 = Device->GetDeviceAddress();
    m_Speed                         = Device->GetSpeed();
    m_EndpointDescriptor            = EndpointDescriptor;
    m_TotalBytesTransferred         = 0;
    m_TransferBufferLengthCompleted = 0;

    //
    // allocate completion event
    //
    m_CompletionEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_USBXHCI);
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
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::InitializeWithCommand(
    IN PDMAMEMORYMANAGER DmaManager,
    IN PCOMMAND_INFORMATION CommandInformation,
    IN PTRB OutTransferRequestBlock)
{
    PC_ASSERT(DmaManager);
    PC_ASSERT(CommandInformation);
    PC_ASSERT(OutTransferRequestBlock);

    m_DmaManager = DmaManager;

    //
    // allocate memory for command information
    //
    m_CommandInformation = (PCOMMAND_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(COMMAND_INFORMATION), TAG_USBXHCI);
    if (!m_CommandInformation)
    {
        //
        // failed to allocate memory for command info
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // save command
    //
    RtlCopyMemory(m_CommandInformation, CommandInformation, sizeof(COMMAND_INFORMATION));

    //
    // allocate memory completion event
    //
    m_CompletionEvent = (PKEVENT)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_USBXHCI);
    if (!m_CompletionEvent)
    {
        //
        // failed to allocate memory for completion event
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize completion event
    //
    KeInitializeEvent(m_CompletionEvent, NotificationEvent, FALSE);

    //
    // save command TRB pointer
    //
    m_CommandTransferRequestBlock = OutTransferRequestBlock;

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::InitializeWithIrp(
    IN PDMAMEMORYMANAGER DmaManager,
    IN PUSBDEVICE Device,
    IN OUT PIRP Irp)
{
    UNREFERENCED_PARAMETER(DmaManager);
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Irp);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_UNSUCCESSFUL;
}

VOID
STDMETHODCALLTYPE
CUSBRequest::CompletionCallback(
    IN NTSTATUS NtStatusCode,
    IN ULONG UrbStatusCode)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;

    //
    // store completion code
    //
    m_NtStatusCode  = NtStatusCode;
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
        // ckech if a MDL was created to describe the buffer
        //
        if (!Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL)
        {
            //
            // free MDL
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
            // only control transfer for now
            //
            UNIMPLEMENTED_DBGBREAK();
        }

        DPRINT("Request %p Completing Irp %p NtStatusCode %x UrbStatusCode %x Transferred Length %lu\n", this, m_Irp, NtStatusCode, UrbStatusCode, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);

        //
        // done with this IRP
        //
        IoCompleteRequest(m_Irp, IO_NO_INCREMENT);
    }
    else
    {
        //
        // signal completion event
        //
        KeSetEvent(m_CompletionEvent, 0, FALSE);
    }
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::CreateCommandDescriptor(
    IN struct _COMMAND_DESCRIPTOR **OutCommandDescriptor)
{
    NTSTATUS            Status;
    PCOMMAND_DESCRIPTOR CommandDescriptor;
    PHYSICAL_ADDRESS    PhysicalTrbAddress;
    PVOID               VirtualTrbAddress;

    PC_ASSERT(OutCommandDescriptor);

    //
    // allocate memory for command descriptor
    //
    CommandDescriptor = (PCOMMAND_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, sizeof(COMMAND_DESCRIPTOR), TAG_USBXHCI);
    if (!CommandDescriptor)
    {
        //
        // allocation failed
        //
        DPRINT1("Failed to allocate memory for command descriptor\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate memory for command transfer request block
    //
    Status = m_DmaManager->Allocate(sizeof(TRB), &VirtualTrbAddress, &PhysicalTrbAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // allocation failed
        //
        DPRINT1("Failed to allocate memory for command TRB\n");
        ExFreePoolWithTag(CommandDescriptor, TAG_USBXHCI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize command descriptor
    //
    CommandDescriptor->VirtualTrbAddress       = (PTRB)VirtualTrbAddress;
    CommandDescriptor->PhysicalTrbAddress      = (PTRB)PhysicalTrbAddress.LowPart;
    CommandDescriptor->CompletedTrb            = NULL;
    CommandDescriptor->CommandType             = XHCI_TRB_TYPE_CMD_NOOP;
    CommandDescriptor->DequeueAddress.QuadPart = 0;
    CommandDescriptor->Request                 = NULL;

    //
    // initialize list head
    //
    InitializeListHead(&CommandDescriptor->DescriptorListEntry);

    //
    // setup out pointer
    //
    *OutCommandDescriptor = CommandDescriptor;

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::GetCommandDescriptor(
    IN struct _COMMAND_DESCRIPTOR **OutCommandDescriptor)
{
    NTSTATUS Status;
    PCOMMAND_DESCRIPTOR CommandDescriptor;

    PC_ASSERT(OutCommandDescriptor);

    //
    // create command descriptor
    //
    Status = CreateCommandDescriptor(&CommandDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT1("Failed to create command descriptor\n");
        return Status;
    }

    //
    // initialize command
    //
    Status = InitCommandDescriptor(CommandDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT1("Failed to initialize command descriptor\n");
        return Status;
    }

    //
    // set pointer to request
    //
    CommandDescriptor->Request = PVOID(this);

    //
    // set out pointer
    //
    *OutCommandDescriptor = CommandDescriptor;

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::FreeCommandDescriptor(
    IN struct _COMMAND_DESCRIPTOR *CommandDescriptor)
{
    NTSTATUS Status;

    //
    // free TRB memory
    //
    Status = m_DmaManager->Release(CommandDescriptor->VirtualTrbAddress, sizeof(TRB));
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to release memory
        //
        DPRINT1("Failed to release memory from address %x\n", CommandDescriptor->VirtualTrbAddress);
        return Status;
    }

    //
    // free command descriptor
    //
    ExFreePoolWithTag(CommandDescriptor, TAG_USBXHCI);

    //
    // free command information
    //
    ExFreePoolWithTag(m_CommandInformation, TAG_USBXHCI);

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBRequest::InitCommandDescriptor(
    IN PCOMMAND_DESCRIPTOR CommandDescriptor)
{
    NTSTATUS Status;

    //
    // build command information
    //
    Status = BuildCommandInformation();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT("Failed to build command information\n");
        return Status;
    }

    //
    // set output trb
    //
    CommandDescriptor->CompletedTrb = m_CommandTransferRequestBlock;

    //
    // build command TRB
    //
    switch (m_CommandInformation->CommandType)
    {
        case XHCI_TRB_TYPE_ENABLE_SLOT:
        {
            CommandDescriptor->VirtualTrbAddress->Field[0] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[1] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[2] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[3] = XHCI_TRB_TYPE(XHCI_TRB_TYPE_ENABLE_SLOT);
            CommandDescriptor->CommandType = XHCI_TRB_TYPE_ENABLE_SLOT;
            break;
        }
        case XHCI_TRB_TYPE_CMD_NOOP:
        {
            CommandDescriptor->VirtualTrbAddress->Field[0] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[1] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[2] = 0;
            CommandDescriptor->VirtualTrbAddress->Field[3] = XHCI_TRB_TYPE(XHCI_TRB_TYPE_CMD_NOOP);
            CommandDescriptor->CommandType = XHCI_TRB_TYPE_CMD_NOOP;
            break;
        }
        default:
            UNIMPLEMENTED_DBGBREAK();
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return Status;
}

BOOLEAN
STDMETHODCALLTYPE
CUSBRequest::IsRequestComplete()
{
    UNIMPLEMENTED_DBGBREAK();
    return FALSE;
}

ULONG
STDMETHODCALLTYPE
CUSBRequest::GetRequestTarget()
{
    PURB Urb;

    //
    // 4.5.4 USB Standard Device Request to xHCI Command Mapping
    //
    if (m_Irp)
    {
        //
        // get URB
        //
        Urb = (PURB)URB_FROM_IRP(m_Irp);

        //
        // for select interface/configuration(and set address) request is send to XHCI
        //
        if (Urb->UrbHeader.Function == URB_FUNCTION_SELECT_INTERFACE ||
            Urb->UrbHeader.Function == URB_FUNCTION_SELECT_CONFIGURATION)
        {
            return USB_TARGET_XHCI;
        }
        else
        {
            //
            // request is for device
            //
            return USB_TARGET_DEVICE;
        }
    }
    else if (!m_SetupPacket)
    {
        //
        // if setup packet is NULL, target is XHCI
        //
        return USB_TARGET_XHCI;
    }
    else if (m_SetupPacket->bRequest == USB_REQUEST_SET_ADDRESS       ||
             m_SetupPacket->bRequest == USB_REQUEST_SET_CONFIGURATION ||
             m_SetupPacket->bRequest == USB_REQUEST_SET_INTERFACE)
    {
        return USB_TARGET_XHCI;
    }

    //
    // request is for device
    //
    return USB_TARGET_DEVICE;
}

UCHAR
STDMETHODCALLTYPE
CUSBRequest::GetRequestDeviceAddress(VOID)
{
    return GetDeviceAddress();
}

VOID
STDMETHODCALLTYPE
CUSBRequest::SetRequestDeviceInformation(IN PDEVICE_INFORMATION DeviceInformation)
{
    m_DeviceInformation = DeviceInformation;
}

ULONG
STDMETHODCALLTYPE
CUSBRequest::GetTransferType()
{
    //
    // call internal implementation
    //
    return InternalGetTransferType();
}

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
CUSBRequest::BuildCommandInformation()
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // check if we need to build a command information
    //

    if (m_CommandInformation)
    {
        //
        // command information is already initialized
        //
        return STATUS_SUCCESS;
    }

    //
    // allocate memory for command information
    //
    m_CommandInformation = (PCOMMAND_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(COMMAND_INFORMATION), TAG_USBXHCI);
    if (!m_CommandInformation)
    {
        //
        // failed to allocate memory for command info
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create setup packet
    //
    Status = BuildSetupPacket();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build setup packet
        //
        DPRINT1("BuildSetupPacket failed with status %x\n", Status);
        return Status;
    }

    //
    // build command information(4.5.4 USB Standard Device Request to xHCI Command Mapping)
    //
    switch (m_DescriptorPacket->bRequest)
    {
        case USB_REQUEST_SET_ADDRESS:
        {
            m_CommandInformation->SlotId = m_DeviceInformation->SlotId;
            m_CommandInformation->InputContext.QuadPart = m_DeviceInformation->PhysicalInputContextAddress.QuadPart;
            m_CommandInformation->BlockSetRequest = FALSE;
            m_CommandInformation->CommandType = XHCI_TRB_TYPE_ADDRESS_DEVICE;
            break;
        }
        case USB_REQUEST_SET_CONFIGURATION:
        case USB_REQUEST_SET_INTERFACE:
        {
            m_CommandInformation->SlotId = m_DeviceInformation->SlotId;
            m_CommandInformation->InputContext.QuadPart = m_DeviceInformation->PhysicalInputContextAddress.QuadPart;
            m_CommandInformation->Deconfigure = FALSE;
            m_CommandInformation->CommandType = XHCI_TRB_TYPE_CONFIGURE_ENDPOINT;
            break;
        }
        default:
        {
            //
            // this setup packet dosen't map to command ringbuffer
            //
            UNIMPLEMENTED_DBGBREAK();
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    return Status;
}

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

VOID
STDMETHODCALLTYPE
CUSBRequest::GetResultStatus(
    OUT OPTIONAL NTSTATUS *NtStatusCode,
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

    //
    // free event
    //
    ExFreePoolWithTag(m_CompletionEvent, TAG_USBXHCI);
}

//
// IMP_IXHCIREQUEST
//

NTSTATUS
NTAPI
InternalCreateUSBRequest(
    PUSBREQUEST *OutRequest)
{
    PUSBREQUEST This;

    This = new(NonPagedPool, TAG_USBXHCI) CUSBRequest(0);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();
    *OutRequest = (PUSBREQUEST)This;

    return STATUS_SUCCESS;
}
