/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regstate.h

Abstract:

    This file contains declarations for data structures
    needed for maintaining state for registry key objects

Author:

    Adam Edwards (adamed) 14-Nov-1997

Notes:

    This file contains declarations for an object which
    can be stored in a collection.  The collection is a linked
    list, whose most frequently accessed members are at the
    front of the list.  Thus it is optimized for situations
    in which the same object may be asked for repeatedly.

    To use this list, you should create structures which
    inherit from the base object (StateObject).  Each object
    has a key which must be specified when you create a new
    object.  The key is used in searches and removes.  The objects
    must all have disinct keys, but this is not enforced by the list --
    the caller should be sure to avoid duplicates.

    Note that the list does not allocate or free objects --
    it simply allows them to be added, removed, and searched
    for in the list.  You will need to initialize any object
    before putting it into the list with the StateObjectInit
    function.

--*/

#ifdef LOCAL

#if !defined(_REGSTATE_H_)
#define _REGSTATE_H_


//
// Data types
//

//
// StateObject
//
// This is the base type -- any object used with the list must
// inherit from this object
//
typedef struct _StateObject
{
    LIST_ENTRY Links;
    PVOID      pvKey;
} StateObject;

//
// Pointer type for a caller supplied function used to
// destroy objects (de-allocate memory, free resources, etc)
//
typedef VOID (*PFNDestroyObject) (StateObject* pObject);


//
// StateObjectList
//
// This is a linked list of StateObjects, with the most frequently accessed
// elements at the front of the list.  If the same item is
// accessed repeatedly, it is found immediately in search
// operations.  Note that the list is itself a StateObject, so
// the list type can be composed with itself.
//
typedef struct _StateObjectList 
{
    StateObject  Object;
    StateObject* pHead;
} StateObjectList;


//
// Exported prototypes
//

//
// Initializes a StateObject -- must be called before
// the object is used
//
VOID StateObjectInit(
    StateObject* pObject,
    PVOID        pvKey);


//
// Initializes a StateObjectList -- must be called before
// the list is used
//
VOID StateObjectListInit(
    StateObjectList* pList,
    PVOID            pvKey);

//
// Tells whether or not a list is empty
//
BOOL StateObjectListIsEmpty(StateObjectList* pList);

//
// Removes an object from the list
//
StateObject*  StateObjectListRemove(
    StateObjectList* pList,
    PVOID            pvKey);

//
// Finds an object in the list
//
StateObject* StateObjectListFind(
    StateObjectList* pList,
    PVOID            pvKey);

//
// Adds an object to the list
//
VOID StateObjectListAdd(
    StateObjectList* pList,
    StateObject*     pObject);

//
// Removes all objects from the list and 
// destroys them.
//
VOID StateObjectListClear(
    StateObjectList* pList,
    PFNDestroyObject pfnDestroy);
                          

#endif // _REGSTATE_H_

#endif // LOCAL



