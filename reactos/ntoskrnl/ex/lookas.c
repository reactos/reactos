/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ex/lookas.c
* PURPOSE:         Lookaside Lists
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpInitLookasideLists)
#endif

/* GLOBALS *******************************************************************/

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;
LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;
LIST_ENTRY ExSystemLookasideListHead;
LIST_ENTRY ExPoolLookasideListHead;
NPAGED_LOOKASIDE_LIST ExpSmallNPagedPoolLookasideLists[MAXIMUM_PROCESSORS];
PAGED_LOOKASIDE_LIST ExpSmallPagedPoolLookasideLists[MAXIMUM_PROCESSORS];

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ExInitializeSystemLookasideList(IN PGENERAL_LOOKASIDE List,
                                IN POOL_TYPE Type,
                                IN ULONG Size,
                                IN ULONG Tag,
                                IN USHORT MaximumDepth,
                                IN PLIST_ENTRY ListHead)
{
    /* Initialize the list */
    List->Tag = Tag;
    List->Type = Type;
    List->Size = Size;
    InsertHeadList(ListHead, &List->ListEntry);
    List->MaximumDepth = MaximumDepth;
    List->Depth = 2;
    List->Allocate = ExAllocatePoolWithTag;
    List->Free = ExFreePool;
    List->ListHead.Next.Next = NULL;
    List->ListHead.Depth = 0;
    List->ListHead.Sequence = 0;
    List->TotalAllocates = 0;
    List->AllocateHits = 0;
    List->TotalFrees = 0;
    List->FreeHits = 0;
    List->LastTotalAllocates = 0;
    List->LastAllocateHits = 0;
}

VOID
NTAPI
ExInitPoolLookasidePointers(VOID)
{
    ULONG i;
    PPP_LOOKASIDE_LIST Entry;
    PNPAGED_LOOKASIDE_LIST ListEntry;
    PPAGED_LOOKASIDE_LIST PagedListEntry;

    /* Loop for all CPUs */
    for (i = 0; i < MAXIMUM_PROCESSORS; i++)
    {
        /* Initialize the non-paged list */
        ListEntry = &ExpSmallNPagedPoolLookasideLists[i];
        InitializeSListHead(&ListEntry->L.ListHead);

        /* Bind to PRCB */
        Entry = &KeGetCurrentPrcb()->PPPagedLookasideList[i];
        Entry->L = &ListEntry->L;
        Entry->P = &ListEntry->L;

        /* Initialize the paged list */
        PagedListEntry = &ExpSmallPagedPoolLookasideLists[i];
        InitializeSListHead(&PagedListEntry->L.ListHead);

        /* Bind to PRCB */
        Entry = &KeGetCurrentPrcb()->PPNPagedLookasideList[i];
        Entry->L = &PagedListEntry->L;
        Entry->P = &PagedListEntry->L;
    }
}

VOID
NTAPI
ExpInitLookasideLists()
{
    ULONG i, j;

    /* Initialize locks and lists */
    InitializeListHead(&ExpNonPagedLookasideListHead);
    InitializeListHead(&ExpPagedLookasideListHead);
    InitializeListHead(&ExSystemLookasideListHead);
    InitializeListHead(&ExPoolLookasideListHead);
    KeInitializeSpinLock(&ExpNonPagedLookasideListLock);
    KeInitializeSpinLock(&ExpPagedLookasideListLock);

    /* Initialize the system lookaside lists */
    for (i = 0, j = 1; i < (MAXIMUM_PROCESSORS - 1); j++, i++)
    {
        /* Initialize the non-paged list */
        ExInitializeSystemLookasideList(&ExpSmallNPagedPoolLookasideLists[i].L,
                                        NonPagedPool,
                                        j * 8,
                                        TAG('P', 'o', 'o', 'l'),
                                        256,
                                        &ExPoolLookasideListHead);

        /* Initialize the paged list */
        ExInitializeSystemLookasideList(&ExpSmallPagedPoolLookasideLists[i].L,
                                        PagedPool,
                                        j * 8,
                                        TAG('P', 'o', 'o', 'l'),
                                        256,
                                        &ExPoolLookasideListHead);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
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
NTAPI
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
NTAPI
ExDeleteNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release their resources */
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
NTAPI
ExDeletePagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release their resources */
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
NTAPI
ExInitializeNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST Lookaside,
                                IN PALLOCATE_FUNCTION Allocate OPTIONAL,
                                IN PFREE_FUNCTION Free OPTIONAL,
                                IN ULONG Flags,
                                IN ULONG Size,
                                IN ULONG Tag,
                                IN USHORT Depth)
{
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
NTAPI
ExInitializePagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside,
                               IN PALLOCATE_FUNCTION Allocate OPTIONAL,
                               IN PFREE_FUNCTION Free OPTIONAL,
                               IN ULONG Flags,
                               IN ULONG Size,
                               IN ULONG Tag,
                               IN USHORT Depth)
{
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
