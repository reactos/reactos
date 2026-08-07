// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

#pragma once

//
// This class represents an array of pointer-sized elements with compaction. The
// full state of the class is stored by a single pointer-sized data member called
// m_data. If there are no elements then m_data is zero. If there is only one
// element then m_data is the element itself. If there are multiple elements then
// m_data points to an array containing the elements. The array also contains
// the count of elements in the first position and the capacity of the array in
// the second, with the actual elements starting in the third position.
//
// The format of m_data also stores the storage state in the lower two bits. If
// the bits are 00 then m_data is all zeroes and the array is empty. If they are
// 01 then there is only one element, and it is (m_data&~0x3). If they are 10 then
// there are multiple elements and (m_data&~0x3) points to the storage array. Note
// that this means that effectively only DWORD-aligned pointers can be stored in
// this array.
//

class CPtrArrayBase
{
protected:
    //
    // Initialization and cleanup.
    // NOTE: the destructor is explicitly non-virtual. This struct is intended
    // to be inlined into a parent struct and not allocated. No virtual
    // methods mean we do not have a v-table pointer.
    //

    inline CPtrArrayBase()
    {
        m_data = 0;
    }

    inline ~CPtrArrayBase()
    {
        Clear();
    }

    //
    // Functions to add and remove entries in the array.
    //

    HRESULT InsertAt(__in_xcount(sizeof(T)) UINT_PTR p, size_t index);
    bool Remove(__in_xcount(sizeof(T)) UINT_PTR p);
    __out_xcount_opt(sizeof(T)) UINT_PTR operator[](size_t index) const;

    void Clear();

    inline size_t GetCount() const
    {
        //
        // there are three cases to consider:
        //  0 elements : m_data is 0x00000000
        //  1 element  : m_data is 0xnnnnnnn1
        //  >1 elements: m_data is 0xnnnnnnn2
        //
        // in other words, bit 1 is on if there is more than
        // one element, off otherwise. If it is off then bit 0
        // differentiates between the 0- and 1-element cases.
        // If there are multiple elements then the count is
        // stored in the first UINT_PTR in the allocated array.
        //

        return IsDataArray() ? GetCountFromArray() : static_cast<size_t>(m_data & 0x1);
    }

private:
    inline bool IsDataArray() const
    {
        return (m_data & 0x2) != 0;
    }

    inline __out_xcount(sizeof(T)) UINT_PTR GetSingletonValue() const
    {
        Assert(!IsDataArray());
        return m_data & ~0x3;
    }

    inline __out_xcount(sizeof(T)) UINT_PTR *GetRawDataArray() const
    {
        Assert(IsDataArray());
        return reinterpret_cast<UINT_PTR*>(m_data & ~0x3);
    }

    inline __out_xcount(sizeof(T)) UINT_PTR *GetElementArray() const
    {
        return GetRawDataArray() + 2;
    }

    inline size_t GetCountFromArray() const 
    {
        return static_cast<size_t>(GetRawDataArray()[0]);
    }

    inline size_t GetArrayAllocatedSize() const
    {
        return static_cast<size_t>(GetRawDataArray()[1]);
    }

    //
    // Data
    //

    UINT_PTR m_data;
};

template <typename T>
class CPtrArray : private CPtrArrayBase
{
public:

    // ----------------------------------------------------------------------
    // Add
    //  Appends an element to the end of the array.
    //
    // Parameters
    //  p           The element to add
    //
    // Return value
    //  S_OK            The operation succeeded
    //  E_INVALIDARG    The parameter is not divisible by four
    //  E_OUTOFMEMORY   There isn't enough memory to store the new element
    // ----------------------------------------------------------------------

    inline HRESULT Add(__in_ecount(1) T *p)
    {
        return CPtrArrayBase::InsertAt(reinterpret_cast<UINT_PTR>(p), CPtrArrayBase::GetCount());
    }

    // ----------------------------------------------------------------------
    // InsertAt
    //  Inserts an element into the array.
    //
    // Parameters
    //  p           The element to add
    //  index       The index of the element, in the range 0<=index<=GetCount()
    //
    // Return value
    //  S_OK            The operation succeeded
    //  E_INVALIDARG    The parameter is not divisible by four or the index
    //                  is out of range
    //  E_OUTOFMEMORY   There isn't enough memory to store the new element
    // ----------------------------------------------------------------------

    inline HRESULT InsertAt(__in_ecount(1) T *p, size_t index)
    {
        return CPtrArrayBase::InsertAt(reinterpret_cast<UINT_PTR>(p), index);
    }

    // ----------------------------------------------------------------------
    // Remove
    //  Removes the first occurrence of an element from the array.
    //
    // Parameters
    //  p           The element to remove
    //
    // Return value
    //  true        The element was found and removed
    //  false       The element was not found in the array
    // ----------------------------------------------------------------------

    inline bool Remove(__in_ecount(1) T *p)
    {
        return CPtrArrayBase::Remove(reinterpret_cast<UINT_PTR>(p));
    }

    // ----------------------------------------------------------------------
    // operator[]
    //  Retrieves an element from the array.
    //
    // Parameters
    //  index       The index of the element to retrieve
    //
    // Return value
    //  p           The element at the specified index if index is in the
    //              range 0<=index<GetCount()
    //  NULL        The index was out of range
    // ----------------------------------------------------------------------

    inline __out_ecount_opt(1) T *operator[](size_t index)
    {
        return reinterpret_cast<T *>(CPtrArrayBase::operator[](index));
    }

    // ----------------------------------------------------------------------
    // Clear
    //  Removes all elements from the array. This can never fail
    // ----------------------------------------------------------------------

    inline void Clear()
    {
        CPtrArrayBase::Clear();
    }

    // ----------------------------------------------------------------------
    // GetCount
    //  Gets the number of elements in the array.
    //
    // Return value
    //  The number of elements in the array.
    // ----------------------------------------------------------------------

    inline size_t GetCount() const
    {
        return CPtrArrayBase::GetCount();
    }
};

