/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            pdo.c
 * PURPOSE:         PDO-specific code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <isapnp.h>
#include <isapnphw.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IsaPdoQueryDeviceRelations(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS DeviceRelations;

    if (IrpSp->Parameters.QueryDeviceRelations.Type == RemovalRelations &&
        PdoExt->Common.Self == PdoExt->FdoExt->DataPortPdo)
    {
        return IsaPnpFillDeviceRelations(PdoExt->FdoExt, Irp, FALSE);
    }

    if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
        return Irp->IoStatus.Status;

    DeviceRelations = ExAllocatePool(PagedPool, sizeof(*DeviceRelations));
    if (!DeviceRelations)
        return STATUS_NO_MEMORY;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = PdoExt->Common.Self;
    ObReferenceObject(PdoExt->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryCapabilities(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    ULONG i;

    DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;
    if (DeviceCapabilities->Version != 1)
        return STATUS_REVISION_MISMATCH;

    if (LogDev)
    {
        DeviceCapabilities->UniqueID = LogDev->SerialNumber != 0xffffffff;
        DeviceCapabilities->Address = LogDev->CSN;
    }
    else
    {
        DeviceCapabilities->UniqueID = TRUE;
        DeviceCapabilities->SilentInstall = TRUE;
        DeviceCapabilities->RawDeviceOK = TRUE;
        for (i = 0; i < POWER_SYSTEM_MAXIMUM; i++)
            DeviceCapabilities->DeviceState[i] = PowerDeviceD3;
        DeviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryPnpDeviceState(
  IN PISAPNP_PDO_EXTENSION PdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
    Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryId(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");
    PUNICODE_STRING Source;
    PWCHAR Buffer;

    switch (IrpSp->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            Source = &PdoExt->DeviceID;
            break;

        case BusQueryHardwareIDs:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
            Source = &PdoExt->HardwareIDs;
            break;

        case BusQueryCompatibleIDs:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            Source = &EmptyString;
            break;

        case BusQueryInstanceID:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            Source = &PdoExt->InstanceID;
            break;

        default:
          DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n",
                  IrpSp->Parameters.QueryId.IdType);
          return Irp->IoStatus.Status;
    }

    Buffer = ExAllocatePool(PagedPool, Source->MaximumLength);
    if (!Buffer)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Buffer, Source->Buffer, Source->MaximumLength);
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryResources(
  IN PISAPNP_PDO_EXTENSION PdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
    USHORT Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS };
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    ListSize = sizeof(CM_RESOURCE_LIST)
             + (ARRAYSIZE(Ports) - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ResourceList = ExAllocatePool(NonPagedPool, ListSize);
    if (!ResourceList)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(ResourceList, ListSize);
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Internal;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = 2;

    for (i = 0; i < ARRAYSIZE(Ports); i++)
    {
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Length = 0x01;
        Descriptor->u.Port.Start.LowPart = Ports[i];
    }

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryResourceRequirements(
  IN PISAPNP_PDO_EXTENSION PdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
    USHORT Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS, 0x274, 0x3e4, 0x204, 0x2e4, 0x354, 0x2f4 };
    ULONG ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
             + 2 * ARRAYSIZE(Ports) * sizeof(IO_RESOURCE_DESCRIPTOR);
    RequirementsList = ExAllocatePool(NonPagedPool, ListSize);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(RequirementsList, ListSize);
    RequirementsList->ListSize = ListSize;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = 2 * ARRAYSIZE(Ports);

    for (i = 0; i < 2 * ARRAYSIZE(Ports); i += 2)
    {
        Descriptor = &RequirementsList->List[0].Descriptors[i];

        /* Expected port */
        Descriptor[0].Type = CmResourceTypePort;
        Descriptor[0].ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor[0].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor[0].u.Port.Length = Ports[i / 2] & 1 ? 0x01 : 0x04;
        Descriptor[0].u.Port.Alignment = 0x01;
        Descriptor[0].u.Port.MinimumAddress.LowPart = Ports[i / 2];
        Descriptor[0].u.Port.MaximumAddress.LowPart = Ports[i / 2] + Descriptor[0].u.Port.Length - 1;

        /* ... but mark it as optional */
        Descriptor[1].Option = IO_RESOURCE_ALTERNATIVE;
        Descriptor[1].Type = CmResourceTypePort;
        Descriptor[1].ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor[1].Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor[1].u.Port.Alignment = 0x01;
    }

    Irp->IoStatus.Information = (ULONG_PTR)RequirementsList;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaPdoStartReadPort(
  IN PISAPNP_FDO_EXTENSION FdoExt,
  IN PIO_STACK_LOCATION IrpSp)
{
    PCM_RESOURCE_LIST ResourceList = IrpSp->Parameters.StartDevice.AllocatedResources;
    NTSTATUS Status;
    KIRQL OldIrql;
    ULONG i;

    if (!ResourceList || ResourceList->Count != 1)
    {
        DPRINT1("No resource list (%p) or bad count (%d)\n", ResourceList, ResourceList ? ResourceList->Count : 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (ResourceList->List[0].PartialResourceList.Version != 1
     || ResourceList->List[0].PartialResourceList.Revision != 1)
    {
        DPRINT1("Bad resource list version (%d.%d)\n", ResourceList->List[0].PartialResourceList.Version, ResourceList->List[0].PartialResourceList.Revision);
        return STATUS_REVISION_MISMATCH;
    }
    for (i = 0; i < ResourceList->List[0].PartialResourceList.Count; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        if (PartialDescriptor->Type == CmResourceTypePort)
        {
            PUCHAR ReadDataPort = (PUCHAR)PartialDescriptor->u.Port.Start.u.LowPart + 3;
            if (PartialDescriptor->u.Port.Length > 1 && !FdoExt->ReadDataPort && NT_SUCCESS(IsaHwTryReadDataPort(ReadDataPort)))
            {
                FdoExt->ReadDataPort = ReadDataPort;
                KeAcquireSpinLock(&FdoExt->Lock, &OldIrql);
                Status = IsaHwFillDeviceList(FdoExt);
                KeReleaseSpinLock(&FdoExt->Lock, OldIrql);
                if (FdoExt->DeviceCount > 0)
                {
                    IoInvalidateDeviceRelations(FdoExt->Pdo, BusRelations);
                    IoInvalidateDeviceRelations(FdoExt->DataPortPdo, RemovalRelations);
                }
            }
        }
    }
    return Status;
}

static
NTSTATUS
NTAPI
IsaPdoOnRepeaterComplete(
    IN PDEVICE_OBJECT Tdo,
    IN PIRP SubIrp,
    PVOID NeedsVote)
{
    PIO_STACK_LOCATION SubStack = IoGetCurrentIrpStackLocation(SubIrp);
    PIRP Irp = (PIRP)SubStack->Parameters.Others.Argument1;
    ObDereferenceObject(Tdo);

    if (SubIrp->IoStatus.Status == STATUS_NOT_SUPPORTED)
    {
        if (NeedsVote)
        {
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        Irp->IoStatus = SubIrp->IoStatus;
    }

    IoFreeIrp(SubIrp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
IsaPdoRepeatRequest(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN BOOLEAN NeedsVote)
{
    PDEVICE_OBJECT Fdo = PdoExt->FdoExt->Common.Self;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

    PDEVICE_OBJECT Tdo = IoGetAttachedDeviceReference(Fdo);
    PIRP SubIrp = IoAllocateIrp(Tdo->StackSize + 1, FALSE);
    PIO_STACK_LOCATION SubStack = IoGetNextIrpStackLocation(SubIrp);

    SubStack->DeviceObject = Tdo;
    SubStack->Parameters.Others.Argument1 = (PVOID)Irp;

    IoSetNextIrpStackLocation(SubIrp);
    SubStack = IoGetNextIrpStackLocation(SubIrp);
    RtlCopyMemory(SubStack, Stack, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
    SubStack->Control = 0;
    IoSetCompletionRoutine(SubIrp, IsaPdoOnRepeaterComplete, (PVOID)(ULONG_PTR)NeedsVote, TRUE, TRUE, TRUE);

    SubIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoMarkIrpPending(Irp);
    IoCallDriver(Tdo, SubIrp);
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
IsaPdoPnp(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status = Irp->IoStatus.Status;

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            if (PdoExt->IsaPnpDevice)
                Status = IsaHwActivateDevice(PdoExt->IsaPnpDevice);
            else
                Status = IsaPdoStartReadPort(PdoExt->FdoExt, IrpSp);

            if (NT_SUCCESS(Status))
                PdoExt->Common.State = dsStarted;
            break;

        case IRP_MN_STOP_DEVICE:
            if (PdoExt->IsaPnpDevice)
                Status = IsaHwDeactivateDevice(PdoExt->IsaPnpDevice);
            else
                Status = STATUS_SUCCESS;

            if (NT_SUCCESS(Status))
                PdoExt->Common.State = dsStopped;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Status = IsaPdoQueryDeviceRelations(PdoExt, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = IsaPdoQueryCapabilities(PdoExt, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            if (PdoExt->Common.Self == PdoExt->FdoExt->DataPortPdo)
                Status = IsaPdoQueryPnpDeviceState(PdoExt, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCES:
            if (PdoExt->Common.Self == PdoExt->FdoExt->DataPortPdo)
                Status = IsaPdoQueryResources(PdoExt, Irp, IrpSp);
            else
                DPRINT1("IRP_MN_QUERY_RESOURCES is UNIMPLEMENTED!\n");
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            if (PdoExt->Common.Self == PdoExt->FdoExt->DataPortPdo)
                Status = IsaPdoQueryResourceRequirements(PdoExt, Irp, IrpSp);
            else
                DPRINT1("IRP_MN_QUERY_RESOURCE_REQUIREMENTS is UNIMPLEMENTED!\n");
            break;

        case IRP_MN_QUERY_ID:
            Status = IsaPdoQueryId(PdoExt, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_DEVICE_TEXT:
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        case IRP_MN_SURPRISE_REMOVAL:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_BUS_INFORMATION:
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return IsaPdoRepeatRequest(PdoExt, Irp, TRUE);

        default:
            DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
