/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     IRP_MJ_PNP operations for PDOs
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>

static
CODE_SEG("PAGE")
VOID
PciIdeXPdoFreeDmaResources(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PDMA_OPERATIONS DmaOperations;
    PDMA_ADAPTER AdapterObject;

    PAGED_CODE();

    PdoExtension->Flags &= ~(PDO_PIO_ONLY | PDO_DRIVE0_DMA_CAPABLE | PDO_DRIVE1_DMA_CAPABLE);

    AdapterObject = PdoExtension->AdapterObject;
    if (!AdapterObject)
        return;

    DmaOperations = AdapterObject->DmaOperations;

    if (PdoExtension->PrdTable)
    {
        PHYSICAL_ADDRESS PrdTablePhysicalAddress;

        PrdTablePhysicalAddress.QuadPart = PdoExtension->PrdTablePhysicalAddress;

        DmaOperations->FreeCommonBuffer(AdapterObject,
                                        PdoExtension->MapRegisterCount * sizeof(PRD_TABLE_ENTRY),
                                        PrdTablePhysicalAddress,
                                        PdoExtension->PrdTable,
                                        FALSE);
        PdoExtension->PrdTable = NULL;
    }

    DmaOperations->PutDmaAdapter(AdapterObject);

    PdoExtension->AdapterObject = NULL;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeXPdoGetDmaAdapter(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    DEVICE_DESCRIPTION DeviceDescription = { 0 };

    PAGED_CODE();

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = ATA_MAX_TRANSFER_LENGTH;

    PdoExtension->AdapterObject = IoGetDmaAdapter(PdoExtension->ParentController->Ldo,
                                                  &DeviceDescription,
                                                  &PdoExtension->MapRegisterCount);
    if (!PdoExtension->AdapterObject)
        return FALSE;

    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeXPdoAllocatePrdTable(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PHYSICAL_ADDRESS LogicalAddress;

    PAGED_CODE();

    PdoExtension->PrdTable = PdoExtension->AdapterObject->DmaOperations->
        AllocateCommonBuffer(PdoExtension->AdapterObject,
                             PdoExtension->MapRegisterCount * sizeof(PRD_TABLE_ENTRY),
                             &LogicalAddress,
                             FALSE);
    if (!PdoExtension->PrdTable)
        return FALSE;

    /* 32-bit DMA */
    ASSERT(LogicalAddress.HighPart == 0);

    PdoExtension->PrdTablePhysicalAddress = LogicalAddress.LowPart;

    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
PciIdeXPdoAllocateDmaResources(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    BOOLEAN Success;
    UCHAR DmaStatus;

    PAGED_CODE();

    PdoExtension->IoBase = PdoExtension->ParentController->BusMasterPortBase;
    if (!IS_PRIMARY_CHANNEL(PdoExtension))
    {
        PdoExtension->IoBase += DMA_SECONDARY_CHANNEL_OFFSET;
    }

    DmaStatus = READ_PORT_UCHAR(PdoExtension->IoBase + DMA_STATUS);

    DPRINT("I/O Base %p, status 0x%02x\n", PdoExtension->IoBase, DmaStatus);

    /* The status bits 3:4 must return 0 on reads */
    if (DmaStatus & (DMA_STATUS_RESERVED1 | DMA_STATUS_RESERVED2))
    {
        DPRINT1("Unexpected DMA status 0x%02x\n", DmaStatus);
        goto DisableDma;
    }

    /* The status bits 5:6 are set by the miniport driver or BIOS firmware at boot */
    if (DmaStatus & DMA_STATUS_DRIVE0_DMA_CAPABLE)
        PdoExtension->Flags |= PDO_DRIVE0_DMA_CAPABLE;
    if (DmaStatus & DMA_STATUS_DRIVE1_DMA_CAPABLE)
        PdoExtension->Flags |= PDO_DRIVE1_DMA_CAPABLE;

    Success = PciIdeXPdoGetDmaAdapter(PdoExtension);
    if (!Success)
    {
        DPRINT1("Unable to get DMA adapter\n");
        goto DisableDma;
    }

    Success = PciIdeXPdoAllocatePrdTable(PdoExtension);
    if (!Success)
    {
        DPRINT1("Unable to allocate PRD table\n");

        PciIdeXPdoFreeDmaResources(PdoExtension);
        goto DisableDma;
    }

    return;

DisableDma:
    /* Failed to setup DMA, falling back to PIO mode */
    PdoExtension->Flags |= PDO_PIO_ONLY;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoStartDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    PFDO_DEVICE_EXTENSION FdoExtension = PdoExtension->ParentController;

    PAGED_CODE();

    if (FdoExtension->Flags & FDO_DMA_CAPABLE)
    {
        PciIdeXPdoAllocateDmaResources(PdoExtension);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoStopDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PAGED_CODE();

    PciIdeXPdoFreeDmaResources(PdoExtension);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoRemoveDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ BOOLEAN FinalRemove)
{
    PFDO_DEVICE_EXTENSION FdoExtension = PdoExtension->ParentController;
    ULONG i;

    PAGED_CODE();

    PciIdeXPdoFreeDmaResources(PdoExtension);

    if (FinalRemove && PdoExtension->ReportedMissing)
    {
        ExAcquireFastMutex(&FdoExtension->DeviceSyncMutex);

        for (i = 0; i < MAX_IDE_CHANNEL; ++i)
        {
            if (FdoExtension->Channels[i] == PdoExtension)
            {
                FdoExtension->Channels[i] = NULL;
                break;
            }
        }

        ExReleaseFastMutex(&FdoExtension->DeviceSyncMutex);

        IoDeleteDevice(PdoExtension->Common.Self);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryStopRemoveDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PAGED_CODE();

    if (PdoExtension->Common.PageFiles ||
        PdoExtension->Common.HibernateFiles ||
        PdoExtension->Common.DumpFiles)
    {
        return STATUS_DEVICE_BUSY;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryTargetDeviceRelations(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;

    PAGED_CODE();

    DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(DEVICE_RELATIONS),
                                            TAG_PCIIDEX);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = PdoExtension->Common.Self;
    ObReferenceObject(PdoExtension->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static IO_COMPLETION_ROUTINE PciIdeXOnRepeaterCompletion;

static
NTSTATUS
NTAPI
PciIdeXOnRepeaterCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent(Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoRepeatRequest(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PDEVICE_OBJECT Fdo, TopDeviceObject;
    PIO_STACK_LOCATION IoStack, SubStack;
    PIRP SubIrp;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    Fdo = PdoExtension->ParentController->Common.Self;
    TopDeviceObject = IoGetAttachedDeviceReference(Fdo);

    SubIrp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!SubIrp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    SubStack = IoGetNextIrpStackLocation(SubIrp);
    RtlCopyMemory(SubStack, IoStack, sizeof(IO_STACK_LOCATION));

    if (DeviceCapabilities)
        SubStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    IoSetCompletionRoutine(SubIrp,
                           PciIdeXOnRepeaterCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    SubIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(TopDeviceObject, SubIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    ObDereferenceObject(TopDeviceObject);

    Status = SubIrp->IoStatus.Status;
    IoFreeIrp(SubIrp);

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryCapabilities(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    DEVICE_CAPABILITIES ParentCapabilities;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the capabilities of the parent device */
    RtlZeroMemory(&ParentCapabilities, sizeof(ParentCapabilities));
    ParentCapabilities.Size = sizeof(ParentCapabilities);
    ParentCapabilities.Version = 1;
    ParentCapabilities.Address = MAXULONG;
    ParentCapabilities.UINumber = MAXULONG;
    Status = PciIdeXPdoRepeatRequest(PdoExtension, Irp, &ParentCapabilities);
    if (!NT_SUCCESS(Status))
        return Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceCapabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;
    *DeviceCapabilities = ParentCapabilities;

    /* Override some fields */
    DeviceCapabilities->UniqueID = FALSE;
    DeviceCapabilities->Address = PdoExtension->Channel;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryPnpDeviceState(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (PdoExtension->Common.PageFiles ||
        PdoExtension->Common.HibernateFiles ||
        PdoExtension->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryResources(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    IDE_CHANNEL_STATE ChannelState;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG CommandPortBase, ControlPortBase, InterruptVector;
    ULONG ListSize;

    PAGED_CODE();

    FdoExtension = PdoExtension->ParentController;
    if (FdoExtension->Flags & FDO_IN_NATIVE_MODE)
        return Irp->IoStatus.Status;

    ChannelState = PciIdeXChannelState(FdoExtension, PdoExtension->Channel);
    if (ChannelState == ChannelDisabled)
        return Irp->IoStatus.Status;

    ListSize = sizeof(CM_RESOURCE_LIST) +
               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (PCIIDE_LEGACY_RESOURCE_COUNT - 1);
    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, TAG_PCIIDEX);
    if (!ResourceList)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Legacy mode resources */
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = PCIIDE_LEGACY_RESOURCE_COUNT;

    if (IS_PRIMARY_CHANNEL(PdoExtension))
    {
        CommandPortBase = PCIIDE_LEGACY_PRIMARY_COMMAND_BASE;
        ControlPortBase = PCIIDE_LEGACY_PRIMARY_CONTROL_BASE;
        InterruptVector = PCIIDE_LEGACY_PRIMARY_IRQ;
    }
    else
    {
        CommandPortBase = PCIIDE_LEGACY_SECONDARY_COMMAND_BASE;
        ControlPortBase = PCIIDE_LEGACY_SECONDARY_CONTROL_BASE;
        InterruptVector = PCIIDE_LEGACY_SECONDARY_IRQ;
    }

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    /* Command port base */
    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    Descriptor->u.Port.Length = PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH;
    Descriptor->u.Port.Start.LowPart = CommandPortBase;
    ++Descriptor;

    /* Control port base */
    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    Descriptor->u.Port.Length = PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH;
    Descriptor->u.Port.Start.LowPart = ControlPortBase;
    ++Descriptor;

    /* Interrupt */
    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    Descriptor->u.Interrupt.Level = InterruptVector;
    Descriptor->u.Interrupt.Vector = InterruptVector;
    Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryResourceRequirements(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    IDE_CHANNEL_STATE ChannelState;
    ULONG CommandPortBase, ControlPortBase, InterruptVector;
    ULONG ListSize;

    PAGED_CODE();

    FdoExtension = PdoExtension->ParentController;
    if (FdoExtension->Flags & FDO_IN_NATIVE_MODE)
        return Irp->IoStatus.Status;

    ChannelState = PciIdeXChannelState(FdoExtension, PdoExtension->Channel);
    if (ChannelState == ChannelDisabled)
        return Irp->IoStatus.Status;

    ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               sizeof(IO_RESOURCE_DESCRIPTOR) * (PCIIDE_LEGACY_RESOURCE_COUNT - 1);
    RequirementsList = ExAllocatePoolZero(PagedPool, ListSize, TAG_PCIIDEX);
    if (!RequirementsList)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Legacy mode resources */
    RequirementsList->InterfaceType = Isa;
    RequirementsList->ListSize = ListSize;
    RequirementsList->AlternativeLists = 1;
    RequirementsList->List[0].Version = 1;
    RequirementsList->List[0].Revision = 1;
    RequirementsList->List[0].Count = PCIIDE_LEGACY_RESOURCE_COUNT;

    if (IS_PRIMARY_CHANNEL(PdoExtension))
    {
        CommandPortBase = PCIIDE_LEGACY_PRIMARY_COMMAND_BASE;
        ControlPortBase = PCIIDE_LEGACY_PRIMARY_CONTROL_BASE;
        InterruptVector = PCIIDE_LEGACY_PRIMARY_IRQ;
    }
    else
    {
        CommandPortBase = PCIIDE_LEGACY_SECONDARY_COMMAND_BASE;
        ControlPortBase = PCIIDE_LEGACY_SECONDARY_CONTROL_BASE;
        InterruptVector = PCIIDE_LEGACY_SECONDARY_IRQ;
    }

    Descriptor = &RequirementsList->List[0].Descriptors[0];

    /* Command port base */
    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    Descriptor->u.Port.Length = PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH;
    Descriptor->u.Port.Alignment = 1;
    Descriptor->u.Port.MinimumAddress.LowPart = CommandPortBase;
    Descriptor->u.Port.MaximumAddress.LowPart = CommandPortBase +
                                                PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH - 1;
    ++Descriptor;

    /* Control port base */
    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    Descriptor->u.Port.Length = PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH;
    Descriptor->u.Port.Alignment = 1;
    Descriptor->u.Port.MinimumAddress.LowPart = ControlPortBase;
    Descriptor->u.Port.MaximumAddress.LowPart = ControlPortBase +
                                                PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH - 1;
    ++Descriptor;

    /* Interrupt */
    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->ShareDisposition = CmResourceShareShared;
    Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    Descriptor->u.Interrupt.MinimumVector = InterruptVector;
    Descriptor->u.Interrupt.MaximumVector = InterruptVector;

    Irp->IoStatus.Information = (ULONG_PTR)RequirementsList;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
PCWSTR
PciIdeXGetControllerVendorId(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    switch (FdoExtension->VendorId)
    {
        case 0x0E11:
            return L"Compaq";
        case 0x1039:
            return L"SiS";
        case 0x1050:
            return L"WinBond";
        case 0x1095:
            return L"CMD";
        case 0x10B9:
            return L"ALi";
        case 0x8086:
            return L"Intel";

        default:
            break;
    }

    /* Only certain controllers have a non-numeric identifier */
    return NULL;
}

static
CODE_SEG("PAGE")
PCWSTR
PciIdeXGetControllerDeviceId(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    /* Intel */
    if (FdoExtension->VendorId == 0x8086)
    {
        switch (FdoExtension->DeviceId)
        {
            case 0x1230:
                return L"PIIX";
            case 0x7010:
                return L"PIIX3";
            case 0x7111:
                return L"PIIX4";

            default:
                break;
        }
    }

    return NULL;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryId(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PWCHAR Buffer, End;
    size_t CharCount, Remaining;
    static const WCHAR IdeCompatibleId[] = L"*PNP0600";

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.QueryId.IdType)
    {
      case BusQueryDeviceID:
      {
          static const WCHAR PciIdeDeviceId[] = L"PCIIDE\\IDEChannel";

          Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(PciIdeDeviceId), TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          RtlCopyMemory(Buffer, PciIdeDeviceId, sizeof(PciIdeDeviceId));

          DPRINT("Device ID: '%S'\n", Buffer);
          break;
      }

      case BusQueryHardwareIDs:
      {
          PFDO_DEVICE_EXTENSION FdoExtension;
          PCWSTR VendorString;
          PWCHAR IdStart;

          DBG_UNREFERENCED_LOCAL_VARIABLE(IdStart);

          /* Maximum string length */
          CharCount = sizeof("WinBond-1234") +
                      sizeof("Secondary_IDE_Channel") +
                      sizeof(IdeCompatibleId) +
                      sizeof(ANSI_NULL); /* multi-string */

          Buffer = ExAllocatePoolWithTag(PagedPool,
                                         CharCount * sizeof(WCHAR),
                                         TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          FdoExtension = PdoExtension->ParentController;
          VendorString = PciIdeXGetControllerVendorId(FdoExtension);

          DPRINT("HardwareIDs:\n");

          /* ID 1 */
          if (VendorString)
          {
              PCWSTR DeviceString = PciIdeXGetControllerDeviceId(FdoExtension);

              if (DeviceString)
              {
                  Status = RtlStringCchPrintfExW(Buffer,
                                                 CharCount,
                                                 &End,
                                                 &Remaining,
                                                 0,
                                                 L"%ls-%ls",
                                                 VendorString,
                                                 DeviceString);
              }
              else
              {
                  Status = RtlStringCchPrintfExW(Buffer,
                                                 CharCount,
                                                 &End,
                                                 &Remaining,
                                                 0,
                                                 L"%ls-%04x",
                                                 VendorString,
                                                 FdoExtension->DeviceId);
              }
          }
          else
          {
              Status = RtlStringCchPrintfExW(Buffer,
                                             CharCount,
                                             &End,
                                             &Remaining,
                                             0,
                                             L"%04x-%04x",
                                             FdoExtension->VendorId,
                                             FdoExtension->DeviceId);
          }
          ASSERT(NT_SUCCESS(Status));

          DPRINT("  '%S'\n", Buffer);

          ++End;
          --Remaining;

          /* ID 2 */
          IdStart = End;
          Status = RtlStringCchPrintfExW(End,
                                         Remaining,
                                         &End,
                                         &Remaining,
                                         0,
                                         L"%ls",
                                         IS_PRIMARY_CHANNEL(PdoExtension) ?
                                         L"Primary_IDE_Channel" :
                                         L"Secondary_IDE_Channel");
          ASSERT(NT_SUCCESS(Status));

          DPRINT("  '%S'\n", IdStart);

          ++End;
          --Remaining;

          /* ID 3 */
          IdStart = End;
          Status = RtlStringCchPrintfExW(End,
                                         Remaining,
                                         &End,
                                         &Remaining,
                                         0,
                                         L"%ls",
                                         IdeCompatibleId);
          ASSERT(NT_SUCCESS(Status));

          DPRINT("  '%S'\n", IdStart);

          *++End = UNICODE_NULL; /* multi-string */
          break;
      }

      case BusQueryCompatibleIDs:
      {
          Buffer = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(IdeCompatibleId) + sizeof(UNICODE_NULL),
                                         TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          RtlCopyMemory(Buffer, IdeCompatibleId, sizeof(IdeCompatibleId));

          Buffer[sizeof(IdeCompatibleId) / sizeof(WCHAR)] = UNICODE_NULL; /* multi-string */

          DPRINT("Compatible ID: '%S'\n", Buffer);
          break;
      }

      case BusQueryInstanceID:
      {
          CharCount = sizeof("0");

          Buffer = ExAllocatePoolWithTag(PagedPool,
                                         CharCount * sizeof(WCHAR),
                                         TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          Status = RtlStringCchPrintfExW(Buffer,
                                         CharCount,
                                         NULL,
                                         NULL,
                                         0,
                                         L"%lu",
                                         PdoExtension->Channel);
          ASSERT(NT_SUCCESS(Status));

          DPRINT("Instance ID: '%S'\n", Buffer);
          break;
      }

      default:
          return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryDeviceText(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PWCHAR Buffer;
    ULONG Size;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextLocationInformation:
        {
            static const WCHAR PrimaryChannelText[] = L"Primary channel";
            static const WCHAR SecondaryChannelText[] = L"Secondary channel";

            if (IS_PRIMARY_CHANNEL(PdoExtension))
                Size = sizeof(PrimaryChannelText);
            else
                Size = sizeof(SecondaryChannelText);

            Buffer = ExAllocatePoolWithTag(PagedPool, Size, TAG_PCIIDEX);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            RtlCopyMemory(Buffer,
                          IS_PRIMARY_CHANNEL(PdoExtension) ?
                          PrimaryChannelText : SecondaryChannelText,
                          Size);

            DPRINT("Device ID: '%S'\n", Buffer);
            break;
        }

        default:
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryDeviceUsageNotification(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    Status = PciIdeXPdoRepeatRequest(PdoExtension, Irp, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &PdoExtension->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &PdoExtension->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &PdoExtension->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);
    IoInvalidateDeviceState(PdoExtension->Common.Self);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXQueryPciIdeInterface(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PPCIIDE_INTERFACE PciIdeInterface;

    PAGED_CODE();

    if (IoStack->Parameters.QueryInterface.Size < sizeof(*PciIdeInterface))
        return Irp->IoStatus.Status;

    PciIdeInterface = (PPCIIDE_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    PciIdeInterface->IoBase = PdoExtension->IoBase;

    if (PdoExtension->Flags & PDO_PIO_ONLY)
    {
        PciIdeInterface->PrdTable = NULL;
    }
    else
    {
        PciIdeInterface->PrdTable = PdoExtension->PrdTable;
        PciIdeInterface->PrdTablePhysicalAddress = PdoExtension->PrdTablePhysicalAddress;
        PciIdeInterface->MaximumTransferLength = PdoExtension->MapRegisterCount << PAGE_SHIFT;
        PciIdeInterface->AdapterObject = PdoExtension->AdapterObject;
        PciIdeInterface->DeviceObject = PdoExtension->Common.Self;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoQueryInterface(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                           &GUID_PCIIDE_INTERFACE))
    {
        Status = PciIdeXQueryPciIdeInterface(PdoExtension, Irp, IoStack);
    }
    else
    {
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoDispatchPnp(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Status = PciIdeXPdoStartDevice(PdoExtension,
                                           IoStack->Parameters.StartDevice.AllocatedResources);
            break;

        case IRP_MN_STOP_DEVICE:
            Status = PciIdeXPdoStopDevice(PdoExtension);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = PciIdeXPdoQueryStopRemoveDevice(PdoExtension);
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:
            Status = PciIdeXPdoRemoveDevice(PdoExtension,
                                            IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
                Status = PciIdeXPdoQueryTargetDeviceRelations(PdoExtension, Irp);
            else
                Status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = PciIdeXPdoQueryCapabilities(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = PciIdeXPdoQueryPnpDeviceState(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_RESOURCES:
            Status = PciIdeXPdoQueryResources(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            Status = PciIdeXPdoQueryResourceRequirements(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_ID:
            Status = PciIdeXPdoQueryId(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            Status = PciIdeXPdoQueryDeviceText(PdoExtension, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = PciIdeXPdoQueryDeviceUsageNotification(PdoExtension, Irp);
            break;

        case IRP_MN_QUERY_INTERFACE:
            Status = PciIdeXPdoQueryInterface(PdoExtension, Irp);
            break;

        default:
            Status = Irp->IoStatus.Status;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
PciIdeXDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    if (IS_FDO(DeviceObject->DeviceExtension))
        return PciIdeXFdoDispatchPnp(DeviceObject->DeviceExtension, Irp);
    else
        return PciIdeXPdoDispatchPnp(DeviceObject->DeviceExtension, Irp);
}
