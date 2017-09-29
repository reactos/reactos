/*
 * PROJECT:     Skiplist implementation for the ReactOS Project
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     All implemented functions operating on the Skiplist
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <intrin.h>
#include <windef.h>
#include <winbase.h>
#include "skiplist.h"

/**
 * @name _GetRandomLevel
 *
 * Returns a random level for the next element to be inserted.
 * This level is geometrically distributed for p = 0.5, so perfectly suitable for an efficient Skiplist implementation.
 *
 * @return
 * A value between 0 and SKIPLIST_LEVELS - 1.
 */
static __inline CHAR
_GetRandomLevel()
{
    // Using a simple fixed seed and the Park-Miller Lehmer Minimal Standard Random Number Generator gives an acceptable distribution for our "random" levels.
    static DWORD dwRandom = 1;

    DWORD dwLevel = 0;
    DWORD dwShifted;

    // Generate 31 uniformly distributed pseudo-random bits using the Park-Miller Lehmer Minimal Standard Random Number Generator.
    dwRandom = (DWORD)(((ULONGLONG)dwRandom * 48271UL) % 2147483647UL);

    // Shift out (31 - SKIPLIST_LEVELS) bits to the right to have no more than SKIPLIST_LEVELS bits set.
    dwShifted = dwRandom >> (31 - SKIPLIST_LEVELS);

    // BitScanForward doesn't operate on a zero input value.
    if (dwShifted)
    {
        // BitScanForward sets dwLevel to the zero-based position of the first set bit (from LSB to MSB).
        // This makes dwLevel a geometrically distributed value between 0 and SKIPLIST_LEVELS - 1 for p = 0.5.
        BitScanForward(&dwLevel, dwShifted);
    }

    // dwLevel can't have a value higher than 30 this way, so a CHAR is more than enough.
    return (CHAR)dwLevel;
}

/**
 * @name _InsertElementSkiplistWithInformation
 *
 * Determines a level for the new element and inserts it at the given position in the Skiplist.
 * This function is internally used by the Skiplist insertion functions.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param Element
 * The element to insert.
 *
 * @param pUpdate
 * Array containing the last nodes before our new node on each level.
 *
 * @param dwDistance
 * Array containing the distance to the last node before our new node on each level.
 *
 * @return
 * TRUE if the node was successfully inserted, FALSE if no memory could be allocated for it.
 */
static BOOL
_InsertElementSkiplistWithInformation(PSKIPLIST Skiplist, PVOID Element, PSKIPLIST_NODE* pUpdate, DWORD* dwDistance)
{
    CHAR chNewLevel;
    CHAR i;
    PSKIPLIST_NODE pNode;

    // Get the highest level, on which the node shall be inserted.
    chNewLevel = _GetRandomLevel();

    // Check if the new level is higher than the maximum level we currently have in the Skiplist.
    if (chNewLevel > Skiplist->MaximumLevel)
    {
        // It is, so we also need to insert the new node right after the Head node on some levels.
        // These are the levels higher than the current maximum level up to the new level.
        // We also need to set the distance of these elements to the new node count to account for the calculations below.
        for (i = Skiplist->MaximumLevel + 1; i <= chNewLevel; i++)
        {
            pUpdate[i] = &Skiplist->Head;
            pUpdate[i]->Distance[i] = Skiplist->NodeCount + 1;
        }

        // The new level is the new maximum level of the entire Skiplist.
        Skiplist->MaximumLevel = chNewLevel;
    }

    // Finally create our new Skiplist node.
    pNode = Skiplist->AllocateRoutine(sizeof(SKIPLIST_NODE));
    if (!pNode)
        return FALSE;

    pNode->Element = Element;

    // For each used level, insert us between the saved node for this level and its current next node.
    for (i = 0; i <= chNewLevel; i++)
    {
        pNode->Next[i] = pUpdate[i]->Next[i];
        pUpdate[i]->Next[i] = pNode;

        // We know the walked distance in this level: dwDistance[i]
        // We also know the element index of the new node: dwDistance[0]
        // The new node's distance is now the walked distance in this level plus the difference between the saved node's distance and the element index.
        pNode->Distance[i] = dwDistance[i] + (pUpdate[i]->Distance[i] - dwDistance[0]);

        // The saved node's distance is now the element index plus one (to account for the added node) minus the walked distance in this level.
        pUpdate[i]->Distance[i] = dwDistance[0] + 1 - dwDistance[i];
    }

    // For all levels above the new node's level, we need to increment the distance, because we've just added our new node.
    for (i = chNewLevel + 1; i <= Skiplist->MaximumLevel; i++)
        ++pUpdate[i]->Distance[i];

    // We've successfully added a node :)
    ++Skiplist->NodeCount;
    return TRUE;
}

/**
 * @name DeleteElementSkiplist
 *
 * Deletes an element from the Skiplist. The efficiency of this operation is O(log N) on average.
 *
 * Instead of the result of a LookupElementSkiplist call, it's sufficient to provide a dummy element with just enough information for your CompareRoutine.
 * A lookup for the element to be deleted needs to be performed in any case.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param Element
 * Information about the element to be deleted.
 *
 * @return
 * Returns the deleted element or NULL if no such element was found.
 * You can then free memory for the deleted element if necessary.
 */
PVOID
DeleteElementSkiplist(PSKIPLIST Skiplist, PVOID Element)
{
    CHAR i;
    PSKIPLIST_NODE pLastComparedNode = NULL;
    PSKIPLIST_NODE pNode = &Skiplist->Head;
    PSKIPLIST_NODE pUpdate[SKIPLIST_LEVELS];
    PVOID pReturnValue;

    // Find the node on every currently used level, after which the node to be deleted must follow.
    // This can be done efficiently by starting from the maximum level and going down a level each time a position has been found.
    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        while (pNode->Next[i] && pNode->Next[i] != pLastComparedNode && Skiplist->CompareRoutine(pNode->Next[i]->Element, Element) < 0)
            pNode = pNode->Next[i];

        // Reduce the number of comparisons by not comparing the same node on different levels twice.
        pLastComparedNode = pNode->Next[i];
        pUpdate[i] = pNode;
    }

    // Check if the node we're looking for has been found.
    pNode = pNode->Next[0];
    if (!pNode || Skiplist->CompareRoutine(pNode->Element, Element) != 0)
    {
        // It hasn't been found, so there's nothing to delete.
        return NULL;
    }

    // Beginning at the lowest level, remove the node from each level of the list and merge distances.
    // We can stop as soon as we found the first level that doesn't contain the node.
    for (i = 0; i <= Skiplist->MaximumLevel && pUpdate[i]->Next[i] == pNode; i++)
    {
        pUpdate[i]->Distance[i] += pNode->Distance[i] - 1;
        pUpdate[i]->Next[i] = pNode->Next[i];
    }

    // Now decrement the distance of the corresponding node in levels higher than the deleted node's level to account for the deleted node.
    while (i <= Skiplist->MaximumLevel)
    {
        --pUpdate[i]->Distance[i];
        i++;
    }

    // Return the deleted element (so the caller can free it if necessary) and free the memory for the node itself (allocated by us).
    pReturnValue = pNode->Element;
    Skiplist->FreeRoutine(pNode);

    // Find all levels which now contain no more nodes and reduce the maximum level of the entire Skiplist accordingly.
    while (Skiplist->MaximumLevel > 0 && !Skiplist->Head.Next[Skiplist->MaximumLevel])
        --Skiplist->MaximumLevel;

    // We've successfully deleted the node :)
    --Skiplist->NodeCount;
    return pReturnValue;
}

/**
 * @name InitializeSkiplist
 *
 * Initializes a new Skiplist structure.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param AllocateRoutine
 * Pointer to a SKIPLIST_ALLOCATE_ROUTINE for allocating memory for new Skiplist nodes.
 *
 * @param CompareRoutine
 * Pointer to a SKIPLIST_COMPARE_ROUTINE for comparing two elements of the Skiplist.
 *
 * @param FreeRoutine
 * Pointer to a SKIPLIST_FREE_ROUTINE for freeing memory allocated with AllocateRoutine.
 */
void
InitializeSkiplist(PSKIPLIST Skiplist, PSKIPLIST_ALLOCATE_ROUTINE AllocateRoutine, PSKIPLIST_COMPARE_ROUTINE CompareRoutine, PSKIPLIST_FREE_ROUTINE FreeRoutine)
{
    // Store the routines.
    Skiplist->AllocateRoutine = AllocateRoutine;
    Skiplist->CompareRoutine = CompareRoutine;
    Skiplist->FreeRoutine = FreeRoutine;

    // Initialize the members and pointers.
    // The Distance array is only used when a node is non-NULL, so it doesn't need initialization.
    Skiplist->MaximumLevel = 0;
    Skiplist->NodeCount = 0;
    ZeroMemory(Skiplist->Head.Next, sizeof(Skiplist->Head.Next));
}

/**
 * @name InsertElementSkiplist
 *
 * Inserts a new element into the Skiplist. The efficiency of this operation is O(log N) on average.
 * Uses CompareRoutine to find the right position for the insertion.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param Element
 * The element to insert.
 *
 * @return
 * TRUE if the node was successfully inserted, FALSE if it already exists or no memory could be allocated for it.
 */
BOOL
InsertElementSkiplist(PSKIPLIST Skiplist, PVOID Element)
{
    CHAR i;
    DWORD dwDistance[SKIPLIST_LEVELS + 1] = { 0 };
    PSKIPLIST_NODE pLastComparedNode = NULL;
    PSKIPLIST_NODE pNode = &Skiplist->Head;
    PSKIPLIST_NODE pUpdate[SKIPLIST_LEVELS];

    // Find the node on every currently used level, after which the new node needs to be inserted.
    // This can be done efficiently by starting from the maximum level and going down a level each time a position has been found.
    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        // When entering this level, we begin at the distance of the last level we walked through.
        dwDistance[i] = dwDistance[i + 1];

        while (pNode->Next[i] && pNode->Next[i] != pLastComparedNode && Skiplist->CompareRoutine(pNode->Next[i]->Element, Element) < 0)
        {
            // Save our position in every level when walking through the nodes.
            dwDistance[i] += pNode->Distance[i];

            // Advance to the next node.
            pNode = pNode->Next[i];
        }

        // Reduce the number of comparisons by not comparing the same node on different levels twice.
        pLastComparedNode = pNode->Next[i];
        pUpdate[i] = pNode;
    }

    // Check if the node already exists in the Skiplist.
    pNode = pNode->Next[0];
    if (pNode && Skiplist->CompareRoutine(pNode->Element, Element) == 0)
    {
        // All elements to be inserted mustn't exist in the list, so we see this as a failure.
        return FALSE;
    }

    // The rest of the procedure is the same for both insertion functions.
    return _InsertElementSkiplistWithInformation(Skiplist, Element, pUpdate, dwDistance);
}

/**
 * @name InsertTailElementSkiplist
 *
 * Inserts a new element at the end of the Skiplist. The efficiency of this operation is O(log N) on average.
 * In contrast to InsertElementSkiplist, this function is more efficient by not calling CompareRoutine at all and always inserting the element at the end.
 * You're responsible for calling this function only when you can guarantee that InsertElementSkiplist would also insert the element at the end.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param Element
 * The element to insert.
 *
 * @return
 * TRUE if the node was successfully inserted, FALSE if it already exists or no memory could be allocated for it.
 */
BOOL
InsertTailElementSkiplist(PSKIPLIST Skiplist, PVOID Element)
{
    CHAR i;
    DWORD dwDistance[SKIPLIST_LEVELS + 1] = { 0 };
    PSKIPLIST_NODE pNode = &Skiplist->Head;
    PSKIPLIST_NODE pUpdate[SKIPLIST_LEVELS];

    // Find the last node on every currently used level, after which the new node needs to be inserted.
    // This can be done efficiently by starting from the maximum level and going down a level each time a position has been found.
    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        // When entering this level, we begin at the distance of the last level we walked through.
        dwDistance[i] = dwDistance[i + 1];

        while (pNode->Next[i])
        {
            // Save our position in every level when walking through the nodes.
            dwDistance[i] += pNode->Distance[i];

            // Advance to the next node.
            pNode = pNode->Next[i];
        }

        pUpdate[i] = pNode;
    }

    // The rest of the procedure is the same for both insertion functions.
    return _InsertElementSkiplistWithInformation(Skiplist, Element, pUpdate, dwDistance);
}

/**
 * @name LookupElementSkiplist
 *
 * Looks up an element in the Skiplist. The efficiency of this operation is O(log N) on average.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param Element
 * Information about the element to look for.
 *
 * @param ElementIndex
 * Pointer to a DWORD that will contain the zero-based index of the element in the Skiplist.
 * If you're not interested in the index, you can set this parameter to NULL.
 *
 * @return
 * Returns the found element or NULL if no such element was found.
 */
PVOID
LookupElementSkiplist(PSKIPLIST Skiplist, PVOID Element, PDWORD ElementIndex)
{
    CHAR i;
    DWORD dwIndex = 0;
    PSKIPLIST_NODE pLastComparedNode = NULL;
    PSKIPLIST_NODE pNode = &Skiplist->Head;

    // Do the efficient lookup in Skiplists:
    //    * Start from the maximum level.
    //    * Walk through all nodes on this level that come before the node we're looking for.
    //    * When we have reached such a node, go down a level and continue there.
    //    * Repeat these steps till we're in level 0, right in front of the node we're looking for.
    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        while (pNode->Next[i] && pNode->Next[i] != pLastComparedNode && Skiplist->CompareRoutine(pNode->Next[i]->Element, Element) < 0)
        {
            dwIndex += pNode->Distance[i];
            pNode = pNode->Next[i];
        }   

        // Reduce the number of comparisons by not comparing the same node on different levels twice.
        pLastComparedNode = pNode->Next[i];
    }

    // We must be right in front of the node we're looking for now, otherwise it doesn't exist in the Skiplist at all.
    pNode = pNode->Next[0];
    if (!pNode || Skiplist->CompareRoutine(pNode->Element, Element) != 0)
    {
        // It hasn't been found, so there's nothing to return.
        return NULL;
    }

    // Return the index of the element if the caller is interested.
    if (ElementIndex)
        *ElementIndex = dwIndex;

    // Return the stored element of the found node.
    return pNode->Element;
}

/**
 * @name LookupNodeByIndexSkiplist
 *
 * Looks up a node in the Skiplist at the given position. The efficiency of this operation is O(log N) on average.
 *
 * @param Skiplist
 * Pointer to the SKIPLIST structure to operate on.
 *
 * @param ElementIndex
 * Zero-based position of the node in the Skiplist.
 *
 * @return
 * Returns the found node or NULL if the position is invalid.
 */
PSKIPLIST_NODE
LookupNodeByIndexSkiplist(PSKIPLIST Skiplist, DWORD ElementIndex)
{
    CHAR i;
    DWORD dwIndex = 0;
    PSKIPLIST_NODE pNode = &Skiplist->Head;

    // The only way the node can't be found is when the index is out of range.
    if (ElementIndex >= Skiplist->NodeCount)
        return NULL;

    // Do the efficient lookup in Skiplists:
    //    * Start from the maximum level.
    //    * Walk through all nodes on this level that come before the node we're looking for.
    //    * When we have reached such a node, go down a level and continue there.
    //    * Repeat these steps till we're in level 0, right in front of the node we're looking for.
    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        // We compare with <= instead of < here, because the added distances make up a 1-based index while ElementIndex is zero-based,
        // so we have to jump one node further.
        while (pNode->Next[i] && dwIndex + pNode->Distance[i] <= ElementIndex)
        {
            dwIndex += pNode->Distance[i];
            pNode = pNode->Next[i];
        }
    }

    // We are right in front of the node we're looking for now.
    return pNode->Next[0];
}
