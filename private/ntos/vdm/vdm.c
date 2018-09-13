/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdm.c

Abstract:

    This module supplies the entry point to the system for manipulating vdms.

Author:

    Dave Hastings (daveh) 6-Apr-1992

Revision History:

--*/


#include <ntos.h>
#include <vdmntos.h>
#include <ntvdmp.h>

#include <zwapi.h>
#include <fsrtl.h>


typedef struct _QueryDirPoolData {
    KEVENT         kevent;
    UNICODE_STRING FileName;
    WCHAR          FileNameBuf[1];
} QDIR_POOLDATA, *PQDIR_POOLDATA;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmQueryDirectoryFile)
#endif

#if !defined(i386)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtVdmControl)
#endif

NTSTATUS
NtVdmControl(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    )
/*++

Routine Description:

    This routine is the entry point for controlling Vdms.
    On risc it returns STATUS_NOT_IMPLEMENTED.
    On 386 the entry point is in i386\vdmentry.c

Arguments:

    Service -- Specifies what service is to be performed
    ServiceData -- Supplies a pointer to service specific data

Return Value:


--*/
{
    PAGED_CODE();


    if (Service == VdmQueryDir) {
        return VdmQueryDirectoryFile(ServiceData);
        }

    return STATUS_NOT_IMPLEMENTED;

}
#endif


extern POBJECT_TYPE IoFileObjectType;

NTSTATUS
VdmQueryDirectoryFile(
    PVDMQUERYDIRINFO pVdmQueryDir
    )

/*++
    This VDM specific service allows vdm to restart searches at a specified
    location in the dir search by using the FileIndex, FileName parameters
    passed back from previous query calls.

    See NtQueryDirectoryFile for additional documentation.

Arguments: PVDMQUERYDIRINFO pVdmQueryDir

    FileHandle - Supplies a handle to the directory file for which information
        should be returned.

    FileInformation - Supplies a buffer to receive the requested information
        returned about the contents of the directory.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileName - Supplies a file name within the specified directory.

    FileIndex - Supplies a file index within the specified directory.

    The FileInformationClass is assumed to be FILE_BOTH_DIR_INFORMATION
    The Caller's mode is assumed to be UserMode
    Synchronous IO is used

--*/

{
    KIRQL    irql;
    NTSTATUS status;
    PKEVENT  Event;

    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID FileInformation;
    ULONG Length;
    UNICODE_STRING FileName;
    PUNICODE_STRING pFileNameSrc;
    ULONG FileIndex;

    PQDIR_POOLDATA QDirPoolData = NULL;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    PMDL mdl;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PCHAR SystemBuffer;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT DeviceObject;


    PAGED_CODE();

    //
    // We assume that the caller is usermode, so verify all parameters
    // accordingly
    //

    try {

        //
        // Copy out the callers service data into local variables
        //
        ProbeForRead( pVdmQueryDir, sizeof(VDMQUERYDIRINFO), sizeof(ULONG));

        FileHandle      = pVdmQueryDir->FileHandle;
        FileInformation = pVdmQueryDir->FileInformation;
        Length          = pVdmQueryDir->Length;
        FileIndex       = pVdmQueryDir->FileIndex;
        pFileNameSrc    = pVdmQueryDir->FileName;

        //
        // Ensure that we have a valid file name string
        //

        //
        // check for pVdmQueryDir->Filename validity first
        //
        if (NULL == pFileNameSrc) {
           return(STATUS_INVALID_PARAMETER);
        }

        FileName = ProbeAndReadUnicodeString(pFileNameSrc);
        if (!FileName.Length ||
            FileName.Length > MAXIMUM_FILENAME_LENGTH<<1) {
            return(STATUS_INVALID_PARAMETER);
        }

        ProbeForRead(FileName.Buffer, FileName.Length, sizeof( UCHAR ));

        //
        // The FileInformation buffer must be writeable by the caller.
        //

        ProbeForWrite( FileInformation, Length, sizeof( ULONG ) );

        //
        // Ensure that the caller's supplied buffer is at least large enough
        // to contain the fixed part of the structure required for this
        // query.
        //

        if (Length < sizeof(FILE_BOTH_DIR_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }



        //
        // Allocate from nonpaged pool a buffer large enough to contain
        // the file name, and the kevent used to wait for io.
        //

        QDirPoolData = (PQDIR_POOLDATA) ExAllocatePoolWithQuotaTag(
                                           NonPagedPool,
                                           sizeof(QDIR_POOLDATA) + FileName.Length,
                                           ' MDV');

        //
        // Capture the file name string into the nonpaged pool block.
        //

        QDirPoolData->FileName.Length = FileName.Length;
        QDirPoolData->FileName.MaximumLength = FileName.Length;
        QDirPoolData->FileName.Buffer = QDirPoolData->FileNameBuf;
        RtlCopyMemory( QDirPoolData->FileNameBuf,
                       FileName.Buffer,
                       FileName.Length );


    } except(EXCEPTION_EXECUTE_HANDLER) {

        if (QDirPoolData) {
            ExFreePool(QDirPoolData);
        }

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
                                        UserMode,
                                        (PVOID *) &fileObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    if (!NT_SUCCESS( status )) {
        if (QDirPoolData) {
            ExFreePool(QDirPoolData);
        }
        return status;
    }

    //
    //  We don't handle FO_SYNCHRONOUS_IO, because it requires
    //  io internal finctionality. Ntvdm can get away with this
    //  because it serializes access to the dir handle.
    //

    //
    // Initialize the kernel event that will signal I/O completion
    //
    Event = &QDirPoolData->kevent;
    KeInitializeEvent(Event, SynchronizationEvent, FALSE);


    //
    // Set the file object to the Not-Signaled state.
    //

    KeClearEvent( &fileObject->Event );

    //
    // Get the address of the target device object.
    //

    DeviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        ObDereferenceObject( fileObject );
        if (QDirPoolData) {
            ExFreePool(QDirPoolData);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->Flags = (ULONG)IRP_SYNCHRONOUS_API;
    irp->RequestorMode = UserMode;

    irp->UserIosb = &IoStatusBlock;
    irp->UserEvent = Event;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    irp->AssociatedIrp.SystemBuffer = (PVOID) NULL;
    SystemBuffer = NULL;


    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    irp->MdlAddress = NULL;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
    irpSp->FileObject = fileObject;


    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    irpSp->Parameters.QueryDirectory.Length = Length;
    irpSp->Parameters.QueryDirectory.FileInformationClass = FileBothDirectoryInformation;
    irpSp->Parameters.QueryDirectory.FileIndex = FileIndex;

    if (QDirPoolData->FileName.Length) {
        irpSp->Parameters.QueryDirectory.FileName = (PSTRING)&QDirPoolData->FileName;
    } else {
        irpSp->Parameters.QueryDirectory.FileName = NULL;
    }

    irpSp->Flags = SL_INDEX_SPECIFIED;


    //
    // Now determine whether this driver expects to have data buffered to it
    // or whether it performs direct I/O.  This is based on the DO_BUFFERED_IO
    // flag in the device object.  If the flag is set, then a system buffer is
    // allocated and the driver's data will be copied into it.  Otherwise, a
    // Memory Descriptor List (MDL) is allocated and the caller's buffer is
    // locked down using it.
    //

    if (DeviceObject->Flags & DO_BUFFERED_IO) {

        //
        // The file system wants buffered I/O.  Pass the address of the
        // "system buffer" in the IRP.  Note that we don't want the buffer
        // deallocated, nor do we want the I/O system to copy to a user
        // buffer, so we don't set the corresponding flags in irp->Flags.
        //


        try {

            //
            // Allocate the intermediary system buffer from nonpaged pool and
            // charge quota for it.
            //

            SystemBuffer = ExAllocatePoolWithQuotaTag( NonPagedPool,
                                                       Length,
                                                       ' MDV' );

            irp->AssociatedIrp.SystemBuffer = SystemBuffer;


        } except(EXCEPTION_EXECUTE_HANDLER) {

            IoFreeIrp(irp);

            ObDereferenceObject( fileObject );

            if (QDirPoolData) {
                ExFreePool(QDirPoolData);
            }

            return GetExceptionCode();
        }


    } else if (DeviceObject->Flags & DO_DIRECT_IO) {

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
            MmProbeAndLockPages( mdl, UserMode, IoWriteAccess );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            if (irp->MdlAddress != NULL) {
                 IoFreeMdl( irp->MdlAddress );
            }

            IoFreeIrp(irp);

            ObDereferenceObject( fileObject );

            if (QDirPoolData) {
                ExFreePool(QDirPoolData);
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
    // Insert the packet at the head of the IRP list for the thread.
    //

    KeRaiseIrql( APC_LEVEL, &irql );
    InsertHeadList( &irp->Tail.Overlay.Thread->IrpList,
                    &irp->ThreadListEntry );
    KeLowerIrql( irql );


    //
    // invoke the driver and wait for it to complete
    //

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {
        status = KeWaitForSingleObject(
                     Event,
                     UserRequest,
                     UserMode,
                     FALSE,
                     NULL );
    }

    if (NT_SUCCESS(status)) {
        status = IoStatusBlock.Status;
        if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) {
            if (SystemBuffer) {
                try {
                    RtlCopyMemory( FileInformation,
                                   SystemBuffer,
                                   IoStatusBlock.Information
                                   );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                }
            }
        }
    }


    //
    // Cleanup any memory allocated
    //
    if (QDirPoolData) {
        ExFreePool(QDirPoolData);
    }

    if (SystemBuffer) {
        ExFreePool(SystemBuffer);
    }


    return status;
}
