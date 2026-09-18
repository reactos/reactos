// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Abstract:

    Array of pointers. Can be dynamically grown.

Environment:

    User mode only.


--*/

#include "precomp.hpp"

#define PTRARRAY_INITIAL_ALLOCATION 4
#define PTRARRAY_GROWTH_FACTOR      1.5

void CPtrArrayBase::Clear()
{
    //
    // If we are holding on to an array then we need to free the memory
    //

    if (IsDataArray())
    {
        FreeHeap(GetRawDataArray());
    }
    m_data = 0;
}

HRESULT CPtrArrayBase::InsertAt(__in_xcount(sizeof(T)) UINT_PTR p, size_t index)
{
    HRESULT     hr = S_OK;
    size_t      cEntries = GetCount();

    if (index > cEntries)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Since we steal the lower two bits of the first value, we need to ensure
    // that every value has zeroes in the lower two bits, since every value
    // can potentially be the first after a sequence of Add/Remove calls.
    //
    if ((p & 0x3) != 0)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Insert the item according to the initial state
    //

    if (cEntries == 0)
    {
        //
        // count is 0 and increasing to 1
        // Optimization for count==1 case. In this case the array is overloaded
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
            PTRARRAY_INITIAL_ALLOCATION + 2,  // Add two for count and capacity storage
            reinterpret_cast<VOID**>(&newArray)
            ));

        //
        // we have two elements now and the new element goes at the
        // specified index, so the original element goes at 1-index,
        // all offset by 2 to account for the count and alloc size
        //

        newArray[0] = 2;                            // we have two elements
        newArray[1] = PTRARRAY_INITIAL_ALLOCATION;  // the initial element capacity
        newArray[3 - index] = p0;
        newArray[2 + index] = p;

        m_data = reinterpret_cast<UINT_PTR>(newArray) | 0x2;
    }
    else
    {
        size_t cAlloc = GetArrayAllocatedSize();
        if (cEntries == cAlloc)
        {
            //
            // it is time to realloc the array considering we
            // got too many entries.
            //

            UINT_PTR    *newArray = NULL;
            size_t      newAllocationSize = static_cast<size_t>(cAlloc * PTRARRAY_GROWTH_FACTOR) + 2;

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

            //
            // copy the old data to the new array
            //

            UINT_PTR *oldArray = GetRawDataArray();

            if (index > 0)
            {
                RtlCopyMemory(
                    newArray + 2,
                    oldArray + 2,
                    index * sizeof(UINT_PTR)
                    );
            }

            newArray[index + 2] = p;

            if (index < cEntries)
            {
                RtlCopyMemory(
                    newArray + index + 3,
                    oldArray + index + 2,
                    (cEntries - index) * sizeof(UINT_PTR)
                    );
            }

            FreeHeap(oldArray);

            newArray[0] = cEntries + 1;             // we have one more element
            newArray[1] = newAllocationSize - 2;    // new allocation, in elements

            m_data = reinterpret_cast<UINT_PTR>(newArray) | 0x2;
        }
        else
        {
            Assert(cEntries < cAlloc);

            //
            // make room for the new element
            //

            UINT_PTR *dataArray = GetRawDataArray();

            if (index < cEntries)
            {
                RtlMoveMemory(
                    dataArray + index + 3,
                    dataArray + index + 2,
                    (cEntries - index) * sizeof(UINT_PTR)
                    );
            }

            dataArray[index + 2] = p;
            dataArray[0]++; // One more element
        }
    }

Cleanup:
    RRETURN(hr);
}

bool CPtrArrayBase::Remove(__in_xcount(sizeof(T)) UINT_PTR p)
{
    size_t  cEntries = GetCount();
    bool    fRemoved = false;

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
            size_t pos = 0;
            UINT_PTR *pElements = GetElementArray();

            //
            // Linear forward search through the array for the specified item.
            //

            for (; pos < cEntries && p != pElements[pos]; pos++)
                ;

            //
            // If we find the item, remove it from the list.
            //

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
                    // Compact the array over the item to be removed by
                    // shifting all the subsequent items down by one.
                    //

                    if (pos < cEntries)
                    {
                        RtlMoveMemory(
                            pElements + pos,
                            pElements + pos + 1,
                            (cEntries - pos) * sizeof(UINT_PTR)
                            );
                    }

                    rawArray[0] = cEntries;
                }
            }
        }
    }

    return fRemoved;
}

__out_xcount_opt(sizeof(T)) UINT_PTR CPtrArrayBase::operator[](size_t index) const
{
    size_t cEntries = GetCount();

    if (index < cEntries)
    {
        if (cEntries == 1)
        {
            return GetSingletonValue();
        }
        else
        {
            return GetElementArray()[index];
        }
    }

    return NULL;
}


