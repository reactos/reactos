/*++

Copyright (c) 2002-2016 Alexandr A. Telyatnikov (Alter)

Module Name:
    id_probe.cpp

Abstract:
    This module scans PCI and ISA buses for IDE controllers
    and determines their Busmaster DMA capabilities

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

    Some parts of hardware-specific code were taken from FreeBSD 4.3-6.1 ATA driver by
         S�ren Schmidt, Copyright (c) 1998-2007

    Some parts of device detection code were taken from from standard ATAPI.SYS from NT4 DDK by
         Mike Glass (MGlass)
         Chuck Park (ChuckP)

    Device search/init algorithm is completly rewritten by
         Alter, Copyright (c) 2002-2004

    Fixes for Native/Compatible modes of onboard IDE controller by
         Vitaliy Vorobyov, deathsoft@yandex.ru (c) 2004

Licence:
    GPLv2

--*/

#include "stdafx.h"

PBUSMASTER_CONTROLLER_INFORMATION BMList = NULL;
ULONG         BMListLen = 0;
ULONG         IsaCount = 0;
ULONG         MCACount = 0;

BOOLEAN FirstMasterOk = FALSE;
// This is our own resource check,
// ReactOS allows to allocate same I/O range for both PCI and ISA controllers
BOOLEAN AtdiskPrimaryClaimed = FALSE;
BOOLEAN AtdiskSecondaryClaimed = FALSE;

#ifndef UNIATA_CORE

UCHAR         pciBuffer[256];
ULONG         maxPciBus = 16;

PDRIVER_OBJECT SavedDriverObject = NULL;

// local routines

ULONG
NTAPI
UniataEnumBusMasterController__(
/*    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again*/
    );

VOID
NTAPI
AtapiDoNothing(VOID)
{
    return;
} // end AtapiDoNothing()

#endif //UNIATA_CORE

USHORT
NTAPI
UniataEnableIoPCI(
    IN  ULONG                  busNumber,
    IN  ULONG                  slotNumber,
 IN OUT PPCI_COMMON_CONFIG     pciData
    )
{
    ULONG i;
    ULONG busDataRead;
    USHORT CmdOrig;

    // Enable Busmastering, IO-space and Mem-space
    // Note: write to CONFIG *may* cause controller to interrupt (not handled yet)
    // even if no bits are updated. Was observed on ICH7
    KdPrint2((PRINT_PREFIX "Enabling Mem/Io spaces and busmastering...\n"));
    KdPrint2((PRINT_PREFIX "Initial pciData.Command = %#x\n", pciData->Command));
    for(i=0; i<3; i++) {
        CmdOrig = pciData->Command;
        switch(i) {
        case 0:
            KdPrint2((PRINT_PREFIX "PCI_ENABLE_IO_SPACE\n"));
            pciData->Command |= PCI_ENABLE_IO_SPACE;
            break;
        case 1:
            KdPrint2((PRINT_PREFIX "PCI_ENABLE_MEMORY_SPACE\n"));
            pciData->Command |= PCI_ENABLE_MEMORY_SPACE;
            break;
        case 2:
            KdPrint2((PRINT_PREFIX "PCI_ENABLE_BUS_MASTER\n"));
            pciData->Command |= PCI_ENABLE_BUS_MASTER;
            break;
        }
        if(CmdOrig == pciData->Command) {
            continue;
        }
        HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotNumber,
                                &(pciData->Command),
                                offsetof(PCI_COMMON_CONFIG, Command),
                                sizeof(pciData->Command));

        // reread config space
        busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotNumber,
                                    pciData, PCI_COMMON_HDR_LENGTH);
        if(busDataRead < PCI_COMMON_HDR_LENGTH) {
            KdPrint2((PRINT_PREFIX "HalGetBusData() failed %#x\n", busDataRead));
            break;
        }
        KdPrint2((PRINT_PREFIX "New pciData.Command = %#x\n", pciData->Command));
    }
    KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData->u.type0.InterruptLine));
    KdPrint2((PRINT_PREFIX "Final pciData.Command = %#x\n", pciData->Command));
    return pciData->Command;
} // end UniataEnableIoPCI()

/*
    Get PCI address by ConfigInfo and RID
*/
ULONGIO_PTR
NTAPI
AtapiGetIoRange(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN PPCI_COMMON_CONFIG pciData,
    IN ULONG SystemIoBusNumber,
    IN ULONG rid,   //range id
    IN ULONG offset,
    IN ULONG length
    )
{
    ULONGIO_PTR io_start = 0;
    KdPrint2((PRINT_PREFIX "  AtapiGetIoRange:\n"));

    if(ConfigInfo->NumberOfAccessRanges <= rid)
        return 0;

    KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: rid %#x, start %#x, offs %#x, len %#x, mem %#x\n",
        rid,
        ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[rid].RangeStart),
        offset,
        length,
        (*ConfigInfo->AccessRanges)[rid].RangeInMemory
        ));

    if(!(*ConfigInfo->AccessRanges)[rid].RangeInMemory) {
        io_start = (pciData->u.type0.BaseAddresses[rid] & ~0x07/*PCI_ADDRESS_IOMASK*/) + offset;
        // if(pciData->u.type0.BaseAddresses[rid] != 0) ;)
        if(io_start > offset) {
            if(/*(WinVer_Id() <= WinVer_NT) &&*/ offset && rid == 4) {
                // MS atapi.sys does so for BusMaster controllers
                (*ConfigInfo->AccessRanges)[rid+1].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(io_start);
                (*ConfigInfo->AccessRanges)[rid+1].RangeLength = length;
            } else {
                (*ConfigInfo->AccessRanges)[rid].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(io_start);
                (*ConfigInfo->AccessRanges)[rid].RangeLength = length;
            }
            if((pciData->u.type0.BaseAddresses[rid] & PCI_ADDRESS_IO_SPACE)) {
                (*ConfigInfo->AccessRanges)[rid].RangeInMemory = FALSE;
            } else {
                KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: adjust mem 0 -> 1\n"));
                (*ConfigInfo->AccessRanges)[rid].RangeInMemory = TRUE;
            }
        } else {
            io_start = 0;
        }
    }

    if((*ConfigInfo->AccessRanges)[rid].RangeInMemory) {
        if(offset) {
            KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: can't map memory range with offset\n"));
            return 0;
        }
        io_start =
            // Get the system physical address for this IO range.
            ((ULONG_PTR)ScsiPortGetDeviceBase(HwDeviceExtension,
                        PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                        SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
                        ScsiPortConvertUlongToPhysicalAddress(
                            (ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[rid].RangeStart) &
                            ~0x07/*PCI_ADDRESS_IOMASK*/) + offset
                                                             ),
                        length,
                        (BOOLEAN)!(*ConfigInfo->AccessRanges)[rid].RangeInMemory)
                               );

        KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: %#x\n", io_start));
   //     if(io_start > offset) {
            return io_start;
   //     }
    }

    KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: (2) %#x\n", io_start));
    return io_start;

} // end AtapiGetIoRange()

#ifndef UNIATA_CORE

/*
    Do nothing, but build list of supported IDE controllers
    It is a hack, ScsiPort architecture assumes, that DriverEntry
    can support only KNOWN Vendor/Device combinations.
    Thus, we build list here. Later we pretend that always knew
    about found devices.

    We shall initiate ISA device init, but callback will use
    Hal routines directly in order to scan PCI bus.
*/
VOID
NTAPI
UniataEnumBusMasterController(
    IN PVOID DriverObject,
    PVOID Argument2
    )
{
    UniataEnumBusMasterController__();

} // end UniataEnumBusMasterController()

BOOLEAN
NTAPI
UniataCheckPCISubclass(
    BOOLEAN known,
    ULONG   RaidFlags,
    UCHAR   SubClass
    )
{
    if(known) {
        if((RaidFlags & UNIATA_RAID_CONTROLLER) &&
            SkipRaids) {
            KdPrint2((PRINT_PREFIX "Skip RAID\n"));
            return FALSE;
        }
        return TRUE;
    }
    KdPrint2((PRINT_PREFIX "unknown\n"));

    switch(SubClass) {
    case PCI_DEV_SUBCLASS_RAID:
        if(SkipRaids) {
            KdPrint2((PRINT_PREFIX "Skip RAID (2)\n"));
            return FALSE;
        }
        break;
    case PCI_DEV_SUBCLASS_IDE:
    case PCI_DEV_SUBCLASS_ATA:
        break;
    case PCI_DEV_SUBCLASS_SATA:
        break;
    default:
        KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
        return FALSE;
    }
    return TRUE;
} // end UniataCheckPCISubclass()

static CONST ULONG StdIsaPorts[] = {IO_WD1, IO_WD1 + ATA_ALTOFFSET, IO_WD2, IO_WD2 + ATA_ALTOFFSET, 0, 0};

/*
    Device initializaton callback
    Builds PCI device list using Hal routines (not ScsiPort wrappers)
*/
ULONG
NTAPI
UniataEnumBusMasterController__(
    )
{
//    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
//    PVOID HwDeviceExtension;
    PHW_DEVICE_EXTENSION  deviceExtension = NULL;
    PCHAR PciDevMap = NULL;
    PCI_SLOT_NUMBER       slotData;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 busNumber;
    ULONG                 slotNumber;
    ULONG                 funcNumber;
    BOOLEAN               no_buses = FALSE;
    BOOLEAN               no_ranges = FALSE;
    BOOLEAN               non_isa = TRUE;
    ULONG                 busDataRead;
//    BOOLEAN               SimplexOnly;

    UCHAR                 vendorString[5];
    UCHAR                 deviceString[5];
    PUCHAR                vendorStrPtr;
    PUCHAR                deviceStrPtr;

    UCHAR   BaseClass;                  // (ro)
    UCHAR   SubClass;                   // (ro)
    ULONG   VendorID;
    ULONG   DeviceID;
    ULONG   dev_id;

    USHORT  SubVendorID;
    USHORT  SubSystemID;

    ULONG   i;
    ULONG   pass=0;

    ULONG   RaidFlags;

    BOOLEAN found;
    BOOLEAN known;
    BOOLEAN NeedPciAltInit;
    BOOLEAN NonZeroSubId = 0;

    UCHAR IrqForCompat = 10;

    vendorStrPtr = vendorString;
    deviceStrPtr = deviceString;
    slotData.u.AsULONG = 0;

    KdPrint2((PRINT_PREFIX "UniataEnumBusMasterController__: maxPciBus=%d\n", maxPciBus));
    if(!maxPciBus) {
        return(SP_RETURN_NOT_FOUND);
    }
    /*HwDeviceExtension =*/
    deviceExtension = (PHW_DEVICE_EXTENSION)ExAllocatePool(NonPagedPool, sizeof(HW_DEVICE_EXTENSION));
    if(!deviceExtension) {
        KdPrint2((PRINT_PREFIX "!deviceExtension\n"));
        return(SP_RETURN_NOT_FOUND);
    }
    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));
    PciDevMap = (PCHAR)ExAllocatePool(NonPagedPool, maxPciBus*PCI_MAX_DEVICES);
    if(!PciDevMap) {
        KdPrint2((PRINT_PREFIX "!PciDevMap\n"));
        goto exit;
    }
    RtlZeroMemory(PciDevMap, maxPciBus*PCI_MAX_DEVICES);

    for(pass=0; pass<3; pass++) {
        KdPrint2((PRINT_PREFIX "  pass %d\n", pass));
        no_buses = FALSE;
        for(busNumber=0 ;busNumber<maxPciBus && !no_buses; busNumber++) {
            for(slotNumber=0; slotNumber<PCI_MAX_DEVICES  && !no_buses; slotNumber++) {
            NeedPciAltInit = FALSE;
            for(funcNumber=0; funcNumber<PCI_MAX_FUNCTION && !no_buses; funcNumber++) {

                if(pass) {
                    // use cached device presence map from the 1st pass
                    if(PciDevMap[busNumber*PCI_MAX_DEVICES + slotNumber] & (1 << funcNumber)) {
                        // ok
                    } else {
                        continue;
                    }
                }
//                KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x\n",busNumber,slotNumber,funcNumber));
                slotData.u.bits.DeviceNumber   = slotNumber;
                slotData.u.bits.FunctionNumber = funcNumber;

                busDataRead = HalGetBusData
                              //ScsiPortGetBusData
                                           (
                                            //HwDeviceExtension,
                                            PCIConfiguration, busNumber, slotData.u.AsULONG,
                                            &pciData, PCI_COMMON_HDR_LENGTH);
                // no more buses
                if(!busDataRead) {
                    no_buses = TRUE; // break all nested bus scan loops and continue with next pass
                    maxPciBus = busNumber;
                    break;
                }
                // indicate that system has PCI bus(es)
                hasPCI = TRUE;

                // no device in this slot
                if(busDataRead == 2) {
                    NeedPciAltInit = TRUE;
                    continue;
                }

                if(busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
                    NeedPciAltInit = TRUE;
                    continue;
                }

                VendorID  = pciData.VendorID;
                DeviceID  = pciData.DeviceID;
                BaseClass = pciData.BaseClass;
                SubClass  = pciData.SubClass;
                dev_id = VendorID | (DeviceID << 16);

                SubVendorID = pciData.u.type0.SubVendorID;
                SubSystemID = pciData.u.type0.SubSystemID;

                if(SubVendorID && SubSystemID) {
                  NonZeroSubId = 1;
                }

                KdPrint2((PRINT_PREFIX "DevId = %8.8X Class = %4.4X/%4.4X, SubVen/Sys %4.4x/%4.4x\n", dev_id, BaseClass, SubClass, SubVendorID, SubSystemID));

                // check for (g_opt_VirtualMachine == VM_AUTO) is performed inside each
                // VM check for debug purposes
                // Do not optimize :)
                if((VendorID == 0x80ee && DeviceID == 0xcafe) ||
                   (VendorID == 0x80ee && DeviceID == 0xbeef)) {
                    KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x - VirtualBox Guest Service\n",busNumber,slotNumber,funcNumber));
                    if(g_opt_VirtualMachine == VM_AUTO) {
                        g_opt_VirtualMachine = VM_VBOX;
                    }
                } else
                if((VendorID == 0x15ad) ||
                   (SubVendorID == 0x15ad && SubSystemID == 0x1976)) {
                    KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x - VMWare\n",busNumber,slotNumber,funcNumber));
                    if(g_opt_VirtualMachine == VM_AUTO) {
                        g_opt_VirtualMachine = VM_VMWARE;
                    }
                } else
                if(SubVendorID == 0x1af4 && SubSystemID == 0x1100) {
                    KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x - QEmu\n",busNumber,slotNumber,funcNumber));
                    if(g_opt_VirtualMachine == VM_AUTO) {
                        g_opt_VirtualMachine = VM_QEMU;
                    }
                } else
                if(VendorID == 0x1234 && DeviceID == 0x1111) {
                    KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x - Bochs\n",busNumber,slotNumber,funcNumber));
                    if(g_opt_VirtualMachine == VM_AUTO) {
                        g_opt_VirtualMachine = VM_BOCHS;
                    }
/*                } else
                if(pass>0 && !NonZeroSubId &&
                   VendorID == 0x8086 &&
                     (DeviceID == 0x7010 ||
                      DeviceID == 0x1230)) {
                    KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x - Bochs PIIX emulation\n",busNumber,slotNumber,funcNumber));
                    if(g_opt_VirtualMachine == VM_AUTO) {
                        g_opt_VirtualMachine = VM_BOCHS;
                    }*/
                }

                if(BaseClass != PCI_DEV_CLASS_STORAGE) {
                    continue;
                }

                KdPrint2((PRINT_PREFIX "-- BusID: %#x:%#x:%#x\n",busNumber,slotNumber,funcNumber));
                KdPrint2((PRINT_PREFIX "Storage Class\n"));
                KdPrint2((PRINT_PREFIX "DevId = %8.8X Class = %4.4X/%4.4X, ProgIf %2.2X\n", dev_id, BaseClass, SubClass, pciData.ProgIf ));
                // look for known chipsets
                found = FALSE;
                known = FALSE;

                if(pciData.u.type0.InterruptPin == 14 ||
                   pciData.u.type0.InterruptPin == 15 ||
                   pciData.u.type0.InterruptLine == 14 ||
                   pciData.u.type0.InterruptLine == 15) {
                    KdPrint2((PRINT_PREFIX "(!) InterruptPin = %#x\n", pciData.u.type0.InterruptPin));
                    KdPrint2((PRINT_PREFIX "(!) InterruptLine = %#x\n", pciData.u.type0.InterruptLine));
                }

                if(deviceExtension) {
                    deviceExtension->slotNumber = slotData.u.AsULONG;
                    deviceExtension->SystemIoBusNumber = busNumber;
                    deviceExtension->DevID = dev_id;
                    deviceExtension->RevID = pciData.RevisionID;
                    deviceExtension->AdapterInterfaceType = PCIBus;
                }

                found = (BOOLEAN)AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"Include", 0);
                if(!found) {
                    KdPrint2((PRINT_PREFIX "No force include, check exclude\n"));
                    found = !AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"Exclude", 0);
                    if(!found) {
                        KdPrint2((PRINT_PREFIX "Device excluded\n"));
                        continue;
                    }
                }

                //known = UniataChipDetect(HwDeviceExtension, NULL, -1, ConfigInfo, &SimplexOnly);
                i = Ata_is_dev_listed((PBUSMASTER_CONTROLLER_INFORMATION_BASE)&BusMasterAdapters[0], VendorID, DeviceID, 0, NUM_BUSMASTER_ADAPTERS);

                known = (i != BMLIST_TERMINATOR);
                if(known) {
                    deviceExtension->FullDevName = BusMasterAdapters[i].FullDevName;
                    RaidFlags = BusMasterAdapters[i].RaidFlags;
                } else {
                    deviceExtension->FullDevName = "Unknown Storage";
                    RaidFlags = 0;
                }
                found = UniataCheckPCISubclass(known, RaidFlags, SubClass);
                if(!found) {
                    KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
                    continue;
                }

                switch(dev_id) {
                /* additional checks for some supported chipsets */
                case 0xc6931080:
                    if (SubClass != PCI_DEV_SUBCLASS_IDE)
                        found = FALSE;
                    break;

                /* unknown chipsets, try generic DMA if it seems possible */
                default:
                    KdPrint2((PRINT_PREFIX "Default device\n"));
                    if(Ata_is_supported_dev(&pciData) ||
                       Ata_is_ahci_dev(&pciData))
                        found = TRUE;
                    break;
                }

                if(!found) {
                    continue;
                }

                KdPrint2((PRINT_PREFIX "found, pass %d\n", pass));

                KdPrint2((PRINT_PREFIX "InterruptPin = %#x\n", pciData.u.type0.InterruptPin));
                KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData.u.type0.InterruptLine));

                if(!pass && known) {
                    UniataEnableIoPCI(busNumber, slotData.u.AsULONG, &pciData);
                }
                // validate Mem/Io ranges
                no_ranges = TRUE;
                non_isa = TRUE;
                for(i=0; i<PCI_TYPE0_ADDRESSES; i++) {
                    if(pciData.u.type0.BaseAddresses[i] & ~0x7) {
                        no_ranges = FALSE;
                        //break;
                        KdPrint2((PRINT_PREFIX "Range %d = %#x\n", i, pciData.u.type0.BaseAddresses[i]));
                        if(i<4) {
                            if(StdIsaPorts[i] == (pciData.u.type0.BaseAddresses[i] & ~0x7)) {
                                non_isa = FALSE;
                            }
                        }
                    }
                }
                if(no_ranges) {
                    KdPrint2((PRINT_PREFIX "No PCI Mem/Io ranges found on device, skip it\n"));
                    continue;
                }
                if(!non_isa) {
                    KdPrint2((PRINT_PREFIX "standard ISA ranges on PCI, special case ?\n"));
                }

                if(pass) {
                    // fill list of detected devices
                    // it'll be used for further init
                    KdPrint2((PRINT_PREFIX "found suitable device\n"));
                    PBUSMASTER_CONTROLLER_INFORMATION newBMListPtr = BMList+BMListLen;

                    if(pass == 1) {
                        if(!IsMasterDev(&pciData)) {
                            continue;
                        }
                        if(AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"NativePCIMode", 0)) {
                            KdPrint2((PRINT_PREFIX "try switch to native mode\n"));

                            IrqForCompat = (UCHAR)AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"NativePCIModeIRQ", 0xff);
                            KdPrint2((PRINT_PREFIX "IrqForCompat = %#x\n", IrqForCompat));
                            if((IrqForCompat & 0xffffff00) /*||
                               (IrqForCompat & 0xff) > 31*/ ||
                               (IrqForCompat == 0xff)) {
                                IrqForCompat = 0x0b;
                                KdPrint2((PRINT_PREFIX "default to IRQ 11\n"));
                            }

                            //ChangePciConfig1(0x09, a | PCI_IDE_PROGIF_NATIVE_ALL); // ProgIf
                            pciData.ProgIf |= PCI_IDE_PROGIF_NATIVE_ALL;
                            HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                    &(pciData.ProgIf),
                                                    offsetof(PCI_COMMON_CONFIG, ProgIf),
                                                    sizeof(pciData.ProgIf));

                            // reread config space
                            busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                        &pciData, PCI_COMMON_HDR_LENGTH);
                            // check if the device have switched to Native Mode
                            if(IsMasterDev(&pciData)) {
                                KdPrint2((PRINT_PREFIX "Can't switch to native mode\n"));
                            } else {
                                KdPrint2((PRINT_PREFIX "switched to native mode\n"));
                                KdPrint2((PRINT_PREFIX "InterruptPin = %#x\n", pciData.u.type0.InterruptPin));
                                KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData.u.type0.InterruptLine));
                                // check if IRQ is assigned to device
                                if(!(pciData.u.type0.InterruptLine) ||
                                    (pciData.u.type0.InterruptLine == 0xff)) {
                                    KdPrint2((PRINT_PREFIX "assign interrupt for device\n"));
                                    pciData.u.type0.InterruptLine = IrqForCompat;
                                    HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                            &(pciData.u.type0.InterruptLine),
                                                            offsetof(PCI_COMMON_CONFIG, u.type0.InterruptLine),
                                                            sizeof(pciData.u.type0.InterruptLine));
                                } else {
                                    KdPrint2((PRINT_PREFIX "Auto-assigned interrupt line %#x\n",
                                        pciData.u.type0.InterruptLine));
                                    IrqForCompat = pciData.u.type0.InterruptLine;
                                }
                                KdPrint2((PRINT_PREFIX "reread config space\n"));
                                // reread config space
                                busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                            &pciData, PCI_COMMON_HDR_LENGTH);
                                KdPrint2((PRINT_PREFIX "busDataRead = %#x\n", busDataRead));
                                KdPrint2((PRINT_PREFIX "reread InterruptLine = %#x\n", pciData.u.type0.InterruptLine));
                                // check if we have successfully assigned IRQ to device
                                if((pciData.u.type0.InterruptLine != IrqForCompat) ||
                                   (pciData.u.type0.InterruptLine == 0xff) ||
                                   !pciData.u.type0.InterruptLine) {
                                    KdPrint2((PRINT_PREFIX "can't assign interrupt for device, revert to compat mode\n"));
                                    pciData.u.type0.InterruptLine = 0xff;
                                    KdPrint2((PRINT_PREFIX "set IntrLine to 0xff\n"));
                                    HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                            &(pciData.u.type0.InterruptLine),
                                                            offsetof(PCI_COMMON_CONFIG, u.type0.InterruptLine),
                                                            sizeof(pciData.u.type0.InterruptLine));
                                    KdPrint2((PRINT_PREFIX "clear PCI_IDE_PROGIF_NATIVE_ALL\n"));
                                    pciData.ProgIf &= ~PCI_IDE_PROGIF_NATIVE_ALL;
                                    HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                            &(pciData.ProgIf),
                                                            offsetof(PCI_COMMON_CONFIG, ProgIf),
                                                            sizeof(pciData.ProgIf));
                                    // reread config space
                                    KdPrint2((PRINT_PREFIX "reread config space on revert\n"));
                                    busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                                &pciData, PCI_COMMON_HDR_LENGTH);
                                } else {
                                    KdPrint2((PRINT_PREFIX "Assigned interrupt %#x for device\n", IrqForCompat));
                                    KdPrint2((PRINT_PREFIX "continue detection on next round\n"));
                                    continue;
                                }
                            }
                        }
                    } else
                    if(pass == 2) {
                        if(IsMasterDev(&pciData)) {
                            continue;
                        }
                    }

/*                        if(known) {
                        RtlCopyMemory(newBMListPtr, (PVOID)&(BusMasterAdapters[i]), sizeof(BUSMASTER_CONTROLLER_INFORMATION));
                    } else {*/
                    sprintf((PCHAR)vendorStrPtr, "%4.4lx", VendorID);
                    sprintf((PCHAR)deviceStrPtr, "%4.4lx", DeviceID);

                    RtlCopyMemory(&(newBMListPtr->VendorIdStr), (PCHAR)vendorStrPtr, 4);
                    RtlCopyMemory(&(newBMListPtr->DeviceIdStr), (PCHAR)deviceStrPtr, 4);

                    newBMListPtr->nVendorId = VendorID;
                    newBMListPtr->VendorId = (PCHAR)&(newBMListPtr->VendorIdStr);
                    newBMListPtr->VendorIdLength = 4;
                    newBMListPtr->nDeviceId = DeviceID;
                    newBMListPtr->DeviceId = (PCHAR)&(newBMListPtr->DeviceIdStr);
                    newBMListPtr->DeviceIdLength = 4;

                    newBMListPtr->RaidFlags = RaidFlags;
//                        }
                    newBMListPtr->slotNumber = slotData.u.AsULONG;
                    newBMListPtr->MasterDev = IsMasterDev(&pciData) ? 1 : 0;
                    newBMListPtr->busNumber = busNumber;

                    newBMListPtr->NeedAltInit = NeedPciAltInit;
                    newBMListPtr->Known = known;

                    if(!non_isa) {
                        KdPrint2((PRINT_PREFIX "* ISA ranges on PCI, special case !\n"));
                        // Do not fail init after unseccessfull call of UniataClaimLegacyPCIIDE()
                        // some SMP HALs fails to reallocate IO range
                        newBMListPtr->ChanInitOk |= 0x40;
                    }

                    KdPrint2((PRINT_PREFIX "Add to BMList, AltInit %d\n", NeedPciAltInit));
                } else {
                    KdPrint2((PRINT_PREFIX "count: BMListLen++\n"));
                    PciDevMap[busNumber*PCI_MAX_DEVICES + slotNumber] |= (1 << funcNumber);
                }

                BMListLen++;

            } // Function
            } // Slot
            if(!hasPCI) {
                break;
            }
        }
        if(!pass) {
             if(!BMListLen)
                 break;
             BMList = (PBUSMASTER_CONTROLLER_INFORMATION)ExAllocatePool(NonPagedPool,
                                         (BMListLen+1)*sizeof(BUSMASTER_CONTROLLER_INFORMATION));
             if(!BMList) {
                 BMListLen=0;
                 break;
             }
             RtlZeroMemory(BMList, (BMListLen+1)*sizeof(BUSMASTER_CONTROLLER_INFORMATION));
             no_buses = FALSE;
             BMListLen=0;
        }
    }
exit:
    KdPrint2((PRINT_PREFIX "  BMListLen=%x\n", BMListLen));
    if(deviceExtension) {
        ExFreePool(deviceExtension);
    }
    if(PciDevMap) {
        ExFreePool(PciDevMap);
    }
    return(SP_RETURN_NOT_FOUND);
} // end UniataEnumBusMasterController__()


/*
    Wrapper for read PCI config space
*/
ULONG
NTAPI
ScsiPortGetBusDataByOffset(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    IN PVOID  Buffer,
    IN ULONG  Offset,
    IN ULONG  Length
    )
{
    UCHAR tmp[256];
    ULONG busDataRead;

    if(Offset+Length > 256)
        return 0;

    busDataRead = HalGetBusData(
        //ScsiPortGetBusData(HwDeviceExtension,
                                     BusDataType,
                                     BusNumber,
                                     SlotNumber,
                                     &tmp,
                                     Offset+Length);
    if(busDataRead < Offset+Length) {
        if(busDataRead < Offset)
            return 0;
        return (Offset+Length-busDataRead);
    }
    RtlCopyMemory(Buffer, tmp+Offset, Length);
    return Length;
} // end ScsiPortGetBusDataByOffset()

/*
    Looks for devices from list on specified bus(es)/slot(s)
    returnts its index in list.
    If no matching record found, -1 is returned
*/
ULONG
NTAPI
AtapiFindListedDev(
    IN PBUSMASTER_CONTROLLER_INFORMATION_BASE BusMasterAdapters,
    IN ULONG     lim,
    IN PVOID  HwDeviceExtension,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    OUT PCI_SLOT_NUMBER* _slotData // optional
    )
{
    PCI_SLOT_NUMBER       slotData;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 busDataRead;

    ULONG busNumber;
    ULONG slotNumber;
    ULONG funcNumber;

    ULONG busNumber2;
    ULONG slotNumber2;

    ULONG i;

    KdPrint2((PRINT_PREFIX "AtapiFindListedDev: lim=%x, Bus=%x, Slot=%x\n", lim, BusNumber, SlotNumber));

    // set start/end bus
    if(BusNumber == PCIBUSNUM_NOT_SPECIFIED) {
        busNumber  = 0;
        busNumber2 = maxPciBus;
    } else {
        busNumber  = BusNumber;
        busNumber2 = BusNumber+1;
    }
    // set start/end slot
    if(SlotNumber == PCISLOTNUM_NOT_SPECIFIED) {
        slotNumber  = 0;
        slotNumber2 = PCI_MAX_DEVICES;
    } else {
        slotNumber  = SlotNumber;
        slotNumber2 = SlotNumber+1;
    }
    slotData.u.AsULONG = 0;

    KdPrint2((PRINT_PREFIX " scanning range Bus %x-%x, Slot %x-%x\n", busNumber, busNumber2-1, slotNumber, slotNumber2-1));

    for(            ; busNumber  < busNumber2       ; busNumber++ ) {
    for(            ; slotNumber < slotNumber2      ; slotNumber++) {
    for(funcNumber=0; funcNumber < PCI_MAX_FUNCTION ; funcNumber++) {

        slotData.u.bits.DeviceNumber   = slotNumber;
        slotData.u.bits.FunctionNumber = funcNumber;

        busDataRead = HalGetBusData(
            //ScsiPortGetBusData(HwDeviceExtension,
                                    PCIConfiguration, busNumber, slotData.u.AsULONG,
                                    &pciData, PCI_COMMON_HDR_LENGTH);
        // no more buses (this should not happen)
        if(!busDataRead) {
            continue;
        }
        // no device in this slot
        if(busDataRead == 2)
            continue;

        if(busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH)
            continue;
/*
        KdPrint2((PRINT_PREFIX "AtapiFindListedDev: b:s:f(%x:%x:%x) %4.4x/%4.4x/%2.2x\n",
                  busNumber, slotNumber, funcNumber,
                  pciData.VendorID, pciData.DeviceID, pciData.RevisionID));
                  */
        i = Ata_is_dev_listed(BusMasterAdapters, pciData.VendorID, pciData.DeviceID, pciData.RevisionID, lim);
        if(i != BMLIST_TERMINATOR) {
            if(_slotData)
                *_slotData = slotData;
            KdPrint2((PRINT_PREFIX "AtapiFindListedDev: found\n"));
            KdPrint2((PRINT_PREFIX "AtapiFindListedDev: b:s:f(%x:%x:%x) %4.4x/%4.4x/%2.2x\n",
                      busNumber, slotNumber, funcNumber,
                      pciData.VendorID, pciData.DeviceID, pciData.RevisionID));
            return i;
        }

    }}}
    return -1;
} // end AtapiFindListedDev()

/*
    Looks for device with specified Device/Vendor and Revision
    on specified Bus/Slot
*/
ULONG
NTAPI
AtapiFindDev(
    IN PVOID  HwDeviceExtension,
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG  BusNumber,
    IN ULONG  SlotNumber,
    IN ULONG  dev_id,
    IN ULONG  RevID
    )
{
    PCI_COMMON_CONFIG     pciData;
    ULONG                 funcNumber;
    ULONG                 busDataRead;

    ULONG   VendorID;
    ULONG   DeviceID;
    PCI_SLOT_NUMBER       slotData;

    slotData.u.AsULONG = SlotNumber;
    // walk through all Function Numbers
    for(funcNumber = 0; funcNumber < PCI_MAX_FUNCTION; funcNumber++) {

        slotData.u.bits.FunctionNumber = funcNumber;
        if(slotData.u.AsULONG == SlotNumber)
            continue;

        busDataRead = HalGetBusData(
            //busDataRead = ScsiPortGetBusData(HwDeviceExtension,
                                         PCIConfiguration,
                                         BusNumber,
                                         slotData.u.AsULONG,
                                         &pciData,
                                         PCI_COMMON_HDR_LENGTH);

        if (busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
            continue;
        }

        VendorID  = pciData.VendorID;
        DeviceID  = pciData.DeviceID;

        if(dev_id != (VendorID | (DeviceID << 16)) )
            continue;
        if(RevID >= pciData.RevisionID)
            return 1;
    }
    return 0;
} // end AtapiFindDev()

#endif //UNIATA_CORE


ULONG
NTAPI
UniataFindCompatBusMasterController1(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    return UniataFindBusMasterController(
        HwDeviceExtension,
        UlongToPtr(0x00000000),
        BusInformation,
        ArgumentString,
        ConfigInfo,
        Again
        );
} // end UniataFindCompatBusMasterController1()

ULONG
NTAPI
UniataFindCompatBusMasterController2(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    return UniataFindBusMasterController(
        HwDeviceExtension,
        UlongToPtr(0x80000000),
        BusInformation,
        ArgumentString,
        ConfigInfo,
        Again
        );
} // end UniataFindCompatBusMasterController2()

/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    BusInformation -
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/
ULONG
NTAPI
UniataFindBusMasterController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL           chan = NULL;
#ifndef UNIATA_CORE
    // this buffer must be global for UNIATA_CORE build
    PCI_COMMON_CONFIG     pciData;
#endif //UNIATA_CORE
    ULONG                 slotNumber;
    ULONG                 busDataRead;
    ULONG                 SystemIoBusNumber;
/*
    UCHAR                 vendorString[5];
    UCHAR                 deviceString[5];

    PUCHAR                vendorStrPtr;
    PUCHAR                deviceStrPtr;
*/
    UCHAR   BaseClass;
    UCHAR   SubClass;
    ULONG   VendorID;
    ULONG   DeviceID;
    ULONG   RevID;
    ULONG   dev_id;
    PCI_SLOT_NUMBER       slotData;

    ULONG   i;
    ULONG   channel;
    ULONG   c = 0;
    PUCHAR  ioSpace;
    UCHAR   statusByte;
    ULONG   bm_offset;

//    UCHAR   tmp8;
//    ULONG   irq;

    BOOLEAN found = FALSE;
    BOOLEAN MasterDev;
    BOOLEAN simplexOnly = FALSE;
#ifndef UNIATA_CORE
#ifdef UNIATA_INIT_ON_PROBE
    BOOLEAN skip_find_dev = FALSE;
#endif
#endif
    BOOLEAN AltInit = FALSE;

    SCSI_PHYSICAL_ADDRESS IoBasePort1;
    SCSI_PHYSICAL_ADDRESS IoBasePort2;

    PIDE_BUSMASTER_REGISTERS BaseIoAddressBM_0 = NULL;
    PIDE_REGISTERS_1 BaseIoAddress1[IDE_MAX_CHAN];
    PIDE_REGISTERS_2 BaseIoAddress2[IDE_MAX_CHAN];

    RtlZeroMemory(&BaseIoAddress1, sizeof(BaseIoAddress1));
    RtlZeroMemory(&BaseIoAddress2, sizeof(BaseIoAddress2));

    NTSTATUS status;
    PPORT_CONFIGURATION_INFORMATION_COMMON _ConfigInfo =
        (PPORT_CONFIGURATION_INFORMATION_COMMON)ConfigInfo;

    if(!WinVer_WDM_Model) {
        *Again = FALSE;
    } else {
        *Again = TRUE;
    }

    KdPrint2((PRINT_PREFIX "UniataFindBusMasterController: Context=%x, BMListLen=%d\n", Context, BMListLen));

    KdPrint2((PRINT_PREFIX "ConfigInfo->Length %x\n", ConfigInfo->Length));

    if(ForceSimplex) {
        KdPrint2((PRINT_PREFIX "ForceSimplex (1)\n"));
        simplexOnly = TRUE;
    }

    if(ConfigInfo->AdapterInterfaceType == Isa) {
        KdPrint2((PRINT_PREFIX "AdapterInterfaceType: Isa\n"));
    }
    if(InDriverEntry) {
        i = PtrToUlong(Context);
        if(i & 0x80000000) {
            AltInit = TRUE;
        }
        i &= ~0x80000000;
        channel = BMList[i].channel;
    } else {
        channel = 0;
        for(i=0; i<BMListLen; i++) {
            if(BMList[i].slotNumber == ConfigInfo->SlotNumber &&
               BMList[i].busNumber  == ConfigInfo->SystemIoBusNumber) {
                break;
            }
        }
        if(i >= BMListLen) {
            KdPrint2((PRINT_PREFIX "unexpected device arrival\n"));
            i = PtrToUlong(Context);
            if(FirstMasterOk) {
                channel = 1;
            }
            i &= ~0x80000000;
            if(i >= BMListLen) {
                KdPrint2((PRINT_PREFIX " => SP_RETURN_NOT_FOUND\n"));
                goto exit_notfound;
            }
        }
        BMList[i].channel = (UCHAR)channel;
    }

    bm_offset = channel ? ATA_BM_OFFSET1 : 0;

    KdPrint2((PRINT_PREFIX "bm_offset %x, channel %x \n", bm_offset, channel));

    if (!deviceExtension) {
        KdPrint2((PRINT_PREFIX "!deviceExtension => SP_RETURN_ERROR\n"));
        return SP_RETURN_ERROR;
    }
    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));
/*
    vendorStrPtr = vendorString;
    deviceStrPtr = deviceString;
*/
    slotNumber = BMList[i].slotNumber;
    SystemIoBusNumber = BMList[i].busNumber;


    KdPrint2((PRINT_PREFIX "AdapterInterfaceType=%#x\n",ConfigInfo->AdapterInterfaceType));
    KdPrint2((PRINT_PREFIX "IoBusNumber=%#x\n",ConfigInfo->SystemIoBusNumber));
    KdPrint2((PRINT_PREFIX "slotNumber=%#x\n",slotNumber));

    // this buffer must be global and already filled for UNIATA_CORE build
    busDataRead = HalGetBusData(
        //busDataRead = ScsiPortGetBusData(HwDeviceExtension,
                                     PCIConfiguration,
                                     SystemIoBusNumber,
                                     slotNumber,
                                     &pciData,
                                     PCI_COMMON_HDR_LENGTH);

#ifndef UNIATA_CORE
    if (busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH) {
        KdPrint2((PRINT_PREFIX "busDataRead < PCI_COMMON_HDR_LENGTH => SP_RETURN_ERROR\n"));
        goto exit_error;
    }

    KdPrint2((PRINT_PREFIX "busDataRead\n"));
    if (pciData.VendorID == PCI_INVALID_VENDORID) {
        KdPrint2((PRINT_PREFIX "PCI_INVALID_VENDORID\n"));
        goto exit_error;
    }
#endif //UNIATA_CORE

    VendorID  = pciData.VendorID;
    DeviceID  = pciData.DeviceID;
    BaseClass = pciData.BaseClass;
    SubClass  = pciData.SubClass;
    RevID     = pciData.RevisionID;
    dev_id = VendorID | (DeviceID << 16);
    slotData.u.AsULONG = slotNumber;
    KdPrint2((PRINT_PREFIX "DevId = %8.8X Class = %4.4X/%4.4X\n", dev_id, BaseClass, SubClass ));

    deviceExtension->slotNumber = slotNumber;
    deviceExtension->SystemIoBusNumber = SystemIoBusNumber;
    deviceExtension->DevID = dev_id;
    deviceExtension->RevID = RevID;
    deviceExtension->NumberChannels = IDE_DEFAULT_MAX_CHAN; // default
    deviceExtension->NumberLuns = IDE_MAX_LUN_PER_CHAN; // default
    deviceExtension->DevIndex = i;

    _snprintf(deviceExtension->Signature, sizeof(deviceExtension->Signature),
              "UATA%8.8x/%1.1x@%8.8x", dev_id, channel, slotNumber);

    if(BaseClass != PCI_DEV_CLASS_STORAGE) {
        KdPrint2((PRINT_PREFIX "BaseClass != PCI_DEV_CLASS_STORAGE => SP_RETURN_NOT_FOUND\n"));
        goto exit_notfound;
    }

    KdPrint2((PRINT_PREFIX "Storage Class\n"));

    // look for known chipsets
    if(VendorID != BMList[i].nVendorId ||
       DeviceID != BMList[i].nDeviceId) {
        KdPrint2((PRINT_PREFIX "device not suitable\n"));
        goto exit_notfound;
    }

    found = UniataCheckPCISubclass(BMList[i].Known, BMList[i].RaidFlags, SubClass);
    if(!found) {
        KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
        goto exit_notfound;
    }

    ConfigInfo->AlignmentMask = 0x00000003;

    MasterDev = IsMasterDev(&pciData);

    if(MasterDev) {
        KdPrint2((PRINT_PREFIX "MasterDev (1)\n"));
        deviceExtension->MasterDev = TRUE;
        KdPrint2((PRINT_PREFIX "Check exclude\n"));
        if(AtapiRegCheckDevValue(deviceExtension, channel, DEVNUM_NOT_SPECIFIED, L"Exclude", 0)) {
            KdPrint2((PRINT_PREFIX "Device excluded\n"));
            goto exit_notfound;
        }
    }

    status = UniataChipDetect(HwDeviceExtension, &pciData, i, ConfigInfo, &simplexOnly);
    switch(status) {
    case STATUS_SUCCESS:
        found = TRUE;
        break;
    case STATUS_NOT_FOUND:
        found = FALSE;
        break;
    default:
        KdPrint2((PRINT_PREFIX "FAILED => SP_RETURN_ERROR\n"));
        goto exit_error;
    }
    KdPrint2((PRINT_PREFIX "ForceSimplex = %d\n", simplexOnly));
    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (0)", deviceExtension->HwFlags));
    switch(dev_id) {
    /* additional checks for some supported chipsets */
    case 0xc6931080:
        if (SubClass != PCI_DEV_SUBCLASS_IDE) {
            KdPrint2((PRINT_PREFIX "0xc6931080, SubClass != PCI_DEV_SUBCLASS_IDE => found = FALSE\n"));
            found = FALSE;
        } else {
            found = FALSE;
        }
        break;

    /* unknown chipsets, try generic DMA if it seems possible */
    default:
        if (found)
            break;
        KdPrint2((PRINT_PREFIX "Default device\n"));
        if(Ata_is_supported_dev(&pciData)) {
            KdPrint2((PRINT_PREFIX "Ata_is_supported_dev\n"));
            found = TRUE;
        } else
        if(deviceExtension->HwFlags & UNIATA_AHCI) {
            KdPrint2((PRINT_PREFIX "AHCI candidate\n"));
            found = TRUE;
        } else {
            KdPrint2((PRINT_PREFIX "!Ata_is_supported_dev => found = FALSE\n"));
            found = FALSE;
        }
        deviceExtension->UnknownDev = TRUE;
        break;
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (1)", deviceExtension->HwFlags));
    if(!found) {
        KdPrint2((PRINT_PREFIX "!found => SP_RETURN_NOT_FOUND\n"));
        goto exit_notfound;
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (2)", deviceExtension->HwFlags));
    KdPrint2((PRINT_PREFIX "found suitable device\n"));

    /***********************************************************/
    /***********************************************************/
    /***********************************************************/

    deviceExtension->UseDpc = TRUE;
#ifndef UNIATA_CORE
    if (g_Dump) {
        deviceExtension->DriverMustPoll = TRUE;
        deviceExtension->UseDpc = FALSE;
        deviceExtension->simplexOnly = TRUE;
        deviceExtension->HwFlags |= UNIATA_NO_DPC;
    }
#endif //UNIATA_CORE
    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (3)", deviceExtension->HwFlags));
    if(deviceExtension->HwFlags & UNIATA_NO_DPC) {
        /* CMD 649, ROSB SWK33, ICH4 */
        KdPrint2((PRINT_PREFIX "UniataFindBusMasterController: UNIATA_NO_DPC (0)\n"));
        deviceExtension->UseDpc = FALSE;
    }

    if(MasterDev) {
        if((WinVer_Id() <= WinVer_NT) && AltInit && FirstMasterOk) {
            // this is the 2nd attempt to init this controller by OUR driver
            KdPrint2((PRINT_PREFIX "Skip primary/secondary claiming checks\n"));
        } else {
            if((channel==0) && ConfigInfo->AtdiskPrimaryClaimed) {
                KdPrint2((PRINT_PREFIX "Error: Primary channel already claimed by another driver\n"));
                goto exit_notfound;
            }
            if((channel==1) && ConfigInfo->AtdiskSecondaryClaimed) {
                KdPrint2((PRINT_PREFIX "Error: Secondary channel already claimed by another driver\n"));
                goto exit_notfound;
            }
        }
    }
    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        KdPrint2((PRINT_PREFIX "  AHCI registers layout\n"));
    } else
    if(deviceExtension->AltRegMap) {
        KdPrint2((PRINT_PREFIX "  Non-standard registers layout\n"));
        if(deviceExtension->HwFlags & UNIATA_SATA) {
            KdPrint2((PRINT_PREFIX "UNIATA_SATA -> IsBusMaster == TRUE\n"));
            if(!deviceExtension->BusMaster) {
                deviceExtension->BusMaster = DMA_MODE_BM;
            }
        }
    } else {
        deviceExtension->BusMaster = DMA_MODE_NONE;

        if(WinVer_WDM_Model && !deviceExtension->UnknownDev) {
            UniataEnableIoPCI(ConfigInfo->SystemIoBusNumber, slotData.u.AsULONG, &pciData);
        }
        // validate Mem/Io ranges
        //no_ranges = TRUE;
        {
            ULONG j;
            for(j=0; j<PCI_TYPE0_ADDRESSES; j++) {
                if(pciData.u.type0.BaseAddresses[j] & ~0x7) {
                    //no_ranges = FALSE;
                    //break;
                    KdPrint2((PRINT_PREFIX "Range %d = %#x\n", j, pciData.u.type0.BaseAddresses[j]));
                }
            }
        }

        if(IsBusMaster(&pciData)) {

            KdPrint2((PRINT_PREFIX "IsBusMaster == TRUE\n"));
            BaseIoAddressBM_0 = (PIDE_BUSMASTER_REGISTERS)
                (AtapiGetIoRange(HwDeviceExtension, ConfigInfo, &pciData, SystemIoBusNumber,
                                4, bm_offset, MasterDev ? 0x08 : 0x10/*ATA_BMIOSIZE*/)/* - bm_offset*/); //range id
            if(BaseIoAddressBM_0) {
                UniataInitMapBM(deviceExtension,
                                BaseIoAddressBM_0,
                                (*ConfigInfo->AccessRanges)[4].RangeInMemory ? TRUE : FALSE);
                deviceExtension->BusMaster = DMA_MODE_BM;
                deviceExtension->BaseIoAddressBM_0.Addr  = (ULONGIO_PTR)BaseIoAddressBM_0;
                if((*ConfigInfo->AccessRanges)[4].RangeInMemory) {
                    deviceExtension->BaseIoAddressBM_0.MemIo = TRUE;
                }
            }
            KdPrint2((PRINT_PREFIX "  BusMasterAddress (base): %#x\n", BaseIoAddressBM_0));
        }

        if(!deviceExtension->BusMaster) {
            KdPrint2((PRINT_PREFIX "  !BusMasterAddress -> PIO4\n"));
            deviceExtension->MaxTransferMode = ATA_PIO4;
        }

        if(deviceExtension->BusMaster && !MasterDev) {
            KdPrint2((PRINT_PREFIX "IsBusMaster == TRUE && !MasterDev\n"));
            statusByte = AtapiReadPort1(&(deviceExtension->chan[0]), IDX_BM_Status);
            KdPrint2((PRINT_PREFIX "  BM statusByte = %x\n", statusByte));
            if(statusByte == IDE_STATUS_WRONG) {
                KdPrint2((PRINT_PREFIX "  invalid port ?\n"));
                deviceExtension->BusMaster = DMA_MODE_NONE;
/*
                if(BaseIoAddressBM_0) {
                    ScsiPortFreeDeviceBase(HwDeviceExtension,
                                           BaseIoAddressBM_0);
                    BaseIoAddressBM_0 = NULL;
                }
*/
            } else
            if(statusByte & BM_STATUS_SIMPLEX_ONLY) {
                KdPrint2((PRINT_PREFIX "  BM_STATUS => simplexOnly\n"));
                simplexOnly = TRUE;
            }
        }
    }

    /*
     * the Cypress chip is a mess, it contains two ATA functions, but
     * both channels are visible on the first one.
     * simply ignore the second function for now, as the right
     * solution (ignoring the second channel on the first function)
     * doesn't work with the crappy ATA interrupt setup on the alpha.
     */
    if (dev_id == 0xc6931080 && slotData.u.bits.FunctionNumber > 1) {
        KdPrint2((PRINT_PREFIX "dev_id == 0xc6931080 && FunctionNumber > 1 => exit_findbm\n"));
        goto exit_findbm;
    }

    /* do extra chipset specific setups */
    AtapiReadChipConfig(HwDeviceExtension, i, CHAN_NOT_SPECIFIED);
    AtapiChipInit(HwDeviceExtension, i, CHAN_NOT_SPECIFIED_CHECK_CABLE);

    simplexOnly |= deviceExtension->simplexOnly;
    deviceExtension->simplexOnly |= simplexOnly;

    KdPrint2((PRINT_PREFIX "simplexOnly = %d (2)", simplexOnly));

    //TODO: fix hang with UseDpc=TRUE in Simplex mode
    //deviceExtension->UseDpc = TRUE;
    if(simplexOnly) {
        KdPrint2((PRINT_PREFIX "simplexOnly => UseDpc = FALSE\n"));
        deviceExtension->UseDpc = FALSE;
    }

    if(simplexOnly && MasterDev) {
        if(deviceExtension->NumberChannels < IDE_DEFAULT_MAX_CHAN) {
            KdPrint2((PRINT_PREFIX "set NumberChannels = %d\n", IDE_DEFAULT_MAX_CHAN));
            deviceExtension->NumberChannels = IDE_DEFAULT_MAX_CHAN;
            if(BaseIoAddressBM_0) {
                UniataInitMapBM(deviceExtension,
                                BaseIoAddressBM_0,
                                (*ConfigInfo->AccessRanges)[4].RangeInMemory ? TRUE : FALSE);
            }
        }
    }
    if((channel > 0) &&
       (deviceExtension->NumberChannels > 1)) {
        KdPrint2((PRINT_PREFIX "Error: channel > 0 && NumberChannels > 1\n"));
        goto exit_findbm;
    }

    // Indicate number of buses.
    ConfigInfo->NumberOfBuses = (UCHAR)(deviceExtension->NumberChannels);
    if(!ConfigInfo->InitiatorBusId[0]) {
        ConfigInfo->InitiatorBusId[0] = (CHAR)(IoGetConfigurationInformation()->ScsiPortCount);
        KdPrint2((PRINT_PREFIX "set ConfigInfo->InitiatorBusId[0] = %#x\n", ConfigInfo->InitiatorBusId[0]));
    }
    // Indicate four devices can be attached to the adapter
    ConfigInfo->MaximumNumberOfTargets = (UCHAR)(deviceExtension->NumberLuns);

    if (MasterDev) {
        KdPrint2((PRINT_PREFIX "MasterDev (2)\n"));
/*
        if((WinVer_Id() > WinVer_NT) ||
           (deviceExtension->NumberChannels > 1)) {

            KdPrint2((PRINT_PREFIX "2 channels & 2 irq for 1 controller Win 2000+\n"));

            if (ConfigInfo->AdapterInterfaceType == MicroChannel) {
                ConfigInfo->InterruptMode2 =
                ConfigInfo->InterruptMode = LevelSensitive;
            } else {
                ConfigInfo->InterruptMode2 =
                ConfigInfo->InterruptMode = Latched;
            }
            ConfigInfo->BusInterruptLevel = 14;
            ConfigInfo->BusInterruptLevel2 = 15;
        } else*/
        if(simplexOnly) {

            KdPrint2((PRINT_PREFIX "2 channels & 2 irq for 1 controller\n"));

            if (ConfigInfo->AdapterInterfaceType == MicroChannel) {
                ConfigInfo->InterruptMode2 =
                ConfigInfo->InterruptMode = LevelSensitive;
            } else {
                ConfigInfo->InterruptMode2 =
                ConfigInfo->InterruptMode = Latched;
            }
            ConfigInfo->BusInterruptLevel = 14;
            ConfigInfo->BusInterruptLevel2 = 15;
        } else {
            KdPrint2((PRINT_PREFIX "1 channels & 1 irq for 1 controller\n"));
            if (ConfigInfo->AdapterInterfaceType == MicroChannel) {
                ConfigInfo->InterruptMode = LevelSensitive;
            } else {
                ConfigInfo->InterruptMode = Latched;
            }
            ConfigInfo->BusInterruptLevel = (channel == 0 ? 14 : 15);
        }
    } else {
        KdPrint2((PRINT_PREFIX "!MasterDev\n"));
        ConfigInfo->SlotNumber = slotNumber;
        ConfigInfo->SystemIoBusNumber = SystemIoBusNumber;
        ConfigInfo->InterruptMode = LevelSensitive;

        /* primary and secondary channels share the same interrupt */
        if(!ConfigInfo->BusInterruptVector ||
            (ConfigInfo->BusInterruptVector != pciData.u.type0.InterruptLine)) {
            KdPrint2((PRINT_PREFIX "patch irq line = %#x\n", pciData.u.type0.InterruptLine));
            ConfigInfo->BusInterruptVector = pciData.u.type0.InterruptLine;  // set default value
            if(!ConfigInfo->BusInterruptVector) {
                KdPrint2((PRINT_PREFIX "patch irq line (2) = 10\n"));
                ConfigInfo->BusInterruptVector = 10;
            }
        }
    }
    ConfigInfo->MultipleRequestPerLu = TRUE;
    ConfigInfo->AutoRequestSense     = TRUE;
    ConfigInfo->TaggedQueuing        = TRUE;

    if((WinVer_Id() >= WinVer_NT) ||
       (ConfigInfo->Length >= sizeof(_ConfigInfo->comm) + sizeof(_ConfigInfo->nt4))) {
        KdPrint2((PRINT_PREFIX "update ConfigInfo->nt4\n"));
        _ConfigInfo->nt4.DeviceExtensionSize         = sizeof(HW_DEVICE_EXTENSION);
        _ConfigInfo->nt4.SpecificLuExtensionSize     = sizeof(HW_LU_EXTENSION);
        //if(deviceExtension->HwFlags & UNIATA_AHCI) {
           _ConfigInfo->nt4.SrbExtensionSize            = sizeof(ATA_REQ);
        //} else {
        //   _ConfigInfo->nt4.SrbExtensionSize            = FIELD_OFFSET(ATA_REQ, dma_tab) + sizeof(BM_DMA_ENTRY)*ATA_DMA_ENTRIES;
        //}
        KdPrint2((PRINT_PREFIX "using AtaReq sz %x\n", _ConfigInfo->nt4.SrbExtensionSize));
    }
    if((WinVer_Id() > WinVer_2k) ||
       (ConfigInfo->Length >= sizeof(_ConfigInfo->comm) + sizeof(_ConfigInfo->nt4) + sizeof(_ConfigInfo->w2k))) {
        KdPrint2((PRINT_PREFIX "update ConfigInfo->w2k: 64bit %d\n",
            deviceExtension->Host64));
#ifdef USE_OWN_DMA
        // We need not set Dma64BitAddresses since we perform address translation manually.
#else
        _ConfigInfo->w2k.Dma64BitAddresses           = deviceExtension->Host64;
#endif //USE_OWN_DMA
        _ConfigInfo->w2k.ResetTargetSupported        = TRUE;
        _ConfigInfo->w2k.MaximumNumberOfLogicalUnits = (UCHAR)deviceExtension->NumberLuns;
    }

    // Save the Interrupe Mode for later use
    deviceExtension->InterruptMode      = ConfigInfo->InterruptMode;
    deviceExtension->BusInterruptLevel  = ConfigInfo->BusInterruptLevel;
    deviceExtension->BusInterruptVector = ConfigInfo->BusInterruptVector;
    deviceExtension->Channel            = channel;
    deviceExtension->DevIndex           = i;
    deviceExtension->OrigAdapterInterfaceType
                                        = ConfigInfo->AdapterInterfaceType;
    deviceExtension->AlignmentMask      = ConfigInfo->AlignmentMask;
    deviceExtension->AdapterInterfaceType = PCIBus;

    KdPrint2((PRINT_PREFIX "chan[%d] InterruptMode: %d, Level %d, Level2 %d, Vector %d, Vector2 %d\n",
        channel,
        ConfigInfo->InterruptMode,
        ConfigInfo->BusInterruptLevel,
        ConfigInfo->BusInterruptLevel2,
        ConfigInfo->BusInterruptVector,
        ConfigInfo->BusInterruptVector2
        ));

    found = FALSE;

    if(deviceExtension->BusMaster) {

        KdPrint2((PRINT_PREFIX "Reconstruct ConfigInfo\n"));
#ifdef USE_OWN_DMA
        ConfigInfo->NeedPhysicalAddresses = FALSE;
#else
        ConfigInfo->NeedPhysicalAddresses = TRUE;
#endif //USE_OWN_DMA
        if(!MasterDev) {
//#ifdef USE_OWN_DMA
//            KdPrint2((PRINT_PREFIX "!MasterDev, own DMA\n"));
//#else
            KdPrint2((PRINT_PREFIX "set Dma32BitAddresses\n"));
            ConfigInfo->Dma32BitAddresses = TRUE;
//#endif //USE_OWN_DMA
        }

        // thanks to Vitaliy Vorobyov aka deathsoft@yandex.ru for
        // better solution:

        if(AltInit) {
            // I'm sorry, I have to do this
            // when Win doesn't

            if(ConfigInfo->AdapterInterfaceType == Isa /*&&
//               InDriverEntry*/) {
                KdPrint2((PRINT_PREFIX "AdapterInterfaceType Isa => PCIBus\n"));
                ConfigInfo->AdapterInterfaceType = PCIBus;
            }
            if(ConfigInfo->AdapterInterfaceType == PCIBus /*&&
//               InDriverEntry*/) {
                KdPrint2((PRINT_PREFIX "AdapterInterfaceType PCIBus, update address\n"));
                ConfigInfo->SlotNumber = slotNumber;
                ConfigInfo->SystemIoBusNumber = SystemIoBusNumber;
            }
        }

#ifndef USE_OWN_DMA
        ConfigInfo->Master        = TRUE;
        ConfigInfo->DmaWidth      = Width16Bits;
#endif //USE_OWN_DMA
        ConfigInfo->ScatterGather = TRUE;
    }
    ConfigInfo->MapBuffers = TRUE; // Need for PIO and OWN_DMA
    ConfigInfo->CachesData = TRUE;

    KdPrint2((PRINT_PREFIX "BMList[i].channel %#x, NumberChannels %#x, channel %#x\n",BMList[i].channel, deviceExtension->NumberChannels, channel));

    for (; channel < (BMList[i].channel + deviceExtension->NumberChannels); channel++, c++) {

        KdPrint2((PRINT_PREFIX "de %#x, Channel %#x\n",deviceExtension, channel));
        //PrintNtConsole("de %#x, Channel %#x, nchan %#x\n",deviceExtension, channel, deviceExtension->NumberChannels);
        chan = &deviceExtension->chan[c];

        KdPrint2((PRINT_PREFIX "chan = %#x\n", chan));
        //PrintNtConsole("chan = %#x, c=%#x\n", chan, c);
        AtapiSetupLunPtrs(chan, deviceExtension, c);

        /* do extra channel-specific setups */
        AtapiReadChipConfig(HwDeviceExtension, i, channel);
        //AtapiChipInit(HwDeviceExtension, i, channel);
        if(deviceExtension->HwFlags & UNIATA_AHCI) {
            KdPrint2((PRINT_PREFIX "  No more setup for AHCI channel\n"));
        } else
        if(deviceExtension->AltRegMap) {
            KdPrint2((PRINT_PREFIX "  Non-standard registers layout\n"));
        } else {
            // Check if the range specified is not used by another driver
            if(MasterDev) {
                KdPrint2((PRINT_PREFIX "set AccessRanges\n"));
                (*ConfigInfo->AccessRanges)[channel * 2 + 0].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(channel ? IO_WD2 : IO_WD1);
                (*ConfigInfo->AccessRanges)[channel * 2 + 0].RangeLength = ATA_IOSIZE;

                (*ConfigInfo->AccessRanges)[channel * 2 + 1].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress((channel ? IO_WD2 : IO_WD1) + ATA_ALTOFFSET);
                (*ConfigInfo->AccessRanges)[channel * 2 + 1].RangeLength = ATA_ALTIOSIZE;
            } else
            if(AltInit &&
               !(*ConfigInfo->AccessRanges)[channel * 2 + 0].RangeStart.QuadPart &&
               !(*ConfigInfo->AccessRanges)[channel * 2 + 1].RangeStart.QuadPart) {
                KdPrint2((PRINT_PREFIX "cheat ScsiPort, sync real PCI and ConfigInfo IO ranges\n"));
                AtapiGetIoRange(HwDeviceExtension, ConfigInfo, &pciData, SystemIoBusNumber,
                        channel * 2 + 0, 0, ATA_IOSIZE);
                AtapiGetIoRange(HwDeviceExtension, ConfigInfo, &pciData, SystemIoBusNumber,
                        channel * 2 + 1, 0, ATA_ALTIOSIZE);
            }

            IoBasePort1 = (*ConfigInfo->AccessRanges)[channel * 2 + 0].RangeStart;
            IoBasePort2 = (*ConfigInfo->AccessRanges)[channel * 2 + 1].RangeStart;

            if(!MasterDev) {
                if(!IoBasePort1.QuadPart || !IoBasePort2.QuadPart) {
                    KdPrint2((PRINT_PREFIX "ScsiPortValidateRange failed (1)\n"));
                    continue;
                }
            }

            if(!ScsiPortValidateRange(HwDeviceExtension,
                                      PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                      SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
                                      IoBasePort1,
                                      ATA_IOSIZE,
                                      TRUE) ) {
                KdPrint2((PRINT_PREFIX "ScsiPortValidateRange failed (1)\n"));
                continue;
            }

            if(!ScsiPortValidateRange(HwDeviceExtension,
                                      PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                      SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
                                      IoBasePort2,
                                      ATA_ALTIOSIZE,
                                      TRUE) ) {
                KdPrint2((PRINT_PREFIX "ScsiPortValidateRange failed (2)\n"));
                continue;
            }

            KdPrint2((PRINT_PREFIX "Getting IO ranges\n"));

            // Ok, translate adresses to io-space
            if(ScsiPortConvertPhysicalAddressToUlong(IoBasePort2)) {
                if(!(MasterDev /* || USE_16_BIT */)) {
                    KdPrint2((PRINT_PREFIX "!MasterDev mode\n"));
                    IoBasePort2 = ScsiPortConvertUlongToPhysicalAddress(
                                      ScsiPortConvertPhysicalAddressToUlong(IoBasePort2) + 2);
                }
            } else {
                KdPrint2((PRINT_PREFIX "use relative IoBasePort2\n"));
                IoBasePort2 = ScsiPortConvertUlongToPhysicalAddress(
                                  ScsiPortConvertPhysicalAddressToUlong(IoBasePort1) + ATA_PCCARD_ALTOFFSET);
            }

            // Get the system physical address for this IO range.
            ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                            MasterDev ? ConfigInfo->AdapterInterfaceType : PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                            MasterDev ? ConfigInfo->SystemIoBusNumber : SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
                                            IoBasePort1,
                                            ATA_IOSIZE,
                                            TRUE);
            KdPrint2((PRINT_PREFIX "IO range 1 %#x\n",ioSpace));

            // Check if ioSpace accessible.
            if (!ioSpace) {
                KdPrint2((PRINT_PREFIX "!ioSpace\n"));
                continue;
            }
/*
            if(deviceExtension->BusMaster) {
                KdPrint2((PRINT_PREFIX "set BusMaster io-range in DO\n"));
                // bm_offset already includes (channel ? ATA_BM_OFFSET1 : 0)
                deviceExtension->BaseIoAddressBM[c] = (PIDE_BUSMASTER_REGISTERS)
                    ((ULONG)(deviceExtension->BaseIoAddressBM_0) + bm_offset + (c ? ATA_BM_OFFSET1 : 0));
            }
*/
            //deviceExtension->BaseIoAddress1[c] = (PIDE_REGISTERS_1)(ioSpace);
            BaseIoAddress1[c] = (PIDE_REGISTERS_1)(ioSpace);

            // Get the system physical address for the second IO range.
            ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                            MasterDev ? ConfigInfo->AdapterInterfaceType : PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                            MasterDev ? ConfigInfo->SystemIoBusNumber : SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
                                            IoBasePort2,
                                            ATA_ALTIOSIZE,
                                            TRUE);
            KdPrint2((PRINT_PREFIX "IO range 2 %#x\n",ioSpace));

            BaseIoAddress2[c] = (PIDE_REGISTERS_2)(ioSpace);
            if(!ioSpace) {
                // Release all allocated resources
                KdPrint2((PRINT_PREFIX "!deviceExtension->BaseIoAddress2\n"));
                //ioSpace = (PUCHAR)BaseIoAddress1[c];
    //            goto free_iospace_1;
                found = FALSE;
                goto exit_findbm;
            }
            UniataInitMapBase(chan, BaseIoAddress1[c], BaseIoAddress2[c]);
        }
        //ioSpace = (PUCHAR)(deviceExtension->BaseIoAddress1[c]);

        DbgDumpRegTranslation(chan, IDX_IO1);
        DbgDumpRegTranslation(chan, IDX_IO2);
        DbgDumpRegTranslation(chan, IDX_BM_IO);
        DbgDumpRegTranslation(chan, IDX_SATA_IO);

        if(!(deviceExtension->HwFlags & UNIATA_AHCI)) {
#ifdef _DEBUG
            UniataDumpATARegs(chan);
#endif

#ifndef UNIATA_CORE
#ifdef UNIATA_INIT_ON_PROBE
//        if(deviceExtension->HwFlags & UNIATA_SATA) {
//#endif //UNIATA_INIT_ON_PROBE
            KdPrint2((PRINT_PREFIX "Check drive 0\n"));
            // Check master.
            SelectDrive(chan, 0);
            AtapiStallExecution(10);
            GetBaseStatus(chan, statusByte);
            skip_find_dev = FALSE;
            if(!(deviceExtension->HwFlags & UNIATA_NO_SLAVE) && (deviceExtension->NumberLuns > 1)) {
                if ((statusByte & 0xf8) == 0xf8 ||
                    (statusByte == 0xa5)) {
                    // Check slave.
                    KdPrint2((PRINT_PREFIX "Check drive 1\n"));
                    SelectDrive(chan, 1);
                    AtapiStallExecution(1);
                    GetBaseStatus(chan, statusByte);
                    if ((statusByte & 0xf8) == 0xf8 ||
                        (statusByte == 0xa5)) {
                        // No controller at this base address.
                        KdPrint2((PRINT_PREFIX "Empty channel\n"));
                        skip_find_dev = TRUE;
                    }
                }
            }

            // Search for devices on this controller.
            if (!skip_find_dev &&
                FindDevices(HwDeviceExtension,
                        0,
                        c)) {
                KdPrint2((PRINT_PREFIX "Found some devices\n"));
                found = TRUE;
            } else {
                KdPrint2((PRINT_PREFIX "no devices\n"));
    /*            KeBugCheckEx(0xc000000e,
                             ScsiPortConvertPhysicalAddressToUlong(IoBasePort1),
                             ScsiPortConvertPhysicalAddressToUlong(IoBasePort2),
                             (ULONG)(deviceExtension->BaseIoAddressBM[c]), skip_find_dev);*/
            }
//#ifdef UNIATA_INIT_ON_PROBE
//        }
#else //UNIATA_INIT_ON_PROBE
            KdPrint2((PRINT_PREFIX "clean IDE intr 0\n"));

            SelectDrive(chan, 0);
            AtapiStallExecution(10);
            GetBaseStatus(chan, statusByte);

            if(!(deviceExtension->HwFlags & UNIATA_NO_SLAVE) && (deviceExtension->NumberLuns > 1)) {
                KdPrint2((PRINT_PREFIX "clean IDE intr 1\n"));

                SelectDrive(chan, 1);
                AtapiStallExecution(1);
                GetBaseStatus(chan, statusByte);

                SelectDrive(chan, 0);
            }

            statusByte = GetDmaStatus(deviceExtension, c);
            KdPrint2((PRINT_PREFIX "  DMA status %#x\n", statusByte));
            if(statusByte & BM_STATUS_INTR) {
                // bullshit, we have DMA interrupt, but had never initiate DMA operation
                KdPrint2((PRINT_PREFIX "  clear unexpected DMA intr\n"));
                AtapiDmaDone(deviceExtension, 0, c, NULL);
                GetBaseStatus(chan, statusByte);
            }

#endif //UNIATA_INIT_ON_PROBE
        }
        found = TRUE;

        chan->PrimaryAddress = FALSE;
        // Claim primary or secondary ATA IO range.
        if (MasterDev) {
            KdPrint2((PRINT_PREFIX "claim Compatible controller\n"));
            if (channel == 0) {
                KdPrint2((PRINT_PREFIX "claim Primary\n"));
                AtdiskPrimaryClaimed =
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                chan->PrimaryAddress = TRUE;

                FirstMasterOk = TRUE;

            } else
            if (channel == 1) {
                KdPrint2((PRINT_PREFIX "claim Secondary\n"));
                AtdiskSecondaryClaimed =
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;

                FirstMasterOk = TRUE;
            }
        } else {
            if(chan->RegTranslation[IDX_IO1].Addr == IO_WD1 &&
               !chan->RegTranslation[IDX_IO1].MemIo) {
                KdPrint2((PRINT_PREFIX "claim Primary (PCI over ISA range)\n"));
                AtdiskPrimaryClaimed =
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
            }
            if(chan->RegTranslation[IDX_IO1].Addr == IO_WD2 &&
               !chan->RegTranslation[IDX_IO1].MemIo) {
                KdPrint2((PRINT_PREFIX "claim Secondary (PCI over ISA range)\n"));
                AtdiskSecondaryClaimed =
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
            }
        }

        AtapiDmaAlloc(HwDeviceExtension, ConfigInfo, c);
#else //UNIATA_CORE
        }
        found = TRUE;
#endif //UNIATA_CORE
    } // end for(channel)

exit_findbm:

#ifndef UNIATA_CORE
    if(!found) {
        KdPrint2((PRINT_PREFIX "exit: !found\n"));
        if(BaseIoAddress1[0])
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   BaseIoAddress1[0]);
        if(BaseIoAddress2[0])
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   BaseIoAddress2[0]);

        if(BaseIoAddress1[1])
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   BaseIoAddress1[1]);
        if(BaseIoAddress2[1])
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   BaseIoAddress2[1]);

        if(BaseIoAddressBM_0)
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   BaseIoAddressBM_0);

        if(deviceExtension->BaseIoAHCI_0.Addr) {
            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   deviceExtension->BaseIoAHCI_0.pAddr);
        }

        KdPrint2((PRINT_PREFIX "return SP_RETURN_NOT_FOUND\n"));
        goto exit_notfound;
    } else {

        KdPrint2((PRINT_PREFIX "exit: init spinlock\n"));
        //KeInitializeSpinLock(&(deviceExtension->DpcSpinLock));
        deviceExtension->ActiveDpcChan =
        deviceExtension->FirstDpcChan = CHAN_NOT_SPECIFIED;

        BMList[i].Isr2Enable = FALSE;

        KdPrint2((PRINT_PREFIX "MasterDev=%#x, NumberChannels=%#x, Isr2DevObj=%#x\n",
            MasterDev, deviceExtension->NumberChannels, BMList[i].Isr2DevObj));

        // ConnectIntr2 should be moved to HwInitialize
        status = UniataConnectIntr2(HwDeviceExtension);

        KdPrint2((PRINT_PREFIX "MasterDev=%#x, NumberChannels=%#x, Isr2DevObj=%#x\n",
            MasterDev, deviceExtension->NumberChannels, BMList[i].Isr2DevObj));

        if(/*WinVer_WDM_Model &&*/ MasterDev) {
            KdPrint2((PRINT_PREFIX "do not tell system, that we know about PCI IO ranges\n"));
/*            if(BaseIoAddressBM_0) {
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       BaseIoAddressBM_0);
            }*/
            (*ConfigInfo->AccessRanges)[4].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
            (*ConfigInfo->AccessRanges)[4].RangeLength = 0;
            (*ConfigInfo->AccessRanges)[5].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
            (*ConfigInfo->AccessRanges)[5].RangeLength = 0;
        }

        if(!NT_SUCCESS(status)) {
            KdPrint2((PRINT_PREFIX "failed\n"));
            found = FALSE;
            goto exit_findbm;
        }

        KdPrint2((PRINT_PREFIX "final chan[%d] InterruptMode: %d, Level %d, Level2 %d, Vector %d, Vector2 %d\n",
            channel,
            ConfigInfo->InterruptMode,
            ConfigInfo->BusInterruptLevel,
            ConfigInfo->BusInterruptLevel2,
            ConfigInfo->BusInterruptVector,
            ConfigInfo->BusInterruptVector2
            ));


    }
#endif //UNIATA_CORE

    KdPrint2((PRINT_PREFIX "return SP_RETURN_FOUND\n"));
    //PrintNtConsole("return SP_RETURN_FOUND, de %#x, c0.lun0 %#x\n", deviceExtension, deviceExtension->chan[0].lun[0]);

    if(MasterDev) {
        KdPrint2((PRINT_PREFIX "Attempt %d of MasterDev ok\n", AltInit));
        FirstMasterOk = TRUE;
    }

    ConfigInfo->NumberOfBuses++; // add virtual channel for communication port
    return SP_RETURN_FOUND;

exit_error:
    UniataFreeLunExt(deviceExtension);
    return SP_RETURN_ERROR;

exit_notfound:
    UniataFreeLunExt(deviceExtension);
    return SP_RETURN_NOT_FOUND;

} // end UniataFindBusMasterController()

#ifndef UNIATA_CORE

/*
   This is for claiming PCI Busmaster in compatible mode under WDM OSes
*/
NTSTATUS
NTAPI
UniataClaimLegacyPCIIDE(
    ULONG i
    )
{
    NTSTATUS status;
    PCM_RESOURCE_LIST resourceList = NULL;
    UNICODE_STRING devname;
#ifdef __REACTOS__
    PCM_RESOURCE_LIST oldResList = NULL;
#endif

    KdPrint2((PRINT_PREFIX "UniataClaimLegacyPCIIDE:\n"));

    if(BMList[i].PciIdeDevObj) {
        KdPrint2((PRINT_PREFIX "Already initialized\n"));
        return STATUS_UNSUCCESSFUL;
    }

    RtlInitUnicodeString(&devname, L"\\Device\\uniata_PCIIDE");
    status = IoCreateDevice(SavedDriverObject, sizeof(PCIIDE_DEVICE_EXTENSION),
                            /*NULL*/ &devname, FILE_DEVICE_UNKNOWN,
                            0, FALSE, &(BMList[i].PciIdeDevObj));

    if(!NT_SUCCESS(status)) {
        KdPrint2((PRINT_PREFIX "IoCreateDevice failed %#x\n", status));
        return status;
    }

    resourceList = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool,
                                sizeof(CM_RESOURCE_LIST));

    if (!resourceList) {
        KdPrint2((PRINT_PREFIX "!resourceList\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
del_do:
        IoDeleteDevice(BMList[i].PciIdeDevObj);
        BMList[i].PciIdeDevObj          = NULL;
#ifdef __REACTOS__
        if (oldResList)
            ExFreePool(oldResList);
#endif
        return status;
    }

    RtlZeroMemory(
        resourceList,
        sizeof(CM_RESOURCE_LIST));

#ifdef __REACTOS__
    oldResList = resourceList;
#endif

    // IoReportDetectedDevice() should be used for WDM OSes

    // TODO: check if resourceList is actually used inside HalAssignSlotResources()
    // Note: with empty resourceList call to HalAssignSlotResources() fails on some HW
    // e.g. Intel ICH4, but works with non-empty.

    resourceList->Count = 1;
    resourceList->List[0].InterfaceType = PCIBus;
    resourceList->List[0].BusNumber = BMList[i].busNumber;
    // we do not report IO ranges since they are used/claimed by ISA part(s)
    resourceList->List[0].PartialResourceList.Count = 0;

    RtlInitUnicodeString(&devname, L"PCIIDE");
    status = HalAssignSlotResources(&SavedRegPath,
                                    &devname,
                                    SavedDriverObject,
                                    BMList[i].PciIdeDevObj,
                                    PCIBus,
                                    BMList[i].busNumber,
                                    BMList[i].slotNumber,
                                    &resourceList);

    if (!NT_SUCCESS(status)) {
        KdPrint2((PRINT_PREFIX "HalAssignSlotResources failed %#x\n", status));
        // this is always deallocated inside HalAssignSlotResources() implementation
        //ExFreePool(resourceList);
        goto del_do;
    }

#ifdef __REACTOS__
    ExFreePool(resourceList);
    ExFreePool(oldResList);
#endif

    KdPrint2((PRINT_PREFIX "ok %#x\n", status));
    BMList[i].ChanInitOk |= 0x80;

    return status;
} // end UniataClaimLegacyPCIIDE()


/*++

Routine Description:

    This function is called to initialize 2nd device object for
    multichannel controllers.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    ULONG

--*/
NTSTATUS
NTAPI
UniataConnectIntr2(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG i = deviceExtension->DevIndex;
    NTSTATUS status;
    PISR2_DEVICE_EXTENSION Isr2DevExt;
    WCHAR devname_str[33];
    UNICODE_STRING devname;

    KdPrint2((PRINT_PREFIX "Init ISR:\n"));

    /*
      We MUST register 2nd ISR for multichannel controllers even for UP systems.
      This is needed for cases when
      multichannel controller generate interrupt while we are still in its ISR for
      other channle's interrupt. New interrupt must be detected and queued for
      further processing. If we do not do this, system will not route this
      interrupt to main ISR (since it is busy) and we shall get to infinite loop
      looking for interrupt handler.
    */

    if(!deviceExtension->MasterDev && (deviceExtension->NumberChannels > 1) &&   // do not touch MasterDev
       !deviceExtension->simplexOnly && /*                        // this is unnecessary on simplex controllers
       !BMList[i].Isr2DevObj*/                                    // handle re-init under w2k+
       /*!ForceSimplex*/
       /*(CPU_num > 1) &&  // unnecessary for UP systems*/
       TRUE) {
        // Ok, continue...
        KdPrint2((PRINT_PREFIX "Multichannel native mode, go...\n"));
#ifndef UNIATA_USE_XXableInterrupts
        // If we raise IRQL to TIMER value, other interrupt cannot occure on the same CPU
/*        if(KeNumberProcessors < 2) {
            KdPrint2((PRINT_PREFIX "Unnecessary (?), UP machine\n"));
            //return STATUS_SUCCESS;
        }*/
#endif //UNIATA_USE_XXableInterrupts
    } else {
        KdPrint2((PRINT_PREFIX "Unnecessary\n"));
        return STATUS_SUCCESS;
    }

    if(BMList[i].Isr2DevObj) {
        KdPrint2((PRINT_PREFIX "Already initialized [%d] %#x\n", i, BMList[i].Isr2DevObj));
        return STATUS_SUCCESS;
    }

    KdPrint2((PRINT_PREFIX "Create DO\n"));

    devname.Length =
        _snwprintf(devname_str, sizeof(devname_str)/sizeof(WCHAR)-1,
              L"\\Device\\uniata%d_2ch", i);
    devname_str[devname.Length] = 0;
    devname.Length *= sizeof(WCHAR);
    devname.MaximumLength = devname.Length;
    devname.Buffer = devname_str;

    KdPrint2((PRINT_PREFIX "DO name:  len(%d, %d), %S\n", devname.Length, devname.MaximumLength, devname.Buffer));

    status = IoCreateDevice(SavedDriverObject, sizeof(ISR2_DEVICE_EXTENSION),
                            /*NULL*/ &devname, FILE_DEVICE_UNKNOWN,
                            0, FALSE, &(BMList[i].Isr2DevObj));

    if(!NT_SUCCESS(status)) {
        KdPrint2((PRINT_PREFIX "IoCreateDevice failed %#x\n", status));
        return status;
    }

    KdPrint2((PRINT_PREFIX "HalGetInterruptVector\n"));
    KdPrint2((PRINT_PREFIX "  OrigAdapterInterfaceType=%d\n", deviceExtension->OrigAdapterInterfaceType));
    KdPrint2((PRINT_PREFIX "  SystemIoBusNumber=%d\n", deviceExtension->SystemIoBusNumber));
    KdPrint2((PRINT_PREFIX "  BusInterruptLevel=%d\n", deviceExtension->BusInterruptLevel));
    KdPrint2((PRINT_PREFIX "  BusInterruptVector=%d\n", deviceExtension->BusInterruptVector));
    BMList[i].Isr2Vector = HalGetInterruptVector(
        deviceExtension->OrigAdapterInterfaceType,
        deviceExtension->SystemIoBusNumber,
        deviceExtension->BusInterruptLevel,
        deviceExtension->BusInterruptVector,
        &(BMList[i].Isr2Irql),
        &(BMList[i].Isr2Affinity));

    Isr2DevExt = (PISR2_DEVICE_EXTENSION)(BMList[i].Isr2DevObj->DeviceExtension);
    Isr2DevExt->HwDeviceExtension = deviceExtension;
    Isr2DevExt->DevIndex = i;

    KdPrint2((PRINT_PREFIX "isr2_de %#x\n", Isr2DevExt));
    KdPrint2((PRINT_PREFIX "isr2_vector %#x\n", BMList[i].Isr2Vector));
    KdPrint2((PRINT_PREFIX "isr2_irql %#x\n", BMList[i].Isr2Irql));
    KdPrint2((PRINT_PREFIX "isr2_affinity %#x\n", BMList[i].Isr2Affinity));

//            deviceExtension->QueueNewIrql = BMList[i].Isr2Irql;

    KdPrint2((PRINT_PREFIX "IoConnectInterrupt\n"));
    status = IoConnectInterrupt(
        &(BMList[i].Isr2InterruptObject),
        AtapiInterrupt2,
        Isr2DevExt,
        NULL,
        BMList[i].Isr2Vector,
        BMList[i].Isr2Irql,
        BMList[i].Isr2Irql,
        (KINTERRUPT_MODE)(deviceExtension->InterruptMode),
        TRUE,
        BMList[i].Isr2Affinity,
        FALSE);

    if(!NT_SUCCESS(status)) {
        KdPrint2((PRINT_PREFIX "IoConnectInterrupt failed\n"));
        IoDeleteDevice(BMList[i].Isr2DevObj);
        BMList[i].Isr2DevObj          = NULL;
        BMList[i].Isr2InterruptObject = NULL;
        return status;
    }

    //deviceExtension->Isr2DevObj = BMList[i].Isr2DevObj;

    return status;
} // end UniataConnectIntr2()

NTSTATUS
NTAPI
UniataDisconnectIntr2(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG i = deviceExtension->DevIndex;
//    NTSTATUS status;

    KdPrint2((PRINT_PREFIX "Deinit ISR:\n"));

    if(!BMList[i].Isr2DevObj) {
        KdPrint2((PRINT_PREFIX "Already uninitialized %#x\n"));
        return STATUS_SUCCESS;
    }

    IoDisconnectInterrupt(BMList[i].Isr2InterruptObject);

    BMList[i].Isr2InterruptObject = NULL;

    IoDeleteDevice(BMList[i].Isr2DevObj);

    BMList[i].Isr2DevObj          = NULL;
    //deviceExtension->Isr2DevObj   = NULL;

    return STATUS_SUCCESS;
} // end UniataDisconnectIntr2()

#endif //UNIATA_CORE

BOOLEAN
NTAPI
AtapiCheckIOInterference(
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    ULONG portBase) {
    // check if Primary/Secondary Master IDE claimed
    if((portBase == IO_WD1) &&
       (ConfigInfo->AtdiskPrimaryClaimed || AtdiskPrimaryClaimed)) {
        KdPrint2((PRINT_PREFIX "AtapiCheckIOInterference: AtdiskPrimaryClaimed\n"));
        return TRUE;
    } else
    if((portBase == IO_WD2) &&
       (ConfigInfo->AtdiskSecondaryClaimed || AtdiskSecondaryClaimed)) {
        KdPrint2((PRINT_PREFIX "AtapiCheckIOInterference: AtdiskSecondaryClaimed\n"));
        return TRUE;
    }
    return FALSE;
} // end AtapiCheckIOInterference()

/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/
ULONG
NTAPI
AtapiFindIsaController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan;
    PULONG               adapterCount    = (PULONG)Context;
    PUCHAR               ioSpace = NULL;
    ULONG                i;
    ULONG                irq=0;
    ULONG                portBase=0;
    ULONG                retryCount;
//    BOOLEAN              atapiOnly;
    UCHAR                statusByte, statusByte2;
    BOOLEAN              preConfig = FALSE;
    //
    PIDE_REGISTERS_1 BaseIoAddress1;
    PIDE_REGISTERS_2 BaseIoAddress2 = NULL;

    // The following table specifies the ports to be checked when searching for
    // an IDE controller.  A zero entry terminates the search.
    static CONST ULONG AdapterAddresses[5] = {IO_WD1, IO_WD2, IO_WD1-8, IO_WD2-8, 0};
//    CONST UCHAR Channels[5] = {0, 1, 0, 1, 0};

    // The following table specifies interrupt levels corresponding to the
    // port addresses in the previous table.
    static CONST ULONG InterruptLevels[5] = {14, 15, 11, 10, 0};

    KdPrint2((PRINT_PREFIX "AtapiFindIsaController (ISA):\n"));

    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }
    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));

    KdPrint2((PRINT_PREFIX "  assume max PIO4\n"));
    deviceExtension->MaxTransferMode = ATA_PIO4;
    deviceExtension->NumberChannels = 1;
    deviceExtension->NumberLuns = IDE_MAX_LUN_PER_CHAN; // default

    if(!UniataAllocateLunExt(deviceExtension, UNIATA_ALLOCATE_NEW_LUNS)) {
        goto exit_error;
    }

    chan = &(deviceExtension->chan[0]);
    AtapiSetupLunPtrs(chan, deviceExtension, 0);

    deviceExtension->AdapterInterfaceType =
    deviceExtension->OrigAdapterInterfaceType
                                        = ConfigInfo->AdapterInterfaceType;

#ifndef UNIATA_CORE

    /* do extra chipset specific setups */
    AtapiReadChipConfig(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, CHAN_NOT_SPECIFIED);
    AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, CHAN_NOT_SPECIFIED);

    // Check to see if this is a special configuration environment.
    portBase = irq = 0;
    if (ArgumentString) {

        irq = AtapiParseArgumentString(ArgumentString, "Interrupt");
        if (irq ) {

            // Both parameters must be present to proceed
            portBase = AtapiParseArgumentString(ArgumentString, "BaseAddress");
            if (!portBase) {

                // Try a default search for the part.
                irq = 0;
            }
        }
    }

#endif //UNIATA_CORE
/*
    for(i=0; i<2; i++) {
        if((*ConfigInfo->AccessRanges)[i].RangeStart) {
            KdPrint2((PRINT_PREFIX "  IoRange[%d], start %#x, len %#x, mem %#x\n",
                i,
                ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[i].RangeStart),
                (*ConfigInfo->AccessRanges)[i].RangeLength,
                (*ConfigInfo->AccessRanges)[i].RangeInMemory
                ));
        }
    }
*/
//    if((*ConfigInfo->AccessRanges)[0].RangeStart) {
        portBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
//    }
    if(portBase) {
        if(!AtapiCheckIOInterference(ConfigInfo, portBase)) {
            ioSpace =  (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                             ConfigInfo->AdapterInterfaceType,
                                             ConfigInfo->SystemIoBusNumber,
                                             (*ConfigInfo->AccessRanges)[0].RangeStart,
                                             (*ConfigInfo->AccessRanges)[0].RangeLength,
                                             (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));
        } else {
            // do not touch resources, just fail later inside loop on next call to
            // AtapiCheckIOInterference()
        }
        *Again = FALSE;
        // Since we have pre-configured information we only need to go through this loop once
        preConfig = TRUE;
        KdPrint2((PRINT_PREFIX "  preconfig, portBase=%x, len=%x\n", portBase, (*ConfigInfo->AccessRanges)[0].RangeLength));
    }

    // Scan through the adapter address looking for adapters.
#ifndef UNIATA_CORE
    while (AdapterAddresses[*adapterCount] != 0) {
#else
    do {
#endif //UNIATA_CORE

        retryCount = 4;
        deviceExtension->DevIndex = (*adapterCount); // this is used inside AtapiRegCheckDevValue()
        KdPrint2((PRINT_PREFIX "AtapiFindIsaController: adapterCount=%d\n", *adapterCount));

        for (i = 0; i < deviceExtension->NumberLuns; i++) {
            // Zero device fields to ensure that if earlier devices were found,
            // but not claimed, the fields are cleared.
            deviceExtension->lun[i].DeviceFlags &= ~(DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT | DFLAGS_TAPE_DEVICE);
        }
        // Get the system physical address for this IO range.

        // Check if configInfo has the default information
        // if not, we go and find ourselves
        if (preConfig == FALSE) {

            ULONG portBase_reg = 0;
            ULONG irq_reg = 0;

            if (!portBase) {
                portBase = AdapterAddresses[*adapterCount];
                KdPrint2((PRINT_PREFIX "portBase[%d]=%x\n", *adapterCount, portBase));
            } else {
                KdPrint2((PRINT_PREFIX "portBase=%x\n", portBase));
            }

            portBase_reg = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"PortBase", 0);
            irq_reg      = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"Irq", 0);
            if(portBase_reg && irq_reg) {
                KdPrint2((PRINT_PREFIX "use registry settings portBase=%x, irq=%d\n", portBase_reg, irq_reg));
                portBase = portBase_reg;
                irq = irq_reg;
            }
            // check if Primary/Secondary Master IDE claimed
            if(AtapiCheckIOInterference(ConfigInfo, portBase)) {
                goto next_adapter;
            }
            ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress(portBase),
                                            ATA_IOSIZE,
                                            TRUE);

        } else {
            KdPrint2((PRINT_PREFIX "preconfig portBase=%x\n", portBase));
            // Check if Primary/Secondary Master IDE claimed
            // We can also get here from preConfig branc with conflicting portBase
            //   (and thus, w/o ioSpace allocated)
            if(AtapiCheckIOInterference(ConfigInfo, portBase)) {
                goto not_found;
            }
        }
        BaseIoAddress1 = (PIDE_REGISTERS_1)ioSpace;
next_adapter:
        // Update the adapter count.
        (*adapterCount)++;

        // Check if ioSpace accessible.
        if (!ioSpace) {
            KdPrint2((PRINT_PREFIX "AtapiFindIsaController: !ioSpace\n"));
            portBase = 0;
            continue;
        }

        // Get the system physical address for the second IO range.
        if (BaseIoAddress1) {
            if(preConfig &&
               !ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[1].RangeStart)) {
                KdPrint2((PRINT_PREFIX "AtapiFindIsaController: PCMCIA ?\n"));
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONGIO_PTR)BaseIoAddress1 + 0x0E),
                                                ATA_ALTIOSIZE,
                                                TRUE);
            } else {
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONGIO_PTR)BaseIoAddress1 + ATA_ALTOFFSET),
                                                ATA_ALTIOSIZE,
                                                TRUE);
            }
        }
        BaseIoAddress2 = (PIDE_REGISTERS_2)ioSpace;
        KdPrint2((PRINT_PREFIX "  BaseIoAddress1=%x\n", BaseIoAddress1));
        KdPrint2((PRINT_PREFIX "  BaseIoAddress2=%x\n", BaseIoAddress2));
        if(!irq) {
            KdPrint2((PRINT_PREFIX "  expected InterruptLevel=%x\n", InterruptLevels[*adapterCount - 1]));
        }

        UniataInitMapBase(chan, BaseIoAddress1, BaseIoAddress2);
        UniataInitMapBM(deviceExtension, 0, FALSE);

#ifdef _DEBUG
        UniataDumpATARegs(chan);
#endif

        // Select master.
        SelectDrive(chan, 0);

        statusByte = AtapiReadPort1(chan, IDX_IO1_i_Status);
        statusByte2 = AtapiReadPort1(chan, IDX_IO2_AltStatus);
        if((statusByte ^ statusByte2) & ~IDE_STATUS_INDEX) {
            KdPrint2((PRINT_PREFIX "AtapiFindIsaController: Status %x vs AltStatus %x missmatch, abort init ?\n", statusByte, statusByte2));

            if(BaseIoAddress2) {
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       (PCHAR)BaseIoAddress2);
                BaseIoAddress2 = NULL;
            }
            BaseIoAddress2 = (PIDE_REGISTERS_2)((ULONGIO_PTR)BaseIoAddress1 + 0x0E);
            KdPrint2((PRINT_PREFIX "  try BaseIoAddress2=%x\n", BaseIoAddress2));
            ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                            ConfigInfo->AdapterInterfaceType,
                                            ConfigInfo->SystemIoBusNumber,
                                            ScsiPortConvertUlongToPhysicalAddress((ULONGIO_PTR)BaseIoAddress2),
                                            ATA_ALTIOSIZE,
                                            TRUE);
            if(!ioSpace) {
                BaseIoAddress2 = NULL;
                KdPrint2((PRINT_PREFIX "    abort (0)\n"));
                goto not_found;
            }
            UniataInitMapBase(chan, BaseIoAddress1, BaseIoAddress2);
            statusByte = AtapiReadPort1(chan, IDX_IO1_i_Status);
            statusByte2 = AtapiReadPort1(chan, IDX_IO2_AltStatus);
            if((statusByte ^ statusByte2) & ~IDE_STATUS_INDEX) {
                KdPrint2((PRINT_PREFIX "    abort: Status %x vs AltStatus %x missmatch\n", statusByte, statusByte2));
                goto not_found;
            }
        }

retryIdentifier:

        // Select master.
        SelectDrive(chan, 0);

        // Check if card at this address.
        AtapiWritePort1(chan, IDX_IO1_o_CylinderLow, 0xAA);
        statusByte = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);

        // Check if indentifier can be read back.
        if (AtapiReadPort1(chan, IDX_IO1_i_CylinderLow) != 0xAA ||
            statusByte == IDE_STATUS_WRONG) {

            KdPrint2((PRINT_PREFIX "AtapiFindIsaController: Identifier read back from Master (%#x)\n",
                        statusByte));

            statusByte = AtapiReadPort1(chan, IDX_IO2_AltStatus);

            if (statusByte != IDE_STATUS_WRONG && (statusByte & IDE_STATUS_BUSY)) {

                i = 0;

                // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
                // warm boots don't clear.
                do {
                    AtapiStallExecution(1000);
                    statusByte = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_Status);
                    KdPrint2((PRINT_PREFIX
                                "AtapiFindIsaController: First access to status %#x\n",
                                statusByte));
                } while ((statusByte & IDE_STATUS_BUSY) && ++i < 10);

                if (retryCount-- && (!(statusByte & IDE_STATUS_BUSY))) {
                    goto retryIdentifier;
                }
            }

            // Select slave.
            SelectDrive(chan, 1);
            statusByte = AtapiReadPort1(chan, IDX_IO2_AltStatus);

            // See if slave is present.
            AtapiWritePort1(chan, IDX_IO1_o_CylinderLow, 0xAA);

            if (AtapiReadPort1(chan, IDX_IO1_i_CylinderLow) != 0xAA ||
                statusByte == IDE_STATUS_WRONG) {

                KdPrint2((PRINT_PREFIX
                            "AtapiFindIsaController: Identifier read back from Slave (%#x)\n",
                            statusByte));
                goto not_found;
            }
        }

        // Fill in the access array information only if default params are not in there.
        if (preConfig == FALSE) {

            // An adapter has been found request another call, only if we didn't get preconfigured info.
            *Again = TRUE;

            if (portBase) {
                (*ConfigInfo->AccessRanges)[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(portBase);
            } else {
                (*ConfigInfo->AccessRanges)[0].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1]);
            }

            (*ConfigInfo->AccessRanges)[0].RangeLength = ATA_IOSIZE;
            (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

            if(BaseIoAddress2) {
                if(hasPCI) {
                    (*ConfigInfo->AccessRanges)[1].RangeStart = ScsiPortConvertUlongToPhysicalAddress((ULONG_PTR)BaseIoAddress2);
                    (*ConfigInfo->AccessRanges)[1].RangeLength = ATA_ALTIOSIZE;
                    (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;
                } else {
                    // NT4 and NT3.51 on ISA-only hardware definitly fail floppy.sys load
                    // when this range is claimed by other driver.
                    // However, floppy should use only 0x3f0-3f5,3f7
                    if((ULONGIO_PTR)BaseIoAddress2 >= 0x3f0 && (ULONGIO_PTR)BaseIoAddress2 <= 0x3f7) {
                        KdPrint2((PRINT_PREFIX "!!! Possible AltStatus vs Floppy IO range interference !!!\n"));
                    }
                    KdPrint2((PRINT_PREFIX "Do not expose to OS on old ISA\n"));
                    (*ConfigInfo->AccessRanges)[1].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
                    (*ConfigInfo->AccessRanges)[1].RangeLength = 0;
                }
            }

            // Indicate the interrupt level corresponding to this IO range.
            if (irq) {
                ConfigInfo->BusInterruptLevel = irq;
            } else {
                ConfigInfo->BusInterruptLevel = InterruptLevels[*adapterCount - 1];
            }

            if (ConfigInfo->AdapterInterfaceType == MicroChannel) {
                ConfigInfo->InterruptMode = LevelSensitive;
            } else {
                ConfigInfo->InterruptMode = Latched;
            }
        }

        ConfigInfo->NumberOfBuses = 1;
        ConfigInfo->MaximumNumberOfTargets = IDE_MAX_LUN_PER_CHAN;

        // Indicate maximum transfer length is 64k.
        ConfigInfo->MaximumTransferLength = 0x10000;
        deviceExtension->MaximumDmaTransferLength = ConfigInfo->MaximumTransferLength;

        KdPrint2((PRINT_PREFIX "de %#x, Channel ???\n", deviceExtension));
        //PrintNtConsole("de %#x, Channel %#x, nchan %#x\n",deviceExtension, channel, deviceExtension->NumberChannels);

        KdPrint2((PRINT_PREFIX "chan = %#x\n", chan));
        //PrintNtConsole("chan = %#x, c=%#x\n", chan, c);
/*
        // should be already set up in AtapiSetupLunPtrs(chan, deviceExtension, 0);

        chan->DeviceExtension = deviceExtension;
        chan->lChannel        = 0;
        chan->lun[0] = &(deviceExtension->lun[0]);
        chan->lun[1] = &(deviceExtension->lun[1]);*/

        /* do extra channel-specific setups */
        AtapiReadChipConfig(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, 0);
        AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, 0);

        KdPrint2((PRINT_PREFIX
                   "AtapiFindIsaController: Found IDE at %#x\n",
                   BaseIoAddress1));

        // For Daytona, the atdisk driver gets the first shot at the
        // primary and secondary controllers.
        if (preConfig == FALSE) {

            if (*adapterCount - 1 < 2) {

                // Determine whether this driver is being initialized by the
                // system or as a crash dump driver.
                if (g_Dump) {
#ifndef UNIATA_CORE
                    deviceExtension->DriverMustPoll = TRUE;
#endif //UNIATA_CORE
                } else {
                    deviceExtension->DriverMustPoll = FALSE;
                }

            } else {
                //atapiOnly = FALSE;
            }

        } else {

            //atapiOnly = FALSE;
            deviceExtension->DriverMustPoll = FALSE;

        }// preConfig check

        // Save the Interrupe Mode for later use
        deviceExtension->InterruptMode = ConfigInfo->InterruptMode;
        deviceExtension->BusInterruptLevel = ConfigInfo->BusInterruptLevel;
        deviceExtension->BusInterruptVector = ConfigInfo->BusInterruptVector;

        KdPrint2((PRINT_PREFIX
                   "AtapiFindIsaController: look for devices\n"));
        // Search for devices on this controller.
        if (FindDevices(HwDeviceExtension,
                        0,
                        0 /* Channel */)) {

            KdPrint2((PRINT_PREFIX
                       "AtapiFindIsaController: detected\n"));
            // Claim primary or secondary ATA IO range.
            if (portBase) {
                switch (portBase) {
                case IO_WD2:
                    ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                    chan->PrimaryAddress = FALSE;
                    break;
                case IO_WD1:
                    ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                    chan->PrimaryAddress = TRUE;
                    break;
                default:
                    break;
                }
            } else {
                if (*adapterCount == 1) {
                    ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                    chan->PrimaryAddress = TRUE;
                } else if (*adapterCount == 2) {
                    ConfigInfo->AtdiskSecondaryClaimed = TRUE;
                    chan->PrimaryAddress = FALSE;
                }
            }

            if(deviceExtension->AdapterInterfaceType == Isa) {
                IsaCount++;
            } else
            if(deviceExtension->AdapterInterfaceType == MicroChannel) {
                MCACount++;
            }

            ConfigInfo->NumberOfBuses++; // add virtual channel for communication port
            KdPrint2((PRINT_PREFIX
                       "AtapiFindIsaController: return SP_RETURN_FOUND\n"));
            return(SP_RETURN_FOUND);
        } else {
not_found:
            // No controller at this base address.
            if(BaseIoAddress1) {
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       (PCHAR)BaseIoAddress1);
                BaseIoAddress1 = NULL;
            }
            if(BaseIoAddress2) {
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       (PCHAR)BaseIoAddress2);
                BaseIoAddress2 = NULL;
            }
            for(i=0; i<2; i++) {
                KdPrint2((PRINT_PREFIX
                           "AtapiFindIsaController: cleanup AccessRanges %d\n", i));
                (*ConfigInfo->AccessRanges)[i].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
                (*ConfigInfo->AccessRanges)[i].RangeLength = 0;
                (*ConfigInfo->AccessRanges)[i].RangeInMemory = FALSE;
            }
            irq = 0;
            portBase = 0;
        }
#ifndef UNIATA_CORE
    }
#else
    } while(FALSE);
#endif //UNIATA_CORE

    // The entire table has been searched and no adapters have been found.
    // There is no need to call again and the device base can now be freed.
    // Clear the adapter count for the next bus.
    *Again = FALSE;
    *(adapterCount) = 0;

    KdPrint2((PRINT_PREFIX
               "AtapiFindIsaController: return SP_RETURN_NOT_FOUND\n"));
    UniataFreeLunExt(deviceExtension);
    return(SP_RETURN_NOT_FOUND);

exit_error:
    UniataFreeLunExt(deviceExtension);
    return SP_RETURN_ERROR;

} // end AtapiFindIsaController()

/*
    Do nothing, but parse ScsiPort ArgumentString and setup global variables.
*/

ULONG
NTAPI
AtapiReadArgumentString(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
#ifndef __REACTOS__
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
#endif

    if (AtapiParseArgumentString(ArgumentString, "dump") == 1) {
        KdPrint2((PRINT_PREFIX
                   "AtapiReadArgumentString: Crash dump\n"));
        //atapiOnly = FALSE;
        g_Dump = TRUE;
    }
    return(SP_RETURN_NOT_FOUND);
} // end AtapiReadArgumentString()

ULONG
NTAPI
UniataAnybodyHome(
    IN PVOID   HwDeviceExtension,
    IN ULONG   lChannel,
    IN ULONG   deviceNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    //ULONG                ldev = GET_LDEV2(lChannel, deviceNumber, 0);
    PHW_LU_EXTENSION     LunExt = chan->lun[deviceNumber];

    SATA_SSTATUS_REG     SStatus;
    UCHAR                signatureLow;
    UCHAR                signatureHigh;

    if(LunExt->DeviceFlags & DFLAGS_HIDDEN) {
        KdPrint2((PRINT_PREFIX "  hidden\n"));
        UniataForgetDevice(LunExt);
        return ATA_AT_HOME_NOBODY;
    }
    if(UniataIsSATARangeAvailable(deviceExtension, lChannel)) {

        SStatus.Reg = UniataSataReadPort4(chan, IDX_SATA_SStatus, deviceNumber);
        KdPrint2((PRINT_PREFIX "SStatus %x\n", SStatus.Reg));
        if(SStatus.DET <= SStatus_DET_Dev_NoPhy) {
            KdPrint2((PRINT_PREFIX "  SATA DET <= SStatus_DET_Dev_NoPhy\n"));
            return ATA_AT_HOME_NOBODY;
        }
        if(SStatus.SPD < SStatus_SPD_Gen1) {
            KdPrint2((PRINT_PREFIX "  SATA SPD < SStatus_SPD_Gen1\n"));
            return ATA_AT_HOME_NOBODY;
        }
        if(SStatus.IPM == SStatus_IPM_NoDev) {
            KdPrint2((PRINT_PREFIX "  SATA IPN == SStatus_IPM_NoDev\n"));
            return ATA_AT_HOME_NOBODY;
        }
        if(!(deviceExtension->HwFlags & UNIATA_AHCI)) {
            // Select the device for legacy.
            goto legacy_select;
        }

    } else {
legacy_select:
        // Select the device.
        SelectDrive(chan, deviceNumber);
        AtapiStallExecution(5);
    }

    if((deviceExtension->HwFlags & UNIATA_AHCI) &&
       UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        KdPrint2((PRINT_PREFIX "  AHCI check\n"));
        ULONG SIG = UniataAhciReadChannelPort4(chan, IDX_AHCI_P_SIG);
        signatureLow = (UCHAR)(SIG >> 16);
        signatureHigh = (UCHAR)(SIG >> 24);
    } else {
        signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
        signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);
    }

    if (signatureLow == ATAPI_MAGIC_LSB && signatureHigh == ATAPI_MAGIC_MSB) {
        KdPrint2((PRINT_PREFIX "  ATAPI at home\n"));
        return ATA_AT_HOME_ATAPI;
    }
    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        KdPrint2((PRINT_PREFIX "  AHCI HDD at home\n"));
        return ATA_AT_HOME_HDD;
    }
    if(g_opt_VirtualMachine > VM_NONE /*== VM_BOCHS ||
       g_opt_VirtualMachine == VM_VBOX*/) {
        GetStatus(chan, signatureLow);
        if(!signatureLow) {
            KdPrint2((PRINT_PREFIX "  0-status VM - not present\n"));
            UniataForgetDevice(LunExt);
            return ATA_AT_HOME_NOBODY;
        }
    }

    AtapiStallExecution(10);

    AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0x55);
    AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0x55);
    AtapiStallExecution(5);
    signatureLow = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
    if(signatureLow != 0x55) {
        if(signatureLow == 0xff || signatureLow == 0) {
            KdPrint2((PRINT_PREFIX "  nobody home! %#x != 0x55\n", signatureLow));
            UniataForgetDevice(LunExt);
            return ATA_AT_HOME_NOBODY;
        }
        // another chance
        signatureLow = AtapiReadPort1(chan, IDX_ATAPI_IO1_o_ByteCountHigh);
        signatureLow += 2;
        AtapiWritePort1(chan, IDX_ATAPI_IO1_o_ByteCountHigh, signatureLow);
        AtapiStallExecution(5);
        signatureHigh = AtapiReadPort1(chan, IDX_ATAPI_IO1_o_ByteCountHigh);
        if(signatureLow != signatureHigh) {
            KdPrint2((PRINT_PREFIX "  nobody home! last chance failed %#x != %#x\n", signatureLow, signatureHigh));
            UniataForgetDevice(LunExt);
            return ATA_AT_HOME_NOBODY;
        }
        return ATA_AT_HOME_XXX;
    }

    AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0xAA);
    AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0xAA);
    AtapiStallExecution(5);
    signatureLow = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
    if(signatureLow != 0xAA) {
        KdPrint2((PRINT_PREFIX "  nobody home! %#x != 0xAA\n", signatureLow));
        UniataForgetDevice(LunExt);
        return ATA_AT_HOME_NOBODY;
    }

    KdPrint2((PRINT_PREFIX "  HDD at home\n"));
    return ATA_AT_HOME_HDD;
} // end UniataAnybodyHome()

ULONG
NTAPI
CheckDevice(
    IN PVOID   HwDeviceExtension,
    IN ULONG   lChannel,
    IN ULONG   deviceNumber,
    IN BOOLEAN ResetDev
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    //ULONG                ldev = GET_LDEV2(lChannel, deviceNumber, 0);
    PHW_LU_EXTENSION     LunExt;

    UCHAR                signatureLow,
                         signatureHigh;
    UCHAR                statusByte;
    ULONG                RetVal=0;
    ULONG                waitCount = g_opt_WaitBusyResetCount;
    ULONG                at_home = 0;

    KdPrint2((PRINT_PREFIX "CheckDevice: Device %#x\n",
               deviceNumber));

    if(deviceNumber >= chan->NumberLuns) {
        return 0;
    }
    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        if(!(at_home = UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber))) {
            return 0;
        }
    }
    LunExt = chan->lun[deviceNumber];

    if(ResetDev) {
        LunExt->PowerState = 0;
    }

    if((deviceExtension->HwFlags & UNIATA_SATA) &&
        !UniataIsSATARangeAvailable(deviceExtension, lChannel) &&
        deviceNumber) {
        KdPrint2((PRINT_PREFIX "  SATA w/o i/o registers, check slave presence\n"));
        SelectDrive(chan, deviceNumber & 0x01);
        statusByte = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_DriveSelect);
        KdPrint2((PRINT_PREFIX "  DriveSelect: %#x\n", statusByte));
        if((statusByte & IDE_DRIVE_MASK) != IDE_DRIVE_SELECT_2) {
            KdPrint2((PRINT_PREFIX "CheckDevice: (no dev)\n"));
            UniataForgetDevice(LunExt);
            return 0;
        }
    }

    if(ResetDev && (deviceExtension->HwFlags & UNIATA_AHCI)) {
        KdPrint2((PRINT_PREFIX "CheckDevice: reset AHCI dev\n"));
        if(UniataAhciSoftReset(HwDeviceExtension, chan->lChannel, deviceNumber) == (ULONG)(-1)) {
            KdPrint2((PRINT_PREFIX "CheckDevice: (no dev)\n"));
            UniataForgetDevice(LunExt);
            return 0;
        }
    } else
    if(ResetDev) {
        KdPrint2((PRINT_PREFIX "CheckDevice: reset dev\n"));

        // Reset device
        AtapiSoftReset(chan, deviceNumber);

        if(!(at_home = UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber))) {
            //KdPrint2((PRINT_PREFIX "CheckDevice: nobody at home 1\n"));
            return 0;
        }
        statusByte = WaitOnBusy(chan);

        if((statusByte | IDE_STATUS_BUSY) == IDE_STATUS_WRONG) {
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: bad status %x\n", statusByte));
        } else
        if(statusByte != IDE_STATUS_WRONG && (statusByte & IDE_STATUS_BUSY)) {
            // Perform hard-reset.
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: BUSY\n"));

            AtapiHardReset(chan, FALSE, 500 * 1000);
/*
            AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_RESET_CONTROLLER );
            chan->last_devsel = -1;
            AtapiStallExecution(500 * 1000);
            AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_REENABLE_CONTROLLER);
*/
            SelectDrive(chan, deviceNumber & 0x01);

            do {
                // Wait for Busy to drop.
                AtapiStallExecution(100);
                GetStatus(chan, statusByte);

            } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

            GetBaseStatus(chan, statusByte);
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: status after hard reset %x\n", statusByte));
        }

        if((statusByte | IDE_STATUS_BUSY) == IDE_STATUS_WRONG) {
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: no dev ?\n"));
            UniataForgetDevice(LunExt);
            return 0;
        } else
        if(UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        //if(deviceExtension->HwFlags & UNIATA_SATA) {
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: try enable SATA Phy\n"));
            statusByte = UniataSataPhyEnable(HwDeviceExtension, lChannel, deviceNumber);
            if(statusByte == IDE_STATUS_WRONG) {
                KdPrint2((PRINT_PREFIX "CheckDevice: status %#x (no dev)\n", statusByte));
                UniataForgetDevice(LunExt);
                return 0;
            }
        }
    }

    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        RetVal = LunExt->DeviceFlags;
        signatureLow = signatureHigh = 0; // make GCC happy
    } else {
        // Select the device.
        SelectDrive(chan, deviceNumber);

        if(!(at_home = UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber))) {
            //KdPrint2((PRINT_PREFIX "CheckDevice: nobody at home 2\n"));
            return 0;
        }

        statusByte = WaitOnBaseBusyLong(chan);

        GetBaseStatus(chan, statusByte);
        if(deviceExtension->HwFlags & UNIATA_SATA) {
            UniataSataClearErr(HwDeviceExtension, lChannel, UNIATA_SATA_IGNORE_CONNECT, deviceNumber);
        }

        KdPrint2((PRINT_PREFIX "CheckDevice: status %#x\n", statusByte));
        if(((statusByte | IDE_STATUS_BUSY) == IDE_STATUS_WRONG) ||
            (statusByte & IDE_STATUS_BUSY)) {
            KdPrint2((PRINT_PREFIX "CheckDevice: busy => return\n"));
            UniataForgetDevice(LunExt);
            return 0;
        }

        signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
        signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);
    }

    // set default costs
    LunExt->RwSwitchCost  = REORDER_COST_SWITCH_RW_HDD;
    LunExt->RwSwitchMCost = REORDER_MCOST_SWITCH_RW_HDD;
    LunExt->SeekBackMCost = REORDER_MCOST_SEEK_BACK_HDD;
    LunExt->AtapiReadyWaitDelay = 0;

    if(deviceExtension->HwFlags & UNIATA_AHCI) {
        if(RetVal & DFLAGS_DEVICE_PRESENT) {
            if(IssueIdentify(HwDeviceExtension,
                              deviceNumber,
                              lChannel,
                              (RetVal & DFLAGS_ATAPI_DEVICE) ? IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY,
                              FALSE)) {
                // OK
                KdPrint2((PRINT_PREFIX "CheckDevice: detected AHCI Device %#x\n",
                           deviceNumber));
            } else {
                //RetVal &= ~DFLAGS_ATAPI_DEVICE;
                //LunExt->DeviceFlags &= ~DFLAGS_ATAPI_DEVICE;

                UniataForgetDevice(LunExt);
                RetVal = 0;
            }
        }
    } else
    if (signatureLow == ATAPI_MAGIC_LSB && signatureHigh == ATAPI_MAGIC_MSB) {

        KdPrint2((PRINT_PREFIX "CheckDevice: ATAPI signature found\n"));
        // ATAPI signature found.
        // Issue the ATAPI identify command if this
        // is not for the crash dump utility.
try_atapi:
        if (!g_Dump) {

            // Issue ATAPI packet identify command.
            if (IssueIdentify(HwDeviceExtension,
                              deviceNumber,
                              lChannel,
                              IDE_COMMAND_ATAPI_IDENTIFY, FALSE)) {

                // Indicate ATAPI device.
                KdPrint2((PRINT_PREFIX "CheckDevice: Device %#x is ATAPI\n",
                           deviceNumber));

                RetVal = DFLAGS_DEVICE_PRESENT | DFLAGS_ATAPI_DEVICE;
                LunExt->DeviceFlags |= (DFLAGS_DEVICE_PRESENT | DFLAGS_ATAPI_DEVICE);

                // some ATAPI devices doesn't work with DPC on CMD-649
                // and probably some other controllers
                if(deviceExtension->HwFlags & UNIATA_NO_DPC_ATAPI) {
                    /* CMD 649, ROSB SWK33, ICH4 */
                    KdPrint2((PRINT_PREFIX "CheckDevice: UNIATA_NO_DPC_ATAPI\n"));
                    deviceExtension->UseDpc = FALSE;
                }

                GetStatus(chan, statusByte);
                if (statusByte & IDE_STATUS_ERROR) {
                    AtapiSoftReset(chan, deviceNumber);
                }

            } else {
forget_device:
                // Indicate no working device.
                KdPrint2((PRINT_PREFIX "CheckDevice: Device %#x not responding\n",
                           deviceNumber));

                UniataForgetDevice(LunExt);
                RetVal = 0;
            }
            GetBaseStatus(chan, statusByte);

        }

    } else {

        KdPrint2((PRINT_PREFIX "CheckDevice: IDE device check\n"));
        // Issue IDE Identify. If an Atapi device is actually present, the signature
        // will be asserted, and the drive will be recognized as such.
        if(deviceExtension->DWordIO) {
            KdPrint2((PRINT_PREFIX "  try 32bit IO\n"));
            LunExt->DeviceFlags |= DFLAGS_DWORDIO_ENABLED;
        }
        if (IssueIdentify(HwDeviceExtension,
                          deviceNumber,
                          lChannel,
                          IDE_COMMAND_IDENTIFY, FALSE)) {

            // IDE drive found.
            KdPrint2((PRINT_PREFIX "CheckDevice: Device %#x is IDE\n",
                       deviceNumber));

            // Indicate IDE - not ATAPI device.
            RetVal = DFLAGS_DEVICE_PRESENT;
            LunExt->DeviceFlags |= DFLAGS_DEVICE_PRESENT;
            LunExt->DeviceFlags &= ~DFLAGS_ATAPI_DEVICE;
        } else
        if(g_opt_VirtualMachine <= VM_NONE) {
            // This can be ATAPI on broken hardware
            GetBaseStatus(chan, statusByte);
            if(!at_home && UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber)) {
                KdPrint2((PRINT_PREFIX "CheckDevice: nobody at home post IDE\n"));
                goto forget_device;
            }
            KdPrint2((PRINT_PREFIX "CheckDevice: try ATAPI %#x, status %#x\n",
                       deviceNumber, statusByte));
            goto try_atapi;
        } else {
            KdPrint2((PRINT_PREFIX "CheckDevice: VM Device %#x not present\n",
                       deviceNumber));
        }
        GetBaseStatus(chan, statusByte);
    }
    KdPrint2((PRINT_PREFIX "CheckDevice: check status: %sfound\n", RetVal ? "" : "not "));
    return RetVal;
} // end CheckDevice()


/*++

Routine Description:

    This routine is called from AtapiFindXxxController to identify
    devices attached to an IDE controller.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    AtapiOnly - Indicates that routine should return TRUE only if
        an ATAPI device is attached to the controller.

Return Value:

    TRUE - True if devices found.

--*/
BOOLEAN
NTAPI
FindDevices(
    IN PVOID HwDeviceExtension,
    IN ULONG   Flags,
    IN ULONG   Channel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[Channel]);
    PHW_LU_EXTENSION     LunExt;
    BOOLEAN              deviceResponded = FALSE,
                         skipSetParameters = FALSE;
    ULONG                waitCount = 10000;
    //ULONG                deviceNumber;
    ULONG                i;
    UCHAR                statusByte;
    ULONG                max_ldev;
    BOOLEAN              AtapiOnly = FALSE;

    KdPrint2((PRINT_PREFIX "FindDevices:\n"));

    // Disable interrupts
    AtapiDisableInterrupts(deviceExtension, Channel);
//    AtapiWritePort1(chan, IDX_IO2_o_Control,IDE_DC_DISABLE_INTERRUPTS | IDE_DC_A_4BIT );

    // Clear expecting interrupt flag and current SRB field.
    UniataExpectChannelInterrupt(chan, FALSE);
//    chan->CurrentSrb = NULL;
//    max_ldev = (chan->ChannelCtrlFlags & CTRFLAGS_NO_SLAVE) ? 1 : IDE_MAX_LUN_PER_CHAN;
    max_ldev = (chan->ChannelCtrlFlags & CTRFLAGS_NO_SLAVE) ? 1 : deviceExtension->NumberLuns;
    KdPrint2((PRINT_PREFIX "  max_ldev %d\n", max_ldev));

    // Search for devices.
    for (i = 0; i < max_ldev; i++) {
        //AtapiDisableInterrupts(deviceExtension, Channel);
        if(Flags & UNIATA_FIND_DEV_UNHIDE) {
            chan->lun[i]->DeviceFlags &= ~DFLAGS_HIDDEN;
        }
        deviceResponded |=
            (CheckDevice(HwDeviceExtension, Channel, i, TRUE) != 0);
        //AtapiEnableInterrupts(deviceExtension, Channel);
    }

    if(chan->DeviceExtension->HwFlags & UNIATA_AHCI) {
        AtapiEnableInterrupts(deviceExtension, Channel);
        KdPrint2((PRINT_PREFIX
                   "FindDevices: returning %d (AHCI)\n",
                   deviceResponded));
        return deviceResponded;
    }

    for (i = 0; i < max_ldev; i++) {
        LunExt = chan->lun[i];

        if ((  LunExt->DeviceFlags & DFLAGS_DEVICE_PRESENT) &&
             !(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED) &&
             !(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) && deviceResponded) {

            // This hideous hack is to deal with ESDI devices that return
            // garbage geometry in the IDENTIFY data.
            // This is ONLY for the crashdump environment as
            // these are ESDI devices.
            if (LunExt->IdentifyData.SectorsPerTrack == 0x35 &&
                LunExt->IdentifyData.NumberOfHeads == 0x07) {

                KdPrint2((PRINT_PREFIX "FindDevices: Found nasty Compaq ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack = 0x34;
                LunExt->IdentifyData.NumberOfHeads = 0x0E;
            }

            if (LunExt->IdentifyData.SectorsPerTrack == 0x35 &&
                LunExt->IdentifyData.NumberOfHeads == 0x0F) {

                KdPrint2((PRINT_PREFIX "FindDevices: Found nasty Compaq ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack = 0x34;
                LunExt->IdentifyData.NumberOfHeads = 0x0F;
            }


            if (LunExt->IdentifyData.SectorsPerTrack == 0x36 &&
                LunExt->IdentifyData.NumberOfHeads == 0x07) {

                KdPrint2((PRINT_PREFIX "FindDevices: Found nasty UltraStor ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack = 0x3F;
                LunExt->IdentifyData.NumberOfHeads = 0x10;
                skipSetParameters = TRUE;
            }


            if (skipSetParameters)
                continue;

            statusByte = WaitOnBusy(chan);

            // Select the device.
            SelectDrive(chan, i & 0x01);
            GetStatus(chan, statusByte);

            if (statusByte & IDE_STATUS_ERROR) {

                // Reset the device.
                KdPrint2((PRINT_PREFIX
                            "FindDevices: Resetting controller before SetDriveParameters.\n"));

                AtapiHardReset(chan, FALSE, 500 * 1000);
/*
                AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_RESET_CONTROLLER );
                chan->last_devsel = -1;
                AtapiStallExecution(500 * 1000);
                AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_REENABLE_CONTROLLER);
*/
                SelectDrive(chan, i & 0x01);

                do {
                    // Wait for Busy to drop.
                    AtapiStallExecution(100);
                    GetStatus(chan, statusByte);

                } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);
            }

            statusByte = WaitOnBusy(chan);

            KdPrint2((PRINT_PREFIX
                        "FindDevices: Status before SetDriveParameters: (%#x) (%#x)\n",
                        statusByte,
                        AtapiReadPort1(chan, IDX_IO1_i_DriveSelect)));

            GetBaseStatus(chan, statusByte);

            // Use the IDENTIFY data to set drive parameters.
            if (!SetDriveParameters(HwDeviceExtension,i,Channel)) {

                KdPrint2((PRINT_PREFIX
                           "FindDevices: Set drive parameters for device %d failed\n",
                           i));
                // Don't use this device as writes could cause corruption.
                LunExt->DeviceFlags &= ~DFLAGS_DEVICE_PRESENT;
                UniataForgetDevice(LunExt);
                continue;

            }
            if (LunExt->DeviceFlags & DFLAGS_REMOVABLE_DRIVE) {

                // Pick up ALL IDE removable drives that conform to Yosemite V0.2...
                AtapiOnly = FALSE;
            }

            // Indicate that a device was found.
            if (!AtapiOnly) {
                deviceResponded = TRUE;
            }
        }
    }

/*    // Reset the controller. This is a feeble attempt to leave the ESDI
    // controllers in a state that ATDISK driver will recognize them.
    // The problem in ATDISK has to do with timings as it is not reproducible
    // in debug. The reset should restore the controller to its poweron state
    // and give the system enough time to settle.
    if (!deviceResponded) {

        AtapiWritePort1(chan, IDX_IO2_o_Control,IDE_DC_RESET_CONTROLLER );
        AtapiStallExecution(50 * 1000);
        AtapiWritePort1(chan, IDX_IO2_o_Control,IDE_DC_REENABLE_CONTROLLER);
    }
*/
    if(deviceResponded) {
        for (i = 0; i < max_ldev; i++) {
            LunExt = chan->lun[i];

            KdPrint2((PRINT_PREFIX
                       "FindDevices: select %d dev to clear INTR\n", i));
            SelectDrive(chan, i);
            GetBaseStatus(chan, statusByte);
            KdPrint2((PRINT_PREFIX
                       "FindDevices: statusByte=%#x\n", statusByte));
        }
        for (i = 0; i < max_ldev; i++) {
            LunExt = chan->lun[i];

            if(LunExt->DeviceFlags & DFLAGS_DEVICE_PRESENT) {
                // Make sure some device (master is preferred) is selected on exit.
                KdPrint2((PRINT_PREFIX
                           "FindDevices: select %d dev on exit\n", i));
                SelectDrive(chan, i);
                break;
            }
        }
    }

    GetBaseStatus(chan, statusByte);
    // Enable interrupts
    AtapiEnableInterrupts(deviceExtension, Channel);
//    AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_A_4BIT );
    GetBaseStatus(chan, statusByte);

    KdPrint2((PRINT_PREFIX
               "FindDevices: returning %d\n",
               deviceResponded));

    return deviceResponded;

} // end FindDevices()
