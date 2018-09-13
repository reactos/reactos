/*++



Copyright (c) 1991  Microsoft Corporation

Module Name:

    RegState.c

Abstract:

    This module contains helper functions for maintaining
    user mode state for registry objects

Author:

    Adam Edwards (adamed) 06-May-1998

Key Functions:

StateObjectInit

StateObjectListInit
StateObjectListIsEmpty
StateObjectListRemove
StateObjectListFind
StateObjectListAdd
StateObjectListClear

Notes:

    The StateObjectList stores the most frequently accessed
    objects at the head of the list for quick retrieval.  All
    objects in the list must inherit from StateObject, and must
    be distinguishable by a unique 32-bit key.  Duplicate
    objects are not supported, so the client must take care
    not to store duplicates (two objects with the same key) in
    the list.

--*/



#ifdef LOCAL

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regstate.h"
#include <malloc.h>


VOID StateObjectInit(
    StateObject* pObject,
    PVOID        pvKey)
/*++
Routine Description:

    This function will initialize a StateObject.  All objects
    which inherit from this must call this function before doing
    any custom initialization.

Arguments:

    pObject -- the object to initialize.

    pvKey -- a unique key for the object used for searches and comparisons
        between objects.

Return Value:

    None -- the function does nothing which could fail.

--*/
{
    RtlZeroMemory(pObject, sizeof(*pObject));

    pObject->pvKey = pvKey;
}


VOID StateObjectListInit(
    StateObjectList* pList,
    PVOID            pvKey)
/*++
Routine Description:

    This function will initialize a StateObjectList.  All objects
    which inherit from this must call this function before doing
    any custom initialization.

Arguments:

    pList -- pointer to list to initialize.

    pvKey -- a unique key that identifies the list.  This parameter is
        necessary since the list itself is a StateObject, which requires a 
        key.  It can be set to 0 if this list will not be searched for
        as part of another list, but should be set otherwise to a unique
        value.

Return Value:

    None -- the function does nothing which could fail.

--*/
{
    StateObjectInit(
        (StateObject*) pList,
        pvKey);

    pList->pHead = NULL;
}


BOOL StateObjectListIsEmpty(StateObjectList* pList)
/*++
Routine Description:

    This function returns information on whether or not this
    list is empty.

Arguments:

    pList -- pointer to list in question

Return Value:

    TRUE if the list is empty,
    FALSE if not.

--*/
{
    return NULL == pList->pHead;
}


StateObject* StateObjectListRemove(
    StateObjectList* pList,
    PVOID            pvKey)
/*++
Routine Description:

    This function will remove an object from the list --
    it does *not* destroy the object.

Arguments:

    pList -- the list to remove the object from

    pvKey -- a unique key identifying the object to remove

Return Value:

    a pointer to the removed object if successful,
    NULL otherwise

--*/
{
    StateObject* pObject;

    //
    // First, we need to find an object with the desired key
    //

    pObject = StateObjectListFind(
        pList,
        pvKey);

    if (!pObject) {
        return NULL;
    }

    //
    // Now that we've executed the find, the object is at the front
    // of the list -- we can remove it by setting the head to the
    // next object in the list
    //
    pList->pHead = (StateObject*) (pObject->Links.Flink);

    //
    // Make sure the new head's previous pointer is NULL since it
    // has no predecessor
    //
    if (pList->pHead) {
        pList->pHead->Links.Blink  = NULL;
    }

    return pObject;
}

StateObject* StateObjectListFind(
    StateObjectList* pList,
    PVOID            pvKey)
/*++
Routine Description:

    This function will find an object in the list

Arguments:

    pList -- the list in which to search

    pvKey -- a unique key identifying the object sought

Return Value:

    a pointer to the object if an object with a key matching pvKey is found,
    NULL otherwise

--*/
{
    StateObject* pCurrent;

    //
    // Loop through all objects in the list until we get to the end
    //
    for (pCurrent = pList->pHead;
         pCurrent != NULL;
         pCurrent = (StateObject*) pCurrent->Links.Flink)
    {
        //
        // See if this object's key matches the desired key
        //
        if (pvKey == pCurrent->pvKey) {
            
            PLIST_ENTRY pFlink;
            PLIST_ENTRY pBlink;

            //
            // If the desired object is at the front, this is a no op --
            // we don't have to move anything, just return the object
            //
            if (pCurrent == pList->pHead) {
                return pCurrent;
            }
    
            //
            // We need to move the found object to the front of the list
            //

            //
            // Remove the object from its current position
            // by severing its links
            //
            pBlink = pCurrent->Links.Blink;
            pFlink = pCurrent->Links.Flink;

            if (pBlink) {
                pBlink->Flink = pFlink;
            }

            if (pFlink) {
                pFlink->Blink = pBlink;
            }

            //
            // Re-add it to the front
            //
            StateObjectListAdd(
                pList,
                pCurrent);

            return pCurrent;
        }
    }

    //
    // We never found an object with the desired key above, so its
    // not in the list
    //
    return NULL;
}


VOID StateObjectListAdd(
    StateObjectList* pList,
    StateObject*     pObject)
/*++
Routine Description:

    This function will add an object to the list

Arguments:

    pList -- the list in which to add the object

    pObject -- pointer to an object which has been initialized
        with StateObjectInit; this object will be stored in the list.

Return Value:

    None -- this function does nothing which could fail.

Note:

    Only one object with a particular key should exist in the list.  This
    requirement is not enforced by this function or the list itself, so
    clients need to ensure that they follow this rule.

--*/
{
    //
    // Create the links between the object and what's currently 
    // at the front of the list
    //
    if (pList->pHead) {
        pObject->Links.Flink = (PLIST_ENTRY) pList->pHead;
        pList->pHead->Links.Blink = (PLIST_ENTRY) pObject;
    }

    //
    // Put the object at the front of the list
    //
    pList->pHead = pObject;
    pList->pHead->Links.Blink = NULL;
}


VOID StateObjectListClear(
    StateObjectList* pList,
    PFNDestroyObject pfnDestroy)
/*++
Routine Description:

    This function will remove and destroy all objects
        in the list.

Arguments:

    pList -- the list to clear

    pfnDestroy -- pointer to a function which will be called for
        each object in order to destroy (free resources such as memory,
        kernel objects, etc) it.

Return Value:

    None -- this function does nothing which could fail.

--*/
{
    StateObject* pCurrent;

    //
    // Keep removing objects until the list is empty
    //
    while (!StateObjectListIsEmpty(pList))
    {
        StateObject* pObject;
        
        //
        // Remove whatever's at the front of the list
        //
        pObject = StateObjectListRemove(
            pList,
            pList->pHead->pvKey);

        ASSERT(pObject);

        //
        // Destroy the removed object
        //
        pfnDestroy(pObject);
    }
}


#endif // LOCAL



