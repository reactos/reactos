/*
* PROJECT:     ReactOS system libraries
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     CHPtrArray class implementation
* COPYRIGHT:   Copyright 2018 Jin Juanshi (2033537949@qq.com)
*/

#include <chptrarr.h>

#define GROW_BY_MAX 256

CHPtrArray::CHPtrArray()
{
    m_pData = NULL;
    m_nSize = 0;
    m_nMaxSize = 0;
    m_nGrowBy = 0;
}

CHPtrArray::~CHPtrArray()
{
    if (m_pData)
    {
        free(m_pData);
    }
}

int CHPtrArray::GetSize() const
{
    return m_nSize;
}

int CHPtrArray::GetUpperBound() const
{
    return m_nSize - 1;
}

void CHPtrArray::SetSize(int nNewSize, int nGrowBy) throw (CHeap_Exception)
{
    if (nGrowBy != -1)
    {
        m_nGrowBy = nGrowBy;
    }

    if (nNewSize == 0)
    {
        if (m_pData)
        {
            free(m_pData);
        }
        m_pData = NULL;
        m_nMaxSize = 0;
        m_nSize = 0;
        return;
    }

    if (m_pData == NULL)
    {
        m_pData = (void**)malloc(nNewSize * sizeof(void*));
        m_nMaxSize = nNewSize;
        m_nSize = nNewSize;
        return;
    }

    if (nNewSize <= m_nMaxSize)
    {
        if (nNewSize > m_nSize)
        {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(void*));
            m_nSize = nNewSize;
            return;
        }
    }

    unsigned int uNewGrowBy = m_nGrowBy;
    if (uNewGrowBy == 0)
    {
        uNewGrowBy = m_nSize / 8;

        if (uNewGrowBy < sizeof(void*))
            uNewGrowBy = sizeof(void*);

        if (uNewGrowBy > GROW_BY_MAX * sizeof(void*))
            uNewGrowBy = GROW_BY_MAX * sizeof(void*);
    }

    int nNewMaxSize = m_nMaxSize + uNewGrowBy;
    if (nNewSize >= nNewMaxSize)
    {
        nNewMaxSize = nNewSize;
    }

    void** pNewData = (void**)malloc(nNewMaxSize * sizeof(void*));
    memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
    memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(void*));
    free(m_pData);

    m_pData = pNewData;
    m_nSize = nNewSize;
    m_nMaxSize = nNewMaxSize;
}

void CHPtrArray::FreeExtra() throw (CHeap_Exception)
{
    if (m_nSize != m_nMaxSize)
    {
        void **pNewData = NULL;

        if (m_nSize)
        {
            pNewData = (void**)malloc(m_nSize * sizeof(void*));
            memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
        }

        if (m_pData)
        {
            free(m_pData);
        }
        
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

void CHPtrArray::RemoveAll()
{
    SetSize(0);
}

void* CHPtrArray::GetAt(int nIndex) const
{
    return m_pData[nIndex];
}

void CHPtrArray::SetAt(int nIndex, void* newElement)
{
    m_pData[nIndex] = newElement;
}

void*& CHPtrArray::ElementAt(int nIndex)
{
    return m_pData[nIndex];
}

const void** CHPtrArray::GetData() const
{
    return (const void**)m_pData;
}

void** CHPtrArray::GetData()
{
    return m_pData;
}

void CHPtrArray::SetAtGrow(int nIndex, void* newElement) throw (CHeap_Exception)
{
    if (nIndex >= m_nSize)
    {
        SetSize(nIndex + 1);
    }
    m_pData[nIndex] = newElement;
}

int CHPtrArray::Add(void* newElement) throw (CHeap_Exception)
{
    SetAtGrow(m_nSize, newElement);
    return m_nSize - 1;
}

int CHPtrArray::Append(const CHPtrArray& src) throw (CHeap_Exception)
{
    int nOldSize = m_nSize;
    SetSize(m_nSize + src.m_nSize);
    memcpy(&m_pData[nOldSize], src.m_pData, src.m_nSize * sizeof(void*));
    return nOldSize;
}

void CHPtrArray::Copy(const CHPtrArray& src) throw (CHeap_Exception)
{
    SetSize(src.m_nSize);
    memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(void*));
}

void* CHPtrArray::operator[](int nIndex) const
{
    return m_pData[nIndex];
}

void*& CHPtrArray::operator[](int nIndex)
{
    return m_pData[nIndex];
}

void CHPtrArray::InsertAt(int nIndex, void* newElement, int nCount) throw (CHeap_Exception)
{
    if (nIndex < m_nSize)
    {
        SetSize(m_nSize + nCount);
        memmove(&m_pData[nIndex + nCount], &m_pData[nIndex], (m_nSize - nIndex) * sizeof(void*));
        memset(&m_pData[nIndex], 0, nCount * sizeof(void*));
    }
    else
    {
        SetSize(nIndex + nCount);
    }

    for (int i = 0; i < nCount; i++)
    {
        m_pData[nIndex + i] = newElement;
    }
}

void CHPtrArray::RemoveAt(int nIndex, int nCount)
{
    if (m_nSize > nIndex + nCount)
    {
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount], (m_nSize - nIndex - nCount) * sizeof(void*));
    }
    m_nSize -= nCount;
}

void CHPtrArray::InsertAt(int nStartIndex, CHPtrArray* pNewArray) throw (CHeap_Exception)
{
    if (pNewArray->m_nSize > 0)
    {
        InsertAt(nStartIndex, pNewArray->m_pData[0], pNewArray->m_nSize);

        for (int i = 1; i < pNewArray->m_nSize; i++)
        {
            m_pData[nStartIndex + i] = pNewArray->m_pData[i];
        }
    }
}
