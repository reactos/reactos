/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero FIFO (First-In-First-Out)
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

// FIFO (First-In-First-Out) ring buffer
template <typename T_ITEM>
class CicFirstInFirstOut
{
protected:
    T_ITEM* m_pItems;
    size_t m_cItems;
    size_t m_iBack;
    size_t m_iFront;
    //static_assert(std::is_trivially_copyable<T_ITEM>::value, ""); // FIXME

public:
    CicFirstInFirstOut(size_t cInitial = 0)
        : m_pItems(NULL)
        , m_cItems(0)
        , m_iBack(0)
        , m_iFront(0)
    {
        if (cInitial)
            GrowBuffer(cInitial);
    }

    ~CicFirstInFirstOut()
    {
        cicMemFree(m_pItems);
    }

    size_t GetSize() const
    {
        if (m_iBack == m_iFront)
            return 0;
        if (m_iBack < m_iFront)
            return m_iBack + (m_cItems - m_iFront);
        return m_iBack - m_iFront;
    }

    // Like push_back
    BOOL SetData(const T_ITEM* pItem)
    {
        if (!m_cItems || GetSize() + 1 >= m_cItems) /* "+1" is for marking */
        {
            if (!GrowBuffer(!m_cItems ? 8 : m_cItems))
                return FALSE;
        }

        m_pItems[m_iBack] = *pItem;

        if (++m_iBack == m_cItems)
            m_iBack = 0;

        return TRUE;
    }

    // Like front and pop_front
    BOOL GetData(T_ITEM* pItem)
    {
        if (m_iFront == m_iBack)
            return FALSE;
        *pItem = m_pItems[m_iFront];
        if (++m_iFront == m_cItems)
            m_iFront = 0;
        return TRUE;
    }

protected:
    BOOL GrowBuffer(size_t nGrow)
    {
        if (nGrow == 0)
            return TRUE;

        if (!m_pItems)
        {
            m_pItems = (T_ITEM*)cicMemAlloc(nGrow * sizeof(T_ITEM));
            if (m_pItems)
                m_cItems = nGrow;
            return !!m_pItems;
        }

        size_t cNewItems = m_cItems + nGrow;
        if (cNewItems < m_cItems || cNewItems > ~(size_t)0 / sizeof(T_ITEM))
            return FALSE;

        T_ITEM* pNewItems = (T_ITEM*)cicMemAlloc(cNewItems * sizeof(T_ITEM));
        if (!pNewItems)
            return FALSE;

        if (m_iBack < m_iFront)
        {
            size_t cTail = m_cItems - m_iFront;
            CopyMemory(pNewItems, &m_pItems[m_iFront], cTail * sizeof(T_ITEM));
            CopyMemory(&pNewItems[cTail], m_pItems, m_iBack * sizeof(T_ITEM));
            m_iBack = cTail + m_iBack;
        }
        else
        {
            size_t nCount = m_iBack - m_iFront;
            CopyMemory(pNewItems, &m_pItems[m_iFront], nCount * sizeof(T_ITEM));
            m_iBack = nCount;
        }
        m_iFront = 0;

        cicMemFree(m_pItems);
        m_pItems = pNewItems;
        m_cItems = cNewItems;
        return TRUE;
    }

private:
    CicFirstInFirstOut(const CicFirstInFirstOut&) = delete;
    CicFirstInFirstOut& operator=(const CicFirstInFirstOut&) = delete;
};
