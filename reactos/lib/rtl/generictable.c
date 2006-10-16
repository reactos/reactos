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

/* FUNCTIONS *****************************************************************/

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
 * @unimplemented
 */
PVOID
NTAPI
RtlInsertElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer,
                             IN ULONG BufferSize,
                             OUT PBOOLEAN NewElement OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(IN PRTL_GENERIC_TABLE Table)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlNumberGenericTableElements(IN PRTL_GENERIC_TABLE Table)
{
    return Table->NumberGenericTableElements;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlLookupElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlLookupElementGenericTableFull(IN PRTL_GENERIC_TABLE Table,
                                 IN PVOID Buffer,
                                 OUT PVOID *NodeOrParent,
                                 OUT TABLE_SEARCH_RESULT *SearchResult)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                             IN PVOID Buffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlEnumerateGenericTable(IN PRTL_GENERIC_TABLE Table,
                         IN BOOLEAN Restart)
{
    UNIMPLEMENTED;
    return 0;
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
 * @unimplemented
 */
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(IN PRTL_GENERIC_TABLE Table,
                                        IN OUT PVOID *RestartKey)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlGetElementGenericTable(IN PRTL_GENERIC_TABLE Table,
                          IN ULONG I)
{
    UNIMPLEMENTED;
    return 0;
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
