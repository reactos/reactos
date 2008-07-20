/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Cdfs called by the
    Fsd/Fsp dispatch routines.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CREATE)

//
//  Local support routines
//

NTSTATUS
CdNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN PCCB RelatedCcb OPTIONAL,
    IN PUNICODE_STRING RelatedFileName OPTIONAL,
    IN OUT PUNICODE_STRING FileName,
    IN OUT PCD_NAME RemainingName
    );

NTSTATUS
CdOpenByFileId (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb
    );

NTSTATUS
CdOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB *CurrentFcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN IgnoreCase,
    IN PCCB RelatedCcb OPTIONAL
    );

NTSTATUS
CdOpenDirectoryFromPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PCD_NAME DirName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PPATH_ENTRY PathEntry,
    IN BOOLEAN PerformUserOpen,
    IN PCCB RelatedCcb OPTIONAL
    );

NTSTATUS
CdOpenFileFromFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PCD_NAME FileName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PFILE_ENUM_CONTEXT FileContext,
    IN PCCB RelatedCcb OPTIONAL
    );

NTSTATUS
CdCompleteFcbOpen (
    IN PIRP_CONTEXT IrpContext,
    PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ULONG UserCcbFlags,
    IN ACCESS_MASK DesiredAccess
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonCreate)
#pragma alloc_text(PAGE, CdCompleteFcbOpen)
#pragma alloc_text(PAGE, CdNormalizeFileNames)
#pragma alloc_text(PAGE, CdOpenByFileId)
#pragma alloc_text(PAGE, CdOpenDirectoryFromPathEntry)
#pragma alloc_text(PAGE, CdOpenExistingFcb)
#pragma alloc_text(PAGE, CdOpenFileFromFileContext)
#endif


NTSTATUS
CdCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for opening a file called by both the
    Fsp and Fsd threads.

    The file can be opened either by name or by file Id either with or without
    a relative name.  The file name field in the file object passed to this routine
    contains either a unicode string or a 64 bit value which is the file Id.
    If this is not a Joliet disk then we will convert the unicode name to
    an Oem string in this routine.  If there is a related file object with
    a name then we will already have converted that name to Oem.

    We will store the full name for the file in the file object on a successful
    open.  We will allocate a larger buffer if necessary and combine the
    related and file object names.  The only exception is the relative open
    when the related file object is for an OpenByFileId file.  If we need to
    allocate a buffer for a case insensitive name then we allocate it at
    the tail of the buffer we will store into the file object.  The upcased
    portion will begin immediately after the name defined by the FileName
    in the file object.

    Once we have the full name in the file object we don't want to split the
    name in the event of a retry.  We use a flag in the IrpContext to indicate
    that the name has been split.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - This is the status from this open operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFILE_OBJECT FileObject;

    COMPOUND_PATH_ENTRY CompoundPathEntry;
    BOOLEAN CleanupCompoundPathEntry = FALSE;

    FILE_ENUM_CONTEXT FileContext;
    BOOLEAN CleanupFileContext = FALSE;
    BOOLEAN FoundEntry;

    PVCB Vcb;

    BOOLEAN OpenByFileId;
    BOOLEAN IgnoreCase;
    ULONG CreateDisposition;

    BOOLEAN ShortNameMatch;
    ULONG ShortNameDirentOffset;

    BOOLEAN VolumeOpen = FALSE;

    //
    //  We will be acquiring and releasing file Fcb's as we move down the
    //  directory tree during opens.  At any time we need to know the deepest
    //  point we have traversed down in the tree in case we need to cleanup
    //  any structures created here.
    //
    //  CurrentFcb - represents this point.  If non-null it means we have
    //      acquired it and need to release it in finally clause.
    //
    //  NextFcb - represents the NextFcb to walk to but haven't acquired yet.
    //

    TYPE_OF_OPEN RelatedTypeOfOpen = UnopenedFileObject;
    PFILE_OBJECT RelatedFileObject;
    PCCB RelatedCcb = NULL;

    PFCB NextFcb;
    PFCB CurrentFcb = NULL;

    //
    //  During the open we need to combine the related file object name
    //  with the remaining name.  We also may need to upcase the file name
    //  in order to do a case-insensitive name comparison.  We also need
    //  to restore the name in the file object in the event that we retry
    //  the request.  We use the following string variables to manage the
    //  name.  We will can put these strings into either Unicode or Ansi
    //  form.
    //
    //  FileName - Pointer to name as currently stored in the file
    //      object.  We store the full name into the file object early in
    //      the open operation.
    //
    //  RelatedFileName - Pointer to the name in the related file object.
    //
    //  RemainingName - String containing remaining name to parse.
    //
    //  MatchingName - Address of name structure in FileContext which matched.
    //      We need this to know whether we matched the long or short name.
    //

    PUNICODE_STRING FileName;
    PUNICODE_STRING RelatedFileName = NULL;

    CD_NAME RemainingName;
    CD_NAME FinalName;
    PCD_NAME MatchingName;

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS.
    //

    if (IrpContext->Vcb == NULL) {

        CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Get create parameters from the Irp.
    //

    OpenByFileId = BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID );
    IgnoreCase = !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE );
    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    //
    //  Do some preliminary checks to make sure the operation is supported.
    //  We fail in the following cases immediately.
    //
    //      - Open a paging file.
    //      - Open a target directory.
    //      - Open a file with Eas.
    //      - Create a file.
    //

    if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE | SL_OPEN_TARGET_DIRECTORY) ||
        (IrpSp->Parameters.Create.EaLength != 0) ||
        (CreateDisposition == FILE_CREATE)) {

        CdCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Copy the Vcb to a local.  Assume the starting directory is the root.
    //

    Vcb = IrpContext->Vcb;
    NextFcb = Vcb->RootIndexFcb;

    //
    //  Reference our input parameters to make things easier
    //

    FileObject = IrpSp->FileObject;
    RelatedFileObject = NULL;

    FileName = &FileObject->FileName;

    //
    //  Set up the file object's Vpb pointer in case anything happens.
    //  This will allow us to get a reasonable pop-up.
    //

    if ((FileObject->RelatedFileObject != NULL) && !OpenByFileId) {

        RelatedFileObject = FileObject->RelatedFileObject;
        FileObject->Vpb = RelatedFileObject->Vpb;

        RelatedTypeOfOpen = CdDecodeFileObject( IrpContext, RelatedFileObject, &NextFcb, &RelatedCcb );

        //
        //  Fail the request if this is not a user file object.
        //

        if (RelatedTypeOfOpen < UserVolumeOpen) {

            CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        //
        //  Remember the name in the related file object.
        //

        RelatedFileName = &RelatedFileObject->FileName;
    }

    //
    //  If we haven't initialized the names then make sure the strings are valid.
    //  If this an OpenByFileId then verify the file id buffer.
    //
    //  After this routine returns we know that the full name is in the
    //  FileName buffer and the buffer will hold the upcased portion
    //  of the name yet to parse immediately after the full name in the
    //  buffer.  Any trailing backslash has been removed and the flag
    //  in the IrpContext will indicate whether we removed the
    //  backslash.
    //

    Status = CdNormalizeFileNames( IrpContext,
                                   Vcb,
                                   OpenByFileId,
                                   IgnoreCase,
                                   RelatedTypeOfOpen,
                                   RelatedCcb,
                                   RelatedFileName,
                                   FileName,
                                   &RemainingName );

    //
    //  Return the error code if not successful.
    //

    if (!NT_SUCCESS( Status )) {

        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  We want to acquire the Vcb.  Exclusively for a volume open, shared otherwise.
    //  The file name is empty for a volume open.
    //

    if ((FileName->Length == 0) &&
        (RelatedTypeOfOpen <= UserVolumeOpen) &&
        !OpenByFileId) {

        VolumeOpen = TRUE;
        CdAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    } else {

        CdAcquireVcbShared( IrpContext, Vcb, FALSE );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the Vcb is not in an unusable condition.  This routine
        //  will raise if not usable.
        //

        CdVerifyVcb( IrpContext, Vcb );

        //
        //  If the Vcb is locked then we cannot open another file
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If we are opening this file by FileId then process this immediately
        //  and exit.
        //

        if (OpenByFileId) {

            //
            //  We only allow Dasd opens of audio disks.  Fail this request at
            //  this point.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_AUDIO_DISK )) {

                try_return( Status = STATUS_INVALID_DEVICE_REQUEST );
            }

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  Make sure we can wait for this request.
            //

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            try_return( Status = CdOpenByFileId( IrpContext,
                                                 IrpSp,
                                                 Vcb,
                                                 &CurrentFcb ));
        }

        //
        //  If we are opening this volume Dasd then process this immediately
        //  and exit.
        //

        if (VolumeOpen) {

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  If they wanted to open a directory, surprise.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }

            //
            //  Acquire the Fcb first.
            //

            CurrentFcb = Vcb->VolumeDasdFcb;
            CdAcquireFcbExclusive( IrpContext, CurrentFcb, FALSE );

            try_return( Status = CdOpenExistingFcb( IrpContext,
                                                    IrpSp,
                                                    &CurrentFcb,
                                                    UserVolumeOpen,
                                                    FALSE,
                                                    NULL ));
        }

        //
        //  At this point CurrentFcb points to the deepest Fcb for this open
        //  in the tree.  Let's acquire this Fcb to keep it from being deleted
        //  beneath us.
        //

        CdAcquireFcbExclusive( IrpContext, NextFcb, FALSE );
        CurrentFcb = NextFcb;

        //
        //  Do a prefix search if there is more of the name to parse.
        //

        if (RemainingName.FileName.Length != 0) {

            //
            //  Do the prefix search to find the longest matching name.
            //

            CdFindPrefix( IrpContext,
                          &CurrentFcb,
                          &RemainingName.FileName,
                          IgnoreCase );
        }

        //
        //  If the remaining name length is zero then we have found our
        //  target.
        //

        if (RemainingName.FileName.Length == 0) {

            //
            //  If this is a file so verify the user didn't want to open
            //  a directory.
            //

            if (SafeNodeType( CurrentFcb ) == CDFS_NTC_FCB_DATA) {

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH ) ||
                    FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_NOT_A_DIRECTORY );
                }

                //
                //  The only create disposition we allow is OPEN.
                //

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    try_return( Status = STATUS_ACCESS_DENIED );
                }

                try_return( Status = CdOpenExistingFcb( IrpContext,
                                                        IrpSp,
                                                        &CurrentFcb,
                                                        UserFileOpen,
                                                        IgnoreCase,
                                                        RelatedCcb ));

            //
            //  This is a directory.  Verify the user didn't want to open
            //  as a file.
            //

            } else if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                try_return( Status = STATUS_FILE_IS_A_DIRECTORY );

            //
            //  Open the file as a directory.
            //

            } else {

                //
                //  The only create disposition we allow is OPEN.
                //

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    try_return( Status = STATUS_ACCESS_DENIED );
                }

                try_return( Status = CdOpenExistingFcb( IrpContext,
                                                        IrpSp,
                                                        &CurrentFcb,
                                                        UserDirectoryOpen,
                                                        IgnoreCase,
                                                        RelatedCcb ));
            }
        }

        //
        //  We have more work to do.  We have a starting Fcb which we own shared.
        //  We also have the remaining name to parse.  Walk through the name
        //  component by component looking for the full name.
        //

        //
        //  Our starting Fcb better be a directory.
        //

        if (!FlagOn( CurrentFcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

            try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  If we can't wait then post this request.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

            CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
        }

        //
        //  Make sure the final name has no version string.
        //

        FinalName.VersionString.Length = 0;

        while (TRUE) {

            ShortNameMatch = FALSE;

            //
            //  Split off the next component from the name.
            //

            CdDissectName( IrpContext,
                           &RemainingName.FileName,
                           &FinalName.FileName );

            //
            //  Go ahead and look this entry up in the path table.
            //

            CdInitializeCompoundPathEntry( IrpContext, &CompoundPathEntry );
            CleanupCompoundPathEntry = TRUE;

            FoundEntry = CdFindPathEntry( IrpContext,
                                          CurrentFcb,
                                          &FinalName,
                                          IgnoreCase,
                                          &CompoundPathEntry );

            //
            //  If we didn't find the entry then check if the current name
            //  is a possible short name.
            //

            if (!FoundEntry) {

                ShortNameDirentOffset = CdShortNameDirentOffset( IrpContext, &FinalName.FileName );

                //
                //  If there is an embedded short name offset then look for the
                //  matching long name in the directory.
                //

                if (ShortNameDirentOffset != MAXULONG) {

                    if (CleanupFileContext) {

                        CdCleanupFileContext( IrpContext, &FileContext );
                    }

                    CdInitializeFileContext( IrpContext, &FileContext );
                    CleanupFileContext = TRUE;

                    FoundEntry = CdFindFileByShortName( IrpContext,
                                                        CurrentFcb,
                                                        &FinalName,
                                                        IgnoreCase,
                                                        ShortNameDirentOffset,
                                                        &FileContext );

                    //
                    //  If we found an entry and it is a directory then look
                    //  this up in the path table.
                    //

                    if (FoundEntry) {

                        ShortNameMatch = TRUE;

                        if (FlagOn( FileContext.InitialDirent->Dirent.DirentFlags,
                                    CD_ATTRIBUTE_DIRECTORY )) {

                            CdCleanupCompoundPathEntry( IrpContext, &CompoundPathEntry );
                            CdInitializeCompoundPathEntry( IrpContext, &CompoundPathEntry );

                            FoundEntry = CdFindPathEntry( IrpContext,
                                                          CurrentFcb,
                                                          &FileContext.InitialDirent->Dirent.CdCaseFileName,
                                                          IgnoreCase,
                                                          &CompoundPathEntry );

                            //
                            //  We better find this entry.
                            //

                            if (!FoundEntry) {

                                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                            }

                            //
                            //  Upcase the name with the short name if case
                            //  insensitive.
                            //

                            if (IgnoreCase) {

                                CdUpcaseName( IrpContext, &FinalName, &FinalName );
                            }

                        //
                        //  We found a matching file.  If we are at the last
                        //  entry then break out of the loop and open the
                        //  file below.  Otherwise we return an error.
                        //

                        } else if (RemainingName.FileName.Length == 0) {

                            //
                            //  Break out of the loop.  We will process the dirent
                            //  below.
                            //

                            MatchingName = &FileContext.ShortName;
                            break;

                        } else {

                            try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                        }
                    }
                }

                //
                //  We didn't find the name in either the path table or as
                //  a short name in a directory.  If the remaining name
                //  length is zero then break out of the loop to search
                //  the directory.
                //

                if (!FoundEntry) {

                    if (RemainingName.FileName.Length == 0) {

                        break;

                    //
                    //  Otherwise this path could not be cracked.
                    //

                    } else {

                        try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                    }
                }
            }

            //
            //  If this is an ignore case open then copy the exact case
            //  in the file object name.  If it was a short name match then
            //  the name must be upcase already.
            //

            if (IgnoreCase && !ShortNameMatch) {

                RtlCopyMemory( FinalName.FileName.Buffer,
                               CompoundPathEntry.PathEntry.CdDirName.FileName.Buffer,
                               CompoundPathEntry.PathEntry.CdDirName.FileName.Length );
            }

            //
            //  If we have found the last component then open this as a directory
            //  and return to our caller.
            //

            if (RemainingName.FileName.Length == 0) {

                if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                //
                //  The only create disposition we allow is OPEN.
                //

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    try_return( Status = STATUS_ACCESS_DENIED );
                }

                try_return( Status = CdOpenDirectoryFromPathEntry( IrpContext,
                                                                   IrpSp,
                                                                   Vcb,
                                                                   &CurrentFcb,
                                                                   &FinalName,
                                                                   IgnoreCase,
                                                                   ShortNameMatch,
                                                                   &CompoundPathEntry.PathEntry,
                                                                   TRUE,
                                                                   RelatedCcb ));
            }

            //
            //  Otherwise open an Fcb for this intermediate index Fcb.
            //

            CdOpenDirectoryFromPathEntry( IrpContext,
                                          IrpSp,
                                          Vcb,
                                          &CurrentFcb,
                                          &FinalName,
                                          IgnoreCase,
                                          ShortNameMatch,
                                          &CompoundPathEntry.PathEntry,
                                          FALSE,
                                          NULL );

            CdCleanupCompoundPathEntry( IrpContext, &CompoundPathEntry );
            CleanupCompoundPathEntry = FALSE;
        }

        //
        //  We need to scan the current directory for a matching file name
        //  if we don't already have one.
        //

        if (!FoundEntry) {

            if (CleanupFileContext) {

                CdCleanupFileContext( IrpContext, &FileContext );
            }

            CdInitializeFileContext( IrpContext, &FileContext );
            CleanupFileContext = TRUE;

            //
            //  Split our search name into separate components.
            //

            CdConvertNameToCdName( IrpContext, &FinalName );

            FoundEntry = CdFindFile( IrpContext,
                                     CurrentFcb,
                                     &FinalName,
                                     IgnoreCase,
                                     &FileContext,
                                     &MatchingName );
        }

        //
        //  If we didn't find a match then check if the name is invalid to
        //  determine which error code to return.
        //

        if (!FoundEntry) {

            if ((CreateDisposition == FILE_OPEN) ||
                (CreateDisposition == FILE_OVERWRITE)) {

                try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
            }

            //
            //  Any other operation return STATUS_ACCESS_DENIED.
            //

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If this is a directory then the disk is corrupt because it wasn't
        //  in the Path Table.
        //

        if (FlagOn( FileContext.InitialDirent->Dirent.Flags, CD_ATTRIBUTE_DIRECTORY )) {

            CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }

        //
        //  Make sure our opener didn't want a directory.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH ) ||
            FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

            try_return( Status = STATUS_NOT_A_DIRECTORY );
        }

        //
        //  The only create disposition we allow is OPEN.
        //

        if ((CreateDisposition != FILE_OPEN) &&
            (CreateDisposition != FILE_OPEN_IF)) {

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If this is an ignore case open then copy the exact case
        //  in the file object name.  Any version portion should
        //  already be upcased.
        //

        if (IgnoreCase) {

            RtlCopyMemory( FinalName.FileName.Buffer,
                           MatchingName->FileName.Buffer,
                           MatchingName->FileName.Length );
        }

        //
        //  Open the file using the file context.  We already have the
        //  first and last dirents.
        //

        try_return( Status = CdOpenFileFromFileContext( IrpContext,
                                                        IrpSp,
                                                        Vcb,
                                                        &CurrentFcb,
                                                        &FinalName,
                                                        IgnoreCase,
                                                        (BOOLEAN) (MatchingName == &FileContext.ShortName),
                                                        &FileContext,
                                                        RelatedCcb ));

    try_exit:  NOTHING;
    } finally {

        //
        //  Cleanup the PathEntry if initialized.
        //

        if (CleanupCompoundPathEntry) {

            CdCleanupCompoundPathEntry( IrpContext, &CompoundPathEntry );
        }

        //
        //  Cleanup the FileContext if initialized.
        //

        if (CleanupFileContext) {

            CdCleanupFileContext( IrpContext, &FileContext );
        }

        //
        //  The result of this open could be success, pending or some error
        //  condition.
        //

        if (AbnormalTermination()) {


            //
            //  In the error path we start by calling our teardown routine if we
            //  have a CurrentFcb.
            //

            if (CurrentFcb != NULL) {

                BOOLEAN RemovedFcb;

                CdTeardownStructures( IrpContext, CurrentFcb, &RemovedFcb );

                if (RemovedFcb) {

                    CurrentFcb = NULL;
                }
            }

            //
            //  No need to complete the request.
            //

            IrpContext = NULL;
            Irp = NULL;

        //
        //  If we posted this request through the oplock package we need
        //  to show that there is no reason to complete the request.
        //

        } else if (Status == STATUS_PENDING) {

            IrpContext = NULL;
            Irp = NULL;
        }

        //
        //  Release the Current Fcb if still acquired.
        //

        if (CurrentFcb != NULL) {

            CdReleaseFcb( IrpContext, CurrentFcb );
        }

        //
        //  Release the Vcb.
        //

        CdReleaseVcb( IrpContext, Vcb );

        //
        //  Call our completion routine.  It will handle the case where either
        //  the Irp and/or IrpContext are gone.
        //

        CdCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN PCCB RelatedCcb OPTIONAL,
    IN PUNICODE_STRING RelatedFileName OPTIONAL,
    IN OUT PUNICODE_STRING FileName,
    IN OUT PCD_NAME RemainingName
    )

/*++

Routine Description:

    This routine is called to store the full name and upcased name into the
    filename buffer.  We only upcase the portion yet to parse.  We also
    check for a trailing backslash and lead-in double backslashes.  This
    routine also verifies the mode of the related open against the name
    currently in the filename.

Arguments:

    Vcb - Vcb for this volume.

    OpenByFileId - Indicates if the filename should be a 64 bit FileId.

    IgnoreCase - Indicates if this open is a case-insensitive operation.

    RelatedTypeOfOpen - Indicates the type of the related file object.

    RelatedCcb - Ccb for the related open.  Ignored if no relative open.

    RelatedFileName - FileName buffer for related open.  Ignored if no
        relative open.

    FileName - FileName to update in this routine.  The name should
        either be a 64-bit FileId or a Unicode string.

    RemainingName - Name with the remaining portion of the name.  This
        will begin after the related name and any separator.  For a
        non-relative open we also step over the initial separator.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the names are OK, appropriate error code
        otherwise.

--*/

{
    ULONG RemainingNameLength;
    ULONG RelatedNameLength = 0;
    ULONG SeparatorLength = 0;

    ULONG BufferLength;

    UNICODE_STRING NewFileName;

    PAGED_CODE();

    //
    //  If this is the first pass then we need to build the full name and
    //  check for name compatibility.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_FULL_NAME )) {

        //
        //  Deal with the regular file name case first.
        //

        if (!OpenByFileId) {

            //
            //  This is here because the Win32 layer can't avoid sending me double
            //  beginning backslashes.
            //
            
            if ((FileName->Length > sizeof( WCHAR )) &&
                (FileName->Buffer[1] == L'\\') &&
                (FileName->Buffer[0] == L'\\')) {

                //
                //  If there are still two beginning backslashes, the name is bogus.
                //

                if ((FileName->Length > 2 * sizeof( WCHAR )) &&
                    (FileName->Buffer[2] == L'\\')) {

                    return STATUS_OBJECT_NAME_INVALID;
                }

                //
                //  Slide the name down in the buffer.
                //

                FileName->Length -= sizeof( WCHAR );

                RtlMoveMemory( FileName->Buffer,
                               FileName->Buffer + 1,
                               FileName->Length );
            }

            //
            //  Check for a trailing backslash.  Don't strip off if only character
            //  in the full name or for relative opens where this is illegal.
            //

            if (((FileName->Length > sizeof( WCHAR)) ||
                 ((FileName->Length == sizeof( WCHAR )) && (RelatedTypeOfOpen == UserDirectoryOpen))) &&
                (FileName->Buffer[ (FileName->Length/2) - 1 ] == L'\\')) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH );
                FileName->Length -= sizeof( WCHAR );
            }

            //
            //  Remember the length we need for this portion of the name.
            //

            RemainingNameLength = FileName->Length;

            //
            //  If this is a related file object then we verify the compatibility
            //  of the name in the file object with the relative file object.
            //

            if (RelatedTypeOfOpen != UnopenedFileObject) {

                //
                //  If the filename length was zero then it must be legal.
                //  If there are characters then check with the related
                //  type of open.
                //

                if (FileName->Length != 0) {

                    //
                    //  The name length must always be zero for a volume open.
                    //

                    if (RelatedTypeOfOpen <= UserVolumeOpen) {

                        return STATUS_INVALID_PARAMETER;

                    //
                    //  The remaining name cannot begin with a backslash.
                    //

                    } else if (FileName->Buffer[0] == L'\\' ) {

                        return STATUS_INVALID_PARAMETER;

                    //
                    //  If the related file is a user file then there
                    //  is no file with this path.
                    //

                    } else if (RelatedTypeOfOpen == UserFileOpen) {

                        return STATUS_OBJECT_PATH_NOT_FOUND;
                    }
                }

                //
                //  Remember the length of the related name when building
                //  the full name.  We leave the RelatedNameLength and
                //  SeparatorLength at zero if the relative file is opened
                //  by Id.
                //

                if (!FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    //
                    //  Add a separator if the name length is non-zero
                    //  unless the relative Fcb is at the root.
                    //

                    if ((FileName->Length != 0) &&
                        (RelatedCcb->Fcb != Vcb->RootIndexFcb)) {

                        SeparatorLength = sizeof( WCHAR );
                    }

                    RelatedNameLength = RelatedFileName->Length;
                }

            //
            //  The full name is already in the filename.  It must either
            //  be length 0 or begin with a backslash.
            //

            } else if (FileName->Length != 0) {

                if (FileName->Buffer[0] != L'\\') {

                    return STATUS_INVALID_PARAMETER;
                }

                //
                //  We will want to trim the leading backslash from the
                //  remaining name we return.
                //

                RemainingNameLength -= sizeof( WCHAR );
                SeparatorLength = sizeof( WCHAR );
            }

            //
            //  Now see if the buffer is large enough to hold the full name.
            //

            BufferLength = RelatedNameLength + SeparatorLength + RemainingNameLength;

            //
            //  Check for an overflow of the maximum filename size.
            //
            
            if (BufferLength > MAXUSHORT) {

                return STATUS_INVALID_PARAMETER;
            }
            
            //
            //  Now see if we need to allocate a new buffer.
            //

            if (FileName->MaximumLength < BufferLength) {

                NewFileName.Buffer = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                               BufferLength,
                                                               TAG_FILE_NAME );

                NewFileName.MaximumLength = (USHORT) BufferLength;

            } else {

                NewFileName.Buffer = FileName->Buffer;
                NewFileName.MaximumLength = FileName->MaximumLength;
            }

            //
            //  If there is a related name then we need to slide the remaining bytes up and
            //  insert the related name.  Otherwise the name is in the correct position
            //  already.
            //

            if (RelatedNameLength != 0) {

                //
                //  Store the remaining name in its correct position.
                //

                if (RemainingNameLength != 0) {

                    RtlMoveMemory( Add2Ptr( NewFileName.Buffer, RelatedNameLength + SeparatorLength, PVOID ),
                                   FileName->Buffer,
                                   RemainingNameLength );
                }

                RtlCopyMemory( NewFileName.Buffer,
                               RelatedFileName->Buffer,
                               RelatedNameLength );

                //
                //  Add the separator if needed.
                //

                if (SeparatorLength != 0) {

                    *(Add2Ptr( NewFileName.Buffer, RelatedNameLength, PWCHAR )) = L'\\';
                }

                //
                //  Update the filename value we got from the user.
                //

                if (NewFileName.Buffer != FileName->Buffer) {

                    if (FileName->Buffer != NULL) {

                        CdFreePool( &FileName->Buffer );
                    }

                    FileName->Buffer = NewFileName.Buffer;
                    FileName->MaximumLength = NewFileName.MaximumLength;
                }

                //
                //  Copy the name length to the user's filename.
                //

                FileName->Length = (USHORT) (RelatedNameLength + SeparatorLength + RemainingNameLength);
            }

            //
            //  Now update the remaining name to parse.
            //

            RemainingName->FileName.MaximumLength =
            RemainingName->FileName.Length = (USHORT) RemainingNameLength;
            RemainingName->VersionString.Length = 0;

            RemainingName->FileName.Buffer = Add2Ptr( FileName->Buffer,
                                                      RelatedNameLength + SeparatorLength,
                                                      PWCHAR );

            //
            //  Upcase the name if necessary.
            //

            if (IgnoreCase && (RemainingNameLength != 0)) {

                CdUpcaseName( IrpContext,
                              RemainingName,
                              RemainingName );
            }

            //
            //  Do a quick check to make sure there are no wildcards.
            //

            if (FsRtlDoesNameContainWildCards( &RemainingName->FileName )) {

                return STATUS_OBJECT_NAME_INVALID;
            }

        //
        //  For the open by file Id case we verify the name really contains
        //  a 64 bit value.
        //

        } else {

            //
            //  Check for validity of the buffer.
            //

            if (FileName->Length != sizeof( FILE_ID )) {

                return STATUS_INVALID_PARAMETER;
            }
        }

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FULL_NAME );

    //
    //  If we are in the retry path then the full name is already in the
    //  file object name.  If this is a case-sensitive operation then
    //  we need to upcase the name from the end of any related file name already stored
    //  there.
    //

    } else {

        //
        //  Assume there is no relative name.
        //

        RemainingName->FileName = *FileName;
        RemainingName->VersionString.Length = 0;

        //
        //  Nothing to do if the name length is zero.
        //

        if (RemainingName->FileName.Length != 0) {

            //
            //  If there is a relative name then we need to walk past it.
            //

            if (RelatedTypeOfOpen != UnopenedFileObject) {

                //
                //  Nothing to walk past if the RelatedCcb is opened by FileId.
                //


                if (!FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID )) {

                    //
                    //  Related file name is a proper prefix of the full name.
                    //  We step over the related name and if we are then
                    //  pointing at a separator character we step over that.
                    //

                    RemainingName->FileName.Buffer = Add2Ptr( RemainingName->FileName.Buffer,
                                                              RelatedFileName->Length,
                                                              PWCHAR );

                    RemainingName->FileName.Length -= RelatedFileName->Length;
                }
            }

            //
            //  If we are pointing at a separator character then step past that.
            //

            if (RemainingName->FileName.Length != 0) {

                if (*(RemainingName->FileName.Buffer) == L'\\') {

                    RemainingName->FileName.Buffer = Add2Ptr( RemainingName->FileName.Buffer,
                                                              sizeof( WCHAR ),
                                                              PWCHAR );

                    RemainingName->FileName.Length -= sizeof( WCHAR );
                }
            }
        }

        //
        //  Upcase the name if necessary.
        //

        if (IgnoreCase && (RemainingName->FileName.Length != 0)) {

            CdUpcaseName( IrpContext,
                          RemainingName,
                          RemainingName );
        }
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
CdOpenByFileId (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb
    )

/*++

Routine Description:

    This routine is called to open a file by the FileId.  The file Id is in
    the FileObject name buffer and has been verified to be 64 bits.

    We extract the Id number and then check to see whether we are opening a
    file or directory and compare that with the create options.  If this
    generates no error then optimistically look up the Fcb in the Fcb Table.

    If we don't find the Fcb then we need to carefully verify there is a file
    at this offset.  First check whether the Parent Fcb is in the table.  If
    not then lookup the parent at the path table offset given by file ID.

    If found then build the Fcb from this entry and store the new Fcb in the
    tree.

    We know have the parent Fcb.  Do a directory scan to find the dirent at
    the given offset in this stream.  This must point to the first entry
    of a valid file.

    Finally we call our worker routine to complete the open on this Fcb.

Arguments:

    IrpSp - Stack location within the create Irp.

    Vcb - Vcb for this volume.

    CurrentFcb - Address to store the Fcb for this open.  We only store the
        CurrentFcb here when we have acquired it so our caller knows to
        free or deallocate it.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    NTSTATUS Status = STATUS_ACCESS_DENIED;

    BOOLEAN UnlockVcb = FALSE;
    BOOLEAN Found;

    ULONG StreamOffset;

    NODE_TYPE_CODE NodeTypeCode;
    TYPE_OF_OPEN TypeOfOpen;

    FILE_ENUM_CONTEXT FileContext;
    BOOLEAN CleanupFileContext = FALSE;

    COMPOUND_PATH_ENTRY CompoundPathEntry;
    BOOLEAN CleanupCompoundPathEntry = FALSE;

    FILE_ID FileId;
    FILE_ID ParentFileId;

    PFCB NextFcb;

    PAGED_CODE();

    //
    //  Extract the FileId from the FileObject.
    //

    RtlCopyMemory( &FileId, IrpSp->FileObject->FileName.Buffer, sizeof( FILE_ID ));

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Go ahead and figure out the TypeOfOpen and NodeType.  We can
        //  get these from the input FileId.
        //

        if (CdFidIsDirectory( FileId )) {

            TypeOfOpen = UserDirectoryOpen;
            NodeTypeCode = CDFS_NTC_FCB_INDEX;

            //
            //  If the offset isn't zero then the file Id is bad.
            //

            if (CdQueryFidDirentOffset( FileId ) != 0) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

        } else {

            TypeOfOpen = UserFileOpen;
            NodeTypeCode = CDFS_NTC_FCB_DATA;
        }

        //
        //  Acquire the Vcb and check if there is already an Fcb.
        //  If not we will need to carefully verify the Fcb.
        //  We will post the request if we don't find the Fcb and this
        //  request can't wait.
        //

        CdLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        NextFcb = CdLookupFcbTable( IrpContext, Vcb, FileId );

        if (NextFcb == NULL) {

            //
            //  Get the path table offset from the file id.
            //

            StreamOffset = CdQueryFidPathTableOffset( FileId );

            //
            //  Build the parent FileId for this and try looking it
            //  up in the PathTable.
            //

            CdSetFidDirentOffset( ParentFileId, 0 );
            CdSetFidPathTableOffset( ParentFileId, StreamOffset );
            CdFidSetDirectory( ParentFileId );

            NextFcb = CdLookupFcbTable( IrpContext, Vcb, ParentFileId );

            //
            //  If not present then walk through the PathTable to this point.
            //

            if (NextFcb == NULL) {

                CdUnlockVcb( IrpContext, Vcb );
                UnlockVcb = FALSE;

                //
                //  Check that the path table offset lies within the path
                //  table.
                //

                if (StreamOffset > Vcb->PathTableFcb->FileSize.LowPart) {

                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                CdInitializeCompoundPathEntry( IrpContext, &CompoundPathEntry );
                CleanupCompoundPathEntry = TRUE;

                //
                //  Start at the first entry in the PathTable.
                //

                CdLookupPathEntry( IrpContext,
                                   Vcb->PathTableFcb->StreamOffset,
                                   1,
                                   TRUE,
                                   &CompoundPathEntry );

                //
                //  Continue looking until we have passed our target offset.
                //

                while (TRUE) {

                    //
                    //  Move to the next entry.
                    //

                    Found = CdLookupNextPathEntry( IrpContext,
                                                   &CompoundPathEntry.PathContext,
                                                   &CompoundPathEntry.PathEntry );

                    //
                    //  If we didn't find the entry or are beyond it then the
                    //  input Id is invalid.
                    //

                    if (!Found ||
                        (CompoundPathEntry.PathEntry.PathTableOffset > StreamOffset)) {

                        try_return( Status = STATUS_INVALID_PARAMETER );
                    }
                }

                //
                //  If the FileId specified a directory then we have found
                //  the entry.  Make sure our caller wanted to open a directory.
                //

                if ((TypeOfOpen == UserDirectoryOpen) &&
                    FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                //
                //  Lock the Vcb and create the Fcb if necessary.
                //

                CdLockVcb( IrpContext, Vcb );
                UnlockVcb = TRUE;

                NextFcb = CdCreateFcb( IrpContext, ParentFileId, NodeTypeCode, &Found );

                //
                //  It's possible that someone got in here ahead of us.
                //

                if (!Found) {

                    CdInitializeFcbFromPathEntry( IrpContext,
                                                  NextFcb,
                                                  NULL,
                                                  &CompoundPathEntry.PathEntry );
                }

                //
                //  If the user wanted to open a directory then we have found
                //  it.  Store this Fcb into the CurrentFcb and skip the
                //  directory scan.
                //

                if (TypeOfOpen == UserDirectoryOpen) {

                    *CurrentFcb = NextFcb;
                    NextFcb = NULL;
                }
            }

            //
            //  Perform the directory scan if we don't already have our target.
            //

            if (NextFcb != NULL) {

                //
                //  Acquire the parent.  We currently own the Vcb lock so
                //  do this without waiting first.
                //

                if (!CdAcquireFcbExclusive( IrpContext,
                                            NextFcb,
                                            TRUE )) {

                    NextFcb->FcbReference += 1;
                    CdUnlockVcb( IrpContext, Vcb );

                    CdAcquireFcbExclusive( IrpContext, NextFcb, FALSE );

                    CdLockVcb( IrpContext, Vcb );
                    NextFcb->FcbReference -= 1;
                    CdUnlockVcb( IrpContext, Vcb );

                } else {

                    CdUnlockVcb( IrpContext, Vcb );
                }

                UnlockVcb = FALSE;

                //
                //  Set up the CurrentFcb pointers.  We know there was
                //  no previous parent in this case.
                //

                *CurrentFcb = NextFcb;

                //
                //  Calculate the offset in the stream.
                //

                StreamOffset = CdQueryFidDirentOffset( FileId );

                //
                //  Create the stream file if it doesn't exist.  This will update
                //  the Fcb with the size from the self entry.
                //

                if (NextFcb->FileObject == NULL) {

                    CdCreateInternalStream( IrpContext, Vcb, NextFcb );
                }

                //
                //  If our offset is beyond the end of the directory then the
                //  FileId is invalid.
                //

                if (StreamOffset > NextFcb->FileSize.LowPart) {

                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                //
                //  Otherwise position ourselves at the self entry and walk
                //  through dirent by dirent until this location is found.
                //

                CdInitializeFileContext( IrpContext, &FileContext );
                CdLookupInitialFileDirent( IrpContext,
                                           NextFcb,
                                           &FileContext,
                                           NextFcb->StreamOffset );

                CleanupFileContext = TRUE;

                while (TRUE) {

                    //
                    //  Move to the first entry of the next file.
                    //

                    Found = CdLookupNextInitialFileDirent( IrpContext,
                                                           NextFcb,
                                                           &FileContext );

                    //
                    //  If we didn't find the entry or are beyond it then the
                    //  input Id is invalid.
                    //

                    if (!Found ||
                        (FileContext.InitialDirent->Dirent.DirentOffset > StreamOffset)) {

                        try_return( Status = STATUS_INVALID_PARAMETER );
                    }
                }

                //
                //  This better not be a directory.  Directory FileIds must
                //  refer to the self entry for directories.
                //

                if (FlagOn( FileContext.InitialDirent->Dirent.DirentFlags,
                            CD_ATTRIBUTE_DIRECTORY )) {

                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                //
                //  Check that our caller wanted to open a file.
                //

                if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_NOT_A_DIRECTORY );
                }

                //
                //  Otherwise we want to collect all of the dirents for this file
                //  and create an Fcb with this.
                //

                CdLookupLastFileDirent( IrpContext, NextFcb, &FileContext );

                CdLockVcb( IrpContext, Vcb );
                UnlockVcb = TRUE;

                NextFcb = CdCreateFcb( IrpContext, FileId, NodeTypeCode, &Found );

                //
                //  It's possible that someone has since created this Fcb since we
                //  first checked.  If so then can simply use this.  Otherwise
                //  we need to initialize a new Fcb and attach it to our parent
                //  and insert it into the Fcb Table.
                //

                if (!Found) {

                    CdInitializeFcbFromFileContext( IrpContext,
                                                    NextFcb,
                                                    *CurrentFcb,
                                                    &FileContext );
                }
            }

        //
        //  We have the Fcb.  Check that the type of the file is compatible with
        //  the desired type of file to open.
        //

        } else {

            if (FlagOn( NextFcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

                if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

            } else if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }
        }

        //
        //  If we have a the previous Fcb and have inserted the next Fcb into
        //  the Fcb Table.  It is safe to release the current Fcb if present
        //  since it is referenced through the child Fcb.
        //

        if (*CurrentFcb != NULL) {

            CdReleaseFcb( IrpContext, *CurrentFcb );
        }

        //
        //  We now know the Fcb and currently hold the Vcb lock.
        //  Try to acquire this Fcb without waiting.  Otherwise we
        //  need to reference it, drop the Vcb, acquire the Fcb and
        //  then dereference the Fcb.
        //

        if (!CdAcquireFcbExclusive( IrpContext, NextFcb, TRUE )) {

            NextFcb->FcbReference += 1;

            CdUnlockVcb( IrpContext, Vcb );

            CdAcquireFcbExclusive( IrpContext, NextFcb, FALSE );

            CdLockVcb( IrpContext, Vcb );
            NextFcb->FcbReference -= 1;
            CdUnlockVcb( IrpContext, Vcb );

        } else {

            CdUnlockVcb( IrpContext, Vcb );
        }

        UnlockVcb = FALSE;

        //
        //  Move to this Fcb.
        //

        *CurrentFcb = NextFcb;

        //
        //  Check the requested access on this Fcb.
        //

        if (!CdIllegalFcbAccess( IrpContext,
                                 TypeOfOpen,
                                 IrpSp->Parameters.Create.SecurityContext->DesiredAccess )) {

            //
            //  Call our worker routine to complete the open.
            //

            Status = CdCompleteFcbOpen( IrpContext,
                                        IrpSp,
                                        Vcb,
                                        CurrentFcb,
                                        TypeOfOpen,
                                        CCB_FLAG_OPEN_BY_ID,
                                        IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
        }

    try_exit:  NOTHING;
    } finally {

        if (UnlockVcb) {

            CdUnlockVcb( IrpContext, Vcb );
        }

        if (CleanupFileContext) {

            CdCleanupFileContext( IrpContext, &FileContext );
        }

        if (CleanupCompoundPathEntry) {

            CdCleanupCompoundPathEntry( IrpContext, &CompoundPathEntry );
        }
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB *CurrentFcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN IgnoreCase,
    IN PCCB RelatedCcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to open an Fcb which is already in the Fcb table.
    We will verify the access to the file and then call our worker routine
    to perform the final operations.

Arguments:

    IrpSp - Pointer to the stack location for this open.

    CurrentFcb - Address of Fcb to open.  We will clear this if the Fcb
        is released here.

    TypeOfOpen - Indicates whether we are opening a file, directory or volume.

    IgnoreCase - Indicates if this open is case-insensitive.

    RelatedCcb - Ccb for related file object if relative open.  We use
        this when setting the Ccb flags for this open.  It will tell
        us whether the name currently in the file object is relative or
        absolute.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    ULONG CcbFlags = 0;

    NTSTATUS Status = STATUS_ACCESS_DENIED;

    PAGED_CODE();

    //
    //  Check that the desired access is legal.
    //

    if (!CdIllegalFcbAccess( IrpContext,
                             TypeOfOpen,
                             IrpSp->Parameters.Create.SecurityContext->DesiredAccess )) {

        //
        //  Set the Ignore case.
        //

        if (IgnoreCase) {

            SetFlag( CcbFlags, CCB_FLAG_IGNORE_CASE );
        }

        //
        //  Check the related Ccb to see if this was an OpenByFileId and
        //  whether there was a version.
        //

        if (ARGUMENT_PRESENT( RelatedCcb )) {

            SetFlag( CcbFlags, FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_WITH_VERSION ));


            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID )) {

                SetFlag( CcbFlags, CCB_FLAG_OPEN_RELATIVE_BY_ID );
            }
        }

        //
        //  Call our worker routine to complete the open.
        //

        Status = CdCompleteFcbOpen( IrpContext,
                                    IrpSp,
                                    (*CurrentFcb)->Vcb,
                                    CurrentFcb,
                                    TypeOfOpen,
                                    CcbFlags,
                                    IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdOpenDirectoryFromPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PCD_NAME DirName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PPATH_ENTRY PathEntry,
    IN BOOLEAN PerformUserOpen,
    IN PCCB RelatedCcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to open a directory where the directory was found
    in the path table.  This routine is called in the case where this is the
    file to open for the user and where this is an intermediate node in the
    full path to open.

    We first check that the desired access is legal for a directory.  Then we
    construct the FileId for this and do a check to see if it is the Fcb
    Table.  It is always possible that either it was created since or simply
    wasn't in the prefix table at the time of the prefix table search.
    Initialize the Fcb and store into the FcbTable if not present.

    Next we will add this to the prefix table of our parent if needed.

    Once we know that the new Fcb has been initialized then we move our pointer
    in the tree down to this position.

    This routine does not own the Vcb lock on entry.  We must be sure to release
    it on exit.

Arguments:

    IrpSp - Stack location for this request.

    Vcb - Vcb for this volume.

    CurrentFcb - On input this is the parent of the Fcb to open.  On output we
        store the Fcb for the file being opened.

    DirName - This is always the exact name used to reach this file.

    IgnoreCase - Indicates the type of case match for the open.

    ShortNameMatch - Indicates if we are opening via the short name.

    PathEntry - Path entry for the entry found.

    PerformUserOpen - TRUE if we are to open this for a user, FALSE otherwise.

    RelatedCcb - RelatedCcb for relative file object used to make this open.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    ULONG CcbFlags = 0;
    FILE_ID FileId;

    BOOLEAN UnlockVcb = FALSE;
    BOOLEAN FcbExisted;

    PFCB NextFcb;
    PFCB ParentFcb = NULL;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check for illegal access to this file.
    //

    if (PerformUserOpen &&
        CdIllegalFcbAccess( IrpContext,
                            UserDirectoryOpen,
                            IrpSp->Parameters.Create.SecurityContext->DesiredAccess )) {

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Check the related Ccb to see if this was an OpenByFileId.
        //

        if (ARGUMENT_PRESENT( RelatedCcb ) &&
            FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID )) {

            CcbFlags = CCB_FLAG_OPEN_RELATIVE_BY_ID;
        }

        if (IgnoreCase) {

            SetFlag( CcbFlags, CCB_FLAG_IGNORE_CASE );
        }

        //
        //  Build the file Id for this file.
        //

        FileId.QuadPart = 0;
        CdSetFidPathTableOffset( FileId, PathEntry->PathTableOffset );
        CdFidSetDirectory( FileId );

        //
        //  Lock the Vcb so we can examine the Fcb Table.
        //

        CdLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        //
        //  Get the Fcb for this directory.
        //

        NextFcb = CdCreateFcb( IrpContext, FileId, CDFS_NTC_FCB_INDEX, &FcbExisted );

        //
        //  If the Fcb was created here then initialize from the values in the
        //  path table entry.
        //

        if (!FcbExisted) {

            CdInitializeFcbFromPathEntry( IrpContext, NextFcb, *CurrentFcb, PathEntry );
        }

        //
        //  Now try to acquire the new Fcb without waiting.  We will reference
        //  the Fcb and retry with wait if unsuccessful.
        //

        if (!CdAcquireFcbExclusive( IrpContext, NextFcb, TRUE )) {

            NextFcb->FcbReference += 1;

            CdUnlockVcb( IrpContext, Vcb );

            CdReleaseFcb( IrpContext, *CurrentFcb );
            CdAcquireFcbExclusive( IrpContext, NextFcb, FALSE );
            CdAcquireFcbExclusive( IrpContext, *CurrentFcb, FALSE );

            CdLockVcb( IrpContext, Vcb );
            NextFcb->FcbReference -= 1;
            CdUnlockVcb( IrpContext, Vcb );

        } else {

            //
            //  Unlock the Vcb and move down to this new Fcb.  Remember that we still
            //  own the parent however.
            //

            CdUnlockVcb( IrpContext, Vcb );
        }

        UnlockVcb = FALSE;

        ParentFcb = *CurrentFcb;
        *CurrentFcb = NextFcb;

        //
        //  Store this name into the prefix table for the parent.
        //

        if (ShortNameMatch) {

            //
            //  Make sure the exact case is always in the tree.
            //

            CdInsertPrefix( IrpContext,
                            NextFcb,
                            DirName,
                            FALSE,
                            TRUE,
                            ParentFcb );

            if (IgnoreCase) {

                CdInsertPrefix( IrpContext,
                                NextFcb,
                                DirName,
                                TRUE,
                                TRUE,
                                ParentFcb );
            }

        } else {

            //
            //  Make sure the exact case is always in the tree.
            //

            CdInsertPrefix( IrpContext,
                            NextFcb,
                            &PathEntry->CdDirName,
                            FALSE,
                            FALSE,
                            ParentFcb );

            if (IgnoreCase) {

                CdInsertPrefix( IrpContext,
                                NextFcb,
                                &PathEntry->CdCaseDirName,
                                TRUE,
                                FALSE,
                                ParentFcb );
            }
        }

        //
        //  Release the parent Fcb at this point.
        //

        CdReleaseFcb( IrpContext, ParentFcb );
        ParentFcb = NULL;

        //
        //  Call our worker routine to complete the open.
        //

        if (PerformUserOpen) {

            Status = CdCompleteFcbOpen( IrpContext,
                                        IrpSp,
                                        Vcb,
                                        CurrentFcb,
                                        UserDirectoryOpen,
                                        CcbFlags,
                                        IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
        }

    } finally {

        //
        //  Unlock the Vcb if held.
        //

        if (UnlockVcb) {

            CdUnlockVcb( IrpContext, Vcb );
        }

        //
        //  Release the parent if held.
        //

        if (ParentFcb != NULL) {

            CdReleaseFcb( IrpContext, ParentFcb );
        }
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdOpenFileFromFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PCD_NAME FileName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PFILE_ENUM_CONTEXT FileContext,
    IN PCCB RelatedCcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to open a file where the file was found in a directory scan.
    This should only be for a file in the case since we will find the directories in the
    path table.

    We first check that the desired access is legal for this file.  Then we
    construct the FileId for this and do a check to see if it is the Fcb
    Table.  It is always possible that either it was created since or simply
    wasn't in the prefix table at the time of the prefix table search.
    Initialize the Fcb and store into the FcbTable if not present.

    Next we will add this to the prefix table of our parent if needed.

    Once we know that the new Fcb has been initialized then we move our pointer
    in the tree down to this position.

    This routine does not own the Vcb lock on entry.  We must be sure to release
    it on exit.

Arguments:

    IrpSp - Stack location for this request.

    Vcb - Vcb for the current volume.

    CurrentFcb - On input this is the parent of the Fcb to open.  On output we
        store the Fcb for the file being opened.

    FileName - This is always the exact name used to reach this file.

    IgnoreCase - Indicates the type of case of CaseName above.

    ShortNameMatch - Indicates if we are opening via the short name.

    FileContext - This is the context used to find the file.

    RelatedCcb - RelatedCcb for relative file object used to make this open.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    ULONG CcbFlags = 0;
    FILE_ID FileId;

    BOOLEAN UnlockVcb = FALSE;
    BOOLEAN FcbExisted;

    PFCB NextFcb;
    PFCB ParentFcb = NULL;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check for illegal access to this file.
    //

    if (CdIllegalFcbAccess( IrpContext,
                            UserFileOpen,
                            IrpSp->Parameters.Create.SecurityContext->DesiredAccess )) {

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Check if a version number was used to open this file.
        //

        if (FileName->VersionString.Length != 0) {

            SetFlag( CcbFlags, CCB_FLAG_OPEN_WITH_VERSION );
        }

        //
        //  Check the related Ccb to see if this was an OpenByFileId.
        //

        if (ARGUMENT_PRESENT( RelatedCcb ) &&
            FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID )) {

            SetFlag( CcbFlags, CCB_FLAG_OPEN_RELATIVE_BY_ID );
        }

        if (IgnoreCase) {

            SetFlag( CcbFlags, CCB_FLAG_IGNORE_CASE );
        }

        //
        //  Build the file Id for this file.  We can use the path table offset from the
        //  parent and the directory offset from the dirent.
        //

        CdSetFidPathTableOffset( FileId, CdQueryFidPathTableOffset( (*CurrentFcb)->FileId ));
        CdSetFidDirentOffset( FileId, FileContext->InitialDirent->Dirent.DirentOffset );

        //
        //  Lock the Vcb so we can examine the Fcb Table.
        //

        CdLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        //
        //  Get the Fcb for this file.
        //

        NextFcb = CdCreateFcb( IrpContext, FileId, CDFS_NTC_FCB_DATA, &FcbExisted );

        //
        //  If the Fcb was created here then initialize from the values in the
        //  dirent.
        //

        if (!FcbExisted) {

            CdInitializeFcbFromFileContext( IrpContext,
                                            NextFcb,
                                            *CurrentFcb,
                                            FileContext );
        }

        //
        //  Now try to acquire the new Fcb without waiting.  We will reference
        //  the Fcb and retry with wait if unsuccessful.
        //

        if (!CdAcquireFcbExclusive( IrpContext, NextFcb, TRUE )) {

            NextFcb->FcbReference += 1;

            CdUnlockVcb( IrpContext, Vcb );

            CdReleaseFcb( IrpContext, *CurrentFcb );
            CdAcquireFcbExclusive( IrpContext, NextFcb, FALSE );
            CdAcquireFcbExclusive( IrpContext, *CurrentFcb, FALSE );

            CdLockVcb( IrpContext, Vcb );
            NextFcb->FcbReference -= 1;
            CdUnlockVcb( IrpContext, Vcb );

        } else {

            //
            //  Unlock the Vcb and move down to this new Fcb.  Remember that we still
            //  own the parent however.
            //

            CdUnlockVcb( IrpContext, Vcb );
        }

        UnlockVcb = FALSE;

        ParentFcb = *CurrentFcb;
        *CurrentFcb = NextFcb;

        //
        //  Store this name into the prefix table for the parent.
        //


        if (ShortNameMatch) {

            //
            //  Make sure the exact case is always in the tree.
            //

            CdInsertPrefix( IrpContext,
                            NextFcb,
                            FileName,
                            FALSE,
                            TRUE,
                            ParentFcb );

            if (IgnoreCase) {

                CdInsertPrefix( IrpContext,
                                NextFcb,
                                FileName,
                                TRUE,
                                TRUE,
                                ParentFcb );
            }

        //
        //  Insert this into the prefix table if we found this without
        //  using a version string.
        //

        } else if (FileName->VersionString.Length == 0) {

            //
            //  Make sure the exact case is always in the tree.
            //

            CdInsertPrefix( IrpContext,
                            NextFcb,
                            &FileContext->InitialDirent->Dirent.CdFileName,
                            FALSE,
                            FALSE,
                            ParentFcb );

            if (IgnoreCase) {

                CdInsertPrefix( IrpContext,
                                NextFcb,
                                &FileContext->InitialDirent->Dirent.CdCaseFileName,
                                TRUE,
                                FALSE,
                                ParentFcb );
            }
        }

        //
        //  Release the parent Fcb at this point.
        //

        CdReleaseFcb( IrpContext, ParentFcb );
        ParentFcb = NULL;

        //
        //  Call our worker routine to complete the open.
        //

        Status = CdCompleteFcbOpen( IrpContext,
                                    IrpSp,
                                    Vcb,
                                    CurrentFcb,
                                    UserFileOpen,
                                    CcbFlags,
                                    IrpSp->Parameters.Create.SecurityContext->DesiredAccess );

    } finally {

        //
        //  Unlock the Vcb if held.
        //

        if (UnlockVcb) {

            CdUnlockVcb( IrpContext, Vcb );
        }

        //
        //  Release the parent if held.
        //

        if (ParentFcb != NULL) {

            CdReleaseFcb( IrpContext, ParentFcb );
        }
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
CdCompleteFcbOpen (
    IN PIRP_CONTEXT IrpContext,
    PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ULONG UserCcbFlags,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This is the worker routine which takes an existing Fcb and completes
    the open.  We will do any necessary oplock checks and sharing checks.
    Finally we will create the Ccb and update the file object and any
    file object flags.

Arguments:

    IrpSp - Stack location for the current request.

    Vcb - Vcb for the current volume.

    CurrentFcb - Address of pointer to Fcb to open.  We clear this field if
        we release the resource for this file.

    TypeOfOpen - Type of open for this request.

    UserCcbFlags - Flags to OR into the Ccb flags.

    DesiredAccess - Desired access for this open.

Return Value:

    NTSTATUS - STATUS_SUCCESS if we complete this request, STATUS_PENDING if
        the oplock package takes the Irp or SHARING_VIOLATION if there is a
        sharing check conflict.

--*/

{
    NTSTATUS Status;
    NTSTATUS OplockStatus  = STATUS_SUCCESS;
    ULONG Information = FILE_OPENED;

    BOOLEAN LockVolume = FALSE;

    PFCB Fcb = *CurrentFcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Expand maximum allowed to something sensible for share access checking
    //

    if (MAXIMUM_ALLOWED == DesiredAccess)  {
    
        DesiredAccess = FILE_ALL_ACCESS & ~((TypeOfOpen != UserVolumeOpen ?
                                             (FILE_WRITE_ATTRIBUTES           |
                                              FILE_WRITE_DATA                 |
                                              FILE_WRITE_EA                   |
                                              FILE_ADD_FILE                   |                     
                                              FILE_ADD_SUBDIRECTORY           |
                                              FILE_APPEND_DATA) : 0)          |
                                            FILE_DELETE_CHILD                 |
                                            DELETE                            |
                                            WRITE_DAC );
    }

    //
    //  If this a volume open and the user wants to lock the volume then
    //  purge and lock the volume.
    //

    if ((TypeOfOpen <= UserVolumeOpen) &&
        !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ )) {

        //
        //  If there are open handles then fail this immediately.
        //

        if (Vcb->VcbCleanup != 0) {

            return STATUS_SHARING_VIOLATION;
        }

        //
        //  If we can't wait then force this to be posted.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

            CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
        }

        LockVolume = TRUE;

        //
        //  Purge the volume and make sure all of the user references
        //  are gone.
        //

        Status = CdPurgeVolume( IrpContext, Vcb, FALSE );

        if (Status != STATUS_SUCCESS) {

            return Status;
        }

        //
        //  Now force all of the delayed close operations to go away.
        //

        CdFspClose( Vcb );

        if (Vcb->VcbUserReference > CDFS_RESIDUAL_USER_REFERENCE) {

            return STATUS_SHARING_VIOLATION;
        }
    }
    
    //
    //  If the Fcb already existed then we need to check the oplocks and
    //  the share access.
    //

    if (Fcb->FcbCleanup != 0) {

        //
        //  If this is a user file open then check whether there are any
        //  batch oplock.
        //

        if (TypeOfOpen == UserFileOpen) {

            //
            //  Store the address of the Fcb for a possible teardown into
            //  the IrpContext.  We will release this in the call to
            //  prepost the Irp.
            //

            IrpContext->TeardownFcb = CurrentFcb;

            if (FsRtlCurrentBatchOplock( &Fcb->Oplock )) {

                //
                //  We remember if a batch oplock break is underway for the
                //  case where the sharing check fails.
                //

                Information = FILE_OPBATCH_BREAK_UNDERWAY;

                OplockStatus = FsRtlCheckOplock( &Fcb->Oplock,
                                                 IrpContext->Irp,
                                                 IrpContext,
                                                 CdOplockComplete,
                                                 CdPrePostIrp );

                if (OplockStatus == STATUS_PENDING) {

                    return STATUS_PENDING;
                }
            }

            //
            //  Check the share access before breaking any exclusive oplocks.
            //

            Status = IoCheckShareAccess( DesiredAccess,
                                         IrpSp->Parameters.Create.ShareAccess,
                                         IrpSp->FileObject,
                                         &Fcb->ShareAccess,
                                         FALSE );

            if (!NT_SUCCESS( Status )) {

                return Status;
            }

            //
            //  Now check that we can continue based on the oplock state of the
            //  file.
            //

            OplockStatus = FsRtlCheckOplock( &Fcb->Oplock,
                                             IrpContext->Irp,
                                             IrpContext,
                                             CdOplockComplete,
                                             CdPrePostIrp );

            if (OplockStatus == STATUS_PENDING) {

                return STATUS_PENDING;
            }

            IrpContext->TeardownFcb = NULL;

        //
        //  Otherwise just do the sharing check.
        //

        } else {

            Status = IoCheckShareAccess( DesiredAccess,
                                         IrpSp->Parameters.Create.ShareAccess,
                                         IrpSp->FileObject,
                                         &Fcb->ShareAccess,
                                         FALSE );

            if (!NT_SUCCESS( Status )) {

                return Status;
            }
        }
    }

    //
    //  Create the Ccb now.
    //

    Ccb = CdCreateCcb( IrpContext, Fcb, UserCcbFlags );

    //
    //  Update the share access.
    //

    if (Fcb->FcbCleanup == 0) {

        IoSetShareAccess( DesiredAccess,
                          IrpSp->Parameters.Create.ShareAccess,
                          IrpSp->FileObject,
                          &Fcb->ShareAccess );

    } else {

        IoUpdateShareAccess( IrpSp->FileObject, &Fcb->ShareAccess );
    }

    //
    //  Set the file object type.
    //

    CdSetFileObject( IrpContext, IrpSp->FileObject, TypeOfOpen, Fcb, Ccb );

    //
    //  Set the appropriate cache flags for a user file object.
    //

    if (TypeOfOpen == UserFileOpen) {

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING )) {

            SetFlag( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );

        } else {

            SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
        }
    }
    else if (TypeOfOpen == UserVolumeOpen)  {

        SetFlag( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );
    }

    //
    //  Update the open and cleanup counts.  Check the fast io state here.
    //

    CdLockVcb( IrpContext, Vcb );

    CdIncrementCleanupCounts( IrpContext, Fcb );
    CdIncrementReferenceCounts( IrpContext, Fcb, 1, 1 );

    if (LockVolume) {

        Vcb->VolumeLockFileObject = IrpSp->FileObject;
        SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
    }

    CdUnlockVcb( IrpContext, Vcb );

    CdLockFcb( IrpContext, Fcb );

    if (TypeOfOpen == UserFileOpen) {

        Fcb->IsFastIoPossible = CdIsFastIoPossible( Fcb );

    } else {

        Fcb->IsFastIoPossible = FastIoIsNotPossible;
    }

    CdUnlockFcb( IrpContext, Fcb );

    //
    //  Show that we opened the file.
    //

    IrpContext->Irp->IoStatus.Information = Information;

    //
    //  Point to the section object pointer in the non-paged Fcb.
    //

    IrpSp->FileObject->SectionObjectPointer = &Fcb->FcbNonpaged->SegmentObject;
    return OplockStatus;
}





