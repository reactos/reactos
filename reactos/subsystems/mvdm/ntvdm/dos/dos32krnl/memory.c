/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/memory.c
 * PURPOSE:         DOS32 Memory Manager
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "bios/umamgr.h" // HACK until we correctly call XMS services for UMBs.

#include "dos.h"
#include "dos/dem.h"
#include "memory.h"
#include "process.h"
#include "himem.h"

// FIXME: Should be dynamically initialized!
#define FIRST_MCB_SEGMENT   (SYSTEM_ENV_BLOCK + 0x200)  // old value: 0x1000
#define USER_MEMORY_SIZE    (0x9FFE - FIRST_MCB_SEGMENT)

/*
 * Activate this line if you want run-time DOS memory arena integrity validation
 * (useful to know whether this is an application, or DOS kernel itself, which
 * messes up the DOS memory arena).
 */
// #define DBG_MEMORY

/* PRIVATE VARIABLES **********************************************************/

/* PUBLIC VARIABLES ***********************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

static inline BOOLEAN ValidateMcb(PDOS_MCB Mcb)
{
    return (Mcb->BlockType == 'M' || Mcb->BlockType == 'Z');
}

/*
 * This is a helper function to help us detecting
 * when the DOS arena starts to become corrupted.
 */
#ifdef DBG_MEMORY
static VOID DosMemValidate(VOID)
{
    WORD PrevSegment, Segment = SysVars->FirstMcb;
    PDOS_MCB CurrentMcb;

    PrevSegment = Segment;
    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (!ValidateMcb(CurrentMcb))
        {
            DPRINT1("The DOS memory arena is corrupted! (CurrentMcb = 0x%04X; PreviousMcb = 0x%04X)\n", Segment, PrevSegment);
            return;
        }

        PrevSegment = Segment;

        /* If this was the last MCB in the chain, quit */
        if (CurrentMcb->BlockType == 'Z') return;

        /* Otherwise, update the segment and continue */
        Segment += CurrentMcb->Size + 1;
    }
}
#else
#define DosMemValidate()
#endif

static VOID DosCombineFreeBlocks(WORD StartBlock)
{
    /* NOTE: This function is always called with valid MCB blocks */

    PDOS_MCB CurrentMcb = SEGMENT_TO_MCB(StartBlock), NextMcb;

    /* If the block is not free, quit */
    if (CurrentMcb->OwnerPsp != 0) return;

    /*
     * Loop while the current block is not the last one. It can happen
     * that the block is not the last one at the beginning, but becomes
     * the last one at the end of the process. This happens in the case
     * where its next following blocks are free but not combined yet,
     * and they are terminated by a free last block. During the process
     * all the blocks are combined together and we end up in the situation
     * where the current (free) block is followed by the last (free) block.
     * At the last step of the algorithm the current block becomes the
     * last one.
     */
    while (CurrentMcb->BlockType != 'Z')
    {
        /* Get a pointer to the next MCB */
        NextMcb = SEGMENT_TO_MCB(StartBlock + CurrentMcb->Size + 1);

        /* Make sure it's valid */
        if (!ValidateMcb(NextMcb))
        {
            DPRINT1("The DOS memory arena is corrupted!\n");
            // Sda->LastErrorCode = ERROR_ARENA_TRASHED;
            return;
        }

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
    WORD Result = 0, Segment = SysVars->FirstMcb, MaxSize = 0;
    PDOS_MCB CurrentMcb;
    BOOLEAN SearchUmb = FALSE;

    DPRINT("DosAllocateMemory: Size 0x%04X\n", Size);

    DosMemValidate();

    if (SysVars->UmbLinked && SysVars->UmbChainStart != 0xFFFF &&
        (Sda->AllocStrategy & (DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW)))
    {
        /* Search UMB first */
        Segment = SysVars->UmbChainStart;
        SearchUmb = TRUE;
    }

    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (!ValidateMcb(CurrentMcb))
        {
            DPRINT1("The DOS memory arena is corrupted!\n");
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

        switch (Sda->AllocStrategy & ~(DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW))
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
                Segment = SysVars->FirstMcb;
                SearchUmb = FALSE;
                continue;
            }

            break;
        }

        /* Otherwise, update the segment and continue */
        Segment += CurrentMcb->Size + 1;
    }

Done:
    DosMemValidate();

    /* If we didn't find a free block, bail out */
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
        if ((Sda->AllocStrategy & ~(DOS_ALLOC_HIGH | DOS_ALLOC_HIGH_LOW)) != DOS_ALLOC_LAST_FIT)
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
    RtlCopyMemory(CurrentMcb->Name, SEGMENT_TO_MCB(Sda->CurrentPsp - 1)->Name, sizeof(CurrentMcb->Name));

    DosMemValidate();

    /* Return the segment of the data portion of the block */
    return Result + 1;
}

BOOLEAN DosResizeMemory(WORD BlockData, WORD NewSize, WORD *MaxAvailable)
{
    BOOLEAN Success = TRUE;
    WORD Segment = BlockData - 1, ReturnSize = 0, NextSegment;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment), NextMcb;

    DPRINT("DosResizeMemory: BlockData 0x%04X, NewSize 0x%04X\n",
           BlockData, NewSize);

    DosMemValidate();

    /* Make sure this is a valid and allocated block */
    if (BlockData == 0 || !ValidateMcb(Mcb) || Mcb->OwnerPsp == 0)
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
        if (Mcb->BlockType == 'Z')
        {
            Success = FALSE;
            goto Done;
        }

        /* Get the pointer and segment of the next MCB */
        NextSegment = Segment + Mcb->Size + 1;
        NextMcb = SEGMENT_TO_MCB(NextSegment);

        /* Make sure it's valid */
        if (!ValidateMcb(NextMcb))
        {
            DPRINT1("The DOS memory arena is corrupted!\n");
            Sda->LastErrorCode = ERROR_ARENA_TRASHED;
            return FALSE;
        }

        /* Make sure the next segment is free */
        if (NextMcb->OwnerPsp != 0)
        {
            DPRINT("Cannot expand memory block 0x%04X: next segment is not free!\n", Segment);
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
            DPRINT("Cannot expand memory block 0x%04X: insufficient free segments available!\n", Segment);
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
                   Mcb->Size, NewSize);

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
                Mcb->Size, NewSize);

        /* Just split the block */
        NextSegment = Segment + NewSize + 1;
        NextMcb = SEGMENT_TO_MCB(NextSegment);
        NextMcb->BlockType = Mcb->BlockType;
        NextMcb->Size = Mcb->Size - NewSize - 1;
        NextMcb->OwnerPsp = 0;

        /* Update the MCB */
        Mcb->BlockType = 'M';
        Mcb->Size = NewSize;

        /* Combine this free block with adjoining free blocks */
        DosCombineFreeBlocks(NextSegment);
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

    DosMemValidate();

    return Success;
}

BOOLEAN DosFreeMemory(WORD BlockData)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(BlockData - 1);

    DPRINT("DosFreeMemory: BlockData 0x%04X\n", BlockData);
    if (BlockData == 0) return FALSE;

    DosMemValidate();

    /* Make sure the MCB is valid */
    if (!ValidateMcb(Mcb))
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
    DWORD Segment = SysVars->FirstMcb;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment);

    DPRINT("Linking UMB\n");

    /* Check if UMBs are initialized and already linked */
    if (SysVars->UmbChainStart == 0xFFFF) return FALSE;
    if (SysVars->UmbLinked) return TRUE;

    DosMemValidate();

    /* Find the last block before the start of the UMB chain */
    while (Segment < SysVars->UmbChainStart)
    {
        /* Get a pointer to the MCB */
        Mcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (!ValidateMcb(Mcb))
        {
            DPRINT1("The DOS memory arena is corrupted!\n");
            Sda->LastErrorCode = ERROR_ARENA_TRASHED;
            return FALSE;
        }

        /* If this was the last MCB in the chain, quit */
        if (Mcb->BlockType == 'Z') break;

        /* Otherwise, update the segment and continue */
        Segment += Mcb->Size + 1;
    }

    /* Make sure it's valid */
    if (Mcb->BlockType != 'Z') return FALSE;

    /* Connect the MCB with the UMB chain */
    Mcb->BlockType = 'M';

    DosMemValidate();

    SysVars->UmbLinked = TRUE;
    return TRUE;
}

BOOLEAN DosUnlinkUmb(VOID)
{
    DWORD Segment = SysVars->FirstMcb;
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment);

    DPRINT("Unlinking UMB\n");

    /* Check if UMBs are initialized and already unlinked */
    if (SysVars->UmbChainStart == 0xFFFF) return FALSE;
    if (!SysVars->UmbLinked) return TRUE;

    DosMemValidate();

    /* Find the last block before the start of the UMB chain */
    while (Segment < SysVars->UmbChainStart)
    {
        /* Get a pointer to the MCB */
        Mcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (!ValidateMcb(Mcb))
        {
            DPRINT1("The DOS memory arena is corrupted!\n");
            Sda->LastErrorCode = ERROR_ARENA_TRASHED;
            return FALSE;
        }

        /* Advance to the next MCB */
        Segment += Mcb->Size + 1;
    }

    /* Mark the MCB as the last MCB */
    Mcb->BlockType = 'Z';

    DosMemValidate();

    SysVars->UmbLinked = FALSE;
    return TRUE;
}

VOID DosChangeMemoryOwner(WORD Segment, WORD NewOwner)
{
    PDOS_MCB Mcb = SEGMENT_TO_MCB(Segment - 1);
    Mcb->OwnerPsp = NewOwner;
}

/*
 * Some information about DOS UMBs:
 * http://textfiles.com/virus/datut010.txt
 * http://www.asmcommunity.net/forums/topic/?id=30884
 */

WORD DosGetPreviousUmb(WORD UmbSegment)
{
    PDOS_MCB CurrentMcb;
    WORD Segment, PrevSegment = 0; // FIXME: or use UmbChainStart ??

    if (SysVars->UmbChainStart == 0xFFFF)
        return 0;

    /* Start scanning the UMB chain */
    Segment = SysVars->UmbChainStart;
    while (TRUE)
    {
        /* Get a pointer to the MCB */
        CurrentMcb = SEGMENT_TO_MCB(Segment);

        /* Make sure it's valid */
        if (!ValidateMcb(CurrentMcb))
        {
            DPRINT1("The UMB DOS memory arena is corrupted!\n");
            Sda->LastErrorCode = ERROR_ARENA_TRASHED;
            return 0;
        }

        /* We went over the UMB segment, quit */
        if (Segment >= UmbSegment) break;

        PrevSegment = Segment;

        /* If this was the last MCB in the chain, quit */
        if (CurrentMcb->BlockType == 'Z') break;

        /* Otherwise, update the segment and continue */
        Segment += CurrentMcb->Size + 1;
    }

    return PrevSegment;
}

VOID DosInitializeUmb(VOID)
{
    BOOLEAN Result;
    USHORT UmbSegment = 0x0000, PrevSegment;
    USHORT Size;
    PDOS_MCB Mcb, PrevMcb;

    ASSERT(SysVars->UmbChainStart == 0xFFFF);

    // SysVars->UmbLinked = FALSE;

    /* Try to allocate all the UMBs */
    while (TRUE)
    {
        /* Find the maximum amount of memory that can be allocated */
        Size = 0xFFFF;
        Result = UmaDescReserve(&UmbSegment, &Size);

        /* If we are out of UMBs, bail out */
        if (!Result && Size == 0) // XMS_STATUS_OUT_OF_UMBS
            break;

        /* We should not have succeeded! */
        ASSERT(!Result);

        /* 'Size' now contains the size of the biggest UMB block. Request it. */
        Result = UmaDescReserve(&UmbSegment, &Size);
        ASSERT(Result); // XMS_STATUS_SUCCESS

        /* If this is our first UMB block, initialize the UMB chain */
        if (SysVars->UmbChainStart == 0xFFFF)
        {
            /* Initialize the link MCB to the UMB area */
            // NOTE: We use the fact that UmbChainStart is still == 0xFFFF
            // so that we initialize this block from 9FFF:0000 up to FFFF:000F.
            // It will be splitted as needed just below.
            Mcb = SEGMENT_TO_MCB(SysVars->FirstMcb + USER_MEMORY_SIZE + 1); // '+1': Readjust the fact that USER_MEMORY_SIZE is based using 0x9FFE instead of 0x9FFF
            Mcb->BlockType = 'Z'; // At the moment it is really the last block
            Mcb->Size = (SysVars->UmbChainStart /* UmbSegment */ - SysVars->FirstMcb - USER_MEMORY_SIZE - 2) + 1;
            Mcb->OwnerPsp = SYSTEM_PSP;
            RtlCopyMemory(Mcb->Name, "SC      ", sizeof("SC      ")-1);

#if 0 // Keep here for reference; this will be deleted as soon as it becomes unneeded.
            /* Initialize the UMB area */
            Mcb = SEGMENT_TO_MCB(SysVars->UmbChainStart);
            Mcb->Size = UMB_END_SEGMENT - SysVars->UmbChainStart;
#endif

            // FIXME: We should adjust the size of the previous block!!

            /* Initialize the start of the UMB chain */
            SysVars->UmbChainStart = SysVars->FirstMcb + USER_MEMORY_SIZE + 1;
        }

        /* Split the block */

        /* Get the previous block */
        PrevSegment = DosGetPreviousUmb(UmbSegment);
        PrevMcb = SEGMENT_TO_MCB(PrevSegment);

        /* Initialize the next block */
        Mcb = SEGMENT_TO_MCB(UmbSegment + /*Mcb->Size*/(Size - 1) + 0);
        // Mcb->BlockType = 'Z'; // FIXME: What if this block happens to be the last one??
        Mcb->BlockType = PrevMcb->BlockType;
        Mcb->Size = PrevMcb->Size - (UmbSegment + Size - PrevSegment) + 1;
        Mcb->OwnerPsp = PrevMcb->OwnerPsp;
        RtlCopyMemory(Mcb->Name, PrevMcb->Name, sizeof(PrevMcb->Name));

        /* The previous block is not the latest one anymore */
        PrevMcb->BlockType = 'M';
        PrevMcb->Size = UmbSegment - PrevSegment - 1;

        /* Initialize the new UMB block */
        Mcb = SEGMENT_TO_MCB(UmbSegment);
        Mcb->BlockType = 'M'; // 'Z' // FIXME: What if this block happens to be the last one??
        Mcb->Size = Size - 1 - 1; // minus 2 because we need to have one arena at the beginning and one at the end.
        Mcb->OwnerPsp = 0;
        // FIXME: Which MCB name should we use? I need to explore more the docs!
        RtlCopyMemory(Mcb->Name, "UMB     ", sizeof("UMB     ")-1);
        // RtlCopyMemory(Mcb->Name, "SM      ", sizeof("SM      ")-1);
    }
}

VOID DosInitializeMemory(VOID)
{
    PDOS_MCB Mcb;

    /* Set the initial allocation strategy to "best fit" */
    Sda->AllocStrategy = DOS_ALLOC_BEST_FIT;

    /* Initialize conventional memory; we don't have UMBs yet */
    SysVars->FirstMcb = FIRST_MCB_SEGMENT; // The Arena Head
    SysVars->UmbLinked = FALSE;
    SysVars->UmbChainStart = 0xFFFF;

    Mcb = SEGMENT_TO_MCB(SysVars->FirstMcb);

    /* Initialize the MCB */
    Mcb->BlockType = 'Z';
    Mcb->Size = USER_MEMORY_SIZE;
    Mcb->OwnerPsp = 0;
}

/* EOF */
