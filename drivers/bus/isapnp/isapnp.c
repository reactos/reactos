/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            isapnp.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  Hervé Poussineau
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
                                  L"ISAPNP\\%.3S%04x",
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
                                  L"ISAPNP\\%.3S%04x@"
                                  L"*%.3S%04x@",
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

static
NTSTATUS
NTAPI
IsaFdoCreateRequirements(
    IN PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    RTL_BITMAP IrqBitmap[ARRAYSIZE(LogDev->Irq)];
    RTL_BITMAP DmaBitmap[ARRAYSIZE(LogDev->Dma)];
    ULONG IrqData[ARRAYSIZE(LogDev->Irq)];
    ULONG DmaData[ARRAYSIZE(LogDev->Dma)];
    ULONG ResourceCount = 0;
    ULONG ListSize, i, j;
    BOOLEAN FirstIrq = TRUE, FirstDma = TRUE;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    /* Count number of requirements */
    for (i = 0; i < ARRAYSIZE(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].Description.Length)
            break;
        ResourceCount++;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            break;
        IrqData[i] = LogDev->Irq[i].Description.Mask;
        RtlInitializeBitMap(&IrqBitmap[i], &IrqData[i], 16);
        ResourceCount += RtlNumberOfSetBits(&IrqBitmap[i]);
        if (LogDev->Irq[i].Description.Information & 0x4)
        {
            /* Add room for level sensitive */
            ResourceCount += RtlNumberOfSetBits(&IrqBitmap[i]);
        }
    }
    if (ResourceCount == 0)
        return STATUS_SUCCESS;
    for (i = 0; i < ARRAYSIZE(LogDev->Irq); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            break;
        DmaData[i] = LogDev->Dma[i].Description.Mask;
        RtlInitializeBitMap(&DmaBitmap[i], &DmaData[i], 8);
        ResourceCount += RtlNumberOfSetBits(&DmaBitmap[i]);
    }

    /* Allocate memory to store requirements */
    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
             + ResourceCount * sizeof(IO_RESOURCE_DESCRIPTOR);
    RequirementsList = ExAllocatePool(PagedPool, ListSize);
    if (!RequirementsList)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(RequirementsList, ListSize);
    RequirementsList->ListSize = ListSize;
    RequirementsList->InterfaceType = Isa;
    RequirementsList->AlternativeLists = 1;

    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = ResourceCount;

    /* Store requirements */
    Descriptor = RequirementsList->List[0].Descriptors;
    for (i = 0; i < ARRAYSIZE(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].Description.Length)
            break;
        DPRINT("Device.Io[%d].Information = 0x%02x\n", i, LogDev->Io[i].Description.Information);
        DPRINT("Device.Io[%d].Minimum = 0x%02x\n", i, LogDev->Io[i].Description.Minimum);
        DPRINT("Device.Io[%d].Maximum = 0x%02x\n", i, LogDev->Io[i].Description.Maximum);
        DPRINT("Device.Io[%d].Alignment = 0x%02x\n", i, LogDev->Io[i].Description.Alignment);
        DPRINT("Device.Io[%d].Length = 0x%02x\n", i, LogDev->Io[i].Description.Length);
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Io[i].Description.Information & 0x1)
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        else
            Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Length = LogDev->Io[i].Description.Length;
        Descriptor->u.Port.Alignment = LogDev->Io[i].Description.Alignment;
        Descriptor->u.Port.MinimumAddress.LowPart = LogDev->Io[i].Description.Minimum;
        Descriptor->u.Port.MaximumAddress.LowPart = LogDev->Io[i].Description.Maximum + LogDev->Io[i].Description.Length - 1;
        Descriptor++;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].Description.Mask)
            break;
        DPRINT("Device.Irq[%d].Mask = 0x%02x\n", i, LogDev->Irq[i].Description.Mask);
        DPRINT("Device.Irq[%d].Information = 0x%02x\n", i, LogDev->Irq[i].Description.Information);
        for (j = 0; j < 15; j++)
        {
            if (!RtlCheckBit(&IrqBitmap[i], j))
                continue;
            if (FirstIrq)
                FirstIrq = FALSE;
            else
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            Descriptor->Type = CmResourceTypeInterrupt;
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
            Descriptor->u.Interrupt.MinimumVector = Descriptor->u.Interrupt.MaximumVector = j;
            Descriptor++;
            if (LogDev->Irq[i].Description.Information & 0x4)
            {
                /* Level interrupt */
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                Descriptor->Type = CmResourceTypeInterrupt;
                Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                Descriptor->u.Interrupt.MinimumVector = Descriptor->u.Interrupt.MaximumVector = j;
                Descriptor++;
            }
        }
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Dma); i++)
    {
        if (!LogDev->Dma[i].Description.Mask)
            break;
        DPRINT("Device.Dma[%d].Mask = 0x%02x\n", i, LogDev->Dma[i].Description.Mask);
        DPRINT("Device.Dma[%d].Information = 0x%02x\n", i, LogDev->Dma[i].Description.Information);
        for (j = 0; j < 8; j++)
        {
            if (!RtlCheckBit(&DmaBitmap[i], j))
                continue;
            if (FirstDma)
                FirstDma = FALSE;
            else
                Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            Descriptor->Type = CmResourceTypeDma;
            switch (LogDev->Dma[i].Description.Information & 0x3)
            {
                case 0x0: Descriptor->Flags |= CM_RESOURCE_DMA_8; break;
                case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_8_AND_16; break;
                case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_16; break;
                default: break;
            }
            if (LogDev->Dma[i].Description.Information & 0x4)
                Descriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
            switch ((LogDev->Dma[i].Description.Information >> 5) & 0x3)
            {
                case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
                case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
                case 0x3: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
                default: break;
            }
            Descriptor->u.Dma.MinimumChannel = Descriptor->u.Dma.MaximumChannel = j;
            Descriptor++;
        }
    }

    PdoExt->RequirementsList = RequirementsList;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaFdoCreateResources(
    IN PISAPNP_PDO_EXTENSION PdoExt)
{
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    ULONG ResourceCount = 0;
    ULONG ListSize, i;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    /* Count number of required resources */
    for (i = 0; i < ARRAYSIZE(LogDev->Io); i++)
    {
        if (LogDev->Io[i].CurrentBase)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Irq); i++)
    {
        if (LogDev->Irq[i].CurrentNo)
            ResourceCount++;
        else
            break;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel != 4)
            ResourceCount++;
        else
            break;
    }
    if (ResourceCount == 0)
        return STATUS_SUCCESS;

    /* Allocate memory to store resources */
    ListSize = sizeof(CM_RESOURCE_LIST)
             + (ResourceCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ResourceList = ExAllocatePool(PagedPool, ListSize);
    if (!ResourceList)
        return STATUS_NO_MEMORY;

    RtlZeroMemory(ResourceList, ListSize);
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = ResourceCount;

    /* Store resources */
    ResourceCount = 0;
    for (i = 0; i < ARRAYSIZE(LogDev->Io); i++)
    {
        if (!LogDev->Io[i].CurrentBase)
            continue;
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Io[i].Description.Information & 0x1)
            Descriptor->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        else
            Descriptor->Flags = CM_RESOURCE_PORT_10_BIT_DECODE;
        Descriptor->u.Port.Length = LogDev->Io[i].Description.Length;
        Descriptor->u.Port.Start.LowPart = LogDev->Io[i].CurrentBase;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Irq); i++)
    {
        if (!LogDev->Irq[i].CurrentNo)
            continue;
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        if (LogDev->Irq[i].CurrentType & 0x01)
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        else
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->u.Interrupt.Level = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Vector = LogDev->Irq[i].CurrentNo;
        Descriptor->u.Interrupt.Affinity = -1;
    }
    for (i = 0; i < ARRAYSIZE(LogDev->Dma); i++)
    {
        if (LogDev->Dma[i].CurrentChannel == 4)
            continue;
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[ResourceCount++];
        Descriptor->Type = CmResourceTypeDma;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        switch (LogDev->Dma[i].Description.Information & 0x3)
        {
            case 0x0: Descriptor->Flags |= CM_RESOURCE_DMA_8; break;
            case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_8 | CM_RESOURCE_DMA_16; break;
            case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_16; break;
            default: break;
        }
        if (LogDev->Dma[i].Description.Information & 0x4)
            Descriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
        switch ((LogDev->Dma[i].Description.Information >> 5) & 0x3)
        {
            case 0x1: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
            case 0x2: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
            case 0x3: Descriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
            default: break;
        }
        Descriptor->u.Dma.Channel = LogDev->Dma[i].CurrentChannel;
    }

    PdoExt->ResourceList = ResourceList;
    PdoExt->ResourceListSize = ListSize;
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

           if (NT_SUCCESS(Status))
              Status = IsaFdoCreateRequirements(PdoExt);

           if (NT_SUCCESS(Status))
              Status = IsaFdoCreateResources(PdoExt);

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

DRIVER_DISPATCH IsaPower;
NTSTATUS
NTAPI
IsaPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PISAPNP_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    if (!DevExt->IsFdo)
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(((PISAPNP_FDO_EXTENSION)DevExt)->Ldo, Irp);
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
    DriverObject->MajorFunction[IRP_MJ_POWER] = IsaPower;
    DriverObject->DriverExtension->AddDevice = IsaAddDevice;

    return STATUS_SUCCESS;
}

/* EOF */
