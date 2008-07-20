/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    CdInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Cdfs


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CDINIT)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
CdUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CdInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FileSystemDeviceObject
    );

NTSTATUS
CdShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CdUnload)
#pragma alloc_text(PAGE, CdShutdown)
#pragma alloc_text(INIT, CdInitializeGlobalData)
#endif


//
//  Local support routine
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Cdrom file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PDEVICE_OBJECT CdfsFileSystemDeviceObject;

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Cdfs" );

    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                             0,
                             FALSE,
                             &CdfsFileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }
    DriverObject->DriverUnload = CdUnload;
    //
    //  Note that because of the way data caching is done, we set neither
    //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not in the cache, or the request is not buffered, we may,
    //  set up for Direct I/O by hand.
    //

    //
    //  Initialize the driver object with this driver's entry points.
    //
    //  NOTE - Each entry in the dispatch table must have an entry in
    //  the Fsp/Fsd dispatch switch statements.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   =
    DriverObject->MajorFunction[IRP_MJ_READ]                    =
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]       =
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]         =
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]=
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]       =
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]     =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          =
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]            =
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 =
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = (PDRIVER_DISPATCH) CdFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                = CdShutdown;

    DriverObject->FastIoDispatch = &CdFastIoDispatch;

    Status = IoRegisterShutdownNotification (CdfsFileSystemDeviceObject);
    if (!NT_SUCCESS (Status)) {
        IoDeleteDevice (CdfsFileSystemDeviceObject);
        return Status;
    }

    //
    //  Initialize the global data structures
    //

    Status = CdInitializeGlobalData( DriverObject, CdfsFileSystemDeviceObject );
    if (!NT_SUCCESS (Status)) {
        IoDeleteDevice (CdfsFileSystemDeviceObject);
        return Status;
    }

    //
    //  Register the file system as low priority with the I/O system.  This will cause
    //  CDFS to receive mount requests after a) other filesystems currently registered
    //  and b) other normal priority filesystems that may be registered later.
    //

    CdfsFileSystemDeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;

    IoRegisterFileSystem( CdfsFileSystemDeviceObject );
    ObReferenceObject (CdfsFileSystemDeviceObject);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}

NTSTATUS
CdShutdown (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the shutdown handler for CDFS.

Arguments:

    DeviceObject - Supplies the registered device object for CDFS.
    Irp - Shutdown IRP
    

Return Value:

    None.

--*/
{
    IoUnregisterFileSystem (DeviceObject);
    IoDeleteDevice (CdData.FileSystemDeviceObject);

    CdCompleteRequest( NULL, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


VOID
CdUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine unload routine for CDFS.

Arguments:

    DriverObject - Supplies the driver object for CDFS.

Return Value:

    None.

--*/
{
    PIRP_CONTEXT IrpContext;

    //
    // Free any IRP contexts
    //
    while (1) {
        IrpContext = (PIRP_CONTEXT) PopEntryList( &CdData.IrpContextList) ;
        if (IrpContext == NULL) {
            break;
        }
        CdFreePool(&IrpContext);
    }

    IoFreeWorkItem (CdData.CloseItem);
    ExDeleteResourceLite( &CdData.DataResource );
    ObDereferenceObject (CdData.FileSystemDeviceObject);
}

//
//  Local support routine
//

NTSTATUS
CdInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FileSystemDeviceObject
    )

/*++

Routine Description:

    This routine initializes the global cdfs data structures.

Arguments:

    DriverObject - Supplies the driver object for CDFS.

    FileSystemDeviceObject - Supplies the device object for CDFS.

Return Value:

    None.

--*/

{
    //
    //  Start by initializing the FastIoDispatch Table.
    //

    RtlZeroMemory( &CdFastIoDispatch, sizeof( FAST_IO_DISPATCH ));

    CdFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    CdFastIoDispatch.FastIoCheckIfPossible =   CdFastIoCheckIfPossible;  //  CheckForFastIo
    CdFastIoDispatch.FastIoRead =              FsRtlCopyRead;            //  Read
    CdFastIoDispatch.FastIoQueryBasicInfo =    CdFastQueryBasicInfo;     //  QueryBasicInfo
    CdFastIoDispatch.FastIoQueryStandardInfo = CdFastQueryStdInfo;       //  QueryStandardInfo
    CdFastIoDispatch.FastIoLock =              CdFastLock;               //  Lock
    CdFastIoDispatch.FastIoUnlockSingle =      CdFastUnlockSingle;       //  UnlockSingle
    CdFastIoDispatch.FastIoUnlockAll =         CdFastUnlockAll;          //  UnlockAll
    CdFastIoDispatch.FastIoUnlockAllByKey =    CdFastUnlockAllByKey;     //  UnlockAllByKey
    CdFastIoDispatch.AcquireFileForNtCreateSection =  CdAcquireForCreateSection;
    CdFastIoDispatch.ReleaseFileForNtCreateSection =  CdReleaseForCreateSection;
    CdFastIoDispatch.FastIoQueryNetworkOpenInfo =     CdFastQueryNetworkInfo;   //  QueryNetworkInfo
    
    CdFastIoDispatch.MdlRead = FsRtlMdlReadDev;
    CdFastIoDispatch.MdlReadComplete = FsRtlMdlReadCompleteDev;
    CdFastIoDispatch.PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
    CdFastIoDispatch.MdlWriteComplete = FsRtlMdlWriteCompleteDev;

    //
    //  Initialize the CdData structure.
    //

    RtlZeroMemory( &CdData, sizeof( CD_DATA ));

    CdData.NodeTypeCode = CDFS_NTC_DATA_HEADER;
    CdData.NodeByteSize = sizeof( CD_DATA );

    CdData.DriverObject = DriverObject;
    CdData.FileSystemDeviceObject = FileSystemDeviceObject;

    InitializeListHead( &CdData.VcbQueue );

    ExInitializeResourceLite( &CdData.DataResource );

    //
    //  Initialize the cache manager callback routines
    //

    CdData.CacheManagerCallbacks.AcquireForLazyWrite  = &CdAcquireForCache;
    CdData.CacheManagerCallbacks.ReleaseFromLazyWrite = &CdReleaseFromCache;
    CdData.CacheManagerCallbacks.AcquireForReadAhead  = &CdAcquireForCache;
    CdData.CacheManagerCallbacks.ReleaseFromReadAhead = &CdReleaseFromCache;

    CdData.CacheManagerVolumeCallbacks.AcquireForLazyWrite  = &CdNoopAcquire;
    CdData.CacheManagerVolumeCallbacks.ReleaseFromLazyWrite = &CdNoopRelease;
    CdData.CacheManagerVolumeCallbacks.AcquireForReadAhead  = &CdNoopAcquire;
    CdData.CacheManagerVolumeCallbacks.ReleaseFromReadAhead = &CdNoopRelease;

    //
    //  Initialize the lock mutex and the async and delay close queues.
    //

    ExInitializeFastMutex( &CdData.CdDataMutex );
    InitializeListHead( &CdData.AsyncCloseQueue );
    InitializeListHead( &CdData.DelayedCloseQueue );

    CdData.CloseItem = IoAllocateWorkItem (FileSystemDeviceObject);
    if (CdData.CloseItem == NULL) {
        
        ExDeleteResourceLite( &CdData.DataResource );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    //  Do the initialization based on the system size.
    //

    switch (MmQuerySystemSize()) {

    case MmSmallSystem:

        CdData.IrpContextMaxDepth = 4;
        CdData.MaxDelayedCloseCount = 8;
        CdData.MinDelayedCloseCount = 2;
        break;

    case MmMediumSystem:

        CdData.IrpContextMaxDepth = 8;
        CdData.MaxDelayedCloseCount = 24;
        CdData.MinDelayedCloseCount = 6;
        break;

    case MmLargeSystem:

        CdData.IrpContextMaxDepth = 32;
        CdData.MaxDelayedCloseCount = 72;
        CdData.MinDelayedCloseCount = 18;
        break;
    }
    return STATUS_SUCCESS;
}

