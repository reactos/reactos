/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fileq3.c

Abstract:

    Setup file queue routines for enqueing delete and rename
    operations.

Author:

    Ted Miller (tedm) 15-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


BOOL
_SetupQueueDelete(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   PathPart1,
    IN PCTSTR   PathPart2,      OPTIONAL
    IN UINT     Flags
    )

/*++

Routine Description:

    Place a delete operation on a setup file queue.

    Note that delete operations are assumed to be on fixed media.
    No prompting will be performed for delete operations when the
    queue is committed.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    PathPart1 - Supplies the first part of the path of
        the file to be deleted. If PathPart2 is not specified, then
        this is the full path of the file to be deleted.

    PathPart2 - if specified, supplies the second part of the path
        of the file to be deleted. This is concatenated to PathPart1
        to form the full pathname.

    Flags - specified flags controlling delete operation.

        DELFLG_IN_USE - if the file is in use, queue it for delayed
            delete, on next reboot. Otherwise in-use files are not deleted.

        DELFLG_IN_USE1 - same behavior as DELFLG_IN_USE--used when the
            same file list section is used for both a CopyFiles and DelFiles.
            (Since DELFLG_IN_USE (0x1) is also COPYFLG_WARN_IF_SKIP!)

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE QueueNode, TempNode, PrevQueueNode;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    //
    // Allocate a queue structure.
    //
    QueueNode = MyMalloc(sizeof(SP_FILE_QUEUE_NODE));
    if(!QueueNode) {
        goto clean0;
    }

    ZeroMemory(QueueNode, sizeof(SP_FILE_QUEUE_NODE));

    //
    // Operation is delete.
    //
    QueueNode->Operation = FILEOP_DELETE;

    //
    // Initialize unused fields.
    //
    QueueNode->SourceRootPath = -1;
    QueueNode->SourcePath = -1;
    QueueNode->SourceFilename = -1;

    //
    // Set internal flag to indicate whether we should queue a delayed delete
    // for this file if it's in-use.
    //
    QueueNode->InternalFlags = (Flags & (DELFLG_IN_USE|DELFLG_IN_USE1)) ?
        IQF_DELAYED_DELETE_OK : 0;

    //
    // NOTE: When adding the following strings to the string table, we cast away
    // their CONST-ness to avoid a compiler warning.  Since we are adding them
    // case-sensitively, we are guaranteed they will not be modified.
    //

    //
    // Set up the target directory.
    //
    QueueNode->TargetDirectory = StringTableAddString(Queue->StringTable,
                                                      (PTSTR)PathPart1,
                                                      STRTAB_CASE_SENSITIVE
                                                     );
    if(QueueNode->TargetDirectory == -1) {
        goto clean1;
    }

    //
    // Set up the target filename.
    //
    if(PathPart2) {
        QueueNode->TargetFilename = StringTableAddString(Queue->StringTable,
                                                         (PTSTR)PathPart2,
                                                         STRTAB_CASE_SENSITIVE
                                                        );
        if(QueueNode->TargetFilename == -1) {
            goto clean1;
        }
    } else {
        QueueNode->TargetFilename = -1;
    }

    //
    // Link the node onto the end of the delete queue.
    //
    QueueNode->Next = NULL;
    if(Queue->DeleteQueue) {
        //
        // Check to see if this same rename operation has already been enqueued,
        // and if so, get rid of the new one, to avoid duplicates.  NOTE: We
        // don't check the "InternalFlags" field, since if the node already
        // exists in the queue (based on all the other relevant fields comparing
        // successfully), then any internal flags that were set on the
        // previously-existing node should be preserved (i.e., our new node
        // always is created with InternalFlags set to zero).
        //
        for(TempNode=Queue->DeleteQueue, PrevQueueNode = NULL;
            TempNode;
            PrevQueueNode = TempNode, TempNode=TempNode->Next) {

            if((TempNode->TargetDirectory == QueueNode->TargetDirectory) &&
               (TempNode->TargetFilename == QueueNode->TargetFilename)) {
                //
                // We've found a duplicate.  However, we need to make sure that
                // if our new node specifies "delayed delete OK", then the
                // existing node has that internal flag set as well.
                //
                MYASSERT(!(QueueNode->InternalFlags & ~IQF_DELAYED_DELETE_OK));

                if(QueueNode->InternalFlags & IQF_DELAYED_DELETE_OK) {
                    TempNode->InternalFlags |= IQF_DELAYED_DELETE_OK;
                }

                //
                // Kill the newly-created queue node and return success.
                //
                MyFree(QueueNode);
                return TRUE;
            }
        }
        MYASSERT(PrevQueueNode);
        PrevQueueNode->Next = QueueNode;
    } else {
        Queue->DeleteQueue = QueueNode;
    }

    Queue->DeleteNodeCount++;

    return(TRUE);

clean1:
    MyFree(QueueNode);
clean0:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return(FALSE);
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueDeleteA(
    IN HSPFILEQ QueueHandle,
    IN PCSTR    PathPart1,
    IN PCSTR    PathPart2       OPTIONAL
    )
{
    PWSTR p1,p2;
    DWORD d;
    BOOL b;

    b = FALSE;
    d = CaptureAndConvertAnsiArg(PathPart1,&p1);
    if(d == NO_ERROR) {

        if(PathPart2) {
            d = CaptureAndConvertAnsiArg(PathPart2,&p2);
        } else {
            p2 = NULL;
        }

        if(d == NO_ERROR) {

            b = _SetupQueueDelete(QueueHandle,p1,p2,0);
            d = GetLastError();

            if(p2) {
                MyFree(p2);
            }
        }

        MyFree(p1);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueDeleteW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR   PathPart1,
    IN PCWSTR   PathPart2       OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(PathPart1);
    UNREFERENCED_PARAMETER(PathPart2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueDelete(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   PathPart1,
    IN PCTSTR   PathPart2       OPTIONAL
    )

/*++

Routine Description:

    Place a delete operation on a setup file queue.

    Note that delete operations are assumed to be on fixed media.
    No prompting will be performed for delete operations when the
    queue is committed.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    PathPart1 - Supplies the first part of the path of
        the file to be deleted. If PathPart2 is not specified, then
        this is the full path of the file to be deleted.

    PathPart2 - if specified, supplies the second part of the path
        of the file to be deleted. This is concatenated to PathPart1
        to form the full pathname.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PTSTR p1,p2;
    DWORD d;
    BOOL b;

    b = FALSE;
    d = CaptureStringArg(PathPart1,&p1);
    if(d == NO_ERROR) {

        if(PathPart2) {
            d = CaptureStringArg(PathPart2,&p2);
        } else {
            p2 = NULL;
        }

        if(d == NO_ERROR) {

            b = _SetupQueueDelete(QueueHandle,p1,p2,0);
            d = GetLastError();

            if(p2) {
                MyFree(p2);
            }
        }

        MyFree(p1);
    }

    SetLastError(d);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueDeleteSectionA(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCSTR    Section
    )
{
    PWSTR section;
    DWORD d;
    BOOL b;

    d = CaptureAndConvertAnsiArg(Section,&section);
    if(d == NO_ERROR) {

        b = SetupQueueDeleteSectionW(QueueHandle,InfHandle,ListInfHandle,section);
        d = GetLastError();

        MyFree(section);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueDeleteSectionW(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCWSTR   Section
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ListInfHandle);
    UNREFERENCED_PARAMETER(Section);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueDeleteSection(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,   OPTIONAL
    IN PCTSTR   Section
    )

/*++

Routine Description:

    Queue an entire section in an inf file for delete. The section must be
    in delete-section format and the inf file must contain [DestinationDirs].

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    InfHandle - supplies a handle to an open inf file, that contains the
        [DestinationDirs] section.

    ListInfHandle - if specified, supplies a handle to the open inf file
        containing the section named by Section. If not specified this
        section is assumed to be in InfHandle.

    Section - supplies the name of the section to be queued for delete.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information. Some files may have been queued successfully.

--*/

{
    BOOL b;
    PTSTR TargetDirectory;
    PCTSTR TargetFilename;
    INFCONTEXT LineContext;
    DWORD SizeRequired;
    DWORD rc;
    UINT Flags;

    if(!ListInfHandle) {
        ListInfHandle = InfHandle;
    }

    //
    // The section has to exist and there sas to be at least one line in it.
    //
    b = SetupFindFirstLine(ListInfHandle,Section,NULL,&LineContext);
    if(!b) {
        try {
            if (QueueHandle != NULL
                && QueueHandle != INVALID_HANDLE_VALUE
                && ((PSP_FILE_QUEUE)QueueHandle)->Signature == SP_FILE_QUEUE_SIG) {

                WriteLogEntry(
                    ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_NOSECTION_MIN,
                    NULL,
                    Section);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
        }
        SetLastError(ERROR_SECTION_NOT_FOUND);
        return(FALSE);
    }

    //
    // Iterate every line in the section.
    //
    do {
        //
        // Get the target filename out of the line.
        //
        TargetFilename = pSetupFilenameFromLine(&LineContext,FALSE);
        if(!TargetFilename) {
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }

        //
        // Determine the target path for the file.
        //
        b = SetupGetTargetPath(InfHandle,&LineContext,NULL,NULL,0,&SizeRequired);
        if(!b) {
            return(FALSE);
        }
        TargetDirectory = MyMalloc(SizeRequired*sizeof(TCHAR));
        if(!TargetDirectory) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
        SetupGetTargetPath(InfHandle,&LineContext,NULL,TargetDirectory,SizeRequired,NULL);

        //
        // If present flags are field 4
        //
        if(!SetupGetIntField(&LineContext,4,(PINT)&Flags)) {
            Flags = 0;
        }

        //
        // Add to queue.
        //
        b = _SetupQueueDelete(QueueHandle,TargetDirectory,TargetFilename,Flags);

        rc = GetLastError();
        MyFree(TargetDirectory);

        if(!b) {
            SetLastError(rc);
            return(FALSE);
        }

    } while(SetupFindNextLine(&LineContext,&LineContext));

    return(TRUE);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueRenameA(
    IN HSPFILEQ QueueHandle,
    IN PCSTR    SourcePath,
    IN PCSTR    SourceFilename, OPTIONAL
    IN PCSTR    TargetPath,     OPTIONAL
    IN PCSTR    TargetFilename
    )
{
    PWSTR sourcepath = NULL;
    PWSTR sourcefilename = NULL;
    PWSTR targetpath = NULL;
    PWSTR targetfilename = NULL;
    DWORD d;
    BOOL b;

    b = FALSE;
    d = CaptureAndConvertAnsiArg(SourcePath,&sourcepath);
    if((d == NO_ERROR) && SourceFilename) {
        d = CaptureAndConvertAnsiArg(SourceFilename,&sourcefilename);
    }
    if((d == NO_ERROR) && TargetPath) {
        d = CaptureAndConvertAnsiArg(TargetPath,&targetpath);
    }
    if(d == NO_ERROR) {
        d = CaptureAndConvertAnsiArg(TargetFilename,&targetfilename);
    }

    if(d == NO_ERROR) {

        b = SetupQueueRenameW(QueueHandle,sourcepath,sourcefilename,targetpath,targetfilename);
        d = GetLastError();
    }

    if(sourcepath) {
        MyFree(sourcepath);
    }
    if(sourcefilename) {
        MyFree(sourcefilename);
    }
    if(targetpath) {
        MyFree(targetpath);
    }
    if(targetfilename) {
        MyFree(targetfilename);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueRenameW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR   SourcePath,
    IN PCWSTR   SourceFilename, OPTIONAL
    IN PCWSTR   TargetPath,     OPTIONAL
    IN PCWSTR   TargetFilename
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(SourcePath);
    UNREFERENCED_PARAMETER(SourceFilename);
    UNREFERENCED_PARAMETER(TargetPath);
    UNREFERENCED_PARAMETER(TargetFilename);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueRename(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   SourcePath,
    IN PCTSTR   SourceFilename, OPTIONAL
    IN PCTSTR   TargetPath,     OPTIONAL
    IN PCTSTR   TargetFilename
    )

/*++

Routine Description:

    Place a rename operation on a setup file queue.

    Note that rename operations are assumed to be on fixed media.
    No prompting will be performed for rename operations when the
    queue is committed.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    SourcePath - Supplies the source path of the file to be renamed.
        If SourceFilename is specified, this is the part part only.
        If SourceFilename is not specified, this is the fully-qualified
        path.

    SourceFilename - if specified, supplies the filename part of the
        file to be renamed. If not specified, SourcePath is the fully-
        qualified path of the file to be renamed.

    TargetPath - if specified, supplies the target directory, and the rename
        is actually a move operation. If not specified, then the rename
        takes place without moving the file.

    TargetFilename - supplies the new name (no path) of the file.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE QueueNode, TempNode, PrevQueueNode;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    //
    // Allocate a queue structure.
    //
    QueueNode = MyMalloc(sizeof(SP_FILE_QUEUE_NODE));
    if(!QueueNode) {
        goto clean0;
    }

    ZeroMemory(QueueNode, sizeof(SP_FILE_QUEUE_NODE));

    //
    // Operation is rename.
    //
    QueueNode->Operation = FILEOP_RENAME;

    //
    // Initialize unused SourceRootPath field.
    //
    QueueNode->SourceRootPath = -1;

    //
    // NOTE: When adding the following strings to the string table, we cast away
    // their CONST-ness to avoid a compiler warning.  Since we are adding them
    // case-sensitively, we are guaranteed they will not be modified.
    //

    //
    // Set up the source path.
    //
    QueueNode->SourcePath = StringTableAddString(Queue->StringTable,
                                                 (PTSTR)SourcePath,
                                                 STRTAB_CASE_SENSITIVE
                                                );
    if(QueueNode->SourcePath == -1) {
        goto clean1;
    }

    //
    // Set up the source filename.
    //
    if(SourceFilename) {
        QueueNode->SourceFilename = StringTableAddString(Queue->StringTable,
                                                         (PTSTR)SourceFilename,
                                                         STRTAB_CASE_SENSITIVE
                                                        );
        if(QueueNode->SourceFilename == -1) {
            goto clean1;
        }
    } else {
        QueueNode->SourceFilename = -1;
    }

    //
    // Set up the target directory.
    //
    if(TargetPath) {
        QueueNode->TargetDirectory = StringTableAddString(Queue->StringTable,
                                                          (PTSTR)TargetPath,
                                                          STRTAB_CASE_SENSITIVE
                                                         );
        if(QueueNode->TargetDirectory == -1) {
            goto clean1;
        }
    } else {
        QueueNode->TargetDirectory = -1;
    }

    //
    // Set up the target filename.
    //
    QueueNode->TargetFilename = StringTableAddString(Queue->StringTable,
                                                     (PTSTR)TargetFilename,
                                                     STRTAB_CASE_SENSITIVE
                                                    );
    if(QueueNode->TargetFilename == -1) {
        goto clean1;
    }


    //
    // Link the node onto the end of the rename queue.
    //
    QueueNode->Next = NULL;
    if(Queue->RenameQueue) {
        //
        // Check to see if this same rename operation has already been enqueued,
        // and if so, get rid of the new one, to avoid duplicates.  NOTE: We
        // don't check the "InternalFlags" field, since if the node already
        // exists in the queue (based on all the other relevant fields comparing
        // successfully), then any internal flags that were set on the
        // previously-existing node should be preserved (i.e., our new node
        // always is created with InternalFlags set to zero).
        //
        for(TempNode=Queue->RenameQueue, PrevQueueNode = NULL;
            TempNode;
            PrevQueueNode = TempNode, TempNode=TempNode->Next) {

            if((TempNode->SourcePath == QueueNode->SourcePath) &&
               (TempNode->SourceFilename == QueueNode->SourceFilename) &&
               (TempNode->TargetDirectory == QueueNode->TargetDirectory) &&
               (TempNode->TargetFilename == QueueNode->TargetFilename)) {
                //
                // We have a duplicate--kill the newly-created queue node and
                // return success.
                //
                MYASSERT(TempNode->StyleFlags == 0);
                MyFree(QueueNode);
                return TRUE;
            }
        }
        MYASSERT(PrevQueueNode);
        PrevQueueNode->Next = QueueNode;
    } else {
        Queue->RenameQueue = QueueNode;
    }

    Queue->RenameNodeCount++;

    return(TRUE);

clean1:
    MyFree(QueueNode);
clean0:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return(FALSE);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueRenameSectionA(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCSTR    Section
    )
{
    PWSTR section;
    DWORD d;
    BOOL b;

    d = CaptureAndConvertAnsiArg(Section,&section);
    if(d == NO_ERROR) {

        b = SetupQueueRenameSectionW(QueueHandle,InfHandle,ListInfHandle,section);
        d = GetLastError();

        MyFree(section);
    } else {
        b = FALSE;
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueRenameSectionW(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCWSTR   Section
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ListInfHandle);
    UNREFERENCED_PARAMETER(Section);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueRenameSection(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,   OPTIONAL
    IN PCTSTR   Section
    )

/*++

Routine Description:

    Queue an entire section in an inf file for delete. The section must be
    in delete-section format and the inf file must contain [DestinationDirs].

    The format of a rename list section dictates that only renames within the
    same directory is supported (ie, you cannot queue file moves with this API).

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    InfHandle - supplies a handle to an open inf file, that contains the
        [DestinationDirs] section.

    ListInfHandle - if specified, supplies a handle to the open inf file
        containing the section named by Section. If not specified this
        section is assumed to be in InfHandle.

    Section - supplies the name of the section to be queued for delete.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    BOOL b;
    INFCONTEXT LineContext;
    PCTSTR TargetFilename;
    PCTSTR SourceFilename;
    PTSTR Directory;
    DWORD SizeRequired;
    DWORD rc;

    if(!ListInfHandle) {
        ListInfHandle = InfHandle;
    }

    //
    // The section has to exist and there has to be at least one line in it.
    //
    b = SetupFindFirstLine(ListInfHandle,Section,NULL,&LineContext);
    if(!b) {
        try {
            if (QueueHandle != NULL
                && QueueHandle != INVALID_HANDLE_VALUE
                && ((PSP_FILE_QUEUE)QueueHandle)->Signature == SP_FILE_QUEUE_SIG) {

                WriteLogEntry(
                    ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_NOSECTION_MIN,
                    NULL,
                    Section);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
        }
        SetLastError(ERROR_SECTION_NOT_FOUND);
        return(FALSE);
    }

    //
    // Iterate every line in the section.
    //
    do {
        //
        // Get the target filename out of the line.
        //
        TargetFilename = pSetupFilenameFromLine(&LineContext,FALSE);
        if(!TargetFilename) {
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }
        //
        // Get source filename out of the line.
        //
        SourceFilename = pSetupFilenameFromLine(&LineContext,TRUE);
        if(!SourceFilename || (*SourceFilename == 0)) {
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }

        //
        // Determine the path of the file.
        //
        b = SetupGetTargetPath(InfHandle,&LineContext,NULL,NULL,0,&SizeRequired);
        if(!b) {
            return(FALSE);
        }
        Directory = MyMalloc(SizeRequired*sizeof(TCHAR));
        if(!Directory) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
        SetupGetTargetPath(InfHandle,&LineContext,NULL,Directory,SizeRequired,NULL);

        //
        // Add to queue.
        //
        b = SetupQueueRename(
                QueueHandle,
                Directory,
                SourceFilename,
                NULL,
                TargetFilename
                );

        rc = GetLastError();
        MyFree(Directory);

        if(!b) {
            SetLastError(rc);
            return(FALSE);
        }

    } while(SetupFindNextLine(&LineContext,&LineContext));

    return(TRUE);
}
