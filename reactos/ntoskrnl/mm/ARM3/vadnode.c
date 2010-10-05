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

#line 15 "ARM³::VADNODE"
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
    /* Insert it into the tree */
    RtlpInsertAvlTreeNode(Table, NewNode, Parent, Result);
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
MiRemoveNode(IN PMMADDRESS_NODE Node,
             IN PMM_AVL_TABLE Table)
{
    DPRINT("Removing address node: %lx %lx\n", Node->StartingVpn, Node->EndingVpn);

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
    ULONG LowVpn, HighVpn;
    PFN_NUMBER PageCount;

    /* Sanity checks */
    ASSERT(BoundaryAddress);
    ASSERT(BoundaryAddress <= ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));
    
    /* Compute page length, make sure the boundary address is valid */
    Length = ROUND_TO_PAGES(Length);
    PageCount = Length >> PAGE_SHIFT;
    if ((BoundaryAddress + 1) < Length) return STATUS_NO_MEMORY;

    /* Check if the table is empty */
    if (Table->NumberGenericTableElements == 0)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = ROUND_DOWN(BoundaryAddress + 1 - Length, Alignment);
        return TableEmptyTree;
    }

    /* Calculate the initial upper margin */
    HighVpn = BoundaryAddress >> PAGE_SHIFT;

    /* Starting from the root, go down until the right-most child, 
       trying to stay below the boundary. */
    LowestNode = Node = RtlRightChildAvl(&Table->BalancedRoot);
    while ( (Child = RtlRightChildAvl(Node)) && 
             Child->EndingVpn < HighVpn ) Node = Child;

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

/* EOF */
