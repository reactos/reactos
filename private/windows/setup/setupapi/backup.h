/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    backup.h

Abstract:

    Private header for....
    Routines to control backup during install process
    And restore of an old install process
    (See also backup.c)

Author:

    Jamie Hunter (jamiehun) 13-Jan-1997

Revision History:

--*/

typedef struct _SP_TARGET_ENT {
    //
    // Used for backup and unwind-backup
    // Data of TargetLookupTable of a file Queue
    //

    // this file information (strings in StringTable)
    LONG        TargetRoot;
    LONG        TargetSubDir;
    LONG        TargetFilename;

    // where file is, or is-to-be backed up (strings in StringTable)
    LONG        BackupRoot;
    LONG        BackupSubDir;
    LONG        BackupFilename;

    // if file has been renamed, what the new target is (string in TargetLookupTable)
    LONG        NewTargetFilename;

    // Various flags as needed
    DWORD       InternalFlags;

    // security attributes etc
    // (jamiehun TODO)

} SP_TARGET_ENT, *PSP_TARGET_ENT;

typedef struct _SP_UNWIND_NODE {
    //
    // List of things to unwind, FILO
    //
    struct _SP_UNWIND_NODE *NextNode;

    LONG TargetID;                          // TargetID to use for UNWIND
    PSECURITY_DESCRIPTOR SecurityDesc;      // Security descriptor to apply
    FILETIME CreateTime;                    // Time stamps to apply
    FILETIME AccessTime;
    FILETIME WriteTime;

} SP_UNWIND_NODE, *PSP_UNWIND_NODE;

typedef struct _SP_DELAYMOVE_NODE {
    //
    // List of things to rename, FIFO
    //
    struct _SP_DELAYMOVE_NODE *NextNode;

    LONG SourceFilename;                    // What to rename
    LONG TargetFilename;                    // what to rename to
    DWORD SecurityDesc;                     // security descriptor index in the string table
    BOOL TargetIsProtected;                 // target file is a protected system file

} SP_DELAYMOVE_NODE, *PSP_DELAYMOVE_NODE;

#define SP_BKFLG_LATEBACKUP      (1)        // backup only if file is modified in any way
#define SP_BKFLG_PREBACKUP       (2)        // backup uninstall files first
#define SP_BKFLG_CALLBACK        (4)        // flag, indicating app should be callback aware

#define SP_TEFLG_SAVED          (0x0001)    // set if file already copied/moved to backup
#define SP_TEFLG_TEMPNAME       (0x0002)    // set if backup is temporary file
#define SP_TEFLG_ORIGNAME       (0x0004)    // set if backup specifies an original name
#define SP_TEFLG_MODIFIED       (0x0008)    // set if target has been modified/deleted (backup has original)
#define SP_TEFLG_MOVED          (0x0010)    // set if target has been moved (to NewTargetFilename)
#define SP_TEFLG_BACKUPQUEUE    (0x0020)    // set if backup queued in backup sub-queue
#define SP_TEFLG_RESTORED       (0x0040)    // set if file already restored during unwind operation
#define SP_TEFLG_UNWIND         (0x0080)    // set if file added to unwind list
#define SP_TEFLG_SKIPPED        (0x0100)    // we didn't manage to back it up, we cannot back it up, we should not try again
#define SP_TEFLG_INUSE          (0x0200)    // while backing up, we determined we cannot backup file because it cannot be read
#define SP_TEFLG_RENAMEEXISTING (0x0400)    // rename existing file to temp filename in same directory.

#define SP_BACKUP_INSTANCE0     (0)
#define SP_BACKUP_DRIVERFILES   TEXT("DriverFiles")
#define SP_BACKUP_OLDFILES      TEXT("Temp") // relative to the windows directory

//
// these are private routines
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
    );

BOOL
pSetupGetFullBackupPath(
    OUT     PTSTR       FullPath,
    IN      PCTSTR      Path,
    IN      UINT        TargetBufferSize,
    OUT     PUINT       RequiredSize    OPTIONAL
    );

DWORD
pSetupBackupCopyString(
    IN PVOID            DestStringTable,
    OUT PLONG           DestStringID,
    IN PVOID            SrcStringTable,
    IN LONG             SrcStringID
    );

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
    );

DWORD
pSetupBackupGetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    OUT PSP_TARGET_ENT  TargetInfo
    );

DWORD
pSetupBackupSetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    IN PSP_TARGET_ENT   TargetInfo
    );

DWORD
pSetupBackupAppendFiles(
    IN HSPFILEQ         TargetQueueHandle,
    IN PCTSTR           BackupSubDir,
    IN DWORD            BackupFlags,
    IN HSPFILEQ         SourceQueueHandle OPTIONAL
    );

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
    BOOL *DelayedBackup
    );

BOOL
pSetupRemoveBackupDirectory(
    IN PCTSTR           BackupDir
    );

DWORD
pSetupGetCurrentlyInstalledDriverNode(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    );

DWORD
pSetupGetBackupQueue(
    IN      PCTSTR      DeviceID,
    IN OUT  HSPFILEQ    FileQueue,
    IN      DWORD       BackupFlags
    );

BOOL
PostDelayedMove(
    IN struct _SP_FILE_QUEUE *Queue,
    IN PCTSTR                 CurrentName,
    IN PCTSTR                 NewName,     OPTIONAL
    IN DWORD                  SecurityDesc,
    IN BOOL                   TargetIsProtected
    );

BOOL
UnPostDelayedMove(
    IN struct _SP_FILE_QUEUE *Queue,
    IN PCTSTR                 CurrentName,
    IN PCTSTR                 NewName      OPTIONAL
    );

DWORD
DoAllDelayedMoves(
    IN struct _SP_FILE_QUEUE *Queue
    );

VOID
pSetupUnwindAll(
    IN struct _SP_FILE_QUEUE *Queue,
    IN BOOL              Succeeded
    );

VOID
RestoreRenamedOrBackedUpFile(
    IN PCTSTR             TargetFilename,
    IN PCTSTR             CurrentFilename,
    IN BOOL               RenameFile,
    IN PSETUP_LOG_CONTEXT LogContext       OPTIONAL
    );

