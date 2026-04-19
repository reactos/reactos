/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero FIFO (First-In-First-Out)
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

template <typename T_ITEM>
class CicFirstInFirstOut
{
    T_ITEM* m_pItems;
    INT_PTR m_cItems;
    INT_PTR m_iFirstItem;
    INT_PTR m_iLastItem;

    CicFirstInFirstOut()
        : m_pItems(NULL)
        , m_cItems(0)
        , m_iFirstItem(0)
        , m_iLastItem(0)
    {
    }

    ~CicFirstInFirstOut()
    {
        cicMemFree(m_pItems);
    }

    INT_PTR GetSize() const
    {
        if (m_iFirstItem == m_iLastItem)
            return 0;
        if (m_iFirstItem <= m_iLastItem)
            return m_iFirstItem + m_cItems - m_iLastItem;
        return m_iFirstItem - m_iLastItem;
    }

    BOOL GetData(T_ITEM* pItem)
    {
        if (m_iLastItem == m_iFirstItem)
            return FALSE;
        *pItem = m_pItems[m_iLastItem];
        if (++m_iLastItem == m_cItems)
            m_iLastItem = 0;
        return TRUE;
    }

    void GrowBuffer(INT_PTR nGrow)
    {
        if (!m_pItems)
        {
            m_pItems = (T_ITEM*)cicMemAllocClear(nGrow * sizeof(T_ITEM));
            if (m_pItems)
                m_cItems = nGrow;
            return;
        }

        INT_PTR cNewItems = m_cItems + nGrow;
        T_ITEM* pNewItems = cicMemAlloc(cNewItems * sizeof(T_ITEM));
        if (!pNewItems)
            return;

        CopyMemory(pNewItems, m_pItems, m_cItems * sizeof(T_ITEM));
        if (m_iLastItem > m_iFirstItem)
        {
            size_t cbCopy = (m_cItems - m_iLastItem) * sizeof(T_ITEM);
            CopyMemory(&pNewItems[nGrow + m_iLastItem], &m_pItems[m_iLastItem], cbCopy);
            ZeroMemory(&pNewItems[m_cItems], nGrow * sizeof(T_ITEM));
            m_iLastItem += nGrow;
        }

        cicMemFree(m_pItems);
        m_pItems = pNewItems;
        m_cItems = cNewItems;
    }

    INT_PTR SetData(T_ITEM* pItem)
    {
        if (!m_cItems || GetSize() + 1 >= m_cItems)
            GrowBuffer(3);

        m_pItems[m_iFirstItem] = *pItem;

        INT_PTR iItem = ++m_iFirstItem;
        if (iItem == m_cItems)
            m_iFirstItem = 0;
        return iItem;
    }
};
