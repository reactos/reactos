/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WMI support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static WMIGUIDREGINFO AtapWmiGuidList[] =
{
    {&MSIde_PortDeviceInfo_GUID, 1, 0}
};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaQueryWmiRegInfo(
    PDEVICE_OBJECT DeviceObject,
    PULONG RegFlags,
    PUNICODE_STRING InstanceName,
    PUNICODE_STRING *RegistryPath,
    PUNICODE_STRING MofResourceName,
    PDEVICE_OBJECT *Pdo)
{
    PATAPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(InstanceName);
    UNREFERENCED_PARAMETER(MofResourceName);

    PAGED_CODE();

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &AtapDriverRegistryPath;
    *Pdo = DeviceExtension->Common.Self;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaQueryWmiDataBlock(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG GuidIndex,
    ULONG InstanceIndex,
    ULONG InstanceCount,
    PULONG InstanceLengthArray,
    ULONG BufferAvail,
    PUCHAR Buffer)
{
    PATAPORT_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    PMSIde_PortDeviceInfo DeviceInfo;

    PAGED_CODE();

    if (GuidIndex > RTL_NUMBER_OF(AtapWmiGuidList))
    {
        Status = STATUS_WMI_GUID_NOT_FOUND;
        goto Complete;
    }

    /* Only ever register 1 instance per GUID */
    if (InstanceIndex != 0 || InstanceCount != 1)
    {
        Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        goto Complete;
    }

    if (!InstanceLengthArray || BufferAvail < sizeof(*DeviceInfo))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    AtaScsiAddress = DeviceExtension->AtaScsiAddress;

    DeviceInfo = (PMSIde_PortDeviceInfo)Buffer;
    DeviceInfo->Bus = AtaScsiAddress.PathId;
    DeviceInfo->Target = AtaScsiAddress.TargetId;
    DeviceInfo->Lun = AtaScsiAddress.Lun;

    *InstanceLengthArray = sizeof(*DeviceInfo);

    Status = STATUS_SUCCESS;

Complete:
    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              Status,
                              sizeof(*DeviceInfo),
                              IO_NO_INCREMENT);
}

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN Register)
{
    PAGED_CODE();

    if (!Register)
    {
        return IoWMIRegistrationControl(DeviceExtension->Common.Self,
                                        WMIREG_ACTION_DEREGISTER);
    }

    DeviceExtension->WmiLibInfo.GuidCount = RTL_NUMBER_OF(AtapWmiGuidList);
    DeviceExtension->WmiLibInfo.GuidList = AtapWmiGuidList;

    DeviceExtension->WmiLibInfo.QueryWmiRegInfo = AtaQueryWmiRegInfo;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock = AtaQueryWmiDataBlock;

    return IoWMIRegistrationControl(DeviceExtension->Common.Self,
                                    WMIREG_ACTION_REGISTER);
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmi(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;

    PAGED_CODE();

    TRACE("%s(%p, %p)\n", __FUNCTION__, DeviceExtension, Irp);

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    Status = WmiSystemControl(&DeviceExtension->WmiLibInfo,
                              DeviceExtension->Common.Self,
                              Irp,
                              &Disposition);
    switch (Disposition)
    {
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IrpForward:
        case IrpNotWmi:
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
            break;
    }

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaDispatchWmi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    if (IS_FDO(DeviceObject->DeviceExtension))
    {
        PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(ChannelExtension->Ldo, Irp);
    }
    else
    {
        return AtaPdoWmi(DeviceObject->DeviceExtension, Irp);
    }
}
