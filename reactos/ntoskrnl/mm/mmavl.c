/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmavl.c
 * PURPOSE:         AVL tree routines for the memory manager
 *
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************'**********/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _MMADDRESS_LIST
{   
    ULONG_PTR StartVpn;  
    ULONG_PTR EndVpn;    
} MMADDRESS_LIST, *PMMADDRESS_LIST; 

typedef struct _MMADDRESS_NODE
{
    union
    {
        LONG_PTR Balance : 2;
        struct _MMADDRESS_NODE* Parent;
    } u1;
    struct _MMADDRESS_NODE* LeftChild;
    struct _MMADDRESS_NODE* RightChild;
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _MM_AVL_TABLE
{
    MMADDRESS_NODE BalancedRoot;
    struct
    {
        ULONG_PTR DepthOfTree : 5;
        ULONG_PTR Unused : 3;
#ifdef _WIN64
        ULONG_PTR NumberGenericTableElements : 56;
#else
        ULONG_PTR NumberGenericTableElements : 24;
#endif
    };
    PVOID NodeHint;
    PVOID NodeFreeHint;
} MM_AVL_TABLE, *PMM_AVL_TABLE;

enum
{
    BALANCED = 0,
    LEFTHEAVY = -1,
    RIGHTHEAVY = 1,
    UNBALANCED = -2,
};

/* FUNCTIONS *****************************************************************/

PMMADDRESS_NODE
FORCEINLINE
MiAvlGetParent(
    IN PMMADDRESS_NODE Node)
{
    return (PMMADDRESS_NODE)((ULONG_PTR)Node->u1.Parent & ~3);
}

void
FORCEINLINE
MiAvlSetParent(
    IN PMMADDRESS_NODE Node,
    IN PMMADDRESS_NODE Parent)
{
    ULONG_PTR Balance;

    /* First mask everything except the balance */
    Balance =(ULONG_PTR)Node->u1.Parent & 3;
    
    Node->u1.Parent = (PMMADDRESS_NODE)(Balance | (ULONG_PTR)Parent);
}

void
FORCEINLINE
MiAvlShiftBalanceLeft(PMMADDRESS_NODE Node)
{
    ULONG_PTR Balance = (ULONG_PTR)Node->u1.Parent;
    Balance--;
    Balance &= 3;
    Node->u1.Parent = (PVOID)((ULONG_PTR)MiAvlGetParent(Node) | Balance);
}

void
FORCEINLINE
MiAvlShiftBalanceRight(PMMADDRESS_NODE Node)
{
    ULONG_PTR Balance = (ULONG_PTR)Node->u1.Parent;
    Balance++;
    Balance &= 3;
    Node->u1.Parent = (PVOID)((ULONG_PTR)MiAvlGetParent(Node) | Balance);
}

PMMADDRESS_NODE
FORCEINLINE
MiAvlRotateRight(
    PMMADDRESS_NODE OldParent,
    PMMADDRESS_NODE OldLeftChild)
{
    PMMADDRESS_NODE NewParent, NewRightChild;

    /* The old left child becomes the new parent */
    NewParent = OldLeftChild;

    /* The old parent becomes the new right child */
    NewRightChild = OldParent;

    /* Move the former right child of the old left child over 
       to the right child */
    NewRightChild->LeftChild = OldLeftChild->RightChild;

    /* Set parent, keep balance, may change BalancedRoot.u1.Parent */
    MiAvlSetParent(NewRightChild->LeftChild, NewRightChild);

    /* Attach the new right child to the new parent */
    NewParent->RightChild = NewRightChild;
    NewRightChild->u1.Parent = NewParent;

    return NewParent;
}

PMMADDRESS_NODE
FORCEINLINE
MiAvlRotateLeft(
    PMMADDRESS_NODE OldParent,
    PMMADDRESS_NODE OldRightChild)
{
    PMMADDRESS_NODE NewParent, NewLeftChild;

    /* The old right child becomes the new parent */
    NewParent = OldRightChild;

    /* The old parent becomes the new left child */
    NewLeftChild = OldParent;

    /* Move the former left child of the old right child over 
       to the left child */
    NewLeftChild->RightChild = OldRightChild->LeftChild;
    
    /* Set parent, keep balance, may change BalancedRoot.u1.Parent */
    MiAvlSetParent(NewLeftChild->RightChild, NewLeftChild);

    /* Attach the new left child to the new parent */
    NewParent->LeftChild = NewLeftChild;
    NewLeftChild->u1.Parent = NewParent;

    return NewParent;
}

void
FORCEINLINE
MiAvlRebalance(
    IN PMMADDRESS_NODE OldParent,
    IN PMMADDRESS_NODE OldChild)
{
    PMMADDRESS_NODE NewParent, NewChild, GrandParent;

    /* Save grandparent for later */
    GrandParent = MiAvlGetParent(OldParent);

    if (OldChild == OldParent->LeftChild)
    {
        if (OldChild->u1.Balance == LEFTHEAVY)
        {
            /* left-left case */
            NewParent = MiAvlRotateRight(OldParent, OldChild);
        }
        else
        {
            /* left-right case */
            NewChild = MiAvlRotateLeft(OldChild, OldChild->RightChild);
            NewParent = MiAvlRotateRight(OldParent, NewChild);
        }
    }
    else /* (OldChild == OldParent->LeftChild) */
    {
        if (OldChild->u1.Balance == RIGHTHEAVY)
        {
            /* right-right case */
            NewParent = MiAvlRotateLeft(OldParent, OldChild);
        }
        else
        {
            /* right-left case */
            NewChild = MiAvlRotateRight(OldChild, OldChild->LeftChild);
            NewParent = MiAvlRotateLeft(OldParent, NewChild);
        }
    }

    /* Link to the grandparent */
    NewParent->u1.Parent = GrandParent;
    ASSERT(NewParent->u1.Balance == BALANCED);

    /* If GrandParent is &BalancedRoot, then this will be true */
    if (OldParent == GrandParent->RightChild)
    {
        GrandParent->RightChild = NewParent;
    }
    else
    {
        GrandParent->LeftChild = NewParent;
    }
}

VOID
NTAPI
MiInitializeAvlTable(
    IN PMM_AVL_TABLE AvlTable)
{
    /* Set all links to BalancedRoot. Nodes will be attached as RightChild */
    AvlTable->BalancedRoot.u1.Parent = &AvlTable->BalancedRoot;
    AvlTable->BalancedRoot.LeftChild = &AvlTable->BalancedRoot;
    AvlTable->BalancedRoot.RightChild = &AvlTable->BalancedRoot;

    /* Set all other members to 0 */
    AvlTable->BalancedRoot.StartingVpn = 0;
    AvlTable->BalancedRoot.EndingVpn = 0;
    AvlTable->DepthOfTree = 0;
    AvlTable->Unused = 0;
    AvlTable->NumberGenericTableElements = 0;
    AvlTable->NodeHint = NULL;
    AvlTable->NodeFreeHint = NULL;
}

ULONG
NTAPI
MiInsertElementAvlTable(
    IN PMM_AVL_TABLE AvlTable,
    IN PMMADDRESS_NODE NewNode)
{
    PMMADDRESS_NODE BalancedRoot, CurrentNode, ParentNode;
    ULONG Depth = 1;

    ASSERT(NewNode->StartingVpn <= NewNode->EndingVpn);

    BalancedRoot = &AvlTable->BalancedRoot;
    ParentNode = CurrentNode = BalancedRoot->RightChild;

    /* First walk down the AVL tree to find a proper position */
    while (CurrentNode != BalancedRoot)
    {
        ParentNode = CurrentNode;
        if (NewNode->EndingVpn < CurrentNode->StartingVpn)
        {
            /* Add the node at the left side */
            CurrentNode = CurrentNode->LeftChild;
        }
        else if (NewNode->StartingVpn > CurrentNode->EndingVpn)
        {
            /* Add the node at the right side */
            CurrentNode = CurrentNode->RightChild;
        }
        else
        {
            /* Clashing with current node, bail out */
            return 0;
        }

        /* Going one node down... */
        Depth++;
    }

    /* We have one element more now */
    AvlTable->NumberGenericTableElements++;

    /* We have a proper parent, attach the new node */
    NewNode->u1.Parent = ParentNode;
    NewNode->LeftChild = BalancedRoot;
    NewNode->RightChild = BalancedRoot;
    if (NewNode->EndingVpn < ParentNode->StartingVpn)
    {
        /* Attach the node at the left side */
        ParentNode->LeftChild = NewNode;
        MiAvlShiftBalanceLeft(ParentNode);
    }
    else /* (NewNode->StartingVpn > ParentNode->EndingVpn) */
    {
        /* Attach the node at the right side */
        ParentNode->RightChild = NewNode;
        MiAvlShiftBalanceRight(ParentNode);
    }

    /* This is a trick to optimize the loop. We simplify the exit condition,
       by making the BalancedRoot leftheavy. */
    BalancedRoot->u1.Balance = LEFTHEAVY;

    /* Walk the tree up again and rebalance */
    while (ParentNode->u1.Balance != BALANCED)
    {
        CurrentNode = ParentNode;
        ParentNode = MiAvlGetParent(CurrentNode);

        /* Check on what side the current node is attached */
        if (CurrentNode == ParentNode->LeftChild)
        {
            /* Left depth was increased */
            MiAvlShiftBalanceLeft(ParentNode);
        }
        else
        {
            /* Right depth was increased */
            MiAvlShiftBalanceRight(ParentNode);
        }

        /* Check if parent is unbalanced now */
        if (ParentNode->u1.Balance == UNBALANCED)
        {
            /* Rebalance it and bail out */
            MiAvlRebalance(ParentNode, CurrentNode);
            // FIXME: depth changed
            break;
        }
    }

    return Depth;
}

void
NTAPI
MiRemoveElementAvlTable(
    IN PMM_AVL_TABLE AvlTable,
    IN PMMADDRESS_NODE Node)
{
    UNIMPLEMENTED;
}


PMMADDRESS_NODE
NTAPI
MiLookupElementAvlTable(
    IN PMM_AVL_TABLE AvlTable,
    ULONG_PTR Vpn)
{
    PMMADDRESS_NODE BalancedRoot, CurrentNode;

    BalancedRoot = &AvlTable->BalancedRoot;
    CurrentNode = BalancedRoot->RightChild;

    /* Walk the AVL tree to find the node */
    do
    {
        if (Vpn < CurrentNode->StartingVpn)
        {
            /* Continue with the left child */
            CurrentNode = CurrentNode->LeftChild;
        }
        else if (Vpn > CurrentNode->EndingVpn)
        {
            /* Continue with the right child */
            CurrentNode = CurrentNode->RightChild;
        }
        else
        {
            /* Current node is the one */
            return CurrentNode;
        }
    }
    while (CurrentNode != BalancedRoot);

    /* Didn't find anything */
    return NULL;
}

