/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    diskspac.c

Abstract:

    APIs and supporting routines for disk space requirement
    calculation.

Author:

    Ted Miller (tedm) 26-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// An HDSKSPC actually points to one of these.
//
typedef struct _DISK_SPACE_LIST {

    MYLOCK Lock;

    PVOID DrivesTable;

    UINT Flags;

} DISK_SPACE_LIST, *PDISK_SPACE_LIST;

#define LockIt(h)   BeginSynchronizedAccess(&h->Lock)
#define UnlockIt(h) EndSynchronizedAccess(&h->Lock)


//
// These structures are stored as data associated with
// paths/filenames in the string table.
//

typedef struct _XFILE {
    //
    // -1 means it doesn't currently exist
    //
    LONGLONG CurrentSize;

    //
    // -1 means it will be deleted.
    //
    LONGLONG NewSize;

} XFILE, *PXFILE;


typedef struct _XDIRECTORY {
    //
    // Value indicating how many bytes will be required
    // to hold all the files in the FilesTable after they
    // are put on a file queue and then the queue is committed.
    //
    // This may be a negative number indicating that space will
    // actually be freed!
    //
    LONGLONG SpaceRequired;

    PVOID FilesTable;

} XDIRECTORY, *PXDIRECTORY;


typedef struct _XDRIVE {
    //
    // Value indicating how many bytes will be required
    // to hold all the files in the space list for this drive.
    //
    // This may be a negative number indicating that space will
    // actually be freed!
    //
    LONGLONG SpaceRequired;

    PVOID DirsTable;

    DWORD BytesPerCluster;

    //
    // This is the amount to skew SpaceRequired, based on
    // SetupAdjustDiskSpaceList(). We track this separately
    // for flexibility.
    //
    LONGLONG Slop;

} XDRIVE, *PXDRIVE;


typedef struct _RETURN_BUFFER_INFO {
    PVOID ReturnBuffer;
    DWORD ReturnBufferSize;
    DWORD RequiredSize;
#ifdef UNICODE
    BOOL IsUnicode;
#endif
} RETURN_BUFFER_INFO, *PRETURN_BUFFER_INFO;


BOOL
pSetupQueryDrivesInDiskSpaceList(
    IN  HDSKSPC DiskSpace,
    OUT PVOID   ReturnBuffer,       OPTIONAL
    IN  DWORD   ReturnBufferSize,
    OUT PDWORD  RequiredSize        OPTIONAL
#ifdef UNICODE
    IN ,BOOL    IsUnicode
#endif
    );

BOOL
pAddOrRemoveFileFromSectionToDiskSpaceList(
    IN OUT PDISK_SPACE_LIST DiskSpaceList,
    IN     HINF             LayoutInf,
    IN     PINFCONTEXT      LineInSection,      OPTIONAL
    IN     PCTSTR           FileName,           OPTIONAL
    IN     PCTSTR           TargetDirectory,
    IN     UINT             Operation,
    IN     BOOL             Add
    );

BOOL
pAddOrRemoveInstallSection(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCTSTR  SectionName,
    IN BOOL    Add
    );

BOOL
pSetupAddToDiskSpaceList(
    IN  PDISK_SPACE_LIST DiskSpaceList,
    IN  PCTSTR           TargetFilespec,
    IN  LONGLONG         FileSize,
    IN  UINT             Operation
    );

BOOL
pSetupRemoveFromDiskSpaceList(
    IN  PDISK_SPACE_LIST DiskSpaceList,
    IN  PCTSTR           TargetFilespec,
    IN  UINT             Operation
    );

VOID
pRecalcSpace(
    IN OUT PDISK_SPACE_LIST DiskSpaceList,
    IN     LONG             DriveStringId
    );

BOOL
pStringTableCBEnumDrives(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

BOOL
pStringTableCBDelDrives(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

BOOL
pStringTableCBDelDirs(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

BOOL
pStringTableCBZeroDirsTableMember(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

BOOL
pStringTableCBDupMemberStringTable(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

DWORD
pParsePath(
    IN  PCTSTR    PathSpec,
    OUT PTSTR     Buffer,
    OUT PTSTR    *DirectoryPart,
    OUT PTSTR    *FilePart,
    OUT LONGLONG *FileSize,
    IN  UINT      Flags
    );




HDSKSPC
SetupCreateDiskSpaceList(
    IN PVOID Reserved1,
    IN DWORD Reserved2,
    IN UINT  Flags
    )

/*++

Routine Description:

    This routine creates a disk space list, which can be used to
    determine required disk space for a set of file operations
    that parallel those that an application will perform later,
    such as via the file queue APIs.

Arguments:

    Reserved1 - Unused, must be 0.

    Reserved2 - Unused, must be 0.

    Flags - Specifies flags that govern operation of the disk space list.

        SPDSL_IGNORE_DISK: If this flag is set, then delete operations
            will be ignored, and copy operations will behave as if
            the target files are not present on the disk, regardless of
            whether the files are actually present. This flag is useful
            to determine an approximate size that can be associated with
            a set of files.

Return Value:

    Handle to disk space list to be used in subsequent operations,
    or NULL if the routine fails, in which case GetLastError()
    returns extended error info.

--*/

{
    PDISK_SPACE_LIST SpaceList;
    DWORD d;

    //
    // Validate args.
    //
    if(Reserved1 || Reserved2) {
        d = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    //
    // Allocate space for a structure.
    //
    SpaceList = MyMalloc(sizeof(DISK_SPACE_LIST));
    if(!SpaceList) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c1;
    }

    ZeroMemory(SpaceList,sizeof(DISK_SPACE_LIST));

    SpaceList->Flags = Flags;

    //
    // Create a string table for the drives.
    //
    SpaceList->DrivesTable = pStringTableInitialize(sizeof(XDRIVE));
    if(!SpaceList->DrivesTable) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c2;
    }

    //
    // Create a locking structure for this guy.
    //
    if(!InitializeSynchronizedAccess(&SpaceList->Lock)) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c3;
    }

    //
    // Success.
    //
    return(SpaceList);

c3:
    pStringTableDestroy(SpaceList->DrivesTable);
c2:
    if(SpaceList) {
        MyFree(SpaceList);
    }
c1:
    SetLastError(d);
c0:
    return(NULL);
}


#ifdef UNICODE
//
// Ansi version.
//
HDSKSPC
SetupCreateDiskSpaceListA(
    IN PVOID Reserved1,
    IN DWORD Reserved2,
    IN UINT  Flags
    )
{
    //
    // Nothing actually ansi/unicode specific now
    //
    return(SetupCreateDiskSpaceListW(Reserved1,Reserved2,Flags));
}
#else
//
// Unicode stub.
//
HDSKSPC
SetupCreateDiskSpaceListW(
    IN PVOID Reserved1,
    IN DWORD Reserved2,
    IN UINT  Flags
    )
{
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(NULL);
}
#endif


HDSKSPC
SetupDuplicateDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN PVOID   Reserved1,
    IN DWORD   Reserved2,
    IN UINT    Flags
    )

/*++

Routine Description:

    This routine duplicates a disk space, creating a new, fully independent
    disk space list.

Arguments:

    DiskSpace - supplies handle of disk space list to be duplicated.

    Reserved1 - reserved, must be 0.

    Reserved2 - reserved, must be 0.

    Flags - reserved, must be 0.

Return Value:

    If successful, returns a handle to a new disk space list.
    NULL if failure; GetLastError() returns extended error info.

--*/

{
    PDISK_SPACE_LIST OldSpaceList;
    PDISK_SPACE_LIST NewSpaceList;
    DWORD d;
    BOOL b;
    XDRIVE xDrive;

    //
    // Validate args.
    //
    if(Reserved1 || Reserved2 || Flags) {
        d = ERROR_INVALID_PARAMETER;
        goto c0;
    }

    //
    // Allocate space for a new structure and create a locking structure.
    //
    NewSpaceList = MyMalloc(sizeof(DISK_SPACE_LIST));
    if(!NewSpaceList) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }
    ZeroMemory(NewSpaceList,sizeof(DISK_SPACE_LIST));

    if(!InitializeSynchronizedAccess(&NewSpaceList->Lock)) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c1;
    }

    //
    // Lock down the existing space list.
    //
    OldSpaceList = DiskSpace;
    d = NO_ERROR;
    try {
        if(!LockIt(OldSpaceList)) {
            d = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_HANDLE;
    }

    if(d != NO_ERROR) {
        goto c2;
    }

    //
    // Duplicate the top-level string table. After we do this, we'll have
    // a string table whose items' extra data is XDRIVE structures,
    // which will each contain a string table handle for a string table for
    // directories. But we don't want to share that string table between
    // the old and new disk space tables. So start by zeroing the DirsTable
    // members of all the XDRIVE structures. This will let us clean up
    // more easily later in the error path.
    //
    MYASSERT(OldSpaceList->DrivesTable);
    NewSpaceList->DrivesTable = pStringTableDuplicate(OldSpaceList->DrivesTable);
    if(!NewSpaceList->DrivesTable) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c3;
    }

    pStringTableEnum(
        NewSpaceList->DrivesTable,
        &xDrive,
        sizeof(XDRIVE),
        pStringTableCBZeroDirsTableMember,
        0
        );

    //
    // Now we enumerate the old drives table and duplicate each directory
    // string table into the new drives table. We take heavy advantage
    // of the fact that the ids are the same between the old and new tables.
    //
    b = pStringTableEnum(
            OldSpaceList->DrivesTable,
            &xDrive,
            sizeof(XDRIVE),
            pStringTableCBDupMemberStringTable,
            (LPARAM)NewSpaceList->DrivesTable
            );

    if(!b) {
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(d != NO_ERROR) {
        pStringTableEnum(
            NewSpaceList->DrivesTable,
            &xDrive,
            sizeof(XDRIVE),
            pStringTableCBDelDrives,
            0
            );
        pStringTableDestroy(NewSpaceList->DrivesTable);
    }
c3:
    //
    // Unlock the existing space list.
    //
    try {
        UnlockIt(OldSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Don't worry if the pointer went bad, we're already done
        // with the important work.
        //
        ;
    }
c2:
    if(d != NO_ERROR) {
        DestroySynchronizedAccess(&NewSpaceList->Lock);
    }
c1:
    if(d != NO_ERROR) {
        MyFree(NewSpaceList);
    }
c0:
    SetLastError(d);
    return((d == NO_ERROR) ? NewSpaceList : NULL);
}

#ifdef UNICODE
//
// Ansi version.
//
HDSKSPC
SetupDuplicateDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN PVOID   Reserved1,
    IN DWORD   Reserved2,
    IN UINT    Flags
    )
{
    //
    // Nothing actually ansi/unicode specific now
    //
    return(SetupDuplicateDiskSpaceListW(DiskSpace,Reserved1,Reserved2,Flags));
}
#else
//
// Unicode stub.
//
HDSKSPC
SetupDuplicateDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN PVOID   Reserved1,
    IN DWORD   Reserved2,
    IN UINT    Flags
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(NULL);
}
#endif


BOOL
SetupDestroyDiskSpaceList(
    IN OUT HDSKSPC DiskSpace
    )

/*++

Routine Description:

    This routine destryos a disk space list which was created
    with SetupCreateDiskSpaceList() and releases all resources
    used thereby.

Arguments:

    DiskSpace - supplies handle to space list to be deconstructed.

Return Value:

    Boolean value indicating outcome. If FALSE, extended error info
    is available from GetLastError().

--*/

{
    PDISK_SPACE_LIST DiskSpaceList;
    DWORD rc;
    XDRIVE xDrive;

    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    try {
        DestroySynchronizedAccess(&DiskSpaceList->Lock);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Just swallow this.
        //
        ;
    }

    try {

        MYASSERT(DiskSpaceList->DrivesTable);
        //
        // Enumerate the drives string table. This in turn causes
        // all directory and file string tables to get destroyed.
        //
        pStringTableEnum(
            DiskSpaceList->DrivesTable,
            &xDrive,
            sizeof(XDRIVE),
            pStringTableCBDelDrives,
            0
            );

        pStringTableDestroy(DiskSpaceList->DrivesTable);

        //
        // Free the disk space list guy.
        //
        MyFree(DiskSpaceList);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}


BOOL
SetupAdjustDiskSpaceList(
    IN HDSKSPC  DiskSpace,
    IN LPCTSTR  DriveRoot,
    IN LONGLONG Amount,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )

/*++

Routine Description:

    This routine is used to add an absolute amount of required disk space
    for a drive.

Arguments:

    DiskSpace - supplies a handle to a disk space list.

    DriveRoot - specifies a valid Win32 drive root. If this drive is not
        currently represented in the disk space list then an entry for it
        is added.

    Amount - supplies the amount of disk space by which to adjust space
        required on the drive. Use a negative number to remove space.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

--*/

{
    DWORD rc;
    BOOL b;

    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = NO_ERROR;

    try {
        if(!LockIt(((PDISK_SPACE_LIST)DiskSpace))) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    //
    // pSetupAddToDiskSpaceList does all the work. That routine
    // uses SEH so no need for try/excepts here.
    //
    b = pSetupAddToDiskSpaceList(DiskSpace,DriveRoot,Amount,(UINT)(-1));
    rc = GetLastError();

    //
    // The try/except around the unlock simply prevents us from faulting
    // but we don't return error if the pointer goes bad.
    //
    try {
        UnlockIt(((PDISK_SPACE_LIST)DiskSpace));
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    SetLastError(rc);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupAdjustDiskSpaceListA(
    IN HDSKSPC  DiskSpace,
    IN LPCSTR   DriveRoot,
    IN LONGLONG Amount,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )
{
    LPCWSTR p;
    BOOL b;
    DWORD rc;

    rc = CaptureAndConvertAnsiArg(DriveRoot,&p);
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = SetupAdjustDiskSpaceListW(DiskSpace,p,Amount,Reserved1,Reserved2);
    rc = GetLastError();

    MyFree(p);

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupAdjustDiskSpaceListW(
    IN HDSKSPC  DiskSpace,
    IN LPCWSTR  DriveRoot,
    IN LONGLONG Amount,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(DriveRoot);
    UNREFERENCED_PARAMETER(Amount);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupAddToDiskSpaceList(
    IN HDSKSPC  DiskSpace,
    IN PCTSTR   TargetFilespec,
    IN LONGLONG FileSize,
    IN UINT     Operation,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )

/*++

Routine Description:

    This routine adds a single delete or copy operation to a
    disk space list.

    Note that disk compression is completely ignored by this routine.
    Files are assumed to occupy their full size on the disk.

Arguments:

    DiskSpace - specifies handle to disk space list created by
        SetupCreateDiskSpaceList().

    TargetFilespec - specifies filename of the file to add
        to the disk space list. This will generally be a full win32
        path, though this is not a requirement. If it is not then
        standard win32 path semantics apply.

    FileSize - supplies the (uncompressed) size of the file as it will
        exist on the target when copied. Ignored for FILEOP_DELETE.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    DWORD rc;
    BOOL b;

    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = NO_ERROR;

    try {
        if(!LockIt(((PDISK_SPACE_LIST)DiskSpace))) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = pSetupAddToDiskSpaceList(DiskSpace,TargetFilespec,FileSize,Operation);
    rc = GetLastError();

    //
    // The try/except around the unlock simply prevents us from faulting
    // but we don't return error if the pointer goes bad.
    //
    try {
        UnlockIt(((PDISK_SPACE_LIST)DiskSpace));
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    SetLastError(rc);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupAddToDiskSpaceListA(
    IN HDSKSPC  DiskSpace,
    IN PCSTR    TargetFilespec,
    IN LONGLONG FileSize,
    IN UINT     Operation,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )
{
    PWSTR targetFilespec;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(TargetFilespec,&targetFilespec);
    if(rc != NO_ERROR) {
        return(rc);
    }

    b = SetupAddToDiskSpaceListW(DiskSpace,targetFilespec,FileSize,Operation,Reserved1,Reserved2);
    rc = GetLastError();

    MyFree(targetFilespec);

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupAddToDiskSpaceListW(
    IN HDSKSPC  DiskSpace,
    IN PCWSTR   TargetFilespec,
    IN LONGLONG FileSize,
    IN UINT     Operation,
    IN PVOID    Reserved1,
    IN UINT     Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(TargetFilespec);
    UNREFERENCED_PARAMETER(FileSize);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupAddSectionToDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCTSTR  SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )

/*++

Routine Description:

    This routine adds a delete or copy section to a disk space list.

    Note that disk compression is completely ignored by this routine.
    Files are assumed to occupy their full size on the disk.

Arguments:

    DiskSpace - specifies handle to disk space list created by
        SetupCreateDiskSpaceList().

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] section, and, if ListInfHandle is not specified,
        contains the section named by SectionName. This handle must be for
        a win95-style inf.

    ListInfHandle - if specified, supplies a handle to an open inf file
        containing the section to be added to the disk space list.
        Otherwise InfHandle is assumed to contain the section.

    SectionName - supplies the name of the section to be added to
        the disk space list.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PDISK_SPACE_LIST DiskSpaceList;
    LONG LineCount;
    PCTSTR TargetFilename;
    BOOL b;
    INFCONTEXT LineContext;
    TCHAR FullTargetPath[MAX_PATH];
    DWORD FileSize;
    DWORD rc;

    //
    // Note throughout this routine that very little structured exception handling
    // is needed, since most of the work is performed by subroutines that are
    // properly guarded.
    //

    if(Reserved1 || Reserved2) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
        goto c0;
    }

    //
    // Lock down the DiskSpace handle/structure.
    //
    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        b = FALSE;
        goto c0;
    }

    if(!ListInfHandle) {
        ListInfHandle = InfHandle;
    }

    //
    // The section must at least exist; an empty section is
    // a trivial success case.
    //
    LineCount = SetupGetLineCount(ListInfHandle,SectionName);
    if(LineCount == -1) {
        rc = ERROR_SECTION_NOT_FOUND;
        b = FALSE;
        goto c1;
    }
    if(!LineCount) {
        b = TRUE;
        goto c1;
    }

    //
    // Find the first line. We know there is at least one since the line count
    // was checked above. Sanity check it anyway.
    //
    b = SetupFindFirstLine(ListInfHandle,SectionName,NULL,&LineContext);
    MYASSERT(b);
    if(!b) {
        rc = ERROR_SECTION_NOT_FOUND;
        goto c1;
    }

    //
    // Find the target path for this section.
    //
    if(!SetupGetTargetPath(NULL,&LineContext,NULL,FullTargetPath,MAX_PATH,NULL)) {
        rc = GetLastError();
        goto c1;
    }

    //
    // Process each line in the section.
    //
    do {

        b = pAddOrRemoveFileFromSectionToDiskSpaceList(
                DiskSpaceList,
                InfHandle,
                &LineContext,
                NULL,
                FullTargetPath,
                Operation,
                TRUE
                );

        if(!b) {
            rc = GetLastError();
        }

    } while(b && SetupFindNextLine(&LineContext,&LineContext));

c1:
    try {
        UnlockIt(DiskSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
c0:
    SetLastError(rc);
    return(b);
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupAddSectionToDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCSTR   SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    PWSTR sectionName;
    BOOL b;
    DWORD rc;

    rc = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    if(rc == NO_ERROR) {

        b = SetupAddSectionToDiskSpaceListW(
                DiskSpace,
                InfHandle,
                ListInfHandle,
                sectionName,
                Operation,
                Reserved1,
                Reserved2
                );

        rc = GetLastError();

        MyFree(sectionName);
    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupAddSectionToDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCWSTR  SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ListInfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupAddInstallSectionToDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCTSTR  SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )

/*++

Routine Description:

    Processes an install section, looking for CopyFiles and DelFiles
    lines, and adds those sections to a disk space list.

Arguments:

Return Value:

    Win32 error code indicating outcome.

--*/

{
    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    return(pAddOrRemoveInstallSection(DiskSpace,InfHandle,LayoutInfHandle,SectionName,TRUE));
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupAddInstallSectionToDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCSTR   SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    PWSTR sectionName;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    if(rc == NO_ERROR) {

        b = SetupAddInstallSectionToDiskSpaceListW(
                DiskSpace,
                InfHandle,
                LayoutInfHandle,
                sectionName,
                Reserved1,
                Reserved2
                );

        rc = GetLastError();

        MyFree(sectionName);
    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupAddInstallSectionToDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCWSTR  SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(LayoutInfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupRemoveFromDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN PCTSTR  TargetFilespec,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )

/*++

Routine Description:

    This routine removes a single delete or copy operation from a
    disk space list.

Arguments:

    DiskSpace - specifies handle to disk space list created by
        SetupCreateDiskSpaceList().

    TargetFilespec - specifies filename of the file to remove from
        the disk space list. This will generally be a full win32
        path, though this is not a requirement. If it is not then
        standard win32 path semantics apply.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

    If the file was not in the list, the routine returns TRUE and
    GetLastError() returns ERROR_INVALID_DRIVE or ERROR_INVALID_NAME.
    If the file was in the list then upon success the routine returns
    TRUE and GetLastError() returns NO_ERROR.

    If the routine fails for some other reason it returns FALSE and GetLastError()
    can be used to fetch extended error info.

--*/

{
    DWORD rc;
    BOOL b;

    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = NO_ERROR;

    try {
        if(!LockIt(((PDISK_SPACE_LIST)DiskSpace))) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = pSetupRemoveFromDiskSpaceList(DiskSpace,TargetFilespec,Operation);
    rc = GetLastError();

    //
    // The try/except around the unlock simply prevents us from faulting
    // but we don't return error if the pointer goes bad.
    //
    try {
        UnlockIt(((PDISK_SPACE_LIST)DiskSpace));
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    SetLastError(rc);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupRemoveFromDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN PCSTR   TargetFilespec,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    PWSTR targetFilespec;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(TargetFilespec,&targetFilespec);
    if(rc != NO_ERROR) {
        return(rc);
    }

    b = SetupRemoveFromDiskSpaceListW(DiskSpace,targetFilespec,Operation,Reserved1,Reserved2);
    rc = GetLastError();

    MyFree(targetFilespec);

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupRemoveFromDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN PCWSTR  TargetFilespec,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(TargetFilespec);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupRemoveSectionFromDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCTSTR  SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )

/*++

Routine Description:

    This routine removes a delete or copy section from a disk space list.
    The section is presumed to have been added via SetupAddSectionToDiskSpaceList,
    though this is not a requirement. Files that have not actually been added
    will not be removed.

    Note that disk compression is completely ignored by this routine.
    Files are assumed to occupy their full size on the disk.

Arguments:

    DiskSpace - specifies handle to disk space list created by
        SetupCreateDiskSpaceList().

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] section, and, if ListInfHandle is not specified,
        contains the section named by SectionName. This handle must be for
        a win95-style inf.

    ListInfHandle - if specified, supplies a handle to an open inf file
        containing the section to be removed from the disk space list.
        Otherwise InfHandle is assumed to contain the section.

    SectionName - supplies the name of the section to be added to
        the disk space list.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PDISK_SPACE_LIST DiskSpaceList;
    LONG LineCount;
    PCTSTR TargetFilename;
    BOOL b;
    INFCONTEXT LineContext;
    TCHAR FullTargetPath[MAX_PATH];
    DWORD rc;

    //
    // Note throughout this routine that very little structured exception handling
    // is needed, since most of the work is performed by subroutines that are
    // properly guarded.
    //

    if(Reserved1 || Reserved2) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
        goto c0;
    }

    //
    // Lock down the DiskSpace handle/structure.
    //
    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        b = FALSE;
        goto c0;
    }

    if(!ListInfHandle) {
        ListInfHandle = InfHandle;
    }

    //
    // The section must at least exist; an empty section is
    // a trivial success case.
    //
    LineCount = SetupGetLineCount(ListInfHandle,SectionName);
    if(LineCount == -1) {
        rc = ERROR_SECTION_NOT_FOUND;
        b = FALSE;
        goto c1;
    }
    if(!LineCount) {
        b = TRUE;
        goto c1;
    }

    //
    // Find the first line. We know there is at least one since the line count
    // was checked above. Sanity check it anyway.
    //
    b = SetupFindFirstLine(ListInfHandle,SectionName,NULL,&LineContext);
    MYASSERT(b);
    if(!b) {
        rc = ERROR_SECTION_NOT_FOUND;
        b = FALSE;
        goto c1;
    }

    //
    // Find the target path for this section.
    //
    if(!SetupGetTargetPath(NULL,&LineContext,NULL,FullTargetPath,MAX_PATH,NULL)) {
        rc = GetLastError();
        b = FALSE;
        goto c1;
    }

    //
    // Process each line in the section.
    //
    do {

        b = pAddOrRemoveFileFromSectionToDiskSpaceList(
                DiskSpaceList,
                InfHandle,
                &LineContext,
                NULL,
                FullTargetPath,
                Operation,
                FALSE
                );

        if(!b) {
            rc = GetLastError();
        }
    } while(b && SetupFindNextLine(&LineContext,&LineContext));

c1:
    try {
        UnlockIt(DiskSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

c0:
    SetLastError(rc);
    return(b);
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupRemoveSectionFromDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCSTR   SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    PWSTR sectionName;
    BOOL b;
    DWORD rc;

    rc = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    if(rc == NO_ERROR) {

        b = SetupRemoveSectionFromDiskSpaceListW(
                DiskSpace,
                InfHandle,
                ListInfHandle,
                sectionName,
                Operation,
                Reserved1,
                Reserved2
                );

        rc = GetLastError();

        MyFree(sectionName);
    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupRemoveSectionFromDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    ListInfHandle,  OPTIONAL
    IN PCWSTR  SectionName,
    IN UINT    Operation,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ListInfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupRemoveInstallSectionFromDiskSpaceList(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCTSTR  SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )

/*++

Routine Description:

    Processes an install section, looking for CopyFiles and DelFiles
    lines, and removes those sections from a disk space list.

Arguments:

    DiskSpace - supplies a handle to a disk space list.

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] section, and, if ListInfHandle is not specified,
        contains the section named by SectionName. This handle must be for
        a win95-style inf.

    ListInfHandle - if specified, supplies a handle to an open inf file
        containing the section to be removed from the disk space list.
        Otherwise InfHandle is assumed to contain the section.

    SectionName - supplies the name of the section to be added to
        the disk space list.

    Reserved1 - must be 0.

    Reserved2 - must be 0.

Return Value:

    Boolean value indicating outcome. If FALSE, extended error info
    is available via GetLastError().


--*/

{
    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    return(pAddOrRemoveInstallSection(DiskSpace,InfHandle,LayoutInfHandle,SectionName,FALSE));
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupRemoveInstallSectionFromDiskSpaceListA(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCSTR   SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    PWSTR sectionName;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    if(rc == NO_ERROR) {

        b = SetupRemoveInstallSectionFromDiskSpaceListW(
                DiskSpace,
                InfHandle,
                LayoutInfHandle,
                sectionName,
                Reserved1,
                Reserved2
                );

        rc = GetLastError();

        MyFree(sectionName);
    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupRemoveInstallSectionFromDiskSpaceListW(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCWSTR  SectionName,
    IN PVOID   Reserved1,
    IN UINT    Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(LayoutInfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupQuerySpaceRequiredOnDrive(
    IN  HDSKSPC   DiskSpace,
    IN  PCTSTR    DriveSpec,
    OUT LONGLONG *SpaceRequired,
    IN  PVOID     Reserved1,
    IN  UINT      Reserved2
    )

/*++

Routine Description:

    Examine a disk space list to determine the space required on a
    particular drive.

Arguments:

    DiskSpace - supplies a handle to a disk space list.

    DriveSpec - specifies the drive for which space info is desired.
        This should be in the form x: or \\server\share.

    SpaceRequired - if the function succeeds, receives the amount
        of space required. This may be 0 or a negative number!

    Reserved1 - reserved, must be 0.

    Reserved2 - reserved, must be 0.

Return Value:

    Boolean value indicating outcome. If TRUE, SpaceRequired is filled in.

    If FALSE, extended error info is available via GetLastError():

    ERROR_INVALID_HANDLE - the specified DiskSpace handle is invalid.
    ERROR_INVALID_DRIVE - the given drive is not in the disk space list.

--*/

{
    PDISK_SPACE_LIST DiskSpaceList;
    DWORD rc;
    BOOL b;
    LONG l;
    DWORD Hash;
    DWORD StringLength;
    XDRIVE xDrive;
    TCHAR drive[MAX_PATH];

    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Lock down the DiskSpace handle/structure.
    //
    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    try {
        lstrcpyn(drive,DriveSpec,MAX_PATH);

        MYASSERT(DiskSpaceList->DrivesTable);

        l = pStringTableLookUpString(
                DiskSpaceList->DrivesTable,
                drive,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                NULL,
                0
                );

        if(l != -1) {
            //
            // Found the drive. Recalc space and return it.
            //
            pRecalcSpace(DiskSpaceList,l);
            pStringTableGetExtraData(DiskSpaceList->DrivesTable,l,&xDrive,sizeof(XDRIVE));
            *SpaceRequired = xDrive.SpaceRequired + xDrive.Slop;
            b = TRUE;
        } else {
            rc = ERROR_INVALID_DRIVE;
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
    }

    try {
        UnlockIt(DiskSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    SetLastError(rc);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQuerySpaceRequiredOnDriveA(
    IN  HDSKSPC   DiskSpace,
    IN  PCSTR     DriveSpec,
    OUT LONGLONG *SpaceRequired,
    IN  PVOID     Reserved1,
    IN  UINT      Reserved2
    )
{
    PCWSTR drivespec;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(DriveSpec,&drivespec);
    if(rc == NO_ERROR) {

        b = SetupQuerySpaceRequiredOnDrive(DiskSpace,drivespec,SpaceRequired,Reserved1,Reserved2);
        rc = GetLastError();

        MyFree(drivespec);
    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQuerySpaceRequiredOnDriveW(
    IN  HDSKSPC   DiskSpace,
    IN  PCWSTR    DriveSpec,
    OUT LONGLONG *SpaceRequired,
    IN  PVOID     Reserved1,
    IN  UINT      Reserved2
    )
{
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(DriveSpec);
    UNREFERENCED_PARAMETER(SpaceRequired);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupQueryDrivesInDiskSpaceListA(
    IN  HDSKSPC DiskSpace,
    OUT PSTR    ReturnBuffer,       OPTIONAL
    IN  DWORD   ReturnBufferSize,
    OUT PDWORD  RequiredSize        OPTIONAL
    )

/*++

Routine Description:

    This routine fills a caller-supplied buffer with drive specs for each
    drive currently represented in the given disk space list.

Arguments:

    DiskSpace - supplies a disk space list handle.

    ReturnBuffer - if supplied, points to a buffer that gets packed with
        the drive specs, followed by a final terminating nul. If not specified
        and not other error occurs, the function succeeds and fills in
        RequiredSize.

    ReturnBufferSize - supplies the size (chars for Unicode, bytes for ANSI)
        of the buffer pointed by ReturnBuffer. Ingored if ReturnBuffer
        is not specified.

    RequiredSize - if specified, receives the size of the buffer required
        to hold the list of drives and terminating nul.

Return Value:

    Boolean value indicating outcome. If the function returns FALSE,
    extended error info is available via GetLastError(). If GetLastError()
    returns ERROR_INSUFFICIENT_BUFFER then ReturnBuffer was specified but
    ReturnBufferSize indicated that the supplied buffer was too small.

--*/

{
    BOOL b;

    b = pSetupQueryDrivesInDiskSpaceList(
            DiskSpace,
            ReturnBuffer,
            ReturnBufferSize,
            RequiredSize
#ifdef UNICODE
           ,FALSE
#endif
            );

    return(b);
}


BOOL
SetupQueryDrivesInDiskSpaceListW(
    IN  HDSKSPC DiskSpace,
    OUT PWSTR   ReturnBuffer,       OPTIONAL
    IN  DWORD   ReturnBufferSize,
    OUT PDWORD  RequiredSize        OPTIONAL
    )

/*++

Routine Description:

    See SetupQueryDrivesInDiskSpaceListA.

Arguments:

    See SetupQueryDrivesInDiskSpaceListA.

Return Value:

    See SetupQueryDrivesInDiskSpaceListA.

--*/

{
    BOOL b;

#ifdef UNICODE
    b = pSetupQueryDrivesInDiskSpaceList(
            DiskSpace,
            ReturnBuffer,
            ReturnBufferSize,
            RequiredSize,
            TRUE
            );
#else
    UNREFERENCED_PARAMETER(DiskSpace);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    b = FALSE;
#endif

    return(b);
}


BOOL
pSetupQueryDrivesInDiskSpaceList(
    IN  HDSKSPC DiskSpace,
    OUT PVOID   ReturnBuffer,       OPTIONAL
    IN  DWORD   ReturnBufferSize,
    OUT PDWORD  RequiredSize        OPTIONAL
#ifdef UNICODE
    IN ,BOOL    IsUnicode
#endif
    )

/*++

Routine Description:

    Worker routine for SetupQueryDrivesInDiskSpaceList.

Arguments:

    Same as SetupQueryDrivesInDiskSpaceListA/W.

    IsUnicode - for Unicode DLL, specifies whether buffer args
        are ansi or unicode.

Return Value:

    Same as SetupQueryDrivesInDiskSpaceListA/W.

--*/

{
    PDISK_SPACE_LIST DiskSpaceList;
    DWORD rc;
    BOOL b;
    XDRIVE xDrive;
    RETURN_BUFFER_INFO ReturnBufferInfo;

    //
    // Lock down the DiskSpace handle/structure.
    //
    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    try {
        ReturnBufferInfo.ReturnBuffer = ReturnBuffer;
        ReturnBufferInfo.ReturnBufferSize = ReturnBufferSize;
        ReturnBufferInfo.RequiredSize = 0;
        #ifdef UNICODE
        ReturnBufferInfo.IsUnicode = IsUnicode;
        #endif

        MYASSERT(DiskSpaceList->DrivesTable);

        b = pStringTableEnum(
                DiskSpaceList->DrivesTable,
                &xDrive,
                sizeof(XDRIVE),
                pStringTableCBEnumDrives,
                (LPARAM)&ReturnBufferInfo
                );

        if(b) {
            //
            // Need one more char slot for the extra terminating nul.
            //
            ReturnBufferInfo.RequiredSize++;
            if(RequiredSize) {
                *RequiredSize = ReturnBufferInfo.RequiredSize;
            }

            if(ReturnBuffer) {

                if(ReturnBufferInfo.RequiredSize <= ReturnBufferSize) {

                    #ifdef UNICODE
                    if(!IsUnicode) {
                        ((PSTR)ReturnBuffer)[ReturnBufferInfo.RequiredSize-1] = 0;
                    } else
                    #endif
                    ((PTSTR)ReturnBuffer)[ReturnBufferInfo.RequiredSize-1] = 0;

                } else {
                    rc = ERROR_INSUFFICIENT_BUFFER;
                    b = FALSE;
                }
            }
        } else {
            rc = ERROR_INSUFFICIENT_BUFFER;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
    }

    try {
        UnlockIt(DiskSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }

    SetLastError(rc);
    return(b);
}


BOOL
pAddOrRemoveInstallSection(
    IN HDSKSPC DiskSpace,
    IN HINF    InfHandle,
    IN HINF    LayoutInfHandle,     OPTIONAL
    IN PCTSTR  SectionName,
    IN BOOL    Add
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD rc;
    BOOL b;
    unsigned i;
    unsigned numops;
    UINT operation;
    PDISK_SPACE_LIST DiskSpaceList;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    DWORD Field;
    PCTSTR SectionSpec;
    PCTSTR Operations[1] = { TEXT("Copyfiles") };
    INFCONTEXT SectionLineContext;
    TCHAR DefaultTarget[MAX_PATH];

    //
    // BUGBUG!!! (jamiehun) Delfiles causes too many issues
    //           removed to give a good "worst case" scenario
    //           however we intend to add it back along with
    //           RenFiles
    //           When this issue is revisited, change numops
    //           The Operations array
    //           and the switch to convert i to operation
    //
    // PCTSTR Operations[2] = { TEXT("Delfiles"),TEXT("Copyfiles") };
    //

    //
    // Lock down the DiskSpace handle/structure.
    //
    DiskSpaceList = DiskSpace;
    rc = NO_ERROR;
    b = TRUE;
    DefaultTarget[0] = 0;

    //
    // only handle Copyfiles at the moment
    //
    numops = 1;

    try {
        if(!LockIt(DiskSpaceList)) {
            rc = ERROR_INVALID_HANDLE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_HANDLE;
    }

    if(rc != NO_ERROR) {
        b = FALSE;
        goto c0;
    }
    if(!LayoutInfHandle) {
        LayoutInfHandle = InfHandle;
    }

    b = TRUE;
    for(i=0; b && (i < numops); i++) {

        //
        // Find the relevent line in the given install section.
        // If not present then we're done with this operation.
        //
        if(!SetupFindFirstLine(InfHandle,SectionName,Operations[i],&LineContext)) {
            continue;
        }

        switch(i) {
        case 0:
            operation = FILEOP_COPY;
            break;
        default:
            //
            // if we get here, someone changed numops
            // without changing this switch
            //
            MYASSERT(FALSE);
            break;
        }


        do {
            //
            // Each value on the line in the given install section
            // is the name of another section.
            //
            FieldCount = SetupGetFieldCount(&LineContext);
            for(Field=1; b && (Field<=FieldCount); Field++) {

                if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                    //
                    // Handle single-file copy specially.
                    //
                    if((operation == FILEOP_COPY) && (*SectionSpec == TEXT('@'))) {

                        if(!DefaultTarget[0]) {
                            //
                            // Fetch the default target path for this inf, for use with
                            // single-file copy specs.
                            //
                            b = SetupGetTargetPath(
                                    InfHandle,
                                    NULL,
                                    NULL,
                                    DefaultTarget,
                                    MAX_PATH,
                                    NULL
                                    );
                        }

                        if(b) {
                            b = pAddOrRemoveFileFromSectionToDiskSpaceList(
                                    DiskSpace,
                                    LayoutInfHandle,
                                    NULL,
                                    SectionSpec+1,
                                    DefaultTarget,
                                    operation,
                                    Add
                                    );
                        }

                        if(!b) {
                            rc = GetLastError();
                        }
                    } else if(SetupGetLineCount(InfHandle,SectionSpec) > 0) {
                        //
                        // The section exists and is not empty.
                        // Add/remove it to the space list.
                        //
                        if(Add) {
                            b = SetupAddSectionToDiskSpaceList(
                                    DiskSpace,
                                    LayoutInfHandle,
                                    InfHandle,
                                    SectionSpec,
                                    operation,
                                    0,0
                                    );
                        } else {
                            b = SetupRemoveSectionFromDiskSpaceList(
                                    DiskSpace,
                                    LayoutInfHandle,
                                    InfHandle,
                                    SectionSpec,
                                    operation,
                                    0,0
                                    );
                        }

                        if(!b) {
                            rc = GetLastError();
                        }
                    }
                }
            }
        } while(b && SetupFindNextMatchLine(&LineContext,Operations[i],&LineContext));
    }

    try {
        UnlockIt(DiskSpaceList);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
c0:
    SetLastError(rc);
    return(b);
}


BOOL
pAddOrRemoveFileFromSectionToDiskSpaceList(
    IN OUT PDISK_SPACE_LIST DiskSpaceList,
    IN     HINF             LayoutInf,
    IN     PINFCONTEXT      LineInSection,      OPTIONAL
    IN     PCTSTR           FileName,           OPTIONAL
    IN     PCTSTR           TargetDirectory,
    IN     UINT             Operation,
    IN     BOOL             Add
    )
{
    PCTSTR TargetFilename;
    TCHAR FullTargetPath[MAX_PATH];
    DWORD FileSize;
    BOOL b;
    DWORD rc;

    //
    // Get the target filename out of the line.
    // Field 1 is the target so there must be one for the line to be valid.
    //
    if(TargetFilename = LineInSection ? pSetupFilenameFromLine(LineInSection,FALSE) : FileName) {

        //
        // Form the full target path by concatenating the target dir
        // for this section and the target filename.
        //
        lstrcpyn(FullTargetPath,TargetDirectory,MAX_PATH);
        ConcatenatePaths(FullTargetPath,TargetFilename,MAX_PATH,NULL);

        if(Add) {
            //
            // Fetch the size of the target file and add the operation
            // to the disk space list.
            //
            if(_SetupGetSourceFileSize(LayoutInf,LineInSection,FileName,NULL,&FileSize,0)) {

                b = pSetupAddToDiskSpaceList(
                        DiskSpaceList,
                        FullTargetPath,
                        (LONGLONG)(LONG)FileSize,
                        Operation
                        );

                if(!b) {
                    rc = GetLastError();
                }
            } else {
                b = FALSE;
                rc = GetLastError();
            }
        } else {
            //
            // Remove the operation from the disk space list.
            //
            b = pSetupRemoveFromDiskSpaceList(
                    DiskSpaceList,
                    FullTargetPath,
                    Operation
                    );

            if (!b) {
                rc = GetLastError();
            }
        }
    } else {
        b = FALSE;
        rc = ERROR_INVALID_DATA;
    }

    SetLastError(rc);
    return(b);
}


BOOL
pSetupAddToDiskSpaceList(
    IN  PDISK_SPACE_LIST DiskSpaceList,
    IN  PCTSTR           TargetFilespec,
    IN  LONGLONG         FileSize,
    IN  UINT             Operation
    )

/*++

Routine Description:

    Worker routine to add an item to a disk space list.
    Assumes locking is done by the caller.

Arguments:

    DiskSpaceList - specifies pointer to disk space list structure
        created by SetupCreateDiskSpaceList().

    TargetFilespec - specifies filename of the file to add
        to the disk space list. This will generally be a full win32
        path, though this is not a requirement. If it is not then
        standard win32 path semantics apply.

    FileSize - supplies the (uncompressed) size of the file as it will
        exist on the target when copied. Ignored for FILEOP_DELETE.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    TCHAR Buffer[MAX_PATH];
    DWORD rc;
    BOOL b;
    PTSTR DirPart;
    PTSTR FilePart;
    PTSTR drivespec;
    TCHAR drivelet[4];
    LONGLONG ExistingFileSize;
    XDRIVE xDrive;
    XDIRECTORY xDir;
    XFILE xFile;
    DWORD StringLength;
    DWORD Hash;
    LONG l;
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD TotalClusters;
    DWORD FreeClusters;

    if((Operation != FILEOP_DELETE) && (Operation != FILEOP_COPY) && (Operation != (UINT)(-1))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = NO_ERROR;

    try {
        rc = pParsePath(
                TargetFilespec,
                Buffer,
                &DirPart,
                &FilePart,
                &ExistingFileSize,
                DiskSpaceList->Flags
                );

        if(rc != NO_ERROR) {
            goto c0;
        }

        //
        // If we're not just doing the adjust case, drivespecs are not
        // acceptable.
        //
        if((Operation != (UINT)(-1)) && (*FilePart == 0)) {
            rc = ERROR_INVALID_PARAMETER;
            goto c0;
        }

        //
        // See whether the drive is already present in the drive list.
        //

        MYASSERT(DiskSpaceList->DrivesTable);

        l = pStringTableLookUpString(
                DiskSpaceList->DrivesTable,
                Buffer,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xDrive,
                sizeof(XDRIVE)
                );

        if(l == -1) {
            //
            // Determine cluster size for the drive and then add the drive
            // to the drive list and create a string table for the
            // directory list for this drive.
            //
            if(xDrive.DirsTable = pStringTableInitialize(sizeof(XDIRECTORY))) {
                //
                // The API is a little picky about what it is passed.
                // For the local drive case we have to use x:\ but pParsePath
                // sets things up so it's x:.
                //
                if(Buffer[1] == TEXT(':')) {
                    drivelet[0] = Buffer[0];
                    drivelet[1] = Buffer[1];
                    drivelet[2] = TEXT('\\');
                    drivelet[3] = 0;
                    drivespec = drivelet;
                } else {
                    drivespec = Buffer;
                }

                b = GetDiskFreeSpace(
                        drivespec,
                        &SectorsPerCluster,
                        &BytesPerSector,
                        &FreeClusters,
                        &TotalClusters
                        );

                if(!b) {
                    //
                    // This should probably be an error but there could be
                    // cases where people want to queue files say to a UNC path
                    // that isn't accessible now or something. Use reasonable defaults.
                    //
                    SectorsPerCluster = 1;
                    BytesPerSector = 512;
                    FreeClusters = 0;
                    TotalClusters = 0;
                }

                xDrive.SpaceRequired = 0;
                xDrive.Slop = 0;
                xDrive.BytesPerCluster = SectorsPerCluster * BytesPerSector;

                l = pStringTableAddString(
                        DiskSpaceList->DrivesTable,
                        Buffer,
                        STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                        &xDrive,
                        sizeof(XDRIVE)
                        );

                if(l == -1) {
                    pStringTableDestroy(xDrive.DirsTable);
                }
            }
        }

        if(l == -1) {
            //
            // Assume OOM.
            //
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto c0;
        }

        if(Operation == (UINT)(-1)) {
            //
            // Only want to add the drive. Adjust the slop for the drive.
            // rc is already set to NO_ERROR.
            //
            xDrive.Slop += FileSize;
            if((DiskSpaceList->Flags & SPDSL_DISALLOW_NEGATIVE_ADJUST) && (xDrive.Slop < 0)) {
                xDrive.Slop = 0;
            }

            pStringTableSetExtraData(
                DiskSpaceList->DrivesTable,
                l,
                &xDrive,
                sizeof(XDRIVE)
                );

            goto c0;
        }

        //
        // Adjust sizes to account for cluster size.
        //
        if(FileSize % xDrive.BytesPerCluster) {
            FileSize += xDrive.BytesPerCluster - (FileSize % xDrive.BytesPerCluster);
        }
        if((ExistingFileSize != -1) && (ExistingFileSize % xDrive.BytesPerCluster)) {
            ExistingFileSize += xDrive.BytesPerCluster - (ExistingFileSize % xDrive.BytesPerCluster);
        }

        //
        // OK, xDrive has the drive info relevent for this file.
        // Now handle the directory part. First see whether the directory
        // is already present in the drive list.
        //
        l = pStringTableLookUpString(
                xDrive.DirsTable,
                DirPart,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xDir,
                sizeof(XDIRECTORY)
                );

        if(l == -1) {
            //
            // Add the directory to the directory string table.
            //
            if(xDir.FilesTable = pStringTableInitialize(sizeof(XFILE))) {

                xDir.SpaceRequired = 0;

                l = pStringTableAddString(
                        xDrive.DirsTable,
                        DirPart,
                        STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                        &xDir,
                        sizeof(XDIRECTORY)
                        );

                if(l == -1) {
                    pStringTableDestroy(xDir.FilesTable);
                }
            }
        }

        if(l == -1) {
            //
            // Assume OOM.
            //
            rc = ERROR_NOT_ENOUGH_MEMORY;
            goto c0;
        }

        //
        // Finally, deal with the file itself.
        // First see if it's in the list already.
        //
        l = pStringTableLookUpString(
                xDir.FilesTable,
                FilePart,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xFile,
                sizeof(XFILE)
                );

        if(l == -1) {
            //
            // The file is not already in there so put it in.
            //
            xFile.CurrentSize = ExistingFileSize;
            xFile.NewSize = (Operation == FILEOP_DELETE) ? -1 : FileSize;

            l = pStringTableAddString(
                    xDir.FilesTable,
                    FilePart,
                    STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                    &xFile,
                    sizeof(XFILE)
                    );

            if(l == -1) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                goto c0;
            }

        } else {

            if((xFile.CurrentSize == -1) && (xFile.NewSize == -1)) {
                //
                // This is a special "no-op" coding.
                //
                // The file is in there, but either the file was previously added
                // for a delete op but it didn't exist on the disk, or it was removed
                // via SetupRemoveFromDiskSpaceList().
                //
                xFile.CurrentSize = ExistingFileSize;
                xFile.NewSize = (Operation == FILEOP_DELETE) ? -1 : FileSize;

            } else {

                //
                // File is already in there. Remembering that deletes are done
                // before copies when a file queue is committed and assuming
                // that operations are put on the disk list in the same order they
                // will eventually be done on the file queue, there are 4 cases:
                //
                // 1) On list as delete, caller wants to delete. Just refresh
                //    the existing file size in case it changed.
                //
                // 2) On list as delete, caller wants to copy. We treat this case
                //    as a copy and override the existing info on the disk space list.
                //
                // 3) On list as copy, caller wants to delete. At commit time the file
                //    will be deleted but then later copied; just refresh the existing
                //    file size, in case it changed.
                //
                // 4) On list as copy, caller wants to copy. Override existing
                //    info in this case.
                //
                // This actually boils down to the following: Always refresh the
                // existing file size, and if the caller wants a copy, then
                // remember the new size.
                //
                xFile.CurrentSize = ExistingFileSize;
                if(Operation == FILEOP_COPY) {

                    xFile.NewSize = FileSize;
                }
            }

            pStringTableSetExtraData(xDir.FilesTable,l,&xFile,sizeof(XFILE));
        }

        c0:

        ;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}


BOOL
pSetupRemoveFromDiskSpaceList(
    IN  PDISK_SPACE_LIST DiskSpace,
    IN  PCTSTR           TargetFilespec,
    IN  UINT             Operation
    )

/*++

Routine Description:

    Worker routine to remove a single delete or copy operation from a
    disk space list.

    Assumes locking is handled by the caller.

Arguments:

    DiskSpaceList - specifies pointer to disk space list structure created by
        SetupCreateDiskSpaceList().

    TargetFilespec - specifies filename of the file to remove from
        the disk space list. This will generally be a full win32
        path, though this is not a requirement. If it is not then
        standard win32 path semantics apply.

    Operation - one of FILEOP_DELETE or FILEOP_COPY.

Return Value:

    If the file was not in the list, the routine returns TRUE and
    GetLastError() returns ERROR_INVALID_DRIVE or ERROR_INVALID_NAME.
    If the file was in the list then upon success the routine returns
    TRUE and GetLastError() returns NO_ERROR.

    If the routine fails for some other reason it returns FALSE and GetLastError()
    can be used to fetch extended error info.

--*/

{
    DWORD rc;
    BOOL b;
    TCHAR Buffer[MAX_PATH];
    PTSTR DirPart;
    PTSTR FilePart;
    LONGLONG ExistingFileSize;
    LONG l;
    DWORD StringLength;
    DWORD Hash;
    XDRIVE xDrive;
    XDIRECTORY xDir;
    XFILE xFile;

    if((Operation != FILEOP_DELETE) && (Operation != FILEOP_COPY)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    rc = NO_ERROR;
    b = TRUE;

    try {
        //
        // Split up the path into its constituent components.
        //
        rc = pParsePath(
                TargetFilespec,
                Buffer,
                &DirPart,
                &FilePart,
                &ExistingFileSize,
                DiskSpace->Flags
                );

        if(rc != NO_ERROR) {
            goto c0;
        }

        //
        // Drivespecs alone are not acceptable.
        //
        if(*FilePart == 0) {
            rc = ERROR_INVALID_PARAMETER;
            goto c0;
        }

        //
        // Follow the trail down to the file string table.
        //

        MYASSERT(DiskSpace->DrivesTable);

        l = pStringTableLookUpString(
                DiskSpace->DrivesTable,
                Buffer,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xDrive,
                sizeof(XDRIVE)
                );

        if(l == -1) {
            //
            // Return success but set last error to indicate condition.
            //
            rc = ERROR_INVALID_DRIVE;
            goto c0;
        }

        MYASSERT(xDrive.DirsTable);

        l = pStringTableLookUpString(
                xDrive.DirsTable,
                DirPart,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xDir,
                sizeof(XDIRECTORY)
                );

        if(l == -1) {
            //
            // Return success but set last error to indicate condition.
            //
            rc = ERROR_INVALID_NAME;
            goto c0;
        }

        MYASSERT(xDir.FilesTable);

        l = pStringTableLookUpString(
                xDir.FilesTable,
                FilePart,
                &StringLength,
                &Hash,
                NULL,
                STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                &xFile,
                sizeof(XFILE)
                );

        if(l == -1) {
            //
            // Return success but set last error to indicate condition.
            //
            rc = ERROR_INVALID_NAME;
            goto c0;
        }

        //
        // Set special 'no-op' code for this file if the operations match.
        //
        if(Operation == FILEOP_DELETE) {
            if(xFile.NewSize == -1) {
                xFile.CurrentSize = -1;
            }
        } else {
            if(xFile.NewSize != -1) {
                xFile.NewSize = -1;
                xFile.CurrentSize = -1;
            }
        }

        pStringTableSetExtraData(xDir.FilesTable,l,&xFile,sizeof(XFILE));

        c0:

        ;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}


BOOL
pStringTableCBEnumDrives(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Internal routine used as the callback when enumerating drives
    in the disk space list. Writes the drivespec into a buffer
    supplies to the enumeration routine.

Arguments:

Return Value:

--*/

{
    PRETURN_BUFFER_INFO p;
    UINT Length;
    BOOL b;
    PCVOID string;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(ExtraData);
    UNREFERENCED_PARAMETER(ExtraDataSize);

    p = (PRETURN_BUFFER_INFO)lParam;

#ifdef UNICODE
    if(!p->IsUnicode) {
        if(string = UnicodeToAnsi(String)) {
            Length = lstrlenA(string) + 1;
        } else {
            return(FALSE);
        }
    } else
#endif
    {
        string = String;
        Length = lstrlen(string) + 1;
    }

    p->RequiredSize += Length;

    if(p->ReturnBuffer) {

        if(p->RequiredSize <= p->ReturnBufferSize) {

            //
            // There's still room in the caller's buffer for this drive spec.
            //
#ifdef UNICODE
            if(!p->IsUnicode) {
                lstrcpyA((PSTR)p->ReturnBuffer+p->RequiredSize-Length,string);
            } else
#endif
            lstrcpy((PTSTR)p->ReturnBuffer+p->RequiredSize-Length,string);

            b = TRUE;

        } else {
            //
            // Buffer is too small. Abort the enumeration.
            //
            b = FALSE;
        }
    } else {
        //
        // No buffer: just update the length required.
        //
        b = TRUE;
    }

#ifdef UNICODE
    if(string != String) {
        MyFree(string);
    }
#endif
    return(b);
}


BOOL
pStringTableCBDelDrives(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Internal routine used as the callback when calling pStringTableEnum
    to determine which drives are part of a disk space list.
    Enumerates directories on the drive, and then deletes the drives
    string table.

Arguments:

Return Value:

--*/

{
    PXDRIVE xDrive;
    XDIRECTORY xDir;
    BOOL b;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(ExtraDataSize);
    UNREFERENCED_PARAMETER(lParam);

    //
    // The extra data for the drives table is an XDRIVE structure.
    //
    xDrive = ExtraData;

    //
    // Enumerate the directory table for this drive. This destroys
    // all of *those* string tables.
    //
    if(xDrive->DirsTable) {
        b = pStringTableEnum(
                xDrive->DirsTable,
                &xDir,
                sizeof(XDIRECTORY),
                pStringTableCBDelDirs,
                0
                );

        pStringTableDestroy(xDrive->DirsTable);
    } else {
        b = FALSE;
    }

    return(b);
}


BOOL
pStringTableCBDelDirs(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Internal routine used as the callback when calling pStringTableEnum
    to determine which directories on a given drive are part of a
    disk space list. Basically we just destroy the directory's file string table.

Arguments:

Return Value:

--*/

{
    PXDIRECTORY xDir;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(ExtraDataSize);
    UNREFERENCED_PARAMETER(lParam);

    //
    // The extra data for the dirs table is an XDIRECTORY structure.
    //
    xDir = ExtraData;

    if(xDir->FilesTable) {
        pStringTableDestroy(xDir->FilesTable);
    }

    return(TRUE);
}


DWORD
pParsePath(
    IN  PCTSTR    PathSpec,
    OUT PTSTR     Buffer,
    OUT PTSTR    *DirectoryPart,
    OUT PTSTR    *FilePart,
    OUT LONGLONG *FileSize,
    IN  UINT      Flags
    )

/*++

Routine Description:

    Given a (possibly relative or incomplete) pathspec, determine
    the drive part, the directory part, and the filename parts and
    return pointers thereto.

Arguments:

    PathSpec - supplies the (possible relative) filename.

    Buffer - must be MAX_PATH TCHAR elements. Receives the full win32
        path, which is then carved up into drive, dir, and file parts.
        When the function returns, the first part of Buffer is the
        0-terminated drive spec, not including a terminating \ char.

    DirectoryPart - receives a pointer within Buffer to the first char
        in the full path (which will not be \). The string starting
        with that char will be nul-terminated.

    FilePart - receives a pointer within Buffer to the nul-terminated
        filename part (ie, the final component) of the win32 path
        (no path sep chars are involved in that part of the path).

    FileSize - receives the size of the file if it exists or -1 if not.

    Flags - specifies flags.
        SPDSL_IGNORE_DISK: this forces the routine to behave as if the file
            does not exist on-disk.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD rc;
    WIN32_FIND_DATA FindData;
    LPTSTR p;

    rc = GetFullPathName(PathSpec,
                         MAX_PATH,
                         Buffer,
                         FilePart
                        );

    if(!rc) {
        return(GetLastError());
    } else if(rc >= MAX_PATH) {
        MYASSERT(0);
        return(ERROR_BUFFER_OVERFLOW);
    }

    //
    // Get the file size, if the file exists.
    //
    if(Flags & SPDSL_IGNORE_DISK) {
        *FileSize = -1;
    } else {
        *FileSize = FileExists(Buffer,&FindData)
                  ? ((LONGLONG)FindData.nFileSizeHigh << 32) | FindData.nFileSizeLow
                  : -1;
    }

    //
    // Figure the drive part. We have no choice but to assume that
    // full paths are either x:\... or \\server\share\... because
    // there isn't any solid way to ask win32 itself what the drive
    // part of the path is.
    //
    // Stick a nul-terminator into the buffer to set off the drive part
    // once we've found it. Note that drive roots are acceptable in
    // the following forms:
    //
    //      x:
    //      x:\
    //      \\server\share
    //      \\server\share\
    //
    if(Buffer[0] && (Buffer[1] == TEXT(':'))) {
        if(Buffer[2] == 0) {
            p = &Buffer[2];
        } else {
            if(Buffer[2] == TEXT('\\')) {
                Buffer[2] = 0;
                p = &Buffer[3];
            } else {
                return(ERROR_INVALID_DRIVE);
            }
        }
    } else {
        if((Buffer[0] == TEXT('\\')) && (Buffer[1] == TEXT('\\')) && Buffer[2]
        && (p = _tcschr(&Buffer[3],TEXT('\\'))) && *(p+1) && (*(p+1) != TEXT('\\'))) {
            //
            // Dir part starts at next \, or it could be a drive root.
            //
            if(p = _tcschr(p+2,TEXT('\\'))) {
                *p++ = 0;
            } else {
                p = _tcschr(p+2,0);
            }
        } else {
            return(ERROR_INVALID_DRIVE);
        }
    }

    //
    // If we have a drive root, we're done. Set the dir and file parts
    // to point at an empty string and return.
    //
    if(*p == 0) {
        *DirectoryPart = p;
        *FilePart = p;
        return(NO_ERROR);
    }

    if(_tcschr(p,TEXT('\\'))) {
        //
        // There are at least 2 path components, so we have
        // a directory and filename. We need to nul-terminate
        // the directory part.
        //
        *DirectoryPart = p;
        *(*FilePart - 1) = 0;
    } else {
        //
        // There's only the one path component, so we have a file
        // at the root of the drive. FilePart is already set from
        // the call to GetFullPathName above. Set DirectoryPart
        // to a nul-terminator to make it an empty string.
        //
        *DirectoryPart = Buffer+lstrlen(Buffer);
    }

    return(NO_ERROR);
}


BOOL
pStringTableCBRecalcFiles(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PXFILE xFile;
    LONGLONG Delta;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(ExtraDataSize);
    UNREFERENCED_PARAMETER(lParam);

    //
    // Extra data points to an XFILE.
    //
    xFile = ExtraData;

    //
    // Calculate the additional space the new file will require
    // or the space that will be freed after the file is copied/deleted.
    //
    if(xFile->NewSize == -1) {
        //
        // File is being deleted. Account for the special 'no-op' coding.
        //
        Delta = (xFile->CurrentSize == -1) ? 0 : (0 - xFile->CurrentSize);

    } else {
        //
        // File is being copied. Account for the fact that the file might not
        // already exist on the disk.
        //
        Delta = (xFile->CurrentSize == -1) ? xFile->NewSize : (xFile->NewSize - xFile->CurrentSize);
    }

    //
    // Update running accumulated total.
    //
    *(LONGLONG *)lParam += Delta;

    return(TRUE);
}


BOOL
pStringTableCBRecalcDirs(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PXDIRECTORY xDir;
    XFILE xFile;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(StringId);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(ExtraDataSize);
    UNREFERENCED_PARAMETER(lParam);

    //
    // Extra data points to an XDIRECTORY.
    //
    xDir = ExtraData;

    xDir->SpaceRequired = 0;

    pStringTableEnum(
        xDir->FilesTable,
        &xFile,
        sizeof(XFILE),
        pStringTableCBRecalcFiles,
        (LPARAM)&xDir->SpaceRequired
        );

    //
    // Update running accumulated total.
    //
    *(LONGLONG *)lParam += xDir->SpaceRequired;

    return(TRUE);
}


BOOL
pStringTableCBZeroDirsTableMember(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

    Standard string table callback arguments.

Return Value:

--*/

{
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(lParam);

    if(lParam) {
        ((PXDIRECTORY)ExtraData)->FilesTable = NULL;
    } else {
        ((PXDRIVE)ExtraData)->DirsTable = NULL;
    }

    MYASSERT(StringTable);

    pStringTableSetExtraData(StringTable,StringId,ExtraData,ExtraDataSize);
    return(TRUE);
}


BOOL
pStringTableCBDupMemberStringTable2(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

    Standard string table callback arguments.

Return Value:

--*/

{
    PXDIRECTORY xDir;
    BOOL b;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(String);

    //
    // Extra data is the XDIRECTORY structure in the old string table.
    //
    xDir = ExtraData;

    //
    // Duplicate the old FilesTable string table into the new table.
    // We can reuse the xDir buffer.
    //
    xDir->FilesTable = pStringTableDuplicate(xDir->FilesTable);
    if(!xDir->FilesTable) {
        return(FALSE);
    }

    pStringTableSetExtraData((PVOID)lParam,StringId,ExtraData,ExtraDataSize);
    return(TRUE);
}


BOOL
pStringTableCBDupMemberStringTable(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

    Standard string table callback arguments.

Return Value:

--*/

{
    PXDRIVE xDrive;
    XDIRECTORY xDir;
    BOOL b;
    PVOID OldTable;

    UNREFERENCED_PARAMETER(StringTable);
    UNREFERENCED_PARAMETER(String);

    //
    // Extra data is the XDRIVE structure in the old string table.
    //
    xDrive = ExtraData;

    //
    // Duplicate the old DirsTable string table into the new table.
    // We can reuse the xDrive buffer.
    //
    OldTable = xDrive->DirsTable;
    xDrive->DirsTable = pStringTableDuplicate(xDrive->DirsTable);
    if(!xDrive->DirsTable) {
        return(FALSE);
    }

    pStringTableSetExtraData((PVOID)lParam,StringId,ExtraData,ExtraDataSize);

    //
    // Now zero out the FilesTable members of the XDIRECTORY extra data
    // items in DirsTable string table.
    //
    pStringTableEnum(
        xDrive->DirsTable,
        &xDir,
        sizeof(XDIRECTORY),
        pStringTableCBZeroDirsTableMember,
        1
        );

    //
    // Finally, take advantage of the fact that the ids in the table we just
    // duplicated are the same in the old and new tables, to iterate the
    // old table to duplicate its FilesTable string tables into the new
    // string table. Clean up if failure.
    //
    b = pStringTableEnum(
            OldTable,
            &xDir,
            sizeof(XDIRECTORY),
            pStringTableCBDupMemberStringTable2,
            (LPARAM)xDrive->DirsTable
            );

    if(!b) {
        //
        // Clean up.
        //
        pStringTableEnum(
            xDrive->DirsTable,
            &xDir,
            sizeof(XDIRECTORY),
            pStringTableCBDelDirs,
            0
            );
    }

    return(b);
}


VOID
pRecalcSpace(
    IN OUT PDISK_SPACE_LIST DiskSpaceList,
    IN     LONG             DriveStringId
    )

/*++

Routine Description:

    Recalcuates the disk space required for a given drive by
    traversing all the dirs and files that are on the space list
    for the drive and performing additions/subtractions as necessary.

    Assumes locking is handled by the caller and does not guard args.

Arguments:

    DiskSpaceList - supplies the disk space list structure created
        by SetupCreateDiskSpaceList().

    DriveStringId - supplies the string id for the drive
        (in DiskSpaceList->DrivesTable) for the drive to be updated.

Return Value:

    None.

--*/

{
    XDRIVE xDrive;
    XDIRECTORY xDir;

    if(DriveStringId == -1) {
        return;
    }

    MYASSERT(DiskSpaceList->DrivesTable);

    pStringTableGetExtraData(DiskSpaceList->DrivesTable,DriveStringId,&xDrive,sizeof(XDRIVE));

    xDrive.SpaceRequired = 0;

    pStringTableEnum(
        xDrive.DirsTable,
        &xDir,
        sizeof(XDIRECTORY),
        pStringTableCBRecalcDirs,
        (LPARAM)&xDrive.SpaceRequired
        );

    pStringTableSetExtraData(DiskSpaceList->DrivesTable,DriveStringId,&xDrive,sizeof(XDRIVE));
}
