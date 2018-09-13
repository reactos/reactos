/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fileq1.c

Abstract:

    Miscellaneous setup file queue routines.

Author:

    Ted Miller (tedm) 15-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

HSPFILEQ
WINAPI
SetupOpenFileQueue(
    VOID
    )

/*++

Routine Description:

    Create a setup file queue.

Arguments:

    None.

Return Value:

    Handle to setup file queue. INVALID_HANDLE_VALUE if error occurs (GetLastError reports the error)

--*/

{
    PSP_FILE_QUEUE Queue = NULL;
    DWORD rc;
    DWORD status = ERROR_INVALID_DATA;

    try {
        //
        // Allocate a queue structure.
        //
        Queue = MyMalloc(sizeof(SP_FILE_QUEUE));
        if(!Queue) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        ZeroMemory(Queue,sizeof(SP_FILE_QUEUE));
    
        //
        // Create a string table for this queue.
        //
        Queue->StringTable = StringTableInitialize();
        if(!Queue->StringTable) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
    
        Queue->TargetLookupTable = StringTableInitializeEx( sizeof(SP_TARGET_ENT), 0 );
        if(!Queue->TargetLookupTable) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }
        Queue->BackupInfID = -1;    // no Backup INF
    
        Queue->Flags = FQF_TRY_SIS_COPY;
        Queue->SisSourceDirectory = NULL;
        Queue->SisSourceHandle = INVALID_HANDLE_VALUE;
    
        Queue->Signature = SP_FILE_QUEUE_SIG;
    
        //
        // Retrieve the codesigning policy currently in effect (policy in
        // effect is for non-driver signing behavior until we are told
        // otherwise).
        //
        Queue->DriverSigningPolicy = GetCurrentDriverSigningPolicy(FALSE);
    
        //
        // Initialize the device description field to the null string id.
        //
        Queue->DeviceDescStringId = -1;
    
        //
        // Initialize the override catalog filename to the null string id.
        //
        Queue->AltCatalogFile = -1;
    
        //
        // Createa a generic log context
        //
        rc = CreateLogContext(NULL, &Queue->LogContext);
        if (rc != NO_ERROR) {
            status = rc;
            leave;
        }
    
        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    if (status == NO_ERROR) {
        //
        // The address of the queue structure is the queue handle.
        //
        return(Queue);        
    }
    //
    // failure cleanup
    //
    if (Queue != NULL) {
        if (Queue->StringTable) {
            StringTableDestroy(Queue->StringTable);            
        }
        if (Queue->TargetLookupTable) {
            StringTableDestroy(Queue->TargetLookupTable);            
        }
        if(Queue->LogContext) {
            DeleteLogContext(Queue->LogContext);
        }
        MyFree(Queue);
    }
    //
    // return with this on error
    //
    SetLastError(status);
    return (HSPFILEQ)INVALID_HANDLE_VALUE;
}


BOOL
WINAPI
SetupCloseFileQueue(
    IN HSPFILEQ QueueHandle
    )

/*++

Routine Description:

    Destroy a setup file queue. Enqueued operations are not performed.

Arguments:

    QueueHandle - supplies handle to setup file queue to be destroyed.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  Presently, the only error that can be
    encountered is ERROR_INVALID_HANDLE or ERROR_FILEQUEUE_LOCKED, which will occur if someone (typically,
    a device installation parameter block) is referencing this queue handle.

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE Node,NextNode;
    PSP_DELAYMOVE_NODE DelayMoveNode,NextDelayMoveNode;
    PSP_UNWIND_NODE UnwindNode,NextUnwindNode;
    PSOURCE_MEDIA_INFO Media,NextMedia;
    BOOL b;
    PSPQ_CATALOG_INFO Catalog,NextCatalog;

    DWORD status = ERROR_INVALID_HANDLE;

    if (QueueHandle == NULL || QueueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    //
    // Primitive queue validation.
    //
    b = TRUE;
    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    try {
        //
        // Don't close the queue if someone is still referencing it.
        //
        if(Queue->LockRefCount) {
            //
            // BUGBUG!!! SPLOG!!!
            //
            status = ERROR_FILEQUEUE_LOCKED;
            leave;
        }
    
        //
        // we may have some unwinding to do, but assume we succeeded
        // ie, delete temp files and cleanup memory used
        //
        pSetupUnwindAll(Queue, TRUE);
    
        Queue->Signature = 0;
    
        //
        // Free the DelayMove list
        //
        for(DelayMoveNode = Queue->DelayMoveQueue; DelayMoveNode; DelayMoveNode = NextDelayMoveNode) {
            NextDelayMoveNode = DelayMoveNode->NextNode;
            MyFree(DelayMoveNode);
        }
        //
        // Free the queue nodes.
        //
        for(Node=Queue->DeleteQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        for(Node=Queue->RenameQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        // Free the backup queue nodes
        for(Node=Queue->BackupQueue; Node; Node=NextNode) {
            NextNode = Node->Next;
            MyFree(Node);
        }
        // Free the unwind queue nodes
        for(UnwindNode=Queue->UnwindQueue; UnwindNode; UnwindNode=NextUnwindNode) {
            NextUnwindNode = UnwindNode->NextNode;
            MyFree(UnwindNode);
        }
    
        //
        // Free the media structures and associated copy queues.
        //
        for(Media=Queue->SourceMediaList; Media; Media=NextMedia) {
    
            for(Node=Media->CopyQueue; Node; Node=NextNode) {
                NextNode = Node->Next;
                MyFree(Node);
            }
    
            NextMedia = Media->Next;
            MyFree(Media);
        }
    
        //
        // Free the catalog nodes.
        //
        for(Catalog=Queue->CatalogList; Catalog; Catalog=NextCatalog) {
    
            NextCatalog = Catalog->Next;
            MyFree(Catalog);
        }
    
        //
        // Free the string table.
        //
        StringTableDestroy(Queue->StringTable);
        //
        // (jamiehun) Free the target lookup table.
        //
        StringTableDestroy(Queue->TargetLookupTable);
    
        //
        // Free SIS-related fields.
        //
        if (Queue->SisSourceHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(Queue->SisSourceHandle);
        }
        if (Queue->SisSourceDirectory != NULL) {
            MyFree(Queue->SisSourceDirectory);
        }
    
        //
        // Unreference log context
        //
        DeleteLogContext(Queue->LogContext);
    
        //
        // Free the queue structure itself.
        //
        MyFree(Queue);
    
        status = NO_ERROR;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // do nothing; this just allows us to catch errors
        //
    }

    if (status != NO_ERROR) {
        SetLastError(status);
        return FALSE;        
    }
    return TRUE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupSetFileQueueAlternatePlatformA(
    IN HSPFILEQ             QueueHandle,
    IN PSP_ALTPLATFORM_INFO AlternatePlatformInfo,      OPTIONAL
    IN PCSTR                AlternateDefaultCatalogFile OPTIONAL
    )
{
    PWSTR UAlternateDefaultCatalogFile;
    DWORD Err;

    if(AlternateDefaultCatalogFile) {
        Err = CaptureAndConvertAnsiArg(AlternateDefaultCatalogFile,
                                       &UAlternateDefaultCatalogFile
                                      );
        if(Err != NO_ERROR) {
            SetLastError(Err);
            return FALSE;
        }
    } else {
        UAlternateDefaultCatalogFile = NULL;
    }

    if(SetupSetFileQueueAlternatePlatformW(QueueHandle,
                                           AlternatePlatformInfo,
                                           UAlternateDefaultCatalogFile)) {
        Err = NO_ERROR;
    } else {
        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
    }

    if(UAlternateDefaultCatalogFile) {
        MyFree(UAlternateDefaultCatalogFile);
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupSetFileQueueAlternatePlatformW(
    IN HSPFILEQ             QueueHandle,
    IN PSP_ALTPLATFORM_INFO AlternatePlatformInfo,      OPTIONAL
    IN PCWSTR               AlternateDefaultCatalogFile OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(AlternatePlatformInfo);
    UNREFERENCED_PARAMETER(AlternateDefaultCatalogFile);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupSetFileQueueAlternatePlatform(
    IN HSPFILEQ             QueueHandle,
    IN PSP_ALTPLATFORM_INFO AlternatePlatformInfo,      OPTIONAL
    IN PCTSTR               AlternateDefaultCatalogFile OPTIONAL
    )

/*++

Routine Description:

    This API associates the specified file queue with an alternate platform in
    order to allow for non-native signature verification (e.g., verifying Win98
    files on Windows NT, verifying x86 Windows NT files on Alpha, etc.).  The
    verification is done using the corresponding catalog files specified via
    platform-specific CatalogFile= entries in the source media descriptor INFs
    (i.e., INFs containing [SourceDisksNames] and [SourceDisksFiles] sections
    used when queueing files to be copied).
    
    The caller may also optionally specify a default catalog file, to be used
    for verification of files that have no associated catalog, thus would
    otherwise be globally validated (e.g., files queued up from the system
    layout.inf).  A side-effect of this is that INFs with no CatalogFile= entry
    are considered valid, even if they exist outside of %windir%\Inf.

    If this file queue is subsequently committed, the nonnative catalogs will be
    installed into the system catalog database, just as native catalogs would.

Arguments:

    QueueHandle - supplies a handle to the file queue with which the alternate
        platform is to be associated.
        
    AlternatePlatformInfo - optionally, supplies the address of a structure
        containing information regarding the alternate platform that is to be
        used for subsequent validation of files contained in the specified file
        queue.  If this parameter is not supplied, then the queue's association
        with an alternate platform is reset, and is reverted back to the default
        (i.e., native) environment.  This information is also used in 
        determining the appropriate platform-specific CatalogFile= entry to be
        used when finding out which catalog file is applicable for a particular
        source media descriptor INF.
        
    AlternateDefaultCatalogFile - optionally, supplies the full path to the 
        catalog file to be used for verification of files contained in the 
        specified file queue that are not associated with any particular catalog
        (hence would normally be globally validated).

        If this parameter is NULL, then the file queue will no longer be
        associated with any 'override' catalog, and all validation will take
        place normally (i.e., using the standard rules for digital signature
        verification via system-supplied and 3rd-party provided INFs/CATs).
        
        If this alternate default catalog is still associated with the file
        queue at commit time, it will be installed using its present name, and
        will overwrite any existing installed catalog file having that name.
        
Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PSP_FILE_QUEUE Queue;
    DWORD Err;
    TCHAR PathBuffer[MAX_PATH];
    DWORD RequiredSize;
    PTSTR TempCharPtr;
    LONG AltCatalogStringId;
    PSPQ_CATALOG_INFO CatalogNode;
    LPCTSTR InfFullPath;

    Err = NO_ERROR; // assume success

    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;

        //
        // Now validate the AlternatePlatformInfo parameter.
        //
        if(AlternatePlatformInfo) {

            if(AlternatePlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO)) {
                Err = ERROR_INVALID_USER_BUFFER;
                goto clean0;
            }

            //
            // Gotta be either Windows or Windows NT
            //
            if((AlternatePlatformInfo->Platform != VER_PLATFORM_WIN32_WINDOWS) &&
               (AlternatePlatformInfo->Platform != VER_PLATFORM_WIN32_NT)) {

                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // Processor had better be either i386, alpha, or ia64
            //
            if((AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ALPHA) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64) &&
               (AlternatePlatformInfo->ProcessorArchitecture != PROCESSOR_ARCHITECTURE_ALPHA64)) {

                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // MajorVersion field must be non-zero (MinorVersion field can be
            // anything), Reserved field must be zero.
            //
            if(!AlternatePlatformInfo->MajorVersion || AlternatePlatformInfo->Reserved) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
        }

        //
        // OK, the platform info structure checks out.  Now, associate the
        // default catalog (if supplied) with the file queue, otherwise reset
        // any existing association with a default catalog.
        //
        if(AlternateDefaultCatalogFile) {

            RequiredSize = GetFullPathName(AlternateDefaultCatalogFile,
                                           SIZECHARS(PathBuffer),
                                           PathBuffer,
                                           &TempCharPtr
                                          );

            if(!RequiredSize) {
                Err = GetLastError();
                goto clean0;
            } else if(RequiredSize >= SIZECHARS(PathBuffer)) {
                MYASSERT(0);
                Err = ERROR_BUFFER_OVERFLOW;
                goto clean0;
            }

            AltCatalogStringId = StringTableAddString(Queue->StringTable,
                                            PathBuffer,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                           );
            if(AltCatalogStringId == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

        } else {
            //
            // Caller has not supplied an alternate default catalog, so reset
            // any existing association.
            //
            AltCatalogStringId = -1;
        }

        //
        // If we've been passed an AltPlatformInfo structure, then we need to
        // process each existing catalog node in our file queue and retrieve the
        // appropriate platform-specific CatalogFile= entry.
        //
        if(AlternatePlatformInfo) {

            for(CatalogNode = Queue->CatalogList; CatalogNode; CatalogNode = CatalogNode->Next) {
                //
                // Get the INF name associated with this catalog node.
                //
                InfFullPath = StringTableStringFromId(Queue->StringTable, 
                                                      CatalogNode->InfFullPath
                                                     );

                Err = pGetInfOriginalNameAndCatalogFile(NULL,
                                                        InfFullPath,
                                                        NULL,
                                                        NULL,
                                                        0,
                                                        PathBuffer,
                                                        SIZECHARS(PathBuffer),
                                                        AlternatePlatformInfo
                                                       );
                if(Err != NO_ERROR) {
                    goto clean0;
                }

                if(*PathBuffer) {
                    //
                    // We retrieved a CatalogFile= entry that's pertinent for
                    // the specified platform from the INF.
                    //
                    CatalogNode->AltCatalogFileFromInfPending = StringTableAddString(
                                                                  Queue->StringTable,
                                                                  PathBuffer,
                                                                  STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                                 );

                    if(CatalogNode->AltCatalogFileFromInfPending == -1) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }

                } else {
                    //
                    // The INF doesn't specify a CatalogFile= entry for this
                    // platform.
                    //
                    CatalogNode->AltCatalogFileFromInfPending = -1;
                }
            }

            //
            // OK, if we get to this point, then we've added all the strings to
            // the string table we need to, and we're done opening INFs.  We
            // should encounter no problems from this point forward, so it's
            // safe to commit our changes.
            //
            for(CatalogNode = Queue->CatalogList; CatalogNode; CatalogNode = CatalogNode->Next) {
                CatalogNode->AltCatalogFileFromInf = CatalogNode->AltCatalogFileFromInfPending;
            }
        }

        Queue->AltCatalogFile = AltCatalogStringId;

        //
        // Finally, update (or reset) the AltPlatformInfo structure in the queue
        // with the data the caller specified.
        //
        if(AlternatePlatformInfo) {
            CopyMemory(&(Queue->AltPlatformInfo),
                       AlternatePlatformInfo,
                       sizeof(SP_ALTPLATFORM_INFO)
                      );
            Queue->Flags |= FQF_USE_ALT_PLATFORM;
        } else {
            Queue->Flags &= ~FQF_USE_ALT_PLATFORM;
        }

        //
        // Clear the "catalog verifications done" flags in the queue, so that
        // we'll redo them the next time _SetupVerifyQueuedCatalogs is called.
        // Also, clear the FQF_DIGSIG_ERRORS_NOUI flag so that the next
        // verification error we encounter will relayed to the user (based on
        // policy).
        //
        Queue->Flags &= ~(FQF_DID_CATALOGS_OK | FQF_DID_CATALOGS_FAILED | FQF_DIGSIG_ERRORS_NOUI);

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}


BOOL
pSetupSetQueueFlags(
    IN HSPFILEQ QueueHandle,
    IN DWORD flags
    )
{
    PSP_FILE_QUEUE Queue;
    DWORD Err = NO_ERROR;
    
    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;
        Queue->Flags = flags;

        if (Queue->Flags & FQF_QUEUE_FORCE_BLOCK_POLICY) {
            Queue->DriverSigningPolicy = DRIVERSIGN_BLOCKING;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          Err = GetExceptionCode();
    }

    SetLastError(Err);
    return (Err == NO_ERROR);

}

DWORD
pSetupGetQueueFlags(
    IN HSPFILEQ QueueHandle
    )
{
    PSP_FILE_QUEUE Queue;
    
    try {
        Queue = (PSP_FILE_QUEUE)QueueHandle;
        return Queue->Flags;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return 0;

}


