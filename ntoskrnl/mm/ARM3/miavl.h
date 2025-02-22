/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/miavl.h
 * PURPOSE:         ARM Memory Manager VAD Node Algorithms
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

/*
 * This is the glue code for the Memory Manager version of AVL Trees that is used
 * to store the MM_AVL_TABLE for Virtual Address Descriptors (VAD) in the VadRoot
 * field in EPROCESS.
 *
 * In this version of the package, the balance and parent pointers are stored in
 * the same field as a union (since we know the parent will be at least 8-byte
 * aligned), saving some space, but requiring special logic to handle setting and
 * querying the parent and balance.
 *
 * The other difference is that the AVL package for Rtl has custom callbacks for
 * comparison purposes (which would access some internal, opaque, user data) while
 * the Mm package stores the user-data inline as StartingVpn and EndingVpn. So
 * when a compare is being made, RtlpAvlCompareRoutine is called, which will either
 * perform the Mm work, or call the user-specified callback in the Rtl case.
 */
#define PRTL_AVL_TABLE              PMM_AVL_TABLE
#define PRTL_BALANCED_LINKS         PMMADDRESS_NODE
#define MI_ASSERT(x)                ASSERT(x)

/* We need to rename the functions to prevent conflicts when not inlined! */
#define RtlpFindAvlTableNodeOrParent MiFindAvlTableNodeOrParent
#define RtlpPromoteAvlTreeNode MiPromoteAvlTreeNode
#define RtlpRebalanceAvlTreeNode MiRebalanceAvlTreeNode
#define RtlpInsertAvlTreeNode MiInsertAvlTreeNode
#define RtlpDeleteAvlTreeNode MiDeleteAvlTreeNode

/* These are implementation specific */
#define RtlpCopyAvlNodeData MiCopyAvlNodeData
#define RtlpAvlCompareRoutine MiAvlCompareRoutine
#define RtlSetParent MiSetParent
#define RtlSetBalance MiSetBalance
#define RtlBalance MiBalance
#define RtlParentAvl MiParentAvl
#define RtlRightChildAvl MiRightChildAvl
#define RtlLeftChildAvl MiLeftChildAvl
#define RtlIsLeftChildAvl MiIsLeftChildAvl
#define RtlIsRightChildAvl MiIsRightChildAvl
#define RtlInsertAsLeftChildAvl MiInsertAsLeftChildAvl
#define RtlInsertAsRightChildAvl MiInsertAsRightChildAvl

FORCEINLINE
VOID
MiCopyAvlNodeData(IN PRTL_BALANCED_LINKS Node1,
                    IN PRTL_BALANCED_LINKS Node2)
{
    Node1->u1.Parent = Node2->u1.Parent;
    Node1->LeftChild = Node2->LeftChild;
    Node1->RightChild = Node2->RightChild;
}

FORCEINLINE
RTL_GENERIC_COMPARE_RESULTS
MiAvlCompareRoutine(IN PRTL_AVL_TABLE Table,
                      IN PVOID Buffer,
                      IN PVOID UserData)
{
    PRTL_BALANCED_LINKS CurrentNode = (PVOID)((ULONG_PTR)UserData - sizeof(RTL_BALANCED_LINKS));
    ULONG_PTR StartingVpn = (ULONG_PTR)Buffer;
    if (StartingVpn < CurrentNode->StartingVpn)
    {
        return GenericLessThan;
    }
    else if (StartingVpn <= CurrentNode->EndingVpn)
    {
        return GenericEqual;
    }
    else
    {
        return GenericGreaterThan;
    }
}

FORCEINLINE
VOID
MiSetParent(IN PRTL_BALANCED_LINKS Node,
             IN PRTL_BALANCED_LINKS Parent)
{
    Node->u1.Parent = (PRTL_BALANCED_LINKS)((ULONG_PTR)Parent | (Node->u1.Balance & 0x3));
}

FORCEINLINE
VOID
MiSetBalance(IN PRTL_BALANCED_LINKS Node,
              IN SCHAR Balance)
{
    Node->u1.Balance = Balance;
}

FORCEINLINE
SCHAR
MiBalance(IN PRTL_BALANCED_LINKS Node)
{
    return (SCHAR)Node->u1.Balance;
}

FORCEINLINE
PRTL_BALANCED_LINKS
MiParentAvl(IN PRTL_BALANCED_LINKS Node)
{
    return (PRTL_BALANCED_LINKS)((ULONG_PTR)Node->u1.Parent & ~3);
}

FORCEINLINE
PRTL_BALANCED_LINKS
MiRightChildAvl(IN PRTL_BALANCED_LINKS Node)
{
    return Node->RightChild;
}

FORCEINLINE
PRTL_BALANCED_LINKS
MiLeftChildAvl(IN PRTL_BALANCED_LINKS Node)
{
    return Node->LeftChild;
}

FORCEINLINE
BOOLEAN
MiIsLeftChildAvl(IN PRTL_BALANCED_LINKS Node)
{
   return (RtlLeftChildAvl(RtlParentAvl(Node)) == Node);
}

FORCEINLINE
BOOLEAN
MiIsRightChildAvl(IN PRTL_BALANCED_LINKS Node)
{
   return (RtlRightChildAvl(RtlParentAvl(Node)) == Node);
}

FORCEINLINE
VOID
MiInsertAsLeftChildAvl(IN PRTL_BALANCED_LINKS Parent,
                        IN PRTL_BALANCED_LINKS Node)
{
    Parent->LeftChild = Node;
    RtlSetParent(Node, Parent);
}

FORCEINLINE
VOID
MiInsertAsRightChildAvl(IN PRTL_BALANCED_LINKS Parent,
                         IN PRTL_BALANCED_LINKS Node)
{
    Parent->RightChild = Node;
    RtlSetParent(Node, Parent);
}

/* EOF */
