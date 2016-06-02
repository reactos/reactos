/*
 * PROJECT:        ReactOS Kernel
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        To Implement AHCI Miniport driver targeting storport NT 5.2
 * PROGRAMMERS:    Aman Priyadarshi (aman.eureka@gmail.com)
 */

#include "storahci.h"

BOOLEAN AhciAdapterReset(
    __in  PAHCI_ADAPTER_EXTENSION           adapterExtension
);

/**
 * @name AhciFindAdapter
 * @implemented
 *
 * The HwStorFindAdapter routine uses the supplied configuration to determine whether a specific
 * HBA is supported and, if it is, to return configuration information about that adapter.
 *
 *  10.1 Platform Communication
 *  http://www.intel.in/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1_2.pdf

 * @param DeviceExtension
 * @param HwContext
 * @param BusInformation
 * @param ArgumentString
 * @param ConfigInfo
 * @param Reserved3
 *
 * @return
 *      SP_RETURN_FOUND
 *          Indicates that a supported HBA was found and that the HBA-relevant configuration information was successfully determined and set in the PORT_CONFIGURATION_INFORMATION structure.
 *
 *      SP_RETURN_ERROR
 *          Indicates that an HBA was found but there was an error obtaining the configuration information. If possible, such an error should be logged with StorPortLogError.
 *
 *      SP_RETURN_BAD_CONFIG
 *          Indicates that the supplied configuration information was invalid for the adapter.
 *
 *      SP_RETURN_NOT_FOUND
 *          Indicates that no supported HBA was found for the supplied configuration information.
 *
 * @remarks Called by Storport.
 */
ULONG AhciFindAdapter(
          IN PVOID                          DeviceExtension,
  __in    PVOID                             HwContext,
  __in    PVOID                             BusInformation,
  __in    IN PVOID                          ArgumentString,
  __inout PPORT_CONFIGURATION_INFORMATION   ConfigInfo,
  __in    PBOOLEAN                          Reserved3
)
{
    ULONG ghc;
    ULONG portCount, portImplemented;
    ULONG pci_cfg_len;
    UCHAR pci_cfg_buf[0x30];
    
    PAHCI_MEMORY_REGISTERS abar;
    PPCI_COMMON_CONFIG pciConfigData;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)DeviceExtension;
    adapterExtension->SlotNumber = ConfigInfo->SlotNumber;
    adapterExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    
    // get PCI configuration header
    pci_cfg_len = StorPortGetBusData(
                        adapterExtension,
                        PCIConfiguration,
                        adapterExtension->SystemIoBusNumber,
                        adapterExtension->SlotNumber,
                        (PVOID)pci_cfg_buf,
                        (ULONG)0x30);

    if (pci_cfg_len != 0x30)
        return SP_RETURN_ERROR;//Not a valid device at the given bus number

    pciConfigData = (PPCI_COMMON_CONFIG)pci_cfg_buf;
    adapterExtension->VendorID = pciConfigData->VendorID;
    adapterExtension->DeviceID = pciConfigData->DeviceID;
    adapterExtension->RevisionID = pciConfigData->RevisionID;
    // The last PCI base address register (BAR[5], header offset 0x24) points to the AHCI base memory, it’s called ABAR (AHCI Base Memory Register).
    adapterExtension->AhciBaseAddress = pciConfigData->u.type0.BaseAddresses[5] & (0xFFFFFFF0);

    // 2.1.11
    abar = NULL;
    if (ConfigInfo->NumberOfAccessRanges > 0)
    {
        ULONG accessIndex;
        for (accessIndex = 0; accessIndex < ConfigInfo->NumberOfAccessRanges; accessIndex++)
        {
            if ((*(ConfigInfo->AccessRanges))[accessIndex].RangeStart.QuadPart == adapterExtension->AhciBaseAddress)
            {
                abar = (PAHCI_MEMORY_REGISTERS)StorPortGetDeviceBase(
                                adapterExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                (*(ConfigInfo->AccessRanges))[accessIndex].RangeStart,
                                (*(ConfigInfo->AccessRanges))[accessIndex].RangeLength,
                                (BOOLEAN)!(*(ConfigInfo->AccessRanges))[accessIndex].RangeInMemory);
                break;
            }
        }
    }

    if (abar == NULL)
        return SP_RETURN_ERROR; // corrupted information supplied

    adapterExtension->ABAR_Address = abar;
    adapterExtension->CAP = StorPortReadRegisterUlong(adapterExtension, &abar->CAP);
    adapterExtension->CAP2 = StorPortReadRegisterUlong(adapterExtension, &abar->CAP2);
    adapterExtension->Version = StorPortReadRegisterUlong(adapterExtension, &abar->VS);

    // 10.1.2
    // 1. Indicate that system software is AHCI aware by setting GHC.AE to ‘1’.
    ghc = StorPortReadRegisterUlong(adapterExtension, &abar->GHC);
    // AE := Highest Significant bit of GHC
    if ((ghc & (0x1<<31)) == 1)//Hmm, controller was already in power state
    {
        // reset controller to have it in know state
        DebugPrint("AhciFindAdapter -> AE Already set, Reset()\n");
        if (!AhciAdapterReset(adapterExtension))
            return SP_RETURN_ERROR;// reset failed
    }

    ghc = 0x1<<31;// only AE=1
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    adapterExtension->IS = abar->IS;
    adapterExtension->PortImplemented = StorPortReadRegisterUlong(adapterExtension, &abar->PI);

    if (adapterExtension->PortImplemented == 0)
        return SP_RETURN_ERROR;

    // get port count -- Number of set bits in `adapterExtension->PortImplemented`
    portCount = 0;
    portImplemented = adapterExtension->PortImplemented;
    while(portImplemented > 0)
    {
        portCount++;
        portImplemented &= (portImplemented - 1);// i love playing with bits :D
    }

    ConfigInfo->MaximumTransferLength = 128 * 1024;//128 KB
    ConfigInfo->NumberOfPhysicalBreaks = 0x21;
    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->NumberOfBuses = 32;
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
    ConfigInfo->ScatterGather = TRUE;

    // Turn IE -- Interrupt Enabled
    ghc |= 0x2;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);



    return SP_RETURN_FOUND;
}// -- AhciFindAdapter();

/**
 * @name DriverEntry
 * @implemented
 *
 * Initial Entrypoint for storahci miniport driver
 *
 * @param DriverObject
 * @param RegistryPath
 *
 * @return
 * NT_STATUS in case of driver loaded successfully.
 */
ULONG DriverEntry(
            IN PVOID                        DriverObject,
            IN PVOID                        RegistryPath
)
{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i, status;

    DebugPrint("Storahci -> DriverEntry()\n");

    // initialize the hardware data structure
    for (i = 0; i < sizeof(HW_INITIALIZATION_DATA); i++)
        ((PUCHAR)&hwInitializationData)[i] = 0;

    // set size of hardware initialization structure
    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // identity required miniport entry point routines
    hwInitializationData.HwFindAdapter = AhciFindAdapter;

    // adapter specific information
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;

    hwInitializationData.NumberOfAccessRanges = 6;
    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;

    // set required extension sizes
    hwInitializationData.SrbExtensionSize = sizeof(AHCI_SRB_EXTENSION);
    hwInitializationData.DeviceExtensionSize = sizeof(AHCI_ADAPTER_EXTENSION);
    
    // register our hw init data
    status = StorPortInitialize(
                    DriverObject,
                    RegistryPath,
                    &hwInitializationData,
                    NULL);

    return status;
}// -- DriverEntry();

/**
 * @name AhciAdapterReset
 * @implemented
 *
 * 10.4.3 HBA Reset
 * If the HBA becomes unusable for multiple ports, and a software reset or port reset does not correct the
 * problem, software may reset the entire HBA by setting GHC.HR to ‘1’. When software sets the GHC.HR
 * bit to ‘1’, the HBA shall perform an internal reset action. The bit shall be cleared to ‘0’ by the HBA when
 * the reset is complete. A software write of ‘0’ to GHC.HR shall have no effect. To perform the HBA reset,
 * software sets GHC.HR to ‘1’ and may poll until this bit is read to be ‘0’, at which point software knows that
 * the HBA reset has completed.
 * If the HBA has not cleared GHC.HR to ‘0’ within 1 second of software setting GHC.HR to ‘1’, the HBA is in
 * a hung or locked state. 
 *
 * @param adapterExtension
 *
 * @return
 * TRUE in case AHCI Controller RESTARTED successfully. i.e GHC.HR == 0
 */
BOOLEAN AhciAdapterReset(
      PAHCI_ADAPTER_EXTENSION           adapterExtension    
)
{
     ULONG ghc, ticks;
     PAHCI_MEMORY_REGISTERS abar = NULL;

     abar = adapterExtension->ABAR_Address;

     if (abar == NULL) // basic sanity
        return FALSE;

     // HR -- Very first bit (lowest significant)
     ghc = 1;
     StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

     for (ticks = 0; (ticks < 50) &&
                     (StorPortReadRegisterUlong(adapterExtension, &abar->GHC) == 1);
                     StorPortStallExecution(20000), ticks++);

     if (ticks == 50)//1 second
        return FALSE;

     return TRUE;
}// -- AhciAdapterReset();
