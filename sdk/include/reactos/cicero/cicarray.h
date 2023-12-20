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
    LPVOID lpVtbl;
    LPBYTE m_pb;
    INT m_cItems;
    INT m_cbItem;
    INT m_cCapacity;

public:
    CicArray(INT cbItem);
    virtual CicArray();

    void Insert(INT iItem, INT cGrow);
    void Append(INT cGrow);
    void Remove(INT iItem, INT cRemove);
};

/******************************************************************************/

inline CicArray::CicArray(INT cbItem)
{
    m_cbItem = cbItem;
    m_pb = NULL;
    m_cItems = m_cCapacity = 0;
}

inline CicArray::~CicArray()
{
    cicMemFree(m_pb);
}

inline void CicArray::Append(INT cGrow)
{
    Insert(m_cItems, cGrow);
}

inline void CicArray::Insert(INT iItem, INT cGrow)
{
    INT cNewCapacity = m_cItems + cGrow;
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
            return;

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
}

inline void CicArray::Remove(INT iItem, INT cRemove)
{
    if (iItem + cRemove < m_cItems)
    {
        MoveMemory(&m_pb[iItem * m_cbItem],
                   &m_pb[(iItem + cRemove) * m_cbItem],
                   (m_cItems - iItem - cRemove) * m_cbItem);
    }

    m_cItems -= cRemove;

    INT cHalfCapacity = m_cCapacity / 2;
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
