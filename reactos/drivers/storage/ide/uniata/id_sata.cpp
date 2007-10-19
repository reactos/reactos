#include "stdafx.h"

UCHAR
UniataSataConnect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel          // logical channel
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

    if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
        KdPrint2((PRINT_PREFIX "  no I/O range\n"));
        return IDE_STATUS_IDLE;
    }

    /* clear SATA error register, some controllers need this */
    AtapiWritePort4(chan, IDX_SATA_SError,
        AtapiReadPort4(chan, IDX_SATA_SError));
    /* wait up to 1 second for "connect well" */
    for(i=0; i<100; i++) {
        SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
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
    AtapiWritePort4(chan, IDX_SATA_SError,
        AtapiReadPort4(chan, IDX_SATA_SError));

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
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel          // logical channel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    SATA_SCONTROL_REG SControl;
    int loop, retry;

    KdPrint2((PRINT_PREFIX "UniataSataPhyEnable:\n"));

    if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
        KdPrint2((PRINT_PREFIX "  no I/O range\n"));
        return IDE_STATUS_IDLE;
    }

    SControl.Reg = AtapiReadPort4(chan, IDX_SATA_SControl);
    KdPrint2((PRINT_PREFIX "SControl %x\n", SControl.Reg));
    if(SControl.DET == SControl_DET_Idle) {
        return UniataSataConnect(HwDeviceExtension, lChannel);
    }

    for (retry = 0; retry < 10; retry++) {
        KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: retry init %d\n", retry));
	for (loop = 0; loop < 10; loop++) {
	    SControl.Reg = 0;
	    SControl.DET = SControl_DET_Init;
            AtapiWritePort4(chan, IDX_SATA_SControl, SControl.Reg);
            AtapiStallExecution(100);
            SControl.Reg = AtapiReadPort4(chan, IDX_SATA_SControl);
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
            AtapiWritePort4(chan, IDX_SATA_SControl, SControl.Reg);
            AtapiStallExecution(100);
            SControl.Reg = AtapiReadPort4(chan, IDX_SATA_SControl);
            KdPrint2((PRINT_PREFIX "  SControl %8.8%x\n", SControl.Reg));
            if(SControl.DET == SControl_DET_Idle) {
                return UniataSataConnect(HwDeviceExtension, lChannel);
	    }
	}
    }

    KdPrint2((PRINT_PREFIX "UniataSataPhyEnable: failed\n"));
    return 0xff;
} // end UniataSataPhyEnable()

BOOLEAN
UniataSataClearErr(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN BOOLEAN do_connect
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;
    SATA_SSTATUS_REG SStatus;
    SATA_SERROR_REG  SError;

    if(deviceExtension->BaseIoAddressSATA_0.Addr) {
    //if(ChipFlags & UNIATA_SATA) {

        SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
        SError.Reg  = AtapiReadPort4(chan, IDX_SATA_SError);

        if(SStatus.Reg) {
            KdPrint2((PRINT_PREFIX "  SStatus %x\n", SStatus.Reg));
        }
        if(SError.Reg) {
            KdPrint2((PRINT_PREFIX "  SError %x\n", SError.Reg));
            /* clear error bits/interrupt */
            AtapiWritePort4(chan, IDX_SATA_SError, SError.Reg);

            if(do_connect) {
                /* if we have a connection event deal with it */
                if(SError.DIAG.N) {
                    KdPrint2((PRINT_PREFIX "  catch SATA connect/disconnect\n"));
                    if(SStatus.SPD >= SStatus_SPD_Gen1) {
                        UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_ATTACH);
                    } else {
                        UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_DETACH);
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
} // end UniataSataClearErr()

BOOLEAN
UniataSataEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG Action
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR Status;
    ULONG ldev = lChannel*2;

    if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
        return FALSE;
    }

    switch(Action) {
    case UNIATA_SATA_EVENT_ATTACH:
        KdPrint2((PRINT_PREFIX "  CONNECTED\n"));
        Status = UniataSataConnect(HwDeviceExtension, lChannel);
        KdPrint2((PRINT_PREFIX "  Status %x\n", Status));
        if(Status != IDE_STATUS_IDLE) {
            return FALSE;
        }
        CheckDevice(HwDeviceExtension, lChannel, 0 /*dev*/, FALSE);
        return TRUE;
        break;
    case UNIATA_SATA_EVENT_DETACH:
        KdPrint2((PRINT_PREFIX "  DISCONNECTED\n"));
        deviceExtension->lun[ldev].DeviceFlags = 0;
        return TRUE;
        break;
    }
    return FALSE;
} // end UniataSataEvent()

BOOLEAN
UniataAhciInit(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG version;
    ULONG c;
    PHW_CHANNEL chan;
    ULONG offs;
    ULONG BaseMemAddress;
    ULONG PI;
    ULONG CAP;
    BOOLEAN MemIo;

    /* reset AHCI controller */
    AtapiWritePortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC,
        AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC) | AHCI_GHC_HR);
    AtapiStallExecution(1000000);
    if(AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC) & AHCI_GHC_HR) {
        KdPrint2((PRINT_PREFIX "  AHCI reset failed\n"));
        return FALSE;
    }

    /* enable AHCI mode */
    AtapiWritePortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC,
        AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC) | AHCI_GHC_AE);

    CAP = AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_CAP);
    /* get the number of HW channels */
    deviceExtension->NumberChannels =
        (CAP & AHCI_CAP_NOP_MASK)+1;
    if(CAP & AHCI_CAP_S64A) {
        KdPrint2((PRINT_PREFIX "  AHCI 64bit\n"));
        deviceExtension->Host64 = TRUE;
    }

    /* clear interrupts */
    AtapiWritePortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_IS,
        AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_IS));

    /* enable AHCI interrupts */
    AtapiWritePortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC,
        AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_GHC) | AHCI_GHC_IE);

    version = AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_VS);
    PI = AtapiReadPortEx4(NULL, (ULONG)&deviceExtension->BaseIoAHCI_0, IDX_AHCI_PI);
    KdPrint2((PRINT_PREFIX "  AHCI version %x%x.%x%x controller with %d ports (mask %x) detected\n",
		  (version >> 24) & 0xff, (version >> 16) & 0xff,
		  (version >> 8) & 0xff, version & 0xff, deviceExtension->NumberChannels, PI));


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

        chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
    }

    return TRUE;
} // end UniataAhciInit()

