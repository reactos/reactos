/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Generic Table Implementation
 * FILE:            lib/rtl/genertictbl.c
 * PROGRAMMERS:     arty
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include "austin/avl.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
PVOID
NTAPI
RtlLookupElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlLookupElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @implemented
*/
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
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
RtlLookupElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	PRTL_BALANCED_LINKS OurNodeOrParent;
	TABLE_SEARCH_RESULT OurSearchResult;
	return RtlLookupElementGenericTableFullAvl 
	(Table, Buffer, (PVOID *)&OurNodeOrParent, &OurSearchResult);
}


/*
* @unimplemented
*/
BOOLEAN
NTAPI
RtlDeleteElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @implemented
*/
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	TABLE_SEARCH_RESULT Result;
	PRTL_BALANCED_LINKS Node;

	RtlLookupElementGenericTableFullAvl
		( Table, Buffer, (PVOID *)&Node, &Result );

	if( Result == TableFoundNode ) 
	{
		avl_delete_node(Table, Node);
		Table->FreeRoutine(Table, Node);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlEnumerateGenericTable (
	PRTL_GENERIC_TABLE Table,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @implemented
*/
PVOID
NTAPI
RtlEnumerateGenericTableAvl (
	PRTL_AVL_TABLE Table,
	BOOLEAN Restart
	)
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
* @unimplemented
*/
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory (
	IN PRTL_AVL_TABLE Table,
	IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
	IN PVOID MatchData,
	IN ULONG NextFlag,
	IN OUT PVOID *RestartKey,
	IN OUT PULONG DeleteCount,
	IN OUT PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying (
	PRTL_GENERIC_TABLE Table,
	PVOID *RestartKey
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @implemented
*/
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl (
	PRTL_AVL_TABLE Table,
	PVOID *RestartKey
	)
{
	return RtlEnumerateGenericTableWithoutSplayingAvl(Table, RestartKey);
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlGetElementGenericTable(
	PRTL_GENERIC_TABLE Table,
	ULONG I
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @implemented
*/
PVOID
NTAPI
RtlGetElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	ULONG I
	)
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
RtlInitializeGenericTable(PRTL_GENERIC_TABLE Table,
                          PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
                          PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
                          PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
                          PVOID TableContext)
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
VOID NTAPI
RtlInitializeGenericTableAvl(IN OUT PRTL_AVL_TABLE Table,
                             IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
                             IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
                             IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
                             IN PVOID TableContext)
{
  RtlZeroMemory(Table,
                sizeof(RTL_AVL_TABLE));
  Table->BalancedRoot.Parent = &Table->BalancedRoot;
  Table->CompareRoutine = CompareRoutine;
  Table->AllocateRoutine = AllocateRoutine;
  Table->FreeRoutine = FreeRoutine;
  Table->TableContext = TableContext;
  avl_init(Table);
}


/*
* @unimplemented
*/
PVOID
NTAPI
RtlInsertElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
    UNIMPLEMENTED;
    return 0;
}

/*
* @unimplemented
*/
PVOID
NTAPI
RtlInsertElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @implemented
*/
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID *NodeOrParent,
	TABLE_SEARCH_RESULT *SearchResult
	)
{
	PRTL_BALANCED_LINKS OurNodeOrParent;
	TABLE_SEARCH_RESULT OurSearchResult;

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
RtlInsertElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
	PVOID NodeOrParent;
	TABLE_SEARCH_RESULT SearchResult;

	return RtlInsertElementGenericTableFullAvl
	(Table, Buffer, BufferSize, NewElement, &NodeOrParent, &SearchResult);
}


/*
* @unimplemented
*/
BOOLEAN
NTAPI
RtlIsGenericTableEmpty (
	PRTL_GENERIC_TABLE Table
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl (
	PRTL_AVL_TABLE Table
	)
{
	return Table->NumberGenericTableElements == 0;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlNumberGenericTableElements(IN PRTL_GENERIC_TABLE Table)
{
  return Table->NumberGenericTableElements;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlNumberGenericTableElementsAvl(IN PRTL_AVL_TABLE Table)
{
  return Table->NumberGenericTableElements;
}

/* EOF */
