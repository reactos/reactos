/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/rawchan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

#define SAC_RAW_OBUFFER_SIZE 0x2000
#define SAC_RAW_IBUFFER_SIZE 0x2000

NTSTATUS
NTAPI
RawChannelCreate(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    Channel->OBuffer = SacAllocatePool(SAC_RAW_OBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->OBuffer);

    Channel->IBuffer = SacAllocatePool(SAC_RAW_IBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->IBuffer);

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

    if (Channel->OBuffer)
    {
        SacFreePool(Channel->OBuffer);
    }

    if (Channel->IBuffer)
    {
        SacFreePool(Channel->IBuffer);
    }

    return ChannelDestroy(Channel);
}

FORCEINLINE
BOOLEAN
ChannelHasNewOBufferData(IN PSAC_CHANNEL Channel)
{
    return Channel->ChannelHasNewOBufferData;
}

FORCEINLINE
BOOLEAN
ChannelHasNewIBufferData(IN PSAC_CHANNEL Channel)
{
    return Channel->ChannelHasNewIBufferData;
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

ULONG
NTAPI
RawChannelGetIBufferIndex(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    ASSERT(Channel->IBufferIndex < SAC_RAW_IBUFFER_SIZE);

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

    Channel->IBufferIndex = BufferIndex;
    Channel->ChannelHasNewIBufferData = BufferIndex != 0;

    if (!Channel->IBufferIndex)
    {
        if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
        {
            ChannelClearEvent(Channel, HasNewDataEvent);
            UNREFERENCED_PARAMETER(Status);
        }
    }
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

    *ReturnBufferSize = 0;

    if (Channel->ChannelInputBufferLength(Channel) == 0)
    {
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);
    }
    else
    {
        CopyChars = Channel->ChannelInputBufferLength(Channel);
        if (CopyChars > BufferSize) CopyChars = BufferSize;
        ASSERT(CopyChars <= Channel->ChannelInputBufferLength(Channel));

        RtlCopyMemory(Buffer, Channel->IBuffer, CopyChars);

        RawChannelSetIBufferIndex(Channel,
                                  RawChannelGetIBufferIndex(Channel) - CopyChars);

        if (Channel->ChannelInputBufferLength(Channel))
        {
            RtlMoveMemory(Channel->IBuffer,
                          &Channel->IBuffer[CopyChars],
                          Channel->ChannelInputBufferLength(Channel));
        }

        *ReturnBufferSize = CopyChars;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RawChannelIBufferIsFull(IN PSAC_CHANNEL Channel,
                        OUT PBOOLEAN BufferStatus)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(BufferStatus);

    *BufferStatus = RawChannelGetIBufferIndex(Channel) > SAC_RAW_IBUFFER_SIZE;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
RawChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    return RawChannelGetIBufferIndex(Channel);
}

CHAR
NTAPI
RawChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    UCHAR LastChar = 0;

    ASSERT(Channel);

    if (Channel->ChannelInputBufferLength(Channel))
    {
        RawChannelSetIBufferIndex(Channel, RawChannelGetIBufferIndex(Channel) - 1);

        LastChar = Channel->IBuffer[RawChannelGetIBufferIndex(Channel)];
        Channel->IBuffer[RawChannelGetIBufferIndex(Channel)] = 0;
    }

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

    Status = RawChannelIBufferIsFull(Channel, &IsFull);
    if (!NT_SUCCESS(Status)) return Status;

    if (IsFull) return STATUS_UNSUCCESSFUL;

    Index = RawChannelGetIBufferIndex(Channel);
    if ((SAC_RAW_IBUFFER_SIZE - Index) >= BufferSize) return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(&Channel->IBuffer[Index], Buffer, BufferSize);

    RawChannelSetIBufferIndex(Channel, BufferSize + Index);

    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        ChannelSetEvent(Channel, HasNewDataEvent);
    }

    return STATUS_SUCCESS;
}
