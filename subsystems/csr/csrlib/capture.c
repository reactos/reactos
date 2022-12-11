/*
 * PROJECT:     ReactOS Client/Server Runtime SubSystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CSR Client Library - CSR API Messages probing and capturing
 * COPYRIGHT:   Copyright 2005 Alex Ionescu <alex@relsoft.net>
 *              Copyright 2012-2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "csrlib.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForRead(
    _In_ PVOID Address,
    _In_ ULONG Length,
    _In_ ULONG Alignment)
{
    volatile UCHAR *Pointer;
    UCHAR Data;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Probe first byte */
    Pointer = Address;
    Data = *Pointer;

    /* Probe last byte */
    Pointer = (PUCHAR)Address + Length - 1;
    Data = *Pointer;
    (void)Data;
}

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForWrite(
    _In_ PVOID Address,
    _In_ ULONG Length,
    _In_ ULONG Alignment)
{
    volatile UCHAR *Pointer;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Probe first byte */
    Pointer = Address;
    *Pointer = *Pointer;

    /* Probe last byte */
    Pointer = (PUCHAR)Address + Length - 1;
    *Pointer = *Pointer;
}

/*
 * @implemented
 */
PCSR_CAPTURE_BUFFER
NTAPI
CsrAllocateCaptureBuffer(
    _In_ ULONG ArgumentCount,
    _In_ ULONG BufferSize)
{
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG OffsetsArraySize;
    ULONG MaximumSize;

    /* Validate the argument count. Note that on server side, CSRSRV
     * limits the count to MAXUSHORT; here we are a bit more lenient. */
    if (ArgumentCount > (MAXLONG / sizeof(ULONG_PTR)))
        return NULL;

    OffsetsArraySize = ArgumentCount * sizeof(ULONG_PTR);

    /*
     * Validate the total buffer size.
     * The total size of the header plus the pointer-offset array and the
     * provided buffer, together with the alignment padding for each argument,
     * must be less than MAXLONG aligned to 4-byte boundary.
     */
    MaximumSize = (MAXLONG & ~3) - FIELD_OFFSET(CSR_CAPTURE_BUFFER, PointerOffsetsArray);
    if (OffsetsArraySize >= MaximumSize)
        return NULL;
    MaximumSize -= OffsetsArraySize;
    if (BufferSize >= MaximumSize)
        return NULL;
    MaximumSize -= BufferSize;
    if ((ArgumentCount * 3) + 3 >= MaximumSize)
        return NULL;

    /* Add the size of the header and of the pointer-offset array */
    BufferSize += FIELD_OFFSET(CSR_CAPTURE_BUFFER, PointerOffsetsArray) +
                    OffsetsArraySize;

    /* Add the size of the alignment padding for each argument */
    BufferSize += ArgumentCount * 3;

    /* Align it to a 4-byte boundary */
    BufferSize = (BufferSize + 3) & ~3;

    /* Allocate memory from the port heap */
    CaptureBuffer = RtlAllocateHeap(CsrPortHeap, HEAP_ZERO_MEMORY, BufferSize);
    if (CaptureBuffer == NULL) return NULL;

    /* Initialize the header */
    CaptureBuffer->Size = BufferSize;
    CaptureBuffer->PointerCount = 0;

    /* Initialize the pointer-offset array */
    RtlZeroMemory(CaptureBuffer->PointerOffsetsArray, OffsetsArraySize);

    /* Point to the start of the free buffer */
    CaptureBuffer->BufferEnd = (PVOID)((ULONG_PTR)CaptureBuffer->PointerOffsetsArray +
                                       OffsetsArraySize);

    /* Return the address of the buffer */
    return CaptureBuffer;
}

/*
 * @implemented
 */
ULONG
NTAPI
CsrAllocateMessagePointer(
    _Inout_ PCSR_CAPTURE_BUFFER CaptureBuffer,
    _In_ ULONG MessageLength,
    _Out_ PVOID* CapturedData)
{
    if (MessageLength == 0)
    {
        *CapturedData = NULL;
        CapturedData = NULL;
    }
    else
    {
        /* Set the capture data at our current available buffer */
        *CapturedData = CaptureBuffer->BufferEnd;

        /* Validate the size */
        if (MessageLength >= MAXLONG) return 0;

        /* Align it to a 4-byte boundary */
        MessageLength = (MessageLength + 3) & ~3;

        /* Move our available buffer beyond this space */
        CaptureBuffer->BufferEnd = (PVOID)((ULONG_PTR)CaptureBuffer->BufferEnd + MessageLength);
    }

    /* Write down this pointer in the array and increase the count */
    CaptureBuffer->PointerOffsetsArray[CaptureBuffer->PointerCount++] = (ULONG_PTR)CapturedData;

    /* Return the aligned length */
    return MessageLength;
}

/*
 * @implemented
 */
VOID
NTAPI
CsrCaptureMessageBuffer(
    _Inout_ PCSR_CAPTURE_BUFFER CaptureBuffer,
    _In_opt_ PVOID MessageBuffer,
    _In_ ULONG MessageLength,
    _Out_ PVOID* CapturedData)
{
    /* Simply allocate a message pointer in the buffer */
    CsrAllocateMessagePointer(CaptureBuffer, MessageLength, CapturedData);

    /* Check if there was any data */
    if (!MessageBuffer || !MessageLength) return;

    /* Copy the data into the buffer */
    RtlMoveMemory(*CapturedData, MessageBuffer, MessageLength);
}

/*
 * @implemented
 */
VOID
NTAPI
CsrFreeCaptureBuffer(
    _In_ _Frees_ptr_ PCSR_CAPTURE_BUFFER CaptureBuffer)
{
    /* Free it from the heap */
    RtlFreeHeap(CsrPortHeap, 0, CaptureBuffer);
}

/*
 * @implemented
 */
VOID
NTAPI
CsrCaptureMessageString(
    _Inout_ PCSR_CAPTURE_BUFFER CaptureBuffer,
    _In_opt_ PCSTR String,
    _In_ ULONG StringLength,
    _In_ ULONG MaximumLength,
    _Out_ PSTRING CapturedString)
{
    ASSERT(CapturedString != NULL);

    /*
     * If we don't have a string, initialize an empty one,
     * otherwise capture the given string.
     */
    if (!String)
    {
        CapturedString->Length = 0;
        CapturedString->MaximumLength = (USHORT)MaximumLength;

        /* Allocate a pointer for it */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  MaximumLength,
                                  (PVOID*)&CapturedString->Buffer);
    }
    else
    {
        /* Cut-off the string length if needed */
        if (StringLength > MaximumLength)
            StringLength = MaximumLength;

        CapturedString->Length = (USHORT)StringLength;

        /* Allocate a buffer and get its size */
        CapturedString->MaximumLength =
            (USHORT)CsrAllocateMessagePointer(CaptureBuffer,
                                              MaximumLength,
                                              (PVOID*)&CapturedString->Buffer);

        /* If the string has data, copy it into the buffer */
        if (StringLength)
            RtlMoveMemory(CapturedString->Buffer, String, StringLength);
    }

    /* Null-terminate the string if we don't take up the whole space */
    if (CapturedString->Length < CapturedString->MaximumLength)
        CapturedString->Buffer[CapturedString->Length] = ANSI_NULL;
}

VOID
NTAPI
CsrCaptureMessageUnicodeStringInPlace(
    _Inout_ PCSR_CAPTURE_BUFFER CaptureBuffer,
    _Inout_ PUNICODE_STRING String)
{
    ASSERT(String != NULL);

    /* This is a way to capture the UNICODE string, since (Maximum)Length are also in bytes */
    CsrCaptureMessageString(CaptureBuffer,
                            (PCSTR)String->Buffer,
                            String->Length,
                            String->MaximumLength,
                            (PSTRING)String);

    /* Null-terminate the string if we don't take up the whole space */
    if (String->Length + sizeof(WCHAR) <= String->MaximumLength)
        String->Buffer[String->Length / sizeof(WCHAR)] = UNICODE_NULL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrCaptureMessageMultiUnicodeStringsInPlace(
    _Inout_ PCSR_CAPTURE_BUFFER* CaptureBuffer,
    _In_ ULONG StringsCount,
    _In_ PUNICODE_STRING* MessageStrings)
{
    ULONG Count;

    if (!CaptureBuffer) return STATUS_INVALID_PARAMETER;

    /* Allocate a new capture buffer if we don't have one already */
    if (!*CaptureBuffer)
    {
        /* Compute the required size for the capture buffer */
        ULONG Size = 0;

        Count = 0;
        while (Count < StringsCount)
        {
            if (MessageStrings[Count])
                Size += MessageStrings[Count]->MaximumLength;

            ++Count;
        }

        /* Allocate the capture buffer */
        *CaptureBuffer = CsrAllocateCaptureBuffer(StringsCount, Size);
        if (!*CaptureBuffer) return STATUS_NO_MEMORY;
    }

    /* Now capture each UNICODE string */
    Count = 0;
    while (Count < StringsCount)
    {
        if (MessageStrings[Count])
            CsrCaptureMessageUnicodeStringInPlace(*CaptureBuffer, MessageStrings[Count]);

        ++Count;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PLARGE_INTEGER
NTAPI
CsrCaptureTimeout(
    _In_ ULONG Milliseconds,
    _Out_ PLARGE_INTEGER Timeout)
{
    /* Validate the time */
    if (Milliseconds == -1) return NULL;

    /* Convert to relative ticks */
    Timeout->QuadPart = Milliseconds * -10000LL;
    return Timeout;
}

/* EOF */
