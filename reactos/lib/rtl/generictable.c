/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            lib/rtl/generictable.c
* PURPOSE:         Splay Tree and AVL Tree Generic Table Implementation
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Art Yerks (ayerkes@speakeasy.net)
*/

/* INCLUDES ******************************************************************/

#include <rtl.h>
#include "austin/avl.h"
#define NDEBUG
#include <debug.h>

/* Internal header for table entries */
typedef struct _TABLE_ENTRY_HEADER
{
    RTL_SPLAY_LINKS SplayLinks;
    LIST_ENTRY ListEntry;
    LONGLONG UserData;
} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

/* PRIVATE FUNCTIONS *********************************************************/

TABLE_SEARCH_RESULT
NTAPI
RtlpFindGenericTableNodeOrParent(IN PRTL_GENERIC_TABLE Table,
                                 IN PVOID Buffer,
                                 OUT PRTL_SPLAY_LINKS *NodeOrParent)
{
    PRTL_SPLAY_LINKS CurrentNode, ChildNode;
    RTL_GENERIC_COMPARE_RESULTS Result;

    /* Quick check to see if the table is empty */
    if (RtlIsGenericTableEmpty(Table))
    {
        *NodeOrParent = NULL;
        return TableEmptyTree;
    }

    /* Set the current node */
    CurrentNode = Table->TableRoot;

    /* Start compare loop */
    while (TRUE)
    {
        /* Do the compare */
        Result = Table->CompareRoutine(Table,
                                       Buffer,
                                       &((PTABLE_ENTRY_HEADER)CurrentNode)->
                                       UserData);
        if (Result == GenericLessThan)
        {
            /* We're less, check if this is the left child */
            if ((ChildNode = RtlLeftChild(CurrentNode)))
            {
                /* Continue searching from this node */
                ChildNode = CurrentNode;
            }
            else
            {
                /* Otherwise, the element isn't in this tree */
                *NodeOrParent = CurrentNode;
                return TableInsertAsLeft;
            }
        }
        else if (Result == GenericGreaterThan)
        {
            /* We're more, check if this is the right child */
            if ((ChildNode = RtlRightChild(CurrentNode)))
            {
                /* Continue searching from this node */
                ChildNode = CurrentNode;
            }
            else
            {
                /* Otherwise, the element isn't in this tree */
                *NodeOrParent = CurrentNode;
                return TableInsertAsRight;
            }
        }
        else
        {
            /* We should've found the node */
            ASSERT(Result == GenericEqual);

            /* Return node found */
            *NodeOrParent = CurrentNode;
            return TableFoundNode;
        }
    }
}

/* SPLAY FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeGenericTable(IN PRTL_GENERIC_TABLE Table,
                          IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
                          IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
                          IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
                          IN PVOID TableContext)
{
    /* Initialize the table to default and passed values */
    InitializeListHead(&Table->InsertOrderList);
    Table->TableRoot = NULL;
    Table->NumberGenericTableElements = 0;
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = &Table->InsertOrderList;
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
RtlInsertElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer,
                             IN ULONG BufferSize,
                             OUT PBOOLEAN NewElement OPTIONAL)
{
    PRTL_SPLAY_LINKS NodeOrParent;
    TABLE_SEARCH_RESULT Result;

    /* Get the splay links and table search result immediately */
    Result = RtlpFindGenericTableNodeOrParent(Table, Buffer, &NodeOrParent);

    /* Now call the routine to do the full insert */
    return RtlInsertElementGenericTableFull(Table,
                                            Buffer,
                                            BufferSize,
                                            NewElement,
                                            NodeOrParent,
                                            Result);
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlInsertElementGenericTableFull(IN PRTL_GENERIC_TABLE Table,
                                 IN PVOID Buffer,
                                 IN ULONG BufferSize,
                                 OUT PBOOLEAN NewElement OPTIONAL,
                                 IN PVOID NodeOrParent,
                                 IN TABLE_SEARCH_RESULT SearchResult)
{
    PRTL_SPLAY_LINKS NewNode;

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

        /* Initialize the new inserted element */
        RtlInitializeSplayLinks(NewNode);
        InsertTailList(&Table->InsertOrderList,
                       &((PTABLE_ENTRY_HEADER)NewNode)->ListEntry);

        /* Increase element count */
        Table->NumberGenericTableElements++;

        /* Check where we should insert the entry */
        if (SearchResult == TableEmptyTree)
        {
            /* This is the new root node */
            Table->TableRoot = NewNode;
        }
        else if (SearchResult == TableInsertAsLeft)
        {
            /* Insert it left */
            RtlInsertAsLeftChild(NodeOrParent, NewNode);
        }
        else
        {
            /* Right node */
            RtlInsertAsRightChild(NodeOrParent, NewNode);
        }

        /* Copy user buffer */
        RtlCopyMemory(&((PTABLE_ENTRY_HEADER)NewNode)->UserData,
                      Buffer,
                      BufferSize);
    }
    else
    {
        /* Return the node we already found */
        NewNode = NodeOrParent;
    }

    /* Splay the tree */
    Table->TableRoot = RtlSplay(NewNode);

    /* Return status */
    if (NewElement) *NewElement = (SearchResult == TableFoundNode);

    /* Return pointer to user data */
    return &((PTABLE_ENTRY_HEADER)NewNode)->UserData;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(IN PRTL_GENERIC_TABLE Table)
{
    /* Check if the table root is empty */
    return (Table->TableRoot) ? FALSE: TRUE;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlNumberGenericTableElements(IN PRTL_GENERIC_TABLE Table)
{
    /* Return the number of elements */
    return Table->NumberGenericTableElements;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlLookupElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer)
{
    PRTL_SPLAY_LINKS NodeOrParent;
    TABLE_SEARCH_RESULT Result;

    /* Call the full version */
    return RtlLookupElementGenericTableFull(Table,
                                            Buffer,
                                            (PVOID)&NodeOrParent,
                                            &Result);
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlLookupElementGenericTableFull(IN PRTL_GENERIC_TABLE Table,
                                 IN PVOID Buffer,
                                 OUT PVOID *NodeOrParent,
                                 OUT TABLE_SEARCH_RESULT *SearchResult)
{
    /* Do the initial lookup */
    *SearchResult = RtlpFindGenericTableNodeOrParent(Table,
                                                     Buffer,
                                                     (PRTL_SPLAY_LINKS *)
                                                     NodeOrParent);

    /* Check if we found anything */
    if ((*SearchResult == TableEmptyTree) || (*SearchResult != TableFoundNode))
    {
        /* Nothing found */
        return NULL;
    }

    /* Otherwise, splay the tree and return this entry */
    Table->TableRoot = RtlSplay(*NodeOrParent);
    return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer)
{
    PRTL_SPLAY_LINKS NodeOrParent;
    TABLE_SEARCH_RESULT Result;

    /* Get the splay links and table search result immediately */
    Result = RtlpFindGenericTableNodeOrParent(Table, Buffer, &NodeOrParent);
    if ((Result == TableEmptyTree) || (Result != TableFoundNode))
    {
        /* Nothing to delete */
        return FALSE;
    }

    /* Delete the entry */
    Table->TableRoot = RtlDelete(NodeOrParent);
    RemoveEntryList(&((PTABLE_ENTRY_HEADER)NodeOrParent)->ListEntry);

    /* Update accounting data */
    Table->NumberGenericTableElements--;
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = &Table->InsertOrderList;

    /* Free the entry */
    Table->FreeRoutine(Table, NodeOrParent);
    return TRUE;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlEnumerateGenericTable(IN PRTL_GENERIC_TABLE Table,
                         IN BOOLEAN Restart)
{
    PRTL_SPLAY_LINKS FoundNode;

    /* Check if the table is empty */
    if (RtlIsGenericTableEmpty(Table)) return NULL;

    /* Check if we have to restart */
    if (Restart)
    {
        /* Then find the leftmost element */
        FoundNode = Table->TableRoot;
        do
        {
            /* Get the left child */
            FoundNode = RtlLeftChild(FoundNode);
        } while(RtlLeftChild(FoundNode));

        /* Splay it */
        Table->TableRoot = RtlSplay(FoundNode);
    }
    else
    {
        /* Otherwise, try using the real successor */
        FoundNode = RtlRealSuccessor(Table->TableRoot);
        if (FoundNode) Table->TableRoot = RtlSplay(FoundNode);
    }

    /* Check if we found the node and return it */
    return FoundNode ? &((PTABLE_ENTRY_HEADER)FoundNode)->UserData : NULL;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(IN PRTL_GENERIC_TABLE Table,
                                        IN OUT PVOID *RestartKey)
{
    PRTL_SPLAY_LINKS FoundNode;

    /* Check if the table is empty */
    if (RtlIsGenericTableEmpty(Table)) return NULL;

    /* Check if we have to restart */
    if (!(*RestartKey))
    {
        /* Then find the leftmost element */
        FoundNode = Table->TableRoot;
        do
        {
            /* Get the left child */
            FoundNode = RtlLeftChild(FoundNode);
        } while(RtlLeftChild(FoundNode));

        /* Splay it */
        *RestartKey = FoundNode;
    }
    else
    {
        /* Otherwise, try using the real successor */
        FoundNode = RtlRealSuccessor(Table->TableRoot);
        if (FoundNode) *RestartKey = FoundNode;
    }

    /* Check if we found the node and return it */
    return FoundNode ? &((PTABLE_ENTRY_HEADER)FoundNode)->UserData : NULL;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(IN PRTL_AVL_TABLE Table,
                                       IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
                                       IN PVOID MatchData,
                                       IN ULONG NextFlag,
                                       IN OUT PVOID *RestartKey,
                                       IN OUT PULONG DeleteCount,
                                       IN OUT PVOID Buffer)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlGetElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                          IN ULONG I)
{
    ULONG OrderedElement, ElementCount;
    PLIST_ENTRY OrderedNode;
    ULONG DeltaUp, DeltaDown;
    ULONG NextI = I + 1;

    /* Setup current accounting data */
    OrderedNode = Table->OrderedPointer;
    OrderedElement = Table->WhichOrderedElement;
    ElementCount = Table->NumberGenericTableElements;

    /* Sanity checks */
    if ((I == MAXULONG) || (NextI > ElementCount)) return NULL;

    /* Check if we already found the entry */
    if (NextI == OrderedElement)
    {
        /* Return it */
        return &((PTABLE_ENTRY_HEADER)CONTAINING_RECORD(OrderedNode,
                                                        TABLE_ENTRY_HEADER,
                                                        ListEntry))->UserData;
    }

    /* Now check if we're farther behind */
    if (OrderedElement > NextI)
    {
        /* Find out if the distance is more then the half-way point */
        if (NextI > (OrderedElement / 2))
        {
            /* Do the search backwards, since this takes less iterations */
            DeltaDown = OrderedElement - NextI;
            do
            {
                /* Get next node */
                OrderedNode = OrderedNode->Blink;
            } while (--DeltaDown);
        }
        else
        {
            /* Follow the list directly instead */
            OrderedNode = &Table->InsertOrderList;
            do
            {
                /* Get next node */
                OrderedNode = OrderedNode->Flink;
            } while (--NextI);
        }
    }
    else
    {
        /* We are farther ahead, calculate distances */
        DeltaUp = NextI - OrderedElement;
        DeltaDown = (ElementCount - NextI) + 1;

        /* Check if the up distance is smaller then the down distance */
        if (DeltaUp <= DeltaDown)
        {
            /* Do the search forwards, since this takes less iterations */
            do
            {
                /* Get next node */
                OrderedNode = OrderedNode->Blink;
            } while (--DeltaUp);
        }
        else
        {
            /* Do the search downwards, since this takes less iterations */
            OrderedNode = &Table->InsertOrderList;
            do
            {
                /* Get next node */
                OrderedNode = OrderedNode->Blink;
            } while (--DeltaDown);
        }
    }

    /* Got the element, save it */
    Table->OrderedPointer = OrderedNode;
    Table->WhichOrderedElement = NextI;

    /* Return the element */
    return &((PTABLE_ENTRY_HEADER)CONTAINING_RECORD(OrderedNode,
                                                    TABLE_ENTRY_HEADER,
                                                    ListEntry))->UserData;
}

/* AVL FUNCTIONS *************************************************************/

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
	PRTL_BALANCED_LINKS OurNodeOrParent;
	TABLE_SEARCH_RESULT OurSearchResult;

	if( !Table->NumberGenericTableElements )
	{
		*SearchResult = TableEmptyTree;
		*NodeOrParent = NULL;
		return NULL;
	}

	OurSearchResult = avl_search
		(Table, Buffer, 
		 Table->BalancedRoot.LeftChild, &OurNodeOrParent);

	if(SearchResult) *SearchResult = OurSearchResult;
	if(NodeOrParent) *NodeOrParent = OurNodeOrParent;

	if(OurSearchResult == TableFoundNode)
		return avl_data(OurNodeOrParent);
	else
		return NULL;
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlLookupElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                IN PVOID Buffer)
{
	PRTL_BALANCED_LINKS OurNodeOrParent;
	TABLE_SEARCH_RESULT OurSearchResult;
	return RtlLookupElementGenericTableFullAvl 
	(Table, Buffer, (PVOID *)&OurNodeOrParent, &OurSearchResult);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                IN PVOID Buffer)
{
	TABLE_SEARCH_RESULT Result;
	PRTL_BALANCED_LINKS Node;

	RtlLookupElementGenericTableFullAvl
		( Table, Buffer, (PVOID *)&Node, &Result );

	if( Result == TableFoundNode ) 
	{
		avl_delete_node(Table, Node);
		Table->FreeRoutine(Table, Node);
		if( Table->NumberGenericTableElements == 0 ) 
			avl_deinit(Table);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableAvl(IN PRTL_AVL_TABLE Table,
                            IN BOOLEAN Restart)
{
	if( Table->NumberGenericTableElements == 0 )
		return NULL;

	if( Restart )
	{
		Table->RestartKey = avl_first(Table);
	}
	else
	{
		Table->RestartKey = avl_next(Table, Table->RestartKey);
	}
	if( !Table->RestartKey )
		return NULL;
	else
		return avl_data(Table->RestartKey);
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(IN PRTL_AVL_TABLE Table,
                                           IN OUT PVOID *RestartKey)
{
    /* FIXME! */
	return RtlEnumerateGenericTableWithoutSplayingAvl(Table, RestartKey);
}

/*
 * @implemented
 */
PVOID
NTAPI
RtlGetElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                             IN ULONG I)
{
	PRTL_BALANCED_LINKS Node;

	if( I >= Table->NumberGenericTableElements ) return NULL;
	else
	{
		Node = avl_first(Table);
		while(I--) Node = avl_next(Table, Node);
		return avl_data(Node);
	}
}

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
                                    IN OUT PVOID *NodeOrParent,
                                    IN OUT TABLE_SEARCH_RESULT *SearchResult)
{
	PRTL_BALANCED_LINKS OurNodeOrParent;
	TABLE_SEARCH_RESULT OurSearchResult;

	if(Table->NumberGenericTableElements == 0) {
		avl_init(Table);
	}

	if(NewElement)
		*NewElement = FALSE;

	OurSearchResult = avl_search
		(Table, Buffer, 
		 Table->BalancedRoot.LeftChild, &OurNodeOrParent);

	if(NodeOrParent) *NodeOrParent = OurNodeOrParent;
	if(SearchResult) *SearchResult = OurSearchResult;

	if(OurSearchResult == TableFoundNode) 
	{
		RtlDeleteElementGenericTableAvl(Table, Buffer);
		return RtlInsertElementGenericTableFullAvl
			(Table, Buffer, BufferSize, 
			 NewElement, NodeOrParent, SearchResult);
	}
	else
	{
		PRTL_BALANCED_LINKS NewNode = 
			Table->AllocateRoutine
			(Table, 
			 BufferSize + sizeof(RTL_BALANCED_LINKS) + BufferSize);

		if( !NewNode ) return NULL;

		NewNode->Balance = 0;
		RtlCopyMemory(avl_data(NewNode), Buffer, BufferSize);

		OurNodeOrParent = NewNode;

		avl_insert_node(Table, NewNode);
		return avl_data(NewNode);
	}
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
	PVOID NodeOrParent;
	TABLE_SEARCH_RESULT SearchResult;

	return RtlInsertElementGenericTableFullAvl
	(Table, Buffer, BufferSize, NewElement, &NodeOrParent, &SearchResult);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(PRTL_AVL_TABLE Table)
{
    return Table->NumberGenericTableElements == 0;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(IN PRTL_AVL_TABLE Table)
{
    return Table->NumberGenericTableElements;
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(IN PRTL_AVL_TABLE Table,
                                             IN PVOID Buffer,
                                             OUT PVOID *RestartKey)
{
    UNIMPLEMENTED;
    return NULL;
}

/* EOF */
