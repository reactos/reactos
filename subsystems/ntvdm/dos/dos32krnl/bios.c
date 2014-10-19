/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/bios.c
 * PURPOSE:         DOS32 Bios
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "int32.h"

#include "dos.h"

#include "bios/bios.h"

/* PRIVATE VARIABLES **********************************************************/

// static BYTE CurrentDrive;
// static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

CHAR DosReadCharacter(WORD FileHandle)
{
    CHAR Character = '\0';
    WORD BytesRead;

    DPRINT("DosReadCharacter\n");

    /* Use the file reading function */
    DosReadFile(FileHandle, &Character, 1, &BytesRead);

    return Character;
}

BOOLEAN DosCheckInput(VOID)
{
    HANDLE Handle = DosGetRealHandle(DOS_INPUT_HANDLE);

    if (IsConsoleHandle(Handle))
    {
        /* Save AX */
        USHORT AX = getAX();

        /* Call the BIOS */
        setAH(0x01); // or 0x11 for enhanced, but what to choose?
        Int32Call(&DosContext, BIOS_KBD_INTERRUPT);

        /* Restore AX */
        setAX(AX);

        /* Return keyboard status */
        return (getZF() == 0);
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

VOID DosPrintCharacter(WORD FileHandle, CHAR Character)
{
    WORD BytesWritten;

    /* Use the file writing function */
    DosWriteFile(FileHandle, &Character, 1, &BytesWritten);
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
    RtlZeroMemory(CurrentDirectories, sizeof(CurrentDirectories));

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


    /* Register the DOS 32-bit Interrupts */
    // RegisterDosInt32(0x20, DosInt20h);

    /* Initialize the DOS kernel */
    return DosKRNLInitialize();
}

/* EOF */
