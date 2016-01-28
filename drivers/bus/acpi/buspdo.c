#include "precomp.h"

#include <initguid.h>
#include <poclass.h>

#define NDEBUG
#include <debug.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_PDO_PnP)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceCaps)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceId)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceText)
#pragma alloc_text (PAGE, Bus_PDO_QueryResources)
#pragma alloc_text (PAGE, Bus_PDO_QueryResourceRequirements)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceRelations)
#pragma alloc_text (PAGE, Bus_PDO_QueryBusInformation)
#pragma alloc_text (PAGE, Bus_GetDeviceCapabilities)
#endif

NTSTATUS
Bus_PDO_PnP (
     PDEVICE_OBJECT       DeviceObject,
     PIRP                 Irp,
     PIO_STACK_LOCATION   IrpStack,
     PPDO_DEVICE_DATA     DeviceData
    )
{
    NTSTATUS                status;
    POWER_STATE             state;
    struct acpi_device      *device = NULL;

    PAGED_CODE ();

    if (DeviceData->AcpiHandle)
        acpi_bus_get_device(DeviceData->AcpiHandle, &device);

    //
    // NB: Because we are a bus enumerator, we have no one to whom we could
    // defer these irps.  Therefore we do not pass them down but merely
    // return them.
    //

    switch (IrpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:
        //
        // Here we do what ever initialization and ``turning on'' that is
        // required to allow others to access this device.
        // Power up the device.
        //
        if (DeviceData->AcpiHandle && acpi_bus_power_manageable(DeviceData->AcpiHandle) &&
            !ACPI_SUCCESS(acpi_bus_set_power(DeviceData->AcpiHandle, ACPI_STATE_D0)))
        {
            DPRINT1("Device %x failed to start!\n", DeviceData->AcpiHandle);
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        DeviceData->InterfaceName.Length = 0;
        status = STATUS_SUCCESS;

        if (!device)
        {
            status = IoRegisterDeviceInterface(DeviceData->Common.Self,
                                               &GUID_DEVICE_SYS_BUTTON,
                                               NULL,
                                               &DeviceData->InterfaceName);
        }
        else if (device->flags.hardware_id &&
                 strstr(device->pnp.hardware_id, ACPI_THERMAL_HID))
        {
            status = IoRegisterDeviceInterface(DeviceData->Common.Self,
                                               &GUID_DEVICE_THERMAL_ZONE,
                                               NULL,
                                               &DeviceData->InterfaceName);
        }
        else if (device->flags.hardware_id &&
                 strstr(device->pnp.hardware_id, ACPI_BUTTON_HID_LID))
        {
            status = IoRegisterDeviceInterface(DeviceData->Common.Self,
                                               &GUID_DEVICE_LID,
                                               NULL,
                                               &DeviceData->InterfaceName);
        }
        else if (device->flags.hardware_id &&
                 strstr(device->pnp.hardware_id, ACPI_PROCESSOR_HID))
        {
            status = IoRegisterDeviceInterface(DeviceData->Common.Self,
                                               &GUID_DEVICE_PROCESSOR,
                                               NULL,
                                               &DeviceData->InterfaceName);
        }

        /* Failure to register an interface is not a fatal failure so don't return a failure status */
        if (NT_SUCCESS(status) && DeviceData->InterfaceName.Length != 0)
            IoSetDeviceInterfaceState(&DeviceData->InterfaceName, TRUE);

        state.DeviceState = PowerDeviceD0;
        PoSetPowerState(DeviceData->Common.Self, DevicePowerState, state);
        DeviceData->Common.DevicePowerState = PowerDeviceD0;
        SET_NEW_PNP_STATE(DeviceData->Common, Started);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:

        if (DeviceData->InterfaceName.Length != 0)
            IoSetDeviceInterfaceState(&DeviceData->InterfaceName, FALSE);

        //
        // Here we shut down the device and give up and unmap any resources
        // we acquired for the device.
        //
        if (DeviceData->AcpiHandle && acpi_bus_power_manageable(DeviceData->AcpiHandle) &&
            !ACPI_SUCCESS(acpi_bus_set_power(DeviceData->AcpiHandle, ACPI_STATE_D3)))
        {
            DPRINT1("Device %x failed to stop!\n", DeviceData->AcpiHandle);
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        state.DeviceState = PowerDeviceD3;
        PoSetPowerState(DeviceData->Common.Self, DevicePowerState, state);
        DeviceData->Common.DevicePowerState = PowerDeviceD3;
        SET_NEW_PNP_STATE(DeviceData->Common, Stopped);
        status = STATUS_SUCCESS;
        break;


    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // No reason here why we can't stop the device.
        // If there were a reason we should speak now, because answering success
        // here may result in a stop device irp.
        //

        SET_NEW_PNP_STATE(DeviceData->Common, StopPending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // The stop was canceled.  Whatever state we set, or resources we put
        // on hold in anticipation of the forthcoming STOP device IRP should be
        // put back to normal.  Someone, in the long list of concerned parties,
        // has failed the stop device query.
        //

        //
        // First check to see whether you have received cancel-stop
        // without first receiving a query-stop. This could happen if someone
        // above us fails a query-stop and passes down the subsequent
        // cancel-stop.
        //

        if (StopPending == DeviceData->Common.DevicePnPState)
        {
            //
            // We did receive a query-stop, so restore.
            //
            RESTORE_PREVIOUS_PNP_STATE(DeviceData->Common);
        }
        status = STATUS_SUCCESS;// We must not fail this IRP.
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // We handle REMOVE_DEVICE just like STOP_DEVICE. This is because
        // the device is still physically present (or at least we don't know any better)
        // so we have to retain the PDO after stopping and removing power from it.
        //
        if (DeviceData->InterfaceName.Length != 0)
            IoSetDeviceInterfaceState(&DeviceData->InterfaceName, FALSE);

        if (DeviceData->AcpiHandle && acpi_bus_power_manageable(DeviceData->AcpiHandle) &&
            !ACPI_SUCCESS(acpi_bus_set_power(DeviceData->AcpiHandle, ACPI_STATE_D3)))
        {
            DPRINT1("Device %x failed to enter D3!\n", DeviceData->AcpiHandle);
            state.DeviceState = PowerDeviceD3;
            PoSetPowerState(DeviceData->Common.Self, DevicePowerState, state);
            DeviceData->Common.DevicePowerState = PowerDeviceD3;
        }
        
        SET_NEW_PNP_STATE(DeviceData->Common, Stopped);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        SET_NEW_PNP_STATE(DeviceData->Common, RemovalPending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        if (RemovalPending == DeviceData->Common.DevicePnPState)
        {
            RESTORE_PREVIOUS_PNP_STATE(DeviceData->Common);
        }
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_CAPABILITIES:

        //
        // Return the capabilities of a device, such as whether the device
        // can be locked or ejected..etc
        //

        status = Bus_PDO_QueryDeviceCaps(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_ID:

        // Query the IDs of the device
        status = Bus_PDO_QueryDeviceId(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        DPRINT("\tQueryDeviceRelation Type: %s\n",DbgDeviceRelationString(\
                    IrpStack->Parameters.QueryDeviceRelations.Type));

        status = Bus_PDO_QueryDeviceRelations(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        status = Bus_PDO_QueryDeviceText(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_RESOURCES:

        status = Bus_PDO_QueryResources(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        status = Bus_PDO_QueryResourceRequirements(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_BUS_INFORMATION:

        status = Bus_PDO_QueryBusInformation(DeviceData, Irp);

        break;

    case IRP_MN_QUERY_INTERFACE:

        status = Bus_PDO_QueryInterface(DeviceData, Irp);

        break;


    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:

        //
        // OPTIONAL for bus drivers.
        // The PnP Manager sends this IRP to a device
        // stack so filter and function drivers can adjust the
        // resources required by the device, if appropriate.
        //

        //break;

    //case IRP_MN_QUERY_PNP_DEVICE_STATE:

        //
        // OPTIONAL for bus drivers.
        // The PnP Manager sends this IRP after the drivers for
        // a device return success from the IRP_MN_START_DEVICE
        // request. The PnP Manager also sends this IRP when a
        // driver for the device calls IoInvalidateDeviceState.
        //

        // break;

    //case IRP_MN_READ_CONFIG:
    //case IRP_MN_WRITE_CONFIG:

        //
        // Bus drivers for buses with configuration space must handle
        // this request for their child devices. Our devices don't
        // have a config space.
        //

        // break;

    //case IRP_MN_SET_LOCK:

        // break;

    default:

        //
        // For PnP requests to the PDO that we do not understand we should
        // return the IRP WITHOUT setting the status or information fields.
        // These fields may have already been set by a filter (eg acpi).
        status = Irp->IoStatus.Status;

        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Bus_PDO_QueryDeviceCaps(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
{

    PIO_STACK_LOCATION      stack;
    PDEVICE_CAPABILITIES    deviceCapabilities;
    struct acpi_device *device = NULL;
    ULONG i;

    PAGED_CODE ();

    if (DeviceData->AcpiHandle)
        acpi_bus_get_device(DeviceData->AcpiHandle, &device);

    stack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the packet.
    //
    deviceCapabilities=stack->Parameters.DeviceCapabilities.Capabilities;

    //
    // Set the capabilities.
    //

    if (deviceCapabilities->Version != 1 ||
            deviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES))
    {
       return STATUS_UNSUCCESSFUL;
    }

    deviceCapabilities->D1Latency = 0;
    deviceCapabilities->D2Latency = 0;
    deviceCapabilities->D3Latency = 0;

    deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    deviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
    deviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    deviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;

    for (i = 0; i < ACPI_D_STATE_COUNT && device; i++)
    {
        if (!device->power.states[i].flags.valid)
            continue;

        switch (i)
        {
           case ACPI_STATE_D0:
              deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
              break;

           case ACPI_STATE_D1:
              deviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
              deviceCapabilities->D1Latency = device->power.states[i].latency;
              break;

           case ACPI_STATE_D2:
              deviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
              deviceCapabilities->D2Latency = device->power.states[i].latency;
              break;

           case ACPI_STATE_D3:
              deviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
              deviceCapabilities->D3Latency = device->power.states[i].latency;
              break;
        }
    }

    // We can wake the system from D1
    deviceCapabilities->DeviceWake = PowerDeviceD1;


    deviceCapabilities->DeviceD1 =
        (deviceCapabilities->DeviceState[PowerSystemSleeping1] == PowerDeviceD1) ? TRUE : FALSE;
    deviceCapabilities->DeviceD2 =
        (deviceCapabilities->DeviceState[PowerSystemSleeping2] == PowerDeviceD2) ? TRUE : FALSE;

    deviceCapabilities->WakeFromD0 = FALSE;
    deviceCapabilities->WakeFromD1 = TRUE; //Yes we can
    deviceCapabilities->WakeFromD2 = FALSE;
    deviceCapabilities->WakeFromD3 = FALSE;

    if (device)
    {
       deviceCapabilities->LockSupported = device->flags.lockable;
       deviceCapabilities->EjectSupported = device->flags.ejectable;
       deviceCapabilities->HardwareDisabled = !device->status.enabled && !device->status.functional;
       deviceCapabilities->Removable = device->flags.removable;
       deviceCapabilities->SurpriseRemovalOK = device->flags.suprise_removal_ok;
       deviceCapabilities->UniqueID = device->flags.unique_id;
       deviceCapabilities->NoDisplayInUI = !device->status.show_in_ui;
       deviceCapabilities->Address = device->pnp.bus_address;
    }

    if (!device ||
        (device->flags.hardware_id &&
         (strstr(device->pnp.hardware_id, ACPI_BUTTON_HID_LID) ||
          strstr(device->pnp.hardware_id, ACPI_THERMAL_HID) ||
          strstr(device->pnp.hardware_id, ACPI_PROCESSOR_HID))))
    {
        /* Allow ACPI to control the device if it is a lid button,
         * a thermal zone, a processor, or a fixed feature button */
        deviceCapabilities->RawDeviceOK = TRUE;
    }

    deviceCapabilities->SilentInstall = FALSE;
    deviceCapabilities->UINumber = (ULONG)-1;

    return STATUS_SUCCESS;

}

NTSTATUS
Bus_PDO_QueryDeviceId(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
{
    PIO_STACK_LOCATION      stack;
    PWCHAR                  buffer, src;
    WCHAR                   temp[256];
    ULONG                   length, i;
    NTSTATUS                status = STATUS_SUCCESS;
    struct acpi_device *Device;

    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryId.IdType) {

    case BusQueryDeviceID:

        /* This is a REG_SZ value */

        if (DeviceData->AcpiHandle)
        {
            acpi_bus_get_device(DeviceData->AcpiHandle, &Device);

            length = swprintf(temp,
                              L"ACPI\\%hs",
                              Device->pnp.hardware_id);
        }
        else
        {
            /* We know it's a fixed feature button because
             * these are direct children of the ACPI root device
             * and therefore have no handle
             */
            length = swprintf(temp,
                              L"ACPI\\FixedButton");
        }

        temp[length++] = UNICODE_NULL;

        NT_ASSERT(length * sizeof(WCHAR) <= sizeof(temp));

        buffer = ExAllocatePoolWithTag(PagedPool, length * sizeof(WCHAR), 'IpcA');

        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        RtlCopyMemory (buffer, temp, length * sizeof(WCHAR));
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        DPRINT("BusQueryDeviceID: %ls\n",buffer);
        break;

    case BusQueryInstanceID:

        /* This is a REG_SZ value */

        /* See comment in BusQueryDeviceID case */
        if(DeviceData->AcpiHandle)
        {
           acpi_bus_get_device(DeviceData->AcpiHandle, &Device);

           if (Device->flags.unique_id)
              length = swprintf(temp,
                                L"%hs",
                                Device->pnp.unique_id);
           else
              /* FIXME: Generate unique id! */
              length = swprintf(temp, L"%ls", L"0000");
        }
        else
        {
           /* FIXME: Generate unique id! */
           length = swprintf(temp, L"%ls", L"0000");
        }

        temp[length++] = UNICODE_NULL;

        NT_ASSERT(length * sizeof(WCHAR) <= sizeof(temp));

        buffer = ExAllocatePoolWithTag(PagedPool, length * sizeof(WCHAR), 'IpcA');
        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        RtlCopyMemory (buffer, temp, length * sizeof (WCHAR));
        DPRINT("BusQueryInstanceID: %ls\n",buffer);
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        break;

    case BusQueryHardwareIDs:

        /* This is a REG_MULTI_SZ value */
        length = 0;
        status = STATUS_NOT_SUPPORTED;

        /* See comment in BusQueryDeviceID case */
        if (DeviceData->AcpiHandle)
        {
            acpi_bus_get_device(DeviceData->AcpiHandle, &Device);
            
            if (!Device->flags.hardware_id)
            {
                /* We don't have the ID to satisfy this request */
                break;
            }

            DPRINT("Device name: %s\n", Device->pnp.device_name);
            DPRINT("Hardware ID: %s\n", Device->pnp.hardware_id);

            if (strcmp(Device->pnp.hardware_id, "Processor") == 0)
            {
                length = ProcessorHardwareIds.Length / sizeof(WCHAR);
                src = ProcessorHardwareIds.Buffer;
            }
            else
            {
                length += swprintf(&temp[length],
                                   L"ACPI\\%hs",
                                   Device->pnp.hardware_id);
                temp[length++] = UNICODE_NULL;

                length += swprintf(&temp[length],
                                   L"*%hs",
                                   Device->pnp.hardware_id);
                temp[length++] = UNICODE_NULL;
                temp[length++] = UNICODE_NULL;
                src = temp;
            }
        }
        else
        {
            length += swprintf(&temp[length],
                               L"ACPI\\FixedButton");
            temp[length++] = UNICODE_NULL;

            length += swprintf(&temp[length],
                               L"*FixedButton");
            temp[length++] = UNICODE_NULL;
            temp[length++] = UNICODE_NULL;
            src = temp;
        }

        NT_ASSERT(length * sizeof(WCHAR) <= sizeof(temp));

        buffer = ExAllocatePoolWithTag(PagedPool, length * sizeof(WCHAR), 'IpcA');

        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        RtlCopyMemory (buffer, src, length * sizeof(WCHAR));
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        DPRINT("BusQueryHardwareIDs: %ls\n",buffer);
        status = STATUS_SUCCESS;
        break;

    case BusQueryCompatibleIDs:

        /* This is a REG_MULTI_SZ value */
        length = 0;
        status = STATUS_NOT_SUPPORTED;

        /* See comment in BusQueryDeviceID case */
        if (DeviceData->AcpiHandle)
        {
            acpi_bus_get_device(DeviceData->AcpiHandle, &Device);

            if (!Device->flags.hardware_id)
            {
                /* We don't have the ID to satisfy this request */
                break;
            }
            
            DPRINT("Device name: %s\n", Device->pnp.device_name);
            DPRINT("Hardware ID: %s\n", Device->pnp.hardware_id);
            
            if (strcmp(Device->pnp.hardware_id, "Processor") == 0)
            {
                length += swprintf(&temp[length],
                                   L"ACPI\\%hs",
                                   Device->pnp.hardware_id);
                temp[length++] = UNICODE_NULL;

                length += swprintf(&temp[length],
                                   L"*%hs",
                                   Device->pnp.hardware_id);
                temp[length++] = UNICODE_NULL;
                temp[length++] = UNICODE_NULL;
            }
            else if (Device->flags.compatible_ids)
            {
                for (i = 0; i < Device->pnp.cid_list->Count; i++)
                {
                    length += swprintf(&temp[length],
                                   L"ACPI\\%hs",
                                   Device->pnp.cid_list->Ids[i].String);
                    temp[length++] = UNICODE_NULL;
                    
                    length += swprintf(&temp[length],
                                   L"*%hs",
                                   Device->pnp.cid_list->Ids[i].String);
                    temp[length++] = UNICODE_NULL;
                }
                
                temp[length++] = UNICODE_NULL;
            }
            else
            {
                /* No compatible IDs */
                break;
            }
            
            NT_ASSERT(length * sizeof(WCHAR) <= sizeof(temp));

            buffer = ExAllocatePoolWithTag(PagedPool, length * sizeof(WCHAR), 'IpcA');
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory (buffer, temp, length * sizeof(WCHAR));
            Irp->IoStatus.Information = (ULONG_PTR) buffer;
            DPRINT("BusQueryCompatibleIDs: %ls\n",buffer);
            status = STATUS_SUCCESS;
        }
        break;

    default:
        status = Irp->IoStatus.Status;
    }
    return status;
}

NTSTATUS
Bus_PDO_QueryDeviceText(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
{
    PWCHAR  Buffer, Temp;
    PIO_STACK_LOCATION   stack;
    NTSTATUS    status = Irp->IoStatus.Status;
    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryDeviceText.DeviceTextType) {

    case DeviceTextDescription:

        if (!Irp->IoStatus.Information) {
          if (wcsstr (DeviceData->HardwareIDs, L"PNP000") != 0)
            Temp = L"Programmable interrupt controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP010") != 0)
            Temp = L"System timer";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP020") != 0)
            Temp = L"DMA controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP03") != 0)
            Temp = L"Keyboard";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP040") != 0)
            Temp = L"Parallel port";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP05") != 0)
            Temp = L"Serial port";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP06") != 0)
            Temp = L"Disk controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP07") != 0)
            Temp = L"Disk controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP09") != 0)
            Temp = L"Display adapter";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0A0") != 0)
            Temp = L"Bus controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0E0") != 0)
            Temp = L"PCMCIA controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0F") != 0)
            Temp = L"Mouse device";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP8") != 0)
            Temp = L"Network adapter";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNPA0") != 0)
            Temp = L"SCSI controller";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNPB0") != 0)
            Temp = L"Multimedia device";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNPC00") != 0)
            Temp = L"Modem";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0C") != 0)
            Temp = L"Power Button";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0E") != 0)
            Temp = L"Sleep Button";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0D") != 0)
            Temp = L"Lid Switch";
          else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C09") != 0)
            Temp = L"ACPI Embedded Controller";
           else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0B") != 0)
            Temp = L"ACPI Fan";
           else if (wcsstr(DeviceData->HardwareIDs, L"PNP0A03") != 0 ||
                    wcsstr(DeviceData->HardwareIDs, L"PNP0A08") != 0 )
            Temp = L"PCI Root Bridge";
           else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0A") != 0)
            Temp = L"ACPI Battery";
           else if (wcsstr(DeviceData->HardwareIDs, L"PNP0C0F") != 0)
            Temp = L"PCI Interrupt Link";
           else if (wcsstr(DeviceData->HardwareIDs, L"ACPI_PWR") != 0)
            Temp = L"ACPI Power Resource";
           else if (wcsstr(DeviceData->HardwareIDs, L"Processor") != 0)
           {
               if (ProcessorNameString != NULL)
                   Temp = ProcessorNameString;
               else
                   Temp = L"Processor";
           }
           else if (wcsstr(DeviceData->HardwareIDs, L"ThermalZone") != 0)
            Temp = L"ACPI Thermal Zone";
           else if (wcsstr(DeviceData->HardwareIDs, L"ACPI0002") != 0)
            Temp = L"Smart Battery";
           else if (wcsstr(DeviceData->HardwareIDs, L"ACPI0003") != 0)
            Temp = L"AC Adapter";
           /* Simply checking if AcpiHandle is NULL eliminates the need to check
            * for the 4 different names that ACPI knows the fixed feature button as internally
            */
           else if (!DeviceData->AcpiHandle)
            Temp = L"ACPI Fixed Feature Button";
          else
            Temp = L"Other ACPI device";

            Buffer = ExAllocatePoolWithTag(PagedPool, (wcslen(Temp) + 1) * sizeof(WCHAR), 'IpcA');

            if (!Buffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory (Buffer, Temp, (wcslen(Temp) + 1) * sizeof(WCHAR));

            DPRINT("\tDeviceTextDescription :%ws\n", Buffer);

            Irp->IoStatus.Information = (ULONG_PTR) Buffer;
            status = STATUS_SUCCESS;
        }
        break;

    default:
        break;
    }

    return status;

}

NTSTATUS
Bus_PDO_QueryResources(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
{
    ULONG NumberOfResources = 0;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    ACPI_STATUS AcpiStatus;
    ACPI_BUFFER Buffer;
    ACPI_RESOURCE* resource;
    ULONG ResourceListSize;
    ULONG i;
    ULONGLONG BusNumber;
    struct acpi_device *device;

    if (!DeviceData->AcpiHandle)
    {
        return Irp->IoStatus.Status;
    }

    /* A bus number resource is not included in the list of current resources
     * for the root PCI bus so we manually query one here and if we find it
     * we create a resource list and add a bus number descriptor to it */
    if (wcsstr(DeviceData->HardwareIDs, L"PNP0A03") != 0 ||
        wcsstr(DeviceData->HardwareIDs, L"PNP0A08") != 0)
    {
        acpi_bus_get_device(DeviceData->AcpiHandle, &device);

        AcpiStatus = acpi_evaluate_integer(DeviceData->AcpiHandle, "_BBN", NULL, &BusNumber);
        if (AcpiStatus != AE_OK)
        {
#if 0
            if (device->flags.unique_id)
            {
                /* FIXME: Try the unique ID */
            }
            else
#endif
            {
                BusNumber = 0;
                DPRINT1("Failed to find a bus number\n");
            }
        }
        else
        {
            DPRINT("Using _BBN for bus number\n");
        }

        DPRINT("Found PCI root hub: %d\n", BusNumber);

        ResourceListSize = sizeof(CM_RESOURCE_LIST);
        ResourceList = ExAllocatePoolWithTag(PagedPool, ResourceListSize, 'RpcA');
        if (!ResourceList)
            return STATUS_INSUFFICIENT_RESOURCES;

        ResourceList->Count = 1;
        ResourceList->List[0].InterfaceType = Internal;
        ResourceList->List[0].BusNumber = 0;
        ResourceList->List[0].PartialResourceList.Version = 1;
        ResourceList->List[0].PartialResourceList.Revision = 1;
        ResourceList->List[0].PartialResourceList.Count = 1;
        ResourceDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

        ResourceDescriptor->Type = CmResourceTypeBusNumber;
        ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        ResourceDescriptor->u.BusNumber.Start = BusNumber;
        ResourceDescriptor->u.BusNumber.Length = 1;

        Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
        return STATUS_SUCCESS;
    }

    /* Get current resources */
    Buffer.Length = 0;
    AcpiStatus = AcpiGetCurrentResources(DeviceData->AcpiHandle, &Buffer);
    if ((!ACPI_SUCCESS(AcpiStatus) && AcpiStatus != AE_BUFFER_OVERFLOW) ||
        Buffer.Length == 0)
    {
      return Irp->IoStatus.Status;
    }

    Buffer.Pointer = ExAllocatePoolWithTag(PagedPool, Buffer.Length, 'BpcA');
    if (!Buffer.Pointer)
      return STATUS_INSUFFICIENT_RESOURCES;

    AcpiStatus = AcpiGetCurrentResources(DeviceData->AcpiHandle, &Buffer);
    if (!ACPI_SUCCESS(AcpiStatus))
    {
      DPRINT1("AcpiGetCurrentResources #2 failed (0x%x)\n", AcpiStatus);
      ASSERT(FALSE);
      return STATUS_UNSUCCESSFUL;
    }

    resource= Buffer.Pointer;
    /* Count number of resources */
    while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG)
    {
        switch (resource->Type)
        {
            case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            {
                ACPI_RESOURCE_EXTENDED_IRQ *irq_data = (ACPI_RESOURCE_EXTENDED_IRQ*) &resource->Data;
                if (irq_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                NumberOfResources += irq_data->InterruptCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_IRQ:
            {
                ACPI_RESOURCE_IRQ *irq_data = (ACPI_RESOURCE_IRQ*) &resource->Data;
                NumberOfResources += irq_data->InterruptCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_DMA:
            {
                ACPI_RESOURCE_DMA *dma_data = (ACPI_RESOURCE_DMA*) &resource->Data;
                NumberOfResources += dma_data->ChannelCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS16:
            case ACPI_RESOURCE_TYPE_ADDRESS32:
            case ACPI_RESOURCE_TYPE_ADDRESS64:
            case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
            {
                ACPI_RESOURCE_ADDRESS *addr_res = (ACPI_RESOURCE_ADDRESS*) &resource->Data;
                if (addr_res->ProducerConsumer == ACPI_PRODUCER)
                    break;
                NumberOfResources++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY24:
            case ACPI_RESOURCE_TYPE_MEMORY32:
            case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            case ACPI_RESOURCE_TYPE_FIXED_IO:
            case ACPI_RESOURCE_TYPE_IO:
            {
                NumberOfResources++;
                break;
            }
            default:
            {
                DPRINT1("Unknown resource type: %d\n", resource->Type);
                break;
            }
        }
        resource = ACPI_NEXT_RESOURCE(resource);
    }

    /* Allocate memory */
    ResourceListSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (NumberOfResources - 1);
    ResourceList = ExAllocatePoolWithTag(PagedPool, ResourceListSize, 'RpcA');

    if (!ResourceList)
    {
        ExFreePoolWithTag(Buffer.Pointer, 'BpcA');
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Internal; /* FIXME */
    ResourceList->List[0].BusNumber = 0; /* We're the only ACPI bus device in the system */
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = NumberOfResources;
    ResourceDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

    /* Fill resources list structure */
        resource = Buffer.Pointer;
    while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG)
    {
        switch (resource->Type)
        {
            case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            {
                ACPI_RESOURCE_EXTENDED_IRQ *irq_data = (ACPI_RESOURCE_EXTENDED_IRQ*) &resource->Data;
                if (irq_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                for (i = 0; i < irq_data->InterruptCount; i++)
                {
                    ResourceDescriptor->Type = CmResourceTypeInterrupt;

                    ResourceDescriptor->ShareDisposition =
                    (irq_data->Sharable == ACPI_SHARED ? CmResourceShareShared : CmResourceShareDeviceExclusive);
                    ResourceDescriptor->Flags =
                    (irq_data->Triggering == ACPI_LEVEL_SENSITIVE ? CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE : CM_RESOURCE_INTERRUPT_LATCHED);
                    ResourceDescriptor->u.Interrupt.Level =
                    ResourceDescriptor->u.Interrupt.Vector = irq_data->Interrupts[i];
                    ResourceDescriptor->u.Interrupt.Affinity = (KAFFINITY)(-1);

                    ResourceDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_IRQ:
            {
                ACPI_RESOURCE_IRQ *irq_data = (ACPI_RESOURCE_IRQ*) &resource->Data;
                for (i = 0; i < irq_data->InterruptCount; i++)
                {
                    ResourceDescriptor->Type = CmResourceTypeInterrupt;

                    ResourceDescriptor->ShareDisposition =
                    (irq_data->Sharable == ACPI_SHARED ? CmResourceShareShared : CmResourceShareDeviceExclusive);
                    ResourceDescriptor->Flags =
                    (irq_data->Triggering == ACPI_LEVEL_SENSITIVE ? CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE : CM_RESOURCE_INTERRUPT_LATCHED);
                    ResourceDescriptor->u.Interrupt.Level =
                    ResourceDescriptor->u.Interrupt.Vector = irq_data->Interrupts[i];
                    ResourceDescriptor->u.Interrupt.Affinity = (KAFFINITY)(-1);

                    ResourceDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_DMA:
            {
                ACPI_RESOURCE_DMA *dma_data = (ACPI_RESOURCE_DMA*) &resource->Data;
                for (i = 0; i < dma_data->ChannelCount; i++)
                {
                    ResourceDescriptor->Type = CmResourceTypeDma;
                    ResourceDescriptor->Flags = 0;
                    switch (dma_data->Type)
                    {
                        case ACPI_TYPE_A: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
                        case ACPI_TYPE_B: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
                        case ACPI_TYPE_F: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
                    }
                    if (dma_data->BusMaster == ACPI_BUS_MASTER)
                        ResourceDescriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
                    switch (dma_data->Transfer)
                    {
                        case ACPI_TRANSFER_8: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_8; break;
                        case ACPI_TRANSFER_16: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_16; break;
                        case ACPI_TRANSFER_8_16: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_8_AND_16; break;
                    }
                    ResourceDescriptor->u.Dma.Channel = dma_data->Channels[i];

                    ResourceDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_IO:
            {
                ACPI_RESOURCE_IO *io_data = (ACPI_RESOURCE_IO*) &resource->Data;
                ResourceDescriptor->Type = CmResourceTypePort;
                ResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
                ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                if (io_data->IoDecode == ACPI_DECODE_16)
                    ResourceDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
                else
                    ResourceDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
                ResourceDescriptor->u.Port.Start.QuadPart = io_data->Minimum;
                ResourceDescriptor->u.Port.Length = io_data->AddressLength;

                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_FIXED_IO:
            {
                ACPI_RESOURCE_FIXED_IO *io_data = (ACPI_RESOURCE_FIXED_IO*) &resource->Data;
                ResourceDescriptor->Type = CmResourceTypePort;
                ResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
                ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                ResourceDescriptor->u.Port.Start.QuadPart = io_data->Address;
                ResourceDescriptor->u.Port.Length = io_data->AddressLength;

                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS16:
            {
                ACPI_RESOURCE_ADDRESS16 *addr16_data = (ACPI_RESOURCE_ADDRESS16*) &resource->Data;
                if (addr16_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                if (addr16_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypeBusNumber;
                    ResourceDescriptor->ShareDisposition = CmResourceShareShared;
                    ResourceDescriptor->Flags = 0;
                    ResourceDescriptor->u.BusNumber.Start = addr16_data->Address.Minimum;
                    ResourceDescriptor->u.BusNumber.Length = addr16_data->Address.AddressLength;
                }
                else if (addr16_data->ResourceType == ACPI_IO_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypePort;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr16_data->Decode == ACPI_POS_DECODE)
                        ResourceDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    ResourceDescriptor->u.Port.Start.QuadPart = addr16_data->Address.Minimum;
                    ResourceDescriptor->u.Port.Length = addr16_data->Address.AddressLength;
                }
                else
                {
                    ResourceDescriptor->Type = CmResourceTypeMemory;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = 0;
                    if (addr16_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr16_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    ResourceDescriptor->u.Memory.Start.QuadPart = addr16_data->Address.Minimum;
                    ResourceDescriptor->u.Memory.Length = addr16_data->Address.AddressLength;
                }
                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS32:
            {
                ACPI_RESOURCE_ADDRESS32 *addr32_data = (ACPI_RESOURCE_ADDRESS32*) &resource->Data;
                if (addr32_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                if (addr32_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypeBusNumber;
                    ResourceDescriptor->ShareDisposition = CmResourceShareShared;
                    ResourceDescriptor->Flags = 0;
                    ResourceDescriptor->u.BusNumber.Start = addr32_data->Address.Minimum;
                    ResourceDescriptor->u.BusNumber.Length = addr32_data->Address.AddressLength;
                }
                else if (addr32_data->ResourceType == ACPI_IO_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypePort;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr32_data->Decode == ACPI_POS_DECODE)
                        ResourceDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    ResourceDescriptor->u.Port.Start.QuadPart = addr32_data->Address.Minimum;
                    ResourceDescriptor->u.Port.Length = addr32_data->Address.AddressLength;
                }
                else
                {
                    ResourceDescriptor->Type = CmResourceTypeMemory;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = 0;
                    if (addr32_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr32_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    ResourceDescriptor->u.Memory.Start.QuadPart = addr32_data->Address.Minimum;
                    ResourceDescriptor->u.Memory.Length = addr32_data->Address.AddressLength;
                }
                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS64:
            {
                ACPI_RESOURCE_ADDRESS64 *addr64_data = (ACPI_RESOURCE_ADDRESS64*) &resource->Data;
                if (addr64_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                if (addr64_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    DPRINT1("64-bit bus address is not supported!\n");
                    ResourceDescriptor->Type = CmResourceTypeBusNumber;
                    ResourceDescriptor->ShareDisposition = CmResourceShareShared;
                    ResourceDescriptor->Flags = 0;
                    ResourceDescriptor->u.BusNumber.Start = (ULONG)addr64_data->Address.Minimum;
                    ResourceDescriptor->u.BusNumber.Length = addr64_data->Address.AddressLength;
                }
                else if (addr64_data->ResourceType == ACPI_IO_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypePort;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr64_data->Decode == ACPI_POS_DECODE)
                        ResourceDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    ResourceDescriptor->u.Port.Start.QuadPart = addr64_data->Address.Minimum;
                    ResourceDescriptor->u.Port.Length = addr64_data->Address.AddressLength;
                }
                else
                {
                    ResourceDescriptor->Type = CmResourceTypeMemory;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = 0;
                    if (addr64_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr64_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    ResourceDescriptor->u.Memory.Start.QuadPart = addr64_data->Address.Minimum;
                    ResourceDescriptor->u.Memory.Length = addr64_data->Address.AddressLength;
                }
                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
            {
                ACPI_RESOURCE_EXTENDED_ADDRESS64 *addr64_data = (ACPI_RESOURCE_EXTENDED_ADDRESS64*) &resource->Data;
                if (addr64_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                if (addr64_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    DPRINT1("64-bit bus address is not supported!\n");
                    ResourceDescriptor->Type = CmResourceTypeBusNumber;
                    ResourceDescriptor->ShareDisposition = CmResourceShareShared;
                    ResourceDescriptor->Flags = 0;
                    ResourceDescriptor->u.BusNumber.Start = (ULONG)addr64_data->Address.Minimum;
                    ResourceDescriptor->u.BusNumber.Length = addr64_data->Address.AddressLength;
                }
                else if (addr64_data->ResourceType == ACPI_IO_RANGE)
                {
                    ResourceDescriptor->Type = CmResourceTypePort;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr64_data->Decode == ACPI_POS_DECODE)
                        ResourceDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    ResourceDescriptor->u.Port.Start.QuadPart = addr64_data->Address.Minimum;
                    ResourceDescriptor->u.Port.Length = addr64_data->Address.AddressLength;
                }
                else
                {
                    ResourceDescriptor->Type = CmResourceTypeMemory;
                    ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    ResourceDescriptor->Flags = 0;
                    if (addr64_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr64_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    ResourceDescriptor->u.Memory.Start.QuadPart = addr64_data->Address.Minimum;
                    ResourceDescriptor->u.Memory.Length = addr64_data->Address.AddressLength;
                }
                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY24:
            {
                ACPI_RESOURCE_MEMORY24 *mem24_data = (ACPI_RESOURCE_MEMORY24*) &resource->Data;
                ResourceDescriptor->Type = CmResourceTypeMemory;
                ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                ResourceDescriptor->Flags = CM_RESOURCE_MEMORY_24;
                if (mem24_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                ResourceDescriptor->u.Memory.Start.QuadPart = mem24_data->Minimum;
                ResourceDescriptor->u.Memory.Length = mem24_data->AddressLength;

                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY32:
            {
                ACPI_RESOURCE_MEMORY32 *mem32_data = (ACPI_RESOURCE_MEMORY32*) &resource->Data;
                ResourceDescriptor->Type = CmResourceTypeMemory;
                ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                ResourceDescriptor->Flags = 0;
                if (mem32_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                ResourceDescriptor->u.Memory.Start.QuadPart = mem32_data->Minimum;
                ResourceDescriptor->u.Memory.Length = mem32_data->AddressLength;

                ResourceDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            {
                ACPI_RESOURCE_FIXED_MEMORY32 *memfixed32_data = (ACPI_RESOURCE_FIXED_MEMORY32*) &resource->Data;
                ResourceDescriptor->Type = CmResourceTypeMemory;
                ResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                ResourceDescriptor->Flags = 0;
                if (memfixed32_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    ResourceDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                ResourceDescriptor->u.Memory.Start.QuadPart = memfixed32_data->Address;
                ResourceDescriptor->u.Memory.Length = memfixed32_data->AddressLength;

                ResourceDescriptor++;
                break;
            }
            default:
            {
                break;
            }
        }
        resource = ACPI_NEXT_RESOURCE(resource);
    }

    ExFreePoolWithTag(Buffer.Pointer, 'BpcA');
    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
    return STATUS_SUCCESS;
}

NTSTATUS
Bus_PDO_QueryResourceRequirements(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
{
    ULONG NumberOfResources = 0;
    ACPI_STATUS AcpiStatus;
    ACPI_BUFFER Buffer;
    ACPI_RESOURCE* resource;
    ULONG i, RequirementsListSize;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR RequirementDescriptor;
    BOOLEAN CurrentRes = FALSE;

    PAGED_CODE ();

    if (!DeviceData->AcpiHandle)
    {
        return Irp->IoStatus.Status;
    }

    /* Handle the PCI root manually */
    if (wcsstr(DeviceData->HardwareIDs, L"PNP0A03") != 0 ||
        wcsstr(DeviceData->HardwareIDs, L"PNP0A08") != 0)
    {
        return Irp->IoStatus.Status;
    }

    /* Get current resources */
    while (TRUE)
    {
        Buffer.Length = 0;
        if (CurrentRes)
        AcpiStatus = AcpiGetCurrentResources(DeviceData->AcpiHandle, &Buffer);
        else
            AcpiStatus = AcpiGetPossibleResources(DeviceData->AcpiHandle, &Buffer);
        if ((!ACPI_SUCCESS(AcpiStatus) && AcpiStatus != AE_BUFFER_OVERFLOW) ||
            Buffer.Length == 0)
        {
            if (!CurrentRes)
                CurrentRes = TRUE;
            else
                return Irp->IoStatus.Status;
        }
        else
            break;
    }

    Buffer.Pointer = ExAllocatePoolWithTag(PagedPool, Buffer.Length, 'BpcA');
    if (!Buffer.Pointer)
      return STATUS_INSUFFICIENT_RESOURCES;

    if (CurrentRes)
        AcpiStatus = AcpiGetCurrentResources(DeviceData->AcpiHandle, &Buffer);
    else
        AcpiStatus = AcpiGetPossibleResources(DeviceData->AcpiHandle, &Buffer);
    if (!ACPI_SUCCESS(AcpiStatus))
    {
      DPRINT1("AcpiGetCurrentResources #2 failed (0x%x)\n", AcpiStatus);
      ASSERT(FALSE);
      return STATUS_UNSUCCESSFUL;
    }

    resource= Buffer.Pointer;
    /* Count number of resources */
    while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG)
    {
        switch (resource->Type)
        {
            case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            {
                ACPI_RESOURCE_EXTENDED_IRQ *irq_data = (ACPI_RESOURCE_EXTENDED_IRQ*) &resource->Data;
                if (irq_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                NumberOfResources += irq_data->InterruptCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_IRQ:
            {
                ACPI_RESOURCE_IRQ *irq_data = (ACPI_RESOURCE_IRQ*) &resource->Data;
                NumberOfResources += irq_data->InterruptCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_DMA:
            {
                ACPI_RESOURCE_DMA *dma_data = (ACPI_RESOURCE_DMA*) &resource->Data;
                NumberOfResources += dma_data->ChannelCount;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS16:
            case ACPI_RESOURCE_TYPE_ADDRESS32:
            case ACPI_RESOURCE_TYPE_ADDRESS64:
            case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
            {
                ACPI_RESOURCE_ADDRESS *res_addr = (ACPI_RESOURCE_ADDRESS*) &resource->Data;
                if (res_addr->ProducerConsumer == ACPI_PRODUCER)
                    break;
                NumberOfResources++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY24:
            case ACPI_RESOURCE_TYPE_MEMORY32:
            case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            case ACPI_RESOURCE_TYPE_FIXED_IO:
            case ACPI_RESOURCE_TYPE_IO:
            {
                NumberOfResources++;
                break;
            }
            default:
            {
                break;
            }
        }
        resource = ACPI_NEXT_RESOURCE(resource);
    }

    RequirementsListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR) * (NumberOfResources - 1);
    RequirementsList = ExAllocatePoolWithTag(PagedPool, RequirementsListSize, 'RpcA');

    if (!RequirementsList)
    {
        ExFreePoolWithTag(Buffer.Pointer, 'BpcA');
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RequirementsList->ListSize = RequirementsListSize;
    RequirementsList->InterfaceType = Internal;
    RequirementsList->BusNumber = 0;
    RequirementsList->SlotNumber = 0; /* Not used by WDM drivers */
    RequirementsList->AlternativeLists = 1;
    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = NumberOfResources;
    RequirementDescriptor = RequirementsList->List[0].Descriptors;

    /* Fill resources list structure */
        resource = Buffer.Pointer;
    while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG && resource->Type != ACPI_RESOURCE_TYPE_END_DEPENDENT)
    {
        switch (resource->Type)
        {
            case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
            {
                ACPI_RESOURCE_EXTENDED_IRQ *irq_data = (ACPI_RESOURCE_EXTENDED_IRQ*) &resource->Data;
                if (irq_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                for (i = 0; i < irq_data->InterruptCount; i++)
                {
                    RequirementDescriptor->Option = (i == 0) ? IO_RESOURCE_PREFERRED : IO_RESOURCE_ALTERNATIVE;
                    RequirementDescriptor->Type = CmResourceTypeInterrupt;
                    RequirementDescriptor->ShareDisposition = (irq_data->Sharable == ACPI_SHARED ? CmResourceShareShared : CmResourceShareDeviceExclusive);
                    RequirementDescriptor->Flags =(irq_data->Triggering == ACPI_LEVEL_SENSITIVE ? CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE : CM_RESOURCE_INTERRUPT_LATCHED);
                    RequirementDescriptor->u.Interrupt.MinimumVector =
                    RequirementDescriptor->u.Interrupt.MaximumVector = irq_data->Interrupts[i];

                    RequirementDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_IRQ:
            {
                ACPI_RESOURCE_IRQ *irq_data = (ACPI_RESOURCE_IRQ*) &resource->Data;
                for (i = 0; i < irq_data->InterruptCount; i++)
                {
                    RequirementDescriptor->Option = (i == 0) ? IO_RESOURCE_PREFERRED : IO_RESOURCE_ALTERNATIVE;
                    RequirementDescriptor->Type = CmResourceTypeInterrupt;
                    RequirementDescriptor->ShareDisposition = (irq_data->Sharable == ACPI_SHARED ? CmResourceShareShared : CmResourceShareDeviceExclusive);
                    RequirementDescriptor->Flags =(irq_data->Triggering == ACPI_LEVEL_SENSITIVE ? CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE : CM_RESOURCE_INTERRUPT_LATCHED);
                    RequirementDescriptor->u.Interrupt.MinimumVector =
                    RequirementDescriptor->u.Interrupt.MaximumVector = irq_data->Interrupts[i];

                    RequirementDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_DMA:
            {
                ACPI_RESOURCE_DMA *dma_data = (ACPI_RESOURCE_DMA*) &resource->Data;
                for (i = 0; i < dma_data->ChannelCount; i++)
                {
                    RequirementDescriptor->Type = CmResourceTypeDma;
                    RequirementDescriptor->Flags = 0;
                    switch (dma_data->Type)
                    {
                        case ACPI_TYPE_A: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
                        case ACPI_TYPE_B: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
                        case ACPI_TYPE_F: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
                    }
                    if (dma_data->BusMaster == ACPI_BUS_MASTER)
                        RequirementDescriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
                    switch (dma_data->Transfer)
                    {
                        case ACPI_TRANSFER_8: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_8; break;
                        case ACPI_TRANSFER_16: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_16; break;
                        case ACPI_TRANSFER_8_16: RequirementDescriptor->Flags |= CM_RESOURCE_DMA_8_AND_16; break;
                    }

                    RequirementDescriptor->Option = (i == 0) ? IO_RESOURCE_PREFERRED : IO_RESOURCE_ALTERNATIVE;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
                    RequirementDescriptor->u.Dma.MinimumChannel =
                    RequirementDescriptor->u.Dma.MaximumChannel = dma_data->Channels[i];
                    RequirementDescriptor++;
                }
                break;
            }
            case ACPI_RESOURCE_TYPE_IO:
            {
                ACPI_RESOURCE_IO *io_data = (ACPI_RESOURCE_IO*) &resource->Data;
                RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                if (io_data->IoDecode == ACPI_DECODE_16)
                    RequirementDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
                else
                    RequirementDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
                RequirementDescriptor->u.Port.Length = io_data->AddressLength;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                RequirementDescriptor->Type = CmResourceTypePort;
                RequirementDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
                RequirementDescriptor->u.Port.Alignment = io_data->Alignment;
                RequirementDescriptor->u.Port.MinimumAddress.QuadPart = io_data->Minimum;
                RequirementDescriptor->u.Port.MaximumAddress.QuadPart = io_data->Maximum + io_data->AddressLength - 1;

                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_FIXED_IO:
            {
                ACPI_RESOURCE_FIXED_IO *io_data = (ACPI_RESOURCE_FIXED_IO*) &resource->Data;
                RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                RequirementDescriptor->u.Port.Length = io_data->AddressLength;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                RequirementDescriptor->Type = CmResourceTypePort;
                RequirementDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
                RequirementDescriptor->u.Port.Alignment = 1;
                RequirementDescriptor->u.Port.MinimumAddress.QuadPart = io_data->Address;
                RequirementDescriptor->u.Port.MaximumAddress.QuadPart = io_data->Address + io_data->AddressLength - 1;

                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS16:
            {
                ACPI_RESOURCE_ADDRESS16 *addr16_data = (ACPI_RESOURCE_ADDRESS16*) &resource->Data;
                if (addr16_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                if (addr16_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypeBusNumber;
                    RequirementDescriptor->ShareDisposition = CmResourceShareShared;
                    RequirementDescriptor->Flags = 0;
                    RequirementDescriptor->u.BusNumber.MinBusNumber = addr16_data->Address.Minimum;
                    RequirementDescriptor->u.BusNumber.MaxBusNumber = addr16_data->Address.Maximum + addr16_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.BusNumber.Length = addr16_data->Address.AddressLength;
                }
                else if (addr16_data->ResourceType == ACPI_IO_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypePort;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr16_data->Decode == ACPI_POS_DECODE)
                        RequirementDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    RequirementDescriptor->u.Port.MinimumAddress.QuadPart = addr16_data->Address.Minimum;
                    RequirementDescriptor->u.Port.MaximumAddress.QuadPart = addr16_data->Address.Maximum + addr16_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Port.Length = addr16_data->Address.AddressLength;
                }
                else
                {
                    RequirementDescriptor->Type = CmResourceTypeMemory;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = 0;
                    if (addr16_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr16_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = addr16_data->Address.Minimum;
                    RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = addr16_data->Address.Maximum + addr16_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Memory.Length = addr16_data->Address.AddressLength;
                }
                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS32:
            {
                ACPI_RESOURCE_ADDRESS32 *addr32_data = (ACPI_RESOURCE_ADDRESS32*) &resource->Data;
                if (addr32_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                if (addr32_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypeBusNumber;
                    RequirementDescriptor->ShareDisposition = CmResourceShareShared;
                    RequirementDescriptor->Flags = 0;
                    RequirementDescriptor->u.BusNumber.MinBusNumber = addr32_data->Address.Minimum;
                    RequirementDescriptor->u.BusNumber.MaxBusNumber = addr32_data->Address.Maximum + addr32_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.BusNumber.Length = addr32_data->Address.AddressLength;
                }
                else if (addr32_data->ResourceType == ACPI_IO_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypePort;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr32_data->Decode == ACPI_POS_DECODE)
                        RequirementDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    RequirementDescriptor->u.Port.MinimumAddress.QuadPart = addr32_data->Address.Minimum;
                    RequirementDescriptor->u.Port.MaximumAddress.QuadPart = addr32_data->Address.Maximum + addr32_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Port.Length = addr32_data->Address.AddressLength;
                }
                else
                {
                    RequirementDescriptor->Type = CmResourceTypeMemory;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = 0;
                    if (addr32_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr32_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = addr32_data->Address.Minimum;
                    RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = addr32_data->Address.Maximum + addr32_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Memory.Length = addr32_data->Address.AddressLength;
                }
                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_ADDRESS64:
            {
                ACPI_RESOURCE_ADDRESS64 *addr64_data = (ACPI_RESOURCE_ADDRESS64*) &resource->Data;
                if (addr64_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                if (addr64_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    DPRINT1("64-bit bus address is not supported!\n");
                    RequirementDescriptor->Type = CmResourceTypeBusNumber;
                    RequirementDescriptor->ShareDisposition = CmResourceShareShared;
                    RequirementDescriptor->Flags = 0;
                    RequirementDescriptor->u.BusNumber.MinBusNumber = (ULONG)addr64_data->Address.Minimum;
                    RequirementDescriptor->u.BusNumber.MaxBusNumber = (ULONG)addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.BusNumber.Length = addr64_data->Address.AddressLength;
                }
                else if (addr64_data->ResourceType == ACPI_IO_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypePort;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr64_data->Decode == ACPI_POS_DECODE)
                        RequirementDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    RequirementDescriptor->u.Port.MinimumAddress.QuadPart = addr64_data->Address.Minimum;
                    RequirementDescriptor->u.Port.MaximumAddress.QuadPart = addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Port.Length = addr64_data->Address.AddressLength;
                }
                else
                {
                    RequirementDescriptor->Type = CmResourceTypeMemory;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = 0;
                    if (addr64_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr64_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = addr64_data->Address.Minimum;
                    RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Memory.Length = addr64_data->Address.AddressLength;
                }
                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
            {
                ACPI_RESOURCE_EXTENDED_ADDRESS64 *addr64_data = (ACPI_RESOURCE_EXTENDED_ADDRESS64*) &resource->Data;
                if (addr64_data->ProducerConsumer == ACPI_PRODUCER)
                    break;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                if (addr64_data->ResourceType == ACPI_BUS_NUMBER_RANGE)
                {
                    DPRINT1("64-bit bus address is not supported!\n");
                    RequirementDescriptor->Type = CmResourceTypeBusNumber;
                    RequirementDescriptor->ShareDisposition = CmResourceShareShared;
                    RequirementDescriptor->Flags = 0;
                    RequirementDescriptor->u.BusNumber.MinBusNumber = (ULONG)addr64_data->Address.Minimum;
                    RequirementDescriptor->u.BusNumber.MaxBusNumber = (ULONG)addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.BusNumber.Length = addr64_data->Address.AddressLength;
                }
                else if (addr64_data->ResourceType == ACPI_IO_RANGE)
                {
                    RequirementDescriptor->Type = CmResourceTypePort;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = CM_RESOURCE_PORT_IO;
                    if (addr64_data->Decode == ACPI_POS_DECODE)
                        RequirementDescriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;
                    RequirementDescriptor->u.Port.MinimumAddress.QuadPart = addr64_data->Address.Minimum;
                    RequirementDescriptor->u.Port.MaximumAddress.QuadPart = addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Port.Length = addr64_data->Address.AddressLength;
                }
                else
                {
                    RequirementDescriptor->Type = CmResourceTypeMemory;
                    RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    RequirementDescriptor->Flags = 0;
                    if (addr64_data->Info.Mem.WriteProtect == ACPI_READ_ONLY_MEMORY)
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                    else
                        RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                    switch (addr64_data->Info.Mem.Caching)
                    {
                        case ACPI_CACHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE; break;
                        case ACPI_WRITE_COMBINING_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE; break;
                        case ACPI_PREFETCHABLE_MEMORY: RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE; break;
                    }
                    RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = addr64_data->Address.Minimum;
                    RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = addr64_data->Address.Maximum + addr64_data->Address.AddressLength - 1;
                    RequirementDescriptor->u.Memory.Length = addr64_data->Address.AddressLength;
                }
                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY24:
            {
                ACPI_RESOURCE_MEMORY24 *mem24_data = (ACPI_RESOURCE_MEMORY24*) &resource->Data;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                RequirementDescriptor->Type = CmResourceTypeMemory;
                RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                RequirementDescriptor->Flags = CM_RESOURCE_MEMORY_24;
                if (mem24_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = mem24_data->Minimum;
                RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = mem24_data->Maximum + mem24_data->AddressLength - 1;
                RequirementDescriptor->u.Memory.Length = mem24_data->AddressLength;

                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_MEMORY32:
            {
                ACPI_RESOURCE_MEMORY32 *mem32_data = (ACPI_RESOURCE_MEMORY32*) &resource->Data;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                RequirementDescriptor->Type = CmResourceTypeMemory;
                RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                RequirementDescriptor->Flags = 0;
                if (mem32_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = mem32_data->Minimum;
                RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = mem32_data->Maximum + mem32_data->AddressLength - 1;
                RequirementDescriptor->u.Memory.Length = mem32_data->AddressLength;

                RequirementDescriptor++;
                break;
            }
            case ACPI_RESOURCE_TYPE_FIXED_MEMORY32:
            {
                ACPI_RESOURCE_FIXED_MEMORY32 *fixedmem32_data = (ACPI_RESOURCE_FIXED_MEMORY32*) &resource->Data;
                RequirementDescriptor->Option = CurrentRes ? 0 : IO_RESOURCE_PREFERRED;
                RequirementDescriptor->Type = CmResourceTypeMemory;
                RequirementDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                RequirementDescriptor->Flags = 0;
                if (fixedmem32_data->WriteProtect == ACPI_READ_ONLY_MEMORY)
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                else
                    RequirementDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
                RequirementDescriptor->u.Memory.MinimumAddress.QuadPart = fixedmem32_data->Address;
                RequirementDescriptor->u.Memory.MaximumAddress.QuadPart = fixedmem32_data->Address + fixedmem32_data->AddressLength - 1;
                RequirementDescriptor->u.Memory.Length = fixedmem32_data->AddressLength;

                RequirementDescriptor++;
                break;
            }
            default:
            {
                break;
            }
        }
        resource = ACPI_NEXT_RESOURCE(resource);
    }
    ExFreePoolWithTag(Buffer.Pointer, 'BpcA');

    Irp->IoStatus.Information = (ULONG_PTR)RequirementsList;

    return STATUS_SUCCESS;
}

NTSTATUS
Bus_PDO_QueryDeviceRelations(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
/*++

Routine Description:

    The PnP Manager sends this IRP to gather information about
    devices with a relationship to the specified device.
    Bus drivers must handle this request for TargetDeviceRelation
    for their child devices (child PDOs).

    If a driver returns relations in response to this IRP,
    it allocates a DEVICE_RELATIONS structure from paged
    memory containing a count and the appropriate number of
    device object pointers. The PnP Manager frees the structure
    when it is no longer needed. If a driver replaces a
    DEVICE_RELATIONS structure allocated by another driver,
    it must free the previous structure.

    A driver must reference the PDO of any device that it
    reports in this IRP (ObReferenceObject). The PnP Manager
    removes the reference when appropriate.

Arguments:

    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:

    NT STATUS

--*/
{

    PIO_STACK_LOCATION   stack;
    PDEVICE_RELATIONS deviceRelations;
    NTSTATUS status;

    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryDeviceRelations.Type) {

    case TargetDeviceRelation:

        deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        if (deviceRelations) {
            //
            // Only PDO can handle this request. Somebody above
            // is not playing by rule.
            //
            ASSERTMSG("Someone above is handling TargetDeviceRelation", !deviceRelations);
        }

        deviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(DEVICE_RELATIONS),
                                                'IpcA');
        if (!deviceRelations) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
        }

        //
        // There is only one PDO pointer in the structure
        // for this relation type. The PnP Manager removes
        // the reference to the PDO when the driver or application
        // un-registers for notification on the device.
        //

        deviceRelations->Count = 1;
        deviceRelations->Objects[0] = DeviceData->Common.Self;
        ObReferenceObject(DeviceData->Common.Self);

        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
        break;

    case BusRelations: // Not handled by PDO
    case EjectionRelations: // optional for PDO
    case RemovalRelations: // // optional for PDO
    default:
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
Bus_PDO_QueryBusInformation(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp )
/*++

Routine Description:

    The PnP Manager uses this IRP to request the type and
    instance number of a device's parent bus. Bus drivers
    should handle this request for their child devices (PDOs).

Arguments:

    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:

    NT STATUS

--*/
{

    PPNP_BUS_INFORMATION busInfo;

    PAGED_CODE ();

    busInfo = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(PNP_BUS_INFORMATION),
                                    'IpcA');

    if (busInfo == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    busInfo->BusTypeGuid = GUID_ACPI_INTERFACE_STANDARD;

    busInfo->LegacyBusType = InternalPowerBus;

    busInfo->BusNumber = 0; //fixme

    Irp->IoStatus.Information = (ULONG_PTR)busInfo;

    return STATUS_SUCCESS;
}


NTSTATUS
Bus_GetDeviceCapabilities(
      PDEVICE_OBJECT          DeviceObject,
      PDEVICE_CAPABILITIES    DeviceCapabilities
    )
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;

    PAGED_CODE();

    //
    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    //
    // Initialize the event
    //
    KeInitializeEvent( &pnpEvent, NotificationEvent, FALSE );

    targetObject = IoGetAttachedDeviceReference( DeviceObject );

    //
    // Build an Irp
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        targetObject,
        NULL,
        0,
        NULL,
        &pnpEvent,
        &ioStatus
        );
    if (pnpIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetDeviceCapabilitiesExit;

    }

    //
    // Pnp Irps all begin life as STATUS_NOT_SUPPORTED;
    //
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // Get the top of stack
    //
    irpStack = IoGetNextIrpStackLocation( pnpIrp );

    //
    // Set the top of stack
    //
    RtlZeroMemory( irpStack, sizeof(IO_STACK_LOCATION ) );
    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    //
    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {

        //
        // Block until the irp comes back.
        // Important thing to note here is when you allocate
        // the memory for an event in the stack you must do a
        // KernelMode wait instead of UserMode to prevent
        // the stack from getting paged out.
        //

        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioStatus.Status;

    }

GetDeviceCapabilitiesExit:
    //
    // Done with reference
    //
    ObDereferenceObject( targetObject );

    //
    // Done
    //
    return status;

}


