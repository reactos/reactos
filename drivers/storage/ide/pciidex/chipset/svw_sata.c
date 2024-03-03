/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     ServerWorks/Broadcom SATA controller minidriver
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Adapted from the FreeBSD ata-serverworks driver
 * Copyright (c) 1998-2008 Søren Schmidt <sos@FreeBSD.org>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_K2_SATA                     0x0240
#define PCI_DEV_BCM5770_SATA                0x0241
#define PCI_DEV_BCM5770R_SATA               0x0242
#define PCI_DEV_HT1000_SATA                 0x024A
#define PCI_DEV_HT1000_SATA_2               0x024B
#define PCI_DEV_HT1100_SATA                 0x0410
#define PCI_DEV_HT1100_SATA_2               0x0411

#define SVW_SATA_PORT_BASE_OFFSET           0x100
#define SVW_SATA_PORT_CONTROL_OFFSET        0x20
#define SVW_SATA_PORT_DMA_OFFSET            0x30
#define SVW_SATA_PORT_SCR_OFFSET            0x40
#define SVW_SATA_PORT_TF_STRIDE             4

#define HW_FLAGS_8_PORTS                    0x01
#define HW_FLAGS_NO_ATAPI_DMA               0x02

PCIIDEX_PAGED_DATA
static const struct
{
    USHORT DeviceID;
    USHORT Flags;
} SvwSataControllerList[] =
{
    { PCI_DEV_K2_SATA,       HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_BCM5770_SATA,  HW_FLAGS_NO_ATAPI_DMA | HW_FLAGS_8_PORTS },
    { PCI_DEV_BCM5770R_SATA, HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1000_SATA,   HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1000_SATA_2, HW_FLAGS_NO_ATAPI_DMA },
    { PCI_DEV_HT1100_SATA,   0 },
    { PCI_DEV_HT1100_SATA_2, 0 },
};

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
SvwSataScrRead(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ SATA_SCR_REGISTER Register,
    _In_ ULONG PortNumber,
    _In_ PULONG Result)
{
    UNREFERENCED_PARAMETER(PortNumber);

    if (Register > ATA_SCONTROL)
        return FALSE;

    *Result = READ_REGISTER_ULONG((PULONG)(ChanData->Regs.Scr + (Register * 4)));
    return TRUE;
}

static
BOOLEAN
SvwSataScrWrite(
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ SATA_SCR_REGISTER Register,
    _In_ ULONG PortNumber,
    _In_ ULONG Value)
{
    UNREFERENCED_PARAMETER(PortNumber);

    if (Register > ATA_SCONTROL)
        return FALSE;

    WRITE_REGISTER_ULONG((PULONG)(ChanData->Regs.Scr + (Register * 4)), Value);
    return TRUE;
}

static
inline
ULONG
SvwSataGetIoBarIndex(
    _In_ PATA_CONTROLLER Controller)
{
    return (Controller->Pci.DeviceID == PCI_DEV_HT1100_SATA) ? 3 : 5;
}

static
BOOLEAN
SvwSataCheckInterrupt(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    PUCHAR IoBase = Controller->AccessRange[SvwSataGetIoBarIndex(Controller)].IoBase;
    ULONG InterruptStatus;

    InterruptStatus = READ_REGISTER_ULONG((PULONG)(IoBase + 0x1F80));
    return !!(InterruptStatus & (1 << ChanData->Channel));
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
    _In_ PCHANNEL_DATA_PATA ChanData,
    _In_ PATA_DEVICE_REQUEST Request)
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
    _In_ PCHANNEL_DATA_PATA ChanData,
    _Inout_ PATA_DEVICE_REQUEST Request)
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
VOID
SvwSataParseResources(
    _Inout_ PCHANNEL_DATA_PATA ChanData,
    _In_ PUCHAR IoBase,
    _In_ ULONG PortIndex)
{
    PUCHAR ChanBase = IoBase + (PortIndex * SVW_SATA_PORT_BASE_OFFSET);

    PAGED_CODE();

    ChanData->ChanInfo |= CHANNEL_FLAG_MRES_TF | CHANNEL_FLAG_MRES_CTRL | CHANNEL_FLAG_MRES_DMA;

    ChanData->Regs.Dma = ChanBase + SVW_SATA_PORT_DMA_OFFSET;
    ChanData->Regs.Scr = ChanBase + SVW_SATA_PORT_SCR_OFFSET;

    PciIdeInitTaskFileIoResources(ChanData,
                                  (ULONG_PTR)ChanBase,
                                  (ULONG_PTR)ChanBase + SVW_SATA_PORT_CONTROL_OFFSET,
                                  SVW_SATA_PORT_TF_STRIDE);
}

CODE_SEG("PAGE")
NTSTATUS
SvwSataGetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i, HwFlags;
    PUCHAR IoBase;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_SERVERWORKS);

    for (i = 0; i < RTL_NUMBER_OF(SvwSataControllerList); ++i)
    {
        HwFlags = SvwSataControllerList[i].Flags;

        if (Controller->Pci.DeviceID == SvwSataControllerList[i].DeviceID)
            break;
    }
    if (i == RTL_NUMBER_OF(SvwSataControllerList))
        return STATUS_NO_MATCH;

    if (HwFlags & HW_FLAGS_8_PORTS)
        Controller->MaxChannels = 8;
    else
        Controller->MaxChannels = 4;

    Controller->Flags |= CTRL_FLAG_MANUAL_RES;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    IoBase = AtaCtrlPciMapBar(Controller, SvwSataGetIoBarIndex(Controller), 0);
    if (!IoBase)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    /*
     * Errata: Set SICR registers to turn off waiting for a status message
     * before sending FIS, fixes some issues with Seagate hard drives.
     */
    WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x80),
                         READ_REGISTER_ULONG((PULONG)(IoBase + 0x80)) & ~0x00040000);

    /* Clear interrupts */
    WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x44), 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)(IoBase + 0x88), 0);

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];

        ChanData->TransferModeSupported = SATA_ALL;
        ChanData->SetTransferMode = SataSetTransferMode;

        if (HwFlags & HW_FLAGS_NO_ATAPI_DMA)
            ChanData->ChanInfo |= CHANNEL_FLAG_NO_ATAPI_DMA;

        /*
         * Errata: Enable the DMA engine before sending the drive command
         * to avoid data corruption.
         */
        ChanData->ChanInfo |= CHANNEL_FLAG_DMA_BEFORE_CMD;

        ChanData->ChanInfo |= CHANNEL_FLAG_NO_SLAVE;
        ChanData->ReadStatus = SvwSataReadStatus;
        ChanData->LoadTaskFile = SvwSataLoadTaskFile;
        ChanData->SaveTaskFile = SvwSataSaveTaskFile;
        ChanData->CheckInterrupt = SvwSataCheckInterrupt;
        ChanData->ScrRead = SvwSataScrRead;
        ChanData->ScrWrite = SvwSataScrWrite;

        SvwSataParseResources(ChanData, IoBase, i);
    }

    return STATUS_SUCCESS;
}
