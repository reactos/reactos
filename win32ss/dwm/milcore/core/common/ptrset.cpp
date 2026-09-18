// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------

//
//  Description: CPtrMultisetBase class implementation
//-----------------------------------------------------------------------------

#include "precomp.hpp"

#define PTRMULTISET_INITIAL_ALLOCATION 4
#define PTRMULTISET_GROWTH_FACTOR      1.5
#define PTRMULTISET_COMPACT_THRESHOLD  0.7
#define PTRMULTISET_ARRAY_CUTOFF       150
#define PTRMULTISET_QSORT_CUTOFF       5
#ifdef DEBUG
    // In Debug we use an extra meta-element for array versioning.
    #define PTRMULTISET_META_ELEMENTS  4
#else
    #define PTRMULTISET_META_ELEMENTS  3
#endif

typedef int (__cdecl *compare_fptr)(const void*, const void*);

//+-----------------------------------------------------------------------------
//  compare_pointers
//
//  Utility to compare two pointers by address, used by qsort() and bsearch().
//------------------------------------------------------------------------------

int __cdecl
compare_pointers(
    const __in_xcount(sizeof(T)) UINT_PTR *ptr1,
    const __in_xcount(sizeof(T)) UINT_PTR *ptr2
    )
{
    if (*ptr1 > *ptr2)
    {
        return 1;
    }
    else if (*ptr1 < *ptr2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

//+-----------------------------------------------------------------------------
//  bsearch_lastoccurence
//
//  An adaptation of standard binary search that returns a pointer to the
//  'last' occurance of the key if it exists in the base array, and NULL if it
//  is not present.
//------------------------------------------------------------------------------

UINT_PTR*
bsearch_lastoccurence(const UINT_PTR *key, UINT_PTR *base, size_t cEntries, compare_fptr compare)
{
    Assert(cEntries >= PTRMULTISET_ARRAY_CUTOFF);

    //
    // We're going to optimize the search for the last occurance of key by
    // actually searching for (key | 0x2).  This value will be just larger
    // than the actual key, and just smaller than occurances of key marked
    // for removal.  Since we're setting 0x2, we check to enforce that the
    // lowest two bits aren't set.
    //
    Assert((NULL != key) && ((*key & 0x3) == 0));

    HRESULT hr = S_OK;
    UINT low = 0;
    UINT mid = 0;
    UINT high = static_cast<UINT>(cEntries - 1);   // Won't underflow since cEntries >= PTRMULTISET_ARRAY_CUTOFF
    UINT_PTR *found = NULL;
    const UINT_PTR modKey = *key | 0x2; // Read the big comment a few lines up for explanation.

    do
    {
        //
        // If high and low are right next to each other, we've reached the
        // end of our search.  First check to see if the pointer at 'high'
        // is our value, next check 'low'.  This gives our "find last match"
        // behavior.  If neither value is our key, we should just return NULL
        // since the key wasn't found in the array.
        //
        if (1 == high - low)
        {
            if (*key == base[high])
            {
                found = base + high;
            }
            else if (*key == base[low])
            {
                found = base + low;
            }
            break;
        }

        IFC(UIntAdd(low, ((high - low) / 2), &mid));

        int comparison = compare(&modKey, base + mid);

        //
        // We augmented key, so we should never actually find (key | 0x2)
        // in the array, if we do, something is messed up.
        //
        Assert(0 != comparison);

        if (comparison > 0)
        {
            low = mid;
        }
        else
        {
            high = mid;
        }

    } while (low <= high);

Cleanup:
    return found;
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::Add
//
//  Adds a new element to the set while potentially invalidating the sorted-ness
//  of the existing elements.
//------------------------------------------------------------------------------

HRESULT
CPtrMultisetBase::Add(__in_xcount(sizeof(T)) UINT_PTR p)
{
    HRESULT hr = S_OK;
    size_t cEntries = GetCount();

#ifdef DEBUG
    IncrementArrayVersion();
#endif

    //
    // Since we steal the lower two bits of every value, we need to ensure
    // that anything we add has zeroes in the lower two bits.  Also, we
    // don't accept NULL pointers since NULL is returned by the 'End'
    // iterator.
    //
    if ((NULL != p) && ((p & 0x3) != 0))
    {
        IFC(E_INVALIDARG);
    }

    if (cEntries == 0)
    {
        //
        // count is 0 and increasing to 1
        // Optimization for count==1 case. In this case the multi-set is overloaded
        // to point to the item directly. This removes one indirection and
        // the need for additional allocated storage. This is motivated by
        // data which shows that count==1 is disproportionally common.
        //

        m_data = (p | 0x1);
    }
    else if (cEntries == 1)
    {
        //
        // count is 1 and increasing to 2
        // count==2, implement the general allocation array scheme.
        //

        UINT_PTR p0 = GetSingletonValue();

        UINT_PTR *newArray = NULL;

        IFC(HrMalloc(
            Mt(MILRawMemory),
            sizeof(UINT_PTR),
            PTRMULTISET_INITIAL_ALLOCATION + PTRMULTISET_META_ELEMENTS,
            reinterpret_cast<VOID**>(&newArray)
            ));

        newArray[0] = 2;                           	    // we have two elements
        newArray[1] = PTRMULTISET_INITIAL_ALLOCATION;   // the initial element capacity
        newArray[2] = 0;                                // removed-count 0, not sorted
#ifdef DEBUG
        // In Debug we use an extra meta-element for array versioning.
        newArray[3] = 0;
        newArray[4] = p0;
        newArray[5] = p;
#else
        newArray[3] = p0;
        newArray[4] = p;
#endif

        m_data = reinterpret_cast<UINT_PTR>(newArray) | 0x2;
    }
    else
    {
        size_t cAlloc = GetArrayAllocatedSize();
        size_t cRemoved = GetTaggedCountFromArray();
        double dCompactionFactor = GetCompactionFactorFromArray();
        bool fIsSorted;

        //
        // If the set has become more sparse than our threshold,
        // compact it.
        //
        if (dCompactionFactor < PTRMULTISET_COMPACT_THRESHOLD)
        {
            CompactDataArray();
            cEntries = GetCount();
            cRemoved = GetTaggedCountFromArray();
        }

        fIsSorted = IsDataSorted();
        UINT_PTR *dataArray = GetRawDataArray();

        if (cEntries == cAlloc)
        {
            //
            // it is time to realloc the array considering we
            // got too many entries.
            //

            UINT_PTR *newArray = NULL;
            size_t newAllocationSize = static_cast<size_t>(cAlloc * PTRMULTISET_GROWTH_FACTOR) + PTRMULTISET_META_ELEMENTS;

            if (newAllocationSize <= cAlloc)
            {
                //
                // if we can't grow the allocation then we wouldn't have
                // enough memory to allocate the result
                //

                IFC(E_OUTOFMEMORY);
            }

            IFC(HrMalloc(
                Mt(MILRawMemory),
                sizeof(UINT_PTR),
                newAllocationSize,
                reinterpret_cast<VOID**>(&newArray)
                ));

            // Copy the old data to the new array
            UINT_PTR *oldArray = GetRawDataArray();
            RtlCopyMemory(
                newArray + PTRMULTISET_META_ELEMENTS,
                oldArray + PTRMULTISET_META_ELEMENTS,
                cEntries * sizeof(UINT_PTR)
                );
            
            // Insert the new element at the end of the new array.
            newArray[cEntries + PTRMULTISET_META_ELEMENTS] = p;

            newArray[0] = cEntries + 1;                                  // we have one more element
            newArray[1] = newAllocationSize - PTRMULTISET_META_ELEMENTS; // new allocation, in elements
            newArray[2] = cRemoved;
#ifdef DEBUG
            // In Debug we use an extra meta-element for array versioning.
            newArray[3] = oldArray[3];
#endif

            // Update the dataArray pointer for the 'isSorted' check later on.
            dataArray = newArray;

            FreeHeap(oldArray);

            m_data = reinterpret_cast<UINT_PTR>(newArray) | 0x2;
        }
        else
        {
            Assert(cEntries < cAlloc);

            // Append the new element to the end of the set.
            dataArray[cEntries + PTRMULTISET_META_ELEMENTS] = p;

            dataArray[0]++; // One more element
        }

        // If we were sorted before, check whether we're still sorted.
        if (fIsSorted && (p < dataArray[cEntries + PTRMULTISET_META_ELEMENTS - 1]))
        {
            SetIsDataSorted(false);
        }
    }

    m_unsortedNewElements++;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::Remove
//
//  Removes an element from the multi-set. If we're above the 'array_cutoff'
//  threshold the multi-set is sorted before the element to remove is located
//  to optimize for the tear-down scenario.  Rather than removing the element
//  and shifting all following elements to the left by one, we just set the
//  lower two bits of the pointer to 0x3 to tag it as removed.  This maintains
//  the sorted-ness of the multi-set over multiple consecutive removals.
//------------------------------------------------------------------------------

bool
CPtrMultisetBase::Remove(__in_xcount(sizeof(T)) UINT_PTR p)
{
    size_t cEntries = GetCount();
    bool fRemoved = false;

#ifdef DEBUG
    IncrementArrayVersion();
#endif

    if (cEntries > 0)
    {
        if (cEntries == 1)
        {
            if (p == GetSingletonValue())
            {
                m_data = 0;
                fRemoved = true;
            }
        }
        else
        {
            UINT_PTR *pElements = GetElementArray();

            if (cEntries <= PTRMULTISET_ARRAY_CUTOFF)
            {
                UINT pos = 0;

                if (cEntries == PTRMULTISET_ARRAY_CUTOFF)
                {
                    CompactDataArray();
                }

                //
                // Array case, linear forward search through the array for the specified item.
                //
                for (; pos < cEntries && p != pElements[pos]; pos++)
                    ;

                if (pos < cEntries)
                {
                    UINT_PTR *rawArray = GetRawDataArray();

                    fRemoved = true;
                    cEntries--;

                    if (cEntries == 1)
                    {
                        //
                        // There are only two elements left in the array and
                        // pos is the index of the one we want to delete. I.e.
                        // 1-pos is the index of the one we want to save.
                        //

                        Assert(pos == 0 || pos == 1);

                        UINT_PTR pvSave = pElements[1 - pos];
                        FreeHeap(rawArray);
                        m_data = pvSave | 0x1;
                    }
                    else
                    {
                        //
                        // Overwrite the removed element with the last element
                        // in the array.
                        //

                        pElements[pos] = pElements[cEntries];
                        rawArray[0] = cEntries;
                    }
                }
            }
            else
            {
                //
                // If the data isn't sorted, sort it so that we call bsearch().
                //
                if (!IsDataSorted())
                {
                    Sort();
                }

                UINT_PTR *found = bsearch_lastoccurence(
                    &p,
                    pElements,
                    cEntries,
                    (compare_fptr)compare_pointers);

                //
                // If we found the item, remove it from the list.
                //
                if (found)
                {
                    UINT_PTR *rawArray = GetRawDataArray();
                    *found |= 0x3;                          // mark as removed
                    rawArray[2]++;                          // increment removed-count
                    fRemoved = true;

                    //
                    // compact the set if the elements have become too sparse.
                    //
                    double dCompactionFactor = GetCompactionFactorFromArray();
                    if (dCompactionFactor < PTRMULTISET_COMPACT_THRESHOLD)
                    {
                        CompactDataArray();
                    }
                }
            }
        }
    }

    return fRemoved;
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::Contains
//
//  Return true if the specified element is contained in the set.
//------------------------------------------------------------------------------

bool
CPtrMultisetBase::Contains(__in_xcount(sizeof(T)) UINT_PTR p)
{
    UINT cEntries = static_cast<UINT>(GetCount());
    bool fFound = false;

    if (cEntries == 0)
    {
        // Do nothing, the element is not in the set.
    }
    else if (cEntries == 1)
    {
        fFound = (p == GetSingletonValue());
    }
    else if (cEntries <= PTRMULTISET_ARRAY_CUTOFF)
    {
        //
        // array case, linear search to find the element
        //
        const UINT_PTR *pElements = GetElementArray();
        for (UINT i = 0; i < cEntries && !fFound; i++)
        {
            fFound = (pElements[i] == p);
        }
    }
    else
    {
        //
        // set case, sort the data if it isn't already so that we can bsearch()
        //
        if (!IsDataSorted())
        {
            this->Sort();
        }

        const UINT_PTR *pElements = GetElementArray();
        if (bsearch(&p, pElements, cEntries, sizeof(UINT_PTR), (compare_fptr)compare_pointers))
        {
            fFound = true;
        }
    }

    return fFound;
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::Clear
//
//  Clears all elements from the multi-set and releases any memory that's been
//  allocated.
//------------------------------------------------------------------------------

void
CPtrMultisetBase::Clear()
{
#ifdef DEBUG
    IncrementArrayVersion();
#endif

    //
    // If we are holding on to an array then we need to free the memory
    //
    if (IsDataArray())
    {
        FreeHeap(GetRawDataArray());
    }
    m_data = 0;
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::Sort
//
//  Sorts the multi-set and sets the 'is-sorted' bit.
//------------------------------------------------------------------------------

void
CPtrMultisetBase::Sort()
{
    Assert(IsDataArray());
    
#ifdef DEBUG
    IncrementArrayVersion();
#endif

    size_t cEntries = GetCount();
    Assert(cEntries > PTRMULTISET_ARRAY_CUTOFF);
    UINT_PTR *pElements = GetElementArray();

    if (m_unsortedNewElements > PTRMULTISET_QSORT_CUTOFF)
    {
        //
        // Many new elements have been added since the previous sort, use qsort.
        //
        qsort(pElements, cEntries, sizeof(UINT_PTR), (compare_fptr)compare_pointers);
    }
    else
    {
        //
        // Not many new elements have been added since the previous sort, so the
        // list is still nearly sorted. Use insertion sort.
        //
        
        size_t inserting = 0;
        UINT_PTR swap;

        //
        // cEntries >= PTRMULTISET_ARRAY_CUTOFF = 150, so cEntries - 2 is
        // positive
        //
        for (size_t sorted = 0; sorted < cEntries - 1; sorted++)
        {
            //
            // Everything with index <= sorted is sorted, find a place to insert
            // element sorted + 1. sorted ranges from 0 to cEntries - 2, so
            // inserting ranges from 1 to cEntries - 1, which is within array
            // bounds.
            //
            inserting = sorted + 1;

            // inserting - 1 is also within array bounds (0 to cEntries - 2)
            while (inserting > 0 && compare_pointers(pElements + inserting, pElements + inserting - 1) == -1)
            {
                //
                // All elements with index < inserting is sorted, and the
                // element at inserting is smaller than the one before it, so
                // swap the two and keep looking towards index 0.
                //
                swap = pElements[inserting - 1];
                pElements[inserting - 1] = pElements[inserting];
                pElements[inserting] = swap;
                inserting--;
            }

            //
            // The element that used to be at sorted + 1 has been inserted, move
            // on to the next element.
            //
        }
    }

    SetIsDataSorted(true);

    m_unsortedNewElements = 0;
}

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase::CompactDataArray
//
//  Compacts the underlying data by moving all untagged elements to the
//  beginning of the underlying element array. This reclaims elements tagged for
//  removal by resetting the 'actual' count to return the number of untagged
//  elements in the set.
//------------------------------------------------------------------------------

void
CPtrMultisetBase::CompactDataArray()
{
    Assert(IsDataArray());
    
#ifdef DEBUG
    IncrementArrayVersion();
#endif

    size_t cEntries = GetCount();
    Assert(cEntries >= PTRMULTISET_ARRAY_CUTOFF);
    size_t cUntaggedEntries = cEntries - GetTaggedCountFromArray();

    if (cEntries != cUntaggedEntries)
    {
        UINT_PTR *pElements = GetElementArray();

        // Search for the first element tagged for removal.
        size_t i = 0;
        while (!IsTaggedForRemoval(pElements[i]))
        {
            i++;
        }

        //
        // In this loop we perform the actual compaction. We keep incrementing
        // j until we find an element which is not tagged for removal, at which
        // point append the untagged element to the end of the compacted region
        // of the set.  We continue compacting elements until i has iterated
        // through the full count of untagged entries.
        //
        for (size_t j = i + 1; i < cUntaggedEntries; j++)
        {
            if (!IsTaggedForRemoval(pElements[j]))
            {
                pElements[i++] = pElements[j];
            }
        }

        GetRawDataArray()[0] = cUntaggedEntries; // reset the number of actual elements
        GetRawDataArray()[2] &= 0x80000000;      // clear the tagged count
    }
}

