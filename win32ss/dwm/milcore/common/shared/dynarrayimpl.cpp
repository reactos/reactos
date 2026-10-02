// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Module Name:
*
*   Dynamic array implementation class
*
* Abstract:
*
*   This is the class which implements the dynamic array.
*   It is used by the wrapper template classes DynArray and DynArrayIA.
*
* Created:
*
*  2/18/1999 agodfrey
*  Refactored Dynarray to use an implementation class (DynArrayImpl).
*
*  6/10/1999 t-wehunt
*  + Added AddMultipleAt and DeleteMultipleAt methods.
*  + Fixed a problem in ShrinkToSize that caused elements to potentially
*    be lost.
*  8/16/2000 bhouse
*  + Changed cpacity growth to be exponential
*  4/15/2003 jasonha
*  + Added support for zeroing memory during init and when growing
*
\**************************************************************************/

#include "precomp.hpp"

MtDefine(MDynArray, MILRawMemory, "MDynArray");
MtDefine(DynArray, MIL, "DynArray");


/**************************************************************************\
*
* Function Description:
*
*   DynArrayImpl constructor
*
* Arguments:
*
*   initialAllocation - initial allocation
*   allocSize         - size of the initial allocation
*   count             - initial count
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/

template <bool fZeroMemory>
DynArrayImpl<fZeroMemory>::DynArrayImpl(
    __inout_bcount_part_opt(allocSize*eltSize, count*eltSize) void *initialAllocation,
    __out_range(==, this->Capacity) __out_range(==, this->AllocSize) UINT allocSize,
    __in_range(<=, allocSize) __out_range(==, this->Count) UINT count,
    __out_range(==, this->ElementSize) UINT eltSize
    )
    :
    m_pData(reinterpret_cast<BYTE *>(initialAllocation)),
    InitialAllocation(initialAllocation),
    AllocSize(allocSize),
    Capacity(allocSize),
    Count(count)
    ANALYSIS_COMMA_PARAM(ElementSize(eltSize))
{
    Assert((initialAllocation != NULL) || (allocSize == 0));
    Assert(count <= allocSize);

    if (UNCONDITIONAL_EXPR(fZeroMemory && initialAllocation))
    {
        // Validate that the initial allocation size in bytes doesn't overflow UINT
        // This constructor is only used by DynArrayIA, which has a C_ASSERT on the same
        // condition.
#if DBG_ANALYSIS
        UINT uResult;

        Assert(SUCCEEDED(UInt32Mult(allocSize, eltSize, &uResult)));
#endif

        ZeroMemory(initialAllocation, allocSize*eltSize);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   DynArrayImpl Destructor
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/
template <bool fZeroMemory>
DynArrayImpl<fZeroMemory>::~DynArrayImpl()
{
    if (m_pData != InitialAllocation)
    {
        WPFFree(ProcessHeap, m_pData);
        m_pData = NULL;
    }

}
/**************************************************************************\
*
* Function Description:
*
*   Shrink the buffer so that it is just big enough for the items
*   the dynamic array holds.
*
* Arguments:
*
*   eltSize - size of each array element
*
* Return Value:
*
*   NONE
*
* Created:
*
*   1/18/1999 agodfrey
*     Added code to reuse the initial allocation.
*
\**************************************************************************/

template <bool fZeroMemory>
VOID DynArrayImpl<fZeroMemory>::ShrinkToSize(
    __in_range(==, this->ElementSize) UINT eltSize
    )
{
    Assert(Count <= Capacity);
    UINT newUsedSize;

    if (m_pData == InitialAllocation)
    {
        // Since we're shrinking, we know that the current data buffer
        // is big enough.

        return;
    }

    if (FAILED(UIntMult(Count, eltSize, &newUsedSize)))
    {
        // Multiplication overflow occurred calculating new buffer size. Keep current allocation.

        TraceTag((tagMILWarning, "ShrinkToSize: MultipyUINT produced overflow calculating new buffer size"));

        return;
    }


    if (Count <= AllocSize)
    {
        // The buffer will fit into the initial allocation.

        RtlCopyMemory(InitialAllocation, m_pData, newUsedSize);
        WPFFree(ProcessHeap, m_pData);
        m_pData = (BYTE *)InitialAllocation;
        Capacity = AllocSize;
        return;
    }

    // If we get here, we know that DataBuffer points to dynamic memory,
    // and that Count != 0.

    VOID *pvData = m_pData;

    if (SUCCEEDED(WPFRealloc(
        ProcessHeap,
        Mt(MDynArray),
        &pvData,
        newUsedSize
        )))
    {
        m_pData = (BYTE*)pvData;
        Capacity = Count;
    }
    else
    {
        // Realloc failed. Keep the current allocation
        TraceTag((tagMILWarning, "ShrinkToSize: Realloc failed."));
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Add space for new elements (if necessary). Does not update Count.
*
* Arguments:
*
*   eltSize     - size of each array element
*   newElements - the number of new elements
*   exactSize   - no exponential growth, just add required amount
*
* Created:
*
*   1/18/1999 agodfrey
*
\**************************************************************************/

template <bool fZeroMemory>
HRESULT
DynArrayImpl<fZeroMemory>::Grow(
    __in_range(==, this->ElementSize) UINT eltSize,
    __out_range(<=, this->Capacity-this->Count) UINT newElements,
    BOOL exactSize
    )
{
    HRESULT hr = S_OK;
    UINT newCapacity, newUsedSize, newAllocatedSize, newCount;
    void *newbuf = NULL;

    // Get the new count
    IFC(AddUINT(Count, newElements, newCount));

    if (newCount <= Capacity)
    {
        // No need to grow
        goto Cleanup;
    }

    // Get the new size of memory used by the elements
    IFC(UIntMult(newCount, eltSize, &newUsedSize));

    // Compute new capacity
    if (exactSize)
    {
        newCapacity = newCount;
        newAllocatedSize = newUsedSize;
    }
    else
    {
        UINT capacityIncrement = newCount - Capacity;
        capacityIncrement = max(capacityIncrement,
                                min(max(Capacity, (UINT)kMinCapacityGrowth),
                                    (UINT)kMaxCapacityGrowth));

        if (FAILED(AddUINT(Capacity, capacityIncrement, OUT newCapacity))  ||
            FAILED(UIntMult(newCapacity, eltSize, &newAllocatedSize)))
        {
            // The computed capacity will overflow, stick with the exact size
            newCapacity = newCount;
            newAllocatedSize = newUsedSize;
        }
    }

    if (newCapacity > (UINT_MAX / eltSize))
    {
        IFC(WINCODEC_ERR_VALUEOVERFLOW);
    }

    if (m_pData == InitialAllocation)
    {
        // Do our first dynamic allocation

        IFC(HrAlloc(Mt(MDynArray), newAllocatedSize, &newbuf));

        if (Count)
        {
            // Assert this success, since we've already ensured that Count < Capacity and that
            // Capacity * eltSize can be represented as a UINT32.
#if DBG_ANALYSIS
            UINT cbContents = 0;

            Assert(SUCCEEDED(UInt32Mult(Count, eltSize, &cbContents)));
#endif

            RtlCopyMemory(newbuf, m_pData, Count * eltSize);
        }
    }
    else
    {
        // Reallocate memory
        newbuf = m_pData;

        IFC(WPFRealloc(
            ProcessHeap,
            Mt(MDynArray),
            &newbuf,
            newAllocatedSize
            ));
    }

    if (UNCONDITIONAL_EXPR(fZeroMemory))
    {
        // Clear newly allocated memory; previous allocation is left intact
        // The check above ensured that newCapacity * eltSize doesn't overflow,
        // and newCapacity is >= Capacity.

        ZeroMemory(
            static_cast<BYTE *>(newbuf)+Capacity*eltSize,
            (newCapacity-Capacity) * eltSize
            );
    }

    Capacity = newCapacity;
    m_pData = (BYTE *)newbuf;

Cleanup:
    RRETURN(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   Detach the data buffer from the dynamic array.
*   Allocates the buffer to detatch if it is the initial allocation.
*
* Return Value:
*
*   The data buffer
*
* Created:
*
*   2/25/1999 agodfrey
*   12/19/2000 asecchia - handle initial allocation by returning a copy.
*
\**************************************************************************/

template <bool fZeroMemory>
HRESULT
DynArrayImpl<fZeroMemory>::DetachData(
    __in_range(==, this->ElementSize) UINT eltSize,
    __deref_out_bcount_part(this->Capacity*eltSize, this->Count*eltSize) void **buffer
    )
{
    HRESULT hr = S_OK;
    void *data = m_pData;

    // Copy the initial allocation if there is one -
    // otherwise we simply use the m_pData.

    if (m_pData == InitialAllocation)
    {
        data = NULL;
        hr = HrMalloc(Mt(MDynArray), eltSize, Capacity, &data);

        if (FAILED(hr))
        {
            *buffer = NULL;
            return hr;
        }

        if (Count)
        {
            GpMemcpy(data, m_pData, Count*eltSize);
        }
    }

    m_pData = NULL;
    Count = Capacity = 0;

    *buffer = data;
    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Add new, uninitialized elements, and return a pointer to them.
*
* Arguments:
*
*   eltSize     - size of each element
*   newElements - number of new elements
*   pData       - pointer to the start of the location for the new data
*
* Return Value:
*
*   Pointer to the new space, or NULL on failure
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/

template <bool fZeroMemory> HRESULT
DynArrayImpl<fZeroMemory>::AddMultiple(
    __in_range(==, this->ElementSize) UINT eltSize,
    __out_range(==, this->Count-pre(this->Count)) __out_range(<=, this->Capacity-pre(this->Count)) UINT newElements,
    __deref_opt_out_xcount_part(newElements*eltSize,
                                fZeroMemory ? newElements*eltSize : 0)
        void **ppData
    )
{
    HRESULT hr;

    IFC(Grow(eltSize, newElements));

    // The arithemetic was checked for overflow inside Grow.
    if (ppData)
    {
        *ppData = static_cast<BYTE *>(m_pData) + Count * eltSize;
    }
    Count += newElements;

Cleanup:
    RRETURN(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   Add new elements, initializing them with the given data.
*
* Arguments:
*
*   eltSize     - size of each element
*   newElements - number of new elements
*   newData     - the data to be copied into the new space
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/

template <bool fZeroMemory>
HRESULT
DynArrayImpl<fZeroMemory>::AddMultipleAndSet(
    __in_range(==, this->ElementSize) UINT eltSize,
    __out_range(==, this->Count-pre(this->Count)) __out_range(<=, this->Capacity-pre(this->Count)) UINT newElements,
    __in_bcount(newElements*eltSize) const void *newData
    )
{
    HRESULT hr = Grow(eltSize, newElements);

    if (SUCCEEDED(hr))
    {
        // NOTE: assume T is a shallow data type, i.e.
        //  it doesn't contain nested references.

        // Grow has checked that m_pData + Capacity * eltSize, the new Count,
        // and Capacity * eltSize will not overflow. Since Count <= Capacity and
        // newElements <= Capacity, we know that none of the computations below
        // will overflow.

#if DBG_ANALYSIS
        UINT tmpOut = 0;
        UINT64 tmpOut64 = 0;

        Assert(SUCCEEDED(UInt32Mult(Capacity, eltSize, &tmpOut)));
        Assert(SUCCEEDED(UInt64Add(reinterpret_cast<__int64>(m_pData), tmpOut, &tmpOut64)));
        Assert(SUCCEEDED(UInt32Mult(newElements, eltSize, &tmpOut)));
#endif

        RtlCopyMemory(
            static_cast<BYTE *>(m_pData) + Count * eltSize,
            newData,
            newElements * eltSize
            );
        Count += newElements;
    }

    return hr;
}


// Explicit template instantiation
template class DynArrayImpl<false>;
template class DynArrayImpl<true>;



