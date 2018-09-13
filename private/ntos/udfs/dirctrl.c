/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Udfs called
    by the Fsd/Fsp dispatch drivers.

Author:

    Dan Lovinger    [DanLo]     27-Nov-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_DIRCTRL)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_DIRCTRL)

//
//  Local structures
//

//
//  The following is used for the more complete enumeration required in the DirectoryControl path
//  and encapsulates the structures for enumerating both directories and ICBs, as well as converted
//  data from the ICB.
//

typedef struct _COMPOUND_DIR_ENUM_CONTEXT {

    //
    //  Standard enumeration contexts.  For this enumeration we walk the directory and lift the
    //  associated ICB for each entry.
    //
    
    DIR_ENUM_CONTEXT DirContext;
    ICB_SEARCH_CONTEXT IcbContext;

    //
    //  Timestamps converted from the ICB into NT-native form.
    //

    TIMESTAMP_BUNDLE Timestamps;

    //
    //  File index corresponding to the current position in the enumeration.
    //

    LARGE_INTEGER FileIndex;

} COMPOUND_DIR_ENUM_CONTEXT, *PCOMPOUND_DIR_ENUM_CONTEXT;

//
//  Local macros
//

//
//  Constants defining the space of FileIndices for directory enumeration.
//

//
//  The virtual (synthesized) file indices
//

#define UDF_FILE_INDEX_VIRTUAL_SELF         0

//
//  The file index where the physical directory entries begin
//

#define UDF_FILE_INDEX_PHYSICAL             1

//
//  Provide initialization and cleanup for compound enumeration contexts.
//

INLINE
VOID
UdfInitializeCompoundDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    )
{

    UdfInitializeDirContext( IrpContext, &CompoundDirContext->DirContext );
    UdfFastInitializeIcbContext( IrpContext, &CompoundDirContext->IcbContext );

    RtlZeroMemory( &CompoundDirContext->Timestamps, sizeof( TIMESTAMP_BUNDLE ));

    CompoundDirContext->FileIndex.QuadPart = 0;
}

INLINE
VOID
UdfCleanupCompoundDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    )
{

    UdfCleanupDirContext( IrpContext, &CompoundDirContext->DirContext );
    UdfCleanupIcbContext( IrpContext, &CompoundDirContext->IcbContext );
}

//
//  UDF directories are unsorted (UDF 1.0.1 2.3.5.3) and do not contain self
//  entries.  For directory enumeration we must provide a way for a restart to
//  occur at a random entry (SL_INDEX_SPECIFIED), but the key used is only
//  32bits.  Since the directory is unsorted, the filename is unsuitable for
//  quickly finding a restart point (even assuming that it was sorted,
//  discovering a directory entry is still not fast).  Additionally, we must
//  synthesize the self-entry.  So, here is how we map the space of file
//  indices to directory entries:
//
//    File Index              Directory Entry
//  
//    0                       self ('.')
//    1                       at byte offset 0 in the stream
//    N                       at byte offset N-1 in the stream
//  
//  The highest 32bit FileIndex returned will be stashed in the Ccb.
//  
//  For FileIndex > 2^32, we will return FileIndex 0 in the query structure.
//  On a restart, we will notice a FileIndex of zero and use the saved high
//  32bit FileIndex as the starting point for a linear scan to find the named
//  directory entry in the restart request.  In this way we only penalize the
//  improbable case of a directory stream > 2^32 bytes.
//
//  The following inline routines assist with this mapping.
//

INLINE
LONGLONG
UdfFileIndexToPhysicalOffset(
    LONGLONG FileIndex
    )
{

    return FileIndex - UDF_FILE_INDEX_PHYSICAL;
}

INLINE
LONGLONG
UdfPhysicalOffsetToFileIndex(
    LONGLONG PhysicalOffset
    )
{

    return PhysicalOffset + UDF_FILE_INDEX_PHYSICAL;
}

INLINE
BOOLEAN
UdfIsFileIndexVirtual(
   LONGLONG FileIndex
   )
{

    return FileIndex < UDF_FILE_INDEX_PHYSICAL;
}

//
//  Local support routines
//

NTSTATUS
UdfQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb
    );

NTSTATUS
UdfNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    );

VOID
UdfInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    OUT PBOOLEAN ReturnNextEntry,
    OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    );

BOOLEAN
UdfEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    IN BOOLEAN ReturnNextEntry
    );

VOID
UdfLookupFileEntryInEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    );

BOOLEAN
UdfLookupInitialFileIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    IN PLONGLONG InitialIndex
    );

BOOLEAN
UdfLookupNextFileIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonDirControl)
#pragma alloc_text(PAGE, UdfEnumerateIndex)
#pragma alloc_text(PAGE, UdfInitializeEnumeration)
#pragma alloc_text(PAGE, UdfLookupFileEntryInEnumeration)
#pragma alloc_text(PAGE, UdfLookupInitialFileIndex)
#pragma alloc_text(PAGE, UdfLookupNextFileIndex)
#pragma alloc_text(PAGE, UdfNotifyChangeDirectory)
#pragma alloc_text(PAGE, UdfQueryDirectory)
#endif


NTSTATUS
UdfCommonDirControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the entry point for the directory control operations.  These
    are directory enumerations and directory notify calls.  We verify the
    user's handle is for a directory and then call the appropriate routine.

Arguments:

    Irp - Irp for this request.

Return Value:

    NTSTATUS - Status returned from the lower level routines.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the user file object and fail this request if it is not
    //  a user directory.
    //

    if (UdfDecodeFileObject( IrpSp->FileObject,
                             &Fcb,
                             &Ccb ) != UserDirectoryOpen) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = UdfQueryDirectory( IrpContext, Irp, IrpSp, Fcb, Ccb );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = UdfNotifyChangeDirectory( IrpContext, Irp, IrpSp, Ccb );
        break;

    default:

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routines
//

NTSTATUS
UdfQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing of enqueuing the input Irp.  We store the state of the
    search in the Ccb.

Arguments:

    Irp - Supplies the Irp to process

    IrpSp - Stack location for this Irp.

    Fcb - Fcb for this directory.

    Ccb - Ccb for this directory open.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Information = 0;

    ULONG LastEntry = 0;
    ULONG NextEntry = 0;

    ULONG FileNameBytes;
    ULONG BytesConverted;

    COMPOUND_DIR_ENUM_CONTEXT CompoundDirContext;

    PNSR_FID ThisFid;
    PICBFILE ThisFe;
    
    BOOLEAN InitialQuery;
    BOOLEAN ReturnNextEntry;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN Found;

    PCHAR UserBuffer;
    ULONG BytesRemainingInBuffer;

    ULONG BaseLength;

    PFILE_BOTH_DIR_INFORMATION DirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    PAGED_CODE();

    //
    //  Check if we support this search mode.  Also remember the size of the base part of
    //  each of these structures.
    //

    switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {

    case FileDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION,
                                   FileName[0] );
        break;

    case FileFullDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION,
                                   FileName[0] );
        break;

    case FileNamesInformation:

        BaseLength = FIELD_OFFSET( FILE_NAMES_INFORMATION,
                                   FileName[0] );
        break;

    case FileBothDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION,
                                   FileName[0] );
        break;

    default:

        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_INFO_CLASS );
        return STATUS_INVALID_INFO_CLASS;
    }

    //
    //  Get the user buffer.
    //

    UdfMapUserBuffer( IrpContext, &UserBuffer);

    //
    //  Initialize our search context.
    //

    UdfInitializeCompoundDirContext( IrpContext, &CompoundDirContext );
    
    //
    //  Acquire the directory.
    //

    UdfAcquireFileShared( IrpContext, Fcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify the Fcb is still good.
        //

        UdfVerifyFcbOperation( IrpContext, Fcb );

        //
        //  Start by getting the initial state for the enumeration.  This will set up the Ccb with
        //  the initial search parameters and let us know the starting offset in the directory
        //  to search.
        //

        UdfInitializeEnumeration( IrpContext,
                                  IrpSp,
                                  Fcb,
                                  Ccb,
                                  &CompoundDirContext,
                                  &ReturnNextEntry,
                                  &ReturnSingleEntry,
                                  &InitialQuery );

        //
        //  At this point we are about to enter our query loop.  We have
        //  determined the index into the directory file to begin the
        //  search.  LastEntry and NextEntry are used to index into the user
        //  buffer.  LastEntry is the last entry we've added, NextEntry is
        //  current one we're working on.  If NextEntry is non-zero, then
        //  at least one entry was added.
        //

        while (TRUE) {

            //
            //  If the user had requested only a single match and we have
            //  returned that, then we stop at this point.  We update the Ccb with
            //  the status based on the last entry returned.
            //

            if ((NextEntry != 0) && ReturnSingleEntry) {

                try_leave( Status );
            }

            //
            //  We try to locate the next matching dirent.  Our search if based on a starting
            //  dirent offset, whether we should return the current or next entry, whether
            //  we should be doing a short name search and finally whether we should be
            //  checking for a version match.
            //

            Found = UdfEnumerateIndex( IrpContext, Ccb, &CompoundDirContext, ReturnNextEntry );

            //
            //  Initialize the value for the next search.
            //

            ReturnNextEntry = TRUE;

            //
            //  If we didn't receive a dirent, then we are at the end of the
            //  directory.  If we have returned any files, we exit with
            //  success, otherwise we return STATUS_NO_MORE_FILES.
            //

            if (!Found) {

                if (NextEntry == 0) {

                    Status = STATUS_NO_MORE_FILES;

                    if (InitialQuery) {

                        Status = STATUS_NO_SUCH_FILE;
                    }
                }

                try_leave( Status );
            }

            //
            //  Remember the dirent/file entry for the file we just found.
            //

            ThisFid = CompoundDirContext.DirContext.Fid;

            //
            //  Here are the rules concerning filling up the buffer:
            //
            //  1.  The Io system garentees that there will always be
            //      enough room for at least one base record.
            //
            //  2.  If the full first record (including file name) cannot
            //      fit, as much of the name as possible is copied and
            //      STATUS_BUFFER_OVERFLOW is returned.
            //
            //  3.  If a subsequent record cannot completely fit into the
            //      buffer, none of it (as in 0 bytes) is copied, and
            //      STATUS_SUCCESS is returned.  A subsequent query will
            //      pick up with this record.
            //

            //
            //  We can look directly at the dirent that we found.
            //

            FileNameBytes = CompoundDirContext.DirContext.CaseObjectName.Length;

            //
            //  If the slot for the next entry would be beyond the length of the
            //  user's buffer just exit (we know we've returned at least one entry
            //  already). This will happen when we align the pointer past the end.
            //

            if (NextEntry > IrpSp->Parameters.QueryDirectory.Length) {

                ReturnNextEntry = FALSE;
                try_leave( Status = STATUS_SUCCESS );
            }

            //
            //  Compute the number of bytes remaining in the buffer.  Round this
            //  down to a WCHAR boundary so we can copy full characters.
            //

            BytesRemainingInBuffer = IrpSp->Parameters.QueryDirectory.Length - NextEntry;
            ClearFlag( BytesRemainingInBuffer, 1 );

            //
            //  If this won't fit and we have returned a previous entry then just
            //  return STATUS_SUCCESS.
            //

            if ((BaseLength + FileNameBytes) > BytesRemainingInBuffer) {

                //
                //  If we already found an entry then just exit.
                //

                if (NextEntry != 0) {

                    ReturnNextEntry = FALSE;
                    try_leave( Status = STATUS_SUCCESS );
                }

                //
                //  Reduce the FileNameBytes to just fit in the buffer.
                //

                FileNameBytes = BytesRemainingInBuffer - BaseLength;

                //
                //  Use a status code of STATUS_BUFFER_OVERFLOW.  Also set
                //  ReturnSingleEntry so that we will exit the loop at the top.
                //

                Status = STATUS_BUFFER_OVERFLOW;
                ReturnSingleEntry = TRUE;
            }

            //
            //  Protect access to the user buffer with an exception handler.
            //  Since (at our request) IO doesn't buffer these requests, we have
            //  to guard against a user messing with the page protection and other
            //  such trickery.
            //

            try {
                
                //
                //  Zero and initialize the base part of the current entry.
                //

                RtlZeroMemory( Add2Ptr( UserBuffer, NextEntry, PVOID ),
                               BaseLength );
    
                //
                //  Now we have an entry to return to our caller.
                //  We'll case on the type of information requested and fill up
                //  the user buffer if everything fits.
                //
    
                switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {
    
                case FileBothDirectoryInformation:
                case FileFullDirectoryInformation:
                case FileDirectoryInformation:
    
                    DirInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_BOTH_DIR_INFORMATION );
    
                    //
                    //  These information types require we look up the file entry.
                    //
                    
                    UdfLookupFileEntryInEnumeration( IrpContext,
                                                     Fcb,
                                                     &CompoundDirContext );
                    //
                    //  Directly reference the file entry we just looked up.
                    //
                    
                    ThisFe = (PICBFILE) CompoundDirContext.IcbContext.Active.View;
                    
                    //
                    //  Now go gather all of the timestamps for this guy.
                    //
            
                    UdfUpdateTimestampsFromIcbContext ( IrpContext,
                                                        &CompoundDirContext.IcbContext,
                                                        &CompoundDirContext.Timestamps );
                    
                    DirInfo->CreationTime = CompoundDirContext.Timestamps.CreationTime;
    
                    DirInfo->LastWriteTime =
                    DirInfo->ChangeTime = CompoundDirContext.Timestamps.ModificationTime;
    
                    DirInfo->LastAccessTime = CompoundDirContext.Timestamps.AccessTime;
    
                    //
                    //  Set the attributes and sizes separately for directories and
                    //  files.
                    //
    
                    if (ThisFe->Icbtag.FileType == ICBTAG_FILE_T_DIRECTORY) {
    
                        DirInfo->EndOfFile.QuadPart = DirInfo->AllocationSize.QuadPart = 0;
    
                        SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
    
                    } else {
    
                        DirInfo->EndOfFile.QuadPart = ThisFe->InfoLength;
                        DirInfo->AllocationSize.QuadPart = LlBlockAlign( Fcb->Vcb, ThisFe->InfoLength );
                    }

                    //
                    //  All Cdrom files are readonly.  We also copy the existence
                    //  bit to the hidden attribute, assuming that synthesized FIDs
                    //  are never hidden.
                    //

                    SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY );
    
                    if (ThisFid && FlagOn( ThisFid->Flags, NSR_FID_F_HIDDEN )) {
    
                        SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
                    }
    
                    //
                    //  The file index for real file indices > 2^32 is zero.  When asked to
                    //  restart at an index of zero, we will know to use a stashed starting
                    //  point to beging to search, by name, for the correct restart point.
                    //
                    
                    if (CompoundDirContext.FileIndex.HighPart == 0) {
                        
                        DirInfo->FileIndex = CompoundDirContext.FileIndex.LowPart;
                    
                    } else {
    
                        DirInfo->FileIndex = 0;
                    }
    
                    DirInfo->FileNameLength = FileNameBytes;
    
                    break;
    
                case FileNamesInformation:
    
                    NamesInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_NAMES_INFORMATION );
    
                    if (CompoundDirContext.FileIndex.HighPart == 0) {
                        
                        NamesInfo->FileIndex = CompoundDirContext.FileIndex.LowPart;
                    
                    } else {
    
                        NamesInfo->FileIndex = 0;
                    }
    
                    NamesInfo->FileNameLength = FileNameBytes;
    
                    break;
                }

                //
                //  Now copy as much of the name as possible.
                //
    
                if (FileNameBytes != 0) {
    
                    //
                    //  This is a Unicode name, we can copy the bytes directly.
                    //
    
                    RtlCopyMemory( Add2Ptr( UserBuffer, NextEntry + BaseLength, PVOID ),
                                   CompoundDirContext.DirContext.ObjectName.Buffer,
                                   FileNameBytes );
                }

                //
                //  Fill in the short name if we got STATUS_SUCCESS.  The short name
                //  may already be in the file context, otherwise we will check
                //  whether the long name is 8.3.  Special case the self and parent
                //  directory names.
                //
    
                if ((Status == STATUS_SUCCESS) &&
                    (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation) &&
                    FlagOn( CompoundDirContext.DirContext.Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT )) {
    
                    //
                    //  If we already have the short name then copy into the user's buffer.
                    //
    
                    if (CompoundDirContext.DirContext.ShortObjectName.Length != 0) {
    
                        RtlCopyMemory( DirInfo->ShortName,
                                       CompoundDirContext.DirContext.ShortObjectName.Buffer,
                                       CompoundDirContext.DirContext.ShortObjectName.Length );
    
                        DirInfo->ShortNameLength = (CCHAR) CompoundDirContext.DirContext.ShortObjectName.Length;
    
                    //
                    //  If the short name length is currently zero then check if
                    //  the long name is not 8.3.  We can copy the short name in
                    //  unicode form directly into the caller's buffer.
                    //
    
                    } else {
    
                        if (!UdfIs8dot3Name( IrpContext,
                                             CompoundDirContext.DirContext.ObjectName )) {
    
                            UNICODE_STRING ShortName;
    
                            ShortName.Buffer = DirInfo->ShortName;
                            ShortName.MaximumLength = BYTE_COUNT_8_DOT_3;
                            
                            UdfGenerate8dot3Name( IrpContext,
                                                  &CompoundDirContext.DirContext.PureObjectName,
                                                  &ShortName );
    
                            DirInfo->ShortNameLength = (CCHAR) ShortName.Length;
                        }
                    }
                }

                //
                //  Update the information with the number of bytes stored in the
                //  buffer.  We quad-align the existing buffer to add any necessary
                //  pad bytes.
                //

                Information = NextEntry + BaseLength + FileNameBytes;

                //
                //  Go back to the previous entry and fill in the update to this entry.
                //

                *(Add2Ptr( UserBuffer, LastEntry, PULONG )) = NextEntry - LastEntry;

                //
                //  Set up our variables for the next dirent.
                //

                InitialQuery = FALSE;

                LastEntry = NextEntry;
                NextEntry = QuadAlign( Information );
            
            } except (!FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

                  //
                  //  We must have had a problem filling in the user's buffer, so stop
                  //  and fail this request.
                  //
                  
                  Information = 0;
                  try_leave( Status = GetExceptionCode());
            }
        }

    } finally {

        if (!AbnormalTermination() && !NT_ERROR( Status )) {
        
            //
            //  Update the Ccb to show the current state of the enumeration.
            //
    
            UdfLockFcb( IrpContext, Fcb );
    
            Ccb->CurrentFileIndex = CompoundDirContext.FileIndex.QuadPart;

            //
            //  Update our notion of a high 32bit file index.  We only do this once to avoid the hit
            //  of thrashing the Fcb mutex to do this for every entry.  If it is ever neccesary to use
            //  this information, the difference of a few dozen entries from the optimal pick-up point
            //  will be trivial.
            //
            
            if (CompoundDirContext.FileIndex.HighPart == 0 &&
                CompoundDirContext.FileIndex.LowPart > Ccb->HighestReturnableFileIndex) {

                    Ccb->HighestReturnableFileIndex = CompoundDirContext.FileIndex.LowPart;
            }
            
            ClearFlag( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );
    
            if (ReturnNextEntry) {
    
                SetFlag( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );
            }
    
            UdfUnlockFcb( IrpContext, Fcb );
        }

        //
        //  Cleanup our search context.
        //

        UdfCleanupCompoundDirContext( IrpContext, &CompoundDirContext );

        //
        //  Release the Fcb.
        //

        UdfReleaseFile( IrpContext, Fcb );
    }

    //
    //  Complete the request here.
    //

    Irp->IoStatus.Information = Information;

    UdfCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routines
//

NTSTATUS
UdfNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing of enqueuing the input Irp.  Although there
    will never be a notify signalled on a readonly disk we still support this call.

    We have already checked that this is not an OpenById handle.

Arguments:

    Irp - Supplies the Irp to process

    IrpSp - Io stack location for this request.

    Ccb - Handle to the directory being watched.

Return Value:

    NTSTATUS - STATUS_PENDING, any other error will raise.

--*/

{
    PAGED_CODE();

    //
    //  Always set the wait bit in the IrpContext so the initial wait can't fail.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Acquire the Vcb shared.
    //

    UdfAcquireVcbShared( IrpContext, IrpContext->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify the Vcb.
        //

        UdfVerifyVcb( IrpContext, IrpContext->Vcb );

        //
        //  Call the Fsrtl package to process the request.  We cast the
        //  unicode strings to ansi strings as the dir notify package
        //  only deals with memory matching.
        //

        FsRtlNotifyFullChangeDirectory( IrpContext->Vcb->NotifySync,
                                        &IrpContext->Vcb->DirNotifyList,
                                        Ccb,
                                        (PSTRING) &IrpSp->FileObject->FileName,
                                        BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE ),
                                        FALSE,
                                        IrpSp->Parameters.NotifyDirectory.CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );

    } finally {

        //
        //  Release the Vcb.
        //

        UdfReleaseVcb( IrpContext, IrpContext->Vcb );
    }

    //
    //  Cleanup the IrpContext.
    //

    UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return STATUS_PENDING;
}


//
//  Local support routine
//

VOID
UdfInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    OUT PBOOLEAN ReturnNextEntry,
    OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    )

/*++

Routine Description:

    This routine is called to initialize the enumeration variables and structures.
    We look at the state of a previous enumeration from the Ccb as well as any
    input values from the user.  On exit we will position the DirContext at
    a file in the directory and let the caller know whether this entry or the
    next entry should be returned.

Arguments:

    IrpSp - Irp stack location for this request.

    Fcb - Fcb for this directory.

    Ccb - Ccb for the directory handle.

    CompoundDirContext - Context to use for this enumeration.

    ReturnNextEntry - Address to store whether we should return the entry at
        the context position or the next entry.

    ReturnSingleEntry - Address to store whether we should only return
        a single entry.

    InitialQuery - Address to store whether this is the first enumeration
        query on this handle.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    PUNICODE_STRING FileName;
    UNICODE_STRING SearchExpression;

    PUNICODE_STRING RestartName = NULL;
    
    ULONG CcbFlags;

    LONGLONG FileIndex;
    ULONG HighFileIndex;
    BOOLEAN KnownIndex;

    BOOLEAN Found;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );
    ASSERT_CCB( Ccb );

    //
    //  If this is the initial query then build a search expression from the input
    //  file name.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

        FileName = (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName;

        CcbFlags = 0;

        //
        //  If the filename is not specified or is a single '*' then we will
        //  match all names.
        //

        if ((FileName == NULL) ||
            (FileName->Buffer == NULL) ||
            (FileName->Length == 0) ||
            ((FileName->Length == sizeof( WCHAR )) &&
             (FileName->Buffer[0] == L'*'))) {

            SetFlag( CcbFlags, CCB_FLAG_ENUM_MATCH_ALL );

            SearchExpression.Length =
            SearchExpression.MaximumLength = 0;
            SearchExpression.Buffer = NULL;

        //
        //  Otherwise build the name from the name in the stack location.
        //  This involves checking for wild card characters and upcasing the
        //  string if this is a case-insensitive search.
        //

        } else {

            //
            //  The name better have at least one character.
            //

            if (FileName->Length == 0) {

                UdfRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER );
            }

            //
            //  Check for wildcards in the separate components.
            //

            if (FsRtlDoesNameContainWildCards( FileName)) {

                SetFlag( CcbFlags, CCB_FLAG_ENUM_NAME_EXP_HAS_WILD );
            }
            
            //
            //  Now create the search expression to store in the Ccb.
            //

            SearchExpression.Buffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                                FileName->Length,
                                                                TAG_ENUM_EXPRESSION );

            SearchExpression.MaximumLength = FileName->Length;

            //
            //  Either copy the name directly or perform the upcase.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE )) {

                Status = RtlUpcaseUnicodeString( &SearchExpression,
                                                 FileName,
                                                 FALSE );

                //
                //  This should never fail.
                //

                ASSERT( Status == STATUS_SUCCESS );

            } else {

                RtlCopyMemory( SearchExpression.Buffer,
                               FileName->Buffer,
                               FileName->Length );
            }

            SearchExpression.Length = FileName->Length;
        }

        //
        //  But we do not want to return the constant "." and ".." entries for
        //  the root directory, for consistency with the rest of Microsoft's
        //  filesystems.
        //

        if (Fcb == Fcb->Vcb->RootIndexFcb) {

            SetFlag( CcbFlags, CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY );
        }

        //
        //  Now lock the Fcb in order to update the Ccb with the inital
        //  enumeration values.
        //

        UdfLockFcb( IrpContext, Fcb );

        //
        //  Check again that this is the initial search.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

            //
            //  Update the values in the Ccb.
            //

            Ccb->CurrentFileIndex = 0;
            Ccb->SearchExpression = SearchExpression;

            //
            //  Set the appropriate flags in the Ccb.
            //

            SetFlag( Ccb->Flags, CcbFlags | CCB_FLAG_ENUM_INITIALIZED );

        //
        //  Otherwise cleanup any buffer allocated here.
        //

        } else {

            if (!FlagOn( CcbFlags, CCB_FLAG_ENUM_MATCH_ALL )) {

                UdfFreePool( &SearchExpression.Buffer );
            }
        }

    //
    //  Otherwise lock the Fcb so we can read the current enumeration values.
    //

    } else {

        UdfLockFcb( IrpContext, Fcb );
    }

    //
    //  Capture the current state of the enumeration.
    //
    //  If the user specified an index then use his offset.  We always
    //  return the next entry in this case.  If  no name is specified,
    //  then we can't perform the restart.
    //

    if (FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED ) &&
        IrpSp->Parameters.QueryDirectory.FileName != NULL) {

        KnownIndex = FALSE;
        FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;
        RestartName = (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName;
        *ReturnNextEntry = TRUE;

        //
        //  We will use the highest file index reportable to the caller as a
        //  starting point as required if we cannot directly land at the
        //  specified location.
        //
        
        HighFileIndex = Ccb->HighestReturnableFileIndex;

    //
    //  If we are restarting the scan then go from the self entry.
    //

    } else if (FlagOn( IrpSp->Flags, SL_RESTART_SCAN )) {

        KnownIndex = TRUE;
        FileIndex = 0;
        *ReturnNextEntry = FALSE;

    //
    //  Otherwise use the values from the Ccb.
    //

    } else {

        KnownIndex = TRUE;
        FileIndex = Ccb->CurrentFileIndex;
        *ReturnNextEntry = BooleanFlagOn( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );
    }

    //
    //  Unlock the Fcb.
    //

    UdfUnlockFcb( IrpContext, Fcb );

    //
    //  We have the starting offset in the directory and whether to return
    //  that entry or the next.  If we are at the beginning of the directory
    //  and are returning that entry, then tell our caller this is the
    //  initial query.
    //

    *InitialQuery = FALSE;

    if ((FileIndex == 0) &&
        !(*ReturnNextEntry)) {

        *InitialQuery = TRUE;
    }

    //
    //  Determine the offset in the stream to position the context and
    //  whether this offset is known to be a file offset.
    //
    //  If this offset is known to be safe then go ahead and position the
    //  context.  This handles the cases where the offset is the beginning
    //  of the stream, the offset is from a previous search or this is the
    //  initial query.
    //

    if (KnownIndex) {

        Found = UdfLookupInitialFileIndex( IrpContext, Fcb, CompoundDirContext, &FileIndex );

        ASSERT( Found );
        
    //
    //  Try to directly jump to the specified file index.  Otherwise we walk through
    //  the directory from the beginning (or the saved highest known offset if that is
    //  useful) until we reach the entry which contains this offset.
    //

    } else {
        
        //
        //  We need to handle the special case of a restart from a synthesized
        //  entry - this is the one time where the restart index can be zero
        //  without requiring us to search above the 2^32 byte mark.
        //
        
        if (UdfFullCompareNames( IrpContext,
                                 RestartName,
                                 &UdfUnicodeDirectoryNames[SELF_ENTRY] ) == EqualTo) {

            FileIndex = UDF_FILE_INDEX_VIRTUAL_SELF;

            Found = UdfLookupInitialFileIndex( IrpContext, Fcb, CompoundDirContext, &FileIndex );
    
            ASSERT( Found );
            
        //
        //  We are restarting from a physical entry.  If the restart index is zero, we were
        //  unable to inform the caller as to the "real" file index due to the dispartity
        //  between the ULONG FileIndex in the return structures and the LONGLONG offsets
        //  possible in directory streams.  In this case, we will go as high as we were able
        //  to inform the caller of and search linearly from that point forward.
        //
        //  It is also possible (realistic? unknown) that the restart index is somewhere in the
        //  middle of an entry and we won't find anything useable.  In this case we try to find
        //  the entry which contains this index, using it as the real restart point.
        //
        
        } else {

            //
            //  See if we need the high water mark.
            //
            
            if (FileIndex == 0) {

                //
                //  We know that this is good.
                //
                
                FileIndex = Max( Ccb->HighestReturnableFileIndex, UDF_FILE_INDEX_PHYSICAL );;
                KnownIndex = TRUE;
            
            }
            
            //
            //  The file index is now useful, falling into two cases
            //
            //      1) KnownIndex == FALSE - searching by index
            //      2) KnownIndex == TRUE  - searching by name
            //
            //  Go set up our inquiry.
            //

            Found = UdfLookupInitialFileIndex( IrpContext, Fcb, CompoundDirContext, &FileIndex );
            
            if (KnownIndex) {
                
                //
                //  Walk forward to discover an entry named per the caller's expectation.
                //
                
                do {
    
                    UdfUpdateDirNames( IrpContext,
                                       &CompoundDirContext->DirContext,
                                       BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));
                    
                    if (UdfFullCompareNames( IrpContext,
                                             &CompoundDirContext->DirContext.CaseObjectName,
                                             RestartName ) == EqualTo) {

                        break;
                    }

                    Found = UdfLookupNextFileIndex( IrpContext, Fcb, CompoundDirContext );
    
                } while (Found);
            
            } else if (!Found) {

                LONGLONG LastFileIndex;

                //
                //  Perform the search for the entry by index from the beginning of the physical directory.
                //

                LastFileIndex = UDF_FILE_INDEX_PHYSICAL;

                Found = UdfLookupInitialFileIndex( IrpContext, Fcb, CompoundDirContext, &LastFileIndex );

                ASSERT( Found );

                //
                //  Keep walking through the directory until we run out of
                //  entries or we find an entry which ends beyond the input
                //  index value (index search case) or corresponds to the
                //  name we are looking for (name search case).
                //
    
                do {
    
                    //
                    //  If we have passed the index value then exit.
                    //

                    if (CompoundDirContext->FileIndex.QuadPart > FileIndex) {

                        Found = FALSE;
                        break;
                    }

                    //
                    //  Remember the current position in case we need to go back.
                    //

                    LastFileIndex = CompoundDirContext->FileIndex.QuadPart;

                    //
                    //  Exit if the next entry is beyond the desired index value.
                    //

                    if (LastFileIndex + ISONsrFidSize( CompoundDirContext->DirContext.Fid ) > FileIndex) {

                        break;
                    }
    
                    Found = UdfLookupNextFileIndex( IrpContext, Fcb, CompoundDirContext );
    
                } while (Found);
    
                //
                //  If we didn't find the entry then go back to the last known entry.
                //
    
                if (!Found) {
    
                    UdfCleanupDirContext( IrpContext, &CompoundDirContext->DirContext );
                    UdfInitializeDirContext( IrpContext, &CompoundDirContext->DirContext );
    
                    Found = UdfLookupInitialFileIndex( IrpContext, Fcb, CompoundDirContext, &LastFileIndex );

                    ASSERT( Found );
                }
            }
        }
    }

    //
    //  Only update the dirent name if we will need it for some reason.
    //  Don't update this name if we are returning the next entry, and
    //  don't update it if it was already done.
    //

    if (!(*ReturnNextEntry) &&
        CompoundDirContext->DirContext.PureObjectName.Buffer == NULL) {

        //
        //  Update the names in the dirent.
        //

        UdfUpdateDirNames( IrpContext,
                           &CompoundDirContext->DirContext,
                           BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));
    }

    //
    //  Look at the flag in the IrpSp indicating whether to return just
    //  one entry.
    //

    *ReturnSingleEntry = FALSE;

    if (FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY )) {

        *ReturnSingleEntry = TRUE;
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
UdfEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    IN BOOLEAN ReturnNextEntry
    )

/*++

Routine Description:

    This routine is the worker routine for index enumeration.  We are positioned
    at some dirent in the directory and will either return the first match
    at that point or look to the next entry.  The Ccb contains details about
    the type of matching to do.

Arguments:

    Ccb - Ccb for this directory handle.

    CompoundDirContext - context already positioned at some entry in the directory.

    ReturnNextEntry - Indicates if we are returning this entry or should start
        with the next entry.

Return Value:

    BOOLEAN - TRUE if next entry is found, FALSE otherwise.

--*/

{
    BOOLEAN Found = FALSE;
    PNSR_FID Fid;
    PDIR_ENUM_CONTEXT DirContext;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( Ccb );

    //
    //  Directly reference the directory enumeration context for convenience.
    //

    DirContext = &CompoundDirContext->DirContext;

    //
    //  Loop until we find a match or exaust the directory.
    //

    while (TRUE) {

        //
        //  Move to the next entry unless we want to consider the current
        //  entry.
        //

        if (ReturnNextEntry) {

            if (!UdfLookupNextFileIndex( IrpContext, Ccb->Fcb, CompoundDirContext )) {

                break;
            }

            UdfUpdateDirNames( IrpContext,
                               DirContext,
                               BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));
        } else {

            ReturnNextEntry = TRUE;
        }

        //
        //  Don't bother if we have a constant entry and are ignoring them.
        //
        
        if (!FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT ) &&
            FlagOn( Ccb->Flags, CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY )) {

            continue;
        }

        //
        //  Directly reference the Fid for convenience.
        //

        Fid = DirContext->Fid;

        //
        //  Look at the current entry if it is pointing at real space on the disk.  If the Fid is NULL, this
        //  means it is to be synthesized (and thus interesting to look at).
        //

        if (Fid == NULL || !FlagOn( Fid->Flags, NSR_FID_F_DELETED )) {

            //
            //  If we match all names then return to our caller.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_ENUM_MATCH_ALL )) {

                DirContext->ShortObjectName.Length = 0;
                Found = TRUE;

                break;
            }

            //
            //  Check if the long name matches the search expression.
            //

            if (UdfIsNameInExpression( IrpContext,
                                       &DirContext->CaseObjectName,
                                       &Ccb->SearchExpression,
                                       BooleanFlagOn( Ccb->Flags, CCB_FLAG_ENUM_NAME_EXP_HAS_WILD ))) {

                //
                //  Let our caller know we found an entry.
                //

                DirContext->ShortObjectName.Length = 0;
                Found = TRUE;

                break;
            }

            //
            //  The long name didn't match so we need to check for a
            //  possible short name match.  There is no match if the
            //  long name is one of the constant entries or already
            //  is 8dot3.
            //

            if (!(!FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT ) ||
                  UdfIs8dot3Name( IrpContext,
                                  DirContext->CaseObjectName ))) {

                //
                //  Allocate the shortname if it isn't already done.
                //
                
                if (DirContext->ShortObjectName.Buffer == NULL) {

                    DirContext->ShortObjectName.Buffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                                                   BYTE_COUNT_8_DOT_3,
                                                                                   TAG_SHORT_FILE_NAME );
                    DirContext->ShortObjectName.MaximumLength = BYTE_COUNT_8_DOT_3;
                }

                UdfGenerate8dot3Name( IrpContext,
                                      &DirContext->PureObjectName,
                                      &DirContext->ShortObjectName );

                //
                //  Check if this name matches.
                //

                if (UdfIsNameInExpression( IrpContext,
                                           &DirContext->ShortObjectName,
                                           &Ccb->SearchExpression,
                                           BooleanFlagOn( Ccb->Flags, CCB_FLAG_ENUM_NAME_EXP_HAS_WILD ))) {
                    
                    //
                    //  Let our caller know we found an entry.
                    //

                    Found = TRUE;
    
                    break;
                }
            }
        }
    }

    return Found;
}


//
//  Local support routine
//

VOID
UdfLookupFileEntryInEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    )

/*++

Routine Description:

    This routine retrieves the file entry associated with the current location in
    the enumeration of a compound directory context.
  
Arguments:

    Fcb - the directory being enumerated.
    
    CompoundDirContext - a corresponding context for the enumeration.
    
Return Value:

    None.  Status may be raised on discovery of corruption.

--*/

{
    PNSR_FID Fid;
    PICBFILE Fe;

    Fid = CompoundDirContext->DirContext.Fid;

    //
    //  Figure out where the ICB we want is.
    //
    
    if (UdfIsFileIndexVirtual( CompoundDirContext->FileIndex.QuadPart )) {

        //
        //  Synthesize!  We only have to synthesize the self entry.  The name is already done,
        //  so the remaining work is trivial.
        //

        ASSERT( Fid == NULL );

        //
        //  Lift the FE corresponding to this directory
        //

        UdfCleanupIcbContext( IrpContext, &CompoundDirContext->IcbContext );
        
        UdfInitializeIcbContextFromFcb( IrpContext,
                                        &CompoundDirContext->IcbContext,
                                        Fcb );

    } else {

        //
        //  Lift the FE corresponding to this FID.
        //

        ASSERT( Fid != NULL );

        UdfCleanupIcbContext( IrpContext, &CompoundDirContext->IcbContext );

        UdfInitializeIcbContext( IrpContext,
                                 &CompoundDirContext->IcbContext,
                                 Fcb->Vcb,
                                 DESTAG_ID_NSR_FILE,
                                 Fid->Icb.Start.Partition,
                                 Fid->Icb.Start.Lbn,
                                 Fid->Icb.Length.Length );
    }

    //
    //  Retrieve the ICB for inspection.
    //
    
    UdfLookupActiveIcb( IrpContext, &CompoundDirContext->IcbContext );

    Fe = (PICBFILE) CompoundDirContext->IcbContext.Active.View;

    //
    //  Perform some basic verification that the FE is of the proper type and that
    //  FID and FE agree as to the type of the object.  We explicitly check that
    //  a legal filesystem-level FE type is discovered, even though we don't support
    //  them in other paths.
    //

    if (Fe->Destag.Ident != DESTAG_ID_NSR_FILE ||

        (((Fid && FlagOn( Fid->Flags, NSR_FID_F_DIRECTORY )) ||
          Fid == NULL) &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_DIRECTORY) ||

        (Fe->Icbtag.FileType != ICBTAG_FILE_T_FILE &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_DIRECTORY &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_BLOCK_DEV &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_CHAR_DEV &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_FIFO &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_C_ISSOCK &&
         Fe->Icbtag.FileType != ICBTAG_FILE_T_PATHLINK)) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }
}


//
//  Local support routine
//

BOOLEAN
UdfLookupInitialFileIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext,
    IN PLONGLONG InitialIndex
    )

/*++

Routine Description:

    This routine begins the enumeration of a directory by setting the context
    at the first avaliable virtual directory entry.
  
Arguments:

    Fcb - the directory being enumerated.
    
    CompoundDirContext - a corresponding context for the enumeration.
    
    InitialIndex - an optional starting file index to base the enumeration.
    
Return Value:

   TRUE will be returned if a valid entry is found at this offset, FALSE otherwise.

--*/

{
    LONGLONG DirOffset;

    if (UdfIsFileIndexVirtual( *InitialIndex )) {

        //
        //  We only synthesize a single virtual directory entry.  Position the context
        //  at the virtual self entry.
        //
        
        CompoundDirContext->FileIndex.QuadPart = UDF_FILE_INDEX_VIRTUAL_SELF;
        
        return TRUE;
    }

    CompoundDirContext->FileIndex.QuadPart = *InitialIndex;

    //
    //  Find the base offset in the directory and look it up.
    //
    
    DirOffset = UdfFileIndexToPhysicalOffset( *InitialIndex );
        
    return UdfLookupInitialDirEntry( IrpContext,
                                     Fcb,
                                     &CompoundDirContext->DirContext,
                                     &DirOffset );
}


//
//  Local support routine
//

BOOLEAN
UdfLookupNextFileIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCOMPOUND_DIR_ENUM_CONTEXT CompoundDirContext
    )

/*++

Routine Description:

    This routine advances the enumeration of a virtual directory by one entry.

Arguments:

    Fcb - the directory being enumerated.
    
    CompoundDirContext - a corresponding context for the enumeration.

Return Value:

    BOOLEAN True if another Fid is avaliable, False if we are at the end.

--*/

{
    ULONG Advance;
    BOOLEAN Result;

    //
    //  Advance from the synthesized to the physical directory.
    //
    
    if (UdfIsFileIndexVirtual( CompoundDirContext->FileIndex.QuadPart )) {

        Result = UdfLookupInitialDirEntry( IrpContext,
                                           Fcb,
                                           &CompoundDirContext->DirContext,
                                           NULL );
        
        if (Result) {
            
            CompoundDirContext->FileIndex.QuadPart = UDF_FILE_INDEX_PHYSICAL;
        }

        return Result;
    }
    
    Advance = ISONsrFidSize( CompoundDirContext->DirContext.Fid );
    
    //
    //  Advance to the next entry in this directory.
    //
    
    Result = UdfLookupNextDirEntry( IrpContext, Fcb, &CompoundDirContext->DirContext );

    if (Result) {

        CompoundDirContext->FileIndex.QuadPart += Advance;
    }
    
    return Result;
}

