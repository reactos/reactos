/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero COM interface
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicbase.h"

template <typename T_IFACE>
class CicInterface_RefCnt
{
protected:
    T_IFACE* m_ptr;

public:
    CicInterface_RefCnt() : m_ptr(NULL) { }

    CicInterface_RefCnt(T_IFACE* ptr) : m_ptr(ptr)
    {
        if (m_ptr)
            m_ptr->AddRef();
    }

    ~CicInterface_RefCnt()
    {
        if (m_ptr)
            m_ptr->Release();
    }

    void Attach(T_IFACE* ptr)
    {
        if (m_ptr)
            m_ptr->Release();
        m_ptr = ptr;
    }

    T_IFACE* Detach()
    {
        T_IFACE* old_ptr = m_ptr;
        m_ptr = NULL;
        return old_ptr;
    }

    operator T_IFACE*() { return m_ptr; }
    T_IFACE* operator->() { return m_ptr; }

    T_IFACE** operator&()
    {
        cicAssert(!m_ptr);
        return &m_ptr;
    }

private:
    CicInterface_RefCnt(const CicInterface_RefCnt<T_IFACE>&) = delete;
    CicInterface_RefCnt<T_IFACE>& operator=(const CicInterface_RefCnt<T_IFACE>&) = delete;
};
