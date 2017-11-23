/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module implements the cache management routines for the Fat
    FSD and FSP, by calling the Common Cache Manager.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)

#if DBG

BOOLEAN
FatIsCurrentOperationSynchedForDcbTeardown (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    );

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCloseEaFile)
#pragma alloc_text(PAGE, FatCompleteMdl)
#pragma alloc_text(PAGE, FatOpenDirectoryFile)
#pragma alloc_text(PAGE, FatOpenEaFile)
#pragma alloc_text(PAGE, FatPinMappedData)
#pragma alloc_text(PAGE, FatPrepareWriteDirectoryFile)
#pragma alloc_text(PAGE, FatPrepareWriteVolumeFile)
#pragma alloc_text(PAGE, FatReadDirectoryFile)
#pragma alloc_text(PAGE, FatReadVolumeFile)
#pragma alloc_text(PAGE, FatRepinBcb)
#pragma alloc_text(PAGE, FatSyncUninitializeCacheMap)
#pragma alloc_text(PAGE, FatUnpinRepinnedBcbs)
#pragma alloc_text(PAGE, FatZeroData)
#if DBG
#pragma alloc_text(PAGE, FatIsCurrentOperationSynchedForDcbTeardown)
#endif
#endif


VOID
FatReadVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called when the specified range of sectors is to be
    read into the cache.  In fat, the volume file only contains the boot
    sector, reserved sectors, and the "fat(s)."  Thus the volume file is
    of fixed size and only extends up to (but not not including) the root
    directory entry, and will never move or change size.

    The fat volume file is also peculiar in that, since it starts at the
    logical beginning of the disk, Vbo == Lbo.

Arguments:

    Vcb - Pointer to the VCB for the volume

    StartingVbo - The virtual offset of the first desired byte

    ByteCount - Number of bytes desired

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

--*/

{
    LARGE_INTEGER Vbo;

    PAGED_CODE();

    //
    //  Check to see that all references are within the Bios Parameter Block
    //  or the fat(s).  A special case is made when StartingVbo == 0 at
    //  mounting time since we do not know how big the fat is.
    //

    ASSERT( ((StartingVbo == 0) || ((StartingVbo + ByteCount) <= (ULONG)
            (FatRootDirectoryLbo( &Vcb->Bpb ) + PAGE_SIZE))));

    DebugTrace(+1, Dbg, "FatReadVolumeFile\n", 0);
    DebugTrace( 0, Dbg, "Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "StartingVbo = %08lx\n", StartingVbo);
    DebugTrace( 0, Dbg, "ByteCount   = %08lx\n", ByteCount);

    //
    //  Call the Cache manager to attempt the transfer.
    //

    Vbo.QuadPart = StartingVbo;

    if (!CcMapData( Vcb->VirtualVolumeFile,
                    &Vbo,
                    ByteCount,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer )) {

        ASSERT( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        //
        // Could not read the data without waiting (cache miss).
        //

        FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    DbgDoit( IrpContext->PinCount += 1 )

    DebugTrace(-1, Dbg, "FatReadVolumeFile -> VOID, *BCB = %08lx\n", *Bcb);

    return;
}


VOID
FatPrepareWriteVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Reversible,
    IN BOOLEAN Zero
    )

/*++

Routine Description:

    This routine first looks to see if the specified range of sectors,
    is already in the cache.  If so, it increments the BCB PinCount,
    sets the BCB dirty, and returns with the location of the sectors.

    If the sectors are not in the cache and Wait is TRUE, it finds a
    free BCB (potentially causing a flush), and clears out the entire
    buffer.  Once this is done, it increments the BCB PinCount, sets the
    BCB dirty, and returns with the location of the sectors.

    If the sectors are not in the cache and Wait is FALSE, this routine
    raises STATUS_CANT_WAIT.

Arguments:

    Vcb - Pointer to the VCB for the volume

    StartingVbo - The virtual offset of the first byte to be written

    ByteCount - Number of bytes to be written

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

    Reversible - Supplies TRUE if the specified range of modification should
        be repinned so that the operation can be reversed in a controlled
        fashion if errors are encountered.
    
    Zero - Supplies TRUE if the specified range of bytes should be zeroed

--*/

{
    LARGE_INTEGER Vbo;

    PAGED_CODE();
    
    //
    //  Check to see that all references are within the Bios Parameter Block
    //  or the fat(s).
    //

    ASSERT( ((StartingVbo + ByteCount) <= (ULONG)
            (FatRootDirectoryLbo( &Vcb->Bpb ))));

    DebugTrace(+1, Dbg, "FatPrepareWriteVolumeFile\n", 0);
    DebugTrace( 0, Dbg, "Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "StartingVbo = %08lx\n", (ULONG)StartingVbo);
    DebugTrace( 0, Dbg, "ByteCount   = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Zero        = %08lx\n", Zero);

    //
    //  Call the Cache manager to attempt the transfer.
    //

    Vbo.QuadPart = StartingVbo;

    if (!CcPinRead( Vcb->VirtualVolumeFile,
                    &Vbo,
                    ByteCount,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer )) {

        ASSERT( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        //
        // Could not read the data without waiting (cache miss).
        //

        FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    //
    //  This keeps the data pinned until we complete the request
    //  and writes the dirty bit through to the disk.
    //

    DbgDoit( IrpContext->PinCount += 1 )

    _SEH2_TRY {

        if (Zero) {
            
            RtlZeroMemory( *Buffer, ByteCount );
        }

        FatSetDirtyBcb( IrpContext, *Bcb, Vcb, Reversible );

    } _SEH2_FINALLY {

        if (_SEH2_AbnormalTermination()) {

            FatUnpinBcb(IrpContext, *Bcb);
        }
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatPrepareWriteVolumeFile -> VOID, *Bcb = %08lx\n", *Bcb);

    return;
}


VOID
FatReadDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    IN BOOLEAN Pin,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine is called when the specified range of sectors is to be
    read into the cache.  If the desired range falls beyond the current
    cache mapping, the fat will be searched, and if the desired range can
    be satisfied, the cache mapping will be extended and the MCB updated
    accordingly.

Arguments:

    Dcb - Pointer to the DCB for the directory

    StartingVbo - The virtual offset of the first desired byte

    ByteCount - Number of bytes desired

    Pin - Tells us if we should pin instead of just mapping.

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

    Status - Returns the status of the operation.

--*/

{
    LARGE_INTEGER Vbo;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatReadDirectoryFile\n", 0);
    DebugTrace( 0, Dbg, "Dcb         = %08lx\n", Dcb);
    DebugTrace( 0, Dbg, "StartingVbo = %08lx\n", StartingVbo);
    DebugTrace( 0, Dbg, "ByteCount   = %08lx\n", ByteCount);

    //
    //  Check for the zero case
    //

    if (ByteCount == 0) {

        DebugTrace(0, Dbg, "Nothing to read\n", 0);

        *Bcb = NULL;
        *Buffer = NULL;
        *Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "FatReadDirectoryFile -> VOID\n", 0);
        return;
    }

    //
    //  If we need to create a directory file and initialize the
    //  cachemap, do so.
    //

    FatOpenDirectoryFile( IrpContext, Dcb );

    //
    //  Now if the transfer is beyond the allocation size return EOF.
    //

    if (StartingVbo >= Dcb->Header.AllocationSize.LowPart) {

        DebugTrace(0, Dbg, "End of file read for directory\n", 0);

        *Bcb = NULL;
        *Buffer = NULL;
        *Status = STATUS_END_OF_FILE;

        DebugTrace(-1, Dbg, "FatReadDirectoryFile -> VOID\n", 0);
        return;
    }

    //
    // If the caller is trying to read past the EOF, truncate the
    // read.
    //

    ByteCount = (Dcb->Header.AllocationSize.LowPart - StartingVbo < ByteCount) ?
                 Dcb->Header.AllocationSize.LowPart - StartingVbo : ByteCount;

    ASSERT( ByteCount != 0 );

    //
    //  Call the Cache manager to attempt the transfer.
    //

    Vbo.QuadPart = StartingVbo;

    if (Pin ?

        !CcPinRead( Dcb->Specific.Dcb.DirectoryFile,
                    &Vbo,
                    ByteCount,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer )
        :

        !CcMapData( Dcb->Specific.Dcb.DirectoryFile,
                    &Vbo,
                    ByteCount,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer ) ) {

        //
        // Could not read the data without waiting (cache miss).
        //

        *Bcb = NULL;
        *Buffer = NULL;
        FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    DbgDoit( IrpContext->PinCount += 1 )

    *Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "FatReadDirectoryFile -> VOID, *BCB = %08lx\n", *Bcb);

    return;
}


VOID
FatPrepareWriteDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Zero,
    IN BOOLEAN Reversible,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine first looks to see if the specified range of sectors
    is already in the cache.  If so, it increments the BCB PinCount,
    sets the BCB dirty, and returns TRUE with the location of the sectors.

    The IrpContext->Flags .. Wait == TRUE/FALSE actions of this routine are identical to
    FatPrepareWriteVolumeFile() above.

Arguments:

    Dcb - Pointer to the DCB for the directory

    StartingVbo - The virtual offset of the first byte to be written

    ByteCount - Number of bytes to be written

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

    Zero - Supplies TRUE if the specified range of bytes should be zeroed
    
    Reversible - Supplies TRUE if the specified range of modification should
        be repinned so that the operation can be reversed in a controlled
        fashion if errors are encountered.
    
    Status - Returns the status of the operation.

--*/

{
    LARGE_INTEGER Vbo;
    ULONG InitialAllocation;
    BOOLEAN UnwindWeAllocatedDiskSpace = FALSE;
    ULONG ClusterSize;

    PVOID LocalBuffer;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatPrepareWriteDirectoryFile\n", 0);
    DebugTrace( 0, Dbg, "Dcb         = %08lx\n", Dcb);
    DebugTrace( 0, Dbg, "StartingVbo = %08lx\n", (ULONG)StartingVbo);
    DebugTrace( 0, Dbg, "ByteCount   = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Zero        = %08lx\n", Zero);

    *Bcb = NULL;
    *Buffer = NULL;

    //
    //  If we need to create a directory file and initialize the
    //  cachemap, do so.
    //

    FatOpenDirectoryFile( IrpContext, Dcb );

    //
    //  If the transfer is beyond the allocation size we need to
    //  extend the directory's allocation.  The call to
    //  AddFileAllocation will raise a condition if
    //  it runs out of disk space.  Note that the root directory
    //  cannot be extended.
    //

    Vbo.QuadPart = StartingVbo;

    _SEH2_TRY {

        if (StartingVbo + ByteCount > Dcb->Header.AllocationSize.LowPart) {

            if (NodeType(Dcb) == FAT_NTC_ROOT_DCB &&
                !FatIsFat32(Dcb->Vcb)) {

                FatRaiseStatus( IrpContext, STATUS_DISK_FULL );
            }

            DebugTrace(0, Dbg, "Try extending normal directory\n", 0);

            InitialAllocation = Dcb->Header.AllocationSize.LowPart;

            FatAddFileAllocation( IrpContext,
                                  Dcb,
                                  Dcb->Specific.Dcb.DirectoryFile,
                                  StartingVbo + ByteCount );

            UnwindWeAllocatedDiskSpace = TRUE;

            //
            //  Inform the cache manager of the new allocation
            //

            Dcb->Header.FileSize.LowPart =
                Dcb->Header.AllocationSize.LowPart;

            CcSetFileSizes( Dcb->Specific.Dcb.DirectoryFile,
                            (PCC_FILE_SIZES)&Dcb->Header.AllocationSize );

            //
            //  Set up the Bitmap buffer if it is not big enough already
            //

            FatCheckFreeDirentBitmap( IrpContext, Dcb );

            //
            //  The newly allocated clusters should be zeroed starting at
            //  the previous allocation size
            //

            Zero = TRUE;
            Vbo.QuadPart = InitialAllocation;
            ByteCount = Dcb->Header.AllocationSize.LowPart - InitialAllocation;
        }

        //
        // Call the Cache Manager to attempt the transfer, going one cluster
        // at a time to avoid pinning across a page boundary.
        //

        ClusterSize =
            1 << Dcb->Vcb->AllocationSupport.LogOfBytesPerCluster;

        while (ByteCount > 0) {

            ULONG BytesToPin;

            *Bcb = NULL;

            if (ByteCount > ClusterSize) {
                BytesToPin = ClusterSize;
            } else {
                BytesToPin = ByteCount;
            }

            ASSERT( (Vbo.QuadPart / ClusterSize) ==
                    (Vbo.QuadPart + BytesToPin - 1)/ClusterSize );

            if (!CcPinRead( Dcb->Specific.Dcb.DirectoryFile,
                            &Vbo,
                            BytesToPin,
                            BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                            Bcb,
                            &LocalBuffer )) {
    
                //
                // Could not read the data without waiting (cache miss).
                //

                FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  Update our caller with the beginning of their request.
            //
            
            if (*Buffer == NULL) {

                *Buffer = LocalBuffer;
            }

            DbgDoit( IrpContext->PinCount += 1 )

            if (Zero) {
                
                //
                //  We set this guy dirty right now so that we can raise CANT_WAIT when
                //  it needs to be done.  It'd be beautiful if we could noop the read IO
                //  since we know we don't care about it.
                //
                
                RtlZeroMemory( LocalBuffer, BytesToPin );
                CcSetDirtyPinnedData( *Bcb, NULL );
            }

            ByteCount -= BytesToPin;
            Vbo.QuadPart += BytesToPin;


            if (ByteCount > 0) {

                FatUnpinBcb( IrpContext, *Bcb );
            }
        }

        //
        //  This lets us get the data pinned until we complete the request
        //  and writes the dirty bit through to the disk.
        //

        FatSetDirtyBcb( IrpContext, *Bcb, Dcb->Vcb, Reversible );

        *Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        DebugUnwind( FatPrepareWriteDirectoryFile );

        if (_SEH2_AbnormalTermination()) {

            //
            //  These steps are carefully arranged - FatTruncateFileAllocation can raise.
            //  Make sure we unpin the buffer.  If FTFA raises, the effect should be benign.
            //
            
            FatUnpinBcb(IrpContext, *Bcb);
            
            if (UnwindWeAllocatedDiskSpace == TRUE) {

                //
                //  Inform the cache manager of the change.
                //

                FatTruncateFileAllocation( IrpContext, Dcb, InitialAllocation );

                Dcb->Header.FileSize.LowPart =
                    Dcb->Header.AllocationSize.LowPart;

                CcSetFileSizes( Dcb->Specific.Dcb.DirectoryFile,
                                (PCC_FILE_SIZES)&Dcb->Header.AllocationSize );
            }
        }

        DebugTrace(-1, Dbg, "FatPrepareWriteDirectoryFile -> (VOID), *Bcb = %08lx\n", *Bcb);
    } _SEH2_END;

    return;
}


#if DBG
BOOLEAN FatDisableParentCheck = 0;

BOOLEAN
FatIsCurrentOperationSynchedForDcbTeardown (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )
{
    PIRP Irp = IrpContext->OriginatingIrp;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation( Irp ) ;
#ifndef __REACTOS__
    PFILE_OBJECT FileObject = Stack->FileObject;
#endif
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PFILE_OBJECT ToCheck[3];
    ULONG Index = 0;

    PAGED_CODE();
    
    //
    //  While mounting, we're OK without having to own anything.
    //
    
    if (Stack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
        Stack->MinorFunction == IRP_MN_MOUNT_VOLUME) {

        return TRUE;
    }
    
    //
    //  With the Vcb held, the close path is blocked out.
    //
    
    if (ExIsResourceAcquiredSharedLite( &Dcb->Vcb->Resource ) ||
        ExIsResourceAcquiredExclusiveLite( &Dcb->Vcb->Resource )) {

        return TRUE;
    }
    
    //
    //  Accept this assertion at face value.  It comes from GetDirentForFcbOrDcb,
    //  and is reliable.
    //
    
    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_PARENT_BY_CHILD )) {

        return TRUE;
    }

    //
    //  Determine which fileobjects are around on this operation.
    //

    if (Stack->MajorFunction == IRP_MJ_SET_INFORMATION &&
        Stack->Parameters.SetFile.FileObject) {

        ToCheck[Index++] = Stack->Parameters.SetFile.FileObject;
    }

    if (Stack->FileObject) {
        
        ToCheck[Index++] = Stack->FileObject;
    }

    ToCheck[Index] = NULL;
    
    //
    //  If the fileobjects we have are for this dcb or a child of it, we are
    //  also guaranteed that this dcb isn't going anywhere (even without
    //  the Vcb).
    //
    
    for (Index = 0; ToCheck[Index] != NULL; Index++) {
    
        (VOID) FatDecodeFileObject( ToCheck[Index], &Vcb, &Fcb, &Ccb );

        while ( Fcb ) {
    
            if (Fcb == Dcb) {
    
                return TRUE;
            }
    
            Fcb = Fcb->ParentDcb;
        }
    }

    return FatDisableParentCheck;
}
#endif // DBG

VOID
FatOpenDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine opens a new directory file if one is not already open.

Arguments:

    Dcb - Pointer to the DCB for the directory

Return Value:

    None.

--*/

{
    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatOpenDirectoryFile\n", 0);
    DebugTrace( 0, Dbg, "Dcb = %08lx\n", Dcb);

    //
    //  If we don't have some hold on this Dcb (there are several ways), there is nothing
    //  to prevent child files from closing and tearing this branch of the tree down in the
    //  midst of our slapping this reference onto it.
    //
    //  I really wish we had a proper Fcb synchronization model (like CDFS/UDFS/NTFS).
    //
    
    ASSERT( FatIsCurrentOperationSynchedForDcbTeardown( IrpContext, Dcb ));

    //
    //  If we haven't yet set the correct AllocationSize, do so.
    //

    if (Dcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

        FatLookupFileAllocationSize( IrpContext, Dcb );

        Dcb->Header.FileSize.LowPart =
        Dcb->Header.AllocationSize.LowPart;
    }

    //
    //  Setup the Bitmap buffer if it is not big enough already
    //

    FatCheckFreeDirentBitmap( IrpContext, Dcb );

    //
    //  Check if we need to create a directory file.
    //
    //  We first do a spot check and then synchronize and check again.
    //

    if (Dcb->Specific.Dcb.DirectoryFile == NULL) {

        PFILE_OBJECT DirectoryFileObject = NULL;

        FatAcquireDirectoryFileMutex( Dcb->Vcb );

        _SEH2_TRY {

            if (Dcb->Specific.Dcb.DirectoryFile == NULL) {

                PDEVICE_OBJECT RealDevice;

                //
                //  Create the special file object for the directory file, and set
                //  up its pointers back to the Dcb and the section object pointer.
                //  Note that setting the DirectoryFile pointer in the Dcb has
                //  to be the last thing done.
                //
                //  Preallocate a close context since we have no Ccb for this object.
                //

                RealDevice = Dcb->Vcb->CurrentDevice;

                DirectoryFileObject = IoCreateStreamFileObject( NULL, RealDevice );
                FatPreallocateCloseContext();

                FatSetFileObject( DirectoryFileObject,
                                  DirectoryFile,
                                  Dcb,
                                  NULL );

                //
                //  Remember this internal open.
                //

#ifndef __REACTOS__
                InterlockedIncrement( &(Dcb->Vcb->InternalOpenCount) );
#else
                InterlockedIncrement( (LONG*)&(Dcb->Vcb->InternalOpenCount) );
#endif

                //
                //  If this is the root directory, it is also a residual open.
                //

                if (NodeType( Dcb ) == FAT_NTC_ROOT_DCB) {
                    
#ifndef __REACTOS__
                    InterlockedIncrement( &(Dcb->Vcb->ResidualOpenCount) );
#else
                    InterlockedIncrement( (LONG*)&(Dcb->Vcb->ResidualOpenCount) );
#endif
                }

                DirectoryFileObject->SectionObjectPointer = &Dcb->NonPaged->SectionObjectPointers;

                DirectoryFileObject->ReadAccess = TRUE;
                DirectoryFileObject->WriteAccess = TRUE;
                DirectoryFileObject->DeleteAccess = TRUE;

#ifndef __REACTOS__
                InterlockedIncrement( &Dcb->Specific.Dcb.DirectoryFileOpenCount );
#else
                InterlockedIncrement( (LONG*)&Dcb->Specific.Dcb.DirectoryFileOpenCount );
#endif

                Dcb->Specific.Dcb.DirectoryFile = DirectoryFileObject;
                
                //
                //  Indicate we're happy with the fileobject now.
                //

                DirectoryFileObject = NULL;
            }

        } _SEH2_FINALLY {

            FatReleaseDirectoryFileMutex( Dcb->Vcb );

            //
            //  Rip the object up if we couldn't get the close context.
            //
            
            if (DirectoryFileObject) {
                
                ObDereferenceObject( DirectoryFileObject );
            }
        } _SEH2_END;
    }

    //
    //  Finally check if we need to initialize the Cache Map for the
    //  directory file.  The size of the section we are going to map
    //  the current allocation size for the directory.  Note that the
    //  cache manager will provide syncronization for us.
    //

    if ( Dcb->Specific.Dcb.DirectoryFile->PrivateCacheMap == NULL ) {

        Dcb->Header.ValidDataLength = FatMaxLarge;
        Dcb->ValidDataToDisk = MAXULONG;

        CcInitializeCacheMap( Dcb->Specific.Dcb.DirectoryFile,
                              (PCC_FILE_SIZES)&Dcb->Header.AllocationSize,
                              TRUE,
                              &FatData.CacheManagerNoOpCallbacks,
                              Dcb );
    }

    DebugTrace(-1, Dbg, "FatOpenDirectoryFile -> VOID\n", 0);

    return;
}


PFILE_OBJECT
FatOpenEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB EaFcb
    )

/*++

Routine Description:

    This routine opens the Ea file.

Arguments:

    EaFcb - Pointer to the Fcb for the Ea file.

Return Value:

    Pointer to the new file object.

--*/

{
    PFILE_OBJECT EaFileObject = NULL;
    PDEVICE_OBJECT RealDevice;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatOpenEaFile\n", 0);
    DebugTrace( 0, Dbg, "EaFcb = %08lx\n", EaFcb);

    //
    //  Create the special file object for the ea file, and set
    //  up its pointers back to the Fcb and the section object pointer
    //

    RealDevice = EaFcb->Vcb->CurrentDevice;

    EaFileObject = IoCreateStreamFileObject( NULL, RealDevice );

    _SEH2_TRY {

        FatPreallocateCloseContext();

        FatSetFileObject( EaFileObject,
                          EaFile,
                          EaFcb,
                          NULL );

        //
        //  Remember this internal, residual open.
        //

#ifndef __REACTOS__
        InterlockedIncrement( &(EaFcb->Vcb->InternalOpenCount) );
        InterlockedIncrement( &(EaFcb->Vcb->ResidualOpenCount) );
#else
        InterlockedIncrement( (LONG*)&(EaFcb->Vcb->InternalOpenCount) );
        InterlockedIncrement( (LONG*)&(EaFcb->Vcb->ResidualOpenCount) );
#endif

        EaFileObject->SectionObjectPointer = &EaFcb->NonPaged->SectionObjectPointers;

        EaFileObject->ReadAccess = TRUE;
        EaFileObject->WriteAccess = TRUE;

        //
        //  Finally check if we need to initialize the Cache Map for the
        //  ea file.  The size of the section we are going to map
        //  the current allocation size for the Fcb.
        //

        EaFcb->Header.ValidDataLength = FatMaxLarge;

        CcInitializeCacheMap( EaFileObject,
                              (PCC_FILE_SIZES)&EaFcb->Header.AllocationSize,
                              TRUE,
                              &FatData.CacheManagerCallbacks,
                              EaFcb );

        CcSetAdditionalCacheAttributes( EaFileObject, TRUE, TRUE );
    
    } _SEH2_FINALLY {

        //
        //  Drop the fileobject if we're raising.  Two cases: couldn't get
        //  the close context, and it is still an UnopenedFileObject, or
        //  we lost trying to build the cache map - in which case we're
        //  OK for the close context if we have to.
        //
        
        if (_SEH2_AbnormalTermination()) {
            
            ObDereferenceObject( EaFileObject );
        }
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatOpenEaFile -> %08lx\n", EaFileObject);

    UNREFERENCED_PARAMETER( IrpContext );

    return EaFileObject;
}


VOID
FatCloseEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushFirst
    )

/*++

Routine Description:

    This routine shuts down the ea file.  Usually this is required when the volume
    begins to leave the system: after verify, dismount, deletion, pnp.
    
Arguments:

    Vcb - the volume to close the ea file on
    
    FlushFirst - whether the file should be flushed
    
Return Value:

    None. As a side effect, the EA fileobject in the Vcb is cleared.
    
    Caller must have the Vcb exclusive.
    
--*/

{
    PFILE_OBJECT EaFileObject = Vcb->VirtualEaFile;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatCloseEaFile\n", 0);
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb);

    ASSERT( FatVcbAcquiredExclusive(IrpContext, Vcb) );

    if (EaFileObject != NULL) {

        EaFileObject = Vcb->VirtualEaFile;

        if (FlushFirst) {

            CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );
        }

        Vcb->VirtualEaFile = NULL;

        //
        //  Empty the Mcb for the Ea file.
        //

        FatRemoveMcbEntry( Vcb, &Vcb->EaFcb->Mcb, 0, 0xFFFFFFFF );

        //
        //  Uninitialize the cache for this file object and dereference it.
        //

        FatSyncUninitializeCacheMap( IrpContext, EaFileObject );

        ObDereferenceObject( EaFileObject );
    }
    
    DebugTrace(-1, Dbg, "FatCloseEaFile -> %08lx\n", EaFileObject);
}


VOID
FatSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PVCB Vcb OPTIONAL,
    IN BOOLEAN Reversible
    )

/*++

Routine Description:

    This routine saves a reference to the bcb in the irp context and
    sets the bcb dirty.  This will have the affect of keeping the page in
    memory until we complete the request

    In addition, a DPC is set to fire in 5 seconds (or if one is pending,
    pushed back 5 seconds) to mark the volume clean.

Arguments:

    Bcb - Supplies the Bcb being set dirty
    
    Vcb - Supplies the volume being marked dirty
    
    Reversible - Supplies TRUE if the specified range of bcb should be repinned
        so that the changes can be reversed in a controlled fashion if errors
        are encountered.
    
Return Value:

    None.

--*/

{
    DebugTrace(+1, Dbg, "FatSetDirtyBcb\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Bcb        = %08lx\n", Bcb );
    DebugTrace( 0, Dbg, "Vcb        = %08lx\n", Vcb );

    //
    //  Repin the bcb as required
    //

    if (Reversible) {
    
        FatRepinBcb( IrpContext, Bcb );
    }

    //
    //  Set the bcb dirty
    //

    CcSetDirtyPinnedData( Bcb, NULL );

    //
    //  If volume dirtying isn't disabled for this operation (for
    //  instance, when we're changing the dirty state), set the
    //  volume dirty if we were given a Vcb that we want to perform
    //  clean volume processing on, and return.
    //
    //  As a historical note, we used to key off of the old floppy
    //  (now deferred flush) bit to disable dirtying behavior.  Since
    //  hotpluggable media can still be yanked while operations are
    //  in flight, recognize that its really the case that FAT12
    //  doesn't have the dirty bit.
    //

    if ( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_DIRTY) &&
         ARGUMENT_PRESENT(Vcb) &&
         !FatIsFat12(Vcb)) {

        KIRQL SavedIrql;

        BOOLEAN SetTimer;

        LARGE_INTEGER TimeSincePreviousCall;
        LARGE_INTEGER CurrentTime;

        //
        //  "Borrow" the irp context spinlock.
        //

        KeQuerySystemTime( &CurrentTime );

        KeAcquireSpinLock( &FatData.GeneralSpinLock, &SavedIrql );

        TimeSincePreviousCall.QuadPart =
                CurrentTime.QuadPart - Vcb->LastFatMarkVolumeDirtyCall.QuadPart;

        //
        //  If more than one second has elapsed since the prior call
        //  to here, bump the timer up again and see if we need to
        //  physically mark the volume dirty.
        //

        if ( (TimeSincePreviousCall.HighPart != 0) ||
             (TimeSincePreviousCall.LowPart > (1000 * 1000 * 10)) ) {

            SetTimer = TRUE;

        } else {

            SetTimer = FALSE;
        }

        KeReleaseSpinLock( &FatData.GeneralSpinLock, SavedIrql );

        if ( SetTimer ) {

            LARGE_INTEGER CleanVolumeTimer;

            //
            //  We use a shorter volume clean timer for hot plug volumes.
            //
            
            CleanVolumeTimer.QuadPart = FlagOn( Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH)
                                        ? (LONG)-1500*1000*10
                                        : (LONG)-8*1000*1000*10;

            (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );
            (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );

            //
            //  We have now synchronized with anybody clearing the dirty
            //  flag, so we can now see if we really have to actually write
            //  out the physical bit.
            //

            if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY) ) {

                //
                //  We want to really mark the volume dirty now.
                //

                if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                    FatMarkVolume( IrpContext, Vcb, VolumeDirty );
                }

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );

                //
                //  Lock the volume if it is removable.
                //

                if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA)) {

                    FatToggleMediaEjectDisable( IrpContext, Vcb, TRUE );
                }
            }

            KeAcquireSpinLock( &FatData.GeneralSpinLock, &SavedIrql );

            KeQuerySystemTime( &Vcb->LastFatMarkVolumeDirtyCall );

            KeReleaseSpinLock( &FatData.GeneralSpinLock, SavedIrql );

            KeSetTimer( &Vcb->CleanVolumeTimer,
                        CleanVolumeTimer,
                        &Vcb->CleanVolumeDpc );
        }
    }

    DebugTrace(-1, Dbg, "FatSetDirtyBcb -> VOID\n", 0 );
}


VOID
FatRepinBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb
    )

/*++

Routine Description:

    This routine saves a reference to the bcb in the irp context. This will
    have the affect of keeping the page in memory until we complete the
    request

Arguments:

    Bcb - Supplies the Bcb being referenced

Return Value:

    None.

--*/

{
    PREPINNED_BCBS Repinned;
    ULONG i;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatRepinBcb\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Bcb        = %08lx\n", Bcb );

    //
    //  The algorithm is to search the list of repinned records until
    //  we either find a match for the bcb or we find a null slot.
    //

    Repinned = &IrpContext->Repinned;

    while (TRUE) {

        //
        //  For every entry in the repinned record check if the bcb's
        //  match or if the entry is null.  If the bcb's match then
        //  we've done because we've already repinned this bcb, if
        //  the entry is null then we know, because it's densely packed,
        //  that the bcb is not in the list so add it to the repinned
        //  record and repin it.
        //

        for (i = 0; i < REPINNED_BCBS_ARRAY_SIZE; i += 1) {

            if (Repinned->Bcb[i] == Bcb) {

                DebugTrace(-1, Dbg, "FatRepinBcb -> VOID\n", 0 );
                return;
            }

            if (Repinned->Bcb[i] == NULL) {

                Repinned->Bcb[i] = Bcb;
                CcRepinBcb( Bcb );

                DebugTrace(-1, Dbg, "FatRepinBcb -> VOID\n", 0 );
                return;
            }
        }

        //
        //  We finished checking one repinned record so now locate the next
        //  repinned record,  If there isn't one then allocate and zero out
        //  a new one.
        //

        if (Repinned->Next == NULL) {

            Repinned->Next = FsRtlAllocatePoolWithTag( PagedPool,
                                                       sizeof(REPINNED_BCBS),
                                                       TAG_REPINNED_BCB );

            RtlZeroMemory( Repinned->Next, sizeof(REPINNED_BCBS) );
        }

        Repinned = Repinned->Next;
    }
}


VOID
FatUnpinRepinnedBcbs (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine frees all of the repinned bcbs, stored in an IRP context.

Arguments:

Return Value:

    None.

--*/

{
    IO_STATUS_BLOCK RaiseIosb;
    PREPINNED_BCBS Repinned;
    BOOLEAN WriteThroughToDisk;
    PFILE_OBJECT FileObject = NULL;
    BOOLEAN ForceVerify = FALSE;
    ULONG i;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatUnpinRepinnedBcbs\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );

    //
    //  The algorithm for this procedure is to scan the entire list of
    //  repinned records unpinning any repinned bcbs.  We start off
    //  with the first record in the irp context, and while there is a
    //  record to scan we do the following loop.
    //

    Repinned = &IrpContext->Repinned;
    RaiseIosb.Status = STATUS_SUCCESS;

    //
    //  If the request is write through or the media is deferred flush,
    //  unpin the bcb's write through.
    //

    WriteThroughToDisk = (BOOLEAN) (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH) &&
                                    IrpContext->Vcb != NULL &&
                                    (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH) ||
                                     FlagOn(IrpContext->Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH)));

    while (Repinned != NULL) {

        //
        //  For every non-null entry in the repinned record unpin the
        //  repinned entry.
        //
        //  If the this is removable media (therefore all requests write-
        //  through) and the write fails, purge the cache so that we throw
        //  away the modifications as we will be returning an error to the
        //  user.
        //

        for (i = 0; i < REPINNED_BCBS_ARRAY_SIZE; i += 1) {

            if (Repinned->Bcb[i] != NULL) {

                IO_STATUS_BLOCK Iosb;

                if (WriteThroughToDisk && 
                    FlagOn(IrpContext->Vcb->VcbState, VCB_STATE_FLAG_DEFERRED_FLUSH)) {

                    FileObject = CcGetFileObjectFromBcb( Repinned->Bcb[i] );
                }

                CcUnpinRepinnedBcb( Repinned->Bcb[i],
                                    WriteThroughToDisk,
                                    &Iosb );

                if ( !NT_SUCCESS(Iosb.Status) ) {

                    if (RaiseIosb.Status == STATUS_SUCCESS) {

                        RaiseIosb = Iosb;
                    }

                    //
                    //  If this was a writethrough device, purge the cache,
                    //  except for Irp major codes that either don't handle
                    //  the error paths correctly or are simple victims like
                    //  cleanup.c.
                    //

                    if (FileObject &&
                        (IrpContext->MajorFunction != IRP_MJ_CLEANUP) &&
                        (IrpContext->MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
                        (IrpContext->MajorFunction != IRP_MJ_SET_INFORMATION)) {

                        //
                        //  The call to CcPurgeCacheSection() below will
                        //  purge the entire file from memory.  It will also
                        //  block until all the file's BCB's are pinned.
                        //
                        //  We end up in a deadlock situation of there
                        //  are any other pinned BCB's in this IRP context
                        //  so the first thing we do is search the list
                        //  for BCB's pinned in the same file and unpin
                        //  them.
                        //
                        //  We are probably not going to lose data because
                        //  it's safe to assume that all flushes will
                        //  fail after the first one fails.
                        //

                        ULONG j;

                        for (j = i + 1; j < REPINNED_BCBS_ARRAY_SIZE; j++) {

                            if (Repinned->Bcb[j] != NULL) {
                                
                                if (CcGetFileObjectFromBcb( Repinned->Bcb[j] ) == FileObject) {

                                    CcUnpinRepinnedBcb( Repinned->Bcb[j],
                                                        FALSE,
                                                        &Iosb );
                    
                                    Repinned->Bcb[j] = NULL;
                                }
                            }
                        }
                        
                        CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                             NULL,
                                             0,
                                             FALSE );

                        //
                        //  Force a verify operation here since who knows
                        //  what state things are in.
                        //

                        ForceVerify = TRUE;
                    }
                }

                Repinned->Bcb[i] = NULL;

            }
        }

        //
        //  Now find the next repinned record in the list, and possibly
        //  delete the one we've just processed.
        //

        if (Repinned != &IrpContext->Repinned) {

            PREPINNED_BCBS Saved;

            Saved = Repinned->Next;
            ExFreePool( Repinned );
            Repinned = Saved;

        } else {

            Repinned = Repinned->Next;
            IrpContext->Repinned.Next = NULL;
        }
    }

    //
    //  Now if we weren't completely successful in the our unpin
    //  then raise the iosb we got
    //

    if (!NT_SUCCESS(RaiseIosb.Status)) {

        if (ForceVerify && FileObject) {

            SetFlag(FileObject->DeviceObject->Flags, DO_VERIFY_VOLUME);

            IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                          FileObject->DeviceObject );
        }

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_RAISE )) {
            
            IrpContext->OriginatingIrp->IoStatus = RaiseIosb;
            FatNormalizeAndRaiseStatus( IrpContext, RaiseIosb.Status );
        }
    }

    DebugTrace(-1, Dbg, "FatUnpinRepinnedBcbs -> VOID\n", 0 );

    return;
}


FINISHED
FatZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingZero,
    IN ULONG ByteCount
    )

/*++

    **** Temporary function - Remove when CcZeroData is capable of handling
    non sector aligned requests.

--*/
{
#ifndef __REACTOS__
    LARGE_INTEGER ZeroStart = {0,0};
    LARGE_INTEGER BeyondZeroEnd = {0,0};
#else
    LARGE_INTEGER ZeroStart = {{0}};
    LARGE_INTEGER BeyondZeroEnd = {{0}};
#endif

    ULONG SectorSize;

    BOOLEAN Finished;

    PAGED_CODE();
    
    SectorSize = (ULONG)Vcb->Bpb.BytesPerSector;

    ZeroStart.LowPart = (StartingZero + (SectorSize - 1)) & ~(SectorSize - 1);

    //
    //  Detect overflow if we were asked to zero in the last sector of the file,
    //  which must be "zeroed" already (or we're in trouble).
    //
    
    if (StartingZero != 0 && ZeroStart.LowPart == 0) {
        
        return TRUE;
    }

    //
    //  Note that BeyondZeroEnd can take the value 4gb.
    //
    
    BeyondZeroEnd.QuadPart = ((ULONGLONG) StartingZero + ByteCount + (SectorSize - 1))
                             & (~((LONGLONG) SectorSize - 1));

    //
    //  If we were called to just zero part of a sector we are in trouble.
    //
    
    if ( ZeroStart.QuadPart == BeyondZeroEnd.QuadPart ) {

        return TRUE;
    }

    Finished = CcZeroData( FileObject,
                           &ZeroStart,
                           &BeyondZeroEnd,
                           BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    return Finished;
}


NTSTATUS
FatCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl read and write
    requests.  It should be called only from FatFsdRead and FatFsdWrite.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_PENDING or STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatCompleteMdl\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    switch( IrpContext->MajorFunction ) {

    case IRP_MJ_READ:

        CcMdlReadComplete( FileObject, Irp->MdlAddress );
        break;

    case IRP_MJ_WRITE:

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT ));

        CcMdlWriteComplete( FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress );

        Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    default:

        DebugTrace( DEBUG_TRACE_ERROR, 0, "Illegal Mdl Complete.\n", 0);
        FatBugCheck( IrpContext->MajorFunction, 0, 0 );
    }

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatCompleteMdl -> STATUS_SUCCESS\n", 0 );

    return STATUS_SUCCESS;
}

VOID
FatSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine performs a CcUnitializeCacheMap to LargeZero synchronously.  That
    is it waits on the Cc event.  This call is useful when we want to be certain
    when a close will actually some in.

Return Value:

    None.

--*/

{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;

    PAGED_CODE();
    
    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &FatLargeZero,
                            &UninitializeCompleteEvent );

    //
    //  Now wait for the cache manager to finish purging the file.
    //  This will garentee that Mm gets the purge before we
    //  delete the Vcb.
    //

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT(WaitStatus == STATUS_SUCCESS);
}

VOID
FatPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine pins data that was previously mapped before setting it dirty.

Arguments:

    Dcb - Pointer to the DCB for the directory

    StartingVbo - The virtual offset of the first desired byte

    ByteCount - Number of bytes desired

    Bcb - Returns a pointer to the BCB which is valid until unpinned

--*/

{
    LARGE_INTEGER Vbo;

    PAGED_CODE();
    
    DebugTrace(+1, Dbg, "FatPinMappedData\n", 0);
    DebugTrace( 0, Dbg, "Dcb         = %08lx\n", Dcb);
    DebugTrace( 0, Dbg, "StartingVbo = %08lx\n", StartingVbo);
    DebugTrace( 0, Dbg, "ByteCount   = %08lx\n", ByteCount);

    //
    //  Call the Cache manager to perform the operation.
    //

    Vbo.QuadPart = StartingVbo;

    if (!CcPinMappedData( Dcb->Specific.Dcb.DirectoryFile,
                          &Vbo,
                          ByteCount,
                          BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                          Bcb )) {

        //
        // Could not pin the data without waiting (cache miss).
        //

        FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    DebugTrace(-1, Dbg, "FatReadDirectoryFile -> VOID, *BCB = %08lx\n", *Bcb);

    return;
}


