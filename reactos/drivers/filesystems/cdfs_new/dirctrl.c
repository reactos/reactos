/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Cdfs called
    by the Fsd/Fsp dispatch drivers.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_DIRCTRL)

//
//  Local support routines
//

NTSTATUS
CdQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN PCCB Ccb
    );

NTSTATUS
CdNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    );

VOID
CdInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    OUT PBOOLEAN ReturnNextEntry,
    OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    );

BOOLEAN
CdEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    IN BOOLEAN ReturnNextEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonDirControl)
#pragma alloc_text(PAGE, CdEnumerateIndex)
#pragma alloc_text(PAGE, CdInitializeEnumeration)
#pragma alloc_text(PAGE, CdNotifyChangeDirectory)
#pragma alloc_text(PAGE, CdQueryDirectory)
#endif


NTSTATUS
CdCommonDirControl (
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

    if (CdDecodeFileObject( IrpContext,
                            IrpSp->FileObject,
                            &Fcb,
                            &Ccb ) != UserDirectoryOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = CdQueryDirectory( IrpContext, Irp, IrpSp, Fcb, Ccb );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = CdNotifyChangeDirectory( IrpContext, Irp, IrpSp, Ccb );
        break;

    default:

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return Status;
}


//
//  Local support routines
//

NTSTATUS
CdQueryDirectory (
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
    ULONG SeparatorBytes;
    ULONG VersionStringBytes;

    FILE_ENUM_CONTEXT FileContext;
    PDIRENT ThisDirent;
    BOOLEAN InitialQuery;
    BOOLEAN ReturnNextEntry;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN Found;
    BOOLEAN DoCcbUpdate = FALSE;

    PCHAR UserBuffer;
    ULONG BytesRemainingInBuffer;

    ULONG BaseLength;

    PFILE_BOTH_DIR_INFORMATION DirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;
    PFILE_ID_FULL_DIR_INFORMATION IdFullDirInfo;
    PFILE_ID_BOTH_DIR_INFORMATION IdBothDirInfo;

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

    case FileIdFullDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION,
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

    case FileIdBothDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION,
                                   FileName[0] );
        break;

    default:

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_INFO_CLASS );
        return STATUS_INVALID_INFO_CLASS;
    }

    //
    //  Get the user buffer.
    //

    CdMapUserBuffer( IrpContext, &UserBuffer);

    //
    //  Initialize our search context.
    //

    CdInitializeFileContext( IrpContext, &FileContext );

    //
    //  Acquire the directory.
    //

    CdAcquireFileShared( IrpContext, Fcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify the Fcb is still good.
        //

        CdVerifyFcbOperation( IrpContext, Fcb );

        //
        //  Start by getting the initial state for the enumeration.  This will set up the Ccb with
        //  the initial search parameters and let us know the starting offset in the directory
        //  to search.
        //

        CdInitializeEnumeration( IrpContext,
                                 IrpSp,
                                 Fcb,
                                 Ccb,
                                 &FileContext,
                                 &ReturnNextEntry,
                                 &ReturnSingleEntry,
                                 &InitialQuery );

        //
        //  The current dirent is stored in the InitialDirent field.  We capture
        //  this here so that we have a valid restart point even if we don't
        //  find a single entry.
        //

        ThisDirent = &FileContext.InitialDirent->Dirent;

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

                DoCcbUpdate = TRUE;
                try_leave( Status );
            }

            //
            //  We try to locate the next matching dirent.  Our search if based on a starting
            //  dirent offset, whether we should return the current or next entry, whether
            //  we should be doing a short name search and finally whether we should be
            //  checking for a version match.
            //

            Found = CdEnumerateIndex( IrpContext, Ccb, &FileContext, ReturnNextEntry );

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

                DoCcbUpdate = TRUE;
                try_leave( Status );
            }

            //
            //  Remember the dirent for the file we just found.
            //

            ThisDirent = &FileContext.InitialDirent->Dirent;

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
            //  Let's compute the number of bytes we need to transfer the current entry.
            //

            SeparatorBytes =
            VersionStringBytes = 0;

            //
            //  We can look directly at the dirent that we found.
            //

            FileNameBytes = ThisDirent->CdFileName.FileName.Length;

            //
            //  Compute the number of bytes for the version string if
            //  we will return this. Allow directories with illegal ";".
            //

            if (((Ccb->SearchExpression.VersionString.Length != 0) ||
                 (FlagOn(ThisDirent->DirentFlags, CD_ATTRIBUTE_DIRECTORY))) &&
                (ThisDirent->CdFileName.VersionString.Length != 0)) {

                SeparatorBytes = 2;

                VersionStringBytes = ThisDirent->CdFileName.VersionString.Length;
            }

            //
            //  If the slot for the next entry would be beyond the length of the
            //  user's buffer just exit (we know we've returned at least one entry
            //  already). This will happen when we align the pointer past the end.
            //

            if (NextEntry > IrpSp->Parameters.QueryDirectory.Length) {

                ReturnNextEntry = FALSE;
                DoCcbUpdate = TRUE;
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

            if ((BaseLength + FileNameBytes + SeparatorBytes + VersionStringBytes) > BytesRemainingInBuffer) {

                //
                //  If we already found an entry then just exit.
                //

                if (NextEntry != 0) {

                    ReturnNextEntry = FALSE;
                    DoCcbUpdate = TRUE;
                    try_leave( Status = STATUS_SUCCESS );
                }

                //
                //  Don't even try to return the version string if it doesn't all fit.
                //  Reduce the FileNameBytes to just fit in the buffer.
                //

                if ((BaseLength + FileNameBytes) > BytesRemainingInBuffer) {

                    FileNameBytes = BytesRemainingInBuffer - BaseLength;
                }

                //
                //  Don't return any version string bytes.
                //

                VersionStringBytes =
                SeparatorBytes = 0;

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
                case FileIdBothDirectoryInformation:
                case FileIdFullDirectoryInformation:
                case FileDirectoryInformation:
    
                    DirInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_BOTH_DIR_INFORMATION );
    
                    //
                    //  Use the create time for all the time stamps.
                    //
    
                    CdConvertCdTimeToNtTime( IrpContext,
                                             FileContext.InitialDirent->Dirent.CdTime,
                                             &DirInfo->CreationTime );
    
                    DirInfo->LastWriteTime = DirInfo->ChangeTime = DirInfo->CreationTime;
    
                    //
                    //  Set the attributes and sizes separately for directories and
                    //  files.
                    //
    
                    if (FlagOn( ThisDirent->DirentFlags, CD_ATTRIBUTE_DIRECTORY )) {
    
                        DirInfo->EndOfFile.QuadPart = DirInfo->AllocationSize.QuadPart = 0;
    
                        SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
    
                    } else {
    
                        DirInfo->EndOfFile.QuadPart = FileContext.FileSize;
                        DirInfo->AllocationSize.QuadPart = LlSectorAlign( FileContext.FileSize );
                    }
    
                    //
                    //  All Cdrom files are readonly.  We also copy the existence
                    //  bit to the hidden attribute.
                    //
    
                    SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_READONLY );
    
                    if (FlagOn( ThisDirent->DirentFlags,
                                CD_ATTRIBUTE_HIDDEN )) {
    
                        SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
                    }
    
                    DirInfo->FileIndex = ThisDirent->DirentOffset;
    
                    DirInfo->FileNameLength = FileNameBytes + SeparatorBytes + VersionStringBytes;
    
                    break;
    
                case FileNamesInformation:
    
                    NamesInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_NAMES_INFORMATION );
    
                    NamesInfo->FileIndex = ThisDirent->DirentOffset;
    
                    NamesInfo->FileNameLength = FileNameBytes + SeparatorBytes + VersionStringBytes;
    
                    break;
                }

                //
                //  Fill in the FileId
                //

                switch (IrpSp->Parameters.QueryDirectory.FileInformationClass) {

                case FileIdBothDirectoryInformation:

                    IdBothDirInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_ID_BOTH_DIR_INFORMATION );
                    CdSetFidFromParentAndDirent( IdBothDirInfo->FileId, Fcb, ThisDirent );
                    break;

                case FileIdFullDirectoryInformation:

                    IdFullDirInfo = Add2Ptr( UserBuffer, NextEntry, PFILE_ID_FULL_DIR_INFORMATION );
                    CdSetFidFromParentAndDirent( IdFullDirInfo->FileId, Fcb, ThisDirent );
                    break;

                default:
                    break;
                }
    
                //
                //  Now copy as much of the name as possible.  We also may have a version
                //  string to copy.
                //
    
                if (FileNameBytes != 0) {
    
                    //
                    //  This is a Unicode name, we can copy the bytes directly.
                    //
    
                    RtlCopyMemory( Add2Ptr( UserBuffer, NextEntry + BaseLength, PVOID ),
                                   ThisDirent->CdFileName.FileName.Buffer,
                                   FileNameBytes );
    
                    if (SeparatorBytes != 0) {
    
                        *(Add2Ptr( UserBuffer,
                                   NextEntry + BaseLength + FileNameBytes,
                                   PWCHAR )) = L';';
    
                        if (VersionStringBytes != 0) {
    
                            RtlCopyMemory( Add2Ptr( UserBuffer,
                                                    NextEntry + BaseLength + FileNameBytes + sizeof( WCHAR ),
                                                    PVOID ),
                                           ThisDirent->CdFileName.VersionString.Buffer,
                                           VersionStringBytes );
                        }
                    }
                }

                //
                //  Fill in the short name if we got STATUS_SUCCESS.  The short name
                //  may already be in the file context.  Otherwise we will check
                //  whether the long name is 8.3.  Special case the self and parent
                //  directory names.
                //

                if ((Status == STATUS_SUCCESS) &&
                    (IrpSp->Parameters.QueryDirectory.FileInformationClass == FileBothDirectoryInformation ||
                     IrpSp->Parameters.QueryDirectory.FileInformationClass == FileIdBothDirectoryInformation) &&
                    (Ccb->SearchExpression.VersionString.Length == 0) &&
                    !FlagOn( ThisDirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY )) {
    
                    //
                    //  If we already have the short name then copy into the user's buffer.
                    //
    
                    if (FileContext.ShortName.FileName.Length != 0) {
    
                        RtlCopyMemory( DirInfo->ShortName,
                                       FileContext.ShortName.FileName.Buffer,
                                       FileContext.ShortName.FileName.Length );
    
                        DirInfo->ShortNameLength = (CCHAR) FileContext.ShortName.FileName.Length;
    
                    //
                    //  If the short name length is currently zero then check if
                    //  the long name is not 8.3.  We can copy the short name in
                    //  unicode form directly into the caller's buffer.
                    //
    
                    } else {
    
                        if (!CdIs8dot3Name( IrpContext,
                                            ThisDirent->CdFileName.FileName )) {
    
                            CdGenerate8dot3Name( IrpContext,
                                                 &ThisDirent->CdCaseFileName.FileName,
                                                 ThisDirent->DirentOffset,
                                                 DirInfo->ShortName,
                                                 &FileContext.ShortName.FileName.Length );
    
                            DirInfo->ShortNameLength = (CCHAR) FileContext.ShortName.FileName.Length;
                        }
                    }
    
                }

                //
                //  Sum the total number of bytes for the information field.
                //

                FileNameBytes += SeparatorBytes + VersionStringBytes;

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
            
            } except (EXCEPTION_EXECUTE_HANDLER) {

                  //
                  //  We had a problem filling in the user's buffer, so stop and
                  //  fail this request.  This is the only reason any exception
                  //  would have occured at this level.
                  //
                  
                  Information = 0;
                  try_leave( Status = GetExceptionCode());
            }
        }
        
        DoCcbUpdate = TRUE;

    } finally {

        //
        //  Cleanup our search context - *before* aquiring the FCB mutex exclusive,
        //  else can block on threads in cdcreateinternalstream/purge which 
        //  hold the FCB but are waiting for all maps in this stream to be released.
        //

        CdCleanupFileContext( IrpContext, &FileContext );

        //
        //  Now we can safely aqure the FCB mutex if we need to.
        //

        if (DoCcbUpdate && !NT_ERROR( Status )) {
        
            //
            //  Update the Ccb to show the current state of the enumeration.
            //

            CdLockFcb( IrpContext, Fcb );
            
            Ccb->CurrentDirentOffset = ThisDirent->DirentOffset;

            ClearFlag( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );

            if (ReturnNextEntry) {

                SetFlag( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );
            }

            CdUnlockFcb( IrpContext, Fcb );
        }

        //
        //  Release the Fcb.
        //

        CdReleaseFile( IrpContext, Fcb );
    }

    //
    //  Complete the request here.
    //

    Irp->IoStatus.Information = Information;

    CdCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routines
//

NTSTATUS
CdNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing of enqueuing the input Irp.  Although there
    will never be a notify signalled on a CDROM disk we still support this call.

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

    CdAcquireVcbShared( IrpContext, IrpContext->Vcb, FALSE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify the Vcb.
        //

        CdVerifyVcb( IrpContext, IrpContext->Vcb );

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

        CdReleaseVcb( IrpContext, IrpContext->Vcb );
    }

    //
    //  Cleanup the IrpContext.
    //

    CdCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    return STATUS_PENDING;
}


//
//  Local support routine
//

VOID
CdInitializeEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN OUT PCCB Ccb,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    OUT PBOOLEAN ReturnNextEntry,
    OUT PBOOLEAN ReturnSingleEntry,
    OUT PBOOLEAN InitialQuery
    )

/*++

Routine Description:

    This routine is called to initialize the enumeration variables and structures.
    We look at the state of a previous enumeration from the Ccb as well as any
    input values from the user.  On exit we will position the FileContext at
    a file in the directory and let the caller know whether this entry or the
    next entry should be returned.

Arguments:

    IrpSp - Irp stack location for this request.

    Fcb - Fcb for this directory.

    Ccb - Ccb for the directory handle.

    FileContext - FileContext to use for this enumeration.

    ReturnNextEntry - Address to store whether we should return the entry at
        the FileContext position or the next entry.

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
    CD_NAME WildCardName;
    CD_NAME SearchExpression;

    ULONG CcbFlags;

    ULONG DirentOffset;
    ULONG LastDirentOffset;
    BOOLEAN KnownOffset;

    BOOLEAN Found;

    PAGED_CODE();

    //
    //  If this is the initial query then build a search expression from the input
    //  file name.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

        FileName = IrpSp->Parameters.QueryDirectory.FileName;

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
            RtlZeroMemory( &SearchExpression, sizeof( SearchExpression ));

        //
        //  Otherwise build the CdName from the name in the stack location.
        //  This involves building both the name and version portions and
        //  checking for wild card characters.  We also upcase the string if
        //  this is a case-insensitive search.
        //

        } else {

            //
            //  Create a CdName to check for wild cards.
            //

            WildCardName.FileName = *FileName;

            CdConvertNameToCdName( IrpContext, &WildCardName );

            //
            //  The name better have at least one character.
            //

            if (WildCardName.FileName.Length == 0) {

                CdRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER );
            }

            //
            //  Check for wildcards in the separate components.
            //

            if (FsRtlDoesNameContainWildCards( &WildCardName.FileName)) {

                SetFlag( CcbFlags, CCB_FLAG_ENUM_NAME_EXP_HAS_WILD );
            }

            if ((WildCardName.VersionString.Length != 0) &&
                (FsRtlDoesNameContainWildCards( &WildCardName.VersionString ))) {

                SetFlag( CcbFlags, CCB_FLAG_ENUM_VERSION_EXP_HAS_WILD );

                //
                //  Check if this is a wild card only and match all version
                //  strings.
                //

                if ((WildCardName.VersionString.Length == sizeof( WCHAR )) &&
                    (WildCardName.VersionString.Buffer[0] == L'*')) {

                    SetFlag( CcbFlags, CCB_FLAG_ENUM_VERSION_MATCH_ALL );
                }
            }

            //
            //  Now create the search expression to store in the Ccb.
            //

            SearchExpression.FileName.Buffer = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                                         FileName->Length,
                                                                         TAG_ENUM_EXPRESSION );

            SearchExpression.FileName.MaximumLength = FileName->Length;

            //
            //  Either copy the name directly or perform the upcase.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE )) {

                Status = RtlUpcaseUnicodeString( (PUNICODE_STRING) &SearchExpression.FileName,
                                                 FileName,
                                                 FALSE );

                //
                //  This should never fail.
                //

                ASSERT( Status == STATUS_SUCCESS );

            } else {

                RtlCopyMemory( SearchExpression.FileName.Buffer,
                               FileName->Buffer,
                               FileName->Length );
            }

            //
            //  Now split into the separate name and version components.
            //

            SearchExpression.FileName.Length = WildCardName.FileName.Length;
            SearchExpression.VersionString.Length = WildCardName.VersionString.Length;
            SearchExpression.VersionString.MaximumLength = WildCardName.VersionString.MaximumLength;

            SearchExpression.VersionString.Buffer = Add2Ptr( SearchExpression.FileName.Buffer,
                                                             SearchExpression.FileName.Length + sizeof( WCHAR ),
                                                             PWCHAR );
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

        CdLockFcb( IrpContext, Fcb );

        //
        //  Check again that this is the initial search.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ENUM_INITIALIZED )) {

            //
            //  Update the values in the Ccb.
            //

            Ccb->CurrentDirentOffset = Fcb->StreamOffset;
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

                CdFreePool( &SearchExpression.FileName.Buffer );
            }
        }

    //
    //  Otherwise lock the Fcb so we can read the current enumeration values.
    //

    } else {

        CdLockFcb( IrpContext, Fcb );
    }

    //
    //  Capture the current state of the enumeration.
    //
    //  If the user specified an index then use his offset.  We always
    //  return the next entry in this case.
    //

    if (FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )) {

        KnownOffset = FALSE;
        DirentOffset = IrpSp->Parameters.QueryDirectory.FileIndex;
        *ReturnNextEntry = TRUE;

    //
    //  If we are restarting the scan then go from the self entry.
    //

    } else if (FlagOn( IrpSp->Flags, SL_RESTART_SCAN )) {

        KnownOffset = TRUE;
        DirentOffset = Fcb->StreamOffset;
        *ReturnNextEntry = FALSE;

    //
    //  Otherwise use the values from the Ccb.
    //

    } else {

        KnownOffset = TRUE;
        DirentOffset = Ccb->CurrentDirentOffset;
        *ReturnNextEntry = BooleanFlagOn( Ccb->Flags, CCB_FLAG_ENUM_RETURN_NEXT );
    }

    //
    //  Unlock the Fcb.
    //

    CdUnlockFcb( IrpContext, Fcb );

    //
    //  We have the starting offset in the directory and whether to return
    //  that entry or the next.  If we are at the beginning of the directory
    //  and are returning that entry, then tell our caller this is the
    //  initial query.
    //

    *InitialQuery = FALSE;

    if ((DirentOffset == Fcb->StreamOffset) &&
        !(*ReturnNextEntry)) {

        *InitialQuery = TRUE;
    }

    //
    //  If there is no file object then create it now.
    //

    if (Fcb->FileObject == NULL) {

        CdCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
    }

    //
    //  Determine the offset in the stream to position the FileContext and
    //  whether this offset is known to be a file offset.
    //
    //  If this offset is known to be safe then go ahead and position the
    //  file context.  This handles the cases where the offset is the beginning
    //  of the stream, the offset is from a previous search or this is the
    //  initial query.
    //

    if (KnownOffset) {

        CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, DirentOffset );

    //
    //  Otherwise we walk through the directory from the beginning until
    //  we reach the entry which contains this offset.
    //

    } else {

        LastDirentOffset = Fcb->StreamOffset;
        Found = TRUE;

        CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, LastDirentOffset );

        //
        //  If the requested offset is prior to the beginning offset in the stream
        //  then don't return the next entry.
        //

        if (DirentOffset < LastDirentOffset) {

            *ReturnNextEntry = FALSE;

        //
        //  Else look for the last entry which ends past the desired index.
        //

        } else {

            //
            //  Keep walking through the directory until we run out of
            //  entries or we find an entry which ends beyond the input
            //  index value.
            //

            do {

                //
                //  If we have passed the index value then exit.
                //

                if (FileContext->InitialDirent->Dirent.DirentOffset > DirentOffset) {

                    Found = FALSE;
                    break;
                }

                //
                //  Remember the current position in case we need to go back.
                //

                LastDirentOffset = FileContext->InitialDirent->Dirent.DirentOffset;

                //
                //  Exit if the next entry is beyond the desired index value.
                //

                if (LastDirentOffset + FileContext->InitialDirent->Dirent.DirentLength > DirentOffset) {

                    break;
                }

                Found = CdLookupNextInitialFileDirent( IrpContext, Fcb, FileContext );

            } while (Found);

            //
            //  If we didn't find the entry then go back to the last known entry.
            //  This can happen if the index lies in the unused range at the
            //  end of a sector.
            //

            if (!Found) {

                CdCleanupFileContext( IrpContext, FileContext );
                CdInitializeFileContext( IrpContext, FileContext );

                CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, LastDirentOffset );
            }
        }
    }

    //
    //  Only update the dirent name if we will need it for some reason.
    //  Don't update this name if we are returning the next entry and
    //  the search string has a version component.
    //

    FileContext->ShortName.FileName.Length = 0;

    if (!(*ReturnNextEntry) ||
        (Ccb->SearchExpression.VersionString.Length == 0)) {

        //
        //  Update the name in the dirent into filename and version components.
        //

        CdUpdateDirentName( IrpContext,
                            &FileContext->InitialDirent->Dirent,
                            FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));
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
CdEnumerateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    IN BOOLEAN ReturnNextEntry
    )

/*++

Routine Description:

    This routine is the worker routine for index enumeration.  We are positioned
    at some dirent in the directory and will either return the first match
    at that point or look to the next entry.  The Ccb contains details about
    the type of matching to do.  If the user didn't specify a version in
    his search string then we only return the first version of a sequence
    of files with versions.  We also don't return any associated files.

Arguments:

    Ccb - Ccb for this directory handle.

    FileContext - File context already positioned at some entry in the directory.

    ReturnNextEntry - Indicates if we are returning this entry or should start
        with the next entry.

Return Value:

    BOOLEAN - TRUE if next entry is found, FALSE otherwise.

--*/

{
    PDIRENT PreviousDirent = NULL;
    PDIRENT ThisDirent = &FileContext->InitialDirent->Dirent;

    BOOLEAN Found = FALSE;

    PAGED_CODE();

    //
    //  Loop until we find a match or exaust the directory.
    //

    while (TRUE) {

        //
        //  Move to the next entry unless we want to consider the current
        //  entry.
        //

        if (ReturnNextEntry) {

            if (!CdLookupNextInitialFileDirent( IrpContext, Ccb->Fcb, FileContext )) {

                break;
            }

            PreviousDirent = ThisDirent;
            ThisDirent = &FileContext->InitialDirent->Dirent;

            CdUpdateDirentName( IrpContext, ThisDirent, FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));
        
        } else {

            ReturnNextEntry = TRUE;
        }

        //
        //  Don't bother if we have a constant entry and are ignoring them.
        //
        
        if (FlagOn( ThisDirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY ) &&
            FlagOn( Ccb->Flags, CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY )) {

            continue;
        }

        //
        //  Look at the current entry if it is not an associated file
        //  and the name doesn't match the previous file if the version
        //  name is not part of the search.
        //

        if (!FlagOn( ThisDirent->DirentFlags, CD_ATTRIBUTE_ASSOC )) {

            //
            //  Check if this entry matches the previous entry except
            //  for version number and whether we should return the
            //  entry in that case.  Go directly to the name comparison
            //  if:
            //
            //      There is no previous entry.
            //      The search expression has a version component.
            //      The name length doesn't match the length of the previous entry.
            //      The base name strings don't match.
            //

            if ((PreviousDirent == NULL) ||
                (Ccb->SearchExpression.VersionString.Length != 0) ||
                (PreviousDirent->CdCaseFileName.FileName.Length != ThisDirent->CdCaseFileName.FileName.Length) ||
                FlagOn( PreviousDirent->DirentFlags, CD_ATTRIBUTE_ASSOC ) ||
                !RtlEqualMemory( PreviousDirent->CdCaseFileName.FileName.Buffer,
                                 ThisDirent->CdCaseFileName.FileName.Buffer,
                                 ThisDirent->CdCaseFileName.FileName.Length )) {

                //
                //  If we match all names then return to our caller.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_ENUM_MATCH_ALL )) {

                    FileContext->ShortName.FileName.Length = 0;
                    Found = TRUE;
                    break;
                }

                //
                //  Check if the long name matches the search expression.
                //

                if (CdIsNameInExpression( IrpContext,
                                          &ThisDirent->CdCaseFileName,
                                          &Ccb->SearchExpression,
                                          Ccb->Flags,
                                          TRUE )) {

                    //
                    //  Let our caller know we found an entry.
                    //

                    Found = TRUE;
                    FileContext->ShortName.FileName.Length = 0;
                    break;
                }

                //
                //  The long name didn't match so we need to check for a
                //  possible short name match.  There is no match if the
                //  long name is 8dot3 or the search expression has a
                //  version component.  Special case the self and parent
                //  entries.
                //

                if ((Ccb->SearchExpression.VersionString.Length == 0) &&
                    !FlagOn( ThisDirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY ) &&
                    !CdIs8dot3Name( IrpContext,
                                    ThisDirent->CdFileName.FileName )) {

                    CdGenerate8dot3Name( IrpContext,
                                         &ThisDirent->CdCaseFileName.FileName,
                                         ThisDirent->DirentOffset,
                                         FileContext->ShortName.FileName.Buffer,
                                         &FileContext->ShortName.FileName.Length );

                    //
                    //  Check if this name matches.
                    //

                    if (CdIsNameInExpression( IrpContext,
                                              &FileContext->ShortName,
                                              &Ccb->SearchExpression,
                                              Ccb->Flags,
                                              FALSE )) {

                        //
                        //  Let our caller know we found an entry.
                        //

                        Found = TRUE;
                        break;
                    }
                }
            }
        }
    }

    //
    //  If we found the entry then make sure we walk through all of the
    //  file dirents.
    //

    if (Found) {

        CdLookupLastFileDirent( IrpContext, Ccb->Fcb, FileContext );
    }

    return Found;
}


