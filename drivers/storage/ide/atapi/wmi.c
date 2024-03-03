/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WMI support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
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
    PATAPORT_DEVICE_EXTENSION DevExt = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(InstanceName);
    UNREFERENCED_PARAMETER(MofResourceName);

    PAGED_CODE();

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &AtapDriverRegistryPath;
    *Pdo = DevExt->Common.Self;

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
    PATAPORT_DEVICE_EXTENSION DevExt;
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

    DevExt = DeviceObject->DeviceExtension;
    AtaScsiAddress = DevExt->AtaScsiAddress;

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
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN Register)
{
    PAGED_CODE();

    if (!Register)
    {
        return IoWMIRegistrationControl(DevExt->Common.Self,
                                        WMIREG_ACTION_DEREGISTER);
    }

    DevExt->WmiLibInfo.GuidCount = RTL_NUMBER_OF(AtapWmiGuidList);
    DevExt->WmiLibInfo.GuidList = AtapWmiGuidList;

    DevExt->WmiLibInfo.QueryWmiRegInfo = AtaQueryWmiRegInfo;
    DevExt->WmiLibInfo.QueryWmiDataBlock = AtaQueryWmiDataBlock;

    return IoWMIRegistrationControl(DevExt->Common.Self,
                                    WMIREG_ACTION_REGISTER);
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;

    PAGED_CODE();

    TRACE("%s(%p, %p)\n", __FUNCTION__, DevExt, Irp);

    Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    Status = WmiSystemControl(&DevExt->WmiLibInfo,
                              DevExt->Common.Self,
                              Irp,
                              &Disposition);
    switch (Disposition)
    {
        case IrpProcessed:
            break;

        case IrpNotCompleted:
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        default:
            ASSERT(FALSE);
            __fallthrough;
        case IrpForward:
        case IrpNotWmi:
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
    }

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoWmi(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(ChanExt->Ldo, Irp);
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
        return AtaFdoWmi(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoWmi(DeviceObject->DeviceExtension, Irp);
}
