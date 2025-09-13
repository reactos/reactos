/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     IRP_MJ_PNP operations for PDOs
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#include "pciidex.h"

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoStartDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PFDO_DEVICE_EXTENSION FdoExtension = PdoExtension->Common.FdoExt;
    NTSTATUS Status;

    PAGED_CODE();

    Status = PciIdeStartChannel(&FdoExtension->Controller, PdoExtension, ResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to start channel 0x%lx\n", PdoExtension->Channel, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoStopDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PFDO_DEVICE_EXTENSION FdoExtension = PdoExtension->Common.FdoExt;

    PAGED_CODE();

    PciIdeStopChannel(&FdoExtension->Controller, PdoExtension);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPdoRemoveDevice(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove,
    _In_ BOOLEAN LockNeeded)
{
    PFDO_DEVICE_EXTENSION FdoExtension = PdoExtension->Common.FdoExt;

    PAGED_CODE();

    PciIdeStopChannel(&FdoExtension->Controller, PdoExtension);

    if (FinalRemove && (PdoExtension->Flags & PDO_FLAG_REPORTED_MISSING))
    {
        IoReleaseRemoveLockAndWait(&PdoExtension->Common.RemoveLock, Irp);

        if (LockNeeded)
            ExAcquireFastMutex(&FdoExtension->PdoListSyncMutex);

        RemoveEntryList(&PdoExtension->ListEntry);

        if (LockNeeded)
            ExReleaseFastMutex(&FdoExtension->PdoListSyncMutex);

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

    DeviceRelations = ExAllocatePoolUninitialized(PagedPool,
                                                  sizeof(*DeviceRelations),
                                                  TAG_PCIIDEX);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = PdoExtension->Common.Self;
    ObReferenceObject(PdoExtension->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
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
    Status = PciIdeXPnpRepeatRequest(&PdoExtension->Common, Irp, &ParentCapabilities);
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

    FdoExtension = PdoExtension->Common.FdoExt;
    if (!(FdoExtension->Controller.Flags & CTRL_FLAG_IN_LEGACY_MOVE))
        return Irp->IoStatus.Status;

    ChannelState = PciIdeXGetChannelState(&FdoExtension->Controller, PdoExtension->Channel);
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

    FdoExtension = PdoExtension->Common.FdoExt;
    if (!(FdoExtension->Controller.Flags & CTRL_FLAG_IN_LEGACY_MOVE))
        return Irp->IoStatus.Status;

    ChannelState = PciIdeXGetChannelState(&FdoExtension->Controller, PdoExtension->Channel);
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

    switch (FdoExtension->Controller.Pci.VendorID)
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
    if (FdoExtension->Controller.Pci.VendorID == 0x8086)
    {
        switch (FdoExtension->Controller.Pci.DeviceID)
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

    /* Only certain controllers have a non-numeric identifier */
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

          Buffer = ExAllocatePoolUninitialized(PagedPool, sizeof(PciIdeDeviceId), TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          RtlCopyMemory(Buffer, PciIdeDeviceId, sizeof(PciIdeDeviceId));

          INFO("Device ID: '%S'\n", Buffer);
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

          Buffer = ExAllocatePoolUninitialized(PagedPool,
                                               CharCount * sizeof(WCHAR),
                                               TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          FdoExtension = PdoExtension->Common.FdoExt;
          VendorString = PciIdeXGetControllerVendorId(FdoExtension);

          INFO("HardwareIDs:\n");

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
                                                 FdoExtension->Controller.Pci.DeviceID);
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
                                             FdoExtension->Controller.Pci.VendorID,
                                             FdoExtension->Controller.Pci.DeviceID);
          }
          ASSERT(NT_SUCCESS(Status));

          INFO("  '%S'\n", Buffer);

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

          INFO("  '%S'\n", IdStart);

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

          INFO("  '%S'\n", IdStart);

          *++End = UNICODE_NULL; /* multi-string */
          break;
      }

      case BusQueryCompatibleIDs:
      {
          Buffer = ExAllocatePoolUninitialized(PagedPool,
                                               sizeof(IdeCompatibleId) + sizeof(UNICODE_NULL),
                                               TAG_PCIIDEX);
          if (!Buffer)
              return STATUS_INSUFFICIENT_RESOURCES;

          RtlCopyMemory(Buffer, IdeCompatibleId, sizeof(IdeCompatibleId));

          Buffer[sizeof(IdeCompatibleId) / sizeof(WCHAR)] = UNICODE_NULL; /* multi-string */

          INFO("Compatible ID: '%S'\n", Buffer);
          break;
      }

      case BusQueryInstanceID:
      {
          CharCount = sizeof("00");

          Buffer = ExAllocatePoolUninitialized(PagedPool,
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

          INFO("Instance ID: '%S'\n", Buffer);
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
    NTSTATUS Status;
    PWCHAR Buffer;
    ULONG Size;
    size_t CharCount;

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextDescription:
        {
            CharCount = sizeof("ATA Channel 99");

            Buffer = ExAllocatePoolUninitialized(PagedPool,
                                                 CharCount * sizeof(WCHAR),
                                                 TAG_PCIIDEX);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            Status = RtlStringCchPrintfExW(Buffer,
                                           CharCount,
                                           NULL,
                                           NULL,
                                           0,
                                           L"ATA Channel %lu",
                                           PdoExtension->Channel);
            ASSERT(NT_SUCCESS(Status));

            INFO("Device Description: '%S'\n", Buffer);
            break;
        }

        case DeviceTextLocationInformation:
        {
            static const WCHAR PrimaryChannelText[] = L"Primary channel";
            static const WCHAR SecondaryChannelText[] = L"Secondary channel";

            if (IS_PRIMARY_CHANNEL(PdoExtension))
                Size = sizeof(PrimaryChannelText);
            else
                Size = sizeof(SecondaryChannelText);

            Buffer = ExAllocatePoolUninitialized(PagedPool, Size, TAG_PCIIDEX);
            if (!Buffer)
                return STATUS_INSUFFICIENT_RESOURCES;

            RtlCopyMemory(Buffer,
                          IS_PRIMARY_CHANNEL(PdoExtension) ?
                          PrimaryChannelText : SecondaryChannelText,
                          Size);

            INFO("Device Location: '%S'\n", Buffer);
            break;
        }

        default:
            return Irp->IoStatus.Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

static
PCM_PARTIAL_RESOURCE_DESCRIPTOR
CODE_SEG("PAGE")
PciIdeXGetInterruptResource(
    _In_ PATA_CHANNEL_DATA ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;

    PAGED_CODE();

    return &Controller->InterruptDesc;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXQueryPciIdeInterface(
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PFDO_DEVICE_EXTENSION FdoExt = PdoExt->Common.FdoExt;
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    PATA_CHANNEL_DATA ChanData;
    PPCIIDE_INTERFACE PciIdeInterface;

    PAGED_CODE();

    if (IoStack->Parameters.QueryInterface.Size < sizeof(*PciIdeInterface))
        return Irp->IoStatus.Status;

    if (IoStack->Parameters.QueryInterface.Version != PCIIDEX_INTERFACE_VERSION)
        return Irp->IoStatus.Status;

    PciIdeInterface = (PPCIIDE_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    ChanData = PdoExt->ChanData;

    RtlCopyMemory(&PciIdeInterface->Regs, &ChanData->Regs, sizeof(ChanData->Regs));

    PciIdeInterface->Controller = Controller;
    PciIdeInterface->ChanData = ChanData;
    PciIdeInterface->Channel = ChanData->Channel;

    PciIdeInterface->MaximumTransferLength = PdoExt->MaximumTransferLength;
    PciIdeInterface->MaximumPhysicalPages = PdoExt->MaximumPhysicalPages;
    PciIdeInterface->TransferModeSupportedBitmap = PdoExt->TransferModeSupportedBitmap;

    PciIdeInterface->HwSyncObject = Controller->HwSyncObject;
    PciIdeInterface->HwSyncContext = PdoExt->Common.Self;

    PciIdeInterface->ChanInfo = ChanData->ChanInfo;

    PciIdeInterface->GetInterruptResource = PciIdeXGetInterruptResource;
    PciIdeInterface->SetTransferMode = AtaCtrlSetTransferMode;

    PciIdeInterface->AdapterDeviceObject = FdoExt->Common.LowerDeviceObject;
    PciIdeInterface->AdapterObject = ChanData->AdapterObject;
    PciIdeInterface->PrdTable = ChanData->PrdTable;
    PciIdeInterface->PrdTablePhysicalAddress = ChanData->PrdTablePhysicalAddress;

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
                           &GUID_PCIIDE_INTERFACE_ROS))
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

    Status = IoAcquireRemoveLock(&PdoExtension->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Status = PciIdeXPdoStartDevice(PdoExtension,
                                           IoStack->Parameters.StartDevice.AllocatedResourcesTranslated);
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
                                            Irp,
                                            IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE,
                                            TRUE);
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
            Status = PciIdeXPnpQueryPnpDeviceState(&PdoExtension->Common, Irp);
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
            Status = PciIdeXPnpQueryDeviceUsageNotification(&PdoExtension->Common, Irp);
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

    IoReleaseRemoveLock(&PdoExtension->Common.RemoveLock, Irp);

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
