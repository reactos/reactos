/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     ServerWorks/Broadcom ATA controller minidriver
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Adapted from the FreeBSD ata-serverworks driver
 * Copyright (c) 1998-2008 Søren Schmidt <sos@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_OSB4_IDE                    0x0211 // TODO: Clear the simplex bit?
#define PCI_DEV_CSB5_IDE                    0x0212
#define PCI_DEV_CSB6_IDE                    0x0213
#define PCI_DEV_CSB6_IDE_THIRD              0x0217
#define PCI_DEV_HT1000_IDE                  0x0214

#define PCI_DEV_K2_SATA                     0x0240
#define PCI_DEV_BCM5770_SATA                0x0241
#define PCI_DEV_BCM5770R_SATA               0x0242
#define PCI_DEV_HT1000_SATA                 0x024A
#define PCI_DEV_HT1000_SATA_2               0x024B
#define PCI_DEV_HT1100_SATA                 0x0410
#define PCI_DEV_HT1100_SATA_2               0x0411

#define PCI_DEV_OSB4_BRIDGE                 0x0210

#define CTRL_FLAGS_HAS_UDMA5                0x01

#define HW_FLAGS_SATA                       0x01
#define HW_FLAGS_NO_ATAPI_DMA               0x02
#define HW_FLAGS_NO_64K_DMA                 0x04

#define SVW_REG_PIO_TIMING                  0x40
#define SVW_REG_DMA_TIMING                  0x44
#define SVW_REG_PIO_MODE                    0x4A // CSB5 and later
#define SVW_REG_UDMA_ENABLE                 0x54
#define SVW_REG_UDMA_MODE                   0x56
#define SVW_REG_UDMA_CONTROL                0x5A // Absent on ATI

#define SVW_UDMA_CTRL_MODE_MASK             0x03
#define SVW_UDMA_CTRL_DISABLE               0x40

#define SVW_UDMA_CTRL_MODE_UDMA4            0x02
#define SVW_UDMA_CTRL_MODE_UDMA5            0x03

#define SVW_SATA_PORT_BASE_OFFSET           0x100
#define SVW_SATA_PORT_CONTROL_OFFSET        0x20
#define SVW_SATA_PORT_DMA_OFFSET            0x30
#define SVW_SATA_PORT_TF_STRIDE             4

PCIIDEX_PAGED_DATA
static const struct
{
    USHORT DeviceID;
    UCHAR PortCount;
    UCHAR Flags;
} SvwControllerList[] =
{
    { PCI_DEV_OSB4_IDE,       2, HW_FLAGS_NO_64K_DMA },
    { PCI_DEV_CSB5_IDE,       2, 0 },
    { PCI_DEV_CSB6_IDE,       2, 0 },
    { PCI_DEV_CSB6_IDE_THIRD, 1, 0 },
    { PCI_DEV_HT1000_IDE,     1, HW_FLAGS_NO_64K_DMA },
    { PCI_DEV_K2_SATA,        4, HW_FLAGS_SATA | HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_BCM5770_SATA,   8, HW_FLAGS_SATA | HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_BCM5770R_SATA,  4, HW_FLAGS_SATA | HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1000_SATA,    4, HW_FLAGS_SATA | HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1000_SATA_2,  4, HW_FLAGS_SATA | HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1100_SATA,    4, HW_FLAGS_SATA },
    { PCI_DEV_HT1100_SATA_2,  4, HW_FLAGS_SATA },
};

static const struct
{
    UCHAR Value;
    ULONG CycleTime;
} SvwPioTimings[] =
{
    { 0x5D, 600 }, // 0
    { 0x47, 390 }, // 1
    { 0x34, 270 }, // 2
    { 0x22, 180 }, // 3
    { 0x20, 120 }, // 4
};

static const struct
{
    UCHAR Value;
    ULONG CycleTime;
} SvwMwDmaTimings[] =
{
    { 0x77, 480 }, // 0
    { 0x21, 150 }, // 1
    { 0x20, 120 }, // 2
};

/* FUNCTIONS ******************************************************************/

static
VOID
SvwChooseDeviceSpeed(
    _In_ ULONG Channel,
    _In_ PCHANNEL_DEVICE_CONFIG Device)
{
    ULONG Mode;

    /* PIO speed */
    for (Mode = Device->PioMode; Mode > PIO_MODE(0); Mode--)
    {
        if (!(Device->SupportedModes & (1 << Mode)))
            continue;

        if (SvwPioTimings[Mode].CycleTime >= Device->MinPioCycleTime)
            break;
    }
    if (Mode != Device->PioMode)
        INFO("CH %lu: Downgrade PIO speed from %lu to %lu\n", Channel, Device->PioMode, Mode);
    Device->PioMode = Mode;

    /* UDMA works independently of any PIO mode */
    if (Device->DmaMode == PIO_MODE(0) || Device->DmaMode >= UDMA_MODE(0))
        return;

    /* DMA speed */
    for (Mode = Device->DmaMode; Mode > PIO_MODE(0); Mode--)
    {
        if (!(Device->SupportedModes & ~PIO_ALL & (1 << Mode)))
            continue;

        ASSERT((1 << Mode) & MWDMA_ALL);

        if (SvwMwDmaTimings[Mode - MWDMA_MODE(0)].CycleTime >= Device->MinMwDmaCycleTime)
            break;
    }
    if (Mode != Device->DmaMode)
    {
        if (Mode == PIO_MODE(0))
            WARN("CH %lu: Too slow device '%s', disabling DMA\n", Channel, Device->FriendlyName);
        else
            INFO("CH %lu: Downgrade DMA speed from %lu to %lu\n", Channel, Device->DmaMode, Mode);
    }
    Device->DmaMode = Mode;
}

VOID
SvwSetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    USHORT PioModeReg, UdmaModeReg;
    UCHAR UdmaEnableReg;
    ULONG i, PioTimReg, DmaTimReg;

    if (Controller->Pci.DeviceID != PCI_DEV_OSB4_IDE)
        PioModeReg = PciRead16(Controller, SVW_REG_PIO_MODE);
    else
        PioModeReg = 0;

    PioTimReg = PciRead32(Controller, SVW_REG_PIO_TIMING);
    DmaTimReg = PciRead32(Controller, SVW_REG_DMA_TIMING);
    UdmaEnableReg = PciRead8(Controller, SVW_REG_UDMA_ENABLE);
    UdmaModeReg = PciRead16(Controller, SVW_REG_UDMA_MODE);

    INFO("CH %lu: Config (before)\n"
         "DMA Timing   %08lX\n"
         "PIO Timing   %08lX\n"
         "PIO Mode         %04X\n"
         "UDMA Mode        %04X\n",
         "UDMA Enable        %02X\n",
         Channel,
         DmaTimReg,
         PioTimReg,
         PioModeReg,
         UdmaModeReg,
         UdmaEnableReg);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        const ULONG DeviceIndex = (Channel << 1) + i; // 0   1   2   3
        const ULONG Shift = (DeviceIndex ^ 1) << 3;   // 8   0   24  16

        UdmaModeReg &= ~(0x0F << (DeviceIndex << 2));
        UdmaEnableReg &= ~(0x01 << DeviceIndex);

        if (!Device)
            continue;

        /* Make sure that the selected PIO and DMA speeds match the cycle time */
        SvwChooseDeviceSpeed(Channel, Device);

        /* UDMA timings */
        if (Device->DmaMode >= UDMA_MODE(0))
        {
            ULONG ModeIndex = Device->DmaMode - UDMA_MODE(0);

            UdmaModeReg |= ModeIndex << (DeviceIndex << 2);
            UdmaEnableReg |= 1 << DeviceIndex;

            DmaTimReg &= ~(0xFF << Shift);
            DmaTimReg |= SvwMwDmaTimings[2].Value << Shift;
        }
        /* DMA timings */
        else if (Device->DmaMode != PIO_MODE(0))
        {
            ULONG ModeIndex = Device->DmaMode - MWDMA_MODE(0);

            DmaTimReg &= ~(0xFF << Shift);
            DmaTimReg |= SvwMwDmaTimings[ModeIndex].Value << Shift;
        }

        /* PIO timings */
        PioModeReg &= ~(0x0F << (DeviceIndex << 2));
        PioModeReg |= Device->PioMode << (DeviceIndex << 2);

        PioTimReg &= ~(0xFF << Shift);
        PioTimReg |= SvwPioTimings[Device->PioMode].Value << Shift;
    }

    PciWrite32(Controller, SVW_REG_PIO_TIMING, PioTimReg);

    if (Controller->Pci.DeviceID != PCI_DEV_OSB4_IDE)
        PciWrite16(Controller, SVW_REG_PIO_MODE, PioModeReg);

    PciWrite32(Controller, SVW_REG_DMA_TIMING, DmaTimReg);
    PciWrite16(Controller, SVW_REG_UDMA_MODE, UdmaModeReg);
    PciWrite8(Controller, SVW_REG_UDMA_ENABLE, UdmaEnableReg);

    INFO("CH %lu: Config (after)\n"
         "DMA Timing   %08lX\n"
         "PIO Timing   %08lX\n"
         "PIO Mode         %04X\n"
         "UDMA Mode        %04X\n",
         "UDMA Enable        %02X\n",
         Channel,
         DmaTimReg,
         PioTimReg,
         PioModeReg,
         UdmaModeReg,
         UdmaEnableReg);
}

CODE_SEG("PAGE")
BOOLEAN
SvwHasUdmaCable(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    UCHAR UdmaModeReg;

    PAGED_CODE();

    UdmaModeReg = PciRead8(Controller, SVW_REG_UDMA_MODE + Channel);

    /* Check mode settings, see if UDMA3 or higher mode is active */
    return ((UdmaModeReg & 0x07) > 2) || ((UdmaModeReg & 0x70) > 0x20);
}

static
UCHAR
SvwSataReadStatus(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    /*
     * The SATA hardware expects 32-bit reads on the status register
     * to get it latched propertly.
     */
    return READ_REGISTER_ULONG((PULONG)ChanData->Regs.Status);
}

static
VOID
SvwSataLoadTaskFile(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _In_ ATA_DEVICE_REQUEST* __restrict Request)
{
    USHORT Features, SectorCount, LbaLow, LbaMid, LbaHigh;

    Features = Request->TaskFile.Feature;
    SectorCount = Request->TaskFile.SectorCount;
    LbaLow = Request->TaskFile.LowLba;
    LbaMid = Request->TaskFile.MidLba;
    LbaHigh = Request->TaskFile.HighLba;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        Features |= (USHORT)Request->TaskFile.FeatureEx << 8;
        SectorCount |= (USHORT)Request->TaskFile.SectorCountEx << 8;
        LbaLow |= (USHORT)Request->TaskFile.LowLbaEx << 8;
        LbaMid |= (USHORT)Request->TaskFile.MidLbaEx << 8;
        LbaHigh |= (USHORT)Request->TaskFile.HighLbaEx << 8;
    }

    /* The SATA hardware needs 16-bit accesses for the second byte of FIFO */
    WRITE_REGISTER_USHORT((PUSHORT)ChanData->Regs.Features, Features);
    WRITE_REGISTER_USHORT((PUSHORT)ChanData->Regs.SectorCount, SectorCount);
    WRITE_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaLow, LbaLow);
    WRITE_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaMid, LbaMid);
    WRITE_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaHigh, LbaHigh);

    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
        WRITE_REGISTER_UCHAR(ChanData->Regs.Device, Request->TaskFile.DriveSelect);
}

static
VOID
SvwSataSaveTaskFile(
    _In_ CHANNEL_DATA_PATA* __restrict ChanData,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->Output;
    USHORT Error, SectorCount, LbaLow, LbaMid, LbaHigh;

    TaskFile->DriveSelect = READ_REGISTER_UCHAR(ChanData->Regs.Device);
    TaskFile->Command = READ_REGISTER_UCHAR(ChanData->Regs.Command);

    Error = READ_REGISTER_USHORT((PUSHORT)ChanData->Regs.Error);
    SectorCount = READ_REGISTER_USHORT((PUSHORT)ChanData->Regs.SectorCount);
    LbaLow = READ_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaLow);
    LbaMid = READ_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaMid);
    LbaHigh = READ_REGISTER_USHORT((PUSHORT)ChanData->Regs.LbaHigh);

    TaskFile->Error = Error & 0xFF;
    TaskFile->SectorCount = SectorCount & 0xFF;
    TaskFile->LowLba = LbaLow & 0xFF;
    TaskFile->MidLba = LbaMid & 0xFF;
    TaskFile->HighLba = LbaHigh & 0xFF;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        TaskFile->FeatureEx = Error >> 8;
        TaskFile->SectorCountEx = SectorCount >> 8;
        TaskFile->LowLbaEx = LbaLow >> 8;
        TaskFile->MidLbaEx = LbaMid >> 8;
        TaskFile->HighLbaEx = LbaHigh >> 8;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
SvwSataParseResources(
    _In_ PVOID ChannelContext,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCHANNEL_DATA_PATA ChanData = ChannelContext;
    PUCHAR IoBase;

    PAGED_CODE();

    IoBase = AtaCtrlPciGetBar(ChanData->Controller,
                              (ChanData->Controller->Pci.DeviceID == PCI_DEV_HT1100_SATA) ? 3 : 5,
                              0);
    if (!IoBase)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    IoBase += ChanData->Channel * SVW_SATA_PORT_BASE_OFFSET;

    ChanData->Regs.Dma = IoBase + SVW_SATA_PORT_DMA_OFFSET;

    PciIdeInitTaskFileIoResources(ChanData,
                                  (ULONG_PTR)IoBase,
                                  (ULONG_PTR)IoBase + SVW_SATA_PORT_CONTROL_OFFSET,
                                  SVW_SATA_PORT_TF_STRIDE);

    // FIXME: Mark the regs as MMIO
    return STATUS_SUCCESS;
}

static
BOOLEAN
SvwOsb4PciBridgeInit(
    _In_ PVOID Context,
    _In_ ULONG BusNumber,
    _In_ PCI_SLOT_NUMBER PciSlot,
    _In_ PPCI_COMMON_HEADER PciConfig)
{
    ULONG Value;

    UNREFERENCED_PARAMETER(Context);

    if (PciConfig->VendorID != PCI_VEN_SERVERWORKS ||
        PciConfig->DeviceID != PCI_DEV_OSB4_BRIDGE)
    {
        return FALSE;
    }

    HalGetBusDataByOffset(PCIConfiguration,
                          BusNumber,
                          PciSlot.u.AsULONG,
                          &Value,
                          0x64,
                          sizeof(Value));
    Value &= ~0x00002000; // Disable 600ns interrupt mask
    Value |= 0x00004000; // Enable UDMA/33 support
    HalSetBusDataByOffset(PCIConfiguration,
                          BusNumber,
                          PciSlot.u.AsULONG,
                          &Value,
                          0x64,
                          sizeof(Value));
    return TRUE;
}

static
VOID
SvwPataControllerStart(
    _In_ PATA_CONTROLLER Controller)
{
    if (Controller->Pci.DeviceID == PCI_DEV_OSB4_IDE)
    {
        PciFindDevice(SvwOsb4PciBridgeInit, NULL);
    }
    else
    {
        UCHAR UdmaCtrlReg = PciRead8(Controller, SVW_REG_UDMA_CONTROL);

        UdmaCtrlReg &= ~(SVW_UDMA_CTRL_DISABLE | SVW_UDMA_CTRL_MODE_MASK);
        if (Controller->CtrlFlags & CTRL_FLAGS_HAS_UDMA5)
            UdmaCtrlReg |= SVW_UDMA_CTRL_MODE_UDMA5;
        else
            UdmaCtrlReg |= SVW_UDMA_CTRL_MODE_UDMA4;

        PciWrite8(Controller, SVW_REG_UDMA_CONTROL, UdmaCtrlReg);
    }
}

CODE_SEG("PAGE")
NTSTATUS
SvwGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, HwFlags, SupportedMode;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_SERVERWORKS);

    for (i = 0; i < RTL_NUMBER_OF(SvwControllerList); ++i)
    {
        HwFlags = SvwControllerList[i].Flags;

        if (Controller->Pci.DeviceID == SvwControllerList[i].DeviceID)
            break;
    }
    if (i == RTL_NUMBER_OF(SvwControllerList))
        return STATUS_NO_MATCH;

    Controller->MaxChannels = SvwControllerList[i].PortCount;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    if (HwFlags & HW_FLAGS_SATA)
    {
        PUCHAR IoBase;

        SupportedMode = SATA_ALL;

        IoBase = AtaCtrlPciGetBar(Controller,
                                  (Controller->Pci.DeviceID == PCI_DEV_HT1100_SATA) ? 3 : 5,
                                  0);
        if (!IoBase)
            return STATUS_DEVICE_CONFIGURATION_ERROR;

        /* Errata fix */
        WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x80),
                             READ_REGISTER_ULONG((PULONG)(IoBase + 0x80)) & ~0x00040000);

        /* Clear interrupts */
        WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x44), 0xFFFFFFFF);
        WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x88), 0);
    }
    else
    {
        switch (Controller->Pci.DeviceID)
        {
            case PCI_DEV_OSB4_IDE:
                /* UDMA is supported but its use is discouraged */
                SupportedMode = PIO_ALL | MWDMA_ALL;
                break;

            case PCI_DEV_CSB5_IDE:
                if (Controller->Pci.RevisionID < 0x92)
                    SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);
                else
                    SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
                break;

            case PCI_DEV_CSB6_IDE_THIRD:
                SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 4);
                break;

            default:
                SupportedMode = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 5);
                break;
        }

        if (SupportedMode & UDMA_MODE5)
            Controller->CtrlFlags |= CTRL_FLAGS_HAS_UDMA5;

        Controller->Start = SvwPataControllerStart;
    }

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->HwFlags = HwFlags;
        ChanData->TransferModeSupported = SupportedMode;

        if (HwFlags & HW_FLAGS_SATA)
        {
            if (HwFlags & HW_FLAGS_NO_ATAPI_DMA)
                ChanData->ChanInfo |= CHANNEL_FLAG_NO_ATAPI_DMA;

            /* Errata fix */
            ChanData->ChanInfo |= CHANNEL_FLAG_DMA_BEFORE_CMD;

            ChanData->ChanInfo |= CHANNEL_FLAG_NO_SLAVE;
            ChanData->SetTransferMode = SataSetTransferMode;
            ChanData->ParseResources = SvwSataParseResources;
            ChanData->ReadStatus = SvwSataReadStatus;
            ChanData->LoadTaskFile = SvwSataLoadTaskFile;
            ChanData->SaveTaskFile = SvwSataSaveTaskFile;
            // TODO: Add interrupt handler for SATA
        }
        else
        {
            ChanData->SetTransferMode = SvwSetTransferMode;

            /* 64K DMA transfers are not supported */
            if (HwFlags & HW_FLAGS_NO_64K_DMA)
                ChanData->MaximumTransferLength = PCIIDE_PRD_LIMIT - 0x4000;

            /* Check for 80-conductor cable */
            if ((ChanData->TransferModeSupported & UDMA_80C_ALL) &&
                !SvwHasUdmaCable(Controller, i))
            {
                INFO("CH %lu: BIOS hasn't selected mode faster than UDMA 2, "
                     "assume 40-conductor cable\n",
                     ChanData->Channel);
                ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
            }

            // TODO: Add interrupt handler for the CSB5
        }
    }

    return STATUS_SUCCESS;
}
