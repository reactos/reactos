#include "precomp.h"

BOOLEAN
NVMeIsCmdSuccessful(PNVMe_COMPLETION_QUEUE_ENTRY pCmpEntry)
{
    if (pCmpEntry == NULL)
        return FALSE;
    
    if (pCmpEntry->DW3.SF.SC == 0 && pCmpEntry->DW3.SF.SCT == 0) {
        return TRUE;
    }

    /* TODO: Check for retryable error we should not be failing commands */
    ASSERT(FALSE);

    DbgPrint("Failed Command CID:0x%x on SQID:0x%x SQDH:0x%x. SC: 0x%x SCT:0x%x\n", pCmpEntry->DW3.CID, 
            pCmpEntry->DW2.SQID, pCmpEntry->DW2.SQHD, pCmpEntry->DW3.SF.SC, pCmpEntry->DW3.SF.SCT);
    return FALSE;
}

BOOLEAN
NVMeIssueCmd(PNVME_DEVICE_EXTENSION pDevExtension, PNVMe_COMMAND pNvmeCmd, PNVMe_COMPLETION_QUEUE_ENTRY pRetCmpEntry, BOOLEAN isIOCmd)
{
    BOOLEAN bRet = FALSE;

    if (pNvmeCmd == NULL || pDevExtension == NULL)
        return bRet;

    PSUB_QUEUE_INFO pSubQ = &pDevExtension->QueueInfo.SubQueueInfo[isIOCmd];
    PCPL_QUEUE_INFO pCmpQ = &pDevExtension->QueueInfo.CplQueueInfo[isIOCmd];

    if (pCmpQ == NULL || pSubQ == NULL) {
        DbgPrint("IO Queues not initialized\n");
        return FALSE;
    }

    if (pSubQ->CID == 0xFFFF)
        pSubQ->CID = 1;

    pNvmeCmd->CDW0.CID = pSubQ->CID++;

    /* Get entry in the Submission Queue */ 
    PNVMe_COMMAND pCmd = (PNVMe_COMMAND)pSubQ->SubQStartVirtual;
    pCmd += pSubQ->SubQTailPtr;

    /* Increment the tail ptr */
    pSubQ->SubQTailPtr++;
    if (pSubQ->SubQTailPtr > pSubQ->SubQEntries) {
        pSubQ->SubQTailPtr = 0;
    }

    /* Copy command into submission queue */
    RtlCopyMemory(pCmd, pNvmeCmd, sizeof(NVMe_COMMAND));

    /* Ring The Door bell */ 
    WRITE_REGISTER_ULONG(pSubQ->pSubTDBL, pSubQ->SubQTailPtr);

    /* Poll for completion */ 
    PNVMe_COMPLETION_QUEUE_ENTRY pCmpEntry = (PNVMe_COMPLETION_QUEUE_ENTRY)pCmpQ->CplQStartVirtual;
    pCmpEntry += pCmpQ->CplQHeadPtr;

    while(TRUE) {
        /* 
        * On VBox, Phase for commands is not inverted. Need to test this on other configurations.
        * WTF is VBox doing. The phase tag should be inverted for each command
        * It seems VBox is inverting Phase for every new iteration over the Completion queue. 
        * So now we have to do more checks.
        * We will Zero out all of completion queue entries and check if the CID is filled by the firmware
        * This will ofcourse mean we cant use CID = 0
        * TODO: We are looping indefinately, need to stop after the CAP.TO and fail the cmd
        */
        if (pCmpEntry->DW3.CID == 0) {
            ScsiPortStallExecution(1000);
            continue;
        }

        break;
    }

    /* Ring The Door bell */ 
    WRITE_REGISTER_ULONG(pCmpQ->pCplHDBL, pCmpQ->CplQHeadPtr);

    /* Increment the completion head */
    pCmpQ->CplQHeadPtr++;
    if (pCmpQ->CplQHeadPtr > pCmpQ->CplQEntries) {
        pCmpQ->CplQHeadPtr = 0;
    }

    /* Copy the completion */
    if (pRetCmpEntry)
        RtlCopyMemory(pRetCmpEntry, pCmpEntry, sizeof(NVMe_COMPLETION_QUEUE_ENTRY));

    /* What a fucking waste. Fuck you Virtual Box */
    RtlZeroMemory(pCmpEntry, sizeof(NVMe_COMPLETION_QUEUE_ENTRY));
    RtlZeroMemory(pCmd, sizeof(NVMe_COMMAND));

    bRet = TRUE;
    return bRet;
}