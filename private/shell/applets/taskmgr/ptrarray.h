//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ptrarray.h
//
//  Contents:   Handles dynamic arrays of void *
//
//  History:    7-13-95  Davepl  Created
//
//--------------------------------------------------------------------------

class CPtrArray
{

public:

    //
    // Constructor / Destructor
    //

    CPtrArray();
    CPtrArray(HANDLE hHeap);
    virtual ~CPtrArray();

    //
    // Attributes
    //

    int     GetSize() const
    {
        return m_nSize;
    }

    int     GetUpperBound() const
    {
        return m_nSize-1;
    }

    BOOL    SetSize(int nNewSize, int nGrowBy = -1);

    BOOL    FreeExtra();
    BOOL    RemoveAll()
    {
        return SetSize(0);
    }

    void*   GetAt(int nIndex) const
    {
        ASSERT(nIndex >= 0 && nIndex < m_nSize);
        return m_pData[nIndex];
    }

    void    SetAt(int nIndex, void* newElement)
    {
        ASSERT(nIndex >= 0 && nIndex < m_nSize);
        m_pData[nIndex] = newElement;
    }

    void*&  ElementAt(int nIndex)
    {
        ASSERT(nIndex >= 0 && nIndex < m_nSize);
        return m_pData[nIndex];
    }

    // Direct Access to the element data (may return NULL)

    const void** GetData() const
    {
        return (const void**)m_pData;
    }

    void**  GetData()
    {
        return (void**)m_pData;
    }

    // Potentially growing the array

    BOOL SetAtGrow(int nIndex, void* newElement)
    {
        ASSERT(nIndex >= 0);

        if (nIndex >= m_nSize)
        {
            if (FALSE == SetSize(nIndex+1))
            {
                return FALSE;
            }
        }
        m_pData[nIndex] = newElement;

        return TRUE;
    }

    BOOL Add(void* newElement, int * pIndex = NULL)
    {
        if (pIndex)
        {
            *pIndex = m_nSize;
        }
        return SetAtGrow(m_nSize, newElement);
    }


    BOOL Append(const CPtrArray& src, int * pOldSize = NULL)
    {
        ASSERT(this != &src);   // cannot append to itself

        int nOldSize = m_nSize;

        if (FALSE == SetSize(m_nSize + src.m_nSize))
        {
            return TRUE;
        }

        CopyMemory(m_pData + nOldSize, src.m_pData, ((DWORD)src.m_nSize) * sizeof(void*));

        if (pOldSize)
        {
            *pOldSize = nOldSize;
        }

        return TRUE;
    }

    BOOL Copy(const CPtrArray& src)
    {
        ASSERT(this != &src);   // cannot append to itself

        if (FALSE == SetSize(src.m_nSize))
        {
            return FALSE;
        }

        CopyMemory(m_pData, src.m_pData, ((DWORD)src.m_nSize) * sizeof(void*));

        return TRUE;

    }

    // overloaded operator helpers

    void*   operator[](int nIndex) const
    {
        return GetAt(nIndex);
    }


    void*&  operator[](int nIndex)
    {
        return ElementAt(nIndex);
    }


    // Operations that move elements around

    BOOL InsertAt(int nIndex, void* newElement, int nCount = 1);
    BOOL InsertAt(int nStartIndex, CPtrArray* pNewArray);

    void RemoveAt(int nIndex, int nCount)
    {
        ASSERT(nIndex >= 0);
        ASSERT(nCount >= 0);
        ASSERT(nIndex + nCount <= m_nSize);

        // just remove a range
        int nMoveCount = m_nSize - (nIndex + nCount);

        if (nMoveCount)
        {
            CopyMemory(&m_pData[nIndex], &m_pData[nIndex + nCount], ((DWORD)(nMoveCount)) * sizeof(void*));
        }

        m_nSize -= nCount;
    }


// Implementation

protected:

    void**  m_pData;     // the actual array of data
    int     m_nSize;     // # of elements (upperBound - 1)
    int     m_nMaxSize;  // max allocated
    int     m_nGrowBy;   // grow amount
    HANDLE  m_hHeap;     // heap to allocate from
};

