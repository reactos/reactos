/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WMI support
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "inport.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InPortWmi)
#pragma alloc_text(PAGE, InPortWmiRegistration)
#pragma alloc_text(PAGE, InPortWmiDeRegistration)
#pragma alloc_text(PAGE, InPortQueryWmiRegInfo)
#pragma alloc_text(PAGE, InPortQueryWmiDataBlock)
#endif

GUID GuidWmiPortData = POINTER_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO InPortWmiGuidList[] =
{
    {&GuidWmiPortData, 1, 0}
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
InPortQueryWmiRegInfo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PULONG RegFlags,
    _Inout_ PUNICODE_STRING InstanceName,
    _Out_opt_ PUNICODE_STRING *RegistryPath,
    _Inout_ PUNICODE_STRING MofResourceName,
    _Out_opt_ PDEVICE_OBJECT *Pdo)
{
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(InstanceName);
    UNREFERENCED_PARAMETER(MofResourceName);

    PAGED_CODE();

    DPRINT("%s()\n", __FUNCTION__);

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &DriverRegistryPath;
    *Pdo = DeviceExtension->Pdo;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
InPortQueryWmiDataBlock(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG GuidIndex,
    _In_ ULONG InstanceIndex,
    _In_ ULONG InstanceCount,
    _Out_opt_ PULONG InstanceLengthArray,
    _In_ ULONG BufferAvail,
    _Out_opt_ PUCHAR Buffer)
{
    NTSTATUS Status;
    PPOINTER_PORT_WMI_STD_DATA InPortData;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    DPRINT("%s()\n", __FUNCTION__);

    if (GuidIndex > RTL_NUMBER_OF(InPortWmiGuidList))
    {
        Status = STATUS_WMI_GUID_NOT_FOUND;
        goto Complete;
    }

    /* Only register 1 instance per GUID */
    if (InstanceIndex != 0 || InstanceCount != 1)
    {
        Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        goto Complete;
    }

    if (!InstanceLengthArray || BufferAvail < sizeof(POINTER_PORT_WMI_STD_DATA))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Complete;
    }

    InPortData = (PPOINTER_PORT_WMI_STD_DATA)Buffer;

    /* Bus mouse connector isn't defined in the DDK, so set type to something generic */
    InPortData->ConnectorType = POINTER_PORT_WMI_STD_I8042;
    /* 1 packet */
    InPortData->DataQueueSize = 1;
    /* Not supported by device */
    InPortData->ErrorCount = 0;

    InPortData->Buttons = DeviceExtension->MouseAttributes.NumberOfButtons;
    InPortData->HardwareType = POINTER_PORT_WMI_STD_MOUSE;
    *InstanceLengthArray = sizeof(POINTER_PORT_WMI_STD_DATA);

    Status = STATUS_SUCCESS;

Complete:
    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              Status,
                              sizeof(POINTER_PORT_WMI_STD_DATA),
                              IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
InPortWmiRegistration(
    _Inout_ PINPORT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    DeviceExtension->WmiLibInfo.GuidCount = RTL_NUMBER_OF(InPortWmiGuidList);
    DeviceExtension->WmiLibInfo.GuidList = InPortWmiGuidList;

    DeviceExtension->WmiLibInfo.QueryWmiRegInfo = InPortQueryWmiRegInfo;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock = InPortQueryWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataBlock = NULL;
    DeviceExtension->WmiLibInfo.SetWmiDataItem = NULL;
    DeviceExtension->WmiLibInfo.ExecuteWmiMethod = NULL;
    DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    return IoWMIRegistrationControl(DeviceExtension->Self,
                                    WMIREG_ACTION_REGISTER);
}

NTSTATUS
NTAPI
InPortWmiDeRegistration(
    _Inout_ PINPORT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    return IoWMIRegistrationControl(DeviceExtension->Self,
                                    WMIREG_ACTION_DEREGISTER);
}

NTSTATUS
NTAPI
InPortWmi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    DPRINT("%s(%p, %p) %X\n", __FUNCTION__, DeviceObject, Irp,
           IoGetCurrentIrpStackLocation(Irp)->MinorFunction);

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    Status = WmiSystemControl(&DeviceExtension->WmiLibInfo,
                              DeviceObject,
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
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->Ldo, Irp);
            break;
    }

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    return Status;
}
