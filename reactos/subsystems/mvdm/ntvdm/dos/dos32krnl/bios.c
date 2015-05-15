/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/bios.c
 * PURPOSE:         DOS32 Bios
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "int32.h"

#include "dos.h"
#include "dosfiles.h"
#include "memory.h"
#include "bios/bios.h"

// This is needed because on UNICODE this symbol is redirected to
// GetEnvironmentStringsW whereas on ANSI it corresponds to the real
// "ANSI" function (and GetEnvironmentStringsA is aliased to it).
#undef GetEnvironmentStrings

// Symmetrize the dumbness of the previous symbol: on UNICODE
// FreeEnvironmentStrings aliases to FreeEnvironmentStringsW but
// on "ANSI" FreeEnvironmentStrings aliases to FreeEnvironmentStringsA
#undef FreeEnvironmentStrings
#define FreeEnvironmentStrings FreeEnvironmentStringsA

#define CHARACTER_ADDRESS 0x007000FF /* 0070:00FF */

/* PRIVATE VARIABLES **********************************************************/

// static BYTE CurrentDrive;
// static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

CHAR DosReadCharacter(WORD FileHandle)
{
    WORD BytesRead;

    Sda->ByteBuffer = '\0';
    DPRINT("DosReadCharacter\n");

    /* Use the file reading function */
    DosReadFile(FileHandle,
                MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer), DOS_DATA_SEGMENT),
                1,
                &BytesRead);

    return Sda->ByteBuffer;
}

BOOLEAN DosCheckInput(VOID)
{
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(DOS_INPUT_HANDLE);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        Sda->LastErrorCode = ERROR_INVALID_HANDLE; // ERROR_FILE_NOT_FOUND
        return FALSE;
    }

    if (Descriptor->DeviceInfo & (1 << 7))
    {
        WORD Result;
        PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);

        if (!Node->InputStatusRoutine) return FALSE;

        Result = Node->InputStatusRoutine(Node);
        return !(Result & DOS_DEVSTAT_BUSY);
    }
    else
    {
        DWORD FileSizeHigh;
        DWORD FileSize = GetFileSize(Descriptor->Win32Handle, &FileSizeHigh);
        LONG LocationHigh = 0;
        DWORD Location = SetFilePointer(Descriptor->Win32Handle, 0, &LocationHigh, FILE_CURRENT);

        return ((Location != FileSize) || (LocationHigh != FileSizeHigh));
    }
}

VOID DosPrintCharacter(WORD FileHandle, CHAR Character)
{
    WORD BytesWritten;

    Sda->ByteBuffer = Character;

    /* Use the file writing function */
    DosWriteFile(FileHandle,
                 MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer), DOS_DATA_SEGMENT),
                 1,
                 &BytesWritten);
}

BOOLEAN DosBIOSInitialize(VOID)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);

    LPSTR SourcePtr, Environment;
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
    SourcePtr = Environment = GetEnvironmentStrings();
    if (Environment == NULL) return FALSE;

    /* Fill the DOS system environment block */
    while (*SourcePtr)
    {
        /*
         * - Ignore environment strings starting with a '=',
         *   they describe current directories.
         * - Ignore also the WINDIR environment variable since
         *   DOS apps should ignore that we started from ReactOS.
         * - Upper-case the environment names, not their values.
         */
        if (*SourcePtr != '=' && _strnicmp(SourcePtr, "WINDIR", 6) != 0)
        {
            PCHAR Delim = NULL;

            /* Copy the environment string */
            strcpy(DestPtr, SourcePtr);

            /* Upper-case the environment name */
            Delim = strchr(DestPtr, '='); // Find the '=' delimiter
            if (Delim) *Delim = '\0';     // Temporarily replace it by NULL
            _strupr(DestPtr);             // Upper-case
            if (Delim) *Delim = '=';      // Restore the delimiter

            DestPtr += strlen(SourcePtr);

            /* NULL-terminate the environment string */
            *(DestPtr++) = '\0';
        }

        /* Move to the next string */
        SourcePtr += strlen(SourcePtr) + 1;
    }
    /* NULL-terminate the environment block */
    *DestPtr = '\0';

    /* Free the memory allocated for environment strings */
    FreeEnvironmentStrings(Environment);


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
    Sda->CurrentDrive = DosDirectory[0] - 'A';

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
        strncpy(CurrentDirectories[Sda->CurrentDrive], Path, DOS_DIR_LENGTH);
    }

    /* Read CONFIG.SYS */
    Stream = _wfopen(DOS_CONFIG_PATH, L"r");
    if (Stream != NULL)
    {
        while (fgetws(Buffer, ARRAYSIZE(Buffer), Stream))
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
