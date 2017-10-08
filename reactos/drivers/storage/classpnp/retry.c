/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    retry.c

Abstract:

    Packet retry routines for CLASSPNP

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

/*
 *  InterpretTransferPacketError
 *
 *      Interpret the SRB error into a meaningful IRP status.
 *      ClassInterpretSenseInfo also may modify the SRB for the retry.
 *
 *      Return TRUE iff packet should be retried.
 */
BOOLEAN NTAPI InterpretTransferPacketError(PTRANSFER_PACKET Pkt)
{
    BOOLEAN shouldRetry = FALSE;
    PCDB pCdb = (PCDB)Pkt->Srb.Cdb;
    
    /*
     *  Interpret the error using the returned sense info first.
     */
    Pkt->RetryIntervalSec = 0;
    if (pCdb->MEDIA_REMOVAL.OperationCode == SCSIOP_MEDIUM_REMOVAL){
        /*
         *  This is an Ejection Control SRB.  Interpret its sense info specially.
         */
        shouldRetry = ClassInterpretSenseInfo(
                            Pkt->Fdo,
                            &Pkt->Srb,
                            IRP_MJ_SCSI,
                            0,
                            MAXIMUM_RETRIES - Pkt->NumRetries,
                            &Pkt->Irp->IoStatus.Status,
                            &Pkt->RetryIntervalSec);
        if (shouldRetry){
            /*
             *  If the device is not ready, wait at least 2 seconds before retrying.
             */
            PSENSE_DATA senseInfoBuffer = Pkt->Srb.SenseInfoBuffer; 
            ASSERT(senseInfoBuffer);
            if (((Pkt->Irp->IoStatus.Status == STATUS_DEVICE_NOT_READY) &&
                (senseInfoBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY)) ||
                    (SRB_STATUS(Pkt->Srb.SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)){
                    
                Pkt->RetryIntervalSec = MAX(Pkt->RetryIntervalSec, 2); 
            }
        }
    }
    else if ((pCdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE) ||
            (pCdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE10)){
        /*
         *  This is an Mode Sense SRB.  Interpret its sense info specially.
         */
        shouldRetry = ClassInterpretSenseInfo(
                            Pkt->Fdo,
                            &Pkt->Srb,
                            IRP_MJ_SCSI,
                            0,
                            MAXIMUM_RETRIES - Pkt->NumRetries,
                            &Pkt->Irp->IoStatus.Status,
                            &Pkt->RetryIntervalSec);
        if (shouldRetry){
            /*
             *  If the device is not ready, wait at least 2 seconds before retrying.
             */
            PSENSE_DATA senseInfoBuffer = Pkt->Srb.SenseInfoBuffer; 
            ASSERT(senseInfoBuffer);
            if (((Pkt->Irp->IoStatus.Status == STATUS_DEVICE_NOT_READY) &&
                (senseInfoBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY)) ||
                    (SRB_STATUS(Pkt->Srb.SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)){
                    
                Pkt->RetryIntervalSec = MAX(Pkt->RetryIntervalSec, 2); 
            }
        }
        
        /*
         *  Some special cases for mode sense.
         */
        if (Pkt->Irp->IoStatus.Status == STATUS_VERIFY_REQUIRED){
            shouldRetry = TRUE;
        }
        else if (SRB_STATUS(Pkt->Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN){
            /*
             *  This is a HACK.
             *  Atapi returns SRB_STATUS_DATA_OVERRUN when it really means 
             *  underrun (i.e. success, and the buffer is longer than needed).
             *  So treat this as a success.
             */
            Pkt->Irp->IoStatus.Status = STATUS_SUCCESS;
            InterlockedExchangeAdd((PLONG)&Pkt->OriginalIrp->IoStatus.Information, (LONG)Pkt->Srb.DataTransferLength);            
            shouldRetry = FALSE;
        }
    }            
    else if (pCdb->CDB10.OperationCode == SCSIOP_READ_CAPACITY){
        /*
         *  This is a Drive Capacity SRB.  Interpret its sense info specially.
         */
        shouldRetry = ClassInterpretSenseInfo(
                            Pkt->Fdo,
                            &Pkt->Srb,
                            IRP_MJ_SCSI,
                            0,
                            MAXIMUM_RETRIES - Pkt->NumRetries,
                            &Pkt->Irp->IoStatus.Status,
                            &Pkt->RetryIntervalSec);
        if (Pkt->Irp->IoStatus.Status == STATUS_VERIFY_REQUIRED){
            shouldRetry = TRUE;            
        }
    }
    else if ((pCdb->CDB10.OperationCode == SCSIOP_READ) ||
            (pCdb->CDB10.OperationCode == SCSIOP_WRITE)){
        /*
         *  This is a Read/Write Data packet.
         */
        PIO_STACK_LOCATION origCurrentSp = IoGetCurrentIrpStackLocation(Pkt->OriginalIrp);
        
        shouldRetry = ClassInterpretSenseInfo(
                            Pkt->Fdo,
                            &Pkt->Srb,
                            origCurrentSp->MajorFunction,
                            0,
                            MAXIMUM_RETRIES - Pkt->NumRetries,
                            &Pkt->Irp->IoStatus.Status,
                            &Pkt->RetryIntervalSec);
        /*
         *  Deal with some special cases.
         */
        if (Pkt->Irp->IoStatus.Status == STATUS_INSUFFICIENT_RESOURCES){
            /*
             *  We are in extreme low-memory stress.  
             *  We will retry in smaller chunks.
             */
            shouldRetry = TRUE;
        }
        else if (TEST_FLAG(origCurrentSp->Flags, SL_OVERRIDE_VERIFY_VOLUME) &&
                (Pkt->Irp->IoStatus.Status == STATUS_VERIFY_REQUIRED)){
            /*
             *  We are still verifying a (possibly) reloaded disk/cdrom.
             *  So retry the request.
             */
            Pkt->Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR; 
            shouldRetry = TRUE;
        }
    }
    else {
        DBGERR(("Unhandled SRB Function %xh in error path for packet %p (did miniport change Srb.Cdb.OperationCode ?)", (ULONG)pCdb->CDB10.OperationCode, Pkt));
    }

    return shouldRetry;
}

/*
 *  RetryTransferPacket
 *
 *      Retry sending a TRANSFER_PACKET.
 *
 *      Return TRUE iff the packet is complete.
 *          (if so the status in pkt->irp is the final status).
 */
BOOLEAN NTAPI RetryTransferPacket(PTRANSFER_PACKET Pkt)
{
    BOOLEAN packetDone;

    DBGTRACE(ClassDebugTrace, ("retrying failed transfer (pkt=%ph, op=%s)", Pkt, DBGGETSCSIOPSTR(&Pkt->Srb)));

    ASSERT(Pkt->NumRetries > 0);
    Pkt->NumRetries--;

    /*
     *  Tone down performance on the retry.  
     *  This increases the chance for success on the retry.
     *  We've seen instances of drives that fail consistently but then start working
     *  once this scale-down is applied.
     */
    SET_FLAG(Pkt->Srb.SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
    SET_FLAG(Pkt->Srb.SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
    CLEAR_FLAG(Pkt->Srb.SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);
    Pkt->Srb.QueueTag = SP_UNTAGGED;

    if (Pkt->Irp->IoStatus.Status == STATUS_INSUFFICIENT_RESOURCES){
        PCDB pCdb = (PCDB)Pkt->Srb.Cdb;
        BOOLEAN isReadWrite = ((pCdb->CDB10.OperationCode == SCSIOP_READ) ||
                                                (pCdb->CDB10.OperationCode == SCSIOP_WRITE));
    
        if (Pkt->InLowMemRetry || !isReadWrite){
            /*
             *  This should never happen.
             *  The memory manager guarantees that at least four pages will
             *  be available to allow forward progress in the port driver.
             *  So a one-page transfer should never fail with insufficient resources.
             */
            ASSERT(isReadWrite && !Pkt->InLowMemRetry);
            packetDone = TRUE;
        }
        else {
            /*
             *  We are in low-memory stress.  
             *  Start the low-memory retry state machine, which tries to
             *  resend the packet in little one-page chunks.
             */
            InitLowMemRetry(  Pkt, 
                                        Pkt->BufPtrCopy, 
                                        Pkt->BufLenCopy, 
                                        Pkt->TargetLocationCopy); 
            StepLowMemRetry(Pkt);
            packetDone = FALSE;
        }
    }
    else {      
        /*
         *  Retry the packet by simply resending it after a delay.
         *  Put the packet back in the pending queue and
         *  schedule a timer to retry the transfer.
         *
         *  Do not call SetupReadWriteTransferPacket again because:
         *  (1)  The minidriver may have set some bits 
         *       in the SRB that it needs again and
         *  (2)  doing so would reset numRetries.
         *
         *  BECAUSE we do not call SetupReadWriteTransferPacket again,
         *  we have to reset a couple fields in the SRB that
         *  some miniports overwrite when they fail an SRB.
         */
         
        Pkt->Srb.DataBuffer = Pkt->BufPtrCopy;
        Pkt->Srb.DataTransferLength = Pkt->BufLenCopy;
        
        if (Pkt->RetryIntervalSec == 0){
            /*
             *  Always delay by at least a little when retrying.
             *  Some problems (e.g. CRC errors) are not recoverable without a slight delay.
             */
            LARGE_INTEGER timerPeriod;

            timerPeriod.HighPart = -1;
            timerPeriod.LowPart = -(LONG)((ULONG)MINIMUM_RETRY_UNITS*KeQueryTimeIncrement());
            KeInitializeTimer(&Pkt->RetryTimer);
            KeInitializeDpc(&Pkt->RetryTimerDPC, TransferPacketRetryTimerDpc, Pkt);
            KeSetTimer(&Pkt->RetryTimer, timerPeriod, &Pkt->RetryTimerDPC);            
        }
        else {
            LARGE_INTEGER timerPeriod;

            ASSERT(Pkt->RetryIntervalSec < 100);    // sanity check
            timerPeriod.HighPart = -1;
            timerPeriod.LowPart = Pkt->RetryIntervalSec*-10000000;
            KeInitializeTimer(&Pkt->RetryTimer);
            KeInitializeDpc(&Pkt->RetryTimerDPC, TransferPacketRetryTimerDpc, Pkt);
            KeSetTimer(&Pkt->RetryTimer, timerPeriod, &Pkt->RetryTimerDPC);
        }
        packetDone = FALSE;
    }

    return packetDone;
}

VOID NTAPI TransferPacketRetryTimerDpc(IN PKDPC Dpc,
                                       IN PVOID DeferredContext,
                                       IN PVOID SystemArgument1,
                                       IN PVOID SystemArgument2)
{
    PTRANSFER_PACKET pkt = (PTRANSFER_PACKET)DeferredContext;
    SubmitTransferPacket(pkt);
}

VOID NTAPI InitLowMemRetry(PTRANSFER_PACKET Pkt, PVOID BufPtr, ULONG Len, LARGE_INTEGER TargetLocation)
{
    ASSERT(Len > 0);
    ASSERT(!Pkt->InLowMemRetry);
    Pkt->InLowMemRetry = TRUE;
    Pkt->LowMemRetry_remainingBufPtr = BufPtr;
    Pkt->LowMemRetry_remainingBufLen = Len;
    Pkt->LowMemRetry_nextChunkTargetLocation = TargetLocation;
}

/*
 *  StepLowMemRetry
 *
 *      During extreme low-memory stress, this function retries
 *      a packet in small one-page chunks, sent serially.
 *
 *      Returns TRUE iff the packet is done.
 */
BOOLEAN NTAPI StepLowMemRetry(PTRANSFER_PACKET Pkt)
{
    BOOLEAN packetDone;

    if (Pkt->LowMemRetry_remainingBufLen == 0){
        packetDone = TRUE;
    }
    else {
        ULONG thisChunkLen;
        ULONG bytesToNextPageBoundary;

        /*
         *  Make sure the little chunk we send is <= a page length
         *  AND that it does not cross any page boundaries.
         */
        bytesToNextPageBoundary = PAGE_SIZE-(ULONG)((ULONG_PTR)Pkt->LowMemRetry_remainingBufPtr%PAGE_SIZE);
        thisChunkLen = MIN(Pkt->LowMemRetry_remainingBufLen, bytesToNextPageBoundary);

        /*
         *  Set up the transfer packet for the new little chunk.
         *  This will reset numRetries so that we retry each chunk as required.
         */
        SetupReadWriteTransferPacket(Pkt, 
                                Pkt->LowMemRetry_remainingBufPtr, 
                                thisChunkLen, 
                                Pkt->LowMemRetry_nextChunkTargetLocation, 
                                Pkt->OriginalIrp);
    
        Pkt->LowMemRetry_remainingBufPtr += thisChunkLen;
        Pkt->LowMemRetry_remainingBufLen -= thisChunkLen;
        Pkt->LowMemRetry_nextChunkTargetLocation.QuadPart += thisChunkLen;

        SubmitTransferPacket(Pkt);
        packetDone = FALSE;
    }

    return packetDone;
}
