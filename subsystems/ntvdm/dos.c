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
#include "bop.h"

#include "dos.h"
#include "bios.h"

#include "registers.h"

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
static WORD DosErrorLevel = 0x0000;

/* PRIVATE FUNCTIONS **********************************************************/

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

    Ptr = SourceBuffer = (PCHAR)SEG_OFF_TO_PTR(SourceSegment, 0);

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

    DestBuffer = (PCHAR)SEG_OFF_TO_PTR(DestSegment, 0);
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
            BiosPrintCharacter(((LPBYTE)Buffer)[i], DOS_CHAR_ATTRIBUTE, Bda->VideoPage);
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

BOOLEAN DosFlushFileBuffers(WORD FileHandle)
{
    HANDLE Handle = DosGetRealHandle(FileHandle);

    /* Make sure the handle is valid */
    if (Handle == INVALID_HANDLE_VALUE) return FALSE;

    /*
     * No need to check whether the handle is a console handle since
     * FlushFileBuffers() automatically does this check and calls
     * FlushConsoleInputBuffer() for us.
     */
    // if (IsConsoleHandle(Handle))
    //    return (BOOLEAN)FlushConsoleInputBuffer(Handle);
    // else
    return (BOOLEAN)FlushFileBuffers(Handle);
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

        /* Set the initial segment registers */
        setDS(Segment);
        setES(Segment);

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
        RtlCopyMemory(SEG_OFF_TO_PTR(Segment, 0x100),
                      Address,
                      FileSize);

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         (WORD)((FileSize + sizeof(DOS_PSP)) >> 4),
                         EnvBlock);

        /* Set the initial segment registers */
        setDS(Segment);
        setES(Segment);

        /* Set the stack to the last word of the segment */
        EmulatorSetStack(Segment, 0xFFFE);

        /*
         * Set the value on the stack to 0, so that a near return
         * jumps to PSP:0000 which has the exit code.
         */
        *((LPWORD)SEG_OFF_TO_PTR(Segment, 0xFFFE)) = 0;

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
        if (CurrentMcb->OwnerPsp == Psp) DosFreeMemory(McbSegment + 1);

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

    /* Save the return code - Normal termination */
    DosErrorLevel = MAKEWORD(ReturnCode, 0x00);

    /* Return control to the parent process */
    EmulatorExecute(HIWORD(PspBlock->TerminateAddress),
                    LOWORD(PspBlock->TerminateAddress));
}

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
            setDX(InfoWord);
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
    DosTerminateProcess(Stack[STACK_CS], 0);
}

VOID WINAPI DosInt21h(LPWORD Stack)
{
    BYTE Character;
    SYSTEMTIME SystemTime;
    PCHAR String;
    PDOS_INPUT_BUFFER InputBuffer;

    /* Check the value in the AH register */
    switch (getAH())
    {
        /* Terminate Program */
        case 0x00:
        {
            DosTerminateProcess(Stack[STACK_CS], 0);
            break;
        }

        /* Read Character from STDIN with Echo */
        case 0x01:
        {
            Character = DosReadCharacter();
            DosPrintCharacter(Character);

            /* Let the BOP repeat if needed */
            if (getCF()) break;

            setAL(Character);
            break;
        }

        /* Write Character to STDOUT */
        case 0x02:
        {
            Character = getDL();
            DosPrintCharacter(Character);

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
            setAL(DosReadCharacter());
            break;
        }

        /* Write Character to STDAUX */
        case 0x04:
        {
            // FIXME: Really write it to STDAUX!
            DPRINT1("INT 16h, 04h: Write character to STDAUX is HALFPLEMENTED\n");
            DosPrintCharacter(getDL());
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

            if (Character != 0xFF)
            {
                /* Output */
                DosPrintCharacter(Character);

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
                    setAL(DosReadCharacter());
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
            Character = DosReadCharacter();

            /* Let the BOP repeat if needed */
            if (getCF()) break;

            setAL(Character);
            break;
        }

        /* Write string to STDOUT */
        case 0x09:
        {
            String = (PCHAR)SEG_OFF_TO_PTR(getDS(), getDX());

            while (*String != '$')
            {
                DosPrintCharacter(*String);
                String++;
            }

            /*
             * We return the terminating character (DOS 2.1+).
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2562.htm
             * for more information.
             */
            setAL('$');
            break;
        }

        /* Read Buffered Input */
        case 0x0A:
        {
            InputBuffer = (PDOS_INPUT_BUFFER)SEG_OFF_TO_PTR(getDS(), getDX());

            while (Stack[STACK_COUNTER] < InputBuffer->MaxLength)
            {
                /* Try to read a character */
                Character = DosReadCharacter();

                /* If it's not ready yet, let the BOP repeat */
                if (getCF()) break;

                /* Echo the character and append it to the buffer */
                DosPrintCharacter(Character);
                InputBuffer->Buffer[Stack[STACK_COUNTER]] = Character;

                if (Character == '\r') break;
                Stack[STACK_COUNTER]++;
            }

            /* Update the length */
            InputBuffer->Length = Stack[STACK_COUNTER];
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
            DosFlushFileBuffers(DOS_INPUT_HANDLE); // Maybe just create a DosFlushInputBuffer...

            /*
             * If the input function number contained in AL is valid, i.e.
             * AL == 0x01 or 0x06 or 0x07 or 0x08 or 0x0A, call ourselves
             * recursively with AL == AH.
             */
            if (InputFunction == 0x01 || InputFunction == 0x06 ||
                InputFunction == 0x07 || InputFunction == 0x08 ||
                InputFunction == 0x0A)
            {
                setAH(InputFunction);
                /*
                 * Instead of calling ourselves really recursively as in:
                 * DosInt21h(Stack);
                 * prefer resetting the CF flag to let the BOP repeat.
                 */
                setCF(1);
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
            DWORD FarPointer = MAKELONG(getDX(), getDS());
            DPRINT1("Setting interrupt 0x%x ...\n", getAL());

            /* Write the new far pointer to the IDT */
            ((PDWORD)BaseAddress)[getAL()] = FarPointer;
            break;
        }

        /* Create New PSP */
        case 0x26:
        {
            DPRINT1("INT 21h, 26h - Create New PSP is UNIMPLEMENTED\n");
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
             * See Ralf Brown: http://www.ctyme.com/intr/rb-2711.htm
             * for more information.
             */

            if (LOBYTE(PspBlock->DosVersion) < 5 || getAL() == 0x00)
            {
                /*
                 * Return DOS OEM number:
                 * 0x00 for IBM PC-DOS
                 * 0x02 for packaged MS-DOS
                 */
                setBH(0x02);
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

            /* Return DOS version: Minor:Major in AH:AL */
            setAX(PspBlock->DosVersion);

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
                setAL(0xFF);
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

        /* Create File */
        case 0x3C:
        {
            WORD FileHandle;
            WORD ErrorCode = DosCreateFile(&FileHandle,
                                           (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                           getCX());

            if (ErrorCode == 0)
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

        /* Open File */
        case 0x3D:
        {
            WORD FileHandle;
            WORD ErrorCode = DosOpenFile(&FileHandle,
                                         (LPCSTR)SEG_OFF_TO_PTR(getDS(), getDX()),
                                         getAL());

            if (ErrorCode == 0)
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

        /* Close File */
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
            WORD Handle    = getBX();
            LPBYTE Buffer  = (LPBYTE)SEG_OFF_TO_PTR(getDS(), getDX());
            WORD Count     = getCX();
            WORD BytesRead = 0;
            WORD ErrorCode = ERROR_SUCCESS;

            if (IsConsoleHandle(DosGetRealHandle(Handle)))
            {
                while (Stack[STACK_COUNTER] < Count)
                {
                    /* Read a character from the BIOS */
                    // FIXME: Security checks!
                    Buffer[Stack[STACK_COUNTER]] = LOBYTE(BiosGetCharacter());

                    /* Stop if the BOP needs to be repeated */
                    if (getCF()) break;

                    /* Increment the counter */
                    Stack[STACK_COUNTER]++;
                }

                if (Stack[STACK_COUNTER] < Count)
                    ErrorCode = ERROR_NOT_READY;
                else
                    BytesRead = Count;
            }
            else
            {
                /* Use the file reading function */
                ErrorCode = DosReadFile(Handle, Buffer, Count, &BytesRead);
            }

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
                                          SEG_OFF_TO_PTR(getDS(), getDX()),
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

            /* Call the API function */
            if (DeleteFileA(FileName))
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
            HANDLE Handle = DosGetRealHandle(getBX());

            if (Handle != INVALID_HANDLE_VALUE)
            {
                /* The handle is invalid */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                setAX(ERROR_INVALID_HANDLE);
                break;
            }

            /* Open a new handle to the same entry */
            NewHandle = DosOpenHandle(Handle);

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

        /* Terminate With Return Code */
        case 0x4C:
        {
            DosTerminateProcess(CurrentPsp, getAL());
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

        /* Unsupported */
        default:
        {
            DPRINT1("DOS Function INT 0x21, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
                    getAH(), getAL());
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

VOID WINAPI DosBreakInterrupt(LPWORD Stack)
{
    UNREFERENCED_PARAMETER(Stack);

    VdmRunning = FALSE;
}

VOID WINAPI DosInt2Fh(LPWORD Stack)
{
    DPRINT1("DOS System Function INT 0x2F, AH = %xh, AL = %xh NOT IMPLEMENTED!\n",
            getAH(), getAL());
    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
}

BOOLEAN DosInitialize(VOID)
{
    BYTE i;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);
    FILE *Stream;
    WCHAR Buffer[256];
    LPWSTR SourcePtr, Environment;
    LPSTR AsciiString;
    LPSTR DestPtr = (LPSTR)SEG_OFF_TO_PTR(SYSTEM_ENV_BLOCK, 0);
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
        DosSystemFileTable[i] = INVALID_HANDLE_VALUE;
        DosSftRefCount[i] = 0;
    }

    /* Get handles to standard I/O devices */
    DosSystemFileTable[0] = GetStdHandle(STD_INPUT_HANDLE);
    DosSystemFileTable[1] = GetStdHandle(STD_OUTPUT_HANDLE);
    DosSystemFileTable[2] = GetStdHandle(STD_ERROR_HANDLE);

    /* Register the DOS-32 Interrupts */
    RegisterInt32(0x20, DosInt20h        );
    RegisterInt32(0x21, DosInt21h        );
    RegisterInt32(0x23, DosBreakInterrupt);
    RegisterInt32(0x2F, DosInt2Fh        );

    return TRUE;
}

/* EOF */
