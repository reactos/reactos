/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/umamgr.c
 * PURPOSE:         Upper Memory Area Manager
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: The UMA Manager is used by the DOS XMS Driver (UMB Provider part),
 *       indirectly by the DOS EMS Driver, and by VDD memory management functions.
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "memory.h"

#include "umamgr.h"

/* PRIVATE VARIABLES **********************************************************/

typedef struct _UMA_DESCRIPTOR
{
    LIST_ENTRY Entry;
    ULONG Start;
    ULONG Size;
    UMA_DESC_TYPE Type;
} UMA_DESCRIPTOR, *PUMA_DESCRIPTOR;

/*
 * Sorted list of UMA descriptors.
 *
 * The descriptor list is (and must always be) sorted by memory addresses,
 * and all consecutive free descriptors are always merged together, so that
 * free ones are always separated from each other by descriptors of other types.
 *
 * TODO: Add the fact that no blocks of size == 0 are allowed.
 */
static LIST_ENTRY UmaDescriptorList = { &UmaDescriptorList, &UmaDescriptorList };

/* PRIVATE FUNCTIONS **********************************************************/

static PUMA_DESCRIPTOR
CreateUmaDescriptor(IN OUT PLIST_ENTRY ListHead,
                    IN ULONG Address,
                    IN ULONG Size,
                    IN UMA_DESC_TYPE Type)
{
    PUMA_DESCRIPTOR UmaDesc;

    ASSERT(Size > 0);

    UmaDesc = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*UmaDesc));
    if (!UmaDesc) return NULL;

    UmaDesc->Start = Address;
    UmaDesc->Size  = Size;
    UmaDesc->Type  = Type;

    /*
     * We use the trick of https://www.osronline.com/article.cfm%5earticle=499.htm to insert
     * the new descriptor just after the current entry that we specify via 'ListHead'.
     * If 'ListHead' is NULL then we insert the descriptor at the tail of 'UmaDescriptorList'
     * (which is equivalent to inserting it at the head of 'UmaDescriptorList.Blink').
     */
    if (ListHead == NULL) ListHead = UmaDescriptorList.Blink;
    InsertHeadList(ListHead, &UmaDesc->Entry);

    return UmaDesc;
}

static VOID FreeUmaDescriptor(PUMA_DESCRIPTOR UmaDesc)
{
    RemoveEntryList(&UmaDesc->Entry);
    RtlFreeHeap(RtlGetProcessHeap(), 0, UmaDesc);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN UmaDescReserve(IN OUT PUSHORT Segment, IN OUT PUSHORT Size)
{
    ULONG Address =  (*Segment << 4); // Convert segment number into address.
    ULONG RequestSize = (*Size << 4); // Convert size in paragraphs into size in bytes.
    ULONG MaxSize = 0;
    PLIST_ENTRY Entry;
    PUMA_DESCRIPTOR UmaDesc, NewUmaDesc;
    PUMA_DESCRIPTOR FoundUmaDesc = NULL;

    // FIXME: Check! What to do?
    if (RequestSize == 0) DPRINT1("Requesting UMA descriptor with null size?!\n");

    Entry = UmaDescriptorList.Flink;
    while (Entry != &UmaDescriptorList)
    {
        UmaDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(Entry, UMA_DESCRIPTOR, Entry);
        Entry = Entry->Flink;

        /* Only check free descriptors */
        if (UmaDesc->Type != UMA_FREE) continue;

        /* Update the maximum descriptor size */
        if (UmaDesc->Size > MaxSize) MaxSize = UmaDesc->Size;

        /* Descriptor too small, continue... */
        if (UmaDesc->Size < RequestSize) continue;

        /* Do we want to reserve the descriptor at a precise address? */
        if (Address)
        {
            /* If the descriptor ends before the desired region, try again */
            if (UmaDesc->Start + UmaDesc->Size <= Address) continue;

            /*
             * If we have a descriptor, but one of its boundaries crosses the
             * desired region (it starts after the desired region, or ends
             * before the end of the desired region), this means that there
             * is already something inside the region, so that we cannot
             * allocate the region here. Bail out.
             */
            if (UmaDesc->Start > Address ||
                UmaDesc->Start + UmaDesc->Size < Address + RequestSize)
            {
                goto Fail;
            }

            /* We now have a free descriptor that overlaps our desired region: split it */

            /*
             * Here, UmaDesc->Start == Address or UmaDesc->Start < Address,
             * in which case we need to split the descriptor to the left.
             */
            if (UmaDesc->Start < Address)
            {
                /* Create a new free descriptor and insert it after the current one */
                NewUmaDesc = CreateUmaDescriptor(&UmaDesc->Entry,
                                                 Address,
                                                 UmaDesc->Size - (Address - UmaDesc->Start),
                                                 UmaDesc->Type); // UMA_FREE
                if (!NewUmaDesc)
                {
                    DPRINT1("CreateUmaDescriptor failed, UMA descriptor list possibly corrupted!\n");
                    goto Fail;
                }

                /* Reduce the size of the splitted left descriptor */
                UmaDesc->Size = (Address - UmaDesc->Start);

                /* Current descriptor is now the new created one */
                UmaDesc = NewUmaDesc;
            }

            /* Here, UmaDesc->Start == Address */
        }

        /* Descriptor of large enough size: split it to the right if needed */
        // FIXME: It might be needed to consider a minimum size starting which we need to split.
        // if (UmaDesc->Size - RequestSize > (3 << 4))
        if (UmaDesc->Size > RequestSize)
        {
            /*
             * Create a new free descriptor and insert it after the current one.
             * Because consecutive free descriptors are always merged together,
             * the descriptor following 'UmaDesc' cannot be free, so that this
             * new free descriptor does not need to be merged with some others.
             */
            NewUmaDesc = CreateUmaDescriptor(&UmaDesc->Entry,
                                             UmaDesc->Start + RequestSize,
                                             UmaDesc->Size - RequestSize,
                                             UmaDesc->Type); // UMA_FREE
            if (!NewUmaDesc)
            {
                DPRINT1("CreateUmaDescriptor failed, UMA descriptor list possibly corrupted!\n");
                goto Fail;
            }

            /* Reduce the size of the splitted left descriptor */
            UmaDesc->Size = RequestSize;
        }

        /* We have a descriptor of correct size, initialize it */
        UmaDesc->Type = UMA_UMB;
        FoundUmaDesc = UmaDesc;
        break;
    }

    if (FoundUmaDesc)
    {
        /* Returned address is a segment and size is in paragraphs */
        *Segment = (FoundUmaDesc->Start >> 4);
        *Size    = (FoundUmaDesc->Size  >> 4);
        return TRUE;
    }
    else
    {
Fail:
        /* Returned address is a segment and size is in paragraphs */
        *Segment = 0x0000;
        *Size    = (MaxSize >> 4);
        return FALSE;
    }
}

BOOLEAN UmaDescRelease(IN USHORT Segment)
{
    ULONG Address = (Segment << 4); // Convert segment number into address.
    PLIST_ENTRY Entry, PrevEntry, NextEntry;
    PUMA_DESCRIPTOR UmaDesc, PrevDesc = NULL, NextDesc = NULL;
    PUMA_DESCRIPTOR FoundUmaDesc = NULL;

    Entry = UmaDescriptorList.Flink;
    while (Entry != &UmaDescriptorList)
    {
        UmaDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(Entry, UMA_DESCRIPTOR, Entry);
        Entry = Entry->Flink;

        /* Search for the descriptor in the list */
        if (UmaDesc->Start == Address && UmaDesc->Type == UMA_UMB)
        {
            /* We found it */
            FoundUmaDesc = UmaDesc;
            break;
        }
    }

    if (FoundUmaDesc)
    {
        FoundUmaDesc->Type = UMA_FREE;

        /* Combine free descriptors adjacent to this one */
        PrevEntry = FoundUmaDesc->Entry.Blink;
        NextEntry = FoundUmaDesc->Entry.Flink;

        if (PrevEntry != &UmaDescriptorList)
            PrevDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(PrevEntry, UMA_DESCRIPTOR, Entry);
        if (NextEntry != &UmaDescriptorList)
            NextDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(NextEntry, UMA_DESCRIPTOR, Entry);

        if (NextDesc && NextDesc->Type == FoundUmaDesc->Type) // UMA_FREE
        {
            FoundUmaDesc->Size += NextDesc->Size;
            FreeUmaDescriptor(NextDesc);
        }

        if (PrevDesc && PrevDesc->Type == FoundUmaDesc->Type) // UMA_FREE
        {
            PrevDesc->Size += FoundUmaDesc->Size;
            FreeUmaDescriptor(FoundUmaDesc);
        }

        return TRUE;
    }

    return FALSE;
}

BOOLEAN UmaDescReallocate(IN USHORT Segment, IN OUT PUSHORT Size)
{
    ULONG Address =   (Segment << 4); // Convert segment number into address.
    ULONG RequestSize = (*Size << 4); // Convert size in paragraphs into size in bytes.
    ULONG MaxSize = 0;
    PLIST_ENTRY Entry, NextEntry;
    PUMA_DESCRIPTOR UmaDesc, NextDesc = NULL, NewUmaDesc;
    PUMA_DESCRIPTOR FoundUmaDesc = NULL;

    // FIXME: Check! What to do?
    if (RequestSize == 0) DPRINT1("Resizing UMA descriptor %04X to null size?!\n", Segment);

    Entry = UmaDescriptorList.Flink;
    while (Entry != &UmaDescriptorList)
    {
        UmaDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(Entry, UMA_DESCRIPTOR, Entry);
        Entry = Entry->Flink;

        /* Only get the maximum size of free descriptors */
        if (UmaDesc->Type == UMA_FREE)
        {
            /* Update the maximum descriptor size */
            if (UmaDesc->Size > MaxSize) MaxSize = UmaDesc->Size;
        }

        /* Search for the descriptor in the list */
        if (UmaDesc->Start == Address && UmaDesc->Type == UMA_UMB)
        {
            /* We found it */
            FoundUmaDesc = UmaDesc;
            break;
        }
    }

    if (FoundUmaDesc)
    {
        /* If we do not resize anything, just quit with success */
        if (FoundUmaDesc->Size == RequestSize) goto Success;

        NextEntry = FoundUmaDesc->Entry.Flink;

        if (NextEntry != &UmaDescriptorList)
            NextDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(NextEntry, UMA_DESCRIPTOR, Entry);

        /* Check for reduction or enlargement */
        if (FoundUmaDesc->Size > RequestSize)
        {
            /* Reduction */

            /*
             * Check if the next descriptor is free, in which case we
             * extend it, otherwise we create a new free descriptor.
             */
            if (NextDesc && NextDesc->Type == UMA_FREE)
            {
                /* Yes it is, expand its size and move it down */
                NextDesc->Size  += (FoundUmaDesc->Size - RequestSize);
                NextDesc->Start -= (FoundUmaDesc->Size - RequestSize);
            }
            else
            {
                // FIXME: It might be needed to consider a minimum size starting which we need to split.
                // if (FoundUmaDesc->Size - RequestSize > (3 << 4))

                /* Create a new free descriptor and insert it after the current one */
                NewUmaDesc = CreateUmaDescriptor(&FoundUmaDesc->Entry,
                                                 FoundUmaDesc->Start + RequestSize,
                                                 FoundUmaDesc->Size - RequestSize,
                                                 FoundUmaDesc->Type);
                if (!NewUmaDesc)
                {
                    DPRINT1("CreateUmaDescriptor failed, UMA descriptor list possibly corrupted!\n");
                    MaxSize = 0;
                    goto Fail;
                }
            }
        }
        else // if (FoundUmaDesc->Size <= RequestSize)
        {
            /* Enlargement */

            /* Check whether the next descriptor is free and large enough for merging */
            if (NextDesc && NextDesc->Type == UMA_FREE &&
                FoundUmaDesc->Size + NextDesc->Size >= RequestSize)
            {
                /* Yes it is, reduce its size and move it up, and enlarge the reallocated descriptor */
                NextDesc->Size  -= (RequestSize - FoundUmaDesc->Size);
                NextDesc->Start += (RequestSize - FoundUmaDesc->Size);

                if (NextDesc->Size == 0) FreeUmaDescriptor(NextDesc);
            }
            else
            {
                MaxSize = 0;
                goto Fail;
            }
        }

        /* Finally, resize the descriptor */
        FoundUmaDesc->Size = RequestSize;

Success:
        /* Returned size is in paragraphs */
        *Size = (FoundUmaDesc->Size >> 4);
        return TRUE;
    }
    else
    {
Fail:
        /* Returned size is in paragraphs */
        *Size = (MaxSize >> 4);
        return FALSE;
    }
}

BOOLEAN UmaMgrInitialize(VOID)
{
// See bios/rom.h
#define ROM_AREA_START  0xE0000
#define ROM_AREA_END    0xFFFFF

#define OPTION_ROM_SIGNATURE    0xAA55

    PUMA_DESCRIPTOR UmaDesc = NULL;
    ULONG StartAddress = 0;
    ULONG Size = 0;
    UMA_DESC_TYPE Type = UMA_FREE;

    UINT i;

    ULONG Start, End;
    ULONG Increment;

    ULONG Address;

    // ULONG RomStart[]   = {};
    // ULONG RomEnd[]   = {};
    ULONG RomBoundaries[] = {0xA0000, 0xC0000, /*0xC8000, 0xE0000,*/ 0xF0000, 0x100000};
    ULONG SizeIncrement[] = {0x20000, 0x00800, /*0x00800, 0x10000,*/ 0x10000, 0x0000  };

    // InitializeListHead(&UmaDescriptorList);

    /* NOTE: There must be always one UMA descriptor at least */
    // FIXME: Maybe it should be a static object?

    for (i = 0; i < ARRAYSIZE(RomBoundaries) - 1; i++)
    {
        Start = RomBoundaries[i];   // RomStart[i]
        End   = RomBoundaries[i+1]; // RomEnd[i]
        Increment = SizeIncrement[i];

        for (Address = Start; Address < End; Address += Increment)
        {
            if (StartAddress == 0)
            {
                /* Initialize data for a new descriptor */
                StartAddress = Address;
                Size = 0;
                Type = UMA_FREE;
            }

            /* Is it a normal system zone/user-excluded zone? */
            if (Address >= 0xA0000 && Address < 0xC0000)
            {
                // StartAddress = Address;
                Size = Increment;
                Type = UMA_SYSTEM;

                /* Create descriptor */
                UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                if (!UmaDesc) return FALSE;

                StartAddress = 0;
                continue;
            }
            /* Is it the PC ROM BIOS? */
            else if (Address >= 0xF0000)
            {
                // StartAddress = Address;
                Size = 0x10000; // Increment;
                Type = UMA_ROM;

                /* Create descriptor */
                UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                if (!UmaDesc) return FALSE;

                StartAddress = 0;
                continue;
            }
            /* Is it an option ROM? */
            else if (Address >= 0xC0000 && Address < 0xF0000)
            {
                ULONG RomSize;
                ULONG PrevRomAddress = 0;
                ULONG PrevRomSize = 0;

                // while (Address < 0xF0000)
                for (; Address < 0xF0000; Address += Increment)
                {

#if 0 // FIXME: Is this block, better here...
                    {
                        // We may be looping inside a ROM block, if: Type == 2 and:
                        // (StartAddress <= Address &&) StartAddress + Size > Address.
                        // In this case, do nothing (do not increase size either)
                        // But if we are now outside of a ROM block, then we need
                        // to create the previous block, and initialize a new empty one!
                        // (and following the next passages, increase its size).

                        // if (StartAddress < Address && Type != 2)
                        if (Type == UMA_ROM && StartAddress + Size > Address)
                        {
                            /* We are inside ROM, do nothing */
                        }
                        else if (Type == UMA_ROM && StartAddress + Size <= Address)
                        {
                            /* We finished a ROM descriptor */

                            /* Create descriptor */
                            UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                            if (!UmaDesc) return FALSE;

                            StartAddress = 0;
                            // goto Restart;
                        }
                        else if (Type != UMA_ROM)
                        {
                            Size += Increment;
                        }
                    }
#endif

Restart:
                    /// if (Address >= 0xE0000) { Increment = 0x10000; }

                    if (StartAddress == 0)
                    {
                        /* Initialize data for a new descriptor */
                        StartAddress = Address;
                        Size = 0;
                        Type = UMA_FREE;

                        PrevRomAddress = 0;
                        PrevRomSize = 0;
                    }

                    if (*(PUSHORT)REAL_TO_PHYS(Address) == OPTION_ROM_SIGNATURE)
                    {
                        /*
                         * If this is an adapter ROM (Start: C8000, End: E0000),
                         * its reported size is stored in byte 2 of the ROM.
                         *
                         * If this is an expansion ROM (Start: E0000, End: F0000),
                         * its real length is 64kB.
                         */
                        RomSize = *(PUCHAR)REAL_TO_PHYS(Address + 2) * 512;
                        // if (Address >= 0xE0000) RomSize = 0x10000;
                        if (Address >= 0xE0000) { RomSize = 0x10000; Increment = RomSize; }

                        DPRINT1("ROM present @ address 0x%p\n", Address);

                        if (StartAddress != 0 && Size != 0 &&
                            StartAddress + Size <= Address)
                        {
                            /* Finish old descriptor */
                            UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                            if (!UmaDesc) return FALSE;
                        }

                        /*
                         * We may have overlapping ROMs, when PrevRomAddress + PrevRomSize > RomAddress.
                         * They must be put inside the same UMA descriptor.
                         */
                        if (PrevRomAddress + PrevRomSize > /*Rom*/Address)
                        {
                            // Overlapping ROM

                            /*
                             * PrevRomAddress remains the same, but adjust
                             * PrevRomSize (ROM descriptors merging).
                             */
                            PrevRomSize = max(PrevRomSize, RomSize + Address - PrevRomAddress);

                            // FIX: Confirm that the start address is really
                            // the one of the previous descriptor.
                            StartAddress = PrevRomAddress;
                            Size = PrevRomSize;
                            Type = UMA_ROM;
                        }
                        else
                        {
                            // Non-overlapping ROM

                            PrevRomAddress = Address;
                            PrevRomSize = RomSize;

                            /* Initialize a new descriptor. We will create it when it's OK */
                            StartAddress = Address;
                            Size = RomSize;
                            Type = UMA_ROM;
                            // continue;
                        }
                    }
#if 1 // FIXME: ...or there??
                    else
                    {
                        // We may be looping inside a ROM block, if: Type == 2 and:
                        // (StartAddress <= Address &&) StartAddress + Size > Address.
                        // In this case, do nothing (do not increase size either)
                        // But if we are now outside of a ROM block, then we need
                        // to create the previous block, and initialize a new empty one!
                        // (and following the next passages, increase its size).

                        // if (StartAddress < Address && Type != UMA_ROM)
                        if (Type == UMA_ROM && StartAddress + Size > Address)
                        {
                            // We are inside ROM, do nothing
                        }
                        else if (Type == UMA_ROM && StartAddress + Size <= Address)
                        {
                            // We finished a ROM descriptor.

                            /* Create descriptor */
                            UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                            if (!UmaDesc) return FALSE;

                            StartAddress = 0;
                            goto Restart;
                        }
                        else if (Type != UMA_ROM)
                        {
                            Size += Increment;
                        }
                    }
#endif

                    // Fixed incroment; we may encounter again overlapping ROMs, etc.
                    // Address += Increment;
                }

                if (StartAddress != 0 && Size != 0)
                {
                    /* Create descriptor */
                    UmaDesc = CreateUmaDescriptor(NULL, StartAddress, Size, Type);
                    if (!UmaDesc) return FALSE;

                    StartAddress = 0;
                }

            }
        }
    }

    return TRUE;
}

VOID UmaMgrCleanup(VOID)
{
    PUMA_DESCRIPTOR UmaDesc;

    while (!IsListEmpty(&UmaDescriptorList))
    {
        UmaDesc = (PUMA_DESCRIPTOR)CONTAINING_RECORD(UmaDescriptorList.Flink, UMA_DESCRIPTOR, Entry);
        FreeUmaDescriptor(UmaDesc);
    }
}

/* EOF */
