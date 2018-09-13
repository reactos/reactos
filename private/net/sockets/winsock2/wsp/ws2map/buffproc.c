/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    buffproc.c

Abstract:

    This module contains buffer managment routines for the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockCalcBufferArrayByteLength()
        SockCopyFlatBufferToBufferArray()
        SockCopyBufferArrayToFlatBuffer()

Author:

    Keith Moore (keithmo) 24-Jul-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


DWORD
SockCalcBufferArrayByteLength(
    LPWSABUF BufferArray,
    DWORD BufferCount
    )

/*++

Routine Description:

    Calculates the total byte length of a scatter/gather buffer array.

Arguments:

    BufferArray - Pointer to an array of WSABUF structures.

    BufferCount - The number of entries in BufferArray.

Return Value:

    DWORD - The total byte length of the buffers described by the
        buffer array.

--*/

{

    DWORD totalLength;

    totalLength = 0;

    while( BufferCount-- ) {

        totalLength += BufferArray->len;
        BufferArray++;

    }

    return totalLength;

}   // SockCalcBufferArrayByteLength



DWORD
SockCopyFlatBufferToBufferArray(
    LPWSABUF BufferArray,
    DWORD BufferCount,
    PVOID FlatBuffer,
    DWORD FlatBufferLength
    )

/*++

Routine Description:

    Copies a flat (linear) buffer to a scatter/gather buffer array.

Arguments:

    BufferArray - Pointer to the destination array of WSABUF structures.

    BufferCount - The number of entries in BufferArray.

    FlatBuffer - Pointer to the source flat buffer.

    FlatBufferLength - The byte length of the source flat buffer.

Return Value:

    DWORD - The number of bytes actually copied to the buffer array.

--*/

{

    DWORD bytesToCopy;
    PCHAR destination;

    destination = (PCHAR)FlatBuffer;

    while( BufferCount > 0 && FlatBufferLength > 0 ) {

        bytesToCopy = min( BufferArray->len, FlatBufferLength );

        CopyMemory(
            destination,
            BufferArray->buf,
            bytesToCopy
            );

        destination += bytesToCopy;
        FlatBufferLength -= bytesToCopy;
        BufferArray++;
        BufferCount--;

    }

    return (DWORD)(destination - (PCHAR)FlatBuffer);

}   // SockCopyFlatBufferToBufferArray



DWORD
SockCopyBufferArrayToFlatBuffer(
    PVOID FlatBuffer,
    DWORD FlatBufferLength,
    LPWSABUF BufferArray,
    DWORD BufferCount
    )

/*++

Routine Description:

    Copies the data in a scatter/gather buffer array to a flat (linear)
    buffer.

Arguments:

    FlatBuffer - Pointer to the destination flat buffer.

    FlatBufferLength - The byte length of the destination flat buffer.

    BufferArray - Pointer to the source array of WSABUF structures.

    BufferCount - The number of entries in BufferArray.

Return Value:

    DWORD - The number of bytes actually copied to the flat buffer.

--*/

{

    DWORD bytesToCopy;
    PCHAR source;

    source = (PCHAR)FlatBuffer;

    while( BufferCount > 0 && FlatBufferLength > 0 ) {

        bytesToCopy = min( BufferArray->len, FlatBufferLength );

        CopyMemory(
            BufferArray->buf,
            source,
            bytesToCopy
            );

        source += bytesToCopy;
        FlatBufferLength -= bytesToCopy;
        BufferArray++;
        BufferCount--;

    }

    return (DWORD)(source - (PCHAR)FlatBuffer);

}   // SockCopyBufferArrayToFlatBuffer

