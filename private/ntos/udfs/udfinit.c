/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    UdfInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Udfs

Author:

    Dan Lovinger    [DanLo]   24-May-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_UDFINIT)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_UDFINIT)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
UdfInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *FileSystemDeviceObjects
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, UdfInitializeGlobalData)
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

    This is the initialization routine for the UDF file system
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
    PDEVICE_OBJECT UdfsFileSystemDeviceObjects[NUMBER_OF_FS_OBJECTS];
    PDEVICE_OBJECT UdfsDiskFileSystemDeviceObject;

    //
    //  Create the device objects for both device "types".  Since
    //  UDF is a legitimate filesystem for media underlying device
    //  drivers claiming both DVD/CDROMs and disks, we must register
    //  this filesystem twice.
    //

    ASSERT( NUMBER_OF_FS_OBJECTS >= 2 );
    RtlZeroMemory( &UdfsFileSystemDeviceObjects, sizeof(PDEVICE_OBJECT) * NUMBER_OF_FS_OBJECTS );
    
    RtlInitUnicodeString( &UnicodeString, L"\\UdfsCdRom" );

    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                             0,
                             FALSE,
                             &UdfsFileSystemDeviceObjects[0] );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }
    
    RtlInitUnicodeString( &UnicodeString, L"\\UdfsDisk" );

    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &UdfsFileSystemDeviceObjects[1] );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject( UdfsFileSystemDeviceObjects[0] );
        return Status;
    }

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
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = (PDRIVER_DISPATCH) UdfFsdDispatch;

    DriverObject->FastIoDispatch = &UdfFastIoDispatch;

    //
    //  Initialize the global data structures
    //

    UdfInitializeGlobalData( DriverObject, UdfsFileSystemDeviceObjects );

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem( UdfsFileSystemDeviceObjects[0] );
    IoRegisterFileSystem( UdfsFileSystemDeviceObjects[1] );

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


//
//  Local support routine
//

VOID
UdfInitializeGlobalData (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *UdfsFileSystemDeviceObjects
    )

/*++

Routine Description:

    This routine initializes the global Udfs data structures.

Arguments:

    DriverObject - Supplies the driver object for UDFS.

    FileSystemDeviceObjects - Supplies a vector of device objects for UDFS.

Return Value:

    None.

--*/

{
    USHORT CcbMaxDepth;
    USHORT FcbDataMaxDepth;
    USHORT FcbIndexMaxDepth;
    USHORT FcbNonPagedMaxDepth;
    USHORT IrpContextMaxDepth;
    USHORT LcbMaxDepth;

    //
    //  Start by initializing the FastIoDispatch Table.
    //

    RtlZeroMemory( &UdfFastIoDispatch, sizeof( FAST_IO_DISPATCH ));

    UdfFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);

    UdfFastIoDispatch.AcquireFileForNtCreateSection =   UdfAcquireForCreateSection;
    UdfFastIoDispatch.ReleaseFileForNtCreateSection =   UdfReleaseForCreateSection;
    UdfFastIoDispatch.FastIoCheckIfPossible =           UdfFastIoCheckIfPossible;   //  CheckForFastIo
    UdfFastIoDispatch.FastIoRead =                      FsRtlCopyRead;              //  Read
    
    UdfFastIoDispatch.FastIoQueryBasicInfo =            NULL;                       //  QueryBasicInfo
    UdfFastIoDispatch.FastIoQueryStandardInfo =         NULL;                       //  QueryStandardInfo
    UdfFastIoDispatch.FastIoLock =                      NULL;                       //  Lock
    UdfFastIoDispatch.FastIoUnlockSingle =              NULL;                       //  UnlockSingle
    UdfFastIoDispatch.FastIoUnlockAll =                 NULL;                       //  UnlockAll
    UdfFastIoDispatch.FastIoUnlockAllByKey =            NULL;                       //  UnlockAllByKey
    UdfFastIoDispatch.FastIoQueryNetworkOpenInfo =      NULL;                       //  QueryNetworkInfo
    
    //
    //  Initialize the CRC table. Per UDF 1.01, we use the seed 10041 octal (4129 dec).
    //

    UdfInitializeCrc16( 4129 );

    //
    //  Initialize the UdfData structure.
    //

    RtlZeroMemory( &UdfData, sizeof( UDF_DATA ));

    UdfData.NodeTypeCode = UDFS_NTC_DATA_HEADER;
    UdfData.NodeByteSize = sizeof( UDF_DATA );

    UdfData.DriverObject = DriverObject;
    RtlCopyMemory( &UdfData.FileSystemDeviceObjects,
                   UdfsFileSystemDeviceObjects,
                   sizeof(PDEVICE_OBJECT) * NUMBER_OF_FS_OBJECTS );

    InitializeListHead( &UdfData.VcbQueue );

    ExInitializeResource( &UdfData.DataResource );

    //
    //  Initialize the cache manager callback routines
    //

    UdfData.CacheManagerCallbacks.AcquireForLazyWrite  = &UdfAcquireForCache;
    UdfData.CacheManagerCallbacks.ReleaseFromLazyWrite = &UdfReleaseFromCache;
    UdfData.CacheManagerCallbacks.AcquireForReadAhead  = &UdfAcquireForCache;
    UdfData.CacheManagerCallbacks.ReleaseFromReadAhead = &UdfReleaseFromCache;

    UdfData.CacheManagerVolumeCallbacks.AcquireForLazyWrite  = &UdfNoopAcquire;
    UdfData.CacheManagerVolumeCallbacks.ReleaseFromLazyWrite = &UdfNoopRelease;
    UdfData.CacheManagerVolumeCallbacks.AcquireForReadAhead  = &UdfNoopAcquire;
    UdfData.CacheManagerVolumeCallbacks.ReleaseFromReadAhead = &UdfNoopRelease;

    //
    //  Initialize the lock mutex and the async and delay close queues.
    //

    ExInitializeFastMutex( &UdfData.UdfDataMutex );
    InitializeListHead( &UdfData.AsyncCloseQueue );
    InitializeListHead( &UdfData.DelayedCloseQueue );

    ExInitializeWorkItem( &UdfData.CloseItem,
                          (PWORKER_THREAD_ROUTINE) UdfFspClose,
                          NULL );

    //
    //  Do the initialization based on the system size.
    //

    switch (MmQuerySystemSize()) {

    case MmSmallSystem:
        
        IrpContextMaxDepth = 4;
        UdfData.MaxDelayedCloseCount = 10;
        UdfData.MinDelayedCloseCount = 2;
        break;

    case MmLargeSystem:

        IrpContextMaxDepth = 24;
        UdfData.MaxDelayedCloseCount = 72;
        UdfData.MinDelayedCloseCount = 18;
        break;

    default:
    case MmMediumSystem:
    
        IrpContextMaxDepth = 8;
        UdfData.MaxDelayedCloseCount = 32;
        UdfData.MinDelayedCloseCount = 8;
        break;
    
    }

    //
    //  Size lookasides to match what will commonly be dumped into them when we
    //  run down the delayed close queues.
    //
    
    LcbMaxDepth =
    CcbMaxDepth =
    FcbDataMaxDepth =
    FcbNonPagedMaxDepth = (USHORT) (UdfData.MaxDelayedCloseCount - UdfData.MinDelayedCloseCount);

    //
    //  We should tend to have fewer indices than files.
    //
    
    FcbIndexMaxDepth = FcbNonPagedMaxDepth / 2;

#define NPagedInit(L,S,T,D) { ExInitializeNPagedLookasideList( (L), NULL, NULL, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }
#define PagedInit(L,S,T,D)  { ExInitializePagedLookasideList(  (L), NULL, NULL, POOL_RAISE_IF_ALLOCATION_FAILURE, S, T, D); }

    NPagedInit( &UdfIrpContextLookasideList, sizeof( IRP_CONTEXT ), TAG_IRP_CONTEXT, IrpContextMaxDepth );
    NPagedInit( &UdfFcbNonPagedLookasideList, sizeof( FCB_NONPAGED ), TAG_FCB_NONPAGED, FcbNonPagedMaxDepth );

    PagedInit( &UdfCcbLookasideList, sizeof( CCB ), TAG_CCB, CcbMaxDepth );
    PagedInit( &UdfFcbIndexLookasideList, SIZEOF_FCB_INDEX, TAG_FCB_INDEX, FcbIndexMaxDepth );
    PagedInit( &UdfFcbDataLookasideList, SIZEOF_FCB_DATA, TAG_FCB_DATA, FcbDataMaxDepth );
    PagedInit( &UdfLcbLookasideList, SIZEOF_LOOKASIDE_LCB, TAG_LCB, LcbMaxDepth );
}
