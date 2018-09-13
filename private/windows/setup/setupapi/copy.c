/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    copy.c

Abstract:

    High-level file copy/installation functions

Author:

    Ted Miller (tedm) 14-Feb-1995

Revision History:

--*/

#include "precomp.h"

#pragma hdrstop

#include <winioctl.h>

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );


//
// Mask for all copy flags that will require us to determine
// version information.
//
#define SP_COPY_MASK_NEEDVERINFO    (SP_COPY_NEWER_OR_SAME | SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER | SP_COPY_LANGUAGEAWARE)


VOID
pGetVersionText(
   OUT PTSTR VersionText,
   IN DWORDLONG Version
   )
/*++

Routine Description:

    Convert a 64-bit version number into either
    n.n.n.n  or "0"

Arguments:

    VersionText - buffer, big enough to hold 4x16 bit numbers
    Version - 64-bit version, or 0 if no version

Return Value:

    none

--*/
{
    if (Version == 0) {
        lstrcpy(VersionText,TEXT("0"));
    } else {
        int m1 = (int)((Version >> 48) & 0xffff);
        int m2 = (int)((Version >> 32) & 0xffff);
        int m3 = (int)((Version >> 16) & 0xffff);
        int m4 = (int)(Version & 0xffff);

        wsprintf(VersionText,TEXT("%d.%d.%d.%d"),m1,m2,m3,m4);
    }
}

DWORD
CreateTargetAsLinkToMaster(
   IN PSP_FILE_QUEUE Queue,
   IN PCTSTR FullSourceFilename,
   IN PCTSTR FullTargetFilename,
   IN PVOID CopyMsgHandler OPTIONAL,
   IN PVOID Context OPTIONAL,
   IN BOOL IsMsgHandlerNativeCharWidth
   )
{
#ifdef ANSI_SETUPAPI
    return ERROR_CALL_NOT_IMPLEMENTED;
#else

    PTSTR p;
    TCHAR c;
    DWORD bytesReturned;
    DWORD error;
    BOOL ok;
    DWORD sourceLength;
    DWORD targetLength;
    DWORD sourceDosDevLength;
    DWORD targetDosDevLength;
    DWORD copyFileSize;
    PSI_COPYFILE copyFile;
    PCHAR s;
    HANDLE targetHandle;

    //
    // Get the name of the source directory.
    //
    p = _tcsrchr( FullSourceFilename, TEXT('\\') );
    if ( (p == NULL) || (p == FullSourceFilename) ) {
        return ERROR_FILE_NOT_FOUND;    // copy by usual means
    }
    if ( *(p-1) == TEXT(':') ) {
        p++;
    }
    c = *p;
    *p = 0;

    //
    // If this is the same as the previous source directory, then we already
    // have a handle to the directory; otherwise, close the old handle and
    // open a handle to this directory.
    //
    if ( (Queue->SisSourceDirectory == NULL) ||
         (_tcsicmp(FullSourceFilename, Queue->SisSourceDirectory) != 0) ) {

        if ( Queue->SisSourceHandle != INVALID_HANDLE_VALUE ) {
            CloseHandle( Queue->SisSourceHandle );
        }
        Queue->SisSourceHandle = CreateFile(
                                    FullSourceFilename,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL
                                    );
        if ( Queue->SisSourceHandle == INVALID_HANDLE_VALUE ) {
            return ERROR_FILE_NOT_FOUND;
        }
        if ( Queue->SisSourceDirectory != NULL ) {
            MyFree( Queue->SisSourceDirectory );
        }
        Queue->SisSourceDirectory = DuplicateString( FullSourceFilename );

        //
        // If the DuplicateString fails, we press on. Because SisSourceDirectory
        // is NULL, we'll reopen the source directory next time.
        //
    }

    *p = c;

    //
    // Build the FSCTL command buffer.
    //

    sourceLength = (_tcslen(FullSourceFilename) + 1) * sizeof(TCHAR);
    if ( *FullSourceFilename != TEXT('\\') ) {
        sourceDosDevLength = _tcslen(TEXT("\\??\\")) * sizeof(TCHAR);
    } else {
        sourceDosDevLength = 0;
    }
    targetLength = (_tcslen(FullTargetFilename) + 1) * sizeof(TCHAR);
    if ( *FullTargetFilename != TEXT('\\') ) {
        targetDosDevLength = _tcslen(TEXT("\\??\\")) * sizeof(TCHAR);
    } else {
        targetDosDevLength = 0;
    }

    copyFileSize = FIELD_OFFSET(SI_COPYFILE, FileNameBuffer) +
                    sourceDosDevLength + sourceLength +
                    targetDosDevLength + targetLength;

    copyFile = MyMalloc( copyFileSize );
    if ( copyFile == NULL ) {
        return ERROR_FILE_NOT_FOUND;
    }

    copyFile->SourceFileNameLength = sourceDosDevLength + sourceLength;
    copyFile->DestinationFileNameLength = targetDosDevLength + targetLength;
    copyFile->Flags = COPYFILE_SIS_REPLACE;

    s = (PCHAR)copyFile->FileNameBuffer;
    if ( sourceDosDevLength != 0 ) {
        RtlCopyMemory(
            s,
            TEXT("\\??\\"),
            sourceDosDevLength
            );
        s += sourceDosDevLength;
    }
    RtlCopyMemory(
        s,
        FullSourceFilename,
        sourceLength
        );
    s += sourceLength;

    if ( targetDosDevLength != 0 ) {
        RtlCopyMemory(
            s,
            TEXT("\\??\\"),
            targetDosDevLength
            );
        s += targetDosDevLength;
    }
    RtlCopyMemory(
        s,
        FullTargetFilename,
        targetLength
        );

    //
    // Invoke the SIS CopyFile FsCtrl.
    //

    ok = DeviceIoControl(
            Queue->SisSourceHandle,
            FSCTL_SIS_COPYFILE,
            copyFile,               // Input buffer
            copyFileSize,           // Input buffer length
            NULL,                   // Output buffer
            0,                      // Output buffer length
            &bytesReturned,
            NULL
            );
    error = GetLastError( );

    MyFree( copyFile );

    if ( ok ) {

        //DbgPrint( "\n\nCreateTargetAsLinkToMaster: SIS copy %ws->%ws succeeded\n\n\n", FullSourceFilename, FullTargetFilename );

        //
        // Open the target file so that CSC knows about it and pins it,
        // if necessary.
        //

        targetHandle = CreateFile(
                            FullTargetFilename,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );
        if ( targetHandle == INVALID_HANDLE_VALUE ) {
            error = GetLastError();
            DbgPrint( "\n\nCreateTargetAsLinkToMaster: SIS copy %ws->%ws succeeded, but open failed: %d\n\n\n", FullSourceFilename, FullTargetFilename, error );
        } else {
            CloseHandle( targetHandle );
        }

        error = NO_ERROR;

    } else {

        //DbgPrint( "\n\nCreateTargetAsLinkToMaster: SIS copy %ws->%ws failed: %d\n\n\n", FullSourceFilename, FullTargetFilename, error );

        //
        // If it looks like SIS isn't active on the remote file system, close
        // the SIS root handle so that we can avoid repeatedly getting this
        // error.
        //
        // Note: NTFS returns STATUS_INVALID_PARAMETER (ERROR_INVALID_PARAMETER).
        // FAT returns STATUS_INVALID_DEVICE_REQUEST (ERROR_INVALID_FUNCTION).
        //

        if ( (error == ERROR_INVALID_PARAMETER) ||
             (error == ERROR_INVALID_FUNCTION) ) {
            CloseHandle( Queue->SisSourceHandle );
            Queue->SisSourceHandle = INVALID_HANDLE_VALUE;
            if ( Queue->SisSourceDirectory != NULL ) {
                MyFree( Queue->SisSourceDirectory );
                Queue->SisSourceDirectory = NULL;
            }
            Queue->Flags &= ~FQF_TRY_SIS_COPY;
        }
    }

    return error;

#endif
}


BOOL
_SetupInstallFileEx(
    IN  PSP_FILE_QUEUE      Queue,             OPTIONAL
    IN  PSP_FILE_QUEUE_NODE QueueNode,         OPTIONAL
    IN  HINF                InfHandle,         OPTIONAL
    IN  PINFCONTEXT         InfContext,        OPTIONAL
    IN  PCTSTR              SourceFile,        OPTIONAL
    IN  PCTSTR              SourcePathRoot,    OPTIONAL
    IN  PCTSTR              DestinationName,   OPTIONAL
    IN  DWORD               CopyStyle,
    IN  PVOID               CopyMsgHandler,    OPTIONAL
    IN  PVOID               Context,           OPTIONAL
    OUT PBOOL               FileWasInUse,
    IN  BOOL                IsMsgHandlerNativeCharWidth,
    OUT PBOOL               SignatureVerifyFailed
    )

/*++

Routine Description:

    Actual implementation of SetupInstallFileEx. Handles either ANSI or
    Unicode callback routine.

Arguments:

    Same as SetupInstallFileEx().

    QueueNode - must be specified if Queue is supplied.  This parameter gives
        us the queue node for this operation so we can get at the pertinent
        catalog info for driver signing.

    IsMsgHandlerNativeCharWidth - supplies a flag indicating whether
        CopyMsgHandler expects native char widths args (or ansi ones, in the
        unicode build of the dll).

    SignatureVerifyFailed - supplies the address of a boolean variable that is
        set to indicate whether or not digital signature verification failed for
        the source file.  This will be set to FALSE if some other failure caused
        us to abort prior to attempting the signature verification.  This is
        used by the queue commit routines to determine whether or not the queue
        callback routine should be given a chance to handle a copy failure
        (skip, retry, etc.).  Digital signature verification failures are
        handled within this routine (including user prompting, if policy
        requires it), and queue callback routines _are not_ allowed to override
        the behavior.

Return Value:

    Same as SetupInstallFileEx().

--*/

{
    BOOL b;
    BOOL Ok;
    DWORD rc = NO_ERROR;
    DWORD SigVerifRc;
    UINT SourceId;
    TCHAR Buffer1[MAX_PATH];
    TCHAR Buffer2[MAX_PATH];
    PCTSTR FullSourceFilename;
    PCTSTR FullTargetFilename;
    PCTSTR SourceFilenamePart;
    PTSTR ActualSourceFilename;
    PTSTR TemporaryTargetFile;
    UINT CompressionType;
    DWORD SourceFileSize;
    DWORD TargetFileSize;
    PTSTR p;
    DWORDLONG SourceVersion, TargetVersion;
    TCHAR SourceVersionText[50], TargetVersionText[50];
    LANGID SourceLanguage;
    LANGID TargetLanguage;
    WIN32_FIND_DATA SourceFindData;
    UINT NotifyFlags;
    PSECURITY_DESCRIPTOR SecurityInfo;
    FILEPATHS FilePaths;
    UINT param;
    FILETIME sFileTime,tFileTime;
    WORD sDosTime,sDosDate,tDosTime,tDosDate;
    BOOL Moved;
    SetupapiVerifyProblem Problem;
    BOOL ExistingTargetFileWasSigned;
    PSETUP_LOG_CONTEXT lc = NULL;
    DWORD slot_fileop = 0;
    SP_TARGET_ENT TargetInfo;
    PCTSTR ExistingFile = NULL;
    PLOADED_INF LoadedInf = NULL;
    DWORD ExemptCopyFlags = 0;

    if (Queue) {
        lc = Queue->LogContext;
    } else if (InfHandle && InfHandle != INVALID_HANDLE_VALUE) {
        //
        // Lock INF for the duration of this routine.
        //
        try {
            if(!LockInf((PLOADED_INF)InfHandle)) {
                rc = ERROR_INVALID_HANDLE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // Assume InfHandle was bad pointer
            //
            rc = ERROR_INVALID_HANDLE;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return FALSE;
        }

        LoadedInf = (PLOADED_INF)InfHandle;

        lc = LoadedInf->LogContext;
    }

    //
    // If Queue is specified, then so must QueueNode (and vice versa).
    //
    MYASSERT((Queue && QueueNode) || !(Queue || QueueNode));

    *SignatureVerifyFailed = FALSE;
    SigVerifRc = NO_ERROR;

    //
    // Assume failure.
    //
    Ok = FALSE;
    SecurityInfo = NULL;
    Moved = FALSE;
    try {
        *FileWasInUse = FALSE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    if((rc == NO_ERROR) && InfContext) {
        if(!InfHandle || (InfHandle == INVALID_HANDLE_VALUE)) {
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    if(rc != NO_ERROR) {
        goto clean0;
    }

    //
    // Determine the full source path and filename of the file.
    //
    if(CopyStyle & SP_COPY_SOURCE_ABSOLUTE) {
        FullSourceFilename = DuplicateString(SourceFile);
    } else {

        //
        // Get the relative path for this file if necessary.
        //
        if(CopyStyle & SP_COPY_SOURCEPATH_ABSOLUTE) {
            Buffer2[0] = 0;
            b = TRUE;
        } else {
            b = SetupGetSourceFileLocation(
                    InfHandle,
                    InfContext,
                    SourceFile,
                    &SourceId,
                    Buffer2,
                    MAX_PATH,
                    NULL
                    );
        }

        //
        // Concatenate the relative path and the filename to the source root.
        //
        if(!b) {
            rc = (GetLastError() == ERROR_INSUFFICIENT_BUFFER
               ? ERROR_FILENAME_EXCED_RANGE : GetLastError());
            goto clean0;
        }

        lstrcpyn(Buffer1,SourcePathRoot,MAX_PATH);

        if(!ConcatenatePaths(Buffer1,Buffer2,MAX_PATH,NULL)
        || !ConcatenatePaths(Buffer1,SourceFile,MAX_PATH,NULL)) {
            rc = ERROR_FILENAME_EXCED_RANGE;
            goto clean0;
        }

        FullSourceFilename = DuplicateString(Buffer1);
    }

    if(!FullSourceFilename) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    SourceFilenamePart = MyGetFileTitle(FullSourceFilename);

    //
    // Determine the full target path and filename of the file.
    // For now ignore the issues regarding compressed vs. uncompressed names.
    //
    if(InfContext) {
        //
        // DestinationName is the filename only (no path) of the target.
        // We'll need to fetch the target path information for the section
        // that InfContext references.
        //
        b = SetupGetTargetPath(
                InfHandle,
                InfContext,
                NULL,
                Buffer1,
                MAX_PATH,
                NULL
                );

        if(!b) {
            rc = (GetLastError() == ERROR_INSUFFICIENT_BUFFER
               ? ERROR_FILENAME_EXCED_RANGE : GetLastError());
            goto clean1;
        }

        lstrcpyn(Buffer2,Buffer1,MAX_PATH);

        b = ConcatenatePaths(
                Buffer2,
                DestinationName ? DestinationName : SourceFilenamePart,
                MAX_PATH,
                NULL
                );

        if(!b) {
            rc = ERROR_FILENAME_EXCED_RANGE;
            goto clean1;
        }

        FullTargetFilename = DuplicateString(Buffer2);
    } else {
        //
        // DestinationName is the full path and filename of the target file.
        //
        FullTargetFilename = DuplicateString(DestinationName);
    }

    if(!FullTargetFilename) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    //
    // Log the file copy - only if we log something else
    //
    slot_fileop = AllocLogInfoSlot(lc,FALSE);   // for conditional display of extra logging info
    WriteLogEntry(
        lc,
        slot_fileop,
        MSG_LOG_COPYING_FILE,
        NULL,
        FullSourceFilename,
        FullTargetFilename);

    //
    // Make sure the target path exists.
    //
    rc = pSetupMakeSurePathExists(FullTargetFilename);
    if(rc != NO_ERROR) {
        goto clean2;
    }

    //
    // Determine if the source file is compressed and get compression type
    // if so.
    //
    rc = SetupInternalGetFileCompressionInfo(
            FullSourceFilename,
            &ActualSourceFilename,
            &SourceFindData,
            &TargetFileSize,
            &CompressionType
            );

    //
    // If the source doesn't exist but the target does, and we don't want to
    // overwrite it, then there is no error and we're finished.
    //
    // BUGBUG (lonnym)--when doing a driver uninstall (i.e., re-installing the
    // previous driver from the backup directory), it's possible that not all
    // source files will be present in that directory (i.e., only those files
    // that were modified got backed up).  In that case, we want to consider a
    // source file-not-found error here to be OK, even if the force-nooverwrite
    // flag isn't set.
    //
    // Note that driver signing isn't relevant here, because if an INF was signed
    // with the force-nooverwrite flag, then the signer (i.e., WHQL) must've been
    // satisfied that the file in question was not crucial to the package's
    // integrity/operation (a default INI file would be an example of this).
    //
    if(rc == ERROR_FILE_NOT_FOUND &&
        CopyStyle & SP_COPY_FORCE_NOOVERWRITE &&
        FileExists(FullTargetFilename,NULL)
        ) {

        rc = NO_ERROR;
        goto clean2;

    } else if(rc != NO_ERROR) {
        goto clean2;
    }

    //
    // Got the actual source file name now.
    //
    MyFree(FullSourceFilename);
    FullSourceFilename = ActualSourceFilename;
    SourceFilenamePart = MyGetFileTitle(FullSourceFilename);

    //
    // If the file to be copied is a .CAB and the source and destination
    // filenames are the same, then we don't want to attempt to decompress it
    // (because if we do, we'd just be pulling the first file out of the cab
    // and renaming it to the destination filename, which is never the desired
    // behavior.
    //
    if(!lstrcmpi(SourceFilenamePart, MyGetFileTitle(FullTargetFilename))) {
        p = _tcsrchr(SourceFilenamePart, TEXT('.'));
        if(p && !lstrcmpi(p, TEXT(".CAB"))) {
            CopyStyle |= SP_COPY_NODECOMP;
        }
    }

    //
    // If the no-decomp flag is set, adjust the target filename so that
    // the filename part is the same as the actual name of the source.
    // We do this regardless of whether the source file is compressed.
    //
    // Note:  For driver signing, the fact that this file is installed in its
    // compressed form means we must have an entry for the compressed file in
    // the catalog.  However, if at some point in the future this file is going
    // to be expanded (as is typically the case), then we need to have the
    // expanded file's signature in the catalog as well, so that sigverif
    // doesn't consider this expanded file to be from a non-certified package.
    //
    if(CopyStyle & SP_COPY_NODECOMP) {
        //
        // Strip out version-related bits and ensure that we treat the file
        // as uncompressed.
        //
        CopyStyle &= ~SP_COPY_MASK_NEEDVERINFO;
        CompressionType = FILE_COMPRESSION_NONE;

        //
        // Isolate the path part of the target filename.
        //
        lstrcpyn(Buffer1, FullTargetFilename, MAX_PATH);
        *((PTSTR)MyGetFileTitle(Buffer1)) = TEXT('\0');

        //
        // Concatenate the source filename onto the target pathname.
        //
        if(!ConcatenatePaths(Buffer1,SourceFilenamePart,MAX_PATH,NULL)) {
            rc = ERROR_FILENAME_EXCED_RANGE;
            goto clean2;
        }

        p = DuplicateString(Buffer1);
        if(!p) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto clean2;
        }

        MyFree(FullTargetFilename);
        FullTargetFilename = p;
    }

    //
    // See if the target file exists, either as a renamed file (i.e., because
    // we're replacing a boot file), or as a file presently existing at the
    // target location.
    //
    if(Queue && (CopyStyle & SP_COPY_REPLACE_BOOT_FILE)) {
        //
        // First, we need to find the corresponding target info node so
        // we can find out what temporary name our file was renamed to.
        //
        rc = pSetupBackupGetTargetByPath((HSPFILEQ)Queue,
                                         NULL, // use Queue's string table
                                         FullTargetFilename,
                                         QueueNode->TargetDirectory,
                                         -1,
                                         QueueNode->TargetFilename,
                                         NULL,
                                         &TargetInfo
                                        );
        if(rc == NO_ERROR) {
            //
            // Has the file previously been renamed (and not yet restored)?
            //
            if((TargetInfo.InternalFlags & (SP_TEFLG_MOVED | SP_TEFLG_RESTORED)) == SP_TEFLG_MOVED) {

                ExistingFile = StringTableStringFromId(
                                    Queue->StringTable,
                                    TargetInfo.NewTargetFilename
                                   );
                MYASSERT(ExistingFile);
            }
        }
    }

    if(!ExistingFile && FileExists(FullTargetFilename, NULL)) {
        ExistingFile = FullTargetFilename;
    }

    if(ExistingFile) {

        if(CopyStyle & SP_COPY_FORCE_NOOVERWRITE) {
            //
            // No overwrite and no callback notification either
            //
            // Note that driver signing isn't relevant here, because if an INF
            // was signed with the force-nooverwrite flag, then the signer
            // (i.e., WHQL) must've been satisfied that the file in question was
            // not crucial to the package's integrity/operation (a default INI
            // file would be an example of this).
            //
            rc = NO_ERROR;
            goto clean2;
        }

        if(CopyStyle & SP_COPY_MASK_NEEDVERINFO) {
            if(!GetVersionInfoFromImage(ExistingFile, &TargetVersion, &TargetLanguage)) {
                TargetVersion = 0;
                TargetLanguage = 0;
            }
        }

        //
        // If the target file exists we'll want to preserve security info on it.
        //
        if(RetreiveFileSecurity(ExistingFile, &SecurityInfo) != NO_ERROR) {
            SecurityInfo = NULL;
        }

    } else {

        if(CopyStyle & SP_COPY_REPLACEONLY) {
            //
            // Target file doesn't exist, so there's nothing to do.
            //
            rc = NO_ERROR;
            goto clean2;
        }
    }

    //
    // If the source is not compressed (LZ or cabinet), and SIS is (or might be)
    // present, create the target as an SIS link to the master instead of copying it.
    //
    // If the target exists, and NOOVERWRITE was specified, don't try to create
    // an SIS link. Instead, fall through to the normal copy code. The overwrite
    // semantics are wrong if the file already exists.
    //
    if((CompressionType == FILE_COMPRESSION_NONE) &&
       (!ExistingFile || ((CopyStyle & SP_COPY_NOOVERWRITE) == 0)) &&
       (Queue != NULL) &&
       ((Queue->Flags & FQF_TRY_SIS_COPY) != 0)) {

        //
        // First, verify that the sourcefile is signed.  If it is not, but the
        // user elects to proceed with the copy (or if the policy is 'ignore')
        // then we'll go ahead and attempt to setup the SIS link.
        //
        rc = VerifySourceFile(lc,
                              Queue,
                              QueueNode,
                              MyGetFileTitle(FullTargetFilename),
                              FullSourceFilename,
                              NULL,
                              ((Queue->Flags & FQF_USE_ALT_PLATFORM)
                                  ? &(Queue->AltPlatformInfo)
                                  : NULL),
                              TRUE,
                              &Problem,
                              Buffer1
                             );

        if(rc != NO_ERROR) {
            *SignatureVerifyFailed = TRUE;
            SigVerifRc = rc;
            if (Queue->Flags & FQF_QUEUE_FORCE_BLOCK_POLICY) {
                goto clean2;
            } else {
                //
                // If this is a device installation and the policy is "Ignore",
                // then crank it up to "Warn" if the file is under SFP's
                // protection.  This will allow the user to update a driver that
                // ships in our box for which no WHQL certification program
                // exists.
                //
                if((Queue->Flags & FQF_DEVICE_INSTALL) && 
                   (Queue->DriverSigningPolicy == DRIVERSIGN_NONE) &&
                   IsFileProtected(FullTargetFilename, lc, NULL)) {

                    Queue->DriverSigningPolicy = DRIVERSIGN_WARNING;

                    //
                    // Log the fact that policy was elevated.
                    //
                    WriteLogEntry(lc,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_POLICY_ELEVATED_FOR_SFC,
                                  NULL
                                 );
                }

                if(!HandleFailedVerification(Queue->hWndDriverSigningUi,
                                             Problem,
                                             Buffer1,
                                             ((Queue->DeviceDescStringId == -1)
                                                 ? NULL
                                                 : pStringTableStringFromId(
                                                       Queue->StringTable,
                                                       Queue->DeviceDescStringId)),
                                             Queue->DriverSigningPolicy,
                                             (Queue->Flags & FQF_DIGSIG_ERRORS_NOUI),
                                             rc,
                                             lc,
                                             &ExemptCopyFlags,
                                             FullTargetFilename))
                {
                    //
                    // User elected not to install the unsigned file (or was blocked
                    // by policy from doing so).
                    //
                    goto clean2;
                }
            }

            //
            // Set a flag in the queue that indicates the user has been informed
            // of a signature problem with this queue, and has elected to go
            // ahead and install anyway.  Don't set this flag if the queue's
            // policy is "Ignore", on the chance that the policy might be
            // altered later, and we'd want the user to get informed on any
            // subsequent errors.
            //
            if(Queue->DriverSigningPolicy != DRIVERSIGN_NONE) {
                Queue->Flags |= FQF_DIGSIG_ERRORS_NOUI;
            }

            if (QueueNode) {
                QueueNode->InternalFlags |= ExemptCopyFlags;
            }

            //
            // Reset rc to NO_ERROR and carry on.
            //
            rc = NO_ERROR;
        }

        if(rc == NO_ERROR) {

            rc = CreateTargetAsLinkToMaster(
                    Queue,
                    FullSourceFilename,
                    FullTargetFilename,
                    CopyMsgHandler,
                    Context,
                    IsMsgHandlerNativeCharWidth
                    );
        }

        if(rc == NO_ERROR) {
            //
            // We're done!
            //
            Ok = TRUE;
            goto clean2;
        }
    }

    //
    // We will copy the file to a temporary location. This makes version checks
    // possible in all cases (even when the source is compressed) and simplifies
    // the logic below. Start by forming the name of the temporary file.
    //
    lstrcpyn(Buffer1, FullTargetFilename, MAX_PATH);
    *((PTSTR)MyGetFileTitle(Buffer1)) = TEXT('\0');

    if(!GetTempFileName(Buffer1, TEXT("SETP"), 0, Buffer2)) {
        rc = GetLastError();
        goto clean2;
    }

    TemporaryTargetFile = DuplicateString(Buffer2);
    if(!TemporaryTargetFile) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto clean2;
    }

    //
    // Perform the actual file copy. This creates the temporary target file.
    // Move is allowed as an optimization if we're deleting the source file.
    // The call we make below will not use move if the file is compressed
    // and we are supposed to decompress it, so the right thing will happen
    // in all cases.
    //
    // There are 2 potential issues:
    //
    // 1) When we call the callback function below for a version check,
    //    the source file won't exist any more if the file was moved. Oh well.
    //
    // 2) If the MoveFileEx below fails, the source will have still been 'deleted'.
    //    This is different from the non-move case, where the source remains
    //    intact unless this function is successful.
    //
    // Otherwise this is a non-issue since any compressed file will be decompressed
    // by this call, so version gathering, etc, will all work properly.
    //
    rc = pSetupDecompressOrCopyFile(
            FullSourceFilename,
            TemporaryTargetFile,
            &CompressionType,
            ((CopyStyle & SP_COPY_DELETESOURCE) != 0),
            &Moved
            );

    if(rc != NO_ERROR) {
        goto clean3;
    }

    //
    // Do digital signature check on source file, now that it exists in its
    // final form under a temp name.  Note that for signed files, we ignore
    // version checking since they're an inherently unreliable mechanism for
    // comparing files provided from two different vendors (who use different
    // versioning schemes, etc.)
    //
    // To see why we ignore version numbers, consider the decision tree we'd
    // use if we were paying attention to version numbers as well as digital
    // signatures.  In the discussion that follows, NewFile is the (signed) file
    // we're going to copy, and OldFile is the file that will be overwritten if
    // the copy goes through...
    //
    // if NewFile is versioned {
    //     if OldFile is signed {
    //         if OldFile is versioned {
    //             //
    //             // Both NewFile and OldFile are signed and versioned.
    //             //
    //             if OldFile is a newer version {
    //                 //
    //                 // This is the controversial case.  Since these two incarnations could've come from different vendors
    //                 // with different versioning schemes, we really can't use versioning as a very accurate method of determining
    //                 // which file is 'better'.  So there are two options:
    //                 //    1.  Leave OldFile alone, and if the package being installed can't work with OldFile, then the user must 'undo'
    //                 //         the installation, and then call their vendor to gripe--there'll be no way for them to get the new package to
    //                 //         work, even though WHQL certified it.
    //                 //    2.  Overwrite OldFile.  Since we're then guaranteeing that every file signed as part of the package will be
    //                 //         present, then we can have a much higher degree of certainty that our WHQL certification will hold true
    //                 //         for every user's machine.  If replacing OldFile breaks someone else (e.g., the previously-installed package
    //                 //         that uses it, then the user can 'undo' the installation.  This scenario is better because even though the old
    //                 //         and new packages can't be used simultaneously, at least one or the other can be made to work
    //                 //         independently.
    //                 //
    //                 overwrite OldFile
    //             } else {  // NewFile is a newer version
    //                 overwrite OldFile
    //             }
    //         } else {  // OldFile isn't versioned--NewFile wins
    //             overwrite OldFile
    //         }
    //     } else {  // OldFile isn't signed--we don't care what its version is
    //         overwrite OldFile
    //     }
    // } else {  // NewFile isn't versioned
    //     if OldFile is versioned {
    //         if OldFile is signed {
    //             //
    //             // (See discussion above where both OldFile and NewFile are signed and versioned, and OldFile is newer.  Note
    //             // that something funny is going on if we've been asked to replace a versionable file with an unversionable one!)
    //             //
    //             overwrite OldFile
    //         } else {  // OldFile isn't signed
    //             overwrite OldFile
    //         }
    //     } else {  // OldFile isn't versioned either
    //         overwrite OldFile
    //     }
    // }
    //

    //
    // Check to see if the source file is signed.  (Note--we may have already
    // checked this previously in determining whether an SIS link could be
    // created.  If we failed to verify the file's digital signature then,
    // there's no use in re-verifying here.)
    //
    if(*SignatureVerifyFailed) {
        //
        // We saved the signature verification failure error when we previously
        // attempted to verify this file.  Restore that code to rc now, because
        // the code below relies on the value of rc.
        //
        MYASSERT(SigVerifRc != NO_ERROR);
        rc = SigVerifRc;

    } else {

        rc = VerifySourceFile(lc,
                              Queue,
                              QueueNode,
                              MyGetFileTitle(FullTargetFilename),
                              TemporaryTargetFile,
                              FullSourceFilename,
                              ((Queue && (Queue->Flags & FQF_USE_ALT_PLATFORM))
                                  ? &(Queue->AltPlatformInfo)
                                  : NULL),
                              TRUE,
                              &Problem,
                              Buffer1
                             );

        *SignatureVerifyFailed = (rc != NO_ERROR);

    }

    //
    // Don't muck with the value of rc below unless you're setting it immediately
    // before jumping to clean4.  The return value from VerifySourceFile needs
    // to be preserved until we finish with the HandleFailedVerification stuff.
    //

    //
    // If we are going to perform version checks, fetch the version data
    // of the source (which is now the temporary target file).
    //
    NotifyFlags = 0;
    if(ExistingFile) {

        param = 0;

        //
        // If we're not supposed to overwrite existing files,
        // then the overwrite check fails.
        //
        if(CopyStyle & SP_COPY_NOOVERWRITE) {
            NotifyFlags |= SPFILENOTIFY_TARGETEXISTS;
        }

        //
        // Even if the source file has a verified digital signature, we still
        // want to retrieve version information for the source and target.  We
        // do this so that we can detect when we've overwritten a newer-versioned
        // file with an older-versioned one.  If we discover that this is the
        // case, then we generate an exception log entry that will help PSS
        // troubleshoot any problems that result.
        //
        if(!GetVersionInfoFromImage(TemporaryTargetFile, &SourceVersion, &SourceLanguage)) {
            SourceVersion = 0;
            SourceLanguage = 0;
        }

        //
        // If we're not supposed to overwrite files in a different language
        // and the languages differ, then the language check fails.
        // If either file doesn't have language data, then don't do a language
        // check.
        //
        //
        // Special case:
        //
        // If
        //  a) the source version is greater than the target version
        //  b) the source doesn't have a lang id
        //  c) the target does have a lang id
        // Then
        //  we will do a language check, and we will consider this language check
        //  as passed since we are replacing an older language specific file with
        //  a language neutral file, which is an OK thing.
        //
        //
        if(CopyStyle & SP_COPY_LANGUAGEAWARE) {
            if ( SourceLanguage
                 && TargetLanguage
                 && (SourceLanguage != TargetLanguage) ) {

                NotifyFlags |= SPFILENOTIFY_LANGMISMATCH;
                param = (UINT)MAKELONG(SourceLanguage, TargetLanguage);

            } else if ( !SourceLanguage
                        && TargetLanguage
                        && (TargetVersion >= SourceVersion) ) {

                NotifyFlags |= SPFILENOTIFY_LANGMISMATCH;
                param = (UINT)MAKELONG(SourceLanguage, TargetLanguage);

            }

        }


        //
        // If we're not supposed to overwrite newer versions and the target is
        // newer than the source, then the version check fails. If either file
        // doesn't have version info, fall back to timestamp comparison.
        //
        // If the files are the same version/timestamp, assume that either
        // replacing the existing one is a benevolent operation, or that
        // we are upgrading an existing file whose version info is unimportant.
        // In this case we just go ahead and copy the file (unless the
        // SP_COPY_NEWER_ONLY flag is set).
        //
        // Note that the version checks below are made without regard to presence
        // or absence of digital signatures on either the source or target files.
        // That will be handled later.  We want to see what would've happened
        // without driver signing, so we can generate a PSS exception log when
        // something weird happens.
        //
        if(SourceVersion || TargetVersion) {

            b = (CopyStyle & SP_COPY_NEWER_ONLY)
              ? (TargetVersion >= SourceVersion)
              : (TargetVersion > SourceVersion);

        } else {
#if 0
            //
            // (tedm) removed timestamp-based checking. It's just not a reliable
            // way of doing things.
            //

            //
            // File time on FAT is only guaranteed accurate to within 2 seconds.
            // Round the filetimes before comparison by converting to DOS time
            // and back. If the conversions fail then just use the original values.
            //
            if(!FileTimeToDosDateTime(&SourceFindData.ftLastWriteTime,&sDosDate,&sDosTime)
            || !FileTimeToDosDateTime(&TargetFindData.ftLastWriteTime,&tDosDate,&tDosTime)
            || !DosDateTimeToFileTime(sDosDate,sDosTime,&sFileTime)
            || !DosDateTimeToFileTime(tDosDate,tDosTime,&tFileTime)) {

                sFileTime = SourceFindData.ftLastWriteTime;
                tFileTime = TargetFindData.ftLastWriteTime;
            }

            //
            // If the FORCE_NEWER flag is set then don't use timestamps
            // for versioning. This more closely matches Win95's setupx behavior.
            //
            b = (CopyStyle & SP_COPY_FORCE_NEWER)
              ? FALSE
              : (CompareFileTime(&tFileTime,&sFileTime) > 0);
#else
            b  = FALSE;
#endif
        }

        //
        // At this point, if b is TRUE then the target file has a later (newer)
        // version than the sourcefile.  If we've been asked to pay attention to
        // that, then set the NotifyFlags to indicate this problem.
        // Note that this may get reset later if the source file is signed.  We
        // still wanted to get this information so we could put it in our PSS
        // logfile.
        //
        if(b &&
           (CopyStyle & (SP_COPY_NEWER_OR_SAME | SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER))) {

            NotifyFlags |= SPFILENOTIFY_TARGETNEWER;
        }
    }

    if(NotifyFlags & SPFILENOTIFY_TARGETNEWER) {

        if(!*SignatureVerifyFailed) {
            //
            // Source file is signed, but the target file is newer.  We know
            // that we still want to replace the existing targetfile with the
            // sourcefile, regardless of version numbers.  However, we need to
            // make note of that in our PSS logfile.
            //
            NotifyFlags &= ~SPFILENOTIFY_TARGETNEWER;

            //
            // Check to see if the target file is signed, in order to include
            // this information in our PSS logfile.
            //
            ExistingTargetFileWasSigned = (NO_ERROR == VerifyFile(lc,
                                                           NULL,
                                                           NULL,
                                                           0,
                                                           MyGetFileTitle(FullTargetFilename),
                                                           ExistingFile,
                                                           NULL,
                                                           NULL,
                                                           FALSE,
                                                           ((Queue && (Queue->Flags & FQF_USE_ALT_PLATFORM))
                                                               ? &(Queue->AltPlatformInfo)
                                                               : NULL),
                                                           NULL,
                                                           NULL
                                                          ));

            //
            // SPLOG -- report that newer target was overwritten by older (signed)
            // source, whether target was signed, both files' versions, etc.
            // Also probably want to throw in the fact that the SP_COPY_FORCE_NEWER
            // flag was ignored, if it's set.
            //
            pGetVersionText(SourceVersionText,SourceVersion);
            pGetVersionText(TargetVersionText,TargetVersion);
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                MSG_LOG_NEWER_FILE_OVERWRITTEN,
                NULL,
                FullTargetFilename,
                SourceVersionText,
                TargetVersionText);

            if (CopyStyle & SP_COPY_FORCE_NEWER) {
                WriteLogEntry(
                    lc,
                    SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                    MSG_LOG_FLAG_FORCE_NEWER_IGNORED,
                    NULL);
            }
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                (ExistingTargetFileWasSigned ?
                    MSG_LOG_TARGET_WAS_SIGNED :
                    MSG_LOG_TARGET_WAS_NOT_SIGNED),
                NULL);
            //
            // flush the log buffer
            //
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING,
                0,
                TEXT("\n"));

        } else {
            //
            // Source file isn't signed, so we'll let the old behavior apply.
            // If version dialogs are allowed, for example, the user will be
            // prompted that they're attempting to overwrite a newer file with an
            // older one.  If they say "go ahead and copy over the older version",
            // _then_ they'll get a prompt about the file not having a valid
            // signature.
            //
            // Check special case where a flag is set indicating that we should
            // just silently not copy the newer file.
            //
            if(CopyStyle & SP_COPY_FORCE_NEWER) {
                //
                // Target is newer; don't copy the file.
                //
                rc = NO_ERROR;
                goto clean4;
            }

        }
    }

    //
    // If we have any reason to notify the caller via the callback,
    // do that here. If there is no callback, then don't copy the file,
    // because one of the conditions has not been met.
    //
    if(NotifyFlags) {

        FilePaths.Source = FullSourceFilename;
        FilePaths.Target = FullTargetFilename;
        FilePaths.Win32Error = NO_ERROR;

        if(!CopyMsgHandler
        || !pSetupCallMsgHandler(
                CopyMsgHandler,
                IsMsgHandlerNativeCharWidth,
                Context,
                NotifyFlags,
                (UINT_PTR)&FilePaths,
                param))
        {
            if(ExistingFile) {
                //
                // Check to see if the target file is signed, in order to include
                // this information in our PSS logfile.
                //
                ExistingTargetFileWasSigned = (NO_ERROR == VerifyFile(lc,
                                                               NULL,
                                                               NULL,
                                                               0,
                                                               MyGetFileTitle(FullTargetFilename),
                                                               ExistingFile,
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               ((Queue && (Queue->Flags & FQF_USE_ALT_PLATFORM))
                                                                   ? &(Queue->AltPlatformInfo)
                                                                   : NULL),
                                                               NULL,
                                                               NULL
                                                              ));
            }

            //
            // SPLOG -- error that occurred, info on whether source and target
            // files were signed, what their versions were, etc.
            //

            //
            // BUGBUG - is this all correct?
            //
            pGetVersionText(SourceVersionText,SourceVersion);
            pGetVersionText(TargetVersionText,TargetVersion);
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                MSG_LOG_FILE_NOT_OVERWRITTEN,
                NULL,
                SourceVersionText,
                TargetVersionText);
            if (NotifyFlags & SPFILENOTIFY_TARGETEXISTS) {
                WriteLogEntry(
                    lc,
                    SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                    MSG_LOG_NOTIFY_TARGETEXISTS,
                    NULL);
            }
            if (NotifyFlags & SPFILENOTIFY_LANGMISMATCH) {
                WriteLogEntry(
                    lc,
                    SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                    MSG_LOG_NOTIFY_LANGMISMATCH,
                    NULL);
            }
            if (NotifyFlags & SPFILENOTIFY_TARGETNEWER) {
                WriteLogEntry(
                    lc,
                    SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                    MSG_LOG_NOTIFY_TARGETNEWER,
                    NULL);
            }
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                (ExistingTargetFileWasSigned ?
                    MSG_LOG_TARGET_WAS_SIGNED :
                    MSG_LOG_TARGET_WAS_NOT_SIGNED),
                NULL);
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                (*SignatureVerifyFailed ?
                    MSG_LOG_SOURCE_WAS_NOT_SIGNED :
                    MSG_LOG_SOURCE_WAS_SIGNED),
                NULL);
            WriteLogEntry(
                lc,
                SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                MSG_LOG_ERROR_ENCOUNTERED,
                NULL);
            WriteLogError(
                lc,
                SETUP_LOG_WARNING,
                rc);

            rc = NO_ERROR;
            goto clean4;
        }
    }

    //
    // OK, now that all non-codesigning stuff is done, tell the user about
    // any digital signature problems we found with the source file.
    //
    // NOTE: If SigVerifRc is set to something other than NO_ERROR, then we know
    // that the HandleFailedVerification routine has already been called, thus
    // we don't want to call it again or otherwise the user may get multiple
    // prompts about a bad signature for the same file.
    //
    if(*SignatureVerifyFailed) {

        if(SigVerifRc == NO_ERROR) {

            BOOL FailureIgnored, DoingDeviceInstall;
            DWORD DriverSigningPolicy;

            MYASSERT(ExemptCopyFlags == 0);

            //
            // Save away the verification error in SigVerifRc, so we can use
            // this later to determine whether we're dealing with an unsigned
            // file.
            //
            MYASSERT(rc != NO_ERROR);
            SigVerifRc = rc;

            //
            // If we're not using a file queue, then try to determine whether or
            // not this is a device installation based on the INF supplied (if
            // there was one).
            //
            if(Queue) {
                //
                // if we're set to block on a per queue basis, then bail out
                // here
                //
                if (Queue->Flags & FQF_QUEUE_FORCE_BLOCK_POLICY) {
                    goto clean4;
                }
                if(Queue->DeviceDescStringId != -1) {
                    p = pStringTableStringFromId(Queue->StringTable,
                                                 Queue->DeviceDescStringId
                                                );
                    MYASSERT(p);
                } else {
                    p = NULL;
                }

                DoingDeviceInstall = Queue->Flags & FQF_DEVICE_INSTALL;
                DriverSigningPolicy = Queue->DriverSigningPolicy;

            } else {

                DoingDeviceInstall = IsInfForDeviceInstall(lc, 
                                                           LoadedInf, 
                                                           &p, 
                                                           &DriverSigningPolicy,
                                                           NULL
                                                          );
            }

            //
            // If this is a device installation and the policy is "Ignore",
            // then crank it up to "Warn" if the file is under SFP's
            // protection.  This will allow the user to update a driver that
            // ships in our box for which no WHQL certification program
            // exists.
            //
            if(DoingDeviceInstall && (DriverSigningPolicy == DRIVERSIGN_NONE) &&
               IsFileProtected(FullTargetFilename, lc, NULL)) {
    
                DriverSigningPolicy = DRIVERSIGN_WARNING;

                //
                // If we have a queue, update the queue's policy as well.
                //
                if(Queue) {
                    Queue->DriverSigningPolicy = DRIVERSIGN_WARNING;
                }

                //
                // Log the fact that policy was elevated.
                //
                WriteLogEntry(lc,
                              SETUP_LOG_ERROR,
                              MSG_LOG_POLICY_ELEVATED_FOR_SFC,
                              NULL
                             );
            }

            FailureIgnored = HandleFailedVerification((Queue ? Queue->hWndDriverSigningUi
                                                             : NULL),
                                                      Problem,
                                                      Buffer1,
                                                      p,
                                                      DriverSigningPolicy,
                                                      (Queue ? (Queue->Flags & FQF_DIGSIG_ERRORS_NOUI)
                                                             : FALSE),
                                                      rc,
                                                      lc,
                                                      &ExemptCopyFlags,
                                                      FullTargetFilename
                                                     );
            //
            // If a buffer was allocated for the device description as a result
            // of calling IsInfForDeviceInstall, then free that buffer now.
            //
            if(!Queue && p) {
                MyFree(p);
            }

            if(!FailureIgnored) {
                //
                // User elected not to install the unsigned file (or was blocked
                // by policy from doing so).
                //
                goto clean4;
            }
        }

        if(Queue) {
            //
            // Set a flag in the queue that indicates the user has been informed
            // of a signature problem with this queue, and has elected to go
            // ahead and install anyway.  Don't set this flag if the queue's
            // policy is "Ignore", on the chance that the policy might be
            // altered later, and we'd want the user to get informed on any
            // subsequent errors.
            //
            if(Queue->DriverSigningPolicy != DRIVERSIGN_NONE) {
                Queue->Flags |= FQF_DIGSIG_ERRORS_NOUI;
            }

            if (QueueNode) {
                QueueNode->InternalFlags |= ExemptCopyFlags;
            }
        }

        //
        // Reset rc to NO_ERROR and carry on.
        //
        rc = NO_ERROR;
    }

    //
    // Move the target file into its final location.
    //
    SetFileAttributes(FullTargetFilename, FILE_ATTRIBUTE_NORMAL);

    //
    // If the target exists and the force-in-use flag is set, then don't try to
    // move the file into place now -- automatically drop into in-use behavior.
    //
    // Want to use MoveFileEx but it didn't exist in Win95. Ugh.
    //
    if(!(ExistingFile && (CopyStyle & SP_COPY_FORCE_IN_USE))
    && (b = DoMove(TemporaryTargetFile, FullTargetFilename))) {
        //
        // Place security information on the target file if necessary.
        // Ignore errors. The theory here is that the file is already on
        // the target, so if this fails the worst case is that the file is
        // not secure. But the user can still use the system.
        //
        //  BUGBUG--log an entry in setupapi.log if this fails.
        //
        if(SecurityInfo) {
            StampFileSecurity(FullTargetFilename, SecurityInfo);
        }
    } else {
        //
        // If this fails, assume the file is in use and mark it for copy on next
        // boot (except in the case where we're copying a boot file, in which
        // case this is a catastrophic failure).
        //
        if(ExistingFile != FullTargetFilename) {

            WriteLogEntry(lc,
                          SETUP_LOG_ERROR,
                          MSG_LOG_REPLACE_BOOT_FILE_FAILED,
                          NULL,
                          FullTargetFilename
                         );

            b = FALSE;
            SetLastError(ERROR_ACCESS_DENIED);
        } else {
            b = TRUE;
            try {
                *FileWasInUse = TRUE;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                b = FALSE;
            }

            if(b) {
                //
                // If we're trying to do a delayed move to replace a protected
                // system file (using an unsigned one), and we've not been
                // granted an exception to do so, then we should skip the
                // operation altogether (and make a log entry about it)
                //
                if((SigVerifRc != NO_ERROR) &&
                   ((ExemptCopyFlags & (IQF_TARGET_PROTECTED | IQF_ALLOW_UNSIGNED)) == IQF_TARGET_PROTECTED)) {

                    WriteLogEntry(lc,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_DELAYED_MOVE_SKIPPED_FOR_SFC,
                                  NULL,
                                  FullTargetFilename
                                 );

                    //
                    // Delete the source file.
                    //
                    SetFileAttributes(TemporaryTargetFile, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(TemporaryTargetFile);

                } else {

                    BOOL TargetIsProtected = IsFileProtected(FullTargetFilename, lc, NULL);

                    if(Queue == NULL) {

                        b = DelayedMove(
                                TemporaryTargetFile,
                                FullTargetFilename
                                );

                        if(b && TargetIsProtected) {
                            //
                            // we have to explicitly tell the session manager it's ok
                            // to replace the changed files on reboot.
                            //
                            // in the case that we're using a queue, we post the
                            // delayed move and set this flag only if all the delayed
                            // move operations succeed
                            //
                            pSetupProtectedRenamesFlag(TRUE);
                        }
                    } else {
                        b = PostDelayedMove(
                                Queue,
                                TemporaryTargetFile,
                                FullTargetFilename,
                                QueueNode->SecurityDesc,
                                TargetIsProtected
                                );
                    }

                    if(b) {
                        //
                        // Couldn't set security info on the actual target, so at least
                        // set it on the temp file that will become the target.
                        //
                        if(SecurityInfo) {
                            StampFileSecurity(TemporaryTargetFile,SecurityInfo);
                        }

                        if (lc) {
                            WriteLogEntry(lc,
                                          SETUP_LOG_INFO,
                                          MSG_LOG_COPY_DELAYED,
                                          NULL,
                                          FullTargetFilename,
                                          TemporaryTargetFile
                                          );
                        }

                        //
                        // Tell the callback that we queued this file for delayed copy.
                        //
                        if(CopyMsgHandler) {

                            FilePaths.Source = TemporaryTargetFile;
                            FilePaths.Target = FullTargetFilename;
                            FilePaths.Win32Error = NO_ERROR;
                            FilePaths.Flags = FILEOP_COPY;

                            pSetupCallMsgHandler(
                                CopyMsgHandler,
                                IsMsgHandlerNativeCharWidth,
                                Context,
                                SPFILENOTIFY_FILEOPDELAYED,
                                (UINT_PTR)&FilePaths,
                                0
                                );
                        }
                    }
                }

            } else {
                //
                // FileWasInUse pointer went bad
                //
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
    }

    if(!b) {
        rc = GetLastError();
        goto clean4;
    }

    //
    // We're done. Delete the source if necessary and return.
    //
    if((CopyStyle & SP_COPY_DELETESOURCE) && !Moved) {
        DeleteFile(FullSourceFilename);
    }

    rc = NO_ERROR;
    Ok = TRUE;
    goto clean3;

clean4:
    //
    // Remove temporary target file.
    // In case pSetupDecompressOrCopyFile MoveFile'd the source,
    // we really need to try to move it back, so the source file isn't
    // blown away when this routine fails.
    //
    if(Moved) {
        MoveFile(TemporaryTargetFile,FullSourceFilename);
    } else {
        SetFileAttributes(TemporaryTargetFile,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(TemporaryTargetFile);
    }
clean3:
    MyFree(TemporaryTargetFile);
clean2:
    MyFree(FullTargetFilename);
clean1:
    MyFree(FullSourceFilename);
clean0:
    if(SecurityInfo) {
        MyFree(SecurityInfo);
    }
    //
    // if there was an error of some sort, log it
    //
    if (rc != NO_ERROR) {
        //
        // maybe we ought to embelish this a bit.
        //
        WriteLogEntry(
            lc,
            SETUP_LOG_ERROR,
            rc,
            NULL);
    }

    if(slot_fileop) {
        ReleaseLogInfoSlot(lc, slot_fileop);
    }

    if(LoadedInf) {
        UnlockInf(LoadedInf);
    }

    SetLastError(rc);
    return(Ok);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFileExA(
    IN  HINF                InfHandle,         OPTIONAL
    IN  PINFCONTEXT         InfContext,        OPTIONAL
    IN  PCSTR               SourceFile,        OPTIONAL
    IN  PCSTR               SourcePathRoot,    OPTIONAL
    IN  PCSTR               DestinationName,   OPTIONAL
    IN  DWORD               CopyStyle,
    IN  PSP_FILE_CALLBACK_A CopyMsgHandler,    OPTIONAL
    IN  PVOID               Context,           OPTIONAL
    OUT PBOOL               FileWasInUse
    )
{
    PCWSTR sourceFile,sourcePathRoot,destinationName;
    BOOL b, DontCare;
    DWORD rc;

    sourceFile = NULL;
    sourcePathRoot = NULL;
    destinationName = NULL;
    rc = NO_ERROR;

    if(SourceFile) {
        rc = CaptureAndConvertAnsiArg(SourceFile,&sourceFile);
    }
    if((rc == NO_ERROR) && SourcePathRoot) {
        rc = CaptureAndConvertAnsiArg(SourcePathRoot,&sourcePathRoot);
    }
    if((rc == NO_ERROR) && DestinationName) {
        rc = CaptureAndConvertAnsiArg(DestinationName,&destinationName);
    }

    if(rc == NO_ERROR) {

        b = _SetupInstallFileEx(
                NULL,
                NULL,
                InfHandle,
                InfContext,
                sourceFile,
                sourcePathRoot,
                destinationName,
                CopyStyle,
                CopyMsgHandler,
                Context,
                FileWasInUse,
                FALSE,
                &DontCare
                );

        rc = b ? NO_ERROR : GetLastError();

    } else {
        b = FALSE;
    }

    if(sourceFile) {
        MyFree(sourceFile);
    }
    if(sourcePathRoot) {
        MyFree(sourcePathRoot);
    }
    if(destinationName) {
        MyFree(destinationName);
    }
    SetLastError(rc);
    return b;
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFileExW(
    IN  HINF                InfHandle,         OPTIONAL
    IN  PINFCONTEXT         InfContext,        OPTIONAL
    IN  PCWSTR              SourceFile,        OPTIONAL
    IN  PCWSTR              SourcePathRoot,    OPTIONAL
    IN  PCWSTR              DestinationName,   OPTIONAL
    IN  DWORD               CopyStyle,
    IN  PSP_FILE_CALLBACK_W CopyMsgHandler,    OPTIONAL
    IN  PVOID               Context,           OPTIONAL
    OUT PBOOL               FileWasInUse
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfContext);
    UNREFERENCED_PARAMETER(SourceFile);
    UNREFERENCED_PARAMETER(SourcePathRoot);
    UNREFERENCED_PARAMETER(DestinationName);
    UNREFERENCED_PARAMETER(CopyStyle);
    UNREFERENCED_PARAMETER(CopyMsgHandler);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(FileWasInUse);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupInstallFileEx(
    IN  HINF              InfHandle,         OPTIONAL
    IN  PINFCONTEXT       InfContext,        OPTIONAL
    IN  PCTSTR            SourceFile,        OPTIONAL
    IN  PCTSTR            SourcePathRoot,    OPTIONAL
    IN  PCTSTR            DestinationName,   OPTIONAL
    IN  DWORD             CopyStyle,
    IN  PSP_FILE_CALLBACK CopyMsgHandler,    OPTIONAL
    IN  PVOID             Context,           OPTIONAL
    OUT PBOOL             FileWasInUse
    )

/*++

Routine Description:

    Same as SetupInstallFile().

Arguments:

    Same as SetupInstallFile().

    FileWasInUse - receives flag indicating whether the file was in use.

Return Value:

    Same as SetupInstallFile().

--*/

{
    BOOL b, DontCare;
    PCTSTR sourceFile,sourcePathRoot,destinationName;
    PCTSTR p;
    DWORD rc;

    //
    // Capture args.
    //
    if(SourceFile) {
        rc = CaptureStringArg(SourceFile,&p);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return FALSE;
        }
        sourceFile = p;
    } else {
        sourceFile = NULL;
    }

    if(SourcePathRoot) {
        rc = CaptureStringArg(SourcePathRoot,&p);
        if(rc != NO_ERROR) {
            if(sourceFile) {
                MyFree(sourceFile);
            }
            SetLastError(rc);
            return FALSE;
        }
        sourcePathRoot = p;
    } else {
        sourcePathRoot = NULL;
    }

    if(DestinationName) {
        rc = CaptureStringArg(DestinationName,&p);
        if(rc != NO_ERROR) {
            if(sourceFile) {
                MyFree(sourceFile);
            }
            if(sourcePathRoot) {
                MyFree(sourcePathRoot);
            }
            SetLastError(rc);
            return FALSE;
        }
        destinationName = p;
    } else {
        destinationName = NULL;
    }

    b = _SetupInstallFileEx(
            NULL,
            NULL,
            InfHandle,
            InfContext,
            sourceFile,
            sourcePathRoot,
            destinationName,
            CopyStyle,
            CopyMsgHandler,
            Context,
            FileWasInUse,
            TRUE,
            &DontCare
            );

    //
    // We GetLastError and then set it back again before returning, so that
    // the memory frees we do below can't blow away the error code.
    //
    rc = b ? NO_ERROR : GetLastError();

    if(sourceFile) {
        MyFree(sourceFile);
    }
    if(sourcePathRoot) {
        MyFree(sourcePathRoot);
    }
    if(destinationName) {
        MyFree(destinationName);
    }

    SetLastError(rc);
    return b;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFileA(
    IN HINF                InfHandle,         OPTIONAL
    IN PINFCONTEXT         InfContext,        OPTIONAL
    IN PCSTR               SourceFile,        OPTIONAL
    IN PCSTR               SourcePathRoot,    OPTIONAL
    IN PCSTR               DestinationName,   OPTIONAL
    IN DWORD               CopyStyle,
    IN PSP_FILE_CALLBACK_A CopyMsgHandler,    OPTIONAL
    IN PVOID               Context            OPTIONAL
    )
{
    BOOL b;
    BOOL InUse;

    b = SetupInstallFileExA(
            InfHandle,
            InfContext,
            SourceFile,
            SourcePathRoot,
            DestinationName,
            CopyStyle,
            CopyMsgHandler,
            Context,
            &InUse
            );

    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFileW(
    IN HINF                InfHandle,         OPTIONAL
    IN PINFCONTEXT         InfContext,        OPTIONAL
    IN PCWSTR              SourceFile,        OPTIONAL
    IN PCWSTR              SourcePathRoot,    OPTIONAL
    IN PCWSTR              DestinationName,   OPTIONAL
    IN DWORD               CopyStyle,
    IN PSP_FILE_CALLBACK_W CopyMsgHandler,    OPTIONAL
    IN PVOID               Context            OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfContext);
    UNREFERENCED_PARAMETER(SourceFile);
    UNREFERENCED_PARAMETER(SourcePathRoot);
    UNREFERENCED_PARAMETER(DestinationName);
    UNREFERENCED_PARAMETER(CopyStyle);
    UNREFERENCED_PARAMETER(CopyMsgHandler);
    UNREFERENCED_PARAMETER(Context);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupInstallFile(
    IN HINF              InfHandle,         OPTIONAL
    IN PINFCONTEXT       InfContext,        OPTIONAL
    IN PCTSTR            SourceFile,        OPTIONAL
    IN PCTSTR            SourcePathRoot,    OPTIONAL
    IN PCTSTR            DestinationName,   OPTIONAL
    IN DWORD             CopyStyle,
    IN PSP_FILE_CALLBACK CopyMsgHandler,    OPTIONAL
    IN PVOID             Context            OPTIONAL
    )

/*++

Routine Description:

    Note: no disk prompting is performed by this routine. The caller must
    ensure that the source specified in SourcePathRoot or SourceFile
    (see below) is accessible.

Arguments:

    InfHandle - handle of inf file containing [SourceDisksNames]
        and [SourceDisksFiles] sections. If InfContext is not specified
        and CopyFlags includes SP_COPY_SOURCE_ABSOLUTE or
        SP_COPY_SOURCEPATH_ABSOLUTE, then InfHandle is ignored.

    InfContext - if specified, supplies context for a line in a copy file
        section in an inf file. The routine looks this file up in the
        [SourceDisksFiles] section of InfHandle to get file copy info.
        If not specified, SourceFile must be.  If this parameter is specified,
        then InfHandle must also be specified.

    SourceFile - if specified, supplies the file name (no path) of the file
        to be copied. The file is looked up in [SourceDisksFiles].
        Must be specified if InfContext is not; ignored if InfContext
        is specified.

    SourcePathRoot - Supplies the root path for the source (for example,
        a:\ or f:\).  Paths in [SourceDisksNames] are appended to this path.
        Ignored if CopyStyle includes SP_COPY_SOURCE_ABSOLUTE.

    DestinationName - if InfContext is specified, supplies the filename only
        (no path) of the target file. Can be NULL to indicate that the
        target file is to have the same name as the source file. If InfContext is
        not specified, supplies the full target path and filename for the target
        file.

    CopyStyle - supplies flags that control the behavior of the copy operation.

        SP_COPY_DELETESOURCE - Delete the source file upon successful copy.
            The caller receives no notification if the delete fails.

        SP_COPY_REPLACEONLY - Copy the file only if doing so would overwrite
            a file at the destination path.

        SP_COPY_NEWER - Examine each file being copied to see if its version resources
            (or timestamps for non-image files) indicate that it it is not newer than
            an existing copy on the target. If so, and a CopyMsgHandler is specified,
            the caller is notified and may veto the copy. If CopyMsgHandler is not
            specified, the file is not copied.

        SP_COPY_NOOVERWRITE - Check whether the target file exists, and, if so,
            notify the caller who may veto the copy. If no CopyMsgHandler is specified,
            the file is not overwritten.

        SP_COPY_NODECOMP - Do not decompress the file. When this option is given,
            the target file is not given the uncompressed form of the source name
            (if appropriate). For example, copying f:\mips\cmd.ex_ to \\foo\bar
            will result a target file \\foo\bar\cmd.ex_. (If this flag wasn't specified
            the file would be decompressed and the target would be called
            \\foo\bar\cmd.exe). The filename part of the target file name
            is stripped and replaced with the filename of the soruce. When this option
            is given, SP_COPY_LANGUAGEAWARE and SP_COPY_NEWER are ignored.

        SP_COPY_LANGUAGEAWARE - Examine each file being copied to see if its language
            differs from the language of any existing file already on the target.
            If so, and a CopyMsgHandler is specified, the caller is notified and
            may veto the copy. If CopyMsgHandler is not specified, the file is not copied.

        SP_COPY_SOURCE_ABSOLUTE - SourceFile is a full source path.
            Do not attempt to look it up in [SourceDisksNames].

        SP_COPY_SOURCEPATH_ABSOLUTE - SourcePathRoot is the full path part of the
            source file. Ignore the relative source specified in the [SourceDisksNames]
            section of the inf file for the source media where the file is located.
            Ignored if SP_COPY_SOURCE_ABSOLUTE is specified.

        SP_COPY_FORCE_IN_USE - if the target exists, behave as if it is in use and
            queue the file for copy on next reboot.

    CopyMsgHandler - if specified, supplies a callback function to be notified of
        various conditions that may arise during the file copy.

    Context - supplies a caller-defined value to be passed as the first
        parameter to CopyMsgHandler.

Return Value:

    TRUE if a file was copied. FALSE if not. Use GetLastError for extended
    error information. If GetLastError returns NO_ERROR, then the file copy was
    aborted because (a) it wasn't needed or (b) a callback function returned
    FALSE.

--*/

{
    BOOL b;
    BOOL InUse;

    b = SetupInstallFileEx(
            InfHandle,
            InfContext,
            SourceFile,
            SourcePathRoot,
            DestinationName,
            CopyStyle,
            CopyMsgHandler,
            Context,
            &InUse
            );

    return(b);
}


DWORD
pSetupMakeSurePathExists(
    IN PCTSTR FullFilespec
    )

/*++

Routine Description:

    This routine ensures that a multi-level path exists by creating individual
    levels one at a time. It is assumed that the caller will pass in a *filename*
    whose path needs to exist. Some examples:

    c:\x                        - C:\ is assumes to always exist.

    c:\x\y\z                    - Ensure that c:\x\y exists.

    \x\y\z                      - \x\y on current drive

    x\y                         - x in current directory

    d:x\y                       - d:x

    \\server\share\p\file       - \\server\share\p

Arguments:

    FullFilespec - supplies the *filename* of a file that the caller wants to
        create. This routine creates the *path* to that file, in other words,
        the final component is assumed to be a filename, and is not a
        directory name. (This routine doesn't actually create this file.)
        If this is invalid, then the results are undefined (for example,
        passing \\server\share, C:\, or C:).

Return Value:

    Win32 error code indicating outcome. If FullFilespec is invalid,
    *may* return ERROR_INVALID_NAME.

--*/

{
    TCHAR Buffer[MAX_PATH];
    PTCHAR p,q;
    BOOL Done;
    DWORD d;
    WIN32_FIND_DATA FindData;

    //
    // The first thing we do is locate and strip off the final component,
    // which is assumed to be the filename. If there are no path separator
    // chars then assume we have a filename in the current directory and
    // that we have nothing to do.
    //
    // Note that if the caller passed an invalid FullFilespec then this might
    // hose us up. For example, \\x\y will result in \\x. We rely on logic
    // in the rest of the routine to catch this.
    //
    lstrcpyn(Buffer,FullFilespec,MAX_PATH);
    if(Buffer[0] && (p = _tcsrchr(Buffer,TEXT('\\'))) && (p != Buffer)) {
        *p = 0;
    } else {
        return(NO_ERROR);
    }

    if(Buffer[0] == TEXT('\\')) {
        if(Buffer[1] == TEXT('\\')) {
            //
            // UNC. Locate the second component, which is the share name.
            // If there's no share name then the original FullFilespec
            // was invalid. Finally locate the first path-sep char in the
            // drive-relative part of the name. Note that there might not
            // be such a char (when the file is in the root). Then skip
            // the path-sep char.
            //
            if(!Buffer[2] || (Buffer[2] == TEXT('\\'))) {
                return(ERROR_INVALID_NAME);
            }
            p = _tcschr(&Buffer[3],TEXT('\\'));
            if(!p || (p[1] == 0) || (p[1] == TEXT('\\'))) {
                return(ERROR_INVALID_NAME);
            }
            if(q = _tcschr(p+2,TEXT('\\'))) {
                q++;
            } else {
                return(NO_ERROR);
            }
        } else {
            //
            // Assume it's a local root-based local path like \x\y.
            //
            q = Buffer+1;
        }
    } else {
        if(Buffer[1] == TEXT(':')) {
            //
            // Assume c:x\y or maybe c:\x\y
            //
            q = (Buffer[2] == TEXT('\\')) ? &Buffer[3] : &Buffer[2];
        } else {
            //
            // Assume path like x\y\z
            //
            q = Buffer;
        }
    }

    //
    // Ignore drive roots.
    //
    if(*q == 0) {
        return(NO_ERROR);
    }

    //
    // If it already exists do nothing.
    //
    if(FileExists(Buffer,&FindData)) {
        return((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NO_ERROR : ERROR_DIRECTORY);
    }

    Done = FALSE;
    do {
        //
        // Locate the next path sep char. If there is none then
        // this is the deepest level of the path.
        //
        if(p = _tcschr(q,TEXT('\\'))) {
            *p = 0;
        } else {
            Done = TRUE;
        }

        //
        // Create this portion of the path.
        //
        if(CreateDirectory(Buffer,NULL)) {
            d = NO_ERROR;
        } else {
            d = GetLastError();
            if(d == ERROR_ALREADY_EXISTS) {
                d = NO_ERROR;
            }
        }

        if(d == NO_ERROR) {
            //
            // Put back the path sep and move to the next component.
            //
            if(!Done) {
                *p = TEXT('\\');
                q = p+1;
            }
        } else {
            Done = TRUE;
        }

    } while(!Done);

    return(d);
}

