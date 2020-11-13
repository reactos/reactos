/*
 * PROJECT:     Partition manager driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Partition device code
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "partmgr.h"

static const WCHAR PartitionSymLinkFormat[] = L"\\Device\\Harddisk%u\\Partition%u";


CODE_SEG("PAGE")
NTSTATUS
PartitionCreateDevice(
    _In_ PDEVICE_OBJECT FDObject,
    _In_ PPARTITION_INFORMATION_EX PartitionEntry,
    _In_ UINT32 PdoNumber,
    _In_ PARTITION_STYLE PartitionStyle,
    _Out_ PDEVICE_OBJECT *PDO)
{
    PAGED_CODE();

    static UINT32 HarddiskVolumeNextId = 1; // this is 1-based

    WCHAR nameBuf[64];
    UNICODE_STRING deviceName;

    // create the device object

    swprintf(nameBuf, L"\\Device\\HarddiskVolume%u", HarddiskVolumeNextId++);
    RtlCreateUnicodeString(&deviceName, nameBuf);

    PDEVICE_OBJECT partitionDevice;
    NTSTATUS status = IoCreateDevice(FDObject->DriverObject,
                                     sizeof(PARTITION_EXTENSION),
                                     &deviceName,
                                     FILE_DEVICE_DISK,
                                     FILE_DEVICE_SECURE_OPEN,
                                     FALSE,
                                     &partitionDevice);

    if (!NT_SUCCESS(status))
    {
        ERR("Unable to create device object %wZ\n", &deviceName);
        return status;
    }

    INFO("Created device object %p %wZ\n", partitionDevice, &deviceName);

    PPARTITION_EXTENSION partExt = partitionDevice->DeviceExtension;
    RtlZeroMemory(partExt, sizeof(*partExt));

    partitionDevice->StackSize = FDObject->StackSize;
    partitionDevice->Flags |= DO_DIRECT_IO;

    if (PartitionStyle == PARTITION_STYLE_MBR)
    {
        partExt->Mbr.PartitionType = PartitionEntry->Mbr.PartitionType;
        partExt->Mbr.BootIndicator = PartitionEntry->Mbr.BootIndicator;
        partExt->Mbr.HiddenSectors = PartitionEntry->Mbr.HiddenSectors;
    }
    else
    {
        partExt->Gpt.PartitionType = PartitionEntry->Gpt.PartitionType;
        partExt->Gpt.PartitionId = PartitionEntry->Gpt.PartitionType;
        partExt->Gpt.Attributes = PartitionEntry->Gpt.Attributes;

        RtlCopyMemory(partExt->Gpt.Name, PartitionEntry->Gpt.Name, sizeof(partExt->Gpt.Name));
    }

    partExt->DeviceName = deviceName;
    partExt->StartingOffset = PartitionEntry->StartingOffset.QuadPart;
    partExt->PartitionLength = PartitionEntry->PartitionLength.QuadPart;
    partExt->OnDiskNumber = PartitionEntry->PartitionNumber; // the "physical" partition number
    partExt->DetectedNumber = PdoNumber; // counts only partitions with PDO created

    partExt->DeviceObject = partitionDevice;
    partExt->LowerDevice = FDObject;

    partitionDevice->Flags &= ~DO_DEVICE_INITIALIZING;

    *PDO = partitionDevice;

    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PartitionHandleStartDevice(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    // fix the damn kernel!
    if (PartExt->DeviceRemoved)
    {
        DPRINT1("IRP_MN_START_DEVICE after IRP_MN_REMOVE_DEVICE!\n");
        return STATUS_SUCCESS;
    }

    // first, create a symbolic link for our device
    WCHAR nameBuf[64];
    UNICODE_STRING partitionSymlink, interfaceName;
    PFDO_EXTENSION fdoExtension = PartExt->LowerDevice->DeviceExtension;

    // \\Device\\Harddisk%u\\Partition%u
    swprintf(nameBuf, PartitionSymLinkFormat,
        fdoExtension->DiskData.DeviceNumber, PartExt->DetectedNumber);

    if (!RtlCreateUnicodeString(&partitionSymlink, nameBuf))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = IoCreateSymbolicLink(&partitionSymlink, &PartExt->DeviceName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PartExt->SymlinkCreated = TRUE;

    TRACE("Symlink created %wZ -> %wZ\n", &PartExt->DeviceName, &partitionSymlink);

    // our partition device will have two interfaces:
    // GUID_DEVINTERFACE_PARTITION and GUID_DEVINTERFACE_VOLUME
    // the former one is used to notify mountmgr about new device

    status = IoRegisterDeviceInterface(PartExt->DeviceObject,
                                       &GUID_DEVINTERFACE_PARTITION,
                                       NULL,
                                       &interfaceName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PartExt->PartitionInterfaceName = interfaceName;
    status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

    INFO("Partition interface %wZ\n", &interfaceName);

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&interfaceName);
        RtlInitUnicodeString(&PartExt->PartitionInterfaceName, NULL);
        return status;
    }

    status = IoRegisterDeviceInterface(PartExt->DeviceObject,
                                       &GUID_DEVINTERFACE_VOLUME,
                                       NULL,
                                       &interfaceName);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PartExt->VolumeInterfaceName = interfaceName;
    status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

    INFO("Volume interface %wZ\n", &interfaceName);

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&interfaceName);
        RtlInitUnicodeString(&PartExt->VolumeInterfaceName, NULL);
        return status;
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PartitionHandleRemove(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ BOOLEAN FinalRemove)
{
    NTSTATUS status;

    PAGED_CODE();

    // remove the symbolic link
    if (PartExt->SymlinkCreated)
    {
        WCHAR nameBuf[64];
        UNICODE_STRING partitionSymlink;
        PFDO_EXTENSION fdoExtension = PartExt->LowerDevice->DeviceExtension;

        swprintf(nameBuf, PartitionSymLinkFormat,
            fdoExtension->DiskData.DeviceNumber, PartExt->DetectedNumber);

        RtlInitUnicodeString(&partitionSymlink, nameBuf);

        status = IoDeleteSymbolicLink(&partitionSymlink);

        if (!NT_SUCCESS(status))
        {
            return status;
        }
        PartExt->SymlinkCreated = FALSE;

        INFO("Symlink removed %wZ -> %wZ\n", &PartExt->DeviceName, &partitionSymlink);
    }

    // release device interfaces
    if (PartExt->PartitionInterfaceName.Buffer)
    {
        status = IoSetDeviceInterfaceState(&PartExt->PartitionInterfaceName, FALSE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        RtlFreeUnicodeString(&PartExt->PartitionInterfaceName);
        RtlInitUnicodeString(&PartExt->PartitionInterfaceName, NULL);
    }

    if (PartExt->VolumeInterfaceName.Buffer)
    {
        status = IoSetDeviceInterfaceState(&PartExt->VolumeInterfaceName, FALSE);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        RtlFreeUnicodeString(&PartExt->VolumeInterfaceName);
        RtlInitUnicodeString(&PartExt->VolumeInterfaceName, NULL);
    }

    if (FinalRemove)
    {
        // fix the damn kernel!
        if (PartExt->DeviceRemoved)
        {
            DPRINT1("Double IRP_MN_REMOVE_DEVICE!\n");
            return STATUS_SUCCESS;
        }

        PartExt->DeviceRemoved = TRUE;

        ASSERT(PartExt->DeviceName.Buffer);
        if (PartExt->DeviceName.Buffer)
        {
            INFO("Removed device %wZ\n", &PartExt->DeviceName);
            RtlFreeUnicodeString(&PartExt->DeviceName);
        }

        IoDeleteDevice(PartExt->DeviceObject);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PartitionHandleDeviceRelations(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    // fix the damn kernel!
    if (PartExt->DeviceRemoved)
    {
        DPRINT1("QDR after device removal!\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    DEVICE_RELATION_TYPE type = ioStack->Parameters.QueryDeviceRelations.Type;

    if (type == TargetDeviceRelation)
    {
        // Device relations has one entry built in to it's size.
        PDEVICE_RELATIONS deviceRelations =
            ExAllocatePoolZero(PagedPool, sizeof(DEVICE_RELATIONS), TAG_PARTMGR);

        if (deviceRelations != NULL)
        {
            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = PartExt->DeviceObject;
            ObReferenceObject(deviceRelations->Objects[0]);

            Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Irp->IoStatus.Information = 0;
        return Irp->IoStatus.Status;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
PartitionHandleQueryId(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    BUS_QUERY_ID_TYPE idType = ioStack->Parameters.QueryId.IdType;
    UNICODE_STRING idString;
    NTSTATUS status;

    PAGED_CODE();

    switch (idType)
    {
        case BusQueryDeviceID:
            status = RtlCreateUnicodeString(&idString, L"STORAGE\\Partition")
                     ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
            break;
        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
        {
            static WCHAR volumeID[] = L"STORAGE\\Volume\0";

            idString.Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(volumeID), TAG_PARTMGR);
            RtlCopyMemory(idString.Buffer, volumeID, sizeof(volumeID));

            status = STATUS_SUCCESS;
            break;
        }
        case BusQueryInstanceID:
        {
            WCHAR string[64];
            PFDO_EXTENSION fdoExtension = PartExt->LowerDevice->DeviceExtension;

            PartMgrAcquireLayoutLock(fdoExtension);

            if (fdoExtension->DiskData.PartitionStyle == PARTITION_STYLE_MBR)
            {
                swprintf(string, L"S%08lx_O%I64x_L%I64x",
                         fdoExtension->DiskData.Mbr.Signature,
                         PartExt->StartingOffset,
                         PartExt->PartitionLength);
            }
            else
            {
                swprintf(string,
                        L"S%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02xS_O%I64x_L%I64x",
                        fdoExtension->DiskData.Gpt.DiskId.Data1,
                        fdoExtension->DiskData.Gpt.DiskId.Data2,
                        fdoExtension->DiskData.Gpt.DiskId.Data3,
                        fdoExtension->DiskData.Gpt.DiskId.Data4[0],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[1],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[2],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[3],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[4],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[5],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[6],
                        fdoExtension->DiskData.Gpt.DiskId.Data4[7],
                        PartExt->StartingOffset,
                        PartExt->PartitionLength);
            }

            PartMgrReleaseLayoutLock(fdoExtension);

            status = RtlCreateUnicodeString(&idString, string)
                     ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Information = NT_SUCCESS(status) ? (ULONG_PTR) idString.Buffer : 0;
    return status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PartitionHandleQueryCapabilities(
    _In_ PPARTITION_EXTENSION PartExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES devCaps = ioStack->Parameters.DeviceCapabilities.Capabilities;

    PAGED_CODE();
    ASSERT(devCaps);

    devCaps->SilentInstall = TRUE;
    devCaps->RawDeviceOK = TRUE;
    devCaps->NoDisplayInUI = TRUE;
    devCaps->Address = PartExt->OnDiskNumber;
    devCaps->UniqueID = 1;

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PartitionHandlePnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PPARTITION_EXTENSION partExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    PAGED_CODE();

    switch (ioStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            status = PartitionHandleStartDevice(partExt, Irp);
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            status = PartitionHandleDeviceRelations(partExt, Irp);
            break;
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        {
            status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_SURPRISE_REMOVAL:
        {
            status = PartitionHandleRemove(partExt, FALSE);
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            status = PartitionHandleRemove(partExt, TRUE);
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            status = PartitionHandleQueryId(partExt, Irp);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            status = PartitionHandleQueryCapabilities(partExt, Irp);
            break;
        }
        default:
        {
            Irp->IoStatus.Information = 0;
            status = STATUS_NOT_SUPPORTED;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
PartitionHandleDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PPARTITION_EXTENSION partExt = DeviceObject->DeviceExtension;
    PFDO_EXTENSION fdoExtension = partExt->LowerDevice->DeviceExtension;
    NTSTATUS status;

    ASSERT(!partExt->IsFDO);

    if (!partExt->IsEnumerated)
    {
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch (ioStack->Parameters.DeviceIoControl.IoControlCode)
    {
        // disk stuff
        case IOCTL_DISK_GET_PARTITION_INFO:
        {
            if (!VerifyIrpOutBufferSize(Irp, sizeof(PARTITION_INFORMATION)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PartMgrAcquireLayoutLock(fdoExtension);

            // not supported on anything other than MBR
            if (fdoExtension->DiskData.PartitionStyle != PARTITION_STYLE_MBR)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                PartMgrReleaseLayoutLock(fdoExtension);
                break;
            }

            PPARTITION_INFORMATION partInfo = Irp->AssociatedIrp.SystemBuffer;

            *partInfo = (PARTITION_INFORMATION){
                .PartitionType = partExt->Mbr.PartitionType,
                .StartingOffset.QuadPart = partExt->StartingOffset,
                .PartitionLength.QuadPart = partExt->PartitionLength,
                .HiddenSectors = partExt->Mbr.HiddenSectors,
                .PartitionNumber = partExt->DetectedNumber,
                .BootIndicator = partExt->Mbr.BootIndicator,
                .RecognizedPartition = partExt->Mbr.RecognizedPartition,
                .RewritePartition = FALSE,
            };

            PartMgrReleaseLayoutLock(fdoExtension);

            Irp->IoStatus.Information = sizeof(*partInfo);
            status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
        {
            if (!VerifyIrpOutBufferSize(Irp, sizeof(PARTITION_INFORMATION_EX)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PPARTITION_INFORMATION_EX partInfoEx = Irp->AssociatedIrp.SystemBuffer;

            PartMgrAcquireLayoutLock(fdoExtension);

            *partInfoEx = (PARTITION_INFORMATION_EX){
                .StartingOffset.QuadPart = partExt->StartingOffset,
                .PartitionLength.QuadPart = partExt->PartitionLength,
                .PartitionNumber = partExt->DetectedNumber,
                .PartitionStyle = fdoExtension->DiskData.PartitionStyle,
                .RewritePartition = FALSE,
            };

            if (fdoExtension->DiskData.PartitionStyle == PARTITION_STYLE_MBR)
            {
                partInfoEx->Mbr = (PARTITION_INFORMATION_MBR){
                    .PartitionType = partExt->Mbr.PartitionType,
                    .HiddenSectors = partExt->Mbr.HiddenSectors,
                    .BootIndicator = partExt->Mbr.BootIndicator,
                    .RecognizedPartition = partExt->Mbr.RecognizedPartition,
                };
            }
            else
            {
                partInfoEx->Gpt = (PARTITION_INFORMATION_GPT){
                    .PartitionType = partExt->Gpt.PartitionType,
                    .PartitionId = partExt->Gpt.PartitionId,
                    .Attributes = partExt->Gpt.Attributes,
                };

                RtlCopyMemory(partInfoEx->Gpt.Name,
                              partExt->Gpt.Name,
                              sizeof(partInfoEx->Gpt.Name));
            }

            PartMgrReleaseLayoutLock(fdoExtension);

            Irp->IoStatus.Information = sizeof(*partInfoEx);
            status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_SET_PARTITION_INFO:
        {
            PSET_PARTITION_INFORMATION inputBuffer = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpInBufferSize(Irp, sizeof(*inputBuffer)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            PartMgrAcquireLayoutLock(fdoExtension);

            // these functions use on disk numbers, not detected ones
            status = IoSetPartitionInformation(fdoExtension->LowerDevice,
                                               fdoExtension->DiskData.BytesPerSector,
                                               partExt->OnDiskNumber,
                                               inputBuffer->PartitionType);

            if (NT_SUCCESS(status))
            {
                partExt->Mbr.PartitionType = inputBuffer->PartitionType;
            }

            PartMgrReleaseLayoutLock(fdoExtension);

            Irp->IoStatus.Information = 0;
            break;
        }
        case IOCTL_DISK_SET_PARTITION_INFO_EX:
        {
            PSET_PARTITION_INFORMATION_EX inputBuffer = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpInBufferSize(Irp, sizeof(*inputBuffer)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            PartMgrAcquireLayoutLock(fdoExtension);

            // these functions use on disk numbers, not detected ones
            status = IoSetPartitionInformationEx(fdoExtension->LowerDevice,
                                                 partExt->OnDiskNumber,
                                                 inputBuffer);

            if (NT_SUCCESS(status))
            {
                if (fdoExtension->DiskData.PartitionStyle == PARTITION_STYLE_MBR)
                {
                    partExt->Mbr.PartitionType = inputBuffer->Mbr.PartitionType;
                }
                else
                {
                    partExt->Gpt.PartitionType = inputBuffer->Gpt.PartitionType;
                    partExt->Gpt.PartitionId = inputBuffer->Gpt.PartitionId;
                    partExt->Gpt.Attributes = inputBuffer->Gpt.Attributes;

                    RtlMoveMemory(partExt->Gpt.Name,
                                  inputBuffer->Gpt.Name,
                                  sizeof(partExt->Gpt.Name));
                }
            }

            PartMgrReleaseLayoutLock(fdoExtension);

            Irp->IoStatus.Information = 0;
            break;
        }
        case IOCTL_DISK_GET_LENGTH_INFO:
        {
            PGET_LENGTH_INFORMATION lengthInfo = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpOutBufferSize(Irp, sizeof(*lengthInfo)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PartMgrAcquireLayoutLock(fdoExtension);

            lengthInfo->Length.QuadPart = partExt->PartitionLength;

            PartMgrReleaseLayoutLock(fdoExtension);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(*lengthInfo);
            break;
        }
        case IOCTL_DISK_VERIFY:
        {
            PVERIFY_INFORMATION verifyInfo = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpInBufferSize(Irp, sizeof(*verifyInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            // Partition device should just adjust the starting offset
            verifyInfo->StartingOffset.QuadPart += partExt->StartingOffset;
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
        case IOCTL_DISK_UPDATE_PROPERTIES:
        {
            fdoExtension->LayoutValid = FALSE;
            IoInvalidateDeviceRelations(fdoExtension->PhysicalDiskDO, BusRelations);

            status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_STORAGE_MEDIA_REMOVAL:
        {
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
        // volume stuff (most of that should be in volmgr.sys one it is implemented)
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        {
            PVOLUME_DISK_EXTENTS volExts = Irp->AssociatedIrp.SystemBuffer;

            // we fill only one extent entry so sizeof(*volExts) is enough
            if (!VerifyIrpOutBufferSize(Irp, sizeof(*volExts)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PartMgrAcquireLayoutLock(fdoExtension);

            // the only type of volume we support right now is disk partition
            // so this structure is simple

            *volExts = (VOLUME_DISK_EXTENTS) {
                .NumberOfDiskExtents = 1,
                .Extents = {{
                    .DiskNumber = fdoExtension->DiskData.DeviceNumber,
                    .StartingOffset.QuadPart = partExt->StartingOffset,
                    .ExtentLength.QuadPart = partExt->PartitionLength
                }}
            };

            PartMgrReleaseLayoutLock(fdoExtension);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(*volExts);
            break;
        }
        case IOCTL_VOLUME_ONLINE:
        {
            status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
        {
            PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION gptAttrs = Irp->AssociatedIrp.SystemBuffer;
            if (!VerifyIrpOutBufferSize(Irp, sizeof(*gptAttrs)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // not supported on anything other than GPT
            if (fdoExtension->DiskData.PartitionStyle != PARTITION_STYLE_GPT)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            gptAttrs->GptAttributes = partExt->Gpt.Attributes;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(*gptAttrs);
            break;
        }
        // mountmgr stuff
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            PMOUNTDEV_NAME name = Irp->AssociatedIrp.SystemBuffer;

            if (!VerifyIrpOutBufferSize(Irp, sizeof(USHORT)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            name->NameLength = partExt->DeviceName.Length;

            // return NameLength back
            if (!VerifyIrpOutBufferSize(Irp, sizeof(USHORT) + name->NameLength))
            {
                Irp->IoStatus.Information = sizeof(USHORT);
                status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            RtlCopyMemory(name->Name, partExt->DeviceName.Buffer, name->NameLength);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(USHORT) + name->NameLength;
            break;
        }
        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        {
            PMOUNTDEV_UNIQUE_ID uniqueId = Irp->AssociatedIrp.SystemBuffer;

            if (!partExt->VolumeInterfaceName.Buffer)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (!VerifyIrpOutBufferSize(Irp, sizeof(USHORT)))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            uniqueId->UniqueIdLength = partExt->VolumeInterfaceName.Length;

            // return UniqueIdLength back
            if (!VerifyIrpOutBufferSize(Irp, sizeof(USHORT) + uniqueId->UniqueIdLength))
            {
                Irp->IoStatus.Information = sizeof(USHORT);
                status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            RtlCopyMemory(uniqueId->UniqueId,
                          partExt->VolumeInterfaceName.Buffer,
                          uniqueId->UniqueIdLength);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(USHORT) + uniqueId->UniqueIdLength;
            break;
        }
        default:
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
