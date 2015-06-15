/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/memory.c
 * PURPOSE:         DOS32 Memory Manager
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "dos.h"
#include "dos/dem.h"
#include "memory.h"
#include "process.h"

/* PUBLIC VARIABLES ***********************************************************/

BOOLEAN DosUmbLinked = FALSE;

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

/* PUBLIC FUNCTIONS ***********************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable)
{
    WORD Result = 0, Segment = FIRST_MCB_SEGMENT, MaxSize = 0;
    PDOS_MCB CurrentMcb;
    BOOLEAN SearchUmb = FALSE;

    DPRINT("DosAllocateMemory: Size 0x%04X\n", Size);

    if (DosUmbLinked && (Sda->AllocStrategy & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW)))
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
            Sda->LastErrorCode = ERROR_ARENA_TRASHED;
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

        switch (Sda->AllocStrategy & 0x3F)
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
            if ((Result == 0) && SearchUmb && (Sda->AllocStrategy & DOS_ALLOC_HIGH_LOW))
            {
                /* Search low memory */
                Segment = FIRST_MCB_SEGMENT;
                SearchUmb = FALSE;
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
        Sda->LastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        if (MaxAvailable) *MaxAvailable = MaxSize;
        return 0;
    }

    /* Get a pointer to the MCB */
    CurrentMcb = SEGMENT_TO_MCB(Result);

    /* Check if the block is larger than requested */
    if (CurrentMcb->Size > Size)
    {
        /* It is, split it into two blocks */
        if ((Sda->AllocStrategy & 0x3F) != DOS_ALLOC_LAST_FIT)
        {
            PDOS_MCB NextMcb = SEGMENT_TO_MCB(Result + Size + 1);

            /* Initialize the new MCB structure */
            NextMcb->BlockType = CurrentMcb->BlockType;
            NextMcb->Size = CurrentMcb->Size - Size - 1;
            NextMcb->OwnerPsp = 0;

            /* Update the current block */
            CurrentMcb->BlockType = 'M';
            CurrentMcb->Size = Size;
        }
        else
        {
            /* Save the location of the current MCB */
            PDOS_MCB PreviousMcb = CurrentMcb;

            /* Move the current MCB higher */
            Result += CurrentMcb->Size - Size;
            CurrentMcb = SEGMENT_TO_MCB(Result);

            /* Initialize the new MCB structure */
            CurrentMcb->BlockType = PreviousMcb->BlockType;
            CurrentMcb->Size = Size;
            CurrentMcb->OwnerPsp = 0;

            /* Update the previous block */
            PreviousMcb->BlockType = 'M';
            PreviousMcb->Size -= Size + 1;
        }
    }

    /* Take ownership of the block */
    CurrentMcb->OwnerPsp = Sda->CurrentPsp;

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
    if (BlockData == 0
        || (Mcb->BlockType != 'M' && Mcb->BlockType != 'Z')
        || Mcb->OwnerPsp == 0)
    {
        Success = FALSE;
        Sda->LastErrorCode = ERROR_INVALID_HANDLE;
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
            Sda->LastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            Success = FALSE;
            goto Done;
        }

        /* Combine this free block with adjoining free blocks */
        DosCombineFreeBlocks(NextSegment);

        /* Set the maximum possible size of the block */
        ReturnSize += NextMcb->Size + 1;

        if (ReturnSize < NewSize)
        {
            DPRINT("Cannot expand memory block: insufficient free segments available!\n");
            Sda->LastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            Success = FALSE;
            goto Done;
        }

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
    if (BlockData == 0) return FALSE;

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

VOID DosChangeMemoryOwner(WORD Segment, WORD NewOwner)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment - 1);

    /* Just set the owner */
    Mcb->OwnerPsp = NewOwner;
}

VOID DosInitializeMemory(VOID)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(FIRST_MCB_SEGMENT);

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
}

/* EOF */
