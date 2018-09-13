/*++

Copyright (c) 1989 - 1995  Microsoft Corporation

Module Name:

    qsquota.c

Abstract:

    This module contains the code to implement the NtQueryQuotaInformationFile
    and the NtSetQuotaInformationFile system services for the NT I/O system.

Author:

    Darryl E. Havens (darrylh) 20-Jun-1995

Environment:

    Kernel mode only

Revision History:


--*/

#include "iop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtQueryQuotaInformationFile)
#pragma alloc_text(PAGE, NtSetQuotaInformationFile)
#endif

NTSTATUS
NtQueryQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID SidList OPTIONAL,
    IN ULONG SidListLength,
    IN PULONG StartSid OPTIONAL,
    IN BOOLEAN RestartScan
    )

/*++

Routine Description:

    This service returns quota entries associated with the volume specified
    by the FileHandle parameter.  The amount of information returned is based
    on the size of the quota information associated with the volume, the size
    of the buffer, and whether or not a specific set of entries has been
    requested.

Arguments:

    FileHandle - Supplies a handle to the file/volume for which the quota
        information is returned.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Supplies a buffer to receive the quota information for the volume.

    Length - Supplies the length, in bytes, of the buffer.

    ReturnSingleEntry - Indicates that only a single entry should be returned
        rather than filling the buffer with as many entries as possible.

    SidList - Optionally supplies a list of SIDs whose quota information is to
        be returned.

    SidListLength - Supplies the length of the SID list, if one was specified.

    StartSid - Supplies an optional SID that indicates that the returned
        information is to start with an entry other than the first.  This
        parameter is ignored if a SidList is specified.

    RestartScan - Indicates whether the scan of the quota information is to be
        restarted from the beginning.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{

#define ALIGN_LONG( Address ) ( (Address + 3) & ~3 )

    PIRP irp;
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PKEVENT event = (PKEVENT) NULL;
    PCHAR auxiliaryBuffer = (PCHAR) NULL;
    ULONG startSidLength = 0;
    PSID startSid = (PSID) NULL;
    PFILE_GET_QUOTA_INFORMATION sidList = (PFILE_GET_QUOTA_INFORMATION) NULL;
    KPROCESSOR_MODE requestorMode;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK localIoStatus;
    BOOLEAN synchronousIo;
    UCHAR subCount;

    PAGED_CODE();

    //
    // Get the previous mode;  i.e., the mode of the caller.
    //

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {

        //
        // The caller's access mode is not kernel so probe each of the arguments
        // and capture them as necessary.  If any failures occur, the condition
        // handler will be invoked to handle them.  It will simply cleanup and
        // return an access violation status code back to the system service
        // dispatcher.
        //

        try {

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

            //
            // The buffer must be writeable by the caller.
            //

#if defined(_X86_)
            ProbeForWrite( Buffer, Length, sizeof( ULONG ) );
#elif defined(_WIN64)

            //
            // If we are a wow64 process, follow the X86 rules
            //

            if (PsGetCurrentProcess()->Wow64Process) {
                ProbeForWrite( Buffer, Length, sizeof( ULONG ) );
            } else {
                ProbeForWrite( Buffer, Length, sizeof( ULONGLONG ) );
            }
#else
            ProbeForWrite( Buffer, Length, sizeof( ULONGLONG ) );
#endif

            //
            // If the optional StartSid parameter was specified, then it must
            // be readable by the caller.  Begin by capturing the length of
            // the SID so that the SID itself can be captured.
            //

            if (ARGUMENT_PRESENT( StartSid )) {

                subCount = ProbeAndReadUchar( &(((SID *)(StartSid))->SubAuthorityCount) );
                startSidLength = RtlLengthRequiredSid( subCount );
                ProbeForRead( StartSid, startSidLength, sizeof( ULONG ) );
            }

            //
            // If the optional SidList parameter was specified, then it must
            // be readable by the caller.  Validate that the buffer contains
            // a legal get information structure.
            //

            if (ARGUMENT_PRESENT( SidList ) && SidListLength) {

                ProbeForRead( SidList, SidListLength, sizeof( ULONG ) );
                auxiliaryBuffer = ExAllocatePoolWithQuota( NonPagedPool,
                                                           ALIGN_LONG( SidListLength ) +
                                                           startSidLength );
                sidList = (PFILE_GET_QUOTA_INFORMATION) auxiliaryBuffer;

                RtlCopyMemory( auxiliaryBuffer, SidList, SidListLength );

            } else {

                //
                // No SidList was specified.  Check to see whether or not a
                // StartSid was specified and, if so, capture it.  Note that
                // the SID has already been probed.
                //

                SidListLength = 0;
                if (ARGUMENT_PRESENT( StartSid )) {
                    auxiliaryBuffer = ExAllocatePoolWithQuota( PagedPool,
                                                               startSidLength );
                }
            }

            //
            // If a StartSid was specified tack it onto the end of the auxiliary
            // buffer.
            //

            if (ARGUMENT_PRESENT( StartSid )) {
                startSid = (PSID) (auxiliaryBuffer + ALIGN_LONG( SidListLength ));

                RtlCopyMemory( startSid, StartSid, startSidLength );
                ((SID *) startSid)->SubAuthorityCount = subCount;
            }


        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception was incurred while probing the caller's
            // parameters, allocating the pool buffer, or copying the
            // caller's EA list to the buffer.  Cleanup and return an
            // appropriate error status code.
            //

            if (auxiliaryBuffer) {
                ExFreePool( auxiliaryBuffer );
            }

            return GetExceptionCode();

        }

    } else {

        //
        // The caller's mode was KernelMode.  Simply allocate pool for the
        // SidList, if one was specified, and copy the string to it.  Also,
        // if a StartSid was specified copy it as well.
        //

        if (ARGUMENT_PRESENT( SidList ) && SidListLength) {
            sidList = SidList;
        }

        if (ARGUMENT_PRESENT( StartSid )) {
            startSid = StartSid;
        }
    }

    //
    // Always check the validity of the buffer since the server uses this 
    // routine.
    //

    if (sidList != NULL) {
        status = IopCheckGetQuotaBufferValidity( sidList,
                                                 SidListLength,
                                                 &IoStatusBlock->Information );
        if (!NT_SUCCESS( status )) {
            if (auxiliaryBuffer != NULL) {
                ExFreePool( auxiliaryBuffer );
            }
            return status;

        }
    }

    if (startSid != NULL) {

        if (!RtlValidSid( startSid )) {
            if (auxiliaryBuffer != NULL) {
                ExFreePool( auxiliaryBuffer );
            }
            return STATUS_INVALID_SID;
        }
    }

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    status = ObReferenceObjectByHandle( FileHandle,
                                        0,
                                        IoFileObjectType,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }
        return status;
    }

    //
    // Make a special check here to determine whether this is a synchronous
    // I/O operation.  If it is, then wait here until the file is owned by
    // the current thread.  If this is not a (serialized) synchronous I/O
    // operation, then allocate and initialize the local event.
    //

    if (fileObject->Flags & FO_SYNCHRONOUS_IO) {

        BOOLEAN interrupted;

        if (!IopAcquireFastLock( fileObject )) {
            status = IopAcquireFileObjectLock( fileObject,
                                               requestorMode,
                                               (BOOLEAN) ((fileObject->Flags & FO_ALERTABLE_IO) != 0),
                                               &interrupted );
            if (interrupted) {
                if (auxiliaryBuffer) {
                    ExFreePool( auxiliaryBuffer );
                }
                ObDereferenceObject( fileObject );
                return status;
            }
        }
        synchronousIo = TRUE;
    } else {

        //
        // This is a synchronous API being invoked for a file that is opened
        // for asynchronous I/O.  This means that this system service is
        // to synchronize the completion of the operation before returning
        // to the caller.  A local event is used to do this.
        //

        event = ExAllocatePool( NonPagedPool, sizeof( KEVENT ) );
        if (!event) {
            if (auxiliaryBuffer) {
                ExFreePool( auxiliaryBuffer );
            }
            ObDereferenceObject( fileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent( event, SynchronizationEvent, FALSE );
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

        if (!(fileObject->Flags & FO_SYNCHRONOUS_IO)) {
            ExFreePool( event );
        }

        IopAllocateIrpCleanup( fileObject, (PKEVENT) NULL );

        if (auxiliaryBuffer) {
            ExFreePool( auxiliaryBuffer );
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = requestorMode;

    //
    // Fill in the service independent parameters in the IRP.
    //

    if (synchronousIo) {
        irp->UserEvent = (PKEVENT) NULL;
        irp->UserIosb = IoStatusBlock;
    } else {
        irp->UserEvent = event;
        irp->UserIosb = &localIoStatus;
        irp->Flags = IRP_SYNCHRONOUS_API;
    }
    irp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_QUERY_QUOTA;
    irpSp->FileObject = fileObject;

    //
    // If the caller specified an SID list of names to be queried, then pass
    // the address of the intermediary buffer containing the list to the
    // driver.
    //

    irp->Tail.Overlay.AuxiliaryBuffer = auxiliaryBuffer;
    irpSp->Parameters.QueryQuota.SidList = sidList;
    irpSp->Parameters.QueryQuota.SidListLength = SidListLength;

    //
    // Now determine whether this driver expects to have data buffered
    // to it or whether it performs direct I/O.  This is based on the
    // DO_BUFFERED_IO flag in the device object.  If the flag is set,
    // then a system buffer is allocated and the driver's data will be
    // copied to it.  If the DO_DIRECT_IO flag is set in the device
    // object, then a Memory Descriptor List (MDL) is allocated and
    // the caller's buffer is locked down using it.  Finally, if the
    // driver specifies neither of the flags, then simply pass the
    // address and length of the buffer and allow the driver to perform
    // all of the checking and buffering if any is required.
    //

    if (deviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The driver wishes the caller's buffered be copied into an
        // intermediary buffer.  Allocate the system buffer and specify
        // that it should be deallocated on completion.  Also indicate
        // that this is an input operation so the data will be copied
        // into the caller's buffer.  This is done using an exception
        // handler that will perform cleanup if the operation fails.
        //

        if (Length) {
            try {

                //
                // Allocate the intermediary system buffer from nonpaged
                // pool and charge quota for it.
                //

                irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithQuota( NonPagedPool, Length );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the
                // caller's buffer or allocating the system buffer.
                // Determine what actually happened, clean everything
                // up, and return an appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                if (auxiliaryBuffer) {
                    ExFreePool( auxiliaryBuffer );
                }

                return GetExceptionCode();
            }

            //
            // Remember the address of the caller's buffer so the copy can
            // take place during I/O completion.  Also, set the flags so
            // that the completion code knows to do the copy and to deallocate
            // the buffer.
            //

            irp->UserBuffer = Buffer;
            irp->Flags |= (ULONG) (IRP_BUFFERED_IO |
                                   IRP_DEALLOCATE_BUFFER |
                                   IRP_INPUT_OPERATION);
        } else {
            irp->AssociatedIrp.SystemBuffer = NULL;
            irp->UserBuffer = Buffer;
        }

    } else if (deviceObject->Flags & DO_DIRECT_IO) {

        PMDL mdl;

        //
        // This is a direct I/O operation.  Allocate an MDL and invoke
        // the memory management routine to lock the buffer into memory.
        // This is done using an exception handler that will perform
        // cleanup if the operation fails.
        //

        mdl = (PMDL) NULL;

        if (Length) {
            try {

                //
                // Allocate an MDL, charging quota for it, and hang it off
                // of the IRP.  Probe and lock the pages associated with
                // the caller's buffer for write access and fill in the MDL
                // with the PFNs of those pages.
                //

                mdl = IoAllocateMdl( Buffer, Length, FALSE, TRUE, irp );
                if (!mdl) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( mdl, requestorMode, IoWriteAccess );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception was incurred while either probing the
                // caller's buffer or allocating the MDL.  Determine what
                // actually happened, clean everything up, and return an
                // appropriate error status code.
                //

                IopExceptionCleanup( fileObject,
                                     irp,
                                     (PKEVENT) NULL,
                                     event );

                if (auxiliaryBuffer) {
                    ExFreePool( auxiliaryBuffer );
                }

                return GetExceptionCode();

            }
        }

    } else {

        //
        // Pass the address of the user's buffer so the driver has access
        // to it.  It is now the driver's responsibility to do everything.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryQuota.Length = Length;
    irpSp->Parameters.QueryQuota.StartSid = StartSid;
    irpSp->Flags = 0;
    if (RestartScan) {
        irpSp->Flags = SL_RESTART_SCAN;
    }
    if (ReturnSingleEntry) {
        irpSp->Flags |= SL_RETURN_SINGLE_ENTRY;
    }
    if (ARGUMENT_PRESENT( StartSid )) {
        irpSp->Flags |= SL_INDEX_SPECIFIED;
    }

    //
    // Queue the packet, call the driver, and synchronize appropriately with
    // I/O completion.
    //

    status = IopSynchronousServiceTail( deviceObject,
                                        irp,
                                        fileObject,
                                        FALSE,
                                        requestorMode,
                                        synchronousIo,
                                        OtherTransfer );

    //
    // If the file for this operation was not opened for synchronous I/O, then
    // synchronization of completion of the I/O operation has not yet occurred
    // since the allocated event must be used for synchronous APIs on files
    // opened for asynchronous I/O.  Synchronize the completion of the I/O
    // operation now.
    //

    if (!synchronousIo) {

        status = IopSynchronousApiServiceTail( status,
                                               event,
                                               irp,
                                               requestorMode,
                                               &localIoStatus,
                                               IoStatusBlock );
    }

    return status;
}

NTSTATUS
NtSetQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This service changes quota entries for the volume associated with the
    FileHandle parameter.  All of the quota entries in the specified buffer
    are applied to the volume.

Arguments:

    FileHandle - Supplies a handle to the file/volume for which the quota
        entries are to be applied.

    IoStatusBlock - Address of the caller's I/O status block.

    Buffer - Supplies a buffer containing the new quota entries that should
        be applied to the volume.

    Length - Supplies the length, in bytes, of the buffer.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    PAGED_CODE();

    //
    // Simply return the status from the internal common routine for setting
    // EAs on a file or quotas on a volume.
    //

    return IopSetEaOrQuotaInformationFile( FileHandle,
                                           IoStatusBlock,
                                           Buffer,
                                           Length,
                                           FALSE );
}
