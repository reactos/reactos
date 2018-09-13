//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ptrarray.cpp
//
//  Contents:   Handles dynamic arrays of void *.  Stolen from MFC
//
//  History:    7-13-95  Davepl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"

//
// Default contstructor just invokes normal constructor, using the
// current process' heap as the heap handle
//

CPtrArray::CPtrArray()
{
    CPtrArray::CPtrArray(GetProcessHeap());
}

//
// Constructor save a handle to the heap supplied to use for future 
// allocations
//

CPtrArray::CPtrArray(HANDLE hHeap)
{
    m_hHeap     = hHeap;
    m_pData     = NULL;
    m_nSize     = 0;
    m_nMaxSize  = 0;
    m_nGrowBy   = 0;
}

CPtrArray::~CPtrArray()
{
    HeapFree(m_hHeap, 0, m_pData);
}

BOOL CPtrArray::SetSize(int nNewSize, int nGrowBy)
{
    ASSERT(nNewSize >= 0);

    //
    // Set the new size
    //

    if (nGrowBy != -1)
    {
        m_nGrowBy = nGrowBy;
    }

    if (nNewSize == 0)
    {
        //
        // Shrink to nothing
        //

        VERIFY( HeapFree(m_hHeap, 0, m_pData) );
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        //
        // Data array doesn't exist yet, allocate it now
        //

        LPVOID * pnew = (LPVOID *) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, nNewSize * sizeof(void*));

        if (pnew)
        {
            m_pData     = pnew;
            m_nSize     = nNewSize;
            m_nMaxSize  = nNewSize;
        }
        else
        {
            return FALSE;
        }
    }
    else if (nNewSize <= m_nMaxSize)
    {
        //
        // It fits
        //

        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            ZeroMemory(&m_pData[m_nSize], (nNewSize-m_nSize) * sizeof(void*));
        }

        m_nSize = nNewSize;
    }
    else
    {
        //
        //  It doesn't fit: grow the array
        //

        m_nGrowBy = nGrowBy;        // BUGBUG verify this
        if (nGrowBy == 0)
        {
            //
            // Heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            //

            nGrowBy = min(1024, max(4, m_nSize / 8));
        }

        int nNewMax;

        if (nNewSize < m_nMaxSize + nGrowBy)
        {
            nNewMax = m_nMaxSize + nGrowBy;     // granularity
        }
        else
        {
            nNewMax = nNewSize;                 // no slush
        }

        ASSERT(nNewMax >= m_nMaxSize);          // no wrap around

        LPVOID * pNewData = (LPVOID *) HeapReAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_pData, nNewMax * sizeof(void*));

        if (NULL == pNewData)
        {
            return FALSE;
        }

        ASSERT(nNewSize > m_nSize);

        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }

    return TRUE;
}

BOOL CPtrArray::FreeExtra()
{

    if (m_nSize != m_nMaxSize)
    {
        //
        // shrink to desired size
        //
        void** pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (void**) HeapAlloc(m_hHeap, 0, m_nSize * sizeof(void*));
            ASSERT(pNewData);

            if (NULL == pNewData)
            {
                return FALSE;
            }

            //
            // copy new data from old
            //

            CopyMemory(pNewData, m_pData, m_nSize * sizeof(void*));
        }

        //
        // get rid of old stuff (note: no destructors called)
        //

        VERIFY( HeapFree(m_hHeap, 0, m_pData) );
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }

    return TRUE;
}


BOOL CPtrArray::InsertAt(int nIndex, void* newElement, int nCount)
{
    ASSERT(nIndex >= 0);    // will expand to meet need
    ASSERT(nCount > 0);     // zero or negative size not allowed

    if (nIndex >= m_nSize)
    {
        //
        // adding after the end of the array
        //

        if (FALSE == SetSize(nIndex + nCount))  // grow so nIndex is valid
        {
            return FALSE;
        }
    }
    else
    {
        //
        // inserting in the middle of the array
        //

        int nOldSize = m_nSize;

        if (FALSE == SetSize(m_nSize + nCount))  // grow it to new size
        {
            return FALSE;
        }

        //
        // shift old data up to fill gap
        //

        MoveMemory(&m_pData[nIndex+nCount], &m_pData[nIndex], (nOldSize-nIndex) * sizeof(void*));

        // re-init slots we copied from

        ZeroMemory(&m_pData[nIndex], nCount * sizeof(void*));

    }

    // insert new value in the gap

    ASSERT(nIndex + nCount <= m_nSize);

    while (nCount--)
    {
        m_pData[nIndex++] = newElement;
    }

    return TRUE;
}

BOOL CPtrArray::InsertAt(int nStartIndex, CPtrArray* pNewArray)
{
    ASSERT(pNewArray != NULL);
    ASSERT(nStartIndex >= 0);

    if (pNewArray->GetSize() > 0)
    {
        if (FALSE == InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize()))
        {
            return FALSE;
        }

        for (int i = 0; i < pNewArray->GetSize(); i++)
        {
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
        }
    }

    return TRUE;
}



