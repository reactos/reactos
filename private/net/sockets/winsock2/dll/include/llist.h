/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    llist.h

Abstract:

    This  module  is  a  standalone  collection  of  linked-list definition and
    manipulation  macros  originally  defined  within  the  internal Windows NT
    development.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 07-08-1995

Notes:

    $Revision:   1.3  $

    $Modtime:   12 Jan 1996 15:09:00  $

    As  of 07-08-95, this header file contains usage comments.  If desired, the
    usage  comments  can  be  removed.   However,  since  this file is only for
    internal  use, there is no strong reason to remove the comments.  Since the
    comments  make  it easier to understand and use these macros they should be
    preserved as far as possible.

Revision History:

    most-recent-revision-date email-name
        description

    07-09-1995  drewsxpa@ashland.intel.com
        Completed  first  complete  version with clean compile and released for
        subsequent implementation.

    07-08-95  drewsxpa@ashland.intel.com
        Adapted   original  compilable  version  from  proposal  received  from
        keithmo@microsoft.com
--*/

#ifndef _LLIST_
#define _LLIST_

#include <windows.h>


#if !defined( _WINNT_ )
//
//  From NTDEF.H.
//

//  Doubly  linked  list  structure.   Can be used as either a list head, or as
//  link storage within a linked-list item.

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;




// LONG
// FIELD_OFFSET(
//     IN <typename>   type,
//     IN <fieldname>  field
//     );
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
/*++

Routine Description:

    Calculates  the  byte offset of a field in a structure of type "type".  The
    offset is from the beginning of the containing structure.

    Note  that  since  this macro uses compile-time type knowledge, there is no
    equivalent C procedure for this macro.

Arguments:

    type  - Supplies the type name of the containing structure.

    field - Supplies  the  field  name  of  the  field  whose  offset  is to be
            computed.

Return Value:

    Returns  the byte offset of the named field within a structure of the named
    type.
--*/




// <typename> FAR *
// CONTAINING_RECORD(
//     IN PVOID       address,
//     IN <typename>  type,
//     IN <fieldname> field
//     );
#define CONTAINING_RECORD(address, type, field) ((type FAR *)( \
                                          (PCHAR)(address) - \
                                          (PCHAR)(&((type *)0)->field)))
/*++

Routine Description:

    Retrieves  a  typed  pointer to a linked list item given the address of the
    link  storage  structure  embedded in the linked list item, the type of the
    linked  list  item,  and  the  field  name  of  the  embedded  link storage
    structure.

    Note  that  since  this macro uses compile-time type knowledge, there is no
    equivalent C procedure for this macro.

Arguments:

    address - Supplies  the  address of a LIST_ENTRY structure embedded in an a
              linked list item.

    type    - Supplies  the  type  name  of  the  containing  linked  list item
              structure.

    field   - Supplies  the  field  name  if  the LIST_ENTRY structure embedded
              within the linked list item structure.

Return Value:

    Returns a pointer to the linked list item.
--*/


#endif  // !defined( _WINNT_ )


//
//  From NTRTL.H.
//

//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.




// VOID
// InitializeListHead(
//     IN PLIST_ENTRY ListHead
//     );
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))
/*++

Routine Description:

    Initializes  a  PLIST_ENTRY  structure to be the head of an initially empty
    linked list.

Arguments:

    ListHead - Supplies a reference to the structure to be initialized.

Return Value:

    None
--*/




// BOOLEAN
// IsListEmpty(
//     IN PLIST_ENTRY ListHead
//     );
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
/*++

Routine Description:

    Determines whether or not a list is empty.

Arguments:

    ListHead - Supplies  a  reference  to  the  head  of  the linked list to be
               examined.

Return Value:

    TRUE  - The linked list is empty.

    FALSE - The linked list contains at least one item.
--*/




// PLIST_ENTRY
// RemoveHeadList(
//     IN PLIST_ENTRY ListHead
//     );
#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}
/*++

Routine Description:

    Removes  the  "head" (first) item from a linked list, returning the pointer
    to  the  removed  entry's embedded linkage structure.  Attempting to remove
    the  head  item  from  a  (properly  initialized)  linked  list is a no-op,
    returning the pointer to the head of the linked list.

    The  caller  may  use  the  CONTAINING_RECORD macro to amplify the returned
    linkage structure pointer to the containing linked list item structure.

Arguments:

    ListHead - Supplies  a  reference  to  the  head  of  the linked list to be
               operated upon.

Return Value:

    Returns  a pointer to the newly removed linked list item's embedded linkage
    structure, or the linked list head in the case of an empty list.
--*/




// PLIST_ENTRY
// RemoveTailList(
//     IN PLIST_ENTRY ListHead
//     );
#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}
/*++

Routine Description:

    Removes the "tail" (last) item from a linked list, returning the pointer to
    the  removed  entry's embedded linkage structure.  Attempting to remove the
    tail  item  from a (properly initialized) linked list is a no-op, returning
    the pointer to the head of the linked list.

    The  caller  may  use  the  CONTAINING_RECORD macro to amplify the returned
    linkage structure pointer to the containing linked list item structure.

Arguments:

    ListHead - Supplies  a  reference  to  the  head  of  the linked list to be
               operated upon.

Return Value:

    Returns  a pointer to the newly removed linked list item's embedded linkage
    structure, or the linked list head in the case of an empty list.
--*/




// VOID
// RemoveEntryList(
//     IN PLIST_ENTRY Entry
//     );
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }
/*++

Routine Description:

    Removes  an  item  from a linked list.  Attempting to remove the head of an
    empty list is a no-op.

Arguments:

    Entry - Supplies  a reference to the linkage structure embedded in a linked
            list item structure.

Return Value:

    None
--*/




// VOID
// InsertTailList(
//     IN PLIST_ENTRY ListHead,
//     IN PLIST_ENTRY Entry
//     );
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }
/*++

Routine Description:

    Inserts a new item as the "tail" (last) item of a linked list.

Arguments:

    ListHead - Supplies  a  reference  to  the  head  of  the linked list to be
               operated upon.

    Entry    - Supplies  a  reference  to the linkage structure embedded in the
               linked list item to be added to the linked list.

Return Value:

    None
--*/




// VOID
// InsertHeadList(
//     IN PLIST_ENTRY ListHead,
//     IN PLIST_ENTRY Entry
//     );
#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }
/*++

Routine Description:

    Inserts a new item as the "head" (first) item of a linked list.

Arguments:

    ListHead - Supplies  a  reference  to  the  head  of  the linked list to be
               operated upon.

    Entry    - Supplies  a  reference  to the linkage structure embedded in the
               linked list item to be added to the linked list.

Return Value:

    None
--*/




// Samples:
//
// //
// //  Define a list head.
// //
//
// LIST_ENTRY FooList;
//
// //
// //  Define a structure that will be on the list.
// //
// //  NOTE:  For debugging purposes, it usually makes life simpler to make the
// //  LIST_ENTRY  the  first  field  of  the  structure,  but  this  is  not a
// //  requirement.
// //
//
// typedef struct _FOO
// {
//     LIST_ENTRY FooListEntry;
//     .
//     .
//     .
//
// } FOO, * PFOO;
//
// //
// //  Initialize an empty list.
// //
//
// InitializeListHead( &FooList );
//
// //
// //  Create an object, append it to the end of the list.
// //
//
// PFOO foo;
//
// foo = ALLOC( sizeof(FOO) );
// {check for errors, initialize FOO structure}
//
// InsertTailList( &FooList, &foo->FooListEntry );
//
// //
// //  Scan list and delete selected items.
// //
//
// PFOO foo;
// PLIST_ENTRY listEntry;
//
// listEntry = FooList.Flink;
//
// while( listEntry != &FooList )
// {
//     foo = CONTAINING_RECORD( listEntry,
//                              FOO,
//                              FooListEntry );
//     listEntry = listEntry->Flink;
//
//     if( SomeFunction( foo ) )
//     {
//         RemoveEntryList( &foo->FooListEntry );
//      FREE( foo );
//     }
// }
//
// //
// //  Purge all items from a list.
// //
//
// PFOO foo;
// PLIST_ENTRY listEntry;
//
// while( !IsListEmpty( &FooList ) )
// {
//     listEntry = RemoveHeadList( &FooList );
//     foo = CONTAINING_RECORD( listEntry,
//                              FOO,
//                              FooListEntry );
//
//     FREE( foo );
// }


#endif  // _LLIST_
