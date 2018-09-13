/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Gentable.c

Abstract:

    This module implements the generic table package.

Author:

    Gary Kimura     [GaryKi]    23-May-1989

Environment:

    Pure Utility Routines

Revision History:

    Anthony V. Ercolano [tonye] 23-May-1990

    Implement package.

    Anthony V. Ercolano [tonye] 1-Jun-1990

    Added ability to get elements out in the order
    inserted.  *NOTE* *NOTE* This depends on the implicit
    ordering of record fields:

        SPLAY_LINKS,
        LIST_ENTRY,
        USER_DATA

--*/

#include <nt.h>

#include <ntrtl.h>

#pragma pack(8)

//
// This structure is the header for a generic table entry.
// Align this structure on a 8 byte boundary so the user
// data is correctly aligned.
//

typedef struct _TABLE_ENTRY_HEADER {

    RTL_SPLAY_LINKS SplayLinks;
    LIST_ENTRY ListEntry;
    LONGLONG UserData;

} TABLE_ENTRY_HEADER, *PTABLE_ENTRY_HEADER;

#pragma pack()


static
TABLE_SEARCH_RESULT
FindNodeOrParent(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    OUT PRTL_SPLAY_LINKS *NodeOrParent
    )

/*++

Routine Description:

    This routine is used by all of the routines of the generic
    table package to locate the a node in the tree.  It will
    find and return (via the NodeOrParent parameter) the node
    with the given key, or if that node is not in the tree it
    will return (via the NodeOrParent parameter) a pointer to
    the parent.

Arguments:

    Table - The generic table to search for the key.

    Buffer - Pointer to a buffer holding the key.  The table
             package doesn't examine the key itself.  It leaves
             this up to the user supplied compare routine.

    NodeOrParent - Will be set to point to the node containing the
                   the key or what should be the parent of the node
                   if it were in the tree.  Note that this will *NOT*
                   be set if the search result is TableEmptyTree.

Return Value:

    TABLE_SEARCH_RESULT - TableEmptyTree: The tree was empty.  NodeOrParent
                                          is *not* altered.

                          TableFoundNode: A node with the key is in the tree.
                                          NodeOrParent points to that node.

                          TableInsertAsLeft: Node with key was not found.
                                             NodeOrParent points to what would be
                                             parent.  The node would be the left
                                             child.

                          TableInsertAsRight: Node with key was not found.
                                              NodeOrParent points to what would be
                                              parent.  The node would be the right
                                              child.

--*/



{

    if (RtlIsGenericTableEmpty(Table)) {

        return TableEmptyTree;

    } else {

        //
        // Used as the iteration variable while stepping through
        // the generic table.
        //
        PRTL_SPLAY_LINKS NodeToExamine = Table->TableRoot;

        //
        // Just a temporary.  Hopefully a good compiler will get
        // rid of it.
        //
        PRTL_SPLAY_LINKS Child;

        //
        // Holds the value of the comparasion.
        //
        RTL_GENERIC_COMPARE_RESULTS Result;

        while (TRUE) {

            //
            // Compare the buffer with the key in the tree element.
            //

            Result = Table->CompareRoutine(
                         Table,
                         Buffer,
                         &((PTABLE_ENTRY_HEADER) NodeToExamine)->UserData
                         );

            if (Result == GenericLessThan) {

                if (Child = RtlLeftChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsLeft;

                }

            } else if (Result == GenericGreaterThan) {

                if (Child = RtlRightChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return TableInsertAsRight;

                }


            } else {

                //
                // Node is in the tree (or it better be because of the
                // assert).  Set the output parameter to point to
                // the node and tell the caller that we found the node.
                //

                ASSERT(Result == GenericEqual);
                *NodeOrParent = NodeToExamine;
                return TableFoundNode;

            }

        }

    }

}

VOID
RtlInitializeGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    IN PVOID TableContext
    )

/*++

Routine Description:

    The procedure InitializeGenericTable takes as input an uninitialized
    generic table variable and pointers to the three user supplied routines.
    This must be called for every individual generic table variable before
    it can be used.

Arguments:

    Table - Pointer to the generic table to be initialized.

    CompareRoutine - User routine to be used to compare to keys in the
                     table.

    AllocateRoutine - User routine to call to allocate memory for a new
                      node in the generic table.

    FreeRoutine - User routine to call to deallocate memory for
                        a node in the generic table.

    TableContext - Supplies user supplied context for the table.

Return Value:

    None.

--*/

{

    //
    // Initialize each field of the Table parameter.
    //

    Table->TableRoot = NULL;
    InitializeListHead(&Table->InsertOrderList);
    Table->NumberGenericTableElements = 0;
    Table->OrderedPointer = &Table->InsertOrderList;
    Table->WhichOrderedElement = 0;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;

}


PVOID
RtlInsertElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL
    )

/*++

Routine Description:

    The function InsertElementGenericTable will insert a new element
    in a table.  It does this by allocating space for the new element
    (this includes splay links), inserting the element in the table, and
    then returning to the user a pointer to the new element (which is
    the first available space after the splay links).  If an element
    with the same key already exists in the table the return value is a pointer
    to the old element.  The optional output parameter NewElement is used
    to indicate if the element previously existed in the table.  Note: the user
    supplied Buffer is only used for searching the table, upon insertion its
    contents are copied to the newly created element.  This means that
    pointer to the input buffer will not point to the new element.

Arguments:

    Table - Pointer to the table in which to (possibly) insert the
            key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

    BufferSize - The amount of space to allocate when the (possible)
                 insertion is made.  Note that if we actually do
                 not find the node and we do allocate space then we
                 will add the size of the SPLAY_LINKS to this buffer
                 size.  The user should really take care not to depend
                 on anything in the first sizeof(SPLAY_LINKS) bytes
                 of the memory allocated via the memory allocation
                 routine.

    NewElement - Optional Flag.  If present then it will be set to
                 TRUE if the buffer was not "found" in the generic
                 table.

Return Value:

    PVOID - Pointer to the user defined data.

--*/

{

    //
    // Holds a pointer to the node in the table or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the table lookup.
    //
    TABLE_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    //
    //  Call the full routine to do the real work.
    //

    return RtlInsertElementGenericTableFull(
                Table,
                Buffer,
                BufferSize,
                NewElement,
                NodeOrParent,
                Lookup
                );
}


PVOID
RtlInsertElementGenericTableFull (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer,
    IN CLONG BufferSize,
    OUT PBOOLEAN NewElement OPTIONAL,
    PVOID NodeOrParent,
    TABLE_SEARCH_RESULT SearchResult
    )

/*++

Routine Description:

    The function InsertElementGenericTableFull will insert a new element
    in a table.  It does this by allocating space for the new element
    (this includes splay links), inserting the element in the table, and
    then returning to the user a pointer to the new element.  If an element
    with the same key already exists in the table the return value is a pointer
    to the old element.  The optional output parameter NewElement is used
    to indicate if the element previously existed in the table.  Note: the user
    supplied Buffer is only used for searching the table, upon insertion its
    contents are copied to the newly created element.  This means that
    pointer to the input buffer will not point to the new element.
    This routine is passed the NodeOrParent and SearchResult from a
    previous RtlLookupElementGenericTableFull.

Arguments:

    Table - Pointer to the table in which to (possibly) insert the
            key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

    BufferSize - The amount of space to allocate when the (possible)
                 insertion is made.  Note that if we actually do
                 not find the node and we do allocate space then we
                 will add the size of the SPLAY_LINKS to this buffer
                 size.  The user should really take care not to depend
                 on anything in the first sizeof(SPLAY_LINKS) bytes
                 of the memory allocated via the memory allocation
                 routine.

    NewElement - Optional Flag.  If present then it will be set to
                 TRUE if the buffer was not "found" in the generic
                 table.

   NodeOrParent - Result of prior RtlLookupElementGenericTableFull.

   SearchResult - Result of prior RtlLookupElementGenericTableFull.

Return Value:

    PVOID - Pointer to the user defined data.

--*/

{
    //
    // Node will point to the splay links of what
    // will be returned to the user.
    //

    PRTL_SPLAY_LINKS NodeToReturn;

    if (SearchResult != TableFoundNode) {

        //
        // We just check that the table isn't getting
        // too big.
        //

        ASSERT(Table->NumberGenericTableElements != (MAXULONG-1));

        //
        // The node wasn't in the (possibly empty) tree.
        // Call the user allocation routine to get space
        // for the new node.
        //

        NodeToReturn = Table->AllocateRoutine(
                           Table,
                           BufferSize+FIELD_OFFSET( TABLE_ENTRY_HEADER, UserData )
                           );

        //
        // If the return is NULL, return NULL from here to indicate that
        // the entry could not be added.
        //

        if (NodeToReturn == NULL) {

            if (ARGUMENT_PRESENT(NewElement)) {

                *NewElement = FALSE;
            }

            return(NULL);
        }

        RtlInitializeSplayLinks(NodeToReturn);

        //
        // Insert the new node at the end of the ordered linked list.
        //

        InsertTailList(
            &Table->InsertOrderList,
            &((PTABLE_ENTRY_HEADER) NodeToReturn)->ListEntry
            );

        Table->NumberGenericTableElements++;

        //
        // Insert the new node in the tree.
        //

        if (SearchResult == TableEmptyTree) {

            Table->TableRoot = NodeToReturn;

        } else {

            if (SearchResult == TableInsertAsLeft) {

                RtlInsertAsLeftChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            } else {

                RtlInsertAsRightChild(
                    NodeOrParent,
                    NodeToReturn
                    );
            }
        }

        //
        // Copy the users buffer into the user data area of the table.
        //

        RtlCopyMemory(
            &((PTABLE_ENTRY_HEADER) NodeToReturn)->UserData,
            Buffer,
            BufferSize
            );

    } else {

        NodeToReturn = NodeOrParent;
    }

    //
    // Always splay the (possibly) new node.
    //

    Table->TableRoot = RtlSplay(NodeToReturn);

    if (ARGUMENT_PRESENT(NewElement)) {

        *NewElement = ((SearchResult == TableFoundNode)?(FALSE):(TRUE));
    }

    //
    // Insert the element on the ordered list;
    //

    return &((PTABLE_ENTRY_HEADER) NodeToReturn)->UserData;
}


BOOLEAN
RtlDeleteElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    The function DeleteElementGenericTable will find and delete an element
    from a generic table.  If the element is located and deleted the return
    value is TRUE, otherwise if the element is not located the return value
    is FALSE.  The user supplied input buffer is only used as a key in
    locating the element in the table.

Arguments:

    Table - Pointer to the table in which to (possibly) delete the
            memory accessed by the key buffer.

    Buffer - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

Return Value:

    BOOLEAN - If the table contained the key then true, otherwise false.

--*/

{

    //
    // Holds a pointer to the node in the table or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the table lookup.
    //
    TABLE_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Table,
                 Buffer,
                 &NodeOrParent
                 );

    if ((Lookup == TableEmptyTree) || (Lookup != TableFoundNode)) {

        return FALSE;

    } else {

        //
        // Delete the node from the splay tree.
        //

        Table->TableRoot = RtlDelete(NodeOrParent);

        //
        // Delete the element from the linked list.
        //

        RemoveEntryList(&((PTABLE_ENTRY_HEADER) NodeOrParent)->ListEntry);
        Table->NumberGenericTableElements--;
        Table->WhichOrderedElement = 0;
        Table->OrderedPointer = &Table->InsertOrderList;

        //
        // The node has been deleted from the splay table.
        // Now give the node to the user deletion routine.
        // NOTE: We are giving the deletion routine a pointer
        // to the splay links rather then the user data.  It
        // is assumed that the deallocation is rather stupid.
        //

        Table->FreeRoutine(Table,NodeOrParent);
        return TRUE;

    }

}


PVOID
RtlLookupElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    The function LookupElementGenericTable will find an element in a generic
    table.  If the element is located the return value is a pointer to
    the user defined structure associated with the element, otherwise if
    the element is not located the return value is NULL.  The user supplied
    input buffer is only used as a key in locating the element in the table.

Arguments:

    Table - Pointer to the users Generic table to search for the key.

    Buffer - Used for the comparasion.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{
    //
    // Holds a pointer to the node in the table or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the table lookup.
    //
    TABLE_SEARCH_RESULT Lookup;

    return RtlLookupElementGenericTableFull(
                Table,
                Buffer,
                &NodeOrParent,
                &Lookup
                );
}


PVOID
NTAPI
RtlLookupElementGenericTableFull (
    PRTL_GENERIC_TABLE Table,
    PVOID Buffer,
    OUT PVOID *NodeOrParent,
    OUT TABLE_SEARCH_RESULT *SearchResult
    )

/*++

Routine Description:

    The function LookupElementGenericTableFull will find an element in a generic
    table.  If the element is located the return value is a pointer to
    the user defined structure associated with the element.  If the element is not
    located then a pointer to the parent for the insert location is returned.  The
    user must look at the SearchResult value to determine which is being returned.
    The user can use the SearchResult and parent for a subsequent FullInsertElement
    call to optimize the insert.

Arguments:

    Table - Pointer to the users Generic table to search for the key.

    Buffer - Used for the comparasion.

    NodeOrParent - Address to store the desired Node or parent of the desired node.

    SearchResult - Describes the relationship of the NodeOrParent with the desired Node.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{

    //
    //  Lookup the element and save the result.
    //

    *SearchResult = FindNodeOrParent(
                        Table,
                        Buffer,
                        (PRTL_SPLAY_LINKS *)NodeOrParent
                        );

    if ((*SearchResult == TableEmptyTree) || (*SearchResult != TableFoundNode)) {

        return NULL;

    } else {

        //
        // Splay the tree with this node.
        //

        Table->TableRoot = RtlSplay(*NodeOrParent);

        //
        // Return a pointer to the user data.
        //

        return &((PTABLE_ENTRY_HEADER)*NodeOrParent)->UserData;
    }
}


PVOID
RtlEnumerateGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN BOOLEAN Restart
    )

/*++

Routine Description:

    The function EnumerateGenericTable will return to the caller one-by-one
    the elements of of a table.  The return value is a pointer to the user
    defined structure associated with the element.  The input parameter
    Restart indicates if the enumeration should start from the beginning
    or should return the next element.  If the are no more new elements to
    return the return value is NULL.  As an example of its use, to enumerate
    all of the elements in a table the user would write:

        for (ptr = EnumerateGenericTable(Table,TRUE);
             ptr != NULL;
             ptr = EnumerateGenericTable(Table, FALSE)) {
                :
        }

Arguments:

    Table - Pointer to the generic table to enumerate.

    Restart - Flag that if true we should start with the least
              element in the tree otherwise, return we return
              a pointer to the user data for the root and make
              the real successor to the root the new root.

Return Value:

    PVOID - Pointer to the user data.

--*/

{

    if (RtlIsGenericTableEmpty(Table)) {

        //
        // Nothing to do if the table is empty.
        //

        return NULL;

    } else {

        //
        // Will be used as the "iteration" through the tree.
        //
        PRTL_SPLAY_LINKS NodeToReturn;

        //
        // If the restart flag is true then go to the least element
        // in the tree.
        //

        if (Restart) {

            //
            // We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = Table->TableRoot;
                RtlLeftChild(NodeToReturn);
                NodeToReturn = RtlLeftChild(NodeToReturn)
                ) {
                ;
            }

            Table->TableRoot = RtlSplay(NodeToReturn);

        } else {

            //
            // The assumption here is that the root of the
            // tree is the last node that we returned.  We
            // find the real successor to the root and return
            // it as next element of the enumeration.  The
            // node that is to be returned is splayed (thereby
            // making it the root of the tree).  Note that we
            // need to take care when there are no more elements.
            //

            NodeToReturn = RtlRealSuccessor(Table->TableRoot);

            if (NodeToReturn) {

                Table->TableRoot = RtlSplay(NodeToReturn);

            }

        }

        //
        // If there actually is a next element in the enumeration
        // then the pointer to return is right after the list links.
        //

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));

    }

}


BOOLEAN
RtlIsGenericTableEmpty (
    IN PRTL_GENERIC_TABLE Table
    )

/*++

Routine Description:

    The function IsGenericTableEmpty will return to the caller TRUE if
    the input table is empty (i.e., does not contain any elements) and
    FALSE otherwise.

Arguments:

    Table - Supplies a pointer to the Generic Table.

Return Value:

    BOOLEAN - if enabled the tree is empty.

--*/

{

    //
    // Table is empty if the root pointer is null.
    //

    return ((Table->TableRoot)?(FALSE):(TRUE));

}

PVOID
RtlGetElementGenericTable (
    IN PRTL_GENERIC_TABLE Table,
    IN ULONG I
    )

/*++

Routine Description:


    The function GetElementGenericTable will return the i'th element
    inserted in the generic table.  I = 0 implies the first element,
    I = (RtlNumberGenericTableElements(Table)-1) will return the last element
    inserted into the generic table.  The type of I is ULONG.  Values
    of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
    an arbitrary element is deleted from the generic table it will cause
    all elements inserted after the deleted element to "move up".

Arguments:

    Table - Pointer to the generic table from which to get the ith element.

    I - Which element to get.


Return Value:

    PVOID - Pointer to the user data.

--*/

{

    //
    // Current location in the table.
    //
    ULONG CurrentLocation = Table->WhichOrderedElement;

    //
    // Hold the number of elements in the table.
    //
    ULONG NumberInTable = Table->NumberGenericTableElements;

    //
    // Holds the value of I+1.
    //
    // Note that we don't care if this value overflows.
    // If we end up accessing it we know that it didn't.
    //
    ULONG NormalizedI = I + 1;

    //
    // Will hold distances to travel to the desired node;
    //
    ULONG ForwardDistance,BackwardDistance;

    //
    // Will point to the current element in the linked list.
    //
    PLIST_ENTRY CurrentNode = Table->OrderedPointer;


    //
    // If it's out of bounds get out quick.
    //

    if ((I == MAXULONG) || (NormalizedI > NumberInTable)) return NULL;

    //
    // If we're already at the node then return it.
    //

    if (NormalizedI == CurrentLocation) {

        return &((PTABLE_ENTRY_HEADER) CONTAINING_RECORD(CurrentNode, TABLE_ENTRY_HEADER, ListEntry))->UserData;
    }

    //
    // Calculate the forward and backward distance to the node.
    //

    if (CurrentLocation > NormalizedI) {

        //
        // When CurrentLocation is greater than where we want to go,
        // if moving forward gets us there quicker than moving backward
        // then it follows that moving forward from the listhead is
        // going to take fewer steps. (This is because, moving forward
        // in this case must move *through* the listhead.)
        //
        // The work here is to figure out if moving backward would be quicker.
        //
        // Moving backward would be quicker only if the location we wish  to
        // go to is more than half way between the listhead and where we
        // currently are.
        //

        if (NormalizedI > (CurrentLocation/2)) {

            //
            // Where we want to go is more than half way from the listhead
            // We can traval backwards from our current location.
            //

            for (
                BackwardDistance = CurrentLocation - NormalizedI;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = CurrentNode->Blink;

            }
        } else {

            //
            // Where we want to go is less than halfway between the start
            // and where we currently are.  Start from the listhead.
            //

            for (
                CurrentNode = &Table->InsertOrderList;
                NormalizedI;
                NormalizedI--
                ) {

                CurrentNode = CurrentNode->Flink;

            }

        }

    } else {


        //
        // When CurrentLocation is less than where we want to go,
        // if moving backwards gets us there quicker than moving forwards
        // then it follows that moving backwards from the listhead is
        // going to take fewer steps. (This is because, moving backwards
        // in this case must move *through* the listhead.)
        //

        ForwardDistance = NormalizedI - CurrentLocation;

        //
        // Do the backwards calculation as if we are starting from the
        // listhead.
        //

        BackwardDistance = (NumberInTable - NormalizedI) + 1;

        if (ForwardDistance <= BackwardDistance) {

            for (
                ;
                ForwardDistance;
                ForwardDistance--
                ) {

                CurrentNode = CurrentNode->Flink;

            }


        } else {

            for (
                CurrentNode = &Table->InsertOrderList;
                BackwardDistance;
                BackwardDistance--
                ) {

                CurrentNode = CurrentNode->Blink;

            }

        }

    }

    //
    // We're where we want to be.  Save our current location and return
    // a pointer to the data to the user.
    //

    Table->OrderedPointer = CurrentNode;
    Table->WhichOrderedElement = I+1;

    return &((PTABLE_ENTRY_HEADER) CONTAINING_RECORD(CurrentNode, TABLE_ENTRY_HEADER, ListEntry))->UserData;

}


ULONG
RtlNumberGenericTableElements(
    IN PRTL_GENERIC_TABLE Table
    )

/*++

Routine Description:

    The function NumberGenericTableElements returns a ULONG value
    which is the number of generic table elements currently inserted
    in the generic table.

Arguments:

    Table - Pointer to the generic table from which to find out the number
    of elements.


Return Value:

    ULONG - The number of elements in the generic table.

--*/
{

    return Table->NumberGenericTableElements;

}


PVOID
RtlEnumerateGenericTableWithoutSplaying (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID *RestartKey
    )

/*++

Routine Description:

    The function EnumerateGenericTableWithoutSplaying will return to the
    caller one-by-one the elements of of a table.  The return value is a
    pointer to the user defined structure associated with the element.
    The input parameter RestartKey indicates if the enumeration should
    start from the beginning or should return the next element.  If the
    are no more new elements to return the return value is NULL.  As an
    example of its use, to enumerate all of the elements in a table the
    user would write:

        *RestartKey = NULL;

        for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
                :
        }

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the search
                will be started from the beginning.

Return Value:

    PVOID - Pointer to the user data.

--*/

{

    if (RtlIsGenericTableEmpty(Table)) {

        //
        // Nothing to do if the table is empty.
        //

        return NULL;

    } else {

        //
        // Will be used as the "iteration" through the tree.
        //
        PRTL_SPLAY_LINKS NodeToReturn;

        //
        // If the restart flag is true then go to the least element
        // in the tree.
        //

        if (*RestartKey == NULL) {

            //
            // We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = Table->TableRoot;
                RtlLeftChild(NodeToReturn);
                NodeToReturn = RtlLeftChild(NodeToReturn)
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            // The caller has passed in the previous entry found
            // in the table to enable us to continue the search.  We call
            // RtlRealSuccessor to step to the next element in the tree.
            //

            NodeToReturn = RtlRealSuccessor(*RestartKey);

            if (NodeToReturn) {

                *RestartKey = NodeToReturn;

            }

        }

        //
        // If there actually is a next element in the enumeration
        // then the pointer to return is right after the list links.
        //

        return ((NodeToReturn)?
                   ((PVOID)&((PTABLE_ENTRY_HEADER)NodeToReturn)->UserData)
                  :((PVOID)(NULL)));

    }

}


