/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            dll/ntdll/csr/capture.c
 * PURPOSE:         Routines for probing and capturing CSR API Messages
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
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
                          OUT PVOID *CapturedData)
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
                        OUT PVOID *CapturedData)
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
 * @unimplemented
 */
NTSTATUS
NTAPI
CsrCaptureMessageMultiUnicodeStringsInPlace(IN PCSR_CAPTURE_BUFFER *CaptureBuffer,
                                            IN ULONG MessageCount,
                                            IN PVOID MessageStrings)
{
    /* FIXME: allocate a buffer if we don't have one, and return it */
    /* FIXME: call CsrCaptureMessageUnicodeStringInPlace for each string */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
CsrCaptureMessageString(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN LPSTR String OPTIONAL,
                        IN ULONG StringLength,
                        IN ULONG MaximumLength,
                        OUT PANSI_STRING CapturedString)
{
    ULONG ReturnedLength;

    /* If we don't have a string, initialize an empty one */
    if (!String)
    {
        CapturedString->Length = 0;
        CapturedString->MaximumLength = (USHORT)MaximumLength;

        /* Allocate a pointer for it */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  MaximumLength,
                                  (PVOID*)&CapturedString->Buffer);
        return;
    }

    /* Initialize this string */
    CapturedString->Length = (USHORT)StringLength;

    /* Allocate a buffer and get its size */
    ReturnedLength = CsrAllocateMessagePointer(CaptureBuffer,
                                               MaximumLength,
                                               (PVOID*)&CapturedString->Buffer);
    CapturedString->MaximumLength = (USHORT)ReturnedLength;

    /* If the string had data */
    if (StringLength)
    {
        /* Copy it into the capture buffer */
        RtlMoveMemory(CapturedString->Buffer, String, MaximumLength);

        /* If we don't take up the whole space */
        if (CapturedString->Length < CapturedString->MaximumLength)
        {
            /* Null-terminate it */
            CapturedString->Buffer[CapturedString->Length] = '\0';
        }
    }
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
    Timeout->QuadPart = Int32x32To64(Milliseconds, -10000);
    return Timeout;
}

/* EOF */
