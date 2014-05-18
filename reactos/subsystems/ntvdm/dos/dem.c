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

#include "emulator.h"
#include "utils.h"

#include "dem.h"
#include "bop.h"

#include "bios/bios.h"

/* Extra PSDK/NDK Headers */
#include <ndk/obtypes.h>

/* PRIVATE VARIABLES **********************************************************/

/**/extern BYTE CurrentDrive;/**/

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_DOS 0x50    // DOS System BOP (for NTIO.SYS and NTDOS.SYS)
#define BOP_CMD 0x54    // DOS Command Interpreter BOP (for COMMAND.COM)

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI DosSystemBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case 0x11:  // Load the DOS kernel
        {
            BOOLEAN Success;
            HANDLE  hDosKernel;
            ULONG   ulDosKernelSize = 0;

            DPRINT1("You are loading Windows NT DOS!\n");

            /* Open the DOS kernel file */
            hDosKernel = FileOpen("ntdos.sys", &ulDosKernelSize);

            /* If we failed, bail out */
            if (hDosKernel == NULL) goto Quit;

            /*
             * Attempt to load the DOS kernel into memory.
             * The segment where to load the DOS kernel is defined
             * by the DOS BIOS and is found in DI:0000 .
             */
            Success = FileLoadByHandle(hDosKernel,
                                       REAL_TO_PHYS(TO_LINEAR(getDI(), 0x0000)),
                                       ulDosKernelSize,
                                       &ulDosKernelSize);

            DPRINT1("Windows NT DOS loading %s at 0x%04X:0x%04X, size 0x%x ; GetLastError() = %u\n",
                    (Success ? "succeeded" : "failed"),
                    getDI(), 0x0000,
                    ulDosKernelSize,
                    GetLastError());

            /* Close the DOS kernel file */
            FileClose(hDosKernel);

Quit:
            if (!Success)
            {
                /* We failed everything, stop the VDM */
                EmulatorTerminate();
            }

            break;
        }

        default:
        {

            DPRINT1("Unknown DOS System BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}

static VOID WINAPI DosCmdInterpreterBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case 0x08:  // Launch external command
        {
#define CMDLINE_LENGTH  1024

            BOOL Result;
            DWORD dwExitCode;

            LPSTR Command = (LPSTR)SEG_OFF_TO_PTR(getDS(), getSI());
            LPSTR CmdPtr  = Command;
            CHAR CommandLine[CMDLINE_LENGTH] = "";
            STARTUPINFOA StartupInfo;
            PROCESS_INFORMATION ProcessInformation;

            /* NULL-terminate the command line by removing the return carriage character */
            while (*CmdPtr && *CmdPtr != '\r') CmdPtr++;
            *CmdPtr = '\0';

            DPRINT1("CMD Run Command '%s'\n", Command);

            /* Spawn a user-defined 32-bit command preprocessor */

            /* Build the command line */
            // FIXME: Use COMSPEC env var!!
            strcpy(CommandLine, "cmd.exe /c ");
            strcat(CommandLine, Command);

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

            StartupInfo.cb = sizeof(StartupInfo);

            VidBiosDetachFromConsole();

            Result = CreateProcessA(NULL,
                                    CommandLine,
                                    NULL,
                                    NULL,
                                    TRUE,
                                    0,
                                    NULL,
                                    NULL,
                                    &StartupInfo,
                                    &ProcessInformation);
            if (Result)
            {
                DPRINT1("Command '%s' launched successfully\n", Command);

                /* Wait for process termination */
                WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

                /* Get the exit code */
                GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);

                /* Close handles */
                CloseHandle(ProcessInformation.hThread);
                CloseHandle(ProcessInformation.hProcess);
            }
            else
            {
                DPRINT1("Failed when launched command '%s'\n");
                dwExitCode = GetLastError();
            }

            VidBiosAttachToConsole();

            setAL((UCHAR)dwExitCode);

            break;
        }

        default:
        {
            DPRINT1("Unknown DOS CMD Interpreter BOP Function: 0x%02X\n", FuncNum);
            // setCF(1); // Disable, otherwise we enter an infinite loop
            break;
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN DosInitialize(IN LPCSTR DosKernelFileName)
{
    /* Register the DOS BOPs */
    RegisterBop(BOP_DOS, DosSystemBop        );
    RegisterBop(BOP_CMD, DosCmdInterpreterBop);

    if (DosKernelFileName)
    {
        BOOLEAN Success;
        HANDLE  hDosBios;
        ULONG   ulDosBiosSize = 0;

        /* Open the DOS BIOS file */
        hDosBios = FileOpen(DosKernelFileName, &ulDosBiosSize);

        /* If we failed, bail out */
        if (hDosBios == NULL) return FALSE;

        /* Attempt to load the DOS BIOS into memory */
        Success = FileLoadByHandle(hDosBios,
                                   REAL_TO_PHYS(TO_LINEAR(0x0070, 0x0000)),
                                   ulDosBiosSize,
                                   &ulDosBiosSize);

        DPRINT1("DOS BIOS loading %s at 0x%04X:0x%04X, size 0x%x ; GetLastError() = %u\n",
                (Success ? "succeeded" : "failed"),
                0x0070, 0x0000,
                ulDosBiosSize,
                GetLastError());

        /* Close the DOS BIOS file */
        FileClose(hDosBios);

        if (Success)
        {
            /* Position execution pointers and return */
            setCS(0x0070);
            setIP(0x0000);
        }

        return Success;
    }
    else
    {
        BOOLEAN Result;

        Result  = DosBIOSInitialize();
        // Result &= DosKRNLInitialize();

        return Result;
    }
}

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
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

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
    PDOS_FIND_FILE_BLOCK FindFileBlock = (PDOS_FIND_FILE_BLOCK)lpFindFileData;

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
