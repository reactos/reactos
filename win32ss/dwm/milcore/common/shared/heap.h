// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Templated classes for quick sorting of abstract
//      elements by binary tree technique.
//
//  Classes:   CHeap
//
//------------------------------------------------------------------------------

#pragma once

#define NULL_INDEX 0

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHeap
//
//  Synopsis:
//      Holds a number of abstract elements of type TElement in a form of binary tree.
//      Elements are sorted so that parent node is greater than both children.
//      This means that the element at the root of the tree is always the
//      greatest element in the heap.
//
//      There are no ordering assumptions between subtrees and siblings:
//      left may be greater than right or vice versa.
//
//      All the elements of the tree are stored in a single solid linear array,
//      named "m_elements". Following rules define the layout of the tree:
//
//          m_elements[0] is never used.
//          m_elements[1] is the root of the tree.
//          m_elements[i/2] is the parent of m_elements[i].
//
//------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity = 0>
class CHeap
{
public:

    CHeap()
    {
        // Add an empty entry so our indices start at 1. This makes the arithmetic nicer.
        m_elements.SetCount(1);
    };

    // Copy constructor
    CHeap(__in_ecount(1) const CHeap & oldHeap)
        : m_elements(oldHeap.m_elements) {}

    ~CHeap() {};


    TElement GetTopElement() const;

    void Pop();

    HRESULT InsertElement(TElement element); 

    void Remove(TElement element);

    UINT GetCount() const { return m_elements.GetCount() - 1; }
    bool IsEmpty() const { return GetCount() == 0; }

    __outro_ecount(1) const TElement & operator[](UINT index) const { return m_elements[index+1]; }

#if DBG
    void Validate(UINT ignore = 0) const;

    void Dump() const;

    bool Includes(TElement element) const;
#endif

private:

    UINT BubbleUp(UINT index);

    void PushDown(UINT index);

    void Swap(UINT index1, UINT index2);

    void RemoveByIndex(UINT index);

    bool GreaterThan(UINT index1, UINT index2) const
    {
        return m_elements[index1].IsGreaterThan(m_elements[index2]);
    }

    DynArrayIANoCtor<TElement, uInitialCapacity+1> m_elements; // +1, since we don't use the first element

}; // End of definition of CHeap

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::BubbleUp(UINT index)
//
//  Synopsis:   Move element stored at index "index" up in the heap until it is no longer
//              greater than its parent. Returns the new location of the element.
//
//--------------------------------------------------------------------------------------------------

template <class TElement, UINT uInitialCapacity>
UINT
CHeap<TElement, uInitialCapacity>::BubbleUp(UINT index)
{
    UINT i = index;

    while ( (i > 1) && GreaterThan(i, i/2) )
    {
        Swap(i, i/2);

        i /= 2;
    }

    return i;
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::PushDown(UINT index)
//
//  Synopsis:   Move element stored at index "index" down the heap until it is no longer
//              less than either of its children.
//
//--------------------------------------------------------------------------------------------------

template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::PushDown(UINT index)
{
    UINT i = index;
    UINT end = m_elements.GetCount();

    while (i < end)
    {
        UINT newI = end;

        bool fLeftGreater = (2*i < end) && GreaterThan(2*i, i);
        bool fRightGreater = (2*i+1 < end) && GreaterThan(2*i+1, i);

        if (fLeftGreater && !fRightGreater)
        {
            Swap(2*i, i);
            newI = 2*i;
        }
        else if (!fLeftGreater && fRightGreater)
        {
            Swap(2*i+1, i);
            newI = 2*i+1;
        }
        else if (fLeftGreater && fRightGreater)
        {
            if (GreaterThan(2*i, 2*i+1))
            {
                Swap(2*i, i);
                newI = 2*i;
            }
            else
            {
                Swap(2*i+1, i);
                newI = 2*i+1;
            }
        }

        i = newI;
    }
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Swap(UINT index1, UINT index2)
//
//  Synopsis:   Swap the two elements stored at index1 and index2
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::Swap(UINT index1, UINT index2)
{
    __if_exists(TElement::SetIndex)
    {
        m_elements[index1].SetIndex(index2);
        m_elements[index2].SetIndex(index1);
    }

    TElement tmp = m_elements[index1];
                   m_elements[index1] = m_elements[index2];
                                        m_elements[index2] = tmp;
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::GetTopElement()
//
//  Synopsis:   Returns the highest chain or null if the heap is empty.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
TElement 
CHeap<TElement, uInitialCapacity>::GetTopElement() const
{
    Assert(!IsEmpty());

    return m_elements[1];
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Pop()
//
//  Synopsis:   Pops the top element from the heap. It is an error to call this if the heap is empty.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::Pop()
{
    Assert(m_elements.GetCount() > 1);

    RemoveByIndex(1);
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::RemoveByIndex(UINT index)
//
//  Synopsis:   Removes the element at a given index from the heap.
//              It is an error to call this if the heap is empty.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::RemoveByIndex(UINT index)
{
    UINT lastIndex = m_elements.GetCount()-1;
    UINT newIndex;

    Assert(index > 0 && index < m_elements.GetCount());

    __if_exists(TElement::SetIndex)
    {
        m_elements[index].SetIndex(NULL_INDEX);
    }

    if (index < lastIndex)
    {
        // Remove the last guy from the heap
        m_elements[index] = m_elements[lastIndex];

        __if_exists(TElement::SetIndex)
        {
            m_elements[index].SetIndex(index);
        }

        VerifySUCCEEDED(m_elements.RemoveAt(lastIndex));

        // Try bubbling him up.
        newIndex = BubbleUp(index);
        if (newIndex == index)
        {
            // Bubbling up didn't go anywhere. Maybe we need to push down instead.
            PushDown(newIndex);
        }
    }
    else
    {
        VerifySUCCEEDED(m_elements.RemoveAt(lastIndex));
    }
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Remove(TElement element)
//
//  Synopsis:   Removes element from the heap. 
//
//  Notes:      It is an error to call this if the heap is empty.
//              This method is only available if TElement::GetIndex() is defined.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::Remove(TElement element)
{
    UINT index = element.GetIndex();

    Assert(m_elements[index] == element);

    RemoveByIndex(index);
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::InsertElement(TElement element)
//
//  Synopsis:   Inserts element into the heap.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
HRESULT
CHeap<TElement, uInitialCapacity>::InsertElement(TElement element)
{
    HRESULT hr = S_OK;

    __if_exists(TElement::GetIndex)
    {
        Assert(element.GetIndex() == NULL_INDEX);
    }

    // First, append the entry on to the end of the heap.
    IFC(m_elements.Add(element));

    UINT lastIndex = m_elements.GetCount()-1;

    __if_exists(TElement::SetIndex)
    {
        element.SetIndex(lastIndex);
    }

    BubbleUp(lastIndex);

Cleanup:
    RRETURN(hr);
}

#if DBG

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Dump.
//
//  Synopsis:   Dump the contents of the heap.
//              This method is only available if TElement::Dump() is defined.
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::Dump() const
{
    for (UINT i = 1; i < m_elements.GetCount(); ++i)
    {
        MILDebugOutput(L"Element %d:\n", i);

        m_elements[i].Dump();
    }
}


//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Validate.
//
//  Synopsis:   Validate the heap, ignoring element "ignore"
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
void
CHeap<TElement, uInitialCapacity>::Validate(UINT ignore) const
{
    UINT count = m_elements.GetCount();

    for (UINT i = 1; i < count; ++i)
    {
        if (i != ignore)
        {
            if (2*i < count && 2*i != ignore && GreaterThan(2*i, i))
            {
                MILDebugOutput(L"CHeap::Validate() failed, comparing %d and %d\n", 2*i, i);
                Dump();
                Assert(false);
            }
            else if (2*i+1 < count && 2*i+1 != ignore && GreaterThan(2*i+1, i))
            {
                MILDebugOutput(L"CHeap::Validate() failed, comparing %d and %d\n", 2*i+1, i);
                Dump();
                Assert(false);
            }
        }
    }
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CHeap::Includes
//
//  Synopsis:   Check if the heap includes a given entry
//
//  Notes:      A debugging utility     
//
//--------------------------------------------------------------------------------------------------
template <class TElement, UINT uInitialCapacity>
bool
CHeap<TElement, uInitialCapacity>::Includes(TElement element) const
{
    for (UINT i = 1; i < m_elements.GetCount(); ++i)
    {
        if (m_elements[i] == element)
        {
            return true;
        }
    }

    return false;
}

#endif // DBG



