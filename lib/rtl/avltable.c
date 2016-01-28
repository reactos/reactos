/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/rtl/avltable.c
 * PURPOSE:         AVL Tree Generic Table Implementation
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* Include RTL version of AVL support */
#include "rtlavl.h"
#include "avlsupp.c"

/* AVL FUNCTIONS *************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeGenericTableAvl(IN OUT PRTL_AVL_TABLE Table,
                             IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
                             IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
                             IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
                             IN PVOID TableContext)
{
    /* Setup the table */
    RtlZeroMemory(Table, sizeof(RTL_AVL_TABLE));
    Table->BalancedRoot.Parent = &Table->BalancedRoot;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(IN PRTL_AVL_TABLE Table,
                                    IN PVOID Buffer,
                                    IN ULONG BufferSize,
                                    OUT PBOOLEAN NewElement OPTIONAL,
                                    IN OUT PVOID NodeOrParent,
                                    IN OUT TABLE_SEARCH_RESULT SearchResult)
{
    PRTL_BALANCED_LINKS NewNode;
    PVOID UserData;

    /* Check if the entry wasn't already found */
    if (SearchResult != TableFoundNode)
    {
        /* We're doing an allocation, sanity check */
        ASSERT(Table->NumberGenericTableElements != (MAXULONG - 1));

        /* Allocate a node */
        NewNode = Table->AllocateRoutine(Table,
                                         BufferSize +
                                         FIELD_OFFSET(TABLE_ENTRY_HEADER,
                                                      UserData));
        if (!NewNode)
        {
            /* No memory or other allocation error, fail */
            if (NewElement) *NewElement = FALSE;
            return NULL;
        }

        /* Data to return to user */
        UserData = &((PTABLE_ENTRY_HEADER)NewNode)->UserData;

        /* Insert the node in the tree */
        RtlZeroMemory(NewNode, sizeof(RTL_BALANCED_LINKS));
        RtlpInsertAvlTreeNode(Table, NewNode, NodeOrParent, SearchResult);

        /* Copy user buffer */
        RtlCopyMemory(UserData, Buffer, BufferSize);
    }
    else
    {
        /* Return the node we already found */
        NewNode = NodeOrParent;
        UserData = &((PTABLE_ENTRY_HEADER)NewNode)->UserData;
    }

    /* Return status */
    if (NewElement) *NewElement = (SearchResult != TableFoundNode);

    /* Return pointer to user data */
    return UserData;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlInsertElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                IN PVOID Buffer,
                                IN ULONG BufferSize,
                                OUT PBOOLEAN NewElement OPTIONAL)
{
    PRTL_BALANCED_LINKS NodeOrParent = NULL;
    TABLE_SEARCH_RESULT Result;

    /* Get the balanced links and table search result immediately */
    Result = RtlpFindAvlTableNodeOrParent(Table, Buffer, &NodeOrParent);

    /* Now call the routine to do the full insert */
    return RtlInsertElementGenericTableFullAvl(Table,
                                               Buffer,
                                               BufferSize,
                                               NewElement,
                                               NodeOrParent,
                                               Result);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(IN PRTL_AVL_TABLE Table)
{
    /* If there's no elements, the table is empty */
    return Table->NumberGenericTableElements == 0;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(IN PRTL_AVL_TABLE Table)
{
    /* Return the element count */
    return Table->NumberGenericTableElements;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(IN PRTL_AVL_TABLE Table,
                                    IN PVOID Buffer,
                                    IN OUT PVOID *NodeOrParent,
                                    IN OUT TABLE_SEARCH_RESULT *SearchResult)
{
    /* Find the node */
    *SearchResult = RtlpFindAvlTableNodeOrParent(Table,
                                                 Buffer,
                                                 (PRTL_BALANCED_LINKS*)NodeOrParent);
    if (*SearchResult != TableFoundNode) return NULL;

    /* Node found, return the user data */
    return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlLookupElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                IN PVOID Buffer)
{
    PRTL_BALANCED_LINKS NodeOrParent;
    TABLE_SEARCH_RESULT Lookup;

    /* Call the full function */
    return RtlLookupElementGenericTableFullAvl(Table,
                                               Buffer,
                                               (PVOID*)&NodeOrParent,
                                               &Lookup);
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableAvl(IN PRTL_AVL_TABLE Table,
                            IN BOOLEAN Restart)
{
    /* Reset the restart key if needed */
    if (Restart) Table->RestartKey = NULL;

    /* Call the full function */
    return RtlEnumerateGenericTableWithoutSplayingAvl(Table,
                                                      (PVOID*)&Table->RestartKey);
}

/*
* @implemented
*/
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                             IN PVOID Buffer,
                                             OUT PVOID *RestartKey)
{
    PRTL_BALANCED_LINKS Node, PreviousNode;
    TABLE_SEARCH_RESULT SearchResult;
    RTL_GENERIC_COMPARE_RESULTS Result = GenericEqual;

    /* Assume failure */
    *RestartKey = NULL;

    /* Find the node */
    SearchResult = RtlpFindAvlTableNodeOrParent(Table, Buffer, &Node);
    if (SearchResult != TableFoundNode) return NULL;

    /* Scan each predecessor until a match is found */
    PreviousNode = Node;
    while (Result == GenericEqual)
    {
        /* Save the node */
        Node = PreviousNode;

        /* Get the predecessor */
        PreviousNode = RtlRealPredecessorAvl(Node);
        if ((!PreviousNode) || (RtlParentAvl(PreviousNode) == PreviousNode)) break;

        /* Check if this node matches */
        Result = RtlpAvlCompareRoutine(Table,
                                       Buffer,
                                       &((PTABLE_ENTRY_HEADER)PreviousNode)->
                                       UserData);
    }

    /* Save the node as the restart key, and return its data */
    *RestartKey = Node;
    return &((PTABLE_ENTRY_HEADER)Node)->UserData;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(IN PRTL_AVL_TABLE Table,
                                           IN OUT PVOID *RestartKey)
{
    PRTL_BALANCED_LINKS CurrentNode;
     
    /* Skip an empty tree */
    if (RtlIsGenericTableEmptyAvl(Table)) return NULL;

    /* Check if we have a starting point */
    if (!*RestartKey)
    {
        /* We'll have to find it, keep going until the leftmost child */
        for (CurrentNode = RtlRightChildAvl(&Table->BalancedRoot);
             RtlLeftChildAvl(CurrentNode);
             CurrentNode = RtlLeftChildAvl(CurrentNode));

        /* Found it */
        *RestartKey = CurrentNode;
    }
    else
    {
        /* We already had a child, keep going by getting its successor */
        CurrentNode = RtlRealSuccessorAvl(*RestartKey);

        /* If there was one, update the restart key */
        if (CurrentNode) *RestartKey = CurrentNode;
    }
    
    /* Return the node's data if it was found, otherwise return NULL */
    if (CurrentNode) return &((PTABLE_ENTRY_HEADER)CurrentNode)->UserData;
    return NULL;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlGetElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                             IN ULONG I)
{
    UNIMPLEMENTED;
	return NULL;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                IN PVOID Buffer)
{
    PRTL_BALANCED_LINKS Node;
    TABLE_SEARCH_RESULT SearchResult;

    /* Find the node */
    SearchResult = RtlpFindAvlTableNodeOrParent(Table, Buffer, &Node);
    if (SearchResult != TableFoundNode) return FALSE;
    
    /* If this node was the key, update it */
    if (Node == Table->RestartKey) Table->RestartKey = RtlRealPredecessorAvl(Node);
    
    /* Do the delete */
    Table->DeleteCount++;
    RtlpDeleteAvlTreeNode(Table, Node);
    Table->NumberGenericTableElements--;
    
    /* Reset accounting */
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = NULL;

    /* Free the node's data */
    Table->FreeRoutine(Table, Node);

    /* It's done */
    return TRUE;
}

/* EOF */
