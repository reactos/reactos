/*++

Copyright (c) 2008-2010 Alexandr A. Telyatnikov (Alter)

Module Name:
    id_probe.cpp

Abstract:
    This module handles SATA-related staff

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

--*/

#include "stdafx.h"

UCHAR
NTAPI
UniataSataConnect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port /* for port multipliers */
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //ULONG Channel = deviceExtension->Channel + lChannel;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    SATA_SSTATUS_REG SStatus;
    ULONG i;
/*
    UCHAR                signatureLow,
                         signatureHigh;
*/
    UCHAR                Status;

    KdPrint2((PRINT_PREFIX "UniataSataConnect:\n"));

    if(!UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        KdPrint2((PRINT_PREFIX "  no I/O range\n"));
        return IDE_STATUS_IDLE;
    }

    /* clear SATA error register, some controllers need this */
    UniataSataWritePort4(chan, IDX_SATA_SError,
        UniataSataReadPort4(chan, IDX_SATA_SError, pm_port), pm_port);
    /* wait up to 1 second for "connect well" */
    for(i=0; i<100; i++) {
        SStatus.Reg = UniataSataReadPort4(chan, IDX_SATA_SStatus, pm_port);
        if(SStatus.SPD == SStatus_SPD_Gen1 ||
           SStatus.SPD == SStatus_SPD_Gen2) {
            deviceExtension->lun[lChannel*2].TransferMode = ATA_SA150 + (UCHAR)(SStatus.SPD - 1);
            break;
        }
        AtapiStallExecution(10000);
    }
    if(i >= 100) {
        KdPrint2((PRINT_PREFIX "UniataSataConnect: SStatus %8.8x\n", SStatus.Reg));
        return 0xff;
    }
    /* clear SATA error register */
    UniataSataWritePort4(chan, IDX_SATA_SError,
        UniataSataReadPort4(chan, IDX_SATA_SError, pm_port), pm_port);

    Status = WaitOnBaseBusyLong(chan);
    if(Status & IDE_STATUS_BUSY) {
        return Status;
    }
/*
    signatureLow = AtapiReadPort1(chan, &deviceExtension->BaseIoAddress1[lChannel].i.CylinderLow);
    signatureHigh = AtapiReadPort1(chan, &deviceExtension->baseIoAddress1[lChannel].i.CylinderHigh);

    if (signatureLow == ATAPI_MAGIC_LSB && signatureHigh == ATAPI_MAGIC_MSB) {
    }
*/
    KdPrint2((PRINT_PREFIX "UniataSataConnect: OK, ATA status %x\n", Status));
    return IDE_STATUS_IDLE;
} // end UniataSataConnect()

UCHAR
NTAPI
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port /* for port multipliers */
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    SATA_SCONTROL_REG SControl;
    int loop, retry;

    KdPrint2((PRINT_PREFIX "UniataSataPhyEnable:\n"));

    if(!UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        KdPrint2((PRINT_PREFIX "  no I/O range\n"));
        return IDE_STATUS_IDLE;
    }

    SControl.Reg = UniataSataReadPort4(chan, IDX_SATA_SControl, pm_port);
    KdPrint2((PRINT_PREFIX "SControl %x\n", SControl.Reg));
    if(SControl.DET == SControl_DET_Idle) {
        return UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
    }

    for (retry = 0; retry < 10; retry++) {
        KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: retry init %d\n", retry));
	for (loop = 0; loop < 10; loop++) {
	    SControl.Reg = 0;
	    SControl.DET = SControl_DET_Init;
            UniataSataWritePort4(chan, IDX_SATA_SControl, SControl.Reg, pm_port);
            AtapiStallExecution(100);
            SControl.Reg = UniataSataReadPort4(chan, IDX_SATA_SControl, pm_port);
            KdPrint2((PRINT_PREFIX "  SControl %8.8%x\n", SControl.Reg));
            if(SControl.DET == SControl_DET_Init) {
		break;
            }
	}
        AtapiStallExecution(5000);
        KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: retry idle %d\n", retry));
	for (loop = 0; loop < 10; loop++) {
	    SControl.Reg = 0;
	    SControl.DET = SControl_DET_DoNothing;
	    SControl.IPM = SControl_IPM_NoPartialSlumber;
            UniataSataWritePort4(chan, IDX_SATA_SControl, SControl.Reg, pm_port);
            AtapiStallExecution(100);
            SControl.Reg = UniataSataReadPort4(chan, IDX_SATA_SControl, pm_port);
            KdPrint2((PRINT_PREFIX "  SControl %8.8%x\n", SControl.Reg));
            if(SControl.DET == SControl_DET_Idle) {
                return UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
	    }
	}
    }

    KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: failed\n"));
    return 0xff;
} // end UniataSataPhyEnable()

BOOLEAN
NTAPI
UniataSataClearErr(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN BOOLEAN do_connect,
    IN ULONG pm_port /* for port multipliers */
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;
    SATA_SSTATUS_REG SStatus;
    SATA_SERROR_REG  SError;

    if(UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
    //if(ChipFlags & UNIATA_SATA) {

        SStatus.Reg = UniataSataReadPort4(chan, IDX_SATA_SStatus, pm_port);
        SError.Reg  = UniataSataReadPort4(chan, IDX_SATA_SError, pm_port); 

        if(SStatus.Reg) {
            KdPrint2((PRINT_PREFIX "  SStatus %x\n", SStatus.Reg));
        }
        if(SError.Reg) {
            KdPrint2((PRINT_PREFIX "  SError %x\n", SError.Reg));
            /* clear error bits/interrupt */
            UniataSataWritePort4(chan, IDX_SATA_SError, SError.Reg, pm_port);

            if(do_connect) {
                /* if we have a connection event deal with it */
                if(SError.DIAG.N) {
                    KdPrint2((PRINT_PREFIX "  catch SATA connect/disconnect\n"));
                    if(SStatus.SPD >= SStatus_SPD_Gen1) {
                        UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_ATTACH, pm_port);
                    } else {
                        UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_DETACH, pm_port);
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
} // end UniataSataClearErr()

BOOLEAN
NTAPI
UniataSataEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG Action,
    IN ULONG pm_port /* for port multipliers */
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR Status;
    ULONG ldev = lChannel*2 + (pm_port ? 1 : 0);

    if(!UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        return FALSE;
    }

    switch(Action) {
    case UNIATA_SATA_EVENT_ATTACH:
        KdPrint2((PRINT_PREFIX "  CONNECTED\n"));
        Status = UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
        KdPrint2((PRINT_PREFIX "  Status %x\n", Status));
        if(Status != IDE_STATUS_IDLE) {
            return FALSE;
        }
        CheckDevice(HwDeviceExtension, lChannel, pm_port ? 1 : 0 /*dev*/, FALSE);
        return TRUE;
        break;
    case UNIATA_SATA_EVENT_DETACH:
        KdPrint2((PRINT_PREFIX "  DISCONNECTED\n"));
        UniataForgetDevice(&(deviceExtension->lun[ldev]));
        return TRUE;
        break;
    }
    return FALSE;
} // end UniataSataEvent()

ULONG
UniataSataReadPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG pm_port /* for port multipliers */
    )
{
    if(chan && (io_port_ndx < IDX_MAX_REG) &&
       chan->RegTranslation[io_port_ndx].Proc) {

        PHW_DEVICE_EXTENSION deviceExtension = chan->DeviceExtension;
        PVOID HwDeviceExtension = (PVOID)deviceExtension;
        ULONG slotNumber = deviceExtension->slotNumber;
        ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
        ULONG VendorID =  deviceExtension->DevID        & 0xffff;
        ULONG offs;
        ULONG p;

        switch(VendorID) {
        case ATA_INTEL_ID: {
            p = pm_port ? 1 : 0;
            if(deviceExtension->HwFlags & ICH5) {
                offs = 0x50+chan->lun[p]->SATA_lun_map*0x10;
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += 0;
                    break;
                case IDX_SATA_SError:
                    offs += 1*4;
                    break;
                case IDX_SATA_SControl:
                    offs += 2*4;
                    break;
                default:
                    return -1;
                }
                SetPciConfig4(0xa0, offs);
                GetPciConfig4(0xa4, offs);
                return offs;
            } else {
                offs = ((deviceExtension->Channel+chan->lChannel)*2+p) * 0x100;
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += 0;
                    break;
                case IDX_SATA_SControl:
                    offs += 1;
                    break;
                case IDX_SATA_SError:
                    offs += 2;
                    break;
                default:
                    return -1;
                }
                AtapiWritePort4(chan, IDX_INDEXED_ADDR, offs);
                return AtapiReadPort4(chan, IDX_INDEXED_DATA);
            }
        } // ATA_INTEL_ID
        } // end switch(VendorID)
        return -1;
    }
    return AtapiReadPort4(chan, io_port_ndx);
} // end UniataSataReadPort4()

VOID
UniataSataWritePort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG data,
    IN ULONG pm_port /* for port multipliers */
    )
{
    if(chan && (io_port_ndx < IDX_MAX_REG) &&
       chan->RegTranslation[io_port_ndx].Proc) {

        PHW_DEVICE_EXTENSION deviceExtension = chan->DeviceExtension;
        PVOID HwDeviceExtension = (PVOID)deviceExtension;
        ULONG slotNumber = deviceExtension->slotNumber;
        ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
        ULONG VendorID =  deviceExtension->DevID        & 0xffff;
        ULONG offs;
        ULONG p;

        switch(VendorID) {
        case ATA_INTEL_ID: {
            p = pm_port ? 1 : 0;
            if(deviceExtension->HwFlags & ICH5) {
                offs = 0x50+chan->lun[p]->SATA_lun_map*0x10;
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += 0;
                    break;
                case IDX_SATA_SError:
                    offs += 1*4;
                    break;
                case IDX_SATA_SControl:
                    offs += 2*4;
                    break;
                default:
                    return;
                }
                SetPciConfig4(0xa0, offs);
                SetPciConfig4(0xa4, data);
                return;
            } else {
                offs = ((deviceExtension->Channel+chan->lChannel)*2+p) * 0x100;
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += 0;
                    break;
                case IDX_SATA_SControl:
                    offs += 1;
                    break;
                case IDX_SATA_SError:
                    offs += 2;
                    break;
                default:
                    return;
                }
                AtapiWritePort4(chan, IDX_INDEXED_ADDR, offs);
                AtapiWritePort4(chan, IDX_INDEXED_DATA, data);
            }
        } // ATA_INTEL_ID
        } // end switch(VendorID)
        return;
    }
    AtapiWritePort4(chan, io_port_ndx, data);
} // end UniataSataWritePort4()

BOOLEAN
NTAPI
UniataAhciInit(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG version;
    ULONG c, i, n;
    PHW_CHANNEL chan;
    ULONG offs;
    ULONG BaseMemAddress;
    ULONG PI;
    ULONG CAP;
    ULONG GHC;
    BOOLEAN MemIo;
    ULONGLONG base;

    /* reset AHCI controller */
    GHC = AtapiReadPortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC);
    KdPrint2((PRINT_PREFIX "  reset AHCI controller, GHC %x\n", GHC));
    AtapiWritePortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC,
        GHC | AHCI_GHC_HR);

    for(i=0; i<1000; i++) {
        AtapiStallExecution(1000);
        GHC = AtapiReadPortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC);
        KdPrint2((PRINT_PREFIX "  AHCI GHC %x\n", GHC));
        if(!(GHC & AHCI_GHC_HR)) {
            break;
        }
    }
    if(GHC & AHCI_GHC_HR) {
        KdPrint2((PRINT_PREFIX "  AHCI reset failed\n"));
        return FALSE;
    }

    /* enable AHCI mode */
    GHC = AtapiReadPortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC);
    KdPrint2((PRINT_PREFIX "  enable AHCI mode, GHC %x\n", GHC));
    AtapiWritePortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC,
        GHC | AHCI_GHC_AE);
    GHC = AtapiReadPortEx4(NULL, (ULONG_PTR)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC);
    KdPrint2((PRINT_PREFIX "  AHCI GHC %x\n", GHC));


    CAP = AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_CAP);
    PI = AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_PI);
    KdPrint2((PRINT_PREFIX "  AHCI CAP %x\n", CAP));
    if(CAP & AHCI_CAP_S64A) {
        KdPrint2((PRINT_PREFIX "  AHCI 64bit\n"));
        deviceExtension->Host64 = TRUE;
    }
    /* get the number of HW channels */
    PI = AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_PI);
    KdPrint2((PRINT_PREFIX "  AHCI PI %x\n", PI));
    for(i=PI, n=0; i; n++, i=i>>1);
    deviceExtension->NumberChannels =
        max((CAP & AHCI_CAP_NOP_MASK)+1, n);

    switch(deviceExtension->DevID) {
    case ATA_M88SX6111:
        deviceExtension->NumberChannels = 1;
        break;
    case ATA_M88SX6121:
        deviceExtension->NumberChannels = 2;
        break;
    case ATA_M88SX6141:
    case ATA_M88SX6145:
        deviceExtension->NumberChannels = 4;
        break;
    } // switch()

    /* clear interrupts */
    AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_IS,
        AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_IS));

    /* enable AHCI interrupts */
    AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_GHC,
        AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_GHC) | AHCI_GHC_IE);

    version = AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_VS);
        KdPrint2((PRINT_PREFIX "  AHCI version %x.%02x controller with %d ports (mask %x) detected\n",
		  ((version >> 20) & 0xf0) + ((version >> 16) & 0x0f),
		  ((version >> 4) & 0xf0) + (version & 0x0f),
		  deviceExtension->NumberChannels, PI));

    KdPrint2((PRINT_PREFIX "  PM%s supported\n",
		  CAP & AHCI_CAP_SPM ? "" : " not"));

    deviceExtension->HwFlags |= UNIATA_SATA;
    deviceExtension->HwFlags |= UNIATA_AHCI;

    BaseMemAddress = deviceExtension->BaseIoAHCI_0.Addr;
    MemIo          = deviceExtension->BaseIoAHCI_0.MemIo;

    for(c=0; c<deviceExtension->NumberChannels; c++) {
        chan = &deviceExtension->chan[c];
        offs = sizeof(IDE_AHCI_REGISTERS) + c*sizeof(IDE_AHCI_PORT_REGISTERS);

        chan->RegTranslation[IDX_IO1_i_Status      ].Addr       = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, TFD.STS);
        chan->RegTranslation[IDX_IO1_i_Status      ].MemIo      = MemIo;
        chan->RegTranslation[IDX_IO2_AltStatus] = chan->RegTranslation[IDX_IO1_i_Status];
        chan->RegTranslation[IDX_IO1_i_Error       ].Addr       = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, TFD.ERR);
        chan->RegTranslation[IDX_IO1_i_Error       ].MemIo      = MemIo;
        chan->RegTranslation[IDX_IO1_i_CylinderLow ].Addr       = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SIG.LbaLow);
        chan->RegTranslation[IDX_IO1_i_CylinderLow ].MemIo      = MemIo;
        chan->RegTranslation[IDX_IO1_i_CylinderHigh].Addr       = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SIG.LbaHigh);
        chan->RegTranslation[IDX_IO1_i_CylinderHigh].MemIo      = MemIo;
        chan->RegTranslation[IDX_IO1_i_BlockCount  ].Addr       = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SIG.SectorCount);
        chan->RegTranslation[IDX_IO1_i_BlockCount  ].MemIo      = MemIo;

        UniataInitSyncBaseIO(chan);

        chan->RegTranslation[IDX_SATA_SStatus].Addr   = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SSTS);
        chan->RegTranslation[IDX_SATA_SStatus].MemIo  = MemIo;
        chan->RegTranslation[IDX_SATA_SError].Addr    = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SERR);
        chan->RegTranslation[IDX_SATA_SError].MemIo   = MemIo;
        chan->RegTranslation[IDX_SATA_SControl].Addr  = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SCTL);
        chan->RegTranslation[IDX_SATA_SControl].MemIo = MemIo;
        chan->RegTranslation[IDX_SATA_SActive].Addr   = BaseMemAddress + offs + FIELD_OFFSET(IDE_AHCI_PORT_REGISTERS, SACT);
        chan->RegTranslation[IDX_SATA_SActive].MemIo  = MemIo;

        AtapiDmaAlloc(HwDeviceExtension, NULL, c);

        base = chan->AHCI_CL_PhAddr;
        if(!base) {
            KdPrint2((PRINT_PREFIX "  AHCI buffer allocation failed\n"));
            return FALSE;
        }
        AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), offs + IDX_AHCI_P_CLB,
            (ULONG)(base & 0xffffffff));
        AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), offs + IDX_AHCI_P_CLB + 4,
            (ULONG)((base >> 32) & 0xffffffff));

        base = chan->AHCI_CL_PhAddr + ATA_AHCI_MAX_TAGS;
        AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), offs + IDX_AHCI_P_FB,
            (ULONG)(base & 0xffffffff));
        AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), offs + IDX_AHCI_P_FB + 4,
            (ULONG)((base >> 32) & 0xffffffff));

        chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
    }

    return TRUE;
} // end UniataAhciInit()

UCHAR
NTAPI
UniataAhciStatus(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    ULONG Channel = deviceExtension->Channel + lChannel;
    ULONG            hIS;
    ULONG            CI;
    AHCI_IS_REG      IS;
    SATA_SSTATUS_REG SStatus;
    SATA_SERROR_REG  SError;
    ULONG offs = sizeof(IDE_AHCI_REGISTERS) + Channel*sizeof(IDE_AHCI_PORT_REGISTERS);
    ULONG_PTR base;
    ULONG tag=0;

    KdPrint(("UniataAhciStatus:\n"));

    hIS = AtapiReadPortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_IS);
    KdPrint((" hIS %x\n", hIS));
    hIS &= (1 << Channel);
    if(!hIS) {
        return 0;
    }
    base = (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0 + offs);
    IS.Reg      = AtapiReadPort4(chan, base + IDX_AHCI_P_IS);
    CI          = AtapiReadPort4(chan, base + IDX_AHCI_P_CI);
    SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
    SError.Reg  = AtapiReadPort4(chan, IDX_SATA_SError); 

    /* clear interrupt(s) */
    AtapiWritePortEx4(NULL, (ULONG_PTR)(&deviceExtension->BaseIoAHCI_0), IDX_AHCI_IS, hIS);
    AtapiWritePort4(chan, base + IDX_AHCI_P_IS, IS.Reg);
    AtapiWritePort4(chan, IDX_SATA_SError, SError.Reg);

    KdPrint((" AHCI: status=%08x sstatus=%08x error=%08x CI=%08x\n",
	   IS.Reg, SStatus.Reg, SError.Reg, CI));

    /* do we have cold connect surprise */
    if(IS.CPDS) {
    }

    /* check for and handle connect events */
    if(IS.PCS) {
        UniataSataEvent(HwDeviceExtension, lChannel, UNIATA_SATA_EVENT_ATTACH);
    }
    if(IS.PRCS) {
        UniataSataEvent(HwDeviceExtension, lChannel, UNIATA_SATA_EVENT_DETACH);
    }
    if(CI & (1 << tag)) {
        return 1;
    }
    KdPrint((" AHCI: unexpected\n"));
    return 2;

} // end UniataAhciStatus()

ULONG
NTAPI
UniataAhciSetupFIS(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
   OUT PUCHAR fis,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature,
    IN ULONG flags
    )
{
    ULONG ldev = lChannel*2 + DeviceNumber;
    ULONG i;
    PUCHAR plba;

    KdPrint2((PRINT_PREFIX "  AHCI setup FIS\n" ));
    i = 0;
    plba = (PUCHAR)&lba;

    if((AtaCommandFlags[command] & ATA_CMD_FLAG_LBAIOsupp) &&
       CheckIfBadBlock(&(deviceExtension->lun[ldev]), lba, count)) {
        KdPrint3((PRINT_PREFIX ": artificial bad block, lba %#I64x count %#x\n", lba, count));
        return IDE_STATUS_ERROR;
        //return SRB_STATUS_ERROR;
    }

    /* translate command into 48bit version */
    if ((lba >= ATA_MAX_LBA28 || count > 256) &&
        deviceExtension->lun[ldev].IdentifyData.FeaturesSupport.Address48) {
        if(AtaCommandFlags[command] & ATA_CMD_FLAG_48supp) {
            command = AtaCommands48[command];
        } else {
            KdPrint2((PRINT_PREFIX "  unhandled LBA48 command\n"));
            return 0;
        }
    }

    fis[0] = 0x27;  /* host to device */
    fis[1] = 0x80;  /* command FIS (note PM goes here) */
    fis[2] = command;
    fis[3] = (UCHAR)feature;

    fis[4] = plba[0];
    fis[5] = plba[1];
    fis[6] = plba[2];
    fis[7] = IDE_USE_LBA | (DeviceNumber ? IDE_DRIVE_2 : IDE_DRIVE_1);
    if ((lba >= ATA_MAX_LBA28 || count > 256) &&
        deviceExtension->lun[ldev].IdentifyData.FeaturesSupport.Address48) {
        i++;
    } else {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4333) // right shift by too large amount, data loss
#endif
        fis[7] |= (plba[3] >> 24) & 0x0f;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    fis[8] = plba[3];
    fis[9] = plba[4];
    fis[10] = plba[5]; 
    fis[11] = (UCHAR)(feature>>8) & 0xff;

    fis[12] = (UCHAR)count & 0xff;
    fis[13] = (UCHAR)(count>>8) & 0xff;
    fis[14] = 0x00;
    fis[15] = IDE_DC_A_4BIT;

    fis[16] = 0x00;
    fis[17] = 0x00;
    fis[18] = 0x00;
    fis[19] = 0x00;
    return 20;
} // end UniataAhciSetupFIS()
