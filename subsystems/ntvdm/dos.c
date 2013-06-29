/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos.c
 * PURPOSE:         VDM DOS Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "dos.h"
#include "bios.h"
#include "emulator.h"

/* PRIVATE VARIABLES **********************************************************/

static WORD CurrentPsp = SYSTEM_PSP;
static DWORD DiskTransferArea;

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

static WORD DosCopyEnvironmentBlock(WORD SourceSegment)
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

    /* Allocate the memory for the environment block */
    DestSegment = DosAllocateMemory((TotalSize + 0x0F) >> 4, NULL);
    if (!DestSegment) return 0;

    Ptr = SourceBuffer;

    DestBuffer = (PCHAR)((ULONG_PTR)BaseAddress + TO_LINEAR(DestSegment, 0));
    while (*Ptr)
    {
        /* Copy the string */
        strcpy(DestBuffer, Ptr);

        /* Advance to the next string */
        Ptr += strlen(Ptr) + 1;
        DestBuffer += strlen(Ptr) + 1;
    }

    /* Set the final zero */
    *DestBuffer = 0;

    return DestSegment;
}

static VOID DosChangeMemoryOwner(WORD Segment, WORD NewOwner)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment - 1);

    /* Just set the owner */
    Mcb->OwnerPsp = NewOwner;
}

/* PUBLIC FUNCTIONS ***********************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable)
{
    WORD Result = 0, Segment = FIRST_MCB_SEGMENT, MaxSize = 0;
    PDOS_MCB CurrentMcb, NextMcb;

    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (CurrentMcb->BlockType != 'M' && CurrentMcb->BlockType != 'Z')
        {
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

        /* It is, update the smallest found so far */
        if ((Result == 0) || (CurrentMcb->Size < SEGMENT_TO_MCB(Result)->Size))
        {
            Result = Segment;
        }

Next:
        /* If this was the last MCB in the chain, quit */
        if (CurrentMcb->BlockType == 'Z') break;

        /* Otherwise, update the segment and continue */
        Segment += CurrentMcb->Size + 1;
    }

    /* If we didn't find a free block, return 0 */
    if (Result == 0)
    {
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

    /* Make sure this is a valid, allocated block */
    if ((Mcb->BlockType != 'M' && Mcb->BlockType != 'Z') || Mcb->OwnerPsp == 0)
    {
        Success = FALSE;
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
        /* Return the maximum possible size */
        if (MaxAvailable) *MaxAvailable = ReturnSize;
    }
    
    return Success;
}

BOOLEAN DosFreeMemory(WORD BlockData)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(BlockData - 1);

    /* Make sure the MCB is valid */
    if (Mcb->BlockType != 'M' && Mcb->BlockType != 'Z') return FALSE;

    /* Mark the block as free */
    Mcb->OwnerPsp = 0;

    return TRUE;
}

WORD DosCreateFile(LPCSTR FilePath)
{
    // TODO: NOT IMPLEMENTED
    return 0;
}

WORD DosOpenFile(LPCSTR FilePath)
{
    // TODO: NOT IMPLEMENTED
    return 0;
}

VOID DosInitializePsp(WORD PspSegment, LPCSTR CommandLine, WORD ProgramSize, WORD Environment)
{
    INT i;
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(PspSegment);
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);

    ZeroMemory(PspBlock, sizeof(DOS_PSP));

    /* Set the exit interrupt */
    PspBlock->Exit[0] = 0xCD; // int 0x20
    PspBlock->Exit[1] = 0x20;

    /* Set the program size */
    PspBlock->MemSize = ProgramSize;

    /* Save the interrupt vectors */
    PspBlock->TerminateAddress = IntVecTable[0x22];
    PspBlock->BreakAddress = IntVecTable[0x23];
    PspBlock->CriticalAddress = IntVecTable[0x24];

    /* Set the parent PSP */
    PspBlock->ParentPsp = CurrentPsp;

    /* Initialize the handle table */
    for (i = 0; i < 20; i++) PspBlock->HandleTable[i] = 0xFF;

    

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
    PspBlock->CommandLineSize = strlen(CommandLine);
    RtlCopyMemory(PspBlock->CommandLine, CommandLine, PspBlock->CommandLineSize);
    PspBlock->CommandLine[PspBlock->CommandLineSize] = '\r';
}

BOOLEAN DosCreateProcess(LPCSTR CommandLine, WORD EnvBlock)
{
    BOOLEAN Success = FALSE, AllocatedEnvBlock = FALSE;
    HANDLE FileHandle = INVALID_HANDLE_VALUE, FileMapping = NULL;
    LPBYTE Address = NULL;
    LPSTR ProgramFilePath, Parameters[128];
    CHAR CommandLineCopy[128];
    INT ParamCount = 0;
    WORD i, Segment = 0, FileSize, ExeSize;
    PIMAGE_DOS_HEADER Header;
    PDWORD RelocationTable;
    PWORD RelocWord;

    /* Save a copy of the command line */
    strcpy(CommandLineCopy, CommandLine);

    /* Get the file name of the executable */
    ProgramFilePath = strtok(CommandLineCopy, " \t");

    /* Load the parameters in the local array */
    while ((ParamCount < 256)
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
                                           : SYSTEM_ENV_BLOCK);
    }

    /* Check if this is an EXE file or a COM file */
    if (Address[0] == 'M' && Address[1] == 'Z')
    {
        /* EXE file */

        /* Get the MZ header */
        Header = (PIMAGE_DOS_HEADER)Address;

        // TODO: Verify checksum and executable!

        /* Get the base size of the file, in paragraphs (rounded up) */
        ExeSize = (((Header->e_cp - 1) << 8) + Header->e_cblp + 0x0F) >> 4;

        /* Loop from the maximum to the minimum number of extra paragraphs */
        for (i = Header->e_maxalloc; i >= Header->e_minalloc; i--)
        {
            /* Try to allocate that much memory */
            Segment = DosAllocateMemory(ExeSize + (sizeof(DOS_PSP) >> 4) + i, NULL);
            if (Segment != 0) break;
        }

        /* Check if at least the lowest allocation was successful */
        if (Segment == 0) goto Cleanup;

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine, ExeSize + (sizeof(DOS_PSP) >> 4) + i,
                         EnvBlock);

        /* The process owns its own memory */
        DosChangeMemoryOwner(Segment, Segment);
        DosChangeMemoryOwner(EnvBlock, Segment);

        /* Copy the program to Segment:0100 */
        RtlCopyMemory((PVOID)((ULONG_PTR)BaseAddress
                      + TO_LINEAR(Segment, 0x100)),
                      Address + (Header->e_cparhdr << 4),
                      FileSize - (Header->e_cparhdr << 4));

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

        /* Allocate memory for the whole program and the PSP */
        Segment = DosAllocateMemory((FileSize + sizeof(DOS_PSP)) >> 4, NULL);
        if (Segment == 0) goto Cleanup;

        /* Copy the program to Segment:0100 */
        RtlCopyMemory((PVOID)((ULONG_PTR)BaseAddress
                      + TO_LINEAR(Segment, 0x100)),
                      Address,
                      FileSize);

        /* Initialize the PSP */
        DosInitializePsp(Segment,
                         CommandLine,
                         (FileSize + sizeof(DOS_PSP)) >> 4,
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
    WORD McbSegment = FIRST_MCB_SEGMENT;
    PDOS_MCB CurrentMcb;
    LPDWORD IntVecTable = (LPDWORD)((ULONG_PTR)BaseAddress);
    PDOS_PSP PspBlock = SEGMENT_TO_PSP(Psp);

    /* Check if this PSP is it's own parent */
    if (PspBlock->ParentPsp == Psp) goto Done;

    // TODO: Close all handles opened by the process

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

CHAR DosReadCharacter()
{
    // TODO: STDIN can be redirected under DOS 2.0+
    CHAR Character = 0;

    /* A zero value for the character indicates a special key */
    do Character = BiosGetCharacter();
    while (!Character);

    return Character;
}

VOID DosPrintCharacter(CHAR Character)
{
    // TODO: STDOUT can be redirected under DOS 2.0+
    if (Character == '\r') Character = '\n';
    putchar(Character);
}

VOID DosInt20h(WORD CodeSegment)
{
    /* This is the exit interrupt */
    DosTerminateProcess(CodeSegment, 0);
}

VOID DosInt21h(WORD CodeSegment)
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
    WORD DataSegment = EmulatorGetRegister(EMULATOR_REG_DS);
    WORD ExtSegment = EmulatorGetRegister(EMULATOR_REG_ES);

    /* Check the value in the AH register */
    switch (HIBYTE(Eax))
    {
        /* Terminate Program */
        case 0x00:
        {
            DosTerminateProcess(CodeSegment, 0);
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
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else
            {
                EmulatorSetFlag(EMULATOR_FLAG_CF);
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
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else
            {
                EmulatorSetFlag(EMULATOR_FLAG_CF);
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

            if (SetCurrentDirectoryA(String))
            {
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else
            {
                EmulatorSetFlag(EMULATOR_FLAG_CF);
                EmulatorSetRegister(EMULATOR_REG_AX,
                                    (Eax & 0xFFFF0000) | LOWORD(GetLastError()));
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
                EmulatorSetRegister(EMULATOR_REG_BX, MaxAvailable);
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else EmulatorSetFlag(EMULATOR_FLAG_CF);

            break;
        }

        /* Free Memory */
        case 0x49:
        {
            if (DosFreeMemory(ExtSegment))
            {
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else EmulatorSetFlag(EMULATOR_FLAG_CF);

            break;
        }

        /* Resize Memory Block */
        case 0x4A:
        {
            WORD Size;

            if (DosResizeMemory(ExtSegment, LOWORD(Ebx), &Size))
            {
                EmulatorClearFlag(EMULATOR_FLAG_CF);
            }
            else
            {
                EmulatorSetFlag(EMULATOR_FLAG_CF);
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

        /* Unsupported */
        default:
        {
            DPRINT1("DOS Function INT 0x21, AH = 0x%02X NOT IMPLEMENTED!\n", HIBYTE(Eax));
            EmulatorSetFlag(EMULATOR_FLAG_CF);
        }
    }
}

VOID DosBreakInterrupt()
{
    VdmRunning = FALSE;
}

BOOLEAN DosInitialize()
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);
    FILE *Stream;
    WCHAR Buffer[256];
    LPWSTR SourcePtr, Environment;
    LPSTR AsciiString;
    LPSTR DestPtr = (LPSTR)((ULONG_PTR)BaseAddress + TO_LINEAR(SYSTEM_ENV_BLOCK, 0));
    DWORD AsciiSize;

    /* Initialize the MCB */
    Mcb->BlockType = 'Z';
    Mcb->Size = (WORD)USER_MEMORY_SIZE;
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

        /* Free the memory */
        HeapFree(GetProcessHeap(), 0, AsciiString);

        /* Move to the next string */
        SourcePtr += wcslen(SourcePtr) + 1;
        DestPtr += strlen(AsciiString) + 1;
    }

    /* Free the memory allocated for environment strings */
    FreeEnvironmentStringsW(Environment);

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

    return TRUE;
}

/* EOF */
