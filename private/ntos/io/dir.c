/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dir.c

Abstract:

    This module contains the code to implement the NtQueryDirectoryFile,
    and the NtNotifyChangeDirectoryFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 21-Jun-1989

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

NTSTATUS
BuildQueryDirectoryIrp(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan,
    IN UCHAR MinorFunction,
    OUT BOOLEAN *SynchronousIo,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT PIRP *Irp,
    OUT PFILE_OBJECT *FileObject,
    OUT KPROCESSOR_MODE *RequestorMode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BuildQueryDirectoryIrp)
#pragma alloc_text(PAGE, NtQueryDirectoryFile)
#pragma alloc_text(PAGE, NtNotifyChangeDirectoryFile)
#endif

NTSTATUS
BuildQueryDirectoryIrp(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan,
    IN UCHAR MinorFunction,
    OUT BOOLEAN *SynchronousIo,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT PIRP *Irp,
    OUT PFILE_OBJECT *FileObject,
    OUT KPROCESSOR_MODE *RequestorMode
    )

/*++

Routine Description:

    This service operates on a directory file or OLE container specified by the
    FileHandle parameter.  The service returns information about files in the
    directory or embeddings and streams in the container specified by the file
    handle.  The ReturnSingleEntry parameter specifies that only a single entry
    should be returned rather than filling the buffer.  The actual number of
    files whose information is returned, is the smallest of the following:

        o  One entry, if the ReturnSingleEntry parameter is TRUE.

        o  The number of entries whose information fits into the specified
           buffer.

        o  The number of entries that exist.

        o  One entry if the optional FileName parameter is specified.

    If the optional FileName parameter is specified, then the only information
    that is returned is for that single entries, if it exists.  Note that the
    file name may not specify any wildcard characters according to the naming
    conventions of the target file system.  The ReturnSingleEntry parameter is
    simply ignored.

    The information that is obtained about the entries in the directory or OLE
    container is based on the FileInformationClass parameter.  Legal values are
    hard coded based on the MinorFunction.

Arguments:

    FileHandle - Supplies a handle to the directory file or OLE container for
        which information should be returned.

    Event - Supplies an optional event to be set to the Signaled state when
        the query is complete.

    ApcRoutine - Supplies an optional APC routine to be executed when the
        query is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine,
        if an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the contents of the directory.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specfies the type of information that is to be
        returned about the files in the specified directory or OLE container.

    ReturnSingleEntry - Supplies a BOOLEAN value that, if TRUE, indicates that
        only a single entry should be returned.

    FileName - Optionally supplies a file name within the specified directory
        or OLE container.

    RestartScan - Supplies a BOOLEAN value that, if TRUE, indicates that the
        scan should be restarted from the beginning.  This parameter must be
        set to TRUE by the caller the first time the service is invoked.

    MinorFunction - IRP_MN_QUERY_DIRECTORY or IRP_MN_QUERY_OLE_DIRECTORY

    SynchronousIo - pointer to returned BOOLEAN; TRUE if synchronous I/O

    DeviceObject - pointer to returned pointer to device object

    Irp - pointer to returned pointer to device object

    FileObject - pointer to returned pointer to file object

    RequestorMode - pointer to returned requestor mode

Return Value:

    The status returned is STATUS_SUCCESS if a valid irp was created for the
    query operation.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT eventObject = (PKEVENT) NULL;
    KPROCESSOR_MODE requestorMode;
    PCHAR auxiliaryBuffer = (PCHAR) NULL;
    PIO_STACK_LOCATION irpSp;
    PMDL mdl;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();
    *RequestorMode = requestorMode;

    try {

        if (requestorMode != KernelMode) {

            ULONG operationlength = 0;  // assume invalid

            //
            // The caller's access mode is not kernel so probe and validate
            // each of the arguments as necessary.  If any failures occur,
            // the condition handler will be invoked to handle them.  It
            // will simply cleanup and return an access violation status
            // code back to the system service dispatcher.
            //

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatusEx( IoStatusBlock, ApcRoutine);

            //
            // Ensure that the FileInformationClass parameter is legal for
            // querying information about files in the directory or object.
            //

            if (FileInformationClass == FileDirectoryInformation) {
                operationlength = sizeof(FILE_DIRECTORY_INFORMATION);
            } else if (MinorFunction == IRP_MN_QUERY_DIRECTORY) {
                switch (FileInformationClass)
                {
                case FileFullDirectoryInformation:
                    operationlength = sizeof(FILE_FULL_DIR_INFORMATION);
                    break;

                case FileBothDirectoryInformation:
                    operationlength = sizeof(FILE_BOTH_DIR_INFORMATION);
                    break;

                case FileNamesInformation:
                    operationlength = sizeof(FILE_NAMES_INFORMATION);
                    break;

                case FileObjectIdInformation:
                    operationlength = sizeof(FILE_OBJECTID_INFORMATION);
                    break;

                case FileQuotaInformation:
                    operationlength = sizeof(FILE_QUOTA_INFORMATION);
                    break;

                case FileReparsePointInformation:
                    operationlength = sizeof(FILE_REPARSE_POINT_INFORMATION);
                    break;                    
                }
            }

            //
            // If the FileInformationClass parameter is illegal, fail now.
            //

            if (operationlength == 0) {
                return STATUS_INVALID_INFO_CLASS;
            }

            //
            // Ensure that the caller's supplied buffer is at least large enough
            // to contain the fixed part of the structure required for this
            // query.
            //

            if (Length < operationlength) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }


            //
            // The FileInformation buffer must be writeable by the caller.
            //

#if defined(_X86_)
            ProbeForWrite( FileInformation, Length, sizeof( ULONG ) );
#elif defined(_WIN64)

            //
            // If we are a wow64 process, follow the X86 rules
            //

            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForWrite( FileInformation, Length, sizeof( ULONG ) );
            } else {
                ProbeForWrite( FileInformation,
                               Length,
                               IopQuerySetAlignmentRequirement[FileInformationClass] );
            }
            
#else
            ProbeForWrite( FileInformation,
                           Length,
                           IopQuerySetAlignmentRequirement[FileInformationClass] );
#endif
        }

        //
        // If the optional FileName parameter was specified, then it must be
        // readable by the caller.  Capture the file name string in a pool
        // block.  Note that if an error occurs during the copy, the cleanup
        // code in the exception handler will deallocate the pool before
        // returning an access violation status.
        //

        if (ARGUMENT_PRESENT( FileName )) {

            UNICODE_STRING fileName;
            PUNICODE_STRING nameBuffer;

            //
            // Capture the string descriptor itself to ensure that the
            // string is readable by the caller without the caller being
            // able to change the memory while its being checked.
            //

            if (requestorMode != KernelMode) {
                fileName = ProbeAndReadUnicodeString( FileName );
            } else {
                fileName = *FileName;
            }

            if (fileName.Length) {

                //
                // The length of the string is non-zero, so probe the
                // buffer described by the descriptor if the caller was
                // not kernel mode.  Likewise, if the caller's mode was
                // not kernel, then check the length of the name string
                // to ensure that it is not too long.
                //

                if (requestorMode != KernelMode) {
                    ProbeForRead( fileName.Buffer,
                                  fileName.Length,
                                  sizeof( UCHAR ) );
                    //
                    // account for unicode
                    //

                    if (fileName.Length > MAXIMUM_FILENAME_LENGTH<<1) {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }
                }

                //
                // Allocate an auxiliary buffer large enough to contain
                // a file name descriptor and to hold the entire file
                // name itself.  Copy the body of the string into the
                // buffer.
                //

                auxiliaryBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                           fileName.Length + sizeof( UNICODE_STRING ) );
                RtlCopyMemory( auxiliaryBuffer + sizeof( UNICODE_STRING ),
                               fileName.Buffer,
                               fileName.Length );

                //
                // Finally, build the Unicode string descriptor in the
                // auxiliary buffer.
                //

                nameBuffer = (PUNICODE_STRING) auxiliaryBuffer;
                nameBuffer->Length = fileName.Length;
                nameBuffer->MaximumLength = fileName.Length;
                nameBuffer->Buffer = (PWSTR) (auxiliaryBuffer + sizeof( UNICODE_STRING ) );
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while probing the caller's buffers,
        // attempting to allocate a pool buffer, or while trying to copy
        // the caller's data.  Determine what happened, clean everything
        // up, and return an appropriate error status code.
        //

        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }

#if DBG
        if (GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) {
            DbgBreakPoint();
        }
#endif // DBG

        return GetExceptionCode();
    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        FILE_LIST_DIRECTORY,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    if (!NT_SUCCESS( status )) {
        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }
        return status;
    }
    *FileObject = fileObject;

    //
    // If this file has an I/O completion port associated w/it, then ensure
    // that the caller did not supply an APC routine, as the two are mutually
    // exclusive methods for I/O completion notification.
    //

    if (fileObject->CompletionContext && IopApcRoutinePresent( ApcRoutine )) {
        ObDereferenceObject( fileObject );
        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }
        return STATUS_INVALID_PARAMETER;

    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here, too, that if
    // the handle does not refer to an event, or if the event cannot be
    // written, then the reference will fail.
    //

    if (ARGUMENT_PRESENT( Event )) {
        status = ObReferenceObjectByHandle( Event,
                                            EVENT_MODIFY_STATE,
                                            ExEventObjectType,
                                            requestorMode,
                                            (PVOID *) &eventObject,
                                            (POBJECT_HANDLE_INFORMATION) NULL );
        if (!NT_SUCCESS( status )) {
            if (auxiliaryBuffer) {
                ExFreePool( auxiliaryBuffer );
            }
            ObDereferenceObject( fileObject );
            return status;
        } else {
            KeClearEvent( eventObject );
        }
    }

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                if (auxiliaryBuffer != NULL) {
                    ExFreePool( auxiliaryBuffer );
                }
                if (eventObject != NULL) {
                    ObDereferenceObject( eventObject );
                }
                ObDereferenceObject( fileObject );
                return status;
            }
        }
        *SynchronousIo = TRUE;
    } else {
        *SynchronousIo = FALSE;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );
    *DeviceObject = deviceObject;

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopAllocateIrpCleanup( fileObject, eventObject );
        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *Irp = irp;

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = requestorMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = MinorFunction;
    irpSp->FileObject = fileObject;

    // Also, copy the caller's parameters to the service-specific portion of
    // the IRP.
    //

    irp->Tail.Overlay.AuxiliaryBuffer = auxiliaryBuffer;
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    irp->MdlAddress = (PMDL) NULL;

    //
    // Now determine whether this driver expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the driver's data will be copied into it.  Otherwise, a
    // Memory Descriptor List (MDL) is allocated and the caller's buffer is
    // locked down using it.
    //

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The device does not support direct I/O.  Allocate a system buffer
        // and specify that it should be deallocated on completion.  Also
        // indicate that this is an input operation so the data will be copied
        // into the caller's buffer.  This is done using an exception handler
        // that will perform cleanup if the operation fails.
        //

        try {

            //
            // Allocate the intermediary system buffer from nonpaged pool and
            // charge quota for it.
            //

            irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithQuota( NonPagedPool, Length );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while either probing the caller's
            // buffer or allocate the system buffer.  Determine what actually
            // happened, clean everything up, and return an appropriate error
            // status code.
            //

            IopExceptionCleanup( fileObject,
                                 irp,
                                 eventObject,
                                 (PKEVENT) NULL );

            if (auxiliaryBuffer != NULL) {
                ExFreePool( auxiliaryBuffer );
            }

            return GetExceptionCode();

        }

        //
        // Remember the address of the caller's buffer so the copy can take
        // place during I/O completion.  Also, set the flags so that the
        // completion code knows to do the copy and to deallocate the buffer.
        //

        irp->UserBuffer = FileInformation;
        irp->Flags = (ULONG) (IRP_BUFFERED_IO |
                              IRP_DEALLOCATE_BUFFER |
                              IRP_INPUT_OPERATION);

    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke the
        // memory management routine to lock the buffer into memory.  This is
        // done using an exception handler that will perform cleanup if the
        // operation fails.
        //

        mdl = (PMDL) NULL;

        try {

            //
            // Allocate an MDL, charging quota for it, and hang it off of the
            // IRP.  Probe and lock the pages associated with the caller's
            // buffer for write access and fill in the MDL with the PFNs of
            // those pages.
            //

            mdl = IoAllocateMdl( FileInformation, Length, FALSE, TRUE, irp );
            if (mdl == NULL) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
            MmProbeAndLockPages( mdl, requestorMode, IoWriteAccess );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while either probing the caller's
            // buffer or allocating the MDL.  Determine what actually happened,
            // clean everything up, and return an appropriate error status code.
            //

            IopExceptionCleanup( fileObject,
                                 irp,
                                 eventObject,
                                 (PKEVENT) NULL );

            if (auxiliaryBuffer != NULL) {
                ExFreePool( auxiliaryBuffer );
            }

            return GetExceptionCode();

        }

    } else {

        //
        // Pass the address of the user's buffer so the driver has access to
        // it.  It is now the driver's responsibility to do everything.
        //

        irp->UserBuffer = FileInformation;

    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryDirectory.Length = Length;
    irpSp->Parameters.QueryDirectory.FileInformationClass = FileInformationClass;
    irpSp->Parameters.QueryDirectory.FileIndex = 0;
    irpSp->Parameters.QueryDirectory.FileName = (PSTRING) auxiliaryBuffer;
    irpSp->Flags = 0;
    if (RestartScan) {
        irpSp->Flags = SL_RESTART_SCAN;
    }
    if (ReturnSingleEntry) {
        irpSp->Flags |= SL_RETURN_SINGLE_ENTRY;
    }

    irp->Flags |= IRP_DEFER_IO_COMPLETION;

    //
    // Return with everything set up for the caller to complete the I/O.
    //

    return STATUS_SUCCESS;
}

NTSTATUS
NtQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
    )

/*++

Routine Description:

    This service operates on a directory file specified by the FileHandle
    parameter.  The service returns information about files in the directory
    specified by the file handle.  The ReturnSingleEntry parameter specifies
    that only a single entry should be returned rather than filling the buffer.
    The actual number of files whose information is returned, is the smallest
    of the following:

        o  One entry, if the ReturnSingleEntry parameter is TRUE.

        o  The number of files whose information fits into the specified
           buffer.

        o  The number of files that exist.

        o  One entry if the optional FileName parameter is specified.

    If the optional FileName parameter is specified, then the only information
    that is returned is for that single file, if it exists.  Note that the
    file name may not specify any wildcard characters according to the naming
    conventions of the target file system.  The ReturnSingleEntry parameter is
    simply ignored.

    The information that is obtained about the files in the directory is based
    on the FileInformationClass parameter.  The legal values are as follows:

        o  FileNamesInformation

        o  FileDirectoryInformation

        o  FileFullDirectoryInformation

Arguments:

    FileHandle - Supplies a handle to the directory file for which information
        should be returned.

    Event - Supplies an optional event to be set to the Signaled state when
        the query is complete.

    ApcRoutine - Supplies an optional APC routine to be executed when the
        query is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine,
        if an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the contents of the directory.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specfies the type of information that is to be
        returned about the files in the specified directory.

    ReturnSingleEntry - Supplies a BOOLEAN value that, if TRUE, indicates that
        only a single entry should be returned.

    FileName - Optionally supplies a file name within the specified directory.

    RestartScan - Supplies a BOOLEAN value that, if TRUE, indicates that the
        scan should be restarted from the beginning.  This parameter must be
        set to TRUE by the caller the first time the service is invoked.

Return Value:

    The status returned is success if the query operation was properly queued
    to the I/O system.  Once the operation completes, the status of the query
    can be determined by examining the Status field of the I/O status block.

--*/

{
    NTSTATUS status;
    BOOLEAN synchronousIo;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PFILE_OBJECT fileObject;
    KPROCESSOR_MODE requestorMode;

    PAGED_CODE();

    //
    // Build the irp with the appropriate minor function & allowed info levels.
    //

    status = BuildQueryDirectoryIrp( FileHandle,
                                     Event,
                                     ApcRoutine,
                                     ApcContext,
                                     IoStatusBlock,
                                     FileInformation,
                                     Length,
                                     FileInformationClass,
                                     ReturnSingleEntry,
                                     FileName,
                                     RestartScan,
                                     IRP_MN_QUERY_DIRECTORY,
                                     &synchronousIo,
                                     &deviceObject,
                                     &irp,
                                     &fileObject,
                                     &requestorMode);
    if (NT_SUCCESS( status )) {

        //
        // Queue the packet, call the driver, and synchronize appopriately with
        // I/O completion.
        //
        status = IopSynchronousServiceTail( deviceObject,
                                            irp,
                                            fileObject,
                                            TRUE,
                                            requestorMode,
                                            synchronousIo,
                                            OtherTransfer );
    }
    return status;
}

NTSTATUS
NtNotifyChangeDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree
    )

/*++

Routine Description:

    This service monitors a directory file for changes.  Once a change is
    made to the directory specified by the FileHandle parameter, the I/O
    operation is completed.

Arguments:

    FileHandle - Supplies a handle to the file whose EAs should be changed.

    Event - Supplies an optional event to be set to the Signaled state when the
        change is complete.

    ApcRoutine - Supplies an optional APC routine to be executed when the change
        is complete.

    ApcContext - Supplies a context parameter to be passed to the ApcRoutine,
        if an ApcRoutine was specified.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Address of variable to receive the names of the files or
        directories that have changed since the last time that the service
        was invoked.

    Length - Length of the output buffer.  On the first call, this parameter
        also serves as a guideline for how large to make the system's
        internal buffer.  Specifying a buffer length of zero causes the request
        to complete when changes are made, but no information about the
        changes are returned.

    CompletionFilter - Indicates the types of changes to files or directories
        within the directory that will complete the I/O operation.

    WatchTree - A BOOLEAN value that indicates whether or not changes to
        directories below the directory referred to by the FileHandle
        parameter cause the operation to complete.

Return Value:

    The status returned is success if the operation was properly queued to the
    I/O system.  Once the operation completes, the status of the operation can
    be determined by examining the Status field of the I/O status block.

--*/

{
    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT eventObject = (PKEVENT) NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN synchronousIo;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is user, so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatusEx( IoStatusBlock , ApcRoutine);

            //
            // The Buffer parameter must be writeable by the caller.
            //

            if (Length != 0) {
                ProbeForWrite( Buffer,
                               Length,
                               sizeof( ULONG ) );
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred probing the caller's I/O status
            // block.  Simply return the appropriate error status code.
            //

            return GetExceptionCode();

        }

        //
        // The CompletionFilter parameter must not contain any values which
        // are illegal, nor may it not specifiy anything at all.  Likewise,
        // the caller must supply a non-null buffer.
        //

        if (((CompletionFilter & ~FILE_NOTIFY_VALID_MASK) ||
            !CompletionFilter)) {
            return STATUS_INVALID_PARAMETER;
        }

    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        FILE_LIST_DIRECTORY,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // If this file has an I/O completion port associated w/it, then ensure
    // that the caller did not supply an APC routine, as the two are mutually
    // exclusive methods for I/O completion notification.
    //

    if (fileObject->CompletionContext && IopApcRoutinePresent( ApcRoutine )) {
        ObDereferenceObject( fileObject );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here too, that if
    // the handle does not refer to an event, or if the event cannot be
    // written, then the reference will fail.
    //

    if (ARGUMENT_PRESENT( Event )) {
        status = ObReferenceObjectByHandle( Event,
                                            EVENT_MODIFY_STATE,
                                            ExEventObjectType,
                                            requestorMode,
                                            (PVOID *) &eventObject,
                                            (POBJECT_HANDLE_INFORMATION) NULL );
        if (!NT_SUCCESS( status )) {
            ObDereferenceObject( fileObject );
            return status;
        } else {
            KeClearEvent( eventObject );
        }
    }

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                if (eventObject != NULL) {
                    ObDereferenceObject( eventObject );
                }
                ObDereferenceObject( fileObject );
                return status;
            }
        }
        synchronousIo = TRUE;
    } else {
        synchronousIo = FALSE;
    }

    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Get the address of the target device object.
    //

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopAllocateIrpCleanup( fileObject, eventObject );

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = requestorMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and the parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
    irpSp->FileObject = fileObject;

    //
    // Now determine whether this device expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the driver's data will be copied into it.  Otherwise, a
    // Memory Descriptor List (MDL) is allocated and the caller's buffer is
    // locked down using it.
    //

    if (Length != 0) {

        if (deviceObject->Flags & DO_BUFFERED_IO) {

            //
            // The device does not support direct I/O.  Allocate a system
            // buffer and specify that it should be deallocated on completion.
            // Also indicate that this is an input operation so the data will
            // be copied into the caller's buffer.  This is done using an
            // exception handler that will perform cleanup if the operation
            // fails.
            //

            try {

                //
                // Allocate the intermediary system buffer from nonpaged pool
                // and charge quota for it.
                //

                irp->AssociatedIrp.SystemBuffer =
                     ExAllocatePoolWithQuota( NonPagedPool, Length );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while attempting to allocate the
                // intermediary system buffer.  Clean everything up and return
                // an appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     eventObject,
                                     (PKEVENT) NULL );

                return GetExceptionCode();

            }

            //
            // Remember the address of the caller's buffer so the copy can take
            // place during I/O completion.  Also, set the flags so that the
            // completion code knows to do the copy and to deallocate the
            // buffer.
            //

            irp->UserBuffer = Buffer;
            irp->Flags = IRP_BUFFERED_IO |
                         IRP_DEALLOCATE_BUFFER |
                         IRP_INPUT_OPERATION;

        } else if (deviceObject->Flags & DO_DIRECT_IO) {

            //
            // This is a direct I/O operation.  Allocate an MDL and invoke the
            // memory management routine to lock the buffer into memory.  This
            // is done using an exception handler that will perform cleanup if
            // the operation fails.
            //

            PMDL mdl;

            mdl = (PMDL) NULL;

            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off of
                // the IRP.  Probe and lock the pages associated with the
                // caller's buffer for write access and fill in the MDL with
                // the PFNs of those pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (mdl == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( mdl, requestorMode, IoWriteAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the caller's
                // buffer of allocating the MDL.  Determine what actually
                // happened, clean everything up, and return an appropriate
                // error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     eventObject,
                                     (PKEVENT) NULL );

                return GetExceptionCode();

            }

        } else {

            //
            // Pass the address of the user's buffer so the driver has access
            // to it.  It is now the driver's responsibility to do everything.
            //

            irp->UserBuffer = Buffer;

        }
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.NotifyDirectory.Length = Length;
    irpSp->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;
    if (WatchTree) {
        irpSp->Flags = SL_WATCH_TREE;
    }

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //

    return IopSynchronousServiceTail( deviceObject,
                                      irp,
                                      fileObject,
                                      FALSE,
                                      requestorMode,
                                      synchronousIo,
                                      OtherTransfer );
}
