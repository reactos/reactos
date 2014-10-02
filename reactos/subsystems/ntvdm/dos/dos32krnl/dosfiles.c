/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/dosfiles.c
 * PURPOSE:         DOS32 Files Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"

#include "dos.h"
#include "dos/dem.h"

#include "bios/bios.h"

/* PUBLIC FUNCTIONS ***********************************************************/

WORD DosCreateFileEx(LPWORD Handle,
                     LPWORD CreationStatus,
                     LPCSTR FilePath,
                     BYTE AccessShareModes,
                     WORD CreateActionFlags,
                     WORD Attributes)
{
    WORD LastError;
    HANDLE FileHandle;
    WORD DosHandle;
    ACCESS_MASK AccessMode = 0;
    DWORD ShareMode = 0;
    DWORD CreationDisposition = 0;
    BOOL InheritableFile = FALSE;
    SECURITY_ATTRIBUTES SecurityAttributes;

    DPRINT1("DosCreateFileEx: FilePath \"%s\", AccessShareModes 0x%04X, CreateActionFlags 0x%04X, Attributes 0x%04X\n",
           FilePath, AccessShareModes, CreateActionFlags, Attributes);

    //
    // The article about OpenFile API: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365430(v=vs.85).aspx
    // explains what are those AccessShareModes (see the uStyle flag).
    //

    /* Parse the access mode */
    switch (AccessShareModes & 0x03)
    {
        /* Read-only */
        case 0:
            AccessMode = GENERIC_READ;
            break;

        /* Write only */
        case 1:
            AccessMode = GENERIC_WRITE;
            break;

        /* Read and write */
        case 2:
            AccessMode = GENERIC_READ | GENERIC_WRITE;
            break;

        /* Invalid */
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /* Parse the share mode */
    switch ((AccessShareModes >> 4) & 0x07)
    {
        /* Compatibility mode */
        case 0:
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;

        /* No sharing "DenyAll" */
        case 1:
            ShareMode = 0;
            break;

        /* No write share "DenyWrite" */
        case 2:
            ShareMode = FILE_SHARE_READ;
            break;

        /* No read share "DenyRead" */
        case 3:
            ShareMode = FILE_SHARE_WRITE;
            break;

        /* Full share "DenyNone" */
        case 4:
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        /* Invalid */
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /*
     * Parse the creation action flags:
     *
     * Bitfields for action:
     * Bit(s)  Description
     *
     * 7-4     Action if file does not exist.
     * 0000    Fail
     * 0001    Create
     *
     * 3-0     Action if file exists.
     * 0000    Fail
     * 0001    Open
     * 0010    Replace/open
     */
    switch (CreateActionFlags)
    {
        /* If the file exists, fail, otherwise, fail also */
        case 0x00:
            // A special case is used after the call to CreateFileA if it succeeds,
            // in order to close the opened handle and return an adequate error.
            CreationDisposition = OPEN_EXISTING;
            break;

        /* If the file exists, open it, otherwise, fail */
        case 0x01:
            CreationDisposition = OPEN_EXISTING;
            break;

        /* If the file exists, replace it, otherwise, fail */
        case 0x02:
            CreationDisposition = TRUNCATE_EXISTING;
            break;

        /* If the file exists, fail, otherwise, create it */
        case 0x10:
            CreationDisposition = CREATE_NEW;
            break;

        /* If the file exists, open it, otherwise, create it */
        case 0x11:
            CreationDisposition = OPEN_ALWAYS;
            break;

        /* If the file exists, replace it, otherwise, create it */
        case 0x12:
            CreationDisposition = CREATE_ALWAYS;
            break;

        /* Invalid */
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /* Check for inheritance */
    InheritableFile = ((AccessShareModes & 0x80) == 0);

    /* Assign default security attributes to the file, and set the inheritance flag */
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = InheritableFile;

    /* Open the file */
    FileHandle = CreateFileA(FilePath,
                             AccessMode,
                             ShareMode,
                             &SecurityAttributes,
                             CreationDisposition,
                             Attributes,
                             NULL);

    LastError = (WORD)GetLastError();

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        /* Return the error code */
        return LastError;
    }

    /*
     * Special case: CreateActionFlags == 0, we must fail because
     * the file exists (if it didn't exist we already failed).
     */
    if (CreateActionFlags == 0)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_FILE_EXISTS;
    }

    /* Set the creation status */
    switch (CreateActionFlags)
    {
        case 0x01:
            *CreationStatus = 0x01; // The file was opened
            break;

        case 0x02:
            *CreationStatus = 0x03; // The file was replaced
            break;

        case 0x10:
            *CreationStatus = 0x02; // The file was created
            break;

        case 0x11:
        {
            if (LastError == ERROR_ALREADY_EXISTS)
                *CreationStatus = 0x01; // The file was opened
            else
                *CreationStatus = 0x02; // The file was created

            break;
        }

        case 0x12:
        {
            if (LastError == ERROR_ALREADY_EXISTS)
                *CreationStatus = 0x03; // The file was replaced
            else
                *CreationStatus = 0x02; // The file was created

            break;
        }
    }

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(FileHandle);

    if (DosHandle == INVALID_DOS_HANDLE)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* It was successful */
    *Handle = DosHandle;
    return ERROR_SUCCESS;
}

WORD DosCreateFile(LPWORD Handle,
                   LPCSTR FilePath,
                   DWORD CreationDisposition,
                   WORD Attributes)
{
    HANDLE FileHandle;
    WORD DosHandle;

    DPRINT("DosCreateFile: FilePath \"%s\", CreationDisposition 0x%04X, Attributes 0x%04X\n",
           FilePath, CreationDisposition, Attributes);

    /* Create the file */
    FileHandle = CreateFileA(FilePath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             CreationDisposition,
                             Attributes,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        /* Return the error code */
        return (WORD)GetLastError();
    }

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(FileHandle);

    if (DosHandle == INVALID_DOS_HANDLE)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* It was successful */
    *Handle = DosHandle;
    return ERROR_SUCCESS;
}

WORD DosOpenFile(LPWORD Handle,
                 LPCSTR FilePath,
                 BYTE AccessShareModes)
{
    HANDLE FileHandle;
    ACCESS_MASK AccessMode = 0;
    DWORD ShareMode = 0;
    BOOL InheritableFile = FALSE;
    SECURITY_ATTRIBUTES SecurityAttributes;
    WORD DosHandle;

    DPRINT("DosOpenFile: FilePath \"%s\", AccessShareModes 0x%04X\n",
           FilePath, AccessShareModes);

    //
    // The article about OpenFile API: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365430(v=vs.85).aspx
    // explains what are those AccessShareModes (see the uStyle flag).
    //

    /* Parse the access mode */
    switch (AccessShareModes & 0x03)
    {
        /* Read-only */
        case 0:
            AccessMode = GENERIC_READ;
            break;

        /* Write only */
        case 1:
            AccessMode = GENERIC_WRITE;
            break;

        /* Read and write */
        case 2:
            AccessMode = GENERIC_READ | GENERIC_WRITE;
            break;

        /* Invalid */
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /* Parse the share mode */
    switch ((AccessShareModes >> 4) & 0x07)
    {
        /* Compatibility mode */
        case 0:
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;

        /* No sharing "DenyAll" */
        case 1:
            ShareMode = 0;
            break;

        /* No write share "DenyWrite" */
        case 2:
            ShareMode = FILE_SHARE_READ;
            break;

        /* No read share "DenyRead" */
        case 3:
            ShareMode = FILE_SHARE_WRITE;
            break;

        /* Full share "DenyNone" */
        case 4:
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        /* Invalid */
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /* Check for inheritance */
    InheritableFile = ((AccessShareModes & 0x80) == 0);

    /* Assign default security attributes to the file, and set the inheritance flag */
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = InheritableFile;

    /* Open the file */
    FileHandle = CreateFileA(FilePath,
                             AccessMode,
                             ShareMode,
                             &SecurityAttributes,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        /* Return the error code */
        return (WORD)GetLastError();
    }

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(FileHandle);

    if (DosHandle == INVALID_DOS_HANDLE)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* It was successful */
    *Handle = DosHandle;
    return ERROR_SUCCESS;
}

WORD DosReadFile(WORD FileHandle,
                 LPVOID Buffer,
                 WORD Count,
                 LPWORD BytesRead)
{
    WORD Result = ERROR_SUCCESS;
    DWORD BytesRead32 = 0;
    HANDLE Handle = DosGetRealHandle(FileHandle);

    DPRINT("DosReadFile: FileHandle 0x%04X, Count 0x%04X\n", FileHandle, Count);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    if (IsConsoleHandle(Handle))
    {
        CHAR Character;

        /*
         * Use BIOS Get Keystroke function
         */

        /* Save AX */
        USHORT AX = getAX();

        for (BytesRead32 = 0; BytesRead32 < Count; BytesRead32++)
        {
            /* Call the BIOS INT 16h, AH=00h "Get Keystroke" */
            setAH(0x00);
            Int32Call(&DosContext, BIOS_KBD_INTERRUPT);

            /* Retrieve the character in AL (scan code is in AH) */
            Character = getAL();

            if (DoEcho) DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

            ((PCHAR)Buffer)[BytesRead32] = Character;

            /* Stop on first carriage return */
            if (Character == '\r')
            {
                if (DoEcho) DosPrintCharacter(DOS_OUTPUT_HANDLE, '\n');
                break;
            }

            // BytesRead32++;
        }

        /* Restore AX */
        setAX(AX);
    }
    else
    {
        /* Read the file */
        if (!ReadFile(Handle, Buffer, Count /* * sizeof(CHAR) */, &BytesRead32, NULL))
        {
            /* Store the error code */
            Result = (WORD)GetLastError();
        }
    }

    /* The number of bytes read is always 16-bit */
    *BytesRead = LOWORD(BytesRead32);

    /* Return the error code */
    return Result;
}

WORD DosWriteFile(WORD FileHandle,
                  LPVOID Buffer,
                  WORD Count,
                  LPWORD BytesWritten)
{
    WORD Result = ERROR_SUCCESS;
    DWORD BytesWritten32 = 0;
    HANDLE Handle = DosGetRealHandle(FileHandle);

    DPRINT("DosWriteFile: FileHandle 0x%04X, Count 0x%04X\n", FileHandle, Count);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    if (IsConsoleHandle(Handle))
    {
        /*
         * Use BIOS Teletype function
         */

        /* Save AX and BX */
        USHORT AX = getAX();
        USHORT BX = getBX();

        // FIXME: Use BIOS Write String function INT 10h, AH=13h ??

        for (BytesWritten32 = 0; BytesWritten32 < Count; BytesWritten32++)
        {
            /* Set the parameters */
            setAL(((PCHAR)Buffer)[BytesWritten32]);
            setBL(DOS_CHAR_ATTRIBUTE);
            setBH(Bda->VideoPage);

            /* Call the BIOS INT 10h, AH=0Eh "Teletype Output" */
            setAH(0x0E);
            Int32Call(&DosContext, BIOS_VIDEO_INTERRUPT);

            // BytesWritten32++;
        }

        /* Restore AX and BX */
        setBX(BX);
        setAX(AX);
    }
    else
    {
        /* Write the file */
        if (!WriteFile(Handle, Buffer, Count /* * sizeof(CHAR) */, &BytesWritten32, NULL))
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

WORD DosSeekFile(WORD FileHandle,
                 LONG Offset,
                 BYTE Origin,
                 LPDWORD NewOffset)
{
    WORD Result = ERROR_SUCCESS;
    DWORD FilePointer;
    HANDLE Handle = DosGetRealHandle(FileHandle);

    DPRINT("DosSeekFile: FileHandle 0x%04X, Offset 0x%08X, Origin 0x%02X\n",
           FileHandle,
           Offset,
           Origin);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    /* Check if the origin is valid */
    if (Origin != FILE_BEGIN && Origin != FILE_CURRENT && Origin != FILE_END)
    {
        return ERROR_INVALID_FUNCTION;
    }

    /* Move the file pointer */
    if (IsConsoleHandle(Handle))
    {
        /* Always succeeds when seeking a console handle */
        FilePointer = 0;
        Result = ERROR_SUCCESS;
    }
    else
    {
        FilePointer = SetFilePointer(Handle, Offset, NULL, Origin);
    }

    /* Check if there's a possibility the operation failed */
    if (FilePointer == INVALID_SET_FILE_POINTER)
    {
        /* Get the real error code */
        Result = (WORD)GetLastError();
    }

    if (Result != ERROR_SUCCESS)
    {
        /* The operation did fail */
        return Result;
    }

    /* Return the file pointer, if requested */
    if (NewOffset) *NewOffset = FilePointer;

    /* Return success */
    return ERROR_SUCCESS;
}

BOOL DosFlushFileBuffers(WORD FileHandle)
{
    HANDLE Handle = DosGetRealHandle(FileHandle);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return FALSE;

    /*
     * This function can either flush files back to disks, or flush
     * console input buffers, in which case there is no need to check
     * whether the handle is a console handle. FlushFileBuffers()
     * automatically does this check and calls FlushConsoleInputBuffer()
     * if needed.
     */
    return FlushFileBuffers(Handle);
}
