#pragma once

#ifndef _CHPTRARR_H
#define _CHPTRARR_H

#include <windows.h>
#include <provexce.h>

class CHPtrArray
{
public:
    CHPtrArray();
    ~CHPtrArray();

    int GetSize() const;
    int GetUpperBound() const;
    void SetSize(int nNewSize, int nGrowBy = -1) throw (CHeap_Exception);

    void FreeExtra() throw ( CHeap_Exception );
    void RemoveAll();

    void* GetAt(int nIndex) const;
    void SetAt(int nIndex, void* newElement);
    void*& ElementAt(int nIndex);

    const void** GetData() const;
    void** GetData();

    void SetAtGrow(int nIndex, void* newElement) throw ( CHeap_Exception );
    int Add(void* newElement) throw ( CHeap_Exception );
    int Append(const CHPtrArray& src) throw ( CHeap_Exception );
    void Copy(const CHPtrArray& src) throw ( CHeap_Exception );

    void* operator[](int nIndex) const;
    void*& operator[](int nIndex);

    void InsertAt(int nIndex, void* newElement, int nCount = 1) throw ( CHeap_Exception );
    void RemoveAt(int nIndex, int nCount = 1) ;
    void InsertAt(int nStartIndex, CHPtrArray* pNewArray) throw ( CHeap_Exception );

protected:
    void** m_pData;
    int m_nSize;
    int m_nMaxSize;
    int m_nGrowBy;
};

#endif
