/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/bios.c
 * PURPOSE:         DOS32 Bios
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "int32.h"

#include "../dem.h"
#include "dos.h"
#include "dosfiles.h"
#include "handle.h"
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

/* PRIVATE VARIABLES **********************************************************/

/* PUBLIC VARIABLES ***********************************************************/

/* Global DOS BIOS data area */
PBIOS_DATA BiosData;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DosEchoCharacter(CHAR Character)
{
    switch (Character)
    {
        case '\0':
        {
            /* Nothing */
            break;
        }

        case '\b':
        {
            /* Erase the character */
            DosPrintCharacter(DOS_OUTPUT_HANDLE, '\b');
            DosPrintCharacter(DOS_OUTPUT_HANDLE, ' ');
            DosPrintCharacter(DOS_OUTPUT_HANDLE, '\b');
            break;
        }

        default:
        {
            /*
             * Check if this is a special character
             * NOTE: \r and \n are handled by the underlying driver!
             */
            if (Character < 0x20 && Character != '\r' && Character != '\n')
            {
                DosPrintCharacter(DOS_OUTPUT_HANDLE, '^');
                Character += 'A' - 1;
            }

            /* Echo the character */
            DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);
        }
    }
}

CHAR DosReadCharacter(WORD FileHandle, BOOLEAN Echo)
{
    WORD BytesRead;
    PDOS_FILE_DESCRIPTOR Descriptor = NULL;
    WORD OldDeviceInfo;

    /* Find the standard input descriptor and switch it to binary mode */
    Descriptor = DosGetHandleFileDescriptor(FileHandle);
    if (Descriptor)
    {
        OldDeviceInfo = Descriptor->DeviceInfo;
        Descriptor->DeviceInfo |= FILE_INFO_BINARY;
    }

    Sda->ByteBuffer = '\0';
    DPRINT("DosReadCharacter\n");

    /* Use the file reading function */
    DosReadFile(FileHandle,
                MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer), DOS_DATA_SEGMENT),
                1,
                &BytesRead);

    /* Check if we should echo and the file is actually the CON device */
    if (Echo && Descriptor && Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        /* Echo the character */
        DosEchoCharacter(Sda->ByteBuffer);
    }

    /* Restore the old mode and return the character */
    if (Descriptor) Descriptor->DeviceInfo = OldDeviceInfo;
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

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
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

BOOLEAN DosBuildSysEnvBlock(VOID)
{
    LPSTR SourcePtr, Environment;
    LPSTR DestPtr = (LPSTR)SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0);

    /*
     * Get the environment strings
     *
     * NOTE: On non-STANDALONE builds, this corresponds to the VDM environment
     * as created by BaseVDM for NTVDM. On STANDALONE builds this is the Win32
     * environment. In this last case we need to convert it to a proper VDM env.
     */
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

    return TRUE;
}

BOOLEAN DosBIOSInitialize(VOID)
{
    FILE *Stream;
    WCHAR Buffer[256];

    /* Set the data segment */
    setDS(BIOS_DATA_SEGMENT);

    /* Initialize the global DOS BIOS data area */
    BiosData = (PBIOS_DATA)SEG_OFF_TO_PTR(BIOS_DATA_SEGMENT, 0x0000);

    /* Initialize the DOS BIOS stack */
    // FIXME: Add a block of fixed size for the stack in BIOS/DOS_DATA instead!
    setSS(0x0F00);
    setSP(0x0FF0);
/// setBP(0x091E); // DOS base stack pointer relic value

    /*
     * Initialize the INT 13h (BIOS Disk Services) handler chain support.
     *
     * The INT 13h handler chain is some functionality that allows DOS
     * to insert disk filter drivers in between the (hooked) INT 13h handler
     * and its original handler.
     * Typically, those are:
     * - filter for detecting disk changes (for floppy disks),
     * - filter for tracking formatting calls and correcting DMA boundary errors,
     * - a possible filter to work around a bug in a particular version of PC-AT's
     *   IBM's ROM BIOS (on systems with model byte FCh and BIOS date "01/10/84" only)
     * (see http://www.ctyme.com/intr/rb-4453.htm for more details).
     *
     * This functionality is known to be used by some legitimate programs,
     * by Windows 3.x, as well as some illegitimate ones (aka. virii).
     *
     * See extra information about this support in dos.h
     */
    // FIXME: Should be done by the DOS BIOS
    BiosData->RomBiosInt13 = ((PULONG)BaseAddress)[0x13];
    BiosData->PrevInt13    = BiosData->RomBiosInt13;
//  RegisterDosInt32(0x13, DosInt13h); // Unused at the moment!

    //
    // HERE: Do all hardware initialization needed for DOS
    //

    /*
     * SysInit part...
     */

    /* Initialize the DOS kernel (DosInit) */
    if (!DosKRNLInitialize())
    {
        BiosDisplayMessage("Failed to load the DOS kernel! Exiting...\n");
        return FALSE;
    }

    /* DOS kernel loading succeeded, we can finish the initialization */

    /* Build the system master (pre-) environment block (inherited by the shell) */
    if (!DosBuildSysEnvBlock())
    {
        DosDisplayMessage("An error occurred when setting up the system environment block.\n");
    }

    /* TODO: Read CONFIG.NT/SYS */
    Stream = _wfopen(DOS_CONFIG_PATH, L"r");
    if (Stream != NULL)
    {
        while (fgetws(Buffer, ARRAYSIZE(Buffer), Stream))
        {
            // TODO: Parse the line
        }
        fclose(Stream);
    }

    return TRUE;
}

/* EOF */
