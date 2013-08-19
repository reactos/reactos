/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/vtutf8chan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

CHAR IncomingUtf8ConversionBuffer[4];
WCHAR IncomingUnicodeValue;

/* FUNCTIONS *****************************************************************/

typedef struct _SAC_CURSOR_DATA
{
    UCHAR CursorX;
    UCHAR CursorY;
    UCHAR CursorVisible;
    WCHAR CursorValue;
} SAC_CURSOR_DATA, *PSAC_CURSOR_DATA;

C_ASSERT(sizeof(SAC_CURSOR_DATA) == 6);

#define SAC_VTUTF8_OBUFFER_SIZE 0x2D00
#define SAC_VTUTF8_IBUFFER_SIZE 0x2000

NTSTATUS
NTAPI
VTUTF8ChannelOInit(IN PSAC_CHANNEL Channel)
{
    PSAC_CURSOR_DATA Cursor;
    ULONG x, y;
    CHECK_PARAMETER(Channel);

    /* Set the current channel cursor parameters */
    Channel->CursorVisible = 0;
    Channel->CursorX = 40;
    Channel->CursorY = 37;

    /* Loop the output buffer height by width */
    Cursor = (PSAC_CURSOR_DATA)Channel->OBuffer;
    y = SAC_VTUTF8_COL_HEIGHT - 1;
    do
    {
        x = SAC_VTUTF8_COL_WIDTH;
        do
        {
            /* For every character, set the defaults */
            Cursor->CursorValue = ' ';
            Cursor->CursorX = 40;
            Cursor->CursorY = 38;

            /* Move to the next character */
            Cursor++;
        } while (--x);
    } while (--y);

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelCreate(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Allocate the output buffer */
    Channel->OBuffer = SacAllocatePool(SAC_VTUTF8_OBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->OBuffer);

    /* Allocate the input buffer */
    Channel->IBuffer = SacAllocatePool(SAC_VTUTF8_IBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->IBuffer);

    /* Initialize the output stream */
    Status = VTUTF8ChannelOInit(Channel);
    if (NT_SUCCESS(Status)) return Status;

    /* Reset all flags and return success */
    _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 0);
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, 0);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Free the buffer and then destroy the channel */
    if (Channel->OBuffer) SacFreePool(Channel->OBuffer);
    if (Channel->IBuffer) SacFreePool(Channel->IBuffer);
    return ChannelDestroy(Channel);
}

NTSTATUS
NTAPI
VTUTF8ChannelORead(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ByteCount
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
VTUTF8ChannelScanForNumber(
	IN PWCHAR String,
	OUT PULONG Number
	)
{
	return FALSE;
}

NTSTATUS
NTAPI
VTUTF8ChannelAnsiDispatch(
	IN NTSTATUS Status,
	IN ULONG AnsiCode,
	IN PWCHAR Data,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelProcessAttributes(
	IN PSAC_CHANNEL Channel,
	IN UCHAR Attribute
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelConsumeEscapeSequence(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite2(IN PSAC_CHANNEL Channel,
                     IN PCHAR String,
                     IN ULONG Size)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOEcho(IN PSAC_CHANNEL Channel,
                   IN PCHAR String,
                   IN ULONG Size)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR String,
                    IN ULONG Length)
{
    NTSTATUS Status;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    /* Call the lower level function */
    Status = VTUTF8ChannelOWrite2(Channel, String, Length / sizeof(WCHAR));
    if (NT_SUCCESS(Status))
    {
        /* Is the channel enabled for output? */
        if ((ConMgrIsWriteEnabled(Channel)) && (Channel->WriteEnabled))
        {
            /* Go ahead and output it */
            Status = VTUTF8ChannelOEcho(Channel, String, Length);
        }
        else
        {
            /* Otherwise, just remember that we have new data */
            _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 1);
        }
    }

    /* We're done */
    return Status;
}

ULONG
NTAPI
VTUTF8ChannelGetIBufferIndex(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    /* Return the current buffer index */
    return Channel->IBufferIndex;
}

VOID
NTAPI
VTUTF8ChannelSetIBufferIndex(IN PSAC_CHANNEL Channel,
                             IN ULONG BufferIndex)
{
    NTSTATUS Status;
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

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
VTUTF8ChannelIRead(IN PSAC_CHANNEL Channel,
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
        CopyChars = min(Channel->ChannelInputBufferLength(Channel) * sizeof(WCHAR),
                        BufferSize);
        ASSERT(CopyChars <= Channel->ChannelInputBufferLength(Channel));

        /* Copy them into the caller's buffer */
        RtlCopyMemory(Buffer, Channel->IBuffer, CopyChars);

        /* Update the channel's index past the copied (read) bytes */
        VTUTF8ChannelSetIBufferIndex(Channel,
                                     VTUTF8ChannelGetIBufferIndex(Channel) - CopyChars);

        /* Are there still bytes that haven't been read yet? */
        if (Channel->ChannelInputBufferLength(Channel))
        {
            /* Shift them up in the buffer */
            RtlMoveMemory(Channel->IBuffer,
                          &Channel->IBuffer[CopyChars],
                          Channel->ChannelInputBufferLength(Channel) *
                          sizeof(WCHAR));
        }

        /* Return the number of bytes we actually copied */
        *ReturnBufferSize = CopyChars;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelIBufferIsFull(IN PSAC_CHANNEL Channel,
                           OUT PBOOLEAN BufferStatus)
{
    CHECK_PARAMETER1(Channel);

    /* If the index is beyond the length, the buffer must be full */
    *BufferStatus = VTUTF8ChannelGetIBufferIndex(Channel) > SAC_VTUTF8_IBUFFER_SIZE;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
VTUTF8ChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);

    /* The index is the length, so divide by two to get character count */
    return VTUTF8ChannelGetIBufferIndex(Channel) / sizeof(WCHAR);
}

WCHAR
NTAPI
VTUTF8ChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    PWCHAR LastCharLocation;
    WCHAR LastChar = 0;
    ASSERT(Channel);

    /* Check if there's anything to read in the buffer */
    if (Channel->ChannelInputBufferLength(Channel))
    {
        /* Go back one character */
        VTUTF8ChannelSetIBufferIndex(Channel,
                                     VTUTF8ChannelGetIBufferIndex(Channel) -
                                     sizeof(WCHAR));

        /* Read it, and clear its current value */
        LastCharLocation = (PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)];
        LastChar = *LastCharLocation;
        *LastCharLocation = UNICODE_NULL;
    }

    /* Return the last character */
    return LastChar;
}

NTSTATUS
NTAPI
VTUTF8ChannelIWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR Buffer,
                    IN ULONG BufferSize)
{
    NTSTATUS Status;
    BOOLEAN IsFull;
    ULONG Index, i;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* First, check if the input buffer still has space */
    Status = VTUTF8ChannelIBufferIsFull(Channel, &IsFull);
    if (!NT_SUCCESS(Status)) return Status;
    if (IsFull) return STATUS_UNSUCCESSFUL;

    /* Get the current buffer index */
    Index = VTUTF8ChannelGetIBufferIndex(Channel);
    if ((SAC_VTUTF8_IBUFFER_SIZE - Index) < BufferSize)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the new data */
    for (i = 0; i < BufferSize; i++)
    {
        /* Convert the character */
        if (SacTranslateUtf8ToUnicode(Buffer[i],
                                      IncomingUtf8ConversionBuffer,
                                      &IncomingUnicodeValue))
        {
            /* Write it into the buffer */
            *(PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)] =
                IncomingUnicodeValue;

            /* Update the index */
            Index = VTUTF8ChannelGetIBufferIndex(Channel);
            VTUTF8ChannelSetIBufferIndex(Channel, Index + sizeof(WCHAR));
        }
    }

    /* Signal the event, if one was set */
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        ChannelSetEvent(Channel, HasNewDataEvent);
    }

    /* All done */
    return STATUS_SUCCESS;
}
