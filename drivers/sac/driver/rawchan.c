/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/rawchan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
RawChannelCreate(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Allocate the output buffer */
    Channel->OBuffer = SacAllocatePool(SAC_RAW_OBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->OBuffer);

    /* Allocate the input buffer */
    Channel->IBuffer = SacAllocatePool(SAC_RAW_IBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->IBuffer);

    /* Reset all flags and return success */
    Channel->OBufferIndex = 0;
    Channel->OBufferFirstGoodIndex = 0;
    Channel->ChannelHasNewIBufferData = FALSE;
    Channel->ChannelHasNewOBufferData = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RawChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Free the buffer and then destroy the channel */
    if (Channel->OBuffer) SacFreePool(Channel->OBuffer);
    if (Channel->IBuffer) SacFreePool(Channel->IBuffer);
    return ChannelDestroy(Channel);
}

FORCEINLINE
BOOLEAN
ChannelHasNewOBufferData(IN PSAC_CHANNEL Channel)
{
    return Channel->ChannelHasNewOBufferData;
}

NTSTATUS
NTAPI
RawChannelORead(IN PSAC_CHANNEL Channel,
                IN PCHAR Buffer,
                IN ULONG BufferSize,
                OUT PULONG ByteCount)
{
    NTSTATUS Status;
    ULONG NextIndex;

    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER3(BufferSize > 0);
    CHECK_PARAMETER4(ByteCount);

    *ByteCount = 0;

    if (ChannelHasNewOBufferData(Channel))
    {
        Status = STATUS_SUCCESS;

        while (TRUE)
        {
            Buffer[(*ByteCount)++] = Channel->OBuffer[Channel->OBufferFirstGoodIndex];

            NextIndex = (Channel->OBufferFirstGoodIndex + 1) & (SAC_OBUFFER_SIZE - 1);
            Channel->OBufferFirstGoodIndex = NextIndex;

            if (NextIndex == Channel->OBufferIndex)
            {
                _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 0);
                break;
            }

            ASSERT(*ByteCount > 0);

            if (*ByteCount >= BufferSize) break;
        }
    }
    else
    {
        Status = STATUS_NO_DATA_DETECTED;
    }

    if (Channel->OBufferFirstGoodIndex == Channel->OBufferIndex)
    {
        ASSERT(ChannelHasNewOBufferData(Channel) == FALSE);
    }

    if (ChannelHasNewOBufferData(Channel) == FALSE)
    {
        ASSERT(Channel->OBufferFirstGoodIndex == Channel->OBufferIndex);
    }

    return Status;
}

NTSTATUS
NTAPI
RawChannelOEcho(IN PSAC_CHANNEL Channel,
                IN PCHAR String,
                IN ULONG Length)
{
    NTSTATUS Status = STATUS_SUCCESS;

    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    if (Length)
    {
        Status = ConMgrWriteData(Channel, String, Length);
        if (NT_SUCCESS(Status)) ConMgrFlushData(Channel);
    }

    return Status;
}

NTSTATUS
NTAPI
RawChannelOWrite2(IN PSAC_CHANNEL Channel,
                  IN PCHAR String,
                  IN ULONG Size)
{
    BOOLEAN Overflow;
    ULONG i, NextIndex;

    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    Overflow = FALSE;

    for (i = 0; i < Size; i++)
    {
        if ((Channel->OBufferIndex == Channel->OBufferFirstGoodIndex) &&
            ((i) || (ChannelHasNewOBufferData(Channel))))
        {
            Overflow = TRUE;
        }

        ASSERT(Channel->OBufferIndex < SAC_RAW_OBUFFER_SIZE);

        Channel->OBuffer[Channel->OBufferIndex] = String[i];

        NextIndex = (Channel->OBufferIndex + 1) & (SAC_RAW_OBUFFER_SIZE - 1);
        Channel->OBufferIndex = NextIndex;

        if (Overflow) Channel->OBufferFirstGoodIndex = NextIndex;
    }

    _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 1);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RawChannelOFlush(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    ULONG ByteCount;
    CHAR Dummy;
    CHECK_PARAMETER1(Channel);

    while (ChannelHasNewOBufferData(Channel))
    {
        Status = RawChannelORead(Channel, &Dummy, sizeof(Dummy), &ByteCount);
        if (!NT_SUCCESS(Status)) return Status;

        CHECK_PARAMETER_WITH_STATUS(ByteCount == 1, STATUS_UNSUCCESSFUL);

        Status = ConMgrWriteData(Channel, &Dummy, sizeof(Dummy));
        if (!NT_SUCCESS(Status)) return Status;
    }

    return ConMgrFlushData(Channel);
}

NTSTATUS
NTAPI
RawChannelOWrite(IN PSAC_CHANNEL Channel,
                 IN PCHAR String,
                 IN ULONG Length)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    if ((ConMgrIsWriteEnabled(Channel)) && (Channel->WriteEnabled))
    {
        return RawChannelOEcho(Channel, String, Length);
    }

    return RawChannelOWrite2(Channel, String, Length);
}

ULONG
NTAPI
RawChannelGetIBufferIndex(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    ASSERT(Channel->IBufferIndex < SAC_RAW_IBUFFER_SIZE);

    /* Return the current buffer index */
    return Channel->IBufferIndex;
}

VOID
NTAPI
RawChannelSetIBufferIndex(IN PSAC_CHANNEL Channel,
                          IN ULONG BufferIndex)
{
    NTSTATUS Status;
    ASSERT(Channel);
    ASSERT(Channel->IBufferIndex < SAC_RAW_IBUFFER_SIZE);

    /* Set the new index, and if it's not zero, it means we have data */
    Channel->IBufferIndex = BufferIndex;
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, BufferIndex != 0);

    /* If we have new data, and an event has been registered... */
    if (!(Channel->IBufferIndex) &&
        (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT))
    {
        /* Go ahead and signal it */
        ChannelClearEvent(Channel, HasNewDataEvent);
        UNREFERENCED_PARAMETER(Status);
    }
}

NTSTATUS
NTAPI
RawChannelIRead(IN PSAC_CHANNEL Channel,
                IN PCHAR Buffer,
                IN ULONG BufferSize,
                IN PULONG ReturnBufferSize)
{
    ULONG CopyChars;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* Assume failure */
    *ReturnBufferSize = 0;

    /* Check how many bytes are in the buffer */
    if (Channel->ChannelInputBufferLength(Channel) == 0)
    {
        /* Apparently nothing. Make sure the flag indicates so too */
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);
    }
    else
    {
        /* Use the smallest number of bytes either in the buffer or requested */
        CopyChars = min(Channel->ChannelInputBufferLength(Channel), BufferSize);
        ASSERT(CopyChars <= Channel->ChannelInputBufferLength(Channel));

        /* Copy them into the caller's buffer */
        RtlCopyMemory(Buffer, Channel->IBuffer, CopyChars);

        /* Update the channel's index past the copied (read) bytes */
        RawChannelSetIBufferIndex(Channel,
                                  RawChannelGetIBufferIndex(Channel) - CopyChars);

        /* Are there still bytes that haven't been read yet? */
        if (Channel->ChannelInputBufferLength(Channel))
        {
            /* Shift them up in the buffer */
            RtlMoveMemory(Channel->IBuffer,
                          &Channel->IBuffer[CopyChars],
                          Channel->ChannelInputBufferLength(Channel));
        }

        /* Return the number of bytes we actually copied */
        *ReturnBufferSize = CopyChars;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RawChannelIBufferIsFull(IN PSAC_CHANNEL Channel,
                        OUT PBOOLEAN BufferStatus)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(BufferStatus);

    /* If the index is beyond the length, the buffer must be full */
    *BufferStatus = RawChannelGetIBufferIndex(Channel) > SAC_RAW_IBUFFER_SIZE;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
RawChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);

    /* The index is the current length (since we're 0-based) */
    return RawChannelGetIBufferIndex(Channel);
}

WCHAR
NTAPI
RawChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    UCHAR LastChar = 0;
    ASSERT(Channel);

    /* Check if there's anything to read in the buffer */
    if (Channel->ChannelInputBufferLength(Channel))
    {
        /* Go back one character */
        RawChannelSetIBufferIndex(Channel,
                                  RawChannelGetIBufferIndex(Channel) - 1);

        /* Read it, and clear its current value */
        LastChar = Channel->IBuffer[RawChannelGetIBufferIndex(Channel)];
        Channel->IBuffer[RawChannelGetIBufferIndex(Channel)] = ANSI_NULL;
    }

    /* Return the last character */
    return LastChar;
}

NTSTATUS
NTAPI
RawChannelIWrite(IN PSAC_CHANNEL Channel,
                 IN PCHAR Buffer,
                 IN ULONG BufferSize)
{
    NTSTATUS Status;
    BOOLEAN IsFull;
    ULONG Index;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* First, check if the input buffer still has space */
    Status = RawChannelIBufferIsFull(Channel, &IsFull);
    if (!NT_SUCCESS(Status)) return Status;
    if (IsFull) return STATUS_UNSUCCESSFUL;

    /* Get the current buffer index */
    Index = RawChannelGetIBufferIndex(Channel);
    if ((SAC_RAW_IBUFFER_SIZE - Index) < BufferSize)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the new data */
    RtlCopyMemory(&Channel->IBuffer[Index], Buffer, BufferSize);

    /* Update the index */
    RawChannelSetIBufferIndex(Channel, BufferSize + Index);

    /* Signal the event, if one was set */
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        ChannelSetEvent(Channel, HasNewDataEvent);
    }

    /* All done */
    return STATUS_SUCCESS;
}
