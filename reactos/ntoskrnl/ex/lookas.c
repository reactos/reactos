/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/lookas.c
 * PURPOSE:         Lookaside lists
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;
LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;

/* FUNCTIONS *****************************************************************/

VOID 
INIT_FUNCTION
ExpInitLookasideLists()
{
    /* Initialize Lock and Listhead */
    InitializeListHead(&ExpNonPagedLookasideListHead);
    KeInitializeSpinLock(&ExpNonPagedLookasideListLock);
    InitializeListHead(&ExpPagedLookasideListHead);
    KeInitializeSpinLock(&ExpPagedLookasideListLock);
}

/*
 * @implemented
 */
PVOID
STDCALL
ExiAllocateFromPagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside)
{
    PVOID Entry;

    Lookaside->L.TotalAllocates++;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (!Entry) 
    {
        Lookaside->L.AllocateMisses++;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size, 
                                        Lookaside->L.Tag);
    }
    return Entry;
}

/*
 * @implemented
 */
VOID
STDCALL
ExiFreeToPagedLookasideList(IN PPAGED_LOOKASIDE_LIST  Lookaside,
                            IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) 
    {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    }
    else
    {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}

/*
 * @implemented
 */
VOID
STDCALL
ExDeleteNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release the resources allocated
       for them */
    for (;;)
    {
        Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
        if (!Entry) break;
        (*Lookaside->L.Free)(Entry);
    }
    
    /* Remove from list */
    KeAcquireSpinLock(&ExpNonPagedLookasideListLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    KeReleaseSpinLock(&ExpNonPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
ExDeletePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release the resources allocated
       for them */
    for (;;)
    {
        Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
        if (!Entry) break;
        (*Lookaside->L.Free)(Entry);
    }
    
    /* Remove from list */
    KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
                                PALLOCATE_FUNCTION Allocate,
                                PFREE_FUNCTION Free,
                                ULONG Flags,
                                ULONG Size,
                                ULONG Tag,
                                USHORT Depth)
{
    DPRINT("Initializing nonpaged lookaside list at 0x%X\n", Lookaside);

    /* Initialize the Header */
    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = NonPagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = Size;
    Lookaside->L.Depth = 4;
    Lookaside->L.MaximumDepth = 256;
    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;

    /* Set the Allocate/Free Routines */
    if (Allocate)
    {
        Lookaside->L.Allocate = Allocate;
    }
    else
    {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;
    }

    if (Free)
    {
        Lookaside->L.Free = Free;
    }
    else
    {
        Lookaside->L.Free = ExFreePool;
    }
    
    /* Insert it into the list */
    ExInterlockedInsertTailList(&ExpNonPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExpNonPagedLookasideListLock);
}


/*
 * @implemented
 */
VOID
STDCALL
ExInitializePagedLookasideList (PPAGED_LOOKASIDE_LIST Lookaside,
                                PALLOCATE_FUNCTION Allocate,
                                PFREE_FUNCTION Free,
                                ULONG Flags,
                                ULONG Size,
                                ULONG Tag,
                                USHORT Depth)
{
    DPRINT("Initializing paged lookaside list at 0x%X\n", Lookaside);

    /* Initialize the Header */
    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = PagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = Size;
    Lookaside->L.Depth = 4;
    Lookaside->L.MaximumDepth = 256;
    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;

    /* Set the Allocate/Free Routines */
    if (Allocate)
    {
        Lookaside->L.Allocate = Allocate;
    }
    else
    {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;
    }

    if (Free)
    {
        Lookaside->L.Free = Free;
    }
    else
    {
        Lookaside->L.Free = ExFreePool;
    }
    
    /* Insert it into the list */
    ExInterlockedInsertTailList(&ExpNonPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExpNonPagedLookasideListLock);
}

/* EOF */
