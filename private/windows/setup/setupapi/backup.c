/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    backup.c

Abstract:

    Routines to control backup during install process
    And restore of an old install process

Author:

    Jamie Hunter (jamiehun) 13-Jan-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// ==========================================================
//

DWORD
pSetupQueueBackupCopy(
    IN HSPFILEQ QueueHandle,
    IN LONG   TargetRootPath,
    IN LONG   TargetSubDir,       OPTIONAL
    IN LONG   TargetFilename,
    IN LONG   BackupRootPath,
    IN LONG   BackupSubDir,       OPTIONAL
    IN LONG   BackupFilename
    )

/*++

Routine Description:

    Place a backup copy operation on a setup file queue.
    Target is to be backed up at Backup location

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    TargetRootPath  - Supplies the source directory, eg C:\WINNT\

    TargetSubDir    - Supplies the optional sub-directory (eg WINNT if RootPath = c:\ )

    TargetFilename - supplies the filename part of the file to be copied.

    BackupRootPath - supplies the directory where the file is to be copied.

    BackupSubDir   - supplies the optional sub-directory

    BackupFilename - supplies the name of the target file.

Return Value:

    same value as GetLastError() indicating error, or NO_ERROR

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE QueueNode,TempNode;
    int Size;
    DWORD Err;
    PVOID StringTable;
    PTSTR FullRootName;

    Queue = (PSP_FILE_QUEUE)QueueHandle;
    Err = NO_ERROR;

    try {
        StringTable = Queue->StringTable;  // used for strings in source queue
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // Allocate a queue structure.
    //
    QueueNode = MyMalloc(sizeof(SP_FILE_QUEUE_NODE));
    if (!QueueNode) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Operation is backup.
    //
    QueueNode->Operation = FILEOP_BACKUP;
    QueueNode->InternalFlags = 0;

    QueueNode->SourceRootPath = BackupRootPath;
    QueueNode->SourcePath = BackupSubDir;
    QueueNode->SourceFilename = BackupFilename;

    // if target has a sub-dir, we have to combine root and subdir into one string
    if (TargetSubDir != -1) {

        FullRootName = pSetupFormFullPath(
                                            StringTable,
                                            TargetRootPath,
                                            TargetSubDir,
                                            -1);

        if (!FullRootName) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        TargetRootPath = StringTableAddString(StringTable,
                                                FullRootName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        MyFree(FullRootName);

        if (TargetRootPath == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        // now combined into TargetRootPath
        TargetSubDir = -1;

    }
    QueueNode->TargetDirectory = TargetRootPath;
    QueueNode->TargetFilename = TargetFilename;

    QueueNode->Next = NULL;

    //
    // Link the node onto the end of the backup queue
    //

    if (Queue->BackupQueue) {
        for (TempNode = Queue->BackupQueue; TempNode->Next; TempNode=TempNode->Next) /* blank */ ;
        TempNode->Next = QueueNode;
    } else {
        Queue->BackupQueue = QueueNode;
    }

    Queue->BackupNodeCount++;

    Err = NO_ERROR;
    goto clean0;

clean1:
    MyFree(QueueNode);
clean0:
    SetLastError(Err);
    return Err;
}


//
// ==========================================================
//

BOOL
pSetupGetFullBackupPath(
    OUT     PTSTR       FullPath,
    IN      PCTSTR      Path,           OPTIONAL
    IN      UINT        TargetBufferSize,
    OUT     PUINT       RequiredSize    OPTIONAL
    )
/*++

Routine Description:

    This routine takes a potentially relative path
    and concatinates it to the base path

Arguments:

    FullPath    - Destination for full path
    Path        - Relative source path to backup directory if specified.
                    If NULL, generates a temporary path
    TargetBufferSize - Size of buffer (characters)
    RequiredSize - Filled in with size required to contain full path

Return Value:

    If the function succeeds, return TRUE
    If there was an error, return FALSE

--*/
{
    UINT PathLen;
    LPCTSTR Base = WindowsBackupDirectory;

    if(!Path) {
        //
        // temporary location
        //
        Path = SP_BACKUP_OLDFILES;
        Base = WindowsDirectory;
    }

    //
    // Backup directory is stored in "WindowsBackupDirectory" for permanent backups
    // and WindowsDirectory\SP_BACKUP_OLDFILES for temporary backups
    //

    PathLen = lstrlen(Base);

    if ( FullPath == NULL || TargetBufferSize <= PathLen ) {
        // just calculate required path len
        FullPath = (PTSTR) Base;
        TargetBufferSize = 0;
    } else {
        // calculate and copy
        lstrcpy(FullPath, Base);
    }
    return ConcatenatePaths(FullPath, Path, TargetBufferSize, RequiredSize);
}

//
// ==========================================================
//

DWORD
pSetupBackupCopyString(
    IN PVOID            DestStringTable,
    OUT PLONG           DestStringID,
    IN PVOID            SrcStringTable,
    IN LONG             SrcStringID
    )
/*++

Routine Description:

    Gets a string from source string table, adds it to destination string table with new ID.

Arguments:

    DestStringTable     - Where string has to go
    DestStringID        - pointer, set to string ID in respect to DestStringTable
    SrcStringTable      - Where string is coming from
    StringID            - string ID in respect to SrcStringTable

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    DWORD Err = NO_ERROR;
    LONG DestID;
    PTSTR String;

    if (DestStringID == NULL) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (SrcStringID == -1) {
        // "not supplied"
        DestID = -1;
    } else {
        // actually need to copy

        String = StringTableStringFromId( SrcStringTable, SrcStringID );
        if (String == NULL) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        DestID = StringTableAddString( DestStringTable, String, STRTAB_CASE_SENSITIVE );
        if (DestID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        *DestStringID = DestID;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupGetTargetByPath(
    IN HSPFILEQ         QueueHandle,
    IN PVOID            PathStringTable,    OPTIONAL
    IN PCTSTR           TargetPath,         OPTIONAL
    IN LONG             TargetRoot,
    IN LONG             TargetSubDir,       OPTIONAL
    IN LONG             TargetFilename,
    OUT PLONG           TableID,            OPTIONAL
    OUT PSP_TARGET_ENT  TargetInfo
    )
/*++

Routine Description:

    Given a pathname, obtains/creates target info

Arguments:

    QueueHandle         - Queue we're looking at
    PathStringTable     - String table used for the Target Root/SubDir/Filename strings, NULL if same as QueueHandle's
    TargetPath          - if given, is the full path, previously generated
    TargetRoot          - root portion, eg c:\winnt
    TargetSubDir        - optional sub-directory portion, -1 if not provided
    TargetFilename      - filename , eg readme.txt
    TableID             - filled with ID for future use in pSetupBackupGetTargetByID or pSetupBackupSetTargetByID
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    LONG PathID;
    TCHAR PathBuffer[MAX_PATH];
    PTSTR TmpPtr;
    PVOID LookupTable = NULL;
    PVOID QueueStringTable = NULL;
    PTSTR FullTargetPath = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;
    DWORD RequiredSize;

    Queue = (PSP_FILE_QUEUE)QueueHandle;
    try {
        LookupTable = Queue->TargetLookupTable;  // used for path lookup in source queue
        QueueStringTable = Queue->StringTable;  // used for strings in source queue
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (PathStringTable == NULL) {
        // default string table is that of queue's
        PathStringTable = QueueStringTable;
    }

    if (TargetPath == NULL) {
        // obtain the complete target path and filename (Duplicated String)
        FullTargetPath = pSetupFormFullPath(
                                            PathStringTable,
                                            TargetRoot,
                                            TargetSubDir,
                                            TargetFilename);

        if (!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;
    }

    //
    // normalize path
    //
    RequiredSize = GetFullPathName(TargetPath,
                                   SIZECHARS(PathBuffer),
                                   PathBuffer,
                                   &TmpPtr
                                  );
    //
    // This call should always succeed.
    //
    MYASSERT((RequiredSize > 0) &&
             (RequiredSize < SIZECHARS(PathBuffer)) // RequiredSize doesn't include terminating NULL char
            );

    //
    // Even though we asserted that this should not be the case above,
    // we should handle failure in case asserts are turned off.
    //
    if(!RequiredSize) {
        Err = GetLastError();
        goto clean0;
    } else if(RequiredSize >= SIZECHARS(PathBuffer)) {
        Err = ERROR_BUFFER_OVERFLOW;
        goto clean0;
    }

    PathID = StringTableLookUpStringEx(LookupTable, PathBuffer, 0, TargetInfo, sizeof(SP_TARGET_ENT));
    if (PathID == -1) {
        ZeroMemory(TargetInfo, sizeof(SP_TARGET_ENT));
        if (PathStringTable != QueueStringTable) {
            // need to add entries to Queue's string table if we're using another

            Err = pSetupBackupCopyString(QueueStringTable, &TargetRoot, PathStringTable, TargetRoot);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(QueueStringTable, &TargetSubDir, PathStringTable, TargetSubDir);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(QueueStringTable, &TargetFilename, PathStringTable, TargetFilename);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            PathStringTable = QueueStringTable;
        }
        TargetInfo->TargetRoot = TargetRoot;
        TargetInfo->TargetSubDir = TargetSubDir;
        TargetInfo->TargetFilename = TargetFilename;
        TargetInfo->BackupRoot = -1;
        TargetInfo->BackupSubDir = -1;
        TargetInfo->BackupFilename = -1;
        TargetInfo->NewTargetFilename = -1;
        TargetInfo->InternalFlags = 0;

        PathID = StringTableAddStringEx(LookupTable, PathBuffer, 0, TargetInfo, sizeof(SP_TARGET_ENT));
        if (PathID == -1)
        {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    if (TableID != NULL) {
        *TableID = PathID;
    }

    Err = NO_ERROR;

clean0:
    if (FullTargetPath != NULL) {
        MyFree(FullTargetPath);
    }

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupGetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    OUT PSP_TARGET_ENT  TargetInfo
    )
/*++

Routine Description:

    Given an entry in the LookupTable, gets info

Arguments:

    QueueHandle         - Queue we're looking at
    TableID             - ID relating to string entry we've found (via pSetupBackupGetTargetByPath)
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        LookupTable = Queue->TargetLookupTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (StringTableGetExtraData(LookupTable, TableID, TargetInfo, sizeof(SP_TARGET_ENT)) == FALSE) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupSetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    IN PSP_TARGET_ENT   TargetInfo
    )
/*++

Routine Description:

    Given an entry in the LookupTable, sets info

Arguments:

    QueueHandle         - Queue we're looking at
    TableID             - ID relating to string entry we've found (via pSetupBackupGetTargetByPath)
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        LookupTable = Queue->TargetLookupTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if ( StringTableSetExtraData(LookupTable, TableID, TargetInfo, sizeof(SP_TARGET_ENT)) == FALSE) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupAppendFiles(
    IN HSPFILEQ         TargetQueueHandle,
    IN PCTSTR           BackupSubDir,
    IN DWORD            BackupFlags,
    IN HSPFILEQ         SourceQueueHandle OPTIONAL
    )
/*++

Routine Description:

    This routine will take a list of files from SourceQueueHandle Copy sub-queue's
    These files will appear in the Target Queue's target cache
    And may be placed into the Target Backup Queue
    Typically the copy queue is entries of..
        <oldsrc-root>\<oldsrc-sub>\<oldsrc-name> copied to
        <olddest-path>\<olddest-name>

Arguments:

    TargetQueueHandle   - Where Backups are queued to
    BackupSubDir        - Directory to backup to, relative to backup root
    BackupFlags         - How backup should occur
    SourceQueueHandle   - Handle that has a series of copy operations (backup hint)
                          created, say, by pretending to do the re-install
                          If not specified, only flags are passed

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/
{
    TCHAR BackupPath[MAX_PATH];
    PSP_FILE_QUEUE SourceQueue = NULL;
    PSP_FILE_QUEUE TargetQueue = NULL;
    PSP_FILE_QUEUE_NODE QueueNode = NULL;
    PSOURCE_MEDIA_INFO SourceMediaInfo = NULL;
    BOOL b = TRUE;
    PVOID SourceStringTable = NULL;
    PVOID TargetStringTable = NULL;
    LONG BackupRootID = -1;
    DWORD Err = NO_ERROR;
    LONG PathID = -1;
    SP_TARGET_ENT TargetInfo;

    SourceQueue = (PSP_FILE_QUEUE)SourceQueueHandle; // optional
    TargetQueue = (PSP_FILE_QUEUE)TargetQueueHandle;

    b=TRUE; // set if we can skip this routine
    try {

            TargetStringTable = TargetQueue->StringTable;  // used for strings in target queue

            if (SourceQueue == NULL) {
                b = TRUE; // nothing to do
            } else {
                SourceStringTable = SourceQueue->StringTable;  // used for strings in source queue
                b = (!SourceQueue->CopyNodeCount);
            }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    // these are backup flags to be passed into the queue
    if (BackupFlags & SP_BKFLG_CALLBACK) {
        TargetQueue->Flags |= FQF_BACKUP_AWARE;
    }

    if (b) {
        // nothing to do
        goto clean0;
    }

    //
    // get full directory path of backup - this appears as the "dest" for any backup entries
    //
    if ( BackupSubDir == NULL ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if ( pSetupGetFullBackupPath(BackupPath, BackupSubDir, MAX_PATH,NULL) == FALSE ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // Target will often use this, so we create the ID now instead of later
    //
    BackupRootID = StringTableAddString(TargetStringTable,
                                              BackupPath,
                                              STRTAB_CASE_SENSITIVE);
    if (BackupRootID == -1) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // CopyQueue is split over a number of media's
    // we're not (currently) bothered about media
    // iterate through all the copy sub-queue's
    // and (1) add them to the target lookup table
    // (2) if wanted, add them into the backup queue

    for (SourceMediaInfo=SourceQueue->SourceMediaList; SourceMediaInfo!=NULL ; SourceMediaInfo=SourceMediaInfo->Next) {
        if (!SourceMediaInfo->CopyNodeCount) {
            continue;
        }
        MYASSERT(SourceMediaInfo->CopyQueue);

        for (QueueNode = SourceMediaInfo->CopyQueue; QueueNode!=NULL; QueueNode = QueueNode->Next) {
            // for each "Copy"
            // we want information about the destination path
            //

            Err = pSetupBackupGetTargetByPath(TargetQueueHandle,
                                                    SourceStringTable,
                                                    NULL, // precalculated string
                                                    QueueNode->TargetDirectory,
                                                    -1,
                                                    QueueNode->TargetFilename,
                                                    &PathID,
                                                    &TargetInfo);
            if (Err != NO_ERROR) {
                goto clean0;
            }

            // we now have a created (or obtained) TargetInfo, and PathID
            // provide a source name for backup
            TargetInfo.BackupRoot = BackupRootID;
            Err = pSetupBackupCopyString(TargetStringTable, &TargetInfo.BackupSubDir, SourceStringTable, QueueNode->SourcePath);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(TargetStringTable, &TargetInfo.BackupFilename, SourceStringTable, QueueNode->SourceFilename);
            if (Err != NO_ERROR) {
                goto clean0;
            }

            if ((BackupFlags & SP_BKFLG_LATEBACKUP) == FALSE) {
                // we need to add this item to the backup queue
                Err = pSetupQueueBackupCopy(TargetQueueHandle,
                                      // source
                                      TargetInfo.TargetRoot,
                                      TargetInfo.TargetSubDir,
                                      TargetInfo.TargetFilename,
                                      TargetInfo.BackupRoot,
                                      TargetInfo.BackupSubDir,
                                      TargetInfo.BackupFilename);

                if (Err != NO_ERROR) {
                    goto clean0;
                }
                // flag that we've added it to the pre-copy backup sub-queue
                TargetInfo.InternalFlags |= SP_TEFLG_BACKUPQUEUE;
            }

            // any backups should go to this specified directory
            TargetInfo.InternalFlags |= SP_TEFLG_ORIGNAME;

            Err = pSetupBackupSetTargetByID(TargetQueueHandle, PathID, &TargetInfo);
            if (Err != NO_ERROR) {
                goto clean0;
            }

        }
    }

    Err = NO_ERROR;

clean0:

    SetLastError(Err);
    return (Err);
}




//
// ==========================================================
//

DWORD
pSetupBackupFile(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR TargetPath,
    IN PCTSTR BackupPath,
    IN LONG   TargetID,         OPTIONAL
    IN LONG   TargetRootPath,
    IN LONG   TargetSubDir,
    IN LONG   TargetFilename,
    IN LONG   BackupRootPath,
    IN LONG   BackupSubDir,
    IN LONG   BackupFilename,
    OUT BOOL *InUseFlag
    )
/*++

Routine Description:

    If BackupFilename not supplied, it is obtained/created
    Will either
    1) copy a file to the backup directory, or
    2) queue that a file is backed up on reboot
    The latter occurs if the file was locked.

Arguments:

HSPFILEQ    - QueueHandle   - specifies Queue
LONG        - TargetID      - if specified (not -1), use for target
LONG        - TargetRootPath - used if TargetID == -1
LONG        - TargetSubDir - used if TargetID == -1
LONG        - TargetFilename - used if TargetID == -1
LONG        - BackupRootPath - alternate root (valid if BackupFilename != -1)
LONG        - BackupSubDir - alternate directory (valid if BackupFilename != -1)
LONG        - BackupFilename - alternate filename

Return Value:

    If the function succeeds, return value is TRUE
    If the function fails, return value is FALSE

--*/
{
    PSP_FILE_QUEUE Queue = NULL;
    PVOID StringTable = NULL;
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    SP_TARGET_ENT TargetInfo;
    PTSTR FullTargetPath = NULL;
    PTSTR FullBackupPath = NULL;
    BOOL InUse = FALSE;
    PTSTR TempNamePtr = NULL, DirTruncPos;
    TCHAR TempPath[MAX_PATH];
    TCHAR TempFilename[MAX_PATH];
    TCHAR ParsedPath[MAX_PATH];
    UINT OldMode;
    LONG NewTargetFilename;
    BOOL DoRename = FALSE;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        StringTable = Queue->StringTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if(TargetPath == NULL && TargetID == -1) {

        if (TargetRootPath == -1 || TargetFilename == -1) {
            Err = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        // complete target path

        FullTargetPath = pSetupFormFullPath(
                                           StringTable,
                                           TargetRootPath,
                                           TargetSubDir,
                                           TargetFilename
                                           );

        if (!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;

    }

    if (TargetID == -1) {
        Err = pSetupBackupGetTargetByPath(QueueHandle,
                                                NULL, // string table
                                                TargetPath, // precalculated string
                                                TargetRootPath,
                                                TargetSubDir,
                                                TargetFilename,
                                                &TargetID,
                                                &TargetInfo);
    } else {
        Err = pSetupBackupGetTargetByID(QueueHandle,
                                                TargetID,
                                                &TargetInfo);
    }

    if(Err != NO_ERROR) {
        goto clean0;
    }

    //
    // if we're not interested in backing up (global flag) we can skip
    // but it's only safe to do so if we'd copy & then later throw the copy away on success
    //
    if ((TargetInfo.InternalFlags & SP_TEFLG_RENAMEEXISTING) == 0 && (GlobalSetupFlags & PSPGF_NO_BACKUP)!=0) {
        goto clean0;
    }
    //
    // Figure out whether we've been asked to rename the existing file to a
    // temp name in the same directory, but haven't yet done so.
    //
    DoRename = ((TargetInfo.InternalFlags & (SP_TEFLG_RENAMEEXISTING | SP_TEFLG_MOVED)) == SP_TEFLG_RENAMEEXISTING);

    if(BackupFilename == -1) {
        //
        // non-specific backup
        //
        if((TargetInfo.InternalFlags & SP_TEFLG_SAVED) && !DoRename) {
            //
            // Already backed up, and we don't need to rename the existing file.
            // Nothing to do.
            //
            Err = NO_ERROR;
            goto clean0;
        }

        if(TargetInfo.InternalFlags & SP_TEFLG_INUSE) {
            //
            // Previously marked as INUSE, not allowed to change it.  If we
            // were asked to rename the existing file, then we need to return
            // failure, otherwise, we can report success.
            //
            //
            InUse = TRUE;

            Err = DoRename ? ERROR_SHARING_VIOLATION : NO_ERROR;
            goto clean0;
        }

        if(TargetInfo.InternalFlags & SP_TEFLG_ORIGNAME) {
            //
            // original name given, use that
            //
            BackupRootPath = TargetInfo.BackupRoot;
            BackupSubDir = TargetInfo.BackupSubDir;
            BackupFilename = TargetInfo.BackupFilename;
        }

    } else {
        //
        // We should never be called if the file has already been
        // saved.
        //
        MYASSERT(!(TargetInfo.InternalFlags & SP_TEFLG_SAVED));

        //
        // Even if the above assert fires, we should still deal with
        // the case where this occurs.  Also, we should deal with the
        // case where a backup was previously attempted but failed due
        // to the file being in-use.
        //
        if(TargetInfo.InternalFlags & SP_TEFLG_SAVED) {
            Err = ERROR_INVALID_DATA;
            goto clean0;
        } else if(TargetInfo.InternalFlags & SP_TEFLG_INUSE) {
            //
            // force the issue of InUse
            //
            InUse = TRUE;

            Err = ERROR_SHARING_VIOLATION;
            goto clean0;
        }

        TargetInfo.BackupRoot = BackupRootPath;
        TargetInfo.BackupSubDir = BackupSubDir;
        TargetInfo.BackupFilename = BackupFilename;
        TargetInfo.InternalFlags |= SP_TEFLG_ORIGNAME;
        TargetInfo.InternalFlags &= ~SP_TEFLG_TEMPNAME;
    }

    if(TargetPath == NULL) {
        //
        // must have looked up using TargetID, use TargetInfo to generate TargetPath
        // complete target path
        //
        FullTargetPath = pSetupFormFullPath(StringTable,
                                            TargetInfo.TargetRoot,
                                            TargetInfo.TargetSubDir,
                                            TargetInfo.TargetFilename
                                           );

        if(!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;
    }

    if(DoRename) {
        //
        // We'd better not already have a temp filename stored in our TargetInfo.
        //
        MYASSERT(TargetInfo.NewTargetFilename == -1);

        //
        // First, strip the filename off the path.
        //
        _tcscpy(TempPath, TargetPath);
        TempNamePtr = (PTSTR)MyGetFileTitle(TempPath);
        *TempNamePtr = TEXT('\0');

        //
        // Now get a temp filename within that directory...
        //
        if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
            Err = GetLastError();
            goto clean0;
        }

        //
        // ...and store this path's string ID in our TargetInfo
        //
        NewTargetFilename = StringTableAddString(StringTable,
                                                 TempFilename,
                                                 STRTAB_CASE_SENSITIVE
                                                );

        if(NewTargetFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    if(!(TargetInfo.InternalFlags & (SP_TEFLG_ORIGNAME | SP_TEFLG_TEMPNAME))) {
        //
        // If we don't yet have a name to use in backing up this file, then
        // generate one now.  If we are doing a rename, we can use that name.
        //
        if(DoRename) {
            //
            // Make sure that all flags agree on the fact that we need to back
            // up this file.
            //
            MYASSERT(!(TargetInfo.InternalFlags & SP_TEFLG_SAVED));

            //
            // Temp filename was stored in TempFilename buffer above.
            //
            TempNamePtr = (PTSTR)MyGetFileTitle(TempFilename);

            BackupFilename = StringTableAddString(StringTable, TempNamePtr, STRTAB_CASE_SENSITIVE);
            if(BackupFilename == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            DirTruncPos = CharPrev(TempFilename, TempNamePtr);

            //
            // (We know MyGetFileTitle will never return a pointer to a path
            // separator character, so the following check is valid.)
            //
            if(*DirTruncPos == TEXT('\\')) {
                //
                // If this is in a root directory (e.g., "A:\"), then we don't want to strip off
                // the trailing backslash.
                //
                if(((DirTruncPos - TempFilename) != 2) || (*CharNext(TempFilename) != TEXT(':'))) {
                    TempNamePtr = DirTruncPos;
                }
            }

            lstrcpyn(TempPath, TempFilename, (int)(TempNamePtr - TempFilename) + 1);

            BackupRootPath = StringTableAddString(StringTable, TempPath, STRTAB_CASE_SENSITIVE);
            if(BackupRootPath == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

        } else {

            //
            // specify "NULL" as the sub-directory, since all we want is a temporary location
            //
            if(pSetupGetFullBackupPath(TempPath, NULL, MAX_PATH,NULL) == FALSE ) {
                Err = ERROR_INVALID_HANDLE;
                goto clean0;
            }
            _tcscpy(TempFilename,TempPath);

            //
            // Note:  In the code below, we employ a "trick" to get the
            // pSetupMakeSurePathExists API to make sure that a directory
            // exists.  Since we don't yet know the filename (we need to call
            // GetTempFileName against an existing directory to find that out),
            // we just use a dummy placeholder filename ("OLD") so that it can
            // be discarded by the pSetupMakeSurePathExists API.
            //
            if(ConcatenatePaths(TempFilename, TEXT("OLD"), MAX_PATH, NULL) == FALSE ) {
                Err = GetLastError();
                goto clean0;
            }
            pSetupMakeSurePathExists(TempFilename);
            if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
                Err = GetLastError();
                goto clean0;
            }

            TempNamePtr = TempFilename + _tcslen(TempPath) + 1 /* 1 to skip past \ */;
            BackupRootPath = StringTableAddString( StringTable, TempPath, STRTAB_CASE_SENSITIVE );
            if(BackupRootPath == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
            BackupFilename = StringTableAddString( StringTable, TempNamePtr, STRTAB_CASE_SENSITIVE );
            if(BackupFilename == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
        }

        BackupPath = TempFilename;

        TargetInfo.BackupRoot = BackupRootPath;
        TargetInfo.BackupSubDir = BackupSubDir = -1;
        TargetInfo.BackupFilename = BackupFilename;
        TargetInfo.InternalFlags |= SP_TEFLG_TEMPNAME;

    }


    if(BackupPath == NULL) {
        //
        // make a complete path from this source
        //
        FullBackupPath = pSetupFormFullPath(StringTable,
                                            BackupRootPath,
                                            BackupSubDir,
                                            BackupFilename
                                           );

        if (!FullBackupPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        BackupPath = FullBackupPath;
    }

    //
    // If we need to make a copy of the existing file, do so now.
    //
    if(!DoRename || (TargetInfo.InternalFlags & SP_TEFLG_ORIGNAME)) {

        SetFileAttributes(BackupPath, FILE_ATTRIBUTE_NORMAL);
        pSetupMakeSurePathExists(BackupPath);
        Err = CopyFile(TargetPath, BackupPath, FALSE) ? NO_ERROR : GetLastError();

        if(Err == NO_ERROR) {
            TargetInfo.InternalFlags |= SP_TEFLG_SAVED;
        } else {
            //
            // Delete placeholder file created by GetTempFileName.
            //
            DeleteFile(BackupPath);

            if(Err == ERROR_SHARING_VIOLATION) {
                //
                // Unless we were also going to attempt a rename, don't
                // consider sharing violations to be errors.
                //
                InUse = TRUE;
                TargetInfo.InternalFlags |= SP_TEFLG_INUSE;
                if(!DoRename) {
                    Err = NO_ERROR;
                }
            }
        }
    }

    //
    // OK, now rename the existing file, if necessary.
    //
    if(DoRename && (Err == NO_ERROR)) {

        if(DoMove(TargetPath, TempFilename)) {

            TargetInfo.InternalFlags |= SP_TEFLG_MOVED;
            TargetInfo.NewTargetFilename = NewTargetFilename;

            //
            // Post a delayed deletion for this temp filename so it'll get
            // cleaned up after reboot.
            //
            if(!PostDelayedMove(Queue, TempFilename, NULL, -1, FALSE)) {
                //
                // Don't abort just because we couldn't schedule a delayed
                // delete.  If this fails, the only bad thing that will happen
                // is a turd will get left over after reboot.
                //
                // We should log an event about this, however.
                //
                Err = GetLastError();

                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_RENAME_EXISTING_DELAYED_DELETE_FAILED,
                              NULL,
                              TargetPath,
                              TempFilename
                             );

                WriteLogError(Queue->LogContext,
                              SETUP_LOG_WARNING,
                              Err
                             );

                Err = NO_ERROR;
            }

        } else {
            Err = GetLastError();
            DeleteFile(TempFilename);
            if(Err == ERROR_SHARING_VIOLATION) {
                InUse = TRUE;
                TargetInfo.InternalFlags |= SP_TEFLG_INUSE;
            }
        }
    }

    //
    // update internal info (this call should never fail)
    //
    pSetupBackupSetTargetByID(QueueHandle,
                              TargetID,
                              &TargetInfo
                             );

clean0:

    if(InUseFlag) {
        *InUseFlag = InUse;
    }

    if (FullTargetPath != NULL) {
        MyFree(FullTargetPath);
    }
    if (FullBackupPath != NULL) {
        MyFree(FullBackupPath);
    }

    SetErrorMode(OldMode);

    SetLastError(Err);

    return Err;

}

//
// ==========================================================
//

BOOL
pSetupRemoveBackupDirectory(
    IN PCTSTR           BackupDir
    )
/*++

Routine Description:

    Deletes all the backups out of a directory
    Where BackupDir is the same format as in pSetupOpenBackup

Arguments:

    BackupDir - Directory relative to the backup directory, or absolute

Return Value:

    If the function succeeds, return value is TRUE
    If the function fails, return value is FALSE

--*/
{
    TCHAR Buffer[MAX_PATH];
    //
    // get full directory path
    //
    if ( BackupDir == NULL ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ( pSetupGetFullBackupPath(Buffer, BackupDir, MAX_PATH,NULL) == FALSE ) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return FALSE;
}


//
// ==========================================================
//

DWORD
pSetupGetCurrentlyInstalledDriverNode(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Get driver node that relates to current INF file of device

Arguments:

    DeviceInfoSet
    DeviceInfoData

Return Value:

    Error Status

--*/
{
    HKEY hKey = NULL;
    DWORD Err;
    DWORD RegDataType, RegDataLength;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;

    //
    // Retrieve the current device install parameters, in preparation for modifying them to
    // target driver search at a particular INF.
    //
    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
        return GetLastError();
    }

    //
    // Open the device's driver key and retrieve the INF from which the device was installed.
    //
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ
                               );

    if(hKey == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }

    RegDataLength = sizeof(DeviceInstallParams.DriverPath); // want in bytes, not chars
    Err = RegQueryValueEx(hKey,
                          REGSTR_VAL_INFPATH,
                          NULL,
                          &RegDataType,
                          (PBYTE)DeviceInstallParams.DriverPath,
                          &RegDataLength
                         );

    if((Err == ERROR_SUCCESS) && (RegDataType != REG_SZ)) {
        Err = ERROR_INVALID_DATA;
    }

    if(Err != ERROR_SUCCESS) {
        goto clean0;
    }

    //
    // set the flag that indicates DriverPath represents a single INF to be searched (and
    // not a directory path).  Then store the parameters back to the device information element.
    //
    DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if(!SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now build a class driver list from this INF.
    //
    if(!SetupDiBuildDriverInfoList(DeviceInfoSet, DeviceInfoData, SPDIT_CLASSDRIVER)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // OK, now select the driver node from that INF that was used to install this device.
    // The three parameters that uniquely identify a driver node are INF Provider,
    // Device Manufacturer, and Device Description.  Retrieve these three pieces of information
    // in preparation for selecting the proper driver node in the class list we just built.
    //
    // First, retrieve the INF Provider.  This is stored in the device's driver key (which we
    // already have open).  The provider name is the only one of the three parameters that the
    // INF may omit (in that case, the ProviderName value entry won't be present in the driver
    // key).
    //
    RegDataLength = sizeof(DriverInfoData.ProviderName);        // want in bytes, not chars
    Err = RegQueryValueEx(hKey,
                          REGSTR_VAL_PROVIDER_NAME,
                          NULL,
                          &RegDataType,
                          (PBYTE)DriverInfoData.ProviderName,
                          &RegDataLength
                         );

    if(Err == ERROR_SUCCESS) {

        if(RegDataType != REG_SZ) {
            Err = ERROR_INVALID_DATA;
            goto clean0;
        }

    } else {
        //
        // Assume there is no provider specified.  If it turns out that the registry query
        // really failed for some other reason, we'll discover that soon enough when we attempt
        // to select the driver node.
        //
        Err = ERROR_SUCCESS;
        *DriverInfoData.ProviderName = TEXT('\0');
    }

    //
    // Next, retrieve the manufacturer (stored in the Mfg device property).
    //
    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_MFG,
                                         NULL,      // datatype is guaranteed to always be REG_SZ.
                                         (PBYTE)DriverInfoData.MfgName,
                                         sizeof(DriverInfoData.MfgName),    // in bytes
                                         NULL)) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // Finally, retrieve the device description (stored in the DeviceDesc device property).
    //
    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,      // datatype is guaranteed to always be REG_SZ.
                                         (PBYTE)DriverInfoData.Description,
                                         sizeof(DriverInfoData.Description),    // in bytes
                                         NULL)) {

        Err = GetLastError();
        goto clean0;
    }

    //
    // Do the final initialization on the driver info data 'template' buffer used in doing the
    // selection search.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
    DriverInfoData.Reserved = 0;  // Search for the driver matching the specified criteria and
                                  // select it if found.

    if(!SetupDiSetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // At this point, we've successfully selected the currently installed driver for the specified
    // device information element.  We're done!
    //

    Err = NO_ERROR;

clean0:
    RegCloseKey(hKey);

    SetLastError(Err);
    return Err;
}


//
// ==========================================================
//

DWORD
pSetupGetBackupQueue(
    IN      PCTSTR      DeviceID,
    IN OUT  HSPFILEQ    FileQueue,
    IN      DWORD       BackupFlags
    )
/*++

Routine Description:

    Creates a backup Queue for current device (DeviceID)
    Also makes sure that the INF file is backed up

Arguments:

    DeviceID            String ID of device
    FileQueue           Backup queue is filled with files that need copying
    BackupFlags         Various flags

Return Value:

    Error Status

--*/


{

    //
    // we want to obtain a copy/move list of device associated with DeviceID
    //
    //
    PSP_FILE_QUEUE FileQ = (PSP_FILE_QUEUE)FileQueue;
    HDEVINFO TempInfoSet = (HDEVINFO)INVALID_HANDLE_VALUE;
    HSPFILEQ TempQueueHandle = (HSPFILEQ)INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA TempInfoData;
    SP_DEVINSTALL_PARAMS TempParams;
    TCHAR SubDir[MAX_DEVICE_ID_LEN + sizeof(SP_BACKUP_DRIVERFILES) + 16];
    LONG Instance;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PTSTR szInfFileName = NULL;
    TCHAR BackupPath[MAX_PATH];
    TCHAR OemOrigName[MAX_PATH];
    TCHAR CatBackupPath[MAX_PATH];
    TCHAR CatSourcePath[MAX_PATH];
    DWORD Err;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    int c;
    DWORD BackupInfID = -1;
    PSP_INF_INFORMATION pInfInformation = NULL;
    DWORD InfInformationSize;
    SP_ORIGINAL_FILE_INFO InfOriginalFileInformation;
    BOOL success;

    CatBackupPath[0] = 0; // by default, don't bother with a catalog
    CatSourcePath[0] = 0;

    // pretend we're installing old INF
    // this gives us a list of files we need

    TempInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if ( TempInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE ) {
        Err = GetLastError();
        goto clean0;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(TempInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // Open the driver info, related to DeviceID I was given
    //

    TempInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if ( SetupDiOpenDeviceInfo(TempInfoSet ,DeviceID, NULL, 0, &TempInfoData) == FALSE ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Get the currently-installed driver node selected for this element.
    //
    if ( pSetupGetCurrentlyInstalledDriverNode(TempInfoSet, &TempInfoData) != NO_ERROR ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now queue all files to be copied by this driver node into our own file queue.
    //
    TempQueueHandle = SetupOpenFileQueue();

    if ( TempQueueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        //
        // SetupOpenFileQueue modified to return error
        //
        Err = GetLastError();
        goto clean0;
    }

    TempParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(TempInfoSet, &TempInfoData, &TempParams) ) {
        Err = GetLastError();
        goto clean0;
    }

    TempParams.FileQueue = TempQueueHandle;
    TempParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(TempInfoSet, &TempInfoData, &TempParams) ) {
        Err = GetLastError();
        goto clean0;
    }

    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, TempInfoSet, &TempInfoData) ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // need a directory - replace "bad" characters with others
    //
    for(c=0; DeviceID[c]; c++)
    {
        if(DeviceID[c] == TEXT('\\') || DeviceID[c] == TEXT('/'))
        {
            SubDir[c] = TEXT('#');
        }
        else if(DeviceID[c] == TEXT('*'))
        {
            SubDir[c] = TEXT('~');
        }
        else
        {
            SubDir[c] = DeviceID[c];
        }
    }

    //
    // insert into this path an instance number and INF dir
    //
    Instance = SP_BACKUP_INSTANCE0;
    wsprintf(SubDir+c, TEXT("\\%04d\\%s"), (LONG) Instance, (PCTSTR) SP_BACKUP_DRIVERFILES );

    //
    // get the path of the INF file, we will need to back it up
    //
    if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                 &TempInfoData,
                                                 NULL))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }
    szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                             DevInfoElem->SelectedDriver->InfFileName
                                            );
    if (szInfFileName == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // we want to get the "real" name of the INF - we may have a precompiled inf
    // BUGBUG (jamiehun) we could check the tail-name first for OEMxxx.INF
    //

    ZeroMemory(&InfOriginalFileInformation, sizeof(InfOriginalFileInformation));

    //
    // if nothing else, use same name as is in the INF directory
    //
    lstrcpy(OemOrigName,MyGetFileTitle(szInfFileName));


    //
    // create the path of the new inf we want to copy to - hopefully we've managed to preserve the original inf name
    //

    if ( pSetupGetFullBackupPath(BackupPath, SubDir, MAX_PATH,NULL) == FALSE ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // but use the original name if available
    //
    InfInformationSize = 8192;  // I'd rather have this too big and succeed first time, than read the INF twice
    pInfInformation = (PSP_INF_INFORMATION)MyMalloc(InfInformationSize);

    if (pInfInformation != NULL) {
        success = SetupGetInfInformation(szInfFileName,INFINFO_INF_NAME_IS_ABSOLUTE,pInfInformation,InfInformationSize,&InfInformationSize);
        if (!success && GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
            PVOID newbuff = MyRealloc(pInfInformation,InfInformationSize);
            if (!newbuff) {
                MyFree(pInfInformation);
                pInfInformation = NULL;
            } else {
                pInfInformation = (PSP_INF_INFORMATION)newbuff;
                success = SetupGetInfInformation(szInfFileName,INFINFO_INF_NAME_IS_ABSOLUTE,pInfInformation,InfInformationSize,&InfInformationSize);
            }
        }
        if (success) {
            InfOriginalFileInformation.cbSize = sizeof(InfOriginalFileInformation);
            if (SetupQueryInfOriginalFileInformation(pInfInformation,0,NULL,&InfOriginalFileInformation)) {
                if (InfOriginalFileInformation.OriginalInfName[0]) {
                    //
                    // we have a "real" inf name
                    //
                    lstrcpy(OemOrigName,MyGetFileTitle(InfOriginalFileInformation.OriginalInfName));
                } else {
                    MYASSERT(InfOriginalFileInformation.OriginalInfName[0]);
                }
                if (InfOriginalFileInformation.OriginalCatalogName[0]) {

                    TCHAR CurrentCatName[MAX_PATH];

                    //
                    // given that the file is ....\OEMx.INF the catalog is "OEMx.CAT"
                    // we key off OemOrigName (eg mydisk.inf )
                    // and we won't bother copying the catalog if we can't verify the inf
                    //
                    lstrcpy(CurrentCatName,MyGetFileTitle(szInfFileName));
                    lstrcpy(_tcsrchr(CurrentCatName, TEXT('.')), pszCatSuffix);

                    //
                    // we have a catalog name
                    // now consider making a copy of the cached catalog into our backup
                    // we get out CatProblem and szCatFileName
                    //
                    // if all seems ok, copy file from szCatFileName to backupdir\OriginalCatalogName
                    //

                    Err = VerifyFile(FileQ->LogContext,
                                    CurrentCatName,     // eg "OEMx.CAT"
                                    NULL,0,             // we're not verifying against another catalog image
                                    OemOrigName,        // eg "mydisk.inf"
                                    szInfFileName,      // eg "....\OEMx.INF"
                                    NULL,               // return: problem info
                                    NULL,               // return: problem file
                                    TRUE,               // assume catalog is self-verified
                                    NULL,               // alt platform info
                                    CatSourcePath,      // return: catalog file, full path
                                    NULL                // return: number of catalogs considered
                                    );
                    if (Err == NO_ERROR && CatSourcePath[0]) {
                        //
                        // we have a catalog file of interest to copy
                        //
                        lstrcpy(CatBackupPath,BackupPath);
                        if (!ConcatenatePaths(CatBackupPath, InfOriginalFileInformation.OriginalCatalogName, MAX_PATH, NULL)) {
                            //
                            // non-fatal
                            //
                            CatSourcePath[0]=0;
                            CatBackupPath[0]=0;
                        }
                    }
                }
            }
        }
        if (pInfInformation != NULL) {
            MyFree(pInfInformation);
            pInfInformation = NULL;
        }
    }
    if ( ConcatenatePaths(BackupPath, OemOrigName, MAX_PATH, NULL) == FALSE ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    SetFileAttributes(BackupPath,FILE_ATTRIBUTE_NORMAL);
    pSetupMakeSurePathExists(BackupPath);
    Err = CopyFile(szInfFileName, BackupPath ,FALSE) ? NO_ERROR : GetLastError();

    if (Err != NO_ERROR) {
        goto clean0;
    }

    if(CatSourcePath[0] && CatBackupPath[0]) {
        //
        // if we copied Inf file, try to copy catalog file
        // if we don't succeed, don't consider this a fatal error
        //
        CopyFile(CatSourcePath, CatBackupPath ,FALSE);
    }

    //
    // Add string indicating Backup INF location to file Queue
    // for later retrieval
    //

    BackupInfID = StringTableAddString(FileQ->StringTable,
                                              BackupPath,
                                              STRTAB_CASE_SENSITIVE);
    if (BackupInfID == -1) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Also backup the PNF file
    //
    // WARNING: We reuse the szInfFileName and BackupPath variables at this point
    // so if you add code that needs them then add it above.
    //
    if ((lstrlen(szInfFileName) > lstrlen(DISTR_INF_PNF_SUFFIX)) &&
        (lstrlen(BackupPath) > lstrlen(DISTR_INF_PNF_SUFFIX))) {

        //
        // Just replace the szInfFileName and BackupPath .INF suffix with .PNF and call
        // CopyFile.
        //
        lstrcpy(&szInfFileName[lstrlen(szInfFileName) - lstrlen(DISTR_INF_PNF_SUFFIX)],
                DISTR_INF_PNF_SUFFIX);
        lstrcpy(&BackupPath[lstrlen(BackupPath) - lstrlen(DISTR_INF_PNF_SUFFIX)],
                DISTR_INF_PNF_SUFFIX);

        CopyFile(szInfFileName, BackupPath, FALSE);
    }

    //
    // update BackupInfID so that INF name can be queried later
    //
    FileQ->BackupInfID = BackupInfID;

    //
    // add items we may need to backup
    // (ie the copy queue of TempQueueHandle is converted to a backup queue of FileQueue)
    //

    if ( pSetupBackupAppendFiles(FileQueue, SubDir, BackupFlags, TempQueueHandle) != NO_ERROR ) {
        Err = GetLastError();
        goto clean0;
    }

    Err = NO_ERROR;

clean0:

    // delete temporary structures used
    if (pDeviceInfoSet != NULL ) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }
    if ( TempInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE ) {
        SetupDiDestroyDeviceInfoList(TempInfoSet);
    }
    if ( TempQueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        SetupCloseFileQueue(TempQueueHandle);
    }

    SetLastError(Err);

    return Err;
}

//
// ==========================================================
//

BOOL
PostDelayedMove(
                IN PSP_FILE_QUEUE    Queue,
                IN PCTSTR CurrentName,
                IN PCTSTR NewName,       OPTIONAL
                IN DWORD SecurityDesc,
                IN BOOL TargetIsProtected
                )
/*++

Routine Description:

    Helper for DelayedMove
    We don't do any delayed Moves until we know all else succeeded

Arguments:

    Queue               Queue that the move is applied to
    CurrentName         Name of file we want to move
    NewName             Name we want to move to
    SecurityDesc        Index in string table of Security Descriptor string or -1 if not present
    TargetIsProtected   Indicates whether target file is a protected system file

Return Value:

    FALSE if error

--*/
{
    PSP_DELAYMOVE_NODE DelayMoveNode;
    LONG SourceFilename;
    LONG TargetFilename;
    DWORD Err;

    if (CurrentName == NULL) {
        SourceFilename = -1;
    } else {
        SourceFilename = StringTableAddString(Queue->StringTable,
                                                (PTSTR)CurrentName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        if (SourceFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }
    if (NewName == NULL) {
        TargetFilename = -1;
    } else {
        TargetFilename = StringTableAddString(Queue->StringTable,
                                                (PTSTR)NewName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        if (TargetFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    DelayMoveNode = MyMalloc(sizeof(SP_DELAYMOVE_NODE));

    if (DelayMoveNode == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    DelayMoveNode->NextNode = NULL;
    DelayMoveNode->SourceFilename = SourceFilename;
    DelayMoveNode->TargetFilename = TargetFilename;
    DelayMoveNode->SecurityDesc = SecurityDesc;
    DelayMoveNode->TargetIsProtected = TargetIsProtected;

    if (Queue->DelayMoveQueueTail == NULL) {
        Queue->DelayMoveQueue = DelayMoveNode;
    } else {
        Queue->DelayMoveQueueTail->NextNode = DelayMoveNode;
    }
    Queue->DelayMoveQueueTail = DelayMoveNode;

    Err = NO_ERROR;

clean0:

    SetLastError(Err);

    return (Err == NO_ERROR);

}

//
// ==========================================================
//

DWORD
DoAllDelayedMoves(
    IN PSP_FILE_QUEUE    Queue
    )
/*++

Routine Description:

    Execute the Delayed Moves previously posted

Arguments:

    Queue               Queue that has the list in

Return Value:

    Error Status

--*/
{
    PSP_DELAYMOVE_NODE DelayMoveNode;
    PTSTR CurrentName;
    PTSTR TargetName;
    BOOL b = TRUE;
    PSP_DELAYMOVE_NODE DoneQueue = NULL;
    PSP_DELAYMOVE_NODE NextNode = NULL;
    DWORD Err = NO_ERROR;
    BOOL EnableProtectedRenames = FALSE;

    for (DelayMoveNode = Queue->DelayMoveQueue ; DelayMoveNode ; DelayMoveNode = NextNode ) {
        NextNode = DelayMoveNode->NextNode;

        MYASSERT(DelayMoveNode->SourceFilename != -1);
        CurrentName = StringTableStringFromId(Queue->StringTable, DelayMoveNode->SourceFilename);
        MYASSERT(CurrentName);

        if (DelayMoveNode->TargetFilename == -1) {
            TargetName = NULL;
        } else {
            TargetName = StringTableStringFromId( Queue->StringTable, DelayMoveNode->TargetFilename );
            MYASSERT(TargetName);
        }

        //
        // Keep track of whether we've encountered any protected system files.
        //
        EnableProtectedRenames |= DelayMoveNode->TargetIsProtected;

#ifdef UNICODE
        //
        // If this is a move (instead of a delete), then set security (letting
        // SCE know what the file's final name will be.
        //
        if((DelayMoveNode->SecurityDesc != -1) && TargetName) {

            Err = pSetupCallSCE(ST_SCE_RENAME,
                                CurrentName,
                                Queue,
                                TargetName,
                                DelayMoveNode->SecurityDesc,
                                NULL
                               );

            if(Err != NO_ERROR ){
                //
                // If we're on the first delay-move node, then we can abort.
                // However, if we've already processed one or more nodes, then
                // we can't abort--we must simply log an error indicating what
                // happened and keep on going.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_MOVE_SCE_FAILED,
                              NULL,
                              CurrentName,
                              TargetName
                             );

                WriteLogError(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              Err
                             );

                if(DelayMoveNode == Queue->DelayMoveQueue) {
                    //
                    // Failure occurred on 1st node--we can abort.
                    //
                    WriteLogEntry(Queue->LogContext,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_OPERATION_CANCELLED,
                                  NULL
                                 );
                    break;
                } else {
                    //
                    // There's no turning back--log an error and keep on going.
                    //
                    WriteLogEntry(Queue->LogContext,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_ERROR_IGNORED,
                                  NULL
                                 );

                    Err = NO_ERROR;
                }
            }

        } else
#endif
        {
            Err = NO_ERROR;
        }

        //
        // finally delay the move
        //
        if(!DelayedMove(CurrentName, TargetName)) {

            Err = GetLastError();

            //
            // Same deal as above with SCE call--if we've already processed one
            // or more delay-move nodes, we can't abort.
            //
            if(TargetName) {
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_MOVE_SCE_FAILED,
                              NULL,
                              CurrentName,
                              TargetName
                             );
            } else {
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_DELETE_FAILED,
                              NULL,
                              CurrentName
                             );
            }

            WriteLogError(Queue->LogContext,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          Err
                         );

            if(DelayMoveNode == Queue->DelayMoveQueue) {
                //
                // Failure occurred on 1st node--we can abort.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_OPERATION_CANCELLED,
                              NULL
                             );
                break;
            } else {
                //
                // There's no turning back--log an error and keep on going.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_ERROR_IGNORED,
                              NULL
                             );

                Err = NO_ERROR;
            }
        }

        //
        // Move node to queue containing nodes that have already been processed
        //
        DelayMoveNode->NextNode = DoneQueue;
        DoneQueue = DelayMoveNode;
    }

    //
    // If we have any replacement of protected system files, then we need to
    // inform session manager so that it allows the replacement to occur upon
    // reboot.
    //
    // NOTE: We don't have to worry about enabling replacement of system files
    // with unsigned (hence, untrusted) versions.  We only queue up unsigned
    // system files for replacement if the user was explicitly warned of (and
    // agreed to) the consequences.
    //
    // BugBug: the session manager only allows the granularity of "allow all
    //         renames" or "allow no renames".  If Err != NO_ERROR, then we
    //         might want to clear out this flag, but that means we'd negate
    //         any renames that were previously allowed.  Yuck.  So we flip a
    //         coin, decide to do nothing, and hope for the best if an error
    //         occurred.  We have similar situation above -- it's all or
    //         nothing.
    //
    if((Err == NO_ERROR) && EnableProtectedRenames) {
        pSetupProtectedRenamesFlag(TRUE);
    }

    //
    // any nodes that are left are dropped
    //
    for ( ; DelayMoveNode ; DelayMoveNode = NextNode ) {
        NextNode = DelayMoveNode->NextNode;

        MyFree(DelayMoveNode);
    }
    Queue->DelayMoveQueue = NULL;
    Queue->DelayMoveQueueTail = NULL;

    //
    // delete all nodes we queue'd
    //
    for ( ; DoneQueue ; DoneQueue = NextNode ) {
        NextNode = DoneQueue->NextNode;
        //
        // done with node
        //
        MyFree(DoneQueue);
    }

    return Err;
}

//
// ==========================================================
//

VOID
pSetupUnwindAll(
    IN PSP_FILE_QUEUE    Queue,
    IN BOOL              Succeeded
    )
/*++

Routine Description:

    Processes the Unwind Queue. If Succeeded is FALSE, restores any data that was backed up

Arguments:

    Queue               Queue to be unwound
    Succeeded           Indicates if we should treat the whole operation as succeeded or failed

Return Value:

    None--this routine should always succeed.  (Any file errors encountered
    along the way are logged in the setupapi logfile.)

--*/

{
    // if Succeeded, we need to delete Temp files
    // if we didn't succeed, we need to restore backups

    PSP_UNWIND_NODE UnwindNode;
    PSP_UNWIND_NODE ThisNode;
    SP_TARGET_ENT TargetInfo;
    PTSTR BackupFilename;
    PTSTR TargetFilename;
    PTSTR RenamedFilename;
    DWORD Err = NO_ERROR;
    TCHAR TempPath[MAX_PATH];
    PTSTR TempNamePtr;
    TCHAR TempFilename[MAX_PATH];
    BOOL  RestoreByRenaming;
    BOOL  OkToDeleteBackup;

    if (Succeeded == FALSE) {
        //
        // we need to restore backups
        //

        WriteLogEntry(
            Queue->LogContext,
            SETUP_LOG_WARNING,
            MSG_LOG_UNWIND,
            NULL);

        for ( UnwindNode = Queue->UnwindQueue; UnwindNode != NULL; ) {
            ThisNode = UnwindNode;
            UnwindNode = UnwindNode->NextNode;

            if (pSetupBackupGetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo) == NO_ERROR) {


                BackupFilename = NULL;
                TargetFilename = NULL;
                RenamedFilename = NULL;

                // restore backup
                if(!(TargetInfo.InternalFlags & SP_TEFLG_RESTORED)) {

                    // get target name
                    TargetFilename = pSetupFormFullPath(
                                        Queue->StringTable,
                                        TargetInfo.TargetRoot,
                                        TargetInfo.TargetSubDir,
                                        TargetInfo.TargetFilename);

                    if(TargetInfo.InternalFlags & SP_TEFLG_MOVED) {
                        //
                        // Get renamed filename
                        //
                        RenamedFilename = StringTableStringFromId(Queue->StringTable,
                                                                  TargetInfo.NewTargetFilename
                                                                 );
                    }

                    if(TargetInfo.InternalFlags & SP_TEFLG_SAVED) {
                        //
                        // get backup name
                        //
                        BackupFilename = pSetupFormFullPath(
                                            Queue->StringTable,
                                            TargetInfo.BackupRoot,
                                            TargetInfo.BackupSubDir,
                                            TargetInfo.BackupFilename);

                    }
                }

                if(TargetFilename && (RenamedFilename || BackupFilename)) {
                    //
                    // We either renamed the original file or we backed it up.
                    // We need to put it back.
                    //
                    RestoreByRenaming = RenamedFilename ? TRUE : FALSE;

                    RestoreRenamedOrBackedUpFile(TargetFilename,
                                                 (RestoreByRenaming
                                                    ? RenamedFilename
                                                    : BackupFilename),
                                                 RestoreByRenaming,
                                                 Queue->LogContext
                                                );

                    //
                    // If we were doing a copy (i.e., from a backup) as opposed
                    // to a rename, then we need to reapply timestamp and
                    // security.
                    //
                    if(!RestoreByRenaming) {

                        Err = GetSetFileTimestamp(TargetFilename,
                                                  &(ThisNode->CreateTime),
                                                  &(ThisNode->AccessTime),
                                                  &(ThisNode->WriteTime),
                                                  TRUE
                                                 );

                        if(Err != NO_ERROR) {
                            //
                            // We just blew away the timestamp on the file--log
                            // an error entry to that effect.
                            //
                            WriteLogEntry(Queue->LogContext,
                                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                          MSG_LOG_BACKUP_EXISTING_RESTORE_FILETIME_FAILED,
                                          NULL,
                                          TargetFilename
                                         );

                            WriteLogError(Queue->LogContext,
                                          SETUP_LOG_ERROR,
                                          Err
                                         );
                        }

                        if(ThisNode->SecurityDesc != NULL){

                            Err = StampFileSecurity(TargetFilename, ThisNode->SecurityDesc);

                            if(Err != NO_ERROR) {
                                //
                                // We just blew away the existing security on
                                // the file--log an error entry to that effect.
                                //
                                WriteLogEntry(Queue->LogContext,
                                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                              MSG_LOG_BACKUP_EXISTING_RESTORE_SECURITY_FAILED,
                                              NULL,
                                              TargetFilename
                                             );

                                WriteLogError(Queue->LogContext,
                                              SETUP_LOG_ERROR,
                                              Err
                                             );
                            }
#ifdef UNICODE
                            Err = pSetupCallSCE(ST_SCE_UNWIND,
                                                TargetFilename,
                                                NULL,
                                                NULL,
                                                -1,
                                                ThisNode->SecurityDesc
                                               );

                            if(Err != NO_ERROR) {
                                //
                                // We just blew away the existing security on
                                // the file--log an error entry to that effect.
                                //
                                WriteLogEntry(Queue->LogContext,
                                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                              MSG_LOG_BACKUP_EXISTING_RESTORE_SCE_FAILED,
                                              NULL,
                                              TargetFilename
                                             );

                                WriteLogError(Queue->LogContext,
                                              SETUP_LOG_ERROR,
                                              Err
                                             );
                            }
#endif
                        }
                    }

                    //
                    // Now mark that we've restored this file.  We'll delete
                    // tempfiles later
                    //
                    TargetInfo.InternalFlags |= SP_TEFLG_RESTORED;
                    pSetupBackupSetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo);
                }

                if(BackupFilename) {
                    MyFree(BackupFilename);
                }
                if(TargetFilename) {
                    MyFree(TargetFilename);
                }
            }
        }
    }

    //
    // cleanup - remove temporary files
    //
    for ( UnwindNode = Queue->UnwindQueue; UnwindNode != NULL; ) {
        ThisNode = UnwindNode;
        UnwindNode = UnwindNode->NextNode;

        if (pSetupBackupGetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo) == NO_ERROR) {
            // delete temporary file
            if (TargetInfo.InternalFlags & SP_TEFLG_TEMPNAME) {
                //
                // get name of file that was used for backup
                //
                BackupFilename = pSetupFormFullPath(
                                    Queue->StringTable,
                                    TargetInfo.BackupRoot,
                                    TargetInfo.BackupSubDir,
                                    TargetInfo.BackupFilename);

                if(BackupFilename) {
                    //
                    // If this operation was a bootfile replacement, then we
                    // don't want to delete the backup (if we used the renamed
                    // file for the backup as well).  A delayed delete will
                    // have been queued to get rid of the file after a reboot.
                    //
                    OkToDeleteBackup = TRUE;

                    if(TargetInfo.InternalFlags & SP_TEFLG_MOVED) {
                        //
                        // Retrieve the renamed filename to see if it's the
                        // same as the backup filename.
                        //
                        RenamedFilename = StringTableStringFromId(Queue->StringTable,
                                                                  TargetInfo.NewTargetFilename
                                                                 );

                        if(!lstrcmpi(BackupFilename, RenamedFilename)) {
                            OkToDeleteBackup = FALSE;
                        }
                    }

                    if(OkToDeleteBackup) {
                        //
                        // since it was temporary, delete it
                        //
                        if(!DeleteFile(BackupFilename)) {
                            //
                            // Alright, see if we can set it up for delayed delete
                            // instead.
                            //
                            if(!DelayedMove(BackupFilename, NULL)) {
                                //
                                // Oh well, just write a log entry indicating that
                                // this file turd was left on the user's disk.
                                //
                                Err = GetLastError();

                                WriteLogEntry(Queue->LogContext,
                                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                              MSG_LOG_BACKUP_DELAYED_DELETE_FAILED,
                                              NULL,
                                              BackupFilename
                                             );

                                WriteLogError(Queue->LogContext,
                                              SETUP_LOG_WARNING,
                                              Err
                                             );
                            }
                        }
                    }

                    MyFree(BackupFilename);
                }
                TargetInfo.InternalFlags = 0; // entry is now invalid
                pSetupBackupSetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo);
            }
        }

        // cleanup node
        if (ThisNode->SecurityDesc != NULL) {
            MyFree(ThisNode->SecurityDesc);
        }
        MyFree(ThisNode);
    }

    Queue->UnwindQueue = NULL;
}

//
// ==========================================================
//

DWORD _SetupGetBackupInformation(
         IN     PSP_FILE_QUEUE               Queue,
         OUT    PSP_BACKUP_QUEUE_PARAMS      BackupParams
         )
/*++

Routine Description:

    Get Backup INF path - Internal version

Arguments:

    Queue - pointer to queue structure (validated)
    BackupParams OUT - filled with INF file path

Return Value:

    TRUE if success, else FALSE

--*/
{
    //
    // Queue is assumed to have been validated
    // BackupParams is in Native format
    //

    LONG BackupInfID;
    ULONG BufSize = MAX_PATH;
    BOOL b;
    DWORD err = NO_ERROR;
    LPCTSTR filename;
    INT offset;

    BackupInfID = Queue->BackupInfID;

    if (BackupInfID>=0) {
        //
        // get inf from stringtable
        //
        b = StringTableStringFromIdEx(Queue->StringTable,
                                    BackupInfID,
                                    BackupParams->FullInfPath,
                                    &BufSize);
        if (b == FALSE) {
            if (BufSize == 0) {
                err = ERROR_NO_BACKUP;
            } else {
                err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto Clean0;
        }
        //
        // find index of filename
        //
        filename = MyGetFileTitle(BackupParams->FullInfPath);
        offset = (INT)(filename - BackupParams->FullInfPath);
        BackupParams->FilenameOffset = offset;

    } else {
        //
        // no backup path
        //
        err = ERROR_NO_BACKUP;
    }

Clean0:

    return err;
}




#ifdef UNICODE
//
// ANSI version in UNICODE
//
BOOL
WINAPI
SetupGetBackupInformationA(
    IN     HSPFILEQ                     QueueHandle,
    OUT    PSP_BACKUP_QUEUE_PARAMS_A    BackupParams
    )
{
    BOOL b;
    int i;
    INT c;
    LPCSTR p;
    SP_BACKUP_QUEUE_PARAMS_W BackupParamsW;

    //
    // confirm structure size
    //

    try {
        if(BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS_A)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            leave;              // exit try block
        }
        //
        // call Unicode version of API
        //
        ZeroMemory( &BackupParamsW, sizeof(BackupParamsW) );
        BackupParamsW.cbSize = sizeof(BackupParamsW);

        b = SetupGetBackupInformationW(QueueHandle,&BackupParamsW);
        if (b) {
            //
            // success, convert structure from UNICODE to ANSI
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    BackupParamsW.FullInfPath,
                    MAX_PATH,
                    BackupParams->FullInfPath,
                    MAX_PATH,
                    NULL,
                    NULL
                    );
            if (i==0) {
                //
                // error occurred (LastError set to error)
                //
                b = FALSE;
                leave;              // exit try block
            }

            //
            // we need to recalc the offset of INF filename
            // taking care of internationalization
            //
            p = BackupParams->FullInfPath;
            for(c = 0; c < BackupParamsW.FilenameOffset; c++) {
                p = CharNextA(p);
            }
            BackupParams->FilenameOffset = (int)(p-(BackupParams->FullInfPath));  // new offset in ANSI
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          SetLastError(ERROR_INVALID_PARAMETER);
          b = FALSE;
    }

    return b;
}

#else
//
// Unicode version in ANSI
//
BOOL
WINAPI
SetupGetBackupInformationW(
   IN     HSPFILEQ                     QueueHandle,
   OUT    PSP_BACKUP_QUEUE_PARAMS_W    BackupParams
   )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(BackupParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

//
// Native version
//
BOOL
WINAPI
SetupGetBackupInformation(
    IN     HSPFILEQ                     QueueHandle,
    OUT    PSP_BACKUP_QUEUE_PARAMS      BackupParams
    )
/*++

Routine Description:

    Get Backup INF path

Arguments:

    QueueHandle - handle of queue to retrieve backup INF file from
    BackupParams - IN - has cbSize set, OUT - filled with INF file path

Return Value:

    TRUE if success, else FALSE

--*/
{
    BOOL b = TRUE;
    PSP_FILE_QUEUE Queue = (PSP_FILE_QUEUE)QueueHandle;
    DWORD res;

    //
    // first validate QueueHandle
    //
    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Clean0;
    }

    //
    // now fill in structure
    // if we except, assume bad pointer
    //
    try {
        if(BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            leave;              // exit try block
        }
        res = _SetupGetBackupInformation(Queue,BackupParams);
        if (res == NO_ERROR) {
            b = TRUE;
        } else {
            SetLastError(res);
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          SetLastError(ERROR_INVALID_PARAMETER);
          b = FALSE;
    }

Clean0:
    return b;
}

//
// ==========================================================
//

VOID
RestoreRenamedOrBackedUpFile(
    IN PCTSTR             TargetFilename,
    IN PCTSTR             CurrentFilename,
    IN BOOL               RenameFile,
    IN PSETUP_LOG_CONTEXT LogContext       OPTIONAL
    )
/*++

Routine Description:

    This routine does its best to restore a backed-up or renamed file back to
    its original name.

Arguments:

    TargetFilename - filename to be restored to

    CurrentFilename - file to restore

    RenameFile - if TRUE, CurrentFilename was previously renamed from
        TargetFilename (hence should be renamed back).  If FALSE,
        CurrentFilename is merely a copy, and should be copied back.

    LogContext - supplies a log context used if errors are encountered.

Return Value:

    None.

--*/
{
    DWORD Err;
    TCHAR TempPath[MAX_PATH];
    PTSTR TempNamePtr;
    TCHAR TempFilename[MAX_PATH];
    DWORD LogTag = AllocLogInfoSlotOrLevel(LogContext,SETUP_LOG_INFO,FALSE);

    WriteLogEntry(
        LogContext,
        LogTag,
        MSG_LOG_UNWIND_FILE,
        NULL,
        CurrentFilename,
        TargetFilename
        );

    //
    // First, clear target attributes...
    //
    SetFileAttributes(TargetFilename, FILE_ATTRIBUTE_NORMAL);

    if(RenameFile) {
        Err = DoMove(CurrentFilename, TargetFilename) ? NO_ERROR : GetLastError();
    } else {
        Err = CopyFile(CurrentFilename, TargetFilename, FALSE) ? NO_ERROR : GetLastError();
    }

    if(Err != NO_ERROR) {
        //
        // Can't replace the file that got copied in place of
        // the original one--try to move that one to a tempname
        // and schedule it for delayed deletion.
        //
        WriteLogEntry(LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_UNWIND_TRY1_FAILED,
                    NULL,
                    CurrentFilename,
                    TargetFilename
                    );
        WriteLogError(LogContext,
                    SETUP_LOG_ERROR,
                    Err
                    );

        //
        // First, strip the filename off the path.
        //
        _tcscpy(TempPath, TargetFilename);
        TempNamePtr = (PTSTR)MyGetFileTitle(TempPath);
        *TempNamePtr = TEXT('\0');

        //
        // Now get a temp filename within that directory...
        //
        if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
            //
            // Uh oh!
            //
            Err = GetLastError();
        } else if(!DoMove(TargetFilename, TempFilename)) {
            Err = GetLastError();
        } else {
            //
            // OK, we were able to rename the current file to a
            // temp filename--now attempt to copy or move the
            // original file back to its original name.
            //
            if(RenameFile) {
                Err = DoMove(CurrentFilename, TargetFilename) ? NO_ERROR : GetLastError();
            } else {
                Err = CopyFile(CurrentFilename, TargetFilename, FALSE) ? NO_ERROR : GetLastError();
            }

            if(Err != NO_ERROR) {
                //
                // This is very bad--put the current file back (it's probably
                // better to have something than nothing at all).
                //
                DoMove(TempFilename, TargetFilename);
            }
        }

        if(Err == NO_ERROR) {
            //
            // We successfully moved the current file to a temp
            // filename, and put the original file back.  Now
            // queue a delayed delete for the temp file.
            //
            if(!DelayedMove(TempFilename, NULL)) {
                //
                // All this means is that a file turd will get
                // left on the disk--simply log an event about
                // this.
                //
                Err = GetLastError();

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_RENAME_EXISTING_DELAYED_DELETE_FAILED,
                              NULL,
                              TargetFilename,
                              TempFilename
                             );

                WriteLogError(LogContext,
                              SETUP_LOG_WARNING,
                              Err
                             );
            }

        } else {
            //
            // We were unable to put the original file back--we
            // can't fail, so just log an error about this and
            // keep on going.
            //
            // BUGBUG (lonnym)--in the case of a backed-up file,
            // we might get away with queueing the original file
            // for a delayed rename and then prompting the user
            // to reboot.  However, that won't work for renamed
            // files, because they're typically needed very
            // early on in the boot (i.e., before session
            // manager has had a chance to process the delayed
            // rename operations).
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          (RenameFile
                              ? MSG_LOG_RENAME_EXISTING_RESTORE_FAILED
                              : MSG_LOG_BACKUP_EXISTING_RESTORE_FAILED),
                          NULL,
                          CurrentFilename,
                          TargetFilename
                         );

            WriteLogError(LogContext,
                          SETUP_LOG_ERROR,
                          Err
                         );
        }
    }

    if (LogTag) {
        ReleaseLogInfoSlot(LogContext,LogTag);
    }
}

//
// ==========================================================
//

BOOL
UnPostDelayedMove(
    IN PSP_FILE_QUEUE Queue,
    IN PCTSTR         CurrentName,
    IN PCTSTR         NewName      OPTIONAL
    )
/*++

Routine Description:

    Locates a delay-move node (either for rename or delete), and removes it
    from the delay-move queue.

Arguments:

    Queue               Queue that the move was applied to
    CurrentName         Name of file to be moved
    NewName             Name to move file to (NULL if delayed-delete)

Return Value:

    If successful, the return value is TRUE, otherwise it is FALSE.

--*/
{
    PSP_DELAYMOVE_NODE CurNode, PrevNode;
    PCTSTR SourceFilename, TargetFilename;

    //
    // Since the path string IDs in the delay-move nodes are case-sensitive, we
    // don't attempt to match on ID.  We instead retrieve the strings, and do
    // case-insensitive string compares.  Since this routine is rarely used, the
    // performance hit isn't a big deal.
    //
    for(CurNode = Queue->DelayMoveQueue, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->NextNode) {

        if(NewName) {
            //
            // We're searching for a delayed rename, so we must pay attention
            // to the target filename.
            //
            if(CurNode->TargetFilename == -1) {
                continue;
            } else {
                TargetFilename = StringTableStringFromId(Queue->StringTable, CurNode->TargetFilename);
                MYASSERT(TargetFilename);
                if(lstrcmpi(NewName, TargetFilename)) {
                    //
                    // Target filenames differ--move on.
                    //
                    continue;
                }
            }

        } else {
            //
            // We're searching for a delayed delete.
            //
            if(CurNode->TargetFilename != -1) {
                //
                // This is a rename, not a delete--move on.
                //
                continue;
            }
        }

        //
        // If we get to here, then the target filenames match (if this is a
        // rename), or they're both empty (if it's a delete).  Now compare the
        // source filenames.
        //
        MYASSERT(CurNode->SourceFilename != -1);
        SourceFilename = StringTableStringFromId(Queue->StringTable, CurNode->SourceFilename);
        MYASSERT(SourceFilename);

        if(lstrcmpi(CurrentName, SourceFilename)) {
            //
            // Source filenames differ--move on.
            //
            continue;
        } else {
            //
            // We have a match--remove the node from the delay-move queue.
            //
            if(PrevNode) {
                PrevNode->NextNode = CurNode->NextNode;
            } else {
                Queue->DelayMoveQueue = CurNode->NextNode;
            }
            if(!CurNode->NextNode) {
                MYASSERT(Queue->DelayMoveQueueTail == CurNode);
                Queue->DelayMoveQueueTail = PrevNode;
            }
            MyFree(CurNode);

            return TRUE;
        }
    }

    //
    // We didn't find a match.
    //
    return FALSE;
}

