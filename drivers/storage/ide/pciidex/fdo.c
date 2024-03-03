/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     ATA controller FDO dispatch routines
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

static
DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaCtrlCallStartController(
    _In_ PATA_CONTROLLER Controller)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Controller->Lock, &OldIrql);
    Controller->Start(Controller);
    KeReleaseSpinLock(&Controller->Lock, OldIrql);
}

static
DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaCtrlCallStopController(
    _In_ PATA_CONTROLLER Controller)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Controller->Lock, &OldIrql);
    Controller->Stop(Controller);
    KeReleaseSpinLock(&Controller->Lock, OldIrql);
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaCtrlStartPciController(
    _In_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;

    PAGED_CODE();

    INFO("Starting controller %04X:%04X.%02X-%04X.%04X-%02X.%02X.%02X\n",
         Controller->Pci.VendorID,
         Controller->Pci.DeviceID,
         Controller->Pci.RevisionID,
         Controller->Pci.SubVendorID,
         Controller->Pci.SubSystemID,
         Controller->Pci.BaseClass,
         Controller->Pci.SubClass,
         Controller->Pci.ProgIf);

    Controller->Flags = 0;
    Controller->MaxChannels = MAX_IDE_CHANNEL;
    Controller->QueueDepth = 1; // PATA_CHANNEL_SLOT
    Controller->AlignmentRequirement = ATA_MIN_BUFFER_ALIGNMENT;
    Controller->AttachChannel = PciIdeAttachChannel;
    Controller->FreeResources = NULL;
    Controller->Start = NULL;
    Controller->Stop = NULL;
    Controller->ChannelEnableBits = NULL;

    if (!(Controller->Pci.Command & PCI_ENABLE_BUS_MASTER))
    {
        INFO("PCI bus mastering disabled\n");
        Controller->Flags |= CTRL_FLAG_PIO_ONLY;
    }

    Status = AhciGetControllerProperties(Controller);
    if (Status == STATUS_NO_MATCH)
    {
        Status = PciIdeGetControllerProperties(Controller);
    }
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to match ATA controller with status %lx\n", Status);
        return Status;
    }

    if (Controller->Pci.BaseClass == PCI_CLASS_MASS_STORAGE_CTLR)
    {
        if ((Controller->Pci.SubClass != PCI_SUBCLASS_MSC_AHCI_CTLR) &&
            !(Controller->Pci.ProgIf & PCIIDE_PROGIF_DMA_CAPABLE))
        {
            INFO("Non DMA capable controller detected\n");
            Controller->Flags |= CTRL_FLAG_PIO_ONLY;
        }

        if (Controller->Pci.SubClass == PCI_SUBCLASS_MSC_IDE_CTLR)
        {
            /*
             * Check for a legacy PCI IDE device.
             * NOTE: NT architecture does not support
             * switching only one IDE channel to native mode.
             */
            if (!(Controller->Pci.ProgIf & PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE) ||
                !(Controller->Pci.ProgIf & PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE))
            {
                Controller->Flags |= CTRL_FLAG_IN_LEGACY_MODE;
            }
        }
    }

    if (Controller->Flags & CTRL_FLAG_IS_AHCI)
    {
        if (Controller->InterruptDesc.Type != CmResourceTypeInterrupt)
        {
            ERR("No interrupt resource\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        if (!(Controller->Flags & CTRL_FLAG_IN_LEGACY_MODE))
        {
            if (Controller->InterruptDesc.Type != CmResourceTypeInterrupt)
            {
                ERR("No interrupt resource\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (Controller->Start)
        AtaCtrlCallStartController(Controller);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaCtrlAttachChannel(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Attach)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;

    PAGED_CODE();

    return Controller->AttachChannel(ChanData, Attach);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaChanEnableInterruptsSync(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Enable)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;
    PKINTERRUPT InterruptObject;
    KIRQL OldIrql;

    if (Controller->Flags & CTRL_FLAG_IS_AHCI)
        InterruptObject = Controller->InterruptObject;
    else
        InterruptObject = ChanData->InterruptObject;
    ASSERT(InterruptObject);

    OldIrql = KeAcquireInterruptSpinLock(InterruptObject);
    ChanData->EnableInterrupts(ChanData, Enable);
    KeReleaseInterruptSpinLock(InterruptObject, OldIrql);
}

VOID
AtaCtrlSetTransferMode(
    _In_ PVOID ChannelContext,
    _In_reads_(ATA_MAX_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;
    KIRQL OldIrql;
    ULONG i;
    BOOLEAN DiscoveredNewDevice = FALSE;

    for (i = 0; i < ATA_MAX_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];

        if (!Device)
            continue;

        if (Device->IsNewDevice)
            DiscoveredNewDevice = TRUE;

        /* Apply the channel limit */
        Device->SupportedModes &= ChanData->Actual.TransferModeSupported;

        if ((ChanData->ChanInfo & CHANNEL_FLAG_NO_ATAPI_DMA) && !Device->IsFixedDisk)
            Device->SupportedModes &= PIO_ALL;

        /*
         * If we are booting in minimal safe mode we disable DMA for ATAPI devices
         * to improve system stability.
         */
        if (InitSafeBootMode == 1)
        {
            if (!(ChanData->ChanInfo & CHANNEL_FLAG_PIO_VIA_DMA) && !Device->IsFixedDisk)
                Device->SupportedModes &= PIO_ALL;
        }

        /* Set initial values for mode selection: Find the fastest supported PIO and DMA mode */
        if (!_BitScanReverse(&Device->DmaMode, Device->SupportedModes & ~PIO_ALL))
            Device->DmaMode = PIO_MODE(0);
        NT_VERIFY(_BitScanReverse(&Device->PioMode, Device->SupportedModes & PIO_ALL));
    }

    KeAcquireSpinLock(&Controller->Lock, &OldIrql);

    /*
     * _GTF should be executed after _STM has been evaluated,
     * because it is expected that ACPI BIOS will use the identity data buffers
     * to construct the list of ATA commands to the drive.
     *
     * Therefore for any new device we have to evaluate a dummy _STM
     * when the whole configuration of the channel's transfer timings
     * is done at the controller minidriver.
     */
    if (DiscoveredNewDevice && (ChanData->ChanInfo & CHANNEL_FLAG_HAS_ACPI_GTM))
    {
        PCHANNEL_DATA_PATA PataData = ChannelContext;

        /* Evaluate _STM */
        AtaAcpiSetTimingMode(PataData->PdoExt->Common.Self,
                             &PataData->CurrentTimingMode,
                             DeviceList[0] ? DeviceList[0]->IdentifyDeviceData : NULL,
                             DeviceList[1] ? DeviceList[1]->IdentifyDeviceData : NULL);
    }

    /* Set the PATA transfer timings */
    ChanData->SetTransferMode(Controller, ChanData->Channel, DeviceList);

    KeReleaseSpinLock(&Controller->Lock, OldIrql);
}

CODE_SEG("PAGE")
VOID
AtaCtrlSetDeviceData(
    _In_ PVOID ChannelContext,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyDeviceData)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;

    PAGED_CODE();

    if (!(Controller->Flags & CTRL_FLAG_SATA_HBA_ACPI))
        return;

    AtaAcpiSetDeviceData(DeviceObject, IdentifyDeviceData);
}

CODE_SEG("PAGE")
PVOID
AtaCtrlGetInitTaskFile(
    _In_ PVOID ChannelContext,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;

    PAGED_CODE();

    if ((Controller->Flags & CTRL_FLAG_SATA_HBA_ACPI) ||
        (ChanData->ChanInfo & CHANNEL_FLAG_HAS_ACPI_GTM))
    {
        return AtaAcpiGetTaskFile(DeviceObject);
    }

    return NULL;
}

BOOLEAN
AtaCtrlDowngradeInterfaceSpeed(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;
    PATA_CONTROLLER Controller = ChanData->Controller;

    PAGED_CODE();

    if (Controller->Flags & CTRL_FLAG_IS_AHCI)
        return AtaAhciDowngradeInterfaceSpeed(ChannelContext);

    /* Optionally, implement a SATA framework at some point in the future */
    return FALSE;
}

VOID
AtaCtrlAbortChannel(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN DisableInterrupts)
{
    PCHANNEL_DATA_COMMON ChanData = ChannelContext;

    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);

    ChanData->ActiveSlotsBitmap = 0;
    ChanData->ActiveQueuedSlotsBitmap = 0;

    /* Disable and clear pending interrupts */
    if (DisableInterrupts)
    {
        ChanData->EnableInterrupts(ChanData, FALSE);

#if DBG
        if (ChanData->Controller->Flags & CTRL_FLAG_IS_AHCI)
        {
            PVOID IoBase = ((PCHANNEL_DATA_AHCI)ChanData)->IoBase;

            DbgPrint("PxIS     0x%08lX\n", AHCI_PORT_READ(IoBase, PxInterruptStatus));
            DbgPrint("PxIE     0x%08lX\n", AHCI_PORT_READ(IoBase, PxInterruptEnable));
            DbgPrint("PxCMD    0x%08lX\n", AHCI_PORT_READ(IoBase, PxCmdStatus));
            DbgPrint("PxTFD    0x%08lX\n", AHCI_PORT_READ(IoBase, PxTaskFileData));
            DbgPrint("PxSIG    0x%08lX\n", AHCI_PORT_READ(IoBase, PxSignature));
            DbgPrint("PxSSTS   0x%08lX\n", AHCI_PORT_READ(IoBase, PxSataStatus));
            DbgPrint("PxSCTL   0x%08lX\n", AHCI_PORT_READ(IoBase, PxSataControl));
            DbgPrint("PxSERR   0x%08lX\n", AHCI_PORT_READ(IoBase, PxSataError));
            DbgPrint("PxSACT   0x%08lX\n", AHCI_PORT_READ(IoBase, PxSataActive));
            DbgPrint("PxCI     0x%08lX\n", AHCI_PORT_READ(IoBase, PxCommandIssue));
            DbgPrint("PxSNTF   0x%08lX\n", AHCI_PORT_READ(IoBase, PxSataNotification));
            DbgPrint("PxFBS    0x%08lX\n", AHCI_PORT_READ(IoBase, PxFisSwitchingControl));
            DbgPrint("PxDEVSLP 0x%08lX\n", AHCI_PORT_READ(IoBase, PxDeviceSleep));
        }
#endif
    }
}

CODE_SEG("PAGE")
PVOID
AtaCtrlPciGetBar(
    _In_ PATA_CONTROLLER Controller,
    _In_range_(0, PCI_TYPE0_ADDRESSES) ULONG Index,
    _In_ ULONG MinimumIoLength)
{
    PAGED_CODE();

    if (!(Controller->AccessRange[Index].Flags & RANGE_IS_VALID))
        return NULL;

    /* Validate the BAR length */
    if (MinimumIoLength != 0)
    {
        if (Controller->AccessRange[Index].Length < MinimumIoLength)
        {
            ERR("%04X:%04X.%02X: Unexpected PCI BAR #%lu length 0x%lx, minimum required 0x%lx\n",
                Controller->Pci.VendorID,
                Controller->Pci.DeviceID,
                Controller->Pci.RevisionID,
                Index,
                Controller->AccessRange[Index].Length,
                MinimumIoLength);
            return NULL;
        }
    }

    if ((Controller->AccessRange[Index].Flags & RANGE_IS_MEMORY) &&
        !(Controller->AccessRange[Index].Flags & RANGE_IS_MAPPED))
    {
        PHYSICAL_ADDRESS PhysicalAddress;

        Controller->AccessRange[Index].Flags |= RANGE_IS_MAPPED;

        PhysicalAddress.QuadPart = (ULONG_PTR)Controller->AccessRange[Index].IoBase;
        Controller->AccessRange[Index].IoBase = MmMapIoSpace(PhysicalAddress,
                                                             Controller->AccessRange[Index].Length,
                                                             MmNonCached);
    }

    return Controller->AccessRange[Index].IoBase;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaCtrlPciGetNextBarIndex(
    _In_ PPCI_COMMON_HEADER PciData,
    _Inout_ PULONG Bar)
{
    PAGED_CODE();

    for (; *Bar < PCI_TYPE0_ADDRESSES; (*Bar)++)
    {
        ULONG IoBase = PciData->u.type0.BaseAddresses[*Bar];

        if (IoBase & PCI_ADDRESS_IO_SPACE)
            IoBase &= ~PCI_ADDRESS_IO_SPACE;
        else
            IoBase &= ~(PCI_ADDRESS_MEMORY_TYPE_MASK | PCI_ADDRESS_MEMORY_PREFETCHABLE);

        /* Look for a valid BAR */
        if (IoBase == 0)
            continue;

        return TRUE;
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
VOID
AtaCtrlPciAssignResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated,
    _In_ PPCI_COMMON_HEADER PciData)
{
    ULONG i, Bar;

    PAGED_CODE();

    if (!ResourcesTranslated)
        return;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc;

        Desc = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        if (Desc->Type == CmResourceTypeInterrupt)
        {
            RtlCopyMemory(&Controller->InterruptDesc, Desc, sizeof(*Desc));
            break;
        }
    }

    for (i = 0, Bar = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc;
        ULONG CurrBar, NextBar;

        Desc = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        if ((Desc->Type != CmResourceTypePort) && (Desc->Type != CmResourceTypeMemory))
            continue;

        if (!AtaCtrlPciGetNextBarIndex(PciData, &Bar))
            break;

        CurrBar = PciData->u.type0.BaseAddresses[Bar];
        if (Bar < (PCI_TYPE0_ADDRESSES - 1))
            NextBar = PciData->u.type0.BaseAddresses[Bar + 1];
        else
            NextBar = 0;

        if (CurrBar & PCI_ADDRESS_IO_SPACE)
        {
            if (Desc->Type != CmResourceTypePort)
                continue;

            if (Desc->u.Port.Start.QuadPart != (CurrBar & PCI_ADDRESS_IO_ADDRESS_MASK))
                continue;

            INFO("BAR[%lu]: I/O %I64X Len %lX\n",
                 Bar, Desc->u.Port.Start.QuadPart, Desc->u.Port.Length);
            Controller->AccessRange[Bar].Flags |= RANGE_IS_VALID;
            Controller->AccessRange[Bar].IoBase = (PVOID)(ULONG_PTR)Desc->u.Port.Start.QuadPart;
            Controller->AccessRange[Bar].Length = Desc->u.Port.Length;
        }
        else
        {
            if (Desc->Type != CmResourceTypeMemory)
                continue;

            if (Desc->u.Memory.Start.LowPart != (CurrBar & PCI_ADDRESS_MEMORY_ADDRESS_MASK))
                continue;

            if ((CurrBar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
            {
                if (Desc->u.Memory.Start.HighPart != NextBar)
                    continue;
            }
            else
            {
                if (Desc->u.Memory.Start.HighPart != 0)
                    continue;
            }

            INFO("BAR[%lu]: MEM %I64X Len %lX\n",
                 Bar, Desc->u.Memory.Start.QuadPart, Desc->u.Memory.Length);
            Controller->AccessRange[Bar].Flags |= RANGE_IS_VALID | RANGE_IS_MEMORY;
            Controller->AccessRange[Bar].IoBase = (PVOID)(ULONG_PTR)Desc->u.Memory.Start.QuadPart;
            Controller->AccessRange[Bar].Length = Desc->u.Memory.Length;
        }

        ++Bar;
    }
}

static
CODE_SEG("PAGE")
VOID
AtaCtrlPciSaveData(
    _Inout_ ATA_CONTROLLER* __restrict Controller,
    _In_ PCI_COMMON_HEADER* __restrict PciData)
{
    PAGED_CODE();

    Controller->Pci.VendorID = PciData->VendorID;
    Controller->Pci.DeviceID = PciData->DeviceID;
    Controller->Pci.Command = PciData->Command;
    Controller->Pci.RevisionID = PciData->RevisionID;
    Controller->Pci.ProgIf = PciData->ProgIf;
    Controller->Pci.SubClass = PciData->SubClass;
    Controller->Pci.BaseClass = PciData->BaseClass;
    Controller->Pci.CacheLineSize = PciData->CacheLineSize;
    Controller->Pci.SubVendorID = PciData->u.type0.SubVendorID;
    Controller->Pci.SubSystemID = PciData->u.type0.SubSystemID;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaCtrlPciCollectInformation(
    _In_ PFDO_DEVICE_EXTENSION FdoExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, u.type0.SubSystemID)];
    PPCI_COMMON_HEADER PciData = (PPCI_COMMON_HEADER)Buffer; // Partial PCI header
    ULONG BytesRead;

    PAGED_CODE();

    BytesRead = (*Controller->BusInterface.GetBusData)(Controller->BusInterface.Context,
                                                       PCI_WHICHSPACE_CONFIG,
                                                       Buffer,
                                                       0,
                                                       sizeof(Buffer));
    if (BytesRead != sizeof(Buffer))
        return STATUS_IO_DEVICE_ERROR;

    AtaCtrlPciSaveData(Controller, PciData);
    AtaCtrlPciAssignResources(Controller, ResourcesTranslated, PciData);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStartDevice(
    _In_ PVOID ControllerContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PFDO_DEVICE_EXTENSION FdoExt = ControllerContext;
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    NTSTATUS Status;

    PAGED_CODE();

    if (Controller->Flags & CTRL_FLAG_NON_PNP)
    {
        // FIXME:
    }
    else
    {
        Status = AtaCtrlPciCollectInformation(FdoExt, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to collect PCI information 0x%lx\n", Status);
            return Status;
        }

        Status = AtaCtrlStartPciController(&FdoExt->Controller);
        if (!NT_SUCCESS(Status))
        {
            ERR("Miniport initialization failed 0x%lx\n", Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
PciIdeXFdoFreeResources(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PATA_CONTROLLER Controller = &FdoExtension->Controller;
    ULONG i;

    PAGED_CODE();

    if (Controller->Stop)
        AtaCtrlCallStopController(Controller);

    if (Controller->FreeResources)
        Controller->FreeResources(Controller);

    Controller->InterruptObject = NULL;

    for (i = 0; i < RTL_NUMBER_OF(Controller->AccessRange); ++i)
    {
        if ((Controller->AccessRange[i].Flags & RANGE_IS_VALID) &&
            (Controller->AccessRange[i].Flags & RANGE_IS_MAPPED))
        {
            MmUnmapIoSpace(Controller->AccessRange[i].IoBase, Controller->AccessRange[i].Length);
        }

        Controller->AccessRange[i].Flags = 0;
    }

    if (Controller->ChanDataBlock)
    {
        ExFreePoolWithTag(Controller->ChanDataBlock, TAG_PCIIDEX);
        Controller->ChanDataBlock = NULL;
    }

    if (Controller->HwSyncObject)
    {
        IoDeleteController(Controller->HwSyncObject);
        Controller->HwSyncObject = NULL;
    }

    if (Controller->HwExt)
    {
        ExFreePoolWithTag(Controller->HwExt, TAG_PCIIDEX);
        Controller->HwExt = NULL;
    }

    Controller->InterruptDesc.Type = 0;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStopDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    PciIdeXFdoFreeResources(FdoExtension);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoRemoveDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;

    PAGED_CODE();

    IoReleaseRemoveLockAndWait(&FdoExtension->Common.RemoveLock, Irp);

    ExAcquireFastMutex(&FdoExtension->PdoListSyncMutex);

    for (ListEntry = FdoExtension->PdoListHead.Flink;
         ListEntry != &FdoExtension->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);

        PdoExt->Flags |= PDO_FLAG_REPORTED_MISSING;
        PciIdeXPdoRemoveDevice(PdoExt, Irp, TRUE, FALSE);
    }

    ExReleaseFastMutex(&FdoExtension->PdoListSyncMutex);

    PciIdeXFdoFreeResources(FdoExtension);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(FdoExtension->Common.LowerDeviceObject, Irp);

    IoDetachDevice(FdoExtension->Common.LowerDeviceObject);
    IoDeleteDevice(FdoExtension->Common.Self);

    return Status;
}

static
CODE_SEG("PAGE")
PPDO_DEVICE_EXTENSION
PciIdeXPdoCreateDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG ChannelNumber)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\PciIde999Channel99-FFF")];
    PDEVICE_OBJECT Pdo;
    PPDO_DEVICE_EXTENSION PdoExtension;
    ULONG Alignment;
    static ULONG PdoNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\PciIde%luChannel%lu-%lx",
                                FdoExtension->ControllerNumber,
                                ChannelNumber,
                                PdoNumber++);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(FdoExtension->Common.Self->DriverObject,
                            sizeof(*PdoExtension),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create PDO 0x%lx\n", Status);
        return NULL;
    }

    INFO("Created device object %p '%wZ'\n", Pdo, &DeviceName);

    /* DMA buffers alignment */
    Alignment = FdoExtension->Controller.AlignmentRequirement;
    Alignment = max(Alignment, FdoExtension->Common.Self->AlignmentRequirement);
    Alignment = max(Alignment, ATA_MIN_BUFFER_ALIGNMENT);
    Pdo->AlignmentRequirement = Alignment;

    PdoExtension = Pdo->DeviceExtension;

    RtlZeroMemory(PdoExtension, sizeof(*PdoExtension));
    PdoExtension->Common.Self = Pdo;
    PdoExtension->Common.FdoExt = FdoExtension;
    IoInitializeRemoveLock(&PdoExtension->Common.RemoveLock, TAG_PCIIDEX, 0, 0);

    PdoExtension->Channel = ChannelNumber;
    PdoExtension->ChanData = FdoExtension->Controller.Channels[ChannelNumber];
    ASSERT(PdoExtension->ChanData);
    PdoExtension->ChanData->PdoExt = PdoExtension;

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return PdoExtension;
}

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    PAGED_CODE();

    if (Controller->Flags & CTRL_FLAG_IS_AHCI)
        return ChannelStateUnknown;

    return PciIdeGetChannelState(Controller, Channel);
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryBusRelations(
    _In_ PFDO_DEVICE_EXTENSION FdoExt,
    _In_ PIRP Irp)
{
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    PDEVICE_RELATIONS DeviceRelations;
    PLIST_ENTRY ListEntry;
    ULONG i, Size, ChannelBitmap = 0;

    PAGED_CODE();

    /*
     * Preallocate the device relations structure we need for the QBR IRP,
     * because we don't want to be in a state where allocation fails.
     */
    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects[MAX_CHANNELS]);
    DeviceRelations = ExAllocatePoolUninitialized(PagedPool, Size, TAG_PCIIDEX);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    ExAcquireFastMutex(&FdoExt->PdoListSyncMutex);

    /* Check existing ports */
    for (ListEntry = FdoExt->PdoListHead.Flink;
         ListEntry != &FdoExt->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;
        IDE_CHANNEL_STATE ChannelState;
        ULONG Channel;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);
        Channel = PdoExt->Channel;

        ChannelBitmap |= 1 << Channel;

        ChannelState = PciIdeXGetChannelState(Controller, Channel);
        if (ChannelState == ChannelDisabled)
        {
            INFO("%04X:%04X.%02X: Skip disabled channel %lu\n",
                 Controller->Pci.VendorID,
                 Controller->Pci.DeviceID,
                 Controller->Pci.RevisionID,
                 Channel);

            PdoExt->Flags |= PDO_FLAG_NOT_PRESENT;
            continue;
        }
    }

    /* Enumerate the remaining ports */
    ChannelBitmap = Controller->ChannelBitmap & ~ChannelBitmap;
    for (i = 0; i < MAX_CHANNELS; ++i)
    {
        PPDO_DEVICE_EXTENSION PdoExt;
        IDE_CHANNEL_STATE ChannelState;

        if (!(ChannelBitmap & (1 << i)))
            continue;

        ChannelState = PciIdeXGetChannelState(Controller, i);
        if (ChannelState == ChannelDisabled)
        {
            INFO("%04X:%04X.%02X: Skip disabled channel %lu\n",
                 Controller->Pci.VendorID,
                 Controller->Pci.DeviceID,
                 Controller->Pci.RevisionID,
                 i);
            continue;
        }

        /* Need to create a PDO */
        PdoExt = PciIdeXPdoCreateDevice(FdoExt, i);
        if (!PdoExt)
        {
            /* We are out of memory, trying to continue process the QBR IRP anyway */
            continue;
        }

        InsertTailList(&FdoExt->PdoListHead, &PdoExt->ListEntry);
    }

    /* Initialize the device relations structure */
    DeviceRelations->Count = 0;
    for (ListEntry = FdoExt->PdoListHead.Flink;
         ListEntry != &FdoExt->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);
        if (PdoExt->Flags & PDO_FLAG_NOT_PRESENT)
        {
            PdoExt->Flags |= PDO_FLAG_REPORTED_MISSING;
            continue;
        }

        DeviceRelations->Objects[DeviceRelations->Count++] = PdoExt->Common.Self;
        ObReferenceObject(PdoExt->Common.Self);
    }

    ExReleaseFastMutex(&FdoExt->PdoListSyncMutex);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryInterface(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PAGED_CODE();

    if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                           &GUID_TRANSLATOR_INTERFACE_STANDARD))
    {
        PTRANSLATOR_INTERFACE TranslatorInterface;
        CM_RESOURCE_TYPE ResourceType;
        ULONG BusNumber;

        /* In native mode the IDE controller does not use any legacy interrupt resources */
        if (!(FdoExtension->Controller.Flags & CTRL_FLAG_IN_LEGACY_MODE))
            return STATUS_NOT_SUPPORTED;

        if (IoStack->Parameters.QueryInterface.Size < sizeof(*TranslatorInterface))
            return STATUS_NOT_SUPPORTED;

        ResourceType = PtrToUlong(IoStack->Parameters.QueryInterface.InterfaceSpecificData);
        if (ResourceType != CmResourceTypeInterrupt)
            return STATUS_NOT_SUPPORTED;

        TranslatorInterface = (PTRANSLATOR_INTERFACE)IoStack->Parameters.QueryInterface.Interface;

        return HalGetInterruptTranslator(PCIBus,
                                         0,
                                         InterfaceTypeUndefined,
                                         sizeof(*TranslatorInterface),
                                         IoStack->Parameters.QueryInterface.Version,
                                         TranslatorInterface,
                                         &BusNumber);
    }

    return STATUS_NOT_SUPPORTED;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoAcquireRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
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
            if (!NT_VERIFY(IoForwardIrpSynchronously(FdoExtension->Common.LowerDeviceObject, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto CompleteIrp;
            }
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Status = PciIdeXFdoStartDevice(FdoExtension,
                                           IoStack->Parameters.
                                           StartDevice.AllocatedResourcesTranslated);
            goto CompleteIrp;
        }

        case IRP_MN_STOP_DEVICE:
        {
            Status = PciIdeXFdoStopDevice(FdoExtension);
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
            return PciIdeXFdoRemoveDevice(FdoExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            Status = PciIdeXPnpQueryPnpDeviceState(&FdoExtension->Common, Irp);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = PciIdeXFdoQueryBusRelations(FdoExtension, Irp);
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            Status = PciIdeXPnpQueryDeviceUsageNotification(&FdoExtension->Common, Irp);
            break;
        }

        case IRP_MN_QUERY_INTERFACE:
        {
            Status = PciIdeXFdoQueryInterface(FdoExtension, IoStack);
            if (Status == STATUS_NOT_SUPPORTED)
                break;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(FdoExtension->Common.LowerDeviceObject, Irp);

    IoReleaseRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
    return Status;

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
    return Status;
}
