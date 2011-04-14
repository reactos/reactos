/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hcd_controller.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbehci.h"

class CHCDController : public IHCDController
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

    // interface functions
    NTSTATUS Initialize(IN PROOTHDCCONTROLLER RootHCDController, IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject);
    NTSTATUS HandlePnp(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    NTSTATUS HandlePower(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
    NTSTATUS HandleDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);

    // local functions
    NTSTATUS CreateFDO(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT * OutDeviceObject);
    NTSTATUS CreatePDO(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT * OutDeviceObject);
    NTSTATUS HandleQueryInterface(PIO_STACK_LOCATION IoStack);
    NTSTATUS SetDeviceInterface(BOOLEAN bEnable);

    // constructor / destructor
    CHCDController(IUnknown *OuterUnknown){}
    virtual ~CHCDController(){}

protected:
    LONG m_Ref;
    PROOTHDCCONTROLLER m_RootController;
    PDRIVER_OBJECT m_DriverObject;
    PDEVICE_OBJECT m_PhysicalDeviceObject;
    PDEVICE_OBJECT m_FunctionalDeviceObject;
    PDEVICE_OBJECT m_NextDeviceObject;
    PUSBHARDWAREDEVICE m_Hardware;
    PDEVICE_OBJECT m_BusPDO;
    UNICODE_STRING m_HubDeviceInterfaceString;
    BOOLEAN m_InterfaceEnabled;
    ULONG m_PDODeviceNumber;
    ULONG m_FDODeviceNumber;
};

//=================================================================================================
// COM
//
NTSTATUS
STDMETHODCALLTYPE
CHCDController::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

//-------------------------------------------------------------------------------------------------
NTSTATUS
CHCDController::Initialize(
    IN PROOTHDCCONTROLLER RootHCDController,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;

    //
    // create usb hardware
    //
    Status = CreateUSBHardware(&m_Hardware);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create hardware object
        //
        DPRINT1("Failed to create hardware object\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize members
    //
    m_DriverObject = DriverObject;
    m_PhysicalDeviceObject = PhysicalDeviceObject;
    m_RootController = RootHCDController;

    //
    // create FDO
    //
    Status = CreateFDO(m_DriverObject, &m_FunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to create PDO
        //
        return Status;
    }

    //
    // now attach to device stack
    //
    m_NextDeviceObject = IoAttachDeviceToDeviceStack(m_FunctionalDeviceObject, m_PhysicalDeviceObject);
    if (!m_NextDeviceObject)
    {
        //
        // failed to attach to device stack
        //
        IoDeleteDevice(m_FunctionalDeviceObject);
        m_FunctionalDeviceObject = 0;

        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // initialize hardware object
    //
    Status = m_Hardware->Initialize(m_DriverObject, m_FunctionalDeviceObject, m_PhysicalDeviceObject, m_NextDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize hardware object %x\n", Status);

        //
        // failed to initialize hardware object, detach from device stack
        //
        IoDetachDevice(m_NextDeviceObject);

        //
        // now delete the device
        //
        IoDeleteDevice(m_FunctionalDeviceObject);

        //
        // nullify pointers :)
        //
        m_FunctionalDeviceObject = 0;
        m_NextDeviceObject = 0;

        return Status;
    }


    //
    // set device flags
    //
    m_FunctionalDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;


    //
    // get device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)m_FunctionalDeviceObject->DeviceExtension;
    PC_ASSERT(DeviceExtension);

    //
    // initialize device extension
    //
    DeviceExtension->IsFDO = TRUE;
    DeviceExtension->IsHub = FALSE;
    DeviceExtension->HcdController = PHCDCONTROLLER(this);

    //
    // device is initialized
    //
    m_FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


    //
    // is there a root controller
    //
    if (m_RootController)
    {
        //
        // add reference
        //
        m_RootController->AddRef();

        //
        // register with controller
        //
        m_RootController->RegisterHCD(this);
    }


    //
    // done
    //
    return STATUS_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
NTSTATUS
CHCDController::HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PUSB_HCD_DRIVERKEY_NAME DriverKey;
    ULONG ResultLength;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    DPRINT1("HandleDeviceControl>Type: FDO %u IoCtl %x InputBufferLength %lu OutputBufferLength %lu NOT IMPLEMENTED\n",
        DeviceExtension->IsFDO,
        IoStack->Parameters.DeviceIoControl.IoControlCode,
        IoStack->Parameters.DeviceIoControl.InputBufferLength,
        IoStack->Parameters.DeviceIoControl.OutputBufferLength);

    //
    // get device type
    //
    if (DeviceExtension->IsFDO)
    {
        //
        // perform ioctl for FDO
        //
        if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_HCD_DRIVERKEY_NAME)
        {
            //
            // check if sizee is at least >= USB_HCD_DRIVERKEY_NAME
            //
            if(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(USB_HCD_DRIVERKEY_NAME))
            {
                //
                // get device property size
                //
                Status = IoGetDeviceProperty(m_PhysicalDeviceObject, DevicePropertyDriverKeyName, 0, NULL, &ResultLength);

                DriverKey = (PUSB_HCD_DRIVERKEY_NAME)Irp->AssociatedIrp.SystemBuffer;

                //
                // check result
                //
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    //
                    // does the caller provide enough buffer space
                    //
                    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength >= ResultLength)
                    {
                        //
                        // it does
                        //
                        Status = IoGetDeviceProperty(m_PhysicalDeviceObject, DevicePropertyDriverKeyName, IoStack->Parameters.DeviceIoControl.OutputBufferLength, DriverKey->DriverKeyName, &ResultLength);

                        //DPRINT1("Result %S\n", DriverKey->DriverKeyName);
                    }

					//
					// FIXME
					//

                    //
                    // store result
                    //
                    DriverKey->ActualLength = ResultLength;
                    Irp->IoStatus.Information = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                //
                // buffer is certainly too small
                //
                Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(USB_HCD_DRIVERKEY_NAME);
            }
        }
    }

//

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
CHCDController::HandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST RawResourceList;
    PCM_RESOURCE_LIST TranslatedResourceList;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    LPGUID Guid;
    NTSTATUS Status;
    ULONG Index;

    DPRINT("HandlePnp\n");

    //
    // get device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT1("IRP_MN_START FDO: %lu\n", DeviceExtension->IsFDO);

            if (DeviceExtension->IsFDO)
            {
                //
                // first start lower device object
                //
                Status = SyncForwardIrp(m_NextDeviceObject, Irp);

                if (NT_SUCCESS(Status))
                {
                    //
                    // operation succeeded, lets start the device
                    //
                    RawResourceList = IoStack->Parameters.StartDevice.AllocatedResources;
                    TranslatedResourceList = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;

                    if (m_Hardware)
                    {
                        //
                        // start the hardware
                        //
                        Status = m_Hardware->PnpStart(RawResourceList, TranslatedResourceList);
                    }
                }

                //
                // HACK / FIXME: Windows XP SP3 fails to enumerate the PDO correctly, which
                //               causes the PDO device never to startup.
                //
                Status = SetDeviceInterface(TRUE);
            }
            else
            {
                //
                // start the PDO device
                //
                ASSERT(0);

                //
                //FIXME create the parent root hub device
                //

                //
                // register device interface 
                //
                Status = SetDeviceInterface(TRUE);
            }

            DPRINT1("IRP_MN_START FDO: %lu Status %x\n", DeviceExtension->IsFDO, Status);

            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DPRINT1("IRP_MN_QUERY_DEVICE_RELATIONS Type %lx FDO: \n", IoStack->Parameters.QueryDeviceRelations.Type, DeviceExtension->IsFDO);

            if (m_BusPDO == NULL)
            {
                //
                // create bus PDO
                //
                Status = CreatePDO(m_DriverObject, &m_BusPDO);

                if (!NT_SUCCESS(Status))
                {
                    //
                    // failed to create bus device object
                    //
                    break;
                }

                //
                // initialize extension
                //
                DeviceExtension = (PCOMMON_DEVICE_EXTENSION)m_BusPDO->DeviceExtension;
                DeviceExtension->IsFDO = FALSE;
                DeviceExtension->IsHub = FALSE;
                DeviceExtension->HcdController = (this);

                //
                // clear init flag
                //
                m_BusPDO->Flags &= ~DO_DEVICE_INITIALIZING;
            }

            if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
            {
                //
                // allocate device relations
                //
                DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
 
                if (!DeviceRelations)
                {
                    //
                    // no memory
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                //
                // init device relations
                //
                DeviceRelations->Count = 1;
                DeviceRelations->Objects [0] = m_BusPDO;

                ObReferenceObject(m_BusPDO);

                //
                // store result
                //
                Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
                Status = STATUS_SUCCESS;
            }
            break;
        }

        case IRP_MN_STOP_DEVICE:
        {
            DPRINT1("IRP_MN_STOP_DEVICE FDO: %lu\n", DeviceExtension->IsFDO);

            if (m_Hardware)
            {
                //
                // stop the hardware
                //
                Status = m_Hardware->PnpStop();
            }
            else
            {
                //
                // fake success 
                //
                Status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(Status))
            {
                //
                // stop lower device
                //
                Status = SyncForwardIrp(m_NextDeviceObject, Irp);
            }
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DPRINT1("IRP_MN_QUERY_CAPABILITIES FDO: %lu\n", DeviceExtension->IsFDO);

            if (DeviceExtension->IsFDO == FALSE)
            {
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
            }
            else
            {
                //
                // forward irp to next device object
                //
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(m_NextDeviceObject, Irp);
            }

            break;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT1("IRP_MN_QUERY_INTERFACE FDO %u\n", DeviceExtension->IsFDO);
            //
            // check if the device is FDO
            //
            if (DeviceExtension->IsFDO)
            {
                //
                // just pass the irp to next device object
                //
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(m_NextDeviceObject, Irp);
            }

            //
            // handle device interface requests
            //
            Status = HandleQueryInterface(IoStack);
            break;
        }
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            DPRINT1("IRP_MN_QUERY_BUS_INFORMATION FDO %lx\n", DeviceExtension->IsFDO);

            if (DeviceExtension->IsFDO == FALSE)
            {
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
            }
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT1("IRP_MN_REMOVE_DEVICE FDO: %lu\n", DeviceExtension->IsFDO);

            if (DeviceExtension->IsFDO == FALSE)
            {
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
                IoDeleteDevice(m_BusPDO);

                //
                // nullify pointer
                //
                m_BusPDO = 0;
                return STATUS_SUCCESS;
            }
            else
            {
                //
                // detach device from device stack
                //
                IoDetachDevice(m_NextDeviceObject);

                //
                // delete device
                //
                IoDeleteDevice(m_FunctionalDeviceObject);
            }

            Status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            if (DeviceExtension->IsFDO)
            {
                DPRINT1("HandlePnp> FDO Dispatch to lower device object %lu\n", IoStack->MinorFunction);

                //
                // forward irp to next device object
                //
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(m_NextDeviceObject, Irp);
            }
            else
            {
                DPRINT1("UNHANDLED PDO Request %lu\n", IoStack->MinorFunction);
            }
        }
    }

    //
    // store result and complete request
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
CHCDController::HandlePower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CHCDController::HandleQueryInterface(
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
CHCDController::CreateFDO(
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
        swprintf(CharDeviceName, L"\\Device\\USBFDO-%d", UsbDeviceNumber);

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

        //
        // check for success
        //
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
            DPRINT1("CreateFDO: Failed to create %wZ, Status %x\n", &DeviceName, Status);
            return Status;
        }
    }

    //
    // store FDO number
    //
    m_FDODeviceNumber = UsbDeviceNumber;

    DPRINT1("CreateFDO: DeviceName %wZ\n", &DeviceName);

    /* done */
    return Status;
}

NTSTATUS
CHCDController::SetDeviceInterface(
    BOOLEAN Enable)
{
    NTSTATUS Status;
    WCHAR LinkName[32];
    WCHAR FDOName[32];
    UNICODE_STRING Link, FDO;

    if (Enable)
    {
        //
        // register device interface
        //
        Status = IoRegisterDeviceInterface(m_PhysicalDeviceObject, &GUID_DEVINTERFACE_USB_HUB, 0, &m_HubDeviceInterfaceString);

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

        //
        // create legacy link
        //
        swprintf(LinkName, L"\\DosDevices\\HCD%d", m_PDODeviceNumber);
        swprintf(FDOName, L"\\Device\\USBFDO-%d", m_FDODeviceNumber);
        RtlInitUnicodeString(&Link, LinkName);
        RtlInitUnicodeString(&FDO, FDOName);

        //
        // create symbolic link
        //
        Status = IoCreateSymbolicLink(&Link, &FDO);

        if (!NT_SUCCESS(Status))
        {
            //
            // FIXME: handle me
            //
            ASSERT(0);
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
CHCDController::CreatePDO(
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
    m_PDODeviceNumber = UsbDeviceNumber;

    DPRINT1("CreateFDO: DeviceName %wZ\n", &DeviceName);

    /* done */
    return Status;
}

NTSTATUS
CreateHCDController(
    PHCDCONTROLLER *OutHcdController)
{
    PHCDCONTROLLER This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, 0) CHCDController(0);
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
    *OutHcdController = (PHCDCONTROLLER)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}
