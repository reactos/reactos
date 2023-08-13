#include "precomp.h"
#include <stdio.h>

BOOLEAN 
NVMeWaitForReady(PNVME_DEVICE_EXTENSION pDevExt, ULONG expectedValue)
{
    NVMe_CONTROLLER_STATUS CSTS = {0};
    CSTS.AsUlong = READ_REGISTER_ULONG(&pDevExt->pCtrlRegister->CSTS.AsUlong);

    while (CSTS.RDY != expectedValue) {
        /* We are looping forever, need to stop after CAP.TO */
        CSTS.AsUlong = READ_REGISTER_ULONG( &pDevExt->pCtrlRegister->CSTS.AsUlong);
    }

    return TRUE;
}

BOOLEAN
NTAPI NVMeHwInterrupt(
  _In_ PVOID Context)
{
    return FALSE;
}

BOOLEAN
NTAPI NVMeHwInitialize(
  _In_ PVOID Context)
{
    /*
    * ROS: We should enable interrupts here
    * To be honest we should ideally we doing the Admin and IO Queue Init here
    * When we enable the interrupts or even for polling it doesnt matter
    * Its a little barren here.
    *
    * PNVME_DEVICE_EXTENSION pDevExt = (PNVME_DEVICE_EXTENSION)Context;
    * WRITE_REGISTER_ULONG(&pDevExt->pCtrlRegister->INTMC, -1);
    */

    return TRUE;
}

BOOLEAN
NTAPI NVMeHwStartIo(
  _In_ PVOID DeviceExtension,
  _In_ PSCSI_REQUEST_BLOCK Srb)
{
    BOOLEAN bRet = TRUE;
    PNVME_DEVICE_EXTENSION pDevExt = (PNVME_DEVICE_EXTENSION)DeviceExtension;
    PNVME_SRB_EXTENSION pSrbExtension = (PNVME_SRB_EXTENSION)Srb->SrbExtension;

    if (pDevExt == NULL || pSrbExtension == NULL) {
        return FALSE;
    }

    /* ROS: NS1 will reside on HBA:0:0 */
    if (Srb->PathId != VALID_NVME_PATH_ID || Srb->TargetId != VALID_NVME_TARGET_ID) {
        /* Dont explicitely ask for new requests on this LUN */
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        ScsiPortNotification(RequestComplete, DeviceExtension,Srb);
        return bRet;
    }

    /* We need check if we're initialized correctly here */
    if (pDevExt->NvmeCntrlState != NVMeStartComplete) {
        Srb->SrbStatus = SRB_STATUS_BUSY;
        ScsiPortNotification(RequestComplete, DeviceExtension,Srb);
        ScsiPortNotification(NextLuRequest,
                            DeviceExtension,
                            Srb->PathId,
                            Srb->TargetId,
                            Srb->Lun);
        return bRet;
    }

    switch (Srb->Function) {
        case SRB_FUNCTION_EXECUTE_SCSI: {
            NVMeExecuteSrb(Srb, pDevExt);
        } break;

        default: {
            DbgPrint("Unsupported Function 0x%x\n", Srb->Function);
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        } break;
    }

    ScsiPortNotification(RequestComplete, DeviceExtension,Srb);
    ScsiPortNotification(NextLuRequest,
                        DeviceExtension,
                        Srb->PathId,
                        Srb->TargetId,
                        Srb->Lun);
    return bRet;
}

VOID
NTAPI NVMeHwDmaStarted(
  _In_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);
}

VOID
NVMeFillPortConfiguration(PNVME_DEVICE_EXTENSION pAE, PPORT_CONFIGURATION_INFORMATION portcfg)
{
    portcfg->MaximumTransferLength = pAE->InitInfo.MaxTxSize; 
    portcfg->NumberOfPhysicalBreaks = pAE->InitInfo.MaxTxSize/PAGE_SIZE;
    portcfg->AlignmentMask = BUFFER_ALIGNMENT_MASK;
    portcfg->InitiatorBusId[0] = 1;
    portcfg->CachesData = FALSE;
    portcfg->MapBuffers = FALSE;
    /* 
    * We can (technically) avoid the whole Adapter things and manage the DMA ourselves but its okay for now.
    * TODO: Revisit and see if this is needed
    */
    portcfg->NeedPhysicalAddresses = TRUE;
    portcfg->Master = TRUE;
    portcfg->MaximumNumberOfTargets = 1;
    portcfg->DeviceExtensionSize = sizeof(NVME_DEVICE_EXTENSION);
    portcfg->MaximumNumberOfLogicalUnits = 1;
    portcfg->NumberOfBuses = 1;
    portcfg->ScatterGather = TRUE;
    // Dont need more context right now but we will need it in the furture
    //portcfg->SrbExtensionSize = sizeof(NVME_SRB_EXTENSION);
}

/*******************************************************************************
 * NVMeHwFindAdapter
 *
 * @brief This function gets called to fill in the Port Configuration
 *        Information structure that indicates more capabillites the adapter
 *        supports.
 *
 * @param Context - Pointer to hardware device extension.
 * @param Reserved1 - Unused.
 * @param Reserved2 - Unused.
 * @param ArgumentString - DriverParameter string.
 * @param pPCI - Pointer to PORT_CONFIGURATION_INFORMATION structure.
 * @param Reserved3 - Unused.
 *
 * @return ULONG
 *     Returns status based upon results of adapter parameter acquisition.
 ******************************************************************************/
ULONG NTAPI
NVMeHwFindAdapter(
    PVOID Context,
    PVOID Reserved1,
    PVOID Reserved2,
    PCHAR ArgumentString,
    PPORT_CONFIGURATION_INFORMATION pPCI,
    UCHAR* Reserved3)
{
    PNVME_DEVICE_EXTENSION pAE = (PNVME_DEVICE_EXTENSION)Context;
    PPCI_NVM_DEVICE_INFO pPciNvmDevInfo = (PPCI_NVM_DEVICE_INFO)Reserved1;

    /* Initialize the hardware device extension structure. */
    RtlZeroMemory(pAE, sizeof(NVME_DEVICE_EXTENSION));

    pAE->SlotNumber = pPCI->SlotNumber;
    pAE->SystemIoBusNumber = pPCI->SystemIoBusNumber;
    pAE->pPCI = pPCI;

    /*
     * Pre-program with default values in case of failure in accessing Registry
     * Defaultly, it can support up to 16 LUNs per target
     */
    pAE->InitInfo.Namespaces = DFT_NAMESPACES;

    /* Max transfer size is 1MB by default */
    pAE->InitInfo.MaxTxSize = DFT_TX_SIZE;
    pAE->PRPListSize = ((pAE->InitInfo.MaxTxSize / PAGE_SIZE) * sizeof(UINT64));

    /* 128 entries by default for Admin queue. */
    pAE->InitInfo.AdQEntries = DFT_AD_QUEUE_ENTRIES;

    /* 1024 entries by default for IO queues. */
    pAE->InitInfo.IoQEntries = DFT_IO_QUEUE_ENTRIES;

    /* Interrupt coalescing by default: 8 millisecond/16 completions. */
    pAE->InitInfo.IntCoalescingTime = DFT_INT_COALESCING_TIME;
    pAE->InitInfo.IntCoalescingEntry = DFT_INT_COALESCING_ENTRY;

    pAE->InitInfo.IoQEntries = 1;

    /* updte in case someone used the registry to change MaxTxSie */
    pAE->PRPListSize = ((pAE->InitInfo.MaxTxSize / PAGE_SIZE) * sizeof(UINT64));

    /* Populate all PORT_CONFIGURATION_INFORMATION fields... */
    pPCI->MaximumTransferLength = pAE->InitInfo.MaxTxSize;
    pPCI->NumberOfPhysicalBreaks = pAE->InitInfo.MaxTxSize / PAGE_SIZE;
    pPCI->NumberOfBuses = 1;
    pPCI->ScatterGather = TRUE;
    pPCI->AlignmentMask = BUFFER_ALIGNMENT_MASK;  /* Double WORD Aligned */
    pPCI->CachesData = FALSE;
    pPCI->ResetTargetSupported = TRUE;
    pPCI->MaximumNumberOfTargets = 1;
    pPCI->MaximumNumberOfLogicalUnits = (UCHAR)pAE->InitInfo.Namespaces;
    pPCI->SrbExtensionSize = sizeof(NVME_SRB_EXTENSION);
    pPCI->InterruptMode = LevelSensitive;

    NVMeFillPortConfiguration(pAE, pPCI);

    ScsiPortGetUncachedExtension(pAE, pPCI, 0);

    /*
    * This is the main function which will init the controller
    * Allocate and init Admin queues and setup the controller with all the required values
    */
    if (!FindAndSetupNvmeController(pAE, pPciNvmDevInfo)){
        DbgPrint("Failed to init NVMe controller. Bus 0x%x Slot 0x%x", pPciNvmDevInfo->Bus, pPciNvmDevInfo->Slot);
        goto ERR_FREE_MEM;
    }

    /* Alright lets enable the controller */
    NVMe_CONTROLLER_CONFIGURATION CC = {0};
    CC.EN = 1;
    CC.CSS = NVME_CC_NVM_CMD;
    CC.MPS = (PAGE_SIZE >> NVME_MEM_PAGE_SIZE_SHIFT);
    CC.AMS = NVME_CC_ROUND_ROBIN;
    CC.SHN = NVME_CC_SHUTDOWN_NONE;
    /* ROS wont be used with proprietary hw anytime soon. Lets use defaults */
    CC.IOCQES = 4;
    CC.IOSQES = 6;
    ScsiPortWriteRegisterUlong((PULONG)(&pAE->pCtrlRegister->CC), CC.AsUlong);

    /* Need to ensure it's enabled in CSTS */
    if(FALSE == NVMeWaitForReady(pAE, 1)) {
        goto ERR_FREE_MEM;
    }

    if (NVMeSendIdentifyController(pAE) == FALSE) {
        DbgPrint("Failed to indentify controller\n");
        goto ERR_FREE_MEM;
    }

    if (NVMeInitNS(pAE) == FALSE) {
        DbgPrint("Failed to Init NS\n");
        goto ERR_FREE_MEM;
    }

    if (NVMeInitIOQueues(pAE) == FALSE) {
        DbgPrint("Failed to init IO queues\n");
        goto ERR_FREE_MEM;
    }

    pAE->NvmeCntrlState = NVMeStartComplete;

    return(SP_RETURN_FOUND);

ERR_FREE_MEM:
    NVMeFreeMemory(pAE);
    pAE->NvmeCntrlState = NVMeStateFailed;
    return SP_RETURN_NOT_FOUND;
} /* NVMeFindAdapter */


BOOLEAN
NTAPI NVMeHwResetBus(
  _In_ PVOID DeviceExtension,
  _In_ ULONG PathId)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(PathId);
    return TRUE;
}



ULONG NTAPI DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HW_INITIALIZATION_DATA hwInitData = { 0 };
    BOOLEAN isNvmeDevPresent = FALSE;
    PCI_NVM_DEVICE_INFO pciNvmeDeviceInfo = {0};
    PCI_COMMON_CONFIG pciData = {0};

    hwInitData.HwInitialize = NVMeHwInitialize;
    hwInitData.HwStartIo = NVMeHwStartIo;
    hwInitData.HwFindAdapter = NVMeHwFindAdapter;
    hwInitData.HwResetBus = NVMeHwResetBus;
    hwInitData.HwInterrupt = NVMeHwInterrupt;

    /* Specifiy adapter specific information.*/
    hwInitData.AutoRequestSense = FALSE;
    hwInitData.NeedPhysicalAddresses = FALSE;
    hwInitData.NumberOfAccessRanges = 2;
    hwInitData.AdapterInterfaceType = PCIBus;
    hwInitData.TaggedQueuing = TRUE;
    hwInitData.MultipleRequestPerLu = TRUE;
    hwInitData.HwDmaStarted = NULL;
    hwInitData.HwAdapterState = NULL;

    /* Set required extension sizes. */
    hwInitData.DeviceExtensionSize = sizeof(NVME_DEVICE_EXTENSION);

    /* Lets see if we have NVMe device */
    do {
        Status = ScanBusAndFindNvme(&isNvmeDevPresent, &pciNvmeDeviceInfo.Bus, &pciNvmeDeviceInfo.Slot);

        if (!NT_SUCCESS(Status) || !isNvmeDevPresent)
           break;

        ULONG retData = HalGetBusData(PCIConfiguration, pciNvmeDeviceInfo.Bus, pciNvmeDeviceInfo.Slot, &pciData, PCI_COMMON_HDR_LENGTH);

        if (retData == 0 || retData == 2) {
            break;
        }

        sprintf(pciNvmeDeviceInfo.VendorIDStr, "%4.4lx", pciData.VendorID);
        sprintf(pciNvmeDeviceInfo.DeviceIDStr, "%4.4lx", pciData.DeviceID);
        pciNvmeDeviceInfo.VendorID = pciData.VendorID;
        pciNvmeDeviceInfo.DeviceID = pciData.DeviceID;

        hwInitData.VendorId = (PVOID)pciNvmeDeviceInfo.VendorIDStr;
        hwInitData.DeviceId = (PVOID)pciNvmeDeviceInfo.DeviceIDStr;
        hwInitData.VendorIdLength = 4;
        hwInitData.DeviceIdLength = 4;
    } while(FALSE);

    /* Call ScsiPortInitialize to register with hwInitData */
    Status = ScsiPortInitialize(DriverObject,
                               RegistryPath,
                               &hwInitData,
                               &pciNvmeDeviceInfo);

    return Status;
}
