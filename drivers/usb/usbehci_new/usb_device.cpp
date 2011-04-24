/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usb_device.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbehci.h"

typedef struct _USB_ENDPOINT
{
    USB_ENDPOINT_DESCRIPTOR EndPointDescriptor;
} USB_ENDPOINT, *PUSB_ENDPOINT;

typedef struct _USB_INTERFACE
{
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USB_ENDPOINT *EndPoints;
} USB_INTERFACE, *PUSB_INTERFACE;

typedef struct _USB_CONFIGURATION
{
    USB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    USB_INTERFACE *Interfaces;
} USB_CONFIGURATION, *PUSB_CONFIGURATION;


class CUSBDevice : public IUSBDevice
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

    // IUSBDevice interface functions
    virtual NTSTATUS Initialize(IN PHUBCONTROLLER HubController, IN PUSBHARDWAREDEVICE Device, IN PVOID Parent, IN ULONG Port, IN ULONG PortStatus);
    virtual BOOLEAN IsHub();
    virtual NTSTATUS GetParent(PVOID * Parent);
    virtual UCHAR GetDeviceAddress();
    virtual ULONG GetPort();
    virtual USB_DEVICE_SPEED GetSpeed();
    virtual USB_DEVICE_TYPE GetType();
    virtual ULONG GetState();
    virtual void SetDeviceHandleData(PVOID Data);
    virtual NTSTATUS SetDeviceAddress(UCHAR DeviceAddress);
    virtual void GetDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual UCHAR GetConfigurationValue();
    virtual NTSTATUS SubmitIrp(PIRP Irp);
    virtual VOID GetConfigurationDescriptors(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer, IN ULONG BufferLength, OUT PULONG OutBufferLength);


    // local function
    virtual NTSTATUS CommitIrp(PIRP Irp);
    virtual NTSTATUS CommitSetupPacket(PUSB_DEFAULT_PIPE_SETUP_PACKET Packet, IN ULONG BufferLength, IN OUT PMDL Mdl);
    virtual NTSTATUS CreateConfigurationDescriptor(ULONG ConfigurationIndex);
    virtual NTSTATUS CreateDeviceDescriptor();

    // constructor / destructor
    CUSBDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBDevice(){}

protected:
    LONG m_Ref;
    PHUBCONTROLLER m_HubController;
    PUSBHARDWAREDEVICE m_Device;
    PVOID m_Parent; 
    ULONG m_Port;
    UCHAR m_DeviceAddress;
    PVOID m_Data;
    KSPIN_LOCK m_Lock;
    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    ULONG m_PortStatus;
    PUSBQUEUE m_Queue;
    PDMAMEMORYMANAGER m_DmaManager;

    PUSB_CONFIGURATION m_ConfigurationDescriptors;
};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CUSBDevice::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::Initialize(
    IN PHUBCONTROLLER HubController, 
    IN PUSBHARDWAREDEVICE Device, 
    IN PVOID Parent, 
    IN ULONG Port,
    IN ULONG PortStatus)
{
    NTSTATUS Status;

    //
    // initialize members
    //
    m_HubController = HubController;
    m_Device = Device;
    m_Parent = Parent;
    m_Port = Port;
    m_PortStatus = PortStatus;

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // no device address has been set yet
    //
    m_DeviceAddress = 0;

    //
    // get usb request queue
    //
    Status = m_Device->GetUSBQueue(&m_Queue);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get usb queue
        //
        DPRINT1("CUSBDevice::Initialize GetUsbQueue failed with %x\n", Status);
        return Status;
    }

    //
    // get dma manager
    //
    Status = m_Device->GetDMA(&m_DmaManager);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get dma manager
        //
        DPRINT1("CUSBDevice::Initialize GetDMA failed with %x\n", Status);
        return Status;
    }

    //
    // sanity check
    //
    PC_ASSERT(m_DmaManager);

    //
    // get device descriptor
    //
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get device descriptor
        //
        DPRINT1("CUSBDevice::Initialize Failed to get device descriptor with %x\n", Status);
        return Status;
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
BOOLEAN
CUSBDevice::IsHub()
{
    //
    // USB Standard Device Class see http://www.usb.org/developers/defined_class/#BaseClass09h
    // for details
    //
    return (m_DeviceDescriptor.bDeviceClass == 0x09 && m_DeviceDescriptor.bDeviceSubClass == 0x00);
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::GetParent(
    PVOID * Parent)
{
    //
    // returns parent
    //
    *Parent = m_Parent;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetDeviceAddress()
{
    //
    // get device address
    //
    return m_DeviceAddress;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetPort()
{
    //
    // get port to which this device is connected to
    //
    return m_Port;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_SPEED
CUSBDevice::GetSpeed()
{
    if (m_PortStatus & USB_PORT_STATUS_LOW_SPEED)
    {
        //
        // low speed device
        //
        return UsbLowSpeed;
    }
    else if (m_PortStatus & USB_PORT_STATUS_HIGH_SPEED)
    {
        //
        // high speed device
        //
        return UsbHighSpeed;
    }

    //
    // default to full speed
    //
    return UsbFullSpeed;
}

//----------------------------------------------------------------------------------------
USB_DEVICE_TYPE
CUSBDevice::GetType()
{
    //
    // device is encoded into bcdUSB
    //
    if (m_DeviceDescriptor.bcdUSB == 0x110)
    {
        //
        // USB 1.1 device
        //
        return Usb11Device;
    }
    else if (m_DeviceDescriptor.bcdUSB == 0x200)
    {
        //
        // USB 2.0 device
        //
        return Usb20Device;
    }

    DPRINT1("CUSBDevice::GetType Unknown bcdUSB Type %x\n", m_DeviceDescriptor.bcdUSB);
    PC_ASSERT(FALSE);

    return Usb11Device;
}

//----------------------------------------------------------------------------------------
ULONG
CUSBDevice::GetState()
{
    UNIMPLEMENTED
    return FALSE;
}

//----------------------------------------------------------------------------------------
void
CUSBDevice::SetDeviceHandleData(
    PVOID Data)
{
    //
    // set device data, for debugging issues
    //
    m_Data = Data;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SetDeviceAddress(
    UCHAR DeviceAddress)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    UCHAR OldAddress;
    ULONG Index;

    //
    // zero request
    //
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // initialize request
    //
    CtrlSetup.bRequest = USB_REQUEST_SET_ADDRESS;
    CtrlSetup.wValue.W = (USHORT)DeviceAddress;

    //
    // set device address
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, 0);

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set device address
        //
        DPRINT1("CUSBDevice::SetDeviceAddress> failed to set device address with %x Address %x\n", Status, DeviceAddress);
        return Status;
    }

    //
    // back up old address
    //
    OldAddress = m_DeviceAddress;

    //
    // store new device address
    //
    m_DeviceAddress = DeviceAddress;

    //
    // check that setting device address succeeded by retrieving the device descriptor
    //
    Status = CreateDeviceDescriptor();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to retrieve device descriptor
        //
        DPRINT1("CUSBbDevice::SetDeviceAddress> failed to set new device address %d, falling back to %d Error %x\n", DeviceAddress, OldAddress, Status);
        m_DeviceAddress = OldAddress;

        //
        // return error status
        //
        return Status;
    }

    //
    // sanity checks
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations);

    //
    // allocate configuration descriptor
    //
    m_ConfigurationDescriptors = (PUSB_CONFIGURATION) ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_CONFIGURATION) * m_DeviceDescriptor.bNumConfigurations, TAG_USBEHCI);

    //
    // zero configuration descriptor
    //
    RtlZeroMemory(m_ConfigurationDescriptors, sizeof(USB_CONFIGURATION) * m_DeviceDescriptor.bNumConfigurations);

    //
    // retrieve the configuration descriptors
    //
    for(Index = 0; Index < m_DeviceDescriptor.bNumConfigurations; Index++)
    {
        //
        // retrieve configuration descriptors from device
        //
        Status = CreateConfigurationDescriptor(Index);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CUSBDevice::SetDeviceAddress> failed to retrieve configuration %lu\n", Index);
            break;
        }
    }

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
void
CUSBDevice::GetDeviceDescriptor(
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    RtlMoveMemory(DeviceDescriptor, &m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
}

//----------------------------------------------------------------------------------------
UCHAR
CUSBDevice::GetConfigurationValue()
{
    UNIMPLEMENTED
    return 0x1;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitIrp(
    PIRP Irp)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue || m_DmaManager)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitUrb> no queue / dma !!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithIrp(m_DmaManager, Irp);

    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitIrp(
    PIRP Irp)
{
    KIRQL OldLevel;
    NTSTATUS Status;

    //
    // acquire device lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // commit urb
    //
    Status = CommitIrp(Irp);

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    return Status;
}
//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CommitSetupPacket(
    PUSB_DEFAULT_PIPE_SETUP_PACKET Packet,
    IN ULONG BufferLength, 
    IN OUT PMDL Mdl)
{
    NTSTATUS Status;
    PUSBREQUEST Request;

    if (!m_Queue)
    {
        //
        // no queue, wtf?
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> no queue!!!\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // build usb request
    //
    Status = m_Queue->CreateUSBRequest(&Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> CreateUSBRequest failed with %x\n", Status);
        return Status;
    }

    //
    // initialize request
    //
    Status = Request->InitializeWithSetupPacket(m_DmaManager, Packet, BufferLength, Mdl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to initialize request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed to initialize  usb request with %x\n", Status);
        Request->Release();
        return Status;
    }


    //
    // now add the request
    //
    Status = m_Queue->AddUSBRequest(Request);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed add request to queue with %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // get the result code when the operation has been finished
    //
    Request->GetResultStatus(&Status, NULL);

    //
    // release request
    //
    Request->Release();

    //
    // done
    //
    return Status;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateDeviceDescriptor()
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PMDL Mdl;
    NTSTATUS Status;

    //
    // zero descriptor
    //
    RtlZeroMemory(&m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // setup request
    //
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.HiByte = USB_DEVICE_DESCRIPTOR_TYPE;
    CtrlSetup.wLength = sizeof(USB_DEVICE_DESCRIPTOR);
    CtrlSetup.bmRequestType.B = 0x80;

    //
    // allocate mdl describing the device descriptor
    //
    Mdl = IoAllocateMdl(&m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit setup packet
    //
    Status = CommitSetupPacket(&CtrlSetup, sizeof(USB_DEVICE_DESCRIPTOR), Mdl);

    //
    // now free the mdl
    //
    IoFreeMdl(Mdl);

    //
    // done
    //
    return Status;

}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::CreateConfigurationDescriptor(
    ULONG Index)
{
    PVOID Buffer;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PMDL Mdl;
    ULONG InterfaceIndex, EndPointIndex;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndPointDescriptor;


    //
    // sanity checks
    //
    PC_ASSERT(m_ConfigurationDescriptors);

    //
    // first allocate a buffer which should be enough to store all different interfaces and endpoints
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_USBEHCI);
    if (!Buffer)
    {
        //
        // failed to allocate buffer
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build setup packet
    //
    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
    CtrlSetup.bmRequestType._BM.Reserved = 0;
    CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = 0;
    CtrlSetup.wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
    CtrlSetup.wIndex.W = 0;
    CtrlSetup.wLength = PAGE_SIZE;

    //
    // FIXME: where put configuration index?
    //

    //
    // now build MDL describing the buffer
    //
    Mdl = IoAllocateMdl(Buffer, PAGE_SIZE, FALSE, FALSE, 0);
    if (!Mdl)
    {
        //
        // failed to allocate mdl
        //
        ExFreePoolWithTag(Buffer, TAG_USBEHCI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for non paged pool
    //
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // commit packet
    //
    Status = CommitSetupPacket(&CtrlSetup, PAGE_SIZE, Mdl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to issue request, cleanup
        //
        IoFreeMdl(Mdl);
        ExFreePool(Buffer);
        return Status;
    }

    //
    // now free the mdl
    //
    IoFreeMdl(Mdl);

    //
    // get configuration descriptor
    //
    ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Buffer;

    //
    // sanity check
    //
    PC_ASSERT(ConfigurationDescriptor->bLength == sizeof(USB_CONFIGURATION_DESCRIPTOR));
    PC_ASSERT(ConfigurationDescriptor->wTotalLength <= PAGE_SIZE);
    PC_ASSERT(ConfigurationDescriptor->bNumInterfaces);

    //
    // request is complete, initialize configuration descriptor
    //
    RtlCopyMemory(&m_ConfigurationDescriptors[Index].ConfigurationDescriptor, ConfigurationDescriptor, ConfigurationDescriptor->bLength);

    //
    // now allocate interface descriptors
    //
    m_ConfigurationDescriptors[Index].Interfaces = (PUSB_INTERFACE)ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_INTERFACE) * ConfigurationDescriptor->bNumInterfaces, TAG_USBEHCI);
    if (!m_ConfigurationDescriptors[Index].Interfaces)
    {
        //
        // failed to allocate interface descriptors
        //
        ExFreePool(Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero interface descriptor
    //
    RtlZeroMemory(m_ConfigurationDescriptors[Index].Interfaces, sizeof(USB_INTERFACE) * ConfigurationDescriptor->bNumInterfaces);

    //
    // get first interface descriptor
    //
    InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(ConfigurationDescriptor + 1);

    //
    // setup interface descriptors
    //
    for(InterfaceIndex = 0; InterfaceIndex < ConfigurationDescriptor->bNumInterfaces; InterfaceIndex++)
    {
        //
        // sanity check
        //
        PC_ASSERT(InterfaceDescriptor->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));
        PC_ASSERT(InterfaceDescriptor->bNumEndpoints);

        //
        // copy current interface descriptor
        //
        RtlCopyMemory(&m_ConfigurationDescriptors[Index].Interfaces[InterfaceIndex].InterfaceDescriptor, InterfaceDescriptor, InterfaceDescriptor->bLength);

        //
        // allocate end point descriptors
        //
        m_ConfigurationDescriptors[Index].Interfaces[InterfaceIndex].EndPoints = (PUSB_ENDPOINT)ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_ENDPOINT) * InterfaceDescriptor->bNumEndpoints, TAG_USBEHCI);
        if (!m_ConfigurationDescriptors[Index].Interfaces[InterfaceIndex].EndPoints)
        {
            //
            // failed to allocate endpoint
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // zero memory
        //
        RtlZeroMemory(m_ConfigurationDescriptors[Index].Interfaces[InterfaceIndex].EndPoints, sizeof(USB_ENDPOINT) * InterfaceDescriptor->bNumEndpoints);

        //
        // initialize end point descriptors
        //
        EndPointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)(InterfaceDescriptor + 1);

        for(EndPointIndex = 0; EndPointIndex < InterfaceDescriptor->bNumEndpoints; EndPointIndex++)
        {
            //
            // sanity check
            //
            PC_ASSERT(EndPointDescriptor->bLength == sizeof(USB_ENDPOINT_DESCRIPTOR));

            //
            // copy endpoint descriptor
            //
            RtlCopyMemory(&m_ConfigurationDescriptors[Index].Interfaces[InterfaceIndex].EndPoints[EndPointIndex].EndPointDescriptor, EndPointDescriptor, EndPointDescriptor->bLength);

            //
            // move to next offset
            //
            EndPointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)((ULONG_PTR)EndPointDescriptor + EndPointDescriptor->bLength);
        }

        //
        // update interface descriptor offset
        //
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)EndPointDescriptor;
    }

    //
    // free buffer
    //
    ExFreePoolWithTag(Buffer, TAG_USBEHCI);

    //
    // done
    //
    return Status;
}
//----------------------------------------------------------------------------------------
VOID
CUSBDevice::GetConfigurationDescriptors(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptorBuffer,
    IN ULONG BufferLength,
    OUT PULONG OutBufferLength)
{
    //
    // sanity check
    //
    PC_ASSERT(BufferLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));
    PC_ASSERT(ConfigDescriptorBuffer);
    PC_ASSERT(OutBufferLength);

    //
    // FIXME: support multiple configurations
    //
    PC_ASSERT(m_DeviceDescriptor.bNumConfigurations == 1);

    //
    // copy first configuration descriptor
    //
    RtlCopyMemory(ConfigDescriptorBuffer, &m_ConfigurationDescriptors[0].ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));

    //
    // store length
    //
    *OutBufferLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
}


//----------------------------------------------------------------------------------------
NTSTATUS
CreateUSBDevice(
    PUSBDEVICE *OutDevice)
{
    CUSBDevice * This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBEHCI) CUSBDevice(0);
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
    *OutDevice = (PUSBDEVICE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

