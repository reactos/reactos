#include "NvmeProbe.h"

BOOLEAN
ScanBusAndFindNvme(BOOLEAN *found, INT *bus, INT *slot)
{
    UINT32 busIdx = 0, slotNum = 0, funcNum = 0;
    PCI_COMMON_CONFIG pciData = {0};
    BOOLEAN           done   = FALSE;
    ULONG             retData = 0;

    for(busIdx = 0; busIdx < PCI_MAX_BUS && !done; busIdx++) {
        for(slotNum = 0; slotNum < PCI_MAX_DEVICES  && !done; slotNum++) {
            for(funcNum = 0; funcNum < PCI_MAX_FUNCTION && !done; funcNum++) {

                retData = HalGetBusData(PCIConfiguration,
                                        busIdx,
                                        slotNum,
                                        &pciData,
                                         PCI_COMMON_HDR_LENGTH);

                if (retData == 0) {
                    done = TRUE;
                    break;
                }

                if (retData == 2)
                    continue;

                if (pciData.VendorID != PCI_INVALID_VENDORID &&
                    pciData.BaseClass == 0x1 &&
                    pciData.SubClass == 0x8) {
                    *bus   = busIdx;
                    *slot  = slotNum;
                    *found = TRUE;
                    DbgPrint("NVMe device found Bus: 0x%x Slot: 0x%x\n", *bus, *slot);
                    // TODO: For now we support only one dev
                    return TRUE;
                }

            }
        }
    }

    DbgPrint("No NVMe device on the system\n");
    return FALSE;
}

PVOID
MapNvmeCtrlRegisters(PVOID pAE, INT Bus, INT SlotNum, PACCESS_RANGE pAccessRanges)
{
    PCI_COMMON_CONFIG pciData = {0};
    ULONG retData  = 0;
    ULONG pBarZero = 0;
    ULONG barSize  = 0;
    SCSI_PHYSICAL_ADDRESS barAddress = {0};

    retData = HalGetBusData(PCIConfiguration, Bus, SlotNum, &pciData, PCI_COMMON_HDR_LENGTH);

    if (retData == 0 || retData == 2) {
        // What?
        return NULL;
    }

    /*
     * To determine the size of the BAR, we will write all 1's to the BAR
     * and Read the PCI config back. This should give us the space
     */

    // Lets Write 1s
    pBarZero = pciData.u.type0.BaseAddresses[0];
    pciData.u.type0.BaseAddresses[0] = -1;
    HalSetBusData(PCIConfiguration, Bus, SlotNum, &pciData, PCI_COMMON_HDR_LENGTH);

    // Lets wait a bit
    ScsiPortStallExecution(1000);

    // Get the Bar again
    retData = HalGetBusData(PCIConfiguration, Bus, SlotNum, &pciData, PCI_COMMON_HDR_LENGTH);
    barSize = pciData.u.type0.BaseAddresses[0];

    // Restore BAR0
    pciData.u.type0.BaseAddresses[0] = pBarZero;
    HalSetBusData(PCIConfiguration, Bus, SlotNum, &pciData, PCI_COMMON_HDR_LENGTH);

    // Now lets map BAR0
    barAddress.QuadPart = pBarZero;
    barSize = ~barSize + 1;
    DbgPrint("%s BAR0: %p BarSize: 0x%lx\n", __FUNCTION__, (UCHAR)pBarZero, barSize);

    if (barSize == 0 || pBarZero == 0)
        return NULL;
    
    pAccessRanges->RangeStart = barAddress;
    pAccessRanges->RangeInMemory = TRUE;
    pAccessRanges->RangeLength = barSize;
    return (PNVMe_CONTROLLER_REGISTERS)ScsiPortGetDeviceBase(pAE, PCIBus, Bus, barAddress, barSize, FALSE);
}

BOOLEAN
NVMeSetupPCI(ULONG busNumber, ULONG slotNumber, PPCI_COMMON_CONFIG pciData)
{
    ULONG busDataRead = 0;
    USHORT CmdOrig;

    /* Setup the command register */
    for(int i = 0; i < 3; i++) {
        CmdOrig = pciData->Command;
        switch(i) {
        case 0:
            pciData->Command |= PCI_ENABLE_IO_SPACE;
            break;
        case 1:
            pciData->Command |= PCI_ENABLE_MEMORY_SPACE;
            break;
        case 2:
            pciData->Command |= PCI_ENABLE_BUS_MASTER;
            break;
        case 3:
            pciData->Command |= PCI_DISABLE_LEVEL_INTERRUPT;
        }
        if(CmdOrig == pciData->Command) {
            continue;
        }
        HalSetBusDataByOffset(PCIConfiguration, busNumber, slotNumber, &(pciData->Command), offsetof(PCI_COMMON_CONFIG, Command), sizeof(pciData->Command));
        busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotNumber, pciData, PCI_COMMON_HDR_LENGTH);
        if(busDataRead < PCI_COMMON_HDR_LENGTH) {
            /* There's nothing we can do if this dosen't go through unless maybe try again in some time */
            return FALSE;
        }
    }

    /* 
    * Disconnect the interrupt line 
    * For now we are just masking interrupts from the controller but this is a good thing to do to free up the interrupt line. 
    * The ScsiPort for some reason does not accept that a target cannot have interrupts setup, We need to fix that in ScsiPort.
    * For now we are connected to an Interrupt line but we have masked interrupts. Eventually either we enable interrupts
    * or give up the line

    busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotNumber, pciData, PCI_COMMON_HDR_LENGTH);
    if(busDataRead < PCI_COMMON_HDR_LENGTH) {
        return FALSE;
    }

    pciData->u.type0.InterruptLine = 0xb;
    HalSetBusData(PCIConfiguration, busNumber, slotNumber, pciData, PCI_COMMON_HDR_LENGTH);

    busDataRead = HalGetBusData(PCIConfiguration, busNumber, slotNumber, pciData, PCI_COMMON_HDR_LENGTH);
    if(busDataRead < PCI_COMMON_HDR_LENGTH) {
        return FALSE;
    }*/

    return TRUE;
} 

ULONG
GetBarSize(PPCI_NVM_DEVICE_INFO pciNvmeDeviceInfo)
{
    PCI_COMMON_CONFIG pciData = {0};
    ULONG retData = 0;
    ULONG barSize = 0;
    ULONG pBarZero = 0;

    // Reread config info    
    retData = HalGetBusData(PCIConfiguration, pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData, PCI_COMMON_HDR_LENGTH); 
    if (retData == 0 || retData == 2) {
        return 0;
    }

    pBarZero = pciData.u.type0.BaseAddresses[0];

    // Write 0xFFFFFFFFF to bar0
    pciData.u.type0.BaseAddresses[0] = -1;
    retData = HalSetBusData(PCIConfiguration, pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData, PCI_COMMON_HDR_LENGTH);
    // Ideally we should retry here
    if (retData == 0 || retData == 2) {
        return 0;
    }
    // Lets wait a bit
    ScsiPortStallExecution(1000);

    // Get the Bar again
    retData = HalGetBusData(PCIConfiguration, pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData, PCI_COMMON_HDR_LENGTH);
    if (retData == 0 || retData == 2) {
        return 0;
    }

    // Find Size of BAR0
    barSize = pciData.u.type0.BaseAddresses[0];
    barSize = ~barSize + 1;

    // Restore BAR0
    pciData.u.type0.BaseAddresses[0] = pBarZero;
    retData = HalSetBusData(PCIConfiguration, pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData, PCI_COMMON_HDR_LENGTH);
    if (retData == 0 || retData == 2) {
        return 0;
    }

    return barSize;
}

BOOLEAN
NvmeAllocIOQueues(PNVME_DEVICE_EXTENSION pAE)
{
    PHYSICAL_ADDRESS Low;
    PHYSICAL_ADDRESS High;
    PHYSICAL_ADDRESS Align;
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    Low.QuadPart = 0;
    High.QuadPart = (-1);
    Align.QuadPart = 0;

    /* Alloc Admin Queues */
    PVOID IoSubQ = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);
    PVOID IoComQ = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);

    /* 
    * We preallocate 1 Pages worth of memory for PRP List. 
    * This means our IOs are currently capped at 2MB.
    */
    PVOID pPRPList = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);

    if (IoSubQ == NULL || IoComQ == NULL || pPRPList == NULL)
        goto ERR_FREE_MEM;

    RtlZeroMemory(IoSubQ,   PAGE_SIZE);
    RtlZeroMemory(IoComQ,   PAGE_SIZE);
    RtlZeroMemory(pPRPList, PAGE_SIZE * 2);

    pQI->SubQueueInfo[1].CID        = 1;
    pQI->SubQueueInfo[1].CplQueueID = 1;
    pQI->SubQueueInfo[1].SubQueueID = 1;
    pQI->SubQueueInfo[1].pSubTDBL   = (PULONG)((PCHAR)&pAE->pCtrlRegister->IODB[0].AsUlong + (2 * (4 << pAE->strideSz)));
    pQI->CplQueueInfo[1].pCplHDBL   = (PULONG)((PCHAR)&pAE->pCtrlRegister->IODB[0].AsUlong + (3 * (4 << pAE->strideSz)));

    pQI->SubQueueInfo[1].SubQStartVirtual   = IoSubQ;
    pQI->SubQueueInfo[1].SubQStartPhysical  = MmGetPhysicalAddress(IoSubQ);
    pQI->SubQueueInfo[1].QueueAllocSize     = PAGE_SIZE;
    pQI->SubQueueInfo[1].SubQEntries        = PAGE_SIZE/sizeof(NVMe_COMMAND) - 1;

    pQI->SubQueueInfo[1].pPRPListAlloc         = pPRPList;
    pQI->SubQueueInfo[1].PRPListStartPhysical  = MmGetPhysicalAddress(pPRPList);
    pQI->SubQueueInfo[1].PRPListStartVirtual   = pPRPList;
    pQI->SubQueueInfo[1].PRPListAllocSize      = PAGE_SIZE;

    pQI->CplQueueInfo[1].CplQStartVirtual = IoComQ;
    pQI->CplQueueInfo[1].CplQStartPhysical  = MmGetPhysicalAddress(IoComQ);
    pQI->CplQueueInfo[1].CplQueueID = 1;
    pQI->CplQueueInfo[1].CplQEntries = PAGE_SIZE/sizeof(NVMe_COMPLETION_QUEUE_ENTRY) - 1;

    return TRUE;

ERR_FREE_MEM:
    if (IoSubQ != NULL) 
        MmFreeContiguousMemory(IoSubQ);
    if (IoComQ != NULL) 
        MmFreeContiguousMemory(IoComQ);
    if (pPRPList != NULL) 
        MmFreeContiguousMemory(pPRPList);
    return FALSE;
}

BOOLEAN
NvmeAllocAndInitAdminQueues(PNVME_DEVICE_EXTENSION pAE)
{
   // Allocating Admin Queues and a page for Identify
    PHYSICAL_ADDRESS Low;
    PHYSICAL_ADDRESS High;
    PHYSICAL_ADDRESS Align;
    PQUEUE_INFO pQI = &pAE->QueueInfo;

    RtlZeroMemory(pQI->SubQueueInfo, sizeof(SUB_QUEUE_INFO) * (DFT_IO_QUEUES + 1));
    RtlZeroMemory(pQI->CplQueueInfo, sizeof(SUB_QUEUE_INFO) * (DFT_IO_QUEUES + 1));

    Low.QuadPart = 0;
    High.QuadPart = (-1);
    Align.QuadPart = 0;
    PVOID AdminSubQ = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);
    PVOID AdminComQ = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);
    PVOID pPRPList  = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, Low, High, Align, MmNonCached);

    if (AdminSubQ == NULL || AdminComQ == NULL || pPRPList == NULL)
        goto ERR_FREE_MEM;
    
    RtlZeroMemory(AdminSubQ, PAGE_SIZE);
    RtlZeroMemory(AdminComQ, PAGE_SIZE);
    RtlZeroMemory(pPRPList,  PAGE_SIZE);
    DbgPrint("SubQ:%p ComQ:%p\n", AdminSubQ, AdminComQ);

    pQI->SubQueueInfo[0].CID               = 1;
    pQI->SubQueueInfo[0].SubQStartVirtual  = AdminSubQ;
    pQI->SubQueueInfo[0].pSubTDBL          = &pAE->pCtrlRegister->IODB[0].AsUlong;
    pQI->SubQueueInfo[0].SubQStartPhysical = MmGetPhysicalAddress(AdminSubQ);
    pQI->SubQueueInfo[0].QueueAllocSize    = PAGE_SIZE;
    pQI->SubQueueInfo[0].SubQEntries       = PAGE_SIZE/sizeof(NVMe_COMMAND) - 1;
    pQI->SubQueueInfo[0].PRPListAllocSize  = PAGE_SIZE;
    pQI->SubQueueInfo[0].PRPListStartPhysical = MmGetPhysicalAddress(pPRPList);
    pQI->SubQueueInfo[0].PRPListStartVirtual  = pPRPList;
    
    pQI->CplQueueInfo[0].CplQStartVirtual  = AdminComQ;
    pQI->CplQueueInfo[0].CplQStartPhysical = MmGetPhysicalAddress(AdminComQ);
    pQI->CplQueueInfo[0].CplQEntries       = PAGE_SIZE/sizeof(NVMe_COMMAND) - 1;
    pQI->CplQueueInfo[0].pCplHDBL          = (PULONG)((PCHAR)&pAE->pCtrlRegister->IODB[0].AsUlong + (4 << pAE->strideSz));

    /* Lets tell the controller about our Admin Queues */
    NVMe_ADMIN_QUEUE_ATTRIBUTES aqa = {0};
    aqa.ACQS = pQI->CplQueueInfo[0].CplQEntries;
    aqa.ASQS = pQI->SubQueueInfo[0].SubQEntries;
    WRITE_REGISTER_ULONG(&pAE->pCtrlRegister->AQA.AsUlong, aqa.AsUlong);

    pQI->SubQueueInfo->SubQEntries = aqa.ASQS;
    pQI->CplQueueInfo->CplQEntries = aqa.ACQS;

    /* Fill queue addr to controller */ 
    PHYSICAL_ADDRESS phySubQ = MmGetPhysicalAddress(AdminSubQ);
    WRITE_REGISTER_ULONG(&pAE->pCtrlRegister->ASQ.LowPart, phySubQ.LowPart);
    WRITE_REGISTER_ULONG(&pAE->pCtrlRegister->ASQ.HighPart, phySubQ.HighPart);
    PHYSICAL_ADDRESS phyComQ = MmGetPhysicalAddress(AdminComQ);
    WRITE_REGISTER_ULONG(&pAE->pCtrlRegister->ACQ.LowPart, phyComQ.LowPart);
    WRITE_REGISTER_ULONG(&pAE->pCtrlRegister->ACQ.HighPart, phyComQ.HighPart);
    
    return TRUE;

ERR_FREE_MEM:
    if (AdminSubQ != NULL) 
        MmFreeContiguousMemory(AdminSubQ);
    if (AdminComQ != NULL) 
        MmFreeContiguousMemory(AdminComQ);
    return FALSE;
}

/*
* ROS: We are essentially _ignoring_ whatever the Scsi port has given us
* We don't really trust the ScsiPort to do whats needed to be done. So we will do everything ourselves.
* We are modifying the PCI bar registers here so we need to be careful.
* TODO: Check the values returned by ScsiPort and use those instead. Maybe a StorNvme?
*/
BOOLEAN
FindAndSetupNvmeController(PNVME_DEVICE_EXTENSION pAE, PPCI_NVM_DEVICE_INFO pciNvmeDeviceInfo)
{
    ULONG barSize  = 0;
    PHYSICAL_ADDRESS barAddress = {0};
    PCI_COMMON_CONFIG pciData = {0};
    ULONG retData = 0;
    NVMe_CONTROLLER_CAPABILITIES CAP = {0};

    retData = HalGetBusData(PCIConfiguration, pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData, PCI_COMMON_HDR_LENGTH);
    if (retData == 0 || retData == 2) {
        return FALSE;
    }

    /* Store BAR 0 */
    barAddress.QuadPart = pciData.u.type0.BaseAddresses[0];

    /* Enable Bus Mastering, IO and MMIO. Disable Interrupts */
    if (!NVMeSetupPCI(pciNvmeDeviceInfo->Bus, pciNvmeDeviceInfo->Slot, &pciData)){
        DbgPrint("Failed to setup the NVMe PCIe device\n");
        return FALSE;
    }
    
    /* BAR 0 size */
    barSize = GetBarSize(pciNvmeDeviceInfo);
    if (barSize == 0){
        DbgPrint("Failed to get barsize of NVMe Device\n");
        return FALSE;
    }
    pAE->CtrlRegSize = barSize;

    /* TODO: We should ideally check with what the scsiport has offered as ranges */

    /* MMap BAR0 */
    PNVMe_CONTROLLER_REGISTERS pNvmeCtrlRegs = (PNVMe_CONTROLLER_REGISTERS)MmMapIoSpace(barAddress, barSize, FALSE);
    pAE->pCtrlRegister = pNvmeCtrlRegs;
    DbgPrint("%s BAR0: %p BarSize: 0x%lx pNvmeCtrlRegs: %p\n", __FUNCTION__, (PVOID)barAddress.QuadPart, barSize, pNvmeCtrlRegs);

    /* Read Capabality register */
    CAP.HighPart = READ_REGISTER_ULONG((PULONG)(&pAE->pCtrlRegister->CAP.HighPart));
    CAP.LowPart = READ_REGISTER_ULONG((PULONG)(&pAE->pCtrlRegister->CAP.LowPart));

    /* Store whatever is needed for later */
    pAE->originalVersion.value = READ_REGISTER_ULONG((PULONG)&pAE->pCtrlRegister->VS);
    pAE->uSecCrtlTimeout       = (ULONG)(CAP.TO * MIN_WAIT_TIMEOUT);
    pAE->uSecCrtlTimeout       = (pAE->uSecCrtlTimeout == 0) ? MIN_WAIT_TIMEOUT : pAE->uSecCrtlTimeout;
    pAE->uSecCrtlTimeout      *= MILLI_TO_MICRO;
    pAE->strideSz              = CAP.DSTRD;
    DbgPrint("NVMeFindAdapter: Stride Size set to 0x%x\n", pAE->strideSz);

    /* Mask interrutps */
    WRITE_REGISTER_ULONG(&pNvmeCtrlRegs->IVMS, -1);

    /* Lets alloc and Init Admin queues */
    if (!NvmeAllocAndInitAdminQueues(pAE)){
        DbgPrint("Failed to Allocate and Init Admin queues\n");
        return FALSE;
    }
    
    return TRUE;
}

/* TODO: Add functionality for passing CNS */
BOOLEAN
NVMeSendIdentifyController(PNVME_DEVICE_EXTENSION pDevExtension)
{
    if (pDevExtension == NULL)
        return FALSE;

    NVMe_COMMAND nvmeCmd = {0};
    NVMe_COMPLETION_QUEUE_ENTRY cmpEntry = {0};

    nvmeCmd.CDW0.OPC = ADMIN_IDENTIFY;
    nvmeCmd.PRP1     = pDevExtension->QueueInfo.SubQueueInfo->PRPListStartPhysical.QuadPart;
    ((PADMIN_IDENTIFY_COMMAND_DW10)&nvmeCmd.CDW10)->CNS = CNS_IDENTIFY_CNTLR;

    if (!NVMeIssueCmd(pDevExtension, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue Identify controller\n");
        return FALSE;
    }

    RtlCopyMemory(&pDevExtension->controllerIdentifyData, 
                  pDevExtension->QueueInfo.SubQueueInfo->PRPListStartVirtual, 
                  sizeof(ADMIN_IDENTIFY_CONTROLLER));

    DbgPrint("Sucessfully identified NVMe controller 0x%x 0x%x\n", pDevExtension->controllerIdentifyData.VID, pDevExtension->controllerIdentifyData.SSVID);

    /* TODO: Currenly we are limiting IOs to 1MB but if the controller somehow demands lower max transfer we need to set that up here */
    return TRUE;
}

BOOLEAN
CreateNS(PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pDevExt == NULL)
        return FALSE;

    BOOLEAN createdNS = FALSE;
 
    NVMe_COMMAND nvmeCmd = {0};
    NVMe_COMPLETION_QUEUE_ENTRY cmpEntry = {0};

    nvmeCmd.CDW0.OPC = ADMIN_IDENTIFY;
    nvmeCmd.NSID     = -1;
    nvmeCmd.PRP1     = pDevExt->QueueInfo.SubQueueInfo->PRPListStartPhysical.QuadPart;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to Issue Identify CNS:0\n");
        return createdNS;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Namespace Mgmt not supported SC:0x%x SCT:0x%x\n", cmpEntry.DW3.SF.SC, cmpEntry.DW3.SF.SCT);
        return createdNS;
    }

    /* In VBox the unallocated NVMe cap is 0 for some reason. Need to test on other configurations */
    if (pDevExt->controllerIdentifyData.UNVMCAP[0] == 0) {
        DbgPrint("Unallocated NVMe capacity is 0. Failed to create NS\n");
        return createdNS;
    }

    /* TODO: Fill this on configuration other than VBOX */

    createdNS = TRUE;
    return createdNS;
}

/*
* The whole NS debacle:
* For ROS we currently only support one NS, if we find multiple NS
* we will need to have some sort of handling for that. The simplest next step
* would be to only use the first NS? IDK
* But for now we are supporting only one NS
*/
BOOLEAN
NVMeInitNS(PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pDevExt == NULL)
        return FALSE;

    BOOLEAN foundNS = FALSE;
    NVMe_COMMAND nvmeCmd = {0};
    NVMe_COMPLETION_QUEUE_ENTRY cmpEntry = {0};

    /* Lets see if we have any attached NS */
    nvmeCmd.CDW0.OPC = ADMIN_IDENTIFY;
    nvmeCmd.PRP1     = pDevExt->QueueInfo.SubQueueInfo->PRPListStartPhysical.QuadPart;
    ((PADMIN_IDENTIFY_COMMAND_DW10)&nvmeCmd.CDW10)->CNS = CNS_LIST_ATTACHED_NAMESPACES;

    RtlZeroMemory(pDevExt->QueueInfo.SubQueueInfo->PRPListStartVirtual, PAGE_SIZE);

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue List attach NS\n");
        return FALSE;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Command Identify failed CNS:0x%x\n", CNS_LIST_ATTACHED_NAMESPACES);
        return FALSE;
    }

    PUINT32 pNSList = (PUINT32)pDevExt->QueueInfo.SubQueueInfo->PRPListStartVirtual;
    for (int i = 0; i < MAX_NAMESPACES; i++){
        if (pNSList[i] == 0)
            break;
        
        /* Valid NS, lets store it */
        pDevExt->NSInfo.namespaceId = pNSList[i];
        pDevExt->NSInfo.nsStatus = NS_ATTACHED;
        foundNS = TRUE;
    }

    if (foundNS == FALSE) {
        /* TODO: Check if we have allocated NS but not attached; else create NS */
        DbgPrint("No Active NS found for controller %s\n", pDevExt->controllerIdentifyData.SN);
        return FALSE;
    }

    /* Reset */
    RtlZeroMemory(&nvmeCmd, sizeof(NVMe_COMMAND));
    RtlZeroMemory(pDevExt->QueueInfo.SubQueueInfo->PRPListStartVirtual, sizeof(NVMe_COMMAND));

    /* We have a NS created and attached. Lets id the NS */
    nvmeCmd.CDW0.OPC = ADMIN_IDENTIFY;
    nvmeCmd.NSID     = pDevExt->NSInfo.namespaceId;
    nvmeCmd.PRP1     = pDevExt->QueueInfo.SubQueueInfo->PRPListStartPhysical.QuadPart;
    ((PADMIN_IDENTIFY_COMMAND_DW10)&nvmeCmd.CDW10)->CNS = CNS_IDENTIFY_NAMESPACE;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue Identify Namespace for NSID:0x%x\n", pDevExt->NSInfo.namespaceId);
        return FALSE;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Command Identify failed CNS:0x%x\n", CNS_IDENTIFY_NAMESPACE);
        return FALSE;
    }

    RtlCopyMemory(&pDevExt->NSInfo.identifyData, pDevExt->QueueInfo.SubQueueInfo->PRPListStartVirtual, sizeof(ADMIN_IDENTIFY_NAMESPACE));
    DbgPrint("Found NS[0x%x] NSZE 0x%llx NCAP 0x%llx\n", pDevExt->NSInfo.namespaceId, pDevExt->NSInfo.identifyData.NSZE, pDevExt->NSInfo.identifyData.NCAP);

    pDevExt->NSInfo.BlockSize = 1 << pDevExt->NSInfo.identifyData.LBAFx[pDevExt->NSInfo.identifyData.FLBAS.SupportedCombination].LBADS;
    pDevExt->NSInfo.IsNamespaceReadOnly = FALSE;
    pDevExt->NSInfo.nsReady = TRUE;

    return TRUE;
}

BOOLEAN
NVMeInitIOQueues(PNVME_DEVICE_EXTENSION pDevExt)
{
    if (pDevExt == NULL)
        return FALSE;

    BOOLEAN bRet = FALSE;
    NVMe_COMMAND nvmeCmd = {0};
    NVMe_COMPLETION_QUEUE_ENTRY cmpEntry = {0};

    /* Set Feature saying we want one IO queue pair */
    nvmeCmd.CDW0.OPC = ADMIN_SET_FEATURES;
    ((PADMIN_SET_FEATURES_COMMAND_DW10)&nvmeCmd.CDW10)->FID = FID_NUMBER_OF_QUEUES;
    
    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue Identify Set Feature FID: 0x%x\n", FID_NUMBER_OF_QUEUES);
        return bRet;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Failed Set Features\n");
        return bRet;
    }

    PADMIN_SET_FEATURES_COMPLETION_NUMBER_OF_QUEUES_DW0 pCDW0 = (PADMIN_SET_FEATURES_COMPLETION_NUMBER_OF_QUEUES_DW0)&cmpEntry.DW0;
    DbgPrint("Controller Allocated 0x%x Submission Queues and 0x%x Completion queues\n", pCDW0->NSQA, pCDW0->NCQA);

    DbgPrint("Creating IO Queues. CAP.MQES 0x%x, CAP.CQR %d", pDevExt->pCtrlRegister->CAP.MQES, pDevExt->pCtrlRegister->CAP.CQR);

    /* Alloc IO Queues */
    if (!NvmeAllocIOQueues(pDevExt)){
        DbgPrint("Failed to Alloc IO queues\n");
        return FALSE;
    }

    /* Reset */
    RtlZeroMemory(&nvmeCmd, sizeof(NVMe_COMMAND));
    RtlZeroMemory(&cmpEntry, sizeof(NVMe_COMPLETION_QUEUE_ENTRY));

    /* Lets create IO completion Queue */
    nvmeCmd.CDW0.OPC = ADMIN_CREATE_IO_COMPLETION_QUEUE;
    nvmeCmd.PRP1     = pDevExt->QueueInfo.CplQueueInfo[1].CplQStartPhysical.QuadPart;
    ((PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10)&nvmeCmd.CDW10)->QID   = pDevExt->QueueInfo.CplQueueInfo[1].CplQueueID;
    ((PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10)&nvmeCmd.CDW10)->QSIZE = pDevExt->QueueInfo.CplQueueInfo[1].CplQEntries;
    ((PADMIN_CREATE_IO_COMPLETION_QUEUE_DW11)&nvmeCmd.CDW11)->PC    = 1;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue Create IO Completion queue\n");
        return bRet;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Failed to create IO Completion queue\n");
        return bRet;
    }

    DbgPrint("Created IO Completion queue[%p] QID[0x%x] Entries[0x%x]\n", pDevExt->QueueInfo.CplQueueInfo[1].CplQStartPhysical.QuadPart,
            pDevExt->QueueInfo.CplQueueInfo[1].CplQueueID, pDevExt->QueueInfo.CplQueueInfo[1].CplQEntries);

    /* Reset */
    RtlZeroMemory(&nvmeCmd, sizeof(NVMe_COMMAND));
    RtlZeroMemory(&cmpEntry, sizeof(NVMe_COMPLETION_QUEUE_ENTRY));

    /* Lets create IO submission Queue */
    nvmeCmd.CDW0.OPC = ADMIN_CREATE_IO_SUBMISSION_QUEUE;
    nvmeCmd.PRP1     = pDevExt->QueueInfo.SubQueueInfo[1].SubQStartPhysical.QuadPart;
    ((PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10)&nvmeCmd.CDW10)->QID   = pDevExt->QueueInfo.SubQueueInfo[1].SubQueueID;
    ((PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10)&nvmeCmd.CDW10)->QSIZE = pDevExt->QueueInfo.SubQueueInfo[1].SubQEntries;
    ((PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11)&nvmeCmd.CDW11)->CQID  = pDevExt->QueueInfo.CplQueueInfo[1].CplQueueID;
    ((PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11)&nvmeCmd.CDW11)->PC    = 1;

    if (!NVMeIssueCmd(pDevExt, &nvmeCmd, &cmpEntry, FALSE)) {
        DbgPrint("Failed to issue Create IO Submission queue\n");
        return bRet;
    }

    if (!NVMeIsCmdSuccessful(&cmpEntry)) {
        DbgPrint("Failed to create IO Submission queue\n");
        return bRet;
    }

    DbgPrint("Created IO Submission queue[%p] QID[0x%x] Entries[0x%x] CPLQIOD[0x%x]\n", pDevExt->QueueInfo.SubQueueInfo[1].SubQStartPhysical.QuadPart,
            pDevExt->QueueInfo.SubQueueInfo[1].SubQueueID, pDevExt->QueueInfo.SubQueueInfo[1].SubQEntries, pDevExt->QueueInfo.CplQueueInfo[1].CplQueueID);

    bRet = TRUE;
    return bRet;
}


VOID
NVMeFreeMemory(PNVME_DEVICE_EXTENSION pDevExt)
{
    for(int i = 0; i < 2; i++) {
        MmFreeContiguousMemory(pDevExt->QueueInfo.SubQueueInfo[i].SubQStartVirtual);
        MmFreeContiguousMemory(pDevExt->QueueInfo.SubQueueInfo[1].pPRPListAlloc);
        MmFreeContiguousMemory(pDevExt->QueueInfo.CplQueueInfo[1].CplQStartVirtual);
        MmUnmapIoSpace(pDevExt->pCtrlRegister, pDevExt->CtrlRegSize);
    }
}