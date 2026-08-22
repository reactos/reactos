// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Abstract:
*
*   Dynamic array template classes. See DynamicArrays.doc in the Specs
*   directory.
*
*   DynArray is a container which keeps its contents in a contiguous buffer,
*   reallocating memory as necessary. It accepts an optional initial
*   allocation, which is used unless it is too small to accommodate the
*   elements.
*
*   DynArrayIA is a cosmetic wrapper which encapsulates the intial allocation,
*   allowing the dynamic array to be treated like a normal class.
*
*
\**************************************************************************/

#pragma once

MtExtern(DynArray);

template <typename T, bool fZeroMemory = false> class DynArray : public DynArrayImpl<fZeroMemory>
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(DynArray));

    // Constructor (no initial allocation)
    //

    DynArray(void):
        DynArrayImpl<fZeroMemory>(NULL, 0, 0, sizeof(T))
    {
    }

    // Return a pointer to the array data
    //   NOTE: We intentionally avoid type conversion operator here
    //   to reduce the chances for confusion.

    __ecount(this->Count) T *GetDataBuffer() const
    {
        return reinterpret_cast<T *>(m_pData);
    }

    //
    // At 
    //
    // Returns the element at the specified index. At performs a range check and 
    // crashes safely if the index is out of range. 
    //

    __out T& At(UINT n) const
    {
        FreAssert(n < Count);
        return GetDataBuffer()[n];
    }


    // Index operator
    //
    // Note that the index operator does not perform any range checks. The caller
    // needs to ensure that the index used does not lead to a buffer overflow.


    __ecount(1) T &operator[](__in_range(0, this->Count-1) UINT n) const
    {
        __pfx_assert(n < Count, "Buffer overflow accessing DynArray");
        Assert(n < Count);

#ifdef DBG
        return At(n); // At performs a range check which is slower. In retail we don't mind it this
                      // and as a benefit we get some double checking on the code that uses the index
                      // operator[].
#else
        return GetDataBuffer()[n];
#endif

    }

    // First/last element of the array

    __ecount(this->Count) T &First() const
    {
        __pfx_assert(Count > 0, "Buffer overflow accessing empty DynArray");
        Assert(Count > 0);
        return GetDataBuffer()[0];
    }

    __ecount(1) T &Last() const
    {
        __pfx_assert(Count > 0, "Buffer overflow accessing empty DynArray");
        Assert(Count > 0);
#pragma prefast (push)
#pragma prefast (disable: 37001 37002 37003, "This operation will not overflow becasuse of the Assert above.")
        return GetDataBuffer()[Count-1];
#pragma prefast (pop)
    }

    // Number of elements in the array

    __range(==, this->Count) UINT GetCount() const
    {
        return Count;
    }

    __range(==, this->Capacity) UINT GetCapacity() const
    {
        return Capacity;
    }

    // Reset the dynamic array to empty state
    //
    // shrink - If FALSE, don't free the current buffer.

    VOID Reset(BOOL shrink=TRUE)
    {
        Count = 0;
        if (shrink)
        {
            ShrinkToSize();
        }
    }

    // Shrink the dynamic array capacity to be just big enough
    // for the number of existing elements in the array.
    //
    // This reuses the initial allocation if possible.

    VOID ShrinkToSize()
    {
        DynArrayImpl<fZeroMemory>::ShrinkToSize(sizeof(T));
    }

    // Add a new element to the end of the dynamic array

    MIL_FORCEINLINE HRESULT Add(
        __in_ecount(1) const T& newItem
        )
    {
         return AddMultipleAndSet(&newItem, 1);
    }

    // Add multiple items to the end of the dynamic array

    MIL_FORCEINLINE HRESULT AddMultipleAndSet(
        __in_ecount(n) const UNALIGNED T* newItems,
        __out_range(<=, this->Capacity-pre(this->Count)) UINT n
        )
    {
        // We need this code to inline since it is very high bandwidth
        HRESULT hr = S_OK;
        UINT cTotal;

        IFC(AddUINT(Count, n, cTotal));

        if (cTotal <= Capacity)
        {
            // We know that none of the arithmetic below will overflow because Grow
            // checked that for Capacity last time we allocated
            memcpy(m_pData + Count * sizeof(T), newItems, sizeof(T) * n);
            Count = cTotal;
        }
        else
        {
            IFC(DynArrayImpl<fZeroMemory>::AddMultipleAndSet(sizeof(T), n, newItems));
        }
    Cleanup:
        RRETURN(hr);
    }

    // Another variation of AddMultiple above
    //
    // In this case, the data for the new elements are
    // not available. Instead, we'll do the following:
    //  (1) reserve the space for additional elements
    //  (2) increase the Count by the number of additional elements
    //  (3) return a pointer to the first new elements

    HRESULT AddMultiple(
        __out_range(<=, this->Capacity-pre(this->Count)) UINT n,
        __deref_opt_out_xcount_part(n, fZeroMemory ? n : 0) T **ppNewElements=NULL
        )
    {
        return DynArrayImpl<fZeroMemory>::AddMultiple(
            sizeof(T),
            n,
            reinterpret_cast<void **>(ppNewElements)
            );
    }

    // Add a given number of elements, setting every one to the given value

    HRESULT AddAndSet(
        __out_range(<=, this->Capacity-pre(this->Count)) UINT n,
        __in_ecount(1) const T &t
        )
    {
        HRESULT hr;
        T *p;
        UINT i;

        IFC(AddMultiple(n, &p));
        Assert(p);

        for (i=0;  i < n;  i++)
        {
            p[i] = t;
        }
    Cleanup:
        RRETURN(hr);
    }

    // Detach the data buffer from the dynamic array
    // Allocates the buffer to detatch if it is the initial allocation.

    HRESULT DetachData(
        __deref_out_ecount_part((this->Capacity), (this->Count)) T **buffer
        )
    {
        return DynArrayImpl<fZeroMemory>::DetachData(sizeof(T), (void **)buffer);
    }

    // Detatch the buffer from another array, and set this array
    // to point to it. NOTE: This modifies the other array.

    HRESULT ReplaceWith(
        __inout_ecount(1) DynArray<T> *dynarr
        )
    {
        if (m_pData != InitialAllocation)
        {
            GpFree(m_pData);
            m_pData = NULL;
        }

        Count = dynarr->Count;
        Capacity = dynarr->Capacity;

        HRESULT hr = dynarr->DetachData((T**)(&m_pData));

        if (FAILED(hr))
        {
            Count = 0;
            Capacity = 0;
        }

        return hr;
    }

    // More dangerous interface:
    //
    // These functions are alternatives to Add/AddMultiple.
    // They can be used to reduce overhead, but you have to know what
    // you're doing.
    //
    // AdjustCount/SetCount/DecrementCount - modify the count directly, without growing or
    //   shrinking the array.
    // ReserveSpace - grow the buffer, but don't actually add any elements to
    //   the array.

    VOID AdjustCount(
        __out_range(==,this->Count-pre(this->Count)) UINT addElts
        )
    {
        Count += addElts;

        Assert(Count <= Capacity);
    }

    VOID SetCount(
        __in_range(<=, this->Capacity) __out_range(==,this->Count) UINT count
        )
    {
        Assert(count <= Capacity);

        Count = count;
    }

    HRESULT ReserveSpace(
        __out_range(<=, this->Capacity-pre(this->Count)) UINT newElements,
        BOOL exact = FALSE
        )
    {
        return Grow(sizeof(T), newElements, exact);
    }

    VOID DecrementCount()
    {
        if (Count > 0)
        {
            --Count;
        }
    }

    //
    // Find the location of t in the array beginning the sequential search
    // from idxStart.
    //

    __range(idxStart, this->Count) UINT Find(
        __in_range(0, this->Count-1) UINT idxStart,
        __in_ecount(1) const T &t
        )
    {
        UINT i = idxStart;
        T *rgDataBuffer = GetDataBuffer();
        for(; i < Count && t != rgDataBuffer[i]; i++);
        return i;
    }

    //
    // Remove the first instance of t from the array. Shift all entries
    // in the array after t down by one to close the gap in the array.
    // If it can't find the item in the array, the function returns FALSE
    //

    BOOL Remove(__in_ecount(1) const T &t)
    {
        //
        // Find the location of t in the array.
        //

        UINT i = Find(0, t);

        if (i >= Count)
        {
            return FALSE;
        }

        T *rgDataBuffer = GetDataBuffer();

        //
        // Move all the subsequent entries down by one, deleting the t
        // entry in the array.
        //

        Assert(Count > 0);

        for(; i < Count-1; i++)
        {
            rgDataBuffer[i] = rgDataBuffer[i+1];
        }

        //
        // Now shrink the array count by one to reflect the deletion.
        //

        SetCount(Count-1);

        return TRUE;
    }

    //
    // Remove the element at index from the array. Shift all entries
    // in the array after index down by one to close the gap in the array.
    // If it can't find the item in the array, the function returns FALSE.
    //

    HRESULT RemoveAt(
        __in_range(0, this->Count-1) UINT index
        )
    {
        HRESULT hr = S_OK;

        if (index >= Count)
        {
            MIL_THR(E_INVALIDARG);
        }

        if (SUCCEEDED(hr))
        {
            T *rgDataBuffer = GetDataBuffer();

            //
            // Move all the subsequent entries down by one, deleting the t
            // entry in the array.
            //

            for(UINT i = index; i < Count-1; i++)
            {
                rgDataBuffer[i] = rgDataBuffer[i+1];
            }

            //
            // Now shrink the array count by one to reflect the deletion.
            //

            SetCount(Count-1);
        }

        RRETURN(hr);
    }

    //
    // Insert a new child t at the index i by shifting all the entries
    // in the array up by one.
    //

    HRESULT InsertAt(
        __in_ecount(1) const T &t,
        __in_range(0, this->Count) UINT idx
        )
    {
        HRESULT hr = S_OK;

        if (idx > Count)
        {
            //
            // Only allow to insert the new element anywhere between
            // the existing elements (0 <= idx < Count) of the array
            // or at the very end of the array (idx == Count).
            //

            MIL_THR(E_INVALIDARG);
        }

        if (SUCCEEDED(hr))
        {
            //
            // Add space at the end of the array
            //

            MIL_THR(AddMultiple(1));

            if (SUCCEEDED(hr))
            {
                T *rgDataBuffer = GetDataBuffer();

                //
                // Shift all the entries from idx onward up by one. Note, we've
                // already added additional space for the new element and
                // incremented the Count in the array using the AddMultiple(1) call
                // above.
                //

                for (UINT i=Count-1; i>idx; i--)
                {
                    rgDataBuffer[i] = rgDataBuffer[i-1];
                }

                //
                // Add the new item at the proper position.
                //

                rgDataBuffer[idx] = t;
            }
        }

        RRETURN(hr);
    }



    //
    // Remove the element at index from the array. The array is compacted
    // but the order of elements is not preserved
    // If it can't find the item in the array, the function returns FALSE
    //

    HRESULT RemoveAtOrderNotPreserved(
        __in_range(0, this->Count-1) UINT index
        )
    {
        HRESULT hr = S_OK;

        if (index < Count)
        {
            T *rgDataBuffer = GetDataBuffer();


            //
            // Move the last element into the vacated space
            // if the element isn't already at the end

            if (index < (Count - 1))
            {
                rgDataBuffer[index] = rgDataBuffer[(Count - 1)];
            }

            //
            // Now shrink the array count by one to reflect the deletion.
            //

            SetCount(Count-1);
        }
        else
        {
            MIL_THR(E_INVALIDARG);
        }

        RRETURN(hr);
    }

    //
    // shifts the entire array so element[index]
    // becomes element[0] and the size is reduced by index
    //
    HRESULT ShiftLeft(
        __in_range(0, this->Count) UINT index
        )
    {
        HRESULT hr = S_OK;

        if ((index > 0) && (index <= Count))
        {
            UINT cNew = Count - index;
            if (cNew > 0)
            {
                T *rgDataBuffer = GetDataBuffer();

                // shift the entire array to the left
                memmove(rgDataBuffer, &rgDataBuffer[index], sizeof(T)*cNew);
            }
            SetCount(cNew);
        }
        else if (index != 0)
        {
            MIL_THR(E_INVALIDARG);
        }

        RRETURN(hr);
    }

    HRESULT Copy(__in_ecount(1) const DynArray<T> &other)
    {
        return DynArrayImpl<fZeroMemory>::AddMultipleAndSet(sizeof(T), other.Count, other.m_pData);
    }

protected:

    // Constructor, for use only by DynArrayIA
    //

    DynArray(
        __inout_ecount_part_opt(allocSize, count) T *initialAllocation,
            // the initial allocation, which can be global,
            // static or dynamic memory (or NULL)
        __out_range(==, this->Capacity) UINT allocSize,
            // size of the initial allocation (0 if there is none)
        __in_range(<=, allocSize) __out_range(==, this->Count) UINT count = 0
            // the initial count
        ):
        DynArrayImpl<fZeroMemory>(initialAllocation, allocSize, count, static_cast<UINT>(sizeof(T)))
    {
        // Validate that the initial allocation size in bytes doesn't overflow
        // UINT.  This constructor is only used by DynArrayIA, which has a
        // C_ASSERT on the same condition.
        C_ASSERT(sizeof(T) <= UINT_MAX);
        Assert(allocSize <= UINT_MAX / sizeof(T));
    }

};


// DynArrayIA: A version of DynArray which encapsulates the initial allocation.
//
// For example:
//
// DynArrayIA<MyType, 10> array;
//
// This declares a DynArray of MyType objects, with an initial allocation of
// 10 elements. Such a declaration can be used on the stack or in another
// object.  The default contructor will be called for each element in the
// initial allocation.

template <typename T, UINT ALLOCSIZE, bool fZeroMemory = false>
class DynArrayIA : public DynArray<T, fZeroMemory>
{
public:
    // Constructor
    //

    DynArrayIA(void):
        DynArray<T, fZeroMemory>(InitialAllocationBuffer, ALLOCSIZE, 0)
    {
    }

private:

    // This C_ASSERT ensures that we can always represent the size of the initial
    // allocation buffer in a UINT.
    C_ASSERT((sizeof(T) <= UINT_MAX) && (ALLOCSIZE <= (UINT_MAX / sizeof(T))));

    T InitialAllocationBuffer[ALLOCSIZE];
};


// DynArrayIANoCtor: A version of DynArray which encapsulates the initial
//                   allocation, but doesn't call constructor for each array
//                   element
//
// For example:
//
// DynArrayIANoCtor<MyType, 10> array;
//
// This declares a DynArray of MyType objects, with an initial allocation of 10
// elements.  Such a declaration can be used on the stack or in another object.
// None of the MyType elements will be initialized.

template <typename T, UINT ALLOCSIZE, bool fZeroMemory = false>
class DynArrayIANoCtor : public DynArray<T, fZeroMemory>
{
public:

    // Constructor
    //

    DynArrayIANoCtor(void) :
        DynArray<T, fZeroMemory>(reinterpret_cast<T *>(InitialAllocationBuffer),
                                 ALLOCSIZE,
                                 0)
    {
    }

private:

    BYTE InitialAllocationBuffer[ALLOCSIZE*sizeof(T)];

};



