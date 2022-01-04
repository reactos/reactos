/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/splitter/filter.c
 * PURPOSE:         Filter File Context Handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

NTSTATUS
NTAPI
FilterProcess(
    IN PKSFILTER  Filter,
    IN PKSPROCESSPIN_INDEXENTRY  ProcessPinsIndex)
{
    ULONG Index;
    PKSPROCESSPIN CurPin, Pin;
    BOOLEAN PendingFrames = FALSE;

    if (ProcessPinsIndex->Count)
    {
        /* check if there are outstanding frames */
        for(Index = 1; Index < ProcessPinsIndex->Count; Index++)
        {
            /* get current pin */
            CurPin = ProcessPinsIndex->Pins[Index];

            if (CurPin->BytesAvailable && CurPin->Pin->DeviceState == KSSTATE_RUN)
            {
                /* pin has pending frames
                 * to keep all pins synchronized, every pin has to wait untill each chained pin has send its frames downwards
                 */
                PendingFrames = TRUE;
            }
        }
    }

    if (!PendingFrames && ProcessPinsIndex->Count)
    {
        /* get first pin */
        Pin = ProcessPinsIndex->Pins[0];

        /* check if there is new data available */
        if (Pin->BytesAvailable)
        {
            for(Index = 1; Index < ProcessPinsIndex->Count; Index++)
            {
                /* get current pin */
                CurPin = ProcessPinsIndex->Pins[Index];

                /* copy the frame to pin */
                RtlMoveMemory(CurPin->Data, Pin->Data, Pin->BytesAvailable);
                CurPin->BytesUsed = Pin->BytesAvailable;
            }
        }
    }
    /* done */
    return STATUS_SUCCESS;
}

