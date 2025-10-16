/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Generic PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

VOID
PciIdeSataSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList)
{
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(DeviceList);

    /* Keep the selected PioMode and DmaMode, nothing to do for SATA hardware */
    NOTHING;
}

static
VOID
PciIdeSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList)
{
    PATA_CHANNEL_DATA ChanData = Controller->Channels[Channel];
    ULONG i;

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_DATA Device = DeviceList[i];
        ULONG DmaFlags;

        if (!Device)
            continue;

        /* Get the PIO and DMA modes set by BIOS */
        if (!_BitScanReverse(&Device->DmaMode, Device->CurrentModes & ~PIO_ALL))
            Device->DmaMode = 0;
        NT_VERIFY(_BitScanReverse(&Device->PioMode, Device->CurrentModes & PIO_ALL));

        if (i == 0)
            DmaFlags = ATA_CHANNEL_FLAG_DRIVE0_DMA_CAPABLE;
        else
            DmaFlags = ATA_CHANNEL_FLAG_DRIVE1_DMA_CAPABLE;

        /* Disable DMA when when it is not supported */
        if (!(ChanData->ChanInfo & DmaFlags))
            Device->DmaMode = 0;
    }
}

static
CODE_SEG("PAGE")
VOID
PciIdeFreeDmaResources(
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PATA_CHANNEL_DATA ChanData)
{
    PDMA_OPERATIONS DmaOperations;
    PDMA_ADAPTER AdapterObject;

    PAGED_CODE();

    AdapterObject = ChanData->AdapterObject;
    if (!AdapterObject)
        return;

    DmaOperations = AdapterObject->DmaOperations;

    if (ChanData->PrdTable)
    {
        PHYSICAL_ADDRESS PrdTablePhysicalAddress;

        PrdTablePhysicalAddress.QuadPart = ChanData->PrdTablePhysicalAddress;

        DmaOperations->FreeCommonBuffer(AdapterObject,
                                        sizeof(*ChanData->PrdTable) * PdoExt->MaximumPhysicalPages,
                                        PrdTablePhysicalAddress,
                                        ChanData->PrdTable,
                                        TRUE); // Cached
        ChanData->PrdTable = NULL;
    }

    DmaOperations->PutDmaAdapter(AdapterObject);

    ChanData->AdapterObject = NULL;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeAllocatePrdTable(
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PATA_CHANNEL_DATA ChanData)
{
    PDMA_OPERATIONS DmaOperations = ChanData->AdapterObject->DmaOperations;
    PHYSICAL_ADDRESS LogicalAddress;
    ULONG BlockSize;

    PAGED_CODE();

    BlockSize = sizeof(*ChanData->PrdTable) * PdoExt->MaximumPhysicalPages;

    ChanData->PrdTable = DmaOperations->AllocateCommonBuffer(ChanData->AdapterObject,
                                                             BlockSize,
                                                             &LogicalAddress,
                                                             TRUE); // Cached
    if (!ChanData->PrdTable)
        return FALSE;
    RtlZeroMemory(ChanData->PrdTable, BlockSize);

    ChanData->PrdTablePhysicalAddress = LogicalAddress.LowPart;

    /* 32-bit DMA */
    ASSERT(LogicalAddress.HighPart == 0);

    /* The descriptor table must be 4 byte aligned */
    ASSERT((LogicalAddress.LowPart % sizeof(ULONG)) == 0);

    return TRUE;
}

static
CODE_SEG("PAGE")
PDMA_ADAPTER
PciIdeGetDmaAdapter(
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PATA_CHANNEL_DATA ChanData)
{
    PFDO_DEVICE_EXTENSION FdoExt = PdoExt->Common.FdoExt;
    DEVICE_DESCRIPTION DeviceDescription = { 0 };

    PAGED_CODE();

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = ChanData->MaximumTransferLength;

    return IoGetDmaAdapter(FdoExt->Common.LowerDeviceObject,
                           &DeviceDescription,
                           &PdoExt->MaximumPhysicalPages);
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeInitDma(
    _In_ PATA_CONTROLLER Controller,
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PATA_CHANNEL_DATA ChanData)
{
    PUCHAR IoBase;
    UCHAR DmaStatus;

    PAGED_CODE();

    /* Check if BAR4 is available */
    IoBase = AtaCtrlPciGetBar(Controller, PCIIDE_DMA_IO_BAR);
    if (!IoBase)
    {
        INFO("CH %lu: No bus master registers", ChanData->Channel);
        return FALSE;
    }

    if (!IS_PRIMARY_CHANNEL(ChanData))
        IoBase += PCIIDE_DMA_SECONDARY_CHANNEL_OFFSET;

    /*
     * Save the DMA I/O address first. Some PATA controllers (Intel ICH)
     * assert the DMA interrupt even if the current command is a PIO command,
     * so we have to always clear the DMA interrupt.
     */
    ChanData->Regs.Dma = IoBase;

    /* This channel does not support DMA */
    if (!(ChanData->TransferModeSupportedBitmap & ~PIO_ALL))
        return FALSE;

    /* Verify DMA status */
    DmaStatus = READ_PORT_UCHAR(IoBase + PCIIDE_DMA_STATUS);
    INFO("CH %lu: I/O Base %p, status 0x%02X\n", ChanData->Channel, IoBase, DmaStatus);

    /* The status bits 3:4 must return 0 on reads */
    if (DmaStatus & (PCIIDE_DMA_STATUS_RESERVED1 | PCIIDE_DMA_STATUS_RESERVED2))
    {
        WARN("CH %lu: Unexpected DMA status 0x%02X\n", ChanData->Channel, DmaStatus);
        return FALSE;
    }

    /* The status bits 5:6 are set by the BIOS firmware at boot */
    if (DmaStatus & PCIIDE_DMA_STATUS_DRIVE0_DMA_CAPABLE)
        ChanData->ChanInfo |= ATA_CHANNEL_FLAG_DRIVE0_DMA_CAPABLE;
    if (DmaStatus & PCIIDE_DMA_STATUS_DRIVE1_DMA_CAPABLE)
        ChanData->ChanInfo |= ATA_CHANNEL_FLAG_DRIVE1_DMA_CAPABLE;

    ChanData->AdapterObject = PciIdeGetDmaAdapter(PdoExt, ChanData);
    if (!ChanData->AdapterObject)
    {
        WARN("CH %lu: Unable to get DMA adapter\n", ChanData->Channel);
        return FALSE;
    }

    if (!PciIdeAllocatePrdTable(PdoExt, ChanData))
    {
        WARN("CH %lu: Unable to allocate PRD table\n", ChanData->Channel);
        PciIdeFreeDmaResources(PdoExt, ChanData);
        return FALSE;
    }

    /* See what transfer length we actually got */
    PdoExt->MaximumTransferLength = min(ChanData->MaximumTransferLength,
                                        PdoExt->MaximumPhysicalPages << PAGE_SHIFT);
    INFO("CH %lu: Allocated %lu PRD pages, maximum xfer length 0x%lx\n",
         ChanData->Channel,
         PdoExt->MaximumPhysicalPages,
         PdoExt->MaximumTransferLength);

    /* The hardware can use DMA */
    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
PciIdeInitTaskFileIoResources(
    _In_ PIDE_REGISTERS Registers,
    _In_ ULONG_PTR CommandPortBase,
    _In_ ULONG_PTR ControlPortBase)
{
    PAGED_CODE();

    /* Standard PATA register layout */
    Registers->Data        = (PVOID)(CommandPortBase + 0);
    Registers->Error       = (PVOID)(CommandPortBase + 1);
    Registers->SectorCount = (PVOID)(CommandPortBase + 2);
    Registers->LbaLow      = (PVOID)(CommandPortBase + 3);
    Registers->LbaMid      = (PVOID)(CommandPortBase + 4);
    Registers->LbaHigh     = (PVOID)(CommandPortBase + 5);
    Registers->Device      = (PVOID)(CommandPortBase + 6);
    Registers->Status      = (PVOID)(CommandPortBase + 7);

    Registers->Control     = (PVOID)ControlPortBase;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeAssignNativeResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PATA_CHANNEL_DATA ChanData)
{
    const ULONG BarIndex = ChanData->Channel * 2;
    PVOID CommandPortBase, ControlPortBase;

    PAGED_CODE();

    CommandPortBase = AtaCtrlPciGetBar(Controller, BarIndex);
    if (!CommandPortBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    ControlPortBase = AtaCtrlPciGetBar(Controller, BarIndex + 1);
    if (!ControlPortBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    PciIdeInitTaskFileIoResources(&ChanData->Regs,
                                  (ULONG_PTR)CommandPortBase,
                                  (ULONG_PTR)CommandPortBase + PCIIDE_CONTROL_IO_BAR_OFFSET);

    return STATUS_SUCCESS;
}

/*
 * PCI IDE Notes:
 *
 * When the PCI IDE controller operates in compatibility mode,
 * the pciidex driver assigns a list of boot resources to each IDE channel.
 *
 *                                        _GTM, _STM                            _GTF
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFA0, Len 10      |   IO:  Start 0:1F0, Len 8        |
 *                                       |   IO:  Start 0:3F6, Len 1        \-->DRV1
 *                                       |   INT: Lev A Vec A Aff FFFFFFFF
 *                                       |
 *                                       \-->CHN1---------------------------+-->DRV0
 *                                           IO:  Start 0:170, Len 8        |
 *                                           IO:  Start 0:376, Len 1        \-->DRV1
 *                                           INT: Lvl F Vec F Aff FFFFFFFF
 *
 * In native-PCI mode, the PCI IDE controller acts as a true PCI device
 * and no resources are assigned to the channel device object at all.
 *
 *                                        _GTM, _STM                            _GTF
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFC0, Len 8       |                                  |
 *        IO:  Start 0:FF8C, Len 4       |                                  \-->DRV1
 *        IO:  Start 0:FF80, Len 8       |
 *        IO:  Start 0:FF88, Len 4       |
 *        IO:  Start 0:FFA0, Len 10      \-->CHN1---------------------------+-->DRV0
 *        INT: Lvl B Vec B Aff FFFFFFFF                                     |
 *                                                                          \-->DRV1
 *
 * Note that the NT architecture does not support switching only one IDE channel to native mode.
 */
static
CODE_SEG("PAGE")
NTSTATUS
PciIdeAssignLegacyResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PATA_CHANNEL_DATA ChanData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDesc = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDesc = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc = NULL;
    ULONG i;

    PAGED_CODE();

    if (!ResourcesTranslated)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc;

        Desc = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Desc->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                if (Desc->u.Port.Length == 1)
                {
                    if (!ControlPortDesc)
                        ControlPortDesc = Desc;
                }
                else if (Desc->u.Port.Length == 8)
                {
                    if (!CommandPortDesc)
                        CommandPortDesc = Desc;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDesc)
                    InterruptDesc = Desc;
                break;
            }

            default:
                break;
        }
    }

    if (!CommandPortDesc || !ControlPortDesc || !InterruptDesc)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    PciIdeInitTaskFileIoResources(&ChanData->Regs,
                                  (ULONG_PTR)CommandPortDesc->u.Port.Start.QuadPart,
                                  (ULONG_PTR)ControlPortDesc->u.Port.Start.QuadPart);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeStartChannel(
    _In_ PATA_CONTROLLER Controller,
    _In_ PPDO_DEVICE_EXTENSION PdoExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PATA_CHANNEL_DATA ChanData = PdoExt->ChanData;
    NTSTATUS Status;

    PAGED_CODE();

    /* Initialize I/O resources */
    if (Controller->Flags & CTRL_FLAG_IN_LEGACY_MOVE)
        Status = PciIdeAssignLegacyResources(Controller, ChanData, ResourcesTranslated);
    else
        Status = PciIdeAssignNativeResources(Controller, ChanData);
    if (!NT_SUCCESS(Status))
    {
        ERR("CH %lu: Failed to assign resources 0x%lx\n", ChanData->Channel, Status);
        return Status;
    }

    /* This controller does not support DMA */
    if (Controller->Flags & CTRL_FLAG_PIO_ONLY)
        ChanData->TransferModeSupportedBitmap &= PIO_ALL;

    if (Controller->Flags & CTRL_FLAG_IS_SIMPLEX)
        ChanData->ChanInfo |= ATA_CHANNEL_FLAG_SIMPLEX;

    /* Init default values */
    if (ChanData->MaximumTransferLength == 0)
        ChanData->MaximumTransferLength = ATA_MAX_TRANSFER_LENGTH;
    PdoExt->MaximumTransferLength = ChanData->MaximumTransferLength;
    PdoExt->TransferModeSupportedBitmap = ChanData->TransferModeSupportedBitmap;

    /* Initialize DMA resources */
    if (!PciIdeInitDma(Controller, PdoExt, ChanData))
    {
        /* Fall back to PIO */
        ChanData->TransferModeSupportedBitmap &= PIO_ALL;
        INFO("CH %lu: Non DMA channel\n", ChanData->Channel);
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
PciIdeStopChannel(
    _In_ PATA_CONTROLLER Controller,
    _In_ PPDO_DEVICE_EXTENSION PdoExt)
{
    PATA_CHANNEL_DATA ChanData = PdoExt->ChanData;

    PAGED_CODE();

    PciIdeFreeDmaResources(PdoExt, ChanData);

    ChanData->Regs.Dma = NULL;

    if (Controller->Flags & CTRL_FLAG_IN_LEGACY_MOVE)
        Controller->Flags &= ~CTRL_FLAG_HAS_INTERRUPT_RES;

    ChanData->ChanInfo &= ~(ATA_CHANNEL_FLAG_DRIVE0_DMA_CAPABLE |
                            ATA_CHANNEL_FLAG_DRIVE1_DMA_CAPABLE);
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeCreateChannelData(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG HwExtensionSize)
{
    PATA_CHANNEL_DATA ChanData;
    ULONG i, Size;

    PAGED_CODE();
    ASSERT(Controller->MaxChannels != 0);

    Size = FIELD_OFFSET(ATA_CHANNEL_DATA, HwExt) + HwExtensionSize;
    ChanData = ExAllocatePoolZero(NonPagedPool, Size * Controller->MaxChannels, TAG_PCIIDEX);
    if (!ChanData)
        return STATUS_INSUFFICIENT_RESOURCES;

    Controller->ChanDataBlock = ChanData;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        Controller->Channels[i] = ChanData;

        ChanData->Channel = i;
        ChanData->Controller = Controller;

        /* Set default values */
        ChanData->TransferModeSupportedBitmap = PIO_ALL | SWDMA_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
        ChanData->SetTransferMode = PciIdeSetTransferMode;

        ChanData = (PVOID)((ULONG_PTR)ChanData + Size);
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
PciIdeControllerIsSimplex(
    _In_ PATA_CONTROLLER Controller)
{
    PUCHAR IoBase;
    UCHAR DmaStatus;

    PAGED_CODE();

    /* Check if BAR4 is available */
    IoBase = AtaCtrlPciGetBar(Controller, PCIIDE_DMA_IO_BAR);
    if (!IoBase)
        return FALSE;

    /* We look at the primary channel status register to determine the simplex mode */
    DmaStatus = READ_PORT_UCHAR(IoBase + PCIIDE_DMA_STATUS);
    return !!(DmaStatus & PCIIDE_DMA_STATUS_SIMPLEX);
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
PciIdeControllerInitDefaults(
    _In_ PATA_CONTROLLER Controller)
{
    PAGED_CODE();

    Controller->Flags = CTRL_FLAG_PCI_IDE;
    Controller->MaxChannels = 2; // Primary and Secondary
    Controller->AlignmentRequirement = ATA_MIN_BUFFER_ALIGNMENT;
    Controller->Start = NULL;
    Controller->ChannelEnableBits = NULL;
    Controller->ChannelEnabledTest = NULL;
}

static
DECLSPEC_NOINLINE_FROM_PAGED
VOID
PciIdeCallStartController(
    _In_ PATA_CONTROLLER Controller)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Controller->Lock, &OldIrql);
    Controller->Start(Controller);
    KeReleaseSpinLock(&Controller->Lock, OldIrql);
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeStartController(
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

    PciIdeControllerInitDefaults(Controller);

    if (!(Controller->Pci.Command & PCI_ENABLE_BUS_MASTER))
    {
        INFO("PCI bus mastering disabled\n");
        Controller->Flags |= CTRL_FLAG_PIO_ONLY;
    }

    switch (Controller->Pci.VendorID)
    {
        case PCI_VEN_AMD:
        case PCI_VEN_NVIDIA:
            Status = AmdGetControllerProperties(Controller);
            break;
        case PCI_VEN_CMD:
            Status = CmdGetControllerProperties(Controller);
            break;
        case PCI_VEN_INTEL:
            Status = IntelGetControllerProperties(Controller);
            break;
        case PCI_VEN_PC_TECH:
            Status = PcTechGetControllerProperties(Controller);
            break;
        case PCI_VEN_TOSHIBA:
            Status = ToshibaGetControllerProperties(Controller);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }
    if (!NT_SUCCESS(Status))
    {
        INFO("Trying generic driver\n");

        if (Controller->Pci.BaseClass != PCI_CLASS_MASS_STORAGE_CTLR)
        {
            ERR("Unknown controller\n");
            return Status;
        }

        PciIdeControllerInitDefaults(Controller);

        Status = PciIdeGetControllerProperties(Controller);
    }
    if (!NT_SUCCESS(Status))
        return Status;

    if (Controller->Pci.BaseClass == PCI_CLASS_MASS_STORAGE_CTLR)
    {
        if (!(Controller->Pci.ProgIf & PCIIDE_PROGIF_DMA_CAPABLE))
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
                Controller->Flags |= CTRL_FLAG_IN_LEGACY_MOVE;
            }
        }
    }

    if (Controller->Flags & CTRL_FLAG_PCI_IDE)
    {
        ASSERT(Controller->MaxChannels <= 2);

        if (!(Controller->Flags & CTRL_FLAG_IN_LEGACY_MOVE))
        {
            if (!(Controller->Flags & CTRL_FLAG_HAS_INTERRUPT_RES))
            {
                ERR("No interrupt resource\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (!(Controller->Flags & CTRL_FLAG_IS_SIMPLEX))
        {
            if (PciIdeControllerIsSimplex(Controller))
                Controller->Flags |= CTRL_FLAG_IS_SIMPLEX;
        }

        if (Controller->Flags & CTRL_FLAG_IS_SIMPLEX)
        {
            INFO("Sync access for hardware is required\n");

            Controller->HwSyncObject = IoCreateController(0);
            if (!Controller->HwSyncObject)
                return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (Controller->Start)
        PciIdeCallStartController(Controller);

    return Status;
}