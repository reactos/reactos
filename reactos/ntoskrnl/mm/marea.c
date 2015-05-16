/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/marea.c
 * PURPOSE:         Implements memory areas
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Philip Susi
 *                  Casper Hornstrup
 *                  Eric Kohl
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Filip Navara
 *                  Herve Poussineau
 *                  Steven Edwards
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <cache/section/newmm.h>
#include <debug.h>

#include "ARM3/miarm.h"

MEMORY_AREA MiStaticMemoryAreas[MI_STATIC_MEMORY_AREAS];
ULONG MiStaticMemoryAreaCount;

/* FUNCTIONS *****************************************************************/

/**
 * @name MmIterateFirstNode
 *
 * @param Node
 *        Head node of the MEMORY_AREA tree.
 *
 * @return The leftmost MEMORY_AREA node (ie. the one with lowest
 *         address)
 */

static PMEMORY_AREA MmIterateFirstNode(PMEMORY_AREA Node)
{
    while (Node->LeftChild != NULL)
        Node = Node->LeftChild;

    return Node;
}

/**
 * @name MmIterateNextNode
 *
 * @param Node
 *        Current node in the tree.
 *
 * @return Next node in the tree (sorted by address).
 */

static PMEMORY_AREA MmIterateNextNode(PMEMORY_AREA Node)
{
    if (Node->RightChild != NULL)
    {
        Node = Node->RightChild;
        while (Node->LeftChild != NULL)
            Node = Node->LeftChild;
    }
    else
    {
        PMEMORY_AREA TempNode = NULL;

        do
        {
            /* Check if we're at the end of tree. */
            if (Node->Parent == NULL)
                return NULL;

            TempNode = Node;
            Node = Node->Parent;
        }
        while (TempNode == Node->RightChild);
    }
    return Node;
}

/**
 * @name MmIterateLastNode
 *
 * @param Node
 *        Head node of the MEMORY_AREA tree.
 *
 * @return The rightmost MEMORY_AREA node (ie. the one with highest
 *         address)
 */

static PMEMORY_AREA MmIterateLastNode(PMEMORY_AREA Node)
{
    while (Node->RightChild != NULL)
        Node = Node->RightChild;

    return Node;
}

/**
 * @name MmIteratePreviousNode
 *
 * @param Node
 *        Current node in the tree.
 *
 * @return Previous node in the tree (sorted by address).
 */

static PMEMORY_AREA MmIteratePrevNode(PMEMORY_AREA Node)
{
    if (Node->LeftChild != NULL)
    {
        Node = Node->LeftChild;
        while (Node->RightChild != NULL)
            Node = Node->RightChild;
    }
    else
    {
        PMEMORY_AREA TempNode = NULL;

        do
        {
            /* Check if we're at the end of tree. */
            if (Node->Parent == NULL)
                return NULL;

            TempNode = Node;
            Node = Node->Parent;
        }
        while (TempNode == Node->LeftChild);
    }
    return Node;
}

PMEMORY_AREA NTAPI
MmLocateMemoryAreaByAddress(
    PMMSUPPORT AddressSpace,
    PVOID Address_)
{
    PMEMORY_AREA Node = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;
    ULONG_PTR Address = (ULONG_PTR)Address_;

    DPRINT("MmLocateMemoryAreaByAddress(AddressSpace %p, Address %p)\n",
           AddressSpace, Address);

    while (Node != NULL)
    {
        if (Address < Node->StartingAddress)
            Node = Node->LeftChild;
        else if (Address >= Node->EndingAddress)
            Node = Node->RightChild;
        else
        {
            DPRINT("MmLocateMemoryAreaByAddress(%p): %p [%p - %p]\n",
                   Address, Node, Node->StartingAddress, Node->EndingAddress);
            return Node;
        }
    }

    DPRINT("MmLocateMemoryAreaByAddress(%p): 0\n", Address);
    return NULL;
}

PMEMORY_AREA NTAPI
MmLocateMemoryAreaByRegion(
    PMMSUPPORT AddressSpace,
    PVOID Address_,
    ULONG_PTR Length)
{
    PMEMORY_AREA Node;
    ULONG_PTR Address = (ULONG_PTR)Address_;
    ULONG_PTR Extent = Address + Length;

    /* Special case for empty tree. */
    if (AddressSpace->WorkingSetExpansionLinks.Flink == NULL)
        return NULL;

    /* Traverse the tree from left to right. */
    for (Node = MmIterateFirstNode((PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink);
            Node != NULL;
            Node = MmIterateNextNode(Node))
    {
        if (Node->StartingAddress >= Address &&
                Node->StartingAddress < Extent)
        {
            DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                   Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                   Node->EndingAddress);
            return Node;
        }
        if (Node->EndingAddress > Address &&
                Node->EndingAddress < Extent)
        {
            DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                   Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                   Node->EndingAddress);
            return Node;
        }
        if (Node->StartingAddress <= Address &&
                Node->EndingAddress >= Extent)
        {
            DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                   Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                   Node->EndingAddress);
            return Node;
        }
        if (Node->StartingAddress >= Extent)
        {
            DPRINT("Finished MmLocateMemoryAreaByRegion() = NULL\n");
            return NULL;
        }
    }

    return NULL;
}

/**
 * @name MmCompressHelper
 *
 * This is helper of MmRebalanceTree. Performs a compression transformation
 * count times, starting at root.
 */

static VOID
MmCompressHelper(
    PMMSUPPORT AddressSpace,
    ULONG Count)
{
    PMEMORY_AREA Root = NULL;
    PMEMORY_AREA Red = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;
    PMEMORY_AREA Black = Red->LeftChild;

    while (Count--)
    {
        if (Root)
            Root->LeftChild = Black;
        else
            AddressSpace->WorkingSetExpansionLinks.Flink = (PVOID)Black;
        Black->Parent = Root;
        Red->LeftChild = Black->RightChild;
        if (Black->RightChild)
            Black->RightChild->Parent = Red;
        Black->RightChild = Red;
        Red->Parent = Black;
        Root = Black;

        if (Count)
        {
            Red = Root->LeftChild;
            Black = Red->LeftChild;
        }
    }
}

/**
 * @name MmRebalanceTree
 *
 * Rebalance a memory area tree using the Tree->Vine->Balanced Tree
 * method described in libavl documentation in chapter 4.12.
 * (http://www.stanford.edu/~blp/avl/libavl.html/)
 */

static VOID
MmRebalanceTree(
    PMMSUPPORT AddressSpace)
{
    PMEMORY_AREA PreviousNode;
    PMEMORY_AREA CurrentNode;
    PMEMORY_AREA TempNode;
    ULONG NodeCount = 0;
    ULONG Vine;   /* Number of nodes in main vine. */
    ULONG Leaves; /* Nodes in incomplete bottom level, if any. */
    INT Height;   /* Height of produced balanced tree. */

    /* Transform the tree into Vine. */

    PreviousNode = NULL;
    CurrentNode = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;
    while (CurrentNode != NULL)
    {
        if (CurrentNode->RightChild == NULL)
        {
            PreviousNode = CurrentNode;
            CurrentNode = CurrentNode->LeftChild;
            NodeCount++;
        }
        else
        {
            TempNode = CurrentNode->RightChild;

            CurrentNode->RightChild = TempNode->LeftChild;
            if (TempNode->LeftChild)
                TempNode->LeftChild->Parent = CurrentNode;

            TempNode->LeftChild = CurrentNode;
            CurrentNode->Parent = TempNode;

            CurrentNode = TempNode;

            if (PreviousNode != NULL)
                PreviousNode->LeftChild = TempNode;
            else
                AddressSpace->WorkingSetExpansionLinks.Flink = (PVOID)TempNode;
            TempNode->Parent = PreviousNode;
        }
    }

    /* Transform Vine back into a balanced tree. */

    Leaves = NodeCount + 1;
    for (;;)
    {
        ULONG Next = Leaves & (Leaves - 1);
        if (Next == 0)
            break;
        Leaves = Next;
    }
    Leaves = NodeCount + 1 - Leaves;

    MmCompressHelper(AddressSpace, Leaves);

    Vine = NodeCount - Leaves;
    Height = 1 + (Leaves > 0);
    while (Vine > 1)
    {
        MmCompressHelper(AddressSpace, Vine / 2);
        Vine /= 2;
        Height++;
    }
}

VOID
NTAPI
MiInsertVad(IN PMMVAD Vad,
            IN PEPROCESS Process);

ULONG
NTAPI
MiMakeProtectionMask(
    IN ULONG Protect
);

static VOID
MmInsertMemoryArea(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA marea)
{
    PMEMORY_AREA Node;
    PMEMORY_AREA PreviousNode;
    ULONG Depth = 0;
    PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

    /* Build a lame VAD if this is a user-space allocation */
    if ((marea->EndingAddress < (ULONG_PTR)MmSystemRangeStart) &&
        (marea->Type != MEMORY_AREA_OWNED_BY_ARM3))
    {
        PMMVAD Vad;

        ASSERT(marea->Type == MEMORY_AREA_SECTION_VIEW || marea->Type == MEMORY_AREA_CACHE);
        Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD), TAG_MVAD);
        ASSERT(Vad);
        RtlZeroMemory(Vad, sizeof(MMVAD));
        Vad->StartingVpn = PAGE_ROUND_DOWN(marea->StartingAddress) >> PAGE_SHIFT;
        /*
         * For some strange reason, it is perfectly valid to create a MAREA from 0x1000 to... 0x1000.
         * In a normal OS/Memory Manager, this would be retarded, but ReactOS allows this (how it works
         * I don't even want to know).
         */
        if (marea->EndingAddress != marea->StartingAddress)
        {
            Vad->EndingVpn = PAGE_ROUND_DOWN((ULONG_PTR)marea->EndingAddress - 1) >> PAGE_SHIFT;
        }
        else
        {
            Vad->EndingVpn = Vad->StartingVpn;
        }
        Vad->u.VadFlags.Spare = 1;
        Vad->u.VadFlags.Protection = MiMakeProtectionMask(marea->Protect);

        /* Insert the VAD */
        MiInsertVad(Vad, Process);
        marea->Vad = Vad;
    }
    else
    {
        marea->Vad = NULL;
    }

    if (AddressSpace->WorkingSetExpansionLinks.Flink == NULL)
    {
        AddressSpace->WorkingSetExpansionLinks.Flink = (PVOID)marea;
        marea->LeftChild = marea->RightChild = marea->Parent = NULL;
        return;
    }

    Node = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;
    do
    {
        DPRINT("marea->EndingAddress: %p Node->StartingAddress: %p\n",
               marea->EndingAddress, Node->StartingAddress);
        DPRINT("marea->StartingAddress: %p Node->EndingAddress: %p\n",
               marea->StartingAddress, Node->EndingAddress);
        ASSERT(marea->EndingAddress <= Node->StartingAddress ||
               marea->StartingAddress >= Node->EndingAddress);
        ASSERT(marea->StartingAddress != Node->StartingAddress);

        PreviousNode = Node;

        if (marea->StartingAddress < Node->StartingAddress)
            Node = Node->LeftChild;
        else
            Node = Node->RightChild;

        if (Node)
        {
            Depth++;
            if (Depth == 22)
            {
                MmRebalanceTree(AddressSpace);
                PreviousNode = Node->Parent;
            }
        }
    }
    while (Node != NULL);

    marea->LeftChild = marea->RightChild = NULL;
    marea->Parent = PreviousNode;
    if (marea->StartingAddress < PreviousNode->StartingAddress)
        PreviousNode->LeftChild = marea;
    else
        PreviousNode->RightChild = marea;
}

static PVOID
MmFindGapBottomUp(
    PMMSUPPORT AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity)
{
    ULONG_PTR LowestAddress, HighestAddress, Candidate;
    PMEMORY_AREA Root, Node;

    /* Get the margins of the address space */
    if (MmGetAddressSpaceOwner(AddressSpace) != NULL)
    {
        LowestAddress = (ULONG_PTR)MM_LOWEST_USER_ADDRESS;
        HighestAddress = (ULONG_PTR)MmHighestUserAddress;
    }
    else
    {
        LowestAddress = (ULONG_PTR)MmSystemRangeStart;
        HighestAddress = MAXULONG_PTR;
    }

    /* Start with the lowest address */
    Candidate = LowestAddress;

    /* Check for overflow */
    if ((Candidate + Length) < Candidate) return NULL;

    /* Get the root of the address space tree */
    Root = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;

    /* Go to the node with lowest address in the tree. */
    Node = Root ? MmIterateFirstNode(Root) : NULL;
    while (Node && ((ULONG_PTR)Node->EndingAddress < LowestAddress))
    {
        Node = MmIterateNextNode(Node);
    }

    /* Traverse the tree from low to high addresses */
    while (Node && ((ULONG_PTR)Node->EndingAddress < HighestAddress))
    {
        /* Check if the memory area fits before the current node */
        if ((ULONG_PTR)Node->StartingAddress >= (Candidate + Length))
        {
            DPRINT("MmFindGapBottomUp: %p\n", Candidate);
            ASSERT(Candidate >= LowestAddress);
            return (PVOID)Candidate;
        }

        /* Calculate next possible adress above this node */
        Candidate = ALIGN_UP_BY((ULONG_PTR)Node->EndingAddress, Granularity);

        /* Check for overflow */
        if ((Candidate + Length) < (ULONG_PTR)Node->EndingAddress) return NULL;

        /* Go to the next higher node */
        Node = MmIterateNextNode(Node);
    }

    /* Check if there is enough space after the last memory area. */
    if ((Candidate + Length) <= HighestAddress)
    {
        DPRINT("MmFindGapBottomUp: %p\n", Candidate);
        ASSERT(Candidate >= LowestAddress);
        return (PVOID)Candidate;
    }

    DPRINT("MmFindGapBottomUp: 0\n");
    return NULL;
}


static PVOID
MmFindGapTopDown(
    PMMSUPPORT AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity)
{
    ULONG_PTR LowestAddress, HighestAddress, Candidate;
    PMEMORY_AREA Root, Node;

    /* Get the margins of the address space */
    if (MmGetAddressSpaceOwner(AddressSpace) != NULL)
    {
        LowestAddress = (ULONG_PTR)MM_LOWEST_USER_ADDRESS;
        HighestAddress = (ULONG_PTR)MmHighestUserAddress;
    }
    else
    {
        LowestAddress = (ULONG_PTR)MmSystemRangeStart;
        HighestAddress = MAXULONG_PTR;
    }

    /* Calculate the highest candidate */
    Candidate = ALIGN_DOWN_BY(HighestAddress + 1 - Length, Granularity);

    /* Check for overflow. */
    if (Candidate > HighestAddress) return NULL;

    /* Get the root of the address space tree */
    Root = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;

    /* Go to the node with highest address in the tree. */
    Node = Root ? MmIterateLastNode(Root) : NULL;
    while (Node && ((ULONG_PTR)Node->StartingAddress > HighestAddress))
    {
        Node = MmIteratePrevNode(Node);
    }

    /* Traverse the tree from high to low addresses */
    while (Node && ((ULONG_PTR)Node->StartingAddress > LowestAddress))
    {
        /* Check if the memory area fits after the current node */
        if ((ULONG_PTR)Node->EndingAddress <= Candidate)
        {
            DPRINT("MmFindGapTopDown: %p\n", Candidate);
            return (PVOID)Candidate;
        }

        /* Calculate next possible adress below this node */
        Candidate = ALIGN_DOWN_BY((ULONG_PTR)Node->StartingAddress - Length,
                                  Granularity);

        /* Check for overflow. */
        if (Candidate > (ULONG_PTR)Node->StartingAddress)
            return NULL;

        /* Go to the next lower node */
        Node = MmIteratePrevNode(Node);
    }

    /* Check if the last candidate is inside the given range */
    if (Candidate >= LowestAddress)
    {
        DPRINT("MmFindGapTopDown: %p\n", Candidate);
        return (PVOID)Candidate;
    }

    DPRINT("MmFindGapTopDown: 0\n");
    return NULL;
}


PVOID NTAPI
MmFindGap(
    PMMSUPPORT AddressSpace,
    ULONG_PTR Length,
    ULONG_PTR Granularity,
    BOOLEAN TopDown)
{
    if (TopDown)
        return MmFindGapTopDown(AddressSpace, Length, Granularity);

    return MmFindGapBottomUp(AddressSpace, Length, Granularity);
}

VOID
NTAPI
MiRemoveNode(IN PMMADDRESS_NODE Node,
             IN PMM_AVL_TABLE Table);

#if DBG

static
VOID
MiRosCheckMemoryAreasRecursive(
    PMEMORY_AREA Node)
{
    /* Check if the allocation is ok */
    ExpCheckPoolAllocation(Node, NonPagedPool, 'ERAM');

    /* Check some fields */
    ASSERT(Node->Magic == 'erAM');
    ASSERT(PAGE_ALIGN(Node->StartingAddress) == (PVOID)Node->StartingAddress);
    ASSERT(Node->EndingAddress != 0);
    ASSERT(PAGE_ALIGN(Node->EndingAddress) == (PVOID)Node->EndingAddress);
    ASSERT(Node->StartingAddress < Node->EndingAddress);
    ASSERT((Node->Type == 0) ||
           (Node->Type == MEMORY_AREA_CACHE) ||
           // (Node->Type == MEMORY_AREA_CACHE_SEGMENT) ||
           (Node->Type == MEMORY_AREA_SECTION_VIEW) ||
           (Node->Type == MEMORY_AREA_OWNED_BY_ARM3) ||
           (Node->Type == (MEMORY_AREA_OWNED_BY_ARM3 | MEMORY_AREA_STATIC)));

    /* Recursively check children */
    if (Node->LeftChild != NULL)
        MiRosCheckMemoryAreasRecursive(Node->LeftChild);
    if (Node->RightChild != NULL)
        MiRosCheckMemoryAreasRecursive(Node->RightChild);
}

VOID
NTAPI
MiRosCheckMemoryAreas(
    PMMSUPPORT AddressSpace)
{
    PMEMORY_AREA RootNode;
    PEPROCESS AddressSpaceOwner;
    BOOLEAN NeedReleaseLock;

    NeedReleaseLock = FALSE;

    /* Get the address space owner */
    AddressSpaceOwner = CONTAINING_RECORD(AddressSpace, EPROCESS, Vm);

    /* Check if we already own the address space lock */
    if (AddressSpaceOwner->AddressCreationLock.Owner != KeGetCurrentThread())
    {
        /* We must own it! */
        MmLockAddressSpace(AddressSpace);
        NeedReleaseLock = TRUE;
    }

    /* Check all memory areas */
    RootNode = (PMEMORY_AREA)AddressSpace->WorkingSetExpansionLinks.Flink;
    MiRosCheckMemoryAreasRecursive(RootNode);

    /* Release the lock, if we acquired it */
    if (NeedReleaseLock)
    {
        MmUnlockAddressSpace(AddressSpace);
    }
}

extern KGUARDED_MUTEX PspActiveProcessMutex;

VOID
NTAPI
MiCheckAllProcessMemoryAreas(VOID)
{
    PEPROCESS Process;
    PLIST_ENTRY Entry;

    /* Acquire the Active Process Lock */
    KeAcquireGuardedMutex(&PspActiveProcessMutex);

    /* Loop the process list */
    Entry = PsActiveProcessHead.Flink;
    while (Entry != &PsActiveProcessHead)
    {
        /* Get the process */
        Process = CONTAINING_RECORD(Entry, EPROCESS, ActiveProcessLinks);

        /* Check memory areas */
        MiRosCheckMemoryAreas(&Process->Vm);

        Entry = Entry->Flink;
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&PspActiveProcessMutex);
}

#endif

/**
 * @name MmFreeMemoryArea
 *
 * Free an existing memory area.
 *
 * @param AddressSpace
 *        Address space to free the area from.
 * @param MemoryArea
 *        Memory area we're about to free.
 * @param FreePage
 *        Callback function for each freed page.
 * @param FreePageContext
 *        Context passed to the callback function.
 *
 * @return Status
 *
 * @remarks Lock the address space before calling this function.
 */
VOID
NTAPI
MiDeletePte(IN PMMPTE PointerPte,
            IN PVOID VirtualAddress,
            IN PEPROCESS CurrentProcess,
            IN PMMPTE PrototypePte);

NTSTATUS NTAPI
MmFreeMemoryArea(
    PMMSUPPORT AddressSpace,
    PMEMORY_AREA MemoryArea,
    PMM_FREE_PAGE_FUNC FreePage,
    PVOID FreePageContext)
{
    PMEMORY_AREA *ParentReplace;
    ULONG_PTR Address;
    PVOID EndAddress;

    /* Make sure we own the address space lock! */
    ASSERT(CONTAINING_RECORD(AddressSpace, EPROCESS, Vm)->AddressCreationLock.Owner == KeGetCurrentThread());

    /* Check magic */
    ASSERT(MemoryArea->Magic == 'erAM');

    if (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3)
    {
        PEPROCESS CurrentProcess = PsGetCurrentProcess();
        PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

        if (Process != NULL &&
                Process != CurrentProcess)
        {
            KeAttachProcess(&Process->Pcb);
        }

        EndAddress = MM_ROUND_UP(MemoryArea->EndingAddress, PAGE_SIZE);
        for (Address = (ULONG_PTR)MemoryArea->StartingAddress;
                Address < (ULONG_PTR)EndAddress;
                Address += PAGE_SIZE)
        {
            BOOLEAN Dirty = FALSE;
            SWAPENTRY SwapEntry = 0;
            PFN_NUMBER Page = 0;

            if (MmIsPageSwapEntry(Process, (PVOID)Address))
            {
                MmDeletePageFileMapping(Process, (PVOID)Address, &SwapEntry);
            }
            else
            {
                MmDeleteVirtualMapping(Process, (PVOID)Address, &Dirty, &Page);
            }
            if (FreePage != NULL)
            {
                FreePage(FreePageContext, MemoryArea, (PVOID)Address,
                         Page, SwapEntry, (BOOLEAN)Dirty);
            }
#if (_MI_PAGING_LEVELS == 2)
            /* Remove page table reference */
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            if ((SwapEntry || Page) && ((PVOID)Address < MmSystemRangeStart))
            {
                ASSERT(AddressSpace != MmGetKernelAddressSpace());
                if (MiQueryPageTableReferences((PVOID)Address) == 0)
                {
                    /* No PTE relies on this PDE. Release it */
                    KIRQL OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
                    PMMPDE PointerPde = MiAddressToPde(Address);
                    ASSERT(PointerPde->u.Hard.Valid == 1);
                    MiDeletePte(PointerPde, MiPdeToPte(PointerPde), Process, NULL);
                    ASSERT(PointerPde->u.Hard.Valid == 0);
                    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
                }
            }
#endif
        }

        if (Process != NULL &&
                Process != CurrentProcess)
        {
            KeDetachProcess();
        }

        if (MemoryArea->Vad)
        {
            ASSERT(MemoryArea->EndingAddress < (ULONG_PTR)MmSystemRangeStart);
            ASSERT(MemoryArea->Type == MEMORY_AREA_SECTION_VIEW || MemoryArea->Type == MEMORY_AREA_CACHE);

            /* MmCleanProcessAddressSpace might have removed it (and this would be MmDeleteProcessAdressSpace) */
            ASSERT(((PMMVAD)MemoryArea->Vad)->u.VadFlags.Spare != 0);
            if (((PMMVAD)MemoryArea->Vad)->u.VadFlags.Spare == 1)
            {
                MiRemoveNode(MemoryArea->Vad, &Process->VadRoot);
            }

            ExFreePoolWithTag(MemoryArea->Vad, TAG_MVAD);
            MemoryArea->Vad = NULL;
        }
    }

    /* Remove the tree item. */
    {
        if (MemoryArea->Parent != NULL)
        {
            if (MemoryArea->Parent->LeftChild == MemoryArea)
                ParentReplace = &MemoryArea->Parent->LeftChild;
            else
                ParentReplace = &MemoryArea->Parent->RightChild;
        }
        else
            ParentReplace = (PMEMORY_AREA*)&AddressSpace->WorkingSetExpansionLinks.Flink;

        if (MemoryArea->RightChild == NULL)
        {
            *ParentReplace = MemoryArea->LeftChild;
            if (MemoryArea->LeftChild)
                MemoryArea->LeftChild->Parent = MemoryArea->Parent;
        }
        else
        {
            if (MemoryArea->RightChild->LeftChild == NULL)
            {
                MemoryArea->RightChild->LeftChild = MemoryArea->LeftChild;
                if (MemoryArea->LeftChild)
                    MemoryArea->LeftChild->Parent = MemoryArea->RightChild;

                *ParentReplace = MemoryArea->RightChild;
                MemoryArea->RightChild->Parent = MemoryArea->Parent;
            }
            else
            {
                PMEMORY_AREA LowestNode;

                LowestNode = MemoryArea->RightChild->LeftChild;
                while (LowestNode->LeftChild != NULL)
                    LowestNode = LowestNode->LeftChild;

                LowestNode->Parent->LeftChild = LowestNode->RightChild;
                if (LowestNode->RightChild)
                    LowestNode->RightChild->Parent = LowestNode->Parent;

                LowestNode->LeftChild = MemoryArea->LeftChild;
                if (MemoryArea->LeftChild)
                    MemoryArea->LeftChild->Parent = LowestNode;

                LowestNode->RightChild = MemoryArea->RightChild;
                MemoryArea->RightChild->Parent = LowestNode;

                *ParentReplace = LowestNode;
                LowestNode->Parent = MemoryArea->Parent;
            }
        }
    }

    ExFreePoolWithTag(MemoryArea, TAG_MAREA);

    DPRINT("MmFreeMemoryAreaByNode() succeeded\n");

    return STATUS_SUCCESS;
}

/**
 * @name MmCreateMemoryArea
 *
 * Create a memory area.
 *
 * @param AddressSpace
 *        Address space to create the area in.
 * @param Type
 *        Type of the memory area.
 * @param BaseAddress
 *        Base address for the memory area we're about the create. On
 *        input it contains either 0 (auto-assign address) or preferred
 *        address. On output it contains the starting address of the
 *        newly created area.
 * @param Length
 *        Length of the area to allocate.
 * @param Attributes
 *        Protection attributes for the memory area.
 * @param Result
 *        Receives a pointer to the memory area on successful exit.
 *
 * @return Status
 *
 * @remarks Lock the address space before calling this function.
 */

NTSTATUS NTAPI
MmCreateMemoryArea(PMMSUPPORT AddressSpace,
                   ULONG Type,
                   PVOID *BaseAddress,
                   ULONG_PTR Length,
                   ULONG Protect,
                   PMEMORY_AREA *Result,
                   BOOLEAN FixedAddress,
                   ULONG AllocationFlags,
                   ULONG Granularity)
{
    ULONG_PTR tmpLength;
    PMEMORY_AREA MemoryArea;
    ULONG_PTR EndingAddress;

    DPRINT("MmCreateMemoryArea(Type 0x%lx, BaseAddress %p, "
           "*BaseAddress %p, Length %p, AllocationFlags %x, "
           "FixedAddress %x, Result %p)\n",
           Type, BaseAddress, *BaseAddress, Length, AllocationFlags,
           FixedAddress, Result);

    if ((*BaseAddress) == 0 && !FixedAddress)
    {
        tmpLength = (ULONG_PTR)MM_ROUND_UP(Length, PAGE_SIZE);
        *BaseAddress = MmFindGap(AddressSpace,
                                 tmpLength,
                                 Granularity,
                                 (AllocationFlags & MEM_TOP_DOWN) == MEM_TOP_DOWN);
        if ((*BaseAddress) == 0)
        {
            DPRINT("No suitable gap\n");
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        EndingAddress = ((ULONG_PTR)*BaseAddress + Length - 1) | (PAGE_SIZE - 1);
        *BaseAddress = ALIGN_DOWN_POINTER_BY(*BaseAddress, Granularity);
        tmpLength = EndingAddress + 1 - (ULONG_PTR)*BaseAddress;

        if (!MmGetAddressSpaceOwner(AddressSpace) && *BaseAddress < MmSystemRangeStart)
        {
            return STATUS_ACCESS_VIOLATION;
        }

        if (MmGetAddressSpaceOwner(AddressSpace) &&
                (ULONG_PTR)(*BaseAddress) + tmpLength > (ULONG_PTR)MmSystemRangeStart)
        {
            DPRINT("Memory area for user mode address space exceeds MmSystemRangeStart\n");
            return STATUS_ACCESS_VIOLATION;
        }

        if (MmLocateMemoryAreaByRegion(AddressSpace,
                                       *BaseAddress,
                                       tmpLength) != NULL)
        {
            DPRINT("Memory area already occupied\n");
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // Is this a static memory area?
    //
    if (Type & MEMORY_AREA_STATIC)
    {
        //
        // Use the static array instead of the pool
        //
        ASSERT(MiStaticMemoryAreaCount < MI_STATIC_MEMORY_AREAS);
        MemoryArea = &MiStaticMemoryAreas[MiStaticMemoryAreaCount++];
        Type &= ~MEMORY_AREA_STATIC;
    }
    else
    {
        //
        // Allocate the memory area from nonpaged pool
        //
        MemoryArea = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(MEMORY_AREA),
                                           TAG_MAREA);
    }

    if (!MemoryArea)
    {
        DPRINT1("Not enough memory.\n");
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(MemoryArea, sizeof(MEMORY_AREA));
    MemoryArea->Type = Type;
    MemoryArea->StartingAddress = (ULONG_PTR)*BaseAddress;
    MemoryArea->EndingAddress = ((ULONG_PTR)*BaseAddress + tmpLength);
    MemoryArea->Protect = Protect;
    MemoryArea->Flags = AllocationFlags;
    //MemoryArea->LockCount = 0;
    MemoryArea->Magic = 'erAM';
    MemoryArea->DeleteInProgress = FALSE;

    MmInsertMemoryArea(AddressSpace, MemoryArea);

    *Result = MemoryArea;

    DPRINT("MmCreateMemoryArea() succeeded (%p)\n", *BaseAddress);
    return STATUS_SUCCESS;
}


VOID
NTAPI
MmDeleteProcessAddressSpace2(IN PEPROCESS Process);

NTSTATUS
NTAPI
MmDeleteProcessAddressSpace(PEPROCESS Process)
{
    PVOID Address;
    PMEMORY_AREA MemoryArea;

    DPRINT("MmDeleteProcessAddressSpace(Process %p (%s))\n", Process,
           Process->ImageFileName);

#ifndef _M_AMD64
    RemoveEntryList(&Process->MmProcessLinks);
#endif
    MmLockAddressSpace(&Process->Vm);

    while ((MemoryArea = (PMEMORY_AREA)Process->Vm.WorkingSetExpansionLinks.Flink) != NULL)
    {
        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Address = (PVOID)MemoryArea->StartingAddress;
            MmUnlockAddressSpace(&Process->Vm);
            MmUnmapViewOfSection(Process, Address);
            MmLockAddressSpace(&Process->Vm);
            break;

        case MEMORY_AREA_CACHE:
            Address = (PVOID)MemoryArea->StartingAddress;
            MmUnlockAddressSpace(&Process->Vm);
            MmUnmapViewOfCacheSegment(&Process->Vm, Address);
            MmLockAddressSpace(&Process->Vm);
            break;

        case MEMORY_AREA_OWNED_BY_ARM3:
            MmFreeMemoryArea(&Process->Vm,
                             MemoryArea,
                             NULL,
                             NULL);
            break;

        default:
            KeBugCheck(MEMORY_MANAGEMENT);
        }
    }

#if (_MI_PAGING_LEVELS == 2)
    {
        KIRQL OldIrql;
        PMMPDE pointerPde;
        /* Attach to Process */
        KeAttachProcess(&Process->Pcb);

        /* Acquire PFN lock */
        OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

        for (Address = MI_LOWEST_VAD_ADDRESS;
                Address < MM_HIGHEST_VAD_ADDRESS;
                Address =(PVOID)((ULONG_PTR)Address + (PAGE_SIZE * PTE_COUNT)))
        {
            /* At this point all references should be dead */
            if (MiQueryPageTableReferences(Address) != 0)
            {
                DPRINT1("Process %p, Address %p, UsedPageTableEntries %lu\n",
                        Process,
                        Address,
                        MiQueryPageTableReferences(Address));
                ASSERT(MiQueryPageTableReferences(Address) == 0);
            }
            pointerPde = MiAddressToPde(Address);
            /* Unlike in ARM3, we don't necesarrily free the PDE page as soon as reference reaches 0,
             * so we must clean up a bit when process closes */
            if (pointerPde->u.Hard.Valid)
                MiDeletePte(pointerPde, MiPdeToPte(pointerPde), Process, NULL);
            ASSERT(pointerPde->u.Hard.Valid == 0);
        }
        /* Release lock */
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

        /* Detach */
        KeDetachProcess();
    }
#endif

    MmUnlockAddressSpace(&Process->Vm);

    DPRINT("Finished MmDeleteProcessAddressSpace()\n");
    MmDeleteProcessAddressSpace2(Process);
    return(STATUS_SUCCESS);
}

/* EOF */
