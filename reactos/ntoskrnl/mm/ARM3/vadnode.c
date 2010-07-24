/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/vadnode.c
 * PURPOSE:         ARM Memory Manager VAD Node Algorithms
 * PROGRAMMERS:     ReactOS Portable Systems Group
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
MiInsertNode(IN PMMADDRESS_NODE NewNode,
             IN PMM_AVL_TABLE Table)
{
    PMMADDRESS_NODE NodeOrParent = NULL;
    TABLE_SEARCH_RESULT Result;

    /* Find the node's parent, and where to insert this node */
    Result = RtlpFindAvlTableNodeOrParent(Table,
                                          (PVOID)NewNode->StartingVpn,
                                          &NodeOrParent);
    
    /* Insert it into the tree */
    RtlpInsertAvlTreeNode(Table, NewNode, NodeOrParent, Result);
}

VOID
NTAPI
MiRemoveNode(IN PMMADDRESS_NODE Node,
             IN PMM_AVL_TABLE Table)
{
    /* Call the AVL code */
    RtlpDeleteAvlTreeNode(Table, Node);
    
    /* Decrease element count */
    Table->NumberGenericTableElements--;
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

NTSTATUS
NTAPI
MiFindEmptyAddressRangeDownTree(IN SIZE_T Length,
                                IN ULONG_PTR BoundaryAddress,
                                IN ULONG_PTR Alignment,
                                IN PMM_AVL_TABLE Table,
                                OUT PULONG_PTR Base)
{
    PMMADDRESS_NODE Node, PreviousNode;
    ULONG_PTR CandidateAddress, EndAddress;
    ULONG AlignEndVpn, CandidateVpn, BoundaryVpn, LowestVpn, StartVpn, EndVpn;
    PFN_NUMBER PageCount;

    /* Sanity checks */
    ASSERT(BoundaryAddress);
    ASSERT(BoundaryAddress <= ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));
    
    /* Compute page length, make sure the boundary address is valid */
    Length = PAGE_ROUND_UP(Length);
    if ((BoundaryAddress + 1) < Length) return STATUS_NO_MEMORY;
    
    /* Compute the highest address to start at */
    CandidateAddress = ROUND_UP(BoundaryAddress + 1 - Length, Alignment);

    /* Check if the table is empty */
    if (!Table->NumberGenericTableElements)
    {
        /* Tree is empty, the candidate address is already the best one */
        *Base = CandidateAddress;
        return STATUS_SUCCESS;
    }

    /* Starting from the root, go down until the right-most child */
    Node = RtlRightChildAvl(&Table->BalancedRoot);
    while (RtlRightChildAvl(Node)) Node = RtlRightChildAvl(Node);

    /* Get the aligned ending address of this VPN */
    EndAddress = ROUND_UP((Node->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1),
                          Alignment);

    /* Can we fit the address without overflowing into the node? */
    if ((EndAddress < BoundaryAddress) &&
        ((BoundaryAddress - EndAddress) > Length))
    {
        /* There is enough space to add our address */
        *Base = ROUND_UP(BoundaryAddress - Length, Alignment);
        return STATUS_SUCCESS;
    }
    
    PageCount = Length >> PAGE_SHIFT;
    CandidateVpn = CandidateAddress >> PAGE_SHIFT;
    BoundaryVpn = BoundaryAddress >> PAGE_SHIFT;
    LowestVpn = (ULONG_PTR)MI_LOWEST_VAD_ADDRESS >> PAGE_SHIFT;
         
    PreviousNode = MiGetPreviousNode(Node);
         
    StartVpn = Node->StartingVpn;
    EndVpn = PreviousNode ? PreviousNode->EndingVpn : 0;
    AlignEndVpn = ROUND_UP(EndVpn + 1, Alignment >> PAGE_SHIFT);
         
    /* Loop until a gap is found */
    for (PageCount = Length >> PAGE_SHIFT,
         CandidateVpn = CandidateAddress >> PAGE_SHIFT,
         BoundaryVpn = BoundaryAddress >> PAGE_SHIFT,
         LowestVpn = (ULONG_PTR)MI_LOWEST_VAD_ADDRESS >> PAGE_SHIFT,
         PreviousNode = MiGetPreviousNode(Node),
         StartVpn = Node->StartingVpn,
         EndVpn = PreviousNode ? PreviousNode->EndingVpn : 0,
         AlignEndVpn = ROUND_UP(EndVpn + 1, Alignment >> PAGE_SHIFT);
         PreviousNode;
         Node = PreviousNode,
         PreviousNode = MiGetPreviousNode(Node),
         StartVpn = Node->StartingVpn,
         EndVpn = PreviousNode ? PreviousNode->EndingVpn : 0,
         AlignEndVpn = ROUND_UP(EndVpn + 1, Alignment >> PAGE_SHIFT))
    {
        /* Can we fit the address without overflowing into the node? */
        if ((StartVpn < CandidateVpn) && ((StartVpn - AlignEndVpn) >= PageCount))
        {
            /* Check if we can get our candidate address */
            if ((CandidateVpn > EndVpn) && (BoundaryVpn < StartVpn))
            {
                /* Use it */
                *Base = CandidateAddress;
                return STATUS_SUCCESS;
            }
            
            /* Otherwise, can we fit it by changing the start address? */
            if (StartVpn > AlignEndVpn)
            {
                /* It'll fit, compute the new base address for that to work */
                *Base = ROUND_UP((StartVpn << PAGE_SHIFT) - Length, Alignment);
                return STATUS_SUCCESS;
            }
        }
        
        PreviousNode = MiGetPreviousNode(Node);
        StartVpn = Node->StartingVpn;
        EndVpn = PreviousNode ? PreviousNode->EndingVpn : 0;
        AlignEndVpn = ROUND_UP(EndVpn + 1, Alignment >> PAGE_SHIFT);
    }

    /* See if we could squeeze into the last descriptor */
    if ((StartVpn > LowestVpn) && ((StartVpn - LowestVpn) >= PageCount))
    {
        /* Check if we can try our candidate address */
        if (BoundaryVpn < StartVpn)
        {
            /* Use it */
            *Base = CandidateAddress;
            return STATUS_SUCCESS;
        }

        /* Otherwise, change the base address to what's needed to fit in */
        *Base = ROUND_UP((StartVpn << PAGE_SHIFT) - Length, Alignment);
        return STATUS_SUCCESS;
    }
        
    /* No address space left at all */
    return STATUS_NO_MEMORY;
}

/* EOF */
