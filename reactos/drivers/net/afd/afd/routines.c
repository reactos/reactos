/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/routines.c
 * PURPOSE:     Support routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/02-2001 Created
 */
#include <afd.h>


ULONG WSABufferSize(
    LPWSABUF Buffers,
    DWORD BufferCount)
{
    ULONG i;
    LPWSABUF p;
    ULONG Count = 0;

    p = Buffers;
    for (i = 0; i < BufferCount; i++) {
        Count += p->len;
        p++;
    }

    AFD_DbgPrint(MAX_TRACE, ("Buffer is %d bytes.\n", Count));

    return Count;
}


NTSTATUS MergeWSABuffers(
    LPWSABUF Buffers,
    DWORD BufferCount,
    PVOID Destination,
    ULONG MaxLength,
    PULONG BytesCopied)
{
    NTSTATUS Status;
    ULONG Length;
    LPWSABUF p;
    ULONG i;

    *BytesCopied = 0;
    if (BufferCount == 0)
        return STATUS_SUCCESS;

    p = Buffers;

    AFD_DbgPrint(MAX_TRACE, ("Destination is 0x%X\n", Destination));
    AFD_DbgPrint(MAX_TRACE, ("p is 0x%X\n", p));

    for (i = 0; i < BufferCount; i++) {
        Length = p->len;
        if (Length > MaxLength)
            /* Don't copy out of bounds */
            Length = MaxLength;

        RtlCopyMemory(Destination, p->buf, Length);
        Destination += Length;
        AFD_DbgPrint(MAX_TRACE, ("Destination is 0x%X\n", Destination));
        p++;
        AFD_DbgPrint(MAX_TRACE, ("p is 0x%X\n", p));

        *BytesCopied += Length;

        MaxLength -= Length;
        if (MaxLength == 0)
            /* Destination buffer is full */
            break;
    }

    return STATUS_SUCCESS;
}


/* EOF */
