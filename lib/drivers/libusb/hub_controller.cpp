/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Driver Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/libusb/hub_controller.cpp
 * PURPOSE:     USB Common Driver Library.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "libusb.h"

#define NDEBUG
#include <debug.h>

VOID NTAPI StatusChangeEndpointCallBack(
    PVOID Context);

class CHubController : public IHubController,
                       public IDispatchIrp
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

    // IHubController interface functions
    virtual NTSTATUS Initialize(IN PDRIVER_OBJECT DriverObject, IN PHCDCONTROLLER Controller, IN PUSBHARDWAREDEVICE Device, IN BOOLEAN IsRootHubDevice, IN ULONG DeviceAddress);
    virtual NTSTATUS GetHubControllerDeviceObject(PDEVICE_OBJECT * HubDeviceObject);
    virtual NTSTATUS GetHubControllerSymbolicLink(ULONG BufferLength, PVOID Buffer, PULONG RequiredLength);

    // IDispatchIrp interface functions
    virtual NTSTATUS HandlePnp(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    virtual NTSTATUS HandlePower(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    virtual NTSTATUS HandleDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    virtual NTSTATUS HandleSystemControl(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);

    // local functions
    NTSTATUS HandleQueryInterface(PIO_STACK_LOCATION IoStack);
    NTSTATUS SetDeviceInterface(BOOLEAN bEnable);
    NTSTATUS CreatePDO(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT * OutDeviceObject);
    PUSBHARDWAREDEVICE GetUsbHardware();
    ULONG AcquireDeviceAddress();
    VOID ReleaseDeviceAddress(ULONG DeviceAddress);
    BOOLEAN ValidateUsbDevice(PUSBDEVICE UsbDevice);
    NTSTATUS AddUsbDevice(PUSBDEVICE UsbDevice);
    NTSTATUS RemoveUsbDevice(PUSBDEVICE UsbDevice);
    VOID SetNotification(PVOID CallbackContext, PRH_INIT_CALLBACK CallbackRoutine);
    // internal ioctl routines
    NTSTATUS HandleGetDescriptor(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleGetDescriptorFromInterface(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleClassDevice(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleGetStatusFromDevice(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleSelectConfiguration(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleSelectInterface(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleClassOther(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleClassInterface(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleClassEndpoint(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleVendorDevice(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleBulkOrInterruptTransfer(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleIsochronousTransfer(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleClearStall(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleSyncResetAndClearStall(IN OUT PIRP Irp, PURB Urb);
    NTSTATUS HandleAbortPipe(IN OUT PIRP Irp, PURB Urb);

    friend VOID NTAPI StatusChangeEndpointCallBack(PVOID Context);

    // constructor / destructor
    CHubController(IUnknown *OuterUnknown){}
    virtual ~CHubController(){}

protected:
    LONG m_Ref;
    PHCDCONTROLLER m_Controller;
    PUSBHARDWAREDEVICE m_Hardware;
    BOOLEAN m_IsRootHubDevice;
    ULONG m_DeviceAddress;

    BOOLEAN m_InterfaceEnabled;
    UNICODE_STRING m_HubDeviceInterfaceString;

    PDEVICE_OBJECT m_HubControllerDeviceObject;
    PDRIVER_OBJECT m_DriverObject;

    PVOID m_HubCallbackContext;
    PRH_INIT_CALLBACK m_HubCallbackRoutine;

    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;

    KSPIN_LOCK m_Lock;
    RTL_BITMAP m_DeviceAddressBitmap;
    PULONG m_DeviceAddressBitmapBuffer;
    LIST_ENTRY m_UsbDeviceList;
    PIRP m_PendingSCEIrp;
    LPCSTR m_USBType;


    //Internal Functions
    BOOLEAN QueryStatusChangeEndpoint(PIRP Irp);
};

typedef struct
{
    LIST_ENTRY Entry;
    PUSBDEVICE Device;
}USBDEVICE_ENTRY, *PUSBDEVICE_ENTRY;

/* Lifted from Linux with slight changes */
const UCHAR ROOTHUB2_DEVICE_DESCRIPTOR [] =
{
    0x12,       /*  bLength; */
    USB_DEVICE_DESCRIPTOR_TYPE,       /*  bDescriptorType; Device */
    0x00, 0x20, /*  bcdUSB; v1.1 */
    USB_DEVICE_CLASS_HUB,       /*  bDeviceClass; HUB_CLASSCODE */
    0x01,       /*  bDeviceSubClass; */
    0x00,       /*  bDeviceProtocol; [ low/full speeds only ] */
    0x08,       /*  bMaxPacketSize0; 8 Bytes */
    /* Fill Vendor and Product in when init root hub */
    0x00, 0x00, /*  idVendor; */
    0x00, 0x00, /*  idProduct; */
    0x00, 0x00, /*  bcdDevice */
    0x00,       /*  iManufacturer; */
    0x00,       /*  iProduct; */
    0x00,       /*  iSerialNumber; */
    0x01        /*  bNumConfigurations; */

};

const USB_CONFIGURATION_DESCRIPTOR ROOTHUB2_CONFIGURATION_DESCRIPTOR =
{
    sizeof(USB_CONFIGURATION_DESCRIPTOR),
    USB_CONFIGURATION_DESCRIPTOR_TYPE,
    sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR) + sizeof(USB_ENDPOINT_DESCRIPTOR),
    1,
    1,
    0,
    0x40, /* self powered */
    0x0
};

const USB_INTERFACE_DESCRIPTOR ROOTHUB2_INTERFACE_DESCRIPTOR =
{
    sizeof(USB_INTERFACE_DESCRIPTOR),                            /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE,                               /* bDescriptorType; Interface */
    0,                                                           /* bInterfaceNumber; */
    0,                                                           /* bAlternateSetting; */
    0x1,                                                         /* bNumEndpoints; */
    0x09,                                                        /* bInterfaceClass; HUB_CLASSCODE */
    0x01,                                                        /* bInterfaceSubClass; */
    0x00,                                                        /* bInterfaceProtocol: */
    0x00,                                                        /* iInterface; */
};

const USB_ENDPOINT_DESCRIPTOR ROOTHUB2_ENDPOINT_DESCRIPTOR =
{
    sizeof(USB_ENDPOINT_DESCRIPTOR),                             /* bLength */
    USB_ENDPOINT_DESCRIPTOR_TYPE,                                /* bDescriptorType */
    0x81,                                                        /* bEndPointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,                                 /* bmAttributes */
    0x01,                                                        /* wMaxPacketSize */
    0xC                                                          /* bInterval */
};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CHubController::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}
//----------------------------------------------------------------------------------------
NTSTATUS
CHubController::Initialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PHCDCONTROLLER Controller,
    IN PUSBHARDWAREDEVICE Device,
    IN BOOLEAN IsRootHubDevice,
    IN ULONG DeviceAddress)
{
    NTSTATUS Status;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    USHORT VendorID, DeviceID;
    ULONG Dummy1;

    //
    // initialize members
    //
    m_Controller = Controller;
    m_Hardware = Device;
    m_IsRootHubDevice = IsRootHubDevice;
    m_DeviceAddress = DeviceAddress;
    m_DriverObject = DriverObject;
    m_USBType = m_Hardware->GetUSBType();
    KeInitializeSpinLock(&m_Lock);
    InitializeListHead(&m_UsbDeviceList);

    //
    // allocate device address bitmap buffer
    //
    m_DeviceAddressBitmapBuffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, 16, TAG_USBLIB);
    if (!m_DeviceAddressBitmapBuffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize device address bitmap
    //
    RtlInitializeBitMap(&m_DeviceAddressBitmap, m_DeviceAddressBitmapBuffer, 128);
    RtlClearAllBits(&m_DeviceAddressBitmap);

    //
    // create PDO
    //
    Status = CreatePDO(m_DriverObject, &m_HubControllerDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create hub device object
        //
        return Status;
    }

    //
    // get device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)m_HubControllerDeviceObject->DeviceExtension;

    //
    // initialize device extension
    //
    DeviceExtension->IsFDO = FALSE;
    DeviceExtension->IsHub = TRUE; //FIXME
    DeviceExtension->Dispatcher = PDISPATCHIRP(this);

    //
    // intialize device descriptor
    //
    C_ASSERT(sizeof(USB_DEVICE_DESCRIPTOR) == sizeof(ROOTHUB2_DEVICE_DESCRIPTOR));
    RtlMoveMemory(&m_DeviceDescriptor, ROOTHUB2_DEVICE_DESCRIPTOR, sizeof(USB_DEVICE_DESCRIPTOR));

    if (NT_SUCCESS(m_Hardware->GetDeviceDetails(&VendorID, &DeviceID, &Dummy1, &Dummy1)))
    {
        //
        // update device descriptor
        //
        m_DeviceDescriptor.idVendor = VendorID;
        m_DeviceDescriptor.idProduct = DeviceID;
        m_DeviceDescriptor.bcdUSB = 0x200; //FIXME
    }

    //
    // Set the SCE Callback that the Hardware Device will call on port status change
    //
    Device->SetStatusChangeEndpointCallBack((PVOID)StatusChangeEndpointCallBack, this);

    //
    // clear init flag
    //
    m_HubControllerDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

//
// Queries the ports to see if there has been a device connected or removed.
//
BOOLEAN
CHubController::QueryStatusChangeEndpoint(
    PIRP Irp)
{
    ULONG PortCount, PortId;
    PIO_STACK_LOCATION IoStack;
    USHORT PortStatus, PortChange;
    PURB Urb;
    PUCHAR TransferBuffer;
    UCHAR Changed = FALSE;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack);

    //
    // Get the Urb
    //
    Urb = (PURB)IoStack->Parameters.Others.Argument1;
    ASSERT(Urb);

    //
    // Get the number of ports and check each one for device connected
    //
    m_Hardware->GetDeviceDetails(NULL, NULL, &PortCount, NULL);
    DPRINT("[%s] SCE Request %p TransferBufferLength %lu Flags %x MDL %p\n", m_USBType, Urb->UrbBulkOrInterruptTransfer.TransferBuffer, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength, Urb->UrbBulkOrInterruptTransfer.TransferFlags, Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL);

    TransferBuffer = (PUCHAR)Urb->UrbBulkOrInterruptTransfer.TransferBuffer;

    //
    // Loop the ports
    //
    for (PortId = 0; PortId < PortCount; PortId++)
    {
        m_Hardware->GetPortStatus(PortId, &PortStatus, &PortChange);

        DPRINT("[%s] Port %d: Status %x, Change %x\n", m_USBType, PortId, PortStatus, PortChange);


        //
        // If there's a flag in PortChange return TRUE so the SCE Irp will be completed
        //
        if (PortChange != 0)
        {
            DPRINT1("[%s] Change state on port %d\n", m_USBType, PortId);
            // Set the value for the port number
             *TransferBuffer = 1 << ((PortId + 1) & 7);
            Changed = TRUE;
        }
    }

    return Changed;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::GetHubControllerDeviceObject(PDEVICE_OBJECT * HubDeviceObject)
{
    //
    // store controller object
    //
    *HubDeviceObject = m_HubControllerDeviceObject;

    return STATUS_SUCCESS;
}
//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::GetHubControllerSymbolicLink(
    ULONG BufferLength,
    PVOID Buffer,
    PULONG RequiredLength)
{
    if (!m_InterfaceEnabled)
    {
        //
        // device interface not yet enabled
        //
        return STATUS_UNSUCCESSFUL;
    }

    if (BufferLength < (ULONG)m_HubDeviceInterfaceString.Length - 8)
    {
        //
        // buffer too small
        // length is without '\??\'
        //
        *RequiredLength = m_HubDeviceInterfaceString.Length- 8;

        //
        // done
        //
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // copy symbolic link
    //
    RtlCopyMemory(Buffer, &m_HubDeviceInterfaceString.Buffer[4], m_HubDeviceInterfaceString.Length - 8);

    //
    // store length, length is without '\??\'
    //
    *RequiredLength = m_HubDeviceInterfaceString.Length - 8;

    //
    // done
    //
    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PPNP_BUS_INFORMATION BusInformation;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;
    ULONG Index = 0, Length;
    USHORT VendorID, DeviceID;
    ULONG HiSpeed, NumPorts;
    WCHAR Buffer[300];
    LPWSTR DeviceName;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT("[%s] HandlePnp IRP_MN_START_DEVICE\n", m_USBType);
            //
            // register device interface
            //
            Status = SetDeviceInterface(TRUE);
            break;
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            //
            // sure
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            DPRINT("[%s] HandlePnp IRP_MN_QUERY_ID Type %x\n", m_USBType, IoStack->Parameters.QueryId.IdType);

            if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
            {
                if (m_Hardware)
                {
                    //
                    // query device id
                    //
                    Status = m_Hardware->GetDeviceDetails(&VendorID, &DeviceID, &NumPorts, &HiSpeed);

                    if (HiSpeed == 0x200)
                    {
                        //
                        // USB 2.0 hub
                        //
                        swprintf(Buffer, L"USB\\ROOT_HUB20");
                    }
                    else
                    {
                        //
                        // USB 1.1 hub
                        //
                        swprintf(Buffer, L"USB\\ROOT_HUB");
                    }

                    //
                    // calculate length
                    //
                    Length = (wcslen(Buffer) + 1);

                    //
                    // allocate buffer
                    //
                    DeviceName = (LPWSTR)ExAllocatePoolWithTag(PagedPool, Length * sizeof(WCHAR), TAG_USBLIB);

                    if (!DeviceName)
                    {
                        //
                        // no memory
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    //
                    // copy device name
                    //
                    wcscpy(DeviceName, Buffer);

                    //
                    // store result
                    //
                    Irp->IoStatus.Information = (ULONG_PTR)DeviceName;
                    Status = STATUS_SUCCESS;
                    break;
                 }
                 Status = STATUS_UNSUCCESSFUL;
                 PC_ASSERT(0);
                 break;
            }

            if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
            {
                if (m_Hardware)
                {
                    //
                    // query device id
                    //
                    Status = m_Hardware->GetDeviceDetails(&VendorID, &DeviceID, &NumPorts, &HiSpeed);

                    if (!NT_SUCCESS(Status))
                    {
                         DPRINT1("[%s] HandlePnp> failed to get hardware id %x\n", m_USBType, Status);
                         VendorID = 0x8086;
                         DeviceID = 0x3A37;
                    }

                    if (HiSpeed == 0x200)
                    {
                        //
                        // USB 2.0 hub
                        //
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID%04x&PID%04x&REV0000", VendorID, DeviceID) + 1;
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20&VID%04x&PID%04x", VendorID, DeviceID) + 1;
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB20") + 1;
                    }
                    else
                    {
                        //
                        // USB 1.1 hub
                        //
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB&VID%04x&PID%04x&REV0000", VendorID, DeviceID) + 1;
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB&VID%04x&PID%04x", VendorID, DeviceID) + 1;
                        Index += swprintf(&Buffer[Index], L"USB\\ROOT_HUB") + 1;
                    }

                    Buffer[Index] = UNICODE_NULL;
                    Index++;

                    //
                    // allocate buffer
                    //
                    DeviceName = (LPWSTR)ExAllocatePoolWithTag(PagedPool, Index * sizeof(WCHAR), TAG_USBLIB);

                    if (!DeviceName)
                    {
                        //
                        // no memory
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    //
                    // copy device name
                    //
                    RtlMoveMemory(DeviceName, Buffer, Index * sizeof(WCHAR));

                    //
                    // store result
                    //
                    Irp->IoStatus.Information = (ULONG_PTR)DeviceName;
                    Status = STATUS_SUCCESS;
                    break;
                }
            }
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT("[%s] HandlePnp IRP_MN_QUERY_CAPABILITIES\n", m_USBType);

            DeviceCapabilities = (PDEVICE_CAPABILITIES)IoStack->Parameters.DeviceCapabilities.Capabilities;

            DeviceCapabilities->LockSupported = FALSE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = FALSE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;
            DeviceCapabilities->SilentInstall = FALSE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            DeviceCapabilities->Address = 0;
            DeviceCapabilities->UINumber = 0;
            DeviceCapabilities->DeviceD2 = 1;

            /* FIXME */
            DeviceCapabilities->HardwareDisabled = FALSE;
            DeviceCapabilities->NoDisplayInUI = FALSE;
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0;
            for (Index = 1; Index < PowerSystemMaximum; Index++)
                DeviceCapabilities->DeviceState[Index] = PowerDeviceD3;
            DeviceCapabilities->DeviceWake = PowerDeviceUnspecified;
            DeviceCapabilities->D1Latency = 0;
            DeviceCapabilities->D2Latency = 0;
            DeviceCapabilities->D3Latency = 0;

            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT("[%s] HandlePnp IRP_MN_QUERY_INTERFACE\n", m_USBType);

            //
            // handle device interface requests
            //
            Status = HandleQueryInterface(IoStack);
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT("[%s] HandlePnp IRP_MN_REMOVE_DEVICE\n", m_USBType);

            //
            // deactivate device interface for BUS PDO
            //
            SetDeviceInterface(FALSE);

            //
            // complete the request first
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            //
            // now delete device
            //
            IoDeleteDevice(m_HubControllerDeviceObject);

            //
            // nullify pointer
            //
            m_HubControllerDeviceObject = 0;

            //
            // done
            //
            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DPRINT("[%s] HandlePnp IRP_MN_QUERY_DEVICE_RELATIONS Type %x\n", m_USBType, IoStack->Parameters.QueryDeviceRelations.Type);

            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
            {
                //
                // allocate device relations
                //
                DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), TAG_USBLIB);
                if (!DeviceRelations)
                {
                    //
                    // no memory
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                //
                // initialize device relations
                //
                DeviceRelations->Count = 1;
                DeviceRelations->Objects[0] = DeviceObject;
                ObReferenceObject(DeviceObject);

                //
                // done
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
            }
            else
            {
                //
                // not handled
                //
                Status = Irp->IoStatus.Status;
            }
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            DPRINT("[%s] HandlePnp IRP_MN_QUERY_BUS_INFORMATION\n", m_USBType);

            //
            // allocate buffer for bus information
            //
            BusInformation = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
            if (BusInformation)
            {
                //
                // copy BUS guid
                //
                RtlMoveMemory(&BusInformation->BusTypeGuid, &GUID_BUS_TYPE_USB, sizeof(GUID));

                //
                // set bus type
                //
                BusInformation->LegacyBusType = PNPBus;
                BusInformation->BusNumber = 0;

                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
            }
            else
            {
                //
                // no memory
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;
        }
        case IRP_MN_STOP_DEVICE:
        {
            DPRINT("[%s] HandlePnp IRP_MN_STOP_DEVICE\n", m_USBType);
            //
            // stop device
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_SURPRISE_REMOVAL:
        {
            DPRINT("[%s] HandlePnp IRP_MN_SURPRISE_REMOVAL\n", m_USBType);
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            //
            // ignore request with default status
            //
            Status = Irp->IoStatus.Status;
            break;
        }
    }

    //
    // complete request
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandlePower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    NTSTATUS Status;
    Status = Irp->IoStatus.Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    NTSTATUS Status;
    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleIsochronousTransfer(
    IN OUT PIRP Irp,
    PURB Urb)
{
    PUSBDEVICE UsbDevice;
    PUSB_ENDPOINT_DESCRIPTOR EndPointDesc = NULL;

    //
    // Check PipeHandle to determine if this is a Bulk or Interrupt Transfer Request
    //
    EndPointDesc = (PUSB_ENDPOINT_DESCRIPTOR)Urb->UrbIsochronousTransfer.PipeHandle;

    if (!EndPointDesc)
    {
        DPRINT1("[%s] Error No EndpointDesc\n", m_USBType);
        Urb->UrbIsochronousTransfer.Hdr.Status = USBD_STATUS_INVALID_PIPE_HANDLE;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // sanity checks
    //
    ASSERT(EndPointDesc);
    DPRINT("[%s] HandleIsochronousTransfer EndPointDesc %p Address %x bmAttributes %x\n", m_USBType, EndPointDesc, EndPointDesc->bEndpointAddress, EndPointDesc->bmAttributes);
    ASSERT((EndPointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleIsochronousTransfer invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

    return UsbDevice->SubmitIrp(Irp);
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleBulkOrInterruptTransfer(
    IN OUT PIRP Irp,
    PURB Urb)
{
    PUSBDEVICE UsbDevice;
    PUSB_ENDPOINT EndPointDesc = NULL;
    //
    // First check if the request is for the Status Change Endpoint
    //

    //
    // Is the Request for the root hub
    //
    if (Urb->UrbHeader.UsbdDeviceHandle == PVOID(this)  || Urb->UrbHeader.UsbdDeviceHandle == NULL)
    {
        ASSERT(m_PendingSCEIrp == NULL);
        if (QueryStatusChangeEndpoint(Irp))
        {
            //
            // We've seen a change already, so return immediately
            //
            return STATUS_SUCCESS;
        }

        //
        // Else pend the IRP, to be completed when a device connects or disconnects.
        //
        DPRINT("[%s] Pending SCE Irp\n", m_USBType);
        m_PendingSCEIrp = Irp;
        IoMarkIrpPending(Irp);
        return STATUS_PENDING;
    }

    //
    // Check PipeHandle to determine if this is a Bulk or Interrupt Transfer Request
    //
    EndPointDesc = (PUSB_ENDPOINT)Urb->UrbBulkOrInterruptTransfer.PipeHandle;

    //
    // sanity checks
    //
    ASSERT(EndPointDesc);
    ASSERT((EndPointDesc->EndPointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK || (EndPointDesc->EndPointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleBulkOrInterruptTransfer invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);
    return UsbDevice->SubmitIrp(Irp);
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleClassOther(
    IN OUT PIRP Irp,
    PURB Urb)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    USHORT PortStatus = 0, PortChange = 0;
    PUSHORT Buffer;
    ULONG NumPort;
    ULONG PortId;

    DPRINT("[%s] HandleClassOther> Request %x Value %x\n", m_USBType, Urb->UrbControlVendorClassRequest.Request, Urb->UrbControlVendorClassRequest.Value);

    //
    // get number of ports available
    //
    Status = m_Hardware->GetDeviceDetails(NULL, NULL, &NumPort, NULL);
    PC_ASSERT(Status == STATUS_SUCCESS);

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbControlVendorClassRequest.Index - 1 < (USHORT)NumPort);

    //
    // port range reported start from 1 -n
    // convert back port id so it matches the hardware
    //
    PortId = Urb->UrbControlVendorClassRequest.Index - 1;

    //
    // check request code
    //
    switch(Urb->UrbControlVendorClassRequest.Request)
    {
        case USB_REQUEST_GET_STATUS:
        {
            //
            // sanity check
            //
            PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBufferLength == sizeof(USHORT) * 2);
            PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer);

            //
            // get port status
            //
            Status = m_Hardware->GetPortStatus(PortId, &PortStatus, &PortChange);

            if (NT_SUCCESS(Status))
            {
                //
                // request contains buffer of 2 ushort which are used from submitting port status and port change status
                //
                DPRINT("[%s] PortId %x PortStatus %x PortChange %x\n", m_USBType, PortId, PortStatus, PortChange);
                Buffer = (PUSHORT)Urb->UrbControlVendorClassRequest.TransferBuffer;

                //
                // store status, then port change
                //
                *Buffer = PortStatus;
                Buffer++;
                *Buffer = PortChange;
            }

            //
            // done
            //
            break;
        }
        case USB_REQUEST_CLEAR_FEATURE:
        {
            switch (Urb->UrbControlVendorClassRequest.Value)
            {
                case C_PORT_CONNECTION:
                    Status = m_Hardware->ClearPortStatus(PortId, C_PORT_CONNECTION);
                    break;
                case C_PORT_RESET:
                    Status = m_Hardware->ClearPortStatus(PortId, C_PORT_RESET);
                    break;
                default:
                    DPRINT("[%s] Unknown Value for Clear Feature %x \n", m_USBType, Urb->UrbControlVendorClassRequest.Value);
                    break;
           }

            break;
        }
        case USB_REQUEST_SET_FEATURE:
        {
            //
            // request set feature
            //
            switch(Urb->UrbControlVendorClassRequest.Value)
            {
                case PORT_ENABLE:
                {
                    //
                    // port enable is a no-op for EHCI
                    //
                    Status = STATUS_SUCCESS;
                    break;
                }

                case PORT_SUSPEND:
                {
                    //
                    // set suspend port feature
                    //
                    Status = m_Hardware->SetPortFeature(PortId, PORT_SUSPEND);
                    break;
                }
                case PORT_POWER:
                {
                    //
                    // set power feature on port
                    //
                    Status = m_Hardware->SetPortFeature(PortId, PORT_POWER);
                    break;
                }

                case PORT_RESET:
                {
                    //
                    // reset port feature
                    //
                    Status = m_Hardware->SetPortFeature(PortId, PORT_RESET);
                    PC_ASSERT(Status == STATUS_SUCCESS);
                    break;
                }
                default:
                    DPRINT1("[%s] Unsupported request id %x\n", m_USBType, Urb->UrbControlVendorClassRequest.Value);
                    PC_ASSERT(FALSE);
            }
            break;
        }
        default:
            DPRINT1("[%s] HandleClassOther Unknown request code %x\n", m_USBType, Urb->UrbControlVendorClassRequest.Request);
            PC_ASSERT(0);
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleSelectConfiguration(
    IN OUT PIRP Irp,
    PURB Urb)
{
    PUSBDEVICE UsbDevice;
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;
    NTSTATUS Status;

    //
    // is the request for the Root Hub
    //
    if (Urb->UrbHeader.UsbdDeviceHandle == NULL)
    {
        //
        // FIXME: support setting device to unconfigured state
        //
        PC_ASSERT(Urb->UrbSelectConfiguration.ConfigurationDescriptor);

        //
        // set device handle
        //
        Urb->UrbSelectConfiguration.ConfigurationHandle = (PVOID)&ROOTHUB2_CONFIGURATION_DESCRIPTOR;

        //
        // copy interface info
        //
        InterfaceInfo = &Urb->UrbSelectConfiguration.Interface;

        InterfaceInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)&ROOTHUB2_INTERFACE_DESCRIPTOR;
        InterfaceInfo->Class = ROOTHUB2_INTERFACE_DESCRIPTOR.bInterfaceClass;
        InterfaceInfo->SubClass = ROOTHUB2_INTERFACE_DESCRIPTOR.bInterfaceSubClass;
        InterfaceInfo->Protocol = ROOTHUB2_INTERFACE_DESCRIPTOR.bInterfaceProtocol;
        InterfaceInfo->Reserved = 0;

        //
        // sanity check
        //
        PC_ASSERT(InterfaceInfo->NumberOfPipes == 1);

        //
        // copy pipe info
        //
        InterfaceInfo->Pipes[0].MaximumPacketSize = ROOTHUB2_ENDPOINT_DESCRIPTOR.wMaxPacketSize;
        InterfaceInfo->Pipes[0].EndpointAddress = ROOTHUB2_ENDPOINT_DESCRIPTOR.bEndpointAddress;
        InterfaceInfo->Pipes[0].Interval = ROOTHUB2_ENDPOINT_DESCRIPTOR.bInterval;
        InterfaceInfo->Pipes[0].PipeType = (USBD_PIPE_TYPE)(ROOTHUB2_ENDPOINT_DESCRIPTOR.bmAttributes & USB_ENDPOINT_TYPE_MASK);
        InterfaceInfo->Pipes[0].PipeHandle = (PVOID)&ROOTHUB2_ENDPOINT_DESCRIPTOR;

        return STATUS_SUCCESS;
    }
    else
    {
        //
        // check if this is a valid usb device handle
        //
        if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
        {
            DPRINT1("[%s] HandleSelectConfiguration invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

            //
            // invalid device handle
            //
            return STATUS_DEVICE_NOT_CONNECTED;
        }

        //
        // get device
        //
        UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

        //
        // select configuration
        //
        Status = UsbDevice->SelectConfiguration(Urb->UrbSelectConfiguration.ConfigurationDescriptor, &Urb->UrbSelectConfiguration.Interface, &Urb->UrbSelectConfiguration.ConfigurationHandle);
        if (NT_SUCCESS(Status))
        {
            // successfully configured device
            Urb->UrbSelectConfiguration.Hdr.Status = USBD_STATUS_SUCCESS;
        }
        return Status;
    }
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleSelectInterface(
    IN OUT PIRP Irp,
    PURB Urb)
{
    PUSBDEVICE UsbDevice;

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbSelectInterface.ConfigurationHandle);

    //
    // is the request for the Root Hub
    //
    if (Urb->UrbHeader.UsbdDeviceHandle == NULL)
    {
        //
        // no op for root hub
        //
        return STATUS_SUCCESS;
    }
    else
    {
        //
        // check if this is a valid usb device handle
        //
        if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
        {
            DPRINT1("[%s] HandleSelectInterface invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

            //
            // invalid device handle
            //
            return STATUS_DEVICE_NOT_CONNECTED;
        }

        //
        // get device
        //
        UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

        //
        // select interface
        //
        return UsbDevice->SelectInterface(Urb->UrbSelectInterface.ConfigurationHandle, &Urb->UrbSelectInterface.Interface);
    }
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleGetStatusFromDevice(
    IN OUT PIRP Irp,
    PURB Urb)
{
    PUSHORT DeviceStatus;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSBDEVICE UsbDevice;

    //
    // sanity checks
    //
    PC_ASSERT(Urb->UrbControlGetStatusRequest.TransferBufferLength >= sizeof(USHORT));
    PC_ASSERT(Urb->UrbControlGetStatusRequest.TransferBuffer);

    //
    // get status buffer
    //
    DeviceStatus = (PUSHORT)Urb->UrbControlGetStatusRequest.TransferBuffer;


    if (Urb->UrbHeader.UsbdDeviceHandle == PVOID(this) || Urb->UrbHeader.UsbdDeviceHandle == NULL)
    {
        //
        // FIXME need more flags ?
        //
        *DeviceStatus = USB_PORT_STATUS_CONNECT;
        return STATUS_SUCCESS;
    }

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleGetStatusFromDevice invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);


    //
    // generate setup packet
    //
    CtrlSetup.bRequest = USB_REQUEST_GET_STATUS;
    CtrlSetup.wValue.LowByte = 0;
    CtrlSetup.wValue.HiByte = 0;
    CtrlSetup.wIndex.W = Urb->UrbControlGetStatusRequest.Index;
    CtrlSetup.wLength = (USHORT)Urb->UrbControlGetStatusRequest.TransferBufferLength;
    CtrlSetup.bmRequestType.B = 0x80;

    if (Urb->UrbHeader.Function == URB_FUNCTION_GET_STATUS_FROM_INTERFACE)
    {
        //
        // add interface type
        //
        CtrlSetup.bmRequestType.B |= 0x01;
    }
    else if (Urb->UrbHeader.Function == URB_FUNCTION_GET_STATUS_FROM_ENDPOINT)
    {
        //
        // add interface type
        //
        CtrlSetup.bmRequestType.B |= 0x02;
    }

    //
    // submit setup packet
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbControlDescriptorRequest.TransferBuffer);
    ASSERT(Status == STATUS_SUCCESS);
    DPRINT1("[%s] HandleGetStatusFromDevice Status %x Length %lu DeviceStatus %x\n", m_USBType, Status, Urb->UrbControlDescriptorRequest.TransferBufferLength, *DeviceStatus);

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleClassDevice(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PUSB_HUB_DESCRIPTOR UsbHubDescriptor;
    ULONG PortCount, Dummy2;
    USHORT Dummy1;
    PUSBDEVICE UsbDevice;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;

    DPRINT("[%s] HandleClassDevice Request %x Class %x\n", m_USBType, Urb->UrbControlVendorClassRequest.Request, Urb->UrbControlVendorClassRequest.Value >> 8);

    //
    // check class request type
    //
    switch(Urb->UrbControlVendorClassRequest.Request)
    {
        case USB_REQUEST_GET_STATUS:
        {
            //
            // check if this is a valid usb device handle
            //
            if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
            {
                DPRINT1("[%s] HandleClassDevice invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

                //
                // invalid device handle
                //
                return STATUS_DEVICE_NOT_CONNECTED;
            }

            //
            // get device
            //
            UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);


            //
            // generate setup packet
            //
            CtrlSetup.bRequest = USB_REQUEST_GET_STATUS;
            CtrlSetup.wValue.W = Urb->UrbControlVendorClassRequest.Value;
            CtrlSetup.wIndex.W = Urb->UrbControlVendorClassRequest.Index;
            CtrlSetup.wLength = (USHORT)Urb->UrbControlGetStatusRequest.TransferBufferLength;
            CtrlSetup.bmRequestType.B = 0xA0;

            //
            // submit setup packet
            //
            Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbControlDescriptorRequest.TransferBuffer);
            ASSERT(Status == STATUS_SUCCESS);
            break;
        }
        case USB_REQUEST_GET_DESCRIPTOR:
        {
            switch (Urb->UrbControlVendorClassRequest.Value >> 8)
            {
                case USB_DEVICE_CLASS_RESERVED: // FALL THROUGH
                case USB_DEVICE_CLASS_HUB:
                {
                    if (Urb->UrbHeader.UsbdDeviceHandle == PVOID(this)  || Urb->UrbHeader.UsbdDeviceHandle == NULL)
                    {
                        //
                        // sanity checks
                        //
                        PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer);
                        PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBufferLength >= sizeof(USB_HUB_DESCRIPTOR));

                        //
                        // get hub descriptor
                        //
                        UsbHubDescriptor = (PUSB_HUB_DESCRIPTOR)Urb->UrbControlVendorClassRequest.TransferBuffer;

                        //
                        // one hub is handled
                        //
                        UsbHubDescriptor->bDescriptorLength = sizeof(USB_HUB_DESCRIPTOR);
                        Urb->UrbControlVendorClassRequest.TransferBufferLength = sizeof(USB_HUB_DESCRIPTOR);

                        //
                        // type should 0x29 according to msdn
                        //
                        UsbHubDescriptor->bDescriptorType = 0x29;

                        //
                        // get port count
                        //
                        Status = m_Hardware->GetDeviceDetails(&Dummy1, &Dummy1, &PortCount, &Dummy2);
                        PC_ASSERT(Status == STATUS_SUCCESS);

                        //
                        // FIXME: retrieve values
                        //
                        UsbHubDescriptor->bNumberOfPorts = (UCHAR)PortCount;
                        UsbHubDescriptor->wHubCharacteristics = 0x00;
                        UsbHubDescriptor->bPowerOnToPowerGood = 0x01;
                        UsbHubDescriptor->bHubControlCurrent = 0x00;

                        //
                        // done
                        //
                        Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
                        {
                            DPRINT1("[%s] HandleClassDevice invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);
                            //
                            // invalid device handle
                            //
                            return STATUS_DEVICE_NOT_CONNECTED;
                        }

                        //
                        // FIXME: implement support for real hubs
                        //
                        UNIMPLEMENTED
                        Status = STATUS_NOT_IMPLEMENTED;
                    }
                    break;
               }
               default:
                   DPRINT1("[%s] HandleClassDevice Class %x not implemented\n", m_USBType, Urb->UrbControlVendorClassRequest.Value >> 8);
                   break;
            }
            break;
        }
        default:
        {
            //
            // check if this is a valid usb device handle
            //
            if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
            {
                DPRINT1("[%s] HandleClassDevice invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

                //
                // invalid device handle
                //
                return STATUS_DEVICE_NOT_CONNECTED;
            }

            //
            // get device
            //
            UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

            //
            // generate setup packet
            //
            CtrlSetup.bmRequestType.B = 0;
            CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
            CtrlSetup.bmRequestType._BM.Type = BMREQUEST_CLASS;
            CtrlSetup.bRequest = Urb->UrbControlVendorClassRequest.Request;
            CtrlSetup.wValue.W = Urb->UrbControlVendorClassRequest.Value;
            CtrlSetup.wIndex.W = Urb->UrbControlVendorClassRequest.Index;
            CtrlSetup.wLength = (USHORT)Urb->UrbControlVendorClassRequest.TransferBufferLength;

            if (Urb->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            {
                //
                // data direction is device to host
                //
                CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
            }

            //
            // submit setup packet
            //
            Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbControlDescriptorRequest.TransferBuffer);
            ASSERT(Status == STATUS_SUCCESS);

            break;
        }
    }

    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleGetDescriptorFromInterface(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    PUSBDEVICE UsbDevice;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;

    //
    // sanity check
    //
    ASSERT(Urb->UrbControlDescriptorRequest.TransferBufferLength);
    ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleGetDescriptorFromInterface invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

    //
    // generate setup packet
    //
    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
    CtrlSetup.wValue.LowByte = Urb->UrbControlDescriptorRequest.Index;
    CtrlSetup.wValue.HiByte = Urb->UrbControlDescriptorRequest.DescriptorType;
    CtrlSetup.wIndex.W = Urb->UrbControlDescriptorRequest.LanguageId;
    CtrlSetup.wLength = (USHORT)Urb->UrbControlDescriptorRequest.TransferBufferLength;
    CtrlSetup.bmRequestType.B = 0x81;

    //
    // submit setup packet
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbControlDescriptorRequest.TransferBuffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[%s] HandleGetDescriptorFromInterface failed with %x\n", m_USBType, Status);
    }

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleGetDescriptor(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    PUCHAR Buffer;
    PUSBDEVICE UsbDevice;
    ULONG Length, BufferLength;

    DPRINT("[%s] HandleGetDescriptor Type %x\n", m_USBType, Urb->UrbControlDescriptorRequest.DescriptorType);

    //
    // check descriptor type
    //
    switch(Urb->UrbControlDescriptorRequest.DescriptorType)
    {
        case USB_DEVICE_DESCRIPTOR_TYPE:
        {
            //
            // sanity check
            //
            PC_ASSERT(Urb->UrbControlDescriptorRequest.TransferBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR));
            PC_ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);

            if (Urb->UrbHeader.UsbdDeviceHandle == PVOID(this) || Urb->UrbHeader.UsbdDeviceHandle == NULL)
            {
                //
                // copy root hub device descriptor
                //
                RtlCopyMemory((PUCHAR)Urb->UrbControlDescriptorRequest.TransferBuffer, &m_DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
                Irp->IoStatus.Information = sizeof(USB_DEVICE_DESCRIPTOR);
                Urb->UrbControlDescriptorRequest.Hdr.Status = USBD_STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // check if this is a valid usb device handle
                //
                if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
                {
                    DPRINT1("[%s] HandleGetDescriptor invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

                    //
                    // invalid device handle
                    //
                    return STATUS_DEVICE_NOT_CONNECTED;
                }

                //
                // get device
                //
                UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

                //
                // retrieve device descriptor from device
                //
                UsbDevice->GetDeviceDescriptor((PUSB_DEVICE_DESCRIPTOR)Urb->UrbControlDescriptorRequest.TransferBuffer);
                Irp->IoStatus.Information = sizeof(USB_DEVICE_DESCRIPTOR);
                Urb->UrbControlDescriptorRequest.Hdr.Status = USBD_STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
            }
            break;
        }
       case USB_CONFIGURATION_DESCRIPTOR_TYPE:
        {
            //
            // sanity checks
            //
            PC_ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);
            //
            // From MSDN
            // The caller must allocate a buffer large enough to hold all of this information or the data is truncated without error.
            //
            BufferLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;
            Buffer = (PUCHAR) Urb->UrbControlDescriptorRequest.TransferBuffer;

            if (Urb->UrbHeader.UsbdDeviceHandle == PVOID(this) || Urb->UrbHeader.UsbdDeviceHandle == NULL)
            {
                //
                // request is for the root bus controller
                //
                Length = BufferLength > sizeof(USB_CONFIGURATION_DESCRIPTOR) ?
                    sizeof(USB_CONFIGURATION_DESCRIPTOR) : BufferLength;
                RtlCopyMemory(Buffer, &ROOTHUB2_CONFIGURATION_DESCRIPTOR, Length);

                //
                // Check if we still have some space left
                //
                if(Length == BufferLength)
                {
                    //
                    // We copied all we could
                    //
                    Status = STATUS_SUCCESS;
                    break;
                }
                //
                // Go further
                //
                Buffer += Length;
                BufferLength -= Length;

                //
                // copy interface descriptor template
                //
                Length = BufferLength > sizeof(USB_INTERFACE_DESCRIPTOR) ?
                    sizeof(USB_INTERFACE_DESCRIPTOR) : BufferLength;
                RtlCopyMemory(Buffer, &ROOTHUB2_INTERFACE_DESCRIPTOR, Length);

                //
                // Check if we still have some space left
                //
                if(Length == BufferLength)
                {
                    //
                    // We copied all we could
                    //
                    Status = STATUS_SUCCESS;
                    break;
                }
                //
                // Go further
                //
                Buffer += Length;
                BufferLength -= Length;


                //
                // copy end point descriptor template
                //
                Length = BufferLength > sizeof(USB_ENDPOINT_DESCRIPTOR) ?
                    sizeof(USB_ENDPOINT_DESCRIPTOR) : BufferLength;
                RtlCopyMemory(Buffer, &ROOTHUB2_ENDPOINT_DESCRIPTOR, Length);

                //
                // done
                //
                Status = STATUS_SUCCESS;

            }
            else
            {
                //
                // check if this is a valid usb device handle
                //
                if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
                {
                    DPRINT1("[%s] USB_CONFIGURATION_DESCRIPTOR_TYPE invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

                    //
                    // invalid device handle
                    //
                    return STATUS_DEVICE_NOT_CONNECTED;
                }

                //
                // get device
                //
                UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

                //
                // Allocate temporary buffer
                //
                BufferLength = UsbDevice->GetConfigurationDescriptorsLength();
                Buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, BufferLength, TAG_USBLIB);
                if(!Buffer)
                {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                //
                // perform work in IUSBDevice
                //
                UsbDevice->GetConfigurationDescriptors((PUSB_CONFIGURATION_DESCRIPTOR)Buffer, BufferLength, &Length);

                //
                // Copy what we can
                //
                Length = Urb->UrbControlDescriptorRequest.TransferBufferLength > Length ?
                    Length : Urb->UrbControlDescriptorRequest.TransferBufferLength;
                RtlCopyMemory(Urb->UrbControlDescriptorRequest.TransferBuffer, Buffer, Length);

                //
                // Free temporary buffer
                //
                ExFreePoolWithTag(Buffer, TAG_USBLIB);

                //
                // store result size
                //
                Irp->IoStatus.Information = Length;
                Urb->UrbControlDescriptorRequest.TransferBufferLength = Length;
                Urb->UrbControlDescriptorRequest.Hdr.Status = USBD_STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
            }
            break;
        }
        case USB_STRING_DESCRIPTOR_TYPE:
        {
            //
            // sanity check
            //
            PC_ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);
            PC_ASSERT(Urb->UrbControlDescriptorRequest.TransferBufferLength);


            //
            // check if this is a valid usb device handle
            //
            if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
            {
                DPRINT1("[%s] USB_STRING_DESCRIPTOR_TYPE invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

                //
                // invalid device handle
                //
                return STATUS_DEVICE_NOT_CONNECTED;
            }

            //
            // get device
            //
            UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

            //
            // generate setup packet
            //
            CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
            CtrlSetup.wValue.LowByte = Urb->UrbControlDescriptorRequest.Index;
            CtrlSetup.wValue.HiByte = Urb->UrbControlDescriptorRequest.DescriptorType;
            CtrlSetup.wIndex.W = Urb->UrbControlDescriptorRequest.LanguageId;
            CtrlSetup.wLength = (USHORT)Urb->UrbControlDescriptorRequest.TransferBufferLength;
            CtrlSetup.bmRequestType.B = 0x80;

            //
            // submit setup packet
            //
            Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlDescriptorRequest.TransferBufferLength, Urb->UrbControlDescriptorRequest.TransferBuffer);
            break;
        }
        default:
            DPRINT1("[%s] CHubController::HandleGetDescriptor DescriptorType %x unimplemented\n", m_USBType, Urb->UrbControlDescriptorRequest.DescriptorType);
            break;
    }

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleClassEndpoint(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSBDEVICE UsbDevice;

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer);
    PC_ASSERT(Urb->UrbControlVendorClassRequest.TransferBufferLength);
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleClassEndpoint invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);


    DPRINT1("URB_FUNCTION_CLASS_ENDPOINT\n");
    DPRINT1("TransferFlags %x\n", Urb->UrbControlVendorClassRequest.TransferFlags);
    DPRINT1("TransferBufferLength %x\n", Urb->UrbControlVendorClassRequest.TransferBufferLength);
    DPRINT1("TransferBuffer %x\n", Urb->UrbControlVendorClassRequest.TransferBuffer);
    DPRINT1("TransferBufferMDL %x\n", Urb->UrbControlVendorClassRequest.TransferBufferMDL);
    DPRINT1("RequestTypeReservedBits %x\n", Urb->UrbControlVendorClassRequest.RequestTypeReservedBits);
    DPRINT1("Request %x\n", Urb->UrbControlVendorClassRequest.Request);
    DPRINT1("Value %x\n", Urb->UrbControlVendorClassRequest.Value);
    DPRINT1("Index %x\n", Urb->UrbControlVendorClassRequest.Index);

    //
    // initialize setup packet
    //
    CtrlSetup.bmRequestType.B = 0x22; //FIXME: Const.
    CtrlSetup.bRequest = Urb->UrbControlVendorClassRequest.Request;
    CtrlSetup.wValue.W = Urb->UrbControlVendorClassRequest.Value;
    CtrlSetup.wIndex.W = Urb->UrbControlVendorClassRequest.Index;
    CtrlSetup.wLength = (USHORT)Urb->UrbControlVendorClassRequest.TransferBufferLength;

    if (Urb->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        //
        // data direction is device to host
        //
        CtrlSetup.bmRequestType.B |= 0x80;
    }


    //
    // issue request
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlVendorClassRequest.TransferBufferLength, Urb->UrbControlVendorClassRequest.TransferBuffer);

    //
    // assert on failure
    //
    PC_ASSERT(NT_SUCCESS(Status));


    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleVendorDevice(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PUSBDEVICE UsbDevice;
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;

    //DPRINT("CHubController::HandleVendorDevice Request %x\n", Urb->UrbControlVendorClassRequest.Request);

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleVendorDevice invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);

    //
    // initialize setup packet
    //
    CtrlSetup.bmRequestType.B = 0;
    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_VENDOR;
    CtrlSetup.bRequest = Urb->UrbControlVendorClassRequest.Request;
    CtrlSetup.wValue.W = Urb->UrbControlVendorClassRequest.Value;
    CtrlSetup.wIndex.W = Urb->UrbControlVendorClassRequest.Index;
    CtrlSetup.wLength = (USHORT)Urb->UrbControlVendorClassRequest.TransferBufferLength;

    if (Urb->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        //
        // data direction is device to host
        //
        CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
    }

    //
    // issue request
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlVendorClassRequest.TransferBufferLength, Urb->UrbControlVendorClassRequest.TransferBuffer);
    if (NT_SUCCESS(Status))
    {
        // success
        Urb->UrbControlVendorClassRequest.Hdr.Status = USBD_STATUS_SUCCESS;
        Irp->IoStatus.Information = Urb->UrbControlVendorClassRequest.TransferBufferLength;
    }

    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleSyncResetAndClearStall(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUSB_ENDPOINT EndpointDescriptor;
    ULONG Type;

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);
    PC_ASSERT(Urb->UrbHeader.Length == sizeof(struct _URB_PIPE_REQUEST));
    PC_ASSERT(Urb->UrbPipeRequest.PipeHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleSyncResetAndClearStall invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // abort pipe
    //
    Status = HandleAbortPipe(Irp, Urb);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        DPRINT1("[%s] failed to reset pipe %x\n", m_USBType, Status);
    }


    //
    // get endpoint descriptor
    //
    EndpointDescriptor = (PUSB_ENDPOINT)Urb->UrbPipeRequest.PipeHandle;

    //
    // get type
    //
    Type = (EndpointDescriptor->EndPointDescriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK);
    if (Type != USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        //
        // clear stall
        //
        Status = HandleClearStall(Irp, Urb);
    }
    DPRINT1("[%s] URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL Status %x\n", m_USBType, Status);

    //
    // reset data toggle
    //
    if (NT_SUCCESS(Status))
        EndpointDescriptor->DataToggle = 0x0;

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleAbortPipe(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    NTSTATUS Status;
    PUSBDEVICE UsbDevice;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);
    PC_ASSERT(Urb->UrbHeader.Length == sizeof(struct _URB_PIPE_REQUEST));
    PC_ASSERT(Urb->UrbPipeRequest.PipeHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleAbortPipe invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get endpoint descriptor
    //
    EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Urb->UrbPipeRequest.PipeHandle;

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);


    //
    // issue request
    //
    Status = UsbDevice->AbortPipe(EndpointDescriptor);
    DPRINT1("[%s] URB_FUNCTION_ABORT_PIPE Status %x\n", m_USBType, Status);

    //
    // done
    //
    return Status;
}


//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleClearStall(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSBDEVICE UsbDevice;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;


    //
    // sanity check
    //
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);
    PC_ASSERT(Urb->UrbHeader.Length == sizeof(struct _URB_PIPE_REQUEST));
    PC_ASSERT(Urb->UrbPipeRequest.PipeHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleClearStall invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get endpoint descriptor
    //
    EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Urb->UrbPipeRequest.PipeHandle;

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);
    DPRINT1("[%s] URB_FUNCTION_SYNC_CLEAR_STALL\n", m_USBType);

    //
    // initialize setup packet
    //
    CtrlSetup.bmRequestType.B = 0x02;
    CtrlSetup.bRequest = USB_REQUEST_CLEAR_FEATURE;
    CtrlSetup.wValue.W = USB_FEATURE_ENDPOINT_STALL;
    CtrlSetup.wIndex.W = EndpointDescriptor->bEndpointAddress;
    CtrlSetup.wLength = 0;
    CtrlSetup.wValue.W = 0;

    //
    // issue request
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, 0, 0);

    DPRINT1("[%s] URB_FUNCTION_CLEAR_STALL Status %x\n", m_USBType, Status);

    //
    // done
    //
    return Status;
}


//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleClassInterface(
    IN OUT PIRP Irp,
    IN OUT PURB Urb)
{
    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
    NTSTATUS Status;
    PUSBDEVICE UsbDevice;

    //
    // sanity check
    //
    //ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer || Urb->UrbControlVendorClassRequest.TransferBufferMDL);
    //ASSERT(Urb->UrbControlVendorClassRequest.TransferBufferLength);
    PC_ASSERT(Urb->UrbHeader.UsbdDeviceHandle);

    //
    // check if this is a valid usb device handle
    //
    if (!ValidateUsbDevice(PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle)))
    {
        DPRINT1("[%s] HandleClassInterface invalid device handle %p\n", m_USBType, Urb->UrbHeader.UsbdDeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device
    //
    UsbDevice = PUSBDEVICE(Urb->UrbHeader.UsbdDeviceHandle);


    DPRINT("URB_FUNCTION_CLASS_INTERFACE\n");
    DPRINT("TransferFlags %x\n", Urb->UrbControlVendorClassRequest.TransferFlags);
    DPRINT("TransferBufferLength %x\n", Urb->UrbControlVendorClassRequest.TransferBufferLength);
    DPRINT("TransferBuffer %x\n", Urb->UrbControlVendorClassRequest.TransferBuffer);
    DPRINT("TransferBufferMDL %x\n", Urb->UrbControlVendorClassRequest.TransferBufferMDL);
    DPRINT("RequestTypeReservedBits %x\n", Urb->UrbControlVendorClassRequest.RequestTypeReservedBits);
    DPRINT("Request %x\n", Urb->UrbControlVendorClassRequest.Request);
    DPRINT("Value %x\n", Urb->UrbControlVendorClassRequest.Value);
    DPRINT("Index %x\n", Urb->UrbControlVendorClassRequest.Index);

    //
    // initialize setup packet
    //
    CtrlSetup.bmRequestType.B = 0x21;
    CtrlSetup.bRequest = Urb->UrbControlVendorClassRequest.Request;
    CtrlSetup.wValue.W = Urb->UrbControlVendorClassRequest.Value;
    CtrlSetup.wIndex.W = Urb->UrbControlVendorClassRequest.Index;
    CtrlSetup.wLength = (USHORT)Urb->UrbControlVendorClassRequest.TransferBufferLength;

    if (Urb->UrbControlVendorClassRequest.TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        //
        // data direction is device to host
        //
        CtrlSetup.bmRequestType.B |= 0x80;
    }

    //
    // issue request
    //
    Status = UsbDevice->SubmitSetupPacket(&CtrlSetup, Urb->UrbControlVendorClassRequest.TransferBufferLength, Urb->UrbControlVendorClassRequest.TransferBuffer);

    //
    // assert on failure
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // display error
        //
        DPRINT1("[%s] URB_FUNCTION_CLASS_INTERFACE failed with Urb Status %x\n", m_USBType, Urb->UrbHeader.Status);
    }

    //
    // done
    //
    return Status;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // determine which request should be performed
    //
    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
        {
            //
            // get urb
            //
            Urb = (PURB)IoStack->Parameters.Others.Argument1;
            PC_ASSERT(Urb);

            switch (Urb->UrbHeader.Function)
            {
                case URB_FUNCTION_SYNC_RESET_PIPE:
                case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
                    Status = HandleSyncResetAndClearStall(Irp, Urb);
                    break;
                case URB_FUNCTION_ABORT_PIPE:
                    Status = HandleAbortPipe(Irp, Urb);
                    break;
                case URB_FUNCTION_SYNC_CLEAR_STALL:
                    Status = HandleClearStall(Irp, Urb);
                    break;
                case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
                    Status = HandleGetDescriptorFromInterface(Irp, Urb);
                    break;
                case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
                    Status = HandleGetDescriptor(Irp, Urb);
                    break;
                case URB_FUNCTION_CLASS_DEVICE:
                    Status = HandleClassDevice(Irp, Urb);
                    break;
                case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
                case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
                case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
                    Status = HandleGetStatusFromDevice(Irp, Urb);
                    break;
                case URB_FUNCTION_SELECT_CONFIGURATION:
                    Status = HandleSelectConfiguration(Irp, Urb);
                    break;
                case URB_FUNCTION_SELECT_INTERFACE:
                    Status = HandleSelectInterface(Irp, Urb);
                    break;
                case URB_FUNCTION_CLASS_OTHER:
                    Status = HandleClassOther(Irp, Urb);
                    break;
                case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                    Status = HandleBulkOrInterruptTransfer(Irp, Urb);
                    break;
                case URB_FUNCTION_ISOCH_TRANSFER:
                    Status = HandleIsochronousTransfer(Irp, Urb);
                    break;
                case URB_FUNCTION_CLASS_INTERFACE:
                    Status = HandleClassInterface(Irp, Urb);
                    break;
                case URB_FUNCTION_CLASS_ENDPOINT:
                    Status = HandleClassEndpoint(Irp, Urb);
                    break;
                case URB_FUNCTION_VENDOR_DEVICE:
                    Status = HandleVendorDevice(Irp, Urb);
                    break;
                default:
                    DPRINT1("[%s] IOCTL_INTERNAL_USB_SUBMIT_URB Function %x NOT IMPLEMENTED\n", m_USBType, Urb->UrbHeader.Function);
                    break;
            }
            //
            // request completed
            //
            break;
        }
        case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:
        {
            DPRINT("[%s] IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE %p\n", m_USBType, this);

            if (IoStack->Parameters.Others.Argument1)
            {
                //
                // store object as device handle
                //
                *(PVOID *)IoStack->Parameters.Others.Argument1 = (PVOID)this;
                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // mis-behaving hub driver
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            // request completed
            //
            break;
        }
        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
        {
            DPRINT("[%s] IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n", m_USBType);

            //
            // this is the first request send, it delivers the PDO to the caller
            //
            if (IoStack->Parameters.Others.Argument1)
            {
                //
                // store root hub pdo object
                //
                *(PVOID *)IoStack->Parameters.Others.Argument1 = DeviceObject;
            }

            if (IoStack->Parameters.Others.Argument2)
            {
                //
                // documentation claims to deliver the hcd controller object, although it is wrong
                //
                *(PVOID *)IoStack->Parameters.Others.Argument2 = DeviceObject;
            }

            //
            // request completed
            //
            Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_INTERNAL_USB_GET_HUB_COUNT:
        {
            DPRINT("[%s] IOCTL_INTERNAL_USB_GET_HUB_COUNT\n", m_USBType);

            //
            // after IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO is delivered, the usbhub driver
            // requests this ioctl to deliver the number of presents.

            if (IoStack->Parameters.Others.Argument1)
            {
                //
                // FIXME / verify: there is only one hub
                //
                *(PULONG)IoStack->Parameters.Others.Argument1 = 1;
            }

            //
            // request completed
            //
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(ULONG);
            break;
        }
        case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:
        {
            DPRINT1("[%s] IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION UNIMPLEMENTED\n", m_USBType);
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("[%s] HandleDeviceControl>Type: IoCtl %x InputBufferLength %lu OutputBufferLength %lu NOT IMPLEMENTED\n", m_USBType,
                    IoStack->Parameters.DeviceIoControl.IoControlCode,
                    IoStack->Parameters.DeviceIoControl.InputBufferLength,
                    IoStack->Parameters.DeviceIoControl.OutputBufferLength);
            break;
        }
    }
    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

//-----------------------------------------------------------------------------------------
PUSBHARDWAREDEVICE
CHubController::GetUsbHardware()
{
    return m_Hardware;
}

//-----------------------------------------------------------------------------------------
ULONG
CHubController::AcquireDeviceAddress()
{
    KIRQL OldLevel;
    ULONG DeviceAddress;

    //
    // acquire device lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // find address
    //
    DeviceAddress = RtlFindClearBits(&m_DeviceAddressBitmap, 1, 0);
    if (DeviceAddress != MAXULONG)
    {
        //
        // reserve address
        //
        RtlSetBits(&m_DeviceAddressBitmap, DeviceAddress, 1);

        //
        // device addresses start from 0x1 - 0xFF
        //
        DeviceAddress++;
    }

    //
    // release spin lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    //
    // return device address
    //
    return DeviceAddress;
}
//-----------------------------------------------------------------------------------------
VOID
CHubController::ReleaseDeviceAddress(
    ULONG DeviceAddress)
{
    KIRQL OldLevel;

    //
    // acquire device lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // sanity check
    //
    PC_ASSERT(DeviceAddress != 0);

    //
    // convert back to bit number
    //
    DeviceAddress--;

    //
    // clear bit
    //
    RtlClearBits(&m_DeviceAddressBitmap, DeviceAddress, 1);

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);
}
//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::RemoveUsbDevice(
    PUSBDEVICE UsbDevice)
{
    PUSBDEVICE_ENTRY DeviceEntry;
    PLIST_ENTRY Entry;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KIRQL OldLevel;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // point to first entry
    //
    Entry = m_UsbDeviceList.Flink;

    //
    // find matching entry
    //
    while(Entry != &m_UsbDeviceList)
    {
        //
        // get entry
        //
        DeviceEntry = (PUSBDEVICE_ENTRY)CONTAINING_RECORD(Entry, USBDEVICE_ENTRY, Entry);

        //
        // is it current entry
        //
        if (DeviceEntry->Device == UsbDevice)
        {
            //
            // remove entry
            //
            RemoveEntryList(Entry);

            //
            // free entry
            //
            ExFreePoolWithTag(DeviceEntry, TAG_USBLIB);

            //
            // done
            //
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // goto next device
        //
        Entry = Entry->Flink;
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    //
    // return result
    //
    return Status;
}
//-----------------------------------------------------------------------------------------
BOOLEAN
CHubController::ValidateUsbDevice(PUSBDEVICE UsbDevice)
{
    PUSBDEVICE_ENTRY DeviceEntry;
    PLIST_ENTRY Entry;
    KIRQL OldLevel;
    BOOLEAN Result = FALSE;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // point to first entry
    //
    Entry = m_UsbDeviceList.Flink;

    //
    // find matching entry
    //
    while(Entry != &m_UsbDeviceList)
    {
        //
        // get entry
        //
        DeviceEntry = (PUSBDEVICE_ENTRY)CONTAINING_RECORD(Entry, USBDEVICE_ENTRY, Entry);

        //
        // is it current entry
        //
        if (DeviceEntry->Device == UsbDevice)
        {
            //
            // device is valid
            //
            Result = TRUE;
            break;
        }

        //
        // goto next device
        //
        Entry = Entry->Flink;
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    //
    // return result
    //
    return Result;

}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::AddUsbDevice(
    PUSBDEVICE UsbDevice)
{
    PUSBDEVICE_ENTRY DeviceEntry;
    KIRQL OldLevel;

    //
    // allocate device entry
    //
    DeviceEntry = (PUSBDEVICE_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(USBDEVICE_ENTRY), TAG_USBLIB);
    if (!DeviceEntry)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize entry
    //
    DeviceEntry->Device = UsbDevice;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // insert entry
    //
    InsertTailList(&m_UsbDeviceList, &DeviceEntry->Entry);

    //
    // release spin lock
    //
    KeReleaseSpinLock(&m_Lock, OldLevel);

    //
    // done
    //
    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------------------
VOID
CHubController::SetNotification(
    PVOID CallbackContext,
    PRH_INIT_CALLBACK CallbackRoutine)
{
    KIRQL OldLevel;

    //
    // acquire hub controller lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // now set the callback routine and context of the hub
    //
    m_HubCallbackContext = CallbackContext;
    m_HubCallbackRoutine = CallbackRoutine;

   //
   // release hub controller lock
   //
   KeReleaseSpinLock(&m_Lock, OldLevel);
}

//=================================================================================================
//
// Generic Interface functions
//
VOID
USB_BUSIFFN
USBI_InterfaceReference(
    PVOID BusContext)
{
    CHubController * Controller = (CHubController*)BusContext;

    DPRINT1("USBH_InterfaceReference\n");

    //
    // add reference
    //
    Controller->AddRef();
}

VOID
USB_BUSIFFN
USBI_InterfaceDereference(
    PVOID BusContext)
{
    CHubController * Controller = (CHubController*)BusContext;

    DPRINT1("USBH_InterfaceDereference\n");

    //
    // release
    //
    Controller->Release();
}
//=================================================================================================
//
// USB Hub Interface functions
//
NTSTATUS
USB_BUSIFFN
USBHI_CreateUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE *NewDevice,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus,
    USHORT PortNumber)
{
    PUSBDEVICE NewUsbDevice;
    CHubController * Controller;
    NTSTATUS Status;

    DPRINT1("USBHI_CreateUsbDevice\n");

    //
    // first get hub controller
    //
    Controller = (CHubController *)BusContext;

    //
    // sanity check
    //
    PC_ASSERT(Controller);
    PC_ASSERT(BusContext == HubDeviceHandle);

    //
    // now allocate usb device
    //
    Status = CreateUSBDevice(&NewUsbDevice);

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // release controller
        //
        Controller->Release();
        DPRINT1("USBHI_CreateUsbDevice: failed to create usb device %x\n", Status);
        return Status;
    }

    //
    // now initialize device
    //
    Status = NewUsbDevice->Initialize(PHUBCONTROLLER(Controller), Controller->GetUsbHardware(), HubDeviceHandle, PortNumber, PortStatus);

    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // release usb device
        //
        NewUsbDevice->Release();
        DPRINT1("USBHI_CreateUsbDevice: failed to initialize usb device %x\n", Status);
        return Status;
    }

    //
    // insert into list
    //
    Status = Controller->AddUsbDevice(NewUsbDevice);
    //
    // check for success
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // release usb device
        //
        NewUsbDevice->Release();

        DPRINT1("USBHI_CreateUsbDevice: failed to add usb device %x\n", Status);
        return Status;
    }

    //
    // store the handle
    //
    *NewDevice = NewUsbDevice;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_InitializeUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle)
{
    PUSBDEVICE UsbDevice;
    CHubController * Controller;
    ULONG DeviceAddress;
    NTSTATUS Status;
    ULONG Index = 0;

    DPRINT("USBHI_InitializeUsbDevice\n");

    //
    // first get controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);

    //
    // get device object
    //
    UsbDevice = (PUSBDEVICE)DeviceHandle;
    PC_ASSERT(UsbDevice);

    //
    // validate device handle
    //
    if (!Controller->ValidateUsbDevice(UsbDevice))
    {
        DPRINT1("USBHI_InitializeUsbDevice invalid device handle %p\n", DeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // now reserve an address
    //
    DeviceAddress = Controller->AcquireDeviceAddress();

    //
    // is the device address valid
    //
    if (DeviceAddress == MAXULONG)
    {
        //
        // failed to get an device address from the device address pool
        //
        DPRINT1("USBHI_InitializeUsbDevice failed to get device address\n");
        return STATUS_DEVICE_DATA_ERROR;
    }

    do
    {
        //
        // now set the device address
        //
        Status = UsbDevice->SetDeviceAddress((UCHAR)DeviceAddress);

        if (NT_SUCCESS(Status))
            break;

    }while(Index++ < 3    );

    //
    // check for failure
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to set device address
        //
        DPRINT1("USBHI_InitializeUsbDevice failed to set address with %x\n", Status);

        //
        // release address
        //
        Controller->ReleaseDeviceAddress(DeviceAddress);

        //
        // return error
        //
        return STATUS_DEVICE_DATA_ERROR;
    }

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetUsbDescriptors(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PUCHAR DeviceDescriptorBuffer,
    PULONG DeviceDescriptorBufferLength,
    PUCHAR ConfigDescriptorBuffer,
    PULONG ConfigDescriptorBufferLength)
{
    PUSBDEVICE UsbDevice;
    CHubController * Controller;

    DPRINT("USBHI_GetUsbDescriptors\n");

    //
    // sanity check
    //
    PC_ASSERT(DeviceDescriptorBuffer);
    PC_ASSERT(DeviceDescriptorBufferLength);
    PC_ASSERT(*DeviceDescriptorBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR));
    PC_ASSERT(ConfigDescriptorBufferLength);
    PC_ASSERT(*ConfigDescriptorBufferLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR));

    //
    // first get controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);


    //
    // get device object
    //
    UsbDevice = (PUSBDEVICE)DeviceHandle;
    PC_ASSERT(UsbDevice);

    //
    // validate device handle
    //
    if (!Controller->ValidateUsbDevice(UsbDevice))
    {
        DPRINT1("USBHI_GetUsbDescriptors invalid device handle %p\n", DeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // get device descriptor
    //
    UsbDevice->GetDeviceDescriptor((PUSB_DEVICE_DESCRIPTOR)DeviceDescriptorBuffer);

    //
    // store result length
    //
    *DeviceDescriptorBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);

    //
    // get configuration descriptor
    //
    UsbDevice->GetConfigurationDescriptors((PUSB_CONFIGURATION_DESCRIPTOR)ConfigDescriptorBuffer, *ConfigDescriptorBufferLength, ConfigDescriptorBufferLength);

    //
    // complete the request
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_RemoveUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    ULONG Flags)
{
    PUSBDEVICE UsbDevice;
    CHubController * Controller;
    NTSTATUS Status;

    DPRINT("USBHI_RemoveUsbDevice\n");

    //
    // first get controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);

    //
    // get device object
    //
    UsbDevice = (PUSBDEVICE)DeviceHandle;
    PC_ASSERT(UsbDevice);

    //
    // validate device handle
    //
    if (!Controller->ValidateUsbDevice(UsbDevice))
    {
        DPRINT1("USBHI_RemoveUsbDevice invalid device handle %p\n", DeviceHandle);

        //
        // invalid device handle
        //
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // check if there were flags passed
    //
    if (Flags & USBD_KEEP_DEVICE_DATA || Flags  & USBD_MARK_DEVICE_BUSY)
    {
        //
        // ignore flags for now
        //
        return STATUS_SUCCESS;
    }

    //
    // remove device
    //
    Status = Controller->RemoveUsbDevice(UsbDevice);
    if (!NT_SUCCESS(Status))
    {
        //
        // invalid device handle
        //
        DPRINT1("USBHI_RemoveUsbDevice Invalid device handle %p\n", UsbDevice);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // release usb device
    //
    UsbDevice->Release();

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_RestoreUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE OldDeviceHandle,
    PUSB_DEVICE_HANDLE NewDeviceHandle)
{
    PUSBDEVICE OldUsbDevice, NewUsbDevice;
    CHubController * Controller;

    DPRINT("USBHI_RestoreUsbDevice\n");

    //
    // first get controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);

    //
    // get device object
    //
    OldUsbDevice = (PUSBDEVICE)OldDeviceHandle;
    NewUsbDevice = (PUSBDEVICE)NewDeviceHandle;
    PC_ASSERT(OldUsbDevice);
    PC_ASSERT(NewDeviceHandle);

    //
    // validate device handle
    //
    PC_ASSERT(Controller->ValidateUsbDevice(NewUsbDevice));
    PC_ASSERT(Controller->ValidateUsbDevice(OldUsbDevice));

    DPRINT1("NewUsbDevice: DeviceAddress %x\n", NewUsbDevice->GetDeviceAddress());
    DPRINT1("OldUsbDevice: DeviceAddress %x\n", OldUsbDevice->GetDeviceAddress());

    //
    // remove old device handle
    //
    USBHI_RemoveUsbDevice(BusContext, OldDeviceHandle, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_QueryDeviceInformation(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PVOID DeviceInformationBuffer,
    ULONG DeviceInformationBufferLength,
    PULONG LengthReturned)
{
    PUSB_DEVICE_INFORMATION_0 DeviceInfo;
    PUSBDEVICE UsbDevice;
    CHubController * Controller;

    DPRINT("USBHI_QueryDeviceInformation %p\n", BusContext);

    //
    // sanity check
    //
    PC_ASSERT(DeviceInformationBufferLength >= sizeof(USB_DEVICE_INFORMATION_0));
    PC_ASSERT(DeviceInformationBuffer);
    PC_ASSERT(LengthReturned);

    //
    // get controller object
    //
    Controller = (CHubController*)BusContext;
    PC_ASSERT(Controller);

    //
    // get device object
    //
    UsbDevice = (PUSBDEVICE)DeviceHandle;
    PC_ASSERT(UsbDevice);

    if (BusContext != DeviceHandle)
    {
        //
        // validate device handle
        //
        if (!Controller->ValidateUsbDevice(UsbDevice))
        {
            DPRINT1("USBHI_QueryDeviceInformation invalid device handle %p\n", DeviceHandle);

            //
            // invalid device handle
            //
            return STATUS_DEVICE_NOT_CONNECTED;
        }

        //
        // access information buffer
        //
        DeviceInfo = (PUSB_DEVICE_INFORMATION_0)DeviceInformationBuffer;

        //
        // initialize with default values
        //
        DeviceInfo->InformationLevel = 0;
        DeviceInfo->ActualLength = sizeof(USB_DEVICE_INFORMATION_0);
        DeviceInfo->PortNumber = UsbDevice->GetPort();
        DeviceInfo->CurrentConfigurationValue = UsbDevice->GetConfigurationValue();
        DeviceInfo->DeviceAddress = UsbDevice->GetDeviceAddress();
        DeviceInfo->HubAddress = 0; //FIXME
        DeviceInfo->DeviceSpeed = UsbDevice->GetSpeed();
        DeviceInfo->DeviceType = UsbDevice->GetType();
        DeviceInfo->NumberOfOpenPipes = 0; //FIXME

        //
        // get device descriptor
        //
        UsbDevice->GetDeviceDescriptor(&DeviceInfo->DeviceDescriptor);

        //
        // FIXME return pipe information
        //

        //
        // store result length
        //
        *LengthReturned = sizeof(USB_DEVICE_INFORMATION_0);

        return STATUS_SUCCESS;
    }

    //
    // access information buffer
    //
    DeviceInfo = (PUSB_DEVICE_INFORMATION_0)DeviceInformationBuffer;

    //
    // initialize with default values
    //
    DeviceInfo->InformationLevel = 0;
    DeviceInfo->ActualLength = sizeof(USB_DEVICE_INFORMATION_0);
    DeviceInfo->PortNumber = 0;
    DeviceInfo->CurrentConfigurationValue = 0; //FIXME;
    DeviceInfo->DeviceAddress = 0;
    DeviceInfo->HubAddress = 0; //FIXME
    DeviceInfo->DeviceSpeed = UsbHighSpeed; //FIXME
    DeviceInfo->DeviceType = Usb20Device; //FIXME
    DeviceInfo->NumberOfOpenPipes = 0; //FIXME

    //
    // get device descriptor
    //
    RtlMoveMemory(&DeviceInfo->DeviceDescriptor, ROOTHUB2_DEVICE_DESCRIPTOR, sizeof(USB_DEVICE_DESCRIPTOR));

    //
    // FIXME return pipe information
    //

    //
    // store result length
    //
#ifdef _MSC_VER
    *LengthReturned = FIELD_OFFSET(USB_DEVICE_INFORMATION_0, PipeList[DeviceInfo->NumberOfOpenPipes]);
#else
    *LengthReturned = sizeof(USB_DEVICE_INFORMATION_0) + (DeviceInfo->NumberOfOpenPipes > 1 ? (DeviceInfo->NumberOfOpenPipes - 1) * sizeof(USB_PIPE_INFORMATION_0) : 0);
#endif
    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetControllerInformation(
    PVOID BusContext,
    PVOID ControllerInformationBuffer,
    ULONG ControllerInformationBufferLength,
    PULONG LengthReturned)
{
    PUSB_CONTROLLER_INFORMATION_0 ControllerInfo;

    DPRINT("USBHI_GetControllerInformation\n");

    //
    // sanity checks
    //
    PC_ASSERT(ControllerInformationBuffer);
    PC_ASSERT(ControllerInformationBufferLength >= sizeof(USB_CONTROLLER_INFORMATION_0));

    //
    // get controller info buffer
    //
    ControllerInfo = (PUSB_CONTROLLER_INFORMATION_0)ControllerInformationBuffer;

    //
    // FIXME only version 0 is supported for now
    //
    PC_ASSERT(ControllerInfo->InformationLevel == 0);

    //
    // fill in information
    //
    ControllerInfo->ActualLength = sizeof(USB_CONTROLLER_INFORMATION_0);
    ControllerInfo->SelectiveSuspendEnabled = FALSE; //FIXME
    ControllerInfo->IsHighSpeedController = TRUE;

    //
    // set length returned
    //
    *LengthReturned = ControllerInfo->ActualLength;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_ControllerSelectiveSuspend(
    PVOID BusContext,
    BOOLEAN Enable)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetExtendedHubInformation(
    PVOID BusContext,
    PDEVICE_OBJECT HubPhysicalDeviceObject,
    PVOID HubInformationBuffer,
    ULONG HubInformationBufferLength,
    PULONG LengthReturned)
{
    PUSB_EXTHUB_INFORMATION_0 HubInfo;
    CHubController * Controller;
    PUSBHARDWAREDEVICE Hardware;
    ULONG Index;
    ULONG NumPort, Dummy2;
    USHORT Dummy1;
    NTSTATUS Status;

    DPRINT("USBHI_GetExtendedHubInformation\n");

    //
    // sanity checks
    //
    PC_ASSERT(HubInformationBuffer);
    PC_ASSERT(HubInformationBufferLength == sizeof(USB_EXTHUB_INFORMATION_0));
    PC_ASSERT(LengthReturned);

    //
    // get hub controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);

    //
    // get usb hardware device
    //
    Hardware = Controller->GetUsbHardware();

    //
    // retrieve number of ports
    //
    Status = Hardware->GetDeviceDetails(&Dummy1, &Dummy1, &NumPort, &Dummy2);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get hardware details, ouch ;)
        //
        DPRINT1("USBHI_GetExtendedHubInformation failed to get hardware details with %x\n", Status);
        return Status;
    }

    //
    // get hub information buffer
    //
    HubInfo = (PUSB_EXTHUB_INFORMATION_0)HubInformationBuffer;

    //
    // initialize hub information
    //
    HubInfo->InformationLevel = 0;

    //
    // store port count
    //
    HubInfo->NumberOfPorts = NumPort;

    //
    // initialize port information
    //
    for(Index = 0; Index < NumPort; Index++)
    {
        HubInfo->Port[Index].PhysicalPortNumber = Index + 1;
        HubInfo->Port[Index].PortLabelNumber = Index + 1;
        HubInfo->Port[Index].VidOverride = 0;
        HubInfo->Port[Index].PidOverride = 0;
        HubInfo->Port[Index].PortAttributes = USB_PORTATTR_SHARED_USB2; //FIXME
    }

    //
    // store result length
    //
#ifdef _MSC_VER
    *LengthReturned = FIELD_OFFSET(USB_EXTHUB_INFORMATION_0, Port[HubInfo->NumberOfPorts]);
#else
    *LengthReturned = FIELD_OFFSET(USB_EXTHUB_INFORMATION_0, Port) + sizeof(USB_EXTPORT_INFORMATION_0) * HubInfo->NumberOfPorts;
#endif

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetRootHubSymbolicName(
    PVOID BusContext,
    PVOID HubSymNameBuffer,
    ULONG HubSymNameBufferLength,
    PULONG HubSymNameActualLength)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

PVOID
USB_BUSIFFN
USBHI_GetDeviceBusContext(
    PVOID HubBusContext,
    PVOID DeviceHandle)
{
    UNIMPLEMENTED
    return NULL;
}

NTSTATUS
USB_BUSIFFN
USBHI_Initialize20Hub(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    ULONG TtCount)
{
    DPRINT("USBHI_Initialize20Hub HubDeviceHandle %p UNIMPLEMENTED TtCount %lu\n", HubDeviceHandle, TtCount);
    return STATUS_SUCCESS;
}


WORKER_THREAD_ROUTINE InitRootHub;

VOID
NTAPI
InitRootHub(IN PVOID Context)
{
    PINIT_ROOT_HUB_CONTEXT WorkItem;

    //
    // get context
    //
    WorkItem = (PINIT_ROOT_HUB_CONTEXT)Context;

    //
    // perform callback
    //
    WorkItem->CallbackRoutine(WorkItem->CallbackContext);

    //
    // free contextg
    //
    ExFreePoolWithTag(Context, TAG_USBLIB);
}

NTSTATUS
USB_BUSIFFN
USBHI_RootHubInitNotification(
    PVOID BusContext,
    PVOID CallbackContext,
    PRH_INIT_CALLBACK CallbackRoutine)
{
    CHubController * Controller;
    PINIT_ROOT_HUB_CONTEXT WorkItem;

    DPRINT("USBHI_RootHubInitNotification %p \n", CallbackContext);

    //
    // get controller object
    //
    Controller = (CHubController*)BusContext;
    PC_ASSERT(Controller);

    //
    // set notification routine
    //
    Controller->SetNotification(CallbackContext, CallbackRoutine);

    //
    // Create and initialize work item data
    //
    WorkItem = (PINIT_ROOT_HUB_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(INIT_ROOT_HUB_CONTEXT), TAG_USBLIB);
    if (!WorkItem)
    {
        DPRINT1("Failed to allocate memory!n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init context
    //
    WorkItem->CallbackRoutine = CallbackRoutine;
    WorkItem->CallbackContext = CallbackContext;

    //
    // Queue the work item to handle initializing the device
    //
    ExInitializeWorkItem(&WorkItem->WorkItem, InitRootHub, (PVOID)WorkItem);
    ExQueueWorkItem(&WorkItem->WorkItem, DelayedWorkQueue);

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
USB_BUSIFFN
USBHI_FlushTransfers(
    PVOID BusContext,
    PVOID DeviceHandle)
{
    UNIMPLEMENTED
}

VOID
USB_BUSIFFN
USBHI_SetDeviceHandleData(
    PVOID BusContext,
    PVOID DeviceHandle,
    PDEVICE_OBJECT UsbDevicePdo)
{
    PUSBDEVICE UsbDevice;
    CHubController * Controller;

    //
    // get controller
    //
    Controller = (CHubController *)BusContext;
    PC_ASSERT(Controller);

    //
    // get device handle
    //
    UsbDevice = (PUSBDEVICE)DeviceHandle;

    //
    // validate device handle
    //
    if (!Controller->ValidateUsbDevice(UsbDevice))
    {
        DPRINT1("USBHI_SetDeviceHandleData DeviceHandle %p is invalid\n", DeviceHandle);

        //
        // invalid handle
        //
        return;
    }
    else
    {
        //
        // usbhub sends this request as a part of the Pnp startup sequence
        // looks like we need apply a dragon voodoo to fixup the device stack
        // otherwise usbhub will cause a bugcheck
        //
        DPRINT1("USBHI_SetDeviceHandleData %p\n", UsbDevicePdo);

        //
        // sanity check
        //
        PC_ASSERT(UsbDevicePdo->AttachedDevice);

        //
        // should be usbstor
        // fixup device stack voodoo part #2
        //
        UsbDevicePdo->AttachedDevice->StackSize++;

        //
        // set device handle data
        //
        UsbDevice->SetDeviceHandleData(UsbDevicePdo);
    }
}

//=================================================================================================
//
// USB Device Interface functions
//

VOID
USB_BUSIFFN
USBDI_GetUSBDIVersion(
    PVOID BusContext,
    PUSBD_VERSION_INFORMATION VersionInformation,
    PULONG HcdCapabilites)
{
    CHubController * Controller;
    PUSBHARDWAREDEVICE Device;
    ULONG Speed, Dummy2;
    USHORT Dummy1;

    DPRINT("USBDI_GetUSBDIVersion\n");

    //
    // get controller
    //
    Controller = (CHubController*)BusContext;

    //
    // get usb hardware
    //
    Device = Controller->GetUsbHardware();
    PC_ASSERT(Device);

    if (VersionInformation)
    {
        //
        // windows xp supported
        //
        VersionInformation->USBDI_Version = 0x00000500;

        //
        // get device speed
        //
        Device->GetDeviceDetails(&Dummy1, &Dummy1, &Dummy2, &Speed);

        //
        // store speed details
        //
        VersionInformation->Supported_USB_Version = Speed;
    }

    //
    // no flags supported
    //
    *HcdCapabilites = 0;
}

NTSTATUS
USB_BUSIFFN
USBDI_QueryBusTime(
    PVOID BusContext,
    PULONG CurrentFrame)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USB_BUSIFFN
USBDI_SubmitIsoOutUrb(
    PVOID BusContext,
    PURB Urb)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
USB_BUSIFFN
USBDI_QueryBusInformation(
    PVOID BusContext,
    ULONG Level,
    PVOID BusInformationBuffer,
    PULONG BusInformationBufferLength,
    PULONG BusInformationActualLength)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
USB_BUSIFFN
USBDI_IsDeviceHighSpeed(
    PVOID BusContext)
{
    CHubController * Controller;
    PUSBHARDWAREDEVICE Device;
    ULONG Speed, Dummy2;
    USHORT Dummy1;

    DPRINT("USBDI_IsDeviceHighSpeed\n");

    //
    // get controller
    //
    Controller = (CHubController*)BusContext;

    //
    // get usb hardware
    //
    Device = Controller->GetUsbHardware();
    PC_ASSERT(Device);

    //
    // get device speed
    //
    Device->GetDeviceDetails(&Dummy1, &Dummy1, &Dummy2, &Speed);

    //
    // USB 2.0 equals 0x200
    //
    return (Speed == 0x200);
}

NTSTATUS
USB_BUSIFFN
USBDI_EnumLogEntry(
    PVOID BusContext,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CHubController::HandleQueryInterface(
    PIO_STACK_LOCATION IoStack)
{
    PUSB_BUS_INTERFACE_HUB_V5 InterfaceHub;
    PUSB_BUS_INTERFACE_USBDI_V2 InterfaceDI;
    UNICODE_STRING GuidBuffer;
    NTSTATUS Status;

    if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, USB_BUS_INTERFACE_HUB_GUID))
    {
        //
        // get request parameters
        //
        InterfaceHub = (PUSB_BUS_INTERFACE_HUB_V5)IoStack->Parameters.QueryInterface.Interface;
        InterfaceHub->Version = IoStack->Parameters.QueryInterface.Version;

        //
        // check version
        //
        if (IoStack->Parameters.QueryInterface.Version >= 6)
        {
            DPRINT1("USB_BUS_INTERFACE_HUB_GUID version %x not supported!\n", IoStack->Parameters.QueryInterface.Version);

            //
            // version not supported
            //
            return STATUS_NOT_SUPPORTED;
        }

        //
        // Interface version 0
        //
        InterfaceHub->Size = IoStack->Parameters.QueryInterface.Size;
        InterfaceHub->BusContext = PVOID(this);
        InterfaceHub->InterfaceReference = USBI_InterfaceReference;
        InterfaceHub->InterfaceDereference = USBI_InterfaceDereference;

        //
        // Interface version 1
        //
        if (IoStack->Parameters.QueryInterface.Version >= 1)
        {
            InterfaceHub->CreateUsbDevice = USBHI_CreateUsbDevice;
            InterfaceHub->InitializeUsbDevice = USBHI_InitializeUsbDevice;
            InterfaceHub->GetUsbDescriptors = USBHI_GetUsbDescriptors;
            InterfaceHub->RemoveUsbDevice = USBHI_RemoveUsbDevice;
            InterfaceHub->RestoreUsbDevice = USBHI_RestoreUsbDevice;
            InterfaceHub->QueryDeviceInformation = USBHI_QueryDeviceInformation;
        }

        //
        // Interface version 2
        //
        if (IoStack->Parameters.QueryInterface.Version >= 2)
        {
            InterfaceHub->GetControllerInformation = USBHI_GetControllerInformation;
            InterfaceHub->ControllerSelectiveSuspend = USBHI_ControllerSelectiveSuspend;
            InterfaceHub->GetExtendedHubInformation = USBHI_GetExtendedHubInformation;
            InterfaceHub->GetRootHubSymbolicName = USBHI_GetRootHubSymbolicName;
            InterfaceHub->GetDeviceBusContext = USBHI_GetDeviceBusContext;
            InterfaceHub->Initialize20Hub = USBHI_Initialize20Hub;

        }

        //
        // Interface version 3
        //
        if (IoStack->Parameters.QueryInterface.Version >= 3)
        {
            InterfaceHub->RootHubInitNotification = USBHI_RootHubInitNotification;
        }

        //
        // Interface version 4
        //
        if (IoStack->Parameters.QueryInterface.Version >= 4)
        {
            InterfaceHub->FlushTransfers = USBHI_FlushTransfers;
        }

        //
        // Interface version 5
        //
        if (IoStack->Parameters.QueryInterface.Version >= 5)
        {
            InterfaceHub->SetDeviceHandleData = USBHI_SetDeviceHandleData;
        }

        //
        // request completed
        //
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, USB_BUS_INTERFACE_USBDI_GUID))
    {
        //
        // get request parameters
        //
        InterfaceDI = (PUSB_BUS_INTERFACE_USBDI_V2) IoStack->Parameters.QueryInterface.Interface;
        InterfaceDI->Version = IoStack->Parameters.QueryInterface.Version;

        //
        // check version
        //
        if (IoStack->Parameters.QueryInterface.Version >= 3)
        {
            DPRINT1("USB_BUS_INTERFACE_USBDI_GUID version %x not supported!\n", IoStack->Parameters.QueryInterface.Version);

            //
            // version not supported
            //
            return STATUS_NOT_SUPPORTED;
        }

        //
        // interface version 0
        //
        InterfaceDI->Size = IoStack->Parameters.QueryInterface.Size;
        InterfaceDI->BusContext = PVOID(this);
        InterfaceDI->InterfaceReference = USBI_InterfaceReference;
        InterfaceDI->InterfaceDereference = USBI_InterfaceDereference;
        InterfaceDI->GetUSBDIVersion = USBDI_GetUSBDIVersion;
        InterfaceDI->QueryBusTime = USBDI_QueryBusTime;
        InterfaceDI->SubmitIsoOutUrb = USBDI_SubmitIsoOutUrb;
        InterfaceDI->QueryBusInformation = USBDI_QueryBusInformation;

        //
        // interface version 1
        //
        if (IoStack->Parameters.QueryInterface.Version >= 1)
        {
            InterfaceDI->IsDeviceHighSpeed = USBDI_IsDeviceHighSpeed;
        }

        //
        // interface version 2
        //
        if (IoStack->Parameters.QueryInterface.Version >= 2)
        {
            InterfaceDI->EnumLogEntry = USBDI_EnumLogEntry;
        }

        //
        // request completed
        //
        return STATUS_SUCCESS;
    }
    else
    {
        //
        // convert guid to string
        //
        Status = RtlStringFromGUID(*IoStack->Parameters.QueryInterface.InterfaceType, &GuidBuffer);
        if (NT_SUCCESS(Status))
        {
            //
            // print interface
            //
            DPRINT1("HandleQueryInterface UNKNOWN INTERFACE GUID: %wZ Version %x\n", &GuidBuffer, IoStack->Parameters.QueryInterface.Version);

            //
            // free guid buffer
            //
            RtlFreeUnicodeString(&GuidBuffer);
        }
    }
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
CHubController::SetDeviceInterface(
    BOOLEAN Enable)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Enable)
    {
        //
        // register device interface
        //
        Status = IoRegisterDeviceInterface(m_HubControllerDeviceObject, &GUID_DEVINTERFACE_USB_HUB, 0, &m_HubDeviceInterfaceString);

        if (NT_SUCCESS(Status))
        {
            //
            // now enable the device interface
            //
            Status = IoSetDeviceInterfaceState(&m_HubDeviceInterfaceString, TRUE);

            //
            // enable interface
            //
            m_InterfaceEnabled = TRUE;
        }
    }
    else if (m_InterfaceEnabled)
    {
        //
        // disable device interface
        //
        Status = IoSetDeviceInterfaceState(&m_HubDeviceInterfaceString, FALSE);

        if (NT_SUCCESS(Status))
        {
            //
            // now delete interface string
            //
            RtlFreeUnicodeString(&m_HubDeviceInterfaceString);
        }

        //
        // disable interface
        //
        m_InterfaceEnabled = FALSE;
    }

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CHubController::CreatePDO(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT * OutDeviceObject)
{
    WCHAR CharDeviceName[64];
    NTSTATUS Status;
    ULONG UsbDeviceNumber = 0;
    UNICODE_STRING DeviceName;

    while (TRUE)
    {
        //
        // construct device name
        //
        swprintf(CharDeviceName, L"\\Device\\USBPDO-%d", UsbDeviceNumber);

        //
        // initialize device name
        //
        RtlInitUnicodeString(&DeviceName, CharDeviceName);

        //
        // create device
        //
        Status = IoCreateDevice(DriverObject,
                                sizeof(COMMON_DEVICE_EXTENSION),
                                &DeviceName,
                                FILE_DEVICE_CONTROLLER,
                                0,
                                FALSE,
                                OutDeviceObject);

        /* check for success */
        if (NT_SUCCESS(Status))
            break;

        //
        // is there a device object with that same name
        //
        if ((Status == STATUS_OBJECT_NAME_EXISTS) || (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            //
            // Try the next name
            //
            UsbDeviceNumber++;
            continue;
        }

        //
        // bail out on other errors
        //
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreatePDO: Failed to create %wZ, Status %x\n", &DeviceName, Status);
            return Status;
        }
    }

    DPRINT("CHubController::CreatePDO: DeviceName %wZ\n", &DeviceName);

    //
    // fixup device stack voodoo part #1
    //
    (*OutDeviceObject)->StackSize++;

    /* done */
    return Status;
}



NTSTATUS
NTAPI
CreateHubController(
    PHUBCONTROLLER *OutHcdController)
{
    PHUBCONTROLLER This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBLIB) CHubController(0);
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
    *OutHcdController = (PHUBCONTROLLER)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID NTAPI StatusChangeEndpointCallBack(PVOID Context)
{
    CHubController* This;
    PIRP Irp;
    This = (CHubController*)Context;

    ASSERT(This);

    Irp = This->m_PendingSCEIrp;
    if (!Irp)
    {
        DPRINT1("There was no pending IRP for SCE. Did the usb hub 2.0 driver (usbhub2) load?\n");
        return;
    }

    This->m_PendingSCEIrp = NULL;
    This->QueryStatusChangeEndpoint(Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}
