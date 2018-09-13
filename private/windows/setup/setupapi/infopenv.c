/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    infopenv.c

Abstract:

    Externally exposed INF routines for INF opening, closing,
    and versioning.

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


BOOL
pSetupVersionNodeFromInfInformation(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    OUT PINF_VERSION_NODE   VersionNode,
    OUT PTSTR               OriginalFilename OPTIONAL
    );


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetInfInformationA(
    IN  LPCVOID             InfSpec,
    IN  DWORD               SearchControl,
    OUT PSP_INF_INFORMATION ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    PCWSTR infspec;
    BOOL b;
    DWORD rc;

    //
    // For this API, the return buffer does not have to be translated
    // from Unicode to ANSI. This makes things much easier since the
    // required size is the same for the ANSI and Unicode versions.
    //
    if((SearchControl == INFINFO_INF_NAME_IS_ABSOLUTE)
    || (SearchControl == INFINFO_DEFAULT_SEARCH)
    || (SearchControl == INFINFO_REVERSE_DEFAULT_SEARCH)
    || (SearchControl == INFINFO_INF_PATH_LIST_SEARCH)) {

        rc = CaptureAndConvertAnsiArg(InfSpec,&infspec);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }

    } else {
        //
        // Not a pointer to a string, just pass it on.
        //
        infspec = InfSpec;
    }

    //
    // Note that the data returned from this API is in an
    // internal format, and thus we don't need any less space
    // for the ANSI API, and can just use the buffer and sizes
    // passed in by the caller.
    //
    b = SetupGetInfInformationW(
            infspec,
            SearchControl,
            ReturnBuffer,
            ReturnBufferSize,
            RequiredSize
            );

    rc = GetLastError();

    if(infspec != InfSpec) {
        MyFree(infspec);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupGetInfInformationW(
    IN  LPCVOID             InfSpec,
    IN  DWORD               SearchControl,
    OUT PSP_INF_INFORMATION ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfSpec);
    UNREFERENCED_PARAMETER(SearchControl);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetInfInformation(
    IN  LPCVOID             InfSpec,
    IN  DWORD               SearchControl,
    OUT PSP_INF_INFORMATION ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    BOOL UnloadInf;
    PLOADED_INF Inf, CurInf;
    UINT InfCount;
    PUCHAR Out;
    DWORD TotalSpaceRequired;
    DWORD d;
    DWORD ErrorLineNumber;
    TCHAR Path[MAX_PATH];
    PINF_VERSION_NODE VersionNode;
    INF_VERSION_BLOCK UNALIGNED *Prev;
    BOOL TryPnf;
    WIN32_FIND_DATA FindData;
    PTSTR DontCare;
    UINT OriginalFilenameSize;

    //
    // Set up some state based on the SearchSpec parameter.
    //
    Inf = NULL;
    switch(SearchControl) {

    case INFINFO_INF_SPEC_IS_HINF:

        Inf = (PLOADED_INF)InfSpec;
        break;

    case INFINFO_INF_NAME_IS_ABSOLUTE:
        //
        // Make sure we have a fully-qualified path.
        //
        d = GetFullPathName((PCTSTR)InfSpec,
                            SIZECHARS(Path),
                            Path,
                            &DontCare
                           );
        if(!d) {
            //
            // LastError has already been set.
            //
            return FALSE;
        } else if(d >= SIZECHARS(Path)) {
            MYASSERT(0);
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }

        if(FileExists(Path, &FindData)) {
            InfSourcePathFromFileName(Path, NULL, &TryPnf);
            break;
        } else {
            //
            // LastError has already been set.
            //
            return FALSE;
        }

    case INFINFO_DEFAULT_SEARCH:
    case INFINFO_REVERSE_DEFAULT_SEARCH:
    case INFINFO_INF_PATH_LIST_SEARCH:

        d = SearchForInfFile((PCTSTR)InfSpec,
                             &FindData,
                             SearchControl,
                             Path,
                             SIZECHARS(Path),
                             NULL
                            );

        if(d == NO_ERROR) {
            TryPnf = TRUE;
            break;
        } else {
            SetLastError(d);
            return FALSE;
        }

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Load the inf if necessary.
    //
    if(Inf) {
        UnloadInf = FALSE;
    } else {

        d = LoadInfFile(Path,
                        &FindData,
                        INF_STYLE_ALL,
                        TryPnf ? LDINF_FLAG_ALWAYS_TRY_PNF : 0,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL, // LogContext
                        &Inf,
                        &ErrorLineNumber,
                        NULL
                       );

        if(d != NO_ERROR) {
            SetLastError(d);
            return(FALSE);
        }

        UnloadInf = TRUE;
    }

    //
    // Determine the number of infs associated with this handle,
    // and calculate the amount of space that will be needed to
    // store version information about them.
    //
    // For each inf we will need space for the version block,
    // as well as an offset in the SP_INF_INFORMATION structure
    // to indicate where that inf's version block is located
    // in the output buffer.
    //
    TotalSpaceRequired = offsetof(SP_INF_INFORMATION, VersionData);
    for(InfCount = 0, CurInf = Inf;
        CurInf;
        InfCount++, CurInf = CurInf->Next)
    {
        OriginalFilenameSize = CurInf->OriginalInfName
                             ? (lstrlen(CurInf->OriginalInfName) + 1) * sizeof(TCHAR)
                             : 0;

        TotalSpaceRequired += (offsetof(INF_VERSION_BLOCK, Filename) +
                               CurInf->VersionBlock.FilenameSize +
                               CurInf->VersionBlock.DataSize +
                               OriginalFilenameSize
                              );
    }

    if(RequiredSize) {
        *RequiredSize = TotalSpaceRequired;
    }

    //
    // See if we have a large enough output buffer.
    // If we have a large enough buffer then set up some
    // initial values in it.
    //
    if(ReturnBufferSize < TotalSpaceRequired) {
        if(UnloadInf) {
            FreeInfFile(Inf);
        }
        if(ReturnBuffer) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        } else {
            return TRUE;
        }
    }

    d = NO_ERROR;

    try {
        ReturnBuffer->InfStyle = Inf->Style;
        ReturnBuffer->InfCount = InfCount;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        if(UnloadInf) {
            FreeInfFile(Inf);
        }
        SetLastError(d = ERROR_INVALID_PARAMETER);
    }

    if(d != NO_ERROR) {
        return FALSE;
    }

    Out = (PUCHAR)ReturnBuffer + offsetof(SP_INF_INFORMATION, VersionData);

    //
    // Traverse all infs associated with this inf handle and copy
    // version data into the caller's buffer. Guard with SEH to ensure
    // that the caller passed a valid buffer.
    //
    try {
        Prev = NULL;
        for(CurInf = Inf; CurInf; CurInf = CurInf->Next) {
            //
            // Store offset into
            //
            if(Prev) {
                Prev->NextOffset = (UINT)((UINT_PTR)Out - (UINT_PTR)ReturnBuffer);
            }
            Prev = (PVOID)Out;

            OriginalFilenameSize = CurInf->OriginalInfName
                                 ? (lstrlen(CurInf->OriginalInfName) + 1) * sizeof(TCHAR)
                                 : 0;

            Prev->LastWriteTime = CurInf->VersionBlock.LastWriteTime;
            Prev->DatumCount    = CurInf->VersionBlock.DatumCount;
            Prev->OffsetToData  = CurInf->VersionBlock.FilenameSize + OriginalFilenameSize;
            Prev->DataSize      = CurInf->VersionBlock.DataSize;
            Prev->TotalSize     = offsetof(INF_VERSION_BLOCK, Filename) +
                                      CurInf->VersionBlock.FilenameSize +
                                      OriginalFilenameSize +
                                      CurInf->VersionBlock.DataSize;

            Out += offsetof(INF_VERSION_BLOCK, Filename);

            //
            // Now copy the filename, (optionally) original filename, and
            // version data into the output buffer.
            //
            CopyMemory(Out, CurInf->VersionBlock.Filename, CurInf->VersionBlock.FilenameSize);
            Out += CurInf->VersionBlock.FilenameSize;

            if(CurInf->OriginalInfName) {
                CopyMemory(Out, CurInf->OriginalInfName, OriginalFilenameSize);
                Out += OriginalFilenameSize;
            }

            CopyMemory(Out, CurInf->VersionBlock.DataBlock, CurInf->VersionBlock.DataSize);
            Out += CurInf->VersionBlock.DataSize;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if(UnloadInf) {
            FreeInfFile(Inf);
        }
        SetLastError(d = ERROR_INVALID_PARAMETER);
    }

    if(d != NO_ERROR) {
        return FALSE;
    }

    Prev->NextOffset = 0;

    //
    // Unload the inf if necessary
    //
    if(UnloadInf) {
        FreeInfFile(Inf);
    }

    return TRUE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueryInfFileInformationA(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    OUT PSTR                ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    WCHAR returnbuffer[MAX_PATH];
    DWORD requiredsize;
    DWORD rc;
    PSTR ansi;
    BOOL b;

    b = SetupQueryInfFileInformationW(
            InfInformation,
            InfIndex,
            returnbuffer,
            MAX_PATH,
            &requiredsize
            );

    rc = GetLastError();

    if(b) {
        if(ansi = UnicodeToAnsi(returnbuffer)) {

            rc = NO_ERROR;
            requiredsize = lstrlenA(ansi);

            if(RequiredSize) {
                try {
                    *RequiredSize = requiredsize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    rc = ERROR_INVALID_PARAMETER;
                    b = FALSE;
                }
            }

            if(b) {
                if(ReturnBuffer) {
                    if(ReturnBufferSize >= requiredsize) {
                        //
                        // lstrcpy returns NULL if it faults
                        //
                        if(!lstrcpyA(ReturnBuffer,ansi)) {
                            rc = ERROR_INVALID_PARAMETER;
                            b = FALSE;
                        }
                    } else {
                        b = FALSE;
                        rc = ERROR_INSUFFICIENT_BUFFER;
                    }
                }
            }

            MyFree(ansi);
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
        }
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupQueryInfFileInformationW(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    OUT PWSTR               ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfInformation);
    UNREFERENCED_PARAMETER(InfIndex);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueryInfFileInformation(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    OUT PTSTR               ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    UINT FilenameLength;
    INF_VERSION_NODE VersionNode;
    DWORD rc;

    //
    // See whether the index is in range and
    // retrieve the version descriptor for this inf.
    //
    rc = NO_ERROR;
    try {
        if(!pSetupVersionNodeFromInfInformation(InfInformation,InfIndex,&VersionNode,NULL)) {
            rc = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    FilenameLength = VersionNode.FilenameSize / sizeof(TCHAR);

    if(RequiredSize) {
        try {
            *RequiredSize = FilenameLength;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
        }
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    }

    //
    // Check length of user's buffer.
    //
    if(FilenameLength > ReturnBufferSize) {
        if(ReturnBuffer) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        } else {
            return TRUE;
        }
    }

    //
    // Copy the data into user's buffer.
    //
    try {
        CopyMemory(ReturnBuffer,VersionNode.Filename,VersionNode.FilenameSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    return TRUE;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupQueryInfOriginalFileInformationA(
    IN  PSP_INF_INFORMATION      InfInformation,
    IN  UINT                     InfIndex,
    IN  PSP_ALTPLATFORM_INFO     AlternatePlatformInfo, OPTIONAL
    OUT PSP_ORIGINAL_FILE_INFO_A OriginalFileInfo
    )
{
    SP_ORIGINAL_FILE_INFO_W UnicodeOriginalFileInfo;
    DWORD rc;
    int i;
    BOOL b;

    rc = NO_ERROR;

    //
    // Do an initial check on user-supplied output buffer to see if it seems
    // to be valid.
    //
    try {
        if(OriginalFileInfo->cbSize != sizeof(SP_ORIGINAL_FILE_INFO_A)) {
            rc = ERROR_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return FALSE;
    }

    UnicodeOriginalFileInfo.cbSize = sizeof(SP_ORIGINAL_FILE_INFO_W);

    b = SetupQueryInfOriginalFileInformationW(
            InfInformation,
            InfIndex,
            AlternatePlatformInfo,
            &UnicodeOriginalFileInfo
           );

    rc = GetLastError();

    if(b) {
        //
        // Convert the Unicode fields of the original file info structure into
        // ANSI, and store the information in the caller-supplied ANSI
        // structure.
        //
        try {
            //
            // First, translate/store the original INF name...
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    UnicodeOriginalFileInfo.OriginalInfName,
                    -1,
                    OriginalFileInfo->OriginalInfName,
                    sizeof(OriginalFileInfo->OriginalInfName),
                    NULL,
                    NULL
                    );

            //
            // ...and if that succeeded, then translate/store the original
            // catalog filename.
            //
            if(i) {
                //
                // Note that the original catalog filename may be the empty
                // string (i.e., the INF didn't specify an associated catalog
                // file).  We don't need to special-case this, since
                // WideCharToMultiByte can handle empty strings just fine.
                //
                i = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        UnicodeOriginalFileInfo.OriginalCatalogName,
                        -1,
                        OriginalFileInfo->OriginalCatalogName,
                        sizeof(OriginalFileInfo->OriginalCatalogName),
                        NULL,
                        NULL
                        );
            }

            if(!i) {
                b = FALSE;
                rc = GetLastError();
                //
                // If we start seeing cases where our Unicode->ANSI expansion
                // blows our buffersize, we need to know about it...
                //
                MYASSERT((rc != NO_ERROR) && (rc != ERROR_INSUFFICIENT_BUFFER));
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            rc = ERROR_INVALID_PARAMETER;
            b = FALSE;
        }
    }

    SetLastError(rc);

    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupQueryInfOriginalFileInformationW(
    IN  PSP_INF_INFORMATION      InfInformation,
    IN  UINT                     InfIndex,
    IN  PSP_ALTPLATFORM_INFO     AlternatePlatformInfo, OPTIONAL
    OUT PSP_ORIGINAL_FILE_INFO_W OriginalFileInfo
    )
{
    UNREFERENCED_PARAMETER(InfInformation);
    UNREFERENCED_PARAMETER(InfIndex);
    UNREFERENCED_PARAMETER(AlternatePlatformInfo);
    UNREFERENCED_PARAMETER(OriginalFileInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupQueryInfOriginalFileInformation(
    IN  PSP_INF_INFORMATION      InfInformation,
    IN  UINT                     InfIndex,
    IN  PSP_ALTPLATFORM_INFO     AlternatePlatformInfo, OPTIONAL
    OUT PSP_ORIGINAL_FILE_INFO   OriginalFileInfo
    )

/*++

Routine Description:

    This routine returns an INF's original name (which will be different from
    its current name if the INF was installed into %windir%\Inf, for example).
    If the INF's original name is the same as its current name, the current
    name is returned.

    It also returns the original filename of the catalog file specified by the
    INF via a (potentially decorated) CatalogFile= entry in the INF's [version]
    section.  The OS/architecture-specific decoration may be overridden from
    the default (i.e., current platform) by passing in an optional alternate
    platform information structure.  If the INF doesn't specify any catalog
    file, then this field in the output OriginalFileInfo structure will be set
    to an empty string.

    Both filenames returned in the OriginalFileInfo are simple filenames, i.e.,
    there is no path information included.

Arguments:

    InfInformation - supplies context from which we retrieve information about
        the INF whose index is specified by InfIndex.

    InfIndex - supplies the zero-based index of the INF within the
        InfInformation context buffer that we're retrieving original file
        information for.

    AlternatePlatformInfo - optionally, supplies alternate platform information
        used when searching for the appropriately decorated CatalogFile= entry
        within the INF's [version] section.

    OriginalFileInfo - supplies the address of an original file information
        buffer that upon success receives information about the original
        (simple) filenames of files associated with this INF.  This structure
        must have its cbSize field set to sizeof(SP_ORIGINAL_FILE_INFO) upon
        entry to this routine or the call will fail with GetLastError()
        returning ERROR_INVALID_USER_BUFFER.

        The fields of this structure are set upon successful return as follows:

        OriginalInfName - receives the INF's original filename, which may be
            different than its current filename in the case where the INF was
            an OEM in that was installed into the %windir%\Inf directory (e.g.,
            via SetupCopyOEMInf).

        OriginalCatalogName - receives the platform-appropriate CatalogFile=
            entry in the INF's [version] section (where the platform is the
            default native one unless AlternatePlatformInfo is supplied).  If
            there is no applicable CatalogFile= entry, this field will be set
            to the empty string.

Return Value:

    If successful, the return value is non-zero.
    If unsuccessful, the return value is FALSE, and GetLastError() may be
    called to determine the cause of failure.

--*/

{
    INF_VERSION_NODE VersionNode;
    DWORD rc;

    rc = NO_ERROR;
    //
    // See whether the index is in range and retrieve the version descriptor
    // and original filename for this inf.
    //
    try {
        //
        // Do an initial check on user-supplied output buffer to see if it
        // seems to be valid.
        //
        if(OriginalFileInfo->cbSize != sizeof(SP_ORIGINAL_FILE_INFO)) {
            rc = ERROR_INVALID_USER_BUFFER;
            goto clean0;
        }

        if(!pSetupVersionNodeFromInfInformation(InfInformation,
                                                InfIndex,
                                                &VersionNode,
                                                OriginalFileInfo->OriginalInfName)) {
            rc = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Now retrieve the (platform-appropriate) catalog file associated with
        // this INF (if there is one).
        //
        if(!pSetupGetCatalogFileValue(&VersionNode,
                                      OriginalFileInfo->OriginalCatalogName,
                                      SIZECHARS(OriginalFileInfo->OriginalCatalogName),
                                      AlternatePlatformInfo)) {
            //
            // No applicable CatalogFile= entry found--set field to empty
            // string.
            //
            *(OriginalFileInfo->OriginalCatalogName) = TEXT('\0');
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupQueryInfVersionInformationA(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    IN  PCSTR               Key,              OPTIONAL
    OUT PSTR                ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    INF_VERSION_NODE VersionNode;
    PCWSTR Data;
    DWORD rc;
    PSTR ansidata;
    UINT ansilength;
    PCWSTR key;

    //
    // See whether the index is in range and
    // get pointer to version descriptor for this inf.
    //
    try {
        if(pSetupVersionNodeFromInfInformation(InfInformation,InfIndex,&VersionNode,NULL)) {
            //
            // See whether we want a specific value.
            //
            if(Key) {

                rc = CaptureAndConvertAnsiArg(Key,&key);
                if(rc == NO_ERROR) {

                    if(Data = pSetupGetVersionDatum(&VersionNode,key)) {

                        if(ansidata = UnicodeToAnsi(Data)) {

                            ansilength = lstrlenA(ansidata) + 1;
                            if(RequiredSize) {
                                *RequiredSize = ansilength;
                            }

                            if(ReturnBuffer) {
                                if(ReturnBufferSize >= ansilength) {
                                    CopyMemory(ReturnBuffer,ansidata,ansilength);
                                    rc = NO_ERROR;
                                } else {
                                    rc = ERROR_INSUFFICIENT_BUFFER;
                                }
                            } else {
                                rc = NO_ERROR;
                            }

                            MyFree(ansidata);
                        } else {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    } else {
                        rc = ERROR_INVALID_DATA;
                    }

                    MyFree(key);
                }
            } else {
                //
                // Caller wants all values. Copy whole data block to caller's buffer,
                // plus a terminating NUL character.
                //
                // Maximum size the data could be in ansi is the exact same
                // size it is in unicode, if every char is a double-byte char.
                //
                if(ansidata = MyMalloc(VersionNode.DataSize)) {

                    ansilength = WideCharToMultiByte(
                                    CP_ACP,
                                    0,
                                    (PWSTR)VersionNode.DataBlock,
                                    VersionNode.DataSize / sizeof(WCHAR),
                                    ansidata,
                                    VersionNode.DataSize,
                                    NULL,
                                    NULL
                                    );

                    if(RequiredSize) {
                        //
                        // account for terminating nul
                        //
                        *RequiredSize = ansilength+1;
                    }

                    if(ReturnBuffer) {
                        if(ReturnBufferSize >= *RequiredSize) {
                            CopyMemory(ReturnBuffer,ansidata,ansilength);
                            ReturnBuffer[ansilength] = 0;
                            rc = NO_ERROR;
                        } else {
                            rc = ERROR_INSUFFICIENT_BUFFER;
                        }
                    } else {
                        rc = NO_ERROR;
                    }

                    MyFree(ansidata);
                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        } else {
            rc = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
SetupQueryInfVersionInformationW(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    IN  PCWSTR              Key,              OPTIONAL
    OUT PWSTR               ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfInformation);
    UNREFERENCED_PARAMETER(InfIndex);
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupQueryInfVersionInformation(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    IN  PCTSTR              Key,              OPTIONAL
    OUT PTSTR               ReturnBuffer,     OPTIONAL
    IN  DWORD               ReturnBufferSize,
    OUT PDWORD              RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INF_VERSION_NODE VersionNode;
    PCTSTR Data;
    UINT DataLength;
    DWORD rc;

    //
    // See whether the index is in range and
    // get pointer to version descriptor for this inf.
    //
    try {
        if(pSetupVersionNodeFromInfInformation(InfInformation,InfIndex,&VersionNode,NULL)) {
            //
            // See whether we want a specific value.
            //
            if(Key) {
                if(Data = pSetupGetVersionDatum(&VersionNode,Key)) {

                    DataLength = lstrlen(Data) + 1;
                    if(RequiredSize) {
                        *RequiredSize = DataLength;
                    }

                    if(ReturnBuffer) {
                        if(ReturnBufferSize >= DataLength) {
                            CopyMemory(ReturnBuffer,Data,DataLength * sizeof(TCHAR));
                            rc = NO_ERROR;
                        } else {
                            rc = ERROR_INSUFFICIENT_BUFFER;
                        }
                    } else {
                        rc = NO_ERROR;
                    }
                } else {
                    rc = ERROR_INVALID_DATA;
                }
            } else {
                //
                // Caller wants all values. Copy whole data block to caller's buffer,
                // plus a terminating NUL character.
                //
                DataLength = (VersionNode.DataSize / sizeof(TCHAR)) + 1;
                if(RequiredSize) {
                    *RequiredSize = DataLength;
                }

                if(ReturnBuffer) {
                    if(ReturnBufferSize >= DataLength) {
                        CopyMemory(ReturnBuffer,VersionNode.DataBlock,VersionNode.DataSize);
                        ReturnBuffer[VersionNode.DataSize/sizeof(TCHAR)] = 0;
                        rc = NO_ERROR;
                    } else {
                        rc = ERROR_INSUFFICIENT_BUFFER;
                    }
                } else {
                    rc = NO_ERROR;
                }
            }
        } else {
            rc = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_PARAMETER;
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}



BOOL
_SetupGetInfFileList(
    IN  PCTSTR DirectoryPath,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PVOID  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
#ifdef UNICODE
    IN ,BOOL   ConvertToAnsi
#endif
    )
{
    TCHAR SearchSpec[MAX_PATH];
    PTCHAR FilenameStart;
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    DWORD Style;
    UINT FileNameLength;
    DWORD RemainingSpaceInBuffer;
    DWORD CurrentOffsetInBuffer;
    DWORD TotalSpaceNeededInBuffer;
    BOOL InsufficientBuffer;
    DWORD d;
    PTSTR DontCare;
#ifdef UNICODE
    CHAR ansi[MAX_PATH];
#endif

    //
    // Set up the search directory
    //
    if(DirectoryPath) {
        //
        // Make sure this directory path is fully-qualified.
        //
        d = GetFullPathName(DirectoryPath,
                            SIZECHARS(SearchSpec),
                            SearchSpec,
                            &DontCare
                           );

        if(!d) {
            //
            // LastError has already been set.
            //
            return FALSE;
        } else if(d >= SIZECHARS(SearchSpec)) {
            MYASSERT(0);
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }

    } else {
        lstrcpy(SearchSpec, InfDirectory);
    }

    ConcatenatePaths(SearchSpec, pszInfWildcard, SIZECHARS(SearchSpec), NULL);
    FilenameStart = (PTSTR)MyGetFileTitle(SearchSpec);

    FindHandle = FindFirstFile(SearchSpec,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        if((d == ERROR_NO_MORE_FILES) || (d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
            if(RequiredSize) {
                d = NO_ERROR;
                try {
                    *RequiredSize = 1;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    d = ERROR_INVALID_PARAMETER;
                }
                if(d != NO_ERROR) {
                    SetLastError(d);
                    return(FALSE);
                }
            }
            if(ReturnBuffer) {
                if(ReturnBufferSize) {
                    d = NO_ERROR;
                    try {
#ifdef UNICODE
                        if(ConvertToAnsi) {
                            *(PCHAR)ReturnBuffer = 0;
                        } else
#endif
                        *(PTCHAR)ReturnBuffer = 0;
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        d = ERROR_INVALID_PARAMETER;
                    }
                    SetLastError(d);
                    return(d == NO_ERROR);
                } else {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
            } else {
                return TRUE;
            }
        }
        SetLastError(d);
        return(FALSE);
    }

    //
    // Leave space for the extra terminating nul char.
    //
    RemainingSpaceInBuffer = ReturnBufferSize;
    if(RemainingSpaceInBuffer) {
        RemainingSpaceInBuffer--;
    }

    TotalSpaceNeededInBuffer = 1;
    CurrentOffsetInBuffer = 0;

    InsufficientBuffer = FALSE;
    d = NO_ERROR;

    do {
        //
        // Skip directories
        //
        if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        //
        // Form full pathname of file in SearchSpec.
        //
        lstrcpy(FilenameStart,FindData.cFileName);

        //
        // Determine the inf type and see whether the caller
        // wants to know about infs of this type.
        //
        Style = DetermineInfStyle(SearchSpec, &FindData);
        if((Style == INF_STYLE_NONE) || !(Style & InfStyle)) {
            continue;
        }

        //
        // Got a legit inf file.
        //
#ifdef UNICODE
        if(ConvertToAnsi) {
            //
            // The nul is included because it's converted
            // so no need to add 1
            //
            FileNameLength = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                FindData.cFileName,
                                -1,
                                ansi,
                                MAX_PATH,
                                NULL,
                                NULL
                                );
        } else
#endif
        FileNameLength = lstrlen(FindData.cFileName) + 1;

        TotalSpaceNeededInBuffer += FileNameLength;

        if(ReturnBuffer) {

            if(RemainingSpaceInBuffer >= FileNameLength ) {

                RemainingSpaceInBuffer -= FileNameLength;

                //
                // lstrcpy will return NULL if it faults
                //
#ifdef UNICODE
                if(ConvertToAnsi) {
                    DontCare = (PVOID)lstrcpyA((PCHAR)ReturnBuffer+CurrentOffsetInBuffer,ansi);
                } else
#endif
                DontCare = lstrcpy((PTCHAR)ReturnBuffer+CurrentOffsetInBuffer,FindData.cFileName);

                if(!DontCare) {

                    d = ERROR_INVALID_PARAMETER;

                } else {

                    CurrentOffsetInBuffer += FileNameLength;

                    try {
#ifdef UNICODE
                        if(ConvertToAnsi) {
                            ((PCHAR)ReturnBuffer)[CurrentOffsetInBuffer] = 0;
                        } else
#endif
                        ((PTCHAR)ReturnBuffer)[CurrentOffsetInBuffer] = 0;
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        d = ERROR_INVALID_PARAMETER;
                    }
                }
            } else {
                InsufficientBuffer = TRUE;
            }
        }

    } while((d == NO_ERROR) && FindNextFile(FindHandle,&FindData));

    FindClose(FindHandle);

    if(d != NO_ERROR) {
        SetLastError(d);
    }

    if(GetLastError() == ERROR_NO_MORE_FILES) {

        d = NO_ERROR;

        try {
            if(RequiredSize) {
                *RequiredSize = TotalSpaceNeededInBuffer;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }

        if(d == NO_ERROR) {
            if(InsufficientBuffer) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            return(TRUE);
        } else {
            SetLastError(d);
        }
    }

    //
    // Last error already set
    //
    return(FALSE);
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupGetInfFileListA(
    IN  PCSTR  DirectoryPath,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PSTR   ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    PWSTR dirpath;
    DWORD rc;
    BOOL b;

    if(DirectoryPath) {
        rc = CaptureAndConvertAnsiArg(DirectoryPath,&dirpath);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    } else {
        dirpath = NULL;
    }


    b = _SetupGetInfFileList(dirpath,InfStyle,ReturnBuffer,ReturnBufferSize,RequiredSize,TRUE);
    rc = GetLastError();

    if(dirpath) {
        MyFree(dirpath);
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupGetInfFileListW(
    IN  PCWSTR DirectoryPath,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PWSTR  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DirectoryPath);
    UNREFERENCED_PARAMETER(InfStyle);
    UNREFERENCED_PARAMETER(ReturnBuffer);
    UNREFERENCED_PARAMETER(ReturnBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupGetInfFileList(
    IN  PCTSTR DirectoryPath,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PTSTR  ReturnBuffer,     OPTIONAL
    IN  DWORD  ReturnBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PTSTR dirpath;
    DWORD rc;
    BOOL b;

    if(DirectoryPath) {
        rc = CaptureStringArg(DirectoryPath,&dirpath);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(FALSE);
        }
    } else {
        dirpath = NULL;
    }


    b = _SetupGetInfFileList(
            dirpath,
            InfStyle,
            ReturnBuffer,
            ReturnBufferSize,
            RequiredSize
#ifdef UNICODE
           ,FALSE
#endif
            );

    rc = GetLastError();

    if(dirpath) {
        MyFree(dirpath);
    }

    SetLastError(rc);
    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
HINF
SetupOpenInfFileA(
    IN  PCSTR FileName,
    IN  PCSTR InfType,     OPTIONAL
    IN  DWORD InfStyle,
    OUT PUINT ErrorLine    OPTIONAL
    )
{
    PCTSTR fileName,infType;
    DWORD err;
    HINF h;

    err = NO_ERROR;
    fileName = NULL;
    infType = NULL;

    //
    // Set error line to 0 since ansi arg conversion could fail
    // and we need to indicate that there's no error in the inf itself.
    //
    if(ErrorLine) {
        try {
            *ErrorLine = 0;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            err = ERROR_INVALID_PARAMETER;
        }
    }

    if(err == NO_ERROR) {
        err = CaptureAndConvertAnsiArg(FileName,&fileName);
        if((err == NO_ERROR) && InfType) {
            err = CaptureAndConvertAnsiArg(InfType,&infType);
        }
    }

    if(err == NO_ERROR) {
        h = SetupOpenInfFileW(fileName,infType,InfStyle,ErrorLine);
        err = GetLastError();
    } else {
        h = INVALID_HANDLE_VALUE;
    }

    if(fileName) {
        MyFree(fileName);
    }
    if(infType) {
        MyFree(infType);
    }

    SetLastError(err);
    return(h);
}
#else
//
// Unicode stub
//
HINF
SetupOpenInfFileW(
    IN  PCWSTR FileName,
    IN  PCWSTR InfType,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PUINT  ErrorLine   OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(InfType);
    UNREFERENCED_PARAMETER(InfStyle);
    UNREFERENCED_PARAMETER(ErrorLine);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif


HINF
SetupOpenInfFile(
    IN  PCTSTR FileName,
    IN  PCTSTR InfClass,    OPTIONAL
    IN  DWORD  InfStyle,
    OUT PUINT  ErrorLine    OPTIONAL
    )

/*++

Routine Description:

    This routine opens an INF file and returns a handle to it.

Arguments:

    FileName - Supplies the address of a NULL-terminated string containing
        the name (and optionally, the path) of the INF file to be opened.  If
        the filename contains no path separator characters, it is searched for
        first in the %windir%\inf directory, and then in the %windir%\system32
        directory.  Otherwise, the name is assumed to be a full path
        specification, and no is opened as-is.

    InfClass - Optionally, supplies the address of a NULL-terminated string
        containing the class of the INF file desired.  For old-style (i.e.,
        Windows NT 3.x script-base) INF files, this string must match the type
        specified in the OptionType value of the [Identification] section of the
        INF (e.g., OptionType=NetAdapter).  For Windows 95-compatibile INF
        files, this string must match the class of the specified INF.  If the
        INF has a Class value in its [version] section, then this value is used
        for the comparison.  If no Class value is present, but a ClassGUID value
        is present in the [version] section, then the corresponding class name
        for that GUID is retrieved, and comparison is done based on that name.

    InfStyle - Specifies the style of the INF to open.  May be a combination of
        the following flags:

            INF_STYLE_OLDNT - Windows NT 3.x script-based INF files.

            INF_STYLE_WIN4 - Windows 95-compatible INF files.

            INF_STYLE_CACHE_ENABLE - always cache INF, even outside of
                %windir%\Inf.

            INF_STYLE_CACHE_DISABLE - delete INF from cache, if outside of
                %windir%\Inf.

    ErrorLine - If an error occurs loading the file, this parameter receives the
        (1-based) line number where the error occurred.  This value is generally
        reliable only if GetLastError does not return ERROR_NOT_ENOUGH_MEMORY.
        If out-of-memory does occur, the ErrorLine may be 0.

Return Value:

    If the function succeeds, the return value is a handle to the opened INF
    file.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    If the load fails because the INF class does not match InfClass, the
    function returns FALSE, and GetLastError returns ERROR_CLASS_MISMATCH.

    The SetupCloseInfFile function is used to close a handle returned by
    SetupOpenInfFile.

    If multiple INF styles are specified, the style of the INF file opened may
    be ascertained by calling SetupGetInfInformation.

    Since there may be multiple class GUIDs all having the same class name,
    callers interested in only INFs of a particular class (i.e., a particular
    class GUID) should retrieve the ClassGuid value from the INF to verify that
    the class matches exactly.

    If both the INF_STYLE_CACHE_ENABLE and INF_STYLE_CACHE_DISABLE InfStyle
    flags are specified, then the existing cached information that is maintained
    about the INF (e.g., original source location) is discarded, and the INF is
    re-added to the cache without this old information.

    (Internal:  Presently, the INF_STYLE_CACHE_ENABLE and
    INF_STYLE_CACHE_DISABLE flags cause us to create or delete PNFs outside of
    %windir%\Inf.  Their rather vague-sounding names were chosen to reflect the
    possibility of modifying the caching/indexing scheme in the future.)

--*/

{
    UINT errorLine;
    DWORD d;
    PLOADED_INF Inf;
    PCTSTR Class;
    TCHAR TempString[MAX_PATH];
    GUID ClassGuid;
    HRESULT hr;
    BOOL TryPnf;
    WIN32_FIND_DATA FindData;
    PTSTR DontCare;
    PTSTR TempCharPtr = NULL;

    //
    // Determine whether just the filename (no path) was specified.  If so,
    // look for it in the DevicePath directory search path.  Otherwise,
    // use the path as-is.
    //
    try {
        if(FileName == MyGetFileTitle(FileName)) {
            //
            // The specified INF name is a simple filename.  Search for it in
            // the INF directories using the default search order.
            //
            d = SearchForInfFile(
                    FileName,
                    &FindData,
                    INFINFO_DEFAULT_SEARCH,
                    TempString,
                    SIZECHARS(TempString),
                    NULL
                    );

            if(d == NO_ERROR) {
                TryPnf = TRUE;
            }
        } else {
            //
            // The specified INF filename contains more than just a filename.
            // Assume it's an absolute path.  (We need to make sure it's
            // fully-qualified, because that's what LoadInfFile expects.)
            //
            d = GetFullPathName(FileName,
                                SIZECHARS(TempString),
                                TempString,
                                &DontCare
                               );
            if(!d) {
                d = GetLastError();
            } else if(d >= SIZECHARS(TempString)) {
                MYASSERT(0);
                d = ERROR_BUFFER_OVERFLOW;
            } else {
                //
                // We successfully retrieved the full pathname, now see if the
                // file exists.
                //
                if(FileExists(TempString, &FindData)) {
                    //
                    // We have everything we need to load this INF.
                    //
                    InfSourcePathFromFileName(TempString, &TempCharPtr, &TryPnf);
                    d = NO_ERROR;
                } else {
                    d = GetLastError();
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Assume FileName was invalid and thus MyGetFileTitle fell over.
        //
        d = ERROR_INVALID_PARAMETER;
        TempCharPtr = TempCharPtr;
    }

    if(d != NO_ERROR) {
        goto PrepareForReturn;
    }

    if(InfStyle & INF_STYLE_CACHE_DISABLE) {
        //
        // Delete the existing PNF for this INF (if any).
        //
        TCHAR PnfFullPath[MAX_PATH];
        PTSTR PnfFileName, PnfFileExt;

        lstrcpy(PnfFullPath, TempString);

        //
        // Find the start of the filename component of the path, and then find
        // the last period (if one exists) in that filename.
        //
        PnfFileName = (PTSTR)MyGetFileTitle(PnfFullPath);
        if(!(PnfFileExt = _tcsrchr(PnfFileName, TEXT('.')))) {
            PnfFileExt = PnfFullPath + lstrlen(PnfFullPath);
        }

        //
        // Now create a corresponding filename with the extension '.PNF'
        //
        lstrcpyn(PnfFileExt, pszPnfSuffix, SIZECHARS(PnfFullPath) - (int)(PnfFileExt - PnfFullPath));

        SetFileAttributes(PnfFullPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(PnfFullPath);
    }

    if(InfStyle & INF_STYLE_CACHE_ENABLE) {
        //
        // The caller has requested that this INF be cached (even though it may
        // be outside of our INF search path).
        //
        TryPnf = TRUE;
    }

    try {
        d = LoadInfFile(
                TempString,
                &FindData,
                InfStyle,
                TryPnf ? LDINF_FLAG_ALWAYS_TRY_PNF : 0,
                NULL,
                TempCharPtr,
                NULL,
                NULL,
                NULL, // LogContext
                &Inf,
                &errorLine,
                NULL
                );
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    if(d == NO_ERROR) {

        if(InfClass) {

            d = ERROR_CLASS_MISMATCH;   // assume mismatch

            //
            // Match based on class of inf. The following check works for
            // both new and old style infs, because old-style infs use
            // [Identification].OptionType as the class (see oldinf.c
            // function ProcessOldInfVersionBlock()).
            //
            if(Class = pSetupGetVersionDatum(&(Inf->VersionBlock), pszClass)) {
                try {
                    if(!lstrcmpi(Class,InfClass)) {
                        d = NO_ERROR;
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    d = ERROR_INVALID_PARAMETER;
                }
            } else {
                //
                // No Class entry--check for ClassGUID entry.
                //
                if(Class = pSetupGetVersionDatum(&(Inf->VersionBlock), pszClassGuid)) {
                    //
                    // Get the class name associated with this GUID, and see if it
                    // matches the caller-supplied class name.
                    //
                    // (We have to cast away the CONST-ness of the Class string, because
                    // the prototype for CLSIDFromString doesn't specify this parameter
                    // as constant.)
                    //
                    if((hr = pSetupGuidFromString((PTSTR)Class, &ClassGuid)) == S_OK) {

                        if(SetupDiClassNameFromGuid(&ClassGuid,
                                                    TempString,
                                                    SIZECHARS(TempString),
                                                    NULL)) {

                            try {
                                if(!lstrcmpi(TempString,InfClass)) {
                                    d = NO_ERROR;
                                }
                            } except(EXCEPTION_EXECUTE_HANDLER) {
                                d = ERROR_INVALID_PARAMETER;
                            }
                        }
                    } else {
                        if(hr == E_OUTOFMEMORY) {
                            d = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                }
            }

            if(d != NO_ERROR) {
                FreeInfFile(Inf);
            }
        }

    } else {
        if(ErrorLine) {
            try {
                *ErrorLine = errorLine;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                d = ERROR_INVALID_PARAMETER;
            }
        }
    }

PrepareForReturn:

    if(TempCharPtr) {
        MyFree(TempCharPtr);
    }

    SetLastError(d);

    return((d == NO_ERROR) ? (HINF)Inf : (HINF)INVALID_HANDLE_VALUE);
}


HINF
SetupOpenMasterInf(
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    TCHAR FileName[MAX_PATH];

    lstrcpy(FileName,InfDirectory);
    lstrcat(FileName,TEXT("\\LAYOUT.INF"));

    return(SetupOpenInfFile(FileName,NULL,INF_STYLE_WIN4,NULL));
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupOpenAppendInfFileA(
    IN  PCSTR  FileName,    OPTIONAL
    IN  HINF   InfHandle,
    OUT PUINT  ErrorLine    OPTIONAL
    )
{
    PCWSTR fileName;
    DWORD d;
    BOOL b;

    //
    // Set error line to 0 since ansi arg conversion could fail
    // and we need to indicate that there's no error in the inf itself.
    //
    d = NO_ERROR;
    if(ErrorLine) {
        try {
            *ErrorLine = 0;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
    }

    if(d == NO_ERROR) {
        if(FileName) {
            d = CaptureAndConvertAnsiArg(FileName,&fileName);
        } else {
            fileName = NULL;
        }
    }

    if(d == NO_ERROR) {
        b = SetupOpenAppendInfFileW(fileName,InfHandle,ErrorLine);
        d = GetLastError();
    } else {
        b = FALSE;
    }

    if(fileName) {
        MyFree(fileName);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupOpenAppendInfFileW(
    IN  PCWSTR FileName,    OPTIONAL
    IN  HINF   InfHandle,
    OUT PUINT  ErrorLine    OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(ErrorLine);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupOpenAppendInfFile(
    IN  PCTSTR FileName,    OPTIONAL
    IN  HINF   InfHandle,
    OUT PUINT  ErrorLine    OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF ExistingInf = NULL, CurInf = NULL;
    DWORD d = NO_ERROR;
    TCHAR Filename[2][MAX_PATH];
    UINT FilenameCount, i, Field;
    UINT errorLine = 0;
    BOOL LookInInfDirAlso;
    BOOL TryPnf;
    WIN32_FIND_DATA FindData;
    PTSTR TempCharPtr = NULL;
    PTSTR DontCare;
    PINF_SECTION InfSection;
    UINT LineNumber;
    PINF_LINE InfLine = NULL;

    try {

        if(LockInf((PLOADED_INF)InfHandle)) {
            ExistingInf = (PLOADED_INF)InfHandle;
        } else {
            d = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        //
        // Check INF Signature field as a further validation on the InfHandle.
        //
        if(ExistingInf->Signature != LOADED_INF_SIG) {
            d = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        //
        // Only allow this for win95-style infs.
        //
        if(ExistingInf->Style != INF_STYLE_WIN4) {
            d = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // If there is no filename, search through the list of existing INFs, looking
        // for a layout entry in their version blocks.  We begin at the end of the list,
        // and search backward, using the first layout file we encounter.  This allows
        // for the possibility of append-loading several INFs together (e.g., as done by the
        // optional components dialog), and calling SetupOpenAppendInfFile with no filename
        // for each.  Each INF could specify its own layout file, and everything works great.
        // (NOTE: In the above example, if all the INFs specified the same layout file, it
        // would only get loaded once, as expected.)
        //
        // We also can now handle 'LayoutFile' entries that specify multiple layout files.  E.g.,
        //
        //     LayoutFile = pluslay.inf, layout.inf
        //
        // In the above example, we would append-load 'pluslay.inf', followed by 'layout.inf'.
        // Because of the way we store INFs, any duplicate entries in both INFs would resolve in
        // favor of pluslay.inf, since it was loaded first (unless, of course, layout.inf was
        // already in our list of loaded INFs).
        //
        if(!FileName) {
            //
            // First, find the end of the list.
            //
            for(CurInf = ExistingInf; CurInf->Next; CurInf = CurInf->Next);

            //
            // Now, search the list, back-to-front, looking for a layout file in each INF's
            // [version] section.
            //
            for(; CurInf; CurInf = CurInf->Prev) {
                //
                // Locate the [Version] section.
                //
                if(InfSection = InfLocateSection(CurInf, pszVersion, NULL)) {
                    //
                    // Now look for a LayoutFile line.
                    //
                    LineNumber = 0;
                    if(InfLocateLine(CurInf, InfSection, pszLayoutFile, &LineNumber, &InfLine)) {
                        //
                        // We've found the line containing the INFs to be append-
                        // loaded.  Get the first field on the line to start off
                        // our loop below.
                        //
                        FileName = InfGetField(CurInf, InfLine, 1, NULL);
                        break;
                    } else {
                        //
                        // Make sure InfLine is still NULL, so we won't try to use it.
                        //
                        InfLine = NULL;
                    }
                }
            }

            if(!FileName) {
                //
                // Then we didn't find any INFs that specify a layout file.
                //
                d = ERROR_INVALID_DATA;
                goto clean0;
            }
        }

        //
        // Now append-load the INF (or the possibly multiple INFs, if we're
        // using a LayoutFile line).
        //
        for(Field = 1;
            FileName;
            FileName = InfLine ? InfGetField(CurInf, InfLine, ++Field, NULL) : NULL) {

            FilenameCount = 0;
            LookInInfDirAlso = TRUE;
            TryPnf = FALSE;

            //
            // Determine whether just the filename (no path) was specified.
            //
            if(FileName == MyGetFileTitle(FileName)) {
                //
                // If we retrieved this filename from an INF's [version] section,
                // then we first attempt to open up the layout file from the
                // directory where we found the INF.  If we don't find it in that
                // directory, and that directory wasn't the Inf directory, then
                // we try to open it up in %windir%\Inf as well.
                //
                if(CurInf) {
                    //
                    // Copy the path without the ending backslash character,
                    // because that's how the 'InfDirectory' string is formatted.
                    //
                    lstrcpyn(Filename[0],
                             CurInf->VersionBlock.Filename,
                             (int)(MyGetFileTitle(CurInf->VersionBlock.Filename) - CurInf->VersionBlock.Filename)
                            );

                    //
                    // Compare this path against the InfDirectory path, to see
                    // if they're the same.
                    //
                    if(!lstrcmpi(Filename[0], InfDirectory)) {
                        TryPnf = TRUE;
                        LookInInfDirAlso = FALSE;
                    }

                    //
                    // Now concatenate the layout filename onto the path.
                    //
                    ConcatenatePaths(Filename[0], FileName, MAX_PATH, NULL);
                    FilenameCount = 1;

                    //
                    // If 'TryPnf' is still FALSE, then that means that the INF
                    // wasn't in the INF directory.  Now find out if it's in a
                    // location that requires a non-NULL SourcePath (i.e.,
                    // something other than the default).
                    //
                    if(!TryPnf) {
                        InfSourcePathFromFileName(Filename[0], &TempCharPtr, &TryPnf);
                    }
                }

                if(LookInInfDirAlso) {
                    lstrcpy(Filename[FilenameCount], InfDirectory);
                    ConcatenatePaths(Filename[FilenameCount], FileName, MAX_PATH, NULL);

                    if(!FilenameCount) {
                        TryPnf = TRUE;
                    }

                    FilenameCount++;
                }

            } else {
                //
                // The INF filename contains more than just a filename.  Assume
                // it's an absolute path.  (We need to make sure it's fully-
                // qualified, because that's what LoadInfFile expects.)
                //
                d = GetFullPathName(FileName,
                                    SIZECHARS(Filename[0]),
                                    Filename[0],
                                    &DontCare
                                   );
                if(!d) {
                    d = GetLastError();
                    goto clean0;
                } else if(d >= SIZECHARS(Filename[0])) {
                    MYASSERT(0);
                    d = ERROR_BUFFER_OVERFLOW;
                    goto clean0;
                }

                InfSourcePathFromFileName(Filename[0], &TempCharPtr, &TryPnf);
                FilenameCount = 1;
                //
                // (Since we're setting FilenameCount to 1, we know we'll go
                // through the loop below at least once, thus d will get set to
                // the proper error, so we don't have to re-initialize it to
                // NO_ERROR here.)
                //
            }

            for(i = 0; i < FilenameCount; i++) {
                //
                // Load the inf
                //
                if(FileExists(Filename[i], &FindData)) {

                    if((d = LoadInfFile(Filename[i],
                                        &FindData,
                                        INF_STYLE_WIN4,
                                        (i | TryPnf) ? LDINF_FLAG_ALWAYS_TRY_PNF : 0,
                                        NULL,
                                        (i | TryPnf) ? NULL : TempCharPtr,
                                        NULL,
                                        ExistingInf,
                                        ExistingInf->LogContext,
                                        &ExistingInf,
                                        &errorLine,
                                        NULL)) == NO_ERROR) {
                        break;
                    }
                } else {
                    d = GetLastError();
                }
            }

            //
            // We no longer need the INF source path--free it if necessary.
            //
            if(TempCharPtr) {
                MyFree(TempCharPtr);
                TempCharPtr = NULL;
            }

            if(d != NO_ERROR) {
                break;
            }
        }

clean0:
        //
        // If the caller requested it, give them the line number at which any error occurred.
        // (This may be zero on non-parse errors.)
        //
        if(ErrorLine) {
            *ErrorLine = errorLine;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we hit an AV, then use invalid parameter error, otherwise, assume an inpage error when dealing
        // with a mapped-in file.
        //
        d = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;

        if(TempCharPtr) {
            MyFree(TempCharPtr);
        }

        //
        // Access the 'ExistingInf' variable, so that the compiler will respect our statement
        // ordering w.r.t. this variable.  Otherwise, we may not always know whether or not
        // we should be unlocking this INF.
        //
        ExistingInf = ExistingInf;
    }

    if(ExistingInf) {
        UnlockInf(ExistingInf);
    }

    SetLastError(d);

    return(d == NO_ERROR);
}



VOID
SetupCloseInfFile(
    IN HINF InfHandle
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PLOADED_INF CurInf, NextInf;

    try {
        //
        // Make sure we can lock the head of the INF list before
        // we start deleting!
        //
        if(LockInf((PLOADED_INF)InfHandle)) {
            //
            // Also check INF Signature field as a further validation.
            //
            if(((PLOADED_INF)InfHandle)->Signature == LOADED_INF_SIG) {

                CurInf = ((PLOADED_INF)InfHandle)->Next;

                DestroySynchronizedAccess(&(((PLOADED_INF)InfHandle)->Lock));
                FreeLoadedInfDescriptor((PLOADED_INF)InfHandle);

                for(; CurInf; CurInf = NextInf) {
                    NextInf = CurInf->Next;
                    FreeInfFile(CurInf);
                }

            } else {
                UnlockInf((PLOADED_INF)InfHandle);
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}


/////////////////////////////////////////////////////////////////
//
// Internal routines
//
/////////////////////////////////////////////////////////////////

BOOL
pSetupVersionNodeFromInfInformation(
    IN  PSP_INF_INFORMATION InfInformation,
    IN  UINT                InfIndex,
    OUT PINF_VERSION_NODE   VersionNode,
    OUT PTSTR               OriginalFilename OPTIONAL
    )

/*++

Routine Description:

    Fills in a caller-supplied INF_VERSION_NODE buffer for an INF file
    from the SP_INF_INFORMATION structure.

Arguments:

    InfInformation - supplies the inf information descriptor

    InfIndex - supplies the 0-based index of the inf whose version block
        is requested. If this value is not inrange an error is returned.

    VersionNode - supplies the address of a buffer that receives the version
        node structure.

    OriginalFilename - optionally, supplies the address of a character buffer
        (that must be at least MAX_PATH characters large) that receives the
        INF's original filename (which may be the same as its current filename
        if the INF isn't an OEM INF.

Return Value:

    If successful, the return value is TRUE, otherwise, it is FALSE.

--*/

{
    PINF_VERSION_BLOCK First;
    INF_VERSION_BLOCK UNALIGNED *Ver;
    PUCHAR Base;
    UINT ord;
    INF_VERSION_BLOCK TempVersionBlock;
    UINT FilenameSize;

    //
    // Get pointer to first version block.
    //
    Base = (PUCHAR)InfInformation;
    First = (PINF_VERSION_BLOCK)(Base+offsetof(SP_INF_INFORMATION,VersionData));

    //
    // Find relevant version block
    //
    ord = 0;
    for(Ver=First; Ver; Ver=(INF_VERSION_BLOCK UNALIGNED *)(Base+Ver->NextOffset)) {

        if(ord++ == InfIndex) {
            break;
        }
    }

    if(!Ver) {
        return FALSE;
    }

    //
    // Now fill in the version node based on the information contained in the version block.
    //
    VersionNode->LastWriteTime = Ver->LastWriteTime;
    VersionNode->DataBlock     = (CONST TCHAR *)((PBYTE)(Ver->Filename) + Ver->OffsetToData);
    VersionNode->DataSize      = Ver->DataSize;
    VersionNode->DatumCount    = Ver->DatumCount;

    //
    // The 'filename' character buffer may actually contain two strings--the
    // first being the INF's current filename (with path), and the second being
    // the INF's original filename (this won't be present if the INF's name
    // hasn't changed from its original name).
    //
    // Copy the first MAX_PATH characters of this buffer (or the entire buffer,
    // whichever is smaller) into the VersionNode's Filename buffer, then after
    // we've computed the string length of that string, we can ascertain whether
    // or not there's another string following it containing the INF's original
    // name.
    //
    FilenameSize = (Ver->OffsetToData < SIZECHARS(VersionNode->Filename))
                 ? Ver->OffsetToData : SIZECHARS(VersionNode->Filename);

    CopyMemory(VersionNode->Filename, Ver->Filename, FilenameSize);
    VersionNode->FilenameSize = (lstrlen(VersionNode->Filename) + 1) * sizeof(TCHAR);

    MYASSERT(Ver->OffsetToData >= VersionNode->FilenameSize);

    if(OriginalFilename) {

        if(Ver->OffsetToData > VersionNode->FilenameSize) {
            //
            // Then there's more data in the Filename buffer, namely the INF's
            // original name--fill this filename into the caller-supplied buffer.
            //
            FilenameSize = Ver->OffsetToData - VersionNode->FilenameSize;
            MYASSERT(((UINT)(FilenameSize / sizeof(TCHAR)) * sizeof(TCHAR)) == FilenameSize);
            MYASSERT(FilenameSize > sizeof(TCHAR));

            CopyMemory(OriginalFilename,
                       (PBYTE)Ver->Filename + VersionNode->FilenameSize,
                       FilenameSize
                      );

            MYASSERT(((lstrlen(OriginalFilename) + 1) * sizeof(TCHAR)) == FilenameSize);

        } else {
            //
            // No original name info stored--must be same as current name.
            //
            lstrcpy(OriginalFilename, MyGetFileTitle(VersionNode->Filename));
        }
    }

    return TRUE;
}


PCTSTR
pSetupGetVersionDatum(
    IN PINF_VERSION_NODE VersionNode,
    IN PCTSTR            DatumName
    )

/*++

Routine Description:

    Look up a piece of version data in an version data node.

Arguments:

    VersionNode - supplies a pointer to the version node to
        be searched for the datum.

    DatumName - supplies the name of the datum to be retreived.

Return Value:

    NULL if the datum does not exist in the data block.
    Otherwise a pointer to the datum value is returned. The caller
        must not free or write into this memory.

--*/

{
    WORD Datum;
    UINT StringLength;
    PCTSTR Data = VersionNode->DataBlock;

    for(Datum=0; Datum < VersionNode->DatumCount; Datum++) {

        StringLength = lstrlen(Data) + 1;

        //
        // Go through the version block looking for a matching datum name.
        //
        if(lstrcmpi(Data, DatumName)) {

            //
            // Point to the next one.
            //
            Data += StringLength;
            Data += lstrlen(Data) + 1;

        } else {

            //
            // Found it. Return datum value to caller.
            //
            return (Data + StringLength);
        }
    }

    return(NULL);
}


BOOL
pSetupGetCatalogFileValue(
    IN  PINF_VERSION_NODE    InfVersionNode,
    OUT LPTSTR               Buffer,
    IN  DWORD                BufferSize,
    IN  PSP_ALTPLATFORM_INFO AltPlatformInfo OPTIONAL
    )

/*++

Routine Description:

    This routine fetches the (potentially decorated) CatalogFile= value from the
    specified inf version section.

Arguments:

    InfVersionNode - points to the INF version node from which we're attempting
        to retrieve the associated catalog file.

    Buffer - if the routine returns TRUE, receives the value for CatalogFile=
        in the [Version] section of the inf.

    BufferSize - supplies the size in bytes (ansi) or chars (unicode) of
        the buffer pointed to by Buffer.

    AltPlatformInfo - optionally, supplies the address of a structure describing
        the platform parameters that should be used in formulating the decorated
        CatalogFile= entry to be used when searching for the INF's associated
        catalog file.

Return Value:

    Boolean value indicating whether a value was found and copied to the
    caller-supplied Buffer.

--*/

{
    TCHAR CatFileWithExt[64];
    LPCTSTR p, NtPlatformSuffixToUse;
    DWORD PlatformId;

    MYASSERT(BufferSize >= MAX_PATH);

    p = NULL;

    CopyMemory(CatFileWithExt, pszCatalogFile, sizeof(pszCatalogFile) - sizeof(TCHAR));

    //
    // Set up some variables based on the native platform or upon the non-native
    // platform specified in the AltPlatformInfo parameter.
    //
    if(AltPlatformInfo) {
        PlatformId = AltPlatformInfo->Platform;
        switch(AltPlatformInfo->ProcessorArchitecture) {

            case PROCESSOR_ARCHITECTURE_INTEL:
                NtPlatformSuffixToUse = pszNtX86Suffix;
                break;

            case PROCESSOR_ARCHITECTURE_ALPHA:
                NtPlatformSuffixToUse = pszNtAlphaSuffix;
                break;

            case PROCESSOR_ARCHITECTURE_IA64:
                NtPlatformSuffixToUse = pszNtIA64Suffix;
                break;

            case PROCESSOR_ARCHITECTURE_ALPHA64:
                NtPlatformSuffixToUse = pszNtAXP64Suffix;
                break;

            default:
                return FALSE;
        }
    } else {
        PlatformId = OSVersionInfo.dwPlatformId;
        NtPlatformSuffixToUse = pszNtPlatformSuffix;
    }

    if(PlatformId == VER_PLATFORM_WIN32_NT) {
        //
        // We're running on NT, so first try the NT architecture-specific
        // extension, then the generic NT extension.
        //
        lstrcpyn((PTSTR)((PBYTE)CatFileWithExt + (sizeof(pszCatalogFile) - sizeof(TCHAR))),
                 NtPlatformSuffixToUse,
                 SIZECHARS(CatFileWithExt) - (sizeof(pszCatalogFile) - sizeof(TCHAR))
                );

        p = pSetupGetVersionDatum(InfVersionNode, CatFileWithExt);

        if(!p) {
            //
            // We didn't find an NT architecture-specific CatalogFile= entry, so
            // fall back to looking for just an NT-specific one.
            //
            CopyMemory((PBYTE)CatFileWithExt + (sizeof(pszCatalogFile) - sizeof(TCHAR)),
                       pszNtSuffix,
                       sizeof(pszNtSuffix)
                      );

            p = pSetupGetVersionDatum(InfVersionNode, CatFileWithExt);
        }

    } else {
        //
        // We're running on Windows 95, so try the Windows-specific extension
        //
        CopyMemory((PBYTE)CatFileWithExt + (sizeof(pszCatalogFile) - sizeof(TCHAR)),
                   pszWinSuffix,
                   sizeof(pszWinSuffix)
                  );

        p = pSetupGetVersionDatum(InfVersionNode, CatFileWithExt);
    }

    //
    // If we didn't find an OS/architecture-specific CatalogFile= entry above,
    // then look for an undecorated entry.
    //
    if(!p) {
        p = pSetupGetVersionDatum(InfVersionNode, pszCatalogFile);
    }

    //
    // If we got back an empty string, then treat this as if there was no
    // CatalogFile= entry (this might be used, for example, so that a system-
    // supplied INF that supports both NT and Win98 could specify an undecorated
    // CatalogFile= entry for Win98, yet supply an NT-specific CatalogFile=
    // entry that's an empty string, so that we'd do global verification on NT).
    //
    if(p && lstrlen(p)) {
        lstrcpyn(Buffer, p, BufferSize);
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
pSetupGetPhysicalInfFilepath(
    IN  PINFCONTEXT LineContext,
    OUT LPTSTR      Buffer,
    IN  DWORD       BufferSize
    )
{
    lstrcpyn(
        Buffer,
        ((PLOADED_INF)LineContext->CurrentInf)->VersionBlock.Filename,
        BufferSize
        );
}
