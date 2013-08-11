/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos.c
 * PURPOSE:         VDM DOS Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "dos.h"
#include "bios.h"
#include "emulator.h"

/* PRIVATE VARIABLES **********************************************************/

static WORD CurrentPsp = SYSTEM_PSP;
static WORD DosLastError = 0;
static DWORD DiskTransferArea;
static BYTE CurrentDrive;
static CHAR LastDrive = 'E';
static CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];
static HANDLE DosSystemFileTable[DOS_SFT_SIZE];
static WORD DosSftRefCount[DOS_SFT_SIZE];
static BYTE DosAllocStrategy = DOS_ALLOC_BEST_FIT;
static BOOLEAN DosUmbLinked = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID DosCombineFreeBlocks(WORD StartBlock)
{
    PDOS_MCB CurrentMcb = SEGMENT_TO_MCB(StartBlock), NextMcb;

    /* If this is the last block or it's not free, quit */
    if (CurrentMcb->BlockType == 'Z' || CurrentMcb->OwnerPsp != 0) return;

    while (TRUE)
    {
        /* Get a pointer to the next MCB */
        NextMcb = SEGMENT_TO_MCB(StartBlock + CurrentMcb->Size + 1);

        /* Check if the next MCB is free */
        if (NextMcb->OwnerPsp == 0)
        {
            /* Combine them */
            CurrentMcb->Size += NextMcb->Size + 1;
            CurrentMcb->BlockType = NextMcb->BlockType;
            NextMcb->BlockType = 'I';
        }
        else
        {
            /* No more adjoining free blocks */
            break;
        }
    }
}

static WORD DosCopyEnvironmentBlock(WORD SourceSegment, LPCSTR ProgramName)
{
    PCHAR Ptr, SourceBuffer, DestBuffer = NULL;
    ULONG TotalSize = 0;
    WORD DestSegment;

    Ptr = SourceBuffer = (PCHAR)((ULONG_PTR)BaseAddress + TO_LINEAR(SourceSegment, 0));

    /* Calculate the size of the environment block */
    while (*Ptr)
    {
        TotalSize += strlen(Ptr) + 1;
        Ptr += strlen(Ptr) + 1;
    }
    TotalSize++;

    /* Add the string buffer size */
    TotalSize += strlen(ProgramName) + 1;

    /* Allocate the memory for the environment block */
    DestSegment = DosAllocateMemory((WORD)((TotalSize + 0x0F) >> 4), NULL);
    if (!DestSegment) return 0;

    Ptr = SourceBuffer;

    DestBuffer = (PCHAR)((ULONG_PTR)BaseAddress + TO_LINEAR(DestSegment, 0));
    while (*Ptr)
    {
        /* Copy the string */
        strcpy(DestBuffer, Ptr);

        /* Advance to the next string */
        DestBuffer += strlen(Ptr);
        Ptr += strlen(Ptr) + 1;

        /* Put a zero after the string */
        *(DestBuffer++) = 0;
    }

    /* Set the final zero */
    *(DestBuffer++) = 0;

    /* Copy the program name after the environment block */
    strcpy(DestBuffer, ProgramName);

    return DestSegment;
}

static VOID DosChangeMemoryOwner(WORD Segment, WORD NewOwner)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment - 1);

    /* Just set the owner */
    Mcb->OwnerPsp = NewOwner;
}

static WORD DosOpenHandle(HANDLE Handle)
{
    BYTE i;
    WORD DosHandle;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

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
    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        /* Check if this is the same handle */
        if (DosSystemFileTable[i] != Handle) continue;

        /* Already in the table, reference it */
        DosSftRefCount[i]++;

        /* Set the JFT entry to that SFT index */
        HandleTable[DosHandle] = i;

        /* Return the new handle */
        return DosHandle;
    }

    /* Add the handle to the SFT */
    for (i = 0; i < DOS_SFT_SIZE; i++)
    {
        /* Make sure this is an empty table entry */
        if (DosSystemFileTable[i] != INVALID_HANDLE_VALUE) continue;

        /* Initialize the empty table entry */
        DosSystemFileTable[i] = Handle;
        DosSftRefCount[i] = 1;

        /* Set the JFT entry to that SFT index */
        HandleTable[DosHandle] = i;

        /* Return the new handle */
        return DosHandle;
    }

    /* The SFT is full */
    return INVALID_DOS_HANDLE;
}

static HANDLE DosGetRealHandle(WORD DosHandle)
{
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return INVALID_HANDLE_VALUE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the handle is open */
    if (HandleTable[DosHandle] == 0xFF) return INVALID_HANDLE_VALUE;

    /* Return the Win32 handle */
    return DosSystemFileTable[HandleTable[DosHandle]];
}

static VOID DosCopyHandleTable(LPBYTE DestinationTable)
{
    INT i;
    PDOS_PSP PspBlock;
    LPBYTE SourceTable;

    /* Clear the table first */
    for (i = 0; i < 20; i++) DestinationTable[i] = 0xFF;

    /* Check if this is the initial process */
    if (CurrentPsp == SYSTEM_PSP)
    {
        /* Set up the standard I/O devices */
        for (i = 0; i <= 2; i++)
        {
            /* Set the index in the SFT */
            DestinationTable[i] = (BYTE)i;

            /* Increase the reference count */
            DosSftRefCount[i]++;
        }

        /* Done */
        return;
    }

    /* Get the parent PSP block and handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    SourceTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Copy the first 20 handles into the new table */
    for (i = 0; i < 20; i++)
    {
        DestinationTable[i] = SourceTable[i];

        /* Increase the reference count */
        DosSftRefCount[SourceTable[i]]++;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable)
{
    WORD Result = 0, Segment = FIRST_MCB_SEGMENT, MaxSize = 0;
    PDOS_MCB CurrentMcb, NextMcb;
    BOOLEAN SearchUmb = FALSE;

    DPRINT("DosAllocateMemory: Size 0x%04X\n", Size);

    if (DosUmbLinked && (DosAllocStrategy & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW)))
    {
        /* Search UMB first */
        Segment = UMB_START_SEGMENT;
        SearchUmb = TRUE;
    }

    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (CurrentMcb->BlockType != 'M' && CurrentMcb->BlockType != 'Z')
        {
            DPRINT("The DOS memory arena is corrupted!\n");
            DosLastError = ERROR_ARENA_TRASHED;
            return 0;
        }

        /* Only check free blocks */
        if (CurrentMcb->OwnerPsp != 0) goto Next;

        /* Combine this free block with adjoining free blocks */
        DosCombineFreeBlocks(Segment);

        /* Update the maximum block size */
        if (CurrentMcb->Size > MaxSize) MaxSize = CurrentMcb->Size;

        /* Check if this block is big enough */
        if (CurrentMcb->Size < Size) goto Next;

        switch (DosAllocStrategy & 0x3F)
        {
            case DOS_ALLOC_FIRST_FIT:
            {
                /* For first fit, stop immediately */
                Result = Segment;
                goto Done;
            }

            case DOS_ALLOC_BEST_FIT:
            {
                /* For best fit, update the smallest block found so far */
                if ((Result == 0) || (CurrentMcb->Size < SEGMENT_TO_MCB(Result)->Size))
                {
                    Result = Segment;
                }

                break;
            }

            case DOS_ALLOC_LAST_FIT:
            {
                /* For last fit, make the current block the result, but keep searching */
                Result = Segment;
                break;
            }
        }

Next:
        /* If this was the last MCB in the chain, quit */
        if (CurrentMcb->BlockType == 'Z')
        {
            /* Check if nothing was found while searching through UMBs */
            if ((Result == 0) && SearchUmb && (DosAllocStrategy & DOS_ALLOC_HIGH_LOW))
            {
                /* Search low memory */
                Segment = FIRST_MCB_SEGMENT;
                continue;
            }

            break;
        }

        /* Otherwise, update the segment and continue */
        Segment += CurrentMcb->Size + 1;
    }

Done:

    /* If we didn't find a free block, return 0 */
    if (Result == 0)
    {
        DosLastError = ERROR_NOT_ENOUGH_MEMORY;
        if (MaxAvailable) *MaxAvailable = MaxSize;
        return 0;
    }

    /* Get a pointer to the MCB */
    CurrentMcb = SEGMENT_TO_MCB(Result);

    /* Check if the block is larger than requested */
    if (CurrentMcb->Size > Size)
    {
        /* It is, split it into two blocks */
        NextMcb = SEGMENT_TO_MCB(Result + Size + 1);

        /* Initialize the new MCB structure */
        NextMcb->BlockType = CurrentMcb->BlockType;
        NextMcb->Size = CurrentMcb->Size - Size - 1;
        NextMcb->OwnerPsp = 0;

        /* Update the current block */
        CurrentMcb->BlockType = 'M';
        CurrentMcb->Size = Size;
    }

    /* Take ownership of the block */
    CurrentMcb->OwnerPsp = CurrentPsp;

    /* Return the segment of the data portion of the block */
    return Result + 1;
}

BOOLEAN DosResizeMemory(WORD BlockData, WORD NewSize, WORD *MaxAvailable)
{
    BOOLEAN Success = TRUE;
    WORD Segment = BlockData - 1, ReturnSize = 0, NextSegment;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment), NextMcb;

    DPRINT("DosResizeMemory: BlockData 0x%04X, NewSize 0x%04X\n",
           BlockData,
           NewSize);

    /* Make sure this is a valid, allocated block */
    if ((Mcb->BlockType != 'M' && Mcb->BlockType != 'Z') || Mcb->OwnerPsp == 0)
    {
        Success = FALSE;
        DosLastError = ERROR_INVALID_HANDLE;
        goto Done;
    }

    ReturnSize = Mcb->Size;

    /* Check if we need to expand or contract the block */
    if (NewSize > Mcb->Size)
    {
        /* We can't expand the last block */
        if (Mcb->BlockType != 'M')
        {
            Success = FALSE;
            goto Done;
        }

        /* Get the pointer and segment of the next MCB */
        NextSegment = Segment + Mcb->Size + 1;
        NextMcb = SEGMENT_TO_MCB(NextSegment);

        /* Make sure the next segment is free */
        if (NextMcb->OwnerPsp != 0)
        {
            DPRINT("Cannot expand memory block: next segment is not free!\n");
            DosLastError = ERROR_NOT_ENOUGH_MEMORY;
            Success = FALSE;
            goto Done;
        }

        /* Combine this free block with adjoining free blocks */
        DosCombineFreeBlocks(NextSegment);

        /* Set the maximum possible size of the block */
        ReturnSize += NextMcb->Size + 1;

        /* Maximize the current block */
        Mcb->Size = ReturnSize;
        Mcb->BlockType = NextMcb->BlockType;

        /* Invalidate the next block */
        NextMcb->BlockType = 'I';

        /* Check if the block is larger than requested */
        if (Mcb->Size > NewSize)
        {
            DPRINT("Block too large, reducing size from 0x%04X to 0x%04X\n",
                   Mcb->Size,
                   NewSize);

            /* It is, split it into two blocks */
            NextMcb = SEGMENT_TO_MCB(Segment + NewSize + 1);
    
            /* Initialize the new MCB structure */
            NextMcb->BlockType = Mcb->BlockType;
            NextMcb->Size = Mcb->Size - NewSize - 1;
            NextMcb->OwnerPsp = 0;

            /* Update the current block */
            Mcb->BlockType = 'M';
            Mcb->Size = NewSize;
        }
    }
    else if (NewSize < Mcb->Size)
    {
        DPRINT("Shrinking block from 0x%04X to 0x%04X\n",
                Mcb->Size,
                NewSize);

        /* Just split the block */
        NextMcb = SEGMENT_TO_MCB(Segment + NewSize + 1);
        NextMcb->BlockType = Mcb->BlockType;
        NextMcb->Size = Mcb->Size - NewSize - 1;
        NextMcb->OwnerPsp = 0;

        /* Update the MCB */
        Mcb->BlockType = 'M';
        Mcb->Size = NewSize;
    }

Done:
    /* Check if the operation failed */
    if (!Success)
    {
        DPRINT("DosResizeMemory FAILED. Maximum available: 0x%04X\n",
               ReturnSize);

        /* Return the maximum possible size */
        if (MaxAvailable) *MaxAvailable = ReturnSize;
    }
    
    return Success;
}

BOOLEAN DosFreeMemory(WORD BlockData)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(BlockData - 1);

    DPRINT("DosFreeMemory: BlockData 0x%04X\n", BlockData);

    /* Make sure the MCB is valid */
    if (Mcb->BlockType != 'M' && Mcb->BlockType != 'Z')
    {
        DPRINT("MCB block type '%c' not valid!\n", Mcb->BlockType);
        return FALSE;
    }

    /* Mark the block as free */
    Mcb->OwnerPsp = 0;

    return TRUE;
}

BOOLEAN DosLinkUmb(VOID)
{
    DWORD Segment = FIRST_MCB_SEGMENT;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment);

    DPRINT("Linking UMB\n");

    /* Check if UMBs are already linked */
    if (DosUmbLinked) return FALSE;

    /* Find the last block */
    while ((Mcb->BlockType == 'M') && (Segment <= 0xFFFF))
    {
        Segment += Mcb->Size + 1;
        Mcb = SEGMENT_TO_MCB(Segment);
    }

    /* Make sure it's valid */
    if (Mcb->BlockType != 'Z') return FALSE;

    /* Connect the MCB with the UMB chain */
    Mcb->BlockType = 'M';

    DosUmbLinked = TRUE;
    return TRUE;
}

BOOLEAN DosUnlinkUmb(VOID)
{
    DWORD Segment = FIRST_MCB_SEGMENT;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment);

    DPRINT("Unlinking UMB\n");

    /* Check if UMBs are already unlinked */
    if (!DosUmbLinked) return FALSE;

    /* Find the block preceding the MCB that links it with the UMB chain */
    while (Segment <= 0xFFFF)
    {
        if ((Segment + Mcb->Size) == (FIRST_MCB_SEGMENT + USER_MEMORY_SIZE))
        {
            /* This is the last non-UMB segment */
            break;
        }

        /* Advance to the next MCB */
        Segment += Mcb->Size + 1;
        Mcb = SEGMENT_TO_MCB(Segment);
    }

    /* Mark the MCB as the last MCB */
    Mcb->BlockType = 'Z';

    DosUmbLinked = FALSE;
    return TRUE;
}

WORD DosCreateFile(LPWORD Handle, LPCSTR FilePath, WORD Attributes)
{
    HANDLE FileHandle;
    WORD DosHandle;

    DPRINT("DosCreateFile: FilePath \"%s\", Attributes 0x%04X\n",
            FilePath,
            Attributes);

    /* Create the file */
    FileHandle = CreateFileA(FilePath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             CREATE_ALWAYS,
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
        /* Close the handle */
        CloseHandle(FileHandle);

        /* Return the error code */
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* It was successful */
    *Handle = DosHandle;
    return ERROR_SUCCESS;
}

WORD DosOpenFile(LPWORD Handle, LPCSTR FilePath, BYTE AccessMode)
{
    HANDLE FileHandle;
    ACCESS_MASK Access = 0;
    WORD DosHandle;

    DPRINT("DosOpenFile: FilePath \"%s\", AccessMode 0x%04X\n",
            FilePath,
            AccessMode);

    /* Parse the access mode */
    switch (AccessMode & 3)
    {
        case 0:
        {
            /* Read-only */
            Access = GENERIC_READ;
            break;
        }

        case 1:
        {
            /* Write only */
            Access = GENERIC_WRITE;
            break;
        }

        case 2:
        {
            /* Read and write */
            Access = GENERIC_READ | GENERIC_WRITE;
            break;
        }

        default:
        {
            /* Invalid */
            return ERROR_INVALID_PARAMETER;
        }
    }

    /* Open the file */
    FileHandle = CreateFileA(FilePath,
                             Access,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
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
        /* Close the handle */
        CloseHandle(FileHandle);

        /* Return the error code */
        return ERROR_TOO_MANY_OPEN_FILES;
    }

    /* It was successful */
    *Handle = DosHandle;
    return ERROR_SUCCESS;
}

WORD DosReadFile(WORD FileHandle, LPVOID Buffer, WORD Count, LPWORD BytesRead)
{
    WORD Result = ERROR_SUCCESS;
    DWORD BytesRead32 = 0;
    HANDLE Handle = DosGetRealHandle(FileHandle);

    DPRINT("DosReadFile: FileHandle 0x%04X, Count 0x%04X\n", FileHandle, Count);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    /* Read the file */
    if (!ReadFile(Handle, Buffer, Count, &BytesRead32, NULL))
    {
        /* Store the error code */
        Result = (WORD)GetLastError();
    }

    /* The number of bytes read is always 16-bit */
    *BytesRead = LOWORD(BytesRead32);

    /* Return the error code */
    return Result;
}

WORD DosWriteFile(WORD FileHandle, LPVOID Buffer, WORD Count, LPWORD BytesWritten)
{
    WORD Result = ERROR_SUCCESS;
    DWORD BytesWritten32 = 0;
    HANDLE Handle = DosGetRealHandle(FileHandle);

    DPRINT("DosWriteFile: FileHandle 0x%04X, Count 0x%04X\n",
           FileHandle,
           Count);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return ERROR_INVALID_HANDLE;

    /* Write the file */
    if (!WriteFile(Handle, Buffer, Count, &BytesWritten32, NULL))
    {
        /* Store the error code */
        Result = (WORD)GetLastError();
    }

    /* The number of bytes written is always 16-bit */
    *BytesWritten = LOWORD(BytesWritten32);

    /* Return the error code */
    return Result;
}

WORD DosSeekFile(WORD FileHandle, LONG Offset, BYTE Origin, LPDWORD NewOffset)
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
    FilePointer = SetFilePointer(Handle, Offset, NULL, Origin);

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

BOOLEAN DosDuplicateHandle(WORD OldHandle, WORD NewHandle)
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
    DosSftRefCount[SftIndex]++;

    /* Make the new handle point to that SFT entry */
    HandleTable[NewHandle] = SftIndex;

    /* Return success */
    return TRUE;
}

BOOLEAN DosCloseHandle(WORD DosHandle)
{
    BYTE SftIndex;
    PDOS_PSP PspBlock;
    LPBYTE HandleTable;

    DPRINT("DosCloseHandle: DosHandle 0x%04X\n", DosHandle);

    /* The system PSP has no handle table */
    if (CurrentPsp == SYSTEM_PSP) return FALSE;

    /* Get a pointer to the handle table */
    PspBlock = SEGMENT_TO_PSP(CurrentPsp);
    HandleTable = (LPBYTE)FAR_POINTER(PspBlock->HandleTablePtr);

    /* Make sure the handle is open */
    if (HandleTable[DosHandle] == 0xFF) return FALSE;

    /* Decrement the reference count of the SFT entry */
    SftIndex = HandleTable[DosHandle];
    DosSftRefCount[SftIndex]--;

    /* Check if the reference count fell to zero */
    if (!DosSftRefCount[SftIndex])
    {
        /* Close the file, it's no longer needed */
        CloseHandle(DosSystemFileTable[SftIndex]);

        /* Clear the handle */
        DosSystemFileTable[SftIndex] = INVALID_HANDLE_VALUE;
    }

    /* Clear the entry in the JFT */
    HandleTable[DosHandle] = 0xFF;

    return TRUE;
}

BOOLEAN DosChangeDrive(BYTE Drive)
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

BOOLEAN DosChangeDirectory(LPSTR Directory)
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
    Path = strchr(Directory, '\\') + 1;

    /* Set the directory for the drive */
    strcpy(CurrentDirectories[DriveNumber], Path);
    
    /* Return success */
    return TRUE;
}

VOID DosInitializePsp(WORD PspSegment, LPCSTR CommandLine, WORD ProgramSize, WORD Environment)
{
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(PspSegment);
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    ZeroMemory(PspBlock, sizeof(DOS_PSP));

    /* Set the exit interrupt */
    PspBlock->Exit[0] = 0xCD; // int 0x20
    PspBlock->Exit[1] = 0x20;

    /* Set the number of the last paragraph */
    PspBlock->LastParagraph = PspSegment + ProgramSize - 1;

    /* Save the interrupt vectors */
    PspBlock->TerminateAddress = IntVecTable[0x22];
    PspBlock->BreakAddress = IntVecTable[0x23];
    PspBlock->CriticalAddress = IntVecTable[0x24];

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

BOOLEAN DosCreateProcess(LPCSTR CommandLine, WORD EnvBlock)
{
    BOOLEAN Success = FALSE, AllocatedEnvBlock = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE, FileMapping = NULL;
    LPBYTE Address = NULL;
    LPSTR ProgramFilePath, Parameters[256];
    CHAR CommandLineCopy[DOS_CMDLINE_LENGTH];
    INT ParamCount = 0;
    WORD Segment = 0;
    WORD MaxAllocSize;
    DWORD i, FileSize, ExeSize;
    PIMAGE_DOS_HEADER Header;
    PDWORD RelocationTable;
    PWORD RelocWord;

    DPRINT("DosCreateProcess: CommandLine \"%s\", EnvBlock 0x%04X\n",
           CommandLine,
           EnvBlock);

    /* Save a copy of the command line */
    strcpy(CommandLineCopy, CommandLine);

    // FIXME: Improve parsing (especially: "some_path\with spaces\program.exe" options)

    /* Get the file name of the executable */
    ProgramFilePath = strtok(CommandLineCopy, " \t");

    /* Load the parameters in the local array */
    while ((ParamCount < sizeof(Parameters)/sizeof(Parameters[0]))
           && ((Parameters[ParamCount] = strtok(NULL, " \t")) != NULL))
    {
        ParamCount++;
    }

    /* Open a handle to the executable */
    FileHandle = CreateFileA(ProgramFilePath,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) goto Cleanup;

    /* Get the file size */
    FileSize = GetFileSize(FileHandle, NULL);

    /* Create a mapping object for the file */
    FileMapping = CreateFileMapping(FileHandle,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);
    if (FileMapping == NULL) goto Cleanup;

    /* Map the file into memory */
    Address = (LPBYTE)MapViewOfFile(FileMapping, FILE_MAP_READ, 0, 0, 0);
    if (Address == NULL) goto Cleanup;

    /* Did we get an environment segment? */
    if (!EnvBlock)
    {
        /* Set a flag to know if the environment block was allocated here */
        AllocatedEnvBlock = TRUE;

        /* No, copy the one from the parent */
        EnvBlock = DosCopyEnvironmentBlock((CurrentPsp != SYSTEM_PSP)
                                           ? SEGMENT_TO_PSP(CurrentPsp)->EnvBlock
                                           : SYSTEM_ENV_BLOCK,
                                           ProgramFilePath);
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

        /* Reduce the size one by one until the allocation is successful */
        for (i = Header->e_maxalloc; i >= Header->e_minalloc; i--, ExeSize--)
        {
            /* Try to allocate that much memory */
            Segment = DosAllocateMemory((WORD)ExeSize, NULL);
            if (Segment != 0) break;
        }

        /* Check if at least the lowest allocation was successful */
        if (Segment == 0) goto Cleanup;

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         (WORD)ExeSize,
                         EnvBlock);

        /* The process owns its own memory */
        DosChangeMemoryOwner(Segment, Segment);
        DosChangeMemoryOwner(EnvBlock, Segment);

        /* Copy the program to Segment:0100 */
        RtlCopyMemory((PVOID)((ULONG_PTR)BaseAddress
                      + TO_LINEAR(Segment, 0x100)),
                      Address + (Header->e_cparhdr << 4),
                      min(FileSize - (Header->e_cparhdr << 4),
                          (ExeSize << 4) - sizeof(DOS_PSP)));

        /* Get the relocation table */
        RelocationTable = (PDWORD)(Address + Header->e_lfarlc);

        /* Perform relocations */
        for (i = 0; i < Header->e_crlc; i++)
        {
            /* Get a pointer to the word that needs to be patched */
            RelocWord = (PWORD)((ULONG_PTR)BaseAddress
                                + TO_LINEAR(Segment + HIWORD(RelocationTable[i]),
                                            0x100 + LOWORD(RelocationTable[i])));

            /* Add the number of the EXE segment to it */
            *RelocWord += Segment + (sizeof(DOS_PSP) >> 4);
        }

        /* Set the initial segment registers */
        EmulatorSetRegister(EMULATOR_REG_DS, Segment);
        EmulatorSetRegister(EMULATOR_REG_ES, Segment);

        /* Set the stack to the location from the header */
        EmulatorSetStack(Segment + (sizeof(DOS_PSP) >> 4) + Header->e_ss,
                         Header->e_sp);

        /* Execute */
        CurrentPsp = Segment;
        DiskTransferArea = MAKELONG(0x80, Segment);
        EmulatorExecute(Segment + Header->e_cs + (sizeof(DOS_PSP) >> 4),
                        Header->e_ip);

        Success = TRUE;
    }
    else
    {
        /* COM file */

        /* Find the maximum amount of memory that can be allocated */
        DosAllocateMemory(0xFFFF, &MaxAllocSize);

        /* Make sure it's enough for the whole program and the PSP */
        if (((DWORD)MaxAllocSize << 4) < (FileSize + sizeof(DOS_PSP))) goto Cleanup;

        /* Allocate all of it */
        Segment = DosAllocateMemory(MaxAllocSize, NULL);
        if (Segment == 0) goto Cleanup;

        /* The process owns its own memory */
        DosChangeMemoryOwner(Segment, Segment);
        DosChangeMemoryOwner(EnvBlock, Segment);

        /* Copy the program to Segment:0100 */
        RtlCopyMemory((PVOID)((ULONG_PTR)BaseAddress
                      + TO_LINEAR(Segment, 0x100)),
                      Address,
                      FileSize);

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         (WORD)((FileSize + sizeof(DOS_PSP)) >> 4),
                         EnvBlock);

        /* Set the initial segment registers */
        EmulatorSetRegister(EMULATOR_REG_DS, Segment);
        EmulatorSetRegister(EMULATOR_REG_ES, Segment);

        /* Set the stack to the last word of the segment */
        EmulatorSetStack(Segment, 0xFFFE);

        /* Execute */
        CurrentPsp = Segment;
        DiskTransferArea = MAKELONG(0x80, Segment);
        EmulatorExecute(Segment, 0x100);

        Success = TRUE;
    }

Cleanup:
    if (!Success)
    {
        /* It was not successful, cleanup the DOS memory */
        if (AllocatedEnvBlock) DosFreeMemory(EnvBlock);
        if (Segment) DosFreeMemory(Segment);
    }

    /* Unmap the file*/
    if (Address != NULL) UnmapViewOfFile(Address);

    /* Close the file mapping object */
    if (FileMapping != NULL) CloseHandle(FileMapping);

    /* Close the file handle */
    if (FileHandle != INVALID_HANDLE_VALUE) CloseHandle(FileHandle);

    return Success;
}

VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode)
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

    for (i = 0; i < PspBlock->HandleTableSize; i++)
    {
        /* Close the handle */
        DosCloseHandle(i);
    }

    /* Free the memory used by the process */
    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(McbSegment);

        /* Make sure the MCB is valid */
        if (CurrentMcb->BlockType != 'M' && CurrentMcb->BlockType !='Z') break;

        /* If this block was allocated by the process, free it */
        if (CurrentMcb->OwnerPsp == Psp) DosFreeMemory(McbSegment);

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
        if (CurrentPsp == SYSTEM_PSP) VdmRunning = FALSE;
    }

    /* Return control to the parent process */
    EmulatorExecute(HIWORD(PspBlock->TerminateAddress),
                    LOWORD(PspBlock->TerminateAddress));
}

CHAR DosReadCharacter(VOID)
{
    CHAR Character = '\0';
    WORD BytesRead;

    /* Use the file reading function */
    DosReadFile(DOS_INPUT_HANDLE, &Character, sizeof(CHAR), &BytesRead);

    return Character;
}

VOID DosPrintCharacter(CHAR Character)
{
    WORD BytesWritten;

    /* Use the file writing function */
    DosWriteFile(DOS_OUTPUT_HANDLE, &Character, sizeof(CHAR), &BytesWritten);
}

BOOLEAN DosHandleIoctl(BYTE ControlCode, WORD FileHandle)
{
    HANDLE Handle = DosGetRealHandle(FileHandle);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        /* Doesn't exist */
        DosLastError = ERROR_FILE_NOT_FOUND;
        return FALSE;
    }

    switch (ControlCode)
    {
        /* Get Device Information */
        case 0x00:
        {
            WORD InfoWord = 0;

            if (Handle == DosSystemFileTable[0])
            {
                /* Console input */
                InfoWord |= 1 << 0;
            }
            else if (Handle == DosSystemFileTable[1])
            {
                /* Console output */
                InfoWord |= 1 << 1;
            }

            /* It is a character device */
            InfoWord |= 1 << 7;

            /* Return the device information word */
            EmulatorSetRegister(EMULATOR_REG_DX, InfoWord);

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

VOID DosInt20h(LPWORD Stack)
{
    /* This is the exit interrupt */
    DosTerminateProcess(Stack[STACK_CS], 0);
}

VOID DosInt21h(LPWORD Stack)
{
    INT i;
    CHAR Character;
    SYSTEMTIME SystemTime;
    PCHAR String;
    PDOS_INPUT_BUFFER InputBuffer;
    DWORD Eax = EmulatorGetRegister(EMULATOR_REG_AX);
    DWORD Ecx = EmulatorGetRegister(EMULATOR_REG_CX);
    DWORD Edx = EmulatorGetRegister(EMULATOR_REG_DX);
    DWORD Ebx = EmulatorGetRegister(EMULATOR_REG_BX);
    WORD DataSegment = (WORD)EmulatorGetRegister(EMULATOR_REG_DS);
    WORD ExtSegment = (WORD)EmulatorGetRegister(EMULATOR_REG_ES);

    /* Check the value in the AH register */
    switch (HIBYTE(Eax))
    {
        /* Terminate Program */
        case 0x00:
        {
            DosTerminateProcess(Stack[STACK_CS], 0);
            break;
        }

        /* Read Character And Echo */
        case 0x01:
        {
            Character = DosReadCharacter();
            DosPrintCharacter(Character);
            EmulatorSetRegister(EMULATOR_REG_AX, (Eax & 0xFFFFFF00) | Character);
            break;
        }

        /* Print Character */
        case 0x02:
        {
            DosPrintCharacter(LOBYTE(Edx));
            break;
        }

        /* Read Character Without Echo */
        case 0x07:
        case 0x08:
        {
            EmulatorSetRegister(EMULATOR_REG_AX,
                               (Eax & 0xFFFFFF00) | DosReadCharacter());
            break;
        }

        /* Print String */
        case 0x09:
        {
            String = (PCHAR)((ULONG_PTR)BaseAddress
                     + TO_LINEAR(DataSegment, LOWORD(Edx)));

            while ((*String) != '$')
            {
                DosPrintCharacter(*String);
                String++;
            }

            break;
        }

        /* Read Buffered Input */
        case 0x0A:
        {
            InputBuffer = (PDOS_INPUT_BUFFER)((ULONG_PTR)BaseAddress
                                              + TO_LINEAR(DataSegment,
                                                          LOWORD(Edx)));

            InputBuffer->Length = 0;
            for (i = 0; i < InputBuffer->MaxLength; i ++)
            {
                Character = DosReadCharacter();
                DosPrintCharacter(Character);
                InputBuffer->Buffer[InputBuffer->Length] = Character;
                if (Character == '\r') break;
                InputBuffer->Length++;
            }

            break;
        }

        /* Set Default Drive  */
        case 0x0E:
        {
            DosChangeDrive(LOBYTE(Edx));
            EmulatorSetRegister(EMULATOR_REG_AX,
                                (Eax & 0xFFFFFF00) | (LastDrive - 'A' + 1));

            break;
        }

        /* Set Disk Transfer Area */
        case 0x1A:
        {
            DiskTransferArea = MAKELONG(LOWORD(Edx), DataSegment);
            break;
        }

        /* Set Interrupt Vector */
        case 0x25:
        {
            DWORD FarPointer = MAKELONG(LOWORD(Edx), DataSegment);

            /* Write the new far pointer to the IDT */
            ((PDWORD)BaseAddress)[LOBYTE(Eax)] = FarPointer;

            break;
        }

        /* Get system date */
        case 0x2A:
        {
            GetLocalTime(&SystemTime);
            EmulatorSetRegister(EMULATOR_REG_CX,
                                (Ecx & 0xFFFF0000) | SystemTime.wYear);
            EmulatorSetRegister(EMULATOR_REG_DX,
                                (Edx & 0xFFFF0000)
                                | (SystemTime.wMonth << 8)
                                | SystemTime.wDay);
            EmulatorSetRegister(EMULATOR_REG_AX,
                                (Eax & 0xFFFFFF00) | SystemTime.wDayOfWeek);
            break;
        }

        /* Set system date */
        case 0x2B:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wYear = LOWORD(Ecx);
            SystemTime.wMonth = HIBYTE(Edx);
            SystemTime.wDay = LOBYTE(Edx);

            if (SetLocalTime(&SystemTime))
            {
                /* Return success */
                EmulatorSetRegister(EMULATOR_REG_AX, Eax & 0xFFFFFF00);
            }
            else
            {
                /* Return failure */
                EmulatorSetRegister(EMULATOR_REG_AX, Eax | 0xFF);
            }

            break;
        }

        /* Get system time */
        case 0x2C:
        {
            GetLocalTime(&SystemTime);
            EmulatorSetRegister(EMULATOR_REG_CX,
                                (Ecx & 0xFFFF0000)
                                | (SystemTime.wHour << 8)
                                | SystemTime.wMinute);
            EmulatorSetRegister(EMULATOR_REG_DX,
                                (Edx & 0xFFFF0000)
                                | (SystemTime.wSecond << 8)
                                | (SystemTime.wMilliseconds / 10));
            break;
        }

        /* Set system time */
        case 0x2D:
        {
            GetLocalTime(&SystemTime);
            SystemTime.wHour = HIBYTE(Ecx);
            SystemTime.wMinute = LOBYTE(Ecx);
            SystemTime.wSecond = HIBYTE(Edx);
            SystemTime.wMilliseconds = LOBYTE(Edx) * 10;

            if (SetLocalTime(&SystemTime))
            {
                /* Return success */
                EmulatorSetRegister(EMULATOR_REG_AX, Eax & 0xFFFFFF00);
            }
            else
            {
                /* Return failure */
                EmulatorSetRegister(EMULATOR_REG_AX, Eax | 0xFF);
            }

            break;
        }

        /* Get Disk Transfer Area */
        case 0x2F:
        {
            EmulatorSetRegister(EMULATOR_REG_ES, HIWORD(DiskTransferArea));
            EmulatorSetRegister(EMULATOR_REG_BX, LOWORD(DiskTransferArea));

            break;
        }

        /* Get DOS Version */
        case 0x30:
        {
            PDOS_PSP PspBlock = SEGMENT_TO_PSP(CurrentPsp);

            EmulatorSetRegister(EMULATOR_REG_AX, PspBlock->DosVersion);
            break;
        }

        /* Get Interrupt Vector */
        case 0x35:
        {
            DWORD FarPointer = ((PDWORD)BaseAddress)[LOBYTE(Eax)];

            /* Read the address from the IDT into ES:BX */
            EmulatorSetRegister(EMULATOR_REG_ES, HIWORD(FarPointer));
            EmulatorSetRegister(EMULATOR_REG_BX, LOWORD(FarPointer));

            break;
        }

        /* Create Directory */
        case 0x39:
        {
            String = (PCHAR)((ULONG_PTR)BaseAddress
                     + TO_LINEAR(DataSegment, LOWORD(Edx)));

            if (CreateDirectoryA(String, NULL))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | LOWORD(GetLastError()));
            }

            break;
        }

        /* Remove Directory */
        case 0x3A:
        {
            String = (PCHAR)((ULONG_PTR)BaseAddress
                     + TO_LINEAR(DataSegment, LOWORD(Edx)));

            if (RemoveDirectoryA(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | LOWORD(GetLastError()));
            }


            break;
        }

        /* Set Current Directory */
        case 0x3B:
        {
            String = (PCHAR)((ULONG_PTR)BaseAddress
                     + TO_LINEAR(DataSegment, LOWORD(Edx)));

            if (DosChangeDirectory(String))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | DosLastError);
            }

            break;
        }

        /* Create File */
        case 0x3C:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)(ULONG_PTR)BaseAddress
                                           + TO_LINEAR(DataSegment, LOWORD(Edx)),
                                           LOWORD(Ecx));

            if (ErrorCode == 0)
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the handle in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | FileHandle);
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ErrorCode);
            }

            break;
        }

        /* Open File */
        case 0x3D:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)(ULONG_PTR)BaseAddress
                                           + TO_LINEAR(DataSegment, LOWORD(Edx)),
                                           LOBYTE(Eax));

            if (ErrorCode == 0)
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the handle in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | FileHandle);
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ErrorCode);
            }

            break;
        }

        /* Close File */
        case 0x3E:
        {
            if (DosCloseHandle(LOWORD(Ebx)))
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Read File */
        case 0x3F:
        {
            WORD BytesRead = 0;
            WORD ErrorCode = DosReadFile(LOWORD(Ebx),
                                         (LPVOID)((ULONG_PTR)BaseAddress
                                         + TO_LINEAR(DataSegment, LOWORD(Edx))),
                                         LOWORD(Ecx),
                                         &BytesRead);

            if (ErrorCode == 0)
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the number of bytes read in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | BytesRead);
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ErrorCode);
            }
            break;
        }

        /* Write File */
        case 0x40:
        {
            WORD BytesWritten = 0;
            WORD ErrorCode = DosWriteFile(LOWORD(Ebx),
                                          (LPVOID)((ULONG_PTR)BaseAddress
                                          + TO_LINEAR(DataSegment, LOWORD(Edx))),
                                          LOWORD(Ecx),
                                          &BytesWritten);

            if (ErrorCode == 0)
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the number of bytes written in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | BytesWritten);
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ErrorCode);
            }

            break;
        }

        /* Delete File */
        case 0x41:
        {
            LPSTR FileName = (LPSTR)((ULONG_PTR)BaseAddress + TO_LINEAR(DataSegment, Edx));

            /* Call the API function */
            if (DeleteFileA(FileName)) Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, GetLastError());
            }

            break;
        }

        /* Seek File */
        case 0x42:
        {
            DWORD NewLocation;
            WORD ErrorCode = DosSeekFile(LOWORD(Ebx),
                                         MAKELONG(LOWORD(Edx), LOWORD(Ecx)),
                                         LOBYTE(Eax),
                                         &NewLocation);

            if (ErrorCode == 0)
            {
                /* Clear CF */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                /* Return the new offset in DX:AX */
                EmulatorSetRegister(EMULATOR_REG_DX,
                                    (Edx & 0xFFFF0000) | HIWORD(NewLocation));
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | LOWORD(NewLocation));
            }
            else
            {
                /* Set CF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Return the error code in AX */
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | ErrorCode);
            }

            break;
        }

        /* Get/Set File Attributes */
        case 0x43:
        {
            DWORD Attributes;
            LPSTR FileName = (LPSTR)((ULONG_PTR)BaseAddress + TO_LINEAR(DataSegment, Edx));

            if (LOBYTE(Eax) == 0x00)
            {
                /* Get the attributes */
                Attributes = GetFileAttributesA(FileName);

                /* Check if it failed */
                if (Attributes == INVALID_FILE_ATTRIBUTES)
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    EmulatorSetRegister(EMULATOR_REG_AX, GetLastError());

                    break;
                }

                /* Return the attributes that DOS can understand */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_CX,
                                    (Ecx & 0xFFFFFF00) | LOBYTE(Attributes));
            }
            else if (LOBYTE(Eax) == 0x01)
            {
                /* Try to set the attributes */
                if (SetFileAttributesA(FileName, LOBYTE(Ecx)))
                {
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
                else
                {
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    EmulatorSetRegister(EMULATOR_REG_AX, GetLastError());
                }
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* IOCTL */
        case 0x44:
        {
            if (DosHandleIoctl(LOBYTE(Eax), LOWORD(Ebx)))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, DosLastError);
            }

            break;
        }

        /* Duplicate Handle */
        case 0x45:
        {
            WORD NewHandle;
            HANDLE Handle = DosGetRealHandle(LOWORD(Ebx));

            if (Handle != INVALID_HANDLE_VALUE)
            {
                /* The handle is invalid */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_HANDLE);

                break;
            }

            /* Open a new handle to the same entry */
            NewHandle = DosOpenHandle(Handle);

            if (NewHandle == INVALID_DOS_HANDLE)
            {
                /* Too many files open */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_TOO_MANY_OPEN_FILES);

                break;
            }

            /* Return the result */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            EmulatorSetRegister(EMULATOR_REG_AX, NewHandle);

            break;
        }

        /* Force Duplicate Handle */
        case 0x46:
        {
            if (DosDuplicateHandle(LOWORD(Ebx), LOWORD(Ecx)))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_HANDLE);
            }

            break;
        }

        /* Allocate Memory */
        case 0x48:
        {
            WORD MaxAvailable = 0;
            WORD Segment = DosAllocateMemory(LOWORD(Ebx), &MaxAvailable);

            if (Segment != 0)
            {
                EmulatorSetRegister(EMULATOR_REG_AX, Segment);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                EmulatorSetRegister(EMULATOR_REG_AX, DosLastError);
                EmulatorSetRegister(EMULATOR_REG_BX, MaxAvailable);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Free Memory */
        case 0x49:
        {
            if (DosFreeMemory(ExtSegment))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_ARENA_TRASHED);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            }

            break;
        }

        /* Resize Memory Block */
        case 0x4A:
        {
            WORD Size;

            if (DosResizeMemory(ExtSegment, LOWORD(Ebx), &Size))
            {
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                EmulatorSetRegister(EMULATOR_REG_AX, DosLastError);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_BX, Size);
            }

            break;
        }

        /* Terminate With Return Code */
        case 0x4C:
        {
            DosTerminateProcess(CurrentPsp, LOBYTE(Eax));
            break;
        }

        /* Get Current Process */
        case 0x51:
        {
            EmulatorSetRegister(EMULATOR_REG_BX, CurrentPsp);

            break;
        }

        /* Get/Set Memory Management Options */
        case 0x58:
        {
            if (LOBYTE(Eax) == 0x00)
            {
                /* Get allocation strategy */

                EmulatorSetRegister(EMULATOR_REG_AX, DosAllocStrategy);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (LOBYTE(Eax) == 0x01)
            {
                /* Set allocation strategy */

                if ((LOBYTE(Ebx) & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                    == (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
                {
                    /* Can't set both */
                    EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_PARAMETER);
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    break;
                }

                if ((LOBYTE(Ebx) & 0x3F) > DOS_ALLOC_LAST_FIT)
                {
                    /* Invalid allocation strategy */
                    EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_PARAMETER);
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    break;
                }

                DosAllocStrategy = LOBYTE(Ebx);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (LOBYTE(Eax) == 0x02)
            {
                /* Get UMB link state */

                Eax &= 0xFFFFFF00;
                if (DosUmbLinked) Eax |= 1;
                EmulatorSetRegister(EMULATOR_REG_AX, Eax);
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (LOBYTE(Eax) == 0x03)
            {
                /* Set UMB link state */

                if (Ebx) DosLinkUmb();
                else DosUnlinkUmb();
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else
            {
                /* Invalid or unsupported function */

                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                EmulatorSetRegister(EMULATOR_REG_AX, ERROR_INVALID_FUNCTION);
            }

            break;
        }

        /* Unsupported */
        default:
        {
            DPRINT1("DOS Function INT 0x21, AH = 0x%02X NOT IMPLEMENTED!\n", HIBYTE(Eax));
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

VOID DosBreakInterrupt(LPWORD Stack)
{
    UNREFERENCED_PARAMETER(Stack);

    VdmRunning = FALSE;
}

BOOLEAN DosInitialize(VOID)
{
    BYTE i;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);
    FILE *Stream;
    WCHAR Buffer[256];
    LPWSTR SourcePtr, Environment;
    LPSTR AsciiString;
    LPSTR DestPtr = (LPSTR)((ULONG_PTR)BaseAddress + TO_LINEAR(SYSTEM_ENV_BLOCK, 0));
    DWORD AsciiSize;
    CHAR CurrentDirectory[MAX_PATH];
    CHAR DosDirectory[DOS_DIR_LENGTH];
    LPSTR Path;

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

    /* Get the path */
    Path = strchr(DosDirectory, '\\');
    if (Path != NULL)
    {
        /* Skip the backslash */
        Path++;
    }

    /* Set the directory */
    if (Path != NULL) strcpy(CurrentDirectories[CurrentDrive], Path);

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
        DosSystemFileTable[i] = INVALID_HANDLE_VALUE;
        DosSftRefCount[i] = 0;
    }

    /* Get handles to standard I/O devices */
    DosSystemFileTable[0] = GetStdHandle(STD_INPUT_HANDLE);
    DosSystemFileTable[1] = GetStdHandle(STD_OUTPUT_HANDLE);
    DosSystemFileTable[2] = GetStdHandle(STD_ERROR_HANDLE);

    return TRUE;
}

/* EOF */
