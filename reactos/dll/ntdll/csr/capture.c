/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/csr/capture.c
 * PURPOSE:         Routines for probing and capturing CSR API Messages
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern HANDLE CsrPortHeap;

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForRead(IN PVOID Address,
                IN ULONG Length,
                IN ULONG Alignment)
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
CsrProbeForWrite(IN PVOID Address,
                 IN ULONG Length,
                 IN ULONG Alignment)
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
CsrAllocateCaptureBuffer(IN ULONG ArgumentCount,
                         IN ULONG BufferSize)
{
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    /* Validate size */
    if (BufferSize >= MAXLONG) return NULL;

    /* Add the size of the header and for each offset to the pointers */
    BufferSize += FIELD_OFFSET(CSR_CAPTURE_BUFFER, PointerOffsetsArray) +
                    (ArgumentCount * sizeof(ULONG_PTR));

    /* Align it to a 4-byte boundary */
    BufferSize = (BufferSize + 3) & ~3;

    /* Add the size of the alignment padding for each argument */
    BufferSize += ArgumentCount * 3;

    /* Allocate memory from the port heap */
    CaptureBuffer = RtlAllocateHeap(CsrPortHeap, HEAP_ZERO_MEMORY, BufferSize);
    if (CaptureBuffer == NULL) return NULL;

    /* Initialize the header */
    CaptureBuffer->Size = BufferSize;
    CaptureBuffer->PointerCount = 0;

    /* Initialize all the offsets */
    RtlZeroMemory(CaptureBuffer->PointerOffsetsArray,
                  ArgumentCount * sizeof(ULONG_PTR));

    /* Point to the start of the free buffer */
    CaptureBuffer->BufferEnd = (PVOID)((ULONG_PTR)CaptureBuffer->PointerOffsetsArray +
                                       ArgumentCount * sizeof(ULONG_PTR));

    /* Return the address of the buffer */
    return CaptureBuffer;
}

/*
 * @implemented
 */
ULONG
NTAPI
CsrAllocateMessagePointer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                          IN ULONG MessageLength,
                          OUT PVOID* CapturedData)
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
CsrCaptureMessageBuffer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN PVOID MessageBuffer OPTIONAL,
                        IN ULONG MessageLength,
                        OUT PVOID* CapturedData)
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
CsrFreeCaptureBuffer(IN PCSR_CAPTURE_BUFFER CaptureBuffer)
{
    /* Free it from the heap */
    RtlFreeHeap(CsrPortHeap, 0, CaptureBuffer);
}

/*
 * @implemented
 */
VOID
NTAPI
CsrCaptureMessageString(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN PCSTR String OPTIONAL,
                        IN ULONG StringLength,
                        IN ULONG MaximumLength,
                        OUT PSTRING CapturedString)
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
        CapturedString->Buffer[CapturedString->Length] = '\0';
}

static VOID
CsrCaptureMessageUnicodeStringInPlace(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                                      IN PUNICODE_STRING String)
{
    ASSERT(String != NULL);

    /* This is a way to capture the UNICODE string, since (Maximum)Length are also in bytes */
    CsrCaptureMessageString(CaptureBuffer,
                            (PCSTR)String->Buffer,
                            String->Length,
                            String->MaximumLength,
                            (PSTRING)String);

    /* Null-terminate the string */
    if (String->MaximumLength >= String->Length + sizeof(WCHAR))
    {
        String->Buffer[String->Length / sizeof(WCHAR)] = L'\0';
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrCaptureMessageMultiUnicodeStringsInPlace(OUT PCSR_CAPTURE_BUFFER* CaptureBuffer,
                                            IN ULONG StringsCount,
                                            IN PUNICODE_STRING* MessageStrings)
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
CsrCaptureTimeout(IN ULONG Milliseconds,
                  OUT PLARGE_INTEGER Timeout)
{
    /* Validate the time */
    if (Milliseconds == -1) return NULL;

    /* Convert to relative ticks */
    Timeout->QuadPart = Milliseconds * -10000LL;
    return Timeout;
}

/* EOF */
