/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA channel device object (FDO) dispatch routines
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const WCHAR IdeDevSymLinkFormat[] = L"\\Device\\ScsiPort%lu";
static const WCHAR IdeDosSymLinkFormat[] = L"\\DosDevices\\Scsi%lu:";

static KSYNCHRONIZE_ROUTINE AtaFdoEnableInterruptsSync;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
AtaFdoClaimLegacyAddressRanges(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
    {
        /* On NEC PC-98 systems we have at least four PDOs in use for the legacy IDE interface */
        ConfigInfo->AtDiskPrimaryAddressClaimed =
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

        ChannelExtension->Flags |= CHANNEL_PRIMARY_ADDRESS_CLAIMED |
                                   CHANNEL_SECONDARY_ADDRESS_CLAIMED;
    }
    else
#endif
    if (ChannelExtension->CommandPortBase == (PVOID)0x1F0)
    {
        ConfigInfo->AtDiskPrimaryAddressClaimed = TRUE;

        ChannelExtension->Flags |= CHANNEL_PRIMARY_ADDRESS_CLAIMED;
    }
    else if (ChannelExtension->CommandPortBase == (PVOID)0x170)
    {
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

        ChannelExtension->Flags |= CHANNEL_SECONDARY_ADDRESS_CLAIMED;
    }
}

static
CODE_SEG("PAGE")
VOID
AtaFdoReleaseLegacyAddressRanges(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

    if (ChannelExtension->Flags & CHANNEL_PRIMARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskPrimaryAddressClaimed = FALSE;
    if (ChannelExtension->Flags & CHANNEL_SECONDARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskSecondaryAddressClaimed = FALSE;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoCreateSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    UNICODE_STRING ChannelName;
    ULONG ScsiAdapter, ScsiPortCount;
    WCHAR ChannelNameBuffer[sizeof("\\Device\\Ide\\IdePort999")];
    NTSTATUS Status;
    static const WCHAR IdeChannelFormat[] = L"\\Device\\Ide\\IdePort%lu";

    PAGED_CODE();

    Status = RtlStringCbPrintfW(ChannelNameBuffer,
                                sizeof(ChannelNameBuffer),
                                IdeChannelFormat,
                                ChannelExtension->ChannelNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&ChannelName, ChannelNameBuffer);

    ScsiPortCount = IoGetConfigurationInformation()->ScsiPortCount;

    /* Search for a free SCSI port adapter in the system */
    for (ScsiAdapter = 0; ScsiAdapter <= ScsiPortCount; ++ScsiAdapter)
    {
        WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];
        UNICODE_STRING SymLinkName;

        Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                    sizeof(SymLinkNameBuffer),
                                    IdeDevSymLinkFormat,
                                    ScsiAdapter);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

        /* Create a symbolic link '\Device\ScsiPortX' -> '\Device\Ide\IdePortN' */
        Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
        if (NT_SUCCESS(Status))
        {
            INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);

            Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                        sizeof(SymLinkNameBuffer),
                                        IdeDosSymLinkFormat,
                                        ScsiAdapter);
            ASSERT(NT_SUCCESS(Status));
            RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

            /* Create a symbolic link '\DosDevices\ScsiX:' -> '\Device\Ide\IdePortN' */
            Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
            if (NT_SUCCESS(Status))
            {
                INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);
            }

            /* Register ourselves (ATA channel) as a SCSI port adapter */
            ++IoGetConfigurationInformation()->ScsiPortCount;

            ChannelExtension->PortNumber = ScsiAdapter;
            ChannelExtension->Flags |= CHANNEL_SYMLINK_CREATED;
            break;
        }
    }

    return Status;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoRemoveSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    NTSTATUS Status;
    UNICODE_STRING SymLinkName;
    WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];

    PAGED_CODE();

    if (!(ChannelExtension->Flags & CHANNEL_SYMLINK_CREATED))
        return;

    /* Delete the '\DosDevices\\ScsiX:' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                IdeDevSymLinkFormat,
                                ChannelExtension->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Delete the '\Device\\ScsiPortX' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                IdeDosSymLinkFormat,
                                ChannelExtension->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Unregister the SCSI port adapter */
    --IoGetConfigurationInformation()->ScsiPortCount;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryInterface(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       ChannelExtension->Ldo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.InterfaceType = Guid;
    IoStack->Parameters.QueryInterface.Size = Size;
    IoStack->Parameters.QueryInterface.Version = 1;
    IoStack->Parameters.QueryInterface.Interface = Interface;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(ChannelExtension->Ldo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoParseResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PIDE_REGISTERS Registers = &ChannelExtension->Registers;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    PVOID IoBase;
    ULONG i, Spare;

    PAGED_CODE();

    if (!ResourcesTranslated)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                C_ASSERT(FIELD_OFFSET(CM_PARTIAL_RESOURCE_DESCRIPTOR, u.Port.Length) ==
                         FIELD_OFFSET(CM_PARTIAL_RESOURCE_DESCRIPTOR, u.Memory.Length));

                switch (Descriptor->u.Port.Length)
                {
                    case 1:
                    {
                        /*
                         * Note that on PC-98 systems we still need to support PCI IDE controllers
                         * with a different register layout. The NEC PC-98 legacy IDE interface
                         * can coexist with a normal PCI IDE controller.
                         */
                        if (IsNEC_98 &&
                            ResourcesTranslated->List[0].PartialResourceList.Count >= 12)
                        {
                            if ((Descriptor->u.Port.Start.QuadPart == (ULONG64)0x640))
                            {
                                CommandPortDescriptor = Descriptor;
                                ChannelExtension->Flags |= CHANNEL_CBUS_IDE;
                            }
                            else if ((Descriptor->u.Port.Start.QuadPart == (ULONG64)0x74C))
                            {
                                ControlPortDescriptor = Descriptor;
                                ChannelExtension->Flags |= CHANNEL_CBUS_IDE;
                            }
                        }
                        else if (!ControlPortDescriptor)
                        {
                            ControlPortDescriptor = Descriptor;
                        }
                        break;
                    }

                    case 8:
                    {
                        if (!CommandPortDescriptor)
                            CommandPortDescriptor = Descriptor;
                        break;
                    }

                    default:
                        break;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

    if (!CommandPortDescriptor || !ControlPortDescriptor || !InterruptDescriptor)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    /* Command port base */
    if ((CommandPortDescriptor->Type == CmResourceTypePort) &&
        (CommandPortDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)CommandPortDescriptor->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(ChannelExtension->Flags & CHANNEL_CBUS_IDE));

        IoBase = MmMapIoSpace(CommandPortDescriptor->u.Memory.Start, 8, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChannelExtension->Flags |= CHANNEL_COMMAND_PORT_BASE_MAPPED;
    }
    ChannelExtension->CommandPortBase =
        (PVOID)(ULONG_PTR)CommandPortDescriptor->u.Port.Start.QuadPart;

#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
    {
        Spare = 2;
    }
    else
#endif
    {
        Spare = 1;
    }
    Registers->Data        = IoBase;
    Registers->Error       = (PVOID)((ULONG_PTR)IoBase + 1 * Spare);
    Registers->SectorCount = (PVOID)((ULONG_PTR)IoBase + 2 * Spare);
    Registers->LowLba      = (PVOID)((ULONG_PTR)IoBase + 3 * Spare);
    Registers->MidLba      = (PVOID)((ULONG_PTR)IoBase + 4 * Spare);
    Registers->HighLba     = (PVOID)((ULONG_PTR)IoBase + 5 * Spare);
    Registers->DriveSelect = (PVOID)((ULONG_PTR)IoBase + 6 * Spare);
    Registers->Status      = (PVOID)((ULONG_PTR)IoBase + 7 * Spare);

    /* Control port base */
    if ((ControlPortDescriptor->Type == CmResourceTypePort) &&
        (ControlPortDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)ControlPortDescriptor->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(ChannelExtension->Flags & CHANNEL_CBUS_IDE));

        IoBase = MmMapIoSpace(ControlPortDescriptor->u.Memory.Start, 1, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChannelExtension->Flags |= CHANNEL_CONTROL_PORT_BASE_MAPPED;
    }
    ChannelExtension->ControlPortBase =
        (PVOID)(ULONG_PTR)ControlPortDescriptor->u.Port.Start.QuadPart;

    Registers->Control = (PVOID)IoBase;

    /* Interrupt */
    ChannelExtension->InterruptVector = InterruptDescriptor->u.Interrupt.Vector;
    ChannelExtension->InterruptLevel = InterruptDescriptor->u.Interrupt.Level;
    ChannelExtension->InterruptAffinity = InterruptDescriptor->u.Interrupt.Affinity;

    if (InterruptDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
        ChannelExtension->InterruptMode = Latched;
    else
        ChannelExtension->InterruptMode = LevelSensitive;

    if (InterruptDescriptor->ShareDisposition == CmResourceShareShared)
        ChannelExtension->Flags |= CHANNEL_INTERRUPT_SHARED;
    else
        ChannelExtension->Flags &= ~CHANNEL_INTERRUPT_SHARED;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoFreeResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PAGED_CODE();

    if (ChannelExtension->InterruptObject)
    {
        IoDisconnectInterrupt(ChannelExtension->InterruptObject);
        ChannelExtension->InterruptObject = NULL;
    }

    if (ChannelExtension->Flags & CHANNEL_COMMAND_PORT_BASE_MAPPED)
    {
        ChannelExtension->Flags &= ~CHANNEL_COMMAND_PORT_BASE_MAPPED;

        MmUnmapIoSpace(ChannelExtension->Registers.Data, 8);
    }

    if (ChannelExtension->Flags & CHANNEL_CONTROL_PORT_BASE_MAPPED)
    {
        ChannelExtension->Flags &= ~CHANNEL_CONTROL_PORT_BASE_MAPPED;

        MmUnmapIoSpace(ChannelExtension->Registers.Control, 1);
    }
}

static
BOOLEAN
NTAPI
AtaFdoEnableInterruptsSync(
    _In_ PVOID SynchronizeContext)
{
    PIDE_ENABLE_INTERRUPTS_CONTEXT Context = SynchronizeContext;
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = Context->ChannelExtension;
    UCHAR Command;

    Command = Context->Enable ? IDE_DC_REENABLE_CONTROLLER : IDE_DC_DISABLE_INTERRUPTS;

#if defined(_M_IX86)
    if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
    {
        ATA_SELECT_BANK(1);
        ATA_WRITE(ChannelExtension->Registers.Control, Command);

        ATA_SELECT_BANK(0);
    }
#endif
    ATA_WRITE(ChannelExtension->Registers.Control, Command);

    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoEnableInterrupts(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN Enable)
{
    IDE_ENABLE_INTERRUPTS_CONTEXT SynchronizeContext;

    PAGED_CODE();

    SynchronizeContext.ChannelExtension = ChannelExtension;
    SynchronizeContext.Enable = Enable;

    KeSynchronizeExecution(ChannelExtension->InterruptObject,
                           AtaFdoEnableInterruptsSync,
                           &SynchronizeContext);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoInitializeMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    ATA_SRB_TYPE i;

    PAGED_CODE();

    for (i = 0; i < SRB_TYPE_MAX; ++i)
    {
        Srb = &ChannelExtension->InternalSrb[i];
        Irp = &ChannelExtension->InternalIrp[i];

        Srb->OriginalRequest = Irp;
    }
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    NTSTATUS Status;
    PKSERVICE_ROUTINE IsrHandler;

    PAGED_CODE();

    INFO("Starting channel %lu\n", ChannelExtension->ChannelNumber);

    ChannelExtension->MaximumTransferLength = ATA_MAX_TRANSFER_LENGTH;

    /* Get the interface of the AHCI Port */
    Status = AtaFdoQueryInterface(ChannelExtension,
                                  &GUID_AHCI_PORT_INTERFACE,
                                  &ChannelExtension->AhciPortInterface,
                                  sizeof(ChannelExtension->AhciPortInterface));
    if (NT_SUCCESS(Status))
        ChannelExtension->Flags |= CHANNEL_AHCI;
    else
        ChannelExtension->Flags &= ~CHANNEL_AHCI;

    if (!IS_AHCI(ChannelExtension))
    {
        /* Get the interface of the PCI IDE controller */
        Status = AtaFdoQueryInterface(ChannelExtension,
                                      &GUID_PCIIDE_INTERFACE,
                                      &ChannelExtension->PciIdeInterface,
                                      sizeof(ChannelExtension->PciIdeInterface));
        if (NT_SUCCESS(Status))
        {
            PPCIIDE_INTERFACE PciIdeInterface = &ChannelExtension->PciIdeInterface;

            ChannelExtension->Flags |= CHANNEL_PCI_IDE;
            ChannelExtension->MaximumTransferLength = PciIdeInterface->MaximumTransferLength;
        }
        else
        {
            ChannelExtension->Flags &= ~CHANNEL_PCI_IDE;
        }

        Status = AtaFdoParseResources(ChannelExtension, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to parse resources 0x%lx\n", Status);
            return Status;
        }

        AtaFdoClaimLegacyAddressRanges(ChannelExtension);
    }

    AtaFdoInitializeMemory(ChannelExtension);

    Status = AtaFdoCreateSymLinks(ChannelExtension);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create symbolic links 0x%lx\n", Status);
        return Status;
    }

    /* Reserve memory resources early. Storage drivers should not fail paging I/O operations */
    if (!ChannelExtension->ReservedVaSpace)
    {
        ChannelExtension->ReservedVaSpace =
            MmAllocateMappingAddress(ATA_RESERVED_PAGES * PAGE_SIZE, IDEPORT_TAG);
    }

    /* Initialize the path ID value for legacy IDE channels */
    if (ChannelExtension->PathId == (UCHAR)-1)
    {
        switch ((ULONG_PTR)ChannelExtension->CommandPortBase)
        {
            case 0x1F0: ChannelExtension->PathId = 0; break;
            case 0x170: ChannelExtension->PathId = 1; break;
            case 0x1E8: ChannelExtension->PathId = 2; break;
            case 0x168: ChannelExtension->PathId = 3; break;
            default:
            {
#if defined(_M_IX86)
                if (ChannelExtension->Flags & CHANNEL_CBUS_IDE)
                    ChannelExtension->PathId = 0;
                else
#endif
                    ChannelExtension->PathId = ChannelExtension->ChannelNumber;
                break;
            }
        }
    }

    ChannelExtension->MaximumTransferLength =
        min(ChannelExtension->MaximumTransferLength, ATA_MAX_TRANSFER_LENGTH);

    if (!IS_AHCI(ChannelExtension))
    {
        if (ChannelExtension->Flags & CHANNEL_PCI_IDE)
            IsrHandler = AtaPciIdeChannelIsr;
        else
            IsrHandler = AtaIdeChannelIsr;
        ChannelExtension->ServiceRoutine = IsrHandler;

        Status = IoConnectInterrupt(&ChannelExtension->InterruptObject,
                                    IsrHandler,
                                    ChannelExtension,
                                    NULL,
                                    ChannelExtension->InterruptVector,
                                    ChannelExtension->InterruptLevel,
                                    ChannelExtension->InterruptLevel,
                                    ChannelExtension->InterruptMode,
                                    !!(ChannelExtension->Flags & CHANNEL_INTERRUPT_SHARED),
                                    ChannelExtension->InterruptAffinity,
                                    FALSE);
        if (!NT_SUCCESS(Status))
        {
            ERR("Could not connect to interrupt %lu, status 0x%lx\n",
                ChannelExtension->InterruptVector, Status);

            return Status;
        }

        AtaFdoEnableInterrupts(ChannelExtension, TRUE);
    }

    IoInitializeTimer(ChannelExtension->Common.Self, AtaIoTimer, ChannelExtension);
    IoStartTimer(ChannelExtension->Common.Self);
    ChannelExtension->Flags |= CHANNEL_IO_TIMER_ACTIVE;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoStopDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (!IS_AHCI(ChannelExtension))
    {
        AtaFdoEnableInterrupts(ChannelExtension, FALSE);
    }

    AtaFdoFreeResources(ChannelExtension);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (ChannelExtension->Flags & CHANNEL_IO_TIMER_ACTIVE)
        IoStopTimer(ChannelExtension->Common.Self);

    AtaFdoRemoveSymLinks(ChannelExtension);
    AtaFdoReleaseLegacyAddressRanges(ChannelExtension);
    AtaFdoFreeResources(ChannelExtension);

    if (ChannelExtension->ReservedVaSpace)
    {
        MmFreeMappingAddress(ChannelExtension->ReservedVaSpace, IDEPORT_TAG);
    }

    if (Irp)
    {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChannelExtension->Ldo, Irp);

        IoReleaseRemoveLockAndWait(&ChannelExtension->RemoveLock, Irp);
    }

    IoDetachDevice(ChannelExtension->Ldo);
    IoDeleteDevice(ChannelExtension->Common.Self);

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryPnpDeviceState(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (ChannelExtension->Common.PageFiles ||
        ChannelExtension->Common.HibernateFiles ||
        ChannelExtension->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryDeviceUsageNotification(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (!NT_VERIFY(IoForwardIrpSynchronously(ChannelExtension->Ldo, Irp)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &ChannelExtension->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &ChannelExtension->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &ChannelExtension->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryId(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;

    PAGED_CODE();

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChannelExtension->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            Status = Irp->IoStatus.Status;

            /*
             * We provide a generic identifier necessary to install the device
             * in case our FDO is root-enumerated device.
             */
            if (Status == STATUS_NOT_SUPPORTED)
            {
                static const WCHAR IdeGenericId[] = L"*PNP0600\0";
                PWCHAR Buffer;

                Buffer = ExAllocatePoolUninitialized(PagedPool, sizeof(IdeGenericId), IDEPORT_TAG);
                if (!Buffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory(Buffer, IdeGenericId, sizeof(IdeGenericId));

                Irp->IoStatus.Information = (ULONG_PTR)Buffer;
                Status = STATUS_SUCCESS;
            }

            break;
        }

        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(ChannelExtension->Ldo, Irp);

            IoReleaseRemoveLock(&ChannelExtension->RemoveLock, Irp);

            return Status;
        }
    }

    IoReleaseRemoveLock(&ChannelExtension->RemoveLock, Irp);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    Status = IoAcquireRemoveLock(&ChannelExtension->RemoveLock, Irp);
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
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChannelExtension->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto CompleteIrp;
            }
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
            {
                goto CompleteIrp;
            }

            Status = AtaFdoStartDevice(ChannelExtension,
                                       IoStack->Parameters.
                                       StartDevice.AllocatedResourcesTranslated);
            goto CompleteIrp;
        }

        case IRP_MN_STOP_DEVICE:
            Status = AtaFdoStopDevice(ChannelExtension, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            return AtaFdoRemoveDevice(ChannelExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = AtaFdoQueryPnpDeviceState(ChannelExtension, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = AtaFdoQueryBusRelations(ChannelExtension, Irp);
            if (!NT_SUCCESS(Status))
            {
                goto CompleteIrp;
            }

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaFdoQueryDeviceUsageNotification(ChannelExtension, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            return AtaFdoQueryId(ChannelExtension, Irp, IoStack);

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(ChannelExtension->Ldo, Irp);

    IoReleaseRemoveLock(&ChannelExtension->RemoveLock, Irp);

    return Status;

CompleteIrp:
    IoReleaseRemoveLock(&ChannelExtension->RemoveLock, Irp);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
