/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fs_rec.c

Abstract:

    This module contains the main functions for the mini-file system recognizer
    driver.

Author:

    Darryl E. Havens (darrylh) 22-nov-1993

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"

//
//  The local debug trace level
//

#define Dbg                              (FSREC_DEBUG_LEVEL_FSREC)

#if DBG

#include <stdarg.h>
#include <stdio.h>

LONG FsRecDebugTraceLevel = 0;
LONG FsRecDebugTraceIndent = 0;

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,FsRecCreateAndRegisterDO)

#pragma alloc_text(PAGE,FsRecCleanupClose)
#pragma alloc_text(PAGE,FsRecCreate)
#pragma alloc_text(PAGE,FsRecFsControl)
#pragma alloc_text(PAGE,FsRecGetDeviceSectorSize)
#pragma alloc_text(PAGE,FsRecGetDeviceSectors)
#pragma alloc_text(PAGE,FsRecLoadFileSystem)
#pragma alloc_text(PAGE,FsRecReadBlock)
#pragma alloc_text(PAGE,FsRecUnload)
#endif // ALLOC_PRAGMA

//
//  Mutex for serializing driver loads.
//

PKEVENT FsRecLoadSync;


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is invoked once when the driver is loaded to allow the driver
    to initialize itself.  The initialization for the driver consists of simply
    creating a device object for each type of file system recognized by this
    driver, and then registering each as active file systems.

Arguments:

    DriverObject - Pointer to the driver object for this driver.

    RegistryPath - Pointer to the registry service node for this driver.

Return Value:

    The function value is the final status of the initialization for the driver.

--*/

{
    PDEVICE_OBJECT UdfsMainRecognizerDeviceObject;
    NTSTATUS status;
    ULONG count = 0;

    PAGED_CODE();

    //
    // Mark the entire driver as pagable.
    //

    MmPageEntireDriver ((PVOID)DriverEntry);

    //
    // Begin by initializing the driver object so that it the driver is
    // prepared to provide services.
    //

    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FsRecFsControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsRecCreate;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FsRecCleanupClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsRecCleanupClose;
    DriverObject->DriverUnload = FsRecUnload;

    FsRecLoadSync = ExAllocatePoolWithTag( NonPagedPool, sizeof(KEVENT), FSREC_POOL_TAG );

    if (FsRecLoadSync == NULL) {

	return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent( FsRecLoadSync, SynchronizationEvent, TRUE );

    //
    // Create and initialize each of the file system driver type device
    // objects.
    //

    status = FsRecCreateAndRegisterDO( DriverObject,
                                       NULL,
                                       NULL,
                                       L"\\Cdfs",
                                       L"\\FileSystem\\CdfsRecognizer",
                                       CdfsFileSystem,
                                       FILE_DEVICE_CD_ROM_FILE_SYSTEM );
    if (NT_SUCCESS( status )) {
        count++;
    }

    status = FsRecCreateAndRegisterDO( DriverObject,
                                       NULL,
                                       &UdfsMainRecognizerDeviceObject,
                                       L"\\UdfsCdRom",
                                       L"\\FileSystem\\UdfsCdRomRecognizer",
                                       UdfsFileSystem,
                                       FILE_DEVICE_CD_ROM_FILE_SYSTEM );
    if (NT_SUCCESS( status )) {
        count++;
    }

    status = FsRecCreateAndRegisterDO( DriverObject,
                                       UdfsMainRecognizerDeviceObject,
                                       NULL,
                                       L"\\UdfsDisk",
                                       L"\\FileSystem\\UdfsDiskRecognizer",
                                       UdfsFileSystem,
                                       FILE_DEVICE_DISK_FILE_SYSTEM );
    if (NT_SUCCESS( status )) {
        count++;
    }

    status = FsRecCreateAndRegisterDO( DriverObject,
                                       NULL,
                                       NULL,
                                       L"\\Fat",
                                       L"\\FileSystem\\FatRecognizer",
                                       FatFileSystem,
                                       FILE_DEVICE_DISK_FILE_SYSTEM );
    if (NT_SUCCESS( status )) {
        count++;
    }

    status = FsRecCreateAndRegisterDO( DriverObject,
                                       NULL,
                                       NULL,
                                       L"\\Ntfs",
                                       L"\\FileSystem\\NtfsRecognizer",
                                       NtfsFileSystem,
                                       FILE_DEVICE_DISK_FILE_SYSTEM );
    if (NT_SUCCESS( status )) {
        count++;
    }

    if (count) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_IMAGE_ALREADY_LOADED;
    }
}


NTSTATUS
FsRecCleanupClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked when someone attempts to cleanup or close one of
    the system recognizer's registered device objects.

Arguments:

    DeviceObject - Pointer to the device object being closed.

    Irp - Pointer to the cleanup/close IRP.

Return Value:

    The final function value is STATUS_SUCCESS.

--*/

{
    PAGED_CODE();

    //
    // Simply complete the request successfully (note that IoStatus.Status in
    // Irp is already initialized to STATUS_SUCCESS).
    //

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}


NTSTATUS
FsRecCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked when someone attempts to open one of the file
    system recognizer's registered device objects.

Arguments:

    DeviceObject - Pointer to the device object being opened.

    Irp - Pointer to the create IRP.

Return Value:

    The final function value indicates whether or not the open was successful.

--*/

{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS status;

    PAGED_CODE();

    //
    // Simply ensure that the name of the "file" being opened is NULL, and
    // complete the request accordingly.
    //

    if (irpSp->FileObject->FileName.Length) {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
    } else {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}


NTSTATUS
FsRecCreateAndRegisterDO (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT HeadRecognizer OPTIONAL,
    OUT PDEVICE_OBJECT *NewRecognizer OPTIONAL,
    IN PWCHAR RecFileSystem,
    IN PWCHAR FileSystemName,
    IN FILE_SYSTEM_TYPE FileSystemType,
    IN DEVICE_TYPE DeviceType
    )

/*++

Routine Description:

    This routine creates a device object for the specified file system type and
    registers it as an active file system.

Arguments:

    DriverObject - Pointer to the driver object for this driver.
    
    HeadRecognizer - Optionally supplies a pre-existing recognizer that the
        newly created DO should be jointly serialized and unregistered with.
        This is useful if a given filesystem exists on multiple device types
        and thus requires multiple recognizers.
    
    NewDeviceObject - Receives the created DO on success..

    RecFileSystem - Name of the file system to be recognized.

    FileSystemName - Name of file system device object to be registered.

    FileSystemType - Type of this file system recognizer device object.
    
    DeviceType - Type of media this file system recognizer device object will inspect.

Return Value:

    The final function value indicates whether or not the device object was
    successfully created and registered.

--*/

{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fsHandle;
    IO_STATUS_BLOCK ioStatus;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    if (NewRecognizer) {

        *NewRecognizer = NULL;
    }

    //
    // Begin by attempting to open the file system driver's device object.  If
    // it works, then the file system is already loaded, so don't load this
    // driver.  Otherwise, this mini-driver is the one that should be loaded.
    //

    RtlInitUnicodeString( &nameString, RecFileSystem );
    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwCreateFile( &fsHandle,
                           SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatus,
                           (PLARGE_INTEGER) NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           0,
                           (PVOID) NULL,
                           0 );

    if (NT_SUCCESS( status )) {
        ZwClose( fsHandle );
    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS( status )) {
        return STATUS_IMAGE_ALREADY_LOADED;
    }

    //
    // Attempt to create a device object for this driver.  This device object
    // will be used to represent the driver as an active file system in the
    // system.
    //

    RtlInitUnicodeString( &nameString, FileSystemName );

    status = IoCreateDevice( DriverObject,
                             sizeof( DEVICE_EXTENSION ),
                             &nameString,
                             DeviceType,
                             0,
                             FALSE,
                             &deviceObject );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Initialize the device extension for this device object.
    //

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
    deviceExtension->FileSystemType = FileSystemType;
    deviceExtension->State = Active;

    //
    //  Is this a filesystem being jointly recognized by recognizers for
    //  different device types?
    //
    
    if (HeadRecognizer) {

        //
        //  Link into the list.
        //
        
        deviceExtension->CoRecognizer = ((PDEVICE_EXTENSION)HeadRecognizer->DeviceExtension)->CoRecognizer;
        ((PDEVICE_EXTENSION)HeadRecognizer->DeviceExtension)->CoRecognizer = deviceObject;
    
    } else {

        //
        //  Initialize the list of codependant recognizer objects.
        //
        
        deviceExtension->CoRecognizer = deviceObject;
    }
    
#if _PNP_POWER_
    deviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

    //
    // Finally, register this driver as an active, loaded file system and
    // return to the caller.
    //

    if (NewRecognizer) {

        *NewRecognizer = deviceObject;
    }

    IoRegisterFileSystem( deviceObject );
    return STATUS_SUCCESS;
}


NTSTATUS
FsRecFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function performs the mount and driver reload functions for this mini-
    file system recognizer driver.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet (IRP) representing the function to
        be performed.

Return Value:

    The function value is the final status of the operation.


--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Simply vector to the appropriate FS control function given the type
    // of file system being interrogated.
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Handle the inactive recognizer states directly.
    //
    
    if (deviceExtension->State != Active && irpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {
        
        if (deviceExtension->State == Transparent) {

            status = STATUS_UNRECOGNIZED_VOLUME;
        
        } else {
        
            status = STATUS_FS_DRIVER_REQUIRED;
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    switch ( deviceExtension->FileSystemType ) {

        case FatFileSystem:

            status = FatRecFsControl( DeviceObject, Irp );
            break;

        case NtfsFileSystem:

            status = NtfsRecFsControl( DeviceObject, Irp );
            break;

        case CdfsFileSystem:

            status = CdfsRecFsControl( DeviceObject, Irp );
            break;

        case UdfsFileSystem:

            status = UdfsRecFsControl( DeviceObject, Irp );
            break;

        default:

            status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return status;
}


VOID
FsRecUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine cleans up the driver's data structures so that it can be
    unloaded.

Arguments:

    DriverObject - Pointer to the driver object for this driver.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Simply delete all of the device objects that this driver has created, the
    // load event, and return.
    //

    while (DriverObject->DeviceObject) {
        IoDeleteDevice( DriverObject->DeviceObject );
    }

    ExFreePool( FsRecLoadSync );

    return;
}


NTSTATUS
FsRecLoadFileSystem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR DriverServiceName
    )

/*++


Routine Description:

    This routine performs the common work of loading a filesystem on behalf
    of one of our recognizers.

Arguments:

    DeviceObject - Pointer to the device object for the recognizer.

    DriverServiceName - Specifies the name of the node in the registry
        associated with the driver to be loaded.

Return Value:

    NTSTATUS.  The recognizer will be set into a transparent mode on return.
    
--*/

{
    UNICODE_STRING driverName;
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_IMAGE_ALREADY_LOADED;

    PAGED_CODE();

    //
    //  Quickly check if the recognizer has already fired.
    //
    
    if (deviceExtension->State != Transparent) {
    
        //
        //  Serialize all threads trying to load this filesystem.
        //
        //  We need to do this for several reasons.  With the new behavior in
        //  IoRegisterFileSystem, we do not know ahead of time whether the
        //  filesystem has been loaded ahead or behind this recognizer in the
        //  scan queue.  This means that we cannot make this recognizer transparent
        //  before the real filesystem has become registered, or else if the
        //  filesystem loads behind us we may let threads go through that will
        //  not find it in that window of time.
        //
        //  The reason this is possible is that NtLoadDriver does not guarantee
        //  that if it returns STATUS_IMAGE_ALREADY_LOADED, that the driver in
        //  question has actually initialized itself, which *is* guaranteed if
        //  it returns STATUS_SUCCESS.  We have to keep these threads bottled
        //  up until they can rescan with the promise that what they need is there.
        //
        //  As a bonus, we can now guarantee that the recognizer goes away in
        //  all cases, not just when the driver successfully loads itself.
        //
        
        KeWaitForSingleObject( FsRecLoadSync,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
        KeEnterCriticalRegion();
    
        //
        //  Attempt the filesystem load precisely once for all recognizers
        //  of a given filesystem.
        //
        
        if (deviceExtension->State == Active) {

            //
            //  For bonus points, in the future we may want to log an event
            //  on failure.
            //

            RtlInitUnicodeString( &driverName, DriverServiceName );
            status = ZwLoadDriver( &driverName );

            //
            //  Now walk all codependant recognizers and instruct them to go
            //  into the fast unload state.  Since IO only expects the fsDO
            //  it is asking to load a filesystem to to unregister itself, if
            //  we unregistered all of the co-recognizers they would dangle.
            //  Unfortunately, this means that fsrec may wind up hanging around
            //  quite a bit longer than strictly neccesary.
            //
            //  Note: we come right back to the original DeviceObject at the
            //  end of this loop (important).  It is also very important that
            //  we only did this once since after we release the mutex the co-
            //  recognizers may begin going away in any order.
            //

            while (deviceExtension->State != FastUnload) {

                deviceExtension->State = FastUnload;

                DeviceObject = deviceExtension->CoRecognizer;
                deviceExtension = DeviceObject->DeviceExtension;
            } 
        }
        
        //
        //  Unregister this recognizer precisely once.
        //

        if (deviceExtension->State != Transparent) {
            
            IoUnregisterFileSystem( DeviceObject );
            deviceExtension->State = Transparent;
        }
        
        KeSetEvent( FsRecLoadSync, 0, FALSE );
        KeLeaveCriticalRegion();
    }
    
    return status;
}


BOOLEAN
FsRecGetDeviceSectors (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BytesPerSector,
    OUT PLARGE_INTEGER NumberOfSectors
    )

/*++

Routine Description:

    This routine returns information about the partition represented by the
    device object.

Arguments:

    DeviceObject - Pointer to the device object from which to read.

    BytesPerSector - The number of bytes per sector for the device being read.

    NumberOfSectors - Variable to receive the number of sectors for this
        partition.

Return Value:

    The function value is TRUE if the information was found, otherwise FALSE.

--*/

{
    PARTITION_INFORMATION partitionInfo;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;
    ULONG remainder;

    PAGED_CODE();

    //
    //  We only do this for disks right now. This is likely to change when we
    //  have to recognize CDUDF media.
    //

    if (DeviceObject->DeviceType != FILE_DEVICE_DISK) {

        return FALSE;
    }

    //
    // Get the number of sectors on this partition.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO,
                                         DeviceObject,
                                         (PVOID) NULL,
                                         0,
                                         &partitionInfo,
                                         sizeof( partitionInfo ),
                                         FALSE,
                                         &event,
                                         &ioStatus );
    if (!irp) {
        return FALSE;
    }

    //
    //  Override verify logic - we don't care. The fact we're in the picture means
    //  someone is trying to mount new/changed media in the first place.
    //
    
    SetFlag( IoGetNextIrpStackLocation( irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    status = IoCallDriver( DeviceObject, irp );
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    *NumberOfSectors = RtlExtendedLargeIntegerDivide( partitionInfo.PartitionLength,
                                                      BytesPerSector,
                                                      &remainder );

    return TRUE;
}


BOOLEAN
FsRecGetDeviceSectorSize (
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG BytesPerSector
    )

/*++

Routine Description:

    This routine returns the sector size of the underlying device.

Arguments:

    DeviceObject - Pointer to the device object from which to read.

    BytesPerSector - Variable to receive the number of bytes per sector for the
        device being read.

Return Value:

    The function value is TRUE if the information was found, otherwise FALSE.

--*/

{
    DISK_GEOMETRY diskGeometry;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;
    ULONG ControlCode;

    PAGED_CODE();

    //
    //  Figure out what kind of device we have so we can use the right IOCTL.
    //

    switch (DeviceObject->DeviceType) {
        case FILE_DEVICE_CD_ROM:
            ControlCode = IOCTL_CDROM_GET_DRIVE_GEOMETRY;
            break;

        case FILE_DEVICE_DISK:
            ControlCode = IOCTL_DISK_GET_DRIVE_GEOMETRY;
            break;

        default:
            return FALSE;
    }

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );
    irp = IoBuildDeviceIoControlRequest( ControlCode,
                                         DeviceObject,
                                         (PVOID) NULL,
                                         0,
                                         &diskGeometry,
                                         sizeof( diskGeometry ),
                                         FALSE,
                                         &event,
                                         &ioStatus );

    if (!irp) {
        return FALSE;
    }

    //
    //  Override verify logic - we don't care. The fact we're in the picture means
    //  someone is trying to mount new/changed media in the first place.
    //
    
    SetFlag( IoGetNextIrpStackLocation( irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    
    status = IoCallDriver( DeviceObject, irp );
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Ensure that the drive actually knows how many bytes there are per
    // sector.  Floppy drives do not know if the media is unformatted.
    //

    if (!diskGeometry.BytesPerSector) {
        return FALSE;
    }

    //
    // Store the return values for the caller.
    //

    *BytesPerSector = diskGeometry.BytesPerSector;

    return TRUE;
}


BOOLEAN
FsRecReadBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG MinimumBytes,
    IN ULONG BytesPerSector,
    OUT PVOID *Buffer,
    OUT PBOOLEAN IsDeviceFailure OPTIONAL
    )

/*++

Routine Description:

    This routine reads a minimum numbers of bytes into a buffer starting at
    the byte offset from the base of the device represented by the device
    object.

Arguments:

    DeviceObject - Pointer to the device object from which to read.

    ByteOffset - Pointer to a 64-bit byte offset from the base of the device
        from which to start the read.

    MinimumBytes - Supplies the minimum number of bytes to be read.

    BytesPerSector - The number of bytes per sector for the device being read.

    Buffer - Variable to receive a pointer to the allocated buffer containing
        the bytes read.
        
    IsDeviceFailure - Variable to receive an indication whether a failure
        was a result of talking to the device.

Return Value:

    The function value is TRUE if the bytes were read, otherwise FALSE.

--*/

{
    #define RoundUp( x, y ) ( ((x + (y-1)) / y) * y )

    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE();

    if (IsDeviceFailure) {
        *IsDeviceFailure = FALSE;
    }
    
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Set the minimum number of bytes to read to the maximum of the bytes that
    // the caller wants to read, and the number of bytes in a sector.
    //

    if (MinimumBytes < BytesPerSector) {
        MinimumBytes = BytesPerSector;
    } else {
        MinimumBytes = RoundUp( MinimumBytes, BytesPerSector );
    }

    //
    // Allocate a buffer large enough to contain the bytes required, round the
    // request to a page boundary to solve any alignment requirements.
    //

    if (!*Buffer) {

        *Buffer = ExAllocatePoolWithTag( NonPagedPool,
					 (MinimumBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
					 FSREC_POOL_TAG );
        if (!*Buffer) {
            return FALSE;
        }
    }

    //
    // Read the actual bytes off of the media.
    //

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        *Buffer,
                                        MinimumBytes,
                                        ByteOffset,
                                        &event,
                                        &ioStatus );
    if (!irp) {
        return FALSE;
    }
    
    //
    //  Override verify logic - we don't care. The fact we're in the picture means
    //  someone is trying to mount new/changed media in the first place.
    //
    
    SetFlag( IoGetNextIrpStackLocation( irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    status = IoCallDriver( DeviceObject, irp );
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {

        if (IsDeviceFailure) {
            *IsDeviceFailure = TRUE;
        }
        return FALSE;
    }

    return TRUE;
}


#if DBG
BOOLEAN
FsRecDebugTrace (
    LONG IndentIncrement,
    ULONG TraceMask,
    PCHAR Format,
    ...
    )

/*++

Routine Description:

    This routine is a simple debug info printer that returns a constant boolean value.  This
    makes it possible to splice it into the middle of boolean expressions to discover which
    elements are firing.
    
    We will use this as our general debug printer.  See udfdata.h for how we use the DebugTrace
    macro to accomplish the effect.
    
Arguments:

    IndentIncrement - amount to change the indentation by.
    
    TraceMask - specification of what debug trace level this call should be noisy at.

Return Value:

    USHORT - The 16bit CRC

--*/

{
    va_list Arglist;
    LONG i;
    UCHAR Buffer[128];
    int Bytes;

#define Min(a, b)   ((a) < (b) ? (a) : (b))
    
    if (TraceMask == 0 || (FsRecDebugTraceLevel & TraceMask) != 0) {

        //
        //  Emit a preamble of our thread ID.
        //
        
        DbgPrint( "%p:", PsGetCurrentThread());

        if (IndentIncrement < 0) {
            
            FsRecDebugTraceIndent += IndentIncrement;
        }

        if (FsRecDebugTraceIndent < 0) {
            
            FsRecDebugTraceIndent = 0;
        }

        //
        //  Build the indent in big chunks since calling DbgPrint repeatedly is expensive.
        //
        
        for (i = FsRecDebugTraceIndent; i > 0; i -= (sizeof(Buffer) - 1)) {

            RtlFillMemory( Buffer, Min( i, (sizeof(Buffer) - 1 )), ' ');
            *(Buffer + Min( i, (sizeof(Buffer) - 1 ))) = '\0';
            
            DbgPrint( Buffer );
        }

        //
        // Format the output into a buffer and then print it.
        //

        va_start( Arglist, Format );
        Bytes = _vsnprintf( Buffer, sizeof(Buffer), Format, Arglist );
        va_end( Arglist );

        //
        // detect buffer overflow
        //

        if (Bytes == -1) {

            Buffer[sizeof(Buffer) - 1] = '\n';
        }

        DbgPrint( Buffer );

        if (IndentIncrement > 0) {

            FsRecDebugTraceIndent += IndentIncrement;
        }
    }

    return TRUE;
}
#endif

