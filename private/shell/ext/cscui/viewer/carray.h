//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       carray.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CARRAY_H
#define _INC_CSCVIEW_CARRAY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: carray.h

    Description: Template class CArray.  Implements a dynamic array class.

        Much of the functionality is based on the feature set of MFC's 
        CArray class.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/16/97    Initial creation.                                    BrianAu
    12/13/97    Changed SetAtGrow to return true/false.  True means  BrianAu
                had to grow array.
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_CSCVIEW_DEBUG_H
#   include "debug.h"
#endif
#ifndef _INC_CSCVIEW_THDSYNC_H
#   include "thdsync.h"
#endif

template <class T>
class CArray
{
    public:
        CArray<T>(VOID) throw();
        explicit CArray<T>(INT cItems);
        CArray<T>(const CArray<T>& rhs);
        CArray<T>& operator = (const CArray<T>& rhs);

        virtual ~CArray<T>(VOID) throw();

        VOID SetAt(const T& item, INT i);
        bool SetAtGrow(const T& item, INT i);
        T GetAt(INT i) const;
        VOID Insert(const T& item, INT i = -1);
        VOID Append(const T& item, INT i = -1);
        INT Find(const T& key);
        VOID Delete(INT i);

        T operator [] (INT i) const;
        T& operator [] (INT i);

        VOID Clear(VOID) throw();

        BOOL IsEmpty(VOID) const throw()
            { return 0 == m_cItems; }

        INT Count(VOID) const throw()
            { return m_cItems; }

        INT UpperBound(VOID) const throw()
            { return m_cItems - 1; }

        INT Size(VOID) const throw()
            { return m_cAlloc; }

        VOID SetSizeGrow(INT cGrow);

        VOID SetGrow(INT cGrow)
            { m_cGrow = cGrow; }

        VOID Copy(const CArray<T>& rhs);

        VOID Append(const CArray<T>& rhs);

        VOID SetSize(INT cEntries, INT iShift = -1);

        VOID BubbleSort(void);

        VOID Lock(VOID) throw()
            { m_cs.Enter(); }

        VOID ReleaseLock(VOID) throw()
            { m_cs.Leave(); }

    protected:
        static INT DEFGROW; // Default growth value.

    private:
        INT m_cAlloc;           // Number of entry allocations.
        INT m_cItems;           // Number of used entries.
        INT m_cGrow;
        T *m_rgItems;           // Array of entries.
        mutable CCriticalSection m_cs;  // For multi-threaded access.

        template <class T>
        const T&
        MAX(const T& a, const T& b)
        {
            return a > b ? a : b;
        }

        template <class T>
        const T&
        MIN(const T& a, const T& b)
        {
            return a < b ? a : b;
        }
};


template <class T>
INT CArray<T>::DEFGROW = 8;

template <class T>
CArray<T>::CArray(
    void
    ) throw()
      : m_cAlloc(0),
        m_cItems(0),
        m_cGrow(DEFGROW),
        m_rgItems(NULL)
{

}

template <class T>
CArray<T>::CArray(
    INT cItems
    ) : m_cAlloc(0),
        m_cItems(0),
        m_cGrow(DEFGROW),
        m_rgItems(NULL)
{
    SetSize(cItems);
    m_cItems = cItems;
}

template <class T>
CArray<T>::CArray(
    const CArray& rhs
    ) : m_cAlloc(0),
        m_cItems(0),
        m_cGrow(DEFGROW),
        m_rgItems(NULL)
{
    *this = rhs;
}

template <class T>
VOID
CArray<T>::Copy(
    const CArray<T>& rhs
    )
{
    AutoLockCs lock1(rhs.m_cs);
    AutoLockCs lock2(m_cs);

    //
    // Place *this in an empty state in case Grow() throws an exception.
    // It should still be a valid CArray object.
    //
    delete[] m_rgItems;
    m_rgItems = NULL;
    m_cAlloc  = 0;
    m_cItems  = 0;

    //
    // Size the object to hold the source array.
    //
    SetSize(rhs.m_cAlloc);

    //
    // Copy the contents.
    //
    DBGASSERT((m_cAlloc >= rhs.m_cItems));
    for (m_cItems = 0; m_cItems < rhs.m_cItems; m_cItems++)
    {
        //
        // This assignment could throw an exception so only update
        // our item count after each successful copy.
        //
        DBGASSERT((m_cItems < m_cAlloc));
        m_rgItems[m_cItems] = rhs.m_rgItems[m_cItems];
    }
}


template <class T>
VOID 
CArray<T>::Append(
    const CArray<T>& rhs
    )
{
    AutoLockCs lock1(rhs.m_cs);
    AutoLockCs lock2(m_cs);

    //
    // Size the object to hold both arrays.
    //
    SetSize(m_cAlloc + rhs.m_cItems);

    //
    // Append the contents.
    //
    DBGASSERT((m_cAlloc >= (m_cItems + rhs.m_cItems)));
    for (int i = 0; i < rhs.m_cItems; i++)
    {
        DBGASSERT((m_cItems < m_cAlloc));
        m_rgItems[m_cItems++] = rhs.m_rgItems[i];
    }
}


template <class T>
CArray<T>& 
CArray<T>::operator = (
    const CArray<T>& rhs
    ) 
{
    if (this != &rhs)
    {
        Copy(rhs);
    }
    return *this;
}


template <class T>
CArray<T>::~CArray(
    VOID
    ) throw()
{
    Clear();
}



template <class T>
T CArray<T>::operator [] (
    INT i
    ) const
{
    return GetAt(i);
}


template <class T>
T& CArray<T>::operator [] (
    INT i
    )
{
    AutoLockCs lock(m_cs);

    if (i < 0 || i >= m_cItems)
        throw CException(ERROR_INVALID_INDEX);

    return *(m_rgItems + i);
}


template <class T>
VOID
CArray<T>::Clear(
    VOID
    ) throw()
{
    AutoLockCs lock(m_cs);
    delete[] m_rgItems;
    m_rgItems = NULL;
    m_cAlloc  = 0;
    m_cItems  = 0;
}

template <class T>
VOID
CArray<T>::Insert(
    const T& item, 
    INT i
    )
{
    AutoLockCs lock(m_cs);

    if (-1 == i)
    {
        //
        // Insert at head of array.
        //
        i = 0;
    }
    //
    // Can only insert an item before an existing item.
    //      i cannot be negative.
    //      If array is empty, i can only be 0.
    //      If array is not empty, i must be index of a valid item.
    //
    if ((0 == m_cItems && 0 != i) ||
        (0 != m_cItems && (i < 0 || i >= m_cItems)))
    {
        throw CException(ERROR_INVALID_INDEX);
    }

    DBGASSERT((m_cItems <= m_cAlloc));
    if (m_cItems >= m_cAlloc)
    {
        //
        // Grow the array if necessary.
        // This will also shift the elements, beginning with element 'i',
        // one element to the right.
        //
        SetSize(m_cAlloc + m_cGrow, i);
    }
    else
    {
        //
        // Growth not necessary.
        // Shift the contents of the array following the insertion point
        // one element to the right.
        //
        for (int j = m_cItems; j > i; j--)
        {
            m_rgItems[j] = m_rgItems[j-1];
        }
    }
    //
    // We've now inserted an item.
    //
    m_cItems++;
    //
    // Set the value at the inserted location.
    // This assignment could throw an exception.
    //
    SetAt(item, i);
}


template <class T>
VOID
CArray<T>::Append(
    const T& item,
    INT i
    )
{
    AutoLockCs lock(m_cs);

    if (-1 == i)
    {
        //
        // Append at end of array.
        //
        i = m_cItems - 1;
    }
    //
    // Can only append an item after an existing item.
    //      When array is empty, i can only be -1.
    //      When array is not empty, i must be index of a valid item.
    //       
    // Note: i will be -1 when m_cItems is 0.
    //
    if ((0 == m_cItems && -1 != i) ||
        (0 != m_cItems && (i < 0 || i >= m_cItems)))
    {
        throw CException(ERROR_INVALID_INDEX);
    }

    DBGASSERT((m_cItems <= m_cAlloc));
    if (m_cItems >= m_cAlloc)
    {
        //
        // Grow the array if necessary.
        // This will also shift the elements, beginning with element 'i + 1',
        // one element to the right.
        //
        SetSize(m_cAlloc + m_cGrow, i+1);
    }
    else
    {
        //
        // Shift the contents of the array following the insertion
        // point, one entry to the right.
        //
        for (int j = m_cItems; j > (i+1); j--)
        {
            m_rgItems[j] = m_rgItems[j-1];
        }
    }
    //
    // We've now appended an item.
    //
    m_cItems++;
    //
    // Set the value at the appended location.
    // This assignment could throw an exception.
    //
    SetAt(item, i+1);
}


template <class T>
VOID
CArray<T>::Delete(
    INT i
    )
{
    AutoLockCs lock(m_cs);

    //
    // Can only delete a valid item.
    //
    if (i < 0 || i >= m_cItems)
        throw CException(ERROR_INVALID_INDEX);
    //
    // Shift memory to remove the item.
    //

    for (int j = i; j < m_cItems; j++)
    {
        m_rgItems[j] = m_rgItems[j+1];
    }
    //
    // Now we have one less item.
    //
    m_cItems--;
    //
    // Shrink the array if it's required size is less than 2X the
    // array's "growth" amount.
    //
    if ((m_cAlloc - m_cItems) > (2 * m_cGrow))
    {
        SetSize(m_cItems);
    }
}

template <class T>
INT
CArray<T>::Find(
    const T& key
    )
{
    AutoLockCs lock(m_cs);

    for (INT i = 0; i < m_cItems; i++)
    {
        if (m_rgItems[i] == key)
        {
            return i;
        }
    }
    return -1;
}


template <class T>
T
CArray<T>::GetAt(
    INT i
    ) const
{
    AutoLockCs lock(m_cs);

    if (i < 0 || i >= m_cItems)
        throw CException(ERROR_INVALID_INDEX);

    return m_rgItems[i];
}


template <class T>
VOID
CArray<T>::SetAt(
    const T& item,
    INT i
    )
{
    AutoLockCs lock(m_cs);

    if (i < 0 || i >= m_cAlloc)
        throw CException(ERROR_INVALID_INDEX);

    m_rgItems[i] = item;
}


//
// Returns:  true = array was extended, false = no extension required.
//
template <class T>
bool
CArray<T>::SetAtGrow(
    const T& item,
    INT i
    )
{
    bool bGrow = false;
    AutoLockCs lock(m_cs);

    if (i >= m_cAlloc)
    {
        //
        // Need to grow the array to accomodate the new item.
        //
        SetSize(i + m_cGrow);
        bGrow = true;
    }
    //
    // Set the new item value.
    //
    SetAt(item, i);
    //
    // Extend the count of "valid" items.
    //
    m_cItems = i + 1;

    return bGrow;
}


template <class T>
VOID
CArray<T>::SetSize(
    INT cEntries,
    INT iShift          // Pass -1 for "no shift".
    )
{
    AutoLockCs lock(m_cs);

    //
    // Don't allow an array of less than 1 element.
    //
    cEntries = MAX(1, cEntries);

    T *pNew = new T[cEntries];
 
    if (NULL != m_rgItems)
    {
        INT cCopy = MIN(cEntries, m_cItems);
        INT j = 0;
        for (INT i = 0; i < cCopy; i++, j++)
        {
            //
            // Shift items [i..(n-1)] to [(i+1)..n]
            //
            if (iShift == j)
                j++;

            *(pNew + j) = m_rgItems[i];
        }
    }
    delete[] m_rgItems;
    m_rgItems = pNew;
    m_cAlloc  = cEntries;
}


template <class T>
void
CArray<T>::BubbleSort(
    void
    )
{
    int n = Count();
    if (1 < n)
    {
        int swaps;
        do
        {
            swaps = 0;
            for (int i = 0; i < (n - 1); i++)
            {
                T& t1 = this->operator[](i);
                T& t2 = this->operator[](i+1);
                if (t2 < t1)
                {
                    SWAP(t1, t2);
                    swaps++;
                }
            }
        }
        while(0 < swaps);
    }
}




template <class T>
class CQueueAsArray : public CArray<T>
{
    public:
        CQueueAsArray<T>(VOID) { }
        ~CQueueAsArray<T>(VOID) { }

        VOID Add(T& item);
        BOOL Remove(T& item);

    private:
        CQueueAsArray<T>(const CQueueAsArray<T>& rhs);
        CQueueAsArray<T>& operator = (const CQueueAsArray<T>& rhs);

};


template <class T>
VOID
CQueueAsArray<T>::Add(
    T& item
    )
{
    Append(item);
}

template <class T>
BOOL
CQueueAsArray<T>::Remove(
    T& item
    )
{
    BOOL bResult = FALSE;
    if (!IsEmpty())
    {
        INT i = UpperBound();
        item = GetAt(i);
        Delete(i);
        bResult = TRUE;
    }
    return bResult;
}






#endif // _INC_CSCVIEW_CARRAY_H

