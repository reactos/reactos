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
protected:
    T_ITEM* m_pItems;
    size_t m_cItems;
    size_t m_iFirstItem;
    size_t m_iLastItem;

public:
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

    size_t GetSize() const
    {
        if (m_iFirstItem == m_iLastItem)
            return 0;
        if (m_iFirstItem < m_iLastItem)
            return m_iFirstItem + (m_cItems - m_iLastItem);
        return m_iFirstItem - m_iLastItem;
    }

    // First-In
    BOOL SetData(const T_ITEM* pItem)
    {
        if (!m_cItems || GetSize() + 1 >= m_cItems) /* "+1" is for marking */
        {
            if (!GrowBuffer(!m_cItems ? 8 : (2 * m_cItems)))
                return FALSE;
        }

        m_pItems[m_iFirstItem] = *pItem;

        if (++m_iFirstItem == m_cItems)
            m_iFirstItem = 0;

        return TRUE;
    }

    // First-Out
    BOOL GetData(T_ITEM* pItem)
    {
        if (m_iLastItem == m_iFirstItem)
            return FALSE;
        *pItem = m_pItems[m_iLastItem];
        ZeroMemory(&m_pItems[m_iLastItem], sizeof(T_ITEM));
        if (++m_iLastItem == m_cItems)
            m_iLastItem = 0;
        return TRUE;
    }

    BOOL GrowBuffer(size_t nGrow)
    {
        if (!nGrow)
            return TRUE;

        if (!m_pItems)
        {
            m_pItems = (T_ITEM*)cicMemAllocClear(nGrow * sizeof(T_ITEM));
            if (m_pItems)
                m_cItems = nGrow;
            return !!m_pItems;
        }

        size_t cNewItems = m_cItems + nGrow;
        T_ITEM* pNewItems = (T_ITEM*)cicMemAllocClear(cNewItems * sizeof(T_ITEM));
        if (!pNewItems)
            return FALSE;

        if (m_iFirstItem < m_iLastItem)
        {
            size_t cTail = m_cItems - m_iLastItem;
            CopyMemory(pNewItems, &m_pItems[m_iLastItem], cTail * sizeof(T_ITEM));
            CopyMemory(&pNewItems[cTail], m_pItems, m_iFirstItem * sizeof(T_ITEM));
            size_t cUsed = cTail + m_iFirstItem;
            m_iLastItem = 0;
            m_iFirstItem = cUsed;
        }
        else
        {
            CopyMemory(pNewItems, m_pItems, m_cItems * sizeof(T_ITEM));
        }

        cicMemFree(m_pItems);
        m_pItems = pNewItems;
        m_cItems = cNewItems;
        return TRUE;
    }

private:
    CicFirstInFirstOut(const CicFirstInFirstOut&) = delete;
    CicFirstInFirstOut& operator=(const CicFirstInFirstOut&) = delete;
};
