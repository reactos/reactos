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

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_LOAD_DOS    0x2B    // DOS Loading and Initializing BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_START_DOS   0x2C    // DOS Starting BOP. In parameter (following bytes) we take a NULL-terminated string indicating the name of the DOS kernel file.
#define BOP_DOS         0x50    // DOS System BOP (for NTIO.SYS and NTDOS.SYS)
#define BOP_CMD         0x54    // DOS Command Interpreter BOP (for COMMAND.COM)

/* VARIABLES ******************************************************************/

/* FUNCTIONS ******************************************************************/

VOID BiosCharPrint(CHAR Character);
#define BiosDisplayMessage(Format, ...) \
    PrintMessageAnsi(BiosCharPrint, (Format), ##__VA_ARGS__)

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

DWORD
WINAPI
demFileFindFirst
(
    OUT PVOID  lpFindFileData,
    IN  LPCSTR FileName,
    IN  WORD   AttribMask
);

DWORD
WINAPI
demFileFindNext
(
    OUT PVOID lpFindFileData
);

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

#endif /* _DEM_H_ */
