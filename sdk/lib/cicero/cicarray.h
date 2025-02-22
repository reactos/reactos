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
    void Remove(size_t iItem, size_t cRemove = 1);
};

template <typename T_ITEM>
class CicArray : protected CicArrayBase
{
public:
    CicArray() : CicArrayBase(sizeof(T_ITEM)) { }

    T_ITEM* data() const { return (T_ITEM*)m_pb; }
    size_t size() const  { return m_cItems;      }
    bool empty() const   { return !size();       }
    void clear()
    {
        cicMemFree(m_pb);
        m_pb = NULL;
        m_cItems = m_cCapacity = 0;
    }

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

    BOOL Add(const T_ITEM& item)
    {
        T_ITEM *pItem = Append(1);
        if (!pItem)
            return FALSE;
        CopyMemory(pItem, &item, sizeof(T_ITEM));
        return TRUE;
    }

    ptrdiff_t Find(const T_ITEM& item) const
    {
        for (size_t iItem = 0; iItem < m_cItems; ++iItem)
        {
            if ((*this)[iItem] == item)
                return iItem;
        }
        return -1;
    }
};
