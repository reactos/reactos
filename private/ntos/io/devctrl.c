/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    devctrl.c

Abstract:

    This module contains the code to implement the NtDeviceIoControlFile system
    service for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 16-Oct-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtDeviceIoControlFile)
#endif

NTSTATUS
NtDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This service builds descriptors or MDLs for the supplied buffer(s) and
    passes the untyped data to the device driver associated with the file
    handle.  It is up to the driver to check the input data and function
    IoControlCode for validity, as well as to make the appropriate access
    checks.

Arguments:

    FileHandle - Supplies a handle to the file on which the service is being
        performed.

    Event - Supplies an optional event to be set to the Signaled state when
        the service is complete.

    ApcRoutine - Supplies an optional APC routine to be executed when the
        service is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine,
        if an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    IoControlCode - Subfunction code to determine exactly what operation is
        being performed.

    InputBuffer - Optionally supplies an input buffer to be passed to the
        device driver.  Whether or not the buffer is actually optional is
        dependent on the IoControlCode.

    InputBufferLength - Length of the InputBuffer in bytes.

    OutputBuffer - Optionally supplies an output buffer to receive information
        from the device driver.  Whether or not the buffer is actually optional
        is dependent on the IoControlCode.

    OutputBufferLength - Length of the OutputBuffer in bytes.

Return Value:

    The status returned is success if the control operation was properly
    queued to the I/O system.   Once the operation completes, the status
    can be determined by examining the Status field of the I/O status block.

--*/

{
    //
    // Simply invoke the common routine that implements both device and file
    // system I/O controls.
    //

    return IopXxxControlFile( FileHandle,
                              Event,
                              ApcRoutine,
                              ApcContext,
                              IoStatusBlock,
                              IoControlCode,
                              InputBuffer,
                              InputBufferLength,
                              OutputBuffer,
                              OutputBufferLength,
                              TRUE );
}
