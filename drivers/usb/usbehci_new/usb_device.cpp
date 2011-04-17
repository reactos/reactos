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
    virtual ULONG GetDeviceAddress();
    virtual ULONG GetPort();
    virtual USB_DEVICE_SPEED GetSpeed();
    virtual USB_DEVICE_TYPE GetType();
    virtual ULONG GetState();
    virtual void SetDeviceHandleData(PVOID Data);
    virtual NTSTATUS SetDeviceAddress(ULONG DeviceAddress);
    virtual void GetDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);
    virtual UCHAR GetConfigurationValue();
    virtual NTSTATUS SubmitUrb(PURB Urb);

    // local function
    virtual NTSTATUS CommitUrb(PURB Urb);
    virtual NTSTATUS CommitSetupPacket(PUSB_DEFAULT_PIPE_SETUP_PACKET Packet, IN ULONG BufferLength, IN OUT PVOID Buffer, IN OPTIONAL PIRP Irp, IN OPTIONAL PKEVENT pEvent);

    // constructor / destructor
    CUSBDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBDevice(){}

protected:
    LONG m_Ref;
    PHUBCONTROLLER m_HubController;
    PUSBHARDWAREDEVICE m_Device;
    PVOID m_Parent; 
    ULONG m_Port;
    ULONG m_DeviceAddress;
    PVOID m_Data;
    KSPIN_LOCK m_Lock;
    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    ULONG m_PortStatus;
    PUSBQUEUE m_Queue;
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
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;

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
    // commit setup packet
    //
    Status = CommitSetupPacket(&CtrlSetup, sizeof(USB_DEVICE_DESCRIPTOR), &m_DeviceDescriptor, 0, 0);

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CUSBDevice::Initialize CommitSetupPacket failed with %x\n", Status);
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
ULONG
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
    ULONG DeviceAddress)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;

    //
    // zero request
    //
    RtlZeroMemory(&CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    //
    // initialize request
    //
    CtrlSetup.bRequest = USB_REQUEST_SET_ADDRESS;
    CtrlSetup.wValue.W = DeviceAddress;

    //
    // set device address
    //
    Status = CommitSetupPacket(&CtrlSetup, 0, 0, 0, 0);

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
    // store device address
    //
    m_DeviceAddress = DeviceAddress;

    //
    // done
    //
    return STATUS_SUCCESS;
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
CUSBDevice::CommitUrb(
    PURB Urb)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------
NTSTATUS
CUSBDevice::SubmitUrb(
    PURB Urb)
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
    Status = CommitUrb(Urb);

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
    IN OUT PVOID Buffer,
    IN PIRP Irp,
    IN PKEVENT pEvent)
{
    NTSTATUS Status;
    PUSBREQUEST Request;
    KEVENT Event;
    BOOLEAN Wait = FALSE;

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
    Status = Request->InitializeWithSetupPacket(Packet, BufferLength, Buffer);
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
    // is a irp or event provided ?
    //
    if (Irp == NULL && pEvent == NULL)
    {
        //
        // no completion details provided, requestor wants synchronized
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        pEvent = &Event;
        Wait = TRUE;
    }

    //
    // set completion details
    //
    Status = Request->SetCompletionDetails(Irp, pEvent);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set completion details
        //
        DPRINT1("CUSBDevice::CommitSetupPacket> failed to set completion details with %x\n", Status);
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

    if (Wait)
    {
        //
        // wait for the operation to complete
        //
        KeWaitForSingleObject(pEvent, Executive, KernelMode, FALSE, NULL);
    }

    //TODO:
    // Get result code from operation
    //

    //
    // returns the result code when the operation has been finished
    //
    //Status = Request->GetResultCode();

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

