// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------

//------------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//  CPtrMultisetBase class
//  (Implementation based on CPtrArray<T>, see ptrarray.h/cpp)
//
//  This class represents a set of pointer-sized elements.  The full state of
//  the class is stored by a single pointer-sized data member called m_data.
//  If there are no elements then m_data is zero.  If there is only one element
//  then m_data is the element itself.  If there are multiple elements then
//  m_data points to an array containing the elements.  The array also contains
//  the count of elements in the first position, the capacity of the array in
//  the second, the number of elements removed from the set since the last
//  compaction in the third (including a single bit indicating whether the set
//  is currently sorted), with the actual elements starting in position four.
//
//  The format of m_data also stores the storage state in the lower two bits.
//  If the bits are 00 then m_data is all zeroes and the array is empty.  If
//  they are 01 then there is only one element, and it is (m_data&~0x3).  If
//  they are 10 then there are multiple elements and (m_data&~0x3) points to
//  the raw data array.  Note that this means effectively only DWORD-aligned
//  pointers can be stored in this set.
//
//  This class is implemented as an unordered compacted array below a certain
//  threshold (PTRMULTISET_ARRAY_CUTOFF in ptrset.cpp), and as an unordered
//  sparse array above it. When elements are removed from the multi-set above
//  this cutoff, rather than compacting the remainder of the array right away,
//  the removed element is simply tagged as removed by setting the lower two
//  bits: (element[i] |= 0x3).  At a later time, if the set's storage array is
//  deemed too sparse, it will be compacted. Being deemed too sparse means
//  that the 'compaction factor' of the array has fallen below a predetermined
//  threshold.  The compaction factor is the ratio of the number of untagged
//  entries to the total number of entries (tagged and untagged).
//------------------------------------------------------------------------------

class CPtrMultisetBase
{
protected:
    //
    // Initialization and cleanup.
    // NOTE: the destructor is explicitly non-virtual.  This struct is intended
    // to be inlined into a parent struct and not allocated.  No virtual methods
    // mean we do not have a v-table pointer.
    //

    inline CPtrMultisetBase()
    {
        m_data = NULL;
        m_unsortedNewElements = 0;
    }

    inline ~CPtrMultisetBase()
    {
        Clear();
    }

    //
    // Functions to add and remove entries in the set.
    //

    HRESULT Add(__in_xcount(sizeof(T)) UINT_PTR p);
    bool Remove(__in_xcount(sizeof(T)) UINT_PTR p);
    bool Contains(__in_xcount(sizeof(T)) UINT_PTR p);

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

    void Sort();

    void CompactDataArray();


    //+-------------------------------------------------------------------------
    // It would be best if the following methods were private, but they can't
    // be since they are accessed by CPtrMultiset<T>::iterator.
    //--------------------------------------------------------------------------

    inline bool IsDataArray() const
    {
        return (m_data & 0x2) != 0;
    }

    inline bool IsDataSorted() const
    {
        return ((GetRawDataArray()[2] & 0x80000000) != 0);
    }

    inline static bool IsTaggedForRemoval(__in_xcount(sizeof(T)) UINT_PTR p)
    {
        return ((p & 0x3) == 0x3);
    }

    inline void SetIsDataSorted(bool isSorted)
    {
        if (isSorted)
        {
            GetRawDataArray()[2] |= 0x80000000;
        }
        else
        {
            GetRawDataArray()[2] &= ~0x80000000;
        }
    }

    inline double GetCompactionFactorFromArray() const
    {
        size_t cEntries = GetCount();
        return (cEntries - GetTaggedCountFromArray()) / static_cast<double>(cEntries);;
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
#ifdef DEBUG
        // In Debug we use an extra meta-element for array versioning.
        return GetRawDataArray() + 4;
#else
        return GetRawDataArray() + 3;
#endif
    }

    inline size_t GetCountFromArray() const
    {
        return static_cast<size_t>(GetRawDataArray()[0]);
    }

    inline size_t GetArrayAllocatedSize() const
    {
        return static_cast<size_t>(GetRawDataArray()[1]);
    }

    inline size_t GetTaggedCountFromArray() const
    {
        return static_cast<size_t>(GetRawDataArray()[2] & ~0x80000000);
    }

    //
    // Data
    //

    UINT_PTR m_data;

    //
    // Number of Add operations since the last Sort, used to determine sorting
    // algorithm
    //
    UINT m_unsortedNewElements;
    
#ifdef DEBUG
    inline void IncrementArrayVersion()
    {
        if (IsDataArray())
        {
            GetRawDataArray()[3]++;
        }
    }
#endif
};

template <typename T>
class CPtrMultiset : private CPtrMultisetBase
{
public:

    //+-------------------------------------------------------------------------
    //  Enumerator class
    //
    //  Allows the user to enumerate the contents of the set without needing
    //  to know about some elements being tagged as removed.
    //--------------------------------------------------------------------------

    class Enumerator
    {
        friend class CPtrMultiset<T>;

#if defined(DBG) || defined(DEBUG) || defined(_DEBUG)
        unsigned __int64 m_version;
#endif

    public:

        //+---------------------------------------------------------------------
        //  Enumerator::Current
        //
        //  Gets the element in the set at the current position of the enumerator.
        //+---------------------------------------------------------------------

        inline T* Current()
        {
            T* elt = NULL;

            if ((0 == m_currIndex) && (1 == m_count))
            {
                //
                // m_pElements points to m_data, remember to mask out the
                // lower two bits.
                //
                elt = reinterpret_cast<T*>((*m_pElements) & ~0x3);
            }
            else if (m_currIndex < m_count)
            {
            #ifdef DBG  // Suppresses PreFast warning about undeclared identifier 'm_version'    
                // In Debug we use the last meta-element before the elements
                // array for versioning.
                Assert(m_version == m_pElements[-1]);
            #endif
                //
                // m_pElements points to the elements array and we can
                // index into it.
                //
                Assert((m_pElements[m_currIndex] & 0x3) == 0x0);
                elt = reinterpret_cast<T*>(m_pElements[m_currIndex] & ~0x3);
            }

            return elt;
        }

        //+---------------------------------------------------------------------
        //  Enumerator::MoveNext
        //
        //  Sets the enumerator to its initial position, which is before the
        //  first element in the set.
        //+---------------------------------------------------------------------

        inline T* MoveNext()
        {
            T* elt = NULL;

            //
            // If m_currIndex == m_count, we've reached the end of the set and
            // we'll just return NULL.  Also, if m_currIndex == UINT_MAX, this
            // is the first call to MoveNext().
            //

            if ((m_count > 0) && ((m_currIndex < m_count) || (m_currIndex == UINT_MAX)))
            {
                //
                // Search for the next untagged element.
                //
                do
                {
                    m_currIndex++;

                    if ((m_currIndex == m_count) || (1 == m_count))
                    {
                        // Break here so we don't end up checking for tagged
                        // elements outside of our actual array.
                        break;
                    }
                } while (IsTaggedForRemoval(m_pElements[m_currIndex]));

                elt = Current();
            }

            return elt;
        }

        //+---------------------------------------------------------------------
        //  Enumerator::Reset
        //
        //  Sets the enumerator to its initial position, which is before the
        //  first element in the set.
        //+---------------------------------------------------------------------

        inline void Reset()
        {
            m_currIndex = UINT_MAX;
        }

    private:
    
        Enumerator(UINT_PTR *pElements, size_t count) :
            m_currIndex(UINT_MAX),
            m_pElements(pElements),
            m_count(count)
        {
#ifdef DEBUG        
            // In Debug we use the last meta-element before the elements
            // array for versioning.
            m_version = (m_count > 1) ? pElements[-1] : -1;
        }
#else
        }
#endif

        //
        // Data
        //
        UINT m_currIndex;
        const size_t m_count;
        const UINT_PTR *m_pElements;
    }; // Enumerator class

    //+-----------------------------------------------------------------------------
    //  CPtrMultiset<T>::GetEnumerator
    //
    //  Returns an Enumerator that iterates through the multi-set.
    //------------------------------------------------------------------------------

    inline Enumerator GetEnumerator()
    {
        UINT_PTR *pElements = NULL;
        UINT cEntries = static_cast<UINT>(CPtrMultisetBase::GetCount());

        if (cEntries == 1)
        {
            pElements = &m_data;
        }
        else if (cEntries > 1)
        {
            pElements = GetElementArray();
        }

        return Enumerator(pElements, cEntries);
    }

    //+-------------------------------------------------------------------------
    //  Non-Enumerator related CPtrMultiset<T> Methods
    //--------------------------------------------------------------------------

    inline HRESULT Add(__in T *p)
    {
        return CPtrMultisetBase::Add(reinterpret_cast<UINT_PTR>(p));
    }

    inline bool Remove(__in T *p)
    {
        return CPtrMultisetBase::Remove(reinterpret_cast<UINT_PTR>(p));
    }

    inline bool Contains(__in T *p)
    {
        return CPtrMultisetBase::Contains(reinterpret_cast<UINT_PTR>(p));
    }

    inline void Clear()
    {
        CPtrMultisetBase::Clear();
    }

    inline size_t GetCount() const
    {
        size_t cEntries = CPtrMultisetBase::GetCount();

        if (CPtrMultisetBase::IsDataArray())
        {
            cEntries -= CPtrMultisetBase::GetTaggedCountFromArray();
        }

        return cEntries;
    }
};

