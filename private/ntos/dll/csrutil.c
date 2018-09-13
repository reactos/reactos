/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllutil.c

Abstract:

    This module contains utility procedures for the Windows Client DLL


Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrdll.h"

NTSTATUS
CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    )

/*++

Routine Description:

    This function sends an API request to the Windows Emulation Subsystem
    Server and waits for a reply.

Arguments:

    m - Pointer to the API request message to send.

    CaptureBuffer - Optional pointer to a capture buffer located in the
        Port Memory section that contains additional data being sent
        to the server.  Since Port Memory is also visible to the server,
        no data needs to be copied, but pointers to locations within the
        capture buffer need to be converted into pointers valid in the
        server's process context, since the server's view of the Port Memory
        is not at the same virtual address as the client's view.

    ApiNumber - Small integer that is the number of the API being called.

    ArgLength - Length, in bytes, of the argument portion located at the
        end of the request message.  Used to calculate the length of the
        request message.

Return Value:

    Status Code from either client or server

--*/

{
    NTSTATUS Status;
    PULONG_PTR PointerOffsets;
    ULONG CountPointers;
    ULONG_PTR Pointer;

    //
    // Initialize the header of the message.
    //

    if ((LONG)ArgLength < 0) {
        ArgLength = (ULONG)(-(LONG)ArgLength);
        m->h.u2.s2.Type = 0;
        }
    else {
        m->h.u2.ZeroInit = 0;
        }

    ArgLength |= (ArgLength << 16);
    ArgLength +=     ((sizeof( CSR_API_MSG ) - sizeof( m->u )) << 16) |
                     (FIELD_OFFSET( CSR_API_MSG, u ) - sizeof( m->h ));
    m->h.u1.Length = ArgLength;
    m->CaptureBuffer = NULL;
    m->ApiNumber = ApiNumber;

    //
    // if the caller is within the server process, do the API call directly
    // and skip the capture buffer fixups and LPC call.
    //

    if (CsrServerProcess == FALSE) {

        //
        // If the CaptureBuffer argument is present, then there is data located
        // in the Port Memory section that is being passed to the server.  All
        // Port Memory pointers need to be converted so they are valid in the
        // Server's view of the Port Memory.
        //

        if (ARGUMENT_PRESENT( CaptureBuffer )) {
            //
            // Store a pointer to the capture buffer in the message that is valid
            // in the server process's context.
            //

            m->CaptureBuffer = (PCSR_CAPTURE_HEADER)
                ((PCHAR)CaptureBuffer + CsrPortMemoryRemoteDelta);

            //
            // Mark the fact that we are done allocating space from the end of
            // the capture buffer.
            //

            CaptureBuffer->FreeSpace = NULL;

            //
            // Loop over all of the pointers to Port Memory within the message
            // itself and convert them into server pointers.  Also, convert
            // the pointers to pointers into offsets.
            //

            PointerOffsets = CaptureBuffer->MessagePointerOffsets;
            CountPointers = CaptureBuffer->CountMessagePointers;
            while (CountPointers--) {
                Pointer = *PointerOffsets++;
                if (Pointer != 0) {
                    *(PULONG_PTR)Pointer += CsrPortMemoryRemoteDelta;
                    PointerOffsets[ -1 ] = Pointer - (ULONG_PTR)m;
                    }
                }
            }

        //
        // Send the request to the server and wait for a reply.  The wait is
        // NOT alertable, because ? FIX,FIX
        //

        Status = NtRequestWaitReplyPort( CsrPortHandle,
                                         (PPORT_MESSAGE)m,
                                         (PPORT_MESSAGE)m
                                       );
        //
        // If the CaptureBuffer argument is present then reverse what we did
        // to the pointers above so that the client side code can use them
        // again.
        //

        if (ARGUMENT_PRESENT( CaptureBuffer )) {
            //
            // Convert the capture buffer pointer back to a client pointer.
            //

            m->CaptureBuffer = (PCSR_CAPTURE_HEADER)
                ((PCHAR)m->CaptureBuffer - CsrPortMemoryRemoteDelta);

            //
            // Loop over all of the pointers to Port Memory within the message
            // itself and convert them into client pointers.  Also, convert
            // the offsets pointers to pointers into back into pointers
            //

            PointerOffsets = CaptureBuffer->MessagePointerOffsets;
            CountPointers = CaptureBuffer->CountMessagePointers;
            while (CountPointers--) {
                Pointer = *PointerOffsets++;
                if (Pointer != 0) {
                    Pointer += (ULONG_PTR)m;
                    PointerOffsets[ -1 ] = Pointer;
                    *(PULONG_PTR)Pointer -= CsrPortMemoryRemoteDelta;
                    }
                }
            }

        //
        // Check for failed status and do something.
        //
        if (!NT_SUCCESS( Status )) {
            IF_DEBUG {
                if (Status != STATUS_PORT_DISCONNECTED &&
                    Status != STATUS_INVALID_HANDLE
                   ) {
                    DbgPrint( "CSRDLL: NtRequestWaitReplyPort failed - Status == %X\n",
                              Status
                            );
                    }
                }

            m->ReturnValue = Status;
            }
        }
    else {
        m->h.ClientId = NtCurrentTeb()->ClientId;
        Status = (CsrServerApiRoutine)((PCSR_API_MSG)m,
                                       (PCSR_API_MSG)m
                                      );

        //
        // Check for failed status and do something.
        //

        if (!NT_SUCCESS( Status )) {
            IF_DEBUG {
                DbgPrint( "CSRDLL: Server side client call failed - Status == %X\n",
                          Status
                        );
                }

            m->ReturnValue = Status;
            }
        }

    //
    // The value of this function is whatever the server function returned.
    //

    return( m->ReturnValue );
}


PCSR_CAPTURE_HEADER
CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG Size
    )

/*++

Routine Description:

    This function allocates a buffer from the Port Memory section for
    use by the client in capture arguments into Port Memory.  In addition to
    specifying the size of the data that needs to be captured, the caller
    needs to specify how many pointers to captured data will be passed.
    Pointers can be located in either the request message itself, and/or
    the capture buffer.

Arguments:

    CountMessagePointers - Number of pointers within the request message
        that will point to locations within the allocated capture buffer.

    Size - Total size of the data that will be captured into the capture
        buffer.

Return Value:

    A pointer to the capture buffer header.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG CountPointers;

    //
    // Calculate the total number of pointers that will be passed
    //

    CountPointers = CountMessagePointers;

    //
    // Calculate the total size of the capture buffer.  This includes the
    // header, the array of pointer offsets and the data length.  We round
    // the data length to a 32-bit boundary, assuming that each pointer
    // points to data whose length is not aligned on a 32-bit boundary.
    //

    if (Size >= MAXLONG) {
        //
        // Bail early if too big
        //
        return NULL;
        }
    Size += FIELD_OFFSET(CSR_CAPTURE_HEADER, MessagePointerOffsets) + (CountPointers * sizeof( PVOID ));
    Size = (Size + (3 * (CountPointers+1))) & ~3;

    //
    // Allocate the capture buffer from the Port Memory Heap.
    //

    CaptureBuffer = RtlAllocateHeap( CsrPortHeap, MAKE_CSRPORT_TAG( CAPTURE_TAG ), Size );
    if (CaptureBuffer == NULL) {

        //
        // FIX, FIX - need to attempt the receive lost reply messages to
        // to see if they contain CaptureBuffer pointers that can be freed.
        //

        return( NULL );
    }

    //
    // Initialize the capture buffer header
    //

    CaptureBuffer->Length = Size;
    CaptureBuffer->CountMessagePointers = 0;

    //
    // If there are pointers being passed then initialize the arrays of
    // pointer offsets to zero.  In either case set the free space pointer
    // in the capture buffer header to point to the first 32-bit aligned
    // location after the header, the arrays of pointer offsets are considered
    // part of the header.
    //

    RtlZeroMemory( CaptureBuffer->MessagePointerOffsets,
                   CountPointers * sizeof( ULONG_PTR )
                 );

    CaptureBuffer->FreeSpace = (PCHAR)
        (CaptureBuffer->MessagePointerOffsets + CountPointers);

    //
    // Returned the address of the capture buffer.
    //

    return( CaptureBuffer );
}


VOID
CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    )

/*++

Routine Description:

    This function frees a capture buffer allocated by CsrAllocateCaptureBuffer.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer.

Return Value:

    None.

--*/

{
    //
    // Free the capture buffer back to the Port Memory heap.
    //

    RtlFreeHeap( CsrPortHeap, 0, CaptureBuffer );
}


ULONG
CsrAllocateMessagePointer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN ULONG Length,
    OUT PVOID *Pointer
    )

/*++

Routine Description:

    This function allocates space from the capture buffer along with a
    pointer to point to it.  The pointer is presumed to be located in
    the request message structure.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer.

    Length - Size of data being allocated from the capture buffer.

    Pointer - Address of the pointer within the request message that
        is to point to the space allocated out of the capture buffer.

Return Value:

    The actual length of the buffer allocated, after it has been rounded
    up to a multiple of 4.

--*/

{
    if (Length == 0) {
        *Pointer = NULL;
        Pointer = NULL;
        }

    else {

        //
        // Set the returned pointer value to point to the next free byte in
        // the capture buffer.
        //

        *Pointer = CaptureBuffer->FreeSpace;

        //
        // Round the length up to a multiple of 4
        //

        if (Length >= MAXLONG) {
            //
            // Bail early if too big
            //
            return 0;
            }

        Length = (Length + 3) & ~3;

        //
        // Update the free space pointer to point to the next available byte
        // in the capture buffer.
        //

        CaptureBuffer->FreeSpace += Length;
        }


    //
    // Remember the location of this pointer so that CsrClientCallServer can
    // convert it into a server pointer prior to sending the request to
    // the server.
    //

    CaptureBuffer->MessagePointerOffsets[ CaptureBuffer->CountMessagePointers++ ] =
        (ULONG_PTR)Pointer;

    //
    // Returned the actual length allocated.
    //

    return( Length );
}


VOID
CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    )

/*++

Routine Description:

    This function captures an ASCII string into a counted string data
    structure located in an API request message.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer.

    Buffer - Optional pointer to the buffer.  If this parameter is
        not present, then the counted string data structure is set to
        the null string and no space is allocated from the capture
        buffer.

    Length - Length of the buffer.

    CaptureString - Pointer to the field in the message that will
        be filled in to point to the capture buffer.

Return Value:

    None.

--*/

{
    //
    // Set the length fields of the captured string structure and allocated
    // the Length for the string from the capture buffer.
    //

    CsrAllocateMessagePointer( CaptureBuffer,
                               Length,
                               CapturedBuffer
                             );

    //
    // If Buffer parameter is not present or the length of the data is zero,
    // return.
    //

    if (!ARGUMENT_PRESENT( Buffer ) || (Length == 0)) {
        return;
        }

    //
    // Copy the buffer data to the capture area.
    //

    RtlMoveMemory( *CapturedBuffer, Buffer, Length );

    return;
}

VOID
CsrCaptureMessageString(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PCSTR String OPTIONAL,
    IN ULONG Length,
    IN ULONG MaximumLength,
    OUT PSTRING CapturedString
    )

/*++

Routine Description:

    This function captures an ASCII string into a counted string data
    structure located in an API request message.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer.

    String - Optional pointer to the ASCII string.  If this parameter is
        not present, then the counted string data structure is set to
        the null string and no space is allocated from the capture
        buffer.

    Length - Length of the ASCII string.

    MaximumLength - Maximum length of the string.  Different for null
        terminated strings, where Length does not include the null and
        MaximumLength does.

    CaptureString - Pointer to the counted string data structure that will
        be filled in to point to the capture ASCII string.

Return Value:

    None.

--*/

{
    //
    // If String parameter is not present, then set the captured string
    // to be the null string and returned.
    //

    if (!ARGUMENT_PRESENT( String )) {
        CapturedString->Length = 0;
        CapturedString->MaximumLength = (USHORT)MaximumLength;
        CsrAllocateMessagePointer( CaptureBuffer,
                                   MaximumLength,
                                   (PVOID *)&CapturedString->Buffer
                                 );
        return;
        }

    //
    // Set the length fields of the captured string structure and allocated
    // the MaximumLength for the string from the capture buffer.
    //

    CapturedString->Length = (USHORT)Length;
    CapturedString->MaximumLength = (USHORT)
        CsrAllocateMessagePointer( CaptureBuffer,
                                   MaximumLength,
                                   (PVOID *)&CapturedString->Buffer
                                 );
    //
    // If the Length of the ASCII string is non-zero then move it to the
    // capture area.
    //

    if (Length != 0) {
        RtlMoveMemory( CapturedString->Buffer, String, MaximumLength );
        if (CapturedString->Length < CapturedString->MaximumLength) {
            CapturedString->Buffer[ CapturedString->Length ] = '\0';
            }
        }

    return;
}



PLARGE_INTEGER
CsrCaptureTimeout(
    IN ULONG MilliSeconds,
    OUT PLARGE_INTEGER Timeout
    )
{
    if (MilliSeconds == -1) {
        return( NULL );
        }
    else {
        Timeout->QuadPart = Int32x32To64( MilliSeconds, -10000 );
        return( (PLARGE_INTEGER)Timeout );
        }
}

VOID
CsrProbeForWrite(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility.
    If the structure is not accessible, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{
    volatile CHAR *StartAddress;
    volatile CHAR *EndAddress;
    CHAR Temp;

    //
    // If the structure has zero length, then do not probe the structure for
    // write accessibility or alignment.
    //

    if (Length != 0) {

        //
        // If the structure is not properly aligned, then raise a data
        // misalignment exception.
        //

        ASSERT((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8));
        StartAddress = (volatile CHAR *)Address;

        if (((ULONG_PTR)StartAddress & (Alignment - 1)) != 0) {
            RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
        } else {
            //
            // BUG, BUG - this should not be necessary once the 386 kernel
            // makes system space inaccessable to user mode.
            //
            if ((ULONG_PTR)StartAddress > CsrNtSysInfo.MaximumUserModeAddress) {
                RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
            }

            Temp = *StartAddress;
            *StartAddress = Temp;
            EndAddress = StartAddress + Length - 1;
            Temp = *EndAddress;
            *EndAddress = Temp;
        }
    }
}

VOID
CsrProbeForRead(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility.
    If the structure is not accessible, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{
    volatile CHAR *StartAddress;
    volatile CHAR *EndAddress;
    CHAR Temp;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility or alignment.
    //

    if (Length != 0) {

        //
        // If the structure is not properly aligned, then raise a data
        // misalignment exception.
        //

        ASSERT((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8));
        StartAddress = (volatile CHAR *)Address;

        if (((ULONG_PTR)StartAddress & (Alignment - 1)) != 0) {
            RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
        } else {
            Temp = *StartAddress;
            EndAddress = StartAddress + Length - 1;
            Temp = *EndAddress;
        }
    }
}
