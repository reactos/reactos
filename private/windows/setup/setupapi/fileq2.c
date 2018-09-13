/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fileq2.c

Abstract:

    Setup file queue routines for enqueing copy operations.

Author:

    Ted Miller (tedm) 15-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#define STR_DRIVERCACHEINF  TEXT("drvindex.inf")


//
// Structure used with _SetupQueueCopy
//
typedef struct _SP_FILE_COPY_PARAMS_AEX {
    DWORD    cbSize;
    HSPFILEQ QueueHandle;
    PCSTR    SourceRootPath;     OPTIONAL
    PCSTR    SourcePath;         OPTIONAL
    PCSTR    SourceFilename;
    PCSTR    SourceDescription;  OPTIONAL
    PCSTR    SourceTagfile;      OPTIONAL
    PCSTR    TargetDirectory;
    PCSTR    TargetFilename;     OPTIONAL
    DWORD    CopyStyle;
    HINF     LayoutInf;          OPTIONAL
    PCSTR    SecurityDescriptor; OPTIONAL
    DWORD    SourceFlags;        OPTIONAL
    BOOL     SourceFlagsSet;     OPTIONAL // we need this flag since SourceFlags may be zero
    PCSTR    CacheName;
} SP_FILE_COPY_PARAMS_AEX, *PSP_FILE_COPY_PARAMS_AEX;

typedef struct _SP_FILE_COPY_PARAMS_WEX {
    DWORD    cbSize;
    HSPFILEQ QueueHandle;
    PCWSTR   SourceRootPath;     OPTIONAL
    PCWSTR   SourcePath;         OPTIONAL
    PCWSTR   SourceFilename;
    PCWSTR   SourceDescription;  OPTIONAL
    PCWSTR   SourceTagfile;      OPTIONAL
    PCWSTR   TargetDirectory;
    PCWSTR   TargetFilename;     OPTIONAL
    DWORD    CopyStyle;
    HINF     LayoutInf;          OPTIONAL
    PCWSTR   SecurityDescriptor; OPTIONAL
    DWORD    SourceFlags;        OPTIONAL
    BOOL     SourceFlagsSet;     OPTIONAL // we need this flag since SourceFlags may be zero
    PCWSTR   CacheName;
} SP_FILE_COPY_PARAMS_WEX, *PSP_FILE_COPY_PARAMS_WEX;

#ifdef UNICODE
typedef SP_FILE_COPY_PARAMS_WEX SP_FILE_COPY_PARAMSEX;
typedef PSP_FILE_COPY_PARAMS_WEX PSP_FILE_COPY_PARAMSEX;
#else
typedef SP_FILE_COPY_PARAMS_AEX SP_FILE_COPY_PARAMSEX;
typedef PSP_FILE_COPY_PARAMS_AEX PSP_FILE_COPY_PARAMSEX;
#endif




BOOL
_SetupQueueCopy(
    IN PSP_FILE_COPY_PARAMSEX CopyParams,
    IN PINFCONTEXT          LayoutLineContext, OPTIONAL
    IN HINF                 AdditionalInfs     OPTIONAL
    );

PSOURCE_MEDIA_INFO
pSetupQueueSourceMedia(
    IN OUT PSP_FILE_QUEUE      Queue,
    IN OUT PSP_FILE_QUEUE_NODE QueueNode,
    IN     LONG                SourceRootStringId,
    IN     PCTSTR              SourceDescription,   OPTIONAL
    IN     PCTSTR              SourceTagfile,       OPTIONAL
    IN     DWORD               MediaFlags
    );

BOOL
pSetupQueueSingleCopy(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,  OPTIONAL
    IN PCTSTR   SectionName,    OPTIONAL
    IN PCTSTR   SourceRootPath,
    IN PCTSTR   SourceFilename,
    IN PCTSTR   TargetFilename,
    IN DWORD    CopyStyle,
    IN PCTSTR   SecurityDescriptor,
    IN PCTSTR   CacheName
    );

BOOL
pSetupGetSourceAllInfo(
    IN  HINF         InfHandle,
    IN  PINFCONTEXT  LayoutLineContext, OPTIONAL
    IN  UINT         SourceId,
    OUT PCTSTR      *Description,
    OUT PCTSTR      *Tagfile,
    OUT PCTSTR      *RelativePath,
    OUT PUINT        SourceFlags
    );

BOOL
pIsDriverCachePresent(
    IN PCTSTR DriverName,
    IN PCTSTR SubDirectory,
    OUT PTSTR DriverBuffer
    );

BOOL
pIsFileInDriverCache(
    IN  HINF   CabInf,
    IN  PCTSTR TargetFilename,
    IN  PCTSTR SubDirectory,
    OUT PCTSTR *CacheName
    );



//
// HACK ALERT!!! HACK HACK HACK!!!!
//
// There might be an override platform specified. If this is so,
// we will look for \i386, \mips, etc as the final component of the
// specified path when queuing files, and replace it with the
// override path. This is a TOTAL HACK.
//
PCTSTR PlatformPathOverride;
CRITICAL_SECTION PlatformPathOverrideCritSect;

VOID
pSetupInitPlatformPathOverrideSupport(
    IN BOOL Init
    )
{
    if(Init) {
        InitializeCriticalSection(&PlatformPathOverrideCritSect);
    } else {
        DeleteCriticalSection(&PlatformPathOverrideCritSect);
        if( PlatformPathOverride )
            MyFree(PlatformPathOverride);
    }
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupSetPlatformPathOverrideA(
    IN PCSTR Override   OPTIONAL
    )
{
    BOOL b;
    DWORD rc;
    PCWSTR p;

    if(Override) {
        rc = CaptureAndConvertAnsiArg(Override,&p);
    } else {
        p = NULL;
        rc = NO_ERROR;
    }

    if(rc == NO_ERROR) {
        b = SetupSetPlatformPathOverrideW(p);
        rc = GetLastError();
    } else {
        b = FALSE;
    }

    if(p) {
        MyFree(p);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupSetPlatformPathOverrideW(
    IN PCWSTR Override  OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Override);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupSetPlatformPathOverride(
    IN PCTSTR Override  OPTIONAL
    )
{
    BOOL b;
    DWORD rc;

    EnterCriticalSection(&PlatformPathOverrideCritSect);

    if(Override) {
        if(PlatformPathOverride) {
            MyFree(PlatformPathOverride);
            PlatformPathOverride = NULL;
        }

        try {
            b = ((PlatformPathOverride = DuplicateString(Override)) != NULL);
            if(!b) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            b = FALSE;
            rc = ERROR_INVALID_PARAMETER;
        }
    } else {
        if(PlatformPathOverride) {
            MyFree(PlatformPathOverride);
            PlatformPathOverride = NULL;
        }
        b = TRUE;
    }

    LeaveCriticalSection(&PlatformPathOverrideCritSect);

    if(!b) {
        SetLastError(rc);
    }
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueCopyA(
    IN HSPFILEQ QueueHandle,
    IN PCSTR    SourceRootPath,     OPTIONAL
    IN PCSTR    SourcePath,         OPTIONAL
    IN PCSTR    SourceFilename,
    IN PCSTR    SourceDescription,  OPTIONAL
    IN PCSTR    SourceTagfile,      OPTIONAL
    IN PCSTR    TargetDirectory,
    IN PCSTR    TargetFilename,     OPTIONAL
    IN DWORD    CopyStyle
    )
{
    PCWSTR sourceRootPath;
    PCWSTR sourcePath;
    PCWSTR sourceFilename;
    PCWSTR sourceDescription;
    PCWSTR sourceTagfile;
    PCWSTR targetDirectory;
    PCWSTR targetFilename;
    BOOL b;
    DWORD rc;
    SP_FILE_COPY_PARAMS_WEX CopyParams = {0};

    sourceRootPath = NULL;
    sourcePath = NULL;
    sourceFilename = NULL;
    sourceDescription = NULL;
    sourceTagfile = NULL;
    targetDirectory = NULL;
    targetFilename = NULL;
    rc = NO_ERROR;
    b = FALSE;

    if(SourceRootPath) {
        rc = CaptureAndConvertAnsiArg(SourceRootPath,&sourceRootPath);
    }
    if((rc == NO_ERROR) && SourcePath) {
        rc = CaptureAndConvertAnsiArg(SourcePath,&sourcePath);
    }
    if((rc == NO_ERROR) && SourceFilename) {
        rc = CaptureAndConvertAnsiArg(SourceFilename,&sourceFilename);
    }
    if((rc == NO_ERROR) && SourceDescription) {
        rc = CaptureAndConvertAnsiArg(SourceDescription,&sourceDescription);
    }
    if((rc == NO_ERROR) && SourceTagfile) {
        rc = CaptureAndConvertAnsiArg(SourceTagfile,&sourceTagfile);
    }
    if((rc == NO_ERROR) && TargetDirectory) {
        rc = CaptureAndConvertAnsiArg(TargetDirectory,&targetDirectory);
    }
    if((rc == NO_ERROR) && TargetFilename) {
        rc = CaptureAndConvertAnsiArg(TargetFilename,&targetFilename);
    }

    if(rc == NO_ERROR) {

        CopyParams.cbSize = sizeof(SP_FILE_COPY_PARAMS_WEX);
        CopyParams.QueueHandle = QueueHandle;
        CopyParams.SourceRootPath = sourceRootPath;
        CopyParams.SourcePath = sourcePath;
        CopyParams.SourceFilename = sourceFilename;
        CopyParams.SourceDescription = sourceDescription;
        CopyParams.SourceTagfile = sourceTagfile;
        CopyParams.TargetDirectory = targetDirectory;
        CopyParams.TargetFilename = targetFilename;
        CopyParams.CopyStyle = CopyStyle;
        CopyParams.LayoutInf = NULL;
        CopyParams.SecurityDescriptor= NULL;

        b = _SetupQueueCopy(&CopyParams, NULL, NULL);
        rc = GetLastError();
    }

    if(sourceRootPath) {
        MyFree(sourceRootPath);
    }
    if(sourcePath) {
        MyFree(sourcePath);
    }
    if(sourceFilename) {
        MyFree(sourceFilename);
    }
    if(sourceDescription) {
        MyFree(sourceDescription);
    }
    if(sourceTagfile) {
        MyFree(sourceTagfile);
    }
    if(targetDirectory) {
        MyFree(targetDirectory);
    }
    if(targetFilename) {
        MyFree(targetFilename);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueCopyW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR   SourceRootPath,     OPTIONAL
    IN PCWSTR   SourcePath,         OPTIONAL
    IN PCWSTR   SourceFilename,
    IN PCWSTR   SourceDescription,  OPTIONAL
    IN PCWSTR   SourceTagfile,      OPTIONAL
    IN PCWSTR   TargetDirectory,
    IN PCWSTR   TargetFilename,     OPTIONAL
    IN DWORD    CopyStyle
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(SourcePath);
    UNREFERENCED_PARAMETER(SourceFilename);
    UNREFERENCED_PARAMETER(SourceDescription);
    UNREFERENCED_PARAMETER(SourceTagfile);
    UNREFERENCED_PARAMETER(TargetDirectory);
    UNREFERENCED_PARAMETER(TargetFilename);
    UNREFERENCED_PARAMETER(CopyStyle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueCopy(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   SourceRootPath,     OPTIONAL
    IN PCTSTR   SourcePath,         OPTIONAL
    IN PCTSTR   SourceFilename,
    IN PCTSTR   SourceDescription,  OPTIONAL
    IN PCTSTR   SourceTagfile,      OPTIONAL
    IN PCTSTR   TargetDirectory,
    IN PCTSTR   TargetFilename,     OPTIONAL
    IN DWORD    CopyStyle
    )

/*++

Routine Description:

    Place a copy operation on a setup file queue.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    SourceRootPath - Supplies the root of the source for this copy,
        such as A:\ or \\FOO\BAR\BAZ.  If this parameter isn't supplied, then
        this queue node will be added to a media descriptor's queue that
        matches on SourceDescription and SourceTagfile.  (This merge will take
        place regardless of whether or not the media descriptor entry was
        already in the queue prior to calling SetupQueueCopy.)

        If there is no matching media descriptor that contains SourceRootPath
        information, the path will be set to the directory where the system was
        installed from.

    SourcePath - if specified, supplies the path relative to SourceRootPath
        where the file can be found.

    SourceFilename - supplies the filename part of the file to be copied.

    SourceDescription - if specified, supplies a description of the source
        media, to be used during disk prompts.

    SourceTagfile - if specified, supplies a tag file whose presence at
        SourceRootPath indicates the presence of the source media.
        If not specified, the file itself will be used as the tag file
        if required (tagfiles are used only for removable media).

    TargetDirectory - supplies the directory where the file is to be copied.

    TargetFilename - if specified, supplies the name of the target file.
        If not specified, the target file will have the same name as the source.

    CopyStyle - supplies flags that control the behavior of the copy operation
        for this file.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    SP_FILE_COPY_PARAMSEX CopyParams = {0};

    //
    // Work is done by common worker routine
    //
    CopyParams.cbSize = sizeof(SP_FILE_COPY_PARAMSEX);
    CopyParams.QueueHandle = QueueHandle;
    CopyParams.SourceRootPath = SourceRootPath;
    CopyParams.SourcePath = SourcePath;
    CopyParams.SourceFilename = SourceFilename;
    CopyParams.SourceDescription = SourceDescription;
    CopyParams.SourceTagfile = SourceTagfile;
    CopyParams.TargetDirectory = TargetDirectory;
    CopyParams.TargetFilename = TargetFilename;
    CopyParams.CopyStyle = CopyStyle;
    CopyParams.LayoutInf = NULL;
    CopyParams.SecurityDescriptor= NULL;
    CopyParams.CacheName = NULL;

    return(_SetupQueueCopy(&CopyParams, NULL, NULL));
}


BOOL
_SetupQueueCopy(
    IN PSP_FILE_COPY_PARAMSEX CopyParams,
    IN PINFCONTEXT            LayoutLineContext, OPTIONAL
    IN HINF                   AdditionalInfs     OPTIONAL
    )

/*++

Routine Description:

    Worker routine for SetupQueueCopy and friends.

Arguments:

    CopyParams - supplies a structure with information about the file
        to be queued. Fields are used as follows.

        cbSize - must be sizeof(SP_FILE_COPY_PARAMS). The caller should
            have verified this before calling this routine.

        QueueHandle - supplies a handle to a setup file queue, as returned
            by SetupOpenFileQueue.

        SourceRootPath - Supplies the root of the source for this copy,
            such as A:\ or \\FOO\BAR\BAZ.   If this field is NULL, then this
            queue node will be added to a media descriptor's queue that matches
            on SourceDescription and SourceTagfile.  (This merge will take
            place regardless of whether or not the media descriptor entry was
            already in the queue prior to calling SetupQueueCopy.)

            If there is no matching media descriptor that contains
            SourceRootPath information, the path will be set to the directory
            where the system was installed from.

        SourcePath - if specified, supplies the path relative to SourceRootPath
            where the file can be found.

        SourceFilename - supplies the filename part of the file to be copied.

        SourceDescription - if specified, supplies a description of the source
            media, to be used during disk prompts.

        SourceTagfile - if specified, supplies a tag file whose presence at
            SourceRootPath indicates the presence of the source media.
            If not specified, the file itself will be used as the tag file
            if required (tagfiles are used only for removable media).

        TargetDirectory - supplies the directory where the file is to be copied.

        TargetFilename - if specified, supplies the name of the target file.  If
            not specified, the target file will have the same name as the source.

        CopyStyle - supplies flags that control the behavior of the copy
            operation for this file.

        LayoutInf - supplies the handle to the inf which contains the source
            layout info for this file, if any.

    LayoutLineContext - if specified, this argument provides the INF context
        for the [SourceDisksFiles] entry pertaining to the file to be copied.
        If not specified, the relevant [SourceDisksFiles] entry will be searched
        for in the LayoutInf handle specified in the CopyParams structure.  This
        context must be contained within either the CopyParams->LayoutInf or
        AdditionalInfs loaded INF handles (because those are the two INFs we're
        gonna lock).  The argument is used to prevent us from having to search
        for the file to be copied in a [SourceDisksFiles] section.  The caller
        has already done that, and is either handing us the context to that INF
        entry, or has passed -1 to indicate that there is no [SourceDisksFiles]
        entry.

    AdditionalInfs - if specified, supplies an additional HINF (potentially
        containing multiple append-loaded INFs) that need to be added to our
        SPQ_CATALOG_INFO list for later validation.  Do not supply this parameter
        if it is identical to the value of the LayoutInf field in the CopyParams
        structure.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE QueueNode, TempNode, PrevQueueNode;
    PSOURCE_MEDIA_INFO Source;
    TCHAR TempBuffer[MAX_PATH];
    TCHAR TempSubDir[MAX_PATH];
    TCHAR DriverCache[MAX_PATH] = {0};
    PCTSTR LastPathPart;
    PCTSTR p;
    int Size;
    DWORD d;
    HINF LayoutInfHandle;
    INFCONTEXT LineContext;
    BOOL b;
    PSPQ_CATALOG_INFO CatalogNode, PrevCatalogNode, LastOldCatalogNode;
    LONG l1,l2, l3, l4;
    PLOADED_INF pLoadedInfs[2];
    DWORD LoadedInfCount, i;
    PLOADED_INF pCurLoadedInf;
    DWORD MediaFlags;
    PCTSTR SourcePath, SourceRootPath;
    BOOL UseCache = FALSE;
#if defined(_X86_)
    BOOL ForcePlatform = FALSE;
#endif

    d = NO_ERROR;
    LoadedInfCount = 0;
    MediaFlags = 0;

    try {
        MYASSERT(CopyParams->cbSize == sizeof(SP_FILE_COPY_PARAMSEX));
        Queue = (PSP_FILE_QUEUE)CopyParams->QueueHandle;
        if (Queue->Signature != SP_FILE_QUEUE_SIG) {
            d = ERROR_INVALID_PARAMETER;
        }

        LayoutInfHandle = CopyParams->LayoutInf;

        //
        // Maintain local pointers to the SourceRootPath and SourcePath strings,
        // since we may be modifying them, and we don't want to muck with the
        // caller-supplied buffer.
        //
        SourcePath = CopyParams->SourcePath;
        if(CopyParams->SourceRootPath) {
            SourceRootPath = CopyParams->SourceRootPath;
        } else {
            SourceRootPath = SystemSourcePath;
            MediaFlags |= SMI_FLAG_NO_SOURCE_ROOT_PATH;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }
    if(d != NO_ERROR) {
        goto clean0;
    }

    //
    // Make sure that we weren't passed the same HINF in both CopyParams->LayoutInf
    // and AdditionalInfs (just adds redundant work to process the same
    // LOADED_INF list twice).
    //
    MYASSERT(!LayoutInfHandle || (LayoutInfHandle != AdditionalInfs));

    //
    // Lock inf(s). We do a whole bunch of operations on the inf later,
    // and we don't want anything changing out from under us.
    //
    if(LayoutInfHandle) {
        if(LockInf((PLOADED_INF)LayoutInfHandle)) {
            pLoadedInfs[LoadedInfCount++] = (PLOADED_INF)LayoutInfHandle;
        } else {
            d = ERROR_INVALID_HANDLE;
            goto clean0;
        }
    }

    if(AdditionalInfs) {
        if(LockInf((PLOADED_INF)AdditionalInfs)) {
            pLoadedInfs[LoadedInfCount++] = (PLOADED_INF)AdditionalInfs;
        } else {
            d = ERROR_INVALID_HANDLE;
            goto clean0;
        }
    }

    if(!(Queue->Flags & FQF_DEVICE_INSTALL)) {
        //
        // Look through all the INFs to see if any of them are device INFs.
        //
        for(i = 0; i < LoadedInfCount; i++) {

            if(IsInfForDeviceInstall(Queue->LogContext, 
                                     pLoadedInfs[i], 
                                     NULL, 
                                     NULL, 
                                     NULL)) {
                //
                // There be device INFs here!  Mark the queue accordingly.
                //
                d = MarkQueueForDeviceInstall(CopyParams->QueueHandle,
                                              (HINF)(pLoadedInfs[i]),
                                              NULL
                                             );
                if(d == NO_ERROR) {
                    break;
                } else {
                    goto clean0;
                }
            }
        }
    }

    //
    // if we have a NULL source path, then we should check 2 things:
    // 1) source flag information, which indicates if we have a service
    //    pack or CDM source location.
    // 2) if the relative source path is null, we'll go looking for
    //    a relative path in the inf file, just in case the caller didn't
    //    supply it
    //
    // note that we use the SourceFlagsSet item in the COPY_FILE structure
    // to optimize this path -- the first item contains the source flags,
    // and the second item indicates that we shouldn't bother looking for
    // any information, we've already done a search
    //
    if (MediaFlags & SMI_FLAG_NO_SOURCE_ROOT_PATH) {
        PINFCONTEXT pContext = LayoutLineContext;
        INFCONTEXT tmpContext,tmpContext2;
        UINT SourceFlags = CopyParams->SourceFlags;
        BOOL CheckSourceFlags = CopyParams->SourceFlagsSet ? FALSE : TRUE;

        //
        // check if we already have a line context for the file we're adding
        // and if we don't, then try to fetch it
        //
        if (!LayoutLineContext || LayoutLineContext == (PINFCONTEXT) -1) {
            if (LayoutInfHandle == (PINFCONTEXT) -1) {
                pContext = NULL;
            } else {
                //
                // find the sourcerootpaths section
                //
                b = _SetupGetSourceFileLocation(
                        LayoutInfHandle,
                        NULL,
                        CopyParams->SourceFilename,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        &tmpContext
                        );

                pContext = b ? &tmpContext : NULL;
            }
        }

        //
        // retrieve the source information given a line context
        //
        if (pContext && LayoutInfHandle && CheckSourceFlags) {
            TCHAR data[32];
            UINT SourceId;

            SetupGetIntField(pContext,1,&SourceId);

            if (pSetupGetSourceInfo(LayoutInfHandle,
                               LayoutLineContext,
                               SourceId,
                               SRCINFO_FLAGS,
                               data,
                               sizeof(data),
                               NULL)) {
                pAToI(data,&SourceFlags);
            }

            if (!SourcePath) {

                pSetupGetSourceInfo(LayoutInfHandle,
                               LayoutLineContext,
                               SourceId,
                               SRCINFO_PATH,
                               TempSubDir,
                               sizeof(TempSubDir),
                               NULL);

                SourcePath = TempSubDir;
            }

        }

        //
        // override the system source path with the servicepack source path
        // if the flags are set
        //
        if (SourceFlags & SRCINFO_SVCPACK_SOURCE) {
            MediaFlags |= SMI_FLAG_USE_SVCPACK_SOURCE_ROOT_PATH;
            SourceRootPath = ServicePackSourcePath;
        }

    }

    //
    // Allocate a queue structure.
    //
    QueueNode = MyMalloc(sizeof(SP_FILE_QUEUE_NODE));
    if(!QueueNode) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Operation is copy.
    //
    QueueNode->Operation = FILEOP_COPY;
    QueueNode->InternalFlags = 0;

    //
    // HACK ALERT!!! HACK HACK HACK!!!!
    //
    // There might be an override platform specified. If this is so,
    // we will look for \i386, \mips, etc as the final component of the
    // specified path, and replace it with the override path.
    // This is a TOTAL HACK.
    //
    EnterCriticalSection(&PlatformPathOverrideCritSect);
    try {
        if(PlatformPathOverride) {
            p = SourcePath ? SourcePath : SourceRootPath;
            if(LastPathPart = _tcsrchr(p,L'\\')) {
                LastPathPart++;
            } else {
                LastPathPart = p;
            }
#if defined(_AXP64_)
            if(!lstrcmpi(LastPathPart,TEXT("axp64"))) {
#elif defined(_ALPHA_)
            if(!lstrcmpi(LastPathPart,TEXT("alpha"))) {
#elif defined(_X86_)
            //
            // NEC98
            //
            // During GUI setup, source path on local disk must be "nec98",
            // so we don't override "i386".
            //
            if (IsNEC98()) {
                HKEY    hKey;
                DWORD   DataType, DataSize;
                PTSTR   ForceOverride;
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    if (QueryRegistryValue(hKey, TEXT("ForcePlatform"), &ForceOverride, &DataType, &DataSize)
                        == NO_ERROR) {
                        ForcePlatform = TRUE;
                        MyFree(ForceOverride);
                    }
                    RegCloseKey(hKey);
                }
                // If use driver cache, There is not i386 driver cache on NEC98.
                if ((CopyParams->CopyStyle & PSP_COPY_USE_DRIVERCACHE) && !lstrcmpi(PlatformPathOverride,TEXT("i386"))) {
                    ForcePlatform = TRUE;
                }
            }
            if((!IsNEC98() && (!lstrcmpi(LastPathPart,TEXT("x86")) || !lstrcmpi(LastPathPart,TEXT("i386"))))
            || (IsNEC98()  && (!lstrcmpi(LastPathPart,TEXT("nec98")) && !ForcePlatform))) {
#elif defined(_IA64_)
            if(!lstrcmpi(LastPathPart,TEXT("ia64"))) {
#endif
                Size = (int)(LastPathPart - p);
                Size = min(Size,MAX_PATH);
                Size *= sizeof(TCHAR);

                CopyMemory(TempBuffer,p,Size);
                TempBuffer[Size/sizeof(TCHAR)] = 0;

                //
                // If the path was something like "mips" then TempBuffer
                // will be empty and we don't want to introduce any extra
                // backslashes.
                //
                if(*TempBuffer) {
                    ConcatenatePaths(TempBuffer,PlatformPathOverride,MAX_PATH,NULL);
                } else {
                    lstrcpyn(TempBuffer,PlatformPathOverride,MAX_PATH);
                }

                if(SourcePath) {
                    SourcePath = TempBuffer;
                } else {
                    SourceRootPath = TempBuffer;
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }
    LeaveCriticalSection(&PlatformPathOverrideCritSect);
    if(d != NO_ERROR) {
        goto clean1;
    }

    //
    // check here if the cab-file is present on the disk.
    // if it isn't, then we fall back on the current
    // sourcerootpath
    //
    if (CopyParams->CopyStyle & PSP_COPY_USE_DRIVERCACHE) {
        if (pIsDriverCachePresent(CopyParams->CacheName,
                                  SourcePath,
                                  DriverCache)) {
            SourceRootPath = DriverCache;
            MediaFlags |= SMI_FLAG_USE_LOCAL_SOURCE_CAB;
        }

        UseCache = TRUE;
    }



    //
    // NOTE: When adding the following strings to the string table, we cast away
    // their CONST-ness to avoid a compiler warning.  Since we are adding them
    // case-sensitively, we are guaranteed they will not be modified.
    //
    try {
        //
        // Set up the source root path.
        //
        QueueNode->SourceRootPath = StringTableAddString(
                                        Queue->StringTable,
                                        (PTSTR)SourceRootPath,
                                        STRTAB_CASE_SENSITIVE
                                        );

        if(QueueNode->SourceRootPath == -1) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Set up the source path.
        //
        if(d == NO_ERROR) {
            if(SourcePath) {
                QueueNode->SourcePath = StringTableAddString(
                                            Queue->StringTable,
                                            (PTSTR)SourcePath,
                                            STRTAB_CASE_SENSITIVE
                                            );

                if(QueueNode->SourcePath == -1) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                QueueNode->SourcePath = -1;
            }
        }

        //
        // Set up the source filename.
        //
        if(d == NO_ERROR) {
            QueueNode->SourceFilename = StringTableAddString(
                                            Queue->StringTable,
                                            (PTSTR)CopyParams->SourceFilename,
                                            STRTAB_CASE_SENSITIVE
                                            );

            if(QueueNode->SourceFilename == -1) {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Set up the target directory.
        //
        if(d == NO_ERROR) {
            QueueNode->TargetDirectory = StringTableAddString(
                                            Queue->StringTable,
                                            (PTSTR)CopyParams->TargetDirectory,
                                            STRTAB_CASE_SENSITIVE
                                            );

            if(QueueNode->TargetDirectory == -1) {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Set up the target filename.
        //
        if(d == NO_ERROR) {
            QueueNode->TargetFilename = StringTableAddString(
                                            Queue->StringTable,
                                            (PTSTR)(CopyParams->TargetFilename ? CopyParams->TargetFilename
                                                                               : CopyParams->SourceFilename),
                                            STRTAB_CASE_SENSITIVE
                                            );

            if(QueueNode->TargetFilename == -1) {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Set up the Security Descriptor
        //
        if(d == NO_ERROR) {
            if( CopyParams->SecurityDescriptor){

                QueueNode->SecurityDesc = StringTableAddString(
                                              Queue->StringTable,
                                              (PTSTR)(CopyParams->SecurityDescriptor),
                                                STRTAB_CASE_SENSITIVE
                                                );

                if(QueueNode->SecurityDesc == -1) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                QueueNode->SecurityDesc = -1;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    if(d != NO_ERROR) {
        goto clean1;
    }

    //
    // Initialize a pointer to the end of the queue's current catalog info list.
    // We do this so that later, we can easily back-out our changes by truncating
    // the list after this node, and freeing all subsequent elements.
    //
    LastOldCatalogNode = Queue->CatalogList;
    if(LastOldCatalogNode) {
        while(LastOldCatalogNode->Next) {
            LastOldCatalogNode = LastOldCatalogNode->Next;
        }
    }

    //
    // Now process all members of our pLoadedInfs lists, adding each one to the
    // SPQ_CATALOG_INFO list (avoiding duplicates entries, of course).
    //
    for(i = 0; i < LoadedInfCount; i++) {

        for(pCurLoadedInf = pLoadedInfs[i]; pCurLoadedInf; pCurLoadedInf = pCurLoadedInf->Next) {
            //
            // First, get the (native) CatalogFile= entry from the version block
            // of this INF member.
            //
            if(pSetupGetCatalogFileValue(&(pCurLoadedInf->VersionBlock),
                                         TempBuffer,
                                         SIZECHARS(TempBuffer),
                                         NULL)) {

                l1 = StringTableAddString(Queue->StringTable,
                                          TempBuffer,
                                          STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                         );
                if(l1 == -1) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean2;
                }
            } else {
                //
                // This INF doesn't have a CatalogFile= entry.
                //
                l1 = -1;
            }

            //
            // If this file queue is currently setup for a platform override,
            // then retrieve that CatalogFile= entry as well.
            //
            if(Queue->Flags & FQF_USE_ALT_PLATFORM) {

                if(pSetupGetCatalogFileValue(&(pCurLoadedInf->VersionBlock),
                                             TempBuffer,
                                             SIZECHARS(TempBuffer),
                                             &(Queue->AltPlatformInfo))) {

                    l3 = StringTableAddString(Queue->StringTable,
                                              TempBuffer,
                                              STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                             );
                    if(l3 == -1) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean2;
                    }
                } else {
                    //
                    // This INF doesn't have a CatalogFile= entry.
                    //
                    l3 = -1;
                }
            } else {
                //
                // We're not in a platform override scenario.
                //
                l3 = -1;
            }

            //
            // Now, get the INF's full path.
            //
            lstrcpyn(TempBuffer, pCurLoadedInf->VersionBlock.Filename, SIZECHARS(TempBuffer));
            l2 = StringTableAddString(Queue->StringTable,
                                      TempBuffer,
                                      STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                     );
            if(l2 == -1) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean2;
            }

            //
            // Finally, retrieve the INF's original name, if different than the
            // current name.
            //
            if(pCurLoadedInf->OriginalInfName) {
                lstrcpyn(TempBuffer, pCurLoadedInf->OriginalInfName, SIZECHARS(TempBuffer));
                l4 = StringTableAddString(Queue->StringTable,
                                          TempBuffer,
                                          STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                         );
                if(l4 == -1) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean2;
                }

            } else {
                //
                // The INF's original name is the same as its current name.
                //
                l4 = -1;
            }

            b = TRUE;
            for(PrevCatalogNode=NULL, CatalogNode=Queue->CatalogList;
                CatalogNode;
                CatalogNode=CatalogNode->Next) {

                if(CatalogNode->InfFullPath == l2) {
                    //
                    // Already in there. No need to create a new node.
                    // Break out here, with CatalogNode pointing at the
                    // proper node for this catalog file.
                    //
                    // In this case, PrevCatalogNode should not be used later,
                    // but it shouldn't need to be used, since we won't be
                    // adding anything new onto the list of catalog nodes.
                    //
                    MYASSERT(CatalogNode->CatalogFileFromInf == l1);
                    MYASSERT(CatalogNode->InfOriginalName == l4);
                    b = FALSE;
                    break;
                }

                //
                // PrevCatalogNode will end up pointing to the final node
                // currently in the linked list, in the case where we need
                // to allocate a new node. This is useful so we don't have to
                // traverse the list again later when we add the new catalog
                // node to the list for this queue.
                //
                PrevCatalogNode = CatalogNode;
            }

            if(b) {
                //
                // Need to create a new catalog node.
                //
                CatalogNode = MyMalloc(sizeof(SPQ_CATALOG_INFO));
                if(!CatalogNode) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean2;
                }
                ZeroMemory(CatalogNode, sizeof(SPQ_CATALOG_INFO));
                CatalogNode->CatalogFileFromInf = l1;
                CatalogNode->InfFullPath = l2;
                CatalogNode->AltCatalogFileFromInf = l3;
                CatalogNode->InfOriginalName = l4;
                CatalogNode->AltCatalogFileFromInfPending = -1;
                CatalogNode->InfFinalPath = -1;
                //
                // Go ahead and link the new node into the list.  If we
                // encounter a failure later, we can easily back out of this by
                // truncating the list, since we know we always append, and we
                // remembered the original list tail.
                //
                if(Queue->CatalogList) {
                    PrevCatalogNode->Next = CatalogNode;
                } else {
                    Queue->CatalogList = CatalogNode;
                }
            }
        }
    }

    //
    // At this point, all the INFs involved in this installation (i.e., all INFs
    // in the HINFs we were passed in) have been added to our catalog info list,
    // if they weren't already present.  Now we need to figure out which one of
    // them should be associated with the file to be copied.
    //
    // Note that we want to get the CatalogFile= line from the actual inf
    // that contains source layout information for the file being queued.
    // If the layout inf handle references multiple append-loaded infs,
    // the simple mechanism of just looking up CatalogFile= in the [Version]
    // section using the given handle might give us the value from the
    // wrong inf.
    //
    // To deal with this, we attempt to locate the file in a [SourceDisksFiles]
    // section using the given inf handle, which gives us back a line context.
    // From the line context we can easily get at the [Version] section of the
    // actual inf where the file's layout info is contained.
    //
    // This handles all cases properly. For example, a file that is shipped by
    // a vendor that replaces one of our files. If the OEM's inf has a
    // SourceDisksFiles section with the file in it, it will be found first
    // when we look the file up using the given inf handle because of the way
    // inf append-loading works.
    //
    // If we cannot find the file in a [SourceDisksFiles] section (such as
    // if there is no such section), then we can't associate the file to be
    // copied with any INF/CAT.  If we do find a [SourceDisksFiles] entry, but
    // the containing INF doesn't specify a CatalogFile= entry, then we'll go
    // ahead and associate that with a SPQ_CATALOG_INFO node for that INF, but
    // that catalog info node will have a CatalogFileFromInf field of -1.
    // That's OK for system-provided INFs, but it will fail validation if it's
    // an OEM INF (this check is done later in _SetupVerifyQueuedCatalogs).
    //
    if(LayoutInfHandle || LayoutLineContext) {
        //
        // If we already have a valid layout line context, we don't need to go
        // looking for the file in [SourceDisksFiles] again (the caller is
        // assumed to have done that already). The caller might also have told
        // us that he *knows* that there is no [SourceDisksFiles] by passing us
        // a LayoutLineContext of -1.
        //
        if(LayoutLineContext == (PINFCONTEXT)(-1)) {
            //
            // For driver signing purposes, this may be an invalid file copy,
            // because it's being copied by an INF that contains no source
            // media information, nor does it use a layout file to supply such
            // information.
            //
            // Since we don't have a LayoutLineContext, we don't know exactly
            // which INF contained the CopyFile directive that initiated this
            // copy.  However, since the context is -1, that means that it was
            // INF based (i.e., as opposed to being manually queued up via
            // SetupQueueCopy).  Therefore, we scan all the INFs passed into
            // this routine (i.e., all the INFs in the pLoadedInfs lists), and
            // check to see if they're all located in %windir%\Inf.  If any of
            // them aren't, then we mark this copynode such that later it will
            // result in a signature verification failure of
            // ERROR_NO_CATALOG_FOR_OEM_INF.
            //
            for(i = 0; i < LoadedInfCount; i++) {

                for(pCurLoadedInf = pLoadedInfs[i]; pCurLoadedInf; pCurLoadedInf = pCurLoadedInf->Next) {

                    if(InfIsFromOemLocation(pCurLoadedInf->VersionBlock.Filename,
                                            TRUE)) {
                        //
                        // INF doesn't exist in %windir%\Inf--mark the copynode
                        // for codesigning verification failure.
                        //
                        QueueNode->InternalFlags |= IQF_FROM_BAD_OEM_INF;
                        break;
                    }

                    //
                    // Even if the INF does exist in %windir%\Inf, it might have
                    // originally been an OEM INF that was installed here--check
                    // its original filename to be sure...
                    //
                    if(pCurLoadedInf->OriginalInfName &&
                       InfIsFromOemLocation(pCurLoadedInf->OriginalInfName, TRUE)) {
                        //
                        // INF was an OEM INF--in this case, too, we need to
                        // mark the copynode for codesigning verification failure.
                        //
                        QueueNode->InternalFlags |= IQF_FROM_BAD_OEM_INF;
                        break;
                    }
                }

                if(QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF) {
                    //
                    // We found an OEM INF--no need to look any further.
                    //
                    break;
                }
            }

            LayoutLineContext = NULL;

        } else {
            if(!LayoutLineContext) {
                b = _SetupGetSourceFileLocation(
                        LayoutInfHandle,
                        NULL,
                        CopyParams->SourceFilename,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        &LineContext
                        );

                LayoutLineContext = b ? &LineContext : NULL;
            }
        }
    }

    //
    // At this point, a non-NULL LayoutLineContext indicates that we found an
    // INF to associate with the file to be copied (via a [SourceDisksFiles]
    // entry).
    //
    if(LayoutLineContext) {

        pSetupGetPhysicalInfFilepath(LayoutLineContext,
                                     TempBuffer,
                                     SIZECHARS(TempBuffer)
                                    );

        l2 = StringTableAddString(Queue->StringTable,
                                  TempBuffer,
                                  STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                 );
        //
        // This INF path should already be in the string table, and since we're
        // supplying a writeable buffer, there's no need for memory allocation.
        // Thus the addition of this string can't fail.
        //
        MYASSERT(l2 != -1);

        for(CatalogNode=Queue->CatalogList; CatalogNode; CatalogNode=CatalogNode->Next) {
            if(CatalogNode->InfFullPath == l2) {
                break;
            }
        }

        //
        // This node had better already be in the list!
        //
        MYASSERT(CatalogNode);

        QueueNode->CatalogInfo = CatalogNode;

    } else {
        //
        // There really is no catalog info.
        //
        QueueNode->CatalogInfo = NULL;
    }

    //
    // Unlock the INF(s) here, since the code below potentially returns without
    // hitting the final clean-up code at the bottom of the routine.
    //
    for(i = 0; i < LoadedInfCount; i++) {
        UnlockInf(pLoadedInfs[i]);
    }
    LoadedInfCount = 0;

    //
    // Set up the copy style flags
    //
    QueueNode->StyleFlags = CopyParams->CopyStyle;
    QueueNode->Next = NULL;

    //
    // Set up the source media.
    //
    try {
        Source = pSetupQueueSourceMedia(
                    Queue,
                    QueueNode,
                    QueueNode->SourceRootPath,
                    CopyParams->SourceDescription,
                    UseCache
                        ? CopyParams->CacheName
                        : CopyParams->SourceTagfile,
                    MediaFlags
                    );
        if(!Source) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    if(d != NO_ERROR) {
        goto clean2;
    }

    //
    // Link the node onto the end of the copy queue for this source media.
    //
    if(Source->CopyQueue) {
        //
        // Check to see if this same copy operation has already been enqueued
        // for this source media, and if so, get rid of the new one, to avoid
        // duplicates.  NOTE: We don't check the "InternalFlags" field, since
        // if the node already exists in the queue (based on all the other
        // fields comparing successfully), then any internal flags that were set
        // on the previously-existing node should be preserved.  (I.e., our new
        // node always is created with InternalFlags set to zero, except for
        // possibly IQF_FROM_BAD_OEM_INF, which we'll OR into the original
        // queue node's InternalFlags, if necessary.)
        //
        for(TempNode=Source->CopyQueue, PrevQueueNode = NULL;
            TempNode;
            PrevQueueNode = TempNode, TempNode=TempNode->Next) {

            if((TempNode->SourceRootPath == QueueNode->SourceRootPath) &&
               (TempNode->SourcePath == QueueNode->SourcePath) &&
               (TempNode->SourceFilename == QueueNode->SourceFilename) &&
               (TempNode->TargetDirectory == QueueNode->TargetDirectory) &&
               (TempNode->TargetFilename == QueueNode->TargetFilename) &&
               (TempNode->StyleFlags == QueueNode->StyleFlags) &&
               (TempNode->CatalogInfo == QueueNode->CatalogInfo)) {
                //
                // We have a duplicate.  OR in the IQF_FROM_BAD_OEM_INF flag
                // from our present queue node, if necessary, into the existing
                // queue node's InternalFlags.
                //
                if(QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF) {
                    TempNode->InternalFlags |= IQF_FROM_BAD_OEM_INF;
                }

                //
                // Now kill the newly-created queue node and return success.
                //
                MyFree(QueueNode);
                return TRUE;
            }
        }
        MYASSERT(PrevQueueNode);
        PrevQueueNode->Next = QueueNode;
    } else {
        Source->CopyQueue = QueueNode;
    }

    Queue->CopyNodeCount++;
    Source->CopyNodeCount++;

    return TRUE;

clean2:
    //
    // Truncate the catalog info node list at its original tail, and free all
    // subsequent (newly-added) nodes.
    //
    if(LastOldCatalogNode) {
        while(LastOldCatalogNode->Next) {
            CatalogNode = LastOldCatalogNode->Next;
            LastOldCatalogNode->Next = CatalogNode->Next;
            MyFree(CatalogNode);
        }
    }

clean1:
    MyFree(QueueNode);

clean0:
    for(i = 0; i < LoadedInfCount; i++) {
        UnlockInf(pLoadedInfs[i]);
    }

    SetLastError(d);
    return FALSE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueCopyIndirectA(
    IN PSP_FILE_COPY_PARAMS_A CopyParams
    )
{
    SP_FILE_COPY_PARAMS_WEX copyParams;
    DWORD rc;
    BOOL b;

    ZeroMemory(&copyParams,sizeof(SP_FILE_COPY_PARAMS_W));
    rc = NO_ERROR;
    b = FALSE;

    try {
        if(CopyParams->cbSize == sizeof(SP_FILE_COPY_PARAMS_W)) {
            copyParams.QueueHandle = CopyParams->QueueHandle;
            copyParams.CopyStyle = CopyParams->CopyStyle;
            copyParams.LayoutInf = CopyParams->LayoutInf;
            copyParams.SecurityDescriptor = NULL;
        } else {
            rc = ERROR_INVALID_PARAMETER;
        }
        if((rc == NO_ERROR) && CopyParams->SourceRootPath) {
            rc = CaptureAndConvertAnsiArg(CopyParams->SourceRootPath,&copyParams.SourceRootPath);
        }
        if((rc == NO_ERROR) && CopyParams->SourcePath) {
            rc = CaptureAndConvertAnsiArg(CopyParams->SourcePath,&copyParams.SourcePath);
        }
        if((rc == NO_ERROR) && CopyParams->SourceFilename) {
            rc = CaptureAndConvertAnsiArg(CopyParams->SourceFilename,&copyParams.SourceFilename);
        }
        if((rc == NO_ERROR) && CopyParams->SourceDescription) {
            rc = CaptureAndConvertAnsiArg(CopyParams->SourceDescription,&copyParams.SourceDescription);
        }
        if((rc == NO_ERROR) && CopyParams->SourceTagfile) {
            rc = CaptureAndConvertAnsiArg(CopyParams->SourceTagfile,&copyParams.SourceTagfile);
        }
        if((rc == NO_ERROR) && CopyParams->TargetDirectory) {
            rc = CaptureAndConvertAnsiArg(CopyParams->TargetDirectory,&copyParams.TargetDirectory);
        }
        if((rc == NO_ERROR) && CopyParams->TargetFilename) {
            rc = CaptureAndConvertAnsiArg(CopyParams->TargetFilename,&copyParams.TargetFilename);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // This is to catch the case where the CopyParams pointer goes bad.
        //
        rc = ERROR_INVALID_PARAMETER;
    }

    if(rc == NO_ERROR) {

        copyParams.cbSize = sizeof(SP_FILE_COPY_PARAMS_WEX);

        b = _SetupQueueCopy(&copyParams, NULL, NULL);
        rc = GetLastError();
    }

    if(copyParams.SourceRootPath) {
        MyFree(copyParams.SourceRootPath);
    }
    if(copyParams.SourcePath) {
        MyFree(copyParams.SourcePath);
    }
    if(copyParams.SourceFilename) {
        MyFree(copyParams.SourceFilename);
    }
    if(copyParams.SourceDescription) {
        MyFree(copyParams.SourceDescription);
    }
    if(copyParams.SourceTagfile) {
        MyFree(copyParams.SourceTagfile);
    }
    if(copyParams.TargetDirectory) {
        MyFree(copyParams.TargetDirectory);
    }
    if(copyParams.TargetFilename) {
        MyFree(copyParams.TargetFilename);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueCopyIndirectW(
    IN PSP_FILE_COPY_PARAMS_W CopyParams
    )
{
    UNREFERENCED_PARAMETER(CopyParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueCopyIndirect(
    IN PSP_FILE_COPY_PARAMS CopyParams
    )
{
    BOOL b;
    SP_FILE_COPY_PARAMSEX copyParamsEx = {0};

    //
    // All work is done by an internal subroutine.
    // The only thing we need to do here is validate the size
    // of the structure we've been given by the caller.
    //
    try {
        b = (CopyParams->cbSize == sizeof(SP_FILE_COPY_PARAMS));
        if (b) {
            CopyMemory(&copyParamsEx,CopyParams,sizeof(SP_FILE_COPY_PARAMS));
            copyParamsEx.cbSize = sizeof(SP_FILE_COPY_PARAMSEX);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }

    if(b) {
        b = _SetupQueueCopy(&copyParamsEx, NULL, NULL);
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueCopySectionA(
    IN HSPFILEQ QueueHandle,
    IN PCSTR    SourceRootPath,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,      OPTIONAL
    IN PCSTR    Section,
    IN DWORD    CopyStyle
    )
{
    PWSTR sourcerootpath;
    PWSTR section;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(SourceRootPath,&sourcerootpath);
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }
    rc = CaptureAndConvertAnsiArg(Section,&section);
    if(rc != NO_ERROR) {
        MyFree(sourcerootpath);
        SetLastError(rc);
        return(FALSE);
    }

    b = SetupQueueCopySectionW(
            QueueHandle,
            sourcerootpath,
            InfHandle,
            ListInfHandle,
            section,
            CopyStyle
            );

    rc = GetLastError();

    MyFree(sourcerootpath);
    MyFree(section);

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueCopySectionW(
    IN HSPFILEQ QueueHandle,
    IN PCWSTR   SourceRootPath,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,      OPTIONAL
    IN PCWSTR   Section,
    IN DWORD    CopyStyle
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ListInfHandle);
    UNREFERENCED_PARAMETER(Section);
    UNREFERENCED_PARAMETER(CopyStyle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueueCopySection(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR   SourceRootPath,
    IN HINF     InfHandle,
    IN HINF     ListInfHandle,   OPTIONAL
    IN PCTSTR   Section,
    IN DWORD    CopyStyle
    )

/*++

Routine Description:

    Queue an entire section in an inf file for copy. The section must be
    in copy-section format and the inf file must contain [SourceDisksFiles]
    and [SourceDisksNames] sections.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    SourceRootPath - supplies the root directory for the intended source.
        This should be a sharepoint or a device root such as a:\ or g:\.

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] and [SourceDisksNames] sections, and, if
        ListInfHandle is not specified, contains the section names by Section.
        This handle must be for a win95-style inf.

    ListInfHandle - if specified, supplies a handle to an open inf file
        containing the section to be queued for copy. Otherwise InfHandle
        is assumed to contain the section.

    Section - supplies the name of the section to be queued for copy.

    CopyStyle - supplies flags that control the behavior of the copy operation
        for this file.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information. Some of the files may have been queued.

--*/

{
    BOOL b;
    INFCONTEXT LineContext;
    PCTSTR SourceFilename;
    PCTSTR TargetFilename;
    PCTSTR SecurityDescriptor = NULL;
    PCTSTR CacheName = NULL;
    UINT Flags;
    DWORD CopyStyleLocal;
    LONG LineCount;
    HINF CabInf = INVALID_HANDLE_VALUE;

    //
    // Note that there are no potential faults here so no try/excepts
    // are necessary. pSetupQueueSingleCopy does all validation.
    //

    if(!ListInfHandle || (ListInfHandle == INVALID_HANDLE_VALUE)) {
        ListInfHandle = InfHandle;
    }

    //
    // Check for missing section
    //
    LineCount = SetupGetLineCount (ListInfHandle, Section);
    if(LineCount == -1) {
        try {
            if (QueueHandle != NULL
                && QueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE
                && ((PSP_FILE_QUEUE)QueueHandle)->Signature == SP_FILE_QUEUE_SIG) {

                WriteLogEntry(
                    ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_NOSECTION,
                    NULL,
                    Section);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
        }
        SetLastError(ERROR_SECTION_NOT_FOUND);
        return(FALSE);
    }

    //
    // if section is empty, do nothing.
    //
    if(LineCount == 0) {
        return(TRUE);
    }

    //
    // The section has to exist and there has to be at least one line in it.
    //
    b = SetupFindFirstLine(ListInfHandle,Section,NULL,&LineContext);
    if(!b) {
        try {
            if (QueueHandle != NULL
                && QueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE
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
    //Get the Security descriptor
    //

    if( !pSetupGetSecurityInfo( ListInfHandle, Section, &SecurityDescriptor ) )
        SecurityDescriptor = NULL;


    //
    // load driver cache inf
    // BugBug will this be a perf problem?
    //
    CabInf = SetupOpenInfFile( STR_DRIVERCACHEINF , NULL, INF_STYLE_WIN4, NULL );
    if (CabInf != INVALID_HANDLE_VALUE) {
        CopyStyle |= PSP_COPY_USE_DRIVERCACHE;
    }

    //
    // Iterate every line in the section.
    //
    do {
        CopyStyleLocal = CopyStyle;
        //
        // Get the target filename out of the line.
        // Field 1 is the target so there must be one for the line to be valid.
        //
        TargetFilename = pSetupFilenameFromLine(&LineContext,FALSE);
        if(!TargetFilename) {
            if (CabInf != INVALID_HANDLE_VALUE) {
                SetupCloseInfFile(CabInf);
            }
            try {
                if (QueueHandle != NULL
                    && QueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE
                    && ((PSP_FILE_QUEUE)QueueHandle)->Signature == SP_FILE_QUEUE_SIG) {

                    WriteLogEntry(
                        ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                        SETUP_LOG_ERROR,
                        MSG_LOG_COPY_TARGET,
                        NULL,
                        Section);
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
            }
            SetLastError(ERROR_INVALID_DATA);
            return(FALSE);
        }

        //
        // Get source filename out of the line. If there is none, use
        // the target name as the source name.
        //
        SourceFilename = pSetupFilenameFromLine(&LineContext,TRUE);
        if(!SourceFilename || (*SourceFilename == 0)) {
            SourceFilename = TargetFilename;
        }

        //
        // if we were asked to use the driver cache, then check if the file
        // is in the associated INF for the cab.
        //
        if (CabInf != INVALID_HANDLE_VALUE) {
            if (!pIsFileInDriverCache(CabInf, SourceFilename, NULL, &CacheName)) {
                CopyStyleLocal &= ~PSP_COPY_USE_DRIVERCACHE;
            }
        }

        //
        // If present, flags are field 3.
        //
        if(SetupGetIntField(&LineContext,4,(PINT)&Flags)) {

            if(Flags & COPYFLG_WARN_IF_SKIP) {
                CopyStyleLocal |= SP_COPY_WARNIFSKIP;
            }

            if(Flags & COPYFLG_NOSKIP) {
                CopyStyleLocal |= SP_COPY_NOSKIP;
            }

            if(Flags & COPYFLG_NOVERSIONCHECK) {
                CopyStyleLocal &= ~SP_COPY_NEWER;
            }

            if(Flags & COPYFLG_FORCE_FILE_IN_USE) {
                CopyStyleLocal |= SP_COPY_FORCE_IN_USE;
                CopyStyleLocal |= SP_COPY_IN_USE_NEEDS_REBOOT;
            }

            if(Flags & COPYFLG_NO_OVERWRITE) {
                CopyStyleLocal |= SP_COPY_FORCE_NOOVERWRITE;
            }

            if(Flags & COPYFLG_NO_VERSION_DIALOG) {
                CopyStyleLocal |= SP_COPY_FORCE_NEWER;
            }

            if(Flags & COPYFLG_OVERWRITE_OLDER_ONLY) {
                CopyStyleLocal |= SP_COPY_NEWER_ONLY;
            }

            if(Flags & COPYFLG_REPLACEONLY) {
                CopyStyleLocal |= SP_COPY_REPLACEONLY;
            }

            if(Flags & COPYFLG_NODECOMP) {
                CopyStyleLocal |= SP_COPY_NODECOMP;
            }

            if(Flags & COPYFLG_REPLACE_BOOT_FILE) {
                CopyStyleLocal |= SP_COPY_REPLACE_BOOT_FILE;
            }

            if(Flags & COPYFLG_NOPRUNE) {
                CopyStyleLocal |= SP_COPY_NOPRUNE;
            }

        }

        b = pSetupQueueSingleCopy(
                QueueHandle,
                InfHandle,
                ListInfHandle,
                Section,
                SourceRootPath,
                SourceFilename,
                TargetFilename,
                CopyStyleLocal,
                SecurityDescriptor,
                CacheName
                );

        if (CacheName) {
            MyFree( CacheName );
            CacheName = NULL;
        }

        if(!b) {
            DWORD LastError = GetLastError();

            if (CabInf != INVALID_HANDLE_VALUE) {
                SetupCloseInfFile(CabInf);
            }

            SetLastError( LastError );

            return(FALSE);
        }
    } while(SetupFindNextLine(&LineContext,&LineContext));

    if (CabInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(CabInf);
    }
    return(TRUE);
}


BOOL
pSetupQueueSingleCopy(
    IN HSPFILEQ    QueueHandle,
    IN HINF        InfHandle,
    IN HINF        ListInfHandle,  OPTIONAL
    IN PCTSTR      SectionName,    OPTIONAL
    IN PCTSTR      SourceRootPath,
    IN PCTSTR      SourceFilename,
    IN PCTSTR      TargetFilename,
    IN DWORD       CopyStyle,
    IN PCTSTR      SecurityDescriptor,
    IN PCTSTR      CacheName
    )

/*++

Routine Description:

    Add a single file to the copy queue, using the default source media
    and destination as specified in an inf file.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] and [SourceDisksNames] sections.
        This handle must be for a win95-style inf.

    ListInfHandle - if specified, supplies handle to the inf in which
        the file being copied appears (such as in a file copy list section).
        If not specified, this is assumed to be the same inf as InfHandle.

    SourceRootPath - supplies the root directory for the intended source.
        This should be a sharepoint or a device root such as a:\ or g:\.

    SourceFilename - supplies the filename of the source file. Filename part
        only.

    TargetFilename - supplies the filename of the target file. Filename part
        only.

    CopyStyle - supplies flags that control the behavior of the copy operation
        for this file.

    SecurityDescriptor - describes the permissions for the target file

    CacheName - if supplied this is the name of the driver cache we should
                use to copy the file out of instead of the specified source path

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    BOOL b;
    UINT SourceId;
    DWORD SizeRequired;
    PTSTR TargetDirectory;
    PCTSTR SourceDescription,SourceTagfile,SourceRelativePath;
    PCTSTR TmpCacheName = CacheName;
    UINT SourceFlags;
    DWORD rc;
    TCHAR FileSubdir[MAX_PATH];
    TCHAR RelativePath[MAX_PATH];
    INFCONTEXT LineContext;
    PINFCONTEXT pLineContext;
    SP_FILE_COPY_PARAMSEX CopyParams;
    HINF CabInf = INVALID_HANDLE_VALUE;

    if(!ListInfHandle || (ListInfHandle == INVALID_HANDLE_VALUE)) {
        ListInfHandle = InfHandle;
    }

    //
    // Determine the source disk id and subdir where the file is located.
    //
    b = _SetupGetSourceFileLocation(
            InfHandle,
            NULL,
            SourceFilename,
            &SourceId,
            FileSubdir,
            MAX_PATH,
            &rc,
            &LineContext
            );

    if(!b) {
        //
        // Assume file subdir is too big and is invalid.
        // Try to fetch just the id and assume there is no subdir.
        //
        b = _SetupGetSourceFileLocation(
                InfHandle,
                NULL,
                SourceFilename,
                &SourceId,
                NULL,
                0,
                NULL,
                &LineContext
                );

        if(b) {
            FileSubdir[0] = 0;
        }
    }

    if(b) {
        //
        // Get information about the source. Need the tag file,
        // description, and relative source path.
        //
        b = pSetupGetSourceAllInfo(
                InfHandle,
                &LineContext,
                SourceId,
                &SourceDescription,
                &SourceTagfile,
                &SourceRelativePath,
                &SourceFlags
                );

        if(!b) {
            //
            // Last error already set.
            //
            rc = GetLastError();
            goto clean1;
        }

        //
        // Set a value that causes _SetupQueueCopy to skip looking for the
        // [SourceDisksFiles] section -- we just found it, so we just pass
        // the info along!
        //
        pLineContext = &LineContext;

    } else {
        //
        // Assume there is no SourceDisksFiles section and fake it as best we can.
        // Assume the media has a description of "Unknown," set the source path to
        // the source root if there is one, and assume no tag file.
        //
        // We also set a special value that tells _SetupQueueCopy not to bother trying
        // to look for the [SourceDisksFiles] section itself, since there isn't one.
        //
        FileSubdir[0] = 0;
        SourceDescription = NULL;
        SourceTagfile = NULL;
        SourceRelativePath = NULL;
        pLineContext = (PINFCONTEXT)(-1);
    }

    if ( CopyStyle & PSP_COPY_CHK_DRIVERCACHE) {
        CabInf = SetupOpenInfFile( STR_DRIVERCACHEINF, NULL, INF_STYLE_WIN4, NULL );
        if (CabInf != INVALID_HANDLE_VALUE) {
            if (pIsFileInDriverCache(CabInf, SourceFilename, SourceRelativePath, &TmpCacheName)) {
                CopyStyle |= PSP_COPY_USE_DRIVERCACHE;
                CopyStyle &= ~PSP_COPY_CHK_DRIVERCACHE;
            }

            SetupCloseInfFile(CabInf);

        }
    }

    if (CopyStyle & PSP_COPY_USE_DRIVERCACHE) {
        //
        // check if the inf we want to copy from is an OEM inf
        //
        if (!pLineContext || pLineContext==(PINFCONTEXT)-1) {
            CopyStyle &= ~PSP_COPY_USE_DRIVERCACHE;
        } else if (InfIsFromOemLocation( ((PLOADED_INF)pLineContext->CurrentInf)->VersionBlock.Filename,TRUE )) {
            CopyStyle &= ~PSP_COPY_USE_DRIVERCACHE;
        } else if ( ((PLOADED_INF)pLineContext->CurrentInf)->OriginalInfName
                    && InfIsFromOemLocation( ((PLOADED_INF)pLineContext->CurrentInf)->OriginalInfName, TRUE) ) {
            CopyStyle &= ~PSP_COPY_USE_DRIVERCACHE;
        }
    }

    //
    // Determine the target path for the file.
    //
    if(b = SetupGetTargetPath(ListInfHandle,NULL,SectionName,NULL,0,&SizeRequired)) {

        if(TargetDirectory = MyMalloc(SizeRequired*sizeof(TCHAR))) {

            if(b = SetupGetTargetPath(ListInfHandle,NULL,SectionName,TargetDirectory,SizeRequired,NULL)) {

                //
                // Append the source relative path and the file subdir.
                //
                if(SourceRelativePath) {
                    lstrcpyn(RelativePath,SourceRelativePath,MAX_PATH);
                    if(FileSubdir[0]) {
                        ConcatenatePaths(RelativePath,FileSubdir,MAX_PATH,NULL);
                    }
                } else {
                    RelativePath[0] = 0;
                }

                //
                // Add to queue.
                //
                CopyParams.cbSize            = sizeof(SP_FILE_COPY_PARAMSEX);
                CopyParams.QueueHandle       = QueueHandle;
                CopyParams.SourceRootPath    = SourceRootPath;
                CopyParams.SourcePath        = RelativePath[0] ? RelativePath : NULL ;
                CopyParams.SourceFilename    = SourceFilename;
                CopyParams.SourceDescription = SourceDescription;
                CopyParams.SourceTagfile     = SourceTagfile;
                CopyParams.TargetDirectory   = TargetDirectory;
                CopyParams.TargetFilename    = TargetFilename;
                CopyParams.CopyStyle         = CopyStyle;
                CopyParams.LayoutInf         = InfHandle;
                CopyParams.SecurityDescriptor= SecurityDescriptor;
                CopyParams.CacheName         = TmpCacheName;
                //
                // first item indicates source flag information
                // second item indicates that we've already retrieved
                // this information, so even if the SourceFlags are zero,
                // we won't go looking for it again
                //
                CopyParams.SourceFlags       = SourceFlags;
                CopyParams.SourceFlagsSet    = TRUE;

                b = _SetupQueueCopy(&CopyParams,
                                    pLineContext,
                                    ((InfHandle == ListInfHandle) ? NULL : ListInfHandle)
                                   );

                rc = GetLastError();
            }

            MyFree(TargetDirectory);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        rc = GetLastError();
    }

    if(SourceDescription) {
        MyFree(SourceDescription);
    }
    if(SourceTagfile) {
        MyFree(SourceTagfile);
    }
    if(SourceRelativePath) {
        MyFree(SourceRelativePath);
    }
    if(TmpCacheName && TmpCacheName != CacheName) {
        MyFree(TmpCacheName);
    }

clean1:
    if(!b) {

        try {
            if (QueueHandle != NULL
                && QueueHandle != INVALID_HANDLE_VALUE
                && ((PSP_FILE_QUEUE)QueueHandle)->Signature == SP_FILE_QUEUE_SIG) {

                WriteLogEntry(
                    ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_COPY_QUEUE_ERROR,
                    NULL,
                    SectionName ? SectionName : TEXT("-"),
                    TargetFilename,
                    SourceFilename);
                WriteLogError(
                    ((PSP_FILE_QUEUE)QueueHandle)->LogContext,
                    SETUP_LOG_ERROR,
                    rc
                    );
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
        }

        SetLastError(rc);
    }

    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueueDefaultCopyA(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN PCSTR    SourceRootPath,
    IN PCSTR    SourceFilename,
    IN PCSTR    TargetFilename,
    IN DWORD    CopyStyle
    )
{
    PWSTR sourcerootpath;
    PWSTR sourcefilename;
    PWSTR targetfilename;
    DWORD rc;
    BOOL b;

    b = FALSE;
    rc = CaptureAndConvertAnsiArg(SourceRootPath,&sourcerootpath);
    if(rc == NO_ERROR) {

        rc = CaptureAndConvertAnsiArg(SourceFilename,&sourcefilename);
        if(rc == NO_ERROR) {

            rc = CaptureAndConvertAnsiArg(TargetFilename,&targetfilename);
            if(rc == NO_ERROR) {

                b = SetupQueueDefaultCopyW(
                        QueueHandle,
                        InfHandle,
                        sourcerootpath,
                        sourcefilename,
                        targetfilename,
                        CopyStyle
                        );

                rc = GetLastError();

                MyFree(targetfilename);
            }

            MyFree(sourcefilename);
        }

        MyFree(sourcerootpath);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueueDefaultCopyW(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN PCWSTR   SourceRootPath,
    IN PCWSTR   SourceFilename,
    IN PCWSTR   TargetFilename,
    IN DWORD    CopyStyle
    )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(SourceFilename);
    UNREFERENCED_PARAMETER(TargetFilename);
    UNREFERENCED_PARAMETER(CopyStyle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
SetupQueueDefaultCopy(
    IN HSPFILEQ QueueHandle,
    IN HINF     InfHandle,
    IN PCTSTR   SourceRootPath,
    IN PCTSTR   SourceFilename,
    IN PCTSTR   TargetFilename,
    IN DWORD    CopyStyle
    )

/*++

Routine Description:

    Add a single file to the copy queue, using the default source media
    and destination as specified in an inf file.

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    InfHandle - supplies a handle to an open inf file, that contains the
        [SourceDisksFiles] and [SourceDisksNames] sections.
        This handle must be for a win95-style inf.

    SourceRootPath - supplies the root directory for the intended source.
        This should be a sharepoint or a device root such as a:\ or g:\.

    SourceFilename - supplies the filename of the source file. Filename part
        only.

    TargetFilename - supplies the filename of the target file. Filename part
        only.

    CopyStyle - supplies flags that control the behavior of the copy operation
        for this file.

Return Value:

    Boolean value indicating outcome. If FALSE, GetLastError() returns
    extended error information.

--*/

{
    BOOL b;

    b = pSetupQueueSingleCopy(
            QueueHandle,
            InfHandle,
            NULL,
            NULL,
            SourceRootPath,
            SourceFilename,
            TargetFilename,
            CopyStyle | PSP_COPY_CHK_DRIVERCACHE,
            NULL,
            NULL
            );

    return(b);
}


PSOURCE_MEDIA_INFO
pSetupQueueSourceMedia(
    IN OUT PSP_FILE_QUEUE      Queue,
    IN OUT PSP_FILE_QUEUE_NODE QueueNode,
    IN     LONG                SourceRootStringId,
    IN     PCTSTR              SourceDescription,   OPTIONAL
    IN     PCTSTR              SourceTagfile,       OPTIONAL
    IN     DWORD               MediaFlags
    )

/*++

Routine Description:

    Set up a file queue node's source media descriptor pointer, creating a new
    source media descriptor if necessary.

Arguments:

    Queue - supplies pointer to file queue with which the queue node
        is associated.

    QueueNode - supplies file queue node whose source media descriptor pointer
        is to be set.

    SourceRootStringId - supplies string id of root to source (something like a:\).

    SourceDescription - if specified, supplies a description for the media.

    SourceTagfile - if specified, supplies a tag file for the media.
        Ignored if SourceDescription is not specified.

    MediaFlags - specifies additional information used in searching for an
        existing source media descriptor in the specified queue, and in adding
        new source media descriptors to that queue.  May be a combination of
        the following values:

        SMI_FLAG_NO_SOURCE_ROOT_PATH : The caller didn't supply a SourceRootPath
            for this copy action, so we're using a default path.  This flag
            causes us to not include the SourceRootStringId as part of our
            match criteria when searching to see if the specified source media
            information is already present in an existing media descriptor.  If
            we don't find a match (i.e., we have to create a new descriptor),
            we'll store this flag away in the SOURCE_MEDIA_INFO.Flags field so
            that if we come along later to add source media descriptors where
            the caller did specify SourceRootPath, then we'll re-use this
            descriptor and overwrite the existing (default) source root path
            with the caller-specified one.

        SMI_FLAG_USE_SVCPACK_SOURCE_ROOT_PATH : The caller didn't supply a SourceRootPath
            for this copy action, and it's a tagged source media, so we're using a
            service pack path.  This flag  causes us to not include the SourceRootStringId
            as part of our match criteria when searching to see if the specified source media
            information is already present in an existing media descriptor.  If
            we don't find a match (i.e., we have to create a new descriptor),
            we'll store this flag away in the SOURCE_MEDIA_INFO.Flags field so
            that if we come along later to add source media descriptors where
            the caller did specify SourceRootPath, then we'll re-use this
            descriptor and overwrite the existing (default) source root path
            with the caller-specified one.

        SMI_FLAG_USE_LOCAL_SOURCE_CAB : The caller wants to use the local source cab containing
            driver files, etc.  In this case, we supply the source description and tagfile,
            ignoring what the caller passes in.  At this point we know the media is present, as
            the caller provided this check.  If it wasnt't, we default to the OS Source path location.


Return Value:

    Pointer to source media info structure, or NULL if out of memory.

--*/

{
    LONG DescriptionStringId;
    LONG TagfileStringId;
    PSOURCE_MEDIA_INFO Source,LastSource, TempSource;
    BOOL b1,b2;
    TCHAR TempTagfileString[MAX_PATH];
    TCHAR TempSrcDescString[LINE_LEN];


    if (MediaFlags & SMI_FLAG_USE_LOCAL_SOURCE_CAB) {
        LoadString( MyDllModuleHandle, IDS_DRIVERCACHE_DESC, TempSrcDescString, sizeof(TempSrcDescString)/sizeof(TCHAR) );
        SourceDescription = TempSrcDescString;
    } else {
        //
        // For the optional SourceDescription and SourceTagfile parameters, treat
        // empty strings as if the parameter had not been specified.
        //
        if(SourceDescription && !(*SourceDescription)) {
            SourceDescription = NULL;
        }
        if(SourceTagfile && !(*SourceTagfile)) {
            SourceTagfile = NULL;
        }

        //
        // If no description is specified, force the tagfile to none.
        //
        if(!SourceDescription) {
            SourceTagfile = NULL;
        }
    }

    if(SourceDescription) {
        //
        // Description specified. See if it's in the table. If not,
        // no need to search the list of media descriptors because we know
        // we can't find a match.
        //
        // (We must first copy this string to a writeable buffer, to speed up the
        // case-insensitive lookup.
        //
        lstrcpyn(TempSrcDescString, SourceDescription, SIZECHARS(TempSrcDescString));
        DescriptionStringId = StringTableLookUpString(Queue->StringTable,
                                                      TempSrcDescString,
                                                      STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                     );
        b1 = (DescriptionStringId != -1);
    } else {
        //
        // No description specified, look for a source media with -1 as the
        // description string id
        //
        DescriptionStringId = -1;
        b1 = TRUE;
    }

    if(SourceTagfile) {
        //
        // Tagfile specified. See if it's in the table. If not,
        // no need to search the list of media descriptors because we know
        // we can't find a match.
        //
        // (Again, we must first copy the string to a writeable buffer.
        //
        lstrcpyn(TempTagfileString, SourceTagfile, SIZECHARS(TempTagfileString));
        TagfileStringId = StringTableLookUpString(Queue->StringTable,
                                                  TempTagfileString,
                                                  STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                 );
        b2 = (TagfileStringId != -1);
    } else {
        //
        // No tagfile specified, look for a source media with -1 as the
        // tagfile string id
        //
        TagfileStringId = -1;
        b2 = TRUE;
    }

    //
    // If we think there's a possibility of finding an existing source that
    // matches the caller's parameters, scan the source media list looking
    // for a match.
    //
    if(b1 && b2) {

        for(Source=Queue->SourceMediaList; Source; Source=Source->Next) {

            if((Source->Description == DescriptionStringId) && (Source->Tagfile == TagfileStringId)) {
                //
                // We only consider the SourceRootPath when both existing
                // media descriptor and new media descriptor have actual
                // caller-supplied paths (as opposed to something we made up).
                //
                if((Source->Flags & SMI_FLAG_NO_SOURCE_ROOT_PATH) ||
                   (MediaFlags & SMI_FLAG_NO_SOURCE_ROOT_PATH) ||
                   (Source->SourceRootPath == SourceRootStringId)) {
                    //
                    // Got a match. Point the queue node at this source and return.
                    //
                    QueueNode->SourceMediaInfo = Source;
                    //
                    // If the existing media descriptor had a made-up source
                    // root path, but the new media information had an actual
                    // caller-supplied one, then replace the made-up one with
                    // the real one and clear the no-source-root-path flag.
                    //
                    if((Source->Flags & SMI_FLAG_NO_SOURCE_ROOT_PATH) &&
                       !(MediaFlags & SMI_FLAG_NO_SOURCE_ROOT_PATH)) {

                        Source->SourceRootPath = SourceRootStringId;
                        Source->Flags &= ~SMI_FLAG_NO_SOURCE_ROOT_PATH;
                    }

                    return(Source);
                }
            }
        }
    }

    //
    // Need to add a new source media descriptor.
    // Allocate the structure and fill it in.
    //
    Source = MyMalloc(sizeof(SOURCE_MEDIA_INFO));
    if(!Source) {
        return(NULL);
    }

    Source->Next = NULL;
    Source->CopyQueue = NULL;
    Source->CopyNodeCount = 0;
    Source->Flags = MediaFlags;

    if(SourceDescription) {
        //
        // Since we already passed this in for a case-insensitive lookup with a writeable
        // buffer, we can add it case-sensitively, because it's already lower-cased.
        //
        Source->Description = StringTableAddString(Queue->StringTable,
                                                   TempSrcDescString,
                                                   STRTAB_CASE_SENSITIVE | STRTAB_ALREADY_LOWERCASE
                                                  );
        //
        // We also must add the description in its original case, since this is a displayable string.
        // (We're safe in casting away the CONST-ness of this string, since it won't be modified.)
        //
        Source->DescriptionDisplayName = StringTableAddString(Queue->StringTable,
                                                              (PTSTR)SourceDescription,
                                                              STRTAB_CASE_SENSITIVE
                                                             );

        if((Source->Description == -1) || (Source->DescriptionDisplayName == -1)) {
            MyFree(Source);
            return(NULL);
        }
    } else {
        Source->Description = Source->DescriptionDisplayName = -1;
    }

    if(SourceTagfile) {
        //
        // Again, we already lower-cased this in a writeable buffer above.
        //
        Source->Tagfile = StringTableAddString(Queue->StringTable,
                                               TempTagfileString,
                                               STRTAB_CASE_SENSITIVE | STRTAB_ALREADY_LOWERCASE
                                              );
        if(Source->Tagfile == -1) {
            MyFree(Source);
            return(NULL);
        }
    } else {
        Source->Tagfile = -1;
    }

    Source->SourceRootPath = SourceRootStringId;

    //
    // insert our media descriptor into the list of descriptors
    // Note: if the new descriptor has the "service pack" or
    // "local cab driver cache" tag set, then we insert it into
    // the head of the list, otherwise we put it into the end
    // of the list.  This ensures that if the user get's a
    // need media complaint for os binaries, and overrides
    // the source path, then the user will first be prompted for service
    // pack media, then the os media. This saves us from adding lots of
    // code to handle need media overrides in this case, since we would
    // potentially have the os source files first in the media list, which
    // would cause us to install the os media files instead of the service
    // pack media files
    //
    if(Queue->SourceMediaList) {
        if (Source->Flags & (SMI_FLAG_USE_SVCPACK_SOURCE_ROOT_PATH |
                             SMI_FLAG_USE_LOCAL_SOURCE_CAB) ) {
            TempSource = Queue->SourceMediaList;
            Queue->SourceMediaList = Source;
            Source->Next = TempSource;
        } else {
            for(LastSource=Queue->SourceMediaList; LastSource->Next; LastSource=LastSource->Next) ;
            LastSource->Next = Source;
        }
    } else {
        Queue->SourceMediaList = Source;
    }

    Queue->SourceMediaCount++;

    QueueNode->SourceMediaInfo = Source;
    return(Source);
}


BOOL
pSetupGetSourceAllInfo(
    IN  HINF         InfHandle,
    IN  PINFCONTEXT  LayoutLineContext, OPTIONAL
    IN  UINT         SourceId,
    OUT PCTSTR      *Description,
    OUT PCTSTR      *Tagfile,
    OUT PCTSTR      *RelativePath,
    OUT PUINT        SourceFlags
    )
{
    BOOL b;
    DWORD RequiredSize;
    PTSTR p;
    DWORD ec;
    TCHAR Buffer[MAX_PATH];

    //
    // Get path relative to the source.
    //
    b = pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_PATH,NULL,0,&RequiredSize);
    if(!b) {
        ec = GetLastError();
        goto clean0;
    }

    p = MyMalloc(RequiredSize*sizeof(TCHAR));
    if(!p) {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }
    pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_PATH,p,RequiredSize,NULL);
    *RelativePath = p;

    //
    // Get description.
    //
    b = pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_DESCRIPTION,NULL,0,&RequiredSize);
    if(!b) {
        ec = GetLastError();
        goto clean1;
    }

    p = MyMalloc(RequiredSize*sizeof(TCHAR));
    if(!p) {
        ec =  ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }
    pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_DESCRIPTION,p,RequiredSize,NULL);
    *Description = p;

    //
    // Get tagfile, if any.
    //
    b = pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_TAGFILE,NULL,0,&RequiredSize);
    if(!b) {
        ec = GetLastError();
        goto clean2;
    }

    p = MyMalloc(RequiredSize*sizeof(TCHAR));
    if(!p) {
        ec =  ERROR_NOT_ENOUGH_MEMORY;
        goto clean2;
    }
    pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_TAGFILE,p,RequiredSize,NULL);
    if(*p) {
        *Tagfile = p;
    } else {
        MyFree(p);
        *Tagfile = NULL;
    }

    //
    // Get flags, if any.
    //
    b = pSetupGetSourceInfo(InfHandle,LayoutLineContext,SourceId,SRCINFO_FLAGS,Buffer,sizeof(Buffer),NULL);
    if(!b) {
        ec = GetLastError();
        goto clean3;
    }

    pAToI( Buffer, SourceFlags );


    return(TRUE);

clean3:
    MyFree(*Tagfile);
clean2:
    MyFree(*Description);
clean1:
    MyFree(*RelativePath);
clean0:
    SetLastError(ec);
    return(FALSE);
}


BOOL
pIsFileInDriverCache(
    IN  HINF   CabInf,
    IN  PCTSTR TargetFilename,
    IN  PCTSTR SubDirectory,
    OUT PCTSTR *CacheName
    )
{
    INFCONTEXT Context,SectionContext;
    PCTSTR      SectionName,CabName;
    TCHAR      TempBuffer[MAX_PATH];

    UINT Field, FieldCount;

    MYASSERT(CabInf != INVALID_HANDLE_VALUE);
    MYASSERT(TargetFilename);
    MYASSERT(CacheName);

    if (!SetupFindFirstLine(CabInf, TEXT("Version"), TEXT("CabFiles"), &SectionContext)) {
        return(FALSE);
    }

    do  {


        FieldCount = SetupGetFieldCount(&SectionContext);
        for(Field=1; Field<=FieldCount; Field++) {

            SectionName = pSetupGetField(&SectionContext,Field);

            if (SetupFindFirstLine(CabInf,SectionName,TargetFilename,&Context)) {
                //
                // we found a match
                //
                if (SetupFindFirstLine(CabInf,TEXT("Cabs"),SectionName,&Context)) {
                    CabName= pSetupGetField(&Context,1);
                    //if (pIsDriverCachePresent(CabName,SubDirectory,TempBuffer)) {
                        *CacheName = DuplicateString( CabName );
                        if (*CacheName) {
                            return(TRUE);
                        }
                    //}
                }
            }
        } // end for

    } while (SetupFindNextMatchLine(&SectionContext,TEXT("CabFiles"),&SectionContext));

    return(FALSE);

}

BOOL
pIsDriverCachePresent(
    IN     PCTSTR DriverName,
    IN     PCTSTR Subdirectory,
    IN OUT PTSTR DriverBuffer
    )
/*++

Routine Description:

    Looks at the proper location for the driver cache cab-file, and if it's
    present, return TRUE.  If present, it returns the partial path to the
    cab file

Arguments:

    DriveName    - the cab file we're looking for

    Subdirectory - if specified, use this as the subdirectory from the root of the driver
                   cache, otherwise use the specified architecture's subdirectory

    DriverBuffer - if the cab file is present, return the source root to the cab file


Return Value:

    TRUE if the cab file is present

--*/

{

    TCHAR TempBuffer[MAX_PATH];

    if (!DriverCacheSourcePath || !DriverName) {
        return FALSE;
    }

    if (!Subdirectory) {
        Subdirectory =
#if   defined(_AXP64_)
         TEXT("axp64");
#elif defined(_ALPHA_)
         TEXT("alpha");
#elif defined(_X86_)
         IsNEC98() ? TEXT("nec98") : TEXT("i386");
#elif defined(_IA64_)
         TEXT("ia64");
#endif
    }

    lstrcpy(TempBuffer, DriverCacheSourcePath);
    ConcatenatePaths(TempBuffer, Subdirectory , MAX_PATH, NULL);
    ConcatenatePaths(TempBuffer, DriverName, MAX_PATH, NULL);

    if (FileExists(TempBuffer,NULL)) {
        lstrcpy(DriverBuffer,DriverCacheSourcePath);
        return(TRUE);
    }

    return(FALSE);

}
