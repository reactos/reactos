/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdo.c
 * PURPOSE:        Functional Device Object routines
 * PROGRAMMERS:    Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "fdc.h"

#include <stdio.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}




static
NTSTATUS
FdcFdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PCM_RESOURCE_LIST ResourceListTranslated)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
//    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptorTranslated;
    ULONG i;

    DPRINT("FdcFdoStartDevice called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    ASSERT(DeviceExtension);

    if (ResourceList == NULL ||
        ResourceListTranslated == NULL)
    {
        DPRINT1("No allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ResourceList->Count != 1)
    {
        DPRINT1("Wrong number of allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ResourceList->List[0].PartialResourceList.Version != 1 ||
        ResourceList->List[0].PartialResourceList.Revision != 1 ||
        ResourceListTranslated->List[0].PartialResourceList.Version != 1 ||
        ResourceListTranslated->List[0].PartialResourceList.Revision != 1)
    {
        DPRINT1("Revision mismatch: %u.%u != 1.1 or %u.%u != 1.1\n",
                ResourceList->List[0].PartialResourceList.Version,
                ResourceList->List[0].PartialResourceList.Revision,
                ResourceListTranslated->List[0].PartialResourceList.Version,
                ResourceListTranslated->List[0].PartialResourceList.Revision);
        return STATUS_REVISION_MISMATCH;
    }

    for (i = 0; i < ResourceList->List[0].PartialResourceList.Count; i++)
    {
        PartialDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
//        PartialDescriptorTranslated = &ResourceListTranslated->List[0].PartialResourceList.PartialDescriptors[i];

        switch (PartialDescriptor->Type)
        {
            case CmResourceTypePort:
                DPRINT("Port: 0x%lx (%lu)\n",
                        PartialDescriptor->u.Port.Start.u.LowPart,
                        PartialDescriptor->u.Port.Length);
                if (PartialDescriptor->u.Port.Length >= 6)
                    DeviceExtension->ControllerInfo.BaseAddress = (PUCHAR)(ULONG_PTR)PartialDescriptor->u.Port.Start.QuadPart;
                break;

            case CmResourceTypeInterrupt:
                DPRINT("Interrupt: Level %lu  Vector %lu\n",
                        PartialDescriptor->u.Interrupt.Level,
                        PartialDescriptor->u.Interrupt.Vector);
/*
                Dirql = (KIRQL)PartialDescriptorTranslated->u.Interrupt.Level;
                Vector = PartialDescriptorTranslated->u.Interrupt.Vector;
                Affinity = PartialDescriptorTranslated->u.Interrupt.Affinity;
                if (PartialDescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
                    InterruptMode = Latched;
                else
                    InterruptMode = LevelSensitive;
                ShareInterrupt = (PartialDescriptorTranslated->ShareDisposition == CmResourceShareShared);
*/
                break;

            case CmResourceTypeDma:
                DPRINT("Dma: Channel %lu\n",
                        PartialDescriptor->u.Dma.Channel);
                break;
        }
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
FdcFdoConfigCallback(
    PVOID Context,
    PUNICODE_STRING PathName,
    INTERFACE_TYPE BusType,
    ULONG BusNumber,
    PKEY_VALUE_FULL_INFORMATION *BusInformation,
    CONFIGURATION_TYPE ControllerType,
    ULONG ControllerNumber,
    PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    CONFIGURATION_TYPE PeripheralType,
    ULONG PeripheralNumber,
    PKEY_VALUE_FULL_INFORMATION *PeripheralInformation)
{
    PKEY_VALUE_FULL_INFORMATION ControllerFullDescriptor;
    PCM_FULL_RESOURCE_DESCRIPTOR ControllerResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION PeripheralFullDescriptor;
    PCM_FULL_RESOURCE_DESCRIPTOR PeripheralResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_FLOPPY_DEVICE_DATA FloppyDeviceData;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDRIVE_INFO DriveInfo;
    BOOLEAN ControllerFound = FALSE;
    ULONG i;

    DPRINT("FdcFdoConfigCallback() called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)Context;

    /* Get the controller resources */
    ControllerFullDescriptor = ControllerInformation[IoQueryDeviceConfigurationData];
    ControllerResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PCHAR)ControllerFullDescriptor +
                                                                  ControllerFullDescriptor->DataOffset);

    for(i = 0; i < ControllerResourceDescriptor->PartialResourceList.Count; i++)
    {
        PartialDescriptor = &ControllerResourceDescriptor->PartialResourceList.PartialDescriptors[i];

        if (PartialDescriptor->Type == CmResourceTypePort)
        {
            if ((PUCHAR)(ULONG_PTR)PartialDescriptor->u.Port.Start.QuadPart == DeviceExtension->ControllerInfo.BaseAddress)
                ControllerFound = TRUE;
        }
    }

    /* Leave, if the enumerated controller is not the one represented by the FDO */
    if (ControllerFound == FALSE)
        return STATUS_SUCCESS;

    /* Get the peripheral resources */
    PeripheralFullDescriptor = PeripheralInformation[IoQueryDeviceConfigurationData];
    PeripheralResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PCHAR)PeripheralFullDescriptor +
                                                                  PeripheralFullDescriptor->DataOffset);

    /* learn about drives attached to controller */
    for(i = 0; i < PeripheralResourceDescriptor->PartialResourceList.Count; i++)
    {
        PartialDescriptor = &PeripheralResourceDescriptor->PartialResourceList.PartialDescriptors[i];

        if (PartialDescriptor->Type != CmResourceTypeDeviceSpecific)
            continue;

        FloppyDeviceData = (PCM_FLOPPY_DEVICE_DATA)(PartialDescriptor + 1);

        DriveInfo = &DeviceExtension->ControllerInfo.DriveInfo[DeviceExtension->ControllerInfo.NumberOfDrives];

        DriveInfo->ControllerInfo = &DeviceExtension->ControllerInfo;
        DriveInfo->UnitNumber = DeviceExtension->ControllerInfo.NumberOfDrives;
        DriveInfo->PeripheralNumber = PeripheralNumber;

        DriveInfo->FloppyDeviceData.MaxDensity = FloppyDeviceData->MaxDensity;
        DriveInfo->FloppyDeviceData.MountDensity = FloppyDeviceData->MountDensity;
        DriveInfo->FloppyDeviceData.StepRateHeadUnloadTime = FloppyDeviceData->StepRateHeadUnloadTime;
        DriveInfo->FloppyDeviceData.HeadLoadTime = FloppyDeviceData->HeadLoadTime;
        DriveInfo->FloppyDeviceData.MotorOffTime = FloppyDeviceData->MotorOffTime;
        DriveInfo->FloppyDeviceData.SectorLengthCode = FloppyDeviceData->SectorLengthCode;
        DriveInfo->FloppyDeviceData.SectorPerTrack = FloppyDeviceData->SectorPerTrack;
        DriveInfo->FloppyDeviceData.ReadWriteGapLength = FloppyDeviceData->ReadWriteGapLength;
        DriveInfo->FloppyDeviceData.FormatGapLength = FloppyDeviceData->FormatGapLength;
        DriveInfo->FloppyDeviceData.FormatFillCharacter = FloppyDeviceData->FormatFillCharacter;
        DriveInfo->FloppyDeviceData.HeadSettleTime = FloppyDeviceData->HeadSettleTime;
        DriveInfo->FloppyDeviceData.MotorSettleTime = FloppyDeviceData->MotorSettleTime;
        DriveInfo->FloppyDeviceData.MaximumTrackValue = FloppyDeviceData->MaximumTrackValue;
        DriveInfo->FloppyDeviceData.DataTransferLength = FloppyDeviceData->DataTransferLength;

        /* Once it's all set up, acknowledge its existence in the controller info object */
        DeviceExtension->ControllerInfo.NumberOfDrives++;
    }

    DeviceExtension->ControllerInfo.Populated = TRUE;

    DPRINT("Detected %lu floppy drives!\n",
            DeviceExtension->ControllerInfo.NumberOfDrives);

    return STATUS_SUCCESS;
}


static
NTSTATUS
PciCreateHardwareIDsString(PUNICODE_STRING HardwareIDs)
{
    WCHAR Buffer[256];
    UNICODE_STRING BufferU;
    ULONG Index;

    Index = 0;
    Index += swprintf(&Buffer[Index],
                      L"FDC\\GENERIC_FLOPPY_DRIVE");
    Index++;

    Buffer[Index] = UNICODE_NULL;

    BufferU.Length = BufferU.MaximumLength = (USHORT) Index * sizeof(WCHAR);
    BufferU.Buffer = Buffer;

    return DuplicateUnicodeString(0, &BufferU, HardwareIDs);
}


static
NTSTATUS
PciCreateCompatibleIDsString(PUNICODE_STRING CompatibleIDs)
{
    WCHAR Buffer[256];
    UNICODE_STRING BufferU;
    ULONG Index;

    Index = 0;
    Index += swprintf(&Buffer[Index],
                      L"GenFloppyDisk");
    Index++;

    Buffer[Index] = UNICODE_NULL;

    BufferU.Length = BufferU.MaximumLength = (USHORT)Index * sizeof(WCHAR);
    BufferU.Buffer = Buffer;

    return DuplicateUnicodeString(0, &BufferU, CompatibleIDs);
}


static
NTSTATUS
PciCreateInstanceIDString(PUNICODE_STRING InstanceID,
                          ULONG PeripheralNumber)
{
    WCHAR Buffer[3];

    swprintf(Buffer, L"%02X", PeripheralNumber & 0xff);

    return RtlCreateUnicodeString(InstanceID, Buffer) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}


static
NTSTATUS
FdcFdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS *DeviceRelations)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    INTERFACE_TYPE InterfaceType = Isa;
    CONFIGURATION_TYPE ControllerType = DiskController;
    CONFIGURATION_TYPE PeripheralType = FloppyDiskPeripheral;
    PDEVICE_RELATIONS Relations;
    PDRIVE_INFO DriveInfo;
    PDEVICE_OBJECT Pdo;
    WCHAR DeviceNameBuffer[80];
    UNICODE_STRING DeviceName;
    ULONG DeviceNumber = 0;
    ULONG Size;
    ULONG i;
    NTSTATUS Status;

    DPRINT("FdcFdoQueryBusRelations() called\n");

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Status = IoQueryDeviceDescription(&InterfaceType,
                                      NULL,
                                      &ControllerType,
                                      NULL,
                                      &PeripheralType,
                                      NULL,
                                      FdcFdoConfigCallback,
                                      FdoDeviceExtension);
    if (!NT_SUCCESS(Status) && (Status != STATUS_NO_MORE_ENTRIES))
        return Status;

    Size = sizeof(DEVICE_RELATIONS) +
           sizeof(Relations->Objects) * (FdoDeviceExtension->ControllerInfo.NumberOfDrives - 1);
    Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
    if (Relations == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Relations->Count = FdoDeviceExtension->ControllerInfo.NumberOfDrives;

    for (i = 0; i < FdoDeviceExtension->ControllerInfo.NumberOfDrives; i++)
    {
        DriveInfo = &FdoDeviceExtension->ControllerInfo.DriveInfo[i];

        if (DriveInfo->DeviceObject == NULL)
        {
            do
            {
                swprintf(DeviceNameBuffer, L"\\Device\\FloppyPDO%lu", DeviceNumber++);
                RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
                DPRINT("Device name: %S\n", DeviceNameBuffer);

                /* Create physical device object */
                Status = IoCreateDevice(FdoDeviceExtension->Common.DeviceObject->DriverObject,
                                        sizeof(PDO_DEVICE_EXTENSION),
                                        &DeviceName,
                                        FILE_DEVICE_MASS_STORAGE,
                                        FILE_DEVICE_SECURE_OPEN,
                                        FALSE,
                                        &Pdo);
            }
            while (Status == STATUS_OBJECT_NAME_COLLISION);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PDO creation failed (Status 0x%08lx)\n", Status);
                goto done;
            }

            DPRINT("PDO created: %S\n", DeviceNameBuffer);

            DriveInfo->DeviceObject = Pdo;

            PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Pdo->DeviceExtension;
            RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

            PdoDeviceExtension->Common.IsFDO = FALSE;
            PdoDeviceExtension->Common.DeviceObject = Pdo;

            PdoDeviceExtension->Fdo = FdoDeviceExtension->Common.DeviceObject;
            PdoDeviceExtension->DriveInfo = DriveInfo;

            Pdo->Flags |= DO_DIRECT_IO;
            Pdo->Flags |= DO_POWER_PAGABLE;
            Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

            /* Add Device ID string */
            RtlCreateUnicodeString(&PdoDeviceExtension->DeviceId,
                                   L"FDC\\GENERIC_FLOPPY_DRIVE");
            DPRINT("DeviceID: %S\n", PdoDeviceExtension->DeviceId.Buffer);

            /* Add Hardware IDs string */
            Status = PciCreateHardwareIDsString(&PdoDeviceExtension->HardwareIds);
            if (!NT_SUCCESS(Status))
            {
//                ErrorStatus = Status;
//                ErrorOccurred = TRUE;
                break;
            }

            /* Add Compatible IDs string */
            Status = PciCreateCompatibleIDsString(&PdoDeviceExtension->CompatibleIds);
            if (!NT_SUCCESS(Status))
            {
//                ErrorStatus = Status;
//                ErrorOccurred = TRUE;
                break;
            }

            /* Add Instance ID string */
            Status = PciCreateInstanceIDString(&PdoDeviceExtension->InstanceId,
                                               DriveInfo->PeripheralNumber);
            if (!NT_SUCCESS(Status))
            {
//                ErrorStatus = Status;
//                ErrorOccurred = TRUE;
                break;
            }

#if 0
             /* Add device description string */
            Status = PciCreateDeviceDescriptionString(&PdoDeviceExtension->DeviceDescription, Device);
            if (!NT_SUCCESS(Status))
            {
//                ErrorStatus = Status;
//                ErrorOccurred = TRUE;
                break;
            }

            /* Add device location string */
            Status = PciCreateDeviceLocationString(&PdoDeviceExtension->DeviceLocation, Device);
            if (!NT_SUCCESS(Status))
            {
//                ErrorStatus = Status;
//                ErrorOccurred = TRUE;
                break;
            }
#endif
        }

        ObReferenceObject(DriveInfo->DeviceObject);
        Relations->Objects[i] = DriveInfo->DeviceObject;
    }

done:
    if (NT_SUCCESS(Status))
    {
        *DeviceRelations = Relations;
    }
    else
    {
        if (Relations != NULL)
            ExFreePool(Relations);
    }

    return Status;
}


NTSTATUS
NTAPI
FdcFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    ULONG_PTR Information = 0;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    DPRINT("FdcFdoPnp()\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DPRINT("  IRP_MN_START_DEVICE received\n");
            
            /* Call lower driver */
            Status = STATUS_UNSUCCESSFUL;
            FdoExtension = DeviceObject->DeviceExtension;

            if (IoForwardIrpSynchronously(FdoExtension->LowerDevice, Irp))
            {
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                {
                    Status = FdcFdoStartDevice(DeviceObject,
                        IrpSp->Parameters.StartDevice.AllocatedResources,
                        IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated);
                }
            }

            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            DPRINT("  IRP_MN_QUERY_REMOVE_DEVICE\n");
            break;

        case IRP_MN_REMOVE_DEVICE:
            DPRINT("  IRP_MN_REMOVE_DEVICE received\n");
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DPRINT("  IRP_MN_CANCEL_REMOVE_DEVICE\n");
            break;

        case IRP_MN_STOP_DEVICE:
            DPRINT("  IRP_MN_STOP_DEVICE received\n");
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            DPRINT("  IRP_MN_QUERY_STOP_DEVICE received\n");
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            DPRINT("  IRP_MN_CANCEL_STOP_DEVICE\n");
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT("  IRP_MN_QUERY_DEVICE_RELATIONS\n");

            switch (IrpSp->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    Status = FdcFdoQueryBusRelations(DeviceObject, &DeviceRelations);
                    Information = (ULONG_PTR)DeviceRelations;
                    break;

                case RemovalRelations:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                default:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            IrpSp->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DPRINT("  IRP_MN_SURPRISE_REMOVAL received\n");
            break;

        default:
            DPRINT("  Unknown IOCTL 0x%lx\n", IrpSp->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */
