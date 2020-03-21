/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            isapnp.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <isapnp.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IsaPnpDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString)
{
    if (SourceString == NULL ||
        DestinationString == NULL ||
        SourceString->Length > SourceString->MaximumLength ||
        (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL) ||
        Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING ||
        Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((SourceString->Length == 0) &&
        (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        USHORT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = ExAllocatePool(PagedPool, DestMaxLength);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaFdoCreateDeviceIDs(
    IN PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    UNICODE_STRING TempString;
    WCHAR TempBuffer[256];
    PWCHAR End;
    NTSTATUS Status;
    USHORT i;

    TempString.Buffer = TempBuffer;
    TempString.MaximumLength = sizeof(TempBuffer);
    TempString.Length = 0;

    /* Device ID */
    Status = RtlStringCbPrintfExW(TempString.Buffer,
                                  TempString.MaximumLength / sizeof(WCHAR),
                                  &End,
                                  NULL, 0,
                                  L"ISAPNP\\%3S%04X",
                                  LogDev->VendorId,
                                  LogDev->ProdId);
    if (!NT_SUCCESS(Status))
        return Status;
    TempString.Length = (USHORT)((End - TempString.Buffer) * sizeof(WCHAR));
    Status = IsaPnpDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &TempString,
        &PdoExt->DeviceID);
    if (!NT_SUCCESS(Status))
        return Status;

    /* HardwareIDs */
    Status = RtlStringCbPrintfExW(TempString.Buffer,
                                  TempString.MaximumLength / sizeof(WCHAR),
                                  &End,
                                  NULL, 0,
                                  L"ISAPNP\\%3S%04X@"
                                  L"*%3S%04X@",
                                  LogDev->VendorId,
                                  LogDev->ProdId,
                                  LogDev->VendorId,
                                  LogDev->ProdId);
    if (!NT_SUCCESS(Status))
        return Status;
    TempString.Length = (USHORT)((End - TempString.Buffer) * sizeof(WCHAR));
    Status = IsaPnpDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &TempString,
        &PdoExt->HardwareIDs);
    if (!NT_SUCCESS(Status))
        return Status;
    for (i = 0; i < PdoExt->HardwareIDs.Length / sizeof(WCHAR); i++)
        if (PdoExt->HardwareIDs.Buffer[i] == '@')
            PdoExt->HardwareIDs.Buffer[i] = UNICODE_NULL;

    /* InstanceID */
    Status = RtlStringCbPrintfExW(TempString.Buffer,
                                  TempString.MaximumLength / sizeof(WCHAR),
                                  &End,
                                  NULL, 0,
                                  L"%X",
                                  LogDev->SerialNumber);
    if (!NT_SUCCESS(Status))
        return Status;
    TempString.Length = (USHORT)((End - TempString.Buffer) * sizeof(WCHAR));
    Status = IsaPnpDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &TempString,
        &PdoExt->InstanceID);
    if (!NT_SUCCESS(Status))
        return Status;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPnpFillDeviceRelations(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp,
    IN BOOLEAN IncludeDataPort)
{
    PISAPNP_PDO_EXTENSION PdoExt;
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY CurrentEntry;
    PISAPNP_LOGICAL_DEVICE IsaDevice;
    PDEVICE_RELATIONS DeviceRelations;
    ULONG i = 0;

    DeviceRelations = ExAllocatePool(NonPagedPool,
                                     sizeof(DEVICE_RELATIONS) + sizeof(DEVICE_OBJECT) * FdoExt->DeviceCount);
    if (!DeviceRelations)
    {
        return STATUS_NO_MEMORY;
    }

    if (IncludeDataPort)
    {
        DeviceRelations->Objects[i++] = FdoExt->DataPortPdo;
        ObReferenceObject(FdoExt->DataPortPdo);
    }

    CurrentEntry = FdoExt->DeviceListHead.Flink;
    while (CurrentEntry != &FdoExt->DeviceListHead)
    {
       IsaDevice = CONTAINING_RECORD(CurrentEntry, ISAPNP_LOGICAL_DEVICE, ListEntry);

       if (!IsaDevice->Pdo)
       {
           Status = IoCreateDevice(FdoExt->DriverObject,
                                   sizeof(ISAPNP_PDO_EXTENSION),
                                   NULL,
                                   FILE_DEVICE_CONTROLLER,
                                   FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME,
                                   FALSE,
                                   &IsaDevice->Pdo);
           if (!NT_SUCCESS(Status))
           {
              break;
           }

           IsaDevice->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

           //Device->Pdo->Flags |= DO_POWER_PAGABLE;

           PdoExt = (PISAPNP_PDO_EXTENSION)IsaDevice->Pdo->DeviceExtension;

           RtlZeroMemory(PdoExt, sizeof(ISAPNP_PDO_EXTENSION));

           PdoExt->Common.IsFdo = FALSE;
           PdoExt->Common.Self = IsaDevice->Pdo;
           PdoExt->Common.State = dsStopped;
           PdoExt->IsaPnpDevice = IsaDevice;
           PdoExt->FdoExt = FdoExt;

           Status = IsaFdoCreateDeviceIDs(PdoExt);
           if (!NT_SUCCESS(Status))
           {
               IoDeleteDevice(IsaDevice->Pdo);
               IsaDevice->Pdo = NULL;
               break;
           }
       }
       DeviceRelations->Objects[i++] = IsaDevice->Pdo;

       ObReferenceObject(IsaDevice->Pdo);

       CurrentEntry = CurrentEntry->Flink;
    }

    DeviceRelations->Count = i;

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return Status;
}


static IO_COMPLETION_ROUTINE ForwardIrpCompletion;

static
NTSTATUS
NTAPI
ForwardIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
IsaForwardIrpSynchronous(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, ForwardIrpCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(FdoExt->Ldo, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
}

static DRIVER_DISPATCH IsaCreateClose;

static
NTSTATUS
NTAPI
IsaCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

static DRIVER_DISPATCH IsaIoctl;

static
NTSTATUS
NTAPI
IsaIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        default:
            DPRINT1("Unknown ioctl code: %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static DRIVER_DISPATCH IsaReadWrite;

static
NTSTATUS
NTAPI
IsaReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
IsaPnpCreateReadPortDORequirements(
    IN PISAPNP_PDO_EXTENSION PdoExt)
{
    USHORT Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS, 0x274, 0x3e4, 0x204, 0x2e4, 0x354, 0x2f4 };
    ULONG ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
             + 2 * ARRAYSIZE(Ports) * sizeof(IO_RESOURCE_DESCRIPTOR);
    RequirementsList = ExAllocatePool(PagedPool, ListSize);
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

    PdoExt->RequirementsList = RequirementsList;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaPnpCreateReadPortDOResources(
    IN PISAPNP_PDO_EXTENSION PdoExt)
{
    USHORT Ports[] = { ISAPNP_WRITE_DATA, ISAPNP_ADDRESS };
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    ListSize = sizeof(CM_RESOURCE_LIST)
             + (ARRAYSIZE(Ports) - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ResourceList = ExAllocatePool(PagedPool, ListSize);
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

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaPnpCreateReadPortDO(PISAPNP_FDO_EXTENSION FdoExt)
{
    UNICODE_STRING DeviceID = RTL_CONSTANT_STRING(L"ISAPNP\\ReadDataPort\0");
    UNICODE_STRING HardwareIDs = RTL_CONSTANT_STRING(L"ISAPNP\\ReadDataPort\0\0");
    UNICODE_STRING CompatibleIDs = RTL_CONSTANT_STRING(L"\0\0");
    UNICODE_STRING InstanceID = RTL_CONSTANT_STRING(L"0\0");
    PISAPNP_PDO_EXTENSION PdoExt;

    NTSTATUS Status;
    Status = IoCreateDevice(FdoExt->DriverObject,
                            sizeof(ISAPNP_PDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &FdoExt->DataPortPdo);
    if (!NT_SUCCESS(Status))
        return Status;
    PdoExt = (PISAPNP_PDO_EXTENSION)FdoExt->DataPortPdo->DeviceExtension;
    RtlZeroMemory(PdoExt, sizeof(ISAPNP_PDO_EXTENSION));
    PdoExt->Common.IsFdo = FALSE;
    PdoExt->Common.Self = FdoExt->DataPortPdo;
    PdoExt->Common.State = dsStopped;
    PdoExt->FdoExt = FdoExt;

    Status = IsaPnpDuplicateUnicodeString(0,
                                          &DeviceID,
                                          &PdoExt->DeviceID);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IsaPnpDuplicateUnicodeString(0,
                                          &HardwareIDs,
                                          &PdoExt->HardwareIDs);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IsaPnpDuplicateUnicodeString(0,
                                          &CompatibleIDs,
                                          &PdoExt->CompatibleIDs);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IsaPnpDuplicateUnicodeString(0,
                                          &InstanceID,
                                          &PdoExt->InstanceID);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IsaPnpCreateReadPortDORequirements(PdoExt);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IsaPnpCreateReadPortDOResources(PdoExt);
    if (!NT_SUCCESS(Status))
        return Status;

    return Status;
}

static
NTSTATUS
NTAPI
IsaAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT Fdo;
    PISAPNP_FDO_EXTENSION FdoExt;
    NTSTATUS Status;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

    Status = IoCreateDevice(DriverObject,
                            sizeof(*FdoExt),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            TRUE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create FDO (0x%x)\n", Status);
        return Status;
    }

    FdoExt = Fdo->DeviceExtension;
    RtlZeroMemory(FdoExt, sizeof(*FdoExt));

    FdoExt->Common.Self = Fdo;
    FdoExt->Common.IsFdo = TRUE;
    FdoExt->Common.State = dsStopped;
    FdoExt->DriverObject = DriverObject;
    FdoExt->Pdo = PhysicalDeviceObject;
    FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                              PhysicalDeviceObject);

    InitializeListHead(&FdoExt->DeviceListHead);
    KeInitializeSpinLock(&FdoExt->Lock);

    Status = IsaPnpCreateReadPortDO(FdoExt);
    if (!NT_SUCCESS(Status))
        return Status;

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    FdoExt->DataPortPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static DRIVER_DISPATCH IsaPnp;

static
NTSTATUS
NTAPI
IsaPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PISAPNP_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

    if (DevExt->IsFdo)
    {
       return IsaFdoPnp((PISAPNP_FDO_EXTENSION)DevExt,
                        Irp,
                        IrpSp);
    }
    else
    {
       return IsaPdoPnp((PISAPNP_PDO_EXTENSION)DevExt,
                        Irp,
                        IrpSp);
    }
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IsaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IsaCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = IsaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = IsaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IsaIoctl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IsaPnp;
    DriverObject->DriverExtension->AddDevice = IsaAddDevice;

    return STATUS_SUCCESS;
}

/* EOF */
