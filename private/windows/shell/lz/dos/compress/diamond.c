/*++

Module Name:

    diamond.c

Abstract:

    Diamond compression interface.

    This module contains functions to compress a file using
    the mszip compression library.

Author:

    Ted Miller

Environment:

    Windows

--*/


#include <windows.h>
#include <stdio.h>

#include "..\..\libs\common.h"
#include "..\..\libs\buffers.h"
#include "..\..\libs\header.h"

#include <io.h>
#include <fcntl.h>
#include <errno.h>

#include "..\main.h"
#include <diamondc.h>
#include "mydiam.h"


typedef struct _DIAMOND_INFO {
    DWORD SourceFileSize;
    DWORD CompressedSize;
    FILETIME SourceFileCreationTime;
    FILETIME SourceFileModifiedTime;
    FILETIME SourceFileAccessedTime;
} DIAMOND_INFO, *PDIAMOND_INFO;

//
// Callback functions to perform memory allocation, io, etc.
// We pass addresses of these functions to diamond.
//
int
DIAMONDAPI
fciFilePlacedCB(
    OUT PCCAB Cabinet,
    IN  PSTR  FileName,
    IN  LONG  FileSize,
    IN  BOOL  Continuation,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to indicate that a file has been
    comitted to a cabinet.

    No action is taken and success is returned.

Arguments:

    Cabinet - cabinet structure to fill in.

    FileName - name of file in cabinet

    FileSize - size of file in cabinet

    Continuation - TRUE if this is a partial file, continuation
        of compression begun in a different cabinet.

    Context - supplies context information.

Return Value:

    0 (success).

--*/

{
    UNREFERENCED_PARAMETER(Cabinet);
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(FileSize);
    UNREFERENCED_PARAMETER(Continuation);
    UNREFERENCED_PARAMETER(Context);

    return(0);
}



PVOID
DIAMONDAPI
fciAllocCB(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by diamond to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return((PVOID)LocalAlloc(LMEM_FIXED,NumberOfBytes));
}


VOID
DIAMONDAPI
fciFreeCB(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by diamond to free a memory block.
    The block must have been allocated with fciAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    LocalFree((HLOCAL)Block);
}



FNFCIGETTEMPFILE(fciTempFileCB)
{
    CHAR TempPath[MAX_PATH];
    DWORD cchTemp;

    cchTemp = GetTempPath(sizeof(TempPath), TempPath);
    if ((cchTemp == 0) || (cchTemp >= sizeof(TempPath))) {
        TempPath[0] = '.';
        TempPath[1] = '\0';
    }

    if(GetTempFileNameA(TempPath,"dc",0,pszTempName)) {
        DeleteFileA(pszTempName);
    }

    return(TRUE);
}


BOOL
DIAMONDAPI
fciNextCabinetCB(
    OUT PCCAB Cabinet,
    IN  DWORD CabinetSizeEstimate,
    IN  PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to request a new cabinet file.
    This functionality is not used in our implementation as
    we deal only with single-file cabinets.

Arguments:

    Cabinet - cabinet structure to be filled in.

    CabinetSizeEstimate - estimated size of cabinet.

    Context - supplies context information.

Return Value:

    FALSE (failure).

--*/

{
    UNREFERENCED_PARAMETER(Cabinet);
    UNREFERENCED_PARAMETER(CabinetSizeEstimate);
    UNREFERENCED_PARAMETER(Context);

    return(FALSE);
}


BOOL
DIAMONDAPI
fciStatusCB(
    IN UINT  StatusType,
    IN DWORD Count1,
    IN DWORD Count2,
    IN PVOID Context
    )

/*++

Routine Description:

    Callback used by diamond to give status on file compression
    and cabinet operations, etc.

Arguments:

    Status Type - supplies status type.

        0 = statusFile   - compressing block into a folder.
                              Count1 = compressed size
                              Count2 = uncompressed size

        1 = statusFolder - performing AddFilder.
                              Count1 = bytes done
                              Count2 = total bytes

    Context - supplies context info.

Return Value:

    TRUE (success).

--*/

{
    PDIAMOND_INFO context;

    UNREFERENCED_PARAMETER(Count2);

    context = (PDIAMOND_INFO)Context;

    if(StatusType == statusFile) {

        //
        // Track compressed size.
        //
        context->CompressedSize += Count1;
    }

    return(TRUE);
}



FNFCIGETOPENINFO(fciOpenInfoCB)
{
    int h;
    WIN32_FIND_DATAA FindData;
    HANDLE FindHandle;
    PDIAMOND_INFO context;
    FILETIME ftLocal;

    context = pv;

    FindHandle = FindFirstFileA(pszName,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        return(-1);
    }
    FindClose(FindHandle);

    context->SourceFileSize = FindData.nFileSizeLow;
    context->SourceFileCreationTime = FindData.ftCreationTime;
    context->SourceFileModifiedTime = FindData.ftLastWriteTime;
    context->SourceFileAccessedTime = FindData.ftLastAccessTime;

    FileTimeToLocalFileTime(&FindData.ftLastWriteTime, &ftLocal);
    FileTimeToDosDateTime(&ftLocal, pdate, ptime);
    *pattribs = (WORD)(FindData.dwFileAttributes &
            ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
              FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE ));

    h = _open(pszName,_O_RDONLY | _O_BINARY);
    if(h == -1) {
        return(-1);
    }

    return(h);
}


FNFCIOPEN(fciOpen)
{
    int result;

    result = _open(pszFile, oflag, pmode);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCIREAD(fciRead)
{
    UINT result;

    result = (UINT) _read((HFILE)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCIWRITE(fciWrite)
{
    UINT result;

    result = (UINT) _write((HFILE)hf, memory, cb);

    if (result != cb) {
        *err = errno;
    }

    return(result);
}

FNFCICLOSE(fciClose)
{
    int result;

    result = _close((HFILE)hf);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}

FNFCISEEK(fciSeek)
{
    long result;

    result = _lseek((HFILE)hf, dist, seektype);

    if (result == -1) {
        *err = errno;
    }

    return(result);

}

FNFCIDELETE(fciDelete)
{
    int result;

    result = _unlink(pszFile);

    if (result == -1) {
        *err = errno;
    }

    return(result);
}


INT
DiamondCompressFile(
    IN  NOTIFYPROC CompressNotify,
    IN  PSTR       SourceFile,
    IN  PSTR       TargetFile,
    IN  BOOL       Rename,
    OUT PLZINFO    pLZI
    )
{
    BOOL b;
    PSTR SourceFilenamePart,p;
    HFCI FciContext;
    ERF  FciError;
    CCAB ccab;
    CHAR targetFile[MAX_PATH];
    DIAMOND_INFO Context;
    INT Status;

    //
    // Isolate the filename part of the source file.
    //
    if(SourceFilenamePart = strrchr(SourceFile,'\\')) {
        SourceFilenamePart++;
    } else {
        SourceFilenamePart = SourceFile;
    }

    //
    // Form the actual name of the target file.
    //
    lstrcpy(targetFile,TargetFile);
    if(Rename) {
        MakeCompressedName(targetFile);
    }

    //
    // Fill in the cabinet structure.
    //
    ZeroMemory(&ccab,sizeof(ccab));

    lstrcpyA(ccab.szCabPath,targetFile);
    if(p=strrchr(ccab.szCabPath,'\\')) {
        lstrcpyA(ccab.szCab,++p);
        *p = 0;
    } else {
        lstrcpyA(ccab.szCab,targetFile);
        ccab.szCabPath[0] = 0;
    }

    //
    // Call the notification function to see whether we are really
    // supposed to compress this file.
    //
    if(!CompressNotify(SourceFile,targetFile,NOTIFY_START_COMPRESS)) {
        return(BLANK_ERROR);
    }

    ZeroMemory(&Context,sizeof(Context));

    //
    // Compress the file.
    //
    FciContext = FCICreate(
                    &FciError,
                    fciFilePlacedCB,
                    fciAllocCB,
                    fciFreeCB,
                    fciOpen,
                    fciRead,
                    fciWrite,
                    fciClose,
                    fciSeek,
                    fciDelete,
                    fciTempFileCB,
                    &ccab,
                    &Context
                    );

    if(FciContext) {

        b = FCIAddFile(
                FciContext,
                SourceFile,         // file to add to cabinet.
                SourceFilenamePart, // filename part, name to store in cabinet.
                FALSE,
                fciNextCabinetCB,   // routine for next cabinet (always fails)
                fciStatusCB,
                fciOpenInfoCB,
                DiamondCompressionType
                );

        if(b) {

            b = FCIFlushCabinet(
                    FciContext,
                    FALSE,
                    fciNextCabinetCB,
                    fciStatusCB
                    );

            if(b) {

                HANDLE FindHandle;
                WIN32_FIND_DATA FindData;

                //
                // Context.CompressedSize does not include headers
                // and any other file overhead.
                //
                FindHandle = FindFirstFile(targetFile,&FindData);
                if(FindHandle == INVALID_HANDLE_VALUE) {
                    pLZI->cblOutSize = (LONG)Context.CompressedSize;
                } else {
                    pLZI->cblOutSize = (LONG)FindData.nFileSizeLow;
                    FindClose(FindHandle);
                }

                pLZI->cblInSize = (LONG)Context.SourceFileSize;

                FindHandle = CreateFile(targetFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
                if (FindHandle != INVALID_HANDLE_VALUE)
                {
                    SetFileTime(FindHandle,
                        &Context.SourceFileCreationTime,
                        &Context.SourceFileAccessedTime,
                        &Context.SourceFileModifiedTime);

                    CloseHandle(FindHandle);
                }
            }
        }

        if(b) {
            Status = TRUE;
        } else {

            switch(FciError.erfOper) {

            case FCIERR_OPEN_SRC:
                Status = LZERROR_BADINHANDLE;
                break;

            case FCIERR_READ_SRC:
                Status = LZERROR_READ;
                break;

            case FCIERR_CAB_FILE:
                Status = LZERROR_WRITE;
                break;

            case FCIERR_ALLOC_FAIL:
                Status = LZERROR_GLOBALLOC;
                break;

            case FCIERR_TEMP_FILE:
            case FCIERR_BAD_COMPR_TYPE:
            case FCIERR_USER_ABORT:
            case FCIERR_MCI_FAIL:
            default:
                Status = FALSE;
            }
        }

        FCIDestroy(FciContext);
    } else {
        Status = LZERROR_GLOBALLOC;
    }

    return(Status);
}
