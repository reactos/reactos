/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/rtl/rtlavl.h
 * PURPOSE:         RTL AVL Glue
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

/*
 * This is the glue code for the AVL package in the RTL meant for external callers.
 * It's not very exciting, it just uses the RTL-defined fields without any magic,
 * unlike the Mm version which has special handling for balances and parents, and
 * does not implement custom comparison callbacks.
 */
#define MI_ASSERT(x)
#define RtlLeftChildAvl(x)          (PRTL_BALANCED_LINKS)(RtlLeftChild(x))
#define RtlRightChildAvl(x)         (PRTL_BALANCED_LINKS)(RtlRightChild(x))
#define RtlParentAvl(x)             (PRTL_BALANCED_LINKS)(RtlParent(x))
#define RtlRealPredecessorAvl(x)    (PRTL_BALANCED_LINKS)(RtlRealPredecessor((PRTL_SPLAY_LINKS)(x)))
#define RtlRealSuccessorAvl(x)      (PRTL_BALANCED_LINKS)(RtlRealSuccessor((PRTL_SPLAY_LINKS)(x)))
#define RtlInsertAsRightChildAvl    RtlInsertAsRightChild
#define RtlInsertAsLeftChildAvl     RtlInsertAsLeftChild
#define RtlIsLeftChildAvl           RtlIsLeftChild

FORCEINLINE
VOID
RtlpCopyAvlNodeData(IN PRTL_BALANCED_LINKS Node1,
                    IN PRTL_BALANCED_LINKS Node2)
{
    *Node1 = *Node2;
}

FORCEINLINE
RTL_GENERIC_COMPARE_RESULTS
RtlpAvlCompareRoutine(IN PRTL_AVL_TABLE Table,
                      IN PVOID Buffer,
                      IN PVOID UserData)
{
    /* Do the compare */
    return Table->CompareRoutine(Table,
                                 Buffer,
                                 UserData);
}

FORCEINLINE
VOID
RtlSetParent(IN PRTL_BALANCED_LINKS Node,
             IN PRTL_BALANCED_LINKS Parent)
{
    Node->Parent = Parent;
}

FORCEINLINE
VOID
RtlSetBalance(IN PRTL_BALANCED_LINKS Node,
              IN CHAR Balance)
{
    Node->Balance = Balance;
}

FORCEINLINE
CHAR
RtlBalance(IN PRTL_BALANCED_LINKS Node)
{
    return Node->Balance;
}

/* EOF */
