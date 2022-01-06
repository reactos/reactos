/*
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            fdo.c
 * PURPOSE:         PCI device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include "pci.h"

#define NDEBUG
#include <debug.h>

/*** PRIVATE *****************************************************************/

static NTSTATUS
FdoLocateChildDevice(
    PPCI_DEVICE *Device,
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PCI_SLOT_NUMBER SlotNumber,
    PPCI_COMMON_CONFIG PciConfig)
{
    PLIST_ENTRY CurrentEntry;
    PPCI_DEVICE CurrentDevice;

    DPRINT("Called\n");

    CurrentEntry = DeviceExtension->DeviceListHead.Flink;
    while (CurrentEntry != &DeviceExtension->DeviceListHead)
    {
        CurrentDevice = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);

        /* If both vendor ID and device ID match, it is the same device */
        if ((PciConfig->VendorID == CurrentDevice->PciConfig.VendorID) &&
            (PciConfig->DeviceID == CurrentDevice->PciConfig.DeviceID) &&
            (SlotNumber.u.AsULONG == CurrentDevice->SlotNumber.u.AsULONG))
        {
            *Device = CurrentDevice;
            DPRINT("Done\n");
            return STATUS_SUCCESS;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    *Device = NULL;
    DPRINT("Done\n");
    return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
FdoEnumerateDevices(
    PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PCI_COMMON_CONFIG PciConfig;
    PPCI_DEVICE Device;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    ULONG Size;
    NTSTATUS Status;

    DPRINT("Called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DeviceExtension->DeviceListCount = 0;

    /* Enumerate devices on the PCI bus */
    SlotNumber.u.AsULONG = 0;
    for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
        SlotNumber.u.bits.DeviceNumber = DeviceNumber;
        for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            DPRINT("Bus %1lu  Device %2lu  Func %1lu\n",
                   DeviceExtension->BusNumber,
                   DeviceNumber,
                   FunctionNumber);

            RtlZeroMemory(&PciConfig,
                          sizeof(PCI_COMMON_CONFIG));

            Size = HalGetBusData(PCIConfiguration,
                                 DeviceExtension->BusNumber,
                                 SlotNumber.u.AsULONG,
                                 &PciConfig,
                                 PCI_COMMON_HDR_LENGTH);
            DPRINT("Size %lu\n", Size);
            if (Size < PCI_COMMON_HDR_LENGTH)
            {
                if (FunctionNumber == 0)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

            DPRINT("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
                   DeviceExtension->BusNumber,
                   DeviceNumber,
                   FunctionNumber,
                   PciConfig.VendorID,
                   PciConfig.DeviceID);

            if (PciConfig.VendorID == 0 && PciConfig.DeviceID == 0)
            {
                DPRINT("Filter out devices with null vendor and device ID\n");
                continue;
            }

            Status = FdoLocateChildDevice(&Device, DeviceExtension, SlotNumber, &PciConfig);
            if (!NT_SUCCESS(Status))
            {
                Device = ExAllocatePoolWithTag(NonPagedPool, sizeof(PCI_DEVICE), TAG_PCI);
                if (!Device)
                {
                    /* FIXME: Cleanup resources for already discovered devices */
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlZeroMemory(Device,
                              sizeof(PCI_DEVICE));

                Device->BusNumber = DeviceExtension->BusNumber;

                RtlCopyMemory(&Device->SlotNumber,
                              &SlotNumber,
                              sizeof(PCI_SLOT_NUMBER));

                RtlCopyMemory(&Device->PciConfig,
                              &PciConfig,
                              sizeof(PCI_COMMON_CONFIG));

                ExInterlockedInsertTailList(
                    &DeviceExtension->DeviceListHead,
                    &Device->ListEntry,
                    &DeviceExtension->DeviceListLock);
            }

            DeviceExtension->DeviceListCount++;

            /* Skip to next device if the current one is not a multifunction device */
            if ((FunctionNumber == 0) &&
                ((PciConfig.HeaderType & 0x80) == 0))
            {
                break;
            }
        }
    }

    DPRINT("Done\n");

    return STATUS_SUCCESS;
}


static NTSTATUS
FdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension = NULL;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS Relations;
    PLIST_ENTRY CurrentEntry;
    PPCI_DEVICE Device;
    NTSTATUS Status;
    BOOLEAN ErrorOccurred;
    NTSTATUS ErrorStatus;
    ULONG Size;
    ULONG i;

    UNREFERENCED_PARAMETER(IrpSp);

    DPRINT("Called\n");

    ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;

    Status = STATUS_SUCCESS;

    ErrorOccurred = FALSE;

    FdoEnumerateDevices(DeviceObject);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (Irp->IoStatus.Information)
    {
        /* FIXME: Another bus driver has already created a DEVICE_RELATIONS
                  structure so we must merge this structure with our own */
        DPRINT1("FIXME: leaking old bus relations\n");
    }

    Size = sizeof(DEVICE_RELATIONS) +
           sizeof(Relations->Objects) * (DeviceExtension->DeviceListCount - 1);
    Relations = ExAllocatePoolWithTag(PagedPool, Size, TAG_PCI);
    if (!Relations)
        return STATUS_INSUFFICIENT_RESOURCES;

    Relations->Count = DeviceExtension->DeviceListCount;

    i = 0;
    CurrentEntry = DeviceExtension->DeviceListHead.Flink;
    while (CurrentEntry != &DeviceExtension->DeviceListHead)
    {
        Device = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);

        PdoDeviceExtension = NULL;

        if (!Device->Pdo)
        {
            /* Create a physical device object for the
               device as it does not already have one */
            Status = IoCreateDevice(DeviceObject->DriverObject,
                                    sizeof(PDO_DEVICE_EXTENSION),
                                    NULL,
                                    FILE_DEVICE_CONTROLLER,
                                    FILE_AUTOGENERATED_DEVICE_NAME,
                                    FALSE,
                                    &Device->Pdo);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

            Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

            //Device->Pdo->Flags |= DO_POWER_PAGABLE;

            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

            RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

            PdoDeviceExtension->Common.IsFDO = FALSE;

            PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

            PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

            PdoDeviceExtension->Fdo = DeviceObject;

            PdoDeviceExtension->PciDevice = Device;

            /* Add Device ID string */
            Status = PciCreateDeviceIDString(&PdoDeviceExtension->DeviceID, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

            DPRINT("DeviceID: %S\n", PdoDeviceExtension->DeviceID.Buffer);

            /* Add Instance ID string */
            Status = PciCreateInstanceIDString(&PdoDeviceExtension->InstanceID, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

            /* Add Hardware IDs string */
            Status = PciCreateHardwareIDsString(&PdoDeviceExtension->HardwareIDs, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

            /* Add Compatible IDs string */
            Status = PciCreateCompatibleIDsString(&PdoDeviceExtension->CompatibleIDs, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

             /* Add device description string */
            Status = PciCreateDeviceDescriptionString(&PdoDeviceExtension->DeviceDescription, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }

            /* Add device location string */
            Status = PciCreateDeviceLocationString(&PdoDeviceExtension->DeviceLocation, Device);
            if (!NT_SUCCESS(Status))
            {
                ErrorStatus = Status;
                ErrorOccurred = TRUE;
                break;
            }
        }

        /* Reference the physical device object. The PnP manager
           will dereference it again when it is no longer needed */
        ObReferenceObject(Device->Pdo);

        Relations->Objects[i] = Device->Pdo;

        i++;

        CurrentEntry = CurrentEntry->Flink;
    }

    if (ErrorOccurred)
    {
        /* FIXME: Cleanup all new PDOs created in this call. Please give me SEH!!! ;-) */
        /* FIXME: Should IoAttachDeviceToDeviceStack() be undone? */
        if (PdoDeviceExtension)
        {
            RtlFreeUnicodeString(&PdoDeviceExtension->DeviceID);
            RtlFreeUnicodeString(&PdoDeviceExtension->InstanceID);
            RtlFreeUnicodeString(&PdoDeviceExtension->HardwareIDs);
            RtlFreeUnicodeString(&PdoDeviceExtension->CompatibleIDs);
            RtlFreeUnicodeString(&PdoDeviceExtension->DeviceDescription);
            RtlFreeUnicodeString(&PdoDeviceExtension->DeviceLocation);
        }

        ExFreePoolWithTag(Relations, TAG_PCI);
        return ErrorStatus;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Relations;

    DPRINT("Done\n");

    return Status;
}


static NTSTATUS
FdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST AllocatedResources;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    ULONG FoundBusNumber = FALSE;
    ULONG i;

    DPRINT("Called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    AllocatedResources = IoGetCurrentIrpStackLocation(Irp)->Parameters.StartDevice.AllocatedResources;
    if (!AllocatedResources)
    {
        DPRINT("No allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (AllocatedResources->Count < 1)
    {
        DPRINT("Not enough allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (AllocatedResources->List[0].PartialResourceList.Version != 1 ||
        AllocatedResources->List[0].PartialResourceList.Revision != 1)
        return STATUS_REVISION_MISMATCH;

    ASSERT(DeviceExtension->State == dsStopped);

    /* By default, use the bus number in the resource list header */
    DeviceExtension->BusNumber = AllocatedResources->List[0].BusNumber;

    for (i = 0; i < AllocatedResources->List[0].PartialResourceList.Count; i++)
    {
        ResourceDescriptor = &AllocatedResources->List[0].PartialResourceList.PartialDescriptors[i];
        switch (ResourceDescriptor->Type)
        {
            case CmResourceTypeBusNumber:
                if (FoundBusNumber || ResourceDescriptor->u.BusNumber.Length < 1)
                    return STATUS_INVALID_PARAMETER;

                /* Use this one instead */
                ASSERT(AllocatedResources->List[0].BusNumber == ResourceDescriptor->u.BusNumber.Start);
                DeviceExtension->BusNumber = ResourceDescriptor->u.BusNumber.Start;
                DPRINT("Found bus number resource: %lu\n", DeviceExtension->BusNumber);
                FoundBusNumber = TRUE;
                break;

            default:
                DPRINT("Unknown resource descriptor type 0x%x\n", ResourceDescriptor->Type);
        }
    }

    InitializeListHead(&DeviceExtension->DeviceListHead);
    KeInitializeSpinLock(&DeviceExtension->DeviceListLock);
    DeviceExtension->DeviceListCount = 0;
    DeviceExtension->State = dsStarted;

    ExInterlockedInsertTailList(
        &DriverExtension->BusListHead,
        &DeviceExtension->ListEntry,
        &DriverExtension->BusListLock);

  Irp->IoStatus.Information = 0;

  return STATUS_SUCCESS;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
FdoPnpControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the PCI device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the PCI driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = Irp->IoStatus.Status;

    DPRINT("Called\n");

    DeviceExtension = DeviceObject->DeviceExtension;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    switch (IrpSp->MinorFunction)
    {
#if 0
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
#endif
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = FdoQueryBusRelations(DeviceObject, Irp, IrpSp);
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
#if 0
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
#endif
        case IRP_MN_START_DEVICE:
            DPRINT("IRP_MN_START_DEVICE received\n");
            Status = STATUS_UNSUCCESSFUL;

            if (IoForwardIrpSynchronously(DeviceExtension->Ldo, Irp))
            {
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                {
                    Status = FdoStartDevice(DeviceObject, Irp);
                }
            }

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_QUERY_STOP_DEVICE:
            /* We don't support stopping yet */
            Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_STOP_DEVICE:
            /* We can't fail this one so we fail the QUERY_STOP request that precedes it */
            break;
#if 0
        case IRP_MN_SURPRISE_REMOVAL:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
#endif

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            break;

        case IRP_MN_REMOVE_DEVICE:
            /* Detach the device object from the device stack */
            IoDetachDevice(DeviceExtension->Ldo);

            /* Delete the device object */
            IoDeleteDevice(DeviceObject);

            /* Return success */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            /* Don't print the warning, too much noise */
            break;

        default:
            DPRINT1("Unknown PNP minor function 0x%x\n", IrpSp->MinorFunction);
            break;
    }

    Irp->IoStatus.Status = Status;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->Ldo, Irp);

    DPRINT("Leaving. Status 0x%lx\n", Status);

    return Status;
}


NTSTATUS
FdoPowerControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the PCI device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the PCI driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DPRINT("Called\n");

    DeviceExtension = DeviceObject->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    Status = PoCallDriver(DeviceExtension->Ldo, Irp);

    DPRINT("Leaving. Status 0x%X\n", Status);

    return Status;
}

/* EOF */
