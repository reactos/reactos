/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero dynamic array
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

class CicArray
{
protected:
    LPBYTE m_pb;
    size_t m_cItems;
    size_t m_cbItem;
    size_t m_cCapacity;

public:
    CicArray(size_t cbItem);
    virtual ~CicArray();

    BOOL Insert(size_t iItem, size_t cGrow);
    LPVOID Append(size_t cGrow);
    void Remove(size_t iItem, size_t cRemove);
};

template <typename T_ITEM>
class CicTypedArray : protected CicArray
{
public:
    CicTypedArray() : CicArray(sizeof(T_ITEM))
    {
    }

    T_ITEM* data() const
    {
        return (T_ITEM*)m_pb;
    }
    size_t size() const
    {
        return m_cItems;
    }
    bool empty() const
    {
        return !size();
    }

    T_ITEM& get_at(size_t iItem)
    {
        return *(T_ITEM*)&m_pb[iItem * m_cbItem];
    }
    const T_ITEM& get_at(size_t iItem) const
    {
        return *(T_ITEM*)&m_pb[iItem * m_cbItem];
    }

    void set_at(size_t iItem, const T_ITEM& item)
    {
        *(T_ITEM*)&m_pb[iItem * m_cbItem] = item;
    }

    T_ITEM* Append(size_t cGrow)
    {
        return (T_ITEM*)CicArray::Append(cGrow);
    }

    using CicArray::Insert;
    using CicArray::Remove;
};

/******************************************************************************/

inline CicArray::CicArray(size_t cbItem)
{
    m_cbItem = cbItem;
    m_pb = NULL;
    m_cItems = m_cCapacity = 0;
}

inline CicArray::~CicArray()
{
    cicMemFree(m_pb);
}

inline LPVOID CicArray::Append(size_t cGrow)
{
    if (!Insert(m_cItems, cGrow))
        return NULL;
    return &m_pb[(m_cItems - cGrow) * m_cbItem];
}

inline BOOL CicArray::Insert(size_t iItem, size_t cGrow)
{
    size_t cNewCapacity = m_cItems + cGrow;
    if (m_cCapacity < cNewCapacity)
    {
        if (cNewCapacity <= m_cItems + m_cItems / 2)
            cNewCapacity = m_cItems + m_cItems / 2;

        BYTE *pbNew;
        if (m_pb)
            pbNew = (BYTE *)cicMemReAlloc(m_pb, cNewCapacity * m_cbItem);
        else
            pbNew = (BYTE *)cicMemAlloc(cNewCapacity * m_cbItem);

        if (!pbNew)
            return FALSE;

        m_pb = pbNew;
        m_cCapacity = cNewCapacity;
    }

    if (iItem < m_cItems)
    {
        MoveMemory(&m_pb[(cGrow + iItem) * m_cbItem],
                   &m_pb[iItem * m_cbItem],
                   (m_cItems - iItem) * m_cbItem);
    }

    m_cItems += cGrow;
    return TRUE;
}

inline void CicArray::Remove(size_t iItem, size_t cRemove)
{
    if (iItem + cRemove < m_cItems)
    {
        MoveMemory(&m_pb[iItem * m_cbItem],
                   &m_pb[(iItem + cRemove) * m_cbItem],
                   (m_cItems - iItem - cRemove) * m_cbItem);
    }

    m_cItems -= cRemove;

    size_t cHalfCapacity = m_cCapacity / 2;
    if (cHalfCapacity > m_cItems)
    {
        BYTE *pb = (BYTE *)cicMemReAlloc(m_pb, cHalfCapacity * m_cbItem);
        if (pb)
        {
            m_pb = pb;
            m_cCapacity = cHalfCapacity;
        }
    }
}
