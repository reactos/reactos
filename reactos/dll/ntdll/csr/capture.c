/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/capture.c
 * PURPOSE:         routines for probing and capturing CSR API Messages
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
extern HANDLE CsrPortHeap;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForRead(IN PVOID Address,
                IN ULONG Length,
                IN ULONG Alignment)
{
    PUCHAR Pointer;
    UCHAR Data;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Do the probe */
    Pointer = (PUCHAR)Address;
    Data = *Pointer;
    Pointer = (PUCHAR)((ULONG)Address + Length -1);
    Data = *Pointer;
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
    PUCHAR Pointer;
    UCHAR Data;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Do the probe */
    Pointer = (PUCHAR)Address;
    Data = *Pointer;
    *Pointer = Data;
    Pointer = (PUCHAR)((ULONG)Address + Length -1);
    Data = *Pointer;
    *Pointer = Data;
}

/*
 * @implemented
 */
PVOID
NTAPI
CsrAllocateCaptureBuffer(ULONG ArgumentCount,
                         ULONG BufferSize)
{
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    /* Validate size */
    if (BufferSize >= MAXLONG) return NULL;

    /* Add the size of the header and for each pointer to the pointers */
    BufferSize += sizeof(CSR_CAPTURE_BUFFER) + (ArgumentCount * sizeof(PVOID));

    /* Allocate memory from the port heap */
    CaptureBuffer = RtlAllocateHeap(CsrPortHeap, 0, BufferSize);
    if (CaptureBuffer == NULL) return NULL;

    /* Initialize the header */
    CaptureBuffer->Size = BufferSize;
    CaptureBuffer->PointerCount = 0;

    /* Initialize all the pointers */
    RtlZeroMemory(CaptureBuffer->PointerArray,
                  ArgumentCount * sizeof(ULONG_PTR));

    /* Point the start of the free buffer */
    CaptureBuffer->BufferEnd = (ULONG_PTR)CaptureBuffer->PointerArray +
                               ArgumentCount * sizeof(ULONG_PTR);

    /* Return the address of the buffer */
    return CaptureBuffer;
}

/*
 * @implemented
 */
ULONG
NTAPI
CsrAllocateMessagePointer(PCSR_CAPTURE_BUFFER CaptureBuffer,
                          ULONG MessageLength,
                          PVOID *CaptureData)
{
    /* If there's no data, our job is easy. */
    if (MessageLength == 0)
    {
        *CaptureData = NULL;
        CaptureData = NULL;
    }
    else
    {
        /* Set the capture data at our current available buffer */
        *CaptureData = (PVOID)CaptureBuffer->BufferEnd;

        /* Validate the size */
        if (MessageLength >= MAXLONG) return 0;

        /* Align it to a 4-byte boundary */
        MessageLength = (MessageLength + 3) & ~3;

        /* Move our available buffer beyond this space */
        CaptureBuffer->BufferEnd += MessageLength;
    }

    /* Write down this pointer in the array */
    CaptureBuffer->PointerArray[CaptureBuffer->PointerCount] = (ULONG_PTR)CaptureData;

    /* Increase the pointer count */
    CaptureBuffer->PointerCount++;

    /* Return the aligned length */
    return MessageLength;
}

/*
 * @implemented
 */
VOID
NTAPI
CsrCaptureMessageBuffer(PCSR_CAPTURE_BUFFER CaptureBuffer,
                        PVOID MessageString,
                        ULONG StringLength,
                        PVOID *CapturedData)
{
    /* Simply allocate a message pointer in the buffer */
    CsrAllocateMessagePointer(CaptureBuffer, StringLength, CapturedData);

    /* Check if there was any data */
    if (!MessageString || !StringLength) return;

    /* Copy the data into the buffer */
    RtlMoveMemory(*CapturedData, MessageString, StringLength);
}

/*
 * @implemented
 */
VOID
NTAPI
CsrFreeCaptureBuffer(PCSR_CAPTURE_BUFFER CaptureBuffer)
{
    /* Free it from the heap */
    RtlFreeHeap(CsrPortHeap, 0, CaptureBuffer);
}

/*
 * @implemented
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
CsrCaptureMessageString(PCSR_CAPTURE_BUFFER CaptureBuffer,
                        LPSTR String,
                        IN ULONG StringLength,
                        IN ULONG MaximumLength,
                        OUT PANSI_STRING CapturedString)
{
    ULONG ReturnedLength;

    /* If we don't have a string, initialize an empty one */
    if (!String)
    {
        CapturedString->Length = 0;
        CapturedString->MaximumLength = MaximumLength;

        /* Allocate a pointer for it */
        CsrAllocateMessagePointer(CaptureBuffer,
                                  MaximumLength,
                                  (PVOID*)&CapturedString->Buffer);
        return;
    }

    /* Initialize this string */
    CapturedString->Length = StringLength;
    
    /* Allocate a buffer and get its size */
    ReturnedLength = CsrAllocateMessagePointer(CaptureBuffer,
                                               MaximumLength,
                                               (PVOID*)&CapturedString->Buffer);
    CapturedString->MaximumLength = ReturnedLength;

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
CsrCaptureTimeout(LONG Milliseconds,
                  PLARGE_INTEGER Timeout)
{
    /* Validate the time */
    if (Milliseconds == -1) return NULL;

    /* Convert to relative ticks */
    Timeout->QuadPart = Milliseconds * -100000;
    return Timeout;
}

/* EOF */
