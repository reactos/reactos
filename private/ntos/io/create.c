/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    create.c

Abstract

    This module contains the code to implement the NtCreateFile,
    the NtCreateNamedPipeFile and the NtCreateMailslotFile system
    services.

Author:

    Darryl E. Havens (darrylh) 14-Apr-1989

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtCreateFile)
#pragma alloc_text(PAGE, NtCreateNamedPipeFile)
#pragma alloc_text(PAGE, NtCreateMailslotFile)
#endif

NTSTATUS
NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )

/*++

Routine Description:

    This service opens or creates a file, or opens a device.  It is used to
    establish a file handle to the open device/file that can then be used
    in subsequent operations to perform I/O operations on.  For purposes of
    readability, files and devices are treated as "files" throughout the
    majority of this module and the system service portion of the I/O system.
    The only time a distinction is made is when it is important to determine
    which is really being accessed.  Then a distinction is also made in the
    comments.

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open file.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    AllocationSize - Initial size that should be allocated to the file.  This
        parameter only has an affect if the file is created.  Further, if
        not specified, then it is taken to mean zero.

    FileAttributes - Specifies the attributes that should be set on the file,
        if it is created.

    ShareAccess - Supplies the types of share access that the caller would like
        to the file.

    CreateDisposition - Supplies the method for handling the create/open.

    CreateOptions - Caller options for how to perform the create/open.

    EaBuffer - Optionally specifies a set of EAs to be applied to the file if
        it is created.

    EaLength - Supplies the length of the EaBuffer.

Return Value:

    The function value is the final status of the create/open operation.

--*/

{
    //
    // Simply invoke the common I/O file creation routine to do the work.
    //

    PAGED_CODE();

    return IoCreateFile( FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         AllocationSize,
                         FileAttributes,
                         ShareAccess,
                         CreateDisposition,
                         CreateOptions,
                         EaBuffer,
                         EaLength,
                         CreateFileTypeNone,
                         (PVOID)NULL,
                         0 );
}

NTSTATUS
NtCreateNamedPipeFile(
     OUT PHANDLE FileHandle,
     IN ULONG DesiredAccess,
     IN POBJECT_ATTRIBUTES ObjectAttributes,
     OUT PIO_STATUS_BLOCK IoStatusBlock,
     IN ULONG ShareAccess,
     IN ULONG CreateDisposition,
     IN ULONG CreateOptions,
     IN ULONG NamedPipeType,
     IN ULONG ReadMode,
     IN ULONG CompletionMode,
     IN ULONG MaximumInstances,
     IN ULONG InboundQuota,
     IN ULONG OutboundQuota,
     IN PLARGE_INTEGER DefaultTimeout OPTIONAL
     )

/*++

Routine Description:

    Creates and opens the server end handle of the first instance of a
    specific named pipe or another instance of an existing named pipe.

Arguments:

    FileHandle - Supplies a handle to the file on which the service is being
        performed.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object
        (name, SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Address of the caller's I/O status block.

    ShareAccess - Supplies the types of share access that the caller would
        like to the file.

    CreateDisposition - Supplies the method for handling the create/open.

    CreateOptions - Caller options for how to perform the create/open.

    NamedPipeType - Type of named pipe to create (Bitstream or message).

    ReadMode - Mode in which to read the pipe (Bitstream or message).

    CompletionMode - Specifies how the operation is to be completed.

    MaximumInstances - Maximum number of simultaneous instances of the named
        pipe.

    InboundQuota - Specifies the pool quota that is reserved for writes to the
        inbound side of the named pipe.

    OutboundQuota - Specifies the pool quota that is reserved for writes to
        the inbound side of the named pipe.

    DefaultTimeout - Optional pointer to a timeout value that is used if a
        timeout value is not specified when waiting for an instance of a named
        pipe.

Return Value:

    The function value is the final status of the create/open operation.

--*/

{
    NAMED_PIPE_CREATE_PARAMETERS namedPipeCreateParameters;

    PAGED_CODE();

    //
    // Check whether or not the DefaultTimeout parameter was specified.  If
    // so, then capture it in the named pipe create parameter structure.
    //

    if (ARGUMENT_PRESENT( DefaultTimeout )) {

        //
        // Indicate that a default timeout period was specified.
        //

        namedPipeCreateParameters.TimeoutSpecified = TRUE;

        //
        // A default timeout parameter was specified.  Check to see whether
        // the caller's mode is kernel and if not capture the parameter inside
        // of a try...except clause.
        //

        if (KeGetPreviousMode() != KernelMode) {
            try {
                ProbeForRead( DefaultTimeout,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );
                namedPipeCreateParameters.DefaultTimeout = *DefaultTimeout;
            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // Something went awry attempting to access the parameter.
                // Get the reason for the error and return it as the status
                // value from this service.
                //

                return GetExceptionCode();
            }
        } else {

            //
            // The caller's mode was kernel so simply store the parameter.
            //

            namedPipeCreateParameters.DefaultTimeout = *DefaultTimeout;
        }
    } else {

        //
        // Indicate that no default timeout period was specified.
        //

        namedPipeCreateParameters.TimeoutSpecified = FALSE;
    }

    //
    // Store the remainder of the named pipe-specific parameters in the
    // structure for use in the call to the common create file routine.
    //

    namedPipeCreateParameters.NamedPipeType = NamedPipeType;
    namedPipeCreateParameters.ReadMode = ReadMode;
    namedPipeCreateParameters.CompletionMode = CompletionMode;
    namedPipeCreateParameters.MaximumInstances = MaximumInstances;
    namedPipeCreateParameters.InboundQuota = InboundQuota;
    namedPipeCreateParameters.OutboundQuota = OutboundQuota;

    //
    // Simply perform the remainder of the service by allowing the common
    // file creation code to do the work.
    //

    return IoCreateFile( FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         (PLARGE_INTEGER) NULL,
                         0L,
                         ShareAccess,
                         CreateDisposition,
                         CreateOptions,
                         (PVOID) NULL,
                         0L,
                         CreateFileTypeNamedPipe,
                         &namedPipeCreateParameters,
                         0 );
}

NTSTATUS
NtCreateMailslotFile(
     OUT PHANDLE FileHandle,
     IN ULONG DesiredAccess,
     IN POBJECT_ATTRIBUTES ObjectAttributes,
     OUT PIO_STATUS_BLOCK IoStatusBlock,
     ULONG CreateOptions,
     IN ULONG MailslotQuota,
     IN ULONG MaximumMessageSize,
     IN PLARGE_INTEGER ReadTimeout
     )

/*++

Routine Description:

    Creates and opens the server end handle of a mailslot file.

Arguments:

    FileHandle - Supplies a handle to the file on which the service is being
        performed.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object
        (name, SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Address of the caller's I/O status block.

    CreateOptions - Caller options for how to perform the create/open.

    MailslotQuota - Specifies the pool quota that is reserved for writes
        to this mailslot.

    MaximumMessageSize - Specifies the size of the largest message that
        can be written to this mailslot.

    ReadTimeout - The timeout period for a read operation.  This must
        be specified as a relative time.

Return Value:

    The function value is the final status of the create operation.

--*/

{
    MAILSLOT_CREATE_PARAMETERS mailslotCreateParameters;

    PAGED_CODE();

    //
    // Check whether or not the DefaultTimeout parameter was specified.  If
    // so, then capture it in the mailslot create parameter structure.
    //

    if (ARGUMENT_PRESENT( ReadTimeout )) {

        //
        // Indicate that a read timeout period was specified.
        //

        mailslotCreateParameters.TimeoutSpecified = TRUE;

        //
        // A read timeout parameter was specified.  Check to see whether
        // the caller's mode is kernel and if not capture the parameter inside
        // of a try...except clause.
        //

        if (KeGetPreviousMode() != KernelMode) {
            try {
                ProbeForRead( ReadTimeout,
                              sizeof( LARGE_INTEGER ),
                              sizeof( ULONG ) );
                mailslotCreateParameters.ReadTimeout = *ReadTimeout;
            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // Something went awry attempting to access the parameter.
                // Get the reason for the error and return it as the status
                // value from this service.
                //

                return GetExceptionCode();
            }
        } else {

            //
            // The caller's mode was kernel so simply store the parameter.
            //

            mailslotCreateParameters.ReadTimeout = *ReadTimeout;
        }
    } else {

        //
        // Indicate that no default timeout period was specified.
        //

        mailslotCreateParameters.TimeoutSpecified = FALSE;
    }

    //
    // Store the mailslot-specific parameters in the structure for use
    // in the call to the common create file routine.
    //

    mailslotCreateParameters.MailslotQuota = MailslotQuota;
    mailslotCreateParameters.MaximumMessageSize = MaximumMessageSize;

    //
    // Simply perform the remainder of the service by allowing the common
    // file creation code to do the work.
    //

    return IoCreateFile( FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         (PLARGE_INTEGER) NULL,
                         0L,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_CREATE,
                         CreateOptions,
                         (PVOID) NULL,
                         0L,
                         CreateFileTypeMailslot,
                         &mailslotCreateParameters,
                         0 );
}
