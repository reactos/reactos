/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Silicon Image 0680 PCI IDE controller minidriver
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* GLOBALS ********************************************************************/

#define PCI_DEV_SIL0680            0x0680

#define SIL_REG_XFER_MODE(Channel)              (0x80 + ((Channel) << 2))
#define SIL_REG_SYS_CFG                         0x8A
#define SIL_REG_CFG(Channel)                    (0xA0 + ((Channel) << 4))
#define SIL_REG_STATUS(Channel)                 (0xA1 + ((Channel) << 4))
#define SIL_REG_TF_TIM(Channel)                 (0xA2 + ((Channel) << 4))
#define SIL_REG_PIO_TIMING(Channel, Drive)      (0xA4 + ((Channel) << 4) + ((Drive) << 1))
#define SIL_REG_DMA_TIMING(Channel, Drive)      (0xA8 + ((Channel) << 4) + ((Drive) << 1))
#define SIL_REG_UDMA_TIMING(Channel, Drive)     (0xAC + ((Channel) << 4) + ((Drive) << 1))

#define SIL_CFG_CABLE_80               0x00000001
#define SIL_CFG_MONITOR_IORDY          0x00000200
#define SIL_STATUS_INTR                0x08

#define SIL_CLK_MASK              0x30
#define SIL_CLK_100MHZ            0x00
#define SIL_CLK_133MHZ            0x10
#define SIL_CLK_X2                0x20
#define SIL_CLK_DISABLED          0x30

static const USHORT Sil680TaskFileTimings[] =
{
    0x328A, // 0
    0x2283, // 1
    0x1281, // 2
    0x10C3, // 3
    0x10C1  // 4
};

static const USHORT Sil680PioTimings[] =
{
    0x328A, // 0
    0x2283, // 1
    0x1104, // 2
    0x10C3, // 3
    0x10C1  // 4
};

static const USHORT Sil680MwDmaTimings[] =
{
    0x2208, // 0
    0x10C2, // 1
    0x10C1, // 2
};

static const UCHAR Sil680UdmaTimings[2][7] =
{
    //   0     1     2     3     4     5     6
    { 0x0B, 0x07, 0x05, 0x04, 0x02, 0x01       }, // 100MHz
    { 0x0F, 0x0B, 0x07, 0x05, 0x03, 0x02, 0x01 }, // 133MHz
};

/* FUNCTIONS ******************************************************************/

static
VOID
Sil680SetTransferMode(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_CONFIG* DeviceList)
{
    ULONG i, SlowestPioMode, Config;
    UCHAR ModeReg;
    BOOLEAN MonitorIoReady = FALSE;

    INFO("CH %lu: Config (before)\n"
         "MODE %08lX\n"
         "CFG  %08lX\n"
         "PIO  %08lX\n"
         "DMA  %08lX\n"
         "UDMA %08lX\n",
         Channel,
         PciRead32(Controller, SIL_REG_XFER_MODE(Channel)),
         PciRead32(Controller, SIL_REG_CFG(Channel)),
         PciRead32(Controller, SIL_REG_PIO_TIMING(Channel, 0)),
         PciRead32(Controller, SIL_REG_DMA_TIMING(Channel, 0)),
         PciRead32(Controller, SIL_REG_UDMA_TIMING(Channel, 0)));

    SlowestPioMode = UDMA_MODE(0);
    ModeReg = PciRead8(Controller, SIL_REG_XFER_MODE(Channel));

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_CONFIG Device = DeviceList[i];
        UCHAR XferMode, SysConfig;

        if (!Device)
            continue;

        /* PIO timings */
        SlowestPioMode = min(SlowestPioMode, Device->PioMode);
        PciWrite16(Controller, SIL_REG_PIO_TIMING(Channel, i), Sil680PioTimings[Device->PioMode]);

        if (Device->PioMode >= PIO_MODE(3))
            MonitorIoReady = TRUE;

        if (Device->DmaMode >= UDMA_MODE(0))
        {
            SysConfig = PciRead8(Controller, SIL_REG_SYS_CFG);

            /* Try to enable the 133MHz IDE clock */
            if ((SysConfig & SIL_CLK_MASK) == SIL_CLK_100MHZ)
            {
                PciWrite8(Controller, SIL_REG_SYS_CFG, SysConfig | SIL_CLK_133MHZ);

                SysConfig = PciRead8(Controller, SIL_REG_SYS_CFG);
            }

            /* If we are unable to do so, skip UDMA6 selection */
            if ((Device->DmaMode == UDMA_MODE(6)) &&
                (SysConfig & SIL_CLK_MASK) == SIL_CLK_100MHZ)
            {
                ULONG SupportedModes = Device->SupportedModes & ~(PIO_ALL | UDMA_MODE6);

                if (!_BitScanReverse(&Device->DmaMode, SupportedModes))
                    Device->DmaMode = PIO_MODE(0);
            }
        }

        /* UDMA timings */
        if (Device->DmaMode >= UDMA_MODE(0))
        {
            ULONG ClockIndex = ((SysConfig & SIL_CLK_MASK) == SIL_CLK_100MHZ) ? 0 : 1;
            USHORT UdmaTim;

            XferMode = 3;

            UdmaTim = PciRead16(Controller, SIL_REG_UDMA_TIMING(Channel, i));
            UdmaTim &= ~0x003F;
            UdmaTim |= Sil680UdmaTimings[ClockIndex][Device->DmaMode - UDMA_MODE(0)];
            PciWrite16(Controller, SIL_REG_UDMA_TIMING(Channel, i), UdmaTim);
        }
        /* DMA timings */
        else if (Device->DmaMode != PIO_MODE(0))
        {
            ULONG ModeIndex = Device->DmaMode - MWDMA_MODE(0);

            XferMode = 2;

            PciWrite16(Controller, SIL_REG_DMA_TIMING(Channel, i), Sil680MwDmaTimings[ModeIndex]);
        }
        else
        {
            if (Device->IoReadySupported)
                XferMode = 1;
            else
                XferMode = 0;
        }

        ModeReg &= ~(0x03 << (i * 4));
        ModeReg |= XferMode << (i * 4);
    }

    /* Transfer mode */
    PciWrite8(Controller, SIL_REG_XFER_MODE(Channel), ModeReg);

    Config = PciRead32(Controller, SIL_REG_CFG(Channel));
    if (MonitorIoReady)
        Config |= SIL_CFG_MONITOR_IORDY;
    else
        Config &= ~SIL_CFG_MONITOR_IORDY;

    /* Task file timings */
    if (SlowestPioMode != UDMA_MODE(0))
    {
        Config &= ~0xFFFF0000;
        Config |= Sil680TaskFileTimings[SlowestPioMode] << 16;
    }

    PciWrite32(Controller, SIL_REG_CFG(Channel), Config);

    INFO("CH %lu: Config (after)\n"
         "MODE %08lX\n"
         "CFG  %08lX\n"
         "PIO  %08lX\n"
         "DMA  %08lX\n"
         "UDMA %08lX\n",
         Channel,
         PciRead32(Controller, SIL_REG_XFER_MODE(Channel)),
         Config,
         PciRead32(Controller, SIL_REG_PIO_TIMING(Channel, 0)),
         PciRead32(Controller, SIL_REG_DMA_TIMING(Channel, 0)),
         PciRead32(Controller, SIL_REG_UDMA_TIMING(Channel, 0)));
}

static
BOOLEAN
Sil680CheckInterrupt(
    _In_ PCHANNEL_DATA_PATA ChanData)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    UCHAR Status;

    Status = PciRead8(Controller, SIL_REG_STATUS(ChanData->Channel));
    return !!(Status & SIL_STATUS_INTR);
}

CODE_SEG("PAGE")
NTSTATUS
Sil680GetControllerProperties(
    _Inout_ PATA_CONTROLLER Controller)
{
    NTSTATUS Status;
    ULONG i;

    PAGED_CODE();
    ASSERT(Controller->Pci.VendorID == PCI_VEN_CMD);

    if (Controller->Pci.VendorID != PCI_DEV_SIL0680)
        return STATUS_NO_MATCH;

    Status = PciIdeCreateChannelData(Controller, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Controller->MaxChannels; ++i)
    {
        PCHANNEL_DATA_PATA ChanData = Controller->Channels[i];
        UCHAR Config;

        ChanData->CheckInterrupt = Sil680CheckInterrupt;
        ChanData->SetTransferMode = Sil680SetTransferMode;
        ChanData->TransferModeSupported = PIO_ALL | MWDMA_ALL | UDMA_MODES(0, 6);

        /* Check for 80-conductor cable */
        Config = PciRead8(Controller, SIL_REG_CFG(i));
        if (!(Config & SIL_CFG_CABLE_80))
        {
            INFO("CH %lu: BIOS detected 40-conductor cable\n", ChanData->Channel);
            ChanData->TransferModeSupported &= ~UDMA_80C_ALL;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}
