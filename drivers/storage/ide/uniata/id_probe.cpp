/*++

Copyright (c) 2002-2007 Alexandr A. Telyatnikov (Alter)

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
         Søren Schmidt, Copyright (c) 1998-2007

    Some parts of device detection code were taken from from standard ATAPI.SYS from NT4 DDK by
         Mike Glass (MGlass)
         Chuck Park (ChuckP)

    Device search/init algorithm is completly rewritten by
         Alter, Copyright (c) 2002-2004

    Fixes for Native/Compatible modes of onboard IDE controller by
         Vitaliy Vorobyov, deathsoft@yandex.ru (c) 2004

--*/

#include "stdafx.h"

PBUSMASTER_CONTROLLER_INFORMATION BMList = NULL;
ULONG         BMListLen = 0;
ULONG         IsaCount = 0;
ULONG         MCACount = 0;

BOOLEAN FirstMasterOk = FALSE;

#ifndef UNIATA_CORE

UCHAR         pciBuffer[256];
ULONG         maxPciBus = 16;

PDRIVER_OBJECT SavedDriverObject = NULL;

// local routines 

ULONG
UniataEnumBusMasterController__(
/*    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again*/
    );

VOID
AtapiDoNothing(VOID)
{
    //ULONG i = 0;
    return;
} // end AtapiDoNothing()

#endif //UNIATA_CORE

/*
    Get PCI address by ConfigInfo and RID
*/
ULONG
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
    ULONG io_start = 0;
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

    if((*ConfigInfo->AccessRanges)[rid].RangeInMemory) {
        io_start =
            // Get the system physical address for this IO range.
            ((ULONG)ScsiPortGetDeviceBase(HwDeviceExtension,
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
    } else {
        io_start = 0;
    }
    KdPrint2((PRINT_PREFIX "  AtapiGetIoRange: (2) %#x\n", io_start));
    return io_start;

} // end AtapiGetIoRange()

#ifndef UNIATA_CORE

/*
    Do nothing, but build list of supported IDE controllers
    It is a hack, ScsiPort architecture assumes, that DriverEntry
    can support only KNOWN Vendor/Device combinations.
    Thus, we build list here. Later will pretend that always knew
    about found devices.

    We shall initiate ISA device init, but callback will use
    Hal routines directly in order to scan PCI bus.
*/
VOID
UniataEnumBusMasterController(
    IN PVOID DriverObject,
    PVOID Argument2
    )
{
    UniataEnumBusMasterController__();

} // end UniataEnumBusMasterController()

BOOLEAN
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
    case PCI_DEV_SUBCLASS_SATA:
        break;
    default:
        KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
        return FALSE;
    }
    return TRUE;
} // end UniataCheckPCISubclass()

/*
    Device initializaton callback
    Builds PCI device list using Hal routines (not ScsiPort wrappers)
*/
ULONG
UniataEnumBusMasterController__(
    )
{
//    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PVOID HwDeviceExtension;
    PHW_DEVICE_EXTENSION  deviceExtension = NULL;
    PCI_SLOT_NUMBER       slotData;
    PCI_COMMON_CONFIG     pciData;
    ULONG                 busNumber;
    ULONG                 slotNumber;
    ULONG                 funcNumber;
    BOOLEAN               no_buses = FALSE;
    BOOLEAN               no_ranges = FALSE;
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

    ULONG   i;
    ULONG   pass=0;

    ULONG   RaidFlags;

    BOOLEAN found;
    BOOLEAN known;

    UCHAR IrqForCompat = 10;

    vendorStrPtr = vendorString;
    deviceStrPtr = deviceString;
    slotData.u.AsULONG = 0;

    HwDeviceExtension =
    deviceExtension = (PHW_DEVICE_EXTENSION)ExAllocatePool(NonPagedPool, sizeof(HW_DEVICE_EXTENSION));
    if(!deviceExtension) {
        return(SP_RETURN_NOT_FOUND);
    }
    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));

    for(pass=0; pass<3; pass++) {
        for(busNumber=0 ;busNumber<maxPciBus && !no_buses; busNumber++) {
            for(slotNumber=0; slotNumber<PCI_MAX_DEVICES  && !no_buses; slotNumber++) {
            for(funcNumber=0; funcNumber<PCI_MAX_FUNCTION && !no_buses; funcNumber++) {

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
                    no_buses = TRUE;
                    maxPciBus = busNumber;
                    break;
                }
                // no device in this slot
                if(busDataRead == 2)
                    continue;

                if(busDataRead < (ULONG)PCI_COMMON_HDR_LENGTH)
                    continue;

                VendorID  = pciData.VendorID;
                DeviceID  = pciData.DeviceID;
                BaseClass = pciData.BaseClass;
                SubClass  = pciData.SubClass;
                dev_id = VendorID | (DeviceID << 16);
                //KdPrint2((PRINT_PREFIX "DevId = %8.8X Class = %4.4X/%4.4X\n", dev_id, BaseClass, SubClass ));

                if(BaseClass != PCI_DEV_CLASS_STORAGE)
                    continue;

                KdPrint2((PRINT_PREFIX "Storage Class\n"));
                KdPrint2((PRINT_PREFIX "DevId = %8.8X Class = %4.4X/%4.4X\n", dev_id, BaseClass, SubClass ));
                // look for known chipsets
                found = FALSE;
                known = FALSE;

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
                i = Ata_is_dev_listed((PBUSMASTER_CONTROLLER_INFORMATION)&BusMasterAdapters[0], VendorID, DeviceID, 0, NUM_BUSMASTER_ADAPTERS);

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
                    if(Ata_is_supported_dev(&pciData))
                        found = TRUE;
                    break;
                }

                if(found) {

                    KdPrint2((PRINT_PREFIX "found, pass %d\n", pass));

                    KdPrint2((PRINT_PREFIX "InterruptPin = %#x\n", pciData.u.type0.InterruptPin));
                    KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData.u.type0.InterruptLine));

                    if(!pass && known) {
                        // Enable Busmastering, IO-space and Mem-space
                        KdPrint2((PRINT_PREFIX "Enabling Mem/Io spaces and busmastering...\n"));
                        KdPrint2((PRINT_PREFIX "Initial pciData.Command = %#x\n", pciData.Command));
                        for(i=0; i<3; i++) {
                            switch(i) {
                            case 0:
                                KdPrint2((PRINT_PREFIX "PCI_ENABLE_IO_SPACE\n"));
                                pciData.Command |= PCI_ENABLE_IO_SPACE;
                                break;
                            case 1:
                                KdPrint2((PRINT_PREFIX "PCI_ENABLE_MEMORY_SPACE\n"));
                                pciData.Command |= PCI_ENABLE_MEMORY_SPACE;
                                break;
                            case 2:
                                KdPrint2((PRINT_PREFIX "PCI_ENABLE_BUS_MASTER\n"));
                                pciData.Command |= PCI_ENABLE_BUS_MASTER;
                                break;
                            }
                            HalSetBusDataByOffset(  PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                    &(pciData.Command),
                                                    offsetof(PCI_COMMON_CONFIG, Command),
                                                    sizeof(pciData.Command));
                            KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData.u.type0.InterruptLine));

                            // reread config space
                            busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotData.u.AsULONG,
                                                        &pciData, PCI_COMMON_HDR_LENGTH);
                            KdPrint2((PRINT_PREFIX "New pciData.Command = %#x\n", pciData.Command));
                        }
                        KdPrint2((PRINT_PREFIX "Final pciData.Command = %#x\n", pciData.Command));
                    }
                    // validate Mem/Io ranges
                    no_ranges = TRUE;
                    for(i=0; i<PCI_TYPE0_ADDRESSES; i++) {
                        if(pciData.u.type0.BaseAddresses[i] & ~0x7) {
                            no_ranges = FALSE;
                            //break;
                            KdPrint2((PRINT_PREFIX "Range %d = %#x\n", i, pciData.u.type0.BaseAddresses[i]));
                        }
                    }
                    if(no_ranges) {
                        KdPrint2((PRINT_PREFIX "No PCI Mem/Io ranges found on device, skip it\n"));
                        continue;
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
                            if(IsMasterDev(&pciData))
                                continue;
                        }

/*                        if(known) {
                            RtlCopyMemory(newBMListPtr, (PVOID)&(BusMasterAdapters[i]), sizeof(BUSMASTER_CONTROLLER_INFORMATION));
                        } else {*/
                        sprintf((PCHAR)vendorStrPtr, "%4.4x", (UINT)VendorID);
                        sprintf((PCHAR)deviceStrPtr, "%4.4x", (UINT)DeviceID);

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

                        newBMListPtr->Known = known;

                        KdPrint2((PRINT_PREFIX "Add to BMList\n"));
                    } else {
                        KdPrint2((PRINT_PREFIX "count: BMListLen++\n"));
                    }

                    BMListLen++;
                }
            }
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
    KdPrint2((PRINT_PREFIX "  BMListLen=%x\n", BMListLen));
    if(deviceExtension) {
        ExFreePool(deviceExtension);
    }
    return(SP_RETURN_NOT_FOUND);
} // end UniataEnumBusMasterController__()


/*
    Wrapper for read PCI config space
*/
ULONG
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
AtapiFindListedDev(
    PBUSMASTER_CONTROLLER_INFORMATION BusMasterAdapters,
    ULONG     lim,
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

        i = Ata_is_dev_listed(BusMasterAdapters, pciData.VendorID, pciData.DeviceID, pciData.RevisionID, lim);
        if(i != BMLIST_TERMINATOR) {
            if(_slotData)
                *_slotData = slotData;
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
        (PVOID)0x00000000,
        BusInformation,
        ArgumentString,
        ConfigInfo,
        Again
        );
} // end UniataFindCompatBusMasterController1()

ULONG
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
        (PVOID)0x80000000,
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

    UCHAR                 vendorString[5];
    UCHAR                 deviceString[5];
    PUCHAR                vendorStrPtr;
    PUCHAR                deviceStrPtr;

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
    BOOLEAN skip_find_dev = FALSE;
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

    if(ForceSimplex) {
        KdPrint2((PRINT_PREFIX "ForceSimplex (1)\n"));
        simplexOnly = TRUE;
    }

    if(ConfigInfo->AdapterInterfaceType == Isa) {
        KdPrint2((PRINT_PREFIX "AdapterInterfaceType: Isa\n"));
    }
    if(InDriverEntry) {
        i = (ULONG)Context;
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
            i = (ULONG)Context;
            if(FirstMasterOk) {
                channel = 1;
            }
            i &= ~0x80000000;
            if(i >= BMListLen) {
                KdPrint2((PRINT_PREFIX " => SP_RETURN_NOT_FOUND\n"));
                return(SP_RETURN_NOT_FOUND);
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

    vendorStrPtr = vendorString;
    deviceStrPtr = deviceString;

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
        return SP_RETURN_ERROR;
    }

    KdPrint2((PRINT_PREFIX "busDataRead\n"));
    if (pciData.VendorID == PCI_INVALID_VENDORID) {
        KdPrint2((PRINT_PREFIX "PCI_INVALID_VENDORID\n"));
        return SP_RETURN_ERROR;
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
    deviceExtension->NumberChannels = 2; // default
    deviceExtension->DevIndex = i;

    _snprintf(deviceExtension->Signature, sizeof(deviceExtension->Signature),
              "UATA%8.8x/%1.1x@%8.8x", dev_id, channel, slotNumber);

    if(BaseClass != PCI_DEV_CLASS_STORAGE) {
        KdPrint2((PRINT_PREFIX "BaseClass != PCI_DEV_CLASS_STORAGE => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    KdPrint2((PRINT_PREFIX "Storage Class\n"));

    // look for known chipsets
    if(VendorID != BMList[i].nVendorId ||
       DeviceID != BMList[i].nDeviceId) {
        KdPrint2((PRINT_PREFIX "device not suitable\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    found = UniataCheckPCISubclass(BMList[i].Known, BMList[i].RaidFlags, SubClass);
    if(!found) {
        KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    ConfigInfo->AlignmentMask = 0x00000003;

    found = UniataChipDetect(HwDeviceExtension, &pciData, i, ConfigInfo, &simplexOnly);
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
        if(!Ata_is_supported_dev(&pciData)) {
            KdPrint2((PRINT_PREFIX "!Ata_is_supported_dev => found = FALSE\n"));
            found = FALSE;
        } else {
            KdPrint2((PRINT_PREFIX "Ata_is_supported_dev\n"));
            found = TRUE;
        }
        deviceExtension->UnknownDev = TRUE;
        break;
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (1)", deviceExtension->HwFlags));
    if(!found) {
        KdPrint2((PRINT_PREFIX "!found => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (2)", deviceExtension->HwFlags));
    KdPrint2((PRINT_PREFIX "found suitable device\n"));

    /***********************************************************/
    /***********************************************************/
    /***********************************************************/

    deviceExtension->UseDpc = TRUE;
    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (3)", deviceExtension->HwFlags));
    if(deviceExtension->HwFlags & UNIATA_NO_DPC) {
        /* CMD 649, ROSB SWK33, ICH4 */
        KdPrint2((PRINT_PREFIX "UniataFindBusMasterController: UNIATA_NO_DPC (0)\n"));
        deviceExtension->UseDpc = FALSE;
    }

    MasterDev = IsMasterDev(&pciData);

    if(MasterDev) {
        KdPrint2((PRINT_PREFIX "MasterDev (1)\n"));
        deviceExtension->MasterDev = TRUE;
        deviceExtension->NumberChannels = 1;
    }

    if(MasterDev) {
        if((WinVer_Id() <= WinVer_NT) && AltInit && FirstMasterOk) {
            // this is the 2nd attempt to init this controller by OUR driver
            KdPrint2((PRINT_PREFIX "Skip primary/secondary claiming checks\n"));
        } else {
            if((channel==0) && ConfigInfo->AtdiskPrimaryClaimed) {
                KdPrint2((PRINT_PREFIX "Error: Primary channel already claimed by another driver\n"));
                return(SP_RETURN_NOT_FOUND);
            }
            if((channel==1) && ConfigInfo->AtdiskSecondaryClaimed) {
                KdPrint2((PRINT_PREFIX "Error: Secondary channel already claimed by another driver\n"));
                return(SP_RETURN_NOT_FOUND);
            }
        }
    }
    if(deviceExtension->AltRegMap) {
        KdPrint2((PRINT_PREFIX "  Non-standard registers layout\n"));
        if(deviceExtension->HwFlags & UNIATA_SATA) {
            KdPrint2((PRINT_PREFIX "UNIATA_SATA -> IsBusMaster == TRUE\n"));
            deviceExtension->BusMaster = TRUE;
        }
    } else {
        deviceExtension->BusMaster = FALSE;

        if(WinVer_WDM_Model && !deviceExtension->UnknownDev) {
            ULONG i;
            // Enable Busmastering, IO-space and Mem-space
            KdPrint2((PRINT_PREFIX "Enabling Mem/Io spaces and busmastering...\n"));
            KdPrint2((PRINT_PREFIX "Initial pciData.Command = %#x\n", pciData.Command));
            for(i=0; i<3; i++) {
                switch(i) {
                case 0:
                    KdPrint2((PRINT_PREFIX "PCI_ENABLE_IO_SPACE\n"));
                    pciData.Command |= PCI_ENABLE_IO_SPACE;
                    break;
                case 1:
                    KdPrint2((PRINT_PREFIX "PCI_ENABLE_MEMORY_SPACE\n"));
                    pciData.Command |= PCI_ENABLE_MEMORY_SPACE;
                    break;
                case 2:
                    KdPrint2((PRINT_PREFIX "PCI_ENABLE_BUS_MASTER\n"));
                    pciData.Command |= PCI_ENABLE_BUS_MASTER;
                    break;
                }
                HalSetBusDataByOffset(  PCIConfiguration, SystemIoBusNumber, slotData.u.AsULONG, 
                                        &(pciData.Command),
                                        offsetof(PCI_COMMON_CONFIG, Command),
                                        sizeof(pciData.Command));
                KdPrint2((PRINT_PREFIX "InterruptLine = %#x\n", pciData.u.type0.InterruptLine));

                // reread config space
                busDataRead = HalGetBusData(PCIConfiguration, SystemIoBusNumber, slotData.u.AsULONG, 
                                            &pciData, PCI_COMMON_HDR_LENGTH);
                KdPrint2((PRINT_PREFIX "New pciData.Command = %#x\n", pciData.Command));
            }
            KdPrint2((PRINT_PREFIX "Final pciData.Command = %#x\n", pciData.Command));
        }
        // validate Mem/Io ranges
        //no_ranges = TRUE;
        {
            ULONG i;
            for(i=0; i<PCI_TYPE0_ADDRESSES; i++) {
                if(pciData.u.type0.BaseAddresses[i] & ~0x7) {
                    //no_ranges = FALSE;
                    //break;
                    KdPrint2((PRINT_PREFIX "Range %d = %#x\n", i, pciData.u.type0.BaseAddresses[i]));
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
                deviceExtension->BusMaster = TRUE;
                deviceExtension->BaseIoAddressBM_0.Addr  = (ULONG)BaseIoAddressBM_0;
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
            KdPrint2((PRINT_PREFIX "  statusByte = %x\n", statusByte));
            if(statusByte == 0xff) {
                KdPrint2((PRINT_PREFIX "  invalid port ?\n"));
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

    //TODO: fix hang with UseDpn=TRUE in Simplex mode
    //deviceExtension->UseDpc = TRUE;
    if(simplexOnly) {
        KdPrint2((PRINT_PREFIX "simplexOnly => UseDpc = FALSE\n"));
        deviceExtension->UseDpc = FALSE;
    }

    if(simplexOnly || !MasterDev /*|| (WinVer_Id() > WinVer_NT)*/) {
        if(deviceExtension->NumberChannels < 2) {
            KdPrint2((PRINT_PREFIX "set NumberChannels = 2\n"));
            deviceExtension->NumberChannels = 2;
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
    ConfigInfo->MaximumNumberOfTargets = (UCHAR)(/*deviceExtension->NumberChannels **/ 2);

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
        _ConfigInfo->nt4.DeviceExtensionSize         = sizeof(HW_DEVICE_EXTENSION);
        _ConfigInfo->nt4.SpecificLuExtensionSize     = sizeof(HW_LU_EXTENSION);
        _ConfigInfo->nt4.SrbExtensionSize            = sizeof(ATA_REQ);
    }
    if((WinVer_Id() > WinVer_2k) ||
       (ConfigInfo->Length >= sizeof(_ConfigInfo->comm) + sizeof(_ConfigInfo->nt4) + sizeof(_ConfigInfo->w2k))) {
        _ConfigInfo->w2k.Dma64BitAddresses           = 0;
        _ConfigInfo->w2k.ResetTargetSupported        = TRUE;
        _ConfigInfo->w2k.MaximumNumberOfLogicalUnits = 2;
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

    found = FALSE;

    if(deviceExtension->BusMaster) {

        KdPrint2((PRINT_PREFIX "Reconstruct ConfigInfo\n"));
        ConfigInfo->MapBuffers = TRUE;
#ifdef USE_OWN_DMA
        ConfigInfo->NeedPhysicalAddresses = FALSE;
#else
        ConfigInfo->NeedPhysicalAddresses = TRUE;
#endif //USE_OWN_DMA
        if(!MasterDev) {
            KdPrint2((PRINT_PREFIX "set Dma32BitAddresses\n"));
            ConfigInfo->Dma32BitAddresses = TRUE;
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
        ConfigInfo->CachesData    = TRUE;
        ConfigInfo->ScatterGather = TRUE;
    }

    // Note: now we can support only 4 channels !!!
    // in order to add support for multichannel controllers we must rewrite
    // io-range claiming algorithm

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

                // do not claim 2nd BM io-range for Secondary channel of
                // Compatible-mode controllers
                if(/*(WinVer_Id() <= WinVer_NT) &&*/ !c && channel == 1) {
                    KdPrint2((PRINT_PREFIX "cheat ScsiPort for 2nd channel, BM io-range\n"));
                    (*ConfigInfo->AccessRanges)[4].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
                    (*ConfigInfo->AccessRanges)[4].RangeLength = 0;
                }
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
                                            PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                            SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
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
                                            PCIBus /*ConfigInfo->AdapterInterfaceType*/,
                                            SystemIoBusNumber /*ConfigInfo->SystemIoBusNumber*/,
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

        KdPrint2((PRINT_PREFIX "IDX_IO1 %x->%x(%s)\n",
          IDX_IO1,
          chan->RegTranslation[IDX_IO1].Addr,
          chan->RegTranslation[IDX_IO1].MemIo ? "mem" : "io"));

        KdPrint2((PRINT_PREFIX "IDX_IO2 %x->%x(%s)\n",
          IDX_IO2,
          chan->RegTranslation[IDX_IO2].Addr,
          chan->RegTranslation[IDX_IO2].MemIo ? "mem" : "io"));

        KdPrint2((PRINT_PREFIX "IDX_BM_IO %x->%x(%s)\n",
          IDX_BM_IO,
          chan->RegTranslation[IDX_BM_IO].Addr,
          chan->RegTranslation[IDX_BM_IO].MemIo ? "mem" : "io"));

        KdPrint2((PRINT_PREFIX "IDX_SATA_IO %x->%x(%s)\n",
          IDX_SATA_IO,
          chan->RegTranslation[IDX_SATA_IO].Addr,
          chan->RegTranslation[IDX_SATA_IO].MemIo ? "mem" : "io"));

        UniataDumpATARegs(chan);

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
            if(!(deviceExtension->HwFlags & UNIATA_NO_SLAVE)) {
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
                        FALSE,
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
#endif //UNIATA_INIT_ON_PROBE
        found = TRUE;

        chan->PrimaryAddress = FALSE;
        // Claim primary or secondary ATA IO range.
        if (MasterDev) {
            KdPrint2((PRINT_PREFIX "claim Compatible controller\n"));
            if (channel == 0) {
                KdPrint2((PRINT_PREFIX "claim Primary\n"));
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
                chan->PrimaryAddress = TRUE;

                FirstMasterOk = TRUE;

            } else
            if (channel == 1) {
                KdPrint2((PRINT_PREFIX "claim Secondary\n"));
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;

                FirstMasterOk = TRUE;
            }
        }

        AtapiDmaAlloc(HwDeviceExtension, ConfigInfo, c);
#else //UNIATA_CORE
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

        KdPrint2((PRINT_PREFIX "return SP_RETURN_NOT_FOUND\n"));
        return SP_RETURN_NOT_FOUND;
    } else {

        KdPrint2((PRINT_PREFIX "exit: init spinlock\n"));
        //KeInitializeSpinLock(&(deviceExtension->DpcSpinLock));
        deviceExtension->ActiveDpcChan =
        deviceExtension->FirstDpcChan = -1;

        BMList[i].Isr2Enable = FALSE;

        KdPrint2((PRINT_PREFIX "MasterDev=%#x, NumberChannels=%#x, Isr2DevObj=%#x\n",
            MasterDev, deviceExtension->NumberChannels, BMList[i].Isr2DevObj));

        // ConnectIntr2 should be moved to HwInitialize
        status = UniataConnectIntr2(HwDeviceExtension);

        KdPrint2((PRINT_PREFIX "MasterDev=%#x, NumberChannels=%#x, Isr2DevObj=%#x\n",
            MasterDev, deviceExtension->NumberChannels, BMList[i].Isr2DevObj));

        if(WinVer_WDM_Model && MasterDev) {
            KdPrint2((PRINT_PREFIX "do not tell system, that we know about this:\n"));
            if(BaseIoAddressBM_0) {
                ScsiPortFreeDeviceBase(HwDeviceExtension,
                                       BaseIoAddressBM_0);
            }
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

} // end UniataFindBusMasterController()

#ifndef UNIATA_CORE

/*
   This is for claiming PCI Busmaster in compatible mode under WDM OSes
*/
ULONG
UniataFindFakeBusMasterController(
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
    // this buffer must be global for UNIATA_CORE build
    PCI_COMMON_CONFIG     pciData;

    ULONG                 slotNumber;
    ULONG                 busDataRead;
    ULONG                 SystemIoBusNumber;

    UCHAR                 vendorString[5];
    UCHAR                 deviceString[5];
    PUCHAR                vendorStrPtr;
    PUCHAR                deviceStrPtr;

    UCHAR   BaseClass;
    UCHAR   SubClass;
    ULONG   VendorID;
    ULONG   DeviceID;
    ULONG   RevID;
    ULONG   dev_id;
    PCI_SLOT_NUMBER       slotData;

    ULONG   i;
//    PUCHAR  ioSpace;
//    UCHAR   statusByte;

//    UCHAR   tmp8;
//    ULONG   irq;

    BOOLEAN found = FALSE;
    BOOLEAN MasterDev;
    BOOLEAN simplexOnly = FALSE;
    BOOLEAN skip_find_dev = FALSE;
    BOOLEAN AltInit = FALSE;

    PIDE_BUSMASTER_REGISTERS BaseIoAddressBM_0 = NULL;

//    NTSTATUS status;
    PPORT_CONFIGURATION_INFORMATION_COMMON _ConfigInfo =
        (PPORT_CONFIGURATION_INFORMATION_COMMON)ConfigInfo;

    *Again = FALSE;

    if(InDriverEntry) {
        i = (ULONG)Context;
    } else {
        for(i=0; i<BMListLen; i++) {
            if(BMList[i].slotNumber == ConfigInfo->SlotNumber &&
               BMList[i].busNumber  == ConfigInfo->SystemIoBusNumber) {
                break;
            }
        }
        if(i >= BMListLen) {
            KdPrint2((PRINT_PREFIX "unexpected device arrival => SP_RETURN_NOT_FOUND\n"));
            return(SP_RETURN_NOT_FOUND);
        }
    }

    KdPrint2((PRINT_PREFIX "UniataFindFakeBusMasterController (WDM)\n"));

    if (!deviceExtension) {
        KdPrint2((PRINT_PREFIX "!deviceExtension => SP_RETURN_ERROR\n"));
        return SP_RETURN_ERROR;
    }

    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));

    vendorStrPtr = vendorString;
    deviceStrPtr = deviceString;

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

    if (busDataRead < PCI_COMMON_HDR_LENGTH) {
        KdPrint2((PRINT_PREFIX "busDataRead < PCI_COMMON_HDR_LENGTH => SP_RETURN_ERROR\n"));
        return SP_RETURN_ERROR;
    }

    KdPrint2((PRINT_PREFIX "busDataRead\n"));
    if (pciData.VendorID == PCI_INVALID_VENDORID) {
        KdPrint2((PRINT_PREFIX "PCI_INVALID_VENDORID\n"));
        return SP_RETURN_ERROR;
    }

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
    deviceExtension->NumberChannels = 2; // default
    deviceExtension->DevIndex = i;

    _snprintf(deviceExtension->Signature, sizeof(deviceExtension->Signature),
              "UATA%8.8x/%1.1x@%8.8x", dev_id, 0xff, slotNumber);

    if(BaseClass != PCI_DEV_CLASS_STORAGE) {
        KdPrint2((PRINT_PREFIX "BaseClass != PCI_DEV_CLASS_STORAGE => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    KdPrint2((PRINT_PREFIX "Storage Class\n"));

    // look for known chipsets
    if(VendorID != BMList[i].nVendorId ||
       DeviceID != BMList[i].nDeviceId) {
        KdPrint2((PRINT_PREFIX "device not suitable\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    if((BMList[i].RaidFlags & UNIATA_RAID_CONTROLLER) &&
        SkipRaids) {
        KdPrint2((PRINT_PREFIX "RAID support disabled\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    switch(SubClass) {
    case PCI_DEV_SUBCLASS_IDE:
    case PCI_DEV_SUBCLASS_RAID:
    case PCI_DEV_SUBCLASS_ATA:
    case PCI_DEV_SUBCLASS_SATA:
        // ok
        break;
    default:
        KdPrint2((PRINT_PREFIX "Subclass not supported\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    ConfigInfo->AlignmentMask = 0x00000003;

    found = UniataChipDetect(HwDeviceExtension, &pciData, i, ConfigInfo, &simplexOnly);
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
        if(!Ata_is_supported_dev(&pciData)) {
            KdPrint2((PRINT_PREFIX "!Ata_is_supported_dev => found = FALSE\n"));
            found = FALSE;
        } else {
            KdPrint2((PRINT_PREFIX "Ata_is_supported_dev\n"));
            found = TRUE;
        }
        deviceExtension->UnknownDev = TRUE;
        break;
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (1)", deviceExtension->HwFlags));
    if(!found) {
        KdPrint2((PRINT_PREFIX "!found => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (2)", deviceExtension->HwFlags));
    KdPrint2((PRINT_PREFIX "found suitable device\n"));

    /***********************************************************/
    /***********************************************************/
    /***********************************************************/

    deviceExtension->UseDpc = TRUE;
    KdPrint2((PRINT_PREFIX "HwFlags = %x\n (3)", deviceExtension->HwFlags));
    if(deviceExtension->HwFlags & UNIATA_NO_DPC) {
        /* CMD 649, ROSB SWK33, ICH4 */
        KdPrint2((PRINT_PREFIX "UniataFindBusMasterController: UNIATA_NO_DPC (0)\n"));
        deviceExtension->UseDpc = FALSE;
    }

    MasterDev = IsMasterDev(&pciData);

    if(MasterDev) {
        KdPrint2((PRINT_PREFIX "MasterDev\n"));
        deviceExtension->MasterDev = TRUE;
        deviceExtension->NumberChannels = 1;
    } else {
        KdPrint2((PRINT_PREFIX "!MasterDev => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    if(deviceExtension->AltRegMap) {
        KdPrint2((PRINT_PREFIX "  Non-standard registers layout => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }
    if(IsBusMaster(&pciData)) {
        KdPrint2((PRINT_PREFIX "  !BusMaster => SP_RETURN_NOT_FOUND\n"));
        return(SP_RETURN_NOT_FOUND);
    }

    KdPrint2((PRINT_PREFIX "IsBusMaster == TRUE\n"));
    BaseIoAddressBM_0 = (PIDE_BUSMASTER_REGISTERS)
        (AtapiGetIoRange(HwDeviceExtension, ConfigInfo, &pciData, SystemIoBusNumber,
                        4, 0, 0x10/*ATA_BMIOSIZE*/)/* - bm_offset*/); //range id
    if(BaseIoAddressBM_0) {
        UniataInitMapBM(deviceExtension,
                        BaseIoAddressBM_0,
                        (*ConfigInfo->AccessRanges)[4].RangeInMemory ? TRUE : FALSE);
        deviceExtension->BusMaster = TRUE;
        deviceExtension->BaseIoAddressBM_0.Addr  = (ULONG)BaseIoAddressBM_0;
        if((*ConfigInfo->AccessRanges)[4].RangeInMemory) {
            deviceExtension->BaseIoAddressBM_0.MemIo = TRUE;
        }
    }
    KdPrint2((PRINT_PREFIX "  BusMasterAddress (base): %#x\n", BaseIoAddressBM_0));

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

    // Indicate number of buses.
    ConfigInfo->NumberOfBuses = 0;
    if(!ConfigInfo->InitiatorBusId[0]) {
        ConfigInfo->InitiatorBusId[0] = (CHAR)(IoGetConfigurationInformation()->ScsiPortCount);
        KdPrint2((PRINT_PREFIX "set ConfigInfo->InitiatorBusId[0] = %#x\n", ConfigInfo->InitiatorBusId[0]));
    }
    // Indicate four devices can be attached to the adapter
    ConfigInfo->MaximumNumberOfTargets = 0;

    ConfigInfo->MultipleRequestPerLu = FALSE;
    ConfigInfo->AutoRequestSense     = FALSE;
    ConfigInfo->TaggedQueuing        = FALSE;

    if((WinVer_Id() >= WinVer_NT) ||
       (ConfigInfo->Length >= sizeof(_ConfigInfo->comm) + sizeof(_ConfigInfo->nt4))) {
        _ConfigInfo->nt4.DeviceExtensionSize         = sizeof(HW_DEVICE_EXTENSION);
        _ConfigInfo->nt4.SpecificLuExtensionSize     = sizeof(HW_LU_EXTENSION);    
        _ConfigInfo->nt4.SrbExtensionSize            = sizeof(ATA_REQ);            
    }
    if((WinVer_Id() > WinVer_2k) ||
       (ConfigInfo->Length >= sizeof(_ConfigInfo->comm) + sizeof(_ConfigInfo->nt4) + sizeof(_ConfigInfo->w2k))) {
        _ConfigInfo->w2k.Dma64BitAddresses           = 0;
        _ConfigInfo->w2k.ResetTargetSupported        = FALSE;
        _ConfigInfo->w2k.MaximumNumberOfLogicalUnits = 0;
    }

    // Save the Interrupe Mode for later use
    deviceExtension->InterruptMode      = ConfigInfo->InterruptMode;
    deviceExtension->BusInterruptLevel  = ConfigInfo->BusInterruptLevel;
    deviceExtension->BusInterruptVector = ConfigInfo->BusInterruptVector;
    deviceExtension->Channel            = 0;
    deviceExtension->DevIndex           = i;
    deviceExtension->OrigAdapterInterfaceType
                                        = ConfigInfo->AdapterInterfaceType;
    deviceExtension->AlignmentMask      = ConfigInfo->AlignmentMask;
    deviceExtension->AdapterInterfaceType = PCIBus;

    KdPrint2((PRINT_PREFIX "Reconstruct ConfigInfo\n"));
    ConfigInfo->MapBuffers = TRUE;
#ifdef USE_OWN_DMA
    ConfigInfo->NeedPhysicalAddresses = FALSE;
#else
    ConfigInfo->NeedPhysicalAddresses = TRUE;
#endif //USE_OWN_DMA

exit_findbm:

    KdPrint2((PRINT_PREFIX "return SP_RETURN_FOUND\n"));
    //PrintNtConsole("return SP_RETURN_FOUND, de %#x, c0.lun0 %#x\n", deviceExtension, deviceExtension->chan[0].lun[0]);

    return SP_RETURN_FOUND;

} // end UniataFindFakeBusMasterController()


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
UniataConnectIntr2(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION  deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG i = deviceExtension->DevIndex;
    NTSTATUS status;
    PISR2_DEVICE_EXTENSION Isr2DevExt;
    WCHAR devname_str[32];
    UNICODE_STRING devname;

    KdPrint2((PRINT_PREFIX "Init ISR:\n"));

    if(BMList[i].Isr2DevObj) {
        KdPrint2((PRINT_PREFIX "Already initialized %#x\n", BMList[i].Isr2DevObj));
        return STATUS_SUCCESS;
    }

    if(!deviceExtension->MasterDev && (deviceExtension->NumberChannels > 1) &&   // do not touch MasterDev
       !deviceExtension->simplexOnly && /*                        // this is unnecessary on simplex controllers
       !BMList[i].Isr2DevObj*/                                    // handle re-init under w2k+
       /*!ForceSimplex*/
       TRUE) {
        // Ok, continue...
        KdPrint2((PRINT_PREFIX "Multichannel native mode, go...\n"));
    } else {
        KdPrint2((PRINT_PREFIX "Unnecessary\n"));
        return STATUS_SUCCESS;
    }

    KdPrint2((PRINT_PREFIX "Create DO\n"));

    devname.Length =
        _snwprintf(devname_str, sizeof(devname_str)/sizeof(WCHAR),
              L"\\Device\\uniata%d_2ch", i);
    devname.Length *= sizeof(WCHAR);
    devname.MaximumLength = devname.Length;
    devname.Buffer = devname_str;

    KdPrint2((PRINT_PREFIX "DO name:  len(%d, %d), %S\n", devname.Length, devname.MaximumLength, devname.Buffer));

    status = IoCreateDevice(SavedDriverObject, sizeof(ISR2_DEVICE_EXTENSION),
                            /*NULL*/ &devname, FILE_DEVICE_UNKNOWN,
                            0, FALSE, &(BMList[i].Isr2DevObj));

    if(!NT_SUCCESS(status)) {
        KdPrint2((PRINT_PREFIX "IoCreateDevice failed %#x\n"));
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
AtapiFindController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[0]);
    PULONG               adapterCount    = (PULONG)Context;
    PUCHAR               ioSpace;
    ULONG                i;
    ULONG                irq=0;
    ULONG                portBase;
    ULONG                retryCount;
    BOOLEAN              atapiOnly;
    UCHAR                statusByte;
    BOOLEAN              preConfig = FALSE;
    //
    PIDE_REGISTERS_1 BaseIoAddress1;
    PIDE_REGISTERS_2 BaseIoAddress2;

    // The following table specifies the ports to be checked when searching for
    // an IDE controller.  A zero entry terminates the search.
    static CONST ULONG AdapterAddresses[5] = {IO_WD1, IO_WD2, IO_WD1-8, IO_WD2-8, 0};
//    CONST UCHAR Channels[5] = {0, 1, 0, 1, 0};

    // The following table specifies interrupt levels corresponding to the
    // port addresses in the previous table.
    static CONST ULONG InterruptLevels[5] = {14, 15, 11, 10, 0};

    KdPrint2((PRINT_PREFIX "AtapiFindController:\n"));

    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }
    RtlZeroMemory(deviceExtension, sizeof(HW_DEVICE_EXTENSION));

    KdPrint2((PRINT_PREFIX "  assume max PIO4\n"));
    deviceExtension->MaxTransferMode = ATA_PIO4;
    deviceExtension->NumberChannels = 1;

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


    // Scan though the adapter address looking for adapters.
    if (ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart) != 0) {
        ioSpace =  (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                         ConfigInfo->AdapterInterfaceType,
                                         ConfigInfo->SystemIoBusNumber,
                                         (*ConfigInfo->AccessRanges)[0].RangeStart,
                                         (*ConfigInfo->AccessRanges)[0].RangeLength,
                                         (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));
        *Again = FALSE;
        // Since we have pre-configured information we only need to go through this loop once
        preConfig = TRUE;
        portBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
        KdPrint2((PRINT_PREFIX "  preconfig, portBase=%x\n", portBase));
    }

#ifndef UNIATA_CORE
    while (AdapterAddresses[*adapterCount] != 0) {
#else
    do {
#endif //UNIATA_CORE

        retryCount = 4;
        deviceExtension->DevIndex = (*adapterCount);

        portBase = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"PortBase", portBase);
        irq      = AtapiRegCheckDevValue(deviceExtension, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"Irq", irq);
        
        for (i = 0; i < 4; i++) {
            // Zero device fields to ensure that if earlier devices were found,
            // but not claimed, the fields are cleared.
            deviceExtension->lun[i].DeviceFlags &= ~(DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT | DFLAGS_TAPE_DEVICE);
        }
        // Get the system physical address for this IO range.

        // Check if configInfo has the default information
        // if not, we go and find ourselves
        if (preConfig == FALSE) {

            if (portBase) {
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress(portBase),
                                                8,
                                                TRUE);
            } else {
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]),
                                                8,
                                                TRUE);
            }

        } 
        BaseIoAddress1 = (PIDE_REGISTERS_1)ioSpace;

        // Update the adapter count.
        (*adapterCount)++;

        // Check if ioSpace accessible.
        if (!ioSpace) {
            KdPrint2((PRINT_PREFIX "AtapiFindController: !ioSpace\n"));
            continue;
        }
        // check if Primary/Secondary Master IDE claimed
        if((ioSpace == (PUCHAR)IO_WD1) &&
           (ConfigInfo->AtdiskPrimaryClaimed)) {
            KdPrint2((PRINT_PREFIX "AtapiFindController: AtdiskPrimaryClaimed\n"));
            goto not_found;
        } else
        if((ioSpace == (PUCHAR)IO_WD2) &&
           (ConfigInfo->AtdiskSecondaryClaimed)) {
            KdPrint2((PRINT_PREFIX "AtapiFindController: AtdiskSecondaryClaimed\n"));
            goto not_found;
        }

        // Get the system physical address for the second IO range.
        if (BaseIoAddress1) {
            if(preConfig && 
               !ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[1].RangeStart)) {
                KdPrint2((PRINT_PREFIX "AtapiFindController: PCMCIA ?\n"));
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONG)BaseIoAddress1 + 0x0E),
                                                ATA_ALTIOSIZE,
                                                TRUE);
            } else {
                ioSpace = (PUCHAR)ScsiPortGetDeviceBase(HwDeviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress((ULONG)BaseIoAddress1 + ATA_ALTOFFSET),
                                                ATA_ALTIOSIZE,
                                                TRUE);
            }
        }
        BaseIoAddress2 = (PIDE_REGISTERS_2)ioSpace;
        KdPrint2((PRINT_PREFIX "  BaseIoAddress1=%x\n", BaseIoAddress1));
        KdPrint2((PRINT_PREFIX "  BaseIoAddress2=%x\n", BaseIoAddress2));

        UniataInitMapBase(chan, BaseIoAddress1, BaseIoAddress2);
        UniataInitMapBM(deviceExtension, 0, FALSE);

retryIdentifier:

        // Select master.
        SelectDrive(chan, 0);

        // Check if card at this address.
        AtapiWritePort1(chan, IDX_IO1_o_CylinderLow, 0xAA);

        // Check if indentifier can be read back.
        if ((statusByte = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow)) != 0xAA) {

            KdPrint2((PRINT_PREFIX "AtapiFindController: Identifier read back from Master (%#x)\n",
                        statusByte));

            statusByte = AtapiReadPort1(chan, IDX_IO2_AltStatus);

            if (statusByte & IDE_STATUS_BUSY) {

                i = 0;

                // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
                // warm boots don't clear.
                do {
                    AtapiStallExecution(1000);
                    statusByte = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_Status);
                    KdPrint2((PRINT_PREFIX
                                "AtapiFindController: First access to status %#x\n",
                                statusByte));
                } while ((statusByte & IDE_STATUS_BUSY) && ++i < 10);

                if (retryCount-- && (!(statusByte & IDE_STATUS_BUSY))) {
                    goto retryIdentifier;
                }
            }

            // Select slave.
            SelectDrive(chan, 1);

            // See if slave is present.
            AtapiWritePort1(chan, IDX_IO1_o_CylinderLow, 0xAA);

            if ((statusByte = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow)) != 0xAA) {

                KdPrint2((PRINT_PREFIX
                            "AtapiFindController: Identifier read back from Slave (%#x)\n",
                            statusByte));
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
                continue;
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

            (*ConfigInfo->AccessRanges)[0].RangeLength = 8;
            (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

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
        ConfigInfo->MaximumNumberOfTargets = 2;

        // Indicate maximum transfer length is 64k.
        ConfigInfo->MaximumTransferLength = 0x10000;
        deviceExtension->MaximumDmaTransferLength = ConfigInfo->MaximumTransferLength;

        KdPrint2((PRINT_PREFIX "de %#x, Channel ???\n", deviceExtension));
        //PrintNtConsole("de %#x, Channel %#x, nchan %#x\n",deviceExtension, channel, deviceExtension->NumberChannels);

        KdPrint2((PRINT_PREFIX "chan = %#x\n", chan));
        //PrintNtConsole("chan = %#x, c=%#x\n", chan, c);
        chan->DeviceExtension = deviceExtension;
        chan->lChannel        = 0;
        chan->lun[0] = &(deviceExtension->lun[0]);
        chan->lun[1] = &(deviceExtension->lun[1]);

        /* do extra channel-specific setups */
        AtapiReadChipConfig(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, 0);
        AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, 0);

        KdPrint2((PRINT_PREFIX
                   "AtapiFindController: Found IDE at %#x\n",
                   BaseIoAddress1));

        // For Daytona, the atdisk driver gets the first shot at the
        // primary and secondary controllers.
        if (preConfig == FALSE) {

            if (*adapterCount - 1 < 2) {

                // Determine whether this driver is being initialized by the
                // system or as a crash dump driver.
                if (ArgumentString) {

#ifndef UNIATA_CORE
                    if (AtapiParseArgumentString(ArgumentString, "dump") == 1) {
                        KdPrint2((PRINT_PREFIX
                                   "AtapiFindController: Crash dump\n"));
                        atapiOnly = FALSE;
                        deviceExtension->DriverMustPoll = TRUE;
                    } else {
                        KdPrint2((PRINT_PREFIX
                                   "AtapiFindController: Atapi Only\n"));
                        atapiOnly = TRUE;
                        deviceExtension->DriverMustPoll = FALSE;
                    }
#endif //UNIATA_CORE
                } else {

                    KdPrint2((PRINT_PREFIX
                               "AtapiFindController: Atapi Only (2)\n"));
                    atapiOnly = TRUE;
                    deviceExtension->DriverMustPoll = FALSE;
                }

            } else {
                atapiOnly = FALSE;
            }

        } else {

            atapiOnly = FALSE;
            deviceExtension->DriverMustPoll = FALSE;

        }// preConfig check

        // Save the Interrupe Mode for later use
        deviceExtension->InterruptMode = ConfigInfo->InterruptMode;

        KdPrint2((PRINT_PREFIX
                   "AtapiFindController: look for devices\n"));
        // Search for devices on this controller.
        if (FindDevices(HwDeviceExtension,
                        0,
                        0 /* Channel */)) {

            KdPrint2((PRINT_PREFIX
                       "AtapiFindController: detected\n"));
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

            KdPrint2((PRINT_PREFIX 
                       "AtapiFindController: return SP_RETURN_FOUND\n"));
            return(SP_RETURN_FOUND);
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
               "AtapiFindController: return SP_RETURN_NOT_FOUND\n"));
    return(SP_RETURN_NOT_FOUND);

} // end AtapiFindController()

BOOLEAN
UniataAnybodyHome(
    IN PVOID   HwDeviceExtension,
    IN ULONG   lChannel,
    IN ULONG   deviceNumber
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    ULONG                ldev = GET_LDEV2(lChannel, deviceNumber, 0);
    PHW_LU_EXTENSION     LunExt = &(deviceExtension->lun[ldev]);

    SATA_SSTATUS_REG     SStatus;
    UCHAR                signatureLow;
    UCHAR                signatureHigh;

    if(LunExt->DeviceFlags & DFLAGS_HIDDEN) {
        KdPrint2((PRINT_PREFIX "  hidden\n"));
        UniataForgetDevice(LunExt);
        return FALSE;
    }
    // Select the device.
    SelectDrive(chan, deviceNumber);

    signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
    signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

    if (signatureLow == ATAPI_MAGIC_LSB && signatureHigh == ATAPI_MAGIC_MSB) {
        KdPrint2((PRINT_PREFIX "  ATAPI at home\n", signatureLow));
        return TRUE;
    }

    if(!UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        AtapiStallExecution(10);

        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0x55);
        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0x55);
        AtapiStallExecution(5);
        signatureLow = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
        if(signatureLow != 0x55) {
            KdPrint2((PRINT_PREFIX "  nobody home! %#x != 0x55\n", signatureLow));
            UniataForgetDevice(LunExt);
            return FALSE;
        }

        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0xAA);
        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber, 0xAA);
        AtapiStallExecution(5);
        signatureLow = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
        if(signatureLow != 0xAA) {
            KdPrint2((PRINT_PREFIX "  nobody home! %#x != 0xAA\n", signatureLow));
            UniataForgetDevice(LunExt);
            return FALSE;
        }
    } else {

        SStatus.Reg = AtapiReadPort4(chan, IDX_SATA_SStatus);
        KdPrint2((PRINT_PREFIX "SStatus %x\n", SStatus.Reg));
        if(SStatus.DET <= SStatus_DET_Dev_NoPhy) {
            KdPrint2((PRINT_PREFIX "  SATA DET <= SStatus_DET_Dev_NoPhy\n"));
            return FALSE;
        }
        if(SStatus.SPD < SStatus_SPD_Gen1) {
            KdPrint2((PRINT_PREFIX "  SATA SPD < SStatus_SPD_Gen1\n"));
            return FALSE;
        }
        if(SStatus.IPM == SStatus_IPM_NoDev) {
            KdPrint2((PRINT_PREFIX "  SATA IPN == SStatus_IPM_NoDev\n"));
            return FALSE;
        }
    }

    return TRUE;
} // end UniataAnybodyHome()

ULONG
CheckDevice(
    IN PVOID   HwDeviceExtension,
    IN ULONG   lChannel,
    IN ULONG   deviceNumber,
    IN BOOLEAN ResetDev
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    ULONG                ldev = GET_LDEV2(lChannel, deviceNumber, 0);
    PHW_LU_EXTENSION     LunExt = &(deviceExtension->lun[ldev]);

    UCHAR                signatureLow,
                         signatureHigh;
    UCHAR                statusByte;
    ULONG                RetVal=0;

    KdPrint2((PRINT_PREFIX "CheckDevice: Device %#x\n",
               deviceNumber));
    if(ResetDev) {
        KdPrint2((PRINT_PREFIX "CheckDevice: reset dev\n"));

        // Reset device
        AtapiSoftReset(chan, deviceNumber);

        if(!UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber)) {
            return 0;
        }
        statusByte = WaitOnBusy(chan);

        if((statusByte | IDE_STATUS_BUSY) == 0xff) {
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: bad status %x\n", statusByte));
        } else
        if(statusByte != 0xff && (statusByte & IDE_STATUS_BUSY)) {
            // Perform hard-reset.
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: BUSY\n"));
            GetBaseStatus(chan, statusByte);
        }

        if((statusByte | IDE_STATUS_BUSY) == 0xff) {
            KdPrint2((PRINT_PREFIX 
                        "CheckDevice: no dev ?\n"));
        } else
        if(UniataIsSATARangeAvailable(deviceExtension, lChannel)) {
        //if(deviceExtension->HwFlags & UNIATA_SATA) {
            KdPrint2((PRINT_PREFIX
                        "CheckDevice: try enable SATA Phy\n"));
            statusByte = UniataSataPhyEnable(HwDeviceExtension, lChannel);
            if(statusByte == 0xff) {
                KdPrint2((PRINT_PREFIX "CheckDevice: status %#x (no dev)\n", statusByte));
                UniataForgetDevice(LunExt);
                return 0;
            }
        }
    }
    // Select the device.
    SelectDrive(chan, deviceNumber);

    if(!UniataAnybodyHome(HwDeviceExtension, lChannel, deviceNumber)) {
        return 0;
    }

    statusByte = WaitOnBaseBusyLong(chan);

    GetBaseStatus(chan, statusByte);
    if(deviceExtension->HwFlags & UNIATA_SATA) {
        UniataSataClearErr(HwDeviceExtension, lChannel, UNIATA_SATA_IGNORE_CONNECT);
    }

    KdPrint2((PRINT_PREFIX "CheckDevice: status %#x\n", statusByte));
    if(((statusByte | IDE_STATUS_BUSY) == 0xff) ||
        (statusByte & IDE_STATUS_BUSY)) {
        KdPrint2((PRINT_PREFIX "CheckDevice: busy => return\n"));
        UniataForgetDevice(LunExt);
        return 0;
    }

    // set default costs
    LunExt->RwSwitchCost  = REORDER_COST_SWITCH_RW_HDD;
    LunExt->RwSwitchMCost = REORDER_MCOST_SWITCH_RW_HDD;
    LunExt->SeekBackMCost = REORDER_MCOST_SEEK_BACK_HDD;

    signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
    signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

    if (signatureLow == ATAPI_MAGIC_LSB && signatureHigh == ATAPI_MAGIC_MSB) {

        KdPrint2((PRINT_PREFIX "CheckDevice: ATAPI signature found\n"));
        // ATAPI signature found.
        // Issue the ATAPI identify command if this
        // is not for the crash dump utility.

        if (!deviceExtension->DriverMustPoll) {

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
        }
        GetBaseStatus(chan, statusByte);
    }
    KdPrint2((PRINT_PREFIX "CheckDevice: check status: %sfound\n", RetVal ? "" : "not "));
    return RetVal;
} // end CheckDevice()


/*++

Routine Description:

    This routine is called from AtapiFindController to identify
    devices attached to an IDE controller.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    AtapiOnly - Indicates that routine should return TRUE only if
        an ATAPI device is attached to the controller.

Return Value:

    TRUE - True if devices found.

--*/
BOOLEAN
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
    ULONG                ldev;
    ULONG                max_ldev;
    BOOLEAN              AtapiOnly = FALSE;

    KdPrint2((PRINT_PREFIX "FindDevices:\n"));

    // Disable interrupts
    AtapiDisableInterrupts(deviceExtension, Channel);
//    AtapiWritePort1(chan, IDX_IO2_o_Control,IDE_DC_DISABLE_INTERRUPTS | IDE_DC_A_4BIT );

    // Clear expecting interrupt flag and current SRB field.
    chan->ExpectingInterrupt = FALSE;
//    chan->CurrentSrb = NULL;
    max_ldev = (chan->ChannelCtrlFlags & CTRFLAGS_NO_SLAVE) ? 1 : 2;
    KdPrint2((PRINT_PREFIX "  max_ldev %d\n", max_ldev));

    // Search for devices.
    for (i = 0; i < max_ldev; i++) {
        AtapiDisableInterrupts(deviceExtension, Channel);
        if(Flags & UNIATA_FIND_DEV_UNHIDE) {
            ldev = GET_LDEV2(Channel, i, 0);
            deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_HIDDEN;
        }
        deviceResponded |= 
            (CheckDevice(HwDeviceExtension, Channel, i, TRUE) != 0);
        AtapiEnableInterrupts(deviceExtension, Channel);
    }

    for (i = 0; i < max_ldev; i++) {
        ldev = GET_LDEV2(Channel, i, 0);
        LunExt = &(deviceExtension->lun[ldev]);

        if ((  LunExt->DeviceFlags & DFLAGS_DEVICE_PRESENT) &&
             !(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED) &&
             !(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) && deviceResponded) {

            // This hideous hack is to deal with ESDI devices that return
            // garbage geometry in the IDENTIFY data.
            // This is ONLY for the crashdump environment as
            // these are ESDI devices.
            if (LunExt->IdentifyData.SectorsPerTrack ==
                    0x35 &&
                LunExt->IdentifyData.NumberOfHeads ==
                    0x07) {

                KdPrint2((PRINT_PREFIX 
                           "FindDevices: Found nasty Compaq ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack =
                    0x34;
                LunExt->IdentifyData.NumberOfHeads =
                    0x0E;
            }

            if (LunExt->IdentifyData.SectorsPerTrack ==
                    0x35 &&
                LunExt->IdentifyData.NumberOfHeads ==
                    0x0F) {

                KdPrint2((PRINT_PREFIX 
                           "FindDevices: Found nasty Compaq ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack =
                    0x34;
                LunExt->IdentifyData.NumberOfHeads =
                    0x0F;
            }


            if (LunExt->IdentifyData.SectorsPerTrack ==
                    0x36 &&
                LunExt->IdentifyData.NumberOfHeads ==
                    0x07) {

                KdPrint2((PRINT_PREFIX "FindDevices: Found nasty UltraStor ESDI!\n"));

                // Change these values to something reasonable.
                LunExt->IdentifyData.SectorsPerTrack =
                    0x3F;
                LunExt->IdentifyData.NumberOfHeads =
                    0x10;
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

                AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_RESET_CONTROLLER );
                AtapiStallExecution(500 * 1000);
                AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_REENABLE_CONTROLLER);
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
                UniataForgetDevice(&(deviceExtension->lun[ldev]));
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
            ldev = GET_LDEV2(Channel, i, 0);
            LunExt = &(deviceExtension->lun[ldev]);

            KdPrint2((PRINT_PREFIX 
                       "FindDevices: select %d dev to clear INTR\n", i));
            SelectDrive(chan, i);
            GetBaseStatus(chan, statusByte);
            KdPrint2((PRINT_PREFIX 
                       "FindDevices: statusByte=%#x\n", statusByte));
        }
        for (i = 0; i < max_ldev; i++) {
            ldev = GET_LDEV2(Channel, i, 0);
            LunExt = &(deviceExtension->lun[ldev]);

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
