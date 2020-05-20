/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Memory object implementation
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/ifxmemory.h"
#include "common/dbgtrace.h"
#include "common/fxverifier.h"


_Must_inspect_result_
NTSTATUS
IFxMemory::CopyToPtr(
    __in_opt PWDFMEMORY_OFFSET SourceOffsets,
    __out_bcount(DestinationBufferLength)PVOID DestinationBuffer,
    __in size_t DestinationBufferLength,
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets
    )
/*++

Routine Description:

    Worker routine for the various copy APIs.  Verifies that a copy will not
    overrun a buffer, or write into a read only memory buffer

Arguments:

    SourceOffsets - Offsets into SourceBuffer from which the copy starts.  If
        NULL, an offset of 0 is used.

    DestinationBuffer - Memory whose contents we are copying into

    DestinationBufferLength - Size of DestinationBuffer in bytes

    DestinationOffsets - Offsets into DestinationMemory from which the copy
        starts and indicates how many bytes to copy.  If length is 0 or
        parameter is NULL, the entire length of DestinationMemory is used.

Return Value:
    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller then the requested number
        of bytes to be copied.
    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied
    NTSTATUS

  --*/
{
    //
    // We are reading from the FxMemoryBuffer, so no need to check for ReadOnly
    //
    return _CopyPtrToPtr(
        GetBuffer(),
        GetBufferSize(),
        SourceOffsets,
        DestinationBuffer,
        DestinationBufferLength,
        DestinationOffsets
        );
}

_Must_inspect_result_
NTSTATUS
IFxMemory::_CopyPtrToPtr(
    __in_bcount(SourceBufferLength)PVOID SourceBuffer,
    __in size_t SourceBufferLength,
    __in_opt PWDFMEMORY_OFFSET SourceOffsets,
    __out_bcount(DestinationBufferLength) PVOID DestinationBuffer,
    __in size_t DestinationBufferLength,
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets
    )
/*++

Routine Description:
    Worker routine for the various copy APIs.  Verifies that a copy will not
    overrun a buffer.

Arguments:
    SourceBuffer - Memory whose contents we are copying from.

    SourceBufferLength - Size of SourceBuffer in bytes

    SourceOffsets - Offsets into SourceBuffer from which the copy starts.  If
        NULL, an offset of 0 is used.

    DestinationBuffer - Memory whose contents we are copying into

    DestinationBufferLength - Size of DestinationBuffer in bytes

    DestinationOffsets - Offsets into DestinationMemory from which the copy
        starts and indicates how many bytes to copy.  If length is 0 or
        parameter is NULL, the entire length of DestinationMemory is used.

Return Value:
    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller then the requested number
        of bytes to be copied.
    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied
    NTSTATUS

  --*/
{
    size_t srcSize, copyLength;
    PUCHAR pSrcBuf, pDstBuf;

    if (SourceBuffer == NULL)
    {
       return STATUS_INVALID_PARAMETER;
    }

    pSrcBuf = (PUCHAR) SourceBuffer;
    srcSize = SourceBufferLength;

    pDstBuf = (PUCHAR) DestinationBuffer;
    copyLength = DestinationBufferLength;

    if (SourceOffsets != NULL)
    {
        if (SourceOffsets->BufferOffset != 0)
        {
            if (SourceOffsets->BufferOffset >= srcSize)
            {
                //
                // Offset is beyond end of buffer
                //
                return STATUS_BUFFER_TOO_SMALL;
            }

            //
            // Adjust the start and source size to reflect the offset info the
            // source
            //
            pSrcBuf += SourceOffsets->BufferOffset;
            srcSize -= SourceOffsets->BufferOffset;
        }
    }

    if (DestinationOffsets != NULL)
    {
        if (DestinationOffsets->BufferOffset != 0)
        {
            if (DestinationOffsets->BufferOffset >= copyLength)
            {
                //
                // Offset is beyond end of buffer
                //
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // Adjust the start and copy length to reflect the offset info the
            // destination
            //
            pDstBuf += DestinationOffsets->BufferOffset;
            copyLength -= DestinationOffsets->BufferOffset;
        }

        //
        // Non zero buffer length overrides previously calculated copy length
        //
        if (DestinationOffsets->BufferLength != 0)
        {
            //
            // Is the desired buffer length greater than the amount of buffer
            // available?
            //
            if (DestinationOffsets->BufferLength > copyLength)
            {
                return STATUS_INVALID_BUFFER_SIZE;
            }

            copyLength = DestinationOffsets->BufferLength;
        }
    }

    //
    // Compare the final computed copy length against the length of the source
    // buffer.
    //
    if (copyLength > srcSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(pDstBuf, pSrcBuf, copyLength);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
IFxMemory::CopyFromPtr(
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets,
    __in_bcount(SourceBufferLength) PVOID SourceBuffer,
    __in size_t  SourceBufferLength,
    __in_opt PWDFMEMORY_OFFSET SourceOffsets
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // We read from the supplied buffer writing to the current FxMemoryBuffer
    //
    if (GetFlags() & IFxMemoryFlagReadOnly)
    {
        //
        // FxMemoryBuffer is not writeable
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Target WDFMEMORY 0x%p is ReadOnly", GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return STATUS_ACCESS_VIOLATION;
    }

    return _CopyPtrToPtr(
        SourceBuffer,
        SourceBufferLength,
        SourceOffsets,
        GetBuffer(),
        GetBufferSize(),
        DestinationOffsets
        );
}
