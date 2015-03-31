/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/dos.c
 * PURPOSE:         DOS32 Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "cpu/cpu.h"
#include "int32.h"

#include "dos.h"
#include "dos/dem.h"
#include "device.h"
#include "memory.h"

#include "bios/bios.h"

#include "io.h"
#include "hardware/ps2.h"

/* PRIVATE VARIABLES **********************************************************/

CALLBACK16 DosContext;

static DWORD DiskTransferArea;
/*static*/ BYTE CurrentDrive;
static CHAR LastDrive = 'E';
static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];
static DOS_SFT_ENTRY DosSystemFileTable[DOS_SFT_SIZE];
static WORD DosErrorLevel = 0x0000;

/* PUBLIC VARIABLES ***********************************************************/

/* Echo state for INT 21h, AH = 01h and AH = 3Fh */
BOOLEAN DoEcho = FALSE;
WORD CurrentPsp = SYSTEM_PSP;
WORD DosLastError = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static WORD DosCopyEnvironmentBlock(LPCSTR Environment OPTIONAL,
                                    LPCSTR ProgramName)
{
    PCHAR Ptr, DestBuffer = NULL;
    ULONG TotalSize = 0;
    WORD DestSegment;

    /* If we have an environment strings list, compute its size */
    if (Environment)
    {
        /* Calculate the size of the environment block */
        Ptr = (PCHAR)Environment;
        while (*Ptr) Ptr += strlen(Ptr) + 1;
        TotalSize = (ULONG_PTR)Ptr - (ULONG_PTR)Environment;
    }
    else
    {
        /* Empty environment string */
        TotalSize = 1;
    }
    /* Add the final environment block NULL-terminator */
    TotalSize++;

    /* Add the two bytes for the program name tag */
    TotalSize += 2;

    /* Add the string buffer size */
    TotalSize += strlen(ProgramName) + 1;

    /* Allocate the memory for the environment block */
    DestSegment = DosAllocateMemory((WORD)((TotalSize + 0x0F) >> 4), NULL);
    if (!DestSegment) return 0;

    DestBuffer = (PCHAR)SEG_OFF_TO_PTR(DestSegment, 0);

    /* If we have an environment strings list, copy it */
    if (Environment)
    {
        Ptr = (PCHAR)Environment;
        while (*Ptr)
        {
            /* Copy the string and NULL-terminate it */
            strcpy(DestBuffer, Ptr);
            DestBuffer += strlen(Ptr);
            *(DestBuffer++) = '\0';

            /* Move to the next string */
            Ptr += strlen(Ptr) + 1;
        }
    }
    else
    {
        /* Empty environment string */
        *(DestBuffer++) = '\0';
    }
    /* NULL-terminate the environment block */
    *(DestBuffer++) = '\0';

    /* Store the special program name tag */
    *(DestBuffer++) = LOBYTE(DOS_PROGRAM_NAME_TAG);
    *(DestBuffer++) = HIBYTE(DOS_PROGRAM_NAME_TAG);

    /* Copy the program name after the environment block */
    strcpy(DestBuffer, ProgramName);

    return DestSegment;
}

/* Taken from base/shell/cmd/console.c */
static BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & FILE_TYPE_CHAR) == 0) return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

static inline PDOS_SFT_ENTRY DosFindFreeSftEntry(VOID)
{
    UINT i;

    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        if (DosSystemFileTable[i].Type == DOS_SFT_ENTRY_NONE)
        {
            return &DosSystemFileTable[i];
        }
    }

    return NULL;
}

static inline PDOS_SFT_ENTRY DosFindWin32SftEntry(HANDLE Handle)
{
    UINT i;

    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        if (DosSystemFileTable[i].Type == DOS_SFT_ENTRY_WIN32
            && DosSystemFileTable[i].Handle == Handle)
        {
            return &DosSystemFileTable[i];
        }
    }

    return NULL;
}

static inline PDOS_SFT_ENTRY DosFindDeviceSftEntry(PDOS_DEVICE_NODE Device)
{
    UINT i;

    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        if (DosSystemFileTable[i].Type == DOS_SFT_ENTRY_DEVICE
            && DosSystemFileTable[i].DeviceNode == Device)
        {
            return &DosSystemFileTable[i];
        }
    }

    return NULL;
}

WORD DosOpenHandle(HANDLE Handle)
{
    WORD DosHandle;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_SFT_ENTRY SftEntry;

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return INVALID_DOS_HANDLE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Find a free entry in the JFT */
    for (DosHandle = 0; DosHandle < PspBlock->HandleTableSize; DosHandle++)
    {
        if (HandleTable[DosHandle] == 0xFF) break;
    }

    /* If there are no free entries, fail */
    if (DosHandle == PspBlock->HandleTableSize) return INVALID_DOS_HANDLE;

    /* Check if the handle is already in the SFT */
    SftEntry = DosFindWin32SftEntry(Handle);
    if (SftEntry != NULL)
    {
        /* Already in the table, reference it */
        SftEntry->RefCount++;
        goto Finish;
    }

    /* Find a free SFT entry to use */
    SftEntry = DosFindFreeSftEntry();
    if (SftEntry == NULL)
    {
        /* The SFT is full */
        return INVALID_DOS_HANDLE;
    }

    /* Initialize the empty table entry */
    SftEntry->Type       = DOS_SFT_ENTRY_WIN32;
    SftEntry->Handle     = Handle;
    SftEntry->RefCount   = 1;

Finish:

    /* Set the JFT entry to that SFT index */
    HandleTable[DosHandle] = ARRAY_INDEX(SftEntry, DosSystemFileTable);

    /* Return the new handle */
    return DosHandle;
}

WORD DosOpenDevice(PDOS_DEVICE_NODE Device)
{
    WORD DosHandle;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_SFT_ENTRY SftEntry;

    DPRINT("DosOpenDevice(\"%Z\")\n", &Device->Name);

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return INVALID_DOS_HANDLE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Find a free entry in the JFT */
    for (DosHandle = 0; DosHandle < PspBlock->HandleTableSize; DosHandle++)
    {
        if (HandleTable[DosHandle] == 0xFF) break;
    }

    /* If there are no free entries, fail */
    if (DosHandle == PspBlock->HandleTableSize) return INVALID_DOS_HANDLE;

    /* Check if the device is already in the SFT */
    SftEntry = DosFindDeviceSftEntry(Device);
    if (SftEntry != NULL)
    {
        /* Already in the table, reference it */
        SftEntry->RefCount++;
        goto Finish;
    }

    /* Find a free SFT entry to use */
    SftEntry = DosFindFreeSftEntry();
    if (SftEntry == NULL)
    {
        /* The SFT is full */
        return INVALID_DOS_HANDLE;
    }

    /* Initialize the empty table entry */
    SftEntry->Type       = DOS_SFT_ENTRY_DEVICE;
    SftEntry->DeviceNode = Device;
    SftEntry->RefCount   = 1;

Finish:

    /* Call the open routine, if it exists */
    if (Device->OpenRoutine) Device->OpenRoutine(Device);

    /* Set the JFT entry to that SFT index */
    HandleTable[DosHandle] = ARRAY_INDEX(SftEntry, DosSystemFileTable);

    /* Return the new handle */
    return DosHandle;
}

static VOID DosCopyHandleTable(LPBYTE DestinationTable)
{
    UINT i;
    PDOS_PSP PspBlock;
    LPBYTE SourceTable;

    /* Clear the table first */
    for (i = 0; i < 20; i++) DestinationTable[i] = 0xFF;

    /* Check if this is the initial process */
    if (CurrentPsp == SYSTEM_PSP)
    {
        PDOS_SFT_ENTRY SftEntry;
        HANDLE StandardHandles[3];
        PDOS_DEVICE_NODE Con = DosGetDevice("CON");
        ASSERT(Con != NULL);

        /* Get the native standard handles */
        StandardHandles[0] = GetStdHandle(STD_INPUT_HANDLE);
        StandardHandles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
        StandardHandles[2] = GetStdHandle(STD_ERROR_HANDLE);

        for (i = 0; i < 3; i++)
        {
            /* Find the corresponding SFT entry */
            if (IsConsoleHandle(StandardHandles[i]))
            {
                SftEntry = DosFindDeviceSftEntry(Con);
            }
            else
            {
                SftEntry = DosFindWin32SftEntry(StandardHandles[i]);
            }

            if (SftEntry == NULL)
            {
                /* Create a new SFT entry for it */
                SftEntry = DosFindFreeSftEntry();
                if (SftEntry == NULL)
                {
                    DPRINT1("Cannot create standard handle %d, the SFT is full!\n", i);
                    continue;
                }

                SftEntry->RefCount = 0;

                if (IsConsoleHandle(StandardHandles[i]))
                {
                    SftEntry->Type = DOS_SFT_ENTRY_DEVICE;
                    SftEntry->DeviceNode = Con;

                    /* Call the open routine */
                    if (Con->OpenRoutine) Con->OpenRoutine(Con);
                }
                else
                {
                    SftEntry->Type = DOS_SFT_ENTRY_WIN32;
                    SftEntry->Handle = StandardHandles[i];
                }
            }

            SftEntry->RefCount++;
            DestinationTable[i] = ARRAY_INDEX(SftEntry, DosSystemFileTable);
        }
    }
    else
    {
        /* Get the parent PSP block and handle table */
        PspBlock = SEGMENT_TO_PSP(CurrentPsp);
        SourceTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

        /* Copy the first 20 handles into the new table */
        for (i = 0; i < DEFAULT_JFT_SIZE; i++)
        {
            DestinationTable[i] = SourceTable[i];

            /* Increase the reference count */
            DosSystemFileTable[SourceTable[i]].RefCount++;
        }
    }
}

static BOOLEAN DosResizeHandleTable(WORD NewSize)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    WORD Segment;

    /* Get the PSP block */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);

    if (NewSize == PspBlock->HandleTableSize)
    {
        /* No change */
        return TRUE;
    }

    if (PspBlock->HandleTableSize > DEFAULT_JFT_SIZE)
    {
        /* Get the segment of the current table */
        Segment = (LOWORD(PspBlock->HandleTablePtr) >> 4) + HIWORD(PspBlock->HandleTablePtr);

        if (NewSize <= DEFAULT_JFT_SIZE)
        {
            /* Get the current handle table */
            HandleTable = FAR_POINTER(PspBlock->HandleTablePtr);

            /* Copy it to the PSP */
            RtlCopyMemory(PspBlock->HandleTable, HandleTable, NewSize);

            /* Free the memory */
            DosFreeMemory(Segment);

            /* Update the handle table pointer and size */
            PspBlock->HandleTableSize = NewSize;
            PspBlock->HandleTablePtr = MAKELONG(0x18, CurrentPsp);
        }
        else
        {
            /* Resize the memory */
            if (!DosResizeMemory(Segment, NewSize, NULL))
            {
                /* Unable to resize, try allocating it somewhere else */
                Segment = DosAllocateMemory(NewSize, NULL);
                if (Segment == 0) return FALSE;

                /* Get the new handle table */
                HandleTable = SEG_OFF_TO_PTR(Segment, 0);

                /* Copy the handles to the new table */
                RtlCopyMemory(HandleTable,
                              FAR_POINTER(PspBlock->HandleTablePtr),
                              PspBlock->HandleTableSize);

                /* Update the handle table pointer */
                PspBlock->HandleTablePtr = MAKELONG(0, Segment);
            }

            /* Update the handle table size */
            PspBlock->HandleTableSize = NewSize;
        }
    }
    else if (NewSize > DEFAULT_JFT_SIZE)
    {
        Segment = DosAllocateMemory(NewSize, NULL);
        if (Segment == 0) return FALSE;

        /* Get the new handle table */
        HandleTable = SEG_OFF_TO_PTR(Segment, 0);

        /* Copy the handles from the PSP to the new table */
        RtlCopyMemory(HandleTable,
                      FAR_POINTER(PspBlock->HandleTablePtr),
                      PspBlock->HandleTableSize);

        /* Update the handle table pointer and size */
        PspBlock->HandleTableSize = NewSize;
        PspBlock->HandleTablePtr = MAKELONG(0, Segment);
    }

    return TRUE;
}

static BOOLEAN DosCloseHandle(WORD DosHandle)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;
    PDOS_SFT_ENTRY SftEntry;

    DPRINT("DosCloseHandle: DosHandle 0x%04X\n", DosHandle);

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return FALSE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the handle is open */
    if (HandleTable[DosHandle] == 0xFF) return FALSE;

    /* Make sure the SFT entry is valid */
    SftEntry = &DosSystemFileTable[HandleTable[DosHandle]];
    if (SftEntry->Type == DOS_SFT_ENTRY_NONE) return FALSE;

    /* Decrement the reference count of the SFT entry */
    SftEntry->RefCount--;

    /* Check if the reference count fell to zero */
    if (!SftEntry->RefCount)
    {
        switch (SftEntry->Type)
        {
            case DOS_SFT_ENTRY_WIN32:
            {
                /* Close the win32 handle and clear it */
                CloseHandle(SftEntry->Handle);

                break;
            }

            case DOS_SFT_ENTRY_DEVICE:
            {
                PDOS_DEVICE_NODE Node = SftEntry->DeviceNode;

                /* Call the close routine, if it exists */
                if (Node->CloseRoutine) SftEntry->DeviceNode->CloseRoutine(SftEntry->DeviceNode);

                break;
            }

            default:
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }
        }

        /* Invalidate the SFT entry */
        SftEntry->Type = DOS_SFT_ENTRY_NONE;
    }

    /* Clear the entry in the JFT */
    HandleTable[DosHandle] = 0xFF;

    return TRUE;
}

static BOOLEAN DosDuplicateHandle(WORD OldHandle, WORD NewHandle)
{
    BYTE SftIndex;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

    DPRINT("DosDuplicateHandle: OldHandle 0x%04X, NewHandle 0x%04X\n",
           OldHandle,
           NewHandle);

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return FALSE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the old handle is open */
    if (HandleTable[OldHandle] == 0xFF) return FALSE;

    /* Check if the new handle is open */
    if (HandleTable[NewHandle] != 0xFF)
    {
        /* Close it */
        DosCloseHandle(NewHandle);
    }

    /* Increment the reference count of the SFT entry */
    SftIndex = HandleTable[OldHandle];
    DosSystemFileTable[SftIndex].RefCount++;

    /* Make the new handle point to that SFT entry */
    HandleTable[NewHandle] = SftIndex;

    /* Return success */
    return TRUE;
}

static BOOLEAN DosChangeDrive(BYTE Drive)
{
    WCHAR DirectoryPath[DOS_CMDLINE_LENGTH];

    /* Make sure the drive exists */
    if (Drive > (LastDrive - 'A')) return FALSE;

    /* Find the path to the new current directory */
    swprintf(DirectoryPath, L"%c\\%S", Drive + 'A', CurrentDirectories[Drive]);

    /* Change the current directory of the process */
    if (!SetCurrentDirectory(DirectoryPath)) return FALSE;

    /* Set the current drive */
    CurrentDrive = Drive;

    /* Return success */
    return TRUE;
}

static BOOLEAN DosChangeDirectory(LPSTR Directory)
{
    BYTE DriveNumber;
    DWORD Attributes;
    LPSTR Path;

    /* Make sure the directory path is not too long */
    if (strlen(Directory) >= DOS_DIR_LENGTH)
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Get the drive number */
    DriveNumber = Directory[0] - 'A';

    /* Make sure the drive exists */
    if (DriveNumber > (LastDrive - 'A'))
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Get the file attributes */
    Attributes = GetFileAttributesA(Directory);

    /* Make sure the path exists and is a directory */
    if ((Attributes == INVALID_FILE_ATTRIBUTES)
        || !(Attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        DosLastError = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }

    /* Check if this is the current drive */
    if (DriveNumber == CurrentDrive)
    {
        /* Change the directory */
        if (!SetCurrentDirectoryA(Directory))
        {
            DosLastError = LOWORD(GetLastError());
            return FALSE;
        }
    }

    /* Get the directory part of the path */
    Path = strchr(Directory, '\\');
    if (Path != NULL)
    {
        /* Skip the backslash */
        Path++;
    }

    /* Set the directory for the drive */
    if (Path != NULL)
    {
        strncpy(CurrentDirectories[DriveNumber], Path, DOS_DIR_LENGTH);
    }
    else
    {
        CurrentDirectories[DriveNumber][0] = '\0';
    }

    /* Return success */
    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

PDOS_SFT_ENTRY DosGetSftEntry(WORD DosHandle)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return NULL;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the handle is open */
    if (HandleTable[DosHandle] == 0xFF) return NULL;

    /* Return a pointer to the SFT entry */
    return &DosSystemFileTable[HandleTable[DosHandle]];
}

VOID DosInitializePsp(WORD PspSegment, LPCSTR CommandLine, WORD ProgramSize, WORD Environment)
{
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(PspSegment);
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    RtlZeroMemory(PspBlock, sizeof(*PspBlock));

    /* Set the exit interrupt */
    PspBlock->Exit[0] = 0xCD; // int 0x20
    PspBlock->Exit[1] = 0x20;

    /* Set the number of the last paragraph */
    PspBlock->LastParagraph = PspSegment + ProgramSize - 1;

    /* Save the interrupt vectors */
    PspBlock->TerminateAddress = IntVecTable[0x22];
    PspBlock->BreakAddress     = IntVecTable[0x23];
    PspBlock->CriticalAddress  = IntVecTable[0x24];

    /* Set the parent PSP */
    PspBlock->ParentPsp = CurrentPsp;

    /* Copy the parent handle table */
    DosCopyHandleTable(PspBlock->HandleTable);

    /* Set the environment block */
    PspBlock->EnvBlock = Environment;

    /* Set the handle table pointers to the internal handle table */
    PspBlock->HandleTableSize = 20;
    PspBlock->HandleTablePtr = MAKELONG(0x18, PspSegment);

    /* Set the DOS version */
    PspBlock->DosVersion = DOS_VERSION;

    /* Set the far call opcodes */
    PspBlock->FarCall[0] = 0xCD; // int 0x21
    PspBlock->FarCall[1] = 0x21;
    PspBlock->FarCall[2] = 0xCB; // retf

    /* Set the command line */
    PspBlock->CommandLineSize = (BYTE)min(strlen(CommandLine), DOS_CMDLINE_LENGTH - 1);
    RtlCopyMemory(PspBlock->CommandLine, CommandLine, PspBlock->CommandLineSize);
    PspBlock->CommandLine[PspBlock->CommandLineSize] = '\r';
}

DWORD DosLoadExecutable(IN DOS_EXEC_TYPE LoadType,
                        IN LPCSTR ExecutablePath,
                        IN LPCSTR CommandLine,
                        IN LPCSTR Environment OPTIONAL,
                        OUT PDWORD StackLocation OPTIONAL,
                        OUT PDWORD EntryPoint OPTIONAL)
{
    DWORD Result = ERROR_SUCCESS;
    HANDLE FileHandle = INVALID_HANDLE_VALUE, FileMapping = NULL;
    LPBYTE Address = NULL;
    WORD Segment = 0;
    WORD EnvBlock = 0;
    WORD MaxAllocSize;
    DWORD i, FileSize, ExeSize;
    PIMAGE_DOS_HEADER Header;
    PDWORD RelocationTable;
    PWORD RelocWord;
    LPSTR CmdLinePtr = (LPSTR)CommandLine;

    DPRINT1("DosLoadExecutable(%d, %s, %s, %s, 0x%08X, 0x%08X)\n",
            LoadType,
            ExecutablePath,
            CommandLine,
            Environment ? Environment : "n/a",
            StackLocation,
            EntryPoint);

    if (LoadType == DOS_LOAD_OVERLAY)
    {
        DPRINT1("Overlay loading is not supported yet.\n");
        return ERROR_NOT_SUPPORTED;
    }

    /* NULL-terminate the command line by removing the return carriage character */
    while (*CmdLinePtr && *CmdLinePtr != '\r') CmdLinePtr++;
    *CmdLinePtr = '\0';

    /* Open a handle to the executable */
    FileHandle = CreateFileA(ExecutablePath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Get the file size */
    FileSize = GetFileSize(FileHandle, NULL);

    /* Create a mapping object for the file */
    FileMapping = CreateFileMapping(FileHandle,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);
    if (FileMapping == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Map the file into memory */
    Address = (LPBYTE)MapViewOfFile(FileMapping, FILE_MAP_READ, 0, 0, 0);
    if (Address == NULL)
    {
        Result = GetLastError();
        goto Cleanup;
    }

    /* Copy the environment block to DOS memory */
    EnvBlock = DosCopyEnvironmentBlock(Environment, ExecutablePath);
    if (EnvBlock == 0)
    {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    /* Check if this is an EXE file or a COM file */
    if (Address[0] == 'M' && Address[1] == 'Z')
    {
        /* EXE file */

        /* Get the MZ header */
        Header = (PIMAGE_DOS_HEADER)Address;

        /* Get the base size of the file, in paragraphs (rounded up) */
        ExeSize = (((Header->e_cp - 1) * 512) + Header->e_cblp + 0x0F) >> 4;

        /* Add the PSP size, in paragraphs */
        ExeSize += sizeof(DOS_PSP) >> 4;

        /* Add the maximum size that should be allocated */
        ExeSize += Header->e_maxalloc;

        /* Make sure it does not pass 0xFFFF */
        if (ExeSize > 0xFFFF) ExeSize = 0xFFFF;

        /* Try to allocate that much memory */
        Segment = DosAllocateMemory((WORD)ExeSize, &MaxAllocSize);

        if (Segment == 0)
        {
            /* Check if there's at least enough memory for the minimum size */
            if (MaxAllocSize < (ExeSize - Header->e_maxalloc + Header->e_minalloc))
            {
                Result = DosLastError;
                goto Cleanup;
            }

            /* Allocate that minimum amount */
            ExeSize = MaxAllocSize;
            Segment = DosAllocateMemory((WORD)ExeSize, NULL);
            ASSERT(Segment != 0);
        }

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         (WORD)ExeSize,
                         EnvBlock);

        /* The process owns its own memory */
        DosChangeMemoryOwner(Segment, Segment);
        DosChangeMemoryOwner(EnvBlock, Segment);

        /* Copy the program to Segment:0100 */
        RtlCopyMemory(SEG_OFF_TO_PTR(Segment, 0x100),
                      Address + (Header->e_cparhdr << 4),
                      min(FileSize - (Header->e_cparhdr << 4),
                          (ExeSize << 4) - sizeof(DOS_PSP)));

        /* Get the relocation table */
        RelocationTable = (PDWORD)(Address + Header->e_lfarlc);

        /* Perform relocations */
        for (i = 0; i < Header->e_crlc; i++)
        {
            /* Get a pointer to the word that needs to be patched */
            RelocWord = (PWORD)SEG_OFF_TO_PTR(Segment + HIWORD(RelocationTable[i]),
                                                0x100 + LOWORD(RelocationTable[i]));

            /* Add the number of the EXE segment to it */
            *RelocWord += Segment + (sizeof(DOS_PSP) >> 4);
        }

        if (LoadType == DOS_LOAD_AND_EXECUTE)
        {
            /* Set the initial segment registers */
            setDS(Segment);
            setES(Segment);

            /* Set the stack to the location from the header */
            setSS(Segment + (sizeof(DOS_PSP) >> 4) + Header->e_ss);
            setSP(Header->e_sp);

            /* Execute */
            CurrentPsp = Segment;
            DiskTransferArea = MAKELONG(0x80, Segment);
            CpuExecute(Segment + Header->e_cs + (sizeof(DOS_PSP) >> 4),
                       Header->e_ip);
        }
    }
    else
    {
        /* COM file */

        /* Find the maximum amount of memory that can be allocated */
        DosAllocateMemory(0xFFFF, &MaxAllocSize);

        /* Make sure it's enough for the whole program and the PSP */
        if (((DWORD)MaxAllocSize << 4) < (FileSize + sizeof(DOS_PSP)))
        {
            Result = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        /* Allocate all of it */
        Segment = DosAllocateMemory(MaxAllocSize, NULL);
        if (Segment == 0)
        {
            Result = DosLastError;
            goto Cleanup;
        }

        /* The process owns its own memory */
        DosChangeMemoryOwner(Segment, Segment);
        DosChangeMemoryOwner(EnvBlock, Segment);

        /* Copy the program to Segment:0100 */
        RtlCopyMemory(SEG_OFF_TO_PTR(Segment, 0x100),
                      Address,
                      FileSize);

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         MaxAllocSize,
                         EnvBlock);

        if (LoadType == DOS_LOAD_AND_EXECUTE)
        {
            /* Set the initial segment registers */
            setDS(Segment);
            setES(Segment);

            /* Set the stack to the last word of the segment */
            setSS(Segment);
            setSP(0xFFFE);

            /*
             * Set the value on the stack to 0, so that a near return
             * jumps to PSP:0000 which has the exit code.
             */
            *((LPWORD)SEG_OFF_TO_PTR(Segment, 0xFFFE)) = 0;

            /* Execute */
            CurrentPsp = Segment;
            DiskTransferArea = MAKELONG(0x80, Segment);
            CpuExecute(Segment, 0x100);
        }
    }

Cleanup:
    if (Result != ERROR_SUCCESS)
    {
        /* It was not successful, cleanup the DOS memory */
        if (EnvBlock) DosFreeMemory(EnvBlock);
        if (Segment) DosFreeMemory(Segment);
    }

    /* Unmap the file*/
    if (Address != NULL) UnmapViewOfFile(Address);

    /* Close the file mapping object */
    if (FileMapping != NULL) CloseHandle(FileMapping);

    /* Close the file handle */
    if (FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);

    return Result;
}

DWORD DosStartProcess(IN LPCSTR ExecutablePath,
                      IN LPCSTR CommandLine,
                      IN LPCSTR Environment OPTIONAL)
{
    DWORD Result;

    Result = DosLoadExecutable(DOS_LOAD_AND_EXECUTE,
                               ExecutablePath,
                               CommandLine,
                               Environment,
                               NULL,
                               NULL);

    if (Result != ERROR_SUCCESS) goto Quit;

    /* Attach to the console */
    VidBiosAttachToConsole(); // FIXME: And in fact, attach the full NTVDM UI to the console

    // HACK: Simulate a ENTER key release scancode on the PS/2 port because
    // some apps expect to read a key release scancode (> 0x80) when they
    // are started.
    IOWriteB(PS2_CONTROL_PORT, 0xD2);     // Next write is for the first PS/2 port
    IOWriteB(PS2_DATA_PORT, 0x80 | 0x1C); // ENTER key release

    /* Start simulation */
    SetEvent(VdmTaskEvent);
    CpuSimulate();

    /* Detach from the console */
    VidBiosDetachFromConsole(); // FIXME: And in fact, detach the full NTVDM UI from the console

Quit:
    return Result;
}

#ifndef STANDALONE
WORD DosCreateProcess(DOS_EXEC_TYPE LoadType,
                      LPCSTR ProgramName,
                      PDOS_EXEC_PARAM_BLOCK Parameters)
{
    DWORD Result;
    DWORD BinaryType;
    LPVOID Environment = NULL;
    VDM_COMMAND_INFO CommandInfo;
    CHAR CmdLine[MAX_PATH];
    CHAR AppName[MAX_PATH];
    CHAR PifFile[MAX_PATH];
    CHAR Desktop[MAX_PATH];
    CHAR Title[MAX_PATH];
    CHAR Env[MAX_PATH];
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    /* Get the binary type */
    if (!GetBinaryTypeA(ProgramName, &BinaryType)) return GetLastError();

    /* Did the caller specify an environment segment? */
    if (Parameters->Environment)
    {
        /* Yes, use it instead of the parent one */
        Environment = SEG_OFF_TO_PTR(Parameters->Environment, 0);
    }

    /* Set up the startup info structure */
    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    /* Create the process */
    if (!CreateProcessA(ProgramName,
                        FAR_POINTER(Parameters->CommandLine),
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        Environment,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo))
    {
        return GetLastError();
    }

    /* Check the type of the program */
    switch (BinaryType)
    {
        /* These are handled by NTVDM */
        case SCS_DOS_BINARY:
        case SCS_WOW_BINARY:
        {
            /* Clear the structure */
            RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

            /* Initialize the structure members */
            CommandInfo.TaskId = SessionId;
            CommandInfo.VDMState = VDM_FLAG_NESTED_TASK | VDM_FLAG_DONT_WAIT;
            CommandInfo.CmdLine = CmdLine;
            CommandInfo.CmdLen = sizeof(CmdLine);
            CommandInfo.AppName = AppName;
            CommandInfo.AppLen = sizeof(AppName);
            CommandInfo.PifFile = PifFile;
            CommandInfo.PifLen = sizeof(PifFile);
            CommandInfo.Desktop = Desktop;
            CommandInfo.DesktopLen = sizeof(Desktop);
            CommandInfo.Title = Title;
            CommandInfo.TitleLen = sizeof(Title);
            CommandInfo.Env = Env;
            CommandInfo.EnvLen = sizeof(Env);

            /* Get the VDM command information */
            if (!GetNextVDMCommand(&CommandInfo))
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }

            /* Increment the re-entry count */
            CommandInfo.VDMState = VDM_INC_REENTER_COUNT;
            GetNextVDMCommand(&CommandInfo);

            /* Load the executable */
            Result = DosLoadExecutable(LoadType,
                                       AppName,
                                       CmdLine,
                                       Env,
                                       &Parameters->StackLocation,
                                       &Parameters->EntryPoint);
            if (Result != ERROR_SUCCESS)
            {
                DisplayMessage(L"Could not load '%S'. Error: %u", AppName, Result);
                // FIXME: Decrement the reenter count. Or, instead, just increment
                // the VDM reenter count *only* if this call succeeds...
            }

            break;
        }

        /* Not handled by NTVDM */
        default:
        {
            /* Wait for the process to finish executing */
            WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        }
    }

    /* Close the handles */
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    return ERROR_SUCCESS;
}
#endif

VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode, WORD KeepResident)
{
    WORD i;
    WORD McbSegment = FIRST_MCB_SEGMENT;
    PDOS_MCB CurrentMcb;
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Psp);

    DPRINT("DosTerminateProcess: Psp 0x%04X, ReturnCode 0x%02X\n",
           Psp,
           ReturnCode);

    /* Check if this PSP is it's own parent */
    if (PspBlock->ParentPsp == Psp) goto Done;

    if (KeepResident == 0)
    {
        for (i = 0; i < PspBlock->HandleTableSize; i++)
        {
            /* Close the handle */
            DosCloseHandle(i);
        }
    }

    /* Free the memory used by the process */
    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(McbSegment);

        /* Make sure the MCB is valid */
        if (CurrentMcb->BlockType != 'M' && CurrentMcb->BlockType != 'Z') break;

        /* Check if this block was allocated by the process */
        if (CurrentMcb->OwnerPsp == Psp)
        {
            if (KeepResident == 0)
            {
                /* Free this entire block */
                DosFreeMemory(McbSegment + 1);
            }
            else if (KeepResident < CurrentMcb->Size)
            {
                /* Reduce the size of the block */
                DosResizeMemory(McbSegment + 1, KeepResident, NULL);

                /* No further paragraphs need to stay resident */
                KeepResident = 0;
            }
            else
            {
                /* Just reduce the amount of paragraphs we need to keep resident */
                KeepResident -= CurrentMcb->Size;
            }
        }

        /* If this was the last block, quit */
        if (CurrentMcb->BlockType == 'Z') break;

        /* Update the segment and continue */
        McbSegment += CurrentMcb->Size + 1;
    }

Done:
    /* Restore the interrupt vectors */
    IntVecTable[0x22] = PspBlock->TerminateAddress;
    IntVecTable[0x23] = PspBlock->BreakAddress;
    IntVecTable[0x24] = PspBlock->CriticalAddress;

    /* Update the current PSP */
    if (Psp == CurrentPsp)
    {
        CurrentPsp = PspBlock->ParentPsp;
        if (CurrentPsp == SYSTEM_PSP)
        {
            ResetEvent(VdmTaskEvent);
            CpuUnsimulate();
        }
    }

#ifndef STANDALONE
    // FIXME: This is probably not the best way to do it
    /* Check if this was a nested DOS task */
    if (CurrentPsp != SYSTEM_PSP)
    {
        VDM_COMMAND_INFO CommandInfo;

        /* Decrement the re-entry count */
        CommandInfo.TaskId = SessionId;
        CommandInfo.VDMState = VDM_DEC_REENTER_COUNT;
        GetNextVDMCommand(&CommandInfo);

        /* Clear the structure */
        RtlZeroMemory(&CommandInfo, sizeof(CommandInfo));

        /* Update the VDM state of the task */
        CommandInfo.TaskId = SessionId;
        CommandInfo.VDMState = VDM_FLAG_DONT_WAIT;
        GetNextVDMCommand(&CommandInfo);
    }
#endif

    /* Save the return code - Normal termination */
    DosErrorLevel = MAKEWORD(ReturnCode, 0x00);

    /* Return control to the parent process */
    CpuExecute(HIWORD(PspBlock->TerminateAddress),
               LOWORD(PspBlock->TerminateAddress));
}

BOOLEAN DosHandleIoctl(BYTE ControlCode, WORD FileHandle)
{
    PDOS_SFT_ENTRY SftEntry = DosGetSftEntry(FileHandle);
    PDOS_DEVICE_NODE Node = NULL;

    /* Make sure it exists */
    if (!SftEntry)
    {
        DosLastError = ERROR_FILE_NOT_FOUND;
        return FALSE;
    }

    if (SftEntry->Type == DOS_SFT_ENTRY_DEVICE) Node = SftEntry->DeviceNode;

    switch (ControlCode)
    {
        /* Get Device Information */
        case 0x00:
        {
            WORD InfoWord = 0;

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2820.htm
             * for a list of possible flags.
             */

            if (Node)
            {
                /* Return the device attributes with bit 7 set */
                InfoWord = Node->DeviceAttributes | (1 << 7);
            }

            setDX(InfoWord);
            return TRUE;
        }

        /* Set Device Information */
        case 0x01:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;

            return FALSE;
        }

        /* Read From Device I/O Control Channel */
        case 0x02:
        {
            WORD Length = getCX();

            if (Node == NULL || !(Node->DeviceAttributes & DOS_DEVATTR_IOCTL))
            {
                DosLastError = ERROR_INVALID_FUNCTION;
                return FALSE;
            }

            /* Do nothing if there is no IOCTL routine */
            if (!Node->IoctlReadRoutine)
            {
                setAX(0);
                return TRUE;
            }

            Node->IoctlReadRoutine(Node, MAKELONG(getDX(), getDS()), &Length);

            setAX(Length);
            return TRUE;
        }

        /* Write To Device I/O Control Channel */
        case 0x03:
        {
            WORD Length = getCX();

            if (Node == NULL || !(Node->DeviceAttributes & DOS_DEVATTR_IOCTL))
            {
                DosLastError = ERROR_INVALID_FUNCTION;
                return FALSE;
            }

            /* Do nothing if there is no IOCTL routine */
            if (!Node->IoctlWriteRoutine)
            {
                setAX(0);
                return TRUE;
            }

            Node->IoctlWriteRoutine(Node, MAKELONG(getDX(), getDS()), &Length);

            setAX(Length);
            return TRUE;
        }

        /* Unsupported control code */
        default:
        {
            DPRINT1("Unsupported IOCTL: 0x%02X\n", ControlCode);

            DosLastError = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
    }
}

VOID WINAPI DosInt20h(LPWORD Stack)
{
    /* This is the exit interrupt */
    DosTerminateProcess(Stack[STACK_CS], 0, 0);
}

VOID WINAPI DosInt21h(LPWORD Stack)
{
    BYTE Character;
    SYSTEMTIME SystemTime;
    PCHAR String;
    PDOS_INPUT_BUFFER InputBuffer;
    PDOS_COUNTRY_CODE_BUFFER CountryCodeBuffer;
    INT Return;

    /* Check the value in the AH register */
    switch (getAH())
    {
        /* Terminate Program */
        case 0x00:
        {
            DosTerminateProcess(Stack[STACK_CS], 0, 0);
            break;
        }

        /* Read Character from STDIN with Echo */
        case 0x01:
        {
            DPRINT("INT 21h, AH = 01h\n");

            // FIXME: Under DOS 2+, input / output handle may be redirected!!!!
            DoEcho = TRUE;
            Character = DosReadCharacter(DOS_INPUT_HANDLE);
            DoEcho = FALSE;

            // FIXME: Check whether Ctrl-C / Ctrl-Break is pressed, and call INT 23h if so.
            // Check also Ctrl-P and set echo-to-printer flag.
            // Ctrl-Z is not interpreted.

            setAL(Character);
            break;
        }

        /* Write Character to STDOUT */
        case 0x02:
        {
            // FIXME: Under DOS 2+, output handle may be redirected!!!!
            Character = getDL();
            DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

            /*
             * We return the output character (DOS 2.1+).
             * Also, if we're going to output a TAB, then
             * don't return a TAB but a SPACE instead.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2554.htm
             * for more information.
             */
            setAL(Character == '\t' ? ' ' : Character);
            break;
        }

        /* Read Character from STDAUX */
        case 0x03:
        {
            // FIXME: Really read it from STDAUX!
            DPRINT1("INT 16h, 03h: Read character from STDAUX is HALFPLEMENTED\n");
            // setAL(DosReadCharacter());
            break;
        }

        /* Write Character to STDAUX */
        case 0x04:
        {
            // FIXME: Really write it to STDAUX!
            DPRINT1("INT 16h, 04h: Write character to STDAUX is HALFPLEMENTED\n");
            // DosPrintCharacter(getDL());
            break;
        }

        /* Write Character to Printer */
        case 0x05:
        {
            // FIXME: Really write it to printer!
            DPRINT1("INT 16h, 05h: Write character to printer is HALFPLEMENTED -\n\n");
            DPRINT1("0x%p\n", getDL());
            DPRINT1("\n\n-----------\n\n");
            break;
        }

        /* Direct Console I/O */
        case 0x06:
        {
            Character = getDL();

            // FIXME: Under DOS 2+, output handle may be redirected!!!!

            if (Character != 0xFF)
            {
                /* Output */
                DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

                /*
                 * We return the output character (DOS 2.1+).
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2558.htm
                 * for more information.
                 */
                setAL(Character);
            }
            else
            {
                /* Input */
                if (DosCheckInput())
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                    setAL(DosReadCharacter(DOS_INPUT_HANDLE));
                }
                else
                {
                    /* No character available */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
                    setAL(0x00);
                }
            }

            break;
        }

        /* Character Input without Echo */
        case 0x07:
        case 0x08:
        {
            DPRINT("Char input without echo\n");

            // FIXME: Under DOS 2+, input handle may be redirected!!!!
            Character = DosReadCharacter(DOS_INPUT_HANDLE);

            // FIXME: For 0x07, do not check Ctrl-C/Break.
            //        For 0x08, do check those control sequences and if needed,
            //        call INT 0x23.

            // /* Let the BOP repeat if needed */
            // if (getCF()) break;

            setAL(Character);
            break;
        }

        /* Write string to STDOUT */
        case 0x09:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            while (*String != '$')
            {
                DosPrintCharacter(DOS_OUTPUT_HANDLE, *String);
                String++;
            }

            /*
             * We return the terminating character (DOS 2.1+).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2562.htm
             * for more information.
             */
            setAL('$'); // *String
            break;
        }

        /* Read Buffered Input */
        case 0x0A:
        {
            WORD Count = 0;
            InputBuffer = (PDOS_INPUT_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            DPRINT("Read Buffered Input\n");

            while (Count < InputBuffer->MaxLength)
            {
                // FIXME!! This function should interpret backspaces etc...

                /* Try to read a character (wait) */
                Character = DosReadCharacter(DOS_INPUT_HANDLE);

                // FIXME: Check whether Ctrl-C / Ctrl-Break is pressed, and call INT 23h if so.

                /* Echo the character and append it to the buffer */
                DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);
                InputBuffer->Buffer[Count] = Character;

                Count++; /* Carriage returns are also counted */

                if (Character == '\r') break;
            }

            /* Update the length */
            InputBuffer->Length = Count;

            break;
        }

        /* Get STDIN Status */
        case 0x0B:
        {
            setAL(DosCheckInput() ? 0xFF : 0x00);
            break;
        }

        /* Flush Buffer and Read STDIN */
        case 0x0C:
        {
            BYTE InputFunction = getAL();

            /* Flush STDIN buffer */
            DosFlushFileBuffers(DOS_INPUT_HANDLE);

            /*
             * If the input function number contained in AL is valid, i.e.
             * AL == 0x01 or 0x06 or 0x07 or 0x08 or 0x0A, call ourselves
             * recursively with AL == AH.
             */
            if (InputFunction == 0x01 || InputFunction == 0x06 ||
                InputFunction == 0x07 || InputFunction == 0x08 ||
                InputFunction == 0x0A)
            {
                /* Call ourselves recursively */
                setAH(InputFunction);
                DosInt21h(Stack);
            }
            break;
        }

        /* Disk Reset */
        case 0x0D:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(CurrentPsp);

            // TODO: Flush what's needed.
            DPRINT1("INT 21h, 0Dh is UNIMPLEMENTED\n");

            /* Clear CF in DOS 6 only */
            if (PspBlock->DosVersion == 0x0006)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        /* Set Default Drive  */
        case 0x0E:
        {
            DosChangeDrive(getDL());
            setAL(LastDrive - 'A' + 1);
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x18:
        {
            /*
             * This function corresponds to the CP/M BDOS function
             * "get bit map of logged drives", which is meaningless
             * under MS-DOS.
             *
             * For: PTS-DOS 6.51 & S/DOS 1.0 - EXTENDED RENAME FILE USING FCB
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2584.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* Get Default Drive */
        case 0x19:
        {
            setAL(CurrentDrive);
            break;
        }

        /* Set Disk Transfer Area */
        case 0x1A:
        {
            DiskTransferArea = MAKELONG(getDX(), getDS());
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x1D:
        case 0x1E:
        {
            /*
             * Function 0x1D corresponds to the CP/M BDOS function
             * "get bit map of read-only drives", which is meaningless
             * under MS-DOS.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2592.htm
             * for more information.
             *
             * Function 0x1E corresponds to the CP/M BDOS function
             * "set file attributes", which was meaningless under MS-DOS 1.x.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2593.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* NULL Function for CP/M Compatibility */
        case 0x20:
        {
            /*
             * This function corresponds to the CP/M BDOS function
             * "get/set default user (sublibrary) number", which is meaningless
             * under MS-DOS.
             *
             * For: S/DOS 1.0+ & PTS-DOS 6.51+ - GET OEM REVISION
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2596.htm
             * for more information.
             */
            setAL(0x00);
            break;
        }

        /* Set Interrupt Vector */
        case 0x25:
        {
            ULONG FarPointer = MAKELONG(getDX(), getDS());
            DPRINT1("Setting interrupt 0x%02X to %04X:%04X ...\n",
                    getAL(), HIWORD(FarPointer), LOWORD(FarPointer));

            /* Write the new far pointer to the IDT */
            ((PULONG)BaseAddress)[getAL()] = FarPointer;
            break;
        }

        /* Create New PSP */
        case 0x26:
        {
            DPRINT1("INT 21h, AH = 26h - Create New PSP is UNIMPLEMENTED\n");
            break;
        }

        /* Get System Date */
        case 0x2A:
        {
            GetLocalTime(&SystemTime);
            setCX(SystemTime.wYear);
            setDX(MAKEWORD(SystemTime.wDay, SystemTime.wMonth));
            setAL(SystemTime.wDayOfWeek);
            break;
        }

        /* Set System Date */
        case 0x2B:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wYear  = getCX();
            SystemTime.wMonth = getDH();
            SystemTime.wDay   = getDL();

            /* Return success or failure */
            setAL(SetLocalTime(&SystemTime) ? 0x00 : 0xFF);
            break;
        }

        /* Get System Time */
        case 0x2C:
        {
            GetLocalTime(&SystemTime);
            setCX(MAKEWORD(SystemTime.wMinute, SystemTime.wHour));
            setDX(MAKEWORD(SystemTime.wMilliseconds / 10, SystemTime.wSecond));
            break;
        }

        /* Set System Time */
        case 0x2D:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wHour         = getCH();
            SystemTime.wMinute       = getCL();
            SystemTime.wSecond       = getDH();
            SystemTime.wMilliseconds = getDL() * 10; // In hundredths of seconds

            /* Return success or failure */
            setAL(SetLocalTime(&SystemTime) ? 0x00 : 0xFF);
            break;
        }

        /* Get Disk Transfer Area */
        case 0x2F:
        {
            setES(HIWORD(DiskTransferArea));
            setBX(LOWORD(DiskTransferArea));
            break;
        }

        /* Get DOS Version */
        case 0x30:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(CurrentPsp);

            /*
             * DOS 2+ - GET DOS VERSION
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2711.htm
             * for more information.
             */

            if (LOBYTE(PspBlock->DosVersion) < 5 || getAL() == 0x00)
            {
                /*
                 * Return DOS OEM number:
                 * 0x00 for IBM PC-DOS
                 * 0x02 for packaged MS-DOS
                 * 0xFF for NT DOS
                 */
                setBH(0xFF);
            }

            if (LOBYTE(PspBlock->DosVersion) >= 5 && getAL() == 0x01)
            {
                /*
                 * Return version flag:
                 * 1 << 3 if DOS is in ROM,
                 * 0 (reserved) if not.
                 */
                setBH(0x00);
            }

            /* Return DOS 24-bit user serial number in BL:CX */
            setBL(0x00);
            setCX(0x0000);

            /*
             * Return DOS version: Minor:Major in AH:AL
             * The Windows NT DOS box returns version 5.00, subject to SETVER.
             */
            setAX(PspBlock->DosVersion);

            break;
        }

        /* Terminate and Stay Resident */
        case 0x31:
        {
            DPRINT1("Process going resident: %u paragraphs kept\n", getDX());
            DosTerminateProcess(CurrentPsp, getAL(), getDX());
            break;
        }

        /* Extended functionalities */
        case 0x33:
        {
            if (getAL() == 0x06)
            {
                /*
                 * DOS 5+ - GET TRUE VERSION NUMBER
                 * This function always returns the true version number, unlike
                 * AH=30h, whose return value may be changed with SETVER.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2730.htm
                 * for more information.
                 */

                /*
                 * Return the true DOS version: Minor:Major in BH:BL
                 * The Windows NT DOS box returns BX=3205h (version 5.50).
                 */
                setBX(NTDOS_VERSION);

                /* DOS revision 0 */
                setDL(0x00);

                /* Unpatched DOS */
                setDH(0x00);
            }
            // else
            // {
                // /* Invalid subfunction */
                // setAL(0xFF);
            // }

            break;
        }

        /* Get Interrupt Vector */
        case 0x35:
        {
            DWORD FarPointer = ((PDWORD)BaseAddress)[getAL()];

            /* Read the address from the IDT into ES:BX */
            setES(HIWORD(FarPointer));
            setBX(LOWORD(FarPointer));
            break;
        }

        /* SWITCH character - AVAILDEV */
        case 0x37:
        {
            if (getAL() == 0x00)
            {
                /*
                 * DOS 2+ - "SWITCHAR" - GET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 4.0+.
                 * MS-DOS 5+ always return AL=00h/DL=2Fh.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2752.htm
                 * for more information.
                 */
                setDL('/');
                setAL(0x00);
            }
            else if (getAL() == 0x01)
            {
                /*
                 * DOS 2+ - "SWITCHAR" - SET SWITCH CHARACTER
                 * This setting is ignored by MS-DOS 5+.
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2753.htm
                 * for more information.
                 */
                // getDL();
                setAL(0xFF);
            }
            else if (getAL() == 0x02)
            {
                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                // setDL();
                setAL(0xFF);
            }
            else if (getAL() == 0x03)
            {
                /*
                 * DOS 2.x and 3.3+ only - "AVAILDEV" - SPECIFY \DEV\ PREFIX USE
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2754.htm
                 * for more information.
                 */
                // getDL();
                setAL(0xFF);
            }
            else
            {
                /* Invalid subfunction */
                setAL(0xFF);
            }

            break;
        }

        /* Get/Set Country-dependent Information */
        case 0x38:
        {
            CountryCodeBuffer = (PDOS_COUNTRY_CODE_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            if (getAL() == 0x00)
            {
                /* Get */
                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE,
                                       &CountryCodeBuffer->TimeFormat,
                                       sizeof(CountryCodeBuffer->TimeFormat) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY,
                                       &CountryCodeBuffer->CurrencySymbol,
                                       sizeof(CountryCodeBuffer->CurrencySymbol) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND,
                                       &CountryCodeBuffer->ThousandSep,
                                       sizeof(CountryCodeBuffer->ThousandSep) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Return = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                                       &CountryCodeBuffer->DecimalSep,
                                       sizeof(CountryCodeBuffer->DecimalSep) / sizeof(TCHAR));
                if (Return == 0)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(LOWORD(GetLastError()));
                    break;
                }

                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;
            }

            break;
        }

        /* Create Directory */
        case 0x39:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (CreateDirectoryA(String, NULL))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(LOWORD(GetLastError()));
            }

            break;
        }

        /* Remove Directory */
        case 0x3A:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (RemoveDirectoryA(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(LOWORD(GetLastError()));
            }

            break;
        }

        /* Set Current Directory */
        case 0x3B:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (DosChangeDirectory(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
            }

            break;
        }

        /* Create or Truncate File */
        case 0x3C:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                           CREATE_ALWAYS,
                                           getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Open File or Device */
        case 0x3D:
        {
            WORD FileHandle;
            WORD ErrorCode;
            LPCSTR FileName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            PDOS_DEVICE_NODE Device = DosGetDevice(FileName);

            if (Device)
            {
                FileHandle = DosOpenDevice(Device);
                ErrorCode =  (FileHandle != INVALID_DOS_HANDLE)
                             ? ERROR_SUCCESS : ERROR_TOO_MANY_OPEN_FILES;
            }
            else
            {
                ErrorCode = DosOpenFile(&FileHandle, FileName, getAL());
            }

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Close File or Device */
        case 0x3E:
        {
            if (DosCloseHandle(getBX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Read from File or Device */
        case 0x3F:
        {
            WORD BytesRead = 0;
            WORD ErrorCode;

            DPRINT("DosReadFile(0x%04X)\n", getBX());

            DoEcho = TRUE;
            ErrorCode = DosReadFile(getBX(),
                                    MAKELONG(getDX(), getDS()),
                                    getCX(),
                                    &BytesRead);
            DoEcho = FALSE;

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(BytesRead);
            }
            else if (ErrorCode != ERROR_NOT_READY)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Write to File or Device */
        case 0x40:
        {
            WORD BytesWritten = 0;
            WORD ErrorCode = DosWriteFile(getBX(),
                                          MAKELONG(getDX(), getDS()),
                                          getCX(),
                                          &BytesWritten);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(BytesWritten);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Delete File */
        case 0x41:
        {
            LPSTR FileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (demFileDelete(FileName) == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                /*
                 * See Ralf Brown: http://www.ctyme.com/intr/rb-2797.htm
                 * "AX destroyed (DOS 3.3) AL seems to be drive of deleted file."
                 */
                setAL(FileName[0] - 'A');
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* Seek File */
        case 0x42:
        {
            DWORD NewLocation;
            WORD ErrorCode = DosSeekFile(getBX(),
                                         MAKELONG(getDX(), getCX()),
                                         getAL(),
                                         &NewLocation);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the new offset in DX:AX */
                setDX(HIWORD(NewLocation));
                setAX(LOWORD(NewLocation));
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Get/Set File Attributes */
        case 0x43:
        {
            DWORD Attributes;
            LPSTR FileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());

            if (getAL() == 0x00)
            {
                /* Get the attributes */
                Attributes = GetFileAttributesA(FileName);

                /* Check if it failed */
                if (Attributes == INVALID_FILE_ATTRIBUTES)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
                else
                {
                    /* Return the attributes that DOS can understand */
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    setCX(Attributes & 0x00FF);
                }
            }
            else if (getAL() == 0x01)
            {
                /* Try to set the attributes */
                if (SetFileAttributesA(FileName, getCL()))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* IOCTL */
        case 0x44:
        {
            if (DosHandleIoctl(getAL(), getBX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
            }

            break;
        }

        /* Duplicate Handle */
        case 0x45:
        {
            WORD NewHandle;
            PDOS_SFT_ENTRY SftEntry = DosGetSftEntry(getBX());

            if (SftEntry == NULL || SftEntry->Type == DOS_SFT_ENTRY_NONE) 
            {
                /* The handle is invalid */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
                break;
            }

            /* Open a new handle to the same entry */
            switch (SftEntry->Type)
            {
                case DOS_SFT_ENTRY_WIN32:
                {
                    NewHandle = DosOpenHandle(SftEntry->Handle);
                    break;
                }

                case DOS_SFT_ENTRY_DEVICE:
                {
                    NewHandle = DosOpenDevice(SftEntry->DeviceNode);
                    break;
                }

                default:
                {
                    /* Shouldn't happen */
                    ASSERT(FALSE);
                }
            }

            if (NewHandle == INVALID_DOS_HANDLE)
            {
                /* Too many files open */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_TOO_MANY_OPEN_FILES);
                break;
            }

            /* Return the result */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            setAX(NewHandle);
            break;
        }

        /* Force Duplicate Handle */
        case 0x46:
        {
            if (DosDuplicateHandle(getBX(), getCX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Get Current Directory */
        case 0x47:
        {
            BYTE DriveNumber = getDL();
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getSI());

            /* Get the real drive number */
            if (DriveNumber == 0)
            {
                DriveNumber = CurrentDrive;
            }
            else
            {
                /* Decrement DriveNumber since it was 1-based */
                DriveNumber--;
            }

            if (DriveNumber <= LastDrive - 'A')
            {
                /*
                 * Copy the current directory into the target buffer.
                 * It doesn't contain the drive letter and the backslash.
                 */
                strncpy(String, CurrentDirectories[DriveNumber], DOS_DIR_LENGTH);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(0x0100); // Undocumented, see Ralf Brown: http://www.ctyme.com/intr/rb-2933.htm
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_DRIVE);
            }

            break;
        }

        /* Allocate Memory */
        case 0x48:
        {
            WORD MaxAvailable = 0;
            WORD Segment = DosAllocateMemory(getBX(), &MaxAvailable);

            if (Segment != 0)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(Segment);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
                setBX(MaxAvailable);
            }

            break;
        }

        /* Free Memory */
        case 0x49:
        {
            if (DosFreeMemory(getES()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_ARENA_TRASHED);
            }

            break;
        }

        /* Resize Memory Block */
        case 0x4A:
        {
            WORD Size;

            if (DosResizeMemory(getES(), getBX(), &Size))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
                setBX(Size);
            }

            break;
        }

#ifndef STANDALONE
        /* Execute */
        case 0x4B:
        {
            DOS_EXEC_TYPE LoadType = (DOS_EXEC_TYPE)getAL();
            LPSTR ProgramName = SEG_OFF_TO_PTR(getDS(), getDX());
            PDOS_EXEC_PARAM_BLOCK ParamBlock = SEG_OFF_TO_PTR(getES(), getBX());
            WORD ErrorCode = DosCreateProcess(LoadType, ProgramName, ParamBlock);

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }
#endif

        /* Terminate With Return Code */
        case 0x4C:
        {
            DosTerminateProcess(CurrentPsp, getAL(), 0);
            break;
        }

        /* Get Return Code (ERRORLEVEL) */
        case 0x4D:
        {
            /*
             * According to Ralf Brown: http://www.ctyme.com/intr/rb-2976.htm
             * DosErrorLevel is cleared after being read by this function.
             */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            setAX(DosErrorLevel);
            DosErrorLevel = 0x0000; // Clear it
            break;
        }

        /* Find First File */
        case 0x4E:
        {
            WORD Result = (WORD)demFileFindFirst(FAR_POINTER(DiskTransferArea),
                                                 SEG_OFF_TO_PTR(getDS(), getDX()),
                                                 getCX());

            setAX(Result);

            if (Result == ERROR_SUCCESS)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Find Next File */
        case 0x4F:
        {
            WORD Result = (WORD)demFileFindNext(FAR_POINTER(DiskTransferArea));

            setAX(Result);

            if (Result == ERROR_SUCCESS)
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Internal - Set Current Process ID (Set PSP Address) */
        case 0x50:
        {
            // FIXME: Is it really what it's done ??
            CurrentPsp = getBX();
            break;
        }

        /* Internal - Get Current Process ID (Get PSP Address) */
        case 0x51:
        /* Get Current PSP Address */
        case 0x62:
        {
            /*
             * Undocumented AH=51h is identical to the documented AH=62h.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2982.htm
             * and http://www.ctyme.com/intr/rb-3140.htm
             * for more information.
             */
            setBX(CurrentPsp);
            break;
        }

        /* Internal - Get "List of lists" (SYSVARS) */
        case 0x52:
        {
            /*
             * On return, ES points at the DOS data segment (see also INT 2F/AX=1203h).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2983.htm
             * for more information.
             */

            /* Return the DOS "list of lists" in ES:BX */
            setES(0x0000);
            setBX(0x0000);

            DisplayMessage(L"Required for AARD code, do you remember? :P");
            break;
        }

        /* Rename File */
        case 0x56:
        {
            LPSTR ExistingFileName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            LPSTR NewFileName      = (LPSTR)SEG_OFF_TO_PTR(getES(), getDI());

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2990.htm
             * for more information.
             */

            if (MoveFileA(ExistingFileName, NewFileName))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* Get/Set Memory Management Options */
        case 0x58:
        {
            if (getAL() == 0x00)
            {
                /* Get allocation strategy */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(DosAllocStrategy);
            }
            else if (getAL() == 0x01)
            {
                /* Set allocation strategy */

                if ((getBL() & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                    == (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                {
                    /* Can't set both */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(ERROR_INVALID_PARAMETER);
                    break;
                }

                if ((getBL() & 0x3F) > DOS_ALLOC_LAST_FIT)
                {
                    /* Invalid allocation strategy */
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(ERROR_INVALID_PARAMETER);
                    break;
                }

                DosAllocStrategy = getBL();
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (getAL() == 0x02)
            {
                /* Get UMB link state */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAL(DosUmbLinked ? 0x01 : 0x00);
            }
            else if (getAL() == 0x03)
            {
                /* Set UMB link state */
                if (getBX()) DosLinkUmb();
                else DosUnlinkUmb();
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                /* Invalid or unsupported function */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* Get Extended Error Information */
        case 0x59:
        {
            DPRINT1("INT 21h, AH = 59h, BX = %04Xh - Get Extended Error Information is UNIMPLEMENTED\n",
                    getBX());
            break;
        }

        /* Create Temporary File */
        case 0x5A:
        {
            LPSTR PathName = (LPSTR)SEG_OFF_TO_PTR(getDS(), getDX());
            LPSTR FileName = PathName; // The buffer for the path and the full file name is the same.
            UINT  uRetVal;
            WORD  FileHandle;
            WORD  ErrorCode;

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3014.htm
             * for more information.
             */

            // FIXME: Check for buffer validity?
            // It should be a ASCIZ path ending with a '\' + 13 zero bytes
            // to receive the generated filename.

            /* First create the temporary file */
            uRetVal = GetTempFileNameA(PathName, NULL, 0, FileName);
            if (uRetVal == 0)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
                break;
            }

            /* Now try to open it in read/write access */
            ErrorCode = DosOpenFile(&FileHandle, FileName, 2);
            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Create New File */
        case 0x5B:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                           CREATE_NEW,
                                           getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Lock/Unlock Region of File */
        case 0x5C:
        {
            PDOS_SFT_ENTRY SftEntry = DosGetSftEntry(getBX());

            if (SftEntry == NULL || SftEntry->Type != DOS_SFT_ENTRY_WIN32)
            {
                /* The handle is invalid */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
                break;
            }

            if (getAL() == 0x00)
            {
                /* Lock region of file */
                if (LockFile(SftEntry->Handle,
                             MAKELONG(getCX(), getDX()), 0,
                             MAKELONG(getSI(), getDI()), 0))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
            }
            else if (getAL() == 0x01)
            {
                /* Unlock region of file */
                if (UnlockFile(SftEntry->Handle,
                               MAKELONG(getCX(), getDX()), 0,
                               MAKELONG(getSI(), getDI()), 0))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    setAX(GetLastError());
                }
            }
            else
            {
                /* Invalid subfunction */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* Canonicalize File Name or Path */
        case 0x60:
        {
            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3137.htm
             * for more information.
             */

            /*
             * We suppose that the DOS app gave to us a valid
             * 128-byte long buffer for the canonicalized name.
             */
            DWORD dwRetVal = GetFullPathNameA(SEG_OFF_TO_PTR(getDS(), getSI()),
                                              128,
                                              SEG_OFF_TO_PTR(getES(), getDI()),
                                              NULL);
            if (dwRetVal == 0)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }
            else
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setAX(0x0000);
            }

            // FIXME: Convert the full path name into short version.
            // We cannot reliably use GetShortPathName, because it fails
            // if the path name given doesn't exist. However this DOS
            // function AH=60h should be able to work even for non-existing
            // path and file names.

            break;
        }

        /* Set Handle Count */
        case 0x67:
        {
            if (!DosResizeHandleTable(getBX()))
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(DosLastError);
            }
            else Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        /* Commit File */
        case 0x68:
        case 0x6A:
        {
            /*
             * Function 6Ah is identical to function 68h,
             * and sets AH to 68h if success.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3176.htm
             * for more information.
             */
            setAH(0x68);

            if (DosFlushFileBuffers(getBX()))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(GetLastError());
            }

            break;
        }

        /* Extended Open/Create */
        case 0x6C:
        {
            WORD FileHandle;
            WORD CreationStatus;
            WORD ErrorCode;

            /* Check for AL == 00 */
            if (getAL() != 0x00)
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_FUNCTION);
                break;
            }

            /*
             * See Ralf Brown: http://www.ctyme.com/intr/rb-3179.htm
             * for the full detailed description.
             *
             * WARNING: BH contains some extended flags that are NOT SUPPORTED.
             */

            ErrorCode = DosCreateFileEx(&FileHandle,
                                        &CreationStatus,
                                        (LPCSTR)SEG_OFF_TO_PTR(getDS(), getSI()),
                                        getBL(),
                                        getDL(),
                                        getCX());

            if (ErrorCode == ERROR_SUCCESS)
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                setCX(CreationStatus);
                setAX(FileHandle);
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ErrorCode);
            }

            break;
        }

        /* Unsupported */
        default:
        {
            DPRINT1("DOS Function INT 0x21, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());

            setAL(0); // Some functions expect AL to be 0 when it's not supported.
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

VOID WINAPI DosBreakInterrupt(LPWORD Stack)
{
    UNREFERENCED_PARAMETER(Stack);

    /* Stop the VDM task */
    ResetEvent(VdmTaskEvent);
    CpuUnsimulate();
}

VOID WINAPI DosFastConOut(LPWORD Stack)
{
    /*
     * This is the DOS 2+ Fast Console Output Interrupt.
     * The default handler under DOS 2.x and 3.x simply calls INT 10h/AH=0Eh.
     *
     * See Ralf Brown: http://www.ctyme.com/intr/rb-4124.htm
     * for more information.
     */

    /* Save AX and BX */
    USHORT AX = getAX();
    USHORT BX = getBX();

    /*
     * Set the parameters:
     * AL contains the character to print (already set),
     * BL contains the character attribute,
     * BH contains the video page to use.
     */
    setBL(DOS_CHAR_ATTRIBUTE);
    setBH(Bda->VideoPage);

    /* Call the BIOS INT 10h, AH=0Eh "Teletype Output" */
    setAH(0x0E);
    Int32Call(&DosContext, BIOS_VIDEO_INTERRUPT);

    /* Restore AX and BX */
    setBX(BX);
    setAX(AX);
}

VOID WINAPI DosInt2Fh(LPWORD Stack)
{
    DPRINT1("DOS Internal System Function INT 0x2F, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
            getAH(), getAL());
    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
}

BOOLEAN DosKRNLInitialize(VOID)
{

#if 1

    UCHAR i;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];
    LPSTR Path;

    FILE *Stream;
    WCHAR Buffer[256];

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
        while (fgetws(Buffer, 256, Stream))
        {
            // TODO: Parse the line
        }
        fclose(Stream);
    }

    /* Initialize the SFT */
    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        DosSystemFileTable[i].Type     = DOS_SFT_ENTRY_NONE;
        DosSystemFileTable[i].RefCount = 0;
    }

    /* Load the EMS driver */
    EmsDrvInitialize();

    /* Load the CON driver */
    ConDrvInitialize();

#endif

    /* Initialize the callback context */
    InitializeContext(&DosContext, 0x0070, 0x0000);

    /* Register the DOS 32-bit Interrupts */
    RegisterDosInt32(0x20, DosInt20h        );
    RegisterDosInt32(0x21, DosInt21h        );
//  RegisterDosInt32(0x22, DosInt22h        ); // Termination
    RegisterDosInt32(0x23, DosBreakInterrupt); // Ctrl-C / Ctrl-Break
//  RegisterDosInt32(0x24, DosInt24h        ); // Critical Error
    RegisterDosInt32(0x29, DosFastConOut    ); // DOS 2+ Fast Console Output
    RegisterDosInt32(0x2F, DosInt2Fh        );

    return TRUE;
}

/* EOF */
