/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            utils.c
 * PURPOSE:         Utility Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
FileClose(IN HANDLE FileHandle)
{
    CloseHandle(FileHandle);
}

HANDLE
FileOpen(IN  PCSTR  FileName,
         OUT PULONG FileSize OPTIONAL)
{
    HANDLE hFile;
    ULONG  ulFileSize;

    /* Open the file */
    SetLastError(0); // For debugging purposes
    hFile = CreateFileA(FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    DPRINT1("File '%s' opening %s ; GetLastError() = %u\n",
            FileName, hFile != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

    /* If we failed, bail out */
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    /* OK, we have a handle to the file */

    /*
     * Retrieve the size of the file. In NTVDM we will handle files
     * of maximum 1Mb so we can largely use GetFileSize only.
     */
    ulFileSize = GetFileSize(hFile, NULL);
    if (ulFileSize == INVALID_FILE_SIZE && GetLastError() != ERROR_SUCCESS)
    {
        /* We failed, bail out */
        DPRINT1("Error when retrieving file size, or size too large (%d)\n", ulFileSize);
        FileClose(hFile);
        return NULL;
    }

    /* Success, return file handle and size if needed */
    if (FileSize) *FileSize = ulFileSize;
    return hFile;
}

BOOLEAN
FileLoadByHandle(IN  HANDLE FileHandle,
                 IN  PVOID  Location,
                 IN  ULONG  FileSize,
                 OUT PULONG BytesRead)
{
    BOOLEAN Success;

    /* Attempt to load the file into memory */
    SetLastError(0); // For debugging purposes
    Success = !!ReadFile(FileHandle,
                         Location, // REAL_TO_PHYS(LocationRealPtr),
                         FileSize,
                         BytesRead,
                         NULL);
    DPRINT1("File loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    return Success;
}

/* EOF */
