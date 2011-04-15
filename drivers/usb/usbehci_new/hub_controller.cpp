/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hub_controller.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbehci.h"

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

    // IDispatchIrp interface functions
    virtual NTSTATUS HandlePnp(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    virtual NTSTATUS HandlePower(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    virtual NTSTATUS HandleDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);

    // local functions
    NTSTATUS HandleQueryInterface(PIO_STACK_LOCATION IoStack);
    NTSTATUS SetDeviceInterface(BOOLEAN bEnable);
    NTSTATUS CreatePDO(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT * OutDeviceObject);
    NTSTATUS GetHubControllerDeviceObject(PDEVICE_OBJECT * HubDeviceObject);

    // constructor / destructor
    CHubController(IUnknown *OuterUnknown){}
    virtual ~CHubController(){}

protected:
    LONG m_Ref;
    PHCDCONTROLLER m_Controller;
    PUSBHARDWAREDEVICE m_Hardware;
    BOOLEAN m_IsRootHubDevice;
    ULONG m_DeviceAddress;
    ULONG m_PDODeviceNumber;
    BOOLEAN m_InterfaceEnabled;
    UNICODE_STRING m_HubDeviceInterfaceString;
    PDEVICE_OBJECT m_HubControllerDeviceObject;
    PDRIVER_OBJECT m_DriverObject;
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

    DPRINT1("CHubController::Initialize\n");

    //
    // initialize members
    //
    m_Controller = Controller;
    m_Hardware = Device;
    m_IsRootHubDevice = IsRootHubDevice;
    m_DeviceAddress = DeviceAddress;
    m_DriverObject = DriverObject;

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
    // clear init flag
    //
    m_HubControllerDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


    return STATUS_SUCCESS;
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
CHubController::HandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    LPGUID Guid;
    NTSTATUS Status;
    ULONG Index;

    //
    // get device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(DeviceExtension->IsFDO == FALSE);

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT1("CHubController::HandlePnp IRP_MN_START_DEVICE\n");
            //
            // register device interface 
            //
            Status = SetDeviceInterface(TRUE);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT1("CHubController::HandlePnp IRP_MN_QUERY_CAPABILITIES\n");

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
            for (Index = 0; Index < PowerSystemMaximum; Index++)
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
            DPRINT1("CHubController::HandlePnp IRP_MN_QUERY_INTERFACE\n");

            //
            // handle device interface requests
            //
            Status = HandleQueryInterface(IoStack);
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT1("CHubController::HandlePnp IRP_MN_REMOVE_DEVICE\n");

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
            return STATUS_SUCCESS;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            DPRINT1("CHubController::HandlePnp IRP_MN_QUERY_BUS_INFORMATION\n");

            //
            // allocate buffer for bus guid
            //
            Guid = (LPGUID)ExAllocatePool(PagedPool, sizeof(GUID));
            if (Guid)
            {
                //
                // copy BUS guid
                //
                RtlMoveMemory(Guid, &GUID_BUS_TYPE_USB, sizeof(GUID));
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR)Guid;
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
            DPRINT1("CHubController::HandlePnp IRP_MN_STOP_DEVICE\n");
            //
            // stop device
            //
            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            DPRINT1("CHubController::HandlePnp Unhandeled %x\n", IoStack->MinorFunction);
            Status = STATUS_NOT_SUPPORTED;
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
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------------------
NTSTATUS
CHubController::HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
CHubController::HandleQueryInterface(
    PIO_STACK_LOCATION IoStack)
{
    UNICODE_STRING GuidBuffer;
    NTSTATUS Status;

    if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, USB_BUS_INTERFACE_HUB_GUID))
    {
        DPRINT1("HandleQueryInterface> UNIMPLEMENTED USB_BUS_INTERFACE_HUB_GUID Version %x\n", IoStack->Parameters.QueryInterface.Version);

    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, USB_BUS_INTERFACE_USBDI_GUID))
    {
        DPRINT1("HandleQueryInterface> UNIMPLEMENTED USB_BUS_INTERFACE_USBDI_GUID Version %x\n", IoStack->Parameters.QueryInterface.Version);
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
            DPRINT1("HandleQueryInterface GUID: %wZ Version %x\n", &GuidBuffer, IoStack->Parameters.QueryInterface.Version);

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
    return Status;
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

    //
    // store PDO number
    //
    //m_PDODeviceNumber = UsbDeviceNumber;

    DPRINT1("CreateFDO: DeviceName %wZ\n", &DeviceName);

    /* done */
    return Status;
}



NTSTATUS
CreateHubController(
    PHUBCONTROLLER *OutHcdController)
{
    PHUBCONTROLLER This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBEHCI) CHubController(0);
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
