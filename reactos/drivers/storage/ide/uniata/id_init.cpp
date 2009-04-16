/*++

Copyright (c) 2004-2008 Alexandr A. Telyatnikov (Alter)

Module Name:
    id_init.cpp

Abstract:
    This is the chip-specific init module for ATA/ATAPI IDE controllers
    with Busmaster DMA and Serial ATA support

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

    Some parts of code were taken from FreeBSD 5.1-6.1 ATA driver by
        Søren Schmidt, Copyright (c) 1998-2007
    added IT8172 IDE controller support from Linux
    added VIA 8233/8235 fix from Linux
    added 80-pin cable detection from Linux for
        VIA, nVidia
    added support for non-standard layout of registers
    added SATA support

--*/

#include "stdafx.h"

BOOLEAN
NTAPI
UniataChipDetectChannels(
    IN PVOID HwDeviceExtension,
    IN PPCI_COMMON_CONFIG pciData, // optional
    IN ULONG DeviceNumber,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    //ULONG slotNumber = deviceExtension->slotNumber;
    //ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
    //ULONG DeviceID = (deviceExtension->DevID >> 16) & 0xffff;
    //ULONG RevID    =  deviceExtension->RevID;
    ULONG ChipType = deviceExtension->HwFlags & CHIPTYPE_MASK;
    ULONG ChipFlags= deviceExtension->HwFlags & CHIPFLAG_MASK;

    KdPrint2((PRINT_PREFIX "UniataChipDetectChannels:\n" ));
    switch(VendorID) {
    case ATA_ACER_LABS_ID:
        switch(deviceExtension->DevID) {
        case 0x528710b9:
        case 0x528810b9:
            deviceExtension->NumberChannels = 4;
            KdPrint2((PRINT_PREFIX "Acer 4 chan\n"));
        }
        break;
    case ATA_PROMISE_ID:

        if(ChipType != PRMIO) {
            break;
        }
        deviceExtension->NumberChannels = 4;
        KdPrint2((PRINT_PREFIX "Promise 4 chan\n"));
        break;
    case ATA_MARVELL_ID:
        KdPrint2((PRINT_PREFIX "Marvell\n"));
        switch(deviceExtension->DevID) {
        case 0x610111ab: 
            /* 88SX6101 only have 1 PATA channel */
            if(BMList[deviceExtension->DevIndex].channel) {
                KdPrint2((PRINT_PREFIX "88SX6101 has no 2nd PATA chan\n"));
                return FALSE;
            }
            deviceExtension->NumberChannels = 1;
            KdPrint2((PRINT_PREFIX "88SX6101 PATA 1 chan\n"));
            break;
        }
        break;
    case ATA_ATI_ID:
        KdPrint2((PRINT_PREFIX "ATI\n"));
        switch(deviceExtension->DevID) {
        case 0x438c1002: 
        case 0x439c1002:
            /* IXP600 & IXP700 only have 1 PATA channel */
            if(BMList[deviceExtension->DevIndex].channel) {
                KdPrint2((PRINT_PREFIX "New ATI no 2nd PATA chan\n"));
                return FALSE;
            }
            deviceExtension->NumberChannels = 1;
            KdPrint2((PRINT_PREFIX "New ATI PATA 1 chan\n"));
            break;
        }
    case ATA_SILICON_IMAGE_ID:

        if(ChipFlags & SIIBUG) {
        }
        if(ChipType != SIIMIO) {
            break;
        }
        if(!pciData) {
            break;
        }

        if(VendorID == ATA_SILICON_IMAGE_ID) {
            KdPrint2((PRINT_PREFIX "New SII\n"));
        } else {
            KdPrint2((PRINT_PREFIX "ATI SATA\n"));
        }
        if(deviceExtension->HwFlags & SII4CH) {
            deviceExtension->NumberChannels = 4;
            KdPrint2((PRINT_PREFIX "4 chan\n"));
        }
        break;
    case ATA_VIA_ID:
        if((deviceExtension->DevID == 0x32491106) &&
           ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[5].RangeStart)) {
            deviceExtension->NumberChannels = 3;
            KdPrint2((PRINT_PREFIX "VIA 3 chan\n"));
        }
        break;
    } // end switch(VendorID)
    return TRUE;

} // end UniataChipDetectChannels()

BOOLEAN
NTAPI
UniataChipDetect(
    IN PVOID HwDeviceExtension,
    IN PPCI_COMMON_CONFIG pciData, // optional
    IN ULONG DeviceNumber,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN BOOLEAN* simplexOnly
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
    ULONG DeviceID = (deviceExtension->DevID >> 16) & 0xffff;
    ULONG RevID    =  deviceExtension->RevID;
    ULONG i, c;
    BUSMASTER_CONTROLLER_INFORMATION* DevTypeInfo;
    PHW_CHANNEL chan;
    ULONG ChipType;
    ULONG ChipFlags;
    ULONG tmp32;
    UCHAR tmp8;
    ULONG BaseMemAddress;
    ULONG BaseIoAddress1;
    ULONG BaseIoAddress2;
    ULONG BaseIoAddressBM;
    BOOLEAN MemIo = FALSE;

    KdPrint2((PRINT_PREFIX "UniataChipDetect:\n" ));
    KdPrint2((PRINT_PREFIX "HwFlags: %#x\n", deviceExtension->HwFlags));

    i = Ata_is_dev_listed((PBUSMASTER_CONTROLLER_INFORMATION)&BusMasterAdapters[0], VendorID, 0xffff, 0, NUM_BUSMASTER_ADAPTERS);

    c = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"ForceSimplex", 0);
    if(c) {
        *simplexOnly = TRUE;
    }

    // defaults
    BaseIoAddressBM = pciData->u.type0.BaseAddresses[4] & ~0x07;
    deviceExtension->MaxTransferMode = BaseIoAddressBM ? ATA_DMA : ATA_PIO4;
    ConfigInfo->MaximumTransferLength = DEV_BSIZE*256;
    deviceExtension->MaximumDmaTransferLength = ConfigInfo->MaximumTransferLength;

    KdPrint2((PRINT_PREFIX "i: %#x\n", i));
    if(i != BMLIST_TERMINATOR) {
        DevTypeInfo = (PBUSMASTER_CONTROLLER_INFORMATION)&BusMasterAdapters[i];
    } else {
        KdPrint2((PRINT_PREFIX "  unknown dev, BM addr %#x", BaseIoAddressBM));
        DevTypeInfo = NULL;
        KdPrint2((PRINT_PREFIX "  MaxTransferMode %#x\n", deviceExtension->MaxTransferMode));

        if(!UniataChipDetectChannels(HwDeviceExtension, pciData, DeviceNumber, ConfigInfo)) {
            return FALSE;
        }
        if(!UniataAllocateLunExt(deviceExtension, UNIATA_ALLOCATE_NEW_LUNS)) {
            return FALSE;
        }

        return FALSE;
    }

    static BUSMASTER_CONTROLLER_INFORMATION const SiSAdapters[] = {
        PCI_DEV_HW_SPEC_BM( 0182, 1039, 0x00, ATA_SA150, "SiS 182"  , SISSATA   | UNIATA_SATA),
        PCI_DEV_HW_SPEC_BM( 0181, 1039, 0x00, ATA_SA150, "SiS 181"  , SISSATA   | UNIATA_SATA),
        PCI_DEV_HW_SPEC_BM( 0180, 1039, 0x00, ATA_SA150, "SiS 180"  , SISSATA   | UNIATA_SATA),
        PCI_DEV_HW_SPEC_BM( 0965, 1039, 0x00, ATA_UDMA6, "SiS 965"  , SIS133NEW        ),
        PCI_DEV_HW_SPEC_BM( 0964, 1039, 0x00, ATA_UDMA6, "SiS 964"  , SIS133NEW        ),
        PCI_DEV_HW_SPEC_BM( 0963, 1039, 0x00, ATA_UDMA6, "SiS 963"  , SIS133NEW        ),
        PCI_DEV_HW_SPEC_BM( 0962, 1039, 0x00, ATA_UDMA6, "SiS 962"  , SIS133NEW        ),

        PCI_DEV_HW_SPEC_BM( 0745, 1039, 0x00, ATA_UDMA5, "SiS 745"  , SIS100NEW        ),
        PCI_DEV_HW_SPEC_BM( 0735, 1039, 0x00, ATA_UDMA5, "SiS 735"  , SIS100NEW        ),
        PCI_DEV_HW_SPEC_BM( 0733, 1039, 0x00, ATA_UDMA5, "SiS 733"  , SIS100NEW        ),
        PCI_DEV_HW_SPEC_BM( 0730, 1039, 0x00, ATA_UDMA5, "SiS 730"  , SIS100OLD        ),

        PCI_DEV_HW_SPEC_BM( 0646, 1039, 0x00, ATA_UDMA6, "SiS 645DX", SIS133NEW        ),
/*        PCI_DEV_HW_SPEC_BM( 0645, 1039, 0x00, ATA_UDMA6, "SiS 645"  , SIS133NEW        ),*/
/*        PCI_DEV_HW_SPEC_BM( 0640, 1039, 0x00, ATA_UDMA4, "SiS 640"  , SIS_SOUTH        ),*/
        PCI_DEV_HW_SPEC_BM( 0635, 1039, 0x00, ATA_UDMA5, "SiS 635"  , SIS100NEW        ),
        PCI_DEV_HW_SPEC_BM( 0633, 1039, 0x00, ATA_UDMA5, "SiS 633"  , SIS100NEW        ),
        PCI_DEV_HW_SPEC_BM( 0630, 1039, 0x00, ATA_UDMA5, "SiS 630S" , SIS100OLD        ),
        PCI_DEV_HW_SPEC_BM( 0630, 1039, 0x00, ATA_UDMA4, "SiS 630"  , SIS66            ),
        PCI_DEV_HW_SPEC_BM( 0620, 1039, 0x00, ATA_UDMA4, "SiS 620"  , SIS66            ),

        PCI_DEV_HW_SPEC_BM( 0550, 1039, 0x00, ATA_UDMA5, "SiS 550"  , SIS66            ),
        PCI_DEV_HW_SPEC_BM( 0540, 1039, 0x00, ATA_UDMA4, "SiS 540"  , SIS66            ),
        PCI_DEV_HW_SPEC_BM( 0530, 1039, 0x00, ATA_UDMA4, "SiS 530"  , SIS66            ),

//        PCI_DEV_HW_SPEC_BM( 0008, 1039, 0x04, ATA_UDMA6, "SiS 962L" , SIS133OLD        ), // ???
//        PCI_DEV_HW_SPEC_BM( 0008, 1039, 0x00, ATA_UDMA6, "SiS 961"  , SIS133OLD        ),

        PCI_DEV_HW_SPEC_BM( 5517, 1039, 0x00, ATA_UDMA5, "SiS 961"  , SIS100NEW | SIS_BASE ),
        PCI_DEV_HW_SPEC_BM( 5518, 1039, 0x00, ATA_UDMA6, "SiS 962/3", SIS133NEW | SIS_BASE ),
        PCI_DEV_HW_SPEC_BM( 5513, 1039, 0xc2, ATA_UDMA2, "SiS 5513" , SIS33 | SIS_BASE ),
        PCI_DEV_HW_SPEC_BM( 5513, 1039, 0x00, ATA_WDMA2, "SiS 5513" , SIS33 | SIS_BASE ),
        PCI_DEV_HW_SPEC_BM( 0601, 1039, 0x00, ATA_UDMA2, "SiS 5513" , SIS33 | SIS_BASE ),
        PCI_DEV_HW_SPEC_BM( ffff, ffff, 0xff, BMLIST_TERMINATOR       , NULL       , BMLIST_TERMINATOR )
        };

    static BUSMASTER_CONTROLLER_INFORMATION const ViaAdapters[] = {
        PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x41, ATA_UDMA2, "VIA 82C586B", VIA33  | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x40, ATA_UDMA2, "VIA 82C586B", VIA33  | VIAPRQ ),
        PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x02, ATA_UDMA2, "VIA 82C586B", VIA33  | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 0586, 1106, 0x00, ATA_WDMA2, "VIA 82C586" , VIA33  | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 0596, 1106, 0x12, ATA_UDMA4, "VIA 82C596B", VIA66  | VIACLK ),
        PCI_DEV_HW_SPEC_BM( 0596, 1106, 0x00, ATA_UDMA2, "VIA 82C596" , VIA33  | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x40, ATA_UDMA5, "VIA 82C686B", VIA100 | VIABUG ),
        PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x10, ATA_UDMA4, "VIA 82C686A", VIA66  | VIACLK ),
        PCI_DEV_HW_SPEC_BM( 0686, 1106, 0x00, ATA_UDMA2, "VIA 82C686" , VIA33  | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 8231, 1106, 0x00, ATA_UDMA5, "VIA 8231"   , VIA100 | VIABUG ),
        PCI_DEV_HW_SPEC_BM( 3074, 1106, 0x00, ATA_UDMA5, "VIA 8233"   , VIA100 | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 3109, 1106, 0x00, ATA_UDMA5, "VIA 8233C"  , VIA100 | 0x00   ),
        PCI_DEV_HW_SPEC_BM( 3147, 1106, 0x00, ATA_UDMA6, "VIA 8233A"  , VIA133 | VIAAST ),
        PCI_DEV_HW_SPEC_BM( 3177, 1106, 0x00, ATA_UDMA6, "VIA 8235"   , VIA133 | VIAAST ),
        PCI_DEV_HW_SPEC_BM( 3227, 1106, 0x00, ATA_UDMA6, "VIA 8237"   , VIA133 | VIAAST ),
        PCI_DEV_HW_SPEC_BM( 0591, 1106, 0x00, ATA_UDMA6, "VIA 8237A"  , VIA133 | VIAAST ),
        PCI_DEV_HW_SPEC_BM( 3349, 1106, 0x00, ATA_UDMA6, "VIA 8251"   , VIA133 | VIAAST ),
        PCI_DEV_HW_SPEC_BM( ffff, ffff, 0xff, BMLIST_TERMINATOR       , NULL         , BMLIST_TERMINATOR )
        };

    static BUSMASTER_CONTROLLER_INFORMATION const ViaSouthAdapters[] = {
        PCI_DEV_HW_SPEC_BM( 3112, 1106, 0x00, -1, "VIA 8361", VIASOUTH ),
        PCI_DEV_HW_SPEC_BM( 0305, 1106, 0x00, -1, "VIA 8363", VIASOUTH ),
        PCI_DEV_HW_SPEC_BM( 0391, 1106, 0x00, -1, "VIA 8371", VIASOUTH ),
        PCI_DEV_HW_SPEC_BM( 3102, 1106, 0x00, -1, "VIA 8662", VIASOUTH ),
        PCI_DEV_HW_SPEC_BM( ffff, ffff, 0xff, BMLIST_TERMINATOR, NULL      , BMLIST_TERMINATOR )
        };

    KdPrint2((PRINT_PREFIX "VendorID/DeviceID/Rev %#x/%#x/%#x\n", VendorID, DeviceID, RevID));

    switch(VendorID) {

    case ATA_SIS_ID:
        KdPrint2((PRINT_PREFIX "ATA_SIS_ID\n"));
        DevTypeInfo = (BUSMASTER_CONTROLLER_INFORMATION*)&SiSAdapters[0];
        i = AtapiFindListedDev(DevTypeInfo, -1, HwDeviceExtension, SystemIoBusNumber, PCISLOTNUM_NOT_SPECIFIED, NULL);
        if(i != BMLIST_TERMINATOR) {
            deviceExtension->FullDevName = SiSAdapters[i].FullDevName;
        }
        goto for_ugly_chips;

    case ATA_VIA_ID: 
        KdPrint2((PRINT_PREFIX "ATA_VIA_ID\n"));
        // New chips have own DeviceId
        if(deviceExtension->DevID != ATA_VIA82C571)
            break;
        // Old chips have same DeviceId, we can distinguish between them
        // only by SouthBridge DeviceId
        DevTypeInfo = (BUSMASTER_CONTROLLER_INFORMATION*)&ViaSouthAdapters[0];
        i = AtapiFindListedDev(DevTypeInfo, -1, HwDeviceExtension, SystemIoBusNumber, slotNumber, NULL);
        if(i != BMLIST_TERMINATOR) {
            KdPrint2((PRINT_PREFIX "VIASOUTH\n"));
            deviceExtension->HwFlags |= VIASOUTH;
        }
        DevTypeInfo = (BUSMASTER_CONTROLLER_INFORMATION*)&ViaAdapters[0];
        i = AtapiFindListedDev(DevTypeInfo, -1, HwDeviceExtension, SystemIoBusNumber, slotNumber, NULL);
        if(i != BMLIST_TERMINATOR) {
            deviceExtension->FullDevName = SiSAdapters[i].FullDevName;
        }
        goto for_ugly_chips;

    default:

        // do nothing
        break;

#if 0
        KdPrint2((PRINT_PREFIX "Default\n"));

        deviceExtension->MaxTransferMode = deviceExtension->BaseIoAddressBM_0 ? ATA_DMA : ATA_PIO4;
        /* do extra chipset specific setups */
        switch(deviceExtension->DevID) {

      //case ATA_CYPRESS_ID:
        case 0xc6931080:         /* 82c693 ATA controller */
            deviceExtension->MaxTransferMode = ATA_WDMA2;
            break;

        case 0x000116ca:         /* Cenatek Rocket Drive controller */
            deviceExtension->MaxTransferMode = ATA_WDMA2;
            break;

/*      case ATA_CYRIX_ID:
            DevTypeInfo = &CyrixAdapters[0];
            break;*/
        case 0x01021078:        /* Cyrix 5530 ATA33 controller */
            deviceExtension->MaxTransferMode = ATA_UDMA2;
            break;

        case 0x06401039:        /* CMD 640 known bad, no DMA */
        case 0x06011039:
            *simplexOnly = TRUE; 

            /* FALLTHROUGH */

        case 0x10001042:        /* RZ 100x known bad, no DMA */
        case 0x10011042:

            if(deviceExtension->BaseIoAddressBM_0)
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       deviceExtension->BaseIoAddressBM_0);

            deviceExtension->BaseIoAddressBM_0 = 0;
            deviceExtension->BusMaster = FALSE;
            deviceExtension->MaxTransferMode = ATA_PIO4;
            break;

        case 0x81721283:        /* IT8172 IDE controller */
            deviceExtension->MaxTransferMode = ATA_UDMA2;
            *simplexOnly = TRUE;
            break;

        default:
            return FALSE;
        }
        return TRUE;
#endif
    }

    i = Ata_is_dev_listed(DevTypeInfo, VendorID, DeviceID, RevID, -1);
for_ugly_chips:
    KdPrint2((PRINT_PREFIX "i: %#x\n", i));
    if(i == BMLIST_TERMINATOR) {
        return FALSE;
    }
    deviceExtension->MaxTransferMode =  DevTypeInfo[i].MaxTransferMode;
    deviceExtension->HwFlags         |= DevTypeInfo[i].RaidFlags;

    KdPrint2((PRINT_PREFIX "HwFlags: %#x\n", deviceExtension->HwFlags));

    tmp32 = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"HwFlagsOverride", deviceExtension->HwFlags);
    KdPrint2((PRINT_PREFIX "HwFlagsOverride: %#x\n", tmp32));
    deviceExtension->HwFlags = tmp32;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"HwFlagsAdd", 0);
    KdPrint2((PRINT_PREFIX "HwFlagsAdd: %#x\n", tmp32));
    deviceExtension->HwFlags |= tmp32;

    KdPrint2((PRINT_PREFIX "HwFlags (final): %#x\n", deviceExtension->HwFlags));
    if(deviceExtension->HwFlags & UNIATA_SIMPLEX_ONLY) {
        KdPrint2((PRINT_PREFIX "UNIATA_SIMPLEX_ONLY\n" ));
        *simplexOnly = TRUE;
    }

    KdPrint2((PRINT_PREFIX "MaxTransferMode: %#x\n", deviceExtension->MaxTransferMode));
    tmp32 = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"MaxTransferMode", deviceExtension->MaxTransferMode);
    if(tmp32 != 0xffffffff) {
        KdPrint2((PRINT_PREFIX "MaxTransferMode (overriden): %#x\n", deviceExtension->MaxTransferMode));
        deviceExtension->MaxTransferMode = tmp32;
    }

    if(deviceExtension->MaxTransferMode >= ATA_SA150) {
        deviceExtension->HwFlags |= UNIATA_SATA;
    }
/*
    ConfigInfo->MaximumTransferLength = DEV_BSIZE*256;
    deviceExtension->MaximumDmaTransferLength = ConfigInfo->MaximumTransferLength;
*/
    ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;

    if(!UniataChipDetectChannels(HwDeviceExtension, pciData, DeviceNumber, ConfigInfo)) {
        return FALSE;
    }
    if(!UniataAllocateLunExt(deviceExtension, UNIATA_ALLOCATE_NEW_LUNS)) {
        return FALSE;
    }

    switch(VendorID) {
    case ATA_ACER_LABS_ID:
        if(ChipFlags & UNIATA_SATA) {
            deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
            BaseIoAddress1  = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                    0, 0, 0x10);
            BaseIoAddress2  = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                    1, 0, 0x10);
            BaseIoAddressBM = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                    4, 0, deviceExtension->NumberChannels*sizeof(IDE_BUSMASTER_REGISTERS));
            for(c=0; c<deviceExtension->NumberChannels; c++) {
                //ULONG unit01 = (c & 1);
                ULONG unit10 = (c & 2);
                chan = &deviceExtension->chan[c];

                for (i=0; i<=IDX_IO1_SZ; i++) {
                    chan->RegTranslation[IDX_IO1+i].Addr           = BaseIoAddress1  + i + (unit10 ? 8 : 0);
                }
                chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseIoAddress2  + 2 + (unit10 ? 4 : 0);
                UniataInitSyncBaseIO(chan);

                for (i=0; i<=IDX_BM_IO_SZ; i++) {
                    chan->RegTranslation[IDX_BM_IO+i].Addr         = BaseIoAddressBM + i + (c * sizeof(IDE_BUSMASTER_REGISTERS));
                }

                // SATA not supported yet

                //chan->RegTranslation[IDX_BM_Command]          = BaseMemAddress + 0x260 + offs7;
                //chan->RegTranslation[IDX_BM_PRD_Table]        = BaseMemAddress + 0x244 + offs7;
                //chan->RegTranslation[IDX_BM_DeviceSpecific0]  = BaseMemAddress + (c << 2);

                chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
            }
        }
        break;
    case ATA_NVIDIA_ID:
        if(ChipFlags & UNIATA_SATA) {
            KdPrint2((PRINT_PREFIX "NVIDIA SATA\n"));
            BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                    5, 0, ((ChipFlags & NV4OFF) ? 0x400 : 0) + 0x40*2);
            KdPrint2((PRINT_PREFIX "BaseMemAddress %x\n", BaseMemAddress));
            if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
                KdPrint2((PRINT_PREFIX "MemIo\n"));
                MemIo = TRUE;
            }
            deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
            deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;
            for(c=0; c<deviceExtension->NumberChannels; c++) {
                chan = &deviceExtension->chan[c];

                chan->RegTranslation[IDX_SATA_SStatus].Addr   = BaseMemAddress +     (c << 6);
                chan->RegTranslation[IDX_SATA_SStatus].MemIo  = MemIo;
                chan->RegTranslation[IDX_SATA_SError].Addr    = BaseMemAddress + 4 + (c << 6);
                chan->RegTranslation[IDX_SATA_SError].MemIo   = MemIo;
                chan->RegTranslation[IDX_SATA_SControl].Addr  = BaseMemAddress + 8 + (c << 6);
                chan->RegTranslation[IDX_SATA_SControl].MemIo = MemIo;

                chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
            }
        }
        break;
    case ATA_PROMISE_ID:

        if(ChipType != PRMIO) {
            break;
        }
        if(!pciData) {
            break;
        }
        deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
        BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                4, 0, 0x4000);
        KdPrint2((PRINT_PREFIX "BaseMemAddress %x\n", BaseMemAddress));
        if((*ConfigInfo->AccessRanges)[4].RangeInMemory) {
            KdPrint2((PRINT_PREFIX "MemIo\n"));
            MemIo = TRUE;
        }
        deviceExtension->BaseIoAddressBM_0.Addr  = BaseMemAddress;
        deviceExtension->BaseIoAddressBM_0.MemIo = MemIo;
        for(c=0; c<deviceExtension->NumberChannels; c++) {

            ULONG offs12, offs7;

            chan = &deviceExtension->chan[c];

            offs12 = c << 12;
            offs7  = c << 7;

            for (i=0; i<=IDX_IO1_SZ; i++) {
                chan->RegTranslation[IDX_IO1+i].Addr           = BaseMemAddress + 0x200 + (i << 2) + offs12;
                chan->RegTranslation[IDX_IO1+i].MemIo          = MemIo;
            }
            chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseMemAddress + 0x238 + offs7;
            chan->RegTranslation[IDX_IO2_AltStatus].MemIo      = MemIo;

            UniataInitSyncBaseIO(chan);

            chan->RegTranslation[IDX_BM_Command].Addr          = BaseMemAddress + 0x260 + offs7;
            chan->RegTranslation[IDX_BM_Command].MemIo         = MemIo;
            chan->RegTranslation[IDX_BM_PRD_Table].Addr        = BaseMemAddress + 0x244 + offs7;
            chan->RegTranslation[IDX_BM_PRD_Table].MemIo       = MemIo;
            chan->RegTranslation[IDX_BM_DeviceSpecific0].Addr  = BaseMemAddress + (c << 2);
            chan->RegTranslation[IDX_BM_DeviceSpecific0].MemIo = MemIo;
        }
        break;

    case ATA_ATI_ID:
        KdPrint2((PRINT_PREFIX "ATI\n"));
    case ATA_SILICON_IMAGE_ID: {

        if(ChipFlags & SIIBUG) {
        }
        if(ChipType != SIIMIO) {
            break;
        }
        if(!pciData) {
            break;
        }

        if(VendorID == ATA_SILICON_IMAGE_ID) {
            KdPrint2((PRINT_PREFIX "New SII\n"));
        } else {
            KdPrint2((PRINT_PREFIX "ATI SATA\n"));
        }
        //if(deviceExtension->HwFlags & SII4CH) {
            deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
        //}
        BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                5, 0, 0x800);
        KdPrint2((PRINT_PREFIX "BaseMemAddress %x\n", BaseMemAddress));
        if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
            KdPrint2((PRINT_PREFIX "MemIo\n"));
            MemIo = TRUE;
        }
        deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
        deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;

        for(c=0; c<deviceExtension->NumberChannels; c++) {
            ULONG unit01 = (c & 1);
            ULONG unit10 = (c & 2);

            chan = &deviceExtension->chan[c];

            if(deviceExtension->AltRegMap) {
                for (i=0; i<=IDX_IO1_SZ; i++) {
                    chan->RegTranslation[IDX_IO1+i].Addr           = BaseMemAddress + 0x80 + i + (unit01 << 6) + (unit10 << 8);
                    chan->RegTranslation[IDX_IO1+i].MemIo          = MemIo;
                }
                chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseMemAddress + 0x8a + (unit01 << 6) + (unit10 << 8);
                chan->RegTranslation[IDX_IO2_AltStatus].MemIo      = MemIo;
                UniataInitSyncBaseIO(chan);

                chan->RegTranslation[IDX_BM_Command].Addr          = BaseMemAddress + 0x00 + (unit01 << 3) + (unit10 << 8);
                chan->RegTranslation[IDX_BM_Command].MemIo         = MemIo;
                chan->RegTranslation[IDX_BM_Status].Addr           = BaseMemAddress + 0x02 + (unit01 << 3) + (unit10 << 8);
                chan->RegTranslation[IDX_BM_Status].MemIo          = MemIo;
                chan->RegTranslation[IDX_BM_PRD_Table].Addr        = BaseMemAddress + 0x04 + (unit01 << 3) + (unit10 << 8);
                chan->RegTranslation[IDX_BM_PRD_Table].MemIo       = MemIo;
                //chan->RegTranslation[IDX_BM_DeviceSpecific0].Addr  = BaseMemAddress + 0xa1 + (unit01 << 6) + (unit10 << 8);
                //chan->RegTranslation[IDX_BM_DeviceSpecific0].MemIo = MemIo;
                chan->RegTranslation[IDX_BM_DeviceSpecific0].Addr  = BaseMemAddress + 0x10 + (unit01 << 3) + (unit10 << 8);
                chan->RegTranslation[IDX_BM_DeviceSpecific0].MemIo = MemIo;
                chan->RegTranslation[IDX_BM_DeviceSpecific1].Addr  = BaseMemAddress + 0x40 + (unit01 << 2) + (unit10 << 8);
                chan->RegTranslation[IDX_BM_DeviceSpecific1].MemIo = MemIo;
            }

            if(ChipFlags & UNIATA_SATA) {
                chan->RegTranslation[IDX_SATA_SStatus].Addr        = BaseMemAddress + 0x104 + (unit01 << 7) + (unit10 << 8);
                chan->RegTranslation[IDX_SATA_SStatus].MemIo       = MemIo;
                chan->RegTranslation[IDX_SATA_SError].Addr         = BaseMemAddress + 0x108 + (unit01 << 7) + (unit10 << 8);
                chan->RegTranslation[IDX_SATA_SError].MemIo        = MemIo;
                chan->RegTranslation[IDX_SATA_SControl].Addr       = BaseMemAddress + 0x100 + (unit01 << 7) + (unit10 << 8);
                chan->RegTranslation[IDX_SATA_SControl].MemIo      = MemIo;

                chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
            }
        }
        break; }

    case ATA_SERVERWORKS_ID: {

        if(ChipType != SWKSMIO) {
            break;
        }
        if(!pciData) {
            break;
        }

        KdPrint2((PRINT_PREFIX "ServerWorks\n"));

        deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
        BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                5, 0, 0x400);
        KdPrint2((PRINT_PREFIX "BaseMemAddress %x\n", BaseMemAddress));
        if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
            KdPrint2((PRINT_PREFIX "MemIo\n"));
            MemIo = TRUE;
        }
        deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
        deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;

        for(c=0; c<deviceExtension->NumberChannels; c++) {
            ULONG offs = c*0x100;

            chan = &deviceExtension->chan[c];
            for (i=0; i<=IDX_IO1_SZ; i++) {
                chan->RegTranslation[IDX_IO1+i].Addr           = BaseMemAddress + offs + i*4;
                chan->RegTranslation[IDX_IO1+i].MemIo          = MemIo;
            }
            chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseMemAddress + offs + 0x20;
            chan->RegTranslation[IDX_IO2_AltStatus].MemIo      = MemIo;
            UniataInitSyncBaseIO(chan);

            chan->RegTranslation[IDX_BM_Command].Addr          = BaseMemAddress + offs + 0x30;
            chan->RegTranslation[IDX_BM_Command].MemIo         = MemIo;
            chan->RegTranslation[IDX_BM_Status].Addr           = BaseMemAddress + offs + 0x32;
            chan->RegTranslation[IDX_BM_Status].MemIo          = MemIo;
            chan->RegTranslation[IDX_BM_PRD_Table].Addr        = BaseMemAddress + offs + 0x34;
            chan->RegTranslation[IDX_BM_PRD_Table].MemIo       = MemIo;

            chan->RegTranslation[IDX_SATA_SStatus].Addr        = BaseMemAddress + offs + 0x40;
            chan->RegTranslation[IDX_SATA_SStatus].MemIo       = MemIo;               
            chan->RegTranslation[IDX_SATA_SError].Addr         = BaseMemAddress + offs + 0x44;
            chan->RegTranslation[IDX_SATA_SError].MemIo        = MemIo;               
            chan->RegTranslation[IDX_SATA_SControl].Addr       = BaseMemAddress + offs + 0x48;
            chan->RegTranslation[IDX_SATA_SControl].MemIo      = MemIo;

            chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
        }
        break; }

    case ATA_SIS_ID: {
        //if(ChipType != SIS_SOUTH) {
        BOOLEAN SIS_182=FALSE;

        if(!(ChipFlags & SIS_BASE)) {
            KdPrint2((PRINT_PREFIX "Found SIS_SOUTH\n"));
            //PrintNtConsole("Found SIS_SOUTH\n");
            break;
        }
        // Make some additional checks
        KdPrint2((PRINT_PREFIX "ChipType == SIS_BASE\n"));
        ChangePciConfig1(0x57, (a & 0x7f));
        GetPciConfig4(0x00, tmp32);
        if(tmp32 == ATA_SIS5518) {
            ChipType = SIS133NEW;
            deviceExtension->HwFlags = (deviceExtension->HwFlags & ~CHIPTYPE_MASK) | SIS133NEW;
            deviceExtension->MaxTransferMode = ATA_UDMA6;
            KdPrint2((PRINT_PREFIX "UniataChipDetect: SiS 962/963 DMA %#x controller\n", deviceExtension->MaxTransferMode));
            //PrintNtConsole("UniataChipDetect: SiS 962/963 DMA %#x controller\n", deviceExtension->MaxTransferMode);
            // Restore device ID
            ChangePciConfig1(0x57, (a | 0x80));
        } else {
            static BUSMASTER_CONTROLLER_INFORMATION const SiSSouthAdapters[] = {
                PCI_DEV_HW_SPEC_BM( 0008, 1039, 0x10, -1, "SiS 961", 0 ),
//                PCI_DEV_HW_SPEC_BM( 0008, 1039, 0x00, -1, "SiS 961", 0 ),
                PCI_DEV_HW_SPEC_BM( ffff, ffff, 0xff, -1, NULL     , -1 )
                };
            // Save settings
            GetPciConfig1(0x4a, tmp8);
            ChangePciConfig1(0x4a, (a | 0x10));
            if(tmp32 == ATA_SIS5513 ||
               tmp32 == ATA_SIS5517) {
                i = AtapiFindListedDev((BUSMASTER_CONTROLLER_INFORMATION*)&SiSSouthAdapters[0],
                     -1, HwDeviceExtension, SystemIoBusNumber, PCISLOTNUM_NOT_SPECIFIED, NULL); 
                if(i != BMLIST_TERMINATOR) {
                    deviceExtension->HwFlags = (deviceExtension->HwFlags & ~CHIPTYPE_MASK) | SIS133OLD;
                    //deviceExtension->MaxTransferMode = ATA_UDMA6;
                    deviceExtension->MaxTransferMode = SiSSouthAdapters[i].MaxTransferMode;
                    if(SiSSouthAdapters[i].RaidFlags & UNIATA_SATA) {
                        deviceExtension->HwFlags |= UNIATA_SATA;
                        if(SiSSouthAdapters[i].nDeviceId == 0x1182) {
                            SIS_182 = TRUE;
                        }
                    }
                } else {
                    // SiS-South not found
                    if(tmp32 == ATA_SIS5517) {
                        deviceExtension->HwFlags = (deviceExtension->HwFlags & ~CHIPTYPE_MASK) | SIS100NEW;
                        deviceExtension->MaxTransferMode = ATA_UDMA5;
                    } else {
                        // generic SiS33
                        KdPrint2((PRINT_PREFIX "Generic SiS DMA\n"));
                    }
                }
            }
            // Restore settings
            SetPciConfig1(0x4a, tmp8);
            KdPrint2((PRINT_PREFIX "UniataChipDetect: SiS 961 DMA %#x controller\n", deviceExtension->MaxTransferMode));
            //PrintNtConsole("UniataChipDetect: SiS 961 DMA %#x controller\n", deviceExtension->MaxTransferMode);
            if(deviceExtension->HwFlags & UNIATA_SATA) {
                KdPrint2((PRINT_PREFIX "SiS SATA\n"));

                BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                        5, 0, 0x400);
                KdPrint2((PRINT_PREFIX "BaseMemAddress %x\n", BaseMemAddress));
                if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
                    KdPrint2((PRINT_PREFIX "MemIo\n"));
                    MemIo = TRUE;
                }
                deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
                deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;

                for(c=0; c<deviceExtension->NumberChannels; c++) {
                    ULONG offs = c << (SIS_182 ? 5 : 6);

                    chan = &deviceExtension->chan[c];
                    chan->RegTranslation[IDX_SATA_SStatus].Addr        = BaseMemAddress + 0 + offs;
                    chan->RegTranslation[IDX_SATA_SStatus].MemIo       = MemIo;
                    chan->RegTranslation[IDX_SATA_SError].Addr         = BaseMemAddress + 4 + offs;
                    chan->RegTranslation[IDX_SATA_SError].MemIo        = MemIo;
                    chan->RegTranslation[IDX_SATA_SControl].Addr       = BaseMemAddress + 8 + offs;
                    chan->RegTranslation[IDX_SATA_SControl].MemIo      = MemIo;

                    chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
                }
            }
        }
        //ChangePciConfig1(0x57, (a | 0x80));
        break; }

    case ATA_VIA_ID: {

        if(ChipFlags & UNIATA_SATA) {

            ULONG IoSize = 0;
            ULONG BaseMemAddress = 0;

            switch(DeviceID) {
            case 0x3149: // VIA 6420
                KdPrint2((PRINT_PREFIX "VIA 6420\n"));
                IoSize = 0x80;
                break;
            case 0x3249: // VIA 6421
                KdPrint2((PRINT_PREFIX "VIA 6421\n"));
                IoSize = 0x40;
                break;
            }
            if(IoSize) {
                KdPrint2((PRINT_PREFIX "IoSize %x\n", IoSize));
                /*deviceExtension->*/BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                        5, 0, IoSize * deviceExtension->NumberChannels);
                if((*ConfigInfo->AccessRanges)[5].RangeInMemory) {
                    KdPrint2((PRINT_PREFIX "MemIo\n"));
                    MemIo = TRUE;
                }
                deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
                deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;
            }
            if(/*deviceExtension->*/BaseMemAddress) {
                KdPrint2((PRINT_PREFIX "UniataChipDetect: BAR5 %x\n", /*deviceExtension->*/BaseMemAddress));
                if(ChipFlags & VIABAR) {

                    ULONG BaseIoAddressBM_0;
                    ULONG BaseIo;

                    KdPrint2((PRINT_PREFIX "UniataChipDetect: VIABAR\n"));
                    /*deviceExtension->*/BaseIoAddressBM_0 = /*(PIDE_BUSMASTER_REGISTERS)*/
                        AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber, 4, 0,
                                        sizeof(IDE_BUSMASTER_REGISTERS)*deviceExtension->NumberChannels);
                    deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
                    for(c=0; c<deviceExtension->NumberChannels; c++) {

                        chan = &deviceExtension->chan[c];

                        BaseIo = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber, c, 0, 0x80);

                        for (i=0; i<=IDX_IO1_SZ; i++) {
                            chan->RegTranslation[IDX_IO1+i].Addr           = BaseIo + i;
                        }
                        chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseIo + sizeof(IDE_REGISTERS_1) + 2 + FIELD_OFFSET(IDE_REGISTERS_2, AltStatus   );
                        UniataInitSyncBaseIO(chan);

                        for (i=0; i<=IDX_BM_IO_SZ; i++) {
                            chan->RegTranslation[IDX_BM_IO+i].Addr         = BaseIoAddressBM_0 + sizeof(IDE_BUSMASTER_REGISTERS)*c + i;
                        }
                        if(c < 2) {
                            // Do not setup SATA registers for PATA part
                            chan->RegTranslation[IDX_SATA_SStatus].Addr        = BaseMemAddress + (c * IoSize);
                            chan->RegTranslation[IDX_SATA_SStatus].MemIo       = MemIo;
                            chan->RegTranslation[IDX_SATA_SError].Addr         = BaseMemAddress + 4 + (c * IoSize);
                            chan->RegTranslation[IDX_SATA_SError].MemIo        = MemIo;
                            chan->RegTranslation[IDX_SATA_SControl].Addr       = BaseMemAddress + 8 + (c * IoSize);
                            chan->RegTranslation[IDX_SATA_SControl].MemIo      = MemIo;

                            chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
                        }

                    }
                }
            }
        }
        break; }
    case ATA_INTEL_ID: {

        if(!(ChipFlags & UNIATA_SATA)) {
            break;
        }

        /* the intel 31244 needs special care if in DPA mode */
        if(DeviceID == 3200 && // Intel 31244
           pciData->SubClass != PCI_DEV_SUBCLASS_IDE) {

            KdPrint2((PRINT_PREFIX "UniataChipDetect: Intel 31244, DPA mode\n"));
            BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
                                    0, 0, 0x0c00);
            if((*ConfigInfo->AccessRanges)[0].RangeInMemory) {
                KdPrint2((PRINT_PREFIX "MemIo\n"));
                MemIo = TRUE;
            }
            deviceExtension->AltRegMap = TRUE; // inform generic resource allocator
            deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
            deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;

            for(c=0; c<deviceExtension->NumberChannels; c++) {
                ULONG offs = 0x200 + c*0x200;

                chan = &deviceExtension->chan[c];
                for (i=0; i<=IDX_IO1_SZ; i++) {
                    chan->RegTranslation[IDX_IO1+i].MemIo          = MemIo;
                    chan->RegTranslation[IDX_IO1_o+i].MemIo        = MemIo;
                }

                chan->RegTranslation[IDX_IO1_i_Data        ].Addr       = BaseMemAddress + 0x00 + offs;
                chan->RegTranslation[IDX_IO1_i_Error       ].Addr       = BaseMemAddress + 0x04 + offs;
                chan->RegTranslation[IDX_IO1_i_BlockCount  ].Addr       = BaseMemAddress + 0x08 + offs;
                chan->RegTranslation[IDX_IO1_i_BlockNumber ].Addr       = BaseMemAddress + 0x0c + offs;
                chan->RegTranslation[IDX_IO1_i_CylinderLow ].Addr       = BaseMemAddress + 0x10 + offs;
                chan->RegTranslation[IDX_IO1_i_CylinderHigh].Addr       = BaseMemAddress + 0x14 + offs;
                chan->RegTranslation[IDX_IO1_i_DriveSelect ].Addr       = BaseMemAddress + 0x18 + offs;
                chan->RegTranslation[IDX_IO1_i_Status      ].Addr       = BaseMemAddress + 0x1c + offs;

                UniataInitSyncBaseIO(chan);

                chan->RegTranslation[IDX_IO1_o_Command     ].Addr       = BaseMemAddress + 0x1d + offs;
                chan->RegTranslation[IDX_IO1_o_Feature     ].Addr       = BaseMemAddress + 0x06 + offs;
                chan->RegTranslation[IDX_IO2_o_Control     ].Addr       = BaseMemAddress + 0x29 + offs;

                chan->RegTranslation[IDX_IO2_AltStatus].Addr       = BaseMemAddress + 0x28 + offs;
                chan->RegTranslation[IDX_IO2_AltStatus].MemIo      = MemIo;

                chan->RegTranslation[IDX_BM_Command].Addr          = BaseMemAddress + offs + 0x70;
                chan->RegTranslation[IDX_BM_Command].MemIo         = MemIo;
                chan->RegTranslation[IDX_BM_Status].Addr           = BaseMemAddress + offs + 0x72;
                chan->RegTranslation[IDX_BM_Status].MemIo          = MemIo;
                chan->RegTranslation[IDX_BM_PRD_Table].Addr        = BaseMemAddress + offs + 0x74;
                chan->RegTranslation[IDX_BM_PRD_Table].MemIo       = MemIo;

                chan->RegTranslation[IDX_SATA_SStatus].Addr        = BaseMemAddress + 0x100 + offs;
                chan->RegTranslation[IDX_SATA_SStatus].MemIo       = MemIo;
                chan->RegTranslation[IDX_SATA_SError].Addr         = BaseMemAddress + 0x104 + offs;
                chan->RegTranslation[IDX_SATA_SError].MemIo        = MemIo;
                chan->RegTranslation[IDX_SATA_SControl].Addr       = BaseMemAddress + 0x108 + offs;
                chan->RegTranslation[IDX_SATA_SControl].MemIo      = MemIo;

                chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
            }

            break;
        }
        /* SATA parts can be either compat or AHCI */
        if(ChipFlags & UNIATA_AHCI) {

            GetPciConfig1(0x90, tmp8);
            if(tmp8 & 0xc0) {
                KdPrint2((PRINT_PREFIX "AHCI not supported yet\n"));
                return FALSE;
            }
            deviceExtension->HwFlags &= ~UNIATA_AHCI;
        }
#if 0
        if(ChipFlags & UNIATA_SATA) {
            //BaseMemAddress = AtapiGetIoRange(HwDeviceExtension, ConfigInfo, pciData, SystemIoBusNumber,
            //                        0, 0, 0x0c00);
            BaseMemAddress = NULL; // HACK-HACK
            if(!BaseMemAddress) {
                KdPrint2((PRINT_PREFIX "Intel: no SATA I/O space, operate in PATA Compatible mode\n"));
                deviceExtension->HwFlags &= ~UNIATA_SATA;
                break;
            }
            if((*ConfigInfo->AccessRanges)[0].RangeInMemory) {
                KdPrint2((PRINT_PREFIX "MemIo\n"));
                MemIo = TRUE;
            }
            deviceExtension->BaseIoAddressSATA_0.Addr  = BaseMemAddress;
            deviceExtension->BaseIoAddressSATA_0.MemIo = MemIo;
        }
#endif
        break; }
    case 0x1078:
        /* Cyrix 5530 ATA33 controller */
        if(deviceExtension->DevID == 0x01021078) {
            ConfigInfo->AlignmentMask = 0x0f;
            deviceExtension->MaximumDmaTransferLength = 63*1024;
        }
        break;
    }

    return TRUE;

} // end UniataChipDetect()


/*
    Do some 'magic staff' for VIA SouthBridge
    This will prevent data losses
*/
VOID
NTAPI
AtapiViaSouthBridgeFixup(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  SystemIoBusNumber,
    IN ULONG  slotNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 funcNumber;
    ULONG                 busDataRead;

    ULONG   VendorID;
    ULONG   DeviceID;
    PCI_SLOT_NUMBER       slotData;
    ULONG dev_id;
    BOOLEAN found = FALSE;

    slotData.u.AsULONG = slotNumber;
    for(funcNumber = 0; funcNumber < PCI_MAX_FUNCTION; funcNumber++) {

        slotData.u.bits.FunctionNumber = funcNumber;

        busDataRead = ScsiPortGetBusData(HwDeviceExtension,
                                         PCIConfiguration,
                                         SystemIoBusNumber,
                                         slotData.u.AsULONG,
                                         &pciData,
                                         PCI_COMMON_HDR_LENGTH);

        if (busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
            continue;
        }

        VendorID  = pciData.VendorID;
        DeviceID  = pciData.DeviceID;
        dev_id = (VendorID | (DeviceID << 16));

        if (dev_id == 0x03051106 ||         /* VIA VT8363 */
            dev_id == 0x03911106 ||         /* VIA VT8371 */
            dev_id == 0x31021106 ||         /* VIA VT8662 */
            dev_id == 0x31121106) {         /* VIA VT8361 */
            UCHAR reg76;

            GetPciConfig1(0x76, reg76);

            if ((reg76 & 0xf0) != 0xd0) {
                SetPciConfig1(0x75, 0x80);
                SetPciConfig1(0x76, (reg76 & 0x0f) | 0xd0);
            }
            found = TRUE;
            break;
        }
    }
    if(!found) {
        deviceExtension->HwFlags &= ~VIABUG;
    }
} // end AtapiViaSouthBridgeFixup()

/*
    Do some 'magic staff' for ROSB SouthBridge
    This will prevent data losses
*/
VOID
NTAPI
AtapiRosbSouthBridgeFixup(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  SystemIoBusNumber,
    IN ULONG  slotNumber
    )
{
    //PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 funcNumber;
    ULONG                 busDataRead;

    ULONG   VendorID;
    ULONG   DeviceID;
    PCI_SLOT_NUMBER       slotData;
    ULONG dev_id;
//    BOOLEAN found = FALSE;

    /* locate the ISA part in the southbridge and enable UDMA33 */
    slotData.u.AsULONG = slotNumber;
    for(funcNumber = 0; funcNumber < PCI_MAX_FUNCTION; funcNumber++) {

        slotData.u.bits.FunctionNumber = funcNumber;

        busDataRead = ScsiPortGetBusData(HwDeviceExtension,
                                         PCIConfiguration,
                                         SystemIoBusNumber,
                                         slotData.u.AsULONG,
                                         &pciData,
                                         PCI_COMMON_HDR_LENGTH);

        if (busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
            continue;
        }

        VendorID  = pciData.VendorID;
        DeviceID  = pciData.DeviceID;
        dev_id = (VendorID | (DeviceID << 16));

        if (dev_id == ATA_ROSB4_ISA) {         /* VIA VT8361 */
            ChangePciConfig4(0x64, ((a & ~0x00002000) | 0x00004000));
            break;
        }
    }
} // end AtapiRosbSouthBridgeFixup()

/*
    Do some 'magic staff' for ROSB SouthBridge
    This will prevent data losses
*/
VOID
NTAPI
AtapiAliSouthBridgeFixup(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  SystemIoBusNumber,
    IN ULONG  slotNumber,
    IN ULONG  c
    )
{
    //PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 funcNumber;
    ULONG                 busDataRead;

    ULONG   VendorID;
    ULONG   DeviceID;
    PCI_SLOT_NUMBER       slotData;
    ULONG dev_id;
//    BOOLEAN found = FALSE;

    /* workaround for datacorruption bug found on at least SUN Blade-100
     * find the ISA function on the southbridge and disable then enable
     * the ATA channel tristate buffer */
    slotData.u.AsULONG = slotNumber;
    for(funcNumber = 0; funcNumber < PCI_MAX_FUNCTION; funcNumber++) {

        slotData.u.bits.FunctionNumber = funcNumber;

        busDataRead = ScsiPortGetBusData(HwDeviceExtension,
                                         PCIConfiguration,
                                         SystemIoBusNumber,
                                         slotData.u.AsULONG,
                                         &pciData,
                                         PCI_COMMON_HDR_LENGTH);

        if (busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
            continue;
        }

        VendorID  = pciData.VendorID;
        DeviceID  = pciData.DeviceID;
        dev_id = (VendorID | (DeviceID << 16));

        if (dev_id == ATA_ALI_1533) {         /* SOUTH */
            ChangePciConfig1(0x58, (a & ~(0x04 << c)));
            ChangePciConfig1(0x58, (a |  (0x04 << c)));
            break;
        }
    }
} // end AtapiRosbSouthBridgeFixup()

ULONG
NTAPI
hpt_cable80(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG channel               // physical channel number (0-1)
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;

    UCHAR reg, val, res;
    PCI_SLOT_NUMBER slotData;

    slotData.u.AsULONG = deviceExtension->slotNumber;

    if(ChipType == HPT374 && slotData.u.bits.FunctionNumber == 1)  {
        reg = channel ? 0x57 : 0x53;
        GetPciConfig1(reg, val);
        SetPciConfig1(reg, val | 0x80);
    }
    else {
        reg = 0x5b;
        GetPciConfig1(reg, val);
        SetPciConfig1(reg, val & 0xfe);
    }
    GetPciConfig1(0x5a, res);
    res = res & (channel ? 0x01 : 0x02);
    SetPciConfig1(reg, val);
    return !res;
} // end hpt_cable80()


ULONG
NTAPI
via_cable80(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG channel               // physical channel number (0-1)
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;

    ULONG reg50;
    ULONG a;
    ULONG i, j;
    BOOLEAN res;

    GetPciConfig1(0x50, reg50);

    switch(ChipType) {
    case VIA133:
        a = 8;
        break;
    case VIA100:
        a = 4;
        break;
    case VIA66:
        a = 2;
        break;
    default:
        return false;
    }

    res = FALSE;
    for (j=0; j>=2; i -= 8) {
        i = (3-(channel*2+j))*8;
        if (((reg50 >> (i & 0x10)) & 8) &&
            ((reg50 >> i) & 0x20) &&
             (((reg50 >> i) & 7) < a)) {

            res |= TRUE; //(1 << (1 - (i >> 4)));
        }
    }
    return res;

} // end via_cable80()

BOOLEAN
NTAPI
generic_cable80(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG channel,               // physical channel number (0-1)
    IN ULONG pci_reg,
    IN ULONG bit_offs
    )
{
    PVOID HwDeviceExtension = (PVOID)deviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;

    //ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    PHW_CHANNEL chan;
    ULONG  c; // logical channel (for Compatible Mode controllers)
    UCHAR tmp8;

    c = channel - deviceExtension->Channel; // logical channel (for Compatible Mode controllers)
    chan = &deviceExtension->chan[c];

    GetPciConfig1(pci_reg, tmp8);
    if(!(tmp8 & (1 << (channel << bit_offs)))) {
        chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
        return FALSE;
    }

    return TRUE;
} // end generic_cable80()

VOID
NTAPI
UniAtaReadLunConfig(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG channel,  // physical channel
    IN ULONG ldev
    )
{
    ULONG tmp32;
    PHW_CHANNEL chan;
    PHW_LU_EXTENSION   LunExt;
    ULONG c;

    c = channel - deviceExtension->Channel; // logical channel

    chan = &deviceExtension->chan[c];
    ldev &= 0x01;
    LunExt = &(deviceExtension->lun[c*2+ldev]);

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"ReadCacheEnable", 1);
    LunExt->opt_ReadCacheEnable = tmp32 ? TRUE : FALSE;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"WriteCacheEnable", 1);
    LunExt->opt_WriteCacheEnable = tmp32 ? TRUE : FALSE;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"MaxTransferMode", chan->MaxTransferMode);
    LunExt->opt_MaxTransferMode = tmp32;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"PreferedTransferMode", 0xffffffff);
    LunExt->opt_PreferedTransferMode = tmp32;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"ReadOnly", 0);
    if(tmp32 <= 2) {
        LunExt->opt_ReadOnly = (UCHAR)tmp32;
    }

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"GeomType", 0xffffffff);
    if(tmp32 > 2) {
        tmp32 = 0xffffffff;
    }
    LunExt->opt_GeomType = tmp32;

    tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, ldev, L"Hidden", 0);
    if(tmp32) {
        LunExt->DeviceFlags |= DFLAGS_HIDDEN;
    }

    return;
} // end UniAtaReadLunConfig()

BOOLEAN
NTAPI
AtapiReadChipConfig(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG channel // physical channel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan;
    ULONG  tmp32;
    ULONG  c; // logical channel (for Compatible Mode controllers)

    KdPrint2((PRINT_PREFIX "AtapiReadChipConfig: devExt %#x\n", deviceExtension ));
    ASSERT(deviceExtension);

    if(channel != CHAN_NOT_SPECIFIED) {
        c = channel - deviceExtension->Channel; // logical channel (for Compatible Mode controllers)
    } else {
        c = CHAN_NOT_SPECIFIED;
    }

    KdPrint2((PRINT_PREFIX "AtapiReadChipConfig: dev %#x, ph chan %d\n", DeviceNumber, channel ));

    if(channel == CHAN_NOT_SPECIFIED) {
        if(AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"ForceSimplex", FALSE)) {
            deviceExtension->simplexOnly = TRUE;
        }
        deviceExtension->opt_AtapiDmaZeroTransfer = FALSE;
        deviceExtension->opt_AtapiDmaControlCmd   = FALSE;
        deviceExtension->opt_AtapiDmaRawRead      = TRUE;
        deviceExtension->opt_AtapiDmaReadWrite    = TRUE;
    }

    if(c == CHAN_NOT_SPECIFIED) {
        KdPrint2((PRINT_PREFIX "MaxTransferMode (base): %#x\n", deviceExtension->MaxTransferMode));
        for(c=0; c<deviceExtension->NumberChannels; c++) {
            chan = &deviceExtension->chan[c];
            chan->MaxTransferMode = deviceExtension->MaxTransferMode;
            tmp32 = AtapiRegCheckDevValue(deviceExtension, channel, DEVNUM_NOT_SPECIFIED, L"MaxTransferMode", chan->MaxTransferMode);
            if(tmp32 != 0xffffffff) {
                KdPrint2((PRINT_PREFIX "MaxTransferMode (overriden): %#x\n", chan->MaxTransferMode));
                chan->MaxTransferMode = tmp32;
            }
            //UniAtaReadLunConfig(deviceExtension, c, 0);
            //UniAtaReadLunConfig(deviceExtension, c, 1);
        }

        deviceExtension->opt_AtapiDmaZeroTransfer =
            AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"AtapiDmaZeroTransfer", deviceExtension->opt_AtapiDmaZeroTransfer) ?
               TRUE : FALSE;

        deviceExtension->opt_AtapiDmaControlCmd =
            AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"AtapiDmaControlCmd", deviceExtension->opt_AtapiDmaControlCmd) ?
               TRUE : FALSE;

        deviceExtension->opt_AtapiDmaRawRead =
            AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"AtapiDmaRawRead", deviceExtension->opt_AtapiDmaRawRead) ?
               TRUE : FALSE;

        deviceExtension->opt_AtapiDmaReadWrite =
            AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"AtapiDmaReadWrite", deviceExtension->opt_AtapiDmaReadWrite) ?
               TRUE : FALSE;

    } else {
        chan = &deviceExtension->chan[c];
        chan->MaxTransferMode = deviceExtension->MaxTransferMode;
        tmp32 = AtapiRegCheckDevValue(deviceExtension, c, DEVNUM_NOT_SPECIFIED, L"MaxTransferMode", chan->MaxTransferMode);
        if(tmp32 != 0xffffffff) {
            KdPrint2((PRINT_PREFIX "MaxTransferMode (overriden): %#x\n", chan->MaxTransferMode));
            chan->MaxTransferMode = tmp32;
        }
        tmp32 = AtapiRegCheckDevValue(deviceExtension, c, DEVNUM_NOT_SPECIFIED, L"ReorderEnable", TRUE);
        chan->UseReorder = tmp32 ? TRUE : FALSE;

        UniAtaReadLunConfig(deviceExtension, channel, 0);
        UniAtaReadLunConfig(deviceExtension, channel, 1);
    }

    return TRUE;
} // end AtapiReadChipConfig()

BOOLEAN
NTAPI
AtapiChipInit(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG channel // physical channel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
#ifdef _DEBUG
    ULONG DeviceID = (deviceExtension->DevID >> 16) & 0xffff;
#endif
    ULONG RevID    =  deviceExtension->RevID;
//    ULONG i;
//    BUSMASTER_CONTROLLER_INFORMATION* DevTypeInfo;
    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;
    ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;
    PHW_CHANNEL chan;
    UCHAR  tmp8;
    USHORT tmp16;
    ULONG  tmp32;
    ULONG  c; // logical channel (for Compatible Mode controllers)
    BOOLEAN CheckCable = FALSE;
    //ULONG BaseIoAddress;

    switch(channel) {
    case CHAN_NOT_SPECIFIED_CHECK_CABLE:
        CheckCable = TRUE;
        /* FALLTHROUGH */
    case CHAN_NOT_SPECIFIED:
        c = CHAN_NOT_SPECIFIED;
        break;
    default:
        c = channel - deviceExtension->Channel; // logical channel (for Compatible Mode controllers)
    }

    KdPrint2((PRINT_PREFIX "AtapiChipInit: dev %#x, ph chan %d\n", DeviceNumber, channel ));

    KdPrint2((PRINT_PREFIX "HwFlags: %#x\n", deviceExtension->HwFlags));
    KdPrint2((PRINT_PREFIX "VendorID/DeviceID/Rev %#x/%#x/%#x\n", VendorID, DeviceID, RevID));

    if(deviceExtension->UnknownDev) {
        KdPrint2((PRINT_PREFIX "  Unknown chip, exiting\n" ));
        //return TRUE;
        VendorID = 0xffffffff;
    }

    switch(VendorID) {
//  case ATA_ACARD_ID:
//      break;
    case ATA_ACER_LABS_ID:
        if(ChipFlags & UNIATA_SATA) {
            if(c == CHAN_NOT_SPECIFIED) {
                for(c=0; c<deviceExtension->NumberChannels; c++) {
                    chan = &deviceExtension->chan[c];
                    chan->ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
                    /* the southbridge might need the data corruption fix */
                    if(RevID == 0xc2 || RevID == 0xc3) {
                        AtapiAliSouthBridgeFixup(HwDeviceExtension, PCIConfiguration,
                                                 SystemIoBusNumber, slotNumber, c);
                    }
                }
                /* enable PCI interrupt */
                ChangePciConfig2(offsetof(PCI_COMMON_CONFIG, Command), (a & ~0x0400));
            }
        } else
        if(ChipFlags & ALINEW) {
            if(c == CHAN_NOT_SPECIFIED) {
                /* use device interrupt as byte count end */
                ChangePciConfig1(0x4a, (a | 0x20));
                /* enable cable detection and UDMA support on newer chips */
                ChangePciConfig1(0x4b, (a | 0x09));

                /* enable ATAPI UDMA mode */
                ChangePciConfig1(0x53, (a | 0x01));

            } else {
                // check 80-pin cable
                generic_cable80(deviceExtension, channel, 0x4a, 0);
            }
        } else {
            if(c == CHAN_NOT_SPECIFIED) {
                /* deactivate the ATAPI FIFO and enable ATAPI UDMA */
                ChangePciConfig1(0x53, (a | 0x03));
            } else {
                // ATAPI DMA R/O
                deviceExtension->chan[c].ChannelCtrlFlags |= CTRFLAGS_DMA_RO;
            }
        }
        break;
    case ATA_AMD_ID:
        if(c == CHAN_NOT_SPECIFIED) {
            /* set prefetch, postwrite */
            if(ChipFlags & AMDBUG) {
                ChangePciConfig1(0x41, (a & 0x0f));
            } else {
                ChangePciConfig1(0x41, (a | 0xf0));
            }
        }
        if(deviceExtension->MaxTransferMode < ATA_UDMA2)
            break;
        // check 80-pin cable
        if(!(ChipFlags & UNIATA_NO80CHK)) {
            if(c == CHAN_NOT_SPECIFIED) {
                // do nothing
            } else {
                generic_cable80(deviceExtension, channel, 0x42, 0);
            }
        }
        break;
    case ATA_HIGHPOINT_ID:

        if(c == CHAN_NOT_SPECIFIED) {

            if(ChipFlags & HPTOLD) {
                /* turn off interrupt prediction */
                ChangePciConfig1(0x51, (a & ~0x80));
            } else {
                /* turn off interrupt prediction */
                ChangePciConfig1(0x51, (a & ~0x03));
                ChangePciConfig1(0x55, (a & ~0x03));
                /* turn on interrupts */
                ChangePciConfig1(0x5a, (a & ~0x10));
                /* set clocks etc */
                if(ChipType < HPT372) {
                    SetPciConfig1(0x5b, 0x22);
                } else {
                    ChangePciConfig1(0x5b, ((a & 0x01) | 0x20));
                }
            }

        } else {
            // check 80-pin cable
            chan = &deviceExtension->chan[c];
            if(!hpt_cable80(deviceExtension, channel)) {
                chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
            }
        }
        break;
    case ATA_INTEL_ID: {
        USHORT reg54;
        if(ChipFlags & UNIATA_SATA) {
            if(c != CHAN_NOT_SPECIFIED)
                break;
#if 0
            /* force all ports active "the legacy way" */
            ChangePciConfig2(0x92, (a | 0x0f));
            /* enable PCI interrupt */
            ChangePciConfig2(/*PCIR_COMMAND*/0x04, (a & ~0x0400));

            if(!(ChipFlags & UNIATA_AHCI)) {
                /* ICH5 in compat mode has SATA ports as master/slave on 1 channel */
                GetPciConfig1(0x90, tmp8);
                if(!(tmp8 & 0x04)) {
            	    /* XXX SOS should be in intel_allocate if we grow it */
                    if(c == CHAN_NOT_SPECIFIED) {
                        for(c=0; c<deviceExtension->NumberChannels; c++) {
                            deviceExtension->chan[c].ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
                        }
                    } else {
                        deviceExtension->chan[c].ChannelCtrlFlags |= CTRFLAGS_NO_SLAVE;
                    }
                }
            }
#else
            /* enable PCI interrupt */
            ChangePciConfig2(/*PCIR_COMMAND*/0x04, (a & ~0x0400));
#endif
            break;
        }
        if(deviceExtension->MaxTransferMode < ATA_UDMA2)
            break;
        // check 80-pin cable
        if(c == CHAN_NOT_SPECIFIED) {
            // do nothing
        } else {
            chan = &deviceExtension->chan[c];
            GetPciConfig2(0x54, reg54);
            if( ((reg54 >> (channel*2)) & 30) != 30) {
                chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
            }
        }
        break; }
    case ATA_NVIDIA_ID: {
        if(ChipFlags & UNIATA_SATA) {
            if(c == CHAN_NOT_SPECIFIED) {
                ULONG offs = (ChipFlags & NV4OFF) ? 0x0440 : 0x0010;
                /* enable control access */
                ChangePciConfig1(0x50, (a | 0x04));
                KdPrint2((PRINT_PREFIX "BaseIoAddressSATA_0=%x\n", deviceExtension->BaseIoAddressSATA_0.Addr));
                if(ChipFlags & NVQ) {
                    /* clear interrupt status */
                    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs, 0x00ff00ff);
                    /* enable device and PHY state change interrupts */
                    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs+4, 0x000d000d);
                    /* disable NCQ support */
                    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x0400, 
                        AtapiReadPortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x0400) & 0xfffffff9);
                } else {
                    /* clear interrupt status */
                    AtapiWritePortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs, 0xff);
                    /* enable device and PHY state change interrupts */
                    AtapiWritePortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs+1, 0xdd);
                }
                /* enable PCI interrupt */
                ChangePciConfig2(offsetof(PCI_COMMON_CONFIG, Command), (a & ~0x0400));
            } else {
                //UniataSataPhyEnable(HwDeviceExtension, c);
            }
        } else {
            //UCHAR reg52;
            
            if(c == CHAN_NOT_SPECIFIED) {
                /* set prefetch, postwrite */
                ChangePciConfig1(0x51, (a & 0x0f));
            } else {
                // check 80-pin cable
                generic_cable80(deviceExtension, channel, 0x52, 1);
/*                chan = &deviceExtension->chan[c];
                GetPciConfig1(0x52, reg52);
                if( !((reg52 >> (channel*2)) & 0x01)) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }*/
            }
        }
        break; }
    case ATA_PROMISE_ID: {
        USHORT Reg50;
        switch(ChipType) {
        case PRNEW:
            /* setup clocks */
            if(c == CHAN_NOT_SPECIFIED) {
//            ATA_OUTB(ctlr->r_res1, 0x11, ATA_INB(ctlr->r_res1, 0x11) | 0x0a);
                AtapiWritePortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11, 
                    AtapiReadPortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x11) | 0x0a );
            }
            /* FALLTHROUGH */
        case PROLD:
            /* enable burst mode */
//            ATA_OUTB(ctlr->r_res1, 0x1f, ATA_INB(ctlr->r_res1, 0x1f) | 0x01);
            if(c == CHAN_NOT_SPECIFIED) {
                AtapiWritePortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x1f, 
                    AtapiReadPortEx1(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x1f) | 0x01 );
            } else {
                // check 80-pin cable
                chan = &deviceExtension->chan[c];
                GetPciConfig2(0x50, Reg50);
                if(Reg50 & (1 << (channel+10))) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }
            }
            break;
        case PRTX:
            if(c == CHAN_NOT_SPECIFIED) {
                // do nothing
            } else {
                // check 80-pin cable
                chan = &deviceExtension->chan[c];
                AtapiWritePort1(chan, IDX_BM_DeviceSpecific0, 0x0b);
                if(AtapiReadPort1(chan, IDX_BM_DeviceSpecific1) & 0x04) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }
            }
            break;
        case PRMIO:
            if(c == CHAN_NOT_SPECIFIED) {
                if(ChipFlags & PRSATA) {
                    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x6c, 0x000000ff);
                }
            } else {
                chan = &deviceExtension->chan[c];
                AtapiWritePort4(chan, IDX_BM_Command, 
                    (AtapiReadPort4(chan, IDX_BM_Command) & ~0x00000f8f) | channel );
                AtapiWritePort4(chan, IDX_BM_DeviceSpecific0, 0x00000001);
                // check 80-pin cable
                if(chan->MaxTransferMode < ATA_SA150 &&
                   (AtapiReadPort4(chan, IDX_BM_Command) & 0x01000000)) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }
            }
            break;
        }
        break; }
    case ATA_SERVERWORKS_ID:
        if(c == CHAN_NOT_SPECIFIED) {
            if(ChipType == SWKS33) {
                AtapiRosbSouthBridgeFixup(HwDeviceExtension, PCIConfiguration,
                                         SystemIoBusNumber, slotNumber);
            } else {
                ChangePciConfig1(0x5a, ((a & ~0x40) | ((ChipType == SWKS100) ? 0x03 : 0x02)));
            }
        }
        break;
    case ATA_ATI_ID:
        if(ChipType == SIIMIO) {
            KdPrint2((PRINT_PREFIX "ATI New\n"));
            // fall to SiI
        } else {
            KdPrint2((PRINT_PREFIX "ATI\n"));
            break;
        }
    case ATA_SILICON_IMAGE_ID:
  /*      if(ChipFlags & SIIENINTR) {
            SetPciConfig1(0x71, 0x01);
        }*/
        switch(ChipType) {
        case SIIMIO: {

            KdPrint2((PRINT_PREFIX "SII\n"));
            USHORT Reg79;

            if(c == CHAN_NOT_SPECIFIED) {
                if(ChipFlags & SIISETCLK)  {
                    KdPrint2((PRINT_PREFIX "SIISETCLK\n"));
                    GetPciConfig1(0x8a, tmp8);
                    if ((tmp8 & 0x30) != 0x10)
                        ChangePciConfig1(0x8a, (a & 0xcf) | 0x10);
                    GetPciConfig1(0x8a, tmp8);
                    if ((tmp8 & 0x30) != 0x10) {
                        KdPrint2((PRINT_PREFIX "Sil 0680 could not set ATA133 clock\n"));
                        deviceExtension->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA5);
                    }
                }
            }
            if(deviceExtension->MaxTransferMode < ATA_SA150) {
                // check 80-pin cable
                if(c == CHAN_NOT_SPECIFIED) {
                    // do nothing
                } else {
                    KdPrint2((PRINT_PREFIX "Check UDMA66 cable\n"));
                    chan = &deviceExtension->chan[c];
                    GetPciConfig2(0x79, Reg79);
                    if(Reg79 & (channel ? 0x02 : 0x01)) {
                        chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                    }
                }
            } else {
                ULONG unit01 = (c & 1);
                ULONG unit10 = (c & 2);
                /* enable/disable PHY state change interrupt */
                if(c == CHAN_NOT_SPECIFIED) {
                    for(c=0; c<deviceExtension->NumberChannels; c++) {
                        unit01 = (c & 1);
                        unit10 = (c & 2);
                        if(ChipFlags & SIINOSATAIRQ) {
                            KdPrint2((PRINT_PREFIX "Disable broken SATA intr on c=%x\n", c));
                            AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x148 + (unit01 << 7) + (unit10 << 8),0);
                        }
                    }
                } else {
                    if(ChipFlags & SIINOSATAIRQ) {
                        KdPrint2((PRINT_PREFIX "Disable broken SATA intr on c=%x\n", c));
                        AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x148 + (unit01 << 7) + (unit10 << 8),0);
                    } else {
                        KdPrint2((PRINT_PREFIX "Enable SATA intr on c=%x\n", c));
                        AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x148 + (unit01 << 7) + (unit10 << 8),(1 << 16));
                    }
                }
            }
            if(c == CHAN_NOT_SPECIFIED) {
                /* enable interrupt as BIOS might not */
                ChangePciConfig1(0x8a, (a & 0x3f));
                // Enable 3rd and 4th channels
                if (ChipFlags & SII4CH) {
                    KdPrint2((PRINT_PREFIX "SII4CH\n"));
                    AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x0200, 0x00000002);
                }
            } else {
                chan = &deviceExtension->chan[c];
                /* dont block interrupts */
                //ChangePciConfig4(0x48, (a & ~0x03c00000));
                tmp32 = AtapiReadPortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x48);
                AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x48, (1 << 22) << c);
                // flush
                tmp32 = AtapiReadPortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),0x48);

                /* Initialize FIFO PCI bus arbitration */
                GetPciConfig1(offsetof(PCI_COMMON_CONFIG, CacheLineSize), tmp8);
                if(tmp8) {
                    KdPrint2((PRINT_PREFIX "SII: CacheLine=%d\n", tmp32));
                    tmp8 = (tmp8/8)+1;
                    AtapiWritePort2(chan, IDX_BM_DeviceSpecific1, ((USHORT)tmp8) << 8 | tmp8);
    		} else {
                    KdPrint2((PRINT_PREFIX "SII: CacheLine=0 !!!\n"));
    		}
            }
            break; }

        case SIICMD: {

            KdPrint2((PRINT_PREFIX "SII_CMD\n"));
            if(c == CHAN_NOT_SPECIFIED) {
                /* Setup interrupts. */
                SetPciConfig1(0x71, 0x01);

    /*            GetPciConfig1(0x8a, tmp8);
                tmp8 &= ~(0x30);
                SetPciConfig1(0x71, tmp8);*/

                /* Use MEMORY READ LINE for reads.
                 * NOTE: Although not mentioned in the PCI0646U specs,
                 *       these bits are write only and won't be read
                 *       back as set or not.  The PCI0646U2 specs clarify
                 *       this point.
                 */
    /*            tmp8 |= 0x02;
                SetPciConfig1(0x71, tmp8);
    */
                /* Set reasonable active/recovery/address-setup values. */
                SetPciConfig1(0x53, 0x40);
                SetPciConfig1(0x54, 0x3f);
                SetPciConfig1(0x55, 0x40);
                SetPciConfig1(0x56, 0x3f);
                SetPciConfig1(0x57, 0x1c);
                SetPciConfig1(0x58, 0x3f);
                SetPciConfig1(0x5b, 0x3f);
            }

            break; }
        }
        break;
    case ATA_SIS_ID:
        if(c == CHAN_NOT_SPECIFIED) {
            switch(ChipType) {
            case SIS33:
                break;
            case SIS66:
            case SIS100OLD:
                ChangePciConfig1(0x52, (a & ~0x04));
                break;
            case SIS100NEW:
            case SIS133OLD:
                ChangePciConfig1(0x49, (a & ~0x01));
                break;
            case SIS133NEW:
                ChangePciConfig2(0x50, (a | 0x0008));
                ChangePciConfig2(0x52, (a | 0x0008));
                break;
            case SISSATA:
                ChangePciConfig2(0x04, (a & ~0x0400));
            }
        }
        if(ChipType == SIS133NEW) {
            USHORT tmp16;
            // check 80-pin cable
            if(c == CHAN_NOT_SPECIFIED) {
                // do nothing
            } else {
                chan = &deviceExtension->chan[c];
                GetPciConfig2(channel ? 0x52 : 0x50, tmp16);
                if(tmp16 & 0x8000) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }
            }
        } else {
            // check 80-pin cable
            if(c == CHAN_NOT_SPECIFIED) {
                // do nothing
            } else {
                chan = &deviceExtension->chan[c];
                GetPciConfig1(48, tmp8);
                if(tmp8 & (0x10 << channel)) {
                    chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
                }
            }
        }
        break;
    case ATA_VIA_ID:

        if(c == CHAN_NOT_SPECIFIED) {
            /* prepare for ATA-66 on the 82C686a and 82C596b */
            if(ChipFlags & VIACLK) {
                ChangePciConfig4(0x50, (a | 0x030b030b));
            }
            // no init for SATA
            if(ChipFlags & UNIATA_SATA) {
                /* enable PCI interrupt */
                ChangePciConfig2(/*PCIR_COMMAND*/0x04, (a & ~0x0400));
                break;
            }

            /* the southbridge might need the data corruption fix */
            if(ChipFlags & VIABUG) {
                AtapiViaSouthBridgeFixup(HwDeviceExtension, PCIConfiguration,
                                         SystemIoBusNumber, slotNumber);
            }
            /* set prefetch, postwrite */
            if(ChipType != VIA133) {
                ChangePciConfig1(0x41, (a | 0xf0));
            }

            /* set fifo configuration half'n'half */
            ChangePciConfig1(0x43, ((a & ((ChipFlags & VIAPRQ) ? 0x80 : 0x90)) | 0x2a));

            /* set status register read retry */
            ChangePciConfig1(0x44, (a | 0x08));

            /* set DMA read & end-of-sector fifo flush */
            ChangePciConfig1(0x46, ((a & 0x0c) | 0xf0));

            /* set sector size */
            SetPciConfig2(0x60, DEV_BSIZE);
            SetPciConfig2(0x68, DEV_BSIZE);
        } else {
            // check 80-pin cable
            chan = &deviceExtension->chan[c];
            if(!via_cable80(deviceExtension, channel)) {
                chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
            }
        }

        break;

    case ATA_ITE_ID:
        // Old chip
        if(ChipType == ITE_33)
            break;
        if(c == CHAN_NOT_SPECIFIED) {
            /* set PCI mode and 66Mhz reference clock */
            ChangePciConfig1(0x50, a & ~0x83);

            /* set default active & recover timings */
            SetPciConfig1(0x54, 0x31);
            SetPciConfig1(0x56, 0x31);
        } else {
            // check 80-pin cable
            GetPciConfig2(0x40, tmp16);
            chan = &deviceExtension->chan[c];
            if(!(tmp16 & (channel ? 0x08 : 0x04))) {
                chan->MaxTransferMode = min(deviceExtension->MaxTransferMode, ATA_UDMA2);
            }
        }

        break;
    default:
        if(c != CHAN_NOT_SPECIFIED) {
            // We don't know how to check for 80-pin cable on unknown controllers.
            // Later we shall check bit in IDENTIFY structure, but it is not reliable way.
            // So, leave this flag to use as hint in error recovery procedures
            KdPrint2((PRINT_PREFIX "UNIATA_NO80CHK\n"));
            deviceExtension->HwFlags |= UNIATA_NO80CHK;
        }
        break;
    }

    // In all places separate channels are inited after common controller init
    // The only exception is probe. But there we may need info about 40/80 pin and MaxTransferRate
    if(CheckCable && !(ChipFlags & (UNIATA_NO80CHK | UNIATA_SATA))) {
        for(c=0; c<deviceExtension->NumberChannels; c++) {
            AtapiChipInit(HwDeviceExtension, DeviceNumber, c);
        }
    }

    return TRUE;
} // end AtapiChipInit()

VOID
NTAPI
UniataInitMapBM(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PIDE_BUSMASTER_REGISTERS BaseIoAddressBM_0,
    IN BOOLEAN MemIo
    )
{
    PHW_CHANNEL chan;
    ULONG c;
    ULONG i;

    if(!BaseIoAddressBM_0) {
        MemIo = FALSE;
    }
    for(c=0; c<deviceExtension->NumberChannels; c++) {
        chan = &deviceExtension->chan[c];
        for (i=0; i<IDX_BM_IO_SZ; i++) {
            chan->RegTranslation[IDX_BM_IO+i].Addr  = BaseIoAddressBM_0 ? ((ULONG)BaseIoAddressBM_0 + i) : 0;
            chan->RegTranslation[IDX_BM_IO+i].MemIo = MemIo;
        }
        if(BaseIoAddressBM_0) {
            BaseIoAddressBM_0++;
        }
    }
} // end UniataInitMapBM()

VOID
NTAPI
UniataInitMapBase(
    IN PHW_CHANNEL chan,
    IN PIDE_REGISTERS_1 BaseIoAddress1,
    IN PIDE_REGISTERS_2 BaseIoAddress2
    )
{
    ULONG i;

    for (i=0; i<IDX_IO1_SZ; i++) {
        chan->RegTranslation[IDX_IO1+i].Addr = BaseIoAddress1 ? ((ULONG)BaseIoAddress1 + i) : 0;
        chan->RegTranslation[IDX_IO1+i].MemIo = FALSE;
    }
    for (i=0; i<IDX_IO2_SZ; i++) {
        chan->RegTranslation[IDX_IO2+i].Addr = BaseIoAddress2 ? ((ULONG)BaseIoAddress2 + i) : 0;
        chan->RegTranslation[IDX_IO2+i].MemIo = FALSE;
    }
    UniataInitSyncBaseIO(chan);
} // end UniataInitMapBase()

VOID
NTAPI
UniataInitSyncBaseIO(
    IN PHW_CHANNEL chan
    )
{
    RtlCopyMemory(&chan->RegTranslation[IDX_IO1_o], &chan->RegTranslation[IDX_IO1], IDX_IO1_SZ*sizeof(chan->RegTranslation[0]));
    RtlCopyMemory(&chan->RegTranslation[IDX_IO2_o], &chan->RegTranslation[IDX_IO2], IDX_IO2_SZ*sizeof(chan->RegTranslation[0]));
} // end UniataInitSyncBaseIO()
