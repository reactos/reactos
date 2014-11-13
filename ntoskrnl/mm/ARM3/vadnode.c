/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/vadnode.c
 * PURPOSE:         ARM Memory Manager VAD Node Algorithms
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* Include Mm version of AVL support */
#include "miavl.h"
#include <lib/rtl/avlsupp.c>

/* GLOBALS ********************************************************************/

CHAR MmReadWrite[32] =
{
    MM_NO_ACCESS_ALLOWED, MM_READ_ONLY_ALLOWED, MM_READ_ONLY_ALLOWED,
    MM_READ_ONLY_ALLOWED, MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,
    MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,

    MM_NO_ACCESS_ALLOWED, MM_READ_ONLY_ALLOWED, MM_READ_ONLY_ALLOWED,
    MM_READ_ONLY_ALLOWED, MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,
    MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,

    MM_NO_ACCESS_ALLOWED, MM_READ_ONLY_ALLOWED, MM_READ_ONLY_ALLOWED,
    MM_READ_ONLY_ALLOWED, MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,
    MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,

    MM_NO_ACCESS_ALLOWED, MM_READ_ONLY_ALLOWED, MM_READ_ONLY_ALLOWED,
    MM_READ_ONLY_ALLOWED, MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,
    MM_READ_WRITE_ALLOWED, MM_READ_WRITE_ALLOWED,
};

/* FUNCTIONS ******************************************************************/

PMMVAD
NTAPI
MiLocateAddress(IN PVOID VirtualAddress)
{
    PMMVAD FoundVad;
    ULONG_PTR Vpn;
    PMM_AVL_TABLE Table = &PsGetCurrentProcess()->VadRoot;
    TABLE_SEARCH_RESULT SearchResult;

    /* Start with the the hint */
    FoundVad = (PMMVAD)Table->NodeHint;
    if (!FoundVad) return NULL;

    /* Check if this VPN is in the hint, if so, use it */
    Vpn = (ULONG_PTR)VirtualAddress >> PAGE_SHIFT;
    if ((Vpn >= FoundVad->StartingVpn) && (Vpn <= FoundVad->EndingVpn)) return FoundVad;

    /* VAD hint didn't work, go look for it */
    SearchResult = RtlpFindAvlTableNodeOrParent(Table,
                                                (PVOID)Vpn,
                                                (PMMADDRESS_NODE*)&FoundVad);
    if (SearchResult != TableFoundNode) return NULL;

    /* We found it, update the hint */
    ASSERT(FoundVad != NULL);
    ASSERT((Vpn >= FoundVad->StartingVpn) && (Vpn <= FoundVad->EndingVpn));
    Table->NodeHint = FoundVad;
    return FoundVad;
}

TABLE_SEARCH_RESULT
NTAPI
MiCheckForConflictingNode(IN ULONG_PTR StartVpn,
                          IN ULONG_PTR EndVpn,
                          IN PMM_AVL_TABLE Table,
                          OUT PMMADDRESS_NODE *NodeOrParent)
{
    PMMADDRESS_NODE ParentNode, CurrentNode;

    /* If the tree is empty, there is no conflict */
    if (Table->NumberGenericTableElements == 0) return TableEmptyTree;

    /* Start looping from the root node */
    CurrentNode = RtlRightChildAvl(&Table->BalancedRoot);
    ASSERT(CurrentNode != NULL);
    while (CurrentNode)
    {
        ParentNode = CurrentNode;

        /* This address comes after */
        if (StartVpn > CurrentNode->EndingVpn)
        {
            /* Keep searching on the right */
            CurrentNode = RtlRightChildAvl(CurrentNode);
        }
        else if (EndVpn < CurrentNode->StartingVpn)
        {
            /* This address ends before the node starts, search on the left */
            CurrentNode = RtlLeftChildAvl(CurrentNode);
        }
        else
        {
            /* This address is part of this node, return it */
            *NodeOrParent = ParentNode;
            return TableFoundNode;
        }
    }

    /* There is no more child, save the current node as parent */
    *NodeOrParent = ParentNode;
    if (StartVpn > ParentNode->EndingVpn)
    {
        return TableInsertAsRight;
    }
    else
    {
        return TableInsertAsLeft;
    }
}

VOID
NTAPI
MiInsertNode(IN PMM_AVL_TABLE Table,
             IN PMMADDRESS_NODE NewNode,
             IN PMMADDRESS_NODE Parent,
             IN TABLE_SEARCH_RESULT Result)
{
    PMMVAD_LONG Vad;

    /* Insert it into the tree */
    RtlpInsertAvlTreeNode(Table, NewNode, Parent, Result);

    /* Now insert an ARM3 MEMORY_AREA for this node, unless the insert was already from the MEMORY_AREA code */
    Vad = (PMMVAD_LONG)NewNode;
    if (Vad->u.VadFlags.Spare == 0)
    {
        NTSTATUS Status;
        PMEMORY_AREA MemoryArea;
        SIZE_T Size;
        PEPROCESS Process = CONTAINING_RECORD(Table, EPROCESS, VadRoot);
        PVOID AllocatedBase = (PVOID)(Vad->StartingVpn << PAGE_SHIFT);

        Size = ((Vad->EndingVpn + 1) - Vad->StartingVpn) << PAGE_SHIFT;
        Status = MmCreateMemoryArea(&Process->Vm,
                                    MEMORY_AREA_OWNED_BY_ARM3,
                                    &AllocatedBase,
                                    Size,
                                    PAGE_READWRITE,
                                    &MemoryArea,
                                    TRUE,
                                    0,
                                    PAGE_SIZE);
        ASSERT(NT_SUCCESS(Status));

        /* Check if this is VM VAD */
        if (Vad->ControlArea == NULL)
        {
            /* We store the reactos MEMORY_AREA here */
            Vad->FirstPrototypePte = (PMMPTE)MemoryArea;
        }
        else
        {
            /* This is a section VAD. Store the MAREA here for now */
            ASSERT(Vad->u4.Banked == (PVOID)0xDEADBABE);
            Vad->u4.Banked = (PVOID)MemoryArea;
        }
    }
}

VOID
NTAPI
MiInsertVad(IN PMMVAD Vad,
            IN PEPROCESS Process)
{
    TABLE_SEARCH_RESULT Result;
    PMMADDRESS_NODE Parent = NULL;

    /* Validate the VAD and set it as the current hint */
    ASSERT(Vad->EndingVpn >= Vad->StartingVpn);
    Process->VadRoot.NodeHint = Vad;

    /* Find the parent VAD and where this child should be inserted */
    Result = RtlpFindAvlTableNodeOrParent(&Process->VadRoot, (PVOID)Vad->StartingVpn, &Parent);
    ASSERT(Result != TableFoundNode);
    ASSERT((Parent != NULL) || (Result == TableEmptyTree));

    /* Do the actual insert operation */
    MiLockProcessWorkingSetUnsafe(PsGetCurrentProcess(), PsGetCurrentThread());
    MiInsertNode(&Process->VadRoot, (PVOID)Vad, Parent, Result);
    MiUnlockProcessWorkingSetUnsafe(PsGetCurrentProcess(), PsGetCurrentThread());
}

NTSTATUS
NTAPI
MiInsertVadEx(
    _Inout_ PMMVAD Vad,
    _In_ ULONG_PTR *BaseAddress,
    _In_ SIZE_T ViewSize,
    _In_ ULONG_PTR HighestAddress,
    _In_ ULONG_PTR Alignment,
    _In_ ULONG AllocationType)
{
    ULONG_PTR StartingAddress, EndingAddress;
    PEPROCESS CurrentProcess;
    PETHREAD CurrentThread;
    TABLE_SEARCH_RESULT Result;
    PMMADDRESS_NODE Parent;

    /* Align the view size to pages */
    ViewSize = ALIGN_UP_BY(ViewSize, PAGE_SIZE);

    /* Get the current process */
    CurrentProcess = PsGetCurrentProcess();

    /* Acquire the address creation lock and make sure the process is alive */
    KeAcquireGuardedMutex(&CurrentProcess->AddressCreationLock);
    if (CurrentProcess->VmDeleted)
    {
        KeReleaseGuardedMutex(&CurrentProcess->AddressCreationLock);
        DPRINT1("The process is dying\n");
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Did the caller specify an address? */
    if (*BaseAddress == 0)
    {
        /* Make sure HighestAddress is not too large */
        HighestAddress = min(HighestAddress, (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS);

        /* Which way should we search? */
        if ((AllocationType & MEM_TOP_DOWN) || CurrentProcess->VmTopDown)
        {
            /* Find an address top-down */
            Result = MiFindEmptyAddressRangeDownTree(ViewSize,
                                                     HighestAddress,
                                                     Alignment,
                                                     &CurrentProcess->VadRoot,
                                                     &StartingAddress,
                                                     &Parent);
        }
        else
        {
            /* Find an address bottom-up */
            Result = MiFindEmptyAddressRangeInTree(ViewSize,
                                                   Alignment,
                                                   &CurrentProcess->VadRoot,
                                                   &Parent,
                                                   &StartingAddress);
        }

        /* Get the ending address, which is the last piece we need for the VAD */
        EndingAddress = StartingAddress + ViewSize - 1;

        /* Check if we found a suitable location */
        if ((Result == TableFoundNode) || (EndingAddress > HighestAddress))
        {
            DPRINT1("Not enough free space to insert this VAD node!\n");
            KeReleaseGuardedMutex(&CurrentProcess->AddressCreationLock);
            return STATUS_NO_MEMORY;
        }

        ASSERT(StartingAddress != 0);
        ASSERT(StartingAddress < (ULONG_PTR)HighestAddress);
        ASSERT(EndingAddress > StartingAddress);
    }
    else
    {
        /* Calculate the starting and ending address */
        StartingAddress = ALIGN_DOWN_BY(*BaseAddress, Alignment);
        EndingAddress = StartingAddress + ViewSize - 1;

        /* Make sure it doesn't conflict with an existing allocation */
        Result = MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                           EndingAddress >> PAGE_SHIFT,
                                           &CurrentProcess->VadRoot,
                                           &Parent);
        if (Result == TableFoundNode)
        {
            DPRINT1("Given address conflicts with existing node\n");
            KeReleaseGuardedMutex(&CurrentProcess->AddressCreationLock);
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    /* Now set the VAD address */
    Vad->StartingVpn = StartingAddress >> PAGE_SHIFT;
    Vad->EndingVpn = EndingAddress >> PAGE_SHIFT;

    /* Check if we already need to charge for the pages */
    if ((Vad->u.VadFlags.PrivateMemory && Vad->u.VadFlags.MemCommit) ||
        (!Vad->u.VadFlags.PrivateMemory &&
         (Vad->u.VadFlags.Protection & PAGE_WRITECOPY)))
    {
        /* Set the commit charge */
        Vad->u.VadFlags.CommitCharge = ViewSize / PAGE_SIZE;
    }

    /* Check if the VAD is to be secured */
    if (Vad->u2.VadFlags2.OneSecured)
    {
        /* This *must* be a long VAD! */
        ASSERT(Vad->u2.VadFlags2.LongVad);

        /* Yeah this is retarded, I didn't invent it! */
        ((PMMVAD_LONG)Vad)->u3.Secured.StartVpn = StartingAddress;
        ((PMMVAD_LONG)Vad)->u3.Secured.EndVpn = EndingAddress;
    }

    /* Lock the working set */
    CurrentThread = PsGetCurrentThread();
    MiLockProcessWorkingSetUnsafe(CurrentProcess, CurrentThread);

    /* Insert the VAD */
    CurrentProcess->VadRoot.NodeHint = Vad;
    MiInsertNode(&CurrentProcess->VadRoot, (PVOID)Vad, Parent, Result);

    /* Release the working set */
    MiUnlockProcessWorkingSetUnsafe(CurrentProcess, CurrentThread);

    /* Update the process' virtual size, and peak virtual size */
    CurrentProcess->VirtualSize += ViewSize;
    if (CurrentProcess->VirtualSize > CurrentProcess->PeakVirtualSize)
    {
        CurrentProcess->PeakVirtualSize = CurrentProcess->VirtualSize;
    }

    /* Unlock the address space */
    KeReleaseGuardedMutex(&CurrentProcess->AddressCreationLock);

    *BaseAddress = StartingAddress;
    return STATUS_SUCCESS;
}

VOID
NTAPI
MiInsertBasedSection(IN PSECTION Section)
{
    TABLE_SEARCH_RESULT Result;
    PMMADDRESS_NODE Parent = NULL;
    ASSERT(Section->Address.EndingVpn >= Section->Address.StartingVpn);

    /* Find the parent VAD and where this child should be inserted */
    Result = RtlpFindAvlTableNodeOrParent(&MmSectionBasedRoot, (PVOID)Section->Address.StartingVpn, &Parent);
    ASSERT(Result != TableFoundNode);
    ASSERT((Parent != NULL) || (Result == TableEmptyTree));
    MiInsertNode(&MmSectionBasedRoot, &Section->Address, Parent, Result);
}

VOID
NTAPI
MiRemoveNode(IN PMMADDRESS_NODE Node,
             IN PMM_AVL_TABLE Table)
{
    PMMVAD_LONG Vad;

    /* Call the AVL code */
    RtlpDeleteAvlTreeNode(Table, Node);

    /* Decrease element count */
    Table->NumberGenericTableElements--;

    /* Check if this node was the hint */
    if (Table->NodeHint == Node)
    {
        /* Get a new hint, unless we're empty now, in which case nothing */
        if (!Table->NumberGenericTableElements) Table->NodeHint = NULL;
        else Table->NodeHint = Table->BalancedRoot.RightChild;
    }

    /* Free the node from ReactOS view as well */
    Vad = (PMMVAD_LONG)Node;
    if (Vad->u.VadFlags.Spare == 0)
    {
        PMEMORY_AREA MemoryArea;
        PEPROCESS Process;

        /* Check if this is VM VAD */
        if (Vad->ControlArea == NULL)
        {
            /* We store the ReactOS MEMORY_AREA here */
            MemoryArea = (PMEMORY_AREA)Vad->FirstPrototypePte;
        }
        else
        {
            /* This is a section VAD. We store the ReactOS MEMORY_AREA here */
            MemoryArea = (PMEMORY_AREA)Vad->u4.Banked;
        }

        /* Make sure one actually still exists */
        if (MemoryArea)
        {
            /* Make sure we have not already freed it */
            ASSERT(MemoryArea != (PVOID)0xDEADBAB1);

            /* Get the process */
            Process = CONTAINING_RECORD(Table, EPROCESS, VadRoot);

            /* We only create fake memory-areas for ARM3 VADs */
            ASSERT(MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3);
            ASSERT(MemoryArea->Vad == NULL);

            /* Free it */
            MmFreeMemoryArea(&Process->Vm, MemoryArea, NULL, NULL);

            /* Check if this is VM VAD */
            if (Vad->ControlArea == NULL)
            {
                /* Delete the pointer to it */
                Vad->FirstPrototypePte = (PVOID)0xDEADBAB1;
            }
            else
            {
                /* Delete the pointer to it */
                Vad->u4.Banked = (PVOID)0xDEADBAB1;
            }
        }
    }
}

PMMADDRESS_NODE
NTAPI
MiGetPreviousNode(IN PMMADDRESS_NODE Node)
{
    PMMADDRESS_NODE Parent;

    /* Get the left child */
    if (RtlLeftChildAvl(Node))
    {
        /* Get right-most child */
        Node = RtlLeftChildAvl(Node);
        while (RtlRightChildAvl(Node)) Node = RtlRightChildAvl(Node);
        return Node;
    }

    Parent = RtlParentAvl(Node);
    ASSERT(Parent != NULL);
    while (Parent != Node)
    {
        /* The parent should be a right child, return the real predecessor */
        if (RtlIsRightChildAvl(Node))
        {
            /* Return it unless it's the root */
            if (Parent == RtlParentAvl(Parent)) Parent = NULL;
            return Parent;
        }

        /* Keep lopping until we find our parent */
        Node = Parent;
        Parent = RtlParentAvl(Node);
    }

    /* Nothing found */
    return NULL;
}

PMMADDRESS_NODE
NTAPI
MiGetNextNode(IN PMMADDRESS_NODE Node)
{
    PMMADDRESS_NODE Parent;

    /* Get the right child */
    if (RtlRightChildAvl(Node))
    {
        /* Get left-most child */
        Node = RtlRightChildAvl(Node);
        while (RtlLeftChildAvl(Node)) Node = RtlLeftChildAvl(Node);
        return Node;
    }

    Parent = RtlParentAvl(Node);
    ASSERT(Parent != NULL);
    while (Parent != Node)
    {
        /* The parent should be a left child, return the real predecessor */
        if (RtlIsLeftChildAvl(Node))
        {
            /* Return it */
            return Parent;
        }

        /* Keep lopping until we find our parent */
        Node = Parent;
        Parent = RtlParentAvl(Node);
    }

    /* Nothing found */
    return NULL;
}

TABLE_SEARCH_RESULT
NTAPI
MiFindEmptyAddressRangeInTree(IN SIZE_T Length,
                              IN ULONG_PTR Alignment,
                              IN PMM_AVL_TABLE Table,
                              OUT PMMADDRESS_NODE *PreviousVad,
                              OUT PULONG_PTR Base)
{
    PMMADDRESS_NODE Node, PreviousNode;
    ULONG_PTR PageCount, AlignmentVpn, LowVpn, HighVpn;
    ASSERT(Length != 0);

    /* Calculate page numbers for the length, alignment, and starting address */
    PageCount = BYTES_TO_PAGES(Length);
    AlignmentVpn = Alignment >> PAGE_SHIFT;
    LowVpn = ALIGN_UP_BY((ULONG_PTR)MM_LOWEST_USER_ADDRESS >> PAGE_SHIFT, AlignmentVpn);

    /* Check if the table is empty */
    if (Table->NumberGenericTableElements == 0)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = LowVpn << PAGE_SHIFT;
        return TableEmptyTree;
    }

    /* Otherwise, follow the leftmost child of the right root node's child */
    Node = RtlRightChildAvl(&Table->BalancedRoot);
    while (RtlLeftChildAvl(Node)) Node = RtlLeftChildAvl(Node);

    /* Start a search to find a gap */
    PreviousNode = NULL;
    while (Node != NULL)
    {
        /* Check if the gap below the current node is suitable */
        if (Node->StartingVpn >= LowVpn + PageCount)
        {
            /* There is enough space to add our node */
            *Base = LowVpn << PAGE_SHIFT;

            /* Can we use the current node as parent? */
            if (RtlLeftChildAvl(Node) == NULL)
            {
                /* Node has no left child, so use it as parent */
                *PreviousVad = Node;
                return TableInsertAsLeft;
            }
            else
            {
                /* Node has a left child, this means that the previous node is
                   the right-most child of it's left child and can be used as
                   the parent. In case we use the space before the left-most
                   node, it's left child must be NULL. */
                ASSERT(PreviousNode != NULL);
                ASSERT(RtlRightChildAvl(PreviousNode) == NULL);
                *PreviousVad = PreviousNode;
                return TableInsertAsRight;
            }
        }

        /* The next candidate is above the current node */
        if (Node->EndingVpn >= LowVpn)
            LowVpn = ALIGN_UP_BY(Node->EndingVpn + 1, AlignmentVpn);

        /* Remember the current node and go to the next node */
        PreviousNode = Node;
        Node = MiGetNextNode(Node);
    }

    /* We're up to the highest VAD, will this allocation fit above it? */
    HighVpn = ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) / PAGE_SIZE;
    if (HighVpn >= LowVpn + PageCount)
    {
        /* Yes! Use this VAD to store the allocation */
        *PreviousVad = PreviousNode;
        *Base = LowVpn << PAGE_SHIFT;
        return TableInsertAsRight;
    }

    /* Nyet, there's no free address space for this allocation, so we'll fail */
    return TableFoundNode;
}

TABLE_SEARCH_RESULT
NTAPI
MiFindEmptyAddressRangeDownTree(IN SIZE_T Length,
                                IN ULONG_PTR BoundaryAddress,
                                IN ULONG_PTR Alignment,
                                IN PMM_AVL_TABLE Table,
                                OUT PULONG_PTR Base,
                                OUT PMMADDRESS_NODE *Parent)
{
    PMMADDRESS_NODE Node, OldNode, Child;
    ULONG_PTR LowVpn, HighVpn, AlignmentVpn;
    PFN_NUMBER PageCount;

    /* Sanity checks */
    ASSERT(BoundaryAddress);
    ASSERT(BoundaryAddress <= ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS));
    ASSERT((Alignment & (PAGE_SIZE - 1)) == 0);

    /* Calculate page numbers for the length and alignment */
    Length = ROUND_TO_PAGES(Length);
    PageCount = Length >> PAGE_SHIFT;
    AlignmentVpn = Alignment / PAGE_SIZE;

    /* Check if there is enough space below the boundary */
    if ((ALIGN_UP_BY((ULONG_PTR)MM_LOWEST_USER_ADDRESS, Alignment) + Length) >
        (BoundaryAddress + 1))
    {
        return TableFoundNode;
    }

    /* Check if the table is empty */
    if (Table->NumberGenericTableElements == 0)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = ALIGN_DOWN_BY(BoundaryAddress + 1 - Length, Alignment);
        return TableEmptyTree;
    }

    /* Calculate the initial upper margin */
    HighVpn = (BoundaryAddress + 1) >> PAGE_SHIFT;

    /* Starting from the root, follow the right children until we found a node
       that ends above the boundary */
    Node = RtlRightChildAvl(&Table->BalancedRoot);
    while ((Node->EndingVpn < HighVpn) &&
           ((Child = RtlRightChildAvl(Node)) != NULL)) Node = Child;

    /* Now loop the Vad nodes */
    while (Node)
    {
        /* Calculate the lower margin */
        LowVpn = ALIGN_UP_BY(Node->EndingVpn + 1, AlignmentVpn);

        /* Check if the current bounds are suitable */
        if ((HighVpn > LowVpn) && ((HighVpn - LowVpn) >= PageCount))
        {
            /* There is enough space to add our node */
            LowVpn = ALIGN_DOWN_BY(HighVpn - PageCount, AlignmentVpn);
            *Base = LowVpn << PAGE_SHIFT;

            /* Can we use the current node as parent? */
            if (!RtlRightChildAvl(Node))
            {
                /* Node has no right child, so use it as parent */
                *Parent = Node;
                return TableInsertAsRight;
            }
            else
            {
                /* Node has a right child, the node we had before is the most
                   left grandchild of that right child, use it as parent. */
                *Parent = OldNode;
                return TableInsertAsLeft;
            }
        }

        /* Update the upper margin if necessary */
        if (Node->StartingVpn < HighVpn) HighVpn = Node->StartingVpn;

        /* Remember the current node and go to the previous node */
        OldNode = Node;
        Node = MiGetPreviousNode(Node);
    }

    /* Check if there's enough space before the lowest Vad */
    LowVpn = ALIGN_UP_BY((ULONG_PTR)MI_LOWEST_VAD_ADDRESS, Alignment) / PAGE_SIZE;
    if ((HighVpn > LowVpn) && ((HighVpn - LowVpn) >= PageCount))
    {
        /* There is enough space to add our address */
        LowVpn = ALIGN_DOWN_BY(HighVpn - PageCount, Alignment >> PAGE_SHIFT);
        *Base = LowVpn << PAGE_SHIFT;
        *Parent = OldNode;
        return TableInsertAsLeft;
    }

    /* No address space left at all */
    *Base = 0;
    *Parent = NULL;
    return TableFoundNode;
}

NTSTATUS
NTAPI
MiFindEmptyAddressRangeDownBasedTree(IN SIZE_T Length,
                                     IN ULONG_PTR BoundaryAddress,
                                     IN ULONG_PTR Alignment,
                                     IN PMM_AVL_TABLE Table,
                                     OUT PULONG_PTR Base)
{
    PMMADDRESS_NODE Node, LowestNode;
    ULONG_PTR LowVpn, BestVpn;

    /* Sanity checks */
    ASSERT(Table == &MmSectionBasedRoot);
    ASSERT(BoundaryAddress);
    ASSERT(BoundaryAddress <= ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));

    /* Compute page length, make sure the boundary address is valid */
    Length = ROUND_TO_PAGES(Length);
    if ((BoundaryAddress + 1) < Length) return STATUS_NO_MEMORY;

    /* Check if the table is empty */
    BestVpn = ROUND_DOWN(BoundaryAddress + 1 - Length, Alignment);
    if (Table->NumberGenericTableElements == 0)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = BestVpn;
        return STATUS_SUCCESS;
    }

    /* Go to the right-most node which should be the biggest address */
    Node = Table->BalancedRoot.RightChild;
    while (RtlRightChildAvl(Node)) Node = RtlRightChildAvl(Node);

    /* Check if we can fit in here */
    LowVpn = ROUND_UP(Node->EndingVpn + 1, Alignment);
    if ((LowVpn < BoundaryAddress) && (Length <= (BoundaryAddress - LowVpn)))
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        /* Return the address. */
        *Base = BestVpn;
#else
        /* Note: this is a compatibility hack that mimics a bug in the 2k3
           kernel. It will can waste up to Alignment bytes of memory above
           the allocation. This bug was fixed in Windows Vista */
        *Base = ROUND_DOWN(BoundaryAddress - Length, Alignment);
#endif
        return STATUS_SUCCESS;
    }

    /* Now loop the Vad nodes */
    do
    {
        /* Break out if we've reached the last node */
        LowestNode = MiGetPreviousNode(Node);
        if (!LowestNode) break;

        /* Check if this node could contain the requested address */
        LowVpn = ROUND_UP(LowestNode->EndingVpn + 1, Alignment);
        if ((LowestNode->EndingVpn < BestVpn) &&
            (LowVpn < Node->StartingVpn) &&
            (Length <= (Node->StartingVpn - LowVpn)))
        {
            /* Check if we need to take BoundaryAddress into account */
            if (BoundaryAddress < Node->StartingVpn)
            {
                /* Return the optimal VPN address */
                *Base = BestVpn;
                return STATUS_SUCCESS;
            }
            else
            {
                /* The upper margin is given by the Node's starting address */
                *Base = ROUND_DOWN(Node->StartingVpn - Length, Alignment);
                return STATUS_SUCCESS;
            }
        }

        /* Move to the next node */
        Node = LowestNode;
    } while (TRUE);

    /* Check if there's enough space before the lowest Vad */
    if ((Node->StartingVpn > (ULONG_PTR)MI_LOWEST_VAD_ADDRESS) &&
        ((Node->StartingVpn - (ULONG_PTR)MI_LOWEST_VAD_ADDRESS) >= Length))
    {
        /* Check if it fits in perfectly */
        if (BoundaryAddress < Node->StartingVpn)
        {
            /* Return the optimal VPN address */
            *Base = BestVpn;
            return STATUS_SUCCESS;
        }

        /* Return an aligned base address within this node */
        *Base = ROUND_DOWN(Node->StartingVpn - Length, Alignment);
        return STATUS_SUCCESS;
    }

    /* No address space left at all */
    return STATUS_NO_MEMORY;
}

NTSTATUS
NTAPI
MiCheckSecuredVad(IN PMMVAD Vad,
                  IN PVOID Base,
                  IN SIZE_T Size,
                  IN ULONG ProtectionMask)
{
    ULONG_PTR StartAddress, EndAddress;

    /* Compute start and end address */
    StartAddress = (ULONG_PTR)Base;
    EndAddress = StartAddress + Size - 1;

    /* Are we deleting/unmapping, or changing? */
    if (ProtectionMask < MM_DELETE_CHECK)
    {
        /* Changing... are we allowed to do so? */
        if ((Vad->u.VadFlags.NoChange == 1) &&
            (Vad->u2.VadFlags2.SecNoChange == 1) &&
            (Vad->u.VadFlags.Protection != ProtectionMask))
        {
            /* Nope, bail out */
            DPRINT1("Trying to mess with a no-change VAD!\n");
            return STATUS_INVALID_PAGE_PROTECTION;
        }
    }
    else
    {
        /* This is allowed */
        ProtectionMask = 0;
    }

    /* ARM3 doesn't support this yet */
    ASSERT(Vad->u2.VadFlags2.MultipleSecured == 0);

    /* Is this a one-secured VAD, like a TEB or PEB? */
    if (Vad->u2.VadFlags2.OneSecured)
    {
        /* Is this allocation being described by the VAD? */
        if ((StartAddress <= ((PMMVAD_LONG)Vad)->u3.Secured.EndVpn) &&
            (EndAddress >= ((PMMVAD_LONG)Vad)->u3.Secured.StartVpn))
        {
            /* Guard page? */
            if (ProtectionMask & MM_DECOMMIT)
            {
                DPRINT1("Not allowed to change protection on guard page!\n");
                return STATUS_INVALID_PAGE_PROTECTION;
            }

            /* ARM3 doesn't have read-only VADs yet */
            ASSERT(Vad->u2.VadFlags2.ReadOnly == 0);

            /* Check if read-write protections are allowed */
            if (MmReadWrite[ProtectionMask] < MM_READ_WRITE_ALLOWED)
            {
                DPRINT1("Invalid protection mask for RW access!\n");
                return STATUS_INVALID_PAGE_PROTECTION;
            }
        }
    }

    /* All good, allow the change */
    return STATUS_SUCCESS;
}

/* EOF */

