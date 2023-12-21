/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero dynamic array
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

class CicArrayBase
{
protected:
    LPBYTE m_pb;
    size_t m_cItems, m_cbItem, m_cCapacity;

public:
    CicArrayBase(size_t cbItem);
    virtual ~CicArrayBase();

    BOOL Insert(size_t iItem, size_t cGrow);
    LPVOID Append(size_t cGrow);
    void Remove(size_t iItem, size_t cRemove);
};

template <typename T_ITEM>
class CicArray : protected CicArrayBase
{
public:
    CicArray() : CicArrayBase(sizeof(T_ITEM)) { }

    T_ITEM* data() const { return (T_ITEM*)m_pb; }
    size_t size() const  { return m_cItems;      }
    bool empty() const   { return !size();       }

    T_ITEM& operator[](size_t iItem)
    {
        return *(T_ITEM*)&m_pb[iItem * m_cbItem];
    }
    const T_ITEM& operator[](size_t iItem) const
    {
        return *(const T_ITEM*)&m_pb[iItem * m_cbItem];
    }

    T_ITEM* Append(size_t cGrow)
    {
        return (T_ITEM*)CicArrayBase::Append(cGrow);
    }

    using CicArrayBase::Insert;
    using CicArrayBase::Remove;
};

/******************************************************************************/

inline CicArrayBase::CicArrayBase(size_t cbItem)
{
    m_cbItem = cbItem;
    m_pb = NULL;
    m_cItems = m_cCapacity = 0;
}

inline CicArrayBase::~CicArrayBase()
{
    cicMemFree(m_pb);
}

inline LPVOID CicArrayBase::Append(size_t cGrow)
{
    if (!Insert(m_cItems, cGrow))
        return NULL;
    return &m_pb[(m_cItems - cGrow) * m_cbItem];
}

inline BOOL CicArrayBase::Insert(size_t iItem, size_t cGrow)
{
    size_t cNewCapacity = m_cItems + cGrow;
    if (m_cCapacity < cNewCapacity)
    {
        if (cNewCapacity <= m_cItems + m_cItems / 2)
            cNewCapacity = m_cItems + m_cItems / 2;

        LPBYTE pbNew = (LPBYTE)cicMemReAlloc(m_pb, cNewCapacity * m_cbItem);
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

inline void CicArrayBase::Remove(size_t iItem, size_t cRemove)
{
    if (iItem + cRemove < m_cItems)
    {
        MoveMemory(&m_pb[iItem * m_cbItem],
                   &m_pb[(iItem + cRemove) * m_cbItem],
                   (m_cItems - iItem - cRemove) * m_cbItem);
    }

    m_cItems -= cRemove;

    size_t cHalfCapacity = m_cCapacity / 2;
    if (cHalfCapacity <= m_cItems)
        return;

    LPBYTE pb = (LPBYTE)cicMemReAlloc(m_pb, cHalfCapacity * m_cbItem);
    if (pb)
    {
        m_pb = pb;
        m_cCapacity = cHalfCapacity;
    }
}
