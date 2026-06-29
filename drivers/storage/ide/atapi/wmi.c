/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     WMI support
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
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
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PULONG RegFlags,
    _Inout_ PUNICODE_STRING InstanceName,
    _Outptr_result_maybenull_  PUNICODE_STRING *RegistryPath,
    _Inout_  PUNICODE_STRING MofResourceName,
    _Outptr_result_maybenull_  PDEVICE_OBJECT *Pdo)
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
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG GuidIndex,
    _In_ ULONG InstanceIndex,
    _In_ ULONG InstanceCount,
    _Out_writes_opt_(InstanceCount) PULONG InstanceLengthArray,
    _In_ ULONG BufferAvail,
    _Out_writes_bytes_opt_(BufferAvail) PUCHAR Buffer)
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
    AtaScsiAddress = DevExt->Device.AtaScsiAddress;

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
    return IoCallDriver(ChanExt->Common.LowerDeviceObject, Irp);
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

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN Register)
{
    ULONG Action;

    PAGED_CODE();

    if (Register)
    {
        DevExt->WmiLibInfo.GuidCount = RTL_NUMBER_OF(AtapWmiGuidList);
        DevExt->WmiLibInfo.GuidList = AtapWmiGuidList;

        DevExt->WmiLibInfo.QueryWmiRegInfo = AtaQueryWmiRegInfo;
        DevExt->WmiLibInfo.QueryWmiDataBlock = AtaQueryWmiDataBlock;

        Action = WMIREG_ACTION_REGISTER;
    }
    else
    {
        Action = WMIREG_ACTION_DEREGISTER;
    }

    return IoWMIRegistrationControl(DevExt->Common.Self, Action);
}
