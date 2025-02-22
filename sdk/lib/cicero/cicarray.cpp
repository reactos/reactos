/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero dynamic array
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "cicarray.h"

CicArrayBase::CicArrayBase(size_t cbItem)
{
    m_cbItem = cbItem;
    m_pb = NULL;
    m_cItems = m_cCapacity = 0;
}

CicArrayBase::~CicArrayBase()
{
    cicMemFree(m_pb);
}

LPVOID CicArrayBase::Append(size_t cGrow)
{
    if (!Insert(m_cItems, cGrow))
        return NULL;
    return &m_pb[(m_cItems - cGrow) * m_cbItem];
}

BOOL CicArrayBase::Insert(size_t iItem, size_t cGrow)
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

void CicArrayBase::Remove(size_t iItem, size_t cRemove)
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
