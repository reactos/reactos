/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dem.c
 * PURPOSE:         DOS 32-bit Emulation Support Library -
 *                  This library is used by the built-in NTVDM DOS32 and by
 *                  the NT 16-bit DOS in Windows (via BOPs). It also exposes
 *                  exported functions that can be used by VDDs.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "dem.h"

/* Extra PSDK/NDK Headers */
#include <ndk/obtypes.h>

/* PRIVATE VARIABLES **********************************************************/

#pragma pack(push, 1)

typedef struct _VDM_FIND_FILE_BLOCK
{
    CHAR DriveLetter;
    CHAR Pattern[11];
    UCHAR AttribMask;
    DWORD Unused;
    HANDLE SearchHandle;

    /* The following part of the structure is documented */
    UCHAR Attributes;
    WORD FileTime;
    WORD FileDate;
    DWORD FileSize;
    CHAR FileName[13];
} VDM_FIND_FILE_BLOCK, *PVDM_FIND_FILE_BLOCK;

#pragma pack(pop)

extern BYTE CurrentDrive;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/* PUBLIC EXPORTED APIS *******************************************************/

// demLFNCleanup
// demLFNGetCurrentDirectory

// demGetFileTimeByHandle_WOW
// demWOWLFNAllocateSearchHandle
// demWOWLFNCloseSearchHandle
// demWOWLFNEntry
// demWOWLFNGetSearchHandle
// demWOWLFNInit

DWORD
WINAPI
demClientErrorEx(IN HANDLE FileHandle,
                 IN CHAR   Unknown,
                 IN BOOL   Flag)
{
    UNIMPLEMENTED;
    return GetLastError();
}

DWORD
WINAPI
demFileDelete(IN LPCSTR FileName)
{
    if (DeleteFileA(FileName)) SetLastError(ERROR_SUCCESS);

    return GetLastError();
}

DWORD
WINAPI
demFileFindFirst(OUT PVOID  lpFindFileData,
                 IN  LPCSTR FileName,
                 IN  WORD   AttribMask)
{
    BOOLEAN Success = TRUE;
    WIN32_FIND_DATAA FindData;
    PVDM_FIND_FILE_BLOCK FindFileBlock = (PVDM_FIND_FILE_BLOCK)lpFindFileData;

    /* Fill the block */
    FindFileBlock->DriveLetter  = CurrentDrive + 'A';
    FindFileBlock->AttribMask   = AttribMask;
    FindFileBlock->SearchHandle = FindFirstFileA(FileName, &FindData);
    if (FindFileBlock->SearchHandle == INVALID_HANDLE_VALUE) return GetLastError();

    do
    {
        /* Check the attributes */
        if (!((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                            FILE_ATTRIBUTE_SYSTEM |
                                            FILE_ATTRIBUTE_DIRECTORY))
            & ~AttribMask))
        {
            break;
        }
    }
    while ((Success = FindNextFileA(FindFileBlock->SearchHandle, &FindData)));

    if (!Success) return GetLastError();

    FindFileBlock->Attributes = LOBYTE(FindData.dwFileAttributes);
    FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                          &FindFileBlock->FileDate,
                          &FindFileBlock->FileTime);
    FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                     : FindData.nFileSizeLow;
    strcpy(FindFileBlock->FileName, FindData.cAlternateFileName);

    return ERROR_SUCCESS;
}

DWORD
WINAPI
demFileFindNext(OUT PVOID lpFindFileData)
{
    WIN32_FIND_DATAA FindData;
    PVDM_FIND_FILE_BLOCK FindFileBlock = (PVDM_FIND_FILE_BLOCK)lpFindFileData;

    do
    {
        if (!FindNextFileA(FindFileBlock->SearchHandle, &FindData))
            return GetLastError();

        /* Update the block */
        FindFileBlock->Attributes = LOBYTE(FindData.dwFileAttributes);
        FileTimeToDosDateTime(&FindData.ftLastWriteTime,
                              &FindFileBlock->FileDate,
                              &FindFileBlock->FileTime);
        FindFileBlock->FileSize = FindData.nFileSizeHigh ? 0xFFFFFFFF
                                                         : FindData.nFileSizeLow;
        strcpy(FindFileBlock->FileName, FindData.cAlternateFileName);
    }
    while((FindData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
                                        FILE_ATTRIBUTE_SYSTEM |
                                        FILE_ATTRIBUTE_DIRECTORY))
          & ~FindFileBlock->AttribMask);

    return ERROR_SUCCESS;
}

UCHAR
WINAPI
demGetPhysicalDriveType(IN UCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return DOSDEVICE_DRIVE_UNKNOWN;
}

BOOL
WINAPI
demIsShortPathName(IN LPCSTR Path,
                   IN BOOL Unknown)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
WINAPI
demSetCurrentDirectoryGetDrive(IN  LPCSTR CurrentDirectory,
                               OUT PUCHAR DriveNumber)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

/* EOF */
