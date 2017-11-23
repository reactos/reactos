/*++


Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Fat called
    by the dispatch driver.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
FatMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDEVICE_OBJECT FsDeviceObject
    );

NTSTATUS
FatVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
FatIsMediaWriteProtected (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

NTSTATUS
FatUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatInvalidateVolumes (
    IN PIRP Irp
    );

VOID
FatScanForDismountedVcb (
    IN PIRP_CONTEXT IrpContext
    );

BOOLEAN
FatPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LBO Lbo,
    IN ULONG NumberOfBytesToRead,
    IN BOOLEAN ReturnOnError
    );

NTSTATUS
FatQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatQueryBpb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatGetStatistics (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatAllowExtendedDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  Local support routine prototypes
//

NTSTATUS
FatGetVolumeBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatGetRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatMoveFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
FatComputeMoveFileSplicePoints (
    PIRP_CONTEXT IrpContext,
    PFCB FcbOrDcb,
    ULONG FileOffset,
    ULONG TargetCluster,
    ULONG BytesToReallocate,
    PULONG FirstSpliceSourceCluster,
    PULONG FirstSpliceTargetCluster,
    PULONG SecondSpliceSourceCluster,
    PULONG SecondSpliceTargetCluster,
    PLARGE_MCB SourceMcb
);

VOID
FatComputeMoveFileParameter (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN ULONG BufferSize,
    IN ULONG FileOffset,
    IN OUT PULONG ByteCount,
    OUT PULONG BytesToReallocate,
    OUT PULONG BytesToWrite,
    OUT PLARGE_INTEGER SourceLbo
);

NTSTATUS
FatSearchBufferForLabel(
    IN  PIRP_CONTEXT IrpContext,
    IN  PVPB  Vpb,
    IN  PVOID Buffer,
    IN  ULONG Size,
    OUT PBOOLEAN LabelFound
);

VOID
FatVerifyLookupFatEntry (
    IN  PIRP_CONTEXT IrpContext,
    IN  PVCB Vcb,
    IN  ULONG FatIndex,
    IN OUT PULONG FatEntry
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatAddMcbEntry)
#pragma alloc_text(PAGE, FatAllowExtendedDasdIo)
#pragma alloc_text(PAGE, FatCommonFileSystemControl)
#pragma alloc_text(PAGE, FatComputeMoveFileParameter)
#pragma alloc_text(PAGE, FatComputeMoveFileSplicePoints)
#pragma alloc_text(PAGE, FatDirtyVolume)
#pragma alloc_text(PAGE, FatDismountVolume)
#pragma alloc_text(PAGE, FatFsdFileSystemControl)
#pragma alloc_text(PAGE, FatGetRetrievalPointers)
#pragma alloc_text(PAGE, FatGetStatistics)
#pragma alloc_text(PAGE, FatGetVolumeBitmap)
#pragma alloc_text(PAGE, FatIsMediaWriteProtected)
#pragma alloc_text(PAGE, FatIsPathnameValid)
#pragma alloc_text(PAGE, FatIsVolumeDirty)
#pragma alloc_text(PAGE, FatIsVolumeMounted)
#pragma alloc_text(PAGE, FatLockVolume)
#pragma alloc_text(PAGE, FatLookupLastMcbEntry)
#pragma alloc_text(PAGE, FatMountVolume)
#pragma alloc_text(PAGE, FatMoveFile)
#pragma alloc_text(PAGE, FatOplockRequest)
#pragma alloc_text(PAGE, FatPerformVerifyDiskRead)
#pragma alloc_text(PAGE, FatQueryBpb)
#pragma alloc_text(PAGE, FatQueryRetrievalPointers)
#pragma alloc_text(PAGE, FatRemoveMcbEntry)
#pragma alloc_text(PAGE, FatScanForDismountedVcb)
#pragma alloc_text(PAGE, FatSearchBufferForLabel)
#pragma alloc_text(PAGE, FatUnlockVolume)
#pragma alloc_text(PAGE, FatUserFsCtrl)
#pragma alloc_text(PAGE, FatVerifyLookupFatEntry)
#pragma alloc_text(PAGE, FatVerifyVolume)
#endif

#if DBG

BOOLEAN FatMoveFileDebug = 0;

#endif

//
//  These wrappers go around the MCB package; we scale the LBO's passed
//  in (which can be bigger than 32 bits on fat32) by the volume's sector
//  size.
//
//  Note we now use the real large mcb package.  This means these shims
//  now also convert the -1 unused LBN number to the 0 of the original
//  mcb package.
//

#define     MCB_SCALE_LOG2      (Vcb->AllocationSupport.LogOfBytesPerSector)
#define     MCB_SCALE           (1 << MCB_SCALE_LOG2)
#define     MCB_SCALE_MODULO    (MCB_SCALE - 1)


BOOLEAN
FatAddMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    IN LBO Lbo,
    IN ULONG SectorCount
    )

{
    PAGED_CODE();

    if (SectorCount) {

        //
        //  Round up sectors, but be careful as SectorCount approaches 4Gb.
        //  Note that for x>0, (x+m-1)/m = ((x-1)/m)+(m/m) = ((x-1)/m)+1
        //

        SectorCount--;
        SectorCount >>= MCB_SCALE_LOG2;
        SectorCount++;
    }

    Vbo >>= MCB_SCALE_LOG2;
    Lbo >>= MCB_SCALE_LOG2;

    return FsRtlAddLargeMcbEntry( Mcb,
                                  ((LONGLONG) Vbo),
                                  ((LONGLONG) Lbo),
                                  ((LONGLONG) SectorCount) );
}


BOOLEAN
FatLookupMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    )
{
    BOOLEAN Results;
    LONGLONG LiLbo;
    LONGLONG LiSectorCount;
    ULONG Remainder;

    LiLbo = 0;
    LiSectorCount = 0;

    Remainder = Vbo & MCB_SCALE_MODULO;

    Results = FsRtlLookupLargeMcbEntry( Mcb,
                                        (Vbo >> MCB_SCALE_LOG2),
                                        &LiLbo,
                                        ARGUMENT_PRESENT(SectorCount) ? &LiSectorCount : NULL,
                                        NULL,
                                        NULL,
                                        Index );

    if ((ULONG) LiLbo != -1) {

        *Lbo = (((LBO) LiLbo) << MCB_SCALE_LOG2);

        if (Results) {

            *Lbo += Remainder;
        }

    } else {

        *Lbo = 0;
    }

    if (ARGUMENT_PRESENT(SectorCount)) {

        *SectorCount = (ULONG) LiSectorCount;

        if (*SectorCount) {

            *SectorCount <<= MCB_SCALE_LOG2;

            if (*SectorCount == 0) {

                *SectorCount = (ULONG) -1;
            }

            if (Results) {

                *SectorCount -= Remainder;
            }
        }

    }

    return Results;
}

//
//  NOTE: Vbo/Lbn undefined if MCB is empty & return code false.
//

BOOLEAN
FatLookupLastMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    OUT PVBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG Index
    )

{
    BOOLEAN Results;
    LONGLONG LiVbo;
    LONGLONG LiLbo;
    ULONG LocalIndex;

    PAGED_CODE();

    LiVbo = LiLbo = 0;
    LocalIndex = 0;

    Results = FsRtlLookupLastLargeMcbEntryAndIndex( Mcb,
                                                    &LiVbo,
                                                    &LiLbo,
                                                    &LocalIndex );

    *Vbo = ((VBO) LiVbo) << MCB_SCALE_LOG2;

    if (((ULONG) LiLbo) != -1) {

        *Lbo = ((LBO) LiLbo) << MCB_SCALE_LOG2;

        *Lbo += (MCB_SCALE - 1);
        *Vbo += (MCB_SCALE - 1);

    } else {

        *Lbo = 0;
    }

    if (Index) {
        *Index = LocalIndex;
    }

    return Results;
}


BOOLEAN
FatGetNextMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN ULONG RunIndex,
    OUT PVBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG SectorCount
    )

{
    BOOLEAN Results;
    LONGLONG LiVbo;
    LONGLONG LiLbo;
    LONGLONG LiSectorCount;

    PAGED_CODE();

    LiVbo = LiLbo = 0;

    Results = FsRtlGetNextLargeMcbEntry( Mcb,
                                         RunIndex,
                                         &LiVbo,
                                         &LiLbo,
                                         &LiSectorCount );

    if (Results) {

        *Vbo = ((VBO) LiVbo) << MCB_SCALE_LOG2;

        if (((ULONG) LiLbo) != -1) {

            *Lbo = ((LBO) LiLbo) << MCB_SCALE_LOG2;

        } else {

            *Lbo = 0;
        }

        *SectorCount = ((ULONG) LiSectorCount) << MCB_SCALE_LOG2;

        if ((*SectorCount == 0) && (LiSectorCount != 0)) {
            *SectorCount = (ULONG) -1;  /* it overflowed */
        }
    }

    return Results;
}


VOID
FatRemoveMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    IN ULONG SectorCount
    )
{

    if ((SectorCount) && (SectorCount != 0xFFFFFFFF)) {

        SectorCount--;
        SectorCount >>= MCB_SCALE_LOG2;
        SectorCount++;
    }

    Vbo >>= MCB_SCALE_LOG2;

#if DBG
    _SEH2_TRY {
#endif

        FsRtlRemoveLargeMcbEntry( Mcb,
                                  (LONGLONG) Vbo,
                                  (LONGLONG) SectorCount);

#if DBG
    } _SEH2_EXCEPT(FatBugCheckExceptionFilter( _SEH2_GetExceptionInformation() )) {

          NOTHING;
    } _SEH2_END;
#endif

}


NTSTATUS
NTAPI
FatFsdFileSystemControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of FileSystem control operations

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    BOOLEAN Wait;
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    DebugTrace(+1, Dbg,"FatFsdFileSystemControl\n", 0);

    //
    //  Call the common FileSystem Control routine, with blocking allowed if
    //  synchronous.  This opeation needs to special case the mount
    //  and verify suboperations because we know they are allowed to block.
    //  We identify these suboperations by looking at the file object field
    //  and seeing if its null.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL) {

        Wait = TRUE;

    } else {

        Wait = CanFsdWait( Irp );
    }

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    _SEH2_TRY {

        PIO_STACK_LOCATION IrpSp;

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  We need to made a special check here for the InvalidateVolumes
        //  FSCTL as that comes in with a FileSystem device object instead
        //  of a volume device object.
        //

        if (FatDeviceIsFatFsdo( IrpSp->DeviceObject) &&
            (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
            (IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
            (IrpSp->Parameters.FileSystemControl.FsControlCode ==
             FSCTL_INVALIDATE_VOLUMES)) {

            Status = FatInvalidateVolumes( Irp );

        } else {

            IrpContext = FatCreateIrpContext( Irp, Wait );

            Status = FatCommonFileSystemControl( IrpContext, Irp );
        }

    } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
FatCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg,"FatCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg,"Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg,"MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = FatUserFsCtrl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = FatMountVolume( IrpContext,
                                 IrpSp->Parameters.MountVolume.DeviceObject,
                                 IrpSp->Parameters.MountVolume.Vpb,
                                 IrpSp->DeviceObject );

        //
        //  Complete the request.
        //
        //  We do this here because FatMountVolume can be called recursively,
        //  but the Irp is only to be completed once.
        //
        //  NOTE: I don't think this is true anymore (danlo 3/15/1999).  Probably
        //  an artifact of the old doublespace attempt.
        //

        FatCompleteRequest( IrpContext, Irp, Status );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = FatVerifyVolume( IrpContext, Irp );
        break;

    default:

        DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "FatCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDEVICE_OBJECT FsDeviceObject
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is a Fat volume,
    and create the VCB and root DCB structures.  The algorithm it uses is
    essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do cached
       volume file I/O.

    2. Read the disk and check if it is a Fat volume.

    3. If it is not a Fat volume then free the cached volume file, delete
       the VCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves reinitializing the cached volume
       file, checking the dirty bit, resetting up the allocation support,
       deleting the VCB, hooking in the old VCB, and completing the IRP.

    5. Otherwise create a root DCB, create Fsp threads as necessary, and
       complete the IRP.

Arguments:

    TargetDeviceObject - This is where we send all of our requests.

    Vpb - This gives us additional information needed to complete the mount.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );
    NTSTATUS Status;

    PBCB BootBcb;
    PPACKED_BOOT_SECTOR BootSector;

    PBCB DirentBcb;
    PDIRENT Dirent;
    ULONG ByteOffset;

    BOOLEAN MountNewVolume = FALSE;
    BOOLEAN WeClearedVerifyRequiredBit = FALSE;
    BOOLEAN DoARemount = FALSE;

    PVCB OldVcb;
    PVPB OldVpb;

    PDEVICE_OBJECT RealDevice;
    PVOLUME_DEVICE_OBJECT VolDo = NULL;
    PVCB Vcb = NULL;
    PFILE_OBJECT RootDirectoryFile = NULL;

    PLIST_ENTRY Links;

    IO_STATUS_BLOCK Iosb;
    ULONG ChangeCount = 0;

    DISK_GEOMETRY Geometry;

    PARTITION_INFORMATION_EX PartitionInformation;
    NTSTATUS StatusPartInfo;

    DebugTrace(+1, Dbg, "FatMountVolume\n", 0);
    DebugTrace( 0, Dbg, "TargetDeviceObject = %08lx\n", TargetDeviceObject);
    DebugTrace( 0, Dbg, "Vpb                = %08lx\n", Vpb);

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );
    ASSERT( FatDeviceIsFatFsdo( FsDeviceObject));

    //
    //  Verify that there is a disk here and pick up the change count.
    //

    Status = FatPerformDevIoCtrl( IrpContext,
                                  IOCTL_DISK_CHECK_VERIFY,
                                  TargetDeviceObject,
                                  &ChangeCount,
                                  sizeof(ULONG),
                                  FALSE,
                                  TRUE,
                                  &Iosb );

    if (!NT_SUCCESS( Status )) {

        //
        //  If we will allow a raw mount then avoid sending the popup.
        //
        //  Only send this on "true" disk devices to handle the accidental
        //  legacy of FAT. No other FS will throw a harderror on empty
        //  drives.
        //
        //  Cmd should really handle this per 9x.
        //

        if (!FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT ) &&
            Vpb->RealDevice->DeviceType == FILE_DEVICE_DISK) {

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        return Status;
    }

    if (Iosb.Information != sizeof(ULONG)) {

        //
        //  Be safe about the count in case the driver didn't fill it in
        //

        ChangeCount = 0;
    }

    //
    //  If this is a CD class device,  then check to see if there is a 
    //  'data track' or not.  This is to avoid issuing paging reads which will
    //  fail later in the mount process (e.g. CD-DA or blank CD media)
    //

    if ((TargetDeviceObject->DeviceType == FILE_DEVICE_CD_ROM) &&
        !FatScanForDataTrack( IrpContext, TargetDeviceObject))  {

        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Ping the volume with a partition query and pick up the partition
    //  type.  We'll check this later to avoid some scurrilous volumes.
    //

    StatusPartInfo = FatPerformDevIoCtrl( IrpContext,
                                          IOCTL_DISK_GET_PARTITION_INFO_EX,
                                          TargetDeviceObject,
                                          &PartitionInformation,
                                          sizeof(PARTITION_INFORMATION_EX),
                                          FALSE,
                                          TRUE,
                                          &Iosb );

    //
    //  Make sure we can wait.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    // 
    // Do a quick check to see if there any Vcb's which can be removed.
    //
    
    FatScanForDismountedVcb( IrpContext );

    //
    //  Initialize the Bcbs and our final state so that the termination
    //  handlers will know what to free or unpin
    //

    BootBcb = NULL;
    DirentBcb = NULL;

    Vcb = NULL;
    VolDo = NULL;
    MountNewVolume = FALSE;

    _SEH2_TRY {

        //
        //  Synchronize with FatCheckForDismount(), which modifies the vpb.
        //

        (VOID)FatAcquireExclusiveGlobal( IrpContext );

        //
        //  Create a new volume device object.  This will have the Vcb
        //  hanging off of its end, and set its alignment requirement
        //  from the device we talk to.
        //

        if (!NT_SUCCESS(Status = IoCreateDevice( FatData.DriverObject,
                                                 sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                                                 NULL,
                                                 FILE_DEVICE_DISK_FILE_SYSTEM,
                                                 0,
                                                 FALSE,
                                                 (PDEVICE_OBJECT *)&VolDo))) {

            try_return( Status );
        }

#ifdef _PNP_POWER_
        //
        // This driver doesn't talk directly to a device, and (at the moment)
        // isn't otherwise concerned about power management.
        //

        VolDo->DeviceObject.DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the TargetDeviceObject
        //

        if (TargetDeviceObject->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = TargetDeviceObject->AlignmentRequirement;
        }

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

        //
        //  We must initialize the stack size in our device object before
        //  the following reads, because the I/O system has not done it yet.
        //  This must be done before we clear the device initializing flag
        //  otherwise a filter could attach and copy the wrong stack size into
        //  it's device object.
        //

        VolDo->DeviceObject.StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);

        //
        //  We must also set the sector size correctly in our device object 
        //  before clearing the device initializing flag.
        //
        
        Status = FatPerformDevIoCtrl( IrpContext,
                                      IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                      TargetDeviceObject,
                                      &Geometry,
                                      sizeof( DISK_GEOMETRY ),
                                      FALSE,
                                      TRUE,
                                      NULL );

        VolDo->DeviceObject.SectorSize = (USHORT)Geometry.BytesPerSector;

        //
        //  Indicate that this device object is now completely initialized
        //

        ClearFlag(VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING);

        //
        //  Now Before we can initialize the Vcb we need to set up the device
        //  object field in the Vpb to point to our new volume device object.
        //  This is needed when we create the virtual volume file's file object
        //  in initialize vcb.
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        //
        //  If the real device needs verification, temporarily clear the
        //  field.
        //

        RealDevice = Vpb->RealDevice;

        if ( FlagOn(RealDevice->Flags, DO_VERIFY_VOLUME) ) {

            ClearFlag(RealDevice->Flags, DO_VERIFY_VOLUME);

            WeClearedVerifyRequiredBit = TRUE;
        }

        //
        //  Initialize the new vcb
        //

        FatInitializeVcb( IrpContext, 
                          &VolDo->Vcb, 
                          TargetDeviceObject, 
                          Vpb, 
                          FsDeviceObject);
        //
        //  Get a reference to the Vcb hanging off the end of the device object
        //

        Vcb = &VolDo->Vcb;

        //
        //  Read in the boot sector, and have the read be the minumum size
        //  needed.  We know we can wait.
        //

        //
        //  We need to commute errors on CD so that CDFS will get its crack.  Audio
        //  and even data media may not be universally readable on sector zero.        
        //
        
        _SEH2_TRY {
        
            FatReadVolumeFile( IrpContext,
                               Vcb,
                               0,                          // Starting Byte
                               sizeof(PACKED_BOOT_SECTOR),
                               &BootBcb,
                               (PVOID *)&BootSector );
        
        } _SEH2_EXCEPT( Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM ?
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

              NOTHING;
        } _SEH2_END;

        //
        //  Call a routine to check the boot sector to see if it is fat
        //

        if (BootBcb == NULL || !FatIsBootSectorFat( BootSector)) {

            DebugTrace(0, Dbg, "Not a Fat Volume\n", 0);
        
            //
            //  Complete the request and return to our caller
            //

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Unpack the BPB.  We used to do some sanity checking of the FATs at
        //  this point, but authoring errors on third-party devices prevent
        //  us from continuing to safeguard ourselves.  We can only hope the
        //  boot sector check is good enough.
        //
        //  (read: digital cameras)
        //
        //  Win9x does the same.
        //

        FatUnpackBios( &Vcb->Bpb, &BootSector->PackedBpb );

        //
        //  Check if we have an OS/2 Boot Manager partition and treat it as an
        //  unknown file system.  We'll check the partition type in from the
        //  partition table and we ensure that it has less than 0x80 sectors,
        //  which is just a heuristic that will capture all real OS/2 BM partitions
        //  and avoid the chance we'll discover partitions which erroneously
        //  (but to this point, harmlessly) put down the OS/2 BM type.
        //
        //  Note that this is only conceivable on good old MBR media.
        //
        //  The OS/2 Boot Manager boot format mimics a FAT16 partition in sector
        //  zero but does is not a real FAT16 file system.  For example, the boot
        //  sector indicates it has 2 FATs but only really has one, with the boot
        //  manager code overlaying the second FAT.  If we then set clean bits in
        //  FAT[0] we'll corrupt that code.
        //

        if (NT_SUCCESS( StatusPartInfo ) &&
            (PartitionInformation.PartitionStyle == PARTITION_STYLE_MBR &&
             PartitionInformation.Mbr.PartitionType == PARTITION_OS2BOOTMGR) &&
            (Vcb->Bpb.Sectors != 0 &&
             Vcb->Bpb.Sectors < 0x80)) {

            DebugTrace( 0, Dbg, "OS/2 Boot Manager volume detected, volume not mounted. \n", 0 );
            
            //
            //  Complete the request and return to our caller
            //
            
            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Verify that the sector size recorded in the Bpb matches what the
        //  device currently reports it's sector size to be.
        //
        
        if ( !NT_SUCCESS( Status) || 
             (Geometry.BytesPerSector != Vcb->Bpb.BytesPerSector))  {

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  This is a fat volume, so extract the bpb, serial number.  The
        //  label we'll get later after we've created the root dcb.
        //
        //  Note that the way data caching is done, we set neither the
        //  direct I/O or Buffered I/O bit in the device object flags.
        //

        if (Vcb->Bpb.Sectors != 0) { Vcb->Bpb.LargeSectors = 0; }

        if (IsBpbFat32(&BootSector->PackedBpb)) {

            CopyUchar4( &Vpb->SerialNumber, ((PPACKED_BOOT_SECTOR_EX)BootSector)->Id );

        } else  {

            CopyUchar4( &Vpb->SerialNumber, BootSector->Id );

            //
            //  Allocate space for the stashed boot sector chunk.  This only has meaning on
            //  FAT12/16 volumes since this only is kept for the FSCTL_QUERY_FAT_BPB and it and
            //  its users are a bit wierd, thinking that a BPB exists wholly in the first 0x24
            //  bytes.
            //

            Vcb->First0x24BytesOfBootSector =
                FsRtlAllocatePoolWithTag( PagedPool,
                                          0x24,
                                          TAG_STASHED_BPB );

            //
            //  Stash a copy of the first 0x24 bytes
            //

            RtlCopyMemory( Vcb->First0x24BytesOfBootSector,
                           BootSector,
                           0x24 );
        }

        //
        //  Now unpin the boot sector, so when we set up allocation eveything
        //  works.
        //

        FatUnpinBcb( IrpContext, BootBcb );

        //
        //  Compute a number of fields for Vcb.AllocationSupport
        //

        FatSetupAllocationSupport( IrpContext, Vcb );

        //
        //  Sanity check the FsInfo information for FAT32 volumes.  Silently deal
        //  with messed up information by effectively disabling FsInfo updates.
        //

        if (FatIsFat32( Vcb )) {

            if (Vcb->Bpb.FsInfoSector >= Vcb->Bpb.ReservedSectors) {

                Vcb->Bpb.FsInfoSector = 0;
            }
        }

        //
        //  Create a root Dcb so we can read in the volume label.  If this is FAT32, we can
        //  discover corruption in the FAT chain.
        //
        //  NOTE: this exception handler presumes that this is the only spot where we can
        //  discover corruption in the mount process.  If this ever changes, this handler
        //  MUST be expanded.  The reason we have this guy here is because we have to rip
        //  the structures down now (in the finally below) and can't wait for the outer
        //  exception handling to do it for us, at which point everything will have vanished.
        //

        _SEH2_TRY {

            FatCreateRootDcb( IrpContext, Vcb );

        } _SEH2_EXCEPT (_SEH2_GetExceptionCode() == STATUS_FILE_CORRUPT_ERROR ? EXCEPTION_EXECUTE_HANDLER :
                                                                    EXCEPTION_CONTINUE_SEARCH) {

            //
            //  The volume needs to be dirtied, do it now.  Note that at this point we have built
            //  enough of the Vcb to pull this off.
            //

            FatMarkVolume( IrpContext, Vcb, VolumeDirty );

            //
            //  Now keep bailing out ...
            //

            FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        } _SEH2_END;

        FatLocateVolumeLabel( IrpContext,
                              Vcb,
                              &Dirent,
                              &DirentBcb,
#ifndef __REACTOS__
                              &ByteOffset );
#else
                              (PVBO)&ByteOffset );
#endif

        if (Dirent != NULL) {

            OEM_STRING OemString;
            UNICODE_STRING UnicodeString;

            //
            //  Compute the length of the volume name
            //

#ifndef __REACTOS__
            OemString.Buffer = &Dirent->FileName[0];
#else
            OemString.Buffer = (PCHAR)&Dirent->FileName[0];
#endif
            OemString.MaximumLength = 11;

            for ( OemString.Length = 11;
                  OemString.Length > 0;
                  OemString.Length -= 1) {

                if ( (Dirent->FileName[OemString.Length-1] != 0x00) &&
                     (Dirent->FileName[OemString.Length-1] != 0x20) ) { break; }
            }

            UnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
            UnicodeString.Buffer = &Vcb->Vpb->VolumeLabel[0];

            Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                         &OemString,
                                                         FALSE );

            if ( !NT_SUCCESS( Status ) ) {

                try_return( Status );
            }

            Vpb->VolumeLabelLength = UnicodeString.Length;

        } else {

            Vpb->VolumeLabelLength = 0;
        }

        //
        //  Use the change count we noted initially *before* doing any work.
        //  If something came along in the midst of this operation, we'll
        //  verify and discover the problem.
        //

        Vcb->ChangeCount = ChangeCount;

        //
        //  Now scan the list of previously mounted volumes and compare
        //  serial numbers and volume labels off not currently mounted
        //  volumes to see if we have a match.
        //

        for (Links = FatData.VcbQueue.Flink;
             Links != &FatData.VcbQueue;
             Links = Links->Flink) {

            OldVcb = CONTAINING_RECORD( Links, VCB, VcbLinks );
            OldVpb = OldVcb->Vpb;

            //
            //  Skip over ourselves since we're already in the VcbQueue
            //

            if (OldVpb == Vpb) { continue; }

            //
            //  Check for a match:
            //
            //  Serial Number, VolumeLabel and Bpb must all be the same.
            //  Also the volume must have failed a verify before (ie.
            //  VolumeNotMounted), and it must be in the same physical
            //  drive than it was mounted in before.
            //

            if ( (OldVpb->SerialNumber == Vpb->SerialNumber) &&
                 (OldVcb->VcbCondition == VcbNotMounted) &&
                 (OldVpb->RealDevice == RealDevice) &&
                 (OldVpb->VolumeLabelLength == Vpb->VolumeLabelLength) &&
                 (RtlEqualMemory(&OldVpb->VolumeLabel[0],
                                 &Vpb->VolumeLabel[0],
                                 Vpb->VolumeLabelLength)) &&
                 (RtlEqualMemory(&OldVcb->Bpb,
                                 &Vcb->Bpb,
                                 IsBpbFat32(&Vcb->Bpb) ?
                                     sizeof(BIOS_PARAMETER_BLOCK) :
                                     FIELD_OFFSET(BIOS_PARAMETER_BLOCK,
                                                  LargeSectorsPerFat) ))) {

                DoARemount = TRUE;

                break;
            }
        }

        if ( DoARemount ) {

            PVPB *IrpVpb;

            DebugTrace(0, Dbg, "Doing a remount\n", 0);
            DebugTrace(0, Dbg, "Vcb = %08lx\n", Vcb);
            DebugTrace(0, Dbg, "Vpb = %08lx\n", Vpb);
            DebugTrace(0, Dbg, "OldVcb = %08lx\n", OldVcb);
            DebugTrace(0, Dbg, "OldVpb = %08lx\n", OldVpb);

            //
            //  Swap target device objects between the VCBs. That way
            //  the old VCB will start using the new target device object,
            //  and the new VCB will be torn down and deference the old
            //  target device object.
            //

            Vcb->TargetDeviceObject = OldVcb->TargetDeviceObject;
            OldVcb->TargetDeviceObject = TargetDeviceObject;

            //
            //  This is a remount, so link the old vpb in place
            //  of the new vpb.

            ASSERT( !FlagOn( OldVcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED ) );

            OldVpb->RealDevice = Vpb->RealDevice;
            OldVpb->RealDevice->Vpb = OldVpb;
            
            OldVcb->VcbCondition = VcbGood;

            //
            //  Use the new changecount.
            //

            OldVcb->ChangeCount = Vcb->ChangeCount;

            //
            //  If the new VPB is the VPB referenced in the original Irp, set
            //  that reference back to the old VPB.
            //

            IrpVpb = &IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->Parameters.MountVolume.Vpb;

            if (*IrpVpb == Vpb) {

                *IrpVpb = OldVpb;
            }

            //
            //  We do not want to touch this VPB again. It will get cleaned up when
            //  the new VCB is cleaned up.
            // 

            ASSERT( Vcb->Vpb == Vpb );

            Vpb = NULL;

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED );
            FatSetVcbCondition( Vcb, VcbBad );

            //
            //  Reinitialize the volume file cache and allocation support.
            //

            {
                CC_FILE_SIZES FileSizes;

                FileSizes.AllocationSize.QuadPart =
                FileSizes.FileSize.QuadPart = ( 0x40000 + 0x1000 );
                FileSizes.ValidDataLength = FatMaxLarge;

                DebugTrace(0, Dbg, "Truncate and reinitialize the volume file\n", 0);

                CcInitializeCacheMap( OldVcb->VirtualVolumeFile,
                                      &FileSizes,
                                      TRUE,
                                      &FatData.CacheManagerNoOpCallbacks,
                                      Vcb );

                //
                //  Redo the allocation support
                //

                FatSetupAllocationSupport( IrpContext, OldVcb );

                //
                //  Get the state of the dirty bit.
                //

                FatCheckDirtyBit( IrpContext, OldVcb );

                //
                //  Check for write protected media.
                //

                if (FatIsMediaWriteProtected(IrpContext, TargetDeviceObject)) {

                    SetFlag( OldVcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

                } else {

                    ClearFlag( OldVcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
                }
            }

            //
            //  Complete the request and return to our caller
            //

            try_return( Status = STATUS_SUCCESS );
        }

        DebugTrace(0, Dbg, "Mount a new volume\n", 0);

        //
        //  This is a new mount
        //
        //  Create a blank ea data file fcb, just not for Fat32.
        //

        if (!FatIsFat32(Vcb)) {

            DIRENT TempDirent;
            PFCB EaFcb;

            RtlZeroMemory( &TempDirent, sizeof(DIRENT) );
            RtlCopyMemory( &TempDirent.FileName[0], "EA DATA  SF", 11 );

            EaFcb = FatCreateFcb( IrpContext,
                                  Vcb,
                                  Vcb->RootDcb,
                                  0,
                                  0,
                                  &TempDirent,
                                  NULL,
                                  FALSE,
                                  TRUE );

            //
            //  Deny anybody who trys to open the file.
            //

            SetFlag( EaFcb->FcbState, FCB_STATE_SYSTEM_FILE );

            Vcb->EaFcb = EaFcb;
        }

        //
        //  Get the state of the dirty bit.
        //

        FatCheckDirtyBit( IrpContext, Vcb );

        //
        //  Check for write protected media.
        //

        if (FatIsMediaWriteProtected(IrpContext, TargetDeviceObject)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

        } else {

            ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
        }

        //
        //  Lock volume in drive if we just mounted the boot drive.
        //

        if (FlagOn(RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION)) {

            SetFlag(Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE);

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA)) {

                FatToggleMediaEjectDisable( IrpContext, Vcb, TRUE );
            }
        }

        //
        //  Indicate to our termination handler that we have mounted
        //  a new volume.
        //

        MountNewVolume = TRUE;

        //
        //  Complete the request
        //

        Status = STATUS_SUCCESS;

        //
        //  Ref the root dir stream object so we can send mount notification.
        //

        RootDirectoryFile = Vcb->RootDcb->Specific.Dcb.DirectoryFile;
        ObReferenceObject( RootDirectoryFile );

        //
        //  Remove the extra reference to this target DO made on behalf of us
        //  by the IO system.  In the remount case, we permit regular Vcb
        //  deletion to do this work.
        //

        ObDereferenceObject( TargetDeviceObject );


    try_exit: NOTHING;

    } _SEH2_FINALLY {

        DebugUnwind( FatMountVolume );

        FatUnpinBcb( IrpContext, BootBcb );
        FatUnpinBcb( IrpContext, DirentBcb );

        //
        //  Check if a volume was mounted.  If not then we need to
        //  mark the Vpb not mounted again.
        //

        if ( !MountNewVolume ) {

            if ( Vcb != NULL ) {

                //
                // A VCB was created and initialized. we need to try to tear it down.
                //

                FatCheckForDismount( IrpContext,
                                     Vcb,
                                     TRUE );

                IrpContext->Vcb = NULL;
            } else if ( VolDo != NULL ) {
                //
                //  The VCB was never initialized, so we need to delete the
                //  device right here.
                //
       
                IoDeleteDevice( &VolDo->DeviceObject );
            }
      
            //
            //  see if a remount failed.
            //

            if (DoARemount && _SEH2_AbnormalTermination()) {
                
                //
                //  The remount failed. Try to tear down the old VCB as well.
                //
                
                FatCheckForDismount( IrpContext,
                                     OldVcb,
                                     TRUE );
            }
        }

        if ( WeClearedVerifyRequiredBit == TRUE ) {

            SetFlag(RealDevice->Flags, DO_VERIFY_VOLUME);
        }

        FatReleaseGlobal( IrpContext );

        DebugTrace(-1, Dbg, "FatMountVolume -> %08lx\n", Status);
    } _SEH2_END;

    //
    //  Now send mount notification. Note that since this is outside of any
    //  synchronization since the synchronous delivery of this may go to
    //  folks that provoke re-entrance to the FS.
    //

    if (RootDirectoryFile != NULL) {

        FsRtlNotifyVolumeEvent( RootDirectoryFile, FSRTL_VOLUME_MOUNT );
        ObDereferenceObject( RootDirectoryFile );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the verify volume operation by checking the volume
    label and serial number physically on the media with the the Vcb
    currently claiming to have the volume mounted. It is responsible for
    either completing or enqueuing the input Irp.

    Regardless of whether the verify operation succeeds, the following
    operations are performed:

        - Set Vcb->VirtualEaFile back to its virgin state.
        - Purge all cached data (flushing first if verify succeeds)
        - Mark all Fcbs as needing verification

    If the volumes verifies correctly we also must:

        - Check the volume dirty bit.
        - Reinitialize the allocation support
        - Flush any dirty data

    If the volume verify fails, it may never be mounted again.  If it is
    mounted again, it will happen as a remount operation.  In preparation
    for that, and to leave the volume in a state that can be "lazy deleted"
    the following operations are performed:

        - Set the Vcb condition to VcbNotMounted
        - Uninitialize the volume file cachemap
        - Tear down the allocation support

    In the case of an abnormal termination we haven't determined the state
    of the volume, so we set the Device Object as needing verification again.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - If the verify operation completes, it will return either
        STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.  If an IO or
        other error is encountered, that status will be returned.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp;

    PDIRENT RootDirectory = NULL;
    PPACKED_BOOT_SECTOR BootSector = NULL;

    BIOS_PARAMETER_BLOCK Bpb;

    PVOLUME_DEVICE_OBJECT VolDo;
    PVCB Vcb;
    PVPB Vpb;

    ULONG SectorSize;
    BOOLEAN ClearVerify = FALSE;
    BOOLEAN ReleaseEntireVolume = FALSE;
    BOOLEAN VerifyAlreadyDone = FALSE;

    DISK_GEOMETRY DiskGeometry;

    LBO RootDirectoryLbo;
    ULONG RootDirectorySize;
    BOOLEAN LabelFound;

    ULONG ChangeCount = 0;
    IO_STATUS_BLOCK Iosb;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatVerifyVolume\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", IrpSp->Parameters.VerifyVolume.DeviceObject);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", IrpSp->Parameters.VerifyVolume.Vpb);

    //
    //  Save some references to make our life a little easier.  Note the Vcb for the purposes
    //  of exception handling.
    //

    VolDo = (PVOLUME_DEVICE_OBJECT)IrpSp->Parameters.VerifyVolume.DeviceObject;

    Vpb                   = IrpSp->Parameters.VerifyVolume.Vpb;
    IrpContext->Vcb = Vcb = &VolDo->Vcb;

    //
    //  If we cannot wait then enqueue the irp to the fsp and
    //  return the status to our caller.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "Cannot wait for verify.\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatVerifyVolume -> %08lx\n", Status );
        return Status;
    }

    //
    //  We are serialized at this point allowing only one thread to
    //  actually perform the verify operation.  Any others will just
    //  wait and then no-op when checking if the volume still needs
    //  verification.
    //

    (VOID)FatAcquireExclusiveGlobal( IrpContext );
    (VOID)FatAcquireExclusiveVcb( IrpContext, Vcb );

    _SEH2_TRY {

        BOOLEAN AllowRawMount = BooleanFlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT );

        //
        //  Mark ourselves as verifying this volume so that recursive I/Os
        //  will be able to complete.
        //

        ASSERT( Vcb->VerifyThread == NULL );
        Vcb->VerifyThread = KeGetCurrentThread();

        //
        //  Check if the real device still needs to be verified.  If it doesn't
        //  then obviously someone beat us here and already did the work
        //  so complete the verify irp with success.  Otherwise reenable
        //  the real device and get to work.
        //

        if (!FlagOn(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

            DebugTrace(0, Dbg, "RealDevice has already been verified\n", 0);

            VerifyAlreadyDone = TRUE;
            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  Ping the volume with a partition query to make Jeff happy.
        //

        {
            PARTITION_INFORMATION_EX PartitionInformation;

            (VOID) FatPerformDevIoCtrl( IrpContext,
                                        IOCTL_DISK_GET_PARTITION_INFO_EX,
                                        Vcb->TargetDeviceObject,
                                        &PartitionInformation,
                                        sizeof(PARTITION_INFORMATION_EX),
                                        FALSE,
                                        TRUE,
                                        &Iosb );
        }

        //
        //  Verify that there is a disk here and pick up the change count.
        //

        Status = FatPerformDevIoCtrl( IrpContext,
                                      IOCTL_DISK_CHECK_VERIFY,
                                      Vcb->TargetDeviceObject,
                                      &ChangeCount,
                                      sizeof(ULONG),
                                      FALSE,
                                      TRUE,
                                      &Iosb );

        if (!NT_SUCCESS( Status )) {

            //
            //  If we will allow a raw mount then return WRONG_VOLUME to
            //  allow the volume to be mounted by raw.
            //

            if (AllowRawMount) {

                try_return( Status = STATUS_WRONG_VOLUME );
            }

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        if (Iosb.Information != sizeof(ULONG)) {

            //
            //  Be safe about the count in case the driver didn't fill it in
            //

            ChangeCount = 0;
        }

        //
        //  Whatever happens we will have verified this volume at this change
        //  count, so record that fact.
        //

        Vcb->ChangeCount = ChangeCount;

        //
        //  If this is a CD class device,  then check to see if there is a 
        //  'data track' or not.  This is to avoid issuing paging reads which will
        //  fail later in the mount process (e.g. CD-DA or blank CD media)
        //

        if ((Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_CD_ROM) &&
            !FatScanForDataTrack( IrpContext, Vcb->TargetDeviceObject))  {

            try_return( Status = STATUS_WRONG_VOLUME);
        }

        //
        //  Some devices can change sector sizes on the fly.  Obviously, it
        //  isn't the same volume if that happens.
        //

        Status = FatPerformDevIoCtrl( IrpContext,
                                      IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                      Vcb->TargetDeviceObject,
                                      &DiskGeometry,
                                      sizeof( DISK_GEOMETRY ),
                                      FALSE,
                                      TRUE,
                                      NULL );

        if (!NT_SUCCESS( Status )) {

            //
            //  If we will allow a raw mount then return WRONG_VOLUME to
            //  allow the volume to be mounted by raw.
            //

            if (AllowRawMount) {

                try_return( Status = STATUS_WRONG_VOLUME );
            }

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }

        //
        //  Read in the boot sector
        //

        SectorSize = (ULONG)Vcb->Bpb.BytesPerSector;

        if (SectorSize != DiskGeometry.BytesPerSector) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        BootSector = FsRtlAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                              (ULONG) ROUND_TO_PAGES( SectorSize ),
                                              TAG_VERIFY_BOOTSECTOR);

        //
        //  If this verify is on behalf of a DASD open, allow a RAW mount.
        //

        if (!FatPerformVerifyDiskRead( IrpContext,
                                       Vcb,
                                       BootSector,
                                       0,
                                       SectorSize,
                                       AllowRawMount )) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Call a routine to check the boot sector to see if it is fat.
        //  If it is not fat then mark the vcb as not mounted tell our
        //  caller its the wrong volume
        //

        if (!FatIsBootSectorFat( BootSector )) {

            DebugTrace(0, Dbg, "Not a Fat Volume\n", 0);

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  This is a fat volume, so extract serial number and see if it is
        //  ours.
        //

        {
            ULONG SerialNumber;

            if (IsBpbFat32(&BootSector->PackedBpb)) {
                CopyUchar4( &SerialNumber, ((PPACKED_BOOT_SECTOR_EX)BootSector)->Id );
            } else {
                CopyUchar4( &SerialNumber, BootSector->Id );
            }

            if (SerialNumber != Vpb->SerialNumber) {

                DebugTrace(0, Dbg, "Not our serial number\n", 0);

                try_return( Status = STATUS_WRONG_VOLUME );
            }
        }

        //
        //  Make sure the Bpbs are not different.  We have to zero out our
        //  stack version of the Bpb since unpacking leaves holes.
        //

        RtlZeroMemory( &Bpb, sizeof(BIOS_PARAMETER_BLOCK) );

        FatUnpackBios( &Bpb, &BootSector->PackedBpb );
        if (Bpb.Sectors != 0) { Bpb.LargeSectors = 0; }

        if ( !RtlEqualMemory( &Bpb,
                              &Vcb->Bpb,
                              IsBpbFat32(&Bpb) ?
                                    sizeof(BIOS_PARAMETER_BLOCK) :
                                    FIELD_OFFSET(BIOS_PARAMETER_BLOCK,
                                                 LargeSectorsPerFat) )) {

            DebugTrace(0, Dbg, "Bpb is different\n", 0);

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Check the volume label.  We do this by trying to locate the
        //  volume label, making two strings one for the saved volume label
        //  and the other for the new volume label and then we compare the
        //  two labels.
        //

        if (FatRootDirectorySize(&Bpb) > 0) {

            RootDirectorySize = FatRootDirectorySize(&Bpb);
        
        } else {

            RootDirectorySize = FatBytesPerCluster(&Bpb);
        }

        RootDirectory = FsRtlAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                  (ULONG) ROUND_TO_PAGES( RootDirectorySize ),
                                                  TAG_VERIFY_ROOTDIR);

        if (!IsBpbFat32(&BootSector->PackedBpb)) {

            //
            //  The Fat12/16 case is simple -- read the root directory in and
            //  search it.
            //

            RootDirectoryLbo = FatRootDirectoryLbo(&Bpb);

            if (!FatPerformVerifyDiskRead( IrpContext,
                                           Vcb,
                                           RootDirectory,
                                           RootDirectoryLbo,
                                           RootDirectorySize,
                                           AllowRawMount )) {

                try_return( Status = STATUS_WRONG_VOLUME );
            }

            Status = FatSearchBufferForLabel(IrpContext, Vpb,
                                             RootDirectory, RootDirectorySize,
                                             &LabelFound);

            if (!NT_SUCCESS(Status)) {

                try_return( Status );
            }

            if (!LabelFound && Vpb->VolumeLabelLength > 0) {

                try_return( Status = STATUS_WRONG_VOLUME );
            }

        } else {

            ULONG RootDirectoryCluster;

            RootDirectoryCluster = Bpb.RootDirFirstCluster;

            while (RootDirectoryCluster != FAT_CLUSTER_LAST) {

                RootDirectoryLbo = FatGetLboFromIndex(Vcb, RootDirectoryCluster);

                if (!FatPerformVerifyDiskRead( IrpContext,
                                               Vcb,
                                               RootDirectory,
                                               RootDirectoryLbo,
                                               RootDirectorySize,
                                               AllowRawMount )) {

                    try_return( Status = STATUS_WRONG_VOLUME );
                }

                Status = FatSearchBufferForLabel(IrpContext, Vpb,
                                                 RootDirectory, RootDirectorySize,
                                                 &LabelFound);

                if (!NT_SUCCESS(Status)) {

                    try_return( Status );
                }

                if (LabelFound) {

                    //
                    //  Found a matching label.
                    //

                    break;
                }

                //
                //  Set ourselves up for the next loop iteration.
                //

                FatVerifyLookupFatEntry( IrpContext, Vcb,
                                         RootDirectoryCluster,
                                         &RootDirectoryCluster );

                switch (FatInterpretClusterType(Vcb, RootDirectoryCluster)) {

                case FatClusterAvailable:
                case FatClusterReserved:
                case FatClusterBad:

                    //
                    //  Bail all the way out if we have a bad root.
                    //

                    FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                    break;

                default:

                    break;
                }

            }

            if (RootDirectoryCluster == FAT_CLUSTER_LAST &&
                Vpb->VolumeLabelLength > 0) {

                //
                //  Should have found a label, didn't find any.
                //

                try_return( Status = STATUS_WRONG_VOLUME );
            }
        }


    try_exit: NOTHING;

        //
        //  Note that we have previously acquired the Vcb to serialize
        //  the EA file stuff the marking all the Fcbs as NeedToBeVerified.
        //
        //  Put the Ea file back in a virgin state.
        //

        FatCloseEaFile( IrpContext, Vcb, (BOOLEAN)(Status == STATUS_SUCCESS) );

        //
        //  Mark all Fcbs as needing verification, but only if we really have
        //  to do it.
        //

        if (!VerifyAlreadyDone) {

            FatMarkFcbCondition( IrpContext, Vcb->RootDcb, FcbNeedsToBeVerified, TRUE );
        }

        //
        //  If the verify didn't succeed, get the volume ready for a
        //  remount or eventual deletion.
        //

        if (Vcb->VcbCondition == VcbNotMounted) {

            //
            //  If the volume was already in an unmounted state, just bail
            //  and make sure we return STATUS_WRONG_VOLUME.
            //

            Status = STATUS_WRONG_VOLUME;

        } else if ( Status == STATUS_WRONG_VOLUME ) {

            //
            //  Grab everything so we can safely transition the volume state without
            //  having a thread stumble into the torn-down allocation engine.
            //

            FatAcquireExclusiveVolume( IrpContext, Vcb );
            ReleaseEntireVolume = TRUE;

            //
            //  Get rid of any cached data, without flushing
            //

            FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, NoFlush );

            //
            //  Uninitialize the volume file cache map.  Note that we cannot
            //  do a "FatSyncUninit" because of deadlock problems.  However,
            //  since this FileObject is referenced by us, and thus included
            //  in the Vpb residual count, it is OK to do a normal CcUninit.
            //

            CcUninitializeCacheMap( Vcb->VirtualVolumeFile,
                                    &FatLargeZero,
                                    NULL );

            FatTearDownAllocationSupport( IrpContext, Vcb );

            Vcb->VcbCondition = VcbNotMounted;

            ClearVerify = TRUE;

        } else if (!VerifyAlreadyDone) {

            //
            //  Grab everything so we can safely transition the volume state without
            //  having a thread stumble into the torn-down allocation engine.
            //

            FatAcquireExclusiveVolume( IrpContext, Vcb );
            ReleaseEntireVolume = TRUE;

            //
            //  Get rid of any cached data, flushing first.
            //
            //  Future work (and for bonus points, around the other flush points)
            //  could address the possibility that the dirent filesize hasn't been
            //  updated yet, causing us to fail the re-verification of a file in
            //  DetermineAndMark. This is pretty subtle and very very uncommon.
            //

            FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, Flush );

            //
            //  Flush and Purge the volume file.
            //

            (VOID)FatFlushFat( IrpContext, Vcb );
            CcPurgeCacheSection( &Vcb->SectionObjectPointers, NULL, 0, FALSE );

            //
            //  Redo the allocation support with newly paged stuff.
            //

            FatTearDownAllocationSupport( IrpContext, Vcb );
            FatSetupAllocationSupport( IrpContext, Vcb );

            FatCheckDirtyBit( IrpContext, Vcb );

            //
            //  Check for write protected media.
            //

            if (FatIsMediaWriteProtected(IrpContext, Vcb->TargetDeviceObject)) {

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

            } else {

                ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
            }

            ClearVerify = TRUE;
        }

        if (ClearVerify) {

            //
            //  Mark the device as no longer needing verification.
            //

            ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

    } _SEH2_FINALLY {

        DebugUnwind( FatVerifyVolume );

        //
        //  Free any buffer we may have allocated
        //

        if ( BootSector != NULL ) { ExFreePool( BootSector ); }
        if ( RootDirectory != NULL ) { ExFreePool( RootDirectory ); }

        //
        //  Show that we are done with this volume.
        //

        ASSERT( Vcb->VerifyThread == KeGetCurrentThread() );
        Vcb->VerifyThread = NULL;

        if (ReleaseEntireVolume) {

            FatReleaseVolume( IrpContext, Vcb );
        }

        FatReleaseVcb( IrpContext, Vcb );
        FatReleaseGlobal( IrpContext );

        //
        //  If this was not an abnormal termination, complete the irp.
        //

        if (!_SEH2_AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatVerifyVolume -> %08lx\n", Status);
    } _SEH2_END;

    return Status;
}


//
//  Local Support Routine
//

BOOLEAN
FatIsBootSectorFat (
    IN PPACKED_BOOT_SECTOR BootSector
    )

/*++

Routine Description:

    This routine checks if the boot sector is for a fat file volume.

Arguments:

    BootSector - Supplies the packed boot sector to check

Return Value:

    BOOLEAN - TRUE if the volume is Fat and FALSE otherwise.

--*/

{
    BOOLEAN Result;
    BIOS_PARAMETER_BLOCK Bpb;

    DebugTrace(+1, Dbg, "FatIsBootSectorFat, BootSector = %08lx\n", BootSector);

    //
    //  The result is true unless we decide that it should be false
    //

    Result = TRUE;

    //
    //  Unpack the bios and then test everything
    //

    FatUnpackBios( &Bpb, &BootSector->PackedBpb );
    if (Bpb.Sectors != 0) { Bpb.LargeSectors = 0; }

    if ((BootSector->Jump[0] != 0xe9) &&
        (BootSector->Jump[0] != 0xeb) &&
        (BootSector->Jump[0] != 0x49)) {

        Result = FALSE;

    //
    //  Enforce some sanity on the sector size (easy check)
    //

    } else if ((Bpb.BytesPerSector !=  128) &&
               (Bpb.BytesPerSector !=  256) &&
               (Bpb.BytesPerSector !=  512) &&
               (Bpb.BytesPerSector != 1024) &&
               (Bpb.BytesPerSector != 2048) &&
               (Bpb.BytesPerSector != 4096)) {

        Result = FALSE;

    //
    //  Likewise on the clustering.
    //

    } else if ((Bpb.SectorsPerCluster !=  1) &&
               (Bpb.SectorsPerCluster !=  2) &&
               (Bpb.SectorsPerCluster !=  4) &&
               (Bpb.SectorsPerCluster !=  8) &&
               (Bpb.SectorsPerCluster != 16) &&
               (Bpb.SectorsPerCluster != 32) &&
               (Bpb.SectorsPerCluster != 64) &&
               (Bpb.SectorsPerCluster != 128)) {

        Result = FALSE;

    //
    //  Likewise on the reserved sectors (must reflect at least the boot sector!)
    //

    } else if (Bpb.ReservedSectors == 0) {

        Result = FALSE;

    //
    //  No FATs? Wrong ...
    //

    } else if (Bpb.Fats == 0) {

        Result = FALSE;

    //
    // Prior to DOS 3.2 might contains value in both of Sectors and
    // Sectors Large.
    //

    } else if ((Bpb.Sectors == 0) && (Bpb.LargeSectors == 0)) {

        Result = FALSE;

    //
    //  Check that FAT32 (SectorsPerFat == 0) claims some FAT space and
    //  is of a version we recognize, currently Version 0.0.
    //

    } else if (Bpb.SectorsPerFat == 0 && ( Bpb.LargeSectorsPerFat == 0 ||
                                           Bpb.FsVersion != 0 )) {

        Result = FALSE;

    } else if ((Bpb.Media != 0xf0) &&
               (Bpb.Media != 0xf8) &&
               (Bpb.Media != 0xf9) &&
               (Bpb.Media != 0xfb) &&
               (Bpb.Media != 0xfc) &&
               (Bpb.Media != 0xfd) &&
               (Bpb.Media != 0xfe) &&
               (Bpb.Media != 0xff) &&
               (!FatData.FujitsuFMR || ((Bpb.Media != 0x00) &&
                                        (Bpb.Media != 0x01) &&
                                        (Bpb.Media != 0xfa)))) {

        Result = FALSE;

    //
    //  If this isn't FAT32, then there better be a claimed root directory
    //  size here ...
    //

    } else if (Bpb.SectorsPerFat != 0 && Bpb.RootEntries == 0) {

        Result = FALSE;

    //
    //  If this is FAT32 (i.e., extended BPB), look for and refuse to mount
    //  mirror-disabled volumes. If we did, we would need to only write to
    //  the FAT# indicated in the ActiveFat field. The only user of this is
    //  the FAT->FAT32 converter after the first pass of protected mode work
    //  (booting into realmode) and NT should absolutely not be attempting
    //  to mount such an in-transition volume.
    //

    } else if (Bpb.SectorsPerFat == 0 && Bpb.MirrorDisabled) {

        Result = FALSE;
    }

    DebugTrace(-1, Dbg, "FatIsBootSectorFat -> %08lx\n", Result);

    return Result;
}


//
//  Local Support Routine
//

BOOLEAN
FatIsMediaWriteProtected (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine determines if the target media is write protected.

Arguments:

    TargetDeviceObject - The target of the query

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Query the partition table
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  See if the media is write protected.  On success or any kind
    //  of error (possibly illegal device function), assume it is
    //  writeable, and only complain if he tells us he is write protected.
    //

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                         TargetDeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    //
    //  Just return FALSE in the unlikely event we couldn't allocate an Irp.
    //

    if ( Irp == NULL ) {

        return FALSE;
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    Status = IoCallDriver( TargetDeviceObject, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return (BOOLEAN)(Status == STATUS_MEDIA_WRITE_PROTECTED);
}


//
//  Local Support Routine
//

NTSTATUS
FatUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg,"FatUserFsCtrl...\n", 0);
    DebugTrace( 0, Dbg,"FsControlCode = %08lx\n", FsControlCode);

    //
    //  Some of these Fs Controls use METHOD_NEITHER buffering.  If the previous mode
    //  of the caller was userspace and this is a METHOD_NEITHER, we have the choice
    //  of realy buffering the request through so we can possibly post, or making the
    //  request synchronous.  Since the former was not done by design, do the latter.
    //

    if (Irp->RequestorMode != KernelMode && (FsControlCode & 3) == METHOD_NEITHER) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Case on the control code.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:
    case FSCTL_REQUEST_FILTER_OPLOCK :

        Status = FatOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = FatLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = FatUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = FatDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        Status = FatDirtyVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_DIRTY:

        Status = FatIsVolumeDirty( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED:

        Status = FatIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:
        Status = FatIsPathnameValid( IrpContext, Irp );
        break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = FatQueryRetrievalPointers( IrpContext, Irp );
        break;

    case FSCTL_QUERY_FAT_BPB:
        Status = FatQueryBpb( IrpContext, Irp );
        break;

    case FSCTL_FILESYSTEM_GET_STATISTICS:
        Status = FatGetStatistics( IrpContext, Irp );
        break;

    case FSCTL_GET_VOLUME_BITMAP:
        Status = FatGetVolumeBitmap( IrpContext, Irp );
        break;

    case FSCTL_GET_RETRIEVAL_POINTERS:
        Status = FatGetRetrievalPointers( IrpContext, Irp );
        break;

    case FSCTL_MOVE_FILE:
        Status = FatMoveFile( IrpContext, Irp );
        break;

    case FSCTL_ALLOW_EXTENDED_DASD_IO:
        Status = FatAllowExtendedDasdIo( IrpContext, Irp );
        break;

    default :

        DebugTrace(0, Dbg, "Invalid control code -> %08lx\n", FsControlCode );

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "FatUserFsCtrl -> %08lx\n", Status );
    return Status;
}



//
//  Local support routine
//

NTSTATUS
FatOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;
    PFCB Fcb;
    PVCB Vcb;
    PCCB Ccb;

    ULONG OplockCount = 0;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN AcquiredVcb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "FatOplockRequest...\n", 0);
    DebugTrace( 0, Dbg, "FsControlCode = %08lx\n", FsControlCode);

    //
    //  We only permit oplock requests on files.
    //

    if ( FatDecodeFileObject( IrpSp->FileObject,
                              &Vcb,
                              &Fcb,
                              &Ccb ) != UserFileOpen ) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "FatOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Make this a waitable Irpcontext so we don't fail to acquire
    //  the resources.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Use a try finally to free the Fcb/Vcb
    //

    _SEH2_TRY {

        //
        //  Switch on the function control code.  We grab the Fcb exclusively
        //  for oplock requests, shared for oplock break acknowledgement.
        //

        switch ( FsControlCode ) {

        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        case FSCTL_REQUEST_BATCH_OPLOCK:
        case FSCTL_REQUEST_FILTER_OPLOCK :

            FatAcquireSharedVcb( IrpContext, Fcb->Vcb );
            AcquiredVcb = TRUE;
            FatAcquireExclusiveFcb( IrpContext, Fcb );
            AcquiredFcb = TRUE;

            if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( &Fcb->Specific.Fcb.FileLock );

            } else {

                OplockCount = Fcb->UncleanCount;
            }

            break;

        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
        case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
        case FSCTL_OPLOCK_BREAK_NOTIFY:
        case FSCTL_OPLOCK_BREAK_ACK_NO_2:

            FatAcquireSharedFcb( IrpContext, Fcb );
            AcquiredFcb = TRUE;
            break;

        default:

            FatBugCheck( FsControlCode, 0, 0 );
        }

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Specific.Fcb.Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    } _SEH2_FINALLY {

        DebugUnwind( FatOplockRequest );

        //
        //  Release all of our resources
        //

        if (AcquiredVcb) {

            FatReleaseVcb( IrpContext, Fcb->Vcb );
        }

        if (AcquiredFcb)  {

            FatReleaseFcb( IrpContext, Fcb );
        }

        if (!_SEH2_AbnormalTermination()) {

            FatCompleteRequest( IrpContext, FatNull, 0 );
        }

        DebugTrace(-1, Dbg, "FatOplockRequest -> %08lx\n", Status );
    } _SEH2_END;

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatLockVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Send our notification so that folks that like to hold handles on
    //  volumes can get out of the way.
    //

    FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_LOCK );

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!FatAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace( 0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", Status);
        return Status;
    }

    _SEH2_TRY {

        Status = FatLockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );

    } _SEH2_FINALLY {

        //
        //  Since we drop and release the vcb while trying to punch the volume
        //  down, it may be the case that we decide the operation should not
        //  continue if the user raced a CloeseHandle() with us (and it finished
        //  the cleanup) while we were waiting for our closes to finish.
        //
        //  In this case, we will have been raised out of the acquire logic with
        //  STATUS_FILE_CLOSED, and the volume will not be held.
        //

        if (!_SEH2_AbnormalTermination() || ExIsResourceAcquiredExclusiveLite( &Vcb->Resource )) {

            FatReleaseVcb( IrpContext, Vcb );
        }

        if (!NT_SUCCESS( Status ) || _SEH2_AbnormalTermination()) {

            //
            //  The volume lock will be failing.
            //

            FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_LOCK_FAILED );
        }
    } _SEH2_END;

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatUnlockVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Status = FatUnlockVolumeInternal( IrpContext, Vcb, IrpSp->FileObject );

    //
    //  Send notification that the volume is avaliable.
    //

    if (NT_SUCCESS( Status )) {

        FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_UNLOCK );
    }

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", Status);

    return Status;
}


NTSTATUS
FatLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine performs the actual lock volume operation.  It will be called
    by anyone wishing to try to protect the volume for a long duration.  PNP
    operations are such a user.

    The volume must be held exclusive by the caller.

Arguments:

    Vcb - The volume being locked.

    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL SavedIrql;
    ULONG RemainingUserReferences = (FileObject? 1: 0);

    ASSERT( ExIsResourceAcquiredExclusiveLite( &Vcb->Resource ) &&
            !ExIsResourceAcquiredExclusiveLite( &FatData.Resource ));
    //
    //  Go synchronous for the rest of the lock operation.  It may be
    //  reasonable to try to revisit this in the future, but for now
    //  the purge below expects to be able to wait.
    //
    //  We know it is OK to leave the flag up given how we're used at
    //  the moment.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  If there are any open handles, this will fail.
    //

    if (!FatIsHandleCountZero( IrpContext, Vcb )) {

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Force Mm to get rid of its referenced file objects.
    //

    FatFlushFat( IrpContext, Vcb );

    FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, Flush );

    FatCloseEaFile( IrpContext, Vcb, TRUE );

    //
    //  Now back out of our synchronization and wait for the lazy writer
    //  to finish off any lazy closes that could have been outstanding.
    //
    //  Since we flushed, we know that the lazy writer will issue all
    //  possible lazy closes in the next tick - if we hadn't, an otherwise
    //  unopened file with a large amount of dirty data could have hung
    //  around for a while as the data trickled out to the disk.
    //
    //  This is even more important now since we send notification to
    //  alert other folks that this style of check is about to happen so
    //  that they can close their handles.  We don't want to enter a fast
    //  race with the lazy writer tearing down his references to the file.
    //

    FatReleaseVcb( IrpContext, Vcb );

    Status = CcWaitForCurrentLazyWriterActivity();

    FatAcquireExclusiveVcb( IrpContext, Vcb );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Now rundown the delayed closes one last time.  We appear to be able
    //  to have additional collisions.
    //

    FatFspClose( Vcb );

    //
    //  Check if the Vcb is already locked, or if the open file count
    //  is greater than 1 (which implies that someone else also is
    //  currently using the volume, or a file on the volume), and that the
    //  VPB reference count only includes our residual and the handle (as
    //  appropriate).
    //
    //  We used to only check for the vpb refcount.  This is unreliable since
    //  the vpb refcount is dropped immediately before final close, meaning
    //  that even though we had a good refcount, the close was inflight and
    //  subsequent operations could get confused.  Especially if the PNP path
    //  was the lock caller, we delete the VCB with an outstanding opencount!
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if (!FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) &&
        (Vcb->Vpb->ReferenceCount <= 2 + RemainingUserReferences) &&
        (Vcb->OpenFileCount == (CLONG)( FileObject? 1: 0 ))) {

        SetFlag(Vcb->Vpb->Flags, VPB_LOCKED);
        SetFlag(Vcb->VcbState, VCB_STATE_FLAG_LOCKED);
        Vcb->FileObjectWithVcbLocked = FileObject;

    } else {

        Status = STATUS_ACCESS_DENIED;
    }

    IoReleaseVpbSpinLock( SavedIrql );

    //
    //  If we successully locked the volume, see if it is clean now.
    //

    if (NT_SUCCESS( Status ) &&
        FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY ) &&
        !FlagOn( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY ) &&
        !CcIsThereDirtyData(Vcb->Vpb)) {

        FatMarkVolume( IrpContext, Vcb, VolumeClean );
        ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );
    }

    ASSERT( !NT_SUCCESS(Status) || (Vcb->OpenFileCount == (CLONG)( FileObject? 1: 0 )));

    return Status;
}


NTSTATUS
FatUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine performs the actual unlock volume operation.

    The volume must be held exclusive by the caller.

Arguments:

    Vcb - The volume being locked.

    FileObject - File corresponding to the handle locking the volume.  If this
        is not specified, a system lock is assumed.

Return Value:

    NTSTATUS - The return status for the operation

    Attempting to remove a system lock that did not exist is OK.

--*/

{
    KIRQL SavedIrql;
    NTSTATUS Status = STATUS_NOT_LOCKED;

    IoAcquireVpbSpinLock( &SavedIrql );

    if (FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) && FileObject == Vcb->FileObjectWithVcbLocked) {

        //
        //  This one locked it, unlock the volume
        //

        ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED );
        ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_LOCKED );
        Vcb->FileObjectWithVcbLocked = NULL;

        Status = STATUS_SUCCESS;
    }

    IoReleaseVpbSpinLock( SavedIrql );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    BOOLEAN VcbHeld = FALSE;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatDismountVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens on media that is not boot/paging and is not
    //  already dismounted ... (but we need to check that stuff while
    //  synchronized)
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        Status = STATUS_INVALID_PARAMETER;
        goto fn_return;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Make some unsynchronized checks to see if this operation is possible.
    //  We will repeat the appropriate ones inside synchronization, but it is
    //  good to avoid bogus notifications.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE )) {

        Status = STATUS_ACCESS_DENIED;
        goto fn_return;
    }

    if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DISMOUNTED )) {

        Status = STATUS_VOLUME_DISMOUNTED;
        goto fn_return;
    }

    //
    //  A bit of historical comment is in order.
    //
    //  In all versions prior to NT5, we only permitted dismount if the volume had
    //  previously been locked.  Now we must permit a forced dismount, meaning that
    //  we grab ahold of the whole kit-n-kaboodle - regardless of activity, open
    //  handles, etc. - to flush and invalidate the volume.
    //
    //  Previously, dismount assumed that lock had come along earlier and done some
    //  of the work that we are now going to do - i.e., flush, tear down the eas. All
    //  we had to do here is flush the device out and kill off as many of the orphan
    //  fcbs as possible. This now changes.
    //
    //  In fact, everything is a forced dismount now. This changes one interesting
    //  aspect, which is that it used to be the case that the handle used to dismount
    //  could come back, read, and induce a verify/remount. This is just not possible
    //  now.  The point of forced dismount is that very shortly someone will come along
    //  and be destructive to the possibility of using the media further - format, eject,
    //  etc.  By using this path, callers are expected to tolerate the consequences.
    //
    //  Note that the volume can still be successfully unlocked by this handle.
    //

    //
    //  Send notification.
    //

    FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_DISMOUNT );

    //
    //  Force ourselves to wait and grab everything.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    (VOID)FatAcquireExclusiveGlobal( IrpContext );

    _SEH2_TRY {

        //
        //  Guess what? This can raise if a cleanup on the fileobject we
        //  got races in ahead of us.
        //

        FatAcquireExclusiveVolume( IrpContext, Vcb );
        VcbHeld = TRUE;

        if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        FatFlushAndCleanVolume( IrpContext, Irp, Vcb, FlushAndInvalidate );

        //
        //  We defer the physical dismount until this handle is closed, per symmetric
        //  implemntation in the other FS. This permits a dismounter to issue IOCTL
        //  through this handle and perform device manipulation without racing with
        //  creates attempting to mount the volume again.
        //
        //  Raise a flag to tell the cleanup path to complete the dismount.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_COMPLETE_DISMOUNT );

        //
        //  Indicate that the volume was dismounted so that we may return the
        //  correct error code when operations are attempted via open handles.
        //

        Vcb->VcbCondition = VcbBad;
        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DISMOUNTED );

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

    } _SEH2_FINALLY {

        if (VcbHeld) {

            FatReleaseVolume( IrpContext, Vcb );
        }

        FatReleaseGlobal( IrpContext );

        //
        //  I do not believe it is possible to raise, but for completeness
        //  notice and send notification of failure.  We absolutely
        //  cannot have raised in CheckForDismount.
        //
        //  We decline to call an attempt to dismount a dismounted volume
        //  a failure to do so.
        //

        if ((!NT_SUCCESS( Status ) && Status != STATUS_VOLUME_DISMOUNTED)
            || _SEH2_AbnormalTermination()) {

            FsRtlNotifyVolumeEvent( IrpSp->FileObject, FSRTL_VOLUME_DISMOUNT_FAILED );
        }
    } _SEH2_END;

    fn_return:

    FatCompleteRequest( IrpContext, Irp, Status );
    DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", Status);
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine marks the volume as dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatDirtyVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatDirtyVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatDirtyVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }


    //
    //  Disable popups, we will just return any error.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS);

    //
    //  Verify the Vcb.  We want to make sure we don't dirty some
    //  random chunk of media that happens to be in the drive now.
    //

    FatVerifyVcb( IrpContext, Vcb );

    SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

    FatMarkVolume( IrpContext, Vcb, VolumeDirty );

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatDirtyVolume -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PULONG VolumeState;

    //
    //  Get the current stack location and extract the output
    //  buffer information.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in the
    //  irp first.  Then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        VolumeState = Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        VolumeState = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, LowPagePriority );

        if (VolumeState == NULL) {

            FatCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the volume isn't dirty.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    *VolumeState = 0;

    //
    //  Decode the file object
    //

    TypeOfOpen = FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (Vcb->VcbCondition != VcbGood) {

        FatCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Disable PopUps, we want to return any error.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS);

    //
    //  Verify the Vcb.  We want to make double sure that this volume
    //  is around so that we know our information is good.
    //

    FatVerifyVcb( IrpContext, Vcb );

    //
    //  Now set the returned information.  We can avoid probing the disk since
    //  we know our internal state is in sync.
    //

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY | VCB_STATE_FLAG_MOUNTED_DIRTY) ) {

        SetFlag( *VolumeState, VOLUME_IS_DIRTY );
    }

    Irp->IoStatus.Information = sizeof( ULONG );

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently mounted.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb = NULL;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Status = STATUS_SUCCESS;

    DebugTrace(+1, Dbg, "FatIsVolumeMounted...\n", 0);

    //
    //  Decode the file object.
    //

    (VOID)FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    ASSERT( Vcb != NULL );

    //
    //  Disable PopUps, we want to return any error.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS);

    //
    //  Verify the Vcb.
    //

    FatVerifyVcb( IrpContext, Vcb );

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatIsVolumeMounted -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a pathname is a-priori illegal by inspecting
    the the characters used.  It is required to be correct on a FALSE return.

    N.B.: current implementation is intentioanlly a no-op.  This may change
    in the future.  A careful reader of the previous implementation of this
    FSCTL in FAT would discover that it violated the requirement stated above
    and could return FALSE for a valid (createable) pathname.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    DebugTrace(+1, Dbg, "FatIsPathnameValid...\n", 0);

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatIsPathnameValid -> %08lx\n", STATUS_SUCCESS);

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatQueryBpb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine simply returns the first 0x24 bytes of sector 0.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;

    PFSCTL_QUERY_FAT_BPB_BUFFER BpbBuffer;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatQueryBpb...\n", 0);

    //
    //  Get the Vcb.  If we didn't keep the information needed for this call,
    //  we had a reason ...
    //

    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    if (Vcb->First0x24BytesOfBootSector == NULL) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        DebugTrace(-1, Dbg, "FatQueryBpb -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Extract the buffer
    //

    BpbBuffer = (PFSCTL_QUERY_FAT_BPB_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    //
    //  Make sure the buffer is big enough.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < 0x24) {

        FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        DebugTrace(-1, Dbg, "FatQueryBpb -> %08lx\n", STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Fill in the output buffer
    //

    RtlCopyMemory( BpbBuffer->First0x24BytesOfBootSector,
                   Vcb->First0x24BytesOfBootSector,
                   0x24 );

    Irp->IoStatus.Information = 0x24;

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    DebugTrace(-1, Dbg, "FatQueryBpb -> %08lx\n", STATUS_SUCCESS);
    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatInvalidateVolumes (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine searches for all the volumes mounted on the same real device
    of the current DASD handle, and marks them all bad.  The only operation
    that can be done on such handles is cleanup and close.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    IRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    LUID TcbPrivilege = {SE_TCB_PRIVILEGE, 0};

    HANDLE Handle;

    PLIST_ENTRY Links;

    PFILE_OBJECT FileToMarkBad;
    PDEVICE_OBJECT DeviceToMarkBad;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatInvalidateVolumes...\n", 0);

    //
    //  Check for the correct security access.
    //  The caller must have the SeTcbPrivilege.
    //

    if (!SeSinglePrivilegeCheck(TcbPrivilege, Irp->RequestorMode)) {

        FatCompleteRequest( FatNull, Irp, STATUS_PRIVILEGE_NOT_HELD );

        DebugTrace(-1, Dbg, "FatInvalidateVolumes -> %08lx\n", STATUS_PRIVILEGE_NOT_HELD);
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    //  Try to get a pointer to the device object from the handle passed in.
    //

#if defined(_WIN64)
    if (IoIs32bitProcess( Irp )) {

        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(UINT32)) {
            
            FatCompleteRequest( FatNull, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace(-1, Dbg, "FatInvalidateVolumes -> %08lx\n", STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }

        Handle = (HANDLE) LongToHandle( (*(PUINT32)Irp->AssociatedIrp.SystemBuffer) );
    } else {
#endif
        if (IrpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(HANDLE)) {

            FatCompleteRequest( FatNull, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace(-1, Dbg, "FatInvalidateVolumes -> %08lx\n", STATUS_INVALID_PARAMETER);
            return STATUS_INVALID_PARAMETER;
        }

        Handle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;
#if defined(_WIN64)
    }
#endif


    Status = ObReferenceObjectByHandle( Handle,
                                        0,
                                        *IoFileObjectType,
                                        KernelMode,
#ifndef __REACTOS__
                                        &FileToMarkBad,
#else
                                        (PVOID *)&FileToMarkBad,
#endif
                                        NULL );

    if (!NT_SUCCESS(Status)) {

        FatCompleteRequest( FatNull, Irp, Status );

        DebugTrace(-1, Dbg, "FatInvalidateVolumes -> %08lx\n", Status);
        return Status;

    } else {

        //
        //  We only needed the pointer, not a reference.
        //

        ObDereferenceObject( FileToMarkBad );

        //
        //  Grab the DeviceObject from the FileObject.
        //

        DeviceToMarkBad = FileToMarkBad->DeviceObject;
    }

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );

    SetFlag( IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT );
    IrpContext.MajorFunction = IrpSp->MajorFunction;
    IrpContext.MinorFunction = IrpSp->MinorFunction;

    FatAcquireExclusiveGlobal( &IrpContext );

    //
    //  First acquire the FatData resource shared, then walk through all the
    //  mounted VCBs looking for candidates to mark BAD.
    //
    //  On volumes we mark bad, check for dismount possibility (which is
    //  why we have to get the next link early).
    //

    Links = FatData.VcbQueue.Flink;

    while (Links != &FatData.VcbQueue) {

        PVCB ExistingVcb;

        ExistingVcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

        Links = Links->Flink;

        //
        //  If we get a match, mark the volume Bad, and also check to
        //  see if the volume should go away.
        //

        if (ExistingVcb->Vpb->RealDevice == DeviceToMarkBad) {

            BOOLEAN VcbDeleted;

            VcbDeleted = FALSE;

            //
            //  Here we acquire the Vcb exclusive and try to purge
            //  all the open files.  The idea is to have as little as
            //  possible stale data visible and to hasten the volume
            //  going away.
            //

            (VOID)FatAcquireExclusiveVcb( &IrpContext, ExistingVcb );

            if (ExistingVcb->Vpb == DeviceToMarkBad->Vpb) {

                KIRQL OldIrql;
                    
                IoAcquireVpbSpinLock( &OldIrql );
    
                if (FlagOn( DeviceToMarkBad->Vpb->Flags, VPB_MOUNTED )) {

                    PVPB NewVpb;

                    NewVpb = ExistingVcb->SwapVpb;
                    ExistingVcb->SwapVpb = NULL;
                    SetFlag( ExistingVcb->VcbState, VCB_STATE_FLAG_VPB_MUST_BE_FREED );
                    
                    RtlZeroMemory( NewVpb, sizeof( VPB ) );
                    NewVpb->Type = IO_TYPE_VPB;
                    NewVpb->Size = sizeof( VPB );
                    NewVpb->RealDevice = DeviceToMarkBad;
                    NewVpb->Flags = FlagOn( DeviceToMarkBad->Vpb->Flags, VPB_REMOVE_PENDING );

                    DeviceToMarkBad->Vpb = NewVpb;
                }

                ASSERT( DeviceToMarkBad->Vpb->DeviceObject == NULL );

                IoReleaseVpbSpinLock( OldIrql );
            }

            FatSetVcbCondition( ExistingVcb, VcbBad );
            
            //
            //  Process the root directory, if it is present.
            //

            if (ExistingVcb->RootDcb != NULL) {

                FatMarkFcbCondition( &IrpContext, ExistingVcb->RootDcb, FcbBad, TRUE );

                //
                //  Purging the file objects on this volume could result in the memory manager
                //  dereferencing it's file pointer which could be the last reference and
                //  trigger object deletion and VCB deletion. Protect against that here by
                //  temporarily biasing the file count, and later checking for dismount.
                //

                ExistingVcb->OpenFileCount += 1;
                
                FatPurgeReferencedFileObjects( &IrpContext,
                                               ExistingVcb->RootDcb,
                                               NoFlush );

                ExistingVcb->OpenFileCount -= 1;

                VcbDeleted = FatCheckForDismount( &IrpContext, ExistingVcb, FALSE );
            }

            //
            //  Drop the resource.
            //

            if (VcbDeleted != TRUE) {
                FatReleaseVcb( &IrpContext, ExistingVcb );
            }
        }
    }

    FatReleaseGlobal( &IrpContext );

    FatCompleteRequest( FatNull, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatInvalidateVolumes -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}


//
//  Local Support routine
//

BOOLEAN
FatPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LBO Lbo,
    IN ULONG NumberOfBytesToRead,
    IN BOOLEAN ReturnOnError
    )

/*++

Routine Description:

    This routine is used to read in a range of bytes from the disk.  It
    bypasses all of the caching and regular I/O logic, and builds and issues
    the requests itself.  It does this operation overriding the verify
    volume flag in the device object.

Arguments:

    Vcb - Supplies the target device object for this operation.

    Buffer - Supplies the buffer that will recieve the results of this operation

    Lbo - Supplies the byte offset of where to start reading

    NumberOfBytesToRead - Supplies the number of bytes to read, this must
        be in multiple of bytes units acceptable to the disk driver.

    ReturnOnError - Indicates that we should return on an error, instead
        of raising.

Return Value:

    BOOLEAN - TRUE if the operation succeded, FALSE otherwise.

--*/

{
    KEVENT Event;
    PIRP Irp;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    DebugTrace(0, Dbg, "FatPerformVerifyDiskRead, Lbo = %08lx\n", Lbo );

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation and also set the overrride flag
    //

    ByteOffset.QuadPart = Lbo;

    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        Vcb->TargetDeviceObject,
                                        Buffer,
                                        NumberOfBytesToRead,
                                        &ByteOffset,
                                        &Event,
                                        &Iosb );

    if ( Irp == NULL ) {

        FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Call the device to do the read and wait for it to finish.
    //

    Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );

    if (Status == STATUS_PENDING) {

        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    ASSERT( Status != STATUS_VERIFY_REQUIRED );

    //
    //  Special case this error code because this probably means we used
    //  the wrong sector size and we want to reject STATUS_WRONG_VOLUME.
    //

    if (Status == STATUS_INVALID_PARAMETER) {

        return FALSE;
    }

    //
    //  If it doesn't succeed then either return or raise the error.
    //

    if (!NT_SUCCESS(Status)) {

        if (ReturnOnError) {

            return FALSE;

        } else {

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }
    }

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Local Support Routine
//

NTSTATUS
FatQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the query retrieval pointers operation.
    It returns the retrieval pointers for the specified input
    file from the start of the file to the request map size specified
    in the input buffer.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PLARGE_INTEGER RequestedMapSize;
    PLARGE_INTEGER *MappingPairs;

    ULONG Index;
    ULONG i;
    ULONG SectorCount;
    LBO Lbo;
    ULONG Vbo;
    ULONG MapSize;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Decode the file object and ensure that it is the paging file
    //
    //  Only Kernel mode clients may query retrieval pointer information about
    //  a file.  Ensure that this is the case for this caller.
    //

    (VOID)FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if (Irp->RequestorMode != KernelMode ||
        Fcb == NULL || 
        !FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Extract the input and output buffer information.  The input contains
    //  the requested size of the mappings in terms of VBO.  The output
    //  parameter will receive a pointer to nonpaged pool where the mapping
    //  pairs are stored.
    //

    ASSERT( IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof(LARGE_INTEGER) );
    ASSERT( IrpSp->Parameters.FileSystemControl.OutputBufferLength == sizeof(PVOID) );

    RequestedMapSize = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    MappingPairs = Irp->UserBuffer;

    //
    //  Acquire exclusive access to the Fcb
    //

    if (!FatAcquireExclusiveFcb( IrpContext, Fcb )) {

        return FatFsdPostRequest( IrpContext, Irp );
    }

    _SEH2_TRY {

        //
        //  Verify the Fcb is still OK
        //

        FatVerifyFcb( IrpContext, Fcb );

        //
        //  Check if the mapping the caller requested is too large
        //

        if ((*RequestedMapSize).QuadPart > Fcb->Header.FileSize.QuadPart) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Now get the index for the mcb entry that will contain the
        //  callers request and allocate enough pool to hold the
        //  output mapping pairs
        //

        (VOID)FatLookupMcbEntry( Fcb->Vcb, &Fcb->Mcb, RequestedMapSize->LowPart - 1, &Lbo, NULL, &Index );

        *MappingPairs = FsRtlAllocatePoolWithTag( NonPagedPool,
                                                  (Index + 2) * (2 * sizeof(LARGE_INTEGER)),
                                                  TAG_OUTPUT_MAPPINGPAIRS );

        //
        //  Now copy over the mapping pairs from the mcb
        //  to the output buffer.  We store in [sector count, lbo]
        //  mapping pairs and end with a zero sector count.
        //

        MapSize = RequestedMapSize->LowPart;

        for (i = 0; i <= Index; i += 1) {

#ifndef __REACTOS__
            (VOID)FatGetNextMcbEntry( Fcb->Vcb, &Fcb->Mcb, i, &Vbo, &Lbo, &SectorCount );
#else
            (VOID)FatGetNextMcbEntry( Fcb->Vcb, &Fcb->Mcb, i, (PVBO)&Vbo, &Lbo, &SectorCount );
#endif

            if (SectorCount > MapSize) {
                SectorCount = MapSize;
            }

            (*MappingPairs)[ i*2 + 0 ].QuadPart = SectorCount;
            (*MappingPairs)[ i*2 + 1 ].QuadPart = Lbo;

            MapSize -= SectorCount;
        }

        (*MappingPairs)[ i*2 + 0 ].QuadPart = 0;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatQueryRetrievalPointers );

        //
        //  Release all of our resources
        //

        FatReleaseFcb( IrpContext, Fcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!_SEH2_AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }
    } _SEH2_END;

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatGetStatistics (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the filesystem performance counters from the
    appropriate VCB.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PVCB Vcb;

    PFILE_SYSTEM_STATISTICS Buffer;
    ULONG BufferLength;
    ULONG StatsSize;
    ULONG BytesToCopy;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatGetStatistics...\n", 0);

    //
    // Extract the buffer
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    //  Get a pointer to the output buffer.
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Make sure the buffer is big enough for at least the common part.
    //

    if (BufferLength < sizeof(FILESYSTEM_STATISTICS)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );

        DebugTrace(-1, Dbg, "FatGetStatistics -> %08lx\n", STATUS_BUFFER_TOO_SMALL );

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Now see how many bytes we can copy.
    //

    StatsSize = sizeof(FILE_SYSTEM_STATISTICS) * KeNumberProcessors;

    if (BufferLength < StatsSize) {

        BytesToCopy = BufferLength;
        Status = STATUS_BUFFER_OVERFLOW;

    } else {

        BytesToCopy = StatsSize;
        Status =  STATUS_SUCCESS;
    }

    //
    //  Get the Vcb.
    //

    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    //
    //  Fill in the output buffer
    //

    RtlCopyMemory( Buffer, Vcb->Statistics, BytesToCopy );

    Irp->IoStatus.Information = BytesToCopy;

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatGetStatistics -> %08lx\n", Status);

    return Status;
}

//
//  Local Support Routine
//

NTSTATUS
FatGetVolumeBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the volume allocation bitmap.

        Input = the STARTING_LCN_INPUT_BUFFER data structure is passed in
            through the input buffer.
        Output = the VOLUME_BITMAP_BUFFER data structure is returned through
            the output buffer.

    We return as much as the user buffer allows starting the specified input
    LCN (trucated to a byte).  If there is no input buffer, we start at zero.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    ULONG BytesToCopy;
    ULONG TotalClusters;
    ULONG DesiredClusters;
    ULONG StartingCluster;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    LARGE_INTEGER StartingLcn;
    PVOLUME_BITMAP_BUFFER OutputBuffer;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatGetVolumeBitmap, FsControlCode = %08lx\n",
               IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    //  Extract and decode the file object and check for type of open.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatGetVolumeBitmap -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    OutputBuffer = (PVOLUME_BITMAP_BUFFER)FatMapUserBuffer( IrpContext, Irp );

    //
    //  Check for a minimum length on the input and output buffers.
    //

    if ((InputBufferLength < sizeof(STARTING_LCN_INPUT_BUFFER)) ||
        (OutputBufferLength < sizeof(VOLUME_BITMAP_BUFFER))) {

        FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Check if a starting cluster was specified.
    //

    TotalClusters = Vcb->AllocationSupport.NumberOfClusters;

    //
    //  Check for valid buffers
    //

    _SEH2_TRY {

        if (Irp->RequestorMode != KernelMode) {

            ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                          InputBufferLength,
                          sizeof(UCHAR) );

            ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(UCHAR) );
        }

        StartingLcn = ((PSTARTING_LCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->StartingLcn;

    } _SEH2_EXCEPT( Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH ) {

          Status = _SEH2_GetExceptionCode();

          FatRaiseStatus( IrpContext,
                          FsRtlIsNtstatusExpected(Status) ?
                          Status : STATUS_INVALID_USER_BUFFER );
    } _SEH2_END;

    if (StartingLcn.HighPart || StartingLcn.LowPart >= TotalClusters) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;

    } else {

        StartingCluster = StartingLcn.LowPart & ~7;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!FatAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace( 0, Dbg, "Cannot acquire Vcb\n", 0);

        ASSERT( Irp->RequestorMode == KernelMode );

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatGetVolumeBitmap -> %08lx\n", Status);
        return Status;
    }

    //
    //  Only return what will fit in the user buffer.
    //

    OutputBufferLength -= FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer);
    DesiredClusters = TotalClusters - StartingCluster;

    if (OutputBufferLength < (DesiredClusters + 7) / 8) {

        BytesToCopy = OutputBufferLength;
        Status = STATUS_BUFFER_OVERFLOW;

    } else {

        BytesToCopy = (DesiredClusters + 7) / 8;
        Status = STATUS_SUCCESS;
    }

    //
    //  Use try/finally for cleanup.
    //

    _SEH2_TRY {

        _SEH2_TRY {

            //
            //  Verify the Vcb is still OK
            //

            FatQuickVerifyVcb( IrpContext, Vcb );

            //
            //  Fill in the fixed part of the output buffer
            //

            OutputBuffer->StartingLcn.QuadPart = StartingCluster;
            OutputBuffer->BitmapSize.QuadPart = DesiredClusters;

            if (Vcb->NumberOfWindows == 1) {

                //
                //  Just copy the volume bitmap into the user buffer.
                //

                ASSERT( Vcb->FreeClusterBitMap.Buffer != NULL );

                RtlCopyMemory( &OutputBuffer->Buffer[0],
                               (PUCHAR)Vcb->FreeClusterBitMap.Buffer + StartingCluster/8,
                               BytesToCopy );
            } else {

                //
                //  Call out to analyze the FAT.  We must bias by two to account for
                //  the zero base of this API and FAT's physical reality of starting
                //  the file heap at cluster 2.
                //
                //  Note that the end index is inclusive - we need to subtract one to
                //  calculcate it.
                //
                //  I.e.: StartingCluster 0 for one byte of bitmap means a start cluster
                //  of 2 and end cluster of 9, a run of eight clusters.
                //

                FatExamineFatEntries( IrpContext,
                                      Vcb,
                                      StartingCluster + 2,
                                      StartingCluster + BytesToCopy * 8 + 2 - 1,
                                      FALSE,
                                      NULL,
                                      (PULONG)&OutputBuffer->Buffer[0] );
            }

        } _SEH2_EXCEPT( Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH ) {

            Status = _SEH2_GetExceptionCode();

            FatRaiseStatus( IrpContext,
                            FsRtlIsNtstatusExpected(Status) ?
                            Status : STATUS_INVALID_USER_BUFFER );
        } _SEH2_END;

    } _SEH2_FINALLY {

        FatReleaseVcb( IrpContext, Vcb );
    } _SEH2_END;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer) +
                                BytesToCopy;

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatGetVolumeBitmap -> VOID\n", 0);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatGetRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine scans the MCB and builds an extent list.  The first run in
    the output extent list will start at the begining of the contiguous
    run specified by the input parameter.

        Input = STARTING_VCN_INPUT_BUFFER;
        Output = RETRIEVAL_POINTERS_BUFFER.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB FcbOrDcb;
    PCCB Ccb;
    TYPE_OF_OPEN TypeOfOpen;

    ULONG Index;
    ULONG ClusterShift;
    ULONG AllocationSize;

    ULONG Run;
    ULONG RunCount;
    ULONG StartingRun;
    LARGE_INTEGER StartingVcn;

    ULONG InputBufferLength;
    ULONG OutputBufferLength;

    PRETRIEVAL_POINTERS_BUFFER OutputBuffer;

    BOOLEAN FcbLocked;

    //
    // Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatGetRetrievalPointers, FsControlCode = %08lx\n",
               IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    // Extract and decode the file object and check for type of open.
    //

    TypeOfOpen = FatDecodeFileObject( IrpSp->FileObject, &Vcb, &FcbOrDcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get the input and output buffer lengths and pointers.
    //  Initialize some variables.
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    OutputBuffer = (PRETRIEVAL_POINTERS_BUFFER)FatMapUserBuffer( IrpContext, Irp );

    //
    //  Check for a minimum length on the input and ouput buffers.
    //

    if ((InputBufferLength < sizeof(STARTING_VCN_INPUT_BUFFER)) ||
        (OutputBufferLength < sizeof(RETRIEVAL_POINTERS_BUFFER))) {

        FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Acquire the Fcb and enqueue the Irp if we didn't get access.  Go for
    //  shared on read-only media so we can allow prototype XIP to get
    //  recursive, as well as recognizing this is safe anyway.
    //

    if (FlagOn( FcbOrDcb->Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED )) {

        FcbLocked = FatAcquireSharedFcb( IrpContext, FcbOrDcb );

    } else {

        FcbLocked = FatAcquireExclusiveFcb( IrpContext, FcbOrDcb );
    }

    if (!FcbLocked) {

        DebugTrace( 0, Dbg, "Cannot acquire Vcb\n", 0);

        ASSERT( Irp->RequestorMode == KernelMode );

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatGetRetrievalPointers -> %08lx\n", Status);
        return Status;
    }
    
    _SEH2_TRY {

        //
        //  Verify the Fcb is still OK
        //

        FatVerifyFcb( IrpContext, FcbOrDcb );

        //
        //  If we haven't yet set the correct AllocationSize, do so.
        //

        if (FcbOrDcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

            FatLookupFileAllocationSize( IrpContext, FcbOrDcb );

            //
            //  If this is a non-root directory, we have a bit more to
            //  do since it has not gone through FatOpenDirectoryFile().
            //

            if (NodeType(FcbOrDcb) == FAT_NTC_DCB ||
                (NodeType(FcbOrDcb) == FAT_NTC_ROOT_DCB && FatIsFat32(Vcb))) {

                FcbOrDcb->Header.FileSize.LowPart =
                    FcbOrDcb->Header.AllocationSize.LowPart;
            }
        }

        //
        //  Check if a starting cluster was specified.
        //

        ClusterShift = Vcb->AllocationSupport.LogOfBytesPerCluster;
        AllocationSize = FcbOrDcb->Header.AllocationSize.LowPart;

        _SEH2_TRY {

            if (Irp->RequestorMode != KernelMode) {

                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              InputBufferLength,
                              sizeof(UCHAR) );

                ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(UCHAR) );
            }

            StartingVcn = ((PSTARTING_VCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->StartingVcn;

        } _SEH2_EXCEPT( Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH ) {

              Status = _SEH2_GetExceptionCode();

              FatRaiseStatus( IrpContext,
                              FsRtlIsNtstatusExpected(Status) ?
                              Status : STATUS_INVALID_USER_BUFFER );
        } _SEH2_END;

        if (StartingVcn.HighPart ||
            StartingVcn.LowPart >= (AllocationSize >> ClusterShift)) {

            try_return( Status = STATUS_END_OF_FILE );

        } else {

            //
            //  If we don't find the run, something is very wrong.
            //

            LBO Lbo;

            if (!FatLookupMcbEntry( FcbOrDcb->Vcb, &FcbOrDcb->Mcb,
                                    StartingVcn.LowPart << ClusterShift,
                                    &Lbo,
                                    NULL,
                                    &StartingRun)) {

                FatBugCheck( (ULONG_PTR)FcbOrDcb, (ULONG_PTR)&FcbOrDcb->Mcb, StartingVcn.LowPart );
            }
        }

        //
        //  Now go fill in the ouput buffer with run information
        //

        RunCount = FsRtlNumberOfRunsInLargeMcb( &FcbOrDcb->Mcb );

        for (Index = 0, Run = StartingRun; Run < RunCount; Index++, Run++) {

            ULONG Vcn;
            LBO Lbo;
            ULONG ByteLength;

            //
            //  Check for an exhausted output buffer.
            //

            if ((ULONG)FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[Index+1]) > OutputBufferLength) {


                //
                //  We've run out of space, so we won't be storing as many runs to the
                //  user's buffer as we had originally planned.  We need to return the
                //  number of runs that we did have room for.
                //

                _SEH2_TRY {

                    OutputBuffer->ExtentCount = Index;

                } _SEH2_EXCEPT( Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH ) {

                    Status = _SEH2_GetExceptionCode();

                    FatRaiseStatus( IrpContext,
                                    FsRtlIsNtstatusExpected(Status) ?
                                    Status : STATUS_INVALID_USER_BUFFER );
                } _SEH2_END;

                Irp->IoStatus.Information = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[Index]);
                try_return( Status = STATUS_BUFFER_OVERFLOW );
            }

            //
            //  Get the extent.  If it's not there or malformed, something is very wrong.
            //

#ifndef __REACTOS__
            if (!FatGetNextMcbEntry(Vcb, &FcbOrDcb->Mcb, Run, &Vcn, &Lbo, &ByteLength)) {
#else
            if (!FatGetNextMcbEntry(Vcb, &FcbOrDcb->Mcb, Run, (PVBO)&Vcn, &Lbo, &ByteLength)) {
#endif
                FatBugCheck( (ULONG_PTR)FcbOrDcb, (ULONG_PTR)&FcbOrDcb->Mcb, Run );
            }

            //
            //  Fill in the next array element.
            //

            _SEH2_TRY {

                OutputBuffer->Extents[Index].NextVcn.QuadPart = (Vcn + ByteLength) >> ClusterShift;
                OutputBuffer->Extents[Index].Lcn.QuadPart = FatGetIndexFromLbo( Vcb, Lbo ) - 2;

                //
                //  If this is the first run, fill in the starting Vcn
                //

                if (Index == 0) {
                    OutputBuffer->ExtentCount = RunCount - StartingRun;
                    OutputBuffer->StartingVcn.QuadPart = Vcn >> ClusterShift;
                }

            } _SEH2_EXCEPT( Irp->RequestorMode != KernelMode ? EXCEPTION_EXECUTE_HANDLER: EXCEPTION_CONTINUE_SEARCH ) {

                Status = _SEH2_GetExceptionCode();

                FatRaiseStatus( IrpContext,
                                FsRtlIsNtstatusExpected(Status) ?
                                Status : STATUS_INVALID_USER_BUFFER );
            } _SEH2_END;
        }

        //
        //  We successfully retrieved extent info to the end of the allocation.
        //

        Irp->IoStatus.Information = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[Index]);
        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

    } _SEH2_FINALLY {

        DebugUnwind( FatGetRetrievalPointers );

        //
        //  Release resources
        //

        FatReleaseFcb( IrpContext, FcbOrDcb );

        //
        //  If nothing raised then complete the irp.
        //

        if (!_SEH2_AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatGetRetrievalPointers -> VOID\n", 0);
    } _SEH2_END;

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatMoveFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    Routine moves a file to the requested Starting Lcn from Starting Vcn for the length
    of cluster count. These values are passed in through the the input buffer as a
    MOVE_DATA structure.

    The call must be made with a DASD handle.  The file to move is passed in as a
    parameter.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB FcbOrDcb;
    PCCB Ccb;

    ULONG InputBufferLength;
    PMOVE_FILE_DATA InputBuffer;

    ULONG ClusterShift;
    ULONG MaxClusters;

    ULONG FileOffset;

    LBO TargetLbo;
    ULONG TargetCluster;
    LARGE_INTEGER LargeSourceLbo;
    LARGE_INTEGER LargeTargetLbo;

    ULONG ByteCount;
    ULONG BytesToWrite;
    ULONG BytesToReallocate;
#ifndef __REACTOS__
    ULONG TargetAllocation;
#endif

    ULONG FirstSpliceSourceCluster;
    ULONG FirstSpliceTargetCluster;
    ULONG SecondSpliceSourceCluster;
    ULONG SecondSpliceTargetCluster;

    LARGE_MCB SourceMcb;
    LARGE_MCB TargetMcb;

    KEVENT StackEvent;

    PVOID Buffer;
    ULONG BufferSize;

    BOOLEAN SourceMcbInitialized = FALSE;
    BOOLEAN TargetMcbInitialized = FALSE;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN EventArmed = FALSE;
    BOOLEAN DiskSpaceAllocated = FALSE;

    PDIRENT Dirent;
    PBCB DirentBcb = NULL;

#if defined(_WIN64)
    MOVE_FILE_DATA LocalMoveFileData;
    PMOVE_FILE_DATA32 MoveFileData32;
#endif

    ULONG LocalAbnormalTermination = 0;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatMoveFile, FsControlCode = %08lx\n",
               IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    //  Force WAIT to true.  We have a handle in the input buffer which can only
    //  be referenced within the originating process.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Extract and decode the file object and check for type of open.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &FcbOrDcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    InputBuffer = (PMOVE_FILE_DATA)Irp->AssociatedIrp.SystemBuffer;

    //
    //  Do a quick check on the input buffer.
    //
    
#if defined(_WIN64)
    if (IoIs32bitProcess( Irp )) {

        if (InputBuffer == NULL || InputBufferLength < sizeof(MOVE_FILE_DATA32)) {

            FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
            return STATUS_BUFFER_TOO_SMALL;
        }

        MoveFileData32 = (PMOVE_FILE_DATA32) InputBuffer;

        LocalMoveFileData.FileHandle = (HANDLE) LongToHandle( MoveFileData32->FileHandle );
        LocalMoveFileData.StartingVcn = MoveFileData32->StartingVcn;
        LocalMoveFileData.StartingLcn = MoveFileData32->StartingLcn;
        LocalMoveFileData.ClusterCount = MoveFileData32->ClusterCount;

        InputBuffer = &LocalMoveFileData;

    } else {
#endif
        if (InputBuffer == NULL || InputBufferLength < sizeof(MOVE_FILE_DATA)) {

            FatCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
            return STATUS_BUFFER_TOO_SMALL;
        }
#if defined(_WIN64)
    }
#endif

    MaxClusters = Vcb->AllocationSupport.NumberOfClusters;
    TargetCluster = InputBuffer->StartingLcn.LowPart + 2;

    if (InputBuffer->StartingVcn.HighPart ||
        InputBuffer->StartingLcn.HighPart ||
        (TargetCluster < 2) ||
        (TargetCluster + InputBuffer->ClusterCount < TargetCluster) ||
        (TargetCluster + InputBuffer->ClusterCount > MaxClusters + 2) ||
        (InputBuffer->StartingVcn.LowPart >= MaxClusters)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Try to get a pointer to the file object from the handle passed in.
    //

    Status = ObReferenceObjectByHandle( InputBuffer->FileHandle,
                                        0,
                                        *IoFileObjectType,
                                        Irp->RequestorMode,
#ifndef __REACTOS__
                                        &FileObject,
#else
                                        (PVOID *)&FileObject,
#endif
                                        NULL );

    if (!NT_SUCCESS(Status)) {

        FatCompleteRequest( IrpContext, Irp, Status );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", Status);
        return Status;
    }

    //
    //  There are three basic ways this could be an invalid attempt, so
    //  we need to
    //
    //    - check that this file object is opened on the same volume as the
    //      DASD handle used to call this routine.
    //
    //    - extract and decode the file object and check for type of open.
    //
    //    - if this is a directory, verify that it's not the root and that
    //      we are not trying to move the first cluster.  We cannot move the
    //      first cluster because sub-directories have this cluster number
    //      in them and there is no safe way to simultaneously update them
    //      all.
    //
    //  We'll allow movefile on the root dir if its fat32, since the root dir
    //  is a real chained file there.
    //

    if (FileObject->Vpb != Vcb->Vpb) {

        ObDereferenceObject( FileObject );
        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    TypeOfOpen = FatDecodeFileObject( FileObject, &Vcb, &FcbOrDcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen &&
         TypeOfOpen != UserDirectoryOpen) ||

        ((TypeOfOpen == UserDirectoryOpen) &&
         ((NodeType(FcbOrDcb) == FAT_NTC_ROOT_DCB && !FatIsFat32(Vcb)) ||
          (InputBuffer->StartingVcn.QuadPart == 0)))) {

        ObDereferenceObject( FileObject );
        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatMoveFile -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Indicate we're getting to parents of this fcb by their child, and that
    //  this is a sufficient assertion of our ability to by synchronized
    //  with respect to the parent directory going away.
    //
    //  The defrag path is an example of one which arrives at an Fcb by
    //  a means which would be unreasonable to duplicate in the assertion
    //  code. See FatOpenDirectoryFile.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_PARENT_BY_CHILD );

    ClusterShift = Vcb->AllocationSupport.LogOfBytesPerCluster;

    _SEH2_TRY {

        //
        //  Initialize our state variables and the event.
        //

        FileOffset = InputBuffer->StartingVcn.LowPart << ClusterShift;

        ByteCount = InputBuffer->ClusterCount << ClusterShift;

        TargetLbo = FatGetLboFromIndex( Vcb, TargetCluster );
        LargeTargetLbo.QuadPart = TargetLbo;

        Buffer = NULL;

        //
        //  Do a quick check on parameters here
        //

        if (FileOffset + ByteCount < FileOffset) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        KeInitializeEvent( &StackEvent, NotificationEvent, FALSE );

        //
        //  Initialize two MCBs we will be using
        //

        FsRtlInitializeLargeMcb( &SourceMcb, PagedPool );
        SourceMcbInitialized = TRUE;

        FsRtlInitializeLargeMcb( &TargetMcb, PagedPool );
        TargetMcbInitialized = TRUE;

        //
        //  Ok, now if this is a directory open we need to switch to the internal
        //  stream fileobject since it is set up for caching.  The top-level
        //  fileobject has no section object pointers in order prevent folks from
        //  mapping it.
        //

        if (TypeOfOpen == UserDirectoryOpen) {

            PFILE_OBJECT DirStreamFileObject;

            //
            //  Open the stream fileobject if neccesary.  We must acquire the Fcb
            //  now to synchronize with other operations (such as dismount ripping
            //  apart the allocator).
            //

            (VOID)FatAcquireExclusiveFcb( IrpContext, FcbOrDcb );
            FcbAcquired = TRUE;

            FatVerifyFcb( IrpContext, FcbOrDcb );

            FatOpenDirectoryFile( IrpContext, FcbOrDcb );
            DirStreamFileObject = FcbOrDcb->Specific.Dcb.DirectoryFile;

            //
            //  Transfer our reference to the internal stream and proceed.  Note that
            //  if we dereferenced first, the user could sneak a teardown through since
            //  we'd have no references.
            //

            ObReferenceObject( DirStreamFileObject );
            ObDereferenceObject( FileObject );
            FileObject = DirStreamFileObject;
        }

        //
        //  Determine the size of the buffer we will use to move data.
        //

        BufferSize = FAT_DEFAULT_DEFRAG_CHUNK_IN_BYTES;

        if (BufferSize < (ULONG)(1 << ClusterShift)) {

            BufferSize = (1 << ClusterShift);
        }

        while (ByteCount) {

            VBO TempVbo;
            LBO TempLbo;
            ULONG TempByteCount;

            //
            //  We must throttle our writes.
            //

            CcCanIWrite( FileObject,
                         BufferSize,
                         TRUE,
                         FALSE );

            //
            //  Aqcuire file resource exclusive to freeze FileSize and block
            //  user non-cached I/O.  Verify the integrity of the fcb - the
            //  media may have changed (or been dismounted) on us.
            //

            if (FcbAcquired == FALSE) {

                (VOID)FatAcquireExclusiveFcb( IrpContext, FcbOrDcb );
                FcbAcquired = TRUE;
                
                FatVerifyFcb( IrpContext, FcbOrDcb );
            }

            //
            //  Allocate our buffer, if we need to.
            //

            if (Buffer == NULL) {

                Buffer = FsRtlAllocatePoolWithTag( NonPagedPool,
                                                   BufferSize,
                                                   TAG_DEFRAG_BUFFER );
            }

            //
            //  Analyzes the range of file allocation we are moving
            //  and determines the actual amount of allocation to be
            //  moved and how much needs to be written.  In addition
            //  it guarantees that the Mcb in the file is large enough
            //  so that later MCB operations cannot fail.
            //

            FatComputeMoveFileParameter( IrpContext,
                                         FcbOrDcb,
                                         BufferSize,
                                         FileOffset,
                                         &ByteCount,
                                         &BytesToReallocate,
                                         &BytesToWrite,
                                         &LargeSourceLbo );

            //
            //  If ByteCount comes back zero, break here.
            //

            if (ByteCount == 0) {
                break;
            }

            //
            //  At this point (before actually doing anything with the disk
            //  meta data), calculate the FAT splice clusters and build an
            //  MCB describing the space to be deallocated.
            //

            FatComputeMoveFileSplicePoints( IrpContext,
                                            FcbOrDcb,
                                            FileOffset,
                                            TargetCluster,
                                            BytesToReallocate,
                                            &FirstSpliceSourceCluster,
                                            &FirstSpliceTargetCluster,
                                            &SecondSpliceSourceCluster,
                                            &SecondSpliceTargetCluster,
                                            &SourceMcb );

            //
            //  Now attempt to allocate the new disk storage using the
            //  Target Lcn as a hint.
            //

            TempByteCount = BytesToReallocate;
            FatAllocateDiskSpace( IrpContext,
                                  Vcb,
                                  TargetCluster,
                                  &TempByteCount,
                                  TRUE,
                                  &TargetMcb );

            DiskSpaceAllocated = TRUE;

            //
            //  If we didn't get EXACTLY what we wanted, return immediately.
            //

            if ((FsRtlNumberOfRunsInLargeMcb( &TargetMcb ) != 1) ||
                !FatGetNextMcbEntry( Vcb, &TargetMcb, 0, &TempVbo, &TempLbo, &TempByteCount ) ||
                (FatGetIndexFromLbo( Vcb, TempLbo) != TargetCluster ) ||
                (TempByteCount != BytesToReallocate)) {

                //
                //  It would be nice if we could be more specific, but such is life.
                //
                try_return( Status = STATUS_INVALID_PARAMETER );
            }

#if DBG
            //
            //  We are going to attempt a move, note it.
            //

            if (FatMoveFileDebug) {
                DbgPrint("%lx: Vcn 0x%lx, Lcn 0x%lx, Count 0x%lx.\n",
                         PsGetCurrentThread(),
                         FileOffset >> ClusterShift,
                         TargetCluster,
                         BytesToReallocate >> ClusterShift );
            }
#endif

            //
            //  Now attempt to commit the new allocation to disk.  If this
            //  raises, the allocation will be deallocated.
            //

            FatFlushFatEntries( IrpContext,
                                Vcb,
                                TargetCluster,
                                BytesToReallocate >> ClusterShift );

            //
            //  Aqcuire both resources exclusive now, guaranteeing that NOBODY
            //  is in either the read or write paths.
            //

            ExAcquireResourceExclusiveLite( FcbOrDcb->Header.PagingIoResource, TRUE );

            //
            //  This is the first part of some tricky synchronization.
            //
            //  Set the Event pointer in the FCB.  Any paging I/O will block on
            //  this event (if set in FCB) after acquiring the PagingIo resource.
            //
            //  This is how I keep ALL I/O out of this path without holding the
            //  PagingIo resource exclusive for an extended time.
            //

            FcbOrDcb->MoveFileEvent = &StackEvent;
            EventArmed = TRUE;

            ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );

            //
            //  Now write out the data, but only if we have to.  We don't have
            //  to copy any file data if the range being reallocated is wholly
            //  beyond valid data length.
            //

            if (BytesToWrite) {

                PIRP IoIrp;
                KEVENT IoEvent;
                IO_STATUS_BLOCK Iosb;

                KeInitializeEvent( &IoEvent,
                                   NotificationEvent,
                                   FALSE );

                ASSERT( LargeTargetLbo.QuadPart >= Vcb->AllocationSupport.FileAreaLbo);

                //
                //  Read in the data that is being moved.
                //

                IoIrp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                                      Vcb->TargetDeviceObject,
                                                      Buffer,
                                                      BytesToWrite,
                                                      &LargeSourceLbo,
                                                      &IoEvent,
                                                      &Iosb );

                if (IoIrp == NULL) {

                    FatRaiseStatus( IrpContext,
                                    STATUS_INSUFFICIENT_RESOURCES );
                }

                Status = IoCallDriver( Vcb->TargetDeviceObject,
                                       IoIrp );

                if (Status == STATUS_PENDING) {

                    (VOID)KeWaitForSingleObject( &IoEvent,
                                                 Executive,
                                                 KernelMode,
                                                 FALSE,
                                                 (PLARGE_INTEGER)NULL );

                    Status = Iosb.Status;
                }

                if (!NT_SUCCESS( Status )) {

                    FatNormalizeAndRaiseStatus( IrpContext,
                                                Status );
                }

                //
                //  Write the data to its new location.
                //

                KeInitializeEvent( &IoEvent, NotificationEvent, FALSE );

                IoIrp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                                      Vcb->TargetDeviceObject,
                                                      Buffer,
                                                      BytesToWrite,
                                                      &LargeTargetLbo,
                                                      &IoEvent,
                                                      &Iosb );

                if (!IoIrp) {
                    FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                //  Set a flag indicating that we want to write through any
                //  cache on the controller.  This eliminates the need for
                //  an explicit flush-device after the write.
                //

                SetFlag( IoGetNextIrpStackLocation(IoIrp)->Flags, SL_WRITE_THROUGH );

                Status = IoCallDriver( Vcb->TargetDeviceObject, IoIrp );

                if (Status == STATUS_PENDING) {
                    (VOID)KeWaitForSingleObject( &IoEvent, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );
                    Status = Iosb.Status;
                }

                if (!NT_SUCCESS(Status)) {
                    FatNormalizeAndRaiseStatus( IrpContext, Status );
                }
            }

            //
            //  Now that the file data has been moved successfully, we'll go
            //  to fix up the links in the FAT table and perhaps change the
            //  entry in the parent directory.
            //
            //  First we'll do the second splice and commit it.  At that point,
            //  while the volume is in an inconsistent state, the file is
            //  still OK.
            //

            FatSetFatEntry( IrpContext,
                            Vcb,
                            SecondSpliceSourceCluster,
                            (FAT_ENTRY)SecondSpliceTargetCluster );

            FatFlushFatEntries( IrpContext, Vcb, SecondSpliceSourceCluster, 1 );

            //
            //  Now do the first splice OR update the dirent in the parent
            //  and flush the respective object.  After this flush the file
            //  now points to the new allocation.
            //

            if (FirstSpliceSourceCluster == 0) {

                ASSERT( NodeType(FcbOrDcb) == FAT_NTC_FCB );

                //
                //  We are moving the first cluster of the file, so we need
                //  to update our parent directory.
                //

                FatGetDirentFromFcbOrDcb( IrpContext, FcbOrDcb, &Dirent, &DirentBcb );
                Dirent->FirstClusterOfFile = (USHORT)FirstSpliceTargetCluster;

                if (FatIsFat32(Vcb)) {

                    Dirent->FirstClusterOfFileHi =
                        (USHORT)(FirstSpliceTargetCluster >> 16);

                }

                FatSetDirtyBcb( IrpContext, DirentBcb, Vcb, TRUE );

                FatUnpinBcb( IrpContext, DirentBcb );
                DirentBcb = NULL;

                FatFlushDirentForFile( IrpContext, FcbOrDcb );

                FcbOrDcb->FirstClusterOfFile = FirstSpliceTargetCluster;

            } else {

                FatSetFatEntry( IrpContext,
                                Vcb,
                                FirstSpliceSourceCluster,
                                (FAT_ENTRY)FirstSpliceTargetCluster );

                FatFlushFatEntries( IrpContext, Vcb, FirstSpliceSourceCluster, 1 );
            }

            //
            //  This was successfully committed.  We no longer want to free
            //  this allocation on error.
            //

            DiskSpaceAllocated = FALSE;

            //
            //  Now we just have to free the orphaned space.  We don't have
            //  to commit this right now as the integrity of the file doesn't
            //  depend on it.
            //

            FatDeallocateDiskSpace( IrpContext, Vcb, &SourceMcb );

            FatUnpinRepinnedBcbs( IrpContext );

            Status = FatHijackIrpAndFlushDevice( IrpContext,
                                                 Irp,
                                                 Vcb->TargetDeviceObject );

            if (!NT_SUCCESS(Status)) {
                FatNormalizeAndRaiseStatus( IrpContext, Status );
            }

            //
            //  Finally we must replace the old MCB extent information with
            //  the new.  If this fails from pool allocation, we fix it in
            //  the finally clause by resetting the file's Mcb.
            //

            FatRemoveMcbEntry( Vcb, &FcbOrDcb->Mcb,
                               FileOffset,
                               BytesToReallocate );

            FatAddMcbEntry( Vcb, &FcbOrDcb->Mcb,
                            FileOffset,
                            TargetLbo,
                            BytesToReallocate );

            //
            //  Now this is the second part of the tricky synchronization.
            //
            //  We drop the paging I/O here and signal the notification
            //  event which allows all waiters (present or future) to proceed.
            //  Then we block again on the PagingIo exclusive.  When
            //  we have it, we again know that there can be nobody in the
            //  read/write path and thus nobody touching the event, so we
            //  NULL the pointer to it and then drop the PagingIo resource.
            //
            //  This combined with our synchronization before the write above
            //  guarantees that while we were moving the allocation, there
            //  was no other I/O to this file and because we do not hold
            //  the paging resource across a flush, we are not exposed to
            //  a deadlock.
            //

            KeSetEvent( &StackEvent, 0, FALSE );

            ExAcquireResourceExclusiveLite( FcbOrDcb->Header.PagingIoResource, TRUE );

            FcbOrDcb->MoveFileEvent = NULL;
            EventArmed = FALSE;

            ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );

            //
            //  Release the resources and let anyone else access the file before
            //  looping back.
            //

            FatReleaseFcb( IrpContext, FcbOrDcb );
            FcbAcquired = FALSE;

            //
            //  Advance the state variables.
            //

            TargetCluster += BytesToReallocate >> ClusterShift;

            FileOffset += BytesToReallocate;
            TargetLbo += BytesToReallocate;
            ByteCount -= BytesToReallocate;

            LargeTargetLbo.QuadPart += BytesToReallocate;

            //
            //  Clear the two Mcbs
            //

            FatRemoveMcbEntry( Vcb, &SourceMcb, 0, 0xFFFFFFFF );
            FatRemoveMcbEntry( Vcb, &TargetMcb, 0, 0xFFFFFFFF );

            //
            //  Make the event blockable again.
            //

            KeClearEvent( &StackEvent );
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

    } _SEH2_FINALLY {

        DebugUnwind( FatMoveFile );

        LocalAbnormalTermination |= _SEH2_AbnormalTermination();

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_PARENT_BY_CHILD );

        //
        //  Free the data buffer, if it was allocated.
        //

        if (Buffer != NULL) {

            ExFreePool( Buffer );
        }

        //
        //  Use a nested try-finally for cleanup if our unpinrepinned
        //  encounters write-through errors.  This may even be a re-raise.
        //

        _SEH2_TRY {

            //
            //  If we have some new allocation hanging around, remove it.  The
            //  pages needed to do this are guaranteed to be resident because
            //  we have already repinned them.
            //

            if (DiskSpaceAllocated) {
                FatDeallocateDiskSpace( IrpContext, Vcb, &TargetMcb );
                FatUnpinRepinnedBcbs( IrpContext );
            }

        } _SEH2_FINALLY {

            LocalAbnormalTermination |= _SEH2_AbnormalTermination();

            //
            //  Check on the directory Bcb
            //

            if (DirentBcb != NULL) {
                FatUnpinBcb( IrpContext, DirentBcb );
            }

            //
            //  Uninitialize our MCBs
            //

            if (SourceMcbInitialized) {
                FsRtlUninitializeLargeMcb( &SourceMcb );
            }

            if (TargetMcbInitialized) {
                FsRtlUninitializeLargeMcb( &TargetMcb );
            }

            //
            //  If this is an abnormal termination then presumably something
            //  bad happened.  Set the Allocation size to unknown and clear
            //  the Mcb, but only if we still own the Fcb.
            //
            //  It is important to make sure we use a 64bit form of -1.  This is
            //  what will convince the fastIO path that it cannot extend the file
            //  in the cache until we have picked up the mapping pairs again.
            //
            //  Also, we have to do this while owning PagingIo or we can tear the
            //  Mcb down in the midst of the noncached IO path looking up extents
            //  (after we drop it and let them all in).
            //

            if (LocalAbnormalTermination && FcbAcquired) {

                if (FcbOrDcb->FirstClusterOfFile == 0) {
                    
                    FcbOrDcb->Header.AllocationSize.QuadPart = 0;
                } else {

                    FcbOrDcb->Header.AllocationSize.QuadPart = FCB_LOOKUP_ALLOCATIONSIZE_HINT;
                }

                FatRemoveMcbEntry( Vcb, &FcbOrDcb->Mcb, 0, 0xFFFFFFFF );
            }

            //
            //  If we broke out of the loop with the Event armed, defuse it
            //  in the same way we do it after a write.
            //

            if (EventArmed) {
                KeSetEvent( &StackEvent, 0, FALSE );
                ExAcquireResourceExclusiveLite( FcbOrDcb->Header.PagingIoResource, TRUE );
                FcbOrDcb->MoveFileEvent = NULL;
                ExReleaseResourceLite( FcbOrDcb->Header.PagingIoResource );
            }

            //
            //  Finally release the main file resource.
            //

            if (FcbAcquired) {
                FatReleaseFcb( IrpContext, FcbOrDcb );
            }

            //
            //  Now dereference the fileobject.  If the user was a wacko they could have
            //  tried to nail us by closing the handle right after they threw this move
            //  down, so we had to keep the fileobject referenced across the entire
            //  operation.
            //

            ObDereferenceObject( FileObject );

        } _SEH2_END;
    } _SEH2_END;

    //
    //  Complete the irp if we terminated normally.
    //

    FatCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

VOID
FatComputeMoveFileParameter (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN ULONG BufferSize,
    IN ULONG FileOffset,
    IN OUT PULONG ByteCount,
    OUT PULONG BytesToReallocate,
    OUT PULONG BytesToWrite,
    OUT PLARGE_INTEGER SourceLbo
)

/*++

Routine Description:

    This is a helper routine for FatMoveFile that analyses the range of
    file allocation we are moving and determines the actual amount
    of allocation to be moved and how much needs to be written.

Arguments:

    FcbOrDcb - Supplies the file and its various sizes.

    BufferSize - Supplies the size of the buffer we are using to store the data
                 being moved.

    FileOffset - Supplies the beginning Vbo of the reallocation zone.

    ByteCount - Supplies the request length to reallocate.  This will
        be bounded by allocation size on return.

    BytesToReallocate - Receives ByteCount bounded by the file allocation size
        and buffer size.

    BytesToWrite - Receives BytesToReallocate bounded by ValidDataLength.

    SourceLbo - Receives the logical byte offset of the source data on the volume.

Return Value:

    VOID

--*/

{
    ULONG ClusterSize;

    ULONG AllocationSize;
    ULONG ValidDataLength;
    ULONG ClusterAlignedVDL;
    LBO RunLbo;
    ULONG RunByteCount;
    ULONG RunIndex;
    BOOLEAN RunAllocated;
    BOOLEAN RunEndOnMax;


    //
    //  If we haven't yet set the correct AllocationSize, do so.
    //

    if (FcbOrDcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

        FatLookupFileAllocationSize( IrpContext, FcbOrDcb );

        //
        //  If this is a non-root directory, we have a bit more to
        //  do since it has not gone through FatOpenDirectoryFile().
        //

        if (NodeType(FcbOrDcb) == FAT_NTC_DCB ||
            (NodeType(FcbOrDcb) == FAT_NTC_ROOT_DCB && FatIsFat32(FcbOrDcb->Vcb))) {

            FcbOrDcb->Header.FileSize.LowPart =
                FcbOrDcb->Header.AllocationSize.LowPart;
        }
    }

    //
    //  Get the number of bytes left to write and ensure that it does
    //  not extend beyond allocation size.  We return here if FileOffset
    //  is beyond AllocationSize which can happn on a truncation.
    //

    AllocationSize = FcbOrDcb->Header.AllocationSize.LowPart;
    ValidDataLength = FcbOrDcb->Header.ValidDataLength.LowPart;

    if (FileOffset + *ByteCount > AllocationSize) {

        if (FileOffset >= AllocationSize) {
            *ByteCount = 0;
            *BytesToReallocate = 0;
            *BytesToWrite = 0;

            return;
        }

        *ByteCount = AllocationSize - FileOffset;
    }

    //
    //  If there is more than our max, then reduce the byte count for this
    //  pass to our maximum. We must also align the file offset to a 
    //  buffer size byte boundary.
    //

    if ((FileOffset & (BufferSize - 1)) + *ByteCount > BufferSize) {

        *BytesToReallocate = BufferSize - (FileOffset & (BufferSize - 1));

    } else {

        *BytesToReallocate = *ByteCount;
    }

    //
    //  Find where this data exists on the volume.
    //

    FatLookupFileAllocation( IrpContext,
                             FcbOrDcb,
                             FileOffset,
                             &RunLbo,
                             &RunByteCount,
                             &RunAllocated,
                             &RunEndOnMax,
                             &RunIndex );

    ASSERT( RunAllocated );

    //
    //  Limit this run to the contiguous length.
    //

    if (RunByteCount < *BytesToReallocate) {

        *BytesToReallocate = RunByteCount;
    }

    //
    //  Set the starting offset of the source.
    //

    SourceLbo->QuadPart = RunLbo;

    //
    //  We may be able to skip some (or all) of the write
    //  if allocation size is significantly greater than valid data length.
    //

    ClusterSize = 1 << FcbOrDcb->Vcb->AllocationSupport.LogOfBytesPerCluster;

    ASSERT( ClusterSize <= BufferSize );

    ClusterAlignedVDL = (ValidDataLength + (ClusterSize - 1)) & ~(ClusterSize - 1);

    if ((NodeType(FcbOrDcb) == FAT_NTC_FCB) &&
        (FileOffset + *BytesToReallocate > ClusterAlignedVDL)) {

        if (FileOffset > ClusterAlignedVDL) {

            *BytesToWrite = 0;

        } else {

            *BytesToWrite = ClusterAlignedVDL - FileOffset;
        }

    } else {

        *BytesToWrite = *BytesToReallocate;
    }
}


//
//  Local Support Routine
//

VOID
FatComputeMoveFileSplicePoints (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN ULONG FileOffset,
    IN ULONG TargetCluster,
    IN ULONG BytesToReallocate,
    OUT PULONG FirstSpliceSourceCluster,
    OUT PULONG FirstSpliceTargetCluster,
    OUT PULONG SecondSpliceSourceCluster,
    OUT PULONG SecondSpliceTargetCluster,
    IN OUT PLARGE_MCB SourceMcb
)

/*++

Routine Description:

    This is a helper routine for FatMoveFile that analyzes the range of
    file allocation we are moving and generates the splice points in the
    FAT table.

Arguments:

    FcbOrDcb - Supplies the file and thus Mcb.

    FileOffset - Supplies the beginning Vbo of the reallocation zone.

    TargetCluster - Supplies the beginning cluster of the reallocation target.

    BytesToReallocate - Suppies the length of the reallocation zone.

    FirstSpliceSourceCluster - Receives the last cluster in previous allocation
        or zero if we are reallocating from VBO 0.

    FirstSpliceTargetCluster - Receives the target cluster (i.e. new allocation)

    SecondSpliceSourceCluster - Receives the final target cluster.

    SecondSpliceTargetCluster - Receives the first cluster of the remaining
        source allocation or FAT_CLUSTER_LAST if the reallocation zone
        extends to the end of the file.

    SourceMcb - This supplies an MCB that will be filled in with run
        information describing the file allocation being replaced.  The Mcb
        must be initialized by the caller.

Return Value:

    VOID

--*/

{
    VBO SourceVbo;
    LBO SourceLbo;
    ULONG SourceIndex;
    ULONG SourceBytesInRun;
    ULONG SourceBytesRemaining;

    ULONG SourceMcbVbo;
    ULONG SourceMcbBytesInRun;

    PVCB Vcb;

    Vcb = FcbOrDcb->Vcb;

    //
    //  Get information on the final cluster in the previous allocation and
    //  prepare to enumerate it in the follow loop.
    //

    if (FileOffset == 0) {

        SourceIndex = 0;
        *FirstSpliceSourceCluster = 0;
        FatGetNextMcbEntry( Vcb, &FcbOrDcb->Mcb,
                            0,
                            &SourceVbo,
                            &SourceLbo,
                            &SourceBytesInRun );

    } else {

        FatLookupMcbEntry( Vcb, &FcbOrDcb->Mcb,
                           FileOffset-1,
                           &SourceLbo,
                           &SourceBytesInRun,
                           &SourceIndex);

        *FirstSpliceSourceCluster = FatGetIndexFromLbo( Vcb, SourceLbo );

        if (SourceBytesInRun == 1) {

            SourceIndex += 1;
            FatGetNextMcbEntry( Vcb, &FcbOrDcb->Mcb,
                                SourceIndex,
                                &SourceVbo,
                                &SourceLbo,
                                &SourceBytesInRun);

        } else {

            SourceVbo = FileOffset;
            SourceLbo += 1;
            SourceBytesInRun -= 1;
        }
    }

    //
    //  At this point the variables:
    //
    //  - SourceIndex - SourceLbo - SourceBytesInRun -
    //
    //  all correctly decribe the allocation to be removed.  In the loop
    //  below we will start here and continue enumerating the Mcb runs
    //  until we are finished with the allocation to be relocated.
    //

    *FirstSpliceTargetCluster = TargetCluster;

    *SecondSpliceSourceCluster =
         *FirstSpliceTargetCluster +
         (BytesToReallocate >> Vcb->AllocationSupport.LogOfBytesPerCluster) - 1;

    for (SourceBytesRemaining = BytesToReallocate, SourceMcbVbo = 0;

         SourceBytesRemaining > 0;

         SourceIndex += 1,
         SourceBytesRemaining -= SourceMcbBytesInRun,
         SourceMcbVbo += SourceMcbBytesInRun) {

        if (SourceMcbVbo != 0) {
            FatGetNextMcbEntry( Vcb, &FcbOrDcb->Mcb,
                                SourceIndex,
                                &SourceVbo,
                                &SourceLbo,
                                &SourceBytesInRun );
        }

        ASSERT( SourceVbo == SourceMcbVbo + FileOffset );

        SourceMcbBytesInRun =
            SourceBytesInRun < SourceBytesRemaining ?
            SourceBytesInRun : SourceBytesRemaining;

        FatAddMcbEntry( Vcb, SourceMcb,
                        SourceMcbVbo,
                        SourceLbo,
                        SourceMcbBytesInRun );
    }

    //
    //  Now compute the cluster of the target of the second
    //  splice.  If the final run in the above loop was
    //  more than we needed, then we can just do arithmetic,
    //  otherwise we have to look up the next run.
    //

    if (SourceMcbBytesInRun < SourceBytesInRun) {

        *SecondSpliceTargetCluster =
            FatGetIndexFromLbo( Vcb, SourceLbo + SourceMcbBytesInRun );

    } else {

        if (FatGetNextMcbEntry( Vcb, &FcbOrDcb->Mcb,
                                SourceIndex,
                                &SourceVbo,
                                &SourceLbo,
                                &SourceBytesInRun )) {

            *SecondSpliceTargetCluster = FatGetIndexFromLbo( Vcb, SourceLbo );

        } else {

            *SecondSpliceTargetCluster = FAT_CLUSTER_LAST;
        }
    }
}


NTSTATUS
FatAllowExtendedDasdIo(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine marks the CCB to indicate that the handle
    may be used to read past the end of the volume file.  The
    handle must be a dasd handle.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if ((Ccb == NULL) || !FlagOn( Ccb->Flags, CCB_FLAG_MANAGE_VOLUME_ACCESS )) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatAllowExtendedDasdIo -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    SetFlag( Ccb->Flags, CCB_FLAG_ALLOW_EXTENDED_DASD_IO );

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}



VOID
FatFlushAndCleanVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN FAT_FLUSH_TYPE FlushType
    )
/*++

Routine Description:

    This routine flushes and otherwise preparse a volume to be eligible
    for deletion.  The dismount and PNP paths share the need for this
    common work.

    The Vcb will always be valid on return from this function. It is the
    caller's responsibility to attempt the dismount/deletion, and to setup
    allocation support again if the volume will be brought back from the
    brink.

Arguments:

    Irp - Irp for the overlying request

    Vcb - the volume being operated on

    FlushType - specifies the kind of flushing desired

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    //
    //  The volume must be held exclusive.
    //

    ASSERT( FatVcbAcquiredExclusive( IrpContext, Vcb ));

    //
    //  There is no fail, flush everything. If invalidating, it is important
    //  that we invalidate as we flush (eventually, w/ paging io held) so that we
    //  error out the maximum number of late writes.
    //

    if (FlushType != NoFlush) {

        (VOID) FatFlushVolume( IrpContext, Vcb, FlushType );
    }

    FatCloseEaFile( IrpContext, Vcb, FALSE );

    //
    //  Now, tell the device to flush its buffers.
    //

    if (FlushType != NoFlush) {

        (VOID)FatHijackIrpAndFlushDevice( IrpContext, Irp, Vcb->TargetDeviceObject );
    }

    //
    //  Now purge everything in sight.  We're trying to provoke as many closes as
    //  soon as possible, this volume may be on its way out.
    //

    if (FlushType != FlushWithoutPurge) {
        
        (VOID) FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, NoFlush );
    }

    //
    //  If the volume was dirty and we were allowed to flush, do the processing that
    //  the delayed callback would have done.
    //

    if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY)) {

        //
        //  Cancel any pending clean volumes.
        //

        (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );
        (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );


        if (FlushType != NoFlush) {

            //
            //  The volume is now clean, note it.
            //

            if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                FatMarkVolume( IrpContext, Vcb, VolumeClean );
                ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );
            }

            //
            //  Unlock the volume if it is removable.
            //

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
                !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE)) {

                FatToggleMediaEjectDisable( IrpContext, Vcb, FALSE );
            }
        }
    }
 }


NTSTATUS
FatSearchBufferForLabel(
    IN  PIRP_CONTEXT IrpContext,
    IN  PVPB  Vpb,
    IN  PVOID Buffer,
    IN  ULONG Size,
    OUT PBOOLEAN LabelFound
    )
/*++

Routine Description:

    Search a buffer (taken from the root directory) for a volume label
    matching the label in the

Arguments:

    IrpContext - Supplies our irp context
    Vpb        - Vpb supplying the volume label
    Buffer     - Supplies the buffer we'll search
    Size       - The size of the buffer in bytes.
    LabelFound - Returns whether a label was found.

Return Value:

    There are four interesting cases:

    1) Some random error occurred - that error returned as status, LabelFound
                                    is indeterminate.

    2) No label was found         - STATUS_SUCCESS returned, LabelFound is FALSE.

    3) A matching label was found - STATUS_SUCCESS returned, LabelFound is TRUE.

    4) A non-matching label found - STATUS_WRONG_VOLUME returned, LabelFound
                                    is indeterminate.

--*/

{
    NTSTATUS Status;
    WCHAR UnicodeBuffer[11];

    PDIRENT Dirent;
    PDIRENT TerminationDirent;
    ULONG VolumeLabelLength;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;

    Dirent = Buffer;

    TerminationDirent = Dirent + Size / sizeof(DIRENT);

    while ( Dirent < TerminationDirent ) {

        if ( Dirent->FileName[0] == FAT_DIRENT_NEVER_USED ) {

            Dirent = TerminationDirent;
            break;
        }

        //
        //  If the entry is the non-deleted volume label break from the loop.
        //
        //  Note that all out parameters are already correctly set.
        //

        if (((Dirent->Attributes & ~FAT_DIRENT_ATTR_ARCHIVE) ==
             FAT_DIRENT_ATTR_VOLUME_ID) &&
            (Dirent->FileName[0] != FAT_DIRENT_DELETED)) {

            break;
        }

        Dirent += 1;
    }

    if (Dirent >= TerminationDirent) {

        //
        //  We've run out of buffer.
        //

        *LabelFound = FALSE;
        return STATUS_SUCCESS;
    }


    //
    //  Compute the length of the volume name
    //

#ifndef __REACTOS__
    OemString.Buffer = &Dirent->FileName[0];
#else
    OemString.Buffer = (PCHAR)&Dirent->FileName[0];
#endif
    OemString.MaximumLength = 11;

    for ( OemString.Length = 11;
          OemString.Length > 0;
          OemString.Length -= 1) {

        if ( (Dirent->FileName[OemString.Length-1] != 0x00) &&
             (Dirent->FileName[OemString.Length-1] != 0x20) ) { break; }
    }

    UnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
    UnicodeString.Buffer = &UnicodeBuffer[0];

    Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                 &OemString,
                                                 FALSE );

    if ( !NT_SUCCESS( Status ) ) {

        return Status;
    }

    VolumeLabelLength = UnicodeString.Length;

    if ( (VolumeLabelLength != (ULONG)Vpb->VolumeLabelLength) ||
         (!RtlEqualMemory(&UnicodeBuffer[0],
                          &Vpb->VolumeLabel[0],
                          VolumeLabelLength)) ) {

        return STATUS_WRONG_VOLUME;
    }

    //
    //  We found a matching label.
    //

    *LabelFound = TRUE;
    return STATUS_SUCCESS;
}


VOID
FatVerifyLookupFatEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FatIndex,
    IN OUT PULONG FatEntry
    )
{
    ULONG PageEntryOffset;
    ULONG OffsetIntoVolumeFile;
    PVOID Buffer;

    ASSERT(Vcb->AllocationSupport.FatIndexBitSize == 32);

    FatVerifyIndexIsValid( IrpContext, Vcb, FatIndex);

    Buffer = FsRtlAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                       PAGE_SIZE,
                                       TAG_ENTRY_LOOKUP_BUFFER );

    OffsetIntoVolumeFile =  FatReservedBytes(&Vcb->Bpb) + FatIndex * sizeof(ULONG);
    PageEntryOffset = (OffsetIntoVolumeFile % PAGE_SIZE) / sizeof(ULONG);

    _SEH2_TRY {

        FatPerformVerifyDiskRead( IrpContext,
                                  Vcb,
                                  Buffer,
                                  OffsetIntoVolumeFile & ~(PAGE_SIZE - 1),
                                  PAGE_SIZE,
                                  TRUE );

        *FatEntry = ((PULONG)(Buffer))[PageEntryOffset];

    } _SEH2_FINALLY {

        ExFreePool( Buffer );
    } _SEH2_END;
}

//
//  Local support routine
//

VOID
FatScanForDismountedVcb (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine walks through the list of Vcb's looking for any which may
    now be deleted.  They may have been left on the list because there were
    outstanding references.

Arguments:

Return Value:

    None

--*/

{
    PVCB Vcb;
    PLIST_ENTRY Links;
    BOOLEAN VcbDeleted;

    PAGED_CODE();

    //
    //  Walk through all of the Vcb's attached to the global data.
    //

    FatAcquireExclusiveGlobal( IrpContext );

    Links = FatData.VcbQueue.Flink;

    while (Links != &FatData.VcbQueue) {

        Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks );

        //
        //  Move to the next link now since the current Vcb may be deleted.
        //

        Links = Links->Flink;

        //
        //  Try to acquire the VCB for exclusive access.  If we cannot, just skip
        //  it for now.
        //

        if (!ExAcquireResourceExclusiveLite( &(Vcb->Resource), FALSE )) {
            
            continue;
        }

        //
        //  Check if this Vcb can go away.
        //

        VcbDeleted = FatCheckForDismount ( IrpContext,
                                           Vcb,
                                           FALSE );

        //
        //  If the VCB was not deleted, release it.
        //

        if (!VcbDeleted) {
            
            ExReleaseResourceLite( &(Vcb->Resource) );
        }
    }

    FatReleaseGlobal( IrpContext );

    return;
}


