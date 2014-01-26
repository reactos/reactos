/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos.c
 * PURPOSE:         VDM DOS Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "dos.h"

#include "bios/bios.h"
#include "bop.h"
// #include "int32.h"

/* PRIVATE VARIABLES **********************************************************/

// static BYTE CurrentDrive;
// static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];

/* BOP Identifiers */
#define BOP_DOS 0x50    // DOS System BOP (for NTIO.SYS and NTDOS.SYS)
#define BOP_CMD 0x54    // DOS Command Interpreter BOP (for COMMAND.COM)

/* PRIVATE FUNCTIONS **********************************************************/

#if 0
static WORD DosWriteFile(WORD FileHandle, LPVOID Buffer, WORD Count, LPWORD BytesWritten)
{
    WORD Result = ERROR_SUCCESS;
    DWORD BytesWritten32 = 0;
    HANDLE Handle = DosGetRealHandle(FileHandle);
    WORD i;

    DPRINT("DosWriteFile: FileHandle 0x%04X, Count 0x%04X\n",
           FileHandle,
           Count);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    if (IsConsoleHandle(Handle))
    {
        for (i = 0; i < Count; i++)
        {
            /* Call the BIOS to print the character */
            VidBiosPrintCharacter(((LPBYTE)Buffer)[i], DOS_CHAR_ATTRIBUTE, Bda->VideoPage);
            BytesWritten32++;
        }
    }
    else
    {
        /* Write the file */
        if (!WriteFile(Handle, Buffer, Count, &BytesWritten32, NULL))
        {
            /* Store the error code */
            Result = (WORD)GetLastError();
        }
    }

    /* The number of bytes written is always 16-bit */
    *BytesWritten = LOWORD(BytesWritten32);

    /* Return the error code */
    return Result;
}
#endif

/* PUBLIC FUNCTIONS ***********************************************************/

CHAR DosReadCharacter(VOID)
{
    CHAR Character = '\0';
    WORD BytesRead;

    if (IsConsoleHandle(DosGetRealHandle(DOS_INPUT_HANDLE)))
    {
        /* Call the BIOS */
        Character = LOBYTE(BiosGetCharacter());
    }
    else
    {
        /* Use the file reading function */
        DosReadFile(DOS_INPUT_HANDLE, &Character, sizeof(CHAR), &BytesRead);
    }

    return Character;
}

BOOLEAN DosCheckInput(VOID)
{
    HANDLE Handle = DosGetRealHandle(DOS_INPUT_HANDLE);

    if (IsConsoleHandle(Handle))
    {
        /* Call the BIOS */
        return (BiosPeekCharacter() != 0xFFFF);
    }
    else
    {
        DWORD FileSizeHigh;
        DWORD FileSize = GetFileSize(Handle, &FileSizeHigh);
        LONG LocationHigh = 0;
        DWORD Location = SetFilePointer(Handle, 0, &LocationHigh, FILE_CURRENT);

        return ((Location != FileSize) || (LocationHigh != FileSizeHigh));
    }
}

VOID DosPrintCharacter(CHAR Character)
{
    WORD BytesWritten;

    /* Use the file writing function */
    DosWriteFile(DOS_OUTPUT_HANDLE, &Character, sizeof(CHAR), &BytesWritten);
}

VOID WINAPI DosSystemBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    DPRINT1("Unknown DOS System BOP Function: 0x%02X\n", FuncNum);
}

VOID WINAPI DosCmdInterpreterBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        case 0x08: // Launch external command
        {
#define CMDLINE_LENGTH  1024

            BOOL Result;
            DWORD dwExitCode;

            LPSTR Command = (LPSTR)SEG_OFF_TO_PTR(getDS(), getSI());
            CHAR CommandLine[CMDLINE_LENGTH] = "";
            STARTUPINFOA StartupInfo;
            PROCESS_INFORMATION ProcessInformation;
            DPRINT1("CMD Run Command '%s'\n", Command);

            Command[strlen(Command)-1] = 0;
            
            strcpy(CommandLine, "cmd.exe /c ");
            strcat(CommandLine, Command);

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

            StartupInfo.cb = sizeof(StartupInfo);

            DosPrintCharacter('\n');

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
                DPRINT1("Command '%s' launched successfully\n");

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
            
            DosPrintCharacter('\n');

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

BOOLEAN DosBIOSInitialize(VOID)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);

    LPWSTR SourcePtr, Environment;
    LPSTR AsciiString;
    DWORD AsciiSize;
    LPSTR DestPtr = (LPSTR)SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0);

#if 0
    UCHAR i;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];
    LPSTR Path;

    FILE *Stream;
    WCHAR Buffer[256];
#endif

    /* Initialize the MCB */
    Mcb->BlockType = 'Z';
    Mcb->Size = USER_MEMORY_SIZE;
    Mcb->OwnerPsp = 0;

    /* Initialize the link MCB to the UMB area */
    Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT + USER_MEMORY_SIZE + 1);
    Mcb->BlockType = 'M';
    Mcb->Size = UMB_START_SEGMENT - FIRST_MCB_SEGMENT - USER_MEMORY_SIZE - 2;
    Mcb->OwnerPsp = SYSTEM_PSP;

    /* Initialize the UMB area */
    Mcb = SEGMENT_TO_MCB(UMB_START_SEGMENT);
    Mcb->BlockType = 'Z';
    Mcb->Size = UMB_END_SEGMENT - UMB_START_SEGMENT;
    Mcb->OwnerPsp = 0;

    /* Get the environment strings */
    SourcePtr = Environment = GetEnvironmentStringsW();
    if (Environment == NULL) return FALSE;

    /* Fill the DOS system environment block */
    while (*SourcePtr)
    {
        /* Get the size of the ASCII string */
        AsciiSize = WideCharToMultiByte(CP_ACP,
                                        0,
                                        SourcePtr,
                                        -1,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);

        /* Allocate memory for the ASCII string */
        AsciiString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AsciiSize);
        if (AsciiString == NULL)
        {
            FreeEnvironmentStringsW(Environment);
            return FALSE;
        }

        /* Convert to ASCII */
        WideCharToMultiByte(CP_ACP,
                            0,
                            SourcePtr,
                            -1,
                            AsciiString,
                            AsciiSize,
                            NULL,
                            NULL);

        /* Copy the string into DOS memory */
        strcpy(DestPtr, AsciiString);

        /* Move to the next string */
        SourcePtr += wcslen(SourcePtr) + 1;
        DestPtr += strlen(AsciiString);
        *(DestPtr++) = 0;

        /* Free the memory */
        HeapFree(GetProcessHeap(), 0, AsciiString);
    }
    *DestPtr = 0;

    /* Free the memory allocated for environment strings */
    FreeEnvironmentStringsW(Environment);


#if 0

    /* Clear the current directory buffer */
    ZeroMemory(CurrentDirectories, sizeof(CurrentDirectories));

    /* Get the current directory */
    if (!GetCurrentDirectoryA(MAX_PATH, CurrentDirectory))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Convert that to a DOS path */
    if (!GetShortPathNameA(CurrentDirectory, DosDirectory, DOS_DIR_LENGTH))
    {
        // TODO: Use some kind of default path?
        return FALSE;
    }

    /* Set the drive */
    CurrentDrive = DosDirectory[0] - 'A';

    /* Get the directory part of the path */
    Path = strchr(DosDirectory, '\\');
    if (Path != NULL)
    {
        /* Skip the backslash */
        Path++;
    }

    /* Set the directory */
    if (Path != NULL)
    {
        strncpy(CurrentDirectories[CurrentDrive], Path, DOS_DIR_LENGTH);
    }

    /* Read CONFIG.SYS */
    Stream = _wfopen(DOS_CONFIG_PATH, L"r");
    if (Stream != NULL)
    {
        while (fgetws(Buffer, sizeof(Buffer)/sizeof(Buffer[0]), Stream))
        {
            // TODO: Parse the line
        }
        fclose(Stream);
    }

#endif


    /* Register the DOS BOPs */
    RegisterBop(BOP_DOS, DosSystemBop        );
    RegisterBop(BOP_CMD, DosCmdInterpreterBop);

    /* Register the DOS 32-bit Interrupts */
    // RegisterInt32(0x20, DosInt20h);

    /* TODO: Initialize the DOS kernel */

    return TRUE;
}

/* EOF */
