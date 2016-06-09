/*
 * PROJECT:        ReactOS Kernel
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        To Implement AHCI Miniport driver targeting storport NT 5.2
 * PROGRAMMERS:    Aman Priyadarshi (aman.eureka@gmail.com)
 */

#include "storahci.h"

/**
 * @name AhciPortInitialize
 * @implemented
 *
 * Initialize port by setting up PxCLB & PxFB Registers
 *
 * @param portExtension
 *
 * @return
 * Return true if intialization was successful
 */
BOOLEAN
AhciPortInitialize (
    __in      PAHCI_PORT_EXTENSION                portExtension
    )
{
    ULONG mappedLength;
    PAHCI_MEMORY_REGISTERS abar;
    PAHCI_ADAPTER_EXTENSION adapterExtension;
    STOR_PHYSICAL_ADDRESS commandListPhysical, receivedFISPhysical;

    DebugPrint("AhciPortInitialize()\n");

    NT_ASSERT(portExtension != NULL);

    adapterExtension = portExtension->AdapterExtension;
    abar = adapterExtension->ABAR_Address;

    NT_ASSERT(abar != NULL);

    portExtension->Port = &abar->PortList[portExtension->PortNumber];

    commandListPhysical = StorPortGetPhysicalAddress(   adapterExtension,
                                                        NULL,
                                                        portExtension->CommandList,
                                                        &mappedLength);

    if ((mappedLength) == 0 || ((commandListPhysical.LowPart % 1024) != 0))
    {
        DebugPrint("\tcommandListPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    receivedFISPhysical = StorPortGetPhysicalAddress(   adapterExtension,
                                                        NULL,
                                                        portExtension->ReceivedFIS,
                                                        &mappedLength);

    if ((mappedLength) == 0 || ((receivedFISPhysical.LowPart % 1024) != 0))
    {
        DebugPrint("\treceivedFISPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    if ((adapterExtension->CAP & AHCI_Global_HBA_CAP_S64A) != 0)
    {
        DebugPrint("\tCAP.S64A not supported\n");
        return FALSE;
    }

    // 10.1.2 For each implemented port, system software shall allocate memory for and program:
    //  PxCLB and PxCLBU (if CAP.S64A is set to ‘1’)
    //  PxFB and PxFBU (if CAP.S64A is set to ‘1’)
    // Note: Assuming 32bit support only
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->CLB, commandListPhysical.LowPart);
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->FB, receivedFISPhysical.LowPart);

    // set device power state flag to D0
    portExtension->DevicePowerState = StorPowerDeviceD0;

    // clear pending interrupts
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->SERR, (ULONG)-1);
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->IS, (ULONG)-1);
    StorPortWriteRegisterUlong(adapterExtension, portExtension->AdapterExtension->IS, (1 << portExtension->PortNumber));

    return TRUE;
}// -- AhciPortInitialize();

/**
 * @name AhciAllocateResourceForAdapter
 * @implemented
 *
 * Allocate memory from poll for required pointers
 *
 * @param adapterExtension
 * @param ConfigInfo
 *
 * @return
 * return TRUE if allocation was successful
 */
BOOLEAN
AhciAllocateResourceForAdapter (
    __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
    __in      PPORT_CONFIGURATION_INFORMATION     ConfigInfo
    )
{
    PVOID portsExtension = NULL;
    PCHAR nonCachedExtension;
    ULONG status, index, NCS, AlignedNCS;
    ULONG portCount, portImplemented, nonCachedExtensionSize;

    DebugPrint("AhciAllocateResourceForAdapter()\n");

    NCS = AHCI_Global_Port_CAP_NCS(adapterExtension->CAP);
    AlignedNCS = ROUND_UP(NCS, 8);

    // get port count -- Number of set bits in `adapterExtension->PortImplemented`
    portCount = 0;
    portImplemented = adapterExtension->PortImplemented;
    while (portImplemented > 0)
    {
        portCount++;
        portImplemented &= (portImplemented - 1);
    }

    NT_ASSERT(portCount != 0);
    DebugPrint("\tPort Count: %d\n", portCount);

    nonCachedExtensionSize =    sizeof(AHCI_COMMAND_HEADER) * AlignedNCS + //should be 1K aligned
                                sizeof(AHCI_RECEIVED_FIS);

    // align nonCachedExtensionSize to 1024
    nonCachedExtensionSize = ROUND_UP(nonCachedExtensionSize, 1024);

    adapterExtension->NonCachedExtension = StorPortGetUncachedExtension(    adapterExtension,
                                                                            ConfigInfo,
                                                                            nonCachedExtensionSize * portCount);

    if (adapterExtension->NonCachedExtension == NULL)
    {
        DebugPrint("\tadapterExtension->NonCachedExtension == NULL\n");
        return FALSE;
    }

    nonCachedExtension = adapterExtension->NonCachedExtension;
    AhciZeroMemory(nonCachedExtension, nonCachedExtensionSize * portCount);

    for (index = 0; index < MAXIMUM_AHCI_PORT_COUNT; index++)
    {
        adapterExtension->PortExtension[index].IsActive = FALSE;
        if ((adapterExtension->PortImplemented & (1 << index)) != 0)
        {
            adapterExtension->PortExtension[index].PortNumber = index;
            adapterExtension->PortExtension[index].IsActive = TRUE;
            adapterExtension->PortExtension[index].AdapterExtension = adapterExtension;
            adapterExtension->PortExtension[index].CommandList = nonCachedExtension;
            adapterExtension->PortExtension[index].ReceivedFIS = (PAHCI_RECEIVED_FIS)(nonCachedExtension + sizeof(AHCI_COMMAND_HEADER) * AlignedNCS);
            nonCachedExtension += nonCachedExtensionSize;
        }
    }

    return TRUE;
}// -- AhciAllocateResourceForAdapter();

/**
 * @name AhciHwInitialize
 * @implemented
 *
 * initializes the HBA and finds all devices that are of interest to the miniport driver.
 *
 * @param adapterExtension
 *
 * @return
 * return TRUE if intialization was successful
 */
BOOLEAN
AhciHwInitialize (
    __in      PVOID                               AdapterExtension
    )
{
    ULONG ghc, messageCount, status;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    DebugPrint("AhciHwInitialize()\n");

    adapterExtension = AdapterExtension;
    adapterExtension->StateFlags.MessagePerPort = FALSE;

    // First check what type of interrupt/synchronization device is using
    ghc = StorPortReadRegisterUlong(adapterExtension, &adapterExtension->ABAR_Address->GHC);

    // When set to ‘1’ by hardware, indicates that the HBA requested more than one MSI vector
    // but has reverted to using the first vector only.  When this bit is cleared to ‘0’,
    // the HBA has not reverted to single MSI mode (i.e. hardware is already in single MSI mode,
    // software has allocated the number of messages requested
    if ((ghc & AHCI_Global_HBA_CONTROL_MRSM) == 0)
    {
        adapterExtension->StateFlags.MessagePerPort = TRUE;
        DebugPrint("\tMultiple MSI based message not supported\n");
    }

    return TRUE;
}// -- AhciHwInitialize();

/**
 * @name AhciInterruptHandler
 * @implemented
 *
 * Interrupt Handler for portExtension
 *
 * @param portExtension
 *
 */
VOID
AhciInterruptHandler (
    __in      PAHCI_PORT_EXTENSION                portExtension
    )
{
    DebugPrint("AhciInterruptHandler()\n");
    DebugPrint("\tPort Number: %d\n", portExtension->PortNumber);

}// -- AhciInterruptHandler();

/**
 * @name AhciHwInterrupt
 * @implemented
 *
 * The Storport driver calls the HwStorInterrupt routine after the HBA generates an interrupt request.
 *
 * @param adapterExtension
 *
 * @return
 * return TRUE Indicates that an interrupt was pending on adapter.
 * return FALSE Indicates the interrupt was not ours.
 */
BOOLEAN
AhciHwInterrupt(
    __in      PVOID                               AdapterExtension
    )
{
    ULONG portPending, nextPort, i;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    DebugPrint("AhciHwInterrupt()\n");

    adapterExtension = AdapterExtension;

    if (adapterExtension->StateFlags.Removed)
    {
        return FALSE;
    }

    portPending = StorPortReadRegisterUlong(adapterExtension, adapterExtension->IS);
    // we process interrupt for implemented ports only
    portPending = portPending & adapterExtension->PortImplemented;

    if (portPending == 0)
    {
        return FALSE;
    }

    for (i = 1; i <= MAXIMUM_AHCI_PORT_COUNT; i++)
    {
        nextPort = (adapterExtension->LastInterruptPort + i) % MAXIMUM_AHCI_PORT_COUNT;

        if ((portPending & (0x1 << nextPort)) == 0)
            continue;

        if ((nextPort == adapterExtension->LastInterruptPort)
            || (adapterExtension->PortExtension[nextPort].IsActive == FALSE))
        {
            return FALSE;
        }

        // we can assign this interrupt to this port
        adapterExtension->LastInterruptPort = nextPort;
        AhciInterruptHandler(&adapterExtension->PortExtension[nextPort]);
        return TRUE;
    }

    DebugPrint("\tSomething wrong");
    return FALSE;
}// -- AhciHwInterrupt();

/**
 * @name AhciHwStartIo
 * @implemented
 *
 * The Storport driver calls the HwStorStartIo routine one time for each incoming I/O request.
 *
 * @param adapterExtension
 * @param Srb
 *
 * @return
 * return TRUE if the request was accepted
 * return FALSE if the request must be submitted later
 */
BOOLEAN
AhciHwStartIo (
    __in      PVOID                               AdapterExtension,
    __in      PSCSI_REQUEST_BLOCK                 Srb
    )
{
    UCHAR function, pathId;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    DebugPrint("AhciHwStartIo()\n");

    pathId = Srb->PathId;
    function = Srb->Function;
    adapterExtension = AdapterExtension;

    if (!IsPortValid(adapterExtension, pathId))
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        StorPortNotification(RequestComplete, adapterExtension, Srb);
        return TRUE;
    }

    // https://msdn.microsoft.com/windows/hardware/drivers/storage/handling-srb-function-pnp
    // If the function member of an SRB is set to SRB_FUNCTION_PNP,
    // the SRB is a structure of type SCSI_PNP_REQUEST_BLOCK.
    if (function == SRB_FUNCTION_PNP)
    {
        PSCSI_PNP_REQUEST_BLOCK pnpRequest;

        pnpRequest = (PSCSI_PNP_REQUEST_BLOCK)Srb;
        if ((pnpRequest->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST) != 0)
        {
            if ((pnpRequest->PnPAction == StorRemoveDevice) ||
                (pnpRequest->PnPAction == StorSurpriseRemoval))
            {
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                adapterExtension->StateFlags.Removed = 1;
                DebugPrint("\tadapter removed\n");
            }
            else if (pnpRequest->PnPAction == StorStopDevice)
            {
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                DebugPrint("\tRequested to Stop the adapter\n");
            }
            else
            {
                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            }

            StorPortNotification(RequestComplete, adapterExtension, Srb);
            return TRUE;
        }
    }

    if (function == SRB_FUNCTION_EXECUTE_SCSI)
    {
        // https://msdn.microsoft.com/en-us/windows/hardware/drivers/storage/handling-srb-function-execute-scsi
        // On receipt of an SRB_FUNCTION_EXECUTE_SCSI request, a miniport driver's HwScsiStartIo
        // routine does the following:
        //
        // - Gets and/or sets up whatever context the miniport driver maintains in its device,
        //   logical unit, and/or SRB extensions
        //   For example, a miniport driver might set up a logical unit extension with pointers
        //   to the SRB itself and the SRB DataBuffer pointer, the SRB DataTransferLength value,
        //   and a driver-defined value (or CDB SCSIOP_XXX value) indicating the operation to be
        //   carried out on the HBA.
        //
        // - Calls an internal routine to program the HBA, as partially directed by the SrbFlags,
        //   for the requested operation
        //   For a device I/O operation, such an internal routine generally selects the target device
        //   and sends the CDB over the bus to the target logical unit.
        if (Srb->CdbLength > 0)
        {
            PCDB cdb = (PCDB)&Srb->Cdb;
            if (cdb->CDB10.OperationCode == SCSIOP_INQUIRY)
            {
                Srb->SrbStatus = DeviceInquiryRequest(adapterExtension, Srb, cdb);
                StorPortNotification(RequestComplete, adapterExtension, Srb);

                return TRUE;
            }
        }
        else
        {
            Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
            StorPortNotification(RequestComplete, adapterExtension, Srb);
            return TRUE;
        }
    }

    DebugPrint("\tUnknow function code recieved: %x\n", function);
    Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
    StorPortNotification(RequestComplete, adapterExtension, Srb);
    return TRUE;
}// -- AhciHwStartIo();

/**
 * @name AhciHwResetBus
 * @not_implemented
 *
 * The HwStorResetBus routine is called by the port driver to clear error conditions.
 *
 * @param adapterExtension
 * @param PathId
 *
 * @return
 * return TRUE if bus was successfully reset
 */
BOOLEAN
AhciHwResetBus (
    __in      PVOID                               AdapterExtension,
    __in      ULONG                               PathId
    )
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    DebugPrint("AhciHwResetBus()\n");

    adapterExtension = AdapterExtension;

    return FALSE;
}// -- AhciHwResetBus();

/**
 * @name AhciHwFindAdapter
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
ULONG
AhciHwFindAdapter (
    __in      PVOID                               AdapterExtension,
    __in      PVOID                               HwContext,
    __in      PVOID                               BusInformation,
    __in      PVOID                               ArgumentString,
    __inout   PPORT_CONFIGURATION_INFORMATION     ConfigInfo,
    __in      PBOOLEAN                            Reserved3
    )
{
    ULONG ghc;
    ULONG index;
    ULONG portCount, portImplemented;
    ULONG pci_cfg_len;
    UCHAR pci_cfg_buf[sizeof(PCI_COMMON_CONFIG)];
    PACCESS_RANGE accessRange;

    PAHCI_MEMORY_REGISTERS abar;
    PPCI_COMMON_CONFIG pciConfigData;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    DebugPrint("AhciHwFindAdapter()\n");

    adapterExtension = AdapterExtension;
    adapterExtension->SlotNumber = ConfigInfo->SlotNumber;
    adapterExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;

    // get PCI configuration header
    pci_cfg_len = StorPortGetBusData(
                        adapterExtension,
                        PCIConfiguration,
                        adapterExtension->SystemIoBusNumber,
                        adapterExtension->SlotNumber,
                        pci_cfg_buf,
                        sizeof(PCI_COMMON_CONFIG));

    if (pci_cfg_len != sizeof(PCI_COMMON_CONFIG))
    {
        DebugPrint("\tpci_cfg_len != %d :: %d", sizeof(PCI_COMMON_CONFIG), pci_cfg_len);
        return SP_RETURN_ERROR;//Not a valid device at the given bus number
    }

    pciConfigData = pci_cfg_buf;
    adapterExtension->VendorID = pciConfigData->VendorID;
    adapterExtension->DeviceID = pciConfigData->DeviceID;
    adapterExtension->RevisionID = pciConfigData->RevisionID;
    // The last PCI base address register (BAR[5], header offset 0x24) points to the AHCI base memory, it’s called ABAR (AHCI Base Memory Register).
    adapterExtension->AhciBaseAddress = pciConfigData->u.type0.BaseAddresses[5] & (0xFFFFFFF0);

    DebugPrint("\tVendorID:%d  DeviceID:%d  RevisionID:%d\n", adapterExtension->VendorID, adapterExtension->DeviceID, adapterExtension->RevisionID);

    // 2.1.11
    abar = NULL;
    if (ConfigInfo->NumberOfAccessRanges > 0)
    {
        for (index = 0; index < ConfigInfo->NumberOfAccessRanges; index++)
        {
            accessRange = *ConfigInfo->AccessRanges;
            if (accessRange[index].RangeStart.QuadPart == adapterExtension->AhciBaseAddress)
            {
                abar = StorPortGetDeviceBase(
                                adapterExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                accessRange[index].RangeStart,
                                accessRange[index].RangeLength,
                                !accessRange[index].RangeInMemory);
                break;
            }
        }
    }

    if (abar == NULL)
    {
        DebugPrint("\tabar == NULL\n");
        return SP_RETURN_ERROR; // corrupted information supplied
    }

    adapterExtension->ABAR_Address = abar;
    adapterExtension->CAP = StorPortReadRegisterUlong(adapterExtension, &abar->CAP);
    adapterExtension->CAP2 = StorPortReadRegisterUlong(adapterExtension, &abar->CAP2);
    adapterExtension->Version = StorPortReadRegisterUlong(adapterExtension, &abar->VS);
    adapterExtension->LastInterruptPort = -1;

    // 10.1.2
    // 1. Indicate that system software is AHCI aware by setting GHC.AE to ‘1’.
    // 3.1.2 -- AE bit is read-write only if CAP.SAM is '0'
    ghc = StorPortReadRegisterUlong(adapterExtension, &abar->GHC);
    // AE := Highest Significant bit of GHC
    if ((ghc & AHCI_Global_HBA_CONTROL_AE) != 0)//Hmm, controller was already in power state
    {
        // reset controller to have it in known state
        DebugPrint("\tAE Already set, Reset()\n");
        if (!AhciAdapterReset(adapterExtension)){
            DebugPrint("\tReset Failed!\n");
            return SP_RETURN_ERROR;// reset failed
        }
    }

    ghc = AHCI_Global_HBA_CONTROL_AE;// only AE=1
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    adapterExtension->IS = &abar->IS;
    adapterExtension->PortImplemented = StorPortReadRegisterUlong(adapterExtension, &abar->PI);

    if (adapterExtension->PortImplemented == 0)
    {
        DebugPrint("\tadapterExtension->PortImplemented == 0\n");
        return SP_RETURN_ERROR;
    }

    ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;//128 KB
    ConfigInfo->NumberOfPhysicalBreaks = 0x21;
    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->NumberOfBuses = MAXIMUM_AHCI_PORT_COUNT;
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
    ConfigInfo->ScatterGather = TRUE;

    // Turn IE -- Interrupt Enabled
    ghc |= AHCI_Global_HBA_CONTROL_IE;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    // allocate necessary resource for each port
    if (!AhciAllocateResourceForAdapter(adapterExtension, ConfigInfo))
    {
        DebugPrint("\tAhciAllocateResourceForAdapter() == FALSE\n");
        return SP_RETURN_ERROR;
    }

    for (index = 0; index < MAXIMUM_AHCI_PORT_COUNT; index++)
    {
        if ((adapterExtension->PortImplemented & (0x1<<index)) != 0)
            AhciPortInitialize(&adapterExtension->PortExtension[index]);
    }

    return SP_RETURN_FOUND;
}// -- AhciHwFindAdapter();

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
ULONG
DriverEntry (
    __in      PVOID                               DriverObject,
    __in      PVOID                               RegistryPath
    )
{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i, status;

    DebugPrint("Storahci Loaded\n");

    // initialize the hardware data structure
    AhciZeroMemory(&hwInitializationData, sizeof(HW_INITIALIZATION_DATA));

    // set size of hardware initialization structure
    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // identity required miniport entry point routines
    hwInitializationData.HwStartIo = AhciHwStartIo;
    hwInitializationData.HwResetBus = AhciHwResetBus;
    hwInitializationData.HwInterrupt = AhciHwInterrupt;
    hwInitializationData.HwInitialize = AhciHwInitialize;
    hwInitializationData.HwFindAdapter = AhciHwFindAdapter;

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

    DebugPrint("\tstatus:%x\n", status);
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
BOOLEAN
AhciAdapterReset (
    __in      PAHCI_ADAPTER_EXTENSION             adapterExtension
    )
{
    ULONG ghc, ticks;
    PAHCI_MEMORY_REGISTERS abar = NULL;

    DebugPrint("AhciAdapterReset()\n");

    abar = adapterExtension->ABAR_Address;
    if (abar == NULL) // basic sanity
    {
        return FALSE;
    }

    // HR -- Very first bit (lowest significant)
    ghc = AHCI_Global_HBA_CONTROL_HR;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    for (ticks = 0; ticks < 50; ++ticks)
    {
        if ((StorPortReadRegisterUlong(adapterExtension, &abar->GHC) & AHCI_Global_HBA_CONTROL_HR) == 0)
        {
            break;
        }
        StorPortStallExecution(20000);
    }

    if (ticks == 50)// 1 second
    {
        DebugPrint("\tDevice Timeout\n");
        return FALSE;
    }

    return TRUE;
}// -- AhciAdapterReset();

/**
 * @name AhciZeroMemory
 * @implemented
 *
 * Clear buffer by filling zeros
 *
 * @param buffer
 */
__inline
VOID
AhciZeroMemory (
    __out     PCHAR                               buffer,
    __in      ULONG                               bufferSize
    )
{
    ULONG i;
    for (i = 0; i < bufferSize; i++)
        buffer[i] = 0;
}// -- AhciZeroMemory();

/**
 * @name IsPortValid
 * @implemented
 *
 * Tells wheather given port is implemented or not
 *
 * @param adapterExtension
 * @param PathId
 *
 * @return
 * return TRUE if provided port is valid (implemented) or not
 */
__inline
BOOLEAN
IsPortValid (
    __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
    __in      UCHAR                               pathId
    )
{
    NT_ASSERT(pathId >= 0);

    if (pathId >= MAXIMUM_AHCI_PORT_COUNT)
    {
        return FALSE;
    }

    return adapterExtension->PortExtension[pathId].IsActive;
}// -- IsPortValid()

/**
 * @name DeviceInquiryRequest
 * @implemented
 *
 * Tells wheather given port is implemented or not
 *
 * @param adapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceInquiryRequest
 *
 * @remark
 * http://www.seagate.com/staticfiles/support/disc/manuals/Interface%20manuals/100293068c.pdf
 */
ULONG
DeviceInquiryRequest (
    __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
    __in      PSCSI_REQUEST_BLOCK                 Srb,
    __in      PCDB                                Cdb
    )
{
    PVOID DataBuffer;
    ULONG DataBufferLength;

    DebugPrint("DeviceInquiryRequest()\n");

    // 3.6.1
    // If the EVPD bit is set to zero, the device server shall return the standard INQUIRY data
    if (Cdb->CDB6INQUIRY3.EnableVitalProductData == 0)
    {
        DebugPrint("\tEVPD Inquired\n");
    }
    else
    {
        DebugPrint("\tVPD Inquired\n");

        DataBuffer = Srb->DataBuffer;
        DataBufferLength = Srb->DataTransferLength;

        if (DataBuffer == NULL)
            return SRB_STATUS_INVALID_REQUEST;

        AhciZeroMemory(DataBuffer, DataBufferLength);
    }

    return SRB_STATUS_BAD_FUNCTION;
}// -- DeviceInquiryRequest();
