/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dem.h
 * PURPOSE:         DOS 32-bit Emulation Support Library -
 *                  This library is used by the built-in NTVDM DOS32 and by
 *                  the NT 16-bit DOS in Windows (via BOPs). It also exposes
 *                  exported functions that can be used by VDDs.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _DEM_H_
#define _DEM_H_

/* INCLUDES *******************************************************************/

#include <crt/dos.h> // For _A_NORMAL etc.
#include "dos32krnl/dos.h"

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_LOAD_DOS    0x2B    // DOS Loading and Initializing BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_START_DOS   0x2C    // DOS Starting BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_DOS         0x50    // DOS System BOP (for NTIO.SYS and NTDOS.SYS)
#define BOP_CMD         0x54    // DOS Command Interpreter BOP (for COMMAND.COM)

/* VARIABLES ******************************************************************/

/* FUNCTIONS ******************************************************************/

VOID Dem_BiosCharPrint(CHAR Character);
#define BiosDisplayMessage(Format, ...) \
    PrintMessageAnsi(Dem_BiosCharPrint, (Format), ##__VA_ARGS__)

VOID DosCharPrint(CHAR Character);
#define DosDisplayMessage(Format, ...)  \
    PrintMessageAnsi(DosCharPrint, (Format), ##__VA_ARGS__)


BOOLEAN DosShutdown(BOOLEAN Immediate);

DWORD DosStartProcess32(IN LPCSTR ExecutablePath,
                        IN LPCSTR CommandLine,
                        IN LPCSTR Environment OPTIONAL,
                        IN DWORD ReturnAddress OPTIONAL,
                        IN BOOLEAN StartComSpec);

DWORD
WINAPI
demClientErrorEx
(
    IN HANDLE FileHandle,
    IN CHAR   Unknown,
    IN BOOL   Flag
);

DWORD
WINAPI
demFileDelete
(
    IN LPCSTR FileName
);

/**
 * @brief   File attributes for demFileFindFirst().
 **/
#define FA_NORMAL       _A_NORMAL // 0x0000
#define FA_READONLY     _A_RDONLY // 0x0001 // FILE_ATTRIBUTE_READONLY
#define FA_HIDDEN       _A_HIDDEN // 0x0002 // FILE_ATTRIBUTE_HIDDEN
#define FA_SYSTEM       _A_SYSTEM // 0x0004 // FILE_ATTRIBUTE_SYSTEM
#define FA_VOLID        _A_VOLID  // 0x0008
#define FA_LABEL        FA_VOLID
#define FA_DIRECTORY    _A_SUBDIR // 0x0010 // FILE_ATTRIBUTE_DIRECTORY
#define FA_ARCHIVE      _A_ARCH   // 0x0020 // FILE_ATTRIBUTE_ARCHIVE
#define FA_DEVICE       0x0040              // FILE_ATTRIBUTE_DEVICE

#define FA_VALID    (FA_ARCHIVE | FA_DIRECTORY | FA_SYSTEM | FA_HIDDEN | FA_READONLY | FA_NORMAL)

/** @brief  Convert Win32/NT file attributes to DOS format. */
#define NT_TO_DOS_FA(Attrs) \
    ( ((Attrs) == FILE_ATTRIBUTE_NORMAL) ? FA_NORMAL : (LOBYTE(Attrs) & FA_VALID) )

DWORD
WINAPI
demFileFindFirst(
    _Out_ PVOID pFindFileData,
    _In_  PCSTR FileName,
    _In_  WORD  AttribMask);

DWORD
WINAPI
demFileFindNext(
    _Inout_ PVOID pFindFileData);

UCHAR
WINAPI
demGetPhysicalDriveType
(
    IN UCHAR DriveNumber
);

BOOL
WINAPI
demIsShortPathName
(
    IN LPCSTR Path,
    IN BOOL Unknown
);

DWORD
WINAPI
demSetCurrentDirectoryGetDrive
(
    IN  LPCSTR CurrentDirectory,
    OUT PUCHAR DriveNumber
);

#endif // _DEM_H_

/* EOF */
