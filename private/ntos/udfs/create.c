/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Udfs called by the
    Fsd/Fsp dispatch routines.

Author:

    Dan Lovinger    [DanLo]     9-October-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_CREATE)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_CREATE)

//
//  Local support routines
//

NTSTATUS
UdfNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN PCCB RelatedCcb OPTIONAL,
    IN PUNICODE_STRING RelatedFileName OPTIONAL,
    IN OUT PUNICODE_STRING FileName,
    IN OUT PUNICODE_STRING RemainingName
    );

NTSTATUS
UdfOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB *CurrentFcb,
    IN PLCB OpenLcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN IgnoreCase,
    IN PCCB RelatedCcb OPTIONAL
    );

NTSTATUS
UdfOpenObjectByFileId (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb
    );

NTSTATUS
UdfOpenObjectFromDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN BOOLEAN ShortNameMatch,                             
    IN BOOLEAN IgnoreCase,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN PerformUserOpen,
    IN PCCB RelatedCcb OPTIONAL
    );

NTSTATUS
UdfCompleteFcbOpen (
    IN PIRP_CONTEXT IrpContext,
    PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PLCB OpenLcb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ULONG UserCcbFlags,
    IN ACCESS_MASK DesiredAccess
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCommonCreate)
#pragma alloc_text(PAGE, UdfCompleteFcbOpen)
#pragma alloc_text(PAGE, UdfNormalizeFileNames)
#pragma alloc_text(PAGE, UdfOpenObjectByFileId)
#pragma alloc_text(PAGE, UdfOpenExistingFcb)
#pragma alloc_text(PAGE, UdfOpenObjectFromDirContext)
#endif


NTSTATUS
UdfCommonCreate (
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
    If there is a related file object with a name then we will already have converted
    that name to Oem. 

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

    DIR_ENUM_CONTEXT DirContext;
    BOOLEAN CleanupDirContext = FALSE;

    BOOLEAN FoundEntry;

    PVCB Vcb;

    BOOLEAN OpenByFileId;
    BOOLEAN IgnoreCase;
    ULONG CreateDisposition;

    BOOLEAN ShortNameMatch;

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
    //  CurrentLcb - represents the name of the CurrentFcb.
    //

    TYPE_OF_OPEN RelatedTypeOfOpen = UnopenedFileObject;
    PFILE_OBJECT RelatedFileObject;
    PCCB RelatedCcb = NULL;

    PFCB NextFcb;
    PFCB CurrentFcb = NULL;

    PLCB CurrentLcb = NULL;

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

    PUNICODE_STRING FileName;
    PUNICODE_STRING RelatedFileName;

    UNICODE_STRING RemainingName;
    UNICODE_STRING FinalName;

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS.
    //

    if (IrpContext->Vcb == NULL) {

        UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
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

        UdfCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
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

        RelatedTypeOfOpen = UdfDecodeFileObject( RelatedFileObject, &NextFcb, &RelatedCcb );

        //
        //  Fail the request if this is not a user file object.
        //

        if (RelatedTypeOfOpen < UserVolumeOpen) {

            UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
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

    Status = UdfNormalizeFileNames( IrpContext,
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

        UdfCompleteRequest( IrpContext, Irp, Status );
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
        UdfAcquireVcbExclusive( IrpContext, Vcb, FALSE );

    } else {

        UdfAcquireVcbShared( IrpContext, Vcb, FALSE );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Verify that the Vcb is not in an unusable condition.  This routine
        //  will raise if not usable.
        //

        UdfVerifyVcb( IrpContext, Vcb );

        //
        //  If the Vcb is locked then we cannot open another file
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            try_leave( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If we are opening this file by FileId then process this immediately
        //  and exit.
        //

        if (OpenByFileId) {

            //
            //  The only create disposition we allow is OPEN.
            //

            if ((CreateDisposition != FILE_OPEN) &&
                (CreateDisposition != FILE_OPEN_IF)) {

                try_leave( Status = STATUS_ACCESS_DENIED );
            }

            try_leave( Status = UdfOpenObjectByFileId( IrpContext,
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

                try_leave( Status = STATUS_ACCESS_DENIED );
            }

	    //
	    //  If they wanted to open a directory, surprise.
	    //
	    
	    if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

		try_leave( Status = STATUS_NOT_A_DIRECTORY );
	    }

            //
            //  Acquire the Fcb first.
            //

            CurrentFcb = Vcb->VolumeDasdFcb;
            UdfAcquireFcbExclusive( IrpContext, CurrentFcb, FALSE );

            try_leave( Status = UdfOpenExistingFcb( IrpContext,
                                                    IrpSp,
                                                    &CurrentFcb,
                                                    NULL,
                                                    UserVolumeOpen,
                                                    FALSE,
                                                    NULL ));
        }

        //
        //  Acquire the Fcb at the beginning of our search to keep it from being
        //  deleted beneath us.
        //

        UdfAcquireFcbExclusive( IrpContext, NextFcb, FALSE );
        CurrentFcb = NextFcb;

        //
        //  Do a prefix search if there is more of the name to parse.
        //

        if (RemainingName.Length != 0) {

            //
            //  Do the prefix search to find the longest matching name.
            //

            CurrentLcb = UdfFindPrefix( IrpContext,
                                        &CurrentFcb,
                                        &RemainingName,
                                        IgnoreCase );
        }

        //
        //  At this point CurrentFcb points at the lowest Fcb in the tree for this
        //  file name, CurrentLcb is that name, and RemainingName is the rest of the
        //  name we have to do any directory traversals for.
        //

        //
        //  If the remaining name length is zero then we have found our
        //  target.
        //

        if (RemainingName.Length == 0) {

            //
            //  If this is a file so verify the user didn't want to open
            //  a directory.
            //

            if (SafeNodeType( CurrentFcb ) == UDFS_NTC_FCB_DATA) {

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH ) ||
                    FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

                    try_leave( Status = STATUS_NOT_A_DIRECTORY );
                }

                //
                //  The only create disposition we allow is OPEN.
                //

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    try_leave( Status = STATUS_ACCESS_DENIED );
                }

                try_leave( Status = UdfOpenExistingFcb( IrpContext,
                                                         IrpSp,
                                                         &CurrentFcb,
                                                         CurrentLcb,
                                                         UserFileOpen,
                                                         IgnoreCase,
                                                         RelatedCcb ));

            //
            //  This is a directory.  Verify the user didn't want to open
            //  as a file.
            //

            } else if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                try_leave( Status = STATUS_FILE_IS_A_DIRECTORY );

            //
            //  Open the file as a directory.
            //

            } else {

                //
                //  The only create disposition we allow is OPEN.
                //

                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF)) {

                    try_leave( Status = STATUS_ACCESS_DENIED );
                }

                try_leave( Status = UdfOpenExistingFcb( IrpContext,
                                                         IrpSp,
                                                         &CurrentFcb,
                                                         CurrentLcb,
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

            try_leave( Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  If we can't wait then post this request.
        //

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

            UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
        }

        //
        //  Prepare the enumeration context for use.
        //
        
        UdfInitializeDirContext( IrpContext, &DirContext );
        CleanupDirContext = TRUE;

        while (TRUE) {

            ShortNameMatch = FALSE;

            //
            //  Split off the next component from the name.
            //

            UdfDissectName( IrpContext,
                            &RemainingName,
                            &FinalName );

            //
            //  Go ahead and look this entry up in the directory.
            //

            FoundEntry = UdfFindDirEntry( IrpContext,
                                          CurrentFcb,
                                          &FinalName,
                                          IgnoreCase,
                                          FALSE,
                                          &DirContext );

            //
            //  If we didn't find the entry then check if the current name
            //  is a possible short name.
            //

            if (!FoundEntry && UdfCandidateShortName( IrpContext, &FinalName)) {

                //
                //  If the name looks like it could be a short name, try to find
                //  a matching real directory entry.
                //

                ShortNameMatch =
                FoundEntry = UdfFindDirEntry( IrpContext,
                                              CurrentFcb,
                                              &FinalName,
                                              IgnoreCase,
                                              TRUE,
                                              &DirContext );
            }

            //
            //  If we didn't find a match then check what the caller was trying to do to
            //  determine which error code to return.
            //
    
            if (!FoundEntry) {
    
                if ((CreateDisposition == FILE_OPEN) ||
                    (CreateDisposition == FILE_OVERWRITE)) {
    
                    try_leave( Status = STATUS_OBJECT_NAME_NOT_FOUND );
                }
    
                //
                //  Any other operation return STATUS_ACCESS_DENIED.
                //
    
                try_leave( Status = STATUS_ACCESS_DENIED );
            }

            //
            //  If this is an ignore case open then copy the exact case
            //  in the file object name.
            //

            if (IgnoreCase && !ShortNameMatch) {

                ASSERT( FinalName.Length == DirContext.ObjectName.Length );
                
                RtlCopyMemory( FinalName.Buffer,
                               DirContext.ObjectName.Buffer,
                               DirContext.ObjectName.Length );
            }

            //
            //  If we have found the last component then break out to open for the caller.
            //

            if (RemainingName.Length == 0) {

                break;
            }
            
            //
            //  The object we just found must be a directory.
            //

            if (!FlagOn( DirContext.Fid->Flags, NSR_FID_F_DIRECTORY )) {

                try_leave( Status = STATUS_OBJECT_PATH_NOT_FOUND );
            }

            //
            //  Now open an Fcb for this intermediate index Fcb.
            //

            UdfOpenObjectFromDirContext( IrpContext,
                                         IrpSp,
                                         Vcb,
                                         &CurrentFcb,
                                         ShortNameMatch,
                                         IgnoreCase,
                                         &DirContext,
                                         FALSE,
                                         NULL );
        }
        
        //
        //  Make sure our opener is about to get what they expect.
        //

        if ((FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TRAIL_BACKSLASH ) ||
             FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) &&
            !FlagOn( DirContext.Fid->Flags, NSR_FID_F_DIRECTORY )) {

            try_leave( Status = STATUS_NOT_A_DIRECTORY );
        
        }

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE ) &&
            FlagOn( DirContext.Fid->Flags, NSR_FID_F_DIRECTORY )) {

            try_leave( Status = STATUS_FILE_IS_A_DIRECTORY );
        }

        //
        //  The only create disposition we allow is OPEN.
        //

        if ((CreateDisposition != FILE_OPEN) &&
            (CreateDisposition != FILE_OPEN_IF)) {

            try_leave( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Open the object for the caller.
        //

        try_leave( Status = UdfOpenObjectFromDirContext( IrpContext,
                                                         IrpSp,
                                                         Vcb,
                                                         &CurrentFcb,
                                                         ShortNameMatch,
                                                         IgnoreCase,
                                                         &DirContext,
                                                         TRUE,
                                                         RelatedCcb ));
    } finally {
        
        //
        //  Cleanup the enumeration context if initialized.
        //

        if (CleanupDirContext) {

            UdfCleanupDirContext( IrpContext, &DirContext );
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

                UdfTeardownStructures( IrpContext, CurrentFcb, FALSE, &RemovedFcb );

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

            UdfReleaseFcb( IrpContext, CurrentFcb );
        }

        //
        //  Release the Vcb.
        //

        UdfReleaseVcb( IrpContext, Vcb );

        //
        //  Call our completion routine.  It will handle the case where either
        //  the Irp and/or IrpContext are gone.
        //

        UdfCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfNormalizeFileNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OpenByFileId,
    IN BOOLEAN IgnoreCase,
    IN TYPE_OF_OPEN RelatedTypeOfOpen,
    IN PCCB RelatedCcb OPTIONAL,
    IN PUNICODE_STRING RelatedFileName OPTIONAL,
    IN OUT PUNICODE_STRING FileName,
    IN OUT PUNICODE_STRING RemainingName
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
            //  Here is the  "M A R K   L U C O V S K Y"  hack.
            //
            //  It's here because Mark says he can't avoid sending me double beginning
            //  backslashes via the Win32 layer.
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

                RtlMoveMemory( FileName->Buffer,
                               FileName->Buffer + 1,
                               FileName->Length );

                FileName->Length -= sizeof( WCHAR );
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

                NewFileName.Buffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
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

                        ExFreePool( FileName->Buffer );
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

            RemainingName->MaximumLength =
            RemainingName->Length = (USHORT) RemainingNameLength;

            RemainingName->Buffer = Add2Ptr( FileName->Buffer,
                                             RelatedNameLength + SeparatorLength,
                                             PWCHAR );

            //
            //  Upcase the name if necessary.
            //

            if (IgnoreCase && (RemainingNameLength != 0)) {

                UdfUpcaseName( IrpContext,
                               RemainingName,
                               RemainingName );
            }

            //
            //  Do a quick check to make sure there are no wildcards.
            //

            if (FsRtlDoesNameContainWildCards( RemainingName )) {

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

        *RemainingName = *FileName;

        //
        //  Nothing to do if the name length is zero.
        //

        if (RemainingName->Length != 0) {

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

                    RemainingName->Buffer = Add2Ptr( RemainingName->Buffer,
                                                     RelatedFileName->Length,
                                                     PWCHAR );

                    RemainingName->Length -= RelatedFileName->Length;
                }
            }

            //
            //  If we are pointing at a separator character then step past that.
            //

            if (RemainingName->Length != 0) {

                if (*(RemainingName->Buffer) == L'\\') {

                    RemainingName->Buffer = Add2Ptr( RemainingName->Buffer,
                                                     sizeof( WCHAR ),
                                                     PWCHAR );

                    RemainingName->Length -= sizeof( WCHAR );
                }
            }
        }

        //
        //  Upcase the name if necessary.
        //

        if (IgnoreCase && (RemainingName->Length != 0)) {

            UdfUpcaseName( IrpContext,
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
UdfOpenObjectByFileId (
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

    If we don't find the Fcb then we take what is effectively a wild-a** guess.
    Since we would need more than 64bits to contain the root extent length along
    with the partition, lbn and dir/file flag we have to speculate that the
    opener knows what they are doing and try to crack an ICB hierarchy at the
    specified location.  This can fail for any number of reasons, which then have
    to be mapped to an open failure.
    
    If found then build the Fcb from this entry and store the new Fcb in the
    tree.

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
    BOOLEAN FcbExisted;

    ICB_SEARCH_CONTEXT IcbContext;
    BOOLEAN CleanupIcbContext = FALSE;

    NODE_TYPE_CODE NodeTypeCode;
    TYPE_OF_OPEN TypeOfOpen;

    FILE_ID FileId;

    PFCB NextFcb = NULL;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    //
    //  Extract the FileId from the FileObject.
    //

    RtlCopyMemory( &FileId, IrpSp->FileObject->FileName.Buffer, sizeof( FILE_ID ));

    //
    //  Now do a quick check that the reserved, unused chunk of the fileid is
    //  unused in this specimen.
    //

    if (UdfGetFidReservedZero( FileId )) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Go ahead and figure out the TypeOfOpen and NodeType.  We can
    //  get these from the input FileId.
    //

    if (UdfIsFidDirectory( FileId )) {

        TypeOfOpen = UserDirectoryOpen;
        NodeTypeCode = UDFS_NTC_FCB_INDEX;

    } else {

        TypeOfOpen = UserFileOpen;
        NodeTypeCode = UDFS_NTC_FCB_DATA;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Acquire the Vcb and check if there is already an Fcb.
        //  If not we will need to carefully hunt for the on-disc
        //  structures.
        //
        //  We will post the request if we don't find the Fcb and this
        //  request can't wait.
        //

        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        NextFcb = UdfCreateFcb( IrpContext, FileId, NodeTypeCode, &FcbExisted );

        //
        //  Now, if the Fcb was not already here we have some work to do.
        //
        
        if (!FcbExisted) {

            //
            //  If we can't wait then post this request.
            //
    
            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {
    
                UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }
    
            //
            //  Use a try-finally to transform errors we get as a result of going
            //  off on a wild goose chase into a simple open failure.
            //
            
            try {

                NextFcb->FileId = FileId;
                
                UdfInitializeIcbContextFromFcb( IrpContext, &IcbContext, NextFcb );
                CleanupIcbContext = TRUE;
    
                UdfLookupActiveIcb( IrpContext, &IcbContext );
                
                UdfInitializeFcbFromIcbContext( IrpContext,
                                                NextFcb,
                                                &IcbContext );
    
                UdfCleanupIcbContext( IrpContext, &IcbContext );
                CleanupIcbContext = FALSE;

            } except( UdfExceptionFilter( IrpContext, GetExceptionInformation() )) {

                //
                //   Any error we receive is an indication that the given fileid is
                //   not valid.
                //

                Status = STATUS_INVALID_PARAMETER;
            }

            //
            //  Do a little dance to leave the exception handler if we had problems.
            //
            
            if (Status == STATUS_INVALID_PARAMETER) {

                try_leave( NOTHING );
            }
        }
        
        //
        //  We have the Fcb.  Check that the type of the file is compatible with
        //  the desired type of file to open.
        //

        if (FlagOn( NextFcb->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

                try_leave( Status = STATUS_FILE_IS_A_DIRECTORY );
            }

        } else if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

            try_leave( Status = STATUS_NOT_A_DIRECTORY );
        }

        //
        //  We now know the Fcb and currently hold the Vcb lock.
        //  Try to acquire this Fcb without waiting.  Otherwise we
        //  need to reference it, drop the Vcb, acquire the Fcb, the
        //  Vcb and then dereference the Fcb.
        //

        if (!UdfAcquireFcbExclusive( IrpContext, NextFcb, TRUE )) {

            NextFcb->FcbReference += 1;
            UdfUnlockVcb( IrpContext, Vcb );

            UdfAcquireFcbExclusive( IrpContext, NextFcb, FALSE );

            UdfLockVcb( IrpContext, Vcb );
            NextFcb->FcbReference -= 1;
        }

        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;

        //
        //  Move to this Fcb.
        //

        *CurrentFcb = NextFcb;

        //
        //  Check the requested access on this Fcb.
        //

        if (!UdfIllegalFcbAccess( IrpContext,
                                  TypeOfOpen,
                                  IrpSp->Parameters.Create.SecurityContext->DesiredAccess )) {

            //
            //  Call our worker routine to complete the open.
            //

            Status = UdfCompleteFcbOpen( IrpContext,
                                         IrpSp,
                                         Vcb,
                                         CurrentFcb,
                                         NULL,
                                         TypeOfOpen,
                                         CCB_FLAG_OPEN_BY_ID,
                                         IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
        }

    } finally {

        if (UnlockVcb) {

            UdfUnlockVcb( IrpContext, Vcb );
        }

        if (CleanupIcbContext) {

            UdfCleanupIcbContext( IrpContext, &IcbContext );
        }
        
        //
        //  Destroy the new Fcb if it was not fully initialized.
        //

        if (NextFcb && !FlagOn( NextFcb->FcbState, FCB_STATE_INITIALIZED )) {

            UdfDeleteFcb( IrpContext, NextFcb );
        }

    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB *CurrentFcb,
    IN PLCB OpenLcb,
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
        
    OpenLcb - Lcb used to find this Fcb.

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
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_EXCLUSIVE_FCB( *CurrentFcb );
    ASSERT_OPTIONAL_CCB( RelatedCcb );

    //
    //  Check that the desired access is legal.
    //

    if (!UdfIllegalFcbAccess( IrpContext,
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

            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_ID | CCB_FLAG_OPEN_RELATIVE_BY_ID )) {

                SetFlag( CcbFlags, CCB_FLAG_OPEN_RELATIVE_BY_ID );
            }
        }

        //
        //  Call our worker routine to complete the open.
        //

        Status = UdfCompleteFcbOpen( IrpContext,
                                     IrpSp,
                                     (*CurrentFcb)->Vcb,
                                     CurrentFcb,
                                     OpenLcb,
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
UdfOpenObjectFromDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN BOOLEAN ShortNameMatch,                             
    IN BOOLEAN IgnoreCase,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN PerformUserOpen,
    IN PCCB RelatedCcb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to open an object found in a directory scan.  This
    can be a directory or a file as indicated in the scan's results.

    We first check that the desired access is legal for this file.  Then we
    construct the FileId for this and do a check to see if it is the Fcb
    Table.  It is always possible that either it was created since or simply
    wasn't in the prefix table at the time of the prefix table search.
    Lookup the active ICB, initialize the Fcb and store into the FcbTable
    if not present.

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
        
    ShortNameMatch - Indicates whether this object was opened by the shortname.

    IgnoreCase - Indicates the case sensitivity of the caller.

    DirContext - This is the context used to find the object.
    
    PerformUserOpen - Indicates if we are at the object the user wants to finally open.

    RelatedCcb - RelatedCcb for relative file object used to make this open.

Return Value:

    NTSTATUS - Status indicating the result of the operation.

--*/

{
    ULONG CcbFlags = 0;
    FILE_ID FileId;

    BOOLEAN UnlockVcb = FALSE;
    BOOLEAN FcbExisted;

    PFCB NextFcb = NULL;
    PFCB ParentFcb = NULL;

    TYPE_OF_OPEN TypeOfOpen;
    NODE_TYPE_CODE NodeTypeCode;

    ICB_SEARCH_CONTEXT IcbContext;
    BOOLEAN CleanupIcbContext = FALSE;

    PLCB OpenLcb;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Figure out what kind of open we will be performing here.  The caller has already insured
    //  that the user is expecting us to do this.
    //

    if (FlagOn( DirContext->Fid->Flags, NSR_FID_F_DIRECTORY )) {

        TypeOfOpen = UserDirectoryOpen;
        NodeTypeCode = UDFS_NTC_FCB_INDEX;
    
    } else {
        
        TypeOfOpen = UserFileOpen;
        NodeTypeCode = UDFS_NTC_FCB_DATA;
    }

    //
    //  Check for illegal access to this file.
    //

    if (PerformUserOpen &&
        UdfIllegalFcbAccess( IrpContext,
                             TypeOfOpen,
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

            SetFlag( CcbFlags, CCB_FLAG_OPEN_RELATIVE_BY_ID );
        }

        if (IgnoreCase) {

            SetFlag( CcbFlags, CCB_FLAG_IGNORE_CASE );
        }

        //
        //  Build the file Id for this object.
        //
        
        UdfSetFidFromLbAddr( FileId, DirContext->Fid->Icb.Start );

        if (TypeOfOpen == UserDirectoryOpen) {

            UdfSetFidDirectory( FileId );
        }

        //
        //  Lock the Vcb so we can examine the Fcb Table.
        //

        UdfLockVcb( IrpContext, Vcb );
        UnlockVcb = TRUE;

        //
        //  Get the Fcb for this file.
        //

        NextFcb = UdfCreateFcb( IrpContext, FileId, NodeTypeCode, &FcbExisted );

        //
        //  If the Fcb was created here then initialize from the values in the
        //  dirent.  We have optimistically assumed that there isn't any corrupt
        //  information to this point - we're about to discover it if there is.
        //

        if (!FcbExisted) {

            //
            //  Set the root extent length and go get the active ICB, initialize.
            //

            NextFcb->RootExtentLength = DirContext->Fid->Icb.Length.Length;

            UdfInitializeIcbContextFromFcb( IrpContext, &IcbContext, NextFcb );
            CleanupIcbContext = TRUE;

            UdfLookupActiveIcb( IrpContext, &IcbContext );
            
            UdfInitializeFcbFromIcbContext( IrpContext,
                                            NextFcb,
                                            &IcbContext );

            UdfCleanupIcbContext( IrpContext, &IcbContext );
            CleanupIcbContext = FALSE;

        }

        //
        //  Now try to acquire the new Fcb without waiting.  We will reference
        //  the Fcb and retry with wait if unsuccessful.
        //

        if (!UdfAcquireFcbExclusive( IrpContext, NextFcb, TRUE )) {

            NextFcb->FcbReference += 1;

            UdfUnlockVcb( IrpContext, Vcb );

            UdfReleaseFcb( IrpContext, *CurrentFcb );
            UdfAcquireFcbExclusive( IrpContext, NextFcb, FALSE );
            UdfAcquireFcbExclusive( IrpContext, *CurrentFcb, FALSE );

            UdfLockVcb( IrpContext, Vcb );
            NextFcb->FcbReference -= 1;
        }

        //
        //  Move down to this new Fcb.  Remember that we still own the parent however.
        //

        ParentFcb = *CurrentFcb;
        *CurrentFcb = NextFcb;

        //
        //  Store this name into the prefix table for the parent.
        //

        OpenLcb = UdfInsertPrefix( IrpContext,
                                   NextFcb,
                                   ( ShortNameMatch?
                                     &DirContext->ShortObjectName :
                                     &DirContext->CaseObjectName ),
                                   ShortNameMatch,
                                   IgnoreCase,
                                   ParentFcb );

        //
        //  Now increment the reference counts for the parent and drop the Vcb.
        //

        DebugTrace(( +1, Dbg,
                     "UdfOpenObjectFromDirContext, PFcb %08x Vcb %d/%d Fcb %d/%d\n", ParentFcb,
                     Vcb->VcbReference,
                     Vcb->VcbUserReference,
                     ParentFcb->FcbReference,
                     ParentFcb->FcbUserReference ));

        UdfIncrementReferenceCounts( IrpContext, ParentFcb, 1, 1 );
        
        DebugTrace(( -1, Dbg, 
                     "UdfOpenObjectFromDirContext, Vcb %d/%d Fcb %d/%d\n",
                     Vcb->VcbReference,
                     Vcb->VcbUserReference,
                     ParentFcb->FcbReference,
                     ParentFcb->FcbUserReference ));

        UdfUnlockVcb( IrpContext, Vcb );
        UnlockVcb = FALSE;

        //
        //  Perform initialization associated with the directory context.
        //
            
        UdfInitializeLcbFromDirContext( IrpContext,
                                        OpenLcb,
                                        DirContext );

        //
        //  Release the parent Fcb at this point.
        //

        UdfReleaseFcb( IrpContext, ParentFcb );
        ParentFcb = NULL;

        //
        //  Call our worker routine to complete the open.
        //

        if (PerformUserOpen) {

            Status = UdfCompleteFcbOpen( IrpContext,
                                         IrpSp,
                                         Vcb,
                                         CurrentFcb,
                                         OpenLcb,
                                         TypeOfOpen,
                                         CcbFlags,
                                         IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
        }

    } finally {

        //
        //  Unlock the Vcb if held.
        //

        if (UnlockVcb) {

            UdfUnlockVcb( IrpContext, Vcb );
        }

        //
        //  Release the parent if held.
        //

        if (ParentFcb != NULL) {

            UdfReleaseFcb( IrpContext, ParentFcb );
        }

        //
        //  Destroy the new Fcb if it was not fully initialized.
        //

        if (NextFcb && !FlagOn( NextFcb->FcbState, FCB_STATE_INITIALIZED )) {

            UdfDeleteFcb( IrpContext, NextFcb );
        }

        //
        //  Clean up the Icb context if used.
        //

        if (CleanupIcbContext) {

            UdfCleanupIcbContext( IrpContext, &IcbContext );
        }
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
UdfCompleteFcbOpen (
    IN PIRP_CONTEXT IrpContext,
    PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN PLCB OpenLcb OPTIONAL,
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
        
    OpenLcb - Lcb this Fcb is being opened by

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
    NTSTATUS OplockStatus = STATUS_SUCCESS;
    ULONG Information = FILE_OPENED;

    BOOLEAN LockVolume = FALSE;

    PFCB Fcb = *CurrentFcb;
    PCCB Ccb;

    PAGED_CODE();

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

            UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
        }

        LockVolume = TRUE;

        //
        //  Purge the volume and make sure all of the user references
        //  are gone.
        //

        Status = UdfPurgeVolume( IrpContext, Vcb, FALSE );

        if (Status != STATUS_SUCCESS) {

            return Status;
        }

        //
        //  Now force all of the delayed close operations to go away.
        //

        UdfFspClose( Vcb );

        if (Vcb->VcbUserReference > Vcb->VcbResidualUserReference) {

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
                                                 UdfOplockComplete,
                                                 UdfPrePostIrp );

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
                                             UdfOplockComplete,
                                             UdfPrePostIrp );

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

    Ccb = UdfCreateCcb( IrpContext, Fcb, OpenLcb, UserCcbFlags );

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

    UdfSetFileObject( IrpContext, IrpSp->FileObject, TypeOfOpen, Fcb, Ccb );

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
    //
    //  Update the open and cleanup counts.  Check the fast io state here.
    //

    UdfLockVcb( IrpContext, Vcb );

    UdfIncrementCleanupCounts( IrpContext, Fcb );
    
    DebugTrace(( +1, Dbg,
                 "UdfCompleteFcbOpen, Fcb %08x Vcb %d/%d Fcb %d/%d\n", Fcb,
                 Vcb->VcbReference,
                 Vcb->VcbUserReference,
                 Fcb->FcbReference,
                 Fcb->FcbUserReference ));

    UdfIncrementReferenceCounts( IrpContext, Fcb, 1, 1 );
    
    DebugTrace(( -1, Dbg,
                 "UdfCompleteFcbOpen, Vcb %d/%d Fcb %d/%d\n",
                 Vcb->VcbReference,
                 Vcb->VcbUserReference,
                 Fcb->FcbReference,
                 Fcb->FcbUserReference ));

    if (LockVolume) {

        Vcb->VolumeLockFileObject = IrpSp->FileObject;
        SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
    }

    UdfUnlockVcb( IrpContext, Vcb );

    UdfLockFcb( IrpContext, Fcb );

    if (TypeOfOpen == UserFileOpen) {

        Fcb->IsFastIoPossible = UdfIsFastIoPossible( Fcb );

    } else {

        Fcb->IsFastIoPossible = FastIoIsNotPossible;
    }

    UdfUnlockFcb( IrpContext, Fcb );

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

