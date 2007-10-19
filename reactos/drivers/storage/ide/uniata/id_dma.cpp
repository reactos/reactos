/*++

Copyright (c) 2002-2007 Alexander A. Telyatnikov (Alter)

Module Name:
    id_dma.cpp

Abstract:
    This is the miniport driver for ATAPI IDE controllers
    With Busmaster DMA support

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

    This module is a port from FreeBSD 4.3-6.1 ATA driver (ata-dma.c, ata-chipset.c) by
         Søren Schmidt, Copyright (c) 1998-2007

    Changed defaulting-to-generic-PIO/DMA policy
    Added PIO settings for VIA
    Optimized VIA/AMD/nVidia init part
    Optimized Promise TX2 init part
    Optimized Intel init part
         by Alex A. Telyatnikov (Alter) (c) 2002-2007


--*/

#include "stdafx.h"

static const ULONG valid_udma[7] = {0,0,2,0,4,5,6};

static const CHAR retry_Wdma[MAX_RETRIES+1] = {2, 2, 2,-1,-1,-1};
static const CHAR retry_Udma[MAX_RETRIES+1] = {6, 2,-1,-1,-1,-1};

PHYSICAL_ADDRESS ph4gb = {{0xFFFFFFFF, 0}};

VOID
cyrix_timing (
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    );

VOID
promise_timing (
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    );

VOID
hpt_timing (
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    );

VOID
via82c_timing (
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    );

ULONG
hpt_cable80(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG channel            // physical channel number (0-1)
    );

#define ATAPI_DEVICE(de, ldev)    (de->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE)

ULONG
AtapiVirtToPhysAddr_(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PUCHAR data,
    IN PULONG count,
   OUT PULONG ph_addru
    )
{
    PHYSICAL_ADDRESS ph_addr;
    ULONG addr;

    ph_addr = MmGetPhysicalAddress(data);
    if(!ph_addru && ph_addr.HighPart) {
        // do so until we add 64bit address support
        // or some workaround
        *count = 0;
        return -1;
    }

    (*ph_addru) = ph_addr.HighPart;
    //addr = ScsiPortConvertPhysicalAddressToUlong(ph_addr);
    addr = ph_addr.LowPart;
    if(!addr && !ph_addr.HighPart) {
        *count = 0;
        return 0;
    }
    if(!Srb) {
        *count = sizeof(BM_DMA_ENTRY)*ATA_DMA_ENTRIES;
    } else {
        *count = PAGE_SIZE - (addr & (PAGE_SIZE-1));
    }
    return addr;
} // end AtapiVirtToPhysAddr_()

VOID
AtapiDmaAlloc(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG lChannel          // logical channel,
    )
{
#ifdef USE_OWN_DMA
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &(deviceExtension->chan[lChannel]);
    ULONG c = lChannel;
    ULONG i;
    ULONG ph_addru;

    deviceExtension->chan[c].CopyDmaBuffer = FALSE;

    if(!deviceExtension->Host64 && (WinVer_Id() > WinVer_NT)) {
        KdPrint2((PRINT_PREFIX "AtapiDmaAlloc: allocate tmp buffers below 4Gb\n"));
        chan->DB_PRD = MmAllocateContiguousMemory(sizeof(((PATA_REQ)NULL)->dma_tab), ph4gb);
        if(chan->DB_PRD) {
            chan->DB_PRD_PhAddr = AtapiVirtToPhysAddr(HwDeviceExtension, NULL, (PUCHAR)(chan->DB_PRD), &i, &ph_addru);
            if(!chan->DB_PRD_PhAddr || !i || ((LONG)(chan->DB_PRD_PhAddr) == -1)) {
                KdPrint2((PRINT_PREFIX "AtapiDmaAlloc: No DB PRD BASE\n" ));
                chan->DB_PRD = NULL;
                chan->DB_PRD_PhAddr = 0;
                return;
            }
            if(ph_addru) {
                KdPrint2((PRINT_PREFIX "AtapiDmaAlloc: No DB PRD below 4Gb\n" ));
                goto err_1;
            }
        }
        chan->DB_IO = MmAllocateContiguousMemory(deviceExtension->MaximumDmaTransferLength, ph4gb);
        if(chan->DB_IO) {
            chan->DB_IO_PhAddr = AtapiVirtToPhysAddr(HwDeviceExtension, NULL, (PUCHAR)(chan->DB_IO), &i, &ph_addru);
            if(!chan->DB_IO_PhAddr || !i || ((LONG)(chan->DB_IO_PhAddr) == -1)) {
                KdPrint2((PRINT_PREFIX "AtapiDmaAlloc: No DB IO BASE\n" ));
err_1:
                MmFreeContiguousMemory(chan->DB_PRD);
                chan->DB_PRD = NULL;
                chan->DB_PRD_PhAddr = 0;
                chan->DB_IO = NULL;
                chan->DB_IO_PhAddr = 0;
                return;
            }
            if(ph_addru) {
                KdPrint2((PRINT_PREFIX "AtapiDmaAlloc: No DB IO below 4Gb\n" ));
                MmFreeContiguousMemory(chan->DB_IO);
                goto err_1;
            }
        }
    }
#endif //USE_OWN_DMA

    return;
} // end AtapiDmaAlloc()

BOOLEAN
AtapiDmaSetup(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PUCHAR data,
    IN ULONG count
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG dma_count, dma_base, dma_baseu;
    ULONG i;
    PHW_CHANNEL chan = &(deviceExtension->chan[lChannel]);
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    BOOLEAN use_DB_IO = FALSE;
    ULONG orig_count = count;

    AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;

    KdPrint2((PRINT_PREFIX "AtapiDmaSetup: mode %#x\n", deviceExtension->lun[lChannel*2+DeviceNumber].TransferMode ));
    if(deviceExtension->lun[lChannel*2+DeviceNumber].TransferMode < ATA_DMA) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: Not DMA mode, assume this is just preparation\n" ));
        //return FALSE;
    }
    if(!count) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: count=0\n" ));
        return FALSE;
    }
    if(count > deviceExtension->MaximumDmaTransferLength) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: deviceExtension->MaximumDmaTransferLength > count\n" ));
        return FALSE;
    }
    if((ULONG)data & deviceExtension->AlignmentMask) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: unaligned data: %#x (%#x)\n", data, deviceExtension->AlignmentMask));
        return FALSE;
    }

    // NOTE: This code is intended for 32-bit capable controllers only.
    // 64-bit capable controllers shall be handled separately.
    dma_base = AtapiVirtToPhysAddr(HwDeviceExtension, NULL, (PUCHAR)&(AtaReq->dma_tab) /*chan->dma_tab*/, &i, &dma_baseu);
    if(dma_baseu) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: SRB built-in PRD above 4Gb: %8.8x%8.8x\n", dma_baseu, dma_base));
        dma_base = chan->DB_PRD_PhAddr;
        AtaReq->Flags |= REQ_FLAG_DMA_DBUF_PRD;
        i = 1;
    } else
    if(!dma_base || !i || ((LONG)(dma_base) == -1)) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: No BASE\n" ));
        return FALSE;
    }
    AtaReq->dma_base = dma_base;

    dma_base = AtapiVirtToPhysAddr(HwDeviceExtension, Srb, data, &dma_count, &dma_baseu);
    if(dma_baseu) {
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: 1st block of buffer above 4Gb: %8.8x%8.8x\n", dma_baseu, dma_base));
retry_DB_IO:
        use_DB_IO = TRUE;
        dma_base = chan->DB_IO_PhAddr;
        data = (PUCHAR)(chan->DB_IO);
    } else
    if(!dma_count || ((LONG)(dma_base) == -1)) {
        AtaReq->dma_base = 0;
        KdPrint2((PRINT_PREFIX "AtapiDmaSetup: No 1st block\n" ));
        return FALSE;
    }

    dma_count = min(count, (PAGE_SIZE - ((ULONG)data & PAGE_MASK)));
    data += dma_count;
    count -= dma_count;
    i = 0;

    while (count) {
        AtaReq->dma_tab[i].base = dma_base;
        AtaReq->dma_tab[i].count = (dma_count & 0xffff);
        i++;
        if (i >= ATA_DMA_ENTRIES) {
            AtaReq->dma_base = 0;
            KdPrint2((PRINT_PREFIX "too many segments in DMA table\n" ));
            return FALSE;
        }
        dma_base = AtapiVirtToPhysAddr(HwDeviceExtension, Srb, data, &dma_count, &dma_baseu);
        if(dma_baseu) {
            KdPrint2((PRINT_PREFIX "AtapiDmaSetup: block of buffer above 4Gb: %8.8x%8.8x\n", dma_baseu, dma_base));
            if(use_DB_IO) {
                KdPrint2((PRINT_PREFIX "AtapiDmaSetup: *ERROR* special buffer above 4Gb: %8.8x%8.8x\n", dma_baseu, dma_base));
                return FALSE;
            }
            count = orig_count;
            goto retry_DB_IO;
        } else
        if(!dma_count || !dma_base || ((LONG)(dma_base) == -1)) {
            AtaReq->dma_base = 0;
            KdPrint2((PRINT_PREFIX "AtapiDmaSetup: No NEXT block\n" ));
            return FALSE;
        }

        dma_count = min(count, PAGE_SIZE);
        data += min(count, PAGE_SIZE);
        count -= min(count, PAGE_SIZE);
    }
    AtaReq->dma_tab[i].base = dma_base;
    AtaReq->dma_tab[i].count = (dma_count & 0xffff) | ATA_DMA_EOT;

    if(use_DB_IO) {
        AtaReq->Flags |= REQ_FLAG_DMA_DBUF;
    }
    AtaReq->Flags |= REQ_FLAG_DMA_OPERATION;

    KdPrint2((PRINT_PREFIX "AtapiDmaSetup: OK\n" ));
    return TRUE;

} // end AtapiDmaSetup()

BOOLEAN
AtapiDmaPioSync(
    PVOID  HwDeviceExtension,
    PSCSI_REQUEST_BLOCK Srb,
    PUCHAR data,
    ULONG  count
    )
{
#ifndef USE_OWN_DMA
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG dma_base;
    PUCHAR DmaBuffer;
    ULONG dma_count;
    ULONG len;
    PATA_REQ AtaReq;

    // This must never be called after DMA operation !!!
    KdPrint2((PRINT_PREFIX "AtapiDmaPioSync: data %#x, len %#x\n", data, count));

    if(!Srb) {
        KdPrint2((PRINT_PREFIX "AtapiDmaPioSync: !Srb\n" ));
        return FALSE;
    }

    AtaReq = (PATA_REQ)(Srb->SrbExtension);

    // do nothing on PCI (This can be changed. We cannot guarantee,
    // that CommonBuffer will always point to User's buffer,
    // however, this usually happens on PCI-32)
    if(deviceExtension->OrigAdapterInterfaceType == PCIBus) {
        return TRUE;
    }
    // do nothing for DMA
    if(AtaReq->Flags & REQ_FLAG_DMA_OPERATION) {
        return TRUE;
    }

    if(!data) {
        KdPrint2((PRINT_PREFIX "AtapiDmaPioSync: !data\n" ));
        return FALSE;
    }

    while(count) {
        dma_base = AtapiVirtToPhysAddr(HwDeviceExtension, Srb, data, &dma_count);
        if(!dma_base) {
            KdPrint2((PRINT_PREFIX "AtapiDmaPioSync: !dma_base for data %#x\n", data));
            return FALSE;
        }
        DmaBuffer = (PUCHAR)ScsiPortGetVirtualAddress(HwDeviceExtension,
                                                      ScsiPortConvertUlongToPhysicalAddress(dma_base));
        if(!DmaBuffer) {
            KdPrint2((PRINT_PREFIX "AtapiDmaPioSync: !DmaBuffer for dma_base %#x\n", dma_base));
            return FALSE;
        }
        len = min(dma_count, count);
        memcpy(DmaBuffer, data, len);
        count -= len;
        data += len;
    }
#endif //USE_OWN_DMA

    return TRUE;
} // end AtapiDmaPioSync()

BOOLEAN
AtapiDmaDBSync(
    PHW_CHANNEL chan,
    PSCSI_REQUEST_BLOCK Srb
    )
{
    PATA_REQ AtaReq;

    AtaReq = (PATA_REQ)(Srb->SrbExtension);
    if((Srb->SrbFlags & SRB_FLAGS_DATA_IN) &&
       (AtaReq->Flags & REQ_FLAG_DMA_DBUF)) {
        KdPrint2((PRINT_PREFIX "  AtapiDmaDBSync is issued.\n"));
        ASSERT(FALSE);
        KdPrint2((PRINT_PREFIX "  DBUF (Read)\n"));
        RtlCopyMemory(AtaReq->DataBuffer, chan->DB_IO,
                              Srb->DataTransferLength);
    }
    return TRUE;
} // end AtapiDmaDBSync()

VOID
AtapiDmaStart(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //PIDE_BUSMASTER_REGISTERS BaseIoAddressBM = deviceExtension->BaseIoAddressBM[lChannel];
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];

    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;

    KdPrint2((PRINT_PREFIX "AtapiDmaStart: %s on %#x:%#x\n",
        (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? "read" : "write",
        lChannel, DeviceNumber ));

    if(!AtaReq->dma_base) {
        KdPrint2((PRINT_PREFIX "AtapiDmaStart: *** !AtaReq->dma_base\n"));
        return;
    }
    if(AtaReq->Flags & REQ_FLAG_DMA_DBUF_PRD) {
        KdPrint2((PRINT_PREFIX "  DBUF_PRD\n"));
        ASSERT(FALSE);
        RtlCopyMemory(chan->DB_PRD, &(AtaReq->dma_tab), sizeof(AtaReq->dma_tab));
    }
    if(!(Srb->SrbFlags & SRB_FLAGS_DATA_IN) &&
       (AtaReq->Flags & REQ_FLAG_DMA_DBUF)) {
        KdPrint2((PRINT_PREFIX "  DBUF (Write)\n"));
        ASSERT(FALSE);
        RtlCopyMemory(chan->DB_IO, AtaReq->DataBuffer,
                              Srb->DataTransferLength);
    }

    // set flag
    chan->ChannelCtrlFlags |= CTRFLAGS_DMA_ACTIVE;

    switch(VendorID) {
    case ATA_PROMISE_ID:
        if(ChipType == PRNEW) {
            ULONG Channel = deviceExtension->Channel + lChannel;
            if(chan->ChannelCtrlFlags & CTRFLAGS_LBA48) {
                AtapiWritePortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11,
                      AtapiReadPortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11) |
                          (Channel ? 0x08 : 0x02));
                AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),(Channel ? 0x24 : 0x20),
                      ((Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? 0x05000000 : 0x06000000) | (Srb->DataTransferLength >> 1)
                      );
            }
/*
        } else
        if(deviceExtension->MemIo) {
            // begin transaction
            AtapiWritePort4(chan,
                 IDX_BM_Command,
                 (AtapiReadPort4(chan,
                     IDX_BM_Command) & ~0x000000c0) |
                     ((Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? 0x00000080 : 0x000000c0) );
            return;
*/
        }
        break;
    }

    // set pointer to Pointer Table
    AtapiWritePort4(chan, IDX_BM_PRD_Table,
          AtaReq->dma_base
          );
    // set transfer direction
    AtapiWritePort1(chan, IDX_BM_Command,
          (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? BM_COMMAND_READ : BM_COMMAND_WRITE);
    // clear Error & Intr bits (writeing 1 clears bits)
    // set DMA capability bit
    AtapiWritePort1(chan, IDX_BM_Status,
         AtapiReadPort1(chan, IDX_BM_Status) |
             (BM_STATUS_INTR | BM_STATUS_ERR) /*|
             (DeviceNumber ? BM_STATUS_DRIVE_1_DMA : BM_STATUS_DRIVE_0_DMA)*/);
    // begin transaction
    AtapiWritePort1(chan, IDX_BM_Command,
         AtapiReadPort1(chan, IDX_BM_Command) |
             BM_COMMAND_START_STOP);
    return;

} // end AtapiDmaStart()

UCHAR
AtapiDmaDone(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //PIDE_BUSMASTER_REGISTERS BaseIoAddressBM = deviceExtension->BaseIoAddressBM[lChannel];
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    UCHAR dma_status;

    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;

    KdPrint2((PRINT_PREFIX "AtapiDmaDone: dev %d\n", DeviceNumber));

    switch(VendorID) {
    case ATA_PROMISE_ID:
        if(ChipType == PRNEW) {
            ULONG Channel = deviceExtension->Channel + lChannel;
            if(chan->ChannelCtrlFlags & CTRFLAGS_LBA48) {
                AtapiWritePortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11,
                      AtapiReadPortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11) &
                          ~(Channel ? 0x08 : 0x02));
                AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),(Channel ? 0x24 : 0x20),
                      0
                      );
            }
/*
        } else
        if(deviceExtension->MemIo) {
            // end transaction
            AtapiWritePort4(chan,
                 IDX_BM_Command,
                 (AtapiReadPort4(chan,
                     IDX_BM_Command) & ~0x00000080) );
            // clear flag
            chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_ACTIVE;
            return 0;
*/
        }
        break;
    }

    // get status
    dma_status = AtapiReadPort1(chan, IDX_BM_Status) & BM_STATUS_MASK;
    // end transaction
    AtapiWritePort1(chan, IDX_BM_Command,
         AtapiReadPort1(chan, IDX_BM_Command) &
             ~BM_COMMAND_START_STOP);
    // clear interrupt and error status
    AtapiWritePort1(chan, IDX_BM_Status, BM_STATUS_ERR | BM_STATUS_INTR);
    // clear flag
    chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_ACTIVE;

    return dma_status;

} // end AtapiDmaDone()

VOID
AtapiDmaReinit(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG ldev,
    IN PATA_REQ AtaReq
    )
{
    PHW_LU_EXTENSION LunExt = &(deviceExtension->lun[ldev]);
    CHAR apiomode;

    apiomode = (CHAR)AtaPioMode(&(LunExt->IdentifyData));

    if(!(AtaReq->Flags & REQ_FLAG_DMA_OPERATION)) {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaReinit: !(AtaReq->Flags & REQ_FLAG_DMA_OPERATION), fall to PIO on Device %d\n", ldev & 1));
        goto limit_pio;
    }
    if(!AtaReq->dma_base) {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaReinit: no PRD, fall to PIO on Device %d\n", ldev & 1));
        goto limit_pio;
    }

    if((deviceExtension->HbaCtrlFlags & HBAFLAGS_DMA_DISABLED_LBA48) &&
       (AtaReq->lba >= ATA_MAX_LBA28) &&
       (LunExt->TransferMode > ATA_PIO5) ) {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaReinit: FORCE_DOWNRATE on Device %d for LBA48\n", ldev & 1));
        goto limit_lba48;
    }


    if(AtaReq->Flags & REQ_FLAG_FORCE_DOWNRATE) {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaReinit: FORCE_DOWNRATE on Device %d\n", ldev & 1));
        if(AtaReq->lba >= ATA_MAX_LBA28) {
limit_lba48:
            LunExt->DeviceFlags |= REQ_FLAG_FORCE_DOWNRATE_LBA48;
limit_pio:
            // do not make extra work if we already use PIO
            KdPrint2((PRINT_PREFIX
                        "AtapiDmaReinit: set PIO mode on Device %d (%x -> %x)\n", ldev & 1, LunExt->TransferMode, ATA_PIO0+apiomode));
            if(/*LunExt->TransferMode >= ATA_DMA*/
               LunExt->TransferMode != ATA_PIO0+apiomode
               ) {
                AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                             apiomode,
                             -1,
                             -1 );
            }
        } else {
            KdPrint2((PRINT_PREFIX
                        "AtapiDmaReinit: set MAX mode on Device %d\n", ldev & 1));
            AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                         apiomode,
                         min( retry_Wdma[AtaReq->retry],
                              (CHAR)AtaWmode(&(LunExt->IdentifyData)) ),
                         min( retry_Udma[AtaReq->retry],
                              (CHAR)AtaUmode(&(LunExt->IdentifyData)) ) );
        }
//            LunExt->DeviceFlags &= ~DFLAGS_FORCE_DOWNRATE;
    } else
    if(/*!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_FORCE_DOWNRATE) &&*/
        (LunExt->LimitedTransferMode >
         LunExt->TransferMode) ||
         (LunExt->DeviceFlags & DFLAGS_REINIT_DMA)) {
        // restore IO mode
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaReinit: restore IO mode on Device %d\n", ldev & 1));
        AtapiDmaInit__(deviceExtension, ldev);
    }
} // end AtapiDmaReinit()

VOID
AtapiDmaInit__(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG ldev
    )
{
    PHW_LU_EXTENSION LunExt = &(deviceExtension->lun[ldev]);

    if(LunExt->IdentifyData.SupportDma) {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaInit__: Set (U)DMA on Device %d\n", ldev & 1));
/*        for(i=AtaUmode(&(LunExt->IdentifyData)); i>=0; i--) {
            AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                         (CHAR)AtaPioMode(&(LunExt->IdentifyData)),
                         (CHAR)AtaWmode(&(LunExt->IdentifyData)),
                         UDMA_MODE0+(CHAR)i );
        }
        for(i=AtaWmode(&(LunExt->IdentifyData)); i>=0; i--) {
            AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                         (CHAR)AtaPioMode(&(LunExt->IdentifyData)),
                         (CHAR)AtaWmode(&(LunExt->IdentifyData)),
                         UDMA_MODE0+(CHAR)i );
        }*/
        AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                     (CHAR)AtaPioMode(&(LunExt->IdentifyData)),
                     (CHAR)AtaWmode(&(LunExt->IdentifyData)),
                     (CHAR)AtaUmode(&(LunExt->IdentifyData)) );
    } else {
        KdPrint2((PRINT_PREFIX
                    "AtapiDmaInit__: Set PIO on Device %d\n", ldev & 1));
        AtapiDmaInit(deviceExtension, ldev & 1, ldev >> 1,
                     (CHAR)AtaPioMode(&(LunExt->IdentifyData)), -1, -1);
    }
} // end AtapiDmaInit__()

BOOLEAN
AtaSetTransferMode(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
    IN PHW_LU_EXTENSION LunExt,
    IN ULONG mode
    )
{
    KdPrint2((PRINT_PREFIX
                "AtaSetTransferMode: Set %#x on Device %d/%d\n", mode, lChannel, DeviceNumber));
    LONG statusByte = 0;
    CHAR apiomode;

    statusByte = AtaCommand(deviceExtension, DeviceNumber, lChannel,
                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                        (UCHAR)((mode > ATA_UDMA6) ? ATA_UDMA6 : mode), ATA_C_F_SETXFER, ATA_WAIT_BASE_READY);
    if(statusByte & IDE_STATUS_ERROR) {
        KdPrint2((PRINT_PREFIX "  wait ready after error\n"));
        if(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) {
            AtapiStallExecution(10);
        } else {
            AtapiStallExecution(100);
        }
        apiomode = (CHAR)AtaPioMode(&(LunExt->IdentifyData));
        if(  apiomode > 0 &&
             (CHAR)AtaWmode(&(LunExt->IdentifyData))   > 0 &&
             (CHAR)AtaUmode(&(LunExt->IdentifyData))   > 0
          ) {
            return FALSE;
        }
        if(mode > ATA_PIO2) {
            return FALSE;
        }
        KdPrint2((PRINT_PREFIX "  assume that drive doesn't support mode swithingm using PIO%d\n", apiomode));
        mode = ATA_PIO0 + apiomode;
    }
    //if(mode <= ATA_UDMA6) {
        LunExt->TransferMode = (UCHAR)mode;
    //}
    return TRUE;
} // end AtaSetTransferMode()

VOID
AtapiDmaInit(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,          // logical channel,
                               // is always 0 except simplex-only controllers
    IN CHAR apiomode,
    IN CHAR wdmamode,
    IN CHAR udmamode
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG Channel = deviceExtension->Channel + lChannel;
    PHW_CHANNEL chan = &deviceExtension->chan[lChannel];
    //LONG statusByte = 0;
    ULONG dev = Channel*2 + DeviceNumber;
    ULONG ldev = lChannel*2 + DeviceNumber;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    LONG i;
    PHW_LU_EXTENSION LunExt = &(deviceExtension->lun[ldev]);

    ULONG VendorID  =  deviceExtension->DevID        & 0xffff;
    //ULONG DeviceID  = (deviceExtension->DevID >> 16) & 0xffff;
    //ULONG RevID     =  deviceExtension->RevID;
    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;

    LONG statusByte = 0;

    //UCHAR *reg_val = NULL;

    LunExt->DeviceFlags &= ~DFLAGS_REINIT_DMA;
    /* set our most pessimistic default mode */
    LunExt->TransferMode = ATA_PIO;
//    if(!deviceExtension->BaseIoAddressBM[lChannel]) {
    if(!deviceExtension->BusMaster) {
        KdPrint2((PRINT_PREFIX "  !deviceExtension->BusMaster: NO DMA\n"));
        wdmamode = udmamode = -1;
    }

    // Limit transfer mode (controller limitation)
    if((LONG)deviceExtension->MaxTransferMode >= ATA_UDMA) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: deviceExtension->MaxTransferMode >= ATA_UDMA\n"));
        udmamode = min( udmamode, (CHAR)(deviceExtension->MaxTransferMode - ATA_UDMA));
    } else
    if((LONG)deviceExtension->MaxTransferMode >= ATA_WDMA) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: deviceExtension->MaxTransferMode >= ATA_WDMA\n"));
        udmamode = -1;
        wdmamode = min( wdmamode, (CHAR)(deviceExtension->MaxTransferMode - ATA_WDMA));
    } else
    if((LONG)deviceExtension->MaxTransferMode >= ATA_PIO0) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: NO DMA\n"));
        wdmamode = udmamode = -1;
        apiomode = min( apiomode, (CHAR)(deviceExtension->MaxTransferMode - ATA_PIO0));
    } else {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: PIO0\n"));
        wdmamode = udmamode = -1;
        apiomode = 0;
    }
    // Limit transfer mode (device limitation)
    KdPrint2((PRINT_PREFIX "AtapiDmaInit: LunExt->LimitedTransferMode %#x\n", LunExt->LimitedTransferMode));
    if((LONG)LunExt->LimitedTransferMode >= ATA_UDMA) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: LunExt->MaxTransferMode >= ATA_UDMA   =>   %#x\n",
         min( udmamode, (CHAR)(LunExt->LimitedTransferMode - ATA_UDMA))
                ));
        udmamode = min( udmamode, (CHAR)(LunExt->LimitedTransferMode - ATA_UDMA));
    } else
    if((LONG)LunExt->LimitedTransferMode >= ATA_WDMA) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: LunExt->MaxTransferMode >= ATA_WDMA   =>   %#x\n",
         min( wdmamode, (CHAR)(LunExt->LimitedTransferMode - ATA_WDMA))
                ));
        udmamode = -1;
        wdmamode = min( wdmamode, (CHAR)(LunExt->LimitedTransferMode - ATA_WDMA));
    } else
    if((LONG)LunExt->LimitedTransferMode >= ATA_PIO0) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: lun NO DMA\n"));
        wdmamode = udmamode = -1;
        apiomode = min( apiomode, (CHAR)(LunExt->LimitedTransferMode - ATA_PIO0));
    } else {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: lun PIO0\n"));
        wdmamode = udmamode = -1;
        apiomode = 0;
    }

    SelectDrive(chan, DeviceNumber);
    GetStatus(chan, statusByte);
    // we can see here IDE_STATUS_ERROR status after previous operation
    if(statusByte & IDE_STATUS_ERROR) {
        KdPrint2((PRINT_PREFIX "IDE_STATUS_ERROR detected on entry, statusByte = %#x\n", statusByte));
        //GetBaseStatus(chan, statusByte);
    }
    if(statusByte && UniataIsIdle(deviceExtension, statusByte & ~IDE_STATUS_ERROR) != IDE_STATUS_IDLE) {
        KdPrint2((PRINT_PREFIX "Can't setup transfer mode: statusByte = %#x\n", statusByte));
        return;
    }

    if(deviceExtension->UnknownDev) {
        KdPrint2((PRINT_PREFIX "Unknown chip, omit Vendor/Dev checks\n"));
        goto try_generic_dma;
    }

    if(deviceExtension->BaseIoAddressSATA_0.Addr) {
    //if(ChipFlags & UNIATA_SATA) {
        /****************/
        /* SATA Generic */
        /****************/
        UCHAR ModeByte;

        KdPrint2((PRINT_PREFIX "SATA Generic\n"));
        if(udmamode > 5) {
            if(LunExt->IdentifyData.SataCapabilities != 0x0000 &&
               LunExt->IdentifyData.SataCapabilities != 0xffff) {
                //udmamode = min(udmamode, 6);
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_SA150)) {
                    return;
                }
                udmamode = min(udmamode, 5);

            } else {
                KdPrint2((PRINT_PREFIX "SATA -> PATA adapter ?\n"));
                if (udmamode > 2 && !LunExt->IdentifyData.HwResCableId) {
                    KdPrint2((PRINT_PREFIX "AtapiDmaInit: DMA limited to UDMA33, non-ATA66 compliant cable\n"));
                    udmamode = 2;
                    apiomode = min( apiomode, (CHAR)(LunExt->LimitedTransferMode - ATA_PIO0));
                } else {
                    udmamode = min(udmamode, 5);
                }
            }
        }
        if(udmamode >= 0) {
            ModeByte = ATA_UDMA0 + udmamode;
        } else
        if(wdmamode >= 0) {
            ModeByte = ATA_WDMA0 + wdmamode;
        } else
        if(apiomode >= 0) {
            ModeByte = ATA_PIO0  + apiomode;
        } else {
            ModeByte = ATA_PIO;
        }

        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ModeByte);
        return;
    }
    if (udmamode > 2 && !LunExt->IdentifyData.HwResCableId) {
        KdPrint2((PRINT_PREFIX "AtapiDmaInit: DMA limited to UDMA33, non-ATA66 compliant cable\n"));
        udmamode = 2;
        apiomode = min( apiomode, (CHAR)(LunExt->LimitedTransferMode - ATA_PIO));
    }

    KdPrint2((PRINT_PREFIX "Setup chip a:w:u=%d:%d:%d\n",
        apiomode,
        wdmamode,
        udmamode));

    switch(VendorID) {
    case ATA_ACARD_ID: {
        /*********/
        /* Acard */
        /*********/
        static const USHORT reg4a = 0xa6;
        UCHAR  reg = 0x40 + (UCHAR)dev;

        if(ChipType == ATPOLD) {
            /* Old Acard 850 */
            static const USHORT reg4x = 0x0301;

            for(i=udmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA + i)) {
set_old_acard:
                    ChangePciConfig1(0x54, a | (0x01 << dev) | ((i+1) << (dev*2)));
                    SetPciConfig1(0x4a, reg4a);
                    SetPciConfig2(reg,  reg4x);
                    return;
                }

            }
            if (wdmamode >= 2 && apiomode >= 4) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA2)) {
                    goto set_old_acard;
                }
            }
        } else {
            /* New Acard 86X */
            static const UCHAR reg4x = 0x31;

            for(i=udmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA + i)) {
set_new_acard:
                    ChangePciConfig2(0x44, (a & ~(0x000f << (dev * 4))) | ((i+1) << (dev*4)));
                    SetPciConfig1(0x4a, reg4a);
                    SetPciConfig1(reg,  reg4x);
                    return;
                }

            }
            if (wdmamode >= 2 && apiomode >= 4) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA2)) {
                    goto set_new_acard;
                }
            }
        }
        /* Use GENERIC PIO */
        break; }
    case ATA_ACER_LABS_ID: {
        /************************/
        /* Acer Labs Inc. (ALI) */
        /************************/
        static const UCHAR ali_udma[] = {0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x0f};
        static const ULONG ali_pio[]  =
                { 0x006d0003, 0x00580002, 0x00440001, 0x00330001,
                  0x00310001, 0x00440001};
        /* the older Aladdin doesn't support ATAPI DMA on both master & slave */
        if ((ChipFlags & ALIOLD) &&
            (udmamode >= 0 || wdmamode >= 0)) {
            if(ATAPI_DEVICE(deviceExtension, lChannel*2) &&
               ATAPI_DEVICE(deviceExtension, lChannel*2 + 1)) {
                // 2 devices on this channel - NO DMA
                chan->MaxTransferMode =
                    min(chan->MaxTransferMode, ATA_PIO4);
                udmamode = wdmamode = -1;
                break;
            }
        }
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                ULONG word54;

                GetPciConfig4(0x54, word54);
                word54 &= ~(0x000f000f << (dev * 4));
                word54 |= (((ali_udma[i]<<16) | 5) << (dev * 4));
                SetPciConfig4(0x54, word54);
                ChangePciConfig1(0x53, a | 0x03);
                SetPciConfig4(0x58 + (Channel<<2), 0x00310001);
                return;
            }
        }
        /* make sure eventual UDMA mode from the BIOS is disabled */
        ChangePciConfig2(0x56, a & ~(0x0008 << (dev * 4)) );
        if (wdmamode >= 2 && apiomode >= 4) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA2)) {
                ChangePciConfig1(0x53, a | 0x03);
                chan->ChannelCtrlFlags |= CTRFLAGS_DMA_RO;
                return;
            }
        }
        ChangePciConfig1(0x53, (a & ~0x01) | 0x02);

        for(i=apiomode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + i)) {
                ChangePciConfig4(0x54, a & ~(0x0008000f << (dev * 4)));
                SetPciConfig4(0x58 + (Channel<<2), ali_pio[i]);
                return;
            }
        }
        return;
        break; }
    case ATA_AMD_ID:
    case ATA_NVIDIA_ID:
    case ATA_VIA_ID: {
        /********************/
        /* AMD, nVidia, VIA */
        /********************/
        static const UCHAR via_modes[5][7] = {
            { 0xc2, 0xc1, 0xc0, 0x00, 0x00, 0x00, 0x00 },        /* ATA33 and New Chips */
            { 0xee, 0xec, 0xea, 0xe9, 0xe8, 0x00, 0x00 },        /* ATA66 */
            { 0xf7, 0xf6, 0xf4, 0xf2, 0xf1, 0xf0, 0x00 },        /* ATA100 */
            { 0xf7, 0xf7, 0xf6, 0xf4, 0xf2, 0xf1, 0xf0 },        /* VIA ATA133 */
            { 0xc2, 0xc1, 0xc0, 0xc4, 0xc5, 0xc6, 0xc7 }};       /* AMD/nVIDIA */
	static const UCHAR via_pio[] =
            { 0xa8, 0x65, 0x42, 0x22, 0x20, 0x42, 0x22, 0x20,
              0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };
        const UCHAR *reg_val = NULL;
        UCHAR reg = 0x53-(UCHAR)dev;

        reg_val = &via_modes[ChipType][0];

        if(VendorID == ATA_NVIDIA_ID)
            reg += 0x10;

        for(i = udmamode; i>=0; i--) {
            SetPciConfig1(reg-0x08, via_pio[8+i]);
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                SetPciConfig1(reg, (UCHAR)reg_val[i]);
                return;
            }
        }
        for(i = wdmamode; i>=0; i--) {
            SetPciConfig1(reg-0x08, via_pio[5+i]);
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                SetPciConfig1(reg, 0x8b);
                return;
            }
        }
        /* set PIO mode timings */
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        if((apiomode >= 0) && (ChipType != VIA133)) {
            SetPciConfig1(reg-0x08, via_pio[apiomode]);
        }
        via82c_timing(deviceExtension, dev, ATA_PIO0 + apiomode);
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        return;

        break; }
    case ATA_CYRIX_ID: {
        /*********/
        /* Cyrix */
        /*********/
        ULONG cyr_piotiming[] =
            { 0x00009172, 0x00012171, 0x00020080, 0x00032010, 0x00040010 };
        ULONG cyr_wdmatiming[] = { 0x00077771, 0x00012121, 0x00002020 };
        ULONG cyr_udmatiming[] = { 0x00921250, 0x00911140, 0x00911030 };
        ULONG mode_reg = 0x24+(dev << 3);

        if(apiomode >= 4)
            apiomode = 4;
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0), mode_reg, cyr_udmatiming[udmamode]);
                return;
            }
        }
        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0), mode_reg, cyr_wdmatiming[wdmamode]);
                return;
            }
        }
        if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode)) {
            AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0), mode_reg, cyr_piotiming[apiomode]);
            return;
        }
        return;

        break; }
    case ATA_NATIONAL_ID: {
        /************/
        /* National */
        /************/
        ULONG nat_piotiming[] =
           { 0x9172d132, 0x21717121, 0x00803020, 0x20102010, 0x00100010,
              0x00803020, 0x20102010, 0x00100010,
              0x00100010, 0x00100010, 0x00100010 };
        ULONG nat_dmatiming[] = { 0x80077771, 0x80012121, 0x80002020 };
        ULONG nat_udmatiming[] = { 0x80921250, 0x80911140, 0x80911030 };

        if(apiomode >= 4)
            apiomode = 4;
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                SetPciConfig4(0x44 + (dev * 8), nat_udmatiming[i]);
                SetPciConfig4(0x40 + (dev * 8), nat_piotiming[i+8]);
                return;
            }
        }
        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                SetPciConfig4(0x44 + (dev * 8), nat_dmatiming[i]);
                SetPciConfig4(0x40 + (dev * 8), nat_piotiming[i+5]);
                return;
            }
        }
        if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode)) {
            ChangePciConfig4(0x44 + (dev * 8), a | 0x80000000);
            SetPciConfig4(0x40 + (dev * 8), nat_piotiming[apiomode]);
            return;
        }
        /* Use GENERIC PIO */
        break; }
    case ATA_CYPRESS_ID:
        /***********/
        /* Cypress */
        /***********/
        if (wdmamode >= 2 && apiomode >= 4) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA2)) {
                SetPciConfig2(Channel ? 0x4e:0x4c, 0x2020);
                return;
            }
        }
        /* Use GENERIC PIO */
        break;
    case ATA_HIGHPOINT_ID: {
        /********************/
        /* High Point (HPT) */
        /********************/
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                hpt_timing(deviceExtension, dev, (UCHAR)(ATA_UDMA + i));       // ???
                return;
            }
        }

        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                hpt_timing(deviceExtension, dev, (UCHAR)(ATA_WDMA0+i));
                return;
            }
        }
        /* try generic DMA, use hpt_timing() */
        if (wdmamode >= 0 && apiomode >= 4) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_DMA)) {
                return;
            }
        }
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        hpt_timing(deviceExtension, dev, ATA_PIO0 + apiomode);
        return;
        break; }
    case ATA_INTEL_ID: {
        /*********/
        /* Intel */
        /*********/

        BOOLEAN udma_ok = FALSE;
        ULONG  idx = 0;
        ULONG  reg40;
        UCHAR  reg44;
        USHORT reg48;
        USHORT reg4a;
        USHORT reg54;
        ULONG  mask40 = 0;
        ULONG  new40  = 0;
        UCHAR  mask44 = 0;
        UCHAR  new44  = 0;
        UCHAR  intel_timings[] = { 0x00, 0x00, 0x10, 0x21, 0x23, 0x10, 0x21, 0x23,
    		                   0x23, 0x23, 0x23, 0x23, 0x23, 0x23 };

        if(deviceExtension->DevID == ATA_I82371FB) {
            if (wdmamode >= 2 && apiomode >= 4) {
                ULONG word40;

                GetPciConfig4(0x40, word40);
                word40 >>= Channel * 16;

                /* Check for timing config usable for DMA on controller */
                if (!((word40 & 0x3300) == 0x2300 &&
                      ((word40 >> ((!(DeviceNumber & 1)) ? 0 : 4)) & 1) == 1)) {
                    udmamode = wdmamode = -1;
                    break;
                }

                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA2)) {
                    return;
                }
            }
            break;
        }

        GetPciConfig2(0x48, reg48);
        if(!(ChipFlags & ICH4_FIX)) {
            GetPciConfig2(0x4a, reg4a);
        }
        GetPciConfig2(0x54, reg54);
//        if(udmamode >= 0) {
            // enable the write buffer to be used in a split (ping/pong) manner.
            reg54 |= 0x400;
//        } else {
//            reg54 &= ~0x400;
//        }

//        reg40 &= ~0x00ff00ff;
//        reg40 |= 0x40774077;

        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {

                SetPciConfig1(0x48, reg48 | (0x0001 << dev));
                if(!(ChipFlags & ICH4_FIX)) {
                    SetPciConfig2(0x4a, (reg4a & ~(0x3 << (dev<<2))) |
                                        (0x01 + !(i & 0x01))  );
                }
                if(i >= 2) {
                    reg54 |= (0x1 << dev);
                } else {
                    reg54 &= ~(0x1 << dev);
                }
                if(i >= 5) {
                    reg54 |= (0x1000 << dev);
                } else {
                    reg54 &= ~(0x1000 << dev);
                }
                SetPciConfig2(0x54, reg54);

                udma_ok = TRUE;
                idx = i+8;
                if(ChipFlags & ICH4_FIX) {
                    return;
                }
                break;
            }
        }

        if(!udma_ok) {
            SetPciConfig1(0x48, reg48 & ~(0x0001 << dev));
            if(!(ChipFlags & ICH4_FIX)) {
                SetPciConfig2(0x4a, (reg4a & ~(0x3 << (dev << 2)))  );
            }
            SetPciConfig2(0x54, reg54 & ~(0x1001 << dev));
            for(i=wdmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                    udma_ok = TRUE;
                    idx = i+5;
                    if(ChipFlags & ICH4_FIX) {
                        return;
                    }
                }
            }
        }

        if(!udma_ok) {
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
            idx = apiomode;
        }

        GetPciConfig4(0x40, reg40);
        GetPciConfig1(0x44, reg44);

        mask40 = 0x000000ff;

        if(!(DeviceNumber & 1)) {
            mask40 |= 0x00003300;
            new40 = ((USHORT)(intel_timings[idx]) << 8);
            //mask44 = 0x00; // already done on init
            //new44  = 0x00;
        } else {
            mask44 = 0x0f;
            new44 = ((intel_timings[idx] & 0x30) >> 2) |
                     (intel_timings[idx] & 0x03);
        }
        new40 |= 0x00004077;

        if (Channel) {
            mask40 <<= 16;
            new40 <<= 16;
            mask44 <<= 4;
            new44 <<= 4;
        }

        SetPciConfig4(0x40, (reg40 & ~mask40) | new40);
        SetPciConfig1(0x44, (reg44 & ~mask44) | new44);

        return;
        break; }
    case ATA_PROMISE_ID:
        /***********/
        /* Promise */
        /***********/
        if(ChipType < PRTX) {
            if (ATAPI_DEVICE(deviceExtension, ldev)) {
                udmamode =
                wdmamode = -1;
            }
        }
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                promise_timing(deviceExtension, dev, (UCHAR)(ATA_UDMA + i));       // ???
                return;
            }
        }

        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                promise_timing(deviceExtension, dev, (UCHAR)(ATA_WDMA0+i));
                return;
            }
        }
        /* try generic DMA, use hpt_timing() */
        if (wdmamode >= 0 && apiomode >= 4) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_DMA)) {
                return;
            }
        }
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        promise_timing(deviceExtension, dev, ATA_PIO0 + apiomode);
        return;
        break;
    case ATA_ATI_ID:

        KdPrint2((PRINT_PREFIX "ATI\n"));
        if(ChipType == SIIMIO) {
            goto l_ATA_SILICON_IMAGE_ID;
        }
        //goto ATA_SERVERWORKS_ID;
        // FALL THROUGH

        //break; }

    case ATA_SERVERWORKS_ID: {
        /***************/
        /* ServerWorks */
        /***************/
//        static const ULONG udma_modes[] = { 0x70, 0x21, 0x20 };
        static const ULONG sw_dma_modes[] = { 0x70, 0x21, 0x20 };
        static const ULONG sw_pio_modes[] = { 0x5d, 0x47, 0x34, 0x22, 0x20, 0x34, 0x22, 0x20,
                                              0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };
        USHORT reg56;
        ULONG  reg44;
        ULONG  reg40;
        ULONG  offset = dev ^ 0x01;
        ULONG  bit_offset = offset * 8;

        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                GetPciConfig2(0x56, reg56);
                reg56 &= ~(0xf << (dev * 4));
                reg56 |= ((USHORT)i << (dev * 4));
                SetPciConfig2(0x56, reg56);
                ChangePciConfig1(0x54, a | (0x01 << dev));
                // 44
                GetPciConfig4(0x44, reg44);
                reg44 = (reg44 & ~(0xff << bit_offset)) |
                             (sw_dma_modes[2] << bit_offset);
                SetPciConfig4(0x44, reg44);
                // 40
                GetPciConfig4(0x40, reg40);
                reg40 = (reg40 & ~(0xff << bit_offset)) |
                             (sw_pio_modes[8+i] << bit_offset);
                SetPciConfig4(0x40, reg40);
                return;
            }
        }

        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {

                ChangePciConfig1(0x54, a & ~(0x01 << dev));
                // 44
                GetPciConfig4(0x44, reg44);
                reg44 = (reg44 & ~(0xff << bit_offset)) |
                             (sw_dma_modes[wdmamode] << bit_offset);
                SetPciConfig4(0x44, reg44);
                // 40
                GetPciConfig4(0x40, reg40);
                reg40 = (reg40 & ~(0xff << bit_offset)) |
                             (sw_pio_modes[5+i] << bit_offset);
                SetPciConfig4(0x40, reg40);
                return;
            }
        }
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
//        SetPciConfig4(0x44, sw_pio_modes[apiomode]);
        if(VendorID == ATA_ATI_ID) {
            // special case for ATI
            // Seems, that PATA ATI are just re-brended ServerWorks
            USHORT reg4a;
            // 4a
            GetPciConfig2(0x4a, reg4a);
            reg4a = (reg4a & ~(0xf << (dev*4))) |
                           (apiomode << (dev*4));
            SetPciConfig2(0x4a, reg4a);
        }

        // 40
        GetPciConfig4(0x40, reg40);
        reg40 = (reg40 & ~(0xff << bit_offset)) |
                       (sw_pio_modes[apiomode] << bit_offset);
        SetPciConfig4(0x40, reg40);
        return;
        break; }
    case ATA_SILICON_IMAGE_ID: {
l_ATA_SILICON_IMAGE_ID:
        /********************/
        /* SiliconImage/CMD */
        /********************/
        if(ChipType == SIIMIO) {

            static const UCHAR sil_modes[7] =
                { 0xf, 0xb, 0x7, 0x5, 0x3, 0x2, 0x1 };
            static const USHORT sil_wdma_modes[3] =
                { 0x2208, 0x10c2, 0x10c1 };
            static const USHORT sil_pio_modes[6] =
                { 0x328a, 0x2283, 0x1104, 0x10c3, 0x10c1, 0x10c1 };

            UCHAR ureg = 0xac + ((UCHAR)DeviceNumber * 0x02) + ((UCHAR)Channel * 0x10);
            UCHAR uval;
            UCHAR mreg = Channel ? 0x84 : 0x80;
            UCHAR mask = DeviceNumber ? 0x30 : 0x03;
            UCHAR mode;

            GetPciConfig1(ureg, uval);
            GetPciConfig1(mreg, mode);

            /* enable UDMA mode */
            for(i = udmamode; i>=0; i--) {

                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                    SetPciConfig1(mreg, mode | mask);
                    SetPciConfig1(ureg, (uval & 0x3f) | sil_modes[i]);
                    return;
                }
            }
            /* enable WDMA mode */
            for(i = wdmamode; i>=0; i--) {

                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                    SetPciConfig1(mreg, mode | (mask & 0x22));
                    SetPciConfig2(ureg - 0x4, sil_wdma_modes[i]);
                    return;
                }
            }
            /* restore PIO mode */
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);

            SetPciConfig1(mreg, mode | (mask & 0x11));
            SetPciConfig2(ureg - 0x8, sil_pio_modes[i]);
            return;

        } else {

            static const UCHAR cmd_modes[2][6] = {
                { 0x31, 0x21, 0x011, 0x25, 0x15, 0x05 },
                { 0xc2, 0x82, 0x042, 0x8a, 0x4a, 0x0a } };
            static const UCHAR cmd_wdma_modes[] = { 0x87, 0x32, 0x3f };
            static const UCHAR cmd_pio_modes[] = { 0xa9, 0x57, 0x44, 0x32, 0x3f };
            ULONG treg = 0x54 + (dev < 3) ? (dev << 1) : 7;

            udmamode = min(udmamode, 5);
            /* enable UDMA mode */
            for(i = udmamode; i>=0; i--) {
                UCHAR umode;

                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                    GetPciConfig1(Channel ? 0x7b : 0x73, umode);
                    umode &= ~(!(DeviceNumber & 1) ? 0x35 : 0xca);
                    umode |= ( cmd_modes[DeviceNumber & 1][i]);
                    SetPciConfig1(Channel ? 0x7b : 0x73, umode);
                    return;
                }
            }
            /* make sure eventual UDMA mode from the BIOS is disabled */
            ChangePciConfig1(Channel ? 0x7b : 0x73, a & ~(!(DeviceNumber & 1) ? 0x35 : 0xca));

            for(i = wdmamode; i>=0; i--) {

                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                    SetPciConfig1(treg, cmd_wdma_modes[i]);
                    ChangePciConfig1(Channel ? 0x7b : 0x73, a & ~(!(DeviceNumber & 1) ? 0x35 : 0xca));
                    return;
                }
            }
            /* set PIO mode timings */
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);

            SetPciConfig1(treg, cmd_pio_modes[apiomode]);
            ChangePciConfig1(Channel ? 0x7b : 0x73, a & ~(!(DeviceNumber & 1) ? 0x35 : 0xca));
            return;

        }
        return;
        break; }
    case ATA_SIS_ID: {
        /*******/
        /* SiS */
        /*******/
        PULONG sis_modes;
        static const ULONG sis_modes_new133[] =
                { 0x28269008, 0x0c266008, 0x04263008, 0x0c0a3008, 0x05093008,
                  0x22196008, 0x0c0a3008, 0x05093008, 0x050939fc, 0x050936ac,
                  0x0509347c, 0x0509325c, 0x0509323c, 0x0509322c, 0x0509321c};
        static const ULONG sis_modes_old133[] =
                { 0x00cb, 0x0067, 0x0044, 0x0033, 0x0031, 0x0044, 0x0033, 0x0031,
                  0x8f31, 0x8a31, 0x8731, 0x8531, 0x8331, 0x8231, 0x8131 };
        static const ULONG sis_modes_old[] =
                { 0x0c0b, 0x0607, 0x0404, 0x0303, 0x0301, 0x0404, 0x0303, 0x0301,
                  0xf301, 0xd301, 0xb301, 0xa301, 0x9301, 0x8301 };
        static const ULONG sis_modes_new100[] =
                { 0x00cb, 0x0067, 0x0044, 0x0033, 0x0031, 0x0044, 0x0033, 0x0031,
                  0x8b31, 0x8731, 0x8531, 0x8431, 0x8231, 0x8131 };

        ULONG reg;
        UCHAR reg57;
        ULONG reg_size;
        ULONG offs;

        switch(ChipType) {
        case SIS133NEW:
            sis_modes = (PULONG)(&sis_modes_new133[0]);
            reg_size = 4;
            GetPciConfig1(0x57, reg57);
            reg = (reg57 & 0x40 ? 0x70 : 0x40) + (dev * 4);
            break;
        case SIS133OLD:
            sis_modes = (PULONG)(&sis_modes_old133[0]);
            reg_size = 2;
            reg = 0x40 + (dev * 2);
            break;
        case SIS100NEW:
            sis_modes = (PULONG)(&sis_modes_new100[0]);
            reg_size = 2;
            reg = 0x40 + (dev * 2);
            break;
        case SIS100OLD:
        case SIS66:
        case SIS33:
            sis_modes = (PULONG)(&sis_modes_old[0]);
            reg_size = 2;
            reg = 0x40 + (dev * 2);
            break;
        }

        offs = 5+3;
        for(i=udmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                if(reg_size == 4) {
                    SetPciConfig4(reg, sis_modes[offs+i]);
                } else {
                    SetPciConfig2(reg, (USHORT)sis_modes[offs+i]);
                }
                return;
            }
        }

        offs = 5;
        for(i=wdmamode; i>=0; i--) {
            if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                if(reg_size == 4) {
                    SetPciConfig4(reg, sis_modes[offs+i]);
                } else {
                    SetPciConfig2(reg, (USHORT)sis_modes[offs+i]);
                }
                return;
            }
        }
        AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        if(reg_size == 4) {
            SetPciConfig4(reg, sis_modes[apiomode]);
        } else {
            SetPciConfig2(reg, (USHORT)sis_modes[apiomode]);
        }
        return;
        break; }
    case 0x16ca:
        /* Cenatek Rocket Drive controller */
        if (wdmamode >= 0 &&
            (AtapiReadPort1(chan, IDX_BM_Status) &
             (DeviceNumber ? BM_STATUS_DRIVE_1_DMA : BM_STATUS_DRIVE_0_DMA))) {
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + wdmamode);
        } else {
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        }
        return;
    case ATA_ITE_ID: {       /* ITE IDE controller */

        if(ChipType == ITE_33) {
            int a_speed = 3 << (dev * 4);
            int u_flag  = 1 << dev;
            int u_speed = 0;
            int pio     = 1;
            UCHAR  reg48, reg4a;
            USHORT drive_enables;
            ULONG  drive_timing;

            GetPciConfig1(0x48, reg48);
            GetPciConfig1(0x4a, reg4a);

            /*
             * Setting the DMA cycle time to 2 or 3 PCI clocks (60 and 91 nsec
             * at 33 MHz PCI clock) seems to cause BadCRC errors during DMA
             * transfers on some drives, even though both numbers meet the minimum
             * ATAPI-4 spec of 73 and 54 nsec for UDMA 1 and 2 respectively.
             * So the faster times are just commented out here. The good news is
             * that the slower cycle time has very little affect on transfer
             * performance.
             */

            for(i=udmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                    SetPciConfig1(0x48, reg48 | u_flag);
                    reg4a &= ~a_speed;
                    SetPciConfig1(0x4a, reg4a | u_speed);
                    pio = 4;
                    goto setup_drive_ite;
                }
            }

            for(i=wdmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {
                    SetPciConfig1(0x48, reg48 & ~u_flag);
                    SetPciConfig1(0x4a, reg4a & ~a_speed);
                    pio = 3;
                    return;
                }
            }
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
            SetPciConfig1(0x48, reg48 & ~u_flag);
            SetPciConfig1(0x4a, reg4a & ~a_speed);

            pio = apiomode;

setup_drive_ite:

            GetPciConfig2(0x40, drive_enables);
            GetPciConfig4(0x44, drive_timing);

            /*
             * FIX! The DIOR/DIOW pulse width and recovery times in port 0x44
             * are being left at the default values of 8 PCI clocks (242 nsec
             * for a 33 MHz clock). These can be safely shortened at higher
             * PIO modes. The DIOR/DIOW pulse width and recovery times only
             * apply to PIO modes, not to the DMA modes.
             */

            /*
             * Enable port 0x44. The IT8172G spec is confused; it calls
             * this register the "Slave IDE Timing Register", but in fact,
             * it controls timing for both master and slave drives.
             */
            drive_enables |= 0x4000;

            drive_enables &= (0xc000 | (0x06 << (DeviceNumber*4)));
            if (pio > 1) {
            /* enable prefetch and IORDY sample-point */
                drive_enables |= (0x06 << (DeviceNumber*4));
            }

            SetPciConfig2(0x40, drive_enables);
        } else
        if(ChipType == ITE_133) {
            static const UCHAR udmatiming[] =
                { 0x44, 0x42, 0x31, 0x21, 0x11, 0xa2, 0x91 };
            static const UCHAR chtiming[] =
                { 0xaa, 0xa3, 0xa1, 0x33, 0x31, 0x88, 0x32, 0x31 };
            ULONG offset = (Channel<<2) + DeviceNumber;
            UCHAR reg54;

            for(i=udmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_UDMA0 + i)) {
                    ChangePciConfig1(0x50, a & ~(1 << (dev + 3)) );
                    SetPciConfig1(0x56 + offset, udmatiming[i]);
                    return;
                }
            }

            for(i=wdmamode; i>=0; i--) {
                if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_WDMA0 + i)) {

                    ChangePciConfig1(0x50, a | (1 << (dev + 3)) );
                    GetPciConfig1(0x54 + offset, reg54);
                    if(reg54 < chtiming[i+5]) {
                        SetPciConfig1(0x54 + offset, chtiming[i+5]);
                    }
                    return;
                }
            }
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
            ChangePciConfig1(0x50, a | (1 << (dev + 3)) );
            GetPciConfig1(0x54 + offset, reg54);
            if(reg54 < chtiming[i]) {
                SetPciConfig1(0x54 + offset, chtiming[i]);
            }
            return;
        }

        return;
        break; }
    case 0x3388:
        /* HiNT Corp. VXPro II EIDE */
        if (wdmamode >= 0 &&
            (AtapiReadPort1(chan, IDX_BM_Status) &
             (DeviceNumber ? BM_STATUS_DRIVE_1_DMA : BM_STATUS_DRIVE_0_DMA))) {
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_DMA);
        } else {
            AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode);
        }
        return;
    }

try_generic_dma:

    /* unknown controller chip */

    /* better not try generic DMA on ATAPI devices it almost never works */
    if (ATAPI_DEVICE(deviceExtension, ldev)) {
        KdPrint2((PRINT_PREFIX "ATAPI on unknown controller -> PIO\n"));
        udmamode =
        wdmamode = -1;
    }

    /* if controller says its setup for DMA take the easy way out */
    /* the downside is we dont know what DMA mode we are in */
    if ((udmamode >= 0 || /*wdmamode > 1*/ wdmamode >= 0) &&
         /*deviceExtension->BaseIoAddressBM[lChannel]*/ deviceExtension->BusMaster &&
        (GetDmaStatus(deviceExtension, lChannel) &
         (!(ldev & 1) ?
          BM_STATUS_DRIVE_0_DMA : BM_STATUS_DRIVE_1_DMA))) {
//        LunExt->TransferMode = ATA_DMA;
//        return;
        KdPrint2((PRINT_PREFIX "try DMA on unknown controller\n"));
        if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_DMA)) {
            return;
        }
    }

#if 0
    /* well, we have no support for this, but try anyways */
    if ((wdmamode >= 0 && apiomode >= 4) && deviceExtension->BaseIoAddressBM[lChannel]) {
        if(AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_DMA/* + wdmamode*/)) {
            return;
        }
    }
#endif

    KdPrint2((PRINT_PREFIX "try PIO%d on unknown controller\n", apiomode));
    if(!AtaSetTransferMode(deviceExtension, DeviceNumber, lChannel, LunExt, ATA_PIO0 + apiomode)) {
        KdPrint2((PRINT_PREFIX "fall to PIO on unknown controller\n"));
        LunExt->TransferMode = ATA_PIO;
    }
    return;
} // end AtapiDmaInit()


VOID
cyrix_timing(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    )
{
//    ASSERT(dev/2 >= deviceExtension->Channel);
//    PHW_CHANNEL chan = &(deviceExtension->chan[dev/2-deviceExtension->Channel]);
    ULONG reg20 = 0x0000e132;
    ULONG reg24 = 0x00017771;

    if(mode == ATA_PIO5)
        mode = ATA_PIO4;

    switch (mode) {
    case ATA_PIO0:        reg20 = 0x0000e132; break;
    case ATA_PIO1:        reg20 = 0x00018121; break;
    case ATA_PIO2:        reg20 = 0x00024020; break;
    case ATA_PIO3:        reg20 = 0x00032010; break;
    case ATA_PIO4:
    case ATA_PIO5:        reg20 = 0x00040010; break;
    case ATA_WDMA2:       reg24 = 0x00002020; break;
    case ATA_UDMA2:       reg24 = 0x00911030; break;
    }
    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),(dev*8) + 0x20, reg20);
    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),(dev*8) + 0x24, reg24);
} // cyrix_timing()

VOID
promise_timing(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    //ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;

    ULONG timing = 0;

    if(mode == ATA_PIO5)
        mode = ATA_PIO4;

    switch(ChipType) {
    case PROLD:
        switch (mode) {
        default:
        case ATA_PIO0:  timing = 0x004ff329; break;
        case ATA_PIO1:  timing = 0x004fec25; break;
        case ATA_PIO2:  timing = 0x004fe823; break;
        case ATA_PIO3:  timing = 0x004fe622; break;
        case ATA_PIO4:  timing = 0x004fe421; break;
        case ATA_WDMA0: timing = 0x004567f3; break;
        case ATA_WDMA1: timing = 0x004467f3; break;
        case ATA_WDMA2: timing = 0x004367f3; break;
        case ATA_UDMA0: timing = 0x004367f3; break;
        case ATA_UDMA1: timing = 0x004247f3; break;
        case ATA_UDMA2: timing = 0x004127f3; break;
        }
        break;

    case PRNEW:
        switch (mode) {
        default:
        case ATA_PIO0:  timing = 0x004fff2f; break;
        case ATA_PIO1:  timing = 0x004ff82a; break;
        case ATA_PIO2:  timing = 0x004ff026; break;
        case ATA_PIO3:  timing = 0x004fec24; break;
        case ATA_PIO4:  timing = 0x004fe822; break;
        case ATA_WDMA0: timing = 0x004acef6; break;
        case ATA_WDMA1: timing = 0x0048cef6; break;
        case ATA_WDMA2: timing = 0x0046cef6; break;
        case ATA_UDMA0: timing = 0x0046cef6; break;
        case ATA_UDMA1: timing = 0x00448ef6; break;
        case ATA_UDMA2: timing = 0x00436ef6; break;
        case ATA_UDMA3: timing = 0x00424ef6; break;
        case ATA_UDMA4: timing = 0x004127f3; break;
        case ATA_UDMA5: timing = 0x004127f3; break;
        }
        break;
    default:
        return;
    }
    SetPciConfig4(0x60 + (dev<<2), timing);
} // end promise_timing()


VOID
hpt_timing(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    //ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;

    ULONG timing;

    if(mode == ATA_PIO5)
        mode = ATA_PIO4;

    switch(ChipType) {
    case HPT374:

        switch (mode) {                                                /* HPT374 */
        case ATA_PIO0:  timing = 0x0ac1f48a; break;
        case ATA_PIO1:  timing = 0x0ac1f465; break;
        case ATA_PIO2:  timing = 0x0a81f454; break;
        case ATA_PIO3:  timing = 0x0a81f443; break;
        case ATA_PIO4:  timing = 0x0a81f442; break;
        case ATA_WDMA0: timing = 0x228082ea; break;
        case ATA_WDMA1: timing = 0x22808254; break;
        case ATA_WDMA2: timing = 0x22808242; break;
        case ATA_UDMA0: timing = 0x121882ea; break;
        case ATA_UDMA1: timing = 0x12148254; break;
        case ATA_UDMA2: timing = 0x120c8242; break;
        case ATA_UDMA3: timing = 0x128c8242; break;
        case ATA_UDMA4: timing = 0x12ac8242; break;
        case ATA_UDMA5: timing = 0x12848242; break;
        case ATA_UDMA6: timing = 0x12808242; break;
        default:        timing = 0x0d029d5e;
        }
        break;

    case HPT372:

        switch (mode) {                                                /* HPT372 */
        case ATA_PIO0:  timing = 0x0d029d5e; break;
        case ATA_PIO1:  timing = 0x0d029d26; break;
        case ATA_PIO2:  timing = 0x0c829ca6; break;
        case ATA_PIO3:  timing = 0x0c829c84; break;
        case ATA_PIO4:  timing = 0x0c829c62; break;
        case ATA_WDMA0: timing = 0x2c82922e; break;
        case ATA_WDMA1: timing = 0x2c829266; break;
        case ATA_WDMA2: timing = 0x2c829262; break;
        case ATA_UDMA0: timing = 0x1c829c62; break;
        case ATA_UDMA1: timing = 0x1c9a9c62; break;
        case ATA_UDMA2: timing = 0x1c929c62; break;
        case ATA_UDMA3: timing = 0x1c8e9c62; break;
        case ATA_UDMA4: timing = 0x1c8a9c62; break;
        case ATA_UDMA5: timing = 0x1c8a9c62; break;
        case ATA_UDMA6: timing = 0x1c869c62; break;
        default:        timing = 0x0d029d5e;
        }
        break;

    case HPT370:

        switch (mode) {                                                /* HPT370 */
        case ATA_PIO0:  timing = 0x06914e57; break;
        case ATA_PIO1:  timing = 0x06914e43; break;
        case ATA_PIO2:  timing = 0x06514e33; break;
        case ATA_PIO3:  timing = 0x06514e22; break;
        case ATA_PIO4:  timing = 0x06514e21; break;
        case ATA_WDMA0: timing = 0x26514e97; break;
        case ATA_WDMA1: timing = 0x26514e33; break;
        case ATA_WDMA2: timing = 0x26514e21; break;
        case ATA_UDMA0: timing = 0x16514e31; break;
        case ATA_UDMA1: timing = 0x164d4e31; break;
        case ATA_UDMA2: timing = 0x16494e31; break;
        case ATA_UDMA3: timing = 0x166d4e31; break;
        case ATA_UDMA4: timing = 0x16454e31; break;
        case ATA_UDMA5: timing = 0x16454e31; break;
        default:        timing = 0x06514e57;
        }

    case HPT366: {
        UCHAR reg41;

        GetPciConfig1(0x41 + (dev << 2), reg41);

        switch (reg41) {
        case 0x85:        /* 25Mhz */
            switch (mode) {
            case ATA_PIO0:        timing = 0x40d08585; break;
            case ATA_PIO1:        timing = 0x40d08572; break;
            case ATA_PIO2:        timing = 0x40ca8542; break;
            case ATA_PIO3:        timing = 0x40ca8532; break;
            case ATA_PIO4:        timing = 0x40ca8521; break;
            case ATA_WDMA2:       timing = 0x20ca8521; break;
            case ATA_UDMA2:       timing = 0x10cf8521; break;
            case ATA_UDMA4:       timing = 0x10c98521; break;
            default:              timing = 0x01208585;
            }
            break;
        default:
        case 0xa7:        /* 33MHz */
            switch (mode) {
            case ATA_PIO0:        timing = 0x40d0a7aa; break;
            case ATA_PIO1:        timing = 0x40d0a7a3; break;
            case ATA_PIO2:        timing = 0x40d0a753; break;
            case ATA_PIO3:        timing = 0x40c8a742; break;
            case ATA_PIO4:        timing = 0x40c8a731; break;
            case ATA_WDMA0:       timing = 0x20c8a797; break;
            case ATA_WDMA1:       timing = 0x20c8a732; break;
            case ATA_WDMA2:       timing = 0x20c8a731; break;
            case ATA_UDMA0:       timing = 0x10c8a731; break;
            case ATA_UDMA1:       timing = 0x10cba731; break;
            case ATA_UDMA2:       timing = 0x10caa731; break;
            case ATA_UDMA3:       timing = 0x10cfa731; break;
            case ATA_UDMA4:       timing = 0x10c9a731; break;
            default:              timing = 0x0120a7a7;
            }
            break;
        case 0xd9:        /* 40Mhz */
            switch (mode) {
            case ATA_PIO0:        timing = 0x4018d9d9; break;
            case ATA_PIO1:        timing = 0x4010d9c7; break;
            case ATA_PIO2:        timing = 0x4010d997; break;
            case ATA_PIO3:        timing = 0x4010d974; break;
            case ATA_PIO4:        timing = 0x4008d963; break;
            case ATA_WDMA2:       timing = 0x2008d943; break;
            case ATA_UDMA2:       timing = 0x100bd943; break;
            case ATA_UDMA4:       timing = 0x100fd943; break;
            default:              timing = 0x0120d9d9;
            }
        }
        break; }
    }

    SetPciConfig4(0x40 + (dev<<2), timing);
} // end hpt_timing()


#define FIT(v,min,max) (((v)>(max)?(max):(v))<(min)?(min):(v))

VOID
via82c_timing(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG dev,               // physical device number (0-3)
    IN CHAR  mode
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    USHORT T = 1000 / /* PciBusClockMHz()*/ 33;

    USHORT setup   = 0;
    USHORT active  = 0;
    USHORT recover = 0;
    USHORT cycle   = 0;

    UCHAR t;

    switch(mode) {
    default:
    case ATA_PIO0:  setup = 70; active = 165; recover = 150; cycle = 600; break;
    case ATA_PIO1:  setup = 50; active = 125; recover = 100; cycle = 383; break;
    case ATA_PIO2:  setup = 30; active = 100; recover = 90;  cycle = 240; break;
    case ATA_PIO3:  setup = 30; active = 80;  recover = 70;  cycle = 180; break;
    case ATA_PIO4:  setup = 25; active = 70;  recover = 25;  cycle = 120; break;
    case ATA_PIO5:  setup = 20; active = 50;  recover = 30;  cycle = 100; break;
    }

    setup   = (setup  -1)/(T+1);
    active  = (active -1)/(T+1);
    recover = (recover-1)/(T+1);
    cycle   = (cycle  -1)/(T+1);

    if (active + recover < cycle) {
        active += (cycle - (active + recover)) / 2;
        recover = cycle - active;
    }

    // Newer chips dislike this:
    if(!(deviceExtension->HwFlags & VIAAST)) {
        /* PIO address setup */
        GetPciConfig1(0x4c, t);
        t = (t & ~(3 << ((3 - dev) << 1))) | (FIT(setup - 1, 0, 3) << ((3 - dev) << 1));
        SetPciConfig1(0x4c, t);
    }

    /* PIO active & recover */
    SetPciConfig1(0x4b-dev, (FIT(active - 1, 0, 0xf) << 4) | FIT(recover - 1, 0, 0xf) );
} // end via82c_timing()

