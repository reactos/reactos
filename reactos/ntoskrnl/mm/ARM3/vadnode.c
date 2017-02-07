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
#include "../ARM3/miarm.h"

/* Include Mm version of AVL support */
#include "../ARM3/miavl.h"
#include "../../../lib/rtl/avlsupp.c"

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

PMMADDRESS_NODE
NTAPI
MiCheckForConflictingNode(IN ULONG_PTR StartVpn,
                          IN ULONG_PTR EndVpn,
                          IN PMM_AVL_TABLE Table)
{
    PMMADDRESS_NODE CurrentNode;

    /* If the tree is empty, there is no conflict */
    if (!Table->NumberGenericTableElements) return NULL;

    /* Start looping from the right */
    CurrentNode = RtlRightChildAvl(&Table->BalancedRoot);
    ASSERT(CurrentNode != NULL);
    while (CurrentNode)
    {
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
            break;
        }
    }

    /* Return either the conflicting node, or no node at all */
    return CurrentNode;
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
        PHYSICAL_ADDRESS BoundaryAddressMultiple;
        SIZE_T Size;
        PEPROCESS Process = CONTAINING_RECORD(Table, EPROCESS, VadRoot);
        PVOID AllocatedBase = (PVOID)(Vad->StartingVpn << PAGE_SHIFT);
        BoundaryAddressMultiple.QuadPart = 0;
        Size = ((Vad->EndingVpn + 1) - Vad->StartingVpn) << PAGE_SHIFT;
        Status = MmCreateMemoryArea(&Process->Vm,
                                    MEMORY_AREA_OWNED_BY_ARM3,
                                    &AllocatedBase,
                                    Size,
                                    PAGE_READWRITE,
                                    &MemoryArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
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
    MiInsertNode(&Process->VadRoot, (PVOID)Vad, Parent, Result);
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

NTSTATUS
NTAPI
MiFindEmptyAddressRangeInTree(IN SIZE_T Length,
                              IN ULONG_PTR Alignment,
                              IN PMM_AVL_TABLE Table,
                              OUT PMMADDRESS_NODE *PreviousVad,
                              OUT PULONG_PTR Base)
{
    PMMADDRESS_NODE Node;
    PMMADDRESS_NODE NextNode;
    ULONG_PTR StartingVpn, HighestVpn, AlignmentVpn, LengthVpn, LowVpn;
    ASSERT(Length != 0);

    /* Precompute page numbers for the length, alignment, and starting address */
    LengthVpn = (Length + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    AlignmentVpn = Alignment >> PAGE_SHIFT;
    StartingVpn = ROUND_UP((ULONG_PTR)MM_LOWEST_USER_ADDRESS >> PAGE_SHIFT,
                           AlignmentVpn);

    /* Check if the table is free, so the lowest possible address is available */
    if (!Table->NumberGenericTableElements) goto FoundAtBottom;

    /* Otherwise, follow the leftmost child of the right root node's child */
    Node = RtlRightChildAvl(&Table->BalancedRoot);
    while (RtlLeftChildAvl(Node)) Node = RtlLeftChildAvl(Node);

    /* This is the node for the remaining gap at the bottom, can it be used? */
    if ((Node->StartingVpn > StartingVpn) &&
        (LengthVpn < Node->StartingVpn - StartingVpn))
    {
FoundAtBottom:
        /* Use this VAD to store the allocation */
        *PreviousVad = NULL;
        *Base = StartingVpn << PAGE_SHIFT;
        return STATUS_SUCCESS;
    }

    /* Otherwise, we start a search to find a gap */
    while (TRUE)
    {
        /* The last aligned page number in this entry */
        LowVpn = ROUND_UP(Node->EndingVpn + 1, AlignmentVpn);

        /* Keep going as long as there's still a next node */
        NextNode = MiGetNextNode(Node);
        if (!NextNode) break;

        /* Can this allocation fit in this node? */
        if ((LengthVpn <= (NextNode->StartingVpn - LowVpn)) &&
            (NextNode->StartingVpn > LowVpn))
        {
Found:
            /* Yes! Use this VAD to store the allocation */
            *PreviousVad = Node;
            *Base = ROUND_UP((Node->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1),
                             Alignment);
            return STATUS_SUCCESS;
        }

        /* Try the next node */
        Node = NextNode;
    }

    /* We're down to the last (top) VAD, will this allocation fit inside it? */
    HighestVpn = ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) >> PAGE_SHIFT;
    if ((HighestVpn > LowVpn) && (LengthVpn <= HighestVpn - LowVpn)) goto Found;

    /* Nyet, there's no free address space for this allocation, so we'll fail */
    return STATUS_NO_MEMORY;
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
    PMMADDRESS_NODE Node, LowestNode, Child;
    ULONG_PTR LowVpn, HighVpn;
    PFN_NUMBER PageCount;

    /* Sanity checks */
    ASSERT(BoundaryAddress);
    ASSERT(BoundaryAddress <= ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));

    /* Compute page length, make sure the boundary address is valid */
    Length = ROUND_TO_PAGES(Length);
    PageCount = Length >> PAGE_SHIFT;
    if ((BoundaryAddress + 1) < Length) return TableFoundNode;

    /* Check if the table is empty */
    if (Table->NumberGenericTableElements == 0)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = ROUND_UP(BoundaryAddress + 1 - Length, Alignment);
        return TableEmptyTree;
    }

    /* Calculate the initial upper margin */
    HighVpn = BoundaryAddress >> PAGE_SHIFT;

    /* Starting from the root, go down until the right-most child
     * which is just behind the boundary*/
    LowestNode = Node = RtlRightChildAvl(&Table->BalancedRoot);
    while (((Child = RtlRightChildAvl(Node)) != 0 )
            && (Node->EndingVpn < HighVpn )) Node = Child;

    /* Now loop the Vad nodes */
    while (Node)
    {
        /* Keep track of the lowest node */
        LowestNode = Node;

        /* Calculate the lower margin */
        LowVpn = ROUND_UP(Node->EndingVpn + 1, Alignment >> PAGE_SHIFT);

        /* Check if the current bounds are suitable */
        if ((HighVpn > LowVpn) && ((HighVpn - LowVpn) >= PageCount))
        {
            /* There is enough space to add our node */
            LowVpn = HighVpn - PageCount;
            *Base = LowVpn << PAGE_SHIFT;

            /* Can we use the current node as parent? */
            Child = RtlRightChildAvl(Node);
            if (!Child)
            {
                /* Node has no right child, so use it as parent */
                *Parent = Node;
                return TableInsertAsRight;
            }
            else
            {
                /* Node has a right child, find most left grand child */
                Node = Child;
                while ((Child = RtlLeftChildAvl(Node))) Node = Child;
                *Parent = Node;
                return TableInsertAsLeft;
            }
        }

        /* Update the upper margin if neccessary */
        if (Node->StartingVpn < HighVpn) HighVpn = Node->StartingVpn;

        /* Go to the next lower node */
        Node = MiGetPreviousNode(Node);
    }

    /* Check if there's enough space before the lowest Vad */
    LowVpn = ROUND_UP((ULONG_PTR)MI_LOWEST_VAD_ADDRESS, Alignment) >> PAGE_SHIFT;
    if ((HighVpn > LowVpn) && ((HighVpn - LowVpn) >= PageCount))
    {
        /* There is enough space to add our address */
        LowVpn = HighVpn - PageCount;
        *Base = LowVpn << PAGE_SHIFT;
        *Parent = LowestNode;
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
    BestVpn = ROUND_UP(BoundaryAddress + 1 - Length, Alignment);
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
    LowVpn = ROUND_UP(Node->EndingVpn, Alignment);
    if ((LowVpn < BoundaryAddress) && (Length < (BoundaryAddress - LowVpn)))
    {
        /* Return the address */
        *Base = ROUND_UP(BoundaryAddress - Length, Alignment);
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
            (Length <= (Node->StartingVpn - LowVpn)))
        {
            /* Check if it fits in perfectly */
            if ((BestVpn > LowestNode->EndingVpn) &&
                (BoundaryAddress < Node->StartingVpn))
            {
                /* Return the optimal VPN address */
                *Base = BestVpn;
                return STATUS_SUCCESS;
            }

            /* It doesn't, check if it can partly fit */
            if (Node->StartingVpn > LowVpn)
            {
                /* Return an aligned base address within this node */
                *Base = ROUND_UP(Node->StartingVpn - Length, Alignment);
                return STATUS_SUCCESS;
            }
        }

        /* Move to the next node */
        Node = LowestNode;
    } while (TRUE);

    /* Check if there's enough space before the lowest Vad */
    if ((Node->StartingVpn > (ULONG_PTR)MI_LOWEST_VAD_ADDRESS) &&
        ((Node->StartingVpn - (ULONG_PTR)MI_LOWEST_VAD_ADDRESS) > Length))
    {
        /* Check if it fits in perfectly */
        if (BoundaryAddress < Node->StartingVpn)
        {
            /* Return the optimal VPN address */
            *Base = BestVpn;
            return STATUS_SUCCESS;
        }

        /* Return an aligned base address within this node */
        *Base = ROUND_UP(Node->StartingVpn - Length, Alignment);
        return STATUS_SUCCESS;
    }

    /* No address space left at all */
    return STATUS_NO_MEMORY;
}

/* EOF */
