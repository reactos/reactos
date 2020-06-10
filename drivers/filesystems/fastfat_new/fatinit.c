/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FatInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Fat


--*/

#include "fatprocs.h"

#ifdef __REACTOS__
#define _Unreferenced_parameter_
#endif

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

_Function_class_(DRIVER_UNLOAD)
VOID
NTAPI
FatUnload(
    _In_ _Unreferenced_parameter_ PDRIVER_OBJECT DriverObject
    );

NTSTATUS
FatGetCompatibilityModeValue(
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    );

BOOLEAN
FatIsFujitsuFMR (
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, FatGetCompatibilityModeValue)
#pragma alloc_text(INIT, FatIsFujitsuFMR)
//#pragma alloc_text(PAGE, FatUnload)
#endif

#define COMPATIBILITY_MODE_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define COMPATIBILITY_MODE_VALUE_NAME L"Win31FileSystem"
#define CODE_PAGE_INVARIANCE_VALUE_NAME L"FatDisableCodePageInvariance"


#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 64)

#define REGISTRY_HARDWARE_DESCRIPTION_W \
        L"\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"

#define REGISTRY_MACHINE_IDENTIFIER_W   L"Identifier"

#define FUJITSU_FMR_NAME_W  L"FUJITSU FMR-"



NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Fat file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    USHORT MaxDepth;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    FS_FILTER_CALLBACKS FilterCallbacks;
    UNICODE_STRING ValueName;
    ULONG Value;

    UNREFERENCED_PARAMETER( RegistryPath );

    //
    // Create the device object for disks.  To avoid problems with filters who
    // know this name, we must keep it.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Fat" );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &FatDiskFileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    // Create the device object for "cdroms".
    //

    RtlInitUnicodeString( &UnicodeString, L"\\FatCdrom" );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                             0,
                             FALSE,
                             &FatCdromFileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        IoDeleteDevice( FatDiskFileSystemDeviceObject);
        return Status;
    }

#ifdef _MSC_VER
#pragma prefast( push )
#pragma prefast( disable:28155, "these are all correct" )
#pragma prefast( disable:28169, "these are all correct" )
#pragma prefast( disable:28175, "this is a filesystem, touching FastIoDispatch is allowed" )
#endif

    DriverObject->DriverUnload = FatUnload;

    //
    //  Note that because of the way data caching is done, we set neither
    //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not in the cache, or the request is not buffered, we may,
    //  set up for Direct I/O by hand.
    //

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)FatFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)FatFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)FatFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)FatFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)FatFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)FatFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)FatFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)FatFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)FatFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)FatFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)FatFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)FatFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)FatFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)FatFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)FatFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)FatFsdDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)FatFsdShutdown;
    DriverObject->MajorFunction[IRP_MJ_PNP]                      = (PDRIVER_DISPATCH)FatFsdPnp;

    DriverObject->FastIoDispatch = &FatFastIoDispatch;

    RtlZeroMemory(&FatFastIoDispatch, sizeof(FatFastIoDispatch));

    FatFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    FatFastIoDispatch.FastIoCheckIfPossible =   FatFastIoCheckIfPossible;  //  CheckForFastIo
    FatFastIoDispatch.FastIoRead =              FsRtlCopyRead;             //  Read
    FatFastIoDispatch.FastIoWrite =             FsRtlCopyWrite;            //  Write
    FatFastIoDispatch.FastIoQueryBasicInfo =    FatFastQueryBasicInfo;     //  QueryBasicInfo
    FatFastIoDispatch.FastIoQueryStandardInfo = FatFastQueryStdInfo;       //  QueryStandardInfo
    FatFastIoDispatch.FastIoLock =              FatFastLock;               //  Lock
    FatFastIoDispatch.FastIoUnlockSingle =      FatFastUnlockSingle;       //  UnlockSingle
    FatFastIoDispatch.FastIoUnlockAll =         FatFastUnlockAll;          //  UnlockAll
    FatFastIoDispatch.FastIoUnlockAllByKey =    FatFastUnlockAllByKey;     //  UnlockAllByKey
    FatFastIoDispatch.FastIoQueryNetworkOpenInfo = FatFastQueryNetworkOpenInfo;
    FatFastIoDispatch.AcquireForCcFlush =       FatAcquireForCcFlush;
    FatFastIoDispatch.ReleaseForCcFlush =       FatReleaseForCcFlush;
    FatFastIoDispatch.MdlRead =                 FsRtlMdlReadDev;
    FatFastIoDispatch.MdlReadComplete =         FsRtlMdlReadCompleteDev;
    FatFastIoDispatch.PrepareMdlWrite =         FsRtlPrepareMdlWriteDev;
    FatFastIoDispatch.MdlWriteComplete =        FsRtlMdlWriteCompleteDev;

#ifdef _MSC_VER
#pragma prefast( pop )
#endif
    
    //
    //  Initialize the filter callbacks we use.
    //

    RtlZeroMemory( &FilterCallbacks,
                   sizeof(FS_FILTER_CALLBACKS) );

    FilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
    FilterCallbacks.PreAcquireForSectionSynchronization = FatFilterCallbackAcquireForCreateSection;

    Status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject,
                                                     &FilterCallbacks );

    if (!NT_SUCCESS( Status )) {

        IoDeleteDevice( FatDiskFileSystemDeviceObject );
        IoDeleteDevice( FatCdromFileSystemDeviceObject );
        return Status;
    }

    //
    //  Initialize the global data structures
    //

    //
    //  The FatData record
    //

    RtlZeroMemory( &FatData, sizeof(FAT_DATA));

    FatData.NodeTypeCode = FAT_NTC_DATA_HEADER;
    FatData.NodeByteSize = sizeof(FAT_DATA);

    InitializeListHead(&FatData.VcbQueue);

    FatData.DriverObject = DriverObject;
    FatData.DiskFileSystemDeviceObject = FatDiskFileSystemDeviceObject;
    FatData.CdromFileSystemDeviceObject = FatCdromFileSystemDeviceObject;

    //
    //  This list head keeps track of closes yet to be done.
    //

    InitializeListHead( &FatData.AsyncCloseList );
    InitializeListHead( &FatData.DelayedCloseList );
    
    FatData.FatCloseItem = IoAllocateWorkItem( FatDiskFileSystemDeviceObject);

    if (FatData.FatCloseItem == NULL) {
        IoDeleteDevice (FatDiskFileSystemDeviceObject);
        IoDeleteDevice (FatCdromFileSystemDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Allocate the zero page
    //

    FatData.ZeroPage = ExAllocatePoolWithTag( NonPagedPoolNx, PAGE_SIZE, 'ZtaF' );
    if (FatData.ZeroPage == NULL) {
        IoDeleteDevice (FatDiskFileSystemDeviceObject);
        IoDeleteDevice (FatCdromFileSystemDeviceObject);        
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory( FatData.ZeroPage, PAGE_SIZE );


    //
    //  Now initialize our general purpose spinlock (gag) and figure out how
    //  deep and wide we want our delayed lists (along with fooling ourselves
    //  about the lookaside depths).
    //

    KeInitializeSpinLock( &FatData.GeneralSpinLock );

    switch ( MmQuerySystemSize() ) {

    case MmSmallSystem:

        MaxDepth = 4;
        FatMaxDelayedCloseCount = FAT_MAX_DELAYED_CLOSES;
        break;

    case MmMediumSystem:

        MaxDepth = 8;
        FatMaxDelayedCloseCount = 4 * FAT_MAX_DELAYED_CLOSES;
        break;

    case MmLargeSystem:
    default:
        
        MaxDepth = 16;
        FatMaxDelayedCloseCount = 16 * FAT_MAX_DELAYED_CLOSES;
        break;
    }


    //
    //  Initialize the cache manager callback routines
    //

    FatData.CacheManagerCallbacks.AcquireForLazyWrite  = &FatAcquireFcbForLazyWrite;
    FatData.CacheManagerCallbacks.ReleaseFromLazyWrite = &FatReleaseFcbFromLazyWrite;
    FatData.CacheManagerCallbacks.AcquireForReadAhead  = &FatAcquireFcbForReadAhead;
    FatData.CacheManagerCallbacks.ReleaseFromReadAhead = &FatReleaseFcbFromReadAhead;

    FatData.CacheManagerNoOpCallbacks.AcquireForLazyWrite  = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = &FatNoOpRelease;
    FatData.CacheManagerNoOpCallbacks.AcquireForReadAhead  = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromReadAhead = &FatNoOpRelease;

    //
    //  Set up global pointer to our process.
    //

    FatData.OurProcess = PsGetCurrentProcess();

    // 
    //  Setup the number of processors we support for statistics as the current number 
    //  running.
    //

#if (NTDDI_VERSION >= NTDDI_VISTA)
    FatData.NumberProcessors = KeQueryActiveProcessorCount( NULL );
#else
    FatData.NumberProcessors = KeNumberProcessors;
#endif


    //
    //  Read the registry to determine if we are in ChicagoMode.
    //

    ValueName.Buffer = COMPATIBILITY_MODE_VALUE_NAME;
    ValueName.Length = sizeof(COMPATIBILITY_MODE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(COMPATIBILITY_MODE_VALUE_NAME);

    Status = FatGetCompatibilityModeValue( &ValueName, &Value );

    if (NT_SUCCESS(Status) && FlagOn(Value, 1)) {

        FatData.ChicagoMode = FALSE;

    } else {

        FatData.ChicagoMode = TRUE;
    }

    //
    //  Read the registry to determine if we are going to generate LFNs
    //  for valid 8.3 names with extended characters.
    //

    ValueName.Buffer = CODE_PAGE_INVARIANCE_VALUE_NAME;
    ValueName.Length = sizeof(CODE_PAGE_INVARIANCE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(CODE_PAGE_INVARIANCE_VALUE_NAME);

    Status = FatGetCompatibilityModeValue( &ValueName, &Value );

    if (NT_SUCCESS(Status) && FlagOn(Value, 1)) {

        FatData.CodePageInvariant = FALSE;

    } else {

        FatData.CodePageInvariant = TRUE;
    }

    //
    //  Initialize our global resource and fire up the lookaside lists.
    //

    ExInitializeResourceLite( &FatData.Resource );

    ExInitializeNPagedLookasideList( &FatIrpContextLookasideList,
                                     NULL,
                                     NULL,
                                     POOL_NX_ALLOCATION | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                     sizeof(IRP_CONTEXT),
                                     TAG_IRP_CONTEXT,
                                     MaxDepth );

    ExInitializeNPagedLookasideList( &FatNonPagedFcbLookasideList,
                                     NULL,
                                     NULL,
                                     POOL_NX_ALLOCATION | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                     sizeof(NON_PAGED_FCB),
                                     TAG_FCB_NONPAGED,
                                     MaxDepth );

    ExInitializeNPagedLookasideList( &FatEResourceLookasideList,
                                     NULL,
                                     NULL,
                                     POOL_NX_ALLOCATION | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                     sizeof(ERESOURCE),
                                     TAG_ERESOURCE,
                                     MaxDepth );

    ExInitializeSListHead( &FatCloseContextSList );
    ExInitializeFastMutex( &FatCloseQueueMutex );
    KeInitializeEvent( &FatReserveEvent, SynchronizationEvent, TRUE );

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem(FatDiskFileSystemDeviceObject);
    ObReferenceObject (FatDiskFileSystemDeviceObject);
    IoRegisterFileSystem(FatCdromFileSystemDeviceObject);
    ObReferenceObject (FatCdromFileSystemDeviceObject);

    //
    //  Find out if we are running an a FujitsuFMR machine.
    //

    FatData.FujitsuFMR = FatIsFujitsuFMR();

#if (NTDDI_VERSION >= NTDDI_WIN8)

    //
    //  Find out global disk accounting state, cache the result
    //

    FatDiskAccountingEnabled = PsIsDiskCountersEnabled();

#endif

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


_Function_class_(DRIVER_UNLOAD)
VOID
NTAPI
FatUnload(
    _In_ _Unreferenced_parameter_ PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the unload routine for the filesystem

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( DriverObject );


    ExDeleteNPagedLookasideList (&FatEResourceLookasideList);
    ExDeleteNPagedLookasideList (&FatNonPagedFcbLookasideList);
    ExDeleteNPagedLookasideList (&FatIrpContextLookasideList);
    ExDeleteResourceLite( &FatData.Resource );
    IoFreeWorkItem (FatData.FatCloseItem);
    ObDereferenceObject( FatDiskFileSystemDeviceObject);
    ObDereferenceObject( FatCdromFileSystemDeviceObject);
}


//
//  Local Support routine
//

NTSTATUS
FatGetCompatibilityModeValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the Chicago compatibilitymode information and get the
    value.

Arguments:

    ValueName - the unicode name for the registry value located in the registry.
    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    KeyName.Buffer = COMPATIBILITY_MODE_KEY_NAME;
    KeyName.Length = sizeof(COMPATIBILITY_MODE_KEY_NAME) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(COMPATIBILITY_MODE_KEY_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    while (1) {

        Status = ZwQueryValueKey(Handle,
                                 ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        NT_ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

                ExFreePool(KeyValueInformation);
            }

            RequestLength += 256;

            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        ' taF');

            if (!KeyValueInformation) {

                ZwClose(Handle);
                return STATUS_NO_MEMORY;
            }

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PULONG DataPtr;

            //
            // Return contents to the caller.
            //

            DataPtr = (PULONG)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
            *Value = *DataPtr;

        } else {

            //
            // Treat as if no value was found
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

        ExFreePool(KeyValueInformation);
    }

    return Status;
}

//
//  Local Support routine
//

BOOLEAN
FatIsFujitsuFMR (
    )

/*++

Routine Description:

    This routine tells us if we are running on a FujitsuFMR machine.

Arguments:


Return Value:

    BOOLEAN - TRUE if we are and FALSE otherwise

--*/

{
    BOOLEAN Result;
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    //
    // Set default as PC/AT
    //

    KeyName.Buffer = REGISTRY_HARDWARE_DESCRIPTION_W;
    KeyName.Length = sizeof(REGISTRY_HARDWARE_DESCRIPTION_W) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(REGISTRY_HARDWARE_DESCRIPTION_W);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return FALSE;
    }

    ValueName.Buffer = REGISTRY_MACHINE_IDENTIFIER_W;
    ValueName.Length = sizeof(REGISTRY_MACHINE_IDENTIFIER_W) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(REGISTRY_MACHINE_IDENTIFIER_W);

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    while (1) {

        Status = ZwQueryValueKey(Handle,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        // NT_ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

                ExFreePool(KeyValueInformation);
            }

            RequestLength += 256;

            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool, RequestLength, ' taF');

            if (!KeyValueInformation) {

                ZwClose(Handle);
                return FALSE;
            }

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status) &&
        (KeyValueInformation->DataLength >= sizeof(FUJITSU_FMR_NAME_W)) &&
        (RtlCompareMemory((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset,
                          FUJITSU_FMR_NAME_W,
                          sizeof(FUJITSU_FMR_NAME_W) - sizeof(WCHAR)) ==
         sizeof(FUJITSU_FMR_NAME_W) - sizeof(WCHAR))) {

        Result = TRUE;

    } else {

        Result = FALSE;
    }

    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

        ExFreePool(KeyValueInformation);
    }

    return Result;
}

