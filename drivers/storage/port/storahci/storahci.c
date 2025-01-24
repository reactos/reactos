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
 * @param PortExtension
 *
 * @return
 * Return true if intialization was successful
 */
BOOLEAN
NTAPI
AhciPortInitialize (
    __in PVOID DeviceExtension
    )
{
    PAHCI_PORT_EXTENSION PortExtension;
    AHCI_PORT_CMD cmd;
    PAHCI_MEMORY_REGISTERS abar;
    ULONG mappedLength, portNumber, ticks;
    PAHCI_ADAPTER_EXTENSION adapterExtension;
    STOR_PHYSICAL_ADDRESS commandListPhysical, receivedFISPhysical;

    AhciDebugPrint("AhciPortInitialize()\n");

    PortExtension = (PAHCI_PORT_EXTENSION)DeviceExtension;
    adapterExtension = PortExtension->AdapterExtension;
    abar = adapterExtension->ABAR_Address;
    portNumber = PortExtension->PortNumber;

    NT_ASSERT(abar != NULL);
    NT_ASSERT(portNumber < adapterExtension->PortCount);

    PortExtension->Port = &abar->PortList[portNumber];

    commandListPhysical = StorPortGetPhysicalAddress(adapterExtension,
                                                     NULL,
                                                     PortExtension->CommandList,
                                                     &mappedLength);

    if ((mappedLength == 0) || ((commandListPhysical.LowPart % 1024) != 0))
    {
        AhciDebugPrint("\tcommandListPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    receivedFISPhysical = StorPortGetPhysicalAddress(adapterExtension,
                                                     NULL,
                                                     PortExtension->ReceivedFIS,
                                                     &mappedLength);

    if ((mappedLength == 0) || ((receivedFISPhysical.LowPart % 256) != 0))
    {
        AhciDebugPrint("\treceivedFISPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    // Ensure that the controller is not in the running state by reading and examining each
    // implemented port’s PxCMD register. If PxCMD.ST, PxCMD.CR, PxCMD.FRE and
    // PxCMD.FR are all cleared, the port is in an idle state. Otherwise, the port is not idle and
    // should be placed in the idle state prior to manipulating HBA and port specific registers.
    // System software places a port into the idle state by clearing PxCMD.ST and waiting for
    // PxCMD.CR to return ‘0’ when read. Software should wait at least 500 milliseconds for
    // this to occur. If PxCMD.FRE is set to ‘1’, software should clear it to ‘0’ and wait at least
    // 500 milliseconds for PxCMD.FR to return ‘0’ when read. If PxCMD.CR or PxCMD.FR do
    // not clear to ‘0’ correctly, then software may attempt a port reset or a full HBA reset to recove

    // TODO: Check if port is in idle state or not, if not then restart port
    cmd.Status = StorPortReadRegisterUlong(adapterExtension, &PortExtension->Port->CMD);
    if ((cmd.FR != 0) || (cmd.CR != 0) || (cmd.FRE != 0) || (cmd.ST != 0))
    {
        cmd.ST = 0;
        cmd.FRE = 0;

        ticks = 3;
        do
        {
            StorPortStallExecution(50000);
            cmd.Status = StorPortReadRegisterUlong(adapterExtension, &PortExtension->Port->CMD);
            if (ticks == 0)
            {
                AhciDebugPrint("\tAttempt to reset port failed: %x\n", cmd);
                return FALSE;
            }
            ticks--;
        }
        while(cmd.CR != 0 || cmd.FR != 0);
    }

    // 10.1.2 For each implemented port, system software shall allocate memory for and program:
    // ? PxCLB and PxCLBU (if CAP.S64A is set to ‘1’)
    // ? PxFB and PxFBU (if CAP.S64A is set to ‘1’)
    // Note: Assuming 32bit support only
    StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->CLB, commandListPhysical.LowPart);
    if (IsAdapterCAPS64(adapterExtension->CAP))
    {
        StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->CLBU, commandListPhysical.HighPart);
    }

    StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->FB, receivedFISPhysical.LowPart);
    if (IsAdapterCAPS64(adapterExtension->CAP))
    {
        StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->FBU, receivedFISPhysical.HighPart);
    }

    PortExtension->IdentifyDeviceDataPhysicalAddress = StorPortGetPhysicalAddress(adapterExtension,
                                                                                  NULL,
                                                                                  PortExtension->IdentifyDeviceData,
                                                                                  &mappedLength);

    // set device power state flag to D0
    PortExtension->DevicePowerState = StorPowerDeviceD0;

    // clear pending interrupts
    StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->SERR, (ULONG)~0);
    StorPortWriteRegisterUlong(adapterExtension, &PortExtension->Port->IS, (ULONG)~0);
    StorPortWriteRegisterUlong(adapterExtension, adapterExtension->IS, (1 << PortExtension->PortNumber));

    return TRUE;
}// -- AhciPortInitialize();

/**
 * @name AhciAllocateResourceForAdapter
 * @implemented
 *
 * Allocate memory from poll for required pointers
 *
 * @param AdapterExtension
 * @param ConfigInfo
 *
 * @return
 * return TRUE if allocation was successful
 */
BOOLEAN
AhciAllocateResourceForAdapter (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PCHAR nonCachedExtension, tmp;
    ULONG index, NCS, AlignedNCS;
    ULONG portCount, portImplemented, nonCachedExtensionSize;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("AhciAllocateResourceForAdapter()\n");

    NCS = AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP);
    AlignedNCS = ROUND_UP(NCS, 8);

    // get port count -- Number of set bits in `AdapterExtension->PortImplemented`
    portCount = 0;
    portImplemented = AdapterExtension->PortImplemented;

    NT_ASSERT(portImplemented != 0);
    for (index = MAXIMUM_AHCI_PORT_COUNT - 1; index > 0; index--)
        if ((portImplemented & (1 << index)) != 0)
            break;

    portCount = index + 1;
    AhciDebugPrint("\tPort Count: %d\n", portCount);

    AdapterExtension->PortCount = portCount;
    nonCachedExtensionSize =    sizeof(AHCI_COMMAND_HEADER) * AlignedNCS + //should be 1K aligned
                                sizeof(AHCI_RECEIVED_FIS) +
                                sizeof(IDENTIFY_DEVICE_DATA);

    // align nonCachedExtensionSize to 1024
    nonCachedExtensionSize = ROUND_UP(nonCachedExtensionSize, 1024);

    AdapterExtension->NonCachedExtension = StorPortGetUncachedExtension(AdapterExtension,
                                                                        ConfigInfo,
                                                                        nonCachedExtensionSize * portCount);

    if (AdapterExtension->NonCachedExtension == NULL)
    {
        AhciDebugPrint("\tadapterExtension->NonCachedExtension == NULL\n");
        return FALSE;
    }

    nonCachedExtension = AdapterExtension->NonCachedExtension;
    AhciZeroMemory(nonCachedExtension, nonCachedExtensionSize * portCount);

    for (index = 0; index < portCount; index++)
    {
        PortExtension = &AdapterExtension->PortExtension[index];

        PortExtension->DeviceParams.IsActive = FALSE;
        if ((AdapterExtension->PortImplemented & (1 << index)) != 0)
        {
            PortExtension->PortNumber = index;
            PortExtension->DeviceParams.IsActive = TRUE;
            PortExtension->AdapterExtension = AdapterExtension;
            PortExtension->CommandList = (PAHCI_COMMAND_HEADER)nonCachedExtension;

            tmp = (PCHAR)(nonCachedExtension + sizeof(AHCI_COMMAND_HEADER) * AlignedNCS);

            PortExtension->ReceivedFIS = (PAHCI_RECEIVED_FIS)tmp;
            PortExtension->IdentifyDeviceData = (PIDENTIFY_DEVICE_DATA)(tmp + sizeof(AHCI_RECEIVED_FIS));
            PortExtension->MaxPortQueueDepth = NCS;
            nonCachedExtension += nonCachedExtensionSize;
        }
    }

    return TRUE;
}// -- AhciAllocateResourceForAdapter();

/**
 * @name AhciStartPort
 * @implemented
 *
 * Try to start the port device
 *
 * @param AdapterExtension
 * @param PortExtension
 *
 */
BOOLEAN
AhciStartPort (
    __in PAHCI_PORT_EXTENSION PortExtension
    )
{
    ULONG index;
    AHCI_PORT_CMD cmd;
    AHCI_TASK_FILE_DATA tfd;
    AHCI_INTERRUPT_ENABLE ie;
    AHCI_SERIAL_ATA_STATUS ssts;
    AHCI_SERIAL_ATA_CONTROL sctl;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciStartPort()\n");

    AdapterExtension = PortExtension->AdapterExtension;
    cmd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CMD);

    if ((cmd.FR == 1) && (cmd.CR == 1) && (cmd.FRE == 1) && (cmd.ST == 1))
    {
        // Already Running
        return TRUE;
    }

    cmd.SUD = 1;
    StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->CMD, cmd.Status);

    if (((cmd.FR == 1) && (cmd.FRE == 0)) ||
        ((cmd.CR == 1) && (cmd.ST == 0)))
    {
        AhciDebugPrint("\tCOMRESET\n");
        // perform COMRESET
        // section 10.4.2

        // Software causes a port reset (COMRESET) by writing 1h to the PxSCTL.DET field to invoke a
        // COMRESET on the interface and start a re-establishment of Phy layer communications. Software shall
        // wait at least 1 millisecond before clearing PxSCTL.DET to 0h; this ensures that at least one COMRESET
        // signal is sent over the interface. After clearing PxSCTL.DET to 0h, software should wait for
        // communication to be re-established as indicated by PxSSTS.DET being set to 3h. Then software should
        // write all 1s to the PxSERR register to clear any bits that were set as part of the port reset.

        sctl.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SCTL);
        sctl.DET = 1;
        StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->SCTL, sctl.Status);

        StorPortStallExecution(1000);

        sctl.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SCTL);
        sctl.DET = 0;
        StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->SCTL, sctl.Status);

        // Poll DET to verify if a device is attached to the port
        index = 0;
        do
        {
            StorPortStallExecution(1000);
            ssts.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SSTS);

            index++;
            if (ssts.DET != 0)
            {
                break;
            }
        }
        while(index < 30);
    }

    ssts.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SSTS);
    switch (ssts.DET)
    {
        case 0x3:
            {
                NT_ASSERT(cmd.ST == 0);

                // make sure FIS Recieve is enabled (cmd.FRE)
                index = 0;
                do
                {
                    StorPortStallExecution(10000);
                    cmd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CMD);
                    cmd.FRE = 1;
                    StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->CMD, cmd.Status);
                    index++;
                }
                while((cmd.FR != 1) && (index < 3));

                if (cmd.FR != 1)
                {
                    // failed to start FIS DMA engine
                    // it can crash the driver later
                    // so better to turn this port off
                    return FALSE;
                }

                // start port channel
                // set cmd.ST

                NT_ASSERT(cmd.FRE == 1);
                NT_ASSERT(cmd.CR == 0);

                // why assert? well If we face such condition on DET = 0x3
                // then we don't have port in idle state and hence before executing this part of code
                // we must have restarted it.
                tfd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->TFD);

                if ((tfd.STS.BSY) || (tfd.STS.DRQ))
                {
                    AhciDebugPrint("\tUnhandled Case BSY-DRQ\n");
                }

                // clear pending interrupts
                StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->SERR, (ULONG)~0);
                StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->IS, (ULONG)~0);
                StorPortWriteRegisterUlong(AdapterExtension, AdapterExtension->IS, (1 << PortExtension->PortNumber));

                // set IE
                ie.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->IE);
                /* Device to Host Register FIS Interrupt Enable */
                ie.DHRE = 1;
                /* PIO Setup FIS Interrupt Enable */
                ie.PSE = 1;
                /* DMA Setup FIS Interrupt Enable  */
                ie.DSE = 1;
                /* Set Device Bits FIS Interrupt Enable */
                ie.SDBE = 1;
                /* Unknown FIS Interrupt Enable */
                ie.UFE = 0;
                /* Descriptor Processed Interrupt Enable */
                ie.DPE = 0;
                /* Port Change Interrupt Enable */
                ie.PCE = 1;
                /* Device Mechanical Presence Enable */
                ie.DMPE = 0;
                /* PhyRdy Change Interrupt Enable */
                ie.PRCE = 1;
                /* Incorrect Port Multiplier Enable */
                ie.IPME = 0;
                /* Overflow Enable */
                ie.OFE = 1;
                /* Interface Non-fatal Error Enable */
                ie.INFE = 1;
                /* Interface Fatal Error Enable */
                ie.IFE = 1;
                /* Host Bus Data Error Enable */
                ie.HBDE = 1;
                /* Host Bus Fatal Error Enable */
                ie.HBFE = 1;
                /* Task File Error Enable */
                ie.TFEE = 1;

                cmd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CMD);
                /* Cold Presence Detect Enable */
                if (cmd.CPD) // does it support CPD?
                {
                    // disable it for now
                    ie.CPDE = 0;
                }

                // should I replace this to single line?
                // by directly setting ie.Status?

                StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->IE, ie.Status);

                cmd.ST = 1;
                StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->CMD, cmd.Status);
                cmd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CMD);

                if (cmd.ST != 1)
                {
                    AhciDebugPrint("\tFailed to start Port\n");
                    return FALSE;
                }

                return TRUE;
            }
        default:
            // unhandled case
            AhciDebugPrint("\tDET == %x Unsupported\n", ssts.DET);
            return FALSE;
    }
}// -- AhciStartPort();

/**
 * @name AhciCommandCompletionDpcRoutine
 * @implemented
 *
 * Handles Completed Commands
 *
 * @param Dpc
 * @param AdapterExtension
 * @param SystemArgument1
 * @param SystemArgument2
 */
VOID
AhciCommandCompletionDpcRoutine (
    __in PSTOR_DPC Dpc,
    __in PVOID HwDeviceExtension,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
  )
{
    PSCSI_REQUEST_BLOCK Srb;
    PAHCI_SRB_EXTENSION SrbExtension;
    STOR_LOCK_HANDLE lockhandle = {0};
    PAHCI_COMPLETION_ROUTINE CompletionRoutine;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    PAHCI_PORT_EXTENSION PortExtension;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);

    AhciDebugPrint("AhciCommandCompletionDpcRoutine()\n");

    AdapterExtension = (PAHCI_ADAPTER_EXTENSION)HwDeviceExtension;
    PortExtension = (PAHCI_PORT_EXTENSION)SystemArgument1;

    StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, &lockhandle);
    Srb = RemoveQueue(&PortExtension->CompletionQueue);
    StorPortReleaseSpinLock(AdapterExtension, &lockhandle);

    NT_ASSERT(Srb != NULL);

    if (Srb->SrbStatus == SRB_STATUS_PENDING)
    {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
    }
    else
    {
        return;
    }

    SrbExtension = GetSrbExtension(Srb);

    CompletionRoutine = SrbExtension->CompletionRoutine;
    NT_ASSERT(CompletionRoutine != NULL);

    // now it's completion routine responsibility to set SrbStatus
    CompletionRoutine(PortExtension, Srb);

    StorPortNotification(RequestComplete, AdapterExtension, Srb);

    return;
}// -- AhciCommandCompletionDpcRoutine();

/**
 * @name AhciHwPassiveInitialize
 * @implemented
 *
 * initializes the HBA and finds all devices that are of interest to the miniport driver. (at PASSIVE LEVEL)
 *
 * @param adapterExtension
 *
 * @return
 * return TRUE if intialization was successful
 */
BOOLEAN
AhciHwPassiveInitialize (
    __in PVOID DeviceExtension
    )
{
    ULONG index;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("AhciHwPassiveInitialize()\n");

    AdapterExtension = (PAHCI_ADAPTER_EXTENSION)DeviceExtension;

    for (index = 0; index < AdapterExtension->PortCount; index++)
    {
        if ((AdapterExtension->PortImplemented & (0x1 << index)) != 0)
        {
            PortExtension = &AdapterExtension->PortExtension[index];
            PortExtension->DeviceParams.IsActive = AhciStartPort(PortExtension);
            StorPortInitializeDpc(AdapterExtension, &PortExtension->CommandCompletion, AhciCommandCompletionDpcRoutine);
        }
    }

    return TRUE;
}// -- AhciHwPassiveInitialize();

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
NTAPI
AhciHwInitialize (
    __in PVOID DeviceExtension
    )
{
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    AHCI_GHC ghc;

    AhciDebugPrint("AhciHwInitialize()\n");

    AdapterExtension = (PAHCI_ADAPTER_EXTENSION)DeviceExtension;
    AdapterExtension->StateFlags.MessagePerPort = FALSE;

    // First check what type of interrupt/synchronization device is using
    ghc.Status = StorPortReadRegisterUlong(AdapterExtension, &AdapterExtension->ABAR_Address->GHC);

    // When set to ‘1’ by hardware, indicates that the HBA requested more than one MSI vector
    // but has reverted to using the first vector only.  When this bit is cleared to ‘0’,
    // the HBA has not reverted to single MSI mode (i.e. hardware is already in single MSI mode,
    // software has allocated the number of messages requested
    if (ghc.MRSM == 0)
    {
        AdapterExtension->StateFlags.MessagePerPort = TRUE;
        AhciDebugPrint("\tMultiple MSI based message not supported\n");
    }

    StorPortEnablePassiveInitialization(AdapterExtension, AhciHwPassiveInitialize);

    return TRUE;
}// -- AhciHwInitialize();

/**
 * @name AhciCompleteIssuedSrb
 * @implemented
 *
 * Complete issued Srbs
 *
 * @param PortExtension
 *
 */
VOID
AhciCompleteIssuedSrb (
    __in PAHCI_PORT_EXTENSION PortExtension,
    __in ULONG CommandsToComplete
    )
{
    ULONG NCS, i;
    PSCSI_REQUEST_BLOCK Srb;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciCompleteIssuedSrb()\n");

    NT_ASSERT(CommandsToComplete != 0);

    AhciDebugPrint("\tCompleted Commands: %d\n", CommandsToComplete);

    AdapterExtension = PortExtension->AdapterExtension;
    NCS = AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP);

    for (i = 0; i < NCS; i++)
    {
        if (((1 << i) & CommandsToComplete) != 0)
        {
            Srb = PortExtension->Slot[i];

            if (Srb == NULL)
            {
                continue;
            }

            SrbExtension = GetSrbExtension(Srb);
            NT_ASSERT(SrbExtension != NULL);

            if (SrbExtension->CompletionRoutine != NULL)
            {
                AddQueue(&PortExtension->CompletionQueue, Srb);
                StorPortIssueDpc(AdapterExtension, &PortExtension->CommandCompletion, PortExtension, Srb);
            }
            else
            {
                NT_ASSERT(Srb->SrbStatus == SRB_STATUS_PENDING);
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                StorPortNotification(RequestComplete, AdapterExtension, Srb);
            }
        }
    }

    return;
}// -- AhciCompleteIssuedSrb();

/**
 * @name AhciInterruptHandler
 * @not_implemented
 *
 * Interrupt Handler for PortExtension
 *
 * @param PortExtension
 *
 */
VOID
AhciInterruptHandler (
    __in PAHCI_PORT_EXTENSION PortExtension
    )
{
    ULONG is, ci, sact, outstanding;
    AHCI_INTERRUPT_STATUS PxIS;
    AHCI_INTERRUPT_STATUS PxISMasked;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciInterruptHandler()\n");
    AhciDebugPrint("\tPort Number: %d\n", PortExtension->PortNumber);

    AdapterExtension = PortExtension->AdapterExtension;
    NT_ASSERT(IsPortValid(AdapterExtension, PortExtension->PortNumber));

    // 5.5.3
    // 1. Software determines the cause of the interrupt by reading the PxIS register.
    //    It is possible for multiple bits to be set
    // 2. Software clears appropriate bits in the PxIS register corresponding to the cause of the interrupt.
    // 3. Software clears the interrupt bit in IS.IPS corresponding to the port.
    // 4. If executing non-queued commands, software reads the PxCI register, and compares the current value to
    //    the list of commands previously issued by software that are still outstanding.
    //    If executing native queued commands, software reads the PxSACT register and compares the current
    //    value to the list of commands previously issued by software.
    //    Software completes with success any outstanding command whose corresponding bit has been cleared in
    //    the respective register. PxCI and PxSACT are volatile registers; software should only use their values
    //    to determine commands that have completed, not to determine which commands have previously been issued.
    // 5. If there were errors, noted in the PxIS register, software performs error recovery actions (see section 6.2.2).
    PxISMasked.Status = 0;
    PxIS.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->IS);

    // 6.2.2
    // Fatal Error
    // signified by the setting of PxIS.HBFS, PxIS.HBDS, PxIS.IFS, or PxIS.TFES
    if (PxIS.HBFS || PxIS.HBDS || PxIS.IFS || PxIS.TFES)
    {
        // In this state, the HBA shall not issue any new commands nor acknowledge DMA Setup FISes to process
        // any native command queuing commands. To recover, the port must be restarted
        // To detect an error that requires software recovery actions to be performed,
        // software should check whether any of the following status bits are set on an interrupt:
        // PxIS.HBFS, PxIS.HBDS, PxIS.IFS, and PxIS.TFES.  If any of these bits are set,
        // software should perform the appropriate error recovery actions based on whether
        // non-queued commands were being issued or native command queuing commands were being issued.

        AhciDebugPrint("\tFatal Error: %x\n", PxIS.Status);
    }

    // Normal Command Completion
    // 3.3.5
    // A D2H Register FIS has been received with the ‘I’ bit set, and has been copied into system memory.
    PxISMasked.DHRS = PxIS.DHRS;
    // A PIO Setup FIS has been received with the ‘I’ bit set, it has been copied into system memory.
    PxISMasked.PSS = PxIS.PSS;
    // A DMA Setup FIS has been received with the ‘I’ bit set and has been copied into system memory.
    PxISMasked.DSS = PxIS.DSS;
    // A Set Device Bits FIS has been received with the ‘I’ bit set and has been copied into system memory/
    PxISMasked.SDBS = PxIS.SDBS;
    // A PRD with the ‘I’ bit set has transferred all of its data.
    PxISMasked.DPS = PxIS.DPS;

    if (PxISMasked.Status != 0)
    {
        StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->IS, PxISMasked.Status);
    }

    // 10.7.1.1
    // Clear port interrupt
    // It is set by the level of the virtual interrupt line being a set, and cleared by a write of ‘1’ from the software.
    is = (1 << PortExtension->PortNumber);
    StorPortWriteRegisterUlong(AdapterExtension, AdapterExtension->IS, is);

    ci = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CI);
    sact = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SACT);

    outstanding = ci | sact; // NOTE: Including both non-NCQ and NCQ based commands
    if ((PortExtension->CommandIssuedSlots & (~outstanding)) != 0)
    {
        AhciCompleteIssuedSrb(PortExtension, (PortExtension->CommandIssuedSlots & (~outstanding)));
        PortExtension->CommandIssuedSlots &= outstanding;
    }

    return;
}// -- AhciInterruptHandler();

/**
 * @name AhciHwInterrupt
 * @implemented
 *
 * The Storport driver calls the HwStorInterrupt routine after the HBA generates an interrupt request.
 *
 * @param AdapterExtension
 *
 * @return
 * return TRUE Indicates that an interrupt was pending on adapter.
 * return FALSE Indicates the interrupt was not ours.
 */
BOOLEAN
NTAPI
AhciHwInterrupt (
    __in PVOID DeviceExtension
    )
{
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    ULONG portPending, nextPort, i, portCount;

    AdapterExtension = (PAHCI_ADAPTER_EXTENSION)DeviceExtension;

    if (AdapterExtension->StateFlags.Removed)
    {
        return FALSE;
    }

    portPending = StorPortReadRegisterUlong(AdapterExtension, AdapterExtension->IS);

    // we process interrupt for implemented ports only
    portCount = AdapterExtension->PortCount;
    portPending = portPending & AdapterExtension->PortImplemented;

    if (portPending == 0)
    {
        return FALSE;
    }

    for (i = 1; i <= portCount; i++)
    {
        nextPort = (AdapterExtension->LastInterruptPort + i) % portCount;
        if ((portPending & (0x1 << nextPort)) == 0)
            continue;

        NT_ASSERT(IsPortValid(AdapterExtension, nextPort));

        if (AdapterExtension->PortExtension[nextPort].DeviceParams.IsActive == FALSE)
        {
            continue;
        }

        // we can assign this interrupt to this port
        AdapterExtension->LastInterruptPort = nextPort;
        AhciInterruptHandler(&AdapterExtension->PortExtension[nextPort]);

        portPending &= ~(1 << nextPort);

        // interrupt belongs to this device
        // should always return TRUE
        return TRUE;
    }

    AhciDebugPrint("\tSomething went wrong");
    return FALSE;
}// -- AhciHwInterrupt();

/**
 * @name AhciHwStartIo
 * @not_implemented
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
NTAPI
AhciHwStartIo (
    __in PVOID DeviceExtension,
    __in PSCSI_REQUEST_BLOCK Srb
    )
{
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciHwStartIo()\n");

    AdapterExtension = (PAHCI_ADAPTER_EXTENSION)DeviceExtension;

    if (!IsPortValid(AdapterExtension, Srb->PathId))
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        StorPortNotification(RequestComplete, AdapterExtension, Srb);
        return TRUE;
    }

    switch(Srb->Function)
    {
        case SRB_FUNCTION_PNP:
            {
                // https://learn.microsoft.com/en-us/previous-versions/windows/drivers/storage/handling-srb-function-pnp
                // If the function member of an SRB is set to SRB_FUNCTION_PNP,
                // the SRB is a structure of type SCSI_PNP_REQUEST_BLOCK.

                PSCSI_PNP_REQUEST_BLOCK pnpRequest;
                pnpRequest = (PSCSI_PNP_REQUEST_BLOCK)Srb;
                if ((pnpRequest->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST) != 0)
                {
                    switch(pnpRequest->PnPAction)
                    {
                        case StorRemoveDevice:
                        case StorSurpriseRemoval:
                            {
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                AdapterExtension->StateFlags.Removed = 1;
                                AhciDebugPrint("\tAdapter removed\n");
                            }
                            break;
                        case StorStopDevice:
                            {
                                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                                AhciDebugPrint("\tRequested to Stop the adapter\n");
                            }
                            break;
                        default:
                            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                            break;
                    }
                }
                else
                {
                    Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                }
            }
            break;
        case SRB_FUNCTION_EXECUTE_SCSI:
            {
                // https://learn.microsoft.com/en-us/previous-versions/windows/drivers/storage/handling-srb-function-execute-scsi
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
                PCDB cdb = (PCDB)&Srb->Cdb;
                if (Srb->CdbLength == 0)
                {
                    AhciDebugPrint("\tOperationCode: %d\n", cdb->CDB10.OperationCode);
                    Srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
                    break;
                }

                NT_ASSERT(cdb != NULL);

                switch(cdb->CDB10.OperationCode)
                {
                    case SCSIOP_INQUIRY:
                        Srb->SrbStatus = DeviceInquiryRequest(AdapterExtension, Srb, cdb);
                        break;
                    case SCSIOP_REPORT_LUNS:
                        Srb->SrbStatus = DeviceReportLuns(AdapterExtension, Srb, cdb);
                        break;
                    case SCSIOP_READ_CAPACITY:
                        Srb->SrbStatus = DeviceRequestCapacity(AdapterExtension, Srb, cdb);
                        break;
                    case SCSIOP_TEST_UNIT_READY:
                        Srb->SrbStatus = DeviceRequestComplete(AdapterExtension, Srb, cdb);
                        break;
                    case SCSIOP_MODE_SENSE:
                        Srb->SrbStatus = DeviceRequestSense(AdapterExtension, Srb, cdb);
                        break;
                    case SCSIOP_READ:
                    case SCSIOP_WRITE:
                        Srb->SrbStatus = DeviceRequestReadWrite(AdapterExtension, Srb, cdb);
                        break;
                    default:
                        AhciDebugPrint("\tOperationCode: %d\n", cdb->CDB10.OperationCode);
                        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                        break;
                }
            }
            break;
        default:
            AhciDebugPrint("\tUnknown function code recieved: %x\n", Srb->Function);
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            break;
    }

    if (Srb->SrbStatus != SRB_STATUS_PENDING)
    {
        StorPortNotification(RequestComplete, AdapterExtension, Srb);
    }
    else
    {
        AhciProcessIO(AdapterExtension, Srb->PathId, Srb);
    }
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
NTAPI
AhciHwResetBus (
    __in PVOID AdapterExtension,
    __in ULONG PathId
    )
{
    STOR_LOCK_HANDLE lockhandle = {0};
//    PAHCI_ADAPTER_EXTENSION adapterExtension;

    AhciDebugPrint("AhciHwResetBus()\n");

//    adapterExtension = AdapterExtension;

    if (IsPortValid(AdapterExtension, PathId))
    {
        // Acquire Lock
        StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, &lockhandle);

        // TODO: Perform port reset

        // Release lock
        StorPortReleaseSpinLock(AdapterExtension, &lockhandle);
    }

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
NTAPI
AhciHwFindAdapter (
    __in PVOID DeviceExtension,
    __in PVOID HwContext,
    __in PVOID BusInformation,
    __in PCHAR ArgumentString,
    __inout PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    __in PBOOLEAN Reserved3
    )
{
    AHCI_GHC ghc;
    ULONG index, pci_cfg_len;
    PACCESS_RANGE accessRange;
    UCHAR pci_cfg_buf[sizeof(PCI_COMMON_CONFIG)];

    PAHCI_MEMORY_REGISTERS abar;
    PPCI_COMMON_CONFIG pciConfigData;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    AhciDebugPrint("AhciHwFindAdapter()\n");

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ArgumentString);
    UNREFERENCED_PARAMETER(Reserved3);

    adapterExtension = DeviceExtension;
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
        AhciDebugPrint("\tpci_cfg_len != %d :: %d", sizeof(PCI_COMMON_CONFIG), pci_cfg_len);
        return SP_RETURN_ERROR;//Not a valid device at the given bus number
    }

    pciConfigData = (PPCI_COMMON_CONFIG)pci_cfg_buf;
    adapterExtension->VendorID = pciConfigData->VendorID;
    adapterExtension->DeviceID = pciConfigData->DeviceID;
    adapterExtension->RevisionID = pciConfigData->RevisionID;
    // The last PCI base address register (BAR[5], header offset 0x24) points to the AHCI base memory, it’s called ABAR (AHCI Base Memory Register).
    adapterExtension->AhciBaseAddress = pciConfigData->u.type0.BaseAddresses[5] & (0xFFFFFFF0);

    AhciDebugPrint("\tVendorID: %04x  DeviceID: %04x  RevisionID: %02x\n",
                   adapterExtension->VendorID,
                   adapterExtension->DeviceID,
                   adapterExtension->RevisionID);

    // 2.1.11
    abar = NULL;
    if (ConfigInfo->NumberOfAccessRanges > 0)
    {
        accessRange = *(ConfigInfo->AccessRanges);
        for (index = 0; index < ConfigInfo->NumberOfAccessRanges; index++)
        {
            if (accessRange[index].RangeStart.QuadPart == adapterExtension->AhciBaseAddress)
            {
                abar = StorPortGetDeviceBase(adapterExtension,
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
        AhciDebugPrint("\tabar == NULL\n");
        return SP_RETURN_ERROR; // corrupted information supplied
    }

    adapterExtension->ABAR_Address = abar;
    adapterExtension->CAP = StorPortReadRegisterUlong(adapterExtension, &abar->CAP);
    adapterExtension->CAP2 = StorPortReadRegisterUlong(adapterExtension, &abar->CAP2);
    adapterExtension->Version = StorPortReadRegisterUlong(adapterExtension, &abar->VS);
    adapterExtension->LastInterruptPort = (ULONG)-1;

    // 10.1.2
    // 1. Indicate that system software is AHCI aware by setting GHC.AE to ‘1’.
    // 3.1.2 -- AE bit is read-write only if CAP.SAM is '0'
    ghc.Status = StorPortReadRegisterUlong(adapterExtension, &abar->GHC);
    // AE := Highest Significant bit of GHC
    if (ghc.AE != 0)// Hmm, controller was already in power state
    {
        // reset controller to have it in known state
        AhciDebugPrint("\tAE Already set, Reset()\n");
        if (!AhciAdapterReset(adapterExtension))
        {
            AhciDebugPrint("\tReset Failed!\n");
            return SP_RETURN_ERROR;// reset failed
        }
    }

    ghc.Status = 0;
    ghc.AE = 1;// only AE=1
    // tell the controller that we know about AHCI
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc.Status);

    adapterExtension->IS = &abar->IS;
    adapterExtension->PortImplemented = StorPortReadRegisterUlong(adapterExtension, &abar->PI);

    if (adapterExtension->PortImplemented == 0)
    {
        AhciDebugPrint("\tadapterExtension->PortImplemented == 0\n");
        return SP_RETURN_ERROR;
    }

    ConfigInfo->Master = TRUE;
    ConfigInfo->AlignmentMask = 0x3;
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->DmaWidth = Width32Bits;
    ConfigInfo->WmiDataProvider = FALSE;
    ConfigInfo->Dma32BitAddresses = TRUE;

    if (IsAdapterCAPS64(adapterExtension->CAP))
    {
        ConfigInfo->Dma64BitAddresses = TRUE;
    }

    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->NumberOfPhysicalBreaks = 0x21;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    ConfigInfo->NumberOfBuses = MAXIMUM_AHCI_PORT_COUNT;
    ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;

    // Turn IE -- Interrupt Enabled
    ghc.Status = StorPortReadRegisterUlong(adapterExtension, &abar->GHC);
    ghc.IE = 1;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc.Status);

    // allocate necessary resource for each port
    if (!AhciAllocateResourceForAdapter(adapterExtension, ConfigInfo))
    {
        NT_ASSERT(FALSE);
        return SP_RETURN_ERROR;
    }

    for (index = 0; index < adapterExtension->PortCount; index++)
    {
        if ((adapterExtension->PortImplemented & (0x1 << index)) != 0)
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
NTAPI
DriverEntry (
    __in PVOID DriverObject,
    __in PVOID RegistryPath
    )
{
    ULONG status;
    // initialize the hardware data structure
    HW_INITIALIZATION_DATA hwInitializationData = {0};

    // set size of hardware initialization structure
    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // identity required miniport entry point routines
    hwInitializationData.HwStartIo = AhciHwStartIo;
    hwInitializationData.HwResetBus = AhciHwResetBus;
    hwInitializationData.HwInterrupt = AhciHwInterrupt;
    hwInitializationData.HwInitialize = AhciHwInitialize;
    hwInitializationData.HwFindAdapter = AhciHwFindAdapter;

    // adapter specific information
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;
    hwInitializationData.NeedPhysicalAddresses = TRUE;

    hwInitializationData.NumberOfAccessRanges = 6;
    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;

    // set required extension sizes
    hwInitializationData.SrbExtensionSize = sizeof(AHCI_SRB_EXTENSION);
    hwInitializationData.DeviceExtensionSize = sizeof(AHCI_ADAPTER_EXTENSION);

    // register our hw init data
    status = StorPortInitialize(DriverObject,
                                RegistryPath,
                                &hwInitializationData,
                                NULL);

    NT_ASSERT(status == STATUS_SUCCESS);
    return status;
}// -- DriverEntry();

/**
 * @name AhciATA_CFIS
 * @implemented
 *
 * create ATA CFIS from Srb
 *
 * @param PortExtension
 * @param Srb
 *
 * @return
 * Number of CFIS fields used in DWORD
 */
ULONG
AhciATA_CFIS (
    __in PAHCI_PORT_EXTENSION PortExtension,
    __in PAHCI_SRB_EXTENSION SrbExtension
    )
{
    PAHCI_COMMAND_TABLE cmdTable;

    UNREFERENCED_PARAMETER(PortExtension);

    AhciDebugPrint("AhciATA_CFIS()\n");

    cmdTable = (PAHCI_COMMAND_TABLE)SrbExtension;

    AhciZeroMemory((PCHAR)cmdTable->CFIS, sizeof(cmdTable->CFIS));

    cmdTable->CFIS[AHCI_ATA_CFIS_FisType] = FIS_TYPE_REG_H2D;       // FIS Type
    cmdTable->CFIS[AHCI_ATA_CFIS_PMPort_C] = (1 << 7);              // PM Port & C
    cmdTable->CFIS[AHCI_ATA_CFIS_CommandReg] = SrbExtension->CommandReg;

    cmdTable->CFIS[AHCI_ATA_CFIS_FeaturesLow] = SrbExtension->FeaturesLow;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA0] = SrbExtension->LBA0;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA1] = SrbExtension->LBA1;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA2] = SrbExtension->LBA2;
    cmdTable->CFIS[AHCI_ATA_CFIS_Device] = SrbExtension->Device;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA3] = SrbExtension->LBA3;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA4] = SrbExtension->LBA4;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA5] = SrbExtension->LBA5;
    cmdTable->CFIS[AHCI_ATA_CFIS_FeaturesHigh] = SrbExtension->FeaturesHigh;
    cmdTable->CFIS[AHCI_ATA_CFIS_SectorCountLow] = SrbExtension->SectorCountLow;
    cmdTable->CFIS[AHCI_ATA_CFIS_SectorCountHigh] = SrbExtension->SectorCountHigh;

    return 5;
}// -- AhciATA_CFIS();

/**
 * @name AhciATAPI_CFIS
 * @not_implemented
 *
 * create ATAPI CFIS from Srb
 *
 * @param PortExtension
 * @param Srb
 *
 * @return
 * Number of CFIS fields used in DWORD
 */
ULONG
AhciATAPI_CFIS (
    __in PAHCI_PORT_EXTENSION PortExtension,
    __in PAHCI_SRB_EXTENSION SrbExtension
    )
{
    PAHCI_COMMAND_TABLE cmdTable;
    UNREFERENCED_PARAMETER(PortExtension);

    AhciDebugPrint("AhciATAPI_CFIS()\n");

    cmdTable = (PAHCI_COMMAND_TABLE)SrbExtension;

    NT_ASSERT(SrbExtension->CommandReg == IDE_COMMAND_ATAPI_PACKET);

    AhciZeroMemory((PCHAR)cmdTable->CFIS, sizeof(cmdTable->CFIS));

    cmdTable->CFIS[AHCI_ATA_CFIS_FisType] = FIS_TYPE_REG_H2D;       // FIS Type
    cmdTable->CFIS[AHCI_ATA_CFIS_PMPort_C] = (1 << 7);              // PM Port & C
    cmdTable->CFIS[AHCI_ATA_CFIS_CommandReg] = SrbExtension->CommandReg;

    cmdTable->CFIS[AHCI_ATA_CFIS_FeaturesLow] = SrbExtension->FeaturesLow;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA0] = SrbExtension->LBA0;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA1] = SrbExtension->LBA1;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA2] = SrbExtension->LBA2;
    cmdTable->CFIS[AHCI_ATA_CFIS_Device] = SrbExtension->Device;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA3] = SrbExtension->LBA3;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA4] = SrbExtension->LBA4;
    cmdTable->CFIS[AHCI_ATA_CFIS_LBA5] = SrbExtension->LBA5;
    cmdTable->CFIS[AHCI_ATA_CFIS_FeaturesHigh] = SrbExtension->FeaturesHigh;
    cmdTable->CFIS[AHCI_ATA_CFIS_SectorCountLow] = SrbExtension->SectorCountLow;
    cmdTable->CFIS[AHCI_ATA_CFIS_SectorCountHigh] = SrbExtension->SectorCountHigh;

    return 5;
}// -- AhciATAPI_CFIS();

/**
 * @name AhciBuild_PRDT
 * @implemented
 *
 * Build PRDT for data transfer
 *
 * @param PortExtension
 * @param Srb
 *
 * @return
 * Return number of entries in PRDT.
 */
ULONG
AhciBuild_PRDT (
    __in PAHCI_PORT_EXTENSION PortExtension,
    __in PAHCI_SRB_EXTENSION SrbExtension
    )
{
    ULONG index;
    PAHCI_COMMAND_TABLE cmdTable;
    PLOCAL_SCATTER_GATHER_LIST sgl;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciBuild_PRDT()\n");

    sgl = SrbExtension->pSgl;
    cmdTable = (PAHCI_COMMAND_TABLE)SrbExtension;
    AdapterExtension = PortExtension->AdapterExtension;

    NT_ASSERT(sgl != NULL);
    NT_ASSERT(sgl->NumberOfElements < MAXIMUM_AHCI_PRDT_ENTRIES);

    for (index = 0; index < sgl->NumberOfElements; index++)
    {
        NT_ASSERT(sgl->List[index].Length <= MAXIMUM_TRANSFER_LENGTH);

        cmdTable->PRDT[index].DBA = sgl->List[index].PhysicalAddress.LowPart;
        if (IsAdapterCAPS64(AdapterExtension->CAP))
        {
            cmdTable->PRDT[index].DBAU = sgl->List[index].PhysicalAddress.HighPart;
        }

        // Data Byte Count (DBC): A ‘0’ based value that Indicates the length, in bytes, of the data block.
        // A maximum of length of 4MB may exist for any entry. Bit ‘0’ of this field must always be ‘1’ to
        // indicate an even byte count. A value of ‘1’ indicates 2 bytes, ‘3’ indicates 4 bytes, etc.
        cmdTable->PRDT[index].DBC = sgl->List[index].Length - 1;
    }

    return sgl->NumberOfElements;
}// -- AhciBuild_PRDT();

/**
 * @name AhciProcessSrb
 * @implemented
 *
 * Prepare Srb for IO processing
 *
 * @param PortExtension
 * @param Srb
 * @param SlotIndex
 *
 */
VOID
AhciProcessSrb (
    __in PAHCI_PORT_EXTENSION PortExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in ULONG SlotIndex
    )
{
    ULONG prdtlen, sig, length, cfl;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_COMMAND_HEADER CommandHeader;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    STOR_PHYSICAL_ADDRESS CommandTablePhysicalAddress;

    AhciDebugPrint("AhciProcessSrb()\n");

    NT_ASSERT(Srb->PathId == PortExtension->PortNumber);

    SrbExtension = GetSrbExtension(Srb);
    AdapterExtension = PortExtension->AdapterExtension;

    NT_ASSERT(SrbExtension != NULL);
    NT_ASSERT(SrbExtension->AtaFunction != 0);

    if ((SrbExtension->AtaFunction == ATA_FUNCTION_ATA_IDENTIFY) &&
        (SrbExtension->CommandReg == IDE_COMMAND_NOT_VALID))
    {
        // Here we are safe to check SIG register
        sig = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->SIG);
        if (sig == 0x101)
        {
            AhciDebugPrint("\tATA Device Found!\n");
            SrbExtension->CommandReg = IDE_COMMAND_IDENTIFY;
        }
        else
        {
            AhciDebugPrint("\tATAPI Device Found!\n");
            SrbExtension->CommandReg = IDE_COMMAND_ATAPI_IDENTIFY;
        }
    }

    NT_ASSERT(SlotIndex < AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP));
    SrbExtension->SlotIndex = SlotIndex;

    // program the CFIS in the CommandTable
    CommandHeader = &PortExtension->CommandList[SlotIndex];

    cfl = 0;
    if (IsAtapiCommand(SrbExtension->AtaFunction))
    {
        cfl = AhciATAPI_CFIS(PortExtension, SrbExtension);
    }
    else if (IsAtaCommand(SrbExtension->AtaFunction))
    {
        cfl = AhciATA_CFIS(PortExtension, SrbExtension);
    }
    else
    {
        NT_ASSERT(FALSE);
    }

    prdtlen = 0;
    if (IsDataTransferNeeded(SrbExtension))
    {
        prdtlen = AhciBuild_PRDT(PortExtension, SrbExtension);
        NT_ASSERT(prdtlen != -1);
    }

    // Program the command header
    CommandHeader->DI.PRDTL = prdtlen; // number of entries in PRD table
    CommandHeader->DI.CFL = cfl;
    CommandHeader->DI.A = (SrbExtension->AtaFunction & ATA_FUNCTION_ATAPI_COMMAND) ? 1 : 0;
    CommandHeader->DI.W = (SrbExtension->Flags & ATA_FLAGS_DATA_OUT) ? 1 : 0;
    CommandHeader->DI.P = 0;    // ATA Specifications says so
    CommandHeader->DI.PMP = 0;  // Port Multiplier

    // Reset -- Manual Configuation
    CommandHeader->DI.R = 0;
    CommandHeader->DI.B = 0;
    CommandHeader->DI.C = 0;

    CommandHeader->PRDBC = 0;

    CommandHeader->Reserved[0] = 0;
    CommandHeader->Reserved[1] = 0;
    CommandHeader->Reserved[2] = 0;
    CommandHeader->Reserved[3] = 0;

    // set CommandHeader CTBA
    CommandTablePhysicalAddress = StorPortGetPhysicalAddress(AdapterExtension,
                                                             NULL,
                                                             SrbExtension,
                                                             &length);

    NT_ASSERT(length != 0);

    // command table alignment
    NT_ASSERT((CommandTablePhysicalAddress.LowPart % 128) == 0);

    CommandHeader->CTBA = CommandTablePhysicalAddress.LowPart;

    if (IsAdapterCAPS64(AdapterExtension->CAP))
    {
        CommandHeader->CTBA_U = CommandTablePhysicalAddress.HighPart;
    }

    // mark this slot
    PortExtension->Slot[SlotIndex] = Srb;
    PortExtension->QueueSlots |= 1 << SlotIndex;
    return;
}// -- AhciProcessSrb();

/**
 * @name AhciActivatePort
 * @implemented
 *
 * Program Port and populate command list
 *
 * @param PortExtension
 *
 */

#ifdef _MSC_VER     // avoid MSVC C4700
    #pragma warning(push)
    #pragma warning(disable: 4700)
#endif

VOID
AhciActivatePort (
    __in PAHCI_PORT_EXTENSION PortExtension
    )
{
    AHCI_PORT_CMD cmd;
    ULONG QueueSlots, slotToActivate, tmp;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;

    AhciDebugPrint("AhciActivatePort()\n");

    AdapterExtension = PortExtension->AdapterExtension;
    QueueSlots = PortExtension->QueueSlots;

    if (QueueSlots == 0)
    {
        return;
    }

    // section 3.3.14
    // Bits in this field shall only be set to ‘1’ by software when PxCMD.ST is set to ‘1’
    cmd.Status = StorPortReadRegisterUlong(AdapterExtension, &PortExtension->Port->CMD);

    if (cmd.ST == 0) // PxCMD.ST == 0
    {
        return;
    }

    // get the lowest set bit
    tmp = QueueSlots & (QueueSlots - 1);

    if (tmp == 0)
        slotToActivate = QueueSlots;
    else
        slotToActivate = (QueueSlots & (~tmp));

    // mark that bit off in QueueSlots
    // so we can know we it is really needed to activate port or not
    PortExtension->QueueSlots &= ~slotToActivate;
    // mark this CommandIssuedSlots
    // to validate in completeIssuedCommand
    PortExtension->CommandIssuedSlots |= slotToActivate;

    // tell the HBA to issue this Command Slot to the given port
    StorPortWriteRegisterUlong(AdapterExtension, &PortExtension->Port->CI, slotToActivate);

    return;
}// -- AhciActivatePort();

#ifdef _MSC_VER     // avoid MSVC C4700
    #pragma warning(pop)
#endif

/**
 * @name AhciProcessIO
 * @implemented
 *
 * Acquire Exclusive lock to port, populate pending commands to command List
 * program controller's port to process new commands in command list.
 *
 * @param AdapterExtension
 * @param PathId
 * @param Srb
 *
 */
VOID
AhciProcessIO (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in UCHAR PathId,
    __in PSCSI_REQUEST_BLOCK Srb
    )
{
    PSCSI_REQUEST_BLOCK tmpSrb;
    STOR_LOCK_HANDLE lockhandle = {0};
    PAHCI_PORT_EXTENSION PortExtension;
    ULONG commandSlotMask, occupiedSlots, slotIndex, NCS;

    AhciDebugPrint("AhciProcessIO()\n");
    AhciDebugPrint("\tPathId: %d\n", PathId);

    PortExtension = &AdapterExtension->PortExtension[PathId];

    NT_ASSERT(PathId < AdapterExtension->PortCount);

    // Acquire Lock
    StorPortAcquireSpinLock(AdapterExtension, InterruptLock, NULL, &lockhandle);

    // add Srb to queue
    AddQueue(&PortExtension->SrbQueue, Srb);

    if (PortExtension->DeviceParams.IsActive == FALSE)
    {
        // Release Lock
        StorPortReleaseSpinLock(AdapterExtension, &lockhandle);
        return; // we should wait for device to get active
    }

    occupiedSlots = (PortExtension->QueueSlots | PortExtension->CommandIssuedSlots); // Busy command slots for given port
    NCS = AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP);
    commandSlotMask = (1 << NCS) - 1; // available slots mask

    commandSlotMask = (commandSlotMask & ~occupiedSlots);
    if(commandSlotMask != 0)
    {
        // iterate over HBA port slots
        for (slotIndex = 0; slotIndex < NCS; slotIndex++)
        {
            // find first free slot
            if ((commandSlotMask & (1 << slotIndex)) != 0)
            {
                tmpSrb = RemoveQueue(&PortExtension->SrbQueue);
                if (tmpSrb != NULL)
                {
                    NT_ASSERT(tmpSrb->PathId == PathId);
                    AhciProcessSrb(PortExtension, tmpSrb, slotIndex);
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    // program HBA port
    AhciActivatePort(PortExtension);

    // Release Lock
    StorPortReleaseSpinLock(AdapterExtension, &lockhandle);

    return;
}// -- AhciProcessIO();

/**
 * @name AtapiInquiryCompletion
 * @implemented
 *
 * AtapiInquiryCompletion routine should be called after device signals
 * for device inquiry request is completed (through interrupt) -- ATAPI Device only
 *
 * @param PortExtension
 * @param Srb
 *
 */
VOID
AtapiInquiryCompletion (
    __in PVOID _Extension,
    __in PVOID _Srb
    )
{
    PAHCI_PORT_EXTENSION PortExtension;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    PSCSI_REQUEST_BLOCK Srb;
    BOOLEAN status;

    AhciDebugPrint("AtapiInquiryCompletion()\n");

    PortExtension = (PAHCI_PORT_EXTENSION)_Extension;
    Srb = (PSCSI_REQUEST_BLOCK)_Srb;

    NT_ASSERT(Srb != NULL);
    NT_ASSERT(PortExtension != NULL);

    AdapterExtension = PortExtension->AdapterExtension;

    // send queue depth
    status = StorPortSetDeviceQueueDepth(PortExtension->AdapterExtension,
                                         Srb->PathId,
                                         Srb->TargetId,
                                         Srb->Lun,
                                         AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP));

    NT_ASSERT(status == TRUE);
    return;
}// -- AtapiInquiryCompletion();

/**
 * @name InquiryCompletion
 * @implemented
 *
 * InquiryCompletion routine should be called after device signals
 * for device inquiry request is completed (through interrupt)
 *
 * @param PortExtension
 * @param Srb
 *
 */
VOID
InquiryCompletion (
    __in PVOID _Extension,
    __in PVOID _Srb
    )
{
    PAHCI_PORT_EXTENSION PortExtension;
    PSCSI_REQUEST_BLOCK Srb;

//    PCDB cdb;
    BOOLEAN status;
    PINQUIRYDATA InquiryData;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_ADAPTER_EXTENSION AdapterExtension;
    PIDENTIFY_DEVICE_DATA IdentifyDeviceData;

    AhciDebugPrint("InquiryCompletion()\n");

    PortExtension = (PAHCI_PORT_EXTENSION)_Extension;
    Srb = (PSCSI_REQUEST_BLOCK)_Srb;

    NT_ASSERT(Srb != NULL);
    NT_ASSERT(PortExtension != NULL);

//    cdb = (PCDB)&Srb->Cdb;
    InquiryData = Srb->DataBuffer;
    SrbExtension = GetSrbExtension(Srb);
    AdapterExtension = PortExtension->AdapterExtension;
    IdentifyDeviceData = PortExtension->IdentifyDeviceData;

    if (Srb->SrbStatus != SRB_STATUS_SUCCESS)
    {
        if (Srb->SrbStatus == SRB_STATUS_NO_DEVICE)
        {
            PortExtension->DeviceParams.DeviceType = AHCI_DEVICE_TYPE_NODEVICE;
        }
        return;
    }

    NT_ASSERT(InquiryData != NULL);
    NT_ASSERT(Srb->SrbStatus == SRB_STATUS_SUCCESS);

    // Device specific data
    PortExtension->DeviceParams.MaxLba.QuadPart = 0;

    if (SrbExtension->CommandReg == IDE_COMMAND_IDENTIFY)
    {
        PortExtension->DeviceParams.DeviceType = AHCI_DEVICE_TYPE_ATA;
        if (IdentifyDeviceData->GeneralConfiguration.RemovableMedia)
        {
            PortExtension->DeviceParams.RemovableDevice = 1;
        }

        if ((IdentifyDeviceData->CommandSetSupport.BigLba) && (IdentifyDeviceData->CommandSetActive.BigLba))
        {
            PortExtension->DeviceParams.Lba48BitMode = 1;
        }

        PortExtension->DeviceParams.AccessType = DIRECT_ACCESS_DEVICE;

        /* Device max address lba */
        if (PortExtension->DeviceParams.Lba48BitMode)
        {
            PortExtension->DeviceParams.MaxLba.LowPart = IdentifyDeviceData->Max48BitLBA[0];
            PortExtension->DeviceParams.MaxLba.HighPart = IdentifyDeviceData->Max48BitLBA[1];
        }
        else
        {
            PortExtension->DeviceParams.MaxLba.LowPart = IdentifyDeviceData->UserAddressableSectors;
        }

        /* Bytes Per Logical Sector */
        if (IdentifyDeviceData->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words)
        {
            AhciDebugPrint("\tBytesPerLogicalSector != DEVICE_ATA_BLOCK_SIZE\n");
            NT_ASSERT(FALSE);
        }

        PortExtension->DeviceParams.BytesPerLogicalSector = DEVICE_ATA_BLOCK_SIZE;

        /* Bytes Per Physical Sector */
        if (IdentifyDeviceData->PhysicalLogicalSectorSize.MultipleLogicalSectorsPerPhysicalSector)
        {
            AhciDebugPrint("\tBytesPerPhysicalSector != DEVICE_ATA_BLOCK_SIZE\n");
            NT_ASSERT(FALSE);
        }

        PortExtension->DeviceParams.BytesPerPhysicalSector = DEVICE_ATA_BLOCK_SIZE;

        // last byte should be NULL
        StorPortCopyMemory(PortExtension->DeviceParams.VendorId, IdentifyDeviceData->ModelNumber, sizeof(PortExtension->DeviceParams.VendorId) - 1);
        StorPortCopyMemory(PortExtension->DeviceParams.RevisionID, IdentifyDeviceData->FirmwareRevision, sizeof(PortExtension->DeviceParams.RevisionID) - 1);
        StorPortCopyMemory(PortExtension->DeviceParams.SerialNumber, IdentifyDeviceData->SerialNumber, sizeof(PortExtension->DeviceParams.SerialNumber) - 1);

        PortExtension->DeviceParams.VendorId[sizeof(PortExtension->DeviceParams.VendorId) - 1] = '\0';
        PortExtension->DeviceParams.RevisionID[sizeof(PortExtension->DeviceParams.RevisionID) - 1] = '\0';
        PortExtension->DeviceParams.SerialNumber[sizeof(PortExtension->DeviceParams.SerialNumber) - 1] = '\0';

        // TODO: Add other device params
        AhciDebugPrint("\tATA Device\n");
    }
    else
    {
        AhciDebugPrint("\tATAPI Device\n");
        PortExtension->DeviceParams.DeviceType = AHCI_DEVICE_TYPE_ATAPI;
        PortExtension->DeviceParams.AccessType = READ_ONLY_DIRECT_ACCESS_DEVICE;
    }

    // INQUIRYDATABUFFERSIZE = 36 ; Defined in storport.h
    if (Srb->DataTransferLength < INQUIRYDATABUFFERSIZE)
    {
        AhciDebugPrint("\tDataBufferLength < sizeof(INQUIRYDATA), Could crash the driver.\n");
        NT_ASSERT(FALSE);
    }

    // update data transfer length
    Srb->DataTransferLength = INQUIRYDATABUFFERSIZE;

    // prepare data to send
    InquiryData->Versions = 2;
    InquiryData->Wide32Bit = 1;
    InquiryData->CommandQueue = 0; // NCQ not supported
    InquiryData->ResponseDataFormat = 0x2;
    InquiryData->DeviceTypeModifier = 0;
    InquiryData->DeviceTypeQualifier = DEVICE_CONNECTED;
    InquiryData->AdditionalLength = INQUIRYDATABUFFERSIZE - 5;
    InquiryData->DeviceType = PortExtension->DeviceParams.AccessType;
    InquiryData->RemovableMedia = PortExtension->DeviceParams.RemovableDevice;

    // Fill VendorID, Product Revision Level and other string fields
    StorPortCopyMemory(InquiryData->VendorId, PortExtension->DeviceParams.VendorId, sizeof(InquiryData->VendorId) - 1);
    StorPortCopyMemory(InquiryData->ProductId, PortExtension->DeviceParams.RevisionID, sizeof(PortExtension->DeviceParams.RevisionID));
    StorPortCopyMemory(InquiryData->ProductRevisionLevel, PortExtension->DeviceParams.SerialNumber, sizeof(InquiryData->ProductRevisionLevel) - 1);

    InquiryData->VendorId[sizeof(InquiryData->VendorId) - 1] = '\0';
    InquiryData->ProductId[sizeof(InquiryData->ProductId) - 1] = '\0';
    InquiryData->ProductRevisionLevel[sizeof(InquiryData->ProductRevisionLevel) - 1] = '\0';

    // send queue depth
    status = StorPortSetDeviceQueueDepth(PortExtension->AdapterExtension,
                                         Srb->PathId,
                                         Srb->TargetId,
                                         Srb->Lun,
                                         AHCI_Global_Port_CAP_NCS(AdapterExtension->CAP));

    NT_ASSERT(status == TRUE);
    return;
}// -- InquiryCompletion();

 /**
 * @name AhciATAPICommand
 * @implemented
 *
 * Handles ATAPI Requests commands
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for AhciATAPICommand
 */
UCHAR
AhciATAPICommand (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    ULONG SrbFlags, DataBufferLength;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("AhciATAPICommand()\n");

    SrbFlags = Srb->SrbFlags;
    SrbExtension = GetSrbExtension(Srb);
    DataBufferLength = Srb->DataTransferLength;
    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    NT_ASSERT(PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI);

    NT_ASSERT(SrbExtension != NULL);

    SrbExtension->AtaFunction = ATA_FUNCTION_ATAPI_COMMAND;
    SrbExtension->Flags = 0;

    if (SrbFlags & SRB_FLAGS_DATA_IN)
    {
        SrbExtension->Flags |= ATA_FLAGS_DATA_IN;
    }

    if (SrbFlags & SRB_FLAGS_DATA_OUT)
    {
        SrbExtension->Flags |= ATA_FLAGS_DATA_OUT;
    }

    SrbExtension->FeaturesLow = 0;

    SrbExtension->CompletionRoutine = NULL;

    NT_ASSERT(Cdb != NULL);
    switch(Cdb->CDB10.OperationCode)
    {
        case SCSIOP_INQUIRY:
            SrbExtension->Flags |= ATA_FLAGS_DATA_IN;
            SrbExtension->CompletionRoutine = AtapiInquiryCompletion;
            break;
        case SCSIOP_READ:
            SrbExtension->Flags |= ATA_FLAGS_USE_DMA;
            SrbExtension->FeaturesLow = 0x5;
            break;
        case SCSIOP_WRITE:
            SrbExtension->Flags |= ATA_FLAGS_USE_DMA;
            SrbExtension->FeaturesLow = 0x1;
            break;
    }

    SrbExtension->CommandReg = IDE_COMMAND_ATAPI_PACKET;

    SrbExtension->LBA0 = 0;
    SrbExtension->LBA1 = (UCHAR)(DataBufferLength >> 0);
    SrbExtension->LBA2 = (UCHAR)(DataBufferLength >> 8);
    SrbExtension->Device = 0;
    SrbExtension->LBA3 = 0;
    SrbExtension->LBA4 = 0;
    SrbExtension->LBA5 = 0;
    SrbExtension->FeaturesHigh = 0;
    SrbExtension->SectorCountLow = 0;
    SrbExtension->SectorCountHigh = 0;

    if ((SrbExtension->Flags & ATA_FLAGS_DATA_IN) || (SrbExtension->Flags & ATA_FLAGS_DATA_OUT))
    {
        SrbExtension->pSgl = (PLOCAL_SCATTER_GATHER_LIST)StorPortGetScatterGatherList(AdapterExtension, Srb);
    }

    return SRB_STATUS_PENDING;
}// -- AhciATAPICommand();

/**
 * @name DeviceRequestSense
 * @implemented
 *
 * Handle SCSIOP_MODE_SENSE OperationCode
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceRequestSense
 */
UCHAR
DeviceRequestSense (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    PMODE_PARAMETER_HEADER ModeHeader;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("DeviceRequestSense()\n");

    NT_ASSERT(IsPortValid(AdapterExtension, Srb->PathId));
    NT_ASSERT(Cdb->CDB10.OperationCode == SCSIOP_MODE_SENSE);

    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    if (PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI)
    {
        return AhciATAPICommand(AdapterExtension, Srb, Cdb);
    }

    ModeHeader = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;

    NT_ASSERT(ModeHeader != NULL);

    AhciZeroMemory((PCHAR)ModeHeader, Srb->DataTransferLength);

    ModeHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER);
    ModeHeader->MediumType = 0;
    ModeHeader->DeviceSpecificParameter = 0;
    ModeHeader->BlockDescriptorLength = 0;

    if (Cdb->MODE_SENSE.PageCode == MODE_SENSE_CURRENT_VALUES)
    {
        ModeHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PARAMETER_BLOCK);
        ModeHeader->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);
    }

    return SRB_STATUS_SUCCESS;
}// -- DeviceRequestSense();

/**
 * @name DeviceRequestReadWrite
 * @implemented
 *
 * Handle SCSIOP_READ SCSIOP_WRITE OperationCode
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceRequestReadWrite
 */
UCHAR
DeviceRequestReadWrite (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    BOOLEAN IsReading;
    ULONG64 StartOffset;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_PORT_EXTENSION PortExtension;
    ULONG DataTransferLength, BytesPerSector, SectorCount;

    AhciDebugPrint("DeviceRequestReadWrite()\n");

    NT_ASSERT(IsPortValid(AdapterExtension, Srb->PathId));
    NT_ASSERT((Cdb->CDB10.OperationCode == SCSIOP_READ) || (Cdb->CDB10.OperationCode == SCSIOP_WRITE));

    SrbExtension = GetSrbExtension(Srb);
    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    if (PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI)
    {
        return AhciATAPICommand(AdapterExtension, Srb, Cdb);
    }

    DataTransferLength = Srb->DataTransferLength;
    BytesPerSector = PortExtension->DeviceParams.BytesPerLogicalSector;

    NT_ASSERT(BytesPerSector > 0);

    //ROUND_UP(DataTransferLength, BytesPerSector);

    SectorCount = DataTransferLength / BytesPerSector;

    Srb->DataTransferLength = SectorCount * BytesPerSector;

    StartOffset = AhciGetLba(Cdb, Srb->CdbLength);
    IsReading = (Cdb->CDB10.OperationCode == SCSIOP_READ);

    NT_ASSERT(SectorCount > 0);

    SrbExtension->AtaFunction = ATA_FUNCTION_ATA_READ;
    SrbExtension->Flags |= ATA_FLAGS_USE_DMA;
    SrbExtension->CompletionRoutine = NULL;

    if (IsReading)
    {
        SrbExtension->Flags |= ATA_FLAGS_DATA_IN;
        SrbExtension->CommandReg = IDE_COMMAND_READ_DMA;
    }
    else
    {
        SrbExtension->Flags |= ATA_FLAGS_DATA_OUT;
        SrbExtension->CommandReg = IDE_COMMAND_WRITE_DMA;
    }

    SrbExtension->FeaturesLow = 0;
    SrbExtension->LBA0 = (StartOffset >> 0) & 0xFF;
    SrbExtension->LBA1 = (StartOffset >> 8) & 0xFF;
    SrbExtension->LBA2 = (StartOffset >> 16) & 0xFF;

    SrbExtension->Device = (0xA0 | IDE_LBA_MODE);

    if (PortExtension->DeviceParams.Lba48BitMode)
    {
        SrbExtension->Flags |= ATA_FLAGS_48BIT_COMMAND;

        if (IsReading)
        {
            SrbExtension->CommandReg = IDE_COMMAND_READ_DMA_EXT;
        }
        else
        {
            SrbExtension->CommandReg = IDE_COMMAND_WRITE_DMA_EXT;
        }

        SrbExtension->LBA3 = (StartOffset >> 24) & 0xFF;
        SrbExtension->LBA4 = (StartOffset >> 32) & 0xFF;
        SrbExtension->LBA5 = (StartOffset >> 40) & 0xFF;
    }
    else
    {
        NT_ASSERT(FALSE);
    }

    SrbExtension->FeaturesHigh = 0;
    SrbExtension->SectorCountLow = (SectorCount >> 0) & 0xFF;
    SrbExtension->SectorCountHigh = (SectorCount >> 8) & 0xFF;

    NT_ASSERT(SectorCount < 0x100);

    SrbExtension->pSgl = (PLOCAL_SCATTER_GATHER_LIST)StorPortGetScatterGatherList(AdapterExtension, Srb);

    return SRB_STATUS_PENDING;
}// -- DeviceRequestReadWrite();

/**
 * @name DeviceRequestCapacity
 * @implemented
 *
 * Handle SCSIOP_READ_CAPACITY OperationCode
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceRequestCapacity
 */
UCHAR
DeviceRequestCapacity (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    ULONG MaxLba, BytesPerLogicalSector;
    PREAD_CAPACITY_DATA ReadCapacity;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("DeviceRequestCapacity()\n");

    UNREFERENCED_PARAMETER(AdapterExtension);
    UNREFERENCED_PARAMETER(Cdb);

    NT_ASSERT(Srb->DataBuffer != NULL);
    NT_ASSERT(IsPortValid(AdapterExtension, Srb->PathId));


    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    if (PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI)
    {
        return AhciATAPICommand(AdapterExtension, Srb, Cdb);
    }

    if (Cdb->CDB10.OperationCode == SCSIOP_READ_CAPACITY)
    {
        ReadCapacity = (PREAD_CAPACITY_DATA)Srb->DataBuffer;

        BytesPerLogicalSector = PortExtension->DeviceParams.BytesPerLogicalSector;
        MaxLba = (ULONG)PortExtension->DeviceParams.MaxLba.QuadPart - 1;

        // I trust you windows :D
        NT_ASSERT(Srb->DataTransferLength >= sizeof(READ_CAPACITY_DATA));

        // I trust you user :D
        NT_ASSERT(PortExtension->DeviceParams.MaxLba.QuadPart < (ULONG)-1);

        // Actually I don't trust anyone :p
        Srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);

        REVERSE_BYTES(&ReadCapacity->BytesPerBlock, &BytesPerLogicalSector);
        REVERSE_BYTES(&ReadCapacity->LogicalBlockAddress, &MaxLba);
    }
    else
    {
        AhciDebugPrint("\tSCSIOP_READ_CAPACITY16 not supported\n");
        NT_ASSERT(FALSE);
    }

    return SRB_STATUS_SUCCESS;
}// -- DeviceRequestCapacity();

/**
 * @name DeviceRequestComplete
 * @implemented
 *
 * Handle UnHandled Requests
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceRequestComplete
 */
UCHAR
DeviceRequestComplete (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    AhciDebugPrint("DeviceRequestComplete()\n");

    UNREFERENCED_PARAMETER(AdapterExtension);
    UNREFERENCED_PARAMETER(Cdb);

    Srb->ScsiStatus = SCSISTAT_GOOD;

    return SRB_STATUS_SUCCESS;
}// -- DeviceRequestComplete();

/**
 * @name DeviceReportLuns
 * @implemented
 *
 * Handle SCSIOP_REPORT_LUNS OperationCode
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceReportLuns
 */
UCHAR
DeviceReportLuns (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    PLUN_LIST LunList;
    PAHCI_PORT_EXTENSION PortExtension;

    AhciDebugPrint("DeviceReportLuns()\n");

    UNREFERENCED_PARAMETER(Cdb);

    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    NT_ASSERT(Srb->DataTransferLength >= sizeof(LUN_LIST));
    NT_ASSERT(Cdb->CDB10.OperationCode == SCSIOP_REPORT_LUNS);

    if (PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI)
    {
        return AhciATAPICommand(AdapterExtension, Srb, Cdb);
    }

    LunList = (PLUN_LIST)Srb->DataBuffer;

    NT_ASSERT(LunList != NULL);

    AhciZeroMemory((PCHAR)LunList, sizeof(LUN_LIST));

    LunList->LunListLength[3] = 8;

    Srb->ScsiStatus = SCSISTAT_GOOD;
    Srb->DataTransferLength = sizeof(LUN_LIST);

    return SRB_STATUS_SUCCESS;
}// -- DeviceReportLuns();

/**
 * @name DeviceInquiryRequest
 * @implemented
 *
 * Tells wheather given port is implemented or not
 *
 * @param AdapterExtension
 * @param Srb
 * @param Cdb
 *
 * @return
 * return STOR status for DeviceInquiryRequest
 *
 * @remark
 * http://www.seagate.com/staticfiles/support/disc/manuals/Interface%20manuals/100293068c.pdf
 */
UCHAR
DeviceInquiryRequest (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in PSCSI_REQUEST_BLOCK Srb,
    __in PCDB Cdb
    )
{
    PVOID DataBuffer;
    PAHCI_SRB_EXTENSION SrbExtension;
    PAHCI_PORT_EXTENSION PortExtension;
    PVPD_SUPPORTED_PAGES_PAGE VpdOutputBuffer;
    ULONG DataBufferLength, RequiredDataBufferLength;

    AhciDebugPrint("DeviceInquiryRequest()\n");

    NT_ASSERT(Cdb->CDB10.OperationCode == SCSIOP_INQUIRY);
    NT_ASSERT(IsPortValid(AdapterExtension, Srb->PathId));

    SrbExtension = GetSrbExtension(Srb);
    PortExtension = &AdapterExtension->PortExtension[Srb->PathId];

    if (PortExtension->DeviceParams.DeviceType == AHCI_DEVICE_TYPE_ATAPI)
    {
        return AhciATAPICommand(AdapterExtension, Srb, Cdb);
    }

    if (Srb->Lun != 0)
    {
        return SRB_STATUS_SELECTION_TIMEOUT;
    }
    else if (Cdb->CDB6INQUIRY3.EnableVitalProductData == 0)
    {
        // 3.6.1
        // If the EVPD bit is set to zero, the device server shall return the standard INQUIRY data
        AhciDebugPrint("\tEVPD Inquired\n");
        NT_ASSERT(SrbExtension != NULL);

        SrbExtension->AtaFunction = ATA_FUNCTION_ATA_IDENTIFY;
        SrbExtension->Flags |= ATA_FLAGS_DATA_IN;
        SrbExtension->CompletionRoutine = InquiryCompletion;
        SrbExtension->CommandReg = IDE_COMMAND_NOT_VALID;

        // TODO: Should use AhciZeroMemory
        SrbExtension->FeaturesLow = 0;
        SrbExtension->LBA0 = 0;
        SrbExtension->LBA1 = 0;
        SrbExtension->LBA2 = 0;
        SrbExtension->Device = 0xA0;
        SrbExtension->LBA3 = 0;
        SrbExtension->LBA4 = 0;
        SrbExtension->LBA5 = 0;
        SrbExtension->FeaturesHigh = 0;
        SrbExtension->SectorCountLow = 0;
        SrbExtension->SectorCountHigh = 0;

        SrbExtension->Sgl.NumberOfElements = 1;
        SrbExtension->Sgl.List[0].PhysicalAddress.LowPart = PortExtension->IdentifyDeviceDataPhysicalAddress.LowPart;
        SrbExtension->Sgl.List[0].PhysicalAddress.HighPart = PortExtension->IdentifyDeviceDataPhysicalAddress.HighPart;
        SrbExtension->Sgl.List[0].Length = sizeof(IDENTIFY_DEVICE_DATA);

        SrbExtension->pSgl = &SrbExtension->Sgl;
        return SRB_STATUS_PENDING;
    }
    else
    {
        AhciDebugPrint("\tVPD Inquired\n");

        DataBuffer = Srb->DataBuffer;
        DataBufferLength = Srb->DataTransferLength;
        RequiredDataBufferLength = DataBufferLength; // make the compiler happy :p

        if (DataBuffer == NULL)
        {
            return SRB_STATUS_INVALID_REQUEST;
        }

        AhciZeroMemory(DataBuffer, DataBufferLength);

        switch(Cdb->CDB6INQUIRY3.PageCode)
        {
            case VPD_SUPPORTED_PAGES:
                {
                    AhciDebugPrint("\tVPD_SUPPORTED_PAGES\n");
                    RequiredDataBufferLength = sizeof(VPD_SUPPORTED_PAGES_PAGE) + 1;

                    if (DataBufferLength < RequiredDataBufferLength)
                    {
                        AhciDebugPrint("\tDataBufferLength: %d Required: %d\n", DataBufferLength, RequiredDataBufferLength);
                        return SRB_STATUS_INVALID_REQUEST;
                    }

                    VpdOutputBuffer = (PVPD_SUPPORTED_PAGES_PAGE)DataBuffer;

                    VpdOutputBuffer->DeviceType = PortExtension->DeviceParams.AccessType;
                    VpdOutputBuffer->DeviceTypeQualifier = 0;
                    VpdOutputBuffer->PageCode = VPD_SUPPORTED_PAGES;
                    VpdOutputBuffer->PageLength = 1;
                    VpdOutputBuffer->SupportedPageList[0] = VPD_SUPPORTED_PAGES;
                    //VpdOutputBuffer->SupportedPageList[1] = VPD_SERIAL_NUMBER;
                    //VpdOutputBuffer->SupportedPageList[2] = VPD_DEVICE_IDENTIFIERS;

                    NT_ASSERT(VpdOutputBuffer->DeviceType == DIRECT_ACCESS_DEVICE);
                }
                break;
            case VPD_SERIAL_NUMBER:
                {
                    AhciDebugPrint("\tVPD_SERIAL_NUMBER\n");
                }
                break;
            case VPD_DEVICE_IDENTIFIERS:
                {
                    AhciDebugPrint("\tVPD_DEVICE_IDENTIFIERS\n");
                }
                break;
            default:
                AhciDebugPrint("\tPageCode: %x\n", Cdb->CDB6INQUIRY3.PageCode);
                return SRB_STATUS_INVALID_REQUEST;
        }

        Srb->DataTransferLength = RequiredDataBufferLength;
        return SRB_STATUS_SUCCESS;
    }
}// -- DeviceInquiryRequest();

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
 * @param AdapterExtension
 *
 * @return
 * TRUE in case AHCI Controller RESTARTED successfully. i.e GHC.HR == 0
 */
BOOLEAN
AhciAdapterReset (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension
    )
{
    ULONG ticks;
    AHCI_GHC ghc;
    PAHCI_MEMORY_REGISTERS abar = NULL;

    AhciDebugPrint("AhciAdapterReset()\n");

    abar = AdapterExtension->ABAR_Address;
    if (abar == NULL) // basic sanity
    {
        return FALSE;
    }

    // HR -- Very first bit (lowest significant)
    ghc.HR = 1;
    StorPortWriteRegisterUlong(AdapterExtension, &abar->GHC, ghc.Status);

    for (ticks = 0; ticks < 50; ++ticks)
    {
        ghc.Status = StorPortReadRegisterUlong(AdapterExtension, &abar->GHC);
        if (ghc.HR == 0)
        {
            break;
        }
        StorPortStallExecution(20000);
    }

    if (ticks == 50)// 1 second
    {
        AhciDebugPrint("\tDevice Timeout\n");
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
 * @param Buffer
 * @param BufferSize
 */
FORCEINLINE
VOID
AhciZeroMemory (
    __out PCHAR Buffer,
    __in ULONG BufferSize
    )
{
    ULONG i;
    for (i = 0; i < BufferSize; i++)
    {
        Buffer[i] = 0;
    }

    return;
}// -- AhciZeroMemory();

/**
 * @name IsPortValid
 * @implemented
 *
 * Tells wheather given port is implemented or not
 *
 * @param AdapterExtension
 * @param PathId
 *
 * @return
 * return TRUE if provided port is valid (implemented) or not
 */
FORCEINLINE
BOOLEAN
IsPortValid (
    __in PAHCI_ADAPTER_EXTENSION AdapterExtension,
    __in ULONG pathId
    )
{
    NT_ASSERT(pathId < MAXIMUM_AHCI_PORT_COUNT);

    if (pathId >= AdapterExtension->PortCount)
    {
        return FALSE;
    }

    return AdapterExtension->PortExtension[pathId].DeviceParams.IsActive;
}// -- IsPortValid()

/**
 * @name AddQueue
 * @implemented
 *
 * Add Srb to Queue
 *
 * @param Queue
 * @param Srb
 *
 * @return
 * return TRUE if Srb is successfully added to Queue
 *
 */
FORCEINLINE
BOOLEAN
AddQueue (
    __inout PAHCI_QUEUE Queue,
    __in PVOID Srb
    )
{
    NT_ASSERT(Queue->Head < MAXIMUM_QUEUE_BUFFER_SIZE);
    NT_ASSERT(Queue->Tail < MAXIMUM_QUEUE_BUFFER_SIZE);

    if (Queue->Tail == ((Queue->Head + 1) % MAXIMUM_QUEUE_BUFFER_SIZE))
        return FALSE;

    Queue->Buffer[Queue->Head++] = Srb;
    Queue->Head %= MAXIMUM_QUEUE_BUFFER_SIZE;

    return TRUE;
}// -- AddQueue();

/**
 * @name RemoveQueue
 * @implemented
 *
 * Remove and return Srb from Queue
 *
 * @param Queue
 *
 * @return
 * return Srb
 *
 */
FORCEINLINE
PVOID
RemoveQueue (
    __inout PAHCI_QUEUE Queue
    )
{
    PVOID Srb;

    NT_ASSERT(Queue->Head < MAXIMUM_QUEUE_BUFFER_SIZE);
    NT_ASSERT(Queue->Tail < MAXIMUM_QUEUE_BUFFER_SIZE);

    if (Queue->Head == Queue->Tail)
        return NULL;

    Srb = Queue->Buffer[Queue->Tail++];
    Queue->Tail %= MAXIMUM_QUEUE_BUFFER_SIZE;

    return Srb;
}// -- RemoveQueue();

/**
 * @name GetSrbExtension
 * @implemented
 *
 * GetSrbExtension from Srb make sure It is properly aligned
 *
 * @param Srb
 *
 * @return
 * return SrbExtension
 *
 */
FORCEINLINE
PAHCI_SRB_EXTENSION
GetSrbExtension (
    __in PSCSI_REQUEST_BLOCK Srb
    )
{
    ULONG Offset;
    ULONG_PTR SrbExtension;

    SrbExtension = (ULONG_PTR)Srb->SrbExtension;
    Offset = SrbExtension % 128;

    // CommandTable should be 128 byte aligned
    if (Offset != 0)
        Offset = 128 - Offset;

    return (PAHCI_SRB_EXTENSION)(SrbExtension + Offset);
}// -- PAHCI_SRB_EXTENSION();

/**
 * @name AhciGetLba
 * @implemented
 *
 * Find the logical address of demand block from Cdb
 *
 * @param Srb
 *
 * @return
 * return Logical Address of the block
 *
 */
FORCEINLINE
ULONG64
AhciGetLba (
    __in PCDB Cdb,
    __in ULONG CdbLength
    )
{
    ULONG64 lba = 0;

    NT_ASSERT(Cdb != NULL);
    NT_ASSERT(CdbLength != 0);

    if (CdbLength == 0x10)
    {
        REVERSE_BYTES_QUAD(&lba, Cdb->CDB16.LogicalBlock);
    }
    else
    {
        lba |= Cdb->CDB10.LogicalBlockByte3 << 0;
        lba |= Cdb->CDB10.LogicalBlockByte2 << 8;
        lba |= Cdb->CDB10.LogicalBlockByte1 << 16;
        lba |= Cdb->CDB10.LogicalBlockByte0 << 24;
    }

    return lba;
}// -- AhciGetLba();
