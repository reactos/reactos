/*++


Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    history.c

Abstract:

    Packet history routines for CLASSPNP

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"
#include "debug.h"

#ifdef DEBUG_USE_WPP
#include "history.tmh"
#endif

//#ifdef ALLOC_PRAGMA
//    #pragma alloc_text(PAGE, InitializeTransferPackets)
//#endif

VOID HistoryInitializeRetryLogs(_Out_ PSRB_HISTORY History, ULONG HistoryCount) {
    ULONG tmpSize = HistoryCount * sizeof(SRB_HISTORY_ITEM);
    tmpSize += sizeof(SRB_HISTORY) - sizeof(SRB_HISTORY_ITEM);
    RtlZeroMemory(History, tmpSize);
    History->TotalHistoryCount = HistoryCount;
    return;
}


VOID HistoryLogSendPacket(TRANSFER_PACKET * Pkt) {

    PSRB_HISTORY history;
    PSRB_HISTORY_ITEM item;

    NT_ASSERT( Pkt->RetryHistory != NULL );
    history = Pkt->RetryHistory;
    // sending a packet implies a new history unit is to be used.
    NT_ASSERT( history->UsedHistoryCount <= history->TotalHistoryCount );

    // if already all used up, request class driver to remove at least one history unit
    if (history->UsedHistoryCount == history->TotalHistoryCount )
    {
        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Pkt->Fdo->DeviceExtension;
        PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
        NT_ASSERT( fdoData->InterpretSenseInfo != NULL );
        NT_ASSERT( fdoData->InterpretSenseInfo->Compress != NULL );
        fdoData->InterpretSenseInfo->Compress( fdoExtension->DeviceObject, history );
        NT_ASSERT( history->UsedHistoryCount < history->TotalHistoryCount );
    }

    // thus, since we are about to increment the count, it must now be less...
    NT_ASSERT( history->UsedHistoryCount < history->TotalHistoryCount );

    // increment the number of history units in use
    history->UsedHistoryCount++;

    // determine index to use
    item = &( history->History[ history->UsedHistoryCount-1 ] );

    // zero out the history item
    RtlZeroMemory(item, sizeof(SRB_HISTORY_ITEM));

    // Query the tick count and store in the history
    KeQueryTickCount(&item->TickCountSent);
    return;
}

VOID HistoryLogReturnedPacket(TRANSFER_PACKET *Pkt) {

    PSRB_HISTORY history;
    PSRB_HISTORY_ITEM item;
    UCHAR senseSize;
    PVOID senseInfoBuffer;
    UCHAR senseInfoBufferLength;
    SENSE_DATA convertedSenseBuffer = {0};
    BOOLEAN validSense = TRUE;

    NT_ASSERT( Pkt->RetryHistory != NULL );
    history = Pkt->RetryHistory;
    NT_ASSERT( history->UsedHistoryCount <= history->TotalHistoryCount );
    item = &( history->History[ history->UsedHistoryCount-1 ] );

    // Query the tick count and store in the history
    KeQueryTickCount(&item->TickCountCompleted);

    // Copy the SRB Status...
    item->SrbStatus = Pkt->Srb->SrbStatus;

    //
    // Process sense data
    //

    senseInfoBuffer = ClasspTransferPacketGetSenseInfoBuffer(Pkt);
    senseInfoBufferLength = ClasspTransferPacketGetSenseInfoBufferLength(Pkt);

    if (IsDescriptorSenseDataFormat(senseInfoBuffer)) {

        validSense = ScsiConvertToFixedSenseFormat(senseInfoBuffer,
                                                   senseInfoBufferLength,
                                                   (PVOID)&convertedSenseBuffer,
                                                   sizeof(convertedSenseBuffer));

        if (validSense) {
            senseInfoBuffer = (PVOID)&convertedSenseBuffer;
            senseInfoBufferLength = sizeof(convertedSenseBuffer);
        }
    }

    RtlZeroMemory(&(item->NormalizedSenseData), sizeof(item->NormalizedSenseData));

    if (validSense) {

        // Determine the amount of valid sense data

        if (!ScsiGetTotalSenseByteCountIndicated(senseInfoBuffer,
                                                 senseInfoBufferLength,
                                                 &senseSize)) {
            senseSize = senseInfoBufferLength;
        }

        // Normalize the sense data copy in the history
        senseSize = min(senseSize, sizeof(item->NormalizedSenseData));
        RtlCopyMemory(&(item->NormalizedSenseData),
                      senseInfoBuffer,
                      senseSize
                      );
    }

    return;
}

