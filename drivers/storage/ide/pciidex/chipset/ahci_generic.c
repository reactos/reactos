/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     AHCI controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

PCIIDEX_PAGED_DATA
static const struct
{
    USHORT VendorID;
    USHORT DeviceID;
} AhciControllerList[] =
{
    { PCI_VEN_NVIDIA, 0x0550 },
    { PCI_VEN_NVIDIA, 0x0584 },
};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
NTSTATUS
AtaAhciAllocateMemory(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PDMA_OPERATIONS DmaOperations = ChanData->DmaAdapter->DmaOperations;
    ULONG i, j, SlotNumber, CommandSlots, BlockSize;
    ULONG CommandListSize, CommandTableLength, CommandTablesPerPage;
    PVOID Buffer;
    ULONG_PTR BufferVa;
    ULONG64 BufferPa;
    PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    /* The maximum size of the command list is 1024 bytes */
    CommandSlots = ChanData->Controller->QueueDepth;
    CommandListSize = FIELD_OFFSET(AHCI_COMMAND_LIST, CommandHeader[CommandSlots]);
    BlockSize = CommandListSize + (AHCI_COMMAND_LIST_ALIGNMENT - 1);

    /* Add the receive area structure (256 bytes) */
    if (!(ChanData->ChanInfo & CHANNEL_FLAG_HAS_FBS))
    {
        BlockSize += sizeof(AHCI_RECEIVED_FIS);

        /* The command list is 1024-byte aligned, which saves us some bytes of allocation size */
        BlockSize += ALIGN_UP_BY(CommandListSize, AHCI_RECEIVED_FIS_ALIGNMENT) - CommandListSize;
    }

    Buffer = DmaOperations->AllocateCommonBuffer(ChanData->DmaAdapter,
                                                 BlockSize,
                                                 &PhysicalAddress,
                                                 TRUE); // Cached
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(Buffer, BlockSize);

    ChanData->Mem.CommandListSize = BlockSize;
    ChanData->Mem.CommandListOriginal = Buffer;
    ChanData->Mem.CommandListPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

    BufferVa = (ULONG_PTR)Buffer;
    BufferPa = PhysicalAddress.QuadPart;

    /* Command list */
    BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_LIST_ALIGNMENT);
    BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_LIST_ALIGNMENT);
    ChanData->CommandList = (PVOID)BufferVa;
    ChanData->Mem.CommandListPhys = BufferPa;
    BufferVa += CommandListSize;
    BufferPa += CommandListSize;

    /* Alignment requirement */
    ASSERT((ULONG_PTR)ChanData->Mem.CommandListPhys % AHCI_COMMAND_LIST_ALIGNMENT == 0);

    /* Received FIS structure */
    if (!(ChanData->ChanInfo & CHANNEL_FLAG_HAS_FBS))
    {
        BufferVa = ALIGN_UP_BY(BufferVa, AHCI_RECEIVED_FIS_ALIGNMENT);
        BufferPa = ALIGN_UP_BY(BufferPa, AHCI_RECEIVED_FIS_ALIGNMENT);
        ChanData->ReceivedFis = (PVOID)BufferVa;
        ChanData->Mem.ReceivedFisPhys = BufferPa;
        BufferVa += sizeof(AHCI_RECEIVED_FIS);
        BufferPa += sizeof(AHCI_RECEIVED_FIS);

        /* Alignment requirement */
        ASSERT((ULONG_PTR)ChanData->Mem.ReceivedFisPhys % AHCI_RECEIVED_FIS_ALIGNMENT == 0);
    }

    if (ChanData->ChanInfo & CHANNEL_FLAG_HAS_FBS)
    {
        /* The FBS receive area is 4kB, allocate a page which is also 4kB-aligned */
        BlockSize = PAGE_SIZE;

        /*
         * Some other architectures, like ia64, use a different page size,
         * that is a multiple of 4096.
         */
        C_ASSERT(PAGE_SIZE % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);

        Buffer = DmaOperations->AllocateCommonBuffer(ChanData->DmaAdapter,
                                                     BlockSize,
                                                     &PhysicalAddress,
                                                     TRUE);
        if (!Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(Buffer, BlockSize);

        ChanData->Mem.ReceivedFisOriginal = Buffer;
        ChanData->Mem.ReceivedFisPhysOriginal.QuadPart = PhysicalAddress.QuadPart;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        ChanData->ReceivedFis = (PVOID)BufferVa;
        ChanData->Mem.ReceivedFisPhys = BufferPa;

        /* Alignment requirement */
        ASSERT(BufferPa % AHCI_RECEIVED_FIS_FBS_ALIGNMENT == 0);
    }

    /* 32-bit DMA */
    if (!(ChanData->ChanInfo & CHANNEL_FLAG_64_BIT_DMA))
    {
        ASSERT((ULONG)(ChanData->Mem.CommandListPhys >> 32) == 0);
        ASSERT((ULONG)(ChanData->Mem.ReceivedFisPhys >> 32) == 0);
    }

    CommandTableLength = FIELD_OFFSET(AHCI_COMMAND_TABLE, PrdTable[ChanData->MaximumPhysicalPages]);

    ASSERT(ChanData->MaximumPhysicalPages != 0 &&
           ChanData->MaximumPhysicalPages <= AHCI_MAX_PRDT_ENTRIES);

    /*
     * See ATA_MAX_TRANSFER_LENGTH, currently the MaximumPhysicalPages is restricted to
     * a maximum of (0x20000 / PAGE_SIZE) + 1 = 33 pages.
     * Each command table will require us 128 + 16 * 33 + (128 - 1) = 783 bytes of shared memory.
     */
    ASSERT(PAGE_SIZE > (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)));

    /* Allocate one-page chunks to avoid having a large chunk of contiguous memory */
    CommandTablesPerPage = PAGE_SIZE / (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1));

    /* Command tables allocation loop */
    SlotNumber = 0;
    i = CommandSlots;
    while (i > 0)
    {
        ULONG TableCount;

        TableCount = min(i, CommandTablesPerPage);
        BlockSize = (CommandTableLength + (AHCI_COMMAND_TABLE_ALIGNMENT - 1)) * TableCount;

        /* Allocate a chunk of memory */
        Buffer = DmaOperations->AllocateCommonBuffer(ChanData->DmaAdapter,
                                                     BlockSize,
                                                     &PhysicalAddress,
                                                     TRUE);
        if (!Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(Buffer, BlockSize);

        ChanData->Mem.CommandTableOriginal[SlotNumber] = Buffer;
        ChanData->Mem.CommandTablePhysOriginal[SlotNumber].QuadPart = PhysicalAddress.QuadPart;
        ChanData->Mem.CommandTableSize[SlotNumber] = BlockSize;

        BufferVa = (ULONG_PTR)Buffer;
        BufferPa = PhysicalAddress.QuadPart;

        /* Split the allocation into command tables */
        for (j = 0; j < TableCount; ++j)
        {
            PAHCI_COMMAND_HEADER CommandHeader;

            BufferVa = ALIGN_UP_BY(BufferVa, AHCI_COMMAND_TABLE_ALIGNMENT);
            BufferPa = ALIGN_UP_BY(BufferPa, AHCI_COMMAND_TABLE_ALIGNMENT);

            /* Alignment requirement */
            ASSERT(BufferPa % AHCI_COMMAND_TABLE_ALIGNMENT == 0);

            /* 32-bit DMA */
            if (!(ChanData->ChanInfo & CHANNEL_FLAG_64_BIT_DMA))
            {
                ASSERT((ULONG)(BufferPa >> 32) == 0);
            }

            ChanData->CommandTable[SlotNumber] = (PAHCI_COMMAND_TABLE)BufferVa;

            CommandHeader = &ChanData->CommandList->CommandHeader[SlotNumber];
            CommandHeader->CommandTableBaseLow = (ULONG)BufferPa;
            CommandHeader->CommandTableBaseHigh = (ULONG)(BufferPa >> 32);

            ++SlotNumber;
            BufferVa += CommandTableLength;
            BufferPa += CommandTableLength;
        }

        i -= TableCount;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
AtaAhciFreeMemory(
    _In_ PVOID ChannelContext)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;
    PDMA_ADAPTER DmaAdapter = ChanData->DmaAdapter;
    PDMA_OPERATIONS DmaOperations = ChanData->DmaAdapter->DmaOperations;
    ULONG i;

    PAGED_CODE();

    if (ChanData->Mem.CommandListOriginal)
    {
        DmaOperations->FreeCommonBuffer(DmaAdapter,
                                        ChanData->Mem.CommandListSize,
                                        ChanData->Mem.CommandListPhysOriginal,
                                        ChanData->Mem.CommandListOriginal,
                                        TRUE); // Cached
        ChanData->Mem.CommandListOriginal = NULL;
    }

    if (ChanData->Mem.ReceivedFisOriginal)
    {
        DmaOperations->FreeCommonBuffer(DmaAdapter,
                                        PAGE_SIZE,
                                        ChanData->Mem.ReceivedFisPhysOriginal,
                                        ChanData->Mem.ReceivedFisOriginal,
                                        TRUE);
        ChanData->Mem.ReceivedFisOriginal = NULL;
    }

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        if (ChanData->Mem.CommandTableOriginal[i])
        {
            DmaOperations->FreeCommonBuffer(DmaAdapter,
                                            ChanData->Mem.CommandTableSize[i],
                                            ChanData->Mem.CommandTablePhysOriginal[i],
                                            ChanData->Mem.CommandTableOriginal[i],
                                            TRUE);
            ChanData->Mem.CommandTableOriginal[i] = NULL;
        }
    }
}

#if DBG
static
CODE_SEG("PAGE")
BOOLEAN
AtaAhciIsVbox(VOID)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, DeviceID)];
    PPCI_COMMON_HEADER PciData = (PPCI_COMMON_HEADER)Buffer; // Partial PCI header
    ULONG BytesRead;
    PCI_SLOT_NUMBER Slot;

    PAGED_CODE();

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber = 4;
    Slot.u.bits.FunctionNumber = 0;

    BytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                      0,
                                      Slot.u.AsULONG,
                                      &Buffer,
                                      FIELD_OFFSET(PCI_COMMON_HEADER, VendorID),
                                      sizeof(Buffer));
    return (BytesRead == sizeof(Buffer)) &&
           (PciData->VendorID == 0x80EE) &&
           (PciData->DeviceID == 0xCAFE);
}
#endif

static
CODE_SEG("PAGE")
VOID
AtaAhciHbaRequestOsOwnership(
    _In_ PVOID IoBase)
{
    ULONG i, Control;

    PAGED_CODE();

    Control = AHCI_HBA_READ(IoBase, HbaBiosHandoffControl);
    if (Control & AHCI_BOHC_OS_SEMAPHORE)
        return;

    INFO("HBA ownership change\n");

    AHCI_HBA_WRITE(IoBase, HbaBiosHandoffControl, Control | AHCI_BOHC_OS_SEMAPHORE);

    /* Wait up to 2 seconds */
    for (i = 0; i < 200000; ++i)
    {
        Control = AHCI_HBA_READ(IoBase, HbaBiosHandoffControl);

        if (!(Control & (AHCI_BOHC_BIOS_BUSY | AHCI_BOHC_BIOS_SEMAPHORE)))
            return;

        KeStallExecutionProcessor(10);
    }

    WARN("Unable to acquire the OS semaphore %08lx\n", Control);
}

/**
 * See eSATA paper
 * http://download.microsoft.com/download/7/E/7/7E7662CF-CBEA-470B-A97E-CE7CE0D98DC2/eSATA.docx
 */
static
CODE_SEG("PAGE")
BOOLEAN
AtaAhciIsPortRemovable(
    _In_ ULONG AhciCapabilities,
    _In_ ULONG CmdStatus)
{
    PAGED_CODE();

    if (CmdStatus & AHCI_PXCMD_HPCP)
        return TRUE;

    if ((AhciCapabilities & AHCI_CAP_SXS) && (CmdStatus & AHCI_PXCMD_ESP))
        return TRUE;

    if ((AhciCapabilities & AHCI_CAP_SMPS) && (CmdStatus & AHCI_PXCMD_MPSP))
        return TRUE;

    return FALSE;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaAhciCreateChannelData(
    _In_ PATA_CONTROLLER Controller)
{
    PCHANNEL_DATA_AHCI ChanData;
    ULONG i;

    PAGED_CODE();
    ASSERT(Controller->MaxChannels != 0);

    ChanData = ExAllocatePoolZero(NonPagedPool,
                                  sizeof(*ChanData) * Controller->MaxChannels,
                                  TAG_PCIIDEX);
    if (!ChanData)
        return STATUS_INSUFFICIENT_RESOURCES;

    Controller->ChanDataBlock = ChanData;

    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        ULONG CmdStatus;

        if (!(Controller->ChannelBitmap & (1 << i)))
            continue;

        Controller->Channels[i] = ChanData;

        ChanData->Channel = i;
        ChanData->Controller = Controller;

        ChanData->AllocateMemory = AtaAhciAllocateMemory;
        ChanData->FreeMemory = AtaAhciFreeMemory;
        ChanData->EnableInterrupts = AtaAhciEnableInterrupts;
        ChanData->PreparePrdTable = AtaAhciPreparePrdTable;
        ChanData->PrepareIo = AtaAhciPrepareIo;
        ChanData->StartIo = AtaAhciStartIo;
        ChanData->SetTransferMode = SataSetTransferMode;
        ChanData->TransferModeSupported = SATA_ALL;

        ChanData->IoBase = AHCI_PORT_BASE(Controller->IoBase, i);

        ChanData->EnableInterrupts(ChanData, FALSE);

        /* Begin the process of stopping the command list DMA engine for later initialization */
        CmdStatus = AHCI_PORT_READ(ChanData->IoBase, PxCmdStatus);
        if (CmdStatus & AHCI_PXCMD_ST)
        {
            CmdStatus &= ~AHCI_PXCMD_ST;
            AHCI_PORT_WRITE(ChanData->IoBase, PxCmdStatus, CmdStatus);
        }

        /* The AHCI HBA can only perform DMA I/O and PIO is not supported */
        ChanData->ChanInfo = CHANNEL_FLAG_PIO_VIA_DMA;

        if (Controller->AhciCapabilities & AHCI_CAP_S64A)
            ChanData->ChanInfo |= CHANNEL_FLAG_64_BIT_DMA;

        if (Controller->AhciCapabilities & AHCI_CAP_SNCQ)
            ChanData->ChanInfo |= CHANNEL_FLAG_HAS_NCQ;

        /* Check for the FIS-based switching feature support */
        if ((Controller->AhciCapabilities & AHCI_CAP_SPM) &&
            (Controller->AhciCapabilities & AHCI_CAP_FBSS) &&
            (CmdStatus & AHCI_PXCMD_FBSCP))
        {
            INFO("CH %lu: FBS supported\n", ChanData->Channel);
            ChanData->ChanInfo |= CHANNEL_FLAG_HAS_FBS;
        }

        if (AtaAhciIsPortRemovable(Controller->AhciCapabilities, CmdStatus))
        {
            INFO("CH %lu: Port is external\n", ChanData->Channel);
            ChanData->ChanInfo |= CHANNEL_FLAG_IS_EXTERNAL;
        }

        ++ChanData;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
PVOID
AtaAhciGetAbar(
    _Inout_ PATA_CONTROLLER Controller)
{
    PVOID Abar;
    ULONG i, Index;
    PCIIDEX_PAGED_DATA static const struct
    {
        USHORT VendorID;
        USHORT DeviceID;
        ULONG Index;
    } AbarLocations[] =
    {
        { 0x177D, 0xA01C, 0 },
    };

    PAGED_CODE();

    /* Default index */
    Index = 5;

    for (i = 0; i < RTL_NUMBER_OF(AbarLocations); ++i)
    {
        if ((Controller->Pci.VendorID == AbarLocations[i].VendorID) &&
            (Controller->Pci.DeviceID == AbarLocations[i].DeviceID))
        {
            Index = AbarLocations[i].Index;
            break;
        }
    }

    if (!(Controller->AccessRange[Index].Flags & RANGE_IS_MEMORY))
        return NULL;

    Abar = AtaCtrlPciGetBar(Controller, Index, 0);
    if (!Abar)
        return NULL;

    return Abar;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaAhciAttachChannel(
    _In_ PVOID ChannelContext,
    _In_ BOOLEAN Attach)
{
    PCHANNEL_DATA_AHCI ChanData = ChannelContext;

    PAGED_CODE();

    if (Attach)
    {
        /* We do enable interrupts in the QBR handler */
    }
    else
    {
        AtaChanEnableInterruptsSync(ChanData, FALSE);
        // FIXME: stop the CR and FRE DMA engines
    }

    return STATUS_SUCCESS;
}

static
VOID
AtaAhciHbaStart(
    _In_ PATA_CONTROLLER Controller)
{
    ULONG GlobalControl;

    GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
    if (!(GlobalControl & AHCI_GHC_AE))
    {
        /* Set AE on power up */
        GlobalControl |= AHCI_GHC_AE;
        AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);
    }

    /* Clear HBA interrupts */
    AHCI_HBA_WRITE(Controller->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    /* Enable interrupts */
    GlobalControl |= AHCI_GHC_IE;
    AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);
}

static
VOID
AtaAhciHbaStop(
    _In_ PATA_CONTROLLER Controller)
{
    ULONG GlobalControl;
    KIRQL OldIrql;

    /* Failed to connect interrupt */
    if (!Controller->InterruptObject)
        return;

    OldIrql = KeAcquireInterruptSpinLock(Controller->InterruptObject);

    /* Disable interrupts */
    GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
    GlobalControl &= ~AHCI_GHC_IE;
    AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);

    /* Clear HBA interrupts */
    AHCI_HBA_WRITE(Controller->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    KeReleaseInterruptSpinLock(Controller->InterruptObject, OldIrql);
}

static
CODE_SEG("PAGE")
VOID
AtaAhciHbaFreeResouces(
    _In_ PATA_CONTROLLER Controller)
{
    PAGED_CODE();

    if (Controller->InterruptObject)
    {
        IoDisconnectInterrupt(Controller->InterruptObject);
        Controller->InterruptObject = NULL;
    }
}

static
CODE_SEG("PAGE")
BOOLEAN
AhciControllerIsSubClassCheckNeeded(
    _In_ PATA_CONTROLLER Controller)
{
    PAGED_CODE();

    if (Controller->Pci.VendorID == PCI_VEN_INTEL)
    {
        /* IDE or AHCI */
        if ((Controller->Pci.DeviceID == 0x2652) || (Controller->Pci.DeviceID == 0x2653))
            return TRUE;
    }

    /* IDE, RAID, or AHCI */
    if ((Controller->Pci.VendorID == PCI_VEN_VIA) && (Controller->Pci.DeviceID == 0x3349))
        return TRUE;

    return FALSE;
}

static
CODE_SEG("PAGE")
BOOLEAN
AhciMatchController(
    _In_ PATA_CONTROLLER Controller)
{
    ULONG i;

    PAGED_CODE();

    /* Some controllers share the same PCI ID between AHCI and IDE/RAID modes */
    if (AhciControllerIsSubClassCheckNeeded(Controller))
    {
        if (Controller->Pci.SubClass != PCI_SUBCLASS_MSC_AHCI_CTLR)
            return FALSE;
    }

    /*
     * Match the controller through the PCI ID.
     * We do not want to check the PCI subclass code because of
     * some AHCI controllers that support AHCI even in IDE emulation mode.
     * For example, nVidia 10DE:0550 uses the same PCI ID between AHCI and IDE modes,
     * but BAR5 will always be available and AHCI can be enabled by setting GHC.AE to 1.
     */
    for (i = 0; i < RTL_NUMBER_OF(AhciControllerList); ++i)
    {
        if ((Controller->Pci.VendorID == AhciControllerList[i].VendorID) &&
            (Controller->Pci.DeviceID == AhciControllerList[i].DeviceID))
        {
            return TRUE;
        }
    }

    /* Check for generic PCI AHCI controller */
    if ((Controller->Pci.BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (Controller->Pci.SubClass == PCI_SUBCLASS_MSC_AHCI_CTLR))
    {
        return TRUE;
    }

    return FALSE;
}

CODE_SEG("PAGE")
NTSTATUS
AhciGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc = &Controller->InterruptDesc;
    ULONG i, GlobalControl;
    NTSTATUS Status;

    PAGED_CODE();

    if (!AhciMatchController(Controller))
        return STATUS_NO_MATCH;

    Controller->IoBase = AtaAhciGetAbar(Controller);
    if (!Controller->IoBase)
        return STATUS_NO_MATCH; // Try IDE/RAID

    Controller->Flags = CTRL_FLAG_IS_AHCI | CTRL_FLAG_SATA_HBA_ACPI;
    Controller->Start = AtaAhciHbaStart;
    Controller->Stop = AtaAhciHbaStop;
    Controller->FreeResources = AtaAhciHbaFreeResouces;
    Controller->AttachChannel = AtaAhciAttachChannel;

    /* Set AE before accessing other AHCI registers */
    GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
    GlobalControl |= AHCI_GHC_AE;
    AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);

    Controller->AhciCapabilities = AHCI_HBA_READ(Controller->IoBase, HbaCapabilities);
    Controller->ChannelBitmap = AHCI_HBA_READ(Controller->IoBase, HbaPortBitmap);
    Controller->MaxChannels = CountSetBits(Controller->ChannelBitmap);
    if (Controller->MaxChannels == 0)
    {
        ASSERT(Controller->MaxChannels == 0);
        return STATUS_DEVICE_HARDWARE_ERROR;
    }

    Controller->AhciVersion = AHCI_HBA_READ(Controller->IoBase, HbaAhciVersion);
    if (Controller->AhciVersion >= AHCI_VERSION_1_2)
    {
        Controller->AhciCapabilitiesEx = AHCI_HBA_READ(Controller->IoBase, HbaCapabilitiesEx);

        if (Controller->AhciCapabilitiesEx & AHCI_CAP2_BOH)
            AtaAhciHbaRequestOsOwnership(Controller->IoBase);
    }

    /* Reset the HBA into a consistent state */
    GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
    GlobalControl |= AHCI_GHC_HR;
    AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);

    /* HBA reset may take up to 1 second */
    for (i = 100000; i > 0; i--)
    {
        GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
        if (!(GlobalControl & AHCI_GHC_HR))
            break;

        KeStallExecutionProcessor(10);
    }
    if (i == 0)
    {
        ERR("HBA reset failed %08lx\n", GlobalControl);
        return STATUS_IO_TIMEOUT;
    }

    /* Re-enable AE */
    GlobalControl |= AHCI_GHC_AE;
    AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);

    /* Disable interrupts */
    GlobalControl = AHCI_HBA_READ(Controller->IoBase, HbaGlobalControl);
    if (GlobalControl & AHCI_GHC_IE)
    {
        GlobalControl &= ~AHCI_GHC_IE;
        AHCI_HBA_WRITE(Controller->IoBase, HbaGlobalControl, GlobalControl);
    }

#if DBG
    /* On virtual machines, this will allow us to test the PMP support code */
    if (AtaAhciIsVbox())
        Controller->AhciCapabilities |= AHCI_CAP_SPM;
#endif

    Status = AtaAhciCreateChannelData(Controller);
    if (!NT_SUCCESS(Status))
        return Status;

    Controller->QueueDepth = ((Controller->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;

    INFO("%04X:%04X.%02X: Ver %08lX, PI %08lX, CAP %08lX, CAP2 %08lX\n",
         Controller->Pci.VendorID,
         Controller->Pci.DeviceID,
         Controller->Pci.RevisionID,
         Controller->AhciVersion,
         Controller->ChannelBitmap,
         Controller->AhciCapabilities,
         Controller->AhciCapabilitiesEx);

    /* Clear HBA interrupts */
    AHCI_HBA_WRITE(Controller->IoBase, HbaInterruptStatus, 0xFFFFFFFF);

    Status = IoConnectInterrupt(&Controller->InterruptObject,
                                AtaAhciHbaIsr,
                                Controller,
                                NULL,
                                InterruptDesc->u.Interrupt.Vector,
                                InterruptDesc->u.Interrupt.Level,
                                InterruptDesc->u.Interrupt.Level,
                                (InterruptDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
                                ? Latched
                                : LevelSensitive,
                                (InterruptDesc->ShareDisposition == CmResourceShareShared),
                                InterruptDesc->u.Interrupt.Affinity,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not connect to interrupt %lu, status 0x%lx\n",
            InterruptDesc->u.Interrupt.Vector, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}
