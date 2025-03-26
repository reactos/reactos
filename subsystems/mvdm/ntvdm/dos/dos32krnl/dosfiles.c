/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/dosfiles.c
 * PURPOSE:         DOS32 Files Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "../../memory.h"

#include "dos.h"
#include "dos/dem.h"
#include "dosfiles.h"
#include "handle.h"
#include "process.h"

#include "bios/bios.h"

/* PRIVATE FUNCTIONS **********************************************************/

static VOID StoreNameInSft(LPCSTR FilePath, PDOS_FILE_DESCRIPTOR Descriptor)
{
    CHAR ShortPath[MAX_PATH];
    PCHAR Name;
    PCHAR Extension;

    /* Try to get the short path */
    if (!GetShortPathNameA(FilePath, ShortPath, sizeof(ShortPath)))
    {
        /* If it failed, just use the uppercase long path */
        strncpy(ShortPath, FilePath, sizeof(ShortPath) - 1);
        _strupr(ShortPath);
    }

    /* Get the name part */
    Name = strrchr(ShortPath, '\\');
    if (Name == NULL) Name = ShortPath;

    /* Find the extension */
    Extension = strchr(Name, '.');

    if (Extension)
    {
        /* Terminate the name string, and move the pointer to after the dot */
        *Extension++ = 0;
    }

    /* Copy the name into the SFT descriptor */
    RtlCopyMemory(Descriptor->FileName, Name, min(strlen(Name), 8));

    if (Extension)
    {
        /* Copy the extension too */
        RtlCopyMemory(&Descriptor->FileName[8], Extension, min(strlen(Extension), 3));
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE DosFindFreeDescriptor(VOID)
{
    UINT i;
    BYTE Count = 0;
    DWORD CurrentSft = SysVars->FirstSft;

    while (LOWORD(CurrentSft) != 0xFFFF)
    {
        PDOS_SFT Sft = (PDOS_SFT)FAR_POINTER(CurrentSft);

        for (i = 0; i < Sft->NumDescriptors; i++)
        {
            if (Sft->FileDescriptors[i].RefCount == 0) return Count;
            Count++;
        }

        /* Go to the next table */
        CurrentSft = Sft->Link;
    }

    /* Invalid ID */
    return 0xFF;
}

BYTE DosFindWin32Descriptor(HANDLE Win32Handle)
{
    UINT i;
    BYTE Count = 0;
    DWORD CurrentSft = SysVars->FirstSft;

    while (LOWORD(CurrentSft) != 0xFFFF)
    {
        PDOS_SFT Sft = (PDOS_SFT)FAR_POINTER(CurrentSft);

        for (i = 0; i < Sft->NumDescriptors; i++)
        {
            if ((Sft->FileDescriptors[i].RefCount > 0)
                && !(Sft->FileDescriptors[i].DeviceInfo & FILE_INFO_DEVICE)
                && (Sft->FileDescriptors[i].Win32Handle == Win32Handle))
            {
                return Count;
            }

            Count++;
        }

        /* Go to the next table */
        CurrentSft = Sft->Link;
    }

    /* Invalid ID */
    return 0xFF;
}

BYTE DosFindDeviceDescriptor(DWORD DevicePointer)
{
    UINT i;
    BYTE Count = 0;
    DWORD CurrentSft = SysVars->FirstSft;

    while (LOWORD(CurrentSft) != 0xFFFF)
    {
        PDOS_SFT Sft = (PDOS_SFT)FAR_POINTER(CurrentSft);

        for (i = 0; i < Sft->NumDescriptors; i++)
        {
            if ((Sft->FileDescriptors[i].RefCount > 0)
                && (Sft->FileDescriptors[i].DeviceInfo & FILE_INFO_DEVICE)
                && (Sft->FileDescriptors[i].DevicePointer == DevicePointer))
            {
                return Count;
            }

            Count++;
        }

        /* Go to the next table */
        CurrentSft = Sft->Link;
    }

    /* Invalid ID */
    return 0xFF;
}

PDOS_FILE_DESCRIPTOR DosGetFileDescriptor(BYTE Id)
{
    DWORD CurrentSft = SysVars->FirstSft;

    while (LOWORD(CurrentSft) != 0xFFFF)
    {
        PDOS_SFT Sft = (PDOS_SFT)FAR_POINTER(CurrentSft);

        /* Return it if it's in this table */
        if (Id <= Sft->NumDescriptors) return &Sft->FileDescriptors[Id];

        /* Go to the next table */
        Id -= Sft->NumDescriptors;
        CurrentSft = Sft->Link;
    }

    /* Invalid ID */
    return NULL;
}

PDOS_FILE_DESCRIPTOR DosGetHandleFileDescriptor(WORD DosHandle)
{
    BYTE DescriptorId = DosQueryHandle(DosHandle);
    if (DescriptorId == 0xFF) return NULL;

    return DosGetFileDescriptor(DescriptorId);
}

WORD DosCreateFileEx(LPWORD Handle,
                     LPWORD CreationStatus,
                     LPCSTR FilePath,
                     BYTE AccessShareModes,
                     WORD CreateActionFlags,
                     WORD Attributes)
{
    WORD LastError;
    HANDLE FileHandle;
    PDOS_DEVICE_NODE Node;
    WORD DosHandle;
    ACCESS_MASK AccessMode = 0;
    DWORD ShareMode = 0;
    DWORD CreationDisposition = 0;
    BOOL InheritableFile = FALSE;
    SECURITY_ATTRIBUTES SecurityAttributes;
    BYTE DescriptorId;
    PDOS_FILE_DESCRIPTOR Descriptor;

    DPRINT1("DosCreateFileEx: FilePath \"%s\", AccessShareModes 0x%04X, CreateActionFlags 0x%04X, Attributes 0x%04X\n",
           FilePath, AccessShareModes, CreateActionFlags, Attributes);

    //
    // The article about OpenFile API: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfile
    // explains what those AccessShareModes are (see the uStyle flag).
    //

    Node = DosGetDevice(FilePath);
    if (Node != NULL)
    {
        if (Node->OpenRoutine) Node->OpenRoutine(Node);
    }
    else
    {
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
    }

    DescriptorId = DosFindFreeDescriptor();
    if (DescriptorId == 0xFF)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* Set up the new descriptor */
    Descriptor = DosGetFileDescriptor(DescriptorId);
    RtlZeroMemory(Descriptor, sizeof(*Descriptor));
    RtlFillMemory(Descriptor->FileName, sizeof(Descriptor->FileName), ' ');

    if (Node != NULL)
    {
        Descriptor->DevicePointer = Node->Driver;
        Descriptor->DeviceInfo = Node->DeviceAttributes | FILE_INFO_DEVICE;
        RtlCopyMemory(Descriptor->FileName, Node->Name.Buffer, Node->Name.Length);
    }
    else
    {
        Descriptor->OpenMode = AccessShareModes;
        Descriptor->Attributes = LOBYTE(GetFileAttributesA(FilePath));
        Descriptor->Size = GetFileSize(FileHandle, NULL);
        Descriptor->Win32Handle = FileHandle;
        StoreNameInSft(FilePath, Descriptor);
    }

    Descriptor->OwnerPsp = Sda->CurrentPsp;

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(DescriptorId);
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
    PDOS_DEVICE_NODE Node;
    WORD DosHandle;
    BYTE DescriptorId;
    PDOS_FILE_DESCRIPTOR Descriptor;

    DPRINT("DosCreateFile: FilePath \"%s\", CreationDisposition 0x%04X, Attributes 0x%04X\n",
           FilePath, CreationDisposition, Attributes);

    Node = DosGetDevice(FilePath);
    if (Node != NULL)
    {
        if (Node->OpenRoutine) Node->OpenRoutine(Node);
    }
    else
    {
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
    }

    DescriptorId = DosFindFreeDescriptor();
    if (DescriptorId == 0xFF)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* Set up the new descriptor */
    Descriptor = DosGetFileDescriptor(DescriptorId);
    RtlZeroMemory(Descriptor, sizeof(*Descriptor));
    RtlFillMemory(Descriptor->FileName, sizeof(Descriptor->FileName), ' ');

    if (Node != NULL)
    {
        Descriptor->DevicePointer = Node->Driver;
        Descriptor->DeviceInfo = Node->DeviceAttributes | FILE_INFO_DEVICE;
        RtlCopyMemory(Descriptor->FileName, Node->Name.Buffer, Node->Name.Length);
    }
    else
    {
        Descriptor->Attributes = LOBYTE(GetFileAttributesA(FilePath));
        Descriptor->Size = GetFileSize(FileHandle, NULL);
        Descriptor->Win32Handle = FileHandle;
        StoreNameInSft(FilePath, Descriptor);
    }

    Descriptor->OwnerPsp = Sda->CurrentPsp;

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(DescriptorId);
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
    HANDLE FileHandle = NULL;
    PDOS_DEVICE_NODE Node;
    WORD DosHandle;
    BYTE DescriptorId;
    PDOS_FILE_DESCRIPTOR Descriptor;

    DPRINT("DosOpenFile: FilePath \"%s\", AccessShareModes 0x%04X\n",
           FilePath, AccessShareModes);

    //
    // The article about OpenFile API: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfile
    // explains what those AccessShareModes are (see the uStyle flag).
    //

    Node = DosGetDevice(FilePath);
    if (Node != NULL)
    {
        if (Node->OpenRoutine) Node->OpenRoutine(Node);
    }
    else
    {
        ACCESS_MASK AccessMode = 0;
        DWORD ShareMode = 0;
        BOOL InheritableFile = FALSE;
        SECURITY_ATTRIBUTES SecurityAttributes;

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
    }

    DescriptorId = DosFindFreeDescriptor();
    if (DescriptorId == 0xFF)
    {
        /* Close the file and return the error code */
        CloseHandle(FileHandle);
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* Set up the new descriptor */
    Descriptor = DosGetFileDescriptor(DescriptorId);
    RtlZeroMemory(Descriptor, sizeof(*Descriptor));
    RtlFillMemory(Descriptor->FileName, sizeof(Descriptor->FileName), ' ');

    if (Node != NULL)
    {
        Descriptor->DevicePointer = Node->Driver;
        Descriptor->DeviceInfo = Node->DeviceAttributes | FILE_INFO_DEVICE;
        RtlCopyMemory(Descriptor->FileName, Node->Name.Buffer, Node->Name.Length);
    }
    else
    {
        Descriptor->OpenMode = AccessShareModes;
        Descriptor->Attributes = LOBYTE(GetFileAttributesA(FilePath));
        Descriptor->Size = GetFileSize(FileHandle, NULL);
        Descriptor->Win32Handle = FileHandle;
        StoreNameInSft(FilePath, Descriptor);
    }

    Descriptor->OwnerPsp = Sda->CurrentPsp;

    /* Open the DOS handle */
    DosHandle = DosOpenHandle(DescriptorId);
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

BYTE DosReadLineBuffered(WORD FileHandle, DWORD Buffer, BYTE MaxSize)
{
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(FileHandle);
    PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);
    BYTE LineSize = 0;
    PCHAR Pointer = FAR_POINTER(Buffer);
    CHAR Character;

    do
    {
        USHORT Amount = 1;

        /* Read a character from the device */
        Node->ReadRoutine(Node,
                          MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer),
                                   DOS_DATA_SEGMENT),
                          &Amount);
        if (Amount == 0) break;

        Character = Sda->ByteBuffer;

        if (LineSize == MaxSize - 1 && Character != '\r' && Character != '\b')
        {
            /* Line buffer full */
            // TODO: Should we beep?
            continue;
        }

        switch (Character)
        {
            /* Extended character */
            case '\0':
            {
                /* Read the scancode and discard it */
                Amount = 1;
                Node->ReadRoutine(Node,
                                  MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer),
                                           DOS_DATA_SEGMENT),
                                  &Amount);
                break;
            }

            /* Ctrl-C */
            case 0x03:
            {
                DosEchoCharacter(Character);

                if (DosControlBreak())
                {
                    /* Set the character to CR to end the loop */
                    Character = '\r';
                }

                break;
            }

            case '\n':
            {
                DosEchoCharacter('\r');
                DosEchoCharacter('\n');
                break;
            }

            case '\b':
            {
                if (LineSize > 0)
                {
                    LineSize--;
                    DosEchoCharacter(Character);

                    /* Erase the '^' too */
                    if (Pointer[LineSize] > 0x00 && Pointer[LineSize] < 0x20)
                    {
                        DosEchoCharacter(Character);
                    }
                }

                break;
            }

            default:
            {
                /* Store the character in the buffer */
                Pointer[LineSize++] = Character;
                DosEchoCharacter(Character);
            }
        }

    /* Stop on a carriage return */
    } while (Character != '\r');

    return LineSize - 1;
}

WORD DosReadFile(WORD FileHandle,
                 DWORD Buffer,
                 WORD Count,
                 LPWORD BytesRead)
{
    WORD Result = ERROR_SUCCESS;
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(FileHandle);
    BYTE StaticBuffer[8192];

    DPRINT("DosReadFile: FileHandle 0x%04X, Count 0x%04X\n", FileHandle, Count);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        return ERROR_INVALID_HANDLE;
    }

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);
        if (!Node->ReadRoutine) return ERROR_INVALID_FUNCTION;

        if (Descriptor->DeviceInfo & FILE_INFO_BINARY)
        {
            /* Read from the device directly */
            Node->ReadRoutine(Node, Buffer, &Count);
            *BytesRead = Count;
        }
        else if (Descriptor->DeviceInfo & FILE_INFO_STDIN)
        {
            /* Line-buffered CON input */
            PCHAR ConBuffer = NULL;
            PCHAR Pointer = FAR_POINTER(Buffer);

            /* Check if the buffer is empty */
            if (!SysVars->UnreadConInput)
            {
                SysVars->UnreadConInput = FIELD_OFFSET(DOS_DATA, UnreadConInputBuffer);

                DosReadLineBuffered(FileHandle,
                                    MAKELONG(SysVars->UnreadConInput, DOS_DATA_SEGMENT),
                                    sizeof(DosData->UnreadConInputBuffer));
            }

            *BytesRead = 0;
            ConBuffer = (PCHAR)SEG_OFF_TO_PTR(DOS_DATA_SEGMENT, SysVars->UnreadConInput);

            while (*BytesRead < Count)
            {
                Pointer[(*BytesRead)++] = *ConBuffer;

                if (*ConBuffer == '\r')
                {
                    /* A carriage return turns into a line feed */
                    *ConBuffer = '\n';
                }
                else if (*ConBuffer == '\n')
                {
                    /* A line feed marks the true end of the line */
                    SysVars->UnreadConInput = 0;

                    /* Echo the line feed */
                    DosEchoCharacter('\n');
                    break;
                }
                else
                {
                    /* Move to the next character */
                    SysVars->UnreadConInput++;
                    ConBuffer++;
                }
            }
        }
        else
        {
            /* Translated input from a character device that isn't CON */
            PCHAR Pointer = FAR_POINTER(Buffer);
            CHAR Character;

            *BytesRead = 0;

            while (*BytesRead < Count)
            {
                USHORT Amount = 1;

                /* Read a character from the device */
                Node->ReadRoutine(Node,
                                  MAKELONG(DOS_DATA_OFFSET(Sda.ByteBuffer),
                                           DOS_DATA_SEGMENT),
                                  &Amount);
                if (Amount == 0) break;

                Character = Sda->ByteBuffer;
                // TODO: Process it somehow?

                /* Store the character in the output buffer */
                Pointer[(*BytesRead)++] = Character;

                /* Check for EOF */
                if (Character == 0x1A) break;
            }
        }
    }
    else
    {
        DWORD BytesRead32 = 0;
        PVOID LocalBuffer;

        if (Count <= sizeof(StaticBuffer))
        {
            LocalBuffer = StaticBuffer;
        }
        else
        {
            LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Count);
            ASSERT(LocalBuffer != NULL);
        }

        /* Read from the file */
        if (ReadFile(Descriptor->Win32Handle, LocalBuffer, Count, &BytesRead32, NULL))
        {
            /* Write to the memory */
            EmulatorWriteMemory(&EmulatorContext,
                                TO_LINEAR(HIWORD(Buffer), LOWORD(Buffer)),
                                LocalBuffer,
                                LOWORD(BytesRead32));

            /* Update the position */
            Descriptor->Position += BytesRead32; // or LOWORD(BytesRead32); ?
        }
        else
        {
            /* Store the error code */
            Result = (WORD)GetLastError();
        }

        /* The number of bytes read is always 16-bit */
        *BytesRead = LOWORD(BytesRead32);

        if (LocalBuffer != StaticBuffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);
    }

    /* Return the error code */
    return Result;
}

WORD DosWriteFile(WORD FileHandle,
                  DWORD Buffer,
                  WORD Count,
                  LPWORD BytesWritten)
{
    WORD Result = ERROR_SUCCESS;
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(FileHandle);
    BYTE StaticBuffer[8192];

    DPRINT("DosWriteFile: FileHandle 0x%04X, Count 0x%04X\n", FileHandle, Count);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        return ERROR_INVALID_HANDLE;
    }

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);
        if (!Node->WriteRoutine) return ERROR_INVALID_FUNCTION;

        /* Read the device */
        Node->WriteRoutine(Node, Buffer, &Count);
        *BytesWritten = Count;
    }
    else
    {
        DWORD BytesWritten32 = 0;
        PVOID LocalBuffer;

        /*
         * Writing zero bytes truncates or extends the file
         * to the current position of the file pointer.
         */
        if (Count == 0)
        {
            if (!SetEndOfFile(Descriptor->Win32Handle))
            {
                /* Store the error code */
                Result = (WORD)GetLastError();
            }
            *BytesWritten = 0;
            return Result;
        }

        if (Count <= sizeof(StaticBuffer))
        {
            LocalBuffer = StaticBuffer;
        }
        else
        {
            LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Count);
            ASSERT(LocalBuffer != NULL);
        }

        /* Read from the memory */
        EmulatorReadMemory(&EmulatorContext,
                           TO_LINEAR(HIWORD(Buffer), LOWORD(Buffer)),
                           LocalBuffer,
                           Count);

        /* Write to the file */
        if (WriteFile(Descriptor->Win32Handle, LocalBuffer, Count, &BytesWritten32, NULL))
        {
            /* Update the position and size */
            Descriptor->Position += BytesWritten32; // or LOWORD(BytesWritten32); ?
            if (Descriptor->Position > Descriptor->Size) Descriptor->Size = Descriptor->Position;
        }
        else
        {
            /* Store the error code */
            Result = (WORD)GetLastError();
        }

        /* The number of bytes written is always 16-bit */
        *BytesWritten = LOWORD(BytesWritten32);

        if (LocalBuffer != StaticBuffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);
    }

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
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(FileHandle);

    DPRINT("DosSeekFile: FileHandle 0x%04X, Offset 0x%08X, Origin 0x%02X\n",
           FileHandle, Offset, Origin);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        return ERROR_INVALID_HANDLE;
    }

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        /* For character devices, always return success */
        return ERROR_SUCCESS;
    }

    /* Check if the origin is valid */
    if (Origin != FILE_BEGIN && Origin != FILE_CURRENT && Origin != FILE_END)
    {
        return ERROR_INVALID_FUNCTION;
    }

    FilePointer = SetFilePointer(Descriptor->Win32Handle, Offset, NULL, Origin);

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

    /* Update the position */
    Descriptor->Position = FilePointer;

    /* Return the file pointer, if requested */
    if (NewOffset) *NewOffset = FilePointer;

    /* Return success */
    return ERROR_SUCCESS;
}

BOOL DosFlushFileBuffers(WORD FileHandle)
{
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(FileHandle);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        PDOS_DEVICE_NODE Node = DosGetDriverNode(Descriptor->DevicePointer);

        if (Node->FlushInputRoutine) Node->FlushInputRoutine(Node);
        if (Node->FlushOutputRoutine) Node->FlushOutputRoutine(Node);

        return TRUE;
    }
    else
    {
        return FlushFileBuffers(Descriptor->Win32Handle);
    }
}

BOOLEAN DosLockFile(WORD DosHandle, DWORD Offset, DWORD Size)
{
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(DosHandle);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    /* Always succeed for character devices */
    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE) return TRUE;

    if (!LockFile(Descriptor->Win32Handle, Offset, 0, Size, 0))
    {
        Sda->LastErrorCode = GetLastError();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN DosUnlockFile(WORD DosHandle, DWORD Offset, DWORD Size)
{
    PDOS_FILE_DESCRIPTOR Descriptor = DosGetHandleFileDescriptor(DosHandle);

    if (Descriptor == NULL)
    {
        /* Invalid handle */
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    /* Always succeed for character devices */
    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE) return TRUE;

    if (!UnlockFile(Descriptor->Win32Handle, Offset, 0, Size, 0))
    {
        Sda->LastErrorCode = GetLastError();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN DosDeviceIoControlDrive(WORD DriveNumber, BYTE ControlCode, DWORD Buffer, PWORD Result)
{
    CHAR RootPath[] = "?:\\";

    if (DriveNumber == 0x00)
        RootPath[0] = 'A' + Sda->CurrentDrive;
    else
        RootPath[0] = 'A' + DriveNumber - 1;

    switch (ControlCode)
    {
        case 0x04:
            DPRINT1("UNIMPLEMENTED INT 21h, 4404h, Read from block device %s\n", RootPath);
            Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
            break;
        case 0x05:
            DPRINT1("UNIMPLEMENTED INT 21h, 4405h, Write block device control string %s\n", RootPath);
            Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
            break;
        case 0x08:
        {
            DWORD DriveType = GetDriveTypeA(RootPath);

            switch (DriveType)
            {
            case DRIVE_UNKNOWN:
            case DRIVE_NO_ROOT_DIR:
            default:
                DPRINT1("INT 21h, 4408h, %s -> DriveType = 0x%x\n", RootPath, DriveType);
                *Result = 0x000f;
                return TRUE;
            case DRIVE_REMOVABLE:
            case DRIVE_CDROM:
                *Result = 0x0000;
                return TRUE;
            case DRIVE_FIXED:
                *Result = 0x0001;
                return TRUE;
            case DRIVE_REMOTE:
            case DRIVE_RAMDISK: // ??
                break;
            }
            Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
            return FALSE;
        }
        case 0x09:
            DPRINT1("UNIMPLEMENTED INT 21h, 4409h, Determine if a logical device is local or remote %s\n", RootPath);
            Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
            return FALSE;
        default:
            assert(0);
            break;
    }

    return FALSE;
}

BOOLEAN DosDeviceIoControl(WORD FileHandle, BYTE ControlCode, DWORD Buffer, PWORD Length)
{
    PDOS_FILE_DESCRIPTOR Descriptor;
    PDOS_DEVICE_NODE Node = NULL;

    switch (ControlCode)
    {
        case 0x04:
        case 0x05:
        case 0x08:
        case 0x09:
            return DosDeviceIoControlDrive(FileHandle, ControlCode, Buffer, Length);
    }

    Descriptor = DosGetHandleFileDescriptor(FileHandle);

    if (!Descriptor)
    {
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    if (Descriptor->DeviceInfo & FILE_INFO_DEVICE)
    {
        Node = DosGetDriverNode(Descriptor->DevicePointer);
    }

    switch (ControlCode)
    {
        /* Get Device Information */
        case 0x00:
        {
            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2820.htm
             * for a list of possible flags.
             */
            setDX(Descriptor->DeviceInfo);
            return TRUE;
        }

        /* Set Device Information */
        case 0x01:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
            return FALSE;
        }

        /* Read from Device I/O Control Channel */
        case 0x02:
        {
            if (Node == NULL || !(Node->DeviceAttributes & DOS_DEVATTR_IOCTL))
            {
                Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
                return FALSE;
            }

            /* Do nothing if there is no IOCTL routine */
            if (!Node->IoctlReadRoutine)
            {
                *Length = 0;
                return TRUE;
            }

            Node->IoctlReadRoutine(Node, Buffer, Length);
            return TRUE;
        }

        /* Write to Device I/O Control Channel */
        case 0x03:
        {
            if (Node == NULL || !(Node->DeviceAttributes & DOS_DEVATTR_IOCTL))
            {
                Sda->LastErrorCode = ERROR_INVALID_FUNCTION;
                return FALSE;
            }

            /* Do nothing if there is no IOCTL routine */
            if (!Node->IoctlWriteRoutine)
            {
                *Length = 0;
                return TRUE;
            }

            Node->IoctlWriteRoutine(Node, Buffer, Length);
            return TRUE;
        }

        /* Get Input Status */
        case 0x06:
        {
            /* Check if this is a file or a device */
            if (Node)
            {
                /* Device*/

                if (!Node->InputStatusRoutine || Node->InputStatusRoutine(Node))
                {
                    /* Set the length to 0xFF to mark that it's ready */
                    *Length = 0xFF;
                }
                else
                {
                    /* Not ready */
                    *Length = 0;
                }
            }
            else
            {
                /* File */

                if (Descriptor->Position < Descriptor->Size)
                {
                    /* Set the length to 0xFF to mark that it's ready */
                    *Length = 0xFF;
                }
                else
                {
                    /* Not ready */
                    *Length = 0;
                }
            }

            return TRUE;
        }

        /* Get Output Status */
        case 0x07:
        {
            /* Check if this is a file or a device */
            if (Node)
            {
                /* Device*/

                if (!Node->OutputStatusRoutine || Node->OutputStatusRoutine(Node))
                {
                    /* Set the length to 0xFF to mark that it's ready */
                    *Length = 0xFF;
                }
                else
                {
                    /* Not ready */
                    *Length = 0;
                }
            }
            else
            {
                /* Files are always ready for output */
                *Length = 0xFF;
            }

            return TRUE;
        }

        /* Unsupported control code */
        default:
        {
            DPRINT1("Unsupported IOCTL: 0x%02X\n", ControlCode);

            Sda->LastErrorCode = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
    }
}

/* EOF */
