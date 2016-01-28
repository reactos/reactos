/*++

Copyright (c) 2008-2014 Alexandr A. Telyatnikov (Alter)

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
           SStatus.SPD == SStatus_SPD_Gen2 ||
           SStatus.SPD == SStatus_SPD_Gen3) {
            // SATA sets actual transfer rate in LunExt on init.
            // There is no run-time SATA rate adjustment yet.
            // On the other hand, we may turn SATA device in PIO mode
            // TODO: make separate states for interface speed and transfer mode (DMA vs PIO)
            chan->lun[0]->LimitedTransferMode =
            chan->lun[0]->PhyTransferMode =
            chan->lun[0]->TransferMode = ATA_SA150 + (UCHAR)(SStatus.SPD - 1);

            KdPrint2((PRINT_PREFIX "SATA TransferMode %#x\n", chan->lun[0]->TransferMode));
            if(chan->MaxTransferMode < chan->lun[0]->TransferMode) {
                KdPrint2((PRINT_PREFIX "SATA upd chan TransferMode\n"));
                chan->MaxTransferMode = chan->lun[0]->TransferMode;
            }
            if(deviceExtension->MaxTransferMode < chan->lun[0]->TransferMode) {
                KdPrint2((PRINT_PREFIX "SATA upd controller TransferMode\n"));
                deviceExtension->MaxTransferMode = chan->lun[0]->TransferMode;
            }

            break;
        }
        AtapiStallExecution(10000);
    }
    if(i >= 100) {
        KdPrint2((PRINT_PREFIX "UniataSataConnect: SStatus %8.8x\n", SStatus.Reg));
        return IDE_STATUS_WRONG;
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
    KdPrint2((PRINT_PREFIX "UniataSataConnect: OK, ATA status %#x\n", Status));
    return IDE_STATUS_IDLE;
} // end UniataSataConnect()

UCHAR
NTAPI
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port, /* for port multipliers */
    IN BOOLEAN doReset
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
    KdPrint2((PRINT_PREFIX "SControl %#x\n", SControl.Reg));
    if(SControl.DET == SControl_DET_Idle) {
        if(!doReset) {
            return UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
        }
    }

    for (retry = 0; retry < 10; retry++) {
        KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: retry init %d\n", retry));
	for (loop = 0; loop < 10; loop++) {
	    SControl.Reg = 0;
	    SControl.DET = SControl_DET_Init;
            UniataSataWritePort4(chan, IDX_SATA_SControl, SControl.Reg, pm_port);
            AtapiStallExecution(100);
            SControl.Reg = UniataSataReadPort4(chan, IDX_SATA_SControl, pm_port);
            KdPrint2((PRINT_PREFIX "  SControl %8.8x\n", SControl.Reg));
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
            KdPrint2((PRINT_PREFIX "  SControl %8.8x\n", SControl.Reg));
            if(SControl.DET == SControl_DET_Idle) {
                return UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
	    }
	}
    }

    KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: failed\n"));
    return IDE_STATUS_WRONG;
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
            KdPrint2((PRINT_PREFIX "  SStatus %#x\n", SStatus.Reg));
        }
        if(SError.Reg) {
            KdPrint2((PRINT_PREFIX "  SError %#x\n", SError.Reg));
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
            //return TRUE;
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
    ULONG DeviceNumber = (pm_port ? 1 : 0);

    if(!UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        return FALSE;
    }

    switch(Action) {
    case UNIATA_SATA_EVENT_ATTACH:
        KdPrint2((PRINT_PREFIX "  CONNECTED\n"));
        Status = UniataSataConnect(HwDeviceExtension, lChannel, pm_port);
        KdPrint2((PRINT_PREFIX "  Status %#x\n", Status));
        if(Status != IDE_STATUS_IDLE) {
            return FALSE;
        }
        CheckDevice(HwDeviceExtension, lChannel, DeviceNumber /*dev*/, FALSE);
        return TRUE;
        break;
    case UNIATA_SATA_EVENT_DETACH:
        KdPrint2((PRINT_PREFIX "  DISCONNECTED\n"));
        UniataForgetDevice(deviceExtension->chan[lChannel].lun[DeviceNumber]);
        return TRUE;
        break;
    }
    return FALSE;
} // end UniataSataEvent()

ULONG
NTAPI
UniataSataReadPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG pm_port /* for port multipliers */
    )
{
    if(chan && (io_port_ndx < IDX_MAX_REG) &&
       chan->RegTranslation[io_port_ndx].Proc) {

        KdPrint3((PRINT_PREFIX "  UniataSataReadPort4 %#x[%d]\n", io_port_ndx, pm_port));

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
                KdPrint3((PRINT_PREFIX "  ICH5 way, offs %#x\n", offs));
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
            } else 
            if(deviceExtension->HwFlags & ICH7) {
                offs = 0x100+chan->lun[p]->SATA_lun_map*0x80;
                KdPrint3((PRINT_PREFIX "  ICH7 way, offs %#x\n", offs));
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += IDX_AHCI_P_SStatus;
                    break;
                case IDX_SATA_SError:
                    offs += IDX_AHCI_P_SError;
                    break;
                case IDX_SATA_SControl:
                    offs += IDX_AHCI_P_SControl;
                    break;
                default:
                    return -1;
                }
                return AtapiReadPortEx4(NULL, (ULONGIO_PTR)(&deviceExtension->BaseIoAddressSATA_0), offs);
            } else {
                offs = ((deviceExtension->Channel+chan->lChannel)*2+p) * 0x100;
                KdPrint3((PRINT_PREFIX "  def way, offs %#x\n", offs));
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
NTAPI
UniataSataWritePort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG data,
    IN ULONG pm_port /* for port multipliers */
    )
{
    if(chan && (io_port_ndx < IDX_MAX_REG) &&
       chan->RegTranslation[io_port_ndx].Proc) {

        KdPrint3((PRINT_PREFIX "  UniataSataWritePort4 %#x[%d]\n", io_port_ndx, pm_port));

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
                KdPrint3((PRINT_PREFIX "  ICH5 way, offs %#x\n", offs));
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
            } else 
            if(deviceExtension->HwFlags & ICH7) {
                offs = 0x100+chan->lun[p]->SATA_lun_map*0x80;
                KdPrint3((PRINT_PREFIX "  ICH7 way, offs %#x\n", offs));
                switch(io_port_ndx) {
                case IDX_SATA_SStatus:
                    offs += IDX_AHCI_P_SStatus;
                    break;
                case IDX_SATA_SError:
                    offs += IDX_AHCI_P_SError;
                    break;
                case IDX_SATA_SControl:
                    offs += IDX_AHCI_P_SControl;
                    break;
                default:
                    return;
                }
                AtapiWritePortEx4(NULL, (ULONGIO_PTR)(&deviceExtension->BaseIoAddressSATA_0), offs, data);
                return;
            } else {
                offs = ((deviceExtension->Channel+chan->lChannel)*2+p) * 0x100;
                KdPrint3((PRINT_PREFIX "  def way, offs %#x\n", offs));
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
UniataSataReadPM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
   OUT PULONG result
    )
{
    if(chan->DeviceExtension->HwFlags & UNIATA_AHCI) {
        return UniataAhciReadPM(chan, DeviceNumber, Reg, result);
    }
    return FALSE;
} // end UniataSataReadPM()

UCHAR
NTAPI
UniataSataWritePM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
    IN ULONG value
    )
{
    if(chan->DeviceExtension->HwFlags & UNIATA_AHCI) {
        return UniataAhciWritePM(chan, DeviceNumber, Reg, value);
    }
    return IDE_STATUS_WRONG;
} // end UniataSataWritePM()

ULONG
NTAPI
UniataSataSoftReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;

    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        return UniataAhciSoftReset(HwDeviceExtension, lChannel, DeviceNumber);
    }
    return 0xffffffff;
} // end UniataSataSoftReset()

VOID
UniataSataIdentifyPM(
    IN PHW_CHANNEL chan
    )
{
    ULONG PM_DeviceId;
    ULONG PM_RevId;
    ULONG PM_Ports;
    UCHAR i;
    ULONG signature;
    PHW_LU_EXTENSION     LunExt;

    KdPrint((PRINT_PREFIX "UniataSataIdentifyPM:\n"));

    chan->PmLunMap = 0;

    /* get PM vendor & product data */
    if(!UniataSataReadPM(chan, AHCI_DEV_SEL_PM, 0, &PM_DeviceId)) {
        KdPrint2((PRINT_PREFIX "  error getting PM vendor data\n"));
	return;
    }
    /* get PM revision data */
    if(!UniataSataReadPM(chan, AHCI_DEV_SEL_PM, 1, &PM_RevId)) {
        KdPrint2((PRINT_PREFIX "  error getting PM revison data\n"));
	return;
    }
    /* get number of HW ports on the PM */
    if(!UniataSataReadPM(chan, AHCI_DEV_SEL_PM, 2, &PM_Ports)) {
        KdPrint2((PRINT_PREFIX "  error getting PM port info\n"));
	return;
    }

    PM_Ports &= 0x0000000f;

    switch(PM_DeviceId) {
    case 0x37261095:
        /* This PM declares 6 ports, while only 5 of them are real.
         * Port 5 is enclosure management bridge port, which has implementation
         * problems, causing probe faults. Hide it for now. */
        KdPrint2((PRINT_PREFIX "  SiI 3726 (rev=%#x) Port Multiplier with %d (5) ports\n",
            PM_RevId, PM_Ports));
        PM_Ports = 5;
        break;
    case 0x47261095:
        /* This PM declares 7 ports, while only 5 of them are real.
         * Port 5 is some fake "Config  Disk" with 640 sectors size,
         * port 6 is enclosure management bridge port.
         * Both fake ports has implementation problems, causing
         * probe faults. Hide them for now. */
        KdPrint2((PRINT_PREFIX "  SiI 4726 (rev=%#x) Port Multiplier with %d (5) ports\n",
            PM_RevId, PM_Ports));
        PM_Ports = 5;
        break;
    default:
        KdPrint2((PRINT_PREFIX "  Port Multiplier (id=%08x rev=%#x) with %d ports\n",
            PM_DeviceId, PM_RevId, PM_Ports));
        break;
    }

    // reset
    for(i=0; i<PM_Ports; i++) {

        LunExt = chan->lun[i];

        KdPrint2((PRINT_PREFIX "    Port %d\n", i));
        if(UniataSataPhyEnable(chan->DeviceExtension, chan->lChannel, i, UNIATA_SATA_RESET_ENABLE) != IDE_STATUS_IDLE) {
            LunExt->DeviceFlags &= ~DFLAGS_DEVICE_PRESENT;
            continue;
        }
	/*
	 * XXX: I have no idea how to properly wait for PMP port hardreset
	 * completion. Without this delay soft reset does not completes
	 * successfully.
	 */
        AtapiStallExecution(1000000);

        signature = UniataSataSoftReset(chan->DeviceExtension, chan->lChannel, i);
        KdPrint2((PRINT_PREFIX "  signature %#x\n", signature));

        LunExt->DeviceFlags |= DFLAGS_DEVICE_PRESENT;
        chan->PmLunMap |= (1 << i);
	/* figure out whats there */
	switch (signature >> 16) {
	case 0x0000:
            LunExt->DeviceFlags &= ~DFLAGS_ATAPI_DEVICE;
	    continue;
	case 0xeb14:
            LunExt->DeviceFlags |= DFLAGS_ATAPI_DEVICE;
	    continue;
	}

    }

} // end UniataSataIdentifyPM()

#ifdef _DEBUG
VOID
NTAPI
UniataDumpAhciRegs(
    IN PHW_DEVICE_EXTENSION deviceExtension
    )
{
    ULONG                j;
    ULONG                xReg;

    KdPrint2((PRINT_PREFIX 
               "  AHCI Base: %#x MemIo %d Proc %d\n",
               deviceExtension->BaseIoAHCI_0.Addr,
               deviceExtension->BaseIoAHCI_0.MemIo,
               deviceExtension->BaseIoAHCI_0.Proc));

    for(j=0; j<=IDX_AHCI_VS; j+=sizeof(ULONG)) {
        xReg = AtapiReadPortEx4(NULL, (ULONGIO_PTR)&deviceExtension->BaseIoAHCI_0, j);
        KdPrint2((PRINT_PREFIX 
                   "  AHCI_%#x (%#x) = %#x\n",
                   j,
                   (deviceExtension->BaseIoAHCI_0.Addr+j),
                   xReg));
    }
    return;
} // end UniataDumpAhciRegs()


VOID
NTAPI
UniataDumpAhciPortRegs(
    IN PHW_CHANNEL chan
    )
{
    ULONG                j;
    ULONG                xReg;

    KdPrint2((PRINT_PREFIX 
               "  AHCI port %d Base: %#x MemIo %d Proc %d\n",
               chan->lChannel,
               chan->BaseIoAHCI_Port.Addr,
               chan->BaseIoAHCI_Port.MemIo,
               chan->BaseIoAHCI_Port.Proc));

    for(j=0; j<=IDX_AHCI_P_SNTF; j+=sizeof(ULONG)) {
        xReg = AtapiReadPortEx4(NULL, (ULONGIO_PTR)&chan->BaseIoAHCI_Port, j);
        KdPrint2((PRINT_PREFIX 
                   "  AHCI%d_%#x (%#x) = %#x\n",
                   chan->lChannel,
                   j,
                   (chan->BaseIoAHCI_Port.Addr+j),
                   xReg));
    }
    return;
} // end UniataDumpAhciPortRegs()
#endif //DBG


BOOLEAN
NTAPI
UniataAhciInit(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG c, i;
    PHW_CHANNEL chan;
    ULONG offs;
    ULONG BaseMemAddress;
    ULONG PI;
    ULONG CAP;
    ULONG CAP2;
    ULONG BOHC;
    ULONG GHC;
    BOOLEAN MemIo = FALSE;

    KdPrint2((PRINT_PREFIX "  UniataAhciInit:\n"));

#ifdef _DEBUG
    UniataDumpAhciRegs(deviceExtension);
#endif //DBG

    CAP2 = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP2);
    if(CAP2 & AHCI_CAP2_BOH) {
        BOHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_BOHC);
        KdPrint2((PRINT_PREFIX "  stage 1 BOHC %#x\n", BOHC));
        UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_BOHC,
            BOHC | AHCI_BOHC_OOS);
        for(i=0; i<50; i++) {
            AtapiStallExecution(500);
            BOHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_BOHC);
            KdPrint2((PRINT_PREFIX "  BOHC %#x\n", BOHC));
            if(BOHC & AHCI_BOHC_BB) {
                break;
            }
            if(!(BOHC & AHCI_BOHC_BOS)) {
                break;
            }
        }
        KdPrint2((PRINT_PREFIX "  stage 2 BOHC %#x\n", BOHC));
        if(BOHC & AHCI_BOHC_BB) {
            for(i=0; i<2000; i++) {
                AtapiStallExecution(1000);
                BOHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_BOHC);
                KdPrint2((PRINT_PREFIX "  BOHC %#x\n", BOHC));
                if(!(BOHC & AHCI_BOHC_BOS)) {
                    break;
                }
            }
        }
        KdPrint2((PRINT_PREFIX "  final BOHC %#x\n", BOHC));
    }

    /* disable AHCI interrupts, for MSI compatibility issue
       see http://www.intel.com/Assets/PDF/specupdate/307014.pdf
       26. AHCI Reset and MSI Request
    */

    KdPrint2((PRINT_PREFIX "  get GHC\n"));
    /* enable AHCI mode */
    GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
    if(!(GHC & AHCI_GHC_AE)) {
        KdPrint2((PRINT_PREFIX "  enable AHCI mode, disable intr, GHC %#x\n", GHC));
        UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
            (GHC | AHCI_GHC_AE) & ~AHCI_GHC_IE);
    } else {
        KdPrint2((PRINT_PREFIX "  disable intr, GHC %#x\n", GHC));
        UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
            GHC & ~AHCI_GHC_IE);
    }
    AtapiStallExecution(100);

    /* read GHC again and reset AHCI controller */
    GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
    KdPrint2((PRINT_PREFIX "  reset AHCI controller, GHC %#x\n", GHC));
    UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
        GHC | AHCI_GHC_HR);

    for(i=0; i<1000; i++) {
        AtapiStallExecution(1000);
        GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
        KdPrint2((PRINT_PREFIX "  AHCI GHC %#x\n", GHC));
        if(!(GHC & AHCI_GHC_HR)) {
            break;
        }
    }
    if(GHC & AHCI_GHC_HR) {
        KdPrint2((PRINT_PREFIX "  AHCI reset failed\n"));
        return FALSE;
    }

    /* re-enable AHCI mode */
    /* Linux: Some controllers need AHCI_EN to be written multiple times.
     * Try a few times before giving up.
     */
    GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
    for(i=0; i<5; i++) {
        if(!(GHC & AHCI_GHC_AE)) {
            KdPrint2((PRINT_PREFIX "  re-enable AHCI mode, GHC %#x\n", GHC));
            UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
                GHC | AHCI_GHC_AE);
            AtapiStallExecution(1000);
            GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
        } else {
            break;
        }
    }
    KdPrint2((PRINT_PREFIX "  AHCI GHC %#x\n", GHC));
    if(!(GHC & AHCI_GHC_AE)) {
        KdPrint2((PRINT_PREFIX "  Can't enable AHCI mode\n"));
        return FALSE;
    }

    deviceExtension->AHCI_CAP =
      CAP = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP);
    KdPrint2((PRINT_PREFIX "  AHCI CAP %#x\n", CAP));
    if(CAP & AHCI_CAP_S64A) {
        KdPrint2((PRINT_PREFIX "  AHCI 64bit\n"));
        deviceExtension->Host64 = TRUE;
    }
    KdPrint2((PRINT_PREFIX "  AHCI %d CMD slots\n", (CAP & AHCI_CAP_NCS_MASK) >> 8 ));
    if(CAP & AHCI_CAP_PMD) {
        KdPrint2((PRINT_PREFIX "  AHCI multi-block PIO\n"));
    }
    if(CAP & AHCI_CAP_SAM) {
        KdPrint2((PRINT_PREFIX "  AHCI legasy SATA\n"));
    }

    /* get the number of HW channels */
    PI = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_PI);
    deviceExtension->AHCI_PI = PI;
    KdPrint2((PRINT_PREFIX "  AHCI PI %#x\n", PI));
    KdPrint2((PRINT_PREFIX "  AHCI PI mask %#x\n", deviceExtension->AHCI_PI_mask));
    deviceExtension->AHCI_PI = PI = PI & deviceExtension->AHCI_PI_mask;
    KdPrint2((PRINT_PREFIX "  masked AHCI PI %#x\n", PI));

    CAP2 = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP2);
    if(CAP2 & AHCI_CAP2_BOH) {
        KdPrint2((PRINT_PREFIX "  retry BOHC\n"));
        BOHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_BOHC);
        KdPrint2((PRINT_PREFIX "  BOHC %#x\n", BOHC));
        UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_BOHC,
            BOHC | AHCI_BOHC_OOS);
    }
    /* clear interrupts */
    UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_IS,
        UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_IS));

    /* enable AHCI interrupts */
    UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
        UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC) | AHCI_GHC_IE);

    BaseMemAddress = deviceExtension->BaseIoAHCI_0.Addr;
    MemIo          = deviceExtension->BaseIoAHCI_0.MemIo;

    deviceExtension->MaxTransferMode = ATA_SA150+(((CAP & AHCI_CAP_ISS_MASK) >> 20)-1);
    KdPrint2((PRINT_PREFIX "  SATA Gen %d\n", ((CAP & AHCI_CAP_ISS_MASK) >> 20) ));

    for(c=0; c<deviceExtension->NumberChannels; c++) {
        chan = &deviceExtension->chan[c];
        offs = sizeof(IDE_AHCI_REGISTERS) + c*sizeof(IDE_AHCI_PORT_REGISTERS);

        KdPrint2((PRINT_PREFIX "  chan %d, offs %#x\n", c, offs));

        chan->MaxTransferMode = deviceExtension->MaxTransferMode;

        AtapiSetupLunPtrs(chan, deviceExtension, c);

        chan->BaseIoAHCI_Port = deviceExtension->BaseIoAHCI_0;
        chan->BaseIoAHCI_Port.Addr = BaseMemAddress + offs;

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

        if(!UniataAhciChanImplemented(deviceExtension, c)) {
            KdPrint2((PRINT_PREFIX "  chan %d not implemented\n", c));
            continue;
        }

        UniataAhciResume(chan);

        chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
    }

    return TRUE;
} // end UniataAhciInit()

BOOLEAN
NTAPI
UniAtaAhciValidateVersion(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG version,
    IN BOOLEAN Strict
    )
{
    switch(version) {
    case 0x00000000:
    case 0xffffffff:
        KdPrint(("  wrong AHCI revision %#x\n", version));
        return FALSE;
    case 0x00000905:
    case 0x00010000:
    case 0x00010100:
    case 0x00010200:
    case 0x00010300:
        break;
    default:
        KdPrint2((PRINT_PREFIX "  Unknown AHCI revision\n"));
        if(AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"CheckAhciRevision", Strict)) {
            KdPrint(("  AHCI revision excluded\n"));
            return FALSE;
        }
    }
    return TRUE;
} // end UniAtaAhciValidateVersion()

BOOLEAN
NTAPI
UniataAhciDetect(
    IN PVOID HwDeviceExtension,
    IN PPCI_COMMON_CONFIG pciData, // optional
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG version;
    ULONG i, n;
    ULONG PI;
    ULONG CAP;
    ULONG CAP2;
    ULONG GHC, GHC0;
#ifdef _DEBUG
    ULONG BOHC;
    ULONG v_Mn, v_Mj;
#endif //DBG
    ULONG NumberChannels;
    ULONG BaseMemAddress;
    BOOLEAN MemIo = FALSE;
    BOOLEAN found = FALSE;

    KdPrint2((PRINT_PREFIX "  UniataAhciDetect:\n"));

    if(AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreAhci", 0)) {
        KdPrint(("  AHCI excluded\n"));
        return FALSE;
    }
    BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                            5, 0, 0x10);
    if(!BaseMemAddress) {
        KdPrint2((PRINT_PREFIX "  AHCI init failed - no IoRange\n"));
        return FALSE;
    }
    if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
        KdPrint2((PRINT_PREFIX "MemIo\n"));
        MemIo = TRUE;
    }
    deviceExtension->BaseIoAHCI_0.Addr  = BaseMemAddress;
    deviceExtension->BaseIoAHCI_0.MemIo = MemIo;

#ifdef _DEBUG
    UniataDumpAhciRegs(deviceExtension);
#endif //DBG

    GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
    if(GHC & AHCI_GHC_HR) {
        KdPrint2((PRINT_PREFIX "  AHCI in reset state\n"));
        return FALSE;
    }

    /* check AHCI mode. Save state and try enable */
    GHC0 = 
    GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);
    KdPrint2((PRINT_PREFIX "  check AHCI mode, GHC %#x\n", GHC));

    version = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_VS);

    if(!(GHC & AHCI_GHC_AE)) {
        KdPrint2((PRINT_PREFIX "  Non-AHCI GHC (!AE), check revision %#x\n", version));
        if(!UniAtaAhciValidateVersion(deviceExtension, version, FALSE)) {
            KdPrint2((PRINT_PREFIX "  Non-AHCI\n"));
            goto exit_detect;
        }
        KdPrint2((PRINT_PREFIX "  try enable\n"));
        UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC,
            (GHC | AHCI_GHC_AE) & ~AHCI_GHC_IE);
        GHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_GHC);

        KdPrint2((PRINT_PREFIX "  re-check AHCI mode, GHC %#x\n", GHC));
        if(!(GHC & AHCI_GHC_AE)) {
            KdPrint2((PRINT_PREFIX "  Non-AHCI GHC (!AE)\n"));
            goto exit_detect;
        }
    }

    CAP = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP);
    CAP2 = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP2);
    KdPrint2((PRINT_PREFIX "  AHCI CAP %#x, CAP2 %#x\n", CAP, CAP2));
    if(CAP & AHCI_CAP_S64A) {
        KdPrint2((PRINT_PREFIX "  64bit"));
        //deviceExtension->Host64 = TRUE; // this is just DETECT, do not update anything
    }
#ifdef _DEBUG
    if(CAP2 & AHCI_CAP2_BOH) {
        BOHC = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_BOHC);
        KdPrint2((PRINT_PREFIX "  BOHC %#x", BOHC));
    }
#endif //DBG
    if(CAP & AHCI_CAP_NCQ) {
        KdPrint2((PRINT_PREFIX "  NCQ"));
    }
    if(CAP & AHCI_CAP_SNTF) {
        KdPrint2((PRINT_PREFIX "  SNTF"));
    }
    if(CAP & AHCI_CAP_CCC) {
        KdPrint2((PRINT_PREFIX "  CCC"));
    }
    KdPrint2((PRINT_PREFIX "\n"));

    /* get the number of HW channels */
    
    /* CAP.NOP sometimes indicate the index of the last enabled
     * port, at other times, that of the last possible port, so
     * determining the maximum port number requires looking at
     * both CAP.NOP and PI.
     */
    PI = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_PI);
    deviceExtension->AHCI_PI = deviceExtension->AHCI_PI_mask = PI;
    KdPrint2((PRINT_PREFIX "  AHCI PI %#x\n", PI));

    for(i=PI, n=0; i; n++, i=i>>1) {
        if(AtapiRegCheckDevValue(deviceExtension, n, DEVNUM_NOT_SPECIFIED, L"Exclude", 0)) {
            KdPrint2((PRINT_PREFIX "Channel %d excluded\n", n));
            deviceExtension->AHCI_PI &= ~((ULONG)1 << n);
            deviceExtension->AHCI_PI_mask &= ~((ULONG)1 << n);
        }
    }
    deviceExtension->AHCI_PI_mask = 
        AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"PortMask", deviceExtension->AHCI_PI_mask);
    KdPrint2((PRINT_PREFIX "Force PortMask %#x\n", deviceExtension->AHCI_PI_mask));

    for(i=PI, n=0; i; n++, i=i>>1);
    NumberChannels =
        max((CAP & AHCI_CAP_NOP_MASK)+1, n);

    KdPrint2((PRINT_PREFIX "  CommandSlots %d\n", (CAP & AHCI_CAP_NCS_MASK)>>8 ));
    KdPrint2((PRINT_PREFIX "  Detected Channels %d / %d\n", NumberChannels, n));

    switch(deviceExtension->DevID) {
    case ATA_M88SE6111:
        KdPrint2((PRINT_PREFIX "  Marvell M88SE6111 -> 1\n"));
        NumberChannels = 1;
        break;
    case ATA_M88SE6121:
        KdPrint2((PRINT_PREFIX "  Marvell M88SE6121 -> 2\n"));
        NumberChannels = min(NumberChannels, 2);
        break;
    case ATA_M88SE6141:
    case ATA_M88SE6145:
    case ATA_M88SE9123:
        KdPrint2((PRINT_PREFIX "  Marvell M88SE614x/9123 -> 4\n"));
        NumberChannels = min(NumberChannels, 4);
        break;
    } // switch()

    if(!NumberChannels) {
        KdPrint2((PRINT_PREFIX "  Non-AHCI - NumberChannels=0\n"));
        found = FALSE;
        goto exit_detect;
    }
    KdPrint2((PRINT_PREFIX "  Adjusted Channels %d\n", NumberChannels));

#ifdef _DEBUG
    v_Mj = ((version >> 20) & 0xf0) + ((version >> 16) & 0x0f);
    v_Mn = ((version >> 4) & 0xf0) + (version & 0x0f);

    KdPrint2((PRINT_PREFIX "  AHCI version %#x.%02x controller with %d ports (mask %#x) detected\n",
		  v_Mj, v_Mn,
		  NumberChannels, PI));
    KdPrint(("  AHCI SATA Gen %d\n", (((CAP & AHCI_CAP_ISS_MASK) >> 20)) ));
#endif //DBG

    if(CAP & AHCI_CAP_SPM) {
        KdPrint2((PRINT_PREFIX "  PM supported\n"));
        if(AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreAhciPM", 1 /* DEBUG */)) {
            KdPrint2((PRINT_PREFIX "SATA/AHCI w/o PM, max luns 1\n"));
            deviceExtension->NumberLuns = 1;
            //chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
        } else {
            KdPrint2((PRINT_PREFIX "SATA/AHCI -> possible PM, max luns %d\n", SATA_MAX_PM_UNITS));
            deviceExtension->NumberLuns = SATA_MAX_PM_UNITS;
            //deviceExtension->NumberLuns = 1;
        }
    } else {
        KdPrint2((PRINT_PREFIX "  PM not supported -> 1 lun/chan\n"));
        deviceExtension->NumberLuns = 1;
    }

    if(!UniAtaAhciValidateVersion(deviceExtension, version, TRUE)) {
        goto exit_detect;
    }

    deviceExtension->HwFlags |= UNIATA_SATA | UNIATA_AHCI;
    if(deviceExtension->NumberChannels < NumberChannels) {
        deviceExtension->NumberChannels = NumberChannels;
    }
    deviceExtension->DmaSegmentLength = 0x3fffff+1; // 4MB
    deviceExtension->DmaSegmentAlignmentMask = -1; // no restrictions

    deviceExtension->BusMaster = DMA_MODE_AHCI;
    deviceExtension->MaxTransferMode = max(deviceExtension->MaxTransferMode, ATA_SA150+(((CAP & AHCI_CAP_ISS_MASK) >> 20)-1) );

    found = TRUE;

exit_detect:
    UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_GHC, GHC0);
    KdPrint(("  AHCI detect status %d\n", found));

    return found;
} // end UniataAhciDetect()

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
    ULONG            CI, ACT;
    AHCI_IS_REG      IS;
    SATA_SSTATUS_REG SStatus;
    SATA_SERROR_REG  SError;
    //ULONG offs = sizeof(IDE_AHCI_REGISTERS) + Channel*sizeof(IDE_AHCI_PORT_REGISTERS);
    ULONG tag=0;

    KdPrint(("UniataAhciStatus(%d-%d):\n", lChannel, Channel));

    hIS = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_IS);
    KdPrint((" hIS %#x\n", hIS));
    hIS &= (1 << Channel);
    if(!hIS) {
        return INTERRUPT_REASON_IGNORE;
    }
    IS.Reg      = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_IS);
    CI          = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CI);
    ACT         = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_ACT);
    SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
    SError.Reg  = AtapiReadPort4(chan, IDX_SATA_SError); 

    /* clear interrupt(s) */
    UniataAhciWriteHostPort4(deviceExtension, IDX_AHCI_IS, hIS);
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IS, IS.Reg);
    AtapiWritePort4(chan, IDX_SATA_SError, SError.Reg);

    KdPrint((" AHCI: is=%08x ss=%08x serror=%08x CI=%08x, ACT=%08x\n",
	   IS.Reg, SStatus.Reg, SError.Reg, CI, ACT));

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
    chan->AhciCompleteCI = (chan->AhciPrevCI ^ CI) & chan->AhciPrevCI; // only 1->0 states
    chan->AhciPrevCI = CI;
    chan->AhciLastSError = SError.Reg;
    KdPrint((" AHCI: complete mask %#x\n", chan->AhciCompleteCI));
    chan->AhciLastIS = IS.Reg;
    if(CI & (1 << tag)) {
#ifdef _DEBUG
        UniataDumpAhciPortRegs(chan);
#endif //DBG
        //deviceExtension->ExpectingInterrupt++; // will be updated in ISR on ReturnEnableInterrupts
        if(IS.Reg &
            (ATA_AHCI_P_IX_OF | ATA_AHCI_P_IX_INF | ATA_AHCI_P_IX_IF |
             ATA_AHCI_P_IX_HBD | ATA_AHCI_P_IX_HBF | ATA_AHCI_P_IX_TFE)) {
            KdPrint((" AHCI: unexpected, error\n"));
        } else {
            KdPrint((" AHCI: unexpected, incomplete command or error ?\n"));
/*
            ULONG TFD;

            TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
            KdPrint2(("  TFD %#x\n", TFD));
            if(TFD & IDE_STATUS_BUSY) {
                KdPrint2(("  Seems to be interrupt on error\n"));
                return INTERRUPT_REASON_OUR;
            }
*/
            return INTERRUPT_REASON_UNEXPECTED;
        }
    }
    return INTERRUPT_REASON_OUR;

} // end UniataAhciStatus()

VOID
NTAPI
UniataAhciSnapAtaRegs(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
 IN OUT PIDEREGS_EX regs
    )
{
    ULONG TFD, SIG;

    regs->bDriveHeadReg    = IDE_DRIVE_SELECT_1;
    TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
    regs->bCommandReg = (UCHAR)(TFD & 0xff);
    regs->bFeaturesReg = (UCHAR)((TFD >> 8) & 0xff);

    SIG = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_SIG);
    regs->bSectorCountReg  = (UCHAR)(SIG & 0xff);
    regs->bSectorNumberReg = (UCHAR)((SIG >> 8) & 0xff);
    regs->bCylLowReg       = (UCHAR)((SIG >> 16) & 0xff);
    regs->bCylHighReg      = (UCHAR)((SIG >> 24) & 0xff);
    regs->bOpFlags = 0;

    return;
} // end UniataAhciSnapAtaRegs()

ULONG
NTAPI
UniataAhciSetupFIS_H2D(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
   OUT PUCHAR fis,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature
    )
{
    //ULONG i;
    PUCHAR plba;
    BOOLEAN need48;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];

    KdPrint2((PRINT_PREFIX "  AHCI setup FIS %x, ch %d, dev %d\n", fis, lChannel, DeviceNumber));
    //i = 0;
    plba = (PUCHAR)&lba;

    RtlZeroMemory(fis, 20);

    fis[0] = AHCI_FIS_TYPE_ATA_H2D;  /* host to device */
    fis[1] = 0x80 | ((UCHAR)DeviceNumber & 0x0f);  /* command FIS (note PM goes here) */
    fis[IDX_AHCI_o_DriveSelect] = IDE_DRIVE_SELECT_1 |
             ((AtaCommandFlags[command] & (ATA_CMD_FLAG_LBAIOsupp | ATA_CMD_FLAG_48)) ? IDE_USE_LBA : 0);
    fis[IDX_AHCI_o_Control] = IDE_DC_A_4BIT;

    // IDE_COMMAND_ATAPI_IDENTIFY should be processed as regular ATA command,
    // the rest of ATAPI requests are processed via IDE_COMMAND_ATAPI_PACKET
    if(/*(chan->lun[DeviceNumber]->DeviceFlags & DFLAGS_ATAPI_DEVICE) && 
        */
        command == IDE_COMMAND_ATAPI_PACKET) { 
        fis[IDX_AHCI_o_Command] = IDE_COMMAND_ATAPI_PACKET;
        if(feature & ATA_F_DMA) {
            fis[IDX_AHCI_o_Feature] = (UCHAR)(feature & 0xff);
        } else {
            fis[IDX_AHCI_o_CylinderLow] = (UCHAR)(count & 0xff);
            fis[IDX_AHCI_o_CylinderHigh] = (UCHAR)(count>>8) & 0xff;
        }
        //fis[IDX_AHCI_o_Control] |= IDE_DC_A_4BIT;
    } else {

        if(((AtaCommandFlags[command] & (ATA_CMD_FLAG_LBAIOsupp|ATA_CMD_FLAG_FUA)) == ATA_CMD_FLAG_LBAIOsupp) &&
           CheckIfBadBlock(chan->lun[DeviceNumber], lba, count)) {
            KdPrint3((PRINT_PREFIX ": artificial bad block, lba %#I64x count %#x\n", lba, count));
            //return IDE_STATUS_ERROR;
            //return SRB_STATUS_ERROR;
            return 0;
        }

        need48 = UniAta_need_lba48(command, lba, count,
            chan->lun[DeviceNumber]->IdentifyData.FeaturesSupport.Address48);

        /* translate command into 48bit version */
        if(need48) {
            if(AtaCommandFlags[command] & ATA_CMD_FLAG_48supp) {
                command = AtaCommands48[command];
            } else {
                KdPrint2((PRINT_PREFIX "  unhandled LBA48 command\n"));
                return 0;
            }
        }

        fis[IDX_AHCI_o_Command] = command;
        fis[IDX_AHCI_o_Feature] = (UCHAR)feature;

        fis[IDX_AHCI_o_BlockNumber] = plba[0];
        fis[IDX_AHCI_o_CylinderLow] = plba[1];
        fis[IDX_AHCI_o_CylinderHigh] = plba[2];

        fis[IDX_AHCI_o_BlockCount] = (UCHAR)count & 0xff;

        if(need48) {
            //i++;
            fis[IDX_AHCI_o_Control] |= IDE_DC_USE_HOB;

            fis[IDX_AHCI_o_BlockNumberExp] = plba[3];
            fis[IDX_AHCI_o_CylinderLowExp] = plba[4];
            fis[IDX_AHCI_o_CylinderHighExp] = plba[5]; 

            fis[IDX_AHCI_o_BlockCountExp] = (UCHAR)(count>>8) & 0xff;

            fis[IDX_AHCI_o_FeatureExp] = (UCHAR)(feature>>8) & 0xff;

            chan->ChannelCtrlFlags |= CTRFLAGS_LBA48;
        } else {
//#ifdef _MSC_VER
//#pragma warning(push)
//#pragma warning(disable:4333) // right shift by too large amount, data loss
//#endif
            fis[IDX_AHCI_o_DriveSelect] |= /*IDE_DRIVE_1 |*/ (plba[3] & 0x0f);
            chan->ChannelCtrlFlags &= ~CTRFLAGS_LBA48;
//#ifdef _MSC_VER
//#pragma warning(pop)
//#endif
        }

        //fis[14] = 0x00;

    }

    KdDump(fis, 20);

    return 20;
} // end UniataAhciSetupFIS_H2D()

ULONG
NTAPI
UniataAhciSetupFIS_H2D_Direct(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
   OUT PUCHAR fis,
    IN PIDEREGS_EX regs
    )
{
    //ULONG i;
    //PUCHAR plba;
    BOOLEAN need48;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    UCHAR command;

    command = regs->bCommandReg;

    KdPrint2((PRINT_PREFIX "  AHCI setup FIS Direct %x, ch %d, dev %d\n", fis, lChannel, DeviceNumber));
    //i = 0;
    //plba = (PUCHAR)&lba;

    RtlZeroMemory(fis, 20);

    fis[0] = AHCI_FIS_TYPE_ATA_H2D;  /* host to device */
    fis[1] = 0x80 | ((UCHAR)DeviceNumber & 0x0f);  /* command FIS (note PM goes here) */
    fis[IDX_AHCI_o_DriveSelect] = IDE_DRIVE_SELECT_1 |
             ((AtaCommandFlags[command] & (ATA_CMD_FLAG_LBAIOsupp | ATA_CMD_FLAG_48)) ? IDE_USE_LBA : 0);
    fis[IDX_AHCI_o_Control] = IDE_DC_A_4BIT;

    // IDE_COMMAND_ATAPI_IDENTIFY should be processed as regular ATA command,
    // the rest of ATAPI requests are processed via IDE_COMMAND_ATAPI_PACKET
    if(/*(chan->lun[DeviceNumber]->DeviceFlags & DFLAGS_ATAPI_DEVICE) && 
        */
        command == IDE_COMMAND_ATAPI_PACKET) { 
/*        fis[IDX_AHCI_o_Command] = IDE_COMMAND_ATAPI_PACKET;
        if(feature & ATA_F_DMA) {
            fis[IDX_AHCI_o_Feature] = (UCHAR)(feature & 0xff);
        } else {
            fis[IDX_AHCI_o_CylinderLow] = (UCHAR)(count & 0xff);
            fis[IDX_AHCI_o_CylinderHigh] = (UCHAR)(count>>8) & 0xff;
        }*/
        return 0;
        //fis[IDX_AHCI_o_Control] |= IDE_DC_A_4BIT;
    } else {

        need48 = (regs->bOpFlags & ATA_FLAGS_48BIT_COMMAND) &&
            chan->lun[DeviceNumber]->IdentifyData.FeaturesSupport.Address48;

        /* translate command into 48bit version */
        if(need48) {
            if(AtaCommandFlags[command] & ATA_CMD_FLAG_48supp) {
                command = AtaCommands48[command];
            } else {
                KdPrint2((PRINT_PREFIX "  unhandled LBA48 command\n"));
                return 0;
            }
        }

        fis[IDX_AHCI_o_Command] = command;
        fis[IDX_AHCI_o_Feature] = regs->bFeaturesReg;

        fis[IDX_AHCI_o_BlockNumber]  = regs->bSectorNumberReg;
        fis[IDX_AHCI_o_CylinderLow]  = regs->bCylLowReg;
        fis[IDX_AHCI_o_CylinderHigh] = regs->bCylHighReg;

        fis[IDX_AHCI_o_BlockCount]   = regs->bSectorCountReg;

        if(need48) {
            //i++;
            fis[IDX_AHCI_o_Control] |= IDE_DC_USE_HOB;

            fis[IDX_AHCI_o_BlockNumberExp]  = regs->bSectorNumberRegH;
            fis[IDX_AHCI_o_CylinderLowExp]  = regs->bCylLowRegH;
            fis[IDX_AHCI_o_CylinderHighExp] = regs->bCylHighRegH;

            fis[IDX_AHCI_o_BlockCountExp]   = regs->bSectorCountRegH;

            fis[IDX_AHCI_o_FeatureExp] = regs->bFeaturesRegH;

            chan->ChannelCtrlFlags |= CTRFLAGS_LBA48;
        } else {
            //fis[IDX_AHCI_o_DriveSelect] |= /*IDE_DRIVE_1 |*/ (plba[3] & 0x0f);
            chan->ChannelCtrlFlags &= ~CTRFLAGS_LBA48;
        }
        fis[IDX_AHCI_o_DriveSelect] |= regs->bDriveHeadReg & 0x0f;
    }

    KdDump(fis, 20);

    return 20;
} // end UniataAhciSetupFIS_H2D_Direct()

UCHAR
NTAPI
UniataAhciWaitCommandReady(
    IN PHW_CHANNEL chan,
    IN ULONG timeout
    )
{
    AHCI_IS_REG      IS;
    //ULONG            ACT;
    ULONG            CI=0;
    ULONG i;
    ULONG SError;
    ULONG tag=0;

    timeout *= 5;

    for (i=0; i<timeout; i++) {
        CI = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CI);
        //ACT = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_ACT);
        if (!(( CI >> tag) & 0x01)) {
            break;
        }
        IS.Reg      = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_IS);
        //KdPrint(("  IS %#x\n", IS.Reg));
        if(IS.Reg) {
            break;
        }
        SError = AtapiReadPort4(chan, IDX_SATA_SError);
        if(SError) {
            KdPrint((" AHCI: error %#x\n", SError));
            i = timeout;
            break;
        }
        AtapiStallExecution(200);
    }
    KdPrint(("  CI %#x\n", CI));

    //SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
    //SError.Reg  = AtapiReadPort4(chan, IDX_SATA_SError); 

    /* clear interrupt(s) */
    IS.Reg      = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_IS);
    KdPrint(("  IS %#x\n", IS.Reg));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IS, IS.Reg);

    if (timeout && (i >= timeout)) {
#ifdef _DEBUG
        ULONG TFD;

        SError = AtapiReadPort4(chan, IDX_SATA_SError);
        KdPrint((" AHCI: timeout, SError %#x\n", SError));

        TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
        KdPrint2(("  TFD %#x\n", TFD));
#endif //DBG
        
        return IDE_STATUS_WRONG;
    }

    return IDE_STATUS_IDLE;
} // end UniataAhciWaitCommandReady()

UCHAR
NTAPI
UniataAhciSendCommand(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN USHORT ahci_flags,
    IN ULONG timeout
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    //ULONG            SError;
    //SATA_SSTATUS_REG SStatus;
    //SATA_SERROR_REG  SError;
    //ULONG offs = sizeof(IDE_AHCI_REGISTERS) + Channel*sizeof(IDE_AHCI_PORT_REGISTERS);
    //ULONGIO_PTR base;
    ULONG tag=0;

    PIDE_AHCI_CMD_LIST AHCI_CL = &(chan->AhciCtlBlock->cmd_list[tag]);

    KdPrint(("UniataAhciSendCommand: lChan %d\n", chan->lChannel));

    AHCI_CL->prd_length = 0;
    //AHCI_CL->cmd_flags = (20 / sizeof(ULONG)) | ahci_flags | (DeviceNumber << 12);
    AHCI_CL->cmd_flags = UniAtaAhciAdjustIoFlags(0, ahci_flags, 20, DeviceNumber);

    AHCI_CL->bytecount = 0;
    AHCI_CL->cmd_table_phys = chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, cmd);
    if(AHCI_CL->cmd_table_phys & AHCI_CMD_ALIGNEMENT_MASK) {
        KdPrint2((PRINT_PREFIX "  AHCI CMD address is not aligned (mask %#x)\n", (ULONG)AHCI_CMD_ALIGNEMENT_MASK));
    }

    //UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_ACT, 0x01 << tag);
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CI, 1 << tag);

    return UniataAhciWaitCommandReady(chan, timeout);

} // end UniataAhciSendCommand()

UCHAR
NTAPI
UniataAhciSendPIOCommand(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PUCHAR data,
    IN ULONG length, /* bytes */
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT bcount, /* block count, just ATA register */
    IN USHORT feature,
    IN USHORT ahci_flags,
    IN ULONG wait_flags,
    IN ULONG timeout
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    UCHAR statusByte;
    PATA_REQ AtaReq;
    ULONG fis_size;
    //ULONG tag=0;
    //PIDE_AHCI_CMD  AHCI_CMD = &(chan->AhciCtlBlock->cmd);
    PIDE_AHCI_CMD  AHCI_CMD = NULL;

    //PIDE_AHCI_CMD_LIST AHCI_CL = &(chan->AhciCtlBlock->cmd_list[tag]);

    KdPrint2((PRINT_PREFIX "UniataAhciSendPIOCommand: cntrlr %#x:%#x dev %#x, cmd %#x, lba %#I64x bcount %#x feature %#x, buff %#x, len %#x, WF %#x \n",
                 deviceExtension->DevIndex, lChannel, DeviceNumber, command, lba, bcount, feature, data, length, wait_flags ));

    if(length/DEV_BSIZE != bcount) {
        KdPrint(("  length/DEV_BSIZE != bcount\n"));
    }

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    if(!Srb) {
        Srb = BuildAhciInternalSrb(HwDeviceExtension, DeviceNumber, lChannel, data, length);
        if(!Srb) {
            KdPrint(("  !Srb\n"));
            return IDE_STATUS_WRONG;
        }
        //UniataAhciSetupCmdPtr(AtaReq); // must be called before DMA setup
        //should be already called on init
    }
    AtaReq = (PATA_REQ)(Srb->SrbExtension);
    //KdPrint(("  Srb %#x, AtaReq %#x\n", Srb, AtaReq));

    AHCI_CMD = AtaReq->ahci.ahci_cmd_ptr;

    fis_size = UniataAhciSetupFIS_H2D(deviceExtension, DeviceNumber, lChannel,
           &(AHCI_CMD->cfis[0]),
            command,
            lba,
            bcount,
            feature
            );

    if(!fis_size) {
        KdPrint2(("!fis_size\n"));
        return IDE_STATUS_WRONG;
    }

    //KdPrint2(("UniAtaAhciAdjustIoFlags(command, ahci_flags, fis_size, DeviceNumber)\n"));
    ahci_flags = UniAtaAhciAdjustIoFlags(command, ahci_flags, fis_size, DeviceNumber);
    KdPrint2(("ahci_flags %#x\n", ahci_flags));

    if(data) {
        if(ahci_flags & ATA_AHCI_CMD_WRITE) {
            AtaReq->Flags &= ~REQ_FLAG_READ;
            Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
            KdPrint(("  assume OUT\n"));
        } else {
            AtaReq->Flags |= REQ_FLAG_READ;
            Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
            KdPrint(("  assume IN\n"));
        }
        if(!AtapiDmaSetup(HwDeviceExtension,
                            DeviceNumber,
                            lChannel,          // logical channel,
                            Srb,
                            data,
                            length)) {
            KdPrint2(("  can't setup buffer\n"));
            return IDE_STATUS_WRONG;
        }
    }

    AtaReq->ahci.io_cmd_flags = ahci_flags;

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    UniataAhciBeginTransaction(HwDeviceExtension, lChannel, DeviceNumber, Srb);

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    if(wait_flags == ATA_IMMEDIATE) {
        statusByte = 0;
        KdPrint2(("  return imemdiately\n"));
    } else {
        statusByte = UniataAhciWaitCommandReady(chan, timeout);
        UniataAhciStatus(HwDeviceExtension, lChannel, DeviceNumber);
        UniataAhciEndTransaction(HwDeviceExtension, lChannel, DeviceNumber, Srb);
    }

    return statusByte;

} // end UniataAhciSendPIOCommand()

UCHAR
NTAPI
UniataAhciSendPIOCommandDirect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIDEREGS_EX regs,
    IN ULONG wait_flags,
    IN ULONG timeout
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    UCHAR statusByte;
    PATA_REQ AtaReq;
    ULONG fis_size;
    //ULONG tag=0;
    //PIDE_AHCI_CMD  AHCI_CMD = &(chan->AhciCtlBlock->cmd);
    PIDE_AHCI_CMD  AHCI_CMD = NULL;
    USHORT ahci_flags=0;
//    USHORT bcount=0;

    //PIDE_AHCI_CMD_LIST AHCI_CL = &(chan->AhciCtlBlock->cmd_list[tag]);

    KdPrint2((PRINT_PREFIX "UniataAhciSendPIOCommand: cntrlr %#x:%#x dev %#x, buff %#x, len %#x, WF %#x \n",
                 deviceExtension->DevIndex, lChannel, DeviceNumber, Srb->DataBuffer, Srb->DataTransferLength, wait_flags ));

//    if(Srb->DataTransferLength/DEV_BSIZE != bcount) {
//        KdPrint(("  length/DEV_BSIZE != bcount\n"));
//    }

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    if(!Srb) {
        KdPrint(("  !Srb\n"));
        return IDE_STATUS_WRONG;
        //UniataAhciSetupCmdPtr(AtaReq); // must be called before DMA setup
        //should be already called on init
    }
    AtaReq = (PATA_REQ)(Srb->SrbExtension);
    //KdPrint(("  Srb %#x, AtaReq %#x\n", Srb, AtaReq));

    AHCI_CMD = AtaReq->ahci.ahci_cmd_ptr;
    if(!AHCI_CMD) {
        KdPrint(("  !AHCI_CMD\n"));
        return IDE_STATUS_WRONG;
    }

    if(Srb->DataTransferLength) {
        if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
            ahci_flags |= ATA_AHCI_CMD_WRITE;
            AtaReq->Flags &= ~REQ_FLAG_READ;
        } else {
            AtaReq->Flags |= REQ_FLAG_READ;
        }
    }

    fis_size = UniataAhciSetupFIS_H2D_Direct(deviceExtension, DeviceNumber, lChannel,
           &(AHCI_CMD->cfis[0]),
            regs);

    if(!fis_size) {
        KdPrint2(("!fis_size\n"));
        return IDE_STATUS_WRONG;
    }

    //KdPrint2(("UniAtaAhciAdjustIoFlags(command, ahci_flags, fis_size, DeviceNumber)\n"));
    ahci_flags = UniAtaAhciAdjustIoFlags(regs->bCommandReg, ahci_flags, fis_size, DeviceNumber);
    KdPrint2(("ahci_flags %#x\n", ahci_flags));

    if(Srb->DataTransferLength) {
        if(!AtapiDmaSetup(HwDeviceExtension,
                            DeviceNumber,
                            lChannel,          // logical channel,
                            Srb,
                            (PUCHAR)(Srb->DataBuffer),
                            Srb->DataTransferLength)) {
            KdPrint2(("  can't setup buffer\n"));
            return IDE_STATUS_WRONG;
        }
    }

    AtaReq->ahci.io_cmd_flags = ahci_flags;

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    UniataAhciBeginTransaction(HwDeviceExtension, lChannel, DeviceNumber, Srb);

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    if(wait_flags == ATA_IMMEDIATE) {
        statusByte = 0;
        KdPrint2(("  return imemdiately\n"));
    } else {
        statusByte = UniataAhciWaitCommandReady(chan, timeout);
        UniataAhciStatus(HwDeviceExtension, lChannel, DeviceNumber);
        UniataAhciEndTransaction(HwDeviceExtension, lChannel, DeviceNumber, Srb);
    }

    return statusByte;

} // end UniataAhciSendPIOCommandDirect()

BOOLEAN
NTAPI
UniataAhciAbortOperation(
    IN PHW_CHANNEL chan
    )
{
    /* kick controller into sane state */
    if(!UniataAhciStop(chan)) {
        return FALSE;
    }
    if(!UniataAhciStopFR(chan)) {
        return FALSE;
    }
    if(!UniataAhciCLO(chan)) {
        return FALSE;
    }
    UniataAhciStartFR(chan);
    UniataAhciStart(chan);

    return TRUE;
} // end UniataAhciAbortOperation()

ULONG
NTAPI
UniataAhciSoftReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    //ULONG            CI;
    //AHCI_IS_REG      IS;
    //ULONG tag=0;

    KdPrint(("UniataAhciSoftReset: lChan %d\n", chan->lChannel));

    PIDE_AHCI_CMD  AHCI_CMD = &(chan->AhciCtlBlock->cmd);
    PUCHAR         RCV_FIS = &(chan->AhciCtlBlock->rcv_fis.rfis[0]);

    /* kick controller into sane state */
    if(!UniataAhciAbortOperation(chan)) {
        KdPrint2(("  abort failed\n"));
        return (ULONG)(-1);
    }

    /* pull reset active */
    RtlZeroMemory(AHCI_CMD->cfis, sizeof(AHCI_CMD->cfis));
    AHCI_CMD->cfis[0] = AHCI_FIS_TYPE_ATA_H2D;
    AHCI_CMD->cfis[1] = (UCHAR)DeviceNumber & 0x0f;
    //AHCI_CMD->cfis[7] = IDE_USE_LBA | IDE_DRIVE_SELECT;
    AHCI_CMD->cfis[15] = (IDE_DC_A_4BIT | IDE_DC_RESET_CONTROLLER);

    if(UniataAhciSendCommand(HwDeviceExtension, lChannel, DeviceNumber, ATA_AHCI_CMD_RESET | ATA_AHCI_CMD_CLR_BUSY, 100) == IDE_STATUS_WRONG) {
        KdPrint2(("  timeout\n"));
        return (ULONG)(-1);
    }
    AtapiStallExecution(50);

    /* pull reset inactive */
    RtlZeroMemory(AHCI_CMD->cfis, sizeof(AHCI_CMD->cfis));
    AHCI_CMD->cfis[0] = AHCI_FIS_TYPE_ATA_H2D;
    AHCI_CMD->cfis[1] = (UCHAR)DeviceNumber & 0x0f;
    //AHCI_CMD->cfis[7] = IDE_USE_LBA | IDE_DRIVE_SELECT;
    AHCI_CMD->cfis[15] = (IDE_DC_A_4BIT);
    if(UniataAhciSendCommand(HwDeviceExtension, lChannel, DeviceNumber, 0, 3000) == IDE_STATUS_WRONG) {
        KdPrint2(("  timeout (2)\n"));
        return (ULONG)(-1);
    }

    UniataAhciWaitReady(chan, 1);

    KdDump(RCV_FIS, sizeof(chan->AhciCtlBlock->rcv_fis.rfis));

    return UniataAhciUlongFromRFIS(RCV_FIS);

} // end UniataAhciSoftReset()

ULONG
NTAPI
UniataAhciWaitReady(
    IN PHW_CHANNEL chan,
    IN ULONG timeout
    )
{
    ULONG TFD;
    ULONG i;

    KdPrint2(("UniataAhciWaitReady: lChan %d\n", chan->lChannel));

    //base = (ULONGIO_PTR)(&deviceExtension->BaseIoAHCI_0 + offs);

    TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
    for(i=0; i<timeout && (TFD &
              (IDE_STATUS_DRQ | IDE_STATUS_BUSY)); i++) {
        AtapiStallExecution(1000);
        TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
    }

    KdPrint2(("  TFD %#x\n", TFD));

    return TFD;

} // end UniataAhciWaitReady()

ULONG
NTAPI
UniataAhciHardReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
   OUT PULONG signature
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    ULONG            TFD;


    KdPrint(("UniataAhciHardReset: lChan %d\n", chan->lChannel));

    (*signature) = 0xffffffff;

    UniataAhciStop(chan);
    if(UniataSataPhyEnable(HwDeviceExtension, lChannel, 0/* dev0*/, UNIATA_SATA_RESET_ENABLE) == IDE_STATUS_WRONG) {
        KdPrint(("  no PHY\n"));
        return IDE_STATUS_WRONG;
    }

    /* Wait for clearing busy status. */
    TFD = UniataAhciWaitReady(chan, 15000);
    if(TFD & (IDE_STATUS_DRQ | IDE_STATUS_BUSY)) {
        KdPrint(("  busy: TFD %#x\n", TFD));
        return TFD;
    }
    KdPrint(("  TFD %#x\n", TFD));

#ifdef _DEBUG
    UniataDumpAhciPortRegs(chan);
#endif // DBG

    (*signature) = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_SIG);
    KdPrint(("  sig: %#x\n", *signature));

    UniataAhciStart(chan);

    return 0;

} // end UniataAhciHardReset()

VOID
NTAPI
UniataAhciReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG offs = sizeof(IDE_AHCI_REGISTERS) + Channel*sizeof(IDE_AHCI_PORT_REGISTERS);
    ULONG CAP;
    //ULONGIO_PTR base;
    ULONG signature;
    ULONG i;
    ULONG VendorID =  deviceExtension->DevID & 0xffff;

    KdPrint(("UniataAhciReset: lChan %d\n", chan->lChannel));

    //base = (ULONGIO_PTR)(&deviceExtension->BaseIoAHCI_0 + offs);

    /* Disable port interrupts */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IE, 0);

    if(UniataAhciHardReset(HwDeviceExtension, lChannel, &signature)) {

        KdPrint(("  No devices in all LUNs\n"));
        for (i=0; i<deviceExtension->NumberLuns; i++) {
            // Zero device fields to ensure that if earlier devices were found,
            // but not claimed, the fields are cleared.
            UniataForgetDevice(chan->lun[i]);
        }

	/* enable wanted port interrupts */
        UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IE,
            ATA_AHCI_P_IX_CPD | ATA_AHCI_P_IX_PRC | ATA_AHCI_P_IX_PC);
        return;
    }

    /* enable wanted port interrupts */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IE,
        (ATA_AHCI_P_IX_CPD | ATA_AHCI_P_IX_TFE | ATA_AHCI_P_IX_HBF |
         ATA_AHCI_P_IX_HBD | ATA_AHCI_P_IX_IF | ATA_AHCI_P_IX_OF |
         ((/*ch->pm_level == */0) ? (ATA_AHCI_P_IX_PRC | ATA_AHCI_P_IX_PC) : 0) |
         ATA_AHCI_P_IX_DP | ATA_AHCI_P_IX_UF | ATA_AHCI_P_IX_SDB |
         ATA_AHCI_P_IX_DS | ATA_AHCI_P_IX_PS | ATA_AHCI_P_IX_DHR) );

    /*
     * Only probe for PortMultiplier if HW has support.
     * Ignore Marvell, which is not working,
     */
    CAP = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP);
    if ((CAP & AHCI_CAP_SPM) &&
	    (VendorID != ATA_MARVELL_ID)) {
        KdPrint(("  check PM\n"));
	signature = UniataAhciSoftReset(HwDeviceExtension, lChannel, AHCI_DEV_SEL_PM);
	/* Workaround for some ATI chips, failing to soft-reset
	 * when port multiplicator supported, but absent.
	 * XXX: We can also check PxIS.IPMS==1 here to be sure. */
	if (signature == 0xffffffff) {
            KdPrint(("  re-check PM\n"));
	    signature = UniataAhciSoftReset(HwDeviceExtension, lChannel, 0);
	}
    } else {
        signature = UniataAhciSoftReset(HwDeviceExtension, lChannel, 0);
    }

    KdPrint(("  signature %#x\n", signature));
    chan->lun[0]->DeviceFlags &= ~(DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT | CTRFLAGS_AHCI_PM);
    switch (signature >> 16) {
    case 0x0000:
        KdPrint(("  ATA dev\n"));
        chan->lun[0]->DeviceFlags |= DFLAGS_DEVICE_PRESENT;
	chan->PmLunMap = 0;
	break;
    case 0x9669:
        KdPrint(("  PM\n"));
        if(deviceExtension->NumberLuns > 1) {
	    chan->ChannelCtrlFlags |= CTRFLAGS_AHCI_PM;
            UniataSataIdentifyPM(chan);
	} else {
            KdPrint(("  no PM supported (1 lun/chan)\n"));
	}
	break;
    case 0xeb14:
        KdPrint(("  ATAPI dev\n"));
        chan->lun[0]->DeviceFlags |= (DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT);
	chan->PmLunMap = 0;
	break;
    default: /* SOS XXX */
        KdPrint(("  default to ATA ???\n"));
        chan->lun[0]->DeviceFlags |= DFLAGS_DEVICE_PRESENT;
	chan->PmLunMap = 0;
    }

    return;

} // end UniataAhciReset()

VOID
NTAPI
UniataAhciStartFR(
    IN PHW_CHANNEL chan
    )
{
    ULONG CMD;

    KdPrint2(("UniataAhciStartFR: lChan %d\n", chan->lChannel));

    CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    KdPrint2(("  CMD %#x\n", CMD));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD | ATA_AHCI_P_CMD_FRE);
    UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */

    return;
} // end UniataAhciStartFR()

BOOLEAN
NTAPI
UniataAhciStopFR(
    IN PHW_CHANNEL chan
    )
{
    ULONG CMD;
    ULONG i;

    KdPrint2(("UniataAhciStopFR: lChan %d\n", chan->lChannel));

    CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    KdPrint2(("  CMD %#x\n", CMD));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD & ~ATA_AHCI_P_CMD_FRE);

    for(i=0; i<1000; i++) {
        CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
        if(!(CMD & ATA_AHCI_P_CMD_FR)) {
            KdPrint2(("  final CMD %#x\n", CMD));
            return TRUE;
        }
        AtapiStallExecution(1000);
    }
    KdPrint2(("  CMD %#x\n", CMD));
    KdPrint(("   SError %#x\n", AtapiReadPort4(chan, IDX_SATA_SError)));
    KdPrint2(("UniataAhciStopFR: timeout\n"));
    return FALSE;
} // end UniataAhciStopFR()

VOID
NTAPI
UniataAhciStart(
    IN PHW_CHANNEL chan
    )
{
    ULONG IS, CMD;
    SATA_SERROR_REG  SError;

    KdPrint2(("UniataAhciStart: lChan %d\n", chan->lChannel));

    /* clear SATA error register */
    SError.Reg  = AtapiReadPort4(chan, IDX_SATA_SError); 

    /* clear any interrupts pending on this channel */
    IS = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_IS);
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IS, IS);

    KdPrint2(("    SError %#x, IS %#x\n", SError.Reg, IS));

    CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    KdPrint2(("  CMD %#x\n", CMD));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD,
        CMD |
        ATA_AHCI_P_CMD_ST |
        ((chan->ChannelCtrlFlags & CTRFLAGS_AHCI_PM) ? ATA_AHCI_P_CMD_PMA : 0));
    UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */

    return;
} // end UniataAhciStart()

BOOLEAN
NTAPI
UniataAhciCLO(
    IN PHW_CHANNEL chan
    )
{
    //PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    ULONG CAP, CMD;
    //SATA_SERROR_REG  SError;
    ULONG i;

    KdPrint2(("UniataAhciCLO: lChan %d\n", chan->lChannel));

    /* issue Command List Override if supported */ 
    //CAP = UniataAhciReadHostPort4(deviceExtension, IDX_AHCI_CAP);
    CAP = chan->DeviceExtension->AHCI_CAP; 
    if(!(CAP & AHCI_CAP_SCLO)) {
        return TRUE;
    }
    KdPrint2(("  send CLO\n"));
    CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    CMD |= ATA_AHCI_P_CMD_CLO;
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD);

    for(i=0; i<1000; i++) {
        CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
        if(!(CMD & ATA_AHCI_P_CMD_CLO)) {
            KdPrint2(("  final CMD %#x\n", CMD));
            return TRUE;
        }
        AtapiStallExecution(1000);
    }
    KdPrint2(("  CMD %#x\n", CMD));
    KdPrint2(("UniataAhciCLO: timeout\n"));
    return FALSE;
} // end UniataAhciCLO()

BOOLEAN
NTAPI
UniataAhciStop(
    IN PHW_CHANNEL chan
    )
{
    ULONG CMD;
    //SATA_SERROR_REG  SError;
    ULONG i;

    KdPrint2(("UniataAhciStop: lChan %d\n", chan->lChannel));

    /* issue Command List Override if supported */ 
    CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    CMD &= ~ATA_AHCI_P_CMD_ST;
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD);

    for(i=0; i<1000; i++) {
        CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
        if(!(CMD & ATA_AHCI_P_CMD_CR)) {
            KdPrint2(("  final CMD %#x\n", CMD));
            return TRUE;
        }
        AtapiStallExecution(1000);
    }
    KdPrint2(("  CMD %#x\n", CMD));
    KdPrint(("   SError %#x\n", AtapiReadPort4(chan, IDX_SATA_SError)));
    KdPrint2(("UniataAhciStop: timeout\n"));
    return FALSE;
} // end UniataAhciStop()

UCHAR
NTAPI
UniataAhciBeginTransaction(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    ULONG            CMD, CMD0;
    //AHCI_IS_REG      IS;
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    //SATA_SSTATUS_REG SStatus;
    //SATA_SERROR_REG  SError;
    //ULONG offs = sizeof(IDE_AHCI_REGISTERS) + Channel*sizeof(IDE_AHCI_PORT_REGISTERS);
    //ULONGIO_PTR base;
    ULONG tag=0;
    //ULONG i;

    PIDE_AHCI_CMD_LIST AHCI_CL = &(chan->AhciCtlBlock->cmd_list[tag]);

    KdPrint2(("UniataAhciBeginTransaction: lChan %d, AtaReq %#x\n", chan->lChannel, AtaReq));

    if(Srb->DataTransferLength && (!AtaReq->dma_entries || AtaReq->dma_entries >= (USHORT)0xffff)) {
        KdPrint2(("UniataAhciBeginTransaction wrong DMA tab len %x\n", AtaReq->dma_entries));
        return 0;
    }

    AHCI_CL->prd_length = (USHORT)(AtaReq->dma_entries);
    AHCI_CL->cmd_flags  = AtaReq->ahci.io_cmd_flags;
    AHCI_CL->bytecount = 0;
    if(AtaReq->ahci.ahci_base64) {
        KdPrint2((PRINT_PREFIX "  AHCI AtaReq CMD %#x (ph %#x)\n", AtaReq->ahci.ahci_cmd_ptr, (ULONG)(AtaReq->ahci.ahci_base64)));
        AHCI_CL->cmd_table_phys = AtaReq->ahci.ahci_base64;
    } else
    if(AtaReq->ahci.ahci_cmd_ptr) {
        KdPrint2((PRINT_PREFIX "  AHCI AtaReq->Chan CMD %#x (ph %#x) -> %#x (ph %#x)\n",
            AtaReq->ahci.ahci_cmd_ptr, (ULONG)(AtaReq->ahci.ahci_base64),
            &(chan->AhciCtlBlock->cmd), chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, cmd) ));
        RtlCopyMemory(&(chan->AhciCtlBlock->cmd), AtaReq->ahci.ahci_cmd_ptr, 
            FIELD_OFFSET(IDE_AHCI_CMD, prd_tab)+AHCI_CL->prd_length*sizeof(IDE_AHCI_PRD_ENTRY));
        AHCI_CL->cmd_table_phys = chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, cmd);
    } else {
        KdPrint2((PRINT_PREFIX "  no AHCI CMD\n"));
        //AHCI_CL->cmd_table_phys = chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, cmd);
        return 0;
    }
    if(AHCI_CL->cmd_table_phys & AHCI_CMD_ALIGNEMENT_MASK) {
        KdPrint2((PRINT_PREFIX "  AHCI CMD address is not aligned (mask %#x)\n", (ULONG)AHCI_CMD_ALIGNEMENT_MASK));
        return 0;
    }

#ifdef _DEBUG
    KdPrint2(("  prd_length %#x, flags %#x, base %I64x\n", AHCI_CL->prd_length, AHCI_CL->cmd_flags, 
            AHCI_CL->cmd_table_phys));
#endif // DBG

    CMD0 = CMD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD);
    KdPrint2(("  CMD %#x\n", CMD));
    // switch controller to ATAPI mode for ATA_PACKET commands only
    if(ATAPI_DEVICE(chan, DeviceNumber) &&
       AtaReq->ahci.ahci_cmd_ptr->cfis[2] == IDE_COMMAND_ATAPI_PACKET) {
        KdPrint2(("  ATAPI\n"));
        CMD |= ATA_AHCI_P_CMD_ATAPI;
        KdDump(&(AtaReq->ahci.ahci_cmd_ptr->acmd), 16);
    } else {
        CMD &= ~ATA_AHCI_P_CMD_ATAPI;
    }
    if(CMD0 != CMD) {
        KdPrint2(("  send CMD %#x, entries %#x\n", CMD, AHCI_CL->prd_length));
        UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD);
        UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */
    }

    /* issue command to controller */
    //UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_ACT, 0x01 << tag);
    KdPrint2(("  Set CI\n"));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CI, 0x01 << tag);
    chan->AhciPrevCI |= 0x01 << tag;

    CMD0 = CMD;
    CMD |= ATA_AHCI_P_CMD_ST |
          ((chan->ChannelCtrlFlags & CTRFLAGS_AHCI_PM) ? ATA_AHCI_P_CMD_PMA : 0);
    if(CMD != CMD0) {
      KdPrint2(("  Send CMD START\n"));
      UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, CMD);
      UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */
    } else {
      KdPrint2(("  No CMD START, already active\n"));
    }

    if(!ATAPI_DEVICE(chan, DeviceNumber)) {
        // TODO: check if we send ATAPI_RESET and wait for ready of so.
        if(AtaReq->ahci.ahci_cmd_ptr->cfis[2] == IDE_COMMAND_ATAPI_RESET) {
            ULONG  TFD;
            ULONG  i;

            for(i=0; i<1000000; i++) {
                TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
                if(!(TFD & IDE_STATUS_BUSY)) {
                    break;
                }
            }
            if(TFD & IDE_STATUS_BUSY) {
                KdPrint2(("  timeout\n"));
            }
            if(TFD & IDE_STATUS_ERROR) {
                KdPrint2(("  ERROR %#x\n", (UCHAR)(TFD >> 8)));
            }
            AtaReq->ahci.in_status = TFD;

            return IDE_STATUS_SUCCESS;
        }
    }

    return IDE_STATUS_IDLE;

} // end UniataAhciBeginTransaction()

UCHAR
NTAPI
UniataAhciEndTransaction(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    ULONG            CI, ACT;
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    ULONG            TFD;
    PUCHAR         RCV_FIS = &(chan->AhciCtlBlock->rcv_fis.rfis[0]);
    ULONG tag=0;
    //ULONG i;
    PIDE_AHCI_CMD_LIST AHCI_CL = &(chan->AhciCtlBlock->cmd_list[tag]);
    //PHW_LU_EXTENSION     LunExt;

    KdPrint2(("UniataAhciEndTransaction: lChan %d\n", chan->lChannel));

    //LunExt = chan->lun[DeviceNumber];

    TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);
    KdPrint2(("  TFD %#x\n", TFD));

    if(TFD & IDE_STATUS_ERROR) {
        AtaReq->ahci.in_error = (UCHAR)(TFD >> 8);
        KdPrint2(("  ERROR %#x\n", AtaReq->ahci.in_error));
    } else {
        AtaReq->ahci.in_error = 0;
    }
    AtaReq->ahci.in_status = TFD;

    //if (request->flags & ATA_R_CONTROL) {

    AtaReq->ahci.in_bcount = (ULONG)(RCV_FIS[12]) | ((ULONG)(RCV_FIS[13]) << 8);
    AtaReq->ahci.in_lba = (ULONG)(RCV_FIS[4]) | ((ULONGLONG)(RCV_FIS[5]) << 8) |
			     ((ULONGLONG)(RCV_FIS[6]) << 16);
    if(chan->ChannelCtrlFlags & CTRFLAGS_LBA48) {
        AtaReq->ahci.in_lba |= ((ULONGLONG)(RCV_FIS[8]) << 24) |
                                ((ULONGLONG)(RCV_FIS[9]) << 32) |
                                ((ULONGLONG)(RCV_FIS[10]) << 40);
    } else {
        AtaReq->ahci.in_lba |= ((ULONGLONG)(RCV_FIS[8]) << 24) |
                                ((ULONGLONG)(RCV_FIS[9]) << 32) |
                                ((ULONGLONG)(RCV_FIS[7] & 0x0f) << 24);
    }
    AtaReq->WordsTransfered = AHCI_CL->bytecount/2;
/*
    if(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) {
        KdPrint2(("RCV:\n"));
        KdDump(RCV_FIS, 24);
        KdPrint2(("PIO:\n"));
        KdDump(&(chan->AhciCtlBlock->rcv_fis.psfis[0]), 24);

        KdPrint2(("len: %d vs %d\n", AHCI_CL->bytecount, (ULONG)RCV_FIS[5] | ((ULONG)RCV_FIS[6] << 8) ));
        if(!AHCI_CL->bytecount) {
            AtaReq->WordsTransfered = ((ULONG)RCV_FIS[5] | ((ULONG)RCV_FIS[6] << 8)) / 2;
        }
    }
*/
    ACT = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_ACT);
    CI = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CI);
    if(CI & (1 << tag)) {
        // clear CI
        KdPrint2(("  Incomplete command, CI %#x, ACT %#x\n", CI, ACT));
        KdPrint2(("  FIS status %#x, error %#x\n", RCV_FIS[2], RCV_FIS[3]));

#ifdef _DEBUG
        UniataDumpAhciPortRegs(chan);
#endif
        if(!UniataAhciAbortOperation(chan)) {
            KdPrint2(("  Abort failed, need RESET\n"));
        }
#ifdef _DEBUG
        UniataDumpAhciPortRegs(chan);
#endif
        chan->AhciPrevCI = CI & ~((ULONG)1 << tag);
        if(chan->AhciPrevCI) {
            KdPrint2(("  Need command list restart, CI %#x\n", chan->AhciPrevCI));
        }
    } else {
        chan->AhciPrevCI &= ~((ULONG)1 << tag);
        RtlZeroMemory(AHCI_CL, sizeof(IDE_AHCI_CMD_LIST));
    }
    //}

    return 0;

} // end UniataAhciEndTransaction()

VOID
NTAPI
UniataAhciResume(
    IN PHW_CHANNEL chan
    )
{
    ULONGLONG base;

    KdPrint2(("UniataAhciResume: lChan %d\n", chan->lChannel));

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    /* Disable port interrupts */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IE, 0);

    /* setup work areas */
    base = chan->AHCI_CTL_PhAddr;
    if(!base) {
        KdPrint2((PRINT_PREFIX "  AHCI buffer allocation failed\n"));
        return;
    }
    KdPrint2((PRINT_PREFIX "  AHCI CLB setup\n"));
    if(base & AHCI_CLB_ALIGNEMENT_MASK) {
        KdPrint2((PRINT_PREFIX "  AHCI CLB address is not aligned (mask %#x)\n", (ULONG)AHCI_FIS_ALIGNEMENT_MASK));
    }
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CLB,
        (ULONG)(base & 0xffffffff));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CLB + 4,
        (ULONG)((base >> 32) & 0xffffffff));

    KdPrint2((PRINT_PREFIX "  AHCI RCV FIS setup\n"));
    base = chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, rcv_fis);
    if(base & AHCI_FIS_ALIGNEMENT_MASK) {
        KdPrint2((PRINT_PREFIX "  AHCI FIS address is not aligned (mask %#x)\n", (ULONG)AHCI_FIS_ALIGNEMENT_MASK));
    }
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_FB,
        (ULONG)(base & 0xffffffff));
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_FB + 4,
        (ULONG)((base >> 32) & 0xffffffff));

    /* activate the channel and power/spin up device */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD,
        (ATA_AHCI_P_CMD_ACTIVE | ATA_AHCI_P_CMD_POD | ATA_AHCI_P_CMD_SUD |
	     (((chan->ChannelCtrlFlags & CTRFLAGS_AHCI_PM)) ? ATA_AHCI_P_CMD_ALPE : 0) |
	     (((chan->ChannelCtrlFlags & CTRFLAGS_AHCI_PM2)) ? ATA_AHCI_P_CMD_ASP : 0 ))
	     );
    UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */

#ifdef _DEBUG
    //UniataDumpAhciPortRegs(chan);
#endif // DBG

    UniataAhciStartFR(chan);
    UniataAhciStart(chan);

#ifdef _DEBUG
    UniataDumpAhciPortRegs(chan);
#endif // DBG

    return;
} // end UniataAhciResume()

#if 0
VOID
NTAPI
UniataAhciSuspend(
    IN PHW_CHANNEL chan
    )
{
    ULONGLONG base;
    SATA_SCONTROL_REG SControl;

    KdPrint2(("UniataAhciSuspend:\n"));

    /* Disable port interrupts */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_IE, 0);

    /* Reset command register. */
    UniataAhciStop(chan);
    UniataAhciStopFR(chan);
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, 0);
    UniataAhciReadChannelPort4(chan, IDX_AHCI_P_CMD); /* flush */

    /* Allow everything including partial and slumber modes. */
    UniataSataWritePort4(chan, IDX_SATA_SControl, 0, 0);

    /* Request slumber mode transition and give some time to get there. */
    UniataAhciWriteChannelPort4(chan, IDX_AHCI_P_CMD, ATA_AHCI_P_CMD_SLUMBER);
    AtapiStallExecution(100);

    /* Disable PHY. */
    SControl.Reg = 0;
    SControl.DET = SStatus_DET_Offline;
    UniataSataWritePort4(chan, IDX_SATA_SControl, SControl.Reg, 0);

    return;
} // end UniataAhciSuspend()
#endif

BOOLEAN
NTAPI
UniataAhciReadPM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
   OUT PULONG result
    )
{
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    //ULONG            CI;
    //AHCI_IS_REG      IS;
    //ULONG tag=0;
    PIDE_AHCI_CMD  AHCI_CMD = &(chan->AhciCtlBlock->cmd);
    PUCHAR         RCV_FIS = &(chan->AhciCtlBlock->rcv_fis.rfis[0]);

    KdPrint(("UniataAhciReadPM: lChan %d [%#x]\n", chan->lChannel, DeviceNumber));

    if(DeviceNumber == DEVNUM_NOT_SPECIFIED) {
        (*result) = UniataSataReadPort4(chan, Reg, 0);
        return TRUE;
    }
    if(DeviceNumber < AHCI_DEV_SEL_PM) {
        switch(Reg) {
        case IDX_SATA_SStatus:
            Reg = 0; break;
        case IDX_SATA_SError:
            Reg = 1; break;
        case IDX_SATA_SControl:
            Reg = 2; break;
        default:
            return FALSE;
        }
    }

    RtlZeroMemory(AHCI_CMD->cfis, sizeof(AHCI_CMD->cfis));
    AHCI_CMD->cfis[0] = AHCI_FIS_TYPE_ATA_H2D;
    AHCI_CMD->cfis[1] = AHCI_FIS_COMM_PM;
    AHCI_CMD->cfis[2] = IDE_COMMAND_READ_PM;
    AHCI_CMD->cfis[3] = (UCHAR)Reg;
    AHCI_CMD->cfis[7] = (UCHAR)(IDE_USE_LBA | DeviceNumber);
    AHCI_CMD->cfis[15] = IDE_DC_A_4BIT;

    if(UniataAhciSendCommand(chan->DeviceExtension, chan->lChannel, DeviceNumber, 0, 10) == IDE_STATUS_WRONG) {
        KdPrint2(("  PM read failed\n"));
        return FALSE;
    }

    KdDump(RCV_FIS, sizeof(chan->AhciCtlBlock->rcv_fis.rfis));

    (*result) = UniataAhciUlongFromRFIS(RCV_FIS);
    return TRUE;

} // end UniataAhciReadPM()

UCHAR
NTAPI
UniataAhciWritePM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
    IN ULONG value
    )
{
    //ULONG Channel = deviceExtension->Channel + lChannel;
    //ULONG            hIS;
    //ULONG            CI;
    //AHCI_IS_REG      IS;
    //ULONG tag=0;
    ULONG          TFD;
    PIDE_AHCI_CMD  AHCI_CMD = &(chan->AhciCtlBlock->cmd);
    //PUCHAR         RCV_FIS = &(chan->AhciCtlBlock->rcv_fis.rfis[0]);

    KdPrint(("UniataAhciWritePM: lChan %d [%#x] %#x\n", chan->lChannel, DeviceNumber, value));

    if(DeviceNumber == DEVNUM_NOT_SPECIFIED) {
        UniataSataWritePort4(chan, Reg, value, 0);
        return 0;
    }
    if(DeviceNumber < AHCI_DEV_SEL_PM) {
        switch(Reg) {
        case IDX_SATA_SStatus:
            Reg = 0; break;
        case IDX_SATA_SError:
            Reg = 1; break;
        case IDX_SATA_SControl:
            Reg = 2; break;
        default:
            return IDE_STATUS_WRONG;
        }
    }

    RtlZeroMemory(AHCI_CMD->cfis, sizeof(AHCI_CMD->cfis));
    AHCI_CMD->cfis[0] = AHCI_FIS_TYPE_ATA_H2D;
    AHCI_CMD->cfis[1] = AHCI_FIS_COMM_PM;
    AHCI_CMD->cfis[2] = IDE_COMMAND_WRITE_PM;
    AHCI_CMD->cfis[3] = (UCHAR)Reg;
    AHCI_CMD->cfis[7] = (UCHAR)(IDE_USE_LBA | DeviceNumber);

    AHCI_CMD->cfis[12] = (UCHAR)(value & 0xff);
    AHCI_CMD->cfis[4]  = (UCHAR)((value >> 8) & 0xff);
    AHCI_CMD->cfis[5]  = (UCHAR)((value >> 16) & 0xff);
    AHCI_CMD->cfis[6]  = (UCHAR)((value >> 24) & 0xff);

    AHCI_CMD->cfis[15] = IDE_DC_A_4BIT;

    if(UniataAhciSendCommand(chan->DeviceExtension, chan->lChannel, DeviceNumber, 0, 100) == IDE_STATUS_WRONG) {
        KdPrint2(("  PM write failed\n"));
        return IDE_STATUS_WRONG;
    }

    TFD = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_TFD);

    if(TFD & IDE_STATUS_ERROR) {
        KdPrint2(("  ERROR %#x\n", (UCHAR)(TFD >> 8)));
    }
    return (UCHAR)(TFD >> 8);

} // end UniataAhciWritePM()

VOID
UniataAhciSetupCmdPtr(
IN OUT PATA_REQ AtaReq
    )
{
    union {
        PUCHAR prd_base;
        ULONGLONG prd_base64;
    };
    union {
        PUCHAR prd_base0;
        ULONGLONG prd_base64_0;
    };
#ifdef _DEBUG
    ULONG d;
#endif // DBG

    prd_base64_0 = prd_base64 = 0;
    prd_base = (PUCHAR)(&AtaReq->ahci_cmd0);
    prd_base0 = prd_base;

    prd_base64 = (prd_base64 + max(FIELD_OFFSET(ATA_REQ, ahci_cmd0), AHCI_CMD_ALIGNEMENT_MASK+1)) & ~AHCI_CMD_ALIGNEMENT_MASK;

#ifdef _DEBUG
    d = (ULONG)(prd_base64 - prd_base64_0);
    KdPrint2((PRINT_PREFIX "  AtaReq %#x: cmd aligned %I64x, d=%x\n", AtaReq, prd_base64, d));
#endif // DBG

    AtaReq->ahci.ahci_cmd_ptr = (PIDE_AHCI_CMD)prd_base64;
    KdPrint2((PRINT_PREFIX "  ahci_cmd_ptr %#x\n", AtaReq->ahci.ahci_cmd_ptr));
} // end UniataAhciSetupCmdPtr()

PSCSI_REQUEST_BLOCK
NTAPI
BuildAhciInternalSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &deviceExtension->chan[lChannel];
    PSCSI_REQUEST_BLOCK srb;
//    PCDB cdb;
    PATA_REQ AtaReq = chan->AhciInternalAtaReq;

    KdPrint(("BuildAhciInternalSrb: lChan %d [%#x]\n", lChannel, DeviceNumber));

    if(!AtaReq) {
        KdPrint2((PRINT_PREFIX "  !chan->AhciInternalAtaReq\n"));
        return NULL;
    }

    //RtlZeroMemory((PCHAR) AtaReq, sizeof(ATA_REQ));
    //RtlZeroMemory((PCHAR) AtaReq, FIELD_OFFSET(ATA_REQ, ahci));
    UniAtaClearAtaReq(AtaReq);

    srb = chan->AhciInternalSrb;

    RtlZeroMemory((PCHAR) srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->PathId     = (UCHAR)lChannel;
    srb->TargetId   = (UCHAR)DeviceNumber;
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

    // Set flags to disable synchronous negociation.
    //srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    // Set timeout to 4 seconds.
    srb->TimeOutValue = 4;

    srb->CdbLength          = 6;
    srb->DataBuffer         = Buffer;
    srb->DataTransferLength = Length;
    srb->SrbExtension       = AtaReq;

    AtaReq->Srb = srb;
    AtaReq->DataBuffer = (PUSHORT)Buffer;
    AtaReq->TransferLength = Length;

    //if(!AtaReq->ahci.ahci_cmd_ptr) {
        //UniataAhciSetupCmdPtr(AtaReq);
        //AtaReq->ahci.ahci_cmd_ptr = &(chan->AhciCtlBlock->cmd);
        //AtaReq->ahci.ahci_base64 = chan->AHCI_CTL_PhAddr + FIELD_OFFSET(IDE_AHCI_CHANNEL_CTL_BLOCK, cmd);
    //}
    //AtaReq->ahci.ahci_cmd_ptr = &(AtaReq->ahci_cmd0);
    //AtaReq->ahci.ahci_base64 = NULL; // indicate that we should copy command to proper place

    KdPrint2((PRINT_PREFIX "  Srb %#x, AtaReq %#x, CMD %#x ph %I64x\n", srb, AtaReq,
        AtaReq->ahci.ahci_cmd_ptr, AtaReq->ahci.ahci_base64));

/*    // Set CDB operation code.
    cdb = (PCDB)srb->Cdb;
    cdb->CDB6INQUIRY.OperationCode    = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);
*/
    return srb;
} // end BuildAhciInternalSrb()

