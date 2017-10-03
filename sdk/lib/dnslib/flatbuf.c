/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/flatbuf.c
 * PURPOSE:     Functions for managing the Flat Buffer Implementation (FLATBUF)
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
WINAPI
FlatBuf_Init(IN PFLATBUFF FlatBuffer,
             IN PVOID Buffer,
             IN SIZE_T Size)
{
    /* Set up the Flat Buffer start, current and ending position */
    FlatBuffer->Buffer = Buffer;
    FlatBuffer->BufferPos = (ULONG_PTR)Buffer;
    FlatBuffer->BufferEnd = (PVOID)(FlatBuffer->BufferPos + Size);

    /* Setup the current size and the available size */
    FlatBuffer->BufferSize = FlatBuffer->BufferFreeSize = Size;
}

PVOID
WINAPI
FlatBuf_Arg_Reserve(IN OUT PULONG_PTR Position,
                    IN OUT PSIZE_T FreeSize,
                    IN SIZE_T Size,
                    IN ULONG Align)
{
    ULONG_PTR NewPosition, OldPosition = *Position;
    SIZE_T NewFreeSize = *FreeSize;

    /* Start by aligning our position */
    if (Align) OldPosition += (Align - 1) & ~Align;

    /* Update it */
    NewPosition = OldPosition + Size;

    /* Update Free Size */
    NewFreeSize += (OldPosition - NewPosition);

    /* Save new values */
    *Position = NewPosition;
    *FreeSize = NewFreeSize;

    /* Check if we're out of space or not */
    if (NewFreeSize > 0) return (PVOID)OldPosition;
    return NULL;
}

PVOID
WINAPI
FlatBuf_Arg_CopyMemory(IN OUT PULONG_PTR Position,
                       IN OUT PSIZE_T FreeSize,
                       IN PVOID Buffer,
                       IN SIZE_T Size,
                       IN ULONG Align)
{
    PVOID Destination;

    /* First reserve the memory */
    Destination = FlatBuf_Arg_Reserve(Position, FreeSize, Size, Align);
    if (Destination)
    {
        /* We have space, do the copy */
        RtlCopyMemory(Destination, Buffer, Size);
    }

    /* Return the pointer to the data */
    return Destination;
}

PVOID
WINAPI
FlatBuf_Arg_WriteString(IN OUT PULONG_PTR Position,
                        IN OUT PSIZE_T FreeSize,
                        IN PVOID String,
                        IN BOOLEAN IsUnicode)
{
    PVOID Destination;
    SIZE_T StringLength;
    ULONG Align;

    /* Calculate the string length */
    if (IsUnicode)
    {
        /* Get the length in bytes and use WCHAR alignment */
        StringLength = (wcslen((LPWSTR)String) + 1) * sizeof(WCHAR);
        Align = sizeof(WCHAR);
    }
    else
    {
        /* Get the length in bytes and use CHAR alignment */
        StringLength = strlen((LPSTR)String) + 1;
        Align = sizeof(CHAR);
    }

    /* Now reserve the memory */
    Destination = FlatBuf_Arg_Reserve(Position, FreeSize, StringLength, Align);
    if (Destination)
    {
        /* We have space, do the copy */
        RtlCopyMemory(Destination, String, StringLength);
    }

    /* Return the pointer to the data */
    return Destination;
}

