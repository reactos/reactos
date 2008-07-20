/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Cdfs in-memory data structure manipulation
    routines


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_STRUCSUP)

//
//  Local macros
//

//
//  PFCB
//  CdAllocateFcbData (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdDeallocateFcbData (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  PFCB
//  CdAllocateFcbIndex (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdDeallocateFcbIndex (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  PFCB_NONPAGED
//  CdAllocateFcbNonpaged (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdDeallocateFcbNonpaged (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB_NONPAGED FcbNonpaged
//      );
//
//  PCCB
//  CdAllocateCcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdDeallocateCcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCCB Ccb
//      );
//

#define CdAllocateFcbData(IC) \
    FsRtlAllocatePoolWithTag( CdPagedPool, SIZEOF_FCB_DATA, TAG_FCB_DATA )

#define CdDeallocateFcbData(IC,F) \
    CdFreePool( &(F) )

#define CdAllocateFcbIndex(IC) \
    FsRtlAllocatePoolWithTag( CdPagedPool, SIZEOF_FCB_INDEX, TAG_FCB_INDEX )

#define CdDeallocateFcbIndex(IC,F) \
    CdFreePool( &(F) )

#define CdAllocateFcbNonpaged(IC) \
    ExAllocatePoolWithTag( CdNonPagedPool, sizeof( FCB_NONPAGED ), TAG_FCB_NONPAGED )

#define CdDeallocateFcbNonpaged(IC,FNP) \
    CdFreePool( &(FNP) )

#define CdAllocateCcb(IC) \
    FsRtlAllocatePoolWithTag( CdPagedPool, sizeof( CCB ), TAG_CCB )

#define CdDeallocateCcb(IC,C) \
    CdFreePool( &(C) )

//
//  Local structures
//

typedef struct _FCB_TABLE_ELEMENT {

    FILE_ID FileId;
    PFCB Fcb;

} FCB_TABLE_ELEMENT, *PFCB_TABLE_ELEMENT;

//
//  Local macros
//

//
//  VOID
//  CdInsertFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdDeleteFcbTable (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//


#define CdInsertFcbTable(IC,F) {                                    \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.Fcb = (F);                                                 \
    _Key.FileId = (F)->FileId;                                      \
    RtlInsertElementGenericTable( &(F)->Vcb->FcbTable,              \
                                  &_Key,                            \
                                  sizeof( FCB_TABLE_ELEMENT ),      \
                                  NULL );                           \
}

#define CdDeleteFcbTable(IC,F) {                                    \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.FileId = (F)->FileId;                                      \
    RtlDeleteElementGenericTable( &(F)->Vcb->FcbTable, &_Key );     \
}

//
//  Local support routines
//

VOID
CdDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PFCB_NONPAGED
CdCreateFcbNonpaged (
    IN PIRP_CONTEXT IrpContext
    );

VOID
CdDeleteFcbNonpaged (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB_NONPAGED FcbNonpaged
    );

RTL_GENERIC_COMPARE_RESULTS
CdFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Fid1,
    IN PVOID Fid2
    );

PVOID
CdAllocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    );

VOID
CdDeallocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    );

ULONG
CdTocSerial (
    IN PIRP_CONTEXT IrpContext,
    IN PCDROM_TOC CdromToc
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdAllocateFcbTable)
#pragma alloc_text(PAGE, CdCleanupIrpContext)
#pragma alloc_text(PAGE, CdCreateCcb)
#pragma alloc_text(PAGE, CdCreateFcb)
#pragma alloc_text(PAGE, CdCreateFcbNonpaged)
#pragma alloc_text(PAGE, CdCreateFileLock)
#pragma alloc_text(PAGE, CdCreateIrpContext)
#pragma alloc_text(PAGE, CdDeallocateFcbTable)
#pragma alloc_text(PAGE, CdDeleteCcb)
#pragma alloc_text(PAGE, CdDeleteFcb)
#pragma alloc_text(PAGE, CdDeleteFcbNonpaged)
#pragma alloc_text(PAGE, CdDeleteFileLock)
#pragma alloc_text(PAGE, CdDeleteVcb)
#pragma alloc_text(PAGE, CdFcbTableCompare)
#pragma alloc_text(PAGE, CdGetNextFcb)
#pragma alloc_text(PAGE, CdInitializeFcbFromFileContext)
#pragma alloc_text(PAGE, CdInitializeFcbFromPathEntry)
#pragma alloc_text(PAGE, CdInitializeStackIrpContext)
#pragma alloc_text(PAGE, CdInitializeVcb)
#pragma alloc_text(PAGE, CdLookupFcbTable)
#pragma alloc_text(PAGE, CdProcessToc)
#pragma alloc_text(PAGE, CdTeardownStructures)
#pragma alloc_text(PAGE, CdTocSerial)
#pragma alloc_text(PAGE, CdUpdateVcbFromVolDescriptor)
#endif


VOID
CdInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PCDROM_TOC CdromToc,
    IN ULONG TocLength,
    IN ULONG TocTrackCount,
    IN ULONG TocDiskFlags,
    IN ULONG BlockFactor,
    IN ULONG MediaChangeCount
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

    CdromToc - Buffer to hold table of contents.  NULL if TOC command not
        supported.

    TocLength - Byte count length of TOC.  We use this as the TOC length to
        return on a user query.

    TocTrackCount - Count of tracks in TOC.  Used to create pseudo files for
        audio disks.

    TocDiskFlags - Flag field to indicate the type of tracks on the disk.

    BlockFactor - Used to decode any multi-session information.

    MediaChangeCount - Initial media change count of the target device

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean.
    //

    RtlZeroMemory( Vcb, sizeof( VCB ));

    //
    //  Set the proper node type code and node byte size.
    //

    Vcb->NodeTypeCode = CDFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof( VCB );

    //
    //  Initialize the DirNotify structs.  FsRtlNotifyInitializeSync can raise.
    //

    InitializeListHead( &Vcb->DirNotifyList );
    FsRtlNotifyInitializeSync( &Vcb->NotifySync );
    
    //
    //  Pick up a VPB right now so we know we can pull this filesystem stack
    //  off of the storage stack on demand.  This can raise - if it does,  
    //  uninitialize the notify structures before returning.
    //
    
    try  {

        Vcb->SwapVpb = FsRtlAllocatePoolWithTag( NonPagedPool,
                                                 sizeof( VPB ),
                                                 TAG_VPB );
    }
    finally {

        if (AbnormalTermination())  {
        
            FsRtlNotifyUninitializeSync( &Vcb->NotifySync );
        }
    }

    //
    //  Nothing beyond this point should raise.
    //

    RtlZeroMemory( Vcb->SwapVpb, sizeof( VPB ) );
    
    //
    //  Initialize the resource variable for the Vcb and files.
    //

    ExInitializeResourceLite( &Vcb->VcbResource );
    ExInitializeResourceLite( &Vcb->FileResource );
    ExInitializeFastMutex( &Vcb->VcbMutex );

    //
    //  Insert this Vcb record on the CdData.VcbQueue.
    //

    InsertHeadList( &CdData.VcbQueue, &Vcb->VcbLinks );

    //
    //  Set the Target Device Object and Vpb fields, referencing the
    //  Target device for the mount.
    //

    ObReferenceObject( TargetDeviceObject );
    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Set the removable media flag based on the real device's
    //  characteristics
    //

    if (FlagOn( Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA )) {

        SetFlag( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA );
    }

    //
    //  Initialize the generic Fcb Table.
    //

    RtlInitializeGenericTable( &Vcb->FcbTable,
                               (PRTL_GENERIC_COMPARE_ROUTINE) CdFcbTableCompare,
                               (PRTL_GENERIC_ALLOCATE_ROUTINE) CdAllocateFcbTable,
                               (PRTL_GENERIC_FREE_ROUTINE) CdDeallocateFcbTable,
                               NULL );

    //
    //  Show that we have a mount in progress.
    //

    CdUpdateVcbCondition( Vcb, VcbMountInProgress);

    //
    //  Refererence the Vcb for two reasons.  The first is a reference
    //  that prevents the Vcb from going away on the last close unless
    //  dismount has already occurred.  The second is to make sure
    //  we don't go into the dismount path on any error during mount
    //  until we get to the Mount cleanup.
    //

    Vcb->VcbReference = 1 + CDFS_RESIDUAL_REFERENCE;

    //
    //  Update the TOC information in the Vcb.
    //

    Vcb->CdromToc = CdromToc;
    Vcb->TocLength = TocLength;
    Vcb->TrackCount = TocTrackCount;
    Vcb->DiskFlags = TocDiskFlags;

    //
    //  If this disk contains audio tracks only then set the audio flag.
    //

    if (TocDiskFlags == CDROM_DISK_AUDIO_TRACK) {

        SetFlag( Vcb->VcbState, VCB_STATE_AUDIO_DISK | VCB_STATE_CDXA );
    }

    //
    //  Set the block factor.
    //

    Vcb->BlockFactor = BlockFactor;

    //
    //  Set the media change count on the device
    //

    CdUpdateMediaChangeCount( Vcb, MediaChangeCount);
}


VOID
CdUpdateVcbFromVolDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PCHAR RawIsoVd OPTIONAL
    )

/*++

Routine Description:

    This routine is called to perform the final initialization of a Vcb from the
    volume descriptor on the disk.

Arguments:

    Vcb - Vcb for the volume being mounted.  We have already set the flags for the
        type of descriptor.

    RawIsoVd - If specified this is the volume descriptor to use to mount the
        volume.  Not specified for a raw disk.

Return Value:

    None

--*/

{
    ULONG Shift;
    ULONG StartingBlock;
    ULONG ByteCount;

    LONGLONG FileId = 0;

    PRAW_DIRENT RawDirent;
    PATH_ENTRY PathEntry;
    PCD_MCB_ENTRY McbEntry;

    BOOLEAN UnlockVcb = FALSE;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Copy the block size and compute the various block masks.
        //  Block size must not be larger than the sector size.  We will
        //  use a default of the CD physical sector size if we are not
        //  on a data-full disc.
        //
        //  This must always be set.
        //

        Vcb->BlockSize = ( ARGUMENT_PRESENT( RawIsoVd ) ?
                            CdRvdBlkSz( RawIsoVd, Vcb->VcbState ) :
                            SECTOR_SIZE );

        //
        //  We no longer accept media where blocksize != sector size.
        //
        
        if (Vcb->BlockSize != SECTOR_SIZE)  {

            CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }

        Vcb->BlocksPerSector = SECTOR_SIZE / Vcb->BlockSize;
        Vcb->BlockMask = Vcb->BlockSize - 1;
        Vcb->BlockInverseMask = ~Vcb->BlockMask;
     
        Vcb->BlockToSectorShift = 0;
        Vcb->BlockToByteShift = SECTOR_SHIFT;

        //
        //  If there is a volume descriptor then do the internal Fcb's and
        //  other Vcb fields.
        //

        if (ARGUMENT_PRESENT( RawIsoVd )) {

            //
            //  Create the path table Fcb and refererence it and the Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;

            Vcb->PathTableFcb = CdCreateFcb( IrpContext,
                                             *((PFILE_ID) &FileId),
                                             CDFS_NTC_FCB_PATH_TABLE,
                                             NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->PathTableFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  Compute the stream offset and size of this path table.
            //

            StartingBlock = CdRvdPtLoc( RawIsoVd, Vcb->VcbState );

            ByteCount = CdRvdPtSz( RawIsoVd, Vcb->VcbState );

            Vcb->PathTableFcb->StreamOffset = BytesFromBlocks( Vcb,
                                                               SectorBlockOffset( Vcb, StartingBlock ));

            Vcb->PathTableFcb->FileSize.QuadPart = (LONGLONG) (Vcb->PathTableFcb->StreamOffset +
                                                               ByteCount);

            Vcb->PathTableFcb->ValidDataLength.QuadPart = Vcb->PathTableFcb->FileSize.QuadPart;

            Vcb->PathTableFcb->AllocationSize.QuadPart = LlSectorAlign( Vcb->PathTableFcb->FileSize.QuadPart );

            //
            //  Now add the mapping information.
            //

            CdLockFcb( IrpContext, Vcb->PathTableFcb );

            CdAddInitialAllocation( IrpContext,
                                    Vcb->PathTableFcb,
                                    StartingBlock,
                                    Vcb->PathTableFcb->AllocationSize.QuadPart );

            CdUnlockFcb( IrpContext, Vcb->PathTableFcb );

            //
            //  Point to the file resource.
            //

            Vcb->PathTableFcb->Resource = &Vcb->FileResource;

            //
            //  Mark the Fcb as initialized and create the stream file for this.
            //

            SetFlag( Vcb->PathTableFcb->FcbState, FCB_STATE_INITIALIZED );

            CdCreateInternalStream( IrpContext, Vcb, Vcb->PathTableFcb );

            //
            //  Create the root index and reference it in the Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;
            Vcb->RootIndexFcb = CdCreateFcb( IrpContext,
                                             *((PFILE_ID) &FileId),
                                             CDFS_NTC_FCB_INDEX,
                                             NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->RootIndexFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  Create the File id by hand for this Fcb.
            //

            CdSetFidPathTableOffset( Vcb->RootIndexFcb->FileId, Vcb->PathTableFcb->StreamOffset );
            CdFidSetDirectory( Vcb->RootIndexFcb->FileId );

            //
            //  Create a pseudo path table entry so we can call the initialization
            //  routine for the directory.
            //

            RawDirent = (PRAW_DIRENT) CdRvdDirent( RawIsoVd, Vcb->VcbState );

            CopyUchar4( &PathEntry.DiskOffset, RawDirent->FileLoc );

            PathEntry.DiskOffset += RawDirent->XarLen;
            PathEntry.Ordinal = 1;
            PathEntry.PathTableOffset = Vcb->PathTableFcb->StreamOffset;

            CdInitializeFcbFromPathEntry( IrpContext,
                                          Vcb->RootIndexFcb,
                                          NULL,
                                          &PathEntry );

            //
            //  Create the stream file for the root directory.
            //

            CdCreateInternalStream( IrpContext, Vcb, Vcb->RootIndexFcb );

            //
            //  Now do the volume dasd Fcb.  Create this and reference it in the
            //  Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;

            Vcb->VolumeDasdFcb = CdCreateFcb( IrpContext,
                                              *((PFILE_ID) &FileId),
                                              CDFS_NTC_FCB_DATA,
                                              NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->VolumeDasdFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  The file size is the full disk.
            //

            StartingBlock = CdRvdVolSz( RawIsoVd, Vcb->VcbState );

            Vcb->VolumeDasdFcb->FileSize.QuadPart = LlBytesFromBlocks( Vcb, StartingBlock );

            Vcb->VolumeDasdFcb->AllocationSize.QuadPart =
            Vcb->VolumeDasdFcb->ValidDataLength.QuadPart = Vcb->VolumeDasdFcb->FileSize.QuadPart;

            //
            //  Now add the extent representing the volume 'by hand'.
            //

            CdLockFcb( IrpContext, Vcb->VolumeDasdFcb );

            McbEntry = Vcb->VolumeDasdFcb->Mcb.McbArray;

            McbEntry->FileOffset = 
            McbEntry->DiskOffset = 0;
            
            McbEntry->ByteCount = Vcb->VolumeDasdFcb->AllocationSize.QuadPart;
            
            McbEntry->DataBlockByteCount =
            McbEntry->TotalBlockByteCount = McbEntry->ByteCount;
            
            Vcb->VolumeDasdFcb->Mcb.CurrentEntryCount = 1;
    
            CdUnlockFcb( IrpContext, Vcb->VolumeDasdFcb );

            //
            //  Point to the file resource.
            //

            Vcb->VolumeDasdFcb->Resource = &Vcb->FileResource;

            Vcb->VolumeDasdFcb->FileAttributes = FILE_ATTRIBUTE_READONLY;

            //
            //  Mark the Fcb as initialized.
            //

            SetFlag( Vcb->VolumeDasdFcb->FcbState, FCB_STATE_INITIALIZED );

            //
            //  Check and see if this is an XA disk.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_ISO | VCB_STATE_JOLIET)
                && RtlEqualMemory( CdXaId,
                                   Add2Ptr( RawIsoVd, 0x400, PCHAR ),
                                   8 )) {

                SetFlag( Vcb->VcbState, VCB_STATE_CDXA );
            }

        //
        //  If this is a music disk then we want to mock this disk to make it
        //  look like ISO disk.  We will create a pseudo root directory in
        //  that case.
        //

        } else if (FlagOn( Vcb->VcbState, VCB_STATE_AUDIO_DISK )) {

            ULONG RootDirectorySize;

            //
            //  Create the path table Fcb and refererence it and the Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;

            Vcb->PathTableFcb = CdCreateFcb( IrpContext,
                                             *((PFILE_ID) &FileId),
                                             CDFS_NTC_FCB_PATH_TABLE,
                                             NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->PathTableFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  We only create a pseudo entry for the root.
            //

            Vcb->PathTableFcb->FileSize.QuadPart = (LONGLONG) (FIELD_OFFSET( RAW_PATH_ISO, DirId ) + 2);

            Vcb->PathTableFcb->ValidDataLength.QuadPart = Vcb->PathTableFcb->FileSize.QuadPart;

            Vcb->PathTableFcb->AllocationSize.QuadPart = LlSectorAlign( Vcb->PathTableFcb->FileSize.QuadPart );

            //
            //  Point to the file resource.
            //

            Vcb->PathTableFcb->Resource = &Vcb->FileResource;

            //
            //  Mark the Fcb as initialized and create the stream file for this.
            //

            SetFlag( Vcb->PathTableFcb->FcbState, FCB_STATE_INITIALIZED );

            CdCreateInternalStream( IrpContext, Vcb, Vcb->PathTableFcb );

            //
            //  Create the root index and reference it in the Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;
            Vcb->RootIndexFcb = CdCreateFcb( IrpContext,
                                             *((PFILE_ID) &FileId),
                                             CDFS_NTC_FCB_INDEX,
                                             NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->RootIndexFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  Create the File id by hand for this Fcb.
            //

            CdSetFidPathTableOffset( Vcb->RootIndexFcb->FileId, Vcb->PathTableFcb->StreamOffset );
            CdFidSetDirectory( Vcb->RootIndexFcb->FileId );

            //
            //  Create a pseudo path table entry so we can call the initialization
            //  routine for the directory.
            //

            RtlZeroMemory( &PathEntry, sizeof( PATH_ENTRY ));


            PathEntry.Ordinal = 1;
            PathEntry.PathTableOffset = Vcb->PathTableFcb->StreamOffset;

            CdInitializeFcbFromPathEntry( IrpContext,
                                          Vcb->RootIndexFcb,
                                          NULL,
                                          &PathEntry );

            //
            //  Set the sizes by hand for this Fcb.  It should have an entry for each track plus an
            //  entry for the root and parent.
            //

            RootDirectorySize = (Vcb->TrackCount + 2) * CdAudioDirentSize;
            RootDirectorySize = SectorAlign( RootDirectorySize );

            Vcb->RootIndexFcb->AllocationSize.QuadPart =
            Vcb->RootIndexFcb->ValidDataLength.QuadPart =
            Vcb->RootIndexFcb->FileSize.QuadPart = RootDirectorySize;

            SetFlag( Vcb->RootIndexFcb->FcbState, FCB_STATE_INITIALIZED );

            //
            //  Create the stream file for the root directory.
            //

            CdCreateInternalStream( IrpContext, Vcb, Vcb->RootIndexFcb );

            //
            //  Now do the volume dasd Fcb.  Create this and reference it in the
            //  Vcb.
            //

            CdLockVcb( IrpContext, Vcb );
            UnlockVcb = TRUE;

            Vcb->VolumeDasdFcb = CdCreateFcb( IrpContext,
                                              *((PFILE_ID) &FileId),
                                              CDFS_NTC_FCB_DATA,
                                              NULL );

            CdIncrementReferenceCounts( IrpContext, Vcb->VolumeDasdFcb, 1, 1 );
            CdUnlockVcb( IrpContext, Vcb );
            UnlockVcb = FALSE;

            //
            //  We won't allow raw reads on this Fcb so leave the size at
            //  zero.
            //

            //
            //  Point to the file resource.
            //

            Vcb->VolumeDasdFcb->Resource = &Vcb->FileResource;

            Vcb->VolumeDasdFcb->FileAttributes = FILE_ATTRIBUTE_READONLY;

            //
            //  Mark the Fcb as initialized.
            //

            SetFlag( Vcb->VolumeDasdFcb->FcbState, FCB_STATE_INITIALIZED );

            //
            //  We will store a hard-coded name in the Vpb and use the toc as
            //  the serial number.
            //

            Vcb->Vpb->VolumeLabelLength = CdAudioLabelLength;

            RtlCopyMemory( Vcb->Vpb->VolumeLabel,
                           CdAudioLabel,
                           CdAudioLabelLength );

            //
            //  Find the serial number for the audio disk.
            //

            Vcb->Vpb->SerialNumber = CdTocSerial( IrpContext, Vcb->CdromToc );

            //
            //  Set the ISO bit so we know how to treat the names.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_ISO );
        }
        
    } finally {

        if (UnlockVcb) { CdUnlockVcb( IrpContext, Vcb ); }
    }
}


VOID
CdDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to delete a Vcb which failed mount or has been
    dismounted.  The dismount code should have already removed all of the
    open Fcb's.  We do nothing here but clean up other auxilary structures.

Arguments:

    Vcb - Vcb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_EXCLUSIVE_CDDATA;
    ASSERT_EXCLUSIVE_VCB( Vcb );

    //
    //  Chuck the backpocket Vpb we kept just in case.
    //

    if (Vcb->SwapVpb) {

        CdFreePool( &Vcb->SwapVpb );
    }
    
    //
    //  If there is a Vpb then we must delete it ourselves.
    //

    if (Vcb->Vpb != NULL) {

        CdFreePool( &Vcb->Vpb );
    }

    //
    //  Dereference our target if we haven't already done so.
    //

    if (Vcb->TargetDeviceObject != NULL) {
    
        ObDereferenceObject( Vcb->TargetDeviceObject );
    }

    //
    //  Delete the XA Sector if allocated.
    //

    if (Vcb->XASector != NULL) {

        CdFreePool( &Vcb->XASector );
    }

    //
    //  Remove this entry from the global queue.
    //

    RemoveEntryList( &Vcb->VcbLinks );

    //
    //  Delete the Vcb and File resources.
    //

    ExDeleteResourceLite( &Vcb->VcbResource );
    ExDeleteResourceLite( &Vcb->FileResource );

    //
    //  Delete the TOC if present.
    //

    if (Vcb->CdromToc != NULL) {

        CdFreePool( &Vcb->CdromToc );
    }

    //
    //  Uninitialize the notify structures.
    //

    if (Vcb->NotifySync != NULL) {

        FsRtlNotifyUninitializeSync( &Vcb->NotifySync );
    }

    //
    //  Now delete the volume device object.
    //

    IoDeleteDevice( (PDEVICE_OBJECT) CONTAINING_RECORD( Vcb,
                                                        VOLUME_DEVICE_OBJECT,
                                                        Vcb ));

    return;
}


PFCB
CdCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId,
    IN NODE_TYPE_CODE NodeTypeCode,
    OUT PBOOLEAN FcbExisted OPTIONAL
    )

/*++

Routine Description:

    This routine is called to find the Fcb for the given FileId.  We will
    look this up first in the Fcb table and if not found we will create
    an Fcb.  We don't initialize it or insert it into the FcbTable in this
    routine.

    This routine is called while the Vcb is locked.

Arguments:

    FileId - This is the Id for the target Fcb.

    NodeTypeCode - Node type for this Fcb if we need to create.

    FcbExisted - If specified, we store whether the Fcb existed.

Return Value:

    PFCB - The Fcb found in the table or created if needed.

--*/

{
    PFCB NewFcb;
    BOOLEAN LocalFcbExisted;

    PAGED_CODE();

    //
    //  Use the local boolean if one was not passed in.
    //

    if (!ARGUMENT_PRESENT( FcbExisted )) {

        FcbExisted = &LocalFcbExisted;
    }

    //
    //  Maybe this is already in the table.
    //

    NewFcb = CdLookupFcbTable( IrpContext, IrpContext->Vcb, FileId );

    //
    //  If not then create the Fcb is requested by our caller.
    //

    if (NewFcb == NULL) {

        //
        //  Allocate and initialize the structure depending on the
        //  type code.
        //

        switch (NodeTypeCode) {

        case CDFS_NTC_FCB_PATH_TABLE:
        case CDFS_NTC_FCB_INDEX:

            NewFcb = CdAllocateFcbIndex( IrpContext );

            RtlZeroMemory( NewFcb, SIZEOF_FCB_INDEX );

            NewFcb->NodeByteSize = SIZEOF_FCB_INDEX;

            InitializeListHead( &NewFcb->FcbQueue );

            break;

        case CDFS_NTC_FCB_DATA :

            NewFcb = CdAllocateFcbData( IrpContext );

            RtlZeroMemory( NewFcb, SIZEOF_FCB_DATA );

            NewFcb->NodeByteSize = SIZEOF_FCB_DATA;

            break;

        default:

            CdBugCheck( 0, 0, 0 );
        }

        //
        //  Now do the common initialization.
        //

        NewFcb->NodeTypeCode = NodeTypeCode;

        NewFcb->Vcb = IrpContext->Vcb;
        NewFcb->FileId = FileId;

        CdInitializeMcb( IrpContext, NewFcb );

        //
        //  Now create the non-paged section object.
        //

        NewFcb->FcbNonpaged = CdCreateFcbNonpaged( IrpContext );

        //
        //  Deallocate the Fcb and raise if the allocation failed.
        //

        if (NewFcb->FcbNonpaged == NULL) {

            CdFreePool( &NewFcb );

            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        *FcbExisted = FALSE;

        //
        //  Initialize Advanced FCB Header fields
        //

        ExInitializeFastMutex( &NewFcb->FcbNonpaged->AdvancedFcbHeaderMutex );
        FsRtlSetupAdvancedHeader( &NewFcb->Header, 
                                  &NewFcb->FcbNonpaged->AdvancedFcbHeaderMutex );
    } else {

        *FcbExisted = TRUE;
    }

    return NewFcb;
}


VOID
CdInitializeFcbFromPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PPATH_ENTRY PathEntry
    )

/*++

Routine Description:

    This routine is called to initialize an Fcb for a directory from
    the path entry.  Since we only have a starting point for the directory,
    not the length, we can only speculate on the sizes.

    The general initialization is performed in CdCreateFcb.

Arguments:

    Fcb - Newly created Fcb for this stream.

    ParentFcb - Parent Fcb for this stream.  It may not be present.

    PathEntry - PathEntry for this Fcb in the Path Table.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Fill in the Index specific fields of the Fcb.
    //

    Fcb->StreamOffset = BytesFromBlocks( Fcb->Vcb,
                                         SectorBlockOffset( Fcb->Vcb, PathEntry->DiskOffset ));

    Fcb->Ordinal = PathEntry->Ordinal;

    //
    //  Initialize the common header in the Fcb.  The node type is already
    //  present.
    //

    Fcb->Resource = &Fcb->Vcb->FileResource;

    //
    //  Always set the sizes to one sector until we read the self-entry.
    //

    Fcb->AllocationSize.QuadPart =
    Fcb->FileSize.QuadPart =
    Fcb->ValidDataLength.QuadPart = SECTOR_SIZE;

    CdAddInitialAllocation( IrpContext,
                            Fcb,
                            PathEntry->DiskOffset,
                            SECTOR_SIZE );
    //
    //  State flags for this Fcb.
    //

    SetFlag( Fcb->FileAttributes,
             FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY );

    //
    //  Link into the other in-memory structures and into the Fcb table.
    //

    if (ParentFcb != NULL) {

        Fcb->ParentFcb = ParentFcb;

        InsertTailList( &ParentFcb->FcbQueue, &Fcb->FcbLinks );

        CdIncrementReferenceCounts( IrpContext, ParentFcb, 1, 1 );
    }

    CdInsertFcbTable( IrpContext, Fcb );
    SetFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );

    return;
}


VOID
CdInitializeFcbFromFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb,
    IN PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called to initialize an Fcb for a file from
    the file context.  We have looked up all of the dirents for this
    stream and have the full file size.  We will load the all of the allocation
    for the file into the Mcb now.

    The general initialization is performed in CdCreateFcb.

Arguments:

    Fcb - Newly created Fcb for this stream.

    ParentFcb - Parent Fcb for this stream.

    FileContext - FileContext for the file.

Return Value:

    None

--*/

{
    PDIRENT ThisDirent = &FileContext->InitialDirent->Dirent;
    PCOMPOUND_DIRENT CurrentCompoundDirent;

    LONGLONG CurrentFileOffset;
    ULONG CurrentMcbEntryOffset;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    CdLockFcb( IrpContext, Fcb );

    try {

        //
        //  Initialize the common header in the Fcb.  The node type is already
        //  present.
        //

        Fcb->Resource = &IrpContext->Vcb->FileResource;

        //
        //  Allocation occurs in block-sized units.
        //

        Fcb->FileSize.QuadPart =
        Fcb->ValidDataLength.QuadPart = FileContext->FileSize;

        Fcb->AllocationSize.QuadPart = LlBlockAlign( Fcb->Vcb, FileContext->FileSize );

        //
        //  Set the flags from the dirent.  We always start with the read-only bit.
        //

        SetFlag( Fcb->FileAttributes, FILE_ATTRIBUTE_READONLY );
        if (FlagOn( ThisDirent->DirentFlags, CD_ATTRIBUTE_HIDDEN )) {

            SetFlag( Fcb->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
        }

        //
        //  Convert the time to NT time.
        //

        CdConvertCdTimeToNtTime( IrpContext,
                                 ThisDirent->CdTime,
                                 (PLARGE_INTEGER) &Fcb->CreationTime );

        //
        //  Set the flag indicating the type of extent.
        //

        if (ThisDirent->ExtentType != Form1Data) {

            if (ThisDirent->ExtentType == Mode2Form2Data) {

                SetFlag( Fcb->FcbState, FCB_STATE_MODE2FORM2_FILE );

            } else {

                SetFlag( Fcb->FcbState, FCB_STATE_DA_FILE );
            }

            Fcb->XAAttributes = ThisDirent->XAAttributes;
            Fcb->XAFileNumber = ThisDirent->XAFileNumber;
        }

        //
        //  Read through all of the dirents for the file until we find the last
        //  and add the allocation into the Mcb.
        //

        CurrentCompoundDirent = FileContext->InitialDirent;
        CurrentFileOffset = 0;
        CurrentMcbEntryOffset = 0;

        while (TRUE) {

            CdAddAllocationFromDirent( IrpContext,
                                       Fcb,
                                       CurrentMcbEntryOffset,
                                       CurrentFileOffset,
                                       &CurrentCompoundDirent->Dirent );

            //
            //  Break out if we are at the last dirent.
            //

            if (!FlagOn( CurrentCompoundDirent->Dirent.DirentFlags, CD_ATTRIBUTE_MULTI )) {

                break;
            }

            CurrentFileOffset += CurrentCompoundDirent->Dirent.DataLength;
            CurrentMcbEntryOffset += 1;

            //
            //  We better be able to find the next dirent.
            //

            if (!CdLookupNextDirent( IrpContext,
                                     ParentFcb,
                                     &CurrentCompoundDirent->DirContext,
                                     &FileContext->CurrentDirent->DirContext )) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            CurrentCompoundDirent = FileContext->CurrentDirent;

            CdUpdateDirentFromRawDirent( IrpContext,
                                         ParentFcb,
                                         &CurrentCompoundDirent->DirContext,
                                         &CurrentCompoundDirent->Dirent );
        }

        //
        //  Show that the Fcb is initialized.
        //

        SetFlag( Fcb->FcbState, FCB_STATE_INITIALIZED );

        //
        //  Link into the other in-memory structures and into the Fcb table.
        //

        Fcb->ParentFcb = ParentFcb;

        InsertTailList( &ParentFcb->FcbQueue, &Fcb->FcbLinks );

        CdIncrementReferenceCounts( IrpContext, ParentFcb, 1, 1 );

        CdInsertFcbTable( IrpContext, Fcb );
        SetFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );

    } finally {

        CdUnlockFcb( IrpContext, Fcb );
    }

    return;
}


PCCB
CdCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine is called to allocate and initialize the Ccb structure.

Arguments:

    Fcb - This is the Fcb for the file being opened.

    Flags - User flags to set in this Ccb.

Return Value:

    PCCB - Pointer to the created Ccb.

--*/

{
    PCCB NewCcb;
    PAGED_CODE();

    //
    //  Allocate and initialize the structure.
    //

    NewCcb = CdAllocateCcb( IrpContext );

    RtlZeroMemory( NewCcb, sizeof( CCB ));

    //
    //  Set the proper node type code and node byte size
    //

    NewCcb->NodeTypeCode = CDFS_NTC_CCB;
    NewCcb->NodeByteSize = sizeof( CCB );

    //
    //  Set the initial value for the flags and Fcb
    //

    NewCcb->Flags = Flags;
    NewCcb->Fcb = Fcb;

    return NewCcb;
}


VOID
CdDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )
/*++

Routine Description:

    This routine is called to cleanup and deallocate a Ccb structure.

Arguments:

    Ccb - This is the Ccb to delete.

Return Value:

    None

--*/

{
    PAGED_CODE();

    if (Ccb->SearchExpression.FileName.Buffer != NULL) {

        CdFreePool( &Ccb->SearchExpression.FileName.Buffer );
    }

    CdDeallocateCcb( IrpContext, Ccb );
    return;
}


BOOLEAN
CdCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called when we want to attach a file lock structure to the
    given Fcb.  It is possible the file lock is already attached.

    This routine is sometimes called from the fast path and sometimes in the
    Irp-based path.  We don't want to raise in the fast path, just return FALSE.

Arguments:

    Fcb - This is the Fcb to create the file lock for.

    RaiseOnError - If TRUE, we will raise on an allocation failure.  Otherwise we
        return FALSE on an allocation failure.

Return Value:

    BOOLEAN - TRUE if the Fcb has a filelock, FALSE otherwise.

--*/

{
    BOOLEAN Result = TRUE;
    PFILE_LOCK FileLock;

    PAGED_CODE();

    //
    //  Lock the Fcb and check if there is really any work to do.
    //

    CdLockFcb( IrpContext, Fcb );

    if (Fcb->FileLock != NULL) {

        CdUnlockFcb( IrpContext, Fcb );
        return TRUE;
    }

    Fcb->FileLock = FileLock =
        FsRtlAllocateFileLock( NULL, NULL );

    CdUnlockFcb( IrpContext, Fcb );

    //
    //  Return or raise as appropriate.
    //

    if (FileLock == NULL) {
         
        if (RaiseOnError) {

            ASSERT( ARGUMENT_PRESENT( IrpContext ));

            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        Result = FALSE;
    }

    return Result;
}


PIRP_CONTEXT
CdCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine is called to initialize an IrpContext for the current
    CDFS request.  We allocate the structure and then initialize it from
    the given Irp.

Arguments:

    Irp - Irp for this request.

    Wait - TRUE if this request is synchronous, FALSE otherwise.

Return Value:

    PIRP_CONTEXT - Allocated IrpContext.

--*/

{
    PIRP_CONTEXT NewIrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    //
    //  The only operations a filesystem device object should ever receive
    //  are create/teardown of fsdo handles and operations which do not
    //  occur in the context of fileobjects (i.e., mount).
    //

    if (IrpSp->DeviceObject == CdData.FileSystemDeviceObject) {

        if (IrpSp->FileObject != NULL &&
            IrpSp->MajorFunction != IRP_MJ_CREATE &&
            IrpSp->MajorFunction != IRP_MJ_CLEANUP &&
            IrpSp->MajorFunction != IRP_MJ_CLOSE) {

            ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
        }

        ASSERT( IrpSp->FileObject != NULL ||
                
                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                 IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_INVALIDATE_VOLUMES) ||
                
                (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                 IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME ) ||

                IrpSp->MajorFunction == IRP_MJ_SHUTDOWN );
    }

    //
    //  Look in our lookaside list for an IrpContext.
    //

    if (CdData.IrpContextDepth) {

        CdLockCdData();
        NewIrpContext = (PIRP_CONTEXT) PopEntryList( &CdData.IrpContextList );
        if (NewIrpContext != NULL) {

            CdData.IrpContextDepth--;
        }

        CdUnlockCdData();
    }

    if (NewIrpContext == NULL) {

        //
        //  We didn't get it from our private list so allocate it from pool.
        //

        NewIrpContext = FsRtlAllocatePoolWithTag( NonPagedPool, sizeof( IRP_CONTEXT ), TAG_IRP_CONTEXT );
    }

    RtlZeroMemory( NewIrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    NewIrpContext->NodeTypeCode = CDFS_NTC_IRP_CONTEXT;
    NewIrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Set the originating Irp field
    //

    NewIrpContext->Irp = Irp;

    //
    //  Copy RealDevice for workque algorithms.  We will update this in the Mount or
    //  Verify since they have no file objects to use here.
    //

    if (IrpSp->FileObject != NULL) {

        NewIrpContext->RealDevice = IrpSp->FileObject->DeviceObject;
    }

    //
    //  Locate the volume device object and Vcb that we are trying to access.
    //  This may be our filesystem device object.  In that case don't initialize
    //  the Vcb field.
    //

    if (IrpSp->DeviceObject != CdData.FileSystemDeviceObject) {

        NewIrpContext->Vcb =  &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;
    
    }

    //
    //  Major/Minor Function codes
    //

    NewIrpContext->MajorFunction = IrpSp->MajorFunction;
    NewIrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Set the wait parameter
    //

    if (Wait) {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    } else {

        SetFlag( NewIrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );
    }

    //
    //  return and tell the caller
    //

    return NewIrpContext;
}


VOID
CdCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    )

/*++

Routine Description:

    This routine is called to cleanup and possibly deallocate the Irp Context.
    If the request is being posted or this Irp Context is possibly on the
    stack then we only cleanup any auxilary structures.

Arguments:

    Post - TRUE if we are posting this request, FALSE if we are deleting
        or retrying this in the current thread.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  If we aren't doing more processing then deallocate this as appropriate.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING)) {

        //
        //  If this context is the top level CDFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            CdRestoreThreadContext( IrpContext );
        }

        //
        //  Deallocate the Io context if allocated.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO )) {

            CdFreeIoContext( IrpContext->IoContext );
        }

        //
        //  Deallocate the IrpContext if not from the stack.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK )) {

            if (CdData.IrpContextDepth < CdData.IrpContextMaxDepth) {

                CdLockCdData();

                PushEntryList( &CdData.IrpContextList, (PSINGLE_LIST_ENTRY) IrpContext );
                CdData.IrpContextDepth++;

                CdUnlockCdData();

            } else {

                //
                //  We couldn't add this to our lookaside list so free it to
                //  pool.
                //

                CdFreePool( &IrpContext );
            }
        }

    //
    //  Clear the appropriate flags.
    //

    } else if (Post) {

        //
        //  If this context is the top level CDFS context then we need to
        //  restore the top level thread context.
        //

        if (IrpContext->ThreadContext != NULL) {

            CdRestoreThreadContext( IrpContext );
        }

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );

    } else {

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY );
    }

    return;
}


VOID
CdInitializeStackIrpContext (
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite
    )

/*++

Routine Description:

    This routine is called to initialize an IrpContext for the current
    CDFS request.  The IrpContext is on the stack and we need to initialize
    it for the current request.  The request is a close operation.

Arguments:

    IrpContext - IrpContext to initialize.

    IrpContextLite - Structure containing the details of this request.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Zero and then initialize the structure.
    //

    RtlZeroMemory( IrpContext, sizeof( IRP_CONTEXT ));

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = CDFS_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Note that this is from the stack.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ON_STACK );

    //
    //  Copy RealDevice for workque algorithms.
    //

    IrpContext->RealDevice = IrpContextLite->RealDevice;

    //
    //  The Vcb is found in the Fcb.
    //

    IrpContext->Vcb = IrpContextLite->Fcb->Vcb;

    //
    //  Major/Minor Function codes
    //

    IrpContext->MajorFunction = IRP_MJ_CLOSE;

    //
    //  Set the wait parameter
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    return;
}


VOID
CdTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartingFcb,
    OUT PBOOLEAN RemovedStartingFcb
    )

/*++

Routine Description:

    This routine is used to walk from some starting point in the Fcb tree towards
    the root.  It will remove the Fcb and continue walking up the tree until
    it finds a point where we can't remove an Fcb.

    We look at the following fields in the Fcb to determine whether we can
    remove this.

        1 - Handle count must be zero.
        2 - If directory then only the only reference can be for a stream file.
        3 - Reference count must either be zero or go to zero here.

    We return immediately if we are recursively entering this routine.

Arguments:

    StartingFcb - This is the Fcb node in the tree to begin with.  This Fcb
        must currently be acquired exclusively.

    RemovedStartingFcb - Address to store whether we removed the starting Fcb.

Return Value:

    None

--*/

{
    PVCB Vcb = StartingFcb->Vcb;
    PFCB CurrentFcb = StartingFcb;
    BOOLEAN AcquiredCurrentFcb = FALSE;
    PFCB ParentFcb;

    PAGED_CODE();

    *RemovedStartingFcb = FALSE;

    //
    //  If this is a recursive call to TearDownStructures we return immediately
    //  doing no operation.
    //

    if (FlagOn( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN )) {

        return;
    }

    SetFlag( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN );

    //
    //  Use a try-finally to safely clear the top-level field.
    //

    try {

        //
        //  Loop until we find an Fcb we can't remove.
        //

        do {

            //
            //  See if there is an internal stream we should delete.
            //  Only do this if it is the last reference on the Fcb.
            //

            if ((SafeNodeType( CurrentFcb ) != CDFS_NTC_FCB_DATA) &&
                (CurrentFcb->FcbUserReference == 0) &&
                (CurrentFcb->FileObject != NULL)) {

                //
                //  Go ahead and delete the stream file object.
                //

                CdDeleteInternalStream( IrpContext, CurrentFcb );
            }

            //
            //  If the reference count is non-zero then break.
            //

            if (CurrentFcb->FcbReference != 0) {

                break;
            }

            //
            //  It looks like we have a candidate for removal here.  We
            //  will need to acquire the parent, if present, in order to
            //  remove this from the parent prefix table.
            //

            ParentFcb = CurrentFcb->ParentFcb;

            if (ParentFcb != NULL) {

                CdAcquireFcbExclusive( IrpContext, ParentFcb, FALSE );
            }

            //
            //  Now lock the vcb.
            //

            CdLockVcb( IrpContext, Vcb );

            //
            //  Final check to see if the reference count is still zero.
            //

            if (CurrentFcb->FcbReference != 0) {

                CdUnlockVcb( IrpContext, Vcb );

                if (ParentFcb != NULL) {

                    CdReleaseFcb( IrpContext, ParentFcb );
                }

                break;
            }

            //
            //  If there is a parent then do the necessary cleanup for the parent.
            //

            if (ParentFcb != NULL) {

                CdRemovePrefix( IrpContext, CurrentFcb );
                RemoveEntryList( &CurrentFcb->FcbLinks );

                CdDecrementReferenceCounts( IrpContext, ParentFcb, 1, 1 );
            }

            if (FlagOn( CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE )) {

                CdDeleteFcbTable( IrpContext, CurrentFcb );
                ClearFlag( CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE );

            }

            //
            //  Unlock the Vcb but hold the parent in order to walk up
            //  the tree.
            //

            CdUnlockVcb( IrpContext, Vcb );
            CdDeleteFcb( IrpContext, CurrentFcb );

            //
            //  Move to the parent Fcb.
            //

            CurrentFcb = ParentFcb;
            AcquiredCurrentFcb = TRUE;

        } while (CurrentFcb != NULL);

    } finally {

        //
        //  Release the current Fcb if we have acquired it.
        //

        if (AcquiredCurrentFcb && (CurrentFcb != NULL)) {

            CdReleaseFcb( IrpContext, CurrentFcb );
        }

        //
        //  Clear the teardown flag.
        //

        ClearFlag( IrpContext->TopLevel->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN );
    }

    *RemovedStartingFcb = (CurrentFcb != StartingFcb);
    return;
}


PFCB
CdLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_ID FileId
    )

/*++

Routine Description:

    This routine will look through the Fcb table looking for a matching
    entry.

Arguments:

    Vcb - Vcb for this volume.

    FileId - This is the key value to use for the search.

Return Value:

    PFCB - A pointer to the matching entry or NULL otherwise.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Hit;
    PFCB ReturnFcb = NULL;

    PAGED_CODE();

    Key.FileId = FileId;

    Hit = (PFCB_TABLE_ELEMENT) RtlLookupElementGenericTable( &Vcb->FcbTable, &Key );

    if (Hit != NULL) {

        ReturnFcb = Hit->Fcb;
    }

    return ReturnFcb;

    UNREFERENCED_PARAMETER( IrpContext );
}


PFCB
CdGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    This routine will enumerate through all of the Fcb's in the Fcb table.

Arguments:

    Vcb - Vcb for this volume.

    RestartKey - This value is used by the table package to maintain
        its position in the enumeration.  It is initialized to NULL
        for the first search.

Return Value:

    PFCB - A pointer to the next fcb or NULL if the enumeration is
        completed

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = (PFCB) RtlEnumerateGenericTableWithoutSplaying( &Vcb->FcbTable, RestartKey );

    if (Fcb != NULL) {

        Fcb = ((PFCB_TABLE_ELEMENT)(Fcb))->Fcb;
    }

    return Fcb;
}


NTSTATUS
CdProcessToc (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PCDROM_TOC CdromToc,
    IN OUT PULONG Length,
    OUT PULONG TrackCount,
    OUT PULONG DiskFlags
    )

/*++

Routine Description:

    This routine is called to verify and process the TOC for this disk.
    We hide a data track for a CD+ volume.

Arguments:

    TargetDeviceObject - Device object to send TOC request to.

    CdromToc - Pointer to TOC structure.

    Length - On input this is the length of the TOC.  On return is the TOC
        length we will show to the user.

    TrackCount - This is the count of tracks for the TOC.  We use this
        when creating a pseudo directory for a music disk.

    DiskFlags - We return flags indicating what we know about this disk.

Return Value:

    NTSTATUS - The result of trying to read the TOC.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    ULONG CurrentTrack;
    ULONG LocalTrackCount;
    ULONG LocalTocLength;

    union {

        UCHAR BigEndian[2];
        USHORT Length;

    } BiasedTocLength;

    PTRACK_DATA Track;

    PAGED_CODE();

    //
    //  Go ahead and read the table of contents
    //

    Status = CdPerformDevIoCtrl( IrpContext,
                                 IOCTL_CDROM_READ_TOC,
                                 TargetDeviceObject,
                                 CdromToc,
                                 sizeof( CDROM_TOC ),
                                 FALSE,
                                 TRUE,
                                 &Iosb );

    //
    //  Nothing to process if this request fails.
    //

    if (Status != STATUS_SUCCESS) {

        return Status;
    }

    //
    //  Get the number of tracks and stated size of this structure.
    //

    CurrentTrack = 0;
    LocalTrackCount = CdromToc->LastTrack - CdromToc->FirstTrack + 1;
    LocalTocLength = PtrOffset( CdromToc, &CdromToc->TrackData[LocalTrackCount + 1] );

    //
    //  Get out if there is an immediate problem with the TOC.
    //

    if ((LocalTocLength > Iosb.Information) ||
        (CdromToc->FirstTrack > CdromToc->LastTrack)) {

        Status = STATUS_DISK_CORRUPT_ERROR;
        return Status;
    }

    //
    //  Walk through the individual tracks.  Stop at the first data track after
    //  any lead-in audio tracks.
    //

    do {

        //
        //  Get the next track.
        //

        Track = &CdromToc->TrackData[CurrentTrack];

        //
        //  If this is a data track then check if we have only seen audio tracks
        //  to this point.
        //

        if (FlagOn( Track->Control, TOC_DATA_TRACK )) {

            //
            //  If we have only seen audio tracks then assume this is a
            //  CD+ disk.  Hide the current data track and only return
            //  the previous audio tracks.  Set the disk type to be mixed
            //  data/audio.
            //

            if (FlagOn( *DiskFlags, CDROM_DISK_AUDIO_TRACK ) &&
                !FlagOn( *DiskFlags, CDROM_DISK_DATA_TRACK )) {

                //
                //  Remove one track from the TOC.
                //

                CdromToc->LastTrack -= 1;

                //
                //  Knock 2.5 minutes off the current track to
                //  hide the final leadin.
                //

                Track->Address[1] -= 2;
                Track->Address[2] += 30;

                if (Track->Address[2] < 60) {

                    Track->Address[1] -= 1;

                } else {

                    Track->Address[2] -= 60;
                }

                Track->TrackNumber = TOC_LAST_TRACK;

                //
                //  Set the disk type to mixed data/audio.
                //

                SetFlag( *DiskFlags, CDROM_DISK_DATA_TRACK );

                break;
            }

            //
            //  Set the flag to indicate data tracks present.
            //

            SetFlag( *DiskFlags, CDROM_DISK_DATA_TRACK );

        //
        //  If this is a audio track then set the flag indicating audio
        //  tracks.
        //

        } else {

            SetFlag( *DiskFlags, CDROM_DISK_AUDIO_TRACK );
        }

        //
        //  Set our index for the next track.
        //

        CurrentTrack += 1;

    } while (CurrentTrack < LocalTrackCount);

    //
    //  Set the length to point just past the last track we looked at.
    //

    *TrackCount = CurrentTrack;
    *Length = PtrOffset( CdromToc, &CdromToc->TrackData[CurrentTrack + 1] );
    BiasedTocLength.Length = (USHORT) *Length - 2;

    CdromToc->Length[0] = BiasedTocLength.BigEndian[1];
    CdromToc->Length[1] = BiasedTocLength.BigEndian[0];

    return Status;
}


//
//  Local support routine
//

VOID
CdDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to cleanup and deallocate an Fcb.  We know there
    are no references remaining.  We cleanup any auxilary structures and
    deallocate this Fcb.

Arguments:

    Fcb - This is the Fcb to deallcoate.

Return Value:

    None

--*/

{
    PVCB Vcb = NULL;
    PAGED_CODE();

    //
    //  Sanity check the counts.
    //

    ASSERT( Fcb->FcbCleanup == 0 );
    ASSERT( Fcb->FcbReference == 0 );

    //
    //  Release any Filter Context structures associated with this FCB
    //

    FsRtlTeardownPerStreamContexts( &Fcb->Header );

    //
    //  Start with the common structures.
    //

    CdUninitializeMcb( IrpContext, Fcb );

    CdDeleteFcbNonpaged( IrpContext, Fcb->FcbNonpaged );

    //
    //  Check if we need to deallocate the prefix name buffer.
    //

    if ((Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer != (PWCHAR) Fcb->FileNamePrefix.FileNameBuffer) &&
        (Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer != NULL)) {

        CdFreePool( &Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer );
    }

    //
    //  Now look at the short name prefix.
    //

    if (Fcb->ShortNamePrefix != NULL) {

        CdFreePool( &Fcb->ShortNamePrefix );
    }

    //
    //  Now do the type specific structures.
    //

    switch (Fcb->NodeTypeCode) {

    case CDFS_NTC_FCB_PATH_TABLE:
    case CDFS_NTC_FCB_INDEX:

        ASSERT( Fcb->FileObject == NULL );
        ASSERT( IsListEmpty( &Fcb->FcbQueue ));

        if (Fcb == Fcb->Vcb->RootIndexFcb) {

            Vcb = Fcb->Vcb;
            Vcb->RootIndexFcb = NULL;

        } else if (Fcb == Fcb->Vcb->PathTableFcb) {

            Vcb = Fcb->Vcb;
            Vcb->PathTableFcb = NULL;
        }

        CdDeallocateFcbIndex( IrpContext, Fcb );
        break;

    case CDFS_NTC_FCB_DATA :

        if (Fcb->FileLock != NULL) {

            FsRtlFreeFileLock( Fcb->FileLock );
        }

        FsRtlUninitializeOplock( &Fcb->Oplock );

        if (Fcb == Fcb->Vcb->VolumeDasdFcb) {

            Vcb = Fcb->Vcb;
            Vcb->VolumeDasdFcb = NULL;
        }

        CdDeallocateFcbData( IrpContext, Fcb );
    }

    //
    //  Decrement the Vcb reference count if this is a system
    //  Fcb.
    //

    if (Vcb != NULL) {

        InterlockedDecrement( &Vcb->VcbReference );
        InterlockedDecrement( &Vcb->VcbUserReference );
    }

    return;
}


//
//  Local support routine
//

PFCB_NONPAGED
CdCreateFcbNonpaged (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to create and initialize the non-paged portion
    of an Fcb.

Arguments:

Return Value:

    PFCB_NONPAGED - Pointer to the created nonpaged Fcb.  NULL if not created.

--*/

{
    PFCB_NONPAGED FcbNonpaged;

    PAGED_CODE();

    //
    //  Allocate the non-paged pool and initialize the various
    //  synchronization objects.
    //

    FcbNonpaged = CdAllocateFcbNonpaged( IrpContext );

    if (FcbNonpaged != NULL) {

        RtlZeroMemory( FcbNonpaged, sizeof( FCB_NONPAGED ));

        FcbNonpaged->NodeTypeCode = CDFS_NTC_FCB_NONPAGED;
        FcbNonpaged->NodeByteSize = sizeof( FCB_NONPAGED );

        ExInitializeResourceLite( &FcbNonpaged->FcbResource );
        ExInitializeFastMutex( &FcbNonpaged->FcbMutex );
    }

    return FcbNonpaged;
}


//
//  Local support routine
//

VOID
CdDeleteFcbNonpaged (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB_NONPAGED FcbNonpaged
    )

/*++

Routine Description:

    This routine is called to cleanup the non-paged portion of an Fcb.

Arguments:

    FcbNonpaged - Structure to clean up.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ExDeleteResourceLite( &FcbNonpaged->FcbResource );

    CdDeallocateFcbNonpaged( IrpContext, FcbNonpaged );

    return;
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
CdFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Fid1,
    IN PVOID Fid2
    )

/*++

Routine Description:

    This routine is the Cdfs compare routine called by the generic table package.
    If will compare the two File Id values and return a comparison result.

Arguments:

    FcbTable - This is the table being searched.

    Fid1 - First key value.

    Fid2 - Second key value.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    FILE_ID Id1, Id2;
    PAGED_CODE();

    Id1 = *((FILE_ID UNALIGNED *) Fid1);
    Id2 = *((FILE_ID UNALIGNED *) Fid2);

    if (Id1.QuadPart < Id2.QuadPart) {

        return GenericLessThan;

    } else if (Id1.QuadPart > Id2.QuadPart) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }

    UNREFERENCED_PARAMETER( FcbTable );
}


//
//  Local support routine
//

PVOID
CdAllocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    FcbTable - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();

    return( FsRtlAllocatePoolWithTag( CdPagedPool, ByteSize, TAG_FCB_TABLE ));
}


//
//  Local support routine
//

VOID
CdDeallocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine that deallocates memory

Arguments:

    FcbTable - Supplies the generic table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    PAGED_CODE();

    CdFreePool( &Buffer );

    UNREFERENCED_PARAMETER( FcbTable );
}


//
//  Local support routine
//

ULONG
CdTocSerial (
    IN PIRP_CONTEXT IrpContext,
    IN PCDROM_TOC CdromToc
    )

/*++

Routine Description:

    This routine is called to generate a serial number for an audio disk.
    The number is based on the starting positions of the tracks.
    The following algorithm is used.

    If the number of tracks is <= 2 then initialize the serial number to the
    leadout block number.

    Then add the starting address of each track (use 0x00mmssff format).

Arguments:

    CdromToc - Valid table of contents to use for track information.

Return Value:

    ULONG - 32 bit serial number based on TOC.

--*/

{
    ULONG SerialNumber = 0;
    PTRACK_DATA ThisTrack;
    PTRACK_DATA LastTrack;

    PAGED_CODE();

    //
    //  Check if there are two tracks or fewer.
    //

    LastTrack = &CdromToc->TrackData[ CdromToc->LastTrack - CdromToc->FirstTrack + 1];
    ThisTrack = &CdromToc->TrackData[0];

    if (CdromToc->LastTrack - CdromToc->FirstTrack <= 1) {

        SerialNumber = (((LastTrack->Address[1] * 60) + LastTrack->Address[2]) * 75) + LastTrack->Address[3];

        SerialNumber -= (((ThisTrack->Address[1] * 60) + ThisTrack->Address[2]) * 75) + ThisTrack->Address[3];
    }

    //
    //  Now find the starting offset of each track and add to the serial number.
    //

    while (ThisTrack != LastTrack) {

        SerialNumber += (ThisTrack->Address[1] << 16);
        SerialNumber += (ThisTrack->Address[2] << 8);
        SerialNumber += ThisTrack->Address[3];
        ThisTrack += 1;
    }

    return SerialNumber;
}


