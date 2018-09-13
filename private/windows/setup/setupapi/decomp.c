/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    decomp.c

Abstract:

    File decompression support routines.

Author:

    Ted Miller (tedm) 1-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <pshpack1.h>
struct LZINFO;
typedef struct LZINFO *PLZINFO;
#include "..\..\shell\lz\libs\header.h"
#include <poppack.h>


typedef struct _SFD_INFO {
    unsigned FileCount;
    PCTSTR TargetFile;
    BOOL GotTimestamp;
    FILETIME FileTime;
} SFD_INFO, *PSFD_INFO;


UINT
pGetCompressInfoCB(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    );

UINT
pSingleFileDecompCB(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    );

PTSTR
SetupGenerateCompressedName(
    IN PCTSTR Filename
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

    Look backwards for a dot.  If there is no dot, append "._" to the name.
    If there is a dot followed by 0, 1, or 2 charcaters, append "_".
    Otherwise there is a 3-character or greater extension and we replace
    the last character with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

Return Value:

    Pointer to buffer containing nul-terminated compressed-form filename.
    The caller must free this buffer via MyFree().

--*/

{
    PTSTR CompressedName,p,q;
    UINT u;

    //
    // The maximum length of the compressed filename is the length of the
    // original name plus 2 (for ._).
    //
    if(CompressedName = MyMalloc((lstrlen(Filename)+3)*sizeof(TCHAR))) {

        lstrcpy(CompressedName,Filename);

        p = _tcsrchr(CompressedName,TEXT('.'));
        q = _tcsrchr(CompressedName,TEXT('\\'));
        if(q < p) {

            //
            // If there are 0, 1, or 2 characters after the dot, just append
            // the underscore.  p points to the dot so include that in the length.
            //
            u = lstrlen(p);
            if(u < 4) {
                lstrcat(CompressedName,TEXT("_"));
            } else {
                //
                // There are at least 3 characters in the extension.
                // Replace the final one with an underscore.
                //
                p[u-1] = TEXT('_');
            }
        } else {
            //
            // No dot, just add ._.
            //
            lstrcat(CompressedName,TEXT("._"));
        }
    }

    return(CompressedName);
}


DWORD
pSetupAttemptLocate(
    IN  PCTSTR           FileName,
    OUT PBOOL            Found,
    OUT PWIN32_FIND_DATA FindData
    )

/*++

Routine Description:

    Attempt to locate a source file via FindFirstFile().

    Errors of the 'file not found' type are not considered errors
    and result in NO_ERROR. Any non-NO_ERROR return indicates that
    we could not determine whether the file is present or not
    because of some hardware or system problem, etc.

Arguments:

    FileName - supplies filename of the file to be located.

    Found - receives a value indicating whether the file was found.
        This value is only valid when the function returns NO_ERROR.

    FindData - if found, returns win32 find data for the file.

Return Value:

    Win32 error code indicating the outcome. If NO_ERROR, check
    the Found return value to see whether the file was found.

--*/

{
    DWORD d;

    if(*Found = FileExists(FileName,FindData)) {
        d = NO_ERROR;
    } else {
        //
        // We didn't find the file. See whether that was because
        // the file wasn't there or because some other error occured.
        //
        d = GetLastError();

        if((d == ERROR_NO_MORE_FILES)
        || (d == ERROR_FILE_NOT_FOUND)
        || (d == ERROR_PATH_NOT_FOUND)
        || (d == ERROR_BAD_NETPATH))
        {
            d = NO_ERROR;
        }
    }

    return(d);
}


DWORD
SetupDetermineSourceFileName(
    IN  PCTSTR            FileName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated,
    OUT PWIN32_FIND_DATA  FindData
    )

/*++

Routine Description:

    Attempt to locate a source file whose name can be compressed
    or uncompressed.

    The order of attempt is

    - the name as given (should be the uncompressed name)
    - the compressed form, using _ as the compression char
    - the compressed form, using $ as the compression char

Arguments:

    FileName - supplies filename of the file to be located.

    UsedCompressedName - receives a boolean indicating whether
        the filename we located seems to indicate that the file
        is compressed.

    FileNameLocated - receives a pointer to the filename actually
        located. The caller must free with MyFree().

    FindData - if found, returns win32 find data for the file.

Return Value:

    Win32 error code indicating the outcome.

    ERROR_FILE_NOT_FOUND - normal code indicating everything is ok
        but we can't find the file

    NO_ERROR - file was located; check UsedCompressedName and FileNameOpened.

    Others - something is wrong with the hardware or system.

--*/

{
    DWORD d;
    PTSTR TryName;
    BOOL Found;


    TryName = DuplicateString(FileName);
    if(!TryName) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *UsedCompressedName = FALSE;
    *FileNameLocated = TryName;

    d = pSetupAttemptLocate(TryName,&Found,FindData);
    if(d != NO_ERROR) {
        MyFree(TryName);
        *FileNameLocated = NULL;
        return(d);
    }

    if(Found) {
        return(NO_ERROR);
    }

    MyFree(TryName);
    *UsedCompressedName = TRUE;
    *FileNameLocated = NULL;

    TryName = SetupGenerateCompressedName(FileName);
    if(!TryName) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *FileNameLocated = TryName;

    d = pSetupAttemptLocate(TryName,&Found,FindData);
    if(d != NO_ERROR) {
        MyFree(TryName);
        *FileNameLocated = NULL;
        return(d);
    }

    if(Found) {
        return(NO_ERROR);
    }

    MYASSERT(TryName[lstrlen(TryName)-1] == TEXT('_'));
    TryName[lstrlen(TryName)-1] = TEXT('$');

    d = pSetupAttemptLocate(TryName,&Found,FindData);

    if((d != NO_ERROR) || !Found) {
        *FileNameLocated = NULL;
        MyFree(TryName);
    }

    return(Found ? NO_ERROR : ERROR_FILE_NOT_FOUND);
}

BOOL
pSetupDoesFileMatch(
    IN  PCTSTR            InputName,
    IN  PCTSTR            CompareName,
    OUT PBOOL             UsedCompressedName,
    OUT PTSTR            *FileNameLocated
    )

/*++

Routine Description:

    determine if the specified input file matches the
    name to compare it with.  We try the undecorated name
    as well as the compressed versions of the file name.

    The order of attempt is

    - the name as given (should be the uncompressed name)
    - the compressed form, using _ as the compression char
    - the compressed form, using $ as the compression char

Arguments:

    FileName - supplies filename we're looking at.

    CompareName -supplies the filename we're comparing against

    UsedCompressedName - receives a boolean indicating whether
        the filename we located seems to indicate that the file
        is compressed.

    FileNameLocated - receives a pointer to the filename actually
        located. The caller must free with MyFree().


Return Value:

    Win32 error code indicating the outcome.

    ERROR_FILE_NOT_FOUND - normal code indicating everything is ok
        but we can't find the file

    NO_ERROR - file was located; check UsedCompressedName and FileNameOpened.

    Others - something is wrong with the hardware or system.

--*/

{
    DWORD d;
    PTSTR TryName,TargetName,src,dst;
    BOOL Found;


    TryName = DuplicateString(InputName);
    if(!TryName) {
        return(FALSE);
    }

    TargetName = DuplicateString(CompareName);
    if(!TargetName) {
        MyFree(TryName);
        return(FALSE);
    }

    dst = _tcsrchr(TryName,TEXT('.'));
    if (dst) {
        *dst = 0;
    }
    src = _tcsrchr(TargetName,TEXT('.'));
    if (src) {
        *src = 0;
    }

    if (lstrcmpi(TargetName,TryName)) {
        // the "surnames" do not match, so none of the other comparisons will work.
        MyFree(TryName);
        MyFree(TargetName);
        return(FALSE);
    }

    if (dst) {
        *dst = TEXT('.');
    }

    if (src) {
        *src = TEXT('.');
    }

    *UsedCompressedName = FALSE;
    *FileNameLocated = TryName;

    if (!lstrcmpi(TryName,TargetName)) {
        // we matched
        MyFree(TargetName);
        return(TRUE);
    }

    MyFree(TryName);
    *UsedCompressedName = TRUE;

    TryName = SetupGenerateCompressedName(TargetName);
    if(!TryName) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    *FileNameLocated = TryName;

    if (!lstrcmpi(TryName,InputName)) {
        // we matched
        MyFree(TargetName);
        return(TRUE);
    }

    MYASSERT(TryName[lstrlen(TryName)-1] == TEXT('_'));
    TryName[lstrlen(TryName)-1] = TEXT('$');

    if (!lstrcmpi(TryName,InputName)) {
        // we matched
        MyFree(TargetName);
        return(TRUE);
    }

    //
    // no match
    //
    MyFree(TargetName);
    MyFree(TryName);

    return(FALSE);
}



DWORD
pSetupDecompressWinLzFile(
    IN PTSTR SourceFileName,
    IN PTSTR TargetFileName
    )

/*++

Routine Description:

    Determine whether a file is compressed, and retreive additional
    information about it.

Arguments:

    SourceFileName - supplies filename of the file to be checked.
        This filename is used as a base; if not found then we look
        for the 2 compressed forms (ie, foo.ex_, foo.ex$) as well.

    ActualSourceFileName - receives a pointer to the filename
        that was actually located. Caller can free with MyFree().
        Valid only if the return code from this routine is NO_ERROR.

    SourceFileSize - receives the size of the located file in its
        current (ie, compressed) form. Valid only if this routine
        returns NO_ERROR.

    TargetFileSize - receives the uncompressed size of the file.
        If the file is not compressed this will be the same as
        SourceFileSize. Valid only if this routine returns NO_ERROR.

    CompressionType - receives a value indicating the compression type.
        Valid only if this routine returns NO_ERROR.

Return Value:

    Win32 error code indicating the outcome.

    ERROR_FILE_NOT_FOUND - normal code indicating everything is ok
        but we can't find the file

    NO_ERROR - file was located and output params are filled in.

    Others - something is wrong with the hardware or system.

--*/

{
    INT hSrc,hDst;
    OFSTRUCT ofSrc,ofDst;
    LONG l;
    DWORD d;
    FILETIME CreateTime,AccessTime,WriteTime;

    //
    // Get the timestamp of the source.
    //
    d = GetSetFileTimestamp(
            SourceFileName,
            &CreateTime,
            &AccessTime,
            &WriteTime,
            FALSE
            );

    if(d != NO_ERROR) {
        return(d);
    }

    hSrc = LZOpenFile(SourceFileName,&ofSrc,OF_READ|OF_SHARE_DENY_WRITE);
    if(hSrc >= 0) {

        hDst = LZOpenFile(TargetFileName,&ofSrc,OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE);
        if(hDst >= 0) {

            l = LZCopy(hSrc,hDst);
            if(l >= 0) {
                l = 0;

                //
                // Set the timestamp of the target. The file is already there
                // so just ignore errors.
                //
                GetSetFileTimestamp(
                    TargetFileName,
                    &CreateTime,
                    &AccessTime,
                    &WriteTime,
                    TRUE
                    );
            }

            LZClose(hDst);

        } else {
            l = hDst;
        }

        LZClose(hSrc);

    } else {
        l = hSrc;
    }

    //
    // lz error to win32 error
    //
    switch(l) {

    case 0:
        return(NO_ERROR);

    case LZERROR_BADINHANDLE:
    case LZERROR_READ:
        return(ERROR_READ_FAULT);

    case LZERROR_BADOUTHANDLE:
    case LZERROR_WRITE:
        return(ERROR_WRITE_FAULT);

    case LZERROR_GLOBALLOC:
    case LZERROR_GLOBLOCK:
        return(ERROR_NOT_ENOUGH_MEMORY);

    case LZERROR_BADVALUE:
    case LZERROR_UNKNOWNALG:
        return(ERROR_INVALID_DATA);

    default:
        return(ERROR_INVALID_FUNCTION);
    }
}


DWORD
SetupInternalGetFileCompressionInfo(
    IN  PCTSTR            SourceFileName,
    OUT PTSTR            *ActualSourceFileName,
    OUT PWIN32_FIND_DATA  SourceFindData,
    OUT PDWORD            TargetFileSize,
    OUT PUINT             CompressionType
    )

/*++

Routine Description:

    Determine whether a file is compressed, and retreive additional
    information about it.

Arguments:

    SourceFileName - supplies filename of the file to be checked.
        This filename is used as a base; if not found then we look
        for the 2 compressed forms (ie, foo.ex_, foo.ex$) as well.

    ActualSourceFileName - receives a pointer to the filename
        that was actually located. Caller can free with MyFree().
        Valid only if the return code from this routine is NO_ERROR.

    SourceFindData - receives win32 find data for the located file in its
        current (ie, compressed) form. Valid only if this routine
        returns NO_ERROR.

    TargetFileSize - receives the uncompressed size of the file.
        If the file is not compressed this will be the same as
        SourceFileSize. Valid only if this routine returns NO_ERROR.

    CompressionType - receives a value indicating the compression type.
        Valid only if this routine returns NO_ERROR.

Return Value:

    Win32 error code indicating the outcome.

    ERROR_FILE_NOT_FOUND - normal code indicating everything is ok
        but we can't find the file

    NO_ERROR - file was located and output params are filled in.

    Others - something is wrong with the hardware or system.

--*/

{
    DWORD d;
    BOOL b;
    HANDLE hFile,hMapping;
    DWORD size;
    FH UNALIGNED *LZHeader;

    d = SetupDetermineSourceFileName(
            SourceFileName,
            &b,
            ActualSourceFileName,
            SourceFindData
            );

    if(d != NO_ERROR) {
        return(d);
    }

    //
    // If the file is 0-length it isn't compressed;
    // trying to map it in below will fail in this case.
    //
    if(SourceFindData->nFileSizeLow) {

        //
        // See if it's a diamond file.
        //
        d = DiamondProcessCabinet(
                *ActualSourceFileName,
                0,
                pGetCompressInfoCB,
                &size,
                TRUE
                );

        if(d == NO_ERROR) {

            *TargetFileSize = size;
            *CompressionType = FILE_COMPRESSION_MSZIP;
            return(NO_ERROR);
        }

        //
        // See if it's a WINLZ file.
        //
        d = OpenAndMapFileForRead(
                *ActualSourceFileName,
                &SourceFindData->nFileSizeLow,
                &hFile,
                &hMapping,
                (PVOID *)&LZHeader
                );

        if(d != NO_ERROR) {
            MyFree(*ActualSourceFileName);
            return(d);
        }

        b = FALSE;
        try {
            if((SourceFindData->nFileSizeLow >= HEADER_LEN)
            && !memcmp(LZHeader->rgbyteMagic,COMP_SIG,COMP_SIG_LEN)
            && RecognizeCompAlg(LZHeader->byteAlgorithm))
            {
                *TargetFileSize = LZHeader->cbulUncompSize;
                b = TRUE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ;
        }

        UnmapAndCloseFile(hFile,hMapping,LZHeader);

        if(b) {
            *CompressionType = FILE_COMPRESSION_WINLZA;
            return(NO_ERROR);
        }
    }

    //
    // File is not compressed.
    //
    *CompressionType = FILE_COMPRESSION_NONE;
    *TargetFileSize = SourceFindData->nFileSizeLow;
    return(NO_ERROR);
}


UINT
pGetCompressInfoCB(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    )
{
    PFILE_IN_CABINET_INFO FileInfo;
    DWORD rc;

    switch(Notification) {

    case SPFILENOTIFY_CABINETINFO:
        //
        // We don't do anything with this.
        //
        rc = NO_ERROR;
        break;

    case SPFILENOTIFY_FILEINCABINET:
        //
        // New file within a cabinet.
        //
        // We don't ever want to copy the file. Save size info
        // and abort.
        //
        FileInfo = (PFILE_IN_CABINET_INFO)Param1;

        *((PDWORD)Context) = FileInfo->FileSize;

        FileInfo->Win32Error = NO_ERROR;
        rc = FILEOP_ABORT;
        break;

    //case SPFILENOTIFY_FILEEXTRACTED:
    //case SPFILENOTIFY_NEEDNEWCABINET:
    default:
        //
        // We should never get these.
        //
        MYASSERT(0);
        rc = ERROR_INVALID_FUNCTION;
        break;
    }

    return(rc);
}


#ifdef UNICODE
//
// ANSI version
//
DWORD
SetupGetFileCompressionInfoA(
    IN  PCSTR   SourceFileName,
    OUT PSTR   *ActualSourceFileName,
    OUT PDWORD  SourceFileSize,
    OUT PDWORD  TargetFileSize,
    OUT PUINT   CompressionType
    )
{
    WIN32_FIND_DATA FindData;
    DWORD d;
    PCWSTR source;
    PWSTR actualsource = NULL;
    PSTR actualsourceansi = NULL;
    PSTR la_actualsourceansi = NULL;
    DWORD targetsize;
    UINT type;

    d = CaptureAndConvertAnsiArg(SourceFileName,&source);
    if(d != NO_ERROR) {
        return(d);
    }

    d = SetupInternalGetFileCompressionInfo(source,&actualsource,&FindData,&targetsize,&type);

    if(d == NO_ERROR) {

        MYASSERT(actualsource);

        if((actualsourceansi = UnicodeToAnsi(actualsource))==NULL) {
            d = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
        if((la_actualsourceansi = (PSTR)LocalAlloc(0,1+strlen(actualsourceansi)))==NULL) {
            d = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
        strcpy(la_actualsourceansi,actualsourceansi);
        try {
            *SourceFileSize = FindData.nFileSizeLow;
            *ActualSourceFileName = la_actualsourceansi; // free using LocalFree
            *TargetFileSize = targetsize;
            *CompressionType = type;
            la_actualsourceansi = NULL;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
    }

clean0:
    if(actualsource) {
        MyFree(actualsource);
    }
    if(actualsourceansi) {
        MyFree(actualsourceansi);
    }
    if(la_actualsourceansi) {
        LocalFree(la_actualsourceansi);
    }

    MyFree(source);

    return(d);
}
#else
//
// Unicode stub
//
DWORD
SetupGetFileCompressionInfoW(
    IN  PCWSTR  SourceFileName,
    OUT PWSTR  *ActualSourceFileName,
    OUT PDWORD  SourceFileSize,
    OUT PDWORD  TargetFileSize,
    OUT PUINT   CompressionType
    )
{
    UNREFERENCED_PARAMETER(SourceFileName);
    UNREFERENCED_PARAMETER(ActualSourceFileName);
    UNREFERENCED_PARAMETER(SourceFileSize);
    UNREFERENCED_PARAMETER(TargetFileSize);
    UNREFERENCED_PARAMETER(CompressionType);
    return(ERROR_CALL_NOT_IMPLEMENTED);
}
#endif

DWORD
SetupGetFileCompressionInfo(
    IN  PCTSTR  SourceFileName,
    OUT PTSTR  *ActualSourceFileName,
    OUT PDWORD  SourceFileSize,
    OUT PDWORD  TargetFileSize,
    OUT PUINT   CompressionType
    )

/*++

Routine Description:

    Determine whether a file is compressed, and retreive additional
    information about it.

    ***** This function should *NOT* be used internally   *****
    ***** Use SetupInternalGetFileCompressionInfo instead *****

Arguments:

    SourceFileName - supplies filename of the file to be checked.
        This filename is used as a base; if not found then we look
        for the 2 compressed forms (ie, foo.ex_, foo.ex$) as well.

    ActualSourceFileName - receives a pointer to the filename
        that was actually located. Caller can free with LocalFree().
        (note that this was changed to correspond to docs)
        Valid only if the return code from this routine is NO_ERROR.

    SourceFileSize - receives the size of the located file in its
        current (ie, compressed) form. Valid only if this routine
        returns NO_ERROR.

    TargetFileSize - receives the uncompressed size of the file.
        If the file is not compressed this will be the same as
        SourceFileSize. Valid only if this routine returns NO_ERROR.

    CompressionType - receives a value indicating the compression type.
        Valid only if this routine returns NO_ERROR.

Return Value:

    Win32 error code indicating the outcome.

    ERROR_FILE_NOT_FOUND - normal code indicating everything is ok
        but we can't find the file

    NO_ERROR - file was located and output params are filled in.

    Others - something is wrong with the hardware or system.

--*/

{
    WIN32_FIND_DATA FindData;
    DWORD d;
    PCTSTR source;
    PTSTR actualsource = NULL;
    PTSTR la_actualsource = NULL;
    DWORD targetsize;
    UINT type;

    d = CaptureStringArg(SourceFileName,&source);
    if(d != NO_ERROR) {
        return(d);
    }

    d = SetupInternalGetFileCompressionInfo(source,&actualsource,&FindData,&targetsize,&type);

    if(d == NO_ERROR) {
        MYASSERT(actualsource);
        la_actualsource = (PTSTR)LocalAlloc(0,sizeof(TCHAR)*(1+lstrlen(actualsource)));
        if (la_actualsource == NULL) {
            MyFree(actualsource);
            MyFree(source);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        lstrcpy(la_actualsource,actualsource);
        try {
            *SourceFileSize = FindData.nFileSizeLow;
            *ActualSourceFileName = la_actualsource; // free using LocalFree
            *TargetFileSize = targetsize;
            *CompressionType = type;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            d = ERROR_INVALID_PARAMETER;
        }
        if(d != NO_ERROR) {
            LocalFree(la_actualsource);
        }
        MyFree(actualsource);
    }

    MyFree(source);

    return(d);
}


#ifdef UNICODE
//
// ANSI version
//
DWORD
SetupDecompressOrCopyFileA(
    IN  PCSTR   SourceFileName,
    OUT PCSTR   TargetFileName,
    OUT PUINT   CompressionType OPTIONAL
    )
{
    DWORD rc;
    PCWSTR s,t;

    rc = CaptureAndConvertAnsiArg(SourceFileName,&s);
    if(rc == NO_ERROR) {

        rc = CaptureAndConvertAnsiArg(TargetFileName,&t);
        if(rc == NO_ERROR) {

            rc = pSetupDecompressOrCopyFile(s,t,CompressionType,FALSE,NULL);
            MyFree(t);
        }

        MyFree(s);
    }

    return(rc);
}
#else
//
// Unicode stub
//
DWORD
SetupDecompressOrCopyFileW(
    IN  PCWSTR  SourceFileName,
    OUT PCWSTR  TargetFileName,
    OUT PUINT   CompressionType OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(SourceFileName);
    UNREFERENCED_PARAMETER(TargetFileName);
    UNREFERENCED_PARAMETER(CompressionType);
    return(ERROR_CALL_NOT_IMPLEMENTED);
}
#endif

DWORD
SetupDecompressOrCopyFile(
    IN PCTSTR SourceFileName,
    IN PCTSTR TargetFileName,
    IN PUINT  CompressionType OPTIONAL
    )

/*++

Routine Description:

    Decompress or copy a file.

Arguments:

    SourceFileName - supplies filename of the file to be decompressed.
        If CompressionType is specified, no additional processing is
        performed on this name -- the caller is responsible for determining
        the actual file name (ie, foo.ex_ instead of foo.exe) before calling
        this routine. If CompressionType is not specified, then this routine
        attempts to locate the compressed form of the filename if the file
        is not found with the name given.

    TargetFileName - supplies filename of target file.

    CompressionType - if specified, supplies type of compression in use
        on the source. This can be determined by calling
        SetupGetFileCompressionInfo(). Specifying FILE_COMPRESSION_NONE
        results in the file being copied and not decompressed,
        regardless of the type of compression that may be in use on the source.
        If this value is not specified then this routine attempts to determine
        the compression type and decompresses/copies accordingly.

Return Value:

    Win32 error code indicating the outcome.

--*/

{
    DWORD rc;
    PCTSTR s,t;

    rc = CaptureStringArg(SourceFileName,&s);
    if(rc == NO_ERROR) {

        rc = CaptureStringArg(TargetFileName,&t);
        if(rc == NO_ERROR) {

            rc = pSetupDecompressOrCopyFile(s,t,CompressionType,FALSE,NULL);
            MyFree(t);
        }

        MyFree(s);
    }

    return(rc);
}


DWORD
pSetupDecompressOrCopyFile(
    IN  PCTSTR SourceFileName,
    IN  PCTSTR TargetFileName,
    IN  PUINT  CompressionType, OPTIONAL
    IN  BOOL   AllowMove,
    OUT PBOOL  Moved            OPTIONAL
    )

/*++

Routine Description:

    Decompress or copy a file.

Arguments:

    SourceFileName - supplies filename of the file to be decompressed.
        If CompressionType is specified, no additional processing is
        performed on this name -- the caller is responsible for determining
        the actual file name (ie, foo.ex_ instead of foo.exe) before calling
        this routine. If CompressionType is not specified, then this routine
        attempts to locate the compressed form of the filename if the file
        is not found with the name given.

    TargetFileName - supplies filename of target file.

    CompressionType - if specified, supplies type of compression in use
        on the source. This can be determined by calling
        SetupGetFileCompressionInfo(). Specifying FILE_COMPRESSION_NONE
        results in the file being copied and not decompressed,
        regardless of the type of compression that may be in use on the source.
        If this value is not specified then this routine attempts to determine
        the compression type and decompresses/copies accordingly.

    AllowMove - if specified, then files that do not require decompression
        will be moved instead of copied.

    Moved - if specified receives a boolean indicating whether the file was
        moved (as opposed to copied or decompressed).

Return Value:

    Win32 error code indicating the outcome.

--*/

{
    DWORD d;
    UINT ComprType;
    PTSTR ActualName;
    DWORD TargetSize;
    FILETIME CreateTime,AccessTime,WriteTime;
    SFD_INFO CBData;
    BOOL moved;
    WIN32_FIND_DATA FindData;

    if(Moved) {
        *Moved = FALSE;
    }

    if(CompressionType) {
        ComprType = *CompressionType;
        ActualName = (PTSTR)SourceFileName;
    } else {
        //
        // Need to determine compresison type.
        //
        d = SetupInternalGetFileCompressionInfo(
                SourceFileName,
                &ActualName,
                &FindData,
                &TargetSize,
                &ComprType
                );

        if(d != NO_ERROR) {
            return(d);
        }
    }

    //
    // Blast the target file. Ignore if failure -- it'll be caught later.
    //
    SetFileAttributes(TargetFileName,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(TargetFileName);

    switch(ComprType) {

    case FILE_COMPRESSION_NONE:
        moved = (AllowMove ? MoveFile(ActualName,TargetFileName) : FALSE);
        if(moved) {
            d = NO_ERROR;
            if(Moved) {
                *Moved = TRUE;
            }
        } else {
            d = GetSetFileTimestamp(ActualName,&CreateTime,&AccessTime,&WriteTime,FALSE);
            if(d == NO_ERROR) {
                d = CopyFile(ActualName,TargetFileName,FALSE) ? NO_ERROR : GetLastError();
                if(d == NO_ERROR) {
                    GetSetFileTimestamp(TargetFileName,&CreateTime,&AccessTime,&WriteTime,TRUE);
                }
            }
        }
        break;

    case FILE_COMPRESSION_WINLZA:
        d = pSetupDecompressWinLzFile(ActualName,(PTSTR)TargetFileName);
        break;

    case FILE_COMPRESSION_MSZIP:

        CBData.FileCount = 0;
        CBData.TargetFile = TargetFileName;
        CBData.GotTimestamp = FALSE;

        d = DiamondProcessCabinet(
                ActualName,
                0,
                pSingleFileDecompCB,
                &CBData,
                TRUE
                );
        break;

    default:
        d = ERROR_INVALID_PARAMETER;
        break;
    }

    if(!CompressionType) {
        MyFree(ActualName);
    }

    return(d);
}


UINT
pSingleFileDecompCB(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    )
{
    PSFD_INFO Data;
    PFILE_IN_CABINET_INFO FileInfo;
    PFILEPATHS FilePaths;
    DWORD rc;
    HANDLE h;

    Data = Context;

    switch(Notification) {

    case SPFILENOTIFY_CABINETINFO:
        //
        // We don't do anything with this.
        //
        rc = NO_ERROR;
        break;

    case SPFILENOTIFY_FILEINCABINET:
        //
        // New file within a cabinet.
        //
        FileInfo = (PFILE_IN_CABINET_INFO)Param1;
        FileInfo->Win32Error = NO_ERROR;

        //
        // We only want the first file. If this is a subsequent file,
        // bail out.
        //
        if(Data->FileCount++) {

            rc = FILEOP_ABORT;

        } else {
            //
            // We want the file. Ignore the names in the cabinet and
            // use the name given to us. Also, we want to preserve
            // the timestamp of the cabinet, not of the file within it.
            //
            lstrcpyn(FileInfo->FullTargetName,Data->TargetFile,MAX_PATH);

            h = CreateFile(
                    (PCTSTR)Param2,         // cabinet filename
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

            if(h != INVALID_HANDLE_VALUE) {
                if(GetFileTime(h,NULL,NULL,&Data->FileTime)) {
                    Data->GotTimestamp = TRUE;
                }
                CloseHandle(h);
            }

            rc = FILEOP_DOIT;
        }

        break;

    case SPFILENOTIFY_FILEEXTRACTED:
        //
        // File was successfully extracted.
        // Preserve timestamp.
        //
        FilePaths = (PFILEPATHS)Param1;

        if(Data->GotTimestamp) {

            h = CreateFile(
                    FilePaths->Target,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

            if(h != INVALID_HANDLE_VALUE) {
                SetFileTime(h,NULL,NULL,&Data->FileTime);
                CloseHandle(h);
            }
        }

        rc = NO_ERROR;
        break;

    //case SPFILENOTIFY_NEEDNEWCABINET:
    default:
        //
        // We should never get this.
        //
        MYASSERT(0);
        rc = ERROR_INVALID_FUNCTION;
        break;
    }

    return(rc);
}



